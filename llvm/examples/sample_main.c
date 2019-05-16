#include "lifter.h"

void
sample_main()
{
    size_t n = 5;
    float *arr = malloc(n* sizeof(float));
    int i;
    for (i = 0; i < n; i++) {
        arr[i] = 1.33;
    }
    float result = sample_func(n, arr);
    printf("%f", result);
}
