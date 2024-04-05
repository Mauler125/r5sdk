/*H********************************************************************************/
/*!
    \File   cryptnist.h

    \Description
        This module implements the math for elliptic curve cryptography
        using curves in short Weierstrass form (NIST curves)

    \Copyright
        Copyright (c) Electronic Arts 2017. ALL RIGHTS RESERVED.
*/
/********************************************************************************H*/

#ifndef _cryptnist_h
#define _cryptnist_h

/*!
\Moduledef CryptNist CryptNist
\Modulemember Crypt
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/crypt/cryptbn.h"
#include "DirtySDK/crypt/cryptdef.h"

/*** Defines **********************************************************************/

//! window size used for sliding window
#define CRYPTNIST_WINDOW_SIZE (5)

//! precomputed table size based on the window size
#define CRYPTNIST_TABLE_SIZE (1 << (CRYPTNIST_WINDOW_SIZE - 1))

/*** Type Definitions *************************************************************/

//! private state that defines the curve
typedef struct CryptNistT
{
    CryptBnT Prime;             //!< the finite field that the curve is defined over
    CryptBnT CoefficientA;      //!< cofficient that defines the curve
    CryptBnT CoefficientB;      //!< cofficient that defines the curve
    CryptEccPointT BasePoint;   //!< generator point on the curve
    CryptBnT Order;             //!< the number of point operations on the curve until the resultant line is vertical

    int32_t iMemGroup;          //!< memgroup id
    void *pMemGroupUserdata;    //!< memgroup userdata
    CryptEccPointT *pTable;     //!< precomputed values used for for computation
    int32_t iKeyIndex;          //!< current bit index state when generating
    int32_t iSize;              //!< size of curve parameters

    uint32_t uCryptUSecs;       //!< number of total usecs the operation took
} CryptNistT;

//! state dealing with diffie hellmen key exchange
typedef struct CryptNistDhT
{
    CryptNistT Ecc;              //!< base ecc state

    CryptBnT PrivateKey;        //!< private key used to calculate public key and shared secret
    CryptEccPointT Result;      //!< working state
} CryptNistDhT;

//! state dealing with dsa sign and verify
typedef struct CryptNistDsaT
{
    CryptNistT Ecc;             //!< base ecc state

    CryptBnT K;                 //!< secret used for signing operations
    CryptBnT U1;                //!< used for verification operations
    CryptBnT U2;                //!< used for verification operations
    CryptEccPointT Result;      //!< working state
} CryptNistDsaT;

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// initializes the crypt state to perform diffie hellmen key exchange
DIRTYCODE_API int32_t CryptNistInitDh(CryptNistDhT *pState, int32_t iCurveType);

// initializes the crypt state to perform dsa sign and verify
DIRTYCODE_API int32_t CryptNistInitDsa(CryptNistDsaT *pState, int32_t iCurveType);

// generates a public key based on the curve parameters: BasePoint * Secret
DIRTYCODE_API int32_t CryptNistPublic(CryptNistDhT *pState, CryptEccPointT *pResult, uint32_t *pCryptUsecs);

// generate a shared secret based on the curve parameters and other parties public key: PublicKey * Secret
DIRTYCODE_API int32_t CryptNistSecret(CryptNistDhT *pState, CryptEccPointT *pPublicKey, CryptEccPointT *pResult, uint32_t *pCryptUsecs);

// generate a ecdsa signature
DIRTYCODE_API int32_t CryptNistSign(CryptNistDsaT *pState, const CryptBnT *pPrivateKey, const uint8_t *pHash, int32_t iHashSize, CryptEccPointT *pSignature, uint32_t *pCryptUsecs);

// verify a ecdsa signature
DIRTYCODE_API int32_t CryptNistVerify(CryptNistDsaT *pState, CryptEccPointT *pPublicKey, const uint8_t *pHash, int32_t iHashSize, CryptEccPointT *pSignature, uint32_t *pCryptUsecs);

// check if the point is on the curve
DIRTYCODE_API uint8_t CryptNistPointValidate(const CryptNistDhT *pState, const CryptEccPointT *pPoint);

// initialize a point on the curve from a buffer
DIRTYCODE_API int32_t CryptNistPointInitFrom(CryptEccPointT *pPoint, const uint8_t *pBuffer, int32_t iBufLen);

// output a point to a buffer (for dh, dsa encoding looks different which we do in protossl)
DIRTYCODE_API int32_t CryptNistPointFinal(const CryptNistDhT *pState, const CryptEccPointT *pPoint, uint8_t bSecret, uint8_t *pBuffer, int32_t iBufLen);

#ifdef __cplusplus
}
#endif

//@}

#endif // _cryptnist_h
