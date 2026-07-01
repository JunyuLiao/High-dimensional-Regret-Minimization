from __future__ import annotations

import hashlib
import subprocess
from pathlib import Path

from .config import DatasetSpec, REPOSITORY_ROOT


class DatasetError(RuntimeError):
    pass


def read_header(path: Path) -> tuple[int, int]:
    try:
        with path.open("r", encoding="utf-8") as source:
            fields = source.readline().split()
    except OSError as error:
        raise DatasetError(f"cannot read dataset {path}: {error}") from error
    if len(fields) != 2:
        raise DatasetError(f"invalid dataset header in {path}")
    try:
        return int(fields[0]), int(fields[1])
    except ValueError as error:
        raise DatasetError(f"non-integer dataset header in {path}") from error


def dataset_seed(spec: DatasetSpec) -> int:
    digest = hashlib.sha256(f"uniform:{spec.size}:{spec.dimension}".encode()).digest()
    return int.from_bytes(digest[:8], "big")


def ensure_dataset(spec: DatasetSpec, generate_missing: bool = True) -> None:
    if not spec.path.exists():
        if not spec.synthetic or not generate_missing:
            raise DatasetError(f"missing dataset: {spec.path}")
        generator = REPOSITORY_ROOT / "generate_uniform"
        if not generator.exists():
            raise DatasetError("generate_uniform is not built; run `make experiments`")
        subprocess.run([
            str(generator), str(spec.path), str(spec.size), str(spec.dimension),
            str(dataset_seed(spec)),
        ], cwd=REPOSITORY_ROOT, check=True)

    actual_size, actual_dimension = read_header(spec.path)
    if (actual_size, actual_dimension) != (spec.size, spec.dimension):
        raise DatasetError(
            f"dataset manifest mismatch for {spec.path}: expected "
            f"({spec.size}, {spec.dimension}), found ({actual_size}, {actual_dimension})"
        )


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        while chunk := source.read(1024 * 1024):
            digest.update(chunk)
    return digest.hexdigest()


def prepared_dataset_path(spec: DatasetSpec, result_root: Path) -> Path:
    if not spec.synthetic or spec.dimension > 20:
        return spec.path
    source_digest = sha256_file(spec.path)
    prepared = result_root / "prepared_datasets" / f"{spec.name}_{source_digest[:12]}.txt"
    if prepared.exists():
        return prepared
    executable = REPOSITORY_ROOT / "prepare_dataset"
    if not executable.exists():
        raise DatasetError("prepare_dataset is not built; run `make experiments`")
    subprocess.run([str(executable), str(spec.path), str(prepared)], check=True)
    return prepared
