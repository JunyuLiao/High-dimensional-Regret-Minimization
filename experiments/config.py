from __future__ import annotations

from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]

DEFAULT_D_INT = 3
DEFAULT_M = 7
DEFAULT_W = 6
DEFAULT_K = 30
DEFAULT_SYNTHETIC_Q = 15
DEFAULT_RUNS = 100
P1_QUESTION_BUDGET = 100
UTILITY_APPROX_P1_MAX_ROUNDS = 10_000

P1_VALUES = {
    "d_int": [2, 3, 4, 5],
    "n": [100, 1_000, 10_000, 100_000, 1_000_000],
    "d": [10, 50, 100, 200, 300, 400, 500],
}

P2_VALUES = {
    "d_int": [2, 3, 4, 5],
    "K": [20, 30, 40, 50, 60],
    "q": [15, 18, 21, 24, 27, 30, 33, 36],
    "n": [100, 1_000, 10_000, 100_000, 1_000_000],
    "d": [10, 30, 50, 70, 100, 200, 500],
}

REAL_Q_VALUES = {
    "house": [5, 9, 12, 15, 18, 21, 24],
    "car": [9, 12, 15, 18, 21, 24, 28],
    "nba": [15, 18, 21, 24, 27, 30, 33, 36],
    "energy": [22, 24, 27, 30, 33, 36, 39, 42],
}


@dataclass(frozen=True)
class DatasetSpec:
    name: str
    path: Path
    size: int
    dimension: int
    synthetic: bool = False

    def serializable(self) -> dict:
        value = asdict(self)
        value["path"] = str(self.path)
        return value


@dataclass(frozen=True)
class TrialConfiguration:
    part: str
    vary: str
    dataset: DatasetSpec
    parameter_value: int
    d_int: int
    m: int
    w: int
    output_size: int
    question_budget: int
    series_value: int | None = None

    @property
    def key(self) -> str:
        pieces = [self.dataset.name, f"{self.vary}_{self.parameter_value}"]
        if self.series_value is not None:
            pieces.append(f"d_int_{self.series_value}")
        return "__".join(pieces)

    def serializable(self) -> dict:
        value = asdict(self)
        value["dataset"] = self.dataset.serializable()
        return value


REAL_DATASETS = {
    "house": DatasetSpec("house", REPOSITORY_ROOT / "datasets/real/house.txt", 15_171, 33),
    "car": DatasetSpec("car", REPOSITORY_ROOT / "datasets/real/car.txt", 4_278, 57),
    "nba": DatasetSpec("nba", REPOSITORY_ROOT / "datasets/real/nba.txt", 4_790, 104),
    "energy": DatasetSpec("energy", REPOSITORY_ROOT / "datasets/real/energy.txt", 36_043, 149),
}


def synthetic_dataset(size: int = 100_000, dimension: int = 100) -> DatasetSpec:
    if dimension == 100 and size == 10_000:
        path = REPOSITORY_ROOT / "datasets/e100-10k.txt"
    elif dimension == 100 and size == 100_000:
        path = REPOSITORY_ROOT / "datasets/e100-100k.txt"
    elif dimension == 100:
        path = REPOSITORY_ROOT / f"datasets/synthetic/n/e100-{size}.txt"
    else:
        path = REPOSITORY_ROOT / f"datasets/synthetic/d/e{dimension}-100k.txt"
    return DatasetSpec(f"synthetic_n{size}_d{dimension}", path, size, dimension, True)


def default_question_budget(dataset: DatasetSpec) -> int:
    return (dataset.dimension + DEFAULT_M - 1) // DEFAULT_M


def selected_real_datasets(selector: str) -> Iterable[DatasetSpec]:
    if selector in {"real", "all-real"}:
        return REAL_DATASETS.values()
    if selector not in REAL_DATASETS:
        raise ValueError(f"unknown real dataset: {selector}")
    return [REAL_DATASETS[selector]]


