/*H********************************************************************************/
/*!
    \File   cryptnist.c

    \Description
        This module implements the math for elliptic curve cryptography
        using curves in short Weierstrass form (NIST curves)

    \Copyright
        Copyright (c) Electronic Arts 2017. ALL RIGHTS RESERVED.
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/crypt/crypthash.h"
#include "DirtySDK/crypt/crypthmac.h"
#include "DirtySDK/crypt/cryptrand.h"
#include "DirtySDK/crypt/cryptnist.h"

/*** Defines **********************************************************************/

//! the marker that marks uncompressed point format
#define CRYPTNIST_UNCOMPRESSED_MARKER (0x04)

//! number of iterations to do per call
#if !defined(CRYPTNIST_NUM_ITERATIONS)
    #define CRYPTNIST_NUM_ITERATIONS (0x10)
#endif

//! memgroup for allocating the table
#define CRYPTNIST_MEMID ('nist')

/*** Type Definitions *************************************************************/

//! defintion of a curve, basis of initialization into big numbers
typedef struct NistCurveT
{
    int32_t iIdent;
    int32_t iSize;
    uint8_t aPrime[48];
    uint8_t aCoefficientA[48];
    uint8_t aCoefficientB[48];
    uint8_t aBasePoint[97];
    uint8_t aOrder[48];
} NistCurveT;

/*** Variables ********************************************************************/

//! used to easily checked for default points
static const CryptEccPointT _Zero = { { { 0x0 }, 1, 0 }, { { 0x0 }, 1, 0 } };

//! the curves we support
static const NistCurveT _aEccCurves[] =
{
    {
        CRYPTCURVE_SECP256R1, 32,
        { 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC },
        { 0x5A, 0xC6, 0x35, 0xD8, 0xAA, 0x3A, 0x93, 0xE7, 0xB3, 0xEB, 0xBD, 0x55, 0x76, 0x98, 0x86, 0xBC, 0x65, 0x1D, 0x06, 0xB0, 0xCC, 0x53, 0xB0, 0xF6, 0x3B, 0xCE, 0x3C, 0x3E, 0x27, 0xD2, 0x60, 0x4B },
        { 0x04, 0x6B, 0x17, 0xD1, 0xF2, 0xE1, 0x2C, 0x42, 0x47, 0xF8, 0xBC, 0xE6, 0xE5, 0x63, 0xA4, 0x40, 0xF2, 0x77, 0x03, 0x7D, 0x81, 0x2D, 0xEB, 0x33, 0xA0, 0xF4, 0xA1, 0x39, 0x45, 0xD8, 0x98, 0xC2, 0x96,
          0x4F, 0xE3, 0x42, 0xE2, 0xFE, 0x1A, 0x7F, 0x9B, 0x8E, 0xE7, 0xEB, 0x4A, 0x7C, 0x0F, 0x9E, 0x16, 0x2B, 0xCE, 0x33, 0x57, 0x6B, 0x31, 0x5E, 0xCE, 0xCB, 0xB6, 0x40, 0x68, 0x37, 0xBF, 0x51, 0xF5 },
        { 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84, 0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51 }
    },
    {
        CRYPTCURVE_SECP384R1, 48,
        { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
          0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
          0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFC },
        { 0xB3, 0x31, 0x2F, 0xA7, 0xE2, 0x3E, 0xE7, 0xE4, 0x98, 0x8E, 0x05, 0x6B, 0xE3, 0xF8, 0x2D, 0x19, 0x18, 0x1D, 0x9C, 0x6E, 0xFE, 0x81, 0x41, 0x12, 0x03, 0x14, 0x08, 0x8F, 0x50, 0x13, 0x87, 0x5A,
          0xC6, 0x56, 0x39, 0x8D, 0x8A, 0x2E, 0xD1, 0x9D, 0x2A, 0x85, 0xC8, 0xED, 0xD3, 0xEC, 0x2A, 0xEF },
        { 0x04, 0xAA, 0x87, 0xCA, 0x22, 0xBE, 0x8B, 0x05, 0x37, 0x8E, 0xB1, 0xC7, 0x1E, 0xF3, 0x20, 0xAD, 0x74, 0x6E, 0x1D, 0x3B, 0x62, 0x8B, 0xA7, 0x9B, 0x98, 0x59, 0xF7, 0x41, 0xE0, 0x82, 0x54, 0x2A, 0x38,
          0x55, 0x02, 0xF2, 0x5D, 0xBF, 0x55, 0x29, 0x6C, 0x3A, 0x54, 0x5E, 0x38, 0x72, 0x76, 0x0A, 0xB7, 0x36, 0x17, 0xDE, 0x4A, 0x96, 0x26, 0x2C, 0x6F, 0x5D, 0x9E, 0x98, 0xBF, 0x92, 0x92, 0xDC, 0x29,
          0xF8, 0xF4, 0x1D, 0xBD, 0x28, 0x9A, 0x14, 0x7C, 0xE9, 0xDA, 0x31, 0x13, 0xB5, 0xF0, 0xB8, 0xC0, 0x0A, 0x60, 0xB1, 0xCE, 0x1D, 0x7E, 0x81, 0x9D, 0x7A, 0x43, 0x1D, 0x7C, 0x90, 0xEA, 0x0E, 0x5F },
        { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC7, 0x63, 0x4D, 0x81, 0xF4, 0x37, 0x2D, 0xDF,
          0x58, 0x1A, 0x0D, 0xB2, 0x48, 0xB0, 0xA7, 0x7A, 0xEC, 0xEC, 0x19, 0x6A, 0xCC, 0xC5, 0x29, 0x73 }
    }
};

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _CryptNistPointCalculate

    \Description
        Does the math for doubling / adding points given they you have calculated
        S over the curve's prime

    \Input *pState  - curve state
    \Input *pP      - point P
    \Input *pQ      - point Q
    \Input *pS      - S used in calculation
    \Input *pResult - [out] point R that is calculation of P+Q

    \Version 02/17/2017 (eesponda)
