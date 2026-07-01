# UtilityApprox

This directory contains the UtilityApprox (`util`) algorithm. Datasets are loaded from
the repository-level `datasets/` directory.
It has no third-party library dependencies.

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Run

```sh
./build/ExistingAlg NUM_REPEAT ALG_NAME INPUT D_PRIME W MAXROUND EPSILON
```

`ALG_NAME` must be `util`. `INPUT` may be an absolute path, a path relative to the
current working directory, or a path relative to `datasets/`. `D_PRIME` controls the
number of nonzero utility dimensions, `W` is the return size, `MAXROUND` is the
question limit, and `EPSILON` is the UtilityApprox error threshold. Results are
written to `results/`, independent of the launch directory. The output contains only
average question count, return size, time in seconds, and regret ratio.

Example:

```sh
./build/ExistingAlg 1 util e100-10k.txt 3 1 30 1.0
```

The repository-level experiment runner also uses a single-trial machine interface:

```sh
./build/ExistingAlg --experiment INPUT D_PRIME W MAXROUND EPSILON UTILITY_FILE
```

This mode reads the exact utility vector from `UTILITY_FILE`, applies the same
normalization as FHDR, and emits a full-precision `EXPERIMENT_RESULT` record. It is
intended for `experiments/run.py`; the positional interface above remains supported.
