/*H********************************************************************************/
/*!
    \File cryptchacha.h

    \Description

    \Copyright
        Copyright (c) 2018 Electronic Arts

    \Version 02/12/2018 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _cryptchacha_h
#define _cryptchacha_h

/*!
\Moduledef CryptChaCha CryptChaCha
\Modulemember Crypt
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

// opaque module state
typedef struct CryptChaChaT CryptChaChaT;

//! all fields are PRIVATE
struct CryptChaChaT
{
    uint32_t aInput[16];
};

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// initialize state for ChaCha encryption/decryption module
DIRTYCODE_API void CryptChaChaInit(CryptChaChaT *pChaCha, const uint8_t *pKeyBuf, int32_t iKeyLen);

// encrypt data with the ChaCha cipher
DIRTYCODE_API int32_t CryptChaChaEncrypt(CryptChaChaT *pChaCha, uint8_t *pBuffer, int32_t iLength, const uint8_t *pInitVec, int32_t iIvLen, const uint8_t *pAddData, int32_t iAddLen, uint8_t *pTag, int32_t iTagLen);

// decrypt data with the ChaCha cipher
DIRTYCODE_API int32_t CryptChaChaDecrypt(CryptChaChaT *pChaCha, uint8_t *pBuffer, int32_t iLength, const uint8_t *pInitVec, int32_t iIvLen, const uint8_t *pAddData, int32_t iAddLen, const uint8_t *pTag, int32_t iTagLen);

#ifdef __cplusplus
}
#endif

//@}

#endif // _cryptgcm_h
