#include <string.h>
#include "mark_and_compact.h"
#include "../allocator/allocator.h"

VM *newVM(void)
{
    VM *vm = (VM *)mem_malloc(sizeof(VM));

    if (!vm)
        return NULL; // Handle memory allocation failure

    array_init(&vm->roots); // Initialize the dynamic array for roots
    vm->heap_capacity = DEFAULT_HEAP_CAPACITY;
    vm->num_objects = 0;

    vm->heap_start = (uint8_t *)mem_malloc(vm->heap_capacity); // Allocate the heap memory
    if (!vm->heap_start)
    {
        mem_free(vm); // Free the VM structure if heap allocation fails
        return NULL;
    }

    vm->next_free = vm->heap_start; // Set the next free pointer to the start of the heap as the heap is empty initially

    return vm;
}

void freeVM(VM *vm)
{
    if (!vm)
        return;

    gc(vm);                   // Compact and reclaim any remaining live objects
    array_free(&vm->roots);   // Free the dynamic array of roots
    mem_free(vm->heap_start); // Free the heap memory
    mem_free(vm);             // Free the VM structure itself
}

void pushRoot(VM *vm, Object *obj)
{
    if (!vm || !obj)
        return;

    array_push(&vm->roots, obj); // Add the object to the root stack and increment the count
}

Object *popRoot(VM *vm)
{
    if (!vm || vm->roots.count == 0)
        return NULL;

    return (Object *)array_pop(&vm->roots); // Decrement the root count and return the popped object
}

static Object *newObject(VM *vm, ObjectType type)
{
    if (!vm)
        return NULL;

    if ((size_t)((vm->heap_start + vm->heap_capacity) - vm->next_free) < sizeof(Object))
    {
        gc(vm);
        if ((size_t)((vm->heap_start + vm->heap_capacity) - vm->next_free) < sizeof(Object))
            return NULL;
    }

    Object *obj = (Object *)vm->next_free; // Allocate the new object at the next free location in the heap
    vm->next_free += sizeof(Object);       // Move the next free pointer forward by the size of the object

    obj->type = type; // Set the object type
    obj->next = NULL; // Initialize the next pointer to NULL (not marked)

    if (type == OBJ_PAIR) // Initialize head and tail to NULL for pair objects
    {
        obj->head = NULL;
        obj->tail = NULL;
    }

    vm->num_objects++; // Increment the total number of allocated objects

    return obj;
}

Object *pushInt(VM *vm, int intValue)
{
    if (!vm)
        return NULL;

    Object *obj = newObject(vm, OBJ_INT);
    if (!obj)
        return NULL;

    obj->int_value = intValue; // Set the integer value
    pushRoot(vm, obj);         // Automatically protect the object from GC by pushing it to roots

    return obj;
}

Object *pushChar(VM *vm, char charValue)
{
    if (!vm)
        return NULL;

    Object *obj = newObject(vm, OBJ_CHAR);
    if (!obj)
        return NULL;

    obj->char_value = charValue; // Set the character value
    pushRoot(vm, obj);           // Automatically protect the object from GC by pushing it to roots

    return obj;
}

Object *pushPair(VM *vm, Object *head, Object *tail)
{
    if (!vm || !head || !tail)
        return NULL;

    pushRoot(vm, head); // Protect the head object from GC
    pushRoot(vm, tail); // Protect the tail object from GC

    Object *obj = newObject(vm, OBJ_PAIR);

    popRoot(vm); // Remove the head object from the root stack after creating the pair
    popRoot(vm); // Remove the tail object from the root stack after creating the pair

    if (!obj)
        return NULL;

    obj->head = head; // Set the head reference
    obj->tail = tail; // Set the tail reference

    pushRoot(vm, obj); // Automatically protect the object from GC by pushing it to roots

    return obj;
}

/* mark all the reachable objects */
static void mark(VM *vm)
{
    if (!vm || vm->roots.count == 0)
        return;

    DynamicArray temp;
    array_init(&temp);

    for (size_t i = 0; i < vm->roots.count; i++)
    {
        Object *root = (Object *)array_get(&vm->roots, i);
        if (root)
            array_push(&temp, root); // Add the root to the temporary array for marking
    }

    while (temp.count > 0)
    {
        Object *obj = (Object *)array_pop(&temp); // Get the next object to mark
        if (!obj || IS_MARKED(obj->next))
            continue;

        obj->next = SET_MARK(obj->next); // Mark the object as reachable

        if (obj->type == OBJ_PAIR)
        {
            if (obj->head)
                array_push(&temp, obj->head); // Add head to the temporary array for marking
            if (obj->tail)
                array_push(&temp, obj->tail); // Add tail to the temporary array for marking
        }
    }

    array_free(&temp); // Free the temporary array used for marking
}

