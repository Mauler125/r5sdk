/*H********************************************************************************/
/*!
    \File cryptrandcommon.h

    \Description
        Common APIs used for internally in the CryptRand module

    \Copyright
        Copyright (c) 2019 Electronic Arts Inc.

    \Version 01/24/2019 (eesponda)
*/
/********************************************************************************H*/

#ifndef _cryptrandcommon_h
#define _cryptrandcommon_h

/*!
\Moduledef CryptRandCommon CryptRandCommon
\Modulemember Crypt
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/crypt/cryptchacha.h"

/*** Type Definitions *************************************************************/

//! common state used for random number generation
typedef struct CryptRandCommonT
{
    CryptChaChaT Cipher;    //!< cipher used to generate random numbers
    int32_t iBytesLeft;     //!< bytes remaining before key rotation
    int32_t (*GetEntropy)(uint8_t *pBuffer, int32_t iBufSize); //!< pointer to internal function to get entropy
} CryptRandCommonT;

/*** Functions ********************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

// initialize the common state
int32_t CryptRandCommonInit(CryptRandCommonT *pCommon);

// generate a psuedo-random number
void CryptRandCommonGet(CryptRandCommonT *pCommon, uint8_t *pBuffer, int32_t iBufSize);

#if defined(__cplusplus)
}
#endif

//@}

#endif // _cryptrandcommon_h
