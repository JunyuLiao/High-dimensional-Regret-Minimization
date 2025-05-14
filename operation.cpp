#include "operation.h"
#include "data_utility.h"
#include <stdlib.h>
#include <chrono>
#include <random>
#include <cmath>
#include <algorithm>
#include <iostream>
using namespace std;

// Common functions from both files:
point_set_t* remove_outliers(point_set_t* &point_set);
void linear_normalize(point_set_t* &point_set);
void reduce_to_unit(point_set_t* &point_set);
vector<vector<double> > generate_JL(int k, int d, int matrix_type);
vector<vector<double> > generate_JL(int k, int d, point_set_t* &P);
point_set_t* dim_reduce(point_set_t* &point_set, vector<vector<double> > JL);
int isZero(double x);
DIST_TYPE calc_len(point_t* point_v);
point_t* copy(point_t* point_v2);
double dot_prod(point_t* point_v1, point_t* point_v2);
double dot_prod(point_t* point_v1, double* v);
point_t* sub(point_t* point_v1, point_t* point_v2);
point_t* add(point_t* point_v1, point_t* point_v2);
point_t* scale(double c, point_t* point_v);

// Unique functions from sphere/operation.cpp:
float rand_f(float min_v, float max_v);
DIST_TYPE calc_dist(point_t* point_v1, point_t* point_v2);
bool isViolated(point_t* normal_q, point_t* normal_p, point_t* e);
point_t* maxPoint(point_set_t* p, double *v);
vector<double> gaussNtimesD(vector<vector<double> > A);
point_t* projectPointsOntoAffineSpace(point_set_t* space, point_t* p);
Vvi build_input(int t, int dim);
void cart_product(Vvi& rvvi, Vi& rvi, Vvi::const_iterator me, Vvi::const_iterator end);
point_set_t* read_points(char* input);
int dominates(point_t* p1, point_t* p2);
point_set_t* skyline_point(point_set_t *p);
void insertOrth(double* &points, int &count, point_t* v);

// Unique functions from interactive/operation.cpp:
DIST_TYPE calc_l1_dist(point_t* point_v1, point_t* point_v2);
double dot_prod(double* v1, double* v2, int dim);
int guassRank(vector<point_t*> P);
bool linearInd(point_t* p, point_t* q);
double compute_intersection_len(hyperplane_t *hp, point_t* s);
point_t* getPoint(point_set_t* p, double* point);

// Implementation of all functions...

// Remove outliers from point set
point_set_t* remove_outliers(point_set_t* &point_set) {
    int n = point_set->numberOfPoints;
    int dim = point_set->points[0]->dim;
    int select_dim_count = 0;
    vector<int> select_dim_index(n, 0);
    vector<vector<double>> bound(2, vector<double>(dim, 0));
    
    // Calculate bounds for each dimension
    for (int i = 0; i < dim; ++i) {
        vector<double> entry(n, 0);
        for (int j = 0; j < n; ++j) {
            entry[j] = point_set->points[j]->coord[i];
        }
        sort(entry.begin(), entry.end());
        double upper = entry[(int)(0.95*n)];
        double lower = entry[(int)(0.05*n)];
        bound[0][i] = lower;
        bound[1][i] = upper;
        if (bound[0][i] != bound[1][i]) {
            select_dim_count++;
            select_dim_index[i] = 1;
        }
    }

    // Find non-outlier points
    int select_n_count = 0;
    vector<int> select_n_index(n, 0);
    for (int i = 0; i < n; ++i) {
        bool is_outlier = false;
        for (int j = 0; j < dim; ++j) {
            if (point_set->points[i]->coord[j] < bound[0][j] || 
                point_set->points[i]->coord[j] > bound[1][j]) {
                is_outlier = true;
                break;
            }
        }
        if (!is_outlier) {
            select_n_count++;
            select_n_index[i] = 1;
        }
    }

    // Create new point set without outliers
    point_set_t* new_point_set = alloc_point_set(select_n_count);
    int index_n = 0;
    for (int i = 0; i < n; ++i) {
        if (select_n_index[i]) {
            int index_dim = 0;
            point_t* new_point = alloc_point(select_dim_count);
            new_point->id = index_n;
            for (int j = 0; j < dim; ++j) {
                if (select_dim_index[j]) {
                    new_point->coord[index_dim] = point_set->points[i]->coord[j];
                    index_dim++;
                }
            }
            new_point_set->points[index_n] = new_point;
            index_n++;
        }
    }
    release_point_set(point_set, false);
    return new_point_set;
}

