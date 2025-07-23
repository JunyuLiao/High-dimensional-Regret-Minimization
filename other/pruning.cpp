#include "pruning.h"

char hidden_options[]=" d n v Qbb QbB Qf Qg Qm Qr QR Qv Qx Qz TR E V Fa FA FC FD FS Ft FV Gt Q0 Q1 Q2 Q3 Q4 Q5 Q6 Q7 Q8 Q9 ";

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

// conduct half space intersection by invoking Qhull based on the data from rPtr and write results to wPtr
int halfspace(FILE* rPtr, FILE* wPtr) {
	int curlong, totlong; /* used !qh_NOmem */
	int exitcode, numpoints, dim;
	coordT *points;
	boolT ismalloc;

	// the required parameters
  	int argc = 3;
	char* argv[3];
	argv[0] = "qhalf";
	argv[1] = "Fp";
	argv[2] = "Fx";

	qh_init_A(rPtr, wPtr, stderr, argc, argv);  /* sets qh qhull_command */
	exitcode= setjmp(qh errexit); /* simple statement for CRAY J916 */
	if (!exitcode) {
		qh NOerrexit = False;
		qh_option("Halfspace", NULL, NULL);
		qh HALFspace= True;    /* 'H'   */
		qh_checkflags(qh qhull_command, hidden_options);
		qh_initflags(qh qhull_command);

		points= qh_readpoints(&numpoints, &dim, &ismalloc);

		//for(int i = 0; i < numpoints; i++)
		//{
		//	for(int j = 0; j < dim; j++)
		//	{
		//		printf("%lf ", points[i* dim + j]);
		//	}
		//	printf("\n");
		//}

		if (dim >= 5) {
			qh_option("Qxact_merge", NULL, NULL);
			qh MERGEexact= True; /* 'Qx' always */
		}
		qh_init_B(points, numpoints, dim, ismalloc);
		qh_qhull();
		qh_check_output();
		qh_produce_output();
		//print_summary();

		if (qh VERIFYoutput && !qh FORCEoutput && !qh STOPpoint && !qh STOPcone)
			qh_check_points();
		exitcode= qh_ERRnone;
	}
	qh NOerrexit= True;  /* no more setjmp */
	#ifdef qh_NOmem
	qh_freeqhull(qh_ALL);
	#else
	qh_freeqhull(!qh_ALL);
	qh_memfreeshort(&curlong, &totlong);
	if (curlong || totlong)
	fprintf(stderr, "qhull internal warning (main): did not free %d bytes of long memory(%d pieces)\n",
		totlong, curlong);
	#endif

	return exitcode;
} /* main */

// get the set of extreme points of the candidate utility range R (bounded by the extreme vectors)
vector<point_t*> get_extreme_pts(vector<point_t*>& ext_vec)
{
	int dim = ext_vec[0]->dim;
	char file1[MAX_FILENAME_LENG];
	sprintf(file1, "output/hyperplane_data");
	char file2[MAX_FILENAME_LENG];
	sprintf(file2, "output/ext_pt");

	// construct the hyperplanes and a feasible point
	vector<hyperplane_t*> utility_hyperplane;
	point_t* normal;
	normal = alloc_point(dim);
	for(int i = 0; i < dim; i++)
		normal->coord[i] = 1;
	utility_hyperplane.push_back( alloc_hyperplane(normal, -1));
	for(int i = 0; i < ext_vec.size(); i++)
	{
		normal = copy(ext_vec[i]);
		utility_hyperplane.push_back( alloc_hyperplane(normal, 0));
	}
	point_t* feasible_pt = find_feasible(utility_hyperplane);

	// prepare the file for computing the convex hull (the candidate utility range R) via half space interaction
	write_hyperplanes(utility_hyperplane, feasible_pt, file1);
	for(int i = 0; i < utility_hyperplane.size(); i++)
		release_hyperplane(utility_hyperplane[i]);

	// write hyperplanes and the feasible point to file1, conduct half space intersection and write reulsts to file2
	FILE* rPtr;
	FILE* wPtr;
	if ((rPtr = fopen(file1, "r")) == NULL)
	{
		fprintf(stderr, "Cannot open the data file.\n");
		exit(0);
	}
	wPtr = (FILE *)fopen(file2, "w");
	halfspace(rPtr, wPtr);
	fclose(rPtr);
	fclose(wPtr);

	//read extreme points in file2
	if ((rPtr = fopen(file2, "r")) == NULL)
	{
		fprintf(stderr, "Cannot open the data file %s.\n", file2);
		exit(0);
	}
	int size;
	vector<point_t*> ext_pts;
	fscanf(rPtr, "%i%i", &dim, &size);
	for (int i = 0; i < size; i++)
	{
		bool allZero = true;
		point_t* p = alloc_point(dim);
		for (int j = 0; j < dim; j++)
		{
			fscanf(rPtr, "%lf", &p->coord[j]);
			if(!isZero(p->coord[j]))
				allZero = false;
		}
		if(allZero)
			release_point(p);
		else
		{
			ext_pts.push_back(p);
			//print_point(p);
		}
	}

	// update the set of extreme vectors
	vector<point_t*> new_ext_vec;
	fscanf(rPtr, "%i", &size);
	for (int i = 0; i < size; i++)
	{
		int idx;
		fscanf(rPtr, "%i", &idx);

		if(idx > 0)
			new_ext_vec.push_back(copy(ext_vec[idx - 1]));
	}
	for(int i = 0; i < ext_vec.size(); i++)
	{
		release_point(ext_vec[i]);
	}
	ext_vec = new_ext_vec;

	fclose(rPtr);
	return ext_pts;
}

