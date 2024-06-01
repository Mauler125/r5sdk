/*H********************************************************************************/
/*!
    \File cryptchacha.c

    \Description
        Implements the ChaCha20-Poly1305 AEAD cipher

    \Notes
        This implementation is based on the IETF Protocol and implements the
        ChaCha20-Poly1305 AEAD cipher as used in TLS et all.

        References:
            - ChaCha20 and Poly1305 for IETF Protocols: https://tools.ietf.org/html/rfc8439
            - ChaCha20-Poly1305 Cipher Suites for TLS: https://tools.ietf.org/html/rfc7905
            - Original implementation of ChaCha: https://cr.yp.to/chacha.html
            - Original implementation of Poly1305: https://cr.yp.to/mac.html

        As per https://tools.ietf.org/html/rfc7539#section-2.7, Poly1305 requires
        a one-time key and is "...biased, unlike HMAC".  As such it is not suitable
        for general use, and is therefore included dicrectly in the ChaCha20
        AEAD cipher implementation.

    \Copyright
        Copyright (c) 2018 Electronic Arts

    \Version 02/12/2018 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"

#include "DirtySDK/crypt/cryptbn.h"
#include "DirtySDK/crypt/cryptchacha.h"

/*** Defines **********************************************************************/

#define CRYPTCHACHA_VERBOSE (DIRTYCODE_DEBUG && FALSE)

/*** Macros ***********************************************************************/

#define CHACHA_RDWORD(_ptr)     (((uint32_t)(_ptr)[0]) | ((uint32_t)((_ptr)[1])<<8) | (((uint32_t)(_ptr)[2])<<16) | (((uint32_t)(_ptr)[3])<<24))
#define CHACHA_WRWORD(_ptr, _x) ((_ptr)[0] = (uint8_t)(_x),\
                                (_ptr)[1] = (uint8_t)((_x)>>8),\
                                (_ptr)[2] = (uint8_t)((_x)>>16),\
                                (_ptr)[3] = (uint8_t)((_x)>>24))
#define CHACHA_ROTATE(_x, _n)    (((_x)<<(_n))|((_x)>>(32-(_n))))
#define CHACHA_XOR(_v, _w)      ((_v) ^ (_w))
#define CHACHA_PLUS(_v, _w)     ((_v) + (_w))
#define CHACHA_PLUSONE(_v)      (CHACHA_PLUS((_v), 1))

#define CHACHA_QUARTERROUND(_x, _a, _b, _c, _d) \
    (_x)[_a] = CHACHA_PLUS((_x)[_a], (_x)[_b]); (_x)[_d] = CHACHA_ROTATE(CHACHA_XOR((_x)[_d], (_x)[_a]), 16); \
    (_x)[_c] = CHACHA_PLUS((_x)[_c], (_x)[_d]); (_x)[_b] = CHACHA_ROTATE(CHACHA_XOR((_x)[_b], (_x)[_c]), 12); \
    (_x)[_a] = CHACHA_PLUS((_x)[_a], (_x)[_b]); (_x)[_d] = CHACHA_ROTATE(CHACHA_XOR((_x)[_d], (_x)[_a]),  8); \
    (_x)[_c] = CHACHA_PLUS((_x)[_c], (_x)[_d]); (_x)[_b] = CHACHA_ROTATE(CHACHA_XOR((_x)[_b], (_x)[_c]),  7);

/*** Type Definitions *************************************************************/

//! Poly1305 state
typedef struct CryptPolyT
{
    CryptBnT r, s, a, p;
} CryptPolyT;

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*
    Poly1305 Routines
*/

