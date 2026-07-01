from __future__ import annotations

import hashlib
import json
import random
import subprocess
from dataclasses import dataclass
from pathlib import Path

from .config import REPOSITORY_ROOT, TrialConfiguration, UTILITY_APPROX_P1_MAX_ROUNDS


RESULT_PREFIX = "EXPERIMENT_RESULT "


@dataclass(frozen=True)
class ProcessResult:
    records: list[dict]
    stdout: str
    stderr: str


def stable_seed(master_seed: int, *parts: object) -> int:
    text = ":".join(str(part) for part in (master_seed, *parts))
    return int.from_bytes(hashlib.sha256(text.encode()).digest()[:4], "big")


def utility_path(result_root: Path, configuration: TrialConfiguration, trial: int) -> Path:
    key = f"d{configuration.dataset.dimension}_dint{configuration.d_int}"
    return result_root / "utilities" / configuration.dataset.name / key / f"trial_{trial:03d}.txt"


def ensure_utility(path: Path, dimension: int, d_int: int, seed: int) -> str:
    if not path.exists():
        generator = random.Random(seed)
        selected = generator.sample(range(dimension), d_int)
        weights = [generator.random() for _ in selected]
        total = sum(weights)
        if total == 0.0:
            weights[0] = total = 1.0
        utility = [0.0] * dimension
        for index, weight in zip(selected, weights):
            utility[index] = weight / total
        path.parent.mkdir(parents=True, exist_ok=True)
        temporary = path.with_suffix(".tmp")
        temporary.write_text(" ".join(format(value, ".17g") for value in utility) + "\n")
        temporary.replace(path)
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_records(output: str) -> list[dict]:
    records = []
    for line in output.splitlines():
        if line.startswith(RESULT_PREFIX):
            records.append(json.loads(line[len(RESULT_PREFIX):]))
    return records


def run_process(command: list[str], timeout: int) -> ProcessResult:
    try:
        completed = subprocess.run(
            command, cwd=REPOSITORY_ROOT, text=True, capture_output=True,
            timeout=timeout, check=False,
        )
    except subprocess.TimeoutExpired as error:
        stdout = error.stdout.decode() if isinstance(error.stdout, bytes) else (error.stdout or "")
        stderr = error.stderr.decode() if isinstance(error.stderr, bytes) else (error.stderr or "")
        return ProcessResult([], stdout, stderr + f"\nTIMEOUT after {timeout} seconds")
    if completed.returncode != 0:
        return ProcessResult([], completed.stdout, completed.stderr + f"\nEXIT {completed.returncode}")
    return ProcessResult(parse_records(completed.stdout), completed.stdout, completed.stderr)


def baseline_policy(configuration: TrialConfiguration) -> tuple[bool, bool]:
    if configuration.part == "p1" or (configuration.part == "internal" and configuration.vary == "m"):
        return True, False
    if configuration.part == "internal" and configuration.vary == "w":
        return True, True
    if configuration.vary == "n" and configuration.parameter_value == 1_000_000:
        return True, False
    if configuration.vary == "d" and configuration.parameter_value > 100:
        return True, True
    return False, False


def run_trial(
    configuration: TrialConfiguration,
    trial: int,
    utility_file: Path,
    algorithm_seed: int,
    timeout: int,
    dataset_path: Path | None = None,
) -> tuple[list[dict], str]:
    skip_sphere, skip_utility_approx = baseline_policy(configuration)
    active_dataset_path = dataset_path or configuration.dataset.path
    fhdr_command = [
        str(REPOSITORY_ROOT / "run"), "--experiment", str(active_dataset_path),
        str(configuration.d_int), str(configuration.m), str(configuration.w),
        str(configuration.output_size), str(configuration.question_budget),
        str(utility_file), str(algorithm_seed), "1" if skip_sphere else "0",
    ]
    fhdr_process = run_process(fhdr_command, timeout)
    fhdr = next((record for record in fhdr_process.records if record["method"] == "FHDR"), None)
    if fhdr is None:
        raise RuntimeError(
            f"FHDR trial failed for {configuration.key}, trial {trial}\n"
            f"stdout:\n{fhdr_process.stdout}\nstderr:\n{fhdr_process.stderr}"
        )

    records = list(fhdr_process.records)
    log = "$ " + " ".join(fhdr_command) + "\n" + fhdr_process.stdout + fhdr_process.stderr

    if skip_utility_approx:
        records.append({
            "method": "UtilityApprox", "status": "unavailable",
            "reason": "not_part_of_experiment" if configuration.part == "internal" else
                "paper_infeasible_for_limited_feedback_high_dimension",
        })
        return records, log

    utility_max_rounds = UTILITY_APPROX_P1_MAX_ROUNDS if configuration.part == "p1" or (
        configuration.part == "internal" and configuration.vary == "m"
    ) else configuration.question_budget
    utility_command = [
        str(REPOSITORY_ROOT / "ExistingAlg/build/ExistingAlg"), "--experiment",
        str(active_dataset_path), str(configuration.d_int),
        str(int(fhdr["output_size"])), str(utility_max_rounds), "0", str(utility_file),
    ]
    utility_process = run_process(utility_command, timeout)
    utility = next((record for record in utility_process.records if record["method"] == "UtilityApprox"), None)
    if utility is None:
        utility = {"method": "UtilityApprox", "status": "unavailable", "reason": "process_failed_or_timed_out"}
    records.append(utility)
    log += "\n$ " + " ".join(utility_command) + "\n" + utility_process.stdout + utility_process.stderr
    return records, log