*/
/********************************************************************************F*/
static void _CryptNistPointCalculate(CryptNistT *pState, CryptEccPointT *pP, CryptEccPointT *pQ, CryptBnT *pS, CryptEccPointT *pResult)
{
    CryptEccPointT Result;

    // X = S² - P.X - Q.X
    CryptBnInitSet(&Result.X, 0);
    CryptBnModMultiply(&Result.X, pS, pS, &pState->Prime);
    CryptBnSubtract(&Result.X, &Result.X, &pP->X);
    CryptBnSubtract(&Result.X, &Result.X, &pQ->X);

    // Y = S * (P.X - X) - P.Y
    CryptBnInitSet(&Result.Y, 0);
    CryptBnSubtract(&Result.Y, &pP->X, &Result.X);
    CryptBnModMultiply(&Result.Y, &Result.Y, pS, &pState->Prime);
    CryptBnSubtract(&Result.Y, &Result.Y, &pP->Y);

    // X %= Curve.Prime
    CryptBnMod(&Result.X, &pState->Prime, &Result.X, NULL);
    // Y %= Curve.Prime
    CryptBnMod(&Result.Y, &pState->Prime, &Result.Y, NULL);

    // write out the result
    ds_memcpy(pResult, &Result, sizeof(*pResult));
}

/*F********************************************************************************/
/*!
    \Function _CryptNistPointAddition

    \Description
        Adds two points together

    \Input *pState  - curve state
    \Input *pPoint1 - first point in addition
    \Input *pPoint2 - second point in addtion
    \Input *pResult - [out] point R that is result of addition

    \Version 02/17/2017 (eesponda)
*/
/********************************************************************************F*/
static void _CryptNistPointAddition(CryptNistT *pState, CryptEccPointT *pPoint1, CryptEccPointT *pPoint2, CryptEccPointT *pResult)
{
    if (memcmp(pPoint1, &_Zero, sizeof(*pPoint1)) == 0)
    {
        ds_memcpy(pResult, pPoint2, sizeof(*pResult));
    }
    else if (memcmp(pPoint2, &_Zero, sizeof(*pPoint2)) == 0)
    {
        ds_memcpy(pResult, pPoint1, sizeof(*pResult));
    }
    else
    {
        CryptBnT A, B;

        CryptBnInitSet(&A, 0);
        CryptBnInitSet(&B, 0);

        // A = y1 - y2
        CryptBnSubtract(&A, &pPoint1->Y, &pPoint2->Y);

        // B = (x1 - x2)⁻¹ % Curve.Prime [Inverse Modulus]
        CryptBnSubtract(&B, &pPoint1->X, &pPoint2->X);
        CryptBnInverseMod(&B, &pState->Prime);

        // A *= B
        CryptBnModMultiply(&A, &A, &B, &pState->Prime);

        // calculate the result
        _CryptNistPointCalculate(pState, pPoint1, pPoint2, &A, pResult);
    }
}

/*F********************************************************************************/
/*!
    \Function _CryptNistPointDouble

    \Description
        Doubles a point

    \Input *pState  - curve state
    \Input *pPoint  - point that we are doubling
    \Input *pResult - [out] result of the doubling

    \Version 02/17/2017 (eesponda)
*/
/********************************************************************************F*/
static void _CryptNistPointDouble(CryptNistT *pState, CryptEccPointT *pPoint, CryptEccPointT *pResult)
{
    CryptBnT A, B;

    // if attempting to double zero, return
    if (memcmp(pPoint, &_Zero, sizeof(*pPoint)) == 0)
    {
        return;
    }

    // A = 3 * x * x + a
    CryptBnInitSet(&A, 3);
    CryptBnMultiply(&A, &A, &pPoint->X);
    CryptBnMultiply(&A, &A, &pPoint->X);
    CryptBnAccumulate(&A, &pState->CoefficientA);

    // B = (2 * y)⁻¹ % Curve.Prime [Inverse Modulus]
    CryptBnClone(&B, &pPoint->Y);
    CryptBnLeftShift(&B);
    CryptBnInverseMod(&B, &pState->Prime);

    // A *= B
    CryptBnModMultiply(&A, &A, &B, &pState->Prime);

    // calculate the result
    _CryptNistPointCalculate(pState, pPoint, pPoint, &A, pResult);
}

