# maro_malloc

I have recently dived into the world of systems programming. I've always been quite facinated with memory, and wanted to understand the innerworkings of assigning memory.

So started this project. This maintains a sorted linked list of free memory, splits memory blocks, if the required memory is less than the size of a selected block. Upon freeing up a block, it sets in the free list, and coalesces it with adjacent blocks in the free list.

## Next Goals

- thread safety
- implement binning.
