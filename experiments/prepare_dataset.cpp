#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

using Point = std::vector<double>;

bool dominates(const Point& first, const Point& second){
    for (std::size_t index = 0; index < first.size(); ++index) {
        if (first[index] < second[index]) return false;
    }
    return true;
}

int main(int argc, char* argv[]){
    if (argc != 3) {
        std::cerr << "usage: prepare_dataset INPUT OUTPUT\n";
        return 2;
    }
    const std::filesystem::path input_path = argv[1];
    const std::filesystem::path output_path = argv[2];
    std::ifstream input(input_path);
    int size = 0;
    int dimension = 0;
    if (!(input >> size >> dimension) || size <= 0 || dimension <= 0) {
        throw std::runtime_error("invalid input header");
    }
    std::vector<Point> points(size, Point(dimension));
    for (Point& point : points) {
        for (double& coordinate : point) {
            if (!(input >> coordinate)) throw std::runtime_error("incomplete input dataset");
        }
    }

    for (int column = 0; column < dimension; ++column) {
        double minimum = points[0][column];
        double maximum = minimum;
        for (int row = 1; row < size; ++row) {
            minimum = std::min(minimum, points[row][column]);
            maximum = std::max(maximum, points[row][column]);
        }
        for (Point& point : points) {
            point[column] = maximum == minimum ? 0.0 :
                (point[column] - minimum) / (maximum - minimum);
        }
    }

    std::vector<int> skyline;
    skyline.reserve(size);
    for (int candidate = 0; candidate < size; ++candidate) {
        bool dominated = false;
        for (int existing : skyline) {
            if (dominates(points[existing], points[candidate])) {
                dominated = true;
                break;
            }
        }
        if (dominated) continue;
        skyline.erase(std::remove_if(skyline.begin(), skyline.end(), [&](int existing) {
            return dominates(points[candidate], points[existing]);
        }), skyline.end());
        skyline.push_back(candidate);
    }

    std::filesystem::create_directories(output_path.parent_path());
    const std::filesystem::path temporary_path = output_path.string() + ".tmp";
    std::ofstream output(temporary_path);
    output << skyline.size() << ' ' << dimension << '\n' << std::setprecision(17);
    for (int index : skyline) {
        for (int column = 0; column < dimension; ++column) {
            if (column != 0) output << ' ';
            output << points[index][column];
        }
        output << '\n';
    }
    output.close();
    std::filesystem::rename(temporary_path, output_path);
    return 0;
}
