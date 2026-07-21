#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allocator/allocator.h"
#include "dynamic_array/dynamic_array.h"

// ============================================================================
// Imports
// ============================================================================
#if defined(TEST_REFERENCE_COUNTING)
    #include "reference_counting/reference_counting.h"
#elif defined(TEST_MARK_COMPACT)
    #include "mark_and_compact/mark_and_compact.h"
#elif defined(TEST_MARK_SWEEP)
    #include "mark_and_sweep/mark_and_sweep.h"
#else
    #error "Select one test suite in the Makefile"
#endif

// ============================================================================
// Array Test (Runs every time)
// ============================================================================
void test_dynamic_array()
{
    printf("--- 1. Testing Dynamic Array ---\n");
    DynamicArray arr;
    if (!array_init(&arr))
    {
        printf("Dynamic array init failed.\n\n");
        return;
    }

    int *first = mem_malloc(sizeof(int));
    int *second = mem_malloc(sizeof(int));
    int *third = mem_malloc(sizeof(int));

    if (!first || !second || !third)
    {
        printf("Allocator failed while preparing dynamic array test.\n");
        // Clean up whatever was successfully allocated
        if (first)
            mem_free(first);
        if (second)
            mem_free(second);
        if (third)
            mem_free(third);
        array_free(&arr);
        printf("\n");
        return;
    }

    *first = 10;
    *second = 20;
    *third = 30;

    array_push(&arr, first);
    array_push(&arr, second);
    array_enqueue(&arr, third);

    printf("Array count after pushes: %zu\n", arr.count);
    printf("Element at index 1: %d\n", *(int *)array_get(&arr, 1));

    int *dequeued = array_dequeue(&arr);
    int *popped = array_pop(&arr);
    printf("Dequeued value: %d\n", dequeued ? *dequeued : -1);
    printf("Popped value: %d\n", popped ? *popped : -1);

    mem_free(dequeued);
    mem_free(popped);
    mem_free((int *)array_get(&arr, 0));

    array_free(&arr);
    printf("Dynamic Array freed successfully.\n\n");
}

// ============================================================================
// GC Tests (Only one runs based on Makefile flag)
// ============================================================================

#ifdef TEST_REFERENCE_COUNTING
void test_rc_gc()
{
    printf("--- 2. Testing Reference Counting GC ---\n");

    Object *val1 = pushInt(42);
    Object *val2 = pushChar('C');
    printf("Created Int Object (val1) and Char Object (val2).\n");

    Object *pair = pushPair(val1, val2);
    printf("Created Pair Object linking val1 and val2.\n");

    rc_release(pair);
    printf("Pair released (cascading release triggered).\n");

    rc_release(val1);
    rc_release(val2);
    printf("Reference Counting Test Completed successfully.\n\n");
}
#endif

#ifdef TEST_MARK_SWEEP
void test_ms_gc()
{
    printf("--- 3. Testing Mark-Sweep GC ---\n");

    VM *vm = newVM();
    if (!vm)
    {
        printf("GC VM initialization failed.\n\n");
        return;
    }

    Object *left = pushInt(vm, 1);
    Object *right = pushChar(vm, 'Z');
    Object *pair = pushPair(vm, left, right);
    Object *garbage = pushInt(vm, 999);

    printf("Before GC: roots=%zu\n", vm->roots.count);
    popRoot(vm);
    printf("Before GC after dropping garbage root: roots=%zu\n", vm->roots.count);

    gc(vm);

    printf("After GC: num_objects=%zu\n", vm->num_objects);
    printf("Live pair head=%d tail=%c\n", pair ? pair->head->int_value : -1, pair ? pair->tail->char_value : '?');

    (void)garbage;
    freeVM(vm);
    printf("GC Test Completed successfully.\n\n");
}
#endif

#ifdef TEST_MARK_COMPACT
void test_mc_gc()
{
    printf("--- 3. Testing Mark-Compact GC ---\n");

    VM *vm = newVM();
    if (!vm)
    {
        printf("GC VM initialization failed.\n\n");
        return;
    }

    Object *left = pushInt(vm, 1);
    Object *right = pushChar(vm, 'Z');
    Object *pair = pushPair(vm, left, right);
    Object *garbage = pushInt(vm, 999);

    printf("Before GC: roots=%zu\n", vm->roots.count);
    popRoot(vm);
    printf("Before GC after dropping garbage root: roots=%zu\n", vm->roots.count);

    gc(vm);

    printf("After GC: num_objects=%zu\n", vm->num_objects);
    printf("Live pair head=%d tail=%c\n", pair ? pair->head->int_value : -1, pair ? pair->tail->char_value : '?');

    (void)garbage;
    freeVM(vm);
    printf("GC Test Completed successfully.\n\n");
}
#endif

// ============================================================================
// Main Execution
// ============================================================================
int main()
{
    printf("Starting Memory Management Test Suite...\n");
    printf("==========================================\n");

    mem_init();

    test_dynamic_array();

#ifdef TEST_REFERENCE_COUNTING
    test_rc_gc();
#elif defined(TEST_MARK_COMPACT)
    test_mc_gc();
#elif defined(TEST_MARK_SWEEP)
    test_ms_gc();
#endif

    printf("==========================================\n");
    printf("All active tests finished gracefully.\n");

    return 0;
}