/*F********************************************************************************/
/*!
    \Function _CryptNistDoubleAndAdd

    \Description
        Performs the scalar multiplication using double and add
        operations using a sliding window for the private key

    \Input *pState      - curve state
    \Input *pInput      - point we are multiplying by the private key
    \Input *pPrivateKey - private key was using to multiply
    \Input *pResult     - [out] result of the operation

    \Output
        int32_t         - zero=success, otherwise=pending

    \Notes
        Adapted from the sliding window algorithm used for exponentiation
        in RSA
        See Handbook of Applied Cryptography Chapter 14.6.1 (14.85):
        http://cacr.uwaterloo.ca/hac/about/chap14.pdf

    \Version 05/08/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _CryptNistDoubleAndAdd(CryptNistT *pState, CryptEccPointT *pInput, CryptBnT *pPrivateKey, CryptEccPointT *pResult)
{
    int32_t iIter = CRYPTNIST_NUM_ITERATIONS;

    do
    {
        const uint64_t uTick = NetTickUsec();

        if (pState->iKeyIndex < 0)
        {
            int32_t iTableIndex;
            CryptEccPointT Double;

            // init working state
            ds_memcpy(pResult, &_Zero, sizeof(*pResult));

            // query memgroup
            DirtyMemGroupQuery(&pState->iMemGroup, &pState->pMemGroupUserdata);

            if ((pState->pTable = (CryptEccPointT *)DirtyMemAlloc(sizeof(*pState->pTable) * CRYPTNIST_TABLE_SIZE, CRYPTNIST_MEMID, pState->iMemGroup, pState->pMemGroupUserdata)) == NULL)
            {
                NetPrintf(("cryptnist: failed to allocate window table\n"));
                return(0);
            }
            ds_memclr(pState->pTable, sizeof(*pState->pTable) * CRYPTNIST_TABLE_SIZE);

            /* put the input into the first
               index of the table, this will make it so further
               entries in the table will not be too large
               for our modulus operations */
            ds_memcpy(&pState->pTable[0], pInput, sizeof(pState->pTable[0]));

            // calculate doubled input as the basis for the other calculations
            _CryptNistPointDouble(pState, pInput, &Double);

            // calculate the further added points for the rest of the table
            for (iTableIndex = 1; iTableIndex < CRYPTNIST_TABLE_SIZE; iTableIndex += 1)
            {
                _CryptNistPointAddition(pState, &pState->pTable[iTableIndex - 1], &Double, &pState->pTable[iTableIndex]);
            }

            // set the defaults
            pState->iKeyIndex = CryptBnBitLen(pPrivateKey) - 1;

            // clear the profiling information
            pState->uCryptUSecs = 0;
        }

        /* scan backwards from the current private key bit
           until a set bit is found to denote the start of the window
           doubling the results until such a bit is found */
        if (CryptBnBitTest(pPrivateKey, pState->iKeyIndex))
        {
            int32_t iWindowBit, iWindowValue, iWindowEnd;

            /* scan backwards from the start of the window until the last set bit in the range of the window size is found which denotes the end bit index of the window
               calculate the value of the window that will be used when adding against our precomputed table.
               we skip the first bit as we know it is set based on the current private key bit check we make right above */
            for (iWindowBit = 1, iWindowValue = 1, iWindowEnd = 0; (iWindowBit < CRYPTNIST_WINDOW_SIZE) && ((pState->iKeyIndex - iWindowBit) >= 0); iWindowBit += 1)
            {
                if (CryptBnBitTest(pPrivateKey, pState->iKeyIndex - iWindowBit))
                {
                    iWindowValue <<= (iWindowBit - iWindowEnd);
                    iWindowValue  |= 1; // force odd
                    iWindowEnd = iWindowBit;
                }
            }

            // double for each bits in the window and moving the private key bit index down each time
            for (iWindowBit = 0; iWindowBit < iWindowEnd + 1; iWindowBit += 1, pState->iKeyIndex -= 1)
            {
                _CryptNistPointDouble(pState, pResult, pResult);
            }

            // use the window value to get which precomputed power we need to multiply
            _CryptNistPointAddition(pState, pResult, &pState->pTable[iWindowValue/2], pResult);
        }
        else
        {
            _CryptNistPointDouble(pState, pResult, pResult);
            pState->iKeyIndex -= 1;
        }

        // update profiler information
        pState->uCryptUSecs += NetTickDiff(NetTickUsec(), uTick);
    }
    while ((pState->iKeyIndex >= 0) && (--iIter > 0));

    // cleanup memory when done
    if (pState->iKeyIndex < 0)
    {
        DirtyMemFree(pState->pTable, CRYPTNIST_MEMID, pState->iMemGroup, pState->pMemGroupUserdata);
        pState->pTable = NULL;
    }

    return(pState->iKeyIndex >= 0);
}

