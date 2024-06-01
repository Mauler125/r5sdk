/*H********************************************************************************/
/*!
    \File dirtypng.c

    \Description
        Routines to decode a PNG into a raw image and palette.  These routines
        were written from scratch based on the published specifications.  The
        functions and style were heavily based on the dirtygif.c and dirtyjpeg.c
        files coded by James Brookes.

    \Notes
        References:
            Info page: http://www.libpng.org/pub/png/
            Sample images: http://www.libpng.org/pub/png/pngsuite.html
            W3C Specification: http://www.libpng.org/pub/png/spec/iso/index-object.html

    \Copyright
        Copyright (c) 2007 Electronic Arts Inc.

    \Version 05/10/2007 (cadam) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "DirtySDK/graph/dirtypng.h"

/*** Defines **********************************************************************/

#define DIRTYPNG_DEBUG         (FALSE)
#define DIRTYPNG_DEBUG_CHUNKS  (DIRTYPNG_DEBUG && FALSE)
#define DIRTYPNG_DEBUG_ZLIB    (DIRTYPNG_DEBUG && FALSE)
#define DIRTYPNG_DEBUG_DEFLATE (DIRTYPNG_DEBUG && FALSE)
#define DIRTYPNG_DEBUG_DYNAMIC (DIRTYPNG_DEBUG && FALSE)
#define DIRTYPNG_DEBUG_INFLATE (DIRTYPNG_DEBUG && FALSE)
#define DIRTYPNG_DEBUG_LDCODES (DIRTYPNG_DEBUG && FALSE)

//! define set lengths
#define TYPESTART 4
#define DATASTART 8
#define HEADER_LEN 25
#define SIGNATURE_LEN 8

//! little endian of the 32-bit CRC polynomial 1110 1101 1011 1000 1000 0011 0010 0000 (1)
#define POLYNOMIAL 0xedb88320

//! define max values for HLIT + 257, HDIST + 1, and HCLEN + 4
#define MAXLIT 286
#define MAXDIST 32
#define MAXCLEN 19

/*** Macros ***********************************************************************/

//! macro to validate that enough space exists in the file for a particular header
#define DIRTYPNG_Validate(_pData, _pEnd, _size, _retcode)   \
{                                                           \
    if (((_pEnd) - (_pData)) < (_size))                     \
    {                                                       \
        return(_retcode);                                   \
    }                                                       \
}

#define DIRTYPNG_GetInt32(_pData, _pOffset)                 \
    ((_pData[_pOffset] << 24) | (_pData[_pOffset+1] << 16) | (_pData[_pOffset+2] << 8) | _pData[_pOffset+3])

/*** Type Definitions *************************************************************/

//! huffman decode tables
typedef struct HuffLitTableT
{
    uint16_t uNoCodes;
    uint16_t uMinCode;
    uint16_t uMaxCode;
    uint16_t Codes[MAXLIT];
} HuffLitTableT;

typedef struct HuffDistTableT
{
    uint16_t uNoCodes;
    uint16_t uMinCode;
    uint16_t uMaxCode;
    uint16_t Codes[MAXDIST];
} HuffDistTableT;

typedef struct HuffCLenTableT
{
    uint16_t uNoCodes;
    uint16_t uMinCode;
    uint16_t uMaxCode;
    uint16_t Codes[MAXCLEN];
} HuffCLenTableT;

typedef struct PngPaletteT
{
    uint32_t uNumColors;
    uint8_t aColor[256][3];
} PngPaletteT;

//! module state
struct DirtyPngStateT
{
    // module memory group
    int32_t  iMemGroup;         //!< module mem group id
    void     *pMemGroupUserData;//!< user data associated with mem group

    uint32_t uBufWidth;         //!< width of buffer to decode into
    uint32_t uBufHeight;        //!< height of buffer to decode into

    uint32_t uWidthOffset;      //!< offset for which byte of the current scanline we are at
    uint32_t uHeightOffset;     //!< offset for which scanline of the buffer we are at
    uint32_t uImageOffset;      //!< current offset of the output buffer

    uint32_t uDataBits;         //!< bits grabbed from the IDAT data
    uint32_t uByteOffset;       //!< byte offset from which the next data should be read
    uint32_t uIdatStart;        //!< byte offset from which the current IDAT data started
    uint32_t uIdatLength;       //!< length of the current IDAT chunk data being read

    uint16_t uCountIdat;        //!< number of IDAT chunks parsed

    uint16_t uHLit;             //!< number of literal/length codes
    uint16_t uHDist;            //!< number of distance codes
    uint16_t uHCLen;            //!< number of code length codes

    uint16_t uWindowLength;     //!< current length of the window
    uint16_t uWindowOffset;     //!< current offset in the window
    uint16_t uWindowSize;       //!< window size to work with

    uint16_t uScanlineWidth;    //!< width of the scanline in bytes
    uint16_t uCurrentScanline;  //!< current scanline that is to be written to the output buffer

    uint8_t uFilterType;        //!< filter type

    uint8_t uCurrentPass;       //!< current pass of the image being parsed

    uint8_t uPixelWidth;        //!< number of bytes per pixel

    uint8_t uWindowLog;        //!< log base 2 of the window size minus 8

    uint8_t uChunkTime;         //!< tIME chunk parsed
    uint8_t uChunkPhys;         //!< pHYs chunk parsed
    uint8_t uChunkIccp;         //!< iCCP or sRGB chunk parsed
    uint8_t uChunkSbit;         //!< sBIT chunk parsed
    uint8_t uChunkGama;         //!< gAMA chunk parsed
    uint8_t uChunkChrm;         //!< cHRM chunk parsed
    uint8_t uChunkPlte;         //!< PLTE chunk parsed
    uint8_t uChunkTrns;         //!< tRNS chunk parsed
    uint8_t uChunkHist;         //!< hIST chunk parsed
    uint8_t uChunkBkgd;         //!< bKGD chunk parsed
    uint8_t uChunkIdat;         //!< IDAT chunk parsed
    uint8_t uParsingIdat;       //!< last chunk parsed was an IDAT chunk

    uint8_t uBType;             //!< compression type for the block
    uint8_t uBlockEnd;          //!< done reading the block
    uint8_t uBitsLeft;          //!< bits left in uDataBits

    uint8_t uLitMinBits;        //!< smallest bit length of the literal/length codes
    uint8_t uDistMinBits;       //!< smallest bit length of the distance codes

    PngPaletteT Palette;

    uint8_t uCRCTableGenerated; //!< CRC table generated
    uint32_t CRCTable[256];     //!< CRC table

    uint32_t InterlacePassPixels[7];    //!< number of pixels per scanline for each pass
    uint32_t InterlaceScanlines[7];     //!< number of scanlines per pass

    uint8_t SlidingWindow[32768];       //!< sliding window

    HuffLitTableT LitTables[16];        //!< huffman tables for the literal/length codes
    HuffDistTableT DistTables[16];      //!< huffman tables for the distance codes
    HuffCLenTableT CLenTables[8];       //!< huffman tables for the code length alphabet

    uint8_t *pPrevScanline;     //!< pointer to the previous scanline
    uint8_t *pCurrScanline;     //!< pointer to the current scanline
    uint8_t *pScanlines[2];     //!< pointers to the two scanlines

    uint8_t *pImageBuf;         //!< pointer to output image buffer (allocated by caller)
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

//! code lengths and how many extra bits to grab
static const uint16_t _HLIT_Codes[] =
{
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27,
    31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};
static const uint8_t _HLIT_Bits[] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
    2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
};

//! code start distances and how many extra bits to grab
static const uint16_t _HDIST_Codes[] =
{
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25,
    33, 49, 65, 97, 129, 193, 257, 385, 513, 769,
    1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};
