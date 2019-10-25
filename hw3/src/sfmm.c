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

#define MINIMUM_BLOCK_SIZE 32
#define PROLOGUE_SIZE 32
#define EPILOGUE_SIZE 8
#define HEADER_SIZE 8
#define FOOTER_SIZE 8

/**
 * Create a sub-block of the specified size from the provided main_block
 * TODO: error check to see if the splitting creates a splinter
 */
// sf_block *create_sub_block(sf_block *main_block, int size) {
//     int old_size = main_block->header;
//     old_size = old_size >> 2;
//     old_size = old_size << 2;
//     debug("old size of block to split is: %d", old_size);
    
//     // main_block->header = main_block->header - size;

//     // sf_footer *loose_footer = main_block + ;
//     return NULL;
// }

void *initialize_first_memgrow(int block_size, void *sentinel) {

    debug("the heap before first grow (should be uninitialized heap");
    //sf_show_heap();

    void *page_start = sf_mem_grow();
    if (page_start == NULL) {
        return NULL;
    }

    debug("succesfully got start of new page at %p", page_start);

    if (sf_mem_start() == sf_mem_end()) {
        debug("mem_grow supposedly succeeded, but start and end of the page is still the same");
        return NULL;
    }

    debug("attempting to create prologue");
    sf_prologue *initial_prologue = sf_mem_start();
    debug("the current size of the prologue is %ld", sizeof(*initial_prologue));
    initial_prologue->header = 32;
    initial_prologue->header |= 0x3;
    initial_prologue->footer = 32;
    initial_prologue->footer |= 0x3;
    initial_prologue->footer ^= sf_magic();
    debug("initialized the prologue");

    debug("the contents of the prologue header are: %ld", ((sf_prologue *)(sf_mem_start() + 0))->header);
    debug("the contents of the prologue footer are: %ld", ((sf_prologue *)(sf_mem_start() + 0))->footer);

    debug("now, create the first block");
    sf_block *initial_block = sf_mem_start() + 32; // 32 instead of 40, so that the nexy blocks 'prev_footer' becomes the footer of prologue.
    debug("the placement of the first block will go at address %p", initial_block);
    debug("what's currently there? %ld", *((long int *)(sf_mem_start() + 32)));
    // NOTE: this is the debacle... whether the first block should start after the prologue, or this footer should be
    // apart of the prev one.
    // trying: completely separate.
    initial_block->prev_footer = initial_prologue->footer;
    debug("what's currently there? %ld", initial_block->prev_footer);
    // -32 b/c i think it only makes sense if the size is 4064 which leaves the 2 lsb 0's
    initial_block->header = 4096 - 32 - 8 - 8; // initial block size - size of prologue ( if it's -40, then that means that the footer is apart of the previous block)
    initial_block->header |= 0x1;
    initial_block->body.links.prev = sentinel;
    initial_block->body.links.next = sentinel;

    sf_footer *loose_footer = sf_mem_end() - 16;
    debug("the address of where the first blocks footer should be is: %p", loose_footer);
    *(loose_footer) = initial_block->header ^ sf_magic();

    sf_epilogue *initial_epilogue = sf_mem_end() - 8;
    initial_epilogue->header = 0x2;

    //debug("the heap after a successful first mem_grow");

    return sf_mem_start() + 40;
}

int which_free_list_minimum(int size) {
    if (size == MINIMUM_BLOCK_SIZE) {
        return 0;
    } else if (size > MINIMUM_BLOCK_SIZE && size <= 2 * MINIMUM_BLOCK_SIZE) {
        return 1;
    } else if (size > 2 * MINIMUM_BLOCK_SIZE && size <= 4 * MINIMUM_BLOCK_SIZE) {
        return 2;
    } else if (size > 4 * MINIMUM_BLOCK_SIZE && size <= 8 * MINIMUM_BLOCK_SIZE) {
        return 3;
    } else if (size > 8 * MINIMUM_BLOCK_SIZE && size <= 16 * MINIMUM_BLOCK_SIZE) {
        return 4;
    } else if (size > 16 * MINIMUM_BLOCK_SIZE && size <= 32 * MINIMUM_BLOCK_SIZE) {
        return 5;
    } else if (size > 32 * MINIMUM_BLOCK_SIZE && size <= 64 * MINIMUM_BLOCK_SIZE) {
        return 6;
    } else if (size > 64 * MINIMUM_BLOCK_SIZE && size <= 128 * MINIMUM_BLOCK_SIZE) {
        return 7;
    } else {
        return 8;
    }

    return 8;
}

