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

            // void *x = sf_malloc(sizeof(double) * 8);

            // int *y = sf_realloc(x, sizeof(int));

            // sf_realloc(y, 3900);

            // int *zz = sf_malloc(80);
            // //sf_free(zz);
            // zz=zz;
            // sf_malloc(17);
            // sf_malloc(4048);
            // int *z = sf_malloc(4064);
            // *z = 999;
            // sf_realloc(z, 148);

            // *y = 321;
            // printf("y has %d\n", *y);
            // y=y;


    sf_errno = 0;

    sf_block ok;
    ok.header = 32 | 3;
    sf_footer nok;
    nok = ok.header ^ sf_magic();
    nok=nok;
    // even though nok is not used, it's technically in memory directly asfter the block ok
    // since it was declared right afterwards.

    sf_free(&ok);

//z=z;
// sf_malloc(1);
// void *x = sf_malloc(PAGE_SZ << 2);
// x=x;

//     sf_show_heap();

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
