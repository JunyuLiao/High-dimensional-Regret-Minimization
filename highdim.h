#include "other/data_utility.h"
#include "other/operation.h"
#include "other/sphere.h"
#include "other/GeoGreedy.h"
#include "other/DMM.h"
#include "other/lp.h"
#include <iostream>
#include "stdlib.h"
#include "stdio.h"
#include "time.h"
#include <cmath>
#include <chrono>
#include <random>
#include <set>
#include <vector>
#include <algorithm>
#include "other/data_struct.h"
#include "other/read_write.h"
#include "other/maxUtility.h"
#include "attribute_subset.h"
#include "other/medianhull.h"
#include <ctime>
using namespace std;

//define a struct to store the information about the output set, and the dimensions chosen
struct highdim_output{
    point_set_t* S;
    std::set<int> final_dimensions;
};

// the complete interactive high-dimensional regret algorithm with attribute subset method
highdim_output* interactive_highdim(point_set_t* skyline, int size, int d_bar, int d_hat, int d_hat_2, point_t* u, int K, int s, double epsilon, int maxRound, double& Qcount, double& Csize, int cmp_option, int stop_option, int prune_option, int dom_option, int& num_questions);