/*F********************************************************************************/
/*!
    \Function _CryptNistDoubleMultiply

    \Description
        Uses Shamir's Trick to do double point multiplication:
        (pInput1 * pPrivateKey1) + (pInput2 * pPrivateKey2)

    \Input *pState      - curve state
    \Input *pInput1     - point we are multiplying by pMul1
    \Input *pMul1       - big number we are multiplying by pInput1
    \Input *pInput2     - point we are multiplying by pMul2
    \Input *pMul2       - big number we are multiplying by pInput2
    \Input *pResult     - [out] result of the operation

    \Output
        int32_t         - zero=success, otherwise=pending

    \Notes
        For more information on the algorithm, see the Double-scalar
        multiplication slides: http://www.lirmm.fr/~imbert/talks/laurent_Asilomar_08.pdf

    \Version 05/15/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _CryptNistDoubleMultiply(CryptNistT *pState, CryptEccPointT *pInput1, CryptBnT *pMul1, CryptEccPointT *pInput2, CryptBnT *pMul2, CryptEccPointT *pResult)
{
    int32_t iIter = CRYPTNIST_NUM_ITERATIONS;

    do
    {
        const uint64_t uTick = NetTickUsec();
        uint8_t bLHBitSet, bRHBitSet;

        if (pState->iKeyIndex < 0)
        {
            DirtyMemGroupQuery(&pState->iMemGroup, &pState->pMemGroupUserdata);

            if ((pState->pTable = (CryptEccPointT *)DirtyMemAlloc(sizeof(*pState->pTable), CRYPTNIST_MEMID, pState->iMemGroup, pState->pMemGroupUserdata)) == NULL)
            {
                NetPrintf(("cryptnist: failed to allocate window table\n"));
                return(0);
            }
            ds_memclr(pState->pTable, sizeof(*pState->pTable));

            // precompute pInput1 + pInput2
            _CryptNistPointAddition(pState, pInput1, pInput2, &pState->pTable[0]);

            // init working state
            ds_memcpy(pResult, &_Zero, sizeof(*pResult));

            // set the maximum bit size
            pState->iKeyIndex = DS_MAX(CryptBnBitLen(pMul1), CryptBnBitLen(pMul2)) - 1;

            // clear the profiling information
            pState->uCryptUSecs = 0;
        }

        // double for each column
        _CryptNistPointDouble(pState, pResult, pResult);

        // check the bit set in the column
        bLHBitSet = CryptBnBitTest(pMul1, pState->iKeyIndex);
        bRHBitSet = CryptBnBitTest(pMul2, pState->iKeyIndex);

        // if only left hand, then add by that side
        if ((bLHBitSet == TRUE) && (bRHBitSet == FALSE))
        {
            _CryptNistPointAddition(pState, pResult, pInput1, pResult);
        }
        // if only right hand, then add by that side
        else if ((bLHBitSet == FALSE) && (bRHBitSet == TRUE))
        {
            _CryptNistPointAddition(pState, pResult, pInput2, pResult);
        }
        // if both sides, then add by the sum of both
        else if ((bLHBitSet == TRUE) && (bRHBitSet == TRUE))
        {
            _CryptNistPointAddition(pState, pResult, &pState->pTable[0], pResult);
        }

        // update profiler information
        pState->uCryptUSecs += NetTickDiff(NetTickUsec(), uTick);
    }
    while ((--pState->iKeyIndex >= 0) && (--iIter > 0));

    // cleanup table memory
    if (pState->iKeyIndex < 0)
    {
        DirtyMemFree(pState->pTable, CRYPTNIST_MEMID, pState->iMemGroup, pState->pMemGroupUserdata);
        pState->pTable = NULL;
    }

    return(pState->iKeyIndex >= 0);
}

/*F********************************************************************************/
/*!
    \Function _CryptNistRand

    \Description
        Generates a random number in the range [1, curve.order-1]

    \Input *pState  - curve state
    \Input *pResult - [out] random number generated

    \Version 05/15/2017 (eesponda)
*/
/********************************************************************************F*/
static void _CryptNistRand(const CryptNistT *pState, CryptBnT *pResult)
{
    uint8_t aSecret[48];
    CryptBnT Temp;

    // generate a secret to the specified size
    CryptRandGet(aSecret, sizeof(aSecret));
    CryptBnInitFrom(pResult, -1, aSecret, DS_MIN((int32_t)sizeof(aSecret), pState->iSize));

    // clone order so we don't modify
    CryptBnClone(&Temp, &pState->Order);

    // random number = (rand() % order-1 + 1)
    CryptBnDecrement(&Temp);
    CryptBnMod(pResult, &Temp, pResult, NULL);
    CryptBnIncrement(pResult);
}

