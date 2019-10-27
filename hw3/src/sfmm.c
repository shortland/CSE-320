/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "sfmm.h"

#define MINIMUM_BLOCK_SIZE 32
#define PROLOGUE_SIZE 32
#define EPILOGUE_SIZE 8
#define HEADER_SIZE 8
#define FOOTER_SIZE 8

/**
 * Validates the block integrity by checking whether the header
 * and the footer are equivalent (after sf_magic()) to one another.
 */
void validate_block_integrity(sf_block *block)
{
    size_t size = (block->header >> 2) << 2;
    debug("the size of the block is %lu", size);

    if (size == 0)
    {
        debug("stopping, since its expected to just be an epilogue");
        return;
    }

    size_t allocated = block->header & 0x2;
    size_t prev_allocated = block->header & 0x1;
    if (allocated == 2)
    {
        debug("header: the block is allocated");
    }

    if (prev_allocated == 1)
    {
        debug("header: the prev block is allocated");
    }

    sf_footer *footer = ((void *)block) + size;
    size_t footer_data = *footer ^ sf_magic();
    size_t footer_size = (footer_data >> 2) << 2;
    footer_size = footer_size;
    debug("the size of the footer is: %lu", footer_size);

    allocated = footer_data & 0x2;
    prev_allocated = footer_data & 0x1;
    if (allocated == 2)
    {
        debug("footer: the block is allocated");
    }

    if (prev_allocated == 1)
    {
        debug("footer: the prev block is allocated");
    }

    if (block->header != footer_data)
    {
        error("mismatch of footer and header");
        abort();
    }
}

/**
 * Find & return an empty block of at least block_size.
 * Begins search at the free-list index, index.
 */
void *find_empty_block(size_t index, size_t block_size)
{
    void *first;
    void *next;

    for (int i = index; i < NUM_FREE_LISTS; i++)
    {
        first = &sf_free_list_heads[i];
        next = sf_free_list_heads[i].body.links.next;

        while (first != next)
        {
            debug("found non matching block; aka non-sentinel.");
            next = ((sf_block *)next)->body.links.next;
        }

        if (first != ((sf_block *)next)->body.links.prev)
        {
            debug("returned next of %p", ((sf_block *)next)->body.links.prev);
            return ((sf_block *)next)->body.links.prev;
        }
    }

    debug("unable to find an 'empty' block in free-list");
    return NULL;
}

/**
 * Add a specified block ptr to the specified free-list index
 */
void add_block_in_list(void *ptr, int index)
{
    sf_block *block = (sf_block *)ptr;
    sf_block *sentinel = &sf_free_list_heads[index];

    block->body.links.next = sentinel->body.links.next;
    block->body.links.prev = sentinel;

    (sentinel->body.links.next)->body.links.prev = block;
    sentinel->body.links.next = block;
}

/**
 * Checks whether a block exists in any free-list
 */
int exists_block_in_list(sf_block *ptr)
{
    sf_block *first;
    sf_block *next;

    for (int i = 0; i < NUM_FREE_LISTS; i++)
    {
        first = &sf_free_list_heads[i];
        next = sf_free_list_heads[i].body.links.next;

        while (first != next)
        {
            if (next == ptr)
            {
                debug("found the block in a list");
                return 1;
            }

            next = next->body.links.next;
        }
    }

    return 0;
}

/**
 * Remove the specified block from its free-list.
 */
void remove_block_in_list(void *ptr)
{
    sf_block *block = (sf_block *)ptr;

    if (exists_block_in_list(block) == 0)
    {
        debug("block was not found in any list");
        return;
    }

    (block->body.links.prev)->body.links.next = block->body.links.next;
    (block->body.links.next)->body.links.prev = block->body.links.prev;
}

/**
 * This function is called once, when the initial sf_mem_grow() is called.
 * It creates the initial prologue, epilogue and first block.
 */
