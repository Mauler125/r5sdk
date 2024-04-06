/*H********************************************************************************/
/*!
    \File   cryptmont.h

    \Description
        This module implements the math for elliptic curve cryptography
        using montgomery curves

    \Copyright
        Copyright (c) Electronic Arts 2018. ALL RIGHTS RESERVED.
*/
/********************************************************************************H*/

#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/crypt/cryptrand.h"

#include "DirtySDK/crypt/cryptmont.h"

/*** Defines **********************************************************************/

//! size of the window used to determined the table size
#define CRYPTMONT_WINDOW_SIZE (5)

//! calculation of the table size based on the window
#define CRYPTMONT_TABLE_SIZE  (1 << (CRYPTMONT_WINDOW_SIZE - 1))

//! memgroup for allocating the table
#define CRYPTMONT_MEMID       ('mont')

//! number of iterations to do per call
#define CRYPTMONT_NUM_ITERATIONS (0x10)

/*** Variables ********************************************************************/

//! prime for x25519
static const uint8_t _aPrime25519[] =
{
    0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xed
};

//! prime for x25519
static const uint8_t _aPrime448[] =
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _CryptMontSwap

    \Description
        Constant time conditional swap of two big numbers

    \Input uSwap    - determintes if we should swap
    \Input *pLhs    - [out] first big number in the swap
    \Input *pRhs    - [out] second big number in the swap

    \Version 04/11/2018 (eesponda)
*/
/********************************************************************************F*/
static void _CryptMontSwap(uint8_t uSwap, CryptBnT *pLhs, CryptBnT *pRhs)
{
    CryptBnT Temp, Mask;

    // mask = (1 << bits) - swap
    CryptBnInitSet(&Mask, 1);
    CryptBnInitSet(&Temp, uSwap);
    CryptBnLeftShift2(&Mask, DS_MAX(CryptBnBitLen(pLhs), CryptBnBitLen(pRhs)));
    CryptBnSubtract(&Mask, &Mask, &Temp);

    // temp = mask & (lhs ^ rhs)
    CryptBnBitXor(&Temp, pLhs, pRhs);
    CryptBnBitAnd(&Temp, &Temp, &Mask);

    // lhs ^= temp
    CryptBnBitXor(pLhs, pLhs, &Temp);

    // rhs ^= temp
    CryptBnBitXor(pRhs, pRhs, &Temp);
}

