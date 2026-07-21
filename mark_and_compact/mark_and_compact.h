#ifndef MARK_AND_COMPACT_H_INCLUDED
#define MARK_AND_COMPACT_H_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "../dynamic_array/dynamic_array.h"

#define MARK_BIT 1UL

#define IS_MARKED(p)    (((uintptr_t)(p) & MARK_BIT) != 0)
#define SET_MARK(p)     ((Object *)((uintptr_t)(p) | MARK_BIT))
#define UNTAGGED_PTR(p) ((Object *)((uintptr_t)(p) & ~MARK_BIT))

#define THRESHOLD (1 * 1024)
#define DEFAULT_HEAP_CAPACITY (1 * 1024)

typedef enum
{
    OBJ_INT,
    OBJ_CHAR,
    OBJ_PAIR
} ObjectType;

typedef struct Object
{
    struct Object *next; // Linked list pointer (also heavily used for Pointer Tagging GC bits)
    ObjectType type;     // To identify what data this object holds

    // The payload: what actual data is stored here
    union
    {
        int int_value;   // OBJ_INT
        char char_value; // OBJ_CHAR
        struct
        { // OBJ_PAIR
            struct Object *head;
            struct Object *tail;
        };
    };
} Object;

typedef struct VM
{
    DynamicArray roots;   // Dynamic array acting as a scalable root stack
    size_t num_objects;   // Total number of allocated objects
    size_t heap_capacity; // Total capacity of the heap in bytes
    uint8_t *heap_start;  // Pointer to the start of the heap memory arena
    uint8_t *next_free;   // Pointer to the next free byte in the heap
} VM;

/**
 * @brief Initializes a new Virtual Machine `VM` instance.
 *
 * @details Allocates memory for the VM state, initializes the dynamic root array,
 * and resets all internal counters and pointers to their default states.
 *
 * **Complexity:**
 * - Time Complexity: O(1)
 * - Space Complexity: O(1) (allocates a fixed initial structure).
 *
 * @return VM* A pointer to the newly created VM, or NULL if memory allocation fails.
 * @note The returned VM must be cleaned up using freeVM() to avoid memory leaks.
 */
VM *newVM(void);

/**
 * @brief Destroys the VM and frees all associated memory.
 *
 * @details Triggers a final garbage collection to safely clear all remaining objects,
 * frees the dynamic root array, and finally frees the VM structure itself.
 *
 * **Complexity:**
 * - Time Complexity: O(N) where N is the number of remaining allocated objects.
 * - Space Complexity: O(1) auxiliary space.
 *
 * @param vm Pointer to the Virtual Machine state.
 * @return void
 * @note This should be called at the very end of the program to ensure total cleanup.
 */
void freeVM(VM *vm);

/**
 * @brief Pushes an object onto the VM's root stack.
 *
 * @details Protects the object from being swept by the Garbage Collector
 * by safely appending it to the dynamic roots array.
 *
 * **Complexity:**
 * - Time Complexity: Amortized O(1).
 * - Space Complexity: O(1) (Amortized).
 *
 * @param vm Pointer to the Virtual Machine state.
 * @param obj Pointer to the Object to be protected.
 * @return void
 */
void pushRoot(VM *vm, Object *obj);

/**
 * @brief Pops the most recently added object from the VM's root stack.
 *
 * @details Removes the object from the roots array, making it eligible for
 * garbage collection during the next cycle (unless it is still referenced elsewhere).
 *
 * **Complexity:**
 * - Time Complexity: O(1)
 * - Space Complexity: O(1)
 *
 * @param vm Pointer to the Virtual Machine state.
 * @return Object* Pointer to the popped Object, or NULL if the root stack is empty.
 */
Object *popRoot(VM *vm);

/**
 * @brief Creates a new Integer object and pushes it to the roots.
 *
 * **Complexity:**
 * - Time Complexity: Amortized O(1)
 * - Space Complexity: O(1)
 *
 * @param vm Pointer to the Virtual Machine state.
 * @param intValue The integer value to store in the object.
 * @return Object* Pointer to the newly allocated Integer object.
 * @note The object is automatically protected from GC upon creation.
 */
Object *pushInt(VM *vm, int intValue);

/**
 * @brief Creates a new Character object and pushes it to the roots.
 *
 * **Complexity:**
 * - Time Complexity: Amortized O(1)
 * - Space Complexity: O(1)
 *
 * @param vm Pointer to the Virtual Machine state.
 * @param charValue The character value to store in the object.
 * @return Object* Pointer to the newly allocated Character object.
 * @note The object is automatically protected from GC upon creation.
 */
Object *pushChar(VM *vm, char charValue);

/**
 * @brief Creates a new Pair object (graph node) and pushes it to the roots.
 *
 * @details A Pair object holds references to two other objects (head and tail),
 * allowing the creation of complex linked structures like lists or AST nodes.
 *
 * **Complexity:**
 * - Time Complexity: Amortized O(1)
 * - Space Complexity: O(1)
 *
 * @param vm Pointer to the Virtual Machine state.
 * @param head Pointer to the first linked Object.
 * @param tail Pointer to the second linked Object.
 * @return Object* Pointer to the newly allocated Pair object.
 * @note The object is automatically protected from GC upon creation.
 */
Object *pushPair(VM *vm, Object *head, Object *tail);

/**
 * @brief Executes the Mark-Compact Garbage Collection algorithm.
 *
 * @details Traverses all objects starting from the roots to mark reachable objects.
 * It then computes forwarding addresses, updates all explicit pointers (head/tail),
 * and finally compacts the heap by moving live objects to the beginning of the memory arena,
 * eliminating memory fragmentation.
 *
 * **Complexity:**
 * - Time Complexity: O(R + N), where R is roots and N is total live objects (multiple passes).
 * - Space Complexity: O(1) auxiliary space (using pointer tagging and in-place compaction).
 *
 * @param vm Pointer to the Virtual Machine state.
 * @return void
 * @note This process pauses the application execution (Stop-The-World phase).
 */
void gc(VM *vm);

#endif // MARK_AND_COMPACT_H_INCLUDED