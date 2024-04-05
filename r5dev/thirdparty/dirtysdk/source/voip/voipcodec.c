/*H********************************************************************************/
/*!
    \File voipcodec.c

    \Description
        VoIP codec manager.

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

#include "DirtySDK/voip/voipdef.h"
#include "DirtySDK/voip/voiptranscribe.h"
#include "DirtySDK/voip/voipcodec.h"

/*** Defines **********************************************************************/

#define VOIPCODEC_MAXENTRIES    (8)
#define VOIPCODEC_TIMER         (DIRTYCODE_LOGGING && FALSE)

//! default VAD thresholds
#define VOIPCODEC_DEFAULT_VAD_AMPLITUDE_THRESHOLD   (0.00015f)
#define VOIPCODEC_DEFAULT_VAD_FRAMES_THRESHOLD      (32)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct VoipCodecRegT
{
    int32_t             iCodecIdent;
    const VoipCodecDefT *pCodecDef;
    VoipCodecRefT       *pCodecRef;
} VoipCodecRegT;


/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

//! codec reg table
static VoipCodecRegT _VoipCodec_RegTable[VOIPCODEC_MAXENTRIES];

//! number of registered codecs
static int32_t       _VoipCodec_iNumCodecs = 0;

//! active codec
static int32_t       _VoipCodec_iActiveCodec = -1;


/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _VoipCodecGet

    \Description
        Get the specified codec.

    \Input iCodecIdent      - codec ident or VOIP_CODEC_ACTIVE
    \Input *pIndex          - [out, optional] storage for codec index

    \Output
        VoipCodecRegT *     - pointer to specified codec, or NULL

    \Version 10/10/2011 (jbrookes)
*/
/********************************************************************************F*/
static VoipCodecRegT *_VoipCodecGet(int32_t iCodecIdent, int32_t *pIndex)
{
    VoipCodecRegT *pRegEntry = NULL;
    int32_t iCodec;

    // zero is reserved/invalid
    if (iCodecIdent == 0)
    {
        return(NULL);
    }

    // search for codec ident
    if (iCodecIdent != VOIP_CODEC_ACTIVE)
    {
        // look up codec by ident
        for (iCodec = 0; iCodec < _VoipCodec_iNumCodecs; iCodec++)
        {
            if (_VoipCodec_RegTable[iCodec].iCodecIdent == iCodecIdent)
            {
                pRegEntry = &_VoipCodec_RegTable[iCodec];
                if (pIndex != NULL)
                {
                    *pIndex = iCodec;
                }
                break;
            }
        }
    }
    else if (_VoipCodec_iActiveCodec != -1)
    {
        pRegEntry = &_VoipCodec_RegTable[_VoipCodec_iActiveCodec];
        if (pIndex != NULL)
        {
            *pIndex = _VoipCodec_iActiveCodec;
        }
    }

    return(pRegEntry);
}

