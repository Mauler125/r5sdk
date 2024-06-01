/*H********************************************************************************/
/*!
    \File crypthmac.c

    \Description
        Implements HMAC as defined in https://tools.ietf.org/html/rfc2104

    \Copyright
        Copyright (c) 2013 Electronic Arts Inc.

    \Version 01/14/2013 (jbrookes) First Version; refactored from ProtoSSL
    \Version 07/26/2018 (jbrookes) Fixed to properly handle keys larger than block size
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/crypt/crypthash.h"

#include "DirtySDK/crypt/crypthmac.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CryptHmacCalc

    \Description
        Implements HMAC as defined in https://tools.ietf.org/html/rfc2104

    \Input *pBuffer     - [out] storage for generated digest
    \Input iBufLen      - size of output buffer
    \Input *pMessage    - input data to hash
    \Input iMessageLen  - size of input data
    \Input *pKey        - seed
    \Input iKeyLen      - seed length
    \Input eHashType    - hash type

    \Output
        int32_t         - negative=error, else success

    \Version 01/14/2013 (jbrookes)
*/
/********************************************************************************F*/
int32_t CryptHmacCalc(uint8_t *pBuffer, int32_t iBufLen, const uint8_t *pMessage, int32_t iMessageLen, const uint8_t *pKey, int32_t iKeyLen, CryptHashTypeE eHashType)
{
    CryptHmacMsgT Message;
    Message.pMessage = pMessage;
    Message.iMessageLen = iMessageLen;
    return(CryptHmacCalcMulti(pBuffer, iBufLen, &Message, 1, pKey, iKeyLen, eHashType));
}

/*F********************************************************************************/
/*!
    \Function CryptHmacCalcMulti

    \Description
        Implements HMAC as defined in https://tools.ietf.org/html/rfc2104.
        This version allows multiple buffers to be specified in one call, which is
        useful when the input to be hashed is not in a single contiguous buffer.

    \Input *pBuffer     - [out] storage for generated MAC
    \Input iBufLen      - size of output buffer (may be smaller than hash size for truncated HMAC)
    \Input *pMessageList- list of messages to hash
    \Input iNumMessages - number of messages to hash
    \Input *pKey        - key
    \Input iKeyLen      - key length
    \Input eHashType    - hash type

    \Output
        int32_t         - negative=error, else success

    \Version 01/14/2013 (jbrookes)
*/
/********************************************************************************F*/
int32_t CryptHmacCalcMulti(uint8_t *pBuffer, int32_t iBufLen, const CryptHmacMsgT *pMessageList, int32_t iNumMessages, const uint8_t *pKey, int32_t iKeyLen, CryptHashTypeE eHashType)
{
    uint8_t aKiPad[128], aKoPad[128], aKeyHash[128]; // max message block size of SHA-384/512
    int32_t iBufCnt, iMessage, iBlockSize;
    const CryptHashT *pHash;
    uint8_t aWorkBuf[CRYPTHASH_MAXDIGEST];
    uint8_t aContext[CRYPTHASH_MAXSTATE];

    // get hash function block for this hash type
    if ((pHash = CryptHashGet(eHashType)) == NULL)
    {
        NetPrintf(("crypthmac: invalid hash type %d\n", eHashType));
        return(-1);
    }
    // get block size
    iBlockSize = (pHash->iHashSize < CRYPTSHA384_HASHSIZE) ? 64 : 128;
    // as per https://tools.ietf.org/html/rfc2104#section-3, keys longer than the block size are hashed to the digest size
    if (iKeyLen > iBlockSize)
    {
        pHash->Init(aContext, pHash->iHashSize);
        pHash->Update(aContext, pKey, iKeyLen);
        pHash->Final(aContext, aKeyHash, pHash->iHashSize);
        pKey = aKeyHash;
        iKeyLen = pHash->iHashSize;
    }
    // limit output to hash size
    if (iBufLen > pHash->iHashSize)
    {
        iBufLen = pHash->iHashSize;
    }

    // copy key to ipad and opad, pad with zero
    ds_memcpy(aKiPad, pKey, iKeyLen);
    ds_memclr(aKiPad+iKeyLen, iBlockSize-iKeyLen);
    ds_memcpy(aKoPad, pKey, iKeyLen);
    ds_memclr(aKoPad+iKeyLen, iBlockSize-iKeyLen);

    // calculate xor of ipad and opad with 0x36/0x5c respectively
    for (iBufCnt = 0; iBufCnt < iBlockSize; iBufCnt += 1)
    {
        aKiPad[iBufCnt] ^= 0x36;
        aKoPad[iBufCnt] ^= 0x5c;
    }

    // execute hash of ipad
    pHash->Init(aContext, pHash->iHashSize);
    pHash->Update(aContext, aKiPad, iBlockSize);
    for (iMessage = 0; iMessage < iNumMessages; iMessage += 1)
    {
        pHash->Update(aContext, pMessageList[iMessage].pMessage, pMessageList[iMessage].iMessageLen);
    }
    pHash->Final(aContext, aWorkBuf, pHash->iHashSize);

    // execute hash of ipad+opad
    pHash->Init(aContext, pHash->iHashSize);
    pHash->Update(aContext, aKoPad, iBlockSize);
    pHash->Update(aContext, aWorkBuf, pHash->iHashSize);
    pHash->Final(aContext, pBuffer, iBufLen);

    // success
    return(0);
}
