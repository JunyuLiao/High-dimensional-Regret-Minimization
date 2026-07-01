#include "Others/data_utility.h"
#include "Others/operation.h"
#include "UtilityApprox/UtilityApprox.h"
#include "exp_stats.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <memory>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{
constexpr int kQuestionSize = 10;
constexpr double kTheta = 0.0;
constexpr const char *kAlgorithm = "util";

struct Config
{
    bool experiment_mode;
    int repeats;
    std::string algorithm;
    fs::path input_argument;
    fs::path input_path;
    int utility_dimensions;
    int return_size;
    int max_rounds;
    double epsilon;
    fs::path utility_path;
};

struct Summary
{
    double average_questions;
    double average_return_size;
    double average_time;
    double average_regret_ratio;
};

struct PointDeleter
{
    void operator()(point_t *point) const
    {
        release_point(point);
    }
};

template <bool ReleasePoints>
struct PointSetDeleter
{
    void operator()(point_set_t *points) const
    {
        release_point_set(points, ReleasePoints);
    }
};

using PointPtr = std::unique_ptr<point_t, PointDeleter>;
using OwnedPointSet = std::unique_ptr<point_set_t, PointSetDeleter<true>>;
using PointSetView = std::unique_ptr<point_set_t, PointSetDeleter<false>>;

void print_usage(const char *program)
{
    (void)program;
    std::cout << "usage: ./ExistingAlg NUM_REPEAT ALG_NAME INPUT D_PRIME W MAXROUND EPSILON\n"
              << "ALG_NAME: hdpi | persist | util ...\n"
              << "D_PRIME: number of dimensions with non-zero values\n"
              << "W: return size\n"
              << "MAXROUND: max number of questions\n"
              << "EPSILON: error rate\n";
}

fs::path find_repository_root(fs::path directory)
{
    directory = fs::weakly_canonical(directory);
    while (true)
    {
        if (fs::is_directory(directory / "datasets") &&
            fs::is_directory(directory / "ExistingAlg"))
            return directory;

        const fs::path parent = directory.parent_path();
        if (parent == directory)
            break;
        directory = parent;
    }
    return {};
}

fs::path locate_repository_root(const char *program, const fs::path &requested_input)
{
    const std::vector<fs::path> search_roots{
        fs::absolute(program).parent_path(),
        fs::current_path(),
        fs::absolute(requested_input).parent_path(),
    };
    for (const fs::path &search_root : search_roots)
    {
        const fs::path repository_root = find_repository_root(search_root);
        if (!repository_root.empty())
            return repository_root;
    }
    throw std::runtime_error("cannot locate the repository dataset directory");
}

fs::path resolve_input(const fs::path &requested, const fs::path &repository_root)
{
    if (fs::is_regular_file(requested))
        return fs::absolute(requested);

    const fs::path dataset_path = repository_root / "datasets" / requested;
    if (fs::is_regular_file(dataset_path))
        return dataset_path;

    throw std::runtime_error("input file not found: " + requested.string());
}

Config parse_config(int argc, char *argv[], const fs::path &repository_root)
{
    const bool experiment_mode = argc == 8 && std::string(argv[1]) == "--experiment";
    if (argc != 1 && argc != 8)
        throw std::invalid_argument("invalid argument count");

    const fs::path input_argument = argc == 1 ? "e100-10k.txt" :
        (experiment_mode ? fs::path(argv[2]) : fs::path(argv[3]));
    Config config{};
    config.experiment_mode = experiment_mode;
    config.repeats = argc == 1 ? 100 : (experiment_mode ? 1 : std::stoi(argv[1]));
    config.algorithm = argc == 1 || experiment_mode ? kAlgorithm : argv[2];
    config.input_argument = input_argument;
    config.input_path = resolve_input(input_argument, repository_root);
    config.utility_dimensions = argc == 1 ? 3 : std::stoi(argv[experiment_mode ? 3 : 4]);
    config.return_size = argc == 1 ? 1 : std::stoi(argv[experiment_mode ? 4 : 5]);
    config.max_rounds = argc == 1 ? 30 : std::stoi(argv[experiment_mode ? 5 : 6]);
    config.epsilon = argc == 1 ? 1.0 : std::stod(argv[experiment_mode ? 6 : 7]);
    if (experiment_mode)
        config.utility_path = argv[7];

    if (config.repeats <= 0 || config.algorithm != kAlgorithm ||
        config.utility_dimensions <= 0 || config.return_size <= 0 ||
        config.max_rounds <= 0 || config.epsilon < 0.0)
        throw std::invalid_argument("invalid argument value");

    return config;
}

