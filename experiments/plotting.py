from __future__ import annotations

import math
import re
import shutil
import subprocess
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

from .config import REPOSITORY_ROOT


PLOT_ROOT = REPOSITORY_ROOT / "plot"
P1_METHODS = ["FHDR", "UtilityApprox"]
P2_METHODS = ["FHDR", "Sphere-Adapt", "UtilityApprox"]


@dataclass(frozen=True)
class PlotSpec:
    script: Path
    working_directory: Path
    data_directory: Path


def plot_spec(part: str, vary: str, dataset: str) -> PlotSpec:
    if part == "internal":
        directory = PLOT_ROOT / "synthetic/internal"
        return PlotSpec(directory / f"{vary}.gnu", directory, directory / "data")

    if dataset == "synthetic":
        directory = PLOT_ROOT / f"synthetic/{part}"
        return PlotSpec(directory / f"{vary}_plots.gnu", PLOT_ROOT, directory / vary)

    directory = PLOT_ROOT / f"real/{dataset}"
    if vary == "d_int":
        suffix = "1" if part == "p1" else "2"
        return PlotSpec(directory / f"d_int_{suffix}_plots.gnu", PLOT_ROOT, directory / f"d_int_{suffix}")
    return PlotSpec(directory / f"{vary}_plots.gnu", PLOT_ROOT, directory / vary)


def _number(value: float) -> str:
    return "NaN" if math.isnan(value) else format(value, ".17g")


def _write_method_data(path: Path, rows: list[dict], metric: str, methods: list[str]) -> None:
    values: dict[str, dict[int, float]] = defaultdict(dict)
    for row in rows:
        values[row["method"]][int(row["parameter_value"])] = float(row[metric])
    x_values = sorted({value for method in methods for value in values[method]})
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as target:
        target.write("#value " + " ".join(methods) + "\n")
        for value in x_values:
            target.write(str(value))
            for method in methods:
                target.write(" " + _number(values[method].get(value, math.nan)))
            target.write("\n")


def _write_outperformance_data(path: Path, rows: list[dict]) -> None:
    fhdr_rows = sorted(
        (row for row in rows if row["method"] == "FHDR"),
        key=lambda row: int(row["parameter_value"]),
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as target:
        target.write("#value FHDR\n")
        for row in fhdr_rows:
            target.write(f"{row['parameter_value']} {_number(float(row['outperformance_rate']))}\n")


def _write_internal_data(path: Path, rows: list[dict], metric: str) -> None:
    values: dict[int, dict[int, float]] = defaultdict(dict)
    for row in rows:
        if row["method"] == "FHDR" and row["series_value"] is not None:
            values[int(row["series_value"])][int(row["parameter_value"])] = float(row[metric])
    series = [2, 3, 4, 5]
    x_values = sorted({value for d_int in series for value in values[d_int]})
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as target:
        target.write("#value d_int_2 d_int_3 d_int_4 d_int_5\n")
        for value in x_values:
            target.write(str(value))
            for d_int in series:
                target.write(" " + _number(values[d_int].get(value, math.nan)))
            target.write("\n")


def export_data_files(summaries: list[dict], part: str, vary: str) -> list[PlotSpec]:
    grouped: dict[str, list[dict]] = defaultdict(list)
    for row in summaries:
        grouped[row["plot_dataset"]].append(row)

    specs = []
    if part == "internal":
        spec = plot_spec(part, vary, "real")
        if vary == "m":
            _write_internal_data(spec.data_directory / "m_q.dat", grouped["nba"], "questions")
        else:
            for dataset in ("car", "nba", "energy"):
                _write_internal_data(
                    spec.data_directory / f"w_rr_{dataset}.dat", grouped[dataset], "regret_ratio"
                )
            _write_internal_data(spec.data_directory / "w_time.dat", grouped["energy"], "time_seconds")
        specs.append(spec)
        return specs

    methods = P1_METHODS if part == "p1" else P2_METHODS
    for dataset, rows in grouped.items():
        spec = plot_spec(part, vary, dataset)
        prefix = vary
        if part == "p1":
            _write_method_data(spec.data_directory / f"{prefix}_questions.dat", rows, "questions", methods)
            _write_method_data(spec.data_directory / f"{prefix}_time.dat", rows, "time_seconds", methods)
        else:
            _write_method_data(spec.data_directory / f"{prefix}_rr.dat", rows, "regret_ratio", methods)
            if vary == "q":
                _write_method_data(spec.data_directory / f"{prefix}_size.dat", rows, "output_size", methods)
            else:
                _write_method_data(spec.data_directory / f"{prefix}_time.dat", rows, "time_seconds", methods)
            _write_outperformance_data(spec.data_directory / f"{prefix}_outperformance_rate.dat", rows)
        specs.append(spec)
    return specs


def _output_path(spec: PlotSpec) -> Path:
    match = re.search(r'^\s*set\s+out\s+"([^"]+)"', spec.script.read_text(encoding="utf-8"), re.MULTILINE)
    if match is None:
        raise RuntimeError(f"cannot determine output path from {spec.script}")
    return spec.working_directory / match.group(1)


def invoke_existing_scripts(specs: list[PlotSpec]) -> list[Path]:
    if shutil.which("gnuplot") is None:
        raise RuntimeError("gnuplot is required to generate experiment plots")
    outputs = []
    for spec in specs:
        if not spec.script.is_file():
            raise RuntimeError(f"missing existing Gnuplot script: {spec.script}")
        subprocess.run(["gnuplot", str(spec.script)], cwd=spec.working_directory, check=True)
        eps_path = _output_path(spec)
        outputs.append(eps_path)
        if shutil.which("epstopdf") is not None and eps_path.suffix == ".eps":
            pdf_path = eps_path.with_suffix(".pdf")
            subprocess.run(["epstopdf", str(eps_path), f"--outfile={pdf_path}"], check=True)
            outputs.append(pdf_path)
    return outputs


def generate_plots(summaries: list[dict], output_directory: Path, part: str, vary: str) -> list[Path]:
    del output_directory
    return invoke_existing_scripts(export_data_files(summaries, part, vary))
