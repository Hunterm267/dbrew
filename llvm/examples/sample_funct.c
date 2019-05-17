#include "lifter.h"

float
sample_func(size_t n, float* arr)
{
    float res = 0;
    for (size_t i = 0; i < n; i++)
        res += arr[i];
    return res;
}

float
nPower(int n)
{
    float x = 3.4;
    float result = x;
    int i;
    for (i = 1; i < n; i++) {
        result = result * x;
    }
    return result;
}

int
compoundAdd(int n)
{
    int x;
    for (x = n; x > 0; x--) {
        n += n;
    }
    return n;
}