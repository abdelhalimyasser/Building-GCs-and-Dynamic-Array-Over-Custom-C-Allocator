#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include "allocator.h"

static void *heap = NULL;             // heap pointer that will created
static bool is_initialized = false;   // indicator if the heap is init or not
static BlockHeader *free_list = NULL; // the heap that created

// function used to split the memory block to avoid fragmentation or using too much blocks
static void split_block(BlockHeader *block, size_t needed_size)
{
    if (block->size >= needed_size + BLOCK_HEADER_SIZE + 8)
    {
        // change the place of the new block in the mem by the amount of current block and the needed size in addition to BLOCK_HEADER_SIZE
        BlockHeader *new_block = (BlockHeader *)((uint8_t *)block + BLOCK_HEADER_SIZE + needed_size);

        // the new block next will be the next of the block and the size will be block size - needed size - block header size
        new_block->next = block->next;
        new_block->size = block->size - needed_size - BLOCK_HEADER_SIZE;
        new_block->is_free = true;

        // change the old block size to the needed one and the next to be the whole new block
        block->next = new_block;
        block->size = needed_size;
    }
}

// function used to reduce the fragmentation and use mem efficiently
static void coalesce_blocks(void)
{
    BlockHeader *current = free_list;

    // loop at all the blocks to prevent any fragmentation in the memory
    while (current != NULL && current->next != NULL)
    {
        if (current->is_free && current->next->is_free)
        {
            current->size += current->next->size + BLOCK_HEADER_SIZE; // sum the size of the memory of the current + the next block + the block header
            current->next = current->next->next;                      // change the pointer to next space
        }
        else
        {
            current = current->next;
        }
    }
}

void mem_init(void)
{
    if (is_initialized)
        return;

    heap = mmap(NULL, HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (heap == MAP_FAILED)
        return;

    free_list = (BlockHeader *)heap;

    free_list->next = NULL;
    free_list->size = HEAP_SIZE - BLOCK_HEADER_SIZE;
    free_list->is_free = true;

    is_initialized = true;
}

void *mem_malloc(size_t size)
{
    if (size <= 0)
        return NULL;

    if (!is_initialized)
        mem_init();

    // take the new size and align it to the next Align size
    size_t aligned_size = ALIGN(size);

    // copy the freelist into current to do our loop on it
    BlockHeader *current = free_list;

    while (current != NULL)
    {
        if (current->is_free && current->size >= aligned_size)
        {
            split_block(current, aligned_size); // split the block to exact needed one to reduce space
            current->is_free = false;           // save that the new block is not empty
            return (void *)((uint8_t *)current + BLOCK_HEADER_SIZE);
        }

        // if this block is not sufficient then go to the next one
        current = current->next;
    }

    return NULL;
}

void mem_free(void *ptr)
{
    if (ptr == NULL)
        return;

    // take the pointer and remove the header from it to be free and unused mem.
    BlockHeader *block = (BlockHeader *)((uint8_t *)ptr - BLOCK_HEADER_SIZE);

    // change the state of the current block
    block->is_free = true;

    // clean to remove any fragmentation
    coalesce_blocks();
}

void *mem_realloc(void *ptr, size_t new_size)
{
    if (ptr == NULL)
        return mem_malloc(new_size);

    if (new_size == 0)
    {
        mem_free(ptr);
        return NULL;
    }

    // take the pointer and remove the header from it to be free and unused mem.
    BlockHeader *block = (BlockHeader *)((uint8_t *)ptr - BLOCK_HEADER_SIZE);

    // take the new size and align it to the next Align size
    size_t aligned_size = ALIGN(new_size);

    // if the block size was very large and just used part from it then split to reduce mem
    if (block->size >= aligned_size)
    {
        split_block(block, aligned_size); // split the large space of mem
        return ptr;                       // return the same ptr
    }

    // case of the space was too large then allocate with bigger one
    void *bigger_ptr = mem_malloc(aligned_size);

    if (bigger_ptr == NULL)
        return NULL;

    // copy the old pointer to the new one
    memcpy(bigger_ptr, ptr, block->size);

    // delete the old pointer
    mem_free(ptr);

    return bigger_ptr;
}
