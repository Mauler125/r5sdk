/*H*************************************************************************************************/
/*!
    \File protossl.h

    \Description
        This module is a from-scratch TLS implementation.  It does not use any
        third-party code of any kind and was developed entirely by EA.

    \Notes
        References:
            TLS1.0 RFC: https://tools.ietf.org/html/rfc2246
            TLS1.1 RFC: https://tools.ietf.org/html/rfc4346
            TLS1.2 RFC: https://tools.ietf.org/html/rfc5246
            TLS1.3 RFC: https://tools.ietf.org/html/rfc8446
            ASN.1 encoding rules: https://www.itu.int/ITU-T/studygroups/com17/languages/X.690-0207.pdf

    \Copyright
        Copyright (c) Electronic Arts 2002-2018

    \Version 03/08/2002 (gschaefer) Initial SSL 2.0 implementation
    \Version 03/03/2004 (sbevan)    Added certificate validation
    \Version 11/05/2005 (gschaefer) Rewritten to follow SSL 3.0 specification
    \Version 10/12/2012 (jbrookes)  Added support for TLS1.0 & TLS1.1
    \Version 10/20/2013 (jbrookes)  Added server handshake, client cert support
    \Version 11/06/2013 (jbrookes)  Added support for TLS1.2
    \Version 03/31/2017 (eesponda)  Added support for EC ciphers
    \Version 03/28/2018 (jbrookes)  Added support for TLS1.3
    \Version 08/15/2018 (jbrookes)  Removed SSLv3 & RC4 ciphers
*/
/*************************************************************************************************H*/

#ifndef _protossl_h
#define _protossl_h

/*!
\Moduledef ProtoSSL ProtoSSL
\Modulemember Proto
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/crypt/cryptdef.h"
#include "DirtySDK/crypt/cryptrsa.h"

/*** Defines ***************************************************************************/

// supported TLS versions
#define PROTOSSL_VERSION_TLS1_0     (0x0301)
#define PROTOSSL_VERSION_TLS1_1     (0x0302)
#define PROTOSSL_VERSION_TLS1_2     (0x0303)
#define PROTOSSL_VERSION_TLS1_3     (0x0304)

// protossl failure codes (retrieve with ProtoSSLStat('fail')
#define PROTOSSL_ERROR_NONE         ( 0)    //!< no error
#define PROTOSSL_ERROR_DNS          (-1)    //!< DNS failure
#define PROTOSSL_ERROR_CONN         (-10)   //!< TCP connection failure
#define PROTOSSL_ERROR_CONN_SSL2    (-11)   //!< connection attempt was using unsupported SSLv2 record format
#define PROTOSSL_ERROR_CONN_NOTSSL  (-12)   //!< connection attempt was not recognized as SSL
#define PROTOSSL_ERROR_CONN_MINVERS (-13)   //!< request failed minimum protocol version restriction
#define PROTOSSL_ERROR_CONN_MAXVERS (-14)   //!< request failed maximum protocol version restriction
#define PROTOSSL_ERROR_CONN_NOCIPHER (-15)   //!< no supported cipher
#define PROTOSSL_ERROR_CONN_NOCURVE (-16)   //!< no supported curve
#define PROTOSSL_ERROR_CERT_INVALID (-20)   //!< certificate invalid
#define PROTOSSL_ERROR_CERT_HOST    (-21)   //!< certificate not issued to this host
#define PROTOSSL_ERROR_CERT_NOTRUST (-22)   //!< certificate is not trusted (recognized)
#define PROTOSSL_ERROR_CERT_MISSING (-23)   //!< certificate not provided in certificate message
#define PROTOSSL_ERROR_CERT_BADDATE (-24)   //!< certificate date range validity check failed
#define PROTOSSL_ERROR_CERT_REQUEST (-25)   //!< CA fetch request failed
#define PROTOSSL_ERROR_SETUP        (-30)   //!< failure in secure setup
#define PROTOSSL_ERROR_SECURE       (-31)   //!< failure in secure connection after setup
#define PROTOSSL_ERROR_UNKNOWN      (-32)   //!< unknown failure