/*F********************************************************************************/
/*!
    \Function _CryptPolyLE16

    \Description
       Covert input to little endian and pad to 16 bytes

    \Input *pOutput     - [out] output for padded and flipped data
    \Input *pInput      - input to pad/flip
    \Input iLength      - length of input data

    \Output
        int32_t         - length (16)

    \Version 02/14/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CryptPolyLE16(uint8_t *pOutput, const uint8_t *pInput, int32_t iLength)
{
    int32_t iData;
    if (iLength > 16)
    {
        iLength = 16;
    }
    for (iData = 0; iData < 16-iLength; iData += 1)
    {
        pOutput[iData] = 0;
    }
    for ( ; iData < 16; iData += 1)
    {
        pOutput[iData] = pInput[16-iData-1];
    }
    return(16);
}

/*F********************************************************************************/
/*!
    \Function _CryptPolyBnInit

    \Description
       Init a BigNumber from input data, optionally extending with 0x01

    \Input *pState      - BigNumber to init
    \Input iWidth       - width to pass through to CryptBnInit
    \Input *pBuffer     - input data
    \Input iLength      - length of input data
    \Input iExtra       - one to extend with 0x01, else zero

    \Version 02/14/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptPolyBnInit(CryptBnT *pState, int32_t iWidth, const uint8_t *pBuffer, int32_t iLength, int32_t iExtra)
{
    uint8_t aLEData[17];
    if (iExtra)
    {
        aLEData[0] = 0x01;
    }
    iLength = _CryptPolyLE16(aLEData+iExtra, pBuffer, iLength);
    CryptBnInitFrom(pState, iWidth, aLEData, iLength+iExtra);
}

/*F********************************************************************************/
/*!
    \Function _CryptPolyClamp

    \Description
        Clamp vector as per https://tools.ietf.org/html/rfc7539#section-2.5

    \Input *pInput          - input vector to clamp

    \Version 02/14/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptPolyClamp(uint8_t *pInput)
{
    pInput[3] &= 15;
    pInput[7] &= 15;
    pInput[11] &= 15;
    pInput[15] &= 15;
    pInput[4] &= 252;
    pInput[8] &= 252;
    pInput[12] &= 252;
}

/*F********************************************************************************/
/*!
    \Function _CryptPolyInit

    \Description
       Init Poly state with specified key

    \Input *pState      - module state
    \Input *pKey        - input encryption key
    \Input iKeyLen      - length of key

    \Version 02/14/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptPolyInit(CryptPolyT *pState, uint8_t *pKey, int32_t iKeyLen)
{
    static const uint8_t p_src[] = { 0x3,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfb };
    // clamp(r): r &= 0x0ffffffc0ffffffc0ffffffc0fffffff
    _CryptPolyClamp(pKey); 
    // r = (le_bytes_to_num(key[0..15])
    _CryptPolyBnInit(&pState->r, -1, pKey, 16, 0);
    // s = le_num(key[16..31])
    _CryptPolyBnInit(&pState->s, -1, pKey+16, 16, 0);
    // accumulator = 0
    CryptBnInitSet(&pState->a, 0);
    // p = (1<<130)-5
    CryptBnInitFrom(&pState->p, -1, p_src, sizeof(p_src));
}

/*F********************************************************************************/
/*!
    \Function _CryptPolyUpdate

    \Description
       Update Poly state with specified input data.  Data is padded to 16 byte
       width and extended with 0x1 as per https://tools.ietf.org/html/rfc7539#section-2.5.1

    \Input *pState      - module state
    \Input *pData       - input data to process
    \Input iLength      - length of data

    \Notes
        BnMultiply and BnMod steps are utilized instead of BnModMultiply because
        the product of a and r can exceed the size of the modulus p; this is not
        supported by CryptBn.

    \Version 02/14/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptPolyUpdate(CryptPolyT *pState, const uint8_t *pData, int32_t iLength)
{
    CryptBnT n;

    // for i=1 upto ceil(msg length in bytes / 16)
    for ( ; iLength > 0; pData += 16, iLength -= 16)
    {
        // n = le_bytes_to_num(msg[((i-1)*16)..(i*16)] | [0x01])
        _CryptPolyBnInit(&n, -1, pData, iLength, 1);
        // a += n
        CryptBnAccumulate(&pState->a, &n);
        // a *= r
        CryptBnMultiply(&pState->a, &pState->r, &pState->a);
        // a %= p
        CryptBnMod(&pState->a, &pState->p, &pState->a, NULL);
    }
}

/*F********************************************************************************/
/*!
    \Function _CryptPolyFinal

    \Description
       Generate final tag

    \Input *pState      - module state
    \Input *pData       - [out] output buffer to write tag to
    \Input iLength      - length of buffer

    \Version 02/14/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptPolyFinal(CryptPolyT *pState, uint8_t *pData, int32_t iLength)
{
    uint8_t aTag[16];
    // a += s
    CryptBnAccumulate(&pState->a, &pState->s);
    // num_to_16_le_bytes(a)
    CryptBnFinal(&pState->a, aTag, sizeof(aTag));
    // write out the tag
    _CryptPolyLE16(pData, aTag, iLength);
}

/*F********************************************************************************/
/*!
    \Function _CryptPolyGenerateTag

    \Description
       Generate authentication tag based on input data, key, and additional
       authentication data.

    \Input *pBuffer     - input data to generate tag for
    \Input iLength      - length of input data
    \Input *pKey        - encryption key used for poly operation
    \Input iKeyLen      - length of key
    \Input *pAddData    - additional unencrypted data
    \Input iAddLen      - length of additional data
    \Input *pTag        - [out] buffer to write tag to
    \Input iTagLen      - length of tag buffer

    \Version 02/14/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptPolyGenerateTag(const uint8_t *pBuffer, int32_t iLength, uint8_t *pKey, int32_t iKeyLen, const uint8_t *pAddData, int32_t iAddLen, uint8_t *pTag, int32_t iTagLen)
{
    CryptPolyT Poly;
    uint8_t aLengths[16];

    // generate add and txt lengths
    aLengths[0] = (uint8_t)(iAddLen>>0);
    aLengths[1] = (uint8_t)(iAddLen>>8);
    aLengths[2] = (uint8_t)(iAddLen>>16);
    aLengths[3] = (uint8_t)(iAddLen>>24);
    aLengths[4] = aLengths[5] = aLengths[6] = aLengths[7] = 0;
    aLengths[8] = (uint8_t)(iLength>>0);
    aLengths[9] = (uint8_t)(iLength>>8);
    aLengths[10] = (uint8_t)(iLength>>16);
    aLengths[11] = (uint8_t)(iLength>>24);
    aLengths[12] = aLengths[13] = aLengths[14] = aLengths[15] = 0;

    // init poly state 
    _CryptPolyInit(&Poly, pKey, iKeyLen);

    // update with add data (padded) | text (padded) | lengths
    _CryptPolyUpdate(&Poly, pAddData, iAddLen);
    _CryptPolyUpdate(&Poly, pBuffer, iLength);
    _CryptPolyUpdate(&Poly, aLengths, sizeof(aLengths));

    // generate tag
    _CryptPolyFinal(&Poly, pTag, iTagLen);
}

/*
    ChaCha20 Routines
*/

