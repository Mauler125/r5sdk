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
 *  @file claim.h
 *  @author Raphael Beck
 *  @brief JWT claims as described in https://auth0.com/docs/tokens/concepts/jwt-claims
 */

#ifndef L8W8JWT_CLAIM_H
#define L8W8JWT_CLAIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "version.h"
#include <stdlib.h>

// Forward declare chillbuff
/** @private */
struct chillbuff;

/**
 * JWT claim value is a string (e.g. <code>"iss": "glitchedpolygons.com"</code>).
 */
#define L8W8JWT_CLAIM_TYPE_STRING 0

/**
 * JWT claim value is an integer (e.g. <code>"exp": 1579610629</code>)
 */
#define L8W8JWT_CLAIM_TYPE_INTEGER 1

/**
 * JWT claim value type number (e.g. <code>"size": 1.85</code>).
 */
#define L8W8JWT_CLAIM_TYPE_NUMBER 2

/**
 * JWT claim value is a boolean (e.g. <code>"done": true</code>).
 */
#define L8W8JWT_CLAIM_TYPE_BOOLEAN 3

/**
 * JWT claim value is null (e.g. <code>"ref": null</code>).
 */
#define L8W8JWT_CLAIM_TYPE_NULL 4

/**
 * JWT claim value type JSON array (e.g. <code>"ids": [2, 4, 8, 16]</code>).
 */
#define L8W8JWT_CLAIM_TYPE_ARRAY 5

/**
 * JWT claim value type is a JSON object (e.g. <code>"objs": { "name": "GMan", "id": 420 }</code>).
 */
#define L8W8JWT_CLAIM_TYPE_OBJECT 6

/**
 * JWT claim value is some other type.
 */
#define L8W8JWT_CLAIM_TYPE_OTHER 7

/**
 * Struct containing a jwt claim key-value pair.<p>
 * If allocated on the heap by the decode function,
 * remember to call <code>l8w8jwt_claims_free()</code> on it once you're done using it.
 */
struct l8w8jwt_claim
{
    /**
     * The token claim key (e.g. "iss", "iat", "sub", etc...). <p>
     * NUL-terminated C-string!
     */
    char* key;

    /**
     * key string length. <p>
     * Set this to <code>0</code> if you want to make the encoder use <code>strlen(key)</code> instead.
     */
    size_t key_length;

    /**
     * The claim's value as a NUL-terminated C-string.
     */
    char* value;

    /**
     * value string length. <p>
     * Set this to <code>0</code> if you want to make the encoder use <code>strlen(value)</code> instead.
     */
    size_t value_length;

    /**
     * The type of the claim's value. <p>
     * 0 = string, 1 = integer, 2 = number, 3 = boolean, 4 = null, 5 = array, 6 = object, 7 = other.
     * @see https://www.w3schools.com/js/js_json_datatypes.asp
     */
    int type;
};

/**
 * Frees a heap-allocated <code>l8w8jwt_claim</code> array.
 * @param claims The claims to free.
 * @param claims_count The size of the passed claims array.
 */
L8W8JWT_API void l8w8jwt_free_claims(struct l8w8jwt_claim* claims, size_t claims_count);

/**
 * Writes a bunch of JWT claims into a chillbuff stringbuilder. <p>
 * Curly braces and trailing commas won't be written; only the "key":"value" pairs!
 * @param stringbuilder The buffer into which to write the claims.
 * @param claims The l8w8jwt_claim array of claims to write.
 * @param claims_count The claims array size.
 * @return Return code as specified inside retcodes.h
 */
L8W8JWT_API int l8w8jwt_write_claims(struct chillbuff* stringbuilder, struct l8w8jwt_claim* claims, size_t claims_count);

/**
 * Gets a claim by key from a l8w8jwt_claim array.
 * @param claims The array to look in.
 * @param claims_count The claims array size.
 * @param key The claim key (e.g. "sub") to look for.
 * @param key_length The claim key's string length.
 * @return The found claim; <code>NULL</code> if no such claim was found in the array.
 */
L8W8JWT_API struct l8w8jwt_claim* l8w8jwt_get_claim(struct l8w8jwt_claim* claims, size_t claims_count, const char* key, size_t key_length);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // L8W8JWT_CLAIM_H
