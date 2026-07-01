#include "experiment_random.h"

#include <cstdlib>

namespace {

std::mt19937& shared_generator(){
    static std::mt19937 generator(std::random_device{}());
    return generator;
}

} // namespace

void seed_experiment_random(unsigned int seed){
    shared_generator().seed(seed);
    srand(seed);
}

std::mt19937& experiment_random_generator(){
    return shared_generator();
}
