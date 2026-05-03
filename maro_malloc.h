#include <stddef.h>

typedef struct BlockHeader block_header_t;

typedef struct BlockHeader {
  size_t size;
  int is_free;
  block_header_t* next_free;
} block_header_t;

void* maro_malloc(size_t size);

void maro_free(void* p);

extern block_header_t* free_list_head;

block_header_t* add_to_sorted_free_list(block_header_t* free_block);

block_header_t* find_free_memory(size_t size);

block_header_t* is_prev_block_free(block_header_t* current_block);

block_header_t* is_next_block_free(block_header_t* current_block);

block_header_t* find_prev_free_block_in_list(block_header_t* current_free_block);

void coalesce_free_blocks(block_header_t* prev, block_header_t* curr, block_header_t* next);

int are_blocks_adjacent(block_header_t*p, block_header_t*q);
