#include "other/data_utility.h"

#include "other/operation.h"
#include "other/sphere.h"
#include "other/GeoGreedy.h"
#include "other/DMM.h"
#include "other/lp.h"
#include "highdim.h"
#include "experiment_random.h"


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
#include <fstream>
#include <iomanip>
#include <string>

namespace {

void print_separator(){
	for (int i = 0; i < 40; ++i) printf("-");
	printf("\n");
}

point_t* generate_sparse_utility(int d, int d_prime){
	point_t* u = alloc_point(d);
	for (int i = 0; i < d; i++) u->coord[i] = 0;

	std::vector<int> indices(d);
	for (int i = 0; i < d; i++) indices[i] = i;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::shuffle(indices.begin(), indices.end(), gen);

	double sum = 0;
	for (int i = 0; i < d_prime; i++) {
		u->coord[indices[i]] = ((double)rand()) / RAND_MAX;
		sum += u->coord[indices[i]];
	}

	for (int i = 0; i < d; i++) {
		u->coord[i] /= sum;
	}

	return u;
}

point_t* read_experiment_utility(const std::string& path, int dimension){
	std::ifstream input(path);
	point_t* utility = alloc_point(dimension);
	for (int i = 0; i < dimension; ++i) {
		if (!(input >> utility->coord[i])) {
			release_point(utility);
			return nullptr;
		}
	}
	return utility;
}

void print_experiment_result(const char* method, double regret_ratio, double time_seconds,
		int output_size, int questions){
	std::cout << "EXPERIMENT_RESULT {\"method\":\"" << method
		<< "\",\"status\":\"ok\",\"regret_ratio\":" << std::setprecision(17) << regret_ratio
		<< ",\"time_seconds\":" << time_seconds
		<< ",\"output_size\":" << output_size
		<< ",\"questions\":" << questions << "}" << std::endl;
}

void print_unavailable_experiment_result(const char* method, const char* reason){
	std::cout << "EXPERIMENT_RESULT {\"method\":\"" << method
		<< "\",\"status\":\"unavailable\",\"reason\":\"" << reason << "\"}" << std::endl;
}

point_set_t* construct_sphere_dataset(point_set_t* skyline, const std::set<int>& final_dimensions){
	point_set_t* D_test = alloc_point_set(skyline->numberOfPoints);
	for (int i = 0; i < skyline->numberOfPoints; i++) {
		D_test->points[i] = alloc_point(final_dimensions.size());
		// Set the ID to be the array index so we can map back correctly
		D_test->points[i]->id = i;
		int j = 0;
		for (auto dim : final_dimensions) {
			D_test->points[i]->coord[j++] = skyline->points[i]->coord[dim];
		}
	}
	return D_test;
}

point_set_t* copy_sphere_result_to_original(point_set_t* skyline, point_set_t* skyline_D_test, point_set_t* S_test){
	point_set_t* S_test_original = alloc_point_set(S_test->numberOfPoints);
	int valid_index = 0;
	for (int i = 0; i < S_test->numberOfPoints; i++){
		int target_id = S_test->points[i]->id;

		// Find the point in skyline_D_test with this ID
		int skyline_index = -1;
		for (int j = 0; j < skyline_D_test->numberOfPoints; j++) {
			if (skyline_D_test->points[j]->id == target_id) {
				skyline_index = j;
				break;
			}
		}

		if (skyline_index == -1) {
			continue;
		}

		// Get the corresponding point from D_test (which has the array index)
		point_t* D_test_point = skyline_D_test->points[skyline_index];
		int original_skyline_index = D_test_point->id;

		if (original_skyline_index < 0 || original_skyline_index >= skyline->numberOfPoints) {
			continue;
		}

		// Create a new point with original dimensions
		point_t* original_point = skyline->points[original_skyline_index];
		if (original_point == NULL) {
			continue;
		}

		point_t* new_point = alloc_point(original_point->dim);
		new_point->id = original_point->id;
		for (int j = 0; j < original_point->dim; j++) {
			new_point->coord[j] = original_point->coord[j];
		}
		S_test_original->points[valid_index++] = new_point;
	}
	return S_test_original;
}

} // namespace