/*F********************************************************************************/
/*!
    \Function _CryptNistDsaInitHash

    \Description
        Initializes the hash, truncating if necessary

    \Input *pState      - curve state
    \Input *pHashData   - the buffer with the hash digest
    \Input iHashSize    - size of the buffer
    \Input *pHash       - [out] big number representation of the hash

    \Notes
        Based on the algorithm for ECDSA, the hash needs to be truncated to the
        bit length of curve.order which can be done by a right shift. The problem
        is that other implementations don't follow this and just truncate whole
        bytes first. For this reason that is what we do here.

    \Version 02/13/2018 (eesponda)
*/
/********************************************************************************F*/
static void _CryptNistDsaInitHash(const CryptNistT *pState, const uint8_t *pHashData, int32_t iHashSize, CryptBnT *pHash)
{
    int32_t iOrderSize = CryptBnBitLen(&pState->Order);

    if ((iHashSize*8) > iOrderSize)
    {
        iHashSize = (iOrderSize + 7) / 8;
    }
    CryptBnInitFrom(pHash, -1, pHashData, iHashSize);
}

/*F********************************************************************************/
/*!
    \Function _CryptNistInit

    \Description
        Initializes the curve state

    \Input *pState          - curve state
    \Input *pCurve          - curve data to initialize the state

    \Output
        int32_t             - 0=success, negative=failure

    \Version 02/17/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _CryptNistInit(CryptNistT *pState, const NistCurveT *pCurve)
{
    // initialize the curve parameters
    pState->iSize = pCurve->iSize;
    CryptBnInitFrom(&pState->Prime, -1, pCurve->aPrime, pCurve->iSize);
    CryptBnInitFrom(&pState->CoefficientA, -1, pCurve->aCoefficientA, pCurve->iSize);
    CryptBnInitFrom(&pState->CoefficientB, -1, pCurve->aCoefficientB, pCurve->iSize);
    CryptBnInitFrom(&pState->Order, -1, pCurve->aOrder, pCurve->iSize);
    if (CryptNistPointInitFrom(&pState->BasePoint, pCurve->aBasePoint, pCurve->iSize * 2 + 1) < 0)
    {
        return(-1);
    }

    // initialize the computation state
    pState->iKeyIndex = -1;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _CryptNistDsaInitK

    \Description
        Generates a K value for signing operations deterministically based on
        the private key, message hash and curve

    \Input *pState          - curve state
    \Input *pPrivateKey     - private key information
    \Input *pHash           - hash of the message we are signing
    \Input iHashSize        - size of the hash
    \Input *pResult         - [out] K value for signing

    \Notes
        This work is based on RFC6979: https://tools.ietf.org/html/rfc6979
        It allows for selecting a K value without the use of random data.
        Aside from this generating benefit the ability to generate deterministic
        signatures are required for other cryptographic operations.

    \Version 01/08/2018 (eesponda)
*/
/********************************************************************************F*/
static void _CryptNistDsaInitK(const CryptNistT *pState, const CryptBnT *pPrivateKey, const uint8_t *pHash, int32_t iHashSize, CryptBnT *pResult)
{
    static const uint8_t _aZero[] = { 0x00 }, _aOne[] = { 0x01 };

    CryptHashTypeE eHashType;
    uint8_t aNewHash[CRYPTHASH_MAXDIGEST], aK[CRYPTHASH_MAXDIGEST], aV[CRYPTHASH_MAXDIGEST], aT[CRYPTHASH_MAXDIGEST], aPrivateKey[48];
    CryptHmacMsgT aMessages[4];
    int32_t iHashOffset = iHashSize < pState->iSize ? pState->iSize - iHashSize : 0;

    // figure out the hash based on the type
    if ((eHashType = CryptHashGetBySize(iHashSize)) == CRYPTHASH_NULL)
    {
        NetPrintf(("cryptnist: cannot generate deterministic k, unsupported hash used (size=%d)\n", iHashSize));
        return;
    }

    // copy the hash to our buffer, this ensures correct format
    ds_memclr(aNewHash, sizeof(aNewHash));
    ds_memcpy_s(aNewHash+iHashOffset, sizeof(aNewHash)-iHashOffset, pHash, iHashSize);

    // init k, v, private key
    ds_memclr(aK, sizeof(aK));
    ds_memset(aV, 0x01, sizeof(aV));
    CryptBnFinal(pPrivateKey, aPrivateKey, pState->iSize);

    // K = HMAC(V || 0x00 || PrivateKey || H(m))
    aMessages[0].pMessage = aV;
    aMessages[0].iMessageLen = iHashSize;
    aMessages[1].pMessage = _aZero;
    aMessages[1].iMessageLen = sizeof(_aZero);
    aMessages[2].pMessage = aPrivateKey;
    aMessages[2].iMessageLen = pState->iSize;
    aMessages[3].pMessage = aNewHash;
    aMessages[3].iMessageLen = pState->iSize;
    CryptHmacCalcMulti(aK, iHashSize, aMessages, 4, aK, iHashSize, eHashType);

    // V = HMAC(V)
    CryptHmacCalc(aV, iHashSize, aV, iHashSize, aK, iHashSize, eHashType);

    // K = HMAC(V || 0x01 || PrivateKey || H(m))
    aMessages[0].pMessage = aV;
    aMessages[0].iMessageLen = iHashSize;
    aMessages[1].pMessage = _aOne;
    aMessages[1].iMessageLen = sizeof(_aOne);
    aMessages[2].pMessage = aPrivateKey;
    aMessages[2].iMessageLen = pState->iSize;
    aMessages[3].pMessage = aNewHash;
    aMessages[3].iMessageLen = pState->iSize;
    CryptHmacCalcMulti(aK, iHashSize, aMessages, 4, aK, iHashSize, eHashType);

    // V = HMAC(V)
    CryptHmacCalc(aV, iHashSize, aV, iHashSize, aK, iHashSize, eHashType);

    // generate T for new K candidate
    while (1)
    {
        int32_t iOffset = 0, iBytes;

        while (iOffset < pState->iSize)
        {
            // V = HMAC(V)
            CryptHmacCalc(aV, iHashSize, aV, iHashSize, aK, iHashSize, eHashType);
            iBytes = DS_MIN(iHashSize, pState->iSize - iOffset);
            ds_memcpy(aT+iOffset, aV, iBytes);
            iOffset += iBytes;
        }

        // copy result and check if suitable
        CryptBnInitFrom(pResult, -1, aT, pState->iSize);
        if (CryptBnCompare(pResult, &pState->Order) < 0)
        {
            break;
        }

        // K = HMAC(V || 0x00)
        aMessages[0].pMessage = aV;
        aMessages[0].iMessageLen = iHashSize;
        aMessages[1].pMessage = _aZero;
        aMessages[1].iMessageLen = sizeof(_aZero);
        CryptHmacCalcMulti(aK, iHashSize, aMessages, 2, aK, iHashSize, eHashType);

        // V = HMAC(V)
        CryptHmacCalc(aV, iHashSize, aV, iHashSize, aK, iHashSize, eHashType);
    }
}

