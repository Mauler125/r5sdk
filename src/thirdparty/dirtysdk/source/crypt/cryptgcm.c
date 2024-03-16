/*H********************************************************************************/
/*!
    \File cryptgcm.c

    \Description
        An implementation of the GCM-128 and GCM-256 cipher, based on the NIST
        standard, intended for use in implementation of TLS1.1 GCM cipher suites.

        This implementation uses Shoup's method utilizing 4-bit tables as described
        in the GCM specifications.  While not particularly fast, table generation
        is quick and memory usage required small.  This implementation is restricted
        to a feature set suitable for implementation of TLS1.1 GCM cipher suites.

    \Notes
        References:
            - GCM specifications: http://csrc.nist.gov/groups/ST/toolkit/BCM/documents/proposedmodes/gcm/gcm-spec.pdf
            - NIST recommendation: http://csrc.nist.gov/publications/nistpubs/800-38D/SP-800-38D.pdf
            - TLS1.1 RFC AEAD ciphers: http://tools.ietf.org/html/rfc5246#section-6.2.3.3
            - TLS1.1 AES-GCM cipher specifications: http://tools.ietf.org/html/rfc5288
            - AEAD AES-GCM definitions: http://tools.ietf.org/html/rfc5116

    \Copyright
        Copyright (c) 2014 Electronic Arts

    \Version 07/01/2014 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"

#include "DirtySDK/crypt/cryptgcm.h"

/*** Defines **********************************************************************/

#define CRYPTGCM_VERBOSE (DIRTYCODE_DEBUG && FALSE)

/*** Macros ***********************************************************************/

/*
    Read/write big-endian words
*/
#define CRYPTGCM_RDWORD(_ptr)       ((((uint32_t)(_ptr)[0]) << 24) | ((uint32_t)((_ptr)[1]) << 16) | (((uint32_t)(_ptr)[2]) << 8) | ((uint32_t)(_ptr)[3]))
#define CRYPTGCM_WRWORD(_x, _ptr)   ((_ptr)[3] = (uint8_t)(_x),\
                                     (_ptr)[2] = (uint8_t)((_x)>>8),\
                                     (_ptr)[1] = (uint8_t)((_x)>>16),\
                                     (_ptr)[0] = (uint8_t)((_x)>>24))

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