/*F********************************************************************************/
/*!
    \Function _ChaCha20Block

    \Description
        ChaCha20 block round as per https://tools.ietf.org/html/rfc7539#section-2.3.1

    \Input *pOutput     - [out] buffer to write output
    \Input *pInput      - input for block round

    \Version 02/12/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _ChaCha20Block(uint8_t *pOutput, const uint32_t *pInput)
{
    uint32_t aBlock[16];
    int32_t iBlock;

    // read iBlockInput
    ds_memcpy(aBlock, pInput, sizeof(aBlock));

    // quarter rounds; note that the original reference had eight rounds, while the IETF version has ten
    for (iBlock = 0; iBlock < 10; iBlock += 1)
    {
        CHACHA_QUARTERROUND(aBlock, 0, 4,  8, 12);
        CHACHA_QUARTERROUND(aBlock, 1, 5,  9, 13);
        CHACHA_QUARTERROUND(aBlock, 2, 6, 10, 14);
        CHACHA_QUARTERROUND(aBlock, 3, 7, 11, 15);
        CHACHA_QUARTERROUND(aBlock, 0, 5, 10, 15);
        CHACHA_QUARTERROUND(aBlock, 1, 6, 11, 12);
        CHACHA_QUARTERROUND(aBlock, 2, 7,  8, 13);
        CHACHA_QUARTERROUND(aBlock, 3, 4,  9, 14);
    }

    // accumulate input and write output
    for (iBlock = 0; iBlock < 16; iBlock += 1)
    {
        CHACHA_WRWORD(pOutput + (4*iBlock), CHACHA_PLUS(aBlock[iBlock], pInput[iBlock]));
    }
}

/*F********************************************************************************/
/*!
    \Function _CryptChaChaInitKey

    \Description
        Initialize crypt state based on input key

    \Input *pChaCha     - cipher state
    \Input *pKeyBuf     - input key
    \Input iKeyLen      - length of key in bytes

    \Version 02/12/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptChaChaInitKey(CryptChaChaT *pChaCha, const uint8_t *pKeyBuf, int32_t iKeyLen)
{
    static const char _strSigma[16] = "expand 32-byte k";
    static const char _strTau[16] = "expand 16-byte k";
    const char *pConstant;

    // read first half of key
    pChaCha->aInput[4] = CHACHA_RDWORD(pKeyBuf+0);
    pChaCha->aInput[5] = CHACHA_RDWORD(pKeyBuf+4);
    pChaCha->aInput[6] = CHACHA_RDWORD(pKeyBuf+8);
    pChaCha->aInput[7] = CHACHA_RDWORD(pKeyBuf+12);

    // determine 2nd half based on key length
    if (iKeyLen == 32)
    {
        pKeyBuf += 16;
        pConstant = _strSigma;
    }
    else
    {
        pConstant = _strTau;
    }

    // read second half of key
    pChaCha->aInput[8] = CHACHA_RDWORD(pKeyBuf+0);
    pChaCha->aInput[9] = CHACHA_RDWORD(pKeyBuf+4);
    pChaCha->aInput[10] = CHACHA_RDWORD(pKeyBuf+8);
    pChaCha->aInput[11] = CHACHA_RDWORD(pKeyBuf+12);

    // read in constant
    pChaCha->aInput[0] = CHACHA_RDWORD(pConstant+0);
    pChaCha->aInput[1] = CHACHA_RDWORD(pConstant+4);
    pChaCha->aInput[2] = CHACHA_RDWORD(pConstant+8);
    pChaCha->aInput[3] = CHACHA_RDWORD(pConstant+12);
}

/*F********************************************************************************/
/*!
    \Function _CryptChaChaGeneratePolyKey

    \Description
        Generate the Poly key as per https://tools.ietf.org/html/rfc7539#section-2.6

    \Input *pChaCha         - cipher state
    \Input *pKey            - [out] buffer for generated key
    \Input iKeyLen          - length of buffer

    \Version 02/14/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptChaChaGeneratePolyKey(CryptChaChaT *pChaCha, uint8_t *pKey, int32_t iKeyLen)
{
    uint8_t aPoly[64];
    uint32_t uCounter;
    // save current counter, set it to zero
    uCounter = pChaCha->aInput[12];
    pChaCha->aInput[12] = 0;
    // use chacha20 to generate key state
    _ChaCha20Block(aPoly, pChaCha->aInput);
    // clamp r
    _CryptPolyClamp(aPoly);
    // copy out key
    ds_memcpy_s(pKey, iKeyLen, aPoly, 32);
    // restore counter
    pChaCha->aInput[12] = uCounter;
}

/*F********************************************************************************/
/*!
    \Function _CryptChaChaInitIv

    \Description
        Initialize IV/Nonce portion of ChaCha state as per
        https://tools.ietf.org/html/rfc7539#section-2.3

    \Input *pChaCha     - cipher state
    \Input *pInitVec    - input iv
    \Input iIvLen       - iv length

    \Version 02/12/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptChaChaInitIv(CryptChaChaT *pChaCha, const uint8_t *pInitVec, int32_t iIvLen)
{
    pChaCha->aInput[12] = 1; // as per https://tools.ietf.org/html/rfc7539#section-2.8 initial counter value is one
    pChaCha->aInput[13] = CHACHA_RDWORD(pInitVec+0);
    pChaCha->aInput[14] = CHACHA_RDWORD(pInitVec+4);
    pChaCha->aInput[15] = CHACHA_RDWORD(pInitVec+8);
}

/*F********************************************************************************/
/*!
    \Function _CryptChaChaEncrypt

    \Description
        Encrypted plaintext as per https://tools.ietf.org/html/rfc7539#section-2.4

    \Input *pChaCha         - cipher state
    \Input *pInput          - input to encrypt
    \Input *pOutput         - [out] buffer to write decrypted data to (may overlap)
    \Input iLength          - data length

    \Version 02/12/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _CryptChaChaEncrypt(CryptChaChaT *pChaCha, const uint8_t *pInput, uint8_t *pOutput, int32_t iLength)
{
    int32_t iCount, iCopyLen;
    uint8_t aOutput[64];

    while (iLength > 0)
    {
        _ChaCha20Block(aOutput, pChaCha->aInput);
        pChaCha->aInput[12] = CHACHA_PLUSONE(pChaCha->aInput[12]);
        if (!pChaCha->aInput[12])
        {
            pChaCha->aInput[13] = CHACHA_PLUSONE(pChaCha->aInput[13]);
        }
        for (iCount = 0, iCopyLen = (iLength >= 64) ? 64 : iLength; iCount < iCopyLen; iCount += 1)
        {
            pOutput[iCount] = pInput[iCount] ^ aOutput[iCount];
        }
        iLength -= iCopyLen;
        pInput += iCopyLen;
        pOutput += iCopyLen;
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CryptChaChaInit

    \Description
        Init the ChaCha cipher with the specified key

    \Input *pChaCha     - cipher state to initialize
    \Input *pKeyBuf     - pointer to key
    \Input iKeyLen      - key length

    \Version 02/12/2018 (jbrookes)
*/
/********************************************************************************F*/
void CryptChaChaInit(CryptChaChaT *pChaCha, const uint8_t *pKeyBuf, int32_t iKeyLen)
{
    // clear state
    ds_memclr(pChaCha, sizeof(*pChaCha));
    // init state with key
    _CryptChaChaInitKey(pChaCha, pKeyBuf, iKeyLen);
}