/*F********************************************************************************/
/*!
    \Function _CryptMontPointCalculate

    \Description
        Calculates the point multiplication on the curve based on the private key
        before doing the final result calculation

    \Input *pState  - curve state
    \Input *pU      - the point we multiply with the private key

    \Output
        uint8_t     - TRUE=complete, FALSE=pending

    \Version 04/11/2018 (eesponda)
*/
/********************************************************************************F*/
static uint8_t _CryptMontPointCalculate(CryptMontT *pState, CryptBnT *pU)
{
    int32_t iIter;

    // initialize the operation
    if (pState->iBitIndex < 0)
    {
        pState->iBitIndex = CryptBnBitLen(&pState->Prime) - 1;
        pState->uCryptUsecs = 0;

        CryptBnInitSet(&pState->X_2, 1);
        CryptBnInitSet(&pState->Result.X, 0);
        CryptBnClone(&pState->X_3, pU);
        CryptBnInitSet(&pState->Result.Y, 1);
    }

    for (iIter = 0; (pState->iBitIndex >= 0) && (iIter < CRYPTMONT_NUM_ITERATIONS); pState->iBitIndex -= 1, iIter += 1)
    {
        CryptBnT A, AA, B, BB, C, D, DA, CB, E;
        uint8_t bBitSet = CryptBnBitTest(&pState->PrivateKey, pState->iBitIndex);
        const uint64_t uTickUsecs = NetTickUsec();

        CryptBnInitSet(&A, 0);
        CryptBnInitSet(&B, 0);
        CryptBnInitSet(&C, 0);
        CryptBnInitSet(&D, 0);
        CryptBnInitSet(&E, 0);

        // swap ^= bitset;
        pState->uSwap ^= bBitSet;

        // constant time conditional swap
        _CryptMontSwap(pState->uSwap, &pState->X_2, &pState->X_3);
        _CryptMontSwap(pState->uSwap, &pState->Result.X, &pState->Result.Y);

        // swap = bitset
        pState->uSwap = bBitSet;

        // A = (x_2 + z_2) % p
        CryptBnModAdd(&A, &pState->X_2, &pState->Result.X, &pState->Prime);

        // AA = (A * A) % p
        CryptBnModMultiply(&AA, &A, &A, &pState->Prime);

        // B = (x_2 - z_2)
        CryptBnSubtract(&B, &pState->X_2, &pState->Result.X);

        // BB = (B * B) % p
        CryptBnModMultiply(&BB, &B, &B, &pState->Prime);

        // E = (AA - BB)
        CryptBnSubtract(&E, &AA, &BB);

        // x_2 = (AA * BB) % p
        CryptBnModMultiply(&pState->X_2, &AA, &BB, &pState->Prime);

        // z_2 = (E * (AA + (a24 * E))) % p
        CryptBnModMultiply(&pState->Result.X, &pState->A24, &E, &pState->Prime);
        CryptBnModAdd(&pState->Result.X, &AA, &pState->Result.X, &pState->Prime);
        CryptBnModMultiply(&pState->Result.X, &E, &pState->Result.X, &pState->Prime);

        // C = (x_3 + z_3) % p
        CryptBnModAdd(&C, &pState->X_3, &pState->Result.Y, &pState->Prime);

        // D = x_3 - z_3
        CryptBnSubtract(&D, &pState->X_3, &pState->Result.Y);

        // DA = (D * A) % p
        CryptBnModMultiply(&DA, &D, &A, &pState->Prime);

        // CB = (C * B) % p
        CryptBnModMultiply(&CB, &C, &B, &pState->Prime);

        // x_3 = (DA+CB)^2 % p
        CryptBnModAdd(&pState->X_3, &DA, &CB, &pState->Prime);
        CryptBnModMultiply(&pState->X_3, &pState->X_3, &pState->X_3, &pState->Prime);

        // z_3 = (u * ((DA-CB)^2 % p)) % p
        CryptBnSubtract(&pState->Result.Y, &DA, &CB);
        CryptBnModMultiply(&pState->Result.Y, &pState->Result.Y, &pState->Result.Y, &pState->Prime);
        CryptBnModMultiply(&pState->Result.Y, pU, &pState->Result.Y, &pState->Prime);

        // update timing
        pState->uCryptUsecs += (uint32_t)NetTickDiff(NetTickUsec(), uTickUsecs);
    }

    // check for completion of first stage, perform required operations and move to next
    if (pState->iBitIndex < 0)
    {
        // constant time conditional swap
        _CryptMontSwap(pState->uSwap, &pState->X_2, &pState->X_3);
        _CryptMontSwap(pState->uSwap, &pState->Result.X, &pState->Result.Y);
    }

    return((pState->iBitIndex < 0) ? TRUE : FALSE);
}

