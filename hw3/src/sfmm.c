/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"

#include <errno.h>

/**
 * I guess remove these later
 */
#include "blocks.h"

void *sf_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    // if (lists_place_into_free_list(size) == -1) {

    //     sf_errno = ENOMEM;
    //     return NULL;
    // }

    /** Convert the size to a multiple of 16 */
    size_t block_size = blocks_minimum_size(size);

    debug("the block is size: %ld", block_size);

    return NULL;
}

void sf_free(void *pp) {
    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    return NULL;
}