def _normal_configurations(part: str, vary: str, dataset: DatasetSpec) -> list[TrialConfiguration]:
    values = P1_VALUES[vary] if part == "p1" else (
        REAL_Q_VALUES[dataset.name] if vary == "q" and not dataset.synthetic else P2_VALUES[vary]
    )
    configurations = []
    for value in values:
        active_dataset = dataset
        if vary == "n":
            active_dataset = synthetic_dataset(size=value, dimension=100)
        elif vary == "d":
            active_dataset = synthetic_dataset(size=100_000, dimension=value)

        d_int = value if vary == "d_int" else DEFAULT_D_INT
        output_size = 1 if part == "p1" else (value if vary == "K" else DEFAULT_K)
        if part == "p1":
            question_budget = P1_QUESTION_BUDGET
        elif vary == "q":
            question_budget = value
        elif active_dataset.synthetic:
            question_budget = DEFAULT_SYNTHETIC_Q
        else:
            question_budget = default_question_budget(active_dataset)

        configurations.append(TrialConfiguration(
            part=part,
            vary=vary,
            dataset=active_dataset,
            parameter_value=value,
            d_int=d_int,
            m=DEFAULT_M,
            w=DEFAULT_W,
            output_size=output_size,
            question_budget=question_budget,
        ))
    return configurations


def experiment_configurations(part: str, vary: str, dataset_selector: str) -> list[TrialConfiguration]:
    if part == "internal":
        return internal_configurations(vary)

    if part not in {"p1", "p2"}:
        raise ValueError(f"unknown experiment part: {part}")
    allowed = P1_VALUES if part == "p1" else P2_VALUES
    if vary not in allowed:
        raise ValueError(f"{part} does not define a {vary} sweep")

    if dataset_selector == "synthetic":
        datasets = [synthetic_dataset()]
    else:
        if vary in {"n", "d"}:
            raise ValueError(f"{vary} scalability experiments are synthetic-only")
        datasets = selected_real_datasets(dataset_selector)

    configurations: list[TrialConfiguration] = []
    for dataset in datasets:
        configurations.extend(_normal_configurations(part, vary, dataset))
    return configurations


def internal_configurations(vary: str) -> list[TrialConfiguration]:
    if vary not in {"m", "w"}:
        raise ValueError("internal experiments support only m and w")
    configurations = []
    datasets = [REAL_DATASETS["nba"]] if vary == "m" else [
        REAL_DATASETS["car"], REAL_DATASETS["nba"], REAL_DATASETS["energy"]
    ]
    for dataset in datasets:
        for d_int in [2, 3, 4, 5]:
            for value in range(5, 11):
                configurations.append(TrialConfiguration(
                    part="internal",
                    vary=vary,
                    dataset=dataset,
                    parameter_value=value,
                    d_int=d_int,
                    m=value if vary == "m" else DEFAULT_M,
                    w=value if vary == "w" else DEFAULT_W,
                    output_size=1 if vary == "m" else DEFAULT_K,
                    question_budget=P1_QUESTION_BUDGET if vary == "m" else dataset.dimension // DEFAULT_M,
                    series_value=d_int,
                ))
    return configurations


def paper_suite() -> list[tuple[str, str, str]]:
    suite = [
        ("p1", "d_int", "synthetic"),
        ("p1", "n", "synthetic"),
        ("p1", "d", "synthetic"),
        ("p2", "q", "synthetic"),
        ("p2", "K", "synthetic"),
        ("p2", "d_int", "synthetic"),
        ("p2", "n", "synthetic"),
        ("p2", "d", "synthetic"),
    ]
    for dataset in REAL_DATASETS:
        suite.extend([
            ("p1", "d_int", dataset),
            ("p2", "d_int", dataset),
            ("p2", "K", dataset),
            ("p2", "q", dataset),
        ])
    suite.extend([("internal", "m", "nba"), ("internal", "w", "real")])
    return suite


def plot_metrics(part: str, vary: str) -> list[str]:
    if part == "p1":
        return ["questions", "time_seconds"]
    if part == "internal":
        return ["questions"] if vary == "m" else ["regret_ratio", "time_seconds"]
    return ["regret_ratio", "output_size", "outperformance_rate"] if vary == "q" else [
        "regret_ratio", "time_seconds", "outperformance_rate"
    ]
