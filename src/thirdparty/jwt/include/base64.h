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
 *  @file base64.h
 *  @author Raphael Beck
 *  @brief Base-64 encode and decode strings/bytes. <p>
 *  @warning The caller is responsible for freeing the returned buffers! <p>
 *  Pass <code>true</code> as first parameter if you want to use base64url encoding instead of base64.
 *  @see https://en.wikipedia.org/wiki/Base64#URL_applications
 */

#ifndef L8W8JWT_BASE64_H
#define L8W8JWT_BASE64_H

#ifdef __cplusplus
extern "C" {
#endif

#include "version.h"
#include <stdint.h>
#include <string.h>

/**
 *  Encodes a byte array to a base-64 string. <p>
 *  If you're encoding a string, don't include the NUL terminator
 *  (pass <code>strlen(data)</code> instead of the array's size to the <code>data_length</code> parameter). <p>
 *
 *  @note The output buffer is NUL-terminated to make it easier to use as a C string.
 *  @note The NUL terminator is NOT included in the <code>out_length</code>.
 *  @note DO NOT FORGET to free the output buffer once you're done using it!
 *
 *  @param url base64url encode instead of base64? Set to \c 0 for \c false; anything else for \c true.
 *  @param data The data (array of bytes) to base-64 encode.
 *  @param data_length The length of the input data array (in case of a C string: array size - 1 in order to omit the NUL terminator).
 *  @param out Output where the base-64 encoded string should be written into (will be malloc'ed, so make sure to <code>free()</code> this as soon as you're done using it!).
 *  @param out_length Pointer to a <code>size_t</code> variable containing the length of the output buffer minus the NUL terminator.
 *
 *  @return Return code as defined in retcodes.h
 */
L8W8JWT_API int l8w8jwt_base64_encode(int url, const uint8_t* data, size_t data_length, char** out, size_t* out_length);

/**
 *  Decodes a base-64 encoded string to an array of bytes. <p>
 *
 *  @note The returned bytes buffer is NUL-terminated to allow usage as a C string.
 *  @note The NUL terminator is NOT included in the <code>out_length</code>.
 *  @note DO NOT FORGET to free the output buffer once you're done using it!
 *
 *  @param url Decode using base64url instead of base64? Set to \c 0 for \c false; anything else for \c true.
 *  @param data The base-64 encoded string to decode (obtained via {@link #l8w8jwt_base64_encode}).
 *  @param data_length The length of the string to decode.
 *  @param out Output where the decoded bytes should be written into (will be malloc'ed, so make sure to <code>free()</code> this as soon as you're done using it!).
 *  @param out_length Pointer to a <code>size_t</code> variable into which to write the output buffer's length.
 *
 *  @return Return code as defined in retcodes.h
 */
L8W8JWT_API int l8w8jwt_base64_decode(int url, const char* data, size_t data_length, uint8_t** out, size_t* out_length);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // L8W8JWT_BASE64_H
