#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import platform
import subprocess
import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
    from experiments.aggregation import aggregate
    from experiments.config import (
        DEFAULT_RUNS, REPOSITORY_ROOT, experiment_configurations, paper_suite,
    )
    from experiments.datasets import ensure_dataset, prepared_dataset_path, sha256_file
    from experiments.execution import ensure_utility, run_trial, stable_seed, utility_path
    from experiments.plotting import generate_plots
else:
    from .aggregation import aggregate
    from .config import DEFAULT_RUNS, REPOSITORY_ROOT, experiment_configurations, paper_suite
    from .datasets import ensure_dataset, prepared_dataset_path, sha256_file
    from .execution import ensure_utility, run_trial, stable_seed, utility_path
    from .plotting import generate_plots


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run and plot FHDR paper experiments")
    parser.add_argument("--part", choices=["p1", "p2", "internal"])
    parser.add_argument("--vary", help="Parameter to vary: d_int, n, d, q, K, m, or w")
    parser.add_argument("--dataset", default="synthetic",
                        help="synthetic, house, car, nba, energy, or real")
    parser.add_argument("--suite", choices=["paper"], help="Run the complete paper suite")
    parser.add_argument("--runs", type=int, default=DEFAULT_RUNS)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--timeout", type=int, default=3600,
                        help="Per-process timeout in seconds")
    parser.add_argument("--run-id", default="paper")
    parser.add_argument("--plot-only", action="store_true")
    parser.add_argument("--no-generate", action="store_true",
                        help="Fail instead of generating missing synthetic datasets")
    arguments = parser.parse_args()
    if not arguments.suite and (not arguments.part or not arguments.vary):
        parser.error("provide --part and --vary, or use --suite paper")
    if arguments.runs <= 0 or arguments.timeout <= 0:
        parser.error("--runs and --timeout must be positive")
    if arguments.vary and arguments.vary.lower() == "k":
        arguments.vary = "K"
    return arguments


def build_executables() -> None:
    subprocess.run(["make", "experiments"], cwd=REPOSITORY_ROOT, check=True)


def git_revision() -> str:
    completed = subprocess.run(
        ["git", "rev-parse", "HEAD"], cwd=REPOSITORY_ROOT,
        text=True, capture_output=True, check=False,
    )
    return completed.stdout.strip() if completed.returncode == 0 else "unknown"


def experiment_directory(result_root: Path, part: str, vary: str, dataset: str) -> Path:
    return result_root / part / dataset / f"vary_{vary}"


def run_experiment(
    part: str,
    vary: str,
    dataset_selector: str,
    result_root: Path,
    runs: int,
    master_seed: int,
    timeout: int,
    generate_missing: bool,
    plot_only: bool,
) -> list[Path]:
    configurations = experiment_configurations(part, vary, dataset_selector)
    output_directory = experiment_directory(result_root, part, vary, dataset_selector)
    raw_directory = output_directory / "raw"
    logs_directory = output_directory / "logs"

    if not plot_only:
        dataset_metadata = {}
        prepared_paths = {}
        for configuration in configurations:
            spec = configuration.dataset
            if spec.name in dataset_metadata:
                continue
            print(f"Preflight dataset {spec.name}: {spec.path}", flush=True)
            ensure_dataset(spec, generate_missing)
            prepared_path = prepared_dataset_path(spec, result_root)
            prepared_paths[spec.name] = prepared_path
            dataset_metadata[spec.name] = {
                **spec.serializable(),
                "sha256": sha256_file(spec.path),
                "prepared_path": str(prepared_path),
                "prepared_sha256": sha256_file(prepared_path),
            }

        manifest = {
            "part": part,
            "vary": vary,
            "dataset_selector": dataset_selector,
            "runs": runs,
            "master_seed": master_seed,
            "timeout_seconds": timeout,
            "git_revision": git_revision(),
            "platform": platform.platform(),
            "datasets": dataset_metadata,
            "configurations": [configuration.serializable() for configuration in configurations],
        }
        output_directory.mkdir(parents=True, exist_ok=True)
        manifest_path = output_directory / "manifest.json"
        if manifest_path.exists():
            existing = json.loads(manifest_path.read_text(encoding="utf-8"))
            comparable = {key: value for key, value in manifest.items() if key not in {"git_revision", "platform"}}
            existing_comparable = {key: value for key, value in existing.items() if key not in {"git_revision", "platform"}}
            if existing_comparable != comparable:
                raise RuntimeError(
                    f"existing run manifest differs at {manifest_path}; choose another --run-id"
                )
        else:
            manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

        completed_count = 0
        total_count = len(configurations) * runs
        for configuration in configurations:
            for trial in range(runs):
                trial_path = raw_directory / configuration.key / f"trial_{trial:03d}.json"
                if trial_path.exists():
                    completed_count += 1
                    continue
                utility_seed = stable_seed(
                    master_seed, "utility", configuration.dataset.dimension,
                    configuration.d_int, trial,
                )
                algorithm_seed = stable_seed(master_seed, "algorithm", configuration.key, trial)
                trial_utility_path = utility_path(result_root, configuration, trial)
                utility_sha256 = ensure_utility(
                    trial_utility_path, configuration.dataset.dimension,
                    configuration.d_int, utility_seed,
                )
                print(
                    f"[{completed_count + 1}/{total_count}] {configuration.key} trial {trial + 1}/{runs}",
                    flush=True,
                )
                records, log = run_trial(
                    configuration, trial, trial_utility_path, algorithm_seed, timeout,
                    prepared_paths[configuration.dataset.name],
                )
                configuration_data = configuration.serializable()
                configuration_data["key"] = configuration.key
                payload = {
                    "configuration": configuration_data,
                    "trial": trial,
                    "utility_seed": utility_seed,
                    "algorithm_seed": algorithm_seed,
                    "utility_sha256": utility_sha256,
                    "results": records,
                }
                trial_path.parent.mkdir(parents=True, exist_ok=True)
                temporary = trial_path.with_suffix(".tmp")
                temporary.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
                temporary.replace(trial_path)
                log_path = logs_directory / configuration.key / f"trial_{trial:03d}.log"
                log_path.parent.mkdir(parents=True, exist_ok=True)
                log_path.write_text(log, encoding="utf-8")
                completed_count += 1

    summaries = aggregate(raw_directory, output_directory)
    if not summaries:
        raise RuntimeError(f"no completed trials found in {raw_directory}")
    plots = generate_plots(summaries, output_directory, part, vary)
    print(f"Results: {output_directory}")
    for plot in plots:
        print(f"Plot: {plot}")
    return plots


def main() -> int:
    arguments = parse_arguments()
    if not arguments.plot_only:
        build_executables()
    result_root = REPOSITORY_ROOT / "results" / arguments.run_id
    experiments = paper_suite() if arguments.suite else [
        (arguments.part, arguments.vary, arguments.dataset)
    ]
    for part, vary, dataset in experiments:
        run_experiment(
            part, vary, dataset, result_root, arguments.runs, arguments.seed,
            arguments.timeout, not arguments.no_generate, arguments.plot_only,
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
