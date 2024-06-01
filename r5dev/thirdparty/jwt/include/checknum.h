/*
   Copyright 2020 Raphael Beck

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
 *  @file checknum.h
 *  @author Raphael Beck
 *  @brief Check whether a given string contains an integer or floating point number.
 */

/* https://github.com/GlitchedPolygons/checknum */

#ifndef CHECKNUM_H
#define CHECKNUM_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CHECKNUM_STATIC
    #define CHECKNUM_API static
#else
    #define CHECKNUM_API extern
#endif

#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

/**
 * Checks whether a given string contains a valid integer or floating point number. <p>
 * If it's an integer, 1 is returned. <P>
 * If it's a float or a double, 2 is returned. <p>
 * If the string doesn't contain a valid number at all, 0 is returned.
 */
CHECKNUM_API int checknum(char* string, size_t string_length)
{
    if (string == NULL)
        return 0;

    if (string_length == 0)
        string_length = strlen(string);

    char* c = string;

    while (*c == ' ' && c < string + string_length)
        c++;

    while (*(string + string_length - 1) == ' ' && c < string + string_length)
        string_length--;

    switch (*c)
    {
        case '+':
        case '-':
            if (++c >= string + string_length)
                return 0;
        default:
            break;
    }

    unsigned int type = 0;

    if (*c == '0')
    {
        type |= 1 << 0;
        if (*++c != '.' && c < string + string_length)
            return 0;
    }

    for (; c < string + string_length; c++)
    {
        switch (*c)
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                type |= 1 << 0;
                continue;
            case '-':
                if (type & 1 << 1 || (*(c - 1) != 'E' && *(c - 1) != 'e'))
                    return 0;
                type |= 1 << 1;
                continue;
            case '.':
                if (type & 1 << 2 || type & 1 << 3)
                    return 0;
                type |= 1 << 2;
                continue;
            case 'E':
            case 'e':
                if (!(type & 1 << 0) || type & 1 << 3 || c + 1 >= string + string_length)
                    return 0;
                type |= 1 << 3;
                continue;
            case '+':
                if (type & 1 << 4 || (*(c - 1) != 'E' && *(c - 1) != 'e'))
                    return 0;
                type |= 1 << 4;
                continue;
            default:
                return 0;
        }
    }

    switch (type)
    {
        case 0:
            return 0;
        case 1 << 0:
            return 1;
        default:
            return type & 1 << 0 ? 2 : 0;
    }
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CHECKNUM_H
