/*H*************************************************************************************/
/*!
    \File voipopus.c

    \Description
        PC Audio Encoder / Decoder using Opus

    \Copyright
        Copyright (c) Electronic Arts 2017. ALL RIGHTS RESERVED.

    \Notes
        We depend on the Speex resampler for resampling (recommended by Opus)

    \Version 07/03/2017 (eesponda)
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#include "DirtySDK/dirtysock.h"

#include <opus.h>
#include <speex/speex_resampler.h>

#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/voip/voip.h"
#include "voipopus.h"

/*** Defines ***************************************************************************/

//! maximum duration frame
#define VOIPOPUS_MAX_FRAME (5760)

//! sampling rate we support in Hz
#if !defined(VOIPOPUS_DEFAULT_SAMPLING_RATE)
    #define VOIPOPUS_DEFAULT_SAMPLING_RATE (16000)
#endif

//! duration of the frame in milliseconds; 20ms
#define VOIPOPUS_FRAMEDURATION (20)

//! number of channels we support (mono or stereo)
#define VOIPOPUS_DEFAULT_CHANNELS      (1)

//! hard-coded maximum output used when encoding, this is taken from value in voippacket.h (VOIP_MAXMICRPKTSIZE)
#define VOIPOPUS_MAX_OUTPUT (1238)

//! speex resampler quality value (it is a number from 1 - 10) 
#define VOIPOPUS_RESAMPLER_QUALITY (3)

//! this much space will be needed to resample 20ms of audio
#define VOIPOPUS_RESAMPLE_BUFFER_SIZE ((VOIPOPUS_FRAMEDURATION * VOIPOPUS_DEFAULT_SAMPLING_RATE * sizeof(float)) / 1000)

/*** Macros ****************************************************************************/

//! calculate the sample rate based on number of samples
#define VOIPOPUS_GetSampleRate(uNumSamples) (((uNumSamples) * 1000) / VOIPOPUS_FRAMEDURATION)

/*** Type Definition *******************************************************************/

//! voipopus module state
typedef struct VoipOpusRefT
{
    VoipCodecRefT CodecState;

    int32_t iMemGroup;          //!< memgroup id
    void *pMemGroupUserData;    //!< memgroup userdata

    int32_t iVerbose;           //!< logging verbosity level
    int32_t iOutputVolume;      //!< volumn configuration

    uint32_t uSampleRateIn;     //!< what is the sample rate of the data being passed to encode
    uint32_t uResamplerRate;    //!< sample rate that our resampler is configured to. this allows us to switch the resampler on and off without reallocation
    uint8_t bInt32Input;        //!< is the input data coming as 32 bit integers
    uint8_t bFloatInput;        //!< is the input data coming as 32 bit floats
    uint8_t _pad[2];

    SpeexResamplerState *pSpeexResampler;   //!< resampler used if sample rate != VOIPOPUS_DEFAULT_SAMPLING_RATE
    OpusEncoder *pEncoder;      //!< opus encoder
    OpusDecoder *aDecoders[1];  //!< opus variable decoders (must come last!)
} VoipOpusRefT;

/*** Function Prototypes ***************************************************************/

static VoipCodecRefT *_VoipOpusCreate(int32_t iNumDecoders);
static void _VoipOpusDestroy(VoipCodecRefT *pState);
static int32_t _VoipOpusEncodeBlock(VoipCodecRefT *pState, uint8_t *pOutput, const int16_t *pInput, int32_t iNumSamples);
static int32_t _VoipOpusDecodeBlock(VoipCodecRefT *pState, int32_t *pOutput, const uint8_t *pInput, int32_t iInputBytes, int32_t iChannel);
static void _VoipOpusReset(VoipCodecRefT *pState);
static int32_t _VoipOpusControl(VoipCodecRefT *pState, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);
static int32_t _VoipOpusStatus(VoipCodecRefT *pState, int32_t iSelect, int32_t iValue, void *pBuffer, int32_t iBufSize);

/*** Variables *************************************************************************/

//! voipopus codec definition
const VoipCodecDefT VoipOpus_CodecDef =
{
    _VoipOpusCreate,
    _VoipOpusDestroy,
    _VoipOpusEncodeBlock,
    _VoipOpusDecodeBlock,
    _VoipOpusReset,
    _VoipOpusControl,
    _VoipOpusStatus
};

