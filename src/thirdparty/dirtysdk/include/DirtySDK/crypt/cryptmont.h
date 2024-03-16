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

#ifndef _cryptmont_h
#define _cryptmont_h

/*!
\Moduledef CryptMont CryptMont
\Modulemember Crypt
*/
//@{

#include "DirtySDK/platform.h"
#include "DirtySDK/crypt/cryptbn.h"
#include "DirtySDK/crypt/cryptdef.h"

/*** Type Definitions **************************************************************/

typedef struct CryptMontT
{
    CryptBnT Prime;             //!< curve prime
    CryptBnT PrimeMin2;         //!< curve prime - 2 used for exponentiation
    CryptBnT BasePoint;         //!< curve base point (only u)
    CryptBnT A24;               //!< constant used for calculations
    CryptBnT PrivateKey;        //!< private key

    //! working state
    CryptBnT X_2;
    CryptBnT X_3;

    CryptEccPointT Result;      //!< cached result and z_2/z_3 during the point state

    int32_t iMemGroup;          //!< memgroup id
    void *pMemGroupUserdata;    //!< memgroup userdata
    CryptBnT *pTable;           //!< table used for sliding window in exponentiation

    enum
    {
        CRYPTMONT_COMPUTE_POINT,//!< calculate the point
        CRYPTMONT_COMPUTE_EXP   //!< calculate the final result
    } eState;                   //!< current state of the calculation

    int16_t iBitIndex;          //!< bit index for the calculation (private key or exponent)
    uint8_t bAccumulOne;        //!< used to skip the first multiply
    uint8_t uSwap;              //!< current swap state
    uint32_t uCryptUsecs;       //!< total number of usecs the operation took
    int32_t iCurveType;         //!< what curve were we initialized with
} CryptMontT;

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// init the curve state
DIRTYCODE_API int32_t CryptMontInit(CryptMontT *pState, int32_t iCurveType);

// for testing-purposes; set the private key
DIRTYCODE_API void CryptMontSetPrivateKey(CryptMontT *pState, const uint8_t *pKey);

// generates a public key based on the curve parameters: BasePoint * Secret
DIRTYCODE_API int32_t CryptMontPublic(CryptMontT *pState, CryptEccPointT *pResult, uint32_t *pCryptUsecs);

// generate a shared secret based on the curve parameters and other parties public key: PublicKey * Secret
DIRTYCODE_API int32_t CryptMontSecret(CryptMontT *pState, CryptEccPointT *pPublicKey, CryptEccPointT *pResult, uint32_t *pCryptUsecs);

// initialize a point on the curve from a buffer
DIRTYCODE_API int32_t CryptMontPointInitFrom(CryptEccPointT *pPoint, const uint8_t *pBuffer, int32_t iBufSize);

// output a point to a buffer
DIRTYCODE_API int32_t CryptMontPointFinal(const CryptMontT *pState, const CryptEccPointT *pPoint, uint8_t bSecret, uint8_t *pBuffer, int32_t iBufSize);

#ifdef __cplusplus
}
#endif

//@}

#endif // _cryptmont_h

