/*H*************************************************************************************************/
/*!

    \File    cryptmd5.c

    \Description
        The MD5 message digest algorithm developed by Ron Rivest and documented
        in RFC1321. This implementation is based on the RFC but does not use the
        sample code.. It should be free from intellectual property concerns and
        a reference is included below which further clarifies this point.

        Note that this implementation is limited to hashing no more than 2^32
        bytes after which its results would be impatible with a fully compliant
        implementation.

    \Notes
        http://www.ietf.org/ietf/IPR/RSA-MD-all

        The following was recevied Fenbruary 23,2000
        From: "Linn, John" <jlinn@rsasecurity.com>

        February 19, 2000

                The purpose of this memo is to clarify the status of intellectual
        property rights asserted by RSA Security Inc. ("RSA") in the MD2, MD4 and
        MD5 message-digest algorithms, which are documented in RFC-1319, RFC-1320,
        and RFC-1321 respectively.

                Implementations of these message-digest algorithms, including
        implementations derived from the reference C code in RFC-1319, RFC-1320, and
        RFC-1321, may be made, used, and sold without license from RSA for any
        purpose.

                No rights other than the ones explicitly set forth above are
        granted.  Further, although RSA grants rights to implement certain
        algorithms as defined by identified RFCs, including implementations derived
        from the reference C code in those RFCs, no right to use, copy, sell, or
        distribute any other implementations of the MD2, MD4, or MD5 message-digest
        algorithms created, implemented, or distributed by RSA is hereby granted by
        implication, estoppel, or otherwise.  Parties interested in licensing
        security components and toolkits written by RSA should contact the company
        to discuss receiving a license.  All other questions should be directed to
        Margaret K. Seif, General Counsel, RSA Security Inc., 36 Crosby Drive,
        Bedford, Massachusetts 01730.

                Implementations of the MD2, MD4, or MD5 algorithms may be subject to
        United States laws and regulations controlling the export of technical data,
        computer software, laboratory prototypes and other commodities (including
        the Arms Export Control Act, as amended, and the Export Administration Act
        of 1970).  The transfer of certain technical data and commodities may
        require a license from the cognizant agency of the United States Government.
        RSA neither represents that a license shall not be required for a particular
        implementation nor that, if required, one shall be issued.


                DISCLAIMER: RSA MAKES NO REPRESENTATIONS AND EXTENDS NO WARRANTIES
        OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
        WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, VALIDITY OF
        INTELLECTUAL PROPERTY RIGHTS, ISSUED OR PENDING, OR THE ABSENCE OF LATENT OR
        OTHER DEFECTS, WHETHER OR NOT DISCOVERABLE, IN CONNECTION WITH THE MD2, MD4,
        OR MD5 ALGORITHMS.  NOTHING IN THIS GRANT OF RIGHTS SHALL BE CONSTRUED AS A
        REPRESENTATION OR WARRANTY GIVEN BY RSA THAT THE IMPLEMENTATION OF THE
        ALGORITHM WILL NOT INFRINGE THE INTELLECTUAL PROPERTY RIGHTS OF ANY THIRD
        PARTY.  IN NO EVENT SHALL RSA, ITS TRUSTEES, DIRECTORS, OFFICERS, EMPLOYEES,
        PARENTS AND AFFILIATES BE LIABLE FOR INCIDENTAL OR CONSEQUENTIAL DAMAGES OF
        ANY KIND RESULTING FROM IMPLEMENTATION OF THIS ALGORITHM, INCLUDING ECONOMIC
        DAMAGE OR INJURY TO PROPERTY AND LOST PROFITS, REGARDLESS OF WHETHER RSA
        SHALL BE ADVISED, SHALL HAVE OTHER REASON TO KNOW, OR IN FACT SHALL KNOW OF

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002.  ALL RIGHTS RESERVED.

    \Version    1.0        03/16/2001 (GWS) First Version

*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <string.h>
#include "DirtySDK/platform.h"
#include "DirtySDK/crypt/cryptmd5.h"

/*** Defines ***************************************************************************/

