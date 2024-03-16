/*H********************************************************************************/
/*!
    \File murmurhash3.c

    \Description
        An implementation of MurmurHash3, based heavily on the x64 128bit output
        implementation.  MurmurHash3 is a public domain hashing algorithm written
        by Austin Appleby, ref http://code.google.com/p/smhasher/wiki/MurmurHash3.

        MurmurHash3 is not considered cryptographically secure, however it has
        excellent collision resistance and is orders of magnitude faster than the
        fastest secure cryptographic hash.

    \Copyright
        Copyright (c) 2014 Electronic Arts

    \Version 03/07/2014 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <stdlib.h>

#include "DirtySDK/util/murmurhash3.h"

/*** Defines **********************************************************************/

#if defined(_MSC_VER)
 #define MH3_FORCE_INLINE __forceinline
#else
 #define MH3_FORCE_INLINE __inline__ __attribute__((always_inline))
#endif


#define MURMURHASH3_C1  (0x87c37b91114253d5)
#define MURMURHASH3_C2  (0x4cf5ad432745937f)

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

#if !defined(_MSC_VER)
/*F********************************************************************************/
/*!
    \Function _rotl64

    \Description
        Implementation of _rotl64 for non-Windows.

    \Input x            - 64bit data to rotate
    \Input r            - number of bits to rotate

    \Output
        uint64_t        - rotated data

    \Version 03/07/2014 (jbrookes)
*/
/********************************************************************************F*/
static MH3_FORCE_INLINE uint64_t _MurmurHash3Rotl64(uint64_t x, int8_t r)
{
    return (x << r) | (x >> (64 - r));
}
#else
    #define _MurmurHash3Rotl64(x, y) _rotl64(x, y)
#endif

/*F********************************************************************************/
/*!
    \Function _MurmurHash3Fmix

    \Description
        'fmix' operation from murmurhash3

    \Input k            - 64bit data to mix

    \Output
        uint64_t        - 64 bits of mixed data

    \Version 03/07/2014 (jbrookes)
*/
/********************************************************************************F*/
static MH3_FORCE_INLINE uint64_t _MurmurHash3Fmix (uint64_t k)
{
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccd;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53;
    k ^= k >> 33;
    return(k);
}

/*F********************************************************************************/
/*!
    \Function _MurmurHash3Read64

    \Description
        Read 64 bits of input data.  Little-endian versions that can handle an
        unaligned 64bit load from memory do a direct read for speed.  Other
        versions execute a portable 64bit load, which is slower.

    \Input *pData       - input data to read
    \Input iData        - 64bit offset of data to read

    \Output
        uint64_t        - 64 bits of data ready to be hashed

    \Version 03/07/2014 (jbrookes)
*/
/********************************************************************************F*/
MH3_FORCE_INLINE static uint64_t _MurmurHash3Read64(const uint64_t *pData, int32_t iData)
{
#if EA_SYSTEM_LITTLE_ENDIAN
    return(pData[iData]);
#else
    /* read data little endian; we do this because we want to optimize
       for running on x64 hardware */
    const uint8_t *_pData = (const uint8_t *)(pData+iData);
    uint64_t uData;
    uData  = ((uint64_t)_pData[0]);
    uData |= ((uint64_t)_pData[1]) << 8;
    uData |= ((uint64_t)_pData[2]) << 16;
    uData |= ((uint64_t)_pData[3]) << 24;
    uData |= ((uint64_t)_pData[4]) << 32;
    uData |= ((uint64_t)_pData[5]) << 40;
    uData |= ((uint64_t)_pData[6]) << 48;
    uData |= ((uint64_t)_pData[7]) << 56;
    return(uData);
#endif
}

/*F********************************************************************************/
/*!
    \Function _MurmurHash3Transform

    \Description
        Add 16 byte block of state data to the MurmurHash3 context (hash the data)

    \Input *pContext    - hash context

    \Version 03/07/2014 (jbrookes)
*/
/********************************************************************************F*/
static void _MurmurHash3Transform(MurmurHash3T *pContext)
{
    uint64_t k1 = _MurmurHash3Read64((uint64_t *)pContext->aData, 0);
    uint64_t k2 = _MurmurHash3Read64((uint64_t *)pContext->aData, 1);
    uint64_t h1 = pContext->aState[0];
    uint64_t h2 = pContext->aState[1];
    const uint64_t c1 = MURMURHASH3_C1;
    const uint64_t c2 = MURMURHASH3_C2;

    k1 *= c1; k1  = _MurmurHash3Rotl64(k1,31); k1 *= c2; h1 ^= k1;

    h1 = _MurmurHash3Rotl64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

    k2 *= c2; k2  = _MurmurHash3Rotl64(k2,33); k2 *= c1; h2 ^= k2;

    h2 = _MurmurHash3Rotl64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;

    pContext->aState[0] = h1;
    pContext->aState[1] = h2;
}