OwnedPointSet read_points(const fs::path &path)
{
    std::ifstream input(path);
    int count = 0;
    int dimension = 0;
    if (!(input >> count >> dimension) || count < 2 || dimension < 2)
        throw std::runtime_error("invalid dataset header in " + path.string());

    OwnedPointSet points(alloc_point_set(count));
    for (int row = 0; row < count; ++row)
    {
        points->points[row] = alloc_point(dimension, row);
        for (int column = 0; column < dimension; ++column)
        {
            if (!(input >> points->points[row]->coord[column]))
                throw std::runtime_error("incomplete dataset row in " + path.string());
        }
    }
    return points;
}

void linear_normalize(point_set_t *points)
{
    const int dimension = points->points[0]->dim;
    for (int column = 0; column < dimension; ++column)
    {
        double minimum = points->points[0]->coord[column];
        double maximum = minimum;
        for (int row = 1; row < points->numberOfPoints; ++row)
        {
            minimum = std::min(minimum, points->points[row]->coord[column]);
            maximum = std::max(maximum, points->points[row]->coord[column]);
        }
        for (int row = 0; row < points->numberOfPoints; ++row)
        {
            points->points[row]->coord[column] = maximum == minimum ? 0.0 :
                (points->points[row]->coord[column] - minimum) / (maximum - minimum);
        }
    }
}

bool dominates(const point_t *first, const point_t *second)
{
    for (int dimension = 0; dimension < first->dim; ++dimension)
    {
        if (first->coord[dimension] < second->coord[dimension])
            return false;
    }
    return true;
}

PointSetView skyline(const point_set_t *input)
{
    std::vector<point_t *> candidates;
    for (int index = 0; index < input->numberOfPoints; ++index)
    {
        point_t *candidate = input->points[index];
        bool dominated = false;
        for (auto existing = candidates.begin(); existing != candidates.end();)
        {
            if (dominates(*existing, candidate))
            {
                dominated = true;
                break;
            }
            existing = dominates(candidate, *existing)
                           ? candidates.erase(existing)
                           : std::next(existing);
        }
        if (!dominated)
            candidates.push_back(candidate);
    }

    PointSetView result(alloc_point_set(static_cast<int>(candidates.size())));
    std::copy(candidates.begin(), candidates.end(), result->points);
    return result;
}

PointSetView all_points_view(point_set_t *input)
{
    PointSetView result(alloc_point_set(input->numberOfPoints));
    std::copy(input->points, input->points + input->numberOfPoints, result->points);
    return result;
}

std::vector<point_t *> top_points(const point_set_t *points,
                                  const point_t *utility,
                                  int count)
{
    std::vector<point_t *> ordered(points->points,
                                   points->points + points->numberOfPoints);
    count = std::min(count, static_cast<int>(ordered.size()));
    std::partial_sort(ordered.begin(), ordered.begin() + count, ordered.end(),
                      [utility](const point_t *left, const point_t *right) {
                          return dot_prod(utility, left) > dot_prod(utility, right);
                      });
    ordered.resize(count);
    return ordered;
}

PointPtr generate_utility(int dimension, int nonzero_dimensions, std::mt19937 &generator)
{
    PointPtr utility(alloc_point(dimension));
    std::vector<int> indices(dimension);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), generator);

    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    double total = 0.0;
    for (int i = 0; i < nonzero_dimensions; ++i)
    {
        utility->coord[indices[i]] = distribution(generator);
        total += utility->coord[indices[i]];
    }
    if (total == 0.0)
        utility->coord[indices.front()] = total = 1.0;
    for (int i = 0; i < dimension; ++i)
        utility->coord[i] /= total;
    return utility;
}

