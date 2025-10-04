#ifndef MAXUTILITY_H
#define MAXUTILITY_H

#include "data_struct.h"
#include "data_utility.h"
#include "read_write.h"

#include <vector>
#include <algorithm> 
#include <map>

#include "rtree.h"
#include "lp.h"
#include "pruning.h"
#include <queue>

#define RANDOM 1
#define SIMPLEX 2
#define SIMPLEX_FLY 3

using namespace std;

// Data structure to store question mappings
// Maps set of dimensions to vector of presented tuple indices (first one is user's choice)
struct question_mapping {
    std::map<std::set<int>, std::vector<int>> questions;
};

// get the index of the "current best" point
int get_current_best_pt(point_set_t* P, vector<int>& C_idx, vector<point_t*>& ext_vec);

// generate s cars for selection in a round
void update_ext_vec(point_set_t* P, vector<int>& C_idx, point_t* u, int s, vector<point_t*>& ext_vec, int& current_best_idx, int& last_best, vector<int>& frame, int cmp_option);

// generate the options for user selection and update the extreme vecotrs based on the user feedback
vector<int> generate_S(point_set_t* P, vector<int>& C_idx, int s, int current_best_idx, int& last_best, vector<int>& frame, int cmp_option);

// the main interactive algorithm
point_t* max_utility(point_set_t* P, point_t* u, int s,  double epsilon, int maxRound, double &Qcount, double &Csize,  int cmp_option, int stop_option, int prune_option, int dom_option);

// the main interactive algorithm with pre-recorded questions
point_t* max_utility_with_questions(point_set_t* P, point_t* u, int s, double epsilon, int maxRound, double &Qcount, double &Csize, int cmp_option, int stop_option, int prune_option, int dom_option, const question_mapping& qm, const std::map<int, int>& dim_mapping, point_set_t* D_prime);

// construct extreme vectors from question mappings
void construct_ext_vec_from_questions(point_set_t* P, const question_mapping& qm, point_t* u, vector<point_t*>& ext_vec, int full_dim, const std::map<int, int>& dim_mapping, point_set_t* D_prime);

#endif