// SSLv3 cipher suites (available for TLS1.0+ although SSLv3 is no longer supported)
#define PROTOSSL_CIPHER_RSA_WITH_AES_128_CBC_SHA                    (1<<0)
#define PROTOSSL_CIPHER_RSA_WITH_AES_256_CBC_SHA                    (1<<1)
// TLS1.0 cipher suites
#define PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_128_CBC_SHA              (1<<2)
#define PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_256_CBC_SHA              (1<<3)
#define PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_CBC_SHA            (1<<4)
#define PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_256_CBC_SHA            (1<<5)
// TLS1.2 cipher suites
#define PROTOSSL_CIPHER_RSA_WITH_AES_128_CBC_SHA256                 (1<<6)
#define PROTOSSL_CIPHER_RSA_WITH_AES_256_CBC_SHA256                 (1<<7)
#define PROTOSSL_CIPHER_RSA_WITH_AES_128_GCM_SHA256                 (1<<8)
#define PROTOSSL_CIPHER_RSA_WITH_AES_256_GCM_SHA384                 (1<<9)
#define PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_128_CBC_SHA256           (1<<10)
#define PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_256_CBC_SHA384           (1<<11)
#define PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_128_GCM_SHA256           (1<<12)
#define PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_256_GCM_SHA384           (1<<13)
#define PROTOSSL_CIPHER_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256     (1<<14)
#define PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256         (1<<15)
#define PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384         (1<<16)
#define PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256         (1<<17)
#define PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384         (1<<18)
#define PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256   (1<<19)
// TLS1.3 cipher suites
#define PROTOSSL_CIPHER_AES_128_GCM_SHA256                          (1<<20)
#define PROTOSSL_CIPHER_AES_256_GCM_SHA384                          (1<<21)
#define PROTOSSL_CIPHER_CHACHA20_POLY1305_SHA256                    (1<<22)

//! all rsa cipher suites (minus disabled rc4-md5)
#define PROTOSSL_CIPHER_RSA (\
    PROTOSSL_CIPHER_RSA_WITH_AES_128_CBC_SHA|PROTOSSL_CIPHER_RSA_WITH_AES_256_CBC_SHA|\
    PROTOSSL_CIPHER_RSA_WITH_AES_128_CBC_SHA256|PROTOSSL_CIPHER_RSA_WITH_AES_128_GCM_SHA256|\
    PROTOSSL_CIPHER_RSA_WITH_AES_256_CBC_SHA256|PROTOSSL_CIPHER_RSA_WITH_AES_256_GCM_SHA384)

//! all ecc cipher suites
#define PROTOSSL_CIPHER_ECC (\
    PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_128_CBC_SHA|PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_128_CBC_SHA256|\
    PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_256_CBC_SHA|PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_256_CBC_SHA384|\
    PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_128_GCM_SHA256|PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_CBC_SHA|\
    PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_256_GCM_SHA384|PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_256_CBC_SHA|\
    PROTOSSL_CIPHER_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256|\
    PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256|PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256|\
    PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384|PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384|\
    PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256)

//! all tls1.3 cipher suites
#define PROTOSSL_CIPHER_ALL_13 (PROTOSSL_CIPHER_AES_128_GCM_SHA256|PROTOSSL_CIPHER_AES_256_GCM_SHA384|PROTOSSL_CIPHER_CHACHA20_POLY1305_SHA256)

//! default cipher suites
#define PROTOSSL_CIPHER_ALL (PROTOSSL_CIPHER_RSA|PROTOSSL_CIPHER_ECC|PROTOSSL_CIPHER_ALL_13)

// client cert flags (ProtoSSLControl() 'ccrt' selector)
#define PROTOSSL_CLIENTCERT_NONE                (0)
#define PROTOSSL_CLIENTCERT_OPTIONAL            (1)
#define PROTOSSL_CLIENTCERT_REQUIRED            (2)

// clienthello extensions
#define PROTOSSL_HELLOEXTN_NONE                 (0)
#define PROTOSSL_HELLOEXTN_SERVERNAME           (1)
#define PROTOSSL_HELLOEXTN_SIGALGS              (2)
#define PROTOSSL_HELLOEXTN_ALPN                 (4)
#define PROTOSSL_HELLOEXTN_ELLIPTIC_CURVES      (8)
// all extensions
#define PROTOSSL_HELLOEXTN_ALL (\
    PROTOSSL_HELLOEXTN_SERVERNAME|PROTOSSL_HELLOEXTN_SIGALGS|PROTOSSL_HELLOEXTN_ALPN)
// default extensions
#define PROTOSSL_HELLOEXTN_DEFAULT (PROTOSSL_HELLOEXTN_ALL)

//! elliptic curves
#define PROTOSSL_CURVE_SECP256R1                (1 << 0)
#define PROTOSSL_CURVE_SECP384R1                (1 << 1)
#define PROTOSSL_CURVE_X25519                   (1 << 2)
#define PROTOSSL_CURVE_X448                     (1 << 3)

//! all curves
#define PROTOSSL_CURVE_ALL (\
    PROTOSSL_CURVE_SECP256R1|PROTOSSL_CURVE_SECP384R1|PROTOSSL_CURVE_X25519|PROTOSSL_CURVE_X448)
//! default curves
#define PROTOSSL_CURVE_DEFAULT (PROTOSSL_CURVE_ALL)

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! identity fields for X509 issuer/subject
typedef struct ProtoSSLCertIdentT
{
    char strCountry[32];
    char strState[32];
    char strCity[32];
    char strOrg[32];
    char strUnit[256];
    char strCommon[64];
} ProtoSSLCertIdentT;

