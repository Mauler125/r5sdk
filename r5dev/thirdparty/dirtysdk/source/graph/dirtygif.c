/*H********************************************************************************/
/*!
    \File dirtygif.c

    \Description
        Routines to decode a GIF into a raw image and palette.  These routines
        were written from scratch based on the published specifications and visual
        inspection of some public-domain decoders.

    \Notes
        References
            GIF specification: https://www.w3.org/Graphics/GIF/spec-gif89a.txt

    \Copyright
        Copyright (c) 2003-2020 Electronic Arts Inc.

    \Version 11/13/2003 (jbrookes) First Version
    \Version 01/28/2020 (jbrookes) Added multiframe (animated) support
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/graph/dirtygif.h"

/*** Defines **********************************************************************/

#define DIRTYGIF_VERBOSE    (DIRTYCODE_LOGGING && FALSE)    //!< enable verbose logging

#define DIRTYGIF_MAXBITS    (12)                            //!< max LZW code size
#define DIRTYGIF_MAXCODE    ((1 << DIRTYGIF_MAXBITS) - 1)   //!< max code value

/*** Macros ***********************************************************************/

//! macro to validate that enough space exists in the file for a particular header
#define DIRTYGIF_Validate(_pData, _pEnd, _size, _retcode)   \
{                                                           \
    if (((_pEnd) - (_pData)) < (_size))                     \
    {                                                       \
        return(_retcode);                                   \
    }                                                       \
}

#define DIRTYGIF_ClrCode(_iSize)  (1 << (iSize) + 0)
#define DIRTYGIF_EndCode(_iSize)  (1 << (iSize) + 1)
#define DIRTYGIF_NewCode(_iSize)  (1 << (iSize) + 2)

/*** Type Definitions *************************************************************/

//! decoder state
typedef struct DirtyGifDecoderT
{
    int32_t iCurCodeSize;                       //!< current code size, in bits
    int32_t iClearCode;                         //!< value of a clear code
    int32_t iEndingCode;                        //!< value of an ending code
    int32_t iNewCodes;                          //!< first available code
    int32_t iTopSlot;                           //!< highest code for current code size
    int32_t iSlot;                              //!< last read code

    int32_t iBytesLeft;                         //!< number of bytes left in block
    int32_t iBitsLeft;                          //!< number of bits left in block
    uint8_t uCurByte;                           //!< current byte
    const uint8_t *pByteStream;                 //!< pointer to byte stream

    uint8_t uSuffixTable[DIRTYGIF_MAXCODE+1];   //!< suffix table
    uint32_t uPrefixTable[DIRTYGIF_MAXCODE+1];  //!< prefix table
    
    uint8_t uStack[DIRTYGIF_MAXCODE+1];         //!< code stack
    uint8_t *pStackPtr;                         //!< current stack pointer
    uint8_t *pStackEnd;                         //!< end of stack

    int32_t iCode;                              //!< most recent table code
    int32_t iCodeBits;                          //!< number of bits used for code
    int32_t iCode1;
    int32_t iCode0;                                      
    int32_t iCodeRaw;                           //!< raw code read from data

    const uint8_t *pCodeData;                   //!< current image data pointer
    const uint8_t *pEndCodeData;                //!< end of image data
    int32_t uHeight;                            //!< image height
    int32_t uWidth;                             //!< image width
    uint8_t *pScanLine;                         //!< start of current scan line                 
    uint8_t *pScanLineEnd;                      //!< end of current scan line
    uint8_t *pBufferEnd;                        //!< end of current buffer line
    int32_t iPass;                              //!< pass for interlaced images
    int32_t iIndex;                             //!< which row within pass
    int32_t iStep;                              //!< interlace step
    int32_t iBufHeight;                         //!< output buffer height
    uint8_t *pImageData;                        //!< output image buffer (byte array)

} DirtyGifDecoderT;

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

static const int32_t stepsize[] = { 8, 8, 4, 2, 0, 1, 0 };
static const int32_t startrow[] = { 0, 4, 2, 1, 0, 0, 0 };


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _DirtyGifParseColorTable

    \Description
        Parse the color table in GIF header.

    \Input *ppColorTable- [out] storage for color table pointer
    \Input *pNumColors  - [out] storage for color table size
    \Input *pGifData    - pointer to current location parsing GIF data
    \Input *pGifEnd     - pointer past end of GIF data
    \Input uBits        - byte containing color table info

    \Output
        const uint8_t * - pointer to gif data after color table (if present)

    \Version 11/13/2003 (jbrookes)
