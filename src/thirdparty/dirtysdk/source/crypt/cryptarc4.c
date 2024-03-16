/*H********************************************************************************/
/*!
    \File cryptarc4.c

    \Description
        This module is a from-scratch ARC4 implementation designed to avoid
        any intellectual property complications. The ARC4 stream cipher is
        known to produce output that is compatible with the RC4 stream cipher.

    \Notes
        This algorithm from this cypher was taken from the web site:
        ciphersaber.gurus.com.  It is based on the RC4 compatible algorithm that
        was published in the 2nd ed of the book Applied Cryptography by Bruce
        Schneier.  This is a private-key stream cipher which means that some other
        secure way of exchanging cipher keys prior to algorithm use is required.
        Its strength is directly related to the key exchange algorithm strength.
        In operation, each individual stream message must use a unique key.  This
        is handled by appending on 10 byte random value onto the private key.  This
        10-byte data can be sent by public means to the receptor (or included at the
        start of a file or whatever). When the private key is combined with this
        public key, it essentially puts the cypher into a random starting state (it
        is like using a message digest routine to generate a random hash for
        password comparison).  The code below was written from scratch using only
        a textual algorithm description.

    \Copyright
        Copyright (c) 2000-2005 Electronic Arts Inc.

    \Version 1.0 02/25/2000 (gschaefer) First Version
    \Version 1.1 11/06/2002 (jbrookes)  Removed Create()/Destroy() to eliminate mem
                                        alloc dependencies.
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/crypt/cryptarc4.h"
#include "DirtySDK/crypt/cryptrand.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private variables

// Public variables


/*** Private Functions ************************************************************/

