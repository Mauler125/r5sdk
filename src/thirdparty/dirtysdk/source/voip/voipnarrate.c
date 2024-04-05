/*H********************************************************************************/
/*!
    \File voipnarrate.c

    \Description
        Voip narration API wrapping Cloud-based text-to-speech services, supporting
        IBM Watson, Microsoft Speech Service, Google Speech, and Amazon Polly.
        Narration requests may be up to 255 characters in length, and overlapping
        requests are queued in order.

    \Notes
        References

        IBM Watson:
            Text-to Speech-API: https://www.ibm.com/watson/developercloud/text-to-speech/api/v1/curl.html

        Microsoft Speech Service:
            Text-to-Speech How-To: https://docs.microsoft.com/en-us/azure/cognitive-services/Speech-Service/how-to-text-to-speech

        Google Text-to-Speech
            Text-to-Speech API: https://cloud.google.com/text-to-speech/docs/reference/rest/

        Amazon Polly
            SynthesizeSpeech API: https://docs.aws.amazon.com/polly/latest/dg/API_SynthesizeSpeech.html
            Amazon Endpoint Names: https://docs.aws.amazon.com/general/latest/gr/rande.html
            VoiceId List: https://docs.aws.amazon.com/polly/latest/dg/API_SynthesizeSpeech.html#polly-SynthesizeSpeech-request-VoiceId

        WAV:
            WAVE file format: https://en.wikipedia.org/wiki/WAV#RIFF_WAVE

    \Copyright
        Copyright 2018 Electronic Arts

    \Version 10/25/2018 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/proto/protostream.h"
#include "DirtySDK/util/aws.h"
#include "DirtySDK/util/base64.h"
#include "DirtySDK/util/jsonformat.h"
#include "DirtySDK/util/jsonparse.h"
#include "DirtySDK/voip/voipdef.h"

#include "DirtySDK/voip/voipnarrate.h"

/*** Defines **********************************************************************/

//! protostream minimum data amount (for base64 decoding; four is the minimum amount but that produces one and a half samples, so we choose eight, BUT...
#define VOIPNARRATE_MINBUF  (8)

//! how many ms of audio received should we treat as being empty audio (for metrics)
#define VOIPNARRATE_EMPTY_AUDIO_THRESHOLD_MS (300)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct VoipNarrateConfigT
{
    VoipNarrateProviderE eProvider;         //!< configured provider
    char strUrl[256];                       //!< URL for text-to-speech request
    char strKey[128];                       //!< API key required for service authentication
} VoipNarrateConfigT;

//! narration request data
typedef struct VoipNarrateRequestT
{
    struct VoipNarrateRequestT *pNext;
    VoipNarrateGenderE eGender;
    char strText[VOIPNARRATE_INPUT_MAX];
    int8_t iUserIndex;
} VoipNarrateRequestT;

struct VoipNarrateRefT
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    ProtoStreamRefT *pProtoStream;          //!< stream transport module to handle buffered download of audio data

    VoipNarrateVoiceDataCbT *pVoiceDataCb;  //!< user callback used to provide voice data
    void *pUserData;                        //!< user data for user callback

    VoipNarrateRequestT *pRequest;          //!< list of queued requests, if any
    VoipNarrateConfigT Config;              //!< module configuration (provider and credentials)

    char strHead[256];                      //!< http head for narration request
    char strBody[512];                      //!< http body for narration request
    const char *pBody;                      //!< pointer to start of body (may not match buffer start)

    VoipTextToSpeechMetricsT Metrics;       //!< Usage metrics of the narration module
    uint32_t uTtsStartTime;                 //!< time when we sent the request

    uint8_t aVoiceBuf[160*3*2];             //!< base64 decode buffer, sized for one 30ms frame of 16khz 16bit voice audio, also a multiple of three bytes to accomodate base64 4->3 ratio
    int32_t iVoiceOff;                      //!< read offset in buffered voice data
    int32_t iVoiceLen;                      //!< end of buffered voice data
    int32_t iSamplesInPhrase;               //!< total number of samples received for this phrase

    uint8_t bStart;                         //!< TRUE if start of stream download, else FALSE
    uint8_t bActive;                        //!< TRUE if stream is active, else FALSE
    int8_t  iUserIndex;                     //!< index of local user current request is being made for
    int8_t  iVerbose;                       //!< verbose debug level (debug only)
};

/*** Variables ********************************************************************/

