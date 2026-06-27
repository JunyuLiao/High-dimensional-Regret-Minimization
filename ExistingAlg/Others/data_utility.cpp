#include "data_utility.h"

#include <cstdlib>
#include <cstring>

point_t *alloc_point(int dim)
{
    point_t *point = static_cast<point_t *>(std::calloc(1, sizeof(point_t)));
    point->dim = dim;
    point->id = -1;
    point->coord = static_cast<double *>(std::calloc(dim, sizeof(double)));
    return point;
}

point_t *alloc_point(int dim, int id)
{
    point_t *point = alloc_point(dim);
    point->id = id;
    return point;
}

point_set_t *alloc_point_set(int number_of_points)
{
    point_set_t *point_set = static_cast<point_set_t *>(std::calloc(1, sizeof(point_set_t)));
    point_set->numberOfPoints = number_of_points;
    point_set->points = static_cast<point_t **>(std::calloc(number_of_points, sizeof(point_t *)));
    return point_set;
}

void release_point(point_t *&point)
{
    if (point == nullptr)
        return;
    std::free(point->coord);
    std::free(point);
    point = nullptr;
}

void release_point_set(point_set_t *&point_set, bool release_points)
{
    if (point_set == nullptr)
        return;
    if (release_points)
        for (int i = 0; i < point_set->numberOfPoints; ++i)
            release_point(point_set->points[i]);
    std::free(point_set->points);
    std::free(point_set);
    point_set = nullptr;
}
