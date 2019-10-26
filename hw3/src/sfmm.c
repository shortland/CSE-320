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

void *find_empty_block(int index, size_t block_size) {
    void *first;
    void *next;
    for (int i = index; i < NUM_FREE_LISTS; i++) {
        first = &sf_free_list_heads[i];
        next = sf_free_list_heads[i].body.links.next;

        while (first != next) {
            debug("FOUND NOT EQUAL");
            next = ((sf_block *)next)->body.links.next;
        }

        if (first != ((sf_block *)next)->body.links.prev) {
            debug("returned next of %p", ((sf_block *)next)->body.links.prev);
            return ((sf_block *)next)->body.links.prev;
        }
    }

    debug("unable to find an 'empty' block");

    return NULL;
}

void add_block_in_list(void *ptr, int index) {
    sf_block *block = (sf_block *)ptr;
    sf_block *sentinel = &sf_free_list_heads[index];

    block->body.links.next = sentinel->body.links.next;
    block->body.links.prev = sentinel;

    //debug("making sentinel prev point to: %p", block);

    (sentinel->body.links.next)->body.links.prev = block;
    sentinel->body.links.next = block;
}

void remove_block_in_list(void *ptr) {
    sf_block *block = (sf_block *)ptr;

    // set the surrounding blocks to link to our new block
    (block->body.links.prev)->body.links.next = block->body.links.next;
    (block->body.links.next)->body.links.prev = block->body.links.prev;
}

/**
 * TODO: make a function that assumes:
 * - new block being add
 * - block being removed
 * Assumes current_block being replaced in the free-list
 */
void replace_block_in_list(void *old_ptr, void *new_ptr) {
    //free_new_ptrs ptrs;

    sf_block *old_block = (sf_block *)old_ptr;
    sf_block *new_block = (sf_block *)new_ptr;

    // set the surrounding blocks to link to our new block
    (old_block->body.links.prev)->body.links.next = new_block;
    (old_block->body.links.next)->body.links.prev = new_block;

    // set the new block links to the old blocks ptrs
    new_block->body.links.next = old_block->body.links.next;
    new_block->body.links.prev = old_block->body.links.prev;
}

