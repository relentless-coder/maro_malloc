#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <unistd.h>
#include "./maro_malloc.h"

void* maro_malloc(size_t size) {
  if (size == 0) {
    return NULL;
  }
  size_t block_size = sizeof(block_header_t) + size;
  block_header_t* block = sbrk(block_size);
  if (block == (void*)-1) {
    return NULL;
  }
  block->size = block_size;
  block->is_free = 0;
  return block + 1;
}
