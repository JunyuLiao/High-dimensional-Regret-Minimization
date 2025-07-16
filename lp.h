#ifndef LP_H
#define LP_H

#include <glpk.h>
#include <set>
#include "data_struct.h"
#include "data_utility.h"
#include "operation.h"
#include <vector>

// solve LP using GLPK

// Use LP to check whehter a point pt is a conical combination of the vectors in ExRays
bool insideCone(std::vector<point_t*> ExRays, point_t* pt);

// Use LP to find a feasible point of the half sapce intersection (used later in Qhull for half space intersection)
point_t* find_feasible(std::vector<hyperplane_t*> hyperplane);

// solve the LP in frame computation
void solveLP(std::vector<point_t*> B, point_t* b, double& theta, point_t* & pi);

/*
 * The linear programs for computing MRR
 */
double worstDirection(point_set_t *s, point_t* pt, double* &v);
double worstDirection(int index, point_set_t *s, point_t* pt, double* &v);
double worstDirection(int index, point_set_t *s, point_t* pt, float* &v);
double determinant(int n, double** a);


/*
 * Compute the MRR of a given set of points
 */
// original MRR evaluation, consider worse case MRR
double evaluateLP(point_set_t *p, point_set_t* S, int VERBOSE);
// use random sampling
double evaluateLP(point_set_t *p, point_set_t* S, int VERBOSE, int d, std::set<int> final_dimensions, int test_rounds = 20);
// use the ground truth utility vector u
double evaluateLP(point_set_t *p, point_set_t* S, int VERBOSE, point_t* u);


#endif