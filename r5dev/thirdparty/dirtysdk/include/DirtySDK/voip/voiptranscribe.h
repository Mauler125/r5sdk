/*H********************************************************************************/
/*!
    \File voiptranscribe.h

    \Description
        Voip transcription API wrapping Cloud-based speech-to-text services, supporting
        IBM Watson, Microsoft Speech Service, Google Speech, and Amazon Transcribe.

    \Copyright
        Copyright 2018 Electronic Arts

    \Version 08/30/2018 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _voiptranscribe_h
#define _voiptranscribe_h

/*!
\Moduledef VoipTranscribe VoipTranscribe
\Modulemember Voip
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

//! maximum length of transcription text, null termination included
#define VOIPTRANSCRIBE_OUTPUT_MAX (1024)

/*
    Transcription profiles
*/

// [......xx] transport (1=http, 2=http/2, 3=websockets)
// [....xx..] audio format (1=li16, 2=wav16, 3=opus)
// [..xx....] provider (1=watson, 2=bing)
// [xx......] reserved
    
//! construct a transcription profile identifier
#define VOIPTRANSCRIBE_PROFILE_CONSTRUCT(_eProvider, _eAudioFormat, _eTransport)    ((_eTransport)|((_eAudioFormat)<<8)|((_eProvider<<16)))
#define VOIPTRANSCRIBE_PROFILE_PROVIDER(_uProfile)          (VoipTranscribeProviderE)(((_uProfile)>>16)&0xff)
#define VOIPTRANSCRIBE_PROFILE_FORMAT(_uProfile)            (VoipTranscribeFormatE)(((_uProfile)>>8)&0xff)
#define VOIPTRANSCRIBE_PROFILE_TRANSPORT(_uProfile)         (VoipTranscribeTransportE)((_uProfile)&0xff)

// disabled profile
#define VOIPTRANSCRIBE_PROFILE_DISABLED                     VOIPTRANSCRIBE_PROFILE_CONSTRUCT(VOIPTRANSCRIBE_PROVIDER_NONE,   VOIPTRANSCRIBE_FORMAT_NONE,  VOIPTRANSCRIBE_TRANSPORT_NONE)
// watson profiles
#define VOIPTRANSCRIBE_PROFILE_IBMWATSON_HTTP_LI16          VOIPTRANSCRIBE_PROFILE_CONSTRUCT(VOIPTRANSCRIBE_PROVIDER_IBMWATSON, VOIPTRANSCRIBE_FORMAT_LI16,  VOIPTRANSCRIBE_TRANSPORT_HTTP)
#define VOIPTRANSCRIBE_PROFILE_IBMWATSON_HTTP_WAV16         VOIPTRANSCRIBE_PROFILE_CONSTRUCT(VOIPTRANSCRIBE_PROVIDER_IBMWATSON, VOIPTRANSCRIBE_FORMAT_WAV16, VOIPTRANSCRIBE_TRANSPORT_HTTP)
#define VOIPTRANSCRIBE_PROFILE_IBMWATSON_HTTP_OPUS          VOIPTRANSCRIBE_PROFILE_CONSTRUCT(VOIPTRANSCRIBE_PROVIDER_IBMWATSON, VOIPTRANSCRIBE_FORMAT_OPUS,  VOIPTRANSCRIBE_TRANSPORT_HTTP)
#define VOIPTRANSCRIBE_PROFILE_IBMWATSON_WEBSOCKETS_LI16    VOIPTRANSCRIBE_PROFILE_CONSTRUCT(VOIPTRANSCRIBE_PROVIDER_IBMWATSON, VOIPTRANSCRIBE_FORMAT_LI16,  VOIPTRANSCRIBE_TRANSPORT_WEBSOCKETS)
#define VOIPTRANSCRIBE_PROFILE_IBMWATSON_WEBSOCKETS_WAV16   VOIPTRANSCRIBE_PROFILE_CONSTRUCT(VOIPTRANSCRIBE_PROVIDER_IBMWATSON, VOIPTRANSCRIBE_FORMAT_WAV16, VOIPTRANSCRIBE_TRANSPORT_WEBSOCKETS)
#define VOIPTRANSCRIBE_PROFILE_IBMWATSON_WEBSOCKETS_OPUS    VOIPTRANSCRIBE_PROFILE_CONSTRUCT(VOIPTRANSCRIBE_PROVIDER_IBMWATSON, VOIPTRANSCRIBE_FORMAT_OPUS,  VOIPTRANSCRIBE_TRANSPORT_WEBSOCKETS)
// bing profiles
#define VOIPTRANSCRIBE_PROFILE_MICROSOFT_HTTP_WAV16         VOIPTRANSCRIBE_PROFILE_CONSTRUCT(VOIPTRANSCRIBE_PROVIDER_MICROSOFT, VOIPTRANSCRIBE_FORMAT_WAV16, VOIPTRANSCRIBE_TRANSPORT_HTTP)
#define VOIPTRANSCRIBE_PROFILE_MICROSOFT_HTTP_OPUS          VOIPTRANSCRIBE_PROFILE_CONSTRUCT(VOIPTRANSCRIBE_PROVIDER_MICROSOFT, VOIPTRANSCRIBE_FORMAT_OPUS,  VOIPTRANSCRIBE_TRANSPORT_HTTP)
// google profiles
#define VOIPTRANSCRIBE_PROFILE_GOOGLE_HTTP_LI16             VOIPTRANSCRIBE_PROFILE_CONSTRUCT(VOIPTRANSCRIBE_PROVIDER_GOOGLE,    VOIPTRANSCRIBE_FORMAT_LI16,  VOIPTRANSCRIBE_TRANSPORT_HTTP)
#define VOIPTRANSCRIBE_PROFILE_GOOGLE_HTTP_OPUS             VOIPTRANSCRIBE_PROFILE_CONSTRUCT(VOIPTRANSCRIBE_PROVIDER_GOOGLE,    VOIPTRANSCRIBE_FORMAT_OPUS,  VOIPTRANSCRIBE_TRANSPORT_HTTP)
#define VOIPTRANSCRIBE_PROFILE_GOOGLE_HTTP2_LI16            VOIPTRANSCRIBE_PROFILE_CONSTRUCT(VOIPTRANSCRIBE_PROVIDER_GOOGLE,    VOIPTRANSCRIBE_FORMAT_LI16,  VOIPTRANSCRIBE_TRANSPORT_HTTP2)
#define VOIPTRANSCRIBE_PROFILE_GOOGLE_HTTP2_OPUS            VOIPTRANSCRIBE_PROFILE_CONSTRUCT(VOIPTRANSCRIBE_PROVIDER_GOOGLE,    VOIPTRANSCRIBE_FORMAT_OPUS,  VOIPTRANSCRIBE_TRANSPORT_HTTP2)
// amazon profiles
#define VOIPTRANSCRIBE_PROFILE_AMAZON_HTTP2                 VOIPTRANSCRIBE_PROFILE_CONSTRUCT(VOIPTRANSCRIBE_PROVIDER_AMAZON,    VOIPTRANSCRIBE_FORMAT_LI16,  VOIPTRANSCRIBE_TRANSPORT_HTTP2)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! possible transcription providers
typedef enum VoipTranscribeProviderE
{
    VOIPTRANSCRIBE_PROVIDER_NONE,
    VOIPTRANSCRIBE_PROVIDER_IBMWATSON,      //!< IBM Watson
    VOIPTRANSCRIBE_PROVIDER_MICROSOFT,      //!< Microsoft Cognitive Services
    VOIPTRANSCRIBE_PROVIDER_GOOGLE,         //!< Google Speech
    VOIPTRANSCRIBE_PROVIDER_AMAZON,         //!< Amazon Transcribe
    VOIPTRANSCRIBE_NUMPROVIDERS
} VoipTranscribeProviderE;

