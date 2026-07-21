#include "mark_and_sweep.h"
#include "../allocator/allocator.h"

VM *newVM(void)
{
    VM *vm = (VM *)mem_malloc(sizeof(VM));

    if (!vm)
        return NULL; // Handle memory allocation failure

    // Initialize the VM structure
    vm->first_object = NULL;
    vm->num_objects = 0;
    array_init(&vm->roots);

    return vm;
}

void freeVM(VM *vm)
{
    if (!vm)
        return;

    Object *current = vm->first_object; // Start from the first allocated object in the VM's object list
    while (current)                     // Loop through all allocated objects
    {
        Object *next = UNTAGGED_PTR(current->next);
        mem_free(current);
        current = next;
    }

    array_free(&vm->roots); // Free the dynamic array of roots
    mem_free(vm);           // Free the VM structure itself
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
        return NULL; // Handle memory allocation failure or max objects reached

    if (vm->num_objects >= THRESHOLD)
        gc(vm); // Trigger garbage collection if the number of objects exceeds the threshold

    Object *obj = (Object *)mem_malloc(sizeof(Object));

    if (!obj)
        return NULL; // Handle memory allocation failure

    obj->type = type;
    obj->next = vm->first_object; // Link the new object to the list of allocated objects

    vm->first_object = obj; // Update the first object pointer
    vm->num_objects++;      // Increment the total number of allocated objects

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
    if (!head || !tail)
        return NULL;

    if (!vm)
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

static void markAll(VM *vm)
{
    if (!vm || vm->roots.count == 0)
        return;

    DynamicArray temp;
    array_init(&temp);

    for (size_t i = 0; i < vm->roots.count; i++)
        array_push(&temp, array_get(&vm->roots, i));

    while (temp.count > 0)
    {
        Object *obj = (Object *)array_pop(&temp);

        if (!obj || IS_MARKED(obj->next)) // Skip if the object is NULL or already marked
            continue;

        obj->next = SET_MARK(obj->next); // Mark the object as reachable

        if (obj->type == OBJ_PAIR)
        {
            array_push(&temp, obj->head); // Add head
            array_push(&temp, obj->tail); // Add tail
        }
    }

    array_free(&temp); // Free the temporary dynamic array used for marking
}

static void sweep(VM *vm)
{
    if (!vm)
        return;

    Object **obj = &vm->first_object;

    while (*obj)
    {
        if (!IS_MARKED((*obj)->next))
        {
            // Object is unreachable, free it
            Object *unreached = *obj;
            *obj = unreached->next; // Remove from the list
            mem_free(unreached);    // Free the memory
            vm->num_objects--;      // Decrement the total number of allocated objects
        }
        else
        {
            // Object is reachable, unmark it for the next GC cycle
            (*obj)->next = UNTAGGED_PTR((*obj)->next);
            obj = &(*obj)->next; // Move to the next object in the list
        }
    }
}

void gc(VM *vm)
{
    if (!vm)
        return;

    markAll(vm); // Mark all reachable objects starting from the roots
    sweep(vm);   // Sweep and free all unreachable objects
}