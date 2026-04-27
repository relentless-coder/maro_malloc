#include <stddef.h>

typedef struct BlockHeader block_header_t;

typedef struct BlockHeader {
  size_t size;
  int is_free;
  block_header_t* next_free;
} block_header_t;

void* maro_malloc(size_t size);

void maro_free(void* p);

static block_header_t* free_list_head = NULL;

block_header_t* add_to_head_of_free_list(block_header_t* free_block);

block_header_t* find_free_memory(size_t size);
