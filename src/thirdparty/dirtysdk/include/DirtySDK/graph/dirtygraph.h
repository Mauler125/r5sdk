/*H********************************************************************************/
/*!
    \File dirtygraph.h

    \Description
        Routines for decoding an encoded graphics image.

    \Copyright
        Copyright (c) 2006-2020 Electronic Arts Inc.

    \Version 03/09/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _dirtygraph_h
#define _dirtygraph_h

/*!
\Moduledef DirtyGraph DirtyGraph
\Modulemember Graph
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

//! supported image types
typedef enum DirtyGraphImageTypeE
{
    DIRTYGRAPH_IMAGETYPE_UNKNOWN = 0,
    DIRTYGRAPH_IMAGETYPE_GIF,
    DIRTYGRAPH_IMAGETYPE_JPG,
    DIRTYGRAPH_IMAGETYPE_PNG
} DirtyGraphImageTypeE;

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! structure representing parsed image info
typedef struct DirtyGraphInfoT
{
    const uint8_t   *pImageData;    //!< pointer to start of data
    uint32_t        uLength;        //!< length of file
    int16_t         iWidth;         //!< image width
    int16_t         iHeight;        //!< image height
    uint16_t        uNumFrames;     //!< number of image frames
    uint8_t         uType;          //!< image type
    uint8_t         _pad;
} DirtyGraphInfoT;

//! opaque module state
typedef struct DirtyGraphRefT DirtyGraphRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create module state
DIRTYCODE_API DirtyGraphRefT *DirtyGraphCreate(void);

// done with module state
DIRTYCODE_API void DirtyGraphDestroy(DirtyGraphRefT *pRef);

// parse the header
DIRTYCODE_API int32_t DirtyGraphDecodeHeader(DirtyGraphRefT *pRef, DirtyGraphInfoT *pInfo, const uint8_t *pImageData, uint32_t uImageLen);

// get extra image info
DIRTYCODE_API int32_t DirtyGraphGetImageInfo(DirtyGraphRefT *pRef, DirtyGraphInfoT *pInfo, int32_t iSelect, void *pBuffer, int32_t iBufSize);

// decode an image
DIRTYCODE_API int32_t DirtyGraphDecodeImage(DirtyGraphRefT *pRef, DirtyGraphInfoT *pInfo, uint8_t *pImageBuf, int32_t iBufWidth, int32_t iBufHeight);

// decode a multiframe image
DIRTYCODE_API int32_t DirtyGraphDecodeImageMulti(DirtyGraphRefT *pRef, DirtyGraphInfoT *pInfo, uint8_t *pImageBuf, int32_t iBufWidth, int32_t iBufHeight);

// decode a single frame of a multiframe image
DIRTYCODE_API int32_t DirtyGraphDecodeImageFrame(DirtyGraphRefT *pDirtyGraph, DirtyGraphInfoT *pInfo, uint8_t *pImageBuf, int32_t iBufWidth, int32_t iBufHeight, int32_t iFrame);

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtygraph_h
