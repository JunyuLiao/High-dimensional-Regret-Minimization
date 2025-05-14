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
	if (argc != 4)
		return 0;

	char* input = argv[1];
	point_set_t* P = read_points(input);
	int n = P->numberOfPoints;
	// printf("num of points: %d\n", n); // 111
	linear_normalize(P);
	reduce_to_unit(P);
	// save the P to a file named data_{num of dimension}.txt
	// FILE* f = fopen(("data_" + to_string(P->points[0]->dim) + ".txt").c_str(), "w");
	// for (int i = 0; i < n; i++) {
	// 	for (int j = 0; j < P->points[i]->dim; j++) {
	// 		fprintf(f, "%f ", P->points[i]->coord[j]);
	// 	}
	// 	fprintf(f, "\n");
	// }	
	// fclose(f);	
	// P = remove_outliers(P);
	int d = P->points[0]->dim;
	// printf("dim of points: %d\n", d); // 222
	point_set_t* skyline = skyline_point(P);
	// print the number of points
	// printf("num of skyline points: %d\n", skyline->numberOfPoints); // 333
	int size = 5; // question size
	int d_prime = 3; 
	int d_hat = 7; // number of dimensions in the cover in phase 1
	int d_hat_2 = atoi(argv[2]); // number of dimensions in the cover in phase 3, attribute subset method
	int rounds = atoi(argv[3]); // number of random samples in attribute subset method
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
	int num_questions = 0; // keep track of the number of questions asked
	int test_rounds = 30; // number of trials in the evaluation function

	//set n  to be the number of skyline points
	n = skyline->numberOfPoints;

	//parameter for interactive
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


	// inspect the utility vector for its non-zero coordinates
	// 444
	// printf("non-zero coordinates of the utility vector: ");
	// for (int i = 0; i < d; i++) {
	// 	if (u->coord[i] != 0) {
	// 		printf("%d ", i);
	// 	}
	// }
	// printf("\n");

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


	highdim_output* h = interactive_highdim(skyline, size, d_prime, d_hat, d_hat_2, u, rounds, s, epsilon, maxRound, Qcount, Csize, cmp_option, stop_option, prune_option, dom_option, num_questions);
	// printf("\nnumber of questions to determine the dimensions: %d\n", num_questions); // 555

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
		// printf("final output size: %d\n", S->numberOfPoints); // 666
		double mrr = evaluateLP(skyline, S, 0, d_prime, final_dimensions, test_rounds);
		// printf("mrr: %f\n", mrr); // 777
	}

	release_point_set(skyline, false);
	release_point(u);
	release_point_set(h->S, false);
	release_point_set(P, true);
	delete h;
	
	return 0;
}
	