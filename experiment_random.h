#ifndef EXPERIMENT_RANDOM_H
#define EXPERIMENT_RANDOM_H

#include <random>

void seed_experiment_random(unsigned int seed);
std::mt19937& experiment_random_generator();

#endif
