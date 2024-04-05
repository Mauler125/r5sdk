/*H********************************************************************************/
/*!
    \File    hpack.c

    \Description
        This module implements a decode/encoder based on the HPACK spec
        (https://tools.ietf.org/html/rfc7541). Which is used for encoding/decoding
        the HEADERS frame in the HTTP/2 protocol.

    \Copyright
        Copyright (c) Electronic Arts 2016. ALL RIGHTS RESERVED.
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/proto/protohttputil.h"
#include "DirtySDK/util/hpack.h"

/*** Defines **********************************************************************/

//! size of the static table
#define HPACK_STATIC_TABLE_SIZE (61)

//! size of huffman code table
#define HPACK_HUFFMAN_TABLE_SIZE (256)

//! overhead for a table entry
#define HPACK_TABLE_ENTRY_OVERHEAD (32)

//! controls if we log the dynamic table info
#define HPACK_DYNAMICTABLE_LOGGING (DIRTYCODE_LOGGING && FALSE)

//! flags for different header field types
#define HPACK_INDEXED_HEADER_FIELD          (1 << 7)
#define HPACK_LITERAL_HEADER_FIELD          (1 << 6)
#define HPACK_DYNAMIC_TABLE_UPDATE          (1 << 5)
#define HPACK_LITERAL_HEADER_FIELD_NEVER    (1 << 4)

/*** Type Definitions *************************************************************/

//! defines an entry in the dynamic table
typedef struct TableEntryT
{
    char *pName;    //!< header name
    char *pValue;   //!< header value
} TableEntryT;

//! defines an entry in the static table
typedef struct StaticTableEntryT
{
    const char *pName;  //!< header name
    const char *pValue; //!< header value
} StaticTableEntryT;

//! linked list node we use to define our dynamic table
typedef struct DynamicNodeT
{
    struct DynamicNodeT *pNext;
    struct DynamicNodeT *pPrev;

    TableEntryT Entry;
} DynamicNodeT;

//! entry in the huffman tree
typedef struct HuffmanCodeT
{
    uint32_t uCode; //!< the huffman code
    uint32_t uLen;   //!< the length of the code in bits
} HuffmanCodeT;

//! tree used for huffman decoding
typedef struct HuffmanNodeT
{
    struct HuffmanNodeT *pLeft;     //!< 1 bit
    struct HuffmanNodeT *pRight;    //!< 0 bit

    uint8_t uChar;                  //!< the character represented by the sequence
    uint8_t _pad[3];
} HuffmanNodeT;

//! data used to keep track of the huffman encoding
typedef struct HuffmanEncodeT
{
    uint32_t aResult[256];
    uint32_t uCount;
    uint32_t uNextBoundary;
} HuffmanEncodeT;

//! module ref for our encoder/decoder
struct HpackRefT
{
    DynamicNodeT *pHead;    //!< points to the head of the dynamic table (for insertion/lookup)
    DynamicNodeT *pTail;    //!< points to the tail of the dynamic table (for removal)

    uint32_t uTableSize;    //!< current size of the dynamic table
    uint32_t uTableMax;     //!< maximum size of the dynamic table

    HuffmanNodeT *pHuffmanTree; //!< huffman tree used for decoding

    //! memgroup data
    int32_t iMemGroup;
    void *pMemGroupUserData;

    uint8_t bUpdatePending; //!< there is a pending update to the dynamic table
    uint8_t _pad[3];
};

/*** Variables ********************************************************************/

//! our fixed static table
static const StaticTableEntryT Hpack_aStaticTable[HPACK_STATIC_TABLE_SIZE] =
{
    { ":authority", "" },
    { ":method", "GET" },
    { ":method", "POST" },
    { ":path", "/" },
    { ":path", "/index.html" },
    { ":scheme", "http" },
    { ":scheme", "https" },
    { ":status", "200" },
    { ":status", "204" },
    { ":status", "206" },
    { ":status", "304" },
    { ":status", "400" },
    { ":status", "404" },
    { ":status", "500" },
    { "accept-charset", "" },
    { "accept-encoding", "gzip, deflate" },
    { "accept-language", "" },
    { "accept-ranges", "" },
    { "accept", "" },
    { "access-control-allow-origin", "" },
    { "age", "" },
    { "allow", "" },
    { "authorization", "" },
    { "cache-control", "" },
    { "content-disposition", "" },
    { "content-encoding", "" },
    { "content-language", "" },
    { "content-length", "" },
    { "content-location", "" },
    { "content-range", "" },
    { "content-type", "" },
    { "cookie", "" },
    { "date", "" },
    { "etag", "" },
    { "expect", "" },
    { "expires", "" },
    { "from", "" },
    { "host", "" },
    { "if-match", "" },
    { "if-modified-since", "" },
    { "if-none-match", "" },
    { "if-range", "" },
    { "if-unmodified-since", "" },
    { "last-modified", "" },
    { "link", "" },
    { "location", "" },
    { "max-forwards", "" },
    { "proxy-authenticate", "" },
    { "proxy-authorization", "" },
    { "range", "" },
    { "referer", "" },
    { "refresh", "" },
    { "retry-after", "" },
    { "server", "" },
    { "set-cookie", "" },
    { "strict-transport-security", "" },
    { "transfer-encoding", "" },
    { "user-agent", "" },
    { "vary", "" },
    { "via", "" },
    { "www-authenticate", "" }
};