void print_summary(void) {
	facetT *facet;
	int k;

	printf("\n%d vertices and %d facets with normals:\n",
		qh num_vertices, qh num_facets);

	FORALLfacets{
		for (k = 0; k < qh hull_dim; k++)
		printf("%lf\t", facet->normal[k]);

	printf("%lf\t%d\n", facet->offset, facet->id);
	}

}

// get bounding hyperplanes of the conical hull (used in the conical hull pruning)
void get_hyperplanes(vector<point_t*>& ext_vec, hyperplane_t*& hp, vector<point_t*>& hyperplanes)
{
	//constuct non-trivial extreme vectors
	vector<int> frame;
	frameConeLP(ext_vec, frame);
	vector<point_t*> new_ext_vec;
	for(int i = 0; i < frame.size(); i++)
		new_ext_vec.push_back(copy(ext_vec[frame[i]]));
	for(int i = 0; i < ext_vec.size(); i ++)
		release_point(ext_vec[i]);
	ext_vec = new_ext_vec;

	int dim = ext_vec[0]->dim;
	
	// used in the necessary condiditon of conical hull pruning
	double offset = 0;
	point_t* normal = alloc_point(dim);
	for(int i = 0; i < dim; i++)
		normal->coord[i] = 0;

	
	if(1)
	{
		for(int i = 0; i < ext_vec.size(); i++)
		{
			point_t* minus = scale(-1, ext_vec[i]);
			double len;
			point_t* pi = alloc_point(dim);

			solveLP(ext_vec, minus, len, pi);

			point_t* new_normal = add(normal, pi);

			offset = dot_prod(normal, ext_vec[0]);
			for (int i = 1; i < ext_vec.size(); i++)
			{
				double temp = dot_prod(normal, ext_vec[i]);
				if (temp > offset)
					offset = temp;
			}

			release_point(pi);
			release_point(minus);
			release_point(normal);

			normal = new_normal;

			if(offset < 0 && !isZero(offset))
				break;
		}

		double length = calc_len(normal);
		for(int i = 0; i < dim; i++)
			normal->coord[i] /= -length;
		offset = dot_prod(normal, ext_vec[0]);
		for (int i = 1; i < ext_vec.size(); i++)
		{
			double temp = dot_prod(normal, ext_vec[i]);
			if (temp < offset)
				offset = temp;
		}	
	}

	// the hyperplane for the necessary condiditon of conical hull pruning
	hp = alloc_hyperplane(normal, offset);


	// invoke Qhull for computing the conical hull
	int n = ext_vec.size() + 1;
	int curlong, totlong; /* used !qh_NOmem */
	int exitcode;
	boolT ismalloc = True;

	coordT *points;
	//temp_points = new coordT[(orthNum * S->numberOfPoints + 1)*(dim)];
	points = qh temp_malloc = (coordT*)qh_malloc(n*(dim)*sizeof(coordT));

	for (int i = 0; i < ext_vec.size(); i++)
	{
		//double len = compute_intersection_len(hp, ext_vec[i]);
		//printf("%lf %lf\n", len, calc_len(ext_vec[i]));
		//point_t* tmp = scale( len, ext_vec[i]);
		//for (int j = 0; j < dim; j++)
		//	points[i*dim + j] = tmp->coord[j];
		for (int j = 0; j < dim; j++)
			points[i*dim + j] = ext_vec[i]->coord[j];
	}

	for (int i = 0; i < dim; i++)
	{
		points[ext_vec.size()*dim + i] = 0;
	}

	//printf("# of points: %d\n", count);
	qh_init_A(stdin, stdout, stderr, 0, NULL);  /* sets qh qhull_command */
	exitcode = setjmp(qh errexit); /* simple statement for CRAY J916 */

	double minCR;

	if (!exitcode) {

		//qh POSTmerge = True;
		////qh postmerge_centrum = 0.01;
		//qh premerge_cos = 0.995;

		qh_initflags(qh qhull_command);
		qh_init_B(points, n, dim, ismalloc);
		qh_qhull();
		qh_check_output();


		if (qh VERIFYoutput && !qh FORCEoutput && !qh STOPpoint && !qh STOPcone)
			qh_check_points();
		exitcode = qh_ERRnone;

		//qh_vertexneighbors();
		//print_summary();


		//vertexT *vertex;
		//FORALLvertices
		//{
		//	bool isPt = true;
		//	for (int i = 0; i < dim; i++)
		//	{
		//		if (!isZero(vertex->point[i]))
		//		{
		//			isPt = false;
		//			break;
		//		}
		//	}

		//	if (isPt)
		//	{
		//		facetT *facet, **facetp;
		//		FOREACHfacet_(vertex->neighbors)
		//		{
		//			point_t* normal = alloc_point(dim);
		//			for (int j = 0; j < dim; j++)
		//				normal->coord[j] = facet->normal[j];
		//			hyperplanes.push_back(normal);
		//		}
		//		break;
		//	}
		//}

		// the bounding hyperplaines of the conical hull
		facetT *facet;
		FORALLfacets{

			if(isZero(facet->offset))
			{
				point_t* normal = alloc_point(dim);
				for (int j = 0; j < dim; j++)
					normal->coord[j] = facet->normal[j];
				hyperplanes.push_back(normal);
			}

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

}

// hyperplane pruning
int hyperplane_dom(point_t* p_i, point_t* p_j, vector<point_t*> ext_pts)
{
	int dim = p_i->dim;

	point_t* normal = sub(p_i, p_j);
	
	int below_count = 0;

	// to perform hyperplane pruning, check each extreme points of R
	for(int i = 0; i < ext_pts.size(); i++)
	{
		point_t* ext_pt = ext_pts[i];
		double v = dot_prod(normal, ext_pt);

		if(v < 0 & !isZero(v))
		{
			below_count++;
			break;
		}
	}
	release_point(normal);

	if (below_count == 0)
		return 1;
	else
		return 0;
}

// conical hull pruning
int conical_hull_dom(point_t* p_i, point_t* p_j, hyperplane_t* hp, vector<point_t*> hyperplanes, vector<point_t*> ext_vec)
{
	int dim = p_i->dim;
	int dominate;

	// check the necessary condition
	point_t* minus = sub(p_j, p_i);
	double len = compute_intersection_len(hp, minus);
	if (len < 1 && len > 0 || isZero(len - 1) || isZero(len))
	{
		bool all_below = true;
		for(int i = 0; i < hyperplanes.size(); i++)
		{
			point_t* normal = hyperplanes[i];
			double v = dot_prod(normal, minus);

			if(v > 0 && !isZero(v))
			{
				all_below = false;
				break;
			}
		}
		// check if below all bounding hyperplanes of the conical hull
		if (all_below)
		{
			dominate = 1;
		}
		else
			dominate = 0;
	}
	else
	{
		dominate = 0;
	}

	release_point(minus);

	return dominate;
}

// check whether p_i has a higher uitlity than p_j based on either Hyperplane Prunning or Conical Hull Pruninig (defined by dom_option)
int dom(point_t* p_i, point_t* p_j, vector<point_t*> ext_pts, hyperplane_t* hp, vector<point_t*> hyperplanes, vector<point_t*> ext_vec, int dom_option)
{
	if(dom_option == HYPER_PLANE) // hyperplane pruning
		return hyperplane_dom(p_i, p_j, ext_pts);
	else // conical hull pruning
		return conical_hull_dom(p_i, p_j, hp, hyperplanes, ext_vec);
}

// get an approximate upper bound bound in O(|ext_pts|) time based on the MBR of R
double get_rrbound_approx(vector<point_t*> ext_pts)
{
	if(ext_pts.size() == 0)
		return 1;
	
	int dim = ext_pts[0]->dim;

	// compute the Minimum Bounding Rectangle (MBR)
	double* max = new double[dim];
	double* min = new double[dim];
	for(int i = 0; i < dim; i++)
	{
		max[i] = ext_pts[0]->coord[i];
		min[i] = ext_pts[0]->coord[i];
	}

	for(int i = 1; i < ext_pts.size(); i++)
	{
		point_t* pt = ext_pts[i];

		for(int j = 0; j < dim; j++)
		{
			if(pt->coord[j] > max[j])
				max[j] = pt->coord[j];
			else if(pt->coord[j] < min[j])
				min[j] = pt->coord[j];
		}
	}

	double bound = 0;

	for(int i = 0; i < dim; i++)
		bound += max[i] - min[i];

	bound *= ext_pts[0]->dim;

	delete [] max;
	delete [] min;

	return bound < 1? bound: 1;
}

// get an "exact" upper bound bound in O(|ext_pts|^2) time based on R
double get_rrbound_exact(vector<point_t*> ext_pts)
{
	if(ext_pts.size() == 0)
		return 1;

	double max = 0;

	// find the maximum pairwise L1-distance between the extreme vertices of R
	for(int i = 0; i < ext_pts.size(); i++)
	{
		for(int j = i+1; j < ext_pts.size(); j++)
		{
			double v = calc_l1_dist(ext_pts[i], ext_pts[j]);
			if(v > max)
				max = v;
		}
	}

	max *= ext_pts[0]->dim;

	return max < 1? max : 1;
}

// use the seqentail way for maintaining the candidate set
// P: the input car set
// C_idx: the indexes of the current candidate favorite car in P
// ext_vec: the set of extreme vecotr
// rr: the upper bound of the regret ratio
// stop_option: the stopping condition, which can be NO_BOUND, EXACT_BOUND and APPROX_BOUND
// dom_option: the skyline options, which can be SQL or RTREE
void sql_pruning(point_set_t* P, vector<int>& C_idx, vector<point_t*>& ext_vec, double& rr, int stop_option, int dom_option)
{
	int dim = P->points[0]->dim;

	vector<point_t*> ext_pts;
	vector<point_t*> hyperplanes;
	hyperplane_t* hp = NULL;
	
	if(dom_option == HYPER_PLANE)
		ext_pts = get_extreme_pts(ext_vec); // in Hyperplane Pruning, we need the set of extreme points of R
	else
	{
		// in Conical Pruning, we need bounding hyperplanes for the conical hull
		get_hyperplanes(ext_vec, hp, hyperplanes); 
		if(stop_option != NO_BOUND) // if an upper bound on the regret ratio is needed, we need the set of extreme points of R
			ext_pts = get_extreme_pts(ext_vec);
	}

	// get the upper bound of the regret ratio based on (the extreme ponits of) R
	if(stop_option == EXACT_BOUND)
		rr = get_rrbound_exact(ext_pts);
	else if (stop_option == APPROX_BOUND)
		rr = get_rrbound_approx(ext_pts);
	else 
		rr = 1;

	//printf("extreme vectors:\n");
	//for(int i = 0; i < ext_vec.size(); i++)
	//	print_point(ext_vec[i]);
	//printf("hyperplanes:\n");
	//for(int i = 0; i < hyperplanes.size(); i++)
	//	print_point(hyperplanes[i]);
	//printf("H: offset - %lf\n", hp->offset);
	//print_point(hp->normal);


	// run the adapted squential skyline algorihtm
	int* sl = new int[C_idx.size()];
	int index = 0;

	for (int i = 0; i < C_idx.size(); ++i)
	{

		int dominated = 0;
		point_t* pt = P->points[C_idx[i]];

		// check if pt is dominated by the skyline so far   
		for (int j = 0; j < index && !dominated; ++j)
		{

			if(dom(P->points[ sl[j] ], pt, ext_pts, hp, hyperplanes, ext_vec, dom_option))
				dominated = 1;
		}

		if (!dominated)
		{
			// eliminate any points in current skyline that it dominates
			int m = index;
			index = 0;
			for (int j = 0; j < m; ++j)
			{

				if(!dom(pt, P->points[sl[j]], ext_pts, hp, hyperplanes, ext_vec, dom_option))
					sl[index++] = sl[j];
			}

			// add this point as well
			sl[index++] = C_idx[i];
		}
	}

	C_idx.clear();
	for(int i = 0; i < index; i++)
		C_idx.push_back(sl[i]);

	delete[] sl;

	if(dom_option == HYPER_PLANE)
	{
		for(int i = 0; i < ext_pts.size(); i++)
			release_point(ext_pts[i]);
	}
	else
	{
		release_hyperplane(hp);
		for(int i = 0; i < hyperplanes.size(); i++)
			release_point(hyperplanes[i]);
	}
	
}

// use the branch-and-bound skyline (BBS) algorithm for maintaining the candidate set
// P: the input car set
// C_idx: the indexes of the current candidate favorite car in P
// ext_vec: the set of extreme vecotr
// rr: the upper bound of the regret ratio
// stop_option: the stopping condition, which can be NO_BOUND, EXACT_BOUND and APPROX_BOUND
// dom_option: the skyline options, which can be SQL or RTREE
void rtree_pruning(point_set_t* P, vector<int>& C_idx, vector<point_t*>& ext_vec, double& rr,  int stop_option, int dom_option)
{
	vector<point_t*> ext_pts;
	vector<point_t*> hyperplanes;
	hyperplane_t* hp = NULL;
	
	if(dom_option == HYPER_PLANE)
		ext_pts = get_extreme_pts(ext_vec); // in Hyperplane Pruning, we need the set of extreme points of R
	else
	{
		// in Conical Pruning, we need bounding hyperplanes for the conical hull
		get_hyperplanes(ext_vec, hp, hyperplanes); 
		if(stop_option != NO_BOUND) // if a upper bound on the regret ratio is needed, we need the set of extreme points of R
			ext_pts = get_extreme_pts(ext_vec);
	}
	
	// get the upper bound of the regret ratio based on (the extreme ponits of) R
	if(stop_option == EXACT_BOUND)
		rr = get_rrbound_exact(ext_pts);
	else if (stop_option == APPROX_BOUND)
		rr = get_rrbound_approx(ext_pts);
	else 
		rr = 1;

	// parameters for building the R-trees
	rtree_info *aInfo;
	aInfo = (rtree_info *)malloc(sizeof(rtree_info));
	memset(aInfo, 0, sizeof(rtree_info));
	aInfo->m = 18;
	aInfo->M = 36;
	aInfo->dim = P->points[0]->dim;
	aInfo->reinsert_p = 27;
	aInfo->no_histogram = C_idx.size();

	// construct R-tree
	node_type *root = contructRtree(P, C_idx, aInfo);

	priority_queue<node_type*, vector<node_type*>, nodeCmp> heap;

	heap.push(root);

	int* sl = new int[C_idx.size()];
	int index = 0;
	int dim = aInfo->dim;

	// run the adapted BBS algorihtm
	while (!heap.empty())
	{
		node_type* n = heap.top();
		heap.pop();

		if (n->attribute != LEAF)
		{
			
			int dominated = 0;
			
			point_t* TRpt = alloc_point(dim);
			for (int i = 0; i < dim; i++)
				TRpt->coord[i] = n->b[i];

			// check if TRpt is dominated by the skyline so far   
			for (int j = 0; j < index && !dominated; ++j)
			{
				
				if(dom(P->points[ sl[j] ], TRpt, ext_pts, hp, hyperplanes, ext_vec, dom_option))
					dominated = 1;

			}


			if (!dominated)
			{

				for (int i = 0; i < aInfo->M - n->vacancy; i++)
				{
					//int child_dominated = 0;
					//for (int i = 0; i < dim; i++)
					//	TRpt->coord[i] = n->ptr[i]->b[i];
	
					//for (int j = 0; j < index && !dominated; ++j)
					//	if (hyperplane_dom(P->points[ sl[j] ], TRpt, ext_pts))
					//		child_dominated = 1;
					//
					//if(!child_dominated)
					heap.push(n->ptr[i]);
				}
			}
				
		}
		else
		{
			int idx = n->id;
			//S = updateS(id, C, S, V);

			int dominated = 0;
			for (int j = 0; j < index && !dominated; ++j)
			{
				if(dom(P->points[ sl[j] ], P->points[ C_idx[idx] ], ext_pts, hp, hyperplanes, ext_vec, dom_option))
					dominated = 1;
			}
			if (dominated)
				continue;

			// eliminate any points in current skyline that it dominates
			int m = index;
			index = 0;
			for (int j = 0; j < m; ++j)
			{
				if(!dom(P->points[C_idx[idx]], P->points[sl[j]], ext_pts, hp, hyperplanes, ext_vec, dom_option))
					sl[index++] = sl[j];
			}

			// add this point as well
			sl[index++] = C_idx[idx];
		}
	}
	
	// clean up
	C_idx.clear();
	for(int i = 0; i < index; i++)
		C_idx.push_back(sl[i]);
	delete[] sl;
	free(aInfo);
	if(dom_option == HYPER_PLANE)
	{
		for(int i = 0; i < ext_pts.size(); i++)
			release_point(ext_pts[i]);
	}
	else
	{
		release_hyperplane(hp);
		for(int i = 0; i < hyperplanes.size(); i++)
			release_point(hyperplanes[i]);
	}
}