void *initialize_first_memgrow(void *sentinel) {
    debug("the heap before first grow (should be uninitialized heap");

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
    debug("the value of the first block footer is: %ld", *loose_footer);

    sf_epilogue *initial_epilogue = sf_mem_end() - 8;
    initial_epilogue->header = 0x2;

    //debug("the heap after a successful first mem_grow");

    return sf_mem_start() + 32;
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

sf_block *coalesce_blocks(sf_block *first, sf_block *second) {
    remove_block_in_list(first);
    remove_block_in_list(second);

    size_t total = ((first->header >> 2) << 2) + ((second->header >> 2) << 2);

    debug("the size of the both blocks combined is gonna be %ld", total);

    sf_footer *prev_block_footer_addr = (sf_footer *)(((void *)(&first->header)) - 8);
    size_t prev_block_size = ((*prev_block_footer_addr ^ sf_magic()) >> 2) << 2;
    sf_block *prev_block = (sf_block *)(((void *)prev_block_footer_addr) - prev_block_size);
    int prev_block_meta = (prev_block->header & 0x2) >> 1;

    first->header = total | prev_block_meta;
    sf_footer *loose_footer = (sf_footer *)(((void *)first) + total);
    *(loose_footer) = first->header ^ sf_magic();

    debug("add the new coalesced block to the index of %d", which_free_list_minimum(total));
    add_block_in_list(first, which_free_list_minimum(total));

    return first;
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

    void *allocated_ptr;
    if (sf_mem_start() == sf_mem_end()) {
        debug("the current heap does not exist! mem-grow should create prologue and epilogue");

        debug("first, initializing all the sentinels");
        for (int i = 0; i < NUM_FREE_LISTS; i++) {
            sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
            sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
        }

        void *placement;
        if ((placement = initialize_first_memgrow(&sf_free_list_heads[min_index])) == NULL) {
            debug("first memgrow failed");

            return NULL;
        } else {
            debug("successfully created first block via memgrow");
            //return placement;

            sf_free_list_heads[min_index].body.links.next = placement;
            sf_free_list_heads[min_index].body.links.prev = placement;
            debug("binded sentinel at %d to first block", min_index);

            // ((sf_block *)placement)->body.links.next = &sf_free_list_heads[min_index];
            // debug("bindinded the block at %p to the sentinel %p", placement, &sf_free_list_heads[min_index]);
            // ((sf_block *)placement)->body.links.prev = &sf_free_list_heads[min_index];


            //sf_free_list_heads[min_index] = *placement;

            // TODO: BREAK OUR BIG FREE BLOCK INTO ONE OF block_size.
            // probably just implement sf_realloc()
            allocated_ptr = sf_realloc(placement, block_size);
            debug("finished its allocated_ptr is %p", allocated_ptr);
            //debug("the size of it is: %ld", sizeof(allocated_ptr));
            return allocated_ptr;
        }
    } else {
        debug("the current heap does exist. don't need to re-create prologue.");
        // TODO: just need to find the placement of it, since heap does exist... but potentially has no space.
        //int need_memgrow = 1;
        //void *allocated_ptr;
        //size_t biggest_size;

        void *empty_block = find_empty_block(min_index, block_size);
        if (empty_block == NULL) {
            debug("there are no empty blocks to use. do mem_grow on our original min_index: %d", min_index);

            if ((empty_block = sf_mem_grow()) == NULL) {
                debug("unable to call memgrow to get another block");

                return NULL;
            } else {
                debug("memgrow was successful, now coalesce it with the immediately previous empty block if there is one");

                sf_epilogue *epilogue = (sf_epilogue *)(sf_mem_end() - 8 - 4096);
                if (epilogue->header == 3) {
                    debug("there is no empty block immediately preceding the old epilogue, make the old epilogue the new header of our new block");
                    // sf_epilogue *new_epilogue = (sf_epilogue *)(sf_mem_end() - 8);
                    // new_epilogue->header = 0x2;

                    // sf_block *lonely_block = &epilogue;
                    // lonely_block->header = 

                    // biggest_size = 4096;

                } else if (epilogue->header == 2) {
                allocate_another_page:
                    debug("there is an empty block immediately preceding the epilogue");

                    sf_footer *empty_footer = (sf_footer *)(sf_mem_end() - 16 - 4096);

                    debug("the empty block size is: %ld", ((*empty_footer ^ sf_magic())) & 0xFFFFFFFC);

                    size_t old_empty_size = ((*empty_footer ^ sf_magic())) & 0xFFFFFFFC;
                    sf_block *old_empty = (sf_block *)(sf_mem_end() - 16 - 4096 - old_empty_size);

                    if ((old_empty->header & 0xFFFFFFFC) != ((*empty_footer ^ sf_magic()) & 0xFFFFFFFC)) {
                        debug("the header of the empty block didnt match its footer... oops");

                        return NULL;
                    }

                    debug("the size from header is: %ld, which should match footer", (old_empty->header & 0xFFFFFFFC));
                    // in essence, becomes new header, just overwrite its size contents
                    old_empty->header = ((old_empty->header & 0xFFFFFFFC) + 4096) | (old_empty->header & 0x1);

                    sf_footer *new_footer = sf_mem_end() - 16;
                    *(new_footer) = old_empty->header ^ sf_magic();
                    sf_epilogue *new_epilogue = sf_mem_end() - 8;
                    new_epilogue->header = 0x2;

                    size_t big_size = old_empty->header & 0xFFFFFFFC;

                    /**
                     * Remove the block from the current free-list
                     * Calculate new index via big_size
                     * insert into new free-list
                     */
                    remove_block_in_list(old_empty);
                    int new_index = which_free_list_minimum(big_size);
                    add_block_in_list(old_empty, new_index);

                    if (big_size >= block_size) {
                        allocated_ptr = sf_realloc(old_empty, block_size);

                        return allocated_ptr;
                    } else {
                        // i'm on a tight deadline. at least i know this thing exists
                        if (sf_mem_grow() == NULL) {
                            debug("Unable to allocate another page");

                            return NULL;
                        }
                        goto allocate_another_page;
                    }
                } else {
                    debug("the epilogue appears to be malformed.");

                    return NULL;
                }
            }
        } else {
            debug("will use the empty block at %p", (sf_block *)empty_block);

            allocated_ptr = sf_realloc(empty_block, block_size);
            debug("finished its allocated_ptr is %p", allocated_ptr);

            return allocated_ptr;
        }
    }

    return NULL;
}

size_t blocks_minimum_size(size_t size, int min) {
    size = size + 16;
    int amt = 16 - (size % 16);
    amt = size + amt;

    //amt = amt + 16; // header and footer
    return amt;
}

void *sf_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    /** Convert the size to a multiple of 16 */
    size_t block_size = blocks_minimum_size(size, MINIMUM_BLOCK_SIZE);

    debug("the block is size: %ld", block_size);

    void *ptr;
    if ((ptr = find_fit(block_size)) == NULL) {
        debug("unable to find_fit for block_size of %ld", block_size);

        return NULL;
    }

    sf_show_heap();

    return ptr;
}