void *initialize_first_memgrow(void *sentinel)
{
    void *page_start = sf_mem_grow();
    if (page_start == NULL)
    {
        error("first initial call the sf_mem_grow() failed");
        return NULL;
    }

    debug("succesfully got start of new page at %p", page_start);
    if (sf_mem_start() == sf_mem_end())
    {
        error("mem_grow supposedly succeeded, but start and end of the page is still the same");
        return NULL;
    }

    debug("attempting to create prologue");
    sf_prologue *initial_prologue = sf_mem_start();
    debug("the current size of the prologue is %lu", sizeof(*initial_prologue));
    initial_prologue->header = 32;
    initial_prologue->header |= 0x3;
    initial_prologue->footer = 32;
    initial_prologue->footer |= 0x3;
    initial_prologue->footer ^= sf_magic();
    debug("initialized the prologue");

    debug("now, creating the first block");
    sf_block *initial_block = sf_mem_start() + 32; // 32 instead of 40, so that the next blocks 'prev_footer' becomes the footer of prologue.
    debug("the placement of the first block will go at address %p", initial_block);
    initial_block->prev_footer = initial_prologue->footer;
    initial_block->header = PAGE_SZ - 32 - 8 - 8; // initial block size - size of prologue ( if it's -40, then that means that the footer is apart of the previous block)
    initial_block->header |= 0x1;

    debug("setting the first block next and prev pointers to the specified sentinel...");
    initial_block->body.links.prev = sentinel;
    initial_block->body.links.next = sentinel;

    debug("creating the footer of the block");
    sf_footer *loose_footer = sf_mem_end() - 16;
    *(loose_footer) = initial_block->header ^ sf_magic();

    debug("creating the epilogue");
    sf_epilogue *initial_epilogue = sf_mem_end() - 8;
    initial_epilogue->header = 0x2;

    return sf_mem_start() + 32;
}

/**
 * Determine which free-list a block of the specified size belongs to.
 * Yeah, this definitely can be done in a few lines...
 */
int which_free_list_minimum(size_t size)
{
    if (size == MINIMUM_BLOCK_SIZE)
    {
        return 0;
    }
    else
    {
        if (size > MINIMUM_BLOCK_SIZE && size <= 2 * MINIMUM_BLOCK_SIZE)
        {
            return 1;
        }
        else if (size > 2 * MINIMUM_BLOCK_SIZE && size <= 4 * MINIMUM_BLOCK_SIZE)
        {
            return 2;
        }
        else if (size > 4 * MINIMUM_BLOCK_SIZE && size <= 8 * MINIMUM_BLOCK_SIZE)
        {
            return 3;
        }
        else if (size > 8 * MINIMUM_BLOCK_SIZE && size <= 16 * MINIMUM_BLOCK_SIZE)
        {
            return 4;
        }
        else if (size > 16 * MINIMUM_BLOCK_SIZE && size <= 32 * MINIMUM_BLOCK_SIZE)
        {
            return 5;
        }
        else if (size > 32 * MINIMUM_BLOCK_SIZE && size <= 64 * MINIMUM_BLOCK_SIZE)
        {
            return 6;
        }
        else if (size > 64 * MINIMUM_BLOCK_SIZE && size <= 128 * MINIMUM_BLOCK_SIZE)
        {
            return 7;
        }
        else
        {
            return 8;
        }
    }

    return 8;
}

/**
 * Coalesce the first and second block together.
 * Very simple and straightforward.
 * Assumes the are both free.
 */