// shoup's 4bit multiplication table
static const uint64_t _CryptGCM_aLast4[16] =
{
    0x0000, 0x1c20, 0x3840, 0x2460,
    0x7080, 0x6ca0, 0x48c0, 0x54e0,
    0xe100, 0xfd20, 0xd940, 0xc560,
    0x9180, 0x8da0, 0xa9c0, 0xb5e0
};

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _CryptGcmGenTable

    \Description
        Precompute GCM multiplication table

    \Input *pGcm            - cipher state

    \Version 07/01/2014 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptGcmGenTable(CryptGcmT *pGcm)
{
    uint64_t uHi, uLo;
    uint64_t uVh, uVl;
    uint8_t hInit[16];
    int32_t i, j;

    // generate table source
    ds_memclr(hInit, sizeof(hInit));
    CryptAesEncryptBlock(&pGcm->KeySchedule, hInit, hInit);

    // convert table source to 64bit ints
    uHi = CRYPTGCM_RDWORD(hInit+0);
    uLo = CRYPTGCM_RDWORD(hInit+4);
    uVh = (uint64_t) uHi << 32 | uLo;

    uHi = CRYPTGCM_RDWORD(hInit+8);
    uLo = CRYPTGCM_RDWORD(hInit+12);
    uVl = (uint64_t) uHi << 32 | uLo;

    // init table
    pGcm->HL[8] = uVl;
    pGcm->HH[8] = uVh;
    pGcm->HH[0] = 0;
    pGcm->HL[0] = 0;

    for (i = 4; i > 0; i >>= 1)
    {
        uint32_t uT= (uVl & 1) * 0xe1000000U;
        uVl = (uVh << 63) | (uVl >> 1);
        uVh = (uVh >>  1) ^ ((uint64_t)uT << 32);

        pGcm->HL[i] = uVl;
        pGcm->HH[i] = uVh;
    }

    for (i = 2; i < 16; i <<= 1)
    {
        uint64_t *pHiL = pGcm->HL + i;
        uint64_t *pHiH = pGcm->HH + i;
        uVh = *pHiH;
        uVl = *pHiL;
        for (j = 1; j < i; j += 1)
        {
            pHiH[j] = uVh ^ pGcm->HH[j];
            pHiL[j] = uVl ^ pGcm->HL[j];
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _CryptGcmMultiply

    \Description
        GCM multiply of input block by precomputed table

    \Input *pGcm        - cipher state
    \Input *pOut        - [out] storage for result
    \Input *pInp        - input block to multiply

    \Notes
        pOut may overlap pInp

    \Version 07/01/2014 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptGcmMultiply(CryptGcmT *pGcm, uint8_t *pOut, const uint8_t *pInp)
{
    uint8_t uLo, uHi;   // lookups into HL/HH
    uint8_t uRem;       // lookup into static table
    uint64_t uZh, uZl;
    int32_t iData;

    uLo = pInp[15] & 0xf;
    uZh = pGcm->HH[uLo];
    uZl = pGcm->HL[uLo];

    for (iData = 15; iData >= 0; iData -= 1)
    {
        uLo = pInp[iData] & 0xf;
        uHi = pInp[iData] >> 4;

        if (iData != 15)
        {
            uRem = (uint8_t) uZl & 0xf;
            uZl = (uZh << 60) | (uZl >> 4);
            uZh = (uZh >>  4);
            uZh ^= (uint64_t) _CryptGCM_aLast4[uRem] << 48;
            uZh ^= pGcm->HH[uLo];
            uZl ^= pGcm->HL[uLo];

        }

        uRem = (uint8_t) uZl & 0xf;
        uZl = (uZh << 60) | (uZl >> 4);
        uZh = (uZh >>  4);
        uZh ^= (uint64_t) _CryptGCM_aLast4[uRem] << 48;
        uZh ^= pGcm->HH[uHi];
        uZl ^= pGcm->HL[uHi];
    }

    CRYPTGCM_WRWORD(uZh >> 32, pOut+0);
    CRYPTGCM_WRWORD(uZh >>  0, pOut+4);
    CRYPTGCM_WRWORD(uZl >> 32, pOut+8);
    CRYPTGCM_WRWORD(uZl >>  0, pOut+12);
}

/*F********************************************************************************/
/*!
    \Function _CryptGcmStart

    \Description
        Start of encryption/decryption processing

    \Input *pGcm        - cipher state
    \Input iMode        - CRYPTAES_KEYTYPE_ENCRYPT or CRYPTAES_KEYTYPE_DECRYPT
    \Input *pInitVec    - initialization vector (IV)
    \Input iIvLen       - length of IV
    \Input *pAddData    - additional authenticated data (AAD)
    \Input iAddLen      - length of AAD

    \Output
        int32_t         - zero=success, negative=failure

    \Version 07/01/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CryptGcmStart(CryptGcmT *pGcm, int32_t iMode, const uint8_t *pInitVec, int32_t iIvLen, const uint8_t *pAddData, int32_t iAddLen)
{
    int32_t iBlockLen, iData;
    const uint8_t *pAad;

    ds_memclr(pGcm->aY, sizeof(pGcm->aY));
    ds_memclr(pGcm->aBuf, sizeof(pGcm->aBuf));

    pGcm->uMode = iMode;
    pGcm->uLen = 0;
    pGcm->uAddLen = 0;

    // validate IV length (we only support 12-byte IVs)
    if (iIvLen == 12)
    {
        // generate y0
        ds_memcpy(pGcm->aY, pInitVec, iIvLen);
        pGcm->aY[15] = 1;
    }
    else
    {
        NetPrintf(("cryptgcm: iv length of %d not supported\n", iIvLen));
        return(-1);
    }

    // encrypt the block (GCM always uses forward encryption) and save output for later tag generation
    CryptAesEncryptBlock(&pGcm->KeySchedule, pGcm->aY, pGcm->aBaseEctr);

    // GCM process AAD
    for (pAad = pAddData, pGcm->uAddLen = iAddLen; iAddLen > 0; )
    {
        iBlockLen = (iAddLen < 16) ? iAddLen : 16;

        for (iData = 0; iData < iBlockLen; iData += 1)
        {
            pGcm->aBuf[iData] ^= pAad[iData];
        }

        _CryptGcmMultiply(pGcm, pGcm->aBuf, pGcm->aBuf);

        iAddLen -= iBlockLen;
        pAad += iBlockLen;
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _CryptGcmUpdate

    \Description
        Encrypt/decrypt the data

    \Input *pGcm        - cipher state
    \Input *pOutput     - [out] output buffer for decrypted data (may overlap with input)
    \Input *pInput      - encrypted data to decrypt
    \Input iLength      - length of input/output

    \Output
        int32_t         - zero=success, negative=failure

    \Version 07/01/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CryptGcmUpdate(CryptGcmT *pGcm, uint8_t *pOutput, const uint8_t *pInput, int32_t iLength)
{
    uint8_t *pOut = pOutput, aInput[16], aEctr[16];
    int32_t iData, iBlockLen;
    const uint8_t *pInp;

    // add in length
    pGcm->uLen += iLength;

    // handle input in blocks of 16
    for (pInp = pInput; iLength > 0;  )
    {
        // get block size
        iBlockLen = (iLength < 16) ? iLength : 16;
        // copy block to local buffer so we can handle overlapping I/O buffers
        ds_memcpy(aInput, pInp, iBlockLen);

        for (iData = 16; iData > 12; iData -= 1)
        {
            if (++pGcm->aY[iData-1] != 0)
            {
                break;
            }
        }

        // encrypt the block (GCM always uses forward encryption)
        CryptAesEncryptBlock(&pGcm->KeySchedule, pGcm->aY, aEctr);

        for (iData = 0; iData < iBlockLen; iData += 1)
        {
            if (pGcm->uMode == CRYPTAES_KEYTYPE_DECRYPT)
            {
                pGcm->aBuf[iData] ^= aInput[iData];
            }
            pOut[iData] = aEctr[iData] ^ aInput[iData];
            if (pGcm->uMode == CRYPTAES_KEYTYPE_ENCRYPT)
            {
                pGcm->aBuf[iData] ^= pOut[iData];
            }
        }

        _CryptGcmMultiply(pGcm, pGcm->aBuf, pGcm->aBuf);

        // move to next block
        iLength -= iBlockLen;
        pInp += iBlockLen;
        pOut += iBlockLen;
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _CryptGcmFinish

    \Description
        Generate authentication tag

    \Input *pGcm        - cipher state
    \Input *pTag        - [out] buffer to hold generated tag
    \Input iTagLen      - size of tag buffer

    \Output
        int32_t         - zero=success, negative=failure

    \Version 07/01/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CryptGcmFinish(CryptGcmT *pGcm, uint8_t *pTag, int32_t iTagLen)
{
    uint8_t aTagBuf[16];
    uint64_t uLen = pGcm->uLen * 8;
    uint64_t uAddLen = pGcm->uAddLen * 8;
    int32_t iTag;

    if (iTagLen > (signed)sizeof(aTagBuf))
    {
        return(-1);
    }
    if (iTagLen > 0)
    {
        ds_memcpy(pTag, pGcm->aBaseEctr, iTagLen);
    }

    if (uLen || uAddLen)
    {
        ds_memclr(aTagBuf, sizeof(aTagBuf));

        CRYPTGCM_WRWORD(uAddLen >> 32, aTagBuf+0);
        CRYPTGCM_WRWORD(uAddLen >>  0, aTagBuf+4);
        CRYPTGCM_WRWORD(uLen    >> 32, aTagBuf+8);
        CRYPTGCM_WRWORD(uLen    >>  0, aTagBuf+12);

        for (iTag = 0; iTag < (signed)sizeof(aTagBuf); iTag += 1)
        {
            pGcm->aBuf[iTag] ^= aTagBuf[iTag];
        }

        _CryptGcmMultiply(pGcm, pGcm->aBuf, pGcm->aBuf);

        for (iTag = 0; iTag < iTagLen; iTag += 1)
        {
            pTag[iTag] ^= pGcm->aBuf[iTag];
        }
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _CryptGcmEncrypt

    \Description
        GCM encrypt/decrypt data

    \Input *pGcm        - cipher state
    \Input iMode        - CRYPTAES_KEYTYPE_ENCRYPT or CRYPTAES_KEYTYPE_DECRYPT
    \Input *pOutput     - [out] output buffer for decrypted data (may overlap with input)
    \Input *pInput      - encrypted data to decrypt
    \Input iLength      - length of input/output
    \Input *pInitVec    - initialization vector (IV)
    \Input iIvLen       - length of IV
    \Input *pAddData    - additional authenticated data (AAD)
    \Input iAddLen      - length of AAD
    \Input *pTag        - tag to compare against for authentication
    \Input iTagLen      - length of tag

    \Output
        int32_t         - zero=success, negative=failure

    \Version 07/01/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CryptGcmEncrypt(CryptGcmT *pGcm, int32_t iMode, uint8_t *pOutput, const uint8_t *pInput, int32_t iLength, const uint8_t *pInitVec, int32_t iIvLen, const uint8_t *pAddData, int32_t iAddLen, uint8_t *pTag, int32_t iTagLen)
{
    int32_t iResult;

    // set up encryption
    if ((iResult = _CryptGcmStart(pGcm, iMode, pInitVec, iIvLen, pAddData, iAddLen)) != 0)
    {
        return(iResult);
    }
    // do the encryption
    if ((iResult = _CryptGcmUpdate(pGcm, pOutput, pInput, iLength)) != 0)
    {
        return(iResult);
    }
    // generate the authentication tag
    if ((iResult = _CryptGcmFinish(pGcm, pTag, iTagLen)) != 0)
    {
        return(iResult);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _CryptGcmDecrypt

    \Description
        GCM decrypt data, validate against authentication tag

    \Input *pGcm        - cipher state
    \Input *pOutput     - [out] output buffer for decrypted data (may overlap with input)
    \Input *pInput      - encrypted data to decrypt
    \Input iLength      - length of input/output
    \Input *pInitVec    - initialization vector (IV)
    \Input iIvLen       - length of IV
    \Input *pAddData    - additional authenticated data (AAD)
    \Input iAddLen      - length of AAD
    \Input *pTag        - tag to compare against for authentication
    \Input iTagLen      - length of tag

    \Output
        int32_t         - zero=success, negative=failure

    \Version 07/01/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CryptGcmDecrypt(CryptGcmT *pGcm, uint8_t *pOutput, const uint8_t *pInput, int32_t iLength, const uint8_t *pInitVec, int32_t iIvLen, const uint8_t *pAddData, int32_t iAddLen, const uint8_t *pTag, int32_t iTagLen)
{
    int32_t iByte, iDiff, iResult;
    uint8_t aDecryptTag[16];

    // do the decryption and generate authentication tag
    if ((iResult = _CryptGcmEncrypt(pGcm, CRYPTAES_KEYTYPE_DECRYPT, pOutput, pInput, iLength, pInitVec, iIvLen, pAddData, iAddLen, aDecryptTag, iTagLen)) != 0)
    {
        return(iResult);
    }

    // constant-time comparison of authentication tag
    for (iByte = 0, iDiff = 0; iByte < iTagLen; iByte += 1)
    {
        iDiff |= pTag[iByte] ^ aDecryptTag[iByte];
    }

    #if CRYPTGCM_VERBOSE
    NetPrintMem(aDecryptTag, iTagLen,  "CT");
    NetPrintMem(pTag, iTagLen, "T");
    #endif

    // if generated authentication tag does not match input tag, clear output and fail
    if (iDiff != 0)
    {
        ds_memclr(pOutput, iLength);
        NetPrintf(("cryptgcm: tag mismatch, decrypt failed\n"));
        return(-1);
    }

    // success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _CryptGcmInit

    \Description
        Initialize crypt state based on input key

    \Input *pGcm            - cipher state
    \Input *pKeyBuf         - input key
    \Input iKeyLen          - length of key in bytes

    \Version 07/01/2014 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptGcmInit(CryptGcmT *pGcm, const uint8_t *pKeyBuf, int32_t iKeyLen)
{
    // init key schedule
    CryptAesInitKeySchedule(&pGcm->KeySchedule, pKeyBuf, iKeyLen, CRYPTAES_KEYTYPE_ENCRYPT);

    // generate Shoop(?) table
    _CryptGcmGenTable(pGcm);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CryptGcmInit

    \Description
        Init the GCM cipher with the specified key

    \Input *pGcm        - cipher state to initialize
    \Input *pKeyBuf     - pointer to key
    \Input iKeyLen      - key length

    \Version 07/01/2014 (jbrookes)
*/
/********************************************************************************F*/
void CryptGcmInit(CryptGcmT *pGcm, const uint8_t *pKeyBuf, int32_t iKeyLen)
{
    // clear state
    ds_memclr(pGcm, sizeof(*pGcm));
    // init state
    _CryptGcmInit(pGcm, pKeyBuf, iKeyLen);
}

/*F********************************************************************************/
/*!
    \Function CryptGcmEncrypt

    \Description
        Encrypt data with AES-GCM

    \Input *pGcm        - cipher state
    \Input *pBuffer     - [inp/out] data to encrypt
    \Input iLength      - length of data to encrypt
    \Input *pInitVec    - initialization vector (IV)
    \Input iIvLen       - IV len
    \Input *pAddData    - additional authenticated data (AAD)
    \Input iAddLen      - length of AAD
    \Input *pTag        - [out] tag buffer
    \Input iTagLen      - tag length

    \Output
        int32_t     - encrypted data size

    \Version 07/01/2014 (jbrookes)
*/
/********************************************************************************F*/
int32_t CryptGcmEncrypt(CryptGcmT *pGcm, uint8_t *pBuffer, int32_t iLength, const uint8_t *pInitVec, int32_t iIvLen, const uint8_t *pAddData, int32_t iAddLen, uint8_t *pTag, int32_t iTagLen)
{
    int32_t iResult;

    #if CRYPTGCM_VERBOSE
    NetPrintf(("cryptgcm: encrypting size=%d ivlen=%d datalen=%d\n", iLength, iIvLen, iAddLen));
    NetPrintMem(pBuffer, iLength,  "P");
    NetPrintMem(pInitVec, iIvLen,  "V");
    NetPrintMem(pAddData, iAddLen, "A");
    #endif

    iResult = _CryptGcmEncrypt(pGcm, CRYPTAES_KEYTYPE_ENCRYPT, pBuffer, pBuffer, iLength, pInitVec, iIvLen, pAddData, iAddLen, pTag, iTagLen);

    #if CRYPTGCM_VERBOSE
    NetPrintMem(pBuffer, iLength, "C");
    #endif

    return(iResult == 0 ? iLength : 0);
}

/*F********************************************************************************/
/*!
    \Function CryptGcmDecrypt

    \Description
        Decrypt data with AES-GCM

    \Input *pGcm        - cipher state
    \Input *pBuffer     - [inp/out] data to decrypt
    \Input iLength      - length of data to decrypt
    \Input *pInitVec    - initialization vector (IV)
    \Input iIvLen       - IV len
    \Input *pAddData    - additional data (ADD)
    \Input iAddLen      - length of AAD
    \Input *pTag        - tag buffer
    \Input iTagLen      - tag length

    \Output
        int32_t     - decrypted data size

    \Version 07/01/2014 (jbrookes)
*/
/********************************************************************************F*/
int32_t CryptGcmDecrypt(CryptGcmT *pGcm, uint8_t *pBuffer, int32_t iLength, const uint8_t *pInitVec, int32_t iIvLen, const uint8_t *pAddData, int32_t iAddLen, uint8_t *pTag, int32_t iTagLen)
{
    int32_t iResult;

    if (iLength < 0)
    {
        NetPrintf(("cryptgcm: skipping decrypt of input with size=%d\n", iLength));
        return(-1);
    }

    #if CRYPTGCM_VERBOSE
    NetPrintf(("cryptgcm: decrypting size=%d ivlen=%d datalen=%d\n", iLength, iIvLen, iAddLen));
    NetPrintMem(pBuffer, iLength,  "C");
    NetPrintMem(pInitVec, iIvLen,  "V");
    NetPrintMem(pAddData, iAddLen, "A");
    #endif

    iResult = _CryptGcmDecrypt(pGcm, pBuffer, pBuffer, iLength, pInitVec, iIvLen, pAddData, iAddLen, pTag, iTagLen);

    #if CRYPTGCM_VERBOSE
    NetPrintMem(pBuffer, iLength, "P");
    #endif

    return(iResult == 0 ? iLength : -1);
}