/*F********************************************************************************/
/*!
    \Function CryptChaChaEncrypt

    \Description
        Encrypt input data in place and optionally generate authentication tag

    \Input *pChaCha     - cipher state
    \Input *pBuffer     - [inp/out] data to encrypt
    \Input iLength      - length of data to encrypt
    \Input *pInitVec    - initialization vector (IV)
    \Input iIvLen       - IV len
    \Input *pAddData    - additional authenticated data
    \Input iAddLen      - length of additional data
    \Input *pTag        - [out] tag buffer
    \Input iTagLen      - tag length

    \Notes
        Pass NULL to pTag if you don't want authentication tag to be generated.

    \Output
        int32_t         - encrypted data size

    \Version 07/01/2014 (jbrookes)
*/
/********************************************************************************F*/
int32_t CryptChaChaEncrypt(CryptChaChaT *pChaCha, uint8_t *pBuffer, int32_t iLength, const uint8_t *pInitVec, int32_t iIvLen, const uint8_t *pAddData, int32_t iAddLen, uint8_t *pTag, int32_t iTagLen)
{
    uint8_t aPolyKey[32];

    // set IV in state
    _CryptChaChaInitIv(pChaCha, pInitVec, iIvLen);

    if (pTag != NULL)
    {
        // generate the Poly1305 key
        _CryptChaChaGeneratePolyKey(pChaCha, aPolyKey, sizeof(aPolyKey));
    }

    // encrypt the input
    _CryptChaChaEncrypt(pChaCha, pBuffer, pBuffer, iLength);

    if (pTag != NULL)
    {
        // generate the authentication tag
        _CryptPolyGenerateTag(pBuffer, iLength, aPolyKey, sizeof(aPolyKey), pAddData, iAddLen, pTag, iTagLen);
    }

    // return success
    return(iLength);
}

