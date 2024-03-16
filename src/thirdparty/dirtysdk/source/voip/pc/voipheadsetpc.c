/*H********************************************************************************/
/*!
    \File voipheadsetpc.c

    \Description
        VoIP headset manager.

    \Copyright
        Copyright Electronic Arts 2004-2011.

    \Notes
        LPCM is supported only in loopback mode.

    \Version 1.0 03/30/2004 (jbrookes)  First Version
    \Version 2.0 10/20/2011 (jbrookes)  Major rewrite for cleanup and to fix I/O bugs
    \Version 3.0 07/09/2012 (akirchner) Added functionality to play buffer
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)

#include <string.h>
#include <mmsystem.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "DirtySDK/voip/voipdef.h"
#include "DirtySDK/voip/voiptranscribe.h"
#include "DirtySDK/voip/voipnarrate.h"

#include "voippriv.h"
#include "voipcommon.h"
#include "voipconnection.h"
#include "voippacket.h"
#include "voipmixer.h"
#include "voipconduit.h"
#include "DirtySDK/voip/voipcodec.h"
#include "voipheadset.h"

#include "voipdvi.h"
#include "voippcm.h"

/*** Defines **********************************************************************/

// Defines taken from mmddk.h - not including mmddk directly as that would
// add a dependancy on the Windows DDK
#define DRVM_MAPPER                                 (0x2000)
#define DRVM_MAPPER_CONSOLEVOICECOM_GET             (DRVM_MAPPER + 23)
#define DRVM_MAPPER_PREFERRED_GET                   (DRVM_MAPPER + 21)
#define DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY   (0x00000001)

#define VOIP_HEADSET_SAMPLERATE             (16000)                                                     //!< sample rate; 16khz audio
#define VOIP_HEADSET_SAMPLEWIDTH            (2)                                                         //!< sample size; 16-bit samples
#define VOIP_HEADSET_FRAMEDURATION          (20)                                                        //!< frame duration in milliseconds; 20ms
#define VOIP_HEADSET_FRAMESAMPLES           ((VOIP_HEADSET_SAMPLERATE*VOIP_HEADSET_FRAMEDURATION)/1000) //!< samples per frame (20ms; 8khz=160, 11.025khz=220.5, 16khz=320)
#define VOIP_HEADSET_FRAMESIZE              (VOIP_HEADSET_FRAMESAMPLES*VOIP_HEADSET_SAMPLEWIDTH)        //!< frame size in bytes; 640
#define VOIP_HEADSET_NUMWAVEBUFFERS         (5)
#define VOIP_HEADSET_MAXDEVICES             (16)
#define VOIP_HEADSET_WAVEFORMAT             (WAVE_FORMAT_1M16)
#define VOIP_HEADSET_PREBUFFERBLOCKS        (4)

#if defined(_WIN64)
    #define VOIP_HEADSET_WAVE_MAPPER ((uint64_t)-1)
#else
    #define VOIP_HEADSET_WAVE_MAPPER (WAVE_MAPPER)
#endif

//! transmission interval in milliseconds
#define VOIP_THREAD_SLEEP_DURATION        (20)

//! headset types (input, output)
typedef enum VoipHeadsetDeviceTypeE
{
    VOIP_HEADSET_INPDEVICE,
    VOIP_HEADSET_OUTDEVICE,
} VoipHeadsetDeviceTypeE;

/*** Macros ************************************************************************/

/*** Type Definitions **************************************************************/
//! playback data
typedef struct VoipHeadsetWaveDataT
{
    WAVEHDR WaveHdr;
    uint8_t FrameData[VOIP_HEADSET_FRAMESIZE];
} VoipHeadsetWaveDataT;

//! wave caps structure; this is a combination of the in and out caps, which are identical other than dwSupport (output only)
typedef struct VoipHeadsetWaveCapsT
{
    WORD    wMid;                  //!< manufacturer ID
    WORD    wPid;                  //!< product ID
    MMVERSION vDriverVersion;      //!< version of the driver
    CHAR    szPname[MAXPNAMELEN];  //!< product name (NULL terminated string)
    DWORD   dwFormats;             //!< formats supported
    WORD    wChannels;             //!< number of sources supported
    WORD    wReserved1;            //!< packing
    DWORD   dwSupport;             //!< functionality supported by driver
} VoipHeadsetWaveCapsT;

//! device info
typedef struct VoipHeadsetDeviceInfoT
{
    VoipHeadsetDeviceTypeE eDevType;    //!< device type

    HANDLE  hDevice;                    //!< handle to currently open device, or NULL if no device is open
    int32_t iActiveDevice;              //!< device index of active device
    int32_t iDeviceToOpen;              //!< device index of device we want to open, or -1 for any

    int32_t iCurVolume;                 //!< playback volume (output devices only)
    int32_t iNewVolume;                 //!< new volume to set (if different from iCurVolume)

    VoipHeadsetWaveDataT WaveData[VOIP_HEADSET_NUMWAVEBUFFERS]; //!< audio input/output buffer
    int32_t iCurWaveBuffer;             //!< current input/output buffer

    DWORD   dwCheckFlag[VOIP_HEADSET_NUMWAVEBUFFERS];   //!< flag to check for empty buffer (output only)

    VoipHeadsetWaveCapsT WaveDeviceCaps[VOIP_HEADSET_MAXDEVICES];
    int32_t iNumDevices;                //!< number of enumerated devices

    uint8_t bActive;                    //!< device is active (recording/playing)
    uint8_t bCloseDevice;               //!< device close requested
    uint8_t bChangeDevice;              //!< device change requested
    uint8_t _pad;
} VoipHeadsetDeviceInfoT;

//!< local user data space
typedef struct PCLocalVoipUserT
{
    uint32_t uPlaybackFlags; //!< mute flag every remote user is 1 bit
} PCLocalVoipUserT;

//! remote user data space
typedef struct PCRemoteVoipUserT
{
    VoipUserT User;
} PCRemoteVoipUserT;

//! VOIP module state data
struct VoipHeadsetRefT
{
    int32_t iMemGroup;                    //!< mem group
    void *pMemGroupUserData;              //!< mem group user data

    VoipHeadsetDeviceInfoT MicrInfo;      //!< input device info
    VoipHeadsetDeviceInfoT SpkrInfo;      //!< output device info

    // conduit info
    int32_t iMaxConduits;
    VoipConduitRefT *pConduitRef;

    // mixer state
    VoipMixerRefT *pMixerRef;

    // accessibility support modules
    VoipTranscribeRefT *pTranscribeRef;
    VoipNarrateRefT *pNarrateRef;

    // codec frame size in bytes
    int32_t iCmpFrameSize;

    // module debug level
    int32_t iDebugLevel;
    
    // which user 0-3 "owns" voip, VOIP_INVALID_LOCAL_USER_INDEX if no one
    int32_t iParticipatingUserIndex;

    // boolean control options
    volatile uint8_t bMicOn;            //!< TRUE if the mic is on, else FALSE
    uint8_t bMuted;                     //!< TRUE if muted (e.g. push-to-talk), else FALSE
    uint8_t bLoopback;                  //!< TRUE if loopback is enabled, else FALSE
    uint8_t bTextChatAccessibility;     //!< TRUE if text chat accessibility features are enabled
    uint8_t bCrossplay;                 //!< TRUE if cross-play is enabled, else FALSE

    uint8_t uSendSeq;                   //!< send sequence count
    uint8_t _pad[2];

    // user callback data
    VoipHeadsetMicDataCbT *pMicDataCb;
    VoipHeadsetTextDataCbT *pTextDataCb;
    VoipHeadsetStatusCbT *pStatusCb;
    void *pCbUserData;

    // speaker callback data
    VoipSpkrCallbackT *pSpkrDataCb;
    void *pSpkrCbUserData;

    // critical section for guarding access to device close/change flags
    NetCritT DevChangeCrit;

    // player
    int32_t iPlayerActive;
    int16_t *pPlayerBuffer;
    uint32_t uPlayerBufferFrameCurrent;
    uint32_t uPlayerBufferFrames;
    uint32_t uPlayerFirstTime;
    int32_t iPlayerFirstUse;
  
    // STT
    uint8_t bVoiceTranscriptionEnabled;

    // TTS
    uint8_t bNarrating;
    int32_t iNarrateWritePos;
    VoipNarrateGenderE eDefaultGender;
    int8_t narrateBuffer[VOIP_HEADSET_FRAMESIZE];

    PCLocalVoipUserT aLocalUsers[VOIP_MAXLOCALUSERS];

    //! remote user list - must come last in ref as it is variable length
    int32_t iMaxRemoteUsers;
    PCRemoteVoipUserT aRemoteUsers[1];
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Public Variables

// Private Variables

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _VoipHeadsetWaveGetNumDevs

    \Description
        Wraps wave(In|Out)GetNumDevs()

    \Input eDevType     - VOIP_HEADSET_INPDEVICE or VOIP_HEADSET_OUTDEVICE

    \Output
        UINT            - wave(In|Out)GetNumDevs() result

