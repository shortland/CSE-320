#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include "debug.h"
#include "sfmm.h"

#define MIN_BLOCK_SIZE (32)

void assert_free_block_count(size_t size, int count);
void assert_free_list_block_count(size_t size, int count);

/*
 * Assert the total number of free blocks of a specified size.
 * If size == 0, then assert the total number of all free blocks.
 */
void assert_free_block_count(size_t size, int count) {
    int cnt = 0;
    for(int i = 0; i < NUM_FREE_LISTS; i++) {
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	while(bp != &sf_free_list_heads[i]) {
	    if(size == 0 || size == (bp->header & BLOCK_SIZE_MASK))
		cnt++;
	    bp = bp->body.links.next;
	}
    }
    cr_assert_eq(cnt, count, "Wrong number of free blocks of size %ld (exp=%d, found=%d)",
		 size, count, cnt);
}

void assert_normal_exit(int status) {
    cr_assert_eq(status, 0, "Did not exit normally (status = 0x%x).\n", status);
}

void assert_error_exit(int status) {
    cr_assert_eq(WEXITSTATUS(status), 0xff, "Did not exit with status 0xff (status was 0x%x).\n", status);
}

Test(sf_memsuite_student, malloc_an_Integer_check_freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	int *x = sf_malloc(sizeof(int));

	cr_assert_not_null(x, "x is NULL!");

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

	assert_free_block_count(0, 1);
	assert_free_block_count(4016, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(sf_mem_start() + PAGE_SZ == sf_mem_end(), "Allocated more than necessary!");
}

Test(sf_memsuite_student, malloc_three_pages, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	void *x = sf_malloc(3 * PAGE_SZ - 2 * sizeof(sf_block));

	//sf_show_heap();

	cr_assert_not_null(x, "x is NULL!");
	assert_free_block_count(0, 0);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

Test(sf_memsuite_student, malloc_over_four_pages, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	void *x = sf_malloc(PAGE_SZ << 2);

	//sf_show_heap();

	cr_assert_null(x, "x is not NULL!");
	assert_free_block_count(0, 1);
	assert_free_block_count(16336, 1);
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sf_memsuite_student, free_quick, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	/* void *x = */ sf_malloc(8);
	void *y = sf_malloc(32);
	/* void *z = */ sf_malloc(1);

	sf_free(y);

	assert_free_block_count(0, 2);
	assert_free_block_count(3936, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, free_no_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	/* void *x = */ sf_malloc(8);
	void *y = sf_malloc(200);
	/* void *z = */ sf_malloc(1);

	sf_free(y);

	assert_free_block_count(0, 2);
	assert_free_block_count(224, 1);
	assert_free_block_count(3760, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, free_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	/* void *w = */ sf_malloc(8);
	void *x = sf_malloc(200);
	void *y = sf_malloc(300);
	/* void *z = */ sf_malloc(4);

	sf_free(y);
	sf_free(x);

	assert_free_block_count(0, 2);
	assert_free_block_count(544, 1);
	assert_free_block_count(3440, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *u = sf_malloc(200);
	/* void *v = */ sf_malloc(300);
	void *w = sf_malloc(200);
	/* void *x = */ sf_malloc(500);
	void *y = sf_malloc(200);
	/* void *z = */ sf_malloc(700);

	sf_free(u);
	sf_free(w);
	sf_free(y);

	assert_free_block_count(0, 4);
	assert_free_block_count(224, 3);
	assert_free_block_count(1808, 1);

	// First block in list should be the most recently freed block.
	int i = 3;
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	cr_assert_eq(bp, (char *)y - 2*sizeof(sf_header),
		     "Wrong first block in free list %d: (found=%p, exp=%p)",
                     i, bp, (char *)y - 2*sizeof(sf_header));
}

Test(sf_memsuite_student, realloc_larger_block, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(int));
	/* void *y = */ sf_malloc(10);
	x = sf_realloc(x, sizeof(int) * 10);

	//sf_show_heap();

	cr_assert_not_null(x, "x is NULL!");
	sf_block *bp = (sf_block *)((char *)x - 2*sizeof(sf_header));
	cr_assert(bp->header & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
	cr_assert((bp->header & BLOCK_SIZE_MASK) == 64, "Realloc'ed block size not what was expected!");

	assert_free_block_count(0, 2);
	assert_free_block_count(3920, 1);
}

Test(sf_memsuite_student, realloc_smaller_block_splinter, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(int) * 8);
	void *y = sf_realloc(x, sizeof(char));

	cr_assert_not_null(y, "y is NULL!");
	cr_assert(x == y, "Payload addresses are different!");

	sf_block *bp = (sf_block *)((char*)y - 2*sizeof(sf_header));
	cr_assert(bp->header & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
	cr_assert((bp->header & BLOCK_SIZE_MASK) == 48, "Block size not what was expected!");

	// There should be only one free block of size 4000.
	assert_free_block_count(0, 1);
	assert_free_block_count(4000, 1);
}

Test(sf_memsuite_student, realloc_smaller_block_free_block, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(double) * 8);

//sf_show_heap();

	void *y = sf_realloc(x, sizeof(int));

	cr_assert_not_null(y, "y is NULL!");

	sf_block *bp = (sf_block *)((char*)y - 2*sizeof(sf_header));
	cr_assert(bp->header & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
	cr_assert((bp->header & BLOCK_SIZE_MASK) == 32, "Realloc'ed block size not what was expected!");

	// After realloc'ing x, we can return a block of size 48 to the freelist.
	// This block will go into the main freelist and be coalesced.
	assert_free_block_count(0, 1);
	assert_free_block_count(4016, 1);
}

//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//############################################

/**
 * This test allocates all the space from the first block,
 * Then attempts to malloc more space which must result in a memgrow.
 * This checks whether a students code actually calls mem_grow on a heap
 * that is 100% full with no splinters/free blocks.
 */
Test(sf_memsuite_student, zmem_grow_called_on_full_heap, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	void *x = sf_malloc(4032);

	assert_free_block_count(0, 0);

	sf_malloc(1);

	assert_free_block_count(4064, 1);

	cr_assert_not_null(x, "x is NULL!");
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

/**
 * This test is very general, but its purpose is to check whether the coalescing and
 * free-ing of the programs ability is intact
 */
Test(sf_memsuite_student, comprehensive_scattered_coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	void *a = sf_malloc(64);
	void *b = sf_malloc(64);
	void *c = sf_malloc(64);
	void *d = sf_malloc(64);
	void *e = sf_malloc(64);
	void *f = sf_malloc(64);

	sf_free(a);
	sf_free(c);
	sf_free(e);

	cr_assert_not_null(a, "x is NULL!");
	cr_assert_not_null(b, "x is NULL!");
	cr_assert_not_null(c, "x is NULL!");
	cr_assert_not_null(d, "x is NULL!");
	cr_assert_not_null(e, "x is NULL!");
	cr_assert_not_null(f, "x is NULL!");

	assert_free_block_count(0, 4);

	sf_free(b);
	sf_free(d);
	sf_free(f);

	assert_free_block_count(0, 1);

	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

// Test(sf_memsuite_student, free_block_not_in_heap, .init = sf_mem_init, .fini = sf_mem_fini) {
// 	sf_errno = 0;

// 	sf_block ok;
// 	ok.header = 32 | 3;
// 	sf_footer nok;
// 	nok = ok.header ^ sf_magic();
// 	nok=nok;
// 	// even though nok is not used, it's technically in memory directly asfter the block ok
// 	// since it was declared right afterwards.

// 	sf_free(&ok);
// 	cr_assert(sf_errno == 0, "sf_errno is not 0!");
// }

// realloc on a free block should be false
// Test(sf_memsuite_student, zrealloc_freed_block, .init = sf_mem_init, .fini = sf_mem_fini) {
// 	sf_errno = 0;
// 	void *x = sf_malloc(96);
// 	sf_free(x);
// 	sf_realloc(x, 40);

// 	cr_assert_not_null(x, "x is NULL!");
// 	assert_free_block_count(0, 1);
// 	cr_assert(sf_errno == 0, "sf_errno is not 0!");
// }

// char *name = "invalid_search_test";
//     int err = run_using_system(name, "", "");
//     assert_normal_exit(err);
