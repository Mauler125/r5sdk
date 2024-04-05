/*H*******************************************************************/
/*!
    \File cryptsha1.c

    \Description
        This module implements SHA1 as defined in RFC 3174.

    \Notes
        The implementation is based on the algorithm description in sections
        3 through 6 of RFC 3174 and not on the C code in section 7.

        Currently the code is a straightforward implementation of the 
        algorithm, no attempt has been made to optimize it in any way.
        See the notes for individual functions for descriptions of where
        optimizations are possible, but note that what is good for an x86
        with large caches may not be good for a MIPS based PS2 which has
        a much smaller instruction cache.

        The code deliberately uses some of the naming conventions from
        the RFC to in order to aid comprehension.  

        This implementation is limited to hashing no more than 2^32-9 bytes.
        It will silently produce the wrong result if an attempt is made to
        hash more data.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 06/06/2004 (sbevan) First Version
*/
/*******************************************************************H*/

/*** Include files ***************************************************/

#include <string.h>             /* memcpy */

#include "DirtySDK/platform.h"
#include "DirtySDK/crypt/cryptsha1.h"

/*** Defines *********************************************************/

// A circular left shift, see RFC 3174 section 3.c
//
// gcc understands the two shift idiom and replaces this
// with single rotate instruction on platforms that have it.
//
#define CRYPTSHA1_rol(n, x) (((x)<<(n))|((x)>>(32-(n))))

/*** Type Definitions ************************************************/

/*** Variables *******************************************************/

/*** Private functions ***********************************************/

/*F*******************************************************************/
/*!
    \Function _CryptSha1CopyHash

    \Description
        Extract the SHA1 hash and copy it to a byte buffer.

    \Input *pSha1   - SHA1 state
    \Input *pBuffer - where to store the hash
    \Input  uLength - how many bytes of the hash to extract

    \Output None

    \Version 1.0 06/06/2004 (sbevan) First Version
*/
/*******************************************************************F*/
static void _CryptSha1CopyHash(CryptSha1T *pSha1, void *pBuffer, uint32_t uLength)
{
    unsigned char *pOutput = pBuffer;
    uint32_t i;

    if (uLength > CRYPTSHA1_HASHSIZE)
    {
        uLength = CRYPTSHA1_HASHSIZE;
    }
    for (i = 0; i != uLength; i += 1)
    {
        pOutput[i] = pSha1->H[(i/4)]>>((3-(i%4))*8);
    }
}