/*F********************************************************************************/
/*!
    \Function _MurmurHash3Finalize

    \Description
        Hash remaining data, output hash result

    \Input *pOutput - [out] buffer to store result hash
    \Input iOutLen  - length of output
    \Input *pInput  - remaining input data to hash
    \Input iInpLen  - total size of input data (not remaining)
    \Input h1       - 1st 64bit word of 128bit state
    \Input h2       - 2nd 64bit word of 128bit state

    \Version 03/07/2014 (jbrookes)
*/
/********************************************************************************F*/
static void _MurmurHash3Finalize(uint8_t *pOutput, int32_t iOutLen, const uint8_t *pInput, int32_t iInpLen, uint64_t h1, uint64_t h2)
{
    const uint64_t c1 = MURMURHASH3_C1;
    const uint64_t c2 = MURMURHASH3_C2;
    uint64_t k1 = 0;
    uint64_t k2 = 0;

    // hash remaining unaligned data
    switch(iInpLen&15)
    {
        case 15: k2 ^= ((uint64_t)pInput[14]) << 48;
        case 14: k2 ^= ((uint64_t)pInput[13]) << 40;
        case 13: k2 ^= ((uint64_t)pInput[12]) << 32;
        case 12: k2 ^= ((uint64_t)pInput[11]) << 24;
        case 11: k2 ^= ((uint64_t)pInput[10]) << 16;
        case 10: k2 ^= ((uint64_t)pInput[ 9]) << 8;
        case  9: k2 ^= ((uint64_t)pInput[ 8]) << 0;
             k2 *= c2; k2  = _MurmurHash3Rotl64(k2,33); k2 *= c1; h2 ^= k2;

        case  8: k1 ^= ((uint64_t)pInput[ 7]) << 56;
        case  7: k1 ^= ((uint64_t)pInput[ 6]) << 48;
        case  6: k1 ^= ((uint64_t)pInput[ 5]) << 40;
        case  5: k1 ^= ((uint64_t)pInput[ 4]) << 32;
        case  4: k1 ^= ((uint64_t)pInput[ 3]) << 24;
        case  3: k1 ^= ((uint64_t)pInput[ 2]) << 16;
        case  2: k1 ^= ((uint64_t)pInput[ 1]) << 8;
        case  1: k1 ^= ((uint64_t)pInput[ 0]) << 0;
             k1 *= c1; k1  = _MurmurHash3Rotl64(k1,31); k1 *= c2; h1 ^= k1;
    };

    // finalization
    h1 ^= iInpLen;
    h2 ^= iInpLen;

    h1 += h2;
    h2 += h1;

    h1 = _MurmurHash3Fmix(h1);
    h2 = _MurmurHash3Fmix(h2);

    h1 += h2;
    h2 += h1;

    // write hash to output
    switch(iOutLen)
    {
        case 16: pOutput[15] = (uint8_t)(h2 >> 56);
        case 15: pOutput[14] = (uint8_t)(h2 >> 48);
        case 14: pOutput[13] = (uint8_t)(h2 >> 40);
        case 13: pOutput[12] = (uint8_t)(h2 >> 32);
        case 12: pOutput[11] = (uint8_t)(h2 >> 24);
        case 11: pOutput[10] = (uint8_t)(h2 >> 16);
        case 10: pOutput[9]  = (uint8_t)(h2 >> 8);
        case  9: pOutput[8]  = (uint8_t)(h2);
        case  8: pOutput[7]  = (uint8_t)(h2 >> 56);
        case  7: pOutput[6]  = (uint8_t)(h1 >> 48);
        case  6: pOutput[5]  = (uint8_t)(h1 >> 40);
        case  5: pOutput[4]  = (uint8_t)(h1 >> 32);
        case  4: pOutput[3]  = (uint8_t)(h1 >> 24);
        case  3: pOutput[2]  = (uint8_t)(h1 >> 16);
        case  2: pOutput[1]  = (uint8_t)(h1 >> 8);
        case  1: pOutput[0]  = (uint8_t)(h1);
    };
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function MurmurHash3Init

    \Description
        Init the MurmurHash3 context

    \Input *pContext    - hash context

    \Version 03/07/2014 (jbrookes)
*/
/********************************************************************************F*/
void MurmurHash3Init(MurmurHash3T *pContext)
{
    ds_memclr(pContext, sizeof(*pContext));
    // borrowed init vector from MD5
    pContext->aState[0] = 0x67452301efcdab89;
    pContext->aState[1] = 0x98badcfe10325476;
}

/*F********************************************************************************/
/*!
    \Function MurmurHash3Init2

    \Description
        Init the MurmurHash3 context, with hash size

    \Input *pContext    - hash context
    \Input iHashSize    - hash size (may be 32 or 128)

    \Version 03/07/2014 (jbrookes)
*/
/********************************************************************************F*/
void MurmurHash3Init2(MurmurHash3T *pContext, int32_t iHashSize)
{
    MurmurHash3Init(pContext);
}

/*F********************************************************************************/
/*!
    \Function MurmurHash3Update

    \Description
        Add data to the MurmurHash3 context (hash the data)

    \Input *pContext    - hash context
    \Input *pBuffer     - input data to hash
    \Input iLength      - length of data to hash

    \Version 03/07/2014 (jbrookes)
*/
/********************************************************************************F*/
void MurmurHash3Update(MurmurHash3T *pContext, const void *pBuffer, int32_t iLength)
{
    const uint8_t *pInput = (const uint8_t *)pBuffer;
    const uint64_t c1 = MURMURHASH3_C1;
    const uint64_t c2 = MURMURHASH3_C2;
    int32_t iCount, iAdd;
    uint64_t h1, h2, k1, k2;

    // get index into block buffer
    iCount = pContext->iCount&15;
    pContext->iCount += iLength;

    // see if we need to append to existing data
    if (iCount > 0)
    {
        // figure out number to fill block
        iAdd = 16-iCount;

        // if less than a full block
        if (iLength < iAdd)
        {
            ds_memcpy(pContext->aData+iCount, pInput, iLength);
            return;
        }

        // finish off the block and transform
        ds_memcpy(pContext->aData+iCount, pInput, iAdd);
        pInput += iAdd;
        iLength -= iAdd;
        _MurmurHash3Transform(pContext);
    }

    // get state & 64-bit data pointer
    h1 = pContext->aState[0];
    h2 = pContext->aState[1];

    // hash 128 bit blocks of data; inline for speed
    for ( ; iLength >= 16; iLength -= 16, pInput += 16)
    {
        k1 = _MurmurHash3Read64((const uint64_t *)pInput, 0);
        k2 = _MurmurHash3Read64((const uint64_t *)pInput, 1);

        k1 *= c1; k1  = _MurmurHash3Rotl64(k1,31); k1 *= c2; h1 ^= k1;

        h1 = _MurmurHash3Rotl64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

        k2 *= c2; k2  = _MurmurHash3Rotl64(k2,33); k2 *= c1; h2 ^= k2;

        h2 = _MurmurHash3Rotl64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
    }

    // update state
    pContext->aState[0] = h1;
    pContext->aState[1] = h2;

    // copy partial block data, if any, into state
    if (iLength > 0)
    {
        ds_memcpy_s(pContext->aData, sizeof(pContext->aData), pInput, iLength);
    }
}

/*F********************************************************************************/
/*!
    \Function MurmurHash3Final

    \Description
        Convert MurmurHash3 state into final output form

    \Input *pContext    - hash context
    \Input *pBuffer     - [out] buffer to hold output
    \Input iLength      - length of output buffer

    \Version 03/07/2014 (jbrookes)
*/
/********************************************************************************F*/
void MurmurHash3Final(MurmurHash3T *pContext, void *pBuffer, int32_t iLength)
{
    _MurmurHash3Finalize((uint8_t *)pBuffer, iLength, pContext->aData, pContext->iCount, pContext->aState[0], pContext->aState[1]);
}

/*F********************************************************************************/
/*!
    \Function MurmurHash3

    \Description
        Generate 128-bit MurmurHash3 of specified input data

    \Input *_pOutput    - [out] buffer to hold output
    \Input iOutLen      - length of output buffer
    \Input *_pInput     - input data to be hashed
    \Input iInpLen      - length of input
    \Input *pKey        - key to init hash
    \Input iKeyLen      - length of key (must be 16)

    \Todo
        Support key lengths other than 16

    \Version 03/07/2014 (jbrookes)
*/
/********************************************************************************F*/
void MurmurHash3(void *_pOutput, int32_t iOutLen, const void *_pInput, int32_t iInpLen, const void *pKey, int32_t iKeyLen)
{
    const uint8_t *pInput = (const uint8_t *)_pInput;
    uint8_t *pOutput = (uint8_t *)_pOutput;
    int32_t iBlock, iNumBlocks = iInpLen/16;
    const uint64_t c1 = MURMURHASH3_C1;
    const uint64_t c2 = MURMURHASH3_C2;
    uint64_t h1, h2, k1, k2;
    const uint64_t *pInput64;

    // get state (seed)
    h1 = _MurmurHash3Read64((const uint64_t *)pKey, 0);
    h2 = _MurmurHash3Read64((const uint64_t *)pKey, 1);

    // get 64-bit pointer to input
    pInput64 = (const uint64_t *)pInput;

    // hash the 128 bit blocks of data
    for (iBlock = 0; iBlock < iNumBlocks; iBlock += 1)
    {
        k1 = _MurmurHash3Read64(pInput64, iBlock*2+0);
        k2 = _MurmurHash3Read64(pInput64, iBlock*2+1);

        k1 *= c1; k1  = _MurmurHash3Rotl64(k1,31); k1 *= c2; h1 ^= k1;

        h1 = _MurmurHash3Rotl64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

        k2 *= c2; k2  = _MurmurHash3Rotl64(k2,33); k2 *= c1; h2 ^= k2;

        h2 = _MurmurHash3Rotl64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
    }

    // hash remaining data and output hash
    _MurmurHash3Finalize(pOutput, iOutLen, pInput + iNumBlocks*16, iInpLen, h1, h2);
}







