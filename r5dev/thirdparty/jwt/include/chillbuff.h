/*
   Copyright 2019 Raphael Beck

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

/**
 *  @file chillbuff.h
 *  @author Raphael Beck
 *  @date 27. December 2019
 *  @brief Array. Dynamic size. Push back 'n' chill. Buffer stuff. Dynamic stuff that's buff. Dynamically reallocating buff.. Yeah!
 *  @see https://github.com/GlitchedPolygons/chillbuff
 */

#ifndef CHILLBUFF_H
#define CHILLBUFF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* The following are the chillbuff exit codes returned from the various chillbuff functions. */

/**
 * Returned from a chillbuff function when everything went smooth 'n' chill. Time to get Schwifty!
 */
#define CHILLBUFF_SUCCESS 0

/**
 * Chill time is over, you're out of memory... Time to reconsider memory usage.
 */
#define CHILLBUFF_OUT_OF_MEM 100

/**
 * Error code returned by a chillbuff function if you passed a NULL argument that shouldn't have been NULL.
 */
#define CHILLBUFF_NULL_ARG 200

/**
 * This error code is returned by a chillbuff function if you passed an invalid parameter into it.
 */
#define CHILLBUFF_INVALID_ARG 300

/**
 * Not good...
 */
#define CHILLBUFF_OVERFLOW 400

/** @private */
static void (*_chillbuff_error_callback)(const char*) = NULL;

/**
 * How should the chillbuff's underlying array grow in size
 * once its maximum capacity is reached during a push_back?
 */
typedef enum chillbuff_growth_method
{
    /**
     * Double the capacity.
     */
    CHILLBUFF_GROW_DUPLICATIVE = 0,

    /**
     * Triple the capacity.
     */
    CHILLBUFF_GROW_TRIPLICATIVE = 1,

    /**
     * Grow by the same capacity every time the buffer is full.
     */
    CHILLBUFF_GROW_LINEAR = 2,

    /**
     * Multiplies the capacity by itself. Not the greatest idea... Use carefully!
     */
    CHILLBUFF_GROW_EXPONENTIAL = 3
} chillbuff_growth_method;

/**
 * Self-reallocating dynamic size array of no strictly defined type.
 * Easy 'n' "chill" (hope you like segmentation fault errors).
 */
typedef struct chillbuff
{
    /**
     * The buffer's underlying array that stores the data.
     */
    void* array;

    /**
     * The current amount of elements stored in the chillbuff. DO NOT touch this yourself, only read!
     */
    size_t length;

    /**
     * The current buffer capacity. This grows dynamically according to the specified {@link #chillbuff_growth_method}.
     */
    size_t capacity;

    /**
     * The size of each stored element. DO NOT CHANGE THIS! Only read (if necessary)...
     */
    size_t element_size;

    /**
     * The way the buffer's capacity is increased when it's full.
     */
    chillbuff_growth_method growth_method;
} chillbuff;

/** @private */
static inline void _chillbuff_printerr(const char* error, const char* origin)
{
    const size_t error_length = 64 + strlen(error) + strlen(origin);
    char* error_msg = (char*)malloc(error_length * sizeof(char)); // cast malloc because of compat with C++ D:
    if (error_msg != NULL)
    {
        snprintf(error_msg, error_length, "\nCHILLBUFF ERROR: (%s) %s\n", origin, error);
        if (_chillbuff_error_callback != NULL)
        {
            _chillbuff_error_callback(error_msg);
        }
        free(error_msg);
    }
}

/**
 * Sets the chillbuff error callback. <p>
 * If errors occur, they'll be passed as a string into the provided callback function.
 * @param error_callback The function to call when errors occur.
 * @return Whether the callback was set up correctly or not (chillbuff exit code, see top of chillbuff.h file for more details).
 */
static inline int chillbuff_set_error_callback(void (*error_callback)(const char*))
{
    if (error_callback == NULL)
    {
        _chillbuff_printerr("The passed error callback is empty; Operation cancelled!", __func__);
        return CHILLBUFF_NULL_ARG;
    }

    _chillbuff_error_callback = error_callback;
    return CHILLBUFF_SUCCESS;
}

/**
 * Clears the chillbuff error callback (errors won't be printed anymore).
 */
static inline void chillbuff_unset_error_callback()
{
    _chillbuff_error_callback = NULL;
}

/**
 * Initializes a chillbuff instance and makes it ready to accept data.
 * @param buff The chillbuff instance to init (or rather, a pointer to it).
 * @param initial_capacity The initial capacity of the underlying array. If you pass <code>0</code> here, <code>16</code> is used by default.
 * @param element_size How big should every array element be? E.g. if you're storing <code>int</code> you should pass <code>sizeof(int)</code>.
 * @param growth_method How should the buffer grow once its maximum capacity is reached? @see chillbuff_growth_method
 * @return Chillbuff exit code as defined at the top of the chillbuff.h header file. <code>0</code> means success.
 */
