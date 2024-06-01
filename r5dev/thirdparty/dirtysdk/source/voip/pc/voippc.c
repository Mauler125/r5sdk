/*H********************************************************************************/
/*!
    \File voippc.c

    \Description
        Voip library interface.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 07/27/2004 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)

// dirtysock includes
#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtythread.h"
#include "DirtySDK/dirtysock/netconn.h"

// voip includes
#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"
#include "DirtySDK/voip/voipcodec.h"
#include "DirtySDK/voip/voip.h"

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

//! transmission interval in milliseconds
#define VOIP_THREAD_SLEEP_DURATION        (20)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! VoIP module state
struct VoipRefT
{
    VoipCommonRefT          Common;             //!< cross-platform voip data (must come first!)
    uint8_t                 aIsReadyToParticipate[VOIP_MAXLOCALUSERS_EXTENDED]; //!< TRUE if user wanted to participate but was unable (on PC there can only be 1 participant)
    DWORD                   dwThreadId;         //!< thread ID
    volatile int32_t        iThreadState;       //!< control variable used during thread creation and exit (0=waiting for start, 1=running, 2=shutdown)

    uint32_t                bSpeakerOutput;     //!< whether we are outputting through speakers or not
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private Variables

//! affinity of the voip thread, if unset uses the global DirtySDK thread affinity
static int32_t _Voip_iThreadAffinity = -1;

// Public Variables

/*** Private Functions ************************************************************/

/*F*************************************************************************************************/
/*!
    \Function    _VoipStatusCb

    \Description
        Callback to handle change of headset status.

    \Input iLocalUserIndex - headset that has changed (currently ignored)
    \Input bStatus         - if TRUE the headset was inserted, else if FALSE it was removed
    \Input eUpdate         - functionality of the headset (input, output or both)
    \Input *pUserData      - pointer to callback user data

    \Version    1.0       04/01/04 (JLB) First Version
*/
/*************************************************************************************************F*/
static void _VoipStatusCb(int32_t iLocalUserIndex, uint32_t bStatus, VoipHeadsetStatusUpdateE eUpdate, void *pUserData)
{
    VoipRefT *pRef = (VoipRefT *)pUserData;

    /* we don't support each user having their own device, we will treat all users 
    as they have the same device status, since it is likely shared */
    for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS; iLocalUserIndex++)
    {
        if (bStatus > 0)
        {
            // Set the appropriate flags
            if (eUpdate == VOIP_HEADSET_STATUS_INPUT || eUpdate == VOIP_HEADSET_STATUS_INOUT)
            {
                pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] |= VOIP_LOCAL_USER_INPUTDEVICEOK;
            }

            if (eUpdate == VOIP_HEADSET_STATUS_OUTPUT || eUpdate == VOIP_HEADSET_STATUS_INOUT)
            {
                pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] |= VOIP_LOCAL_USER_OUTPUTDEVICEOK;
            }
        }
        else
        {
            // Reset the appropriate flags
            if (eUpdate == VOIP_HEADSET_STATUS_INPUT || eUpdate == VOIP_HEADSET_STATUS_INOUT)
            {
                pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] &= ~VOIP_LOCAL_USER_INPUTDEVICEOK;
            }

            if (eUpdate == VOIP_HEADSET_STATUS_OUTPUT || eUpdate == VOIP_HEADSET_STATUS_INOUT)
            {
                pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] &= ~VOIP_LOCAL_USER_OUTPUTDEVICEOK;
            }
        }

        if ((pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] & VOIP_LOCAL_USER_INPUTDEVICEOK) &&
            (pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] & VOIP_LOCAL_USER_OUTPUTDEVICEOK))
        {
            NetPrintf(("voippc: headset active (audio in: on, audio out: on\n)"));
            pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] |= VOIP_LOCAL_USER_HEADSETOK;
        }
        else
        {
            NetPrintf(("voippc: headset inactive (audio in: %s, audio out: %s\n)", 
                ((pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] & VOIP_LOCAL_USER_INPUTDEVICEOK) ? "on" : "off"), 
                ((pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] & VOIP_LOCAL_USER_OUTPUTDEVICEOK) ? "on" : "off")));
            pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] &= ~VOIP_LOCAL_USER_HEADSETOK;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipActivateLocalTalker

    \Description
        Register the participating talker on the given port.

    \Input *pVoipCommon     - voip common state
    \Input iLocalUserIndex  - local user index associated with the user
    \Input *pVoipUser       - VoipUserT to register
    \Input bActivate        - TRUE to activate, FALSE to deactivate

    \Output
        int32_t             - negative if failed, zero if successful

    \Version 04/25/2013 (tcho)
