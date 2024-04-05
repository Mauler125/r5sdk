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

#ifdef __cplusplus
extern "C" {
#endif

#include "include/claim.h"
#include "include/version.h"
#include "include/retcodes.h"
#include "include/chillbuff.h"

#include <string.h>
#include <mbedtls/md.h>

void l8w8jwt_free_claims(struct l8w8jwt_claim* claims, const size_t claims_count)
{
    if (claims != NULL && claims_count > 0)
    {
        for (struct l8w8jwt_claim* claim = claims; claim < claims + claims_count; ++claim)
        {
            if (claim == NULL)
                continue;

            l8w8jwt_zero(claim->key, claim->key_length);
            l8w8jwt_zero(claim->value, claim->value_length);

            l8w8jwt_free(claim->key);
            l8w8jwt_free(claim->value);
        }

        l8w8jwt_zero(claims, claims_count * sizeof(struct l8w8jwt_claim));
        l8w8jwt_free(claims);
    }
}

static inline void l8w8jwt_escape_claim_string(struct chillbuff* stringbuilder, const char* string, const size_t string_length)
{
    for (size_t i = 0; i < string_length; ++i)
    {
        const char c = string[i];

        switch (c)
        {
            case '\\':
                chillbuff_push_back(stringbuilder, "\\\\", 2);
                break;
            case '\"':
                chillbuff_push_back(stringbuilder, "\\\"", 2);
                break;
            default:
                chillbuff_push_back(stringbuilder, &c, 1);
                break;
        }
    }
}

int l8w8jwt_write_claims(struct chillbuff* stringbuilder, struct l8w8jwt_claim* claims, const size_t claims_count)
{
    if (stringbuilder == NULL || claims == NULL)
    {
        return L8W8JWT_NULL_ARG;
    }

    if (claims_count == 0)
    {
        return L8W8JWT_INVALID_ARG;
    }

    struct chillbuff escape_buffer;
    if (chillbuff_init(&escape_buffer, 256, sizeof(char), CHILLBUFF_GROW_LINEAR) != 0)
    {
        return L8W8JWT_OUT_OF_MEM;
    }

    int first = 1;
    for (struct l8w8jwt_claim* claim = claims; claim < claims + claims_count; ++claim)
    {
        if (claim->key == NULL)
        {
            continue;
        }

        if (!first)
        {
            chillbuff_push_back(stringbuilder, ",", 1);
        }

        const size_t key_length = claim->key_length ? claim->key_length : strlen(claim->key);
        const size_t value_length = claim->value_length ? claim->value_length : strlen(claim->value);

        chillbuff_clear(&escape_buffer);
        l8w8jwt_escape_claim_string(&escape_buffer, claim->key, key_length);

        chillbuff_push_back(stringbuilder, "\"", 1);
        chillbuff_push_back(stringbuilder, escape_buffer.array, escape_buffer.length);
        chillbuff_push_back(stringbuilder, "\":", 2);

        if (claim->type == L8W8JWT_CLAIM_TYPE_STRING)
            chillbuff_push_back(stringbuilder, "\"", 1);

        chillbuff_clear(&escape_buffer);
        l8w8jwt_escape_claim_string(&escape_buffer, claim->value, value_length);

        chillbuff_push_back(stringbuilder, escape_buffer.array,escape_buffer.length);

        if (claim->type == L8W8JWT_CLAIM_TYPE_STRING)
            chillbuff_push_back(stringbuilder, "\"", 1);

        first = 0;
    }

    chillbuff_free(&escape_buffer);
    return L8W8JWT_SUCCESS;
}

struct l8w8jwt_claim* l8w8jwt_get_claim(struct l8w8jwt_claim* claims, const size_t claims_count, const char* key, const size_t key_length)
{
    if (claims == NULL || key == NULL || claims_count == 0 || key_length == 0)
        return NULL;

    for (struct l8w8jwt_claim* claim = claims; claim < claims + claims_count; ++claim)
    {
        if (strncmp(claim->key, key, key_length) == 0)
            return claim;
    }

    return NULL;
}

#ifdef __cplusplus
} // extern "C"
#endif