/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CryptNistInitDh

    \Description
        Initializes the curve state for performing diffie hellmen key exchange

    \Input *pState          - curve state
    \Input iCurveType       - identifier for the curve we are using

    \Output
        int32_t             - 0=success, negative=failure

    \Version 02/17/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptNistInitDh(CryptNistDhT *pState, int32_t iCurveType)
{
    int32_t iResult, iCurveIdx;
    const NistCurveT *pCurve = NULL;

    ds_memclr(pState, sizeof(*pState));

    // find the curve
    for (iCurveIdx = 0; iCurveIdx < (signed)(sizeof(_aEccCurves)/sizeof(_aEccCurves[0])); iCurveIdx += 1)
    {
        if (iCurveType == _aEccCurves[iCurveIdx].iIdent)
        {
            pCurve = &_aEccCurves[iCurveIdx];
            break;
        }
    }
    // check to make sure we found a valid curve
    if (pCurve == NULL)
    {
        NetPrintf(("cryptnist: could not find matching curve information for dh given ident %d\n", iCurveType));
        return(-1);
    }

    // initialize the curve and computation data
    if ((iResult = _CryptNistInit(&pState->Ecc, pCurve)) == 0)
    {
        // generate random private key
        _CryptNistRand(&pState->Ecc, &pState->PrivateKey);
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function CryptNistInitDsa

    \Description
        Initializes the curve state for performing dsa sign and verify

    \Input *pState          - curve state
    \Input iCurveType       - identifier for the curve we are using

    \Output
        int32_t             - 0=success, negative=failure

    \Version 04/12/2018 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptNistInitDsa(CryptNistDsaT *pState, int32_t iCurveType)
{
    int32_t iCurveIdx;
    const NistCurveT *pCurve = NULL;

    ds_memclr(pState, sizeof(*pState));

    // find the curve
    for (iCurveIdx = 0; iCurveIdx < (signed)(sizeof(_aEccCurves)/sizeof(_aEccCurves[0])); iCurveIdx += 1)
    {
        if (iCurveType == _aEccCurves[iCurveIdx].iIdent)
        {
            pCurve = &_aEccCurves[iCurveIdx];
            break;
        }
    }
    // check to make sure we found a valid curve
    if (pCurve == NULL)
    {
        NetPrintf(("cryptnist: could not find matching curve information for dh given ident %d\n", iCurveType));
        return(-1);
    }

    // initialize the curve and computation data
    return(_CryptNistInit(&pState->Ecc, pCurve));
}

/*F********************************************************************************/
/*!
    \Function CryptNistPublic

    \Description
        Generates a public key

    \Input *pState      - curve state
    \Input *pResult     - [out] point on the curve that represents the public key (optional)
    \Input *pCryptUsecs - [out] timing information for the operation (optional)

    \Output
        int32_t         - zero=success, otherwise=pending

    \Notes
        If pResult is NULL, the result can be taken from CryptNistT.Result

    \Version 02/17/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptNistPublic(CryptNistDhT *pState, CryptEccPointT *pResult, uint32_t *pCryptUsecs)
{
    int32_t iResult;

    if ((iResult = _CryptNistDoubleAndAdd(&pState->Ecc, &pState->Ecc.BasePoint, &pState->PrivateKey, &pState->Result)) == 0)
    {
        // copy the timing if passed in
        if (pCryptUsecs != NULL)
        {
            *pCryptUsecs = pState->Ecc.uCryptUSecs;
        }
        /* copy the final result if a valid destination is provided (the caller can copy the result
           later if desired) */
        if (pResult != NULL)
        {
            ds_memcpy(pResult, &pState->Result, sizeof(*pResult));
        }
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function CryptNistSecret

    \Description
        Generates a shared secret

    \Input *pState      - curve state
    \Input *pPublicKey  - point as a basis to compute the shared secret
    \Input *pResult     - [out] point on the curve that represents the shared secret (optional)
    \Input *pCryptUsecs - [out] timing information for the operation (optional)

    \Output
        int32_t         - zero=success, otherwise=pending

    \Notes
        If pResult is NULL, the result can be taken from CryptNistT.Result

    \Version 02/17/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptNistSecret(CryptNistDhT *pState, CryptEccPointT *pPublicKey, CryptEccPointT *pResult, uint32_t *pCryptUsecs)
{
    int32_t iResult;

    if ((iResult = _CryptNistDoubleAndAdd(&pState->Ecc, pPublicKey, &pState->PrivateKey, &pState->Result)) == 0)
    {
        // copy the timing if passed in
        if (pCryptUsecs != NULL)
        {
            *pCryptUsecs = pState->Ecc.uCryptUSecs;
        }
        /* copy the final result if a valid destination is provided (the caller can copy the result
           later if desired) */
        if (pResult != NULL)
        {
            ds_memcpy(pResult, &pState->Result, sizeof(*pResult));
        }
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function CryptNistSign

    \Description
        Generates an elliptic curve dsa signature

    \Input *pState      - curve state
    \Input *pPrivateKey - key used to sign
    \Input *pHash       - hash we are creating a signature for
    \Input iHashSize    - size of the hash
    \Input *pSignature  - [out] point on the curve that represents our signature (optional)
    \Input *pCryptUsecs - [out] timing information for the operation (optional)

    \Output
        int32_t         - zero=success, otherwise=pending

    \Notes
        If pSignature is NULL, the result can be taken from CryptNistT.Result

    \Version 05/15/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptNistSign(CryptNistDsaT *pState, const CryptBnT *pPrivateKey, const uint8_t *pHash, int32_t iHashSize, CryptEccPointT *pSignature, uint32_t *pCryptUsecs)
{
    int32_t iResult;
    CryptNistT *pEcc = &pState->Ecc;

    // generate a secret to the specified size
    if (CryptBnBitLen(&pState->K) == 0)
    {
        _CryptNistDsaInitK(pEcc, pPrivateKey, pHash, iHashSize, &pState->K);
    }

    // calculate x,y using k * basepoint
    if ((iResult = _CryptNistDoubleAndAdd(pEcc, &pEcc->BasePoint, &pState->K, &pState->Result)) == 0)
    {
        const uint64_t uTickUsecs = NetTickUsec();
        CryptBnT Hash;

        // r = x % order
        CryptBnMod(&pState->Result.X, &pEcc->Order, &pState->Result.X, NULL);

        // s1 = ((hash + r * private)
        CryptBnInitFrom(&Hash, -1, pHash, iHashSize);

        // truncate the hash if necessary
        _CryptNistDsaInitHash(pEcc, pHash, iHashSize, &Hash);

        CryptBnMultiply(&pState->Result.Y, &pState->Result.X, pPrivateKey);
        CryptBnAccumulate(&pState->Result.Y, &Hash);

        // s2 = inverse_mod(k, order)
        CryptBnInverseMod(&pState->K, &pEcc->Order);

        // s = s1 * s2 % order
        CryptBnModMultiply(&pState->Result.Y, &pState->Result.Y, &pState->K, &pEcc->Order);

        /* copy the final result if a valid destination is provided (the caller can copy the result
           later if desired) */
        if (pSignature != NULL)
        {
            ds_memcpy(pSignature, &pState->Result, sizeof(*pSignature));
        }

        // update profiler information
        pEcc->uCryptUSecs += NetTickDiff(NetTickUsec(), uTickUsecs);

        // copy the timing if passed in
        if (pCryptUsecs != NULL)
        {
            *pCryptUsecs = pEcc->uCryptUSecs;
        }
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function CryptNistVerify

    \Description
        Verify the elliptic curve dsa signature

    \Input *pState      - curve state
    \Input *pPublicKey  - the public used for verifying the signature
    \Input *pHash       - hash we are verifying a signature for
    \Input iHashSize    - size of the hash
    \Input *pSignature  - point on the curve that represents our signature
    \Input *pCryptUsecs - [out] timing information for the operation (optional)

    \Output
        int32_t         - zero=success, >0=pending, <0=failure

    \Version 05/15/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptNistVerify(CryptNistDsaT *pState, CryptEccPointT *pPublicKey, const uint8_t *pHash, int32_t iHashSize, CryptEccPointT *pSignature, uint32_t *pCryptUsecs)
{
    int32_t iResult;
    CryptNistT *pEcc = &pState->Ecc;

    // initialize state if necessary
    if (pEcc->iKeyIndex < 0)
    {
        CryptBnT S, Hash;

        // truncate the hash if necessary
        _CryptNistDsaInitHash(pEcc, pHash, iHashSize, &Hash);

        // s' = inverse_mod(signature.s, order);
        CryptBnClone(&S, &pSignature->Y);
        CryptBnInverseMod(&S, &pEcc->Order);

        // u1 = s' * hash % order
        CryptBnModMultiply(&pState->U1, &Hash, &S, &pEcc->Order);
        // u2 = s' * signature.r % order
        CryptBnModMultiply(&pState->U2, &pSignature->X, &S, &pEcc->Order);
    }

    // result = (u1 * basepoint) + (u2 * publickey)
    if ((iResult = _CryptNistDoubleMultiply(pEcc, &pEcc->BasePoint, &pState->U1, pPublicKey, &pState->U2, &pState->Result)) == 0)
    {
        // copy the timing if passed in
        if (pCryptUsecs != NULL)
        {
            *pCryptUsecs = pEcc->uCryptUSecs;
        }
        // signature valid if result.x == signature.x
        iResult = (memcmp(&pSignature->X, &pState->Result.X, sizeof(pSignature->X)) == 0) ? 0 : -1;
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function CryptNistPointValidate

    \Description
        Validate that the point is on the curve

    \Input *pState      - curve state
    \Input *pPoint      - point we are validating

    \Output
        uint8_t         - TRUE=on the curve, FALSE=not on the curve

    \Version 02/17/2017 (eesponda)
*/
/********************************************************************************F*/
uint8_t CryptNistPointValidate(const CryptNistDhT *pState, const CryptEccPointT *pPoint)
{
    CryptBnT Result, X, Y, A;
    const CryptNistT *pEcc = &pState->Ecc;

    // a' = curve.a * x
    CryptBnMultiply(&A, &pEcc->CoefficientA, &pPoint->X);

    // y' = y * y
    CryptBnMultiply(&Y, &pPoint->Y, &pPoint->Y);

    // x' = x * x * x
    CryptBnMultiply(&X, &pPoint->X, &pPoint->X);
    CryptBnMultiply(&X, &X, &pPoint->X);

    // result = y' - x' - a' - curve.b
    CryptBnSubtract(&Result, &Y, &X);
    CryptBnSubtract(&Result, &Result, &A);
    CryptBnSubtract(&Result, &Result, &pEcc->CoefficientB);

    CryptBnMod(&Result, &pEcc->Prime, &Result, NULL);
    return(CryptBnBitLen(&Result) == 0);
}

/*F********************************************************************************/
/*!
    \Function CryptNistPointInitFrom

    \Description
        Loads a point from a buffer

    \Input *pPoint      - [out] the point we are writing the information to
    \Input *pBuffer     - buffer we are reading the point data from
    \Input iBufLen      - size of the buffer

    \Output
        int32_t         - 0=success, negative=failure

    \Notes
        We only support reading uncompressed points

    \Version 02/17/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptNistPointInitFrom(CryptEccPointT *pPoint, const uint8_t *pBuffer, int32_t iBufLen)
{
    int32_t iPointLen = (iBufLen-1)/2;

    // we only support uncompressed points, so ensure that this is the case
    if (*pBuffer++ != CRYPTNIST_UNCOMPRESSED_MARKER)
    {
        NetPrintf(("cryptnist: could not initialize curve, base point in wrong format\n"));
        return(-1);
    }
    CryptBnInitFrom(&pPoint->X, -1, pBuffer, iPointLen);
    CryptBnInitFrom(&pPoint->Y, -1, pBuffer+iPointLen, iPointLen);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function CryptNistPointFinal

    \Description
        Loads a point into a buffer for dh

    \Input *pState      - curve state
    \Input *pPoint      - point state or NULL to use curve result
    \Input bSecret      - is this the shared secret? (only need x)
    \Input *pBuffer     - buffer we are writing the point data to
    \Input iBufLen      - size of the buffer

    \Output
        int32_t         - 0=success, negative=failure

    \Notes
        We only support writing uncompressed points

    \Version 02/17/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptNistPointFinal(const CryptNistDhT *pState, const CryptEccPointT *pPoint, uint8_t bSecret, uint8_t *pBuffer, int32_t iBufLen)
{
    if ((pState != NULL) && (pPoint == NULL))
    {
        pPoint = &pState->Result;
    }
    const int32_t iXSize = CryptBnByteLen(&pPoint->X);
    const int32_t iYSize = CryptBnByteLen(&pPoint->Y);
    const int32_t iOutputSize = !bSecret ? (iXSize + iYSize + 1) : iXSize;

    if (iBufLen < iOutputSize)
    {
        NetPrintf(("cryptnist: not enough space in output buffer to encode points\n"));
        return(-1);
    }

    if (!bSecret)
    {
        *pBuffer++ = CRYPTNIST_UNCOMPRESSED_MARKER;
        CryptBnFinal(&pPoint->X, pBuffer, iXSize);
        CryptBnFinal(&pPoint->Y, pBuffer+iXSize, iYSize);
    }
    else
    {
        CryptBnFinal(&pPoint->X, pBuffer, iXSize);
    }
    return(iOutputSize);
}