void sf_free(void *pp) {
    if (pp == NULL) {
        debug("tried to pass in null pointer");

        abort();
    }

    pp = pp - 8 - 8;

    sf_block *block = (sf_block *)pp;
    if ((block->header & 0x2) == 0) {
        debug("tried to free a non-allocated block at %p", block);

        abort();
    }

    if ((sf_mem_start() + 40) > (void *)(&(block->header))) {
        debug("tried to free block not starting in heap");
        debug("start: %p", (sf_mem_start() + 40));
        debug("pointing: %p", (void *)(&(block->header)));

        abort();
    }

    void *footer_addr = &block->header + ((block->header >> 2) << 2);
    // debug("the footer addr is %p", footer_addr);
    if (footer_addr > (sf_mem_end() - 8)) {
        debug("tried to free block not ending in heap");

        abort();
    }

    if (((block->header >> 2) << 2) < 32) {
        debug("tried to free a block that has a size smaller than permitted minimum");

        abort();
    }

    if ((block->header & 0x1) == 0) {
        debug("the previous block, according the prev-header bit, is NOT allocated");

        if (*((sf_header *)((void *)(&block->header) - 0 - (((*( (sf_footer *)((void *)(&block->header) - 8)) ^ sf_magic()) >> 2) << 2) )) & 0x2) {
            debug("the previous header block says it IS allocated");

            abort();
        }
    }

    if ( (*((sf_footer *)(( ((void *)(&block->header)) + ((block->header >> 2) << 2) ) - 8)) ^ sf_magic()) != block->header ) {
        debug("footer and header do not match");

        abort();
    }

    // debug("the footer address is: %p", (sf_footer *)(( ((void *)(&block->header)) + ((block->header >> 2) << 2) ) - 8) );
    // debug("the footer value is %ld", *((sf_footer *)(( ((void *)(&block->header)) + ((block->header >> 2) << 2) ) - 8)) ^ sf_magic() );
    // debug("the footer value is: %ld", *((sf_footer *)((void *)((&block->header) - 0 + ((block->header >> 2) << 2)))));

    //debug("header says current is %ld", block->header);
    // debug("address of header is %p", (void *)(&block->header));
    // debug("address of the footer before is: %p", (void *)(&block->header) - 8);
    // debug("size of the footer/block before: %ld", ((*( (sf_footer *)((void *)(&block->header) - 8)) ^ sf_magic()) >> 2) << 2 );

    // debug("address header of block before: %p", (void *)(&block->header) - 8 - (((*( (sf_footer *)((void *)(&block->header) - 8)) ^ sf_magic()) >> 2) << 2) );
    // debug("value header of block before: %ld", *((sf_header *)((void *)(&block->header) - 0 - (((*( (sf_footer *)((void *)(&block->header) - 8)) ^ sf_magic()) >> 2) << 2) )));

    // debug("prev block footer says size is: %ld", (*((sf_footer *)(&block->header)) >> 2 ) << 2 );

    // debug("the next block header value is: %ld", (*((sf_header *)(( ((void *)(&block->header)) + ((block->header >> 2) << 2) ) - 0)) ) );

    int prev_allocated = block->header & 0x1;
    block->header = ((block->header >> 2) << 2) | prev_allocated;
    sf_footer *block_footer = (sf_footer *)(((void *)block) + ((block->header >> 2) << 2));
    //debug("sf_footer has a value of %ld", *block_footer ^ sf_magic());
    *(block_footer) = block->header ^ sf_magic();

    sf_footer *prev_block_footer_addr = (sf_footer *)(((void *)(&block->header)) - 8);
    size_t prev_block_size = ((*prev_block_footer_addr ^ sf_magic()) >> 2) << 2;
    sf_block *prev_block = (sf_block *)(((void *)prev_block_footer_addr) - prev_block_size);

    size_t current_block_size = (block->header >> 2) << 2;
    sf_block *next_block = ((void *)block) + current_block_size;
    //next_block->header);
    //debug("the next block size is: %ld", next_block->header);
    next_block->header = ((next_block->header >> 1) << 1);
    sf_footer *next_footer = (sf_footer *)(((void *)next_block) + ((next_block->header >> 2) << 2));
    *(next_footer) = next_block->header ^ sf_magic();

    // debug("prev block (footer) size is %ld", prev_block_size);
    // debug("prev block (header) size is %ld", prev_block->header);

    add_block_in_list(block, which_free_list_minimum(current_block_size));

    if ((block->header & 0x1) == 0) {
        debug("previous block is not allocated, should coalesce with it");
        block = coalesce_blocks(prev_block, block);
    }

    if ((*((sf_header *)(( ((void *)(&block->header)) + ((block->header >> 2) << 2) ) - 0)) & 0x2) == 0) {
        debug("next block is not allocated, should coalesce with it");
        block = coalesce_blocks(block, next_block);
    }

    sf_show_heap();

    return;
}

