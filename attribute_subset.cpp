#include "attribute_subset.h"
#include <iostream>
using namespace std;


point_set_t* attribute_subset(point_set_t* skyline, point_set_t* S_output, int final_d, int d_prime, int d_hat_2, int rounds, std::set<int> set_final_dimensions){
    // apply the attribute subset method
    // printf("applying the attribute subset method\n"); // 111
    // printf("final_d: %d, d_prime: %d\n", final_d, d_prime); // 222
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution(0, final_d-1);
    d_hat_2 = d_hat_2; //d_hat_2 in the paper, 
    int k = d_hat_2 + 1;

    for (int i=0;i<rounds;++i){
        //randomly select d_hat dimensions
        std::set<int> selected_dimensions;
        while (selected_dimensions.size()<d_hat_2){
            //use the generator to generate a random number
            int index = distribution(generator);
            selected_dimensions.insert(*next(set_final_dimensions.begin(), index));
        }
        std::vector<int> dimension_indices(selected_dimensions.begin(), selected_dimensions.end());
        // construct a new subset S_hat based on S with d_hat_2 dimension
        int n_skyline = skyline->numberOfPoints;
        point_set_t* S_hat = alloc_point_set(n_skyline);
        for (int j = 0; j < n_skyline; ++j){
            S_hat->points[j] = alloc_point(d_hat_2);
            S_hat->points[j]->id = skyline->points[j]->id;
            for (int p = 0; p < d_hat_2; ++p){
                S_hat->points[j]->coord[p] = skyline->points[j]->coord[dimension_indices[p]];
            }
        }
        // Sphere
        point_set_t* S = sphereWSImpLP(S_hat, k);
        // take the union of S_output and S
        // record the current set of id of S_output
        std::set<int> set_S_output;
        if (S_output){
            for (int j=0; j<S_output->numberOfPoints; ++j){
                if (S_output->points[j]==nullptr){
                    printf("S_output point %d: nullptr\n", j+1);
                }
                set_S_output.insert(S_output->points[j]->id);
            }
        }
        std::set<int> set_S;
        for (int j=0; j<S->numberOfPoints; ++j){
            set_S.insert(S->points[j]->id);
        }
        std::set<int> set_union;
        std::set_union(set_S_output.begin(), set_S_output.end(), set_S.begin(), set_S.end(), std::inserter(set_union, set_union.begin()));
        if (S_output){
            release_point_set(S_output, false);
        }
        S_output = alloc_point_set(set_union.size());
        std::vector<int> union_indices(set_union.begin(), set_union.end());
        for (int j=0; j<set_union.size(); ++j){
            int match_id = union_indices[j];
            for (int k=0; k<skyline->numberOfPoints; ++k){
                if (skyline->points[k]->id == match_id){
                    S_output->points[j] = skyline->points[k];
                    break;
                }
            }
        }
        release_point_set(S, false);
        release_point_set(S_hat, true);
    }
    return S_output;
}