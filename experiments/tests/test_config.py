import unittest

from experiments.config import (
    REAL_DATASETS, default_question_budget, experiment_configurations,
)


class ConfigTest(unittest.TestCase):
    def test_real_question_budgets_use_ceiling(self):
        expected = {"house": 5, "car": 9, "nba": 15, "energy": 22}
        self.assertEqual(
            {name: default_question_budget(dataset) for name, dataset in REAL_DATASETS.items()},
            expected,
        )

    def test_car_manifest_matches_local_experiment_input(self):
        car = REAL_DATASETS["car"]
        self.assertEqual((car.size, car.dimension), (4_278, 57))

    def test_p2_k_matrix(self):
        configurations = experiment_configurations("p2", "K", "synthetic")
        self.assertEqual([configuration.parameter_value for configuration in configurations], [20, 30, 40, 50, 60])
        self.assertTrue(all(configuration.question_budget == 15 for configuration in configurations))

    def test_p1_is_single_output_complete_interaction(self):
        configurations = experiment_configurations("p1", "d_int", "nba")
        self.assertTrue(all(configuration.output_size == 1 for configuration in configurations))
        self.assertTrue(all(configuration.question_budget == 100 for configuration in configurations))

    def test_internal_w_stops_at_implementation_phase_one_boundary(self):
        configurations = experiment_configurations("internal", "w", "real")
        budgets = {
            configuration.dataset.name: configuration.question_budget
            for configuration in configurations
        }
        self.assertEqual(budgets, {"car": 8, "nba": 14, "energy": 21})


if __name__ == "__main__":
    unittest.main()
