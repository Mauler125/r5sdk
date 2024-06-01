/*H*******************************************************************/
/*!
    \File crypthash.h

    \Description
        This module implements a generic wrapper for all cryptograph
        hash modules.

    \Copyright
        Copyright (c) Electronic Arts 2013

    \Version 1.0 11/05/2013 (jbrookes) First Version
*/
/*******************************************************************H*/

#ifndef _crypthash_h
#define _crypthash_h

/*!
\Moduledef CryptHash CryptHash
\Modulemember Crypt
*/
//@{

/*** Include files ***************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/crypt/cryptmd5.h"
#include "DirtySDK/crypt/cryptsha1.h"
#include "DirtySDK/crypt/cryptsha2.h"
#include "DirtySDK/util/murmurhash3.h"

/*** Defines *********************************************************/

typedef enum CryptHashTypeE
{
    CRYPTHASH_NULL,
    CRYPTHASH_MURMUR3,      //!< murmerhash3; NOT cryto grade but VERY fast, use with caution
    CRYPTHASH_MD5,          //!< MD5; semi-obsolete, use with caution
    CRYPTHASH_SHA1,         //!< SHA1
    CRYPTHASH_SHA224,       //!< SHA2-224
    CRYPTHASH_SHA256,       //!< SHA2-256
    CRYPTHASH_SHA384,       //!< SHA2-384
    CRYPTHASH_SHA512,       //!< SHA2-512
    CRYPTHASH_NUMHASHES
} CryptHashTypeE;

#define CRYPTHASH_MAXDIGEST (CRYPTSHA2_HASHSIZE_MAX)    //!< SHA2 has the largest digest size
#define CRYPTHASH_MAXSTATE  (sizeof(CryptSha2T))        //!< SHA2 has the largest state

/*** Macros **********************************************************/

/*** Type Definitions ************************************************/

//! crypthash init func
typedef void (CryptHashInitT)(void *pState, int32_t iHashSize);

//! crypthash update func
typedef void (CryptHashUpdateT)(void *pState, const uint8_t *pInput, uint32_t uInputLength);

//! crypthash final func
typedef void (CryptHashFinalT)(void *pState, void *pBuffer, uint32_t uLength);

//! crypthash function block
typedef struct CryptHashT
{
    CryptHashInitT *Init;
    CryptHashUpdateT *Update;
    CryptHashFinalT *Final;
    CryptHashTypeE eHashType;
    int32_t iHashSize;
} CryptHashT;

/*** Variables *******************************************************/

/*** Functions *******************************************************/

#ifdef __cplusplus
extern "C" {
#endif

//!< get reference to hash function
DIRTYCODE_API const CryptHashT *CryptHashGet(CryptHashTypeE eHashType);

//!< get hash size of specified hash function
DIRTYCODE_API int32_t CryptHashGetSize(CryptHashTypeE eHashType);

//!< get hash type based on size of hash (excludes murmurhash3)
DIRTYCODE_API CryptHashTypeE CryptHashGetBySize(int32_t iHashSize);

#ifdef __cplusplus
}
#endif

//@}

#endif // _crypthash_h

