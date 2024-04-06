/*H********************************************************************************/
/*!
    \File aws.h

    \Description
        Implements AWS utility functions, including SigV4 signing and signed binary
        event reading and writing.

    \Copyright
        Copyright 2018 Electronic Arts

    \Version 12/26/2018 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _aws_h
#define _aws_h

/*!
\Moduledef AWS AWS
\Modulemember Util
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! AWS signing info
typedef struct AWSSignInfoT
{
    char strRegion[32];     //!< region request is being made in
    char strService[32];    //!< name of service for request
    char strKeyPath[64];    //!< keypath for request
    char strSignature[65];  //!< latest signature, hex encoded
    char strKey[64];        //!< secret key used to sign request
} AWSSignInfoT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// sign given header with AWS Signature Version 4 signing process
DIRTYCODE_API int32_t AWSSignSigV4(char *pHeader, int32_t iHeaderSize, const char *pRequest, const char *pKeyInfo, const char *pService, AWSSignInfoT *pSignInfo);

// write a signed binary event 
DIRTYCODE_API int32_t AWSWriteEvent(uint8_t *pBuffer, int32_t iBufSize, const uint8_t *pData, int32_t *pDataSize, const char *pEvent, AWSSignInfoT *pSignInfo);

// read a signed binary event
DIRTYCODE_API int32_t AWSReadEvent(const uint8_t *pBuffer, int32_t iBufLen, char *pEventType, int32_t iEventSize, char *pMessage, int32_t *pMessageSize);

#ifdef __cplusplus
}
#endif

//@}

#endif // _aws_h


