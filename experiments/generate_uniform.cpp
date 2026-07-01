#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>

int main(int argc, char* argv[]){
    if (argc != 5) {
        std::cerr << "usage: generate_uniform OUTPUT N D SEED\n";
        return 2;
    }

    const std::filesystem::path output_path = argv[1];
    const int size = std::stoi(argv[2]);
    const int dimension = std::stoi(argv[3]);
    const unsigned long long seed = std::stoull(argv[4]);
    if (size <= 0 || dimension <= 0) {
        throw std::invalid_argument("N and D must be positive");
    }

    std::filesystem::create_directories(output_path.parent_path());
    const std::filesystem::path temporary_path = output_path.string() + ".tmp";
    std::ofstream output(temporary_path);
    if (!output) {
        throw std::runtime_error("cannot create " + temporary_path.string());
    }

    std::mt19937_64 generator(seed);
    std::uniform_int_distribution<int> distribution(1, 67'108'864);
    output << size << ' ' << dimension << '\n';
    for (int row = 0; row < size; ++row) {
        for (int column = 0; column < dimension; ++column) {
            if (column != 0) output << ' ';
            output << distribution(generator);
        }
        output << '\n';
    }
    output.close();
    std::filesystem::rename(temporary_path, output_path);
    return 0;
}
