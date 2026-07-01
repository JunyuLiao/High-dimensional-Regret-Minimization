#include "attribute_subset.h"
#include "experiment_random.h"
#include <algorithm>
#include <iostream>
using namespace std;

namespace {

std::vector<int> sample_dimensions(const std::set<int>& set_final_dimensions, int final_d, int d_hat_2, std::mt19937& generator, std::uniform_int_distribution<int>& distribution){
    //randomly select d_hat dimensions
    std::set<int> selected_dimensions;
    while (selected_dimensions.size()<d_hat_2){
        //use the generator to generate a random number
        int index = distribution(generator);
        selected_dimensions.insert(*next(set_final_dimensions.begin(), index));
    }
    return std::vector<int>(selected_dimensions.begin(), selected_dimensions.end());
}

point_set_t* project_points(point_set_t* skyline, const std::vector<int>& dimension_indices){
    // construct a new subset S_hat based on S with d_hat_2 dimension
    point_set_t* S_hat = alloc_point_set(skyline->numberOfPoints);
    for (int j = 0; j < skyline->numberOfPoints; ++j){
        S_hat->points[j] = alloc_point(dimension_indices.size());
        S_hat->points[j]->id = skyline->points[j]->id;
        for (int p = 0; p < dimension_indices.size(); ++p){
            S_hat->points[j]->coord[p] = skyline->points[j]->coord[dimension_indices[p]];
        }
    }
    return S_hat;
}

std::set<int> collect_output_ids(point_set_t* S_output){
    std::set<int> point_ids;
    if (S_output){
        for (int j=0; j<S_output->numberOfPoints; ++j){
            if (S_output->points[j]==nullptr){
                printf("S_output point %d: nullptr\n", j+1);
            }
            point_ids.insert(S_output->points[j]->id);
        }
    }
    return point_ids;
}

std::set<int> collect_ids(point_set_t* points){
    std::set<int> point_ids;
    for (int j=0; j<points->numberOfPoints; ++j){
        point_ids.insert(points->points[j]->id);
    }
    return point_ids;
}

point_set_t* build_output_from_ids(point_set_t* skyline, const std::set<int>& point_ids, bool final_section){
    point_set_t* S_output = alloc_point_set(point_ids.size());
    std::vector<int> ids(point_ids.begin(), point_ids.end());
    for (int j=0; j<point_ids.size(); ++j){
        int match_id = ids[j];
        bool found = false;
        for (int k=0; k<skyline->numberOfPoints; ++k){
            if (skyline->points[k]->id == match_id){
                S_output->points[j] = skyline->points[k];
                found = true;
                break;
            }
        }
        if (!found) {
            if (final_section) {
                printf("Error: Could not find point with id %d in skyline (final section)\n", match_id);
            }
            else {
                printf("Error: Could not find point with id %d in skyline\n", match_id);
            }
            S_output->points[j] = nullptr; // Explicitly set to null for debugging
        }
    }
    return S_output;
}

point_set_t* truncate_output(point_set_t* S_output, int K){
    // if the size of S_output is larger than K, keep only the first K points
    point_set_t* temp = alloc_point_set(K);
    for (int i = 0; i < K; ++i){
        temp->points[i] = S_output->points[i];
    }
    release_point_set(S_output, false);
    return temp;
}

point_set_t* fill_output_to_k(point_set_t* skyline, point_set_t* S_output, int K, std::mt19937& generator){
    std::set<int> point_ids;
    for (int i = 0; i < S_output->numberOfPoints; ++i){
        point_ids.insert(S_output->points[i]->id);
    }
    while (point_ids.size() < K){
        // randomly select a point from S_output
        std::uniform_int_distribution<int> distribution(0, skyline->numberOfPoints-1);
        int index = distribution(generator);
        while (point_ids.find(skyline->points[index]->id) != point_ids.end()){
            index = distribution(generator);
        }
        int point_id = skyline->points[index]->id;
        // add the id to point_ids
        point_ids.insert(point_id);
    }
    release_point_set(S_output, false);
    // construct the new S_output
    return build_output_from_ids(skyline, point_ids, true);
}

} // namespace

point_set_t* attribute_subset(point_set_t* skyline, point_set_t* S_output, int final_d, int d_hat_2, int K, std::set<int> set_final_dimensions){
    if (final_d < d_hat_2){
        // error: final_d < d_hat_2
        printf("error: final_d < d_hat_2\n");
        exit(1);
    }
    std::mt19937& generator = experiment_random_generator();
    std::uniform_int_distribution<int> distribution(0, final_d-1);
    int k = d_hat_2 + 1;
    int num_rounds = 0;

    while (S_output == nullptr || S_output->numberOfPoints < K){
        num_rounds++;
        if (num_rounds > 50){
            // exit if the number of rounds exceeds 30
            // printf("rounds exceeded 30, exiting...\n");
            break;
        }
        std::vector<int> dimension_indices = sample_dimensions(set_final_dimensions, final_d, d_hat_2, generator, distribution);
        point_set_t* S_hat = project_points(skyline, dimension_indices);
        // take the skyline of the newly constructed dataset S_hat
        point_set_t* skyline_S_hat = skyline_point(S_hat);
        // Sphere
        point_set_t* S = sphereWSImpLP(skyline_S_hat, k);
        // take the union of S_output and S
        // record the current set of id of S_output
        std::set<int> set_S_output = collect_output_ids(S_output);
        std::set<int> set_S = collect_ids(S);
        std::set<int> set_union;
        std::set_union(set_S_output.begin(), set_S_output.end(), set_S.begin(), set_S.end(), std::inserter(set_union, set_union.begin()));
        if (S_output){
            release_point_set(S_output, false);
        }
        S_output = build_output_from_ids(skyline, set_union, false);
        release_point_set(S, false);
        release_point_set(skyline_S_hat, false);
        release_point_set(S_hat, true);
    }
    printf("number of rounds: %d\n", num_rounds);
    if (S_output->numberOfPoints > K){
        S_output = truncate_output(S_output, K);
    }
    // print the number of rounds
    // printf("Number of rounds in attribute subset method: %d\n", num_rounds);
    if (S_output->numberOfPoints < K){
        S_output = fill_output_to_k(skyline, S_output, K, generator);
    }
    return S_output;
}
