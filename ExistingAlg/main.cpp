#include "Others/data_utility.h"
#include "Others/operation.h"
#include "UtilityApprox/UtilityApprox.h"
#include "exp_stats.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
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
    int repeats;
    std::string algorithm;
    fs::path input_argument;
    fs::path input_path;
    int utility_dimensions;
    int return_size;
    int max_rounds;
    double epsilon;
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
    if (argc != 1 && argc != 8)
        throw std::invalid_argument("invalid argument count");

    const fs::path input_argument = argc == 1 ? "e100-10k.txt" : argv[3];
    Config config{};
    config.repeats = argc == 1 ? 100 : std::stoi(argv[1]);
    config.algorithm = argc == 1 ? kAlgorithm : argv[2];
    config.input_argument = input_argument;
    config.input_path = resolve_input(input_argument, repository_root);
    config.utility_dimensions = argc == 1 ? 3 : std::stoi(argv[4]);
    config.return_size = argc == 1 ? 1 : std::stoi(argv[5]);
    config.max_rounds = argc == 1 ? 30 : std::stoi(argv[6]);
    config.epsilon = argc == 1 ? 1.0 : std::stod(argv[7]);

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

bool strictly_dominates(const point_t *first, const point_t *second)
{
    bool strictly_better = false;
    for (int dimension = 0; dimension < first->dim; ++dimension)
    {
        if (first->coord[dimension] < second->coord[dimension])
            return false;
        strictly_better = strictly_better ||
                          first->coord[dimension] > second->coord[dimension];
    }
    return strictly_better;
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
            if (strictly_dominates(*existing, candidate))
            {
                dominated = true;
                break;
            }
            existing = strictly_dominates(candidate, *existing)
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
    for (int round = 0; round < config.repeats; ++round)
    {
        output << "round " << round << '\n';
        PointPtr utility = generate_utility(
            dimension, config.utility_dimensions, generator);
        const auto top = top_points(points, utility.get(), 2);
        set_best_score(top, utility.get());
        utilityapprox(points, utility.get(), kQuestionSize, config.epsilon,
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
        const fs::path requested_input = argc == 1 ? "e100-10k.txt" : argv[3];
        const fs::path repository_root = locate_repository_root(argv[0], requested_input);
        const Config config = parse_config(argc, argv, repository_root);

        const fs::path results_dir = repository_root / "ExistingAlg" / "results";
        fs::create_directories(results_dir);
        const fs::path result_path = results_dir /
            ("_" + config.algorithm + "_" + config.input_argument.filename().string() + "_" +
             std::to_string(config.repeats) + ".txt");
        std::ofstream output(result_path);
        if (!output)
            throw std::runtime_error("cannot create result file: " + result_path.string());

        OwnedPointSet all_points = read_points(config.input_path);
        PointSetView points = skyline(all_points.get());
        if (points->numberOfPoints < 2)
            throw std::runtime_error("UtilityApprox requires at least two skyline points");

        if (config.utility_dimensions > points->points[0]->dim)
            throw std::runtime_error("D_PRIME exceeds the dataset dimension");

        std::cout << config.input_path << '\n' << config.algorithm << '\n';
        const Summary summary = run_experiment(config, points.get(), output);
        write_summary(std::cout, summary);
        write_summary(output, summary);
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
