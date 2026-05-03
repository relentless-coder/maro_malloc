#include "maro_malloc.h"
#include "munit.h"
#include <stdio.h>
#include <string.h>

// Reset free list between tests
extern block_header_t *free_list_head;

static void reset_free_list(void) { free_list_head = NULL; }

// Helper: Count nodes in free list
static int count_free_list_nodes(void) {
  printf("DEBUG count: free_list_head=%p\n", free_list_head);
  int count = 0;
  block_header_t *curr = free_list_head;
  while (curr != NULL) {
    printf("  node %d at %p, next=%p\n", count, curr, curr->next_free);
    count++;
    curr = curr->next_free;
  }
  printf("  total count=%d\n", count);
  return count;
}

// Helper: Verify free list is sorted by address
static int is_free_list_sorted(void) {
  if (free_list_head == NULL)
    return 1;

  block_header_t *curr = free_list_head;
  while (curr->next_free != NULL) {
    if (curr >= curr->next_free) {
      return 0; // Not sorted
    }
    curr = curr->next_free;
  }
  return 1;
}

/* Test: Allocate memory successfully */
static MunitResult test_malloc_basic(const MunitParameter params[],
                                     void *data) {
  (void)params;
  (void)data;

  void *ptr = maro_malloc(100);
  munit_assert_not_null(ptr);

  // Should be able to write to it
  memset(ptr, 0x42, 100);

  maro_free(ptr);
  return MUNIT_OK;
}

/* Test: NULL for zero-size allocation */
static MunitResult test_malloc_zero_size(const MunitParameter params[],
                                         void *data) {
  (void)params;
  (void)data;

  void *ptr = maro_malloc(0);
  munit_assert_null(ptr);

  return MUNIT_OK;
}

/* Test: Free NULL pointer (should not crash) */
static MunitResult test_free_null(const MunitParameter params[], void *data) {
  (void)params;
  (void)data;

  maro_free(NULL); // Should not crash

  return MUNIT_OK;
}

/* Test: Multiple allocations */
static MunitResult test_malloc_multiple(const MunitParameter params[],
                                        void *data) {
  (void)params;
  (void)data;

  void *p1 = maro_malloc(100);
  void *p2 = maro_malloc(200);
  void *p3 = maro_malloc(50);

  munit_assert_not_null(p1);
  munit_assert_not_null(p2);
  munit_assert_not_null(p3);

  // All should be different
  munit_assert_ptr_not_equal(p1, p2);
  munit_assert_ptr_not_equal(p2, p3);
  munit_assert_ptr_not_equal(p1, p3);

  maro_free(p1);
  maro_free(p2);
  maro_free(p3);

  return MUNIT_OK;
}

/* Test: Reuse freed memory */
static MunitResult test_free_list_reuse(const MunitParameter params[],
                                        void *data) {
  (void)params;
  (void)data;

  reset_free_list();

  // Allocate TWO blocks, so first one won't be at edge when freed
  void *p1 = maro_malloc(100);
  void *p_keep = maro_malloc(50); // Keep this allocated so p1 isn't at edge
  munit_assert_not_null(p1);
  munit_assert_not_null(p_keep);

  // Get the header to check later
  block_header_t *header1 = (block_header_t *)p1 - 1;

  maro_free(p1); // Now p1 is NOT at edge, should go to free list

  // Should be marked as free
  munit_assert_int(header1->is_free, ==, 1);

  // Allocate same size - should reuse
  void *p2 = maro_malloc(100);
  munit_assert_not_null(p2);

  // Should be the same block (or at least same header)
  block_header_t *header2 = (block_header_t *)p2 - 1;
  munit_assert_ptr_equal(header1, header2);

  // Should be marked as not free
  munit_assert_int(header2->is_free, ==, 0);

  maro_free(p2);
  maro_free(p_keep);

  return MUNIT_OK;
}

/* Test: Allocate smaller than freed block (should still reuse) */
static MunitResult test_free_list_smaller_reuse(const MunitParameter params[],
                                                void *data) {
  (void)params;
  (void)data;

  reset_free_list();

  void *p1 = maro_malloc(200);
  void *p_keep = maro_malloc(50); // Keep this so p1 isn't at edge
  munit_assert_not_null(p1);
  munit_assert_not_null(p_keep);

  block_header_t *header1 = (block_header_t *)p1 - 1;

  maro_free(p1); // Not at edge, goes to free list

  // Allocate smaller - should still fit in the 200-byte block
  void *p2 = maro_malloc(50);
  munit_assert_not_null(p2);

  block_header_t *header2 = (block_header_t *)p2 - 1;
  munit_assert_ptr_equal(header1, header2);

  maro_free(p2);
  maro_free(p_keep);

  return MUNIT_OK;
}