// the core transformations (ff/gg slightly optimized)
#define FF(x, y, z) (z ^ ((y ^ z) & x))
#define GG(x, y, z) (y ^ ((x ^ y) & z))
#define HH(x, y, z) (x ^ y ^ z)
#define II(x, y, z) (y ^ (x | ~z))

// accumulate the result of the transformation
#define ACC(r, t1, t2, t3, s, x) \
    r += (t1); r += (t2); r+= (t3); r = (r<<s) | (r>>(32-s)); r += x;

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

static const char *_MD5_HexChars = "0123456789abcdef";

// Public variables


/*** Private Functions *****************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    _CryptMD5Transform

    \Description
        Incorpate 512 bit data block into current context

    \Input *pContext    - target MD5 context
    \Input *pBlock      - pointer to data block

    \Version 03/16/02 (GWS)
*/
/*************************************************************************************************F*/
static void _CryptMD5Transform(CryptMD5T *pContext, const unsigned char *pBlock)
{
    register uint32_t a;
    register uint32_t b;
    register uint32_t c;
    register uint32_t d;
    uint32_t uData[16];
    // not really volatile, but keeps gcc optimizer from
    // going crazy and creating really slow code
    volatile uint32_t *pVector = (uint32_t *)pBlock;

    // this is not actually required on a little-endian
    // machine like intel, but gcc screws up the code so
    // badly without it that its faster with this
    // unneeded reordering code than without it.
    if (1)
    {
        // convert the incoming data stream to little endian
        pBlock += 64;
        pVector = uData+16;
        while (pVector != uData)
        {
            // go from end to start since data is in little endian
            // (this is the most efficient method to grab the data)
            b = *--pBlock;
            b = (b << 8) + *--pBlock;
            b = (b << 8) + *--pBlock;
            b = (b << 8) + *--pBlock;
            *--pVector = b;
        }
    }

    // load the register values
    a = pContext->uRegs[0];
    b = pContext->uRegs[1];
    c = pContext->uRegs[2];
    d = pContext->uRegs[3];

    // this magic comes from the rfc
    ACC(a, FF(b, c, d), pVector[ 0],0xd76aa478,  7, b);
    ACC(d, FF(a, b, c), pVector[ 1],0xe8c7b756, 12, a);
    ACC(c, FF(d, a, b), pVector[ 2],0x242070db, 17, d);
    ACC(b, FF(c, d, a), pVector[ 3],0xc1bdceee, 22, c);
    ACC(a, FF(b, c, d), pVector[ 4],0xf57c0faf,  7, b);
    ACC(d, FF(a, b, c), pVector[ 5],0x4787c62a, 12, a);
    ACC(c, FF(d, a, b), pVector[ 6],0xa8304613, 17, d);
    ACC(b, FF(c, d, a), pVector[ 7],0xfd469501, 22, c);
    ACC(a, FF(b, c, d), pVector[ 8],0x698098d8,  7, b);
    ACC(d, FF(a, b, c), pVector[ 9],0x8b44f7af, 12, a);
    ACC(c, FF(d, a, b), pVector[10],0xffff5bb1, 17, d);
    ACC(b, FF(c, d, a), pVector[11],0x895cd7be, 22, c);
    ACC(a, FF(b, c, d), pVector[12],0x6b901122,  7, b);
    ACC(d, FF(a, b, c), pVector[13],0xfd987193, 12, a);
    ACC(c, FF(d, a, b), pVector[14],0xa679438e, 17, d);
    ACC(b, FF(c, d, a), pVector[15],0x49b40821, 22, c);

    ACC(a, GG(b, c, d), pVector[ 1],0xf61e2562,  5, b);
    ACC(d, GG(a, b, c), pVector[ 6],0xc040b340,  9, a);
    ACC(c, GG(d, a, b), pVector[11],0x265e5a51, 14, d);
    ACC(b, GG(c, d, a), pVector[ 0],0xe9b6c7aa, 20, c);
    ACC(a, GG(b, c, d), pVector[ 5],0xd62f105d,  5, b);
    ACC(d, GG(a, b, c), pVector[10],0x02441453,  9, a);
    ACC(c, GG(d, a, b), pVector[15],0xd8a1e681, 14, d);
    ACC(b, GG(c, d, a), pVector[ 4],0xe7d3fbc8, 20, c);
    ACC(a, GG(b, c, d), pVector[ 9],0x21e1cde6,  5, b);
    ACC(d, GG(a, b, c), pVector[14],0xc33707d6,  9, a);
    ACC(c, GG(d, a, b), pVector[ 3],0xf4d50d87, 14, d);
    ACC(b, GG(c, d, a), pVector[ 8],0x455a14ed, 20, c);
    ACC(a, GG(b, c, d), pVector[13],0xa9e3e905,  5, b);
    ACC(d, GG(a, b, c), pVector[ 2],0xfcefa3f8,  9, a);
    ACC(c, GG(d, a, b), pVector[ 7],0x676f02d9, 14, d);
    ACC(b, GG(c, d, a), pVector[12],0x8d2a4c8a, 20, c);

    ACC(a, HH(b, c, d), pVector[ 5],0xfffa3942,  4, b);
    ACC(d, HH(a, b, c), pVector[ 8],0x8771f681, 11, a);
    ACC(c, HH(d, a, b), pVector[11],0x6d9d6122, 16, d);
    ACC(b, HH(c, d, a), pVector[14],0xfde5380c, 23, c);
    ACC(a, HH(b, c, d), pVector[ 1],0xa4beea44,  4, b);
    ACC(d, HH(a, b, c), pVector[ 4],0x4bdecfa9, 11, a);
    ACC(c, HH(d, a, b), pVector[ 7],0xf6bb4b60, 16, d);
    ACC(b, HH(c, d, a), pVector[10],0xbebfbc70, 23, c);
    ACC(a, HH(b, c, d), pVector[13],0x289b7ec6,  4, b);
    ACC(d, HH(a, b, c), pVector[ 0],0xeaa127fa, 11, a);
    ACC(c, HH(d, a, b), pVector[ 3],0xd4ef3085, 16, d);
    ACC(b, HH(c, d, a), pVector[ 6],0x04881d05, 23, c);
    ACC(a, HH(b, c, d), pVector[ 9],0xd9d4d039,  4, b);
    ACC(d, HH(a, b, c), pVector[12],0xe6db99e5, 11, a);
    ACC(c, HH(d, a, b), pVector[15],0x1fa27cf8, 16, d);
    ACC(b, HH(c, d, a), pVector[ 2],0xc4ac5665, 23, c);

    ACC(a, II(b, c, d), pVector[ 0],0xf4292244,  6, b);
    ACC(d, II(a, b, c), pVector[ 7],0x432aff97, 10, a);
    ACC(c, II(d, a, b), pVector[14],0xab9423a7, 15, d);
    ACC(b, II(c, d, a), pVector[ 5],0xfc93a039, 21, c);
    ACC(a, II(b, c, d), pVector[12],0x655b59c3,  6, b);
    ACC(d, II(a, b, c), pVector[ 3],0x8f0ccc92, 10, a);
    ACC(c, II(d, a, b), pVector[10],0xffeff47d, 15, d);
    ACC(b, II(c, d, a), pVector[ 1],0x85845dd1, 21, c);
    ACC(a, II(b, c, d), pVector[ 8],0x6fa87e4f,  6, b);
    ACC(d, II(a, b, c), pVector[15],0xfe2ce6e0, 10, a);
    ACC(c, II(d, a, b), pVector[ 6],0xa3014314, 15, d);
    ACC(b, II(c, d, a), pVector[13],0x4e0811a1, 21, c);
    ACC(a, II(b, c, d), pVector[ 4],0xf7537e82,  6, b);
    ACC(d, II(a, b, c), pVector[11],0xbd3af235, 10, a);
    ACC(c, II(d, a, b), pVector[ 2],0x2ad7d2bb, 15, d);
    ACC(b, II(c, d, a), pVector[ 9],0xeb86d391, 21, c);

    // update the registers
    pContext->uRegs[0] += a;
    pContext->uRegs[1] += b;
    pContext->uRegs[2] += c;
    pContext->uRegs[3] += d;
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    CryptMD5Init

    \Description
        Init the MD5 context.

    \Input *pContext    - target MD5 context

    \Version 03/16/02 (GWS)
*/
/*************************************************************************************************F*/
void CryptMD5Init(CryptMD5T *pContext)
{
    // reset the byte count
    pContext->uCount = 0;

    // init as per RFC1321
    pContext->uRegs[0] = 0x67452301;  // word A
    pContext->uRegs[1] = 0xefcdab89;  // word B
    pContext->uRegs[2] = 0x98badcfe;  // word C
    pContext->uRegs[3] = 0x10325476;  // word D
}

/*F*************************************************************************************************/
/*!
    \Function    CryptMD5Init2

    \Description
        Init the MD5 context (alternate form)

    \Input *pContext    - target MD5 context
    \Input iHashSize    - hash size (unused)

    \Version 11/05/13 (jbrookes)
*/
/*************************************************************************************************F*/
void CryptMD5Init2(CryptMD5T *pContext, int32_t iHashSize)
{
    CryptMD5Init(pContext);
}

/*F*************************************************************************************************/
/*!
    \Function    CryptMD5Update

    \Description
        Add data to the MD5 context (hash the data).

    \Input *pContext    - target MD5 context
    \Input *_pBuffer    - input data to hash
    \Input iLength      - length of buffer (-1=treat pBuffer as asciiz)

    \Version 03/16/02 (GWS)
*/
/*************************************************************************************************F*/
void CryptMD5Update(CryptMD5T *pContext, const void *_pBuffer, int32_t iLength)
{
    int32_t uAdd;
    uint32_t uCount;
    const unsigned char *pBuffer = _pBuffer;

    // allow easy string access
    if (iLength < 0)
    {
        for (iLength = 0; pBuffer[iLength] != 0; ++iLength)
            ;
    }

    // get index into block buffer
    uCount = pContext->uCount&63;
    pContext->uCount += iLength;

    // see if we need to append to existing data
    if (uCount > 0)
    {
        // figure out number to fill block
        uAdd = 64-uCount;

        // if less than a full block
        if (iLength < uAdd)
        {
            ds_memcpy(pContext->strData+uCount, pBuffer, iLength);
            return;
        }

        // finish off the block and transform
        ds_memcpy(pContext->strData+uCount, pBuffer, uAdd);
        pBuffer += uAdd;
        iLength -= uAdd;
        _CryptMD5Transform(pContext, pContext->strData);
    }

    // do 64 byte blocks of data
    while (iLength >= 64)
    {
        _CryptMD5Transform(pContext, pBuffer);
        pBuffer += 64;
        iLength -= 64;
    }

    // store leftover data
    if (iLength > 0)
    {
        ds_memcpy(pContext->strData, pBuffer, iLength);
    }
}

/*F*************************************************************************************************/
/*!
    \Function    CryptMD5Final

    \Description
        Convert MD5 state into final output form

    \Input *pContext    - the MD5 state (from create)
    \Input *_pBuffer    - the digest output
    \Input iLength      - length of output buffer

    \Version 03/16/02 (GWS)
*/
/*************************************************************************************************F*/
void CryptMD5Final(CryptMD5T *pContext, void *_pBuffer, int32_t iLength)
{
    int32_t uIndex;
    uint32_t uZero;
    uint32_t *pZero;
    uint32_t uData = 0;
    unsigned char *pBuffer = _pBuffer;

    // add ending marker
    uIndex = pContext->uCount & 63;
    pContext->strData[uIndex++] = 0x80;

    // transform block if no room for length data
    if (uIndex > 56)
    {
        // zero rest of the buffer
        // (we have 8 extra bytes so this can run over)
        pContext->strData[uIndex+0] = 0;
        pContext->strData[uIndex+1] = 0;
        pContext->strData[uIndex+2] = 0;
        pContext->strData[uIndex+3] = 0;
        pContext->strData[uIndex+4] = 0;
        pContext->strData[uIndex+5] = 0;
        pContext->strData[uIndex+6] = 0;
        pContext->strData[uIndex+7] = 0;
        // transform the block
        _CryptMD5Transform(pContext, pContext->strData);
        uIndex = 0;
    }

    // force zero to next int32_t
    pContext->strData[uIndex+0] = 0;
    pContext->strData[uIndex+1] = 0;
    pContext->strData[uIndex+2] = 0;
    // zero to end of block
    uZero = (uIndex+3)>>2;
    pZero = ((uint32_t *)pContext->strData)+uZero;
    do
    {
        *pZero++ = 0;
    }
    while (++uZero < 64/4);

    // setup length mask
    pContext->strData[56] = (unsigned char)(pContext->uCount<<3);
    pContext->strData[57] = (unsigned char)(pContext->uCount>>5);
    pContext->strData[58] = (unsigned char)(pContext->uCount>>13);
    pContext->strData[59] = (unsigned char)(pContext->uCount>>21);
    pContext->strData[60] = (unsigned char)(pContext->uCount>>29);
    // final transformation
    _CryptMD5Transform(pContext, pContext->strData);

#ifdef __linux__
    // fast output of binary data in linux (more memory)
    if (iLength == MD5_BINARY_OUT/2)
    {
        uData = pContext->uRegs[0];
        pBuffer[0] = uData;
        uData >>= 8;
        pBuffer[1] = uData;
        uData >>= 8;
        pBuffer[2] = uData;
        uData >>= 8;
        pBuffer[3] = uData;

        uData = pContext->uRegs[1];
        pBuffer[4] = uData;
        uData >>= 8;
        pBuffer[5] = uData;
        uData >>= 8;
        pBuffer[6] = uData;
        uData >>= 8;
        pBuffer[7] = uData;
        return;
    }

    if (iLength == MD5_BINARY_OUT)
    {
        uData = pContext->uRegs[0];
        pBuffer[0] = uData;
        uData >>= 8;
        pBuffer[1] = uData;
        uData >>= 8;
        pBuffer[2] = uData;
        uData >>= 8;
        pBuffer[3] = uData;

        uData = pContext->uRegs[1];
        pBuffer[4] = uData;
        uData >>= 8;
        pBuffer[5] = uData;
        uData >>= 8;
        pBuffer[6] = uData;
        uData >>= 8;
        pBuffer[7] = uData;

        uData = pContext->uRegs[2];
        pBuffer[8] = uData;
        uData >>= 8;
        pBuffer[9] = uData;
        uData >>= 8;
        pBuffer[10] = uData;
        uData >>= 8;
        pBuffer[11] = uData;

        uData = pContext->uRegs[3];
        pBuffer[12] = uData;
        uData >>= 8;
        pBuffer[13] = uData;
        uData >>= 8;
        pBuffer[14] = uData;
        uData >>= 8;
        pBuffer[15] = uData;
        return;
    }
#endif

    // extract data from buffer and save
    for (uIndex = 0; uIndex < 16; ++uIndex)
    {
        // see if we need to fetch a byte
        if ((uIndex & 3) == 0)
        {
            uData = pContext->uRegs[uIndex>>2];
        }
        // store the byte
        if (iLength >= MD5_STRING_OUT)
        {
            *pBuffer++ = _MD5_HexChars[(uData>>4)&15];
            *pBuffer++ = _MD5_HexChars[(uData>>0)&15];
        }
        else if (uIndex < iLength)
        {
            *pBuffer++ = (unsigned char)uData;
        }
        // shift down the data
        uData >>= 8;
    }
    
    // add final terminator if needed
    if (iLength >= MD5_STRING_OUT)
    {
        *pBuffer = 0;
    }
}

