//#include "stdafx.h"

#include "data_utility.h"


/*
 *	Allocate memory for a point in dim-dimensional space
 */
point_t* alloc_point( int dim)
{
	point_t* point_v;

	point_v = ( point_t*)malloc( sizeof( point_t));
	memset( point_v, 0, sizeof( point_t));

	point_v->dim = dim;

	point_v->coord = ( COORD_TYPE*)malloc( dim * sizeof( COORD_TYPE));
	memset( point_v->coord, 0, dim * sizeof( COORD_TYPE));

	point_v->id = -1;

	return point_v;
}

/*
 *	Allocate memory for a point in dim-dimensional space with id
 */
point_t* alloc_point(int dim, int id)
{
	point_t* point_v = alloc_point(dim);

	point_v->id = id;

	return point_v;
}

/*
 *	Generate a random point in dim-dimensional space
 */
point_t* rand_point(int dim)
{
	point_t* point_v = alloc_point(dim);

	for (int i = 0; i < dim; i++)
		point_v->coord[i] = rand() * 1.0 / RAND_MAX;

	//print_point(point_v);

	return point_v;
}

/*
 *	Release memory for a point
 */
void release_point( point_t* &point_v)
{
	if(point_v == NULL)
		return;
	
	if(point_v->coord != NULL)
	{
		free( point_v->coord);
		point_v->coord = NULL;
	}

	free( point_v);
	point_v = NULL;
}

/*
 *	Allocate memory for a hyperplane in dim-dimensional space
 */
hyperplane_t* alloc_hyperplane(point_t* normal, double offset)
{
	hyperplane_t* hyperplane_v;

	hyperplane_v = (hyperplane_t*)malloc(sizeof(hyperplane_t));
	memset(hyperplane_v, 0, sizeof(hyperplane_t));

	hyperplane_v->normal = normal;
	hyperplane_v->offset = offset;

	return hyperplane_v;
}

/*
 *	Release memory for a point in dim-dimensional space
 */
void release_hyperplane(hyperplane_t* &hyperplane_v)
{
	if (hyperplane_v == NULL)
		return;

	free(hyperplane_v);
	hyperplane_v = NULL;
}

/*
 *	Allocate memory for numberOfPoints number of points
 */
point_set_t* alloc_point_set(int numberOfPoints)
{
	point_set_t* point_set_v;

	point_set_v = ( point_set_t*)malloc( sizeof( point_set_t));
	memset( point_set_v, 0, sizeof( point_set_t));

	point_set_v->numberOfPoints = numberOfPoints;

	point_set_v->points = (point_t **) malloc(numberOfPoints * sizeof(point_t*));
	memset(point_set_v->points, 0, numberOfPoints * sizeof(point_t*));

	return point_set_v;
}

/*
 *	Release memory for numberOfPoints number of points
 */
void release_point_set(point_set_t* &point_set_v, bool clear)
{
	if(point_set_v == NULL)
		return;

	if(point_set_v->points != NULL)
	{
		if(clear)
		{
			for(int i = 0; i < point_set_v->numberOfPoints; i++)
				release_point( point_set_v->points[i]);
		}

		free(point_set_v->points);
		point_set_v->points = NULL;
	}

	free(point_set_v);
	point_set_v = NULL;
}

/*
*	For degug purpose, print the coordinates for a given point
*/
void print_point(point_t* point_v)
{
	//printf("printing point:\n");
	//printf("%d ", point_v->id);
	for (int i = 0; i < point_v->dim; i++)
		printf("%f ", point_v->coord[i]);
	printf("\n");
}

/*
 *	For debug purose, print the contents of a given point set
 */
void print_point_set(point_set_t* point_set_v)
{
	for(int i = 0; i < point_set_v->numberOfPoints; i++)
		//printf("%i ", point_set_v->points[i]->id);
		print_point(point_set_v->points[i]);

	printf("\n");
}
