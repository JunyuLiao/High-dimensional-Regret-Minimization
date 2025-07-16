#include "data_utility.h"

#include "operation.h"
#include "sphere.h"
#include "GeoGreedy.h"
#include "DMM.h"
#include "lp.h"
#include "highdim.h"


// #ifndef WIN32
// #include <sys/time.h>
// #else
// #include <ctime>
// #endif

#include <iostream>
using namespace std;
#include "stdlib.h"
#include "stdio.h"
#include "time.h"
#include <cmath>
#include <chrono>
#include <random>
#include <set>
#include <vector>
#include <algorithm>


//interactive version
int main(int argc, char *argv[]){
	if (argc != 7) return 0;
	char* input = argv[1];
	point_set_t* P = read_points(input);
	int n = P->numberOfPoints;
	linear_normalize(P);
	reduce_to_unit(P);
	int d = P->points[0]->dim;
	point_set_t* skyline = skyline_point(P);
	int size = 2; // question size
	int d_prime = atoi(argv[2]); // number of dimensions in the utility vector
	int d_hat = atoi(argv[3]); // number of dimensions in the cover in phase 1 (d_hat_1)
	int d_hat_2 = atoi(argv[4]); // number of dimensions in the cover in phase 3, attribute subset method
	// int rounds = atoi(argv[5]); // number of random samples in attribute subset method
	int K = atoi(argv[5]); // return size of the attribute subset method
	int d_bar = 5;
	int num_questions = atoi(argv[6]); // number of questions allowed
	int num_quest_init = num_questions; // keep the initial amount of questions allowed
	// below are default parameters from the interactive paper
	//-------------------------------------
	int s = 3; //question size in interactive algorithm (3 as default)
	double epsilon = 0;
	int maxRound = 1000;
	double Qcount, Csize;
	int prune_option = RTREE;
	int dom_option = HYPER_PLANE;
	int stop_option = EXACT_BOUND;
	int cmp_option = RANDOM;
	//-------------------------------------
	//set n  to be the number of skyline points
	n = skyline->numberOfPoints;

	point_t* u = alloc_point(d);
	// Initialize all coordinates to 0
	for (int i = 0; i < d; i++) u->coord[i] = 0;

	// Randomly select d_prime dimensions to have non-zero values
	std::vector<int> indices(d);
	for (int i = 0; i < d; i++) indices[i] = i;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::shuffle(indices.begin(), indices.end(), gen);

	// Generate random values
	double sum = 0;
	for (int i = 0; i < d_prime; i++) {
		u->coord[indices[i]] = ((double)rand()) / RAND_MAX;
		sum += u->coord[indices[i]];
	}

	// normalize the utility vector
	for (int i = 0; i < d; i++) {
		u->coord[i] /= sum;
	}

	// look for the ground truth maximum utility point
	int maxIdx = 0;
	double maxValue = 0;
	for(int i = 0; i < skyline->numberOfPoints; i++)
	{
		double value = dot_prod(u, skyline->points[i]);
		if(value > maxValue)
		{
			maxValue = value;
			maxIdx = i;
		}
	}


	highdim_output* h = interactive_highdim(skyline, size, d_bar, d_hat, d_hat_2, u, K, s, epsilon, maxRound, Qcount, Csize, cmp_option, stop_option, prune_option, dom_option, num_questions);
	printf("number of questions asked: %d\n", num_quest_init-num_questions); // 555

	point_set_t* S = h->S;
	std::set<int> final_dimensions = h->final_dimensions;

	// evaluation
	// printf("-----------------------------------------------\n"); // 777
	if (S->numberOfPoints == 1){
		printf("|%15s |%15s |%10s |\n", "Method", "# of Questions", "Point #ID");
		printf("-----------------------------------------------\n");
		printf("|%15s |%15s |%10d |\n", "Ground Truth", "-", skyline->points[maxIdx]->id);
		printf("-----------------------------------------------\n");
		printf("|%15s |%15.0lf |%10d |\n", "UH-Random", Qcount, S->points[0]->id);
	}
	else{

		// for comparison, test the mrr returned by the Sphere algorithm
		// construct the dataset with the final dimensions
		point_set_t* D_test = alloc_point_set(n);
		for (int i = 0; i < n; i++) {
			D_test->points[i] = alloc_point(final_dimensions.size());
			D_test->points[i]->id = skyline->points[i]->id;
			int j = 0;
			for (auto dim : final_dimensions) {
				D_test->points[i]->coord[j++] = skyline->points[i]->coord[dim];
			}
		}
		point_set_t* skyline_D_test = skyline_point(D_test);
		point_set_t* S_test = sphereWSImpLP(skyline_D_test, K);

		// revert S_test to the original dimensions
		for (int i = 0; i < S_test->numberOfPoints; i++){
			S_test->points[i] = skyline->points[S_test->points[i]->id];
		}
		double mrr_test = evaluateLP(skyline, S_test, 0, u);
		printf("output size of Sphere: %d | RR of Sphere: %f\n", S_test->numberOfPoints, mrr_test);
		printf("-----------------------------------------------\n");
		double mrr = evaluateLP(skyline, S, 0, u);
		printf("output size of Attsub: %d | RR of Attsub: %f\n", S->numberOfPoints, mrr);
	}

	release_point_set(skyline, false);
	release_point(u);
	release_point_set(h->S, false);
	release_point_set(P, true);
	delete h;
	
	return 0;
}
	