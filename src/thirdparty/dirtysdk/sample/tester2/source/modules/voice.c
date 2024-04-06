/*H********************************************************************************/
/*!
    \File voice.c

    \Description
        Test Voice over IP

    \Copyright
        Copyright (c) Electronic Arts 2004-2005.  ALL RIGHTS RESERVED.

    \Version 07/22/2004 (jbrookes)  First Version
    \Version 07/09/2012 (akirchner) Added functionality to play pre-recorded file
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#ifdef _WIN32
 #pragma warning(push,0)
 #include <windows.h>
 #pragma warning(pop)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/voip/voip.h"
#include "DirtySDK/voip/voipdef.h"
#include "DirtySDK/voip/voipgroup.h"
#include "DirtySDK/voip/voipnarrate.h"
#include "DirtySDK/voip/voiptranscribe.h"

#if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
#include "DirtySDK/DirtySock/dirtyaddr.h"
#endif

#include "libsample/zlib.h"
#include "libsample/zfile.h"
#include "libsample/zmem.h"
#include "testersubcmd.h"
#include "testermodules.h"

#if !defined(DIRTYCODE_PS4) && (!defined(DIRTYCODE_XBOXONE) || !defined(DIRTYCODE_GDK))
 #include "DirtySDK/voip/voipcodec.h"
#endif

#if defined(DIRTYCODE_PC)
 #include "t2hostresource.h"
 #include "voipaux/voipspeex.h"
#endif
#if defined(DIRTYCODE_PS4) || defined(DIRTYCODE_PC) || defined(DIRTYCODE_STADIA)
 #include "voipaux/voipopus.h"
#endif

/*** Defines **********************************************************************/

// defined in math.h but not part of the standard so just define it here
#define M_PI 3.14159265358979323846

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct VoiceAppT     // gamer daemon state
{
    VoipRefT *pVoip;
    VoipGroupRefT *pVoipGroup;
    int16_t *pBuffer;
    ZFileT iFile;
    int32_t iAddress;
    int32_t iConnID;
    int32_t iSamples;
    int32_t iModulation;
    uint8_t bRecording;
    uint8_t bPlaying;
    uint8_t bZCallback;
    uint8_t _pad;
    uint32_t uClientId;
} VoiceAppT;

/*** Function Prototypes **********************************************************/

