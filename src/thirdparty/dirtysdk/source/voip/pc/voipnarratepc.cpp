/*H********************************************************************************/
/*!
\File voipnarratepc.cpp

    \Description
        Voip narration API wrapping SAPI text to speech APIs for PC

    \Copyright
        Copyright 2018 Electronic Arts

    \Version 12/4/2018 (tcho) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#pragma warning(push, 0)
#include <sapi.h>
#include <sphelper.h>
#pragma warning(pop)

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "DirtySDK/voip/voipdef.h"
#include "DirtySDK/voip/voipnarrate.h"
/*** Defines **********************************************************************/
// must be the same as voipheadsetpc
#define VOIPNARRATE_SAMPLEWIDTH            (2)                                                         //!< sample size; 16-bit samples
#define VOIPNARRATE_FRAMEDURATION          (20)                                                        //!< frame duration in milliseconds; 20ms
#define VOIPNARRATE_FRAMESAMPLES           ((VOIPNARRATE_SAMPLERATE*VOIPNARRATE_FRAMEDURATION)/1000)   //!< samples per frame (20ms; 8khz=160, 11.025khz=220.5, 16khz=320)
#define VOIPNARRATE_FRAMESIZE              (VOIPNARRATE_FRAMESAMPLES*VOIPNARRATE_SAMPLEWIDTH)          //!< frame size in bytes; 640
#define VOIPNARRATE_READLEN                (VOIPNARRATE_FRAMESIZE+30)
#define VOIPNARRATE_BUFFER_LEN             (50000)
/*** Macros ***********************************************************************/
/*** Type Definitions *************************************************************/

//! narration request data
typedef struct VoipNarrateRequestT
{
    struct VoipNarrateRequestT *pNext;
    VoipNarrateGenderE eGender;
    char strText[VOIPNARRATE_INPUT_MAX];
} VoipNarrateRequestT;

//! TTS States
typedef enum VoipNarrateVoiceStateE
{
    VOIPNARRATE_VOICE_UNINITIALIZED,
    VOIPNARRATE_VOICE_UNINITIALIZING,
    VOIPNARRATE_VOICE_READY,
    VOIPNARRATE_VOICE_BUSY,
    VOIPNARRATE_VOICE_INITIALIZATION_FAILED
} VoipNarrateVoiceStateE;

//! tts result voice stream
class VoipNarrateVoiceStream : public IStream
{
    public:
        VoipNarrateVoiceStream(int32_t iMemGroup, void *pMemGroupUserData);
        virtual ~VoipNarrateVoiceStream();

        // from IStream
        HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
        HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize);
        HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags);
        HRESULT STDMETHODCALLTYPE Revert(void);
        HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
        HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
        HRESULT STDMETHODCALLTYPE Clone(IStream **ppStream);
        HRESULT STDMETHODCALLTYPE Stat(STATSTG *pstatstg, DWORD grfStatFlag);
        HRESULT STDMETHODCALLTYPE CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);

        // from ISequentialStream
        HRESULT STDMETHODCALLTYPE Read(void *pData, ULONG uDataLen, ULONG *pRead);
        HRESULT STDMETHODCALLTYPE Write(const void *pData, ULONG uDataLen, ULONG *pWritten);

        // from IUnknown
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject);
        ULONG STDMETHODCALLTYPE AddRef(void);
        ULONG STDMETHODCALLTYPE Release(void);

        // useful methods
        uint32_t GetDataSize();

    private:
        NetCritT m_BufferCrit;
        uint8_t* m_pBuffer;
        uint32_t m_uBufferSize;
        uint32_t m_uReadPos;
        uint32_t m_uWritePos;
        int32_t  m_iMemGroup;
        void    *m_pMemGroupUserData;
        LONG    m_uRefCount;
};