void *find_fit(int block_size) {
    debug("finding a fit for a block of size %d", block_size);

    /** Determine which free-list (index) this size should start at */
    int min_index = which_free_list_minimum(block_size);

    debug("min_index is %d, try to place a block in that free-list", min_index);

    // int need_memgrow = 1;
    // if (sf_free_list_heads[min_index].body.links.next == sf_free_list_heads[min_index].body.links.prev) {
    //     debug("the free-list at that index is only a sentinel! should check if there higher free-lists that aren't null. Otherwise we just call mem-grow on the min-index free-list");
    //     for (int i = min_index; i < NUM_FREE_LISTS; i++) {
    //         if (sf_free_list_heads[min_index].body.links.next != sf_free_list_heads[min_index].body.links.prev) {
    //             min_index = i;
    //             need_memgrow = 0;
    //             debug("found a greater free-list that potentially has space");
    //             break;
    //         }
    //     }
    // }
    // debug("will attempt to use free list of index %d", min_index);

    // if (need_memgrow == 1) {
    //     // need to grow free-list at index 0
    //     debug("need to grow free-list at index %d", min_index);
    // } else {
    //     // should see if the free list has any space.
    //     // TODO: probably a for loop here to check for larger candidates until a new min_index is found
    //     debug("the free-list exists at index %d, check to see if it has any space in it already.", min_index);
    // }

    debug("the start of the current heap is: %p", sf_mem_start());
    debug("the end of the current heap is: %p", sf_mem_end());

    if (sf_mem_start() == sf_mem_end()) {
        debug("the current heap does not exist! mem-grow should create prologue and epilogue");

        debug("first, initializing all the sentinels");
        for (int i = 0; i < NUM_FREE_LISTS; i++) {
            sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
            sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
        }

        void *placement;
        if ((placement = initialize_first_memgrow(block_size, &sf_free_list_heads[min_index])) == NULL) {
            debug("first memgrow failed");

            return NULL;
        } else {
            debug("successfully created first block via memgrow");
            //return placement;

            sf_free_list_heads[min_index].body.links.next = placement - 8;
            sf_free_list_heads[min_index].body.links.prev = placement - 8;
            debug("binded sentinel at %d to first block", min_index);

            // ((sf_block *)placement)->body.links.next = &sf_free_list_heads[min_index];
            // debug("bindinded the block at %p to the sentinel %p", placement, &sf_free_list_heads[min_index]);
            // ((sf_block *)placement)->body.links.prev = &sf_free_list_heads[min_index];


            //sf_free_list_heads[min_index] = *placement;
            sf_show_heap();

            debug("finished");
        }
    } else {
        debug("the current heap does exist. don't need to re-create prologue.");
        // TODO: just need to find the placement of it, since heap does exist... but potentially has no space.
    }

    return NULL;
}

size_t blocks_minimum_size(size_t size, int min) {
    int amt = 16 - (size % 16);
    amt = size + amt;

    if (amt < min) {
        return min;
    }

    return amt;
}

void *sf_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    /** Convert the size to a multiple of 16 */
    size_t block_size = blocks_minimum_size(size, MINIMUM_BLOCK_SIZE);

    debug("the block is size: %ld", block_size);

    if (find_fit(block_size) == NULL) {
        debug("unable to find_fit for block_size of %ld", block_size);

        return NULL;
    }

    return NULL;
}

void sf_free(void *pp) {
    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    return NULL;
}