/*F*************************************************************************************************/
/*!
    \Function    _VoipCodecVadProcess16BitFrame

    \Description
        Use this function to pass next frame of 16-bit voice samples to the VAD engine.

    \Input *pVoipCodecRef   - codec reference pointer
    \Input *pVoiceFrame     - pointer to voice buffer
    \Input iNumSamples      - number of samples in voice frame

    \Output
        uint8_t         - TRUE if voice is considered silent after processing that frame, FALSE otherwise

    \Version 06/13/2012 (mclouatre)
*/
/*************************************************************************************************F*/
static uint8_t _VoipCodecVadProcess16BitFrame(VoipCodecRefT *pVoipCodecRef, const int16_t *pVoiceFrame, int32_t iNumSamples)
{
    int32_t iCount;
    float fAmp, fPower, fPowerSum;

    fAmp = fPower = fPowerSum = 0.0f;

    // walk the frame contents and calculate the amplitude of the signal
    for (iCount = 0; iCount < iNumSamples; iCount += 1)
    {
        fAmp = (float)abs(pVoiceFrame[iCount]);
        fPowerSum += (fAmp * fAmp);
    }

    fPower = (fPowerSum / (32768.0f * 32768.0f * (float)iNumSamples));

    // save current power level for external visibility
    pVoipCodecRef->fPowerLevel = fPower;

    if (fPower < pVoipCodecRef->fVadPowerThreshold)
    {
        if (pVoipCodecRef->iVadCumulSilenceFrames > pVoipCodecRef->iVadSilenceFramesThreshold)
        {
            if (!pVoipCodecRef->bVadSilent)
            {
                NetPrintfVerbose((pVoipCodecRef->iDebugLevel, 1, "voipcodec: silence detected (fPowerThreshold=%f, fPower=%f)\n",
                    pVoipCodecRef->fVadPowerThreshold, fPower));
                pVoipCodecRef->bVadSilent = TRUE;
            }
        }
        else
        {
            pVoipCodecRef->iVadCumulSilenceFrames++;
        }
    }
    else
    {
        if (pVoipCodecRef->bVadSilent)
        {
            NetPrintfVerbose((pVoipCodecRef->iDebugLevel, 1, "voipcodec: voice detected (fPowerThreshold=%f, fPower=%f)\n",
                    pVoipCodecRef->fVadPowerThreshold, fPower));
            pVoipCodecRef->bVadSilent = FALSE;
        }
        pVoipCodecRef->iVadCumulSilenceFrames = 0;
    }

    return(pVoipCodecRef->bVadSilent);
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipCodecRegister

    \Description
        Register a codec in the table.

    \Input  iCodecIdent - identifier with which the codec will be tagged
    \Input  *pCodecDef  - pointer to codec definition table

    \Version 08/04/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipCodecRegister(int32_t iCodecIdent, const VoipCodecDefT *pCodecDef)
{
    int32_t iCodec;

    for (iCodec = _VoipCodec_iNumCodecs; iCodec < VOIPCODEC_MAXENTRIES; iCodec++)
    {
        if (_VoipCodec_RegTable[iCodec].iCodecIdent == 0)
        {
            NetPrintf(("voipcodec: registering codec '%C'\n", iCodecIdent));
            _VoipCodec_RegTable[iCodec].iCodecIdent = iCodecIdent;
            _VoipCodec_RegTable[iCodec].pCodecDef = pCodecDef;
            _VoipCodec_iNumCodecs++;
            return;
        }
    }

    NetPrintf(("voipcodec: unable to register codec '%C'\n", iCodecIdent));
}

/*F********************************************************************************/
/*!
    \Function VoipCodecCreate

    \Description
        Create a codec module of the given type.

    \Input  iCodecIdent     - identifier with which the codec will be tagged
    \Input  iDecodeChannels - number of decoder channels

    \Output
        int32_t             - zero=success, negative=failure

    \Version 08/04/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipCodecCreate(int32_t iCodecIdent, int32_t iDecodeChannels)
{
    VoipCodecRegT *pRegEntry = NULL;
    int32_t iCodec = 0;

    // get codec reg entry
    if ((pRegEntry = _VoipCodecGet(iCodecIdent, &iCodec)) == NULL)
    {
        NetPrintf(("voipcodec: codec '%C' is not registered\n", iCodecIdent));
        return(-1);
    }

    // if there is an active codec, destroy it
    VoipCodecDestroy();

    // create it and return to caller
    if ((pRegEntry->pCodecRef = pRegEntry->pCodecDef->pCreate(iDecodeChannels)) != NULL)
    {
        NetPrintf(("voipcodec: created codec '%C'\n", iCodecIdent));
        _VoipCodec_iActiveCodec = iCodec;
    }
    else
    {
        NetPrintf(("voipcodec: unable to create codec '%C'\n", iCodecIdent));
        return(-1);
    }

    // initialize default values
    pRegEntry->pCodecRef->fVadPowerThreshold = VOIPCODEC_DEFAULT_VAD_AMPLITUDE_THRESHOLD;
    pRegEntry->pCodecRef->iVadSilenceFramesThreshold = VOIPCODEC_DEFAULT_VAD_FRAMES_THRESHOLD;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function VoipCodecControl

    \Description
        Control codec.

    \Input iCodecIdent - codec identifier, or VOIP_CODEC_ACTIVE
    \Input iControl - control selector
    \Input iValue   - selector specific
    \Input iValue2  - selector specific
    \Input *pValue  - selector specific

    \Output
        int32_t     - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            '+vad' - enable/disable vad
            'vada' - signal level threshold used to detect silence in a single frame (in millionths)
            'vadf' - number of silence frames required for silence detection to kick in
        \endverbatim

    \Version 08/09/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipCodecControl(int32_t iCodecIdent, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    VoipCodecRegT *pRegEntry;

    // make sure active codec is valid
    if ((pRegEntry = _VoipCodecGet(iCodecIdent, NULL)) == NULL)
    {
        NetPrintf(("voipcodec: invalid codec, cannot process control message '%C'\n", iControl));
        return(-1);
    }

    if (iControl == '+vad')
    {
        if (pRegEntry->pCodecRef->bVadEnabled != (iValue?TRUE:FALSE))
        {
            NetPrintf(("voipcodec: VAD mode changed from %s to %s\n", (pRegEntry->pCodecRef->bVadEnabled?"ON":"OFF"), (iValue?"ON":"OFF")));
            pRegEntry->pCodecRef->bVadEnabled= (iValue?TRUE:FALSE);
        }
        return(0);
    }

    #if DIRTYCODE_LOGGING
    if (iControl == 'spam')
    {
        NetPrintf(("voipcodec: debug verbosity level changed from %d to %d\n", pRegEntry->pCodecRef->iDebugLevel, iValue));
        pRegEntry->pCodecRef->iDebugLevel = iValue;

        // do not return here, we want to pass-through to codec-specific control funct.
    }
    #endif


    if (iControl == 'vada')
    {
        float fNewThreshold;
        fNewThreshold = iValue * 0.000001f;
        NetPrintf(("voipcodec: VAD signal amplitude threshold (power of) changed from %f to %f\n", pRegEntry->pCodecRef->fVadPowerThreshold, fNewThreshold));
        pRegEntry->pCodecRef->fVadPowerThreshold = fNewThreshold;
        return(0);
    }

    if (iControl == 'vadf')
    {
        NetPrintf(("voipcodec: VAD frames threshold changed from %d to %d\n", pRegEntry->pCodecRef->iVadSilenceFramesThreshold, iValue));
        pRegEntry->pCodecRef->iVadSilenceFramesThreshold = iValue;
        return(0);
    }

    // then call the registered codec control function
    return(pRegEntry->pCodecDef->pControl(pRegEntry->pCodecRef, iControl, iValue, iValue2, pValue));
}

/*F********************************************************************************/
/*!
    \Function VoipCodecStatus

    \Description
        Get codec status

    \Input iCodecIdent - codec identifier, or VOIP_CODEC_ACTIVE
    \Input iSelect  - status selector
    \Input iValue   - selector-specific
    \Input *pBuffer - [out] selector-specific buffer space
    \Input iBufSize - size of output buffer

    \Output
        int32_t     - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'coco' -    returns the number of register codecs
            'plvl' -    returns current voice power level (same calc as VAD threshold)
        \endverbatim

    \Version 08/09/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipCodecStatus(int32_t iCodecIdent, int32_t iSelect, int32_t iValue, void *pBuffer, int32_t iBufSize)
{
    VoipCodecRegT *pRegEntry;
    
    // returns the number of active codecs
    if (iSelect == 'coco')
    {
        return(_VoipCodec_iNumCodecs);
    }
    // make sure active codec is valid
    if ((pRegEntry = _VoipCodecGet(iCodecIdent, NULL)) == NULL)
    {
        NetPrintf(("voipcodec: invalid codec, cannot process status message '%C'\n", iSelect));
        return(-1);
    }
    // returns current voice power level (same calculation as is used for VAD threshold)
    if (iSelect == 'plvl')
    {
        return((int32_t)(pRegEntry->pCodecRef->fPowerLevel / 0.000001f));
    }
    // call the registered status function
    return(pRegEntry->pCodecDef->pStatus(pRegEntry->pCodecRef, iSelect, iValue, pBuffer, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function VoipCodecDestroy

    \Description
        Destroy the active codec

    \Version 08/04/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipCodecDestroy(void)
{
    VoipCodecRegT *pRegEntry;

    // get active codec
    if ((pRegEntry = _VoipCodecGet(VOIP_CODEC_ACTIVE, NULL)) == NULL)
    {
        return;
    }

    // call the registered destroy function
    pRegEntry->pCodecRef->pCodecDef->pDestroy(pRegEntry->pCodecRef);
    // clear codec ref state pointer
    pRegEntry->pCodecRef = NULL;
    // reset active codec index
    _VoipCodec_iActiveCodec = -1;
}

/*F********************************************************************************/
/*!
    \Function VoipCodecReset

    \Description
        Resets codec state of active codec.

    \Version 08/04/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipCodecReset(void)
{
    VoipCodecRegT *pRegEntry;

    // get active codec
    if ((pRegEntry = _VoipCodecGet(VOIP_CODEC_ACTIVE, NULL)) == NULL)
    {
        return;
    }
    // call the registered reset function
    pRegEntry->pCodecDef->pReset(pRegEntry->pCodecRef);
}

/*F********************************************************************************/
/*!
    \Function VoipCodecEncode

    \Description
        Encode input data with the active codec.

    \Input *pOutput         - pointer to output buffer
    \Input *pInput          - pointer to input sample data
    \Input iNumSamples      - number of input samples
    \Input *pTranscribeRef  - transcription ref, if available

    \Output
        int32_t             - size of output data

    \Version 08/04/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipCodecEncode(uint8_t *pOutput, const int16_t *pInput, int32_t iNumSamples, VoipTranscribeRefT *pTranscribeRef)
{
    VoipCodecRegT *pRegEntry;
    int32_t iResult;
    #if VOIPCODEC_TIMER
    uint64_t uTimer;
    #endif

    // make sure active codec is valid
    if ((pRegEntry = _VoipCodecGet(VOIP_CODEC_ACTIVE, NULL)) == NULL)
    {
        NetPrintf(("voipcodec: no active codec, cannot encode\n"));
        return(0);
    }

    // if VAD is enabled and we detect silence, skip the encode
    if (pRegEntry->pCodecRef->bVadEnabled && (_VoipCodecVadProcess16BitFrame(pRegEntry->pCodecRef, pInput, iNumSamples)))
    {
        return(0);
    }

    #if VOIPCODEC_TIMER
    uTimer = NetTickUsec();
    #endif

    // call the registered encoder
    iResult = pRegEntry->pCodecDef->pEncode(pRegEntry->pCodecRef, pOutput, pInput, iNumSamples);

    #if VOIPCODEC_TIMER
    NetPrintf(("voipcodecencode: %dus\n", NetTickDiff(NetTickUsec(), uTimer)));
    #endif

    // submit voice data for transcription if we were passed a transcription ref
    if ((pTranscribeRef != NULL) && (iResult > 0))
    {
        if (VoipTranscribeStatus(pTranscribeRef, 'cmpr', 0, NULL, 0))
        {
            // transcribing compressed audio
            VoipTranscribeSubmit(pTranscribeRef, pOutput, iResult);
        }
        else
        {
            // transcribing uncompressed audio
            VoipTranscribeSubmit(pTranscribeRef, (const uint8_t *)pInput, iNumSamples*sizeof(*pInput));
        }
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function VoipCodecDecode

    \Description
        Decode input data with the active codec.

    \Input  *pOutput    - pointer to output buffer
    \Input  *pInput     - pointer to input compressed data
    \Input  iInputBytes - size in bytes of input data
    \Input  iChannel    - the input channel whose data we are decoding

    \Output
        int32_t         - number of output samples

    \Version 08/04/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipCodecDecode(int32_t *pOutput, const uint8_t *pInput, int32_t iInputBytes, int32_t iChannel)
{
    VoipCodecRegT *pRegEntry;
    int32_t iResult;
    #if VOIPCODEC_TIMER
    uint64_t uTimer = NetTickUsec();
    #endif

    // make sure active codec is valid
    if ((pRegEntry = _VoipCodecGet(VOIP_CODEC_ACTIVE, NULL)) == NULL)
    {
        NetPrintf(("voipcodec: no active codec, cannot decode\n"));
        return(0);
    }

    // call the registered decoder
    iResult = pRegEntry->pCodecDef->pDecode(pRegEntry->pCodecRef, pOutput, pInput, iInputBytes, iChannel);

    #if VOIPCODEC_TIMER
    NetPrintf(("voipcodecdecode: %dus\n", NetTickDiff(NetTickUsec(), uTimer)));
    #endif

    return(iResult);
}

