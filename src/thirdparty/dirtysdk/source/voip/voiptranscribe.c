/*H********************************************************************************/
/*!
    \File voiptranscribe.c

    \Description
        VoIP transcription API wrapping Cloud-based speech-to-text services, supporting
        IBM Watson, Microsoft Speech Service, Google Speech, and Amazon Transcribe.

    \Notes
        References

        Google Speech-to-Text:
            Main page: https://cloud.google.com/speech-to-text/docs/
            REST API: https://cloud.google.com/speech-to-text/docs/reference/rest/
            gRPC API: https://cloud.google.com/speech-to-text/docs/reference/rpc/
            Protobuf definitions: https://github.com/googleapis/googleapis/blob/master/google/cloud/speech/v1/cloud_speech.proto 
            Audio Formats: https://cloud.google.com/speech-to-text/docs/reference/rest/v1/RecognitionConfig#AudioEncoding

        IBM Watson:
            Speech to Text API: https://www.ibm.com/watson/developercloud/speech-to-text/api/v1/curl.html
            HTTP interface: https://console.bluemix.net/docs/services/speech-to-text/http.html
            WebSockets interface: https://console.bluemix.net/docs/services/speech-to-text/websockets.html
            Audio Formats: https://console.bluemix.net/docs/services/speech-to-text/audio-formats.html

        Microsoft Speech Service:
            Main page: https://docs.microsoft.com/en-us/azure/cognitive-services/Speech-Service/
            REST API: https://docs.microsoft.com/en-us/azure/cognitive-services/Speech-Service/rest-apis
            Speech Service WebSocket protocol: https://docs.microsoft.com/en-us/azure/cognitive-services/speech/api-reference-rest/websocketprotocol

        Amazon Transcribe:
            Main Page: https://docs.aws.amazon.com/transcribe/latest/dg/what-is-transcribe.html
            Streaming Transcription: https://docs.aws.amazon.com/transcribe/latest/dg/streaming.html
            StartStreamTranscription: https://docs.aws.amazon.com/transcribe/latest/dg/API_streaming_StartStreamTranscription.html
                NOTE: Amazon public API documentation is as of this writing not fully correct; the streaming
                format is completely different than what is described and there are other minor changes.
            Streaming Format: https://docs.aws.amazon.com/transcribe/latest/dg/streaming-format.html

        Ogg/Opus:
            Ogg file format: https://tools.ietf.org/html/rfc3533
            Ogg encapsulation for the Opus Audio Codec: https://tools.ietf.org/html/rfc7845.html
            Definition of the Opus Audio Codec: https://tools.ietf.org/html/rfc6716

        WAV:
            WAVE file format: https://en.wikipedia.org/wiki/WAV#RIFF_WAVE

    \Copyright
        Copyright 2018 Electronic Arts

    \Version 08/30/2018 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/util/aws.h"
#include "DirtySDK/util/base64.h"
#include "DirtySDK/util/jsonparse.h"

#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/proto/protohttp2.h"
#include "DirtySDK/proto/protowebsocket.h"
#include "DirtySDK/proto/protossl.h"

#include "DirtySDK/util/protobufcommon.h"
#include "DirtySDK/util/protobufread.h"
#include "DirtySDK/util/protobufwrite.h"

#include "DirtySDK/crypt/cryptrand.h" //$$temp 

#include "DirtySDK/voip/voipdef.h"
#include "DirtySDK/voip/voiptranscribe.h"

/*** Defines **********************************************************************/

#define VOIPTRANSCRIBE_MAXURL       (1024)
#define VOIPTRANSCRIBE_MINBUFFER    (8*32*1024) //!< buffering for up to eight seconds of uncompressed audio
#define VOIPTRANSCRIBE_SENDTIMEOUT  (100)       //!< milliseconds of silence audio before we consider an active recording sequence to be complete
#define VOIPTRANSCRIBE_WAIT         (-100)      //!< replaces PROTHTTP(2)_WAIT

#define VOIPTRANSCRIBE_CONSECEMPTY  (3)         //!< default number of consecutive empty results before we backoff
#define VOIPTRANSCRIBE_CONSECERROR  (3)         //!< default number of consecutive request failures before we backoff

#define VOIPTRANSCRIBE_AUDIORATE    (16000)     //!< audio rate in samples per second

/*! maximum number of samples (eight seconds worth) we allow in a single request; we limit this in case a user's VAD
    is not effective as well as to limit the size of the audio buffers required.  if we don't break the requests up,
    the user will wait indefinitely for a very long transcription result.  if the user's microphone is too sensitive
    and picking up music, background noise etc continuously, breaking up the requests will cause multiple transactions
    with non-voice data to be sent, and will trigger backoff due to empty transcription results being received */
#define VOIPTRANSCRIBE_MAXREQSAMPLES (8*VOIPTRANSCRIBE_AUDIORATE)

#define OGG_HEAD_LENGTH             (26)        //!< header length
#define OGG_HEAD_TYPE_OFFSET        (5)         //!< offset of type within header
#define OGG_HEAD_GPOS_OFFSET        (6)         //!< offset of granule position within header

#define OGG_PAGE_SEG_MAX            (255)       //!< maximum number of pages in an ogg segment table
#define OGG_PAGE_SEG_DEF            (50)        //!< 50 pages with each page being 20ms of audio equals one second of audio per page

#define OGG_TYPE_DAT                (0x00)      //!< data page
#define OGG_TYPE_CNT                (0x01)      //!< continuation
#define OGG_TYPE_BOS                (0x02)      //!< beginning of stream
#define OGG_TYPE_EOS                (0x04)      //!< end of stream

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*
    Transport stream API used to provide a single interface to HTTP, HTTP2, and WebSocket stream transport
*/

// forward declaration for transport type
typedef struct TransportT TransportT;

