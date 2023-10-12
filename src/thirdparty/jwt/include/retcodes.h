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
 *  @file retcodes.h
 *  @author Raphael Beck
 *  @brief Macros for possible integer codes returned by the various l8w8jwt functions.
 */

#ifndef L8W8JWT_RETCODES_H
#define L8W8JWT_RETCODES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returned from a l8w8jwt function when everything went smooth 'n' chill. Time to get Schwifty, Morteyy!
 */
#define L8W8JWT_SUCCESS 0

/**
 * Error code returned by a l8w8jwt function if you passed a NULL argument that shouldn't have been NULL.
 */
#define L8W8JWT_NULL_ARG 100

/**
 * This error code is returned by a l8w8jwt function if you passed an invalid parameter into it.
 */
#define L8W8JWT_INVALID_ARG 200

/**
 * This is returned if some allocation inside a l8w8jwt function failed: you're out of memory at this point.
 */
#define L8W8JWT_OUT_OF_MEM 300

/**
 * Not good...
 */
#define L8W8JWT_OVERFLOW 310

/**
 * Returned if signing a JWT failed.
 */
#define L8W8JWT_SIGNATURE_CREATION_FAILURE 400

/**
 * If one of the SHA-2 functions fails (e.g. SHA-256).
 */
#define L8W8JWT_SHA2_FAILURE 410

/**
 * Returned if some PEM-formatted key string couldn't be parsed.
 */
#define L8W8JWT_KEY_PARSE_FAILURE 420

/**
 * Base64(URL) encoding or decoding error.
 */
#define L8W8JWT_BASE64_FAILURE 425

/**
 * Returned if you passed the wrong private or public key type (e.g. trying to use an RSA key for ECDSA tokens, etc...). <p>
 * Especially for the ECDSA algorithms like ES256, ES384 and ES512 double-check that you passed keys of the correct curve! <p>
 * Only use the P-256 curve for ES256, P-384 (a.k.a. secp384r1) for ES384 and P-521 (a.k.a. secp521r1) for ES512.
 */
#define L8W8JWT_WRONG_KEY_TYPE 450

/**
 * When the <code>mbedtls_ctr_drbg_seed()</code> function fails...
 */
#define L8W8JWT_MBEDTLS_CTR_DRBG_SEED_FAILURE 500

/**
 * Returned if the token is invalid (format-wise).
 */
#define L8W8JWT_DECODE_FAILED_INVALID_TOKEN_FORMAT 600

/**
 * Returned if the token is invalid because it's missing the signature (despite having specified an alg that isn't "none").
 */
#define L8W8JWT_DECODE_FAILED_MISSING_SIGNATURE 700

/**
 * Returned if the JWT signing alg parameter that was passed is not supported (e.g. the used l8w8jwt library was built without support for that algo, e.g. Ed25519).
 * See the README.md for more details!
 */
#define L8W8JWT_UNSUPPORTED_ALG 800

#ifdef __cplusplus
} // extern "C"
#endif

#endif // L8W8JWT_RETCODES_H
