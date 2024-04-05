/*H********************************************************************************/
/*!
    \File dirtygif.h

    \Description
        Routines to parse and decode a GIF file.

    \Copyright
        Copyright (c) 2003-2020 Electronic Arts Inc.

    \Version 11/13/2003 (jbrookes) First Version
    \Version 01/28/2020 (jbrookes) Added multiframe (animated) support
*/
/********************************************************************************H*/

#ifndef _dirtygif_h
#define _dirtygif_h

/*!
\Moduledef DirtyGif DirtyGif
\Modulemember Graph
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! structure to represent a parsed gif/gif frame
typedef struct DirtyGifHdrT
{
    // image info
    const uint8_t *pImageData;  //!< pointer to start of gif bits
    const uint8_t *pImageEnd;   //!< pointer to end of gif bits
    const uint8_t *pColorTable; //!< pointer to gif color table

    uint32_t uNumFrames;        //!< number of image frames

    uint16_t uTop;              //!< top offset of image frame
    uint16_t uLeft;             //!< left offset of image frame
    uint16_t uWidth;            //!< image width
    uint16_t uHeight;           //!< image height
    uint16_t uNumColors;        //!< number of color table entries
    uint16_t uDelay;            //!< time delay after displaying current frame before moving to next in 1/100ths of a second

    // misc info
    uint8_t bInterlaced;        //!< flag indicating an interlaced image
    uint8_t bHasAlpha;          //!< flag indicating transparent color present
    uint8_t uTransColor;        //!< which color index is transparent
    uint8_t pad;
} DirtyGifHdrT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// identify if image is a gif or not
DIRTYCODE_API int32_t DirtyGifIdentify(const uint8_t *pImageData, uint32_t uImageLen);

// parse GIF into something we can use
DIRTYCODE_API int32_t DirtyGifParse(DirtyGifHdrT *pGifHdr, const uint8_t *pGifData, const uint8_t *pGifEnd);

// parse GIF with extended info
DIRTYCODE_API int32_t DirtyGifParseEx(DirtyGifHdrT *pGifHdr, const uint8_t *pGifData, const uint8_t *pGifEnd, DirtyGifHdrT *pFrames, uint32_t uNumFrames);

// decode a GIF palette to 32bit color table
DIRTYCODE_API int32_t DirtyGifDecodePalette(DirtyGifHdrT *pGifHdr, uint8_t *pPalette, uint8_t *pPaletteEnd, uint8_t uAlpha);

// decode a GIF image to 8bit paletted image
DIRTYCODE_API int32_t DirtyGifDecodeImage(DirtyGifHdrT *pGifHdr, uint8_t *pImageData, int32_t iBufWidth, int32_t iBufHeight, uint32_t bVflip);

// decode a GIF image to 32bit direct-color image
DIRTYCODE_API int32_t DirtyGifDecodeImage32(DirtyGifHdrT *pGifHdr, uint8_t *pImageData, int32_t iBufWidth, int32_t iBufHeight, uint32_t bVflip);

// decode a GIF image to one or more 32bit direct-color images
DIRTYCODE_API int32_t DirtyGifDecodeImage32Multi(DirtyGifHdrT *pGifHdr, DirtyGifHdrT *pFrameInfo, uint8_t *pImageData, int32_t iBufWidth, int32_t iBufHeight, int32_t iNumFrames, uint32_t bVflip);

// decode a specific frame of a GIF image into a single 32bit ARGB direct-color bitmap.
DIRTYCODE_API int32_t DirtyGifDecodeImage32Frame(DirtyGifHdrT *pGifHdr, DirtyGifHdrT *pFrameInfo, uint8_t *pImageData, uint8_t *p8BitImage, int32_t iBufWidth, int32_t iBufHeight, int32_t iFrame, int32_t iNumFrames, uint32_t bVflip);

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtygif_h
