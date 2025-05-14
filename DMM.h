#ifndef DMM_H
#define DMM_H

#include "data_utility.h"
#include "operation.h"
#include <algorithm>
#include <set>

point_set_t* DMM(point_set_t* point_set, int k);

point_set_t* DMM_Greedy(point_set_t* point, int k);

#endif
