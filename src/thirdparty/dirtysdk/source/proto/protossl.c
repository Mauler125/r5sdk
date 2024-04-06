/*H********************************************************************************/
/*!
    \File protossl.c

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
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtycert.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "DirtySDK/crypt/cryptdef.h"
#include "DirtySDK/crypt/cryptaes.h"
#include "DirtySDK/crypt/cryptchacha.h"
#include "DirtySDK/crypt/cryptcurve.h"
#include "DirtySDK/crypt/cryptgcm.h"
#include "DirtySDK/crypt/crypthash.h"
#include "DirtySDK/crypt/crypthmac.h"
#include "DirtySDK/crypt/cryptmd5.h"
#include "DirtySDK/crypt/cryptrand.h"
#include "DirtySDK/crypt/cryptrsa.h"
#include "DirtySDK/crypt/cryptsha1.h"
#include "DirtySDK/util/base64.h"

#include "DirtySDK/proto/protossl.h"

#include "cryptrandpriv.h"

/*** Defines **********************************************************************/

#define DEBUG_RAW_DATA  (DIRTYCODE_LOGGING && 0)  // display raw debug data
#define DEBUG_ENC_PERF  (DIRTYCODE_LOGGING && 0)  // show verbose crypto performance
#define DEBUG_MSG_LIST  (DIRTYCODE_LOGGING && 0)  // list the message states
#define DEBUG_ALL_OBJS  (DIRTYCODE_LOGGING && 0)  // display all headers/objects while parsing
#define DEBUG_VAL_CERT  (DIRTYCODE_LOGGING && 0)  // display verbose certificate validation info
#define DEBUG_RES_SESS  (DIRTYCODE_LOGGING && 0)  // display verbose session resume info
#define DEBUG_MOD_PRNT  (DIRTYCODE_LOGGING && 0)  // display public key modulus when parsing certificate (useful when adding new CA)

#define SSL_VERBOSE_DEFAULT         (1)

#define SSL_CACERTFLAG_NONE         (0)
#define SSL_CACERTFLAG_GOSCA        (1)                     //!< identifies a GOS CA, for use on internal (*.ea.com) servers only
#define SSL_CACERTFLAG_CAPROVIDER   (2)                     //!< identifies a CA that can function as a CA provider (used for cert loading)

#define SSL_ERR_GOSCA_INVALIDUSE    (-100)                  //!< error indicating invalid use of a GOS CA to access a non ea.com domain
#define SSL_ERR_CERT_INVALIDDATE    (-101)                  //!< error indicating a CA certificate is expired
#define SSL_ERR_CAPROVIDER_INVALID  (-102)                  //!< attempt to use an invalid CA for CA provider use

#define SSL_MIN_PACKET      5                               // smallest packet (5 bytes framing)
#define SSL_CRYPTO_PAD      2048                            // maximum crypto overhead ref: http://tools.ietf.org/html/rfc5246#section-6.2.3
#define SSL_RAW_PACKET      16384                           // max raw data size ref: http://tools.ietf.org/html/rfc5246#section-6.2.1
#define SSL_RCVMAX_PACKET   ((SSL_RAW_PACKET+SSL_CRYPTO_PAD)*2) // max recv packet buffer; sized to allow a handshake packet up to twice the ssl frame size
#define SSL_SNDMAX_PACKET   (SSL_RAW_PACKET)                // max send packet buffer
#define SSL_SNDOVH_PACKET   (384)                           // reserve space for header/mac in send packet
#define SSL_SNDLIM_PACKET   (SSL_SNDMAX_PACKET-SSL_SNDOVH_PACKET) // max send user payload size

#define SSL_KEYBLOCK_LEN    (512)                           //!< length of keyblock that holds generated key material/tls1.3 secrets/keys/ivs
#define SSL_KEYMATERIAL_LEN (256)                           //!< length of key material we generate for TLS1.2 and prior

#define SSL_SIG_MAX         (512)                           //!< max signature size (4096 bit)

#define SSL3_SESSION_TICKET_MAX (1536)                      //!< max session ticket size
#define SSL3_SESSION_NONCE_MAX  (256)                       //!< max session ticket nonce size

#define SSL_SESSHIST_MAX    (32)                            // max number of previous sessions that will be tracked for possible later resumption
#define SSL_SESSVALIDTIME_MAX (2*60*60*1000)                // session validity cached for a max of two hours (tls1.2 and prior)
#define SSL_SESSID_SIZE     (32)

#define SSL_CERTVALID_MAX     (32)
#define SSL_CERTVALIDTIME_MAX (2*60*60*1000)                // certificate validity cached for a max of two hours

#define SSL_FINGERPRINT_SIZE  (CRYPTSHA1_HASHSIZE)

// min/default/max protocol versions supported
#define SSL3_VERSION_MIN        (PROTOSSL_VERSION_TLS1_0)
#define SSL3_VERSION            (PROTOSSL_VERSION_TLS1_2)
#define SSL3_VERSION_MAX        (PROTOSSL_VERSION_TLS1_3)

// internal names for TLS protocol versions
#define SSL3_SSLv3              (0x0300)                    // defined internally strictly to identify SSLv3 cipher suites that are still supported
#define SSL3_TLS1_0             (PROTOSSL_VERSION_TLS1_0)
#define SSL3_TLS1_1             (PROTOSSL_VERSION_TLS1_1)
#define SSL3_TLS1_2             (PROTOSSL_VERSION_TLS1_2)
#define SSL3_TLS1_3             (PROTOSSL_VERSION_TLS1_3)

// TLS record types
#define SSL3_REC_CIPHER         20  // cipher change record
#define SSL3_REC_ALERT          21  // alert record
#define SSL3_REC_HANDSHAKE      22  // handshake record
#define SSL3_REC_APPLICATION    23  // application data record

// TLS handshake header length
#define SSL3_MSG_HEADER_SIZE    4

// TLS handshake message types; see https://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml#tls-parameters-7
#define SSL3_MSG_HELLO_REQUEST          0   // hello request
#define SSL3_MSG_CLIENT_HELLO           1   // client hello
#define SSL3_MSG_SERVER_HELLO           2   // server hello
#define SSL3_MSG_NEW_SESSION_TICKET     4   // session ticket
#define SSL3_MSG_END_OF_EARLY_DATA      5   // end of early data (tls1.3)
#define SSL3_MSG_ENCRYPTED_EXTENSIONS   8   // encrypted extensions (tls1.3)
#define SSL3_MSG_CERTIFICATE            11  // certificate
#define SSL3_MSG_SERVER_KEY             12  // server key data
#define SSL3_MSG_CERT_REQ               13  // certificate request
#define SSL3_MSG_SERVER_DONE            14  // server handshake done
#define SSL3_MSG_CERT_VERIFY            15  // certificate verify
#define SSL3_MSG_CLIENT_KEY             16  // client key data
#define SSL3_MSG_FINISHED               20  // handshake finished
#define SSL3_MSG_KEY_UPDATE             24  // key update (tls1.3)
#define SSL3_MSG_MESSAGE_HASH           254 // synthetic message for HRR

// MAC types (ident equals size)
#define SSL3_MAC_NULL           0   // none
#define SSL3_MAC_MD5            16  // md5
#define SSL3_MAC_SHA            20  // sha1
#define SSL3_MAC_SHA256         32  // sha2-256
#define SSL3_MAC_SHA384         48  // sha2-384
#define SSL3_MAC_SHA512         64  // sha2-512
#define SSL3_MAC_MAXSIZE        (CRYPTHASH_MAXDIGEST) // maximum MAC size

// PRF types
#define SSL3_PRF_NULL           0   // no cipher-suite defined prf
#define SSL3_PRF_SHA256         CRYPTHASH_SHA256
#define SSL3_PRF_SHA384         CRYPTHASH_SHA384

// key types
#define SSL3_KEY_NULL           0   // no record key exchange
#define SSL3_KEY_RSA            1   // use rsa
#define SSL3_KEY_ECDHE          2   // use ecdhe

// signing types
#define SSL3_SIGN_RSA           1
#define SSL3_SIGN_ECDSA         64

// key lengths
#define SSL3_KEYLEN_NULL        0
#define SSL3_KEYLEN_128         16  // 128-bit key length in bytes
#define SSL3_KEYLEN_256         32  // 256-bit key length in bytes

// cipher types
#define SSL3_ENC_NULL           0   // no record encryption
#define SSL3_ENC_AES            1   // use aes
#define SSL3_ENC_GCM            2   // use gcm
#define SSL3_ENC_CHACHA         3   // use chacha

// hash ids
#define SSL3_HASHID_NONE        (0)
#define SSL3_HASHID_MD5         (1)
#define SSL3_HASHID_SHA1        (2)
#define SSL3_HASHID_SHA224      (3)
#define SSL3_HASHID_SHA256      (4)
#define SSL3_HASHID_SHA384      (5)
#define SSL3_HASHID_SHA512      (6)
#define SSL3_HASHID_MAX         (SSL3_HASHID_SHA512)

// signature algorithm ids
#define SSL3_SIGALG_NONE        (0)
#define SSL3_SIGALG_RSA         (1)
#define SSL3_SIGALG_DSA         (2)
#define SSL3_SIGALG_ECDSA       (3)

// signature verification schemes
#define SSL3_SIGVERIFY_NONE         (0)
#define SSL3_SIGVERIFY_RSA_PKCS1    (1)
#define SSL3_SIGVERIFY_RSA_PSS      (2)

// curve types
enum
{
    SSL3_CURVE_NONE = 0xff,
    SSL3_CURVE_SECP256R1 = 0,
    SSL3_CURVE_X25519,
    SSL3_CURVE_SECP384R1,
    SSL3_CURVE_X448,
    SSL3_NUM_CURVES
};
// index of default ec
#define SSL3_CURVE_DEFAULT      (SSL3_CURVE_SECP256R1)
// index of max ec
#define SSL3_CURVE_MAX          (SSL3_CURVE_X448)

/* signature schemes (tls1.3, but overlaps with previous signature algorithms); see
   https://tools.ietf.org/html/rfc8446#section-4.2.3 */

// RSASSA-PKCS1-v1_5 algorithms
#define SSL3_SIGSCHEME_RSA_PKCS1_MD5            (0x0101) // deprecated
#define SSL3_SIGSCHEME_RSA_PKCS1_SHA1           (0x0201) // legacy
#define SSL3_SIGSCHEME_RSA_PKCS1_SHA256         (0x0401)
#define SSL3_SIGSCHEME_RSA_PKCS1_SHA384         (0x0501)
#define SSL3_SIGSCHEME_RSA_PKCS1_SHA512         (0x0601)
// ECDSA algorithms
#define SSL3_SIGSCHEME_ECDSA_SHA1               (0x0203) // legacy, required for TLS1.0/1.1 ECDSA cipher suites
#define SSL3_SIGSCHEME_ECDSA_SHA256             (0x0403)
#define SSL3_SIGSCHEME_ECDSA_SHA384             (0x0503)
#define SSL3_SIGSCHEME_ECDSA_SHA512             (0x0603)
// RSASSA-PSS algorithms with public key OID rsaEncryption
#define SSL3_SIGSCHEME_RSA_PSS_RSAE_SHA256      (0x0804)
#define SSL3_SIGSCHEME_RSA_PSS_RSAE_SHA384      (0x0805)
#define SSL3_SIGSCHEME_RSA_PSS_RSAE_SHA512      (0x0806)
// EdDSA algorithms
#define SSL3_SIGSCHEME_EDDSA_ED25519            (0x0807) // unsupported
#define SSL3_SIGSCHEME_EDDSA_ED448              (0x0808) // unsupported
// RSASSA-PSS algorithms with public key OID RSASSA-PSS
#define SSL3_SIGSCHEME_RSA_PSS_PSS_SHA256       (0x0809)
#define SSL3_SIGSCHEME_RSA_PSS_PSS_SHA384       (0x080a)
#define SSL3_SIGSCHEME_RSA_PSS_PSS_SHA512       (0x080b)

// curve types
#define SSL3_CURVETYPE_EXPLICIT_PRIME   (1)
#define SSL3_CURVETYPE_EXPLICIT_CHAR    (2)
#define SSL3_CURVETYPE_NAMED_CURVE      (3)

// TLS alert defines

// alert level
#define SSL3_ALERT_LEVEL_WARNING   1
#define SSL3_ALERT_LEVEL_FATAL     2

// alert identifiers
#define SSL3_ALERT_DESC_CLOSE_NOTIFY                0
#define SSL3_ALERT_DESC_UNEXPECTED_MESSAGE          10
#define SSL3_ALERT_DESC_BAD_RECORD_MAC              20
#define SSL3_ALERT_DESC_DECRYPTION_FAILED           21  // reserved
#define SSL3_ALERT_DESC_RECORD_OVERFLOW             22
#define SSL3_ALERT_DESC_DECOMPRESSION_FAILURE       30  // reserved
#define SSL3_ALERT_DESC_HANDSHAKE_FAILURE           40
#define SSL3_ALERT_DESC_NO_CERTIFICATE              41  // reserved
#define SSL3_ALERT_DESC_BAD_CERTFICIATE             42
#define SSL3_ALERT_DESC_UNSUPPORTED_CERTIFICATE     43
#define SSL3_ALERT_DESC_CERTIFICATE_REVOKED         44
#define SSL3_ALERT_DESC_CERTIFICATE_EXPIRED         45
#define SSL3_ALERT_DESC_CERTIFICATE_UNKNOWN         46
#define SSL3_ALERT_DESC_ILLEGAL_PARAMETER           47
// the following alert types are all TLS only
#define SSL3_ALERT_DESC_UNKNOWN_CA                  48
#define SSL3_ALERT_DESC_ACCESS_DENIED               49
#define SSL3_ALERT_DESC_DECODE_ERROR                50
#define SSL3_ALERT_DESC_DECRYPT_ERROR               51
#define SSL3_ALERT_DESC_EXPORT_RESTRICTION          60 // reserved
#define SSL3_ALERT_DESC_PROTOCOL_VERSION            70
#define SSL3_ALERT_DESC_INSUFFICIENT_SECURITY       71
#define SSL3_ALERT_DESC_INTERNAL_ERROR              80
#define SSL3_ALERT_DESC_INAPPROPRIATE_FALLBACK      86
#define SSL3_ALERT_DESC_USER_CANCELLED              90
#define SSL3_ALERT_DESC_NO_RENEGOTIATION            100 // reserved
#define SSL3_ALERT_DESC_MISSING_EXTENSION           109
#define SSL3_ALERT_DESC_UNSUPPORTED_EXTENSION       110
#define SSL3_ALERT_DESC_CERTIFICATE_UNOBTAINABLE    111
#define SSL3_ALERT_DESC_UNRECOGNIZED_NAME           112
#define SSL3_ALERT_DESC_BAD_CERTIFICATE_STATUS      113
#define SSL3_ALERT_DESC_BAD_CERTIFICATE_HASH        114
#define SSL3_ALERT_DESC_UNKNOWN_PSK_IDENTITY        115
#define SSL3_ALERT_DESC_CERTIFICATE_REQUIRED        116
#define SSL3_ALERT_DESC_NO_APPLICATION_PROTOCOL     120

// certificate setup
#define SSL_CERT_X509   1

// tls extension types
#define SSL_EXTN_SERVER_NAME            (0x0000)    //!< identifier for server name extension
#define SSL_EXTN_ELLIPTIC_CURVES        (0x000a)    //!< identifier for elliptic curves extension and supported curves
#define SSL_EXTN_SIGNATURE_ALGORITHMS   (0x000d)    //!< identifier for signature algorithms extension
#define SSL_EXTN_ALPN                   (0x0010)    //!< identifier for ALPN (Application Level Protocol Negotiation) extension
#define SSL_EXTN_PRE_SHARED_KEY         (0x0029)    //!< identifier for pre_shared_key extension
#define SSL_EXTN_EARLY_DATA             (0x002a)    //!< identifier for early_data extension (not supported)
#define SSL_EXTN_SUPPORTED_VERSIONS     (0x002b)    //!< identifier for supported_versions extension
#define SSL_EXTN_COOKIE                 (0x002c)    //!< identifier for cookie extension
#define SSL_EXTN_PSK_MODES              (0x002d)    //!< identifier for psk_key_exchange_modes extension
#define SSL_EXTN_CERT_AUTH              (0x002f)    //!< identifier for certificate_authorities extension (not supported)
#define SSL_EXTN_OID_FILTER             (0x0030)    //!< idetntifer for oid_filter extension (not supported)
#define SSL_EXTN_POST_HANDSHAKE_AUTH    (0x0031)    //!< identifier for post_handshake_auth extension (not supported)
#define SSL_EXTN_SIGALGS_CERT           (0x0032)    //!< identifier for signature_algorithms_cert extension (not supported)
#define SSL_EXTN_KEY_SHARE              (0x0033)    //!< identifier for key_share extension
#define SSL_EXTN_MAX                    (SSL_EXTN_KEY_SHARE)    //!< max defined extension; renegotiation_info is special case

#define SSL_EXTN_RENEGOTIATION_INFO     (0xff01)     //!< renegotiation_info extension (not supported)

#define SSL_EXTN_ALL                    (0xffff)    //!< special value used to indicate all extensions, not a bitfield

// ALPN defines
#define SSL_ALPN_MAX_PROTOCOLS          (4)         //!< max supported ALPN protocols

// tls states
enum
{
    // network states
    ST_IDLE = 0,
    ST_ADDR,
    ST_CONN,
    ST_WAIT_CONN,
    ST_WAIT_CA,                 // waiting for the CA to be fetched

    // ssl states
    ST3_SEND_HELLO = 20,
    ST3_RECV_HELLO,
    ST3_SEND_HELLO_RETRY,
    ST3_SEND_EXTN,
    ST3_SEND_CERT,
    ST3_SEND_CERT_REQ,
    ST3_SEND_KEY,
    ST3_SEND_DONE,
    ST3_SEND_VERIFY,
    ST3_SEND_CHANGE,
    ST3_SEND_FINISH,
    ST3_RECV_CHANGE,
    ST3_RECV_FINISH,
    // post-finish states
    ST3_SEND_HELLO_REQUEST,
    ST3_PROC_ASYNC,             // synthetic state for iterative async operations
    ST3_SECURE,
    ST_UNSECURE,

    // failure states
    ST_FAIL = 0x1000,
    ST_FAIL_DNS,
    ST_FAIL_CONN,
    ST_FAIL_CONN_SSL2,
    ST_FAIL_CONN_NOTSSL,
    ST_FAIL_CONN_MINVERS,
    ST_FAIL_CONN_MAXVERS,
    ST_FAIL_CONN_NOCIPHER,
    ST_FAIL_CONN_NOCURVE,
    ST_FAIL_CERT_NONE,
    ST_FAIL_CERT_INVALID,
    ST_FAIL_CERT_HOST,
    ST_FAIL_CERT_NOTRUST,
    ST_FAIL_CERT_BADDATE,
    ST_FAIL_SETUP,
    ST_FAIL_SECURE,
    ST_FAIL_CERT_REQUEST
};

// ASN.1 definitions
#define ASN_CLASS_UNIV      0x00
#define ASN_CLASS_APPL      0x40
#define ASN_CLASS_CONT      0x80
#define ASN_CLASS_PRIV      0xc0

#define ASN_PRIMITIVE       0x00
#define ASN_CONSTRUCT       0x20

#define ASN_TYPE_BOOLEAN    0x01
#define ASN_TYPE_INTEGER    0x02
#define ASN_TYPE_BITSTRING  0x03
#define ASN_TYPE_OCTSTRING  0x04
#define ASN_TYPE_NULL       0x05
#define ASN_TYPE_OBJECT     0x06
#define ASN_TYPE_UTF8STR    0x0c
#define ASN_TYPE_SEQN       0x10
#define ASN_TYPE_SET        0x11
#define ASN_TYPE_PRINTSTR   0x13
#define ASN_TYPE_T61        0x14
#define ASN_TYPE_IA5        0x16
#define ASN_TYPE_UTCTIME    0x17
#define ASN_TYPE_GENERALIZEDTIME 0x18
#define ASN_TYPE_UNICODESTR 0x1e

#define ASN_IMPLICIT_TAG   0x80
#define ASN_EXPLICIT_TAG   0xa0

enum {
    ASN_OBJ_NONE = 0,
    ASN_OBJ_COUNTRY,
    ASN_OBJ_STATE,
    ASN_OBJ_CITY,
    ASN_OBJ_ORGANIZATION,
    ASN_OBJ_UNIT,
    ASN_OBJ_COMMON,
    ASN_OBJ_SUBJECT_ALT,
    ASN_OBJ_BASIC_CONSTRAINTS,
    ASN_OBJ_MD5,
    ASN_OBJ_SHA1,
    ASN_OBJ_SHA256,
    ASN_OBJ_SHA384,
    ASN_OBJ_SHA512,
    ASN_OBJ_RSA_PKCS_KEY,
    ASN_OBJ_ECDSA_KEY,
    ASN_OBJ_RSA_PKCS_MD5,
    ASN_OBJ_RSA_PKCS_SHA1,
    ASN_OBJ_RSA_PKCS_SHA256,
    ASN_OBJ_RSA_PKCS_SHA384,
    ASN_OBJ_RSA_PKCS_SHA512,
    ASN_OBJ_RSASSA_PSS,
    ASN_OBJ_ECDSA_SHA256,
    ASN_OBJ_ECDSA_SHA384,
    ASN_OBJ_ECDSA_SHA512,
    ASN_OBJ_ECDSA_SECP256R1,
    ASN_OBJ_ECDSA_SECP384R1,
    ASN_OBJ_ECDSA_SECP521R1,
    ASN_OBJ_PKCS1_MGF1,
    ASN_OBJ_COUNT,
};

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! ASN object table
typedef struct ASNObjectT
{
    int32_t iType;              //!< symbolic type
    int32_t iSize;              //!< size of object
    uint8_t strData[16];        //!< object identifier
} ASNObjectT;

//! generic binary certificate container plus some parsed certificate info
typedef struct CertificateDataT
{
    int32_t iCrvType;
    int32_t iKeyType;
    int32_t iSigType;
    int32_t iCertSize;
    uint8_t aCertData[1];
} CertificateDataT;

//! structure holding .pem certificate signature begin/end strings
typedef struct CertificateSignatureT
{
    const char *pCertBeg;
    const char *pCertEnd;
    uint32_t uCertType;
} CertificateSignatureT;

//! note: this layout differs somewhat from x.509, but I think it
//! makes more sense this way
typedef struct X509CertificateT
{
    ProtoSSLCertIdentT Issuer;      //!< certificate was issued by (matches subject in another cert)
    ProtoSSLCertIdentT Subject;     //!< certificate was issued for this site/authority

    char strGoodFrom[32];           //!< good from this date
    char strGoodTill[32];           //!< until this date
    uint64_t uGoodFrom;
    uint64_t uGoodTill;

    const uint8_t *pSubjectAlt;     //!< subject alternative name, if present
    int32_t iSubjectAltLen;         //!< subject alternative length

    int32_t iSerialSize;            //!< certificate serial number
    uint8_t SerialData[32];         //!< certificate serial data

    int32_t iSigType;               //!< digital signature type
    int32_t iSigHash;               //!< signature hash algorithm
    int32_t iSigSize;               //!< size of signature data
    int32_t iSigSalt;               //!< salt length when signature algorithm is rsassa-pss
    int32_t iMgfHash;               //!< message-generation function hash when signature algorithm is rsassa-pss
    uint8_t SigData[SSL_SIG_MAX];   //!< digital signature data

    int32_t iKeyType;               //!< public key algorithm type
    int32_t iCrvType;               //!< type of curve, if keytype==ecdsa
    int32_t iKeyModSize;            //!< size of public key modulus (rsa) / curve data (ecdsa)
    uint8_t KeyModData[SSL_SIG_MAX];//!< public key modulus
    int32_t iKeyExpSize;            //!< size of public key exponent
    uint8_t KeyExpData[16];         //!< public key exponent

    // iMaxHeight is valid only if iCertIsCA is set
    int32_t iCertIsCA;              //!< whether this cert can be used as a CA cert
    int32_t iMaxHeight;             //!< the pathLenConstraints of a CA, 0 means no limit/field not present

    int32_t iHashSize;              //!< size of certificate signature hash
    CryptHashTypeE eHashType;       //!< type of certificate signature hash
    uint8_t HashData[CRYPTHASH_MAXDIGEST];
} X509CertificateT;

//! private key definition
typedef struct X509PrivateKeyT
{
    CryptBinaryObjT Modulus;            //!< key modulus
    CryptBinaryObjT PublicExponent;     //!< public key exponent
    CryptBinaryObjT PrivateExponent;    //!< private key exponent
    CryptBinaryObjT PrimeP;             //!< prime factor (p) of modulus
    CryptBinaryObjT PrimeQ;             //!< prime factor (q) of modulus
    CryptBinaryObjT ExponentP;          //!< exponent d mod p-1
    CryptBinaryObjT ExponentQ;          //!< exponent d mod q-1
    CryptBinaryObjT Coefficient;        //!< inverse of q mod p
    char strPrivKeyData[4096];          //!< buffer to hold private key data
} X509PrivateKeyT;

//! defines data for formatting/processing Finished packet
struct SSLFinished
{
    char strLabel[16];
    uint8_t uNextState[2]; // [not resuming, resuming]
};

//! define data for handshake transitions
typedef struct SSLHandshakeMapT
{
    int8_t iCurMsg;
    int8_t iNxtMsg;
} SSLHandshakeMapT;

//! minimal certificate authority data (just enough to validate another certificate)
typedef struct ProtoSSLCACertT
{
    ProtoSSLCertIdentT Subject;     //!< identity of this certificate authority
    uint32_t uFlags;                //!< SSL_CACERTFLAG_*

    int32_t iKeyType;               //!< public key algorithm type
    int32_t iCrvType;               //!< type of curve, if keytype==ecdsa

    int32_t iKeyModSize;            //!< size of public key modulus
    const uint8_t *pKeyModData;     //!< public key modulus for signature verification

    int32_t iKeyExpSize;            //!< size of public key exponent
    uint8_t KeyExpData[16];         //!< public key exponent

    int32_t iMemGroup;              //!< memgroup cert was allocated with (zero == static)
    void   *pMemGroupUserData;      //!< memgroup user data

    X509CertificateT *pX509Cert;    //!< X509 certificate, if this cert has not yet been validated
    struct ProtoSSLCACertT *pNext;  //!< link to next cert in list
} ProtoSSLCACertT;

//! async op function pointer type
typedef int32_t (AsyncOpT)(ProtoSSLRefT *pState);

//! info for async op
typedef struct AsyncOpInfoT
{
    AsyncOpT *pAsyncOp;             //!< async operation to execute, when in ASYNC state
    int32_t iNextState;             //!< next state, when in ASYNC
    int32_t iFailState;             //!< fail state, if async op fails
    int32_t iFailAlert;             //!< fail alert, if async op fails
} AsyncOpInfoT;

//! cipher parameter lookup structure
typedef struct CipherSuiteT
{
    uint16_t uIdent;                //!< two-byte identifier
    uint16_t uMinVers;              //!< minimum required SSL version
    uint8_t uKey;                   //!< key exchange algorithm (SSL3_KEY_*)
    uint8_t uLen;                   //!< key length (SSL3_KEYLEN_*)
    uint8_t uSig;                   //!< signature algorithm (SSL3_SIGALG_*)
    uint8_t uEnc;                   //!< encryption algorithm (SSL3_ENC_*)
    uint8_t uMac;                   //!< MAC digest size (SSL3_MAC_*)
    uint8_t uMacType;               //!< MAC digest type (CRYPTHASH_*)
    uint8_t uVecSize;               //!< explicit IV size
    uint8_t uPrfType;               //!< cipher-suite PRF type (SSL3_PRF_*)
    // no explicit pad here to make static initialization cleaner
    uint32_t uId;                   //!< PROTOSSL_CIPHER_*
    char    strName[66];            //!< cipher name
} CipherSuiteT;

//! elliptic curve used for key exchange
typedef struct EllipticCurveT
{
    uint16_t uIdent;                //!< two-byte identifier
    uint16_t uId;                   //!< PROTOSSL_CURVE_*
    char strName[32];               //!< curve name
} EllipticCurveT;

//! session info, used for tls1.2 and prior resume
typedef struct SessionInfoT
{
    uint16_t uSslVersion;               //!< ssl version used for connection
    uint16_t uCipherId;                 //!< cipher used for connection
    uint8_t SessionId[SSL_SESSID_SIZE]; //!< session id used to identify session
    uint8_t MasterSecret[48];       //!< master secret generated for session
} SessionInfoT;

//! session ticket data (tls1.3 session tickets only, not supported in previous versions)
typedef struct SessionTicketT
{
    CryptHashTypeE eHashType; 
    uint32_t uRecvTime;
    uint32_t uLifetime;
    uint32_t uAgeAdd;
    uint32_t uMaxEarlyDataSize;
    uint16_t uTickLen;
    uint16_t uExtnLen;
    uint8_t  uNonceLen;
    uint8_t  aResumeKey[CRYPTHASH_MAXDIGEST];
    uint8_t  aTicketNonce[SSL3_SESSION_NONCE_MAX];
    uint8_t  aTicketData[SSL3_SESSION_TICKET_MAX];
} SessionTicketT;

//! secure session history, used for ssl resume
typedef struct SessionHistoryT
{
    char strHost[256];              //!< hostname for the session
    uint16_t uPort;                 //!< port for the session
    uint8_t  bSessionTicket;        //!< TRUE if session ticket (tls1.3) else tls1.2-style resume info
    uint8_t  _pad;
    uint32_t uSessionUseTick;       //!< tick when session was last used
    uint32_t uSessionExpTick;       //!< tick when session will expire
    union
    {
        SessionInfoT SessionInfo;
        SessionTicketT SessionTicket;
    };
} SessionHistoryT;

//! certificate validation history, used to skip validation of certificates we've already valdiated
typedef struct CertValidHistoryT
{
    uint32_t uCertValidTick;        //!< tick when certificate was validated
    uint32_t uCertSize;             //!< size of certificate data
    uint8_t FingerprintId[SSL_FINGERPRINT_SIZE]; //!< certificate fingerprint
} CertValidHistoryT;

//! alpn extension protocol information
typedef struct AlpnProtocolT
{
    uint8_t uLength;
    uint8_t _pad[3];
    char strName[256];
} AlpnProtocolT;

//! signature algorithm information
typedef struct SignatureAlgorithmT
{
    uint8_t uHashId;
    uint8_t uSigAlg;
} SignatureAlgorithmT;

//! signature scheme
typedef struct SignatureSchemeT
{
    uint16_t uIdent;                //!< two-byte signature scheme identifier
    SignatureAlgorithmT SigAlg;     //!< hash and signature algorithm used by the signature scheme
    uint8_t uVerifyScheme;          //!< verification algorithm (RSA signature schemes only)
    uint8_t uOidType;               //!< certificate oid used for signature scheme selection
} SignatureSchemeT;

//! signature generate/verify data for async op
typedef struct SignatureVerifyT
{
    const SignatureSchemeT *pSigScheme;
    const X509PrivateKeyT *pPrivateKey;
    uint8_t DsaContext[CRYPTCURVE_DSA_MAXSTATE];
    uint8_t aSigData[SSL_SIG_MAX];
    int32_t iSigSize;
    uint8_t aHashDigest[CRYPTHASH_MAXDIGEST];
    int32_t iHashSize;
    CryptHashTypeE eHashType;
    uint8_t aKeyData[128];
    int32_t iKeySize;
    int32_t iNextState;
    uint8_t bEccContextInitialized;
    uint8_t _pad[3];
} SignatureVerifyT;

//! extra state information required for secure connections (not allocated for unsecure connections)
typedef struct SecureStateT
{
    uint64_t uSendSeqn;             //!< send sequence number
    uint64_t uRecvSeqn;             //!< recv sequence number

    uint32_t uTimer;                //!< base of setup timing

    int32_t iSendProg;              //!< progress within send
    int32_t iSendSize;              //!< total bytes to send

    int32_t iRecvHead;              //!< progress receiving packet header
    int32_t iRecvProg;              //!< progress receiving packet data
    int32_t iRecvSize;              //!< total bytes to receive (tls record size)
    int32_t iRecvBase;              //!< progress decrypting packet data
    int32_t iRecvHshkProg;          //!< progress processing handshake messages
    int32_t iRecvHshkSize;          //!< size of current handshake message being processed (including header)

    const CipherSuiteT *pCipher;    //!< selected cipher suite
    const EllipticCurveT *pEllipticCurve; //!< selected elliptic curve
    const SignatureSchemeT *pSigScheme; //!< selected signature scheme
    uint32_t uSentCiphers;          //!< bitmask of ciphers that were sent in ClientHello
    uint16_t uRetryCipher;          //!< cipher ident sent by HelloRetryRequest (tls1.3)
    uint16_t _pad16;

    SignatureVerifyT SigVerify;

    uint8_t ClientRandom[32];       //!< clients random seed
    uint8_t ServerRandom[32];       //!< servers random seed
    uint8_t SessionId[SSL_SESSID_SIZE]; //!< session id
    uint16_t uSslVersion;           //!< ssl version of connection
    uint16_t uSslClientVersion;     //!< client-requested ssl version (validated in premaster secret)

    uint8_t bSessionResume;         //!< true if session is resumed during handshaking
    uint8_t bSendSecure;            //!< true if sending secure
    uint8_t bRecvSecure;            //!< true if receiving secure
    uint8_t bDateVerifyFailed;      //!< true if date verification of a cert in chain failed
    uint8_t bRSAContextInitialized; //!< true if we have initialized the RSAContext
    uint8_t bEccContextInitialized; //!< true if we have initialized the EccContext
    uint8_t bEccKeyGenerated;       //!< true if an Ecc Public Key has been generated
    uint8_t bSigGenerated;          //!< true if signature source has been generated
    uint8_t bRecvProc;              //!< true if packet has been processed by _RecvPacket()
    uint8_t bHelloRetry;            //!< true if we're in a HelloRetryRequest flow (tls1.3)
    uint8_t bSentCert;              //!< true if a certificate was sent in handshaking
    uint8_t bRecvCert;              //!< true if a certificate was received in handshaking
    uint8_t bSentSessionId;         //!< true if we sent a session identifier in clienthello
    uint8_t bRenegotiationInfo;     //!< true if renegotation info was sent by peer
    uint8_t uPubKeyLength;          //!< length of pubkey used for ECDHE key exchange
    int8_t  iCurMsg;                //!< current handshake message id, used for flow verification

    uint8_t PubKey[256];            //!< peer's public key, used in ECDHE key exchange (note that in the server flow, the server's public key lives here temporarily)
    uint8_t PreMasterKey[48];       //!< pre-master-key
    uint8_t MasterKey[48];          //!< master key
    uint8_t PreSharedKey[CRYPTHASH_MAXDIGEST]; //!< PSK for tls1.3, used in resume flow

    CryptRSAT RSAContext;

    uint8_t KeyBlock[SSL_KEYBLOCK_LEN];          //!< key block
    uint8_t *pServerMAC;            //!< server mac secret
    uint8_t *pClientMAC;            //!< client mac secret
    uint8_t *pServerKey;            //!< server key secret
    uint8_t *pClientKey;            //!< client key secret
    uint8_t *pServerInitVec;        //!< init vector (CBC ciphers)
    uint8_t *pClientInitVec;        //!< init vector (CBC ciphers)
    uint8_t *pServerSecret;         //!< tls1.3 server early/handshake/application/... secret
    uint8_t *pClientSecret;         //!< tls1.3 client early/handshake/application/... secret
    uint8_t *pResumeSecret;         //!< tls1.3 resumption secret

    const uint8_t *pCookie;         //!< pointer to cookie in receive buffer (tls1.3 extension)

    CryptMD5T HandshakeMD5;         //!< MD5 of all handshake data
    CryptSha1T HandshakeSHA;        //!< SHA of all handshake data
    CryptSha2T HandshakeSHA256;     //!< SHA256 of all handshake data
    CryptSha2T HandshakeSHA384;     //!< SHA384 of all handshake data
    CryptSha2T HandshakeSHA512;     //!< SHA512 of all handshake data
    uint8_t aFinishHash[CRYPTHASH_MAXDIGEST]; //!< handshake data storage for tls1.3 flow

    uint8_t aClientHelloHash[CRYPTSHA256_HASHSIZE]; //!< hash of sent clienthello; used in HelloRetryRequest flow

    CryptAesT ReadAes;              //!< aes read cipher state
    CryptAesT WriteAes;             //!< aes write cipher state

    CryptGcmT ReadGcm;              //!< gcm read cipher state
    CryptGcmT WriteGcm;             //!< gcm write cipher state

    CryptChaChaT ReadChaCha;        //!< chacha read cipher state
    CryptChaChaT WriteChaCha;       //!< chacha write cipher state

    X509CertificateT Cert;          //!< the x509 certificate

    uint8_t EccContext[CRYPTCURVE_DH_MAXSTATE]; //!< elliptic curve state

    char strAlpnProtocol[256];      //!< protocol negotiated using the alpn extension

    uint8_t RecvHead[SSL_MIN_PACKET];       //!< buffer to receive the SSL packet header
    uint8_t SendData[SSL_SNDMAX_PACKET];    //!< put at end to make references efficient, include space for debug fence
    uint8_t RecvData[SSL_RCVMAX_PACKET];    //!< put at end to make references efficient, include space for debug fence
} SecureStateT;

//! module state
struct ProtoSSLRefT
{
    SocketT *pSock;                 //!< comm socket
    HostentT *pHost;                //!< host entry

    // module memory group
    int32_t iMemGroup;              //!< module mem group id
    void *pMemGroupUserData;        //!< user data associated with mem group

    NetCritT SecureCrit;            //!< for guarding multithreaded access to secure state

    char strHost[256];              //!< host that we connect to.
    struct sockaddr PeerAddr;       //!< peer info
    struct sockaddr LocalAddr;      //!< cached value of our local address being used, some connections are very short lived making it otherwise difficult to read the local addr info reliably

    AsyncOpInfoT AsyncInfo;         //!< info for async op execution

    int32_t iState;                 //!< protocol state
    int32_t iClosed;                //!< socket closed flag
    SecureStateT *pSecure;          //!< secure state reference
    X509CertificateT *pCertToVal;   //!< server cert to be validated (used in ST_WAIT_CA state)
    ProtoSSLCertInfoT CertInfo;     //!< certificate info (used on failure)
    CertificateDataT *pCertificate; //!< server/client certificate
    char *pPrivateKey;              //!< server/client private key
    int32_t iPrivateKeyLen;         //!< private key length

    uint32_t uEnabledCiphers;       //!< enabled ciphers
    uint32_t uEnabledCurves;        //!< enabled curves
    int32_t iRecvBufSize;           //!< TCP recv buffer size; 0=default
    int32_t iSendBufSize;           //!< TCP send buffer size; 0=default
    int32_t iLastSocketError;       //!< Last socket error before closing the socket
    int32_t iCARequestId;           //!< CA request id (valid if positive)

    int32_t iMaxSendRate;           //!< max send rate (0=uncapped)
    int32_t iMaxRecvRate;           //!< max recv rate (0=uncapped)

    uint16_t uSslVersion;           //!< ssl version application wants us to use
    uint16_t uSslVersionMin;        //!< minimum ssl version application will accept

    uint8_t bAllowAnyCert;          //!< bypass certificate validation
    uint8_t bSessionResumeEnabled;  //!< trueif session resume is enabled (default)
    uint8_t bServer;                //!< true if server, else client
    uint8_t bReuseAddr;             //!< if true set SO_REUSEADDR
    uint8_t bNoDelay;               //!< if true set TCP_NODELAY on the socket
    uint8_t bKeepAlive;             //!< if true override tcp keep-alive (disabled by default)
    uint8_t bAsyncRecv;             //!< if true enable async receive on the ssl socket
    int8_t  iClientCertLevel;       //!< 0=no client cert required; 1=client cert requested, 2=client cert required
    uint8_t uHelloExtn;             //!< enabled ClientHello extensions
    int8_t  iCurveDflt;             //!< default elliptic curve to use in ClientHello (tls1.3)
    int8_t  iVerbose;               //!< spam level

    uint8_t bCertInfoSet;           //!< true if cert info has been set
    uint8_t uAlertLevel;            //!< level of most recent alert
    uint8_t uAlertValue;            //!< value of most recent alert
    uint8_t bAlertSent;             //!< true if most recent alert was sent, else false if it was received
    uint8_t _pad;

    uint32_t uKeepAliveTime;        //!< tcp keep-alive time; 0=default

    /* for alpn extension;
       on the client: these are the protocol preferences
       on the server: these are the protocols it supports */
    AlpnProtocolT aAlpnProtocols[SSL_ALPN_MAX_PROTOCOLS];
    uint16_t uNumAlpnProtocols;      //!< the number of alpn protocols in the list
    uint16_t uAlpnExtensionLength;   //!< the size of the list we encode in ClientHello (we calculate when we build the list)
};

//! global state
typedef struct ProtoSSLStateT
{
    //! critical section for locking access to state memory
    NetCritT StateCrit;

    //! previous session info, used for secure session share/resume
    SessionHistoryT SessionHistory[SSL_SESSHIST_MAX];

    //! validated certificate info
    CertValidHistoryT CertValidHistory[SSL_CERTVALID_MAX];

    // allocation identifiers
    int32_t iMemGroup;
    void *pMemGroupUserData;

    /* global default settings */

    int32_t iDfltVers;              //!< global version setting
    int32_t iDfltMinVers;           //!< global min version setting
    int32_t iDfltCiph;              //!< global cipher setting
    int32_t iDfltCurves;            //!< global curve setting
}ProtoSSLStateT;

/*** Function Prototypes **********************************************************/

// issue a CA request
static int32_t _ProtoSSLInitiateCARequest(ProtoSSLRefT *pState);

// update recv handshake hash
static void _ProtoSSLRecvHandshakeFinish(ProtoSSLRefT *pState);

// set async execution
static int32_t _ProtoSSLUpdateSetAsyncState(ProtoSSLRefT *pState, AsyncOpT *pAsyncOp, int32_t iNextState, int32_t iFailState, int32_t iFailAlert);

/*** Variables ********************************************************************/

static ProtoSSLStateT *_ProtoSSL_pState = NULL;

//! ServerRandom value that identifies a ServerHello as a HelloRetryRequest 
static const uint8_t _SSL3_HelloRetryRequestRandom[] =
{
    0xcf, 0x21, 0xad, 0x74, 0xe5, 0x9a, 0x61, 0x11, 0xbe, 0x1d, 0x8c, 0x02, 0x1e, 0x65, 0xb8, 0x91,
    0xc2, 0xa2, 0x11, 0x16, 0x7a, 0xbb, 0x8c, 0x5e, 0x07, 0x9e, 0x09, 0xe2, 0xc8, 0xa8, 0x33, 0x9c
};

//! ServerRandom trailing eight bytes that identify an illegitimate downgrade from tls1.3 to tls1.2
static const uint8_t _SSL3_ServerRandomDowngrade12[] =
{
    0x44, 0x4f, 0x57, 0x4e, 0x47, 0x52, 0x44, 0x01
};
//! ServerRandom trailing eight bytes that identify an illegitimate downgrade from tls1.3 to tls1.1 or previous
static const uint8_t _SSL3_ServerRandomDowngrade11[] =
{
    0x44, 0x4f, 0x57, 0x4e, 0x47, 0x52, 0x44, 0x00
};

//! supported ssl cipher suites in order of preference; see http://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml#tls-parameters-4
static const CipherSuiteT _SSL3_CipherSuite[] =
{
    // TLS1.3 cipher suites
    { 0x1301, SSL3_TLS1_3, SSL3_KEY_NULL,  SSL3_KEYLEN_128,  SSL3_SIGALG_NONE,  SSL3_ENC_GCM,    SSL3_MAC_NULL,   CRYPTHASH_NULL,   12, SSL3_PRF_SHA256, PROTOSSL_CIPHER_AES_128_GCM_SHA256,                        "TLS_AES_128_GCM_SHA256" },                        // TLS1.3 suite 2: gcm128+sha256
    { 0x1302, SSL3_TLS1_3, SSL3_KEY_NULL,  SSL3_KEYLEN_256,  SSL3_SIGALG_NONE,  SSL3_ENC_GCM,    SSL3_MAC_NULL,   CRYPTHASH_NULL,   12, SSL3_PRF_SHA384, PROTOSSL_CIPHER_AES_256_GCM_SHA384,                        "TLS_AES_256_GCM_SHA384" },                        // TLS1.3 suite 1: gcm256+sha384
    { 0x1303, SSL3_TLS1_3, SSL3_KEY_NULL,  SSL3_KEYLEN_256,  SSL3_SIGALG_NONE,  SSL3_ENC_CHACHA, SSL3_MAC_NULL,   CRYPTHASH_NULL,   12, SSL3_PRF_SHA256, PROTOSSL_CIPHER_CHACHA20_POLY1305_SHA256,                  "TLS_CHACHA20_POLY1305_SHA256" },                  // TLS1.3 suite 3: chacha20_poly1305+sha256
    // TLS1.2 AEAD cipher suites
    { 0xc02b, SSL3_TLS1_2, SSL3_KEY_ECDHE, SSL3_KEYLEN_128,  SSL3_SIGALG_ECDSA, SSL3_ENC_GCM,    SSL3_MAC_NULL,   CRYPTHASH_NULL,    4, SSL3_PRF_SHA256, PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,       "TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256" },       // ecdhe+ecdsa+gcm+sha256
    { 0xc02c, SSL3_TLS1_2, SSL3_KEY_ECDHE, SSL3_KEYLEN_256,  SSL3_SIGALG_ECDSA, SSL3_ENC_GCM,    SSL3_MAC_NULL,   CRYPTHASH_NULL,    4, SSL3_PRF_SHA384, PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,       "TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384" },       // ecdhe+ecdsa+gcm+sha384
    { 0xcca9, SSL3_TLS1_2, SSL3_KEY_ECDHE, SSL3_KEYLEN_256,  SSL3_SIGALG_ECDSA, SSL3_ENC_CHACHA, SSL3_MAC_NULL,   CRYPTHASH_NULL,   12, SSL3_PRF_SHA256, PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256, "TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256" }, // ecdhe+ecdsa+chacha+sha256
    { 0xc02f, SSL3_TLS1_2, SSL3_KEY_ECDHE, SSL3_KEYLEN_128,  SSL3_SIGALG_RSA,   SSL3_ENC_GCM,    SSL3_MAC_NULL,   CRYPTHASH_NULL,    4, SSL3_PRF_SHA256, PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_128_GCM_SHA256,         "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256" },         // ecdhe+rsa+gcm+sha256
    { 0xc030, SSL3_TLS1_2, SSL3_KEY_ECDHE, SSL3_KEYLEN_256,  SSL3_SIGALG_RSA,   SSL3_ENC_GCM,    SSL3_MAC_NULL,   CRYPTHASH_NULL,    4, SSL3_PRF_SHA384, PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_256_GCM_SHA384,         "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384" },         // ecdhe+rsa+gcm+sha384
    { 0xcca8, SSL3_TLS1_2, SSL3_KEY_ECDHE, SSL3_KEYLEN_256,  SSL3_SIGALG_RSA,   SSL3_ENC_CHACHA, SSL3_MAC_NULL,   CRYPTHASH_NULL,   12, SSL3_PRF_SHA256, PROTOSSL_CIPHER_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,   "TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256" },   // ecdhe+rsa+chacha+sha256
    { 0x009c, SSL3_TLS1_2, SSL3_KEY_RSA,   SSL3_KEYLEN_128,  SSL3_SIGALG_RSA,   SSL3_ENC_GCM,    SSL3_MAC_NULL,   CRYPTHASH_NULL,    4, SSL3_PRF_SHA256, PROTOSSL_CIPHER_RSA_WITH_AES_128_GCM_SHA256,               "TLS_RSA_WITH_AES_128_GCM_SHA256" },               // suite 156: rsa+gcm+sha256
    { 0x009d, SSL3_TLS1_2, SSL3_KEY_RSA,   SSL3_KEYLEN_256,  SSL3_SIGALG_RSA,   SSL3_ENC_GCM,    SSL3_MAC_NULL,   CRYPTHASH_NULL,    4, SSL3_PRF_SHA384, PROTOSSL_CIPHER_RSA_WITH_AES_256_GCM_SHA384,               "TLS_RSA_WITH_AES_256_GCM_SHA384" },               // suite 157: rsa+gcm+sha384
    // TLS1.2 cipher suites
    { 0xc023, SSL3_TLS1_2, SSL3_KEY_ECDHE, SSL3_KEYLEN_128,  SSL3_SIGALG_ECDSA, SSL3_ENC_AES,    SSL3_MAC_SHA256, CRYPTHASH_SHA256, 16, SSL3_PRF_SHA256, PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256,       "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256" },       // ecdhe+ecdsa+aes+sha256
    { 0xc024, SSL3_TLS1_2, SSL3_KEY_ECDHE, SSL3_KEYLEN_256,  SSL3_SIGALG_ECDSA, SSL3_ENC_AES,    SSL3_MAC_SHA384, CRYPTHASH_SHA384, 16, SSL3_PRF_SHA384, PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384,       "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384" },       // ecdhe+ecdsa+aes+sha384
    { 0xc027, SSL3_TLS1_2, SSL3_KEY_ECDHE, SSL3_KEYLEN_128,  SSL3_SIGALG_RSA,   SSL3_ENC_AES,    SSL3_MAC_SHA256, CRYPTHASH_SHA256, 16, SSL3_PRF_SHA256, PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_128_CBC_SHA256,         "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256" },         // ecdhe+rsa+aes+sha256
    { 0xc028, SSL3_TLS1_2, SSL3_KEY_ECDHE, SSL3_KEYLEN_256,  SSL3_SIGALG_RSA,   SSL3_ENC_AES,    SSL3_MAC_SHA384, CRYPTHASH_SHA384, 16, SSL3_PRF_SHA384, PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_256_CBC_SHA384,         "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384" },         // ecdhe+rsa+aes+sha384
    { 0x003c, SSL3_TLS1_2, SSL3_KEY_RSA,   SSL3_KEYLEN_128,  SSL3_SIGALG_RSA,   SSL3_ENC_AES,    SSL3_MAC_SHA256, CRYPTHASH_SHA256, 16, SSL3_PRF_SHA256, PROTOSSL_CIPHER_RSA_WITH_AES_128_CBC_SHA256,               "TLS_RSA_WITH_AES_128_CBC_SHA256" },               // suite 60: rsa+aes+sha256
    { 0x003d, SSL3_TLS1_2, SSL3_KEY_RSA,   SSL3_KEYLEN_256,  SSL3_SIGALG_RSA,   SSL3_ENC_AES,    SSL3_MAC_SHA256, CRYPTHASH_SHA256, 16, SSL3_PRF_SHA256, PROTOSSL_CIPHER_RSA_WITH_AES_256_CBC_SHA256,               "TLS_RSA_WITH_AES_256_CBC_SHA256" },               // suite 61: rsa+aes+sha256
    // TLS1.0 cipher suites
    { 0xc009, SSL3_TLS1_0, SSL3_KEY_ECDHE, SSL3_KEYLEN_128,  SSL3_SIGALG_ECDSA, SSL3_ENC_AES,    SSL3_MAC_SHA,    CRYPTHASH_SHA1,   16, SSL3_PRF_SHA256, PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_CBC_SHA,          "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA" },          // ecdhe+ecdsa+aes+sha
    { 0xc00a, SSL3_TLS1_0, SSL3_KEY_ECDHE, SSL3_KEYLEN_256,  SSL3_SIGALG_ECDSA, SSL3_ENC_AES,    SSL3_MAC_SHA,    CRYPTHASH_SHA1,   16, SSL3_PRF_SHA256, PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_256_CBC_SHA,          "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA" },          // ecdhe+ecdsa+aes+sha
    { 0xc013, SSL3_TLS1_0, SSL3_KEY_ECDHE, SSL3_KEYLEN_128,  SSL3_SIGALG_RSA,   SSL3_ENC_AES,    SSL3_MAC_SHA,    CRYPTHASH_SHA1,   16, SSL3_PRF_SHA256, PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_128_CBC_SHA,            "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA" },            // ecdhe+rsa+aes+sha
    { 0xc014, SSL3_TLS1_0, SSL3_KEY_ECDHE, SSL3_KEYLEN_256,  SSL3_SIGALG_RSA,   SSL3_ENC_AES,    SSL3_MAC_SHA,    CRYPTHASH_SHA1,   16, SSL3_PRF_SHA256, PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_256_CBC_SHA,            "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA" },            // ecdhe+rsa+aes+sha
    // SSLv3 cipher suites
    { 0x002f, SSL3_SSLv3,  SSL3_KEY_RSA,   SSL3_KEYLEN_128,  SSL3_SIGALG_RSA,   SSL3_ENC_AES,    SSL3_MAC_SHA,    CRYPTHASH_SHA1,   16, SSL3_PRF_SHA256, PROTOSSL_CIPHER_RSA_WITH_AES_128_CBC_SHA,                  "TLS_RSA_WITH_AES_128_CBC_SHA" },                  // suite 47: rsa+aes+sha
    { 0x0035, SSL3_SSLv3,  SSL3_KEY_RSA,   SSL3_KEYLEN_256,  SSL3_SIGALG_RSA,   SSL3_ENC_AES,    SSL3_MAC_SHA,    CRYPTHASH_SHA1,   16, SSL3_PRF_SHA256, PROTOSSL_CIPHER_RSA_WITH_AES_256_CBC_SHA,                  "TLS_RSA_WITH_AES_256_CBC_SHA" },                  // suite 53: rsa+aes+sha
};

//! supported elliptic curves; see http://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml#tls-parameters-8
static const EllipticCurveT _SSL3_EllipticCurves[SSL3_NUM_CURVES] =
{
    // curve 23: P-256
    { CRYPTCURVE_SECP256R1, PROTOSSL_CURVE_SECP256R1,   "SECP256R1" },
    // curve 29: X25519
    { CRYPTCURVE_X25519,    PROTOSSL_CURVE_X25519,      "X25519" },
    // curve 24: P-384
    { CRYPTCURVE_SECP384R1, PROTOSSL_CURVE_SECP384R1,   "SECP384R1" },
    // curve 30: X448
    { CRYPTCURVE_X448,      PROTOSSL_CURVE_X448,        "X448" }
};

//! signature schemes; see https://tools.ietf.org/html/rfc8446#section-4.2.3
static const SignatureSchemeT _SSL3_SignatureSchemes[] =
{
    { SSL3_SIGSCHEME_ECDSA_SHA256,        { SSL3_HASHID_SHA256, SSL3_SIGALG_ECDSA }, SSL3_SIGVERIFY_NONE,      ASN_OBJ_ECDSA_SECP256R1 },
    { SSL3_SIGSCHEME_RSA_PSS_PSS_SHA256,  { SSL3_HASHID_SHA256, SSL3_SIGALG_RSA   }, SSL3_SIGVERIFY_RSA_PSS,   ASN_OBJ_RSASSA_PSS      },
    { SSL3_SIGSCHEME_RSA_PSS_RSAE_SHA256, { SSL3_HASHID_SHA256, SSL3_SIGALG_RSA   }, SSL3_SIGVERIFY_RSA_PSS,   ASN_OBJ_RSA_PKCS_KEY    },
    { SSL3_SIGSCHEME_RSA_PKCS1_SHA256,    { SSL3_HASHID_SHA256, SSL3_SIGALG_RSA   }, SSL3_SIGVERIFY_RSA_PKCS1, ASN_OBJ_RSA_PKCS_KEY    },
    { SSL3_SIGSCHEME_ECDSA_SHA384,        { SSL3_HASHID_SHA384, SSL3_SIGALG_ECDSA }, SSL3_SIGVERIFY_NONE,      ASN_OBJ_ECDSA_SECP384R1 },
    { SSL3_SIGSCHEME_RSA_PSS_PSS_SHA384,  { SSL3_HASHID_SHA384, SSL3_SIGALG_RSA   }, SSL3_SIGVERIFY_RSA_PSS,   ASN_OBJ_RSASSA_PSS      },
    { SSL3_SIGSCHEME_RSA_PSS_RSAE_SHA384, { SSL3_HASHID_SHA384, SSL3_SIGALG_RSA   }, SSL3_SIGVERIFY_RSA_PSS,   ASN_OBJ_RSA_PKCS_KEY    },
    { SSL3_SIGSCHEME_RSA_PKCS1_SHA384,    { SSL3_HASHID_SHA384, SSL3_SIGALG_RSA   }, SSL3_SIGVERIFY_RSA_PKCS1, ASN_OBJ_RSA_PKCS_KEY    },
    { SSL3_SIGSCHEME_RSA_PSS_PSS_SHA512,  { SSL3_HASHID_SHA512, SSL3_SIGALG_RSA   }, SSL3_SIGVERIFY_RSA_PSS,   ASN_OBJ_RSASSA_PSS      },
    { SSL3_SIGSCHEME_RSA_PSS_RSAE_SHA512, { SSL3_HASHID_SHA512, SSL3_SIGALG_RSA   }, SSL3_SIGVERIFY_RSA_PSS,   ASN_OBJ_RSA_PKCS_KEY    },
    { SSL3_SIGSCHEME_RSA_PKCS1_SHA512,    { SSL3_HASHID_SHA512, SSL3_SIGALG_RSA   }, SSL3_SIGVERIFY_RSA_PKCS1, ASN_OBJ_RSA_PKCS_KEY    },
    { SSL3_SIGSCHEME_ECDSA_SHA1,          { SSL3_HASHID_SHA1,   SSL3_SIGALG_ECDSA }, SSL3_SIGVERIFY_NONE,      ASN_OBJ_ECDSA_KEY       },
    { SSL3_SIGSCHEME_RSA_PKCS1_SHA1,      { SSL3_HASHID_SHA1,   SSL3_SIGALG_RSA   }, SSL3_SIGVERIFY_RSA_PKCS1, ASN_OBJ_RSA_PKCS_KEY    }
};

//! handshake transition validation map for TLS1.2 and prior clients; see https://tools.ietf.org/html/rfc5246#section-7.3
static const SSLHandshakeMapT _SSL3_ClientRecvMsgMap[] =
{
    { SSL3_MSG_CLIENT_HELLO, SSL3_MSG_SERVER_HELLO },
    { SSL3_MSG_SERVER_HELLO, SSL3_MSG_CERTIFICATE },
    { SSL3_MSG_SERVER_HELLO, SSL3_MSG_FINISHED }, 
    { SSL3_MSG_CERTIFICATE,  SSL3_MSG_SERVER_KEY },
    { SSL3_MSG_CERTIFICATE,  SSL3_MSG_CERT_REQ },
    { SSL3_MSG_CERTIFICATE,  SSL3_MSG_SERVER_DONE },
    { SSL3_MSG_SERVER_KEY,   SSL3_MSG_CERT_REQ },
    { SSL3_MSG_SERVER_KEY,   SSL3_MSG_SERVER_DONE },
    { SSL3_MSG_CERT_REQ,     SSL3_MSG_SERVER_DONE },
    { SSL3_MSG_SERVER_DONE,  SSL3_MSG_FINISHED },
    { -1,                    -1 }
};
//! handshake transition validation map for TLS1.2 and prior servers
static const SSLHandshakeMapT _SSL3_ServerRecvMsgMap[] =
{
    { 0,                      SSL3_MSG_CLIENT_HELLO },
    { SSL3_MSG_CLIENT_HELLO,  SSL3_MSG_CERTIFICATE },
    { SSL3_MSG_CLIENT_HELLO,  SSL3_MSG_CLIENT_KEY },
    { SSL3_MSG_CLIENT_HELLO,  SSL3_MSG_FINISHED },
    { SSL3_MSG_CERTIFICATE,   SSL3_MSG_CLIENT_KEY },
    { SSL3_MSG_CLIENT_KEY,    SSL3_MSG_CERT_VERIFY },
    { SSL3_MSG_CLIENT_KEY,    SSL3_MSG_FINISHED },
    { SSL3_MSG_CERT_VERIFY,   SSL3_MSG_FINISHED },
    { -1,                     -1 }
};

//! handshake transition validation map for TLS1.3 clients; see https://tools.ietf.org/html/rfc8446#section-2
static const SSLHandshakeMapT _SSL3_ClientRecvMsgMap_13[] =
{
    { SSL3_MSG_CLIENT_HELLO,         SSL3_MSG_SERVER_HELLO },
    { SSL3_MSG_SERVER_HELLO,         SSL3_MSG_ENCRYPTED_EXTENSIONS },
    { SSL3_MSG_SERVER_HELLO,         SSL3_MSG_FINISHED }, 
    { SSL3_MSG_ENCRYPTED_EXTENSIONS, SSL3_MSG_CERT_REQ }, 
    { SSL3_MSG_ENCRYPTED_EXTENSIONS, SSL3_MSG_CERTIFICATE }, 
    { SSL3_MSG_ENCRYPTED_EXTENSIONS, SSL3_MSG_FINISHED }, 
    { SSL3_MSG_CERT_REQ,             SSL3_MSG_CERTIFICATE }, 
    { SSL3_MSG_CERTIFICATE,          SSL3_MSG_CERT_VERIFY }, 
    { SSL3_MSG_CERT_VERIFY,          SSL3_MSG_FINISHED },
    { SSL3_MSG_FINISHED,             SSL3_MSG_NEW_SESSION_TICKET },
    { SSL3_MSG_NEW_SESSION_TICKET,   SSL3_MSG_NEW_SESSION_TICKET },
    { -1,                            -1 }
};
//! handshake transition validation map for TLS1.3 servers
static const SSLHandshakeMapT _SSL3_ServerRecvMsgMap_13[] =
{
    { 0,                             SSL3_MSG_CLIENT_HELLO },
    { SSL3_MSG_CLIENT_HELLO,         SSL3_MSG_CLIENT_HELLO },
    { SSL3_MSG_CLIENT_HELLO,         SSL3_MSG_CERTIFICATE },
    { SSL3_MSG_CLIENT_HELLO,         SSL3_MSG_FINISHED },
    { SSL3_MSG_CERTIFICATE,          SSL3_MSG_CERT_VERIFY }, 
    { SSL3_MSG_CERTIFICATE,          SSL3_MSG_FINISHED }, 
    { SSL3_MSG_CERT_VERIFY,          SSL3_MSG_FINISHED },
    { -1,                            -1 }
};

//! .pem certificate signature identifiers; see https://tools.ietf.org/html/rfc7468
static const CertificateSignatureT _SSL3_CertSignatures[] =
{
    { "-----BEGIN CERTIFICATE-----",      "-----END CERTIFICATE-----",      ASN_OBJ_RSA_PKCS_KEY },     // pkcs#1 certificate
    { "-----BEGIN X509 CERTIFICATE-----", "-----END X509 CERTIFICATE-----", ASN_OBJ_RSA_PKCS_KEY },     // pkcs#1 certificate (alternate/obsolete form)
    { "-----BEGIN RSA PRIVATE KEY-----",  "-----END RSA PRIVATE KEY-----",  ASN_OBJ_RSA_PKCS_KEY },     // pkcs#1 private key
    { "-----BEGIN PRIVATE KEY-----",      "-----END PRIVATE KEY-----",      ASN_OBJ_RSASSA_PSS   },     // pkcs#8 private key (rsa-pss)
    { "-----BEGIN EC PRIVATE KEY-----",   "-----END EC PRIVATE KEY-----",   ASN_OBJ_ECDSA_KEY    }      // pkcs#8 private key (ecdsa)
};

//! translation table for SSL3_HASHID_* to CryptHashE type
static const CryptHashTypeE _SSL3_HashIdToCrypt[SSL3_HASHID_MAX+1] =
{
    CRYPTHASH_NULL,     // SSL3_HASHID_NONE
    CRYPTHASH_MD5,      // SSL3_HASHID_MD5
    CRYPTHASH_SHA1,     // SSL3_HASHID_SHA1
    CRYPTHASH_SHA224,   // SSL3_HASHID_SHA224
    CRYPTHASH_SHA256,   // SSL3_HASHID_SHA256
    CRYPTHASH_SHA384,   // SSL3_HASHID_SHA384
    CRYPTHASH_SHA512    // SSL3_hashid_sha512
};

//! translation table for SSL3_HASHID_* to ASN hash object identifier; ASN_OBJ_NONE means unsupported
static const uint32_t _SSL3_CrypttoASN[CRYPTHASH_NUMHASHES] =
{
    ASN_OBJ_NONE,       // CRYPTHASH_NULL
    ASN_OBJ_NONE,       // CRYPTHASH_MURMUR3
    ASN_OBJ_MD5,        // CRYPTHASH_MD5
    ASN_OBJ_SHA1,       // CRYPTHASH_SHA1
    ASN_OBJ_NONE,       // CRYPTHASH_SHA224
    ASN_OBJ_SHA256,     // CRYPTHASH_SHA256
    ASN_OBJ_SHA384,     // CRYPTHASH_SHA384
    ASN_OBJ_SHA512      // CRYPTHASH_SHA512
};

//! ssl version name table
static const char *_SSL3_strVersionNames[] =
{
    "SSLv3",            // defined here to retain easy version number to name translation, do not remove
    "TLSv1",
    "TLSv1.1",
    "TLSv1.2",
    "TLSv1.3"
};

// signature types - subtract type from ASN_OBJ_RSA_PKCS_MD5 to get offset for this table
static const char *_SSL3_strSignatureTypes[] =
{
    "md5WithRSAEncryption",
    "sha1WithRSAEncryption",
    "sha256WithRSAEncryption",
    "sha384WithRSAEncryption",
    "sha512WithRSAEncryption",
    "ecdsa-with-SHA256",
    "ecdsa-with-SHA384",
    "ecdsa-with-SHA512"
};

#if DIRTYCODE_LOGGING
#if DEBUG_ALL_OBJS
static const char *_SSL3_strAsnTypes[] =
{
    "ASN_00",
    "ASN_TYPE_BOOLEAN",         // 0x01
    "ASN_TYPE_INTEGER",         // 0x02
    "ASN_TYPE_BITSTRING",       // 0x03
    "ASN_TYPE_OCTSTRING",       // 0x04
    "ASN_TYPE_NULL",            // 0x05
    "ASN_TYPE_OBJECT",          // 0x06
    "ASN_07", "ASN_08", "ASN_09",
    "ASN_0A", "ASN_0B",
    "ASN_TYPE_UTF8STR",         // 0x0c
    "ASN_0D", "ASN_0E", "ASN_0F",
    "ASN_TYPE_SEQN",            // 0x10
    "ASN_TYPE_SET",             // 0x11
    "ASN_12",                   // 0x12
    "ASN_TYPE_PRINTSTR",        // 0x13
    "ASN_TYPE_T61",             // 0x14 (Teletex string)
    "ASN_15",
    "ASN_TYPE_IA5",             // 0x16 (IA5 string)
    "ASN_TYPE_UTCTIME",         // 0x17
    "ASN_TYPE_GENERALIZEDTIME", // 0x18
    "ASN_19", "ASN_1A", "ASN_1B",
    "ASN_1C", "ASN_1D",
    "ASN_TYPE_UNICODESTR",      // 0x1e
    "ASN_1F",
};
static const char *_SSL3_strAsnObjs[ASN_OBJ_COUNT] =
{
    "ASN_OBJ_NONE",
    "ASN_OBJ_COUNTRY",
    "ASN_OBJ_STATE",
    "ASN_OBJ_CITY",
    "ASN_OBJ_ORGANIZATION",
    "ASN_OBJ_UNIT",
    "ASN_OBJ_COMMON",
    "ASN_OBJ_SUBJECT_ALT",
    "ASN_OBJ_BASIC_CONSTRAINTS",
    "ASN_OBJ_MD5",
    "ASN_OBJ_SHA1",
    "ASN_OBJ_SHA256",
    "ASN_OBJ_SHA384",
    "ASN_OBJ_SHA512",
    "ASN_OBJ_RSA_PKCS_KEY",
    "ASN_OBJ_ECDSA_KEY",
    "ASN_OBJ_RSA_PKCS_MD5",
    "ASN_OBJ_RSA_PKCS_SHA1",
    "ASN_OBJ_RSA_PKCS_SHA256",
    "ASN_OBJ_RSA_PKCS_SHA384",
    "ASN_OBJ_RSA_PKCS_SHA512",
    "ASN_OBJ_RSASSA_PSS",
    "ASN_OBJ_ECDSA_SHA256",
    "ASN_OBJ_ECDSA_SHA384",
    "ASN_OBJ_ECDSA_SHA512",
    "ASN_OBJ_ECDSA_SECP256R1",
    "ASN_OBJ_ECDSA_SECP384R1",
    "ASN_OBJ_ECDSA_SECP512R1",
    "ASN_OBJ_PKCS1_MGF1"
};
#endif // DEBUG_ALL_OBJS

// tls extension names
static const char *_SSL3_strExtensionNames[] =
{
    "server_name",
    "max_fragment_length",
    "client_certificate_url",
    "trusted_ca_keys",
    "truncated_hmac",
    "status_request",
    "user_mapping",
    "client_authz",
    "server_authz",
    "cert_type",
    "supported_groups", // (renamed from "elliptic_curves")
    "ec_point_formats",
    "srp",
    "signature_algorithms",
    "use_srtp",
    "heartbeat",
    "application_layer_protocol_negotiation",
    "status_request_v2",
    "signed_certificate_timestamp",
    "client_certificate_type",
    "server_certificate_type",
    "padding",
    "encrypt_then_mac",
    "extended_master_secret",
    "token_binding",
    "cached_info",
    "unassigned", "unassigned", "unassigned", "unassigned", "unassigned", "unassigned", "unassigned", "unassigned", "unassigned",
    "SessionTicket TLS",
    "unassigned", "unassigned", "unassigned", "unassigned", "unassigned",
    "pre_shared_key",
    "early_data",
    "supported_versions",
    "cookie",
    "psk_key_exchange_modes",
    "unassigned",
    "certificate_authorities",
    "oid_filter",
    "post_handshake_auth",
    "signature_algorithms_cert",
    "key_share",
};

#endif // DIRTYCODE_LOGGING

//! alert description table; see https://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml
static const ProtoSSLAlertDescT _ProtoSSL_AlertList[] =
{
    { SSL3_ALERT_DESC_CLOSE_NOTIFY,               "close notify"              },    //   0
    { SSL3_ALERT_DESC_UNEXPECTED_MESSAGE,         "unexpected message"        },    //  10
    { SSL3_ALERT_DESC_BAD_RECORD_MAC,             "bad record mac"            },    //  20
    { SSL3_ALERT_DESC_DECRYPTION_FAILED,          "decryption failed"         },    //  21 - reserved
    { SSL3_ALERT_DESC_RECORD_OVERFLOW,            "record overflow"           },    //  22
    { SSL3_ALERT_DESC_DECOMPRESSION_FAILURE,      "decompression failure"     },    //  30 - reserved
    { SSL3_ALERT_DESC_HANDSHAKE_FAILURE,          "handshake failure"         },    //  40
    { SSL3_ALERT_DESC_NO_CERTIFICATE,             "no certificate"            },    //  41 - reserved
    { SSL3_ALERT_DESC_BAD_CERTFICIATE,            "bad certificate"           },    //  42
    { SSL3_ALERT_DESC_UNSUPPORTED_CERTIFICATE,    "unsupported certificate"   },    //  43
    { SSL3_ALERT_DESC_CERTIFICATE_REVOKED,        "certificate revoked"       },    //  44
    { SSL3_ALERT_DESC_CERTIFICATE_EXPIRED,        "certificate expired"       },    //  45
    { SSL3_ALERT_DESC_CERTIFICATE_UNKNOWN,        "certificate unknown"       },    //  46
    { SSL3_ALERT_DESC_ILLEGAL_PARAMETER,          "illegal parameter"         },    //  47
    // the following alert types are all TLS only
    { SSL3_ALERT_DESC_UNKNOWN_CA,                 "unknown ca"                },    //  48
    { SSL3_ALERT_DESC_ACCESS_DENIED,              "access denied"             },    //  49
    { SSL3_ALERT_DESC_DECODE_ERROR,               "decode error"              },    //  50
    { SSL3_ALERT_DESC_DECRYPT_ERROR,              "decrypt error"             },    //  51
    { SSL3_ALERT_DESC_EXPORT_RESTRICTION,         "export restriction"        },    //  60 - reserved
    { SSL3_ALERT_DESC_PROTOCOL_VERSION,           "protocol version"          },    //  70
    { SSL3_ALERT_DESC_INSUFFICIENT_SECURITY,      "insufficient security"     },    //  71
    { SSL3_ALERT_DESC_INTERNAL_ERROR,             "internal error"            },    //  80
    { SSL3_ALERT_DESC_INAPPROPRIATE_FALLBACK,     "inappropriate fallback"    },    //  86
    { SSL3_ALERT_DESC_USER_CANCELLED,             "user cancelled"            },    //  90
    { SSL3_ALERT_DESC_NO_RENEGOTIATION,           "no renegotiation"          },    // 100 - reserved
    { SSL3_ALERT_DESC_MISSING_EXTENSION,          "missing extension"         },    // 109
    { SSL3_ALERT_DESC_UNSUPPORTED_EXTENSION,      "unsupported extension"     },    // 110
    // alert extensions; see http://tools.ietf.org/html/rfc6066#section-9
    { SSL3_ALERT_DESC_CERTIFICATE_UNOBTAINABLE,   "certificate unobtainable"  },    // 111
    { SSL3_ALERT_DESC_UNRECOGNIZED_NAME,          "unrecognized name"         },    // 112
    { SSL3_ALERT_DESC_BAD_CERTIFICATE_STATUS,     "bad certificate status"    },    // 113
    { SSL3_ALERT_DESC_BAD_CERTIFICATE_HASH,       "bad certificate hash"      },    // 114
    // alert extension; see http://tools.ietf.org/html/rfc4279#section-6
    { SSL3_ALERT_DESC_UNKNOWN_PSK_IDENTITY,       "unknown psk identify"      },    // 115
    // alert extension; see https://tools.ietf.org/html/rfc8446#section-4.4.2.4
    { SSL3_ALERT_DESC_CERTIFICATE_REQUIRED,       "certificate required"      },    // 116
    // alert extension; see http://tools.ietf.org/html/rfc7301#section-3.2
    { SSL3_ALERT_DESC_NO_APPLICATION_PROTOCOL,    "no application protocol"   },    // 120
    { -1, NULL                                                                },    // list terminator
};

// ASN object identification table
static const struct ASNObjectT _SSL_ObjectList[] =
{
    { ASN_OBJ_COUNTRY, 3, { 0x55, 0x04, 0x06 } },
    { ASN_OBJ_CITY, 3, { 0x55, 0x04, 0x07 } },
    { ASN_OBJ_STATE, 3, { 0x55, 0x04, 0x08 } },
    { ASN_OBJ_ORGANIZATION, 3, { 0x55, 0x04, 0x0a } },
    { ASN_OBJ_UNIT, 3, { 0x55, 0x04, 0x0b } },
    { ASN_OBJ_COMMON, 3, { 0x55, 0x04, 0x03 } },
    { ASN_OBJ_SUBJECT_ALT, 3, { 0x55, 0x1d, 0x11 } },
    { ASN_OBJ_BASIC_CONSTRAINTS, 3, { 0x55, 0x1d, 0x13 } },

    // OBJ_md5 - OID 1.2.840.113549.2.5
    { ASN_OBJ_MD5, 8, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x02, 0x05 } },
    // OBJ_sha1 - OID 1.3.14.3.2.26
    { ASN_OBJ_SHA1, 5, { 0x2b, 0x0e, 0x03, 0x02, 0x1a } },
    // OBJ_sha256 - OID 2.16.840.1.101.3.4.2.1
    { ASN_OBJ_SHA256, 9, { 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01 } },
    // OBJ_sha384 - OID 2.16.840.1.101.3.4.2.2
    { ASN_OBJ_SHA384, 9, { 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02 } },
    // OBJ_sha512 - OID 2.16.840.1.101.3.4.2.3
    { ASN_OBJ_SHA512, 9, { 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03 } },

    // RSA (PKCS #1 v1.5) key transport algorithm, OID 1.2.840.113349.1.1.1
    { ASN_OBJ_RSA_PKCS_KEY, 9, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01 } },
    // RSA (PKCS #1 v1.5) with MD5 signature, OID 1.2.840.113549.1.1.4
    { ASN_OBJ_RSA_PKCS_MD5, 9, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x04 } },
    // RSA (PKCS #1 v1.5) with SHA-1 signature; sha1withRSAEncryption OID 1.2.840.113549.1.1.5
    { ASN_OBJ_RSA_PKCS_SHA1, 9, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05 } },
    /* the following are obsolete alternate definitions of sha1withRSAEncryption; we define them
       here for compatibility, because some certificates are still generated with these ids
       (by makecert.exe, included with WindowsSDK, for example) */
    // RSA (PKCS #1 v1.5) with SHA-1 signature; sha-1WithRSAEncryption (obsolete) OID 1.3.14.3.2.29
    { ASN_OBJ_RSA_PKCS_SHA1, 5, { 0x2b, 0x0e, 0x03, 0x02, 0x1d } },
    // RSA (PKCS #1 v1.5) with SHA-1 signature; rsaSignatureWithsha1 (obsolete) OID 1.3.36.3.3.1.2
    { ASN_OBJ_RSA_PKCS_SHA1, 5, { 0x2b, 0x24, 0x03, 0x03, 0x01, 0x02 } },

    // PKCS-MGF1 1.2.840.113549.1.1.8
    { ASN_OBJ_PKCS1_MGF1, 9, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x08 } },
    // RSASSA-PSS PKCS#1 1.2.840.113549.1.1.10
    { ASN_OBJ_RSASSA_PSS, 9, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0a } },

    /* sha2+rsa combinations */
    // RSA (PKCS #1 v1.5) with SHA-256 signature; sha256withRSAEncryption OID 1.2.840.113549.1.1.11
    { ASN_OBJ_RSA_PKCS_SHA256, 9, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b } },
    // RSA (PKCS #1 v1.5) with SHA-384 signature; sha384withRSAEncryption OID 1.2.840.113549.1.1.12
    { ASN_OBJ_RSA_PKCS_SHA384, 9, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0c } },
    // RSA (PKCS #1 v1.5) with SHA-512 signature; sha512withRSAEncryption OID 1.2.840.113549.1.1.13
    { ASN_OBJ_RSA_PKCS_SHA512, 9, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0d } },

    // ecdsa key PKCS#8 1.2.840.10045.2.1
    { ASN_OBJ_ECDSA_KEY, 7, { 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01 } },
    // prime256v1/secp256r1 1.2.840.10045.3.1.7
    { ASN_OBJ_ECDSA_SECP256R1, 8, {0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07 } },
    // ecdsa-with-SHA256 1.2.840.10045.4.3.2
    { ASN_OBJ_ECDSA_SHA256, 8, {0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02 } },
    // ecdsa-with-SHA384 1.2.840.10045.4.3.3
    { ASN_OBJ_ECDSA_SHA384, 8, {0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x03 } },
    // ecdsa-with-SHA512 1.2.840.10045.4.3.4
    { ASN_OBJ_ECDSA_SHA512, 8, {0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x04 } },

    // prime384v1/secp384r1 1.3.132.0.34
    { ASN_OBJ_ECDSA_SECP384R1, 5, {0x2b, 0x81, 0x04, 0x00, 0x22 } },
    // prime521v1/secp521r1 1.3.132.0.35
    { ASN_OBJ_ECDSA_SECP521R1, 5, {0x2b, 0x81, 0x04, 0x00, 0x23 } },

    // array terminator
    { ASN_OBJ_NONE, 0, { 0 } }
};

// The 2048-bit public key modulus for 2015 GOS CA Cert signed with sha256 and an exponent of 65537
static const uint8_t _ProtoSSL_GOS2015ServerModulus2048[] =
{
    0xcc, 0x6d, 0x54, 0xb6, 0xf4, 0xe4, 0x84, 0xe7, 0x20, 0x76, 0x02, 0xd7, 0x97, 0x48, 0x75, 0x7e,
    0x7e, 0xfb, 0x43, 0x9c, 0x3d, 0xa1, 0x96, 0x47, 0xc1, 0x5d, 0x07, 0x8b, 0x30, 0x73, 0xbf, 0x9d,
    0xfe, 0x75, 0x94, 0x55, 0x21, 0xd0, 0x88, 0x74, 0x66, 0x4c, 0xa2, 0xb7, 0xfe, 0x9f, 0xc0, 0x3b,
    0xf0, 0x60, 0xa0, 0xdb, 0x08, 0x33, 0x2b, 0x6e, 0xf8, 0x02, 0x05, 0xb9, 0x87, 0x9d, 0xac, 0x65,
    0xd5, 0x06, 0x9d, 0x05, 0xe8, 0xd1, 0xb6, 0xf5, 0xde, 0x7d, 0xa5, 0xa4, 0x7d, 0x8a, 0xcb, 0x99,
    0x31, 0xb6, 0x85, 0x9b, 0xa2, 0xce, 0x39, 0xe2, 0x8c, 0x65, 0xaa, 0x07, 0xfc, 0x15, 0x33, 0x07,
    0x00, 0xd1, 0x72, 0x15, 0x13, 0x0d, 0x87, 0x0f, 0x5c, 0xa2, 0x5e, 0xd0, 0xd5, 0xbf, 0xd9, 0x03,
    0x32, 0x62, 0xaf, 0xf5, 0xef, 0x53, 0x36, 0xa8, 0x34, 0xda, 0xb6, 0xa3, 0xec, 0x5c, 0x6a, 0xc0,
    0x67, 0xf8, 0xbe, 0x37, 0x9f, 0xb3, 0xc8, 0x2d, 0xf0, 0x36, 0x4a, 0x6f, 0x6b, 0x06, 0xee, 0xb7,
    0x85, 0xf2, 0x7f, 0x73, 0x6c, 0x01, 0x84, 0x83, 0xe4, 0xda, 0x46, 0xd0, 0x23, 0x9a, 0x6d, 0xf1,
    0x77, 0x7c, 0x05, 0x81, 0x90, 0x4f, 0x6a, 0x44, 0x83, 0x78, 0x3b, 0x71, 0xad, 0x12, 0xc0, 0x48,
    0xc8, 0x73, 0x89, 0xf1, 0x98, 0x78, 0x7b, 0xb4, 0x08, 0x4a, 0xba, 0xe8, 0x59, 0x57, 0xe2, 0xfc,
    0x29, 0xac, 0xbf, 0xf5, 0xa2, 0x9d, 0x4f, 0x2c, 0x64, 0xdc, 0xd7, 0x92, 0x19, 0x1c, 0xc5, 0xfa,
    0xdb, 0x92, 0xc0, 0x90, 0x4b, 0xa8, 0xe9, 0xf2, 0x0d, 0x94, 0x1a, 0xb2, 0x5f, 0xdd, 0x33, 0xae,
    0xff, 0x66, 0x90, 0x97, 0xb2, 0xa8, 0xa5, 0x1b, 0xfa, 0x6f, 0x41, 0xb2, 0x84, 0xba, 0x52, 0x34,
    0x97, 0x4a, 0xd3, 0xc7, 0xb2, 0x3f, 0xdd, 0xdb, 0xc9, 0xb1, 0x13, 0x82, 0x77, 0xe8, 0x6a, 0xcd
};

// the 256 bit public key curve data for 2019 GS CA cert signed with sha384
static const uint8_t _ProtoSSL_GS2019ServerKey[] =
{
    0x04, 0x13, 0x3f, 0x21, 0x93, 0x86, 0xf7, 0x65, 0xc4, 0x7f, 0x8c, 0x1c, 0xef, 0x49, 0xa8, 0x2a,
    0x32, 0xe3, 0x6c, 0xd4, 0xd0, 0x12, 0x9a, 0x1e, 0x18, 0x10, 0xce, 0xd3, 0xb0, 0xd8, 0x1e, 0xff,
    0xcc, 0x5b, 0x73, 0x5b, 0xc7, 0x5b, 0xeb, 0x0b, 0xf4, 0x06, 0x04, 0x0e, 0x1c, 0x27, 0xd6, 0x87,
    0x31, 0xde, 0x68, 0x7d, 0xdb, 0xfa, 0x03, 0x32, 0x89, 0x2a, 0x30, 0x5d, 0xb3, 0xf8, 0xc2, 0xeb,
    0xa4  
};

// The 2048-bit modulus for the VeriSign 2006 CA Cert signed with sha1 and an exponent of 65537
static const uint8_t _ProtoSSL_VeriSign2006ServerModulus[] =
{
    0xaf, 0x24, 0x08, 0x08, 0x29, 0x7a, 0x35, 0x9e, 0x60, 0x0c, 0xaa, 0xe7, 0x4b, 0x3b, 0x4e, 0xdc,
    0x7c, 0xbc, 0x3c, 0x45, 0x1c, 0xbb, 0x2b, 0xe0, 0xfe, 0x29, 0x02, 0xf9, 0x57, 0x08, 0xa3, 0x64,
    0x85, 0x15, 0x27, 0xf5, 0xf1, 0xad, 0xc8, 0x31, 0x89, 0x5d, 0x22, 0xe8, 0x2a, 0xaa, 0xa6, 0x42,
    0xb3, 0x8f, 0xf8, 0xb9, 0x55, 0xb7, 0xb1, 0xb7, 0x4b, 0xb3, 0xfe, 0x8f, 0x7e, 0x07, 0x57, 0xec,
    0xef, 0x43, 0xdb, 0x66, 0x62, 0x15, 0x61, 0xcf, 0x60, 0x0d, 0xa4, 0xd8, 0xde, 0xf8, 0xe0, 0xc3,
    0x62, 0x08, 0x3d, 0x54, 0x13, 0xeb, 0x49, 0xca, 0x59, 0x54, 0x85, 0x26, 0xe5, 0x2b, 0x8f, 0x1b,
    0x9f, 0xeb, 0xf5, 0xa1, 0x91, 0xc2, 0x33, 0x49, 0xd8, 0x43, 0x63, 0x6a, 0x52, 0x4b, 0xd2, 0x8f,
    0xe8, 0x70, 0x51, 0x4d, 0xd1, 0x89, 0x69, 0x7b, 0xc7, 0x70, 0xf6, 0xb3, 0xdc, 0x12, 0x74, 0xdb,
    0x7b, 0x5d, 0x4b, 0x56, 0xd3, 0x96, 0xbf, 0x15, 0x77, 0xa1, 0xb0, 0xf4, 0xa2, 0x25, 0xf2, 0xaf,
    0x1c, 0x92, 0x67, 0x18, 0xe5, 0xf4, 0x06, 0x04, 0xef, 0x90, 0xb9, 0xe4, 0x00, 0xe4, 0xdd, 0x3a,
    0xb5, 0x19, 0xff, 0x02, 0xba, 0xf4, 0x3c, 0xee, 0xe0, 0x8b, 0xeb, 0x37, 0x8b, 0xec, 0xf4, 0xd7,
    0xac, 0xf2, 0xf6, 0xf0, 0x3d, 0xaf, 0xdd, 0x75, 0x91, 0x33, 0x19, 0x1d, 0x1c, 0x40, 0xcb, 0x74,
    0x24, 0x19, 0x21, 0x93, 0xd9, 0x14, 0xfe, 0xac, 0x2a, 0x52, 0xc7, 0x8f, 0xd5, 0x04, 0x49, 0xe4,
    0x8d, 0x63, 0x47, 0x88, 0x3c, 0x69, 0x83, 0xcb, 0xfe, 0x47, 0xbd, 0x2b, 0x7e, 0x4f, 0xc5, 0x95,
    0xae, 0x0e, 0x9d, 0xd4, 0xd1, 0x43, 0xc0, 0x67, 0x73, 0xe3, 0x14, 0x08, 0x7e, 0xe5, 0x3f, 0x9f,
    0x73, 0xb8, 0x33, 0x0a, 0xcf, 0x5d, 0x3f, 0x34, 0x87, 0x96, 0x8a, 0xee, 0x53, 0xe8, 0x25, 0x15
};

// The 2048-bit public key modulus for DigiCert Global Root CA
static const uint8_t _ProtoSSL_DigiCertServerModulus[] =
{
    0xE2, 0x3B, 0xE1, 0x11, 0x72, 0xDE, 0xA8, 0xA4, 0xD3, 0xA3, 0x57, 0xAA, 0x50, 0xA2, 0x8F, 0x0B,
    0x77, 0x90, 0xC9, 0xA2, 0xA5, 0xEE, 0x12, 0xCE, 0x96, 0x5B, 0x01, 0x09, 0x20, 0xCC, 0x01, 0x93,
    0xA7, 0x4E, 0x30, 0xB7, 0x53, 0xF7, 0x43, 0xC4, 0x69, 0x00, 0x57, 0x9D, 0xE2, 0x8D, 0x22, 0xDD,
    0x87, 0x06, 0x40, 0x00, 0x81, 0x09, 0xCE, 0xCE, 0x1B, 0x83, 0xBF, 0xDF, 0xCD, 0x3B, 0x71, 0x46,
    0xE2, 0xD6, 0x66, 0xC7, 0x05, 0xB3, 0x76, 0x27, 0x16, 0x8F, 0x7B, 0x9E, 0x1E, 0x95, 0x7D, 0xEE,
    0xB7, 0x48, 0xA3, 0x08, 0xDA, 0xD6, 0xAF, 0x7A, 0x0C, 0x39, 0x06, 0x65, 0x7F, 0x4A, 0x5D, 0x1F,
    0xBC, 0x17, 0xF8, 0xAB, 0xBE, 0xEE, 0x28, 0xD7, 0x74, 0x7F, 0x7A, 0x78, 0x99, 0x59, 0x85, 0x68,
    0x6E, 0x5C, 0x23, 0x32, 0x4B, 0xBF, 0x4E, 0xC0, 0xE8, 0x5A, 0x6D, 0xE3, 0x70, 0xBF, 0x77, 0x10,
    0xBF, 0xFC, 0x01, 0xF6, 0x85, 0xD9, 0xA8, 0x44, 0x10, 0x58, 0x32, 0xA9, 0x75, 0x18, 0xD5, 0xD1,
    0xA2, 0xBE, 0x47, 0xE2, 0x27, 0x6A, 0xF4, 0x9A, 0x33, 0xF8, 0x49, 0x08, 0x60, 0x8B, 0xD4, 0x5F,
    0xB4, 0x3A, 0x84, 0xBF, 0xA1, 0xAA, 0x4A, 0x4C, 0x7D, 0x3E, 0xCF, 0x4F, 0x5F, 0x6C, 0x76, 0x5E,
    0xA0, 0x4B, 0x37, 0x91, 0x9E, 0xDC, 0x22, 0xE6, 0x6D, 0xCE, 0x14, 0x1A, 0x8E, 0x6A, 0xCB, 0xFE,
    0xCD, 0xB3, 0x14, 0x64, 0x17, 0xC7, 0x5B, 0x29, 0x9E, 0x32, 0xBF, 0xF2, 0xEE, 0xFA, 0xD3, 0x0B,
    0x42, 0xD4, 0xAB, 0xB7, 0x41, 0x32, 0xDA, 0x0C, 0xD4, 0xEF, 0xF8, 0x81, 0xD5, 0xBB, 0x8D, 0x58,
    0x3F, 0xB5, 0x1B, 0xE8, 0x49, 0x28, 0xA2, 0x70, 0xDA, 0x31, 0x04, 0xDD, 0xF7, 0xB2, 0x16, 0xF2,
    0x4C, 0x0A, 0x4E, 0x07, 0xA8, 0xED, 0x4A, 0x3D, 0x5E, 0xB5, 0x7F, 0xA3, 0x90, 0xC3, 0xAF, 0x27
};

// only certificates from these authorities are supported
static ProtoSSLCACertT _ProtoSSL_CACerts[] =
{
    // gos2015 CA
    { { "US", "California", "Redwood City", "Electronic Arts, Inc.",
        "Global Online Studio/emailAddress=GOSDirtysockSupport@ea.com",
        "GOS 2015 Certificate Authority" },
      SSL_CACERTFLAG_GOSCA|SSL_CACERTFLAG_CAPROVIDER, ASN_OBJ_RSA_PKCS_SHA256, 0,
      sizeof(_ProtoSSL_GOS2015ServerModulus2048), _ProtoSSL_GOS2015ServerModulus2048,
      3, { 0x01, 0x00, 0x01 },
      0, NULL, NULL, &_ProtoSSL_CACerts[1] },
    // gos2019 CA
    { { "US", "California", "Redwood City", "Electronic Arts, Inc.",
        "EADP Gameplay Services",
        "Gameplay Services 2019 Certificate Authority" },
      SSL_CACERTFLAG_GOSCA, ASN_OBJ_ECDSA_KEY, ASN_OBJ_ECDSA_SECP256R1,
      sizeof(_ProtoSSL_GS2019ServerKey), _ProtoSSL_GS2019ServerKey,
      0, { 0x00 },
      0, NULL, NULL, &_ProtoSSL_CACerts[2] },
    // verisign 2006 CA
    { { "US", "", "", "VeriSign, Inc.",
        "VeriSign Trust Network, (c) 2006 VeriSign, Inc. - For authorized use only",
        "VeriSign Class 3 Public Primary Certification Authority - G5" },
      SSL_CACERTFLAG_NONE, ASN_OBJ_RSA_PKCS_SHA1, 0,
      256, _ProtoSSL_VeriSign2006ServerModulus,
      3, { 0x01, 0x00, 0x01 },
      0, NULL, NULL, &_ProtoSSL_CACerts[3]},
    // digicert CA
    { { "US", "", "", "DigiCert Inc", "www.digicert.com", "DigiCert Global Root CA" },
      SSL_CACERTFLAG_NONE, ASN_OBJ_RSA_PKCS_SHA256, 0,
      256, _ProtoSSL_DigiCertServerModulus,
      3, { 0x01, 0x00, 0x01 },
      0, NULL, NULL, NULL },
};


/*** Private functions ************************************************************/

/*
    safe (bounded) reading
*/

/*F********************************************************************************/
/*!
    \Function _SafeRead8

    \Description
        Read a uint8_t from buffer 

    \Input *pData           - pointer to buffer to read from
    \Input *pDataEnd        - pointer to end of buffer

    \Output
        uint8_t             - value from buffer, or zero if buffer overrun

    \Version 01/02/2018 (jbrookes)
*/
/********************************************************************************F*/
static uint8_t _SafeRead8(const uint8_t *pData, const uint8_t *pDataEnd)
{
    uint8_t u8 = ((pData+1) <= pDataEnd) ? pData[0] : 0;
    return(u8);
}

/*F********************************************************************************/
/*!
    \Function _SafeRead16

    \Description
        Read a uint16_t from buffer in network order

    \Input *pData           - pointer to buffer to read from
    \Input *pDataEnd        - pointer to end of buffer

    \Output
        uint16_t            - value from buffer, or zero if buffer overrun

    \Version 01/02/2018 (jbrookes)
*/
/********************************************************************************F*/
static uint16_t _SafeRead16(const uint8_t *pData, const uint8_t *pDataEnd)
{
    uint16_t u16 = ((pData+2) <= pDataEnd) ? ((uint16_t)pData[0]<<8)|((uint16_t)pData[1]) : 0;
    return(u16);
}

/*F********************************************************************************/
/*!
    \Function _SafeRead24

    \Description
        Read a 24bit value from buffer in network order

    \Input *pData           - pointer to buffer to read from
    \Input *pDataEnd        - pointer to end of buffer

    \Output
        uint32_t            - value from buffer, or zero if buffer overrun

    \Version 01/02/2018 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _SafeRead24(const uint8_t *pData, const uint8_t *pDataEnd)
{
    uint32_t u32 = ((pData+3) <= pDataEnd) ? ((uint32_t)pData[0]<<16)|((uint32_t)pData[1]<<8)|((uint32_t)pData[2]) : 0;
    return(u32);
}

/*F********************************************************************************/
/*!
    \Function _SafeRead32

    \Description
        Read a uint32_t from buffer in network order

    \Input *pData           - pointer to buffer to read from
    \Input *pDataEnd        - pointer to end of buffer

    \Output
        uint32_t            - value from buffer, or zero if buffer overrun

    \Version 01/02/2018 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _SafeRead32(const uint8_t *pData, const uint8_t *pDataEnd)
{
    uint32_t u32 = ((pData+4) <= pDataEnd) ? ((uint32_t)pData[0]<<24)|((uint32_t)pData[1]<<16)|((uint32_t)pData[2]<<8)|((uint32_t)pData[3]) : 0;
    return(u32);
}

/*F********************************************************************************/
/*!
    \Function _SafeReadBytes

    \Description
        Read specified bytes from buffer

    \Input *pBuffer         - [out] buffer to write to
    \Input uBufSize         - size of output buffe
    \Input *pData           - pointer to buffer to read from
    \Input uNumBytes        - number of bytes to read
    \Input *pDataEnd        - pointer to end of buffer

    \Version 01/02/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _SafeReadBytes(uint8_t *pBuffer, uint32_t uBufSize, const uint8_t *pData, uint32_t uNumBytes, const uint8_t *pDataEnd)
{
    if ((pData+uNumBytes) <= pDataEnd)
    {
        ds_memcpy_s(pBuffer, uBufSize, pData, uNumBytes);
    }
    else
    {
        ds_memclr(pBuffer, uBufSize);
    }
}

/*F********************************************************************************/
/*!
    \Function _SafeReadString

    \Description
        Read length-delimited string from buffer

    \Input *pBuffer         - [out] string buffer to write to
    \Input uBufSize         - size of output buffe
    \Input *pString         - pointer to source to read from
    \Input uStrLen          - length of source string
    \Input *pDataEnd        - pointer to end of buffer

    \Version 01/02/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _SafeReadString(char *pBuffer, uint32_t uBufSize, const char *pString, uint32_t uStrLen, const uint8_t *pDataEnd)
{
    if ((pString+uStrLen) <= (const char *)pDataEnd)
    {
        ds_strsubzcpy(pBuffer, uBufSize, pString, uStrLen);
    }
    else
    {
        ds_memclr(pBuffer, uBufSize);
    }
}

/*
    asn.1 parsing routines
*/

/*F********************************************************************************/
/*!
    \Function _AsnGetHashType

    \Description
        Get CryptHashE from ASN.1 hash object type

    \Input iHashType    - asn.1 hash type

    \Output
        CryptHashTypeE  - hash type

    \Version 11/28/2017 (jbrookes)
*/
/********************************************************************************F*/
static CryptHashTypeE _AsnGetHashType(int32_t iHashType)
{
    CryptHashTypeE eHashType;
    // convert from asn object type to crypt hash type
    switch (iHashType)
    {
        case ASN_OBJ_MD5:
            eHashType = CRYPTHASH_MD5;
            break;
        case ASN_OBJ_SHA1:
            eHashType = CRYPTHASH_SHA1;
            break;
        case ASN_OBJ_SHA256:
            eHashType = CRYPTHASH_SHA256;
            break;
        case ASN_OBJ_SHA384:
            eHashType = CRYPTHASH_SHA384;
            break;
        case ASN_OBJ_SHA512:
            eHashType = CRYPTHASH_SHA512;
            break;
        default:
            eHashType = CRYPTHASH_NULL;
            break;
    }
    // return hash type to caller
    return(eHashType);
}

/*F********************************************************************************/
/*!
    \Function _AsnGetObject

    \Description
        Return OID based on type

    \Input iType    - Type of OID (ASN_OBJ_*)

    \Output
        ASNObjectT *- pointer to OID, or NULL if not found

    \Version 03/18/2015 (jbrookes)
*/
/********************************************************************************F*/
static const ASNObjectT *_AsnGetObject(int32_t iType)
{
    const ASNObjectT *pObject;
    int32_t iIndex;

    // locate the matching type
    for (iIndex = 0, pObject = NULL; _SSL_ObjectList[iIndex].iType != ASN_OBJ_NONE; iIndex += 1)
    {
        if (_SSL_ObjectList[iIndex].iType == iType)
        {
            pObject = &_SSL_ObjectList[iIndex];
            break;
        }
    }

    return(pObject);
}

/*F********************************************************************************/
/*!
    \Function _AsnParseHeader

    \Description
        Parse an asn.1 header

    \Input *pData    - pointer to header data to parse
    \Input *pLast    - pointer to end of header
    \Input *pType    - pointer to storage for header type
    \Input *pSize    - pointer to storage for data size

    \Output uint8_t * - pointer to next block, or NULL if end of stream

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************F*/
static const uint8_t *_AsnParseHeader(const uint8_t *pData, const uint8_t *pLast, int32_t *pType, int32_t *pSize)
{
    int32_t iCnt;
    uint32_t uLen;
    int32_t iType, iSize;

    // reset the output
    if (pSize != NULL)
    {
        *pSize = 0;
    }
    if (pType != NULL)
    {
        *pType = 0;
    }

    /* handle end of data (including early termination due to data truncation or
       invalid data) and make sure we have at least enough data for the type and
       size  */
    if ((pData == NULL) || (pData+2 > pLast))
    {
        return(NULL);
    }

    // get the type
    iType = *pData++;
    if (pType != NULL)
    {
        *pType = iType;
    }

    // figure the length
    if ((uLen = *pData++) > 127)
    {
        // get number of bytes
        iCnt = (uLen & 127);
        // validate length (do not allow overflow) and make sure there is enough data
        if ((iCnt > (int32_t)sizeof(uLen)) || ((pData + iCnt) > pLast))
        {
            return(NULL);
        }
        // calc the length
        for (uLen = 0; iCnt > 0; --iCnt)
        {
            uLen = (uLen << 8) | *pData++;
        }
    }
    iSize = (signed)uLen;
    // validate record size
    if ((iSize < 0) || ((pData+iSize) > pLast))
    {
        return(NULL);
    }
    // save the size
    if (pSize != NULL)
    {
        *pSize = iSize;
    }

    #if DEBUG_ALL_OBJS
    NetPrintf(("protossl: _AsnParseHeader type=%s (0x%02x) size=%d\n",
        _SSL3_strAsnTypes[iType&~(ASN_CONSTRUCT|ASN_IMPLICIT_TAG|ASN_EXPLICIT_TAG)],
        iType, iSize));
    #if DEBUG_RAW_DATA
    NetPrintMem(pData, iSize, _SSL3_strAsnTypes[iType&~(ASN_CONSTRUCT|ASN_IMPLICIT_TAG|ASN_EXPLICIT_TAG)]);
    #endif
    #endif

    // return pointer to next
    return(pData);
}

/*F********************************************************************************/
/*!
    \Function _AsnParseHeaderType

    \Description
        Parse an asn.1 header of specified type

    \Input *pData   - pointer to header data to parse
    \Input *pLast   - pointer to end of header
    \Input iType    - type of header to extract
    \Input *pSize   - pointer to storage for data size

    \Output uint8_t * - pointer to next block, or NULL if end of stream or unexpected type

    \Version 10/10/2012 (jbrookes)
*/
/********************************************************************************F*/
static const uint8_t *_AsnParseHeaderType(const uint8_t *pData, const uint8_t *pLast, int32_t iType, int32_t *pSize)
{
    int32_t iTypeParsed;
    pData = _AsnParseHeader(pData, pLast, &iTypeParsed, pSize);
    if (iTypeParsed != iType)
    {
        return(NULL);
    }
    return(pData);
}

/*F********************************************************************************/
/*!
    \Function _AsnParseObject

    \Description
        Parse an object type

    \Input *pData   - pointer to object
    \Input iSize    - size of object

    \Output
        int32_t     - type of object; zero if object is unrecognized

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _AsnParseObject(const uint8_t *pData, int32_t iSize)
{
    int32_t iType = 0;
    int32_t iIndex;

    // locate the matching type
    for (iIndex = 0; _SSL_ObjectList[iIndex].iType != ASN_OBJ_NONE; ++iIndex)
    {
        if ((iSize >= _SSL_ObjectList[iIndex].iSize) && (memcmp(pData, _SSL_ObjectList[iIndex].strData, _SSL_ObjectList[iIndex].iSize) == 0))
        {
            // save the type and return it
            iType = _SSL_ObjectList[iIndex].iType;

            #if DEBUG_ALL_OBJS
            NetPrintf(("protossl: _AsnParseObject obj=%s (%d)\n", _SSL3_strAsnObjs[iType], iType));
            #if DEBUG_RAW_DATA
            NetPrintMem(pData, _SSL_ObjectList[iIndex].iSize, _SSL3_strAsnObjs[iType]);
            #endif
            #endif

            break;
        }
    }

    #if DEBUG_ALL_OBJS
    if (iType == 0)
    {
        NetPrintMem(pData, iSize, "unrecognized asn.1 object");
    }
    #endif

    return(iType);
}

/*F********************************************************************************/
/*!
    \Function _AsnParseString

    \Description
        Extract a string

    \Input *pData   - pointer to data to extract string from
    \Input iSize    - size of source data
    \Input *pString - pointer to buffer to copy string into
    \Input iLength  - size of buffer
    \Input iType    - ASN string type

    \Notes
        The number of characters copied will be whichever is smaller between
        iSize and iLength-1.  If iLength-1 is greater than iSize (ie, the buffer
        is larger than the source string) the string output in pString will be
        NULL-terminated.

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************F*/
static void _AsnParseString(const uint8_t *pData, int32_t iSize, char *pString, int32_t iLength, int32_t iType)
{
    if (iType != ASN_TYPE_UNICODESTR)
    {
        for (; (iSize > 0) && (iLength > 1); --iSize, --iLength)
        {
            *pString++ = *pData++;
        }
        if (iLength > 0)
        {
            *pString = 0;
        }
    }
    else // we do a straight conversion to ASCII here
    {
        for (pData += 1; (iSize > 0) && (iLength > 1); iSize -= 2, iLength -= 1)
        {
            *pString++ = *pData;
            pData += 2;
        }
        if (iLength > 0)
        {
            *pString = 0;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _AsnParseStringMulti

    \Description
        Extract a string, appending to output instead of overwriting.

    \Input *pData   - pointer to data to extract string from
    \Input iSize    - size of source data
    \Input *pString - pointer to buffer to copy string into
    \Input iLength  - size of buffer

    \Notes
        The number of characters copied will be whichever is smaller between
        iSize and iLength-1.  If iLength-1 is greater than iSize (ie, the buffer
        is larger than the source string) the string output in pString will be
        NULL-terminated.

    \Version 03/25/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _AsnParseStringMulti(const uint8_t *pData, int32_t iSize, char *pString, int32_t iLength)
{
    // find append point
    for (; (*pString != '\0') && (iLength > 1); --iLength)
    {
        pString += 1;
    }
    // extract
    for (; (iSize > 0) && (iLength > 1); --iSize, --iLength)
    {
        *pString++ = *pData++;
    }
    // terminate
    if (iLength > 0)
    {
        *pString = '\0';
    }
}

/*F********************************************************************************/
/*!
    \Function _AsnParseIdent

    \Description
        Extract an Ident (subject/issuer) from certificate

    \Input *pData   - pointer to data to extract string from
    \Input iSize    - size of source data
    \Input *pIdent  - [out] storage for parsed ident fields

    \Output
        const uint8_t *     - pointer past end of ident

    \Version 10/09/2012 (jbrookes)
*/
/********************************************************************************F*/
static const uint8_t *_AsnParseIdent(const uint8_t *pData, int32_t iSize, ProtoSSLCertIdentT *pIdent)
{
    int32_t iObjType, iType;
    const uint8_t *pDataEnd;

    for (iObjType = 0, pDataEnd = pData+iSize; (pData = _AsnParseHeader(pData, pDataEnd, &iType, &iSize)) != NULL; )
    {
        // skip past seqn/set references
        if ((iType == ASN_TYPE_SEQN+ASN_CONSTRUCT) || (iType == ASN_TYPE_SET+ASN_CONSTRUCT))
        {
            continue;
        }
        if (iType == ASN_TYPE_OBJECT+ASN_PRIMITIVE)
        {
            iObjType = _AsnParseObject(pData, iSize);
        }
        if ((iType == ASN_TYPE_PRINTSTR+ASN_PRIMITIVE) || (iType == ASN_TYPE_UTF8STR+ASN_PRIMITIVE) || (iType == ASN_TYPE_T61+ASN_PRIMITIVE) || (iType == ASN_TYPE_UNICODESTR+ASN_PRIMITIVE))
        {
            if (iObjType == ASN_OBJ_COUNTRY)
            {
                _AsnParseString(pData, iSize, pIdent->strCountry, sizeof(pIdent->strCountry), iType);
            }
            if (iObjType == ASN_OBJ_STATE)
            {
                _AsnParseString(pData, iSize, pIdent->strState, sizeof(pIdent->strState), iType);
            }
            if (iObjType == ASN_OBJ_CITY)
            {
                _AsnParseString(pData, iSize, pIdent->strCity, sizeof(pIdent->strCity), iType);
            }
            if (iObjType == ASN_OBJ_ORGANIZATION)
            {
                _AsnParseString(pData, iSize, pIdent->strOrg, sizeof(pIdent->strOrg), iType);
            }
            if (iObjType == ASN_OBJ_UNIT)
            {
                if (pIdent->strUnit[0] != '\0')
                {
                    ds_strnzcat(pIdent->strUnit, ", ", sizeof(pIdent->strUnit));
                }
                _AsnParseStringMulti(pData, iSize, pIdent->strUnit, sizeof(pIdent->strUnit));
            }
            if (iObjType == ASN_OBJ_COMMON)
            {
                _AsnParseString(pData, iSize, pIdent->strCommon, sizeof(pIdent->strCommon), iType);
            }
            iObjType = 0;
        }
        pData += iSize;
    }

    return(pDataEnd);
}

/*F********************************************************************************/
/*!
    \Function _AsnParseDate

    \Description
        Parse and extract a date object from ASN.1 certificate

    \Input *pData   - pointer to header data
    \Input iSize    - size of object
    \Input *pBuffer - [out] output for date string object
    \Input iBufSize - size of output buffer
    \Input *pDateTime - [out] storage for converted time, optional

    \Output
        const uint8_t * - pointer past object, or NULL if an error occurred

    \Version 10/10/2012 (jbrookes)
*/
/********************************************************************************F*/
static const uint8_t *_AsnParseDate(const uint8_t *pData, int32_t iSize, char *pBuffer, int32_t iBufSize, uint64_t *pDateTime)
{
    int32_t iType;
    if (((pData = _AsnParseHeader(pData, pData+iSize, &iType, &iSize)) == NULL) || ((iType != ASN_TYPE_UTCTIME) && (iType != ASN_TYPE_GENERALIZEDTIME)))
    {
        return(NULL);
    }
    _AsnParseString(pData, iSize, pBuffer, iBufSize, iType);
    if (pDateTime != NULL)
    {
        *pDateTime = ds_strtotime2(pBuffer, iType == ASN_TYPE_UTCTIME ? TIMETOSTRING_CONVERSION_ASN1_UTCTIME : TIMETOSTRING_CONVERSION_ASN1_GENTIME);
    }
    pData += iSize;
    return(pData);
}

/*F********************************************************************************/
/*!
    \Function _AsnParseBinaryPtr

    \Description
        Parse and extract a binary object from ASN.1 certificate

    \Input *pData   - pointer to header data
    \Input *pLast   - pointer to end of data
    \Input iType    - type of data we are expecting
    \Input **ppObj   - [out] storage for pointer to binary object
    \Input *pObjSize - [out] storage for size of binary object
    \Input *pName   - name of object (used for debug output)

    \Output
        const uint8_t * - pointer past object, or NULL if an error occurred

    \Version 11/18/2013 (jbrookes)
*/
/********************************************************************************F*/
static const uint8_t *_AsnParseBinaryPtr(const uint8_t *pData, const uint8_t *pLast, int32_t iType, uint8_t **ppObj, int32_t *pObjSize, const char *pName)
{
    int32_t iSize;
    if ((pData = _AsnParseHeaderType(pData, pLast, iType, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _AsnParseBinary: could not get %s\n", pName));
        return(NULL);
    }
    // skip leading zero if present
    if ((iSize > 0) && (*pData == '\0'))
    {
        pData += 1;
        iSize -= 1;
    }
    // save object pointer
    *ppObj = (uint8_t *)pData;
    // save object size
    *pObjSize = iSize;
    // skip object
    return(pData+iSize);
}

/*F********************************************************************************/
/*!
    \Function _AsnParseBinary

    \Description
        Parse and extract a binary object from ASN.1 certificate

    \Input *pData   - pointer to header data
    \Input *pLast   - pointer to end of data
    \Input iType    - type of data we are expecting
    \Input *pBuffer - [out] output for binary object (may be null to skip)
    \Input iBufSize - size of output buffer
    \Input *pOutSize - [out] output for binary object size
    \Input *pName   - name of object (used for debug output)

    \Output
        const uint8_t * - pointer past object, or NULL if an error occurred

    \Version 10/10/2012 (jbrookes)
*/
/********************************************************************************F*/
static const uint8_t *_AsnParseBinary(const uint8_t *pData, const uint8_t *pLast, int32_t iType, uint8_t *pBuffer, int32_t iBufSize, int32_t *pOutSize, const char *pName)
{
    const uint8_t *pObjData = NULL, *pNext;
    // parse object info
    pNext = _AsnParseBinaryPtr(pData, pLast, iType, (uint8_t **)&pObjData, pOutSize, pName);
    // save data
    if ((pObjData != NULL) && (pBuffer != NULL))
    {
        // validate size
        if (*pOutSize > iBufSize)
        {
            NetPrintf(("protossl: _AsnParseBinary: %s is too large (size=%d, max=%d)\n", pName, *pOutSize, iBufSize));
            return(NULL);
        }
        ds_memcpy(pBuffer, pObjData, *pOutSize);
    }
    // skip object
    return(pNext);
}

/*F********************************************************************************/
/*!
    \Function _AsnParseOptional

    \Description
        Parse optional object of ASN.1 certificate

    \Input *pData   - pointer to header data
    \Input iSize    - size of header
    \Input *pCert   - pointer to certificate to fill in from header data

    \Output
        int32_t     - negative=error, zero=no error

    \Version 10/10/2012 (jbrookes) Extracted from _AsnParseCertificate()
*/
/********************************************************************************F*/
static int32_t _AsnParseOptional(const uint8_t *pData, int32_t iSize, X509CertificateT *pCert)
{
    const uint8_t *pLast;
    int32_t iCritical, iObjType, iLen, iType;

    for (iCritical = iObjType = 0, pLast = pData+iSize; (pData = _AsnParseHeader(pData, pLast, &iType, &iSize)) != NULL; )
    {
        // ignore seqn/set references
        if ((iType == ASN_TYPE_SEQN+ASN_CONSTRUCT) || (iType == ASN_TYPE_SET+ASN_CONSTRUCT))
        {
            continue;
        }

        // parse the object type
        if (iType == ASN_TYPE_OBJECT+ASN_PRIMITIVE)
        {
            iObjType = _AsnParseObject(pData, iSize);
        }

        // parse a subject alternative name (SAN) object
        if (iObjType == ASN_OBJ_SUBJECT_ALT)
        {
            if (iType == ASN_TYPE_OCTSTRING+ASN_PRIMITIVE)
            {
                // save reference to subject alternative blob
                pCert->iSubjectAltLen = iSize;
                pCert->pSubjectAlt = pData;
            }
        }

        // parse a basic constraints object
        if (iObjType == ASN_OBJ_BASIC_CONSTRAINTS)
        {
            // obj_basic_constraints: [boolean: critical]<oct><seq><boolean: isCA>[integer: pathLenConstraints]
            if (iType == ASN_TYPE_OCTSTRING+ASN_PRIMITIVE)
            {
                // if no critical flag is present, mark it as critical. ex: https://www.wellsfargo.com/
                if (iCritical == 0)
                {
                    iCritical = 1;
                }
                // do not add pData for oct
                continue;
            }

            if ((iType == ASN_TYPE_BOOLEAN+ASN_PRIMITIVE) && (iSize == 1))
            {
                if (!iCritical)
                {
                    // check if the critical flag is set (the basic constraints MUST be critical)
                    if ((iCritical = *pData) == 0)
                    {
                        return(-1);
                    }
                }
                else
                {
                    // this is the flag to indicate whether it's a CA
                    pCert->iCertIsCA = (*pData != 0) ? TRUE : FALSE;
                }
            }

            if ((iType == ASN_TYPE_INTEGER+ASN_PRIMITIVE) && (pCert->iCertIsCA != FALSE) && (iSize <= (signed)sizeof(pCert->iMaxHeight)))
            {
                for (iLen = 0; iLen < iSize; iLen++)
                {
                    pCert->iMaxHeight = (pCert->iMaxHeight << 8) | pData[iLen];
                }
                /* As per http://tools.ietf.org/html/rfc2459#section-4.2.1.10: A value of zero indicates that only an end-entity certificate
                   may follow in the path. Where it appears, the pathLenConstraint field MUST be greater than or equal to zero. Where pathLenConstraint
                   does not appear, there is no limit to the allowed length of the certification path. In our case (iMaxHeight), a value of zero means
                   no pathLenConstraint is present, 1 means the pathLenConstraint is 0, 2 means the pathLenConstraint is 1, ... */
                //$$ todo - https://tools.ietf.org/html/rfc5280#section-4.2.1.9 updates this to add the keyCertSign bit in the key usage extension as a requirement
                if (pCert->iMaxHeight++ < 0)
                {
                    return(-2);
                }
            }
        }
        pData += iSize;
    }
    if (pCert->pSubjectAlt != NULL)
    {
        NetPrintfVerbose((DEBUG_VAL_CERT, 0, "protossl: parsed SAN; length=%d\n", pCert->iSubjectAltLen));
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _AsnParseSequencedObject

    \Description
        Parse a sequenced object

    \Input *pData   - pointer to object to parse
    \Input *pLast   - pointer to end of data
    \Input *pObjType - [out] storage for parsed object type

    \Output
        uint8_t *    - pointer past end of object, or NULL

    \Version 11/21/2017 (jbrookes) 
*/
/********************************************************************************F*/
static const uint8_t *_AsnParseSequencedObject(const uint8_t *pData, const uint8_t *pLast, int32_t *pObjType)
{
    int32_t iSize;
    if ((pData = _AsnParseHeaderType(pData, pLast, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        return(NULL);
    }
    if ((pData = _AsnParseHeaderType(pData, pLast, ASN_TYPE_OBJECT+ASN_PRIMITIVE, &iSize)) == NULL)
    {
        return(NULL);
    }
    if ((*pObjType = _AsnParseObject(pData, iSize)) < 0)
    {
        return(NULL);
    }
    return(pData+iSize);
}

/*F********************************************************************************/
/*!
    \Function _AsnParseSignatureAlgorithms

    \Description
        Parse Signature Algorithms object of ASN.1 certificate

    \Input *pData   - pointer to header data
    \Input *pLast   - pointer to end of data
    \Input *pCert   - pointer to certificate to fill in from sigalg object

    \Output
        uint8_t *   - pointer past end of object

    \Version 11/21/2017 (jbrookes) 
*/
/********************************************************************************F*/
static const uint8_t *_AsnParseSignatureAlgorithms(const uint8_t *pData, const uint8_t *pLast, X509CertificateT *pCert)
{
    int32_t iMgfFunc, iSize;
    const uint8_t *pData2;

    // parse signature algorithm identifier
    if ((pData = _AsnParseHeaderType(pData, pLast, ASN_TYPE_OBJECT+ASN_PRIMITIVE, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _AsnParseCertificate: could not get signature algorithm identifier\n"));
        return(NULL);
    }
    pCert->iSigType = _AsnParseObject(pData, iSize);
    pData += iSize;

    // RSA-PKCS?
    if ((pCert->iSigType >= ASN_OBJ_RSA_PKCS_MD5) && (pCert->iSigType <= ASN_OBJ_RSA_PKCS_SHA512))
    {
        // get sig hash type from sig type
        pCert->iSigHash = pCert->iSigType-ASN_OBJ_RSA_PKCS_MD5+ASN_OBJ_MD5;
        return(pData);
    }
    else if (pCert->iSigType == ASN_OBJ_ECDSA_SHA256)
    {
        pCert->iSigHash = ASN_OBJ_SHA256;
        return(pData);
    }
    else if (pCert->iSigType == ASN_OBJ_ECDSA_SHA384)
    {
        pCert->iSigHash = ASN_OBJ_SHA384;
        return(pData);
    }
    else if (pCert->iSigType != ASN_OBJ_RSASSA_PSS)
    {
        NetPrintf(("protossl: unsupported signature algorithm %d\n", pCert->iSigType));
        return(NULL);
    }

    // set default parameter types, which are used when only partial settings are specified
    pCert->iSigHash = ASN_OBJ_SHA1;
    pCert->iSigSalt = 20;
    iMgfFunc = ASN_OBJ_PKCS1_MGF1;
    pCert->iMgfHash = ASN_OBJ_SHA1;

    // parse rsassa-pss parameter sequence; see https://tools.ietf.org/html/rfc3447#appendix-A.2.3
    if ((pData = _AsnParseHeaderType(pData, pLast, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _AsnParseCertificate: could not parse rsassa_pss parameter sequence\n"));
        return(NULL);
    }

    // parse optional signature hash object
    if ((pData2 = _AsnParseHeaderType(pData, pLast, ASN_EXPLICIT_TAG+ASN_PRIMITIVE+0, &iSize)) != NULL)
    {
        if ((pData = _AsnParseSequencedObject(pData2, pLast, &pCert->iSigHash)) == NULL)
        {
            NetPrintf(("protossl: _AsnParseSignatureAlgorithm: error parsing rsassa_pss hash parameter\n"));
            return(NULL);
        }
    }

    // parse optional mgf+hash object
    if ((pData2 = _AsnParseHeaderType(pData, pLast, ASN_EXPLICIT_TAG+ASN_PRIMITIVE+1, &iSize)) != NULL)
    {
        // parse mgf
        if (((pData2 = _AsnParseSequencedObject(pData2, pLast, &iMgfFunc)) == NULL) || (iMgfFunc != ASN_OBJ_PKCS1_MGF1))
        {
            NetPrintf(("protossl: _AsnParseSignatureAlgorithm: error parsing rsassa_pss mgf parameter"));
            return(NULL);
        }
        // parse mgf hash
        if ((pData = _AsnParseSequencedObject(pData2, pLast, &pCert->iMgfHash)) == NULL)
        {
            NetPrintf(("protossl: _AsnParseSignatureAlgorithm: error parsing rsassa_pss hash parameter\n"));
            return(NULL);
        }
    }

    // look for explicit signature parameters (optional)
    if ((pData2 = _AsnParseHeaderType(pData, pLast, ASN_EXPLICIT_TAG+ASN_PRIMITIVE+2, &iSize)) != NULL)
    {
        int32_t iSigSaltLen = 0;
        uint8_t uSigSalt = 0;

        // parse salt length
        if (((pData = _AsnParseBinary(pData2, pLast, ASN_TYPE_INTEGER+ASN_PRIMITIVE, &uSigSalt, sizeof(uSigSalt), &iSigSaltLen, "salt length")) == NULL) || (iSigSaltLen != 1))
        {
            return(NULL);
        }
        pCert->iSigSalt = uSigSalt;
    }

    #if DEBUG_VAL_CERT
    NetPrintf(("protossl: rsassa-pss parameters\n"));
    NetPrintf(("protossl: iSigType=%d\n", pCert->iSigType));
    NetPrintf(("protossl: iSigHash=%d\n", pCert->iSigHash));
    NetPrintf(("protossl: iMgfFunc=%d\n", iMgfFunc));
    NetPrintf(("protossl: iMgfHash=%d\n", pCert->iMgfHash));
    NetPrintf(("protossl: iSigSalt=%d\n", pCert->iSigSalt));
    #endif

    // return pointer past end of data
    return(pData);
}

/*F********************************************************************************/
/*!
    \Function _AsnParseCertificate

    \Description
        Parse an x.509 certificate in ASN.1 format

    \Input *pCert   - pointer to certificate to fill in from header data
    \Input *pData   - pointer to header data
    \Input iSize    - size of header

    \Output
        int32_t     - negative=error, zero=no error

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _AsnParseCertificate(X509CertificateT *pCert, const uint8_t *pData, int32_t iSize)
{
    const uint8_t *pInfData;
    const uint8_t *pInfSkip;
    const uint8_t *pSigSkip;
    const uint8_t *pKeySkip;
    const uint8_t *pKeyData;
    const uint8_t *pLast = pData+iSize;
    const CryptHashT *pHash;
    int32_t iKeySize, iType;

    // clear the certificate
    ds_memclr(pCert, sizeof(*pCert));

    // process the base sequence
    if ((pData = _AsnParseHeaderType(pData, pLast, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _AsnParseCertificate: could not parse base sequence\n"));
        return(-1);
    }

    // process the info sequence
    if ((pData = _AsnParseHeaderType(pInfData = pData, pLast, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _AsnParseCertificate: could not parse info sequence\n"));
        return(-2);
    }
    pInfSkip = pData+iSize;

    // skip any non-integer tag (optional version)
    if (*pData != ASN_TYPE_INTEGER+ASN_PRIMITIVE)
    {
        if ((pData = _AsnParseHeader(pData, pLast, NULL, &iSize)) == NULL)
        {
            NetPrintf(("protossl: _AsnParseCertificate: could not skip non-integer tag\n"));
            return(-3);
        }
        pData += iSize;
    }

    // grab the certificate serial number
    if (((pData = _AsnParseHeader(pData, pInfSkip, &iType, &iSize)) == NULL) || ((unsigned)iSize > sizeof(pCert->SerialData)))
    {
        NetPrintf(("protossl: _AsnParseCertificate: could not get certificate serial number (iSize=%d)\n", iSize));
        return(-4);
    }
    pCert->iSerialSize = iSize;
    ds_memcpy(pCert->SerialData, pData, iSize);
    pData += iSize;

    // find signature algorithm
    if ((pData = _AsnParseHeaderType(pData, pInfSkip, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _AsnParseCertificate: could not get signature algorithm\n"));
        return(-5);
    }
    pSigSkip = pData+iSize;

    // get the signature algorithm type
    if ((pData = _AsnParseHeaderType(pData, pInfSkip, ASN_TYPE_OBJECT+ASN_PRIMITIVE, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _AsnParseCertificate: could not get signature algorithm type\n"));
        return(-6);
    }
    if ((pCert->iSigType = _AsnParseObject(pData, iSize)) == ASN_OBJ_NONE)
    {
        NetPrintMem(pData, iSize, "protossl: unsupported signature algorithm");
        return(-7);
    }

    // parse issuer
    if ((pData = _AsnParseHeaderType(pSigSkip, pInfSkip, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _AsnParseCertificate: could not get issuer\n"));
        return(-8);
    }
    pData = _AsnParseIdent(pData, iSize, &pCert->Issuer);

    // parse the validity info
    if ((pData = _AsnParseHeaderType(pData, pInfSkip, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _AsnParseCertificate: could not get validity info\n"));
        return(-9);
    }

    // get validity dates
    if ((pData = _AsnParseDate(pData, iSize, pCert->strGoodFrom, sizeof(pCert->strGoodFrom), &pCert->uGoodFrom)) == NULL)
    {
        NetPrintf(("protossl: _AsnParseCertificate: could not get from date\n"));
        return(-10);
    }
    if ((pData = _AsnParseDate(pData, iSize, pCert->strGoodTill, sizeof(pCert->strGoodTill), &pCert->uGoodTill)) == NULL)
    {
        NetPrintf(("protossl: _AsnParseCertificate: could not get to date\n"));
        return(-11);
    }
    #if DEBUG_VAL_CERT
    {
        char strTimeFrom[32], strTimeTill[32];
        NetPrintf(("protossl: certificate valid from %s to %s\n",
            ds_secstostr(pCert->uGoodFrom, TIMETOSTRING_CONVERSION_RFC_0822, FALSE, strTimeFrom, sizeof(strTimeFrom)),
            ds_secstostr(pCert->uGoodTill, TIMETOSTRING_CONVERSION_RFC_0822, FALSE, strTimeTill, sizeof(strTimeTill))));
    }
    #endif

    // get subject
    if ((pData = _AsnParseHeaderType(pData, pInfSkip, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _AsnParseCertificate: could not get subject\n"));
        return(-12);
    }
    pData = _AsnParseIdent(pData, iSize, &pCert->Subject);

    // parse the public key
    if ((pData = _AsnParseHeaderType(pData, pInfSkip, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _AsnParseCertificate: could not get public key\n"));
        return(-13);
    }

    // find the key algorithm sequence
    if ((pData = _AsnParseHeaderType(pData, pInfSkip, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _AsnParseCertificate: could not get key algorithm sequence\n"));
        return(-14);
    }
    pKeySkip = pData+iSize;

    // grab the public key algorithm
    if ((pData = _AsnParseHeaderType(pData, pKeySkip, ASN_TYPE_OBJECT+ASN_PRIMITIVE, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _AsnParseCertificate: could not get public key algorithm\n"));
        return(-15);
    }
    pCert->iKeyType = _AsnParseObject(pData, iSize);

    // if ecdsa, grab the curve type before we move past the public key object
    if (pCert->iKeyType == ASN_OBJ_ECDSA_KEY)
    {
        if ((pData = _AsnParseHeaderType(pData+iSize, pKeySkip, ASN_TYPE_OBJECT+ASN_PRIMITIVE, &iSize)) == NULL)
        {
            NetPrintf(("protossl: _AsnParseCertificate: could not get ecdsa curve type\n"));
            return(-15);
        }
        pCert->iCrvType = _AsnParseObject(pData, iSize);
    }

    // find the actual public key
    if (((pData = _AsnParseHeaderType(pKeySkip, pLast, ASN_TYPE_BITSTRING+ASN_PRIMITIVE, &iSize)) == NULL) || (iSize < 1))
    {
        NetPrintf(("protossl: _AsnParseCertificate: could not get actual public key\n"));
        return(-16);
    }

    // skip the "extra bits" indicator and save keydata ref
    pKeyData = pData+1;
    iKeySize = iSize-1;
    pData += iSize;

    // parse optional info object, if present; see https://tools.ietf.org/html/rfc5280#section-4.1 
    if (((pData = _AsnParseHeaderType(pData, pInfSkip, ASN_EXPLICIT_TAG+3, &iSize)) != NULL) && (_AsnParseOptional(pData, iSize, pCert) < 0))
    {
        return(-17);
    }

    // parse signature algorithm sequence
    if ((pData = _AsnParseHeaderType(pInfSkip, pLast, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _AsnParseCertificate: could not get signature algorithm sequence\n"));
        return(-18);
    }
    pSigSkip = pData+iSize;

    // parse signature algorithms
    if ((pData = _AsnParseSignatureAlgorithms(pData, pLast, pCert)) == NULL)
    {
        return(-19);
    }

    // parse the signature data
    if ((pCert->iSigType == ASN_OBJ_ECDSA_SHA256) || (pCert->iSigType == ASN_OBJ_ECDSA_SHA384) || (pCert->iSigType == ASN_OBJ_ECDSA_SHA512))
    {
        // find the bitstring object
        if ((pData = _AsnParseHeaderType(pSigSkip, pLast, ASN_TYPE_BITSTRING+ASN_PRIMITIVE, &iSize)) == NULL)
        {
            NetPrintf(("protossl: _AsnParseCertificate: could not get signature algorithm identifier\n"));
            return(-20);
        }
        // skip leading pad byte; note we assume it is zero (no padding)
        pData += 1;
        iSize -= 1;
        // save signature object data; we will parse it later
        ds_memcpy_s(pCert->SigData, sizeof(pCert->SigData), pData, iSize);
        pCert->iSigSize = iSize;
    }
    else if ((pData = _AsnParseBinary(pSigSkip, pLast, ASN_TYPE_BITSTRING+ASN_PRIMITIVE, pCert->SigData, sizeof(pCert->SigData), &pCert->iSigSize, "signature data")) == NULL)
    {
        return(-21);
    }

    // parse the public key data (extract modulus & exponent)
    if ((pCert->iKeyType == ASN_OBJ_RSA_PKCS_KEY) || (pCert->iKeyType == ASN_OBJ_RSASSA_PSS))
    {
        pLast = pKeyData+iKeySize;

        // parse the sequence
        if ((pData = _AsnParseHeaderType(pKeyData, pLast, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
        {
            NetPrintf(("protossl: _AsnParseCertificate: could not get public key sequence\n"));
            return(-22);
        }
        // parse the key modulus
        if ((pData = _AsnParseBinary(pData, pLast, ASN_TYPE_INTEGER+ASN_PRIMITIVE, pCert->KeyModData, sizeof(pCert->KeyModData), &pCert->iKeyModSize, "key modulus")) == NULL)
        {
            return(-23);
        }
        #if DEBUG_MOD_PRNT
        NetPrintMem(pCert->KeyModData, pCert->iKeyModSize, "public key modulus");
        #endif
        // parse the key exponent
        if ((pData = _AsnParseBinary(pData, pLast, ASN_TYPE_INTEGER+ASN_PRIMITIVE, pCert->KeyExpData, sizeof(pCert->KeyExpData), &pCert->iKeyExpSize, "key exponent")) == NULL)
        {
            return(-24);
        }
    }
    else if (pCert->iKeyType == ASN_OBJ_ECDSA_KEY)
    {
        // copy the curve data; unlike rsa, keydata points to the actual data
        ds_memcpy_s(pCert->KeyModData, sizeof(pCert->KeyModData), pKeyData, iKeySize);
        pCert->iKeyModSize = iKeySize;
        #if DEBUG_MOD_PRNT
        NetPrintMem(pCert->KeyModData, pCert->iKeyModSize, "public key curve data");
        #endif
    }
    else
    {
        NetPrintf(("protossl: _AsnParseCertificate: unrecognized key type %d\n", pCert->iKeyType));
        return(-25);
    }

    // get cert hash type
    pCert->eHashType = _AsnGetHashType(pCert->iSigHash);

    // generate the certificate hash
    if ((pHash = CryptHashGet(pCert->eHashType)) != NULL)
    {
        // do the hash
        uint8_t aContext[CRYPTHASH_MAXSTATE];
        pHash->Init(aContext, pHash->iHashSize);
        pHash->Update(aContext, pInfData, (int32_t)(pInfSkip-pInfData));
        pHash->Final(aContext, pCert->HashData,  pHash->iHashSize);
        // save hash size
        pCert->iHashSize = pHash->iHashSize;
    }
    else
    {
        NetPrintf(("protossl: could not get hash type %d\n", pCert->iSigHash));
        return(-26);
    }

    // certificate parsed successfully
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _AsnParsePrivateKeyRSA

    \Description
        Parse public key modulus and public or private key exponent from private
        key certificate.

    \Input *pPrivateKeyData - private key certificate data in base64 form
    \Input iPrivateKeyLen   - length of private key certificate data
    \Input *pPrivateKey     - [out] parsed private key data

    \Output
        int32_t             - private key certificate binary (decoded) size, or negative on error

    \Notes
        \verbatim
            RSAPrivateKey ::= SEQUENCE {
              version Version,
              modulus INTEGER,
              publicExponent INTEGER,
              privateExponent INTEGER,
              primeP INTEGER,
              primeQ INTEGER,
              exponentP INTEGER,
              exponentQ INTEGER,
              coefficient INTEGER
            }
        \endverbatim

    \Version 03/15/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _AsnParsePrivateKeyRSA(const char *pPrivateKeyData, int32_t iPrivateKeyLen, X509PrivateKeyT *pPrivateKey)
{
    int32_t iPrivKeySize, iSize, iSize2, iTemp = 0;
    const uint8_t *pData, *pData2;

    // clear output structure
    ds_memclr(pPrivateKey, sizeof(*pPrivateKey));

    // base64 decode into buffer
    if ((iPrivKeySize = Base64Decode3(pPrivateKeyData, iPrivateKeyLen, pPrivateKey->strPrivKeyData, sizeof(pPrivateKey->strPrivKeyData))) == 0)
    {
        NetPrintf(("protossl: _AsnParsePrivateKey: could not decode private key\n"));
        return(-2);
    }
    // parse the sequence
    if ((pData = _AsnParseHeaderType((uint8_t *)pPrivateKey->strPrivKeyData, (uint8_t *)pPrivateKey->strPrivKeyData+iPrivKeySize, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _AsnParsePrivateKey: could not get private key sequence\n"));
        return(-3);
    }
    // skip the version
    if ((pData = _AsnParseBinary(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, NULL, 0, &iTemp, "version")) == NULL)
    {
        return(-4);
    }
    // check for sequence... if there is one, this is an RSA-PSS PKCS#8 certificate with some additional stuff we need to skip
    if ((pData2 = _AsnParseHeaderType(pData, pData+iSize, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize2)) != NULL)
    {
        if ((pData = _AsnParseHeaderType(pData2, pData2+iSize2, ASN_TYPE_OBJECT+ASN_PRIMITIVE, &iSize2)) == NULL)
        {
            return(-5);
        }
        if (_AsnParseObject(pData, iSize2) != ASN_OBJ_RSASSA_PSS)
        {
            return(-6);
        }
        pData += iSize2;
        if ((pData = _AsnParseHeaderType(pData, pData+iSize, ASN_TYPE_OCTSTRING+ASN_PRIMITIVE, &iSize)) == NULL)
        {
            return(-7);
        }
        if ((pData = _AsnParseHeaderType(pData, pData+iSize, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
        {
            return(-8);
        }
        if ((pData = _AsnParseBinary(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, NULL, 0, &iTemp, "zero")) == NULL)
        {
            return(-9);
        }
    }
    // parse public key modulus
    if ((pData = _AsnParseBinaryPtr(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, &pPrivateKey->Modulus.pObjData, &pPrivateKey->Modulus.iObjSize, "key modulus")) == NULL)
    {
        return(-10);
    }
    // parse public key exponent
    if ((pData = _AsnParseBinaryPtr(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, &pPrivateKey->PublicExponent.pObjData, &pPrivateKey->PublicExponent.iObjSize, "public key exponent")) == NULL)
    {
        return(-11);
    }
    // parse private key exponent
    if ((pData = _AsnParseBinaryPtr(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, &pPrivateKey->PrivateExponent.pObjData, &pPrivateKey->PrivateExponent.iObjSize, "private key exponent")) == NULL)
    {
        return(-12);
    }
    // parse primeP
    if ((pData = _AsnParseBinaryPtr(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, &pPrivateKey->PrimeP.pObjData, &pPrivateKey->PrimeP.iObjSize, "primeP")) == NULL)
    {
        return(-13);
    }
    // parse primeQ
    if ((pData = _AsnParseBinaryPtr(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, &pPrivateKey->PrimeQ.pObjData, &pPrivateKey->PrimeQ.iObjSize, "primeQ")) == NULL)
    {
        return(-14);
    }
    // parse exponentP
    if ((pData = _AsnParseBinaryPtr(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, &pPrivateKey->ExponentP.pObjData, &pPrivateKey->ExponentP.iObjSize, "exponentP")) == NULL)
    {
        return(-15);
    }
    // parse exponentQ
    if ((pData = _AsnParseBinaryPtr(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, &pPrivateKey->ExponentQ.pObjData, &pPrivateKey->ExponentQ.iObjSize, "exponentQ")) == NULL)
    {
        return(-16);
    }
    // parse coefficient
    if ((pData = _AsnParseBinaryPtr(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, &pPrivateKey->Coefficient.pObjData, &pPrivateKey->Coefficient.iObjSize, "coefficient")) == NULL)
    {
        return(-17);
    }
    return(iPrivKeySize);
}

/*F********************************************************************************/
/*!
    \Function _AsnParsePrivateKeyEcdsa

    \Description
        Parse public key modulus and public or private key exponent from private
        key certificate.

    \Input *pPrivateKeyData - private key certificate data in base64 form
    \Input iPrivateKeyLen   - length of private key certificate data
    \Input *pPrivateKey     - [out] parsed private key data

    \Output
        int32_t             - private key certificate binary (decoded) size, or negative on error

    \Notes
    
        ref: https://tools.ietf.org/html/rfc5915#section-3
    
        ECPrivateKey ::= SEQUENCE {
            version        INTEGER { ecPrivkeyVer1(1) } (ecPrivkeyVer1),
            privateKey     OCTET STRING,
            parameters [0] ECParameters {{ NamedCurve }} OPTIONAL,
            publicKey  [1] BIT STRING OPTIONAL
        }

    \Version 03/09/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _AsnParsePrivateKeyEcdsa(const char *pPrivateKeyData, int32_t iPrivateKeyLen, X509PrivateKeyT *pPrivateKey)
{
    int32_t iPrivKeySize, iSize, iTemp = 0;
    const uint8_t *pData;

    // clear output structure
    ds_memclr(pPrivateKey, sizeof(*pPrivateKey));

    // decode private key into private key buffer
    if ((iPrivKeySize = Base64Decode3(pPrivateKeyData, iPrivateKeyLen, pPrivateKey->strPrivKeyData, sizeof(pPrivateKey->strPrivKeyData))) == 0)
    {
        NetPrintf(("protossl: _AsnParsePrivateKey: could not decode private key\n"));
        return(-2);
    }
    // parse the sequence
    if ((pData = _AsnParseHeaderType((uint8_t *)pPrivateKey->strPrivKeyData, (uint8_t *)pPrivateKey->strPrivKeyData+iPrivKeySize, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _AsnParsePrivateKey: could not get private key sequence\n"));
        return(-3);
    }
    // skip the version
    if ((pData = _AsnParseBinary(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, NULL, 0, &iTemp, "version")) == NULL)
    {
        return(-4);
    }
    // parse the private key; we store it in the Modulus field
    if ((pData = _AsnParseBinaryPtr(pData, pData+iSize, ASN_TYPE_OCTSTRING+ASN_PRIMITIVE, &pPrivateKey->Modulus.pObjData, &pPrivateKey->Modulus.iObjSize, "key modulus")) == NULL)
    {
        return(-5);
    }
    return(iPrivKeySize);
}

/*
    asn.1 writing
*/

/*F********************************************************************************/
/*!
    \Function _AsnWriteBn

    \Description
        Writes a BigNumber as an ASN.1 object

    \Input *pBuffer     - [out] storage for generated object
    \Input iBufSize     - size of output buffer
    \Input *pBn         - BigNumber to write
    \Input uAsnType     - ASN.1 object type to write

    \Output
        uint8_t *       - pointer past end of written object

    \Version 03/13/2018 (jbrookes)
*/
/********************************************************************************F*/
static uint8_t *_AsnWriteBn(uint8_t *pBuffer, int32_t iBufSize, const CryptBnT *pBn, uint8_t uAsnType)
{
    int32_t iByteLen = CryptBnByteLen(pBn);
    uint8_t *pSize;
    *pBuffer++ = uAsnType;
    pSize = pBuffer++;
    if ((uAsnType & (ASN_TYPE_BITSTRING|ASN_TYPE_INTEGER)) && (CryptBnBitTest(pBn, (iByteLen*8)-1)))
    {
        *pBuffer++ = 0;
    }
    CryptBnFinal(pBn, pBuffer, iByteLen);
    pBuffer += iByteLen;
    *pSize = pBuffer-pSize-1;
    return(pBuffer);
}

/*F********************************************************************************/
/*!
    \Function _AsnWriteSignatureEcdsa

    \Description
        Writes an ASN.1 Elliptic Curve signature

    \Input *pSigData    - [out] storage for generated ASN.1 encoded signature
    \Input iSigSize     - size of output buffer
    \Input *pSignature  - signature to write

    \Output
        int32_t         - size of written signature

    \Version 03/13/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _AsnWriteSignatureEcdsa(uint8_t *pSigData, int32_t iSigSize, CryptEccPointT *pSignature)
{
    uint8_t *pSigOut = pSigData, *pSigLen;
    // write the signature to output buffer
    *pSigOut++ = ASN_CONSTRUCT|ASN_TYPE_SEQN;
    // save pointer to length
    pSigLen = pSigOut++;
    // add signature.x
    pSigOut = _AsnWriteBn(pSigOut, iSigSize, &pSignature->X, ASN_TYPE_INTEGER|ASN_PRIMITIVE);
    // add signature.y
    pSigOut = _AsnWriteBn(pSigOut, iSigSize, &pSignature->Y, ASN_TYPE_INTEGER|ASN_PRIMITIVE);
    // write construct length (does not include type or length)
    *pSigLen = pSigOut-pSigData-2;
    // return total size
    return(*pSigLen+2);
}

/*F********************************************************************************/
/*!
    \Function _AsnWriteDigitalHashObject

    \Description
        Generate a DigitalHash object

    \Input *pBuffer     - [out] storage for generated digitalhash object
    \Input iBufSize     - size of output buffer
    \Input *pHashData   - pointer to hash data to embed in the object (may be NULL)
    \Input iHashSize    - size of hash data
    \Input eHashType    - type of hash

    \Output
        int32_t         - size of generated object

    \Version 04/30/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _AsnWriteDigitalHashObject(uint8_t *pBuffer, int32_t iBufSize, const uint8_t *pHashData, int32_t iHashSize, CryptHashTypeE eHashType)
{
    const ASNObjectT *pObject;
    uint8_t *pBufStart = pBuffer;

    // make sure there's space
    if (iBufSize < (CRYPTHASH_MAXDIGEST+64))
    {
        NetPrintf(("protossl: insufficient space for digital hash object\n"));
        return(0);
    }
    // get ASN object to encode
    if ((pObject = _AsnGetObject(_SSL3_CrypttoASN[eHashType])) == NULL)
    {
        NetPrintf(("protossl: failed to get digital hash ASN object\n"));
        return(0);
    }

    /* generate DER-encoded DigitalHash object as per http://tools.ietf.org/html/rfc5246#section-4.7; note that
       this code assumes the total object size is 127 bytes or fewer as it uses single-byte length encoding.
       format defined by https://tools.ietf.org/html/rfc8017#section-9.2 */
    *pBuffer++ = ASN_CONSTRUCT | ASN_TYPE_SEQN;
    *pBuffer++ = (2) + (2 + pObject->iSize) + (2) + (2 + iHashSize);
    *pBuffer++ = ASN_CONSTRUCT | ASN_TYPE_SEQN;
    *pBuffer++ = (2 + pObject->iSize) + (2);
    // add hash object
    *pBuffer++ = ASN_TYPE_OBJECT;
    *pBuffer++ = pObject->iSize;
    // add asn.1 object identifier for hash
    ds_memcpy(pBuffer, pObject->strData, pObject->iSize);
    pBuffer += pObject->iSize;
    // add null object
    *pBuffer++ = ASN_TYPE_NULL;
    *pBuffer++ = 0x00;
    // add hash object (octet string)
    *pBuffer++ = ASN_TYPE_OCTSTRING;
    *pBuffer++ = iHashSize;
    // add hash data
    if (pHashData != NULL)
    {
        ds_memcpy(pBuffer, pHashData, iHashSize);
        pBuffer += iHashSize;
    }

    // return final object size
    return(pBuffer - pBufStart);
}

/*
    certificate functions
*/

/*F********************************************************************************/
/*!
    \Function _CertificateFindSignature

    \Description
        Find PEM signature in the input data, if it exists, and return a
        pointer to the encapsulated data.

    \Input *pCertData       - pointer to data to scan
    \Input iCertSize        - size of input data
    \Input *pSigText        - signature text to find

    \Output
        const uint8_t *     - pointer to data, or null if not found

    \Version 01/14/2009 (jbrookes)
*/
/********************************************************************************F*/
static const uint8_t *_CertificateFindSignature(const uint8_t *pCertData, int32_t iCertSize, const char *pSigText)
{
    int32_t iSigLen = (int32_t)strlen(pSigText);
    int32_t iCertIdx;

    for (iCertIdx = 0; iCertIdx < iCertSize; iCertIdx += 1)
    {
        if ((pCertData[iCertIdx] == *pSigText) && ((iCertSize - iCertIdx) >= iSigLen))
        {
            if (!strncmp((const char *)pCertData+iCertIdx, pSigText, iSigLen))
            {
                return(pCertData+iCertIdx);
            }
        }
    }
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function _CertificateFindData

    \Description
        Finds PEM signature in the input data, if it exists, and stores the
        offsets to the beginning and end of the embedded signature data.

    \Input *pCertData       - pointer to data to scan
    \Input iCertSize        - size of input data
    \Input **pCertBeg       - [out] storage for index to beginning of certificate data
    \Input **pCertEnd       - [out] storage for index to end of certificate data
    \Input *pCertType       - [out] storage for certificate type (ASN_OBJ_\*)

    \Output
        int32_t             - 0=did not find certificate, else did find certificate

    \Version 01/14/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CertificateFindData(const uint8_t *pCertData, int32_t iCertSize, const uint8_t **pCertBeg, const uint8_t **pCertEnd, uint32_t *pCertType)
{
    const CertificateSignatureT *pCertSig;
    int32_t iCertSig;

    for (iCertSig = 0; iCertSig < (signed)(sizeof(_SSL3_CertSignatures)/sizeof(_SSL3_CertSignatures[0])); iCertSig += 1)
    {
        pCertSig = &_SSL3_CertSignatures[iCertSig];

        // make sure "end-cert" occurs after start since we support bundles
        if (((*pCertBeg = _CertificateFindSignature(pCertData, iCertSize, pCertSig->pCertBeg)) != NULL) &&
            ((*pCertEnd = _CertificateFindSignature(*pCertBeg, (int32_t)(pCertData+iCertSize-*pCertBeg), pCertSig->pCertEnd)) != NULL))
        {
            // skip begin signature
            *pCertBeg += strlen(pCertSig->pCertBeg);
            // write cert type
            *pCertType = pCertSig->uCertType;
            // return size to caller
            return((int32_t)(*pCertEnd-*pCertBeg));
        }
    }

    // if no signature, assume it's just straight base64 pkcs1
    *pCertBeg = pCertData;
    *pCertEnd = pCertData + iCertSize;
    *pCertType = ASN_OBJ_RSA_PKCS_KEY;
    return(iCertSize);
}

/*F********************************************************************************/
/*!
    \Function _CertificateDecodePrivate

    \Description
        Parse public key modulus and public or private key exponent from private
        key certificate.

    \Input *pPrivateKeyData - private key certificate data in base64 form
    \Input iPrivateKeyLen   - length of private key certificate data
    \Input *pPrivateKey     - [out] parsed private key data

    \Output
        int32_t             - private key certificate binary (decoded) size, or negative on error

    \Version 03/09/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CertificateDecodePrivate(const char *pPrivateKeyData, int32_t iPrivateKeyLen, X509PrivateKeyT *pPrivateKey)
{
    const uint8_t *pCertBeg, *pCertEnd;
    int32_t iCertSize, iResult;
    uint32_t uCertType;

    // make sure we have key data first
    if (pPrivateKeyData == NULL)
    {
        NetPrintf(("protossl: _AsnParsePrivateKey: no private key set\n"));
        return(-1);
    }

    // find the base64-encoded certificate data
    iCertSize = _CertificateFindData((const uint8_t *)pPrivateKeyData, iPrivateKeyLen, &pCertBeg, &pCertEnd, &uCertType);

    // parse ecdsa or rsa?
    if (uCertType == ASN_OBJ_ECDSA_KEY)
    {
        iResult = _AsnParsePrivateKeyEcdsa((const char *)pCertBeg, iCertSize, pPrivateKey);
    }
    else
    {
        iResult = _AsnParsePrivateKeyRSA((const char *)pCertBeg, iCertSize, pPrivateKey);
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _CertificateDecodePublic

    \Description
        Decode the specified PEM certificate into binary format, parse and extract some information

    \Input *pState      - module state reference
    \Input *pPemData    - certificate to decode
    \Input iPemSize     - certificate length

    \Output
        CertificateDataT * - pointer to new certificate data, or null

    \Version 03/09/2018 (jbrookes)
*/
/********************************************************************************F*/
static CertificateDataT *_CertificateDecodePublic(ProtoSSLRefT *pState, const uint8_t *pPemData, int32_t iPemSize)
{
    const uint8_t *pCertBeg, *pCertEnd;
    CertificateDataT *pCertificate = NULL;
    X509CertificateT Cert;
    int32_t iCertSize;
    uint32_t uCertType;

    // find the base64-encoded certificate data
    iPemSize = _CertificateFindData(pPemData, iPemSize, &pCertBeg, &pCertEnd, &uCertType);
    // get decoded size
    iCertSize = Base64Decode3((const char *)pCertBeg, iPemSize, NULL, 0x7fffffff);
    // allocate certificate buffer
    if ((iCertSize > 0) && ((pCertificate = DirtyMemAlloc(sizeof(*pCertificate)+iCertSize, PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) != NULL))
    {
        // base64 decode to binary 
        Base64Decode3((const char *)pCertBeg, iPemSize, (char *)pCertificate->aCertData, iCertSize);
        // parse the certificate
        _AsnParseCertificate(&Cert, pCertificate->aCertData, iCertSize);
        // save some cert info
        pCertificate->iCertSize = iCertSize;
        pCertificate->iCrvType = Cert.iCrvType;
        pCertificate->iKeyType = Cert.iKeyType;
        pCertificate->iSigType = Cert.iSigType;
    }
    return(pCertificate);
}

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function _CertificateDebugFormatIdent

    \Description
        Print cert ident info to debug output.

    \Input *pIdent  - cert ident info
    \Input *pStrBuf - [out] buffer to format cert ident into
    \Input iBufLen  - length of buffer

    \Output
        char *      - pointer to result string

    \Version 01/13/2009 (jbrookes)
*/
/********************************************************************************F*/
static char *_CertificateDebugFormatIdent(const ProtoSSLCertIdentT *pIdent, char *pStrBuf, int32_t iBufLen)
{
    ds_snzprintf(pStrBuf, iBufLen, "C=%s, ST=%s, L=%s, O=%s, OU=%s, CN=%s",
        pIdent->strCountry, pIdent->strState, pIdent->strCity,
        pIdent->strOrg, pIdent->strUnit, pIdent->strCommon);
    return(pStrBuf);
}
#else
#define _CertificateDebugFormatIdent(__pIdent, __pStrBuf, __iBufLen)
#endif

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function _CertificateDebugPrint

    \Description
        Print cert ident info to debug output.

    \Input *pCert       - cert to print debug info for
    \Input *pMessage    - string identifier (may be null)

    \Version 01/13/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _CertificateDebugPrint(const X509CertificateT *pCert, const char *pMessage)
{
    char strCertIdent[1024];
    #if DEBUG_VAL_CERT
    char strTimeFrom[32], strTimeTill[32];
    char strMaxHeight[32];
    #endif
    if (pMessage != NULL)
    {
        NetPrintf(("protossl: %s\n", pMessage));
    }
    NetPrintf(("protossl:  issuer: %s\n", _CertificateDebugFormatIdent(&pCert->Issuer, strCertIdent, sizeof(strCertIdent))));
    NetPrintf(("protossl: subject: %s\n", _CertificateDebugFormatIdent(&pCert->Subject, strCertIdent, sizeof(strCertIdent))));
    #if DEBUG_VAL_CERT
    NetPrintf(("protossl:   valid: from %s to %s\n",
            ds_secstostr(pCert->uGoodFrom, TIMETOSTRING_CONVERSION_RFC_0822, FALSE, strTimeFrom, sizeof(strTimeFrom)),
            ds_secstostr(pCert->uGoodTill, TIMETOSTRING_CONVERSION_RFC_0822, FALSE, strTimeTill, sizeof(strTimeTill))));
    NetPrintf(("protossl: keytype: %d\n", pCert->iKeyType));
    NetPrintf(("protossl: keyexp size: %d\n", pCert->iKeyExpSize));
    NetPrintf(("protossl: keymod size: %d\n", pCert->iKeyModSize));
    NetPrintf(("protossl: sigtype: %d\n", pCert->iSigType));
    NetPrintf(("protossl: sigsize: %d\n", pCert->iSigSize));
    ds_snzprintf(strMaxHeight, sizeof(strMaxHeight), "%d", pCert->iMaxHeight - 1);
    NetPrintf(("protossl: CertIsCA(%d): pathLenConstraints(%s)\n", pCert->iCertIsCA, (pCert->iMaxHeight == 0) ? "unlimited" : strMaxHeight));
    #endif
}
#else
#define _CertificateDebugPrint(__pCert, __pMessage)
#endif

/*F********************************************************************************/
/*!
    \Function _CertificateSetFailureInfo

    \Description
        Set the certificate info (used on failure)

    \Input *pState      - module state (may be NULL)
    \Input *pCert       - pointer to certificate to fill in from header data (may be NULL)
    \Input bIssuer      - if TRUE copy issuer, else subject

    \Version 01/24/2012 (szhu)
*/
/********************************************************************************F*/
static void _CertificateSetFailureInfo(ProtoSSLRefT *pState, X509CertificateT *pCert, uint8_t bIssuer)
{
    if ((pState != NULL) && (pCert != NULL) && (pState->bCertInfoSet == FALSE))
    {
        const ProtoSSLCertIdentT *pCertIdent = bIssuer ? &pCert->Issuer : &pCert->Subject;
        ds_memcpy_s(&pState->CertInfo.Ident, sizeof(pState->CertInfo.Ident), pCertIdent, sizeof(*pCertIdent));
        pState->CertInfo.iKeyModSize = pCert->iSigSize; // CACert::KeyModSize == Cert::SigSize
        pState->bCertInfoSet = TRUE;
    }
}

/*F********************************************************************************/
/*!
    \Function _CertificateCompareIdent

    \Description
        Compare two identities

    \Input *pIdent1     - pointer to ProtoSSLCertIdentT structure
    \Input *pIdent2     - pointer to ProtoSSLCertIdentT structure
    \Input bMatchUnit   - if TRUE compare the Unit string

    \Output
        int32_t         - zero=match, non-zero=no-match

    \Notes
        Unit is considered an optional match criteria by openssl.

    \Version 03/11/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CertificateCompareIdent(const ProtoSSLCertIdentT *pIdent1, const ProtoSSLCertIdentT *pIdent2, uint8_t bMatchUnit)
{
    int32_t iResult;

    #if DEBUG_VAL_CERT
    char strIdent1[256], strIdent2[256];
    _CertificateDebugFormatIdent(pIdent1, strIdent1, sizeof(strIdent1));
    _CertificateDebugFormatIdent(pIdent2, strIdent2, sizeof(strIdent2));
    NetPrintf(("protossl: comparing '%s' to '%s'\n", strIdent2, strIdent1));
    #endif

    iResult  = strcmp(pIdent1->strCountry, pIdent2->strCountry) != 0;
    iResult += strcmp(pIdent1->strState, pIdent2->strState) != 0;
    iResult += strcmp(pIdent1->strCity, pIdent2->strCity) != 0;
    iResult += strcmp(pIdent1->strOrg, pIdent2->strOrg) != 0;
    iResult += strcmp(pIdent1->strCommon, pIdent2->strCommon) != 0;
    if (bMatchUnit)
    {
        iResult += strcmp(pIdent1->strUnit, pIdent2->strUnit) != 0;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _CertificateMatchHostname

    \Description
        Perform wildcard case-insensitive string comparison of given input
        strings.  This implementation assumes the first string does not
        include a wildcard character and the second string includes exactly
        zero or one wildcard characters, which must be an asterisk.  The
        wildcard is intentionally limited as per the notes below to only
        match a single domain fragment.

    \Input *pString1    - first string to compare
    \Input *pString2    - second string to compare

    \Output
        int32_t         - like strcmp()

    \Notes
       As specified per RFC 2818:

       \verbatim
       Matching is performed using the matching rules specified by
       [RFC2459].  If more than one identity of a given type is present in
       the certificate (e.g., more than one dNSName name, a match in any one
       of the set is considered acceptable.) Names may contain the wildcard
       character * which is considered to match any single domain name
       component or component fragment. E.g., *.a.com matches foo.a.com but
       not bar.foo.a.com. f*.com matches foo.com but not bar.com.
       \endverbatim

    \Version 03/19/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CertificateMatchHostname(const char *pString1, const char *pString2)
{
    int32_t r;
    char c1, c2;

    do {
        c1 = *pString1++;
        if ((c1 >= 'A') && (c1 <= 'Z'))
            c1 ^= 32;
        c2 = *pString2;
        if ((c2 >= 'A') && (c2 <= 'Z'))
            c2 ^= 32;
        if ((c2 == '*') && (c1 != '.') && (c1 != '\0'))
        {
            r = _CertificateMatchHostname(pString1, pString2+1);
            if (r == 0)
            {
                break;
            }
            r = 0;
        }
        else
        {
            pString2 += 1;
            r = c1-c2;
        }
    } while ((c1 != 0) && (c2 != 0) && (r == 0));

    return(r);
}

/*F********************************************************************************/
/*!
    \Function _CertificateMatchSubjectAlternativeName

    \Description
        Takes as input a hostname and Subject Alternative Name object,
        returns zero if the hostname matches a SAN object entry and non-zero
        otherwise.

    \Input *pStrHost    - pointer to hostname to compare against
    \Input *pSubject    - pointer to subject alternative object
    \Input iSubjectAltLen - length of subject alternative object

    \Output
        int32_t         - like strcmp()

    \Version 11/01/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CertificateMatchSubjectAlternativeName(const char *pStrHost, const uint8_t *pSubject, int32_t iSubjectAltLen)
{
    const uint8_t *pSubjectEnd = pSubject + iSubjectAltLen;
    char strCompare[256], *pStrCompare;
    int32_t iType, iSize;

    // parse the wrapper object
    pSubject = _AsnParseHeader(pSubject, pSubjectEnd, &iType, &iSize);

    // check against subject alternative strings
    for (pSubjectEnd = pSubject + iSubjectAltLen; pSubject != NULL; pSubject += iSize)
    {
        // parse the object
        pSubject = _AsnParseHeader(pSubject, pSubjectEnd, &iType, &iSize);

        // if it's not a dnsname object, skip it
        if (iType != ASN_IMPLICIT_TAG+2)
        {
            continue;
        }

        // make a copy of the comparison string
        ds_strsubzcpy(strCompare, sizeof(strCompare), (const char *)pSubject, iSize);

        // skip leading whitespace, if any
        for (pStrCompare = strCompare; ((*pStrCompare > 0) && (*pStrCompare <= ' ')); pStrCompare += 1)
            ;

        // do the name compare
        if (_CertificateMatchHostname(pStrHost, pStrCompare) == 0)
        {
            return(0);
        }
    }

    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _CertificateGetSigAlg

    \Description
        Get signature algorithm to use based on certificate key type

    \Input *pCertificate        - certificate to get signature algorithm for

    \Output
        uint32_t uCertSigAlg    - signature algorithm

    \Version 05/12/2017 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _CertificateGetSigAlg(const CertificateDataT *pCertificate)
{
    uint32_t uCertSigAlg = SSL3_SIGALG_NONE;
    if (pCertificate != NULL)
    {
        uCertSigAlg = (pCertificate->iKeyType == ASN_OBJ_ECDSA_KEY) ? SSL3_SIGALG_ECDSA : SSL3_SIGALG_RSA;
    }
    return(uCertSigAlg);
}

/*
    pkcs1 verification and generation
*/

/*F********************************************************************************/
/*!
    \Function _Pkcs1VerifyRSAES

    \Description
        Validate RSAES-PKCS1-v1_5 padding as defined by
        https://tools.ietf.org/html/rfc8017#section-7.2.

    \Input *pData       - data to validate
    \Input iKeySize     - key modulus size, in bytes
    \Input iSecretSize  - premaster secret size, in bytes

    \Output
        int32_t         - zero=invalid, else valid

    \Version 02/18/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _Pkcs1VerifyRSAES(const uint8_t *pData, int32_t iKeySize, int32_t iSecretSize)
{
    int32_t iIndex;
    uint8_t bValid = TRUE;

    // validate signature
    if ((pData[0] != 0) || (pData[1] != 2))
    {
        NetPrintf(("protossl: pkcs1 signature invalid\n"));
        bValid = FALSE;
    }
    // validate random padding; must be non-zero
    for (iIndex = 2; iIndex < iKeySize-iSecretSize-1; iIndex += 1)
    {
        if (pData[iIndex] == 0)
        {
            NetPrintf(("protossl: pkcs1 padding validation found zero byte\n"));
            bValid = FALSE;
            break;
        }
    }
    // validate zero byte before premaster key data
    if (pData[iKeySize-iSecretSize-1] != 0)
    {
        NetPrintf(("protossl: pkcs1 premaster key padding not zero\n"));
        bValid = FALSE;
    }
    // return result
    return(bValid);
}

/*F********************************************************************************/
/*!
    \Function _Pkcs1VerifyEMSA

    \Description
        As per https://tools.ietf.org/html/rfc5246#section-4.7 validate
        EMSA-PKCS1-v1_5 padding, and return pointer to signature hash if valid.
        Format defined by https://tools.ietf.org/html/rfc8017#section-9.2.

    \Input *pData       - data to validate
    \Input iSigSize     - signature size, in bytes
    \Input iHashSize    - signature hash size, in bytes
    \Input eHashType    - signature hash type (if CRYPTHASH_NULL, don't validate hash object)

    \Output
        uint8_t *       - pointer to signature data, or NULL if invalid

    \Version 04/27/2017 (jbrookes)
*/
/********************************************************************************F*/
static const uint8_t *_Pkcs1VerifyEMSA(const uint8_t *pData, int32_t iSigSize, int32_t iHashSize, CryptHashTypeE eHashType)
{
    uint8_t aHashObj[CRYPTHASH_MAXDIGEST+64];
    int32_t iIndex, iSize;

    // validate EMSA signature
    if ((pData[0] != 0) || (pData[1] != 1))
    {
        return(NULL);
    }
    // validate padding; must be 0xff
    for (iIndex = 2; (iIndex < (iSigSize-iHashSize)) && (pData[iIndex] == 0xff); iIndex += 1)
        ;
    // validate zero byte immediately preceding signature hash object
    if (pData[iIndex++] != 0)
    {
        return(NULL);
    }

    // validate the digital hash object that wraps the signature hash
    if (eHashType != CRYPTHASH_NULL)
    {
        // generate digitahash object we expect, and validate we got it
        if (((iSize = _AsnWriteDigitalHashObject(aHashObj, sizeof(aHashObj), NULL, iHashSize, eHashType)) == 0) || (memcmp(aHashObj, pData + iIndex, iSize)))
        {
            return(NULL);
        }
        iIndex += iSize;
    }

    // make sure we validated the entire signature, minus the hash itself
    if ((iIndex+iHashSize) != iSigSize)
    {
        return(NULL);
    }

    // return pointer to signature hash
    return(pData+iIndex);
}

/*F********************************************************************************/
/*!
    \Function _Pkcs1DoMGF1

    \Description
        Implements MFG1 as per https://tools.ietf.org/html/rfc8017#appendix-B.2.1 

    \Input *pOutput     - [out] output of mgf
    \Input iOutputLen   - length of output buf
    \Input *pInput      - mgf input
    \Input iInputLen    - length of mgf input
    \Input eHashType    - mgf hash type

    \Version 05/11/2017 (jbrookes)
*/
/********************************************************************************F*/
static void _Pkcs1DoMGF1(uint8_t *pOutput, int32_t iOutputLen, const uint8_t *pInput, int32_t iInputLen, CryptHashTypeE eHashType)
{
    uint8_t aHashState[CRYPTHASH_MAXSTATE], aCounter[4];
    const CryptHashT *pHash;
    int32_t iHashLen;
    uint32_t uRound;

    // get hash object and length
    if ((pHash = CryptHashGet(eHashType)) == NULL)
    {
        return;
    }
    iHashLen = CryptHashGetSize(eHashType);

    // do the hashing
    for (uRound = 0; iOutputLen > 0; uRound += 1, pOutput += iHashLen, iOutputLen -= iHashLen)
    {
        aCounter[0] = (uint8_t)(uRound >> 24);
        aCounter[1] = (uint8_t)(uRound >> 16);
        aCounter[2] = (uint8_t)(uRound >> 8);
        aCounter[3] = (uint8_t)(uRound);

        pHash->Init(aHashState, iHashLen);
        pHash->Update(aHashState, pInput, iHashLen);
        pHash->Update(aHashState, aCounter, sizeof(aCounter));
        pHash->Final(aHashState, pOutput, iHashLen);
    }
}

/*F********************************************************************************/
/*!
    \Function _Pkcs1VerifyEMSA_PSS

    \Description
        As per https://tools.ietf.org/html/rfc8017#section-9.1.2 validate
        EMSA-PSS envelope, and compare against the specified hash value.

    \Input *pSigData    - signature data
    \Input iSigSize     - signature size, in bytes
    \Input eSigHash     - signature hash
    \Input iSaltSize    - salt size, in bytes
    \Input *pMsgHash    - message hash data
    \Input iHashSize    - signature hash size, in bytes
    \Input eMgfHash     - mgf hash type

    \Output
        uint32_t        - zero=verify failure, else success

    \Notes
        This function deliberately uses coding standards and comments matching
        the specification to make it easier to compare while reading the specs.

        The maximum salt length supported is 128 bytes (2*CRYPTHASH_MAXDIGEST)

    \Version 05/11/2017 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _Pkcs1VerifyEMSA_PSS(const uint8_t *pSigData, int32_t iSigSize, CryptHashTypeE eSigHash, int32_t iSaltSize, const uint8_t *pMsgHash, int32_t iHashSize, CryptHashTypeE eMgfHash)
{
    uint8_t maskedDB[SSL_SIG_MAX], dbMask[SSL_SIG_MAX], DB[SSL_SIG_MAX], H[CRYPTHASH_MAXDIGEST], salt[CRYPTHASH_MAXDIGEST*2];
    const CryptHashT *pHash = CryptHashGet(eSigHash);
    uint8_t aHashState[CRYPTHASH_MAXSTATE];
    uint8_t Mprime[CRYPTHASH_MAXDIGEST*3 + 8], Hprime[CRYPTHASH_MAXDIGEST];
    int32_t iDB, iDBLen;
    uint8_t uCheck;

    // if the rightmost octet of EM does not have hexadecimal value 0xbc, output "inconsistent" and stop.
    if (pSigData[iSigSize-1] != 0xBC)
    {
        NetPrintf(("protossl: signature failed identity check\n"));
        return(0);
    }

    // let maskedDB be the leftmost emLen - hLen - 1 octets of EM, and let H be the next hLen octets.
    iDBLen = iSigSize - iHashSize - 1;
    ds_memcpy_s(maskedDB, sizeof(maskedDB), pSigData, iDBLen);
    ds_memcpy_s(H, sizeof(H), pSigData + iDBLen, iHashSize);

    /* if the leftmost 8emLen - emBits bits of the leftmost octet in maskedDB are not all equal to zero, output "inconsistent" and stop.
       note that we assume our modulus size here is a multiple of eight bits */
    if (maskedDB[0] & 0x80)
    {
        NetPrintf(("protossl: signature failed masked validity check\n"));
        return(0);
    }

    // let dbMask = MGF(H, emLen - hLen - 1).
    _Pkcs1DoMGF1(dbMask, iDBLen, H, iDBLen, eSigHash);

    // let DB = maskedDB \xor dbMask.
    for (iDB = 0; iDB < iDBLen; iDB += 1)
    {
        DB[iDB] = maskedDB[iDB] ^ dbMask[iDB];
    }

    /* set the leftmost 8emLen - emBits bits of the leftmost octet in DB to zero.
       note that we assume our modulus size here is a multiple of eight bits */
    DB[0] &= 0x7f;

    #if DEBUG_RAW_DATA
    NetPrintMem(DB, iDBLen, "certificate verify unmasked data");
    #endif

    /* if the emLen - hLen - sLen - 2 leftmost octets of DB are not zero or if the octet at position emLen - hLen - sLen - 1
       (the leftmost position is "position 1") does not have hexadecimal value 0x01, output "inconsistent" and stop. */
    for (iDB = 0, uCheck = 0; iDB < (iDBLen - iSaltSize - 1); iDB += 1)
        uCheck |= DB[iDB];
    if ((uCheck != 0) || (DB[iDB] != 1))
    {
        NetPrintf(("protossl: signature failed padding validity check\n"));
        return(0);
    }

    // validate salt length
    if (iSaltSize > (signed)sizeof(salt))
    {
        NetPrintf(("protossl: salt length %d too big in signature verify\n", iSaltSize));
        return(0);
    }
    // let salt be the last sLen octets of DB.
    ds_memcpy_s(salt, sizeof(salt), DB + iDBLen - iSaltSize, iSaltSize);

    // let M' = (0x)00 00 00 00 00 00 00 00 || mHash || salt
    ds_memclr(Mprime, 8);
    ds_memcpy(Mprime + 8, pMsgHash, iHashSize);
    ds_memcpy(Mprime + 8 + iHashSize, salt, iSaltSize);

    // let H' = Hash(M'), an octet string of length hLen.
    pHash->Init(aHashState, iHashSize);
    pHash->Update(aHashState, Mprime, 8 + iHashSize + iSaltSize);
    pHash->Final(aHashState, Hprime, iHashSize);

    #if DEBUG_RAW_DATA
    NetPrintMem(H, iHashSize, "H");
    NetPrintMem(Hprime, iHashSize, "Hprime");
    NetPrintMem(pMsgHash, iHashSize, "MsgHash");
    NetPrintMem(salt, iSaltSize, "salt");
    #endif

    // if H = H', output "consistent".  Otherwise, output "inconsistent".
    if (memcmp(H, Hprime, iHashSize))
    {
        NetPrintf(("protossl: signature failed hash validity check\n"));
        return(0);
    }

    // success
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _Pkcs1EncodeEMSA_PSS

    \Description
        As per https://tools.ietf.org/html/rfc8017#section-9.1.1 encode
        specified hash data in an EMSA-PSS envelope.

    \Input *pBuffer     - [out] storage for encoded message
    \Input iBufSize     - size of output buffer
    \Input *pHashData   - message hash
    \Input iHashSize    - size of hash
    \Input eHashType    - hash type

    \Output
        int32_t         - size of encoded message, or negative on error

    \Notes
        This function deliberately uses coding standards and comments matching
        the specification to make it easier to compare while reading the specs.

    \Version 05/16/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _Pkcs1EncodeEMSA_PSS(uint8_t *pBuffer, int32_t iBufSize, const uint8_t *pHashData, int32_t iHashSize, CryptHashTypeE eHashType)
{
    uint8_t maskedDB[SSL_SIG_MAX], dbMask[SSL_SIG_MAX], DB[SSL_SIG_MAX];
    uint8_t aSaltBuf[CRYPTHASH_MAXDIGEST], Mprime[(CRYPTHASH_MAXDIGEST*2)+8], H[(CRYPTHASH_MAXDIGEST*2)+8], *pTemp;
    const CryptHashT *pHash = CryptHashGet(eHashType);
    uint8_t aHashState[CRYPTHASH_MAXSTATE];
    int32_t iDB, iDBLen = iBufSize - iHashSize - 1;

    // if emLen < hLen + sLen + 2, output "encoding error" and stop.
    if (iBufSize < ((iHashSize*2)+2))
    {
        return(-1);
    }
    // generate a random octet string salt of length sLen; if sLen = 0, then salt is the empty string.
    CryptRandGet(aSaltBuf, sizeof(aSaltBuf));
    // let M' = (0x)00 00 00 00 00 00 00 00 || mHash || salt
    pTemp = Mprime;
    ds_memclr(pTemp, 8);
    pTemp += 8;
    ds_memcpy(pTemp, pHashData, iHashSize);
    pTemp += iHashSize;
    ds_memcpy(pTemp, aSaltBuf, iHashSize);

    // let H = Hash(M'), an octet string of length hLen.
    pHash->Init(aHashState, iHashSize);
    pHash->Update(aHashState, Mprime, 8 + iHashSize + iHashSize);
    pHash->Final(aHashState, H, iHashSize);

    #if DEBUG_RAW_DATA
    NetPrintMem(H, iHashSize, "H");
    NetPrintMem(pHashData, iHashSize, "MsgHash");
    NetPrintMem(aSaltBuf, iHashSize, "salt");
    #endif

    // generate an octet string PS consisting of emLen - sLen - hLen - 2 zero octets.  The length of PS may be 0.
    ds_memclr(DB, iBufSize - (iHashSize*2) - 2);
    pTemp = DB + iBufSize - (iHashSize*2) - 2;

    // let DB = PS || 0x01 || salt; DB is an octet string of length emLen - hLen - 1.
    *pTemp++ = 0x01;
    ds_memcpy(pTemp, aSaltBuf, iHashSize);

    // let dbMask = MGF(H, emLen - hLen - 1).
    _Pkcs1DoMGF1(dbMask, iDBLen, H, iDBLen, eHashType);

    // let maskedDB = DB \xor dbMask.
    for (iDB = 0; iDB < iDBLen; iDB += 1)
    {
        maskedDB[iDB] = DB[iDB] ^ dbMask[iDB];
    }

    /* set the leftmost 8emLen - emBits bits of the leftmost octet in maskedDB to zero.
       note that we assume our modulus size here is a multiple of eight bits */
    maskedDB[0] &= 0x7f;     

    // let EM = maskedDB || H || 0xbc.
    pTemp = pBuffer;
    ds_memcpy(pTemp, maskedDB, iDBLen);
    pTemp += iDBLen;
    ds_memcpy(pTemp, H, iHashSize);
    pTemp += iHashSize;
    *pTemp++ = 0xbc;

    // output EM
    return(pTemp-pBuffer);
}

/*
    session history
*/

/*F********************************************************************************/
/*!
    \Function _SessionHistorySet

    \Description
        Set entry in session history buffer

    \Input *pSecure     - secure state
    \Input *pSessHist   - session history entry to set
    \Input *pSessTick   - session ticket for session (if tls1.3; else NULL)
    \Input *pHostName   - hostname session is associated with
    \Input uPort        - port session is associated with
    \Input *pSessionId  - session id of session
    \Input *pMasterSecret- master secret associated with session
    \Input uCurTick     - current tick count

    \Version 03/15/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _SessionHistorySet(SecureStateT *pSecure, SessionHistoryT *pSessHist, SessionTicketT *pSessTick, const char *pHostName, uint16_t uPort, const uint8_t *pSessionId, const uint8_t *pMasterSecret, uint32_t uCurTick)
{
    // save hostname & port
    ds_strnzcpy(pSessHist->strHost, pHostName, sizeof(pSessHist->strHost));
    pSessHist->uPort = uPort;
    // set access tick
    pSessHist->uSessionUseTick = uCurTick;
    // set expire tick; add a 250ms fudge factor to make sure we don't send an expiring ticket
    pSessHist->uSessionExpTick = (pSessTick != NULL) ? pSessTick->uLifetime * 1000 : SSL_SESSVALIDTIME_MAX;
    pSessHist->uSessionExpTick += uCurTick-250;

    if (pSessTick != NULL)
    {
        pSessHist->bSessionTicket = TRUE;
        ds_memcpy_s(&pSessHist->SessionTicket, sizeof(pSessHist->SessionTicket), pSessTick, sizeof(*pSessTick));
    }
    else
    {
        SessionInfoT *pSessInfo = &pSessHist->SessionInfo;
        pSessHist->bSessionTicket = FALSE;
        pSessInfo->uSslVersion = pSecure->uSslVersion;
        pSessInfo->uCipherId = pSecure->pCipher->uIdent;
        ds_memcpy(pSessInfo->SessionId, pSessionId, sizeof(pSessInfo->SessionId));
        ds_memcpy(pSessInfo->MasterSecret, pMasterSecret, sizeof(pSessInfo->MasterSecret));
    }
}

/*F********************************************************************************/
/*!
    \Function _SessionHistoryGet

    \Description
        Returns pointer to specified session history entry, or NULL if not found.

    \Input *pHostName   - hostname associated with session (may be null)
    \Input uPort        - port associated with session (if using hostname)
    \Input *pSessionId  - session id to look for (may be null)

    \Output
        SessionHistoryT * - session history entry, or NULL if not found

    \Version 03/15/2012 (jbrookes)
*/
/********************************************************************************F*/
static SessionHistoryT *_SessionHistoryGet(const char *pHostName, uint16_t uPort, const uint8_t *pSessionId)
{
    SessionHistoryT *pSessHist, *pSessInfo;
    ProtoSSLStateT *pState = _ProtoSSL_pState;
    const uint8_t aZeroSessionId[32] = { 0 };
    int32_t iSess;

    #if DIRTYCODE_DEBUG
    if (pState == NULL)
    {
        NetPrintf(("protossl: warning, protossl not initialized, session history feature is disabled\n"));
        return(NULL);
    }
    #endif

    // exclude NULL sessionId
    if ((pSessionId != NULL) && !memcmp(pSessionId, aZeroSessionId, sizeof(aZeroSessionId)))
    {
        return(NULL);
    }

    // search session history for a match
    for (iSess = 0, pSessInfo = NULL; (iSess < SSL_SESSHIST_MAX) && (pSessInfo == NULL); iSess += 1)
    {
        pSessHist = &pState->SessionHistory[iSess];

        // make sure the ticket isn't expired
        if (NetTickDiff(NetTick(), pSessHist->uSessionExpTick) > 0)
        {
            NetPrintfVerbose((DEBUG_RES_SESS, 0, "protossl: expiring session for %s:%d\n", pSessHist->strHost, pSessHist->uPort));
            continue;
        }

        // check for resume by hostname/port
        if ((pHostName != NULL) && (!strcmp(pSessHist->strHost, pHostName)) && (pSessHist->uPort == uPort))
        {
            pSessInfo = pSessHist;
        }
        // check for resume by sessionId (tls1.2 and prior)
        if ((pSessionId != NULL) && (!memcmp(pSessHist->SessionInfo.SessionId, pSessionId, sizeof(pSessHist->SessionInfo.SessionId))))
        {
            pSessInfo = pSessHist;
        }
    }

    // return session (or null) to caller
    return(pSessInfo);
}

/*F********************************************************************************/
/*!
    \Function _SessionHistoryAdd

    \Description
        Save a new session in the session history buffer

    \Input *pSecure     - secure state
    \Input *pHostName   - hostname session is for
    \Input uPort        - port session is for
    \Input *pSessTick   - session ticket for session, if tls1.3
    \Input *pSessionId  - session id for session
    \Input iSessionLen  - length of session
    \Input *pMasterSecret - master secret associated with session

    \Version 03/15/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _SessionHistoryAdd(SecureStateT *pSecure, const char *pHostName, uint16_t uPort, SessionTicketT *pSessTick, const uint8_t *pSessionId, uint32_t iSessionLen, const uint8_t *pMasterSecret)
{
    ProtoSSLStateT *pState = _ProtoSSL_pState;
    SessionHistoryT *pSessHist;
    int32_t iSess, iSessOldest;
    int32_t iTickDiff, iTickDiffMax;
    uint32_t uCurTick = NetTick();

    #if DIRTYCODE_DEBUG
    if (pState == NULL)
    {
        NetPrintf(("protossl: warning, protossl not initialized, session history feature is disabled\n"));
        return;
    }
    #endif

    NetCritEnter(&pState->StateCrit);

    // see if we already have a session for this peer
    for (iSess = 0; iSess < SSL_SESSHIST_MAX; iSess += 1)
    {
        pSessHist = &pState->SessionHistory[iSess];
        // already have this peer in our history?
        if (!strcmp(pSessHist->strHost, pHostName) && (pSessHist->uPort == uPort))
        {
            NetPrintfVerbose((DEBUG_RES_SESS, 0, "protossl: updating session history entry %d (host=%s:%d)\n", iSess, pHostName, uPort));
            _SessionHistorySet(pSecure, pSessHist, pSessTick, pHostName, uPort, pSessionId, pMasterSecret, uCurTick);
            NetCritLeave(&pState->StateCrit);
            return;
        }
    }

    // try to find an unallocated session
    for (iSess = 0; iSess < SSL_SESSHIST_MAX; iSess += 1)
    {
        pSessHist = &pState->SessionHistory[iSess];
        // empty slot?
        if (pSessHist->strHost[0] == '\0')
        {
            NetPrintfVerbose((DEBUG_RES_SESS, 0, "protossl: adding session history entry %d (host=%s:%d)\n", iSess, pHostName, uPort));
            _SessionHistorySet(pSecure, pSessHist, pSessTick, pHostName, uPort, pSessionId, pMasterSecret, uCurTick);
            NetCritLeave(&pState->StateCrit);
            return;
        }
    }

    // find the oldest session
    for (iSess = 0, iTickDiffMax = 0, iSessOldest = 0; iSess < SSL_SESSHIST_MAX; iSess += 1)
    {
        pSessHist = &pState->SessionHistory[iSess];
        // find least recently updated session
        if ((iTickDiff = NetTickDiff(uCurTick, pSessHist->uSessionUseTick)) > iTickDiffMax)
        {
            iTickDiffMax = iTickDiff;
            iSessOldest = iSess;
        }
    }

    NetPrintfVerbose((DEBUG_RES_SESS, 0, "protossl: replacing session history entry %d (host=%s:%d)\n", iSessOldest, pHostName, uPort));
    _SessionHistorySet(pSecure, &pState->SessionHistory[iSessOldest], pSessTick, pHostName, uPort, pSessionId, pMasterSecret, uCurTick);

    NetCritLeave(&pState->StateCrit);
}

/*F********************************************************************************/
/*!
    \Function _SessionHistoryInvalidate

    \Description
        Invalidate specified session from session history cache

    \Input *pHostName   - hostname of session to invalidate
    \Input uPort        - port of session to invalidate

    \Version 02/04/2014 (jbrookes)
*/
/********************************************************************************F*/
static void _SessionHistoryInvalidate(const char *pHostName, uint16_t uPort)
{
    SessionHistoryT *pSessInfo = NULL;
    ProtoSSLStateT *pState = _ProtoSSL_pState;

    #if DIRTYCODE_DEBUG
    if (pState == NULL)
    {
        NetPrintf(("protossl: warning, protossl not initialized, session history feature is disabled\n"));
        return;
    }
    #endif

    // acquire access to session history
    NetCritEnter(&pState->StateCrit);

    // find session history
    if ((pSessInfo = _SessionHistoryGet(pHostName, uPort, NULL)) != NULL)
    {
        // reset entry
        NetPrintf(("protossl: invalidating session entry\n"));
        ds_memclr(pSessInfo, sizeof(*pSessInfo));
    }

    // release access to session history, return to caller
    NetCritLeave(&pState->StateCrit);
}

/*F********************************************************************************/
/*!
    \Function _SessionHistoryGetInfo

    \Description
        Check to see if we can find a session for the specified address or session
        id in our history buffer, and return sesession info if found.

    \Input *pSessBuff   - [out] storage for session history entry
    \Input *pHostName   - hostname of session (may be null)
    \Input uPort        - port of session (if hostname is not null)
    \Input *pSessionId  - session id to look for (may be null)

    \Output
        SessionHistoryT * - session history entry, or NULL if not found

    \Version 03/15/2012 (jbrookes)
*/
/********************************************************************************F*/
static SessionHistoryT *_SessionHistoryGetInfo(SessionHistoryT *pSessBuff, const char *pHostName, uint16_t uPort, const uint8_t *pSessionId)
{
    SessionHistoryT *pSessInfo = NULL;
    ProtoSSLStateT *pState = _ProtoSSL_pState;

    #if DIRTYCODE_DEBUG
    if (pState == NULL)
    {
        NetPrintf(("protossl: warning, protossl not initialized, session history feature is disabled\n"));
        return(NULL);
    }
    #endif

    // acquire access to session history
    NetCritEnter(&pState->StateCrit);

    // find session history
    if ((pSessInfo = _SessionHistoryGet(pHostName, uPort, pSessionId)) != NULL)
    {
        // update timestamp
        pSessInfo->uSessionUseTick = NetTick();
        // copy to caller
        ds_memcpy(pSessBuff, pSessInfo, sizeof(*pSessBuff));
        pSessInfo = pSessBuff;
    }

    // release access to session history, return to caller
    NetCritLeave(&pState->StateCrit);
    return(pSessInfo);
}

/*
    certificate validation vistory
*/

/*F********************************************************************************/
/*!
    \Function _CertValidHistorySet

    \Description
        Set entry in certvalid history buffer

    \Input *pCertHist   - session history entry to set
    \Input *pFingerprintId - fingerprint for certificates validated
    \Input uCertSize    - certificate size
    \Input uCurTick     - current tick count

    \Version 04/29/2014 (jbrookes)
*/
/********************************************************************************F*/
static void _CertValidHistorySet(CertValidHistoryT *pCertHist, const uint8_t *pFingerprintId, uint32_t uCertSize, uint32_t uCurTick)
{
    // update fingerprint
    ds_memcpy(pCertHist->FingerprintId, pFingerprintId, sizeof(pCertHist->FingerprintId));
    // save cert size
    pCertHist->uCertSize = uCertSize;
    // set timestamp
    pCertHist->uCertValidTick = uCurTick;
}

/*F********************************************************************************/
/*!
    \Function _CertValidHistoryGet

    \Description
        Returns pointer to specified session history entry, or NULL if not found.

    \Input *pFingerprintId - certificate fingerprint
    \Input uCertSize    - size of certificate to check in validity history

    \Output
        CertValidHistoryT * - certificate validation history entry, or NULL if not found

    \Version 04/29/2014 (jbrookes)
*/
/********************************************************************************F*/
static CertValidHistoryT *_CertValidHistoryGet(const uint8_t *pFingerprintId, uint32_t uCertSize)
{
    CertValidHistoryT *pCertHist, *pCertInfo;
    ProtoSSLStateT *pState = _ProtoSSL_pState;
    const uint8_t aZeroFingerprintId[SSL_FINGERPRINT_SIZE] = { 0 };
    uint32_t uCurTick = NetTick();
    int32_t iCert;

    #if DIRTYCODE_DEBUG
    if (pState == NULL)
    {
        NetPrintf(("protossl: warning, protossl not initialized, certificate validation history feature is disabled\n"));
        return(NULL);
    }
    #endif

    // exclude NULL fingerprintId request
    if ((pFingerprintId == NULL) || !memcmp(pFingerprintId, aZeroFingerprintId, sizeof(aZeroFingerprintId)))
    {
        return(NULL);
    }

    // find certificate history
    for (iCert = 0, pCertInfo = NULL; iCert < SSL_CERTVALID_MAX; iCert += 1)
    {
        // ref history entry
        pCertHist = &pState->CertValidHistory[iCert];
        // skip null entries
        if (pCertHist->uCertSize == 0)
        {
            continue;
        }
        // check for expiration
        if (NetTickDiff(uCurTick, pCertHist->uCertValidTick) > SSL_CERTVALIDTIME_MAX)
        {
            NetPrintfVerbose((DEBUG_VAL_CERT, 0, "protossl: expiring certificate validity for entry %d\n", iCert));
            ds_memclr(pCertHist, sizeof(*pCertHist));
            continue;
        }
        // check for match
        if ((pCertHist->uCertSize == uCertSize) && (!memcmp(pCertHist->FingerprintId, pFingerprintId, sizeof(pCertHist->FingerprintId))))
        {
            pCertInfo = pCertHist;
            break;
        }
    }

    // return session (or null) to caller
    return(pCertInfo);
}

/*F********************************************************************************/
/*!
    \Function _CertValidHistoryAdd

    \Description
        Added fingerprint of validated certificate to certificate validity history buffer

    \Input *pFingerprintId  - certificate fingerprint
    \Input uCertSize        - size of certificate

    \Version 03/15/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _CertValidHistoryAdd(const uint8_t *pFingerprintId, uint32_t uCertSize)
{
    ProtoSSLStateT *pState = _ProtoSSL_pState;
    CertValidHistoryT *pCertHist;
    int32_t iCert, iCertOldest;
    int32_t iTickDiff, iTickDiffMax;
    uint32_t uCurTick = NetTick();

    #if DIRTYCODE_DEBUG
    if (pState == NULL)
    {
        NetPrintf(("protossl: warning, protossl not initialized, certificate validation history feature is disabled\n"));
        return;
    }
    #endif

    NetCritEnter(&pState->StateCrit);

    // try to find an unallocated session
    for (iCert = 0; iCert < SSL_CERTVALID_MAX; iCert += 1)
    {
        pCertHist = &pState->CertValidHistory[iCert];
        // empty slot?
        if (pCertHist->uCertSize == 0)
        {
            #if DEBUG_VAL_CERT
            NetPrintf(("protossl: adding certificate validation history entry %d\n", iCert));
            #endif
            _CertValidHistorySet(pCertHist, pFingerprintId, uCertSize, uCurTick);
            NetCritLeave(&pState->StateCrit);
            return;
        }
    }

    // find the oldest session
    for (iCert = 0, iTickDiffMax = 0, iCertOldest = 0; iCert < SSL_CERTVALID_MAX; iCert += 1)
    {
        pCertHist = &pState->CertValidHistory[iCert];
        // find least recently updated session
        if ((iTickDiff = NetTickDiff(uCurTick, pCertHist->uCertValidTick)) > iTickDiffMax)
        {
            iTickDiffMax = iTickDiff;
            iCertOldest = iCert;
        }
    }

    NetPrintfVerbose((DEBUG_VAL_CERT, 0, "protossl: replacing certificate validation entry %d\n", iCertOldest));
    _CertValidHistorySet(&pState->CertValidHistory[iCertOldest], pFingerprintId, uCertSize, uCurTick);

    NetCritLeave(&pState->StateCrit);
}

/*F********************************************************************************/
/*!
    \Function _CertValidHistoryCheck

    \Description
        Check to see if certificate fingerprint is in certificiate validity history

    \Input *pFingerprintId  - certificate fingerprint
    \Input uCertSize        - size of certificate

    \Output
        int32_t             - zero=not validated, else validated

    \Version 04/29/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CertValidHistoryCheck(const uint8_t *pFingerprintId, uint32_t uCertSize)
{
    ProtoSSLStateT *pState = _ProtoSSL_pState;
    CertValidHistoryT *pCertInfo;
    int32_t iResult = 0;

    #if DIRTYCODE_DEBUG
    if (pState == NULL)
    {
        NetPrintf(("protossl: warning, protossl not initialized, certificate validation history feature is disabled\n"));
        return(0);
    }
    #endif

    // acquire access to session history
    NetCritEnter(&pState->StateCrit);

    // find certificate validity history
    if ((pCertInfo = _CertValidHistoryGet(pFingerprintId, uCertSize)) != NULL)
    {
        iResult = 1;
        NetPrintfVerbose((DEBUG_VAL_CERT, 0, "protossl: certificate validation deferred to cache\n"));
    }

    // release access to session history, return to caller
    NetCritLeave(&pState->StateCrit);
    return(iResult);
}

/*
    digital signature generation and verification
*/

/*F********************************************************************************/
/*!
    \Function _ProtoSSLEccInitContext

    \Description
        Initialize ECC context for key exchange and retrieve the dh interface

    \Input *pSecure             - secure state
    \Input *pEllipticCurve      - curve to init state with

    \Output
        const CryptCurveDhT *   - the found dh interface or NULL if not found for curve

    \Version 03/14/2018 (jbrookes)
*/
/********************************************************************************F*/
static const CryptCurveDhT *_ProtoSSLEccInitContext(SecureStateT *pSecure, const EllipticCurveT *pEllipticCurve)
{
    const CryptCurveDhT *pEcc;

    // find the dh interface, this _should not_ fail if everything is correctly configured
    if ((pEcc = CryptCurveGetDh(pEllipticCurve->uIdent)) == NULL)
    {
        NetPrintf(("protossl: could not find curve %s (ident=0x%04x), mismatch between protossl and cryptcurve?\n",
            pEllipticCurve->strName, pEllipticCurve->uIdent));
        return(NULL);
    }
    if (pSecure->bEccContextInitialized)
    {
        return(pEcc);
    }
    // init the state
    if (pEcc->Init(pSecure->EccContext, pEllipticCurve->uIdent) == 0)
    {
        pSecure->bEccContextInitialized = TRUE;
    }
    else
    {
        NetPrintf(("protossl: initialize failed for curve %s (ident=0x%04x)\n", pEllipticCurve->strName, pEllipticCurve->uIdent));
    }
    return(pEcc);
}

/*F********************************************************************************/
/*
    \Function _ProtoSSLDsaInitContext

    \Description
        Gets the interface for the ecdsa curve that we use for signing
        and verifying

    \Input *pSigVerify          - the signature verify state
    \Input iCrvType             - the asn identifier for the ecdsa curve

    \Output
        const CryptCurveDsaT *  - the interface or NULL if not found

    \Version 05/10/2018 (eesponda)
*/
/********************************************************************************F*/
static const CryptCurveDsaT *_ProtoSSLDsaInitContext(SignatureVerifyT *pSigVerify, int32_t iCrvType)
{
    const CryptCurveDsaT *pDsa;
    int32_t iCurveIdent = 0;

    // map the ASN to the curve we want to use
    if (iCrvType == ASN_OBJ_ECDSA_SECP256R1)
    {
        iCurveIdent = CRYPTCURVE_SECP256R1;
    }
    else if (iCrvType == ASN_OBJ_ECDSA_SECP384R1)
    {
        iCurveIdent = CRYPTCURVE_SECP384R1;
    }

    // find the dsa interface, this _should not_ fail if everything is correctly configured
    if ((pDsa = CryptCurveGetDsa(iCurveIdent)) == NULL)
    {
        NetPrintf(("protossl: could not find curve ident 0x%04x for dsa, mismatch between protossl and cryptcurve?\n",
            iCurveIdent));
        return(NULL);
    }
    if (pSigVerify->bEccContextInitialized)
    {
        return(pDsa);
    }

    // attempt to initialize, catch failure in case we get an unsupported curve
    if (pDsa->Init(pSigVerify->DsaContext, iCurveIdent) == 0)
    {
        pSigVerify->bEccContextInitialized = TRUE;
    }
    else
    {
        NetPrintf(("protossl: initialize failed for dsa curve (ident=0x%04x)\n", iCurveIdent));
    }
    return(pDsa);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLGenerateSignatureHash

    \Description
        Generate a signature for key exchange

    \Input *pSecure     - secure state
    \Input *pHash       - signature hash algorithm
    \Input *pHashDigest - [out] buffer for signature hash
    \Input *pData       - data to sign
    \Input iDataLen     - length of data

    \Output
        int32_t         - size of signature hash

    \Version 02/17/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLGenerateSignatureHash(const SecureStateT *pSecure, const CryptHashT *pHash, uint8_t *pHashDigest, const uint8_t *pData, int32_t iDataLen)
{
    uint8_t HashState[CRYPTHASH_MAXSTATE];
    pHash->Init(HashState, pHash->iHashSize);
    pHash->Update(HashState, pSecure->ClientRandom, sizeof(pSecure->ClientRandom));
    pHash->Update(HashState, pSecure->ServerRandom, sizeof(pSecure->ServerRandom));
    pHash->Update(HashState, pData, iDataLen);
    pHash->Final(HashState, pHashDigest, pHash->iHashSize);
    return(pHash->iHashSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLGenerateSignatureEcdsa

    \Description
        Generate an ECDSA signature; expected to be called iteratively

    \Input *pSecure     - secure state
    \Input *pSigData    - [out] signature data buffer
    \Input iSigSize     - signature data size
    \Input *pPrivateKey - ECC private key data
    \Input iKeySize     - size of key
    \Input iCrvType     - curve type

    \Output
        int32_t         - one=call again, zero=success, negative=failure

    \Version 03/09/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLGenerateSignatureEcdsa(SecureStateT *pSecure, const uint8_t *pSigData, int32_t iSigSize, const uint8_t *pPrivateKey, int32_t iKeySize, int32_t iCrvType)
{
    CryptEccPointT Signature;
    int32_t iResult;
    CryptBnT PrivKey;
    uint32_t uCryptUsecs;
    SignatureVerifyT *pSigVerify = &pSecure->SigVerify;
    const CryptCurveDsaT *pDsa;

    // get the dsa functions
    if ((pDsa = _ProtoSSLDsaInitContext(pSigVerify, iCrvType)) == NULL)
    {
        return(-1);
    }
    // if the state is not initialized then assume a failure
    if (!pSigVerify->bEccContextInitialized)
    {
        return(-1);
    }

    // init private key
    CryptBnInitFrom(&PrivKey, -1, pPrivateKey, iKeySize);

    // sign digest with the private key
    if ((iResult = pDsa->Sign(pSigVerify->DsaContext, &PrivKey, pSigData, iSigSize, &Signature, &uCryptUsecs)) != 1)
    {
        NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (ecdsa sig encrypt) %dms\n", uCryptUsecs/1000));
        pSecure->uTimer += uCryptUsecs/1000;
        pSigVerify->iSigSize = _AsnWriteSignatureEcdsa(pSigVerify->aSigData, sizeof(pSigVerify->aSigData), &Signature);
        pSigVerify->bEccContextInitialized = FALSE;
    }
    return(iResult);
}
/*F********************************************************************************/
/*!
    \Function _ProtoSSLGenerateSignatureRSA

    \Description
        Generate an RSA signature; expected to be called iteratively

    \Input *pSecure     - secure state
    \Input *pSigData    - signature data
    \Input iSigSize     - signature data size
    \Input *pPrivateKey - private key used to sign

    \Output
        int32_t         - one=call again, zero=success, negative=failure

    \Version 03/09/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLGenerateSignatureRSA(SecureStateT *pSecure, const uint8_t *pSigData, int32_t iSigSize, const X509PrivateKeyT *pPrivateKey)
{
    // set up for rsa encryption
    if (!pSecure->bRSAContextInitialized)
    {
        // setup to encrypt signature object using private key
        if (CryptRSAInit2(&pSecure->RSAContext, pPrivateKey->Modulus.iObjSize, &pPrivateKey->PrimeP, &pPrivateKey->PrimeQ, &pPrivateKey->ExponentP, &pPrivateKey->ExponentQ, &pPrivateKey->Coefficient) != 0)
        {
            NetPrintf(("protossl: RSA init failed on private key encrypt\n"));
            return(-1);
        }
        // encrypt signature object
        if ((pSecure->uSslVersion < SSL3_TLS1_2) || (pSecure->pSigScheme->uVerifyScheme == SSL3_SIGVERIFY_RSA_PKCS1))
        {
            CryptRSAInitPrivate(&pSecure->RSAContext, pSigData, iSigSize);
        }
        else
        {
            CryptRSAInitSignature(&pSecure->RSAContext, pSigData, iSigSize);
        }
        pSecure->bRSAContextInitialized = TRUE;
    }

    /* encrypt signature hash; break it up into chunks of 64 to prevent blocking thread entire time.
       we don't do them one at a time because for a typical private key that would be 2048 calls and
       would incur unacceptable overhead, so this is a compromise between doing it all at once and
       doing it one step at a time */
    if (CryptRSAEncrypt(&pSecure->RSAContext, 64) > 0)
    {
        return(1);
    }

    // copy data to signature output
    pSecure->SigVerify.iSigSize = pSecure->RSAContext.iKeyModSize;
    ds_memcpy_s(pSecure->SigVerify.aSigData, sizeof(pSecure->SigVerify.aSigData), pSecure->RSAContext.EncryptBlock, pSecure->SigVerify.iSigSize);

    // done
    NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (rsa sig encrypt) %dms (%d calls)\n",
        pSecure->RSAContext.uCryptMsecs, pSecure->RSAContext.uNumExpCalls));
    pSecure->uTimer += pSecure->RSAContext.uCryptMsecs;
    pSecure->bRSAContextInitialized = FALSE;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLGenerateSignatureAsync

    \Description
        Generate a digital signature; expected to be called iteratively
        by the async update function.

    \Input *pState      - module state

    \Output
        int32_t         - one=call again, zero=success, negative=failure

    \Version 03/09/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLGenerateSignatureAsync(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;
    SignatureVerifyT *pSigVerify = &pSecure->SigVerify;
    int32_t iResult;

    if ((pSigVerify->pSigScheme != NULL) && (pSigVerify->pSigScheme->SigAlg.uSigAlg == SSL3_SIGALG_ECDSA))
    {
        iResult = _ProtoSSLGenerateSignatureEcdsa(pSecure, pSigVerify->aSigData, pSigVerify->iSigSize, pSigVerify->aKeyData, pSigVerify->iKeySize, (pSigVerify->iKeySize == 32) ? ASN_OBJ_ECDSA_SECP256R1 : ASN_OBJ_ECDSA_SECP384R1);
    }
    else
    {
        iResult = _ProtoSSLGenerateSignatureRSA(pSecure, pSigVerify->aSigData, pSigVerify->iSigSize, pSigVerify->pPrivateKey);
    }
    if (iResult <= 0)
    {
        if (iResult < 0)
        {
            NetPrintf(("protossl: %s signature generate failed\n", pSigVerify->pSigScheme->SigAlg.uSigAlg == SSL3_SIGALG_ECDSA ? "ecdsa" : "rsa"));
        }
        pSigVerify->pSigScheme = NULL;
        pSecure->bSigGenerated = TRUE;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLGenerateSignature

    \Description
        Starts async operation to generate a digital signature

    \Input *pState      - module state
    \Input *pSecure     - secure state
    \Input *pSigScheme  - signature scheme to use
    \Input *pSigData    - signature to sign
    \Input iSigSize     - signature size
    \Input *pPrivateKey - private key
    \Input iNextState   - next state to transition to after async op

    \Output
        int32_t         - one=call again, zero=success, negative=failure

    \Version 03/09/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLGenerateSignature(ProtoSSLRefT *pState, SecureStateT *pSecure, const SignatureSchemeT *pSigScheme, const uint8_t *pSigData, int32_t iSigSize, X509PrivateKeyT *pPrivateKey, int32_t iNextState)
{
    SignatureVerifyT *pSigVerify = &pSecure->SigVerify;
    
    pSigVerify->pSigScheme = pSigScheme;
    ds_memcpy_s(pSigVerify->aSigData, sizeof(pSigVerify->aSigData), pSigData, iSigSize);
    pSigVerify->iSigSize = iSigSize;
    pSigVerify->pPrivateKey = pPrivateKey;
    pSigVerify->iNextState = iNextState;
    pSigVerify->bEccContextInitialized = FALSE;
    if (pSigScheme->SigAlg.uSigAlg == SSL3_SIGALG_ECDSA)
    {
        pSigVerify->iKeySize = pPrivateKey->Modulus.iObjSize;
        ds_memcpy_s(pSigVerify->aKeyData, sizeof(pSigVerify->aKeyData), pPrivateKey->Modulus.pObjData, pSigVerify->iKeySize);
    }

    return(_ProtoSSLUpdateSetAsyncState(pState, _ProtoSSLGenerateSignatureAsync, iNextState, ST_FAIL_SETUP, SSL3_ALERT_DESC_INTERNAL_ERROR));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLVerifySignatureEcdsa

    \Description
        Verify an ECDSA signature; expected to be called iteratively

    \Input *pSecure     - secure state
    \Input *pSigData    - signature data
    \Input iSigSize     - signature data size
    \Input *pHashData   - hash to validate
    \Input iHashSize    - hash size
    \Input *pPubKey     - ECC public key data
    \Input iPubKeySize  - size of key
    \Input iCrvType     - curve type

    \Output
        int32_t         - one=call again, zero=success, negative=failure

    \Version 02/09/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLVerifySignatureEcdsa(SecureStateT *pSecure, const uint8_t *pSigData, int32_t iSigSize, const uint8_t *pHashData, int32_t iHashSize, const uint8_t *pPubKey, int32_t iPubKeySize, int32_t iCrvType)
{
    CryptEccPointT PubKey, Signature;
    const uint8_t *pData = pSigData, *pLast = pSigData+iSigSize;
    SignatureVerifyT *pSigVerify = &pSecure->SigVerify;
    const CryptCurveDsaT *pDsa;
    uint8_t aSigData[128];
    int32_t iSize, iResult;
    uint32_t uCryptUsecs;

    // get the dsa functions
    if ((pDsa = _ProtoSSLDsaInitContext(pSigVerify, iCrvType)) == NULL)
    {
        return(-1);
    }
    // if the state is not initialized then assume a failure
    if (!pSigVerify->bEccContextInitialized)
    {
        return(-1);
    }

    // init public key
    pDsa->PointInit(&PubKey, pPubKey, iPubKeySize);

    // parse the sequence
    if ((pData = _AsnParseHeaderType(pData, pLast, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        return(-1);
    }
    // parse and set the first component
    if ((pData = _AsnParseBinary(pData, pLast, ASN_TYPE_INTEGER+ASN_PRIMITIVE, aSigData, sizeof(aSigData), &iSize, "signature data x")) == NULL)
    {
        return(-1);
    }
    CryptBnInitFrom(&Signature.X, -1, aSigData, iSize);
    // parse and set the second component
    if ((pData = _AsnParseBinary(pData, pLast, ASN_TYPE_INTEGER+ASN_PRIMITIVE, aSigData, sizeof(aSigData), &iSize, "signature data y")) == NULL)
    {
        return(-1);
    }
    CryptBnInitFrom(&Signature.Y, -1, aSigData, iSize);

    // do the verify
    if ((iResult = pDsa->Verify(pSigVerify->DsaContext, &PubKey, pHashData, iHashSize, &Signature, &uCryptUsecs)) != 1)
    {
        NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (ecdsa sig verify) %dms\n", uCryptUsecs/1000));
        pSecure->uTimer += uCryptUsecs/1000;
        pSigVerify->bEccContextInitialized = FALSE;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLVerifySignatureRSA

    \Description
        Verify an RSA signature; expected to be called iteratively

    \Input *pSecure     - secure state
    \Input *pSigData    - signature data
    \Input iSigSize     - signature data size
    \Input iSigType     - signature type
    \Input *pHashData   - hash to validate
    \Input iHashSize    - hash size
    \Input eHashType    - hash type
    \Input iSigSalt     - signature salt length
    \Input eMgfHash     - signature message generation function (mgf)
    \Input *pKeyModData - rsa public key modulus
    \Input iKeyModSize  - key modulus length
    \Input *pKeyExpData - rsa public key exponent
    \Input iKeyExpSize  - key exponent length

    \Output
        int32_t         - one=call again, zero=success, negative=failure

    \Version 07/22/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLVerifySignatureRSA(SecureStateT *pSecure, const uint8_t *pSigData, int32_t iSigSize, int32_t iSigType, const uint8_t *pHashData, int32_t iHashSize, CryptHashTypeE eHashType, int32_t iSigSalt, CryptHashTypeE eMgfHash, const uint8_t *pKeyModData, int32_t iKeyModSize, const uint8_t *pKeyExpData, int32_t iKeyExpSize)
{
    int32_t iResult;

    // set up for rsa encryption
    if (!pSecure->bRSAContextInitialized)
    {
        if (CryptRSAInit(&pSecure->RSAContext, pKeyModData, iKeyModSize, pKeyExpData, iKeyExpSize))
        {
            return(-1);
        }
        CryptRSAInitSignature(&pSecure->RSAContext, pSigData, iSigSize);
        pSecure->bRSAContextInitialized = TRUE;
    }
    // do a round of encryption
    if ((iResult = CryptRSAEncrypt(&pSecure->RSAContext, 16)) > 0)
    {
        return(iResult);
    }
    pSecure->bRSAContextInitialized = FALSE;
    NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (rsa sig verify) %dms\n", pSecure->RSAContext.uCryptMsecs));
    pSecure->uTimer += pSecure->RSAContext.uCryptMsecs;

    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->RSAContext.EncryptBlock, pSecure->RSAContext.iKeyModSize, "decrypted signature");
    #endif

    // validate decrypted signature
    if (iSigType != ASN_OBJ_RSASSA_PSS)
    {
        iResult = -1;
        // extract hash data from signature block
        if ((pSigData = _Pkcs1VerifyEMSA(pSecure->RSAContext.EncryptBlock, iSigSize, iHashSize, eHashType)) != NULL)
        {
            // compare hash data with precalculated certificate body hash
            iResult = !memcmp(pHashData, pSigData, iHashSize) ? 0 : -1;
        }
    }
    else
    {
        // extract and validate signature hash
        iResult = _Pkcs1VerifyEMSA_PSS(pSecure->RSAContext.EncryptBlock, iKeyModSize, eHashType, iSigSalt, pHashData, iHashSize, eMgfHash) ? 0 : -1;
    }

    // return result to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLVerifySignatureAsync

    \Description
        Verify a digital signature; expected to be called iteratively
        by the async update function.

    \Input *pState      - module state

    \Output
        int32_t         - one=call again, zero=success, negative=failure

    \Version 02/16/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLVerifySignatureAsync(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;
    SignatureVerifyT *pSigVerify = &pSecure->SigVerify;
    int32_t iResult;

    if ((pSigVerify->pSigScheme != NULL) && (pSigVerify->pSigScheme->SigAlg.uSigAlg == SSL3_SIGALG_ECDSA))
    {
        iResult = _ProtoSSLVerifySignatureEcdsa(pSecure, pSigVerify->aSigData, pSigVerify->iSigSize, pSigVerify->aHashDigest, pSigVerify->iHashSize, pSecure->Cert.KeyModData, pSecure->Cert.iKeyModSize, pSecure->Cert.iCrvType);
    }
    else
    {
        iResult = _ProtoSSLVerifySignatureRSA(pSecure, pSigVerify->aSigData, pSigVerify->iSigSize, (pSigVerify->pSigScheme->uVerifyScheme == SSL3_SIGVERIFY_RSA_PSS) ? ASN_OBJ_RSASSA_PSS : ASN_OBJ_RSA_PKCS_SHA256,
            pSigVerify->aHashDigest, pSigVerify->iHashSize, pSigVerify->eHashType, pSigVerify->iHashSize, pSigVerify->eHashType, pSecure->Cert.KeyModData, pSecure->Cert.iKeyModSize, pSecure->Cert.KeyExpData,
            pSecure->Cert.iKeyExpSize);
    }
    if (iResult <= 0)
    {
        if (iResult < 0)
        {
            NetPrintf(("protossl: %s signature verify failed\n", pSigVerify->pSigScheme->SigAlg.uSigAlg == SSL3_SIGALG_ECDSA ? "ecdsa" : "rsa"));
        }
        pSigVerify->pSigScheme = NULL;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLVerifySignature

    \Description
        Starts async operation to verify a digital signature

    \Input *pState      - module state
    \Input *pSecure     - secure state
    \Input *pSigScheme  - signature scheme to use
    \Input *pSigData    - signature data
    \Input iSigSize     - signature data size
    \Input *pHashData   - hash to validate
    \Input iHashSize    - hash size
    \Input eHashType    - hash type
    \Input iNextState   - next state to transition to after async op

    \Output
        int32_t         - one=call again, zero=success, negative=failure

    \Version 02/16/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLVerifySignature(ProtoSSLRefT *pState, SecureStateT *pSecure, const SignatureSchemeT *pSigScheme, const uint8_t *pSigData, int32_t iSigSize, const uint8_t *pHashData, int32_t iHashSize, CryptHashTypeE eHashType, int32_t iNextState)
{
    SignatureVerifyT *pSigVerify = &pSecure->SigVerify;

    pSigVerify->pSigScheme = pSigScheme;
    ds_memcpy_s(pSigVerify->aSigData, sizeof(pSigVerify->aSigData), pSigData, iSigSize);
    pSigVerify->iSigSize = iSigSize;
    ds_memcpy_s(pSigVerify->aHashDigest, sizeof(pSigVerify->aHashDigest), pHashData, iHashSize);
    pSigVerify->iHashSize = iHashSize;
    pSigVerify->eHashType = eHashType;
    pSigVerify->iNextState = iNextState;
    pSigVerify->bEccContextInitialized = FALSE;

    return(_ProtoSSLUpdateSetAsyncState(pState, _ProtoSSLVerifySignatureAsync, iNextState, ST_FAIL_SETUP, SSL3_ALERT_DESC_DECRYPT_ERROR));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLVerifyCertificateSignature

    \Description
        Check an X.509 signature for validity; BLOCKING

    \Input *pSecure     - secure state
    \Input *pCert       - pointer to certificate to validate
    \Input *pKeyModData - pointer to key modulus data
    \Input iKeyModSize  - size of key modulus data
    \Input *pKeyExpData - pointer to key exponent data
    \Input iKeyExpSize  - size of key exponent data
    \Input iKeyType     - key type
    \Input iCrvType     - key curve type, if ec

    \Output
        int32_t         - zero=success, else failure

    \Version 07/22/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLVerifyCertificateSignature(SecureStateT *pSecure, X509CertificateT *pCert, const uint8_t *pKeyModData, int32_t iKeyModSize, const uint8_t *pKeyExpData, int32_t iKeyExpSize, int32_t iKeyType, int32_t iCrvType)
{
    int32_t iResult;

    NetPrintfVerbose((DEBUG_VAL_CERT, 0, "protossl: verifying signature\n"));

    // if ecdsa
    if (iKeyType == ASN_OBJ_ECDSA_KEY)
    {
        // verify the signature
        while((iResult = _ProtoSSLVerifySignatureEcdsa(pSecure, pCert->SigData, pCert->iSigSize, pCert->HashData, pCert->iHashSize, pKeyModData, iKeyModSize, iCrvType)) > 0)
            ;
    }
    else
    {
        while((iResult = _ProtoSSLVerifySignatureRSA(pSecure, pCert->SigData, pCert->iSigSize, pCert->iSigType, pCert->HashData, pCert->iHashSize, pCert->eHashType, pCert->iSigSalt,
            _AsnGetHashType(pCert->iMgfHash), pKeyModData, iKeyModSize, pKeyExpData, iKeyExpSize)) > 0)
            ;
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLVerifyCertificate

    \Description
        Verify a previously parsed x.509 certificate

    \Input *pState      - module state (may be NULL)
    \Input *pSecure     - secure state
    \Input *pCert       - pointer to certificate to fill in from header data
    \Input bCertIsCA    - is the certificate a CA?

    \Output int32_t     - zero=success, positive=duplicate, else error

    \Version 03/11/2009 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLVerifyCertificate(ProtoSSLRefT *pState, SecureStateT *pSecure, X509CertificateT *pCert, uint8_t bCertIsCA)
{
    int32_t iResult = 0;

    // if self-signed permitted and certificate is self-signed, then point to its key info (mod+exp)
    if ((bCertIsCA == TRUE) && (_CertificateCompareIdent(&pCert->Subject, &pCert->Issuer, TRUE) == 0))
    {
        if (_ProtoSSLVerifyCertificateSignature(pSecure, pCert, pCert->KeyModData, pCert->iKeyModSize, pCert->KeyExpData, pCert->iKeyExpSize, pCert->iKeyType, pCert->iCrvType) != 0)
        {
            NetPrintf(("protossl: _VerifyCertificate: signature hash mismatch on self-signed cert\n"));
            _CertificateDebugPrint(pCert, "failed cert");
            return(-50);
        }
    }
    else
    {
        ProtoSSLCACertT *pCACert;

        #if DIRTYCODE_LOGGING
        if ((bCertIsCA == TRUE) && (pCert->iCertIsCA == FALSE)) // when ProtoSSLSetCACert* is called
        {
            NetPrintf(("protossl: _VerifyCertificate: warning --- trying to add a non-CA cert as a CA.\n"));
            _CertificateDebugPrint(pCert, "warning cert");
        }
        #endif

        // locate a matching CA
        for (pCACert = _ProtoSSL_CACerts; pCACert != NULL; pCACert = pCACert->pNext)
        {
            // first, compare to see if our subject matches their issuer
            if ((_CertificateCompareIdent(&pCACert->Subject, &pCert->Issuer, (bCertIsCA||pCert->iCertIsCA)) != 0) ||
                ((pCACert->iKeyModSize != pCert->iSigSize) && (pCert->iKeyType != ASN_OBJ_ECDSA_KEY)))
            {
                continue;
            }

            // verify against this CA
            if (_ProtoSSLVerifyCertificateSignature(pSecure, pCert, pCACert->pKeyModData, pCACert->iKeyModSize, pCACert->KeyExpData, pCACert->iKeyExpSize, pCACert->iKeyType, pCACert->iCrvType) != 0)
            {
                continue;
            }

            NetPrintfVerbose((DEBUG_VAL_CERT, 0, "protossl: verifying succeeded\n"));

            // special processing when validating against a GOS CA
            if (pCACert->uFlags & SSL_CACERTFLAG_GOSCA)
            {
                // make sure the domain of the certificate is an EA domain
                if (ds_stricmpwc(pCert->Subject.strCommon, "*.ea.com") && ds_stricmpwc(pCert->Subject.strCommon, "*.easports.com"))
                {
                    return(SSL_ERR_GOSCA_INVALIDUSE);
                }
                // ignore date range validation failure
                if (pSecure->bDateVerifyFailed)
                {
                    NetPrintf(("protossl: ignoring date range validation failure for GOS CA\n"));
                    pSecure->bDateVerifyFailed = FALSE;
                }
            }
            // only CAPROVIDER CAs can be used against gosca
            if (!ds_stricmpwc(pCert->Subject.strCommon, "gosca.ea.com") && !(pCACert->uFlags & SSL_CACERTFLAG_CAPROVIDER))
            {
                return(SSL_ERR_CAPROVIDER_INVALID);
            }

            // if the CA hasn't been verified already, do that now
            if (pCACert->pX509Cert != NULL)
            {
                if ((iResult = _ProtoSSLVerifyCertificate(pState, pSecure, pCACert->pX509Cert, TRUE)) == 0)
                {
                    #if DIRTYCODE_LOGGING
                    char strIdentSubject[512], strIdentIssuer[512];
                    int32_t iVerbose = (pState != NULL) ? pState->iVerbose : 0;
                    NetPrintfVerbose((iVerbose, 0, "protossl: ca (%s) validated by ca (%s)\n", _CertificateDebugFormatIdent(&pCACert->pX509Cert->Subject, strIdentSubject, sizeof(strIdentSubject)),
                        _CertificateDebugFormatIdent(&pCACert->pX509Cert->Issuer, strIdentIssuer, sizeof(strIdentIssuer))));
                    #endif

                    // cert successfully verified
                    DirtyMemFree(pCACert->pX509Cert, PROTOSSL_MEMID, pCACert->iMemGroup, pCACert->pMemGroupUserData);
                    pCACert->pX509Cert = NULL;
                }
                else
                {
                    _CertificateSetFailureInfo(pState, pCACert->pX509Cert, TRUE);
                    continue;
                }
            }

            // verified
            break;
        }
        if (pCACert == NULL)
        {
            #if DIRTYCODE_LOGGING
            // output debug logging only if we're manually loading a CA cert
            if (bCertIsCA)
            {
                _CertificateDebugPrint(pCert, "_VerifyCertificate() -- no CA available for this certificate");
            }
            #endif
            _CertificateSetFailureInfo(pState, pCert, TRUE);
            return(-51);
        }
    }
    // success
    return(iResult);
}

/*
    hmac, building tls1 keys and key material, generating and verifying mac
*/

/*F********************************************************************************/
/*!
    \Function _ProtoSSLDoHmac

    \Description
        Calculates HMAC using CryptHmac

    \Input *pBuffer     - [out] storage for generated MAC
    \Input iBufLen      - size of output
    \Input *pMessage    - input data to hash
    \Input iMessageLen  - size of input data
    \Input *pMessage2   - more input data to hash (optional)
    \Input iMessageLen2 - size of more input data (if pMessage2 != NULL)
    \Input pKey         - seed
    \Input iKeyLen      - seed length
    \Input eHashType    - hash type

    \Version 10/11/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLDoHmac(uint8_t *pBuffer, int32_t iBufLen, const uint8_t *pMessage, int32_t iMessageLen, const uint8_t *pMessage2, int32_t iMessageLen2, const uint8_t *pKey, int32_t iKeyLen, CryptHashTypeE eHashType)
{
    if (pMessage2 != NULL)
    {
        CryptHmacMsgT Message[2];
        Message[0].pMessage = pMessage;
        Message[0].iMessageLen = iMessageLen;
        Message[1].pMessage = pMessage2;
        Message[1].iMessageLen = iMessageLen2;
        CryptHmacCalcMulti(pBuffer, iBufLen, Message, 2, pKey, iKeyLen, eHashType);
    }
    else
    {
        CryptHmacCalc(pBuffer, iBufLen, pMessage, iMessageLen, pKey, iKeyLen, eHashType);
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLDoPHash

    \Description
        Implements P_hash as defined in https://tools.ietf.org/html/rfc5246#section-5

    \Input *pBuffer     - [out] storage for P_hash result
    \Input iOutLen      - size of output expected
    \Input *pSecret     - secret
    \Input iSecretLen   - length of secret
    \Input *pSeed       - seed
    \Input iSeedLen     - length of seed
    \Input eHashType    - hash type

    \Version 10/11/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLDoPHash(uint8_t *pBuffer, int32_t iOutLen, const uint8_t *pSecret, int32_t iSecretLen, const uint8_t *pSeed, int32_t iSeedLen, CryptHashTypeE eHashType)
{
    uint8_t aWork[128];
    int32_t iHashSize = CryptHashGetSize(eHashType);

    // A(1)
    _ProtoSSLDoHmac(aWork, sizeof(aWork), pSeed, iSeedLen, NULL, 0, pSecret, iSecretLen, eHashType);
    ds_memcpy(aWork+iHashSize, pSeed, iSeedLen);
    _ProtoSSLDoHmac(pBuffer, iOutLen, aWork, iSeedLen+iHashSize, NULL, 0, pSecret, iSecretLen, eHashType);

    // A(n)
    while (iOutLen > iHashSize)
    {
        uint8_t aWork2[128];

        pBuffer += iHashSize;
        iOutLen -= iHashSize;

        _ProtoSSLDoHmac(aWork2, sizeof(aWork2), aWork, iHashSize, NULL, 0, pSecret, iSecretLen, eHashType);
        ds_memcpy(aWork, aWork2, iHashSize);
        _ProtoSSLDoHmac(pBuffer, iOutLen, aWork, iSeedLen+iHashSize, NULL, 0, pSecret, iSecretLen, eHashType);
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLDoPRF

    \Description
        Implements PRF as defined in https://tools.ietf.org/html/rfc5246#section-5

    \Input *pBuffer     - [out] storage for P_hash result
    \Input iOutLen      - size of output expected
    \Input *pSecret     - secret
    \Input iSecretLen   - length of secret
    \Input *pSeed       - seed
    \Input iSeedLen     - length of seed

    \Version 10/11/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLDoPRF(uint8_t *pBuffer, int32_t iOutLen, const uint8_t *pSecret, int32_t iSecretLen, const uint8_t *pSeed, int32_t iSeedLen)
{
    uint8_t aMD5Buf[SSL_KEYMATERIAL_LEN], aSHABuf[SSL_KEYMATERIAL_LEN];
    int32_t iLS = iSecretLen/2;         // split secret in two
    int32_t iBufCnt;
    iLS += iSecretLen & 1;              // handle odd secret lengths

    // execute md5 p_hash
    _ProtoSSLDoPHash(aMD5Buf, iOutLen, pSecret, iLS, pSeed, iSeedLen, CRYPTHASH_MD5);

    // execute sha1 p_hash
    _ProtoSSLDoPHash(aSHABuf, iOutLen, pSecret+iLS, iLS, pSeed, iSeedLen, CRYPTHASH_SHA1);

    // execute XOR of MD5 and SHA hashes
    for (iBufCnt = 0; iBufCnt < iOutLen; iBufCnt += 1)
    {
        pBuffer[iBufCnt] = aMD5Buf[iBufCnt] ^ aSHABuf[iBufCnt];
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLBuildKey

    \Description
        Builds key material/master secret for TLS versions prior to 1.3

    \Input *pState      - module state reference
    \Input *pBuffer     - [out] output for generated key data
    \Input iBufSize     - size of output buffer
    \Input *pSource     - source data
    \Input iSourceLen   - length of source data
    \Input *pRandomA    - random data
    \Input *pRandomB    - random data
    \Input iRandomLen   - length of random data
    \Input *pLabel      - label (TLS1 PRF only)
    \Input uSslVersion  - ssl version being used

    \Version 10/11/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLBuildKey(ProtoSSLRefT *pState, uint8_t *pBuffer, int32_t iBufSize, uint8_t *pSource, int32_t iSourceLen, uint8_t *pRandomA, uint8_t *pRandomB, int32_t iRandomLen, const char *pLabel, uint16_t uSslVersion)
{
    SecureStateT *pSecure = pState->pSecure;
    uint8_t aWork[128]; // must hold at least 13+32+32 bytes

    ds_strnzcpy((char *)aWork, pLabel, sizeof(aWork));
    ds_memcpy(aWork+13, pRandomA, iRandomLen);
    ds_memcpy(aWork+45, pRandomB, iRandomLen);
    if (uSslVersion < SSL3_TLS1_2)
    {
        // ref http://tools.ietf.org/html/rfc2246#section-6.3
        _ProtoSSLDoPRF(pBuffer, iBufSize, pSource, iSourceLen, aWork, 77);
    }
    else
    {
        // ref https://tools.ietf.org/html/rfc5246#section-6.3; tls1.2 drops md5+sha1 prf and uses the cipher prf hash directly instead
        _ProtoSSLDoPHash(pBuffer, iBufSize, pSource, iSourceLen, aWork, 77, (CryptHashTypeE)pSecure->pCipher->uPrfType);
    }

    if (pBuffer == pSecure->MasterKey)
    {
        #if DIRTYCODE_LOGGING
        char strBuf1[65], strBuf2[97];
        #endif  

        #if DEBUG_RAW_DATA
        NetPrintMem(pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey), "PreMaster");
        NetPrintMem(pSecure->MasterKey, sizeof(pSecure->MasterKey), "Master");
        #endif

        // log params for wireshark decrypt
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: CLIENT_RANDOM %s %s\n",
            ds_fmtoctstring(strBuf1, sizeof(strBuf1), pSecure->ClientRandom, sizeof(pSecure->ClientRandom)),
            ds_fmtoctstring(strBuf2, sizeof(strBuf2), pBuffer, iBufSize)));

        // get rid of premaster secret from memory
        ds_memclr(pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey));
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLBuildKeyMaterial

    \Description
        Build and distribute key material from master secret, server
        random, and client random.  Used for TLS1.2 and prior.

    \Input *pState      - module state reference

    \Version 03/19/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLBuildKeyMaterial(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;
    uint8_t *pData;

    #if DEBUG_RAW_DATA
    NetPrintf(("protossl: building key material\n"));
    NetPrintMem(pSecure->MasterKey, sizeof(pSecure->MasterKey), "MasterKey");
    NetPrintMem(pSecure->ServerRandom, sizeof(pSecure->ServerRandom), "ServerRandom");
    NetPrintMem(pSecure->ClientRandom, sizeof(pSecure->ClientRandom), "ClientRandom");
    #endif

    // build key material; limit to SSL_KEYMATERIAL_LEN for upstream code even though the keyblock is bigger
    _ProtoSSLBuildKey(pState, pSecure->KeyBlock, SSL_KEYMATERIAL_LEN, pSecure->MasterKey, sizeof(pSecure->MasterKey),
        pSecure->ServerRandom, pSecure->ClientRandom, sizeof(pSecure->ServerRandom), "key expansion",
        pSecure->uSslVersion);

    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->KeyBlock, sizeof(pSecure->KeyBlock), "KeyExpansion");
    #endif

    // distribute the key material
    pData = pSecure->KeyBlock;
    pSecure->pClientMAC = pData;
    pData += pSecure->pCipher->uMac;
    pSecure->pServerMAC = pData;
    pData += pSecure->pCipher->uMac;
    pSecure->pClientKey = pData;
    pData += pSecure->pCipher->uLen;
    pSecure->pServerKey = pData;
    pData += pSecure->pCipher->uLen;
    pSecure->pClientInitVec = pData;
    pData += pSecure->pCipher->uVecSize;
    pSecure->pServerInitVec = pData;
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLGenerateMacSource

    \Description
        Build the MAC (Message Authentication Code) source data

    \Input *pBuffer - [out] storage for generated MAC
    \Input uSeqn    - sequence number
    \Input uType    - message type
    \Input uSslVers - ssl version
    \Input uSize    - data size

    \Output
        uint8_t *   - pointer past end of generated MAC

    \Version 10/11/2012 (jbrookes) Refactored and added TLS support
*/
/********************************************************************************F*/
static uint8_t *_ProtoSSLGenerateMacSource(uint8_t *pBuffer, uint64_t uSeqn, uint32_t uType, uint32_t uSslVers, uint32_t uSize)
{
    *pBuffer++ = (uint8_t)((uSeqn>>56)&255);
    *pBuffer++ = (uint8_t)((uSeqn>>48)&255);
    *pBuffer++ = (uint8_t)((uSeqn>>40)&255);
    *pBuffer++ = (uint8_t)((uSeqn>>32)&255);
    *pBuffer++ = (uint8_t)((uSeqn>>24)&255);
    *pBuffer++ = (uint8_t)((uSeqn>>16)&255);
    *pBuffer++ = (uint8_t)((uSeqn>>8)&255);
    *pBuffer++ = (uint8_t)((uSeqn>>0)&255);
    *pBuffer++ = (uint8_t)uType;
    *pBuffer++ = (uint8_t)(uSslVers>>8);
    *pBuffer++ = (uint8_t)(uSslVers>>0);
    *pBuffer++ = (uint8_t)(uSize>>8);
    *pBuffer++ = (uint8_t)(uSize>>0);
    return(pBuffer);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLGenerateMac

    \Description
        Generate MAC for secure send when using non-AEAD ciphers

    \Input *pSecure     - secure state
    \Input *pSend       - data to authenticate
    \Input iSize        - size of data to authenticate
    \Input bServer      - TRUE if operating as server, else FALSE

    \Output
        int32_t         - size of authenticated data plus MAC

    \Version 03/02/2018 (jbrookes) Refactored from _ProtoSSLSendPacket()
*/
/********************************************************************************F*/
static int32_t _ProtoSSLGenerateMac(SecureStateT *pSecure, uint8_t *pSend, int32_t iSize, uint8_t bServer)
{
    uint8_t MacTemp[CRYPTHASH_MAXDIGEST], *pMacTemp;

    // generate the mac source
    pMacTemp = _ProtoSSLGenerateMacSource(MacTemp, pSecure->uSendSeqn, pSecure->SendData[0], pSecure->uSslVersion, (uint32_t)iSize);

    // hash the mac and append to send data
    _ProtoSSLDoHmac(pSend+iSize, pSecure->pCipher->uMac, MacTemp, (int32_t)(pMacTemp-MacTemp), pSend, iSize,
        bServer ? pSecure->pServerMAC : pSecure->pClientMAC,
        pSecure->pCipher->uMac, (CryptHashTypeE)pSecure->pCipher->uMacType);

    // return size+mac to caller
    return(iSize+pSecure->pCipher->uMac);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLVerifyMac

    \Description
        Verify MAC for secure recv when using non-AEAD ciphers

    \Input *pSecure     - secure state
    \Input iSize        - size of data to authenticate
    \Input bServer      - TRUE if operating as server, else FALSE
    \Input *pBadMac     - [in/out] bad MAC flag

    \Output
        int32_t         - zero=success, negative=failure

    \Version 03/02/2018 (jbrookes) Refactored from _ProtoSSLSendPacket()
*/
/********************************************************************************F*/
static int32_t _ProtoSSLVerifyMac(SecureStateT *pSecure, int32_t iSize, uint8_t bServer, uint8_t *pBadMac)
{
    uint8_t MacTemp[CRYPTHASH_MAXDIGEST], *pMacTemp;
    uint8_t bBadMac;

    // make sure there is room for mac
    if (iSize >= (int32_t)pSecure->pCipher->uMac)
    {
        iSize -= pSecure->pCipher->uMac;
        // remove the mac from size
        pSecure->iRecvProg = pSecure->iRecvSize = pSecure->iRecvBase+iSize;
    }
    else
    {
        NetPrintf(("protossl: _ProtoSSLVerifyMac: no room for mac (%d < %d)\n", iSize, pSecure->pCipher->uMac));
        *pBadMac = TRUE;
        return(-1);
    }

    // generate MAC source
    pMacTemp = _ProtoSSLGenerateMacSource(MacTemp, pSecure->uRecvSeqn, pSecure->RecvHead[0], pSecure->uSslVersion, (uint32_t)iSize);

    // do the hash
    _ProtoSSLDoHmac(MacTemp, pSecure->pCipher->uMac, MacTemp, (int32_t)(pMacTemp-MacTemp),
        pSecure->RecvData+pSecure->iRecvBase, pSecure->iRecvSize-pSecure->iRecvBase,
        bServer ? pSecure->pClientMAC : pSecure->pServerMAC,
        pSecure->pCipher->uMac, (CryptHashTypeE)pSecure->pCipher->uMacType);

    // validate MAC
    bBadMac = memcmp(MacTemp, pSecure->RecvData+pSecure->iRecvSize, pSecure->pCipher->uMac) ? TRUE : FALSE;
    // accumulate MAC flag
    *pBadMac |= bBadMac;
    // return success or failure to caller
    return(!(*pBadMac) ? 0 : -1);
}

/*
    handshake hash management
*/

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHandshakeHashInit

    \Description
        Init handshake hash states

    \Input *pSecure - secure state ref

    \Version 03/21/2017 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLHandshakeHashInit(SecureStateT *pSecure)
{
    CryptMD5Init(&pSecure->HandshakeMD5);
    CryptSha1Init(&pSecure->HandshakeSHA);
    CryptSha2Init(&pSecure->HandshakeSHA256, SSL3_MAC_SHA256);
    CryptSha2Init(&pSecure->HandshakeSHA384, SSL3_MAC_SHA384);
    CryptSha2Init(&pSecure->HandshakeSHA512, SSL3_MAC_SHA512);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHandshakeHashUpdate

    \Description
        Update handshake hash with specified data

    \Input *pSecure - secure state ref
    \Input *pData   - data to add to hash
    \Input iSize    - size of data to add
    \Input *pLabel  - debug label

    \Version 03/21/2017 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLHandshakeHashUpdate(SecureStateT *pSecure, const uint8_t *pData, int32_t iSize, const char *pLabel)
{
    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: %s handshake update (size=%d)\n", pLabel, iSize));
    CryptMD5Update(&pSecure->HandshakeMD5, pData, iSize);
    CryptSha1Update(&pSecure->HandshakeSHA, pData, iSize);
    CryptSha2Update(&pSecure->HandshakeSHA256, pData, iSize);
    CryptSha2Update(&pSecure->HandshakeSHA384, pData, iSize);
    CryptSha2Update(&pSecure->HandshakeSHA512, pData, iSize);

    #if DEBUG_RAW_DATA
    NetPrintMem(pData, iSize, "handshake data");
    #endif
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHandshakeHashGet

    \Description
        Get the current handshake hash, for the specified hash type

    \Input *pSecure     - secure state ref
    \Input eHashType    - hash type to get
    \Input *pBuffer     - [out] buffer to store hash
    \Input iBufSize     - size of output buffer (should be bigger than hash size)

    \Output
        int32_t         - size of hash data, or zero on failure

    \Version 03/21/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHandshakeHashGet(SecureStateT *pSecure, CryptHashTypeE eHashType, uint8_t *pBuffer, int32_t iBufSize)
{
    CryptMD5T MD5Context;
    CryptSha1T SHA1Context;
    CryptSha2T SHA2Context;
    int32_t iHashSize;

    if ((iHashSize = CryptHashGetSize(eHashType)) < 0)
    {
        NetPrintf(("protossl: handshake hash type %d unknown\n", eHashType));
        ds_memclr(pBuffer, iBufSize);
        return(0);
    }
    else if (iHashSize > iBufSize)
    {
        NetPrintf(("protossl: handshake hash too large for buffer; truncating\n"));
        iHashSize = iBufSize;
    }

    switch (eHashType)
    {
        case CRYPTHASH_MD5:
            ds_memcpy_s(&MD5Context, sizeof(MD5Context), &pSecure->HandshakeMD5, sizeof(pSecure->HandshakeMD5));
            CryptMD5Final(&MD5Context, pBuffer, iHashSize);
            break;
        case CRYPTHASH_SHA1:
            ds_memcpy_s(&SHA1Context, sizeof(SHA1Context), &pSecure->HandshakeSHA, sizeof(pSecure->HandshakeSHA));
            CryptSha1Final(&SHA1Context, pBuffer, iHashSize);
            break;
        case CRYPTHASH_SHA256:
            ds_memcpy(&SHA2Context, &pSecure->HandshakeSHA256, sizeof(pSecure->HandshakeSHA256));
            CryptSha2Final(&SHA2Context, pBuffer, iHashSize);
            break;
        case CRYPTHASH_SHA384:
            ds_memcpy(&SHA2Context, &pSecure->HandshakeSHA384, sizeof(pSecure->HandshakeSHA384));
            CryptSha2Final(&SHA2Context, pBuffer, iHashSize);
            break;
        case CRYPTHASH_SHA512:
            ds_memcpy(&SHA2Context, &pSecure->HandshakeSHA512, sizeof(pSecure->HandshakeSHA512));
            CryptSha2Final(&SHA2Context, pBuffer, iHashSize);
            break;
        default:
            break;
    }

    return(iHashSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHandshakeHashInject

    \Description
        Inject synthetic message hash in Hello Retry Request as per
        https://tools.ietf.org/html/rfc8446#section-4.4.1

    \Input *pSecure - secure state ref
    \Input eHashType - hash type to update

    \Version 08/10/2017 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLHandshakeHashInject(SecureStateT *pSecure, CryptHashTypeE eHashType)
{
    uint8_t aHash[CRYPTHASH_MAXDIGEST], aHead[4];
    CryptSha2T *pHandshakeCtx;
    int32_t iHashSize = CryptHashGetSize(eHashType);

    // get handshake context
    pHandshakeCtx = (eHashType == CRYPTHASH_SHA256) ? &pSecure->HandshakeSHA256 : &pSecure->HandshakeSHA384;

    // get and reinit current hash
    CryptSha2Final(pHandshakeCtx, aHash, iHashSize);
    CryptSha2Init(pHandshakeCtx, iHashSize);

    // create synthetic message header
    aHead[0] = SSL3_MSG_MESSAGE_HASH;
    aHead[1] = 0;
    aHead[2] = 0;
    aHead[3] = (uint8_t)iHashSize;

    #if DEBUG_RAW_DATA
    NetPrintMem(aHead, sizeof(aHead), "msghead");
    NetPrintMem(aHash, iHashSize, "msghash");
    #endif

    // hash the message header
    CryptSha2Update(pHandshakeCtx, aHead, sizeof(aHead));
    // and the message hash
    CryptSha2Update(pHandshakeCtx, aHash, iHashSize);
}

/*
    tls 1.3 key schedule
*/

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHkdfExtract

    \Description
        Implements HKDF-Extract as defined in rfc 5869; see
        https://tools.ietf.org/html/rfc5869#section-2.2

    \Input *pOutput     - [out] output of extract operation
    \Input iOutputLen   - desired length of output
    \Input *pSalt       - salt input to extract
    \Input iSaltLen     - salt length
    \Input *pKey        - key input to extract
    \Input iKeyLen      - length of key
    \Input eHashType    - hash to use for extract

    \Version 03/08/2017 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLHkdfExtract(uint8_t *pOutput, int32_t iOutputLen, const uint8_t *pSalt, int32_t iSaltLen, const uint8_t *pKey, int32_t iKeyLen, CryptHashTypeE eHashType)
{
    // the RFC defines this as HMAC-Hash(salt, key) but our CryptHmac params are reversed
    CryptHmacCalc(pOutput, iOutputLen, pKey, iKeyLen, pSalt, iSaltLen, eHashType);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHkdfExpand

    \Description
        Implements HKDF-Expand as defined in rfc 5869; see
        https://tools.ietf.org/html/rfc5869#section-2.3

    \Input *pOutput     - [out] output of expand operation
    \Input iOutputLen   - desired length of output
    \Input *pKey        - key input to expand (prk)
    \Input iKeyLen      - length of key
    \Input *pInfo       - info input to expand
    \Input iInfoLen     - length of info
    \Input eHashType    - hash to use for expand

    \Version 03/08/2017 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLHkdfExpand(uint8_t *pOutput, int32_t iOutputLen, const uint8_t *pKey, int32_t iKeyLen, const uint8_t *pInfo, int32_t iInfoLen, CryptHashTypeE eHashType)
{
    uint32_t uRound, uNumRounds;
    int32_t iHashLen = CryptHashGetSize(eHashType);
    uint8_t uPrev, *pPrev;
    uint32_t uPrevLen;
    CryptHmacMsgT Message[3];
    uint8_t uCount;

    // N = ceil(L/HashLen)
    uNumRounds = (iOutputLen/ iHashLen);
    uNumRounds += iOutputLen % iHashLen != 0 ? 1 : 0;
    // T(0) = empty string (zero length)
    uPrev = 0;
    pPrev = &uPrev;
    uPrevLen = 0;
    // T(1) ... T(N)
    for (uRound = 1; uRound <= uNumRounds; uRound += 1, pOutput += iHashLen, iOutputLen -= iHashLen)
    {
        uCount = (uint8_t)uRound;

        Message[0].pMessage = pPrev;
        Message[0].iMessageLen = uPrevLen;
        Message[1].pMessage = pInfo;
        Message[1].iMessageLen = iInfoLen;
        Message[2].pMessage = &uCount;
        Message[2].iMessageLen = sizeof(uCount);
        CryptHmacCalcMulti(pOutput, iOutputLen, Message, sizeof(Message)/sizeof(Message[0]), pKey, iKeyLen, eHashType);

        pPrev = pOutput;
        uPrevLen = iHashLen;
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHkdfExpandLabel

    \Description
        Implements HKDF-Expand-Label as per
        https://tools.ietf.org/html/rfc8446#section-7.1

    \Input *pOutput     - [out] output of expand operation
    \Input iOutputLen   - desired length of output
    \Input *pKey        - key input to expand (ikm)
    \Input iKeyLen      - length of key
    \Input *pLabel      - string context label
    \Input *pHashValue  - hash data
    \Input iHashLen     - hash data length
    \Input eHashType    - hash to use for label expand

    \Output
        int32_t         - length of output

    \Version 03/08/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHkdfExpandLabel(uint8_t *pOutput, int32_t iOutputLen, const uint8_t *pKey, int32_t iKeyLen, const char *pLabel, const uint8_t *pHashValue, int32_t iHashLen, CryptHashTypeE eHashType)
{
    const char *_strLabelPrefix = "tls13 ";
    const int32_t iPrefixLen = (int32_t)strlen(_strLabelPrefix);
    int32_t iLabelLen = (int32_t)strlen(pLabel);
    uint8_t aInfoBuf[1024];
    int32_t iInfoLen = 0, iBufLen = sizeof(aInfoBuf);

    // HkdfLabel.length -- secret key length u16
    aInfoBuf[iInfoLen++] = (uint8_t)(iOutputLen >> 8);
    aInfoBuf[iInfoLen++] = (uint8_t)(iOutputLen);
    // HkdfLabelT.label -- length-prefixed string
    aInfoBuf[iInfoLen++] = (uint8_t)(iLabelLen + iPrefixLen);
    iInfoLen += ds_snzprintf((char *)aInfoBuf + iInfoLen, iBufLen - iInfoLen, "%s%s", _strLabelPrefix, pLabel);
    // HkdfLabelT.hash_value -- hash data
    aInfoBuf[iInfoLen++] = (uint8_t)iHashLen;
    ds_memcpy_s(aInfoBuf + iInfoLen, iBufLen - iInfoLen, pHashValue, iHashLen);
    iInfoLen += iHashLen;
    
    // do the expand
    if (iInfoLen <= iBufLen)
    {
        _ProtoSSLHkdfExpand(pOutput, iOutputLen, pKey, iKeyLen, aInfoBuf, iInfoLen, eHashType);
        #if DEBUG_RAW_DATA
        NetPrintMem(pKey, iKeyLen, "key");
        NetPrintMem(aInfoBuf, iInfoLen, "info");
        NetPrintMem(pOutput, iOutputLen, "output");
        #endif
    }
    else
    {
        NetPrintf(("protossl: buffer too small in Hkdf expansion\n"));
    }
    // return size of output
    return(iOutputLen);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLDeriveSecret

    \Description
        Implements Derive-Secret as per
        https://tools.ietf.org/html/rfc8446#section-7.1

    \Input *pSecure     - secure state
    \Input *pOutput     - [out] output of expand operation
    \Input iOutputLen   - desired length of output
    \Input *pKey        - key input to expand (ikm)
    \Input iKeyLen      - length of key
    \Input *pLabel      - string context label
    \Input eHashType    - hash to use for label expand
    \Input iHashLen     - hash data length (zero for empty-message hash)
    \Input *pHashData   - hash data to use (NULL to use current handshake hash)

    \Output
        int32_t         - output length of secret

    \Version 03/08/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLDeriveSecret(SecureStateT *pSecure, uint8_t *pOutput, int32_t iOutputLen, const uint8_t *pKey, int32_t iKeyLen, const char *pLabel, CryptHashTypeE eHashType, int32_t iHashLen, const uint8_t *pHashData)
{
    uint8_t aHashBuf[CRYPTHASH_MAXDIGEST];

    // zero hash length indicates a request for an empty-message hash
    if (iHashLen == 0)
    {
        const CryptHashT *pHash = CryptHashGet(eHashType);
        uint8_t aHashState[CRYPTHASH_MAXSTATE];
        iHashLen = CryptHashGetSize(eHashType);
        pHash->Init(aHashState, iHashLen);
        pHash->Final(aHashState, aHashBuf, iHashLen);
        pHashData = aHashBuf;
    }
    else if (pHashData == NULL)
    {
        // get handshake hash data
        _ProtoSSLHandshakeHashGet(pSecure, eHashType, aHashBuf, sizeof(aHashBuf));
        pHashData = aHashBuf;
    }
    #if DEBUG_RAW_DATA
    NetPrintMem(pHashData, iHashLen, "handshake hash");
    #endif

    // do the expand & return length of output
    _ProtoSSLHkdfExpandLabel(pOutput, iOutputLen, pKey, iKeyLen, pLabel, pHashData, iHashLen, eHashType);
    return(iOutputLen);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLBuildSecrets

    \Description
        Builds early, handshake, and master secrets as per
        https://tools.ietf.org/html/rfc8446#section-7.1

    \Input *pSecure     - secure state
    \Input *pHandshakeSecret - [out] buffer to hold handshake secret
    \Input *pMasterSecret - [out] buffer to hold master secret
    \Input *pKey        - ECDHE key used to build handshake/master secret
    \Input iKeyLen      - length of key
    \Input eHashType    - hash to use for label expand
    \Input iHashLen     - hash data length

    \Version 03/21/2017 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLBuildSecrets(SecureStateT *pSecure, uint8_t *pHandshakeSecret, uint8_t *pMasterSecret, uint8_t *pKey, int32_t iKeyLen, CryptHashTypeE eHashType, int32_t iHashLen)
{
    uint8_t aZeroBuf[CRYPTHASH_MAXDIGEST], aWorkBuf[CRYPTHASH_MAXDIGEST], aWorkBuf2[CRYPTHASH_MAXDIGEST];

    // create zero buf
    ds_memclr(aZeroBuf, iHashLen);

    // HKDF-Extract(PSK|0, 0) (Early Secret)
    _ProtoSSLHkdfExtract(aWorkBuf, iHashLen, aZeroBuf, iHashLen, pSecure->bSessionResume ? pSecure->PreSharedKey : aZeroBuf, iHashLen, eHashType);
    #if DEBUG_RAW_DATA
    NetPrintMem(aWorkBuf, iHashLen, "early secret");
    #endif

    // Derive-Secret(., "derived", "")
    _ProtoSSLDeriveSecret(pSecure, aWorkBuf2, iHashLen, aWorkBuf, iHashLen, "derived", eHashType, 0, NULL);
    // HKDF-Extract(salt, ECDHE)   (Handshake Secret)
    _ProtoSSLHkdfExtract(pHandshakeSecret, iHashLen, aWorkBuf2, iHashLen, pKey, iKeyLen, eHashType);
    #if DEBUG_RAW_DATA
    NetPrintMem(pHandshakeSecret, iHashLen, "handshake secret");
    #endif

    // Derive-Secret(., "derived", "")
    _ProtoSSLDeriveSecret(pSecure, aWorkBuf2, iHashLen, pHandshakeSecret, iHashLen, "derived", eHashType, 0, NULL);
    // HKDF-Extract(salt, 0)   (Master Secret)
    _ProtoSSLHkdfExtract(pMasterSecret, iHashLen, aWorkBuf2, iHashLen, aZeroBuf, iHashLen, eHashType);
    #if DEBUG_RAW_DATA
    NetPrintMem(pMasterSecret, iHashLen, "master secret");
    #endif
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLBuildHandshakeKey

    \Description
        Builds TLS1.3 secrets and keys for handshake as per
        https://tools.ietf.org/html/rfc8446#section-7.1

    \Input *pState      - protossl ref

    \Output
        int32_t         - result of curve secret function

    \Notes
        Secrets, keys, and IVs are stored in the KeyBlock of the SecureStateT
        as follows.  When more than one version of an item exists, later
        versions overwrite earlier versions in place:

            master_secret

            server_secret (early, handshake, application_traffic)
            client_secret (early, handshake, application_traffic)

            server_write_key (handshake, application)
            server_write_iv  (handshake, application)
            client_write_key (handshake, application)
            client_write_iv  (handshake, application)

            resumption_master_secret

        Currently, only EC(DHE) key exchange is supported.

    \Version 03/21/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLBuildHandshakeKey(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;
    const CryptCurveDhT *pEcc;
    CryptHashTypeE eHashType = pSecure->pCipher->uPrfType;
    int32_t iHashLen = CryptHashGetSize(eHashType);
    uint8_t *pKeyBlock = pSecure->KeyBlock;
    uint8_t aHandshakeSecret[CRYPTHASH_MAXDIGEST];
    uint8_t uZero = 0;
    CryptEccPointT PublicKey;
    int32_t iResult, iSecretSize = 0;

    // init context with key exchange private key, if we have not already done so
    if ((pEcc = _ProtoSSLEccInitContext(pSecure, pSecure->pEllipticCurve)) != NULL)
    {
        uint32_t uCryptUsecs;

        // derive EC(DHE) key for handshake traffic encryption
        pEcc->PointInit(&PublicKey, pSecure->PubKey, pSecure->uPubKeyLength);
        if ((iResult = pEcc->Secret(&pSecure->EccContext, &PublicKey, NULL, &uCryptUsecs)) > 0)
        {
            return(iResult);
        }
        iSecretSize = pEcc->PointFinal(pSecure->EccContext, NULL, TRUE, pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey));
        // done with ECC state
        pSecure->bEccContextInitialized = FALSE;

        NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (generate key for traffic secret) %dms\n", uCryptUsecs/1000));
        pSecure->uTimer += uCryptUsecs/1000;
    }

    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->PreMasterKey, iSecretSize, "ec(dhe) key");
    #endif

    // build main secrets used to derive keying secrets; we save the master secret for later generation of application key info
    _ProtoSSLBuildSecrets(pSecure, aHandshakeSecret, pKeyBlock, pSecure->PreMasterKey, iSecretSize, eHashType, iHashLen);
    pKeyBlock += iHashLen;

    // build server and client handshake secrets: https://tools.ietf.org/html/rfc8446#section-7.1

    // build server handshake secret
    pKeyBlock += _ProtoSSLDeriveSecret(pSecure, pSecure->pServerSecret = pKeyBlock, iHashLen, aHandshakeSecret, iHashLen, "s hs traffic", eHashType, iHashLen, NULL);

    // build client handshake secret
    pKeyBlock += _ProtoSSLDeriveSecret(pSecure, pSecure->pClientSecret = pKeyBlock, iHashLen, aHandshakeSecret, iHashLen, "c hs traffic", eHashType, iHashLen, NULL);

    // build server and client handshake key material: https://tools.ietf.org/html/rfc8446#section-7.3

    // [sender]_write_key = HKDF-Expand-Label(Secret, "key", "", key_length)
    pKeyBlock += _ProtoSSLHkdfExpandLabel(pSecure->pServerKey = pKeyBlock, pSecure->pCipher->uLen, pSecure->pServerSecret, iHashLen, "key", &uZero, 0, eHashType);
    // [sender]_write_iv = HKDF-Expand-Label(Secret, "iv", "", iv_length) -- NOTE that iv_length of 12 is the full iv size for the cipher, as the iv is fully derived in TLS1.3
    pKeyBlock += _ProtoSSLHkdfExpandLabel(pSecure->pServerInitVec = pKeyBlock, 12, pSecure->pServerSecret, iHashLen, "iv", &uZero, 0, eHashType);

    // [sender]_write_key = HKDF-Expand-Label(Secret, "key", "", key_length)
    pKeyBlock += _ProtoSSLHkdfExpandLabel(pSecure->pClientKey = pKeyBlock, pSecure->pCipher->uLen, pSecure->pClientSecret, iHashLen, "key", &uZero, 0, eHashType);
    // [sender]_write_iv = HKDF-Expand-Label(Secret, "iv", "", iv_length) -- see note above
    pKeyBlock += _ProtoSSLHkdfExpandLabel(pSecure->pClientInitVec = pKeyBlock, 12, pSecure->pClientSecret, iHashLen, "iv", &uZero, 0, eHashType);

    // save pointer to buffer for resumption secret, derived later
    pSecure->pResumeSecret = pKeyBlock;

    // log params for wireshark decrypt
    #if DIRTYCODE_LOGGING
    if (pState->iVerbose > 0)
    {
        char strBuf1[128], strBuf2[128];
        NetPrintf(("protossl: [%p] CLIENT_RANDOM %s %s\n", pState, ds_fmtoctstring(strBuf1, sizeof(strBuf1), pSecure->ClientRandom, sizeof(pSecure->ClientRandom)),
            ds_fmtoctstring(strBuf2, sizeof(strBuf2), pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey))));
        NetPrintf(("protossl: [%p] CLIENT_HANDSHAKE_TRAFFIC_SECRET %s %s\n", pState, ds_fmtoctstring(strBuf1, sizeof(strBuf1), pSecure->ClientRandom, sizeof(pSecure->ClientRandom)),
            ds_fmtoctstring(strBuf2, sizeof(strBuf2), pSecure->pClientSecret, iHashLen)));
        NetPrintf(("protossl: [%p] SERVER_HANDSHAKE_TRAFFIC_SECRET %s %s\n", pState, ds_fmtoctstring(strBuf1, sizeof(strBuf1), pSecure->ClientRandom, sizeof(pSecure->ClientRandom)),
            ds_fmtoctstring(strBuf2, sizeof(strBuf2), pSecure->pServerSecret, iHashLen)));
    }
    #endif

    // init secure state with handshake traffic keys
    if (pSecure->pCipher->uEnc == SSL3_ENC_GCM)
    {
        CryptGcmInit(&pSecure->ReadGcm, pState->bServer ? pSecure->pClientKey : pSecure->pServerKey, pSecure->pCipher->uLen);
        CryptGcmInit(&pSecure->WriteGcm, pState->bServer ? pSecure->pServerKey : pSecure->pClientKey, pSecure->pCipher->uLen);
    }
    else
    {
        CryptChaChaInit(&pSecure->ReadChaCha, pState->bServer ? pSecure->pClientKey : pSecure->pServerKey, pSecure->pCipher->uLen);
        CryptChaChaInit(&pSecure->WriteChaCha, pState->bServer ? pSecure->pServerKey : pSecure->pClientKey, pSecure->pCipher->uLen);
    }
    // sending and receiving secure from this point
    pSecure->bSendSecure = pSecure->bRecvSecure = TRUE;
    pSecure->uSendSeqn = pSecure->uRecvSeqn = 0;

    // done
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLBuildApplicationKey

    \Description
        Builds TLS1.3 secrets and keys for application data as per
        https://tools.ietf.org/html/rfc8446#section-7.1

    \Input *pState      - protossl ref
    \Input *pSecure     - secure state

    \Notes
        See _ProtoSSLBuildHandshakeKey notes for keyblock layout

    \Version 03/22/2017 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLBuildApplicationKey(ProtoSSLRefT *pState, SecureStateT *pSecure)
{
    CryptHashTypeE eHashType = pSecure->pCipher->uPrfType;
    int32_t iHashLen = CryptHashGetSize(eHashType);
    uint8_t uZero = 0;

    // build server application traffic secret (note this overwrites the server handshake traffic secret)
    _ProtoSSLDeriveSecret(pSecure, pSecure->pServerSecret, iHashLen, pSecure->KeyBlock, iHashLen, "s ap traffic", eHashType, iHashLen, pSecure->aFinishHash);

    // build client application traffic secret (note this overwrites the client handshake traffic secret)
    _ProtoSSLDeriveSecret(pSecure, pSecure->pClientSecret, iHashLen, pSecure->KeyBlock, iHashLen, "c ap traffic", eHashType, iHashLen, pSecure->aFinishHash);

    // log params for wireshark decrypt        
    #if DIRTYCODE_LOGGING
    if (pState->iVerbose > 0)
    {
        char strBuf1[128], strBuf2[128];
        NetPrintf(("protossl: [%p] CLIENT_TRAFFIC_SECRET_0 %s %s\n", pState, ds_fmtoctstring(strBuf1, sizeof(strBuf1), pSecure->ClientRandom, sizeof(pSecure->ClientRandom)),
            ds_fmtoctstring(strBuf2, sizeof(strBuf2), pSecure->pClientSecret, iHashLen)));
        NetPrintf(("protossl: [%p] SERVER_TRAFFIC_SECRET_0 %s %s\n", pState, ds_fmtoctstring(strBuf1, sizeof(strBuf1), pSecure->ClientRandom, sizeof(pSecure->ClientRandom)),
            ds_fmtoctstring(strBuf2, sizeof(strBuf2), pSecure->pServerSecret, iHashLen)));
    }
    #endif

    // [sender]_write_key = HKDF-Expand-Label(Secret, "key", "", key_length)
    _ProtoSSLHkdfExpandLabel(pSecure->pServerKey, pSecure->pCipher->uLen, pSecure->pServerSecret, iHashLen, "key", &uZero, 0, eHashType);
    // [sender]_write_iv = HKDF-Expand-Label(Secret, "iv", "", iv_length) -- NOTE that iv_length of 12 is the full iv size for the cipher, as the iv is fully derived in TLS1.3
    _ProtoSSLHkdfExpandLabel(pSecure->pServerInitVec, 12, pSecure->pServerSecret, iHashLen, "iv", &uZero, 0, eHashType);

    // [sender]_write_key = HKDF-Expand-Label(Secret, "key", "", key_length)
    _ProtoSSLHkdfExpandLabel(pSecure->pClientKey, pSecure->pCipher->uLen, pSecure->pClientSecret, iHashLen, "key", &uZero, 0, eHashType);
    // [sender]_write_iv = HKDF-Expand-Label(Secret, "iv", "", iv_length) -- see note above
    _ProtoSSLHkdfExpandLabel(pSecure->pClientInitVec, 12, pSecure->pClientSecret, iHashLen, "iv", &uZero, 0, eHashType);

    // build resumption master secret
    _ProtoSSLDeriveSecret(pSecure, pSecure->pResumeSecret, iHashLen, pSecure->KeyBlock, iHashLen, "res master", eHashType, iHashLen, NULL);

    #if DEBUG_RES_SESS
    NetPrintMem(pSecure->KeyBlock, iHashLen, "master secret");
    NetPrintMem(pSecure->pResumeSecret, iHashLen, "resumption_master_secret");
    #endif

    // reinit secure state with application traffic keys
    if (pSecure->pCipher->uEnc == SSL3_ENC_GCM)
    {
        CryptGcmInit(&pSecure->ReadGcm, pState->bServer ? pSecure->pClientKey : pSecure->pServerKey, pSecure->pCipher->uLen);
        CryptGcmInit(&pSecure->WriteGcm, pState->bServer ? pSecure->pServerKey : pSecure->pClientKey, pSecure->pCipher->uLen);
    }
    else
    {
        CryptChaChaInit(&pSecure->ReadChaCha, pState->bServer ? pSecure->pClientKey : pSecure->pServerKey, pSecure->pCipher->uLen);
        CryptChaChaInit(&pSecure->WriteChaCha, pState->bServer ? pSecure->pServerKey : pSecure->pClientKey, pSecure->pCipher->uLen);
    }
    // sending and receiving secure from this point
    pSecure->bSendSecure = pSecure->bRecvSecure = TRUE;
    pSecure->uSendSeqn = pSecure->uRecvSeqn = 0;
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLCalcResumeBinder

    \Description
        Calculate the psk binder as per
        https://tools.ietf.org/html/rfc8446#section-4.2.11.2

    \Input *pState      - protossl ref
    \Input *pBuffer     - [out] buffer to store calculated binder
    \Input iBufLen      - size of output buffer
    \Input *pSessTick   - session ticket
    \Input *pHshkMsgBuf - handshake message buffer
    \Input iHshkMsgLen  - length of handshake message
    \Input iHashSize    - binder hash size

    \Output
        uint8_t *       - pointer past end of binder

    \Version 12/19/2017 (jbrookes)
*/
/********************************************************************************F*/
static uint8_t *_ProtoSSLCalcResumeBinder(ProtoSSLRefT *pState, uint8_t *pBuffer, int32_t iBufLen, const SessionTicketT *pSessTick, const uint8_t *pHshkMsgBuf, int32_t iHshkMsgLen, int32_t iHashSize)
{
    uint8_t aZeroBuf[CRYPTHASH_MAXDIGEST], aWorkBuf[CRYPTHASH_MAXDIGEST], aWorkBuf2[CRYPTHASH_MAXDIGEST], aBinderKey[CRYPTHASH_MAXDIGEST];
    uint8_t aHash[CRYPTHASH_MAXDIGEST], aHashCtx[CRYPTHASH_MAXSTATE];
    uint8_t aMsgHead[] = { SSL3_MSG_CLIENT_HELLO, 0, 0, 0 };
    CryptSha2T *pBinderCtx = (CryptSha2T *)aHashCtx;
    SecureStateT *pSecure = pState->pSecure;

    // calculate buffer length - this includes the binders we haven't written out yet
    iBufLen = iHshkMsgLen + iHashSize + 3;
    // set in clienthello header
    aMsgHead[2] = (uint8_t)(iBufLen>>8);
    aMsgHead[3] = (uint8_t)(iBufLen>>0);

    // create zero buf
    ds_memclr(aZeroBuf, iHashSize);

    // if we're in a HelloRetryRequest flow, we need to include the original [ClientHello...HelloRetryRequest] in the binder hash
    if (pSecure->bHelloRetry)
    {
        // get handshake context
        CryptSha2T *pHandshakeCtx = (pSessTick->eHashType == CRYPTHASH_SHA256) ? &pSecure->HandshakeSHA256 : &pSecure->HandshakeSHA384;
        // copy to binder hash
        ds_memcpy_s(pBinderCtx, sizeof(*pBinderCtx), pHandshakeCtx, sizeof(*pHandshakeCtx));
    }
    else
    {
        // init binder hash
        CryptSha2Init(pBinderCtx, iHashSize);
    }
    // hash the ClientHello message header
    CryptSha2Update(pBinderCtx, aMsgHead, sizeof(aMsgHead));
    // hash the ClientHello message body up to binders list
    CryptSha2Update(pBinderCtx, pHshkMsgBuf, iHshkMsgLen);
    // get binder hash
    CryptSha2Final(pBinderCtx, aHash, iHashSize);

    #if DEBUG_RES_SESS
    #if DEBUG_RAW_DATA
    NetPrintMem(aMsgHead, sizeof(aMsgHead), "message head");
    NetPrintMem(pHshkMsgBuf, iHshkMsgLen, "message body");
    #endif
    NetPrintMem(aHash, iHashSize, "binder hash");
    #endif

    // calculate the binder

    // resumption_key = HKDF-Expand-Label(resumption_key, "resumption", ticket_nonce, ticket_nonce.len)
    _ProtoSSLHkdfExpandLabel(pSecure->PreSharedKey, iHashSize, pSessTick->aResumeKey, iHashSize, "resumption", pSessTick->aTicketNonce, pSessTick->uNonceLen, pSessTick->eHashType);
    // HKDF-Extract(0, PSK) (Early Secret)
    _ProtoSSLHkdfExtract(aWorkBuf, iHashSize, aZeroBuf, iHashSize, pSecure->PreSharedKey, iHashSize, pSessTick->eHashType);
    // Derive-Secret(., "res binder", "")
    _ProtoSSLDeriveSecret(pSecure, aWorkBuf2, iHashSize, aWorkBuf, iHashSize, "res binder", pSessTick->eHashType, 0, NULL);
    // finished_key = HKDF-Expand-Label(binder_key, "finished", "", Hash.length)
    _ProtoSSLHkdfExpandLabel(aBinderKey, iHashSize, aWorkBuf2, iHashSize, "finished", NULL, 0, pSessTick->eHashType);
    // verify_data = HMAC(finished_key, Hash(ClientHello*)
    _ProtoSSLHkdfExtract(pBuffer, iHashSize, aBinderKey, iHashSize, aHash, iHashSize, pSessTick->eHashType);

    #if DEBUG_RES_SESS
    NetPrintMem(pSecure->PreSharedKey, iHashSize, "PSK");
    NetPrintMem(aWorkBuf, iHashSize, "early_secret");
    NetPrintMem(aWorkBuf2, iHashSize, "binder key");
    NetPrintMem(aBinderKey, iHashSize, "finished key");
    NetPrintMem(pBuffer, iHashSize, "verify_data");
    #endif

    // return pointer past end of binder   
    return(pBuffer+iHashSize);
}

/*
    encryption and decryption
*/

/*F********************************************************************************/
/*!
    \Function _ProtoSSLAesEncrypt

    \Description
        Encrypt data with AES

    \Input *pSecure     - secure state
    \Input *pSend       - data to encrypt
    \Input iSize        - size of data to encrypt

    \Output
        int32_t         - size of encrypted data

    \Version 01/14/2018 (jbrookes) Refactored from _ProtoSSLSendPacket()
*/
/********************************************************************************F*/
static int32_t _ProtoSSLAesEncrypt(SecureStateT *pSecure, uint8_t *pSend, int32_t iSize)
{
    int32_t iPadBytes;

    // calculate padding
    if ((iPadBytes = 16 - (iSize % 16)) == 0)
    {
        iPadBytes = 16;
    }

    // set the padding data
    ds_memset(pSend+iSize, iPadBytes-1, iPadBytes);
    iSize += iPadBytes;

    // fill in the explict IV for TLS1.1
    if (pSecure->uSslVersion >= SSL3_TLS1_1)
    {
        pSend -= 16;
        CryptRandGet(pSend, 16);
        #if DEBUG_RAW_DATA
        NetPrintMem(pSend, 16, "explicit IV");
        #endif
        iSize += 16;
    }

    // do the encryption
    CryptAesEncrypt(&pSecure->WriteAes, pSend, iSize);

    // return encrypted size to caller
    return(iSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLAesDecrypt

    \Description
        Decrypt data with AES

    \Input *pSecure     - secure state
    \Input *pRecv       - data to decrypt
    \Input iSize        - size of data to decrypt
    \Input *pBadMac     - [out] bad mac flag

    \Output
        int32_t         - size of decrypted data, or -1 on error

    \Version 01/14/2018 (jbrookes) Refactored from _RecvPacket()
*/
/********************************************************************************F*/
static int32_t _ProtoSSLAesDecrypt(SecureStateT *pSecure, uint8_t *pRecv, int32_t iSize, uint8_t *pBadMac)
{
    int32_t iPad, iPadBytes, iPadStart;

    // decrypt the data
    CryptAesDecrypt(&pSecure->ReadAes, pRecv, iSize);

    // if TLS1.1 or greater, skip explicit IV
    if ((pSecure->uSslVersion >= SSL3_TLS1_1) && (pSecure->iRecvSize >= 16))
    {
        pSecure->iRecvBase += 16;
        pRecv += 16;
        iSize -= 16;
    }

    // read number of pad bytes
    iPadBytes = pRecv[iSize-1];

    /* As per http://tools.ietf.org/html/rfc5246#section-6.2.3.2, padding may be up to 255 bytes
       in length.  Each uint8 in the padding data vector MUST be filled with the padding length
       value, padding MUST be checked, and a padding error MUST result in a bad_record_mac
       alert.  To eliminate a possible timing attack, we note the error here but wait until
       after the MAC is generated to report it. */
    for (iPad = 0, iPadStart = iSize-iPadBytes-1; iPad < iPadBytes; iPad += 1)
    {
        if (pRecv[iPadStart+iPad] != iPadBytes)
        {
            #if DIRTYCODE_LOGGING
            if (!(*pBadMac))
            {
                NetPrintf(("protossl: _ProtoSSLAesDecrypt bad padding (len=%d)\n", iPadBytes));
                NetPrintMem(pRecv+iSize-iPadBytes-1, iPadBytes, "_ProtoSSLAesDecrypt padding");
            }
            #endif
            *pBadMac = TRUE;
            iSize = -1;
            break;
        }
    }

    // remove pad if validation was successful
    if (!(*pBadMac))
    {
        iSize -= iPadBytes+1;
    }
    // return size to caller
    return(iSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLAeadGenerateNonce

    \Description
        Generate AEAD Nonce (IV)

    \Input *pSecure     - secure state
    \Input *pBuffer     - [out] storage for generated nonce
    \Input uSeqn        - sequence number
    \Input *pSeqn       - sequence pointer (NULL to use sequence number)
    \Input *pInitVec    - implicit initialization vector (IV)

    \Version 07/08/2014 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLAeadGenerateNonce(SecureStateT *pSecure, uint8_t *pBuffer, uint64_t uSeqn, const uint8_t *pSeqn, const uint8_t *pInitVec)
{
    if ((pSecure->uSslVersion < PROTOSSL_VERSION_TLS1_3) && (pSecure->pCipher->uEnc != SSL3_ENC_CHACHA))
    {
        *pBuffer++ = *pInitVec++;
        *pBuffer++ = *pInitVec++;
        *pBuffer++ = *pInitVec++;
        *pBuffer++ = *pInitVec++;
        if (pSeqn == NULL)
        {
            *pBuffer++ = (uint8_t)((uSeqn>>56)&255);
            *pBuffer++ = (uint8_t)((uSeqn>>48)&255);
            *pBuffer++ = (uint8_t)((uSeqn>>40)&255);
            *pBuffer++ = (uint8_t)((uSeqn>>32)&255);
            *pBuffer++ = (uint8_t)((uSeqn>>24)&255);
            *pBuffer++ = (uint8_t)((uSeqn>>16)&255);
            *pBuffer++ = (uint8_t)((uSeqn>>8)&255);
            *pBuffer++ = (uint8_t)((uSeqn>>0)&255);
        }
        else
        {
            ds_memcpy(pBuffer, pSeqn, 8);
        }
    }
    else
    {
        /* tls1.3 (and chacha) nonce generated as per: https://tools.ietf.org/html/rfc8446#section-5.3;
           the 64-bit record number is extended to iv_length, and xor'd with iv */
        uint32_t uCount;
        pBuffer[ 0] = 0;
        pBuffer[ 1] = 0;
        pBuffer[ 2] = 0;
        pBuffer[ 3] = 0;
        pBuffer[ 4] = (uint8_t)((uSeqn>>56)&255);
        pBuffer[ 5] = (uint8_t)((uSeqn>>48)&255);
        pBuffer[ 6] = (uint8_t)((uSeqn>>40)&255);
        pBuffer[ 7] = (uint8_t)((uSeqn>>32)&255);
        pBuffer[ 8] = (uint8_t)((uSeqn>>24)&255);
        pBuffer[ 9] = (uint8_t)((uSeqn>>16)&255);
        pBuffer[10] = (uint8_t)((uSeqn>>8)&255);
        pBuffer[11] = (uint8_t)((uSeqn>>0)&255);
        for (uCount = 0; uCount < 12; uCount += 1)
        {
            pBuffer[uCount] ^= pInitVec[uCount];
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLAeadEncrypt

    \Description
        Encrypt data with an AEAD cipher

    \Input *pSecure     - secure state
    \Input *pSend       - data to encrypt
    \Input iSize        - size of data to encrypt
    \Input bServer      - TRUE if server, else FALSE

    \Output
        int32_t         - size of encrypted data

    \Version 01/14/2018 (jbrookes) Refactored from _ProtoSSLSendPacket()
*/
/********************************************************************************F*/
static int32_t _ProtoSSLAeadEncrypt(SecureStateT *pSecure, uint8_t *pSend, int32_t iSize, uint8_t bServer)
{
    uint8_t AeadData[13], AeadNonce[12], AeadTag[16];
    int32_t iAeadDataSize;

    // change content type to application_data, append content type to output
    if (pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3)
    {
        uint8_t uContentType = pSecure->SendData[0];
        pSecure->SendData[0] = SSL3_REC_APPLICATION;
        pSend[iSize++] = uContentType;
    }

    // generate AEAD additional data
    if (pSecure->uSslVersion < PROTOSSL_VERSION_TLS1_3)
    {
        // generate aead data (13 bytes, matches TLS MAC)
        _ProtoSSLGenerateMacSource(AeadData, pSecure->uSendSeqn, pSecure->SendData[0], pSecure->uSslVersion, (uint32_t)iSize);
        iAeadDataSize = sizeof(AeadData);
    }
    else
    {
        /* as per https://tools.ietf.org/html/rfc8446#section-5.2, tls1.3 AEAD data is the record header.
           we already have the first three bytes of the record header formatted in SendData but send length isn't written
           yet, so we calculate it here as the input size plus the tag size */
        int32_t iSendSize = iSize + sizeof(AeadTag);
        AeadData[0] = pSecure->SendData[0];
        AeadData[1] = pSecure->SendData[1];
        AeadData[2] = pSecure->SendData[2];
        AeadData[3] = (uint8_t)(iSendSize>>8);
        AeadData[4] = (uint8_t)(iSendSize>>0);
        iAeadDataSize = 5;
        #if DEBUG_RAW_DATA
        NetPrintMem(AeadData, iAeadDataSize, "AeadData-send");
        #endif
    }

    // generate nonce
    if ((pSecure->uSslVersion < PROTOSSL_VERSION_TLS1_3) && (pSecure->pCipher->uEnc != SSL3_ENC_CHACHA))
    {
        // generate nonce (4 bytes implicit salt from the IV; 8 bytes explicit, we use sequence)
        _ProtoSSLAeadGenerateNonce(pSecure, AeadNonce, pSecure->uSendSeqn, NULL, bServer ? pSecure->pServerInitVec : pSecure->pClientInitVec);
        // write explicit IV to output; this is not encrypted
        ds_memcpy(pSend-8, AeadNonce+4, 8);
        #if DEBUG_RAW_DATA
        NetPrintMem(pSend-8, 8, "explicit IV");
        #endif
    }
    else
    {
        // generate tls1.3/chacha nonce
        _ProtoSSLAeadGenerateNonce(pSecure, AeadNonce, pSecure->uSendSeqn, NULL, bServer ? pSecure->pServerInitVec : pSecure->pClientInitVec);
    }

    // do the encryption
    if (pSecure->pCipher->uEnc == SSL3_ENC_GCM)
    {
        iSize = CryptGcmEncrypt(&pSecure->WriteGcm, pSend, iSize, AeadNonce, sizeof(AeadNonce), AeadData, iAeadDataSize, AeadTag, sizeof(AeadTag));
    }
    else
    {
        iSize = CryptChaChaEncrypt(&pSecure->WriteChaCha, pSend, iSize, AeadNonce, sizeof(AeadNonce), AeadData, iAeadDataSize, AeadTag, sizeof(AeadTag));
    }

    // append tag to output
    ds_memcpy(pSend+iSize, AeadTag, sizeof(AeadTag));
    iSize += sizeof(AeadTag);

    // add explicit IV to size *after* encrypt+tag
    if ((pSecure->uSslVersion < PROTOSSL_VERSION_TLS1_3) && (pSecure->pCipher->uEnc != SSL3_ENC_CHACHA))
    {
        iSize += 8;
    }

    // return encrypted size to caller
    return(iSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLAeadDecrypt

    \Description
        Decrypt data with an AEAD cipher

    \Input *pSecure     - secure state
    \Input *pRecv       - data to decrypt
    \Input iSize        - size of data to decrypt
    \Input bServer      - TRUE if server, else FALSE
    \Input *pBadMac     - [out] bad mac flag
    \Input *pAlert      - [out] alert, on error

    \Output
        int32_t         - size of decrypted data, or -1 on error

    \Version 01/14/2018 (jbrookes) Refactored from _RecvPacket()
*/
/********************************************************************************F*/
static int32_t _ProtoSSLAeadDecrypt(SecureStateT *pSecure, uint8_t *pRecv, int32_t iSize, uint8_t bServer, uint8_t *pBadMac, int32_t *pAlert)
{
    uint8_t AeadData[13], AeadNonce[12];
    int32_t iAeadDataSize;

    // generate nonce
    if ((pSecure->uSslVersion < PROTOSSL_VERSION_TLS1_3) && (pSecure->pCipher->uEnc != SSL3_ENC_CHACHA))
    {
        // generate nonce (4 bytes implicit salt from TLS IV; 8 bytes explicit from packet)
        _ProtoSSLAeadGenerateNonce(pSecure, AeadNonce, 0, pRecv, bServer ? pSecure->pClientInitVec : pSecure->pServerInitVec);

        // skip explicit IV in packet data
        pSecure->iRecvBase += 8;
        pRecv += 8;
    }
    else 
    {
        // generate tls1.3/chacha nonce
        _ProtoSSLAeadGenerateNonce(pSecure, AeadNonce, pSecure->uRecvSeqn, NULL, bServer ? pSecure->pClientInitVec : pSecure->pServerInitVec);
    }

    // remove authentication tag
    pSecure->iRecvSize -= 16;
    pSecure->iRecvProg = pSecure->iRecvSize;

    // recalculate size
    iSize = pSecure->iRecvSize - pSecure->iRecvBase;

    // generate AEAD additional data
    if (pSecure->uSslVersion < PROTOSSL_VERSION_TLS1_3)
    {
        // generate aead data (13 bytes, matches TLS MAC)
        _ProtoSSLGenerateMacSource(AeadData, pSecure->uRecvSeqn, pSecure->RecvHead[0], pSecure->uSslVersion, (uint32_t)iSize);
        iAeadDataSize = sizeof(AeadData);
    }
    else
    {
        // as per https://tools.ietf.org/html/rfc8446#section-5.2, tls1.3 AEAD data is the record header
        iAeadDataSize = sizeof(pSecure->RecvHead);
        ds_memcpy_s(AeadData, sizeof(AeadData), pSecure->RecvHead, iAeadDataSize);
        #if DEBUG_RAW_DATA
        NetPrintMem(AeadData, iAeadDataSize, "AeadData-recv");
        #endif
    }

    // decrypt data
    if (pSecure->pCipher->uEnc == SSL3_ENC_GCM)
    {
        iSize = CryptGcmDecrypt(&pSecure->ReadGcm, pRecv, iSize, AeadNonce, sizeof(AeadNonce), AeadData, iAeadDataSize, pRecv+iSize, 16);
    }
    else
    {
         iSize = CryptChaChaDecrypt(&pSecure->ReadChaCha, pRecv, iSize, AeadNonce, sizeof(AeadNonce), AeadData, iAeadDataSize, pRecv+iSize, 16);
    }

    /* as per http://tools.ietf.org/html/rfc5288#section-3: Implementations MUST send TLS Alert bad_record_mac for all types of
       failures encountered in processing the AES-GCM algorithm; this gets handled later in the receive flow */
    if (iSize < 0)
    {
        NetPrintf(("protossl: aead decrypt of received data failed\n"));
        *pBadMac = TRUE;
    }

    // for tls1.3, handle padding and content-type extraction
    if ((pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3) && !(*pBadMac))
    {
        // handle record padding as per https://tools.ietf.org/html/rfc8446#section-5.4
        for ( ; (iSize > 0) && (pRecv[iSize-1] == 0); iSize -= 1)
            ;
        // read and overwrite content type
        if (iSize > 0)
        {
            pSecure->iRecvSize = pSecure->iRecvProg = pSecure->iRecvBase+iSize-1;
            pSecure->RecvHead[0] = pSecure->RecvData[pSecure->iRecvSize];
        }
        else
        {
            /* Implementations MUST limit their scanning to the cleartext returned from the AEAD decryption.  If a receiving implementation
               does not find a non-zero octet in the cleartext, it MUST terminate the connection with an "unexpected_message" alert. */
            NetPrintf(("protossl: _ProtoSSLAeadDecrypt: no content-type included in message\n"));
            *pAlert = SSL3_ALERT_DESC_UNEXPECTED_MESSAGE;
            iSize = -1;
        }
    }
    // return size to caller
    return(iSize);
}

/*
    secure packet send and receive
*/

/*F********************************************************************************/
/*!
    \Function _ProtoSSLSendSecure

    \Description
        Send secure queued data

    \Input *pState      - module state
    \Input *pSecure     - secure state

    \Output
        int32_t         - zero if nothing was sent, else one

    \Version 11/07/2013 (jbrookes) Refactored from _ProtoSSLUpdateSend()
*/
/********************************************************************************F*/
static int32_t _ProtoSSLSendSecure(ProtoSSLRefT *pState, SecureStateT *pSecure)
{
    int32_t iResult, iXfer = 0;

    // see if there is data to send
    if (pSecure->iSendProg < pSecure->iSendSize)
    {
        // try to send
        iResult = SocketSend(pState->pSock, (char *)pSecure->SendData+pSecure->iSendProg, pSecure->iSendSize-pSecure->iSendProg, 0);
        if (iResult > 0)
        {
            pSecure->iSendProg += iResult;
            iXfer = 1;
        }
        if (iResult < 0)
        {
            pState->iState = (pState->iState < ST3_SECURE) ? ST_FAIL_SETUP : ST_FAIL_SECURE;
            pState->iClosed = 1;
        }
        // see if the data can be cleared
        if (pSecure->iSendProg == pSecure->iSendSize)
        {
            pSecure->iSendProg = pSecure->iSendSize = 0;
        }
    }

    // return if something was sent
    return(iXfer);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLSendPacket

    \Description
        Build an outgoing SSL packet. Accepts head/body buffers to allow easy
        contruction of header / data in individual buffers (or set one to null
        if data already combind).

    \Input *pState   - ssl state ptr
    \Input uType     - record type
    \Input *pHeadPtr - pointer to head buffer
    \Input iHeadLen  - length of head buffer
    \Input *pBodyPtr - pointer to data to send
    \Input iBodyLen  - length of data to send

    \Output int32_t - -1=invalid length, zero=no error

    \Version 11/10/2005 gschaefer
*/
/********************************************************************************F*/
static int32_t _ProtoSSLSendPacket(ProtoSSLRefT *pState, uint8_t uType, const void *pHeadPtr, int32_t iHeadLen, const void *pBodyPtr, int32_t iBodyLen)
{
    SecureStateT *pSecure = pState->pSecure;
    uint32_t uRecordVersion;
    uint8_t *pSend;
    int32_t iSize;

    // verify if the input buffer length is good
    if ((iHeadLen + iBodyLen + SSL_SNDOVH_PACKET) > (signed)sizeof(pSecure->SendData))
    {
        NetPrintf(("protossl: _ProtoSSLSendPacket: buffer overflow (iHeadLen=%d, iBodyLen=%d)\n", iHeadLen, iBodyLen));
        return(-1);
    }

    // setup record frame
    pSecure->SendData[0] = uType;
    // get ssl version to embed in header; as per https://tools.ietf.org/html/rfc8446#section-5.1 the record layer version is frozen at 1.2 for TLS1.3+
    uRecordVersion = (pSecure->uSslVersion < SSL3_TLS1_3) ? pSecure->uSslVersion : SSL3_TLS1_2;
    pSecure->SendData[1] = (uint8_t)(uRecordVersion>>8);
    pSecure->SendData[2] = (uint8_t)(uRecordVersion>>0);

    // point to data area
    pSend = pSecure->SendData+5;
    iSize = 0;

    /* reserve space for explicit IV if we're TLS1.1 or greater and are using AES (block cipher).  reserving
       space here means we don't have to shuffle packet data around later.  note that the IV is *not* included
       in calculating the handshake data for the finish packet */
    if (pSecure->bSendSecure && (pSecure->pCipher != NULL) && (pSecure->pCipher->uEnc == SSL3_ENC_AES) && (pSecure->uSslVersion >= SSL3_TLS1_1))
    {
        pSend += 16;
    }
    // reserve space for explicit IV for GCM (TLS1.2 and lower)
    if (pSecure->bSendSecure && (pSecure->pCipher != NULL) && (pSecure->pCipher->uEnc == SSL3_ENC_GCM) && (pSecure->uSslVersion < SSL3_TLS1_3))
    {
        pSend += 8;
    }

    // copy over the head
    ds_memcpy(pSend+iSize, pHeadPtr, iHeadLen);
    iSize += iHeadLen;

    // copy over the body
    ds_memcpy(pSend+iSize, pBodyPtr, iBodyLen);
    iSize += iBodyLen;

    // hash handshake data for "finish" packet
    if (uType == SSL3_REC_HANDSHAKE)
    {
        _ProtoSSLHandshakeHashUpdate(pSecure, pSend, iSize, "_ProtoSSLSendPacket");
    }

    // handle encryption
    if (pSecure->bSendSecure && (pSecure->pCipher != NULL))
    {
        // add MAC for non-AEAD ciphers
        if (pSecure->pCipher->uMacType != CRYPTHASH_NULL)
        {
            iSize = _ProtoSSLGenerateMac(pSecure, pSend, iSize, pState->bServer);
        }

        NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: _ProtoSSLSendPacket (secure enc=%d mac=%d): type=%d, size=%d, seq=%qd\n",
            pSecure->pCipher->uEnc, pSecure->pCipher->uMac, pSecure->SendData[0], iSize, pSecure->uSendSeqn));
        #if (DEBUG_RAW_DATA > 1)
        NetPrintMem(pSecure->SendData, iSize+5, "_ProtoSSLSendPacket");
        #endif

        // encrypt the data
        if (pSecure->pCipher->uEnc == SSL3_ENC_AES)
        {
            iSize = _ProtoSSLAesEncrypt(pSecure, pSend, iSize);
        }
        if ((pSecure->pCipher->uEnc == SSL3_ENC_GCM) || (pSecure->pCipher->uEnc == SSL3_ENC_CHACHA))
        {
            iSize = _ProtoSSLAeadEncrypt(pSecure, pSend, iSize, pState->bServer);
        }
    }
    else
    {
        NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: _ProtoSSLSendPacket (unsecure): type=%d, size=%d, seq=%qd\n", pSecure->SendData[0], iSize, pSecure->uSendSeqn));
        #if (DEBUG_RAW_DATA > 1)
        NetPrintMem(pSecure->SendData, iSize+5, "_ProtoSSLSendPacket");
        #endif
    }

    // setup total record size
    pSecure->SendData[3] = (uint8_t)(iSize>>8);
    pSecure->SendData[4] = (uint8_t)(iSize>>0);

    // setup buffer pointers
    pSecure->iSendProg = 0;
    pSecure->iSendSize = iSize+5;

    // increment the sequence
    pSecure->uSendSeqn += 1;
    return(0);
}

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvGetRecordTypeName

    \Description
        Get debug ssl record type name

    \Input uRecordType  - tls record type

    \Output
        const char *    - pointer to record type name, or "unknown" if not recognized

    \Version 11/15/2017 (jbrookes)
*/
/********************************************************************************F*/
static const char *_ProtoSSLRecvGetRecordTypeName(uint32_t uRecordType)
{
    static const char *_ContentTypeNames[5] = { "ChangeCipherSpec", "Alert", "Handshake", "Application", "Heartbeat" };
    const char *pContentType = ((uRecordType >= 20) && (uRecordType <= 24)) ? _ContentTypeNames[uRecordType-20] : "unknown";
    return(pContentType);
}
#endif

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvGetHandshakeTypeName

    \Description
        Get debug ssl handshake type name

    \Input uHandshakeType  - tls record type

    \Output
        const char *        - pointer to handshake type name, or "unknown" if not recognized

    \Version 05/03/2019 (jbrookes)
*/
/********************************************************************************F*/
static const char *_ProtoSSLRecvGetHandshakeTypeName(uint32_t uHandshakeType)
{
    static const char *_HandshakeTypeNames[] =
    {
        "hello_request", "client_hello", "server_hello", "hello_verify_request",
        "new_session_ticket", "end_of_early_data", "hello_retry_request", "unassigned_7", 
        "encrypted_extensions", "unassigned_9", "unassigned_10", "certificate",
        "server_key_exchange", "certificate_request", "server_hello_done", "certificate_verify",
        "client_key_exchange", "unassigned_17", "unassigned_18", "unassigned_19",
        "finished", "certificate_url", "certificate_status", "supplemental_data",
        "key_update",
        "message_hash"
    };
    const char *pHandshakeType;
    if (uHandshakeType <= SSL3_MSG_KEY_UPDATE)
    {
        pHandshakeType = _HandshakeTypeNames[uHandshakeType];
    }
    else if (uHandshakeType == SSL3_MSG_MESSAGE_HASH)
    {
        pHandshakeType = _HandshakeTypeNames[SSL3_MSG_KEY_UPDATE+1];
    }
    else
    {
        pHandshakeType = "unknown";
    }
    return(pHandshakeType);
}
#endif

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvReset

    \Description
        Reset receive tracking; called when we're done processing a packet or
        in an error condition

    \Input *pSecure - secure state

    \Version 11/14/2017 (jbrookes)
*/
/********************************************************************************F*/
static inline void _ProtoSSLRecvReset(SecureStateT *pSecure)
{
    pSecure->iRecvHead = pSecure->iRecvProg = pSecure->iRecvSize = pSecure->iRecvBase = pSecure->iRecvHshkProg = 0;
    pSecure->bRecvProc = FALSE;
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvPacket

    \Description
        Decode ssl3 record header, decrypt data, verify mac.

    \Input *pState  - ssl state ptr
    \Input *pAlert  - [out] alert, on error

    \Output
        int32_t     - 0 for success, negative for error

    \Version 11/10/2005 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLRecvPacket(ProtoSSLRefT *pState, int32_t *pAlert)
{
    SecureStateT *pSecure = pState->pSecure;
    int32_t iSize = pSecure->iRecvSize-pSecure->iRecvBase;
    uint8_t bBadMac = FALSE;

    /* check the record type - as per http://tools.ietf.org/html/rfc5246#section-6, if a TLS implementation
       receives an unexpected record type, it MUST send an unexpected_message alert */
    if ((pSecure->RecvHead[0] < SSL3_REC_CIPHER) || (pSecure->RecvHead[0] > SSL3_REC_APPLICATION))
    {
        NetPrintf(("protossl: _RecvPacket: unknown record type (%s) %d\n", _ProtoSSLRecvGetRecordTypeName(pSecure->RecvHead[0]), pSecure->RecvHead[0]));
        *pAlert = SSL3_ALERT_DESC_UNEXPECTED_MESSAGE;
        return(-1);
    }

    /* as per https://tools.ietf.org/html/rfc8446#section-5 check for change_cipher_spec record; if 
       we find one, we ignore it */
    if ((pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3) && (pSecure->RecvHead[0] == SSL3_REC_CIPHER))
    {
        // validate ccs message
        if ((iSize != 1) || (pSecure->RecvData[0] != 1))
        {
            NetPrintf(("protossl: invalid change_cipher_spec message in tls1.3 flow\n"));
            *pAlert = SSL3_ALERT_DESC_DECODE_ERROR;
            return(-1);
        }
        else
        {
            NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: skipping dummy change_cipher_spec message in tls1.3 flow\n"));
            pSecure->iRecvHead = pSecure->iRecvProg = pSecure->iRecvSize = pSecure->iRecvBase = pSecure->iRecvHshkProg = 0;
            return(0);
        }
    }

    // if we are reciving the finished packet, we need to transition to secure receiving
    if (pState->iState == ST3_RECV_FINISH)
    {
        pSecure->bRecvSecure = TRUE;
    }

    // handle decryption
    if (pSecure->bRecvSecure && (pSecure->pCipher != NULL))
    {
        // decrypt the data
        if (pSecure->pCipher->uEnc == SSL3_ENC_AES)
        {
            iSize = _ProtoSSLAesDecrypt(pSecure, pSecure->RecvData+pSecure->iRecvBase, iSize, &bBadMac);
        }
        if ((pSecure->pCipher->uEnc == SSL3_ENC_GCM) || (pSecure->pCipher->uEnc == SSL3_ENC_CHACHA))
        {
            iSize = _ProtoSSLAeadDecrypt(pSecure, pSecure->RecvData+pSecure->iRecvBase, iSize, pState->bServer, &bBadMac, pAlert);
        }

        // validate MAC for non-AEAD ciphers
        if (pSecure->pCipher->uMacType != CRYPTHASH_NULL)
        {
            iSize = _ProtoSSLVerifyMac(pSecure, iSize, pState->bServer, &bBadMac);
        }

        // handle mac/decrypt errors
        if (iSize < 0)
        {
            if (bBadMac)
            {
                NetPrintf(("protossl: _RecvPacket: bad MAC!\n"));
                *pAlert = SSL3_ALERT_DESC_BAD_RECORD_MAC;
            }
            return(-1);
        }

        NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: _RecvPacket (secure enc=%d mac=%d): type=%s, size=%d, seq=%qd\n",
            pSecure->pCipher->uEnc, pSecure->pCipher->uMac, _ProtoSSLRecvGetRecordTypeName(pSecure->RecvHead[0]), iSize, pSecure->uRecvSeqn));
        #if (DEBUG_RAW_DATA > 1)
        NetPrintMem(pSecure->RecvData, pSecure->iRecvSize, "_RecvPacket");
        #endif
    }
    else
    {
        NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: _RecvPacket (unsecure): type=%s, size=%d, seq=%qd\n", _ProtoSSLRecvGetRecordTypeName(pSecure->RecvHead[0]), iSize, pSecure->uRecvSeqn));
        #if (DEBUG_RAW_DATA > 1)
        NetPrintMem(pSecure->RecvData+pSecure->iRecvBase, iSize, "_RecvPacket");
        #endif
    }

    // increment the sequence number
    pSecure->uRecvSeqn += 1;

    /* check for empty packet; some implementations of SSL emit a zero-length frame followed immediately
       by an n-length frame when TLS1.0 is employed with a CBC cipher like AES.  this is done as a defense
       against the BEAST attack.  we need to detect and clear the empty packet here so the following code
       doesn't use the old packet header with new data */
    if (pSecure->iRecvSize == 0)
    {
        NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: detected empty packet\n"));
        pSecure->iRecvHead = 0;
    }

    // return success
    return(0);
}

/*
    alerts
*/

/*F********************************************************************************/
/*!
    \Function _ProtoSSLGetAlert

    \Description
        Get alert from table based on alert type

    \Input *pState          - module state
    \Input *pAlertDesc      - [out] storage for alert description
    \Input uAlertLevel      - alert level
    \Input uAlertType       - alert type

    \Output
        int32_t             - 0=no alert, 1=alert

    \Version 10/31/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLGetAlert(ProtoSSLRefT *pState, ProtoSSLAlertDescT *pAlertDesc, uint8_t uAlertLevel, uint8_t uAlertType)
{
    int32_t iErr, iResult = 0;

    // set up default alertdesc
    pAlertDesc->iAlertType = uAlertType;
    pAlertDesc->pAlertDesc = "unknown";

    // find alert description
    if (uAlertLevel != 0)
    {
        for (iErr = 0; _ProtoSSL_AlertList[iErr].iAlertType != -1; iErr += 1)
        {
            if (_ProtoSSL_AlertList[iErr].iAlertType == uAlertType)
            {
                pAlertDesc->pAlertDesc = _ProtoSSL_AlertList[iErr].pAlertDesc;
                iResult = 1;
                break;
            }
        }
    }
    return(iResult);
}

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function _ProtoSSLDebugAlert

    \Description
        Debug-only display of info following server alert

    \Input *pState      - module state
    \Input uAlertLevel  - alert level
    \Input uAlertType   - alert type

    \Version 04/04/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLDebugAlert(ProtoSSLRefT *pState, uint8_t uAlertLevel, uint8_t uAlertType)
{
    ProtoSSLAlertDescT Alert;
    _ProtoSSLGetAlert(pState, &Alert, uAlertLevel, uAlertType);
    NetPrintf(("protossl: ALERT: level=%d, type=%d, name=%s\n", uAlertLevel, uAlertType, Alert.pAlertDesc));
}
#else
#define _ProtoSSLDebugAlert(_pState, _uAlertLevel, _uAlertType)
#endif

/*F********************************************************************************/
/*!
    \Function _ProtoSSLSendAlert

    \Description
        Send an alert

    \Input *pState      - module state reference
    \Input iLevel       - alert level (1=warning, 2=error)
    \Input iValue       - alert value

    \Output
        int32_t         - current state

    \Version 11/06/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLSendAlert(ProtoSSLRefT *pState, int32_t iLevel, int32_t iValue)
{
    /* $$TODO: we can currently only send if the send state is empty; this should
       be addressed so we can queue an alert for sending at any time */
    if ((pState->pSecure != NULL) && (pState->pSecure->iSendProg == 0) && (pState->pSecure->iSendSize == 0))
    {
        uint8_t strHead[2];
        #if DIRTYCODE_LOGGING
        ProtoSSLAlertDescT Alert;
        _ProtoSSLGetAlert(pState, &Alert, iLevel, iValue);
        #endif
        pState->uAlertLevel = strHead[0] = (uint8_t)iLevel;
        pState->uAlertValue = strHead[1] = (uint8_t)iValue;
        pState->bAlertSent = TRUE;
        NetPrintf(("protossl: sending alert: level=%d type=%s (%d)\n", iLevel, Alert.pAlertDesc, iValue));
        // stage the alert for sending
        _ProtoSSLSendPacket(pState, SSL3_REC_ALERT, strHead, sizeof(strHead), NULL, 0);
        // flush alert
        _ProtoSSLSendSecure(pState, pState->pSecure);
        /* As per http://tools.ietf.org/html/rfc5246#section-7.2.2, any error alert sent
           or received must result in current session information being reset (no resume) */
        if (iLevel == SSL3_ALERT_LEVEL_FATAL)
        {
            _SessionHistoryInvalidate(pState->strHost, SockaddrInGetPort(&pState->PeerAddr));
        }
    }
    else
    {
        NetPrintf(("protossl: unable to send alert (pSecure=%p, iSendProg=%d, iSendSize=%d)\n", pState->pSecure,
            pState->pSecure->iSendProg, pState->pSecure->iSendSize));
    }

    // return current state
    return(pState->iState);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvAlert

    \Description
        Process an alert message

    \Input *pState      - module state reference
    \Input *pSecure     - secure state reference

    \Version 11/14/2017 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLRecvAlert(ProtoSSLRefT *pState, SecureStateT *pSecure)
{
    pState->uAlertLevel = pSecure->RecvData[pSecure->iRecvBase];
    pState->uAlertValue = pSecure->RecvData[pSecure->iRecvBase+1];
    pState->bAlertSent = FALSE;

    // process warnings
    if (pState->uAlertLevel == SSL3_ALERT_LEVEL_WARNING)
    {
        if (pState->uAlertValue == SSL3_ALERT_DESC_CLOSE_NOTIFY)
        {
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: received close notification\n"));
            // only an error if we are still in setup
            if (pState->iState < ST3_SECURE)
            {
                pState->iState = ST_FAIL_SETUP;
            }
        }
        if (pState->uAlertValue == SSL3_ALERT_DESC_NO_RENEGOTIATION)
        {
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: received no_renegotiation alert\n"));
            pState->iState = (pState->iState == ST3_RECV_HELLO) ? ST3_SECURE : ST_FAIL_SECURE;
        }
    }
    else // if not a warning, it is an error
    {
        _ProtoSSLDebugAlert(pState, pSecure->RecvData[pSecure->iRecvBase], pSecure->RecvData[pSecure->iRecvBase+1]);
        pState->iState = (pState->iState < ST3_SECURE) ? ST_FAIL_SETUP : ST_FAIL_SECURE;
    }

    // consume the alert message
    _ProtoSSLRecvReset(pSecure);

    // process if the alert resulted in a failure state
    if (pState->iState >= ST_FAIL)
    {
        /* As per http://tools.ietf.org/html/rfc5246#section-7.2.2 any error alert sent or
           received must result in current session information being reset (no resume) */
        _SessionHistoryInvalidate(pState->strHost, SockaddrInGetPort(&pState->PeerAddr));
        pState->iClosed = 1;
    }
}

/*
    handshake utility functions
*/

/*F********************************************************************************/
/*!
    \Function _ProtoSSLGenerateFingerprint

    \Description
        Calculate fingerprint for given data

    \Input *pOutBuf - output buffer to store fingerprint
    \Input iOutSize - size of output buffer
    \Input *pInpData - input data to calculate fingerprint for
    \Input iInpSize - size of input data
    \Input eHashType - hash operation to use

    \Version 04/29/2014 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLGenerateFingerprint(uint8_t *pOutBuf, int32_t iOutSize, const uint8_t *pInpData, int32_t iInpSize, CryptHashTypeE eHashType)
{
    const CryptHashT *pHash;

    // calculate the fingerprint using designated hash type
    if ((pHash = CryptHashGet(eHashType)) != NULL)
    {
        uint8_t aHashState[CRYPTHASH_MAXSTATE];
        int32_t iHashSize = CryptHashGetSize(eHashType);
        if (iHashSize > iOutSize)
        {
            iHashSize = iOutSize;
        }
        pHash->Init(aHashState, iHashSize);
        pHash->Update(aHashState, pInpData, iInpSize);
        pHash->Final(aHashState, pOutBuf, iHashSize);
        #if DEBUG_VAL_CERT
        NetPrintMem(pOutBuf, iHashSize, "sha1-fingerprint");
        #endif
    }
    else
    {
        NetPrintf(("protossl: _AsnParseCertificate: could not calculate fingerprint\n"));
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLGenerateEccSharedSecret

    \Description
        Generate the ECDHE shared secret if necessary

    \Input *pState      - module state reference

    \Output
        uint8_t         - TRUE=operation complete, FALSE=operation ongoing

    \Version 03/07/2017 (eesponda)
*/
/********************************************************************************F*/
static uint8_t _ProtoSSLGenerateEccSharedSecret(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;
    const CryptCurveDhT *pEcc;
    CryptEccPointT PublicKey;
    int32_t iSecretSize = 0;
    uint32_t uCryptUsecs = 0;

    // only generate the secret if we are using an ECDHE cipher
    if (pSecure->pCipher->uKey != SSL3_KEY_ECDHE)
    {
        return(TRUE);
    }

    /* init context with key exchange private key, if we have not already done so.
       if we fail to get the dh functions then our master secret will be bad which will
       lead to handshake failure */
    if ((pEcc = _ProtoSSLEccInitContext(pSecure, pSecure->pEllipticCurve)) != NULL)
    {
        /* generate shared secret: elliptic curve generation takes multiple frames
           so stay in the same state until the operation is complete */
        pEcc->PointInit(&PublicKey, pSecure->PubKey, pSecure->uPubKeyLength);
        if (pEcc->Secret(&pSecure->EccContext, &PublicKey, NULL, &uCryptUsecs) > 0)
        {
            return(FALSE);
        }
        iSecretSize = pEcc->PointFinal(pSecure->EccContext, NULL, TRUE, pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey));
    }

    NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (generate shared secret) %dms\n",
        uCryptUsecs/1000));
    pSecure->uTimer += uCryptUsecs/1000;

    // build master secret
    _ProtoSSLBuildKey(pState, pSecure->MasterKey, sizeof(pSecure->MasterKey), pSecure->PreMasterKey, iSecretSize,
        pSecure->ClientRandom, pSecure->ServerRandom, sizeof(pSecure->ClientRandom), "master secret",
        pSecure->uSslVersion);

    // finished with ecc context
    pSecure->bEccContextInitialized = FALSE;
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLGenerateCertificateVerifyHash

    \Description
        Generate a tls1.3 DigitalSignatureHash; see
        https://tools.ietf.org/html/rfc8446#section-4.4.3

    \Input *pBuffer     - [out] storage for generated digitalsignaturehash object
    \Input eHashType    - signature algorithm hash type
    \Input *pHashData   - pointer to hash data to embed in the object
    \Input iHashSize    - size of hash data
    \Input bServer      - TRUE if server, else FALSE
    \Input bSending     - TRUE if sending, else receiving

    \Output
        int32_t         - size of generated hash

    \Version 05/18/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLGenerateCertificateVerifyHash(uint8_t *pBuffer, CryptHashTypeE eHashType, const uint8_t *pHashData, int32_t iHashSize, uint8_t bServer, uint8_t bSending)
{
    uint8_t aTempBuf[64 + 34 + CRYPTHASH_MAXDIGEST], *pTempBuf = aTempBuf;
    uint8_t aHashState[CRYPTHASH_MAXSTATE];
    const CryptHashT *pHash = CryptHashGet(eHashType);

    // 64 bytes of spaces
    ds_memset(pTempBuf, 32, 64);
    pTempBuf += 64;
    // context string
    pTempBuf += ds_snzprintf((char *)pTempBuf, sizeof(aTempBuf) - 64, "TLS 1.3, %s CertificateVerify", ((bServer && bSending) || (!bServer && !bSending)) ? "server" : "client");
    // zero separator
    *pTempBuf++ = '\0';
    // handshake hash
    ds_memcpy(pTempBuf, pHashData, iHashSize);
    pTempBuf += iHashSize;

    // hash the envelope with the signature algorithm hash (may not match handshake hash)
    iHashSize = CryptHashGetSize(eHashType);
    pHash->Init(aHashState, iHashSize);
    pHash->Update(aHashState, aTempBuf, pTempBuf - aTempBuf);
    pHash->Final(aHashState, pBuffer, iHashSize);

    // return hash size to caller
    return(iHashSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLGenerateFinishHash

    \Description
        Generates finish hash data

    \Input *pBuffer     - [out] storage for generated finish hash
    \Input *pSecure     - secure state
    \Input *pLabelTLS   - label to use for TLS1 finish hash calculation
    \Input bServer      - TRUE if we're the server, else client
    \Input bSending     - TRUE if we're sending the finish hash, else receiving

    \Output
        int32_t         - finish hash size

    \Version 10/11/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLGenerateFinishHash(uint8_t *pBuffer, SecureStateT *pSecure, const char *pLabelTLS, uint8_t bServer, uint8_t bSending)
{
    uint8_t aMacTemp[128];
    int32_t iHashSize;

    if (pSecure->uSslVersion < SSL3_TLS1_3)
    {
        iHashSize = 12;
        ds_strnzcpy((char *)aMacTemp, pLabelTLS, sizeof(aMacTemp));

        if (pSecure->uSslVersion < SSL3_TLS1_2)
        {
            // setup the finish verification hashes as per https://tools.ietf.org/html/rfc4346#section-7.4.9
            _ProtoSSLHandshakeHashGet(pSecure, CRYPTHASH_MD5, aMacTemp+15, sizeof(aMacTemp)-15);
            _ProtoSSLHandshakeHashGet(pSecure, CRYPTHASH_SHA1, aMacTemp+31, sizeof(aMacTemp)-31);
            _ProtoSSLDoPRF(pBuffer, iHashSize, pSecure->MasterKey, sizeof(pSecure->MasterKey), aMacTemp, 51);
        }
        else
        {
            // setup the finish verification hashes as per https://tools.ietf.org/html/rfc5246#section-7.4.9
            int32_t iHashSize2 = _ProtoSSLHandshakeHashGet(pSecure, pSecure->pCipher->uPrfType, aMacTemp+15, sizeof(aMacTemp)-15);
            _ProtoSSLDoPHash(pBuffer, iHashSize, pSecure->MasterKey, sizeof(pSecure->MasterKey), aMacTemp, 15+iHashSize2, pSecure->pCipher->uPrfType);
        }
    }
    else
    {
        // setup the finish verification hashes as per https://tools.ietf.org/html/rfc8446#section-4.4.4
        uint8_t aFinKey[CRYPTHASH_MAXDIGEST], *pHandshakeSecret;
        // get handshake hash
        iHashSize = _ProtoSSLHandshakeHashGet(pSecure, pSecure->pCipher->uPrfType, aMacTemp, sizeof(aMacTemp));
        // get handshake secret
        pHandshakeSecret = ((bServer && bSending) || (!bServer && !bSending)) ? pSecure->pServerSecret : pSecure->pClientSecret;
        // finished_key = HKDF-Expand-Label(BaseKey, "finished", "", Hash.length)
        _ProtoSSLHkdfExpandLabel(aFinKey, iHashSize, pHandshakeSecret, iHashSize, "finished", aMacTemp, 0, pSecure->pCipher->uPrfType);
        // verify_data = HMAC(finished_key, Hash(Handshake Context + Certificate* + CertificateVerify*)
        _ProtoSSLHkdfExtract(pBuffer, iHashSize, aFinKey, iHashSize, aMacTemp, iHashSize, pSecure->pCipher->uPrfType);
    }

    return(iHashSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLGetSignatureScheme

    \Description
        Get SignatureScheme from two-byte ident

    \Input uIdent                 - ident

    \Output
        const SignatureSchemeT *  - pointer to signature scheme, or NULL

    \Version 05/12/2017 (jbrookes)
*/
/********************************************************************************F*/
static const SignatureSchemeT *_ProtoSSLGetSignatureScheme(const uint16_t uIdent)
{
    const SignatureSchemeT *pSigScheme;
    int32_t iTable;

    for (iTable = 0, pSigScheme = NULL; iTable < (signed)(sizeof(_SSL3_SignatureSchemes) / sizeof(_SSL3_SignatureSchemes[0])); iTable += 1)
    {
        if (_SSL3_SignatureSchemes[iTable].uIdent != uIdent)
        {
            continue;
        }
        pSigScheme = &_SSL3_SignatureSchemes[iTable];
    }
    return(pSigScheme);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLChooseSignatureScheme

    \Description
        Choose a SignatureScheme from a list

    \Input *pSecure         - secure state
    \Input *pCertificate    - server/client certificate
    \Input *pData           - signature scheme list
    \Input uSigSchemeLen    - length of list in bytes
    \Input *pDataEnd        - safe parse limit

    \Output
        const SignatureSchemeT *  - pointer to signature scheme, or NULL

    \Version 05/12/2017 (jbrookes)
*/
/********************************************************************************F*/
static const SignatureSchemeT *_ProtoSSLChooseSignatureScheme(SecureStateT *pSecure, CertificateDataT *pCertificate, const uint8_t *pData, uint32_t uSigSchemeLen, const uint8_t *pDataEnd)
{
    const SignatureSchemeT *pSigScheme;
    int32_t iSigScheme, iNumSigSchemes;
    uint32_t uCertSigAlg;

    // get signature scheme we need for certificate, if available
    uCertSigAlg = _CertificateGetSigAlg(pCertificate);

    // pick first supported signature scheme
    for (iSigScheme = 0, iNumSigSchemes = (signed)(uSigSchemeLen/sizeof(SignatureAlgorithmT)); iSigScheme < iNumSigSchemes; iSigScheme += 1, pData += 2)
    {
        // skip unsupported signature schemes
        if ((pSigScheme = _ProtoSSLGetSignatureScheme(_SafeRead16(pData, pDataEnd))) == NULL)
        {
            continue;
        }
        // don't pick a signature scheme we don't have a certificate for
        if ((uCertSigAlg != SSL3_SIGALG_NONE) && (pSigScheme->SigAlg.uSigAlg != uCertSigAlg))
        {
            continue;
        }
        // don't allow pss signature schemes with a pkcs1 certificate
        if ((uCertSigAlg == SSL3_SIGALG_RSA) && (pCertificate->iKeyType != pSigScheme->uOidType))
        {
            continue;
        }
        // tls1.3 specific checks
        if (pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3)
        {
            // don't allow pkcs1 signature schemes
            if (pSigScheme->uVerifyScheme == SSL3_SIGVERIFY_RSA_PKCS1)
            {
                continue;
            }
            // enforce tls1.3 more restrictive signature schemes based on certificate key type
            if ((uCertSigAlg == SSL3_SIGALG_ECDSA) && (pCertificate->iCrvType != pSigScheme->uOidType))
            {
                continue;
            }
        }
        return(pSigScheme);
    }
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLGetCipher

    \Description
        Get cipher from ident, validate that we previously sent it

    \Input *pSecure     - secure state
    \Input uCipherIdent - cipher ident to get cipher for

    \Output
        CipherSuiteT *  - pointer to cipher, or null

    \Version 01/18/2018 (jbrookes)
*/
/********************************************************************************F*/
static const CipherSuiteT *_ProtoSSLGetCipher(SecureStateT *pSecure, uint16_t uCipherIdent)
{
    const CipherSuiteT *pCipher;
    int32_t iIndex;

    // match the cipher from our list
    for (iIndex = 0, pCipher = NULL; (iIndex < (signed)(sizeof(_SSL3_CipherSuite)/sizeof(_SSL3_CipherSuite[0])) && (pCipher == NULL)); iIndex += 1)
    {
        if (uCipherIdent != _SSL3_CipherSuite[iIndex].uIdent)
        {
            continue;
        }
        // validate that we sent it
        if ((_SSL3_CipherSuite[iIndex].uId & pSecure->uSentCiphers) == 0)
        {
            NetPrintf(("protossl: received cipher from server that we did not send\n"));
            break;
        }
        pCipher = &_SSL3_CipherSuite[iIndex];
    }

    // return to caller
    return(pCipher);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLChooseCipher

    \Description
        Choose cipher from list of idents idents

    \Input *pSecure     - secure state
    \Input *pCertificate - server/client cert, if we have one
    \Input *pCipherList - list of cipher idents
    \Input iNumCiphers  - number of ciphers
    \Input *pDataEnd    - end of data for safe parsing
    \Input uCipherMask  - mask of enabled ciphers
    \Input uCipherPref  - ident of preferred cipher, or zero for no preference

    \Output
        CipherSuiteT *  - pointer to cipher, or null

    \Version 02/19/2018 (jbrookes) Refactored from _ProtoSSLUpdateRecvClientHello
*/
/********************************************************************************F*/
static const CipherSuiteT *_ProtoSSLChooseCipher(SecureStateT *pSecure, CertificateDataT *pCertificate, const uint8_t *pCipherList, int32_t iNumCiphers, const uint8_t *pDataEnd, uint32_t uCipherMask, uint32_t uCipherPref)
{
    const uint8_t *pCipherStart = pCipherList;
    const CipherSuiteT *pCipher;
    int32_t iCipher, iIndex;
    uint16_t uCipherIdent;
    uint32_t uCertSigAlg;

    // get signature algorithm we need for certificate, if available
    uCertSigAlg = _CertificateGetSigAlg(pCertificate);

    // pick first supported cipher
    for (iCipher = 0, iIndex = 0, pSecure->pCipher = NULL; (iCipher < iNumCiphers) && (pSecure->pCipher == NULL); iCipher += 1, pCipherList += 2)
    {
        // read cipher
        uCipherIdent = _SafeRead16(pCipherList, pDataEnd);
        
        // check against our list
        for (iIndex = 0; iIndex < (signed)(sizeof(_SSL3_CipherSuite)/sizeof(_SSL3_CipherSuite[0])); iIndex += 1)
        {
            // ref supported cipher list entry
            pCipher = &_SSL3_CipherSuite[iIndex];

            // skip non-matching ciphers
            if (uCipherIdent != pCipher->uIdent)
            {
                continue;
            }
            // skip non-preferred cipher, if set
            if ((uCipherPref != 0) && (uCipherIdent != uCipherPref))
            {
                continue;
            }
            // skip disabled ciphers
            if ((pCipher->uId & uCipherMask) == 0)
            {
                continue;
            }
            /* skip elliptic curve ciphers if we found no supported elliptic curves in the extensions; for tls1.3
               we let this throogh, and will send a HelloRetryRequest asking for an elliptic curve we support */
            if ((pSecure->uSslVersion < PROTOSSL_VERSION_TLS1_3) && (pCipher->uKey != SSL3_KEY_RSA) && (pSecure->pEllipticCurve == NULL))
            {
                continue;
            }
            // tls1.2 or prior; skip ecdsa/rsa ciphers if we don't have an ecdsa/rsa cert
            if ((pSecure->uSslVersion < PROTOSSL_VERSION_TLS1_3) && (pCipher->uSig != uCertSigAlg))
            {
                continue;
            }
            // skip ciphers that require a newer version of SSL than has been negotiated
            if (pCipher->uMinVers > pSecure->uSslVersion)
            {
                continue;
            }
            // tls1.3 requires tls1.3 ciphers
            if ((pSecure->uSslVersion >= SSL3_TLS1_3) && (pCipher->uMinVers < SSL3_TLS1_3))
            {
                continue;
            }

            // found a cipher
            pSecure->pCipher = pCipher;
            break;
        }
    }
    // if we were looking for a preferred cipher and didn't find it, try again with no preference
    if ((pSecure->pCipher == NULL) && (uCipherPref != 0))
    {
        pSecure->pCipher = _ProtoSSLChooseCipher(pSecure, pCertificate, pCipherStart, iNumCiphers, pDataEnd, uCipherMask, 0);
    }
    return(pSecure->pCipher);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLChooseCurve

    \Description
        Choose curve based on enabled curves and default curve preferences

    \Input uEnabledCurves   - enabled curves
    \Input iCurveDflt       - preferred default curve

    \Output
        EllipticCurveT *    - pointer to curve, or null

    \Version 06/07/2019 (eesponda)
*/
/********************************************************************************F*/
static const EllipticCurveT *_ProtoSSLChooseCurve(uint32_t uEnabledCurves, int32_t iCurveDflt)
{
    int32_t iEllipticCurve;
    const EllipticCurveT *pEllipticCurve = NULL;

    // if the default curve is enabled and supported use that
    if ((iCurveDflt != -1) && ((_SSL3_EllipticCurves[iCurveDflt].uId & uEnabledCurves) != 0))
    {
        pEllipticCurve = &_SSL3_EllipticCurves[iCurveDflt];
    }
    // otherwise choose the first supported curve in our list
    for (iEllipticCurve = 0; (iEllipticCurve < SSL3_NUM_CURVES) && (pEllipticCurve == NULL); iEllipticCurve += 1)
    {
        // skip disabled curves
        if ((_SSL3_EllipticCurves[iEllipticCurve].uId & uEnabledCurves) == 0)
        {
            continue;
        }
        // found a curve
        pEllipticCurve = &_SSL3_EllipticCurves[iEllipticCurve];
    }
    return(pEllipticCurve);
}

/*
   hello extension writing
*/

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnWriteAlpn

    \Description
        Write Application Level Protocol Negotiation (ALPN) extension to ClientHello
        and ServerHello handshake packets; ref http://tools.ietf.org/html/rfc7301

    \Input *pState      - module state reference
    \Input *pBuffer     - buffer to write extension into
    \Input iBufLen      - length of buffer

    \Output
        int32_t         - number of bytes added to buffer

    \Version 08/03/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHelloExtnWriteAlpn(ProtoSSLRefT *pState, uint8_t *pBuffer, int32_t iBufLen)
{
    uint16_t uAlpnExtensionLength, uExtnLength;
    uint8_t *pBufStart = pBuffer;
    const SecureStateT *pSecure = pState->pSecure;

    // if no protocols, don't write anything into ClientHello
    if ((!pState->bServer) && (pState->uNumAlpnProtocols == 0))
    {
        return(0);
    }
    // if no protocol selected, don't write anything into ServerHello
    else if ((pState->bServer) && ((pSecure == NULL) || (*pSecure->strAlpnProtocol == '\0')))
    {
        return(0);
    }

    // calculate the size of the extension based on if we are client or server
    uAlpnExtensionLength = (!pState->bServer) ? pState->uAlpnExtensionLength : ((uint8_t)strlen(pSecure->strAlpnProtocol) + sizeof(uint8_t));
    uExtnLength = 2+uAlpnExtensionLength; // ident+length+protocol list length

    // make sure we have room
    if ((2+2+uExtnLength) > iBufLen)
    {
        NetPrintf(("protossl: could not add extension alpn; insufficient buffer\n"));
        return(0);
    }

    // extension ident
    *pBuffer++ = (uint8_t)(SSL_EXTN_ALPN>>8);
    *pBuffer++ = (uint8_t)(SSL_EXTN_ALPN>>0);
    // extension length
    *pBuffer++ = (uint8_t)(uExtnLength>>8);
    *pBuffer++ = (uint8_t)(uExtnLength>>0);
    // alpn length
    *pBuffer++ = (uint8_t)(pState->uAlpnExtensionLength>>8);
    *pBuffer++ = (uint8_t)(pState->uAlpnExtensionLength>>0);

    // write the protocols
    if (!pState->bServer)
    {
        int16_t iProtocol;

        // alpn protocols (length + byte string)
        for (iProtocol = 0; iProtocol < pState->uNumAlpnProtocols; iProtocol += 1)
        {
            const AlpnProtocolT *pProtocol = &pState->aAlpnProtocols[iProtocol];

            *pBuffer++ = pProtocol->uLength;
            ds_memcpy(pBuffer, pProtocol->strName, pProtocol->uLength);
            pBuffer += pProtocol->uLength;
        }
    }
    else
    {
        const uint8_t uProtocolLen = uAlpnExtensionLength-(uint8_t)sizeof(uint8_t);

        // alpn protocol
        *pBuffer++ = uProtocolLen;
        ds_memcpy(pBuffer, pSecure->strAlpnProtocol, uProtocolLen);
        pBuffer += uProtocolLen;
    }

    return((int32_t)(pBuffer-pBufStart));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnWriteCookie

    \Description
        Write Cookie extension, if we have one

    \Input *pState      - module state reference
    \Input *pBuffer     - buffer to write extension into
    \Input iBufLen      - length of buffer

    \Output
        int32_t         - number of bytes added to buffer

    \Version 11/29/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHelloExtnWriteCookie(ProtoSSLRefT *pState, uint8_t *pBuffer, int32_t iBufLen)
{
    uint8_t *pBufStart = pBuffer;
    SecureStateT *pSecure = pState->pSecure;
    int32_t iCookieLength;

    // only if we have a cookie
    if (pSecure->pCookie == NULL)
    {
        return(0);
    }

    // read cookie length
    iCookieLength = (pSecure->pCookie[0] << 8) | pSecure->pCookie[1];

    // make sure we have enough room
    if ((2+2+2+iCookieLength) > iBufLen)
    {
        NetPrintf(("protossl: could not add extension cookie; insufficient buffer\n"));
        return(0);
    }

    // extension ident
    *pBuffer++ = (uint8_t)(SSL_EXTN_COOKIE>>8);
    *pBuffer++ = (uint8_t)(SSL_EXTN_COOKIE>>0);
    // extension length
    *pBuffer++ = (uint8_t)((iCookieLength+2)>>8);
    *pBuffer++ = (uint8_t)((iCookieLength+2)>>0);
    // cookie length
    *pBuffer++ = (uint8_t)(iCookieLength>>8);
    *pBuffer++ = (uint8_t)(iCookieLength>>0);
    // add the cookie
    ds_memcpy_s(pBuffer, iBufLen-6, pSecure->pCookie+2, iCookieLength);
    pBuffer += iCookieLength;

    // as per https://tools.ietf.org/html/rfc8446#section-4.2.2 cookies can only be used once
    pSecure->pCookie = NULL;

    return((int32_t)(pBuffer-pBufStart));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnWriteEllipticCurves

    \Description
        Write the Elliptic Curves / Supported Groups extension to the ClientHello;
        ref http://tools.ietf.org/html/rfc4492

    \Input *pState      - module state reference
    \Input *pBuffer     - buffer to write extension into
    \Input iBufLen      - length of buffer

    \Output
        int32_t         - number of bytes added to buffer

    \Notes
        TLS1.3 calls this "supported_groups", but the actual extension format is
        identical.

    \Version 01/19/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHelloExtnWriteEllipticCurves(ProtoSSLRefT *pState, uint8_t *pBuffer, int32_t iBufLen)
{
    const uint16_t uMaxEllipticCurvesLength = sizeof(_SSL3_EllipticCurves[0].uIdent)*(sizeof(_SSL3_EllipticCurves)/sizeof(*_SSL3_EllipticCurves));
    uint16_t uEllipticCurvesLength, uExtnLength;
    uint8_t *pBufStart = pBuffer, *pExtnLength, uNumCurves;
    int32_t iEllipticCurve;

    // make sure we have enough room
    if ((2+2+2+uMaxEllipticCurvesLength) > iBufLen)
    {
        NetPrintf(("protossl: could not add extension supported_groups; insufficient buffer\n"));
        return(0);
    }
    // if no curves are enabled don't write anything
    if (pState->uEnabledCurves == 0)
    {
        return(0);
    }

    // extension ident
    *pBuffer++ = (uint8_t)(SSL_EXTN_ELLIPTIC_CURVES>>8);
    *pBuffer++ = (uint8_t)(SSL_EXTN_ELLIPTIC_CURVES>>0);
    // save the location and skip past
    pExtnLength = pBuffer;
    pBuffer += 4;
    // supported elliptic curves
    for (iEllipticCurve = 0, uNumCurves = 0; iEllipticCurve < (signed)(sizeof(_SSL3_EllipticCurves)/sizeof(*_SSL3_EllipticCurves)); iEllipticCurve += 1)
    {
        // skip disabled curves
        if ((_SSL3_EllipticCurves[iEllipticCurve].uId & pState->uEnabledCurves) == 0)
        {
            continue;
        }
        *pBuffer++ = (uint8_t)(_SSL3_EllipticCurves[iEllipticCurve].uIdent>>8);
        *pBuffer++ = (uint8_t)(_SSL3_EllipticCurves[iEllipticCurve].uIdent>>0);
        uNumCurves += 1;
    }
    uEllipticCurvesLength = sizeof(_SSL3_EllipticCurves[0].uIdent)*uNumCurves;
    uExtnLength = uEllipticCurvesLength+2;

    // extension length
    *pExtnLength++ = (uint8_t)(uExtnLength>>8);
    *pExtnLength++ = (uint8_t)(uExtnLength>>0);
    // elliptic curves length
    *pExtnLength++ = (uint8_t)(uEllipticCurvesLength>>8);
    *pExtnLength++ = (uint8_t)(uEllipticCurvesLength>>0);

    return((int32_t)(pBuffer-pBufStart));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnWriteKeyShare

    \Description
        Write the KeyShare extension to the ClientHello;
        ref https://tools.ietf.org/html/rfc8446#section-4.2.8

    \Input *pState      - module state reference
    \Input *pBuffer     - buffer to write extension into
    \Input iBufLen      - length of buffer

    \Output
        int32_t         - number of bytes added to buffer

    \Version 01/25/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHelloExtnWriteKeyShare(ProtoSSLRefT *pState, uint8_t *pBuffer, int32_t iBufLen)
{
    SecureStateT *pSecure = pState->pSecure;
    int32_t iDataLength = 0;
    uint8_t *pBufStart = pBuffer;
    uint8_t aPublicKey[128];
    const EllipticCurveT *pEllipticCurve;

    // ref elliptic curve
    if (((pEllipticCurve = pSecure->pEllipticCurve) == NULL) && (pState->iCurveDflt != -1) && ((_SSL3_EllipticCurves[pState->iCurveDflt].uId & pState->uEnabledCurves) != 0))
    {
        pEllipticCurve = _ProtoSSLChooseCurve(pState->uEnabledCurves, pState->iCurveDflt);
    }

    // set up curve data
    if (pEllipticCurve != NULL)
    {
        const CryptCurveDhT *pEcc;
        int32_t iPublicKeySize = 0;

        // set public key ident
        aPublicKey[0] = (uint8_t)(pEllipticCurve->uIdent>>8);
        aPublicKey[1] = (uint8_t)(pEllipticCurve->uIdent>>0);

        // if server in helloretryrequest flow, use KeyShareHelloRetryRequest value which does not include a key or key size
        if (pState->bServer && (pState->iState == ST3_SEND_HELLO_RETRY))
        {
            iDataLength = 2;
        }
        else
        {
            iDataLength = 4;

            // encode public key generated in Hello into buffer
            if (pSecure->bEccKeyGenerated && ((pEcc = _ProtoSSLEccInitContext(pSecure, pEllipticCurve)) != NULL))
            {
                iPublicKeySize = pEcc->PointFinal(pSecure->EccContext, NULL, FALSE, aPublicKey+4, sizeof(aPublicKey)-4);
                iDataLength += iPublicKeySize;
            }

            // encode public key size 
            aPublicKey[2] = (uint8_t)(iPublicKeySize>>8);
            aPublicKey[3] = (uint8_t)(iPublicKeySize>>0);
        }
    }

    // make sure we have enough room
    if ((2+2+2+iDataLength) > iBufLen)
    {
        NetPrintf(("protossl: could not add extension key_share; insufficient buffer\n"));
        return(0);
    }
    
    // extension ident
    *pBuffer++ = (uint8_t)(SSL_EXTN_KEY_SHARE>>8);
    *pBuffer++ = (uint8_t)(SSL_EXTN_KEY_SHARE>>0);
    // extension length (client only)
    if (!pState->bServer)
    {
        *pBuffer++ = (uint8_t)((iDataLength+2)>>8);
        *pBuffer++ = (uint8_t)((iDataLength+2)>>0);
    }
    // key_share length
    *pBuffer++ = (uint8_t)(iDataLength>>8);
    *pBuffer++ = (uint8_t)(iDataLength>>0);
    // key share data
    if (iDataLength > 0)
    {
        ds_memcpy(pBuffer, aPublicKey, iDataLength);
        pBuffer += iDataLength;
    }
    // return extension size to caller
    return((int32_t)(pBuffer-pBufStart));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnWritePreSharedKey

    \Description
        Write Pre-Shared Key (PSK) extension to the ClientHello handshake packet;
        ref https://tools.ietf.org/html/rfc8446#section-4.2.11

    \Input *pState      - module state reference
    \Input *pBuffer     - buffer to write extension into
    \Input iBufLen      - length of buffer
    \Input *pHshkMsgBuf - handshake message buffer pointer

    \Output
        int32_t         - number of bytes added to buffer

    \Version 12/07/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHelloExtnWritePreSharedKey(ProtoSSLRefT *pState, uint8_t *pBuffer, int32_t iBufLen, const uint8_t *pHshkMsgBuf)
{
    uint8_t *pBufStart = pBuffer, *pHshkMsgEnd;
    const SessionHistoryT *pSessHist;
    const SessionTicketT *pSessTick;
    int32_t iExtnLength, iHashSize; 
    uint32_t uAgeAdd;

    // only add if we have a ticket to match
    if (((pSessHist = _SessionHistoryGet(pState->strHost, SockaddrInGetPort(&pState->PeerAddr), NULL)) == NULL) || !pSessHist->bSessionTicket)
    {
        return(0);
    }
    pSessTick = &pSessHist->SessionTicket;

    // get hash length
    iHashSize = CryptHashGetSize(pSessTick->eHashType);

    // make sure we have enough room
    iExtnLength = 4 + 2 + 2 + pSessTick->uTickLen + 4 + 2 + 1 + iHashSize;
    if (iExtnLength > iBufLen)
    {
        return((iBufLen == 0) ? iExtnLength : 0);
    }

    // pre_shared_Key extension ident
    *pBuffer++ = (uint8_t)(SSL_EXTN_PRE_SHARED_KEY>>8);
    *pBuffer++ = (uint8_t)(SSL_EXTN_PRE_SHARED_KEY>>0);

    // client adds offered psks (we only support one)
    if (!pState->bServer)
    {
        // set pre_shared_key extension length
        *pBuffer++ = (uint8_t)((iExtnLength-4)>>8);
        *pBuffer++ = (uint8_t)((iExtnLength-4)>>0);

        // add psks identity length
        *pBuffer++ = (uint8_t)((2+pSessTick->uTickLen+sizeof(uint32_t))>>8);
        *pBuffer++ = (uint8_t)((2+pSessTick->uTickLen+sizeof(uint32_t))>>0);
        // add identity length
        *pBuffer++ = (uint8_t)(pSessTick->uTickLen>>8);
        *pBuffer++ = (uint8_t)(pSessTick->uTickLen>>0);
        // add ticket 
        ds_memcpy(pBuffer, pSessTick->aTicketData, pSessTick->uTickLen);
        pBuffer += pSessTick->uTickLen;
        // add obfuscated ticket age
        uAgeAdd = (time(NULL) - pSessTick->uRecvTime) * 1000;
        uAgeAdd += pSessTick->uAgeAdd;
        *pBuffer++ = (uint8_t)(uAgeAdd>>24);
        *pBuffer++ = (uint8_t)(uAgeAdd>>16);
        *pBuffer++ = (uint8_t)(uAgeAdd>>8);
        *pBuffer++ = (uint8_t)(uAgeAdd>>0);
        // save handshake message end
        pHshkMsgEnd = pBuffer;
        // add binders length
        *pBuffer++ = (uint8_t)((iHashSize+1)>>8);
        *pBuffer++ = (uint8_t)((iHashSize+1)>>0);
        // add binder length
        *pBuffer++ = (uint8_t)(iHashSize>>0);
        // calculate the binder
        pBuffer = _ProtoSSLCalcResumeBinder(pState, pBuffer, iBufLen, pSessTick, pHshkMsgBuf, pHshkMsgEnd-pHshkMsgBuf, iHashSize);
    }
    else // indicated selected psk - we only support one
    {
        *pBuffer++ = (uint8_t)(2>>8);
        *pBuffer++ = (uint8_t)(2>>0);
        *pBuffer++ = (uint8_t)(0>>8);
        *pBuffer++ = (uint8_t)(0>>0);
    }
    return((int32_t)(pBuffer-pBufStart));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnWritePreSharedKeyModes

    \Description
        Write Pre-Shared Key Exchange Modes extension to the ClientHello handshake
        packet; ref https://tools.ietf.org/html/rfc8446#section-4.2.9

        This message is client-only

    \Input *pState      - module state reference
    \Input *pBuffer     - buffer to write extension into
    \Input iBufLen      - length of buffer

    \Output
        int32_t         - number of bytes added to buffer

    \Version 12/12/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHelloExtnWritePreSharedKeyModes(ProtoSSLRefT *pState, uint8_t *pBuffer, int32_t iBufLen)
{
    const uint16_t uExtnLength = 2;
    uint8_t *pBufStart = pBuffer;

    // make sure we have enough room
    if ((2+2+uExtnLength) > iBufLen)
    {
        NetPrintf(("protossl: could not add extension psk_modes; insufficient buffer\n"));
        return(0);
    }

    // extension ident
    *pBuffer++ = (uint8_t)(SSL_EXTN_PSK_MODES>>8);
    *pBuffer++ = (uint8_t)(SSL_EXTN_PSK_MODES>>0);
    // extension length
    *pBuffer++ = (uint8_t)(uExtnLength>>8);
    *pBuffer++ = (uint8_t)(uExtnLength>>0);
    // psk_mode array length
    *pBuffer++ = 1;
    // psk_modes (only offer psk_dhe_ke; most servers explicitly disallow psk_ke)
    *pBuffer++ = 1;

    return((int32_t)(pBuffer-pBufStart));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnWriteRenegotiationInfo

    \Description
        Write Renegotiation Info extension to the ClientHello handshake packet;
        ref https://tools.ietf.org/html/rfc5746

    \Input *pState      - module state reference
    \Input *pBuffer     - buffer to write extension into
    \Input iBufLen      - length of buffer

    \Output
        int32_t         - number of bytes added to buffer

    \Notes
        This extension is added strictly for compatibility with sites that require
        it and will fail the connection if it is not present.  ProtoSSL does not
        support renegotiation of any kind and an attempt by a server to renegotiate
        will result in a no_renegotiation alert.

    \Version 03/27/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHelloExtnWriteRenegotiationInfo(ProtoSSLRefT *pState, uint8_t *pBuffer, int32_t iBufLen)
{
    uint32_t uHostLen = (uint32_t)strlen(pState->strHost);
    uint8_t *pBufStart = pBuffer;

    // make sure we have enough room
    if ((uHostLen+9) > (unsigned)iBufLen)
    {
        NetPrintf(("protossl: could not add extension server_name; insufficient buffer\n"));
        return(0);
    }

    // renegotiation_info extension ident
    *pBuffer++ = (uint8_t)((SSL_EXTN_RENEGOTIATION_INFO>>8)&0xff);
    *pBuffer++ = (uint8_t)((SSL_EXTN_RENEGOTIATION_INFO>>0)&0xff);
    // empty renegotiation_info length
    *pBuffer++ = (uint8_t)0;
    *pBuffer++ = (uint8_t)1;
    // empty renegotiation_info
    *pBuffer++ = (uint8_t)0;

    return((int32_t)(pBuffer-pBufStart));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnWriteServerName

    \Description
        Write Server Name Indication (SNI) extension to the ClientHello handshake
        packet; ref http://tools.ietf.org/html/rfc6066#page-6

    \Input *pState      - module state reference
    \Input *pBuffer     - buffer to write extension into
    \Input iBufLen      - length of buffer

    \Output
        int32_t         - number of bytes added to buffer

    \Version 10/01/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHelloExtnWriteServerName(ProtoSSLRefT *pState, uint8_t *pBuffer, int32_t iBufLen)
{
    uint32_t uHostLen = (uint32_t)strlen(pState->strHost);
    uint8_t *pBufStart = pBuffer;

    // make sure we have enough room
    if ((uHostLen+9) > (unsigned)iBufLen)
    {
        NetPrintf(("protossl: could not add extension server_name; insufficient buffer\n"));
        return(0);
    }

    // server name extension ident
    *pBuffer++ = (uint8_t)(SSL_EXTN_SERVER_NAME>>8);
    *pBuffer++ = (uint8_t)(SSL_EXTN_SERVER_NAME>>0);
    // server name extension length
    *pBuffer++ = (uint8_t)((uHostLen+5) >> 8);
    *pBuffer++ = (uint8_t)(uHostLen+5);

    // server name list length
    *pBuffer++ = (uint8_t)((uHostLen+3) >> 8);
    *pBuffer++ = (uint8_t)(uHostLen+3);
    // type: hostname
    *pBuffer++ = 0;
    // hostname length
    *pBuffer++ = (uint8_t)(uHostLen >> 8);
    *pBuffer++ = (uint8_t)(uHostLen);

    // hostname
    ds_memcpy(pBuffer, pState->strHost, uHostLen);
    pBuffer += (int32_t)uHostLen;

    return((int32_t)(pBuffer-pBufStart));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnWriteSignatureAlgorithms

    \Description
        Write Signature Algorithms extension to the ClientHello handshake packet;
        ref https://tools.ietf.org/html/rfc5246#section-7.4.1.4.1

    \Input *pState      - module state reference
    \Input *pBuffer     - buffer to write extension into
    \Input iBufLen      - length of buffer

    \Output
        int32_t         - number of bytes added to buffer

    \Version 10/01/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHelloExtnWriteSignatureAlgorithms(ProtoSSLRefT *pState, uint8_t *pBuffer, int32_t iBufLen)
{
    const int32_t _iListLength = sizeof(_SSL3_SignatureSchemes)/sizeof(_SSL3_SignatureSchemes[0])*2; // signature hash algorithms list length
    const int32_t _iExtnLength = 2+_iListLength; // ident+length+algorithms list length
    SecureStateT *pSecure = pState->pSecure;
    uint8_t *pBufStart = pBuffer;
    int32_t iIndex;

    // as per RFC, this extension is not meaningful for TLS versions prior to 1.2; clients MUST NOT offer it if they are offering prior versions
    if (pSecure->uSslVersion < SSL3_TLS1_2)
    {
        return(0);
    }
    // make sure we have enough room
    if ((2+2+_iExtnLength) > iBufLen)
    {
        NetPrintf(("protossl: could not add extension signature_algorithms; insufficient buffer\n"));
        return(0);
    }

    // signature hash algorithms extension ident
    *pBuffer++ = (uint8_t)(SSL_EXTN_SIGNATURE_ALGORITHMS>>8);
    *pBuffer++ = (uint8_t)(SSL_EXTN_SIGNATURE_ALGORITHMS>>0);
    // signature hash algorithms extension length
    *pBuffer++ = (uint8_t)(_iExtnLength>>8);
    *pBuffer++ = (uint8_t)(_iExtnLength>>0);
    // signature hash algorithms extension list length
    *pBuffer++ = (uint8_t)(_iListLength>>8);
    *pBuffer++ = (uint8_t)(_iListLength>>0);
    // supported signature algorithms
    for (iIndex = 0; iIndex < (signed)(sizeof(_SSL3_SignatureSchemes)/sizeof(*_SSL3_SignatureSchemes)); iIndex += 1)
    {
        *pBuffer++ = (uint8_t)(_SSL3_SignatureSchemes[iIndex].uIdent>>8);
        *pBuffer++ = (uint8_t)(_SSL3_SignatureSchemes[iIndex].uIdent>>0);
    }

    return((int32_t)(pBuffer-pBufStart));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnWriteSupportedVersions

    \Description
        Write the Supported Versions extension to the ClientHello;
        ref https://tools.ietf.org/html/rfc8446#section-4.2.1

    \Input *pState      - module state reference
    \Input *pBuffer     - buffer to write extension into
    \Input iBufLen      - length of buffer

    \Output
        int32_t         - number of bytes added to buffer

    \Notes
        Unlike other extensions, supported_versions is an extension required to
        support TLS1.3.  A supported_versions extension overrides the legacy TLS
        client version in the ClientHello, which remains fixed at 1.2.

    \Version 01/24/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHelloExtnWriteSupportedVersions(ProtoSSLRefT *pState, uint8_t *pBuffer, int32_t iBufLen)
{
    int32_t iVersion, iNumVersions;
    int32_t iDataLength, iExtnLength;
    uint8_t *pBufStart = pBuffer;
    SecureStateT *pSecure = pState->pSecure;

    // list all versions if not server
    if (!pState->bServer)
    {
        iNumVersions = pState->uSslVersion - pState->uSslVersionMin + 1;
        iDataLength = (iNumVersions * 2) + 1;
    }
    else
    {
        iNumVersions = 1;
        iDataLength = 2;
    }
    // calculate extension length
    iExtnLength = 2 + 2 + iDataLength;

    // make sure we have enough room
    if (iExtnLength > iBufLen)
    {
        NetPrintf(("protossl: could not add extension supported_versions; insufficient buffer\n"));
        return(0);
    }
    
    // extension ident
    *pBuffer++ = (uint8_t)(SSL_EXTN_SUPPORTED_VERSIONS>>8);
    *pBuffer++ = (uint8_t)(SSL_EXTN_SUPPORTED_VERSIONS>>0);
    // extension length
    *pBuffer++ = (uint8_t)((iDataLength)>>8);
    *pBuffer++ = (uint8_t)((iDataLength)>>0);

    // add extension data
    if (!pState->bServer)
    {
        // size of versions list
        *pBuffer++ = (uint8_t)(iNumVersions*2);
        // supported_versions data
        for (iVersion = pState->uSslVersion; iVersion >= pState->uSslVersionMin; iVersion -= 1)
        {
            *pBuffer++ = (uint8_t)(iVersion>>8);
            *pBuffer++ = (uint8_t)(iVersion>>0);
        }
    }
    else
    {
        *pBuffer++ = (uint8_t)(pSecure->uSslVersion>>8);
        *pBuffer++ = (uint8_t)(pSecure->uSslVersion>>0);
    }

    return((int32_t)(pBuffer-pBufStart));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLAddHelloExtensions

    \Description
        Add any ClientHello extensions

    \Input *pState      - module state reference
    \Input *pBuffer     - buffer to write extension into
    \Input iBufLen      - length of buffer
    \Input *pHshkMsgBuf - handshake message buffer
    \Input uHelloExtn   - flag of which hello extensions to include

    \Output
        uint8_t *       - pointer past end of extension chunk

    \Notes
        ClientHello extension registry:
        http://www.iana.org/assignments/tls-extensiontype-values/tls-extensiontype-values.xhtml

    \Version 10/01/2014 (jbrookes)
*/
/********************************************************************************F*/
static uint8_t *_ProtoSSLAddHelloExtensions(ProtoSSLRefT *pState, uint8_t *pBuffer, int32_t iBufLen, const uint8_t *pHshkMsgBuf, uint32_t uHelloExtn)
{
    SecureStateT *pSecure = pState->pSecure;
    uint32_t uExtnLen;
    int32_t iBufOff, iPskOff = 0;

    // skip extensions length field
    iBufOff = 2;

    // add enabled extensions (client only)
    if (!pState->bServer)
    {
        if (uHelloExtn & PROTOSSL_HELLOEXTN_SERVERNAME)
        {
            iBufOff += _ProtoSSLHelloExtnWriteServerName(pState, pBuffer+iBufOff, iBufLen-iBufOff);
        }
        if (uHelloExtn & PROTOSSL_HELLOEXTN_ELLIPTIC_CURVES)
        {
            iBufOff += _ProtoSSLHelloExtnWriteEllipticCurves(pState, pBuffer+iBufOff, iBufLen-iBufOff);
        }
    }

    // add enabled extensions (client & server)
    if (uHelloExtn & PROTOSSL_HELLOEXTN_ALPN)
    {
        iBufOff += _ProtoSSLHelloExtnWriteAlpn(pState, pBuffer+iBufOff, iBufLen-iBufOff);
    }
    if ((!pState->bServer && (pState->uSslVersionMin < SSL3_TLS1_3)) || (pState->bServer && pSecure->bRenegotiationInfo && (pSecure->uSslVersion < SSL3_TLS1_3)))
    {
        iBufOff += _ProtoSSLHelloExtnWriteRenegotiationInfo(pState, pBuffer+iBufOff, iBufLen-iBufOff);
    }
    if ((pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3) && (pState->iState != ST3_SEND_EXTN) && (pState->iState != ST3_SEND_CERT_REQ))
    {
        iBufOff += _ProtoSSLHelloExtnWriteKeyShare(pState, pBuffer+iBufOff, iBufLen-iBufOff);
        iBufOff += _ProtoSSLHelloExtnWriteSupportedVersions(pState, pBuffer+iBufOff, iBufLen-iBufOff);
        iBufOff += _ProtoSSLHelloExtnWriteCookie(pState, pBuffer+iBufOff, iBufLen-iBufOff);
    }
    if (((uHelloExtn & PROTOSSL_HELLOEXTN_SIGALGS) && !pState->bServer) || ((pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3) && (!pState->bServer || pState->iState == ST3_SEND_CERT_REQ)))
    {
        iBufOff += _ProtoSSLHelloExtnWriteSignatureAlgorithms(pState, pBuffer+iBufOff, iBufLen-iBufOff);
    }

    // add psk-modes (client only)
    if (!pState->bServer)
    {
        iBufOff += _ProtoSSLHelloExtnWritePreSharedKeyModes(pState, pBuffer+iBufOff, iBufLen-iBufOff);
    }

    /* get pre-shared key length so we can write overall extension length.  we need that value
       to be correct when we actually write the pre_shared_key extension, so that we can calculate
       the binder hash correctly */
    if (!pState->bServer && (pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3))
    {
        int32_t iPskLen = _ProtoSSLHelloExtnWritePreSharedKey(pState, NULL, 0, NULL);
        iPskOff = iBufOff;
        if ((iBufOff+iPskLen) > iBufLen)
        {
            NetPrintf(("protossl: could not add extension psk_modes; insufficient buffer\n"));
            iPskLen = 0;
        }
        iBufOff += iPskLen;
    }

    // update extension length
    if (iBufOff > 2)
    {
        uExtnLen = (uint32_t)(iBufOff-2);
        pBuffer[0] = (uint8_t)(uExtnLen>>8);
        pBuffer[1] = (uint8_t)(uExtnLen>>0);
    }
    else
    {
        iBufOff = 0;
    }

    // add pre_shared_key extension
    if ((iBufOff > 2) && (iPskOff > 0))
    {
        _ProtoSSLHelloExtnWritePreSharedKey(pState, pBuffer+iPskOff, iBufLen-iPskOff, pHshkMsgBuf);
    }

    // return updated buffer pointer
    return(pBuffer+iBufOff);
}

/*
    hello extension parsing
*/

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnGetName

    \Description
        Get debug extension name for logging

    \Input uExtnId

    \Output
        const char *    - pointer to name, or "unknown" if not recognized

    \Version 04/11/2017 (jbrookes)
*/
/********************************************************************************F*/
static const char *_ProtoSSLHelloExtnGetName(uint32_t uExtnId)
{
    const char *pName = "unknown";
    if (uExtnId <= SSL_EXTN_MAX)
    {
        pName = _SSL3_strExtensionNames[uExtnId];
    }
    else if (uExtnId == SSL_EXTN_RENEGOTIATION_INFO)
    {
        pName = "renegotiation_info";
    }
    return(pName);
}
#endif

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnParseAlpn

    \Description
        Parse the ALPN extension from the client & server hello

    \Input *pState      - module state reference
    \Input *pData       - the extension data we are parsing
    \Input *pDataEnd    - end of extension data

    \Output
        int32_t - result of opertion (zero or ST_FAIL_* on error)

    \Version 08/05/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHelloExtnParseAlpn(ProtoSSLRefT *pState, const uint8_t *pData, const uint8_t *pDataEnd)
{
    SecureStateT *pSecure = pState->pSecure;
    uint16_t uAlpnExtensionLength = _SafeRead16(pData, pDataEnd);
    pData += 2;

    if (uAlpnExtensionLength == 0)
    {
        /* if the extension has data but the list length is zero
           treat this as handshake failure */
        NetPrintf(("protossl: received invalid alpn extension length in client hello\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
        return(ST_FAIL_SETUP);
    }

    if (!pState->bServer)
    {
        // save the negotiated protocol in the secure state so it can be queried (add 1 for nul terminator)
        _SafeReadString(pSecure->strAlpnProtocol, sizeof(pSecure->strAlpnProtocol), (const char *)pData+1, _SafeRead8(pData, pDataEnd), pDataEnd);
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: server negotiated protocol %s using alpn extension\n", pSecure->strAlpnProtocol));
    }
    else if (pState->uNumAlpnProtocols > 0) // skip parsing if the user has not chosed any protocols
    {
        uint32_t uProtocol;

        // loop through the protocols and pick the first supported
        for (uProtocol = 0; (uProtocol < pState->uNumAlpnProtocols) && (pSecure->strAlpnProtocol[0] == '\0'); uProtocol += 1)
        {
            const AlpnProtocolT *pProtocol = &pState->aAlpnProtocols[uProtocol];
            uint32_t uOffset, uDataLen;

            for (uOffset = 0; uOffset < uAlpnExtensionLength; uOffset += uDataLen+1)
            {
                const uint8_t *pExtensionData = pData+uOffset;
                uDataLen = _SafeRead8(pExtensionData, pDataEnd);
                _SafeReadString(pSecure->strAlpnProtocol, sizeof(pSecure->strAlpnProtocol), (const char *)pExtensionData+1, uDataLen, pDataEnd);

                if (!strcmp(pSecure->strAlpnProtocol, pProtocol->strName))
                {
                    NetPrintfVerbose((pState->iVerbose, 0, "protossl: negotiated protocol %s using alpn extension\n", pSecure->strAlpnProtocol));
                    break;
                }
                else
                {
                    pSecure->strAlpnProtocol[0] = '\0';
                }
            }
        }

        if (pSecure->strAlpnProtocol[0] == '\0')
        {
            /* if we did not match a protocol then we do not support any
               of the protocols that the client prefers; treat this as handshake failure */
            NetPrintf(("protossl: client requested protocols that are not supported by the server via alpn extension\n"));
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_NO_APPLICATION_PROTOCOL);
            return(ST_FAIL_SETUP);
        }
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnParseCookie

    \Description
        Parse the Cookie extension from the client & server hello

    \Input *pState      - module state reference
    \Input *pData       - the extension data we are parsing
    \Input *pDataEnd    - end of extension data

    \Output
        int32_t - result of opertion (zero or ST_FAIL_* on error)

    \Version 11/29/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHelloExtnParseCookie(ProtoSSLRefT *pState, const uint8_t *pData, const uint8_t *pDataEnd)
{
    SecureStateT *pSecure = pState->pSecure;

    // point to cookie in receive buffer so we don't have to waste memory saving it off
    pSecure->pCookie = pData;
    NetPrintfVerbose((pState->iVerbose, 0, "protossl: parsed cookie of length %d\n", _SafeRead16(pData, pDataEnd)));

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnParseEllipticCurves

    \Description
        Parse the elliptic curves extension from the client hello

    \Input *pState      - module state reference
    \Input *pData       - the extension data we are parsing
    \Input *pDataEnd    - end of extension data

    \Output
        int32_t - result of opertion (zero or ST_FAIL_* on error)

    \Version 01/19/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHelloExtnParseEllipticCurves(ProtoSSLRefT *pState, const uint8_t *pData, const uint8_t *pDataEnd)
{
    int32_t iEllipticCurve, iIndex;
    SecureStateT *pSecure = pState->pSecure;
    uint16_t uEllipticCurveExtensionLength = _SafeRead16(pData, pDataEnd);
    pData += 2;

    if (uEllipticCurveExtensionLength == 0)
    {
        /* if the extension has data but the list length is zero
           treat this as handshake failure */
        NetPrintf(("protossl: received invalid elliptic curves extension length in client hello\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
        return(ST_FAIL_SETUP);
    }

    // pick first supported elliptic curve
    for (iEllipticCurve = 0, pSecure->pEllipticCurve = NULL; (iEllipticCurve < (signed)(uEllipticCurveExtensionLength/2)) && (pSecure->pEllipticCurve == NULL); iEllipticCurve += 1, pData += 2)
    {
        for (iIndex = 0; iIndex < (signed)(sizeof(_SSL3_EllipticCurves)/sizeof(*_SSL3_EllipticCurves)); iIndex += 1)
        {
            if ((pData+2) > pDataEnd)
            {
                continue;
            }
            // skip non-matching elliptic curve
            if ((pData[0] != (uint8_t)(_SSL3_EllipticCurves[iIndex].uIdent >> 8)) || (pData[1] != (uint8_t)(_SSL3_EllipticCurves[iIndex].uIdent)))
            {
                continue;
            }
            // skip disabled curves
            if ((_SSL3_EllipticCurves[iIndex].uId & pState->uEnabledCurves) == 0)
            {
                continue;
            }
            // found an elliptic curve
            pSecure->pEllipticCurve = &_SSL3_EllipticCurves[iIndex];
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: using elliptic curve %s (ident=0x%04x)\n", pSecure->pEllipticCurve->strName, pSecure->pEllipticCurve->uIdent));
            break;
        }
    }

    // ecdhe key exchange is required in TLS1.3+. if we cannot find a curve that we share in common, we need to fail the handshake
    if ((pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3) && (pSecure->pEllipticCurve == NULL))
    {
        NetPrintf(("protossl: no elliptic curve found\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
        return(ST_FAIL_SETUP);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnParseKeyShare

    \Description
        Parse the key share extension from the hello (tls1.3)

    \Input *pState      - module state reference
    \Input *pData       - the extension data we are parsing
    \Input *pDataEnd    - end of extension data

    \Output
        int32_t - result of opertion (zero or ST_FAIL_* on error)

    \Version 01/25/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHelloExtnParseKeyShare(ProtoSSLRefT *pState, const uint8_t *pData, const uint8_t *pDataEnd)
{
    SecureStateT *pSecure = pState->pSecure;
    int32_t iKeyShare, iIndex;
    uint16_t uKeyShareCurveId;
    uint32_t uKeyShareLen;
    uint32_t uExtnLen;

    NetPrintfVerbose((pState->iVerbose, 0, "protossl: parsing keyshare extension\n"));

    // process extension length; server only
    if (pState->bServer)
    {
        if ((uExtnLen = _SafeRead16(pData, pDataEnd)) == 0)
        {
            NetPrintf(("protossl: received empty key_share\n"));
        }
        pDataEnd = DS_MIN(&pData[2]+uExtnLen, pDataEnd);
        pData += 2;
    }

    // iterate through array of key share objects
    for (iKeyShare = 0, uKeyShareLen = 0, pSecure->pEllipticCurve = NULL; (pData < pDataEnd) && (pSecure->pEllipticCurve == NULL); iKeyShare += 1)
    {
        // get keyshare object curve id
        uKeyShareCurveId = _SafeRead16(pData, pDataEnd);
        pData += 2;
        // get keyshare size (if key is present)
        if (pData < pDataEnd)
        {
            uKeyShareLen = _SafeRead16(pData, pDataEnd);
            // skip to keyshare data
            pData += 2;
        }

        // see if we have a match
        for (iIndex = 0; iIndex < (signed)(sizeof(_SSL3_EllipticCurves)/sizeof(*_SSL3_EllipticCurves)); iIndex += 1)
        {
            // skip non-matching elliptic curve
            if (uKeyShareCurveId != _SSL3_EllipticCurves[iIndex].uIdent)
            {
                continue;
            }
            // skip disabled curves
            if ((_SSL3_EllipticCurves[iIndex].uId & pState->uEnabledCurves) == 0)
            {
                continue;
            }
            // found an elliptic curve
            pSecure->pEllipticCurve = &_SSL3_EllipticCurves[iIndex];

            // save key
            if (uKeyShareLen > 0)
            {
                _SafeReadBytes(pSecure->PubKey, sizeof(pSecure->PubKey), pData, uKeyShareLen, pDataEnd);
                pSecure->uPubKeyLength = uKeyShareLen;
            }

            NetPrintfVerbose((pState->iVerbose, 0, "protossl: using elliptic curve %s (ident=0x%04x)\n", pSecure->pEllipticCurve->strName, pSecure->pEllipticCurve->uIdent));
            #if DEBUG_RAW_DATA
            NetPrintMem(pSecure->PubKey, pSecure->uPubKeyLength, "key_share");
            #endif
            break;
        }
        // move to next keyshare object
        pData += uKeyShareLen;
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnParsePreSharedKey

    \Description
        Parse the pre_shared_key extension from the hello (tls1.3)

    \Input *pState      - module state reference
    \Input *pData       - the extension data we are parsing
    \Input *pDataEnd    - end of extension data

    \Output
        int32_t     - result of opertion (zero or ST_FAIL_* on error)

    \Version 12/13/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHelloExtnParsePreSharedKey(ProtoSSLRefT *pState, const uint8_t *pData, const uint8_t *pDataEnd)
{
    SecureStateT *pSecure = pState->pSecure;

    NetPrintfVerbose((pState->iVerbose, 0, "protossl: parsing pre_shared_key extension\n"));

    if (pState->bServer)
    {
        //$$TODO add server parsing of psk
    }
    else
    {
        uint16_t uIdentity = _SafeRead16(pData, pDataEnd);
        pSecure->bSessionResume = (uIdentity == 0) ? TRUE : FALSE;
        NetPrintf(("protossl: psk selected identity=%d; resume %s\n", uIdentity, pSecure->bSessionResume ? "true" : "false"));
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnParseRenegotiationInfo

    \Description
        Parse the Renegotiation Info extension from the client & server hello
        ref https://tools.ietf.org/html/rfc5746

    \Input *pState      - module state reference
    \Input *pData       - the extension data we are parsing
    \Input *pDataEnd    - end of extension data

    \Output
        int32_t - result of opertion (zero or ST_FAIL_* on error)

    \Version 03/27/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHelloExtnParseRenegotiationInfo(ProtoSSLRefT *pState, const uint8_t *pData, const uint8_t *pDataEnd)
{
    SecureStateT *pSecure = pState->pSecure;
    pSecure->bRenegotiationInfo = TRUE;
    NetPrintfVerbose((pState->iVerbose, 0, "protossl: parsed renegotation_info\n"));
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnParseSignatureAlgorithms

    \Description
        Parse the signature algorithms (tls1.3 calls them signature
        schemes) extension.

    \Input *pState      - module state reference
    \Input *pData       - the extension data we are parsing
    \Input *pDataEnd    - end of extension data

    \Output
        int32_t - result of opertion (zero or ST_FAIL_* on error)

    \Notes
        This is needed when sending ServerKeyExchange messages to the client

    \Version 03/03/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHelloExtnParseSignatureAlgorithms(ProtoSSLRefT *pState, const uint8_t *pData, const uint8_t *pDataEnd)
{
    SecureStateT *pSecure = pState->pSecure;
    uint16_t uSigSchemeLen;

    // get sig scheme length
    uSigSchemeLen = _SafeRead16(pData, pDataEnd);
    pData += 2;

    // if the extension has data but the list length is zero treat this as handshake failure
    if (uSigSchemeLen == 0)
    {
        NetPrintf(("protossl: received invalid signature algorithms length in client hello\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
        return(ST_FAIL_SETUP);
    }

    // pick first supported signature algorithm
    pSecure->pSigScheme = _ProtoSSLChooseSignatureScheme(pSecure, pState->pCertificate, pData, uSigSchemeLen, pDataEnd);

    /* tls1.2 https://tools.ietf.org/html/rfc5246#section-7.4.1.4.1) and tls1.3 (https://tools.ietf.org/html/rfc8446#section-9.2)
       define actions around the presence/absence of this extension, but do not define actions in the case no suitable signature algorithms 
       are included.  we treat this as a handshake failure based around observed behavior of other implementations e.g. openssl */
    if (pSecure->pSigScheme == NULL)
    {
        NetPrintf(("protossl: no acceptable signature algorithms included in signature_algorithms extension\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
        return(ST_FAIL_SETUP);
    }

    // log choice and return success
    NetPrintfVerbose((pState->iVerbose, 0, "protossl: using signature scheme 0x%04x\n", pSecure->pSigScheme->uIdent));
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLHelloExtnParseSupportedVersions

    \Description
        Parse the Supported Versions extension from the ClientHello
        ref https://tools.ietf.org/html/rfc8446#section-4.2.1

    \Input *pState      - module state reference
    \Input *pData       - the extension data we are parsing
    \Input *pDataEnd    - end of extension data

    \Output
        int32_t - result of opertion (zero or ST_FAIL_* on error)

    \Version 01/26/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLHelloExtnParseSupportedVersions(ProtoSSLRefT *pState, const uint8_t *pData, const uint8_t *pDataEnd)
{
    SecureStateT *pSecure = pState->pSecure;
    uint32_t uExtnLength = pState->bServer ? _SafeRead8(pData++, pDataEnd) : 2;
    uint32_t uSslVersion;

    // pick first valid supported version
    for (pDataEnd = pData + uExtnLength; pData < pDataEnd; pData += 2)
    {
        uSslVersion = _SafeRead16(pData, pDataEnd);

        // range check
        if ((uSslVersion < pState->uSslVersionMin) || (uSslVersion > pState->uSslVersion))
        {
            continue;
        }

        // found a version, roll with it
        pSecure->uSslClientVersion = pSecure->uSslVersion = uSslVersion;
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: ssl protocol version overriden by supported_versions extension to %s\n", _SSL3_strVersionNames[pSecure->uSslVersion & 0xff]));
        return(0);
    }

    /* if the "supported_versions" extension in the ServerHello contains a version not offered by the client or contains
       a version prior to TLS 1.3, the client MUST abort the handshake with an "illegal_parameter" alert */
    if (!pState->bServer)
    {
        NetPrintf(("protossl: supported_versions extension did not contain a valid version\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_ILLEGAL_PARAMETER);
        return(ST_FAIL_SETUP);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLParseHelloExtensions

    \Description
        Parse the hello extensions in the client & server hello handshake packets

    \Input *pState              - module state reference
    \Input *pData               - start of extension data (length word)
    \Input *pDataEnd            - end of the payload data
    \Input uExtensionParse      - id of extension to parse, or -1 to parse all

    \Output
        int32_t - result of opertion (zero or ST_FAIL_* on error)

    \Version 08/05/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLParseHelloExtensions(ProtoSSLRefT *pState, const uint8_t *pData, const uint8_t *pDataEnd, uint16_t uExtensionParse)
{
    SecureStateT *pSecure = pState->pSecure;
    uint16_t uExtensionType, uExtensionLength;
    int32_t iResult = 0;

    // read overall length of extensions, and constrain reading to extension end
    uExtensionLength = _SafeRead16(pData, pDataEnd);
    pDataEnd = DS_MIN(&pData[2]+uExtensionLength, pDataEnd);
    pData += 2;

    // parse the complete extension section while we have not encountered an error
    for (iResult = 0; (pData < pDataEnd) && (iResult == 0); pData += uExtensionLength)
    {
        // get extension type and length
        uExtensionType = _SafeRead16(pData, pDataEnd);
        uExtensionLength = _SafeRead16(pData+2, pDataEnd);
        // skip header
        pData += 4;

        // validate extension fits in available space
        if ((pData+uExtensionLength) > pDataEnd)
        {
            NetPrintf(("protossl: extension length %d too big\n", uExtensionLength));
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_DECODE_ERROR);
            return(ST_FAIL_SETUP);
        }

        // check for specific extension parsing
        if ((uExtensionParse != SSL_EXTN_ALL) && (uExtensionParse != uExtensionType))
        {
            continue;
        }

        NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: parsing extension %s (%d) with length %d\n", _ProtoSSLHelloExtnGetName(uExtensionType), uExtensionType, uExtensionLength));

        // skip extensions without data
        if (uExtensionLength == 0)
        {
            continue;
        }

        #if DEBUG_RAW_DATA
        NetPrintMem(pData, uExtensionLength, "extension data");
        #endif

        // parse extensions only clients send
        if (pState->bServer)
        {
            if (uExtensionType == SSL_EXTN_ELLIPTIC_CURVES)
            {
                iResult = _ProtoSSLHelloExtnParseEllipticCurves(pState, pData, pDataEnd);
            }
        }

        // parse ALPN extension
        if (uExtensionType == SSL_EXTN_ALPN)
        {
            iResult = _ProtoSSLHelloExtnParseAlpn(pState, pData, pDataEnd);
        }

        // the following extensions are only parsed if this is a tls1.2 or greater connection request
        if (pSecure->uSslVersion < SSL3_TLS1_2)
        {
            continue;
        }

        // parse supported_versions extension only if tls1.3 is enabled, hello version is 1.2, and we are explicitly parsing for it
        if ((pState->uSslVersion >= SSL3_TLS1_3) && (pSecure->uSslVersion == SSL3_TLS1_2) && (uExtensionType == SSL_EXTN_SUPPORTED_VERSIONS) && (uExtensionParse == uExtensionType))
        {
            iResult = _ProtoSSLHelloExtnParseSupportedVersions(pState, pData, pDataEnd);
        }

        // parse SignatureAlgorithms
        if (uExtensionType == SSL_EXTN_SIGNATURE_ALGORITHMS)
        {
            iResult = _ProtoSSLHelloExtnParseSignatureAlgorithms(pState, pData, pDataEnd);
        }
        if (uExtensionType == SSL_EXTN_RENEGOTIATION_INFO)
        {
            iResult = _ProtoSSLHelloExtnParseRenegotiationInfo(pState, pData, pDataEnd);
        }

        // the following extensions are only parsed if this is a tls1.3 connection request
        if (pSecure->uSslVersion < SSL3_TLS1_3)
        {
            continue;
        }

        // parse cookie extension
        if (uExtensionType == SSL_EXTN_COOKIE)
        {
            iResult = _ProtoSSLHelloExtnParseCookie(pState, pData, pDataEnd);
        }
        // parse KeyShare extnesion
        if (uExtensionType == SSL_EXTN_KEY_SHARE)
        {
            iResult = _ProtoSSLHelloExtnParseKeyShare(pState, pData, pDataEnd);
        }
        // parse PreSharedKey extnesion
        if (uExtensionType == SSL_EXTN_PRE_SHARED_KEY)
        {
            iResult = _ProtoSSLHelloExtnParsePreSharedKey(pState, pData, pDataEnd);
        }
    }

    return(iResult);
}

/*
    handshaking
*/

/*F********************************************************************************/
/*!
    \Function _ProtoSSLSendClientHello

    \Description
        Send ClientHello handshake packet; ref http://tools.ietf.org/html/rfc5246#section-7.4.1.2
        for TLS1.2 and https://tools.ietf.org/html/rfc8446#section-4.1.2 for TLS1.3

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST3_SEND_HELLO, ST3_RECV_HELLO, ST_FAIL_SETUP)

    \Version 03/15/2013 (jbrookes) Added session resume support
*/
/********************************************************************************F*/
static int32_t _ProtoSSLSendClientHello(ProtoSSLRefT *pState)
{
    int32_t iCipher, iNumCiphers, iBodyLen;
    uint8_t strHead[4], strBody[2048];
    uint8_t *pData = strBody, *pCiphSize;
    SecureStateT *pSecure = pState->pSecure;
    SessionHistoryT SessHist;
    uint32_t uHelloExtn = pState->uHelloExtn, uEnabledCiphers = pState->uEnabledCiphers;

    // if we haven't picked an elliptic curve, pick our default
    if ((pSecure->pEllipticCurve == NULL) && (pState->iCurveDflt != -1))
    {
        pSecure->pEllipticCurve = _ProtoSSLChooseCurve(pState->uEnabledCurves, pState->iCurveDflt);
    }

    // for tls1.3 start ecdhe key generation here so we can spread it out
    if ((pState->uSslVersion >= PROTOSSL_VERSION_TLS1_3) && (pSecure->pEllipticCurve != NULL))
    {
        const CryptCurveDhT *pEcc;
        uint32_t uCryptUsecs;

        // initialize elliptic curve context if not already initialized
        if ((pEcc = _ProtoSSLEccInitContext(pSecure, pSecure->pEllipticCurve)) == NULL)
        {
            /* if we cannot find the dh functions, there is some configuration mishap or the server is faulty.
               let's fail early here so we can debug the issue instead of a null pointer exception */
            return(ST_FAIL_SETUP);
        }
        // generate the public key
        if (pEcc->Public(pSecure->EccContext, NULL, &uCryptUsecs) > 0)
        {
            return(ST3_SEND_HELLO);
        }
        pSecure->uPubKeyLength = pEcc->PointFinal(pSecure->EccContext, NULL, FALSE, pSecure->PubKey, sizeof(pSecure->PubKey));
        pSecure->bEccKeyGenerated = TRUE;
        NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (generate public key for server key message) %dms\n",
            uCryptUsecs/1000));
        pSecure->uTimer += uCryptUsecs/1000;
    }

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send ClientHello\n"));

    // initialize the ssl performance timer
    pSecure->uTimer = 0;

    // reset client cert level (server will let us know if we need to send one)
    pState->iClientCertLevel = 0;

    // set desired ssl version
    pSecure->uSslVersion = pState->uSslVersion;
    // remember requested version; this is later used to detect a version rollback attack
    pSecure->uSslClientVersion = pState->uSslVersion;

    // set TLS1.3 specific options
    if (pSecure->uSslClientVersion >= PROTOSSL_VERSION_TLS1_3)
    {
        // freeze hello version at 1.2 as per https://tools.ietf.org/html/rfc8446#section-4.1.2
        pSecure->uSslClientVersion = PROTOSSL_VERSION_TLS1_2;
        // add required elliptic_curves (which TLS1.3 calls supported_groups)
        uHelloExtn |= PROTOSSL_HELLOEXTN_ELLIPTIC_CURVES;
        // if no tls1.3 ciphers are enabled, do that now
        if ((pState->uSslVersion >= PROTOSSL_VERSION_TLS1_3) && !(uEnabledCiphers & PROTOSSL_CIPHER_ALL_13))
        {
            uEnabledCiphers |= PROTOSSL_CIPHER_ALL_13;
        }
    }
    *pData++ = (uint8_t)(pSecure->uSslClientVersion>>8);
    *pData++ = (uint8_t)(pSecure->uSslClientVersion>>0);

    // set random data
    if (pState->uSslVersion < PROTOSSL_VERSION_TLS1_3)
    {
        // TLS1.2 and prior specify first four bytes of client random are utc time
        uint32_t uUtcTime = (uint32_t)ds_timeinsecs();
        pSecure->ClientRandom[0] = (uint8_t)(uUtcTime >> 24);
        pSecure->ClientRandom[1] = (uint8_t)(uUtcTime >> 16);
        pSecure->ClientRandom[2] = (uint8_t)(uUtcTime >> 8);
        pSecure->ClientRandom[3] = (uint8_t)(uUtcTime >> 0);
        CryptRandGet(&pSecure->ClientRandom[4], sizeof(pSecure->ClientRandom) - 4);
    }
    else
    {
        // TLS1.3 has 32 bytes of random data
        CryptRandGet(pSecure->ClientRandom, sizeof(pSecure->ClientRandom));
    }
    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->ClientRandom, sizeof(pSecure->ClientRandom), "ClientRandom");
    #endif
    // set client random in packet
    ds_memcpy(pData, pSecure->ClientRandom, sizeof(pSecure->ClientRandom));
    pData += 32;

    // if we have a previous session for this peer and resume is enabled, set it
    if ((_SessionHistoryGetInfo(&SessHist, pState->strHost, SockaddrInGetPort(&pState->PeerAddr), NULL) != NULL) && pState->bSessionResumeEnabled)
    {
        SessionInfoT *pSessInfo = &SessHist.SessionInfo;
        NetPrintfVerbose((DEBUG_RES_SESS, 0, "protossl: setting session id for resume\n"));
        #if DEBUG_RES_SESS && DEBUG_RAW_DATA
        NetPrintMem(pSessInfo->SessionId, sizeof(pSessInfo->SessionId), "ClientHello session id");
        #endif
        /* save session id and master secret; the sessionid will be compared when we
           receive the serverhello to determine if we are in the resume flow or not */
        ds_memcpy(pSecure->SessionId, pSessInfo->SessionId, sizeof(pSecure->SessionId));
        ds_memcpy(pSecure->MasterKey, pSessInfo->MasterSecret, sizeof(pSecure->MasterKey));
        pSecure->bSentSessionId = TRUE;
    }
    else if (pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3)
    {
        /* as per https://tools.ietf.org/html/rfc8446#section-4.1.2 a tls 1.3 client MUST always send
           a non-empty sessionID in support of middlebox compatibility mode */
        if (!pSecure->bSentSessionId)
        {
            /* generate a new session id if we haven't sent one already; otherwise, in an HRR flow we
               need to resend the same sessionid as we did the first time */
            CryptRandGet(pSecure->SessionId, sizeof(pSecure->SessionId));
            pSecure->bSentSessionId = TRUE;
        }
    }
    else
    {
        pSecure->bSentSessionId = FALSE;
    }

    // add sessionid to ClientHello
    if (pSecure->bSentSessionId)
    {
        *pData++ = (uint8_t)sizeof(pSecure->SessionId);
        ds_memcpy(pData, pSecure->SessionId, sizeof(pSecure->SessionId));
        pData += sizeof(pSecure->SessionId);
    }
    else // add in empty session identifier
    {
        *pData++ = 0;
    }

    // add the cipher suite list
    *pData++ = 0;
    pCiphSize = pData++; // save and skip cipher size location
    for (iCipher = 0, iNumCiphers = 0, pSecure->uSentCiphers = 0; iCipher < (signed)(sizeof(_SSL3_CipherSuite)/sizeof(_SSL3_CipherSuite[0])); iCipher += 1)
    {
        // skip ciphers that require a newer version of SSL than we are offering
        if (_SSL3_CipherSuite[iCipher].uMinVers > pState->uSslVersion)
        {
            continue;
        }
        // skip disabled ciphers
        if ((_SSL3_CipherSuite[iCipher].uId & uEnabledCiphers) == 0)
        {
            continue;
        }
        // add cipher to list
        *pData++ = (uint8_t)(_SSL3_CipherSuite[iCipher].uIdent>>8);
        *pData++ = (uint8_t)(_SSL3_CipherSuite[iCipher].uIdent>>0);
        iNumCiphers += 1;
        // remember we sent it
        pSecure->uSentCiphers |= _SSL3_CipherSuite[iCipher].uId;
        // add the elliptic curve extension if we are using any ecdhe ciphers
        if (_SSL3_CipherSuite[iCipher].uKey == SSL3_KEY_ECDHE)
        {
            uHelloExtn |= PROTOSSL_HELLOEXTN_ELLIPTIC_CURVES;
        }
    }
    // make sure we selected at least one cipher
    if (iNumCiphers == 0)
    {
        NetPrintf(("protossl: no ciphers selected in ClientHello\n"));
        return(ST_FAIL_SETUP);
    }
    // write cipher suite list size
    *pCiphSize = iNumCiphers*2;

    // add compression_methods list with only null compression (we don't support compression)
    *pData++ = 1;
    *pData++ = 0;

    // add extensions
    if (uHelloExtn != 0)
    {
        pData = _ProtoSSLAddHelloExtensions(pState, pData, (uint32_t)sizeof(strBody)-(uint32_t)(pData-strBody), strBody, uHelloExtn);
    }

    // setup the header
    iBodyLen = pData-strBody;
    strHead[0] = SSL3_MSG_CLIENT_HELLO;
    strHead[1] = 0;
    strHead[2] = (uint8_t)(iBodyLen>>8);
    strHead[3] = (uint8_t)(iBodyLen>>0);

    // if in hrr flow, make sure we're not sending the same clienthello we sent last time
    if (pSecure->uSslVersion >= SSL3_TLS1_3)
    {
        uint8_t aClientHelloHash[sizeof(pSecure->aClientHelloHash)];
        // hash clienthello for possible hrr compare
        _ProtoSSLGenerateFingerprint(aClientHelloHash, sizeof(aClientHelloHash), strBody, iBodyLen, CRYPTHASH_SHA256);
        // if in hrr flow, compare
        if ((pSecure->bHelloRetry) && (!memcmp(aClientHelloHash, pSecure->aClientHelloHash, sizeof(aClientHelloHash))))
        {
            /* as per https://tools.ietf.org/html/rfc8446#section-4.1.4 sending the same clienthello twice
               must generate an illegal_parameter alert */
            NetPrintf(("protossl: sending the same ClientHello twice\n"));
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_ILLEGAL_PARAMETER);
            return(ST_FAIL_SETUP);

        }
        // copy hello hash to state for possible future compare
        ds_memcpy_s(pSecure->aClientHelloHash, sizeof(pSecure->aClientHelloHash), aClientHelloHash, sizeof(aClientHelloHash));
    }

    // set starting handshake message
    pSecure->iCurMsg = SSL3_MSG_CLIENT_HELLO;

    // setup packet for send
    _ProtoSSLSendPacket(pState, SSL3_REC_HANDSHAKE, strHead, 4, strBody, iBodyLen);
    return(ST3_RECV_HELLO);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvClientHello

    \Description
        Process ClientHello handshake packet

    \Input *pState      - module state reference
    \Input *pData       - packet data
    \Input iDataSize    - size of packet data

    \Output
        int32_t         - next state (ST3_SEND_HELLO, ST3_SEND_HELLO_RETRY, or ST_FAIL_* on error)

    \Version 10/15/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLRecvClientHello(ProtoSSLRefT *pState, const uint8_t *pData, int32_t iDataSize)
{
    SecureStateT *pSecure = pState->pSecure;
    int32_t iNumCiphers, iParseResult;
    const uint8_t *pDataEnd = pData + iDataSize;
    const uint8_t *pCipherList;
    SessionHistoryT SessHist;
    uint32_t uEnabledCiphers = pState->uEnabledCiphers, uPreferredCipher=0, uSessionSize;
    uint8_t aSessionId[SSL_SESSID_SIZE], uCompression, uCompressionMethods;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process ClientHello\n"));

    // get protocol version client wants from us; make sure it is at most TLS1.2, as that is the highest allowed version for this field
    if ((pSecure->uSslClientVersion = pSecure->uSslVersion = _SafeRead16(pData, pDataEnd)) > PROTOSSL_VERSION_TLS1_2)
    {
        NetPrintf(("protossl: invalid ssl version %d in client hello\n", pSecure->uSslClientVersion));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_ILLEGAL_PARAMETER);
        return(ST_FAIL_CONN);
    }
    pData += 2;

    // save client random data
    _SafeReadBytes(pSecure->ClientRandom, sizeof(pSecure->ClientRandom), pData, sizeof(pSecure->ClientRandom), pDataEnd);
    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->ClientRandom, sizeof(pSecure->ClientRandom), "ClientRandom");
    #endif
    pData += 32;

    // check for possible session resume
    uSessionSize = _SafeRead8(pData++, pDataEnd);
    _SafeReadBytes(aSessionId, sizeof(aSessionId), pData, SSL_SESSID_SIZE, pDataEnd);
    pData += uSessionSize;

    // read cipher suite list size and save cipher list pointer (we will parse it after extensions, if present)
    iNumCiphers = _SafeRead16(pData, pDataEnd) / 2;
    pData += 2;
    pCipherList = pData;
    // skip cipher list
    pData += iNumCiphers*2;

    // save and skip compression_methods
    uCompressionMethods = _SafeRead8(pData++, pDataEnd);
    // read first method; allows us to later check for one null method if we have a tls1.3 connection
    uCompression = _SafeRead8(pData, pDataEnd);
    pData += uCompressionMethods;

    // parse hello extensions for supported_versions first; this allows extension parsers to know if the connection will be tls1.3 or not
    if ((iParseResult = _ProtoSSLParseHelloExtensions(pState, pData, pDataEnd, SSL_EXTN_SUPPORTED_VERSIONS)) != 0)
    {
        return(iParseResult);
    }

    // pick protocol version; this must be done after possible supported_versions extension parsing
    if (pSecure->uSslVersion > pState->uSslVersion)
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protossl: client requested SSL version %d.%d, downgrading to our max supported version\n", pData[0], pData[1]));
        pSecure->uSslVersion = pState->uSslVersion;
    }
    else if (pSecure->uSslVersion < pState->uSslVersionMin)
    {
        NetPrintf(("protossl: client requested SSL version %d.%d is not supported\n", pData[0], pData[1]));
        /* As per http://tools.ietf.org/html/rfc5246#appendix-E.1, if server supports (or is willing to use)
           only versions greater than client_version, it MUST send a "protocol_version" alert message and
           close the connection. */
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_PROTOCOL_VERSION);
        return(ST_FAIL_CONN_MINVERS);
    }
    NetPrintfVerbose((pState->iVerbose, 0, "protossl: using %s\n", _SSL3_strVersionNames[pSecure->uSslVersion&0xff]));

    // parse all other extensions
    if ((iParseResult = _ProtoSSLParseHelloExtensions(pState, pData, pDataEnd, SSL_EXTN_ALL)) != 0)
    {
        return(iParseResult);
    }

    // check for session resume info
    if (pSecure->uSslVersion < PROTOSSL_VERSION_TLS1_3)
    {
        if (pState->bSessionResumeEnabled)
        {
            // get info for possible resume
            if ((uSessionSize == sizeof(pSecure->SessionId)) && _SessionHistoryGetInfo(&SessHist, NULL, 0, aSessionId))
            {
                // flag that we got the info
                pSecure->bSessionResume = TRUE;
                // save cipher preference
                uPreferredCipher = SessHist.SessionInfo.uCipherId;
            }
        }
    }
    else
    {
        // save session info for later echo as per https://tools.ietf.org/html/rfc8446#section-4.1.3
        if (uSessionSize == sizeof(pSecure->SessionId))
        {
            ds_memcpy(pSecure->SessionId, aSessionId, sizeof(pSecure->SessionId));
            pSecure->bSessionResume = TRUE;
        }
        else
        {
            pSecure->bSessionResume = FALSE;
        }
        // if we are in the HelloRetryRequest flow, set the preferred cipher to the cipher we previously selected
        if (pSecure->bHelloRetry)
        {
            uPreferredCipher = pSecure->uRetryCipher;
        }
    }

    // choose a cipher from cipher list, with optional cipher preference
    if ((pSecure->pCipher = _ProtoSSLChooseCipher(pSecure, pState->pCertificate, pCipherList, iNumCiphers, pDataEnd, uEnabledCiphers, uPreferredCipher)) == NULL)
    {
        NetPrintf(("protossl: no matching cipher\n"));
        /* As per http://tools.ietf.org/html/rfc5246#section-7.4.1.3, a client providing no
           ciphers we support results in a handshake failure alert */
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
        return(ST_FAIL_CONN_NOCIPHER);
    }
    else
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: using cipher suite %s (ident=%d,%d)\n", pSecure->pCipher->strName, pCipherList[0], pCipherList[1]));
    }

    // make sure legacy_compression field includes only null compression if tls 1.3
    if ((pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3) && ((uCompressionMethods != 1) || (uCompression != 0)))
    {
        /* as per https://tools.ietf.org/html/rfc8446#section-4.1.2 compression must be disabled; "if a TLS 1.3 ClientHello is
           received with any other value in this field, the server MUST abort the handshake with an "illegal_parameter" alert */
        NetPrintf(("protossl: legacy_compression_methods not null for tls 1.3 connection\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_ILLEGAL_PARAMETER);
        return(ST_FAIL_CONN);
    }

    // now that we've selected the cipher, check for possible resume
    if ((pSecure->uSslVersion < SSL3_TLS1_3) && (pSecure->bSessionResume))
    {
        SessionInfoT *pSessInfo = &SessHist.SessionInfo;
        if ((pSessInfo->uSslVersion == pSecure->uSslVersion) && (pSessInfo->uCipherId == pSecure->pCipher->uIdent))
        {
            ds_memcpy(pSecure->SessionId, aSessionId, sizeof(pSecure->SessionId));
            #if DEBUG_RAW_DATA
            NetPrintMem(pSecure->SessionId, sizeof(pSecure->SessionId), "ClientHello session id");
            #endif
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: resuming previous session\n"));
            // copy session id and master secret
            ds_memcpy(pSecure->MasterKey, pSessInfo->MasterSecret, sizeof(pSecure->MasterKey));
            ds_memcpy(pSecure->SessionId, pSessInfo->SessionId, sizeof(pSecure->SessionId));
        }
        else
        {
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: no session resume due to cipher or version mismatch\n"));
            pSecure->bSessionResume = FALSE;
        }
    }

    // if no session resume, generate a new sessionid for possible future reuse
    if ((pSecure->uSslVersion < SSL3_TLS1_3) && !pSecure->bSessionResume)
    {
        CryptRandGet(pSecure->SessionId, sizeof(pSecure->SessionId));
    }

    // make sure we found a key share we support
    if ((pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3) && ((pSecure->pEllipticCurve == NULL) || (pSecure->uPubKeyLength == 0)))
    {
        // pick default key share
        pSecure->pEllipticCurve = _ProtoSSLChooseCurve(pState->uEnabledCurves, pState->iCurveDflt);
        return(ST3_SEND_HELLO_RETRY);
    }

    // if not tls1.3 and we got a key_share from a 1.3 client, zero it here so we don't think it's a valid key later in the handshake flow
    if ((pSecure->uSslVersion < SSL3_TLS1_3) && (pSecure->uPubKeyLength != 0))
    {
        pSecure->uPubKeyLength = 0;
    }

    return(ST3_SEND_HELLO);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLSendHelloRetryRequest

    \Description
        Send Hello Retry Request (TLS 1.3+) as defined by
        https://tools.ietf.org/html/rfc8446#section-4.1.4

    \Input *pState      - module state reference

    \Output
        int32_t         - ST3_RECV_HELLO

    \Version 08/08/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLSendHelloRetryRequest(ProtoSSLRefT *pState)
{
    uint8_t strBody[512], *pData = strBody;
    uint8_t strHead[4];
    uint16_t uSslVersion;
    SecureStateT *pSecure = pState->pSecure;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send HelloRetryRequest\n"));

    // remember we're in HRR flow
    pSecure->bHelloRetry = TRUE;

    // inject synthetic message hash (we only need to do tls1.3 hashes here)
    _ProtoSSLHandshakeHashInject(pSecure, CRYPTHASH_SHA256);
    _ProtoSSLHandshakeHashInject(pSecure, CRYPTHASH_SHA384);

    // add fixed tls1.2 version to message body
    uSslVersion = SSL3_TLS1_2;
    *pData++ = (uint8_t)(uSslVersion>>8);
    *pData++ = (uint8_t)(uSslVersion>>0);

    // add special random value to indicate this is HelloRetryRequest, not a ServerHello
    ds_memcpy(pData, _SSL3_HelloRetryRequestRandom, sizeof(_SSL3_HelloRetryRequestRandom));
    pData += 32;
    // echo client's sessionid
    if (pSecure->bSessionResume)
    {
        *pData++ = sizeof(pSecure->SessionId);
        ds_memcpy(pData, pSecure->SessionId, sizeof(pSecure->SessionId));
        pData += sizeof(pSecure->SessionId);
    }
    else
    {
        *pData++ = 0;
    }

    // the cipher we send here MUST be chosen in the subsequent ServerHello
    pSecure->uRetryCipher = pSecure->pCipher->uIdent;
    *pData++ = (uint8_t)(pSecure->uRetryCipher>>8);
    *pData++ = (uint8_t)(pSecure->uRetryCipher>>0);

    // set legacy_compression_method
    *pData++ = 0;

    // add keyshare extension
    pData = _ProtoSSLAddHelloExtensions(pState, pData, (uint32_t)(sizeof(strBody)-(pData-strBody)), strBody, 0);

    // setup the header
    strHead[0] = SSL3_MSG_SERVER_HELLO;
    strHead[1] = 0;
    strHead[2] = (uint8_t)((pData-strBody)>>8);
    strHead[3] = (uint8_t)((pData-strBody)>>0);

    // setup packet for send
    _ProtoSSLSendPacket(pState, SSL3_REC_HANDSHAKE, strHead, 4, strBody, pData-strBody);

    // return next state
    return(ST3_RECV_HELLO);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvHelloRetryRequest

    \Description
        Process Hello Retry Request from server (TLS1.3+) as per
        https://tools.ietf.org/html/rfc8446#section-4.1.4

    \Input *pState      - module state reference
    \Input *pData       - pointer to message data
    \Input iDataSize    - size of packet data

    \Output
        int32_t         - next state (ST_SEND_HELLO, or ST_FAIL_* on error)

    \Notes
        The HelloRetryRequest masquerades as a ServerHello, but with a specific
        ServerRandom value.  This function expects to be given data immediately
        following the ServerRandom.

    \Version 08/08/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLRecvHelloRetryRequest(ProtoSSLRefT *pState, const uint8_t *pData, int32_t iDataSize)
{
    SecureStateT *pSecure = pState->pSecure;
    int32_t iParseResult;
    const uint8_t *pDataEnd = pData + iDataSize;
    const CipherSuiteT *pCipher;
    uint8_t aSessionId[SSL_SESSID_SIZE];
    uint32_t uSessionLen;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process HelloRetryRequest\n"));

    // validate legacy_version, which must be TLS1.2
    if (pSecure->uSslVersion != PROTOSSL_VERSION_TLS1_2)
    {
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_ILLEGAL_PARAMETER);
        return(ST_FAIL_SETUP);
    }

    // as per https://tools.ietf.org/html/rfc8446#section-4.1.4, a second HRR results in a fatal error
    if (pSecure->bHelloRetry)
    {
        NetPrintf(("protossl: received HRR after already receiving an HRR\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_UNEXPECTED_MESSAGE);
        return(ST_FAIL_SETUP);
    }

    // inject synthetic message hash (we only need to do tls1.3 hashes here)
    _ProtoSSLHandshakeHashInject(pState->pSecure, CRYPTHASH_SHA256);
    _ProtoSSLHandshakeHashInject(pState->pSecure, CRYPTHASH_SHA384);

    // read legacy session id
    if ((uSessionLen = _SafeRead8(pData, pDataEnd)) == SSL_SESSID_SIZE)
    {
        _SafeReadBytes(aSessionId, sizeof(aSessionId), pData+1, SSL_SESSID_SIZE, pDataEnd);
    }
    pData += uSessionLen+1;

    /* as per https://tools.ietf.org/html/rfc8446#section-4.1.3, a client receiving a legacy_session_id field that
       does not match what it sent in the ClientHello MUST abort the handshake with an "illegal_parameter" alert */
    if (pSecure->bSentSessionId ? memcmp(pSecure->SessionId, aSessionId, sizeof(pSecure->SessionId)) : uSessionLen != 0)
    {
        NetPrintf(("protossl: received mismatched sessionid\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_ILLEGAL_PARAMETER);
        return(ST_FAIL_SETUP);
    }

    // as per https://tools.ietf.org/html/rfc8446#section-4.1.4, a client receiving a cipher suite that was not offered MUST abort the handshake
    if ((pCipher = _ProtoSSLGetCipher(pSecure, _SafeRead16(pData, pDataEnd))) == NULL)
    {
        NetPrintf(("protossl: received cipher in HelloRetryRequest that we didn't send (ident=%d,%d)\n", pData[0], pData[1]));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_ILLEGAL_PARAMETER);
        return(ST_FAIL_SETUP);
    }
    // save cipher for later verification when we receive the ServerHello
    pSecure->uRetryCipher = pCipher->uIdent;
    pData += 2;

    // skip legacy_compression_method, make sure it's zero
    if (_SafeRead8(pData++, pDataEnd))
    {
        /* as per https://tools.ietf.org/html/rfc8446#section-4.1.2 compression must be disabled; "if a TLS 1.3 ClientHello is
           received with any other value in this field, the server MUST abort the handshake with an "illegal_parameter" alert */
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_ILLEGAL_PARAMETER);
        return(ST_FAIL_CONN);
    }

    // parse hello extensions for supported_versions first; this allows extension parsers to know if the connection will be tls1.3 or not
    if ((iParseResult = _ProtoSSLParseHelloExtensions(pState, pData, pDataEnd, SSL_EXTN_SUPPORTED_VERSIONS)) != 0)
    {
        return(iParseResult);
    }
    // parse the rest of the extensions
    if ((iParseResult = _ProtoSSLParseHelloExtensions(pState, pData, pDataEnd, SSL_EXTN_ALL)) != 0)
    {
        return(iParseResult);
    }
    // remember we're in the HelloRetryRequest flow
    pSecure->bHelloRetry = TRUE;
    /* if we hit HRR, we assume that we need to make a change to our curve. let's throw
       away the context and generated key */
    pSecure->bEccContextInitialized = FALSE;
    pSecure->bEccKeyGenerated = FALSE;

    // retry ClientHello
    return(ST3_SEND_HELLO);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLSendServerHello

    \Description
        Send ServerHello handshake packet

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST3_SEND_HELLO, ST3_SEND_EXTN, ST3_SEND_CHANGE,
                                      ST3_SEND_CERT, or ST_FAIL_* on error)

    \Version 10/15/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLSendServerHello(ProtoSSLRefT *pState)
{
    uint8_t strHead[4];
    uint8_t strBody[512];
    uint8_t *pData = strBody;
    SecureStateT *pSecure = pState->pSecure;
    uint32_t uSslVersion;
    int32_t iNextState;

    // if tls1.3, generate public key (tls1.2 and prior do this in SendServerKeyExchange)
    if (pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3)
    {
        const CryptCurveDhT *pEcc;
        uint32_t uCryptUsecs;

        // initialize elliptic curve context if not already initialized
        if ((pEcc = _ProtoSSLEccInitContext(pSecure, pSecure->pEllipticCurve)) == NULL)
        {
            /* if we cannot find the dh functions, there is some configuration mishap or the server is faulty.
               let's fail early here so we can debug the issue instead of a null pointer exception */
            return(ST_FAIL_SETUP);
        }
        // generate the public key; public key is extracted and sent to peer in the key_share extension
        if (pEcc->Public(pSecure->EccContext, NULL, &uCryptUsecs) > 0)
        {
            return(ST3_SEND_HELLO);
        }
        pSecure->bEccKeyGenerated = TRUE;
        NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (generate public key for server key message) %dms\n",
            uCryptUsecs/1000));
        pSecure->uTimer += uCryptUsecs/1000;
    }

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send ServerHello\n"));

    // tls1.3 fixes hello version number at 1.2
    uSslVersion = (pSecure->uSslVersion < PROTOSSL_VERSION_TLS1_3) ? pSecure->uSslVersion : SSL3_TLS1_2;

    // set negotiated protocol version
    *pData++ = (uint8_t)(uSslVersion>>8);
    *pData++ = (uint8_t)(uSslVersion>>0);

    // generate and set server random data
    CryptRandGet(pSecure->ServerRandom, sizeof(pSecure->ServerRandom));
    // set random downgrade bytes as appropriate, as per https://tools.ietf.org/html/rfc8446#section-4.1.3
    if ((pSecure->uSslVersion == SSL3_TLS1_2) && (pState->uSslVersion >= SSL3_TLS1_3))
    {
        ds_memcpy(&pSecure->ServerRandom[24], _SSL3_ServerRandomDowngrade12, sizeof(_SSL3_ServerRandomDowngrade12));
    }
    else if ((pSecure->uSslVersion < SSL3_TLS1_2) && (pState->uSslVersion >= SSL3_TLS1_2))
    {
        ds_memcpy(&pSecure->ServerRandom[24], _SSL3_ServerRandomDowngrade11, sizeof(_SSL3_ServerRandomDowngrade11));
    }
    ds_memcpy(pData, pSecure->ServerRandom, sizeof(pSecure->ServerRandom));
    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->ServerRandom, sizeof(pSecure->ServerRandom), "ServerRandom");
    #endif
    pData += 32;

    // add sessionid to ServerHello (TLS1.2 and prior) / echo client's legacy sessionid (TLS1.3)
    if (((pSecure->uSslVersion < SSL3_TLS1_3) && pState->bSessionResumeEnabled) || (pSecure->bSessionResume))
    {
        #if DEBUG_RAW_DATA
        NetPrintMem(pSecure->SessionId, sizeof(pSecure->SessionId), "SessionId");
        #endif
        *pData++ = sizeof(pSecure->SessionId);
        ds_memcpy(pData, pSecure->SessionId, sizeof(pSecure->SessionId));
        pData += sizeof(pSecure->SessionId);
    }
    else
    {
        *pData++ = 0;
    }

    // add the selected cipher suite
    *pData++ = (uint8_t)(pSecure->pCipher->uIdent>>8);
    *pData++ = (uint8_t)(pSecure->pCipher->uIdent>>0);

    // add null compression_method
    *pData++ = 0;

    // add extensions; note that for TLS1.3 we only add extensions required to establish secure comms; the rest are sent in EncryptedExtensions
    pData = _ProtoSSLAddHelloExtensions(pState, pData, (uint32_t)sizeof(strBody)-(uint32_t)(pData-strBody), strBody, (pSecure->uSslVersion < PROTOSSL_VERSION_TLS1_3) ? pState->uHelloExtn : 0);

    // setup the header
    strHead[0] = SSL3_MSG_SERVER_HELLO;
    strHead[1] = 0;
    strHead[2] = (uint8_t)((pData-strBody)>>8);
    strHead[3] = (uint8_t)((pData-strBody)>>0);

    // send the packet
    _ProtoSSLSendPacket(pState, SSL3_REC_HANDSHAKE, strHead, 4, strBody, (int32_t)(pData-strBody));

    // determine next state
    if (pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3)
    {
        // send encrypted extensions
        iNextState = ST3_SEND_EXTN;
    }
    else
    {
        iNextState = pSecure->bSessionResume ? ST3_SEND_CHANGE : ST3_SEND_CERT;
    }
    return(iNextState);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvServerHello

    \Description
        Process ServerHello handshake packet

    \Input *pState      - module state reference
    \Input *pData       - packet data
    \Input iDataSize    - size of packet data

    \Output
        int32_t         - next state (ST3_RECV_HELLO, ST3_RECV_CHANGE, ST3_PROC_ASYNC,
                                      or ST_FAIL_* on error)

    \Notes
        Also handles initial parsing of tls1.3 HelloRetryRequest; if an HRR is
        identified, processing is handed off to the HRR-specific handler.

    \Version 03/15/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLRecvServerHello(ProtoSSLRefT *pState, const uint8_t *pData, int32_t iDataSize)
{
    int32_t iParseResult, iState = ST3_RECV_HELLO;
    const uint8_t *pDataEnd, *pSessionId=NULL, *pCipher;
    SecureStateT *pSecure = pState->pSecure;
    const uint8_t *pDataStart = pData;
    uint8_t aSessionId[SSL_SESSID_SIZE], uSessionLen;

    /* get the location of server hello end; some servers will not send any extension length
       so we need to make sure we don't parse past the end of the packet server hello */
    pDataEnd = pData + iDataSize;

    // get server-specified version of the protocol
    pSecure->uSslVersion = _SafeRead16(pData, pDataEnd);
    pData += 2;

    // save server random data
    _SafeReadBytes(pSecure->ServerRandom, sizeof(pSecure->ServerRandom), pData, sizeof(pSecure->ServerRandom), pDataEnd);
    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->ServerRandom, sizeof(pSecure->ServerRandom), "ServerRandom");
    #endif
    pData += 32;

    /* as per https://tools.ietf.org/html/rfc8446#section-4.1.3, check for special random value,
       indicating this is a HelloRetryRequest and not a ServerHello */
    if (!memcmp(pSecure->ServerRandom, _SSL3_HelloRetryRequestRandom, sizeof(_SSL3_HelloRetryRequestRandom)))
    {
        return(_ProtoSSLRecvHelloRetryRequest(pState, pData, iDataSize - (pData-pDataStart)));
    }
    
    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process Server Hello\n"));

    // read sessionid length
    uSessionLen = _SafeRead8(pData++, pDataEnd);
    // save pointer to and skip sessionid
    pSessionId = pData;
    pData += uSessionLen;

    // save cipher offset
    pCipher = pData;
    pData += 2;

    // validate and skip compression
    if (_SafeRead8(pData++, pDataEnd) != 0)
    {
        /* as per https://tools.ietf.org/html/rfc8446#section-4.1.2 compression must be disabled; "if a TLS 1.3 ClientHello is received
           with any other value in this field, the server MUST abort the handshake with an "illegal_parameter" alert.  unlike the server
           flow, the client flow enforces this restriction unconditionally, as our client does not support compression */
        NetPrintf(("protossl: compression_methods not zero\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_ILLEGAL_PARAMETER);
        return(ST_FAIL_CONN);
    }

    // parse hello extensions for supported_versions first; this allows extension parsers to know if the connection will be tls1.3 or not
    if ((iParseResult = _ProtoSSLParseHelloExtensions(pState, pData, pDataEnd, SSL_EXTN_SUPPORTED_VERSIONS)) != 0)
    {
        return(iParseResult);
    }

    // make sure we support version; this must be done after possible supported_versions extension parsing
    if (pSecure->uSslVersion != pState->uSslVersion)
    {
        if ((pSecure->uSslVersion < pState->uSslVersionMin) || (pSecure->uSslVersion > pState->uSslVersion))
        {
            NetPrintf(("protossl: server specified SSL version 0x%04x is not supported\n", pSecure->uSslVersion));
            /* As per http://tools.ietf.org/html/rfc5246#appendix-E.1 - If the version chosen by
                the server is not supported by the client (or not acceptable), the client MUST send a
                "protocol_version" alert message and close the connection. */
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_PROTOCOL_VERSION);
            return((pSecure->uSslVersion < pState->uSslVersionMin) ? ST_FAIL_CONN_MINVERS : ST_FAIL_CONN_MAXVERS);
        }
        else
        {
            NetPrintfVerbose((pState->iVerbose, 1, "protossl: downgrading SSL version\n"));
        }
    }
    NetPrintfVerbose((pState->iVerbose, 0, "protossl: using %s to connect to %s\n", _SSL3_strVersionNames[pSecure->uSslVersion&0xff], pState->strHost));

    // parse all other extensions
    if ((iParseResult = _ProtoSSLParseHelloExtensions(pState, pData, pDataEnd, SSL_EXTN_ALL)) != 0)
    {
        return(iParseResult);
    }

    // protect against possible downgrade attack as per https://tools.ietf.org/html/rfc8446#section-4.1.3 
    if ((pSecure->uSslVersion == SSL3_TLS1_2) && (pState->uSslVersion >= SSL3_TLS1_3) && !memcmp(&pSecure->ServerRandom[24], _SSL3_ServerRandomDowngrade12, sizeof(_SSL3_ServerRandomDowngrade12)))
    {
        NetPrintf(("protossl: invalid tls1.3 downgrade detected\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_ILLEGAL_PARAMETER);
        return(ST_FAIL_SETUP);
    }
    else if ((pSecure->uSslVersion < SSL3_TLS1_2) && (pState->uSslVersion >= SSL3_TLS1_2) && !memcmp(&pSecure->ServerRandom[24], _SSL3_ServerRandomDowngrade11, sizeof(_SSL3_ServerRandomDowngrade11)))
    {
        NetPrintf(("protossl: invalid tls1.2 downgrade detected\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_ILLEGAL_PARAMETER);
        return(ST_FAIL_SETUP);
    }

    // get and validate cipher
    if ((pSecure->pCipher = _ProtoSSLGetCipher(pSecure, _SafeRead16(pCipher, pDataEnd))) == NULL)
    {
        NetPrintf(("protossl: no matching cipher (ident=%d,%d)\n", pCipher[0], pCipher[1]));
        /* as per https://tools.ietf.org/html/rfc8446#section-4.1.3: a client receiving a cipher suite that was not offered
           MUST abort the handshake with an "illegal_parameter" alert */
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_ILLEGAL_PARAMETER);
        return(ST_FAIL_CONN_NOCIPHER);
    }
    else
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: using cipher suite %s (ident=%d,%d)\n", pSecure->pCipher->strName, pCipher[0], pCipher[1]));
    }

    // if in retry flow, make sure we got the expected cipher as per https://tools.ietf.org/html/rfc8446#section-4.1.4
    if (pSecure->bHelloRetry && (pSecure->pCipher->uIdent != pSecure->uRetryCipher))
    {
        NetPrintf(("protossl: cipher specified in ServerHello does not match the cipher from HelloRetryRequest\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_ILLEGAL_PARAMETER);
        return(ST_FAIL_SETUP);
    }

    // read session info
    if (uSessionLen == SSL_SESSID_SIZE)
    {
        _SafeReadBytes(aSessionId, sizeof(aSessionId), pSessionId, SSL_SESSID_SIZE, pDataEnd);
        #if DEBUG_RAW_DATA
        NetPrintMem(aSessionId, sizeof(aSessionId), "ServerHello session id");
        #endif
    }

    // process session id (tls1.2 and previous only)
    if (pSecure->uSslVersion < PROTOSSL_VERSION_TLS1_3)
    {
        // save server session id
        if (uSessionLen == sizeof(pSecure->SessionId))
        {
            // check for ClientHello/ServerHello session match; a match indicates we are resuming the session and bypassing key exchange
            if (!memcmp(pSecure->SessionId, aSessionId, sizeof(pSecure->SessionId)))
            {
                // set resume and update state
                NetPrintfVerbose((pState->iVerbose, 0, "protossl: resuming previous session\n"));
                pSecure->bSessionResume = TRUE;
                iState = ST3_RECV_CHANGE;
            }
            else
            {
                // save session id for possible future resumption
                ds_memcpy(pSecure->SessionId, aSessionId, sizeof(pSecure->SessionId));
            }
        }

        // clear premaster secret if not resuming
        if (!pSecure->bSessionResume)
        {
            ds_memclr(pSecure->MasterKey, sizeof(pSecure->MasterKey));
        }
    }
    else if (pSecure->bSentSessionId ? memcmp(pSecure->SessionId, aSessionId, sizeof(pSecure->SessionId)) : uSessionLen != 0)
    {
        /* as per https://tools.ietf.org/html/rfc8446#section-4.1.3, a client receiving a legacy_session_id field that
           does not match what it sent in the ClientHello MUST abort the handshake with an "illegal_parameter" alert */
        NetPrintf(("protossl: received mismatched sessionid\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_ILLEGAL_PARAMETER);
        return(ST_FAIL_SETUP);
    }

    // tls1.3+ data is immediately encrypted following ServerHello and keyshare/psk
    if (pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3)
    {
        // make sure the server gave us a keyshare we support
        if (pSecure->pEllipticCurve == NULL)
        {
            NetPrintf(("protossl: no supported elliptic curve given in tls1.3 server hello\n"));
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_ILLEGAL_PARAMETER);
            return(ST_FAIL_SETUP);
        }
        // update the handshake hash with ServerHello, which hasn't been processed yet
        //$$todo - is this actually required?
        _ProtoSSLRecvHandshakeFinish(pState);
        // set up async execution to build secrets and key material
        iState = _ProtoSSLUpdateSetAsyncState(pState, _ProtoSSLBuildHandshakeKey, iState, ST_FAIL_SETUP, SSL3_ALERT_DESC_INTERNAL_ERROR);
    }

    // return new state
    return(iState);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLSendEncryptedExtensions

    \Description
        Send EncryptedExtensions handshake message (TLS1.3+ only)

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST3_SEND_EXTN, ST3_SEND_CERT_REQ or ST3_SEND_CERT)

    \Version 01/26/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLSendEncryptedExtensions(ProtoSSLRefT *pState)
{
    uint8_t strBody[512], strHead[4], *pData = strBody+2;
    uint32_t uExtnLen;

    // data after ServerHello is encrypted
    if (_ProtoSSLBuildHandshakeKey(pState) > 0)
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protossl: generating handshake key\n"));
        return(ST3_SEND_EXTN);
    }

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send EncryptedExtensions\n"));

    // add extensions to message body
    pData = _ProtoSSLAddHelloExtensions(pState, pData, (uint32_t)sizeof(strBody)-2, strBody, pState->uHelloExtn);
    uExtnLen = pData - strBody - 2;
    // set extn length
    strBody[0] = (uint8_t)(uExtnLen>>8);
    strBody[1] = (uint8_t)(uExtnLen>>0);

    // setup the header
    strHead[0] = SSL3_MSG_ENCRYPTED_EXTENSIONS;
    strHead[1] = 0;
    strHead[2] = (uint8_t)((uExtnLen+2)>>8);
    strHead[3] = (uint8_t)((uExtnLen+2)>>0);

    // setup packet for send
    _ProtoSSLSendPacket(pState, SSL3_REC_HANDSHAKE, strHead, 4, strBody, uExtnLen+2);

    // return next state
    return((pState->iClientCertLevel > 0) ? ST3_SEND_CERT_REQ : ST3_SEND_CERT);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvEncryptedExtensions

    \Description
        Process EncryptedExtensions handshake packet (TLS1.3+ only)

    \Input *pState      - module state reference
    \Input *pData       - packet data
    \Input iDataSize    - size of packet data

    \Output
        int32_t         - next state (ST3_RECV_FINISH, ST3_RECV_HELLO, or ST_FAIL_* on error)

    \Version 01/26/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLRecvEncryptedExtensions(ProtoSSLRefT *pState, const uint8_t *pData, int32_t iDataSize)
{
    SecureStateT *pSecure = pState->pSecure;
    int32_t iParseResult, iNextState = (pSecure->bSessionResume) ? ST3_RECV_FINISH : ST3_RECV_HELLO;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process EncryptedExtensions\n"));

    // parse the extensions
    iParseResult = _ProtoSSLParseHelloExtensions(pState, pData, pData+iDataSize, SSL_EXTN_ALL);

    // move to next state
    return((iParseResult == 0) ? iNextState : iParseResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateSendCertificate

    \Description
        Send Certificate handshake packet

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST3_SEND_KEY, ST3_SEND_VERIFY, ST3_SEND_CERT_REQ,
                                      or ST3_SEND_DONE)

    \Version 10/15/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLSendCertificate(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;
    uint8_t strHead[4];
    uint8_t strBody[4096];
    int32_t iCertSize, iNumCerts, iBodySize=0, iExtnSize=0;
    int32_t iState = (pSecure->uSslVersion < PROTOSSL_VERSION_TLS1_3) ? ST3_SEND_KEY : ST3_SEND_VERIFY;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send Certificate\n"));

    // set certificate_request_context
    if (pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3)
    {
        strBody[iBodySize++] = 0;
        iExtnSize = 2;
    }

    /* if we have a certificate, add it to body $$todo if client we need to validate certificate matches sigalg/cipher, and send an
       empty message if it does not.  if server we should have already validated that or aborted the connection if it does not */
    if (pState->pCertificate != NULL)
    {
        iCertSize = pState->pCertificate->iCertSize;
        ds_memcpy_s(strBody+iBodySize+6, sizeof(strBody)-iBodySize-6, pState->pCertificate->aCertData, pState->pCertificate->iCertSize);
    }
    else if (pState->bServer)
    {
        /* as per https://tools.ietf.org/html/rfc8446#section-4.4.2 a server's certificate list MUST not be empty; this is
           a general practice in earlier versions but not specifically enforced prior */ 
        NetPrintf(("protossl: no valid certificate\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
        return(ST_FAIL_CERT_INVALID);
    }
    else
    {
        /* as per https://tools.ietf.org/html/rfc5246#section-7.4.6, a client MUST send an empty certificate_list if it does not have an
           appropriate certificate to send in response to the server's authentication request. */
        iCertSize = 0;
        // if we're a tls1.3 client skip sending CertificateVerify and go directly to Finished
        if (pSecure->uSslVersion >= SSL3_TLS1_3)
        {
            iState = ST3_SEND_FINISH;
        }
    }
    iNumCerts = (iCertSize) ? 1 : 0;

    // copy the certificate list into message body
    strBody[iBodySize++] = 0;
    strBody[iBodySize++] = (uint8_t)(((iCertSize+iExtnSize)+(iNumCerts*3))>>8);
    strBody[iBodySize++] = (uint8_t)(((iCertSize+iExtnSize)+(iNumCerts*3))>>0);
    if (iNumCerts)
    {
        strBody[iBodySize++] = 0;
        strBody[iBodySize++] = (uint8_t)((iCertSize)>>8);
        strBody[iBodySize++] = (uint8_t)((iCertSize)>>0);
    }
    iBodySize += iCertSize;

    /* set Certificate extensions - note that this field is expected after every certificate, so this
       code only works when a single certificate is being sent */
    if (pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3)
    {
        strBody[iBodySize++] = 0;
        strBody[iBodySize++] = 0;
    }

    // setup the header
    strHead[0] = SSL3_MSG_CERTIFICATE;
    strHead[1] = 0;
    strHead[2] = (uint8_t)((iBodySize)>>8);
    strHead[3] = (uint8_t)((iBodySize)>>0);

    // setup packet for send
    _ProtoSSLSendPacket(pState, SSL3_REC_HANDSHAKE, strHead, 4, strBody, iBodySize);

    /* remember we sent a certificate (since we will send an empty message if no certificate
       is available, only remember if we *actually* sent a certificate).  this is used later
       to determine if we should send a CertificateVerify handshake message */
    pSecure->bSentCert = iNumCerts ? TRUE : FALSE;

    // if we're a server executing a TLS1.2 or prior handshake, override state transition set above
    if ((pSecure->uSslVersion <= SSL3_TLS1_2) && (pState->bServer) && (pSecure->pCipher->uKey == SSL3_KEY_RSA))
    {
        iState = (pState->iClientCertLevel > 0) ? ST3_SEND_CERT_REQ : ST3_SEND_DONE;
    }
    return(iState);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvCertificate

    \Description
        Process Certificate handshake packet; ref http://tools.ietf.org/html/rfc5246#section-7.4.2

    \Input *pState      - module state reference
    \Input *pData       - packet data
    \Input iDataSize    - size of packet data

    \Output
        int32_t         - next state (ST3_RECV_HELLO, ST_WAIT_CA, or ST_FAIL_* on error)

    \Version 08/26/2011 (szhu) Added multi-certificate support
*/
/********************************************************************************F*/
static int32_t _ProtoSSLRecvCertificate(ProtoSSLRefT *pState, const uint8_t *pData, int32_t iDataSize)
{
    SecureStateT *pSecure = pState->pSecure;
    int32_t iCertSize, iCertHeight, iSize1, iSize2, iResult, iVerify;
    uint8_t aFingerprintId[SSL_FINGERPRINT_SIZE], bCertCached = FALSE;
    X509CertificateT leafCert, prevCert;
    #if DIRTYCODE_LOGGING
    char strIdentSubject[512], strIdentIssuer[512];
    #endif
    const uint8_t *pDataEnd = pData+iDataSize;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process Certificate\n"));

    ds_memclr(&leafCert, sizeof(leafCert));
    ds_memclr(&prevCert, sizeof(prevCert));

    // skip certificate_request_context
    if (pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3)
    {
        uint32_t uContextLen = _SafeRead8(pData, pDataEnd);
        NetPrintfVerbose((DEBUG_RAW_DATA, 0, "protossl: certificate_request_context length=%d\n", uContextLen));
        pData += uContextLen + 1;
        iDataSize -= uContextLen + 1;
    }

    // get outer size
    if ((iCertSize = iSize1 = _SafeRead24(pData, pDataEnd)) <= 3)
    {
        NetPrintf(("protossl: no certificate included in certificate message\n"));
        if (!pState->bServer)
        {
            /* as per https://tools.ietf.org/html/rfc8446#section-4.4.2.4, If the server supplies an empty Certificate message, the client MUST
               abort the handshake with a "decode_error" alert. */
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_DECODE_ERROR);
            iResult = ST_FAIL_CERT_NONE;
        }
        else if (pState->bServer && (pState->iClientCertLevel == 2))
        {
            /* as per http://tools.ietf.org/html/rfc5246#section-7.4.6 (TLS 1.2), a client providing an empty certificate response to a certificate
               request can either be ignored and treated as unauthenticated, or responded to with a fatal handshake failure.  a server
               MUST always provide a certificate if the agreed-upon key exchange method uses certificates for authentication
               as per https://tools.ietf.org/html/rfc8446#section-4.4.2.4 (TLS 1.3), If the client does not send any certificates (i.e., it sends an empty Certificate message),
               the server MAY at its discretion either continue the handshake without client authentication or abort the handshake with a "certificate_required" alert.
               Also, if some aspect of the certificate chain was unacceptable (e.g., it was not signed by a known, trusted CA), the server MAY at its discretion either
               continue the handshake (considering the client unauthenticated) or abort the handshake. */
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, pSecure->uSslVersion < SSL3_TLS1_3 ? SSL3_ALERT_DESC_HANDSHAKE_FAILURE : SSL3_ALERT_DESC_CERTIFICATE_REQUIRED);
            iResult = ST_FAIL_CERT_NONE;
        }
        else
        {
            iResult = (pSecure->uSslVersion < SSL3_TLS1_3) ? ST3_RECV_HELLO : ST3_RECV_FINISH;
        }
        return(iResult);
    }
    else if (iCertSize != (iDataSize-3))
    {
        NetPrintf(("protossl: invalid certificate length\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_BAD_CERTFICIATE);
        return(ST_FAIL_CERT_INVALID);
    }
    else
    {
        // remember we received a certificate in the certificate message
        pSecure->bRecvCert = TRUE;

        // generate a fingerprint of entire certificate envelope
        _ProtoSSLGenerateFingerprint(aFingerprintId, sizeof(aFingerprintId), pData, iCertSize, CRYPTHASH_SHA1);

        // see if we've already validated this chain
        if (_CertValidHistoryCheck(aFingerprintId, iCertSize))
        {
            NetPrintfVerbose((DEBUG_VAL_CERT, 0, "protossl: matched certificate fingerprint in certificate validity history\n"));
            bCertCached = TRUE;
        }
    }

    // skip outer size
    pData += 3;

    // process certificates
    for (iVerify = -1, iCertHeight = 0; (iSize1 > 0) && (iVerify != 0); pData += iSize2, iSize1 -= iSize2)
    {
        uint64_t uTime;
        int32_t iParse;

        // get certificate size
        iSize2 = _SafeRead24(pData, pDataEnd);

        // make sure certificate size isn't too large
        if (iSize2 > iSize1)
        {
            NetPrintf(("protossl: _ServerCert: certificate is larger than envelope (%d/%d)\n", iSize1, iSize2));
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_BAD_CERTFICIATE);
            return(ST_FAIL_CERT_INVALID);
        }

        // skip certificate object length
        pData += 3;
        iSize1 -= 3;

        // parse the certificate
        if ((iParse = _AsnParseCertificate(&pSecure->Cert, pData, iSize2)) < 0)
        {
            NetPrintf(("protossl: _ServerCert: x509 cert invalid (error=%d)\n", iParse));
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_BAD_CERTFICIATE);
            return(ST_FAIL_CERT_INVALID);
        }
        #if DEBUG_VAL_CERT
        _CertificateDebugPrint(&pSecure->Cert, "parsed certificate");
        #endif  

        // skip TLS1.3 certificate extension field
        if (pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3)
        {
            const uint8_t *pExtn = pData + iSize2;
            uint16_t uExtnLen = _SafeRead16(pExtn, pDataEnd);
            iSize2 += uExtnLen + 2;
        }

        // allow verification bypass; note this must come after first certificate is parsed since we need it later for key exchange
        if (pState->bAllowAnyCert)
        {
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: bypassing certificate verify (disabled)\n"));
            return(ST3_RECV_HELLO);
        }

        // validate date range
        if ((uTime = ds_timeinsecs()) != 0)
        {
            if ((uTime < pSecure->Cert.uGoodFrom) || (uTime > pSecure->Cert.uGoodTill))
            {
                #if DIRTYCODE_LOGGING
                struct tm TmTime;
                char strTime[32];
                _CertificateDebugPrint(&pSecure->Cert, "certificate failed date range validation:");
                NetPrintf(("protossl:    from: %s\n", ds_timetostr(ds_secstotime(&TmTime, pSecure->Cert.uGoodFrom), TIMETOSTRING_CONVERSION_ISO_8601, FALSE, strTime, sizeof(strTime))));
                NetPrintf(("protossl:    till: %s\n", ds_timetostr(ds_secstotime(&TmTime, pSecure->Cert.uGoodTill), TIMETOSTRING_CONVERSION_ISO_8601, FALSE, strTime, sizeof(strTime))));
                #endif
                iVerify = SSL_ERR_CERT_INVALIDDATE;
                pSecure->bDateVerifyFailed = TRUE;
            }
        }

        // processing specific to first (leaf) certificate
        if (iCertHeight == 0)
        {
            // ensure certificate was issued to this host (server certificates only)
            if (!pState->bServer && (_CertificateMatchHostname(pState->strHost, pSecure->Cert.Subject.strCommon) != 0))
            {
                // check against alternative name(s), if present
                if (_CertificateMatchSubjectAlternativeName(pState->strHost, pSecure->Cert.pSubjectAlt, pSecure->Cert.iSubjectAltLen) != 0)
                {
                    NetPrintf(("protossl: _ServerCert: certificate not issued to this host (%s != %s)\n", pState->strHost, pSecure->Cert.Subject.strCommon));
                    _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_CERTIFICATE_UNKNOWN);
                    _CertificateDebugPrint(&pSecure->Cert, "cert");
                    _CertificateSetFailureInfo(pState, &pSecure->Cert, FALSE);
                    return(ST_FAIL_CERT_HOST);
                }
            }
            // save leaf cert as we will need it later for handshake
            ds_memcpy(&leafCert, &pSecure->Cert, sizeof(leafCert));
            // debug display of leaf certificate signature algorithm and signature length
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: sigalg=%s, siglen=%d\n", _SSL3_strSignatureTypes[pSecure->Cert.iSigType-ASN_OBJ_RSA_PKCS_MD5], pSecure->Cert.iSigSize*8));

            // if we cached this cert chain allow bypass; this must come after we've verified the hostname matches
            if (bCertCached)
            {
                NetPrintfVerbose((pState->iVerbose, 0, "protossl: bypassing certificate verify (cached)\n"));
                iVerify = 0;
                break;
            }
        }
        else
        {
            // check to see if the cert is the next cert in the trust chain anchored by the leaf certificate
            if ((_CertificateCompareIdent(&pSecure->Cert.Subject, &prevCert.Issuer, prevCert.iCertIsCA) != 0) ||
                ((pSecure->Cert.iKeyModSize != prevCert.iSigSize) && (pSecure->Cert.iKeyType != ASN_OBJ_ECDSA_KEY)))
            {
                // not part of our chain, so skip it
                _CertificateDebugPrint(&pSecure->Cert, "_ServerCert: certificate not part of chain, skipping:");
                continue;
            }

            // check basic constraints (CA)
            if (pSecure->Cert.iCertIsCA == FALSE)
            {
                continue;
            }
                
            // check basic constraints (pathLenConstraints)
            if ((pSecure->Cert.iMaxHeight != 0) && (iCertHeight > pSecure->Cert.iMaxHeight))
            {
                continue;
            }

            // verify previous cert's signature with current cert's public key
            if ((_ProtoSSLVerifyCertificateSignature(pSecure, &prevCert, pSecure->Cert.KeyModData, pSecure->Cert.iKeyModSize,
                pSecure->Cert.KeyExpData, pSecure->Cert.iKeyExpSize, pSecure->Cert.iKeyType, pSecure->Cert.iCrvType)) != 0)
            {
                continue;
            }

            NetPrintfVerbose((pState->iVerbose, 0, "protossl: cert (%s) verified by ca (%s)\n", _CertificateDebugFormatIdent(&prevCert.Subject, strIdentSubject, sizeof(strIdentSubject)),
                _CertificateDebugFormatIdent(&prevCert.Issuer, strIdentIssuer, sizeof(strIdentIssuer))));
        }

        // track cert height
        iCertHeight += 1;
        // force _VerifyCertificate to update pState->CertInfo on cert failure
        pState->bCertInfoSet = FALSE;
        // see if we can verify against a trust anchor; a success here means we are done parsing the certificate chain, whether there are more certificates or not
        iVerify = _ProtoSSLVerifyCertificate(pState, pSecure, &pSecure->Cert, FALSE);

        // save current cert for validation of next cert
        ds_memcpy(&prevCert, &pSecure->Cert, sizeof(prevCert));
    }

    // process verification result
    if (iVerify != 0)
    {
        #if DIRTYCODE_LOGGING
        if (iVerify == SSL_ERR_GOSCA_INVALIDUSE)
        {
            NetPrintf(("protossl: gos ca (%s) may not be used for non-ea domain %s\n", _CertificateDebugFormatIdent(&pSecure->Cert.Issuer, strIdentSubject, sizeof(strIdentSubject)),
                pSecure->Cert.Subject.strCommon));
        }
        else if (iVerify == SSL_ERR_CAPROVIDER_INVALID)
        {
            NetPrintf(("protossl: ca (%s) may not be used as a ca provider\n", _CertificateDebugFormatIdent(&pSecure->Cert.Issuer, strIdentSubject, sizeof(strIdentSubject))));
        }
        else
        {
            _CertificateDebugPrint(&pSecure->Cert, "_VerifyCertificate() failed -- no CA available for this certificate");
            NetPrintf(("protossl: _ServerCert: x509 cert untrusted (error=%d)\n", iVerify));
        }
        #endif

        // check if we need to fetch CA
        if ((iVerify != SSL_ERR_GOSCA_INVALIDUSE) && (_ProtoSSLInitiateCARequest(pState) == 0))
        {
            iResult = ST_WAIT_CA;
        }
        else
        {
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_UNKNOWN_CA);
            iResult = ST_FAIL_CERT_NOTRUST;
        }
    }
    else if (pSecure->bDateVerifyFailed)
    {
        // an invalid date is not flagged as a verification failure, so we check the flag directly here
        NetPrintf(("protossl: _ServerCert: failed date validation\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_CERTIFICATE_EXPIRED);
        iResult = ST_FAIL_CERT_BADDATE;
    }
    else
    {
        // cert chain is valid
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: cert (%s) verified by ca (%s)\n", _CertificateDebugFormatIdent(&pSecure->Cert.Subject, strIdentSubject, sizeof(strIdentSubject)),
            _CertificateDebugFormatIdent(&pSecure->Cert.Issuer, strIdentIssuer, sizeof(strIdentIssuer))));

        // process next handshake packet
        iResult = ST3_RECV_HELLO;
        // add cert fingerprint to cert validation history if it's not already there
        if (!bCertCached)
        {
            _CertValidHistoryAdd(aFingerprintId, iCertSize);
        }
    }
    // restore the leaf cert for use during handshake
    if ((iCertHeight != 0) && (iResult != ST_FAIL_CERT_NOTRUST))
    {
        ds_memcpy(&pSecure->Cert, &leafCert, sizeof(pSecure->Cert));
    }

    // still in hello state (if validated)
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLSendServerKeyExchange

    \Description
        Send the ServerKeyExchange message (only for ECDHE key exchange)

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST3_SEND_KEY, ST3_SEND_CERT_REQ, ST3_SEND_DONE
                                      or ST_FAIL_SETUP on error)

    \Version 03/03/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLSendServerKeyExchange(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;
    uint8_t aBody[1024], aHead[4];
    uint8_t *pData = aBody;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send ServerKeyExchange\n"));

    // if using RSA for key exchange we do not send this message, treat as error
    if (pSecure->pCipher->uKey == SSL3_KEY_RSA)
    {
        NetPrintf(("protossl: _SendServerKey: wrong key exchange algorithm RSA"));
        return(ST_FAIL_SETUP);
    }
    if (pSecure->pSigScheme == NULL)
    {
        if (pSecure->uSslVersion < SSL3_TLS1_2)
        {
            pSecure->pSigScheme = _ProtoSSLGetSignatureScheme((pSecure->pCipher->uSig == SSL3_SIGALG_RSA) ? SSL3_SIGSCHEME_RSA_PKCS1_SHA1 : SSL3_SIGSCHEME_ECDSA_SHA1);
        }
        else
        {
            /* if we failed to find a matching signature algorithm and the key exchange uses ecdhe we fail
               as we won't be able to match a signature algorithm to sign with */
            NetPrintf(("protossl: _SendServerKey: no valid signature algorithm needed for key exchange\n"));
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
            return(ST_FAIL_SETUP);
        }
    }
    // make sure we found a supported elliptic curve if we are using ECC cipher suite
    if (pSecure->pEllipticCurve == NULL)
    {
        NetPrintf(("protossl: _SendServerKey: no matching elliptic curve when using ecc cipher suite\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
        /* when using ECC cipher suite make sure that we have a suitable elliptic curve.
           we are allowed to try a different cipher suite but I do not believe it is worth
           the hassle of reparsing.
           note: ECDHE-ECDSA is allowed to use a different curve for the ephemeral ECDH key
           in the ServerKeyExchange message */
        return(ST_FAIL_SETUP);
    }

    /* generate public key
       elliptic curve generation takes multiple frames
       so stay in the same state until the operation is complete */
    if (!pSecure->bEccKeyGenerated)
    {
        const CryptCurveDhT *pEcc;
        uint32_t uCryptUsecs;

        // initialize elliptic curve context if not already initialized
        if ((pEcc = _ProtoSSLEccInitContext(pSecure, pSecure->pEllipticCurve)) == NULL)
        {
            /* if we cannot find the dh functions, there is some configuration mishap or the server is faulty.
               let's fail early here so we can debug the issue instead of a null pointer exception */
            return(ST_FAIL_SETUP);
        }
        // generate the public key
        if (pEcc->Public(pSecure->EccContext, NULL, &uCryptUsecs) > 0)
        {
            return(ST3_SEND_KEY);
        }
        pSecure->uPubKeyLength = pEcc->PointFinal(pSecure->EccContext, NULL, FALSE, pSecure->PubKey, sizeof(pSecure->PubKey));
        pSecure->bEccKeyGenerated = TRUE;
        NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (generate public key for server key message) %dms\n",
            uCryptUsecs/1000));
        pSecure->uTimer += uCryptUsecs/1000;
    }

    // setup the data that needed to be hashed
    *pData++ = SSL3_CURVETYPE_NAMED_CURVE;
    *pData++ = (uint8_t)(pSecure->pEllipticCurve->uIdent>>8);
    *pData++ = (uint8_t)(pSecure->pEllipticCurve->uIdent>>0);
    *pData++ = pSecure->uPubKeyLength;
    ds_memcpy_s(pData, pSecure->uPubKeyLength, pSecure->PubKey, sizeof(pSecure->PubKey));
    pData += pSecure->uPubKeyLength;

    // setup the encryption for the server key exchange signature
    if (!pSecure->bSigGenerated)
    {
        CryptHashTypeE eHashType = _SSL3_HashIdToCrypt[pSecure->pSigScheme->SigAlg.uHashId];
        const CryptHashT *pHash;

        // calculate signature
        if ((pHash = CryptHashGet(eHashType)) != NULL)
        {
            uint8_t aSigObj[SSL_SIG_MAX], aHashDigest[CRYPTHASH_MAXDIGEST];
            int32_t iPrivateKeySize, iHashSize, iSigSize;
            X509PrivateKeyT PrivateKey;

            // decode private key, extract key modulus and private exponent
            if ((iPrivateKeySize = _CertificateDecodePrivate(pState->pPrivateKey, pState->iPrivateKeyLen, &PrivateKey)) < 0)
            {
                NetPrintf(("protossl: unable to decode private key\n"));
                _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_INTERNAL_ERROR);
                return(ST_FAIL_SETUP);
            }

            // generate signature hash and encode (where appropriate) to create signature hash object
            if ((pSecure->uSslVersion < SSL3_TLS1_2) && (pSecure->pCipher->uSig != SSL3_SIGALG_ECDSA))
            {
                iSigSize = _ProtoSSLGenerateSignatureHash(pSecure, CryptHashGet(CRYPTHASH_MD5), aSigObj, aBody, (uint32_t)(pData-aBody));
                iSigSize += _ProtoSSLGenerateSignatureHash(pSecure, CryptHashGet(CRYPTHASH_SHA1), aSigObj+iSigSize, aBody, (uint32_t)(pData-aBody));
            }
            else
            {
                iHashSize = _ProtoSSLGenerateSignatureHash(pSecure, pHash, aHashDigest, aBody, (uint32_t)(pData-aBody));
                if (pSecure->pSigScheme->uVerifyScheme == SSL3_SIGVERIFY_RSA_PKCS1)
                {
                    iSigSize = _AsnWriteDigitalHashObject(aSigObj, sizeof(aSigObj), aHashDigest, iHashSize, eHashType);
                }
                else if (pSecure->pSigScheme->uVerifyScheme == SSL3_SIGVERIFY_RSA_PSS)
                {
                    iSigSize = _Pkcs1EncodeEMSA_PSS(aSigObj, PrivateKey.Modulus.iObjSize, aHashDigest, iHashSize, eHashType);
                }
                else // ecdsa signatures have no additional encoding
                {
                    ds_memcpy_s(aSigObj, sizeof(aSigObj), aHashDigest, iHashSize);
                    iSigSize = iHashSize;
                }
            }

            // start async signature generation
            return(_ProtoSSLGenerateSignature(pState, pSecure, pSecure->pSigScheme, aSigObj, iSigSize, &PrivateKey, ST3_SEND_KEY));
        }
    }

    // signature is ready, format the message
    if (pSecure->uSslVersion >= SSL3_TLS1_2)
    {
        *pData++ = (uint8_t)(pSecure->pSigScheme->uIdent>>8);
        *pData++ = (uint8_t)(pSecure->pSigScheme->uIdent>>0);
    }
    *pData++ = (uint8_t)(pSecure->SigVerify.iSigSize>>8);
    *pData++ = (uint8_t)(pSecure->SigVerify.iSigSize>>0);
    ds_memcpy(pData, pSecure->SigVerify.aSigData, pSecure->SigVerify.iSigSize);
    pData += pSecure->SigVerify.iSigSize;

    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->SigVerify.aSigData, pSecure->SigVerify.iSigSize, "encrypted server key exchange signature");
    #endif

    // format the packet header
    aHead[0] = SSL3_MSG_SERVER_KEY;
    aHead[1] = 0;
    aHead[2] = (uint8_t)((pData-aBody)>>8);
    aHead[3] = (uint8_t)((pData-aBody)>>0);

    // setup packet for send
    _ProtoSSLSendPacket(pState, SSL3_REC_HANDSHAKE, aHead, sizeof(aHead), aBody, (int32_t)(pData-aBody));
    return((pState->iClientCertLevel > 0) ? ST3_SEND_CERT_REQ : ST3_SEND_DONE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvServerKeyExchange

    \Description
        Process ServerKeyExchange handshake packet; not used with RSA cipher suites

    \Input *pState      - module state reference
    \Input *pData       - packet data
    \Input iDataSize    - size of packet data

    \Output
        int32_t         - next state (ST3_RECV_HELLO, ST_PROC_ASYNC, or ST_FAIL_SETUP on error)

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLRecvServerKeyExchange(ProtoSSLRefT *pState, const uint8_t *pData, int32_t iDataSize)
{
    int32_t iIndex;
    SecureStateT *pSecure = pState->pSecure;
    CryptHashTypeE eHashType;
    const CryptHashT *pHash;
    const uint8_t *pBegin = pData, *pDataEnd = pData+iDataSize, *pSigData;
    const SignatureSchemeT *pSigScheme;
    int32_t iNextState = ST3_RECV_HELLO;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process ServerKeyExchange\n"));

    // if using RSA for key exchange we do not receive this message, treat as error
    if (pSecure->pCipher->uKey == SSL3_KEY_RSA)
    {
        NetPrintf(("protossl: _ServerKey: wrong key exchange algorithm RSA\n"));
        return(ST_FAIL_SETUP);
    }
    // make sure it is using the curve type we support
    if ((_SafeRead8(pData, pDataEnd) == SSL3_CURVETYPE_NAMED_CURVE) && (pData+3 <= pDataEnd))
    {
        // validate the elliptic curve sent by the server
        for (iIndex = 0; iIndex < (signed)(sizeof(_SSL3_EllipticCurves)/sizeof(*_SSL3_EllipticCurves)); iIndex += 1)
        {
            // skip non-matching elliptic curve
            if ((pData[1] != (uint8_t)(_SSL3_EllipticCurves[iIndex].uIdent >> 8)) || (pData[2] != (uint8_t)(_SSL3_EllipticCurves[iIndex].uIdent)))
            {
                continue;
            }
            // found an elliptic curve
            pSecure->pEllipticCurve = &_SSL3_EllipticCurves[iIndex];
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: _ServerKey: using named curve %s (ident=0x%04x)\n", pSecure->pEllipticCurve->strName, pSecure->pEllipticCurve->uIdent));
            break;
        }
        pData += 3;
    }
    // make sure the server sent us one of our supported elliptic curves
    if (pSecure->pEllipticCurve == NULL)
    {
        NetPrintf(("protossl: _ServerKey: no matching named curve when using ecdhe key exchange\n"));
        return(ST_FAIL_SETUP);
    }

    // copy the public key
    pSecure->uPubKeyLength = _SafeRead8(pData++, pDataEnd);
    _SafeReadBytes(pSecure->PubKey, sizeof(pSecure->PubKey), pData, pSecure->uPubKeyLength, pDataEnd);
    pData += pSecure->uPubKeyLength;

    // if we attempted a TLS 1.3 connection but were downgraded, reset the ecc parameters
    if (pState->uSslVersion >= PROTOSSL_VERSION_TLS1_3)
    {
        pSecure->bEccContextInitialized = FALSE;
        pSecure->bEccKeyGenerated = FALSE;
    }

    // get signature scheme
    if (pSecure->uSslVersion < SSL3_TLS1_2)
    {
        // EC ciphers in TLS versions prior to 1.2 do not include their signature algorithm in the packet data and are assumed to be SHA1
        pSigScheme = _ProtoSSLGetSignatureScheme(pSecure->Cert.iKeyType == ASN_OBJ_ECDSA_KEY ? SSL3_SIGSCHEME_ECDSA_SHA1 : SSL3_SIGSCHEME_RSA_PKCS1_SHA1);
        pSigData = pData;
    }
    else
    {
        if ((pSigScheme = _ProtoSSLGetSignatureScheme(_SafeRead16(pData, pDataEnd))) == NULL)
        {
            NetPrintf(("protossl: _ServerKey: unsupported signature scheme or signature algorithm\n"));
            return(ST_FAIL_SETUP);
        }
        pSigData = pData+2;
    }
    // get signature hash
    eHashType = _SSL3_HashIdToCrypt[pSigScheme->SigAlg.uHashId];

    // calculate and verify the signature
    if ((pHash = CryptHashGet(eHashType)) != NULL)
    {
        uint8_t aHashDigest[CRYPTHASH_MAXDIGEST];
        int32_t iHashSize, iSigSize = _SafeRead16(pSigData, pDataEnd);
        pSigData += 2;

        // create signature hash
        if ((pSecure->uSslVersion < SSL3_TLS1_2) && (pSigScheme->uIdent != SSL3_SIGSCHEME_ECDSA_SHA1))
        {
            // ecdhe-rsa-sha cipher suites
            iHashSize = _ProtoSSLGenerateSignatureHash(pSecure, CryptHashGet(CRYPTHASH_MD5), aHashDigest, pBegin, (uint32_t)(pData-pBegin));
            iHashSize += _ProtoSSLGenerateSignatureHash(pSecure, CryptHashGet(CRYPTHASH_SHA1), aHashDigest+iHashSize, pBegin, (uint32_t)(pData-pBegin));
        }
        else
        {
            iHashSize = _ProtoSSLGenerateSignatureHash(pSecure, pHash, aHashDigest, pBegin, (uint32_t)(pData-pBegin));
        }

        // verify the signature; note this is executed iteratively
        iNextState = _ProtoSSLVerifySignature(pState, pSecure, pSigScheme, pSigData, iSigSize, aHashDigest, iHashSize, pSecure->uSslVersion >= SSL3_TLS1_2 ? eHashType : CRYPTHASH_NULL, ST3_RECV_HELLO);
    }
    else
    {
        NetPrintf(("protossl: _ServerKey: unsupported signature hash\n"));
        return(ST_FAIL_SETUP);
    }

    return(iNextState);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLSendCertificateRequest

    \Description
        Send CertificateRequest handshake packet

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST3_SEND_DONE)

    \Version 10/15/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLSendCertificateRequest(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;
    uint8_t strHead[4], strBody[128];
    int32_t iBodySize;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send Certificate Request\n"));

    if (pSecure->uSslVersion < SSL3_TLS1_3)
    {
        // setup the body
        iBodySize = 0;
        strBody[iBodySize++] = 2;                   // number of certificate types
        strBody[iBodySize++] = SSL3_SIGN_RSA;       // rsa signing support
        strBody[iBodySize++] = SSL3_SIGN_ECDSA;     // ecdsa signing support

        // add TLS1.2 required SignatureAndHashAlgorithm as per http://tools.ietf.org/html/rfc5246#section-7.4.4
        if (pSecure->uSslVersion >= SSL3_TLS1_2)
        {
            int32_t iScheme, iNumSchemes = sizeof(_SSL3_SignatureSchemes)/sizeof(_SSL3_SignatureSchemes[0]);
            strBody[iBodySize++] = 0;
            strBody[iBodySize++] = iNumSchemes * 2;
            for (iScheme = 0; iScheme < iNumSchemes; iScheme += 1)
            {
                strBody[iBodySize++] = (uint8_t)(_SSL3_SignatureSchemes[iScheme].uIdent>>8);
                strBody[iBodySize++] = (uint8_t)(_SSL3_SignatureSchemes[iScheme].uIdent>>0);
            }
        }

        // number of certificate authorities (not supported)
        strBody[iBodySize++] = 0;
        strBody[iBodySize++] = 0;
    }
    else
    {
        const uint8_t *pDataEnd;
        // write empty certificate_request_context, which we do not require as we don't support post-handshake authentication
        strBody[0] = 0;
        // add signature_algorithms extension
        pDataEnd = _ProtoSSLAddHelloExtensions(pState, strBody+1, sizeof(strBody)-1, strBody, PROTOSSL_HELLOEXTN_SIGALGS);
        iBodySize = pDataEnd - strBody;
    }

    // setup the header
    strHead[0] = SSL3_MSG_CERT_REQ;
    strHead[1] = 0;
    strHead[2] = 0;
    strHead[3] = iBodySize;

    // setup packet for send
    _ProtoSSLSendPacket(pState, SSL3_REC_HANDSHAKE, strHead, sizeof(strHead), strBody, iBodySize);
    return((pSecure->uSslVersion < SSL3_TLS1_3) ? ST3_SEND_DONE : ST3_SEND_CERT);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvCertificateRequest

    \Description
        Process CertificateRequest handshake packet

    \Input *pState      - module state reference
    \Input *pData       - packet data
    \Input iDataSize    - size of packet data

    \Output
        int32_t         - next state (ST3_RECV_HELLO, or ST_FAIL_* on error)

    \Version 10/18/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLRecvCertificateRequest(ProtoSSLRefT *pState, const uint8_t *pData, int32_t iDataSize)
{
    SecureStateT *pSecure = pState->pSecure;
    const uint8_t *pDataEnd = pData + iDataSize;
    int32_t iType, iNumTypes;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process CertificateRequest\n"));

    // process tls1.2 and prior certificate request
    if (pSecure->uSslVersion <= SSL3_TLS1_2)
    {
        uint8_t uSign;
        // make sure an rsa/ecdsa certificate is permissable
        for (iType = 0, iNumTypes = _SafeRead8(pData++, pDataEnd); iType < iNumTypes; iType += 1)
        {
            uSign = _SafeRead8(pData+iType, pDataEnd);
            if ((uSign == SSL3_SIGN_RSA) || (uSign == SSL3_SIGN_ECDSA))
            {
                break;
            }
        }
        // makes sure we got a valid signing algorithm
        if (iType == iNumTypes)
        {
            NetPrintf(("protossl: no valid cert signing option found in RecvCertificateRequest\n"));
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_DECODE_ERROR);
            return(ST_FAIL_SETUP);
        }
        // skip past cert types
        pData += iNumTypes;

        // TLS1.2 includes SignatureAndHashAlgorithm specification: http://tools.ietf.org/html/rfc5246#section-7.4.4
        if (pSecure->uSslVersion == SSL3_TLS1_2)
        {
            // get and skip list length
            uint32_t uSigSchemeLen = _SafeRead16(pData, pDataEnd);
            pData += 2;
            // pick first supported signature algorithm
            if ((pSecure->pSigScheme = _ProtoSSLChooseSignatureScheme(pSecure, pState->pCertificate, pData, uSigSchemeLen, pDataEnd)) == NULL)
            {
                NetPrintf(("protossl: no valid signature algorithm found RecvCertificateRequest\n"));
                _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
                return(ST_FAIL_SETUP);
            }
        }
    }
    else
    {
        // parse tls1.3 certificate request as per https://tools.ietf.org/html/rfc8446#section-4.3.2
        int32_t iParseResult;
        // skip certificate_request_context, which we do not require as we don't support post-handshake authentication
        pData += _SafeRead8(pData, pDataEnd)+1;
        // parse signature_algorithms 
        if ((iParseResult = _ProtoSSLParseHelloExtensions(pState, pData, pDataEnd, SSL_EXTN_SIGNATURE_ALGORITHMS)) != 0)
        {
            return(iParseResult);
        }
    }

    /* As per http://tools.ietf.org/html/rfc5246#section-7.4.6, if no suitable certificate is available,
       the client MUST send a certificate message containing no certificates, so we don't bother to check
       whether we have a certificate here */
    pState->iClientCertLevel = 1;
    return(ST3_RECV_HELLO);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLSendServerHelloDone

    \Description
        Send ServerHelloDone handshake packet; marks end of server hello sequence

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST3_RECV_HELLO)

    \Notes
        Applies to TLS1.2 and prior only

    \Version 10/15/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLSendServerHelloDone(ProtoSSLRefT *pState)
{
    uint8_t strHead[4];

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send ServerHelloDone\n"));

    // setup the header
    strHead[0] = SSL3_MSG_SERVER_DONE;
    strHead[1] = 0;
    strHead[2] = 0;
    strHead[3] = 0;

    // setup packet for send
    _ProtoSSLSendPacket(pState, SSL3_REC_HANDSHAKE, strHead, 4, NULL, 0);
    return(ST3_RECV_HELLO);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvServerHelloDone

    \Description
        Process ServerHelloDone handshake packet; marks end of server hello sequence

    \Input *pState      - module state reference
    \Input *pData       - packet data
    \Input iDataSize    - size of packet data

    \Output
        int32_t         - next state (ST3_SEND_CERT or ST3_SEND_KEY)

    \Notes
        Applies to TLS1.2 and prior only

    \Version 10/15/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLRecvServerHelloDone(ProtoSSLRefT *pState, const uint8_t *pData, int32_t iDataSize)
{
    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process ServerHelloDone\n"));
    return((pState->iClientCertLevel != 0) ? ST3_SEND_CERT : ST3_SEND_KEY);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateSendClientKeyExchange

    \Description
        Send ClientKeyExchange handshake packet to the server

    \Input *pState  - module state reference

    \Output
        int32_t     - next state (ST3_SEND_VERIFY or ST3_SEND_CHANGE on completion,
                      ST3_SEND_KEY if computation is ongoing, ST_FAIL_SETUP on error)

    \Version 01/23/2017 (eesponda) Updated to support ECDHE
*/
/********************************************************************************F*/
static int32_t _ProtoSSLSendClientKeyExchange(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;
    uint8_t aHead[6];

    // handle rsa key exchange
    if (pState->pSecure->pCipher->uKey == SSL3_KEY_RSA)
    {
        if (!pSecure->bRSAContextInitialized)
        {
            // build pre-master secret
            CryptRandGet(pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey));
            /* From http://tools.ietf.org/html/rfc5246#section-7.4.7.1, client_version: The version requested
               by the client in the ClientHello.  This is used to detect version roll-back attacks */
            pSecure->PreMasterKey[0] = (uint8_t)(pSecure->uSslClientVersion>>8);
            pSecure->PreMasterKey[1] = (uint8_t)(pSecure->uSslClientVersion>>0);

            // init rsa state
            if (CryptRSAInit(&pSecure->RSAContext, pSecure->Cert.KeyModData, pSecure->Cert.iKeyModSize, pSecure->Cert.KeyExpData, pSecure->Cert.iKeyExpSize))
            {
                _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_INTERNAL_ERROR);
                return(ST_FAIL_SETUP);
            }
            CryptRSAInitMaster(&pSecure->RSAContext, pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey));
            pSecure->bRSAContextInitialized = TRUE;
        }

        // encrypt the premaster key (iterative)
        if (CryptRSAEncrypt(&pSecure->RSAContext, 1))
        {
            return(ST3_SEND_KEY);
        }
        pSecure->bRSAContextInitialized = FALSE;

        // update ssl perf timer
        NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (encrypt rsa client premaster secret) %dms (%d calls)\n",
            pSecure->RSAContext.uCryptMsecs, pSecure->RSAContext.uNumExpCalls));
        pSecure->uTimer += pSecure->RSAContext.uCryptMsecs;

        // build master secret
        _ProtoSSLBuildKey(pState, pSecure->MasterKey, sizeof(pSecure->MasterKey), pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey),
            pSecure->ClientRandom, pSecure->ServerRandom, sizeof(pSecure->ClientRandom), "master secret",
            pSecure->uSslVersion);

        // setup the header
        aHead[0] = SSL3_MSG_CLIENT_KEY;
        aHead[1] = 0;
        aHead[2] = (uint8_t)((pSecure->Cert.iKeyModSize+2)>>8);
        aHead[3] = (uint8_t)((pSecure->Cert.iKeyModSize+2)>>0);
        aHead[4] = (uint8_t)(pSecure->Cert.iKeyModSize>>8);
        aHead[5] = (uint8_t)(pSecure->Cert.iKeyModSize>>0);

        // setup packet for send
        _ProtoSSLSendPacket(pState, SSL3_REC_HANDSHAKE, aHead, 6, pSecure->RSAContext.EncryptBlock, pSecure->RSAContext.iKeyModSize);
    }
    else
    {
        uint8_t aPublicKey[128];
        int32_t iPublicKeySize;
        uint32_t uCryptUsecs;
        const CryptCurveDhT *pEcc;

        /* generate public key; elliptic curve generation takes multiple frames
           so stay in the same state until the operation is complete */

        // initialize elliptic curve context if not already initialized
        if ((pEcc = _ProtoSSLEccInitContext(pSecure, pSecure->pEllipticCurve)) == NULL)
        {
            /* if we cannot find the dh functions, there is some configuration mishap or the server is faulty.
               let's fail early here so we can debug the issue instead of a null pointer exception */
            return(ST_FAIL_SETUP);
        }
        // generate the public key
        if (pEcc->Public(&pSecure->EccContext, NULL, &uCryptUsecs) > 0)
        {
            return(ST3_SEND_KEY);
        }
        NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (generate public key for client key message) %dms\n",
            uCryptUsecs/1000));
        pSecure->uTimer += uCryptUsecs/1000;

        // encode public key into buffer
        iPublicKeySize = pEcc->PointFinal(pSecure->EccContext, NULL, FALSE, aPublicKey+1, sizeof(aPublicKey)-1);
        aPublicKey[0] = iPublicKeySize;

        // format header
        aHead[0] = SSL3_MSG_CLIENT_KEY;
        aHead[1] = 0;
        aHead[2] = 0;
        aHead[3] = iPublicKeySize+1;

        // setup packet for send
        _ProtoSSLSendPacket(pState, SSL3_REC_HANDSHAKE, aHead, 4, aPublicKey, iPublicKeySize+1);
    }

    // move to next state
    return(pSecure->bSentCert ? ST3_SEND_VERIFY : ST3_SEND_CHANGE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvClientKeyExchange

    \Description
        Process ClientKeyExchange handshake packet

    \Input *pState      - module state reference
    \Input *pData       - packet data
    \Input iDataSize    - size of packet data

    \Output
        int32_t         - next state (ST3_RECV_HELLO, ST3_RECV_CHANGE, or ST_FAIL_SETUP on error)

    \Version 10/15/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLRecvClientKeyExchange(ProtoSSLRefT *pState, const uint8_t *pData, int32_t iDataSize)
{
    SecureStateT *pSecure = pState->pSecure;
    int32_t iCryptKeySize, iPrivKeySize;
    uint8_t strRandomSecret[48];
    X509PrivateKeyT PrivateKey;
    const uint8_t *pCryptKeyData;
    uint16_t uSslVersion;
    const uint8_t *pDataEnd = pData+iDataSize;

    // handle RSA key exchange
    if (pSecure->pCipher->uKey == SSL3_KEY_RSA)
    {
        // read encrypted key data
        iCryptKeySize = _SafeRead16(pData, pDataEnd);
        pCryptKeyData = pData + 2;

        // decode private key and extract key modulus and private exponent
        if ((iPrivKeySize = _CertificateDecodePrivate(pState->pPrivateKey, pState->iPrivateKeyLen, &PrivateKey)) < 0)
        {
            NetPrintf(("protossl: unable to decode private key\n"));
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_INTERNAL_ERROR);
            return(ST_FAIL_SETUP);
        }

        // decrypt client premaster secret using private key
        NetPrintfVerbose((pState->iVerbose, 1, "protossl: decrypt client key (iKeySize=%d, iKeyModSize=%d, iKeyExpSize=%d)\n", iPrivKeySize, PrivateKey.Modulus.iObjSize, PrivateKey.PrivateExponent.iObjSize));
        if (CryptRSAInit2(&pSecure->RSAContext, PrivateKey.Modulus.iObjSize, &PrivateKey.PrimeP, &PrivateKey.PrimeQ, &PrivateKey.ExponentP, &PrivateKey.ExponentQ, &PrivateKey.Coefficient))
        {
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_INTERNAL_ERROR);
            return(ST_FAIL_SETUP);
        }
        CryptRSAInitSignature(&pSecure->RSAContext, pCryptKeyData, iCryptKeySize);
        CryptRSAEncrypt(&pSecure->RSAContext, 0);

        NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (rsa decrypt client premaster secret) %dms\n", pSecure->RSAContext.uCryptMsecs));
        pSecure->uTimer += pSecure->RSAContext.uCryptMsecs;

        // copy out and display decrypted client premaster key data
        ds_memcpy(pSecure->PreMasterKey, pSecure->RSAContext.EncryptBlock+iCryptKeySize-sizeof(pSecure->PreMasterKey), sizeof(pSecure->PreMasterKey));
        #if DEBUG_RAW_DATA
        NetPrintMem(pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey), "client premaster key");
        #endif

        // generate random for use in possible roll-back attack handling
        CryptRandGet(strRandomSecret, sizeof(strRandomSecret));
        // validate client TLS version (first two bytes of premaster secret)
        if ((uSslVersion = (pSecure->PreMasterKey[0] << 8) | pSecure->PreMasterKey[1]) != pSecure->uSslClientVersion)
        {
            NetPrintf(("protossl: detected possible roll-back attack; premaster secret tls version %d.%d does not match client-specified version\n",
                pSecure->PreMasterKey[0], pSecure->PreMasterKey[1]));
        }

        /* As per http://tools.ietf.org/html/rfc5246#section-7.4.7.1, a mismatch in the premaster secret tls version
           and the client-specified version should be handled by generating a random premaster secret and continuing
           on.  The random data should be generated unconditionally to prevent possible timing attacks.  This same
           response is also utilized for any detected PKCS#1 padding errors. */
        if ((uSslVersion != pSecure->uSslClientVersion) || !_Pkcs1VerifyRSAES(pSecure->RSAContext.EncryptBlock, iCryptKeySize, sizeof(pSecure->PreMasterKey)))
        {
            ds_memcpy(pSecure->PreMasterKey, strRandomSecret, sizeof(strRandomSecret));
        }

        // build master secret
        _ProtoSSLBuildKey(pState, pSecure->MasterKey, sizeof(pSecure->MasterKey), pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey),
            pSecure->ClientRandom, pSecure->ServerRandom, sizeof(pSecure->ClientRandom), "master secret",
            pSecure->uSslVersion);
    }
    else
    {
        // for ecdhe key exchange we just copy the public key
        pSecure->uPubKeyLength = _SafeRead8(pData++, pDataEnd);
        _SafeReadBytes(pSecure->PubKey, sizeof(pSecure->PubKey), pData, pSecure->uPubKeyLength, pDataEnd);
    }

    // move on to next state
    return(((pState->iClientCertLevel > 0) && pSecure->bRecvCert) ? ST3_RECV_HELLO : ST3_RECV_CHANGE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLSendCertificateVerify

    \Description
        Send CertificateVerify handshake packet

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST3_SEND_CHANGE or ST3_SEND_FINISH on completion,
                          ST3_SEND_VERIFY if RSA computation is ongoing, or ST_FAIL_SETUP
                          on error)

    \Version 10/30/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLSendCertificateVerify(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;
    uint32_t uHeadSize;
    uint8_t strHead[16];

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send CertificateVerify\n"));

    // set up for encryption of certificate verify message
    if (!pSecure->bSigGenerated)
    {
        uint8_t aSigObj[SSL_SIG_MAX+128]; // must be big enough to hold max hash size plus ASN.1 object encoding (TLS1.2), or a full signature object (TLS1.3+)
        int32_t iPrivKeySize, iHashSize=0;
        X509PrivateKeyT PrivateKey;

        // decode private key and extract key modulus and private exponent
        if ((iPrivKeySize = _CertificateDecodePrivate(pState->pPrivateKey, pState->iPrivateKeyLen, &PrivateKey)) < 0)
        {
            NetPrintf(("protossl: unable to decode private key\n"));
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_INTERNAL_ERROR);
            return(ST_FAIL_SETUP);
        }

        // set up buffer with current handshake hash
        if (pSecure->uSslVersion < SSL3_TLS1_2)
        {
            // select signature scheme based on server/client certificate
            pSecure->pSigScheme = _ProtoSSLGetSignatureScheme(pState->pCertificate->iKeyType == ASN_OBJ_ECDSA_KEY ? SSL3_SIGSCHEME_ECDSA_SHA1 : SSL3_SIGSCHEME_RSA_PKCS1_SHA1);
            // generate digest hash
            if (pSecure->pSigScheme->SigAlg.uSigAlg != SSL3_SIGALG_ECDSA)
            {
                iHashSize = _ProtoSSLHandshakeHashGet(pSecure, CRYPTHASH_MD5, aSigObj, SSL3_MAC_MD5);
            }
            iHashSize += _ProtoSSLHandshakeHashGet(pSecure, CRYPTHASH_SHA1, aSigObj+iHashSize, SSL3_MAC_SHA);
        }
        else
        {
            CryptHashTypeE eHashType = _SSL3_HashIdToCrypt[pSecure->pSigScheme->SigAlg.uHashId];
            uint8_t aMessageHash[CRYPTHASH_MAXDIGEST], aTempHash[CRYPTHASH_MAXDIGEST];

            // get message digest hash
            if (pSecure->uSslVersion < SSL3_TLS1_3)
            {
                // get handshake hash using signature hash algorithm
                iHashSize = _ProtoSSLHandshakeHashGet(pSecure, eHashType, aMessageHash, sizeof(aMessageHash));
            }
            else
            {
                // get handshake hash using cipher prf hash algorithm
                iHashSize = _ProtoSSLHandshakeHashGet(pSecure, pSecure->pCipher->uPrfType, aTempHash, sizeof(aTempHash));
                // construct tls1.3 digital signature envelope using signature hash algorithm
                iHashSize = _ProtoSSLGenerateCertificateVerifyHash(aMessageHash, eHashType, aTempHash, iHashSize, pState->bServer, TRUE);
            }

            // encode message digest hash
            if ((pSecure->pSigScheme->uVerifyScheme == SSL3_SIGVERIFY_RSA_PKCS1) && (pSecure->uSslVersion < SSL3_TLS1_3))
            {
                iHashSize = _AsnWriteDigitalHashObject(aSigObj, sizeof(aSigObj), aMessageHash, iHashSize, eHashType);
            }
            else if (pSecure->pSigScheme->uVerifyScheme == SSL3_SIGVERIFY_RSA_PSS)
            {
                iHashSize = _Pkcs1EncodeEMSA_PSS(aSigObj, PrivateKey.Modulus.iObjSize, aMessageHash, iHashSize, eHashType);
            }
            else // ecdsa signatures have no additional encoding
            {
                ds_memcpy_s(aSigObj, sizeof(aSigObj), aMessageHash, iHashSize);
            }
        }

        #if DEBUG_RAW_DATA
        NetPrintMem(aSigObj, iHashSize, "certificate verify hash");
        #endif

        // start async signature generation
        return(_ProtoSSLGenerateSignature(pState, pSecure, pSecure->pSigScheme, aSigObj, iHashSize, &PrivateKey, ST3_SEND_VERIFY));
    }

    // setup the header
    strHead[0] = SSL3_MSG_CERT_VERIFY;
    strHead[1] = 0;

    if (pSecure->uSslVersion < SSL3_TLS1_2)
    {
        strHead[2] = (uint8_t)((pSecure->SigVerify.iSigSize+2)>>8);
        strHead[3] = (uint8_t)((pSecure->SigVerify.iSigSize+2)>>0);
        strHead[4] = (uint8_t)(pSecure->SigVerify.iSigSize>>8);
        strHead[5] = (uint8_t)(pSecure->SigVerify.iSigSize>>0);
        uHeadSize = 6;
    }
    else
    {
        strHead[2] = (uint8_t)((pSecure->SigVerify.iSigSize+4)>>8);
        strHead[3] = (uint8_t)((pSecure->SigVerify.iSigSize+4)>>0);
        strHead[4] = (uint8_t)(pSecure->pSigScheme->uIdent>>8);
        strHead[5] = (uint8_t)(pSecure->pSigScheme->uIdent>>0);
        strHead[6] = (uint8_t)(pSecure->SigVerify.iSigSize>>8);
        strHead[7] = (uint8_t)(pSecure->SigVerify.iSigSize>>0);
        uHeadSize = 8;
    }

    // send the packet
    _ProtoSSLSendPacket(pState, SSL3_REC_HANDSHAKE, strHead, uHeadSize, pSecure->SigVerify.aSigData, pSecure->SigVerify.iSigSize);

    // transition to next state
    return((pSecure->uSslVersion < SSL3_TLS1_3) ? ST3_SEND_CHANGE : ST3_SEND_FINISH);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvCertificateVerify

    \Description
        Process CertificateVerify handshake packet

    \Input *pState      - module state reference
    \Input *pData       - packet data
    \Input iDataSize    - size of packet data

    \Output
        int32_t         - next state (ST3_RECV_CHANGE, ST3_RECV_FINISH, ST_PROC_ASYNC,
                          or ST_FAIL_SETUP on error)

    \Version 10/30/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLRecvCertificateVerify(ProtoSSLRefT *pState, const uint8_t *pData, int32_t iDataSize)
{
    SecureStateT *pSecure = pState->pSecure;
    int32_t iCryptKeySize, iHashSize;
    const uint8_t *pCryptKeyData;
    CryptHashTypeE eHashType = CRYPTHASH_NULL;
    const uint8_t *pDataEnd = pData+iDataSize;
    uint8_t aHandshakeHash[CRYPTHASH_MAXDIGEST];
    const SignatureSchemeT *pSigScheme = NULL;
    int32_t iNextState = (pSecure->uSslVersion < SSL3_TLS1_3) ? ST3_RECV_CHANGE : ST3_RECV_FINISH;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process CertificateVerify\n"));

    // get encrypted key data size and offset
    if (pSecure->uSslVersion < SSL3_TLS1_2)
    {
        iCryptKeySize = _SafeRead16(pData, pDataEnd);
        pCryptKeyData = pData + 2;
        pSigScheme = _ProtoSSLGetSignatureScheme((pSecure->Cert.iKeyType == ASN_OBJ_ECDSA_KEY) ? SSL3_SIGSCHEME_ECDSA_SHA1 : SSL3_SIGSCHEME_RSA_PKCS1_SHA1);
    }
    else
    {
        // get signature info
        pSigScheme = _ProtoSSLGetSignatureScheme(_SafeRead16(pData, pDataEnd));
        // validate scheme and key type; as per https://tools.ietf.org/html/rfc8446#section-4.4.3 PKCS1 is not a supported signature scheme in tls1.3+
        if ((pSigScheme == NULL) || ((pSigScheme->uVerifyScheme == SSL3_SIGVERIFY_RSA_PKCS1) && (pSecure->uSslVersion >= SSL3_TLS1_3)))
        {
            NetPrintf(("protossl: unsupported hashid or signature algorithm in CertificateVerify\n"));
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_DECODE_ERROR);
            return(ST_FAIL_SETUP);
        }
        // get crypt key size and data
        iCryptKeySize = _SafeRead16(pData+2, pDataEnd);
        pCryptKeyData = pData + 4;
    }

    // get digest hash from signature scheme
    eHashType = _SSL3_HashIdToCrypt[pSigScheme->SigAlg.uHashId];

    // get handshake hash
    if ((pSecure->uSslVersion < SSL3_TLS1_2) && (pSigScheme->SigAlg.uSigAlg != SSL3_SIGALG_ECDSA))
    {
        iHashSize = _ProtoSSLHandshakeHashGet(pSecure, CRYPTHASH_MD5, aHandshakeHash, SSL3_MAC_MD5);
        iHashSize += _ProtoSSLHandshakeHashGet(pSecure, CRYPTHASH_SHA1, aHandshakeHash + iHashSize, SSL3_MAC_SHA);
    }
    else if (pSecure->uSslVersion < SSL3_TLS1_3)
    {
        iHashSize = _ProtoSSLHandshakeHashGet(pSecure, eHashType, aHandshakeHash, sizeof(aHandshakeHash));
    }
    else
    {
        // tls1.3 wraps handshake hash in a digital signature envelope
        uint8_t aHandshakeHash2[CRYPTHASH_MAXDIGEST];
        // get handshake hash
        iHashSize = _ProtoSSLHandshakeHashGet(pSecure, pSecure->pCipher->uPrfType, aHandshakeHash2, sizeof(aHandshakeHash2));
        // construct tls1.3 digital signature envelope
        iHashSize = _ProtoSSLGenerateCertificateVerifyHash(aHandshakeHash, eHashType, aHandshakeHash2, iHashSize, pState->bServer, FALSE);
    }

    // verify the signature; note this is executed iteratively
    return(_ProtoSSLVerifySignature(pState, pSecure, pSigScheme, pCryptKeyData, iCryptKeySize, aHandshakeHash, iHashSize, pSecure->uSslVersion >= SSL3_TLS1_2 ? eHashType : CRYPTHASH_NULL, iNextState));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLSendChangeCipherSpec

    \Description
        Send ChangeCipherSpec handshake packet

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST3_CHANGE, ST3_SEND_FINISH)

    \Notes
        Applies to TLS1.2 and prior only

    \Version 10/15/2013 (jbrookes) Updated to handle client and server
*/
/********************************************************************************F*/
static int32_t _ProtoSSLSendChangeCipherSpec(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;
    uint8_t strHead[4];

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send ChangeCipherSpec\n"));

    // generate the shared secret if necessary
    if (!pState->bServer && !pSecure->bSessionResume && !_ProtoSSLGenerateEccSharedSecret(pState))
    {
        return(ST3_SEND_CHANGE);
    }

    // build key material if we haven't already
    if (pSecure->pServerKey == NULL)
    {
        _ProtoSSLBuildKeyMaterial(pState);
    }

    // initialize write cipher
    if (pSecure->pCipher->uEnc == SSL3_ENC_AES)
    {
        CryptAesInit(&pSecure->WriteAes, pState->bServer ? pSecure->pServerKey : pSecure->pClientKey, pSecure->pCipher->uLen, CRYPTAES_KEYTYPE_ENCRYPT,
            pState->bServer ? pSecure->pServerInitVec : pSecure->pClientInitVec);
    }
    if (pSecure->pCipher->uEnc == SSL3_ENC_GCM)
    {
        CryptGcmInit(&pSecure->WriteGcm, pState->bServer ? pSecure->pServerKey : pSecure->pClientKey, pSecure->pCipher->uLen);
    }
    if (pSecure->pCipher->uEnc == SSL3_ENC_CHACHA)
    {
        CryptChaChaInit(&pSecure->WriteChaCha, pState->bServer ? pSecure->pServerKey : pSecure->pClientKey, pSecure->pCipher->uLen);
    }

    // mark as cipher change
    strHead[0] = 1;
    _ProtoSSLSendPacket(pState, SSL3_REC_CIPHER, strHead, 1, NULL, 0);

    // reset the sequence number
    pSecure->uSendSeqn = 0;
    return(ST3_SEND_FINISH);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvChangeCipherSpec

    \Description
        Process ChangeCipherSpec handshake packet

    \Input *pState      - module state reference
    \Input *pData       - packet data
    \Input iDataSize    - size of packet data

    \Output
        int32_t         - next state (ST3_RECV_CHANGE, ST3_RECV_FINISH)

    \Notes
        Applies to TLS1.2 and prior only

    \Version 10/15/2013 (jbrookes) Updated to handle client and server
*/
/********************************************************************************F*/
static int32_t _ProtoSSLRecvChangeCipherSpec(ProtoSSLRefT *pState, const uint8_t *pData, int32_t iDataSize)
{
    SecureStateT *pSecure = pState->pSecure;
    const uint8_t *pDataEnd = pData + iDataSize;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process ChangeCipherSpec\n"));

    // validate ccs message
    if ((iDataSize != 1) || (_SafeRead8(pData, pDataEnd) != 1))
    {
        NetPrintf(("protossl: invalid ChangeCipherSpec message\n"));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_DECODE_ERROR);
        pState->iClosed = 1;
        _ProtoSSLRecvReset(pSecure);
        pState->iState = ST_FAIL_SETUP;
    }

    // generate the shared secret if necessary
    if (pState->bServer && !pSecure->bSessionResume && !_ProtoSSLGenerateEccSharedSecret(pState))
    {
        return(ST3_RECV_CHANGE);
    }

    // build key material if we haven't already
    if (pSecure->pServerKey == NULL)
    {
        _ProtoSSLBuildKeyMaterial(pState);
    }

    // initialize read cipher
    if (pSecure->pCipher->uEnc == SSL3_ENC_AES)
    {
        CryptAesInit(&pSecure->ReadAes, pState->bServer ? pSecure->pClientKey : pSecure->pServerKey, pSecure->pCipher->uLen, CRYPTAES_KEYTYPE_DECRYPT,
            pState->bServer ? pSecure->pClientInitVec : pSecure->pServerInitVec);
    }
    if (pSecure->pCipher->uEnc == SSL3_ENC_GCM)
    {
        CryptGcmInit(&pSecure->ReadGcm, pState->bServer ? pSecure->pClientKey : pSecure->pServerKey, pSecure->pCipher->uLen);
    }
    if (pSecure->pCipher->uEnc == SSL3_ENC_CHACHA)
    {
        CryptChaChaInit(&pSecure->ReadChaCha, pState->bServer ? pSecure->pClientKey : pSecure->pServerKey, pSecure->pCipher->uLen);
    }

    // reset sequence number
    pSecure->uRecvSeqn = 0;

    // just gobble packet -- we assume next state is crypted regardless
    return(ST3_RECV_FINISH);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLSendFinished

    \Description
        Send Finished handshake packet

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST3_RECV_CHANGE, ST3_RECV_HELLO,
                                      ST3_RECV_FINISH, ST3_SECURE)

    \Version 10/23/2013 (jbrookes) Rewritten to handle client and server
*/
/********************************************************************************F*/
static int32_t _ProtoSSLSendFinished(ProtoSSLRefT *pState)
{
    uint8_t strHead[4];
    uint8_t strBody[256];
    SecureStateT *pSecure = pState->pSecure;
    static const struct SSLFinished _SendFinished[2] = {
        { "client finished", {ST3_RECV_CHANGE, ST3_SECURE } },
        { "server finished", {ST3_SECURE, ST3_RECV_CHANGE } },
    };
    const struct SSLFinished *pFinished = &_SendFinished[pState->bServer];
    int32_t iNextState = ST3_SECURE;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: SendFinished\n"));

    // do finish hash, save size in packet header
    strHead[3] = (uint8_t)_ProtoSSLGenerateFinishHash(strBody, pSecure, pFinished->strLabel, pState->bServer, TRUE);

    // setup the header
    strHead[0] = SSL3_MSG_FINISHED;
    strHead[1] = 0;
    strHead[2] = 0;

    // all sends from here on out are secure
    pSecure->bSendSecure = TRUE;

    // setup packet for send
    _ProtoSSLSendPacket(pState, SSL3_REC_HANDSHAKE, strHead, sizeof(strHead), strBody, strHead[3]);

    // tls1.3 handshake processing
    if (pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3)
    {
        if (pState->bServer)
        {
            // save handshake hash for building application secrets
            _ProtoSSLHandshakeHashGet(pSecure, pSecure->pCipher->uPrfType, pSecure->aFinishHash, sizeof(pSecure->aFinishHash));
        }
        else
        {
            // switch to application keys; this must come after _ProtoSSLSendPacket as the server expects the Finished packet to be sent using the handshake key
            _ProtoSSLBuildApplicationKey(pState, pSecure);
        }
    }

    // determine next state
    if (pSecure->uSslVersion < PROTOSSL_VERSION_TLS1_3)
    {
        // next state will be secure mode (handshaking complete) or recv cipher change depending on client/server and if resuming or not
        iNextState = pFinished->uNextState[pSecure->bSessionResume];
    }
    else if (pState->bServer)
    {
        // server will expect either Finished or Certificate if requesting a client cert
        iNextState = (pState->iClientCertLevel != 0) ? ST3_RECV_HELLO : ST3_RECV_FINISH;
    }
    return(iNextState);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvFinished

    \Description
        Process Finished handshake packet

    \Input *pState      - module state reference
    \Input *pData       - packet data
    \Input iDataSize    - size of packet data

    \Output
    int32_t             - next state (ST3_SEND_CHANGE, ST3_SEND_CERT, ST3_SEND_FINISH,
                          ST3_SECURE, or ST_FAIL_SETUP on error)

    \Version 10/23/2013 (jbrookes) Rewritten to handle client and server
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateRecvFinished(ProtoSSLRefT *pState, const uint8_t *pData, int32_t iDataSize)
{
    SecureStateT *pSecure = pState->pSecure;
    uint8_t aMacFinal[CRYPTHASH_MAXDIGEST];
    int32_t iMacSize, iNextState = ST3_SECURE;
    static const struct SSLFinished _RecvFinished[2] = {
        { "server finished", {ST3_SECURE, ST3_SEND_CHANGE } },
        { "client finished", {ST3_SEND_CHANGE, ST3_SECURE } },
    };
    const struct SSLFinished *pFinished = &_RecvFinished[pState->bServer];
    const uint8_t *pDataEnd = pData + iDataSize;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process RecvFinished\n"));

    // calculate the finish verification hash
    iMacSize = _ProtoSSLGenerateFinishHash(aMacFinal, pSecure, pFinished->strLabel, pState->bServer, FALSE);

    // verify the hash
    if (memcmp(aMacFinal, pData, DS_MIN(iMacSize, pDataEnd-pData)))
    {
        NetPrintf(("protossl: finish hash mismatch; failed setup\n"));
        #if DEBUG_RAW_DATA
        NetPrintMem(aMacFinal, iMacSize, "aMacFinal");
        NetPrintMem(pData, iMacSize, "pData");
        #endif  
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_DECRYPT_ERROR);
        return(ST_FAIL_SETUP);
    }

    // save session (tls1.2 and previous only)
    if (pSecure->uSslVersion <= PROTOSSL_VERSION_TLS1_2)
    {
        _SessionHistoryAdd(pSecure, pState->strHost, SockaddrInGetPort(&pState->PeerAddr), NULL, pSecure->SessionId, sizeof(pSecure->SessionId), pSecure->MasterKey);
    }

    // tls1.3 handshake processing
    if (pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3)
    {
        if (pState->bServer)
        {
            // switch to application keys
            _ProtoSSLBuildApplicationKey(pState, pSecure);
        }
        else
        {
            // update hash state with received Finished packet from server (which hasn't been processed yet)
            _ProtoSSLRecvHandshakeFinish(pState);
            // save current handshake hash
            _ProtoSSLHandshakeHashGet(pSecure, pSecure->pCipher->uPrfType, pSecure->aFinishHash, sizeof(pSecure->aFinishHash));
        }
    }
    
    // determine next state
    if (pSecure->uSslVersion < PROTOSSL_VERSION_TLS1_3)
    {
        // next state will be secure mode (handshaking complete) or send cipher change depending on client/server and if resuming or not
        iNextState = pFinished->uNextState[pSecure->bSessionResume];
    }
    else if (!pState->bServer)
    {
        // client will send either Finished or Certificate if a client cert was requested by the server
        iNextState = (pState->iClientCertLevel != 0) ? ST3_SEND_CERT : ST3_SEND_FINISH;
    }
    return(iNextState);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLSendHelloRequest

    \Description
        Send HelloRequest handshake packet

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST3_RECV_HELLO)

    \Version 03/28/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLSendHelloRequest(ProtoSSLRefT *pState)
{
    uint8_t strHead[4];

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send HelloRequest\n"));

    // setup the header
    strHead[0] = SSL3_MSG_HELLO_REQUEST;
    strHead[1] = 0;
    strHead[2] = 0;
    strHead[3] = 0;

    // setup packet for send
    _ProtoSSLSendPacket(pState, SSL3_REC_HANDSHAKE, strHead, sizeof(strHead), strHead, 0);

    // set up to receive ClientHello
    return(ST3_RECV_HELLO);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvHelloRequest

    \Description
        Receive HelloRequest handshake packet; we respond with a no_renegotation
        warning alert, or unexpected_message if the connection is TLS 1.3.

    \Input *pState      - module state reference
    \Input *pData       - packet data
    \Input iDataSize    - size of packet data

    \Output
        int32_t         - next state (current state)

    \Version 03/28/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLRecvHelloRequest(ProtoSSLRefT *pState, const uint8_t *pData, int32_t iDataSize)
{
    SecureStateT *pSecure = pState->pSecure;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process HelloRequest\n"));

    if (pSecure->uSslVersion < PROTOSSL_VERSION_TLS1_3)
    {
        // respond with no_renegotation alert; if we were to implement renegotation, we'd move to ST3_SEND_HELLO
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_WARNING, SSL3_ALERT_DESC_NO_RENEGOTIATION);
    }
    else
    {
        // hello request is not a valid tls 1.3 message
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_UNEXPECTED_MESSAGE);
    }

    return(pState->iState);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvNewSessionTicket

    \Description
        Process NewSessionTicket handshake packet; see
        https://tools.ietf.org/html/rfc8446#section-4.6.1

    \Input *pState      - module state reference
    \Input *pData       - packet data
    \Input iDataSize    - size of packet data

    \Output
        int32_t         - next state (ST3_SECURE)

    \Version 03/17/2017 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLRecvNewSessionTicket(ProtoSSLRefT *pState, const uint8_t *pData, int32_t iDataSize)
{
    const uint8_t *pDataEnd = pData + iDataSize;
    SecureStateT *pSecure = pState->pSecure;
    SessionTicketT SessTick;
    CryptHashTypeE eHashType = pSecure->pCipher->uPrfType;
    int32_t iHashLen = CryptHashGetSize(eHashType);

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process NewSessionTicket\n"));

    // initialize session ticket memory
    ds_memset(&SessTick, 0, sizeof(SessTick));
    // save receipt time
    SessTick.uRecvTime = time(NULL);
    // save resumption key
    ds_memcpy_s(SessTick.aResumeKey, sizeof(SessTick.aResumeKey), pSecure->pResumeSecret, iHashLen);
    // save hash type
    SessTick.eHashType = eHashType;

    // parse required session ticket fields
    SessTick.uLifetime = _SafeRead32(pData, pDataEnd);
    pData += 4;
    SessTick.uAgeAdd = _SafeRead32(pData, pDataEnd);
    pData += 4;

    // copy ticket nonce
    SessTick.uNonceLen = _SafeRead8(pData, pDataEnd);
    pData += 1;
    _SafeReadBytes(SessTick.aTicketNonce, sizeof(SessTick.aTicketNonce), pData, SessTick.uNonceLen, pDataEnd);
    pData += SessTick.uNonceLen;

    // copy ticket length
    SessTick.uTickLen = _SafeRead16(pData, pDataEnd);
    pData += 2;

    // make sure we have enough space
    if (SessTick.uTickLen > sizeof(SessTick.aTicketData))
    {
        NetPrintf(("protossl: session ticket too large for session history buffer\n"));
        return(ST3_SECURE);
    }

    // copy ticket data
    _SafeReadBytes(SessTick.aTicketData, sizeof(SessTick.aTicketData), pData, SessTick.uTickLen, pDataEnd);
    pData += SessTick.uTickLen;

    // parse extensions length, if present
    if (pData <= (pDataEnd-2))
    {
        SessTick.uExtnLen = _SafeRead16(pData, pDataEnd);
    }

    NetPrintfVerbose((pState->iVerbose, 0, "protossl: ticket_sni: %s:%d, ticket_lifetime=%d, ticket_age_add=0x%08x, nonce_len=%d, ticket_len=%d, extn_len=%d\n",
        pState->strHost, SockaddrInGetPort(&pState->PeerAddr), SessTick.uLifetime, SessTick.uAgeAdd, SessTick.uNonceLen, SessTick.uTickLen, SessTick.uExtnLen));

    // add ticket to session history
    _SessionHistoryAdd(pSecure, pState->strHost, SockaddrInGetPort(&pState->PeerAddr), &SessTick, NULL, 0, NULL);

    return(ST3_SECURE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvHandshakeValidate

    \Description
        Validate handshake message transition is permissible; as per
        https://tools.ietf.org/html/rfc8446#section-4: "a peer which receives a
        handshake message in an unexpected order MUST abort the handshake
        with an "unexpected_message" alert."

    \Input *pState      - protossl state
    \Input *pSecure     - secure state
    \Input iNxtMsg      - next message

    \Output
        int32_t         - true if validated, else false

    \Notes
        This function validates that a given handshake state transition is possible
        within the spec, but it does not validate that the transition is correct
        given the specific set of parameters (ciphers/signature algorithms/etc)
        selected.  Such validation is left up to the specific handshake handler.

    \Version 05/02/2019 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLRecvHandshakeValidate(ProtoSSLRefT *pState, SecureStateT *pSecure, int32_t iNxtMsg)
{
    static const SSLHandshakeMapT *_aHandshakeMaps[2][2] =
    {
        { _SSL3_ClientRecvMsgMap, _SSL3_ClientRecvMsgMap_13 },
        { _SSL3_ServerRecvMsgMap, _SSL3_ServerRecvMsgMap_13 }
    };
    uint32_t uMapVersIdx = (pSecure->uSslVersion >= PROTOSSL_VERSION_TLS1_3) ? 1 : 0;
    const SSLHandshakeMapT *pMap;
    int32_t iCount;

    for (iCount = 0, pMap = _aHandshakeMaps[pState->bServer][uMapVersIdx]; pMap[iCount].iCurMsg != -1; iCount += 1)
    {
        if (pSecure->iCurMsg != pMap[iCount].iCurMsg)
        {
            continue;
        }
        if (iNxtMsg != pMap[iCount].iNxtMsg)
        {
            continue;
        }
        break;
    }

    NetPrintfVerbose((DEBUG_MSG_LIST || (pMap[iCount].iCurMsg == -1), 0, "protossl: %s handshake message state transition %s(%d)->%s(%d)\n", (pMap[iCount].iCurMsg != -1) ? "validated" : "invalid",
        _ProtoSSLRecvGetHandshakeTypeName(pSecure->iCurMsg), pSecure->iCurMsg,
        _ProtoSSLRecvGetHandshakeTypeName(iNxtMsg), iNxtMsg));

    return(pMap[iCount].iCurMsg != -1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvHandshake

    \Description
        Decode ssl handshake packet, validate message flow

    \Input *pState      - protossl state
    \Input *pSecure     - secure state
    \Input *pMsgType    - [out] handshake message type

    \Output
        uint8_t *       - pointer to handshake packet start, or NULL if error

    \Version 11/10/2005 (gschaefer)
*/
/********************************************************************************F*/
static const uint8_t *_ProtoSSLRecvHandshake(ProtoSSLRefT *pState, SecureStateT *pSecure, uint8_t *pMsgType)
{
    uint8_t *pRecv = pSecure->RecvData+pSecure->iRecvHshkProg;
    
    // get message type
    *pMsgType = pRecv[0];

    // make sure next message is possible in handshake flow
    if (!_ProtoSSLRecvHandshakeValidate(pState, pSecure, *pMsgType))
    {
        return(NULL);
    }

    // update current message
    pSecure->iCurMsg = *pMsgType;

    // point to data
    return(pRecv+4);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLRecvHandshakeFinish

    \Description
        Complete processing of handshake packet, including computation of
        handshake hash.

    \Input *pState      - module state reference

    \Version 03/16/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLRecvHandshakeFinish(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;

    // if this is a handshake packet and we haven't already processed it
    if ((pSecure->RecvHead[0] == SSL3_REC_HANDSHAKE) && (pSecure->iRecvSize > 0) && (pSecure->iRecvSize >= pSecure->iRecvHshkSize))
    {
        // keep a running hash of received data for verify/finish hashes
        _ProtoSSLHandshakeHashUpdate(pSecure, pSecure->RecvData+pSecure->iRecvHshkProg, pSecure->iRecvHshkSize, "recv");
        // consume the packet
        pSecure->iRecvHshkProg += pSecure->iRecvHshkSize;
        pSecure->iRecvHshkSize = 0;
    }

    // see if all the data was consumed
    if (pSecure->iRecvHshkProg == pSecure->iRecvSize)
    {
        _ProtoSSLRecvReset(pSecure);
    }
}

/*
    main update loop
*/

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateAsync

    \Description
        Perform iterative operations

    \Input *pState      - module state reference
    \Input *pSecure     - secure state reference

    \Output
        int32_t         - one if async operation occurred, else zero

    \Version 02/15/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateAsync(ProtoSSLRefT *pState, SecureStateT *pSecure)
{
    #if DEBUG_ENC_PERF
    uint64_t uTickUsec = NetTickUsec();
    #endif
    int32_t iResult;
    // not in async state, return
    if (pState->iState != ST3_PROC_ASYNC)
    {
        return(0);
    }
    // execute async op and check for completion
    iResult = pState->AsyncInfo.pAsyncOp(pState);
    #if DEBUG_ENC_PERF
    NetPrintf(("protossl: async op took %qd us\n", NetTickDiff(NetTickUsec(), uTickUsec)));
    #endif
    if (iResult == 0)
    {
        pState->iState = pState->AsyncInfo.iNextState;
        pState->AsyncInfo.iNextState = 0;
        return(0);
    }
    else if (iResult < 0)
    {
        pState->iState = pState->AsyncInfo.iFailState;
        pState->AsyncInfo.iNextState = 0;
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, pState->AsyncInfo.iFailAlert);
        return(0);
    }
    // continuing to execute async op
    return(ST3_PROC_ASYNC);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateSetAsyncState

    \Description
        Set up for 'async' execution of pAsyncOp.  Asynchronous in this
        case means iterative execution from the main update loop.

    \Input *pState      - module state
    \Input *pAsyncOp    - function to execute asynchronously
    \Input iNextState   - state to transition to on success
    \Input iFailState   - state to transition to on failure
    \Input iFailAlert   - alert to send on failure

    \Output
        int32_t         - updated state (ST3_PROC_ASYNC)

    \Version 02/16/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateSetAsyncState(ProtoSSLRefT *pState, AsyncOpT *pAsyncOp, int32_t iNextState, int32_t iFailState, int32_t iFailAlert)
{
    pState->iState = ST3_PROC_ASYNC;
    pState->AsyncInfo.iNextState = iNextState;
    pState->AsyncInfo.iFailState = iFailState;
    pState->AsyncInfo.iFailAlert = iFailAlert;
    pState->AsyncInfo.pAsyncOp = pAsyncOp;
    return(_ProtoSSLUpdateAsync(pState, pState->pSecure));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateSend

    \Description
        Send data on a secure connection

    \Input *pState      - module state reference
    \Input *pSecure     - secure state reference

    \Output
        int32_t         - one if data was sent, else zero

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateSend(ProtoSSLRefT *pState, SecureStateT *pSecure)
{
    // handle send states if output buffer is empty
    if (pSecure->iSendProg == pSecure->iSendSize)
    {
        // deal with send states
        if (pState->iState == ST3_SEND_HELLO)
        {
            pState->iState = pState->bServer ? _ProtoSSLSendServerHello(pState) : _ProtoSSLSendClientHello(pState);
        }
        else if (pState->iState == ST3_SEND_HELLO_RETRY)
        {
            pState->iState = _ProtoSSLSendHelloRetryRequest(pState);
        }
        else if (pState->iState == ST3_SEND_EXTN)
        {
            pState->iState = _ProtoSSLSendEncryptedExtensions(pState);
        }
        else if (pState->iState == ST3_SEND_CERT)
        {
            pState->iState = _ProtoSSLSendCertificate(pState);
        }
        else if (pState->iState == ST3_SEND_CERT_REQ)
        {
            pState->iState = _ProtoSSLSendCertificateRequest(pState);
        }
        else if (pState->iState == ST3_SEND_DONE)
        {
            pState->iState = _ProtoSSLSendServerHelloDone(pState);
        }
        else if (pState->iState == ST3_SEND_KEY)
        {
            pState->iState = pState->bServer ? _ProtoSSLSendServerKeyExchange(pState) : _ProtoSSLSendClientKeyExchange(pState);
        }
        else if (pState->iState == ST3_SEND_VERIFY)
        {
            pState->iState = _ProtoSSLSendCertificateVerify(pState);
        }
        else if (pState->iState == ST3_SEND_CHANGE)
        {
            pState->iState = _ProtoSSLSendChangeCipherSpec(pState);
        }
        else if (pState->iState == ST3_SEND_FINISH)
        {
            pState->iState = _ProtoSSLSendFinished(pState);
        }
        else if (pState->iState == ST3_SEND_HELLO_REQUEST)
        {
            pState->iState = _ProtoSSLSendHelloRequest(pState);
        }
    }

    // send any queued data
    return(_ProtoSSLSendSecure(pState, pState->pSecure));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateRecvHandshake

    \Description
        Process a handshake message

    \Input *pState      - module state reference
    \Input *pSecure     - secure state reference

    \Output
            uint32_t    - FALSE if there was an error, else TRUE

    \Version 11/14/2017 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _ProtoSSLUpdateRecvHandshake(ProtoSSLRefT *pState, SecureStateT *pSecure)
{
    uint8_t bUnhandledMsg = FALSE, uMsgType;
    const uint8_t *pData;
    int32_t iDataSize;

    // make sure it's a handshake message
    if (pSecure->RecvHead[0] != SSL3_REC_HANDSHAKE)
    {
        /* as per https://tools.ietf.org/html/rfc8446#section-5, if a TLS implementation receives an unexpected record type,
           it MUST terminate the connection with an "unexpected_message" alert */
        NetPrintf(("protossl: received unexpected record %s (%d) when expecting a handshake record\n", _ProtoSSLRecvGetRecordTypeName(pSecure->RecvHead[0]), pSecure->RecvHead[0]));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_UNEXPECTED_MESSAGE);
        return(FALSE);
    }

    // calculate handshake packet size
    if (pSecure->iRecvHshkSize == 0)
    {
        pData = pSecure->RecvData + pSecure->iRecvHshkProg;
        pSecure->iRecvHshkSize = ((pData[1]<<16)|(pData[2]<<8)|(pData[3]<<0)) + SSL3_MSG_HEADER_SIZE;
        // make sure packet fits in our buffer
        if ((pSecure->iRecvHshkProg+pSecure->iRecvHshkSize) > (int32_t)sizeof(pSecure->RecvData))
        {
            NetPrintf(("protossl: _ProtoSSLUpdateRecvHandshake: packet at base %d is too long (%d vs %d)\n", pSecure->iRecvHshkProg,
                pSecure->iRecvHshkSize, sizeof(pSecure->RecvData)-pSecure->iRecvHshkProg));
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_DECODE_ERROR);
            return(FALSE);
        }
    }

    // make sure we have the entire packet before proceeding
    if (pSecure->iRecvSize < pSecure->iRecvHshkSize)
    {
        // receive next packet header
        pSecure->iRecvHead = 0;
        pSecure->bRecvProc = FALSE;
        return(TRUE);
    }

    // size of handshake data is handshake size minus header size
    iDataSize = pSecure->iRecvHshkSize - SSL3_MSG_HEADER_SIZE;

    // get handshake message and ensure it is valid in the handshake state machine
    if ((pData = _ProtoSSLRecvHandshake(pState, pSecure, &uMsgType)) == NULL)
    {
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_UNEXPECTED_MESSAGE);
        return(FALSE);
    }

    // process handshake message based on state
    if (pState->iState == ST3_RECV_HELLO)
    {
        if (uMsgType == SSL3_MSG_CLIENT_HELLO)
        {
            pState->iState = _ProtoSSLRecvClientHello(pState, pData, iDataSize);
        }
        else if (uMsgType == SSL3_MSG_SERVER_HELLO)
        {
            pState->iState = _ProtoSSLRecvServerHello(pState, pData, iDataSize);
        }
        else if (uMsgType == SSL3_MSG_ENCRYPTED_EXTENSIONS)
        {
            pState->iState = _ProtoSSLRecvEncryptedExtensions(pState, pData, iDataSize);
        }
        else if (uMsgType == SSL3_MSG_CERTIFICATE)
        {
            pState->iState = _ProtoSSLRecvCertificate(pState, pData, iDataSize);
        }
        else if (uMsgType == SSL3_MSG_CERT_REQ)
        {
            pState->iState = _ProtoSSLRecvCertificateRequest(pState, pData, iDataSize);
        }
        else if (uMsgType == SSL3_MSG_CERT_VERIFY)
        {
            pState->iState = _ProtoSSLRecvCertificateVerify(pState, pData, iDataSize);
        }
        else if (uMsgType == SSL3_MSG_CLIENT_KEY)
        {
            pState->iState = _ProtoSSLRecvClientKeyExchange(pState, pData, iDataSize);
        }
        else if (uMsgType == SSL3_MSG_SERVER_KEY)
        {
            pState->iState = _ProtoSSLRecvServerKeyExchange(pState, pData, iDataSize);
        }
        else if (uMsgType == SSL3_MSG_SERVER_DONE)
        {
            pState->iState = _ProtoSSLRecvServerHelloDone(pState, pData, iDataSize);
        }
        else
        {
            bUnhandledMsg = TRUE;
        }
    }
    else if (pState->iState == ST3_RECV_FINISH)
    {
        if (uMsgType == SSL3_MSG_FINISHED)
        {
            pState->iState = _ProtoSSLUpdateRecvFinished(pState, pData, iDataSize);
        }
        else
        {
            bUnhandledMsg = TRUE;
        }
    }
    else if (pState->iState == ST3_SECURE)
    {
        // handle post-finish handshake messages
        if (uMsgType == SSL3_MSG_HELLO_REQUEST)
        {
            pState->iState = _ProtoSSLRecvHelloRequest(pState, pData, iDataSize);
        }
        else if (uMsgType == SSL3_MSG_NEW_SESSION_TICKET)
        {
            pState->iState = _ProtoSSLRecvNewSessionTicket(pState, pData, iDataSize);
        }
        // finish here since we won't in _ProtoSSLUpdateRecvPacket()
        _ProtoSSLRecvHandshakeFinish(pState);
    }
    else
    {
        bUnhandledMsg = TRUE;
    }

    // fatal alert if message was unhandled
    if (bUnhandledMsg)
    {
        NetPrintf(("protossl: unhandled handshake message %s(%d) in state %d\n", _ProtoSSLRecvGetHandshakeTypeName(uMsgType), uMsgType, pState->iState));
        _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_UNEXPECTED_MESSAGE);
    }

    return(!bUnhandledMsg);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateRecvPacket

    \Description
        Process a received packet

    \Input *pState      - module state reference
    \Input *pSecure     - secure state reference

    \Version 11/14/2017 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLUpdateRecvPacket(ProtoSSLRefT *pState, SecureStateT *pSecure)
{
    int32_t iState = pState->iState;

    if (pSecure->RecvHead[0] == SSL3_REC_ALERT)
    {
        _ProtoSSLRecvAlert(pState, pSecure);
    }
    else if ((pSecure->RecvHead[0] == SSL3_REC_CIPHER) && (pState->iState == ST3_RECV_CHANGE))
    {
        if ((pState->iState = _ProtoSSLRecvChangeCipherSpec(pState, pSecure->RecvData+pSecure->iRecvHshkProg, pSecure->iRecvSize)) != ST3_RECV_CHANGE)
        {
            pSecure->iRecvHshkProg = pSecure->iRecvSize;
        }
    }
    else if (!_ProtoSSLUpdateRecvHandshake(pState, pSecure))
    {
        pState->iClosed = 1;
        _ProtoSSLRecvReset(pSecure);
        pState->iState = (pState->iState < ST3_SECURE) ? ST_FAIL_SETUP : ST_FAIL_SECURE;
    }

    // output final crypto timing, if we have just reached secure state
    if ((iState != pState->iState) && (pState->iState == ST3_SECURE) && (pSecure->uTimer > 0))
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: SSL Perf (setup) %dms\n", pSecure->uTimer));
        pSecure->uTimer = 0;
    }

    /* finish recv handshake processing: due to the fact that the recv hello state handles which state we are in based on the
       packet data we want to move forward regardless.  when we are not in the recv hello state we want to make sure to to only
       move to the next state when the state changes to allow for any additional processing that happens over multiple frames. */
    if ((pState->iState == ST3_RECV_HELLO) || (iState != pState->iState))
    {
        _ProtoSSLRecvHandshakeFinish(pState);
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateRecvValidate

    \Description
        Validate received packet

    \Input *pState      - module state reference
    \Input *pSecure     - secure state reference

    \Version 11/14/2017 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoSSLUpdateRecvValidate(ProtoSSLRefT *pState, SecureStateT *pSecure)
{
    /* validate the format version, and make sure it is a 3.x protocol version.  we make an assumption here that
       the 3.x packet format will not change in future revisions to the TLS protocol (this was confirmed with the
       1.3 version of the protocol).  validation of the specific protocol version is handled during handshaking. */
    if (pSecure->RecvHead[1] == (SSL3_VERSION>>8))
    {
        uint32_t uRecvSize = (pSecure->RecvHead[3]<<8)|(pSecure->RecvHead[4]<<0);
        // check data length to make sure it is valid; as per http://tools.ietf.org/html/rfc5246#section-6.2.1: The length MUST NOT exceed 2^14+2^11
        if (uRecvSize > SSL_RCVMAX_PACKET)
        {
            NetPrintf(("protossl: received oversized packet (%d/%d); terminating connection\n", uRecvSize, SSL_RCVMAX_PACKET));
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_RECORD_OVERFLOW);
            pState->iClosed = 1;
            _ProtoSSLRecvReset(pSecure);
            pState->iState = (pState->iState < ST3_SECURE) ? ST_FAIL_SETUP : ST_FAIL_SECURE;
        }
        // save data offset
        pSecure->iRecvBase = pSecure->iRecvSize;
        // add in the data length
        pSecure->iRecvSize += uRecvSize;
        // make sure it doesn't exceed our buffer
        if (pSecure->iRecvSize > (int32_t)sizeof(pSecure->RecvData))
        {
            NetPrintf(("protossl: packet length of %d is too large\n", pSecure->iRecvSize));
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_RECORD_OVERFLOW);
            pState->iClosed = 1;
            _ProtoSSLRecvReset(pSecure);
            pState->iState = (pState->iState < ST3_SECURE) ? ST_FAIL_SETUP : ST_FAIL_SECURE;
        }
    }
    else
    {
        if ((pSecure->RecvHead[0] == 0x80) && (pSecure->RecvHead[2] == 1))
        {
            NetPrintf(("protossl: received %d byte SSLv2 ClientHello offering SSLv%d.%d; disconnecting\n",
                pSecure->RecvHead[1], pSecure->RecvHead[3], pSecure->RecvHead[4]));
            pState->iState = ST_FAIL_CONN_SSL2;
        }
        else
        {
            NetPrintf(("protossl: received what appears to be a non-SSL connection attempt; disconnecting\n"));
            pState->iState = ST_FAIL_CONN_NOTSSL;
        }
        pState->iClosed = 1;
        _ProtoSSLRecvReset(pSecure);
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateRecv

    \Description
        Receive data on a secure connection

    \Input *pState      - module state reference
    \Input *pSecure     - secure state reference

    \Output
        int32_t         - 1=data was received, else 0

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateRecv(ProtoSSLRefT *pState, SecureStateT *pSecure)
{
    int32_t iResult, iXfer = 0;

    // receive ssl record header
    if (pSecure->iRecvHead < SSL_MIN_PACKET)
    {
        iResult = SocketRecv(pState->pSock, (char *)pSecure->RecvHead+pSecure->iRecvHead, SSL_MIN_PACKET-pSecure->iRecvHead, 0);
        if (iResult > 0)
        {
            pSecure->iRecvHead += iResult;
            iXfer = 1;
        }
        if (iResult < 0)
        {
            pState->iState = (pState->iState < ST3_SECURE) ? ST_FAIL_SETUP : ST_FAIL_SECURE;
            pState->iClosed = 1;
        }
    }
    // wait for a complete ssl record header before passing this point
    if (pSecure->iRecvHead < SSL_MIN_PACKET)
    {
        return(iXfer);
    }

    /* see if we can determine the full packet size
       this needs to handle the following situations:
       - initial receive of a packet (just the header has been received)
       - receive of the second or later packet in a fragmented handshake packet
       it needs to NOT be executed in the following scenario:
       - when processing a packet that has been received but has not had all of its handshake packets processed */
    if ((pSecure->iRecvProg == pSecure->iRecvSize) && !pSecure->bRecvProc)
    {
        _ProtoSSLUpdateRecvValidate(pState, pSecure);
    }

    // finish receiving ssl record data
    if (pSecure->iRecvProg < pSecure->iRecvSize)
    {
        iResult = SocketRecv(pState->pSock, (char *)pSecure->RecvData+pSecure->iRecvProg, pSecure->iRecvSize-pSecure->iRecvProg, 0);
        if (iResult > 0)
        {
            pSecure->iRecvProg += iResult;
            iXfer = 1;
        }
        if (iResult < 0)
        {
            pState->iState = (pState->iState < ST3_SECURE) ? ST_FAIL_SETUP : ST_FAIL_SECURE;
            pState->iClosed = 1;
        }
    }

    // handle decryption and data validation
    if ((pSecure->iRecvProg == pSecure->iRecvSize) && !pSecure->bRecvProc)
    {
        uint8_t bFirstRecord = (pSecure->iRecvBase == 0) ? TRUE : FALSE;
        int32_t iAlert = 0;
        // at end of receive, process the packet
        if ((iResult = _ProtoSSLRecvPacket(pState, &iAlert)) < 0)
        {
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_FATAL, iAlert);
            pState->iState = (pState->iState < ST3_SECURE) ? ST_FAIL_SETUP : ST_FAIL_SECURE;
            _ProtoSSLRecvReset(pSecure);
            pState->iClosed = 1;
        }
        // remember we've received it
        pSecure->bRecvProc = (pSecure->iRecvSize > 0) ? TRUE : FALSE;
        // initial handshake offset needs to be updated to skip stuff decryption might have skipped (e.g. AEAD explicit IV in tls1.2)
        if (bFirstRecord)
        {
            pSecure->iRecvHshkProg = pSecure->iRecvBase;
        }
    }

    // process received packet - note that this might get called multiple times per ssl record, or one or more times per multiple ssl records
    if ((pSecure->iRecvSize > 0) && (pSecure->iRecvProg == pSecure->iRecvSize) && ((pSecure->RecvHead[0] != SSL3_REC_APPLICATION) || (pState->iState == ST3_RECV_HELLO)) && (pState->iClosed == 0))
    {
        _ProtoSSLUpdateRecvPacket(pState, pSecure);
    }

    return(iXfer);
}

/*
    on-demand ca install
*/

/*F********************************************************************************/
/*!
    \Function _ProtoSSLInitiateCARequest

    \Description
        Initiate CA fetch request

    \Input *pState      - module state reference

    \Output
        int32_t         - 0=success, negative=failure

    \Version 02/28/2012 (szhu)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLInitiateCARequest(ProtoSSLRefT *pState)
{
    // we allow only one fetch request for each ssl negotiation attempt
    if (pState->bCertInfoSet && (pState->iCARequestId <= 0) && !pState->bServer)
    {
        if ((pState->iCARequestId = DirtyCertCARequestCert(&pState->CertInfo, pState->strHost, SockaddrInGetPort(&pState->PeerAddr))) > 0)
        {
            // save the failure cert, it will be validated again after fetching CA cert
            if (pState->pCertToVal == NULL)
            {
                pState->pCertToVal = DirtyMemAlloc(sizeof(*pState->pCertToVal), PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            }
            // continue to fetch CA cert even if we fail to allocate cert memory
            if (pState->pCertToVal != NULL)
            {
                ds_memcpy(pState->pCertToVal, &pState->pSecure->Cert, sizeof(*pState->pCertToVal));
            }
            return(0);
        }
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateCARequest

    \Description
        Update CA fetch request status

    \Input *pState      - module state reference

    \Output
        int32_t         - updated module state

    \Version 02/28/2012 (szhu)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateCARequest(ProtoSSLRefT *pState)
{
    int32_t iComplete;
    // see if request completed
    if ((iComplete = DirtyCertCARequestDone(pState->iCARequestId)) != 0)
    {
        DirtyCertCARequestFree(pState->iCARequestId);
        pState->iCARequestId = 0;
        // if CA fetch request failed
        if (iComplete < 0)
        {
            _CertificateSetFailureInfo(pState, pState->pCertToVal, TRUE);
            pState->iState = ST_FAIL_CERT_REQUEST;
        }
        // if cert not validated
        else if ((pState->pCertToVal == NULL) || (_ProtoSSLVerifyCertificate(pState, pState->pSecure, pState->pCertToVal, FALSE) != 0))
        {
            _CertificateSetFailureInfo(pState, pState->pCertToVal, TRUE);
            pState->iState = ST_FAIL_CERT_NOTRUST;
        }
        else
        {
            // cert validated
            #if DIRTYCODE_LOGGING
            char strIdentSubject[512], strIdentIssuer[512];
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: cert (%s) validated by ca (%s)\n", _CertificateDebugFormatIdent(&pState->pCertToVal->Subject, strIdentSubject, sizeof(strIdentSubject)),
                _CertificateDebugFormatIdent(&pState->pCertToVal->Issuer, strIdentIssuer, sizeof(strIdentIssuer))));
            #endif
            pState->iState = ST3_RECV_HELLO;
        }

        if (pState->pCertToVal != NULL)
        {
            DirtyMemFree(pState->pCertToVal, PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            pState->pCertToVal = NULL;
        }
    }
    return(pState->iState);
}

/*
    module state management
*/

/*F********************************************************************************/
/*!
    \Function _ProtoSSLAllocSecureState

    \Description
        Allocate secure state

    \Input iMemGroup            - memgroup info for alloc
    \Input *pMemGroupUserData   - memgroup info for alloc

    \Output
        SecureStateT *          - secure state, or null on alloc failure

    \Version 03/17/2010 (jbrookes)
*/
/********************************************************************************F*/
static SecureStateT *_ProtoSSLAllocSecureState(int32_t iMemGroup, void *pMemGroupUserData)
{
    SecureStateT *pSecure = DirtyMemAlloc(sizeof(*pSecure), PROTOSSL_MEMID, iMemGroup, pMemGroupUserData);
    if (pSecure != NULL)
    {
        ds_memclr(pSecure, sizeof(*pSecure));
    }
    return(pSecure);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLResetSecureState

    \Description
        Reset secure state.  Does not affect the TCP connection, if any.

    \Input *pState  - Reference pointer
    \Input iSecure  - secure status (0=disabled, 1=enabled)

    \Output
        int32_t     - SOCKERR_NONE on success, SOCKERR_NOMEM on failure

    \Version 03/17/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLResetSecureState(ProtoSSLRefT *pState, int32_t iSecure)
{
    SecureStateT *pSecure;

    // acquire access to secure state
    NetCritEnter(&pState->SecureCrit);

    // see if we need to get rid of secure state
    if (!iSecure && (pState->pSecure != NULL))
    {
        DirtyMemFree(pState->pSecure, PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        pState->pSecure = NULL;
    }

    // see if we need to alloc secure state
    if (iSecure && (pState->pSecure == NULL))
    {
        pState->pSecure = _ProtoSSLAllocSecureState(pState->iMemGroup, pState->pMemGroupUserData);
    }

    // reset secure state if present
    if ((pSecure = pState->pSecure) != NULL)
    {
        // clear secure state
        ds_memclr(pSecure, sizeof(*pSecure));

        // reset handshake hashes
        _ProtoSSLHandshakeHashInit(pSecure);
    }

    // release access to secure state
    NetCritLeave(&pState->SecureCrit);

    // return allocate error if secure wanted and failed
    return((iSecure && !pSecure) ? SOCKERR_NOMEM : SOCKERR_NONE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLResetState

    \Description
        Reset connection back to unconnected state (will disconnect from server).

    \Input *pState  - Reference pointer
    \Input iSecure  - to be completed

    \Output
        int32_t     - SOCKERR_NONE on success, SOCKERR_NOMEM on failure

    \Version 03/25/2004 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLResetState(ProtoSSLRefT *pState, int32_t iSecure)
{
    // close socket if needed
    if (pState->pSock != NULL)
    {
        pState->iLastSocketError = SocketInfo(pState->pSock, 'serr', 0, NULL, 0);
        SocketClose(pState->pSock);
        pState->pSock = NULL;
    }

    // done with resolver record
    if (pState->pHost != NULL)
    {
        pState->pHost->Free(pState->pHost);
        pState->pHost = NULL;
    }

    // free dirtycert certificate
    if (pState->pCertToVal != NULL)
    {
        DirtyMemFree(pState->pCertToVal, PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        pState->pCertToVal = NULL;
    }

    // done with dirtycert
    if (pState->iCARequestId > 0)
    {
        DirtyCertCARequestFree(pState->iCARequestId);
    }
    pState->iCARequestId = 0;

    // reset the state
    pState->iState = ST_IDLE;
    pState->iClosed = 1;
    pState->uAlertLevel = 0;
    pState->uAlertValue = 0;
    pState->bAlertSent = FALSE;

    // reset secure state
    return(_ProtoSSLResetSecureState(pState, iSecure));
}

/*
    trusted store management
*/

/*F********************************************************************************/
/*!
    \Function _ProtoSSLChkCACert

    \Description
        Check to see if the given CA cert already exists in our list of
        CA certs.

    \Input *pNewCACert  - pointer to new CA cert to check for duplicates of

    \Output
        int32_t         - zero=not duplicate, non-zero=duplicate

    \Version 05/11/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLChkCACert(const X509CertificateT *pNewCACert)
{
    const ProtoSSLCACertT *pCACert;

    for (pCACert = _ProtoSSL_CACerts; pCACert != NULL; pCACert = pCACert->pNext)
    {
        if (!_CertificateCompareIdent(&pCACert->Subject, &pNewCACert->Subject, TRUE) && (pCACert->iKeyModSize == pNewCACert->iKeyModSize) &&
            !memcmp(pCACert->pKeyModData, pNewCACert->KeyModData, pCACert->iKeyModSize))
        {
            break;
        }
    }

    return(pCACert != NULL);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLAddCACert

    \Description
        Add a new CA certificate to certificate list

    \Input *pCert       - pointer to new cert to add
    \Input bVerified    - TRUE if verified, else false
    \Input iMemGroup    - memgroup to use for alloc
    \Input *pMemGroupUserData - memgroup userdata for alloc

    \Output
        int32_t         - zero=error/duplicate, one=added

    \Version 01/13/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLAddCACert(X509CertificateT *pCert, uint8_t bVerified, int32_t iMemGroup, void *pMemGroupUserData)
{
    ProtoSSLCACertT *pCACert;
    int32_t iCertSize = sizeof(*pCACert) + pCert->iKeyModSize;

    // see if this certificate already exists
    if (_ProtoSSLChkCACert(pCert))
    {
        _CertificateDebugPrint(pCert, "ignoring redundant add of CA cert");
        return(0);
    }

    // find append point for new CA
    for (pCACert = _ProtoSSL_CACerts; pCACert->pNext != NULL; pCACert = pCACert->pNext)
        ;

    // allocate new record
    if ((pCACert->pNext = (ProtoSSLCACertT *)DirtyMemAlloc(iCertSize, PROTOSSL_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        _CertificateDebugPrint(pCert, "failed to allocate memory for cert");
        return(0);
    }

    // clear allocated memory
    pCACert = pCACert->pNext;
    ds_memclr(pCACert, iCertSize);

    // if this cert has not already been verified, allocate memory for X509 cert data and copy the X509 cert data for later validation
    if (!bVerified)
    {
        if ((pCACert->pX509Cert = (X509CertificateT *)DirtyMemAlloc(sizeof(*pCert), PROTOSSL_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
        {
            _CertificateDebugPrint(pCert, "failed to allocate memory for X509 cert");
            DirtyMemFree(pCACert->pNext, PROTOSSL_MEMID, iMemGroup, pMemGroupUserData);
            pCACert->pNext = NULL;
            return(0);
        }
        // copy cert data
        ds_memcpy(pCACert->pX509Cert, pCert, sizeof(*pCert));
    }

    // copy textual identity of this certificate
    // (don't need to save issuer since we already trust this certificate)
    ds_memcpy(&pCACert->Subject, &pCert->Subject, sizeof(pCACert->Subject));

    // copy key info
    pCACert->iKeyType = pCert->iKeyType;
    pCACert->iCrvType = pCert->iCrvType;

    // copy exponent data
    pCACert->iKeyExpSize = pCert->iKeyExpSize;
    ds_memcpy(pCACert->KeyExpData, pCert->KeyExpData, pCACert->iKeyExpSize);

    // copy modulus data, immediately following header
    pCACert->iKeyModSize = pCert->iKeyModSize;
    pCACert->pKeyModData = (uint8_t *)pCACert + sizeof(*pCACert);
    ds_memcpy((uint8_t *)pCACert->pKeyModData, pCert->KeyModData, pCACert->iKeyModSize);

    // save memgroup and user info used to allocate
    pCACert->iMemGroup = iMemGroup;

    _CertificateDebugPrint(pCert, "added new certificate authority");
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLSetCACert

    \Description
        Add one or more X.509 CA certificates to the trusted list. A
        certificate added will be available to all ProtoSSL modules for
        the lifetime of the application. This functional can add one or more
        PEM certificates or a single DER certificate.

    \Input *pCertData   - pointer to certificate data (PEM or DER)
    \Input iCertSize    - size of certificate data
    \Input bVerify      - if TRUE verify cert chain on add

    \Output
        int32_t         - negative=error, positive=count of CAs added

    \Notes
        The certificate must be in .DER (binary) or .PEM (base64-encoded)
        format.

    \Version 01/13/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLSetCACert(const uint8_t *pCertData, int32_t iCertSize, uint8_t bVerify)
{
    int32_t iResult, iCount = -1;
    X509CertificateT Cert;
    uint8_t *pCertBuffer = NULL;
    const int32_t _iMaxCertSize = 4096;
    const uint8_t *pCertBeg, *pCertEnd;
    int32_t iMemGroup;
    uint32_t uCertType;
    void *pMemGroupUserData;
    SecureStateT *pSecure;

    #if DIRTYCODE_LOGGING
    uint32_t uTick = NetTick();
    #endif

    // process PEM signature if present
    if (_CertificateFindData(pCertData, iCertSize, &pCertBeg, &pCertEnd, &uCertType) == 0)
    {
        // no markers -- consume all the data
        pCertBeg = pCertData;
        pCertEnd = pCertData+iCertSize;
    }

    // remember remaining data for possible further parsing
    iCertSize -= pCertEnd-pCertData;
    pCertData = pCertEnd;

    // get memgroup settings for certificate blob
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // if the cert is base64 encoded we decode it; otherwise we assume it is binary and parse it directly
    if ((iResult = Base64Decode2((int32_t)(pCertEnd-pCertBeg), (const char *)pCertBeg, NULL)) > 0)
    {
        if (iResult > _iMaxCertSize)
        {
            return(-111);
        }
        // allocate cert buffer
        if ((pCertBuffer = (uint8_t *)DirtyMemAlloc(_iMaxCertSize, PROTOSSL_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
        {
            return(-112);
        }
        // decode the cert
        Base64Decode2((int32_t)(pCertEnd-pCertBeg), (const char *)pCertBeg, (char *)pCertBuffer);
        pCertBeg = pCertBuffer;
        pCertEnd = pCertBeg+iResult;
    }

    // allocate temporary secure state to verify certificate with
    if ((pSecure = _ProtoSSLAllocSecureState(iMemGroup, pMemGroupUserData)) == NULL)
    {
        DirtyMemFree(pCertBuffer, PROTOSSL_MEMID, iMemGroup, pMemGroupUserData);
        return(-113);
    }

    // parse the x.509 certificate onto stack
    if ((iResult = _AsnParseCertificate(&Cert, pCertBeg, (int32_t)(pCertEnd-pCertBeg))) == 0)
    {
        // verify signature of this certificate (self-signed allowed)
        if (!bVerify || ((iResult = _ProtoSSLVerifyCertificate(NULL, pSecure, &Cert, TRUE)) == 0))
        {
            // add certificate to CA list
            iCount = _ProtoSSLAddCACert(&Cert, bVerify, iMemGroup, pMemGroupUserData);
        }
    }

    // if CA was PEM encoded and there is extra data, check for more CAs
    while ((iResult == 0) && (iCertSize > 0) && (_CertificateFindData(pCertData, iCertSize, &pCertBeg, &pCertEnd, &uCertType) != 0))
    {
        // remember remaining data for possible further parsing
        iCertSize -=  pCertEnd-pCertData;
        pCertData = pCertEnd;

        // cert must be base64 encoded
        if (((iResult = Base64Decode2((int32_t)(pCertEnd-pCertBeg), (const char *)pCertBeg, NULL)) <= 0) || (iResult > _iMaxCertSize))
        {
            break;
        }
        Base64Decode2((int32_t)(pCertEnd-pCertBeg), (const char *)pCertBeg, (char *)pCertBuffer);

        // parse the x.509 certificate onto stack
        if ((iResult = _AsnParseCertificate(&Cert, pCertBuffer, iResult)) < 0)
        {
            continue;
        }

        // verify signature of this certificate (self-signed allowed)
        if (bVerify && ((iResult = _ProtoSSLVerifyCertificate(NULL, pSecure, &Cert, TRUE)) < 0))
        {
            continue;
        }

        // add certificate to CA list
        iCount += _ProtoSSLAddCACert(&Cert, bVerify, iMemGroup, pMemGroupUserData);
    }

    // cleanup temp secure state
    DirtyMemFree(pSecure, PROTOSSL_MEMID, iMemGroup, pMemGroupUserData);

    // cleanup temp allocation
    if (pCertBuffer != NULL)
    {
        DirtyMemFree(pCertBuffer, PROTOSSL_MEMID, iMemGroup, pMemGroupUserData);
    }

    NetPrintf(("protossl: SSL Perf (CA load) %dms\n", NetTickDiff(NetTick(), uTick)));
    return(iCount);
}

/*
    misc helpers
*/

/*F********************************************************************************/
/*!
    \Function _ProtoSSLSetSockOpt

    \Description
        Set socket options

    \Input  *pState     - module state

    \Version 09/12/2012 (jbrookes) Refactored from ProtoSSLBind() and ProtoSSLConnect()
*/
/********************************************************************************F*/
static void _ProtoSSLSetSockOpt(ProtoSSLRefT *pState)
{
    // set debug level
    SocketControl(pState->pSock, 'spam', pState->iVerbose, NULL, NULL);

    // set recv/send buffer size?
    if (pState->iRecvBufSize != 0)
    {
        SocketControl(pState->pSock, 'rbuf', pState->iRecvBufSize, NULL, NULL);
    }
    if (pState->iSendBufSize != 0)
    {
        SocketControl(pState->pSock, 'sbuf', pState->iSendBufSize, NULL, NULL);
    }

    // set max send/recv rate?
    if (pState->iMaxRecvRate != 0)
    {
        SocketControl(pState->pSock, 'maxr', pState->iMaxRecvRate, NULL, NULL);
    }
    if (pState->iMaxSendRate != 0)
    {
        SocketControl(pState->pSock, 'maxs', pState->iMaxSendRate, NULL, NULL);
    }

    // set keep-alive options?
    if (pState->bKeepAlive != 0)
    {
        SocketControl(pState->pSock, 'keep', pState->bKeepAlive, &pState->uKeepAliveTime, &pState->uKeepAliveTime);
    }

    // set nodelay?
    if (pState->bNoDelay)
    {
        SocketControl(pState->pSock, 'ndly', TRUE, NULL, NULL);
    }

    // set reuseaddr
    if (pState->bReuseAddr)
    {
        SocketControl(pState->pSock, 'radr', TRUE, NULL, NULL);
    }

    // if async receive is enabled set it on the socket and adjust the packet queue to fit a large packet (SSL_RCVMAX_PACKET)
    if (pState->bAsyncRecv)
    {
        SocketControl(pState->pSock, 'arcv', TRUE, NULL, NULL);
        SocketControl(pState->pSock, 'pque', (SSL_RCVMAX_PACKET / SOCKET_MAXUDPRECV) + 1, NULL, NULL);
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLCacheLocalAddress

    \Description
        Cache value of our local address being used, some connections are very short
        lived making it otherwise difficult to read the local addr info reliably

    \Input *pState      - reference pointer

    \Version 09/13/2017 (cvienneau) used in Qos2.0
*/
/********************************************************************************F*/
static void _ProtoSSLCacheLocalAddress(ProtoSSLRefT *pState)
{
    if (pState->pSock != NULL)
    {
        if (SocketInfo(pState->pSock, 'bind', 0, &pState->LocalAddr, sizeof(pState->LocalAddr)) != 0)
        {
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: _ProtoSSLCacheLocalAddress failed to read local address info from 'bind'.\n"));
        }
    }
    else
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: _ProtoSSLCacheLocalAddress socket is NULL.\n"));
    }
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoSSLStartup

    \Description
        Start up ProtoSSL.  Used to create global state shared across SSL refs.

    \Output
        int32_t         - negative=failure, else success

    \Version 09/14/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoSSLStartup(void)
{
    ProtoSSLStateT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // make sure we haven't already been called
    if (_ProtoSSL_pState != NULL)
    {
        NetPrintf(("protossl: global state already allocated\n"));
        return(-1);
    }

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pState = DirtyMemAlloc(sizeof(*pState), PROTOSSL_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protossl: could not allocate global state\n"));
        return(-1);
    }
    ds_memclr(pState, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    NetCritInit(&pState->StateCrit, "ProtoSSL Global State");

    // initalize cyptrand module
    if (CryptRandInit() != 0)
    {
        NetCritKill(&pState->StateCrit);
        DirtyMemFree(pState, PROTOSSL_MEMID, iMemGroup, pMemGroupUserData);
        return(-1);
    }

    // set global defaults
    pState->iDfltCiph = PROTOSSL_CIPHER_ALL;
    pState->iDfltVers = SSL3_VERSION;
    pState->iDfltMinVers = SSL3_TLS1_0;
    pState->iDfltCurves = PROTOSSL_CURVE_ALL;

    // save state ref
    _ProtoSSL_pState = pState;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLShutdown

    \Description
        Shut down ProtoSSL.  Cleans up global state.

    \Version 09/14/2012 (jbrookes)
*/
/********************************************************************************F*/
void ProtoSSLShutdown(void)
{
    ProtoSSLStateT *pState = _ProtoSSL_pState;

    // make sure we haven't already been called
    if (pState == NULL)
    {
        NetPrintf(("protossl: global state not allocated\n"));
        return;
    }

    // deallocate stored CAs
    ProtoSSLClrCACerts();

    // shutdown cyptrand module
    CryptRandShutdown();

    // shut down, deallocate resources
    NetCritKill(&pState->StateCrit);
    DirtyMemFree(pState, PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);

    // clear state pointer
    _ProtoSSL_pState = NULL;
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLCreate

    \Description
        Allocate an SSL connection and prepare for use

    \Output
        ProtoSSLRefT * - module state; NULL=failure

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************F*/
ProtoSSLRefT *ProtoSSLCreate(void)
{
    ProtoSSLRefT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pState = DirtyMemAlloc(sizeof(*pState), PROTOSSL_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protossl: could not allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pState, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;

    // set defaults
    pState->iLastSocketError = SOCKERR_NONE;
    pState->iVerbose = SSL_VERBOSE_DEFAULT;
    pState->bSessionResumeEnabled = TRUE;
    pState->iCurveDflt = SSL3_CURVE_DEFAULT;
    pState->uHelloExtn = PROTOSSL_HELLOEXTN_DEFAULT;

    // set defaults with global overrides
    if (_ProtoSSL_pState != NULL)
    {
        pState->uSslVersion = _ProtoSSL_pState->iDfltVers;
        pState->uSslVersionMin = _ProtoSSL_pState->iDfltMinVers;
        pState->uEnabledCiphers = _ProtoSSL_pState->iDfltCiph;
        pState->uEnabledCurves = _ProtoSSL_pState->iDfltCurves;
    }
    else
    {
        pState->uSslVersion = SSL3_VERSION;
        pState->uSslVersionMin = SSL3_TLS1_0;
        pState->uEnabledCiphers = PROTOSSL_CIPHER_ALL;
        pState->uEnabledCurves = PROTOSSL_CURVE_ALL;
    }

    // init secure state critical section
    NetCritInit(&pState->SecureCrit, "ProtoSSL Secure State");

    // return module state
    return(pState);
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLReset

    \Description
        Reset connection back to unconnected state (will disconnect from server).

    \Input *pState      - module state reference

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************F*/
void ProtoSSLReset(ProtoSSLRefT *pState)
{
    // reset to unsecure mode
    _ProtoSSLResetState(pState, 0);
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLDestroy

    \Description
        Destroy the module and release its state. Will disconnect from the
        server if required.

    \Input *pState      - module state reference

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************F*/
void ProtoSSLDestroy(ProtoSSLRefT *pState)
{
    // reset to unsecure mode (free all secure resources)
    _ProtoSSLResetState(pState, 0);
    // free certificate, if allocated
    if (pState->pCertificate != NULL)
    {
        DirtyMemFree(pState->pCertificate, PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }
    // free private key, if allocated
    if (pState->pPrivateKey != NULL)
    {
        DirtyMemFree(pState->pPrivateKey, PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }
    // kill critical section
    NetCritKill(&pState->SecureCrit);
    // free remaining state
    DirtyMemFree(pState, PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLAccept

    \Description
        Accept an incoming connection.

    \Input *pState      - module state reference
    \Input iSecure      - flag indicating use of secure connection: 1=secure, 0=unsecure
    \Input *pAddr       - where the client's address should be written
    \Input *pAddrlen    - the length of the client's address space

    \Output
        ProtoSSLRefT *  - accepted connection or NULL if not available

    \Version 10/27/2013 (jbrookes)
*/
/********************************************************************************F*/
ProtoSSLRefT *ProtoSSLAccept(ProtoSSLRefT *pState, int32_t iSecure, struct sockaddr *pAddr, int32_t *pAddrlen)
{
    ProtoSSLRefT *pClient;
    SocketT *pSocket;

    // check for connect
    pSocket = SocketAccept(pState->pSock, pAddr, pAddrlen);
    if (pSocket == NULL)
    {
        return(NULL);
    }

    // we have an incoming connection attempt, so create an ssl object for it
    DirtyMemGroupEnter(pState->iMemGroup, pState->pMemGroupUserData);
    pClient = ProtoSSLCreate();
    DirtyMemGroupLeave();
    if (pClient == NULL)
    {
        SocketClose(pSocket);
        return(NULL);
    }

    // set up new ssl object with the socket we just accepted
    if (_ProtoSSLResetState(pClient, iSecure) != SOCKERR_NONE)
    {
        ProtoSSLDestroy(pClient);
        return(NULL);
    }
    pClient->pSock = pSocket;
    ds_memcpy(&pClient->PeerAddr, pAddr, *pAddrlen);

    // update socket status
    SocketInfo(pClient->pSock, 'stat', 0, NULL, 0);

    // set client state
    pClient->iState = (pClient->pSecure ? ST3_RECV_HELLO : ST_UNSECURE);
    pClient->iClosed = 0;
    pClient->bServer = TRUE;
    return(pClient);
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLBind

    \Description
        Create a socket bound to the given address.

    \Input *pState  - module state reference
    \Input *pAddr   - the IPv4 address
    \Input iAddrlen - the size of the IPv4 address.

    \Output
        int32_t     - SOCKERR_xxx (zero=success, negative=failure)

    \Version 03/03/2004 (sbevan)
*/
/********************************************************************************F*/
int32_t ProtoSSLBind(ProtoSSLRefT *pState, const struct sockaddr *pAddr, int32_t iAddrlen)
{
    // if we had a socket, get last error from it and close it
    if (pState->pSock != NULL)
    {
        pState->iLastSocketError = SocketInfo(pState->pSock, 'serr', 0, NULL, 0);
        SocketClose(pState->pSock);
    }

    // create the socket
    if ((pState->pSock = SocketOpen(AF_INET, SOCK_STREAM, 0)) == NULL)
    {
        return(SOCKERR_OTHER);
    }

    // set socket options
    _ProtoSSLSetSockOpt(pState);

    // do the bind, return result
    return(SocketBind(pState->pSock, pAddr, iAddrlen));
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLConnect

    \Description
        Make a secure connection to an SSL server.

    \Input *pState  - module state reference
    \Input iSecure  - flag indicating use of secure connection (1=secure, 0=unsecure)
    \Input *pAddr   - textual form of address (1.2.3.4 or www.ea.com)
    \Input uAddr    - the IP address of the server (if not in textual form)
    \Input iPort    - the TCP port of the server (if not in textual form)

    \Output
        int32_t     - SOCKERR_xxx (zero=success, negative=failure)

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************F*/
int32_t ProtoSSLConnect(ProtoSSLRefT *pState, int32_t iSecure, const char *pAddr, uint32_t uAddr, int32_t iPort)
{
    int32_t iIndex;
    int32_t iError;

    // reset connection state
    iError = _ProtoSSLResetState(pState, iSecure);
    if (iError != SOCKERR_NONE)
    {
        return(iError);
    }

    // only allow secure connection if dirtycert service name has been set
    if ((iSecure != 0) && (DirtyCertStatus('snam', NULL, 0) < 0))
    {
        NetPrintf(("protossl: ************************************************************************************\n"));
        NetPrintf(("protossl: ProtoSSLConnect() requires a valid DirtyCert service name in the format\n"));
        NetPrintf(("protossl: \"game-year-platform\", set when calling NetConnStartup() by using the -servicename\n"));
        NetPrintf(("protossl: argument, to be set before SSL use is allowed.  If a service name doesn't include\n"));
        NetPrintf(("protossl: dashes it is assumed to simply be the 'game' identifier, in which case DirtySDK will\n"));
        NetPrintf(("protossl: fill in the year and platform.  For titles using BlazeSDK, the service name specified\n"));
        NetPrintf(("protossl: must match the BlazeSDK service name, therefore the full name should be specified.\n"));
        NetPrintf(("protossl: *** PLEASE SET A UNIQUE AND MEANINGFUL SERVICE NAME, EVEN FOR TOOL OR SAMPLE USE ***\n"));
        return(SOCKERR_INVALID);
    }

    // allocate the socket
    if ((pState->pSock = SocketOpen(AF_INET, SOCK_STREAM, 0)) == NULL)
    {
        return(SOCKERR_NORSRC);
    }

    // set socket options
    _ProtoSSLSetSockOpt(pState);

    // init peer structure
    SockaddrInit(&pState->PeerAddr, AF_INET);

    // clear previous cert info, if any
    pState->bCertInfoSet = FALSE;
    ds_memclr(&pState->CertInfo, sizeof(pState->CertInfo));

    // handle default address case
    if (pAddr == NULL)
    {
        pAddr = "";
    }

    // parse the address string
    for (iIndex = 0; (pAddr[iIndex] != 0) && (pAddr[iIndex] != ':') && ((unsigned)iIndex < sizeof(pState->strHost)-1); ++iIndex)
    {
        // copy over to host
        pState->strHost[iIndex] = pAddr[iIndex];
    }
    pState->strHost[iIndex] = 0;

    // attempt to set host address
    SockaddrInSetAddrText(&pState->PeerAddr, pState->strHost);
    if (SockaddrInGetAddr(&pState->PeerAddr) == 0)
    {
        SockaddrInSetAddr(&pState->PeerAddr, uAddr);
    }

    // attempt to set peer address
    if (pAddr[iIndex] == ':')
    {
        SockaddrInSetPort(&pState->PeerAddr, atoi(pAddr+iIndex+1));
    }
    else
    {
        SockaddrInSetPort(&pState->PeerAddr, iPort);
    }

    // see if we need to start DNS request
    if (SockaddrInGetAddr(&pState->PeerAddr) == 0)
    {
        // do dns lookup prior to connect
        pState->pHost = SocketLookup(pState->strHost, 30*1000);
        pState->iState = ST_ADDR;
    }
    else
    {
        // set to connect state
        pState->iState = ST_CONN;
    }

    // return error code
    return(SOCKERR_NONE);
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLListen

    \Description
        Start listening for an incoming connection.

    \Input *pState  - module state reference
    \Input iBacklog - number of pending connections allowed

    \Output
        int32_t     - SOCKERR_xxx (zero=success, negative=failure)

    \Version 03/03/2004 (sbevan)
*/
/********************************************************************************F*/
int32_t ProtoSSLListen(ProtoSSLRefT *pState, int32_t iBacklog)
{
    return(SocketListen(pState->pSock, iBacklog));
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLDisconnect

    \Description
        Disconnect from the server

    \Input *pState  - module state reference

    \Output
        int32_t     - SOCKERR_xxx (zero=success, negative=failure)

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************F*/
int32_t ProtoSSLDisconnect(ProtoSSLRefT *pState)
{
    if (pState->pSock)
    {
        // send a close_notify alert as per http://tools.ietf.org/html/rfc5246#section-7.2.1
        if ((pState->pSecure != NULL) && (pState->iState == ST3_SECURE))
        {
            // send the alert
            _ProtoSSLSendAlert(pState, SSL3_ALERT_LEVEL_WARNING, SSL3_ALERT_DESC_CLOSE_NOTIFY);
        }

        /* if in server mode, just close the write side.  on the client we do an immediate hard
           close of the socket as the calling application that is polling to drive our operation
           may stop updating us once they have received all of the data, which would prevent us
           from closing the socket */
        if (pState->bServer)
        {
            SocketShutdown(pState->pSock, SOCK_NOSEND);
        }
        else
        {
            SocketClose(pState->pSock);
            pState->pSock = NULL;
        }
    }

    pState->iState = ST_IDLE;
    pState->iClosed = 1;

    // done with dirtycert
    if (pState->iCARequestId > 0)
    {
        DirtyCertCARequestFree(pState->iCARequestId);
    }
    pState->iCARequestId = 0;
    return(SOCKERR_NONE);
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLUpdate

    \Description
        Give time to module to do its thing (should be called
        periodically to allow module to perform work). Calling
        once per 100ms or so should be enough.

        This is actually a collection of state functions plus
        the overall update function. They are kept together for
        ease of reading.

    \Input *pState - module state reference

    \Version 11/10/2005 gschaefer
*/
/********************************************************************************F*/
void ProtoSSLUpdate(ProtoSSLRefT *pState)
{
    int32_t iXfer;
    int32_t iResult;
    SecureStateT *pSecure = pState->pSecure;

    // resolve the address
    if (pState->iState == ST_ADDR)
    {
        // check for completion
        if (pState->pHost->Done(pState->pHost))
        {
            pState->iState = (pState->pHost->addr != 0) ? ST_CONN : ST_FAIL_DNS;
            SockaddrInSetAddr(&pState->PeerAddr, pState->pHost->addr);
            // free the record
            pState->pHost->Free(pState->pHost);
            pState->pHost = NULL;
        }
    }

    // see if we should start a connection
    if (pState->iState == ST_CONN)
    {
        // start the connection attempt
        if ((iResult = SocketConnect(pState->pSock, &pState->PeerAddr, sizeof pState->PeerAddr)) == SOCKERR_NONE)
        {
            pState->iState = ST_WAIT_CONN;
        }
        else
        {
            pState->iState = ST_FAIL_CONN;
            pState->iClosed = 1;
        }
    }

    // wait for connection
    if (pState->iState == ST_WAIT_CONN)
    {
        iResult = SocketInfo(pState->pSock, 'stat', 0, NULL, 0);
        if (iResult > 0)
        {
            pState->iState = pSecure ? ST3_SEND_HELLO : ST_UNSECURE;
            pState->iClosed = 0;
            _ProtoSSLCacheLocalAddress(pState);
        }
        if (iResult < 0)
        {
            pState->iState = ST_FAIL_CONN;
            pState->iClosed = 1;
        }
    }

    // handle secure i/o (non-secure is done immediately in ProtoSSLSend/ProtoSSLRecv)
    while ((pState->pSock != NULL) && (pState->iState >= ST3_SEND_HELLO) && (pState->iState <= ST3_SECURE))
    {
        // get access to secure state
        NetCritEnter(&pState->SecureCrit);

        // update async processing, if any
        if (_ProtoSSLUpdateAsync(pState, pSecure))
        {
            NetCritLeave(&pState->SecureCrit);
            break;
        }

        // update send processing
        iXfer = _ProtoSSLUpdateSend(pState, pSecure);

        // update recv processing
        iXfer += _ProtoSSLUpdateRecv(pState, pSecure);

        // release access to secure state
        NetCritLeave(&pState->SecureCrit);

        // break out of loop if no i/o activity
        if (iXfer == 0)
        {
            break;
        }
    }

    // wait for CA cert (this comes last intentionally)
    if (pState->iState == ST_WAIT_CA)
    {
        // acquire secure state crit section to guard dirtycert access
        NetCritEnter(&pState->SecureCrit);

        // update CA request processing
        _ProtoSSLUpdateCARequest(pState);

        // release secure state crit section
        NetCritLeave(&pState->SecureCrit);
    }
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLSend

    \Description
        Send data to the server over secure TCP connection (actually, whether the
        connection is secure or not is determined by the secure flag passed during
        the SSLConnect call).

    \Input *pState  - module state reference
    \Input *pBuffer - data to send
    \Input iLength  - length of data (if negative, input data is assumed to be null-terminated string)

    \Output
        int32_t     - negative=error, otherwise number of bytes sent

    \Version 06/03/2002 (gschaefer)
*/
/********************************************************************************F*/
int32_t ProtoSSLSend(ProtoSSLRefT *pState, const char *pBuffer, int32_t iLength)
{
    int32_t iResult = SOCKERR_CLOSED;
    SecureStateT *pSecure = pState->pSecure;

    // allow easy string sends
    if (iLength < 0)
    {
        iLength = (int32_t)strlen(pBuffer);
    }
    // guard against zero-length sends, which can result in an invalid send condition with some stream (e.g. RC4) ciphers
    if (iLength == 0)
    {
        return(0);
    }

    // make sure connection established
    if (pState->iState == ST3_SECURE)
    {
        iResult = 0;

        // get access to secure state
        NetCritEnter(&pState->SecureCrit);

        // make sure buffer is empty
        if (pSecure->iSendSize == 0)
        {
            // limit send length
            if (iLength > SSL_SNDLIM_PACKET)
            {
                iLength = SSL_SNDLIM_PACKET;
            }

            // setup packet for send
            if (_ProtoSSLSendPacket(pState, SSL3_REC_APPLICATION, NULL, 0, pBuffer, iLength) == 0)
            {
                iResult = iLength;
                // try and send now
                ProtoSSLUpdate(pState);
            }
        }

        // release access to secure state
        NetCritLeave(&pState->SecureCrit);
    }

    // handle unsecure sends
    if (pState->iState == ST_UNSECURE)
    {
        iResult = SocketSend(pState->pSock, pBuffer, iLength, 0);
    }

    // return the result
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLRecv

    \Description
        Receive data from the server

    \Input *pState  - module state reference
    \Input *pBuffer - receiver data
    \Input iLength  - maximum buffer length

    \Output
        int32_t     - negative=error, zero=nothing available, positive=bytes received

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************F*/
int32_t ProtoSSLRecv(ProtoSSLRefT *pState, char *pBuffer, int32_t iLength)
{
    SecureStateT *pSecure = pState->pSecure;
    int32_t iResult = 0;

    // make sure in right state
    if (pState->iState == ST3_SECURE)
    {
        // get access to secure state
        NetCritEnter(&pState->SecureCrit);

        // check for more data if no packet pending
        if ((pSecure->iRecvProg == 0) || (pSecure->iRecvProg != pSecure->iRecvSize))
        {
            ProtoSSLUpdate(pState);
        }

        // check for end of data
        if (((pSecure->iRecvSize < SSL_MIN_PACKET) || (pSecure->iRecvProg < pSecure->iRecvSize)) && (pState->iClosed))
        {
            iResult = SOCKERR_CLOSED;
        }
        // see if data pending
        else if ((pSecure->iRecvProg == pSecure->iRecvSize) && (pSecure->iRecvBase < pSecure->iRecvSize) &&
            (pSecure->RecvHead[0] == SSL3_REC_APPLICATION))
        {
            iResult = pSecure->iRecvSize-pSecure->iRecvBase;
            // only return what user can store
            if (iResult > iLength)
            {
                iResult = iLength;
            }
            // return the data
            ds_memcpy(pBuffer, pSecure->RecvData+pSecure->iRecvBase, iResult);
            pSecure->iRecvBase += iResult;

            // see if we can grab a new packet
            if (pSecure->iRecvBase >= pSecure->iRecvSize)
            {
                _ProtoSSLRecvReset(pSecure);
            }
        }

        // release access to secure state
        NetCritLeave(&pState->SecureCrit);
    }

    // handle unsecure receive
    if (pState->iState == ST_UNSECURE)
    {
        iResult = SocketRecv(pState->pSock, pBuffer, iLength, 0);
    }

    // return error if in failure state
    if (pState->iState >= ST_FAIL)
    {
        iResult = -1;
    }

    // terminate buffer if there is room
    if ((iResult > 0) && (iResult < iLength))
    {
        pBuffer[iResult] = 0;
    }

    // return the data size
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLStat

    \Description
        Return the current module status (according to selector)

    \Input *pState  - module state reference
    \Input iSelect  - status selector ('conn'=return "am i connected" flag)
    \Input pBuffer  - buffer pointer
    \Input iLength  - buffer size

    \Output
        int32_t     - negative=error, zero=false, positive=true

    \Notes
        Selectors are:

        \verbatim
            SELECTOR    DESCRIPTION
            'addr'      Address of peer we are connecting/connected to
            'alpn'      Get the negotiated protocol using the ALPN extension from the secure state
            'alrt'      Return alert status 0=no alert, 1=recv alert, 2=sent alert; alert desc copied to pBuffer
            'cert'      Return SSL cert info (valid after 'fail')
            'cfip'      TRUE/FALSE indication if a CA fetch is in progress
            'ciph'      Cipher suite negotiated (string name in output buffer)
            'crpt'      Returns whether we are performing any crypto operations at this time
            'fail'      Return PROTOSSL_ERROR_* or zero if no error
            'hres'      Return hResult containing either the socket error or ssl error
            'htim'      Return timing of most recent SSL handshake in milliseconds
            'ladd'      cached local client address used for the last connection; copy into pBuffer, buffer must be at least sizeof(struct sockaddr)
            'maxr'      Return max recv rate (0=uncapped)
            'maxs'      Return max send rate (0=uncapped)
            'recv'      Return number of bytes in recv buffer (secure only)
            'resu'      Returns whether session was resumed or not
            'rsao'      [DEPRECATED] - same as 'crpt'
            'send'      Return number of bytes in send buffer (secure only)
            'serr'      Return socket error
            'salg'      Signature algorithm negotiated (string name in output buffer); not available on resume
            'sock'      Copy SocketT ref to output buffer
            'stat'      Return like SocketInfo('stat')
            'vers'      Return current SSL version
        \endverbatim

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************F*/
int32_t ProtoSSLStat(ProtoSSLRefT *pState, int32_t iSelect, void *pBuffer, int32_t iLength)
{
    int32_t iResult = -1;

    // pass-through to SocketInfo(NULL,...)
    if (pState == NULL)
    {
        return(SocketInfo(NULL, iSelect, 0, pBuffer, iLength));
    }

    // return address of peer we are trying to connect to
    if (iSelect == 'addr')
    {
        if ((pBuffer != NULL) && (iLength == sizeof(pState->PeerAddr)))
        {
            ds_memcpy(pBuffer, &pState->PeerAddr, iLength);
        }
        return(SockaddrInGetAddr(&pState->PeerAddr));
    }
    if ((iSelect == 'alpn') && (pState->pSecure != NULL))
    {
        if (pBuffer != NULL)
        {
            ds_strnzcpy(pBuffer, pState->pSecure->strAlpnProtocol, iLength);
        }

        return(0);
    }
    // return most recent alert if any
    if (iSelect == 'alrt')
    {
        if ((pBuffer != NULL) && (iLength == sizeof(ProtoSSLAlertDescT)))
        {
            ProtoSSLAlertDescT Alert;
            if ((iResult = _ProtoSSLGetAlert(pState, &Alert, pState->uAlertLevel, pState->uAlertValue)) != 0)
            {
                ds_memcpy(pBuffer, &Alert, sizeof(Alert));
                iResult = pState->bAlertSent ? 2 : 1;
            }
        }
        return(iResult);
    }
    // return certificate info (valid after 'fail' response)
    if ((iSelect == 'cert') && (pBuffer != NULL) && (iLength == sizeof(pState->CertInfo)))
    {
        ds_memcpy(pBuffer, &pState->CertInfo, sizeof(pState->CertInfo));
        return(0);
    }
    // return if a CA fetch request is in progress
    if (iSelect == 'cfip')
    {
        return((pState->iState == ST_WAIT_CA) ? 1 : 0);
    }
    // return current cipher suite
    if ((iSelect == 'ciph') && (pState->pSecure != NULL) && (pState->pSecure->pCipher != NULL))
    {
        if (pBuffer != NULL)
        {
            ds_strnzcpy(pBuffer, pState->pSecure->pCipher->strName, iLength);
        }
        return(pState->pSecure->pCipher->uId);
    }
    // return whether we are performing any crypto operations at this time
    if ((iSelect == 'crpt') || (iSelect == 'rsao'))
    {
        switch (pState->iState)
        {
            case ST3_SEND_HELLO:
            case ST3_SEND_EXTN:
            case ST3_SEND_KEY:
            case ST3_SEND_VERIFY:
            case ST3_SEND_CHANGE:
            case ST3_RECV_CHANGE:
            case ST3_PROC_ASYNC:
                iResult = TRUE;
                break;
            default:
                iResult = FALSE;
                break;
        }
        return(iResult);
    }
    // return timing of most recent SSL handshake in milliseconds
    if ((iSelect == 'htim') && (pState->pSecure != NULL))
    {
        return(pState->pSecure->uTimer);
    }
    // cached local client address used for the last connection; copy into pBuffer, buffer must be at least sizeof(struct sockaddr)
    if (iSelect == 'ladd')
    {
        if ((pBuffer != NULL) && (iLength >= (int32_t)sizeof(pState->LocalAddr)))
        {
            ds_memcpy(pBuffer, &pState->LocalAddr, sizeof(pState->LocalAddr));
            return(0);
        }
        return(-1);
    }
    // return configured max receive rate
    if (iSelect == 'maxr')
    {
        return(pState->iMaxRecvRate);
    }
    // return configured max send rate
    if (iSelect == 'maxs')
    {
        return(pState->iMaxSendRate);
    }
    // return number of bytes in recv buffer (useful only when connection type is secure)
    if (iSelect == 'recv')
    {
        return((pState->pSecure != NULL) ? pState->pSecure->iRecvSize-pState->pSecure->iRecvProg : 0);
    }
    // return whether session was resumed or not
    if ((iSelect == 'resu') && (pState->pSecure != NULL))
    {
        return(pState->pSecure->bSessionResume);
    }
    // return current signature algorithm if available
    if ((iSelect == 'salg') && (pState->pSecure != NULL))
    {
        if ((pBuffer != NULL) && (pState->pSecure->Cert.iSigType >= ASN_OBJ_RSA_PKCS_MD5))
        {
            ds_strnzcpy(pBuffer, _SSL3_strSignatureTypes[pState->pSecure->Cert.iSigType-ASN_OBJ_RSA_PKCS_MD5], iLength);
            return(1);
        }
    }
    // return number of bytes in send buffer (useful only when connection type is secure)
    if (iSelect == 'send')
    {
        return((pState->pSecure != NULL) ? pState->pSecure->iSendSize-pState->pSecure->iSendProg : 0);
    }
    // return last socket error
    if (iSelect == 'serr')
    {
        // pass through to socket module if we have a socket, else return cached last error
        return((pState->pSock != NULL) ? SocketInfo(pState->pSock, iSelect, 0, pBuffer, iLength) : pState->iLastSocketError);
    }
    // return socket ref
    if (iSelect == 'sock')
    {
        if ((pBuffer == NULL) || (iLength != sizeof(pState->pSock)))
        {
            return(-1);
        }
        ds_memcpy(pBuffer, &pState->pSock, sizeof(pState->pSock));
        return(0);
    }
    // return current ssl version for the connection
    if ((iSelect == 'vers') && (pState->pSecure != NULL))
    {
        if (pBuffer != NULL)
        {
            ds_strnzcpy(pBuffer, _SSL3_strVersionNames[pState->pSecure->uSslVersion&0xff], iLength);
        }
        return(pState->pSecure->uSslVersion);
    }
    // return failure code
    if (iSelect == 'fail')
    {
        if (pState->iState & ST_FAIL)
        {
            switch (pState->iState)
            {
                case ST_FAIL_DNS:
                    iResult = PROTOSSL_ERROR_DNS;
                    break;
                case ST_FAIL_CONN:
                    iResult = PROTOSSL_ERROR_CONN;
                    break;
                case ST_FAIL_CONN_SSL2:
                    iResult = PROTOSSL_ERROR_CONN_SSL2;
                    break;
                case ST_FAIL_CONN_NOTSSL:
                    iResult = PROTOSSL_ERROR_CONN_NOTSSL;
                    break;
                case ST_FAIL_CONN_MINVERS:
                    iResult = PROTOSSL_ERROR_CONN_MINVERS;
                    break;
                case ST_FAIL_CONN_MAXVERS:
                    iResult = PROTOSSL_ERROR_CONN_MAXVERS;
                    break;
                case ST_FAIL_CONN_NOCIPHER:
                    iResult = PROTOSSL_ERROR_CONN_NOCIPHER;
                    break;
                case ST_FAIL_CONN_NOCURVE:
                    iResult = PROTOSSL_ERROR_CONN_NOCURVE;
                    break;    
                case ST_FAIL_CERT_NONE:
                    iResult = PROTOSSL_ERROR_CERT_MISSING;
                    break;
                case ST_FAIL_CERT_INVALID:
                    iResult = PROTOSSL_ERROR_CERT_INVALID;
                    break;
                case ST_FAIL_CERT_HOST:
                    iResult = PROTOSSL_ERROR_CERT_HOST;
                    break;
                case ST_FAIL_CERT_NOTRUST:
                    iResult = PROTOSSL_ERROR_CERT_NOTRUST;
                    break;
                case ST_FAIL_CERT_BADDATE:
                    iResult = PROTOSSL_ERROR_CERT_BADDATE;
                    break;
                case ST_FAIL_CERT_REQUEST:
                    iResult = PROTOSSL_ERROR_CERT_REQUEST;
                    break;
                case ST_FAIL_SETUP:
                    iResult = PROTOSSL_ERROR_SETUP;
                    break;
                case ST_FAIL_SECURE:
                    iResult = PROTOSSL_ERROR_SECURE;
                    break;
                default:
                    iResult = PROTOSSL_ERROR_UNKNOWN;
                    break;
            }
        }
        else
        {
            iResult = 0;
        }
        return(iResult);
    }

    if (iSelect == 'hres')
    {
        uint32_t hResult;
        int32_t iSerr = ProtoSSLStat(pState, 'serr', NULL, 0);
        int32_t iFail = ProtoSSLStat(pState, 'fail', NULL, 0);

        if (iSerr < SOCKERR_CLOSED)
        {
            hResult = DirtyErrGetHResult(DIRTYAPI_SOCKET, iSerr, TRUE);
        }
        else if (iFail != 0)
        {
            hResult = DirtyErrGetHResult(DIRTYAPI_PROTO_SSL, iFail, TRUE);
        }
        else
        {
            hResult = DirtyErrGetHResult(DIRTYAPI_PROTO_SSL, 0, FALSE); //success hResult
        }
        return(hResult);
    }

    // only pass through if socket is valid
    if (pState->pSock != NULL)
    {
        // special processing for 'stat' selector
        if (iSelect == 'stat')
        {
            // if we're in a failure state, return error
            if (pState->iState >= ST_FAIL)
            {
               return(-1);
            }
            // don't check connected status until we are connected and done with secure negotiation (if secure)
            if (pState->iState < ST3_SECURE)
            {
               return(0);
            }
            // if we're connected (in ST_UNSECURE or ST3_SECURE state) fall through
        }

        // pass through request to the socket module
        iResult = SocketInfo(pState->pSock, iSelect, 0, pBuffer, iLength);
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLControl

    \Description
        ProtoSSL control function.  Different selectors control different behaviors.

    \Input *pState  - module state reference
    \Input iSelect  - control selector
    \Input iValue   - selector specific
    \Input iValue2  - selector specific
    \Input *pValue  - selector specific

    \Output
        int32_t     - selector specific

    \Notes
        Selectors are:

        \verbatim
            SELECTOR    DESCRIPTION
            'alpn'      Set the ALPN protocols (using IANA registerd named) as a comma delimited list
            'arcv'      Set async receive on the ssl socket
            'ccrt'      Set client certificate level (0=disabled, 1=requested, 2=required)
            'ciph'      Set enabled/disabled ciphers (PROTOSSL_CIPHER_*)
            'crvd'      Set default curve (-1=no default; else [0,SSL3_EC_MAX]
            'curv'      Set enabled/disabled curves (PROTOSS_CURVE_*)
            'extn'      Set enabled ClientHello extensions (PROTOSSL_HELLOEXTN_*), (default=0=disabled)
            'gcph'      Set global cipher default
            'gcrv'      Set global curve default
            'gvrs'      Set global version default
            'gvmn'      Set global version min default
            'host'      Set remote host
            'hreq'      Send HelloRequest (server only)
            'maxr'      Set max recv rate (0=uncapped, default)
            'maxs'      Set max send rate (0=uncapped, default)
            'ncrt'      Disable client certificate validation
            'rbuf'      Set socket recv buffer size (must be called before Connect())
            'resu'      Set whether session resume is enabled or disabled (default=1=enabled)
            'sbuf'      Set socket send buffer size (must be called before Connect())
            'scrt'      Set certificate (pValue=cert, iValue=len)
            'snod'      Set whether TCP_NODELAY option is enabled or disabled on the socket (must be called before Connect())
            'secu'      Start secure negotiation on an established unsecure connection
            'skey'      Set private key (pValue=key, iValue=len)
            'skep'      Set socket keep-alive settings (iValue=enable/disable, iValue2=keep-alive time/interval)
            'spam'      Set debug logging level (default=1)
            'vers'      Set client-requested SSL version (default=0x302, TLS1.1)
            'vmin'      Set minimum SSL version application will accept
        \endverbatim

    \Version 01/27/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoSSLControl(ProtoSSLRefT *pState, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue)
{
    int32_t iResult = -1;

    if (iSelect == 'alpn')
    {
        uint16_t uProtocol, uAlpnExtensionLength;
        const char *pSrc = (const char *)pValue;
        if (pSrc == NULL)
        {
            NetPrintf(("protossl: invalid ALPN extension protocol list provided\n"));
            return(-1);
        }
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: setting the ALPN extension protocols using %s\n", (const char *)pValue));
        ds_memclr(pState->aAlpnProtocols, sizeof(pState->aAlpnProtocols));

        for (uProtocol = 0, uAlpnExtensionLength = 0; uProtocol < SSL_ALPN_MAX_PROTOCOLS; uProtocol += 1)
        {
            AlpnProtocolT *pProtocol = &pState->aAlpnProtocols[uProtocol];
            if ((pProtocol->uLength = (uint8_t)ds_strsplit(pSrc, ',', pProtocol->strName, sizeof(pProtocol->strName), &pSrc)) == 0)
            {
                break;
            }

            uAlpnExtensionLength += pProtocol->uLength;
            uAlpnExtensionLength += sizeof(pProtocol->uLength);
        }
        pState->uNumAlpnProtocols = uProtocol;
        pState->uAlpnExtensionLength = uAlpnExtensionLength;

        return(0);
    }
    if (iSelect == 'arcv')
    {
        pState->bAsyncRecv = (uint8_t)iValue;
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: async recv set to %s\n", pState->bAsyncRecv ? "TRUE" : "FALSE"));
        return(0);
    }
    if (iSelect == 'ccrt')
    {
        NetPrintf(("protossl: setting client cert level to %d\n", iValue));
        pState->iClientCertLevel = iValue;
        return(0);
    }
    if (iSelect == 'ciph')
    {
        pState->uEnabledCiphers = (uint32_t)iValue;
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: enabled ciphers=%d\n", iValue));
        return(0);
    }
    if (iSelect == 'crvd')
    {
        // attempt to validate the curve is enabled
        int8_t iCurveDflt = (int8_t)DS_CLAMP(iValue, -1, SSL3_CURVE_MAX);
        if ((iCurveDflt == -1) || ((_SSL3_EllipticCurves[iCurveDflt].uId & pState->uEnabledCurves) != 0))
        {
            pState->iCurveDflt = iCurveDflt;
            NetPrintf(("protossl: choosing curve %d\n", pState->iCurveDflt));
            return(0);
        }
        else
        {
            NetPrintf(("protossl: selected curve %d is disabled and cannot be used as default\n", iCurveDflt));
            return(-1);
        }
    }
    if (iSelect == 'curv')
    {
        pState->uEnabledCurves = (uint32_t)iValue;
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: enabled curves=%u\n", pState->uEnabledCurves));
        return(0);
    }
    if (iSelect == 'extn')
    {
        pState->uHelloExtn = (uint8_t)iValue;
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: clienthello extensions set to 0x%02x\n", pState->uHelloExtn));
        return(0);
    }
    // handle global settings
    if (((iSelect == 'gcph') || (iSelect == 'gcrv') || (iSelect == 'gver') || (iSelect == 'gvmn')) && (_ProtoSSL_pState != NULL))
    {
        if (iSelect == 'gcph')
        {
            NetPrintf(("protossl: setting global default cipher mask to 0x%x\n", iValue));
            _ProtoSSL_pState->iDfltCiph = iValue;
            return(0);
        }
        if (iSelect == 'gcrv')
        {
            NetPrintf(("protossl: setting global default curve mask to 0x%08x\n", iValue));
            _ProtoSSL_pState->iDfltCurves = iValue;
            return(0);
        }
        if (iSelect == 'gver')
        {
            NetPrintf(("protossl: setting global default version to 0x%x\n", iValue));
            _ProtoSSL_pState->iDfltVers = iValue;
            return(0);
        }
        if (iSelect == 'gvmn')
        {
            NetPrintf(("protossl: setting global default min version to 0x%x\n", iValue));
            _ProtoSSL_pState->iDfltMinVers = iValue;
            return(0);
        }
    }
    if (iSelect == 'host')
    {
        NetPrintf(("protossl: setting host to '%s'\n", (const char *)pValue));
        ds_strnzcpy(pState->strHost, (const char *)pValue, sizeof(pState->strHost));
        return(0);
    }
    if ((iSelect == 'hreq') && (pState->bServer) && (pState->iState == ST3_SECURE))
    {
        NetPrintf(("protossl: sending Hello Request\n"));
        pState->iState = ST3_SEND_HELLO_REQUEST;
        return(0);
    }
    if (iSelect == 'maxr')
    {
        pState->iMaxRecvRate = iValue;
        if (pState->pSock != NULL)
        {
            SocketControl(pState->pSock, iSelect, iValue, NULL, NULL);
        }
        return(0);
    }
    if (iSelect == 'maxs')
    {
        pState->iMaxSendRate = iValue;
        if (pState->pSock != NULL)
        {
            SocketControl(pState->pSock, iSelect, iValue, NULL, NULL);
        }
        return(0);
    }
    if (iSelect == 'ncrt')
    {
        pState->bAllowAnyCert = (uint8_t)iValue;
        return(0);
    }
    if (iSelect == 'radr')
    {
        pState->bReuseAddr = TRUE;
        return(0);
    }
    if (iSelect == 'rbuf')
    {
        pState->iRecvBufSize = iValue;
        return(0);
    }
    if (iSelect == 'resu')
    {
        pState->bSessionResumeEnabled = iValue ? TRUE : FALSE;
        return(0);
    }
    if (iSelect == 'sbuf')
    {
        pState->iSendBufSize = iValue;
        return(0);
    }
    if (iSelect == 'scrt')
    {
        if (pState->pCertificate != NULL)
        {
            DirtyMemFree(pState->pCertificate, PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        }
        pState->pCertificate = _CertificateDecodePublic(pState, (uint8_t *)pValue, iValue);
        return(0);
    }
    if (iSelect == 'snod')
    {
        pState->bNoDelay = (uint8_t)iValue;
        return(0);
    }
    if (iSelect == 'skey')
    {
        if (pState->pPrivateKey != NULL)
        {
            DirtyMemFree(pState->pPrivateKey, PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            pState->iPrivateKeyLen = 0;
        }
        if ((pState->pPrivateKey = DirtyMemAlloc(iValue, PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) != NULL)
        {
            ds_memcpy(pState->pPrivateKey, pValue, iValue);
            pState->iPrivateKeyLen = iValue;
        }
        else
        {
            NetPrintf(("protossl: could not allocate memory for private key\n"));
        }
        return(0);
    }
    if (iSelect == 'skep')
    {
        pState->bKeepAlive = (uint8_t)iValue;
        pState->uKeepAliveTime = (uint32_t)iValue2;
        return(0);
    }
    if (iSelect == 'secu')
    {
        if (pState->iState != ST_UNSECURE)
        {
            NetPrintf(("protossl: cannot promote to a secure connection unless connected in unsecure state\n"));
            return(-1);
        }
        _ProtoSSLResetSecureState(pState, 1);
        pState->iState = ST3_SEND_HELLO;
        return(0);
    }
    if (iSelect == 'spam')
    {
        pState->iVerbose = (int8_t)iValue;
        return(0);
    }
    if (iSelect == 'vers')
    {
        uint32_t uSslVersion = DS_CLAMP(iValue, pState->uSslVersionMin, SSL3_VERSION_MAX);
        if (pState->uSslVersion != uSslVersion)
        {
            NetPrintf(("protossl: setting sslvers to %s (0x%04x)\n", _SSL3_strVersionNames[uSslVersion&0xff], uSslVersion));
            pState->uSslVersion = uSslVersion;
        }
        return(0);
    }
    if (iSelect == 'vmin')
    {
        uint32_t uSslVersionMin = DS_CLAMP(iValue, SSL3_VERSION_MIN, SSL3_VERSION_MAX);
        if (pState->uSslVersionMin != uSslVersionMin)
        {
            NetPrintf(("protossl: setting min sslvers to %s (0x%04x)\n", _SSL3_strVersionNames[uSslVersionMin&0xff], uSslVersionMin));
            pState->uSslVersionMin = uSslVersionMin;
            // make sure requested version is at least minimum version
            ProtoSSLControl(pState, 'vers', pState->uSslVersion, 0, NULL);
        }
        return(0);
    }
    // if we have a socket ref, pass unhandled selector through
    if (pState->pSock != NULL)
    {
        iResult = SocketControl(pState->pSock, iSelect, iValue, pValue, NULL);
    }
    else
    {
        NetPrintf(("protossl: ProtoSSLControl('%C') unhandled\n", iSelect));
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLSetCACert

    \Description
        Add one or more X.509 CA certificates to the trusted list. A
        certificate added will be available to all ProtoSSL instances for
        the lifetime of the application. This function can add one or more
        PEM certificates or a single DER certificate.

    \Input *pCertData   - pointer to certificate data (PEM or DER)
    \Input iCertSize    - size of certificate data

    \Output
        int32_t         - negative=error, positive=count of CAs added

    \Notes
        The certificate must be in .DER (binary) or .PEM (base64-encoded)
        format.

    \Version 01/13/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoSSLSetCACert(const uint8_t *pCertData, int32_t iCertSize)
{
    return(_ProtoSSLSetCACert(pCertData, iCertSize, TRUE));
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLSetCACert2

    \Description
        Add one or more X.509 CA certificates to the trusted list. A
        certificate added will be available to all ProtoSSL instances for
        the lifetime of the application. This function can add one or more
        PEM certificates or a single DER certificate.

        This version of the function does not validate the CA at load time.
        The X509 certificate data will be copied and retained until the CA
        is validated, either by use of ProtoSSLValidateAllCA() or by the CA
        being used to validate a certificate.

    \Input *pCertData   - pointer to certificate data (PEM or DER)
    \Input iCertSize    - size of certificate data

    \Output
        int32_t         - negative=error, positive=count of CAs added

    \Notes
        The certificate must be in .DER (binary) or .PEM (base64-encoded)
        format.

    \Version 04/21/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoSSLSetCACert2(const uint8_t *pCertData, int32_t iCertSize)
{
    return(_ProtoSSLSetCACert(pCertData, iCertSize, FALSE));
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLValidateAllCA

    \Description
        Validate all CA that have been added but not yet been validated.  Validation
        is a one-time process and disposes of the X509 certificate that is retained
        until validation.

    \Output
        int32_t         - zero on success; else the number of certs that could not be validated

    \Version 04/21/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoSSLValidateAllCA(void)
{
    ProtoSSLCACertT *pCACert;
    SecureStateT *pSecure;
    void *pMemGroupUserData;
    int32_t iInvalid, iMemGroup;

    // get memgroup settings for certificate blob
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate secure state for certificate validation
    if ((pSecure = _ProtoSSLAllocSecureState(iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protossl: could not allocate secure state for ca validation\n"));
        return(-1);
    }

    // validate all installed CA Certs that have not yet been validated
    for (pCACert = _ProtoSSL_CACerts, iInvalid = 0; pCACert != NULL; pCACert = pCACert->pNext)
    {
        // if the CA hasn't been verified already, do that now
        if (pCACert->pX509Cert != NULL)
        {
            if (_ProtoSSLVerifyCertificate(NULL, pSecure, pCACert->pX509Cert, TRUE) == 0)
            {
                #if DIRTYCODE_LOGGING
                char strIdentSubject[512], strIdentIssuer[512];
                NetPrintf(("protossl: ca (%s) validated by ca (%s)\n", _CertificateDebugFormatIdent(&pCACert->pX509Cert->Subject, strIdentSubject, sizeof(strIdentSubject)),
                    _CertificateDebugFormatIdent(&pCACert->pX509Cert->Issuer, strIdentIssuer, sizeof(strIdentIssuer))));
                #endif

                // cert successfully verified
                DirtyMemFree(pCACert->pX509Cert, PROTOSSL_MEMID, pCACert->iMemGroup, pCACert->pMemGroupUserData);
                pCACert->pX509Cert = NULL;
            }
            else
            {
                _CertificateDebugPrint(pCACert->pX509Cert, "ca could not be validated");
                iInvalid += 1;
            }
        }
    }

    // free secure state used for validation
    DirtyMemFree(pSecure, PROTOSSL_MEMID, iMemGroup, pMemGroupUserData);

    // return number of certs we could not validate
    return(iInvalid);
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLClrCACerts

    \Description
        Clears all dynamic CA certs from the list.

    \Version 01/14/2009 (jbrookes)
*/
/********************************************************************************F*/
void ProtoSSLClrCACerts(void)
{
    ProtoSSLCACertT *pCACert, *pCACert0=NULL;

    /*
     * This code makes the following assumptions:
     *    1) There is at least one static cert.
     *    2) All static certs come first, followed by all dynamic certs.
     */

    // scan for first dynamic certificate
    for (pCACert = _ProtoSSL_CACerts; (pCACert != NULL) && (pCACert->iMemGroup == 0); )
    {
        pCACert0 = pCACert;
        pCACert = pCACert->pNext;
    }
    // any dynamic certs?
    if ((pCACert != NULL) && (pCACert0 != NULL))
    {
        // null-terminate static list
        pCACert0->pNext = NULL;

        // delete dynamic certs
        for ( ; pCACert != NULL; )
        {
            pCACert0 = pCACert->pNext;
            if (pCACert->pX509Cert != NULL)
            {
                DirtyMemFree(pCACert->pX509Cert, PROTOSSL_MEMID, pCACert->iMemGroup, pCACert->pMemGroupUserData);
            }
            DirtyMemFree(pCACert, PROTOSSL_MEMID, pCACert->iMemGroup, pCACert->pMemGroupUserData);
            pCACert = pCACert0;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLPkcs1GenerateInit

    \Description
        Init for generating a PKCSv1.5 RSA signature 

    \Input *pPkcs1      - pkcs1 state
    \Input *pHashData   - hash of the data which was signed
    \Input iHashLen     - length of the hash
    \Input iHashType    - the hash which was used
    \Input iModSize     - size of the modulus
    \Input *pPrimeP     - prime p from the private key
    \Input *pPrimeQ     - prime q from the private key
    \Input *pExponentP  - exponent p from the private key
    \Input *pExponentQ  - exponent q from the private key
    \Input *pCoefficient- coefficient from the private key

    \Notes
        This function will block on the RSA operation.

    \Output
        int32_t         - zero=validation successful, negative=validation failed

    \Version 03/15/2020 (eesponda)
*/
/********************************************************************************F*/
int32_t ProtoSSLPkcs1GenerateInit(ProtoSSLPkcs1T *pPkcs1, const uint8_t *pHashData, int32_t iHashLen, int32_t iHashType, int32_t iModSize, const CryptBinaryObjT *pPrimeP, const CryptBinaryObjT *pPrimeQ, const CryptBinaryObjT *pExponentP, const CryptBinaryObjT *pExponentQ, const CryptBinaryObjT *pCoefficient)
{
    static uint8_t aSigData[SSL_SIG_MAX];
    int32_t iSigSize;

    // generate the signature data
    if ((iSigSize = _AsnWriteDigitalHashObject(aSigData, sizeof(aSigData), pHashData, iHashLen, (CryptHashTypeE)iHashType)) == 0)
    {
        return(-1);
    }
    #if DEBUG_RAW_DATA
    NetPrintMem(pHashData, iHashLen, "message digest");
    #endif

    // init the rsa module with the private key and signature information
    if (CryptRSAInit2(&pPkcs1->RSAContext, iModSize, pPrimeP, pPrimeQ, pExponentP, pExponentQ, pCoefficient))
    {
        return(-1);
    }
    CryptRSAInitPrivate(&pPkcs1->RSAContext, aSigData, iSigSize);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLPkcs1GenerateUpdate

    \Description
        Generate the PKCSv1.5 RSA signature 

    \Input *pPkcs1          - pkcs1 state
    \Input iNumIterations   - number of iterations to perform or zero to block until complete
    \Input *pSigData        - [out] signature data
    \Input iSigSize         - size of the signature

    \Output
        int32_t         - 1=operation pending, 0=operation complete

    \Version 03/15/2020 (eesponda)
*/
/********************************************************************************F*/
int32_t ProtoSSLPkcs1GenerateUpdate(ProtoSSLPkcs1T *pPkcs1, int32_t iNumIterations, uint8_t *pSigData, int32_t iSigSize)
{
    // perform the encryption rounds
    if (CryptRSAEncrypt(&pPkcs1->RSAContext, iNumIterations) > 0)
    {
        return(1);
    }
    NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (pkcs1 sig encrypt) %dms\n", pPkcs1->RSAContext.uCryptMsecs));

    #if DEBUG_RAW_DATA
    NetPrintMem(pPkcs1->RSAContext.EncryptBlock, pPkcs1->RSAContext.iKeyModSize, "encrypted signature");
    #endif

    // copy the signature
    ds_memcpy_s(pSigData, iSigSize, pPkcs1->RSAContext.EncryptBlock, pPkcs1->RSAContext.iKeyModSize);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoSSLPkcs1Verify

    \Description
        Verify the PKCSv1.5 RSA signature 

    \Input *pSigData    - signature data
    \Input iSigLen      - length of the signature
    \Input *pHashData   - hash of the data which was signed
    \Input iHashLen     - length of the hash
    \Input iHashType    - the hash which was used
    \Input *pMod        - public key modulus
    \Input iModSize     - size of the modulus
    \Input *pExp        - public key exponent
    \Input iExpSize     - size of the exponent

    \Notes
        This function will block on the RSA operation.

    \Output
        int32_t         - zero=validation successful, negative=validation failed

    \Version 02/14/2020 (eesponda)
*/
/********************************************************************************F*/
int32_t ProtoSSLPkcs1Verify(const uint8_t *pSigData, int32_t iSigLen, const uint8_t *pHashData, int32_t iHashLen, int32_t iHashType, const uint8_t *pMod, int32_t iModSize, const uint8_t *pExp, int32_t iExpSize)
{
    CryptRSAT RSA;
    int32_t iResult = -1;

    // init the rsa module with the public key and signature information
    if (CryptRSAInit(&RSA, pMod, iModSize, pExp, iExpSize))
    {
        return(-1);
    }
    CryptRSAInitSignature(&RSA, pSigData, iSigLen);

    // perform the encryption rounds
    CryptRSAEncrypt(&RSA, 0);
    NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (pkcs1 sig verify) %dms\n", RSA.uCryptMsecs));

    #if DEBUG_RAW_DATA
    NetPrintMem(RSA.EncryptBlock, RSA.iKeyModSize, "decrypted signature");
    NetPrintMem(pHashData, iHashLen, "message digest");
    #endif

    // extract hash data from signature block
    if ((pSigData = _Pkcs1VerifyEMSA(RSA.EncryptBlock, iSigLen, iHashLen, (CryptHashTypeE)iHashType)) != NULL)
    {
        // compare hash data with precalculated certificate body hash
        iResult = !memcmp(pHashData, pSigData, iHashLen) ? 0 : -1;
    }

    return(iResult);
}