// Linear normalization of point set
void linear_normalize(point_set_t* &point_set) {
    int dim = point_set->points[0]->dim;
    int n = point_set->numberOfPoints;
    
    for (int i = 0; i < dim; ++i) {
        double min = point_set->points[0]->coord[i];
        double max = point_set->points[0]->coord[i];
        
        // Find min and max for dimension i
        for (int j = 1; j < n; ++j) {
            min = (point_set->points[j]->coord[i] < min) ? point_set->points[j]->coord[i] : min;
            max = (point_set->points[j]->coord[i] > max) ? point_set->points[j]->coord[i] : max;
        }
        
        // Normalize dimension i
        for (int j = 0; j < n; ++j) {
            if (max == min) {
                point_set->points[j]->coord[i] = 0;
            } else {
                point_set->points[j]->coord[i] = (point_set->points[j]->coord[i] - min) / (max - min);
            }
        }
    }
}

// Reduce points to unit length
void reduce_to_unit(point_set_t* &point_set) {
    int n = point_set->numberOfPoints;
    double max_len = 0;
    
    // Find maximum length
    for (int i = 0; i < n; ++i) {
        double len = 0;
        for (int j = 0; j < point_set->points[i]->dim; ++j) {
            len += point_set->points[i]->coord[j] * point_set->points[i]->coord[j];
        }
        max_len = (len > max_len) ? len : max_len;
    }
    
    // Scale all points
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < point_set->points[i]->dim; ++j) {
            point_set->points[i]->coord[j] /= max_len;
        }
    }
}


// Basic utility functions
int isZero(double x) {
    return x > -EQN_EPS && x < EQN_EPS;
}

DIST_TYPE calc_len(point_t* point_v) {
    int dim = point_v->dim;
    DIST_TYPE diff = 0;
    
    for (int i = 0; i < dim; i++) {
        diff += point_v->coord[i] * point_v->coord[i];
    }
    return (DIST_TYPE)sqrt(diff);
}

point_t* copy(point_t* point_v2) {
    if (point_v2 == NULL)
        return NULL;

    point_t* point_v1 = alloc_point(point_v2->dim);
    for (int i = 0; i < point_v2->dim; i++)
        point_v1->coord[i] = point_v2->coord[i];
    return point_v1;
}

// Vector operations
double dot_prod(point_t* point_v1, point_t* point_v2) {
    int dim = point_v1->dim;
    double result = 0;
    for (int i = 0; i < dim; i++) {
        result += point_v1->coord[i] * point_v2->coord[i];
    }
    return result;
}

double dot_prod(point_t* point_v1, double* v) {
    int dim = point_v1->dim;
    double result = 0;
    for (int i = 0; i < dim; i++) {
        result += point_v1->coord[i] * v[i];
    }
    return result;
}

double dot_prod(double* v1, double* v2, int dim) {
    double result = 0;
    for (int i = 0; i < dim; i++) {
        result += v1[i] * v2[i];
    }
    return result;
}

point_t* sub(point_t* point_v1, point_t* point_v2) {
    point_t* result = alloc_point(point_v1->dim);
    for (int i = 0; i < point_v1->dim; i++) {
        result->coord[i] = point_v1->coord[i] - point_v2->coord[i];
    }
    return result;
}

point_t* add(point_t* point_v1, point_t* point_v2) {
    point_t* result = alloc_point(point_v1->dim);
    for (int i = 0; i < point_v1->dim; i++) {
        result->coord[i] = point_v1->coord[i] + point_v2->coord[i];
    }
    return result;
}

