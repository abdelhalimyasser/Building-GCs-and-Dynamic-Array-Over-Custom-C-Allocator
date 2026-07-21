#ifndef ALLOCATOR_H_INCLUDED
#define ALLOCATOR_H_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define HEAP_SIZE 64 * 1024

typedef struct BlockHeader
{
    struct BlockHeader *next;
    size_t size;
    bool is_free;
} BlockHeader;

#define ALIGN(size) (((size) + 7) & ~7)
#define BLOCK_HEADER_SIZE ALIGN(sizeof(BlockHeader))

/**
 * @brief Initializes the custom memory allocator.
 *
 * Sets up the initial heap space using mmap and initializes
 * the free list. This function prepares the underlying memory core
 * before any allocations can occur.
 *
 * @param void No parameters required.
 * @return void
 * @note This function MUST be called exactly once at the beginning of the program
 *       before calling malloc, realloc, or free.
 */
void mem_init(void);

/**
 * @brief Allocates a block of memory of the specified size.
 *
 * Searches the free list for an available block that satisfies the requested size.
 * If a larger block is found, it may be split to minimize internal fragmentation.
 * The size will be aligned to the architecture's word boundary (e.g., 8 bytes).
 *
 * @param size The number of bytes to allocate.
 * @return void* A pointer to the allocated memory, or NULL if allocation fails.
 * @note The returned memory is uninitialized. The actual allocated space might be
 *       slightly larger than requested due to alignment and block headers.
 */
void *mem_malloc(size_t size);

/**
 * @brief Deallocates a previously allocated memory block.
 *
 * Returns the memory block pointed to by ptr to the free list, making it available
 * for future allocations. It may also attempt to coalesce (merge) adjacent free
 * blocks to reduce memory fragmentation.
 *
 * @param ptr Pointer to the memory block to be freed.
 * @return void
 * @note If ptr is NULL, no operation is performed. Passing a pointer that was not
 *       allocated by this allocator results in undefined behavior.
 */
void mem_free(void *ptr);

/**
 * @brief Resizes a previously allocated memory block.
 *
 * Changes the size of the memory block pointed to by ptr to new_size bytes.
 * If the new size is larger and cannot be expanded in place, a new block is
 * allocated, the existing data is copied, and the old block is freed.
 *
 * @param ptr Pointer to the previously allocated memory block.
 * @param new_size The new requested size in bytes.
 * @return void* A pointer to the newly sized memory block, which may be different
 *         from the original ptr. Returns NULL if the allocation fails.
 * @note If ptr is NULL, the call is equivalent to mem_malloc(new_size).
 *       If new_size is 0, the call is equivalent to mem_free(ptr).
 */
void *mem_realloc(void *ptr, size_t new_size);

#endif // ALLOCATOR_H_INCLUDED