#if DIRTYSOCK_ERRORNAMES
//! errors returned from the opus api
static const DirtyErrT _VoipOpus_aErrList[] =
{
    DIRTYSOCK_ErrorName(OPUS_OK),                   //  0; No Error
    DIRTYSOCK_ErrorName(OPUS_BAD_ARG),              // -1; One of more invalid/out of range arguments
    DIRTYSOCK_ErrorName(OPUS_BUFFER_TOO_SMALL),     // -2; The mode struct passed is invalid
    DIRTYSOCK_ErrorName(OPUS_INTERNAL_ERROR),       // -3; An internal error was detected
    DIRTYSOCK_ErrorName(OPUS_INVALID_PACKET),       // -4; The compressed data passed is corrupted
    DIRTYSOCK_ErrorName(OPUS_UNIMPLEMENTED),        // -5; Invalid/unsupported request number
    DIRTYSOCK_ErrorName(OPUS_INVALID_STATE),        // -6; An encoder or decoder structure is invalid or already freed
    DIRTYSOCK_ErrorName(OPUS_ALLOC_FAIL),           // -7; Memory allocation has failed
    DIRTYSOCK_ListEnd()
};
#endif

/*** Private Functions *****************************************************************/

/*F*************************************************************************************/
/*!
    \Function _VoipOpusCreate

    \Description
        Create a Opus codec state.

    \Input iNumDecoders     - number of decoder channels

    \Output
        VoipCodecStateT *   - pointer to opus codec state

    \Version 07/03/2017 (eesponda)
*/
/*************************************************************************************F*/
static VoipCodecRefT *_VoipOpusCreate(int32_t iNumDecoders)
{
    VoipOpusRefT *pState;
    int32_t iResult, iMemGroup, iDecoder, iMemSize;
    void *pMemGroupUserData;

    // query memgroup information
    iMemGroup = VoipStatus(NULL, 'mgrp', 0, NULL, 0);
    VoipStatus(NULL, 'mgud', 0, &pMemGroupUserData, sizeof(pMemGroupUserData));

    // allocate and initialize module state
    iMemSize = sizeof(*pState) + (sizeof(OpusDecoder *) * (iNumDecoders - 1));
    if ((pState = (VoipOpusRefT *)DirtyMemAlloc(iMemSize, VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipopus: unable to allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pState, iMemSize);
    pState->CodecState.pCodecDef = &VoipOpus_CodecDef;
    pState->CodecState.iDecodeChannels = iNumDecoders;
    pState->CodecState.bVadEnabled = TRUE;
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->iVerbose = 2;
    pState->iOutputVolume = 1 << VOIP_CODEC_OUTPUT_FRACTIONAL;

    // allocate and initialize the encoder
    if ((iMemSize = opus_encoder_get_size(VOIPOPUS_DEFAULT_CHANNELS)) <= 0)
    {
        NetPrintf(("voipopus: unable to get encoder size for allocation\n"));
        _VoipOpusDestroy(&pState->CodecState);
        return(NULL);
    }
    if ((pState->pEncoder = (OpusEncoder *)DirtyMemAlloc(iMemSize, VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipopus: unable to allocate the encoder\n"));
        _VoipOpusDestroy(&pState->CodecState);
        return(NULL);
    }
    if ((iResult = opus_encoder_init(pState->pEncoder, VOIPOPUS_DEFAULT_SAMPLING_RATE, VOIPOPUS_DEFAULT_CHANNELS, OPUS_APPLICATION_VOIP)) != OPUS_OK)
    {
        NetPrintf(("voipopus: unable to initialize the encoder (err=%s)\n", DirtyErrGetNameList(iResult, _VoipOpus_aErrList)));

        _VoipOpusDestroy(&pState->CodecState);
        return(NULL);
    }
    // allocate and initialize the decoders
    if ((iMemSize = opus_decoder_get_size(VOIPOPUS_DEFAULT_CHANNELS)) <= 0)
    {
        NetPrintf(("voipopus: unable to get decoder size for allocation\n"));
        _VoipOpusDestroy(&pState->CodecState);
        return(NULL);
    }
    for (iDecoder = 0; iDecoder < iNumDecoders; iDecoder += 1)
    {
        if ((pState->aDecoders[iDecoder] = (OpusDecoder *)DirtyMemAlloc(iMemSize, VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
        {
            NetPrintf(("voipopus: unable to allocate the decoder\n"));
            _VoipOpusDestroy(&pState->CodecState);
            return(NULL);
        }
        if ((iResult = opus_decoder_init(pState->aDecoders[iDecoder], VOIPOPUS_DEFAULT_SAMPLING_RATE, VOIPOPUS_DEFAULT_CHANNELS)) != OPUS_OK)
        {
            NetPrintf(("voipopus: unable to initialize the decoder (err=%s)\n", DirtyErrGetNameList(iResult, _VoipOpus_aErrList)));
            _VoipOpusDestroy(&pState->CodecState);
            return(NULL);
        }
    }
    return(&pState->CodecState);
}

/*F*************************************************************************************/
/*!
    \Function _VoipOpusDestroy

    \Description
        Destroy the Opus codec state

    \Input *pState  - codec state

    \Version 07/03/2017 (eesponda)
*/
/*************************************************************************************F*/
static void _VoipOpusDestroy(VoipCodecRefT *pState)
{
    OpusDecoder **pDecoder;
    int32_t iDecoder;
    VoipOpusRefT *pOpus = (VoipOpusRefT *)pState;

    for (iDecoder = 0; iDecoder < pOpus->CodecState.iDecodeChannels; iDecoder += 1)
    {
        if ((pDecoder = &pOpus->aDecoders[iDecoder]) != NULL)
        {
            DirtyMemFree(*pDecoder, VOIP_MEMID, pOpus->iMemGroup, pOpus->pMemGroupUserData);
            *pDecoder = NULL;
        }
    }
    if (pOpus->pEncoder != NULL)
    {
        DirtyMemFree(pOpus->pEncoder, VOIP_MEMID, pOpus->iMemGroup, pOpus->pMemGroupUserData);
        pOpus->pEncoder = NULL;
    }

    if (pOpus->pSpeexResampler != NULL)
    {
        speex_resampler_destroy(pOpus->pSpeexResampler);
        pOpus->pSpeexResampler = NULL;
    }

    DirtyMemFree(pOpus, VOIP_MEMID, pOpus->iMemGroup, pOpus->pMemGroupUserData);
}

/*F*************************************************************************************/
/*!
    \Function _VoipOpusConvertInt32ToFloat

    \Description
        Convert int32_t samples into floats, within the same buffer passed in

    \Input *pState        - codec state
    \Input *pInSamples    - input int32_t samples
    \Input iNumSamples    - number of samples in pInSamples

    \Output
        int32_t         - number of samples converted

    \Version 04/09/2019 (cvienneau)
*/
/*************************************************************************************F*/
static int32_t _VoipOpusConvertInt32ToFloat(VoipCodecRefT *pState, uint8_t *pInBytes, int32_t iNumSamples)
{
    VoipOpusRefT *pOpus = (VoipOpusRefT *)pState;
    
    if (pOpus->bInt32Input)
    {
        int32_t *pInput = (int32_t*)pInBytes;
        float *pOutput = (float*)pInBytes;

        int32_t iBufferIndex;
        for (iBufferIndex = 0; iBufferIndex < iNumSamples; ++iBufferIndex)
        {
            pOutput[iBufferIndex] = (float)pInput[iBufferIndex] / INT32_MAX;
        }
        return (iBufferIndex);
    }
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function _VoipOpusResample

    \Description
        Resample in coming samples to the rate of VOIPOPUS_DEFAULT_SAMPLING_RATE.
        If VOIPOPUS_DEFAULT_SAMPLING_RATE equal iNumSamples no resampling is done.

    \Input *pState        - codec state
    \Input *pInBytes      - input samples in byte array
    \Input iNumSamples    - number of samples in pInBytes
    \Input *pOutBytes     - output samples written
    \Input uOutBuffSize   - size of pOutBytes

    \Output
        int32_t         - number of samples written

    \Version 03/26/2019 (tcho)
*/
/*************************************************************************************F*/
static int32_t _VoipOpusResample(VoipCodecRefT *pState, const uint8_t *pInBytes, int32_t iNumSamples, uint8_t *pOutBytes, uint32_t uOutBuffSize)
{
    VoipOpusRefT *pOpus = (VoipOpusRefT *)pState;
    int32_t iOutputSamples = iNumSamples; //default skipped resample
    int32_t iError;

    if (pOpus->bFloatInput)
    {
        iOutputSamples = uOutBuffSize / sizeof(float);  //goes into speex_resampler_process_float as buffer size, comes out as samples written
        if ((iError = speex_resampler_process_float(pOpus->pSpeexResampler, 0, (const float *)pInBytes, (uint32_t *)&iNumSamples, (float *)pOutBytes, (uint32_t *)&iOutputSamples)) != 0)
        {
            NetPrintf(("voipopus: error resampling float, %d input samples (Error = %d).\n", iNumSamples, iError));
        }
    }
    else
    {
        iOutputSamples = uOutBuffSize / sizeof(int16_t);  //goes into speex_resampler_process_int as buffer size, comes out as samples written
        if ((iError = speex_resampler_process_int(pOpus->pSpeexResampler, 0, (const int16_t *)pInBytes, (uint32_t *)&iNumSamples, (int16_t *)pOutBytes, (uint32_t *)&iOutputSamples)) != 0)
        {
            NetPrintf(("voipopus: error resampling int16, %d input samples, %d (Error = %d).\n", iNumSamples, iError));
        }
    }

    return(iOutputSamples);
}


/*F*************************************************************************************/
/*!
    \Function _VoipOpusEncodeBlock

    \Description
        Encode a buffer 16-bit audio or float sample using the Opus encoder

    \Input *pState      - codec state
    \Input *pOutput     - [out] outbut buffer
    \Input *pInput      - input buffer
    \Input iNumSamples  - the number of samples to encode

    \Output
        int32_t         - positive=number of encoded bytes, negative=error

    \Version 07/03/2017 (eesponda)
*/
/*************************************************************************************F*/
static int32_t _VoipOpusEncodeBlock(VoipCodecRefT *pState, uint8_t *pOutput, const int16_t *pInput, int32_t iNumSamples)
{
    VoipOpusRefT *pOpus = (VoipOpusRefT *)pState;
    int32_t iResult = -1;
    uint32_t uSampleRateIn;
    uint8_t aResampledData[VOIPOPUS_RESAMPLE_BUFFER_SIZE];

    /* if we haven't set a sample rate manually via the 'insr' control calculate the sample rate based on number of samples. this is with the assumption of 20ms audio
       $$todo$$ investigate removing the manual calls of 'insr' on xbox if we can confirm that this calculation works */
    uSampleRateIn = (pOpus->uSampleRateIn == 0) ? VOIPOPUS_GetSampleRate(iNumSamples) : pOpus->uSampleRateIn;

    // convert the data to a useable format if needed
    iResult = _VoipOpusConvertInt32ToFloat(pState, (uint8_t*)pInput, iNumSamples);

    if (uSampleRateIn != VOIPOPUS_DEFAULT_SAMPLING_RATE)
    {
        // re-create the resampler if needed
        if (uSampleRateIn != pOpus->uResamplerRate)
        {
            if (pOpus->pSpeexResampler != NULL)
            {
                speex_resampler_destroy(pOpus->pSpeexResampler);
            }
            if ((pOpus->pSpeexResampler = speex_resampler_init_frac(VOIPOPUS_DEFAULT_CHANNELS, uSampleRateIn, VOIPOPUS_DEFAULT_SAMPLING_RATE, uSampleRateIn, VOIPOPUS_DEFAULT_SAMPLING_RATE, VOIPOPUS_RESAMPLER_QUALITY, &iResult)) == NULL)
            {
                NetPrintf(("voipopus: unable to allocate resampler (Error = %d).\n", iResult));
                return(iResult);
            }
            pOpus->uResamplerRate = uSampleRateIn;
        }

        // resample the data if needed
        iResult = _VoipOpusResample(pState, (uint8_t*)pInput, iNumSamples, aResampledData, sizeof(aResampledData));
        if (iResult != iNumSamples)
        {
            pInput = (const int16_t*)aResampledData;
            iNumSamples = iResult;
        }
    }

    // encode as float or int16
    if (pOpus->bFloatInput)
    {
        if ((iResult = opus_encode_float(pOpus->pEncoder, (float*)pInput, iNumSamples, pOutput, VOIPOPUS_MAX_OUTPUT)) < 0)
        {
            NetPrintf(("voipopus: unable to encode float (err=%s)\n", DirtyErrGetNameList(iResult, _VoipOpus_aErrList)));
        }
    }
    else
    {
        if ((iResult = opus_encode(pOpus->pEncoder, pInput, iNumSamples, pOutput, VOIPOPUS_MAX_OUTPUT)) < 0)
        {
            NetPrintf(("voipopus: unable to encode int16_t (err=%s)\n", DirtyErrGetNameList(iResult, _VoipOpus_aErrList)));
        }
    }
    return(iResult);
}

/*F*************************************************************************************/
/*!
    \Function _VoipOpusDecodeBlock

    \Description
        Decode a Opus encoded input to 16-bit linear PCM samples

    \Input *pState      - codec state
    \Input *pOutput     - [out] outbut buffer
    \Input *pInput      - input buffer
    \Input iInputBytes  - size of the input buffer
    \Input iChannel     - the decode channel for which we are decoding data

    \Output
        int32_t         - positive=number of decoded samples, negative=error

    \Version 07/03/2017 (eesponda)
*/
/*************************************************************************************F*/
static int32_t _VoipOpusDecodeBlock(VoipCodecRefT *pState, int32_t *pOutput, const uint8_t *pInput, int32_t iInputBytes, int32_t iChannel)
{
    VoipOpusRefT *pOpus = (VoipOpusRefT *)pState;
    int32_t iResult, iSample;
    int16_t aOutput[VOIPOPUS_MAX_FRAME];

    if ((iChannel < 0) || (iChannel > pOpus->CodecState.iDecodeChannels))
    {
        NetPrintf(("voipopus: trying to decode with invalid decoder channel\n"));
        return(-1);
    }

    if ((iResult = opus_decode(pOpus->aDecoders[iChannel], pInput, iInputBytes, aOutput, VOIPOPUS_MAX_FRAME, 0)) < 0)
    {
        NetPrintf(("voipopus: unable to decode (err=%s)\n", DirtyErrGetNameList(iResult, _VoipOpus_aErrList)));
    }

    // accumulate output in the expected format
    for (iSample = 0; iSample < iResult; iSample += 1)
    {
        pOutput[iSample] += (aOutput[iSample] * pOpus->iOutputVolume) >> VOIP_CODEC_OUTPUT_FRACTIONAL;
    }
    return(iResult);
}

/*F*************************************************************************************/
/*!
    \Function _VoipOpusReset

    \Description
        Reset the codec state

    \Input *pState      - codec state

    \Version 07/03/2017 (eesponda)
*/
/*************************************************************************************F*/
static void _VoipOpusReset(VoipCodecRefT *pState)
{
    int32_t iChannel;
    VoipOpusRefT *pOpus = (VoipOpusRefT *)pState;

    opus_encoder_ctl(pOpus->pEncoder, OPUS_RESET_STATE);

    for (iChannel = 0; iChannel < pOpus->CodecState.iDecodeChannels; iChannel += 1)
    {
        opus_decoder_ctl(pOpus->aDecoders[iChannel], OPUS_RESET_STATE);
    }
}

/*F*************************************************************************************/
/*!
    \Function _VoipOpusControl

    \Description
        Set control options

    \Input *pState  - codec state
    \Input iControl - control selector
    \Input iValue   - selector specific
    \Input iValue2  - selector specific
    \Input *pValue  - selector specific

    \Output
        int32_t     - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'plvl'  - Set the output volumn
            'infl'  - Set if the input samples are float via iValue, (default int16)
            'inin'  - Set if the input samples are int32_t via iValue, (default int16)
            'insr'  - Set the input sample rate
            'spam'  - Set debug output verbosity
        \endverbatim

    \Version 07/03/2017 (eesponda)
*/
/*************************************************************************************F*/
static int32_t _VoipOpusControl(VoipCodecRefT *pState, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    VoipOpusRefT *pOpus = (VoipOpusRefT *)pState;

    if (iControl == 'plvl')
    {
        pOpus->iOutputVolume = iValue;
        return(0);
    }
    if (iControl == 'infl')
    {
        pOpus->bInt32Input = FALSE;
        pOpus->bFloatInput = iValue;
        return(0);
    }
    if (iControl == 'inin')
    {
        pOpus->bInt32Input = iValue; //int32s are converted to float, so we set both
        pOpus->bFloatInput = iValue;
        return(0);
    }

    if (iControl == 'insr')
    {
        pOpus->uSampleRateIn = iValue;
        return(0);
    }  
    if (iControl == 'spam')
    {
        pOpus->iVerbose = iValue;
        return(0);
    }
    // unhandled control
    return(-1);
}

/*F*************************************************************************************/
/*!
    \Function _VoipOpusStatus

    \Description
        Get codec status

    \Input *pState  - codec state
    \Input iSelect  - status selector
    \Input iValue   - selector-specific
    \Input *pBuffer - [out] storage for selector output
    \Input iBufSize - size of output buffer

    \Output
        int32_t     - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'vlen'      - returns TRUE to indicate we are using variable length frames
        \endverbatim

    \Version 07/03/2017 (eesponda)
*/
/*************************************************************************************F*/
static int32_t _VoipOpusStatus(VoipCodecRefT *pState, int32_t iSelect, int32_t iValue, void *pBuffer, int32_t iBufSize)
{
    if (iSelect == 'vlen')
    {
        *(uint8_t *)pBuffer = TRUE;
        return(0);
    }
    // unhandle selector
    return(-1);
}
