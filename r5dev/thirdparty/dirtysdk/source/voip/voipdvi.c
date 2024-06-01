/*H********************************************************************************/
/*!
    \File voipdvi.c

    \Description
        Table based 16:3 ADPCM compression originally based off EAC SIMEX code,
        modified by Greg Schaefer.

    \Copyright
        Copyright (c) Electronic Arts 2003-2004. ALL RIGHTS RESERVED.

    \Version 1.0 11/01/2002 (ischmidt)  First version (based on SIMEX by Dave Mercier)
    \Version 2.0 05/13/2003 (gschaefer) Rewrite to 16:3 (from 16:4)
    \Version 3.0 08/04/2004 (jbrookes)  Modifications for multi-codec support.
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
#include "voipdvi.h"

/*** Defines **********************************************************************/

#define DIV_RANGE   (4)
#define MAX_RANGE   (8191)

//! encode/decode information
#define VOIPDVI_FRAME_SIZE    (8)

/*** Macros ***********************************************************************/

#define CLIP(x,lo,hi)   \
    if (x < lo) \
    { \
        x = lo; \
    } \
    else if (x > hi)    \
    { \
        x = hi; \
    }


/*** Type Definitions *************************************************************/

//! DVI compression state
typedef struct DVICompStateT
{
    int16_t iEstimate;
    int16_t iStepIndex;
} DVICompStateT;

typedef struct VoipDVIStateT
{
    VoipCodecRefT CodecState;

    // codec-specifc data goes here
    DVICompStateT EncodeState;
    int32_t iOutputVolume;
} VoipDVIStateT;


/*** Function Prototypes **********************************************************/

static VoipCodecRefT *_VoipDVICreate(int32_t iDecoderChannels);
static void _VoipDVIDestroy(VoipCodecRefT *pCodecState);
static int32_t _VoipDVIEncodeBlock3(VoipCodecRefT *pCodecState, uint8_t *pOut, const int16_t *pInp, int32_t iNumSamples);
static int32_t _VoipDVIDecodeBlock3(VoipCodecRefT *pCodecState, int32_t *pOut, const uint8_t *pInp, int32_t iInputBytes, int32_t iChannel);
static void _VoipDVIReset(VoipCodecRefT *pCodecState);
static int32_t _VoipDVIControl(VoipCodecRefT *pCodecState, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);
static int32_t _VoipDVIStatus(VoipCodecRefT *pCodecState, int32_t iSelect, int32_t iValue, void *pBuffer, int32_t iBufSize);

/*** Variables ********************************************************************/

// Public variables

//! public DVI codec definition
const VoipCodecDefT VoipDVI_CodecDef =
{
    _VoipDVICreate,
    _VoipDVIDestroy,
    _VoipDVIEncodeBlock3,
    _VoipDVIDecodeBlock3,
    _VoipDVIReset,
    _VoipDVIControl,
    _VoipDVIStatus,
};

// Private variables

static int16_t _StepLimit3 = 0;
static int16_t _StepTable3[64];
static int16_t _StepIndex3[4] = { -2, -1, 2, 5 };


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _DeltaEncode3

    \Description
        Encode the prediction error into a 3-bit value

    \Input iStep    - current step size
    \Input iDelta   - error delta to encode

    \Output
        int32_t     - Three bit encoded value

    \Version 05/13/2003 (gschaefer)
*/
/********************************************************************************F*/
static __inline int32_t _DeltaEncode3(int32_t iStep, int32_t iDelta)
{
    int32_t iEncode = 0;

    // check for negative direction
    if (iDelta < 0)
    {
        iEncode |= 4;
        iDelta = -iDelta;
    }

    // primary delta shift
    if (iDelta >= iStep)
    {
        iEncode |= 2;
        iDelta -= iStep;
    }

    // secondary delta shift
    iStep >>= 1;
    if (iDelta >= iStep)
    {
        iEncode |= 1;
        iDelta -= iStep;
    }

    // return encoded value
    return(iEncode);
}

