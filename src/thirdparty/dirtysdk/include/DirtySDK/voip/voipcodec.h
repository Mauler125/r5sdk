/*H********************************************************************************/
/*!
    \File voipcodec.h

    \Description
        Defines an interface for Voip codec modules, and a singleton dispatcher
        used to manage codec instances.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 08/04/2004 (jbrookes)  First version
*/
/********************************************************************************H*/

#ifndef _voipcodec_h
#define _voipcodec_h

/*!
\Moduledef VoipCodec VoipCodec
\Modulemember Voip
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/voip/voiptranscribe.h"

/*** Defines **********************************************************************/

//! used by some of the fractional integer codes
#define VOIP_CODEC_OUTPUT_FRACTIONAL    (12)

//! operate on active codec (passed for ident, valid for VoipDestroy, VoipControl)
#define VOIP_CODEC_ACTIVE               (-1)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! opaque module ref
typedef struct VoipCodecRefT VoipCodecRefT;

//! create function type
typedef VoipCodecRefT *(VoipCodecCreateT)(int32_t iNumDecoders);

//! destroy function type
typedef void (VoipCodecDestroyT)(VoipCodecRefT *pState);

//! encode function type
typedef int32_t (VoipCodecEncodeT)(VoipCodecRefT *pState, uint8_t *pOutput, const int16_t *pInput, int32_t iNumSamples);

//! decode function type
typedef int32_t (VoipCodecDecodeT)(VoipCodecRefT *pState, int32_t *pOutput, const uint8_t *pInput, int32_t iInputBytes, int32_t iChannel);

//! reset function type
typedef void (VoipCodecResetT)(VoipCodecRefT *pState);

//! control function type
typedef int32_t (VoipCodecControlT)(VoipCodecRefT *pState, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue);

//! status function type
typedef int32_t (VoipCodecStatusT)(VoipCodecRefT *pState, int32_t iSelect, int32_t iValue, void *pBuffer, int32_t iBufSize);

//! codec function block
typedef struct VoipCodecDefT
{
    VoipCodecCreateT    *pCreate;
    VoipCodecDestroyT   *pDestroy;
    VoipCodecEncodeT    *pEncode;
    VoipCodecDecodeT    *pDecode;
    VoipCodecResetT     *pReset;
    VoipCodecControlT   *pControl;
    VoipCodecStatusT    *pStatus;
} VoipCodecDefT;

//! codec state structure
struct VoipCodecRefT
{
    const VoipCodecDefT *pCodecDef;
    int32_t             iDecodeChannels;

    #if DIRTYCODE_LOGGING
    int32_t iDebugLevel;            //<! debug level
    #endif

    // VAD
    float               fPowerLevel;               //<! current calculated audio power level
    float               fVadPowerThreshold;        //<! signal level threshold (power of) used to detect silence in a single frame
    int32_t             iVadCumulSilenceFrames;    //<! cumulative count of recently detected silent frame (in non-stop sequence)
    int32_t             iVadSilenceFramesThreshold;//<! number of back-to-back silence frames required to detect silence
    uint8_t             bVadEnabled;               //<! whether VAD is enabled or not
    uint8_t             bVadSilent;                //<! whether voice is currently considered silent or not
    uint8_t             _pad[2];       
};

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


// register a codec type
DIRTYCODE_API void VoipCodecRegister(int32_t iCodecIdent, const VoipCodecDefT *pCodecDef);

// create a codec module
DIRTYCODE_API int32_t VoipCodecCreate(int32_t iCodecIdent, int32_t iDecodeChannels);

// customize codec
DIRTYCODE_API int32_t VoipCodecControl(int32_t iCodecIdent, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// get codec status
DIRTYCODE_API int32_t VoipCodecStatus(int32_t iCodecIdent, int32_t iSelect, int32_t iValue, void *pBuffer, int32_t iBufSize);

// destroy active codec
DIRTYCODE_API void VoipCodecDestroy(void);

// reset active codec
DIRTYCODE_API void VoipCodecReset(void);

// encode using the active codec
DIRTYCODE_API int32_t VoipCodecEncode(uint8_t *pOutput, const int16_t *pInput, int32_t iNumSamples, VoipTranscribeRefT *pTranscribeRef);

// decode using the active codec
DIRTYCODE_API int32_t VoipCodecDecode(int32_t *pOutput, const uint8_t *pInput, int32_t iInputBytes, int32_t iChannel);

#ifdef __cplusplus
}
#endif

//@}

#endif // _voipcodec_h
