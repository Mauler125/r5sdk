/*H*******************************************************************/
/*!
    \File cryptsha2.h

    \Description
        This module implements SHA2 as defined in RFC 6234, which is
        itself based on FIPS 180-2.

    \Notes
        This implementation is limited to hashing no more than 2^32-9
        bytes.  It will silently produce the wrong result if an attempt
        is made to hash more data.

    \Copyright
        Copyright (c) Electronic Arts 2013

    \Version 1.0 11/04/2013 (jbrookes) First Version
*/
/*******************************************************************H*/

#ifndef _cryptsha2_h
#define _cryptsha2_h

/*!
\Moduledef CryptSha2 CryptSha2
\Modulemember Crypt
*/
//@{

/*** Include files ***************************************************/

#include "DirtySDK/platform.h"

/*** Defines *********************************************************/

//! maximum hash result
#define CRYPTSHA224_HASHSIZE    (28)
#define CRYPTSHA256_HASHSIZE    (32)
#define CRYPTSHA384_HASHSIZE    (48)
#define CRYPTSHA512_HASHSIZE    (64)
#define CRYPTSHA2_HASHSIZE_MAX  (CRYPTSHA512_HASHSIZE)

/*** Macros **********************************************************/

/*** Type Definitions ************************************************/

//! private SHA2 state
typedef struct CryptSha2T
{
    uint32_t uCount;            //!< total length of hash data in bytes
    uint8_t uHashSize;          //!< hash size
    uint8_t uBlockSize;         //!< 64/128 depending on mode
    uint8_t uPartialCount;      //!< # bytes in the partial data block
    uint8_t _pad;
    union
    {
        uint32_t H_32[8];
        uint64_t H_64[8];
    }TempHash;                  //!< temporary hash state
    uint8_t strData[128];       //!< partial data block
} CryptSha2T;

/*** Variables *******************************************************/

/*** Functions *******************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// init the SHA2 context.
DIRTYCODE_API void CryptSha2Init(CryptSha2T *pSha2, int32_t iHashSize);

// add data to the SHA2 context (hash the data).
DIRTYCODE_API void CryptSha2Update(CryptSha2T *pSha2, const uint8_t *pInput, uint32_t uInputLength);

// convert SHA2 state into final output form
DIRTYCODE_API void CryptSha2Final(CryptSha2T *pSha2, uint8_t *pBuffer, uint32_t uLength);

#ifdef __cplusplus
}
#endif

//@}

#endif // _cryptsha2_h

