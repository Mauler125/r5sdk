/*H********************************************************************************/
/*!
    \File cryptrsa.c

    \Description
        This module is a from-scratch RSA implementation in order to avoid
        any intellectual property issues. The 1024 bit RSA public key
        encryption algorithm was implemented from a specification provided
        by Netscape for SSL implementation (see protossl.h).

    \Copyright
        Copyright (c) Electronic Arts 2002-2011.

    \Todo
        64bit word support (UCRYPT_SIZE=8) has been tested to work with the unix64-gcc
        (Linux) and ps3-gcc (PS3) compilers; however the current implementation does
        not work with certain odd modulus sizes (for example the 1000-bit modulus of
        the built-in RSA CA certificate), so it is not currently enabled.

    \Version 1.0 03/08/2002 (gschaefer) First Version (protossl)
    \Version 1.1 03/05/2003 (jbrookes)  Split RSA encryption from protossl
    \Version 1.2 11/16/2011 (jbrookes)  Optimizations to improve dbg&opt performance
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <string.h>             // memset

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/crypt/cryptrand.h"

#include "DirtySDK/crypt/cryptrsa.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private variables


// Public variables


/*** Private functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _CryptRSAExponentiate

    \Description
        Do the actual encryption: result = (value ^ exponent) % modulus.
        Exponentiation is done using a sliding window, this function needs to
        be called iteratively until the function returns zero.

    \Input *pState      - crypt state
    \Input *pAccumul    - [out] where we accumulate our result
    \Input *pPowerof    - contains our initial value and is used for squaring
    \Input *pExponent   - the exponent of the equation
    \Input *pModulus    - the modulus of the equation

    \Output
        int32_t         - zero=done, else call again

    \Notes
        See Handbook of Applied Cryptography Chapter 14.6.1 (14.85):
        http://cacr.uwaterloo.ca/hac/about/chap14.pdf

    \Version 05/08/2017 (eesponda)
*/
/********************************************************************************H*/
static int32_t _CryptRSAExponentiate(CryptRSAT *pState, CryptBnT *pAccumul, CryptBnT *pPowerof, const CryptBnT *pExponent, const CryptBnT *pModulus)
{
    uint64_t uTickUsecs = NetTickUsec();
    int32_t iResult = 1;
    int32_t iMaxExponentBit = CryptBnBitLen(pExponent);
    /* use a fixed window size of 6 for larger exponents,
       we provide a fallthrough for smaller exponents */
    const int32_t iWindowSize = iMaxExponentBit > 512 ? 6 : 1;

    // if this is the first time, handle our precomputations
    if (pState->pTable == NULL)
    {
        const int32_t iTableSize = 1 << (iWindowSize - 1);

        if (iTableSize == 1)
        {
            // point to fixed array
            pState->pTable = pState->aTable;
        }
        else
        {
            DirtyMemGroupQuery(&pState->iMemGroup, &pState->pMemGroupUserData);

            // allocate memory for the table to avoid large data on the stack
            if ((pState->pTable = (CryptBnT *)DirtyMemAlloc(sizeof(CryptBnT) * iTableSize, CRYPTRSA_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
            {
                NetPrintf(("cryptrsa: [%p] unable to allocate memory for precomputed table used for sliding window\n", pState));
                return(0);
            }
        }
        ds_memclr(pState->pTable, sizeof(CryptBnT) * iTableSize);

        /* put the already reduced input into the first
           index of the table, this will make it so further
           entries in the table will not be too large
           for our modulus operations */
        CryptBnClone(&pState->pTable[0], pPowerof);

        /* precompute the different powers of input
           that we use when we have a window that is
           greater than one */
        if (iTableSize > 1)
        {
            int32_t iTableIndex;
            CryptBnT Square;

            // calculate (input^2) % mod as the basis for the other calculations
            CryptBnModMultiply(&Square, &pState->pTable[0], &pState->pTable[0], pModulus);

            // calculate (input^(2+iTableIndex)) % mod for the rest of the table
            for (iTableIndex = 1; iTableIndex < iTableSize; iTableIndex += 1)
            {
                CryptBnModMultiply(&pState->pTable[iTableIndex], &pState->pTable[iTableIndex - 1], &Square, pModulus);
            }
        }

        // set the defaults
        pState->iExpBitIndex = iMaxExponentBit - 1;
    }

    /* scan backwards from the current exponent bit
       until a set bit is found to denote the start of the window
       squaring the results until such a bit is found */
    if (CryptBnBitTest(pExponent, pState->iExpBitIndex))
    {
        int32_t iWindowBit, iWindowValue, iWindowEnd;

        /* scan backwards from the start of the window until the last set bit in the range of the window size is found which denotes the end bit index of the window 
           calculate the value of the window that will be used when multiplying against our precomputed table
           we skip the first bit as we know it is set based on the current exponent bit check we make right above */
        for (iWindowBit = 1, iWindowValue = 1, iWindowEnd = 0; (iWindowBit < iWindowSize) && ((pState->iExpBitIndex - iWindowBit) >= 0); iWindowBit += 1)
        {
            if (CryptBnBitTest(pExponent, pState->iExpBitIndex - iWindowBit))
            {
                iWindowValue <<= (iWindowBit - iWindowEnd);
                iWindowValue  |= 1; /* force odd */
                iWindowEnd = iWindowBit;
            }
        }

        // square for all the bits in the window and moving the exponent bit index down each time
        for (iWindowBit = 0; iWindowBit < iWindowEnd + 1; iWindowBit += 1, pState->iExpBitIndex -= 1)
        {
            CryptBnModMultiply(pAccumul, pAccumul, pAccumul, pModulus);
        }

        // use the window value to get which precomputed power we need to multiply skip the first multiply
        if (!pState->bAccumulOne)
        {
            CryptBnModMultiply(pAccumul, pAccumul, &pState->pTable[iWindowValue/2], pModulus);
        }
        else
        {
            CryptBnClone(pAccumul, &pState->pTable[iWindowValue/2]);
            pState->bAccumulOne = FALSE;
        }
    }
    else
    {
        CryptBnModMultiply(pAccumul, pAccumul, pAccumul, pModulus);
        pState->iExpBitIndex -= 1;
    }

    // check if we are done
    if (pState->iExpBitIndex < 0)
    {
        // we are done, update timings and return appropriate result
        pState->uCryptMsecs = (pState->uCryptUsecs+500)/1000;
        iResult = 0;

        if (pState->pTable != pState->aTable)
        {
            // clean up the table memory
            DirtyMemFree(pState->pTable, CRYPTRSA_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        }
        pState->pTable = NULL;
    }

    // update timing
    pState->uCryptUsecs += (uint32_t)NetTickDiff(NetTickUsec(), uTickUsecs);
    pState->uNumExpCalls += 1;

    return(iResult);
}


/*F********************************************************************************/
/*!
    \Function _CryptRSAEncryptPublic

    \Description
        Handle the exponentiate calls when using the public key data

    \Input *pState      - crypt state
    \Input iIter        - number of iterations

    \Output
        int32_t         - zero=done, else call again

    \Version 04/11/2017 (eesponda)
*/
/********************************************************************************H*/
static int32_t _CryptRSAEncryptPublic(CryptRSAT *pState, int32_t iIter)
{
    int32_t iCount, iResult;

    for (iCount = 0, iResult = 1; (iCount < iIter) && (iResult > 0); iCount += 1)
    {
        iResult = _CryptRSAExponentiate(pState, &pState->Working.PublicKey.Accumul, &pState->Working.PublicKey.Powerof, &pState->Working.PublicKey.Exponent, &pState->Working.PublicKey.Modulus);
    }
    // if we are done, return encrypted data
    if (iResult == 0)
    {
        CryptBnFinal(&pState->Working.PublicKey.Accumul, pState->EncryptBlock, pState->iKeyModSize);
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _CryptRSAEncryptPrivate

    \Description
        Handle the exponentiate calls when using the private key data which
        using chinese remainder theorum

    \Input *pState      - crypt state
    \Input iIter        - number of iterations

    \Output
        int32_t         - zero=done, else call again

    \Version 04/11/2017 (eesponda)
*/
/********************************************************************************H*/
static int32_t _CryptRSAEncryptPrivate(CryptRSAT *pState, int32_t iIter)
{
    int32_t iCount = 0, iResult = 1;

    if (pState->Working.PrivateKey.eState == CRT_COMPUTE_M1)
    {
        /* compute m1
           m1 = c ^ dP % p */
        for (; (iCount < iIter) && (iResult > 0); iCount += 1)
        {
            iResult = _CryptRSAExponentiate(pState, &pState->Working.PrivateKey.M1, &pState->Working.PrivateKey.PowerofP, &pState->Working.PrivateKey.ExponentP, &pState->Working.PrivateKey.PrimeP);
        }

        if (iResult == 0)
        {
            // reset the state and move to next part of computation
            pState->Working.PrivateKey.eState = CRT_COMPUTE_M2;
            pState->bAccumulOne = TRUE;
            pState->iExpBitIndex = 0;
            iResult = 1;
        }
    }
    if (pState->Working.PrivateKey.eState == CRT_COMPUTE_M2)
    {
        /* compute m2
           m2 = c ^ dQ % q */
        for (; (iCount < iIter) && (iResult > 0); iCount += 1)
        {
            iResult = _CryptRSAExponentiate(pState, &pState->Working.PrivateKey.M2, &pState->Working.PrivateKey.PowerofQ, &pState->Working.PrivateKey.ExponentQ, &pState->Working.PrivateKey.PrimeQ);
        }

        if (iResult == 0)
        {
            CryptBnT Result;

            /* compute h
               h = qInv * (m1 - m2) % p */
            CryptBnInit(&Result, 1);
            CryptBnSubtract(&Result, &pState->Working.PrivateKey.M1, &pState->Working.PrivateKey.M2);
            CryptBnModMultiply(&Result, &Result, &pState->Working.PrivateKey.Coeffecient, &pState->Working.PrivateKey.PrimeP);

            /* compute m
               m = m2 + h * q */
            CryptBnMultiply(&Result, &Result, &pState->Working.PrivateKey.PrimeQ);
            CryptBnAccumulate(&Result, &pState->Working.PrivateKey.M2);

            // return final result
            CryptBnFinal(&Result, pState->EncryptBlock, pState->iKeyModSize);
        }
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _CryptRSAInitEncrypt

    \Description
        Initializes the common encrypt data and data depending on the type
        of key

    \Input *pState      - crypt state

    \Version 04/11/2017 (eesponda)
*/
/********************************************************************************H*/
static void _CryptRSAInitEncrypt(CryptRSAT *pState)
{
    // initialize accumulator to 1 (and remember for first-multiply optimization)
    pState->bAccumulOne = TRUE;

    // convert data to encrypt to native word size we will operate in
    if (!pState->bPrivate)
    {
        CryptBnInitFrom(&pState->Working.PublicKey.Powerof, -1, pState->EncryptBlock, pState->iKeyModSize);
    }
    else
    {
        /* we need to reduce before every operation to get the cipher
        text to less than our modulus in the operations.
        our modular multiply in bn doesn't handle the cases where they
        are larger */
        CryptBnInitFrom(&pState->Working.PrivateKey.PowerofP, -1, pState->EncryptBlock, pState->iKeyModSize);
        CryptBnInitFrom(&pState->Working.PrivateKey.PowerofQ, -1, pState->EncryptBlock, pState->iKeyModSize);
        CryptBnMod(&pState->Working.PrivateKey.PowerofP, &pState->Working.PrivateKey.PrimeP, &pState->Working.PrivateKey.PowerofP, NULL);
        CryptBnMod(&pState->Working.PrivateKey.PowerofQ, &pState->Working.PrivateKey.PrimeQ, &pState->Working.PrivateKey.PowerofQ, NULL);
    }
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CryptRSAInit

    \Description
        Init RSA state.

    \Input *pState      - module state
    \Input *pModulus    - crypto modulus
    \Input iModSize     - size of modulus
    \Input *pExponent   - crypto exponent
    \Input iExpSize     - size of exponent

    \Output
        int32_t         - zero for success, negative for error

    \Notes
        This module supports a max modulus size of 4096 (iModSize=512) and requires
        a minimum size of 1024 (iModSize=64).  The exponent must be between 1
        and 512 bytes in size, inclusive.

    \Version 03/05/03 (jbrookes) Split from protossl
*/
/********************************************************************************H*/
int32_t CryptRSAInit(CryptRSAT *pState, const uint8_t *pModulus, int32_t iModSize, const uint8_t *pExponent, int32_t iExpSize)
{
    int32_t iResult = 0;

    // validate modulus size
    if ((iModSize < 64) || ((iModSize/UCRYPT_SIZE) > CRYPTBN_MAX_WIDTH))
    {
        NetPrintf(("cryptrsa: iModSize of %d is invalid\n", iModSize));
        return(-1);
    }
    // validate exponent size
    if ((iExpSize < 1) || ((iExpSize/UCRYPT_SIZE) > CRYPTBN_MAX_WIDTH))
    {
        NetPrintf(("cryptrsa: iExpSize of %d is invalid\n", iExpSize));
        return(-2);
    }

    // initialize state
    ds_memclr(pState, sizeof(*pState));
    CryptBnInitFrom(&pState->Working.PublicKey.Modulus, -1, pModulus, pState->iKeyModSize = iModSize);
    CryptBnInitFrom(&pState->Working.PublicKey.Exponent, -1, pExponent, iExpSize);

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function CryptRSAInit2

    \Description
        Init RSA state for private key operations

    \Input *pState      - module state
    \Input iModSize     - size of the public key modulus
    \Input *pPrimeP     - factor p of modulus
    \Input *pPrimeQ     - factor q of modulus
    \Input *pExponentP  - inverse of exponent mod p-1
    \Input *pExponentQ  - inverse of exponent mod q-1
    \Input *pCoeffecient- inverse of q mod p
    \Output
        int32_t         - zero for success, negative for error

    \Version 04/11/2017 (eesponda)
*/
/********************************************************************************H*/
int32_t CryptRSAInit2(CryptRSAT *pState, int32_t iModSize, const CryptBinaryObjT *pPrimeP, const CryptBinaryObjT *pPrimeQ, const CryptBinaryObjT *pExponentP, const CryptBinaryObjT *pExponentQ, const CryptBinaryObjT *pCoeffecient)
{
    // initialize state
    ds_memclr(pState, sizeof(*pState));

    // init state
    CryptBnInitFrom(&pState->Working.PrivateKey.PrimeP, -1, pPrimeP->pObjData, pPrimeP->iObjSize);
    CryptBnInitFrom(&pState->Working.PrivateKey.PrimeQ, -1, pPrimeQ->pObjData, pPrimeQ->iObjSize);
    CryptBnInitFrom(&pState->Working.PrivateKey.ExponentP, -1, pExponentP->pObjData, pExponentP->iObjSize);
    CryptBnInitFrom(&pState->Working.PrivateKey.ExponentQ, -1, pExponentQ->pObjData, pExponentQ->iObjSize);
    CryptBnInitFrom(&pState->Working.PrivateKey.Coeffecient, -1, pCoeffecient->pObjData, pCoeffecient->iObjSize);
    pState->iKeyModSize = iModSize;
    pState->bPrivate = TRUE;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function CryptRSAInitMaster

    \Description
        Setup the master shared secret for encryption

    \Input *pState      - module state
    \Input *pMaster     - the master shared secret to encrypt
    \Input iMasterLen   - the length of the master shared secret

    \Version 02/07/2014 (jbrookes) Rewritten to use CryptRand
*/
/********************************************************************************H*/
void CryptRSAInitMaster(CryptRSAT *pState, const uint8_t *pMaster, int32_t iMasterLen)
{
    int32_t iIndex;
    uint32_t uRandom;

    // fill encrypt block with random data
    CryptRandGet(pState->EncryptBlock, pState->iKeyModSize);
    /* As per PKCS1 http://www.emc.com/emc-plus/rsa-labs/pkcs/files/h11300-wp-pkcs-1v2-2-rsa-cryptography-standard.pdf section 7.2.1,
       the pseudo-random padding octets must be non-zero.  Failing to adhere to this restriction (or any other errors in pkcs1 encoding)
       will typically result in a bad_record_mac fatal alert from the remote host.  The following code gets four random bytes as seed
       and uses a simple Knuth RNG to fill in any zero bytes that were originally generated. */
    CryptRandGet((uint8_t *)&uRandom, sizeof(uRandom));
    for (iIndex = 0; iIndex < pState->iKeyModSize; iIndex += 1)
    {
        while (pState->EncryptBlock[iIndex] == 0)
        {
            uRandom = (uRandom * 69069) + 69069;
            pState->EncryptBlock[iIndex] ^= (uint8_t)uRandom;
        }
    }
    // set PKCS public key signature
    pState->EncryptBlock[0] = 0;        // zero pad
    pState->EncryptBlock[1] = 2;        // public key is type 2
    // add data to encrypt block
    iIndex = pState->iKeyModSize - iMasterLen;
    pState->EncryptBlock[iIndex-1] = 0; // zero byte prior to data
    ds_memcpy(pState->EncryptBlock+iIndex, pMaster, iMasterLen);
    // handle initializing the state according to the type of key
    _CryptRSAInitEncrypt(pState);
}

/*F********************************************************************************/
/*!
    \Function CryptRSAInitPrivate

    \Description
        Setup the master shared secret for encryption using PKCS1.5

    \Input *pState      - module state
    \Input *pMaster     - the master shared secret to encrypt
    \Input iMasterLen   - the length of the master shared secret

    \Version 11/03/2013 (jbrookes)
*/
/********************************************************************************H*/
void CryptRSAInitPrivate(CryptRSAT *pState, const uint8_t *pMaster, int32_t iMasterLen)
{
    int32_t iIndex;
    // put in PKCS1.5 signature
    pState->EncryptBlock[0] = 0;
    pState->EncryptBlock[1] = 1;
    // PKCS1.5 signing pads with 0xff
    ds_memset(pState->EncryptBlock+2, 0xff, pState->iKeyModSize-2);
    // add data to encrypt block
    iIndex = pState->iKeyModSize - iMasterLen;
    pState->EncryptBlock[iIndex-1] = 0; // zero byte prior to data
    ds_memcpy(pState->EncryptBlock+iIndex, pMaster, iMasterLen);
    // handle initializing the state according to the type of key
    _CryptRSAInitEncrypt(pState);
}

/*F********************************************************************************/
/*!
    \Function CryptRSAInitSignature

    \Description
        Setup the encrypted signature for decryption.

    \Input *pState   - module state
    \Input *pSig     - the encrypted signature
    \Input iSigLen   - the length of the encrypted signature.

    \Version 03/03/2004 (sbevan)
*/
/********************************************************************************H*/
void CryptRSAInitSignature(CryptRSAT *pState, const uint8_t *pSig, int32_t iSigLen)
{
    ds_memcpy(pState->EncryptBlock, pSig, iSigLen);
    // handle initializing the state according to the type of key
    _CryptRSAInitEncrypt(pState);
}

/*F********************************************************************************/
/*!
    \Function CryptRSAEncrypt

    \Description
        Encrypt data.

    \Input *pState      - module state
    \Input iIter        - number of iterations to execute (zero=do all)

    \Output
        int32_t         - zero=operation complete, else call again

    \Version 03/05/2003 (jbrookes) Split from protossl
*/
/********************************************************************************H*/
int32_t CryptRSAEncrypt(CryptRSAT *pState, int32_t iIter)
{
    if (iIter == 0)
    {
        iIter = 0x7fffffff;
    }

    return((!pState->bPrivate) ? _CryptRSAEncryptPublic(pState, iIter) : _CryptRSAEncryptPrivate(pState, iIter));
}


