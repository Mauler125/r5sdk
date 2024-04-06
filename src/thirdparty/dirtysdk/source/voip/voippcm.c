/*H********************************************************************************/
/*!
    \File voippcm.c

    \Description
        PCM codec (ie, pass-through).

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 08/04/2004 (jbrookes) First version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"
#include "voippcm.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct VoipPCMStateT
{
    VoipCodecRefT CodecState;

    int32_t iOutputVolume;
} VoipPCMStateT;

/*** Function Prototypes **********************************************************/

static VoipCodecRefT *_VoipPCMCreate(int32_t iNumDecoders);
static void _VoipPCMDestroy(VoipCodecRefT *pState);
static int32_t _VoipPCMEncodeBlock(VoipCodecRefT *pState, uint8_t *pOut, const int16_t *pInp, int32_t iNumSamples);
static int32_t _VoipPCMDecodeBlock(VoipCodecRefT *pState, int32_t *pOut, const uint8_t *pInp, int32_t iInputBytes, int32_t iChannel);
static void _VoipPCMReset(VoipCodecRefT *pState);
static int32_t _VoipPCMControl(VoipCodecRefT *pState, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);
static int32_t _VoipPCMStatus(VoipCodecRefT *pCodecState, int32_t iSelect, int32_t iValue, void *pBuffer, int32_t iBufSize);

/*** Variables ********************************************************************/

// Public variables

const VoipCodecDefT VoipPCM_CodecDef =
{
    _VoipPCMCreate,
    _VoipPCMDestroy,
    _VoipPCMEncodeBlock,
    _VoipPCMDecodeBlock,
    _VoipPCMReset,
    _VoipPCMControl,
    _VoipPCMStatus,
};

/*** Private Functions ************************************************************/


/*F*************************************************************************************************/
/*!
    \Function _VoipPCMCreate

    \Description
        Create a PCM codec state.

    \Input  iDecodeChannels - number of decoder channels

    \Output
        VoipCodecStateT *   - pointer to pcm codec state

    \Version 08/04/2004 (jbrookes)
*/
/*************************************************************************************************F*/
static VoipCodecRefT *_VoipPCMCreate(int32_t iDecodeChannels)
{
    VoipPCMStateT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    VoipCommonMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    pState = (VoipPCMStateT *) DirtyMemAlloc (sizeof(*pState), VOIP_MEMID, iMemGroup, pMemGroupUserData);
    pState->CodecState.pCodecDef = &VoipPCM_CodecDef;
    pState->CodecState.iDecodeChannels = iDecodeChannels;

    // set the output level
    pState->iOutputVolume = 1 << VOIP_CODEC_OUTPUT_FRACTIONAL;

    return(&pState->CodecState);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipPCMDestroy

    \Description
        Destroy a PCM codec state.

    \Input  *pState - state to destroy

    \Version 08/04/2004 (jbrookes)
*/
/*************************************************************************************************F*/
static void _VoipPCMDestroy(VoipCodecRefT *pState)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    VoipCommonMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    DirtyMemFree(pState, VOIP_MEMID, iMemGroup, pMemGroupUserData);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipPCMEncodeBlock

    \Description
        Encode a 16-bit linear PCM buffer.

    \Input *pState      - pointer to encode state
    \Input *pOut        - pointer to output buffer
    \Input *pInp        - pointer to input buffer
    \Input iNumSamples  - number of samples to encode

    \Output int32_t     - size of compressed data in bytes

    \Version 08/04/2004 (jbrookes)
*/
/*************************************************************************************************F*/
static int32_t _VoipPCMEncodeBlock(VoipCodecRefT *pState, uint8_t *pOut, const int16_t *pInp, int32_t iNumSamples)
{
    int32_t iCount;
    uint8_t *pBeg = pOut;

    // copy data to output buffer
    for (iCount = 0; iCount < iNumSamples; iCount += 1)
    {
        *pOut++ = (pInp[iCount] & 0xff00) >> 8;
        *pOut++ = (pInp[iCount] & 0xff);
    }

    // return the length
    return(pOut - pBeg);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipPCMDecodeBlock

    \Description
        Decode input to 16-bit linear PCM samples, and accumulate in the given output buffer.

    \Input *pState      - pointer to decode state
    \Input *pOut        - pointer to output buffer
    \Input *pInp        - pointer to input buffer
    \Input iInputBytes  - size of input data
    \Input iChannel     - ignored

    \Output
        int32_t         - number of samples decoded

    \Version 08/04/2004 (jbrookes)
*/
/*************************************************************************************************F*/
static int32_t _VoipPCMDecodeBlock(VoipCodecRefT *pState, int32_t *pOut, const uint8_t *pInp, int32_t iInputBytes, int32_t iChannel)
{
    VoipPCMStateT *pPCMState = (VoipPCMStateT *)pState;
    int32_t iCount, iSample;

    // decode input and add it to output
    for (iCount = 0; iCount < iInputBytes/2; iCount++)
    {
        // read sample
        iSample = *pInp++ << 8;
        iSample |= *pInp++;

        // sign extend
        iSample = (iSample << 16) >> 16;

        // write out with output scaling
        pOut[iCount] += ((iSample * pPCMState->iOutputVolume) >> VOIP_CODEC_OUTPUT_FRACTIONAL);
    }

    // return the number of samples decoded
    return(iCount);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipPCMReset

    \Description
        Resets codec state.

    \Input *pState      - pointer to decode state

    \Version 08/04/2004 (jbrookes)
*/
/*************************************************************************************************F*/
static void _VoipPCMReset(VoipCodecRefT *pState)
{

}

/*F*************************************************************************************************/
/*!
    \Function _VoipPCMControl

    \Description
        Modifies parameters of the codec

    \Input *pCodecState - pointer to decode state
    \Input iControl     - control selector
    \Input iValue       - selector specific
    \Input iValue2      - selector specific
    \Input *pValue      - selector specific

    \Output
        int32_t         - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
                'plvl' - Set the output power level
        \endverbatim

    \Version 03/12/2008 (grouse)
*/
/*************************************************************************************************F*/
static int32_t _VoipPCMControl(VoipCodecRefT *pCodecState, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    VoipPCMStateT *pState = (VoipPCMStateT *)pCodecState;

    if (iControl == 'plvl')
    {
        pState->iOutputVolume = iValue;
        return(0);
    }
    NetPrintf(("voippcm: unhandled control selector '%C'\n", iControl));
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipPCMStatus

    \Description
        Get codec status

    \Input *pCodecState - pointer to decode state
    \Input iSelect      - status selector
    \Input iValue       - selector-specific
    \Input *pBuffer     - [out] storage for selector output
    \Input iBufSize     - size of output buffer

    \Output
        int32_t         - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'fsiz' - size of encoder output / decoder input in bytes (iValue=samples per frame)
        \endverbatim

    \Version 10/11/2011 (jbrookes)
*/
/*************************************************************************************************F*/
static int32_t _VoipPCMStatus(VoipCodecRefT *pCodecState, int32_t iSelect, int32_t iValue, void *pBuffer, int32_t iBufSize)
{
    VoipPCMStateT *pModuleState = (VoipPCMStateT *)pCodecState;

    // these options require module state
    if (pModuleState != NULL)
    {
        if (iSelect == 'fsiz')
        {
            return(iValue*2);
        }
    }
    NetPrintfVerbose((pModuleState->CodecState.iDebugLevel, 1, "voippcm: unhandled status selector '%C'\n", iSelect));
    return(-1);
}