/*F*******************************************************************/
/*!
    \Function _CryptSha1ProcessBlock

    \Description
        SHA1 a 64-byte block of data.

    \Input *pSha1  - SHA1 state
    \Input *M      - start of 64-bytes to be processed

    \Output None

    \Notes
        This is a literal translation of Method 1 as described in
        RFC 3174 section 6.1.  The variable names deliberately match those
        used in that section in order to aid manual verification of algorithm
        correctness.

        There is a lot of scope for performance improvements.  For example :-

          a) performing manual loop unrolling and loop fusion.
             An the extreme this could result in a single straight line
             chunk of code that on a register rich machine this could
             result in everything except the input and hash living in
             a register.

          b) taking advantage of hardware that can perform big-endian (MIPS) 
             and/or mis-aligned (x86) loads.

        OpenSSL does a) but it causes problems for some compilers since
        they don't do a good job of optimizing functions containing 80+ 
        local variables.  GCC should be able to cope but the x86 and
        MIPS assembly would need to be checked to make sure.

    \Version 1.0 06/06/2004 (sbevan) First Version
*/
/*******************************************************************F*/
static void _CryptSha1ProcessBlock(CryptSha1T *pSha1, const unsigned char *M)
{
    uint32_t i;
    uint32_t t;
    unsigned A, B, C, D, E;
    uint32_t W[80];

    // RFC 3174 section 6.1.a, divide input into 16 words (big-endian format)
    for (i = 0; i != 16; i += 1)
    {
        W[i] = (M[i*4]<<24)|(M[i*4+1]<<16)|(M[i*4+2]<<8)|M[i*4+3];
    }
    // RFC 3174 section 6.1.b
    for (t = 16; t != 80; t += 1)
    {
        W[t] = CRYPTSHA1_rol(1, W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }
    // RFC 3174 section 6.1.c
    A = pSha1->H[0];
    B = pSha1->H[1];
    C = pSha1->H[2];
    D = pSha1->H[3];
    E = pSha1->H[4];

    // RFC 3174 section 6.1.d split into 4 groups one for each variation of
    // f as defined in RFC 3174 section 5.
    for (t = 0; t != 20; t += 1)
    {
        uint32_t TEMP = CRYPTSHA1_rol(5, A) + ((B&C)|((~B)&D)) + E + W[t] + 0x5A827999;
        E = D; D = C; C = CRYPTSHA1_rol(30, B); B = A; A = TEMP;
    }

    for (t = 20; t != 40; t += 1)
    {
        uint32_t TEMP = CRYPTSHA1_rol(5, A) + (B^C^D) + E + W[t] + 0x6ED9EBA1;
        E = D; D = C; C = CRYPTSHA1_rol(30, B); B = A; A = TEMP;
    }

    for (t = 40; t != 60; t += 1)
    {
        uint32_t TEMP = CRYPTSHA1_rol(5, A) + ((B&C)|(B&D)|(C&D)) + E + W[t] + 0x8F1BBCDC;
        E = D; D = C; C = CRYPTSHA1_rol(30, B); B = A; A = TEMP;
    }

    for (t = 60; t != 80; t += 1)
    {
        uint32_t TEMP = CRYPTSHA1_rol(5, A) + (B^C^D) + E + W[t] + 0xCA62C1D6;
        E = D; D = C; C = CRYPTSHA1_rol(30, B); B = A; A = TEMP;
    }

    // Section 6.1.e
    pSha1->H[0] += A;
    pSha1->H[1] += B;
    pSha1->H[2] += C;
    pSha1->H[3] += D;
    pSha1->H[4] += E;
}


/*** Public functions ************************************************/


/*F*******************************************************************/
/*!
    \Function CryptSha1Init

    \Description
        Hash the input and add it to the state

    \Input *pSha1    - SHA1 state

    \Output None

    \Version 1.0 06/06/2004 (sbevan) First Version
*/
/*******************************************************************F*/
void CryptSha1Init(CryptSha1T *pSha1)
{
    pSha1->uCount = 0;
    pSha1->uPartialCount = 0;
    // All the constants come from RFC 3174 section 6.1
    pSha1->H[0] = 0x67452301;
    pSha1->H[1] = 0xEFCDAB89;
    pSha1->H[2] = 0x98BADCFE;
    pSha1->H[3] = 0x10325476;
    pSha1->H[4] = 0xC3D2E1F0;
}

/*F*******************************************************************/
/*!
    \Function CryptSha1Init2

    \Description
        Hash the input and add it to the state (alternate form)

    \Input *pSha1    - SHA1 state
    \Input iHashSize - hash size (unused)

    \Version 11/05/2013 (jbrookes)
*/
/*******************************************************************F*/
void CryptSha1Init2(CryptSha1T *pSha1, int32_t iHashSize)
{
    CryptSha1Init(pSha1);
}

/*F*******************************************************************/
/*!
    \Function CryptSha1Update

    \Description
        Hash the input and add it to the state

    \Input *pSha1    - SHA1 state
    \Input *pInput   - the input
    \Input uInputLen - length of input in bytes

    \Output None

    \Version 1.0 06/06/2004 (sbevan) First Version
*/
/*******************************************************************F*/
void CryptSha1Update(CryptSha1T *pSha1, const unsigned char *pInput, uint32_t uInputLen)
{
    if (pSha1->uPartialCount != 0)
    {
        uint32_t uWant = sizeof(pSha1->strData) - pSha1->uPartialCount;
        uint32_t uHave = uWant > uInputLen ? uInputLen : uWant;
        ds_memcpy(&pSha1->strData[pSha1->uPartialCount], pInput, uHave);
        pInput += uHave;
        uInputLen -= uHave;
        if (uHave == uWant)
        {
            _CryptSha1ProcessBlock(pSha1, pSha1->strData);
            pSha1->uCount += sizeof(pSha1->strData);
            pSha1->uPartialCount = 0;
        }
        else
        {
            pSha1->uPartialCount += uHave;
        }
    }
    while (uInputLen >= sizeof(pSha1->strData))
    {
        _CryptSha1ProcessBlock(pSha1, pInput);
        pSha1->uCount += sizeof(pSha1->strData);
        uInputLen -= sizeof(pSha1->strData);
        pInput += sizeof(pSha1->strData);
    }
    if (uInputLen != 0)
    {
        ds_memcpy(&pSha1->strData[pSha1->uPartialCount], pInput, uInputLen);
        pSha1->uPartialCount += uInputLen;
    }
}

/*F*******************************************************************/
/*!
    \Function CryptSha1Final

    \Description
        Generate the final hash from the SHA1 state

    \Input *pSha1   - the SHA1 state
    \Input *pBuffer - where the hash should be written
    \Input uLength  - the number of bytes to write, [0..CRYPTSHA1_HASHSIZE]

    \Output none

    \Notes
        Usually callers want the whole hash and so uOutputLen would be
        CRYPTSHA1_HASHSIZE.  However, if only a partial hash is needed then
        any value up to CRYPTSHA1_HASHSIZE can be used.  Any value
        greater than CRYPTSHA1_HASHSIZE is silently truncated to
        CRYPTSHA1_HASHSIZE.

        CryptSha1Final is not idempotent.  It could easily be made so but
        callers typically tend not to need it so it is not supported.

    \Version 1.0 06/06/2004 (sbevan) First Version
*/
/*******************************************************************F*/
void CryptSha1Final(CryptSha1T *pSha1, void *pBuffer, uint32_t uLength)
{
    uint32_t i;
    uint8_t uPad = 0x80;
    uint32_t uSpace = sizeof(pSha1->strData) - pSha1->uPartialCount;

    pSha1->uCount += pSha1->uPartialCount;
    if (uSpace < 9)
    {
        pSha1->strData[pSha1->uPartialCount] = uPad;
        for (i = pSha1->uPartialCount+1; i < sizeof(pSha1->strData); i += 1)
        {
            pSha1->strData[i] = 0x0;
        }
        _CryptSha1ProcessBlock(pSha1, pSha1->strData);
        uPad = 0x0;
        pSha1->uPartialCount = 0;
    }
    pSha1->strData[pSha1->uPartialCount] = uPad;
    for (i = pSha1->uPartialCount+1; i < sizeof(pSha1->strData)-8; i += 1)
    {
        pSha1->strData[i] = 0x0;
    }
    
    /* Append length in bits, as per RFC 3174 section 4.c except we only
       support a 32-bit byte length / 35-bit bit length.  Uses some
       bit shifting to avoid having to explicitly calculate
       pSha1->uCount*8 which could overflow 32-bits. */
    pSha1->strData[56] = 0;
    pSha1->strData[57] = 0;
    pSha1->strData[58] = 0;
    pSha1->strData[59] = (pSha1->uCount>>(32-3))&0xFF;
    pSha1->strData[60] = (pSha1->uCount>>(24-3))&0xFF;
    pSha1->strData[61] = (pSha1->uCount>>(16-3))&0xFF;
    pSha1->strData[62] = (pSha1->uCount>>(8-3))&0xFF;
    pSha1->strData[63] = (pSha1->uCount<<3)&0xFF;
    _CryptSha1ProcessBlock(pSha1, pSha1->strData);

    _CryptSha1CopyHash(pSha1, pBuffer, uLength);
}
