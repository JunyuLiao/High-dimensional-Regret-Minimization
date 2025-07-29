#include "highdim.h"


highdim_output* interactive_highdim(point_set_t* skyline, int size, int d_bar, int d_hat, int d_hat_2, point_t* u, int K, int s, double epsilon, int maxRound, double& Qcount, double& Csize, int cmp_option, int stop_option, int prune_option, int dom_option, int& num_questions){
    int n = skyline->numberOfPoints;
    int d = skyline->points[0]->dim;
    // record the time for phase 1 and 2
    double time_12 = 0.0;
    auto start_time_12 = std::chrono::high_resolution_clock::now();
    // phase 1: narrow down the dimensions
    // printf("Phase 1\n"); // 111
	// store the dimensions if the user is interested in at least one in the set
	std::set<int> selected_dimensions;
    for (int i = 0; i < d; ++i) selected_dimensions.insert(i); // initial all dimensions

	for (int i=0;i<d/d_hat;++i){
        // first check if the user is willing to answer questions
        if (num_questions <= 0){
            break; // no more questions left
        }
		//Restrict D to dimensions i*d_hat, i*d_hat+1, ..., i*d_hat+d_hat-1
		point_set_t* D = alloc_point_set(n);
		for (int j=0;j<n;++j){
			D->points[j] = alloc_point(d_hat);
			D->points[j]->id = skyline->points[j]->id;
			for (int p=0;p<d_hat;++p){
				D->points[j]->coord[p] = skyline->points[j]->coord[i*d_hat+p];
			}
		}
		point_set_t* S = alloc_point_set(size);
        // select randomly size points from D
        // create a random number generator
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(0, n-1);
        // select size points from D
        for (int j = 0; j < size; ++j) {
            int idx = dis(gen);
            S->points[j] = D->points[idx];
        }
		// interaction
		// int id = -1; //user's choice, -1 means no choice
        // for the purpose of testing, we will select the id based on the utility vector u
        double maxValue = 0;
        int maxIdx = 0;
        // create a subset of u with d_hat dimensions
        point_t* u_hat = alloc_point(d_hat);
        for (int j = 0; j < d_hat; ++j) {
            u_hat->coord[j] = u->coord[i*d_hat+j];
        }
        for (int j = 0; j < S->numberOfPoints; j++) {
            double value = dot_prod(u_hat, S->points[j]);
            if (value > maxValue) {
                maxValue = value;
                maxIdx = j;
            }
        }
        int id = maxValue > 0 ? S->points[maxIdx]->id : -1;
        num_questions -= 1;
        release_point(u_hat);
        
		if (id==-1){
			for (int j=0;j<d_hat;++j) selected_dimensions.erase(i*d_hat+j);
		}
		release_point_set(S, false);
        release_point_set(D, true);
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
                bool answer = u->coord[*selected_dimensions.begin()] > 0;
                num_questions -= 1;
				if (!answer){
					selected_dimensions.erase(selected_dimensions.begin());
                    d_left -= 1;
				}
				else {
					final_dimensions.insert(*selected_dimensions.begin());
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
                point_set_t* D = alloc_point_set(n);
                for (int j=0;j<n;++j){
                    D->points[j] = alloc_point(group_size);
                    D->points[j]->id = skyline->points[j]->id;
                    for (int p=0;p<group_size;++p){
                        D->points[j]->coord[p] = skyline->points[j]->coord[*next(selected_dimensions.begin(), p)];
                    }
                }
                point_set_t* S = alloc_point_set(size);
                // select randomly size points from D
                // create a random number generator
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<int> dis(0, n-1);
                // select size points from D
                for (int j = 0; j < size; ++j) {
                    int idx = dis(gen);
                    S->points[j] = D->points[idx];
                }
                // insert interaction
                // int id = -1; //user's choice, -1 means no choice
                // for the purpose of testing, we will select the id based on the utility vector u
                double maxValue = 0;
                int maxIdx = 0;
                // create a subset of u with group_size dimensions
                point_t* u_hat = alloc_point(group_size);
                for (int j = 0; j < group_size; ++j) {
                    u_hat->coord[j] = u->coord[*next(selected_dimensions.begin(), j)];
                }   
                for (int j = 0; j < S->numberOfPoints; j++) {
                    double value = dot_prod(u_hat, S->points[j]);
                    if (value > maxValue) { 
                        maxValue = value;
                        maxIdx = j;
                    }
                }
                id = maxValue > 0 ? S->points[maxIdx]->id : -1;
                num_questions -= 1;
                release_point(u_hat);
                release_point_set(S, false);
                release_point_set(D, true);
            }
			if (id == -1){
				for (int j=0;j<group_size;++j){
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
                                if (u->coord[*next(selected_dimensions.begin(), right)] > 0){
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
                        point_set_t* D = alloc_point_set(n);
                        for (int j=0;j<n;++j){
                            D->points[j] = alloc_point(mid - left + 1);
                            D->points[j]->id = skyline->points[j]->id;
                            for (int p=0; p < mid-left+1; ++p){
                                D->points[j]->coord[p] = skyline->points[j]->coord[*next(selected_dimensions.begin(), left+p)];
                            }
                        }
                        point_set_t* S = alloc_point_set(size);
                        // select randomly size points from D
                        // create a random number generator
                        std::random_device rd;
                        std::mt19937 gen(rd());
                        std::uniform_int_distribution<int> dis(0, n-1);
                        // select size points from D
                        for (int j = 0; j < size; ++j) {
                            int idx = dis(gen);
                            S->points[j] = D->points[idx];
                        }
                        // insert interaction
                        // int id = -1; //user's choice, -1 means no choice
                        // for the purpose of testing, we will select the id based on the utility vector u
                        double maxValue = 0;
                        int maxIdx = 0;
                        // create a subset of u with mid - left + 1 dimensions
                        point_t* u_hat = alloc_point(mid - left + 1);
                        for (int j = 0; j < mid-left+1; ++j) {
                            u_hat->coord[j] = u->coord[*next(selected_dimensions.begin(), left+j)];
                        }
                        for (int j = 0; j < S->numberOfPoints; j++) {
                            double value = dot_prod(u_hat, S->points[j]);
                            if (value > maxValue) {
                                maxValue = value;
                                maxIdx = j; 
                            }
                        }
                        int id = maxValue > 0 ? S->points[maxIdx]->id : -1;
                        num_questions -= 1;
                        release_point_set(S, false);
                        release_point_set(D, true);
                        release_point(u_hat);
                        
                        if (id == -1){
                            // the dimension is in the right half, remove all dimensions in the left half
                            for (int j=0; j < mid-left+1; ++j){
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
	std::set<int> set_final_dimensions;
    if (selected_dimensions.size() == 0 || final_dimensions.size() == d_bar){
        set_final_dimensions = final_dimensions;
    }
    else {
        std::set_union(final_dimensions.begin(), final_dimensions.end(),
            selected_dimensions.begin(), selected_dimensions.end(),
            std::inserter(set_final_dimensions, set_final_dimensions.begin()));
    }
	// construct the final subset
	point_set_t* S_output = nullptr;

	int final_d = set_final_dimensions.size();
    // printf("number of final dimensions: %d\n", final_d);
	point_set_t* D_prime = alloc_point_set(n);
	for (int j=0;j<n;++j){
		D_prime->points[j] = alloc_point(final_d);
		D_prime->points[j]->id = skyline->points[j]->id;
		for (int p=0;p<final_d;++p){
			D_prime->points[j]->coord[p] = skyline->points[j]->coord[*next(set_final_dimensions.begin(), p)];
		}
	}
    // take the skyline of the newly constructed dataset D_prime
    point_set_t* skyline_D_prime = skyline_point(D_prime);

    // printf("number of points in skyline_D: %d\n", skyline_D_prime->numberOfPoints);
    // construct the final u
    point_t* u_final = alloc_point(final_d);
    for (int j = 0; j < final_d; ++j) {
        u_final->coord[j] = u->coord[*next(set_final_dimensions.begin(), j)];
    }

    double time_3 = 0.0;
    auto start_time_3 = std::chrono::high_resolution_clock::now();
	if (num_questions > 0){
		// apply the interactive code to select the optimal tuple
        // printf("Applying the interactive algorithm to select the optimal tuple\n");
        S_output = alloc_point_set(1);
        // if (skyline_D_prime->points[0]->dim == final_d) printf("dimension: %d\n", final_d);
        point_t* opt_p = max_utility(skyline_D_prime, u_final, s, epsilon, num_questions, Qcount, Csize, cmp_option, stop_option, prune_option, dom_option);
        S_output->points[0] = skyline->points[opt_p->id];
        num_questions -= Qcount;
    }
	else {
        if (final_d >= K){
            // exit
            printf("error: final_d >= K\n");
            exit(1);
        }
        // printf("Applying the attribute subset method to output a regret minimizing subset\n");
        if (final_d <= d_hat_2){
            point_set_t* S = sphereWSImpLP(skyline_D_prime, K);
            S_output = alloc_point_set(S->numberOfPoints);
            for (int j = 0; j < S->numberOfPoints; ++j){
                S_output->points[j] = skyline->points[S->points[j]->id];
            }
        }
        else {
            S_output = attribute_subset(skyline, S_output, final_d, d_hat_2, K, set_final_dimensions);
        }
	}

    auto end_time_3 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_3 = end_time_3 - start_time_3;
    time_3 = duration_3.count();
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