// NOTE: DO NOT COALESCE HERE.
void *sf_realloc(void *pp, size_t rsize) {
    if (rsize % 16 != 0) {
        rsize = rsize + (16 - (rsize % 16));
    }

    debug("new calculated realloc size is: %ld", rsize);

    // I get all these, so that I know whether or not we can coalesce
    // might be useless in this function, but can use it in the free() function.
    // coalescing is supposed to happen only during free()

    int current_header = ((sf_block *)(pp))->header;

    int previous_allocated = current_header;
    previous_allocated = previous_allocated & 0x1;
    debug("the previous block is %d (0 false (not), 1 true) allocated", previous_allocated);

    int current_allocated = current_header;
    current_allocated = (current_allocated & 0x2) >> 1;
    debug("the current block is %d (0 false (not), 1 true) allocated", current_allocated);

    sf_header *next_header = (sf_header *)(pp + (current_header & 0xFFFFFFFC) + 8);
    int next_allocated = (*next_header & 0x2) >> 1;
    debug("the next block is %d (0 false (not), 1 true) allocated", next_allocated);

    // typically, a block would always be allocated if its being re-sized/reallocated
    // NOTE: this might cause issues in the future if im not supposed to realloc a "non-allocated" block

    int current_size = ((sf_block *)(pp))->header;
    current_size = current_size >> 2;
    current_size = current_size << 2;
    debug("the current_size is %d, will change it to a size of %ld", current_size, rsize);

    if (rsize < current_size) {
        debug("will shrink current block.");

        if (current_size - rsize < 32) {
            debug("splitting will cause a splinter, so just resize header - which makes padding aka internal fragmentation");
            // but can't really resize header... since block is already said size... so do nothing :]
            return (void *)((sf_block *)pp)->body.payload;
        } else {
            debug("the remaining block size is greater than the minimum block size of 32. so size down into 2.");

            // change the previous blocks header
            sf_block *old_block = (sf_block *)pp;
            if (previous_allocated == 1) {
                old_block->header = rsize | 0x3;
            } else {
                old_block->header = rsize | 0x2;
            }

            sf_footer prev_footer = old_block->header ^ sf_magic();

            size_t new_next_header = (current_size - rsize) | 0x1;

            sf_block *new_split = pp + rsize;

            /**
             * Remove pp from list.
             * Get new index for new_split size
             * Add new_split at new_split index
             */
            remove_block_in_list(pp);
            debug("removed %p from free-list", pp);
            int new_index = which_free_list_minimum(current_size - rsize);
            add_block_in_list(new_split, new_index);


            new_split->header = new_next_header;
            //new_split->body.links.next = free_ptrs.next;
            //new_split->body.links.prev = free_ptrs.prev;
            new_split->prev_footer = prev_footer;
            sf_footer *new_loose_footer = pp + rsize + (current_size - rsize);
            *(new_loose_footer) = new_split->header ^ sf_magic();

            return (void *)old_block->body.payload;
        }
    } else if (rsize > current_size) {
        debug("need to expand current block, maybe memgrow or coallesce or even going up a tier");
    } else {
        debug("the given block is already the specified size");

        return pp;
    }

    return NULL;
}
