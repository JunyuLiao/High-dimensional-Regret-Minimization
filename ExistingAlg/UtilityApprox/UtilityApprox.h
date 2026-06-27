#ifndef UTILITY_APPROX_H
#define UTILITY_APPROX_H

#include "../Others/data_struct.h"

int utilityapprox(point_set_t *points, point_t *utility, int question_size,
                  double epsilon, int max_rounds, int return_size,
                  double error_probability);

#endif