/*F********************************************************************************/
/*!
    \Function CryptChaChaDecrypt

    \Description
        Decrypt input data in place and optionally generate authentication tag;
        verify it matches the specified tag.

    \Input *pChaCha     - cipher state
    \Input *pBuffer     - [inp/out] data to decrypt
    \Input iLength      - length of data to decrypt
    \Input *pInitVec    - initialization vector (IV)
    \Input iIvLen       - IV len
    \Input *pAddData    - additional authenticated data
    \Input iAddLen      - length of additional data
    \Input *pTag        - tag buffer
    \Input iTagLen      - tag length

    \Notes
        Pass NULL to pTag if you don't want to generate and verify the authentication
        tag.

    \Output
        int32_t     - decrypted data size

    \Version 02/12/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t CryptChaChaDecrypt(CryptChaChaT *pChaCha, uint8_t *pBuffer, int32_t iLength, const uint8_t *pInitVec, int32_t iIvLen, const uint8_t *pAddData, int32_t iAddLen, const uint8_t *pTag, int32_t iTagLen)
{
    uint8_t aPolyKey[32], aTag[16];

    // set IV in state
    _CryptChaChaInitIv(pChaCha, pInitVec, iIvLen);

    if (pTag != NULL)
    {
        // generate the Poly1305 key
        _CryptChaChaGeneratePolyKey(pChaCha, aPolyKey, sizeof(aPolyKey));

        // generate the authentication tag on the encrypted data
        _CryptPolyGenerateTag(pBuffer, iLength, aPolyKey, sizeof(aPolyKey), pAddData, iAddLen, aTag, sizeof(aTag));
    }

    // do the decrypt
    _CryptChaChaEncrypt(pChaCha, pBuffer, pBuffer, iLength);

    // return length of decrypted data, or -1 if tag mismatch
    return((pTag == NULL) || !memcmp(pTag, aTag, iTagLen) ? iLength : -1);
}


