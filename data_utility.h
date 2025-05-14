#ifndef DATA_UTILITY_H
#define DATA_UTILITY_H
#include "data_struct.h"

// sort the 2d points clockwise
struct angleCmp
{
	bool operator()(const point_t* lhs, const point_t* rhs) const
	{
		double lhs_angle = lhs->coord[1] == 0 ? PI / 2 : atan(lhs->coord[0] / lhs->coord[1]);
		double rhs_angle = rhs->coord[1] == 0 ? PI / 2 : atan(rhs->coord[0] / rhs->coord[1]);
		return lhs_angle < rhs_angle;
	}
};


// allocate/release memory for points/hyperplanes 
point_t* alloc_point(int dim);
point_t* alloc_point(int dim, int id);
void release_point( point_t* &point_v);
point_t* rand_point(int dim);
point_set_t* alloc_point_set(int numberOfPoints);
void release_point_set(point_set_t* &point_set_v, bool clear);
hyperplane_t* alloc_hyperplane(point_t* normal, double offset);
void release_hyperplane(hyperplane_t* &hyperplane_v);

// print informaiton
void print_point(point_t* point_v);
void print_point_set(point_set_t* point_set_v);

#endif