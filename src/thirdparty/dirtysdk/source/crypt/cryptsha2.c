/*H*******************************************************************/
/*!
    \File cryptsha2.c

    \Description
        This module implements SHA2 as defined in RFC 6234, which is
        itself based on FIPS 180-2.  This implementation is modeled
        after the CryptSha1 implementation by sbevan.

    \Notes
        The implementation is based on the algorithm description in sections
        4 through 6 of RFC 6234 and not on the C code in section 8.

        Currently the code is a straightforward implementation of the 
        algorithm, no attempt has been made to optimize it in any way.

        The code deliberately uses some of the naming conventions from
        the RFC to in order to aid comprehension.  

        This implementation is limited to hashing no more than 2^32-9
        bytes.  It will silently produce the wrong result if an attempt
        is made to hash more data.

    \Copyright
        Copyright (c) Electronic Arts 2013

    \Version 1.0 11/05/2013 (jbrookes) First Version
*/
/*******************************************************************H*/

/*** Include files ***************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/crypt/cryptsha2.h"

/*** Defines *********************************************************/

// definitions for functions and constants used: http://tools.ietf.org/html/rfc6234#section-5
#define SHR(_n,_x)          ((_x)>>(_n))

#define ROTR(_n,_x,_w)      (((_x)>>(_n))|((_x)<<((_w)-(_n))))
#define ROTL(_n,_x,_w)      (((_x)<<(_n))|((_x)>>((_w)-(_n))))

#define CH(_x,_y,_z)        (((_x)&(_y))^(~(_x)&(_z)))
#define MAJ(_x,_y,_z)       (((_x)&(_y))^((_x)&(_z))^((_y)&(_z)))

#define BSIG0_32(_x)        (ROTR(2,_x,32)^ROTR(13,_x,32)^ROTR(22,_x,32))
#define BSIG1_32(_x)        (ROTR(6,_x,32)^ROTR(11,_x,32)^ROTR(25,_x,32))
#define SSIG0_32(_x)        (ROTR(7,_x,32)^ROTR(18,_x,32)^SHR(3,_x))
#define SSIG1_32(_x)        (ROTR(17,_x,32)^ROTR(19,_x,32)^SHR(10,_x))

#define BSIG0_64(_x)        (ROTR(28,_x,64)^ROTR(34,_x,64)^ROTR(39,_x,64))
#define BSIG1_64(_x)        (ROTR(14,_x,64)^ROTR(18,_x,64)^ROTR(41,_x,64))
#define SSIG0_64(_x)        (ROTR(1,_x,64)^ROTR(8,_x,64)^SHR(7,_x))
#define SSIG1_64(_x)        (ROTR(19,_x,64)^ROTR(61,_x,64)^SHR(6,_x))

/*** Type Definitions ************************************************/

/*** Variables *******************************************************/