sf_block *coalesce_blocks(sf_block *first, sf_block *second)
{
    // remove the blocks from any free-lists
    remove_block_in_list(first);
    remove_block_in_list(second);

    // validate the block footers/headers
    validate_block_integrity(first);
    validate_block_integrity(second);

    size_t total = ((first->header >> 2) << 2) + ((second->header >> 2) << 2);
    debug("the size of the both blocks combined is gonna be %lu", total);

    // rewrite the previous block tag data
    sf_footer *prev_block_footer_addr = (sf_footer *)(((void *)(&first->header)) - 8);
    size_t prev_block_size = ((*prev_block_footer_addr ^ sf_magic()) >> 2) << 2;
    sf_block *prev_block = (sf_block *)(((void *)prev_block_footer_addr) - prev_block_size);
    size_t prev_block_meta = (prev_block->header & 0x2) >> 1;

    // create the "new" block at the spot of the "first" block
    first->header = total | prev_block_meta;
    sf_footer *loose_footer = (sf_footer *)(((void *)first) + total);
    *(loose_footer) = first->header ^ sf_magic();

    // add the coalesced block to the free-list
    debug("add the new coalesced block to the index of %u", which_free_list_minimum(total));
    add_block_in_list(first, which_free_list_minimum(total));

    return first;
}

/**
 * Takes a pointer to a block, and splits it into rsize.
 * A more specified type of "realloc(),..." where it only reallocates to a smaller size.
 */
