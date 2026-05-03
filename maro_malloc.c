#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include "./maro_malloc.h"
#include <stdio.h>
#include <unistd.h>

block_header_t *free_list_head = NULL;

int are_blocks_adjacent(block_header_t *p, block_header_t *q) {
  if (p == NULL || q == NULL) {
    return 0;
  }
  char *prev_block_end = (char *)p + p->size;
  if (prev_block_end == (char *)q) {
    return 1;
  }
  return 0;
}

block_header_t *add_to_sorted_free_list(block_header_t *free_block) {
  printf("DEBUG add_to_sorted_free_list: adding %p, free_list_head=%p\n",
         free_block, free_list_head);
  if (free_block == NULL) {
    return NULL;
  }
  if (free_list_head == NULL) {
    printf("  -> Empty list, setting as head\n");
    free_list_head = free_block;
    free_list_head->next_free = NULL;
  } else {
    printf("  -> Finding insertion point...\n");
    block_header_t *prev = NULL;
    block_header_t *curr = free_list_head;
    while (curr != NULL && curr < free_block) {
      prev = curr;
      curr = curr->next_free;
    }
    printf("  -> Found position: prev=%p, curr=%p\n", prev, curr);
    if (are_blocks_adjacent(prev, free_block) == 1 &&
        are_blocks_adjacent(free_block, curr) == 1) {
      printf("  -> Case 1: Both adjacent\n");
      prev->size = prev->size + free_block->size + curr->size;
      prev->next_free = curr->next_free;
      return free_list_head;
    } else if (are_blocks_adjacent(prev, free_block) == 0 &&
               are_blocks_adjacent(free_block, curr) == 1) {
      printf("  -> Case 2: next adjacent\n");

      free_block->size = free_block->size + curr->size;
      free_block->next_free = curr->next_free;
      if (prev == NULL) {
        free_list_head = free_block;
      } else {
        prev->next_free = free_block;
      }
    } else if (are_blocks_adjacent(prev, free_block) == 1 &&
               are_blocks_adjacent(free_block, curr) == 0) {

      printf("  -> Case 3: prev adjacent\n");
      prev->size = prev->size + free_block->size;
    } else {
      printf("  -> Case 4: None adjacent\n");
      if (prev == NULL) {
        free_list_head = free_block;
      } else {
        prev->next_free = free_block;
      }
      free_block->next_free = curr;
    }
  }
  return free_list_head;
}

block_header_t *find_free_memory(size_t size) {
  if (free_list_head == NULL) {
    return NULL;
  }
  block_header_t *prev = NULL;
  block_header_t *curr = free_list_head;
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

void *maro_malloc(size_t size) {
  if (size == 0) {
    return NULL;
  }
  size_t block_size = sizeof(block_header_t) + size;
  block_header_t *free_available = find_free_memory(block_size);
  if (free_available != NULL) {
    free_available->is_free = 0;
    free_available->next_free = NULL;
    return free_available + 1;
  }
  block_header_t *block = sbrk(block_size);
  if (block == (void *)-1) {
    return NULL;
  }
  block->size = block_size;
  block->is_free = 0;
  return block + 1;
}

void maro_free(void *p) {
  if (p == NULL) {
    return;
  }

  block_header_t *header = (block_header_t *)p - 1;

  void *block_end = (char *)header + header->size;

  printf("DEBUG maro_free: freeing %p, block_end=%p, sbrk(0)=%p\n", header,
         block_end, sbrk(0));

  if (block_end == sbrk(0)) {
    printf("  -> Returning to OS\n");
    sbrk(-(header->size));
    return;
  }

  header->is_free = 1;
  printf("  -> Adding to free list\n");
  add_to_sorted_free_list(header);
}
