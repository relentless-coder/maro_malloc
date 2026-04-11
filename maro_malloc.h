#include <stddef.h>

typedef struct BlockHeader block_header_t;

typedef struct BlockHeader {
  size_t size;
  int is_free;
} block_header_t;

void* maro_malloc(size_t size);
