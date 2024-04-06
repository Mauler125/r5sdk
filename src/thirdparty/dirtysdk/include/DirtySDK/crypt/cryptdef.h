/*H*******************************************************************/
/*!
    \File cryptdef.h

    \Description
        Common definitions for Crypt modules

    \Copyright
        Copyright (c) Electronic Arts 2013

    \Version 1.0 11/19/2013 (jbrookes) First Version
*/
/*******************************************************************H*/

#ifndef _cryptdef_h
#define _cryptdef_h

/*!
\Moduledef CryptDef CryptDef
\Modulemember Crypt
*/
//@{

/*** Include files ***************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/crypt/cryptbn.h"

/*** Defines *********************************************************/

/*** Macros **********************************************************/

/*** Type Definitions ************************************************/

//! a binary large object, such as a modulus or exponent
typedef struct CryptBinaryObjT
{
    uint8_t *pObjData;
    int32_t iObjSize;
} CryptBinaryObjT;

//! used for the point calculations that are used for elliptic curve crypto
typedef struct CryptEccPointT
{
    CryptBnT X;
    CryptBnT Y;
} CryptEccPointT;

//! curve supported groups as per: https://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml#tls-parameters-8
typedef enum CryptCurveE
{
    CRYPTCURVE_UNASSIGNED,
    CRYPTCURVE_SECT163K1,
    CRYPTCURVE_SECT163R1,
    CRYPTCURVE_SECT163R2,
    CRYPTCURVE_SECT193R1,
    CRYPTCURVE_SECT193R2,
    CRYPTCURVE_SECT233K1,
    CRYPTCURVE_SECT233R1,
    CRYPTCURVE_SECT239K1,
    CRYPTCURVE_SECT283K1,
    CRYPTCURVE_SECT283R1,
    CRYPTCURVE_SECT409K1,
    CRYPTCURVE_SECT409R1,
    CRYPTCURVE_SECT571K1,
    CRYPTCURVE_SECT571R1,
    CRYPTCURVE_SECP160K1,
    CRYPTCURVE_SECP160R1,
    CRYPTCURVE_SECP160R2,
    CRYPTCURVE_SECP192K1,
    CRYPTCURVE_SECP192R1,
    CRYPTCURVE_SECP224K1,
    CRYPTCURVE_SECP224R1,
    CRYPTCURVE_SECP256K1,
    CRYPTCURVE_SECP256R1,
    CRYPTCURVE_SECP384R1,
    CRYPTCURVE_SECP521R1,
    CRYPTCURVE_BRAINPOOLP256R1,
    CRYPTCURVE_BRAINPOOLP384R1,
    CRYPTCURVE_BRAINPOOLP512R1,
    CRYPTCURVE_X25519,
    CRYPTCURVE_X448,
    CRYPTCURVE_MAX = CRYPTCURVE_X448
} CryptCurveE;

//@}

#endif // _cryptdef_h