/*F********************************************************************************/
/*!
    \Function _DeltaDecode3

    \Description
        Decode 3-bit value into prediction error delta

    \Input iStep    - current step size
    \Input iEncode  - encoded 3-bit value

    \Output
        int32_t     - Prediction error delta

    \Version 105/13/2003 (gschaefer)
*/
/********************************************************************************F*/
static __inline int32_t _DeltaDecode3(int32_t iStep, int32_t iEncode)
{
    int32_t iDelta = 0;

    // primary delta shift
    if (iEncode & 2)
    {
        iDelta += iStep;
    }

    // secondary delta shift
    iStep >>= 1;
    if (iEncode & 1)
    {
        iDelta += iStep;
    }

    // always add final fraction to reduce truncation error
    iDelta += (iStep >> 1);

    // handle negative shift
    if (iEncode & 4)
    {
        iDelta = -iDelta;
    }
    return(iDelta);
}

/*F********************************************************************************/
/*!
    \Function _VoipDVISetupBlock3

    \Description
        Dynamically build the initial quantization table

    \Version 05/13/2003 (gschaefer)
*/
/********************************************************************************F*/
static void _VoipDVISetupBlock3(void)
{
    int32_t iStep;
    int32_t iLast = 0;

    // reset number of table entries
    _StepLimit3 = 0;

    // use fixed point math to allow step = step * 1.2
    for (iStep = 300; iStep < MAX_RANGE*100; iStep = (iStep * 120) / 100)
    {
        // skip non integer changes
        if ((iStep/100) != iLast)
        {
            iLast = iStep/100;
            _StepTable3[_StepLimit3++] = iLast;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipDVICreate

    \Description
        Create Voip DVI state

    \Input iDecodeChannels  - number of channels to support decoding for

    \Output
        VoipCodecRefT *     - pointer to new codec state, or NULL if failure

    \Version 08/05/2004 (jbrookes)
*/
/********************************************************************************F*/
static VoipCodecRefT *_VoipDVICreate(int32_t iDecodeChannels)
{
    VoipDVIStateT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // set up DVI tables
    _VoipDVISetupBlock3();

    // allocate and clear state
    VoipCommonMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    if ((pState = (VoipDVIStateT *) DirtyMemAlloc (sizeof(*pState), VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipdvi: unable to allocate state\n"));
        return(NULL);
    }
    ds_memclr(pState, sizeof(*pState));

    // set up codec state
    pState->CodecState.pCodecDef = &VoipDVI_CodecDef;
    pState->CodecState.iDecodeChannels = iDecodeChannels;

    // set the output level
    pState->iOutputVolume = 1 << VOIP_CODEC_OUTPUT_FRACTIONAL;

    // return generic state ref to caller
    return(&pState->CodecState);
}

/*F********************************************************************************/
/*!
    \Function _VoipDVIDestroy

    \Description
        Destroy given Voip DVI codec state

    \Input *pCodecState  - pointer to state to destroy

    \Version 08/05/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipDVIDestroy(VoipCodecRefT *pCodecState)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    VoipCommonMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    DirtyMemFree(pCodecState, VOIP_MEMID, iMemGroup, pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function _VoipDVIEncodeBlock3

    \Description
        Encode a 16-bit linear PCM sample into a 3-bit value using ADPCM.

    \Input *pCodecState - module state
    \Input *pOut        - packed output buffer
    \Input *pInp        - 16-bit mono sample buffer pointer
    \Input iNumSamples  - number of samples to compress

    \Output
        int32_t         - size of compressed data in bytes

    \Version 05/13/2003 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _VoipDVIEncodeBlock3(VoipCodecRefT *pCodecState, uint8_t *pOut, const int16_t *pInp, int32_t iNumSamples)
{
    int32_t iCount;
    int32_t iIndex;
    int32_t iDelta;
    int32_t iSample;
    int32_t iEstimate;
    int32_t iStepIndex;
    unsigned char Stage[8];
    unsigned char *pBeg = pOut;
    DVICompStateT *pCompState = &((VoipDVIStateT *)pCodecState)->EncodeState;

    // DVI encoder requires input samples to be a multiple of VOIPDVI_FRAME_SIZE
    if ((iNumSamples % VOIPDVI_FRAME_SIZE) != 0)
    {
        NetPrintf(("voipdvi: error - dvi encoder can only encode multiples of %d samples (%d submitted).\n", VOIPDVI_FRAME_SIZE, iNumSamples));
        return(0);
    }

    // save initial state to output buffer
    ds_memcpy(pOut, pCompState, sizeof(*pCompState));
    pOut += sizeof(*pCompState);

    // fetch initial state
    iEstimate = pCompState->iEstimate;
    iStepIndex = pCompState->iStepIndex;

    for (iCount = 0; iCount < iNumSamples; ++iCount)
    {
        // calc delta from previous estimate
        iDelta = (*pInp++ / DIV_RANGE) - iEstimate;
        CLIP(iDelta, -MAX_RANGE, MAX_RANGE);
        // encode the sample
        iSample = _DeltaEncode3(_StepTable3[iStepIndex], iDelta);
        // adjust the estimate based on decoded sample (since this is what decoded will use)
        iEstimate += _DeltaDecode3(_StepTable3[iStepIndex], iSample);
        CLIP(iEstimate, -MAX_RANGE, MAX_RANGE);

        #if 0 //$$ debug diagnostic to help tune/debug algorithm
        NetPrintf(("sample #%d: orig=%5d comp=%5d err=%4d delta=%4d idx=%2d step=%5d code=%02x\n",
            iCount, pInp[-1], iEstimate, pInp[-1]-iEstimate, iDelta,
            iStepIndex, _StepTable3[iStepIndex], iSample));
        #endif

        // adjust the step size for next iteration
        iStepIndex += _StepIndex3[iSample & 3];
        CLIP(iStepIndex, 0, _StepLimit3-1);
        // save into staging buffer
        iIndex = iCount&7;
        Stage[iIndex] = iSample;
        // see if we need to store the output
        if (iIndex == 7)
        {
            pOut[0] = (Stage[0]<<0) | (Stage[1]<<3) | (Stage[2]<<6);
            pOut[1] = (Stage[2]>>2) | (Stage[3]<<1) | (Stage[4]<<4) | (Stage[5]<<7);
            pOut[2] = (Stage[5]>>1) | (Stage[6]<<2) | (Stage[7]<<5);
            pOut += 3;
        }
    }

    // update with current state
    pCompState->iEstimate = iEstimate;
    pCompState->iStepIndex = iStepIndex;

    // return the length
    return(pOut - pBeg);
}

/*F********************************************************************************/
/*!
    \Function _VoipDVIDecodeBlock3

    \Description
        Decode 3-bit ADPCM packed data and accumulate into a 32-bit buffer of 16-bit linear PCM
        samples.

    \Input *pCodecState - module state
    \Input *pOut        - 32-bit linear PCM sample accumulation buffer
    \Input *pInp        - packed data from encoder
    \Input iInputBytes  - to be completed
    \Input iChannel     - ignored

    \Output
        int32_t         - number of samples decoded

    \Version 05/13/2003 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _VoipDVIDecodeBlock3(VoipCodecRefT *pCodecState, int32_t *pOut, const unsigned char *pInp, int32_t iInputBytes, int32_t iChannel)
{
    VoipDVIStateT *pState = (VoipDVIStateT *)pCodecState;
    int32_t iCount, iNumSamples;
    int32_t iIndex;
    int32_t iInput;
    int32_t iEstimate;
    int32_t iStepIndex;
    int32_t *pBeg = pOut;
    unsigned char Stage[VOIPDVI_FRAME_SIZE];
    DVICompStateT DecodeState;

    // first four bytes of input are DVI state data
    ds_memcpy(&DecodeState, pInp, sizeof(DecodeState));
    pInp += sizeof(DecodeState);
    iInputBytes -= sizeof(DecodeState);

    if ((iInputBytes % 3) != 0)
    {
        NetPrintf(("voipdvi: dvi decoder can only decode multiples of 3 bytes (%d submitted)\n", iInputBytes));
        return(0);
    }

    // read state
    iEstimate = DecodeState.iEstimate;
    iStepIndex = DecodeState.iStepIndex;

    // calculate number of output samples based on the input data size
    iNumSamples = (iInputBytes * VOIPDVI_FRAME_SIZE)/3;

    // loop based on decoded data size
    for (iCount = 0; iCount < iNumSamples; ++iCount)
    {
        // see if we need to fetch data
        iIndex = iCount & 7;
        if (iIndex == 0)
        {
            Stage[0] = pInp[0];
            Stage[1] = pInp[0]>>3;
            Stage[2] = (pInp[0]>>6) | (pInp[1]<<2);
            Stage[3] = pInp[1]>>1;
            Stage[4] = pInp[1]>>4;
            Stage[5] = (pInp[1]>>7) | (pInp[2]<<1);
            Stage[6] = pInp[2]>>2;
            Stage[7] = pInp[2]>>5;
            pInp += 3;
        }

        // get input byte
        iInput = (Stage[iIndex] & 7);
        // decode to delta and add to previous estimate
        iEstimate += _DeltaDecode3(_StepTable3[iStepIndex], iInput);
        CLIP(iEstimate, -MAX_RANGE, MAX_RANGE);
        // update the index based on delta direction
        iStepIndex += _StepIndex3[iInput & 3];
        CLIP(iStepIndex, 0, _StepLimit3-1);
        // scale the volume using the output level and save to output buffer
        *pOut++ += ((iEstimate * pState->iOutputVolume) >> VOIP_CODEC_OUTPUT_FRACTIONAL) * DIV_RANGE;
    }

    // return the length
    return(pOut - pBeg);
}

/*F********************************************************************************/
/*!
    \Function _VoipDVIReset

    \Description
        Reset DVI Encoder state.

    \Input *pCodecState  - pointer to DVI module

    \Version 08/05/2004
*/
/********************************************************************************F*/
static void _VoipDVIReset(VoipCodecRefT *pCodecState)
{
    VoipDVIStateT *pState = (VoipDVIStateT *)pCodecState;
    ds_memclr(&pState->EncodeState, sizeof(pState->EncodeState));
}

/*F*************************************************************************************************/
/*!
    \Function _VoipDVIControl

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
static int32_t _VoipDVIControl(VoipCodecRefT *pCodecState, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    VoipDVIStateT *pState = (VoipDVIStateT *)pCodecState;

    if (iControl == 'plvl')
    {
        pState->iOutputVolume = iValue;
        return(0);
    }

    NetPrintf(("voipdvi: unhandled control selector '%C'\n", iControl));
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipDVIStatus

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
static int32_t _VoipDVIStatus(VoipCodecRefT *pCodecState, int32_t iSelect, int32_t iValue, void *pBuffer, int32_t iBufSize)
{
    VoipDVIStateT *pState = (VoipDVIStateT *)pCodecState;

    // these options require module state
    if (pState != NULL)
    {
        if (iSelect == 'fsiz')
        {
            // 3 bits per sample + 4 bytes of overhead per frame
            return(((iValue*3)/VOIPDVI_FRAME_SIZE) + sizeof(DVICompStateT));
        }
    }
    NetPrintfVerbose((pState->CodecState.iDebugLevel, 1, "voipdvi: unhandled status selector '%C'\n", iSelect));
    return(-1);
}