//! table of huffman codes used for encoding / decoding based on
//! https://tools.ietf.org/html/rfc7541#appendix-B
static const HuffmanCodeT Hpack_aHuffmanCodes[256] =
{
    { 0x00001ff8, 13 }, { 0x007fffd8, 23 }, { 0x0fffffe2, 28 }, { 0x0fffffe3, 28 }, { 0x0fffffe4, 28 }, { 0x0fffffe5, 28 }, { 0x0fffffe6, 28 }, { 0x0fffffe7, 28 }, /*   0 -   7 */
    { 0x0fffffe8, 28 }, { 0x00ffffea, 24 }, { 0x3ffffffc, 30 }, { 0x0fffffe9, 28 }, { 0x0fffffea, 28 }, { 0x3ffffffd, 30 }, { 0x0fffffeb, 28 }, { 0x0fffffec, 28 }, /*   8 -  15 */
    { 0x0fffffed, 28 }, { 0x0fffffee, 28 }, { 0x0fffffef, 28 }, { 0x0ffffff0, 28 }, { 0x0ffffff1, 28 }, { 0x0ffffff2, 28 }, { 0x3ffffffe, 30 }, { 0x0ffffff3, 28 }, /*  16 -  23 */
    { 0x0ffffff4, 28 }, { 0x0ffffff5, 28 }, { 0x0ffffff6, 28 }, { 0x0ffffff7, 28 }, { 0x0ffffff8, 28 }, { 0x0ffffff9, 28 }, { 0x0ffffffa, 28 }, { 0x0ffffffb, 28 }, /*  24 -  31 */
    { 0x00000014,  6 }, { 0x000003f8, 10 }, { 0x000003f9, 10 }, { 0x00000ffa, 12 }, { 0x00001ff9, 13 }, { 0x00000015,  6 }, { 0x000000f8,  8 }, { 0x000007fa, 11 }, /*  32 -  39 */
    { 0x000003fa, 10 }, { 0x000003fb, 10 }, { 0x000000f9,  8 }, { 0x000007fb, 11 }, { 0x000000fa,  8 }, { 0x00000016,  6 }, { 0x00000017,  6 }, { 0x00000018,  6 }, /*  40 -  47 */
    { 0x00000000,  5 }, { 0x00000001,  5 }, { 0x00000002,  5 }, { 0x00000019,  6 }, { 0x0000001a,  6 }, { 0x0000001b,  6 }, { 0x0000001c,  6 }, { 0x0000001d,  6 }, /*  48 -  55 */
    { 0x0000001e,  6 }, { 0x0000001f,  6 }, { 0x0000005c,  7 }, { 0x000000fb,  8 }, { 0x00007ffc, 15 }, { 0x00000020,  6 }, { 0x00000ffb, 12 }, { 0x000003fc, 10 }, /*  56 -  63 */
    { 0x00001ffa, 13 }, { 0x00000021,  6 }, { 0x0000005d,  7 }, { 0x0000005e,  7 }, { 0x0000005f,  7 }, { 0x00000060,  7 }, { 0x00000061,  7 }, { 0x00000062,  7 }, /*  64 -  71 */
    { 0x00000063,  7 }, { 0x00000064,  7 }, { 0x00000065,  7 }, { 0x00000066,  7 }, { 0x00000067,  7 }, { 0x00000068,  7 }, { 0x00000069,  7 }, { 0x0000006a,  7 }, /*  72 -  79 */
    { 0x0000006b,  7 }, { 0x0000006c,  7 }, { 0x0000006d,  7 }, { 0x0000006e,  7 }, { 0x0000006f,  7 }, { 0x00000070,  7 }, { 0x00000071,  7 }, { 0x00000072,  7 }, /*  80 -  87 */
    { 0x000000fc,  8 }, { 0x00000073,  7 }, { 0x000000fd,  8 }, { 0x00001ffb, 13 }, { 0x0007fff0, 19 }, { 0x00001ffc, 13 }, { 0x00003ffc, 14 }, { 0x00000022,  6 }, /*  88 -  95 */
    { 0x00007ffd, 15 }, { 0x00000003,  5 }, { 0x00000023,  6 }, { 0x00000004,  5 }, { 0x00000024,  6 }, { 0x00000005,  5 }, { 0x00000025,  6 }, { 0x00000026,  6 }, /*  96 - 103 */
    { 0x00000027,  6 }, { 0x00000006,  5 }, { 0x00000074,  7 }, { 0x00000075,  7 }, { 0x00000028,  6 }, { 0x00000029,  6 }, { 0x0000002a,  6 }, { 0x00000007,  5 }, /* 104 - 111 */
    { 0x0000002b,  6 }, { 0x00000076,  7 }, { 0x0000002c,  6 }, { 0x00000008,  5 }, { 0x00000009,  5 }, { 0x0000002d,  6 }, { 0x00000077,  7 }, { 0x00000078,  7 }, /* 112 - 119 */
    { 0x00000079,  7 }, { 0x0000007a,  7 }, { 0x0000007b,  7 }, { 0x00007ffe, 15 }, { 0x000007fc, 11 }, { 0x00003ffd, 14 }, { 0x00001ffd, 13 }, { 0x0ffffffc, 28 }, /* 120 - 127 */
    { 0x000fffe6, 20 }, { 0x003fffd2, 22 }, { 0x000fffe7, 20 }, { 0x000fffe8, 20 }, { 0x003fffd3, 22 }, { 0x003fffd4, 22 }, { 0x003fffd5, 22 }, { 0x007fffd9, 23 }, /* 128 - 135 */
    { 0x003fffd6, 22 }, { 0x007fffda, 23 }, { 0x007fffdb, 23 }, { 0x007fffdc, 23 }, { 0x007fffdd, 23 }, { 0x007fffde, 23 }, { 0x00ffffeb, 24 }, { 0x007fffdf, 23 }, /* 136 - 143 */
    { 0x00ffffec, 24 }, { 0x00ffffed, 24 }, { 0x003fffd7, 22 }, { 0x007fffe0, 23 }, { 0x00ffffee, 24 }, { 0x007fffe1, 23 }, { 0x007fffe2, 23 }, { 0x007fffe3, 23 }, /* 144 - 151 */
    { 0x007fffe4, 23 }, { 0x001fffdc, 21 }, { 0x003fffd8, 22 }, { 0x007fffe5, 23 }, { 0x003fffd9, 22 }, { 0x007fffe6, 23 }, { 0x007fffe7, 23 }, { 0x00ffffef, 24 }, /* 152 - 159 */
    { 0x003fffda, 22 }, { 0x001fffdd, 21 }, { 0x000fffe9, 20 }, { 0x003fffdb, 22 }, { 0x003fffdc, 22 }, { 0x007fffe8, 23 }, { 0x007fffe9, 23 }, { 0x001fffde, 21 }, /* 160 - 167 */
    { 0x007fffea, 23 }, { 0x003fffdd, 22 }, { 0x003fffde, 22 }, { 0x00fffff0, 24 }, { 0x001fffdf, 21 }, { 0x003fffdf, 22 }, { 0x007fffeb, 23 }, { 0x007fffec, 23 }, /* 168 - 175 */
    { 0x001fffe0, 21 }, { 0x001fffe1, 21 }, { 0x003fffe0, 22 }, { 0x001fffe2, 21 }, { 0x007fffed, 23 }, { 0x003fffe1, 22 }, { 0x007fffee, 23 }, { 0x007fffef, 23 }, /* 176 - 183 */
    { 0x000fffea, 20 }, { 0x003fffe2, 22 }, { 0x003fffe3, 22 }, { 0x003fffe4, 22 }, { 0x007ffff0, 23 }, { 0x003fffe5, 22 }, { 0x003fffe6, 22 }, { 0x007ffff1, 23 }, /* 184 - 191 */
    { 0x03ffffe0, 26 }, { 0x03ffffe1, 26 }, { 0x000fffeb, 20 }, { 0x0007fff1, 19 }, { 0x003fffe7, 22 }, { 0x007ffff2, 23 }, { 0x003fffe8, 22 }, { 0x01ffffec, 25 }, /* 192 - 199 */
    { 0x03ffffe2, 26 }, { 0x03ffffe3, 26 }, { 0x03ffffe4, 26 }, { 0x07ffffde, 27 }, { 0x07ffffdf, 27 }, { 0x03ffffe5, 26 }, { 0x00fffff1, 24 }, { 0x01ffffed, 25 }, /* 200 - 207 */
    { 0x0007fff2, 19 }, { 0x001fffe3, 21 }, { 0x03ffffe6, 26 }, { 0x07ffffe0, 27 }, { 0x07ffffe1, 27 }, { 0x03ffffe7, 26 }, { 0x07ffffe2, 27 }, { 0x00fffff2, 24 }, /* 208 - 215 */
    { 0x001fffe4, 21 }, { 0x001fffe5, 21 }, { 0x03ffffe8, 26 }, { 0x03ffffe9, 26 }, { 0x0ffffffd, 28 }, { 0x07ffffe3, 27 }, { 0x07ffffe4, 27 }, { 0x07ffffe5, 27 }, /* 216 - 223 */
    { 0x000fffec, 20 }, { 0x00fffff3, 24 }, { 0x000fffed, 20 }, { 0x001fffe6, 21 }, { 0x003fffe9, 22 }, { 0x001fffe7, 21 }, { 0x001fffe8, 21 }, { 0x007ffff3, 23 }, /* 224 - 231 */
    { 0x003fffea, 22 }, { 0x003fffeb, 22 }, { 0x01ffffee, 25 }, { 0x01ffffef, 25 }, { 0x00fffff4, 24 }, { 0x00fffff5, 24 }, { 0x03ffffea, 26 }, { 0x007ffff4, 23 }, /* 232 - 239 */
    { 0x03ffffeb, 26 }, { 0x07ffffe6, 27 }, { 0x03ffffec, 26 }, { 0x03ffffed, 26 }, { 0x07ffffe7, 27 }, { 0x07ffffe8, 27 }, { 0x07ffffe9, 27 }, { 0x07ffffea, 27 }, /* 240 - 247 */
    { 0x07ffffeb, 27 }, { 0x0ffffffe, 28 }, { 0x07ffffec, 27 }, { 0x07ffffed, 27 }, { 0x07ffffee, 27 }, { 0x07ffffef, 27 }, { 0x07fffff0, 27 }, { 0x03ffffee, 26 }  /* 248 - 255 */
};

