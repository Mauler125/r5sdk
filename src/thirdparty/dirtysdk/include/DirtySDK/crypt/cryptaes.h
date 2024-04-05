/*H********************************************************************************/
/*!
    \File cryptaes.h

    \Description
        An implementation of the AES-128 and AES-256 cipher, based on the FIPS
        standard, intended for use with the TLS AES cipher suites.

        This implementation deliberately uses the naming conventions from the
        standard where possible in order to aid comprehension.

    \Notes
        References:
            FIPS197 standard: http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf

    \Copyright
        Copyright (c) 2011 Electronic Arts

    \Version 01/20/2011 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _cryptaes_h
#define _cryptaes_h

/*!
\Moduledef CryptAes CryptAes
\Modulemember Crypt
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

#define CRYPTAES_MAXROUNDS          (14)

#define CRYPTAES_KEYTYPE_ENCRYPT    (0)
#define CRYPTAES_KEYTYPE_DECRYPT    (1)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct CryptAesKeyScheduleT
{
    uint16_t uNumRounds;
    uint16_t uKeyWords;
    uint32_t aKeySchedule[(CRYPTAES_MAXROUNDS+1)*8];
} CryptAesKeyScheduleT;

// opaque module state
typedef struct CryptAesT CryptAesT;

//! all fields are PRIVATE
struct CryptAesT
{
    CryptAesKeyScheduleT KeySchedule;
    uint8_t  aInitVec[16];                  //!< initialization vector/CBC state
};

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// initialize state for AES encryption/decryption module
DIRTYCODE_API void CryptAesInit(CryptAesT *pAes, const uint8_t *pKeyBuf, int32_t iKeyLen, uint32_t uKeyType, const uint8_t *pInitVec);

// encrypt data with the AES cipher in CBC mode
DIRTYCODE_API void CryptAesEncrypt(CryptAesT *pAes, uint8_t *pBuffer, int32_t iLength);

// decrypt data with the AES cipher in CBC mode
DIRTYCODE_API void CryptAesDecrypt(CryptAesT *pAes, uint8_t *pBuffer, int32_t iLength);

// encrypt a 16 byte data block with AES cipher
DIRTYCODE_API void CryptAesEncryptBlock(CryptAesKeyScheduleT *pKeySchedule, const uint8_t *pInput, uint8_t *pOutput);

// decrypt a 16 byte data block with AES cipher
DIRTYCODE_API void CryptAesDecryptBlock(CryptAesKeyScheduleT *pKeySchedule, const uint8_t *pInput, uint8_t *pOutput);

// initialize AES key schedule
DIRTYCODE_API void CryptAesInitKeySchedule(CryptAesKeyScheduleT *pKeySchedule, const uint8_t *pKeyBuf, int32_t iKeyLen, uint32_t uKeyType);


#ifdef __cplusplus
}
#endif

//@}

#endif // _cryptaes_h
