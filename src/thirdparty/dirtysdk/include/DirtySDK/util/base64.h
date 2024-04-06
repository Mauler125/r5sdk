/*H*******************************************************************/
/*!
    \File base64.h

    \Description
        This module Base-64 encoding/decoding as defined in RFC
        989, 1040, 1113, 1421, 1521 and 2045.

    \Copyright
        Copyright (c) Electronic Arts 2003. ALL RIGHTS RESERVED.

    \Version 1.0 12/11/2003 (SJB) First Version
*/
/*******************************************************************H*/

#ifndef _base64_h
#define _base64_h

/*!
\Moduledef Base64 Base64
\Modulemember Util
*/
//@{

/*** Include files ***************************************************/

#include "DirtySDK/platform.h"

/*** Defines *********************************************************/

/*** Macros **********************************************************/

/*! The number of bytes it takes to Base-64 encode the given number
    of bytes.  The result includes any required padding but not a '\0'
    terminator. */
#define Base64EncodedSize(x) ((((x)+2)/3)*4)

/*! The maximum number of bytes it takes to hold the decoded version
    of a Base-64 encoded string that is 'x' bytes long.

    In this version of Base-64, 'x' is always a multiple of 4 and the
    result is always a multiple of 3 (i.e. there may be up to 2 padding
    bytes at the end of the decoded value).  It is assumed that the
    exact length of the string (minus any padding) is either encoded in
    the string or is external to the string. */
#define Base64DecodedSize(x) (((x)/4)*3)

/*** Type Definitions ************************************************/

/*** Variables *******************************************************/

/*** Functions *******************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// Base64 encode a string
DIRTYCODE_API void Base64Encode(int32_t iInputLen, const char *pInput, char *pOutput);

// Base64 encode a string, buffer-safe version
DIRTYCODE_API int32_t Base64Encode2(const char *pInput, int32_t iInputLen, char *pOutput, int32_t iOutputLen);

// decode a Base64-encoded string
DIRTYCODE_API int32_t Base64Decode(int32_t iInputLen, const char *pInput, char *pOutput);

// decode a Base64-encoded string, return decoded size
DIRTYCODE_API int32_t Base64Decode2(int32_t iInputLen, const char *pInput, char *pOutput);

// decode a Base64-encoded string, return decoded size, buffer-safe version
DIRTYCODE_API int32_t Base64Decode3(const char *pInput, int32_t iInputLen, char *pOutput, int32_t iOutputLen);

// Base64 encode a url string
DIRTYCODE_API int32_t Base64EncodeUrl(const char *pInput, int32_t iInputLen, char *pOutput, int32_t iOutputLen);

// Base64 encode a url string with more options
DIRTYCODE_API int32_t Base64EncodeUrl2(const char *pInput, int32_t iInputLen, char *pOutput, int32_t iOutputLen, uint8_t bPadded);

// decode a Base64-encode url string
DIRTYCODE_API int32_t Base64DecodeUrl(const char *pInput, int32_t iInputLen, char *pOutput, int32_t iOutputLen);

#ifdef __cplusplus
}
#endif

//@}

#endif
