#ifndef REFERENCE_COUNTING_H_INCLUDED
#define REFERENCE_COUNTING_H_INCLUDED

#include <stddef.h>

typedef enum
{ // Define an enumeration for object types
    OBJ_INT,
    OBJ_CHAR,
    OBJ_PAIR
} ObjectType;

typedef struct Object
{                    // Define a structure for an object
    ObjectType type; // The type of the object (int, char, or pair)

    union
    {
        int int_value;   // Integer value
        char char_value; // Character value
        struct
        { // Structure for a pair of objects
            struct Object *head;
            struct Object *tail;
        };
    };

    size_t ref_count; // Reference count for the object
} Object;

/**
 * @brief Creates a new Integer object and initializes its reference count.
 *
 * @details Allocates memory for a new object of type OBJ_INT and sets its initial
 * reference count to 1.
 *
 * **Complexity:**
 * - Time Complexity: O(1)
 * - Space Complexity: O(1)
 *
 * @param value The integer value to store in the object.
 * @return Object* A pointer to the newly allocated Integer object, or NULL if allocation fails.
 * @note The returned object must be managed using rc_retain() and rc_release().
 */
Object *pushInt(int value);

/**
 * @brief Creates a new Character object and initializes its reference count.
 *
 * @details Allocates memory for a new object of type OBJ_CHAR and sets its initial
 * reference count to 1.
 *
 * **Complexity:**
 * - Time Complexity: O(1)
 * - Space Complexity: O(1)
 *
 * @param value The character value to store in the object.
 * @return Object* A pointer to the newly allocated Character object, or NULL if allocation fails.
 * @note The returned object must be managed using rc_retain() and rc_release().
 */
Object *pushChar(char value);

/**
 * @brief Creates a new Pair object that links two existing objects.
 *
 * @details Allocates memory for a new OBJ_PAIR object. It automatically increments
 * the reference counts of both the head and tail objects to ensure they are kept
 * alive as long as this pair exists.
 *
 * **Complexity:**
 * - Time Complexity: O(1)
 * - Space Complexity: O(1)
 *
 * @param head Pointer to the first linked Object.
 * @param tail Pointer to the second linked Object.
 * @return Object* A pointer to the newly allocated Pair object, or NULL if allocation fails.
 */
Object *pushPair(Object *head, Object *tail);

/**
 * @brief Increments the reference count of an object.
 *
 * @details Safely increases the reference count by 1. This should be called whenever
 * a new reference to the object is created (e.g., assigning it to a new variable or structure).
 *
 * **Complexity:**
 * - Time Complexity: O(1)
 * - Space Complexity: O(1)
 *
 * @param obj A pointer to the object to retain.
 * @return void
 * @note If the pointer is NULL, this function safely does nothing.
 */
void rc_retain(Object *obj);

/**
 * @brief Decrements the reference count of an object.
 *
 * @details Decreases the reference count by 1. If the count reaches zero, the object's
 * memory is freed. For OBJ_PAIR types, it recursively calls rc_release() on its head
 * and tail before freeing itself to prevent memory leaks.
 *
 * **Complexity:**
 * - Time Complexity: O(1) for standalone objects. For trees/pairs, it is O(N) where N is the depth of the cascade.
 * - Space Complexity: O(1)
 *
 * @param obj A pointer to the object to release.
 * @return void
 * @note If the pointer is NULL, this function safely does nothing.
 */
void rc_release(Object *obj);

#endif // REFERENCE_COUNTING_H_INCLUDED