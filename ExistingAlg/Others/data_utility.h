#ifndef DATA_UTILITY_H
#define DATA_UTILITY_H

#include "data_struct.h"

point_t *alloc_point(int dim);
point_t *alloc_point(int dim, int id);
point_set_t *alloc_point_set(int number_of_points);
void release_point(point_t *&point);
void release_point_set(point_set_t *&point_set, bool release_points);

#endif