/* Test: Free list doesn't reuse if block too small */
static MunitResult test_free_list_no_reuse_small(const MunitParameter params[],
                                                 void *data) {
  (void)params;
  (void)data;

  reset_free_list();

  void *p1 = maro_malloc(50);
  void *p_keep = maro_malloc(50); // Prevent edge optimization
  munit_assert_not_null(p1);
  munit_assert_not_null(p_keep);

  block_header_t *header1 = (block_header_t *)p1 - 1;

  maro_free(p1);

  // Allocate bigger - should NOT reuse the 50-byte block
  void *p2 = maro_malloc(200);
  munit_assert_not_null(p2);

  block_header_t *header2 = (block_header_t *)p2 - 1;
  munit_assert_ptr_not_equal(header1, header2);

  maro_free(p2);
  maro_free(p_keep);

  return MUNIT_OK;
}

/* Test: Free list is maintained in sorted order */
static MunitResult test_free_list_sorted(const MunitParameter params[],
                                         void *data) {
  (void)params;
  (void)data;

  reset_free_list();

  // Allocate several blocks
  void *p1 = maro_malloc(100);
  void *p2 = maro_malloc(100);
  void *p3 = maro_malloc(100);
  void *p4 = maro_malloc(100);
  void *p_keep = maro_malloc(50); // Prevent edge optimization

  // Free in random order
  maro_free(p3);
  maro_free(p1);
  maro_free(p4);
  maro_free(p2);

  // Free list should still be sorted
  munit_assert_int(is_free_list_sorted(), ==, 1);

  maro_free(p_keep);

  return MUNIT_OK;
}

/* Test: Coalesce - both neighbors free (Case 1) */
static MunitResult test_coalesce_both_neighbors(const MunitParameter params[],
                                                void *data) {
  (void)params;
  (void)data;

  reset_free_list();

  // Allocate 4 adjacent blocks: A B C D
  void *a = maro_malloc(100);
  void *b = maro_malloc(100);
  void *c = maro_malloc(100);
  void *d = maro_malloc(100);
  void *keep = maro_malloc(50); // Prevent edge optimization

  munit_assert_not_null(a);
  munit_assert_not_null(b);
  munit_assert_not_null(c);
  munit_assert_not_null(d);

  // Free A and C (leaving B allocated in between)
  maro_free(a);
  maro_free(c);

  // Should have 2 nodes in free list
  munit_assert_int(count_free_list_nodes(), ==, 2);

  // Now free B - it should coalesce with both A and C
  maro_free(b);

  // Should now have only 1 node (A+B+C merged)
  munit_assert_int(count_free_list_nodes(), ==, 1);

  // The merged block should be large enough for all three
  size_t total_size = 3 * (sizeof(block_header_t) + 100);
  munit_assert_true(free_list_head->size >= total_size);

  maro_free(d);
  maro_free(keep);

  return MUNIT_OK;
}

/* Test: Coalesce - only previous neighbor free (Case 3) */
static MunitResult test_coalesce_prev_only(const MunitParameter params[],
                                           void *data) {
  (void)params;
  (void)data;

  reset_free_list();

  // Allocate 3 blocks: A B C
  void *a = maro_malloc(100);
  void *b = maro_malloc(100);
  void *c = maro_malloc(100);
  void *keep = maro_malloc(50);

  // Free A (keep B and C allocated)
  maro_free(a);

  // Should have 1 node
  munit_assert_int(count_free_list_nodes(), ==, 1);

  block_header_t *a_header = (block_header_t *)a - 1;
  size_t original_size = a_header->size;

  // Free B - should coalesce with A
  maro_free(b);

  // Should still have 1 node (A+B merged)
  munit_assert_int(count_free_list_nodes(), ==, 1);

  // Size should have grown
  munit_assert_true(a_header->size > original_size);

  maro_free(c);
  maro_free(keep);

  return MUNIT_OK;
}

/* Test: Coalesce - only next neighbor free (Case 2) */
static MunitResult test_coalesce_next_only(const MunitParameter params[],
                                           void *data) {
  (void)params;
  (void)data;

  reset_free_list();

  // Allocate 3 blocks: A B C
  void *a = maro_malloc(100);
  void *b = maro_malloc(100);
  void *c = maro_malloc(100);
  void *keep = maro_malloc(50);

  // Free B (keep A and C allocated)
  maro_free(b);

  // Should have 1 node
  munit_assert_int(count_free_list_nodes(), ==, 1);

  // Free A - should coalesce with B
  maro_free(a);

  // Should still have 1 node (A+B merged)
  munit_assert_int(count_free_list_nodes(), ==, 1);

  // The merged block should be at A's address
  munit_assert_ptr_equal(free_list_head, (block_header_t *)a - 1);

  maro_free(c);
  maro_free(keep);

  return MUNIT_OK;
}

