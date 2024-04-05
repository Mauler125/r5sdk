/*H********************************************************************************/
/*!
    \File cryptgcm.h

    \Description
        An implementation of the GCM-128 and GCM-256 cipher, based on the NIST
        standard, intended for use in implementation of TLS1.1 GCM cipher suites.

        This implementation uses Shoup's method utilizing 4-bit tables as described
        in the GCM specifications.  While not particularly fast, table generation
        is quick and memory usage required small.  This implementation is restricted
        to a feature set suitable for implementation of TLS1.1 GCM cipher suites.

    \Copyright
        Copyright (c) 2014 Electronic Arts

    \Version 07/01/2014 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _cryptgcm_h
#define _cryptgcm_h

/*!
\Moduledef CryptGcm CryptGcm
\Modulemember Crypt
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/crypt/cryptaes.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

// opaque module state
typedef struct CryptGcmT CryptGcmT;

//! all fields are PRIVATE
struct CryptGcmT
{
    uint64_t HL[16];        //!< precalculated htable, low
    uint64_t HH[16];        //!< precalculated htable, high
    uint64_t uLen;          //!< data length
    uint64_t uAddLen;       //!< additional data length
    uint8_t  aBaseEctr[16]; //!< first ECTR for tag
    uint8_t  aY[16];        //!< y scratch buffer
    uint8_t  aBuf[16];      //!< I/O scratch buffer
    uint32_t uMode;         //!< CRYPTAES_KETYPE_ENCRYPT or CRYPTAES_KEYTYPE_DECRYPT
    CryptAesKeyScheduleT KeySchedule; //!< AES key schedule
};

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// initialize state for GCM encryption/decryption module
DIRTYCODE_API void CryptGcmInit(CryptGcmT *pGcm, const uint8_t *pKeyBuf, int32_t iKeyLen);

// encrypt data with the GCM cipher
DIRTYCODE_API int32_t CryptGcmEncrypt(CryptGcmT *pGcm, uint8_t *pBuffer, int32_t iLength, const uint8_t *pInitVec, int32_t iIvLen, const uint8_t *pAddData, int32_t iAddLen, uint8_t *pTag, int32_t iTagLen);

// decrypt data with the GCM cipher
DIRTYCODE_API int32_t CryptGcmDecrypt(CryptGcmT *pGcm, uint8_t *pBuffer, int32_t iLength, const uint8_t *pInitVec, int32_t iIvLen, const uint8_t *pAddData, int32_t iAddLen, uint8_t *pTag, int32_t iTagLen);

#ifdef __cplusplus
}
#endif

//@}

#endif // _cryptgcm_h
