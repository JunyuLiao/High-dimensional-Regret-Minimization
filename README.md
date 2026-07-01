# High-dimensional Regret Minimization

[![arXiv](https://img.shields.io/badge/arXiv-2512.24078-b31b1b.svg)](https://arxiv.org/abs/2512.24078)

This repository implements an **interactive k-regret minimization** algorithm tailored for **high-dimensional** datasets where only a small subset of attributes (dimensions) truly influence user utility. By progressively reducing dimensions through two user-guided phases, the algorithm efficiently narrows down to the relevant features before executing a final regret-minimization step.

## Features

* **Two-Phase Dimension Reduction**

  * **Phase 1 (Coarse Elimination):** Quickly discards irrelevant attributes in blocks.
  * **Phase 2 (Fine Selection):** Pinpoints the key dimensions (the dimensions with non-zero weights) using an adaptive group-testing strategy.
* **Best Point Determination**: Returns the user their favorite option, with much fewer questions needed compared to existing works.
* **Robust to Partial Feedback**: Gracefully handles skipped or incomplete answers using whatever candidate set is available, with our proposed _AttributeSubset_ algorithm.

## Repository Structure

```
├── datasets/              # Example datasets
├── other/                 # Helper code files
│   ├── data_struct.h      # Data structures
│   ├── data_utility.cpp   # Utility functions
│   ├── sphere.cpp         # Sphere algorithm 
│   ├── operation.cpp      # Core operations
│   ├── lp.cpp            # Linear programming 
│   └── ...               # Other helper files
├── main.cpp              # Main entry point
├── highdim.cpp           # Core high-dimensional 
├── highdim.h             # High-dimensional 
├── attribute_subset.cpp  # Attribute subset 
├── attribute_subset.h    # Attribute subset 
├── Makefile              # Build configuration
└── README.md             # This documentation
```

## Installation

1. Install the GLPK package (for solving LPs) first. See GLPK webpage http://www.gnu.org/software/glpk/glpk.html. The Makefile should handle all compilation (once you get GLPK installed correctly). Note that "-lglpk" in the Makefile is the GLPK package and you may need to change it to the location where you installed the GLPK package.
2. Run 'make' to compile code.

## Usage

### Prepare your dataset

* Ensure your data is in a .txt format with numeric columns for each attribute. The first line should describe the number of tuples and dimensions (e.g., "10000 100" for a dataset of 10k tuples with 100 dimensions each)

## Reproducibility:

To run the algorithm in the paper 'High-dimensional Regret Minimization', use 

./run <dataset_path> <d_int> <m> <w> <K> <q>

For example, "./run datasets/e100-10k.txt 3 7 6 1 35" will run the algorithm on synthetic dataset of n=10k, d=100, with 35 questions allowed and output size 1. The output also contains results from _Sphere-Adapt_ (as introduced in our paper) for comparison, if it can be executed normally.

Note that due to the size limit of Github, the standard synthetic dataset used in our experiments (n=100k, d=100) is not provided. 

## End-to-end experiments

Prerequisites are GLPK, CMake, Python 3.10 or newer, and Gnuplot. Run one experiment with:

```sh
python3 experiments/run.py \
  --part <p1|p2|internal> \
  --vary <parameter> \
  --dataset <dataset> \
  --runs <number-of-trials> \
  --run-id <result-name>
```

Datasets are `synthetic`, `house`, `car`, `nba`, and `energy`. Parameters are:

- P1: `d_int`, `n`, `d`
- P2: `d_int`, `K`, `q`, `n`, `d`
- Internal: `m`, `w`

Examples:

```sh
# P2 on Car, varying K, with 100 trials per value
python3 experiments/run.py --part p2 --vary K --dataset car \
  --runs 100 --run-id p2-car-k

# Small P1 validation run
python3 experiments/run.py --part p1 --vary d_int --dataset synthetic \
  --runs 2 --run-id smoke

# Internal parameter experiment
python3 experiments/run.py --part internal --vary m --dataset nba \
  --runs 100 --run-id internal-m
```

The default is 100 trials. Reusing the same `--run-id` resumes an interrupted run.
Use a different run ID when changing the trial count, seed, or timeout. To regenerate
plots from completed results without rerunning algorithms, add `--plot-only`.

Results are written to:

```text
results/<run-id>/<part>/<dataset>/vary_<parameter>/
```

The runner also writes `.dat` files under `plot/`, invokes the existing `.gnu` script,
and generates EPS and PDF plots. Run `python3 experiments/run.py --help` for all options.
