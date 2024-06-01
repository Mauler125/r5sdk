/*H*******************************************************************/
/*!
    \File crypthash.c

    \Description
        This module implements a generic wrapper for all cryptograph
        hash modules.

    \Copyright
        Copyright (c) Electronic Arts 2013

    \Version 1.0 11/05/2013 (jbrookes) First Version
*/
/*******************************************************************H*/

/*** Include files ***************************************************/

#include <string.h>

#include "DirtySDK/platform.h"

#include "DirtySDK/crypt/cryptmd5.h"
#include "DirtySDK/crypt/cryptsha1.h"
#include "DirtySDK/crypt/cryptsha2.h"
#include "DirtySDK/util/murmurhash3.h"

#include "DirtySDK/crypt/crypthash.h"

/*** Defines *********************************************************/

/*** Type Definitions ************************************************/

/*** Variables *******************************************************/

static const CryptHashT _CryptHash_Hashes[] =
{
    { (CryptHashInitT *)MurmurHash3Init2, (CryptHashUpdateT *)MurmurHash3Update, (CryptHashFinalT *)MurmurHash3Final, CRYPTHASH_MURMUR3, MURMURHASH_HASHSIZE  },
    { (CryptHashInitT *)CryptMD5Init2,    (CryptHashUpdateT *)CryptMD5Update,    (CryptHashFinalT *)CryptMD5Final,    CRYPTHASH_MD5,     MD5_BINARY_OUT       },
    { (CryptHashInitT *)CryptSha1Init2,   (CryptHashUpdateT *)CryptSha1Update,   (CryptHashFinalT *)CryptSha1Final,   CRYPTHASH_SHA1,    CRYPTSHA1_HASHSIZE   },
    { (CryptHashInitT *)CryptSha2Init,    (CryptHashUpdateT *)CryptSha2Update,   (CryptHashFinalT *)CryptSha2Final,   CRYPTHASH_SHA224,  CRYPTSHA224_HASHSIZE },
    { (CryptHashInitT *)CryptSha2Init,    (CryptHashUpdateT *)CryptSha2Update,   (CryptHashFinalT *)CryptSha2Final,   CRYPTHASH_SHA256,  CRYPTSHA256_HASHSIZE },
    { (CryptHashInitT *)CryptSha2Init,    (CryptHashUpdateT *)CryptSha2Update,   (CryptHashFinalT *)CryptSha2Final,   CRYPTHASH_SHA384,  CRYPTSHA384_HASHSIZE },
    { (CryptHashInitT *)CryptSha2Init,    (CryptHashUpdateT *)CryptSha2Update,   (CryptHashFinalT *)CryptSha2Final,   CRYPTHASH_SHA512,  CRYPTSHA512_HASHSIZE }
};

/*** Private functions ***********************************************/

/*** Public functions ************************************************/


/*F*******************************************************************/
/*!
    \Function CryptHashGet

    \Description
        Get CryptHash function block based on input hash size

    \Input eHashType    - hash type to get size of

    \Output
        CryptHashT *    - hash block pointer, or NULL if not found

    \Version 11/05/2013 (jbrookes)
*/
/*******************************************************************F*/
const CryptHashT *CryptHashGet(CryptHashTypeE eHashType)
{
    const CryptHashT *pHash;
    pHash = ((eHashType > CRYPTHASH_NULL) && (eHashType < CRYPTHASH_NUMHASHES)) ? &_CryptHash_Hashes[((int32_t)eHashType)-1] : (const CryptHashT *)NULL;
    return(pHash);
}
/*F*******************************************************************/
/*!
    \Function CryptHashGetSize

    \Description
        Get hash size for specified hash type

    \Input eHashType    - hash type to get size of

    \Output
        int32_t         - size of specified hash

    \Version 03/07/2014 (jbrookes)
*/
/*******************************************************************F*/
int32_t CryptHashGetSize(CryptHashTypeE eHashType)
{
    const CryptHashT *pHash = CryptHashGet(eHashType);
    int32_t iHashSize = (pHash != NULL) ? pHash->iHashSize : -1;
    return(iHashSize);
}

/*F*******************************************************************/
/*!
    \Function CryptHashGetBySize

    \Description
        Get the hash based on its size

    \Input iHashSize    - size of hash we are trying to lookup

    \Output
        CryptHashTypeE  - hash type, CRYPTHASH_NULL if not found

    \Notes
        Due to murmurhash3 using the same size as MD5, we choose to
        not include it.

    \Version 01/09/2018 (jbrookes)
*/
/*******************************************************************F*/
CryptHashTypeE CryptHashGetBySize(int32_t iHashSize)
{
    switch (iHashSize)
    {
        case MD5_BINARY_OUT:
            return(CRYPTHASH_MD5);
        case CRYPTSHA1_HASHSIZE:
            return(CRYPTHASH_SHA1);
        case CRYPTSHA224_HASHSIZE:
            return(CRYPTHASH_SHA224);
        case CRYPTSHA256_HASHSIZE:
            return(CRYPTHASH_SHA256);
        case CRYPTSHA384_HASHSIZE:
            return(CRYPTHASH_SHA384);
        case CRYPTSHA512_HASHSIZE:
            return(CRYPTHASH_SHA512);
        default:
            return(CRYPTHASH_NULL);
    }
}