static const uint8_t _HDIST_Bits[] =
{
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6,
    6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

//! code length value order for the code length alphabet
static const uint8_t _HCLEN_Codes[] =
{
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5,
    11, 4, 12, 3, 13, 2, 14, 1, 15
};

/*** Private Functions ************************************************************/

#if DIRTYPNG_DEBUG_CHUNKS
/*F********************************************************************************/
/*!
    \Function _DirtyPngPrintChunk

    \Description
        Display the Chunk Information.  Used only for debugging.

    \Input *pPngData    - pointer to the PNG data
    \Input uLength      - length of the chunk to print
    \Input uShowData    - boolean (show data or not)

    Version 02/06/2007 (cadam)
*/
/********************************************************************************F*/
static void _DirtyPngPrintChunk(const uint8_t *pPngData, uint32_t uLength, uint8_t uShowData)
{
    NetPrintf(("dirtypng: chunk type=%c%c%c%c, len=%d, crc=0x%08x\n", pPngData[TYPESTART], pPngData[TYPESTART+1], pPngData[TYPESTART+2], pPngData[TYPESTART+3],
        uLength, DIRTYPNG_GetInt32(pPngData, uLength+DATASTART)));
    if (uShowData != 0)
    {
        NetPrintMem(pPngData+DATASTART, uLength, "chunk data");
    }
}
#endif

#if DIRTYPNG_DEBUG_DYNAMIC
/*F********************************************************************************/
/*!
    \Function _DirtyPngPrintLitTables

    \Description
        Print the Literal/Length Tables.  Used only for debugging.

    \Input *pState      - pointer to the module state

    Version 02/09/2007 (cadam)
*/
/********************************************************************************F*/
static void _DirtyPngPrintLitTables(DirtyPngStateT *pState)
{
    uint16_t u, v, uNumCodes;

    NetPrintf(("dirtypng: Literal/Length Table\n"));
    for (u = 0; u < 16; u++)
    {
        if (pState->LitTables[u].uNoCodes)
        {
            NetPrintf(("dirtypng: NULL\n"));
        }
        else
        {
            uNumCodes = pState->LitTables[u].uMaxCode - pState->LitTables[u].uMinCode + 1;

            NetPrintf(("dirtypng: uMinCode %u : ", pState->LitTables[u].uMinCode));
            NetPrintf(("uMaxCode %u : ", pState->LitTables[u].uMaxCode));
            for (v = 0; v < uNumCodes; v++)
            {
                NetPrintf(("%u ", pState->LitTables[u].Codes[v]));
            }
            NetPrintf(("\n"));
        }
    }
    NetPrintf(("\n"));
}
#endif

#if DIRTYPNG_DEBUG_DYNAMIC
/*F********************************************************************************/
/*!
    \Function _DirtyPngPrintDistTables

    \Description
        Print the Distance Tables.  Used only for debugging.

    \Input *pState      - pointer to the module state

    Version 02/09/2007 (cadam)
*/
/********************************************************************************F*/
static void _DirtyPngPrintDistTables(DirtyPngStateT *pState)
{
    uint16_t u, v, uNumCodes;

    NetPrintf(("dirtypng: Distance Table\n"));
    for (u = 0; u < 16; u++)
    {
        if (pState->DistTables[u].uNoCodes)
        {
            NetPrintf(("dirtypng: NULL\n"));
        }
        else
        {
            uNumCodes = pState->DistTables[u].uMaxCode - pState->DistTables[u].uMinCode + 1;

            NetPrintf(("dirtypng: uMinCode %u : ", pState->DistTables[u].uMinCode));
            NetPrintf(("uMaxCode %u : ", pState->DistTables[u].uMaxCode));
            for (v = 0; v < uNumCodes; v++)
            {
                NetPrintf(("%u ", pState->DistTables[u].Codes[v]));
            }
            NetPrintf(("\n"));
        }
    }
    NetPrintf(("\n"));
}
#endif

#if DIRTYPNG_DEBUG_DYNAMIC
/*F********************************************************************************/
/*!
    \Function _DirtyPngPrintCLenTables

    \Description
        Print the Code Length Tables.  Used only for debugging.

    \Input *pState      - pointer to the module state

    \Output
        None.

    Version 02/09/2007 (cadam)
*/
/********************************************************************************F*/
static void _DirtyPngPrintCLenTables(DirtyPngStateT *pState)
{
    uint16_t u, v, uNumCodes;

    NetPrintf(("dirtypng: Code Length Table\n"));
    for (u = 0; u < 8; u++)
    {
        if (pState->CLenTables[u].uNoCodes)
        {
            NetPrintf(("dirtypng: NULL\n"));
        }
        else
        {
            uNumCodes = pState->CLenTables[u].uMaxCode - pState->CLenTables[u].uMinCode + 1;

            NetPrintf(("dirtypng: uMinCode %u : ", pState->CLenTables[u].uMinCode));
            NetPrintf(("uMaxCode %u : ", pState->CLenTables[u].uMaxCode));
            for (v = 0; v < uNumCodes; v++)
            {
                NetPrintf(("%u ", pState->CLenTables[u].Codes[v]));
            }
            NetPrintf(("\n"));
        }
    }
    NetPrintf(("\n"));
}
#endif

/*F********************************************************************************/
/*!
    \Function _DirtyPngGenerateCRCTable

    \Description
        Generate the CRC Table by calculating the CRC for all byte values possible.

    \Input *pState      - pointer to the module state

    Version 05/02/2007 (cadam)
*/
/********************************************************************************F*/
static void _DirtyPngGenerateCRCTable(DirtyPngStateT *pState)
{
    uint32_t uCRC, uByteValue, uBit;

    for (uByteValue = 0; uByteValue < 256; uByteValue++)
    {
        uCRC = uByteValue;

        for (uBit = 0; uBit < 8; uBit++)
        {
            if (uCRC & 1)
            {
                uCRC = (uCRC >> 1) ^ POLYNOMIAL;
            }
            else
            {
                uCRC >>= 1;
            }
        }

        pState->CRCTable[uByteValue] = uCRC;
    }
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngCheckCRC

    \Description
        Calculate the CRC value for the chunk and compare to the received CRC.

    \Input *pState      - pointer to the module state
    \Input *pPngData    - pointer to the PNG data
    \Input uLength      - length of the chunk being checked

    \Output
        int32_t         - nonnegative=success, negative=error

    Version 02/06/2007 (cadam)
*/
/********************************************************************************F*/
static int32_t _DirtyPngCheckCRC(DirtyPngStateT *pState, const uint8_t *pPngData, uint32_t uLength)
{
    uint32_t uLen, uChunkCRC;
    // PNG requires the CRC initially be set to all 1s
    uint32_t uCRC = 0xFFFFFFFF;

    for (uLen = 0; uLen < uLength+TYPESTART; uLen++)
    {
        uCRC = uCRC ^ pPngData[uLen+TYPESTART];

        uCRC = pState->CRCTable[uCRC & 0x000000FF] ^ (uCRC >> 8);
    }

    uChunkCRC = DIRTYPNG_GetInt32(pPngData, uLength+DATASTART);

    // PNG requires that the calculated CRC's 1s complement is what gets stored for each chunk
    if (~uCRC == uChunkCRC)
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngReset

    \Description
        Reset the DirtyPngState structure.

    \Input *pState      - pointer to the module state

    Version 05/02/2007 (cadam)
*/
/********************************************************************************F*/
static void _DirtyPngReset(DirtyPngStateT *pState)
{
    pState->uWidthOffset = 0;
    pState->uHeightOffset = 0;
    pState->uImageOffset = 0;

    pState->uDataBits = 0;

    pState->uWindowLength = 0;
    pState->uWindowOffset = 0;

    pState->uCurrentScanline = 0;

    pState->uFilterType = (uint8_t)-1;

    pState->uCurrentPass = 0;

    pState->uChunkTime = FALSE;
    pState->uChunkPhys = FALSE;
    pState->uChunkIccp = FALSE;
    pState->uChunkSbit = FALSE;
    pState->uChunkGama = FALSE;
    pState->uChunkChrm = FALSE;
    pState->uChunkPlte = FALSE;
    pState->uChunkTrns = FALSE;
    pState->uChunkHist = FALSE;
    pState->uChunkBkgd = FALSE;
    pState->uChunkIdat = FALSE;
    pState->uParsingIdat = FALSE;

    pState->uBitsLeft = 0;

    ds_memclr(pState->InterlacePassPixels, sizeof(pState->InterlacePassPixels));
    ds_memclr(pState->InterlaceScanlines, sizeof(pState->InterlaceScanlines));

    ds_memclr(pState->LitTables, sizeof(pState->LitTables));
    ds_memclr(pState->DistTables, sizeof(pState->DistTables));
    ds_memclr(pState->CLenTables, sizeof(pState->CLenTables));

    if (pState->uCRCTableGenerated == 0)
    {
        _DirtyPngGenerateCRCTable(pState);
        pState->uCRCTableGenerated = 1;
    }
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngParseHeader

    \Description
        Parse the PNG header.

    \Input pState       - module state
    \Input *pPngHdr     - [out] pointer to PNG header to fill in
    \Input *pPngData    - pointer to PNG data
    \Input *pPngEnd     - pointer past the end of PNG data

    \Output
        int32_t         - nonnegative=success, negative=error

    \Version 02/06/2007 (cadam)
*/
/********************************************************************************F*/
static int32_t _DirtyPngParseHeader(DirtyPngStateT *pState, DirtyPngHdrT *pPngHdr, const uint8_t *pPngData, const uint8_t *pPngEnd)
{
    // iClearCode header
    ds_memclr(pPngHdr, sizeof(*pPngHdr));

    // skip the PNG signature
    pPngData += SIGNATURE_LEN;
    
    // validate the IHDR chunk
    DIRTYPNG_Validate(pPngData, pPngEnd, HEADER_LEN, DIRTYPNG_ERR_TOOSHORT);
    
    // make sure the length field specifies a chunk length of 13
    if (DIRTYPNG_GetInt32(pPngData, 0) != HEADER_LEN-12)
    {
        return(DIRTYPNG_ERR_TOOSHORT);
    }

    #if DIRTYPNG_DEBUG_CHUNKS
    _DirtyPngPrintChunk(pPngData, HEADER_LEN-12, 1);
    #endif

    // check that the CRC value of the chunk is valid
    if (_DirtyPngCheckCRC(pState, pPngData, HEADER_LEN-12) != TRUE)
    {
        return(DIRTYPNG_ERR_BADCRC);
    }

    // verify that the chunk type is IHDR
    if (memcmp(pPngData+4, "IHDR", 4))
    {
        return(DIRTYPNG_ERR_BADTYPE);
    }
    
    // get width and height from image descriptor
    pPngHdr->uWidth = DIRTYPNG_GetInt32(pPngData, DATASTART);
    pPngHdr->uHeight = DIRTYPNG_GetInt32(pPngData, DATASTART+4);

    // get bit depth and colour type from the image descriptor
    pPngHdr->iBitDepth = pPngData[DATASTART+8];
    pPngHdr->iColourType = pPngData[DATASTART+9];

    // verify that the Bit Depth and Colour type combination is valid
    // check Table 11.1 of the PNG Specification for valid combinations
    switch(pPngHdr->iColourType)
    {
        case 0:
            pState->uPixelWidth = 1;
            if ((pPngHdr->iBitDepth != 1) && (pPngHdr->iBitDepth != 2) && (pPngHdr->iBitDepth != 4) &&
                (pPngHdr->iBitDepth != 8))
            {
                return(DIRTYPNG_ERR_BADDEPTH);
            }
            break;
        case 2:
            pState->uPixelWidth = 3;
            if (pPngHdr->iBitDepth != 8)
            {
                return(DIRTYPNG_ERR_BADDEPTH);
            }
            break;
        case 3:
            pState->uPixelWidth = 1;
            break;
        case 4:
            pState->uPixelWidth = 2;
            if (pPngHdr->iBitDepth != 8)
            {
                return(DIRTYPNG_ERR_BADDEPTH);
            }
            break;
        case 6:
            pState->uPixelWidth = 4;
            if (pPngHdr->iBitDepth != 8)
            {
                return(DIRTYPNG_ERR_BADDEPTH);
            }
            break;
        default:
            return(DIRTYPNG_ERR_BADCOLOR);
    }

    // get compression, filter, and interlace methods from the image descriptor
    pPngHdr->iCompression = pPngData[DATASTART+10];
    pPngHdr->iFilter = pPngData[DATASTART+11];
    pPngHdr->iInterlace = pPngData[DATASTART+12];

    // verify that each method specified is valid
    if (pPngHdr->iCompression != 0)
    {
        return(DIRTYPNG_ERR_BADCOMPR);
    }
    if (pPngHdr->iFilter != 0)
    {
        return(DIRTYPNG_ERR_BADFILTR);
    }
    if ((pPngHdr->iInterlace != 0) && (pPngHdr->iInterlace != 1))
    {
        return(DIRTYPNG_ERR_BADINTRL);
    }

    // setup the interlace arrays
    if (pPngHdr->iInterlace == 0)
    {
        pState->InterlacePassPixels[0] = pPngHdr->uWidth;
        pState->InterlaceScanlines[0] = pPngHdr->uHeight;
    }
    else
    {
        // compute how many pixels per scanline for each pass
        pState->InterlacePassPixels[0] = (pPngHdr->uWidth + 7) / 8;
        pState->InterlacePassPixels[1] = (pPngHdr->uWidth + 3) / 8;
        pState->InterlacePassPixels[2] = (pPngHdr->uWidth + 3) / 4;
        pState->InterlacePassPixels[3] = (pPngHdr->uWidth + 1) / 4;
        pState->InterlacePassPixels[4] = (pPngHdr->uWidth + 1) / 2;
        pState->InterlacePassPixels[5] = pPngHdr->uWidth / 2;
        pState->InterlacePassPixels[6] = pPngHdr->uWidth;

        // compute how many scanlines per pass
        pState->InterlaceScanlines[0] = (pPngHdr->uHeight + 7) / 8;
        pState->InterlaceScanlines[1] = (pPngHdr->uHeight + 7) / 8;
        pState->InterlaceScanlines[2] = (pPngHdr->uHeight + 3) / 8;
        pState->InterlaceScanlines[3] = (pPngHdr->uHeight + 3) / 4;
        pState->InterlaceScanlines[4] = (pPngHdr->uHeight + 1) / 4;
        pState->InterlaceScanlines[5] = (pPngHdr->uHeight + 1) / 2;
        pState->InterlaceScanlines[6] = pPngHdr->uHeight / 2;
    }
    
    // return success
    return(DIRTYPNG_ERR_NONE);
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngParseChunks

    \Description
        Parse the PNG chunks.

    \Input *pState      - pointer to the module state
    \Input *pPngHdr     - [out] pointer to PNG header to fill in
    \Input *pPngData    - pointer to PNG data
    \Input *pPngEnd     - pointer past the end of PNG data

    \Output
        int32_t         - nonnegative=success, negative=error

    \Version 02/06/2007 (cadam)
*/
/********************************************************************************F*/
static int32_t _DirtyPngParseChunks(DirtyPngStateT *pState, DirtyPngHdrT *pPngHdr, const uint8_t *pPngData, const uint8_t *pPngEnd)
{
    char strChunkType[5];
    uint32_t uLength;
    int32_t iEndReached = FALSE;

    pPngData += SIGNATURE_LEN + HEADER_LEN;

    do
    {
        //! make sure there is enough data left for another chunk
        DIRTYPNG_Validate(pPngData, pPngEnd, 12, DIRTYPNG_ERR_TOOSHORT);

        // get the chunk length
        uLength = DIRTYPNG_GetInt32(pPngData, 0);

        DIRTYPNG_Validate(pPngData, pPngEnd, (int32_t)uLength+12, DIRTYPNG_ERR_TOOSHORT);

        // copy the chunk type value from the PNG byte stream
        ds_memclr(strChunkType, sizeof(strChunkType));
        ds_strnzcpy(strChunkType, (const char *)pPngData+TYPESTART, (int32_t)sizeof(strChunkType));

        // calculate the chunk CRC and compare with the received CRC
        if (_DirtyPngCheckCRC(pState, pPngData, uLength) == FALSE)
        {
            return(DIRTYPNG_ERR_BADCRC);
        }

        #if DIRTYPNG_DEBUG_CHUNKS
        if (memcmp(strChunkType, "IDAT", 4) == 0)
        {
            _DirtyPngPrintChunk(pPngData, uLength, 0);
        }
        else
        {
            _DirtyPngPrintChunk(pPngData, uLength, 1);
        }
        #endif

        // check the chunk type and call its parser if required
        // checks to make sure the chunks occur in order and duplicates
        // of chunks where multiples are not allowed do not exist
        if (memcmp(strChunkType, "IDAT", 4) == 0)
        {
            #if DIRTYPNG_DEBUG_CHUNKS
            NetPrintf(("dirtypng: parsing chunk type = %s\n", strChunkType));
            #endif

            // make sure the IDAT chunks are sequential
            if (pState->uChunkIdat && !pState->uParsingIdat)
            {
                return(DIRTYPNG_ERR_BADORDER);
            }

            if (!pState->uChunkIdat)
            {
                pPngHdr->pImageData = pPngData;
                pState->uChunkIdat = TRUE;
                pState->uCountIdat = 1;
                pState->uParsingIdat = TRUE;
            }
            else
            {
                pState->uCountIdat++;
            }
        }
        else if (memcmp(strChunkType, "IEND", 4) == 0)
        {
            #if DIRTYPNG_DEBUG_CHUNKS
            NetPrintf(("dirtypng: parsing chunk type = %s\n", strChunkType));
            #endif

            // make sure there was at least one IDAT chunk
            if (!pState->uChunkIdat)
            {
                return(DIRTYPNG_ERR_TYPEMISS);
            }

            if (pState->uParsingIdat)
            {
                pPngHdr->pImageEnd = pPngData;
                pState->uParsingIdat = FALSE;
            }

            iEndReached = TRUE;
        }
        else if (memcmp(strChunkType, "PLTE", 4) == 0)
        {
            if (pState->uChunkIdat || pState->uChunkTrns || pState->uChunkBkgd)
            {
                return(DIRTYPNG_ERR_BADORDER);
            }
            if (pState->uChunkPlte)
            {
                return(DIRTYPNG_ERR_TYPEDUPL);
            }
            // calculate palette size
            pState->Palette.uNumColors = uLength/3;
            // palette length must be a multiple of three and no larger than 256
            if ((((pState->Palette.uNumColors)*3) != uLength) || (uLength > (256*3)))
            {
                NetPrintf(("dirtypng: palette size of %d is invalid\n", uLength));
                return(DIRTYPNG_ERR_BADDEPTH);
            }
            // copy palette data
            ds_memcpy_s(pState->Palette.aColor, sizeof(pState->Palette.aColor), pPngData+DATASTART, uLength);
            // mark as parsed, but only if this is a paletted image; otherwise the palette is optional and we treat the image as nonpaletted
            if (pState->uPixelWidth == 1)
            {
                pState->uChunkPlte = TRUE;
            }
        }
        else if (memcmp(strChunkType, "tRNS", 4) == 0)
        {
            #if DIRTYPNG_DEBUG_CHUNKS
            NetPrintf(("dirtypng: unsupported chunk type = %s\n", strChunkType));
            #endif

            if (pState->uChunkIdat)
            {
                return(DIRTYPNG_ERR_BADORDER);
            }
            if (pState->uChunkTrns)
            {
                return(DIRTYPNG_ERR_TYPEDUPL);
            }

            pState->uChunkTrns = TRUE;
        }
        else if (memcmp(strChunkType, "cHRM", 4) == 0)
        {
            #if DIRTYPNG_DEBUG_CHUNKS
            NetPrintf(("dirtypng: unsupported chunk type = %s\n", strChunkType));
            #endif

            if (pState->uChunkIdat || pState->uChunkPlte)
            {
                return(DIRTYPNG_ERR_BADORDER);
            }
            if (pState->uChunkChrm)
            {
                return(DIRTYPNG_ERR_TYPEDUPL);
            }

            pState->uChunkChrm = TRUE;
        }
        else if (memcmp(strChunkType, "gAMA", 4) == 0)
        {
            #if DIRTYPNG_DEBUG_CHUNKS
            NetPrintf(("dirtypng: unsupported chunk type = %s\n", strChunkType));
            #endif

            if (pState->uChunkIdat || pState->uChunkPlte)
            {
                return(DIRTYPNG_ERR_BADORDER);
            }
            if (pState->uChunkGama)
            {
                return(DIRTYPNG_ERR_TYPEDUPL);
            }

            pState->uChunkGama = TRUE;
        }
        else if (memcmp(strChunkType, "iCCP", 4) == 0)
        {
            #if DIRTYPNG_DEBUG_CHUNKS
            NetPrintf(("dirtypng: unsupported chunk type = %s\n", strChunkType));
            #endif

            if (pState->uChunkIdat || pState->uChunkPlte)
            {
                return(DIRTYPNG_ERR_BADORDER);
            }
            if (pState->uChunkIccp)
            {
                return(DIRTYPNG_ERR_TYPEDUPL);
            }

            pState->uChunkIccp = TRUE;
        }
        else if (memcmp(strChunkType, "sBIT", 4) == 0)
        {
            #if DIRTYPNG_DEBUG_CHUNKS
            NetPrintf(("dirtypng: unsupported chunk type = %s\n", strChunkType));
            #endif

            if (pState->uChunkIdat || pState->uChunkPlte)
            {
                return(DIRTYPNG_ERR_BADORDER);
            }
            if (pState->uChunkSbit)
            {
                return(DIRTYPNG_ERR_TYPEDUPL);
            }

            pState->uChunkSbit = TRUE;
        }
        else if (memcmp(strChunkType, "sRGB", 4) == 0)
        {
            #if DIRTYPNG_DEBUG_CHUNKS
            NetPrintf(("dirtypng: unsupported chunk type = %s\n", strChunkType));
            #endif

            if (pState->uChunkIdat || pState->uChunkPlte)
            {
                return(DIRTYPNG_ERR_BADORDER);
            }
            if (pState->uChunkIccp)
            {
                return(DIRTYPNG_ERR_TYPEDUPL);
            }

            pState->uChunkIccp = TRUE;
        }
        else if (memcmp(strChunkType, "tEXt", 4) == 0)
        {
            #if DIRTYPNG_DEBUG_CHUNKS
            NetPrintf(("dirtypng: unsupported chunk type = %s\n", strChunkType));
            #endif

            if (pState->uParsingIdat)
            {
                pPngHdr->pImageEnd = pPngData;
                pState->uParsingIdat = FALSE;
            }
        }
        else if (memcmp(strChunkType, "zTXt", 4) == 0)
        {
            #if DIRTYPNG_DEBUG_CHUNKS
            NetPrintf(("dirtypng: unsupported chunk type = %s\n", strChunkType));
            #endif

            if (pState->uParsingIdat)
            {
                pPngHdr->pImageEnd = pPngData;
                pState->uParsingIdat = FALSE;
            }
        }
        else if (memcmp(strChunkType, "iTXt", 4) == 0)
        {
            #if DIRTYPNG_DEBUG_CHUNKS
            NetPrintf(("dirtypng: unsupported chunk type = %s\n", strChunkType));
            #endif

            if (pState->uParsingIdat)
            {
                pPngHdr->pImageEnd = pPngData;
                pState->uParsingIdat = FALSE;
            }
        }
        else if (memcmp(strChunkType, "bKGD", 4) == 0)
        {
            #if DIRTYPNG_DEBUG_CHUNKS
            NetPrintf(("dirtypng: unsupported chunk type = %s\n", strChunkType));
            #endif

            if (pState->uChunkIdat)
            {
                return(DIRTYPNG_ERR_BADORDER);
            }
            if (pState->uChunkBkgd)
            {
                return(DIRTYPNG_ERR_TYPEDUPL);
            }

            pState->uChunkBkgd = TRUE;
        }
        else if (memcmp(strChunkType, "hIST", 4) == 0)
        {
            #if DIRTYPNG_DEBUG_CHUNKS
            NetPrintf(("dirtypng: unsupported chunk type = %s\n", strChunkType));
            #endif

            if (!pState->uChunkPlte)
            {
                return(DIRTYPNG_ERR_TYPEMISS);
            }
            if (pState->uChunkIdat)
            {
                return(DIRTYPNG_ERR_BADORDER);
            }
        }
        else if (memcmp(strChunkType, "pHYs", 4) == 0)
        {
            #if DIRTYPNG_DEBUG_CHUNKS
            NetPrintf(("dirtypng: unsupported chunk type = %s\n", strChunkType));
            #endif

            if (pState->uChunkIdat)
            {
                return(DIRTYPNG_ERR_BADORDER);
            }
            if (pState->uChunkPhys)
            {
                return(DIRTYPNG_ERR_TYPEDUPL);
            }

            pState->uChunkPhys = TRUE;
        }
        else if (memcmp(strChunkType, "sPLT", 4) == 0)
        {
            #if DIRTYPNG_DEBUG_CHUNKS
            NetPrintf(("dirtypng: unsupported chunk type = %s\n", strChunkType));
            #endif

            if (pState->uChunkIdat)
            {
                return(DIRTYPNG_ERR_BADORDER);
            }
        }
        else if (memcmp(strChunkType, "tIME", 4) == 0)
        {
            #if DIRTYPNG_DEBUG_CHUNKS
            NetPrintf(("dirtypng: unsupported chunk type = %s\n", strChunkType));
            #endif

            if (pState->uParsingIdat)
            {
                pPngHdr->pImageEnd = pPngData;
                pState->uParsingIdat = FALSE;
            }

            pState->uChunkTime = TRUE;
        }
        else
        {
            #if DIRTYPNG_DEBUG_CHUNKS
            NetPrintf(("dirtypng: unknown chunk type = %s\n", strChunkType));
            #endif

            // if the 5th bit of the first byte of the chunk is 0 it is
            // a critical chunk and it cannot be skipped, return an error
            if (!(strChunkType[0] & 0x20))
            {
                return(DIRTYPNG_ERR_UNKNCRIT);
            }
        }

        pPngData += uLength + 12;
    } while (!iEndReached);

    #if DIRTYPNG_DEBUG_CHUNKS
    NetPrintf(("dirtypng: Parsing Complete\n\n"));
    #endif

    // return success
    return(DIRTYPNG_ERR_NONE);
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngGetNextBytes

    \Description
        Check if there is another iBytes bytes available and add it to DataBits.

    \Input *pState      - pointer to the module state
    \Input *pPngHdr     - pointer to header describing png to decode
    \Input iBytes       - byte count

    \Output
        int32_t         - nonnegative=success, negative=error

    \Version 02/14/2007 (cadam)
*/
/********************************************************************************F*/
static int32_t _DirtyPngGetNextBytes(DirtyPngStateT *pState, DirtyPngHdrT *pPngHdr, int32_t iBytes)
{
    int32_t i;

    for (i = 0; i < iBytes; i++)
    {
        pState->uByteOffset++;

        if (pState->uByteOffset == pState->uIdatStart + pState->uIdatLength)
        {
            pState->uCountIdat--;

            if (!pState->uCountIdat && !pState->uBlockEnd)
            {
                return(DIRTYPNG_ERR_NOBLKEND);
            }
            else if (pState->uCountIdat)
            {
                pState->uIdatLength = DIRTYPNG_GetInt32(pPngHdr->pImageData, pState->uByteOffset+4);
                pState->uIdatStart = pState->uByteOffset = pState->uByteOffset + 12;
            }
        }

        pState->uDataBits += ((uint32_t)(pPngHdr->pImageData[pState->uByteOffset]) << pState->uBitsLeft);
        pState->uBitsLeft += 8;
    }

    return(DIRTYPNG_ERR_NONE);
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngRemoveBits

    \Description
        Remove the specified number of bits from the DataBits.

    \Input *pState      - pointer to the module state
    \Input iBits        - bit count

    \Version 02/14/2007 (cadam)
*/
/********************************************************************************F*/
static void _DirtyPngRemoveBits(DirtyPngStateT *pState, int32_t iBits)
{
    pState->uDataBits >>= iBits;
    pState->uBitsLeft -= iBits;
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngReverseBits

    \Description
        Reverse the bits in a uint16_t.

    \Input uValue       - uint16_t to reverse
    \Input uBits        - number of bits to reverse

    \Output
        uint16_t        - the reversed value of the passed in uint16_t

    \Version 02/14/2007 (cadam)
*/
/********************************************************************************F*/
static uint16_t _DirtyPngReverseBits(uint16_t uValue, uint16_t uBits)
{
    uint16_t u, uReversed = 0;

    uReversed = uValue & 1;

    for (u = 1; u < uBits; u++)
    {
        uReversed <<= 1;
        uValue >>= 1;
        uReversed |= (uValue & 1);
    }

    return(uReversed);
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngParseZlibInfo

    \Description
        Parse the zlib CMF and FLG bytes.

    \Input *pState      - pointer to the module state
    \Input *pPngHdr     - pointer to header describing png to decode

    \Output
        int32_t         - nonnegative=success, negative=error

    \Version 02/13/2007 (cadam)
*/
/********************************************************************************F*/
static int32_t _DirtyPngParseZlibInfo(DirtyPngStateT *pState, DirtyPngHdrT *pPngHdr)
{
    #if DIRTYPNG_DEBUG_ZLIB
    NetPrintf(("\n\ndirtypng: parsing zlib info\n"));
    #endif

    // make sure there are at least enough space for the CMF and FLG bytes
    if ((pState->uIdatLength = DIRTYPNG_GetInt32(pPngHdr->pImageData, 0)) < 2)
    {
        return(DIRTYPNG_ERR_TOOSHORT);
    }

    pState->uIdatStart = DATASTART;

    // make sure bits 0 to 3 of the CMF byte (CM) contains a value of 8 (Most-Significant Bit first)
    if ((pPngHdr->pImageData[DATASTART] & 0x0f) != 8)
    {
        return(DIRTYPNG_ERR_BADCM);
    }

    #if DIRTYPNG_DEBUG_ZLIB
    NetPrintf(("dirtypng: CM=%u\n", pPngHdr->pImageData[DATASTART] & 0x0f));
    #endif

    // make sure bits 4 to 7 of the CMF byte (CINFO) specifies a value of 7 or less
    pState->uWindowLog = pPngHdr->pImageData[DATASTART] >> 4;
    if (pState->uWindowLog >= 8)
    {
        return(DIRTYPNG_ERR_BADCI);
    }
    pState->uWindowSize = 1 << (pState->uWindowLog + 8);

    #if DIRTYPNG_DEBUG_ZLIB
    NetPrintf(("dirtypng: CINFO=%u\n\n", pPngHdr->pImageData[DATASTART] >> 4));
    #endif

    // the CMF byte * 256 + the FLG byte should be a multiple of 31
    if ((((uint16_t)pPngHdr->pImageData[DATASTART]) << 8 | pPngHdr->pImageData[DATASTART+1]) % 31 != 0)
    {
        return(DIRTYPNG_ERR_BADFLG);
    }

    if (pPngHdr->pImageData[DATASTART+1] & 0x20)
    {
        return(DIRTYPNG_ERR_FDICTSET);
    }

    pState->uByteOffset = DATASTART + 1;

    return(DIRTYPNG_ERR_NONE);
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngParseDeflateInfo

    \Description
        Parse the deflate information.

    \Input *pState      - pointer to the module state
    \Input *pPngHdr     - pointer to header describing png to decode

    \Output
        int32_t         - nonnegative=success, negative=error

    \Version 02/13/2007 (cadam)
*/
/********************************************************************************F*/
static int32_t _DirtyPngParseDeflateInfo(DirtyPngStateT *pState, DirtyPngHdrT *pPngHdr)
{
    int32_t iRetVal;

    #if DIRTYPNG_DEBUG_DEFLATE
    NetPrintf(("dirtypng: parsing deflate info\n"));
    #endif

    if (pState->uBitsLeft < 3)
    {
        if ((iRetVal = _DirtyPngGetNextBytes(pState, pPngHdr, 1)) != DIRTYPNG_ERR_NONE)
        {
            return(iRetVal);
        }
    }

    iRetVal = pState->uDataBits & 1;
    _DirtyPngRemoveBits(pState, 1);

    pState->uBType = pState->uDataBits & ((1 << 2) - 1);
    _DirtyPngRemoveBits(pState, 2);

    #if DIRTYPNG_DEBUG_DEFLATE
    NetPrintf(("dirtypng: BFINAL=%u\n", iRetVal));
    NetPrintf(("dirtypng: BTYPE=%u\n\n", pState->uBType));
    #endif

    if (pState->uBType == 3)
    {
        iRetVal = DIRTYPNG_ERR_BADBTYPE;
    }

    return(iRetVal);
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngWriteScanline

    \Description
        Write the original scanline into the output buffer.

    \Input *pState      - pointer to the module state
    \Input *pPngHdr     - pointer to header describing png to decode

    \Version 05/11/2007 (cadam)
*/
/********************************************************************************F*/
static void _DirtyPngWriteScanline(DirtyPngStateT *pState, DirtyPngHdrT *pPngHdr)
{
    uint32_t u = 0, uScanlineOffset = 0, uScanlineWidth, uPixelWidth;
    uint8_t uShiftsPerByte = 8 / pPngHdr->iBitDepth;
    uint8_t uShift = uShiftsPerByte - 1;
    uint8_t uBits = (1 << pPngHdr->iBitDepth) - 1;

    uPixelWidth = !pState->uChunkPlte ? pState->uPixelWidth : 4;
    uScanlineWidth = uPixelWidth * pState->InterlacePassPixels[pState->uCurrentPass];

    if (pPngHdr->iInterlace == 1)
    {
        uint32_t uA7HeightOffset = 0, uA7WidthOffset = 0, uA7WidthShift = 0;

        // setup the width shift, width offset, and height offset for the current Adam7 scanline
        switch(pState->uCurrentPass)
        {
            case 0:
                uA7HeightOffset = 8 * pState->uCurrentScanline * uPixelWidth * pPngHdr->uWidth;
                uA7WidthShift = 7 * uPixelWidth;
                break;
            case 1:
                uA7WidthOffset = 4 * uPixelWidth;
                uA7HeightOffset = 8 * pState->uCurrentScanline * uPixelWidth * pPngHdr->uWidth;
                uA7WidthShift = 7 * uPixelWidth;
                break;
            case 2:
                uA7HeightOffset = (8 * pState->uCurrentScanline + 4) * uPixelWidth * pPngHdr->uWidth;
                uA7WidthShift = 3 * uPixelWidth;
                break;
            case 3:
                uA7WidthOffset = 2 * uPixelWidth;
                uA7HeightOffset = 4 * pState->uCurrentScanline * uPixelWidth * pPngHdr->uWidth;
                uA7WidthShift = 3 * uPixelWidth;
                break;
            case 4:
                uA7HeightOffset = (4 * pState->uCurrentScanline + 2 )* uPixelWidth * pPngHdr->uWidth;
                uA7WidthShift = uPixelWidth;
                break;
            case 5:
                uA7WidthOffset = 1 * uPixelWidth;
                uA7HeightOffset = 2 * pState->uCurrentScanline * uPixelWidth * pPngHdr->uWidth;
                uA7WidthShift = uPixelWidth;
                break;
            case 6:
                uA7HeightOffset = (2 * pState->uCurrentScanline + 1)* uPixelWidth * pPngHdr->uWidth;
                break;
        }

        while (u < uScanlineWidth)
        {
            uint32_t uValue = ((pState->pCurrScanline[uScanlineOffset] >> (pPngHdr->iBitDepth * uShift)) & uBits) * (255 / uBits);
            uint32_t uOffset = uA7HeightOffset + uA7WidthOffset;

            if (!pState->uChunkPlte)
            {
                pState->pImageBuf[uOffset] = uValue;
                pState->uImageOffset += 1;
                uA7WidthOffset += 1;
                u += 1;
            }
            else
            {
                pState->pImageBuf[uOffset+0] = 0xff;
                pState->pImageBuf[uOffset+1] = pState->Palette.aColor[uValue][0];
                pState->pImageBuf[uOffset+2] = pState->Palette.aColor[uValue][1];
                pState->pImageBuf[uOffset+3] = pState->Palette.aColor[uValue][2];;
                pState->uImageOffset += 4;
                uA7WidthOffset += 4;
                u += 4;
            }

            if (uShift == 0)
            {
                uShift = uShiftsPerByte - 1;
                uScanlineOffset++;
            }
            else
            {
                uShift--;
            }
            
            // for each pixel written make sure to shift to the next pixel offset for the current Adam7 scanline
            if (u % uPixelWidth == 0)
            {
                uA7WidthOffset += uA7WidthShift;
            }
        }
    }
    else
    {
        while (u < uScanlineWidth)
        {
            uint8_t uValue = ((pState->pCurrScanline[uScanlineOffset] >> (pPngHdr->iBitDepth * uShift)) & uBits) * (255 / uBits);

            if (!pState->uChunkPlte)
            {
                pState->pImageBuf[pState->uImageOffset] = uValue;
                pState->uImageOffset += 1;
                u += 1;
            }
            else
            {
                pState->pImageBuf[pState->uImageOffset+0] = 0xff;
                pState->pImageBuf[pState->uImageOffset+1] = pState->Palette.aColor[uValue][0];
                pState->pImageBuf[pState->uImageOffset+2] = pState->Palette.aColor[uValue][1];
                pState->pImageBuf[pState->uImageOffset+3] = pState->Palette.aColor[uValue][2];
                pState->uImageOffset += 4;
                u += 4;
            }

            if (uShift == 0)
            {
                uShift = uShiftsPerByte - 1;
                uScanlineOffset++;
            }
            else
            {
                uShift--;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngGetPaeth

    \Description
        Get the distance total from the bits after the length code.

    \Input a            - the byte one pixel to the left of x
    \Input b            - the byte at the offset of x in the previous scanline
    \Input c            - the byte at the offset of a in the previous scanline

    \Output
        uint8_t         - nonnegative=success, zero=error

    \Version 02/13/2007 (cadam)
*/
/********************************************************************************F*/
static uint8_t _DirtyPngGetPaeth(uint8_t a, uint8_t b, uint8_t c)
{
    int16_t p, pa, pb, pc;

    p = (int16_t)a + (int16_t)b - (int16_t)c;
    pa = p - (int16_t)a;
    pb = p - (int16_t)b;
    pc = p - (int16_t)c;

    if (pa < 0)
    {
        pa = -pa;
    }
    if (pb < 0)
    {
        pb = -pb;
    }
    if (pc < 0)
    {
        pc = -pc;
    }

    if (pa <= pb && pa <= pc)
    {
        return(a);
    }
    else if (pb <= pc)
    {
        return(b);
    }
    else
    {
        return(c);
    }
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngFilterLine

    \Description
        Reconstruct the original line.

    \Input *pState      - pointer to the module state
    \Input *pPngHdr     - pointer to header describing png to decode

    \Version 05/07/2007 (cadam)
*/
/********************************************************************************F*/
static void _DirtyPngFilterLine(DirtyPngStateT *pState, DirtyPngHdrT *pPngHdr)
{
    uint8_t *pTempScanline;
    uint32_t u, uScanlineEnd, uOffsetX = 0;

    uScanlineEnd = (pState->uPixelWidth * pState->InterlacePassPixels[pState->uCurrentPass] * pPngHdr->iBitDepth + 8 - pPngHdr->iBitDepth) / 8;

    switch(pState->uFilterType)
    {
        case 1:
            uOffsetX = pState->uPixelWidth;
            while (uOffsetX != uScanlineEnd)
            {
                pState->pCurrScanline[uOffsetX] = pState->pCurrScanline[uOffsetX] + pState->pCurrScanline[uOffsetX - pState->uPixelWidth];
                uOffsetX++;
            }
            break;
        case 2:
            while (uOffsetX != uScanlineEnd)
            {
                pState->pCurrScanline[uOffsetX] = pState->pCurrScanline[uOffsetX] + pState->pPrevScanline[uOffsetX];
                uOffsetX++;
            }
            break;
        case 3:
            for (u = 0; u < pState->uPixelWidth; u++)
            {
                pState->pCurrScanline[uOffsetX] = pState->pCurrScanline[uOffsetX] + (pState->pPrevScanline[uOffsetX] >> 1);
                uOffsetX++;
            }
            while (uOffsetX != uScanlineEnd)
            {
                pState->pCurrScanline[uOffsetX] = pState->pCurrScanline[uOffsetX] + (uint8_t)(((uint16_t)pState->pCurrScanline[uOffsetX - pState->uPixelWidth] +
                                                                                               (uint16_t)pState->pPrevScanline[uOffsetX]) >> 1);
                uOffsetX++;
            }
            break;
        case 4:
            for (u = 0; u < pState->uPixelWidth; u++)
            {
                pState->pCurrScanline[uOffsetX] = pState->pCurrScanline[uOffsetX] + pState->pPrevScanline[uOffsetX];
                uOffsetX++;
            }
            while (uOffsetX != uScanlineEnd)
            {
                pState->pCurrScanline[uOffsetX] = pState->pCurrScanline[uOffsetX] + _DirtyPngGetPaeth(pState->pCurrScanline[uOffsetX - pState->uPixelWidth],
                                                                                                      pState->pPrevScanline[uOffsetX],
                                                                                                      pState->pPrevScanline[uOffsetX - pState->uPixelWidth]);
                uOffsetX++;
            }
            break;
    };

    // write the scanline to the image buffer
    _DirtyPngWriteScanline(pState, pPngHdr);

    // swap the scanlines then clear the current
    pTempScanline = pState->pPrevScanline;
    pState->pPrevScanline = pState->pCurrScanline;
    pState->pCurrScanline = pTempScanline;
    ds_memclr(pState->pCurrScanline, pState->uScanlineWidth);
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngNoCompBlock

    \Description
        Parse the length field and validate, then copy the data to the image buffer

    \Input *pState      - pointer to the module state
    \Input *pPngHdr     - pointer to header describing png to decode

    \Output
        int32_t         - nonnegative=success, negative=error

    \Version 05/02/2007 (cadam)
*/
/********************************************************************************F*/
static int32_t _DirtyPngNoCompBlock(DirtyPngStateT *pState, DirtyPngHdrT *pPngHdr)
{
    int32_t iRetVal;
    uint16_t u, uBlockLen;

    _DirtyPngRemoveBits(pState, pState->uBitsLeft);

    if ((iRetVal = _DirtyPngGetNextBytes(pState, pPngHdr, 4)) != DIRTYPNG_ERR_NONE)
    {
        return(iRetVal);
    }

    uBlockLen = (uint16_t)(pState->uDataBits & 0xffff);
    _DirtyPngRemoveBits(pState, 16);

    if (uBlockLen != (uint16_t)~pState->uDataBits)
    {
        return(DIRTYPNG_ERR_BADBLKLEN);
    }

    _DirtyPngRemoveBits(pState, pState->uBitsLeft);

    for (u = 0; u < uBlockLen; u++)
    {
        if ((iRetVal = _DirtyPngGetNextBytes(pState, pPngHdr, 1)) != DIRTYPNG_ERR_NONE)
        {
            return(iRetVal);
        }

        pState->SlidingWindow[pState->uWindowOffset++] = (uint8_t)pState->uDataBits;

        // wrap around the window if needed
        if (pState->uWindowOffset == pState->uWindowSize)
        {
            pState->uWindowOffset = 0;
        }

        // increment the uWindowLength if we haven't already reached the max length
        if (pState->uWindowLength < pState->uWindowSize)
        {
            pState->uWindowLength++;
        }

        // skip the filter byte
        if (pState->uWidthOffset == 0)
        {
            pState->uFilterType = (int8_t)pState->uDataBits;
            // make sure the filter type is valid
            if (pState->uFilterType > 4)
            {
                return(DIRTYPNG_ERR_BADTYPE);
            }
            pState->uWidthOffset++;
        }
        else
        {
            pState->pCurrScanline[pState->uWidthOffset++ - 1] = (uint8_t)pState->uDataBits;
                        
            if (pState->uWidthOffset - 1 == (pState->uPixelWidth * pState->InterlacePassPixels[pState->uCurrentPass] * pPngHdr->iBitDepth + 8 - pPngHdr->iBitDepth) / 8)
            {
                pState->uWidthOffset = 0;
                _DirtyPngFilterLine(pState, pPngHdr);

                pState->InterlaceScanlines[pState->uCurrentPass]--;
                pState->uCurrentScanline++;

                if (pState->InterlaceScanlines[pState->uCurrentPass] == 0)
                {
                    pState->uCurrentPass++;
                    pState->uCurrentScanline = 0;
                    ds_memclr(pState->pPrevScanline, pState->uScanlineWidth);
                }
                while ((pState->uCurrentPass < 7) && (pState->InterlaceScanlines[pState->uCurrentPass] == 0))
                {
                    pState->uCurrentPass++;
                }

                if ((pState->uCurrentPass == 7) && (u + 1 != uBlockLen))
                {
                    return(DIRTYPNG_ERR_INVFILE);
                }
            }
        }

        _DirtyPngRemoveBits(pState, pState->uBitsLeft);
    }

    return(DIRTYPNG_ERR_NONE);
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngFixedBlock

    \Description
        Use the default huffman codes.  These codes are specified in section 3.2.6 of rfc1951.
        Literal Information:
            0-143       00110000->10111111      (8 bits)
            144-255     110010000->111111111    (9 bits)
            256-279     0000000->0010111        (7 bits)
            280-287     11000000->11000111      (8 bits)
        Distance Information:
            0-31        00000->11111            (5 bits)
            Distance codes 30 and 31 will never appear in the compressed data

    \Input *pState      - pointer to the module state

    \Output
        int32_t         - nonnegative=success, negative=error

    \Version 02/13/2007 (cadam)
*/
/********************************************************************************F*/
static int32_t _DirtyPngFixedBlock(DirtyPngStateT *pState)
{
    // form the table with the fixed codes-literal value combinations
    // found in section 3.2.6 of RFC1951
    uint16_t u, uCodeValue;

    pState->uLitMinBits = 7;
    pState->uDistMinBits = 5;

    // set no codes to 1 for all bit lengths.  7, 8 and 9 bit codes will get set to zero later.
    for (u = 0; u < 16; u++)
    {
        pState->LitTables[u].uNoCodes = 1;
        pState->DistTables[u].uNoCodes = 1;
    }

    // setup the literal/length table
    pState->LitTables[7].uNoCodes = 0;
    pState->LitTables[7].uMinCode = 0;
    pState->LitTables[7].uMaxCode = 23;

    uCodeValue = 256;
    for (u = 0; u < 24; u++)
    {
        pState->LitTables[7].Codes[u] = uCodeValue++;
    }

    pState->LitTables[8].uNoCodes = 0;
    pState->LitTables[8].uMinCode = 48;
    pState->LitTables[8].uMaxCode = 199;

    uCodeValue = 0;
    for (u = 0; u < 144; u++)
    {
        pState->LitTables[8].Codes[u] = uCodeValue++;
    }
    uCodeValue = 280;
    for (; u < 152; u++)
    {
        pState->LitTables[8].Codes[u] = uCodeValue++;
    }

    pState->LitTables[9].uNoCodes = 0;
    pState->LitTables[9].uMinCode = 400;
    pState->LitTables[9].uMaxCode = 511;

    uCodeValue = 144;
    for (u = 0; u < 112; u++)
    {
        pState->LitTables[9].Codes[u] = uCodeValue++;
    }

    // setup the distance table
    pState->DistTables[5].uNoCodes = 0;
    pState->DistTables[5].uMinCode = 0;
    pState->DistTables[5].uMaxCode = 29; // 29 since 30 and 31 shouldn't occur

    for (u = 0; u < 30; u++)
    {
        pState->DistTables[5].Codes[u] = u;
    }

    return(DIRTYPNG_ERR_NONE);
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngDynamicBlock

    \Description
        Parse and setup the dynamic huffman table

    \Input *pState      - pointer to the module state
    \Input *pPngHdr     - pointer to header describing png to decode

    \Output
        int32_t         - nonnegative=success, negative=error

    \Version 02/13/2007 (cadam)
*/
/********************************************************************************F*/
static int32_t _DirtyPngDynamicBlock(DirtyPngStateT *pState, DirtyPngHdrT *pPngHdr)
{
    int32_t iRetVal;
    uint16_t u, v, uReversed, uMinimumBits = 0;
    uint16_t uCodeFound, uCodeOffset, uCodeOverlap = 0, uCodeRepeat = 0, uCodeValue, uPrevCode = 0;
    uint8_t CodeLength[MAXCLEN];
    uint16_t CodeCount[MAXCLEN];

    ds_memclr(CodeLength, MAXCLEN);
    ds_memclr(CodeCount, MAXCLEN * 2);

    if ((iRetVal = _DirtyPngGetNextBytes(pState, pPngHdr, ((14 - pState->uBitsLeft) >> 3) + 1)) != DIRTYPNG_ERR_NONE)
    {
        return(iRetVal);
    }

    pState->uHLit = (pState->uDataBits & 0x1f) + 257;
    _DirtyPngRemoveBits(pState, 5);

    pState->uHDist = (pState->uDataBits & 0x1f) + 1;
    _DirtyPngRemoveBits(pState, 5);

    pState->uHCLen = (pState->uDataBits & 0x0f) + 4;
    _DirtyPngRemoveBits(pState, 4);

    if (pState->uHLit > 286)
    {
        return(DIRTYPNG_ERR_MAXCODES);
    }

    #if DIRTYPNG_DEBUG_DYNAMIC
    NetPrintf(("dirtypng: parsing Dynamic Block Info\n"));
    NetPrintf(("dirtypng: HLit=%u\n", pState->uHLit));
    NetPrintf(("dirtypng: HDist=%u\n", pState->uHDist));
    NetPrintf(("dirtypng: HCLen=%u\n\n", pState->uHCLen));
    #endif

    // get the length codes for the code length alphabet
    for (u = 0; u < pState->uHCLen; u++)
    {
        if (pState->uBitsLeft < 3)
        {
            if ((iRetVal = _DirtyPngGetNextBytes(pState, pPngHdr, 1)) != DIRTYPNG_ERR_NONE)
            {
                return(iRetVal);
            }
        }

        CodeLength[_HCLEN_Codes[u]] = pState->uDataBits & 7;
        _DirtyPngRemoveBits(pState, 3);
        CodeCount[CodeLength[_HCLEN_Codes[u]]]++;
    }
    for ( ; u < 19; u++)
    {
        CodeLength[_HCLEN_Codes[u]] = 0;
    }

    uCodeValue = 0;
    // zero code lengths are ignored
    pState->CLenTables[0].uNoCodes = 1;
    for (u = 1; u < 8; u++)
    {
        if (CodeCount[u] > 0)
        {
            if (uMinimumBits == 0)
            {
                uMinimumBits = u;
            }
            pState->CLenTables[u].uMinCode = uCodeValue;
            pState->CLenTables[u].uMaxCode = uCodeValue - 1;
            uCodeValue = (uCodeValue + CodeCount[u]) << 1;
            pState->CLenTables[u].uNoCodes = 0;
        }
        else
        {
            uCodeValue <<= 1;
            pState->CLenTables[u].uNoCodes = 1;
        }
    }

    // if there are no code lengths set return a NOCODES error
    if (uMinimumBits == 0)
    {
        return(DIRTYPNG_ERR_NOCODES);
    }

    for (u = 0; u < MAXCLEN; u++)
    {
        if (CodeLength[u] > 0)
        {
            uCodeOffset = pState->CLenTables[CodeLength[u]].uMaxCode - pState->CLenTables[CodeLength[u]].uMinCode + 1;
            pState->CLenTables[CodeLength[u]].Codes[uCodeOffset] = u;
            pState->CLenTables[CodeLength[u]].uMaxCode++;
        }
    }

    #if DIRTYPNG_DEBUG_DYNAMIC
    NetPrintf(("dirtypng: CodeLength"));
    for (u = 0; u < MAXCLEN; u++)
    {
        if (u % 5 == 0)
        {
            NetPrintf(("\ndirtypng: "));
        }

        NetPrintf(("%u ", CodeLength[u]));
    }
    NetPrintf(("\n\n"));

    _DirtyPngPrintCLenTables(pState);
    #endif

    // reset the code count
    ds_memclr(CodeCount, MAXCLEN * 2);

    // construct the literal/length and distance tables
    for (u = 0; u < pState->uHLit + pState->uHDist; u++)
    {
        v = uMinimumBits;
        uCodeFound = 0;

        do
        {
            while ((pState->CLenTables[v].uNoCodes == 1) && (v < 8))
            {
                v++;
            }
            if (v == 8)
            {
                return(DIRTYPNG_ERR_BADCODE);
            }

            if (pState->uBitsLeft < v)
            {
                if ((iRetVal = _DirtyPngGetNextBytes(pState, pPngHdr, 1)) != DIRTYPNG_ERR_NONE)
                {
                    return(iRetVal);
                }
            }

            uReversed = _DirtyPngReverseBits(pState->uDataBits, v);

            if ((uReversed >= pState->CLenTables[v].uMinCode) && (uReversed <=  pState->CLenTables[v].uMaxCode))
            {
                // code found so remove the bits
                _DirtyPngRemoveBits(pState, v);

                uCodeFound = 1;

                uCodeValue = pState->CLenTables[v].Codes[uReversed - pState->CLenTables[v].uMinCode];
                
                // details of the code length values found in RFC1951
                if ((uCodeValue > 0) && (uCodeValue < 16))
                {
                    if (u < pState->uHLit)
                    {
                        pState->LitTables[uCodeValue].Codes[CodeCount[uCodeValue]++] = u;
                    }
                    else
                    {
                        if (uCodeOverlap == 0)
                        {
                            // set the min/max codes for the literal/length Tables
                            uCodeOffset = 0;
                            pState->uLitMinBits = 0;
                            pState->LitTables[0].uNoCodes = 1;
                            for (v = 1; v < 16; v++)
                            {
                                if (CodeCount[v] != 0)
                                {
                                    if (pState->uLitMinBits == 0)
                                    {
                                        pState->uLitMinBits = v;
                                    }
                                    pState->LitTables[v].uMinCode = uCodeOffset;
                                    pState->LitTables[v].uMaxCode = CodeCount[v] + uCodeOffset - 1;
                                    uCodeOffset = (CodeCount[v] + uCodeOffset) << 1;
                                    pState->LitTables[v].uNoCodes = 0;
                                }
                                else
                                {
                                    uCodeOffset <<= 1;
                                    pState->LitTables[v].uNoCodes = 1;
                                }
                            }

                            // reset the code count
                            ds_memclr(CodeCount, MAXCLEN * 2);

                            uCodeOverlap = 1;
                        }
                        
                        pState->DistTables[uCodeValue].Codes[CodeCount[uCodeValue]++] = u - pState->uHLit;
                    }

                    uPrevCode = uCodeValue;
                }
                else if (uCodeValue == 16)
                {
                    // a code length of 16 cannot occur on the first code
                    if (u == 0)
                    {
                        return(DIRTYPNG_ERR_BADCODE);
                    }

                    if (pState->uBitsLeft < 2)
                    {
                        if ((iRetVal = _DirtyPngGetNextBytes(pState, pPngHdr, 1)) != DIRTYPNG_ERR_NONE)
                        {
                            return(iRetVal);
                        }
                    }

                    uCodeRepeat = 3 + (pState->uDataBits & ((1 << 2) - 1));
                    _DirtyPngRemoveBits(pState, 2);
                }
                else if (uCodeValue == 17)
                {
                    if (pState->uBitsLeft < 3)
                    {
                        if ((iRetVal = _DirtyPngGetNextBytes(pState, pPngHdr, 1)) != DIRTYPNG_ERR_NONE)
                        {
                            return(iRetVal);
                        }
                    }

                    uPrevCode = 0;
                    uCodeRepeat = 3 + (pState->uDataBits & ((1 << 3) - 1));
                    _DirtyPngRemoveBits(pState, 3);
                }
                else if (uCodeValue == 18)
                {
                    if (pState->uBitsLeft < 7)
                    {
                        if ((iRetVal = _DirtyPngGetNextBytes(pState, pPngHdr, 1)) != DIRTYPNG_ERR_NONE)
                        {
                            return(iRetVal);
                        }
                    }

                    uPrevCode = 0;
                    uCodeRepeat = 11 + (pState->uDataBits & ((1 << 7) - 1));
                    _DirtyPngRemoveBits(pState, 7);
                }

                if (uCodeRepeat > 0)
                {
                    // check to make sure the repeat value is valid
                    if ((u + uCodeRepeat) > (pState->uHLit + pState->uHDist))
                    {
                        return(DIRTYPNG_ERR_BADCODE);
                    }

                    if (uPrevCode == 0)
                    {
                        u += uCodeRepeat - 1;
                        uCodeRepeat = 0;
                    }
                    else
                    {
                        while (uCodeRepeat != 0)
                        {
                            if (u < pState->uHLit)
                            {
                                pState->LitTables[uPrevCode].Codes[CodeCount[uPrevCode]++] = u;
                            }
                            else
                            {
                                if (uCodeOverlap == 0)
                                {
                                    // set the min/max codes for the literal/length Tables
                                    uCodeOffset = 0;
                                    pState->uLitMinBits = 0;
                                    pState->LitTables[0].uNoCodes = 1;
                                    for (v = 1; v < 16; v++)
                                    {
                                        if (CodeCount[v] != 0)
                                        {
                                            if (pState->uLitMinBits == 0)
                                            {
                                                pState->uLitMinBits = v;
                                            }
                                            pState->LitTables[v].uMinCode = uCodeOffset;
                                            pState->LitTables[v].uMaxCode = CodeCount[v] + uCodeOffset - 1;
                                            uCodeOffset = (CodeCount[v] + uCodeOffset) << 1;
                                            pState->LitTables[v].uNoCodes = 0;
                                        }
                                        else
                                        {
                                            uCodeOffset <<= 1;
                                            pState->LitTables[v].uNoCodes = 1;
                                        }
                                    }

                                    // there should always be at least one code
                                    if (pState->uLitMinBits == 0)
                                    {
                                        return(DIRTYPNG_ERR_NOCODES);
                                    }

                                    pState->DistTables[uPrevCode].Codes[0] = u - pState->uHLit;

                                    // reset the code count
                                    ds_memclr(CodeCount, MAXCLEN * 2);

                                    CodeCount[uPrevCode] = 1;
                                    uCodeOverlap = 1;
                                }
                                else
                                {
                                    pState->DistTables[uPrevCode].Codes[CodeCount[uPrevCode]++] = u - pState->uHLit;
                                }
                            }

                            uCodeRepeat--;
                            u++;
                        }
                        
                        // make sure to decrement u by one since the loop increments it at the end
                        u--;
                    }
                }
            }

            v++;
        } while ((uCodeFound == 0) && (v < 8));

        if (uCodeFound == 0)
        {
            return(DIRTYPNG_ERR_BADCODE);
        }
    }

    // set the min/max codes for the distance tables
    uCodeOffset = 0;
    pState->uDistMinBits = 0;
    pState->DistTables[0].uNoCodes = 1;
    for (u = 1; u < 16; u++)
    {
        if (CodeCount[u] != 0)
        {
            if (pState->uDistMinBits == 0)
            {
                pState->uDistMinBits = u;
            }
            pState->DistTables[u].uMinCode = uCodeOffset;
            pState->DistTables[u].uMaxCode = CodeCount[u] + uCodeOffset - 1;
            uCodeOffset = (CodeCount[u] + uCodeOffset) << 1;
            pState->DistTables[u].uNoCodes = 0;
        }
        else
        {
            uCodeOffset <<= 1;
            pState->DistTables[u].uNoCodes = 1;
        }
    }

    if (pState->uDistMinBits == 0)
    {
        return(DIRTYPNG_ERR_NOCODES);
    }

    #if DIRTYPNG_DEBUG_DYNAMIC
    _DirtyPngPrintLitTables(pState);
    _DirtyPngPrintDistTables(pState);
    #endif

    return(DIRTYPNG_ERR_NONE);
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngGetDistance

    \Description
        Get the distance total from the bits after the length code.

    \Input *pState      - pointer to the module state
    \Input *pPngHdr     - pointer to header describing png to decode

    \Output
        uint32_t         - nonnegative=success, zero=error

    \Version 02/13/2007 (cadam)
*/
/********************************************************************************F*/
static uint32_t _DirtyPngGetDistance(DirtyPngStateT *pState, DirtyPngHdrT *pPngHdr)
{
    uint16_t u, uDistance, uExtraBits, uParsedCode;

    u = pState->uDistMinBits;

    do
    {
        while ((pState->DistTables[u].uNoCodes == 1) && (u < 16))
        {
            u++;
        }
        if (u == 16)
        {
            return(0);
        }

        if (pState->uBitsLeft < u)
        {
            if (_DirtyPngGetNextBytes(pState, pPngHdr, (((u - pState->uBitsLeft) >> 3) + 1)) != DIRTYPNG_ERR_NONE)
            {
                return(0);
            }
        }

        uParsedCode = _DirtyPngReverseBits(pState->uDataBits, u);

        if ((uParsedCode >= pState->DistTables[u].uMinCode) && (uParsedCode <= pState->DistTables[u].uMaxCode))
        {
            _DirtyPngRemoveBits(pState, u);

            uParsedCode = pState->DistTables[u].Codes[uParsedCode - pState->DistTables[u].uMinCode];

            #if DIRTYPNG_DEBUG_LDCODES
            NetPrintf(("\ndistance: code=%u", uParsedCode));
            #endif

            if (uParsedCode > 29)
            {
                return(0);
            }

            uDistance = _HDIST_Codes[uParsedCode];
            uExtraBits = _HDIST_Bits[uParsedCode];

            #if DIRTYPNG_DEBUG_LDCODES
            NetPrintf((" base=%u bits=%u", uDistance, uExtraBits));
            #endif

            if (uExtraBits > 0)
            {
                if (pState->uBitsLeft < uExtraBits)
                {
                    if (_DirtyPngGetNextBytes(pState, pPngHdr, (((uExtraBits - pState->uBitsLeft) >> 3) + 1)) !=
                            DIRTYPNG_ERR_NONE)
                    {
                        return(0);
                    }
                }

                uDistance += (pState->uDataBits & ((1 << uExtraBits) - 1));
                _DirtyPngRemoveBits(pState, uExtraBits);
            }

            #if DIRTYPNG_DEBUG_LDCODES
            NetPrintf((" total=%u\n", uDistance));
            #endif

            return(uDistance);
        }

        u++;
    } while (u < 16);

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngCompBlock

    \Description
        Parse the compressed codes, then copy the data to the image buffer

    \Input *pState      - pointer to the module state
    \Input *pPngHdr     - pointer to header describing png to decode

    \Output
        int32_t         - nonnegative=success, negative=error

    \Version 05/04/2007 (cadam)
*/
/********************************************************************************F*/
static int32_t _DirtyPngCompBlock(DirtyPngStateT *pState, DirtyPngHdrT *pPngHdr)
{
    int32_t iRetVal = -1;
    uint16_t u, v, uExtraBits, uFoundCode = 0, uLength, uParsedCode;
    uint16_t uDistance, uDistStart, uDistEnd, uDistOffset;

    #if DIRTYPNG_DEBUG_INFLATE
    uint16_t uCountFound = 0;
    #endif

    pState->uBlockEnd = 0;

    #if DIRTYPNG_DEBUG_INFLATE
    NetPrintf(("dirtypng: byte offset=%u\n", pState->uByteOffset));
    #endif

    do
    {
        u = pState->uLitMinBits;

        uFoundCode = 0;

        do
        {
            while ((pState->LitTables[u].uNoCodes == 1) && (u < 16))
            {
                u++;
            }
            if (u == 16)
            {
                return(DIRTYPNG_ERR_BADCODE);
            }

            if (pState->uBitsLeft < u)
            {
                if ((iRetVal = _DirtyPngGetNextBytes(pState, pPngHdr, (((u - pState->uBitsLeft) >> 3) + 1))) !=
                        DIRTYPNG_ERR_NONE)
                {
                    return(iRetVal);
                }
            }

            uParsedCode = _DirtyPngReverseBits(pState->uDataBits, u);

            if ((uParsedCode >= pState->LitTables[u].uMinCode) && (uParsedCode <= pState->LitTables[u].uMaxCode))
            {
                _DirtyPngRemoveBits(pState, u);

                uFoundCode = 1;

                uParsedCode = pState->LitTables[u].Codes[uParsedCode - pState->LitTables[u].uMinCode];

                if (uParsedCode < 256)
                {
                    if (pState->uCurrentPass == 7)
                    {
                        return(DIRTYPNG_ERR_INVFILE);
                    }

                    #if DIRTYPNG_DEBUG_INFLATE
                    if (uCountFound % 20 == 0)
                    {
                        NetPrintf(("\ndirtypng: "));
                    }
                    NetPrintf(("%u ", uParsedCode));
                    uCountFound++;
                    #endif

                    pState->SlidingWindow[pState->uWindowOffset++] = (uint8_t)(uParsedCode & 0xff);

                    // wrap around the window if needed
                    if (pState->uWindowOffset == pState->uWindowSize)
                    {
                        pState->uWindowOffset = 0;
                    }

                    // increment the uWindowLength if we haven't already reached the max length
                    if (pState->uWindowLength < pState->uWindowSize)
                    {
                        pState->uWindowLength++;
                    }

                    // skip the filter byte
                    if (pState->uWidthOffset == 0)
                    {
                        pState->uFilterType = (uint8_t)(uParsedCode & 0xff);
                        // make sure the filter type is valid
                        if (pState->uFilterType > 4)
                        {
                            return(DIRTYPNG_ERR_BADTYPE);
                        }
                        pState->uWidthOffset++;
                    }
                    else
                    {
                        pState->pCurrScanline[pState->uWidthOffset++ - 1] = (uint8_t)(uParsedCode & 0xff);
                       
                        if (pState->uWidthOffset - 1 == (pState->uPixelWidth * pState->InterlacePassPixels[pState->uCurrentPass] * pPngHdr->iBitDepth + 8 - pPngHdr->iBitDepth) / 8)
                        {
                            pState->uWidthOffset = 0;
                            _DirtyPngFilterLine(pState, pPngHdr);

                            pState->InterlaceScanlines[pState->uCurrentPass]--;
                            pState->uCurrentScanline++;

                            if (pState->InterlaceScanlines[pState->uCurrentPass] == 0)
                            {
                                pState->uCurrentPass++;
                                pState->uCurrentScanline = 0;
                                ds_memclr(pState->pPrevScanline, pState->uScanlineWidth);
                            }
                            while ((pState->uCurrentPass < 7) && (pState->InterlaceScanlines[pState->uCurrentPass] == 0))
                            {
                                pState->uCurrentPass++;
                            }
                        }
                    }
                }
                else if ((uParsedCode > 256) && (uParsedCode < 286))
                {
                    if (pState->uCurrentPass == 7)
                    {
                        return(DIRTYPNG_ERR_INVFILE);
                    }

                    #if DIRTYPNG_DEBUG_LDCODES
                    NetPrintf(("\nlength: code=%u", uParsedCode));
                    #endif

                    uParsedCode -= 257;

                    uLength = _HLIT_Codes[uParsedCode];
                    uExtraBits = _HLIT_Bits[uParsedCode];

                    #if DIRTYPNG_DEBUG_LDCODES
                    NetPrintf((" base=%u bits=%u", uLength, uExtraBits));
                    #endif

                    if (uExtraBits > 0)
                    {
                        if (pState->uBitsLeft < uExtraBits)
                        {
                            if ((iRetVal = _DirtyPngGetNextBytes(pState, pPngHdr, 1)) != DIRTYPNG_ERR_NONE)
                            {
                                return(iRetVal);
                            }
                        }

                        uLength += (pState->uDataBits & ((1 << uExtraBits) - 1));
                        _DirtyPngRemoveBits(pState, uExtraBits);
                    }

                    #if DIRTYPNG_DEBUG_LDCODES
                    NetPrintf((" total=%u", uLength));
                    #endif

                    if ((uDistance = _DirtyPngGetDistance(pState, pPngHdr)) == 0)
                    {
                        return(DIRTYPNG_ERR_BADCODE);
                    }

                    if (uDistance > pState->uWindowLength)
                    {
                        return(DIRTYPNG_ERR_BADCODE);
                    }

                    uDistEnd = pState->uWindowOffset;
                    if (pState->uWindowOffset < uDistance)
                    {
                        uDistOffset = pState->uWindowSize + pState->uWindowOffset - uDistance;
                    }
                    else
                    {
                        uDistOffset = pState->uWindowOffset - uDistance;
                    }
                    uDistStart = uDistOffset;

                    for (v = 0; v < uLength; v++)
                    {
                        if (pState->uCurrentPass == 7)
                        {
                            return(DIRTYPNG_ERR_INVFILE);
                        }

                        #if DIRTYPNG_DEBUG_INFLATE
                        if (uCountFound % 20 == 0)
                        {
                            NetPrintf(("\ndirtypng: "));
                        }
                        NetPrintf(("%u ", pState->SlidingWindow[uDistOffset]));
                        uCountFound++;
                        #endif

                        pState->SlidingWindow[pState->uWindowOffset++] = pState->SlidingWindow[uDistOffset];

                        if (pState->uWindowOffset == pState->uWindowSize)
                        {
                            pState->uWindowOffset = 0;
                        }

                        // increment the uWindowLength if we haven't already reached the max length
                        if (pState->uWindowLength < pState->uWindowSize)
                        {
                            pState->uWindowLength++;
                        }

                        // skip the filter byte
                        if (pState->uWidthOffset == 0)
                        {
                            pState->uFilterType = pState->SlidingWindow[uDistOffset++];
                            // make sure the filter type is valid
                            if (pState->uFilterType > 4)
                            {
                                return(DIRTYPNG_ERR_BADTYPE);
                            }
                            pState->uWidthOffset++;
                        }
                        else
                        {
                            pState->pCurrScanline[pState->uWidthOffset++ - 1] = pState->SlidingWindow[uDistOffset++];
                       
                            if (pState->uWidthOffset == (pState->uPixelWidth * pState->InterlacePassPixels[pState->uCurrentPass] * pPngHdr->iBitDepth + 8 - pPngHdr->iBitDepth) / 8 + 1)
                            {
                                pState->uWidthOffset = 0;
                                _DirtyPngFilterLine(pState, pPngHdr);

                                pState->InterlaceScanlines[pState->uCurrentPass]--;
                                pState->uCurrentScanline++;

                                if (pState->InterlaceScanlines[pState->uCurrentPass] == 0)
                                {
                                    pState->uCurrentPass++;
                                    pState->uCurrentScanline = 0;
                                    ds_memclr(pState->pPrevScanline, pState->uScanlineWidth);
                                }
                                while ((pState->uCurrentPass < 7) && (pState->InterlaceScanlines[pState->uCurrentPass] == 0))
                                {
                                    pState->uCurrentPass++;
                                }
                            }
                        }
                        
                        if (uDistOffset == pState->uWindowSize)
                        {
                            uDistOffset = 0;
                        }
                        if (uDistOffset == uDistEnd)
                        {
                            uDistOffset = uDistStart;
                        }
                    }
                }
                else if (uParsedCode >= 286)
                {
                    return(DIRTYPNG_ERR_BADCODE);
                }
            }

            u++;
        } while ((uFoundCode == 0) && (u < 16));

        if (uFoundCode == 0)
        {
            return(DIRTYPNG_ERR_BADCODE);
        }
    } while (uParsedCode != 256);

    pState->uBlockEnd = 1;

    #if DIRTYPNG_DEBUG_INFLATE
    NetPrintf(("dirtypng: byte offset=%u\n", pState->uByteOffset));
    #endif

    return(DIRTYPNG_ERR_NONE);
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngConvertImage

    \Description
        Convert the image to the expected ordering of DirtyGraph.

    \Input *pState      - pointer to the module state
    \Input *pPngHdr     - pointer to header describing png to decode

    \Version 02/13/2007 (cadam)
*/
/********************************************************************************F*/
static void _DirtyPngConvertImage(DirtyPngStateT *pState, DirtyPngHdrT *pPngHdr)
{
    uint8_t u = 0, uTemp = 0;
    uint32_t uInOffset, uOutOffset, uPixelWidth = pState->uPixelWidth;

    switch(pPngHdr->iColourType)
    {
        case 0:
            uInOffset = pState->uImageOffset - 1;
            uPixelWidth += 3;
            uOutOffset = pState->uBufHeight * uPixelWidth * pState->uBufWidth - 1;

            while (uInOffset > 0)
            {
                pState->pImageBuf[uOutOffset-2] = pState->pImageBuf[uOutOffset-1] = pState->pImageBuf[uOutOffset] = pState->pImageBuf[uInOffset--];
                pState->pImageBuf[uOutOffset-3] = 0xff;

                uOutOffset -= 4;
            }

            break;
        case 2:
            uInOffset = pState->uImageOffset - 1;
            uPixelWidth++;
            uOutOffset = pState->uBufHeight * uPixelWidth * pState->uBufWidth - 1;

            while (uInOffset > 0)
            {
                if (uOutOffset % uPixelWidth == 0)
                {
                    pState->pImageBuf[uOutOffset] = 0xff;
                }
                else
                {
                    pState->pImageBuf[uOutOffset] = pState->pImageBuf[uInOffset--];
                }

                uOutOffset--;
            }

            pState->pImageBuf[0] = 0xff;
            break;
        case 4:
            uInOffset = pState->uImageOffset - 1;
            uPixelWidth += 2;
            uOutOffset = pState->uBufHeight * uPixelWidth * pState->uBufWidth - 1;

            while (uInOffset > 0)
            {
                uTemp = pState->pImageBuf[uInOffset--];
                pState->pImageBuf[uOutOffset-2] = pState->pImageBuf[uOutOffset-1] = pState->pImageBuf[uOutOffset] = pState->pImageBuf[uInOffset];
                pState->pImageBuf[uOutOffset-3] = uTemp;

                if (uInOffset == 0)
                {
                    break;
                }

                uInOffset--;
                uOutOffset -= 4;
            }

            break;
        case 6:
            uOutOffset = pState->uBufHeight * uPixelWidth  * pState->uBufWidth - 1;

            while (uOutOffset > 0)
            {
                uTemp = pState->pImageBuf[uOutOffset];
                for (u = 0; u < uPixelWidth - 1; u++)
                {
                    pState->pImageBuf[uOutOffset] = pState->pImageBuf[uOutOffset - 1];
                    uOutOffset--;
                }
                pState->pImageBuf[uOutOffset] = uTemp;

                if (uOutOffset == 0)
                {
                    break;
                }
                uOutOffset--;
            }
            break;
    }
}

/*F********************************************************************************/
/*!
    \Function _DirtyPngFreeScanlines

    \Description
        Free the scanline data

    \Input *pState  - pointer to module state

    \Version 05/22/2007 (cadam)
*/
/********************************************************************************F*/
static void _DirtyPngFreeScanlines(DirtyPngStateT *pState)
{
    if (pState->pPrevScanline != NULL)
    {
        DirtyMemFree(pState->pPrevScanline, DIRTYPNG_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        pState->pPrevScanline = NULL;
    }
    if (pState->pCurrScanline != NULL)
    {
        DirtyMemFree(pState->pCurrScanline, DIRTYPNG_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        pState->pCurrScanline = NULL;
    }
}

/*** Public Functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function DirtyPngCreate

    \Description
        Create the DirtyPng module state

    \Output
        DirtyPngStateT *   - pointer to new ref, or NULL if error

    \Version 02/07/2007 (cadam)
*/
/********************************************************************************F*/
DirtyPngStateT *DirtyPngCreate(void)
{
    DirtyPngStateT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pState = DirtyMemAlloc(sizeof(*pState), DIRTYPNG_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtypng: error allocating module state\n"));
        return(NULL);
    }
    ds_memclr(pState, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;

    // return the state pointer
    return(pState);
}

/*F********************************************************************************/
/*!
    \Function DirtyPngDestroy

    \Description
        Destroy the DirtyPng module

    \Input *pState  - pointer to module state

    \Version 05/01/2007 (cadam)
*/
/********************************************************************************F*/
void DirtyPngDestroy(DirtyPngStateT *pState)
{
    DirtyMemFree(pState, DIRTYPNG_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function DirtyPngIdentify

    \Description
        Identify if input image is a PNG image.
        
    \Input *pImageData  - pointer to image data
    \Input uImageLen    - size of image data

    \Output
        int32_t         - TRUE if a PNG, else FALSE

    \Version 02/05/2007 (cadam)
*/
/********************************************************************************F*/
int32_t DirtyPngIdentify(const uint8_t *pImageData, uint32_t uImageLen)
{
    // make sure we have enough data
    if (uImageLen < SIGNATURE_LEN)
    {
        return(FALSE);
    }
    // see of we're a PNG
    // the first eight bytes of a PNG file contain the decimal values: 137 80 78 71 13 10 26 10
    if ((pImageData[0] != 0x89) || (pImageData[1] != 0x50) || (pImageData[2] != 0x4E) || (pImageData[3] != 0x47) ||
        (pImageData[4] != 0x0D) || (pImageData[5] != 0x0A) || (pImageData[6] != 0x1A) || (pImageData[7] != 0x0A))
    {
        return(FALSE);
    }

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function DirtyPngParse

    \Description
        Parse PNG header.

    \Input *pState      - pointer to the module state
    \Input *pPngHdr     - [out] pointer to PNG header to fill in
    \Input *pPngData    - pointer to PNG data
    \Input *pPngEnd     - pointer past the end of PNG data

    \Version 02/06/2007 (cadam)
*/
/********************************************************************************F*/
int32_t DirtyPngParse(DirtyPngStateT *pState, DirtyPngHdrT *pPngHdr, const uint8_t *pPngData, const uint8_t *pPngEnd)
{
    int32_t iRetVal;

    // reset internal state
    _DirtyPngReset(pState);

    iRetVal = _DirtyPngParseHeader(pState, pPngHdr, pPngData, pPngEnd); 

    if (iRetVal == 0)
    {
        iRetVal = _DirtyPngParseChunks(pState, pPngHdr, pPngData, pPngEnd);
    }

    return(iRetVal);
}


/*F********************************************************************************/
/*!
    \Function DirtyPngDecodeImage

    \Description
        Decode a PNG image into an 8bit paletteized bitmap.
  
    \Input *pState      - pointer to the module state
    \Input *pPngHdr     - pointer to header describing png to decode
    \Input *pImageData  - [out] pointer to buffer to write decoded image data to
    \Input iBufWidth    - width of output buffer
    \Input iBufHeight   - height of output buffer

    \Output
        int32_t         - positive=number of bytes decoded, negative=error

    \Version 05/01/2007 (cadam)
*/
/********************************************************************************F*/
int32_t DirtyPngDecodeImage(DirtyPngStateT *pState, DirtyPngHdrT *pPngHdr, uint8_t *pImageData, int32_t iBufWidth, int32_t iBufHeight)
{
    int32_t iLastBlock, iResult;

    pState->pImageBuf = pImageData;
    pState->uBufWidth = (uint32_t)iBufWidth;
    pState->uBufHeight = (uint32_t)iBufHeight;

    if ((iResult = _DirtyPngParseZlibInfo(pState, pPngHdr)) != DIRTYPNG_ERR_NONE)
    {
        return(iResult);
    }

    // allocate memory for the previous and current scanlines
    pState->uScanlineWidth = (pState->uPixelWidth * pPngHdr->uWidth * pPngHdr->iBitDepth + 8 - pPngHdr->iBitDepth) / 8;
    if ((pState->pScanlines[0] = DirtyMemAlloc(pState->uScanlineWidth, DIRTYPNG_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtypng: error allocating previous scanline\n"));
        return(DIRTYPNG_ERR_ALLOCFAIL);
    }
    ds_memclr(pState->pScanlines[0], pState->uScanlineWidth);
    pState->pPrevScanline = pState->pScanlines[0];
    if ((pState->pScanlines[1] = DirtyMemAlloc(pState->uScanlineWidth, DIRTYPNG_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtypng: error allocating current scanline\n"));
        DirtyMemFree(pState->pPrevScanline, DIRTYPNG_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        pState->pPrevScanline = NULL;
        return(DIRTYPNG_ERR_ALLOCFAIL);
    }
    ds_memclr(pState->pScanlines[1], pState->uScanlineWidth);
    pState->pCurrScanline = pState->pScanlines[1];

    do
    {
        iLastBlock = _DirtyPngParseDeflateInfo(pState, pPngHdr);

        if (iLastBlock < 0)
        {
            _DirtyPngFreeScanlines(pState);
            return(iLastBlock);
        }

        switch(pState->uBType)
        {
            case 0:
                if ((iResult = _DirtyPngNoCompBlock(pState, pPngHdr)) != DIRTYPNG_ERR_NONE)
                {
                    _DirtyPngFreeScanlines(pState);
                    return(iResult);
                }
                break;
            case 1:
                if ((iResult = _DirtyPngFixedBlock(pState)) != DIRTYPNG_ERR_NONE)
                {
                    _DirtyPngFreeScanlines(pState);
                    return(iResult);
                }
                if ((iResult = _DirtyPngCompBlock(pState, pPngHdr)) != DIRTYPNG_ERR_NONE)
                {
                    _DirtyPngFreeScanlines(pState);
                    return(iResult);
                }
                break;
            case 2:
                if ((iResult = _DirtyPngDynamicBlock(pState, pPngHdr)) != DIRTYPNG_ERR_NONE)
                {
                    _DirtyPngFreeScanlines(pState);
                    return(iResult);
                }
                if ((iResult = _DirtyPngCompBlock(pState, pPngHdr)) != DIRTYPNG_ERR_NONE)
                {
                    _DirtyPngFreeScanlines(pState);
                    return(iResult);
                }
                break;
        }
    } while (iLastBlock != TRUE);
    
    _DirtyPngConvertImage(pState, pPngHdr);

    // _DirtyPngCheckAlder() - Needs to be coded;

    // free the previous and current scanlines
    _DirtyPngFreeScanlines(pState);

    return(DIRTYPNG_ERR_NONE);
}