/*F********************************************************************************/
/*!
    \Function _CryptMontResultCalculate

    \Description
        Determines the final using the final result using the formula:
        result = x_2 * (z_2 ^ (p - 2) % p) % p

    \Input *pState  - curve state

    \Output
        uint8_t     - TRUE=complete, FALSE=pending

    \Version 04/11/2018 (eesponda)
*/
/********************************************************************************F*/
static uint8_t _CryptMontResultCalculate(CryptMontT *pState)
{
    int32_t iIter;

    // initialize the operation
    if (pState->iBitIndex < 0)
    {
        int32_t iTableIndex;
        CryptBnT Square;

        // query memgroup
        DirtyMemGroupQuery(&pState->iMemGroup, &pState->pMemGroupUserdata);

        if ((pState->pTable = (CryptBnT *)DirtyMemAlloc(sizeof(*pState->pTable) * CRYPTMONT_TABLE_SIZE, CRYPTMONT_MEMID, pState->iMemGroup, pState->pMemGroupUserdata)) == NULL)
        {
            NetPrintf(("cryptmont: failed to allocate window table\n"));
            return(TRUE);
        }
        ds_memclr(pState->pTable, sizeof(*pState->pTable) * CRYPTMONT_TABLE_SIZE);

        /* put the already reduced input into the first index of the table, this will make it so further
           entries in the table will not be too large for our modulus operations */
        CryptBnClone(&pState->pTable[0], &pState->Result.X);

        // calculate (input^2) % mod as the basis for the other calculations
        CryptBnModMultiply(&Square, &pState->pTable[0], &pState->pTable[0], &pState->Prime);

        // calculate (input^(2+iTableIndex)) % mod for the rest of the table
        for (iTableIndex = 1; iTableIndex < CRYPTMONT_TABLE_SIZE; iTableIndex += 1)
        {
            CryptBnModMultiply(&pState->pTable[iTableIndex], &pState->pTable[iTableIndex - 1], &Square, &pState->Prime);
        }

        pState->bAccumulOne = TRUE;

        // save prime - 2
        CryptBnInitSet(&pState->PrimeMin2, 2);
        CryptBnSubtract(&pState->PrimeMin2, &pState->Prime, &pState->PrimeMin2);

        pState->iBitIndex = CryptBnBitLen(&pState->PrimeMin2) - 1;
    }

    for (iIter = 0; (pState->iBitIndex >= 0) && (iIter < CRYPTMONT_NUM_ITERATIONS); iIter += 1)
    {
        const uint64_t uTickUsecs = NetTickUsec();

        /* scan backwards from the current exponent bit until a set bit is found to denote the start of the window
           squaring the results until such a bit is found */
        if (CryptBnBitTest(&pState->PrimeMin2, pState->iBitIndex))
        {
            int32_t iWindowBit, iWindowValue, iWindowEnd;

            /* scan backwards from the start of the window until the last set bit in the range of the window size is found which denotes the end bit index of the window
               calculate the value of the window that will be used when multiplying against our precomputed table we skip the first bit as we know it is set based on the
               current exponent bit check we make right above */
            for (iWindowBit = 1, iWindowValue = 1, iWindowEnd = 0; (iWindowBit < CRYPTMONT_WINDOW_SIZE) && ((pState->iBitIndex - iWindowBit) >= 0); iWindowBit += 1)
            {
                if (CryptBnBitTest(&pState->PrimeMin2, pState->iBitIndex - iWindowBit))
                {
                    iWindowValue <<= (iWindowBit - iWindowEnd);
                    iWindowValue  |= 1; /* force odd */
                    iWindowEnd = iWindowBit;
                }
            }

            // square for all the bits in the window and moving the exponent bit index down each time
            for (iWindowBit = 0; iWindowBit < iWindowEnd + 1; iWindowBit += 1, pState->iBitIndex -= 1)
            {
                CryptBnModMultiply(&pState->Result.X, &pState->Result.X, &pState->Result.X, &pState->Prime);
            }

            // skip the first multiply
            if (!pState->bAccumulOne)
            {
                CryptBnModMultiply(&pState->Result.X, &pState->Result.X, &pState->pTable[iWindowValue/2], &pState->Prime);
            }
            else
            {
                CryptBnClone(&pState->Result.X, &pState->pTable[iWindowValue/2]);
                pState->bAccumulOne = FALSE;
            }
        }
        else
        {
            CryptBnModMultiply(&pState->Result.X, &pState->Result.X, &pState->Result.X, &pState->Prime);
            pState->iBitIndex -= 1;
        }

        // update timing
        pState->uCryptUsecs += (uint32_t)NetTickDiff(NetTickUsec(), uTickUsecs);
    }

    // calculate the final result
    if (pState->iBitIndex < 0)
    {
        DirtyMemFree(pState->pTable, CRYPTMONT_MEMID, pState->iMemGroup, pState->pMemGroupUserdata);
        pState->pTable = NULL;

        CryptBnModMultiply(&pState->Result.X, &pState->X_2, &pState->Result.X, &pState->Prime);
    }

    return((pState->iBitIndex < 0) ? TRUE : FALSE);
}

/*F********************************************************************************/
/*!
    \Function _CryptMontInit25519PrivateKey

    \Description
        Initializes the private key based on the requirements for this curve
        (x25519)

    \Input *pPrivateKey - [out] private key state
    \Input *pK          - private key buffer or NULL if we should generate one

    \Version 04/11/2018 (eesponda)
*/
/********************************************************************************F*/
static void _CryptMontInit25519PrivateKey(CryptBnT *pPrivateKey, const uint8_t *pK)
{
    uint8_t aSecret[32];

    /* per: https://tools.ietf.org/html/rfc7748#section-5
       For X25519, in order to decode 32 random bytes as an integer scalar, set the three least significant bits of the first byte and
       the most significant bit of the last to zero, set the second most significant bit of the last byte to 1 and, finally, decode as
       little-endian.  This means that the resulting integer is of the form 2^254 plus eight times a value between 0 and 2^251 - 1 (inclusive) */

    // retrieve the random bytes if not provided one
    if (pK == NULL)
    {
        CryptRandGet(aSecret, sizeof(aSecret));
    }
    else
    {
        ds_memcpy(aSecret, pK, sizeof(aSecret));
    }

    aSecret[0]  &= 248;
    aSecret[31] &= 127;
    aSecret[31] |= 64;
    CryptBnInitLeFrom(pPrivateKey, aSecret, sizeof(aSecret));
}

