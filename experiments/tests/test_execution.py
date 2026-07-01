import tempfile
import unittest
from pathlib import Path

from experiments.config import DatasetSpec, REPOSITORY_ROOT, TrialConfiguration
from experiments.execution import baseline_policy, ensure_utility, parse_records, run_trial


class ExecutionTest(unittest.TestCase):
    def test_result_record_parser(self):
        records = parse_records('noise\nEXPERIMENT_RESULT {"method":"FHDR","status":"ok"}\n')
        self.assertEqual(records, [{"method": "FHDR", "status": "ok"}])

    def test_infeasible_baseline_policy(self):
        million_points = TrialConfiguration(
            part="p2", vary="n",
            dataset=DatasetSpec("n", Path("n"), 1_000_000, 100, True),
            parameter_value=1_000_000, d_int=3, m=7, w=6,
            output_size=30, question_budget=15,
        )
        high_dimension = TrialConfiguration(
            part="p2", vary="d",
            dataset=DatasetSpec("d", Path("d"), 100_000, 200, True),
            parameter_value=200, d_int=3, m=7, w=6,
            output_size=30, question_budget=15,
        )
        self.assertEqual(baseline_policy(million_points), (True, False))
        self.assertEqual(baseline_policy(high_dimension), (True, True))

    def test_paired_p1_trial_on_sample_dataset(self):
        dataset = DatasetSpec(
            "sample", REPOSITORY_ROOT / "datasets/e100-10k.txt", 10_000, 100, True,
        )
        configuration = TrialConfiguration(
            part="p1", vary="d_int", dataset=dataset, parameter_value=3,
            d_int=3, m=7, w=6, output_size=1, question_budget=100,
        )
        with tempfile.TemporaryDirectory() as directory:
            utility = Path(directory) / "utility.txt"
            ensure_utility(utility, 100, 3, 123)
            records, _ = run_trial(configuration, 0, utility, 456, 60)
        methods = {record["method"]: record for record in records}
        self.assertEqual(methods["FHDR"]["status"], "ok")
        self.assertEqual(methods["UtilityApprox"]["status"], "ok")
        self.assertEqual(methods["FHDR"]["output_size"], 1)
        self.assertEqual(methods["UtilityApprox"]["output_size"], 1)

    def test_p2_trial_runs_all_algorithms_and_matches_output_size(self):
        dataset = DatasetSpec(
            "sample", REPOSITORY_ROOT / "datasets/e100-10k.txt", 10_000, 100, True,
        )
        configuration = TrialConfiguration(
            part="p2", vary="K", dataset=dataset, parameter_value=30,
            d_int=3, m=7, w=6, output_size=30, question_budget=15,
        )
        with tempfile.TemporaryDirectory() as directory:
            utility = Path(directory) / "utility.txt"
            ensure_utility(utility, 100, 3, 789)
            records, _ = run_trial(configuration, 0, utility, 101112, 60)
        methods = {record["method"]: record for record in records}
        self.assertEqual(methods["FHDR"]["status"], "ok")
        self.assertEqual(methods["Sphere-Adapt"]["status"], "ok")
        self.assertEqual(methods["UtilityApprox"]["status"], "ok")
        self.assertEqual(methods["UtilityApprox"]["output_size"], methods["FHDR"]["output_size"])


if __name__ == "__main__":
    unittest.main()
