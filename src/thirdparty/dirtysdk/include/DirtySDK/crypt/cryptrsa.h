/*H*************************************************************************************/
/*!
    \File    cryptrsa.h

    \Description
        This module is a from-scratch RSA implementation in order to avoid any
        intellectual property issues. The 1024 bit RSA public key encryption algorithm
        was implemented from a specification provided by Netscape for SSL implementation
        (see protossl.h).

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002.  ALL RIGHTS RESERVED.

    \Version 03/08/2002 (gschaefer) First Version (protossl)
    \Version 11/18/2002 (jbrookes)  Names changed to include "Proto"
    \Version 03/05/2003 (jbrookes)  Split RSA encryption from protossl
*/
/*************************************************************************************H*/

#ifndef _cryptrsa_h
#define _cryptrsa_h

/*!
\Moduledef CryptRSA CryptRSA
\Modulemember Crypt
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/crypt/cryptbn.h"
#include "DirtySDK/crypt/cryptdef.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! union of our working state depending on the type of operations
typedef union CryptRSAWorkingT
{
    struct
    {
        CryptBnT Modulus;       //!< public key modulus
        CryptBnT Exponent;      //!< public key exponent

        // working memory for modular exponentiation
        CryptBnT Powerof;
        CryptBnT Accumul;
    } PublicKey;

    struct
    {
        /* selected private key state we use for chinese remainder theorum computation
           we have access to the other data (modulus, exponent) but it is not used so
           not worth storing */
        CryptBnT PrimeP;        //!< prime factor p of modulus
        CryptBnT PrimeQ;        //!< prime factor q of modulus
        CryptBnT ExponentP;     //!< exponent dP mod p-1
        CryptBnT ExponentQ;     //!< exponent dQ mod q-1
        CryptBnT Coeffecient;   //!< inverse of q mod p

        // working memory for modular exponentiation
        CryptBnT PowerofP;
        CryptBnT PowerofQ;
        CryptBnT M1;
        CryptBnT M2;

        // current state of computation
        enum
        {
            CRT_COMPUTE_M1,
            CRT_COMPUTE_M2
        } eState;
    } PrivateKey;
} CryptRSAWorkingT;

//! cryptrsa state
typedef struct CryptRSAT
{
    CryptRSAWorkingT Working;       //!< working state & data for doing modular exponentiation

    CryptBnT *pTable;               //!< table used in sliding window for precomputed powers
    CryptBnT aTable[1];             //!< fixed sized table used for public key operations which is our base case

    // memory allocation data
    int32_t iMemGroup;
    void *pMemGroupUserData;

    int32_t iKeyModSize;            //!< size of public key modulus
    uint8_t EncryptBlock[1024];     //!< input/output data

    int16_t iExpBitIndex;           //!< bit index into the current exponent we are working on
    uint8_t bAccumulOne;            //!< can we skip the first multiply?
    uint8_t bPrivate;               //!< are we public or private key operation?

    // rsa profiling info
    uint32_t uCryptMsecs;
    uint32_t uCryptUsecs;
    uint32_t uNumExpCalls;
} CryptRSAT;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// init rsa state
DIRTYCODE_API int32_t CryptRSAInit(CryptRSAT *pState, const uint8_t *pModulus, int32_t iModSize, const uint8_t *pExponent, int32_t iExpSize);

// init rsa state for private key data
DIRTYCODE_API int32_t CryptRSAInit2(CryptRSAT *pState, int32_t iModSize, const CryptBinaryObjT *pPrimeP, const CryptBinaryObjT *pPrimeQ, const CryptBinaryObjT *pExponentP, const CryptBinaryObjT *pExponentQ, const CryptBinaryObjT *pCoeffecient);

// setup the master shared secret for encryption
DIRTYCODE_API void CryptRSAInitMaster(CryptRSAT *pState, const uint8_t *pMaster, int32_t iMasterLen);

// setup the master shared secret for encryption using PKCS1.5
DIRTYCODE_API void CryptRSAInitPrivate(CryptRSAT *pState, const uint8_t *pMaster, int32_t iMasterLen);

// setup the encrypted signature for decryption.
DIRTYCODE_API void CryptRSAInitSignature(CryptRSAT *pState, const uint8_t *pSig, int32_t iSigLen);

// do the encryption/decryption
DIRTYCODE_API int32_t CryptRSAEncrypt(CryptRSAT *pState, int32_t iIter);

#ifdef __cplusplus
}
#endif

//@}

#endif // _cryptrsa_h

