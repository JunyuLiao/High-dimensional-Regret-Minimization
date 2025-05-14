#ifndef MEDIANHULL_H
#define MEDIANHULL_H

#include "data_struct.h"
#include "data_utility.h"

#include "operation.h"
#include <algorithm>

// Algorithm Median
point_t* medianVertices(point_set_t* P, point_t* u, int maxRournd, double &Qcount, double &Csize);

// Algorithm Hull
point_t* hullVertices(point_set_t* P, point_t* u, int s, int maxRound, double &Qcount, double &Csize);

#endif