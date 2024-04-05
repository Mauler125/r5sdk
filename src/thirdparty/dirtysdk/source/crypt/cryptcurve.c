/*H********************************************************************************/
/*!
    \File   cryptcurve.c

    \Description
        This module implements an interface over our different curve crypto APIs

    \Copyright
        Copyright (c) Electronic Arts 2018. ALL RIGHTS RESERVED.
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "DirtySDK/crypt/cryptmont.h"
#include "DirtySDK/crypt/cryptcurve.h"

/*** Variables ********************************************************************/

//! mappings for the dh functions
static const CryptCurveDhT _CryptCurve_Dh[] =
{
    { (CryptCurveInitT *)CryptNistInitDh, (CryptCurvePublicT *)CryptNistPublic, (CryptCurveSecretT *)CryptNistSecret, (CryptCurvePointInitT *)CryptNistPointInitFrom, (CryptCurvePointFinalT *)CryptNistPointFinal, CRYPTCURVE_SECP256R1 },
    { (CryptCurveInitT *)CryptNistInitDh, (CryptCurvePublicT *)CryptNistPublic, (CryptCurveSecretT *)CryptNistSecret, (CryptCurvePointInitT *)CryptNistPointInitFrom, (CryptCurvePointFinalT *)CryptNistPointFinal, CRYPTCURVE_SECP384R1 },
    { (CryptCurveInitT *)CryptMontInit, (CryptCurvePublicT *)CryptMontPublic, (CryptCurveSecretT *)CryptMontSecret, (CryptCurvePointInitT *)CryptMontPointInitFrom, (CryptCurvePointFinalT *)CryptMontPointFinal, CRYPTCURVE_X25519 },
    { (CryptCurveInitT *)CryptMontInit, (CryptCurvePublicT *)CryptMontPublic, (CryptCurveSecretT *)CryptMontSecret, (CryptCurvePointInitT *)CryptMontPointInitFrom, (CryptCurvePointFinalT *)CryptMontPointFinal, CRYPTCURVE_X448 }
};

//! mappings for the dsa functions
static const CryptCurveDsaT _CryptCurve_Dsa[] =
{
    { (CryptCurveInitT *)CryptNistInitDsa, (CryptCurveSignT *)CryptNistSign, (CryptCurveVerifyT *)CryptNistVerify, (CryptCurvePointInitT *)CryptNistPointInitFrom, CRYPTCURVE_SECP256R1 },
    { (CryptCurveInitT *)CryptNistInitDsa, (CryptCurveSignT *)CryptNistSign, (CryptCurveVerifyT *)CryptNistVerify, (CryptCurvePointInitT *)CryptNistPointInitFrom, CRYPTCURVE_SECP384R1 }
};

/*** Public Functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function CryptCurveGetDh

    \Description
        Gets the diffie hellmen functions based on the curve type

    \Input iCurveType           - type that identifies the curve

    \Output
        const CryptCurveDhT *   - pointer containing functions or NULL (not found)

    \Version 05/04/2018 (eesponda)
*/
/********************************************************************************F*/
const CryptCurveDhT *CryptCurveGetDh(int32_t iCurveType)
{
    const CryptCurveDhT *pCurve = NULL;
    int32_t iCurveIdx;

    for (iCurveIdx = 0; iCurveIdx < (signed)(sizeof(_CryptCurve_Dh)/sizeof(_CryptCurve_Dh[0])); iCurveIdx += 1)
    {
        if (_CryptCurve_Dh[iCurveIdx].iCurveType == iCurveType)
        {
            pCurve = &_CryptCurve_Dh[iCurveIdx];
            break;
        }
    }
    return(pCurve);
}

/*F********************************************************************************/
/*!
    \Function CryptCurveGetDsa

    \Description
        Gets the digital signature algorithm functions based on the curve type

    \Input iCurveType           - type that identifies the curve

    \Output
        const CryptCurveDsaT *  - pointer containing functions or NULL (not found)

    \Version 05/04/2018 (eesponda)
*/
/********************************************************************************F*/
const CryptCurveDsaT *CryptCurveGetDsa(int32_t iCurveType)
{
    const CryptCurveDsaT *pCurve = NULL;
    int32_t iCurveIdx;

    for (iCurveIdx = 0; iCurveIdx < (signed)(sizeof(_CryptCurve_Dsa)/sizeof(_CryptCurve_Dsa[0])); iCurveIdx += 1)
    {
        if (_CryptCurve_Dsa[iCurveIdx].iCurveType == iCurveType)
        {
            pCurve = &_CryptCurve_Dsa[iCurveIdx];
            break;
        }
    }
    return(pCurve);
}