point_t* scale(double c, point_t* point_v) {
    point_t* result = alloc_point(point_v->dim);
    for (int i = 0; i < point_v->dim; i++) {
        result->coord[i] = point_v->coord[i] * c;
    }
    return result;
}

// Functions from sphere/operation.cpp
float rand_f(float min_v, float max_v) {
    if (min_v > max_v)
        return 0;
    float rand_v = float(rand()) / RAND_MAX;
    return (max_v - min_v) * rand_v + min_v;
}

DIST_TYPE calc_dist(point_t* point_v1, point_t* point_v2) {
    int dim = point_v1->dim;
    DIST_TYPE diff = 0;
    
    for (int i = 0; i < dim; i++) {
        diff += (DIST_TYPE)pow(point_v1->coord[i] - point_v2->coord[i], 2);
    }
    return (DIST_TYPE)sqrt(diff);
}

bool isViolated(point_t* normal_q, point_t* normal_p, point_t* e) {
    if (isZero(calc_dist(normal_q, normal_p))) {
        return true;
    }

    point_t* temp_normal = sub(normal_q, normal_p);
    point_t* temp = sub(e, normal_p);
    
    bool result = (dot_prod(temp_normal, temp) > 0 && !isZero(dot_prod(temp_normal, temp)));
    
    release_point(temp_normal);
    release_point(temp);
    return result;
}

point_t* maxPoint(point_set_t* p, double* v) {
    int N = p->numberOfPoints;
    int maxIndex = 0;
    double max = 0.0;
    
    for (int i = 0; i < N; ++i) {
        if (dot_prod(p->points[i], v) > max) {
            maxIndex = i;
            max = dot_prod(p->points[i], v);
        }
    }
    return p->points[maxIndex];
}

// Gauss elimination for N x D matrix
vector<double> gaussNtimesD(vector<vector<double>> A) {
    int n = A.size();
    int d = A[0].size() - 1;

    // Forward elimination
    for (int i = 0; i < d; i++) {
        // Find maximum in this column
        double maxEl = abs(A[i][i]);
        int maxRow = i;
        for (int k = i + 1; k < n; k++) {
            if (abs(A[k][i]) > maxEl) {
                maxEl = abs(A[k][i]);
                maxRow = k;
            }
        }

        // Swap maximum row with current row
        for (int k = i; k < d + 1; k++) {
            double tmp = A[maxRow][k];
            A[maxRow][k] = A[i][k];
            A[i][k] = tmp;
        }

        // Make all rows below this one 0 in current column
        for (int k = i + 1; k < n; k++) {
            double c = -A[k][i] / A[i][i];
            for (int j = i; j < d + 1; j++) {
                if (i == j) {
                    A[k][j] = 0;
                } else {
                    A[k][j] += c * A[i][j];
                }
            }
        }
    }

    // Check validity
    int count = 0;
    for (int i = 0; i < n; i++) {
        bool allZero = true;
        for (int j = 0; j < d + 1; j++) {
            if (!isZero(A[i][j])) {
                allZero = false;
                break;
            }
        }
        if (!allZero)
            count++;
    }
    if (count != d) {
        vector<double> x(d);
        x[d - 1] = -1;
        return x;
    }

    // Back substitution
    vector<double> x(d);
    for (int i = d - 1; i >= 0; i--) {
        x[i] = A[i][d] / A[i][i];
        for (int k = i - 1; k >= 0; k--) {
            A[k][d] -= A[k][i] * x[i];
        }
    }
    return x;
}

