import hashlib
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from experiments.config import REPOSITORY_ROOT
from experiments.plotting import export_data_files, plot_spec


def gnuplot_checksums():
    return {
        path: hashlib.sha256(path.read_bytes()).hexdigest()
        for path in (REPOSITORY_ROOT / "plot").glob("**/*.gnu")
    }


class PlottingTest(unittest.TestCase):
    def test_existing_script_mapping(self):
        synthetic = plot_spec("p2", "K", "synthetic")
        self.assertEqual(synthetic.script, REPOSITORY_ROOT / "plot/synthetic/p2/K_plots.gnu")
        real = plot_spec("p1", "d_int", "car")
        self.assertEqual(real.script, REPOSITORY_ROOT / "plot/real/car/d_int_1_plots.gnu")

    def test_data_export_does_not_modify_gnuplot_scripts(self):
        before = gnuplot_checksums()
        rows = []
        for value in [2, 3, 4, 5]:
            for method, questions in [("FHDR", 30.0), ("UtilityApprox", 1100.0)]:
                rows.append({
                    "plot_dataset": "synthetic", "method": method,
                    "parameter_value": value, "questions": questions,
                    "time_seconds": 0.1, "outperformance_rate": float("nan"),
                })
        with tempfile.TemporaryDirectory() as directory, patch(
            "experiments.plotting.PLOT_ROOT", Path(directory)
        ):
            specs = export_data_files(rows, "p1", "d_int")
            self.assertTrue((specs[0].data_directory / "d_int_questions.dat").is_file())
        self.assertEqual(before, gnuplot_checksums())


if __name__ == "__main__":
    unittest.main()