/* Test: No coalesce when neighbors are allocated (Case 4) */
static MunitResult
test_no_coalesce_neighbors_allocated(const MunitParameter params[],
                                     void *data) {
  (void)params;
  (void)data;

  reset_free_list();

  // Allocate 5 blocks: A B C D E
  void *a = maro_malloc(100);
  void *b = maro_malloc(100);
  void *c = maro_malloc(100);
  void *d = maro_malloc(100);
  void *e = maro_malloc(100);
  void *keep = maro_malloc(50);

  // Free A, C, E (keep B and D allocated)
  maro_free(a);
  maro_free(c);
  maro_free(e);

  // Should have 3 separate nodes (no coalescing possible)
  munit_assert_int(count_free_list_nodes(), ==, 3);

  // Free list should still be sorted
  munit_assert_int(is_free_list_sorted(), ==, 1);

  maro_free(b);
  maro_free(d);
  maro_free(keep);

  return MUNIT_OK;
}

/* Test: Coalesce at head of list */
static MunitResult test_coalesce_at_head(const MunitParameter params[],
                                         void *data) {
  (void)params;
  (void)data;

  reset_free_list();

  // Allocate 3 blocks
  void *a = maro_malloc(100);
  void *b = maro_malloc(100);
  void *c = maro_malloc(100);
  void *keep = maro_malloc(50);

  // Free B first (goes to head)
  maro_free(b);

  // Free A (should coalesce with B at head)
  maro_free(a);

  // Should have 1 node
  munit_assert_int(count_free_list_nodes(), ==, 1);

  // Head should be at A's address
  munit_assert_ptr_equal(free_list_head, (block_header_t *)a - 1);

  maro_free(c);
  maro_free(keep);

  return MUNIT_OK;
}

/* Test: Write and read data */
static MunitResult test_data_integrity(const MunitParameter params[],
                                       void *data) {
  (void)params;
  (void)data;

  int *numbers = (int *)maro_malloc(5 * sizeof(int));
  munit_assert_not_null(numbers);

  // Write data
  for (int i = 0; i < 5; i++) {
    numbers[i] = i * 10;
  }

  // Read back
  munit_assert_int(numbers[0], ==, 0);
  munit_assert_int(numbers[1], ==, 10);
  munit_assert_int(numbers[2], ==, 20);
  munit_assert_int(numbers[3], ==, 30);
  munit_assert_int(numbers[4], ==, 40);

  maro_free(numbers);

  return MUNIT_OK;
}

/* Test: Complex interleaved malloc/free pattern */
static MunitResult test_complex_pattern(const MunitParameter params[],
                                        void *data) {
  (void)params;
  (void)data;

  reset_free_list();

  void *blocks[10];

  // Allocate 10 blocks
  for (int i = 0; i < 10; i++) {
    blocks[i] = maro_malloc(100);
    munit_assert_not_null(blocks[i]);
  }

  void *keep = maro_malloc(50); // Prevent edge optimization

  // Free odd indices
  for (int i = 1; i < 10; i += 2) {
    maro_free(blocks[i]);
  }

  // Should have 5 nodes
  munit_assert_int(count_free_list_nodes(), ==, 5);

  // Free even indices (should trigger coalescing)
  for (int i = 0; i < 10; i += 2) {
    maro_free(blocks[i]);
  }

  // After coalescing, should have fewer nodes
  int final_count = count_free_list_nodes();
  munit_assert_true(final_count <= 5);

  // Free list should be sorted
  munit_assert_int(is_free_list_sorted(), ==, 1);

  maro_free(keep);

  return MUNIT_OK;
}

static MunitTest test_suite_tests[] = {
    {(char *)"/malloc_basic", test_malloc_basic, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/malloc_zero_size", test_malloc_zero_size, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/free_null", test_free_null, NULL, NULL, MUNIT_TEST_OPTION_NONE,
     NULL},
    {(char *)"/malloc_multiple", test_malloc_multiple, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/free_list_reuse", test_free_list_reuse, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/free_list_smaller_reuse", test_free_list_smaller_reuse, NULL,
     NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/free_list_no_reuse_small", test_free_list_no_reuse_small, NULL,
     NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/free_list_sorted", test_free_list_sorted, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/coalesce_both_neighbors", test_coalesce_both_neighbors, NULL,
     NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/coalesce_prev_only", test_coalesce_prev_only, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/coalesce_next_only", test_coalesce_next_only, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/no_coalesce_neighbors_allocated",
     test_no_coalesce_neighbors_allocated, NULL, NULL, MUNIT_TEST_OPTION_NONE,
     NULL},
    {(char *)"/coalesce_at_head", test_coalesce_at_head, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/data_integrity", test_data_integrity, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/complex_pattern", test_complex_pattern, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};

static const MunitSuite test_suite = {(char *)"/maro_malloc", test_suite_tests,
                                      NULL, 1, MUNIT_SUITE_OPTION_NONE};

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
  return munit_suite_main(&test_suite, NULL, argc, argv);
}
