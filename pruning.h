#ifndef PRUNING_H
#define PRUNING_H

#include "data_struct.h"
#include "data_utility.h"

#include "operation.h"
#include "lp.h"
#include "rtree.h"
#include "frame.h"
#include "read_write.h"
#include <queue>

// the domination options
#define HYPER_PLANE 1
#define CONICAL_HULL 2

// the skyline options
#define SQL 1
#define RTREE 2

//  the stopping options
#define NO_BOUND 1
#define EXACT_BOUND 2
#define APPROX_BOUND 3

using namespace std;

// get the set of extreme points of the candidate utility range R (bounded by the extreme vectors)
vector<point_t*> get_extreme_pts(vector<point_t*>& ext_vec);

// use the seqentail way for maintaining the candidate set
void sql_pruning(point_set_t* P, vector<int>& C_idx, vector<point_t*>& ext_vec, double& rr, int stop_option, int dom_option);

// use the branch-and-bound skyline (BBS) algorithm for maintaining the candidate set
void rtree_pruning(point_set_t* P, vector<int>& C_idx, vector<point_t*>& ext_vec, double& rr,  int stop_option, int dom_option);

#endif