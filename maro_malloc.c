#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <unistd.h>
#include "./maro_malloc.h"

block_header_t* add_to_head_of_free_list(block_header_t* free_block) {
  if (free_block == NULL) {
    return NULL;
  }
  if (free_list_head == NULL) {
    free_list_head = free_block;
    free_list_head->next_free = NULL;
  } else {
    free_block->next_free = free_list_head;
    free_list_head = free_block;
  }
  return free_list_head;
}

block_header_t* find_free_memory(size_t size) {
  if (free_list_head == NULL) {
    return NULL;
  }
  block_header_t* prev = NULL;
  block_header_t* curr = free_list_head;
  while (curr != NULL) {
    if (curr->size >= size) {
      if (prev == NULL) {
        free_list_head = free_list_head->next_free;
      } else {
        prev->next_free = curr->next_free;
      }
      return curr;
    } else {
      prev = curr;
      curr = curr->next_free;
    }
  }
  return NULL;
}

void* maro_malloc(size_t size) {
  if (size == 0) {
    return NULL;
  }
  size_t block_size = sizeof(block_header_t) + size;
  block_header_t* free_available = find_free_memory(block_size);
  if (free_available != NULL) {
    free_available->is_free = 0;
    free_available->next_free = NULL;
    return free_available + 1;
  }
  block_header_t* block = sbrk(block_size);
  if (block == (void*)-1) {
    return NULL;
  }
  block->size = block_size;
  block->is_free = 0;
  return block + 1;
}

void maro_free(void* p) {
  if (p == NULL) {
    return;
  }

  block_header_t* header = (block_header_t*)p - 1;

  void* block_end = (char*)header +  header->size;

  if (block_end == sbrk(0)) {
    sbrk(-(header->size));
    return;
  }

  header->is_free = 1;
  add_to_head_of_free_list(header);
}
