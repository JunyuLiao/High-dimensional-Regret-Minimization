#include "highdim.h"

namespace {

std::vector<int> dimension_range(int start, int count){
    std::vector<int> dimensions;
    for (int i = 0; i < count; ++i) {
        dimensions.push_back(start + i);
    }
    return dimensions;
}

std::vector<int> selected_dimension_slice(const std::set<int>& selected_dimensions, int start, int count){
    std::vector<int> dimensions;
    for (int i = 0; i < count; ++i) {
        dimensions.push_back(*next(selected_dimensions.begin(), start + i));
    }
    return dimensions;
}

point_set_t* project_points(point_set_t* source, const std::vector<int>& dimensions){
    point_set_t* projected = alloc_point_set(source->numberOfPoints);
    for (int i = 0; i < source->numberOfPoints; ++i){
        projected->points[i] = alloc_point(dimensions.size());
        projected->points[i]->id = source->points[i]->id;
        for (int j = 0; j < dimensions.size(); ++j){
            projected->points[i]->coord[j] = source->points[i]->coord[dimensions[j]];
        }
    }
    return projected;
}

point_set_t* select_random_points(point_set_t* D, int size){
    point_set_t* S = alloc_point_set(size);
    // create a random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, D->numberOfPoints-1);
    // select size points from D
    for (int i = 0; i < size; ++i) {
        int idx = dis(gen);
        S->points[i] = D->points[idx];
    }
    return S;
}

point_t* project_utility(point_t* u, const std::vector<int>& dimensions){
    point_t* u_hat = alloc_point(dimensions.size());
    for (int i = 0; i < dimensions.size(); ++i) {
        u_hat->coord[i] = u->coord[dimensions[i]];
    }
    return u_hat;
}

int simulated_choice_id(point_set_t* S, point_t* u_hat){
    double maxValue = 0;
    int maxIdx = 0;
    for (int i = 0; i < S->numberOfPoints; i++) {
        double value = dot_prod(u_hat, S->points[i]);
        if (value > maxValue) {
            maxValue = value;
            maxIdx = i;
        }
    }
    return maxValue > 0 ? S->points[maxIdx]->id : -1;
}

void record_question(question_mapping& qm, const std::vector<int>& dimensions, point_set_t* S, int id){
    // Record the question: dimensions shown and tuple indices (selected first)
    std::set<int> question_dims(dimensions.begin(), dimensions.end());
    std::vector<int> tuple_indices;
    if (id != -1) {
        // Add selected point first, then others
        tuple_indices.push_back(id);
        for (int i = 0; i < S->numberOfPoints; ++i) {
            if (S->points[i]->id != id) {
                tuple_indices.push_back(S->points[i]->id);
            }
        }
    }
    qm.questions[question_dims] = tuple_indices;
}

int ask_projected_question(point_set_t* skyline, point_t* u, const std::vector<int>& dimensions, int size, question_mapping& qm){
    point_set_t* D = project_points(skyline, dimensions);
    point_set_t* S = select_random_points(D, size);
    point_t* u_hat = project_utility(u, dimensions);
    int id = simulated_choice_id(S, u_hat);

    record_question(qm, dimensions, S, id);

    release_point(u_hat);
    release_point_set(S, false);
    release_point_set(D, true);
    return id;
}

std::set<int> combine_candidate_dimensions(const std::set<int>& final_dimensions, const std::set<int>& selected_dimensions, int d_bar){
    std::set<int> set_final_dimensions;
    if (selected_dimensions.size() == 0 || final_dimensions.size() == d_bar){
        set_final_dimensions = final_dimensions;
    }
    else {
        std::set_union(final_dimensions.begin(), final_dimensions.end(),
            selected_dimensions.begin(), selected_dimensions.end(),
            std::inserter(set_final_dimensions, set_final_dimensions.begin()));
    }
    return set_final_dimensions;
}

std::map<int, int> build_dimension_mapping(const std::set<int>& dimensions){
    std::map<int, int> dim_mapping;
    int reduced_dim = 0;
    for (int dim : dimensions) {
        dim_mapping[dim] = reduced_dim++;
    }
    return dim_mapping;
}

point_t* find_point_by_id(point_set_t* points, int id){
    for (int i = 0; i < points->numberOfPoints; ++i) {
        if (points->points[i]->id == id) {
            return points->points[i];
        }
    }
    return nullptr;
}

point_set_t* map_sphere_result_to_skyline(point_set_t* skyline, point_set_t* S){
    point_set_t* S_output = alloc_point_set(S->numberOfPoints);
    for (int i = 0; i < S->numberOfPoints; ++i){
        // Find the point in skyline with the corresponding id
        int target_id = S->points[i]->id;
        point_t* matched_point = find_point_by_id(skyline, target_id);
        if (matched_point == nullptr) {
            printf("Error: Could not find point in skyline with id %d\n", target_id);
            exit(1);
        }
        S_output->points[i] = matched_point;
    }
    return S_output;
}

} // namespace