// EOS (End of String) specifier
static const HuffmanCodeT Hpack_EOS = { 0x3fffffff, 30 };

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _HpackDynamicTableGet

    \Description
        Attempts to find a table entry in the static and dynamic tables

    \Input *pRef  - module state
    \Input iIndex - the index of the table entry we are looking for

    \Output
        const TableEntryT * - the found entry or NULL

    \Version 10/05/2016 (eesponda)
*/
/********************************************************************************F*/
static const TableEntryT *_HpackDynamicTableGet(HpackRefT *pRef, int32_t iIndex)
{
    int32_t iCount;
    const DynamicNodeT *pNode;

    // loop through the list either finding our index or hitting the end, if index is found return the entry
    for (pNode = pRef->pHead, iCount = 0; pNode != NULL; pNode = pNode->pNext, iCount += 1)
    {
        if (iCount == iIndex)
        {
            break;
        }
    }
    return(pNode != NULL ? &pNode->Entry : NULL);
}

/*F********************************************************************************/
/*!
    \Function _HpackHuffmanTreeInsert

    \Description
        Inserts a character into the huffman tree recursively

    \Input *pRef    - module state
    \Input **pNode  - current node we are visiting
    \Input uCode    - huffman code
    \Input uLen     - number of bits in the code
    \Input uChar    - character corresponding to huffman code

    \Output
        uint8_t     - TRUE if successfully, FALSE otherwise

    \Version 10/21/2016 (eesponda)
*/
/********************************************************************************F*/
static uint8_t _HpackHuffmanTreeInsert(HpackRefT *pRef, HuffmanNodeT **pNode, uint32_t uCode, uint32_t uLen, uint8_t uChar)
{
    HuffmanNodeT **pTargetNode = ((uCode >> (uLen-1)) & 1) ? &(*pNode)->pLeft : &(*pNode)->pRight;
    if (*pTargetNode == NULL)
    {
        if ((*pTargetNode = (HuffmanNodeT *)DirtyMemAlloc(sizeof(**pTargetNode), HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) == NULL)
        {
            NetPrintf(("hpack: [%p] could not allocate node for huffman tree\n", pRef));
            return(FALSE);
        }
        ds_memclr(*pTargetNode, sizeof(**pTargetNode));
    }

    if ((uLen-1) > 0)
    {
        return(_HpackHuffmanTreeInsert(pRef, pTargetNode, uCode, uLen-1, uChar));
    }
    else
    {
        (*pTargetNode)->uChar = uChar;
        return(TRUE);
    }
}

/*F********************************************************************************/
/*!
    \Function _HpackHuffmanTreeBuild

    \Description
        Builds the huffman tree based on the static huffman code table

    \Input *pRef    - module state

    \Output
        uint8_t     - TRUE if successfully, FALSE otherwise

    \Version 10/19/2016 (eesponda)
*/
/********************************************************************************F*/
static uint8_t _HpackHuffmanTreeBuild(HpackRefT *pRef)
{
    uint8_t uChar, bResult = TRUE;

    // allocate the root of the tree
    if ((pRef->pHuffmanTree = (HuffmanNodeT *)DirtyMemAlloc(sizeof(*pRef->pHuffmanTree), HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("hpack: [%p] could not allocate root of huffman tree\n", pRef));
        return(FALSE);
    }
    ds_memclr(pRef->pHuffmanTree, sizeof(*pRef->pHuffmanTree));

    /* go through each character one at a time 0-255
       building a path in the huffman tree based on the code for the character
       at the end save the character in the leaf

       if any allocation fails this will be all cleaned up in the destroy that
       happens if this function returns FALSE
    */
    for (uChar = 0; (uChar < HPACK_HUFFMAN_TABLE_SIZE-1) && (bResult == TRUE); uChar += 1)
    {
        const HuffmanCodeT *pHuffman = &Hpack_aHuffmanCodes[uChar];
        bResult = _HpackHuffmanTreeInsert(pRef, &pRef->pHuffmanTree, pHuffman->uCode, pHuffman->uLen, uChar);
    }

    return(bResult);
}

/*F********************************************************************************/
/*!
    \Function _HpackHuffmanTreeDestroy

    \Description
        Recursively clean up the huffman tree

    \Input *pRef    - module state
    \Input *pNode   - currently visited node to cleanup

    \Version 10/19/2016 (eesponda)
*/
/********************************************************************************F*/
static void _HpackHuffmanTreeDestroy(HpackRefT *pRef, HuffmanNodeT *pNode)
{
    if (pNode == NULL)
    {
        return;
    }

    _HpackHuffmanTreeDestroy(pRef, pNode->pLeft);
    pNode->pLeft = NULL;

    _HpackHuffmanTreeDestroy(pRef, pNode->pRight);
    pNode->pRight = NULL;

    DirtyMemFree(pNode, HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function _HpackHuffmanTreeDecode

    \Description
        Use the huffman tree to decode a series of octets

    \Input *pRef    - module state
    \Input *pData   - input buffer with octets to decode
    \Input iLen     - size of the input buffer to decode
    \Input *pBuffer - [out] output string that we write data to
    \Input *pSuccess- [out] if the decoding was success

    \Version 10/19/2016 (eesponda)
*/
/********************************************************************************F*/
static void _HpackHuffmanTreeDecode(HpackRefT *pRef, const uint8_t *pData, int32_t iLen, char **pBuffer, uint8_t *pSuccess)
{
    int32_t iIndex, iWrite;
    char strTemp[256];
    const HuffmanNodeT *pNode = pRef->pHuffmanTree;

    /* loop through each octet traversing the tree bit by bit
       when a leaf node is found write that to our output */
    for (iIndex = 0, iWrite = 0; iIndex < iLen; iIndex += 1)
    {
        int32_t iBit;
        for (iBit = 7; iBit >= 0; iBit -= 1)
        {
            pNode = ((pData[iIndex] >> iBit) & 1) ? pNode->pLeft : pNode->pRight;
            if (pNode == NULL)
            {
                NetPrintf(("hpack: [%p] huffman code does not exist in tree\n", pRef));
                *pSuccess = FALSE;
                return;
            }
            if ((pNode->pLeft != NULL) || (pNode->pRight != NULL))
            {
                continue;
            }

            // copy string but leave enough room for null terminator
            if ((uint32_t)iWrite < sizeof(strTemp)-1)
            {
                strTemp[iWrite++] = (char)pNode->uChar;
            }
            pNode = pRef->pHuffmanTree;
        }
    }

    // write null terminator and return number of bytes written
    strTemp[iWrite++] = '\0';

    // allocate space for the output buffer
    if ((*pBuffer = (char *)DirtyMemAlloc(iWrite, HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) != NULL)
    {
        ds_strnzcpy(*pBuffer, strTemp, iWrite);
    }
    else
    {
        NetPrintf(("hpack: [%p] could not allocate space for decoding the string into\n", pRef));
        *pSuccess = FALSE;
    }
}

/*F********************************************************************************/
/*!
    \Function _HpackDecodeInteger

    \Description
        Decodes an integer from the sequence of octets

    \Input *pBuf    - where we decode the integer from
    \Input iBufLen  - size of the input buffer
    \Input uMask    - enable bits we use for decoding the integer
    \Input *pValue  - [out] where we write the decoded integer

    \Output
        int32_t     - amount of bytes read from the buffer

    \Version 10/07/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _HpackDecodeInteger(const uint8_t *pBuf, int32_t iBufLen, uint8_t uMask, uint32_t *pValue)
{
    int32_t iRead = 1; //!< we always at least read one byte

    if ((*pValue = *pBuf & uMask) == uMask)
    {
        uint8_t uMSB = 0, uByte;

        // read until you decode the full integer or reach the end of the buffer
        do
        {
            uByte = pBuf[iRead++];
            *pValue += (uByte & 0x7f) * (1 << uMSB);
            uMSB += 7;
        } while (((uByte & 0x80) == 0x80) && (iRead < iBufLen));

        // log in the case of reaching the end
        if (iRead == iBufLen)
        {
            NetPrintf(("hpack: reached the end of the buffer before decoding full integer\n"));
        }
    }
    return(iRead);
}

/*F********************************************************************************/
/*!
    \Function _HpackDecodeString

    \Description
        Decodes an string from the sequence of octets

    \Input *pRef    - module state
    \Input *pBuf    - where we decode the string from
    \Input iBufLen  - size of the input buffer
    \Input *pOutput - [out] where we write the decode string
    \Input *pSuccess- [out] if the decode was successful

    \Output
        int32_t     - amount of bytes read from the buffer

    \Version 10/07/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _HpackDecodeString(HpackRefT *pRef, const uint8_t *pBuf, int32_t iBufLen, char **pOutput, uint8_t *pSuccess)
{
    int32_t iRead = 0;
    uint8_t bHuffman;
    uint32_t uLen;

    // initialize the output to NULL in cases of error
    *pOutput = NULL;

    // decode if huffman encoding was used and the length of the string
    bHuffman = (*pBuf & 0x80) == 0x80;
    iRead += _HpackDecodeInteger(pBuf, iBufLen, 0x7f, &uLen);

    // make sure we have enough space in input
    if ((uint32_t)iBufLen-iRead < uLen)
    {
        NetPrintf(("hpack: [%p] not enough space in input buffer to decode string\n", pRef));
        *pSuccess = FALSE;
        return(iBufLen-iRead);
    }

    // decode using the huffman tree or just copy the literal string
    if (bHuffman == TRUE)
    {
        _HpackHuffmanTreeDecode(pRef, pBuf+iRead, uLen, pOutput, pSuccess);
    }
    else
    {
        // allocate the buffer for decoding
        if ((*pOutput = (char *)DirtyMemAlloc(uLen+1, HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) != NULL)
        {
            ds_strnzcpy(*pOutput, (const char *)pBuf+iRead, uLen+1);
        }
        else
        {
            NetPrintf(("hpack: [%p] could not allocate space for decoding the string into\n", pRef));
            *pSuccess = FALSE;
        }
    }

    return(iRead+uLen);
}

/*F********************************************************************************/
/*!
    \Function _HpackDecodeIndexedField

    \Description
        Decodes an indexed header field

    \Input *pRef    - module state
    \Input uIndex   - index of the header field
    \Input *pEntry  - [out] header field data retrieved from the table

    \Output
        uint8_t     - TRUE if successfully pulled the indexed field from the table

    \Version 10/17/2016 (eesponda)
*/
/********************************************************************************F*/
static uint8_t _HpackDecodeIndexedField(HpackRefT *pRef, uint32_t uIndex, TableEntryT *pEntry)
{
    const TableEntryT *pFound;
    uint8_t bResult = TRUE;

    if (uIndex == 0)
    {
        NetPrintf(("hpack: [%p] received invalid index of 0 when decoding indexed header field\n", pRef));
        return(FALSE);
    }
    uIndex -= 1; // make it zero-indexed

    // try to find the entry in the static table index space
    if (uIndex < HPACK_STATIC_TABLE_SIZE)
    {
        ds_memcpy(pEntry, &Hpack_aStaticTable[uIndex], sizeof(*pEntry));
    }
    //  try to find the entry in the dynamic table index space
    else if ((pFound = _HpackDynamicTableGet(pRef, uIndex - HPACK_STATIC_TABLE_SIZE)) != NULL)
    {
        ds_memcpy(pEntry, pFound, sizeof(*pEntry));
    }
    else
    {
        NetPrintf(("hpack: [%p] indexed header field could not be found at index %u\n", pRef, uIndex));
        bResult = FALSE;
    }
    return(bResult);
}
/*F********************************************************************************/
/*!
    \Function _HpackFindIndexedField

    \Description
        Find an index given a header field

    \Input *pRef    - module state
    \Input *pEntry  - header field we need the index for
    \Input *pIndex  - [out] index of the header field

    \Output
        uint8_t     - TRUE if successfully found the index for the header field

    \Version 10/21/2016 (eesponda)
*/
/********************************************************************************F*/
static uint8_t _HpackFindIndexedField(HpackRefT *pRef, const StaticTableEntryT *pEntry, uint32_t *pIndex)
{
    uint32_t uIndex;
    const TableEntryT *pFound;

    // try to find a valid index in the static table space
    for (uIndex = 0; uIndex < HPACK_STATIC_TABLE_SIZE; uIndex += 1)
    {
        if (strncmp(pEntry->pName, Hpack_aStaticTable[uIndex].pName, strlen(pEntry->pName)) == 0 &&
            strncmp(pEntry->pValue, Hpack_aStaticTable[uIndex].pValue, strlen(pEntry->pValue)) == 0)
        {
            *pIndex = uIndex+1;
            return(TRUE);
        }
    }

    // if not found in static, try the dynamic table space
    uIndex = 0;
    while ((pFound = _HpackDynamicTableGet(pRef, uIndex)) != NULL)
    {
        if (strncmp(pEntry->pName, pFound->pName, strlen(pEntry->pName)) == 0 &&
            strncmp(pEntry->pValue, pFound->pValue, strlen(pEntry->pValue)) == 0)
        {
            *pIndex = uIndex+HPACK_STATIC_TABLE_SIZE+1;
            return(TRUE);
        }
        uIndex += 1;
    }

    // otherwise it was not found
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function _HpackFindIndexedFieldName

    \Description
        Find an index given a header field name

    \Input *pRef    - module state
    \Input *pName   - the header field name we are looking for
    \Input *pIndex  - [out] index of the header field

    \Output
        uint8_t     - TRUE if successfully found the index for the header field name

    \Version 10/21/2016 (eesponda)
*/
/********************************************************************************F*/
static uint8_t _HpackFindIndexedFieldName(HpackRefT *pRef, const char *pName, uint32_t *pIndex)
{
    uint32_t uIndex;
    const TableEntryT *pFound;

    // try to find a valid index in the static table space
    for (uIndex = 0; uIndex < HPACK_STATIC_TABLE_SIZE; uIndex += 1)
    {
        if (strncmp(pName, Hpack_aStaticTable[uIndex].pName, strlen(Hpack_aStaticTable[uIndex].pName)) == 0)
        {
            *pIndex = uIndex+1;
            return(TRUE);
        }
    }

    // if not found in static, try the dynamic table space
    uIndex = 0;
    while ((pFound = _HpackDynamicTableGet(pRef, uIndex)) != NULL)
    {
        if (strncmp(pName, pFound->pName, strlen(pFound->pName)) == 0)
        {
            *pIndex = uIndex+HPACK_STATIC_TABLE_SIZE+1;
            return(TRUE);
        }
        uIndex += 1;
    }

    // otherwise it was not found
    return(FALSE);
}


/*F********************************************************************************/
/*!
    \Function _HpackDecodeLiteralField

    \Description
        Decodes a literal header field

    \Input *pRef    - module state
    \Input *pBuf    - bytes we are decoding the header field from
    \Input iBufSize - size of the input buffer
    \Input uIndex   - index of the header field
    \Input *pEntry  - [out] header field data retrieved from the table
    \Input *pSuccess- [out] if the decode was successful

    \Output
        int32_t     - number of bytes read from the buffer

    \Version 10/17/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _HpackDecodeLiteralField(HpackRefT *pRef, const uint8_t *pBuf, int32_t iBufSize, uint32_t uIndex, TableEntryT *pEntry, uint8_t *pSuccess)
{
    int32_t iRead = 0;

    // if the header name is indexed then pull it from the table
    if (uIndex > 0)
    {
        char *pName;
        int32_t iLen;

        // retrieve the entry, the value in pEntry will point to the static table entry
        if (_HpackDecodeIndexedField(pRef, uIndex, pEntry) == FALSE)
        {
            *pSuccess = FALSE;
            return(0);
        }

        // allocate the space for the string as this will go into the dynamic table
        iLen = (int32_t)strlen(pEntry->pName)+1;
        if ((pName = (char *)DirtyMemAlloc(iLen, HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) != NULL)
        {
            ds_strnzcpy(pName, pEntry->pName, iLen);
        }
        else
        {
            NetPrintf(("hpack: [%p] could not allocate space for the entry name\n", pRef));
            *pSuccess = FALSE;
        }
        pEntry->pName = pName;
    }
    // otherwise decode it from the buffer
    else
    {
        iRead += _HpackDecodeString(pRef, pBuf+iRead, iBufSize-iRead, &pEntry->pName, pSuccess);
    }

    // decode the value from the buffer
    iRead += _HpackDecodeString(pRef, pBuf+iRead, iBufSize-iRead, &pEntry->pValue, pSuccess);

    // return how much data was read
    return(iRead);
}

/*F********************************************************************************/
/*!
    \Function _HpackDynamicTableEject

    \Description
        Ejects header fields from the dynamic table until size hits a specified
        value

    \Input *pRef    - module state
    \Input uSize    - size we need the table to be under

    \Version 10/17/2016 (eesponda)
*/
/********************************************************************************F*/
static void _HpackDynamicTableEject(HpackRefT *pRef, uint32_t uSize)
{
    // if we are over the size, eject entries
    while (pRef->uTableSize > uSize)
    {
        DynamicNodeT *pNode = pRef->pTail;

        // recalculate new size
        pRef->uTableSize -= (uint32_t)strlen(pNode->Entry.pName) + (uint32_t)strlen(pNode->Entry.pValue) + HPACK_TABLE_ENTRY_OVERHEAD;

        // fix up pointers and delete memory
        if (pRef->pTail != pRef->pHead)
        {
            pRef->pTail = pNode->pPrev;
            pRef->pTail->pNext = NULL;
        }
        else
        {
            pRef->pTail = pRef->pHead = NULL;
        }
        DirtyMemFree(pNode->Entry.pName, HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        DirtyMemFree(pNode->Entry.pValue, HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        DirtyMemFree(pNode, HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    }
}

/*F********************************************************************************/
/*!
    \Function _HpackDynamicTableResize

    \Description
        Updates the maximum size of the dynamic table

    \Input *pRef    - module state
    \Input uSize    - new maximum size of the table

    \Version 10/17/2016 (eesponda)
*/
/********************************************************************************F*/
static void _HpackDynamicTableResize(HpackRefT *pRef, uint32_t uSize)
{
    // eject entries
    _HpackDynamicTableEject(pRef, uSize);

    // update size
    pRef->uTableMax = uSize;

#if HPACK_DYNAMICTABLE_LOGGING
    NetPrintf(("hpack: [%p] new table size maximum %u\n", pRef, pRef->uTableMax));
#endif
}

/*F********************************************************************************/
/*!
    \Function _HpackDynamicTableInsert

    \Description
        Inserts a new entry into the dynamic table, ejecting any necessary entries

    \Input *pRef    - module state
    \Input *pEntry  - new entry we are adding to the table

    \Output
        uint8_t     - success=TRUE, failure=FALSE

    \Version 10/17/2016 (eesponda)
*/
/********************************************************************************F*/
static uint8_t _HpackDynamicTableInsert(HpackRefT *pRef, const TableEntryT *pEntry)
{
    DynamicNodeT *pNode;
    uint32_t uEntrySize;

    // make sure entry is valid
    if ((pEntry->pName == NULL) || (pEntry->pValue == NULL))
    {
        return(FALSE);
    }
    // allocate memory for the node in the dynamic table
    if ((pNode = (DynamicNodeT *)DirtyMemAlloc(sizeof(*pNode), HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) == NULL)
    {
        return(FALSE);
    }
    ds_memclr(pNode, sizeof(*pNode));
    ds_memcpy(&pNode->Entry, pEntry, sizeof(pNode->Entry));

    uEntrySize = (uint32_t)strlen(pEntry->pName) + (uint32_t)strlen(pEntry->pValue) + HPACK_TABLE_ENTRY_OVERHEAD;
    pRef->uTableSize += uEntrySize;

    // attach to list
    if (pRef->pTail != NULL)
    {
        pNode->pNext = pRef->pHead;
        pRef->pHead->pPrev = pNode;
    }
    else
    {
        pRef->pTail = pNode;
    }
    pRef->pHead = pNode;

    // if we are over the size, eject entries
    _HpackDynamicTableEject(pRef, pRef->uTableMax);

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _HpackEncodeInteger

    \Description
        Encodes an integer into a sequence of octets

    \Input uValue   - integer we are encoding
    \Input uMask    - enable bits we use for encoding the integer
    \Input *pBuf    - [out] where we encode the integer into
    \Input iBufLen  - size of the output buffer

    \Output
        int32_t     - amount of bytes written to the buffer

    \Version 10/21/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _HpackEncodeInteger(uint32_t uValue, uint8_t uMask, uint8_t *pBuf, int32_t iBufLen)
{
    int32_t iWrite = 1;

    if (uValue < uMask)
    {
        *pBuf |= (uint8_t)uValue;
    }
    else
    {
        *pBuf  |= uMask;    //!< turn on all the bits in the prefix
        uValue -= uMask;    //!< decrease the value by that amount

        for (iWrite = 1; (iWrite < iBufLen) && (uValue >= 0x80); iWrite += 1)
        {
            // set the lsb of the value and set the msb as a continuation flag
            pBuf[iWrite] = (uValue % 0x80) + 0x80;
            uValue /= 0x80;
        }
        pBuf[iWrite++] = uValue;
    }
    return(iWrite);
}

/*F********************************************************************************/
/*!
    \Function _HpackHuffmanEncode

    \Description
        Huffman encodes a character

    \Input *pEncode     - encoding state
    \Input *pHuffman    - huffman code information

    \Version 10/21/2016 (eesponda)
*/
/********************************************************************************F*/
static void _HpackHuffmanEncode(HuffmanEncodeT *pEncode, const HuffmanCodeT *pHuffman)
{
    int32_t iIndex;
    const int32_t iBits = sizeof(*pEncode->aResult) * 8;
    const int32_t iSize = sizeof(pEncode->aResult) / sizeof(*pEncode->aResult);

    // increase the count of bits and track the next octet boundary
    pEncode->uCount += pHuffman->uLen;
    if (pEncode->uCount > pEncode->uNextBoundary)
    {
        /* figure out how much we need to increment the boundary by
           if the count is on an octet, just move the boundary there
           if the count is over an octet, just move to the next octet past count (might be more than 1 octet) */
        const uint8_t uDifference = pEncode->uCount - pEncode->uNextBoundary;
        if ((uDifference % 8) == 0)
        {
            pEncode->uNextBoundary = pEncode->uCount;
        }
        else
        {
            const uint8_t uNumOctets = (uDifference / 8) + 1;
            pEncode->uNextBoundary += 8 * uNumOctets;
        }
    }

    // loop through the bytes backwards shifting by the left
    for (iIndex = iSize-1; iIndex > 0; iIndex -= 1)
    {
        uint32_t uResult;
        if ((uResult = (pEncode->aResult[iIndex] << pHuffman->uLen) | (pEncode->aResult[iIndex-1] >> (iBits-pHuffman->uLen))) != 0)
        {
            pEncode->aResult[iIndex] = uResult;
        }
    }
    // finally encode using the huffman code
    *pEncode->aResult = (*pEncode->aResult << pHuffman->uLen) | pHuffman->uCode;
}

/*F********************************************************************************/
/*!
    \Function _HpackEncodeString

    \Description
        Encodes a string into a sequence of octets

    \Input *pRef    - module state
    \Input *pBuf    - where we encode the string from
    \Input iBufSize - size of the input buffer
    \Input *pOutput - [out] where we write the encoded string
    \Input iOutSize - size of the output buffer
    \Input bHuffman - use huffman encoding?

    \Output
        int32_t     - amount of bytes written into the buffer

    \Version 10/21/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _HpackEncodeString(HpackRefT *pRef, const char *pBuf, int32_t iBufSize, uint8_t *pOutput, int32_t iOutSize, uint8_t bHuffman)
{
    int32_t iWrite = 0;

    if (bHuffman == TRUE)
    {
        int32_t iIndex, iTotalBytes;
        uint32_t uLeftover;
        HuffmanCodeT Huffman;
        HuffmanEncodeT Encode;

        ds_memclr(Encode.aResult, sizeof(Encode.aResult));
        Encode.uCount = 0;
        Encode.uNextBoundary = 8;

        // encode each character one at a time
        for (iIndex = 0; iIndex < iBufSize; iIndex += 1)
        {
            uint8_t uChar = pBuf[iIndex];
            _HpackHuffmanEncode(&Encode, &Hpack_aHuffmanCodes[uChar]);
        }

        // finalize the huffman encoding using the EOS code
        if ((uLeftover = Encode.uNextBoundary - Encode.uCount) > 0)
        {
            Huffman.uCode = Hpack_EOS.uCode >> (Hpack_EOS.uLen - uLeftover);
            Huffman.uLen = uLeftover;
            _HpackHuffmanEncode(&Encode, &Huffman);
        }

        pOutput[iWrite] = 0x80;
        iTotalBytes = Encode.uCount/8;
        iWrite += _HpackEncodeInteger(iTotalBytes, 0x7f, pOutput, iOutSize);

        /* write the sequence to the output buffer
           to ensure we write only as much as needed we calculate
           how many bytes we need to write each iteration */
        while ((iTotalBytes > 0) && (iWrite < iOutSize))
        {
            // figure out how many bytes we need to write
            int32_t iRemainder = iTotalBytes % sizeof(*Encode.aResult);
            // figure out what index into the result we need to read from
            iIndex = (iTotalBytes-1) / sizeof(*Encode.aResult);

            // if on the boundary set the bytes to equal to the boundary
            if (iRemainder == 0)
            {
                iRemainder = sizeof(*Encode.aResult);
            }
            // write the correct number of bytes to the output
            for (; iRemainder > 0; iRemainder -= 1, iTotalBytes -= 1)
            {
                pOutput[iWrite++] = (uint8_t)(Encode.aResult[iIndex] >> ((iRemainder-1) * 8));
            }
        }
    }
    else
    {
        iWrite += _HpackEncodeInteger(iBufSize, 0x7f, pOutput, iOutSize);

        ds_memcpy_s(pOutput+iWrite, iOutSize-iWrite, pBuf, iBufSize);
        iWrite += iBufSize;
    }

    return(iWrite);
}


#if HPACK_DYNAMICTABLE_LOGGING
/*F********************************************************************************/
/*!
    \Function _HpackPrintTable

    \Description
        Prints the entries in the dynamic table

    \Input *pRef    - module state

    \Version 11/04/2016 (eesponda)
*/
/********************************************************************************F*/
static void _HpackPrintTable(HpackRefT *pRef)
{
    const TableEntryT *pEntry;
    uint32_t uIndex = 0;

    while ((pEntry = _HpackDynamicTableGet(pRef, uIndex)) != NULL)
    {
        uIndex += 1;
        NetPrintf(("hpack: [%p] [%02u] (s = %u) %s: %s\n", pRef, uIndex, strlen(pEntry->pName) + strlen(pEntry->pValue) + HPACK_TABLE_ENTRY_OVERHEAD, pEntry->pName, pEntry->pValue));
    }
    NetPrintf(("hpack: [%p] table size: %u\n", pRef, pRef->uTableSize));
}
#endif

/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function HpackCreate

    \Description
        Allocate module state and prepare for use

    \Input uTableMax    - maximum size of the dynamic table
    \Input bDecoder     - is the ref for a decoder?

    \Output
        HpackRefT *     - pointer to module state, or NULL

    \Notes
        uTableMax is based upon SETTINGS_HEADER_TABLE_SIZE in the
        HTTP/2 header frame

    \Version 10/05/2016 (eesponda)
*/
/********************************************************************************F*/
HpackRefT *HpackCreate(uint32_t uTableMax, uint8_t bDecoder)
{
    HpackRefT *pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // query memgroup data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate state
    if ((pRef = (HpackRefT *)DirtyMemAlloc(sizeof(*pRef), HPACK_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("hpack: unable to allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pRef, sizeof(*pRef));
    pRef->uTableMax = uTableMax;
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;

    // build huffman tree
    if ((bDecoder == TRUE) && (_HpackHuffmanTreeBuild(pRef) == FALSE))
    {
        HpackDestroy(pRef);
        return(NULL);
    }

    return(pRef);
}

/*F********************************************************************************/
/*!
    \Function HpackDestroy

    \Description
        Destroy the module and release its state

    \Input *pRef    - module state

    \Version 10/05/2016 (eesponda)
*/
/********************************************************************************F*/
void HpackDestroy(HpackRefT *pRef)
{
    // clean up the tree recursively
    if (pRef->pHuffmanTree != NULL)
    {
        _HpackHuffmanTreeDestroy(pRef, pRef->pHuffmanTree);
        pRef->pHuffmanTree = NULL;
    }

    // clear the dynamic table
    HpackClear(pRef);

    DirtyMemFree(pRef, HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function HpackDecode

    \Description
        Decode the byte array into formatted header string

    \Input *pRef    - module state
    \Input *pInput  - the bytes we are decoding
    \Input iInpSize - number of bytes in the input
    \Input *pOutput - [out] formatted header string CRLF delimited
    \Input iOutSize - size of the string we are writing to

    \Output
        int32_t     - success=amount of data written to output, failure=negative

    \Version 10/17/2016 (eesponda)
*/
/********************************************************************************F*/
int32_t HpackDecode(HpackRefT *pRef, const uint8_t *pInput, int32_t iInpSize, char *pOutput, int32_t iOutSize)
{
    int32_t iRead = 0, iWrite = 0;
    uint32_t uIndex;
    uint8_t bSuccess = TRUE;

    // clear the output
    ds_memclr(pOutput, iOutSize);

    while ((iRead < iInpSize) && (bSuccess == TRUE))
    {
        TableEntryT Entry;
        uint32_t uSize;
        uint8_t uByte = pInput[iRead], bWrite = TRUE, bFree = FALSE;
        ds_memclr(&Entry, sizeof(Entry));

        /* mask out the higher bits to figure out what type of header field it is
           then pass a mask of the lower bits to the decode */

        // indexed header field '1' 7-bit prefix
        if ((uByte & HPACK_INDEXED_HEADER_FIELD) != 0)
        {
            iRead += _HpackDecodeInteger(pInput+iRead, iInpSize-iRead, HPACK_INDEXED_HEADER_FIELD-1, &uIndex);
            bSuccess = _HpackDecodeIndexedField(pRef, uIndex, &Entry);
        }
        // literal header field with incremental indexing '01' 6-bit prefix
        else if ((uByte & HPACK_LITERAL_HEADER_FIELD) != 0)
        {
            iRead += _HpackDecodeInteger(pInput+iRead, iInpSize-iRead, HPACK_LITERAL_HEADER_FIELD-1, &uIndex);
            iRead += _HpackDecodeLiteralField(pRef, pInput+iRead, iInpSize-iRead, uIndex, &Entry, &bSuccess);

            // add entry to dynamic table
            if (_HpackDynamicTableInsert(pRef, &Entry) == FALSE)
            {
                bFree = TRUE; /* free the memory */
                bSuccess = FALSE; /* set the failure flag */
            }
        }
        // dynamic table size update '001' 5-bit prefix
        else if ((uByte & HPACK_DYNAMIC_TABLE_UPDATE) != 0)
        {
            bWrite = FALSE; /* disable writing headers */

            iRead += _HpackDecodeInteger(pInput+iRead, iInpSize-iRead, HPACK_DYNAMIC_TABLE_UPDATE-1, &uSize);
            _HpackDynamicTableResize(pRef, uSize);
        }
        // literal header field never indexed '0001' 4-bit prefix
        else if ((uByte & HPACK_LITERAL_HEADER_FIELD_NEVER) != 0)
        {
            iRead += _HpackDecodeInteger(pInput+iRead, iInpSize-iRead, HPACK_LITERAL_HEADER_FIELD_NEVER-1, &uIndex);
            iRead += _HpackDecodeLiteralField(pRef, pInput+iRead, iInpSize-iRead, uIndex, &Entry, &bSuccess);
            bFree = TRUE; /* cleanup after it is written */
        }
        // literal header field without indexing '0000' 4-bit prefix
        else
        {
            iRead += _HpackDecodeInteger(pInput+iRead, iInpSize-iRead, HPACK_LITERAL_HEADER_FIELD_NEVER-1, &uIndex);
            iRead += _HpackDecodeLiteralField(pRef, pInput+iRead, iInpSize-iRead, uIndex, &Entry, &bSuccess);
            bFree = TRUE; /* cleanup after it is written */
        }

        // write the header field out if not dynamic table size update
        if (bWrite == TRUE)
        {
            iWrite += ds_snzprintf(pOutput+iWrite, iOutSize-iWrite, "%s: %s\r\n", Entry.pName, Entry.pValue);
        }
        // if we need to cleanup entry memory do so
        if (bFree == TRUE)
        {
            DirtyMemFree(Entry.pName, HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
            DirtyMemFree(Entry.pValue, HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        }
    }

#if HPACK_DYNAMICTABLE_LOGGING
    _HpackPrintTable(pRef);
#endif

    return(bSuccess == TRUE ? iWrite : -1);
}

/*F********************************************************************************/
/*!
    \Function HpackEncode

    \Description
        Encode the formatted header string into a byte array

    \Input *pRef    - module state
    \Input *pInput  - formatted header string CRLF delimited
    \Input *pOutput - [out] byte array we are writing into
    \Input iOutSize - size of the byte array we are writing to
    \Input bHuffman - do we need to huffman encode the strings?

    \Output
        int32_t     - success=amount of data written, failure=negative

    \Version 10/17/2016 (eesponda)
*/
/********************************************************************************F*/
int32_t HpackEncode(HpackRefT *pRef, const char *pInput, uint8_t *pOutput, int32_t iOutSize, uint8_t bHuffman)
{
    char strName[256], strValue[4*1024];
    StaticTableEntryT TempEntry;
    int32_t iWrite = 0;

    // clear the output
    ds_memclr(pOutput, iOutSize);

    // if there is a pending dynamic table update, encode that first
    if (pRef->bUpdatePending == TRUE)
    {
        pOutput[iWrite] = HPACK_DYNAMIC_TABLE_UPDATE;
        iWrite += _HpackEncodeInteger(pRef->uTableMax, HPACK_DYNAMIC_TABLE_UPDATE-1, pOutput+iWrite, iOutSize-iWrite);

        pRef->bUpdatePending = FALSE;
    }

    // point to the address of the stack variables
    TempEntry.pName = strName;
    TempEntry.pValue = strValue;

    // for each header entry try to encode
    while (ProtoHttpGetNextHeader(NULL, pInput, strName, sizeof(strName), strValue, sizeof(strValue), &pInput) == 0)
    {
        uint32_t uIndex;
        TableEntryT Entry;

        /* force the strName to lowercase */
        for (uIndex = 0; (strName[uIndex] != '\0') && (uIndex < sizeof(strName)); uIndex += 1)
        {
            if (isupper(strName[uIndex]))
            {
                strName[uIndex] = tolower(strName[uIndex]);
            }
        }

        /* attempt to find the header entry in the static/dynamic table
           if found, then encode it as an indexed header field */
        if (_HpackFindIndexedField(pRef, &TempEntry, &uIndex) == TRUE)
        {
            // write indexed header field '1' followed by index using 7-bit prefix
            pOutput[iWrite] = HPACK_INDEXED_HEADER_FIELD;
            iWrite += _HpackEncodeInteger(uIndex, HPACK_INDEXED_HEADER_FIELD-1, pOutput+iWrite, iOutSize-iWrite);
        }
        // otherwise encode it as literal header field with indexing
        else
        {
            int32_t iLen;

            /* write literal header field with incremental indexing '01' followed by index (if indexable name) or encoded string (if not)
               and encoded string (for value) */
            pOutput[iWrite] = HPACK_LITERAL_HEADER_FIELD;
            if (_HpackFindIndexedFieldName(pRef, TempEntry.pName, &uIndex) == TRUE)
            {
                iWrite += _HpackEncodeInteger(uIndex, HPACK_LITERAL_HEADER_FIELD-1, pOutput+iWrite, iOutSize-iWrite);
            }
            else
            {
                iWrite += 1;
                iWrite += _HpackEncodeString(pRef, TempEntry.pName, (int32_t)strlen(TempEntry.pName), pOutput+iWrite, iOutSize-iWrite, bHuffman);
            }
            iWrite += _HpackEncodeString(pRef, TempEntry.pValue, (int32_t)strlen(TempEntry.pValue), pOutput+iWrite, iOutSize-iWrite, bHuffman);

            // allocate space for the entry name in the dynamic table
            iLen = (int32_t)strlen(TempEntry.pName)+1;
            if ((Entry.pName = (char *)DirtyMemAlloc(iLen, HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) != NULL)
            {
                ds_strnzcpy(Entry.pName, TempEntry.pName, iLen);
            }
            else
            {
                NetPrintf(("hpack: [%p] could not allocate space for entry name for encoding dynamic table\n", pRef));
            }
            // allocate space for the entry value in the dynamic table
            iLen = (int32_t)strlen(TempEntry.pValue)+1;
            if ((Entry.pValue = (char *)DirtyMemAlloc(iLen, HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) != NULL)
            {
                ds_strnzcpy(Entry.pValue, TempEntry.pValue, iLen);
            }
            else
            {
                NetPrintf(("hpack: [%p] could not allocate space for entry value for encoding dynamic table\n", pRef));
            }

            // insert the new entry into the dynamic table
            if (_HpackDynamicTableInsert(pRef, &Entry) == FALSE)
            {
                DirtyMemFree(Entry.pName, HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
                DirtyMemFree(Entry.pValue, HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
                return(-1);
            }
        }
    }

#if HPACK_DYNAMICTABLE_LOGGING
    _HpackPrintTable(pRef);
#endif

    return(iWrite);
}

/*F********************************************************************************/
/*!
    \Function HpackClear

    \Description
        Clears the dynamic table

    \Input *pRef    - module state

    \Version 10/31/2016 (eesponda)
*/
/********************************************************************************F*/
void HpackClear(HpackRefT *pRef)
{
    DynamicNodeT *pNode;

    //! walk the list and clean up the memory
    while ((pNode = pRef->pHead) != NULL)
    {
        pRef->pHead = pNode->pNext;

        DirtyMemFree(pNode->Entry.pName, HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        DirtyMemFree(pNode->Entry.pValue, HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        DirtyMemFree(pNode, HPACK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    }
    pRef->pTail = NULL;
    pRef->uTableSize = 0;
}

/*F********************************************************************************/
/*!
    \Function HpackResize

    \Description
        Resizes the dynamic table

    \Input *pRef        - module state
    \Input uTableSize   - new maximum table size
    \Input bSendUpdate  - do we need to notify peer of dynamic table size change

    \Version 11/09/2016 (eesponda)
*/
/********************************************************************************F*/
void HpackResize(HpackRefT *pRef, uint32_t uTableSize, uint8_t bSendUpdate)
{
    _HpackDynamicTableResize(pRef, uTableSize);
    pRef->bUpdatePending = bSendUpdate;
}
