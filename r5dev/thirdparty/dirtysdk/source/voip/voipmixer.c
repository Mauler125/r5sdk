/*H********************************************************************************/
/*!
    \File voipmixer.c

    \Description
        VoIP audio mixer

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 07/29/2004 (jbrookes) Split from voipheadset.c
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"
#include "voipmixer.h"

/*** Defines **********************************************************************/

//! max number of supported mixbuffers
#define VOIP_MAXMIXBUFFERS  (16)

/*** Macros ***********************************************************************/

//! TRUE if value is a power of two, else FALSE
#define VOIP_PowerOf2(_val)                 ( ((_val) & -(_val)) == (_val) )

//! index the next mixbuffer
#define VOIP_NextMixBuffer(_pRef, _iCur)    (((_iCur) + 1) % ((_pRef)->iNumMixBuffers))

//! absolute value of an 32bit integer
#define VOIP_Abs32(_val)                    (((_val) ^ ((_val) >> 31)) - ((_val) >> 31))

/*** Type Definitions *************************************************************/

typedef struct VoipMixBufferT
{
    int32_t *pMixBuffer;
    int32_t iMixCount;
    int32_t iMixMask;
} VoipMixBufferT;

//! VOIP module state data
struct VoipMixerRefT
{
    //! number of mixbuffers
    int32_t iNumMixBuffers;

    //! current mix buffer to mix into
    int32_t iCurMixBuffer;

    //! size of a single mixbuffer
    int32_t iMixBufferSize;

    //! mixbuffer list
    VoipMixBufferT MixBuffers[VOIP_MAXMIXBUFFERS];
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Public Variables

// Private Variables

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _VoipMixerPostMixSingle

    \Description
        Processes the integer accumulation buffer in the case where only one
        data packet was accumulated.

    \Input *pDst        - [out] output play buffer
    \Input *pSrc        - mixbuffer pointer
    \Input iNumSamples  - number of samples to mix

    \Output
        None.