// Project point onto affine space
point_t* projectPointsOntoAffineSpace(point_set_t* space, point_t* p) {
    if (space->numberOfPoints < 1) {
        printf("ERROR in pt to affine\n");
        exit(0);
    }
    if (space->numberOfPoints == 1)
        return space->points[0];

    int dim = space->points[0]->dim;
    int n = space->numberOfPoints - 1;
    point_set_t* dirVecs = alloc_point_set(n);
    
    // Calculate direction vectors
    for (int i = 0; i < n; i++)
        dirVecs->points[i] = sub(space->points[i + 1], space->points[0]);

    // Gram-Schmidt orthogonalization
    for (int i = 1; i < n; i++) {
        for (int j = 0; j < i; j++) {
            double c_j = dot_prod(dirVecs->points[i], dirVecs->points[j]) / 
                        dot_prod(dirVecs->points[j], dirVecs->points[j]);

            for (int k = 0; k < dim; k++)
                dirVecs->points[i]->coord[k] = dirVecs->points[i]->coord[k] - 
                                             c_j * dirVecs->points[j]->coord[k];
        }
    }

    // Normalize direction vectors
    for (int i = 0; i < dirVecs->numberOfPoints; i++) {
        double norm = calc_len(dirVecs->points[i]);
        for (int j = 0; j < dim; j++)
            dirVecs->points[i]->coord[j] = dirVecs->points[i]->coord[j] / norm;
    }

    // Project point
    point_t* tmp = sub(p, space->points[0]);
    point_t* coord = alloc_point(n);
    for (int i = 0; i < n; i++)
        coord->coord[i] = dot_prod(tmp, dirVecs->points[i]);

    for (int i = 0; i < dim; i++) {
        tmp->coord[i] = 0;
        for (int j = 0; j < n; j++)
            tmp->coord[i] += coord->coord[j] * dirVecs->points[j]->coord[i];
    }

    point_t* proj = add(tmp, space->points[0]);

    // Cleanup
    release_point(tmp);
    release_point(coord);
    release_point_set(dirVecs, true);

    return proj;
}

// Build input for cartesian product
Vvi build_input(int t, int dim) {
    double dist_bet = 1.0 / (t + 1);
    Vvi vvi;
    Vi vi;
    
    // Make points at center of each hypercube
    for (int i = 0; i < t + 1; i++) {
        vi.push_back(i * dist_bet + dist_bet / 2);
    }

    // Add dimensions
    for (int i = 0; i < dim - 1; i++)
        vvi.push_back(vi);
    return vvi;
}

// Compute cartesian product
void cart_product(Vvi& rvvi, Vi& rvi, Vvi::const_iterator me, Vvi::const_iterator end) {
    if (me == end) {
        rvvi.push_back(rvi);
        return;
    }

    const Vi& mevi = *me;
    for (Vi::const_iterator it = mevi.begin(); it != mevi.end(); it++) {
        rvi.push_back(*it);
        cart_product(rvvi, rvi, me + 1, end);
        rvi.pop_back();
    }
}

// Read points from file
point_set_t* read_points(char* input) {
    FILE* c_fp;
    char filename[MAX_FILENAME_LENG];
    sprintf(filename, "%s", input);

    if ((c_fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "Cannot open the data file %s.\n", filename);
        exit(0);
    }

    int number_of_points, dim;
    fscanf(c_fp, "%i%i", &number_of_points, &dim);

    point_set_t* point_set = alloc_point_set(number_of_points);

    for (int i = 0; i < number_of_points; i++) {
        point_t* p = alloc_point(dim, i);
        for (int j = 0; j < dim; j++) {
            fscanf(c_fp, "%lf", &p->coord[j]);
        }
        point_set->points[i] = p;
    }

    fclose(c_fp);
    return point_set;
}

// Check dominance for skyline computation
int dominates(point_t* p1, point_t* p2) {
    for (int i = 0; i < p1->dim; ++i)
        if (p1->coord[i] < p2->coord[i])
            return 0;
    return 1;
}

// Compute skyline points
point_set_t* skyline_point(point_set_t* p) {
    int* sl = new int[p->numberOfPoints];
    int index = 0;

    for (int i = 0; i < p->numberOfPoints; ++i) {
        int dominated = 0;
        point_t* pt = p->points[i];

        // Check if pt is dominated by current skyline
        for (int j = 0; j < index && !dominated; ++j)
            if (dominates(p->points[sl[j]], pt))
                dominated = 1;

        if (!dominated) {
            // Remove dominated points from current skyline
            int m = index;
            index = 0;
            for (int j = 0; j < m; ++j)
                if (!dominates(pt, p->points[sl[j]]))
                    sl[index++] = sl[j];

            sl[index++] = i;
        }
    }

    point_set_t* skyline = alloc_point_set(index);
    for (int i = 0; i < index; i++)
        skyline->points[i] = p->points[sl[i]];

    delete[] sl;
    return skyline;
}