// constants defined in FIPS 180-3 section 4.2.2
static const uint32_t K_32[64] =
{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// constants defined in FIPS 180-3 section 4.2.3
static const uint64_t K_64[80] =
{
    0x428A2F98D728AE22ull, 0x7137449123EF65CDull, 0xB5C0FBCFEC4D3B2Full, 0xE9B5DBA58189DBBCull,
    0x3956C25BF348B538ull, 0x59F111F1B605D019ull, 0x923F82A4AF194F9Bull, 0xAB1C5ED5DA6D8118ull,
    0xD807AA98A3030242ull, 0x12835B0145706FBEull, 0x243185BE4EE4B28Cull, 0x550C7DC3D5FFB4E2ull,
    0x72BE5D74F27B896Full, 0x80DEB1FE3B1696B1ull, 0x9BDC06A725C71235ull, 0xC19BF174CF692694ull,
    0xE49B69C19EF14AD2ull, 0xEFBE4786384F25E3ull, 0x0FC19DC68B8CD5B5ull, 0x240CA1CC77AC9C65ull,
    0x2DE92C6F592B0275ull, 0x4A7484AA6EA6E483ull, 0x5CB0A9DCBD41FBD4ull, 0x76F988DA831153B5ull,
    0x983E5152EE66DFABull, 0xA831C66D2DB43210ull, 0xB00327C898FB213Full, 0xBF597FC7BEEF0EE4ull,
    0xC6E00BF33DA88FC2ull, 0xD5A79147930AA725ull, 0x06CA6351E003826Full, 0x142929670A0E6E70ull,
    0x27B70A8546D22FFCull, 0x2E1B21385C26C926ull, 0x4D2C6DFC5AC42AEDull, 0x53380D139D95B3DFull,
    0x650A73548BAF63DEull, 0x766A0ABB3C77B2A8ull, 0x81C2C92E47EDAEE6ull, 0x92722C851482353Bull,
    0xA2BFE8A14CF10364ull, 0xA81A664BBC423001ull, 0xC24B8B70D0F89791ull, 0xC76C51A30654BE30ull,
    0xD192E819D6EF5218ull, 0xD69906245565A910ull, 0xF40E35855771202Aull, 0x106AA07032BBD1B8ull,
    0x19A4C116B8D2D0C8ull, 0x1E376C085141AB53ull, 0x2748774CDF8EEB99ull, 0x34B0BCB5E19B48A8ull,
    0x391C0CB3C5C95A63ull, 0x4ED8AA4AE3418ACBull, 0x5B9CCA4F7763E373ull, 0x682E6FF3D6B2B8A3ull,
    0x748F82EE5DEFB2FCull, 0x78A5636F43172F60ull, 0x84C87814A1F0AB72ull, 0x8CC702081A6439ECull,
    0x90BEFFFA23631E28ull, 0xA4506CEBDE82BDE9ull, 0xBEF9A3F7B2C67915ull, 0xC67178F2E372532Bull,
    0xCA273ECEEA26619Cull, 0xD186B8C721C0C207ull, 0xEADA7DD6CDE0EB1Eull, 0xF57D4F7FEE6ED178ull,
    0x06F067AA72176FBAull, 0x0A637DC5A2C898A6ull, 0x113F9804BEF90DAEull, 0x1B710B35131C471Bull,
    0x28DB77F523047D84ull, 0x32CAAB7B40C72493ull, 0x3C9EBE0A15C9BEBCull, 0x431D67C49C100D4Cull,
    0x4CC5D4BECB3E42B6ull, 0x597F299CFC657E2Aull, 0x5FCB6FAB3AD6FAECull, 0x6C44198C4A475817ull
};


/*** Private functions ***********************************************/


/*F*******************************************************************/
/*!
    \Function _CryptSha2CopyHash224_256

    \Description
        Extract the SHA2 hash and copy it to a byte buffer.

    \Input *pSha2   - SHA2 state
    \Input *pBuffer - where to store the hash
    \Input  uLength - how many bytes of the hash to extract

    \Version 11/04/2013 (jbrookes)
*/
/*******************************************************************F*/
static void _CryptSha2CopyHash224_256(CryptSha2T *pSha2, uint8_t *pBuffer, uint32_t uLength)
{
    uint32_t uByte;

    if (uLength > pSha2->uHashSize)
    {
        uLength = pSha2->uHashSize;
    }
    for (uByte = 0; uByte != uLength; uByte += 1)
    {
        pBuffer[uByte] = pSha2->TempHash.H_32[(uByte/4)]>>((3-(uByte%4))*8);
    }
}

/*F*******************************************************************/
/*!
    \Function _CryptSha2CopyHash384_512

    \Description
        Extract the SHA2 hash and copy it to a byte buffer.

    \Input *pSha2   - SHA2 state
    \Input *pBuffer - where to store the hash
    \Input  uLength - how many bytes of the hash to extract

    \Version 11/04/2013 (jbrookes)
*/
/*******************************************************************F*/
static void _CryptSha2CopyHash384_512(CryptSha2T *pSha2, uint8_t *pBuffer, uint32_t uLength)
{
    uint32_t uByte;

    if (uLength > pSha2->uHashSize)
    {
        uLength = pSha2->uHashSize;
    }
    for (uByte = 0; uByte != uLength; uByte += 1)
    {
        pBuffer[uByte] = pSha2->TempHash.H_64[(uByte/8)]>>((7-(uByte%8))*8);
    }
}

/*F*******************************************************************/
/*!
    \Function _CryptSha2ProcessBlock224_256

    \Description
        SHA2 a 64-byte block of data using 224 or 256 bit version.

    \Input *pSha2  - SHA2 state
    \Input *M      - start of 64-bytes to be processed

    \Notes
        This is a literal translation of the method described in RFC
        6234 section 6.2.  The variable names deliberately match those
        used in that section in order to aid manual verification of
        algorithm correctness.

    \Version 11/04/2013 (jbrookes)
*/
/*******************************************************************F*/
static void _CryptSha2ProcessBlock224_256(CryptSha2T *pSha2, const uint8_t *M)
{
    unsigned a, b, c, d, e, f, g, h;
    uint32_t W[64];
    uint32_t t;
    unsigned T1, T2;

    // RFC 6234 section 6.2: http://tools.ietf.org/html/rfc6234#section-6.2
    for (t = 0; t < 16; t += 1)
    {
        W[t] = (M[t*4]<<24)|(M[t*4+1]<<16)|(M[t*4+2]<<8)|M[t*4+3];
    }
    for (t = 16; t < 64; t += 1)
    {
        W[t] = SSIG1_32(W[t-2]) + W[t-7] + SSIG0_32(W[t-15]) + W[t-16];
    }

    // section 6.2.2
    a = pSha2->TempHash.H_32[0];
    b = pSha2->TempHash.H_32[1];
    c = pSha2->TempHash.H_32[2];
    d = pSha2->TempHash.H_32[3];
    e = pSha2->TempHash.H_32[4];
    f = pSha2->TempHash.H_32[5];
    g = pSha2->TempHash.H_32[6];
    h = pSha2->TempHash.H_32[7];

    // section 6.1.3
    for (t = 0; t < 64; t += 1)
    {
        T1 = h + BSIG1_32(e) + CH(e, f, g) + K_32[t] + W[t];
        T2 = BSIG0_32(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    // section 6.1.4
    pSha2->TempHash.H_32[0] += a;
    pSha2->TempHash.H_32[1] += b;
    pSha2->TempHash.H_32[2] += c;
    pSha2->TempHash.H_32[3] += d;
    pSha2->TempHash.H_32[4] += e;
    pSha2->TempHash.H_32[5] += f;
    pSha2->TempHash.H_32[6] += g;
    pSha2->TempHash.H_32[7] += h;
}

/*F*******************************************************************/
/*!
    \Function _CryptSha2ProcessBlock384_512

    \Description
        SHA2 a 64-byte block of data using 384 or 512 bit version

    \Input *pSha2  - SHA2 state
    \Input *M      - start of 64-bytes to be processed

    \Notes
        This is a literal translation of the method described in RFC
        6234 section 6.4.  The variable names deliberately match those
        used in that section in order to aid manual verification of
        algorithm correctness.

    \Version 11/04/2013 (jbrookes)
*/
/*******************************************************************F*/
static void _CryptSha2ProcessBlock384_512(CryptSha2T *pSha2, const uint8_t *M)
{
    uint64_t a, b, c, d, e, f, g, h;
    uint64_t T1, T2;
    uint64_t W[80];
    uint32_t t;

    // RFC 6234 section 6.4: http://tools.ietf.org/html/rfc6234#section-6.4
    for (t = 0; t < 16; t += 1)
    {
        W[t] = ((uint64_t)M[t*8+0]<<56)|((uint64_t)M[t*8+1]<<48)|((uint64_t)M[t*8+2]<<40)|((uint64_t)M[t*8+3]<<32)|
               ((uint64_t)M[t*8+4]<<24)|((uint64_t)M[t*8+5]<<16)|((uint64_t)M[t*8+6]<<8)|(uint64_t)M[t*8+7];
    }
    for (t = 16; t < 80; t += 1)
    {
        W[t] = SSIG1_64(W[t-2]) + W[t-7] + SSIG0_64(W[t-15]) + W[t-16];
    }

    // section 6.4.2
    a = pSha2->TempHash.H_64[0];
    b = pSha2->TempHash.H_64[1];
    c = pSha2->TempHash.H_64[2];
    d = pSha2->TempHash.H_64[3];
    e = pSha2->TempHash.H_64[4];
    f = pSha2->TempHash.H_64[5];
    g = pSha2->TempHash.H_64[6];
    h = pSha2->TempHash.H_64[7];

    // section 6.4.3
    for (t = 0; t < 80; t += 1)
    {
        T1 = h + BSIG1_64(e) + CH(e, f, g) + K_64[t] + W[t];
        T2 = BSIG0_64(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    // section 6.4.4
    pSha2->TempHash.H_64[0] += a;
    pSha2->TempHash.H_64[1] += b;
    pSha2->TempHash.H_64[2] += c;
    pSha2->TempHash.H_64[3] += d;
    pSha2->TempHash.H_64[4] += e;
    pSha2->TempHash.H_64[5] += f;
    pSha2->TempHash.H_64[6] += g;
    pSha2->TempHash.H_64[7] += h;
}


/*** Public functions ************************************************/


/*F*******************************************************************/
/*!
    \Function CryptSha2Init

    \Description
        Initialize SHA2 state based on mode

    \Input *pSha2       - SHA2 state
    \Input iHashSize    - hash size (CRYPTSHA*_HASHSIZE)

    \Version 11/04/2013 (jbrookes)
*/
/*******************************************************************F*/
void CryptSha2Init(CryptSha2T *pSha2, int32_t iHashSize)
{
    pSha2->uCount = 0;
    pSha2->uPartialCount = 0;
    pSha2->uHashSize = (uint8_t)iHashSize;
    pSha2->uBlockSize = (pSha2->uHashSize < CRYPTSHA384_HASHSIZE) ? 64 : 128;

    // all the constants come from RFC 6234 section 6.1
    if (pSha2->uHashSize == CRYPTSHA224_HASHSIZE)
    {
        pSha2->TempHash.H_32[0] = 0xc1059ed8;
        pSha2->TempHash.H_32[1] = 0x367cd507;
        pSha2->TempHash.H_32[2] = 0x3070dd17;
        pSha2->TempHash.H_32[3] = 0xf70e5939;
        pSha2->TempHash.H_32[4] = 0xffc00b31;
        pSha2->TempHash.H_32[5] = 0x68581511;
        pSha2->TempHash.H_32[6] = 0x64f98fa7;
        pSha2->TempHash.H_32[7] = 0xbefa4fa4;
    }
    else if (pSha2->uHashSize == CRYPTSHA256_HASHSIZE)
    {
        pSha2->TempHash.H_32[0] = 0x6a09e667;
        pSha2->TempHash.H_32[1] = 0xbb67ae85;
        pSha2->TempHash.H_32[2] = 0x3c6ef372;
        pSha2->TempHash.H_32[3] = 0xa54ff53a;
        pSha2->TempHash.H_32[4] = 0x510e527f;
        pSha2->TempHash.H_32[5] = 0x9b05688c;
        pSha2->TempHash.H_32[6] = 0x1f83d9ab;
        pSha2->TempHash.H_32[7] = 0x5be0cd19;
    }
    else if (pSha2->uHashSize == CRYPTSHA384_HASHSIZE)
    {
        pSha2->TempHash.H_64[0] = 0xcbbb9d5dc1059ed8;
        pSha2->TempHash.H_64[1] = 0x629a292a367cd507;
        pSha2->TempHash.H_64[2] = 0x9159015a3070dd17;
        pSha2->TempHash.H_64[3] = 0x152fecd8f70e5939;
        pSha2->TempHash.H_64[4] = 0x67332667ffc00b31;
        pSha2->TempHash.H_64[5] = 0x8eb44a8768581511;
        pSha2->TempHash.H_64[6] = 0xdb0c2e0d64f98fa7;
        pSha2->TempHash.H_64[7] = 0x47b5481dbefa4fa4;
    }
    else if (pSha2->uHashSize == CRYPTSHA512_HASHSIZE)
    {
        pSha2->TempHash.H_64[0] = 0x6a09e667f3bcc908;
        pSha2->TempHash.H_64[1] = 0xbb67ae8584caa73b;
        pSha2->TempHash.H_64[2] = 0x3c6ef372fe94f82b;
        pSha2->TempHash.H_64[3] = 0xa54ff53a5f1d36f1;
        pSha2->TempHash.H_64[4] = 0x510e527fade682d1;
        pSha2->TempHash.H_64[5] = 0x9b05688c2b3e6c1f;
        pSha2->TempHash.H_64[6] = 0x1f83d9abfb41bd6b;
        pSha2->TempHash.H_64[7] = 0x5be0cd19137e2179;
    }
    else
    {
        NetPrintf(("cryptsha2: invalid hashsize %d\n", pSha2->uHashSize));
    }
}

/*F*******************************************************************/
/*!
    \Function CryptSha2Update

    \Description
        Hash the input and add it to the state

    \Input *pSha2    - SHA2 state
    \Input *pInput   - the input
    \Input uInputLen - length of input in bytes

    \Version 11/04/2013 (jbrookes)
*/
/*******************************************************************F*/
void CryptSha2Update(CryptSha2T *pSha2, const uint8_t *pInput, uint32_t uInputLen)
{
    if (pSha2->uPartialCount != 0)
    {
        uint32_t uWant = pSha2->uBlockSize - pSha2->uPartialCount;
        uint32_t uHave = uWant > uInputLen ? uInputLen : uWant;
        ds_memcpy(&pSha2->strData[pSha2->uPartialCount], pInput, uHave);
        pInput += uHave;
        uInputLen -= uHave;
        if (uHave == uWant)
        {
            pSha2->uHashSize < CRYPTSHA384_HASHSIZE ? _CryptSha2ProcessBlock224_256(pSha2, pSha2->strData) : _CryptSha2ProcessBlock384_512(pSha2, pSha2->strData);
            pSha2->uCount += pSha2->uBlockSize;
            pSha2->uPartialCount = 0;
        }
        else
        {
            pSha2->uPartialCount += uHave;
        }
    }
    while (uInputLen >= pSha2->uBlockSize)
    {
        pSha2->uHashSize < CRYPTSHA384_HASHSIZE ? _CryptSha2ProcessBlock224_256(pSha2, pInput) : _CryptSha2ProcessBlock384_512(pSha2, pInput);
        pSha2->uCount += pSha2->uBlockSize;
        uInputLen -= pSha2->uBlockSize;
        pInput += pSha2->uBlockSize;
    }
    if (uInputLen != 0)
    {
        ds_memcpy(&pSha2->strData[pSha2->uPartialCount], pInput, uInputLen);
        pSha2->uPartialCount += uInputLen;
    }
}

/*F*******************************************************************/
/*!
    \Function CryptSha2Final

    \Description
        Generate the final hash from the SHA2 state

    \Input *pSha2   - the SHA2 state
    \Input *pBuffer - [out] where the hash should be written
    \Input uLength  - the number of bytes to write (may be less than hash size)

    \Version 11/04/2013 (jbrookes)
*/
/*******************************************************************F*/
void CryptSha2Final(CryptSha2T *pSha2, uint8_t *pBuffer, uint32_t uLength)
{
    uint32_t uByte, uSpace = pSha2->uBlockSize - pSha2->uPartialCount;
    const uint32_t uLengthSize = (pSha2->uHashSize < CRYPTSHA384_HASHSIZE) ? 8 : 16;
    uint8_t uPad = 0x80;

    pSha2->uCount += pSha2->uPartialCount;
    if (uSpace < (uLengthSize+1))
    {
        pSha2->strData[pSha2->uPartialCount] = uPad;
        for (uByte = pSha2->uPartialCount+1; uByte < pSha2->uBlockSize; uByte += 1)
        {
            pSha2->strData[uByte] = 0x0;
        }
        pSha2->uHashSize < CRYPTSHA384_HASHSIZE ? _CryptSha2ProcessBlock224_256(pSha2, pSha2->strData) : _CryptSha2ProcessBlock384_512(pSha2, pSha2->strData);
        uPad = 0x0;
        pSha2->uPartialCount = 0;
    }
    pSha2->strData[pSha2->uPartialCount] = uPad;
    for (uByte = pSha2->uPartialCount+1; uByte < (uint32_t)pSha2->uBlockSize-uLengthSize; uByte += 1)
    {
        pSha2->strData[uByte] = 0x0;
    }

    /* append length in bits, as per RFC 6234 section 4 except we only support a 32-bit byte
       length (35-bit bit length).  uses some bit shifting to avoid having to explicitly calculate
       pSha1->uCount*8 which could overflow 32-bits */
    if (uLengthSize == 16)
    {
        pSha2->strData[pSha2->uBlockSize-16] = 0;
        pSha2->strData[pSha2->uBlockSize-15] = 0;
        pSha2->strData[pSha2->uBlockSize-14] = 0;
        pSha2->strData[pSha2->uBlockSize-13] = 0;
        pSha2->strData[pSha2->uBlockSize-12] = 0;
        pSha2->strData[pSha2->uBlockSize-11] = 0;
        pSha2->strData[pSha2->uBlockSize-10] = 0;
        pSha2->strData[pSha2->uBlockSize- 9] = 0;
    }
    pSha2->strData[pSha2->uBlockSize-8] = 0;
    pSha2->strData[pSha2->uBlockSize-7] = 0;
    pSha2->strData[pSha2->uBlockSize-6] = 0;
    pSha2->strData[pSha2->uBlockSize-5] = (pSha2->uCount>>(32-3))&0xFF;
    pSha2->strData[pSha2->uBlockSize-4] = (pSha2->uCount>>(24-3))&0xFF;
    pSha2->strData[pSha2->uBlockSize-3] = (pSha2->uCount>>(16-3))&0xFF;
    pSha2->strData[pSha2->uBlockSize-2] = (pSha2->uCount>>(8-3))&0xFF;
    pSha2->strData[pSha2->uBlockSize-1] = (pSha2->uCount<<3)&0xFF;

    // process final block, and copy the hash to final buffer
    if (pSha2->uHashSize < CRYPTSHA384_HASHSIZE)
    {
        _CryptSha2ProcessBlock224_256(pSha2, pSha2->strData);
        _CryptSha2CopyHash224_256(pSha2, pBuffer, uLength);
    }
    else
    {
        _CryptSha2ProcessBlock384_512(pSha2, pSha2->strData);
        _CryptSha2CopyHash384_512(pSha2, pBuffer, uLength);
    }
}



