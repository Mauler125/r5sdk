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

#include "include/version.h"
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void l8w8jwt_free(void* mem)
{
    free(mem);
}

void l8w8jwt_zero(void* buf, size_t len)
{
    if (len > 0)
    {
#if defined(MBEDTLS_PLATFORM_HAS_EXPLICIT_BZERO)
        explicit_bzero(buf, len);
#if defined(HAVE_MEMORY_SANITIZER)
        /* You'd think that Msan would recognize explicit_bzero() as
         * equivalent to bzero(), but it actually doesn't on several
         * platforms, including Linux (Ubuntu 20.04).
         * https://github.com/google/sanitizers/issues/1507
         * https://github.com/openssh/openssh-portable/commit/74433a19bb6f4cef607680fa4d1d7d81ca3826aa
         */
        __msan_unpoison(buf, len);
#endif
#elif defined(__STDC_LIB_EXT1__)
        memset_s(buf, len, 0, len);
#elif defined(_WIN32)
        RtlSecureZeroMemory(buf, len);
#else
        memset_func(buf, 0, len);
#endif
    }
}

int l8w8jwt_get_version_number()
{
    return (int)L8W8JWT_VERSION;
}

void l8w8jwt_get_version_string(char out[32])
{
    const char version_string[] = L8W8JWT_VERSION_STR;
    const size_t version_string_length = sizeof(version_string) - 1;

    memcpy(out, version_string, version_string_length);
    out[version_string_length] = '\0';
}

#ifdef __cplusplus
} // extern "C"
#endif