// Insert orthope set
void insertOrth(double* &points, int &count, point_t* v) {
    int dim = v->dim;
    int orthNum = pow(2.0, dim) - 1;

    for (int i = 0; i < dim; i++) {
        points[count * dim + i] = v->coord[i];
    }
    count++;

    int startEnumPow;
    for (int i = 0; i < orthNum - 1; i++) {
        startEnumPow = i + 1;
        for (int j = 0; j < dim; j++) {
            points[count * dim + j] = v->coord[j] * (startEnumPow % 2);
            startEnumPow /= 2;
        }
        count++;
    }
}

// L1 distance calculation
DIST_TYPE calc_l1_dist(point_t* point_v1, point_t* point_v2) {
    int dim = point_v1->dim;
    DIST_TYPE diff = 0;
    
    for (int i = 0; i < dim; i++) {
        diff += (DIST_TYPE)abs(point_v1->coord[i] - point_v2->coord[i]);
    }
    return diff;
}

// Compute Gauss rank
int guassRank(vector<point_t*> P) {
    int dim = P[0]->dim;
    vector<vector<double>> A;
    
    for (int i = 0; i < P.size(); i++) {
        vector<double> v;
        for (int j = 0; j < dim; j++) {
            v.push_back(P[i]->coord[j]);
        }
        A.push_back(v);
    }
    
    while (A.size() < dim) {
        vector<double> v(dim, 0);
        A.push_back(v);
    }

    int n = A.size();
    int d = A[0].size();

    // Gaussian elimination
    for (int i = 0; i < d; i++) {
        double maxEl = abs(A[i][i]);
        int maxRow = i;
        for (int k = i + 1; k < n; k++) {
            if (abs(A[k][i]) > maxEl) {
                maxEl = abs(A[k][i]);
                maxRow = k;
            }
        }

        for (int k = i; k < d; k++) {
            double tmp = A[maxRow][k];
            A[maxRow][k] = A[i][k];
            A[i][k] = tmp;
        }

        for (int k = i + 1; k < n; k++) {
            double c = -A[k][i] / A[i][i];
            for (int j = i; j < d; j++) {
                if (i == j)
                    A[k][j] = 0;
                else
                    A[k][j] += c * A[i][j];
            }
        }
    }

    int count = 0;
    for (int i = 0; i < n; i++) {
        bool allZero = true;
        for (int j = 0; j < d; j++) {
            if (!isZero(A[i][j])) {
                allZero = false;
                break;
            }
        }
        if (!allZero)
            count++;
    }
    return count;
}

// Check linear independence
bool linearInd(point_t* p, point_t* q) {
    int dim = p->dim;
    bool pZero = true, qZero = true;
    double ratio;

    for (int i = 0; i < dim; i++) {
        if (!isZero(q->coord[i])) {
            qZero = false;
            ratio = p->coord[i] / q->coord[i];
        }
        if (!isZero(p->coord[i]))
            pZero = false;
    }

    if (pZero || qZero)
        return true;

    for (int i = 0; i < dim; i++) {
        if (!isZero(ratio - p->coord[i] / q->coord[i]))
            return true;
    }
    return false;
}

// Compute intersection length
double compute_intersection_len(hyperplane_t* hp, point_t* s) {
    return calc_len(s) * (hp->offset / (dot_prod(hp->normal, s)));
}

// Get point from point set
point_t* getPoint(point_set_t* p, double* point) {
    int dim = p->points[0]->dim;

    for (int i = 0; i < p->numberOfPoints; i++) {
        bool match = true;
        for (int j = 0; j < dim; j++) {
            if (!isZero(p->points[i]->coord[j] - point[j])) {
                match = false;
                break;
            }
        }
        if (match)
            return p->points[i];
    }
    return NULL;
}

// Continue with more implementations... 