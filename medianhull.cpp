#include "medianhull.h"

#ifdef WIN32
#ifdef __cplusplus 
	extern "C" { 
#endif 
#endif

//#include "data_utility.h"

#include "mem.h"
#include "qset.h"
#include "libqhull.h"
#include "qhull_a.h"

#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#if __MWERKS__ && __POWERPC__
#include <SIOUX.h>
#include <Files.h>
#include <console.h>
#include <Desk.h>

#elif __cplusplus
extern "C" {
  int isatty(int);
}

#elif _MSC_VER
#include <io.h>
#define isatty _isatty
int _isatty(int);

#else
int isatty(int);  /* returns 1 if stdin is a tty
                   if "Undefined symbol" this can be deleted along with call in main() */
#endif

#ifdef WIN32
#ifdef __cplusplus 
	}  
#endif
#endif

// get the set of vertices of Conv(P)
vector<point_t*> getVertices(point_set_t* P)
{
	int n = P->numberOfPoints;
	int dim = P->points[0]->dim;
	vector<point_t*> vertices;

	if(dim != 2)
	{
		printf("dim != 2\n");
		exit(0);
	}

	int curlong, totlong; /* used !qh_NOmem */
	int exitcode;
	boolT ismalloc = True;

	coordT *points;
	//temp_points = new coordT[(orthNum * S->numberOfPoints + 1)*(dim)];
	points = qh temp_malloc = (coordT*)qh_malloc((n+3)*(dim)*sizeof(coordT));
	int count = 1;

	for(int i = 0; i < dim; i++)
	{
		points[i] = 0;
	}

	double max[2];
	for(int i = 0; i < 2; i++)
	{
		max[i] = P->points[0]->coord[i];
		for(int j = 1; j < n; j++)
		{
			if(max[i] < P->points[j]->coord[i])
				max[i] = P->points[j]->coord[i];
		}
	}

	for(int j = 0; j < 2; j++)
	{
		for(int i = 0; i < dim; i++)
		{
			if(i == j)
				points[i + count * dim] = max[i];
			else
				points[i + count * dim] = 0;
			//printf("%d, %lf\n",i + count * dim, points[i + count * dim]);
		}
		count++;
	}

	//for(int i = 0; i< count; i++)
	//{
	//	for(int j = 0; j < dim; j++)
	//		printf("%lf ", points[i* dim + j]);
	//	printf("\n");
	//}

	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < dim; j++)
			points[count*dim + j] = P->points[i]->coord[j];
		count++;
	}

	//for(int i = 0; i< count; i++)
	//{
	//	for(int j = 0; j < dim; j++)
	//		printf("%lf ", points[i* dim + j]);
	//	printf("\n");
	//}

	//printf("# of points: %d\n", count);
	qh_init_A(stdin, stdout, stderr, 0, NULL);  /* sets qh qhull_command */
	exitcode = setjmp(qh errexit); /* simple statement for CRAY J916 */

	if (!exitcode) {

		qh_initflags(qh qhull_command);
		qh_init_B(points, count, dim, ismalloc);
		qh_qhull();
		qh_check_output();

		//printf ("\n%d vertices and %d facets with normals:\n",
        //         qh num_vertices, qh num_facets);

		if (qh VERIFYoutput && !qh FORCEoutput && !qh STOPpoint && !qh STOPcone)
			qh_check_points();
		exitcode = qh_ERRnone;

		vertexT *vertex;
		

		FORALLvertices
		{
			int index;
			point_t* pt = getPoint(P, vertex->point);
			
			if(pt != NULL)
				vertices.push_back(pt);
		}

	}

	qh NOerrexit = True;  /* no more setjmp */
#ifdef qh_NOmem
	qh_freeqhull(True);
#else
	qh_freeqhull(False);
	qh_memfreeshort(&curlong, &totlong);
	if (curlong || totlong)
		fprintf(stderr, "qhull internal warning (main): did not free %d bytes of long memory(%d pieces)\n",
			totlong, curlong);
#endif

	return vertices;
}

// Algorithm Median
point_t* medianVertices(point_set_t* P, point_t* u, int maxRound, double &Qcount, double &Csize)
{
	int n = P->numberOfPoints;
	int dim = P->points[0]->dim;

	if(dim != 2)
	{
		printf("median requires dim = 2\n");
		exit(0);
	}

	Qcount = 0;

	vector<point_t*> vertices = getVertices(P);

	sort(vertices.begin(), vertices.end(), angleCmp());

	//for(int i = 0; i < vertices.size(); i++)
	//{
	//	print_point(vertices[i]);
	//}

	int start = 0;
	int end = vertices.size();

	while(end - start != 1 && Qcount < maxRound)
	{
		int mid = start + (end - start-1)/2;

		if(dot_prod(u, vertices[mid]) >= dot_prod(u, vertices[mid+1]))
			end = mid + 1;
		else
			start = mid + 1;

		Qcount++;
	}
	
	Csize = end - start;

	int current_best = (start + end - 1)/2;

	return vertices[current_best];
}

// Algorithm Hull
point_t* hullVertices(point_set_t* P, point_t* u, int s, int maxRound, double &Qcount, double &Csize)
{
	int n = P->numberOfPoints;
	int dim = P->points[0]->dim;


	if(dim != 2)
	{
		printf("hull requires dim = 2\n");
		exit(0);
	}

	Qcount = 0;

	vector<point_t*> vertices = getVertices(P);

	sort(vertices.begin(), vertices.end(), angleCmp());

	//for(int i = 0; i < vertices.size(); i++)
	//{
	//	print_point(vertices[i]);
	//}

	int start = 0;
	int end = vertices.size();

	while(end - start != 1 && Qcount < maxRound)
	{
		vector<int> S_idx;

		for(int i = 0; i < s; i++)
		{
			S_idx.push_back(start + (end - start - 1)/(s+1) * (i+1));
		}

		//for(int i = 0; i < s; i++)
		//	printf("%d\n", S_idx[i]);

		double max_utility = 0;
		int max_i = -1;
		if( end - start > s)
		{

			for(int i = 0; i < s; i++)
			{
				double new_utility = dot_prod(u, vertices[S_idx[i]]);
				if(max_utility < new_utility)
				{
					max_utility = new_utility;
					max_i = i;
				}
			}

			start = (max_i == 0)? start: S_idx[max_i-1]+1;
			end = (max_i == s-1)? end :  S_idx[max_i+1]+1;
		}
		else
		{
			for(int i = start; i < end; i++)
			{
				double new_utility = dot_prod(u, vertices[i]);
				if(max_utility < new_utility)
				{
					max_utility = new_utility;
					max_i = i;
				}
			}

			start = max_i;
			end = max_i + 1;
		}
		Qcount++;
	}


	Csize = end - start;

	int current_best = (start + end - 1)/2;

	return vertices[current_best];
}