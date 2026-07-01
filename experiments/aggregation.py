from __future__ import annotations

import csv
import json
import math
import statistics
from collections import defaultdict
from pathlib import Path


METRIC_FIELDS = ["regret_ratio", "time_seconds", "output_size", "questions"]


def load_trials(raw_directory: Path) -> list[dict]:
    trials = []
    for path in sorted(raw_directory.glob("**/trial_*.json")):
        with path.open("r", encoding="utf-8") as source:
            trials.append(json.load(source))
    return trials


def fhdr_wins(results: list[dict]) -> bool:
    successful = {result["method"]: result for result in results if result.get("status") == "ok"}
    fhdr = successful.get("FHDR")
    if fhdr is None:
        return False
    for method in ("Sphere-Adapt", "UtilityApprox"):
        competitor = successful.get(method)
        if competitor is None:
            continue
        regret_difference = float(fhdr["regret_ratio"]) - float(competitor["regret_ratio"])
        if regret_difference > 1e-12:
            return False
        if abs(regret_difference) <= 1e-12 and float(fhdr["time_seconds"]) >= float(competitor["time_seconds"]):
            return False
    return True


def aggregate(raw_directory: Path, output_directory: Path) -> list[dict]:
    trials = load_trials(raw_directory)
    grouped: dict[tuple[str, str], list[dict]] = defaultdict(list)
    wins: dict[str, list[bool]] = defaultdict(list)

    raw_rows = []
    for trial in trials:
        configuration = trial["configuration"]
        key = configuration["key"]
        if configuration["part"] != "p1" and not (
            configuration["part"] == "internal" and configuration["vary"] == "m"
        ):
            wins[key].append(fhdr_wins(trial["results"]))
        for result in trial["results"]:
            grouped[(key, result["method"])].append(result)
            raw_rows.append({
                "configuration": key,
                "dataset": configuration["dataset"]["name"],
                "parameter_value": configuration["parameter_value"],
                "series_value": configuration.get("series_value"),
                "trial": trial["trial"],
                "utility_seed": trial["utility_seed"],
                "algorithm_seed": trial["algorithm_seed"],
                "utility_sha256": trial["utility_sha256"],
                **result,
            })

    output_directory.mkdir(parents=True, exist_ok=True)
    raw_fields = [
        "configuration", "dataset", "parameter_value", "series_value", "trial",
        "utility_seed", "algorithm_seed", "utility_sha256", "method", "status",
        "reason", *METRIC_FIELDS,
    ]
    with (output_directory / "raw.csv").open("w", newline="", encoding="utf-8") as target:
        writer = csv.DictWriter(target, fieldnames=raw_fields, extrasaction="ignore")
        writer.writeheader()
        for row in raw_rows:
            writer.writerow(row)

    summaries = []
    configurations = {trial["configuration"]["key"]: trial["configuration"] for trial in trials}
    for (key, method), results in sorted(grouped.items()):
        configuration = configurations[key]
        successful = [result for result in results if result.get("status") == "ok"]
        row = {
            "configuration": key,
            "dataset": configuration["dataset"]["name"],
            "plot_dataset": "synthetic" if configuration["dataset"]["synthetic"] else configuration["dataset"]["name"],
            "parameter_value": configuration["parameter_value"],
            "series_value": configuration.get("series_value"),
            "method": method,
            "successful_runs": len(successful),
            "total_runs": len(results),
            "outperformance_rate": statistics.fmean(wins[key]) if method == "FHDR" and wins[key] else math.nan,
        }
        for metric in METRIC_FIELDS:
            values = [float(result[metric]) for result in successful if metric in result]
            row[metric] = statistics.fmean(values) if values else math.nan
            row[f"{metric}_stdev"] = statistics.stdev(values) if len(values) > 1 else 0.0 if values else math.nan
        summaries.append(row)

    summary_fields = [
        "configuration", "dataset", "plot_dataset", "parameter_value", "series_value",
        "method", "successful_runs", "total_runs", *METRIC_FIELDS,
        *[f"{metric}_stdev" for metric in METRIC_FIELDS], "outperformance_rate",
    ]
    with (output_directory / "summary.csv").open("w", newline="", encoding="utf-8") as target:
        writer = csv.DictWriter(target, fieldnames=summary_fields)
        writer.writeheader()
        writer.writerows(summaries)
    return summaries
