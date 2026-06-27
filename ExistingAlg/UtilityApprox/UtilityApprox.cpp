#include "UtilityApprox.h"
#include "../Others/data_utility.h"
#include "../Others/operation.h"
#include "../exp_stats.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
void find_top_k(point_t *utility, point_set_t *points, std::vector<point_t *> &top, int count)
{
    top.assign(points->points, points->points + points->numberOfPoints);
    count = std::min(count, static_cast<int>(top.size()));
    std::partial_sort(top.begin(), top.begin() + count, top.end(),
                      [utility](const point_t *left, const point_t *right) {
                          return dot_prod(utility, left) > dot_prod(utility, right);
                      });
    top.resize(count);
}
} // namespace

/**
 * @brief Finds the point q in p[0...N-1] which has the largest utility based on u
 * @param P         point set
 * @param u         the utility vector
 * @return the id of the best point
 */
int findbestpoint(point_set_t *P, point_t *u)
{
    int best = 0, i;
    double bestValue = 0, sum;

    for (i = 0; i < P->numberOfPoints; ++i)
    {
        sum = dot_prod(P->points[i], u);
        if (sum > bestValue)
        {
            best = i;
            bestValue = sum;
        }
    }
    return best;
}

/**
 * @brief Calculated the regret ratio of utility range
 * @param L the lower bound of each dimension
 * @param U the upper bound of each dimension
 * @param D the dimension
 * @return the regret ratio
 */
double rr_bound(double *L, double *U, int D)
{
    double bound = 0;

    for (int i = 0; i < D; i++)
    {
        bound += U[i] - L[i];
    }
    return bound;
}


/**
 * @brief Danupon's Fake-Points algorithm
 * @param P the dataset
 * @param u the real utility vector
 * @param s the number of points shown in each question
 * @param epsilon the threshold of regret ratio
 * @param maxRound the upper bound of number of questions
 */
int utilityapprox(point_set_t *P, point_t *u, int s, double epsilon, int maxRound, int w, double theta)
{
    (void)theta; // Retained for compatibility with the experiment CLI.
    start_timer();
    int D = P->points[0]->dim;
    double delta, gamma, beta, sum;
    int i, j, l, qIndex, alpha, t;
    int istar = 0;
    double *U = new double[D];
    double *L = new double[D];
    double *chi = new double[s + 1];
    double round = 0;

    point_t *v = alloc_point(D);
    point_set_t *q_set = alloc_point_set(s);

    // determine the attribute with highest rating in prefs
    for (i = 1; i < D; ++i)
    {
        if (u->coord[i] > u->coord[istar])
        {
        istar = i;
        }
    }

    // increment number of rounds according to how many were needed
    // to determine highest rating above
    if ((D - 1) % (s - 1) == 0)
    {
        round = (D - 1) / (s - 1);
    }
    else
    {
        round = (D - 1) / (s - 1) + 1;
    }

    // initialize U and L
    for (i = 0; i < D; ++i)
    {
        U[i] = 1.0;
        if (i == istar)
        {
            L[i] = 1.0;
        }
        else
        {
            L[i] = 0.0;
        }
    }

    // create memory for the K points that will be shown
    for (i = 0; i < s; ++i)
    {
        q_set->points[i] = alloc_point(D);
    }

    i = 0;
    t = 1;
    while ((rr_bound(L, U, D) > epsilon && !isZero(rr_bound(L, U, D) - epsilon)) && round < maxRound)
    {
        if (i != istar)
        {
            // compute v
            for (j = 0; j < D; ++j)
                v->coord[j] = (U[j] + L[j]) / 2.0;

            // find (the index of) the point that is most preferred by v
            qIndex = findbestpoint(P, v);

            // calculate chi
            for (j = 0; j <= s; ++j)
                chi[j] = L[i] + j * (U[i] - L[i]) / s;

            // compute delta as in latest version
            sum = 0.0;
            for (j = 1; j < s; ++j)
                sum = sum + chi[j] - L[i];

            delta = 1.0 / (pow(s * 1.0, (t / (D - 1)) * 1.0) * sum);

            if (delta > 1e-5)
            {
                delta = 1e-5;
            }


            // compute gamma
            gamma = P->points[qIndex]->coord[istar] / P->points[qIndex]->coord[i];

            // create the set of fake points
            for (l = 0; l < D; ++l)
                q_set->points[s - 1]->coord[l] = P->points[qIndex]->coord[l];

            for (j = s - 2; j >= 0; j -= 1)
            {
                for (l = 0; l < D; ++l)
                    if (l == istar)
                    {
                        q_set->points[j]->coord[l] = q_set->points[j + 1]->coord[l] +
                                                     chi[j + 1] * delta * gamma * q_set->points[j + 1]->coord[i];
                    }
                    else if (l == i)
                    {
                        q_set->points[j]->coord[l] =
                                q_set->points[j + 1]->coord[l] - delta * gamma * q_set->points[j + 1]->coord[i];
                    }
                    else
                    {
                        q_set->points[j]->coord[l] = P->points[qIndex]->coord[l];
                    }
            }
            // compute beta and rescale points by it
            sum = 0.0;
            for (j = 0; j < s; ++j)
                sum += chi[j + 1] - L[i];
            beta = 1.0 / (1.0 + sum * delta);  // multiply by delta at the end to try and avoid underflow
            for (j = 0; j < s; ++j)
                for (l = 0; l < D; ++l)
                    q_set->points[j]->coord[l] = q_set->points[j]->coord[l] * beta;

            // check which point the user would pick and update L and U accordingly
            alpha = findbestpoint(q_set, u);
            L[i] = chi[alpha];
            U[i] = chi[alpha + 1];
            round = round + 1;
        }
        i = (i + 1) % D; // cycle through all the dimensions
        t += 1;
    }
    stop_timer();
    delete[] U;
    delete[] L;
    delete[] chi;

    std::vector<point_t*> top_current;
    find_top_k(v, P, top_current, w);
    release_point(v);
    release_point_set(q_set, true);

    //get the returned points
    int output_size = std::min(w, int(top_current.size()));
    double returned_score = dot_prod(u, top_current[0]);
    for (int i = 1; i < output_size; ++i)
        returned_score = std::max(returned_score, dot_prod(u, top_current[i]));
    const double run_regret = best_score > 0.0
                                  ? std::max(0.0, (best_score - returned_score) / best_score)
                                  : 0.0;
    regret_ratio += run_regret;
    question_num += round;
    return_size += output_size;
    return 0;
}
