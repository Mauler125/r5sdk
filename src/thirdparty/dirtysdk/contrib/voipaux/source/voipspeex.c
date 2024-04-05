/*H*************************************************************************************************/
/*!
    \File voipspeex.c

    \Description
        PC Audio Encoder / Decoder using Speex

    \Copyright
        Copyright (c) Electronic Arts 2007. ALL RIGHTS RESERVED.

    \Version 1.0 04/02/2007 (cadam) First version
*/
/*************************************************************************************************H*/

/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "voipspeex.h"

#include <speex/speex.h>
#include <speex/speex_preprocess.h>

/*** Defines ***************************************************************************/

#define VOIPSPEEX_ENABLED       (TRUE) // enable speex codec

#ifndef VOIPSPEEX_PREPROCESS
#define VOIPSPEEX_PREPROCESS    (0)   // Speex preprocessor
#endif

// encode/decode information
#define VOIPSPEEX_FRAME_SIZE    (160)
#define VOIPSPEEX_MAX_BYTES     (200)
#define VOIPSPEEX_COMP_SIZE     (38)

// speex settings
#define VOIPSPEEX_COMPLEXITY    (1)
#define VOIPSPEEX_QUALITY       (8)
#define VOIPSPEEX_PERCEPTUAL    (1)

// speex preprocessor settings
#define VOIPSPEEX_DENOISE       (1)
#define VOIPSPEEX_AGC           (1)
#define VOIPSPEEX_PROB_START    (30)
#define VOIPSPEEX_PROB_CONTINUE (7)

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! voipspeex decoder ref
typedef struct VoipSpeexDecoderT
{
    int32_t         iChannel;
    void            *pDecodeState;
} VoipSpeexDecoderT;

//! voipspeex module state
typedef struct VoipSpeexStateT
{
    VoipCodecRefT CodecState;

    // module memory group
    int32_t iMemGroup;
    void *pMemGroupUserData;

    int32_t iNumChannels;

    int32_t iOutputVolume;

    SpeexBits sEncodeBits;
    SpeexBits *pDecodeBits;

    void *pEncodeState;
    VoipSpeexDecoderT *pDecoders;
    SpeexPreprocessState *pPreprocessor;

    #if DIRTYCODE_LOGGING
    int32_t iDebugLevel;
    #endif
} VoipSpeexStateT;


/*** Function Prototypes ***************************************************************/

#if VOIPSPEEX_ENABLED
static VoipCodecRefT *_VoipSpeexCreate(int32_t iDecodeChannels);
static void _VoipSpeexDestroy(VoipCodecRefT *pState);
static int32_t _VoipSpeexEncodeBlock(VoipCodecRefT *pState, uint8_t *pOut, const int16_t *pInp, int32_t iNumSamples);
static int32_t _VoipSpeexDecodeBlock(VoipCodecRefT *pState, int32_t *pOut, const uint8_t *pInp, int32_t iInputBytes, int32_t iChannel);
static void _VoipSpeexReset(VoipCodecRefT *pState);
static int32_t _VoipSpeexControl(VoipCodecRefT *pState, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);
static int32_t _VoipSpeexStatus(VoipCodecRefT *pState, int32_t iSelect, int32_t iValue, void *pBuffer, int32_t iBufSize);
#else
static VoipCodecRefT *_VoipSpeexCreate(int32_t iDecodeChannels) { return NULL; }
static void _VoipSpeexDestroy(VoipCodecRefT *pState) { }
static int32_t _VoipSpeexEncodeBlock(VoipCodecRefT *pState, uint8_t *pOut, const int16_t *pInp, int32_t iNumSamples) { return -1; }
static int32_t _VoipSpeexDecodeBlock(VoipCodecRefT *pState, int32_t *pOut, const uint8_t *pInp, int32_t iInputBytes, int32_t iChannel) { return -1; }
static void _VoipSpeexReset(VoipCodecRefT *pState) { }
static int32_t _VoipSpeexControl(VoipCodecRefT *pState, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue) { return -1; }
static int32_t _VoipSpeexStatus(VoipCodecRefT *pState, int32_t iSelect, int32_t iValue, void *pBuffer, int32_t iBufSize) { return -1; }
#endif