    \Version 10/12/2011 (jbrookes)
*/
/********************************************************************************F*/
static UINT _VoipHeadsetWaveGetNumDevs(VoipHeadsetDeviceTypeE eDevType)
{
    return((eDevType == VOIP_HEADSET_INPDEVICE) ? waveInGetNumDevs() : waveOutGetNumDevs());
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetWaveGetDevCaps

    \Description
        Wraps wave(In|Out)GetDevCaps()

    \Input uDeviceID    - device to get capabilities of
    \Input *pWaveCaps   - wave capabilities (note this structure does double-duty for input and output)
    \Input eDevType     - VOIP_HEADSET_INPDEVICE or VOIP_HEADSET_OUTDEVICE

    \Output
        MMRESULT        - wave(In|Out)GetNumDevs() result

    \Version 10/12/2011 (jbrookes)
*/
/********************************************************************************F*/
static MMRESULT _VoipHeadsetWaveGetDevCaps(UINT_PTR uDeviceID, VoipHeadsetWaveCapsT *pWaveCaps, VoipHeadsetDeviceTypeE eDevType)
{
    return((eDevType == VOIP_HEADSET_INPDEVICE) ? waveInGetDevCaps(uDeviceID, (LPWAVEINCAPSA)pWaveCaps, sizeof(WAVEINCAPSA)) : waveOutGetDevCaps(uDeviceID, (LPWAVEOUTCAPSA)pWaveCaps, sizeof(WAVEOUTCAPSA)));
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetWaveOpen

    \Description
        Wraps wave(In|Out)Open()

    \Input *pHandle     - [out] storage for handle to opened device
    \Input uDeviceID    - device to open
    \Input *pWaveFormat - wave format we want
    \Input eDevType     - VOIP_HEADSET_INPDEVICE or VOIP_HEADSET_OUTDEVICE

    \Output
        MMRESULT        - wave(In|Out)Open() result

    \Version 10/12/2011 (jbrookes)
*/
/********************************************************************************F*/
static MMRESULT _VoipHeadsetWaveOpen(HANDLE *pHandle, UINT uDeviceID, LPCWAVEFORMATEX pWaveFormat, VoipHeadsetDeviceTypeE eDevType)
{
    return((eDevType == VOIP_HEADSET_INPDEVICE) ? waveInOpen((LPHWAVEIN)pHandle, uDeviceID, pWaveFormat, 0, 0, 0) : waveOutOpen((LPHWAVEOUT)pHandle, uDeviceID, pWaveFormat, 0, 0, 0));
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetWaveClose

    \Description
        Wraps wave(In|Out)Close()

    \Input hHandle      - handle to device to close
    \Input eDevType     - VOIP_HEADSET_INPDEVICE or VOIP_HEADSET_OUTDEVICE

    \Output
        MMRESULT        - wave(In|Out)Close() result

    \Version 10/12/2011 (jbrookes)
*/
/********************************************************************************F*/
static MMRESULT _VoipHeadsetWaveClose(HANDLE hHandle, VoipHeadsetDeviceTypeE eDevType)
{
    return((eDevType == VOIP_HEADSET_INPDEVICE) ? waveInClose((HWAVEIN)hHandle) : waveOutClose((HWAVEOUT)hHandle));
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetWavePrepareHeader

    \Description
        Wraps wave(In|Out)PrepareHeader()

    \Input hHandle      - handle to device to prepare wave header for
    \Input pWaveHeader  - wave header to prepare
    \Input eDevType     - VOIP_HEADSET_INPDEVICE or VOIP_HEADSET_OUTDEVICE

    \Output
        MMRESULT        - wave(In|Out)PrepareHeader() result

    \Version 10/12/2011 (jbrookes)
*/
/********************************************************************************F*/
static MMRESULT _VoipHeadsetWavePrepareHeader(HANDLE hHandle, LPWAVEHDR pWaveHeader, VoipHeadsetDeviceTypeE eDevType)
{
    return((eDevType == VOIP_HEADSET_INPDEVICE) ? waveInPrepareHeader((HWAVEIN)hHandle, pWaveHeader, sizeof(WAVEHDR)) : waveOutPrepareHeader((HWAVEOUT)hHandle, pWaveHeader, sizeof(WAVEHDR)));
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetWaveUnprepareHeader

    \Description
        Wraps wave(In|Out)UnprepareHeader()

    \Input hHandle      - handle to device to prepare wave header for
    \Input pWaveHeader  - wave header to unprepare
    \Input eDevType     - VOIP_HEADSET_INPDEVICE or VOIP_HEADSET_OUTDEVICE

    \Output
        MMRESULT        - wave(In|Out)UnprepareHeader() result

    \Version 10/12/2011 (jbrookes)
*/
/********************************************************************************F*/
static MMRESULT _VoipHeadsetWaveUnprepareHeader(HANDLE hHandle, LPWAVEHDR pWaveHeader, VoipHeadsetDeviceTypeE eDevType)
{
    return((eDevType == VOIP_HEADSET_INPDEVICE) ? waveInUnprepareHeader((HWAVEIN)hHandle, pWaveHeader, sizeof(WAVEHDR)) : waveOutUnprepareHeader((HWAVEOUT)hHandle, pWaveHeader, sizeof(WAVEHDR)));
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetWaveReset

    \Description
        Wraps wave(In|Out)Reset()

    \Input hHandle      - handle to device to prepare wave header for
    \Input eDevType     - VOIP_HEADSET_INPDEVICE or VOIP_HEADSET_OUTDEVICE

    \Output
        MMRESULT        - wave(In|Out)Reset() result

    \Version 10/12/2011 (jbrookes)
*/
/********************************************************************************F*/
static MMRESULT _VoipHeadsetWaveReset(HANDLE hHandle, VoipHeadsetDeviceTypeE eDevType)
{
    return((eDevType == VOIP_HEADSET_INPDEVICE) ? waveInReset((HWAVEIN)hHandle) : waveOutReset((HWAVEOUT)hHandle));
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetPrepareWaveHeaders

    \Description
        Prepare wave headers for use.

    \Input *pDeviceInfo - device info
    \Input iNumBuffers - number of buffers in array
    \Input hDevice     - handle to wave device

    \Output
        int32_t         - negative=failure, zero=success

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetPrepareWaveHeaders(VoipHeadsetDeviceInfoT *pDeviceInfo, int32_t iNumBuffers, HANDLE hDevice)
{
    HRESULT hResult;
    int32_t iPlayBuf;

    for (iPlayBuf = 0; iPlayBuf < iNumBuffers; iPlayBuf++)
    {
        // set up the wave header
        pDeviceInfo->WaveData[iPlayBuf].WaveHdr.lpData = (LPSTR)pDeviceInfo->WaveData[iPlayBuf].FrameData;
        pDeviceInfo->WaveData[iPlayBuf].WaveHdr.dwBufferLength = sizeof(pDeviceInfo->WaveData[iPlayBuf].FrameData);
        pDeviceInfo->WaveData[iPlayBuf].WaveHdr.dwFlags = 0L;
        pDeviceInfo->WaveData[iPlayBuf].WaveHdr.dwLoops = 0L;

        // prepare the header
        if ((hResult = _VoipHeadsetWavePrepareHeader(hDevice, &pDeviceInfo->WaveData[iPlayBuf].WaveHdr, pDeviceInfo->eDevType)) != MMSYSERR_NOERROR)
        {
            NetPrintf(("voipheadsetpc: error %d preparing %s buffer %d\n", hResult,
                pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output", iPlayBuf));
            return(-1);
        }
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetUnprepareWaveHeaders

    \Description
        Unprepare wave headers, prior to releasing them.

    \Input *pDeviceInfo - device info
    \Input iNumBuffers - number of buffers in array
    \Input hDevice     - handle to wave device

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetUnprepareWaveHeaders(VoipHeadsetDeviceInfoT *pDeviceInfo, int32_t iNumBuffers, HANDLE hDevice)
{
    HRESULT hResult;
    int32_t iPlayBuf;

    for (iPlayBuf = 0; iPlayBuf < iNumBuffers; iPlayBuf++)
    {
        if ((hResult = _VoipHeadsetWaveUnprepareHeader(hDevice, &pDeviceInfo->WaveData[iPlayBuf].WaveHdr, pDeviceInfo->eDevType)) != MMSYSERR_NOERROR)
        {
            NetPrintf(("voipheadsetpc: error %d unpreparing %s buffer %d\n", hResult,
                pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output", iPlayBuf));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetOpenDevice

    \Description
        Open a wave device.

    \Input *pDeviceInfo - device state
    \Input iDevice      - handle to wave device (may be WAVE_MAPPER)
    \Input iNumBuffers  - number of input buffers

    \Output
        int32_t         - negative=error, zero=success

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetOpenDevice(VoipHeadsetDeviceInfoT *pDeviceInfo, int32_t iDevice, int32_t iNumBuffers)
{
    WAVEFORMATEX WaveFormatEx = { WAVE_FORMAT_PCM, 1, VOIP_HEADSET_SAMPLERATE, VOIP_HEADSET_SAMPLERATE * VOIP_HEADSET_SAMPLEWIDTH, 2, 16, 0 };
    HANDLE hDevice = NULL;
    HRESULT hResult;

    // open device
    if ((hResult = _VoipHeadsetWaveOpen(&hDevice, iDevice, &WaveFormatEx, pDeviceInfo->eDevType)) != S_OK)
    {
        NetPrintf(("voipheadsetpc: error %d opening input device\n", hResult));
        return(-1);
    }

    // set up input wave data
    if (_VoipHeadsetPrepareWaveHeaders(pDeviceInfo, iNumBuffers, hDevice) >= 0)
    {
        // set volume to -1 so it will be reset
        pDeviceInfo->iCurVolume = -1;

        // set active device
        pDeviceInfo->iActiveDevice = iDevice;

        if (pDeviceInfo->eDevType == VOIP_HEADSET_OUTDEVICE)
        {
            int32_t iBuffer;

            // mark as playing
            pDeviceInfo->bActive = TRUE;

            // set up check flag (we have to check for WHDR_PREPARED until we've written into the buffer)
            for (iBuffer = 0; iBuffer < VOIP_HEADSET_NUMWAVEBUFFERS; iBuffer += 1)
            {
                pDeviceInfo->dwCheckFlag[iBuffer] = WHDR_PREPARED;
            }
        }

        NetPrintf(("voipheadsetpc: opened %s device\n", pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output"));
    }
    else
    {
        _VoipHeadsetWaveClose(hDevice, pDeviceInfo->eDevType);
        hDevice = NULL;
    }

    pDeviceInfo->hDevice = hDevice;
    return((hDevice == NULL) ? -2 : 0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetCloseDevice

    \Description
        Close an open device.

    \Input *pHeadset    - module state
    \Input *pDeviceInfo - device info

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetCloseDevice(VoipHeadsetRefT *pHeadset, VoipHeadsetDeviceInfoT *pDeviceInfo)
{
    MMRESULT hResult;
    int32_t iDevice;

    // no open device, nothing to do
    if (pDeviceInfo->hDevice == 0)
    {
        return;
    }

    // reset the device
    if ((hResult = _VoipHeadsetWaveReset(pDeviceInfo->hDevice, pDeviceInfo->eDevType)) != MMSYSERR_NOERROR)
    {
        NetPrintf(("voipheadsetpc: failed to reset %s device (err=%d)\n", pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "waveIn" : "waveOut", hResult));
    }

    // unprepare the wave headers
    _VoipHeadsetUnprepareWaveHeaders(pDeviceInfo, 2, pDeviceInfo->hDevice);

    // close the device
    if ((hResult = _VoipHeadsetWaveClose(pDeviceInfo->hDevice, pDeviceInfo->eDevType)) != MMSYSERR_NOERROR)
    {
        NetPrintf(("voipheadsetpc: failed to close %s device (err=%d)\n", pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "waveIn" : "waveOut", hResult));
    }

    // reset device state
    NetPrintf(("voipheadsetpc: closed %s device\n", pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output"));

    pDeviceInfo->hDevice = NULL;
    pDeviceInfo->bActive = FALSE;
    iDevice = pDeviceInfo->iActiveDevice;
    pDeviceInfo->iActiveDevice = -1;

    // trigger device inactive callback
    // user index is always 0 because PC does not support MLU
    pHeadset->pStatusCb(0, FALSE, pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? VOIP_HEADSET_STATUS_INPUT : VOIP_HEADSET_STATUS_OUTPUT, pHeadset->pCbUserData);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetEnumerateDevices

    \Description
        Enumerate devices attached to system, and add those that are compatible
        to device list.

    \Input  *pHeadset   - headset module state
    \Input  *pDeviceInfo - device info

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetEnumerateDevices(VoipHeadsetRefT *pHeadset, VoipHeadsetDeviceInfoT *pDeviceInfo)
{
    int32_t iDevice, iNumDevices, iAddedDevices;
    VoipHeadsetWaveCapsT *pDeviceCaps;
    const char *pActiveDeviceName = NULL;
    int32_t iActiveDevice = -1;

    // get total number of eDevType devices
    iNumDevices = _VoipHeadsetWaveGetNumDevs(pDeviceInfo->eDevType);

    // get the active device name, if a device is active
    if (pDeviceInfo->iActiveDevice != -1)
    {
        iActiveDevice = pDeviceInfo->iActiveDevice;
        pActiveDeviceName = pDeviceInfo->WaveDeviceCaps[iActiveDevice].szPname;
    }

    // walk device list
    for (iDevice = 0, iAddedDevices = 0; (iDevice < iNumDevices) && (iAddedDevices < VOIP_HEADSET_MAXDEVICES); iDevice++)
    {
        // get device capabilities
        pDeviceCaps = &pDeviceInfo->WaveDeviceCaps[iAddedDevices];
        _VoipHeadsetWaveGetDevCaps(iDevice, pDeviceCaps, pDeviceInfo->eDevType);

        // print device info
        NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetpc: querying %s device %d\n", pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output", iDevice));
        NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetpc:   wMid=%d\n", pDeviceCaps->wMid));
        NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetpc:   wPid=%d\n", pDeviceCaps->wPid));
        NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetpc:   vDriverVersion=%d.%d\n", (pDeviceCaps->vDriverVersion&0xff00) >> 8, pDeviceCaps->vDriverVersion&0xff));
        NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetpc:   szPname=%s\n", pDeviceCaps->szPname));
        NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetpc:   dwFormats=0x%08x\n", pDeviceCaps->dwFormats));
        NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetpc:   wChannels=%d\n", pDeviceCaps->wChannels));

        // is it compatible?
        if (pDeviceCaps->dwFormats & VOIP_HEADSET_WAVEFORMAT)
        {
            // if this device is our active input device make sure to update our iActiveDevice value;
            if ((iActiveDevice != -1) && (strcmp(pActiveDeviceName, pDeviceCaps->szPname) == 0))
            {
                // if the active input device index changed log the change
                if (iActiveDevice != iAddedDevices)
                {
                    pDeviceInfo->iActiveDevice = iAddedDevices;
                    NetPrintf(("voipheadsetpc: active %s device, %s, moved from index %d to %d\n", pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output",
                        pActiveDeviceName, iActiveDevice, iAddedDevices));
                }

                // already matched don't compare names again
                iActiveDevice = -1;
            }

            NetPrintf(("voipheadsetpc: %s device %d is compatible\n", pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output", iDevice));
            iAddedDevices++;
        }
        else
        {
            NetPrintf(("voipheadsetpc: %s device %d is not compatible\n", pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output", iDevice));
        }

        NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetpc:\n"));
    }

    // set number of devices
    pDeviceInfo->iNumDevices = iAddedDevices;
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetStop

    \Description
        Stop recording & playback, and close USB audio device.

    \Notes
        This function is safe to call regardless of device state.

    \Version 11/03/2003 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetStop(VoipHeadsetRefT *pHeadset)
{
    _VoipHeadsetCloseDevice(pHeadset, &pHeadset->MicrInfo);
    _VoipHeadsetCloseDevice(pHeadset, &pHeadset->SpkrInfo);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetEnumerate

    \Description
        Enumerate all connected audio devices.

    \Input *pHeadset   - headset module state

    \Notes
        Although we only support one headset, we will scan up to eight to try and
        locate one that meets our needs for data format and sample rate.

    \Version 07/27/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetEnumerate(VoipHeadsetRefT *pHeadset)
{
    NetPrintf(("voipheadsetpc: enumerating devices\n"));

    // re-enumerating will kill our existing devices, so clean up appropriately.
    _VoipHeadsetStop(pHeadset);

    // enumerate input devices
    _VoipHeadsetEnumerateDevices(pHeadset, &pHeadset->MicrInfo);

    // enumerate output devices
    _VoipHeadsetEnumerateDevices(pHeadset, &pHeadset->SpkrInfo);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetSetCodec

    \Description
        Sets the specified codec.

    \Input *pHeadset    - pointer to module state
    \Input iCodecIdent  - codec identifier to set

    \Output
        int32_t         - zero=success, negative=failure

    \Version 10/26/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetSetCodec(VoipHeadsetRefT *pHeadset, int32_t iCodecIdent)
{
    int32_t iResult;

    // pass through codec creation request
    if ((iResult = VoipCodecCreate(iCodecIdent, pHeadset->iMaxConduits)) < 0)
    {
        return(iResult);
    }

    // query codec output size
    pHeadset->iCmpFrameSize = VoipCodecStatus(iCodecIdent, 'fsiz', VOIP_HEADSET_FRAMESAMPLES, NULL, 0);

    // return result to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetSendEncodedAudio

    \Description
        Send encoded audio on the wire, or into the conduit for local playback
        in loopback mode.

    \Input *pHeadset    - module state
    \Input *pPacket     - compressed audio data
    \Input iCompBytes   - size of the compressed audio data

    \Version 12/05/2018 (tcho)
*/
/********************************************************************************F*/
static void _VoipHeadsetSendEncodedAudio(VoipHeadsetRefT *pHeadset, VoipMicrPacketT *pPacket, int32_t iCompBytes)
{
    if (!pHeadset->bLoopback)
    {
        pHeadset->pMicDataCb(&pPacket->aData, iCompBytes, NULL, 0, pHeadset->iParticipatingUserIndex, pHeadset->uSendSeq++, pHeadset->pCbUserData);
    }
    else
    {
        // set up a null user for loopback
        VoipUserT VoipUser;
        ds_memclr(&VoipUser, sizeof(VoipUser));
        
        VoipConduitReceiveVoiceData(pHeadset->pConduitRef, &VoipUser, pPacket->aData, iCompBytes);
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetNarrateCb

    \Description
        VoipNarrate callback handler

    \Input *pNarrateRef - narration module state
    \Input iUserIndex   - local user index of user who requested narration
    \Input *pSamples    - sample data, or NULL if no data
    \Input iSize        - size of sample data in bytes, or zero if no data
    \Input *pUserData   - callback user data (headset module ref)

    \Version 12/05/2018 (tcho)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetNarrateCb(VoipNarrateRefT *pNarrateRef, int32_t iUserIndex, const int16_t *pSamples, int32_t iSize, void *pUserData)
{
    VoipHeadsetRefT *pHeadset = (VoipHeadsetRefT *)pUserData;
    const int8_t *pBuffer = (const int8_t *)pSamples;
    VoipMicrPacketT PacketData;
    int32_t iCompBytes;

    if (iSize > 0)
    {
        int32_t iRemain = iSize;
        while (iRemain != 0)
        {
            int32_t iCopySize = 0;
            int8_t *pFrameBuffer = pHeadset->narrateBuffer + pHeadset->iNarrateWritePos;

            if (pHeadset->iNarrateWritePos == 0)
            {
                ds_memclr(pFrameBuffer, VOIP_HEADSET_FRAMESIZE);
            }

            if (iRemain >= (VOIP_HEADSET_FRAMESIZE - pHeadset->iNarrateWritePos))
            {
                iCopySize = VOIP_HEADSET_FRAMESIZE - pHeadset->iNarrateWritePos;
            }
            else
            {
                iCopySize = iRemain;
            }

            ds_memcpy(pFrameBuffer, pBuffer + (iSize - iRemain), iCopySize);
            iRemain -= iCopySize;

            if ((iCopySize == VOIP_HEADSET_FRAMESIZE) || ((pHeadset->iNarrateWritePos + iCopySize) == VOIP_HEADSET_FRAMESIZE))
            {
                pHeadset->iNarrateWritePos = 0;
              
                // compress the input data, and if there is compressed data send it to the appropriate function
                if ((iCompBytes = VoipCodecEncode(PacketData.aData, (const int16_t*)(pHeadset->narrateBuffer), VOIP_HEADSET_FRAMESAMPLES, (!pHeadset->bTextChatAccessibility && pHeadset->bVoiceTranscriptionEnabled) ? pHeadset->pTranscribeRef : NULL)) > 0)
                {
                    _VoipHeadsetSendEncodedAudio(pHeadset, &PacketData, iCompBytes);
                }
            }
            else
            {
                pHeadset->iNarrateWritePos += iCopySize;
            }
        }
    }
    else
    {
        pHeadset->bNarrating = (iSize == VOIPNARRATE_STREAM_START) ? TRUE : FALSE;
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetProcessPlay

    \Description
        Process recording of data from the buffer and sending it to the registered
        mic data user callback.

    \Input *pHeadset - pointer to headset state

    \Version 07/09/2012 (akirchner)
*/
/********************************************************************************F*/
static void _VoipHeadsetProcessPlay(VoipHeadsetRefT *pHeadset)
{
    VoipMicrPacketT PacketData;
    int32_t         iCompBytes;
    uint32_t        uPlayerLastTime;
    uint32_t        uPackets;
    uint32_t        uPacket;

    if (!pHeadset->iPlayerFirstUse)
    {
        pHeadset->iPlayerFirstUse = 1;
        pHeadset->uPlayerFirstTime = NetTick();
    }

    uPlayerLastTime = NetTick();
    uPackets = (uPlayerLastTime - pHeadset->uPlayerFirstTime) / VOIP_THREAD_SLEEP_DURATION;

    for (uPacket = 0; uPacket < uPackets; uPacket++)
    {
        pHeadset->uPlayerBufferFrameCurrent += VOIP_HEADSET_SAMPLEWIDTH;
        pHeadset->uPlayerBufferFrameCurrent = (pHeadset->uPlayerBufferFrameCurrent >= pHeadset->uPlayerBufferFrames) ? 0 : pHeadset->uPlayerBufferFrameCurrent;

        // compress the buffer data, and if there is compressed data send it to the appropriate function
        if ((iCompBytes = VoipCodecEncode(PacketData.aData, &pHeadset->pPlayerBuffer[VOIP_HEADSET_FRAMESAMPLES * pHeadset->uPlayerBufferFrameCurrent], VOIP_HEADSET_FRAMESAMPLES, pHeadset->pTranscribeRef)) > 0)
        {
            _VoipHeadsetSendEncodedAudio(pHeadset, &PacketData, iCompBytes);
        }
    }

    pHeadset->uPlayerFirstTime = uPlayerLastTime;
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetProcessRecord

    \Description
        Process recording of data from the mic and sending it to the registered
        mic data user callback.

    \Input *pHeadset    - pointer to headset state

    \Version 04/01/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetProcessRecord(VoipHeadsetRefT *pHeadset)
{
    VoipHeadsetDeviceInfoT *pMicrInfo = &pHeadset->MicrInfo;
    VoipMicrPacketT PacketData;
    uint8_t FrameData[VOIP_HEADSET_FRAMESIZE];      //!< current recorded audio frame
    int32_t iCompBytes;
    HRESULT hResult;

    // only record if we have a recording device open
    if ((pMicrInfo->hDevice == NULL) || (pHeadset->iParticipatingUserIndex == VOIP_INVALID_LOCAL_USER_INDEX))
    {
        return;
    }

    // read data from the mic
    if (pMicrInfo->bActive == TRUE)
    {
        if (pHeadset->bMicOn == FALSE)
        {
            // tell hardware to stop recording
            if ((hResult = waveInStop((HWAVEIN)pMicrInfo->hDevice)) == MMSYSERR_NOERROR)
            {
                // mark as not recording
                NetPrintf(("voipheadsetpc: stop recording\n"));
                pMicrInfo->bActive = FALSE;
            }
            else
            {
                NetPrintf(("voipheadsetpc: error %d trying to stop recording\n", hResult));
            }
        }

        // pull in any waiting data
        for (; pMicrInfo->WaveData[pMicrInfo->iCurWaveBuffer].WaveHdr.dwFlags & WHDR_DONE; )
        {
            // copy audio data out of buffer
            ds_memcpy_s(FrameData, sizeof(FrameData), &pMicrInfo->WaveData[pMicrInfo->iCurWaveBuffer].FrameData, VOIP_HEADSET_FRAMESIZE);

            // compress the input data, and if there is compressed data send it to the appropriate function
            if (!pHeadset->bNarrating && (!pHeadset->bMuted || pHeadset->bLoopback))
            {
                if ((iCompBytes = VoipCodecEncode(PacketData.aData, (int16_t *)FrameData, VOIP_HEADSET_FRAMESAMPLES, pHeadset->bVoiceTranscriptionEnabled ? pHeadset->pTranscribeRef : NULL)) > 0)
                {
                    if (pHeadset->bTextChatAccessibility == FALSE)
                    {
                       _VoipHeadsetSendEncodedAudio(pHeadset, &PacketData, iCompBytes);
                    }
                }
            }

            // re-queue buffer to read more data
            if ((hResult = waveInAddBuffer((HWAVEIN)pMicrInfo->hDevice, &pMicrInfo->WaveData[pMicrInfo->iCurWaveBuffer].WaveHdr, sizeof(WAVEHDR))) != MMSYSERR_NOERROR)
            {
                if (hResult == MMSYSERR_NODRIVER)
                {
                    NetPrintf(("voipheadsetpc: waveInAddBuffer returned MMSYSERR_NODRIVER. Headset removed? Closing headset\n"));
                    _VoipHeadsetStop(pHeadset);
                    break;
                }
                else if (hResult != WAVERR_STILLPLAYING)
                {
                    NetPrintf(("voipheadsetpc: could not add buffer %d to record queue (err=%d)\n", pMicrInfo->iCurWaveBuffer, hResult));
                }
            }

            // index to next read buffer
            pMicrInfo->iCurWaveBuffer = (pMicrInfo->iCurWaveBuffer + 1) % VOIP_HEADSET_NUMWAVEBUFFERS;
        }
    }
    // if not recording and a mic-on request comes in, start recording
    else if (pHeadset->bMicOn == TRUE)
    {
        int32_t iBuffer;

        if ((hResult = waveInStart((HWAVEIN)pMicrInfo->hDevice)) == MMSYSERR_NOERROR)
        {
            // mark as recording
            NetPrintf(("voipheadsetpc: recording...\n"));
            pMicrInfo->bActive = TRUE;
        }
        else
        {
            NetPrintf(("voipheadsetpc: error %d trying to start recording\n", hResult));
        }

        // add buffers to start recording
        for (iBuffer = 0; iBuffer < VOIP_HEADSET_NUMWAVEBUFFERS; iBuffer += 1)
        {
            if ((hResult = waveInAddBuffer((HWAVEIN)pMicrInfo->hDevice, &pMicrInfo->WaveData[iBuffer].WaveHdr, sizeof(WAVEHDR))) != MMSYSERR_NOERROR)
            {
                NetPrintf(("voipheadsetpc: waveInAddBuffer for buffer %d failed err=%d\n", iBuffer, hResult));
            }
        }
        pMicrInfo->iCurWaveBuffer = 0;

        // reset compression state
        VoipCodecReset();
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetProcessPlayback

    \Description
        Process playback of data received from the network to the headset earpiece.

    \Input *pHeadset    - pointer to headset state

    \Version 04/01/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetProcessPlayback(VoipHeadsetRefT *pHeadset)
{
    VoipHeadsetDeviceInfoT *pSpkrInfo = &pHeadset->SpkrInfo;
    uint8_t FrameData[VOIP_HEADSET_FRAMESIZE];      //!< current recorded audio frame
    int32_t iSampBytes;
    HRESULT hResult;

    // only play if we have a playback device open
    if ((pSpkrInfo->hDevice == NULL) || (pHeadset->iParticipatingUserIndex == VOIP_INVALID_LOCAL_USER_INDEX))
    {
        return;
    }

    // write data to the audio output if it's available
    if (pSpkrInfo->bActive == TRUE)
    {
        int32_t iLoop, iNumBufAvail;

        // update play volume, if requested
        if ((pSpkrInfo->iCurVolume != pSpkrInfo->iNewVolume) && (pSpkrInfo->iNewVolume != -1))
        {
            int32_t iVolume;

            iVolume = (pSpkrInfo->iNewVolume * 65535) / 100;
            iVolume |= iVolume << 16;

            if ((hResult = waveOutSetVolume((HWAVEOUT)pSpkrInfo->hDevice, iVolume)) == MMSYSERR_NOERROR)
            {
                NetPrintf(("voipheadsetpc: changed play volume to %d\n", pSpkrInfo->iNewVolume));
                pSpkrInfo->iCurVolume = pSpkrInfo->iNewVolume;
            }
            else
            {
                NetPrintf(("voipheadsetpc: error %d trying to change play volume\n", hResult));
            }
        }

        // count number of buffers available to write into
        for (iLoop = 0, iNumBufAvail = 0; iLoop < VOIP_HEADSET_NUMWAVEBUFFERS; iLoop += 1)
        {
            iNumBufAvail += (pSpkrInfo->WaveData[iLoop].WaveHdr.dwFlags & WHDR_DONE) ? 1 : 0;
        }

        // if we have space to write
        for ( ; pSpkrInfo->WaveData[pSpkrInfo->iCurWaveBuffer].WaveHdr.dwFlags & pSpkrInfo->dwCheckFlag[pSpkrInfo->iCurWaveBuffer]; )
        {
            // decode and mix buffered packet data
            if ((iSampBytes = VoipMixerProcess(pHeadset->pMixerRef, FrameData)) == VOIP_HEADSET_FRAMESIZE)
            {
                // if there's nothing playing, we want to prebuffer with silence
                if (iNumBufAvail == VOIP_HEADSET_NUMWAVEBUFFERS)
                {
                    uint8_t FrameSilenceData[VOIP_HEADSET_FRAMESIZE];
                    int32_t iBlock;

                    ds_memclr(FrameSilenceData, sizeof(FrameSilenceData));
                    NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetpc: prebuffering %d blocks\n", VOIP_HEADSET_PREBUFFERBLOCKS));
                    for (iBlock = 0; iBlock < VOIP_HEADSET_PREBUFFERBLOCKS; iBlock += 1)
                    {
                        ds_memcpy_s(pSpkrInfo->WaveData[pSpkrInfo->iCurWaveBuffer].FrameData, sizeof(pSpkrInfo->WaveData[pSpkrInfo->iCurWaveBuffer].FrameData), FrameSilenceData, sizeof(FrameSilenceData));
                        if ((hResult = waveOutWrite((HWAVEOUT)pSpkrInfo->hDevice, &pSpkrInfo->WaveData[pSpkrInfo->iCurWaveBuffer].WaveHdr, sizeof(WAVEHDR))) == MMSYSERR_NOERROR)
                        {
                            pSpkrInfo->dwCheckFlag[pSpkrInfo->iCurWaveBuffer] = WHDR_DONE;
                            pSpkrInfo->iCurWaveBuffer = (pSpkrInfo->iCurWaveBuffer + 1) % VOIP_HEADSET_NUMWAVEBUFFERS;
                        }
                        else
                        {
                            NetPrintf(("voipheadsetpc: write failed (error=%d)\n", hResult));
                        }
                    }
                    iNumBufAvail = 0;
                }

                // forward data to speaker callback, if callback is specified
                if (pHeadset->pSpkrDataCb != NULL)
                {
                    pHeadset->pSpkrDataCb((int16_t *)FrameData, VOIP_HEADSET_FRAMESAMPLES, pHeadset->pSpkrCbUserData);
                }

                // copy data to prepared wave buffer
                ds_memcpy_s(pSpkrInfo->WaveData[pSpkrInfo->iCurWaveBuffer].FrameData, sizeof(pSpkrInfo->WaveData[pSpkrInfo->iCurWaveBuffer].FrameData), FrameData, VOIP_HEADSET_FRAMESIZE);

                // write out wave buffer
                if ((hResult = waveOutWrite((HWAVEOUT)pSpkrInfo->hDevice, &pSpkrInfo->WaveData[pSpkrInfo->iCurWaveBuffer].WaveHdr, sizeof(WAVEHDR))) == MMSYSERR_NOERROR)
                {
                    NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetpc: [%2d] wrote %d samples\n", pSpkrInfo->iCurWaveBuffer, VOIP_HEADSET_FRAMESAMPLES));
                    pSpkrInfo->dwCheckFlag[pSpkrInfo->iCurWaveBuffer] = WHDR_DONE;
                    pSpkrInfo->iCurWaveBuffer = (pSpkrInfo->iCurWaveBuffer + 1) % VOIP_HEADSET_NUMWAVEBUFFERS;
                }
                else
                {
                    if (hResult == MMSYSERR_NODRIVER)
                    {
                        NetPrintf(("voipheadsetpc: returned MMSYSERR_NODRIVER. Headset removed? Closing headset\n"));
                        _VoipHeadsetStop(pHeadset);
                    }
                    else if (hResult == WAVERR_STILLPLAYING)
                    {
                        NetPrintf(("voipheadsetpc: WAVERR_STILLPLAYING\n"));
                    }
                    else
                    {
                        NetPrintf(("voipheadsetpc: write failed (error=%d)\n", hResult));
                    }
                }
            }
            else if (iSampBytes > 0)
            {
                NetPrintf(("voipheadsetpc: error - got %d bytes from mixer when we were expecting %d\n", iSampBytes, VOIP_HEADSET_FRAMESIZE));
            }
            else
            {
                // no data waiting in the mixer, so break out
                break;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetProcessDeviceChange

    \Description
        Process a device change request.

    \Input *pHeadset    - headset state
    \Input *pDeviceInfo - device info

    \Version 10/12/2011 (jbrookes) Split from VoipHeadsetProcess()
*/
/********************************************************************************F*/
static void _VoipHeadsetProcessDeviceChange(VoipHeadsetRefT *pHeadset, VoipHeadsetDeviceInfoT *pDeviceInfo)
{
    // early out if no change is requested
    if (!pDeviceInfo->bChangeDevice && !pDeviceInfo->bCloseDevice)
    {
        return;
    }

    // device change requires sole access to headset critical section
    NetCritEnter(&pHeadset->DevChangeCrit);

    // close the device
    _VoipHeadsetCloseDevice(pHeadset, pDeviceInfo);
    pDeviceInfo->bCloseDevice = FALSE;

    // process change input device request
    if (pDeviceInfo->bChangeDevice)
    {
        if (_VoipHeadsetOpenDevice(pDeviceInfo, pDeviceInfo->iDeviceToOpen, VOIP_HEADSET_NUMWAVEBUFFERS) == 0)
        {
            // user index is always 0 because PC does not support MLU
            pHeadset->pStatusCb(0, TRUE, pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? VOIP_HEADSET_STATUS_INPUT : VOIP_HEADSET_STATUS_OUTPUT, pHeadset->pCbUserData);
        }

        pDeviceInfo->bChangeDevice = FALSE;
    }

    NetCritLeave(&pHeadset->DevChangeCrit);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetProcessTranscription

    \Description
        Process voice transcription

    \Input pHeadset     - headset ref

    \Version 09/07/2018 (tcho)
*/
/********************************************************************************F*/
static void _VoipHeadsetProcessTranscription(VoipHeadsetRefT *pHeadset)
{
    char strTranscribeBuf[VOIPTRANSCRIBE_OUTPUT_MAX];

    if (pHeadset->pTranscribeRef != NULL)
    {
        // process transcription
        VoipTranscribeUpdate(pHeadset->pTranscribeRef);

        // if a transcription is available, send it along
        if (VoipTranscribeGet(pHeadset->pTranscribeRef, strTranscribeBuf, sizeof(strTranscribeBuf)) > 0)
        {
            if (pHeadset->iParticipatingUserIndex != VOIP_INVALID_LOCAL_USER_INDEX)
            {
                pHeadset->pTextDataCb(strTranscribeBuf, pHeadset->iParticipatingUserIndex, pHeadset->pCbUserData);
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetGetRemoteUserIndex

    \Description
       Returns the user index of the remote user

    \Input *pHeadset    - pHeadset ref
    \Input *pRemoteUser - remote user

    \Output
        int32_t         - remote user index, negative if user is not found

    \Version 05/28/2019 (tcho)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetGetRemoteUserIndex(VoipHeadsetRefT *pHeadset, VoipUserT *pRemoteUser)
{
    int32_t iRemoteUserIndex;

    for (iRemoteUserIndex = 0; iRemoteUserIndex < pHeadset->iMaxRemoteUsers; ++iRemoteUserIndex)
    {
        if (VOIP_SameUser(&pHeadset->aRemoteUsers[iRemoteUserIndex].User, pRemoteUser))
        {
            break;
        }
    }

    return((iRemoteUserIndex != pHeadset->iMaxRemoteUsers) ? iRemoteUserIndex : -1);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetPlaybackCallback

    \Description
        Determines if we should be submit audio to the given mixer

    \Input *pMixer          - mixer ref
    \Input *pRemoteUser     - remote user who sent the mic packets
    \Input *pUserData       - callback user data (VoipHeadsetRefT)

    \Output
        uint8_t             - TRUE for playback, FALSE for not to playback

    \Version 05/28/2019 (cvienneau)
*/
/********************************************************************************F*/
static uint8_t _VoipHeadsetPlaybackCallback(VoipMixerRefT *pMixer, VoipUserT *pRemoteUser, void *pUserData)
{
    uint8_t bPlayback = TRUE;
    VoipHeadsetRefT *pHeadset = (VoipHeadsetRefT *)pUserData;

    int32_t iRemoteUserIndex;
    int32_t iLocalUserIndex;

    // find the remote user index
    iRemoteUserIndex = _VoipHeadsetGetRemoteUserIndex(pHeadset, pRemoteUser);

    if (iRemoteUserIndex < 0)
    {
        NetPrintf(("voipheadsetpc: _VoipHeadsetPlaybackCallback() cannot find the remote user iPersonaId: %lld", pRemoteUser->AccountInfo.iPersonaId));
        return(TRUE);
    }

    // loop through all local users determine if ANY of them have this remote user index muted (we have a shared device on PC)
    for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS; ++iLocalUserIndex)
    {
        // muting from +-pbk selectors
        if ((pHeadset->aLocalUsers[iLocalUserIndex].uPlaybackFlags & (1 << iRemoteUserIndex)) == 0)
        {
            bPlayback = FALSE;
            break;
        }
    }
    return(bPlayback);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipHeadsetCreate

    \Description
        Create the headset manager.

    \Input iMaxConduits     - max number of conduits
    \Input *pMicDataCb      - pointer to user callback to trigger when mic data is ready
    \Input *pTextDataCb     - pointer to user callback to trigger when transcribed text is ready
    \Input *pOpaqueDataCb   - pointer to user callback to trigger when opaque data is ready (ignored)
    \Input *pStatusCb       - pointer to user callback to trigger when headset status changes
    \Input *pCbUserData     - pointer to user callback data
    \Input iData            - platform-specific - unused for PC

    \Output
        VoipHeadsetRefT *   - pointer to module state, or NULL if an error occured

    \Version 03/30/2004 (jbrookes)
*/
/********************************************************************************F*/
VoipHeadsetRefT *VoipHeadsetCreate(int32_t iMaxConduits, VoipHeadsetMicDataCbT *pMicDataCb, VoipHeadsetTextDataCbT *pTextDataCb, VoipHeadsetOpaqueDataCbT *pOpaqueDataCb, VoipHeadsetStatusCbT *pStatusCb, void *pCbUserData, int32_t iData)
{
    VoipHeadsetRefT *pHeadset;
    int32_t iMemGroup, iSize, iLocalUserIndex;
    void *pMemGroupUserData;

    // Query mem group data
    VoipCommonMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // make sure we don't exceed maxconduits
    if (iMaxConduits > VOIP_MAXCONNECT)
    {
        NetPrintf(("voipheadsetpc: request for %d conduits exceeds max\n", iMaxConduits));
        return(NULL);
    }

    iSize = sizeof(*pHeadset) + (sizeof(PCRemoteVoipUserT) * (iMaxConduits + VOIP_MAX_LOW_LEVEL_CONNS - 1));

    // allocate and clear module state
    if ((pHeadset = (VoipHeadsetRefT *)DirtyMemAlloc(iSize, VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        return(NULL);
    }
    ds_memclr(pHeadset, iSize);

    // allocate mixer
    if ((pHeadset->pMixerRef = VoipMixerCreate(16, VOIP_HEADSET_FRAMESAMPLES)) == NULL)
    {
        NetPrintf(("voipheadsetpc: unable to create mixer\n"));
        DirtyMemFree(pHeadset, VOIP_MEMID, iMemGroup, pMemGroupUserData);
        return(NULL);
    }

    // allocate conduit manager
    if ((pHeadset->pConduitRef = VoipConduitCreate(iMaxConduits)) == NULL)
    {
        NetPrintf(("voipheadsetpc: unable to allocate conduit manager\n"));
        VoipMixerDestroy(pHeadset->pMixerRef);
        DirtyMemFree(pHeadset, VOIP_MEMID, iMemGroup, pMemGroupUserData);
        return(NULL);
    }
    pHeadset->iMaxConduits = iMaxConduits;
    pHeadset->iMaxRemoteUsers = iMaxConduits + VOIP_MAX_LOW_LEVEL_CONNS;
    VoipConduitRegisterPlaybackCb(pHeadset->pConduitRef, _VoipHeadsetPlaybackCallback, pHeadset);
    
    // initialize playback flags (default on for everyone)
    for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS; ++iLocalUserIndex)
    {
        pHeadset->aLocalUsers[iLocalUserIndex].uPlaybackFlags = 0xFFFFFFFF;
    }

    // allocate narration module
    if ((pHeadset->pNarrateRef = VoipNarrateCreate(_VoipHeadsetNarrateCb, pHeadset)) == NULL)
    {
        VoipTranscribeDestroy(pHeadset->pTranscribeRef);
        VoipConduitDestroy(pHeadset->pConduitRef);
        VoipMixerDestroy(pHeadset->pMixerRef);
        DirtyMemFree(pHeadset, VOIP_MEMID, iMemGroup, pMemGroupUserData);
        return(NULL);
    }

    // set no participating user at the start
    pHeadset->iParticipatingUserIndex = VOIP_INVALID_LOCAL_USER_INDEX;

    // set TTS default gender
    pHeadset->eDefaultGender = VOIPNARRATE_GENDER_MALE;
    
    // set mixer
    VoipConduitMixerSet(pHeadset->pConduitRef, pHeadset->pMixerRef);

    // register codecs
    VoipCodecRegister('dvid', &VoipDVI_CodecDef);
    VoipCodecRegister('lpcm', &VoipPCM_CodecDef);

    // set up to use dvi codec by default
    VoipHeadsetControl(pHeadset, 'cdec', 'dvid', 0, NULL);

    // enable microphone
    VoipHeadsetControl(pHeadset, 'micr', TRUE, 0, NULL);

    // save mem info
    pHeadset->iMemGroup = iMemGroup;
    pHeadset->pMemGroupUserData = pMemGroupUserData;

    // save info
    pHeadset->pMicDataCb = pMicDataCb;
    pHeadset->pTextDataCb = pTextDataCb;
    pHeadset->pStatusCb = pStatusCb;
    pHeadset->pCbUserData = pCbUserData;

    // no currently active device
    pHeadset->SpkrInfo.eDevType = VOIP_HEADSET_OUTDEVICE;
    pHeadset->SpkrInfo.iActiveDevice = -1;
    pHeadset->SpkrInfo.iDeviceToOpen = WAVE_MAPPER;

    pHeadset->MicrInfo.eDevType = VOIP_HEADSET_INPDEVICE;
    pHeadset->MicrInfo.iActiveDevice = -1;
    pHeadset->MicrInfo.iDeviceToOpen = WAVE_MAPPER;

    // play
    pHeadset->iPlayerActive = 0;

    // set initial volume
    pHeadset->SpkrInfo.iNewVolume = -1;

    // set default debuglevel
    pHeadset->iDebugLevel = 1;

    // enumerate headsets on startup
    _VoipHeadsetEnumerate(pHeadset);

    // init the critical section
    NetCritInit(&pHeadset->DevChangeCrit, "voipheadsetpc-devchange");
    
    // return module ref to caller
    return(pHeadset);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetDestroy

    \Description
        Destroy the headset manager.

    \Input *pHeadset    - pointer to headset state

    \Version 03/31/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetDestroy(VoipHeadsetRefT *pHeadset)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // stop headsets
    _VoipHeadsetStop(pHeadset);

    // destroy transcription ref
    if (pHeadset->pTranscribeRef != NULL)
    {
        VoipTranscribeDestroy(pHeadset->pTranscribeRef);
    }

    // destroy narrate ref
    VoipNarrateDestroy(pHeadset->pNarrateRef);

    // free conduit manager
    VoipConduitDestroy(pHeadset->pConduitRef);

    // free mixer
    VoipMixerDestroy(pHeadset->pMixerRef);

    // free active codec
    VoipCodecDestroy();

    // kill the critical section
    NetCritKill(&pHeadset->DevChangeCrit);

    // dispose of module memory
    VoipCommonMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    DirtyMemFree(pHeadset, VOIP_MEMID, iMemGroup, pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetReceiveVoiceDataCb

    \Description
        Connectionlist callback to handle receiving a voice packet from a remote peer.

    \Input *pRemoteUsers    - user we're receiving the voice data from
    \Input iRemoteUserSize  - pRemoteUsers array size
    \Input iConsoleId       - generic identifier for the console to which the users belong
    \Input *pMicrInfo       - micr info from inbound packet
    \Input *pPacketData     - pointer to beginning of data in packet payload
    \Input *pUserData       - VoipHeadsetT ref

    \Version 03/21/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetReceiveVoiceDataCb(VoipUserT *pRemoteUsers, int32_t iRemoteUserSize, int32_t iConsoleId, VoipMicrInfoT *pMicrInfo, uint8_t *pPacketData, void *pUserData)
{
    VoipHeadsetRefT *pHeadset = (VoipHeadsetRefT *)pUserData;
    uint32_t uRemoteUserIndex = 0;
    uint32_t uMicrPkt;
    const BYTE *pSubPacket = pPacketData;

    // if we're not playing, ignore it
    if (pHeadset->SpkrInfo.bActive == FALSE)
    {
        #if DIRTYCODE_LOGGING
        if ((pMicrInfo->uSeqn % 30) == 0)
        {
            NetPrintfVerbose((pHeadset->iDebugLevel, 2, "voipheadsetpc: playback disabled, discarding voice data (seqn=%d)\n", pMicrInfo->uSeqn));
        }
        #endif
        return;
    }

    // validate subpacket size if we are dealing with a fixed-length scenario
    if ((pMicrInfo->uSubPacketSize != 0xFF) && (pMicrInfo->uSubPacketSize != pHeadset->iCmpFrameSize))
    {
        NetPrintf(("voipheadsetpc: discarding voice packet with %d voice bundles and mismatched sub-packet size %d (expecting %d)\n",
            pMicrInfo->uNumSubPackets, pMicrInfo->uSubPacketSize, pHeadset->iCmpFrameSize));
        return;
    }

    // if this is the shared user index
    if (pMicrInfo->uUserIndex == VOIP_SHARED_REMOTE_INDEX)
    {
        int32_t iIndex;

        // find the first valid user to playback the audio
        for (iIndex = 0; iIndex < iRemoteUserSize; iIndex += 1)
        {
            if (!VOIP_NullUser(&pRemoteUsers[iIndex]))
            {
                uRemoteUserIndex = iIndex;
                break;
            }
        }

        if (iIndex == iRemoteUserSize)
        {
            // didn't find a remote user to play back the shared audio
            NetPrintf(("voipheadsetpc: discarding voice packet from shared user because we cannot find a remote user to play it back as!\n"));
            return;
        }
    }
    else
    {
        uRemoteUserIndex = pMicrInfo->uUserIndex;
    }

    // submit voice sub-packets
    for (uMicrPkt = 0; uMicrPkt < pMicrInfo->uNumSubPackets; uMicrPkt++)
    {
        // get the size of the subpacket based on variable or not
        uint32_t uSubPacketSize = (pMicrInfo->uSubPacketSize != 0xFF) ? pMicrInfo->uSubPacketSize : *pSubPacket++;

        // send it to conduit manager
        VoipConduitReceiveVoiceData(pHeadset->pConduitRef, &pRemoteUsers[uRemoteUserIndex], pSubPacket, uSubPacketSize);

        // move to next packet
        pSubPacket += uSubPacketSize;
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetSetRemoteUserVoicePlayback

    \Description
        Helper function to enable or disable playback of voice from a remote user
        for a given local user.

    \Input *pHeadset            - pointer to headset state
    \Input *pRemoteUser         - remote user
    \Input iLocalUserIndex      - local user index
    \Input bEnablePlayback      - TRUE to enable voice playback. FALSE to disable voice playback.

    \Output
        int32_t                 - negative=error, zero=success

    \Version 06/09/2019 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetSetRemoteUserVoicePlayback(VoipHeadsetRefT *pHeadset, VoipUserT *pRemoteUser, int32_t iLocalUserIndex, uint8_t bEnablePlayback)
{
    int32_t iRemoteUserIndex;

    // find the remote user index
    iRemoteUserIndex = _VoipHeadsetGetRemoteUserIndex(pHeadset, pRemoteUser);

    if (iRemoteUserIndex < 0)
    {
        NetPrintf(("voipheadsetpc: _VoipHeadsetSetRemoteUserVoicePlayback() cannot find the remote user iPersonaId: %lld", pRemoteUser->AccountInfo.iPersonaId));
        return(-1);
    }

    if (bEnablePlayback)
    {
        pHeadset->aLocalUsers[iLocalUserIndex].uPlaybackFlags |= (1 << iRemoteUserIndex);
    }
    else
    {
        pHeadset->aLocalUsers[iLocalUserIndex].uPlaybackFlags &= ~(1 << iRemoteUserIndex);
    }
    NetPrintf(("voipheadsetpc: local user[%d] playback flags now: 0x%08x.\n", iLocalUserIndex, pHeadset->aLocalUsers[iLocalUserIndex].uPlaybackFlags));

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetAddRemoteTalker

    \Description
        Create the game chat participant for this remote user.

    \Input *pHeadset    - headset module
    \Input *pRemoteUser - remote user
    \Input uConsoleId   - unique console identifier (local scope only, does not need to be the same on all hosts)

    \Output
        int32_t         - negative=error, zero=success

    \Version 06/09/2019 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetAddRemoteTalker(VoipHeadsetRefT *pHeadset, VoipUserT *pRemoteUser, uint64_t uConsoleId)
{
    int32_t iRemoteUserIndex;
    PCRemoteVoipUserT *pPcRemoteUser = NULL;

    // find a duplicate entry
    for (iRemoteUserIndex = 0; iRemoteUserIndex < pHeadset->iMaxRemoteUsers; ++iRemoteUserIndex)
    {
        if (VOIP_SameUser(&pHeadset->aRemoteUsers[iRemoteUserIndex].User, pRemoteUser))
        {
            pPcRemoteUser = &pHeadset->aRemoteUsers[iRemoteUserIndex];
            break;
        }
    }

    // early return if we remote user already exists
    if (pPcRemoteUser != NULL)
    {
        NetPrintf(("voipheadsetpc: adding remote talker %lld failed because it already exists\n", pRemoteUser->AccountInfo.iPersonaId));
        return(-1);
    }

    // find a empty remote user entry
    for (iRemoteUserIndex = 0; iRemoteUserIndex < pHeadset->iMaxRemoteUsers; ++iRemoteUserIndex)
    {
        if (pHeadset->aRemoteUsers[iRemoteUserIndex].User.AccountInfo.iPersonaId == 0)
        {
            ds_memclr(&pHeadset->aRemoteUsers[iRemoteUserIndex], sizeof(PCRemoteVoipUserT));
            ds_memcpy(&pHeadset->aRemoteUsers[iRemoteUserIndex].User, pRemoteUser, sizeof(VoipUserT));
            pPcRemoteUser = &pHeadset->aRemoteUsers[iRemoteUserIndex];
                
            // register the remote user with all the conduits
            VoipConduitRegisterUser(pHeadset->pConduitRef, pRemoteUser, TRUE);

            NetPrintf(("voipheadsetpc: registered remote talker %lld at remote user index %d\n", pRemoteUser->AccountInfo.iPersonaId, iRemoteUserIndex));
            break;
        }
    }

    if (pPcRemoteUser == NULL)
    {
        NetPrintf(("voipheadsetpc: adding remote talker %lld failed because aRemoteUsers is full\n", pRemoteUser->AccountInfo.iPersonaId));
        return(-2);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetRemoveRemoteTalker

    \Description
        Remove the specified remote user from the collection of users known by the chat manager.

    \Input *pHeadset    - headset module
    \Input *pUser       - user to be removed

    \Output
        int32_t         - negative=error, zero=success

    \Version 11/22/2018 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetRemoveRemoteTalker(VoipHeadsetRefT *pHeadset, VoipUserT *pUser)
{
    int32_t iRetCode = 0;
    int32_t iRemoteUserIndex;
    PCRemoteVoipUserT *pPcRemoteUser = NULL;

    VoipConduitRegisterUser(pHeadset->pConduitRef, pUser, FALSE);

    // find the remote user
    for (iRemoteUserIndex = 0; iRemoteUserIndex < pHeadset->iMaxRemoteUsers; ++iRemoteUserIndex)
    {
        if (VOIP_SameUser(&pHeadset->aRemoteUsers[iRemoteUserIndex].User, pUser))
        {
            pPcRemoteUser = &pHeadset->aRemoteUsers[iRemoteUserIndex];
            break;
        }
    }

    if (pPcRemoteUser != NULL)
    {
        NetPrintf(("voipheadsetpc: unregistered remote talker %lld at remote user index %d\n", pPcRemoteUser->User.AccountInfo.iPersonaId, iRemoteUserIndex));
        ds_memclr(pPcRemoteUser, sizeof(PCRemoteVoipUserT));
    }
    else
    {
        NetPrintf(("voipheadsetpc: unregistered remote talker %lld failed because it does not exist\n", pUser->AccountInfo.iPersonaId));
        iRetCode = -1;
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetRegisterUserCb

    \Description
        Connectionlist callback to register/unregister a new user with the
        VoipConduit module.

    \Input *pRemoteUser - user to register
    \Input iConsoleId   - generic identifier for the console to which the user belongs (ignored)
    \Input bRegister    - true=register, false=unregister
    \Input *pUserData   - voipheadset module ref

    \Version 03/21/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetRegisterUserCb(VoipUserT *pRemoteUser, int32_t iConsoleId, uint32_t bRegister, void *pUserData)
{
    VoipHeadsetRefT *pHeadset = (VoipHeadsetRefT *)pUserData;

    // early exit if invalid remote talker
    if (VOIP_NullUser(pRemoteUser))
    {
        NetPrintf(("voipheadsetpc: can't %s NULL remote talker\n", (bRegister ? "register" : "unregister")));
        return;
    }

    if (bRegister)
    {
        _VoipHeadsetAddRemoteTalker(pHeadset, pRemoteUser, (uint64_t)iConsoleId);
    }
    else
    {
        _VoipHeadsetRemoveRemoteTalker(pHeadset, pRemoteUser);
    }
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetProcess

    \Description
        Headset process function.

    \Input *pHeadset    - pointer to headset state
    \Input uFrameCount  - process iteration counter

    \Version 03/31/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetProcess(VoipHeadsetRefT *pHeadset, uint32_t uFrameCount)
{
    if (pHeadset->iPlayerActive)
    {
        // process playing
        _VoipHeadsetProcessPlay(pHeadset);
    }
    else
    {
        // process recording
        _VoipHeadsetProcessRecord(pHeadset);
    }

    // process narration
    VoipNarrateUpdate(pHeadset->pNarrateRef);

    // process transcription
    _VoipHeadsetProcessTranscription(pHeadset);

    // process playback
    _VoipHeadsetProcessPlayback(pHeadset);
    
    // process device change requests
    _VoipHeadsetProcessDeviceChange(pHeadset, &pHeadset->MicrInfo);
    _VoipHeadsetProcessDeviceChange(pHeadset, &pHeadset->SpkrInfo);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetSetVolume

    \Description
        Sets play and record volume.

    \Input *pHeadset    - pointer to headset state
    \Input iPlayVol     - play volume to set
    \Input iRecVol      - record volume to set

    \Notes
        To not set a value, specify it as -1.

    \Version 03/31/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetSetVolume(VoipHeadsetRefT *pHeadset, int32_t iPlayVol, uint32_t iRecVol)
{
    if (iPlayVol != -1)
    {
        pHeadset->SpkrInfo.iNewVolume = iPlayVol;
    }
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetControl

    \Description
        Control function.

    \Input *pHeadset    - headset module state
    \Input iControl      - control selector
    \Input iValue       - control value
    \Input iValue2      - control value
    \Input *pValue      - control value

    \Output
        int32_t         - selector specific, or -1 if no such selector

    \Notes
        iControl can be one of the following:

        \verbatim
            'aloc' - promote user to 'participating' state, or demote user from 'participating' state
            'cdec' - create new codec
            'cide' - close voip input device
            'code' - close voip output device
            'cstm' - clear speech to text metrics in VoipSpeechToTextMetricsT
            'ctsm' - clear text to speech metrics in VoipTextToSpeechMetricsT
            'edev' - enumerate voip input/output devices
            'idev' - select voip input device
            'loop' - enable/disable loopback
            'micr' - enable/disable recording
            'mute' - enable/disable audio input muting
            'odev' - select voip output device
            'play' - enable/disable playing
            'txta' - enable/disable text chat accessibility, this also controls loopback in this context
            'tran' - enable/disable local generation of transcribed text for speech procuced by local users (speech-to-text component)
            'svol' - changes speaker volume
            'ttos' - send utf8 text in pValue as voice and initialize TTS voice
            'xply' - enable/disable crossplay (no difference on PC)

        \endverbatim

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipHeadsetControl(VoipHeadsetRefT *pHeadset, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'aloc')
    {
        int32_t iLocalUserIndex;

        if (iValue2 == 0)
        {
            pHeadset->iParticipatingUserIndex = VOIP_INVALID_LOCAL_USER_INDEX;
            NetPrintf(("voipheadsetpc: no user participating\n", iValue));
        }
        else
        {
            pHeadset->iParticipatingUserIndex = iValue;
            NetPrintf(("voipheadsetpc: user %d, entering participating state\n", iValue));
        }

        // whenever active user status changes all mute masks will be re-calculated so lets reset our state
        for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS; ++iLocalUserIndex)
        {
            pHeadset->aLocalUsers[iLocalUserIndex].uPlaybackFlags = 0xFFFFFFFF;
        }

        return(0);
    }
    if (iControl == 'cdec')
    {
        return(_VoipHeadsetSetCodec(pHeadset, iValue));
    }
    if ((iControl == 'cide') || (iControl == 'code'))
    {
        VoipHeadsetDeviceInfoT *pDeviceInfo = (iControl == 'cide') ? &pHeadset->MicrInfo : &pHeadset->SpkrInfo;
        NetCritEnter(&pHeadset->DevChangeCrit);
        pDeviceInfo->bCloseDevice = TRUE;
        NetCritLeave(&pHeadset->DevChangeCrit);
    }
    if (iControl == 'cstm')
    {
        if (pHeadset->pTranscribeRef != NULL)
        {
            return(VoipTranscribeControl(pHeadset->pTranscribeRef, iControl, iValue, 0, NULL));
        }
        return(-1);
    }
    if (iControl == 'ctsm')
    {
        return(VoipNarrateControl(pHeadset->pNarrateRef, iControl, 0, 0, NULL));
    }
    if (iControl == 'edev')
    {
        _VoipHeadsetEnumerateDevices(pHeadset, &pHeadset->MicrInfo);
        _VoipHeadsetEnumerateDevices(pHeadset, &pHeadset->SpkrInfo);
        return(0);
    }
    if ((iControl == 'idev') || (iControl == 'odev'))
    {
        VoipHeadsetDeviceInfoT *pDeviceInfo = (iControl == 'idev') ? &pHeadset->MicrInfo : &pHeadset->SpkrInfo;
        if (pDeviceInfo->iActiveDevice != iValue)
        {
            NetPrintf(("voipheadsetpc: '%C' selector used to change %s device from %d to %d\n", iControl, pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output",
                pDeviceInfo->iActiveDevice, iValue));
            NetCritEnter(&pHeadset->DevChangeCrit);
            pDeviceInfo->iDeviceToOpen = iValue;
            pDeviceInfo->bChangeDevice = TRUE;
            NetCritLeave(&pHeadset->DevChangeCrit);
        }
        else
        {
            NetPrintf(("voipheadsetpc: '%C' selector ignored because %s device %d is already active\n", iControl, pDeviceInfo->eDevType == VOIP_HEADSET_INPDEVICE ? "input" : "output",
                pDeviceInfo->iActiveDevice, iValue));
        }
        return(0);
    }
    if (iControl == 'loop')
    {
        pHeadset->bLoopback = iValue;
        NetPrintf(("voipheadsetpc: loopback mode %s\n", (iValue ? "enabled" : "disabled")));
        return(0);
    }
    if (iControl == 'micr')
    {
        pHeadset->bMicOn = (uint8_t)iValue;
        NetPrintf(("voipheadsetpc: mic %s\n", (iValue ? "enabled" : "disabled")));
        return(0);
    }
    if (iControl == 'mute')
    {
        uint8_t bMuted = iValue2 ? TRUE : FALSE;
        if (pHeadset->bMuted != bMuted)
        {
            NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetpc: mute %s\n", (pHeadset->bMuted ? "enabled" : "disabled")));
            pHeadset->bMuted = bMuted;
        }
        return(0);
    }
    if ((iControl == '-pbk') || (iControl == '+pbk'))
    {
        uint8_t bVoiceEnable = (iControl == '+pbk') ? TRUE : FALSE;
        int32_t iRetCode = 0; // default to success

        VoipUserT *pRemoteUser;

        if ((pValue != NULL) && (iValue < VOIP_MAXLOCALUSERS_EXTENDED))
        {
            pRemoteUser = (VoipUserT *)pValue;

            // make sure the local user and the remote user are not a shared user (shared user concept not supported on pc)
            if ((iValue != VOIP_SHARED_USER_INDEX) && ((pRemoteUser->AccountInfo.iPersonaId & VOIP_SHARED_USER_MASK) != VOIP_SHARED_USER_VALUE))
            {
                _VoipHeadsetSetRemoteUserVoicePlayback(pHeadset, pRemoteUser, iValue, bVoiceEnable);
            }
        }
        else
        {
            NetPrintf(("voipheadsetpc: VoipHeadsetControl('%C', %d) invalid arguments\n", iControl, iValue));
            iRetCode = -2;
        }
        return(iRetCode);
    }
    #if DIRTYCODE_LOGGING
    if (iControl == 'spam')
    {
        pHeadset->iDebugLevel = iValue;
        NetPrintf(("voipheadsetpc: debuglevel=%d\n", pHeadset->iDebugLevel));
        VoipConduitControl(pHeadset->pConduitRef, 'spam', iValue, pValue);
        return(VoipCodecControl(VOIP_CODEC_ACTIVE, iControl, iValue, 0, NULL));
    }
    #endif
    if (iControl == 'txta')
    {
        pHeadset->bTextChatAccessibility = pHeadset->bLoopback = iValue;
        NetPrintf(("voipheadsetpc: text chat accessibility mode %s\n", (iValue ? "enabled" : "disabled")));
        return(0);
    }
    if (iControl == 'play')
    {
        if (iValue)
        {
            pHeadset->pPlayerBuffer = (int16_t *) pValue;
            pHeadset->uPlayerBufferFrameCurrent = 0;
            pHeadset->uPlayerBufferFrames = iValue / (VOIP_HEADSET_SAMPLEWIDTH * VOIP_HEADSET_FRAMESAMPLES);
            pHeadset->uPlayerFirstTime = 0;

            pHeadset->iPlayerActive = 1;
        }
        else
        {
            pHeadset->iPlayerActive = 0;
        }

        NetPrintf(("voipheadsetpc: play %s\n", ((pHeadset->iPlayerActive) ? "enabled" : "disabled")));

        return(0);
    }
    if (iControl == 'tran')
    {
        if (iValue != pHeadset->bVoiceTranscriptionEnabled)
        {
            pHeadset->bVoiceTranscriptionEnabled = iValue;
            NetPrintf(("voipheadsetpc: %s voice transcription locally\n", pHeadset->bVoiceTranscriptionEnabled ? "enabling" : "disabling"));
            
            // create the transcription module the first time we need it
            if (pHeadset->bVoiceTranscriptionEnabled && (pHeadset->pTranscribeRef == NULL))
            {
                // allocate transcription module; give it 16k buffer to match ssl max frame size for efficient network transport
                if ((pHeadset->pTranscribeRef = VoipTranscribeCreate(16 * 1024)) != NULL)
                {
                    // since this is being created late, set the debug level to the current setting
                    #if DIRTYCODE_LOGGING
                    VoipTranscribeControl(pHeadset->pTranscribeRef, 'spam', pHeadset->iDebugLevel, 0, NULL);
                    #endif
                }
                else
                {
                    NetPrintf(("voipheadsetpc: unable to allocate transcription manager\n"));
                    return(-1);
                }
            }
        }
        return(0);
    }
    if (iControl == 'svol')
    {
        VoipHeadsetSetVolume(pHeadset, iValue, 0);
        return(0);
    }
    if (iControl == 'voic')
    {
        int iRet = 0;
        const VoipSynthesizedSpeechCfgT *pCfg = (const VoipSynthesizedSpeechCfgT *)pValue;
        pHeadset->eDefaultGender = (pCfg->iPersonaGender == 1) ? VOIPNARRATE_GENDER_FEMALE : VOIPNARRATE_GENDER_MALE;

        if (pCfg->iLanguagePackCode != 0)
        {
            iRet = VoipNarrateControl(pHeadset->pNarrateRef, 'lang', pCfg->iLanguagePackCode, 0, NULL);
        }

        return(iRet);
    }
    if (iControl == 'ttos')
    {
        VoipNarrateGenderE eGender = ((iValue2 > VOIPNARRATE_GENDER_NONE) && (iValue2 < VOIPNARRATE_NUMGENDERS)) ? (VoipNarrateGenderE)iValue2 : pHeadset->eDefaultGender;
        return(VoipNarrateInput(pHeadset->pNarrateRef, iValue, eGender, (const char *)pValue));
    }
    if (iControl == 'uvoc')
    {
        VoipNarrateControl(pHeadset->pNarrateRef, 'uvoc', 0, 0, NULL);
        return(0);
    }
    if(iControl == 'xply')
    {
        uint8_t bCrossplay = iValue ? TRUE : FALSE;

        if (pHeadset->bCrossplay != bCrossplay)
        {
            NetPrintf(("voipheadsetpc: changing crossplay mode to: %s", bCrossplay ? "crossplay" : "native"));
            pHeadset->bCrossplay = bCrossplay;
        }
        return(0);
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetStatus

    \Description
        Status function.

    \Input *pHeadset    - headset module state
    \Input iSelect      - control selector
    \Input iValue       - selector specific
    \Input *pBuf        - buffer pointer
    \Input iBufSize     - buffer size

    \Output
        int32_t         - selector specific, or -1 if no such selector

    \Notes
        iSelect can be one of the following:

        \verbatim
            'ndev' - zero
            'idev' - get name of input device at index iValue (-1 returns number of input devices)
            'odev' - get name of output device at index iValue (-1 returns number of input devices)
            'idft' - get the default input device index (as specified in control panel)
            'mute' - get muted status
            'odft' - get the default output device index (as specified in control panel)
            'ruvu' - return TRUE if the given remote user (pBuf) is registered with voipheadset, FALSE if not.
            'sttm' - get the VoipSpeechToTextMetricsT via pBuf
            'ttos' - returns TRUE if TTS is active, else FALSE
            'ttsm' - get the VoipTextToSpeechMetricsT via pBuf
            'xply' - return status on crossplay

        \endverbatim

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipHeadsetStatus(VoipHeadsetRefT *pHeadset, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'ndev')
    {
        return(0);
    }
    if ((iSelect == 'idev') || (iSelect == 'odev'))
    {
        VoipHeadsetDeviceInfoT *pDeviceInfo = (iSelect == 'idev') ? &pHeadset->MicrInfo : &pHeadset->SpkrInfo;
        if (iValue == -1)
        {
            return(pDeviceInfo->iNumDevices);
        }
        else if ((iValue >= 0) && (iValue < pDeviceInfo->iNumDevices))
        {
            ds_memcpy(pBuf, pDeviceInfo->WaveDeviceCaps[iValue].szPname, sizeof(pDeviceInfo->WaveDeviceCaps[iValue].szPname));
            return(iValue);
        }
    }
    if (iSelect == 'idft')
    {
        DWORD dwPreferredDevice=0;
        DWORD dwStatusFlags=0;
        DWORD dwRetVal = 0;

        dwRetVal = waveInMessage((HWAVEIN)VOIP_HEADSET_WAVE_MAPPER, DRVM_MAPPER_CONSOLEVOICECOM_GET, (DWORD_PTR)&dwPreferredDevice, (DWORD_PTR)&dwStatusFlags);
        return((int32_t)dwPreferredDevice);
    }
    if (iSelect == 'mute')
    {
        return(pHeadset->bMuted);
    }
    if (iSelect == 'odft')
    {
        DWORD dwPreferredDevice=0;
        DWORD dwStatusFlags;
        DWORD dwRetVal;
        uint32_t uMessage;
                
        switch (iValue)
        {
            case VOIP_DEFAULTDEVICE_VOICECOM:
                dwStatusFlags = 0;
                uMessage = DRVM_MAPPER_CONSOLEVOICECOM_GET;
                break;

            case VOIP_DEFAULTDEVICE_VOICECOM_ONLY:
                dwStatusFlags = DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY;
                uMessage = DRVM_MAPPER_CONSOLEVOICECOM_GET;
                break;

            case VOIP_DEFAULTDEVICE_PREFERRED:
                dwStatusFlags = 0;
                uMessage = DRVM_MAPPER_PREFERRED_GET;
                break;

            case VOIP_DEFAULTDEVICE_PREFERRED_ONLY:
                dwStatusFlags = DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY;
                uMessage = DRVM_MAPPER_PREFERRED_GET;
                break;

            default:
                NetPrintf(("voipheadsetpc: iValue=%d, which is not a VoipHeadsetPreferredDeviceTypeE, using default\n", iValue));
                dwStatusFlags = 0;
                uMessage = DRVM_MAPPER_CONSOLEVOICECOM_GET;
                break;
        }

        dwRetVal = waveOutMessage((HWAVEOUT)VOIP_HEADSET_WAVE_MAPPER, uMessage, (DWORD_PTR)&dwPreferredDevice, (DWORD_PTR)&dwStatusFlags);

        return((int32_t)dwPreferredDevice);
    }
    if (iSelect == 'ruvu')
    {
        int32_t iRemoteUserSpaceIndex = 0;

        for (iRemoteUserSpaceIndex = 0; iRemoteUserSpaceIndex < pHeadset->iMaxRemoteUsers; ++iRemoteUserSpaceIndex)
        {
            if (VOIP_SameUser(&pHeadset->aRemoteUsers[iRemoteUserSpaceIndex].User, (VoipUserT *)pBuf))
            {
                // remote user found and it is registered with voipheadset
                return (TRUE);
            }
        }

        return(FALSE);
    }
    if (iSelect == 'sttm')
    {
        if ((pHeadset->pTranscribeRef != NULL) && (iValue ==  pHeadset->iParticipatingUserIndex))
        {
            return(VoipTranscribeStatus(pHeadset->pTranscribeRef, iSelect, iValue, pBuf, iBufSize));
        }
        return(-1);
    }
    if (iSelect == 'ttos')
    {
        return(pHeadset->bNarrating);
    }
    if (iSelect == 'ttsm')
    {
        if (iValue == pHeadset->iParticipatingUserIndex)
        {
            return(VoipNarrateStatus(pHeadset->pNarrateRef, iSelect, iValue, pBuf, iBufSize));
        }
        return(-1);
    }
    if (iSelect == 'xply')
    {
        return(pHeadset->bCrossplay);
    }
    // unhandled result, fallthrough to active codec
    return(VoipCodecStatus(VOIP_CODEC_ACTIVE, iSelect, iValue, pBuf, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetSpkrCallback

    \Description
        Set speaker output callback.

    \Input *pHeadset    - headset module state
    \Input *pCallback   - what to call when output data is available
    \Input *pUserData   - user data for callback

    \Version 12/12/2005 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetSpkrCallback(VoipHeadsetRefT *pHeadset, VoipSpkrCallbackT *pCallback, void *pUserData)
{
    pHeadset->pSpkrDataCb = pCallback;
    pHeadset->pSpkrCbUserData = pUserData;
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetConfigTranscription

    \Description
        Configure the transcribe module

    \Input *pHeadset    - headset module state
    \Input uProfile     - transcribe profile
    \Input *pUrl        - transcribe provider url
    \Input *pKey        - transcribe key
    
    \Version 11/06/2018 (tcho)
*/
/********************************************************************************F*/
void VoipHeadsetConfigTranscription(VoipHeadsetRefT *pHeadset, uint32_t uProfile, const char *pUrl, const char *pKey)
{
    VoipTranscribeConfig(uProfile, pUrl, pKey);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetConfigNarration

    \Description
        Configure the transcribe module

    \Input *pHeadset    - headset module state
    \Input uProvider    - narrate provider identifier
    \Input *pUrl        - narrate provider url
    \Input *pKey        - narrate key

    \Version 1/07/2019 (tcho)
*/
/********************************************************************************F*/
void VoipHeadsetConfigNarration(VoipHeadsetRefT *pHeadset, uint32_t uProvider, const char *pUrl, const char *pKey)
{
    VoipNarrateConfig((VoipNarrateProviderE)uProvider, pUrl, pKey);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetSetFirstPartyIdCallback

    \Description
        Callback to tell dirtysdk what the first party id is for this user

    \Input *pHeadset    - headset module state
    \Input *pCallback   - what to call when transcribed text is received from remote player
    \Input *pUserData   - user data for callback

    \Version 04/28/2020 (eesponda)
*/
/********************************************************************************F*/
void VoipHeadsetSetFirstPartyIdCallback(VoipHeadsetRefT *pHeadset, VoipFirstPartyIdCallbackCbT *pCallback, void *pUserData)
{
}
