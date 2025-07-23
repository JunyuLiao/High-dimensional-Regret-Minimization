# High-dimensional Regret Minimization

This repository implements an **interactive k-regret minimization** algorithm tailored for **high-dimensional** datasets where only a small subset of attributes (dimensions) truly influence user utility. By progressively reducing dimensions through two user-guided phases, the algorithm efficiently narrows down to the relevant $d'$ features before executing a final regret-minimization step.

## Features

* **Two-Phase Dimension Reduction**

  * **Phase 1 (Coarse Elimination):** Quickly discards irrelevant attributes in blocks of size $\hat d$ via user feedback on small batches of tuples.
  * **Phase 2 (Fine Selection):** Pinpoints the exact set of $d'$ nonzero dimensions using an adaptive group-testing strategy and direct yes/no tests when only one candidate remains.
* **Interactive Queries**: Presents users with $s$-tuple forced-choice tasks (plus a "None apply" option) to ensure natural, low-cognitive-load responses.
* **Final Regret Minimization**: Runs a standard k-regret algorithm on the reduced $d'$-dimensional subspace, guaranteeing low worst-case regret ratio.
* **Robust to Partial Feedback**: Gracefully handles skipped or incomplete answers by jumping to the final stage with whatever candidate set is available.

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
├── script.py             # Main experimental 
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

To reproduce the experiments in the paper 'High-dimensional Regret Minimization', use 

python script.py <dataset_path> <d_prime> <d_hat_1> <d_hat_2> <rounds>

which will run the code 100 times and report the average performance, with the parameters above and all other parameters as default.