/*** Variables *************************************************************************/

//! voipspeex codec block
const VoipCodecDefT VoipSpeex_CodecDef =
{
    _VoipSpeexCreate,
    _VoipSpeexDestroy,
    _VoipSpeexEncodeBlock,
    _VoipSpeexDecodeBlock,
    _VoipSpeexReset,
    _VoipSpeexControl,
    _VoipSpeexStatus,
};

#if VOIPSPEEX_ENABLED

/*** Private Functions *****************************************************************/

/*F*************************************************************************************************/
/*!
    \Function _VoipSpeexSetControls

    \Description
        Sets all the controls on the codec

    \Input pCodecState  - codec state

    \Version 01/12/08 (grouse)
*/
/*************************************************************************************************F*/
static void _VoipSpeexSetControls( VoipCodecRefT *pCodecState )
{
    VoipSpeexStateT *pState = (VoipSpeexStateT *)pCodecState;

    int32_t iChannel;

    int32_t iComplexity = VOIPSPEEX_COMPLEXITY;
    int32_t iQuality = VOIPSPEEX_QUALITY;
    int32_t iPerceptual = VOIPSPEEX_PERCEPTUAL;

#if VOIPSPEEX_PREPROCESS
    int32_t iDenoise = VOIPSPEEX_DENOISE;
    int32_t iAGC = VOIPSPEEX_AGC;
    int32_t iProbStart = VOIPSPEEX_PROB_START;
    int32_t iProbContinue = VOIPSPEEX_PROB_CONTINUE;
#endif

    int32_t iReturnVal = 0;

    iReturnVal = speex_encoder_ctl(pState->pEncodeState, SPEEX_SET_COMPLEXITY, &iComplexity);
    if( iReturnVal != 0 )
    {
        NetPrintf(("voipspeex: error setting encoder control SPEEX_SET_COMPLEXITY with code %d\n", iReturnVal));
    }

    iReturnVal = speex_encoder_ctl(pState->pEncodeState, SPEEX_SET_QUALITY, &iQuality);
    if( iReturnVal != 0 )
    {
        NetPrintf(("voipspeex: error setting encoder control SPEEX_SET_QUALITY with code %d\n", iReturnVal));
    }

    for (iChannel=0; iChannel < pState->CodecState.iDecodeChannels; iChannel++)
    {
        iReturnVal = speex_decoder_ctl(pState->pDecoders[iChannel].pDecodeState, SPEEX_SET_ENH, &iPerceptual);
        if( iReturnVal != 0 )
        {
            NetPrintf(("voipspeex: error setting decoder control SPEEX_SET_ENH for decoder %d with code %d\n", iChannel, iReturnVal));
        }
    }

#if VOIPSPEEX_PREPROCESS
    iReturnVal = speex_preprocess_ctl(pState->pPreprocessor, SPEEX_PREPROCESS_SET_DENOISE, &iDenoise);
    if( iReturnVal != 0 )
    {
        NetPrintf(("voipspeex: error setting preprocessor control SPEEX_PREPROCESS_SET_DENOISE with code %d\n", iReturnVal));
    }

    iReturnVal = speex_preprocess_ctl(pState->pPreprocessor, SPEEX_PREPROCESS_SET_AGC, &iAGC);
    if( iReturnVal != 0 )
    {
        NetPrintf(("voipspeex: error setting preprocessor control SPEEX_PREPROCESS_SET_AGC with code %d\n", iReturnVal));
    }

    iReturnVal = speex_preprocess_ctl(pState->pPreprocessor, SPEEX_PREPROCESS_SET_PROB_START, &iProbStart);
    if( iReturnVal != 0 )
    {
        NetPrintf(("voipspeex: error re-setting preprocessor control SPEEX_PREPROCESS_SET_PROB_START with code %d\n", iReturnVal));
    }

    iReturnVal = speex_preprocess_ctl(pState->pPreprocessor, SPEEX_PREPROCESS_SET_PROB_CONTINUE, &iProbContinue);
    if( iReturnVal != 0 )
    {
        NetPrintf(("voipspeex: error re-setting preprocessor control SPEEX_PREPROCESS_SET_PROB_CONTINUE with code %d\n", iReturnVal));
    }
#endif
}