//! alert info, returned by ProtoSSLStat() 'alrt' selector after an SSL alert has been received
typedef struct ProtoSSLAlertDescT
{
    int32_t iAlertType;
    const char *pAlertDesc;
} ProtoSSLAlertDescT;

//! cert info, returned by ProtoSSLStat() 'cert' selector after certificate failure
typedef struct ProtoSSLCertInfoT
{
    ProtoSSLCertIdentT Ident;
    int32_t iKeyModSize;
} ProtoSSLCertInfoT;

//! state for pkcs1 operations
typedef struct ProtoSSLPkcs1T
{
    CryptRSAT RSAContext;   //!< context used for rsa operations
} ProtoSSLPkcs1T;

// opaque module state ref
typedef struct ProtoSSLRefT ProtoSSLRefT;

// forward declaration of sockaddr
struct sockaddr;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// protossl startup
DIRTYCODE_API int32_t ProtoSSLStartup(void);

// protossl shutdown
DIRTYCODE_API void ProtoSSLShutdown(void);

// allocate an SSL connection and prepare for use
DIRTYCODE_API ProtoSSLRefT *ProtoSSLCreate(void);

// reset connection back to base state.
DIRTYCODE_API void ProtoSSLReset(ProtoSSLRefT *pState);

// destroy the module and release its state
DIRTYCODE_API void ProtoSSLDestroy(ProtoSSLRefT *pState);

// give time to module to do its thing (should be called periodically to allow module to perform work)
DIRTYCODE_API void ProtoSSLUpdate(ProtoSSLRefT *pState);

// Accept an incoming connection.
DIRTYCODE_API ProtoSSLRefT* ProtoSSLAccept(ProtoSSLRefT *pState, int32_t iSecure, struct sockaddr *pAddr, int32_t *pAddrlen);

// Create a socket bound to the given address.
DIRTYCODE_API int32_t ProtoSSLBind(ProtoSSLRefT *pState, const struct sockaddr *pAddr, int32_t pAddrlen);

// make a secure connection to a server.
DIRTYCODE_API int32_t ProtoSSLConnect(ProtoSSLRefT *pState, int32_t iSecure, const char *pAddr, uint32_t uAddr, int32_t iPort);

// disconnect from the server.
DIRTYCODE_API int32_t ProtoSSLDisconnect(ProtoSSLRefT *pState);

// Start listening for an incoming connection.
DIRTYCODE_API int32_t ProtoSSLListen(ProtoSSLRefT *pState, int32_t iBacklog);

// send secure data to the server.
DIRTYCODE_API int32_t ProtoSSLSend(ProtoSSLRefT *pState, const char *pBuffer, int32_t iLength);

// receive secure data from the server.
DIRTYCODE_API int32_t ProtoSSLRecv(ProtoSSLRefT *pState, char *pBuffer, int32_t iLength);

// return the current module status (according to selector)
DIRTYCODE_API int32_t ProtoSSLStat(ProtoSSLRefT *pState, int32_t iSelect, void *pBuffer, int32_t iLength);

// control module behavior
DIRTYCODE_API int32_t ProtoSSLControl(ProtoSSLRefT *pState, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue);

// add an X.509 CA certificate that will be recognized in future transactions
DIRTYCODE_API int32_t ProtoSSLSetCACert(const uint8_t *pCACert, int32_t iCertSize);

// same as ProtoSSLSetCACert(), but certs are not validated at load time
DIRTYCODE_API int32_t ProtoSSLSetCACert2(const uint8_t *pCACert, int32_t iCertSize);

// validate all CAs that have not already been validated
DIRTYCODE_API int32_t ProtoSSLValidateAllCA(void);

// clear all CA certs
DIRTYCODE_API void ProtoSSLClrCACerts(void);

// generate a pkcs1.5 rsa signature - init
DIRTYCODE_API int32_t ProtoSSLPkcs1GenerateInit(ProtoSSLPkcs1T *pPkcs1, const uint8_t *pHashData, int32_t iHashLen, int32_t iHashType, int32_t iModSize, const CryptBinaryObjT *pPrimeP, const CryptBinaryObjT *pPrimeQ, const CryptBinaryObjT *pExponentP, const CryptBinaryObjT *pExponentQ, const CryptBinaryObjT *pCoefficient);

// generate a pkcs1.5 rsa signature
DIRTYCODE_API int32_t ProtoSSLPkcs1GenerateUpdate(ProtoSSLPkcs1T *pPkcs1, int32_t iNumIterations, uint8_t *pSigData, int32_t iSigSize);

// verify the pkcs1.5 rsa signature
DIRTYCODE_API int32_t ProtoSSLPkcs1Verify(const uint8_t *pSignature, int32_t iSigLen, const uint8_t *pHashData, int32_t iHashLen, int32_t iHashType, const uint8_t *pMod, int32_t iModSize, const uint8_t *pExp, int32_t iExpSize);

#ifdef __cplusplus
}
#endif

//@}

#endif // _protossl_h

