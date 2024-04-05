/*H********************************************************************************/
/*!
    \File dirtyjpg.h

    \Description
        Routines to parse and decode a JPEG image file.  JFIF and EXIF file formats
        are supported.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 02/23/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _dirtyjpg_h
#define _dirtyjpg_h

/*!
\Moduledef DirtyJpg DirtyJpg
\Modulemember Graph
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

#define DIRTYJPG_ERR_NONE      (0)
#define DIRTYJPG_ERR_NOMAGIC   (-1)
#define DIRTYJPG_ERR_TOOSHORT  (-2)
#define DIRTYJPG_ERR_NOBUFFER  (-3)
#define DIRTYJPG_ERR_BADSTATE  (-4)
#define DIRTYJPG_ERR_BADFRAME  (-5)
#define DIRTYJPG_ERR_BADVERS   (-6)
#define DIRTYJPG_ERR_ENDOFDATA (-7)
#define DIRTYJPG_ERR_BADDQT    (-8)
#define DIRTYJPG_ERR_BADDHT    (-9)
#define DIRTYJPG_ERR_BADSOF0   (-10)
#define DIRTYJPG_ERR_BITDEPTH  (-11)   // unsupported bitdepth
#define DIRTYJPG_ERR_DECODER   (-12)   // decoding error
#define DIRTYJPG_ERR_NOSUPPORT (-13)   // unsupported feature
#define DIRTYJPG_ERR_NOFORMAT  (-14)   // unsupported format (not JFIF or EXIF)

typedef struct DirtyJpgStateT DirtyJpgStateT;

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! structure to represent a parsed gif
typedef struct DirtyJpgHdrT
{
    // image info
    const uint8_t   *pImageData;    //!< pointer to start of data
    uint32_t        uLength;        //!< length of file
    uint32_t        uWidth;         //!< image width
    uint32_t        uHeight;        //!< image height
} DirtyJpgHdrT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create module state
DIRTYCODE_API DirtyJpgStateT *DirtyJpgCreate(void);

// reset to default
DIRTYCODE_API void DirtyJpgReset(DirtyJpgStateT *pState);

// done with module state
DIRTYCODE_API void DirtyJpgDestroy(DirtyJpgStateT *pState);

// identify if image is a jpg or not
DIRTYCODE_API int32_t DirtyJpgIdentify(DirtyJpgStateT *pState, const uint8_t *pImage, uint32_t uLength);

// parse the header
DIRTYCODE_API int32_t DirtyJpgDecodeHeader(DirtyJpgStateT *pState, DirtyJpgHdrT *pJpgHdr, const uint8_t *pImage, uint32_t uLength);

// parse the image
DIRTYCODE_API int32_t DirtyJpgDecodeImage(DirtyJpgStateT *pState, DirtyJpgHdrT *pJpgHdr, uint8_t *pImageData, int32_t iBufWidth, int32_t iBufHeight);

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtyjpg_h
