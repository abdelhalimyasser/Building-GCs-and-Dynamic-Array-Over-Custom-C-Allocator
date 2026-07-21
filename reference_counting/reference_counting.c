#include "reference_counting.h"
#include "../allocator/allocator.h"

static Object *newObject(ObjectType type)
{
    Object *obj = (Object *)mem_malloc(sizeof(Object)); // Allocate memory for the new object
    if (!obj)                                           // Check if memory allocation was successful
        return NULL;                                    // Return NULL if allocation failed

    obj->type = type; // Set the type of the object

    if (type == OBJ_PAIR) // If the object is a pair, initialize its head and tail to NULL
    {
        obj->head = NULL;
        obj->tail = NULL;
    }

    obj->ref_count = 1; // Initialize the reference count to 1

    return obj; // Return the newly created object
}

Object *pushInt(int value)
{
    Object *obj = newObject(OBJ_INT);
    if (!obj)
        return NULL;

    obj->int_value = value;

    return obj;
}

Object *pushChar(char value)
{
    Object *obj = newObject(OBJ_CHAR);
    if (!obj)
        return NULL;

    obj->char_value = value;

    return obj;
}

Object *pushPair(Object *head, Object *tail)
{
    Object *obj = newObject(OBJ_PAIR);
    if (!obj)
        return NULL;

    rc_retain(head);
    rc_retain(tail);

    obj->head = head;
    obj->tail = tail;

    return obj;
}

void rc_retain(Object *obj)
{
    if (!obj)
        return;

    obj->ref_count++;
}

void rc_release(Object *obj)
{
    if (!obj)
        return;

    obj->ref_count--;

    if (obj->ref_count == 0)
    {
        if (obj->type == OBJ_PAIR)
        {
            rc_release(obj->head);
            rc_release(obj->tail);
        }

        mem_free(obj);
    }
}