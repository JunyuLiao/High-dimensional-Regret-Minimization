import json
import tempfile
import unittest
from pathlib import Path

from experiments.aggregation import aggregate, fhdr_wins


class AggregationTest(unittest.TestCase):
    def test_outperformance_uses_time_to_break_regret_tie(self):
        self.assertTrue(fhdr_wins([
            {"method": "FHDR", "status": "ok", "regret_ratio": 0.1, "time_seconds": 1.0},
            {"method": "Sphere-Adapt", "status": "ok", "regret_ratio": 0.1, "time_seconds": 2.0},
            {"method": "UtilityApprox", "status": "ok", "regret_ratio": 0.2, "time_seconds": 0.1},
        ]))
        self.assertFalse(fhdr_wins([
            {"method": "FHDR", "status": "ok", "regret_ratio": 0.1, "time_seconds": 2.0},
            {"method": "Sphere-Adapt", "status": "ok", "regret_ratio": 0.1, "time_seconds": 1.0},
        ]))

    def test_aggregate_writes_raw_and_summary_files(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            raw = root / "raw/config"
            raw.mkdir(parents=True)
            payload = {
                "configuration": {
                    "key": "config", "part": "p2", "vary": "K",
                    "dataset": {"name": "sample", "synthetic": True},
                    "parameter_value": 30, "series_value": None,
                },
                "trial": 0, "utility_seed": 1, "algorithm_seed": 2,
                "utility_sha256": "abc",
                "results": [
                    {"method": "FHDR", "status": "ok", "regret_ratio": 0.1,
                     "time_seconds": 1.0, "output_size": 30, "questions": 15},
                    {"method": "UtilityApprox", "status": "ok", "regret_ratio": 0.2,
                     "time_seconds": 0.5, "output_size": 30, "questions": 15},
                ],
            }
            (raw / "trial_000.json").write_text(json.dumps(payload))
            summaries = aggregate(root / "raw", root)
            self.assertTrue((root / "raw.csv").is_file())
            self.assertTrue((root / "summary.csv").is_file())
            fhdr = next(row for row in summaries if row["method"] == "FHDR")
            self.assertEqual(fhdr["outperformance_rate"], 1.0)


if __name__ == "__main__":
    unittest.main()