struct VoipNarrateRefT
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    VoipNarrateVoiceDataCbT *pVoiceDataCb;  //!< user callback used to provide voice data
    void *pUserData;                        //!< user data for user callback

    VoipNarrateRequestT *pRequest;          //!< list of queued requests, if any

    VoipTextToSpeechMetricsT Metrics;       //!< Usage metrics of the narration module
    uint32_t uTtsStartTime;

    uint8_t                     bFirstSynthOfPhrase;
    uint8_t                     bChangeLang;
    int32_t                     iLangCode;
    NetCritT                    VoiceCrit;
    ISpVoice                    *pVoice;
    ISpStream                   *pVoiceStream;
    VoipNarrateVoiceStream      *pBaseStream;
    VoipNarrateVoiceStateE      eVoiceState;
    VoipNarrateGenderE          eGender;
};
/*** Private Functions ************************************************************/
/*F********************************************************************************/
/*!
    \Function _VoipNarrateUninitialize

    \Description
        Uninitialize SAPI

    \Input *pVoipNarrate    - pointer to module state

    \Version 12/05/2018 (tcho)
*/
/********************************************************************************F*/
static void _VoipNarrateUninitialize(VoipNarrateRefT *pVoipNarrate)
{
    // release SpStream interface
    if (pVoipNarrate->pVoiceStream != NULL)
    {
        pVoipNarrate->pVoiceStream->Release();
        pVoipNarrate->pVoiceStream = NULL;
    }

    // release SpVoice interface
    if (pVoipNarrate->pVoice != NULL)
    {
        pVoipNarrate->pVoice->Release();
        pVoipNarrate->pVoice = NULL;
        CoUninitialize();
    }

    // set state
    NetCritEnter(&pVoipNarrate->VoiceCrit);
    pVoipNarrate->eVoiceState = VOIPNARRATE_VOICE_UNINITIALIZED;
    NetCritLeave(&pVoipNarrate->VoiceCrit);
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateInitialize

    \Description
        Initialize SAPI

    \Input *pVoipNarrate    - pointer to module state

    \Version 12/05/2018 (tcho)
*/
/********************************************************************************F*/
static void _VoipNarrateInitialize(VoipNarrateRefT *pVoipNarrate)
{
    HRESULT hResult;
    WAVEFORMATEX WaveFormatEx = { WAVE_FORMAT_PCM, 1, VOIPNARRATE_SAMPLERATE, VOIPNARRATE_SAMPLERATE * VOIPNARRATE_SAMPLEWIDTH, 2, 16, 0 };

    // initialize COM library
    hResult = CoInitialize(NULL);
    if (FAILED(hResult) && (hResult != RPC_E_CHANGED_MODE))
    {
        NetPrintf(("voipnarratepc: failed to initialize COM library! (err=%s)\n", DirtyErrGetName(hResult)));
        pVoipNarrate->eVoiceState = VOIPNARRATE_VOICE_INITIALIZATION_FAILED;
        return;
    }

    // create our own IStream
    pVoipNarrate->pBaseStream = new (DirtyMemAlloc(sizeof(VoipNarrateVoiceStream), VOIP_MEMID, pVoipNarrate->iMemGroup, pVoipNarrate->pMemGroupUserData)) VoipNarrateVoiceStream(pVoipNarrate->iMemGroup, pVoipNarrate->pMemGroupUserData);

    // create SpVoice interface
    if ((hResult = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoipNarrate->pVoice)) != S_OK)
    {
        NetPrintf(("voipnarratepc: failed to create ISpVoice interface!(err=%s)\n", DirtyErrGetName(hResult)));
        pVoipNarrate->eVoiceState = VOIPNARRATE_VOICE_INITIALIZATION_FAILED;
        CoUninitialize();
        return;
    }

    // create SpStream interface
    if ((hResult = CoCreateInstance(CLSID_SpStream, NULL, CLSCTX_ALL, IID_ISpStream, (void **)&pVoipNarrate->pVoiceStream)) != S_OK)
    {
        NetPrintf(("voipnarratepc: failed to create ISpStream interface!(err=%s)\n", DirtyErrGetName(hResult)));
        pVoipNarrate->pVoice->Release();
        pVoipNarrate->pVoice = NULL;
        pVoipNarrate->eVoiceState = VOIPNARRATE_VOICE_INITIALIZATION_FAILED;
        CoUninitialize();
        return;
    }

    // set the voice stream's base stream and wav format
    if (pVoipNarrate->pVoiceStream->SetBaseStream(pVoipNarrate->pBaseStream, SPDFID_WaveFormatEx, &WaveFormatEx) != S_OK)
    {
        NetPrintf(("voipnarratepc: failed to set base stream!\n"));
    }

    // set audio output of the voice interface
    if (pVoipNarrate->pVoice->SetOutput(pVoipNarrate->pVoiceStream, TRUE) != S_OK)
    {
        NetPrintf(("voipnarratepc: failed to set voice stream!\n"));
    }

    // set interest
    if (pVoipNarrate->pVoice->SetInterest(SPFEI_ALL_TTS_EVENTS, NULL) != S_OK)
    {
        NetPrintf(("voipnarratepc: failed to set voice interest!\n"));
    }

    pVoipNarrate->eVoiceState = VOIPNARRATE_VOICE_READY;
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateGenderChange

    \Description
        Change sythesize voice gender (right only english is supported for now)

    \Input *pVoipNarrate    - pointer to module state
    \Input eGender          - preferred gender for voice narration

    \Output
        int32_t             - negative=failure, else success

    

    \Version 12/04/2018 (tcho)
*/
/********************************************************************************F*/
static int32_t _VoipNarrateGenderChange(VoipNarrateRefT *pVoipNarrate, VoipNarrateGenderE eGender)
{
    wchar_t strLang[20];
    wchar_t strGender[20];
    ISpObjectToken *pVoiceToken;
    IEnumSpObjectTokens *pVoiceTokenList;

    _snwprintf(strLang, sizeof(strLang), L"Language=%d", pVoipNarrate->iLangCode);
    _snwprintf(strGender, sizeof(strGender), L"Gender=%s", eGender == VOIPNARRATE_GENDER_FEMALE ? L"Female" : L"Male");

    if (SpEnumTokens(SPCAT_VOICES, strLang, strGender, &pVoiceTokenList) != S_OK)
    {
        NetPrintf(("voipheadsetpc: cannot retrieve voice token list\n"));
        return(-1);
    }

    pVoiceTokenList->Next(1, &pVoiceToken, NULL);

    if (pVoiceToken != NULL)
    {
        pVoipNarrate->pVoice->SetVoice(pVoiceToken);
    }
    else
    {
        NetPrintf(("voipheadsetpc: Lang:%i Gender:%s voice not found \n", pVoipNarrate->iLangCode, pVoipNarrate->eGender == VOIPNARRATE_GENDER_FEMALE ? "Female" : "Male"));
        return(-2);
    }

    return(0);
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

    \Version 12/04/2018 (tcho)
*/
/********************************************************************************F*/
static int32_t _VoipNarrateRequestAdd(VoipNarrateRefT *pVoipNarrate, int32_t iUserIndex, VoipNarrateGenderE eGender, const char *pText)
{
    VoipNarrateRequestT *pRequest;

    // allocate and clear the request
    if ((pRequest = (VoipNarrateRequestT *)DirtyMemAlloc(sizeof(*pRequest), VOIPNARRATE_MEMID, pVoipNarrate->iMemGroup, pVoipNarrate->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipnarratepc: could not allocate request\n"));
        pVoipNarrate->Metrics.uErrorCount += 1;
        return(-1);
    }
    ds_memclr(pRequest, sizeof(*pRequest));

    // copy the request data
    ds_strnzcpy(pRequest->strText, pText, sizeof(pRequest->strText));
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
    \Input *pRequest        - [out] storage for request

    \Version 12/04/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipNarrateRequestGet(VoipNarrateRefT *pVoipNarrate, VoipNarrateRequestT *pRequest)
{
    VoipNarrateRequestT **ppRequest;
    // get oldest request (we add to head, so get from tail)
    for (ppRequest = &pVoipNarrate->pRequest; (*ppRequest)->pNext != NULL; ppRequest = &((*ppRequest)->pNext))
        ;
    // copy request
    ds_memcpy_s(pRequest, sizeof(*pRequest), *ppRequest, sizeof(**ppRequest));
    // free request
    DirtyMemFree(*ppRequest, VOIPNARRATE_MEMID, pVoipNarrate->iMemGroup, pVoipNarrate->pMemGroupUserData);
    // remove from list
    *ppRequest = NULL;
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateStart

    \Description
        Starts SAPI voice sythesis

    \Input *pVoipNarrate    - pointer to module state
    \Input eGender          - preferred gender for voice for narration
    \Input *pText           - pointer to text request

    \Version 12/04/2018 (tcho)
*/
/********************************************************************************F*/
static void _VoipNarrateStart(VoipNarrateRefT *pVoipNarrate, VoipNarrateGenderE eGender, const char *pText)
{
    HRESULT hResult;
    int32_t iStringLength = 0;
    int32_t iSizeNeeded = MultiByteToWideChar(CP_UTF8, 0, pText, -1, NULL, 0);
    wchar_t *pWideText = (wchar_t *)DirtyMemAlloc((int32_t)(sizeof(wchar_t) * iSizeNeeded), VOIP_MEMID, pVoipNarrate->iMemGroup, pVoipNarrate->pMemGroupUserData);

    iStringLength = MultiByteToWideChar(CP_UTF8, 0, pText, -1, pWideText, iSizeNeeded);
    pVoipNarrate->Metrics.uEventCount++;
    pVoipNarrate->Metrics.uCharCountSent += iStringLength;

    // change the gender if needed
    if ((pVoipNarrate->eGender != eGender) || (pVoipNarrate->bChangeLang == TRUE))
    {
        VoipNarrateGenderE eTempGender = eGender;

        if (eGender == VOIPNARRATE_GENDER_NEUTRAL)
        {
            eTempGender = pVoipNarrate->eGender;
        }

        if (_VoipNarrateGenderChange(pVoipNarrate, eTempGender) == 0)
        {
            pVoipNarrate->eGender = eGender;
            pVoipNarrate->bChangeLang = FALSE;
        }
    }

    // submit text for sythesis
    hResult = pVoipNarrate->pVoice->Speak(pWideText, SPF_ASYNC, 0);
    pVoipNarrate->uTtsStartTime = NetTick();

    DirtyMemFree(pWideText, VOIP_MEMID, pVoipNarrate->iMemGroup, pVoipNarrate->pMemGroupUserData);
    
    if (hResult != S_OK)
    {
        pVoipNarrate->Metrics.uErrorCount++;
        return;
    }

    pVoipNarrate->bFirstSynthOfPhrase = TRUE;
    pVoipNarrate->eVoiceState = VOIPNARRATE_VOICE_BUSY;
}

/*F********************************************************************************/
/*!
    \Function _VoipNarrateProcessResult

    \Description
        Receive streamed voice data and submit it to callback

    \Input *pVoipNarrate    - pointer to module state

    \Version 12/04/2018 (tcho)
*/
/********************************************************************************F*/
static void _VoipNarrateProcessResult(VoipNarrateRefT *pVoipNarrate)
{
    ULONG iReadLen = 0;
    SPVOICESTATUS status;
    uint8_t buffer[VOIPNARRATE_READLEN];
    VoipNarrateVoiceStream *pStream = (VoipNarrateVoiceStream *)(pVoipNarrate->pBaseStream);

    // get voice synth status
    pVoipNarrate->pVoice->GetStatus(&status, NULL);
    ds_memclr(&buffer, VOIPNARRATE_READLEN);

    if (status.dwRunningState == SPRS_DONE)
    {
        if (pStream->GetDataSize() != 0)
        {
            // time how long it took to get the tts results
            if (pVoipNarrate->bFirstSynthOfPhrase)
            {
                pVoipNarrate->Metrics.uDelay += NetTickDiff(NetTick(), pVoipNarrate->uTtsStartTime);
            }

            pStream->Read(&buffer[0], VOIPNARRATE_READLEN, &iReadLen);
            pVoipNarrate->Metrics.uDurationMsRecv += ((iReadLen * 1000) / VOIPNARRATE_SAMPLERATE);

            if (iReadLen != 0)
            {
                // if this is the beginning of a new phrase signal stream start
                if (pVoipNarrate->bFirstSynthOfPhrase == TRUE)
                {
                    pVoipNarrate->bFirstSynthOfPhrase = FALSE;
                    pVoipNarrate->pVoiceDataCb(pVoipNarrate, 0, (const int16_t *)buffer, VOIPNARRATE_STREAM_START, pVoipNarrate->pUserData);
                }
                pVoipNarrate->pVoiceDataCb(pVoipNarrate, 0, (const int16_t *)buffer, iReadLen, pVoipNarrate->pUserData);
            }
        }
        else
        {
            if (pVoipNarrate->bFirstSynthOfPhrase)
            {
                pVoipNarrate->Metrics.uEmptyResultCount++;
            }

            // signal stream end
            pVoipNarrate->pVoiceDataCb(pVoipNarrate, 0, (const int16_t *)buffer, VOIPNARRATE_STREAM_END, pVoipNarrate->pUserData);
            pVoipNarrate->eVoiceState = VOIPNARRATE_VOICE_READY;
        }
    }
}

/*** Public Functions *************************************************************/
/*F********************************************************************************/
/*!
    \Function VoipNarrateCreate

    \Description
        Create the narration module

    \Input *pVoiceDataCb        - callback used to provide voice data
    \Input *pUserData           - callback user data

    \Output
        VoipNarrateRefT *       - new module state, or NULL

    \Version 12/04/2018 (tcho)
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
        NetPrintf(("voipnarratepc: could not create module with null callback\n"));
        return(NULL);
    }

    // allocate and init module state
    if ((pVoipNarrate = (VoipNarrateRefT *)DirtyMemAlloc(sizeof(*pVoipNarrate), VOIPNARRATE_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipnarratepc: could not allocate module state\n"));
        return(NULL);
    }
    
    ds_memclr(pVoipNarrate, sizeof(*pVoipNarrate));
    pVoipNarrate->iMemGroup = iMemGroup;
    pVoipNarrate->pMemGroupUserData = pMemGroupUserData;
    pVoipNarrate->iLangCode = 409; // 409 is enUS
    pVoipNarrate->eGender = VOIPNARRATE_GENDER_MALE;
    pVoipNarrate->pVoiceDataCb = pVoiceDataCb;
    pVoipNarrate->pUserData = pUserData;
    NetCritInit(&pVoipNarrate->VoiceCrit, "voipnarratepc-tts");

    return(pVoipNarrate);
}

/*F********************************************************************************/
/*!
    \Function VoipNarrateConfig

    \Description
        Configure the VoipNarrate module

    \Input eProvider        - VOIPNARRATE_PROVIDER_*
    \Input *pUrl            - pointer to url to use for tts requests
    \Input *pKey            - pointer to authentication key to use for tts requests

    \Version 12/04/2018 (tcho)
*/
/********************************************************************************F*/
void VoipNarrateConfig(VoipNarrateProviderE eProvider, const char *pUrl, const char *pKey)
{
    NetPrintf(("voipnarratepc: VoipNarrateConfig() is not implemented on PC\n"));
}

/*F********************************************************************************/
/*!
    \Function VoipNarrateDestroy

    \Description
    Destroy the VoipNarrate module

    \Input *pVoipNarrate    - pointer to module state

    \Version 12/04/2018 (tcho)
*/
/********************************************************************************F*/
void VoipNarrateDestroy(VoipNarrateRefT *pVoipNarrate)
{
    VoipNarrateRequestT *pRequest = pVoipNarrate->pRequest;

    // free queued up request 
    if (pRequest != NULL)
    {
        for (pRequest = pVoipNarrate->pRequest; pRequest->pNext != NULL; pRequest = pRequest->pNext)
        {
            DirtyMemFree(pRequest, VOIPNARRATE_MEMID, pVoipNarrate->iMemGroup, pVoipNarrate->pMemGroupUserData);
        }
    }

    // dispose of the crit
    NetCritKill(&pVoipNarrate->VoiceCrit);

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

    \Version 12/04/2018 (tcho)
*/
/********************************************************************************F*/
int32_t VoipNarrateInput(VoipNarrateRefT *pVoipNarrate, int32_t iUserIndex, VoipNarrateGenderE eGender, const char *pText)
{
    return(_VoipNarrateRequestAdd(pVoipNarrate, iUserIndex, eGender, pText));
}

/*F********************************************************************************/
/*!
    \Function VoipNarrateUpdate

    \Description
        Update the narration module

    \Input *pVoipNarrate    - pointer to module state

    \Version 12/04/2018 (tcho)
*/
/********************************************************************************F*/
void VoipNarrateUpdate(VoipNarrateRefT *pVoipNarrate)
{
    // initialize SAPI
    if (pVoipNarrate->eVoiceState == VOIPNARRATE_VOICE_UNINITIALIZED)
    {
        _VoipNarrateInitialize(pVoipNarrate);
    }

    // see if we need to start a queued narration request
    if ((pVoipNarrate->pRequest != NULL) && (pVoipNarrate->eVoiceState == VOIPNARRATE_VOICE_READY))
    {
        VoipNarrateRequestT Request;
        _VoipNarrateRequestGet(pVoipNarrate, &Request);
        _VoipNarrateStart(pVoipNarrate, Request.eGender, Request.strText);
    }

    if (pVoipNarrate->eVoiceState == VOIPNARRATE_VOICE_BUSY)
    {
        _VoipNarrateProcessResult(pVoipNarrate);
    }

    if (pVoipNarrate->eVoiceState == VOIPNARRATE_VOICE_UNINITIALIZING)
    {
        _VoipNarrateUninitialize(pVoipNarrate);
    }
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

    \Version 12/05/2018 (tcho)
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
    
    return(-1);
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
        'lang' - set language code
        'uvoc' - uninitialize SAPI
    \endverbatim

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
    if (iControl == 'lang')
    {
        NetCritEnter(&pVoipNarrate->VoiceCrit);
        if (pVoipNarrate->iLangCode != iValue)
        {
            pVoipNarrate->bChangeLang = TRUE;
            pVoipNarrate->iLangCode = iValue;
        }
        NetCritLeave(&pVoipNarrate->VoiceCrit);
        return(0);
    }
    if (iControl == 'uvoc')
    {
        NetCritEnter(&pVoipNarrate->VoiceCrit);
        pVoipNarrate->eVoiceState = VOIPNARRATE_VOICE_UNINITIALIZING;
        NetCritLeave(&pVoipNarrate->VoiceCrit);
        return(0);
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \relates     VoipNarrateVoiceStream
    \Function    VoipNarrateVoiceStream

    \Description
        Constructor for VoipNarrateVoiceStream

    \Input iMemGroup          - iMemGroup
    \Input pMemGroupUserData  - pMemGroupUserData

    \Version 05/17/2018 (tcho)
*/
/********************************************************************************F*/
VoipNarrateVoiceStream::VoipNarrateVoiceStream(int32_t iMemGroup, void *pMemGroupUserData)
{
    m_pBuffer = NULL;
    m_uReadPos = 0;
    m_uWritePos = 0;
    m_uBufferSize = 0;
    m_uRefCount = 0;
    m_iMemGroup = iMemGroup;
    m_pMemGroupUserData = pMemGroupUserData;
    NetCritInit(&m_BufferCrit, "voipnarrate-tts-stream");
}

/*F********************************************************************************/
/*!
    \relates     VoipNarrateVoiceStream
    \fn          ~VoipNarrateVoiceStream

    \Description
        Destructor for VoipNarrateVoiceStream

    \Version 05/17/2018 (tcho)
*/
/********************************************************************************F*/
VoipNarrateVoiceStream::~VoipNarrateVoiceStream()
{
    if (m_pBuffer != NULL)
    {
        DirtyMemFree(m_pBuffer, VOIP_MEMID, m_iMemGroup, m_pMemGroupUserData);
    }

    NetCritKill(&m_BufferCrit);
}

/*F********************************************************************************/
/*!
    \relates     VoipNarrateVoiceStream
    \Function    GetDataSize

    \Description
        Return current size of the stream

    \Output
    uint32_t - size of the stream

    \Version 05/17/2018 (tcho)
*/
/********************************************************************************F*/
uint32_t VoipNarrateVoiceStream::GetDataSize()
{
    return m_uWritePos;
}

/*F********************************************************************************/
/*!
    \relates     VoipNarrateVoiceStream
    \Function    QueryInterface

    \Description
        Returns the interface of the VoipNarrateVoiceStream

    \Input iid          - interface id
    \Input **ppvObject  - pointer to store the pointer to the interface

    \Output
    HRESULT         - S_OK if successful, E_NOINTERFACE if not successful

    \Version 05/17/2018 (tcho)
*/
/********************************************************************************F*/
HRESULT STDMETHODCALLTYPE VoipNarrateVoiceStream::QueryInterface(REFIID iid, void ** ppvObject)
{
    if (iid == __uuidof(IUnknown)
        || iid == __uuidof(IStream)
        || iid == __uuidof(ISequentialStream))
    {
        *ppvObject = (IStream *)(this);
        AddRef();
        return S_OK;
    }
    else
        return E_NOINTERFACE;
}

/*F********************************************************************************/
/*!
    \relates     VoipNarrateVoiceStream
    \Function    AddRef

    \Description
        Intcrement the ref count for VoipNarrateVoiceStream

    \Output
    ULONG    - returns the current ref count

    \Version 05/17/2018 (tcho)
*/
/********************************************************************************F*/
ULONG STDMETHODCALLTYPE VoipNarrateVoiceStream::AddRef(void)
{
    return (ULONG)InterlockedIncrement(&m_uRefCount);
}

/*F********************************************************************************/
/*!
    \relates     VoipNarrateVoiceStream
    \Function    Release

    \Description
        Free the VoipNarrateVoiceStream

    \Output
    ULONG    - returns the current ref count

    \Version 05/17/2018 (tcho)
*/
/********************************************************************************F*/
ULONG STDMETHODCALLTYPE VoipNarrateVoiceStream::Release(void)
{
    ULONG res = (ULONG)InterlockedDecrement(&m_uRefCount);
    if (res == 0)
    {
        this->~VoipNarrateVoiceStream();
        DirtyMemFree(this, VOIP_MEMID, m_iMemGroup, m_pMemGroupUserData);
    }
    return res;
}


/*F********************************************************************************/
/*!
    \relates     VoipNarrateVoiceStream
    \Function    Read

    \Description
        Read from the stream

    \Input *pData       - pointer to buffer where data will be read into
    \Input uDataLen     - the max size of pData
    \Input *pRead       - output the actual amount of data that is read

    \Output
    HRESULT         - S_OK if sucessful, S_FALSE if not

    \Version 05/17/2018 (tcho)
*/
/********************************************************************************F*/
HRESULT STDMETHODCALLTYPE VoipNarrateVoiceStream::Read(void *pData, ULONG uDataLen, ULONG *pRead)
{
    if ((pData == NULL) || (pRead == NULL))
    {
        return(STG_E_INVALIDPOINTER);
    }

    if (NetCritTry(&m_BufferCrit) == 0)
    {
        return (S_OK);
    }

    // remaining data is bigger than or equal to bytes requested to be read
    if ((m_uWritePos - m_uReadPos) >= uDataLen)
    {
        *pRead = uDataLen;
    }
    // remaining data is smaller then bytes requested to be read
    else
    {
        *pRead = (m_uWritePos - m_uReadPos);
    }

    // check to see if we have a buffer and the read len is not zero
    if ((m_pBuffer == NULL) || (*pRead == 0))
    {
        *pRead = 0;
        NetCritLeave(&m_BufferCrit);
        return(S_FALSE);
    }

    // copy data
    ds_memcpy(pData, m_pBuffer, *pRead);
    m_uReadPos += *pRead;

    //compact the buffer
    memmove(m_pBuffer, m_pBuffer + m_uReadPos, m_uWritePos - m_uReadPos);
    m_uWritePos -= m_uReadPos;
    m_uReadPos = 0;

    NetCritLeave(&m_BufferCrit);
    return(S_OK);
}

/*F********************************************************************************/
/*!
    \relates     VoipNarrateVoiceStream
    \Function    Write

    \Description
        Write to the stream

    \Input *pData       - data to be written to stream
    \Input uDataLen     - the size of pData
    \Input *pWritten    - output the actual amount of data that is written

    \Output
        HRESULT         - S_OK if sucessful, S_FALSE if not

    \Version 05/17/2018 (tcho)
*/
/********************************************************************************F*/
HRESULT STDMETHODCALLTYPE VoipNarrateVoiceStream::Write(const void *pData, ULONG uDataLen, ULONG *pWritten)
{
    if (pData == NULL)
    {
        return(STG_E_INVALIDPOINTER);
    }

    NetCritEnter(&m_BufferCrit);

    // check to see if we have enough room to write the data
    if ((m_uBufferSize - m_uWritePos) < uDataLen)
    {
        int32_t iNewBufferSize;
        uint8_t *pNewBuffer;

        // set new buffer size
        // we are doubling the buffer size everytime
        m_uBufferSize ? (iNewBufferSize = 2 * m_uBufferSize) : (iNewBufferSize = VOIPNARRATE_BUFFER_LEN);

        // need to expand buffer
        if ((pNewBuffer = (uint8_t *)DirtyMemAlloc(iNewBufferSize, VOIP_MEMID, m_iMemGroup, m_pMemGroupUserData)) == NULL)
        {
            NetPrintf(("voipheadsetpc: cannot allocate memory for tts stream buffer!\n"));
            NetCritLeave(&m_BufferCrit);
            return(STG_E_MEDIUMFULL);
        }

        ds_memclr(pNewBuffer, iNewBufferSize);

        // cleanup existing pBuffer and copy contents
        if (m_pBuffer != NULL)
        {
            ds_memcpy(pNewBuffer, m_pBuffer, m_uBufferSize);
            DirtyMemFree(m_pBuffer, VOIP_MEMID, m_iMemGroup, m_pMemGroupUserData);
        }

        m_pBuffer = pNewBuffer;
        m_uBufferSize = iNewBufferSize;
    }

    // now write the new data into the buffer
    ds_memcpy(m_pBuffer + m_uWritePos, pData, uDataLen);
    m_uWritePos += uDataLen;

    if (pWritten != NULL)
    {
        *pWritten = uDataLen;
    }

    NetCritLeave(&m_BufferCrit);
    return(S_OK);
}

/*F********************************************************************************/
/*!
    \relates     VoipNarrateVoiceStream
    \Function    CopyTo

    \Description
        Not Implementated

    \Version 05/17/2018 (tcho)
*/
/********************************************************************************F*/
HRESULT STDMETHODCALLTYPE VoipNarrateVoiceStream::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    return(E_NOTIMPL);
}

/*F********************************************************************************/
/*!
    \relates     VoipNarrateVoiceStream
    \Function    Stat

    \Description
        Not Implementated

    \Version 05/17/2018 (tcho)
*/
/********************************************************************************F*/
HRESULT STDMETHODCALLTYPE VoipNarrateVoiceStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    return(E_NOTIMPL);
}

/*F********************************************************************************/
/*!
    \relates     VoipNarrateVoiceStream
    \Function    UnlockRegion

    \Description
        Not Implementated

    \Version 05/17/2018 (tcho)
*/
/********************************************************************************F*/
HRESULT STDMETHODCALLTYPE VoipNarrateVoiceStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return(E_NOTIMPL);
}

/*F********************************************************************************/
/*!
    \relates     VoipNarrateVoiceStream
    \Function    LockRegion

    \Description
        Not Implementated

    \Version 05/17/2018 (tcho)
*/
/********************************************************************************F*/
HRESULT STDMETHODCALLTYPE VoipNarrateVoiceStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return(E_NOTIMPL);
}

/*F********************************************************************************/
/*!
    \relates     VoipNarrateVoiceStream
    \Function    Revert

    \Description
        Not Implementated

    \Version 05/17/2018 (tcho)
*/
/********************************************************************************F*/
HRESULT STDMETHODCALLTYPE VoipNarrateVoiceStream::Revert(void)
{
    return(E_NOTIMPL);
}

/*F********************************************************************************/
/*!
    \relates     VoipNarrateVoiceStream
    \Function    Commit

    \Description
        Not Implementated

    \Version 05/17/2018 (tcho)
*/
/********************************************************************************F*/
HRESULT STDMETHODCALLTYPE VoipNarrateVoiceStream::Commit(DWORD grfCommitFlags)
{
    return(E_NOTIMPL);
}

/*F********************************************************************************/
/*!
    \relates     VoipNarrateVoiceStream
    \Function    SetSize

    \Description
        Not Implementated

    \Version 05/17/2018 (tcho)
*/
/********************************************************************************F*/
HRESULT STDMETHODCALLTYPE VoipNarrateVoiceStream::SetSize(ULARGE_INTEGER libNewSize)
{
    return(E_NOTIMPL);
}

/*F********************************************************************************/
/*!
    \relates     VoipNarrateVoiceStream
    \Function    Clone

    \Description
        Not Implementated

    \Version 05/17/2018 (tcho)
*/
/********************************************************************************F*/
HRESULT STDMETHODCALLTYPE VoipNarrateVoiceStream::Clone(IStream **ppStream)
{
    return(E_NOTIMPL);
}

/*F********************************************************************************/
/*!
    \relates     VoipNarrateVoiceStream
    \Function    Seek

    \Description
        Not Implementated

    \Version 05/17/2018 (tcho)
*/
/********************************************************************************F*/
HRESULT STDMETHODCALLTYPE VoipNarrateVoiceStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    // need to return S_OK becuase the voice synthesizer expect this to be implementated (but not necessary in our case)
    return(S_OK);
}