//interactive version
int main(int argc, char *argv[]){
	const bool experiment_mode = argc == 11 && std::string(argv[1]) == "--experiment";
	if (!experiment_mode && argc != 7) return 0;
	char* input = experiment_mode ? argv[2] : argv[1];
	point_set_t* P = read_points(input);
	int n = P->numberOfPoints;
	linear_normalize(P);
	// reduce_to_unit(P);
	int d = P->points[0]->dim;
	point_set_t* skyline;
	if (experiment_mode) {
		skyline = alloc_point_set(P->numberOfPoints);
		for (int i = 0; i < P->numberOfPoints; ++i) skyline->points[i] = P->points[i];
	}
	else {
		skyline = skyline_point(P);
	}
	printf("number of skyline points: %d\n", skyline->numberOfPoints);

	int size = 2; // question size
	int argument_offset = experiment_mode ? 1 : 0;
	int d_prime = atoi(argv[2 + argument_offset]); // number of dimensions in the utility vector
	int d_hat = atoi(argv[3 + argument_offset]); // number of dimensions in the cover in phase 1 (d_hat_1)
	int d_hat_2 = atoi(argv[4 + argument_offset]); // number of dimensions in the cover in phase 3, attribute subset method
	// int rounds = atoi(argv[5]); // number of random samples in attribute subset method
	int K = atoi(argv[5 + argument_offset]); // return size of the attribute subset method
	int d_bar = 5;
	int num_questions = atoi(argv[6 + argument_offset]); // number of questions allowed
	int num_quest_init = num_questions; // keep the initial amount of questions allowed
	// below are default parameters from the interactive paper
	//-------------------------------------
	int s = 3; //question size in interactive algorithm (3 as default)
	double epsilon = 0.0;
	int maxRound = 1000;
	double Qcount, Csize;
	int prune_option = RTREE;
	int dom_option = HYPER_PLANE;
	int stop_option = EXACT_BOUND;
	int cmp_option = RANDOM;
	//-------------------------------------
	//set n  to be the number of skyline points
	n = skyline->numberOfPoints;

	point_t* u;
	bool skip_sphere = false;
	if (experiment_mode) {
		seed_experiment_random(static_cast<unsigned int>(std::stoul(argv[9])));
		u = read_experiment_utility(argv[8], d);
		skip_sphere = atoi(argv[10]) != 0;
		if (u == nullptr) {
			std::cerr << "Error: invalid utility file " << argv[8] << "\n";
			release_point_set(skyline, false);
			release_point_set(P, true);
			return 2;
		}
	}
	else {
		u = generate_sparse_utility(d, d_prime);
	}

	highdim_output* h = interactive_highdim(skyline, size, d_bar, d_hat, d_hat_2, u, K, s, epsilon, maxRound, Qcount, Csize, cmp_option, stop_option, prune_option, dom_option, num_questions);
	double time_12 = h->time_12;
	double time_3 = h->time_3;
	
	point_set_t* S = h->S;
	std::set<int> final_dimensions = h->final_dimensions;

	// evaluation
	// for comparison, evaluate the performance of Sphere. the return size is either
	// (# questions asked in interactive algorithm) * s (Phase 3B) or K (Phase 3A)
	int K_sphere;
	if (S->numberOfPoints == 1){
		printf("Phase 3A: \n");
		K_sphere =  Qcount * s;

	}
	else{
		printf("Phase 3B: \n");
		K_sphere = K;
	}

	print_separator();
	printf("|%7s |%13s |%5s | %5s |\n", "Method", "Regret Ratio", "Time", "Size");
	print_separator();
	double mrr = evaluateLP(skyline, S, 0, u);
	printf("|%7s |%13.3lf |%5.2lf | %5d |\n", "OurAlg", mrr, time_12+time_3, S->numberOfPoints);
	int questions_used = num_quest_init-num_questions;
	if (experiment_mode) {
		print_experiment_result("FHDR", mrr, time_12+time_3, S->numberOfPoints, questions_used);
	}

	// for comparison, test the mrr returned by the Sphere algorithm
	// construct the dataset with the final dimensions
	point_set_t* D_test = construct_sphere_dataset(skyline, final_dimensions);
	// record the time in seconds
	if (!skip_sphere && final_dimensions.size() <= K_sphere){
		auto start_time_sphere = std::chrono::high_resolution_clock::now();
		point_set_t* skyline_D_test = skyline_point(D_test);
		point_set_t* S_test = sphereWSImpLP(skyline_D_test, K_sphere);
		auto end_time_sphere = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> duration_sphere = end_time_sphere - start_time_sphere;
		double time_sphere = duration_sphere.count();

		point_set_t* S_test_original = copy_sphere_result_to_original(skyline, skyline_D_test, S_test);
		double mrr_test = evaluateLP(skyline, S_test_original, 0, u);
		
		print_separator();
		printf("|%7s |%13.3lf |%5.2lf | %5d |\n", "Sphere", mrr_test, time_12+time_sphere, S_test->numberOfPoints);
		if (experiment_mode) {
			print_experiment_result("Sphere-Adapt", mrr_test, time_12+time_sphere,
				S_test->numberOfPoints, questions_used);
		}
	    
		release_point_set(skyline_D_test, false);
		release_point_set(S_test, false); // Don't clear since points are references
		release_point_set(S_test_original, true);
	}
	else if (experiment_mode) {
		print_unavailable_experiment_result("Sphere-Adapt",
			skip_sphere ? "skipped_by_experiment_policy" : "candidate_dimension_count_exceeds_output_size");
	}
	
	


	// Create a new point set with original dimensions
	// The IDs in S_test->points[i] are the original IDs from the input points
	// We need to find the points in skyline_D_test that have those IDs
	
	print_separator();
	printf("number of questions: %d\n", questions_used); // 555

	release_point_set(skyline, false);
	release_point(u);
	release_point_set(h->S, false);
	release_point_set(P, true);
	release_point_set(D_test, true);
	delete h;

	return 0;
}