    \Version 04/02/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipMixerPostMixSingle(int16_t *pDst, int32_t *pSrc, int32_t iNumSamples)
{
    int32_t iData;

    // process data
    for (iData = 0; iData < iNumSamples; iData++)
    {
        pDst[iData] = (int16_t)pSrc[iData];
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipMixerPostMixMulti

    \Description
        Processes the integer accumulation buffer in the case where more than one
        packet was accumulated.

    \Input *pDst        - [out] output play buffer
    \Input *pSrc        - mixbuffer pointer
    \Input uBufMax      - largest absolute value in sample buffer
    \Input iNumSamples  - number of samples to mix

    \Output
        None.

    \Version 04/02/2004 (jbrookes) Split from VoipProcessHeadset()
*/
/********************************************************************************F*/
static void _VoipMixerPostMixMulti(int16_t *pDst, int32_t *pSrc, uint32_t uBufMax, int32_t iNumSamples)
{
    int32_t iData;
    int32_t iMult;
    int64_t lVal;

    // calculate scaler
    iMult = (0x7fffffff/uBufMax)*32767;

    // scale buffer data
    for (iData = 0; iData < iNumSamples; iData++)
    {
        lVal = (int64_t)(pSrc[iData]<<1) * (int64_t)iMult;
        pDst[iData] = (int32_t)(lVal >> 32);
    }
}


/*** Public functions **************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipMixerCreate

    \Description
        Create the mixer module.

    \Input uMixBuffers      - number of mixbuffers to implement
    \Input uFrameSamples    - size of a frame in samples

    \Output
        VoipMixerRefT * - pointer to mixer module, or NULL if an error occurred

    \Version 07/29/2004 (jbrookes)
*/
/********************************************************************************F*/
VoipMixerRefT *VoipMixerCreate(uint32_t uMixBuffers, uint32_t uFrameSamples)
{
    VoipMixerRefT *pMixerRef;
    int32_t iMixBuffer, iMixBufferSize;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // make sure mixbuffer count is between 2 and VOIP_MAXMIXBUFFERS
    if (uMixBuffers < 2)
    {
        NetPrintf(("voipmixer: clamping create request of %d mixbuffers to 2\n", uMixBuffers));
        uMixBuffers = 2;
    }
    if (uMixBuffers > VOIP_MAXMIXBUFFERS)
    {
        NetPrintf(("voipmixer: clamping create request of %d mixbuffers to %d\n", uMixBuffers, VOIP_MAXMIXBUFFERS));
        uMixBuffers = VOIP_MAXMIXBUFFERS;
    }

    // allocate and wipe module state
    VoipCommonMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    if ((pMixerRef = DirtyMemAlloc(sizeof(*pMixerRef), VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipmixer: unable to allocate %d bytes for mixer state\n", sizeof(*pMixerRef)));
        return(NULL);
    }
    ds_memclr(pMixerRef, sizeof(*pMixerRef));

    // init ref
    pMixerRef->iNumMixBuffers = uMixBuffers;
    pMixerRef->iMixBufferSize = uFrameSamples;

    // allocate mix buffers
    iMixBufferSize = pMixerRef->iNumMixBuffers * pMixerRef->iMixBufferSize * sizeof(int32_t);
    if ((pMixerRef->MixBuffers[0].pMixBuffer = DirtyMemAlloc(iMixBufferSize, VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipmixer: unable to allocate %d bytes for %d mixbuffers\n", iMixBufferSize, pMixerRef->iNumMixBuffers));
        DirtyMemFree(pMixerRef, VOIP_MEMID, iMemGroup, pMemGroupUserData);
        return(NULL);
    }
    ds_memclr(pMixerRef->MixBuffers[0].pMixBuffer, iMixBufferSize);

    // set up mixbuffers 1...N
    for (iMixBuffer = 1; iMixBuffer < pMixerRef->iNumMixBuffers; iMixBuffer++)
    {
        pMixerRef->MixBuffers[iMixBuffer].pMixBuffer = pMixerRef->MixBuffers[iMixBuffer-1].pMixBuffer + pMixerRef->iMixBufferSize;
    }

    // return module state to caller
    return(pMixerRef);
}

/*F********************************************************************************/
/*!
    \Function VoipMixerDestroy

    \Description
        Destroy the mixer module.

    \Input  *pMixerRef  - pointer to mixer module

    \Output
        None.

    \Version 07/29/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipMixerDestroy(VoipMixerRefT *pMixerRef)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query mem group data
    VoipCommonMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    DirtyMemFree(pMixerRef->MixBuffers[0].pMixBuffer, VOIP_MEMID, iMemGroup, pMemGroupUserData);
    DirtyMemFree(pMixerRef, VOIP_MEMID, iMemGroup, pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function VoipMixerAccumulate

    \Description
        Decodes and accumulates incoming data into mixbuffer.

    \Input  *pMixerRef          - pointer to mixer module
    \Input  *pInputData         - pointer to input voice data
    \Input  iDataSize           - size of incoming voice data
    \Input  iMixMask            - bitmask uniquely representing who is doing the writing
    \Input  iChannel            - input channel number

    \Output
        int32_t                 - negative=error, else next mixbuffer to write into

    \Version 07/29/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipMixerAccumulate(VoipMixerRefT *pMixerRef, uint8_t *pInputData, int32_t iDataSize, int32_t iMixMask, int32_t iChannel)
{
    int32_t iNumSamples;
    int32_t iIndex, iMixBuffer;

    iMixBuffer = pMixerRef->iCurMixBuffer;

    // find the first spot that we can write into for this conduit
    for (iIndex = 0; iIndex != pMixerRef->iNumMixBuffers; iIndex++)
    {
        // if we've found a spot
        if (!(pMixerRef->MixBuffers[iMixBuffer].iMixMask & iMixMask))
        {
            break;
        }

        iMixBuffer = (iMixBuffer + 1) % pMixerRef->iNumMixBuffers;
    }

    // make sure we're not accumulating into a mixbuffer we've already written into
    if (iIndex == pMixerRef->iNumMixBuffers)
    {
        return(-1);
    }

    // decode into mixbuffer
    iNumSamples = VoipCodecDecode(pMixerRef->MixBuffers[iMixBuffer].pMixBuffer, pInputData, iDataSize, iChannel);

    // validate size
    if (iNumSamples != pMixerRef->iMixBufferSize)
    {
        NetPrintf(("voipmixer: warning, expecting %d samples but got %d samples\n", pMixerRef->iMixBufferSize, iNumSamples));
    }

    // increment the mixcount for the mixbuffer and mark that we've written into it
    pMixerRef->MixBuffers[iMixBuffer].iMixCount += 1;
    pMixerRef->MixBuffers[iMixBuffer].iMixMask |= iMixMask;

    // swap this channel to the next mixbuffer
    iMixBuffer = VOIP_NextMixBuffer(pMixerRef, iMixBuffer);
    return(iMixBuffer);
}

/*F********************************************************************************/
/*!
    \Function VoipMixerProcess

    \Description
        Mixes queued data into one output buffer suitable for playback.

    \Input *pMixerRef   - pointer to mixer
    \Input *pFrameData  - pointer to output

    \Output
        int32_t         - number of bytes written to output buffer

    \Version 04/02/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipMixerProcess(VoipMixerRefT *pMixerRef, uint8_t *pFrameData)
{
    uint32_t uAbsVal, uBufMax;
    int32_t *pMixBuffer;
    int32_t iSample;

    // make sure we have data in the mixbuffer
    if (pMixerRef->MixBuffers[pMixerRef->iCurMixBuffer].iMixCount == 0)
    {
        return(0);
    }
    pMixBuffer = pMixerRef->MixBuffers[pMixerRef->iCurMixBuffer].pMixBuffer;

    // calculate the buffer absolute max
    for (iSample = 0, uBufMax = 0; iSample < pMixerRef->iMixBufferSize; iSample++)
    {
        uAbsVal = (uint32_t)VOIP_Abs32(pMixBuffer[iSample]);
        if (uBufMax < uAbsVal)
        {
            uBufMax = uAbsVal;
        }
    }

    // if the buffer max was less than our max sample value
    if (uBufMax < 32767)
    {
        _VoipMixerPostMixSingle((int16_t *)pFrameData, pMixBuffer, pMixerRef->iMixBufferSize);
    }
    else
    {
        _VoipMixerPostMixMulti((int16_t *)pFrameData, pMixBuffer, uBufMax, pMixerRef->iMixBufferSize);
    }

    // reset current mixbuffer
    ds_memclr(pMixBuffer, pMixerRef->iMixBufferSize*sizeof(int32_t));
    pMixerRef->MixBuffers[pMixerRef->iCurMixBuffer].iMixCount = 0;
    pMixerRef->MixBuffers[pMixerRef->iCurMixBuffer].iMixMask = 0;

    // index next mixbuffer
    pMixerRef->iCurMixBuffer = VOIP_NextMixBuffer(pMixerRef, pMixerRef->iCurMixBuffer);
    return(pMixerRef->iMixBufferSize*2);
}