static void _VoiceCreate(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceDestroy(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceConnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
#if !defined(DIRTYCODE_PS4) && !defined(DIRTYCODE_STADIA)
static void _VoiceCodecControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
#endif
static void _VoiceSTT(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceTTS(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceRecord(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoicePlay(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceModulate(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceResetChannels(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceSelectChannel(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceShowChannels(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceVolume(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);

static void _VoiceConn(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceUserLocal(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceUserRemote(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceSetLocalUser(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceActivateLocalUser(void *CmdRef, int32_t argc, char *argv[], unsigned char bHelp);

/*** Variables ********************************************************************/

static T2SubCmdT _Voice_Commands[] =
{
    { "create",     _VoiceCreate            },
    { "destroy",    _VoiceDestroy           },
    { "connect",    _VoiceConnect           },
#if !defined(DIRTYCODE_PS4) && (!defined(DIRTYCODE_XBOXONE) || !defined(DIRTYCODE_GDK)) && !defined(DIRTYCODE_STADIA)
    { "cdec",       _VoiceCodecControl      },
#endif
    { "ctrl",       _VoiceControl           },
    { "record",     _VoiceRecord            },
    { "play",       _VoicePlay              },
    { "modulate",   _VoiceModulate          },
    { "resetchans", _VoiceResetChannels     },
    { "selectchan", _VoiceSelectChannel     },
    { "showchans",  _VoiceShowChannels      },
    { "volume",     _VoiceVolume            },
    { "conn",       _VoiceConn              },
    { "userlocal",  _VoiceUserLocal         },
    { "userremote", _VoiceUserRemote        },
    { "set",        _VoiceSetLocalUser      },
    { "activate",   _VoiceActivateLocalUser },
    { "stt",        _VoiceSTT               },
    { "tts",        _VoiceTTS               },
    { "",           NULL                    },
};

static VoiceAppT _Voice_App = { NULL, NULL, NULL, (ZFileT)NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _VoiceSpkrModulate

    \Description
        Voice modulation using a simple ring modulation filter.  Used to test
        modification of voice data using the speaker callback.

    \Input *pApp        - module state
    \Input *pFrameData  - pointer to output data
    \Input iNumSamples  - number of samples in output data

    \Version 11/27/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _VoiceSpkrModulate(VoiceAppT *pApp, int16_t *pFrameData, int32_t iNumSamples)
{
    static int _iCounter = 0;
    int32_t iSample;
    double dSin;

    if (pApp->iModulation == 0)
    {
        return;
    }

    for (iSample = 0; iSample < iNumSamples; iSample += 1)
    {
        dSin = sin(2 * M_PI * pApp->iModulation * _iCounter/15999);
        pFrameData[iSample] = (int16_t)(((int32_t)pFrameData[iSample] * (int32_t)(dSin*32768.0))/32768);
        if (++_iCounter >= 16000)
        {
            _iCounter = 0;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoiceEvntCallback

    \Description
        Event callback - optional callback to receive voice events

    \Input *pVoip       - voip module state
    \Input eCbType      - callback type (VOIP_CBTYPE_*)
    \Input iValue       - callback value
    \Input *pUserData   - user data pointer

    \Version 10/31/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _VoiceEvntCallback(VoipRefT *pVoip, VoipCbTypeE eCbType, int32_t iValue, void *pUserData)
{
    static const char *_strEventName[] = { "VOIP_CBTYPE_AMBREVENT", "VOIP_CBTYPE_HSETEVENT", "VOIP_CBTYPE_FROMEVENT", "VOIP_CBTYPE_SENDEVENT", "VOIP_CBTYPE_TTOSEVENT" };
    ZPrintf("voice: %s event callback (iValue=%d)\n", _strEventName[eCbType], iValue);
}

/*F********************************************************************************/
/*!
    \Function _VoiceSpkrCallback

    \Description
        Speaker callback - optional callback to receive voice data ready for
        output; this module uses this callback to capture voice data for writing
        to a file.

    \Input *pFrameData  - pointer to output data
    \Input iNumSamples  - number of samples in output data
    \Input *pUserData   - app ref

    \Version 10/31/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _VoiceSpkrCallback(int16_t *pFrameData, int32_t iNumSamples, void *pUserData)
{
    VoiceAppT *pApp = (VoiceAppT *)pUserData;
    int32_t iDataSize, iResult;

    // apply modulation if enabled
    _VoiceSpkrModulate(pApp, pFrameData, iNumSamples);

    // no file to write to?
    if (pApp->iFile <= 0)
    {
        return;
    }

    // write audio to output file
    iDataSize = iNumSamples * sizeof(*pFrameData);
    if ((iResult = ZFileWrite(pApp->iFile, pFrameData, iDataSize)) != iDataSize)
    {
        ZPrintf("voice: error %d writing %d samples to file\n", iResult, iNumSamples);
    }
    else
    {
        pApp->iSamples += iNumSamples;
        ZPrintf("voice: wrote %d samples to output file (%d total)\n", iNumSamples, pApp->iSamples);
    }
}

/*F********************************************************************************/
/*!
    \Function _VoiceFirstyParytIdCallback

    \Description
        Retrieves the id of the local user for testing loopback

    \Input uPersonaId   - unused
    \Input *pUserData   - unused

    \Output
        uint64_t        - local player id or zero (error/unhandled)
*/
/********************************************************************************F*/
static uint64_t _VoiceFirstyParytIdCallback(uint64_t uPersonaId, void *pUserData)
{
    uint64_t uPlayerId = 0;
    #if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
    NetConnStatus('xuid', 0, &uPlayerId, sizeof(uPlayerId));
    #elif defined(DIRTYCODE_STADIA)
    NetConnStatus('gpid', 0, &uPlayerId, sizeof(uPlayerId));
    #endif
    return(uPlayerId);
}

/*F********************************************************************************/
/*!
    \Function _VoiceDisplayTranscribedTextCallback

    \Description
        Callback handling notification about transcribed text being ready for local display.

    \Input iConnId          - connection identifier
    \Input iRemoteUserIndex - remote user index
    \Input *pText           - transcribed text
    \Input *pUserData       - callback user data

    \Output
        int32_t             - TRUE

    \Version 10/31/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoiceDisplayTranscribedTextCallback(int32_t iConnId, int32_t iUserIndex, const char *pText, void *pUserData)
{
    ZPrintf("voice: %s user[%d] \"%s\"\n", ((iConnId != -1) ? "remote" : "local"), iUserIndex, pText);
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _CmdVoiceDevSelectProc

    \Description
        Voice device select for PC,

    \Input win      - window handle
    \Input msg      - window message
    \Input wparm    - message parameter
    \Input lparm    - message parameter

    \Output
        LRESULT     - FALSE

    \Version 07/22/2004 (jbrookes)
*/
/********************************************************************************F*/
#if defined(DIRTYCODE_PC)
static LRESULT CALLBACK _CmdVoiceDevSelectProc(HWND win, UINT msg, WPARAM wparm, LPARAM lparm)
{
    // handle init special (create the class)
    if (msg == WM_INITDIALOG)
    {
        char pDeviceName[64];
        int32_t iDevice, iNumDevices;
        VoipRefT *pVoip;

        pVoip = VoipGetRef();

        // add input devices to combo box and select the first item
        iNumDevices = VoipStatus(pVoip, 'idev', -1, NULL, 0);
        for (iDevice = 0; iDevice < iNumDevices; iDevice++)
        {
            VoipStatus(pVoip, 'idev', iDevice, pDeviceName, 64);
            SendDlgItemMessage(win, IDC_VOICEINP, CB_ADDSTRING, 0, (LPARAM)pDeviceName);
        }
        // select default input device, if available; otherwise just pick the first
        if ((iDevice = VoipStatus(pVoip, 'idft', 0, NULL, 0)) == -1)
        {
            iDevice = 0;
        }
        SendDlgItemMessage(win, IDC_VOICEINP, CB_SETCURSEL, iDevice, 0);

        // add output devices to combo box and select the first item
        iNumDevices = VoipStatus(pVoip, 'odev', -1, NULL, 0);
        for (iDevice = 0; iDevice < iNumDevices; iDevice++)
        {
            VoipStatus(pVoip, 'odev', iDevice, pDeviceName, 64);
            SendDlgItemMessage(win, IDC_VOICEOUT, CB_ADDSTRING, 0, (LPARAM)pDeviceName);
        }
        // select default output device, if available; otherwise just pick the first
        if ((iDevice = VoipStatus(pVoip, 'odft', VOIP_DEFAULTDEVICE_VOICECOM, NULL, 0)) == -1)
        {
            iDevice = 0;
        }
        SendDlgItemMessage(win, IDC_VOICEOUT, CB_SETCURSEL, iDevice, 0);
    }

    // handle ok
    if ((msg == WM_COMMAND) && (LOWORD(wparm) == IDOK))
    {
        VoipRefT *pVoip;
        int32_t iInpDev, iOutDev;

        pVoip = VoipGetRef();

        iInpDev = SendDlgItemMessage(win, IDC_VOICEINP, CB_GETCURSEL, 0, 0);
        iOutDev = SendDlgItemMessage(win, IDC_VOICEOUT, CB_GETCURSEL, 0, 0);

        VoipControl(pVoip, 'idev', iInpDev, NULL);
        VoipControl(pVoip, 'odev', iOutDev, NULL);

        DestroyWindow(win);
    }

    // let windows handle
    return(FALSE);
}
#endif

/*F********************************************************************************/
/*!
    \Function _VoiceDestroyApp

    \Description
        Destroy app, freeing modules.

    \Input *pApp    - app state

    \Output
        None.

    \Version 12/12/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _VoiceDestroyApp(VoiceAppT *pApp)
{
    if (pApp->pVoipGroup != NULL)
    {
        VoipGroupDestroy(pApp->pVoipGroup);
    }
    if (pApp->pVoip)
    {
        VoipShutdown(pApp->pVoip, 0);
    }
    ds_memclr(pApp, sizeof(*pApp));
}

/*F********************************************************************************/
/*!
    \Function _VoiceConnSharingCb

    \Description
        Connection Sharing Callback fired by the VoipGroup

    \Input *pVoipGroup  - voip group ref
    \Input eCbType      - event identifier
    \Input iConnId      - connection ID
    \Input *pUserData   - user callback data
    \Input bSending     - client sending flag
    \Input bReceiving   - client receiving flag

    \Version 02/13/2019 (eesponda)
*/
/********************************************************************************F*/
static void _VoiceConnSharingCb(VoipGroupRefT *pVoipGroup, ConnSharingCbTypeE eCbType, int32_t iConnId, void *pUserData, uint8_t bSending, uint8_t bReceiving)
{
    // no behavior is implemented, the connections should be tracked if we want to support multiple concurrent groups
    ZPrintf("voice: conn sharing callback hit with %s for id %d\n",
        eCbType == VOIPGROUP_CBTYPE_CONNSUSPEND ? "VOIPGROUP_CBTYPE_CONNSUSPEND" : "VOIPGROUP_CBTYPE_CONNRESUME", iConnId); 
}

/*

    Voice Commands

*/

/*F*************************************************************************************/
/*!
    \Function _VoiceCreate

    \Description
        Voice subcommand - create voice module

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output None.

    \Version 12/12/2005 (jbrookes)
*/
/**************************************************************************************F*/
static void _VoiceCreate(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;
    int32_t iArg=2, iMaxPeers, iQuality=-1;
    #if defined(DIRTYCODE_PC)
    uint8_t bDevSelect = TRUE;
    #elif defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
    const char *pOpt;
    #endif

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s create -nodevselect -qual <maxpeers>\n", argv[0]);
        return;
    }

    // check for device select disable
    #if defined(DIRTYCODE_PC)
    if ((argc > 2) && !strcmp(argv[iArg], "-nodevselect"))
    {
        bDevSelect = FALSE;
        iArg += 1;
        argc -= 1;
    }
    #endif

    // set quality?
    #if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
    if ((argc > 2) && ((pOpt = strstr(argv[iArg], "-qual")) != NULL))
    {
        pOpt += strlen("-qual") + 1;
        iQuality = strtol(pOpt, NULL, 10);
        iArg += 1;
        argc -= 1;
    }
    #endif

    // allow setting of max number of remote clients, defaulting to sixteen
    iMaxPeers = (argc == 3) ? strtol(argv[iArg], NULL, 10) : 16;

    // start up voice module if it isn't already started
    if ((pApp->pVoip = VoipGetRef()) == NULL)
    {
        if ((pApp->pVoip = VoipStartup(iMaxPeers, 0)) == NULL)
        {
            ZPrintf("voice: failed to create voip module\n");
            return;
        }
    }

    // register and select speex (for PC) and set the default volume to 90
    #if defined(DIRTYCODE_PC)
    VoipControl(pApp->pVoip, 'creg', 'spex', (void *)&VoipSpeex_CodecDef);
    VoipControl(pApp->pVoip, 'svol', 90, NULL);
    #endif
    #if defined(DIRTYCODE_PS4) || defined(DIRTYCODE_PC) || defined(DIRTYCODE_STADIA)
    VoipControl(pApp->pVoip, 'creg', 'opus', (void *)&VoipOpus_CodecDef);
    VoipControl(pApp->pVoip, 'cdec', 'opus', NULL);
    #endif

    // set speaker callback
    VoipSpkrCallback(pApp->pVoip, _VoiceSpkrCallback, pApp);

    // set first party id callback
    VoipRegisterFirstPartyIdCallback(pApp->pVoip, _VoiceFirstyParytIdCallback, pApp);

    if (iQuality != -1)
    {
        VoipControl(pApp->pVoip, 'qual', iQuality, NULL);
    }

    // create the voipgroup, with max 8 groups
    pApp->pVoipGroup = VoipGroupCreate(8);
    // set event callback
    VoipGroupSetEventCallback(pApp->pVoipGroup, _VoiceEvntCallback, pApp);
    // set the connection sharing callback
    VoipGroupSetConnSharingEventCallback(pApp->pVoipGroup, _VoiceConnSharingCb, pApp);

    // set our clientId using local address
    pApp->uClientId = SocketInfo(NULL, 'addr', 0, NULL, 0);
    VoipGroupControl(pApp->pVoipGroup, 'clid', pApp->uClientId, 0, NULL);

    // select output device (PC only)
    #if defined(DIRTYCODE_PC)
    if (bDevSelect)
    {
        ShowWindow(CreateDialogParam(GetModuleHandle(NULL), "VOICEDEVSELECT", HWND_DESKTOP, (DLGPROC)_CmdVoiceDevSelectProc,0), TRUE);
    }
    #endif
}

/*F*************************************************************************************/
/*!
    \Function _VoiceDestroy

    \Description
        Voice subcommand - destroy voice module

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output None.

    \Version 12/12/2005 (jbrookes)
*/
/**************************************************************************************F*/
static void _VoiceDestroy(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s destroy\n", argv[0]);
        return;
    }

    if (pApp->pBuffer)
    {
        ZMemFree((void *) pApp->pBuffer);
        pApp->pBuffer = NULL;
    }

    _VoiceDestroyApp(pApp);
}

/*F*************************************************************************************/
/*!
    \Function _VoiceConnect

    \Description
        Voice subcommand - connect to peer

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output None.

    \Version 12/12/2005 (jbrookes)
*/
/**************************************************************************************F*/
static void _VoiceConnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;

    if ((bHelp == TRUE) || (argc > 5))
    {
        ZPrintf("   usage: %s connect <address[:port:port]> [rclientid] [connindex]\n", argv[0]);
        return;
    }

    if (argc >= 3)
    {
        uint32_t uRemoteAddr, uRemotePort, uManglePort, uRemoteClientId;
        int32_t iClientIndex;

        // get remote address, port, and local port (if specified) in addr:port:port2 format
        SockaddrInParse2(&uRemoteAddr, (int32_t *)&uRemotePort, (int32_t *)&uManglePort, argv[2]);
        if (uRemotePort == 0)
        {
            uRemotePort = VOIP_PORT;
        }
        if (uManglePort == 0)
        {
            uManglePort = VOIP_PORT;
        }

        // get remote client id, or use remote address if unspecified
        uRemoteClientId = (argc >= 4) ? strtoul(argv[3], NULL, 16) : uRemoteAddr;
        iClientIndex = (argc >= 5) ? strtol(argv[4], NULL, 10) : VOIP_CONNID_NONE;

        // disable loop-back mode, in case it was previously set
        VoipGroupControl(pApp->pVoipGroup, 'loop', FALSE, 0, NULL);

        // log connection attempt
        ZPrintf("%s: connecting to %a:%u:%u (clientId=0x%08x)\n", argv[0], uRemoteAddr, uManglePort, uRemotePort, uRemoteClientId);

        // start connect to remote peer
        VoipGroupControl(pApp->pVoipGroup, 'vcid', iClientIndex, 0, &pApp->uClientId);
        pApp->iConnID = VoipGroupConnect(pApp->pVoipGroup, iClientIndex, uRemoteAddr, uManglePort, uRemotePort, pApp->uClientId, 0, FALSE, uRemoteClientId);
    }
    else
    {
        #if !defined(DIRTYCODE_XBOXONE) || !defined(DIRTYCODE_GDK)
        // set loopback
        ZPrintf("%s: enabling loopback\n", argv[0]);
        VoipGroupControl(pApp->pVoipGroup, 'loop', TRUE, 0, NULL);
        #else
        ZPrintf("%s: loopback not supported on xboxone\n", argv[0]);
        #endif
    }
}

#if !defined(DIRTYCODE_PS4) && (!defined(DIRTYCODE_XBOXONE) || !defined(DIRTYCODE_GDK)) && !defined(DIRTYCODE_STADIA)
/*F*************************************************************************************/
/*!
    \Function _VoiceCodecControl

    \Description
        Voice control subcommand - set control options

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output None.

    \Version 05/06/2011 (jbrookes)
*/
/**************************************************************************************F*/
static void _VoiceCodecControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s cdec <ident> <cmd> <arg1> [arg2]\n", argv[0]);
        return;
    }

    if ((argc > 4) && (argc < 7))
    {
        int32_t iIdent, iCmd, iValue = 0, iValue2 = 0;

        iIdent = ZGetIntArg(argv[2]);
        iCmd = ZGetIntArg(argv[3]);

        if (argc > 4)
        {
            iValue = ZGetIntArg(argv[4]);
        }
        if (argc > 5)
        {
            iValue2 = ZGetIntArg(argv[5]);
        }

        VoipCodecControl(iIdent, iCmd, iValue, iValue2, NULL);
    }
}
#endif

/*F*************************************************************************************/
/*!
    \Function _VoiceControl

    \Description
        Voice control subcommand - set control options

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output None.

    \Version 05/05/2011 (jbrookes)
*/
/**************************************************************************************F*/
static void _VoiceControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;
    void *pValue = NULL;
    int32_t iValue2;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s ctrl <cmd> <val> <strval>\n", argv[0]);
        return;
    }

    if (argc > 2)
    {
        int32_t iCmd, iValue = 0;

        // get control selector
        iCmd = ZGetIntArg(argv[2]);

        // get control value if specified
        if (argc > 3)
        {
            iValue = ZGetIntArg(argv[3]);
        }
        // if control is clid save the value
        if (iCmd == 'clid')
        {
            pApp->uClientId = (unsigned)iValue;
        }

        // if enabling speech-to-text, set callback
        if (iCmd == 'stot')
        {
            iValue2 = (argc > 4) ? ZGetIntArg(argv[4]) : 0;
            pValue = &iValue2;
            VoipGroupSetDisplayTranscribedTextCallback(pApp->pVoipGroup, _VoiceDisplayTranscribedTextCallback, pApp);
        }
        else if (argc > 4)
        {
            pValue = argv[4];
        }

        VoipControl(pApp->pVoip, iCmd, iValue, pValue);
    }
    else
    {
        ZPrintf("%s: invalid ctrl command\n", argv[0]);
    }
}

/*F*************************************************************************************/
/*!
    \Function _VoiceRecord

    \Description
        Voice subcommand - start/stop recording

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 10/28/2011 (jbrookes)
*/
/**************************************************************************************F*/
static void _VoiceRecord(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;
    uint8_t bStart;

    if ((bHelp == TRUE) || (argc < 3) || (argc > 4))
    {
        ZPrintf("   usage: %s record [start|stop] <filename>\n", argv[0]);
        return;
    }

    // see if we are stopping or starting
    if (!strcmp(argv[2], "start"))
    {
        bStart = TRUE;
    }
    else if (!strcmp(argv[2], "stop"))
    {
        bStart = FALSE;
    }
    else
    {
        ZPrintf("%s: invalid start/stop argument %s\n", argv[0], argv[2]);
        return;
    }

    if (bStart)
    {
        if (pApp->bPlaying)
        {
            ZPrintf("%s: cannot start, currently playing\n", argv[0]);
            return;
        }

        if (pApp->bRecording)
        {
            ZPrintf("%s: cannot start, already recording\n", argv[0]);
            return;
        }
        if (argc != 4)
        {
            ZPrintf("%s: cannot record without a filename to write to\n", argv[0]);
            return;
        }
        if ((pApp->iFile = ZFileOpen(argv[3], ZFILE_OPENFLAG_CREATE|ZFILE_OPENFLAG_WRONLY|ZFILE_OPENFLAG_BINARY)) < 0)
        {
            ZPrintf("%s: error %d opening file '%s' for recording\n", pApp->iFile, argv[0], argv[3]);
        }
        ZPrintf("%s: opened file '%s' for recording\n", argv[0], argv[3]);
        pApp->bRecording = TRUE;
        pApp->iSamples = 0;
    }
    else
    {
        int32_t iResult;
        ZFileT iFileId;

        if (!pApp->bRecording)
        {
            ZPrintf("%s: cannot stop, not recording\n", argv[0]);
            return;
        }

        // save fileid, clear it from ref (stop the callback from writing to it)
        iFileId = pApp->iFile;
        pApp->iFile = 0;
        pApp->bRecording = FALSE;
        ZPrintf("%s: recording stopped, %d samples (%d bytes) written\n", argv[0], pApp->iSamples, pApp->iSamples * sizeof(int16_t));

        // close the file
        if ((iResult = ZFileClose(iFileId)) < 0)
        {
            ZPrintf("%s: error %d closing recording file\n", argv[0], iResult);
        }
    }
}

/*F*************************************************************************************/
/*!
    \Function _VoicePlay

    \Description
        Voice subcommand - start/stop playing

    \Input *pCmdRef - unused
    \Input  argc    - argument count
    \Input *argv[]  - argument list

    \Version 7/9/2012 (akirchner)
*/
/**************************************************************************************F*/
static void _VoicePlay(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT * pApp = & _Voice_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s play [start|stop] <filename>\n", argv[0]);
        return;
    }

    if ((argc >= 3) && (!strcmp(argv[2], "start")))
    {
        int32_t iResult;

        if (pApp->bRecording)
        {
            ZPrintf("%s: cannot start, currently recording\n", argv[0]);
            return;
        }

        if (pApp->bPlaying)
        {
            ZPrintf("%s: cannot start, already playing\n", argv[0]);
            return;
        }

        if (argc != 4)
        {
            ZPrintf("   usage: %s play start <filename>\n", argv[0]);
            return;
        }

        // open file
        if ((pApp->iFile = ZFileOpen(argv[3], ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_BINARY)) < 0)
        {
            ZPrintf("%s: error %d opening file '%s' for playing\n", argv[0], pApp->iFile, argv[3]);
            return;
        }

        // get size
        if ((pApp->iSamples = ZFileSize(pApp->iFile)) < 0)
        {
            ZPrintf("%s: error %d invalid size of file '%s'\n", argv[0], pApp->iFile, argv[3]);
            return;
        }

        // read file
        if ((pApp->pBuffer = (int16_t *) ZMemAlloc(pApp->iSamples)) == NULL)
        {
            ZPrintf("%s: error allocating %d bytes of memory\n", argv[0], pApp->iSamples);
            return;
        }

        if ((iResult = ZFileSeek(pApp->iFile, 0, ZFILE_SEEKFLAG_SET)) < 0)
        {
            ZMemFree((void *) pApp->pBuffer);
            pApp->pBuffer = NULL;

            ZPrintf("%s: error %d seeking begging of file '%s'\n", argv[0], iResult, argv[3]);
            return;
        }

        if (ZFileRead(pApp->iFile, (void *) pApp->pBuffer, pApp->iSamples) < 0)
        {
            ZMemFree((void *) pApp->pBuffer);
            pApp->pBuffer = NULL;

            ZPrintf("%s: error %d reading file '%s'\n", argv[0], pApp->iFile, argv[3]);
            return;
        }

        // close file
        if ((iResult = ZFileClose(pApp->iFile)) < 0)
        {
            ZMemFree((void *) pApp->pBuffer);
            pApp->pBuffer = NULL;

            ZPrintf("%s: error %d closing file '%s' for playing\n", argv[0], iResult, argv[3]);
            return;
        }

        if (VoipControl(pApp->pVoip, 'play', pApp->iSamples, (void *) pApp->pBuffer) < 0)
        {
            ZMemFree((void *) pApp->pBuffer);
            pApp->pBuffer = NULL;

            ZPrintf("%s: failed to activate playing mode\n", argv[0]);
            return;
        }

        ZPrintf("%s: opened file '%s' of %d bytes for playing\n", argv[0], argv[3], pApp->iSamples);

        pApp->iFile    = 0;
        pApp->bPlaying = TRUE;
    }
    else if ((argc >= 3) && (!strcmp(argv[2], "stop")))
    {
        if (argc != 3)
        {
            ZPrintf("   usage: %s play stop\n", argv[0]);
            return;
        }

        if (!pApp->bPlaying)
        {
            ZPrintf("%s: cannot stop, not playing\n", argv[0]);
            return;
        }

        if (VoipControl(pApp->pVoip, 'play', 0, NULL) < 0)
        {
            ZMemFree((void *) pApp->pBuffer);

            ZPrintf("%s: failed to deactivate playing mode\n", argv[0]);
            return;
        }

        pApp->bPlaying = FALSE;

        if (pApp->pBuffer)
        {
            ZMemFree((void *) pApp->pBuffer);
            pApp->pBuffer = NULL;
        }

        ZPrintf("%s: playing stopped\n", argv[0]);
    }
    else
    {
        ZPrintf("%s: invalid argument. Neither start nor stop\n", argv[0]);
        return;
    }
}

/*F*************************************************************************************/
/*!
    \Function _VoiceModulate

    \Description
        Voice subcommand - set voice modulation (zero=disabled)

    \Input *pCmdRef - unused
    \Input  argc    - argument count
    \Input *argv[]  - argument list

    \Version 11/27/2018 (jbrookes)
*/
/**************************************************************************************F*/
static void _VoiceModulate(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT * pApp = & _Voice_App;

    if ((bHelp == TRUE) || (argc > 3))
    {
        ZPrintf("   usage: %s modulate <rate>\n", argv[0]);
        return;
    }

    // get modulation rate
    pApp->iModulation = (argc == 3) ? strtol(argv[2], NULL, 10) : 1000;
}

/*F*************************************************************************************/
/*!
    \Function _VoiceResetChannels

    \Description
        Voice subcommand - reset channel configuration

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 02/16/2012 (mclouatre)
*/
/**************************************************************************************F*/
static void _VoiceResetChannels(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s resetchans\n", argv[0]);
        return;
    }

    ZPrintf("%s: resetting channel config\n", argv[0]);
    VoipResetChannels(VoipGetRef(), 0);
}

/*F*************************************************************************************/
/*!
    \Function _VoiceSelectChannel

    \Description
        Voice subcommand - subscribe/unsubcribes to/from a voip channel

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 02/16/2012 (mclouatre)
*/
/**************************************************************************************F*/
static void _VoiceSelectChannel(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    int32_t iChannel;
    VoipChanModeE eMode;

    if ((bHelp == TRUE) || (argc != 4))
    {
        ZPrintf("   usage: %s selectchan <channel id [0,63]> <talk|listen|both|none>\n", argv[0]);
        return;
    }

    sscanf(argv[2], "%d", &iChannel);
    if ((iChannel < 0) && (iChannel > 63))
    {
        ZPrintf("%s: invalid channel id argument: %s\n", argv[0], argv[2]);
        return;
    }

    if (!strcmp(argv[3], "talk"))
    {
        eMode = VOIP_CHANSEND;
    }
    else if (!strcmp(argv[3], "listen"))
    {
        eMode = VOIP_CHANRECV;
    }
    else if (!strcmp(argv[3], "both"))
    {
        eMode = VOIP_CHANSENDRECV;
    }
    else if (!strcmp(argv[3], "none"))
    {
        eMode = VOIP_CHANNONE;
    }
    else
    {
        ZPrintf("%s: invalid channel mode argument: %s\n", argv[0], argv[3]);
        return;
    }

    ZPrintf("%s: setting channel %d:%s\n", argv[0], iChannel, argv[3]);
    VoipSelectChannel(VoipGetRef(), 0, iChannel, eMode);
}

/*F*************************************************************************************/
/*!
    \Function _VoiceTTS

    \Description
        Voice subcommand - Send Text as Voice

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 05/14/2018 (tcho)
*/
/**************************************************************************************F*/
static void _VoiceTTS(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    char strText[1024];

    if (bHelp)
    {
        ZPrintf("   usage: %s tts config [params] or gender [male|female] or \"text you wish to say\" [user index]\n", argv[0]);
        return;
    }

    if (!ds_stricmp(argv[2], "config"))
    {
        if (argc == 6)
        {
            VoipNarrateProviderE eProvider = VOIPNARRATE_PROVIDER_NONE;
            if (!ds_stricmp(argv[3], "watson")) eProvider = VOIPNARRATE_PROVIDER_IBMWATSON;
            if (!ds_stricmp(argv[3], "microsoft")) eProvider = VOIPNARRATE_PROVIDER_MICROSOFT;
            if (!ds_stricmp(argv[3], "google")) eProvider = VOIPNARRATE_PROVIDER_GOOGLE;
            if (!ds_stricmp(argv[3], "amazon")) eProvider = VOIPNARRATE_PROVIDER_AMAZON;
            VoipConfigNarration(VoipGetRef(), eProvider, argv[4], argv[5]);
        }
        else
        {
            ZPrintf("   usage: %s tts config [none|watson|microsoft|google] <url> <key>\n", argv[0]);
        }
    }
    else if (!ds_stricmp(argv[2], "gender"))
    {
        VoipSynthesizedSpeechCfgT VoipSpeechConfig;
        ds_memclr(&VoipSpeechConfig, sizeof(VoipSpeechConfig));
        if (!ds_stricmp(argv[3], "female"))
        {
            VoipSpeechConfig.iPersonaGender = 1;
        }
        VoipControl(VoipGetRef(), 'voic', 0, &VoipSpeechConfig);
    }
    else
    {
        sscanf(argv[2], "%256[^\n]", strText);
        VoipControl(VoipGetRef(), 'ttos', (argc > 3) ? ds_strtoll(argv[3], NULL, 10) : 0, strText);
    }
}

/*F*************************************************************************************/
/*!
    \Function _VoiceSTT

    \Description
        Voice subcommand - Speech to Text functionality

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 12/13/2018 (jbrookes)
*/
/**************************************************************************************F*/
static void _VoiceSTT(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoipTranscribeProviderE eProvider = VOIPTRANSCRIBE_PROVIDER_NONE;
    VoipTranscribeTransportE eTransport = VOIPTRANSCRIBE_TRANSPORT_HTTP;
    VoipTranscribeFormatE eFormat = VOIPTRANSCRIBE_FORMAT_LI16;
    if (bHelp || ds_stricmp(argv[2], "config") || ((ds_stricmp(argv[3], "none") && (argc != 8))))
    {
        ZPrintf("   usage: %s stt config [none|watson|microsoft|google|amazon] [li16|wav|opus] [http|h2|ws] <url> <key>\n", argv[0]);
        return;
    }
    if (!ds_stricmp(argv[3], "watson")) eProvider = VOIPTRANSCRIBE_PROVIDER_IBMWATSON;
    if (!ds_stricmp(argv[3], "microsoft")) eProvider = VOIPTRANSCRIBE_PROVIDER_MICROSOFT;
    if (!ds_stricmp(argv[3], "google")) eProvider = VOIPTRANSCRIBE_PROVIDER_GOOGLE;
    if (!ds_stricmp(argv[3], "amazon")) eProvider = VOIPTRANSCRIBE_PROVIDER_AMAZON;
    if (!ds_stricmp(argv[4], "wav")) eFormat = VOIPTRANSCRIBE_FORMAT_WAV16;
    if (!ds_stricmp(argv[4], "opus")) eFormat = VOIPTRANSCRIBE_FORMAT_OPUS;
    if (!ds_stricmp(argv[5], "h2")) eTransport = VOIPTRANSCRIBE_TRANSPORT_HTTP2;
    if (!ds_stricmp(argv[5], "ws")) eTransport = VOIPTRANSCRIBE_TRANSPORT_WEBSOCKETS;
    VoipConfigTranscription(VoipGetRef(), VOIPTRANSCRIBE_PROFILE_CONSTRUCT(eProvider, eFormat, eTransport), argv[6], argv[7]);
}

/*F*************************************************************************************/
/*!
    \Function _VoiceShowChannels

    \Description
        Voice subcommand - show local voip channel config

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 02/16/2012 (mclouatre)
*/
/**************************************************************************************F*/
static void _VoiceShowChannels(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    int32_t iChannelSlot, iChannelId;
    char *pChannelModeStr;
    VoipChanModeE eChannelMode;
    char strChannelId[8];
    char strChannelSlot[32];
    char strChannelCfg[32];

    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s showchans\n", argv[0]);
        return;
    }

    for (iChannelSlot = 0; iChannelSlot < VoipGroupStatus(NULL, 'chnc', 0, NULL, 0); iChannelSlot++)
    {
        // get channel id and channel mode
        iChannelId = VoipGroupStatus(NULL, 'chnl', iChannelSlot, &eChannelMode, sizeof(eChannelMode));
        // init to default
        pChannelModeStr = "UNKNOWN";

        // initialize channel mode string and 
        switch (eChannelMode)
        {
            case VOIP_CHANNONE:
                pChannelModeStr = "UNSUBSCRIBED";
                ds_snzprintf(strChannelId, sizeof(strChannelId), "n/a");
                break;
            case VOIP_CHANSEND:
                pChannelModeStr = "TALK-ONLY";
                ds_snzprintf(strChannelId, sizeof(strChannelId), "%03d", iChannelId);
                break;
            case VOIP_CHANRECV:
                pChannelModeStr = "LISTEN-ONLY";
                ds_snzprintf(strChannelId, sizeof(strChannelId), "%03d", iChannelId);
                break;
            case VOIP_CHANSENDRECV:
                pChannelModeStr = "TALK+LISTEN";
                ds_snzprintf(strChannelId, sizeof(strChannelId), "%03d", iChannelId);
                break;
        }

        // build channel slot string
        ds_snzprintf(strChannelSlot, sizeof(strChannelSlot), "VOIP channel slot %d", iChannelSlot);

        // build channel config string
        ds_snzprintf(strChannelCfg, sizeof(strChannelCfg), "id=%s  mode=%s", strChannelId, pChannelModeStr);

        ZPrintf("     %s ----> %s\n", strChannelSlot, strChannelCfg);
    }
}

/*F*************************************************************************************/
/*!
    \Function _VoiceVolume

    \Description
        Voice subcommand - set the output volume level

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output None.

    \Version 04/22/2009 (cadam)
*/
/**************************************************************************************F*/
static void _VoiceVolume(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;
    float fOutputLevel;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s volume <level>\n", argv[0]);
        return;
    }

    if (argc == 3)
    {
        fOutputLevel = strtod(argv[2], NULL);
        ZPrintf("%s: setting volume to %f\n", argv[0], fOutputLevel);
        VoipControl(pApp->pVoip, 'plvl', 0, &fOutputLevel);
    }
}

static void _VoiceSetLocalUser(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;
    int32_t iLocalUserIndex;
    uint32_t bRegister;
    int32_t iOnOffIndex;

#if defined(DIRTYCODE_PS4) || defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
    if (bHelp == TRUE || argc != 4)
    {
        ZPrintf("   usage: %s set <local user index> <on|off>\n", argv[0]);
        return;
    }

    iLocalUserIndex = strtol(argv[2], NULL, 10);
    iOnOffIndex = 3;
#else
    if (bHelp == TRUE || argc != 3)
    {
        ZPrintf("   usage: %s set <on|off>\n", argv[0]);
        return;
    }

    iLocalUserIndex = 0;
    iOnOffIndex = 2;
#endif

    if (strcmp(argv[iOnOffIndex], "on") == 0)
    {
        bRegister = TRUE;
        ZPrintf("%s: registering local user at index %d with voip sub-system\n", argv[0], iLocalUserIndex);
    }
    else
    {
        bRegister = FALSE;
        ZPrintf("%s: unregistering local user at index %d from voip sub-system\n", argv[0], iLocalUserIndex);
    }

    VoipSetLocalUser(pApp->pVoip, iLocalUserIndex, bRegister);
}

static void _VoiceActivateLocalUser(void *CmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;
    uint32_t iLocalUserIndex;
    uint32_t bActivate;
    int32_t iOnOffIndex;

#if defined(DIRTYCODE_PS4) || defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
    if (bHelp == TRUE || argc != 4)
    {
        ZPrintf("   usage: %s participate <local user index> <on|off> \n", argv[0]);
        return;
    }

    iLocalUserIndex = strtol(argv[2], NULL, 10);
    iOnOffIndex = 3;
#else
    if (bHelp == TRUE || argc != 3)
    {
        ZPrintf("   usage: %s participate <on|off> \n", argv[0]);
        return;
    }

    iLocalUserIndex = 0;
    iOnOffIndex = 2;
#endif

    if (strcmp(argv[iOnOffIndex], "on") == 0)
    {
        bActivate = TRUE;
        ZPrintf("%s: promoting local user %d to participating state\n", argv[0], iLocalUserIndex);
    }
    else
    {
        bActivate = FALSE;
        ZPrintf("%s: forcing local user %d out of participating state\n", argv[0], iLocalUserIndex);
    }

    VoipGroupActivateLocalUser(pApp->pVoipGroup, iLocalUserIndex, bActivate);
}

static void _VoiceConn(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;
    uint32_t uOutput;
    int32_t  iConnId = 0;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s connremote <ConnId>\n", argv[0]);
        return;
    }

    if (argc == 3)
    {
        iConnId = strtod(argv[2], NULL);
        uOutput = VoipGroupConnStatus(pApp->pVoipGroup, iConnId);
        ZPrintf("Status = %08x\n", uOutput);
        if (uOutput == 0)
        {
            ZPrintf("No VoipConnRemote flags set\n");
        }
        else
        {
            if (uOutput & VOIP_CONN_CONNECTED)
                ZPrintf("VOIP_CONN_CONNECTED\n");
            if (uOutput & VOIP_CONN_BROADCONN)
                ZPrintf("VOIP_CONN_BROADCONN\n");
            if (uOutput & VOIP_CONN_ACTIVE)
                ZPrintf("VOIP_CONN_ACTIVE\n");
            if (uOutput & VOIP_CONN_STOPPED)
                ZPrintf("VOIP_CONN_STOPPED\n");
        }
    }
}

static void _VoiceUserLocal(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{    
    VoiceAppT *pApp = &_Voice_App;
    uint32_t uOutput;
    int32_t  iUserIndex = 0;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s userlocal <userindex>\n", argv[0]);
        return;
    }

    if (argc == 3)
    {
        iUserIndex = strtod(argv[2], NULL);
        uOutput = VoipLocalUserStatus(pApp->pVoip, iUserIndex);
        ZPrintf("Status = %08x\n", uOutput);
        if (uOutput == 0)
        {
            ZPrintf("No VoipUserLocal flags set\n");
        }
        else
        {
            if (uOutput & VOIP_LOCAL_USER_HEADSETOK)
                ZPrintf("VOIP_LOCAL_USER_HEADSETOK\n");
            if (uOutput & VOIP_LOCAL_USER_TALKING)
                ZPrintf("VOIP_LOCAL_USER_TALKING\n");
            if (uOutput & VOIP_LOCAL_USER_SENDVOICE)
                ZPrintf("VOIP_LOCAL_USER_SENDVOICE\n");
            if (uOutput & VOIP_LOCAL_USER_INPUTDEVICEOK)
                ZPrintf("VOIP_LOCAL_USER_INPUTDEVICEOK\n");
            if (uOutput & VOIP_LOCAL_USER_OUTPUTDEVICEOK)
                ZPrintf("VOIP_LOCAL_USER_OUTPUTDEVICEOK\n");
        }
    }
}

static void _VoiceUserRemote(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;
    uint32_t uOutput;
    int32_t  iUserIndex = 0;
    int32_t  iConnId = 0;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s userremote <ConnId> <userindex>\n", argv[0]);
        return;
    }

    if (argc == 4)
    {
        iConnId = strtod(argv[2], NULL);
        iUserIndex = strtod(argv[3], NULL);
        uOutput = VoipGroupRemoteUserStatus(pApp->pVoipGroup, iConnId, iUserIndex);
        ZPrintf("Status = %08x\n", uOutput);
        if (uOutput == 0)
        {
            ZPrintf("No VoipUserRemote flags set\n");
        }
        else
        {
            if (uOutput & VOIP_REMOTE_USER_HEADSETOK)
                ZPrintf("VOIP_REMOTE_USER_HEADSETOK\n");
            if (uOutput & VOIP_REMOTE_USER_RECVVOICE)
                ZPrintf("VOIP_REMOTE_CONN_RECVVOICE\n");
        }
    }
}


/*F********************************************************************************/
/*!
    \Function _CmdVoiceCb

    \Description
        Update voice command

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[]  - standard arg list

    \Output standard return value

    \Version 07/22/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdVoiceCb(ZContext *argz, int32_t argc, char *argv[])
{
    VoiceAppT *pApp = &_Voice_App;

    // check for kill
    if (argc == 0)
    {
        _VoiceDestroyApp(pApp);
        ZPrintf("%s: killed\n", argv[0]);
        return(0);
    }

    // keep running
    return(ZCallback(&_CmdVoiceCb, 16));
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdVoice

    \Description
        Initiate Voice connection.

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list

    \Output standard return value

    \Version 07/22/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdVoice(ZContext *argz, int32_t argc, char *argv[])
{
    T2SubCmdT *pCmd;
    VoiceAppT *pApp = &_Voice_App;
    unsigned char bHelp;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_Voice_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   test the voip module\n");
        T2SubCmdUsage(argv[0], _Voice_Commands);
        return(0);
    }

    // if no ref yet, make one
    if ((pCmd->pFunc != _VoiceCreate) && (pApp->pVoip == NULL))
    {
        // only create the new ref if we are not exercising VoiceControl('dcde')\VoiceControl('drat')
        // to modify the default sample rate and codec BEFORE the module creation
        if ( !((strcmp(argv[1], "ctrl") == 0) && (strcmp(argv[2], "dcde") == 0)) &&
             !((strcmp(argv[1], "ctrl") == 0) && (strcmp(argv[2], "drat") == 0)) )
        {
            char *pCreate = "create";
            ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
            _VoiceCreate(pApp, 1, &pCreate, bHelp);
        }
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    // one-time install of periodic callback
    if (pApp->bZCallback == FALSE)
    {
        pApp->bZCallback = TRUE;
        return(ZCallback(_CmdVoiceCb, 16));
    }
    else
    {
        return(0);
    }
}
