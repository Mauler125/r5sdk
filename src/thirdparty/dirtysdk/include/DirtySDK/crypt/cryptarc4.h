/*H*************************************************************************************/
/*!
    \File    cryptarc4.h

    \Description
        This module is a from-scratch ARC4 implementation designed to avoid
        any intellectual property complications. The ARC4 stream cipher is
        known to produce output that is compatible with the RC4 stream cipher.

    \Notes
        This algorithm from this cypher was taken from the web site: ciphersaber.gurus.com
        It is based on the RC4 compatible algorithm that was published in the 2nd ed of
        the book Applied Cryptography by Bruce Schneier. This is a private-key stream
        cipher which means that some other secure way of exchanging cipher keys prior
        to algorithm use is required. Its strength is directly related to the key exchange
        algorithm strengh. In operation, each individual stream message must use a unique
        key. This is handled by appending on 10 byte random value onto the private key.
        This 10-byte data can be sent by public means to the receptor (or included at the
        start of a file or whatever). When the private key is combined with this public
        key, it essentially puts the cypher into a random starting state (it is like
        using a message digest routine to generate a random hash for password comparison).
        The code below was written from scratch using only a textual algorithm description.

    \Copyright
        Copyright (c) Electronic Arts 2000-2002

    \Version 1.0 02/25/2000 (gschaefer) First Version
    \Version 1.1 11/06/2002 (jbrookes) Removed Create()/Destroy() to eliminate mem alloc dependencies.
*/
/*************************************************************************************H*/

#ifndef _cryptarc4_h
#define _cryptarc4_h

/*!
\Moduledef CryptArc4 CryptArc4
\Modulemember Crypt
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct CryptArc4T CryptArc4T;

//! all fields are PRIVATE
struct CryptArc4T
{
    uint8_t state[256];
    uint8_t walk;
    uint8_t swap;
};

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create state for ARC4 encryption/decryption module.
DIRTYCODE_API void CryptArc4Init(CryptArc4T *pState, const unsigned char *pKeyBuf, int32_t iKeyLen, int32_t iIter);

// apply the cipher to the data. calling twice undoes the uncryption
DIRTYCODE_API void CryptArc4Apply(CryptArc4T *pState, unsigned char *pBuffer, int32_t iLength);

// advance the cipher state by iLength bytes
DIRTYCODE_API void CryptArc4Advance(CryptArc4T *pState, int32_t iLength);

// encrypt an asciiz string, with a 7-bit asciiz string result
DIRTYCODE_API void CryptArc4StringEncrypt(char *pDst, int32_t iLen, const char *pSrc, const uint8_t *pKey, int32_t iKey, int32_t iIter);

// decrypt an asciiz string, from a 7-bit asciiz encrypted string
DIRTYCODE_API void CryptArc4StringDecrypt(char *pDst, int32_t iLen, const char *pSrc, const uint8_t *pKey, int32_t iKey, int32_t iIter);

#if DIRTYCODE_DEBUG
  // encryption/decryption helper intended for use with static strings that need to be encrypted in the binary
  #define CryptArc4StringEncryptStatic(_pStr, _iStrSize, _pKey, _iKeySize, _pStrSrc) CryptArc4StringEncryptStaticCode(_pStr, _iStrSize, _pKey, _iKeySize, _pStrSrc)
#else
  // release version of string encrypt; does not pass source (plaintext) string
  #define CryptArc4StringEncryptStatic(_pStr, _iStrSize, _pKey, _iKeySize, _pStrSrc) CryptArc4StringEncryptStaticCode(_pStr, _iStrSize, _pKey, _iKeySize, NULL)
#endif

// encryption/decryption helper intended for use with static strings that need to be encrypted in the binary (do not call directly; use CryptArc4StringEncryptStatic() wrapper)
DIRTYCODE_API int32_t CryptArc4StringEncryptStaticCode(char *pStr, int32_t iStrSize, const uint8_t *pKey, int32_t iKeySize, const char *pStrSrc);

#ifdef __cplusplus
}
#endif

//@}

#endif // _cryptarc4_h