*/
/********************************************************************************F*/
static int32_t _VoipActivateLocalTalker(VoipCommonRefT *pVoipCommon, int32_t iLocalUserIndex, VoipUserT *pVoipUser, uint8_t bActivate)
{
    int32_t iResult = 0;

    if (pVoipUser != NULL)
    {
        VoipHeadsetControl(pVoipCommon->pHeadset, 'aloc', iLocalUserIndex, bActivate, pVoipUser);
        
        // mark local user as participating, or not
        pVoipCommon->Connectionlist.aIsParticipating[iLocalUserIndex] = bActivate;

        // reapply playback muting config based on the channel configuration always (a user who was blocking may be gone now)
        pVoipCommon->bApplyChannelConfig = TRUE;

        // announce jip add or remove event on connection that are already active
        VoipConnectionReliableBroadcastUser(&pVoipCommon->Connectionlist, iLocalUserIndex, bActivate);
    }
    else
    {
        NetPrintf(("voippc: cannot %s a null user %s participating state (local user index %d)\n",
            (bActivate ? "promote" : "demote"), (bActivate ? "to" : "from"), iLocalUserIndex));

        iResult = -1;
    }
    
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function   _VoipThread

    \Description
        Main Voip thread that updates all active connection and calls the
        XHV Engine's DoWork() function.

    \Input *pParam      - pointer to voip module state

    \Version 1.0 03/19/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static void _VoipThread(void *pParam)
{
    VoipRefT *pVoip = (VoipRefT *)pParam;
    uint32_t uProcessCt = 0;
    uint32_t uTime0, uTime1;
    int32_t iSleep;

    // wait until we're started up
    while(pVoip->iThreadState == 0)
    {
        NetConnSleep(VOIP_THREAD_SLEEP_DURATION);
    }

    // execute until we're killed
    while(pVoip->iThreadState == 1)
    {
        uTime0 = NetTick();

        NetCritEnter(&pVoip->Common.ThreadCrit);
        NetCritEnter(&pVoip->Common.Connectionlist.NetCrit);
        
        if (pVoip->Common.bApplyChannelConfig)
        {
            pVoip->Common.bApplyChannelConfig = FALSE;
            VoipCommonApplyChannelConfig((VoipCommonRefT *)pVoip);
        }     
        
        // update status of remote users
        VoipCommonUpdateRemoteStatus(&pVoip->Common);
        NetCritLeave(&pVoip->Common.Connectionlist.NetCrit);

        // update connections
        VoipConnectionUpdate(&pVoip->Common.Connectionlist);

        // set headset mute status based on connectionlist send mask
        VoipHeadsetControl(pVoip->Common.pHeadset, 'mute', 0, pVoip->Common.Connectionlist.uSendMask == 0, NULL);

        // update headset
        VoipHeadsetProcess(pVoip->Common.pHeadset, uProcessCt++);

        // update ttos status
        pVoip->Common.uTtsStatus = VoipHeadsetStatus(pVoip->Common.pHeadset, 'ttos', 0, NULL, 0);

        NetCritLeave(&pVoip->Common.ThreadCrit);

        uTime1 = NetTick();

        iSleep = VOIP_THREAD_SLEEP_DURATION - NetTickDiff(uTime1, uTime0);

        // wait for next tick
        if (iSleep >= 0)
        {
            NetConnSleep(iSleep);
        }
        else
        {
            NetPrintf(("voippc: [WARNING] Overall voip thread update took %d ms\n", NetTickDiff(uTime1, uTime0)));
        }
    }

    // cleanup voice and give voipheadset process time to destroy SAPI related components
    VoipHeadsetControl(pVoip->Common.pHeadset, 'uvoc', 0, 0, NULL);
    VoipHeadsetProcess(pVoip->Common.pHeadset, uProcessCt++);

    // indicate we're exiting
    pVoip->iThreadState = 0;
}

/*F********************************************************************************/
/*!
    \Function   _VoipGetActiveParticipantIndex

    \Description
        Gets the user index of the current active user (there can only be one)

    \Input *pVoip   - module state

    \Output
        int32_t     - User index 0-4 or VOIP_INVALID_LOCAL_USER_INDEX if none.

    \Version 03/14/2019 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _VoipGetActiveParticipantIndex(VoipRefT *pVoip)
{
    uint32_t uIndex;
    for (uIndex = 0; uIndex < VOIP_MAXLOCALUSERS; uIndex++)
    {
        if (pVoip->Common.Connectionlist.aIsParticipating[uIndex] == TRUE)
        {
            return(uIndex);
        }
    }
    return(VOIP_INVALID_LOCAL_USER_INDEX);
}

/*F********************************************************************************/
/*!
    \Function   _VoipGetFirstReadyParticipantIndex

    \Description
        Gets the user index of the first user who would like to be the active user

    \Input *pVoip   - module state

    \Output
        int32_t     - User index 0-4 or VOIP_INVALID_LOCAL_USER_INDEX if none.

    \Version 03/14/2019 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _VoipGetFirstReadyParticipantIndex(VoipRefT *pVoip)
{
    uint32_t uIndex;
    for (uIndex = 0; uIndex < VOIP_MAXLOCALUSERS; uIndex++)
    {
        if (pVoip->aIsReadyToParticipate[uIndex] == TRUE)
        {
            return(uIndex);
        }
    }
    return(VOIP_INVALID_LOCAL_USER_INDEX);
}

/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function   VoipStartup

    \Description
        Prepare VoIP for use.

    \Input iMaxPeers    - maximum number of peers supported (up to VOIP_MAXCONNECT)
    \Input iData        - platform-specific - unused for PC

    \Output
        VoipRefT        - state reference that is passed to all other functions

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
VoipRefT *VoipStartup(int32_t iMaxPeers, int32_t iData)
{
    VoipRefT *pVoip;
    VoipCommonRefT *pVoipCommon;
    DirtyThreadConfigT ThreadConfig;

    // common startup
    if ((pVoip = VoipCommonStartup(iMaxPeers, sizeof(*pVoip), _VoipStatusCb, iData)) == NULL)
    {
        return(NULL);
    }
    pVoipCommon = (VoipCommonRefT *)pVoip;

    // bPrivileged is always TRUE on PC
    pVoipCommon->bPrivileged = TRUE;

    // configure thread parameters
    ds_memclr(&ThreadConfig, sizeof(ThreadConfig));
    ThreadConfig.pName = "VoIP";
    ThreadConfig.iAffinity = (_Voip_iThreadAffinity == -1) ? NetConnStatus('affn', 0, NULL, 0) : _Voip_iThreadAffinity;
    ThreadConfig.iPriority = THREAD_PRIORITY_HIGHEST;

    // create worker thread
    if (DirtyThreadCreate(_VoipThread, pVoip, &ThreadConfig) == 0)
    {
        // tell thread to start executing
        pVoip->iThreadState = 1;
    }
    else
    {
        pVoip->iThreadState = 0;
        VoipShutdown(pVoip, 0);
        return(NULL);
    }

    // return to caller
    return(pVoip);
}

/*F********************************************************************************/
/*!
    \Function   VoipShutdown

    \Description
        Release all VoIP resources.

    \Input *pVoip           - module state from VoipStartup
    \Input uShutdownFlags   - NET_SHUTDOWN_* flags

    \Version 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void VoipShutdown(VoipRefT *pVoip, uint32_t uShutdownFlags)
{
    // make sure we're really started up
    if (VoipGetRef() == NULL)
    {
        NetPrintf(("voippc: module shutdown called when not in a started state\n"));
        return;
    }

    // tell thread to shut down and wait for shutdown confirmation (if running)
    if (pVoip->iThreadState == 1)
    {
        for (pVoip->iThreadState = 2; pVoip->iThreadState != 0; )
        {
            NetPrintf(("voippc: waiting for shutdown\n"));
            NetConnSleep(VOIP_THREAD_SLEEP_DURATION);
        }
    }

    // unregister local talker

    // common shutdown (must come last as this frees the memory)
    VoipCommonShutdown(&pVoip->Common);
}

/*F********************************************************************************/
/*!
    \Function   VoipSetLocalUser

    \Description
        Register/unregister specified local user with the voip sub-system.

    \Input *pVoip           - module state from VoipStartup
    \Input iLocalUserIndex  - local user index
    \Input bRegister        - true to register user, false to unregister user

    \Version 04/15/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipSetLocalUser(VoipRefT *pVoip, int32_t iLocalUserIndex, uint32_t bRegister)
{
    if ((iLocalUserIndex < VOIP_MAXLOCALUSERS) && (iLocalUserIndex >= 0))
    {
        NetCritEnter(&pVoip->Common.ThreadCrit);

        if (bRegister)
        {
            if (VOIP_NullUser(&pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex]))
            {
                VoipUserT voipUser;
                ds_memclr(&voipUser, sizeof(voipUser));

                voipUser.ePlatform = VOIP_PLATFORM_PC;
                NetConnStatus('acid', iLocalUserIndex, &voipUser.AccountInfo.iAccountId, sizeof(voipUser.AccountInfo.iAccountId));
                NetConnStatus('peid', iLocalUserIndex, &voipUser.AccountInfo.iPersonaId, sizeof(voipUser.AccountInfo.iPersonaId));

                // register the local talker
                NetPrintf(("voippc: registering a local user %lld at index %d\n", voipUser.AccountInfo.iPersonaId, iLocalUserIndex));
                VOIP_CopyUser(&pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex], &voipUser);
            }
            else
            {
                NetPrintf(("voippc: warning -- ignoring attempt to register user index %d because user %lld is already registered at that index\n",
                    iLocalUserIndex, pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex].AccountInfo.iPersonaId));
            }
        }
        else
        {
            if (!VOIP_NullUser(&pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex]))
            {
                // if a participating user demote him first 
                if (pVoip->Common.Connectionlist.aIsParticipating[iLocalUserIndex] == TRUE)
                {
                    VoipCommonActivateLocalUser(&pVoip->Common, iLocalUserIndex, FALSE);
                }

                NetPrintf(("voippc: unregistering local user %lld at index %d\n", pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex].AccountInfo.iPersonaId, iLocalUserIndex));
                ds_memclr(&pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex], sizeof(pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex]));
            }
            else
            {
                NetPrintf(("voippc: warning -- ignoring attempt to unregister local user because it is not yet registered\n"));
            }
        }

        NetCritLeave(&pVoip->Common.ThreadCrit);
    }
    else
    {
        NetPrintf(("voippc: VoipSetLocalUser() invoked with an invalid user index %d\n", iLocalUserIndex));
    }
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonActivateLocalUser

    \Description
        Promote/demote specified local user to/from "participating" state

    \Input *pVoipCommon     - voip common state
    \Input iLocalUserIndex  - local user index
    \Input bActivate        - TRUE to activate, FALSE to deactivate

    \Version 04/25/2013 (tcho)
*/
/********************************************************************************F*/
void VoipCommonActivateLocalUser(VoipCommonRefT *pVoipCommon, int32_t iLocalUserIndex, uint8_t bActivate)
{
    VoipRefT* pVoip = VoipGetRef();
    if ((iLocalUserIndex < NETCONN_MAXLOCALUSERS) && (iLocalUserIndex >= 0))
    {
        NetCritEnter(&pVoipCommon->ThreadCrit);

        if (!VOIP_NullUser(&pVoipCommon->Connectionlist.LocalUsers[iLocalUserIndex]))
        {
            if (bActivate == TRUE)
            {
                if (_VoipGetActiveParticipantIndex(pVoip) == VOIP_INVALID_LOCAL_USER_INDEX)
                {
                    NetPrintf(("voippc: promoting local user %lld to participating state.\n", &pVoipCommon->Connectionlist.LocalUsers[iLocalUserIndex].AccountInfo.iPersonaId));
                    _VoipActivateLocalTalker(pVoipCommon, iLocalUserIndex, &pVoipCommon->Connectionlist.LocalUsers[iLocalUserIndex], TRUE);
                }
                else
                {
                    NetPrintf(("voippc: a user is already active, %lld to ready state.\n", &pVoipCommon->Connectionlist.LocalUsers[iLocalUserIndex].AccountInfo.iPersonaId));
                }
                pVoip->aIsReadyToParticipate[iLocalUserIndex] = TRUE;
            }
            else
            {
                NetPrintf(("voippc: demoting local user %lld from participating/ready state\n", &pVoipCommon->Connectionlist.LocalUsers[iLocalUserIndex].AccountInfo.iPersonaId));
                
                // were we the active user, if so deactivate
                if (_VoipGetActiveParticipantIndex(pVoip) == iLocalUserIndex)
                {
                    _VoipActivateLocalTalker(pVoipCommon, iLocalUserIndex, &pVoipCommon->Connectionlist.LocalUsers[iLocalUserIndex], FALSE);
                }
                pVoip->aIsReadyToParticipate[iLocalUserIndex] = FALSE;

                // do we still have an active user?
                if (_VoipGetActiveParticipantIndex(pVoip) == VOIP_INVALID_LOCAL_USER_INDEX)
                {
                    // no, but do we have someone who still wants to be the active user? if so make them the active user
                    int32_t iNewParticpantIndex = _VoipGetFirstReadyParticipantIndex(pVoip);
                    if (iNewParticpantIndex != VOIP_INVALID_LOCAL_USER_INDEX)
                    {
                        NetPrintf(("voippc: promoting local user %lld to participating state\n", &pVoipCommon->Connectionlist.LocalUsers[iNewParticpantIndex].AccountInfo.iPersonaId));
                        _VoipActivateLocalTalker(pVoipCommon, iNewParticpantIndex, &pVoipCommon->Connectionlist.LocalUsers[iNewParticpantIndex], TRUE);
                    }
                }
            }
        }
        else
        {
            NetPrintf(("voippc: VoipActivateLocalUser() failed because no local user registered at index %d\n", iLocalUserIndex));
        }

        NetCritLeave(&pVoipCommon->ThreadCrit);
    }
    else
    {
        NetPrintf(("voippc: VoipActivateLocalUser() invoked with an invalid user index %d\n", iLocalUserIndex));
    }
}

/*F********************************************************************************/
/*!
    \Function   VoipStatus

    \Description
        Return status.

    \Input *pVoip   - module state from VoipStartup
    \Input iSelect  - status selector
    \Input iValue   - selector-specific
    \Input *pBuf    - [out] storage for selector-specific output
    \Input iBufSize - size of output buffer

    \Output
        int32_t     - status-specific data

    \Notes
        iSelect can be one of the following:

        \verbatim
            No PC-specific VoipStatus() selectors
        \endverbatim

        Unhandled selectors are passed through to VoipCommonStatus(), and
        further to VoipHeadsetStatus() if unhandled there.

    \Version 1.0 03/02/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipStatus(VoipRefT *pVoip, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    return(VoipCommonStatus(&pVoip->Common, iSelect, iValue, pBuf, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function   VoipControl

    \Description
        Set control options.

    \Input *pVoip   - module state from VoipStartup
    \Input iControl - status selector
    \Input iValue   - selector-specific
    \Input *pValue  - register value

    \Output
        int32_t     - selector-specific data

    \Notes
        iControl can be one of the following:

        \verbatim
            'affn' - set the voip thread cpu affinity, please call this before voip startup is called with pVoip being null
            '+pbk' - enable voice playback for a given remote user (pValue is the remote user VoipUserT).
            '-pbk' - disable voice playback for a given remote user (pValue is the remote user VoipUserT).
            'cdec' - switch to new codec; passes through to VoipHeadsetControl()
            'creg' - register codec
            'plvl' - set the output power level
        \endverbatim

        Unhandled selectors are passed through to VoipCommonControl(), and
        further to VoipHeadsetControl() if unhandled there.

    \Version 1.0 04/08/2007 (cadam)
*/
/********************************************************************************F*/
int32_t VoipControl(VoipRefT *pVoip, int32_t iControl, int32_t iValue, void *pValue)
{
    if ('affn' == iControl)
    {
        _Voip_iThreadAffinity = iValue;
        return(0);
    }
    if (iControl == 'cdec')
    {
        int32_t iResult;
        // if switching codecs we require sole access to thread critical section
        NetCritEnter(&pVoip->Common.ThreadCrit);
        iResult = VoipHeadsetControl(pVoip->Common.pHeadset, iControl, iValue, 0, pValue);
        NetCritLeave(&pVoip->Common.ThreadCrit);
        return(iResult);
    }
    if (iControl == 'creg')
    {
        VoipCodecRegister(iValue, pValue);
        return(0);
    }
    if (iControl == '+pbk')
    {
        return(VoipHeadsetControl(pVoip->Common.pHeadset, '+pbk', iValue, 0, (VoipUserT *)pValue));
    }
    if (iControl == '-pbk')
    {
        return(VoipHeadsetControl(pVoip->Common.pHeadset, '-pbk', iValue, 0, (VoipUserT *)pValue));
    }
    if (iControl == 'plvl')
    {
        int32_t iFractal;
        float fValue = *(float *)pValue;
        if ((fValue >= 20.0f) || (fValue < 0.0f))
        {
            NetPrintf(("voippc: setting power level failed; value must be less than 20 and greater than or equal to 0."));
            return(-1);
        }
        iFractal = (int32_t)(fValue * (1 << VOIP_CODEC_OUTPUT_FRACTIONAL));
        VoipCodecControl(-1, iControl, iFractal, 0, NULL);
        return(0);
    }

    // let VoipCommonControl take a shot at it
    return(VoipCommonControl(&pVoip->Common, iControl, iValue, pValue));
}
