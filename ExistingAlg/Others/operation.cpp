#include "operation.h"

#include <cmath>

int isZero(double value)
{
    return std::abs(value) < 1e-9;
}

double dot_prod(const point_t *first, const point_t *second)
{
    double result = 0.0;
    for (int i = 0; i < first->dim; ++i)
        result += first->coord[i] * second->coord[i];
    return result;
}