PointPtr read_utility(const fs::path &path, int dimension)
{
    std::ifstream input(path);
    PointPtr utility(alloc_point(dimension));
    for (int index = 0; index < dimension; ++index)
    {
        if (!(input >> utility->coord[index]))
            throw std::runtime_error("invalid utility file: " + path.string());
    }
    return utility;
}

void set_best_score(const std::vector<point_t *> &top, const point_t *utility)
{
    best_score = dot_prod(utility, top[0]);
}

Summary run_experiment(const Config &config, point_set_t *points, std::ostream &output)
{
    reset_timer();
    question_num = 0.0;
    return_size = 0.0;
    regret_ratio = 0.0;

    std::mt19937 generator(std::random_device{}());
    const int dimension = points->points[0]->dim;
    PointPtr fixed_utility = config.experiment_mode
        ? read_utility(config.utility_path, dimension)
        : PointPtr(nullptr);
    for (int round = 0; round < config.repeats; ++round)
    {
        output << "round " << round << '\n';
        PointPtr generated_utility = config.experiment_mode
            ? PointPtr(nullptr)
            : generate_utility(dimension, config.utility_dimensions, generator);
        point_t *utility = config.experiment_mode ? fixed_utility.get() : generated_utility.get();
        const auto top = top_points(points, utility, 2);
        set_best_score(top, utility);
        utilityapprox(points, utility, kQuestionSize, config.epsilon,
                      config.max_rounds, config.return_size, kTheta);
    }

    return {
        question_num / config.repeats,
        return_size / config.repeats,
        avg_time() / 1000000.0,
        regret_ratio / config.repeats,
    };
}

void write_summary(std::ostream &output, const Summary &summary)
{
    output << "avg question num: " << summary.average_questions << '\n'
           << "avg return size: " << summary.average_return_size << '\n'
           << "avg time: " << summary.average_time << '\n'
           << "avg regret ratio: " << summary.average_regret_ratio << '\n';
}
} // namespace

int main(int argc, char *argv[])
{
    try
    {
        if (argc != 1 && argc != 8)
            throw std::invalid_argument("invalid argument count");
        const bool experiment_mode = argc == 8 && std::string(argv[1]) == "--experiment";
        const fs::path requested_input = argc == 1 ? "e100-10k.txt" :
            (experiment_mode ? fs::path(argv[2]) : fs::path(argv[3]));
        const fs::path repository_root = locate_repository_root(argv[0], requested_input);
        const Config config = parse_config(argc, argv, repository_root);

        std::ofstream result_output;
        std::ostream *output = &std::cout;
        if (!config.experiment_mode)
        {
            const fs::path results_dir = repository_root / "ExistingAlg" / "results";
            fs::create_directories(results_dir);
            const fs::path result_path = results_dir /
                ("_" + config.algorithm + "_" + config.input_argument.filename().string() + "_" +
                 std::to_string(config.repeats) + ".txt");
            result_output.open(result_path);
            if (!result_output)
                throw std::runtime_error("cannot create result file: " + result_path.string());
            output = &result_output;
        }

        OwnedPointSet all_points = read_points(config.input_path);
        linear_normalize(all_points.get());
        PointSetView points = config.experiment_mode
            ? all_points_view(all_points.get())
            : skyline(all_points.get());
        if (points->numberOfPoints < 2)
            throw std::runtime_error("UtilityApprox requires at least two skyline points");

        if (config.utility_dimensions > points->points[0]->dim)
            throw std::runtime_error("D_PRIME exceeds the dataset dimension");

        std::cout << config.input_path << '\n' << config.algorithm << '\n';
        const Summary summary = run_experiment(config, points.get(), *output);
        write_summary(std::cout, summary);
        if (!config.experiment_mode)
            write_summary(result_output, summary);
        else
            std::cout << "EXPERIMENT_RESULT {\"method\":\"UtilityApprox\",\"status\":\"ok\""
                      << ",\"regret_ratio\":" << std::setprecision(17) << summary.average_regret_ratio
                      << ",\"time_seconds\":" << summary.average_time
                      << ",\"output_size\":" << summary.average_return_size
                      << ",\"questions\":" << summary.average_questions << "}" << std::endl;
        return 0;
    }
    catch (const std::invalid_argument &)
    {
        print_usage(argv[0]);
        return 2;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