/*F*************************************************************************************************/
/*!
    \Function _VoipSpeexCreate

    \Description
        Create a Speex codec state.

    \Input iDecodeChannels  - number of decoder channels

    \Output
        VoipCodecStateT *   - pointer to speex codec state

    \Version 04/02/2007 (cadam)
*/
/*************************************************************************************************F*/
static VoipCodecRefT *_VoipSpeexCreate(int32_t iDecodeChannels)
{
    VoipSpeexStateT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    int32_t iChannel;

    // Query mem group id
    iMemGroup = VoipStatus(NULL, 'mgrp', 0, NULL, 0);

    // Query mem group user data
    VoipStatus(NULL, 'mgud', 0, &pMemGroupUserData, sizeof(pMemGroupUserData));

    // allocate memory for the state structure and initialize the variables
    pState = (VoipSpeexStateT *)DirtyMemAlloc(sizeof(VoipSpeexStateT), VOIP_MEMID, iMemGroup, pMemGroupUserData);
    ds_memclr(pState, sizeof(VoipSpeexStateT));
    NetPrintfVerbose((pState->iDebugLevel, 0, "voipspeex: allocated module reference at %p\n", pState));

    // initialize the state variables
    pState->CodecState.pCodecDef = &VoipSpeex_CodecDef;
    pState->CodecState.iDecodeChannels = iDecodeChannels;
    pState->CodecState.bVadEnabled = TRUE;
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->iNumChannels = iDecodeChannels;

    // set the power threshold and output level
    pState->iOutputVolume = 1 << VOIP_CODEC_OUTPUT_FRACTIONAL;

    // initialize the speex bits
    speex_bits_init(&pState->sEncodeBits);

    // create and initialize the encoder
    pState->pEncodeState = speex_encoder_init(&speex_nb_mode);
    if (pState->pEncodeState == NULL)
    {
        NetPrintf(("voipspeex: failed to create encoder\n"));
        speex_bits_destroy(&pState->sEncodeBits);
        DirtyMemFree(pState, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(NULL);
    }

    // allocate an array of pointers to hold pointers to one decoder per channel
    pState->pDecoders = DirtyMemAlloc(sizeof(VoipSpeexDecoderT) * iDecodeChannels, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    NetPrintfVerbose((pState->iDebugLevel, 0, "voipspeex: decoder array allocated %d bytes at %p\n", sizeof(VoipSpeexDecoderT) * iDecodeChannels, pState->pDecoders));
    pState->pDecodeBits = DirtyMemAlloc(sizeof(SpeexBits) * iDecodeChannels, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    NetPrintfVerbose((pState->iDebugLevel, 0, "voipspeex: decode bits array allocated %d bytes at %p\n", sizeof(SpeexBits) * iDecodeChannels, pState->pDecodeBits));

    // initialize the decoders
    for (iChannel = 0; iChannel < pState->iNumChannels; iChannel++)
    {
        pState->pDecoders[iChannel].iChannel = iChannel;
        pState->pDecoders[iChannel].pDecodeState = speex_decoder_init(&speex_nb_mode);

        if (pState->pDecoders[iChannel].pDecodeState == NULL)
        {
            // we only need to destroy the ones before the failure
            pState->iNumChannels = iChannel;

            NetPrintf(("voipspeex: failed to create decoder %d\n", iChannel));
            for (iChannel = 0; iChannel < pState->iNumChannels; iChannel++)
            {
                speex_decoder_destroy(pState->pDecoders[iChannel].pDecodeState);
                speex_bits_destroy(&pState->pDecodeBits[iChannel]);
            }

            DirtyMemFree(pState->pDecoders, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            DirtyMemFree(pState->pDecodeBits, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            speex_encoder_destroy(pState->pEncodeState);
            DirtyMemFree(pState, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);

            return(NULL);
        }

        speex_bits_init(&pState->pDecodeBits[iChannel]);
    }

#if VOIPSPEEX_PREPROCESS
    pState->pPreprocessor = speex_preprocess_state_init(VOIPSPEEX_FRAME_SIZE, 8000);
    if (pState->pPreprocessor == NULL)
    {
        NetPrintf(("voipspeex: Failed to create pre-processor\n"));

        for (iChannel = 0; iChannel < pState->iNumChannels; iChannel++)
        {
            if (pState->pDecoders[iChannel].pDecodeState)
            {
                speex_decoder_destroy(pState->pDecoders[iChannel].pDecodeState);
                speex_bits_destroy(&pState->pDecodeBits[iChannel]);
            }
        }

        DirtyMemFree(pState->pDecoders, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        DirtyMemFree(pState->pDecodeBits, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        speex_encoder_destroy(pState->pEncodeState);
        DirtyMemFree(pState, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);

        return(NULL);
    }
#else
    pState->pPreprocessor = NULL;
#endif

    _VoipSpeexSetControls(&pState->CodecState);

    NetPrintfVerbose((pState->iDebugLevel, 0, "voipspeex: returning %p\n", &pState->CodecState));
    return(&pState->CodecState);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipSpeexDestroy

    \Description
        Destroy a Speex codec state.

    \Input *pState          - state to destroy

    \Version 04/02/2007 (cadam)
*/
/*************************************************************************************************F*/
static void _VoipSpeexDestroy(VoipCodecRefT *pCodecState)
{
    int32_t i;

    VoipSpeexStateT *pState = (VoipSpeexStateT *)pCodecState;

#if VOIPSPEEX_PREPROCESS
    speex_preprocess_state_destroy(pState->pPreprocessor);
#endif

    // destroy the decoders then free the memory that was allocated for them
    for (i = 0; i < pState->iNumChannels; i++)
    {
        speex_decoder_destroy(pState->pDecoders[i].pDecodeState);
        speex_bits_destroy(&pState->pDecodeBits[i]);
    }

    // free the array that held the decoders
    DirtyMemFree(pState->pDecoders, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);

    // free the array that held the decode bits
    DirtyMemFree(pState->pDecodeBits, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);

    // destroy the encoder
    speex_encoder_destroy(pState->pEncodeState);

    // destroy the SpeexBits
    speex_bits_destroy(&pState->sEncodeBits);

    // free the state
    DirtyMemFree(pState, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipSpeexEncodeBlock

    \Description
        Encode a buffer 16-bit audio samples using the Speex encoder.

    \Input *pState          - pointer to encode state
    \Input *pOut            - pointer to output buffer
    \Input *pInp            - pointer to input buffer
    \Input iNumSamples      - number of samples to encode

    \Output
        int32_t             - size of compressed data in bytes

    \Version 04/05/2007 (cadam)
*/
/*************************************************************************************************F*/
static int32_t _VoipSpeexEncodeBlock(VoipCodecRefT *pCodecState, unsigned char *pOut, const int16_t *pInp, int32_t iNumSamples)
{
    int32_t iFrame, iNumFrames, iSample;
    VoipSpeexStateT *pState = (VoipSpeexStateT *)pCodecState;
    unsigned char *pOutput = pOut;
    const int16_t *pInput = pInp;
    spx_int16_t iFrameBuffer[VOIPSPEEX_FRAME_SIZE];
    int32_t iNumBytes, iNumBytesEncoded = 0;

    iNumFrames = iNumSamples/VOIPSPEEX_FRAME_SIZE;

    NetPrintfVerbose((pState->iDebugLevel, 2, "voipspeex: encoding %d samples (%d frames)\n", iNumSamples, iNumFrames));

    if ((iNumSamples % VOIPSPEEX_FRAME_SIZE) != 0)
    {
        NetPrintf(("voipspeex: error - speex encoder can only encode multiples of %d samples.\n", VOIPSPEEX_FRAME_SIZE));
        return(0);
    }

    for (iFrame = 0; iFrame < iNumFrames; iFrame++)
    {
        ds_memclr(iFrameBuffer, sizeof(iFrameBuffer));

        // convert the 16 bit input values to a float buffer for Speex
        for (iSample = 0; iSample < VOIPSPEEX_FRAME_SIZE; iSample++)
        {
            iFrameBuffer[iSample] = pInput[iSample];
        }

        // reset the SpeexBits
        speex_bits_reset(&pState->sEncodeBits);

#if VOIPSPEEX_PREPROCESS
        speex_preprocess(pState->pPreprocessor, iFrameBuffer, NULL);
#endif

        // encode the frame
        speex_encode_int(pState->pEncodeState, iFrameBuffer, &pState->sEncodeBits);

        // write the result to the output and get the number of bytes written
        iNumBytes = speex_bits_write(&pState->sEncodeBits, (char *)pOutput, VOIPSPEEX_MAX_BYTES);

        // increment the pointers and the total number of bytes written
        pInput += VOIPSPEEX_FRAME_SIZE;
        pOutput += iNumBytes;
        iNumBytesEncoded += iNumBytes;
    }

    return(iNumBytesEncoded);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipSpeexDecodeBlock

    \Description
        Decode Speex-encoded input to 16-bit linear PCM samples, and accumulate in the given output buffer.

    \Input *pState          - pointer to decode state
    \Input *pOut            - pointer to output buffer
    \Input *pInp            - pointer to input buffer
    \Input iInputBytes      - size of input data
    \Input iChannel         - the decode channel for which we are decoding data

    \Output
        int32_t             - number of samples decoded

    \Version 04/05/2007 (cadam)
*/
/*************************************************************************************************F*/
static int32_t _VoipSpeexDecodeBlock(VoipCodecRefT *pCodecState, int32_t *pOut, const unsigned char *pInp, int32_t iInputBytes, int32_t iChannel)
{
    int32_t iFrame, iFrameSize, iNumFrames, iSample;
    VoipSpeexStateT *pState = (VoipSpeexStateT *)pCodecState;
    int32_t *pOutput = pOut;
    unsigned char *pInput = (unsigned char *)pInp;
    spx_int16_t iFrameBuffer[VOIPSPEEX_FRAME_SIZE];

    iFrameSize = VOIPSPEEX_COMP_SIZE;
    iNumFrames = iInputBytes/iFrameSize;
    if (iInputBytes == 0)
    {
        NetPrintf(("voipspeex: no data to decode\n"));
        return(0);
    }
    if ((iInputBytes % iFrameSize) != 0)
    {
        NetPrintf(("voipspeex: speex decoder can only decode multiples of %d bytes (%d submitted)\n", iFrameSize, iInputBytes));
        return(0);
    }

    NetPrintfVerbose((pState->iDebugLevel, 2, "voipspeex: decoding %d bytes   (%d frames)\n", iInputBytes, iNumFrames));

    for (iFrame = 0; iFrame < iNumFrames; iFrame++)
    {
        ds_memclr(iFrameBuffer, sizeof(iFrameBuffer));

        // reset the SpeexBits
        speex_bits_reset(&pState->pDecodeBits[iChannel]);

        // read the bits from the input
        speex_bits_read_from(&pState->pDecodeBits[iChannel], (char *)pInput, iFrameSize);

        // decode the bits
        speex_decode_int(pState->pDecoders[iChannel].pDecodeState, &pState->pDecodeBits[iChannel], iFrameBuffer);

        // convert the float buffer to an int32_t array for Speex
        for (iSample = 0; iSample < VOIPSPEEX_FRAME_SIZE; iSample++)
        {
            pOutput[iSample] += (((int32_t)iFrameBuffer[iSample] * pState->iOutputVolume) >> VOIP_CODEC_OUTPUT_FRACTIONAL);
        }

        // increment the pointers and the total number of bytes written
        pInput += iFrameSize;
        pOutput += VOIPSPEEX_FRAME_SIZE;
    }

    return(iNumFrames * VOIPSPEEX_FRAME_SIZE);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipSpeexReset

    \Description
        Resets codec state.

    \Input *pState          - pointer to decode state

    \Version 04/05/2007 (cadam)
*/
/*************************************************************************************************F*/
static void _VoipSpeexReset(VoipCodecRefT *pCodecState)
{
    int32_t i;

    VoipSpeexStateT *pState = (VoipSpeexStateT *)pCodecState;

    // reset the SpeexBits
    speex_bits_reset(&pState->sEncodeBits);

    // reset the encoder
    speex_encoder_ctl(pState->pEncodeState, SPEEX_RESET_STATE, NULL);

    // reset the decoders
    for (i = 0; i < pState->iNumChannels; i++)
    {
        speex_decoder_ctl(pState->pDecoders[i].pDecodeState, SPEEX_RESET_STATE, NULL);
    }

    _VoipSpeexSetControls(&pState->CodecState);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipSpeexControl

    \Description
        Modifies parameters of the codec

    \Input *pState  - pointer to decode state
    \Input iControl - control selector
    \Input iValue   - selector specific
    \Input iValue2  - selector specific
    \Input *pValue  - selector specific

    \Output
        int32_t         - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'plvl' - Set the output power level
            'spam' - Set debug output verbosity (debug only)
        \endverbatim

    \Version 03/12/2008 (grouse)
*/
/*************************************************************************************************F*/
static int32_t _VoipSpeexControl(VoipCodecRefT *pCodecState, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    VoipSpeexStateT *pState = (VoipSpeexStateT *)pCodecState;

    if ((iControl == 'plvl') && (pState != NULL))
    {
        pState->iOutputVolume = iValue;
        return(0);
    }
    #if DIRTYCODE_LOGGING
    if ((iControl == 'spam') && (pState != NULL))
    {
        pState->iDebugLevel = iValue;
        return(0);
    }
    #endif
    NetPrintf(("voipspeex: unhandled control selector '%C'\n", iControl));
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipSpeexStatus

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
            'fsmp' - frame sample size for current codec
        \endverbatim

    \Version 10/11/2011 (jbrookes)
*/
/*************************************************************************************************F*/
static int32_t _VoipSpeexStatus(VoipCodecRefT *pCodecState, int32_t iSelect, int32_t iValue, void *pBuffer, int32_t iBufSize)
{
    VoipSpeexStateT *pModuleState = (VoipSpeexStateT *)pCodecState;

    // these options require module state
    if (pModuleState != NULL)
    {
        if (iSelect == 'fsiz')
        {
            return((iValue/VOIPSPEEX_FRAME_SIZE) * VOIPSPEEX_COMP_SIZE);
        }
        if (iSelect == 'fsmp')
        {
            return(VOIPSPEEX_FRAME_SIZE);
        }
    }
    NetPrintfVerbose((pModuleState->iDebugLevel, 1, "voipspeex: unhandled status selector '%C'\n", iSelect));
    return(-1);
}

// speex function overrides
void _speex_fatal(const char *str, const char *file, int line)
{
   NetPrintf(("Fatal (internal) error in %s, line %d: %s\n", file, line, str ));
}

void speex_warning(const char *str)
{
   NetPrintf(("libspeex: warning: %s\n", str));
}

void speex_warning_int(const char *str, int val)
{
   NetPrintf(("libspeex: warning: %s %d\n", str, val));
}

void speex_notify(const char *str)
{
    NetPrintf(("libspeex: notification: %s\n", str));
}

#endif // VOIPSPEEX_ENABLED

