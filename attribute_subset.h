#include "data_utility.h"
#include "sphere.h"
#include <iostream>
#include <chrono>
#include <random>
#include <set>
#include <vector>
#include "data_struct.h"

point_set_t* attribute_subset(point_set_t* skyline, point_set_t* S_output, int final_d, int d_prime, int d_hat_2, int rounds, std::set<int> set_final_dimensions);