//! possible transcription audio formats
typedef enum VoipTranscribeFormatE
{
    VOIPTRANSCRIBE_FORMAT_NONE,
    VOIPTRANSCRIBE_FORMAT_LI16,             //!< 16bit linear PCM audio with no framing
    VOIPTRANSCRIBE_FORMAT_WAV16,            //!< 16bit linear PCM in a WAV wrapper
    VOIPTRANSCRIBE_FORMAT_OPUS,             //!< Opus codec
    VOIPTRANSCRIBE_NUMFORMATS
} VoipTranscribeFormatE;

//! possible transport protocols
typedef enum VoipTranscribeTransportE
{
    VOIPTRANSCRIBE_TRANSPORT_NONE,
    VOIPTRANSCRIBE_TRANSPORT_HTTP,          //!< HTTPS
    VOIPTRANSCRIBE_TRANSPORT_HTTP2,         //!< HTTP2
    VOIPTRANSCRIBE_TRANSPORT_WEBSOCKETS,    //!< WebSockets
    VOIPTRANSCRIBE_NUMTRANSPORTS
} VoipTranscribeTransportE;

//! opaque module state
typedef struct VoipTranscribeRefT VoipTranscribeRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module
DIRTYCODE_API VoipTranscribeRefT *VoipTranscribeCreate(int32_t iBufSize);

// configure the transcribe module
DIRTYCODE_API void VoipTranscribeConfig(uint32_t uProfile, const char *pUrl, const char *pKey);

// destroy the module
DIRTYCODE_API void VoipTranscribeDestroy(VoipTranscribeRefT *pVoipTranscribe);

// submit voice data to be transcribed
DIRTYCODE_API int32_t VoipTranscribeSubmit(VoipTranscribeRefT *pVoipTranscribe, const uint8_t *pBuffer, int32_t iBufLen);

// get a transcription
DIRTYCODE_API int32_t VoipTranscribeGet(VoipTranscribeRefT *pVoipTranscribe, char *pBuffer, int32_t iBufLen);

// get module status
DIRTYCODE_API int32_t VoipTranscribeStatus(VoipTranscribeRefT *pVoipTranscribe, int32_t iStatus, int32_t iValue, void *pBuffer, int32_t iBufSize);

// set control options
DIRTYCODE_API int32_t VoipTranscribeControl(VoipTranscribeRefT *pVoipTranscribe, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// update module
DIRTYCODE_API void VoipTranscribeUpdate(VoipTranscribeRefT *pVoipTranscribe);

#ifdef __cplusplus
}
#endif

//@}

#endif // _voiptranscribe_h

