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
 *  @file algs.h
 *  @author Raphael Beck
 *  @brief JWT algorithms as defined in https://tools.ietf.org/html/rfc7518#section-3.1
 */

#ifndef L8W8JWT_ALGS_H
#define L8W8JWT_ALGS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * HMAC-SHA256 signing algorithm.
 */
#define L8W8JWT_ALG_HS256 0

/**
 * HMAC-SHA384 signing algorithm.
 */
#define L8W8JWT_ALG_HS384 1

/**
 * HMAC-SHA512 signing algorithm.
 */
#define L8W8JWT_ALG_HS512 2

/**
 * RSASSA-PKCS1-v1_5-SHA256 signing algorithm.
 */
#define L8W8JWT_ALG_RS256 3

/**
 * RSASSA-PKCS1-v1_5-SHA384 signing algorithm.
 */
#define L8W8JWT_ALG_RS384 4

/**
 * RSASSA-PKCS1-v1_5-SHA512 signing algorithm.
 */
#define L8W8JWT_ALG_RS512 5

/**
 * RSASSA-PSS MGF1 SHA-256 signing algorithm.
 */
#define L8W8JWT_ALG_PS256 6

/**
 * RSASSA-PSS MGF1 SHA-384 signing algorithm.
 */
#define L8W8JWT_ALG_PS384 7

/**
 * RSASSA-PSS MGF1 SHA-512 signing algorithm.
 */
#define L8W8JWT_ALG_PS512 8

/**
 * ECDSA + P-256 + SHA256 signing algorithm.
 */
#define L8W8JWT_ALG_ES256 9

/**
 * ECDSA + P-384 + SHA384 signing algorithm.
 */
#define L8W8JWT_ALG_ES384 10

/**
 * ECDSA + P-521 + SHA512 signing algorithm.
 */
#define L8W8JWT_ALG_ES512 11

/**
 * ECDSA over secp256k1 + SHA256 signing algorithm.
 */
#define L8W8JWT_ALG_ES256K 12

/**
 * EdDSA over ed25519 + SHA512 signing algorithm.
 */
#define L8W8JWT_ALG_ED25519 13

#ifndef L8W8JWT_ENABLE_EDDSA
/**
 * Set this to \c 1 if you want to enable the EdDSA signing algorithm
 */
#define L8W8JWT_ENABLE_EDDSA 0
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // L8W8JWT_ALGS_H
