#ifndef OPERATION_H
#define OPERATION_H

#include "data_struct.h"
#include <vector>
#include <cstdio>

using namespace std;

// Type definitions for cartesian product
typedef vector<int> Vi;
typedef vector<Vi> Vvi;

// Constants
#define MAX_FILENAME_LENG 256
#define EQN_EPS 1e-9

// Common functions
point_set_t* remove_outliers(point_set_t* &point_set);
void linear_normalize(point_set_t* &point_set);
void reduce_to_unit(point_set_t* &point_set);
int isZero(double x);
DIST_TYPE calc_len(point_t* point_v);
point_t* copy(point_t* point_v2);

// Vector operations
double dot_prod(point_t* point_v1, point_t* point_v2);
double dot_prod(point_t* point_v1, double* v);
double dot_prod(double* v1, double* v2, int dim);
point_t* sub(point_t* point_v1, point_t* point_v2);
point_t* add(point_t* point_v1, point_t* point_v2);
point_t* scale(double c, point_t* point_v);

// Functions from sphere/operation.cpp
float rand_f(float min_v, float max_v);
DIST_TYPE calc_dist(point_t* point_v1, point_t* point_v2);
bool isViolated(point_t* normal_q, point_t* normal_p, point_t* e);
point_t* maxPoint(point_set_t* p, double *v);
vector<double> gaussNtimesD(vector<vector<double>> A);
point_t* projectPointsOntoAffineSpace(point_set_t* space, point_t* p);
Vvi build_input(int t, int dim);
void cart_product(Vvi& rvvi, Vi& rvi, Vvi::const_iterator me, Vvi::const_iterator end);
point_set_t* read_points(char* input);
int dominates(point_t* p1, point_t* p2);
point_set_t* skyline_point(point_set_t *p);
void insertOrth(double* &points, int &count, point_t* v);

// Functions from interactive/operation.cpp
DIST_TYPE calc_l1_dist(point_t* point_v1, point_t* point_v2);
int guassRank(vector<point_t*> P);
bool linearInd(point_t* p, point_t* q);
double compute_intersection_len(hyperplane_t *hp, point_t* s);
point_t* getPoint(point_set_t* p, double* point);

#endif // OPERATION_H 