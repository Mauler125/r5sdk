/*H********************************************************************************/
/*!
    \File murmurhash3.h

    \Description
        An implementation of MurmurHash3, based heavily on the x64 128bit output
        implementation.  MurmurHash3 is a public domain hashing algorithm written
        by Austin Appleby, ref http://code.google.com/p/smhasher/wiki/MurmurHash3.

        MurmurHash3 is not cryptographically secure, however it has excellent
        collision resistance and is orders of magnitude faster than the fastest
        secure cryptographic hash.

    \Copyright
        Copyright (c) 2014 Electronic Arts

    \Version 03/07/2014 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _murmurhash3_h
#define _murmurhash3_h

/*!
\Moduledef MurmurHash3 MurmurHash3
\Modulemember Util
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

#define MURMURHASH_HASHSIZE (16)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct MurmurHash3T
{
    uint64_t aState[2];     //!< hash state
    int32_t  iCount;        //!< total byte count
    uint8_t  aData[16];     //!< partial data block
} MurmurHash3T;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// init the MurmurHash3 context
DIRTYCODE_API void MurmurHash3Init(MurmurHash3T *pContext);

// init the MurmurHash3 context, with hash size
DIRTYCODE_API void MurmurHash3Init2(MurmurHash3T *pContext, int32_t iHashSize);

// add data to the MurmurHash3 context (hash the data)
DIRTYCODE_API void MurmurHash3Update(MurmurHash3T *pContext, const void *pBuffer, int32_t iLength);

// convert MurmurHash3 state into final output form
DIRTYCODE_API void MurmurHash3Final(MurmurHash3T *pContext, void *pBuffer, int32_t iLength);

// hash the data, all-in-one version
DIRTYCODE_API void MurmurHash3(void *_pOutput, int32_t iOutLen, const void *_pInput, int32_t iInpLen, const void *pKey, int32_t iKeyLen);

#ifdef __cplusplus
}
#endif

//@}

#endif // _murmurhash3_h