// transport API
typedef void *(TransportCreate)(int32_t iBufSize);
typedef void (TransportDestroy)(void *pState);
typedef int32_t (TransportConnect)(void *pState, const char *pUrl);
typedef void (TransportDisconnect)(void *pState);
typedef void (TransportUpdate)(void *pState);
typedef int32_t (TransportRequest)(void *pState, const char *pUrl, const char *pBuffer, int32_t iLength, int32_t *pRequestId);
typedef int32_t (TransportSend)(void *pState, int32_t iRequestId, const char *pBuffer, int32_t iLength);
typedef int32_t (TransportRecv)(void *pState, int32_t iRequestId, char *pBuffer, int32_t iLength);
typedef int32_t (TransportStatus)(void *pState, int32_t iRequestId, int32_t iStatus, void *pBuffer, int32_t iBufSize);
typedef int32_t (TransportControl)(void *pState, int32_t iRequestId, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

//! supported transport types
typedef enum TransportE
{
    TRANSPORT_HTTP = 0,
    TRANSPORT_HTTP2,
    TRANSPORT_WEBSOCKETS,
    TRANSPORT_NUMPROTOCOLS
} TransportE;

//! transport class
struct TransportT
{
    TransportE eTransport;
    void *pState;
    int32_t iStreamId;
    TransportCreate *Create;
    TransportDestroy *Destroy;
    TransportConnect *Connect;
    TransportDisconnect *Disconnect;
    TransportUpdate *Update;
    TransportRequest *Request;
    TransportSend *Send;
    TransportRecv *Recv;
    TransportStatus *Status;
    TransportControl *Control;
};

//! ogg file writer
typedef struct OggWriterT
{
    uint8_t *pBuffer;                   //!< ogg write buffer
    uint8_t *pHeader;                   //!< pointer to current header
    uint8_t *pChecksum;                 //!< pointer to current header checksum
    uint8_t *pSegmentTable;             //!< pointer to current segment table
    uint64_t uGranulePos;               //!< ogg granule position in units of 48khz audio samples
    uint32_t uPageSeqn;                 //!< monotonically increasing page number
    uint32_t uSerial;                   //!< stream serial number
    int32_t iBufLen;                    //!< ogg buffer length
    int32_t iBufOff;                    //!< ogg write offset
    int32_t iBufAudioStart;             //!< start of buffered audio
    int32_t iNumSegments;               //!< number of segments written to current Ogg page
} OggWriterT;

/*
    Voip transcription types
*/

//! buffer to hold audio data while it is being submitted
typedef struct VoipBufferT
{
    uint8_t *pBuffer;                   //!< buffer memory
    int32_t iBufLen;                    //!< buffer length
    int32_t iBufOff;                    //!< writing offset within buffer (buffering audio)
    int32_t iBufInp;                    //!< reading offset within buffer (sending buffered audio)
    int32_t iNumSamples;                //!< number of samples in buffer
    int8_t iBuffer;                     //!< buffer index
    uint8_t bRecStarting;               //!< TRUE if recording is starting
    uint8_t bRecFinished;               //!< TRUE if recording is finished
    uint8_t bRecFull;                   //!< TRUE if recording buffer is full
    uint8_t bMinDiscard;                //!< minimum discard status for this buffer
    uint8_t _pad[3];
    OggWriterT OggWriter;               //!< ogg writer type, used for writing compressed audio
} VoipBufferT;

typedef struct VoipTranscribeConfigT
{
    uint32_t uProfile;                  //!> transcription profile
    char    strUrl[VOIPTRANSCRIBE_MAXURL]; //!< url to access transcription service
    char    strKey[128];                //!< api key for access to transcription service
} VoipTranscribeConfigT;

//! module state memory
struct VoipTranscribeRefT
{
    // module memory group
    int32_t iMemGroup;                  //!< module mem group id
    void *pMemGroupUserData;            //!< user data associated with mem group

    // module states
    enum
    {
        ST_FAIL=-1,                     //!< fail
        ST_IDLE,                        //!< idle
        ST_CONN,                        //!< connecting
        ST_SEND,                        //!< sending voice data
        ST_RECV                         //!< receiving transcription result
    } eState;

    int32_t iTimeout;                   //!< current http timeout
    uint32_t uVoipTick;                 //!< timestamp when last voice sample was submitted

    uint32_t uProfile;                  //!> transcription profile
    VoipTranscribeProviderE eProvider;  //!< transcription provider
    VoipTranscribeFormatE eFormat;      //!< audio format
    VoipTranscribeTransportE eTransport; //!< transport protocol
    int32_t iAudioRate;                 //!< sampling rate of audio in hz

    char    strUrl[VOIPTRANSCRIBE_MAXURL]; //!< url to access transcription service
    char    strKey[128];                //!< api key for access to transcription service
    char    strAudioFormat[64];         //!< current audio format e.g. audio/li16

    int32_t iConsecErrorCt;             //!< number of consecutive request failures
    int32_t iConsecErrorMax;            //!< maximum number of consecutive request failures before we backoff
    int32_t iConsecEmptyCt;             //!< number of consecutive empty request results
    int32_t iConsecEmptyMax;            //!< maximum number of consecutive empty request results before we backoff
    uint32_t uBackoffTimer;             //!< millisecond counter tracking when current backoff time expires

    uint8_t bConnected;                 //!< connected to transcription service
    uint8_t bCompressed;                //!< TRUE if transcribing compressed audio, else FALSE
    uint8_t bMinDiscard;                //!< true if minimum voice sample discard enabled
    int8_t  iRecBuffer;                 //!< current recording buffer
    int8_t  iSndBuffer;                 //!< current sending buffer
    int8_t  iVerbose;                   //!< verbose debug level (debug only)
    uint8_t _pad[2];

    AWSSignInfoT AWSSignInfo;           //!< AWS Signing info for current request

    TransportT Transport;               //!< transport object
    VoipSpeechToTextMetricsT Metrics;   //!< metrics object
    uint32_t uSttStartTime;             //!< time we finished sending the request

    uint16_t aJsonParseBuf[2*1024];     //!< buffer to parse JSON results
    char    strResponse[16*1024*4];     //!< transcription server response $$temp - increased 4x for large Amazon partial results
    char    strTranscription[VOIPTRANSCRIBE_OUTPUT_MAX];     //!< transcription result
    char    strSessionId[128];          //!< AWS session id

    VoipBufferT VoipBuffer[2];          //!< voip audio double buffer
    VoipBufferT VoipBufferSnd;          //!< voip send buffer (amazon & google need audio encoded)
};

/*** Variables ********************************************************************/

//! global config state
static VoipTranscribeConfigT _VoipTranscribe_Config = { VOIPTRANSCRIBE_PROFILE_DISABLED, "", "" };

//! table for calculating Ogg CRC checksum
static const uint32_t _Ogg_CRCTable[256] =
{
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
    0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61, 0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
    0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039, 0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
    0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1, 0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
    0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
    0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde, 0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
    0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
    0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6, 0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
    0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
    0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637, 0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
    0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff, 0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
    0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7, 0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
    0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
    0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8, 0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
    0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
    0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0, 0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
    0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
    0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668, 0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

//! Ogg Opus Identity Header
static const uint8_t _aOggOpusIdentHeader[] =
{
    // magic numbers
    'O', 'p', 'u', 's', 
    'H', 'e', 'a', 'd',
    1, // version
    1, // output channel count
    0, 0, // pre-skip
    0x80, 0x3e, 0x00, 0x00, // input sample rate; 16khz
    0, 0, // output gain
    0, // output channel mapping
};

//! Ogg Opus Comment Header
static const uint8_t _aOggOpusCommentHeader[] =
{
    // magic numbers
    'O', 'p', 'u', 's', 
    'T', 'a', 'g', 's',
    // vendor string length
    2, 0, 0, 0,
    // vendor string
    'E', 'A',
    // user comment list length
    0, 0, 0, 0
};

//! Baltimore Cybertrust Root CA, needed for Microsoft Speech
static const char _strCyberTrustRootCA[] =
{
    "-----BEGIN CERTIFICATE-----"
    "MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ"
    "RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD"
    "VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX"
    "DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y"
    "ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy"
    "VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr"
    "mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr"
    "IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK"
    "mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu"
    "XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy"
    "dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye"
    "jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1"
    "BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3"
    "DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92"
    "9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx"
    "jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0"
    "Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz"
    "ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS"
    "R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp"
    "-----END CERTIFICATE-----"
};

//! GlobalSign Root CA R2, needed for Google
static const char _strGlobalSignRootCAR2[] =
{
    "-----BEGIN CERTIFICATE-----"
    "MIIDujCCAqKgAwIBAgILBAAAAAABD4Ym5g0wDQYJKoZIhvcNAQEFBQAwTDEgMB4G"
    "A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjIxEzARBgNVBAoTCkdsb2JhbFNp"
    "Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDYxMjE1MDgwMDAwWhcNMjExMjE1"
    "MDgwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEG"
    "A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI"
    "hvcNAQEBBQADggEPADCCAQoCggEBAKbPJA6+Lm8omUVCxKs+IVSbC9N/hHD6ErPL"
    "v4dfxn+G07IwXNb9rfF73OX4YJYJkhD10FPe+3t+c4isUoh7SqbKSaZeqKeMWhG8"
    "eoLrvozps6yWJQeXSpkqBy+0Hne/ig+1AnwblrjFuTosvNYSuetZfeLQBoZfXklq"
    "tTleiDTsvHgMCJiEbKjNS7SgfQx5TfC4LcshytVsW33hoCmEofnTlEnLJGKRILzd"
    "C9XZzPnqJworc5HGnRusyMvo4KD0L5CLTfuwNhv2GXqF4G3yYROIXJ/gkwpRl4pa"
    "zq+r1feqCapgvdzZX99yqWATXgAByUr6P6TqBwMhAo6CygPCm48CAwEAAaOBnDCB"
    "mTAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUm+IH"
    "V2ccHsBqBt5ZtJot39wZhi4wNgYDVR0fBC8wLTAroCmgJ4YlaHR0cDovL2NybC5n"
    "bG9iYWxzaWduLm5ldC9yb290LXIyLmNybDAfBgNVHSMEGDAWgBSb4gdXZxwewGoG"
    "3lm0mi3f3BmGLjANBgkqhkiG9w0BAQUFAAOCAQEAmYFThxxol4aR7OBKuEQLq4Gs"
    "J0/WwbgcQ3izDJr86iw8bmEbTUsp9Z8FHSbBuOmDAGJFtqkIk7mpM0sYmsL4h4hO"
    "291xNBrBVNpGP+DTKqttVCL1OmLNIG+6KYnX3ZHu01yiPqFbQfXf5WRDLenVOavS"
    "ot+3i9DAgBkcRcAtjOj4LaR0VknFBbVPFd5uRHg5h6h+u/N5GJG79G+dwfCMNYxd"
    "AfvDbbnvRG15RjF+Cv6pgsH/76tuIMRQyV+dTZsXjAzlAcmgQWpzU/qlULRuJQ/7"
    "TBj0/VLZjmmx6BEP3ojY+x1J96relc8geMJgEtslQIxq/H5COEBkEveegeGTLg=="
    "-----END CERTIFICATE-----"
};

//! Amazon Root CA R1, needed for Amazon Transcribe
static const char _strAmazonRootCAR1[] =
{
    "-----BEGIN CERTIFICATE-----"
    "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF"
    "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6"
    "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL"
    "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv"
    "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj"
    "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM"
    "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw"
    "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6"
    "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L"
    "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm"
    "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC"
    "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA"
    "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI"
    "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs"
    "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv"
    "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU"
    "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy"
    "rqXRfboQnoZsG4q5WTP468SQvvG5"
    "-----END CERTIFICATE-----"
};


/*** Private Functions ************************************************************/

/*
    Transport wrapper class providing protocol-agnostic API for using HTTP, HTTP2, or WebSockets for stream transport
*/

/*
    TransportRequest
*/
static int32_t _TransportHttpRequest(void *pProtoHttp, const char *pUrl, const char *pBuffer, int32_t iLength, int32_t *pRequestId)
{
    return(ProtoHttpPost(pProtoHttp, pUrl, (iLength != PROTOHTTP_STREAM_BEGIN) ? pBuffer : NULL, iLength, FALSE));
}

static int32_t _TransportHttp2Request(void *pProtoHttp2, const char *pUrl, const char *pBuffer, int32_t iLength, int32_t *pRequestId)
{
    return(ProtoHttp2Request(pProtoHttp2, pUrl, NULL, PROTOHTTP2_STREAM_BEGIN, PROTOHTTP_REQUESTTYPE_POST, pRequestId));
}

static int32_t _TransportWebSocketRequest(void *pState, const char *pUrl, const char *pBuffer, int32_t iLength, int32_t *pRequestId)
{
    return(ProtoWebSocketSendText(pState, pBuffer));
}

/*
    TransportSend
*/
static int32_t _TransportHttpSend(void *pState, int32_t iRequestId, const char *pBuffer, int32_t iLength)
{
    return(ProtoHttpSend(pState, pBuffer, iLength));
}

static int32_t _TransportHttp2Send(void *pState, int32_t iRequestId, const char *pBuffer, int32_t iLength)
{
    int32_t iResult = ProtoHttp2Send(pState, iRequestId, (const uint8_t *)pBuffer, iLength);
    return(iResult);
}

static int32_t _TransportWebSocketSend(void *pState, int32_t iRequestId, const char *pBuffer, int32_t iLength)
{
    return(ProtoWebSocketSend(pState, pBuffer, iLength));
}

/*
    TransportRecv - note, a zero result returned by one of these functions indicates completion with no data
*/
static int32_t _TransportHttpRecv(void *pState, int32_t iRequestId, char *pBuffer, int32_t iLength)
{
    int32_t iResult = ProtoHttpRecvAll(pState, pBuffer, iLength);
    return((iResult != PROTOHTTP_RECVWAIT) ? iResult : VOIPTRANSCRIBE_WAIT);
}

static int32_t _TransportHttp2Recv(void *pState, int32_t iRequestId, char *pBuffer, int32_t iLength)
{
    int32_t iResult = ProtoHttp2RecvAll(pState, iRequestId, (uint8_t *)pBuffer, iLength);
    return((iResult != PROTOHTTP2_RECVWAIT) ? iResult : VOIPTRANSCRIBE_WAIT);
}

static int32_t _TransportWebSocketRecv(void *pState, int32_t iRequestId, char *pBuffer, int32_t iLength)
{
    int32_t iResult = ProtoWebSocketRecv(pState, pBuffer, iLength);
    return((iResult != 0) ? iResult : VOIPTRANSCRIBE_WAIT);
}

/*
    TransportStatus
*/
static int32_t _TransportHttpStatus(void *pState, int32_t iRequestId, int32_t iStatus, void *pBuffer, int32_t iBufSize)
{
    return(ProtoHttpStatus(pState, iStatus, pBuffer, iBufSize));
}

static int32_t _TransportWebSocketStatus(void *pState, int32_t iRequestId, int32_t iStatus, void *pBuffer, int32_t iBufSize)
{
    return(ProtoWebSocketStatus(pState, iStatus, pBuffer, iBufSize));
}

/*
    TransportControl
*/
static int32_t _TransportHttpControl(void *pState, int32_t iRequestId, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    return(ProtoHttpControl(pState, iControl, iValue, iValue2, pValue));
}

static int32_t _TransportWebSocketControl(void *pState, int32_t iRequestId, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    return(ProtoWebSocketControl(pState, iControl, iValue, iValue2, pValue));
}


/*F********************************************************************************/
/*!
    \Function _TransportInit

    \Description
        Init Transport handler for specified transport method

    \Input *pTransport      - transport handler structure
    \Input eTransport       - transport handler type

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _TransportInit(TransportT *pTransport, TransportE eTransport)
{
    pTransport->eTransport = eTransport;
    switch (eTransport)
    {
        case TRANSPORT_HTTP:
            pTransport->Create = (TransportCreate *)ProtoHttpCreate;
            pTransport->Destroy = (TransportDestroy *)ProtoHttpDestroy;
            pTransport->Connect = NULL;
            pTransport->Disconnect = NULL;
            pTransport->Update = (TransportUpdate *)ProtoHttpUpdate;
            pTransport->Request = _TransportHttpRequest;
            pTransport->Send = _TransportHttpSend;
            pTransport->Recv = _TransportHttpRecv;
            pTransport->Status = _TransportHttpStatus;
            pTransport->Control = _TransportHttpControl;
            break;
        case TRANSPORT_HTTP2:
            pTransport->Create = (TransportCreate *)ProtoHttp2Create;
            pTransport->Destroy = (TransportDestroy *)ProtoHttp2Destroy;
            pTransport->Connect = NULL;
            pTransport->Disconnect = NULL;
            pTransport->Update = (TransportUpdate *)ProtoHttp2Update;
            pTransport->Request = _TransportHttp2Request;
            pTransport->Send = _TransportHttp2Send;
            pTransport->Recv = _TransportHttp2Recv;
            pTransport->Status = (TransportStatus *)ProtoHttp2Status;
            pTransport->Control = (TransportControl *)ProtoHttp2Control;
            break;
        case TRANSPORT_WEBSOCKETS:
            pTransport->Create = (TransportCreate *)ProtoWebSocketCreate;
            pTransport->Destroy = (TransportDestroy *)ProtoWebSocketDestroy;
            pTransport->Connect = (TransportConnect *)ProtoWebSocketConnect;
            pTransport->Disconnect = (TransportDisconnect *)ProtoWebSocketDisconnect;
            pTransport->Update = (TransportUpdate *)ProtoWebSocketUpdate;
            pTransport->Request = _TransportWebSocketRequest;
            pTransport->Send = _TransportWebSocketSend;
            pTransport->Recv = _TransportWebSocketRecv;
            pTransport->Status = _TransportWebSocketStatus;
            pTransport->Control = _TransportWebSocketControl;
            break;
        default:
            NetPrintf(("transport: init error\n"));
            break;
    }
}

/*
    Misc functions we may need for Microsoft when using WebSockets
*/

/*F********************************************************************************/
/*!
    \Function _GenerateUUID

    \Description
        Generate a type Version 4 UUID as per
        https://en.wikipedia.org/wiki/Universally_unique_identifier#Version_4_(random)

    \Input *pBuffer         - [out] storage for UUID
    \Input iBufLen          - buffer length
    \Input bDashes          - include dashes if true

    \Version 09/15/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _GenerateUUID(char *pBuffer, int32_t iBufLen, uint8_t bDashes)
{
    uint32_t uRand[4];

#if 0
    // generate 128 bits of vaguely random data
    int32_t iRand;
    for (iRand = 0; iRand < 4; iRand += 1)
    {
        uRand[iRand] = NetRand(0xffffffff);
    }
#else
    CryptRandGet((uint8_t *)uRand, sizeof(uRand));
#endif

    /* fixup:    set the four most significant bits of the 7th byte to 0100'B, so the high nibble is "4"
    set the two most significant bits of the 9th byte to 10'B, so the high nibble will be one of "8", "9", "A", or "B". */
    uRand[1] &= ~0xf000;
    uRand[1] |=  0x4000;
    uRand[2] &= ~0xc0000000;
    uRand[2] |=  0x80000000;

    // format it out 
    if (bDashes)
    {
        ds_snzprintf(pBuffer, iBufLen, "%08x-%04x-%04x-%04x-%04x%0x", uRand[0], uRand[1]>>16, uRand[1]&0xff, uRand[2]>>16, uRand[2]&0xff, uRand[3]);
    }
    else
    {
        ds_snzprintf(pBuffer, iBufLen, "%08x%08x%08x%08x", uRand[0], uRand[1], uRand[2], uRand[3]);
    }
}

/*F********************************************************************************/
/*!
    \Function _GenerateTimestamp

    \Description
        Generate a timestamp following 8601 format plus milliseconds

    \Input *pBuffer         - [out] storage for timestamp
    \Input iBufLen          - buffer length

    \Version 09/15/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _GenerateTimestamp(char *pBuffer, int32_t iBufLen)
{
    struct tm CurTime;
    char strMillis[8];
    int32_t iMillis;
    // get current time... this is equivalent to time(0)
    //$$todo - make sure this is UTC
    NetPlattimeToTimeMs(&CurTime, &iMillis);
    // convert to ISO_8601
    ds_timetostr(&CurTime, TIMETOSTRING_CONVERSION_ISO_8601, 1, pBuffer, iBufLen);
    // append milliseconds
    ds_snzprintf(strMillis, sizeof(strMillis), "%d", iMillis);
    ds_strnzcat(pBuffer, strMillis, iBufLen);
}

/*
    Wave functions to encapsulate our PCM16 audio in a WAV header
*/

/*F********************************************************************************/
/*!
    \Function _WaveWriteHeader

    \Description
        Write WAV header into output buffer

    \Input *pBuffer         - [out] buffer to write header to
    \Input iBufLen          - length of buffer
    \Input iAudioRate       - audio rate in hz
    \Input iDataSize        - size of all data (audio+headers)

    \Output
        int32_t             - size of header

    \Version 09/13/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _WaveWriteHeader(uint8_t *pBuffer, int32_t iBufLen, int32_t iAudioRate, int32_t iDataSize)
{
    int32_t iOffset=0, iSize;
    static const int32_t _Wav_iFmtLen = 4+4+2+2+4+4+2+4; //! wav format chunk length

    // write group id
    pBuffer[iOffset++] = 'R';
    pBuffer[iOffset++] = 'I';
    pBuffer[iOffset++] = 'F';
    pBuffer[iOffset++] = 'F';
    // write total length as counted after size field
    iSize = iDataSize-iOffset-4;
    pBuffer[iOffset++] = (uint8_t)(iSize);
    pBuffer[iOffset++] = (uint8_t)(iSize>>8);
    pBuffer[iOffset++] = (uint8_t)(iSize>>16);
    pBuffer[iOffset++] = (uint8_t)(iSize>>24);
    // write RIFF type
    pBuffer[iOffset++] = 'W';
    pBuffer[iOffset++] = 'A';
    pBuffer[iOffset++] = 'V';
    pBuffer[iOffset++] = 'E';

    // write format chunk

    // format group id
    pBuffer[iOffset++] = 'f';
    pBuffer[iOffset++] = 'm';
    pBuffer[iOffset++] = 't';
    pBuffer[iOffset++] = ' ';
    // write chunk size as counted after size field
    iSize = _Wav_iFmtLen-8;
    pBuffer[iOffset++] = (uint8_t)(iSize);
    pBuffer[iOffset++] = (uint8_t)(iSize>>8);
    pBuffer[iOffset++] = (uint8_t)(iSize>>16);
    pBuffer[iOffset++] = (uint8_t)(iSize>>24);
    // format tag (16 bit, always 1)
    pBuffer[iOffset++] = 1;
    pBuffer[iOffset++] = 0;
    // channels (16 bit)
    pBuffer[iOffset++] = 1;
    pBuffer[iOffset++] = 0;
    // sampling rate
    pBuffer[iOffset++] = (uint8_t)(iAudioRate);
    pBuffer[iOffset++] = (uint8_t)(iAudioRate>>8);
    pBuffer[iOffset++] = (uint8_t)(iAudioRate>>16);
    pBuffer[iOffset++] = (uint8_t)(iAudioRate>>24);
    // average bytes per second - rate x 2
    pBuffer[iOffset++] = (uint8_t)(iAudioRate*2);
    pBuffer[iOffset++] = (uint8_t)((iAudioRate*2)>>8);
    pBuffer[iOffset++] = (uint8_t)((iAudioRate*2)>>16);
    pBuffer[iOffset++] = (uint8_t)((iAudioRate*2)>>24);
    // block alignment (bytes per sample)
    pBuffer[iOffset++] = 2;
    pBuffer[iOffset++] = 0;
    // bits per sample
    pBuffer[iOffset++] = 0x10;
    pBuffer[iOffset++] = 0;
    pBuffer[iOffset++] = 0;
    pBuffer[iOffset++] = 0;

    // write data chunk
    pBuffer[iOffset++] = 'd';
    pBuffer[iOffset++] = 'a';
    pBuffer[iOffset++] = 't';
    pBuffer[iOffset++] = 'a';
    // write data size
    iSize = iDataSize-iOffset-4;
    pBuffer[iOffset++] = (uint8_t)(iSize);
    pBuffer[iOffset++] = (uint8_t)(iSize>>8);
    pBuffer[iOffset++] = (uint8_t)(iSize>>16);
    pBuffer[iOffset++] = (uint8_t)(iSize>>24);

    // return header offset
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _WaveWriteOpen

    \Description
        Writes required WAV header and returns offset to the start of data.

    \Input *pVoipBuffer     - voip buffer to write header to
    \Input iAudioRate       - audio rate in hz

    \Output
        int32_t             - offset to end of data in buffer

    \Version 09/13/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _WaveWriteOpen(VoipBufferT *pVoipBuffer, int32_t iAudioRate)
{
    // we write an arbitrarily large size here as we don't know the data size in advance; both microsoft and watson support this
    return(_WaveWriteHeader(pVoipBuffer->pBuffer, pVoipBuffer->iBufLen, iAudioRate, 1024*1024));
}

/*
    Ogg/Opus functions to encapsulate our Opus codec data in the proper format for upload
*/

/*F********************************************************************************/
/*!
    \Function _OggWriteChecksum

    \Description
        Calculate Ogg CRC32 on specified data, and write to output buffer in
        little-endian

    \Input *pChecksum       - [out] output buffer for crc32 checksum
    \Input *pBuffer         - data to checksum
    \Input iBufSize         - amount of data to checksum

    \Version 09/12/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _OggWriteChecksum(uint8_t *pChecksum, const uint8_t *pBuffer, int32_t iBufSize)
{
    uint32_t uChecksum;
    int32_t iByte;

    // calculate crc32
    for (iByte = 0, uChecksum = 0; iByte < iBufSize; iByte += 1)
    {
        uChecksum = (uChecksum<<8)^_Ogg_CRCTable[((uChecksum>>24)&0xff)^pBuffer[iByte]];
    }

    // write crc32
    pChecksum[0] = (uint8_t)(uChecksum);
    pChecksum[1] = (uint8_t)(uChecksum>>8);
    pChecksum[2] = (uint8_t)(uChecksum>>16);
    pChecksum[3] = (uint8_t)(uChecksum>>24);
}

/*F********************************************************************************/
/*!
    \Function _OggWriteGranulePosition

    \Description
        Write granule position to buffer

    \Input *pBuffer         - [out] pointer to write location
    \Input uGranulePos      - granule position to write

    \Version 09/13/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _OggWriteGranulePosition(uint8_t *pBuffer, uint64_t uGranulePos)
{
    pBuffer[0] = (uint8_t)(uGranulePos);
    pBuffer[1] = (uint8_t)(uGranulePos>>8);
    pBuffer[2] = (uint8_t)(uGranulePos>>16);
    pBuffer[3] = (uint8_t)(uGranulePos>>24);
    pBuffer[4] = (uint8_t)(uGranulePos>>32);
    pBuffer[5] = (uint8_t)(uGranulePos>>40);
    pBuffer[6] = (uint8_t)(uGranulePos>>48);
    pBuffer[7] = (uint8_t)(uGranulePos>>56);
}

/*F********************************************************************************/
/*!
    \Function _OggWriteHeader

    \Description
        Write Ogg header into output buffer

    \Input *pWriter         - ogg writer
    \Input *pBuffer         - [out] buffer to write header to
    \Input iHeaderOffset    - offset to write header within buffer
    \Input uType            - page type (OGG_TYPE_*)
    \Input iNumSegments     - number of segments in segment table
    \Input iDataSize        - size of page data, or zero if it is not yet known

    \Output
        int32_t             - offset past end of header

    \Version 09/12/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _OggWriteHeader(OggWriterT *pWriter, uint8_t *pBuffer, int32_t iHeaderOffset, uint8_t uType, int32_t iNumSegments, int32_t iDataSize)
{
    int32_t iOffset = iHeaderOffset;

    // save pointer to current header
    pWriter->pHeader = pBuffer+iHeaderOffset;

    // write capture pattern
    pBuffer[iOffset++] = 'O';
    pBuffer[iOffset++] = 'g';
    pBuffer[iOffset++] = 'g';
    pBuffer[iOffset++] = 'S';
    // write version (always zero)
    pBuffer[iOffset++] = 0;
    // write header type - 1=continuation, 2=beginning of stream, 4=end of stream
    pBuffer[iOffset++] = uType;
    // reserve space for granule position
    ds_memclr(pBuffer+iOffset, 8);
    iOffset += 8;
    // write bitstream serial number (32 bit)
    pBuffer[iOffset++] = (uint8_t)(pWriter->uSerial);
    pBuffer[iOffset++] = (uint8_t)(pWriter->uSerial>>8);
    pBuffer[iOffset++] = (uint8_t)(pWriter->uSerial>>16);
    pBuffer[iOffset++] = (uint8_t)(pWriter->uSerial>>24);
    // write page sequence number
    pBuffer[iOffset++] = (uint8_t)(pWriter->uPageSeqn);
    pBuffer[iOffset++] = (uint8_t)(pWriter->uPageSeqn>>8);
    pBuffer[iOffset++] = (uint8_t)(pWriter->uPageSeqn>>16);
    pBuffer[iOffset++] = (uint8_t)(pWriter->uPageSeqn>>24);
    pWriter->uPageSeqn += 1;
    // write blank 32 bit checksum and save pointer to it
    pWriter->pChecksum = pBuffer+iOffset;
    pBuffer[iOffset++] = 0;
    pBuffer[iOffset++] = 0;
    pBuffer[iOffset++] = 0;
    pBuffer[iOffset++] = 0;
    // write page segments count
    pBuffer[iOffset++] = iNumSegments;
    // save segment table pointer
    pWriter->pSegmentTable = pBuffer+iOffset;
    // copy in page segments or reserve space if no segment table included
    if (iNumSegments == 1)
    {
        pBuffer[iOffset] = (uint8_t)iDataSize;
    }
    else
    {
        ds_memclr(pBuffer+iOffset, iNumSegments); //$$temp - for debugging, doesn't really need to be cleared
    }
    // move offset past segment table
    iOffset += iNumSegments;

    // return offset past header
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _OggWriteOpen

    \Description
        Set up a buffer for writing Ogg-encapsulated data

    \Input *pWriter         - ogg writer
    \Input *pBuffer         - buffer data will be written to
    \Input *iBufLen         - length of buffer

    \Output
        int32_t             - offset in buffer where audio data writing starts

    \Version 09/12/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _OggWriteOpen(OggWriterT *pWriter, uint8_t *pBuffer, int32_t iBufLen)
{
    // reset ogg segment counter
    pWriter->iNumSegments = 0;
    // set ogg buffer info
    pWriter->pBuffer = pBuffer;
    pWriter->iBufOff = 0;
    pWriter->iBufLen = iBufLen;
    // set bitstream serial number
    pWriter->uSerial = NetRand(0xffffffff);
    // reset page sequence
    pWriter->uPageSeqn = 0;
    // return offset to caller
    return(pWriter->iBufOff);
}

/*F********************************************************************************/
/*!
    \Function _OggWriteSegment

    \Description
        Write an Opus audio segment to the current page

    \Input *pWriter         - ogg writer
    \Input *pData           - opus data
    \Input *iDataLen        - length of opus data
    \Input iVerbose         - debug verbosity level

    \Output
        int32_t             - negative=buffer full, positive=updated offset in bytes if page is complete, else zero

    \Version 09/12/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _OggWriteSegment(OggWriterT *pWriter, const uint8_t *pData, int32_t iDataLen, int32_t iVerbose)
{
    // if we're at the beginning of a page, write the header
    if (pWriter->iNumSegments == 0)
    {
        pWriter->iBufOff = _OggWriteHeader(pWriter, pWriter->pBuffer, pWriter->iBufOff, 0, OGG_PAGE_SEG_DEF, 0);
    }
    // bail if we don't have room for the segment
    if ((pWriter->iBufOff+iDataLen) > pWriter->iBufLen)
    {
        NetPrintfVerbose((iVerbose, 1, "voiptranscribe: ogg/opus writer full\n"));
        return(-1);
    }
    // copy data to buffer
    ds_memcpy_s(pWriter->pBuffer+pWriter->iBufOff, pWriter->iBufLen-pWriter->iBufOff, pData, iDataLen);
    pWriter->iBufOff += iDataLen;
    // add to segment table
    pWriter->pSegmentTable[pWriter->iNumSegments++] = (uint8_t)iDataLen;
    // add to granule position
    pWriter->uGranulePos += 960; //$$temp - assume 20ms audio == 960 samples @48khz
    // if we've filled up the page, calculate the CRC and reset for the new page
    if (pWriter->iNumSegments == OGG_PAGE_SEG_DEF)
    {
        NetPrintfVerbose((iVerbose, 1, "voiptranscribe: wrote ogg/opus page with %d segments and length %d\n", pWriter->iNumSegments, pWriter->pBuffer+pWriter->iBufOff-pWriter->pHeader));
        // write updated granule position
        _OggWriteGranulePosition(pWriter->pHeader+OGG_HEAD_GPOS_OFFSET, pWriter->uGranulePos);
        // calculate the crc32 checksum
        _OggWriteChecksum(pWriter->pChecksum, pWriter->pHeader, pWriter->pBuffer+pWriter->iBufOff-pWriter->pHeader);
        // reset segment count
        pWriter->iNumSegments = 0;
        // return updated offset to caller, only once we've finalized the page
        return(pWriter->iBufOff);
    }
    // return zero for unfinalized page
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _OggOpusWriteHeader

    \Description
        Write an ogg/opus header

    \Input *pWriter         - ogg writer
    \Input *pBuffer         - buffer data will be written to
    \Input iOffset          - offset to write header
    \Input iBufLen          - length of buffer
    \Input uType            - header type (OGG_TYPE_*)
    \Input *pOpusHeader     - pointer to body of header we are writing
    \Input iOpusHeaderLen   - size of body we're writing

    \Output
        int32_t             - offset in buffer following header

    \Version 09/13/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _OggOpusWriteHeader(OggWriterT *pWriter, uint8_t *pBuffer, int32_t iOffset, int32_t iBufLen, uint8_t uType, const uint8_t *pOpusHeader, int32_t iOpusHeaderLen)
{
    int32_t iHeaderOffset = iOffset;
    // write the header
    iOffset = _OggWriteHeader(pWriter, pWriter->pBuffer, iOffset, uType, 1, iOpusHeaderLen);
    // copy the data
    ds_memcpy_s(pWriter->pBuffer+iOffset, pWriter->iBufLen-iOffset, pOpusHeader, iOpusHeaderLen);
    iOffset += iOpusHeaderLen;
    // calculate the crc32 checksum
    _OggWriteChecksum(pWriter->pChecksum, pBuffer+iHeaderOffset, iOffset-iHeaderOffset);
    // return offset to caller
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _OggOpusWriteOpen

    \Description
        Open an Ogg/Opus header for writing as per
        https://tools.ietf.org/html/rfc7845.html#section-5.1

    \Input *pWriter         - ogg writer
    \Input *pBuffer         - buffer data will be written to
    \Input *iBufLen         - length of buffer

    \Output
        int32_t                 - offset in buffer where audio data writing starts

    \Version 09/12/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _OggOpusWriteOpen(OggWriterT *pWriter, uint8_t *pBuffer, int32_t iBufLen)
{
    // initialize for writing
    pWriter->iBufOff = _OggWriteOpen(pWriter, pBuffer, iBufLen);
    // write Ogg Opus Ident Header
    pWriter->iBufOff = _OggOpusWriteHeader(pWriter, pWriter->pBuffer, pWriter->iBufOff, pWriter->iBufLen, OGG_TYPE_BOS, _aOggOpusIdentHeader, sizeof(_aOggOpusIdentHeader));
    // write Ogg Opus Comment Header
    pWriter->iBufOff = _OggOpusWriteHeader(pWriter, pWriter->pBuffer, pWriter->iBufOff, pWriter->iBufLen, OGG_TYPE_DAT, _aOggOpusCommentHeader, sizeof(_aOggOpusCommentHeader));
    // return offset to caller
    return(pWriter->iBufOff);
}

/*F********************************************************************************/
/*!
    \Function _OggOpusWriteFinish

    \Description
        Fixes up final page and marks it as end of stream.

    \Input *pWriter         - ogg writer
    \Input iVerbose         - debug verbosity level

    \Output
        int32_t             - offset to end of data

    \Version 09/12/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _OggOpusWriteFinish(OggWriterT *pWriter, int32_t iVerbose)
{
    // if we have a partially-filled page, finish it here
    if (pWriter->iNumSegments > 0)
    {
        int32_t iEmptySegments = OGG_PAGE_SEG_DEF-pWriter->iNumSegments;
        int32_t iMoveSize = (pWriter->pBuffer+pWriter->iBufOff) - (pWriter->pSegmentTable+OGG_PAGE_SEG_DEF);
        // contract to remove unwritten segment table entries
        memmove(pWriter->pSegmentTable+pWriter->iNumSegments, pWriter->pSegmentTable+OGG_PAGE_SEG_DEF, iMoveSize);
        pWriter->iBufOff -= iEmptySegments;
        NetPrintfVerbose((iVerbose, 1, "voiptranscribe: wrote ogg/opus page with %d segments and length %d\n", pWriter->iNumSegments, pWriter->pBuffer+pWriter->iBufOff-pWriter->pHeader));
        // update segment count
        pWriter->pHeader[OGG_HEAD_LENGTH] = (uint8_t)pWriter->iNumSegments;
        // update granule position
        _OggWriteGranulePosition(pWriter->pHeader+OGG_HEAD_GPOS_OFFSET, pWriter->uGranulePos);
        // write CRC for last page
        _OggWriteChecksum(pWriter->pChecksum, pWriter->pHeader, pWriter->pBuffer+pWriter->iBufOff-pWriter->pHeader);
        // reset segment count
        pWriter->iNumSegments = 0;
    }
    // mark final page as end of stream
    pWriter->pHeader[OGG_HEAD_TYPE_OFFSET] = OGG_TYPE_EOS;

    // return offset to start of data
    return(pWriter->iBufOff);
}

/*
    Voip Transcription
*/

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeBufferReset

    \Description
        Reset voip buffer state

    \Input *pVoipTranscribe     - module state
    \Input *pVoipBuffer         - buffer to initialize

    \Version 10/22/2019 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipTranscribeBufferReset(VoipTranscribeRefT *pVoipTranscribe, VoipBufferT *pVoipBuffer)
{
    NetPrintfVerbose((pVoipTranscribe->iVerbose, 1, "voiptranscribe: [%d] resetting buffer\n", pVoipBuffer->iBuffer));
    pVoipBuffer->iBufOff = 0;
    pVoipBuffer->iBufInp = 0;
    pVoipBuffer->iNumSamples = 0;
    pVoipBuffer->bRecStarting = TRUE;
    pVoipBuffer->bRecFinished = FALSE;
    pVoipBuffer->bRecFull = FALSE;
    pVoipBuffer->bMinDiscard = TRUE;
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeBufferInit

    \Description
        Allocate and initialize voip buffer

    \Input *pVoipTranscribe     - module state
    \Input *pVoipBuffer         - buffer to initialize
    \Input iBufSize             - size of streaming buffer
    \Input iBuffer              - buffer index to set up

    \Output
        int32_t                 - zero=failure, else success

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeBufferInit(VoipTranscribeRefT *pVoipTranscribe, VoipBufferT *pVoipBuffer, int32_t iBufSize, int32_t iBuffer)
{
    pVoipBuffer->iBuffer = iBuffer;
    pVoipBuffer->iBufLen = iBufSize;
    _VoipTranscribeBufferReset(pVoipTranscribe, pVoipBuffer);
    return((pVoipBuffer->pBuffer = DirtyMemAlloc(iBufSize, VOIPTRANSCRIBE_MEMID, pVoipTranscribe->iMemGroup, pVoipTranscribe->pMemGroupUserData)) != NULL);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeCustomHeaderCb

    \Description
        Custom header callback used to sign AWS requests

    \Input *pState          - http module state
    \Input *pHeader         - pointer to http header buffer
    \Input uHeaderSize      - size of http header buffer
    \Input *pData           - pointer to data (unused)
    \Input iDataLen         - data length (unused)
    \Input *pUserRef        - voiptranscribe ref

    \Output
        int32_t             - output header length

    \Version 12/28/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeCustomHeaderCb(ProtoHttp2RefT *pState, char *pHeader, uint32_t uHeaderSize, const uint8_t *pData, int64_t iDataLen, void *pUserRef)
{
    VoipTranscribeRefT *pVoipTranscribe = (VoipTranscribeRefT *)pUserRef;
    int32_t iHdrLen = (int32_t)strlen(pHeader);
    
    // if we have room, sign the request
    if (uHeaderSize < (unsigned)iHdrLen)
    {
        return(iHdrLen);
    }

    // sign the request and return the updated size
    iHdrLen += AWSSignSigV4(pHeader, uHeaderSize, "", pVoipTranscribe->strKey, "transcribe", &pVoipTranscribe->AWSSignInfo);
    // return size to protohttp
    return(iHdrLen);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeTransportInit

    \Description
        Init transport module

    \Input *pVoipTranscribe - pointer to module state

    \Output
        int32_t             - negative=failure, else success

    \Version 09/17/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeTransportInit(VoipTranscribeRefT *pVoipTranscribe)
{
    TransportT *pTransport = &pVoipTranscribe->Transport;

    // init transport class
    pVoipTranscribe->eTransport = VOIPTRANSCRIBE_PROFILE_TRANSPORT(pVoipTranscribe->uProfile);
    if (pVoipTranscribe->eTransport == VOIPTRANSCRIBE_TRANSPORT_HTTP)
    {
        _TransportInit(pTransport, TRANSPORT_HTTP);
    }
    else if (pVoipTranscribe->eTransport == VOIPTRANSCRIBE_TRANSPORT_HTTP2)
    {
        _TransportInit(pTransport, TRANSPORT_HTTP2);
    }
    else if (pVoipTranscribe->eTransport == VOIPTRANSCRIBE_TRANSPORT_WEBSOCKETS)
    {
        _TransportInit(pTransport, TRANSPORT_WEBSOCKETS);
    }

    // allocate transport ref; give it a big enough buffer to max out SSL frame size
    if ((pTransport->pState = pTransport->Create(16*1024)) == NULL)
    {
        NetPrintf(("voiptranscribe: could not allocate transport module\n"));
        VoipTranscribeDestroy(pVoipTranscribe);
        return(-1);
    }

    // perform transport-specific initialization
    if (pTransport->eTransport == TRANSPORT_HTTP)
    {
        // don't request connection close
        pTransport->Control(pTransport->pState, pTransport->iStreamId, 'keep', 1, 0, NULL);
        // enable reuse on put/post
        pTransport->Control(pTransport->pState, pTransport->iStreamId, 'rput', 1, 0, NULL);
    }
    else if (pTransport->eTransport == TRANSPORT_WEBSOCKETS)
    {
        // increase temporary input buffer used for connection establishment to allow for header info
        pTransport->Control(pTransport->pState, pTransport->iStreamId, 'ires', 2*1024, 0, NULL);
    }

    // perform provider-specific initialization
    if (pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_AMAZON)
    {
        // set request header callback for AWS signing
        ProtoHttp2Callback(pTransport->pState, _VoipTranscribeCustomHeaderCb, NULL, pVoipTranscribe);
    }

    // set common transport parameters
    pVoipTranscribe->iTimeout = 60*1000;
    pTransport->Control(pTransport->pState, pTransport->iStreamId, 'time', pVoipTranscribe->iTimeout, 0, NULL);
    // return success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeTransportCleanup

    \Description
        Cleanup transport module

    \Input *pVoipTranscribe - pointer to module state

    \Version 12/13/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipTranscribeTransportCleanup(VoipTranscribeRefT *pVoipTranscribe)
{
    TransportT *pTransport = &pVoipTranscribe->Transport;

    // destroy previous transport ref, if allocated
    if (pTransport->pState != NULL)
    {
        NetPrintfVerbose((pVoipTranscribe->iVerbose, 1, "voiptranscribe: cleaning up previous transport state\n"));
        pTransport->Destroy(pTransport->pState);
    }

    // reset transport state
    ds_memclr(&pVoipTranscribe->Transport, sizeof(pVoipTranscribe->Transport));
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeBasicAuth

    \Description
        Encode Basic HTTP authorization header as per https://tools.ietf.org/html/rfc7617

    \Input *pBuffer     - [out] output buffer for encoded base64 string
    \Input iBufSize     - size of output buffer
    \Input *pUser       - user identifer
    \Input *pPass       - user password

    \Output
        const char *    - pointer to output buffer

    \Version 02/27/2019 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipTranscribeBasicAuth(char *pBuffer, int32_t iBufSize, const char *pUser, const char *pPass)
{
    char strAuth[128];
    ds_snzprintf(strAuth, sizeof(strAuth), "%s:%s", pUser, pPass);
    Base64Encode2(strAuth, (int32_t)strlen(strAuth), pBuffer, iBufSize);
    return(pBuffer);
}

/*F********************************************************************************/
/*!
\Function _VoipTranscribeParseResponseWatson

    \Description
        Parse response from IBM Watson transcription service

    \Input *pVoipTranscribe - pointer to module state
    \Input *pResponse       - server response
    \Input *pResult         - parse result buffer
    \Input iResultSize      - length of result buffer

    \Output
        int32_t             - negative=failure, zero=listening, else success

    \Notes
        A zero result indicates an intermediate response ("listening") that should
        be consumed while remaining in the receiving state.

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeParseResponseWatson(VoipTranscribeRefT *pVoipTranscribe, const char *pResponse, char *pResult, int32_t iResultSize)
{
    const char *pCurrent, *pAlt;
    uint16_t *pJsonParseBuf;
    int32_t iResult = -1;
    char strText[128], *pText;

    // parse the response
    if (JsonParse(pVoipTranscribe->aJsonParseBuf, sizeof(pVoipTranscribe->aJsonParseBuf)/sizeof(pVoipTranscribe->aJsonParseBuf[0]), pResponse, -1) == 0)
    {
        NetPrintf(("voiptranscribe: warning: parse results truncated\n"));
    }
    pJsonParseBuf = pVoipTranscribe->aJsonParseBuf;

    if ((pCurrent = JsonFind2(pJsonParseBuf, NULL, "results[", 0)) != NULL)
    {
        if ((pAlt = JsonFind2(pJsonParseBuf, pCurrent, ".alternatives[", 0)) != NULL)
        {
            JsonGetString(JsonFind2(pJsonParseBuf, pAlt, ".transcript", 0), pResult, iResultSize, "");

            /* as per https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-basic-response#hesitation, results
               can include %HESITATION in some circumstances.  we don't want that, so we remove it from the output if detected.
               note it seems that smart_formatting also removes it, but we leave this here in case that changes at some point */
            for (pText = pVoipTranscribe->strTranscription; (pText = ds_stristr(pText, "%HESITATION")) != NULL; )
            {
                iResult = (int32_t)strlen(pText)+1;
                memmove(pText, pText+12, iResult-12);
            }
        }
        iResult = 1;
    }
    else if ((pCurrent = JsonFind(pJsonParseBuf, "state")) != NULL)
    {
        JsonGetString(pCurrent, strText, sizeof(strText), "");
        if (!strcmp(strText, "listening"))
        {
            NetPrintfVerbose((pVoipTranscribe->iVerbose, 1, "voiptranscribe: state: listening\n"));
            iResult = 0;
        }
    }
    else
    {
        *pResult = '\0';
        if ((pCurrent = JsonFind(pJsonParseBuf, "error")) != NULL)
        {
            JsonGetString(pCurrent, pResult, iResultSize, "");
            // if a timeout, don't consider it an error
            if (!ds_stricmp(pVoipTranscribe->strTranscription, "Session timed out."))
            {
                pVoipTranscribe->strTranscription[0] = '\0';
                iResult = 0;
            }
        }
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeParseResponseMicrosoft

    \Description
        Parse response from Microsoft Speech transcription service

    \Input *pVoipTranscribe - pointer to module state
    \Input *pResponse       - server response
    \Input *pResult         - parse result buffer
    \Input iResultSize      - length of result buffer

    \Output
        int32_t             - negative=failure, else success

    \Notes
        RecognitionStatus: Success, NoMatch, InitialSilenceTimeout, BabbleTimeout, Error

        DisplayText represents the recognized phrase after capitalization, punctuation, and
        inverse-text-normalization have been applied and profanity has been masked with
        asterisks. The DisplayText field is present only if the RecognitionStatus field has
        the value Success.

        Ref: https://docs.microsoft.com/en-us/azure/cognitive-services/speech/concepts#transcription-responses

    \Version 09/15/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeParseResponseMicrosoft(VoipTranscribeRefT *pVoipTranscribe, const char *pResponse, char *pResult, int32_t iResultSize)
{
    int32_t iResult = -1;
    const char *pCurrent;
    char strText[128];
    uint16_t *pJsonParseBuf;

    // parse the response
    if (JsonParse(pVoipTranscribe->aJsonParseBuf, sizeof(pVoipTranscribe->aJsonParseBuf)/sizeof(pVoipTranscribe->aJsonParseBuf[0]), pResponse, -1) == 0)
    {
        NetPrintf(("voiptranscribe: warning: parse results truncated\n"));
    }
    pJsonParseBuf = pVoipTranscribe->aJsonParseBuf;

    // get status
    if ((pCurrent = JsonFind2(pJsonParseBuf, NULL, "RecognitionStatus", 0)) != NULL)
    {
        JsonGetString(pCurrent, strText, sizeof(strText), "");
        NetPrintfVerbose((pVoipTranscribe->iVerbose, 1, "voiptranscribe: RecognitionStatus=%s\n", strText));
        if (strcmp(strText, "Error"))
        {
            iResult = 1;
        }
    }
    // get display text
    if ((pCurrent = JsonFind2(pJsonParseBuf, NULL, "DisplayText", 0)) != NULL)
    {
        JsonGetString(pCurrent, pResult, iResultSize, "");
        iResult = 1;
    }

    // return result to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeParseResponseGoogleJson

    \Description
        Parse response from Google Cloud transcription service

    \Input *pVoipTranscribe - pointer to module state
    \Input *pResponse       - server response
    \Input *pResult         - parse result buffer
    \Input iResultSize      - length of result buffer

    \Output
        int32_t             - negative=failure, else success

    \Notes
        Ref: https://cloud.google.com/speech-to-text/docs/reference/rpc/google.cloud.speech.v1#google.cloud.speech.v1.StreamingRecognizeResponse

    \Version 09/27/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeParseResponseGoogleJson(VoipTranscribeRefT *pVoipTranscribe, const char *pResponse, char *pResult, int32_t iResultSize)
{
    const char *pCurrent, *pAlt;
    uint16_t *pJsonParseBuf;
    int32_t iResult = -1;

    // parse the response
    if (JsonParse(pVoipTranscribe->aJsonParseBuf, sizeof(pVoipTranscribe->aJsonParseBuf)/sizeof(pVoipTranscribe->aJsonParseBuf[0]), pResponse, -1) == 0)
    {
        NetPrintf(("voiptranscribe: warning: parse results truncated\n"));
    }
    pJsonParseBuf = pVoipTranscribe->aJsonParseBuf;

    // check for transcript result
    if ((pCurrent = JsonFind2(pJsonParseBuf, NULL, "results[", 0)) != NULL)
    {
        if ((pAlt = JsonFind2(pJsonParseBuf, pCurrent, ".alternatives[", 0)) != NULL)
        {
            JsonGetString(JsonFind2(pJsonParseBuf, pAlt, ".transcript", 0), pResult, iResultSize, "");
        }
        iResult = 1;
    }
    // process error, if there is one
    else if ((pCurrent = JsonFind2(pJsonParseBuf, NULL, "error", 0)) != NULL)
    {
        char strText[128];
        int32_t iCode = JsonGetInteger(JsonFind2(pJsonParseBuf, pCurrent, ".code", 0), 0);
        JsonGetString(JsonFind2(pJsonParseBuf, pCurrent, ".message", 0), strText, sizeof(strText), "");
        ds_snzprintf(pResult, iResultSize, "error %d (%s)", iCode, strText);
    }

    // return result to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeParseResponseGoogleProtobuf

    \Description
        Parse response from Google Cloud transcription service

    \Input *pVoipTranscribe - pointer to module state
    \Input *pResponse       - server response
    \Input iResponseSize    - server response length
    \Input *pResult         - parse result buffer
    \Input iResultSize      - length of result buffer

    \Output
        int32_t             - negative=failure, else success

    \Notes
        See file header for response format and protobuf definition reference.

    \Version 10/02/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeParseResponseGoogleProtobuf(VoipTranscribeRefT *pVoipTranscribe, const char *pResponse, int32_t iResponseSize, char *pResult, int32_t iResultSize)
{
    ProtobufReadT Reader, Msg, Msg2;
    const uint8_t *pCurrent = NULL, *pCurrent2 = NULL;
    const uint8_t *pBuffer = (const uint8_t *)pResponse;
    int32_t iMsgSize, iResult=-1;

    // an empty response means the audio produced no transcription; this indicates the request is complete, so we return completion
    if (iResponseSize == 0)
    {
        return(1);
    }

    // get message size (skipping compression)
    if ((pBuffer = ProtobufCommonReadSize(pBuffer+1, iResponseSize-1, &iMsgSize)) == NULL)
    {
        return(iResult);
    }
    ProtobufReadInit(&Reader, pBuffer, iMsgSize);

    // pull out the error info if included
    if (ProtobufReadMessage(&Reader, ProtobufReadFind(&Reader, 1 /* error */), &Msg) != NULL)
    {
        char strText[128];
        int32_t iCode = ProtobufReadVarint(&Msg, ProtobufReadFind(&Msg, 1 /* code */));
        ProtobufReadString(&Msg, ProtobufReadFind(&Msg, 2 /* message */), strText, sizeof(strText));
        ds_snzprintf(pResult, iResultSize, "error %d (%s)", iCode, strText);
    }

    // read repeated results
    if ((pCurrent = ProtobufReadMessage(&Reader, ProtobufReadFind2(&Reader, 2 /* results */, pCurrent), &Msg)) != NULL)
    {
        // read repeated alternatives
        if ((pCurrent2 = ProtobufReadMessage(&Msg, ProtobufReadFind2(&Msg, 1 /* alternatives */, pCurrent2), &Msg2)) != NULL)
        {
            ProtobufReadString(&Msg2, ProtobufReadFind(&Msg2, 1 /* transcript */), pResult, iResultSize);
            iResult = 1;
        }
    }

    // return result to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeParseResponseAmazonJson

    \Description
        Parse JSON response from Amazon Transcribe service, after being extracted from
        binary event.

    \Input *pVoipTranscribe - pointer to module state
    \Input *pResponse       - server response
    \Input *pResult         - parse result buffer
    \Input iResultSize      - length of result buffer

    \Output
        int32_t             - negative=failure, zero=listening, else success

    \Version 01/17/2019 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeParseResponseAmazonJson(VoipTranscribeRefT *pVoipTranscribe, const char *pResponse, char *pResult, int32_t iResultSize)
{
    const char *pCurrent, *pAlt;
    uint16_t *pJsonParseBuf;
    int32_t iResult = -1;

    // parse the response
    if (JsonParse(pVoipTranscribe->aJsonParseBuf, sizeof(pVoipTranscribe->aJsonParseBuf)/sizeof(pVoipTranscribe->aJsonParseBuf[0]), pResponse, -1) == 0)
    {
        NetPrintf(("voiptranscribe: warning: parse results truncated\n"));
    }
    pJsonParseBuf = pVoipTranscribe->aJsonParseBuf;

    // check for transcript response
    if ((pCurrent = JsonFind2(pJsonParseBuf, NULL, "Transcript.Results[", 0)) != NULL)
    {
        // swallow intermediate/empty results
        iResult = 0;
        // check for completion
        if (((pAlt = JsonFind2(pJsonParseBuf, pCurrent, ".IsPartial", 0)) != NULL) && !JsonGetBoolean(pAlt, FALSE))
        {
            if ((pAlt = JsonFind2(pJsonParseBuf, pCurrent, ".Alternatives[", 0)) != NULL)
            {
                JsonGetString(JsonFind2(pJsonParseBuf, pAlt, ".Transcript", 0), pResult, iResultSize, "");
            }
            iResult = 1;
        }
    }
    else if ((pCurrent = JsonFind2(pJsonParseBuf, NULL, "Message", 0)) != NULL)
    {
        // get error message result
        JsonGetString(pCurrent, pResult, iResultSize, "");
    }

    // return result to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeParseResponseAmazon

    \Description
        Parse binary event response from Amazon

    \Input *pVoipTranscribe - pointer to module state
    \Input *pResponse       - server response
    \Input iResponseSize    - server response length
    \Input *pResult         - parse result buffer
    \Input iResultSize      - length of result buffer

    \Output
        int32_t             - negative=failure, else success

    \Version 01/17/2019 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeParseResponseAmazon(VoipTranscribeRefT *pVoipTranscribe, const char *pResponse, int32_t iResponseSize, char *pResult, int32_t iResultSize)
{
    char strEventType[32], strHeader[512], strMessage[4096];
    int32_t iMessageLen, iOffset, iReadResult, iResult;
    TransportT *pTransport = &pVoipTranscribe->Transport;

    /* get session id from response header, to use in future requests; amazon recommends this as it can improve
       transcription accuracy across requests */
    if (pTransport->Status(pTransport->pState, pTransport->iStreamId, 'htxt', strHeader, sizeof(strHeader)) != -1)
    {
        ProtoHttpGetHeaderValue(NULL, strHeader, "x-amzn-transcribe-session-id", pVoipTranscribe->strSessionId, sizeof(pVoipTranscribe->strSessionId), NULL);
    }
    // parse error response
    if ((iResult = pTransport->Status(pTransport->pState, pTransport->iStreamId, 'code', strHeader, sizeof(strHeader))) != PROTOHTTP_RESPONSE_OK)
    {
        NetPrintfVerbose((pVoipTranscribe->iVerbose, 0, "voiptranscribe: received %d result\n", iResult));
    }
    // read binary events from response data
    for (iOffset = 0, iResult = 0; (iOffset < iResponseSize) && (iResult == 0); iOffset += iReadResult)
    {
        if ((iReadResult = AWSReadEvent((const uint8_t *)pResponse+iOffset, iResponseSize-iOffset, strEventType, sizeof(strEventType), strMessage, (iMessageLen=(int32_t)sizeof(strMessage), &iMessageLen))) > 0)
        {
            if (!ds_stricmp(strEventType, "TranscriptEvent"))
            {
                NetPrintfVerbose((pVoipTranscribe->iVerbose, 1, "voiptranscribe: %s\n", strMessage));
                iResult = _VoipTranscribeParseResponseAmazonJson(pVoipTranscribe, strMessage, pResult, iResultSize);
            }
            if (!ds_stricmp(strEventType, "BadRequestException"))
            {
                NetPrintfVerbose((pVoipTranscribe->iVerbose, 0, "voiptranscribe: BadRequestException:\n%s\n", strMessage));
                iResult = _VoipTranscribeParseResponseAmazonJson(pVoipTranscribe, strMessage, pResult, iResultSize);
            }
        }
        else
        {
            break;
        }
    }

    return((iResult >= 0) ? 1 : -1);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeParseResponse

    \Description
        Parse response from transcription service

    \Input *pVoipTranscribe - pointer to module state
    \Input *pResponse       - server response
    \Input iResponseSize    - server response length
    \Input *pResult         - parse result buffer
    \Input iResultSize      - length of result buffer

    \Output
        int32_t             - negative=failure, zero=continue receiving, else success

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeParseResponse(VoipTranscribeRefT *pVoipTranscribe, const char *pResponse, int32_t iResponseSize, char *pResult, int32_t iResultSize)
{
    int32_t iResult = -1;
    if (pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_IBMWATSON)
    {
        iResult = _VoipTranscribeParseResponseWatson(pVoipTranscribe, pResponse, pResult, iResultSize);
    }
    else if (pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_MICROSOFT)
    {
        iResult = _VoipTranscribeParseResponseMicrosoft(pVoipTranscribe, pResponse, pResult, iResultSize);
    }
    else if (pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_GOOGLE)
    {
        iResult = (pVoipTranscribe->eTransport == VOIPTRANSCRIBE_TRANSPORT_HTTP) ? _VoipTranscribeParseResponseGoogleJson(pVoipTranscribe, pResponse, pResult, iResultSize) : _VoipTranscribeParseResponseGoogleProtobuf(pVoipTranscribe, pResponse, iResponseSize, pResult, iResultSize);
    }
    else if (pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_AMAZON)
    {
        iResult = _VoipTranscribeParseResponseAmazon(pVoipTranscribe, pResponse, iResponseSize, pResult, iResultSize);
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeFormatHeaderWatson

    \Description
        Format connection header for Watson service

    \Input *pVoipTranscribe - pointer to module state
    \Input *pBuffer         - [out] buffer to hold formatted header 
    \Input iBufLen          - buffer length

    \Output
        int32_t             - negative=failure, else success

    \Version 09/17/2018 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipTranscribeFormatHeaderWatson(VoipTranscribeRefT *pVoipTranscribe, char *pBuffer, int32_t iBufLen)
{
    TransportT *pTransport = &pVoipTranscribe->Transport;
    char strAuth[128];
    int32_t iOffset;

    /* note: pre-encoded auth strings are 68 characters in length, the auth keys are 44 chars.  we use this to decide whether
       to do the encode or not.  this code should be removed in the future once pre-encoded auth keys are no longer in use */

    // encode Basic authorization string with string apikey:<key>
    if (strlen(pVoipTranscribe->strKey) < 68)
    {
        _VoipTranscribeBasicAuth(strAuth, sizeof(strAuth), "apikey", pVoipTranscribe->strKey);
    }
    else // just copy it
    {
        ds_strnzcpy(strAuth, pVoipTranscribe->strKey, sizeof(strAuth));
    }

    // format request header
    iOffset = ds_snzprintf(pBuffer, iBufLen, "Authorization: Basic %s\r\n", strAuth);
    // set http-specific options
    if (pTransport->eTransport == TRANSPORT_HTTP)
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufLen-iOffset, "Content-Type: %s\r\n", pVoipTranscribe->strAudioFormat);
    }
    // return transport-specific url
    return(pVoipTranscribe->strUrl);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeFormatHeaderMicrosoft

    \Description
        Format connection header for Microsoft Speech service

    \Input *pVoipTranscribe - pointer to module state
    \Input *pBuffer         - [out] buffer to hold formatted header
    \Input iBufLen          - buffer length

    \Output
        int32_t             - negative=failure, else success

    \Version 09/17/2018 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipTranscribeFormatHeaderMicrosoft(VoipTranscribeRefT *pVoipTranscribe, char *pBuffer, int32_t iBufLen)
{
    TransportT *pTransport = &pVoipTranscribe->Transport;
    int32_t iOffset=0;

    // set http-specific options
    if (pTransport->eTransport == TRANSPORT_HTTP)
    {
        // format request header
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufLen-iOffset, "Accept: application/json;text/xml\r\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufLen-iOffset, "Content-Type: %s\r\n", pVoipTranscribe->strAudioFormat);
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufLen-iOffset, "Ocp-Apim-Subscription-Key: %s\r\n", pVoipTranscribe->strKey);
    }
    // set websockets-specific options
    else if (pTransport->eTransport == TRANSPORT_WEBSOCKETS)
    {
        char strUUID[36], strTimestamp[36];
        // get a UUID
        _GenerateUUID(strUUID, sizeof(strUUID), FALSE);
        _GenerateTimestamp(strTimestamp, sizeof(strTimestamp));
        // format request header
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufLen-iOffset, "X-ConnectionId: %s\r\n", strUUID);
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufLen-iOffset, "X-Timestamp: %s\r\n", strTimestamp);
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufLen-iOffset, "Content-Type: %s\r\n", pVoipTranscribe->strAudioFormat);
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufLen-iOffset, "Ocp-Apim-Subscription-Key: %s\r\n", pVoipTranscribe->strKey);
    }
    // return transport-specific url
    return(pVoipTranscribe->strUrl);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeFormatHeaderGoogle

    \Description
        Format connection header for Google service

    \Input *pVoipTranscribe - pointer to module state
    \Input *pBuffer         - [out] buffer to hold formatted header 
    \Input iBufLen          - buffer length

    \Output
        int32_t             - negative=failure, else success

    \Version 09/18/2018 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipTranscribeFormatHeaderGoogle(VoipTranscribeRefT *pVoipTranscribe, char *pBuffer, int32_t iBufLen)
{
    TransportT *pTransport = &pVoipTranscribe->Transport;
    static char strUrl[256] = "";
    const char *pUrl;
    
    // format url with api key
    if (pTransport->eTransport == TRANSPORT_HTTP)
    {
        ds_snzprintf(strUrl, sizeof(strUrl), "%s?key=%s", pVoipTranscribe->strUrl, pVoipTranscribe->strKey);
        pUrl = strUrl;
    }
    else
    {
        pUrl = pVoipTranscribe->strUrl;
        ds_snzprintf(pBuffer, iBufLen, "te: trailers\r\ncontent-type: application/grpc\r\nX-Goog-Api-Key: %s\r\n", pVoipTranscribe->strKey);
    }
    // return url
    return(pUrl);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeFormatHeaderAmazon

    \Description
        Format connection header for Amazon Transcribe service

    \Input *pVoipTranscribe - pointer to module state
    \Input *pBuffer         - [out] buffer to hold formatted header 
    \Input iBufLen          - buffer length

    \Output
        int32_t             - negative=failure, else success

    \Version 09/17/2018 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipTranscribeFormatHeaderAmazon(VoipTranscribeRefT *pVoipTranscribe, char *pBuffer, int32_t iBufLen)
{
    int32_t iOffset=0;

    // format request header
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufLen-iOffset, "content-type: application/x-amz-json-1.1\r\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufLen-iOffset, "x-amzn-content-sha256: STREAMING-AWS4-HMAC-SHA256-EVENTS\r\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufLen-iOffset, "x-amzn-target: com.amazonaws.transcribe.Transcribe.StartStreamTranscription\r\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufLen-iOffset, "x-amzn-transcribe-language-code: en-US\r\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufLen-iOffset, "x-amzn-transcribe-media-encoding: pcm\r\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufLen-iOffset, "x-amzn-transcribe-sample-rate: %d\r\n", pVoipTranscribe->iAudioRate);
    if (pVoipTranscribe->strSessionId[0] != '\0')
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufLen-iOffset, "x-amzn-transcribe-session-id: %s\r\n", pVoipTranscribe->strSessionId);
    }

    // return transport-specific url
    return(pVoipTranscribe->strUrl);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeSetHeader

    \Description
        Set connection header; note that this might be used in the Connect() or
        Request() call depending on whether we are using a connection-oriented
        protocol or not.

    \Input *pVoipTranscribe - pointer to module state
    \Input *pTransport      - transport handler

    \Output
        int32_t             - negative=failure, else success

    \Version 09/17/2018 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipTranscribeSetHeader(VoipTranscribeRefT *pVoipTranscribe, TransportT *pTransport)
{
    char strHeader[512] = "";
    const char *pUrl = NULL;
    // format header with provider-specific fields
    if (pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_IBMWATSON)
    {
        pUrl = _VoipTranscribeFormatHeaderWatson(pVoipTranscribe, strHeader, sizeof(strHeader));
    }
    else if (pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_MICROSOFT)
    {
        pUrl = _VoipTranscribeFormatHeaderMicrosoft(pVoipTranscribe, strHeader, sizeof(strHeader));
    }
    else if (pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_GOOGLE)
    {
        pUrl = _VoipTranscribeFormatHeaderGoogle(pVoipTranscribe, strHeader, sizeof(strHeader));
    }
    else if (pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_AMAZON)
    {
        pUrl = _VoipTranscribeFormatHeaderAmazon(pVoipTranscribe, strHeader, sizeof(strHeader));
    }
    // set the header
    pTransport->Control(pTransport->pState, pTransport->iStreamId, 'apnd', 0, 0, strHeader);
    // return request header
    return(pUrl);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeConnectCheck

    \Description
        Check for connection completion, for protocols that require an explicit connection

    \Input *pVoipTranscribe - pointer to module state
    \Input *pTransport      - transport handler

    \Output
        int32_t             - negative=failure, zero=connecting, else success

    \Version 09/06/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeConnectCheck(VoipTranscribeRefT *pVoipTranscribe, TransportT *pTransport)
{
    int32_t iResult = (pTransport->eTransport == TRANSPORT_WEBSOCKETS) ? pTransport->Status(pTransport->pState, pTransport->iStreamId, 'stat', NULL, 0) : 1;
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeConnect

    \Description
        Open a connection to a transcription service, if we're not already connected

    \Input *pVoipTranscribe - pointer to module state

    \Output
        int32_t             - negative=failure, else success

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeConnect(VoipTranscribeRefT *pVoipTranscribe)
{
    TransportT *pTransport = &pVoipTranscribe->Transport;
    const char *pUrl;
    int32_t iResult;

    // up the logging level
    pTransport->Control(pTransport->pState, pTransport->iStreamId, 'spam', 1, 0, NULL);

    // early out if we're already connected or don't need to connect
    if ((iResult = _VoipTranscribeConnectCheck(pVoipTranscribe, pTransport)) > 0)
    {
        return(1);
    }
    // set connect headers
    if ((pUrl = _VoipTranscribeSetHeader(pVoipTranscribe, pTransport)) == NULL)
    {
        return(-1);
    }

    // make the connection request
    NetPrintfVerbose((pVoipTranscribe->iVerbose, 1, "voiptranscribe: connecting to %s\n", pUrl));
    if ((iResult = pTransport->Connect(pTransport->pState, pUrl)) < 0)
    {
        NetPrintf(("voiptranscribe: error connecting to '%s'\n", pUrl));
        return(iResult);
    }

    // return result code to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeRequest

    \Description
        Make a request against transcription service

    \Input *pVoipTranscribe - pointer to module state

    \Output
        int32_t             - negative=failure, else success

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeRequest(VoipTranscribeRefT *pVoipTranscribe)
{
    TransportT *pTransport = &pVoipTranscribe->Transport;
    char strRequest[128] = "", *pRequest = strRequest;
    int32_t iRequestLen=0, iResult;
    const char *pUrl;

    // set request headers
    if ((pUrl = _VoipTranscribeSetHeader(pVoipTranscribe, pTransport)) == NULL)
    {
        return(-1);
    }

    // set content-type for watson+websockets
    if ((pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_IBMWATSON) && (pTransport->eTransport == TRANSPORT_WEBSOCKETS))
    {
        // format websocket request body
        iRequestLen = ds_snzprintf(strRequest, sizeof(strRequest), "{ \"action\": \"start\", \"content-type\": \"%s\", \"smart_formatting\": true }", pVoipTranscribe->strAudioFormat);
    }
    
    // http transfers are streaming (use chunked encoding)
    if (pTransport->eTransport == TRANSPORT_HTTP)
    {
        iRequestLen = PROTOHTTP_STREAM_BEGIN;
    }

    // start the request
    NetPrintfVerbose((pVoipTranscribe->iVerbose, 1, "voiptranscribe: sending request\n"));
    if ((iResult = pTransport->Request(pTransport->pState, pUrl, pRequest, iRequestLen, &pTransport->iStreamId)) < 0)
    {
        NetPrintf(("voiptranscribe: error %d issuing request'%s'\n", iResult, pUrl));
        return(iResult);
    }

    // return result code to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeSubmitRaw

    \Description
        Submit uncompressed voice data to be transcribed

    \Input *pVoipTranscribe - pointer to module state
    \Input *pVoipBuffer     - buffer to write to
    \Input *pBuffer         - voice data to be transcribed
    \Input iBufLen          - size of voice data in bytes

    \Output
        int32_t             - number of bytes copied

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t _VoipTranscribeSubmitRaw(VoipTranscribeRefT *pVoipTranscribe, VoipBufferT *pVoipBuffer, const uint8_t *pBuffer, int32_t iBufLen)
{
    // start of buffer processing
    if (pVoipBuffer->bRecStarting)
    {
        // reserve a WAV header to encapsulate the data
        if (pVoipTranscribe->eFormat == VOIPTRANSCRIBE_FORMAT_WAV16)
        {
            pVoipBuffer->iBufOff = _WaveWriteOpen(pVoipBuffer, pVoipTranscribe->iAudioRate);
        }
        pVoipBuffer->bRecStarting = FALSE;
    }

    // copy data to output buffer
    ds_memcpy(pVoipBuffer->pBuffer+pVoipBuffer->iBufOff, pBuffer, iBufLen);

    // adjust buffer parameters
    pVoipBuffer->iBufOff += iBufLen;

    // note if we're full
    if (pVoipBuffer->iBufOff == pVoipBuffer->iBufLen)
    {
        pVoipBuffer->bRecFull = TRUE;
    }

    // return amount copied to caller
    return(iBufLen);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeSubmitOpus

    \Description
        Submit Opus voice data to be transcribed

    \Input *pVoipTranscribe - pointer to module state
    \Input *pVoipBuffer     - buffer to write to
    \Input *pBuffer         - voice data to be transcribed
    \Input iBufLen          - size of voice data in bytes

    \Output
        int32_t             - number of bytes copied

    \Version 09/10/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t _VoipTranscribeSubmitOpus(VoipTranscribeRefT *pVoipTranscribe, VoipBufferT *pVoipBuffer, const uint8_t *pBuffer, int32_t iBufLen)
{
    OggWriterT *pOggWriter = &pVoipBuffer->OggWriter;
    int32_t iResult;

    // if we're at the start of the buffer, reserve an ogg header to encapsulate the opus data
    if (pVoipBuffer->bRecStarting)
    {
        pVoipBuffer->iBufOff = _OggOpusWriteOpen(pOggWriter, pVoipBuffer->pBuffer, pVoipBuffer->iBufLen);
        pVoipBuffer->bRecStarting = FALSE;
    }

    // write voice bundle as an ogg segment
    if ((iResult = _OggWriteSegment(pOggWriter, pBuffer, iBufLen, pVoipTranscribe->iVerbose)) > 0)
    {
        pVoipBuffer->iBufOff = iResult;
    }
    else if (iResult < 0)
    {
        pVoipBuffer->bRecFull = TRUE;
    }

    // return amount copied to caller
    return(iBufLen);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeSubmit

    \Description
        Submit voice data to be transcribed

    \Input *pVoipTranscribe - pointer to module state
    \Input *pBuffer         - voice data to be transcribed
    \Input iBufLen          - size of voice data in bytes

    \Output
        int32_t             - number of bytes copied

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeSubmit(VoipTranscribeRefT *pVoipTranscribe, const uint8_t *pBuffer, int32_t iBufLen)
{
    VoipBufferT *pVoipBuffer = &pVoipTranscribe->VoipBuffer[pVoipTranscribe->iRecBuffer];
    int32_t iBufAvail = pVoipBuffer->iBufLen - pVoipBuffer->iBufOff;
    int32_t iResult;

    // determine amount of data to copy
    if (iBufLen > iBufAvail)
    {
        NetPrintf(("voiptranscribe: [%d] warning; truncating input from %d to %d bytes\n", pVoipBuffer->iBuffer, iBufLen, iBufAvail));
        iBufLen = iBufAvail;
    }
    // make sure we have something to submit
    if (iBufLen == 0)
    {
        return(0);
    }

    NetPrintfVerbose((pVoipTranscribe->iVerbose, 3, "voiptranscribe: [%d] copy [0x%04x,0x%04x]\n", pVoipBuffer->iBuffer, pVoipBuffer->iBufOff, pVoipBuffer->iBufOff+iBufLen));

    // submit data to buffer
    if (!pVoipTranscribe->bCompressed)
    {
        iResult = _VoipTranscribeSubmitRaw(pVoipTranscribe, pVoipBuffer, pBuffer, iBufLen);
    }
    else
    {
        iResult = _VoipTranscribeSubmitOpus(pVoipTranscribe, pVoipBuffer, pBuffer, iBufLen);
    }

    // keep track of samples submitted; if compressed assume 20ms of samples at 16khz
    if (iResult > 0)
    {
        pVoipBuffer->iNumSamples += !pVoipTranscribe->bCompressed ? iBufLen/2 : 320;
    }

    // update voip timestamp
    pVoipTranscribe->uVoipTick = NetTick();

    // return result to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeSubmitFinish

    \Description
        Finish processing of data submission

    \Input *pVoipTranscribe - pointer to module state
    \Input *pVoipBuffer     - buffer to write to

    \Output
        int32_t             - negative=skip, else process

    \Version 09/13/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeSubmitFinish(VoipTranscribeRefT *pVoipTranscribe, VoipBufferT *pVoipBuffer)
{
    // if we have less than a second of audio don't send it
    if ((pVoipBuffer->iNumSamples < pVoipTranscribe->iAudioRate) && pVoipBuffer->bMinDiscard)
    {
        NetPrintf(("voiptranscribe: [%d] discarding short audio segment with only %d samples\n", pVoipBuffer->iBuffer, pVoipBuffer->iNumSamples));
        _VoipTranscribeBufferReset(pVoipTranscribe, pVoipBuffer);
        return(-1);
    }
    NetPrintfVerbose((pVoipTranscribe->iVerbose, 1, "voiptranscribe: [%d] submit finish\n", pVoipBuffer->iBuffer));
    
    // record metrics
    pVoipTranscribe->Metrics.uEventCount += 1;
    pVoipTranscribe->Metrics.uDurationMsSent += ((pVoipBuffer->iNumSamples * 1000) / pVoipTranscribe->iAudioRate);
    pVoipTranscribe->uSttStartTime = NetTick();

    // handle specific audio format requirements
    if (pVoipTranscribe->eFormat == VOIPTRANSCRIBE_FORMAT_OPUS)
    {
        pVoipBuffer->iBufOff = _OggOpusWriteFinish(&pVoipBuffer->OggWriter, pVoipTranscribe->iVerbose);
    }

    // finalize current buffer
    pVoipBuffer->bRecFinished = TRUE;
    // set current buffer as send buffer
    pVoipTranscribe->iSndBuffer = pVoipTranscribe->iRecBuffer;
    // move to next recording buffer
    pVoipTranscribe->iRecBuffer = (pVoipTranscribe->iRecBuffer+1)%2;
    // reset the buffer
    _VoipTranscribeBufferReset(pVoipTranscribe, &pVoipTranscribe->VoipBuffer[pVoipTranscribe->iRecBuffer]);

    // return success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeBackoffSet

    \Description
        Set backoff timer on failure or empty result, if appropriate

    \Input *pVoipTranscribe - pointer to module state

    \Version 12/05/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipTranscribeBackoffSet(VoipTranscribeRefT *pVoipTranscribe)
{
    int32_t iCount, iEmptyCt, iErrorCt;
    // see if we need to set the backoff timer
    iEmptyCt = pVoipTranscribe->iConsecEmptyCt - pVoipTranscribe->iConsecEmptyMax;
    iErrorCt = pVoipTranscribe->iConsecErrorCt - pVoipTranscribe->iConsecErrorMax;
    // pick the biggest of the two
    iCount = DS_MAX(iEmptyCt, iErrorCt);
    // if positive, calculate backoff timer
    if (iCount > 0)
    {
        // 2^n backoff on failures above the max
        iCount = (1 << iCount) * 1000;
        // clamp to maximum of sixty seconds
        iCount = DS_MIN(iCount, 60*1000);
        // set the backoff timer and make sure it doesn't equal zero (reserved for disabled status)
        if ((pVoipTranscribe->uBackoffTimer = NetTick()+iCount) == 0)
        {
            pVoipTranscribe->uBackoffTimer = 1;
        }

        NetPrintfVerbose((pVoipTranscribe->iVerbose, 0, "voiptranscribe: setting backoff timer to +%dms\n", iCount));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeBackoffCheck

    \Description
        Get if backoff is enabled

    \Input *pVoipTranscribe - pointer to module state
    \Input *pVoipBuffer     - voip buffer

    \Output
        int32_t             - zero if backoff is enabled, else one

    \Version 12/05/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeBackoffCheck(VoipTranscribeRefT *pVoipTranscribe, VoipBufferT *pVoipBuffer)
{
    if (pVoipTranscribe->uBackoffTimer != 0)
    {
        NetPrintfVerbose((pVoipTranscribe->iVerbose, 0, "voiptranscribe: [%d] discarding audio segment with %d samples due to backoff\n", pVoipBuffer->iBuffer, pVoipBuffer->iNumSamples));
        _VoipTranscribeBufferReset(pVoipTranscribe, pVoipBuffer);
    }
    return(pVoipTranscribe->uBackoffTimer != 0);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeSendFinish

    \Description
        Complete the send request

    \Input *pVoipTranscribe - pointer to module state

    \Output
        int32_t             - negative=failure, zero=retry, else success

    \Version 09/08/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeSendFinish(VoipTranscribeRefT *pVoipTranscribe)
{
    TransportT *pTransport = &pVoipTranscribe->Transport;
    char strRequest[128];
    int32_t iRequestLen, iResult = 0;

    if ((pTransport->eTransport == TRANSPORT_HTTP) || (pTransport->eTransport == TRANSPORT_HTTP2))
    {
        if ((iResult = pTransport->Send(pTransport->pState, pTransport->iStreamId, NULL, PROTOHTTP_STREAM_END)) == 0)
        {
            // a successful STREAM_END returns zero, we want to return nonzero so the caller knows the operation completed successfully
            iResult = 1;
        }
    }
    if (pTransport->eTransport == TRANSPORT_WEBSOCKETS)
    {
        iRequestLen =  (pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_IBMWATSON) ? ds_snzprintf(strRequest, sizeof(strRequest), "{ \"action\": \"stop\" }") : 0;
        iResult = _TransportWebSocketRequest(pTransport->pState, NULL, strRequest, iRequestLen, NULL);
    }

    // return result code to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeBase64Start

    \Description
        Format start of Base64 JSON envelope

    \Input *pVoipTranscribe - pointer to module state
    \Input *pVoipBuffer     - buffer to write to
    
    \Output
        int32_t             - size of output data

    \Version 12/16/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeBase64Start(VoipTranscribeRefT *pVoipTranscribe, VoipBufferT *pVoipBuffer)
{
    int32_t iResult = ds_snzprintf((char *)pVoipBuffer->pBuffer, pVoipBuffer->iBufLen, "{ \"config\": { \"encoding\": \"%s\", \"sampleRateHertz\": %d, \"languageCode\": \"en-US\", \"profanity_filter\": \"true\"  }, \"audio\": { \"content\": \"",
        pVoipTranscribe->strAudioFormat, pVoipTranscribe->iAudioRate);
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeBase64Encode

    \Description
        Base64 encode audio data

    \Input *pVoipBufferOut  - buffer to hold encoded output
    \Input *pVoipBufferInp  - buffer holding binary source data

    \Output
        int32_t             - length of encoded data

    \Version 12/16/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeBase64Encode(VoipBufferT *pVoipBufferOut, VoipBufferT *pVoipBufferInp)
{
    int32_t iInpLen = pVoipBufferInp->iBufOff-pVoipBufferInp->iBufInp;
    int32_t iOutLen = pVoipBufferOut->iBufLen-pVoipBufferOut->iBufOff-4; // save room for terminating "}} at end of base64 encoded input
                                                                         // pick smallest of input and output length
    iInpLen = DS_MIN(iInpLen, Base64DecodedSize(iOutLen-1));
    // make sure input length is a multiple of three; this ensures we have an integral output length with no padding
    if (iInpLen > 3)
    {
        iInpLen = (iInpLen/3)*3;
    }
    // encode into output buffer
    iOutLen = Base64Encode2((const char *)pVoipBufferInp->pBuffer+pVoipBufferInp->iBufInp, iInpLen, (char *)pVoipBufferOut->pBuffer+pVoipBufferOut->iBufOff, iOutLen);
    // update input buffer offset
    pVoipBufferInp->iBufInp += iInpLen;
    // return output buffer offset;
    return(iOutLen);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeProtobufStart

    \Description
        Format start of Protobuf envelope

    \Input *pVoipTranscribe - pointer to module state
    \Input *pVoipBuffer     - buffer to encode

    \Output
        int32_t             - number of bytes in encoded output

    \Notes
        See file header for request format and protobuf definition reference.

    \Version 10/02/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeProtobufStart(VoipTranscribeRefT *pVoipTranscribe, VoipBufferT *pVoipBuffer)
{
    static const uint8_t _aAudioFormatTypes[VOIPTRANSCRIBE_NUMFORMATS] = { 0xff, 1, 0xff, 6 };
    ProtobufWriteRefT *pEncoder;
    int32_t iSize=0;

    // write audio settings
    pVoipBuffer->pBuffer[0] = 0;
    if ((pEncoder = ProtobufWriteCreate(pVoipBuffer->pBuffer+1, pVoipBuffer->iBufLen-1, TRUE)) != NULL)
    {
        ProtobufWriteMessageBegin(pEncoder, 1 /* streaming_config */); 
        ProtobufWriteMessageBegin(pEncoder, 1 /* config */);
        ProtobufWriteVarint(pEncoder, _aAudioFormatTypes[pVoipTranscribe->eFormat], 1 /* encoding */);
        ProtobufWriteVarint(pEncoder, VOIPTRANSCRIBE_AUDIORATE, 2 /* sample_rate_hertz */);
        ProtobufWriteString(pEncoder, "en-US", (signed)strlen("en-US"), 3 /* language_code */);
        ProtobufWriteVarint(pEncoder, TRUE, 5 /* profanity_filter */);
        ProtobufWriteMessageEnd(pEncoder);
        ProtobufWriteMessageEnd(pEncoder);
        iSize = ProtobufWriteDestroy(pEncoder) + 1;
    }

    // return size to caller
    return(iSize);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeProtobufEncode

    \Description
        Protobuf encode audio data

    \Input *pVoipBufferOut  - buffer to hold encoded output
    \Input *pVoipBufferInp  - buffer holding binary source data

    \Output
        int32_t             - number of bytes in encoded output

    \Notes
        See file header for request format and protobuf definition reference.

    \Version 10/02/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeProtobufEncode(VoipBufferT *pVoipBufferOut, VoipBufferT *pVoipBufferInp)
{
    ProtobufWriteRefT *pEncoder;
    int32_t iBufAvail, iBufWrite, iSize=0;
    const int32_t _iMaxProtobufOverhead = 1+4+1+2; // compression byte+protobuf length+audio field tag+audio data size (progressive encoded max 2048)
    // early out if no data available or buffer full
    if ((pVoipBufferInp->iBufInp == pVoipBufferInp->iBufOff) || (pVoipBufferOut->iBufOff == pVoipBufferOut->iBufLen))
    {
        return(0);
    }
    // calculate output buffer space available
    iBufAvail = pVoipBufferOut->iBufLen-pVoipBufferOut->iBufOff;
    // calculate how much we're going to write (min of available output buffer minus overhead and available input data)
    iBufWrite = DS_MIN(iBufAvail-_iMaxProtobufOverhead, pVoipBufferInp->iBufOff-pVoipBufferInp->iBufInp);
    // write audio data
    pVoipBufferOut->pBuffer[pVoipBufferOut->iBufOff] = 0;
    if ((pEncoder = ProtobufWriteCreate(pVoipBufferOut->pBuffer+pVoipBufferOut->iBufOff+1, iBufAvail, TRUE)) != NULL)
    {
        ProtobufWriteBytes(pEncoder, pVoipBufferInp->pBuffer+pVoipBufferInp->iBufInp, iBufWrite, 2 /* audio_content */); 
        iSize = ProtobufWriteDestroy(pEncoder) + 1;
        // update input buffer offset
        pVoipBufferInp->iBufInp += iBufWrite;
    }
    // return size to caller
    return(iSize);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeAwsEncode

    \Description
        AWS encode audio data in signed binary event format

    \Input *pVoipTranscribe - module state
    \Input *pVoipBufferOut  - buffer to hold encoded output
    \Input *pVoipBufferInp  - buffer holding binary source data (NULL to write empty chunk)

    \Output
        int32_t             - number of bytes in encoded output

    \Version 01/16/2019 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeAwsEncode(VoipTranscribeRefT *pVoipTranscribe, VoipBufferT *pVoipBufferOut, VoipBufferT *pVoipBufferInp)
{
    int32_t iBufOut, iInpWrite=0;
    const uint8_t *pInpData = NULL;
    // point to input data and calculate size to encode; if no input we write an empty chunk
    if (pVoipBufferInp != NULL)
    {
        // require send buffer to be empty to ensure our sends are full size
        if (pVoipBufferOut->iBufOff != 0) 
        {
            return(0);
        }
        // locate data to read
        pInpData = pVoipBufferInp->pBuffer+pVoipBufferInp->iBufInp;
        // calculate how much input we have to write
        iInpWrite = pVoipBufferInp->iBufOff-pVoipBufferInp->iBufInp;
    }
    // write signed audioevent chunk
    iBufOut = AWSWriteEvent(pVoipBufferOut->pBuffer+pVoipBufferOut->iBufOff, pVoipBufferOut->iBufLen-pVoipBufferOut->iBufOff, pInpData, &iInpWrite, "AudioEvent", &pVoipTranscribe->AWSSignInfo);
    // consume input
    if (pVoipBufferInp != NULL)
    {
        pVoipBufferInp->iBufInp += iInpWrite;
    }
    // return size of output written
    return(iBufOut);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeSendEncode

    \Description
        Special encoding for providers that need it in either base64/protobuf
        (Google) or binary event (Amazon) format.

    \Input *pVoipTranscribe - module state
    \Input *pTransport      - transport ref
    \Input *pVoipBufferSrc  - buffer of data to encode for sending

    \Output
        VoipBufferT *       - pointer to VoipBuffer to send from

    \Version 12/16/2018 (jbrookes)
*/
/********************************************************************************F*/
static VoipBufferT *_VoipTranscribeSendEncode(VoipTranscribeRefT *pVoipTranscribe, TransportT *pTransport, VoipBufferT *pVoipBufferSrc)
{
    VoipBufferT *pVoipBufferSnd = &pVoipTranscribe->VoipBufferSnd;

    // if send buffer has been emptied, reset    
    if (pVoipBufferSnd->iBufInp == pVoipBufferSnd->iBufOff)
    {
        pVoipBufferSnd->iBufInp = pVoipBufferSnd->iBufOff = 0;
    }

    // encode the audio - this consumes data from the source buffer and writes encoded audio into the send buffer
    if (pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_GOOGLE)
    {
        // if we're at the start of a send, we need to prefix the audio with the encoding
        if (pVoipBufferSrc->iBufInp == 0) 
        {
            pVoipBufferSnd->iBufOff = (pTransport->eTransport == TRANSPORT_HTTP) ? _VoipTranscribeBase64Start(pVoipTranscribe, pVoipBufferSnd) : _VoipTranscribeProtobufStart(pVoipTranscribe, pVoipBufferSnd);
        }
        // encode the audio based on transport type
        pVoipBufferSnd->iBufOff += (pTransport->eTransport == TRANSPORT_HTTP) ? _VoipTranscribeBase64Encode(pVoipBufferSnd, pVoipBufferSrc) : _VoipTranscribeProtobufEncode(pVoipBufferSnd, pVoipBufferSrc);
    }
    else // Amazon
    {
        pVoipBufferSnd->iBufOff += _VoipTranscribeAwsEncode(pVoipTranscribe, pVoipBufferSnd, pVoipBufferSrc);
    }

    // if recording is finished and we've sent all the data, finish the request
    if (pVoipBufferSrc->bRecFinished && (pVoipBufferSrc->iBufInp == pVoipBufferSrc->iBufOff))
    {
        if ((pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_GOOGLE) && (pTransport->eTransport == TRANSPORT_HTTP))
        {
            pVoipBufferSnd->iBufOff += ds_snzprintf((char *)pVoipBufferSnd->pBuffer+pVoipBufferSnd->iBufOff, pVoipBufferSnd->iBufLen-pVoipBufferSnd->iBufOff, "\"}}");
        }
        else if (pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_AMAZON)
        {
            pVoipBufferSnd->iBufOff += _VoipTranscribeAwsEncode(pVoipTranscribe, pVoipBufferSnd, NULL);
        }
    }

    // return send voipbuffer
    return(pVoipBufferSnd);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeSend

    \Description
        Send a transcription request

    \Input *pVoipTranscribe - module state
    \Input *pVoipBuffer     - buffer to send

    \Output
        int32_t             - negative=failure, else success

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeSend(VoipTranscribeRefT *pVoipTranscribe, VoipBufferT *pVoipBuffer)
{
    TransportT *pTransport = &pVoipTranscribe->Transport;
    int32_t iResult=0;

    // amazon and google need audio encoded for transport; do that here
    if ((pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_AMAZON) || (pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_GOOGLE))
    {
        pVoipBuffer = _VoipTranscribeSendEncode(pVoipTranscribe, pTransport, pVoipBuffer);
    }

    // if we have data to send, send it
    if (pVoipBuffer->iBufInp < pVoipBuffer->iBufOff)
    {
        iResult = pTransport->Send(pTransport->pState, pTransport->iStreamId, (const char *)pVoipBuffer->pBuffer+pVoipBuffer->iBufInp, pVoipBuffer->iBufOff-pVoipBuffer->iBufInp);
        if (iResult > 0)
        {
            NetPrintfVerbose((pVoipTranscribe->iVerbose, 1, "voiptranscribe: [%d] sent [0x%04x,0x%04x]\n", pVoipBuffer->iBuffer, pVoipBuffer->iBufInp, pVoipBuffer->iBufInp+iResult));
            pVoipBuffer->iBufInp += iResult;
        }
        else if (iResult < 0)
        {
            NetPrintf(("voiptranscribe: Send() returned %d\n", iResult));
        }
    }

    // return result code to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeUpdateBackoff

    \Description
        Do backoff processing

    \Input *pVoipTranscribe - pointer to module state

    \Version 12/05/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipTranscribeUpdateBackoff(VoipTranscribeRefT *pVoipTranscribe)
{
    // do not process if backoff timer is not set
    if (pVoipTranscribe->uBackoffTimer == 0)
    {
        return;
    }
    // reset/clear backoff timer on expiration
    if (NetTickDiff(pVoipTranscribe->uBackoffTimer, NetTick()) <= 0)
    {
        NetPrintfVerbose((pVoipTranscribe->iVerbose, 0, "voiptranscribe: clearing backoff timer\n"));
        pVoipTranscribe->uBackoffTimer = 0;
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeUpdateRecord

    \Description
        Update recording of audio data; this function tracks if the recording
        should be considered done for this buffer due to the silence timeout
        being exceeded, and finalizes the audio buffer.

    \Input *pVoipTranscribe - module state
    \Input uCurTick         - current tick count

    \Version 12/14/2018 (jbrookes) Split from VoipTranscribeUpdate()
*/
/********************************************************************************F*/
static void _VoipTranscribeUpdateRecord(VoipTranscribeRefT *pVoipTranscribe, uint32_t uCurTick)
{
    VoipBufferT *pVoipBuffer = &pVoipTranscribe->VoipBuffer[pVoipTranscribe->iRecBuffer];
    #if DIRTYCODE_LOGGING
    static const char *_strStates[] = { "ST_FAIL", "ST_IDLE", "ST_CONN", "ST_SEND", "ST_RECV" };
    #endif

    // see if we have any audio to process
    if (pVoipBuffer->iNumSamples == 0)
    {
        return;
    }
    // see if we're done submitting data on active recording buffer
    if ((pVoipBuffer->bRecStarting || (NetTickDiff(uCurTick, pVoipTranscribe->uVoipTick) < VOIPTRANSCRIBE_SENDTIMEOUT)) && !pVoipBuffer->bRecFull && (pVoipBuffer->iNumSamples < VOIPTRANSCRIBE_MAXREQSAMPLES))
    {
        return;
    }
    /* if this buffer is ready to submit, but our other buffer is in a non-idle state, we gate
       submitting the buffer until the other buffer is idle (not connecting/sending/receiving) */
    if ((pVoipTranscribe->eState != ST_IDLE) && (pVoipTranscribe->iSndBuffer != pVoipTranscribe->iRecBuffer))
    {
        NetPrintfVerbose((pVoipTranscribe->iVerbose, 0, "voiptranscribe: [%d] waiting to finish submitting due to being in state %s(%d)\n", pVoipBuffer->iBuffer, _strStates[pVoipTranscribe->eState+1], pVoipTranscribe->eState));
        return;
    }
    // check to see if we should squelch this
    if (_VoipTranscribeBackoffCheck(pVoipTranscribe, pVoipBuffer))
    {
        return;
    }

    // finish transcribing audio
    _VoipTranscribeSubmitFinish(pVoipTranscribe, pVoipBuffer);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeUpdateSend

    \Description
        Update sending of audio data.  This function meters send size to a minimum
        amount for network efficiency, and handles complention of sending when
        all of the recorded data has been sent.

    \Input *pVoipTranscribe - module state
    \Input *pVoipBuffer     - pointer to voipbuffer being sent

    \Output
        int32_t             - updated state

    \Version 12/14/2018 (jbrookes) Split from VoipTranscribeUpdate()
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeUpdateSend(VoipTranscribeRefT *pVoipTranscribe, VoipBufferT *pVoipBuffer)
{
    int32_t iResult, iState=ST_SEND;
    // wait until we have enough data to send (or if we are done recording)
    if (((pVoipBuffer->iBufOff-pVoipBuffer->iBufInp) < 1280) && !pVoipBuffer->bRecFinished)
    {
        return(iState);
    }
    // send the data
    if ((iResult = _VoipTranscribeSend(pVoipTranscribe, pVoipBuffer)) < 0)
    {
        NetPrintf(("voiptranscribe: [%d] send failed result=%d\n", iResult));
        return(ST_FAIL);
    }
    // see if we're done
    if ((pVoipBuffer->iBufInp == pVoipBuffer->iBufOff) && pVoipBuffer->bRecFinished)
    {
        // finish sending process and transition to receive state
        if ((iResult = _VoipTranscribeSendFinish(pVoipTranscribe)) > 0)
        {
            NetPrintfVerbose((pVoipTranscribe->iVerbose, 1, "voiptranscribe: [%d] send complete result=%d\n", pVoipBuffer->iBuffer, iResult));
            iState = ST_RECV;
        }
        else if (iResult < 0)
        {
            iState = ST_FAIL;
        }
        else
        {
            NetPrintfVerbose((pVoipTranscribe->iVerbose, 1, "voiptranscribe: [%d] could not send finish; will try again\n", pVoipBuffer->iBuffer));
        }
    }
    // if transitioning to recv, reset buffer
    if (iState == ST_RECV)
    {
        _VoipTranscribeBufferReset(pVoipTranscribe, pVoipBuffer);
        pVoipTranscribe->iSndBuffer = -1;
    }
    return(iState);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeUpdateRecv

    \Description
        Update receiving of transcription response.

    \Input *pVoipTranscribe - module state

    \Output
        int32_t             - updated state

    \Version 12/14/2018 (jbrookes) Split from VoipTranscribeUpdate()
*/
/********************************************************************************F*/
static int32_t _VoipTranscribeUpdateRecv(VoipTranscribeRefT *pVoipTranscribe)
{
    TransportT *pTransport = &pVoipTranscribe->Transport;
    int32_t iResult, iState=pVoipTranscribe->eState;

    // see if there's anything to receive
    if ((iResult = pTransport->Recv(pTransport->pState, pTransport->iStreamId, pVoipTranscribe->strResponse, sizeof(pVoipTranscribe->strResponse))) >= 0)
    {
        // null terminate and log response if we're expecting text
        if (pTransport->eTransport != TRANSPORT_HTTP2)
        {
            pVoipTranscribe->strResponse[iResult] = '\0';
            NetPrintfVerbose((pVoipTranscribe->iVerbose, 2, "voiptranscribe: response (%d bytes)\n%s\n", iResult, pVoipTranscribe->strResponse));
        }

        // parse the result
        if ((iResult = _VoipTranscribeParseResponse(pVoipTranscribe, pVoipTranscribe->strResponse, iResult, pVoipTranscribe->strTranscription, sizeof(pVoipTranscribe->strTranscription))) > 0)
        {
            // update transcription length metric
            uint32_t uTranscriptionLength = (uint32_t)strnlen(pVoipTranscribe->strTranscription, sizeof(pVoipTranscribe->strTranscription));
            pVoipTranscribe->Metrics.uCharCountRecv += uTranscriptionLength;
            pVoipTranscribe->Metrics.uDelay += NetTickDiff(NetTick(), pVoipTranscribe->uSttStartTime);
            if (uTranscriptionLength == 0)
            {
                // keep track of number of consecutive empty results
                pVoipTranscribe->iConsecEmptyCt += 1;
                // update overall empty result count
                pVoipTranscribe->Metrics.uEmptyResultCount += 1;
                // set backoff if appropriate
                _VoipTranscribeBackoffSet(pVoipTranscribe);
            }
            else
            {
                // reset consecutive empty result tracker
                pVoipTranscribe->iConsecEmptyCt = 0;
            }
            // reset consecutive error count metric
            pVoipTranscribe->iConsecErrorCt = 0;
            // log transcription and transition back to idle state
            NetPrintfVerbose((pVoipTranscribe->iVerbose, 1, "voiptranscribe: transcript=%s\n", pVoipTranscribe->strTranscription));
            iState = ST_IDLE;
        }
        else if (iResult < 0)
        {
            NetPrintf(("voiptranscribe: service error: %s\n", pVoipTranscribe->strTranscription));
            pVoipTranscribe->strTranscription[0] = '\0';
            iState = ST_FAIL;
        }

        // clean up transaction if http2
        if (pTransport->eTransport == TRANSPORT_HTTP2)
        {
            ProtoHttp2StreamFree(pTransport->pState, pTransport->iStreamId);
            pTransport->iStreamId = PROTOHTTP2_INVALID_STREAMID;
        }
    }
    else if ((iResult < 0) && (iResult != VOIPTRANSCRIBE_WAIT))
    {
        NetPrintf(("voiptranscribe: recv() returned %d\n", iResult));
        iState = ST_FAIL;
    }
    // return updated state
    return(iState);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranscribeConfig

    \Description
        Configure the VoipTranscribe module for use.  This call is required to
        specify the provider, url, and credentials that will be used to access
        the transcription service.

    \Input *pVoipTranscribe    - pointer to module state
    \Input uProfile            - transcribe profile (VOIPTRANSCRIBE_PROFILE_DISABLED to disable)
    \Input *pUrl               - transcribe provider url
    \Input *pCred              - transcribe credentials
    
    \Output
        uint32_t               - TRUE if configured successfully

    \Version 11/08/2018 (tcho)
*/
/********************************************************************************F*/
static uint32_t _VoipTranscribeConfig(VoipTranscribeRefT *pVoipTranscribe, uint32_t uProfile, const char *pUrl, const char *pCred)
{
    NetCritEnter(NULL);
    
    // clean up previous transport state
    _VoipTranscribeTransportCleanup(pVoipTranscribe);

    // save configuration parameters
    if (VOIPTRANSCRIBE_PROFILE_PROVIDER(uProfile) != VOIPTRANSCRIBE_PROVIDER_NONE)
    {
        pVoipTranscribe->uProfile = uProfile;
        ds_strnzcpy(pVoipTranscribe->strKey, pCred, sizeof(pVoipTranscribe->strKey));
        ds_strnzcpy(pVoipTranscribe->strUrl, pUrl, sizeof(pVoipTranscribe->strUrl));
    }
    else
    {
        NetPrintf(("voiptranscribe: disabled\n"));
        pVoipTranscribe->uProfile = uProfile;
        ds_memclr(pVoipTranscribe->strKey, sizeof(pVoipTranscribe->strKey));
        ds_memclr(pVoipTranscribe->strUrl, sizeof(pVoipTranscribe->strUrl));
        NetCritLeave(NULL);
        return(FALSE);
    }

    // set provider info
    pVoipTranscribe->eProvider = VOIPTRANSCRIBE_PROFILE_PROVIDER(pVoipTranscribe->uProfile);
    if (pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_MICROSOFT)
    {
        // install CA certificate required to access microsoft servers
        ProtoSSLSetCACert((const uint8_t *)_strCyberTrustRootCA, sizeof(_strCyberTrustRootCA));
    }
    else if (pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_GOOGLE)
    {
        // install CA certificate required to access Google Speech-to-text server
        ProtoSSLSetCACert((const uint8_t *)_strGlobalSignRootCAR2, sizeof(_strGlobalSignRootCAR2));
    }
    else if (pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_AMAZON)
    {
        // install CA certificate required to access Amazon Transcribe
        ProtoSSLSetCACert((const uint8_t *)_strAmazonRootCAR1, sizeof(_strAmazonRootCAR1));
    }

    // init transport class
    if (_VoipTranscribeTransportInit(pVoipTranscribe) < 0)
    {
        NetPrintf(("voiptranscribe: could not initialize transport module\n"));
        VoipTranscribeDestroy(pVoipTranscribe);
        NetCritLeave(NULL);
        return(FALSE);
    }

    // set audio parameters
    pVoipTranscribe->iAudioRate = VOIPTRANSCRIBE_AUDIORATE;
    pVoipTranscribe->eFormat = VOIPTRANSCRIBE_PROFILE_FORMAT(pVoipTranscribe->uProfile);
    if (pVoipTranscribe->eProvider != VOIPTRANSCRIBE_PROVIDER_GOOGLE)
    {
        if (pVoipTranscribe->eFormat == VOIPTRANSCRIBE_FORMAT_LI16)
        {
            ds_snzprintf(pVoipTranscribe->strAudioFormat, sizeof(pVoipTranscribe->strAudioFormat), "audio/l16; rate=%d; endianness=little-endian", pVoipTranscribe->iAudioRate);
            pVoipTranscribe->bCompressed = FALSE;
        }
        else if (pVoipTranscribe->eFormat == VOIPTRANSCRIBE_FORMAT_WAV16)
        {
            ds_snzprintf(pVoipTranscribe->strAudioFormat, sizeof(pVoipTranscribe->strAudioFormat), "audio/wav; codec=audio/pcm; samplerate=%d", pVoipTranscribe->iAudioRate);
            pVoipTranscribe->bCompressed = FALSE;
        }
        else if (pVoipTranscribe->eFormat == VOIPTRANSCRIBE_FORMAT_OPUS)
        {
            ds_strnzcpy(pVoipTranscribe->strAudioFormat, "audio/ogg; codecs=opus", sizeof(pVoipTranscribe->strAudioFormat));
            pVoipTranscribe->bCompressed = TRUE;
        }
    }
    else
    {
        if (pVoipTranscribe->eFormat == VOIPTRANSCRIBE_FORMAT_LI16)
        {
            ds_strnzcpy(pVoipTranscribe->strAudioFormat, "LINEAR16", sizeof(pVoipTranscribe->strAudioFormat));
            pVoipTranscribe->bCompressed = FALSE;
        }
        else if (pVoipTranscribe->eFormat == VOIPTRANSCRIBE_FORMAT_OPUS)
        {
            ds_strnzcpy(pVoipTranscribe->strAudioFormat, "OGG_OPUS", sizeof(pVoipTranscribe->strAudioFormat));
            pVoipTranscribe->bCompressed = TRUE;
        }
    }
    NetCritLeave(NULL);

    return(TRUE);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipTranscribeCreate

    \Description
        Create the stream module

    \Input iBufSize             - size of streaming buffer (at least VOIPTRANSCRIBE_MINBUFFER)

    \Output
        VoipTranscribeRefT *    - new module state, or NULL

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
VoipTranscribeRefT *VoipTranscribeCreate(int32_t iBufSize)
{
    VoipTranscribeRefT *pVoipTranscribe;
    void *pMemGroupUserData;
    int32_t iMemGroup;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // enforce minimum buffer size
    if (iBufSize < VOIPTRANSCRIBE_MINBUFFER)
    {
        iBufSize = VOIPTRANSCRIBE_MINBUFFER;
    }

    // allocate and init module state
    if ((pVoipTranscribe = DirtyMemAlloc(sizeof(*pVoipTranscribe), VOIPTRANSCRIBE_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voiptranscribe: could not allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pVoipTranscribe, sizeof(*pVoipTranscribe));
    pVoipTranscribe->iMemGroup = iMemGroup;
    pVoipTranscribe->pMemGroupUserData = pMemGroupUserData;

    // allocate and initialize buffers
    if (!_VoipTranscribeBufferInit(pVoipTranscribe, &pVoipTranscribe->VoipBuffer[0], iBufSize, 0) || !_VoipTranscribeBufferInit(pVoipTranscribe, &pVoipTranscribe->VoipBuffer[1], iBufSize, 1))
    {
        NetPrintf(("voiptranscribe: could not allocate voip buffers\n"));
        VoipTranscribeDestroy(pVoipTranscribe);
        return(NULL);
    }
    // allocate and initialize voip send buffer; this is used for google, which needs data to be encoded
    if (!_VoipTranscribeBufferInit(pVoipTranscribe, &pVoipTranscribe->VoipBufferSnd, 2*1024, -1))
    {
        NetPrintf(("voiptranscribe: could not allocate voip send buffer\n"));
        VoipTranscribeDestroy(pVoipTranscribe);
        return(NULL);
    }

    // init other state variables
    pVoipTranscribe->bMinDiscard = TRUE;
    pVoipTranscribe->iConsecEmptyMax = VOIPTRANSCRIBE_CONSECEMPTY;
    pVoipTranscribe->iConsecErrorMax = VOIPTRANSCRIBE_CONSECERROR;
    pVoipTranscribe->iSndBuffer = -1;
    pVoipTranscribe->iVerbose = 1;

    // configure for particular provider
    if (!_VoipTranscribeConfig(pVoipTranscribe, _VoipTranscribe_Config.uProfile, _VoipTranscribe_Config.strUrl, _VoipTranscribe_Config.strKey))
    {
        NetPrintf(("voiptranscribe: could not configure for provider\n"));
        VoipTranscribeDestroy(pVoipTranscribe);
        return(NULL);
    }

    // return ref to caller
    return(pVoipTranscribe);
}

/*F********************************************************************************/
/*!
    \Function VoipTranscribeConfig

    \Description
        Set global configuration of transcription service.  This call is required to
        specify the provider, url, and credentials that will be used to access
        the transcription service.

    \Input uProfile            - transcribe profile (VOIPTRANSCRIBE_PROFILE_DISABLED to disable)
    \Input *pUrl               - transcribe provider url
    \Input *pKey               - transcribe access key

    \Version 11/08/2018 (tcho)
*/
/********************************************************************************F*/
void VoipTranscribeConfig(uint32_t uProfile, const char *pUrl, const char *pKey)
{
    NetCritEnter(NULL);
    _VoipTranscribe_Config.uProfile = uProfile;
    ds_strnzcpy(_VoipTranscribe_Config.strKey, pKey, sizeof(_VoipTranscribe_Config.strKey));
    ds_strnzcpy(_VoipTranscribe_Config.strUrl, pUrl, sizeof(_VoipTranscribe_Config.strUrl));
    NetCritLeave(NULL);
}

/*F********************************************************************************/
/*!
    \Function VoipTranscribeDestroy

    \Description
        Destroy the VoipTranscribe module

    \Input *pVoipTranscribe    - pointer to module state

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
void VoipTranscribeDestroy(VoipTranscribeRefT *pVoipTranscribe)
{
    // dispose of audio buffers
    if (pVoipTranscribe->VoipBuffer[0].pBuffer != NULL)
    {
        DirtyMemFree(pVoipTranscribe->VoipBuffer[0].pBuffer, VOIPTRANSCRIBE_MEMID, pVoipTranscribe->iMemGroup, pVoipTranscribe->pMemGroupUserData);
    }
    if (pVoipTranscribe->VoipBuffer[1].pBuffer != NULL)
    {
        DirtyMemFree(pVoipTranscribe->VoipBuffer[1].pBuffer, VOIPTRANSCRIBE_MEMID, pVoipTranscribe->iMemGroup, pVoipTranscribe->pMemGroupUserData);
    }
    if (pVoipTranscribe->VoipBufferSnd.pBuffer != NULL)
    {
        DirtyMemFree(pVoipTranscribe->VoipBufferSnd.pBuffer, VOIPTRANSCRIBE_MEMID, pVoipTranscribe->iMemGroup, pVoipTranscribe->pMemGroupUserData);
    }

    // cleanup transport state
    _VoipTranscribeTransportCleanup(pVoipTranscribe);

    // dispose of module memory
    DirtyMemFree(pVoipTranscribe, VOIPTRANSCRIBE_MEMID, pVoipTranscribe->iMemGroup, pVoipTranscribe->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function VoipTranscribeSubmit

    \Description
        Submit voice data to be transcribed

    \Input *pVoipTranscribe - pointer to module state
    \Input *pBuffer         - voice data to be transcribed
    \Input iBufLen          - size of voice data in bytes

    \Output
        int32_t             - number of bytes copied

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipTranscribeSubmit(VoipTranscribeRefT *pVoipTranscribe, const uint8_t *pBuffer, int32_t iBufLen)
{
    // submit and return amount copied to caller
    return(_VoipTranscribeSubmit(pVoipTranscribe, pBuffer, iBufLen));
}

/*F********************************************************************************/
/*!
    \Function VoipTranscribeGet

    \Description
        Get transcription if available; if a transcription is available, this
        call copies it and clears it.

    \Input *pVoipTranscribe - pointer to module state
    \Input *pBuffer         - [out] output buffer
    \Input iBufLen          - size of output buffer

    \Output
        int32_t             - zero=no transcription, else transcription copied

    \Version 09/07/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipTranscribeGet(VoipTranscribeRefT *pVoipTranscribe, char *pBuffer, int32_t iBufLen)
{
    if (pVoipTranscribe->strTranscription[0] == '\0')
    {
        return(0);
    }
    ds_strnzcpy((char *)pBuffer, pVoipTranscribe->strTranscription, iBufLen);
    pVoipTranscribe->strTranscription[0] = '\0';
    return(1);
}

/*F********************************************************************************/
/*!
    \Function VoipTranscribeStatus

    \Description
        Get module status.

    \Input *pVoipTranscribe - pointer to module state
    \Input iStatus          - status selector
    \Input iValue           - selector specific
    \Input *pBuffer         - selector specific
    \Input iBufSize         - selector specific

    \Output
        int32_t             - selector specific

    \Notes
        iStatus can be one of the following:

        \verbatim
            'cmpr' - most recent http result code
            'sttm' - get the VoipSpeechToTextMetricsT via pBuf
        \endverbatim

        Unrecognized codes are passed down to the transport handler

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipTranscribeStatus(VoipTranscribeRefT *pVoipTranscribe, int32_t iStatus, int32_t iValue, void *pBuffer, int32_t iBufSize)
{
    TransportT *pTransport = &pVoipTranscribe->Transport;
    // return whether audio format is compressed or not
    if (iStatus == 'cmpr')
    {
        return(pVoipTranscribe->bCompressed);
    }
    if (iStatus == 'sttm')
    {
        if ((pBuffer != NULL) && (iBufSize >= (int32_t)sizeof(VoipSpeechToTextMetricsT)))
        {
            ds_memcpy_s(pBuffer, iBufSize, &pVoipTranscribe->Metrics, sizeof(VoipSpeechToTextMetricsT));
            return(0);
        }
        return(-1);
    }
    return(pTransport->Status(pTransport->pState, pTransport->iStreamId, iStatus, pBuffer, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function VoipTranscribeControl

    \Description
        Set control options

    \Input *pVoipTranscribe - pointer to module state
    \Input iControl         - control selector
    \Input iValue           - selector specific
    \Input iValue2          - selector specific
    \Input *pValue          - selector specific

    \Output
        int32_t             - selector specific

    \Notes
        iStatus can be one of the following:

        \verbatim
            'cstm' - clear speech to text metrics in VoipSpeechToTextMetricsT
            'spam' - set verbose debug level (debug only)
            'time' - set timeout value
            'vdis' - set voice discard on minimum threshold (default=TRUE)
        \endverbatim

        Unhandled codes are passed through to the transport handler

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipTranscribeControl(VoipTranscribeRefT *pVoipTranscribe, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    TransportT *pTransport = &pVoipTranscribe->Transport;

    if (iControl == 'cstm')
    {
        ds_memclr(&(pVoipTranscribe->Metrics), sizeof(pVoipTranscribe->Metrics));
        return(0);
    }
    #if DIRTYCODE_LOGGING
    // set verbosity for us and pass through to transport handler
    if (iControl == 'spam')
    {
        pVoipTranscribe->iVerbose = iValue;
    }
    #endif
    if (iControl == 'time')
    {
        // remember most recent timeout value, and pass through to transport handler
        pVoipTranscribe->iTimeout = iValue;
    }
    if (iControl == 'vdis')
    {
        uint8_t bDiscard = iValue ? TRUE : FALSE;
        if (pVoipTranscribe->bMinDiscard != bDiscard)
        {
            NetPrintfVerbose((pVoipTranscribe->iVerbose, 1, "voiptranscribe: min discard %s\n", bDiscard ? "enabled" : "disabled"));
            pVoipTranscribe->bMinDiscard = bDiscard;
        }
        return(0);
    }
    // if not handled, let transport handler take a stab at it
    return(pTransport->Control(pTransport->pState, pTransport->iStreamId, iControl, iValue, iValue2, pValue));
}

/*F********************************************************************************/
/*!
    \Function VoipTranscribeUpdate

    \Description
        Update the VoipTranscribe module

    \Input *pVoipTranscribe    - pointer to module state

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
void VoipTranscribeUpdate(VoipTranscribeRefT *pVoipTranscribe)
{ 
    TransportT *pTransport = &pVoipTranscribe->Transport;
    uint32_t uCurTick = NetTick();
    int32_t iResult;

    // give time to transport module
    pTransport->Update(pTransport->pState);

    // update backoff processing
    _VoipTranscribeUpdateBackoff(pVoipTranscribe);

    // update recording processing
    _VoipTranscribeUpdateRecord(pVoipTranscribe, uCurTick);

    /* if we have a websockets connection to watson and are in the idle state, we receive to consume "listening" responses that come after transcription
       responses.  if we do not read these responses, the unread data prevents us from detecting if the server has timed out the connection on us, and
       results in the next transcription request failing */    
    if ((pVoipTranscribe->eProvider == VOIPTRANSCRIBE_PROVIDER_IBMWATSON) && (pVoipTranscribe->eTransport == VOIPTRANSCRIBE_TRANSPORT_WEBSOCKETS) &&
        (pVoipTranscribe->eState == ST_IDLE) && (_VoipTranscribeConnectCheck(pVoipTranscribe, &pVoipTranscribe->Transport) > 0))
    {
        pVoipTranscribe->eState = _VoipTranscribeUpdateRecv(pVoipTranscribe);
    }

    // check for enough data in the current record buffer to start request
    if ((pVoipTranscribe->eState == ST_IDLE) && (pVoipTranscribe->iSndBuffer == -1))
    {
        VoipBufferT *pVoipBuffer = &pVoipTranscribe->VoipBuffer[pVoipTranscribe->iRecBuffer];
        // see if we have enough data in our current record buffer to start sending (minimum one second)
        if ((pVoipBuffer->iNumSamples < pVoipTranscribe->iAudioRate) && pVoipTranscribe->bMinDiscard)
        {
            return;
        }
        // if backoff timer is set, defer sending as we might end up squelching it
        if (pVoipTranscribe->uBackoffTimer != 0)
        {
            return;
        }
        // set send buffer
        NetPrintfVerbose((pVoipTranscribe->iVerbose, 1, "voiptranscribe: [%d] starting transcription request on recording buffer\n", pVoipBuffer->iBuffer));
        pVoipTranscribe->iSndBuffer = pVoipTranscribe->iRecBuffer;
    }

    // if we're in idle state and have an assigned send buffer, start the request
    if ((pVoipTranscribe->eState == ST_IDLE) && (pVoipTranscribe->iSndBuffer != -1))
    {
        VoipBufferT *pVoipBuffer = &pVoipTranscribe->VoipBuffer[pVoipTranscribe->iSndBuffer];
        NetPrintfVerbose((pVoipTranscribe->iVerbose, 1, "voiptranscribe: [%d] starting transcription request\n", pVoipBuffer->iBuffer));
        // copy mindiscard flag for this buffer
        pVoipBuffer->bMinDiscard = pVoipTranscribe->bMinDiscard;
        // perform explicit connection for transport handlers that require it
        pVoipTranscribe->eState = (_VoipTranscribeConnect(pVoipTranscribe) >= 0) ? ST_CONN : ST_FAIL;
    }

    // update module in connecting state
    if (pVoipTranscribe->eState == ST_CONN)
    {
        // check for connection completion for transport handlers that require it
        if ((iResult = _VoipTranscribeConnectCheck(pVoipTranscribe, &pVoipTranscribe->Transport)) < 0)
        {
            pVoipTranscribe->eState = ST_FAIL;
            return;
        }
        else if (iResult == 0)
        {
            return;
        }

        // make transcription request and transition to sending voice data for transcription if successful
        pVoipTranscribe->eState = (_VoipTranscribeRequest(pVoipTranscribe) >= 0) ? ST_SEND : ST_FAIL;
    }

    // update while sending the transcription request
    if (pVoipTranscribe->eState == ST_SEND)
    {
        pVoipTranscribe->eState = _VoipTranscribeUpdateSend(pVoipTranscribe, &pVoipTranscribe->VoipBuffer[pVoipTranscribe->iSndBuffer]);
    }

    // update while receiving the transcription response
    if (pVoipTranscribe->eState == ST_RECV)
    {
        pVoipTranscribe->eState = _VoipTranscribeUpdateRecv(pVoipTranscribe);
    }

    // update when in failed state
    if (pVoipTranscribe->eState == ST_FAIL)
    {
        // keep track of number of consecutive failures
        pVoipTranscribe->iConsecErrorCt += 1;
        // update overall failure count
        pVoipTranscribe->Metrics.uErrorCount += 1;
        // set backoff if appropriate
        _VoipTranscribeBackoffSet(pVoipTranscribe);
        // reset current record buffer
        _VoipTranscribeBufferReset(pVoipTranscribe, &pVoipTranscribe->VoipBuffer[pVoipTranscribe->iRecBuffer]);
        // go back to idle state
        pVoipTranscribe->eState = ST_IDLE;
    }
}
