/*H********************************************************************************/
/*!
    \File   cryptcurve.h

    \Description
        This module implements an interface over our different curve crypto APIs

    \Copyright
        Copyright (c) Electronic Arts 2018. ALL RIGHTS RESERVED.
*/
/********************************************************************************H*/

#ifndef _cryptcurve_h
#define _cryptcurve_h

/*!
\Moduledef CryptCurve CryptCurve
\Modulemember Crypt
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/crypt/cryptbn.h"
#include "DirtySDK/crypt/cryptdef.h"
#include "DirtySDK/crypt/cryptnist.h"

/*** Defines **********************************************************************/

//! maximum state storage we need for diffie hellman
#define CRYPTCURVE_DH_MAXSTATE  (sizeof(CryptNistDhT))
//! maximum state storage we need for digital signature algorithm
#define CRYPTCURVE_DSA_MAXSTATE (sizeof(CryptNistDsaT))

/*** Type Definitions *************************************************************/

//! curve init function
typedef int32_t (CryptCurveInitT)(void *pState, int32_t iCurveType);

//! curve generate public function
typedef int32_t (CryptCurvePublicT)(void *pState, CryptEccPointT *pResult, uint32_t *pCryptUsecs);

//! curve generate secret function
typedef int32_t (CryptCurveSecretT)(void *pState, CryptEccPointT *pPublicKey, CryptEccPointT *pResult, uint32_t *pCryptUsecs);

//! curve init point
typedef int32_t (CryptCurvePointInitT)(CryptEccPointT *pPoint, const uint8_t *pBuffer, int32_t iBufLen);

//! curve output point to buffer (for dh cases)
typedef int32_t (CryptCurvePointFinalT)(void *pState, const CryptEccPointT *pPoint, uint8_t bSecret, uint8_t *pBuffer, int32_t iBufLen);

//! curve generate dsa signature
typedef int32_t (CryptCurveSignT)(void *pState, const CryptBnT *pPrivateKey, const uint8_t *pHash, int32_t iHashSize, CryptEccPointT *pSignature, uint32_t *pCryptUsecs);

//! curve verify dsa signature
typedef int32_t (CryptCurveVerifyT)(void *pState, CryptEccPointT *pPublicKey, const uint8_t *pHash, int32_t iHashSize, CryptEccPointT *pSignature, uint32_t *pCryptUsecs);

//! interface for dh functionality
typedef struct CryptCurveDhT
{
    CryptCurveInitT *Init;
    CryptCurvePublicT *Public;
    CryptCurveSecretT *Secret;
    CryptCurvePointInitT *PointInit;
    CryptCurvePointFinalT *PointFinal;
    int32_t iCurveType;
} CryptCurveDhT;

//! interface for dsa functionality
typedef struct CryptCurveDsaT
{
    CryptCurveInitT *Init;
    CryptCurveSignT *Sign;
    CryptCurveVerifyT *Verify;
    CryptCurvePointInitT *PointInit;
    int32_t iCurveType;
} CryptCurveDsaT;

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// get dh reference and initializes based on curve type
DIRTYCODE_API const CryptCurveDhT *CryptCurveGetDh(int32_t iCurveType);

// get dsa reference and initializes based on curve type
DIRTYCODE_API const CryptCurveDsaT *CryptCurveGetDsa(int32_t iCurveType);

#ifdef __cplusplus
}
#endif

//@}

#endif // _cryptcurve_h