static inline int chillbuff_init(chillbuff* buff, const size_t initial_capacity, const size_t element_size, const chillbuff_growth_method growth_method)
{
    if (buff == NULL)
    {
        _chillbuff_printerr("Tried to init a NULL chillbuff instance; wouldn't end well. Cancelled...", __func__);
        return CHILLBUFF_NULL_ARG;
    }

    if (element_size == 0)
    {
        _chillbuff_printerr("Storing elements of size \"0\" makes no sense...", __func__);
        return CHILLBUFF_INVALID_ARG;
    }

    if (growth_method < 0 || growth_method > 3)
    {
        _chillbuff_printerr("Invalid grow method! Please use the appropriate chillbuff_growth_method enum!", __func__);
        return CHILLBUFF_INVALID_ARG;
    }

    buff->length = 0;
    buff->element_size = element_size;
    buff->growth_method = growth_method;
    buff->capacity = initial_capacity == 0 ? 16 : initial_capacity;
    buff->array = calloc(buff->capacity, buff->element_size);

    if (buff->array == NULL)
    {
        _chillbuff_printerr("OUT OF MEMORY!", __func__);
        return CHILLBUFF_OUT_OF_MEM;
    }

    return CHILLBUFF_SUCCESS;
}

/**
 * Frees a chillbuff instance.
 * @param buff The chillbuff to deallocate. If this is <code>NULL</code>, nothing happens at all.
 */
static inline void chillbuff_free(chillbuff* buff)
{
    if (buff == NULL)
    {
        return;
    }

    memset(buff->array, '\0', buff->length);
    free(buff->array);
    buff->array = NULL;
    buff->length = buff->capacity = buff->element_size = 0;
}

/**
 * Clears a chillbuff's data. <p>
 * Deletes all of the underlying array's elements and resets the length to <code>0</code>. <p>
 * Leaves the array allocated at the current capacity.
 * @param buff The chillbuff to clear. If this is <code>NULL</code>, nothing happens at all.
 */
static inline void chillbuff_clear(chillbuff* buff)
{
    if (buff == NULL)
    {
        return;
    }

    memset(buff->array, '\0', buff->capacity);
    buff->length = 0;
}

/**
 * Appends one or more elements to the buffer.
 * If the buffer is full, it will be expanded automatically.
 * @param buff The buffer into which to insert the elements.
 * @param elements The array of elements to insert (pointer to the first element).
 * @param elements_count Amount of elements to add (for example: if your buffer stores the type <code>uint32_t</code>, you'd pass <code>sizeof(elements_to_add) / sizeof(uint32_t)</code> here). If you're only adding a single element, pass <code>1</code>.
 * @return Chillbuff exit code that describes the insertion's outcome.
 */
static int chillbuff_push_back(chillbuff* buff, const void* elements, const size_t elements_count)
{
    if (buff == NULL)
    {
        _chillbuff_printerr("Tried to append to a NULL chillbuff instance!", __func__);
        return CHILLBUFF_NULL_ARG;
    }

    if (elements == NULL)
    {
        _chillbuff_printerr("Tried to append NULL element(s) to a chillbuff instance!", __func__);
        return CHILLBUFF_NULL_ARG;
    }

    if (elements_count == 0)
    {
        _chillbuff_printerr("The passed \"elements_count\" argument is zero; nothing to append!", __func__);
        return CHILLBUFF_INVALID_ARG;
    }

    for (size_t i = 0; i < elements_count; i++)
    {
        if (buff->length == buff->capacity)
        {
            size_t new_capacity;

            switch (buff->growth_method)
            {
                default:
                    _chillbuff_printerr("Invalid grow method! Please use the appropriate chillbuff_growth_method enum!", __func__);
                    return CHILLBUFF_INVALID_ARG;
                case CHILLBUFF_GROW_DUPLICATIVE:
                    new_capacity = (buff->capacity * 2);
                    break;
                case CHILLBUFF_GROW_TRIPLICATIVE:
                    new_capacity = (buff->capacity * 3);
                    break;
                case CHILLBUFF_GROW_LINEAR:
                    new_capacity = (buff->capacity + buff->element_size);
                    break;
                case CHILLBUFF_GROW_EXPONENTIAL:
                    new_capacity = (buff->capacity * buff->capacity);
                    break;
            }

            if (new_capacity <= buff->capacity || new_capacity >= UINT64_MAX / buff->element_size)
            {
                _chillbuff_printerr("Couldn't push back due to buffer OVERFLOW!", __func__);
                return CHILLBUFF_OVERFLOW;
            }

            void* new_array = realloc(buff->array, new_capacity * buff->element_size);
            if (new_array == NULL)
            {
                _chillbuff_printerr("Couldn't resize chillbuff underlying array; OUT OF MEMORY!", __func__);
                return CHILLBUFF_OUT_OF_MEM;
            }

            memset((char*)new_array + (buff->element_size * buff->length), '\0', (new_capacity - buff->length) * buff->element_size);
            buff->array = new_array;
            buff->capacity = new_capacity;
        }

        memcpy((char*)buff->array + (buff->element_size * buff->length++), (char*)elements + (i * buff->element_size), buff->element_size);
    }

    return CHILLBUFF_SUCCESS;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CHILLBUFF_H
