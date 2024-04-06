/*H********************************************************************************/
/*!
    \File dirtypng.h

    \Description
        Routines to decode a PNG into a raw image and palette.  These routines
        were written from scratch based on the published specifications.  The
        functions and style were heavily based on the dirtygif.c and dirtyjpeg.c
        files coded by James Brookes.

    Supported features
        Bit Depth of 1, 2, 4, and 8
        Colour Type 0, 2, 4, 6 (grayscale, truecolor, grayscale with alpha, truecolor with alpha)
        Noncompressed blocks, statically compressed blocks, dynamically compressed blocks
        Zlib compression levels 0, 1, 2 and 9 (none through maximum)
        Interlace method Adam7
        Filter types 0-4 (none, sub, up, average, paeth)

    Unsupported features
        Bit Depth of  16 support
        Colour Type 3 (Indexed Color support)
        PLTE chunk support
        Ancillary chunk support

    \Notes
        References:
            [1] http://www.w3.org/TR/PNG/
            [2] http://www.gzip.org/zlib/rfc1950.pdf
            [3] http://www.gzip.org/zlib/rfc1951.pdf
            [4] http://www.zlib.net/feldspar.html
            [5] http://www.schaik.com/pngsuite/#basic

    \Copyright
        Copyright (c) 2007 Electronic Arts Inc.

    \Version 02/05/2007 (christianadam) First Version
*/
/********************************************************************************H*/

#ifndef _dirtypng_h
#define _dirtypng_h

/*!
\Moduledef DirtyPng DirtyPng
\Modulemember Graph
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

#define DIRTYPNG_ERR_NONE      (0)
#define DIRTYPNG_ERR_TOOSHORT  (-1)
#define DIRTYPNG_ERR_BADCRC    (-2)
#define DIRTYPNG_ERR_BADTYPE   (-3)
#define DIRTYPNG_ERR_BADCOLOR  (-4)
#define DIRTYPNG_ERR_BADDEPTH  (-5)
#define DIRTYPNG_ERR_BADCOMPR  (-6)
#define DIRTYPNG_ERR_BADFILTR  (-7)
#define DIRTYPNG_ERR_BADINTRL  (-8)
#define DIRTYPNG_ERR_ALLOCFAIL (-9)
#define DIRTYPNG_ERR_TYPEMISS  (-10)
#define DIRTYPNG_ERR_BADORDER  (-11)
#define DIRTYPNG_ERR_TYPEDUPL  (-12)
#define DIRTYPNG_ERR_UNKNCRIT  (-13)
#define DIRTYPNG_ERR_BADCM     (-14)
#define DIRTYPNG_ERR_BADCI     (-15)
#define DIRTYPNG_ERR_BADFLG    (-16)
#define DIRTYPNG_ERR_FDICTSET  (-17)
#define DIRTYPNG_ERR_BADBTYPE  (-18)
#define DIRTYPNG_ERR_NOBLKEND  (-19)
#define DIRTYPNG_ERR_BADBLKLEN (-20)
#define DIRTYPNG_ERR_MAXCODES  (-21)
#define DIRTYPNG_ERR_NOCODES   (-22)
#define DIRTYPNG_ERR_BADCODE   (-23)
#define DIRTYPNG_ERR_INVFILE   (-24)
#define DIRTYPNG_ERR_UNKNOWN   (-25)

typedef struct DirtyPngStateT DirtyPngStateT;

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! structure to represent a parsed png
typedef struct DirtyPngHdrT
{
    // image info
    const uint8_t *pImageData;  //!< pointer to start of png bits
    const uint8_t *pImageEnd;   //!< pointer to end of png bits
    uint32_t uWidth;            //!< image width
    uint32_t uHeight;           //!< image height

    // misc info
    int8_t iBitDepth;           //!< bit depth of the image
    int8_t iColourType;         //!< colour type of the image
    int8_t iCompression;        //!< compression method of the image
    int8_t iFilter;             //!< filter method of the image
    int8_t iInterlace;          //!< interlace method of the image
} DirtyPngHdrT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create module state
DIRTYCODE_API DirtyPngStateT *DirtyPngCreate(void);

// done with module state
DIRTYCODE_API void DirtyPngDestroy(DirtyPngStateT *pState);

// identify if image is a png or not
DIRTYCODE_API int32_t DirtyPngIdentify(const uint8_t *pImageData, uint32_t uImageLen);

// parse PNG into something we can use
DIRTYCODE_API int32_t DirtyPngParse(DirtyPngStateT *pState, DirtyPngHdrT *pPngHdr, const uint8_t *pImageData, const uint8_t *pImageEnd);

// decode a PNG image
DIRTYCODE_API int32_t DirtyPngDecodeImage(DirtyPngStateT *pState, DirtyPngHdrT *pPngHdr, uint8_t *pImageData, int32_t iBufWidth, int32_t iBufHeight);

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtypng_h