void *split_down(void *pp, size_t rsize)
{
    debug("new calculated split down size is: %lu", rsize);

    size_t current_header = ((sf_block *)(pp))->header;

    size_t previous_allocated = current_header;
    previous_allocated = previous_allocated & 0x1;
    debug("the previous block is %lu (0 false (not), 1 true) allocated", previous_allocated);

    size_t current_allocated = current_header;
    current_allocated = (current_allocated & 0x2) >> 1;
    debug("the current block is %lu (0 false (not), 1 true) allocated", current_allocated);

    size_t current_size = ((sf_block *)(pp))->header;
    current_size = current_size >> 2;
    current_size = current_size << 2;
    debug("the current_size is %lu, will change it to a size of %lu", current_size, rsize);

    // determine if we really are supposed to shrink the block...
    if (rsize < current_size)
    {
        debug("will shrink current block.");

        if (current_size - rsize < 32)
        {
            debug("splitting will cause a splinter, so just resize header - which makes padding aka internal fragmentation");
            return (void *)((sf_block *)pp)->body.payload;
        }

        debug("the remaining block size is greater than the minimum block size of 32. so size down into 2.");

        // change the previous blocks header
        sf_block *old_block = (sf_block *)pp;
        if (previous_allocated == 1)
        {
            old_block->header = rsize | 0x3;
        }
        else
        {
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
        add_block_in_list(new_split, which_free_list_minimum(current_size - rsize));
        debug("added the free block to a free-list");

        // new split should be all free, but im forcing it to be allocated - s.t. we can directly call free on it
        new_split->header = new_next_header | 0x2;
        new_split->prev_footer = prev_footer;
        sf_footer *new_loose_footer = (sf_footer *)(pp + rsize + (current_size - rsize));
        *(new_loose_footer) = new_split->header ^ sf_magic();
        remove_block_in_list(new_split);

        // need to set the prev-alloc bit of the next block header and footer
        //sf_block *next_block = (sf_block *)((void *)new_split + ((new_split->header >> 2) << 2));
        size_t next_next_header = *((sf_header *)(((void *)new_loose_footer) + 8));
        validate_block_integrity(new_split);
        validate_block_integrity(old_block);
        debug("the value of the next header is: %lu", next_next_header);

        if (((next_next_header >> 2) << 2) == 0)
        {
            debug("next block is an epilogue, so just set the that");
            next_next_header = next_next_header | 0x2;
        }
        else
        {
            sf_block *next_block = (sf_block *)((void *)new_split + ((new_split->header >> 2) << 2));
            next_block->header = next_block->header | 0x1;
            debug("next block is a real block that i need the change the prev-alloc bits for");
            sf_footer *loose_footer = ((void *)next_block) + (((next_block->header >> 2) << 2));
            *loose_footer = next_block->header ^ sf_magic();
        }

        debug("free the split off portion");
        sf_free(new_split->body.payload);

        return (void *)old_block->body.payload;
    }
    else if (rsize > current_size)
    {
        error("the provided block isn't big enough, can't split_down(). should be split_up()");
        return NULL;
    }
    else
    {
        debug("the given block is already the specified size, now set it to allocated");
        remove_block_in_list(pp);
        debug("removed the current free block from the list");

        // set the header of current block to allocated
        sf_block *current_block = (sf_block *)pp;
        current_block->header = current_block->header | 0x2;

        // set the footer of the current block to allocated
        sf_footer *current_footer = (sf_footer *)((void *)current_block + ((current_block->header >> 2) << 2));
        *(current_footer) = current_block->header ^ sf_magic();
        debug("the contents of the current footer is: %lu", *current_footer ^ sf_magic());

        // set the header of the next block to prev-allocated
        sf_block *next_block = (sf_block *)((void *)current_block + ((current_block->header >> 2) << 2));
        next_block->header = next_block->header | 0x1;
        size_t next_block_size = (next_block->header >> 2) << 2;
        sf_footer *next_foot = (sf_footer *)((void *)current_block + ((current_block->header >> 2) << 2) + next_block_size);
        *(next_foot) = next_block->header ^ sf_magic();

        return (void *)current_block->body.payload;
    }

    return NULL;
}

/**
 * Returns an allocated block of the specified block_size
 */
void *find_fit(size_t block_size)
{
    debug("finding a fit for a block of size %lu", block_size);

    /** Determine which free-list (index) this size should start at */
    int min_index = which_free_list_minimum(block_size);

    debug("min_index is %d, try to place a block in that free-list", min_index);
    debug("the start of the current heap is: %p", sf_mem_start());
    debug("the end of the current heap is: %p", sf_mem_end());

    void *empty_block;
    void *allocated_ptr;
    if (sf_mem_start() == sf_mem_end())
    {
        debug("the current heap does not exist! mem-grow should create prologue and epilogue");

        debug("first, initializing all the sentinels");
        for (int i = 0; i < NUM_FREE_LISTS; i++)
        {
            sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
            sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
        }

        void *placement;
        if ((placement = initialize_first_memgrow(&sf_free_list_heads[min_index])) == NULL)
        {
            debug("first memgrow failed");
            return NULL;
        }
        else
        {
            debug("successfully created first block via memgrow");

            sf_free_list_heads[min_index].body.links.next = placement;
            sf_free_list_heads[min_index].body.links.prev = placement;
            debug("binded sentinel at %d to first block", min_index);

            if (block_size <= 4048)
            {
                debug("need a split of the main block.");
                allocated_ptr = split_down(placement, block_size);
            }
            else
            {
                debug("need more than just the main block.");
                empty_block = find_empty_block(min_index, block_size);
                goto heap_exists_just_more_block;
            }

            debug("finished its allocated_ptr is %p", allocated_ptr);
            return allocated_ptr;
        }
    }
    else
    {
        debug("the current heap does exist. don't need to re-create prologue.");

        empty_block = find_empty_block(min_index, block_size);
        if (empty_block == NULL)
        {
            debug("there are no empty blocks to use. do mem_grow on our original min_index: %d", min_index);

            if ((empty_block = sf_mem_grow()) == NULL)
            {
                debug("unable to call memgrow to get another block");
                return NULL;
            }
            else
            {
                debug("memgrow was successful, now coalesce it with the immediately previous empty block if there is one");

                sf_epilogue *epilogue = (sf_epilogue *)(sf_mem_end() - 8 - PAGE_SZ);
                if (epilogue->header == 3)
                {
                    debug("there is no empty block immediately preceding the old epilogue, make the old epilogue the new header of our new block");

                    sf_block *new_lone_block = (sf_block *)((void *)epilogue - 8);
                    new_lone_block->header = PAGE_SZ | 0x1;
                    sf_footer *new_lone_footer = (sf_footer *)((void *)new_lone_block + PAGE_SZ);
                    *new_lone_footer = new_lone_block->header ^ sf_magic();

                    // add the block to a free-list
                    add_block_in_list(new_lone_block, which_free_list_minimum(PAGE_SZ));

                    sf_epilogue *new_epilogue = sf_mem_end() - 8;
                    new_epilogue->header = 0x2;

                    if (block_size < PAGE_SZ)
                    {
                        allocated_ptr = split_down(new_lone_block, block_size);
                        return allocated_ptr;
                    }
                    else if (block_size > PAGE_SZ)
                    {
                        goto allocate_another_page;
                    }
                    else
                    {
                        return new_lone_block->body.payload;
                    }
                }
                else if (epilogue->header == 2)
                {
                allocate_another_page:
                    debug("there is an empty block immediately preceding the epilogue");

                    sf_footer *empty_footer = (sf_footer *)(sf_mem_end() - 16 - PAGE_SZ);
                    debug("the empty block size is: %lu", ((*empty_footer ^ sf_magic())) & 0xFFFFFFFC);
                    size_t old_empty_size = ((*empty_footer ^ sf_magic())) & 0xFFFFFFFC;
                    sf_block *old_empty = (sf_block *)(sf_mem_end() - 16 - PAGE_SZ - old_empty_size);

                    if ((old_empty->header & 0xFFFFFFFC) != ((*empty_footer ^ sf_magic()) & 0xFFFFFFFC))
                    {
                        error("footer and header size mismatch");
                        return NULL;
                    }

                    // in essence, becomes new header, just overwrite its size contents
                    old_empty->header = ((old_empty->header & 0xFFFFFFFC) + PAGE_SZ) | (old_empty->header & 0x1);

                    // set the contents
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

                    if (big_size >= block_size)
                    {
                        allocated_ptr = split_down(old_empty, block_size);
                        return allocated_ptr;
                    }
                    else
                    {
                    /**
                     * The heap already exists, so we already have a prologue and epilogue,
                     * but instead of our first new block being split, we need to add more to it.
                     */
                    heap_exists_just_more_block:
                        debug("our block still needs more space than what we have. will try memgrow");
                        if (sf_mem_grow() == NULL)
                        {
                            debug("Unable to allocate another page");

                            allocated_ptr = split_down(old_empty, block_size);
                            debug("this returns a ptr to a block that isnt of sufficient size...");

                            return allocated_ptr;
                        }

                        goto allocate_another_page;
                    }
                }
                else
                {
                    debug("the epilogue appears to be malformed.");
                    return NULL;
                }
            }
        }
        else
        {
            debug("will use the empty block at %p", (sf_block *)empty_block);

            size_t empty_block_size = (((sf_block *)empty_block)->header >> 2) << 2;
            if (empty_block_size >= block_size)
            {
                allocated_ptr = split_down(empty_block, block_size);
            }
            else
            {
                goto heap_exists_just_more_block;
            }

            debug("finished its allocated_ptr is %p", allocated_ptr);
            return allocated_ptr;
        }
    }

    return NULL;
}

/**
 * Determine a minimum size for a alloc call with given size
 */
size_t blocks_minimum_size(size_t size, size_t min)
{
    size = size + 16;
    size_t amt = 16 - (size % 16);
    if (amt == 16)
    {
        amt = 0;
    }
    amt = size + amt;

    return amt;
}

/**
 * The malloc function
 */
void *sf_malloc(size_t size)
{
    sf_errno = 0;

    if (size == 0)
    {
        return NULL;
    }

    size_t block_size = blocks_minimum_size(size, MINIMUM_BLOCK_SIZE);
    debug("the block is size: %lu", block_size);

    if (size > block_size) {
        error("uint overflow");
        return NULL;
    }

    void *ptr;
    if ((ptr = find_fit(block_size)) == NULL)
    {
        error("unable to find_fit for block_size of %lu", block_size);
        sf_errno = ENOMEM;
        return NULL;
    }

    return ptr;
}

/**
 * The free function
 */
void sf_free(void *pp)
{
    sf_errno = 0;

    if (pp == NULL)
    {
        debug("tried to pass in null pointer");
        abort();
    }

    pp = pp - 8 - 8;
    sf_block *block = (sf_block *)pp;

    if ((block->header & 0x2) == 0)
    {
        debug("tried to free a non-allocated block at %p", block);
        abort();
    }

    size_t size_of_block = (block->header >> 2) << 2;
    size_of_block = size_of_block;
    debug("the size of the block im trying to free is: %lu", size_of_block);

    if ((sf_mem_start() + 40) > (void *)(&(block->header)))
    {
        debug("tried to free block not starting in heap");
        debug("start: %p", (sf_mem_start() + 40));
        debug("pointing: %p", (void *)(&(block->header)));
        abort();
    }

    void *footer_addr = ((void *)&block->header) + ((block->header >> 2) << 2);
    if (footer_addr > (sf_mem_end() - 8))
    {
        debug("tried to free block not ending in heap %p is footer;; %p is end of heap", footer_addr, sf_mem_end() - 8);
        abort();
    }

    if (((block->header >> 2) << 2) < 32)
    {
        debug("tried to free a block that has a size smaller than permitted minimum");
        abort();
    }

    if ((block->header & 0x1) == 0)
    {
        debug("the previous block, according the prev-header bit, is NOT allocated");

        if (*((sf_header *)((void *)(&block->header) - 0 - (((*((sf_footer *)((void *)(&block->header) - 8)) ^ sf_magic()) >> 2) << 2))) & 0x2)
        {
            debug("the previous header block says it IS allocated");
            abort();
        }
    }

    if ((*((sf_footer *)((((void *)(&block->header)) + ((block->header >> 2) << 2)) - 8)) ^ sf_magic()) != block->header)
    {
        debug("footer and header do not match");
        abort();
    }

    validate_block_integrity(block);

    size_t prev_allocated = block->header & 0x1;
    block->header = ((block->header >> 2) << 2) | prev_allocated;
    sf_footer *block_footer = (sf_footer *)(((void *)block) + ((block->header >> 2) << 2));
    *(block_footer) = block->header ^ sf_magic();

    sf_footer *prev_block_footer_addr = (sf_footer *)(((void *)(&block->header)) - 8);
    size_t prev_block_size = ((*prev_block_footer_addr ^ sf_magic()) >> 2) << 2;
    sf_block *prev_block = (sf_block *)(((void *)prev_block_footer_addr) - prev_block_size);

    // get current block size to get the next block header
    size_t current_block_size = (block->header >> 2) << 2;
    sf_block *next_block = ((void *)block) + current_block_size;

    validate_block_integrity(next_block);

    // get the value of the header
    size_t header_value = (next_block->header >> 2) << 2;
    if (header_value == 0)
    {
        debug("next block is the epilogue, set its prv-alloc value to 0");
        next_block->header = 2;
    }
    else
    {
        debug("next block is normal block, set its prv-alloc head and foot to 0");
        next_block->header = (next_block->header >> 1) << 1;
        sf_footer *next_footer = (sf_footer *)((void *)next_block + ((next_block->header >> 2) << 2));
        *(next_footer) = next_block->header ^ sf_magic();
    }

    add_block_in_list(block, which_free_list_minimum(current_block_size));

    if ((block->header & 0x1) == 0)
    {
        debug("previous block is not allocated, should coalesce with it");
        block = coalesce_blocks(prev_block, block);
    }

    if ((*((sf_header *)((((void *)(&block->header)) + ((block->header >> 2) << 2)) - 0)) & 0x2) == 0)
    {
        debug("next block is not allocated, should coalesce with it");
        block = coalesce_blocks(block, next_block);
    }

    return;
}

/**
 * The realloc function.
 * Can grow/shrink a given block.
 * Given block must be already allocated (not free!)
 */
void *sf_realloc(void *pp, size_t rsize)
{
    sf_errno = 0;

    if (rsize == 0)
    {
        debug("realloc called with size 0, will try to free.");
        sf_free(pp);
        return NULL;
    }

    size_t orsize = rsize;
    if (rsize % 16 != 0)
    {
        debug("rsize is: %lu", rsize);
        rsize = rsize + (16 - (rsize % 16));
        debug("rsize is: %lu", rsize);
    }

    if (orsize > rsize) {
        error("size_t int overflow");
        return NULL;
    }

    if (pp == NULL)
    {
        debug("tried to pass in null pointer");
        sf_errno = EINVAL;
        abort();
    }

    pp = pp - 8 - 8;
    sf_block *block = (sf_block *)pp;

    if ((block->header & 0x2) == 0)
    {
        debug("tried to realloc a non-allocated block at %p", block);
        sf_errno = EINVAL;
        abort();
    }

    size_t size_of_block = (block->header >> 2) << 2;
    size_of_block = size_of_block;
    debug("the size of the block im trying to realloc is: %lu", size_of_block);

    if ((sf_mem_start() + 40) > (void *)(&(block->header)))
    {
        debug("tried to realloc block not starting in heap");
        debug("start: %p", (sf_mem_start() + 40));
        debug("pointing: %p", (void *)(&(block->header)));
        sf_errno = EINVAL;
        abort();
    }

    void *footer_addr = ((void *)&block->header) + ((block->header >> 2) << 2);
    if (footer_addr > (sf_mem_end() - 8))
    {
        debug("tried to realloc block not ending in heap %p is footer;; %p is end of heap", footer_addr, sf_mem_end() - 8);
        sf_errno = EINVAL;
        abort();
    }

    if (((block->header >> 2) << 2) < 32)
    {
        debug("tried to realloc a block that has a size smaller than permitted minimum");
        sf_errno = EINVAL;
        abort();
    }

    if ((block->header & 0x1) == 0)
    {
        debug("the previous block, according the prev-header bit, is NOT allocated");

        if (*((sf_header *)((void *)(&block->header) - 0 - (((*((sf_footer *)((void *)(&block->header) - 8)) ^ sf_magic()) >> 2) << 2))) & 0x2)
        {
            debug("the previous header block says it IS allocated");
            sf_errno = EINVAL;
            abort();
        }
    }

    if ((*((sf_footer *)((((void *)(&block->header)) + ((block->header >> 2) << 2)) - 8)) ^ sf_magic()) != block->header)
    {
        debug("footer and header do not match");
        sf_errno = EINVAL;
        abort();
    }

    debug("new calculated realloc size is: %lu", rsize);
    size_t current_size = (block->header >> 2) << 2;

    if (current_size < rsize)
    {
        debug("realloc to a larger block");

        void *new_block = sf_malloc(rsize);
        if (new_block == NULL)
        {
            error("unable to malloc a larger block");
            sf_errno = ENOMEM;
            return NULL;
        }

        memcpy(new_block, block->body.payload, current_size);

        debug("free the extra split block");
        if (current_size == 0)
        {
            debug("nothing to free, current block is size 0");
        }
        else
        {
            sf_free(block->body.payload);
        }

        return new_block;
    }
    else if (current_size > rsize)
    {
        rsize = rsize + 16;
        debug("realloc to a smaller block %lu", rsize);

        if (current_size - rsize < 32)
        {
            debug("it would result in a splinter, thus do not split");
            return block->body.payload;
        }
        else
        {
            debug("attempt to split down the block");
            return split_down(pp, rsize);
        }
    }
    else
    {
        debug("block is already of the specified size");
        return block->body.payload;
    }

    error("shouldn't be able to reach");
    return NULL;
}