/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CryptArc4Init

    \Description
        Initialize state for ARC4 encryption/decryption module.

    \Input *pState  - module state
    \Input *pKeyBuf - pointer to crypt key
    \Input iKeyLen  - length of crypt key (sixteen for RC4-128)
    \Input iIter    - internal iterator (1=RC4 compat, larger for slightly improved security

    \Version 02/26/2000 (gschaefer)
*/
/********************************************************************************F*/
void CryptArc4Init(CryptArc4T *pState, const uint8_t *pKeyBuf, int32_t iKeyLen, int32_t iIter)
{
    uint32_t uWalk;
    uint8_t uSwap;
    uint8_t uTemp0;
    uint8_t uTemp1;

    // clamp iteration count
    if (iIter < 1)
    {
        iIter = 1;
    }

    // reset the permutators
    pState->walk = 0;
    pState->swap = 0;

    // init the state array
    for (uWalk = 0; uWalk < 256; ++uWalk)
    {
        pState->state[uWalk] = (uint8_t)uWalk;
    }

    // only do setup if key is valid
    if ((iKeyLen > 0) && (iIter > 0))
    {
        // mixup the state table
        for (uWalk = 0, uSwap = 0; uWalk < 256; ++uWalk)
        {
            // determine the swap point
            uSwap += pState->state[uWalk] + pKeyBuf[uWalk % iKeyLen];
            // swap the entries
            uTemp0 = pState->state[uWalk];
            uTemp1 = pState->state[uSwap];
            pState->state[uWalk] = uTemp1;
            pState->state[uSwap] = uTemp0;
        }

        // advance state for added security (not RC4 compatible)
        if (iIter > 1)
        {
            CryptArc4Advance(pState, iIter*256);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function CryptArc4Apply

    \Description
        Apply the cipher to the data. Uses reversible XOR so calling twice undoes
        the uncryption (assuming the key state has been reset).

    \Input *pState  - module state
    \Input *pBuffer - buffer to encrypt/decrypt
    \Input iLength  - length of buffer

    \Version 02/26/2000 (gschaefer)
*/
/********************************************************************************F*/
void CryptArc4Apply(CryptArc4T *pState, uint8_t *pBuffer, int32_t iLength)
{
    uint32_t uTemp0;
    uint32_t uTemp1;
    uint32_t uWalk = pState->walk;
    uint32_t uSwap = pState->swap;

    // do each data byte
    for (; iLength > 0; --iLength)
    {
        // determine the swap points
        uWalk = (uWalk+1)&255;
        uSwap = (uSwap+pState->state[uWalk])&255;

        // swap the state entries
        uTemp0 = pState->state[uWalk];
        uTemp1 = pState->state[uSwap];
        pState->state[uWalk] = uTemp1;
        pState->state[uSwap] = uTemp0;

        // apply the cypher
        *pBuffer++ ^= pState->state[(uTemp0+uTemp1)&255];
    }

    // update the state
    pState->walk = uWalk;
    pState->swap = uSwap;
}

/*F********************************************************************************/
/*!
    \Function CryptArc4Advance

    \Description
        Advance the cipher state with by iLength bytes.

    \Input *pState  - module state
    \Input iLength  - length to advance

    \Version 12/06/2005 (jbrookes)
*/
/********************************************************************************F*/
void CryptArc4Advance(CryptArc4T *pState, int32_t iLength)
{
    uint32_t uTemp0;
    uint32_t uTemp1;
    uint32_t uWalk = pState->walk;
    uint32_t uSwap = pState->swap;

    // do each data byte
    for (; iLength > 0; --iLength)
    {
        // determine the swap points
        uWalk = (uWalk+1)&255;
        uSwap = (uSwap+pState->state[uWalk])&255;

        // swap the state entries
        uTemp0 = pState->state[uWalk];
        uTemp1 = pState->state[uSwap];
        pState->state[uWalk] = uTemp1;
        pState->state[uSwap] = uTemp0;
    }

    // update the state
    pState->walk = uWalk;
    pState->swap = uSwap;
}

/*F********************************************************************************/
/*!
    \Function CryptArc4StringEncrypt

    \Description
        Easy way to encode an asciiz string. The resulting string is iLen chars
        of visible ascii characters.  String characters must fall in the range of
        32 to 127 to be considered valid.

    \Input *pDst    - pointer to output string
    \Input iLen     - length of output string
    \Input *pSrc    - pointer to source string (asciiz)
    \Input *pKey    - key to encrypt with
    \Input iKey     - length of key (-1=key is asciiz)
    \Input iIter    - number of initialization iterations

    \Version 01/10/2013 (jbrookes) Based on CryptSSC2 by GWS
*/
/********************************************************************************F*/
void CryptArc4StringEncrypt(char *pDst, int32_t iLen, const char *pSrc, const uint8_t *pKey, int32_t iKey, int32_t iIter)
{
    static uint8_t _CryptArc4_aRandom[32], _CryptArc4_bRandStateInit = FALSE;
    static CryptArc4T _CryptArc4_Rand;
    uint8_t uDat, uEnc;
    int32_t iSum;
    CryptArc4T Arc4Data;

    // get initial random data
    if (_CryptArc4_bRandStateInit == FALSE)
    {
        CryptRandGet(_CryptArc4_aRandom, sizeof(_CryptArc4_aRandom));
        CryptArc4Init(&_CryptArc4_Rand, pKey, iKey, iIter);
        _CryptArc4_bRandStateInit = TRUE;
    }

    // init state for string encryption
    CryptArc4Init(&Arc4Data, pKey, iKey, iIter);

    // update the random cipher
    CryptArc4Apply(&_CryptArc4_Rand, _CryptArc4_aRandom, sizeof(_CryptArc4_aRandom));
    CryptArc4Init(&_CryptArc4_Rand, _CryptArc4_aRandom, sizeof(_CryptArc4_aRandom), iIter);

    // encrypt the string
    for (uDat = 0, uEnc = 0; iLen > 1; pDst += 1, iLen -= 1)
    {
        // get source data
        if (pSrc == NULL)
        {
            // append random fill data
            CryptArc4Apply(&_CryptArc4_Rand, &uDat, 1);
            // fix the range
            uDat = 32+(uDat & 63);
        }
        else if ((uDat = *pSrc++) == '\0')
        {
            // append random fill data
            pSrc = NULL;
        }

        // deal with out of range data
        if ((uDat < 32) || (uDat >= 127))
        {
            uDat = 127;
        }

        // get cipher character
        CryptArc4Apply(&Arc4Data, &uEnc, 1);

        // encode the data
        iSum = (96 - 32) + uDat + (uEnc % 96);
        *pDst = (char)(32 + (iSum % 96));
    }

    // terminate buffer
    if (iLen > 0)
    {
        *pDst = 0;
    }
}

/*F*************************************************************************************************/
/*!
    \Function CryptArc4StringDecrypt

    \Description
        Decode an asciiz string encoded with CryptArc4StringEncrypt.

    \Input *pDst    - pointer to output string
    \Input iLen     - length of output string
    \Input *pSrc    - pointer to source string (asciiz)
    \Input *pKey    - key to decrypt with
    \Input iKey     - length of key (-1=key is asciiz)
    \Input iIter    - number of initialization iterations

    \Version 01/10/2013 (jbrookes) Based on CryptSSC2 by GWS
*/
/*************************************************************************************************F*/
void CryptArc4StringDecrypt(char *pDst, int32_t iLen, const char *pSrc, const uint8_t *pKey, int32_t iKey, int32_t iIter)
{
    int32_t iIdx, iSum;
    uint8_t uDat, uDec;
    CryptArc4T Arc4;

    // setup initial state
    CryptArc4Init(&Arc4, pKey, iKey, iIter);

    // decrypt the string
    for (iIdx = 0, uDat = 0, uDec = 0; iIdx < iLen-1; pDst += 1)
    {
        // get encoded source
        if ((uDat = pSrc[iIdx++]) == '\0')
        {
            break;
        }

        // get cipher character
        CryptArc4Apply(&Arc4, &uDec, 1);

        // decode the data
        iSum = (96 - 32) + uDat - (uDec % 96);
        *pDst = (char)(32 + (iSum % 96));
        if (*pDst == 127)
        {
            break;
        }
    }

    // terminate buffer
    if (iIdx < iLen)
    {
        *pDst = '\0';
    }
}

/*F*************************************************************************************************/
/*!
    \Function CryptArc4StringEncryptStaticCode

    \Description
        This function decrypts an encrypted key/encoded string pair into the source plaintext string.
        It also accepts the plaintext string as an argument to verify that the source plaintext string
        matches the resulting decrypted string.  If there is a mismatch updated key and encoded string
        data is printed in c-style array form and an error is returned.  The source plaintext string
        parameter is only referenced in debug builds to prevent the plaintext string from leaking into
        the release binary.  The CryptArc4StringEncryptStatic() macro wrapper should always be used to
        call this function, and the plaintext string itself should be defined via the preprocessor,
        to prevent the plaintext string from leaking into the release binary.

    \Input *pStr    - [in/out] pointer to encrypted string, and output buffer for decrypted string
    \Input iStrSize - length of string buffer
    \Input *pKey    - key to decrypt with
    \Input iKeySize - length of key
    \Input *pStrSrc - decrypted key (debug only)

    \Output
        int32_t     - zero=success, negative means the decrypted string does not match the plaintext string (debug only)

    \Version 06/05/2014 (jbrookes)
*/
/*************************************************************************************************F*/
int32_t CryptArc4StringEncryptStaticCode(char *pStr, int32_t iStrSize, const uint8_t *pKey, int32_t iKeySize, const char *pStrSrc)
{
    CryptArc4T Arc4;
    int32_t iResult = 0;

    // decrypt the url using RC4-dropN
    CryptArc4Init(&Arc4, pKey, iKeySize, 3*1024);
    CryptArc4Apply(&Arc4, (uint8_t *)pStr, iStrSize);

    /* validate the decrypted string matches what we expect; if not it means someone forgot
       to update the encrypted data.  we only compile this in a DEBUG build to prevent the
       plaintext string from leaking into the executable */
    #if DIRTYCODE_DEBUG
    if ((pStrSrc != NULL) && (strcmp(pStr, pStrSrc)))
    {
        uint8_t aKey[32], aEnc[1024];
        NetPrintf(("cryptarc4: mismatch between decrypted string %s and source string %s; encrypted string needs to be updated\n", pStr, pStrSrc));
        // set up url buffer to encrypt... we encrypt the whole buffer so first we clear it
        ds_memclr(aEnc, sizeof(aEnc));
        ds_strnzcpy((char *)aEnc, pStrSrc, iStrSize);
        // encrypt the url buffer using RC4-dropN
        CryptRandGet(aKey, sizeof(aKey));
        CryptArc4Init(&Arc4, aKey, sizeof(aKey), 3*1024);
        CryptArc4Apply(&Arc4, aEnc, iStrSize);
        // format the encryption key and encrypted url buffer to debug output
        NetPrintArray(aKey, sizeof(aKey), "strKey");
        NetPrintArray(aEnc, iStrSize, "strEnc");
        // return failure to caller
        iResult = -1;
    }
    else
    {
        NetPrintf(("cryptarc4: decrypted string %s\n", pStr));
    }
    #endif

    return(iResult);
}