*/
/********************************************************************************F*/
static const uint8_t *_DirtyGifParseColorTable(const uint8_t **ppColorTable, uint16_t *pNumColors, const uint8_t *pGifData, const uint8_t *pGifEnd, uint32_t uBits)
{
    if (uBits & 0x80)
    {
        // get number of colors
        *pNumColors = 2 << (uBits & 0x7);

        // validate color table
        if ((pGifEnd-pGifData) < (signed)(*pNumColors*3))
        {
            return(NULL);
        }

        // ref color table
        NetPrintfVerbose((DIRTYGIF_VERBOSE, 0, "dirtygif: pColorTable=%p\n", pGifData));
        *ppColorTable = pGifData;
        pGifData += *pNumColors*3;
    }
    else
    {
        *ppColorTable = NULL;
        *pNumColors = 0;
    }

    return(pGifData);
}

/*F********************************************************************************/
/*!
    \Function _DirtyGifParseHeader

    \Description
        Parse the GIF header.

    \Input *pGifHdr     - [out] pointer to GIF header to fill in
    \Input *pGifData    - pointer to GIF data
    \Input *pGifEnd     - pointer past the end of GIF data
    \Input *pFrames     - [out] storage for list of frames or NULL if none
    \Input uNumFrames   - size of output frame array or zero if no frames output

    \Output
        int32_t         - nonnegative=success, negative=error

    \Version 01/22/2020 (jbrookes) Rewrote for more advanced parsing
*/
/********************************************************************************F*/
static int32_t _DirtyGifParseHeader(DirtyGifHdrT *pGifHdr, const uint8_t *pGifData, const uint8_t *pGifEnd, DirtyGifHdrT *pFrames, uint32_t uNumFrames)
{
    uint8_t uSize, uKind;
    uint16_t uDelay;
    uint8_t uTransColor=0, bHasAlpha=FALSE;

    // iClearCode header
    ds_memclr(pGifHdr, sizeof(*pGifHdr));

    // validate and skip signature
    DIRTYGIF_Validate(pGifData, pGifEnd, 6, -1);
    if (memcmp(pGifData, "GIF87a", 6) && memcmp(pGifData, "GIF89a", 6))
    {
        return(-2);
    }
    pGifData += 6;

    // get logical screen width and height
    pGifHdr->uWidth = pGifData[0] | pGifData[1] << 8;
    pGifHdr->uHeight = pGifData[2] | pGifData[3] << 8;
    NetPrintfVerbose((DIRTYGIF_VERBOSE, 0, "dirtygif: logical screen w=%d h=%d\n", pGifHdr->uWidth, pGifHdr->uHeight));

    // validate logical screen descriptor
    DIRTYGIF_Validate(pGifData, pGifEnd, 7, -3);
    
    // parse global color table info from logical screen descriptor
    pGifData = _DirtyGifParseColorTable(&pGifHdr->pColorTable, &pGifHdr->uNumColors, pGifData+7, pGifEnd, pGifData[4]);

    // process blocks
    for (uSize = 0, uDelay = 0; ((pGifEnd - pGifData) > 2); )
    {
        uKind = pGifData[0];

        // parse extension
        if (uKind == 0x21)
        {
            uKind = pGifData[1];
            uSize = pGifData[2];
            pGifData += 3;

            NetPrintfVerbose((DIRTYGIF_VERBOSE, 0, "dirtygif: parsing extension block type 0x%02x (size=%d)\n", uKind, uSize));

            // validate extension
            DIRTYGIF_Validate(pGifData, pGifEnd, uSize + 1, -5);

            // parse graphic control extension block
            if (uKind == 0xf9)
            {
                // get alpha
                bHasAlpha = pGifData[0] & 0x1;
                uTransColor = pGifData[3];
                // get delay time and convert from hundreths of a second to milliseconds
                uDelay = (pGifData[1]|(pGifData[2]<<8))*10;
                NetPrintfVerbose((DIRTYGIF_VERBOSE, 0, "dirtygif: delay=%d alpha=%d color=%d\n", uDelay, bHasAlpha, uTransColor));
                // skip size plus trailer
                pGifData += uSize + 1;
            }
            // skip application extension block
            else if (uKind == 0xff)
            {
                for (; uSize != 0; uSize = *pGifData++)
                {
                    pGifData += uSize;
                }
            }
            // unhandled extension... just skip it
            else
            {
                pGifData += uSize + 1;
            }
        }
        // parse image block
        else if (uKind == 0x2c)
        {
            DirtyGifHdrT FrameInfo;
            uint32_t uBlockCount;
            uint8_t uBlockSize;

            DIRTYGIF_Validate(pGifData, pGifEnd, 10, -7);

            // get width and height from image descriptor
            ds_memclr(&FrameInfo, sizeof(FrameInfo));
            FrameInfo.uLeft = pGifData[1] | pGifData[2] << 8;
            FrameInfo.uTop = pGifData[3] | pGifData[4] << 8;
            FrameInfo.uWidth = pGifData[5] | pGifData[6] << 8;
            FrameInfo.uHeight = pGifData[7] | pGifData[8] << 8;
            FrameInfo.uDelay = uDelay;
            FrameInfo.bHasAlpha = bHasAlpha;
            FrameInfo.uTransColor = uTransColor;

            // determine if it is an interlaced image
            FrameInfo.bInterlaced = (pGifData[9] & 0x40) != 0;

            // parse local color table info from image descriptor
            pGifData = _DirtyGifParseColorTable(&FrameInfo.pColorTable, &FrameInfo.uNumColors, &pGifData[10], pGifEnd, pGifData[9]);

            // save image start
            FrameInfo.pImageData = pGifData++;

            // parse through image blocks to find end
            for (uBlockSize = 0xff, uBlockCount = 0; (pGifData < pGifEnd) && (uBlockSize != 0); pGifData += uBlockSize)
            {
                uBlockSize = *pGifData++;
            }
            // save image end
            FrameInfo.pImageEnd = pGifData;

            // if first image, copy frame info to main gif header info
            if (pGifHdr->uNumFrames == 0)
            {
                pGifHdr->pImageData = FrameInfo.pImageData;
                pGifHdr->pImageEnd = FrameInfo.pImageEnd;
                pGifHdr->uWidth = FrameInfo.uWidth;
                pGifHdr->uHeight = FrameInfo.uHeight;
                pGifHdr->bInterlaced = FrameInfo.bInterlaced;
                pGifHdr->uTransColor = FrameInfo.uTransColor;
                pGifHdr->bHasAlpha = FrameInfo.bHasAlpha;
            }

            // if we have a frames list, write to that
            if ((pFrames != NULL) && (pGifHdr->uNumFrames < uNumFrames))
            {
                ds_memcpy_s(&pFrames[pGifHdr->uNumFrames], sizeof(*pFrames), &FrameInfo, sizeof(FrameInfo));
            }

            // log frame
            NetPrintfVerbose((DIRTYGIF_VERBOSE, 0, "dirtygif: frame %02d] t=%d l=%d w=%d h=%d i=%d bc=%d\n", pGifHdr->uNumFrames, FrameInfo.uTop, FrameInfo.uLeft, FrameInfo.uWidth, FrameInfo.uHeight, FrameInfo.bInterlaced, uBlockCount));

            // track frame count
            pGifHdr->uNumFrames += 1;
        }
        else
        {
            // invalid block kind
            NetPrintf(("dirtygif: invalid block kind 0x%02x\n", uKind));
            return(-6);
        }
    }

    NetPrintfVerbose((DIRTYGIF_VERBOSE, 0, "dirtygif: nFrames=%d\n", pGifHdr->uNumFrames));

    // return success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _DirtyGifResetDecoder

    \Description
        Reset the LZW decoder.

    \Input *pDecoder    - pointer to decoder state

    \Version 11/20/2003 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyGifResetDecoder(DirtyGifDecoderT *pDecoder)
{
    pDecoder->iCurCodeSize = pDecoder->iCodeBits + 1;
    pDecoder->iSlot = pDecoder->iNewCodes;
    pDecoder->iTopSlot = 1 << pDecoder->iCurCodeSize;
    pDecoder->iCode = 0;
}

/*F********************************************************************************/
/*!
    \Function _DirtyGifInitDecoder

    \Description
        Init the LZW decoder.

    \Input *pDecoder    - pointer to decoder state
    \Input *pGifHdr     - pointer to GIF header
    \Input *pImageData  - pointer to output pixel buffer
    \Input iStep        - output step size
    \Input iHeight      - output buffer height

    \Version 11/20/2003 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _DirtyGifInitDecoder(DirtyGifDecoderT *pDecoder, DirtyGifHdrT *pGifHdr, uint8_t *pImageData, int32_t iStep, int32_t iHeight)
{
    ds_memclr(pDecoder, sizeof(*pDecoder));
    pDecoder->pCodeData = pGifHdr->pImageData;
    pDecoder->pEndCodeData = pGifHdr->pImageEnd;
    pDecoder->uHeight = pGifHdr->uHeight;
    pDecoder->uWidth = pGifHdr->uWidth;
    pDecoder->iStep = iStep;
    pDecoder->iBufHeight = iHeight;
    pDecoder->pImageData = pImageData;

    // set starting imaging location
    pDecoder->iPass = (pGifHdr->bInterlaced ? 0 : 5);
    pDecoder->iIndex = startrow[pDecoder->iPass];
    pDecoder->pScanLine = NULL;

    // get the initial code length
    if (pDecoder->pCodeData == pDecoder->pEndCodeData)
    {
       return(-2);
    }
    pDecoder->iCodeBits = *pDecoder->pCodeData++;
    if ((pDecoder->iCodeBits < 2) || (pDecoder->iCodeBits > 9))
    {
        return(-2);
    }

    // setup for decoding
    pDecoder->iBytesLeft = pDecoder->iBitsLeft = 0;
    pDecoder->iClearCode = 1 << pDecoder->iCodeBits;
    pDecoder->iEndingCode = pDecoder->iClearCode + 1;
    pDecoder->iNewCodes = pDecoder->iEndingCode + 1;
    _DirtyGifResetDecoder(pDecoder);

    // protect against missing iClearCode code
    pDecoder->iCode0 = pDecoder->iCode1 = 0;

    // init stack
    pDecoder->pStackPtr = pDecoder->uStack;
    pDecoder->pStackEnd = pDecoder->uStack+sizeof(pDecoder->uStack);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _DirtyGifGetByte

    \Description
        Get next byte from current block.

        If the current block is exhausted, this function advances to the next block, and
        returns the first byte from that block.

    \Input *pDecoder    - pointer to decoder state

    \Output
        int32_t         - zero

    \Version 11/20/2003 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _DirtyGifGetByte(DirtyGifDecoderT *pDecoder)
{
    // have we consumed all bytes in this block?
    if (pDecoder->iBytesLeft <= 0)
    {
        // get byte length of next block
        pDecoder->iBytesLeft = *pDecoder->pCodeData++;
        
        // validate block
        DIRTYGIF_Validate(pDecoder->pCodeData, pDecoder->pEndCodeData, pDecoder->iBytesLeft, -1);
        
        // point to bytestream for this block
        pDecoder->pByteStream = pDecoder->pCodeData;
        
        // skip decode pointer past block
        pDecoder->pCodeData += pDecoder->iBytesLeft;
    }

    // get next byte from block
    pDecoder->uCurByte = *pDecoder->pByteStream++;
    pDecoder->iBytesLeft--;
    
    // return success to caller
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _DirtyGifGetNextCode

    \Description
        Get next code from the current block.

    \Input *pDecoder    - pointer to decoder state

    \Output
        int32_t         - TRUE if we should continue decoding, else FALSE if we've
                          reached the end code.

    \Version 11/20/2003 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _DirtyGifGetNextCode(DirtyGifDecoderT *pDecoder)
{
    // out of bits?
    if (pDecoder->iBitsLeft == 0)
    {
        if (_DirtyGifGetByte(pDecoder) < 0)
        {
            return(-1);
        }
        
        pDecoder->iBitsLeft += 8;
    }

    // add from bit bucket
    pDecoder->iCodeRaw = pDecoder->uCurByte >> (8 - pDecoder->iBitsLeft);

    // fill out the code
    while (pDecoder->iCurCodeSize > pDecoder->iBitsLeft)
    {
        if (_DirtyGifGetByte(pDecoder) < 0)
        {
            return(-1);
        }
        
        pDecoder->iCodeRaw |= pDecoder->uCurByte << pDecoder->iBitsLeft;
        pDecoder->iBitsLeft += 8;
    }
    
    // update bits left, and mask to code range
    pDecoder->iBitsLeft -= pDecoder->iCurCodeSize;
    pDecoder->iCodeRaw &= (1 << pDecoder->iCurCodeSize) - 1;

    // check for end of data
    return((pDecoder->iCodeRaw == pDecoder->iEndingCode) ? 0 : 1);
}

/*F********************************************************************************/
/*!
    \Function _DirtyGifUpdateDecoder

    \Description
        Update decoder state.

    \Input *pDecoder    - pointer to decoder state

    \Version 11/20/2003 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyGifUpdateDecoder(DirtyGifDecoderT *pDecoder)
{
    // set up for decode
    pDecoder->iCode = pDecoder->iCodeRaw;

    // if we get a bogus code use the last valid code read instead
    if (pDecoder->iCode >= pDecoder->iSlot)
    {
        pDecoder->iCode = pDecoder->iCode0;
        if (pDecoder->pStackPtr < pDecoder->pStackEnd)
        {
            *pDecoder->pStackPtr++ = (uint8_t)pDecoder->iCode1;
        }
    }

    // push characters onto stack
    while (pDecoder->iCode >= pDecoder->iNewCodes)
    {
        if (pDecoder->pStackPtr >= pDecoder->pStackEnd)
        {
            // overflow error
            break;
        }
        *pDecoder->pStackPtr++ = pDecoder->uSuffixTable[pDecoder->iCode];
        pDecoder->iCode = pDecoder->uPrefixTable[pDecoder->iCode];
    }

    // push last char onto the uStack
    if (pDecoder->pStackPtr < pDecoder->pStackEnd)
    {
        *pDecoder->pStackPtr++ = (uint8_t)pDecoder->iCode;
    }

    // set up new prefix and uSuffixTable
    if (pDecoder->iSlot < pDecoder->iTopSlot)
    {
        pDecoder->iCode1 = pDecoder->iCode;
        pDecoder->uSuffixTable[pDecoder->iSlot] = (uint8_t)pDecoder->iCode1;
        pDecoder->uPrefixTable[pDecoder->iSlot++] = pDecoder->iCode0;
        pDecoder->iCode0 = pDecoder->iCodeRaw;
    }

    // if required iSlot number is greater than bit size allows, increase bit size (up to 12 bits)
    if ((pDecoder->iSlot >= pDecoder->iTopSlot)  && (pDecoder->iCurCodeSize < 12))
    {
        pDecoder->iTopSlot <<= 1;
        pDecoder->iCurCodeSize++;
    } 
}

/*F********************************************************************************/
/*!
    \Function _DirtyGifUpdateBitmap

    \Description
        Write decoded string to output bitmap buffer.

    \Input *pDecoder    - pointer to decoder state
    \Input bVflip       - if TRUE, flip image vertically

    \Output
        int32_t         - one to continue, zero to abort

    \Version 11/20/2003 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _DirtyGifUpdateBitmap(DirtyGifDecoderT *pDecoder, uint32_t bVflip)
{
    uint8_t cPixel;

    // pop decoded string off stack into output buffer
    for ( ; pDecoder->pStackPtr > pDecoder->uStack; )
    {
        // see if we need to calc scanline start
        if (pDecoder->pScanLine == NULL)
        {
            // if we've hit the output buffer height, quit early
            if (pDecoder->iIndex == pDecoder->iBufHeight)
            {
                return(0);
            }
            
            // calc the new line start
            if (bVflip)
            {
                pDecoder->pScanLine = pDecoder->pImageData + ((pDecoder->uHeight-1) * pDecoder->iStep);
                if (pDecoder->iIndex < pDecoder->uHeight)
                {
                    pDecoder->pScanLine -= pDecoder->iStep*((pDecoder->uHeight-1)-pDecoder->iIndex);
                }
            }
            else
            {
                pDecoder->pScanLine = pDecoder->pImageData;
                if (pDecoder->iIndex < pDecoder->uHeight)
                {
                    pDecoder->pScanLine += pDecoder->iStep*((pDecoder->uHeight-1)-pDecoder->iIndex);
                }
            }
            
            pDecoder->pScanLineEnd = pDecoder->pScanLine+pDecoder->uWidth;
            pDecoder->pBufferEnd = pDecoder->pScanLine+pDecoder->iStep;
        }

        // read pixel from stack
        cPixel = *(--pDecoder->pStackPtr);
        
        // put translated pixel into output buffer
        if (pDecoder->pScanLine < pDecoder->pBufferEnd)
        {
            *pDecoder->pScanLine = cPixel;
        }
        pDecoder->pScanLine += 1;

        // see if we are at end of scanline
        if (pDecoder->pScanLine == pDecoder->pScanLineEnd)
        {
            // if width and step size differ, zero fill extra
            if (pDecoder->uWidth < pDecoder->iStep)
            {
                *pDecoder->pScanLine++ = 0x00;
            }
            
            // see if we are done with this pass
            if (((pDecoder->iIndex += stepsize[pDecoder->iPass]) >= pDecoder->uHeight) && (stepsize[pDecoder->iPass] > 0))
            {
                pDecoder->iIndex = startrow[++pDecoder->iPass];
            }
            
            // invalidate the scanline pointer
            pDecoder->pScanLine = NULL;
        }
    }
    
    // normal return condition
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _DirtyGifDecodeImage32

    \Description
        Decode a GIF image into a 32bit ARGB direct-color bitmap

    \Input *pGifHdr     - pointer to header describing gif to decode
    \Input *pImageData  - [out] pointer to buffer to write decoded image data to
    \Input *pImageDataPrev  - pointer to buffer with previous image, or NULL if first image
    \Input *p8BitImage  - pointer to scratch buffer for 8bit image decoding, or NULL
    \Input iBufWidth    - width of output buffer in pixels
    \Input iBufHeight   - height of output buffer in pixels
    \Input bVflip       - if TRUE, flip image vertically

    \Output
        int32_t         - positive=number of bytes decoded, negative=error

    \Version 01/28/2020 (jbrookes) Rewrote to handle animated gif frames
*/
/********************************************************************************F*/
static int32_t _DirtyGifDecodeImage32(DirtyGifHdrT *pGifHdr, uint8_t *pImageData, const uint8_t *pImageDataPrev, uint8_t *p8BitImage, int32_t iBufWidth, int32_t iBufHeight, uint32_t bVflip)
{
    uint8_t aPaletteData[256][4];
    uint8_t *pSrc, *pDst;
    const uint8_t *pSrc32;
    int32_t i8BitSize, iWidth, iHeight;
    uint8_t bAlpha, bInFrame;
    int32_t iError;

    // first, decode palette info
    if ((iError = DirtyGifDecodePalette(pGifHdr, (uint8_t *)aPaletteData, (uint8_t *)aPaletteData + sizeof(aPaletteData), 0xff)) < 0)
    {
        return(iError);
    }

    // if we didn't get an 8bit decode buffer, put it at the end of the output buffer
    if (p8BitImage == NULL)
    {
        // calculate size of decoded 8bit image
        i8BitSize = pGifHdr->uWidth * pGifHdr->uHeight;
        // locate 8bit image at end of buffer
        p8BitImage = pImageData + (iBufWidth * iBufHeight * 4) - i8BitSize;
    }

    // decode the image
    if ((iError = DirtyGifDecodeImage(pGifHdr, p8BitImage, pGifHdr->uWidth, pGifHdr->uHeight, bVflip)) < 0)
    {
        return(iError);
    }

    // now translate the 8bit image to 32bits
    for (pSrc = p8BitImage, iHeight = 0; iHeight < iBufHeight; iHeight += 1)
    {
        for (iWidth = 0; iWidth < iBufWidth; iWidth += 1)
        {
            // get palette index and read if we have an alpha
            bAlpha = pGifHdr->bHasAlpha && (*pSrc == pGifHdr->uTransColor);
            // see if our pixel is in frame or not
            bInFrame = ((iWidth >= pGifHdr->uLeft) && (iWidth < (pGifHdr->uLeft + pGifHdr->uWidth)) &&
                (iHeight >= pGifHdr->uTop) && (iHeight < (pGifHdr->uTop + pGifHdr->uHeight))) ||
                (pImageDataPrev == NULL);
            // locate output
            pDst = pImageData + ((iHeight * iBufWidth) + iWidth) * 4;
            // write output
            if (bInFrame && !bAlpha)
            {
                pDst[0] = aPaletteData[*pSrc][3];
                pDst[1] = aPaletteData[*pSrc][0];
                pDst[2] = aPaletteData[*pSrc][1];
                pDst[3] = aPaletteData[*pSrc][2];
            }
            else if (pImageData != pImageDataPrev)
            {
                pSrc32 = pImageDataPrev + ((iHeight * iBufWidth) + iWidth) * 4;
                pDst[0] = pSrc32[0];
                pDst[1] = pSrc32[1];
                pDst[2] = pSrc32[2];
                pDst[3] = pSrc32[3];
            }

            // advance source pointer if we're in frame
            if (bInFrame)
            {
                pSrc += 1;
            }
        }
    }

    // return success
    return(0);
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function DirtyGifIdentify

    \Description
        Identify if input image is a GIF image.

    \Input *pImageData  - pointer to image data
    \Input uImageLen    - size of image data

    \Output
        int32_t         - TRUE if a GIF, else FALSE

    \Version 03/09/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyGifIdentify(const uint8_t *pImageData, uint32_t uImageLen)
{
    // make sure we have enough data
    if (uImageLen < 6)
    {
        return(0);
    }
    // see of we're a GIF
    if (memcmp(pImageData, "GIF87a", 6) && memcmp(pImageData, "GIF89a", 6))
    {
        return(0);
    }
    return(1);
}

/*F********************************************************************************/
/*!
    \Function DirtyGifParse

    \Description
        Parse GIF header.

    \Input *pGifHdr     - [out] pointer to GIF header to fill in
    \Input *pGifData    - pointer to GIF data
    \Input *pGifEnd     - pointer past the end of GIF data

    \Version 11/13/2003 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyGifParse(DirtyGifHdrT *pGifHdr, const uint8_t *pGifData, const uint8_t *pGifEnd)
{
    return(_DirtyGifParseHeader(pGifHdr, pGifData, pGifEnd, NULL, 0));
}

/*F********************************************************************************/
/*!
    \Function DirtyGifParseEx

    \Description
        Parse GIF header, with extended info

    \Input *pGifHdr     - [out] pointer to GIF header to fill in
    \Input *pGifData    - pointer to GIF data
    \Input *pGifEnd     - pointer past the end of GIF data
    \Input *pFrames     - [out] storage for list of frames
    \Input uNumFrames   - size of output frame array; zero if unknown

    \Version 01/16/2020 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyGifParseEx(DirtyGifHdrT *pGifHdr, const uint8_t *pGifData, const uint8_t *pGifEnd, DirtyGifHdrT *pFrames, uint32_t uNumFrames)
{
    return(_DirtyGifParseHeader(pGifHdr, pGifData, pGifEnd, pFrames, uNumFrames));
}

/*F********************************************************************************/
/*!
    \Function DirtyGifDecodePalette

    \Description
        Decode a GIF palette into an RGBA palette.

    \Input *pGifHdr     - pointer to GIF header
    \Input *pPalette    - [out] pointer to output for RGBA palette
    \Input *pPaletteEnd - pointer past end of RGBA output buffer
    \Input uAlpha       - alpha value to use for normal pixels

    \Version 11/13/2003 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyGifDecodePalette(DirtyGifHdrT *pGifHdr, uint8_t *pPalette, uint8_t *pPaletteEnd, uint8_t uAlpha)
{
    const uint8_t *pColorTable;
    uint8_t *pPalStart = pPalette;

    // validate parameters
    if ((pGifHdr->pColorTable == NULL) || (pPalette == NULL))
    {
        return(-1);
    }
    
    // extract palette colors
    for (pColorTable = pGifHdr->pColorTable; pPalette < pPaletteEnd; pPalette += 4, pColorTable += 3)
    {
        pPalette[0] = pColorTable[0];
        pPalette[1] = pColorTable[1];
        pPalette[2] = pColorTable[2];
        pPalette[3] = uAlpha;
    }
    
    // handle alpha transparency
    if (pGifHdr->bHasAlpha)
    {
        uint8_t *pAlphaColor = pPalStart + (pGifHdr->uTransColor * 4);
        if (pAlphaColor < pPaletteEnd)
        {
            pAlphaColor[3] = 0x00;
        }
    }
    
    return(0);
}

/*F********************************************************************************/
/*!
    \Function DirtyGifDecodeImage

    \Description
        Decode a GIF image into an 8bit paletteized bitmap.
        
    \Input *pGifHdr     - pointer to header describing gif to decode
    \Input *pImageData  - [out] pointer to buffer to write decoded image data to
    \Input iBufWidth    - width of output buffer
    \Input iBufHeight   - height of output buffer
    \Input bVflip       - if TRUE, flip image vertically

    \Output
        int32_t         - positive=number of bytes decoded, negative=error

    \Version 11/13/2003 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyGifDecodeImage(DirtyGifHdrT *pGifHdr, uint8_t *pImageData, int32_t iBufWidth, int32_t iBufHeight, uint32_t bVflip)
{
    DirtyGifDecoderT Decoder, *pDecoder = &Decoder;
    
    // init the decoder
    if (_DirtyGifInitDecoder(pDecoder, pGifHdr, pImageData, iBufWidth, iBufHeight) < 0)
    {
        return(-1);
    }

    // decode the image
    for (;;)
    {
        // get next code
        if (_DirtyGifGetNextCode(pDecoder) <= 0)
        {
            break;
        }

        // iClearCode code?
        if (pDecoder->iCodeRaw == pDecoder->iClearCode)
        {
            // reset the decoder
            _DirtyGifResetDecoder(pDecoder);
            
            // get code following iClearCode code
            if (_DirtyGifGetNextCode(pDecoder) <= 0)
            {
                break;
            }

            // if code is out of range, set code=0 to protect against broken encoders
            if (pDecoder->iCodeRaw >= pDecoder->iSlot)
            {
                pDecoder->iCodeRaw = 0;
            }

            // update
            pDecoder->iCode0 = pDecoder->iCode1 = pDecoder->iCodeRaw;

            // push the single value onto stack
            if (pDecoder->pStackPtr < pDecoder->pStackEnd)
            {
                *pDecoder->pStackPtr++ = (uint8_t)pDecoder->iCodeRaw;
            }
        }
        else
        {
            // decode code to stack and update decoder
            _DirtyGifUpdateDecoder(pDecoder);
        }

        // write any decoded codes into bitmap
        if (_DirtyGifUpdateBitmap(pDecoder, bVflip) == 0)
        {
            // buffer is too small in height - bail early
            break;
        }
    }

    // number number of bytes processed
    return((int32_t)(pDecoder->pCodeData - pGifHdr->pImageData));
}

/*F********************************************************************************/
/*!
    \Function DirtyGifDecodeImage32

    \Description
        Decode a GIF image into a 32bit ARGB direct-color bitmap.

    \Input *pGifHdr     - pointer to header describing gif to decode
    \Input *pImageData  - [out] pointer to buffer to write decoded image data to
    \Input iBufWidth    - width of output buffer in pixels
    \Input iBufHeight   - height of output buffer in pixels
    \Input bVflip       - if TRUE, flip image vertically

    \Output
        int32_t         - positive=number of bytes decoded, negative=error

    \Notes
        This version may not always decode the first frame of a multi-frame GIF
        correctly; use DirtyGifDecodeImage32Multi() with iNumFrames=1 instead.

    \Version 03/09/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyGifDecodeImage32(DirtyGifHdrT *pGifHdr, uint8_t *pImageData, int32_t iBufWidth, int32_t iBufHeight, uint32_t bVflip)
{
    return(_DirtyGifDecodeImage32(pGifHdr, pImageData, NULL, NULL, iBufWidth, iBufHeight, bVflip));
}

/*F********************************************************************************/
/*!
    \Function DirtyGifDecodeImage32Multi

    \Description
        Decode a GIF image into a multiple 32bit ARGB direct-color bitmaps

    \Input *pGifHdr     - pointer to header describing gif to decode
    \Input *pFrameInfo  - frame info
    \Input *pImageData  - [out] pointer to buffer to write decoded images to
    \Input iBufWidth    - width of output buffer in pixels
    \Input iBufHeight   - height of output buffer in pixels
    \Input iNumFrames   - number of image frames to decode
    \Input bVflip       - if TRUE, flip image vertically

    \Output
        int32_t         - positive=number of bytes decoded, negative=error

    \Version 01/28/2020 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyGifDecodeImage32Multi(DirtyGifHdrT *pGifHdr, DirtyGifHdrT *pFrameInfo, uint8_t *pImageData, int32_t iBufWidth, int32_t iBufHeight, int32_t iNumFrames, uint32_t bVflip)
{
    int32_t iFrame, iFrameSize, iResult;
    DirtyGifHdrT GifHdr;
    uint8_t *pImageDataPrev;

    for (iFrame = 0, iResult = 0, iFrameSize = iBufWidth*iBufHeight*4, pImageDataPrev = NULL; (iFrame < iNumFrames) && (iResult == 0); iFrame += 1)
    {
        // copy frame info
        ds_memcpy_s(&GifHdr, sizeof(GifHdr), &pFrameInfo[iFrame], sizeof(pFrameInfo[iFrame]));

        // use global color table if not set for this frame
        if (pFrameInfo[iFrame].pColorTable == NULL)
        {
            GifHdr.pColorTable = pGifHdr->pColorTable;
            GifHdr.uNumColors = pGifHdr->uNumColors;
        }

        // decode to current buffer
        iResult = _DirtyGifDecodeImage32(&GifHdr, pImageData + iFrame*iFrameSize, pImageDataPrev, NULL, iBufWidth, iBufHeight, bVflip);

        // remember current frame for next decode
        pImageDataPrev = pImageData + iFrame*iFrameSize;
    }

    // return most recent result code
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function DirtyGifDecodeImage32Frame

    \Description
        Decode a specific frame of a GIF image into a single 32bit ARGB direct-color
        bitmap.  The frame order must be from the first frame to the last in sequence
        and the image data output buffer must be preserved between calls, except when
        decoding the first frame.

    \Input *pGifHdr     - pointer to header describing gif to decode
    \Input *pFrameInfo  - frame info
    \Input *pImageData  - [out] pointer to buffer to write decoded image to
    \Input *p8BitImage  - pointer to scratch buffer for 8bit image decoding
    \Input iBufWidth    - width of output buffer in pixels
    \Input iBufHeight   - height of output buffer in pixels
    \Input iFrame       - frame to decode
    \Input iNumFrames   - number of image frames to decode
    \Input bVflip       - if TRUE, flip image vertically

    \Output
        int32_t         - positive=numb

    \Version 02/06/2020 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyGifDecodeImage32Frame(DirtyGifHdrT *pGifHdr, DirtyGifHdrT *pFrameInfo, uint8_t *pImageData, uint8_t *p8BitImage, int32_t iBufWidth, int32_t iBufHeight, int32_t iFrame, int32_t iNumFrames, uint32_t bVflip)
{
    DirtyGifHdrT GifHdr;

    // copy frame info for the frame we want to decode
    ds_memcpy_s(&GifHdr, sizeof(GifHdr), &pFrameInfo[iFrame], sizeof(pFrameInfo[iFrame]));

    // use global color table if not set for this frame
    if (pFrameInfo[iFrame].pColorTable == NULL)
    {
        GifHdr.pColorTable = pGifHdr->pColorTable;
        GifHdr.uNumColors = pGifHdr->uNumColors;
    }

    // decode to current buffer and return result to caller
    return(_DirtyGifDecodeImage32(&GifHdr, pImageData, pImageData, p8BitImage, iBufWidth, iBufHeight, bVflip));
}

