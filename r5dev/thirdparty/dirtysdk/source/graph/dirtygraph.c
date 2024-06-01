/*H********************************************************************************/
/*!
    \File dirtygraph.c

    \Description
        Routines for decoding an encoded graphics image.

    \Copyright
        Copyright (c) 2006-2020 Electronic Arts Inc.

    \Version 03/09/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "DirtySDK/graph/dirtygraph.h"
#include "DirtySDK/graph/dirtygif.h"
#include "DirtySDK/graph/dirtyjpg.h"
#include "DirtySDK/graph/dirtypng.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct DirtyGraphBufferT
{
    uint8_t *pBuffer;
    int32_t iBufWidth;
    int32_t iBufHeight;
} DirtyGraphBufferT;

struct DirtyGraphRefT
{
    //! module memory group
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    DirtyGifHdrT *pGifFrameInfo;
    DirtyGraphBufferT GifImage8;
    DirtyJpgStateT *pJpg;
    DirtyPngStateT *pPng;
    DirtyGifHdrT GifHdr;
    DirtyJpgHdrT JpgHdr;
    DirtyPngHdrT PngHdr;
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function DirtyGraphCreate

    \Description
        Create the DirtyGraph module.

    \Output
        DirtyGraphStateT *  - module state, or NULL if unable to create

    \Version 03/09/2006 (jbrookes) First Version
*/
/********************************************************************************F*/
DirtyGraphRefT *DirtyGraphCreate(void)
{
    DirtyGraphRefT *pDirtyGraph;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    
    // allocate and init module state
    if ((pDirtyGraph = DirtyMemAlloc(sizeof(*pDirtyGraph), DIRTYGRAPH_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtygraph: could not allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pDirtyGraph, sizeof(*pDirtyGraph));
    pDirtyGraph->iMemGroup = iMemGroup;
    pDirtyGraph->pMemGroupUserData = pMemGroupUserData;

    // allocate jpeg module state
    if ((pDirtyGraph->pJpg = DirtyJpgCreate()) == NULL)
    {
        NetPrintf(("dirtygraph: unable to allocate jpg module state\n"));
    }

    // allocate png module state
    if ((pDirtyGraph->pPng = DirtyPngCreate()) == NULL)
    {
        NetPrintf(("dirtygraph: unable to allocate png module state\n"));
    }
    
    // return module state ref
    return(pDirtyGraph);
}

/*F********************************************************************************/
/*!
    \Function DirtyGraphDestroy

    \Description
        Destroy the DirtyGraph module.

    \Input *pDirtyGraph - pointer to module state

    \Output
        None.

    \Version 03/09/2006 (jbrookes)
*/
/********************************************************************************F*/
void DirtyGraphDestroy(DirtyGraphRefT *pDirtyGraph)
{
    // destroy gif 8bit frame buffer?
    if (pDirtyGraph->GifImage8.pBuffer != NULL)
    {
        DirtyMemFree(pDirtyGraph->GifImage8.pBuffer, DIRTYGRAPH_MEMID, pDirtyGraph->iMemGroup, pDirtyGraph->pMemGroupUserData);
    }
    // destroy gif frame info?
    if (pDirtyGraph->pGifFrameInfo != NULL)
    {
        DirtyMemFree(pDirtyGraph->pGifFrameInfo, DIRTYGRAPH_MEMID, pDirtyGraph->iMemGroup, pDirtyGraph->pMemGroupUserData);
    }
    // destroy jpeg module?
    if (pDirtyGraph->pJpg != NULL)
    {
        DirtyJpgDestroy(pDirtyGraph->pJpg);
    }
    // destroy png module?
    if (pDirtyGraph->pPng != NULL)
    {
        DirtyPngDestroy(pDirtyGraph->pPng);
    }
    // free module state
    DirtyMemFree(pDirtyGraph, DIRTYGRAPH_MEMID, pDirtyGraph->iMemGroup, pDirtyGraph->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function DirtyGraphDecodeHeader

    \Description
        Decode an image header

    \Input *pDirtyGraph - pointer to module state
    \Input *pInfo       - [out] storage for image info
    \Input *pImageData  - pointer to input image info
    \Input uImageLen    - size of input image
    
    \Output
        int32_t         - negative=error, else success

    \Version 03/09/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyGraphDecodeHeader(DirtyGraphRefT *pDirtyGraph, DirtyGraphInfoT *pInfo, const uint8_t *pImageData, uint32_t uImageLen)
{
    int32_t iError, iResult=-1;
    
    // init info structure
    ds_memclr(pInfo, sizeof(*pInfo));
    pInfo->pImageData = pImageData;
    pInfo->uLength = uImageLen;

    // try and identify image    
    if (DirtyGifIdentify(pImageData, uImageLen))
    {
        if ((iError = DirtyGifParse(&pDirtyGraph->GifHdr, pImageData, pImageData+uImageLen)) == 0)
        {
            pInfo->uType = DIRTYGRAPH_IMAGETYPE_GIF;
            pInfo->iWidth = pDirtyGraph->GifHdr.uWidth;
            pInfo->iHeight = pDirtyGraph->GifHdr.uHeight;
            pInfo->uNumFrames = pDirtyGraph->GifHdr.uNumFrames;
            // if we have a previous gif frame buffer, destroy it now
            if (pDirtyGraph->pGifFrameInfo != NULL)
            {
                DirtyMemFree(pDirtyGraph->pGifFrameInfo, DIRTYGRAPH_MEMID, pDirtyGraph->iMemGroup, pDirtyGraph->pMemGroupUserData);
            }
            // now allocate and parse frame data
            if ((pDirtyGraph->pGifFrameInfo = DirtyMemAlloc(sizeof(*pDirtyGraph->pGifFrameInfo)*pInfo->uNumFrames, DIRTYGRAPH_MEMID, pDirtyGraph->iMemGroup, pDirtyGraph->pMemGroupUserData)) != NULL)
            {
                iResult = DirtyGifParseEx(&pDirtyGraph->GifHdr, pImageData, pImageData+uImageLen, pDirtyGraph->pGifFrameInfo, pInfo->uNumFrames);
            }
            else
            {
                NetPrintf(("dirtygraph: unable to allocate memory for gif frames list\n"));
                iResult = -1;
            }
        }
        else
        {
            NetPrintf(("dirtygraph: error %d parsing gif image\n", iError));
        }
        
    }
    else if ((pDirtyGraph->pJpg != NULL) && DirtyJpgIdentify(pDirtyGraph->pJpg, pImageData, uImageLen))
    {
        if ((iError = DirtyJpgDecodeHeader(pDirtyGraph->pJpg, &pDirtyGraph->JpgHdr, pImageData, uImageLen)) == DIRTYJPG_ERR_NONE)
        {
            pInfo->uType = DIRTYGRAPH_IMAGETYPE_JPG;
            pInfo->iWidth = pDirtyGraph->JpgHdr.uWidth;
            pInfo->iHeight = pDirtyGraph->JpgHdr.uHeight;
            pInfo->uNumFrames = 1;
            iResult = 0;
        }
        else
        {
            NetPrintf(("dirtygraph: error %d parsing jpg image\n", iError));
        }
    }
    else if ((pDirtyGraph->pPng != NULL) && DirtyPngIdentify(pImageData, uImageLen))
    {
        if ((iError = DirtyPngParse(pDirtyGraph->pPng, &pDirtyGraph->PngHdr, pImageData, pImageData+uImageLen)) == DIRTYPNG_ERR_NONE)
        {
            pInfo->uType = DIRTYGRAPH_IMAGETYPE_PNG;
            pInfo->iWidth = pDirtyGraph->PngHdr.uWidth;
            pInfo->iHeight = pDirtyGraph->PngHdr.uHeight;
            pInfo->uNumFrames = 1;
            iResult = 0;
        }
        else
        {
            NetPrintf(("dirtygraph: error %d parsing png image\n", iError));
        }
    }
    else
    {
        NetPrintf(("dirtygraph: cannot parse image of unknown type\n"));
        pInfo->uType = DIRTYGRAPH_IMAGETYPE_UNKNOWN;
        pInfo->pImageData = NULL;
        pInfo->uLength = 0;
    }

    // return result to caller    
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function DirtyGraphGetImageInfo

    \Description
        Get extended information about an image

    \Input *pDirtyGraph - pointer to module state
    \Input *pInfo       - image information (filled in by DirtyGraphDecodeHeader)
    \Input iSelect      - info selector
    \Input *pBuffer     - [out] selector output
    \Input iBufSize     - size of output buffer
    
    \Output
        int32_t         - selector return

    \Notes
        Selectors are:

    \verbatim
        'anim'      fills buffer with int32_t anim info (delay) in milliseconds, and returns number of frames
    \endverbatim

    \Version 01/27/2020 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyGraphGetImageInfo(DirtyGraphRefT *pDirtyGraph, DirtyGraphInfoT *pInfo, int32_t iSelect, void *pBuffer, int32_t iBufSize)
{
    if ((iSelect == 'anim') && (pInfo->uType == DIRTYGRAPH_IMAGETYPE_GIF) && (pBuffer != NULL) && (iBufSize == (signed)(pInfo->uNumFrames*sizeof(int32_t))))
    {
        int32_t *pDelayBuf = (int32_t *)pBuffer, iFrame;
        // collect delay info in integer output array
        for (iFrame = 0; iFrame < pInfo->uNumFrames; iFrame += 1)
        {
            pDelayBuf[iFrame] = pDirtyGraph->pGifFrameInfo[iFrame].uDelay;
        }
        // return count of delay frame values copied to output buffer
        return(iFrame);
    }
    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function DirtyGraphDecodeImage

    \Description
        Decode an image into the given output buffer.

    \Input *pDirtyGraph - pointer to module state
    \Input *pInfo       - image information (filled in by DirtyGraphDecodeHeader)
    \Input *pImageBuf   - [out] pointer to buffer to store decoded image
    \Input iBufWidth    - width of output buffer
    \Input iBufHeight   - height of output buffer
    
    \Output
        int32_t         - negative=error, else success

    \Version 03/09/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyGraphDecodeImage(DirtyGraphRefT *pDirtyGraph, DirtyGraphInfoT *pInfo, uint8_t *pImageBuf, int32_t iBufWidth, int32_t iBufHeight)
{
    if (pInfo->uType == DIRTYGRAPH_IMAGETYPE_GIF)
    {
        return(DirtyGifDecodeImage32Multi(&pDirtyGraph->GifHdr, pDirtyGraph->pGifFrameInfo, pImageBuf, iBufWidth, iBufHeight, 1, TRUE));
    }
    else if (pInfo->uType == DIRTYGRAPH_IMAGETYPE_JPG)
    {
        return(DirtyJpgDecodeImage(pDirtyGraph->pJpg, &pDirtyGraph->JpgHdr, pImageBuf, iBufWidth, iBufHeight));
    }
    else if (pInfo->uType == DIRTYGRAPH_IMAGETYPE_PNG)
    {
        return(DirtyPngDecodeImage(pDirtyGraph->pPng, &pDirtyGraph->PngHdr, pImageBuf, iBufWidth, iBufHeight));
    }
    else
    {
        NetPrintf(("dirtygraph: cannot decode image of unknown type\n"));
        return(-1);
    }
}

/*F********************************************************************************/
/*!
    \Function DirtyGraphDecodeImageMulti

    \Description
        Decode a multiframe image into an array of images in the given output buffer.

    \Input *pDirtyGraph - pointer to module state
    \Input *pInfo       - image information (filled in by DirtyGraphDecodeHeader)
    \Input *pImageBuf   - [out] pointer to buffer to store decoded images
    \Input iBufWidth    - width of output buffer
    \Input iBufHeight   - height of output buffer
    
    \Output
        int32_t         - negative=error, else success

    \Version 01/28/2020 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyGraphDecodeImageMulti(DirtyGraphRefT *pDirtyGraph, DirtyGraphInfoT *pInfo, uint8_t *pImageBuf, int32_t iBufWidth, int32_t iBufHeight)
{
    if ((pInfo->uType == DIRTYGRAPH_IMAGETYPE_GIF) && (pDirtyGraph->pGifFrameInfo != NULL))
    {
        return(DirtyGifDecodeImage32Multi(&pDirtyGraph->GifHdr, pDirtyGraph->pGifFrameInfo, pImageBuf, iBufWidth, iBufHeight, pDirtyGraph->GifHdr.uNumFrames, TRUE));
    }
    else
    {
        NetPrintf(("dirtygraph: multi-image decoding not supported for image type\n"));
        return(-1);
    }
}

/*F********************************************************************************/
/*!
    \Function DirtyGraphDecodeImageFrame

    \Description
        Decode a multiframe image into the given output buffer.  The frame order must
        be from the first frame to the last in sequence and the image data output
        buffer must be preserved between calls, except when decoding the first frame.

    \Input *pDirtyGraph - pointer to module state
    \Input *pInfo       - image information (filled in by DirtyGraphDecodeHeader)
    \Input *pImageBuf   - [out] pointer to buffer to store decoded image
    \Input iBufWidth    - width of output buffer
    \Input iBufHeight   - height of output buffer
    \Input iFrame       - frame to decode
    
    \Output
        int32_t         - negative=error, else success

    \Version 02/06/2020 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyGraphDecodeImageFrame(DirtyGraphRefT *pDirtyGraph, DirtyGraphInfoT *pInfo, uint8_t *pImageBuf, int32_t iBufWidth, int32_t iBufHeight, int32_t iFrame)
{
    if ((pInfo->uType == DIRTYGRAPH_IMAGETYPE_GIF) && (pDirtyGraph->pGifFrameInfo != NULL))
    {
        // if eight-bit frame cache exists but the frame size has changed, kill it
        if ((pDirtyGraph->GifImage8.pBuffer != NULL) && ((pDirtyGraph->GifImage8.iBufWidth != iBufWidth) || (pDirtyGraph->GifImage8.iBufHeight != iBufHeight)))
        {
            DirtyMemFree(pDirtyGraph->GifImage8.pBuffer, DIRTYGRAPH_MEMID, pDirtyGraph->iMemGroup, pDirtyGraph->pMemGroupUserData);
            pDirtyGraph->GifImage8.pBuffer = NULL;
        }
        // allocate eight-bit frame cache
        if (pDirtyGraph->GifImage8.pBuffer == NULL)
        {
            if ((pDirtyGraph->GifImage8.pBuffer = DirtyMemAlloc(iBufWidth * iBufHeight, DIRTYGRAPH_MEMID, pDirtyGraph->iMemGroup, pDirtyGraph->pMemGroupUserData)) == NULL)
            {
                NetPrintf(("dirtygraph: could not allocate 8bit decode buffer\n"));
                return(-1);
            }
            pDirtyGraph->GifImage8.iBufWidth = iBufWidth;
            pDirtyGraph->GifImage8.iBufHeight = iBufHeight;
        }
        // decode the frame and return result to caller
        return(DirtyGifDecodeImage32Frame(&pDirtyGraph->GifHdr, pDirtyGraph->pGifFrameInfo, pImageBuf, pDirtyGraph->GifImage8.pBuffer, iBufWidth, iBufHeight, iFrame, pDirtyGraph->GifHdr.uNumFrames, TRUE));
    }
    else
    {
        NetPrintf(("dirtygraph: multi-image decoding not supported for image type\n"));
        return(-1);
    }
}