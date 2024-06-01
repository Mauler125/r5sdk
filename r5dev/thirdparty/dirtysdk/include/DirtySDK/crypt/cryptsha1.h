/*H*******************************************************************/
/*!
    \File cryptsha1.h

    \Description
        This module implements SHA1 as defined in RFC 3174.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Notes
        The implementation is based on the algorithm description in sections
        3 through 6 of RFC 3174 and not on the C code in section 7.

        The code deliberately uses some of the naming conventions from
        the RFC to in order to aid comprehension.

        This implementation is limited to hashing no more than 2^32-9 bytes.
        It will silently produce the wrong result if an attempt is made to
        hash more data.

    \Version 1.0 06/06/2004 (sbevan) First Version
*/
/*******************************************************************H*/

#ifndef _cryptsha1_h
#define _cryptsha1_h

/*!
\Moduledef CryptSha1 CryptSha1
\Modulemember Crypt
*/
//@{

/*** Include files ***************************************************/

#include "DirtySDK/platform.h"

/*** Defines *********************************************************/

#define CRYPTSHA1_HASHSIZE (160/8)  //!< length of SHA1 hash

/*** Macros **********************************************************/

/*** Type Definitions ************************************************/

typedef struct CryptSha1T CryptSha1T;

//! all fields are PRIVATE
struct CryptSha1T
{
    uint32_t uCount;         //!< total length of hash data in bytes
    uint32_t uPartialCount;  //!< # bytes in the strData
    uint32_t H[5];           //!< message digest
    uint8_t  strData[16*4];  //!< partial data block
};

/*** Variables *******************************************************/

/*** Functions *******************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// init the SHA1 context.
DIRTYCODE_API void CryptSha1Init(CryptSha1T *pSha1);

// init the SHA1 context (alternate form)
DIRTYCODE_API void CryptSha1Init2(CryptSha1T *pSha1, int32_t iHashSize);

// add data to the SHA1 context (hash the data).
DIRTYCODE_API void CryptSha1Update(CryptSha1T *pSha1, const uint8_t *pInput, uint32_t uInputLength);

// convert SHA1 state into final output form
DIRTYCODE_API void CryptSha1Final(CryptSha1T *pSha1, void *pBuffer, uint32_t uLength);

#ifdef __cplusplus
}
#endif

//@}

#endif
