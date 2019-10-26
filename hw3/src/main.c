#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    sf_mem_init();

    // double *ptr = sf_malloc(sizeof(double));

    // *ptr = 320320320;

    // printf("%f\n", *ptr);

    // void *w = sf_malloc(8);
    // void *x = sf_malloc(200);
    // void *y = sf_malloc(300);
    // void *z = sf_malloc(4);


    // sf_free(w);
    // sf_free(x);
    // sf_free(y);
    // sf_free(z);

    // x=x;y=y;z=z;

sf_malloc(1);
void *x = sf_malloc(PAGE_SZ << 2);
x=x;

    // double *ptr2 = sf_malloc(sizeof(double));

    // *ptr2 = 911.911;

    // printf("%f\n", *ptr2);

    // double *ptr3 = sf_malloc(sizeof(long double));

    // *ptr3 = 123.3321;

    // printf("%f\n", *ptr3);

    // double *ptr4 = sf_malloc(15000);

    // *ptr4 = 4000.4040;

    // printf("%f\n", *ptr4);

    // sf_free(ptr);

    // sf_mem_fini();

    return EXIT_SUCCESS;
}