highdim_output* interactive_highdim(point_set_t* skyline, int size, int d_bar, int d_hat, int d_hat_2, point_t* u, int K, int s, double epsilon, int maxRound, double& Qcount, double& Csize, int cmp_option, int stop_option, int prune_option, int dom_option, int& num_questions){

    int n = skyline->numberOfPoints;
    int d = skyline->points[0]->dim;
    // record the time for phase 1 and 2
    double time_12 = 0.0;
    auto start_time_12 = std::chrono::high_resolution_clock::now();
    
    // Initialize question mapping to record all questions asked
    question_mapping qm;
    
    // phase 1: narrow down the dimensions
    // printf("Phase 1\n"); // 111
	// store the dimensions if the user is interested in at least one in the set
	std::set<int> selected_dimensions;
    for (int i = 0; i < d; ++i) selected_dimensions.insert(i); // initial all dimensions

    // Keep the original full-block behavior; tail dimensions remain selected when d is not divisible by d_hat.
	for (int i=0;i<d/d_hat;++i){
        // first check if the user is willing to answer questions
        if (num_questions <= 0){
            break; // no more questions left
        }
		//Restrict D to dimensions i*d_hat, i*d_hat+1, ..., i*d_hat+d_hat-1
        std::vector<int> dimensions = dimension_range(i*d_hat, d_hat);
        int id = ask_projected_question(skyline, u, dimensions, size, qm);
        num_questions -= 1;
        
		if (id==-1){
			// for (int j=0;j<d_hat;++j) selected_dimensions.erase(i*d_hat+j);
            for (int j=0;j<d_hat;++j){
                int dim_to_remove = i*d_hat+j;
                selected_dimensions.erase(dim_to_remove);
            }
		}
	}
	// phase 2: narrow down the at most d_hat*d' dimensions further
    // printf("Phase 2\n"); // 222
	// apply the Generalized_binary-splitting_algorithm
	// store the dimensions the user is interested in
	set<int> final_dimensions;
	int d_left = selected_dimensions.size();
    // for debugging purpose
    if (d_left == 0){
        printf("error_1: d_left is 0\n");
        exit(1);
    }
    int d_target = d_bar;
	while (d_left > 0 && num_questions > 0 && final_dimensions.size() < d_bar){
        d_left = selected_dimensions.size();
		if (d_left <= 2*d_target-2){
			for (int i=0;i<d_left;++i){
				// insert interaction
				// show the user the dimension, ask if interested in
				// bool answer = false; //user's answer
                // for the purpose of testing, we will use the utility vector u to select the answer
                // extract the first dimension in selected_dimensions
                // for debugging purpose
                if (selected_dimensions.size() == 0){
                    printf("error_2: selected_dimensions is empty\n");
                    exit(1);
                }
                int current_dim = *selected_dimensions.begin();
                bool answer = u->coord[current_dim] > 0;
                num_questions -= 1;
				if (!answer){
					selected_dimensions.erase(selected_dimensions.begin());
                    d_left -= 1;
				}
				else {
					final_dimensions.insert(current_dim);
					selected_dimensions.erase(selected_dimensions.begin());
                    d_left -= 1;
                    d_target -= 1;

				}
			}
			// break; // unnecessary
		}
		else {
			int l = d_left - d_target + 1;
            int alpha = floor(log2(double(l)/d_target));
            int group_size = pow(2, alpha);
            // for debugging purpose
            if (group_size == 0){
                printf("error_3: group_size is 0\n");
                exit(1);
            }
            if (group_size > d_left){
                printf("error_4: group_size is greater than d_left\n");
                exit(1);
            }
            int id;
            if (group_size == 1){
                id = u->coord[*selected_dimensions.begin()] > 0 ? skyline->points[0]->id : -1;
                num_questions -= 1;
            }
            else {
                // select the first size dimensions
                std::vector<int> dimensions = selected_dimension_slice(selected_dimensions, 0, group_size);
                id = ask_projected_question(skyline, u, dimensions, size, qm);
                num_questions -= 1;
            }
			if (id == -1){
				for (int j=0;j<group_size;++j){
					int dim_to_remove = *selected_dimensions.begin();
					selected_dimensions.erase(selected_dimensions.begin());
				}
				d_left -= group_size;
				continue;
			}
			else {
                // check if the user is willing to answer more questions
                if (num_questions <= 0) break;
                if (group_size == 1){
                    final_dimensions.insert(*selected_dimensions.begin());
                    selected_dimensions.erase(selected_dimensions.begin());
                    d_left -= 1;
                    d_target -= 1;
                }
                else{
                    // do a binary search to find one dimension
                    int left = 0;
                    int right = group_size-1;
                    while (left < right && num_questions > 0){
                        int mid = left + (right - left) / 2;
                        if (mid == left){
                            if (right == left){
                                // one dimension has been found
                                final_dimensions.insert(*next(selected_dimensions.begin(), mid));
                                selected_dimensions.erase(next(selected_dimensions.begin(), mid));
                                d_left -= 1;
                                d_target -= 1;
                                break;
                            }
                            else {
                                // check whether the dimension is in the left half or the right half
                                if (u->coord[*next(selected_dimensions.begin(), left)] > 0){
                                    num_questions -= 1;
                                    // found the dimension
                                    final_dimensions.insert(*next(selected_dimensions.begin(), mid));
                                    selected_dimensions.erase(next(selected_dimensions.begin(), mid));
                                    d_left -= 1;
                                    d_target -= 1;
                                }
                                // if (u->coord[*next(selected_dimensions.begin(), right)] > 0){
                                else{
                                    num_questions -= 1;
                                    // the dimension is in the right half
                                    final_dimensions.insert(*next(selected_dimensions.begin(), right));
                                    selected_dimensions.erase(next(selected_dimensions.begin(), right));
                                    d_left -= 1;
                                    d_target -= 1;
                                }
                                break;
                            }
                        }
                        std::vector<int> dimensions = selected_dimension_slice(selected_dimensions, left, mid - left + 1);
                        int id = ask_projected_question(skyline, u, dimensions, size, qm);
                        num_questions -= 1;
                        
                        if (id == -1){
                            // the dimension is in the right half, remove all dimensions in the left half
                            for (int j=0; j < mid-left+1; ++j){
                                int dim_to_remove = *next(selected_dimensions.begin(), left);
                                selected_dimensions.erase(next(selected_dimensions.begin(), left));
                                d_left -= 1;
                            }
                            // left = mid+1;
                            right = right - mid + left - 1; // adjust the right pointer (since we are now operating a set)
                        }
                        else {
                            // the dimension is in the left half, however cannot remove dimensions in the right half
                            // for debugging purpose
                            if (right == mid){
                                printf("error_5: right == mid\n");
                                exit(1);
                            }
                            right = mid;
                        }
                    }
                }
			}
		}
	}
    auto end_time_12 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_12 = end_time_12 - start_time_12;
    time_12 = duration_12.count();
	// phase 3: find the optimal tuple or the optimal subset
    // printf("Phase 3\n"); // 444
	// take the union of the final_dimensions and the selected_dimensions
	std::set<int> set_final_dimensions = combine_candidate_dimensions(final_dimensions, selected_dimensions, d_bar);
	// construct the final subset
	point_set_t* S_output = nullptr;

	int final_d = set_final_dimensions.size();
    printf("number of dimensions left in Candidate Set: %d\n", final_d);
    std::vector<int> final_dimension_list(set_final_dimensions.begin(), set_final_dimensions.end());
	point_set_t* D_prime = project_points(skyline, final_dimension_list);

    // take the skyline of the newly constructed dataset D_prime
    point_set_t* skyline_D_prime = skyline_point(D_prime);


    // printf("number of points in skyline_D: %d\n", skyline_D_prime->numberOfPoints);
    // construct the final u
    point_t* u_final = project_utility(u, final_dimension_list);

    double time_3 = 0.0;
    auto start_time_3 = std::chrono::high_resolution_clock::now();
	if (num_questions > 0){
		// apply the interactive code to select the optimal tuple
        // printf("Applying the interactive algorithm to select the optimal tuple\n");
        S_output = alloc_point_set(1);
        // if (skyline_D_prime->points[0]->dim == final_d) printf("dimension: %d\n", final_d);
        
        // Create a mapping from original dimensions to reduced dimensions
        std::map<int, int> dim_mapping = build_dimension_mapping(set_final_dimensions);
        
        // Use max_utility_with_questions instead of max_utility to incorporate pre-recorded questions
        point_t* opt_p = max_utility_with_questions(skyline_D_prime, u_final, s, epsilon, num_questions, Qcount, Csize, cmp_option, stop_option, prune_option, dom_option, qm, dim_mapping, D_prime);
        // Find the point in skyline that matches the id of opt_p
        point_t* matched_point = find_point_by_id(skyline, opt_p->id);
        if (matched_point == nullptr) {
            printf("Error: Could not find point in skyline with id %d\n", opt_p->id);
            exit(1);
        }
        S_output->points[0] = matched_point;
        num_questions -= Qcount;
    }
	else {
        // if (final_d >= K){
        //     // exit
        //     printf("error: final_d >= K\n");
        //     exit(1);
        // }
        // printf("Applying the attribute subset method to output a regret minimizing subset\n");
        if (final_d <= d_hat_2){
            point_set_t* S = sphereWSImpLP(skyline_D_prime, K);
            S_output = map_sphere_result_to_skyline(skyline, S);
            release_point_set(S, false);
        }
        else {
            S_output = attribute_subset(skyline, S_output, final_d, d_hat_2, K, set_final_dimensions);
        }
	}

    auto end_time_3 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_3 = end_time_3 - start_time_3;
    time_3 = duration_3.count();
    
    // Safety check: ensure S_output is not null
    if (S_output == nullptr) {
        printf("Error: S_output is null, creating empty point set\n");
        S_output = alloc_point_set(0);
    }
    
    // Remove any null points from S_output
    point_set_t* cleaned_S_output = remove_null_points(S_output);
    if (cleaned_S_output != S_output) {
        printf("Warning: Removed null points from S_output\n");
        release_point_set(S_output, false);
        S_output = cleaned_S_output;
    }
    
    //also return the information about the dimensions chosen, as stored in set_final_dimensions
    highdim_output* output = new highdim_output;
    output->S = S_output;
    output->final_dimensions = set_final_dimensions;
    output->time_12 = time_12;
    output->time_3 = time_3;
    // release the memory
    release_point_set(skyline_D_prime, false);
    release_point_set(D_prime, true);
    release_point(u_final);
    return output;
}

