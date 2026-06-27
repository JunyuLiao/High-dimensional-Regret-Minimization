#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H

struct point_t
{
    int dim;
    int id;
    double *coord;
};

struct point_set_t
{
    int numberOfPoints;
    point_t **points;
};

#endif