/**
 * @brief Computes the forwarding addresses for all marked (reachable) objects in the heap.
 *
 * @details This function scans the heap and calculates the new addresses for all marked objects.
 * It updates the `next` pointer of each marked object to point to its new location in the compacted heap.
 * else, it leaves the `next` pointer unchanged for unmarked (unreachable) objects.
 */
static void compute_forwarding_addresses(VM *vm)
{
    if (!vm || vm->num_objects == 0)
        return;

    uint8_t *scan = vm->heap_start;    // Start of the new free space in the heap
    uint8_t *new_loc = vm->heap_start; // Pointer to the next free byte in the heap

    while (scan < vm->next_free)
    {
        Object *obj = (Object *)scan; // cast the scan pointer to an Object pointer

        if (IS_MARKED(obj->next)) // If the object is marked, it is reachable and should be moved
        {
            obj->next = (Object *)((uintptr_t)new_loc | MARK_BIT); // Set the forwarding address for the marked object
            new_loc += sizeof(Object);                             // Move the new location pointer forward by the size of the
        }

        scan += sizeof(Object); // Move the scan pointer forward by the size of the object
    }
}

/**
 * @brief Updates all explicit pointers (head/tail) in the heap to point to the new addresses of reachable objects.
 *
 * @details This function scans the heap and updates the `next`, `head`, and `tail` pointers of all marked objects
 * to point to their new locations in the compacted heap. It ensures that all references are consistent after the compaction process.
 * Unmarked (unreachable) objects are ignored, as they will be removed during the compaction phase.
 */
static void update_pointers(VM *vm)
{
    if (!vm || vm->num_objects == 0 || vm->roots.count == 0)
        return;

    for (size_t i = 0; i < vm->roots.count; i++)
    {
        Object *root = (Object *)array_get(&vm->roots, i);
        if (root && IS_MARKED(root->next))
            array_set(&vm->roots, i, UNTAGGED_PTR(root->next));
    }

    uint8_t *scan = vm->heap_start; // Start scanning the heap from the beginning

    while (scan < vm->next_free)
    {
        Object *obj = (Object *)scan;

        if (IS_MARKED(obj->next)) // If the object is marked, it is reachable and its pointers need to be updated
        {
            Object *real_dest = UNTAGGED_PTR(obj->next); // Get the real destination of the marked object

            if (real_dest->type == OBJ_PAIR) // If the object is a pair, update its head and tail pointers to point to the new addresses of reachable objects
            {
                if (real_dest->head && IS_MARKED(real_dest->head->next)) // Update head pointer if it is marked
                    real_dest->head = UNTAGGED_PTR(real_dest->head->next);

                if (real_dest->tail && IS_MARKED(real_dest->tail->next))
                    real_dest->tail = UNTAGGED_PTR(real_dest->tail->next);
            }
        }
        scan += sizeof(Object); // Move the scan pointer forward by the size of the object
    }
}

/**
 * @brief Compacts the heap by moving all marked (reachable) objects to the beginning of the heap memory arena.
 *
 * @details This function scans the heap and moves all marked objects to the front of the heap,
 * eliminating any gaps left by unmarked (unreachable) objects.
 * It updates the `next_free` pointer to point to the end of the compacted heap,
 * effectively reclaiming memory from unreachable objects and reducing fragmentation.
 * Unmarked objects are ignored and will be left behind in the heap,
 * as they are considered garbage and will be collected in future allocations.
 *
 */
static void compact_heap(VM *vm)
{
    if (!vm)
        return;

    size_t alive_objects = 0;              // Count of alive objects after compaction
    uint8_t *scan = vm->heap_start;        // Pointer to scan through the heap memory
    uint8_t *compact_ptr = vm->heap_start; // Pointer to the next free byte in the compacted heap

    while (scan < vm->next_free)
    {
        Object *obj = (Object *)scan;

        if (IS_MARKED(obj->next))
        {
            Object *dest = (Object *)compact_ptr; // Get the destination pointer for the compacted object

            memmove(dest, obj, sizeof(Object)); // Move the marked object to the compacted location in the heap

            dest->next = NULL;             // Clear the next pointer of the compacted object to indicate it is no longer marked
            compact_ptr += sizeof(Object); // Move the compact pointer forward by the size of the object
            alive_objects++;               // Increment the count of alive objects after compaction
        }

        scan += sizeof(Object); // Move the scan pointer forward by the size of the object
    }

    vm->next_free = compact_ptr;     // Update the next free pointer to the end of the compacted heap
    vm->num_objects = alive_objects; // Update the total number of allocated objects to the number of alive objects after compaction
}

void gc(VM *vm)
{
    if (!vm || vm->num_objects == 0)
        return;

    mark(vm);
    compute_forwarding_addresses(vm);
    update_pointers(vm);
    compact_heap(vm);
}