//! global config state
static VoipNarrateConfigT _VoipNarrate_Config = { VOIPNARRATE_PROVIDER_NONE, "", "" };

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _VoipNarrateCustomHeaderCb

    \Description
        Custom header callback used to sign AWS requests

    \Input *pState          - http module state
    \Input *pHeader         - pointer to http header buffer
    \Input uHeaderSize      - size of http header buffer
    \Input *pData           - pointer to data (unused)
    \Input iDataLen         - data length (unused)
    \Input *pUserRef        - voipnarrate ref

    \Output
        int32_t             - output header length

    \Version 12/28/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipNarrateCustomHeaderCb(ProtoHttpRefT *pState, char *pHeader, uint32_t uHeaderSize, const char *pData, int64_t iDataLen, void *pUserRef)
{
    VoipNarrateRefT *pVoipNarrate = (VoipNarrateRefT *)pUserRef;
    int32_t iHdrLen = (int32_t)strlen(pHeader);
    
    // if amazon and we have room, sign the request
    if ((pVoipNarrate->Config.eProvider != VOIPNARRATE_PROVIDER_AMAZON) || (uHeaderSize < (unsigned)iHdrLen))
    {
        return(iHdrLen);
    }

    // sign the request and return the updated size
    iHdrLen += AWSSignSigV4(pHeader, uHeaderSize, pVoipNarrate->pBody, pVoipNarrate->Config.strKey, "polly", NULL);
    // return size to protohttp
    return(iHdrLen);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateSkipWavHeader

    \Description
        Return offset past WAV header in input data

    \Input *pData           - pointer to wav header
    \Input iDataLen         - length of data

    \Output
        int32_t             - offset past WAV header, or zero

    \Version 11/06/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipNarrateSkipWavHeader(const uint8_t *pData, int32_t iDataLen)
{
    int32_t iOffset = 0, iChkLen;
    uint8_t bFoundData;

    // validate and skip RIFF/WAVE header
    if ((iDataLen < 12) || ds_strnicmp((const char *)pData, "RIFF", 4) || ds_strnicmp((const char *)pData+8, "WAVE", 4))
    {
        return(0);
    }
    iOffset += 12;

    // process chunks
    for (bFoundData = FALSE; iOffset < (iDataLen+12); iOffset += iChkLen+8)
    {
        // get chunk length
        iChkLen  = pData[iOffset+4];
        iChkLen |= pData[iOffset+5]<<8;
        iChkLen |= pData[iOffset+6]<<16;
        iChkLen |= pData[iOffset+7]<<24;

        // look for data chunk
        if (!ds_strnicmp((const char *)pData+iOffset, "data", 4))
        {
            bFoundData = TRUE;
            iOffset += 8;
            break;
        }
    }
    return(bFoundData ? iOffset : 0);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateBasicAuth

    \Description
        Encode Basic HTTP authorization header as per https://tools.ietf.org/html/rfc7617

    \Input *pBuffer     - [out] output buffer for encoded base64 string
    \Input iBufSize     - size of output buffer
    \Input *pUser       - user identifer
    \Input *pPass       - user password

    \Output
        const char *    - pointer to output buffer

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipNarrateBasicAuth(char *pBuffer, int32_t iBufSize, const char *pUser, const char *pPass)
{
    char strAuth[128];
    ds_snzprintf(strAuth, sizeof(strAuth), "%s:%s", pUser, pPass);
    Base64Encode2(strAuth, (int32_t)strlen(strAuth), pBuffer, iBufSize);
    return(pBuffer);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateBase64Decode

    \Description
        Decode Base64-encoded voice data

    \Input *pVoipNarrate    - pointer to module state
    \Input *pOutput         - [out] buffer to hold decoded voice data
    \Input *pOutSize        - [in/out] output buffer length, size of output data
    \Input *pInput          - base64-encoded input data
    \Input iInpSize         - input buffer length

    \Output
        int32_t             - negative=failure, else input bytes consumed

    \Version 10/27/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipNarrateBase64Decode(VoipNarrateRefT *pVoipNarrate, char *pOutput, int32_t *pOutSize, const char *pInput, int32_t iInpSize)
{
    static const char _strJson[] = "\"audioContent\":";
    const char *pInput2, *pInpEnd = pInput + iInpSize;
    int32_t iInpOff = 0;

    // if we have the beginning of json envelope, skip it
    if ((pInput2 = strstr(pInput, _strJson)) != NULL)
    {
        // skip json header
        pInput2 += sizeof(_strJson);
        // skip to base64 data
        for (; (*pInput2 != '"') && (pInput2 < pInpEnd); pInput2 += 1)
            ;
        if (*pInput2 != '"')
        {
            return(-1);
        }
        // skip quote
        pInput2 += 1;
        // remember to consume this in addition to base64 data
        iInpOff = pInput2 - pInput;
        pInput = pInput2;
    }
    // if we have end of json envelope, trim it
    if ((pInput2 = strchr(pInput, '"')) != NULL)
    {
        // handle end of data
        if (pInput2 == pInput)
        {
            iInpOff = iInpSize;
        }
        iInpSize = pInput2-pInput;
    }

    // constrain input size to what will fit in output buffer
    if (iInpSize > Base64EncodedSize(*pOutSize))
    {
        iInpSize = Base64EncodedSize(*pOutSize);
    }

    // make sure input size is a multiple of four to produce an integral number of output bytes
    iInpSize &= ~0x03;

    // base64 decode and save output size
    *pOutSize = Base64Decode3(pInput, iInpSize, pOutput, *pOutSize);
    // return number of bytes of input consumed
    return(iInpSize+iInpOff);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateRequestAdd

    \Description
        Queue request for later sending

    \Input *pVoipNarrate    - pointer to module state
    \Input iUserIndex       - local user index of user who is requesting speech synthesis
    \Input eGender          - preferred gender for voice narration
    \Input *pText           - text to be converted

    \Output
        int32_t             - negative=failure, else success

    \Version 11/09/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipNarrateRequestAdd(VoipNarrateRefT *pVoipNarrate, int32_t iUserIndex, VoipNarrateGenderE eGender, const char *pText)
{
    VoipNarrateRequestT *pRequest;

    // allocate and clear the request
    if ((pRequest = DirtyMemAlloc(sizeof(*pRequest), VOIPNARRATE_MEMID, pVoipNarrate->iMemGroup, pVoipNarrate->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipnarrate: could not allocate request\n"));
        pVoipNarrate->Metrics.uErrorCount += 1;
        return(-1);
    }
    ds_memclr(pRequest, sizeof(*pRequest));

    // copy the request data
    ds_strnzcpy(pRequest->strText, pText, sizeof(pRequest->strText));
    pRequest->iUserIndex = iUserIndex;
    pRequest->eGender = eGender;

    // add to queue
    pRequest->pNext = pVoipNarrate->pRequest;
    pVoipNarrate->pRequest = pRequest;

    // return success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateRequestGet

    \Description
        Get queued request

    \Input *pVoipNarrate    - pointer to module state
    \Input *pRequest        - [out] storage for request (may be null)

    \Version 11/09/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipNarrateRequestGet(VoipNarrateRefT *pVoipNarrate, VoipNarrateRequestT *pRequest)
{
    VoipNarrateRequestT **ppRequest;
    // get oldest request (we add to head, so get from tail)
    for (ppRequest = &pVoipNarrate->pRequest; (*ppRequest)->pNext != NULL; ppRequest = &((*ppRequest)->pNext))
        ;
    // copy request
    if (pRequest != NULL)
    {
        ds_memcpy_s(pRequest, sizeof(*pRequest), *ppRequest, sizeof(**ppRequest));
    }
    // free request
    DirtyMemFree(*ppRequest, VOIPNARRATE_MEMID, pVoipNarrate->iMemGroup, pVoipNarrate->pMemGroupUserData);
    // remove from list
    *ppRequest = NULL;
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateFormatHeadWatson

    \Description
        Format connection header for IBM Watson Speech service
        Ref: https://console.bluemix.net/docs/services/text-to-speech/http.html#usingHTTP

    \Input *pVoipNarrate    - pointer to module state
    \Input *pUrl            - [out] buffer for formatted url
    \Input iUrlLen          - length of url buffer
    \Input *pHead           - [out] buffer for formatted request header
    \Input iHeadLen         - length of header buffer
    \Input eGender          - preferred gender for voice narration

    \Output
        int32_t             - negative=failure, else success

    \Version 11/07/2018 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipNarrateFormatHeadWatson(VoipNarrateRefT *pVoipNarrate, char *pUrl, int32_t iUrlLen, char *pHead, int32_t iHeadLen, VoipNarrateGenderE eGender)
{
    char strAuth[128];
    int32_t iOffset=0;

    // encode Basic authorization string with string apikey:<key>
    _VoipNarrateBasicAuth(strAuth, sizeof(strAuth), "apikey", pVoipNarrate->Config.strKey);

    // format request header
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Content-Type: application/json\r\n");
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Accept: audio/wav; rate=%d\r\n", VOIPNARRATE_SAMPLERATE);
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Authorization: Basic %s\r\n", strAuth);

    // format url with voice based on preferred gender
    ds_snzprintf(pUrl, iUrlLen, "%s?voice=%s", pVoipNarrate->Config.strUrl, (eGender == VOIPNARRATE_GENDER_FEMALE) ? "en-US_AllisonVoice" : "en-US_MichaelVoice");
    return(pUrl);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateFormatBodyWatson

    \Description
        Format request body for IBM Watson Speech service

    \Input *pVoipNarrate    - pointer to module state
    \Input *pBody           - [out] buffer to hold request body
    \Input iBodyLen         - buffer length
    \Input *pText           - pointer to text request

    \Output
        int32_t             - negative=failure, else success

        \Version 11/07/2018 (jbrookes)
*/
/********************************************************************************F*/
static char *_VoipNarrateFormatBodyWatson(VoipNarrateRefT *pVoipNarrate, char *pBody, int32_t iBodyLen, const char *pText)
{
    JsonInit(pBody, iBodyLen, JSON_FL_WHITESPACE);
    JsonAddStr(pBody, "text", pText);
    pBody = JsonFinish(pBody);
    return(pBody);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateFormatHeadMicrosoft

    \Description
        Format connection header for Microsoft Speech service

    \Input *pVoipNarrate    - pointer to module state
    \Input *pUrl            - [out] buffer for formatted url
    \Input iUrlLen          - length of url buffer
    \Input *pHead           - [out] buffer for formatted request header
    \Input iHeadLen         - length of header buffer

    \Output
        int32_t             - negative=failure, else success

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipNarrateFormatHeadMicrosoft(VoipNarrateRefT *pVoipNarrate, char *pUrl, int32_t iUrlLen, char *pHead, int32_t iHeadLen)
{
    int32_t iOffset=0;

    // format request header
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Content-Type: application/ssml+xml\r\n");
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "X-Microsoft-OutputFormat: raw-%dkhz-16bit-mono-pcm\r\n", VOIPNARRATE_SAMPLERATE/1000);
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Ocp-Apim-Subscription-Key: %s\r\n", pVoipNarrate->Config.strKey);

    // copy url
    ds_strnzcpy(pUrl, pVoipNarrate->Config.strUrl, iUrlLen);
    return(pUrl);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateFormatBodyMicrosoft

    \Description
        Format request body for Microsoft Speech service

    \Input *pVoipNarrate    - pointer to module state
    \Input *pBody           - [out] buffer to hold request body
    \Input iBodyLen         - buffer length
    \Input eGender          - preferred gender for voice narration
    \Input *pText           - pointer to text request

    \Output
        int32_t             - negative=failure, else success

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
static char *_VoipNarrateFormatBodyMicrosoft(VoipNarrateRefT *pVoipNarrate, char *pBody, int32_t iBodyLen, VoipNarrateGenderE eGender, const char *pText)
{
    int32_t iOffset=0;
    // format request body
    iOffset += ds_snzprintf(pBody+iOffset, iBodyLen-iOffset, "<speak version='1.0' xmlns=\"http://www.w3.org/2001/10/synthesis\" xml:lang='en-US'>");
    iOffset += ds_snzprintf(pBody+iOffset, iBodyLen-iOffset, "<voice name='Microsoft Server Speech Text to Speech Voice (en-US, %s)'>%s</voice>",
        (eGender == VOIPNARRATE_GENDER_FEMALE) ? "JessaRUS" : "BenjaminRUS", pText);
    iOffset += ds_snzprintf(pBody+iOffset, iBodyLen-iOffset, "</speak>");
    // return pointer to body
    return(pBody);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateFormatHeadGoogle

    \Description
        Format connection header for Google Text to Speech

    \Input *pVoipNarrate    - pointer to module state
    \Input *pUrl            - [out] buffer for formatted url
    \Input iUrlLen          - length of url buffer
    \Input *pHead           - [out] buffer for formatted request header
    \Input iHeadLen         - length of header buffer

    \Output
        int32_t             - negative=failure, else success

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipNarrateFormatHeadGoogle(VoipNarrateRefT *pVoipNarrate, char *pUrl, int32_t iUrlLen, char *pHead, int32_t iHeadLen)
{
    // format request header
    *pHead = '\0';
    // format request url
    ds_snzprintf(pUrl, iUrlLen, "%s?key=%s", pVoipNarrate->Config.strUrl, pVoipNarrate->Config.strKey);
    // return url
    return(pUrl);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateFormatBodyGoogle

    \Description
        Format request body for Google text-to-speech request

    \Input *pVoipNarrate    - pointer to module state
    \Input *pBody           - [out] buffer to hold request body
    \Input iBodyLen         - buffer length
    \Input eGender          - preferred gender for voice narration
    \Input *pText           - pointer to text request

    \Output
        int32_t             - negative=failure, else success

    \Notes
        Ref: https://cloud.google.com/text-to-speech/docs/reference/rest/

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
static char * _VoipNarrateFormatBodyGoogle(VoipNarrateRefT *pVoipNarrate, char *pBody, int32_t iBodyLen, VoipNarrateGenderE eGender, const char *pText)
{
    static const char *_strGender[VOIPNARRATE_NUMGENDERS] = { "SSML_VOICE_GENDER_UNSPECIFIED", "FEMALE", "MALE", "NEUTRAL" };
    static const char *_strVoice[VOIPNARRATE_NUMGENDERS] = { "en-US-Standard-D", "en-US-Standard-C", "en-US-Standard-B", "en-US-Standard-D" };
    JsonInit(pBody, iBodyLen, JSON_FL_WHITESPACE);

    JsonObjectStart(pBody, "input");
     JsonAddStr(pBody, "text", pText);
    JsonObjectEnd(pBody);

    JsonObjectStart(pBody, "voice");
     JsonAddStr(pBody, "languageCode", "en-US");
     JsonAddStr(pBody, "name", _strVoice[eGender]);
     JsonAddStr(pBody, "ssmlGender", _strGender[eGender]);  // we specify gender here, but it is unclear if it does anything
    JsonObjectEnd(pBody);

    JsonObjectStart(pBody, "audioConfig");
     JsonAddStr(pBody, "audioEncoding", "LINEAR16");
     JsonAddInt(pBody, "sampleRateHertz", VOIPNARRATE_SAMPLERATE);
    JsonObjectEnd(pBody);

    pBody = JsonFinish(pBody);
    return(pBody);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateFormatHeadAmazon

    \Description
        Format connection header for Amazon Polly

    \Input *pVoipNarrate    - pointer to module state
    \Input *pUrl            - [out] buffer for formatted url
    \Input iUrlLen          - length of url buffer
    \Input *pHead           - [out] buffer for formatted request header
    \Input iHeadLen         - length of header buffer

    \Output
        int32_t             - negative=failure, else success

    \Version 11/21/2018 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipNarrateFormatHeadAmazon(VoipNarrateRefT *pVoipNarrate, char *pUrl, int32_t iUrlLen, char *pHead, int32_t iHeadLen)
{
    int32_t iOffset=0;
    // format request header
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Content-Type: application/json\r\n");
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Accept: audio/wav; rate=%d\r\n", VOIPNARRATE_SAMPLERATE);
    // copy url
    ds_strnzcpy(pUrl, pVoipNarrate->Config.strUrl, iUrlLen);
    return(pUrl);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateFormatBodyAmazon

    \Description
        Format request body for Amazon Polly

    \Input *pVoipNarrate    - pointer to module state
    \Input *pBody           - [out] buffer to hold request body
    \Input iBodyLen         - buffer length
    \Input eGender          - preferred gender for voice narration
    \Input *pText           - pointer to text request

    \Output
        int32_t             - negative=failure, else success

    \Version 12/21/2018 (jbrookes)
*/
/********************************************************************************F*/
static char *_VoipNarrateFormatBodyAmazon(VoipNarrateRefT *pVoipNarrate, char *pBody, int32_t iBodyLen, VoipNarrateGenderE eGender, const char *pText)
{
    JsonInit(pBody, iBodyLen, JSON_FL_WHITESPACE);
    JsonAddStr(pBody, "OutputFormat", "pcm");
    JsonAddStr(pBody, "Text", pText);
    JsonAddStr(pBody, "VoiceId", (eGender == VOIPNARRATE_GENDER_FEMALE) ? "Joanna" : "Joey");
    pBody = JsonFinish(pBody);
    return(pBody);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateStreamCallbackGoogle

    \Description
        Decode Base64-encoded voice data

    \Input *pVoipNarrate    - pointer to module state
    \Input eStatus          - ProtoStream status
    \Input *pData           - base64-encoded input data
    \Input iDataSize        - input buffer length

    \Output
        int32_t             - negative=failure, else number of input bytes consumed

    \Version 10/30/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipNarrateStreamCallbackGoogle(VoipNarrateRefT *pVoipNarrate, ProtoStreamStatusE eStatus, const uint8_t *pData, int32_t iDataSize)
{
    int32_t iDataRead, iDataDecoded;

    // submit any base64-decoded data we have first
    if (pVoipNarrate->iVoiceLen > 0)
    {
        // if start of stream, see if we need to skip WAV header
        if (pVoipNarrate->bStart)
        {
            pVoipNarrate->iVoiceOff = _VoipNarrateSkipWavHeader(pVoipNarrate->aVoiceBuf, pVoipNarrate->iVoiceLen);
            pVoipNarrate->iVoiceLen -= pVoipNarrate->iVoiceOff;
            pVoipNarrate->bStart = FALSE;
        }
        // pass data to user
        iDataRead = pVoipNarrate->pVoiceDataCb(pVoipNarrate, pVoipNarrate->iUserIndex, (const int16_t *)(pVoipNarrate->aVoiceBuf+pVoipNarrate->iVoiceOff), pVoipNarrate->iVoiceLen, pVoipNarrate->pUserData);
        // mark data as read
        pVoipNarrate->iVoiceOff += iDataRead;
        pVoipNarrate->iVoiceLen -= iDataRead;
    }
    // if we don't have data to decode, or we still have decoded voice data that hasn't been consumed yet, don't decode more
    if ((pVoipNarrate->iVoiceLen > 0) || (iDataSize <= 0))
    {
        return(0);
    }
    pVoipNarrate->iVoiceOff = 0;

    // base64-decode voice data 
    if ((iDataRead = _VoipNarrateBase64Decode(pVoipNarrate, (char *)pVoipNarrate->aVoiceBuf, (iDataDecoded = sizeof(pVoipNarrate->aVoiceBuf), &iDataDecoded), (const char *)pData, iDataSize)) >= 0)
    {
        pVoipNarrate->iVoiceLen = iDataDecoded;
        pVoipNarrate->iSamplesInPhrase += iDataDecoded;
        pData = pVoipNarrate->aVoiceBuf;
    }
    else
    {
        NetPrintf(("voipnarrate: error; could not base64 decode data\n"));
        NetPrintMem(pData, iDataSize, "base64 data");
        pVoipNarrate->Metrics.uErrorCount += 1;
    }
    return(iDataRead);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateStreamCallback

    \Description
        Receive streamed voice data and submit it to callback

    \Input *pProtoStream    - ProtoStream module state
    \Input eStatus          - ProtoStream status
    \Input *pData           - base64-encoded input data
    \Input iDataSize        - input buffer length
    \Input *pUserData       - callback user data (VoipNarrate module ref)

    \Output
        int32_t             - negative=failure, else number of input bytes consumed

    \Version 10/30/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipNarrateStreamCallback(ProtoStreamRefT *pProtoStream, ProtoStreamStatusE eStatus, const uint8_t *pData, int32_t iDataSize, void *pUserData)
{
    VoipNarrateRefT *pVoipNarrate = (VoipNarrateRefT *)pUserData;
    int32_t iDataRead, iResult;
    char strError[256] = "";

    // handle start callback notification
    if (eStatus == PROTOSTREAM_STATUS_BEGIN)
    {
        pVoipNarrate->iSamplesInPhrase = 0;
        pVoipNarrate->Metrics.uDelay += NetTickDiff(NetTick(), pVoipNarrate->uTtsStartTime);
        pVoipNarrate->pVoiceDataCb(pVoipNarrate, pVoipNarrate->iUserIndex, (const int16_t *)pData, VOIPNARRATE_STREAM_START, pVoipNarrate->pUserData);
    }
    // handle end callback notification
    if (eStatus == PROTOSTREAM_STATUS_DONE)
    {
        // save metrics
        int32_t iPhraseDuration = ((pVoipNarrate->iSamplesInPhrase * 1000) / VOIPNARRATE_SAMPLERATE);
        pVoipNarrate->Metrics.uDurationMsRecv += iPhraseDuration;
        if (iPhraseDuration < VOIPNARRATE_EMPTY_AUDIO_THRESHOLD_MS)
        {
            pVoipNarrate->Metrics.uEmptyResultCount += 1;
        }
        
        // signal end of stream
        pVoipNarrate->pVoiceDataCb(pVoipNarrate, pVoipNarrate->iUserIndex, (const int16_t *)pData, VOIPNARRATE_STREAM_END, pVoipNarrate->pUserData);
        pVoipNarrate->bActive = FALSE;

        // check for a completion result that is not successful, and log error response (if any) to debug output
        if ((iResult = ProtoStreamStatus(pProtoStream, 'code', NULL, 0)) != PROTOHTTP_RESPONSE_SUCCESSFUL)
        {
            ProtoStreamStatus(pProtoStream, 'serr', strError, sizeof(strError));
            NetPrintf(("voipnarrate: stream failed with http result %d:\n%s\n", iResult, strError));
            pVoipNarrate->Metrics.uErrorCount += 1;
        }
    }

    // read data and pass it to callback, processing as necessary
    for (iDataRead = 0, iResult = 1; (iResult > 0) && (iDataSize > 0); )
    {
        if (pVoipNarrate->Config.eProvider != VOIPNARRATE_PROVIDER_GOOGLE)
        {
            // if start of stream, see if we need to skip WAV header
            if ((pVoipNarrate->bStart) && (iDataSize > 0))
            {
                iDataRead = _VoipNarrateSkipWavHeader(pData, iDataSize);
                pData += iDataRead;
                iDataSize -= iDataRead;
                pVoipNarrate->bStart = FALSE;
            }
            iResult = pVoipNarrate->pVoiceDataCb(pVoipNarrate, pVoipNarrate->iUserIndex, (const int16_t *)pData, iDataSize, pVoipNarrate->pUserData);
            pVoipNarrate->iSamplesInPhrase += iResult;
        }
        else
        {
            // google-specific processing to deal with base64 encoded audio
            iResult = _VoipNarrateStreamCallbackGoogle(pVoipNarrate, eStatus, pData, iDataSize);
        }

        iDataRead += iResult;
        iDataSize -= iResult;
        pData += iResult;
    }
    return(iDataRead);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateStart

    \Description
        Receive streamed voice data and submit it to callback

    \Input *pVoipNarrate    - pointer to module state
    \Input iUserIndex       - local user index of user who is requesting speech synthesis
    \Input eGender          - preferred gender for voice for narration
    \Input *pText           - pointer to text request

    \Output
        int32_t             - ProtoStream request result

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipNarrateStart(VoipNarrateRefT *pVoipNarrate, int32_t iUserIndex, VoipNarrateGenderE eGender, const char *pText)
{
    const char *pUrl, *pReq;
    char strUrl[256];
    int32_t iResult;

    // format header/url and request body
    if (pVoipNarrate->Config.eProvider == VOIPNARRATE_PROVIDER_MICROSOFT)
    {
        pUrl = _VoipNarrateFormatHeadMicrosoft(pVoipNarrate, strUrl, sizeof(strUrl), pVoipNarrate->strHead, sizeof(pVoipNarrate->strHead));
        pReq = _VoipNarrateFormatBodyMicrosoft(pVoipNarrate, pVoipNarrate->strBody, sizeof(pVoipNarrate->strBody), eGender, pText);
    }
    else if (pVoipNarrate->Config.eProvider == VOIPNARRATE_PROVIDER_GOOGLE)
    {
        pUrl = _VoipNarrateFormatHeadGoogle(pVoipNarrate, strUrl, sizeof(strUrl), pVoipNarrate->strHead, sizeof(pVoipNarrate->strHead));
        pReq = _VoipNarrateFormatBodyGoogle(pVoipNarrate, pVoipNarrate->strBody, sizeof(pVoipNarrate->strBody), eGender, pText);
    }
    else if (pVoipNarrate->Config.eProvider == VOIPNARRATE_PROVIDER_IBMWATSON)
    {
        pUrl = _VoipNarrateFormatHeadWatson(pVoipNarrate, strUrl, sizeof(strUrl), pVoipNarrate->strHead, sizeof(pVoipNarrate->strHead), eGender);
        pReq = _VoipNarrateFormatBodyWatson(pVoipNarrate, pVoipNarrate->strBody, sizeof(pVoipNarrate->strBody), pText);
    }
    else if (pVoipNarrate->Config.eProvider == VOIPNARRATE_PROVIDER_AMAZON)
    {
        pUrl = _VoipNarrateFormatHeadAmazon(pVoipNarrate, strUrl, sizeof(strUrl), pVoipNarrate->strHead, sizeof(pVoipNarrate->strHead));
        pReq = _VoipNarrateFormatBodyAmazon(pVoipNarrate, pVoipNarrate->strBody, sizeof(pVoipNarrate->strBody), eGender, pText);
    }
    else
    {
        NetPrintf(("voipnarrate: undefined provider\n"));
        return(-1);
    }
    NetPrintfVerbose((pVoipNarrate->iVerbose, 1, "voipnarrate: request body\n%s\n", pReq));
    pVoipNarrate->pBody = pReq;

    // set request header
    ProtoStreamControl(pVoipNarrate->pProtoStream, 'apnd', 0, 0, pVoipNarrate->strHead);

    pVoipNarrate->Metrics.uEventCount += 1;
    pVoipNarrate->Metrics.uCharCountSent += (uint32_t)strlen(pText);
    pVoipNarrate->uTtsStartTime = NetTick();

    // make the request
    if ((iResult = ProtoStreamOpen2(pVoipNarrate->pProtoStream, pUrl, pReq, PROTOSTREAM_FREQ_ONCE)) >= 0)
    {
        // mark as stream start and active
        pVoipNarrate->bStart = pVoipNarrate->bActive = TRUE;
    }
    else
    {
        NetPrintf(("voipnarrate: failed to open stream\n"));
        pVoipNarrate->Metrics.uErrorCount += 1;
    }

    // return to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateConfig

    \Description
        Configure the VoipNarrate module

    \Input *pVoipNarrate    - pointer to module state
    \Input *pConfig         - module configuration to set

    \Output
        uint32_t            - TRUE if configured successfully

    \Version 11/07/2018 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _VoipNarrateConfig(VoipNarrateRefT *pVoipNarrate, VoipNarrateConfigT *pConfig)
{
    uint8_t uRet = TRUE;
    NetCritEnter(NULL);
    if (pConfig->eProvider != VOIPNARRATE_PROVIDER_NONE)
    {
        ds_memcpy_s(&pVoipNarrate->Config, sizeof(pVoipNarrate->Config), pConfig, sizeof(*pConfig));
    }
    else
    {
        NetPrintfVerbose((pVoipNarrate->iVerbose, 0, "voipnarrate: narration disabled\n"));
        ds_memclr(&pVoipNarrate->Config, sizeof(pVoipNarrate->Config));
        uRet = FALSE;
    }
    NetCritLeave(NULL);
    return(uRet);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipNarrateCreate

    \Description
        Create the narration module

    \Input *pVoiceDataCb        - callback used to provide voice data
    \Input *pUserData           - callback user data

    \Output
        VoipNarrateRefT *       - new module state, or NULL

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
VoipNarrateRefT *VoipNarrateCreate(VoipNarrateVoiceDataCbT *pVoiceDataCb, void *pUserData)
{
    VoipNarrateRefT *pVoipNarrate;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // validate callback
    if (pVoiceDataCb == NULL)
    {
        NetPrintf(("voipnarrate: could not create module with null callback\n"));
        return(NULL);
    }

    // allocate and init module state
    if ((pVoipNarrate = DirtyMemAlloc(sizeof(*pVoipNarrate), VOIPNARRATE_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipnarrate: could not allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pVoipNarrate, sizeof(*pVoipNarrate));
    pVoipNarrate->iMemGroup = iMemGroup;
    pVoipNarrate->pMemGroupUserData = pMemGroupUserData;
    pVoipNarrate->pVoiceDataCb = pVoiceDataCb;
    pVoipNarrate->pUserData = pUserData;
    pVoipNarrate->iVerbose = 1;

    // allocate streaming module with a buffer to hold up to 1s of 16khz 16bit streaming audio
    if ((pVoipNarrate->pProtoStream = ProtoStreamCreate(16*2*1024)) == NULL)
    {
        VoipNarrateDestroy(pVoipNarrate);
        return(NULL);
    }
    // set protostream callback with a 20ms call rate
    ProtoStreamSetCallback(pVoipNarrate->pProtoStream, 20, _VoipNarrateStreamCallback, pVoipNarrate);
    // set protostream minimum data amount (for base64 decoding; four is the minimum amount but that produces one and a half samples, so we choose eight)
    ProtoStreamControl(pVoipNarrate->pProtoStream, 'minb', 8, 0, NULL);
    // set protostream debug level
    ProtoStreamControl(pVoipNarrate->pProtoStream, 'spam', 1, 0, NULL);
    // set keepalive
    ProtoStreamControl(pVoipNarrate->pProtoStream, 'keep', 1, 0, NULL);
    // set protostream http custom header callback, used to sign AWS requests
    ProtoStreamSetHttpCallback(pVoipNarrate->pProtoStream, _VoipNarrateCustomHeaderCb, NULL, pVoipNarrate);

    // configure for particular provider
    if (!_VoipNarrateConfig(pVoipNarrate, &_VoipNarrate_Config))
    {
        NetPrintf(("voipnarrate: could not configure for provider\n"));
        VoipNarrateDestroy(pVoipNarrate);
        return(NULL);
    }

    // return ref to caller
    return(pVoipNarrate);
}

/*F********************************************************************************/
/*!
    \Function VoipNarrateConfig

    \Description
        Set global state to configure the VoipNarrate modules

    \Input eProvider        - VOIPNARRATE_PROVIDER_* (VOIPNARRATE_PROVIDER_NONE to disable)
    \Input *pUrl            - pointer to url to use for tts requests
    \Input *pKey            - pointer to authentication key to use for tts requests

    \Version 11/07/2018 (jbrookes)
*/
/********************************************************************************F*/
void VoipNarrateConfig(VoipNarrateProviderE eProvider, const char *pUrl, const char *pKey)
{
    NetCritEnter(NULL);
    _VoipNarrate_Config.eProvider = eProvider;
    ds_strnzcpy(_VoipNarrate_Config.strUrl, pUrl, sizeof(_VoipNarrate_Config.strUrl));
    ds_strnzcpy(_VoipNarrate_Config.strKey, pKey, sizeof(_VoipNarrate_Config.strKey));
    NetCritLeave(NULL);
}

/*F********************************************************************************/
/*!
    \Function VoipNarrateDestroy

    \Description
        Destroy the VoipNarrate module

    \Input *pVoipNarrate    - pointer to module state

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
void VoipNarrateDestroy(VoipNarrateRefT *pVoipNarrate)
{
    // destroy protostream module, if allocated
    if (pVoipNarrate->pProtoStream != NULL)
    {
        ProtoStreamDestroy(pVoipNarrate->pProtoStream);
    }
    // release any queued requests
    while (pVoipNarrate->pRequest != NULL)
    {
        _VoipNarrateRequestGet(pVoipNarrate, NULL);
    }
    // dispose of module memory
    DirtyMemFree(pVoipNarrate, VOIPNARRATE_MEMID, pVoipNarrate->iMemGroup, pVoipNarrate->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function VoipNarrateInput

    \Description
        Input text to be convert to speech

    \Input *pVoipNarrate    - pointer to module state
    \Input iUserIndex       - local user index of user who is requesting speech synthesis
    \Input eGender          - preferred gender for voice narration
    \Input *pText           - text to be converted

    \Output
        int32_t             - zero=success, otherwise=failure

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipNarrateInput(VoipNarrateRefT *pVoipNarrate, int32_t iUserIndex, VoipNarrateGenderE eGender, const char *pText)
{
    // make sure a provider is configured
    if (pVoipNarrate->Config.eProvider == VOIPNARRATE_PROVIDER_NONE)
    {
        NetPrintfVerbose((pVoipNarrate->iVerbose, 0, "voipnarrate: no provider configured\n"));
        return(-1);
    }
    // handle if there is already narration ongoing
    if (pVoipNarrate->bActive)
    {
        NetPrintfVerbose((pVoipNarrate->iVerbose, 1, "voipnarrate: queueing request '%s'\n", pText));
        return(_VoipNarrateRequestAdd(pVoipNarrate, iUserIndex, eGender, pText));
    }
    // if ready, start the request
    return(_VoipNarrateStart(pVoipNarrate, iUserIndex, eGender, pText));
}

/*F********************************************************************************/
/*!
    \Function VoipNarrateStatus

    \Description
        Get module status.

    \Input *pVoipNarrate    - pointer to module state
    \Input iStatus          - status selector
    \Input iValue           - selector specific
    \Input *pBuffer         - selector specific
    \Input iBufSize         - selector specific

    \Output
        int32_t             - selector specific

    \Notes
        Other status codes are passed down to the stream transport handler.

    \verbatim
        'ttsm' - get the VoipTextToSpeechMetricsT via pBuffer
    \endverbatim

    \Version 11/15/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipNarrateStatus(VoipNarrateRefT *pVoipNarrate, int32_t iStatus, int32_t iValue, void *pBuffer, int32_t iBufSize)
{
    if (iStatus == 'ttsm')
    {
        if ((pBuffer != NULL) && (iBufSize >= (int32_t)sizeof(VoipTextToSpeechMetricsT)))
        {
            ds_memcpy_s(pBuffer, iBufSize, &pVoipNarrate->Metrics, sizeof(VoipTextToSpeechMetricsT));
            return(0);
        }
        return(-1);
    }
    return(ProtoStreamStatus(pVoipNarrate->pProtoStream, iStatus, pBuffer, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function VoipNarrateControl

    \Description
        Set control options

    \Input *pVoipNarrate    - pointer to module state
    \Input iControl         - control selector
    \Input iValue           - selector specific
    \Input iValue2          - selector specific
    \Input *pValue          - selector specific

    \Output
        int32_t             - selector specific

    \Notes
        iStatus can be one of the following:

        \verbatim
            'ctsm' - clear text to speech metrics in VoipTextToSpeechMetricsT
            'spam' - set verbose debug level (debug only)
        \endverbatim

        Unhandled codes are passed through to the stream transport handler

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipNarrateControl(VoipNarrateRefT *pVoipNarrate, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'ctsm')
    {
        ds_memclr(&(pVoipNarrate->Metrics), sizeof(pVoipNarrate->Metrics));
        return(0);
    }
    #if DIRTYCODE_LOGGING
    // set verbosity for us and pass through to stream transport handler
    if (iControl == 'spam')
    {
        pVoipNarrate->iVerbose = iValue;
    }
    #endif
    // if not handled, let stream transport handler take a stab at it
    return(ProtoStreamControl(pVoipNarrate->pProtoStream, iControl, iValue, iValue2, pValue));
}

/*F********************************************************************************/
/*!
    \Function VoipNarrateUpdate

    \Description
        Update the narration module

    \Input *pVoipNarrate    - pointer to module state

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
void VoipNarrateUpdate(VoipNarrateRefT *pVoipNarrate)
{
    // see if we need to start a queued narration request
    if ((pVoipNarrate->pRequest != NULL) && !pVoipNarrate->bActive)
    {
        VoipNarrateRequestT Request;
        _VoipNarrateRequestGet(pVoipNarrate, &Request);
        _VoipNarrateStart(pVoipNarrate, Request.iUserIndex, Request.eGender, Request.strText);
    }
    // give life to stream module
    ProtoStreamUpdate(pVoipNarrate->pProtoStream);
}