/*F********************************************************************************/
/*!
    \Function _CryptMontInit448PrivateKey

    \Description
        Initializes the private key based on the requirements for this curve
        (x448)

    \Input *pPrivateKey - [out] private key state
    \Input *pK          - private key buffer or NULL if we should generate one

    \Version 04/11/2018 (eesponda)
*/
/********************************************************************************F*/
static void _CryptMontInit448PrivateKey(CryptBnT *pPrivateKey, const uint8_t *pK)
{
    uint8_t aSecret[56];

    /* per: https://tools.ietf.org/html/rfc7748#section-5
       Likewise, for X448, set the two least significant bits of the first byte to 0, and the most significant bit of the last byte to 1.  This
       means that the resulting integer is of the form 2^447 plus four times a value between 0 and 2^445 - 1 (inclusive). */

    if (pK == NULL)
    {
        CryptRandGet(aSecret, sizeof(aSecret));
    }
    else
    {
        ds_memcpy(aSecret, pK, sizeof(aSecret));
    }

    aSecret[0]  &= 252;
    aSecret[55] |= 128;
    CryptBnInitLeFrom(pPrivateKey, aSecret, sizeof(aSecret));
}

/*** Public Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function CryptMontInit

    \Description
        Initializes the curve given an identifier

    \Input *pState      - curve state we are initializing
    \Input iCurveType   - the curve identifier (CRYPTMONT_CURVE_*)

    \Output
        int32_t         - 0=success, negative=failure

    \Version 04/11/2018 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptMontInit(CryptMontT *pState, int32_t iCurveType)
{
    // init state
    ds_memclr(pState, sizeof(*pState));
    pState->iBitIndex = -1;
    pState->iCurveType = iCurveType;

    if (iCurveType == CRYPTCURVE_X25519)
    {
        // save prime
        CryptBnInitFrom(&pState->Prime, -1, _aPrime25519, sizeof(_aPrime25519));
        // save a24
        CryptBnInitSet(&pState->A24, 121665);
        // save u
        CryptBnInitSet(&pState->BasePoint, 9);
        // get k
        _CryptMontInit25519PrivateKey(&pState->PrivateKey, NULL);
    }
    else if (iCurveType == CRYPTCURVE_X448)
    {
        // save prime
        CryptBnInitFrom(&pState->Prime, -1, _aPrime448, sizeof(_aPrime448));
        // save a24
        CryptBnInitSet(&pState->A24, 39081);
        // save u
        CryptBnInitSet(&pState->BasePoint, 5);
        // get k
        _CryptMontInit448PrivateKey(&pState->PrivateKey, NULL);
    }
    else
    {
        NetPrintf(("cryptmont: unrecognized curve type (%d) passed to init\n", iCurveType));
        return(-1);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function CryptMontSetPrivateKey

    \Description
        Sets our internal private key for verifying our test vectors based
        on the state's curve type

    \Input *pState      - curve state
    \Input *pKey        - the private key buffer

    \Notes
        This is for testing purposes only

    \Version 04/11/2018 (eesponda)
*/
/********************************************************************************F*/
void CryptMontSetPrivateKey(CryptMontT *pState, const uint8_t *pKey)
{
    #if DIRTYCODE_DEBUG
    if (pState->iCurveType == CRYPTCURVE_X25519)
    {
        _CryptMontInit25519PrivateKey(&pState->PrivateKey, pKey);
    }
    else if (pState->iCurveType == CRYPTCURVE_X448)
    {
        _CryptMontInit448PrivateKey(&pState->PrivateKey, pKey);
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function CryptMontPublic

    \Description
        Generates our public key by doing our PrivateKey * BasePoint on the curve

    \Input *pState      - curve state
    \Input *pResult     - [out] result output (optional)
    \Input *pCryptUsecs - [out] timing information for the operation (optional)

    \Output
        int32_t         - zero=success, otherwise=pending

    \Notes
        If pResult is NULL, the result can be pulled from CryptMontT.Result

    \Version 04/11/2018 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptMontPublic(CryptMontT *pState, CryptEccPointT *pResult, uint32_t *pCryptUsecs)
{
    int32_t iResult = 1;

    if ((pState->eState == CRYPTMONT_COMPUTE_POINT) && (_CryptMontPointCalculate(pState, &pState->BasePoint)))
    {
        // switch state on completion
        pState->eState = CRYPTMONT_COMPUTE_EXP;
    }
    else if ((pState->eState == CRYPTMONT_COMPUTE_EXP) && (_CryptMontResultCalculate(pState)))
    {
        // reset back to original state in case they want to perform a different operation
        pState->eState = CRYPTMONT_COMPUTE_POINT;

        // copy the timing if passed in
        if (pCryptUsecs != NULL)
        {
            *pCryptUsecs = pState->uCryptUsecs;
        }
        // signal completion
        if (pResult != NULL)
        {
            CryptBnClone(&pResult->X, &pState->Result.X);
        }
        iResult = 0;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function CryptMontSecret

    \Description
        Generates our shared secret by doing our PrivateKey * PublicKey on the curve

    \Input *pState      - curve state
    \Input *pPublicKey  - the peer's public key
    \Input *pResult     - [out] result output (optional)
    \Input *pCryptUsecs - [out] timing information for the operation (optional)

    \Output
        int32_t         - zero=success, otherwise=pending

    \Notes
        If pResult is NULL, the result can be pulled from CryptMontT.Result

    \Version 04/11/2018 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptMontSecret(CryptMontT *pState, CryptEccPointT *pPublicKey, CryptEccPointT *pResult, uint32_t *pCryptUsecs)
{
    int32_t iResult = 1;

    if ((pState->eState == CRYPTMONT_COMPUTE_POINT) && (_CryptMontPointCalculate(pState, &pPublicKey->X)))
    {
        // switch state on completion
        pState->eState = CRYPTMONT_COMPUTE_EXP;
    }
    else if ((pState->eState == CRYPTMONT_COMPUTE_EXP) && (_CryptMontResultCalculate(pState)))
    {
        // reset back to original state in case they want to perform a different operation
        pState->eState = CRYPTMONT_COMPUTE_POINT;

        // copy the timing if passed in
        if (pCryptUsecs != NULL)
        {
            *pCryptUsecs = pState->uCryptUsecs;
        }
        // signal completion
        if (pResult != NULL)
        {
            CryptBnClone(&pResult->X, &pState->Result.X);
        }
        iResult = 0;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function CryptMontPointInitFrom

    \Description
        Initializes our point representation given a buffer

    \Input *pPoint  - point state
    \Input *pBuffer - the buffer we are copying from
    \Input iBufSize - size of the buffer

    \Output
        int32_t     - zero=success, otherwise=failure

    \Notes
        These curves work in little endian so need to copy different functions
        from our nist curves.

    \Version 04/11/2018 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptMontPointInitFrom(CryptEccPointT *pPoint, const uint8_t *pBuffer, int32_t iBufSize)
{
    CryptBnInitLeFrom(&pPoint->X, pBuffer, iBufSize);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function CryptMontPointFinal

    \Description
        Copies our point data into an output buffer

    \Input *pState  - curve state
    \Input *pPoint  - point state or NULL to use curve result
    \Input bSecret  - is this the shared secret?
    \Input *pBuffer - [out] the buffer we are copying into
    \Input iBufSize - size of the buffer

    \Output
        int32_t     - number of bytes encoded into the buffer

    \Notes
        These curves work in little endian so need to copy different functions
        from our nist curves.

        The bSecret is unused here but is left for compatibility with the other
        curves' API.

    \Version 04/11/2018 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptMontPointFinal(const CryptMontT *pState, const CryptEccPointT *pPoint, uint8_t bSecret, uint8_t *pBuffer, int32_t iBufSize)
{
    // if point not provided assume we are using the result
    if ((pState != NULL) && (pPoint == NULL))
    {
        pPoint = &pState->Result;
    }

    CryptBnFinalLe(&pPoint->X, pBuffer, iBufSize);
    return(CryptBnByteLen(&pPoint->X));
}
