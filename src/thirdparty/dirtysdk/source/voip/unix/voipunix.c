/*H********************************************************************************/
/*!
    \File voipunix.c

    \Description
        voip interface (not production ready - for DirtyCast voip stress testing client)

    \Copyright
        Copyright (c) Electronic Arts 2017. ALL RIGHTS RESERVED.

    \Version 06/08/2017 (mclouatre) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

// dirtysock includes
#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"

// voip includes
#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"
#include "DirtySDK/voip/voip.h"


/*** Defines **********************************************************************/
#define VOIP_THREAD_SLEEP_DURATION        (20)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! VoIP module state
struct VoipRefT
{
    VoipCommonRefT  Common;             //!< cross-platform voip data (must come first!)

    // dummy traffic injection config
    uint32_t uSendSeqn;                 //!< send sequence number
    int32_t  iDummyVoipSubPacketSize;   //!< size of test packets (in bytes)
    uint32_t uDummyVoipSendPeriod;      //!< rate of artificially injected subpackets   0: disabled; [1,1024]: 0% to 100% of voip bandwidth produced over a 20-sec window
    uint32_t uDummyVoipWrappingCounter; //!< 32-bit wrapping counter used to implement the right dummy send period
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _VoipStatusCb

    \Description
        Callback to handle change of headset status.

    \Input iLocalUserIndex - headset that has changed
    \Input bStatus         - if TRUE the headset was inserted, else if FALSE it was removed
    \Input eUpdate         - functionality of the headset (input, output or both)
    \Input *pUserData      - pointer to callback user data

    \Notes
        It is assumed that eUpdate will always be VOIP_HEADSET_STATUS_INOUT for 
        this platform.

    \Version 06/12/2017 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipStatusCb(int32_t iLocalUserIndex, uint32_t bStatus, VoipHeadsetStatusUpdateE eUpdate, void *pUserData)
{
    VoipRefT *pRef = (VoipRefT *)pUserData;

    if (eUpdate != VOIP_HEADSET_STATUS_INOUT)
    {
        NetPrintf(("voipunix: warning, unexpected headset event for this platform!\n"));
    }

    if (bStatus)
    {
        NetPrintf(("voipunix: [%d] headset inserted\n", iLocalUserIndex));
        pRef->Common.uPortHeadsetStatus |= 1 << iLocalUserIndex;
        pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] |= (VOIP_LOCAL_USER_HEADSETOK|VOIP_LOCAL_USER_INPUTDEVICEOK|VOIP_LOCAL_USER_OUTPUTDEVICEOK);
    }
    else
    {
        NetPrintf(("voipunix: [%d] headset removed\n", iLocalUserIndex));
        pRef->Common.uPortHeadsetStatus &= ~(1 << iLocalUserIndex);
        pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] &= ~(VOIP_LOCAL_USER_HEADSETOK|VOIP_LOCAL_USER_INPUTDEVICEOK|VOIP_LOCAL_USER_OUTPUTDEVICEOK);
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipInjectDummyTraffic

    \Description
        Inject dummy voip traffic on all active voip connections

    \Input *pVoip       - module state

    \Version 06/12/2017 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipInjectDummyTraffic(VoipRefT *pVoip)
{
    static uint8_t DummyVoipSubPacket[VOIP_MAXMICRPKTSIZE];
    VoipConnectionlistT *pConnectionlist = &pVoip->Common.Connectionlist;

    // only inject dummy sub-packets if the 1024-large counter is in between [0, pVoip->uDummyVoipSendPeriod[
    if ((++pVoip->uDummyVoipWrappingCounter % 1024) < pVoip->uDummyVoipSendPeriod)
    {
        // send dummy traffic on all active connections
        VoipConnectionSend(pConnectionlist, 0xFFFFFFFF, &DummyVoipSubPacket[0], pVoip->iDummyVoipSubPacketSize, NULL, 0, 0, pVoip->uSendSeqn++);
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipUpdate

    \Description
        Update function to be used in a single-threaded environment

    \Input *pVoip    - voip ref

    \Version 01/18/2018 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipUpdate(VoipRefT *pVoip)
 {
    uint32_t uTime0, uTime1;
    int32_t iSleep;

    uTime0 = NetTick();

    if (pVoip->Common.bApplyChannelConfig)
    {
        pVoip->Common.bApplyChannelConfig = FALSE;
        VoipCommonApplyChannelConfig((VoipCommonRefT *)pVoip);
    }

    // update status of remote users
    VoipCommonUpdateRemoteStatus(&pVoip->Common);
    VoipConnectionUpdate(&pVoip->Common.Connectionlist);

    // inject dummy voip traffic for stress test purposes
    _VoipInjectDummyTraffic(pVoip);

    uTime1 = NetTick();

    iSleep = VOIP_THREAD_SLEEP_DURATION - NetTickDiff(uTime1, uTime0);

    // wait for next tick
    if (iSleep >= 0)
    {
        NetConnSleep(iSleep);
    }
    else
    {
        NetPrintf(("voipunix: [WARNING] Overall voip update took %d ms\n", NetTickDiff(uTime1, uTime0)));
    }
}

/*** Public Functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function   VoipStartup

    \Description
        Prepare VoIP for use.

    \Input iMaxPeers    - maximum number of peers supported (up to VOIP_MAXCONNECT)
    \Input iData        - platform-specific - unused on Unix

    \Output
        VoipRefT        - state reference that is passed to all other functions

    \Version 06/12/2017 (mclouatre)
*/
/********************************************************************************F*/
VoipRefT *VoipStartup(int32_t iMaxPeers, int32_t iData)
{
    VoipRefT *pVoip;

    NetPrintf(("voipunix: WARNING - VoipUnix is not for production. Usage reserved for DirtyCast voip stress testing only!\n"));

    // common startup
    if ((pVoip = VoipCommonStartup(iMaxPeers, sizeof(*pVoip), _VoipStatusCb, iData)) == NULL)
    {
        return(NULL);
    }

    // injection of dummy traffic is disabled by default
    pVoip->iDummyVoipSubPacketSize = 0;
    pVoip->uDummyVoipSendPeriod = 0;

    // bPrivileged is always TRUE on unix
    pVoip->Common.bPrivileged = TRUE;

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

    \Version 06/12/2017 (mclouatre)
*/
/********************************************************************************F*/
void VoipShutdown(VoipRefT *pVoip, uint32_t uShutdownFlags)
{
    // make sure we're really started up
    if (VoipGetRef() == NULL)
    {
        NetPrintf(("voipunix: module shutdown called when not in a started state\n"));
        return;
    }

    // common shutdown (must come last as this frees the memory)
    VoipCommonShutdown(&pVoip->Common);
}

/*F********************************************************************************/
/*!
    \Function   VoipSetLocalUser

    \Description
        Register/unregister specified local user with the voip sub-system.

    \Input *pVoip           - module state from VoipStartup
    \Input iLocalUserIndex  - local user index (ignored because MLU not supported on PC)
    \Input bRegister        - ignored

    \Version 06/12/2017 (mclouatre)
*/
/********************************************************************************F*/
void VoipSetLocalUser(VoipRefT *pVoip, int32_t iLocalUserIndex, uint32_t bRegister)
{
    if (bRegister)
    {
        if (VOIP_NullUser(&pVoip->Common.Connectionlist.LocalUsers[0]))
        {
            VoipUserT voipUser;
            ds_memclr(&voipUser, sizeof(voipUser));

            voipUser.ePlatform = VOIP_PLATFORM_LINUX;
            NetConnStatus('acid', iLocalUserIndex, &voipUser.AccountInfo.iAccountId, sizeof(voipUser.AccountInfo.iAccountId));
            NetConnStatus('peid', iLocalUserIndex, &voipUser.AccountInfo.iPersonaId, sizeof(voipUser.AccountInfo.iPersonaId));

            NetPrintf(("voipunix: registering a local user %llx at index %d\n", voipUser.AccountInfo.iPersonaId, iLocalUserIndex));
            VOIP_CopyUser(&pVoip->Common.Connectionlist.LocalUsers[0], &voipUser);
        }
        else
        {
            NetPrintf(("voipunix: warning -- ignoring attempt to register local user because user %llx is already registered\n", pVoip->Common.Connectionlist.LocalUsers[0].AccountInfo.iPersonaId));
        }
    }
    else
    {
        if (!VOIP_NullUser(&pVoip->Common.Connectionlist.LocalUsers[0]))
        {
            // if a participating user demote him first 
            if (pVoip->Common.Connectionlist.aIsParticipating[0] == TRUE)
            {
                VoipCommonActivateLocalUser(&pVoip->Common, 0, FALSE);
            }

            NetPrintf(("voipunix: unregistering local user %llx\n", pVoip->Common.Connectionlist.LocalUsers[0].AccountInfo.iPersonaId));
            ds_memclr(&pVoip->Common.Connectionlist.LocalUsers[0], sizeof(pVoip->Common.Connectionlist.LocalUsers[0]));
        }
        else
        {
            NetPrintf(("voipunix: warning -- ignoring attempt to unregister local user because it is not yet registered\n"));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonActivateLocalUser

    \Description
        Promote/demote specified local user to/from "participating" state

    \Input *pVoipCommon     - voip common state
    \Input iLocalUserIndex  - local user index (ignored because MLU not supported on PC)
    \Input bActivate        - TRUE to activate, FALSE to deactivate

    \Version 06/12/2017 (mclouatre)
*/
/********************************************************************************F*/
void VoipCommonActivateLocalUser(VoipCommonRefT *pVoipCommon, int32_t iLocalUserIndex, uint8_t bActivate)
{
    if (!VOIP_NullUser(&pVoipCommon->Connectionlist.LocalUsers[0]))
    {
        if (bActivate == TRUE)
        {
            NetPrintf(("voipunix: promoting user %llx to participating state\n", &pVoipCommon->Connectionlist.LocalUsers[0].AccountInfo.iPersonaId));
            pVoipCommon->Connectionlist.aIsParticipating[0] = TRUE;

            // reapply playback muting config based on the channel configuration only if this is a participating user
            pVoipCommon->bApplyChannelConfig = TRUE;
        }
        else
        {
            NetPrintf(("voipunix: demoting user %llx from participating state\n", &pVoipCommon->Connectionlist.LocalUsers[0].AccountInfo.iPersonaId));
            pVoipCommon->Connectionlist.aIsParticipating[0] = FALSE;
        }
        VoipHeadsetControl(pVoipCommon->pHeadset, 'aloc', 0, bActivate, &pVoipCommon->Connectionlist.LocalUsers[0]);
    }
    else
    {
        NetPrintf(("voipunix: VoipCommonActivateLocalUser() failed because no local user registered\n"));
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
        int32_t     - selector-specific data

    \Version 06/12/2017 (mclouatre)
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

    \Input *pVoip       - module state from VoipStartup
    \Input iControl     - control selector
    \Input iValue       - selector-specific
    \Input *pValue      - selector-specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'psiz' - set size of artificially injected test voip sub-packets
            'prat' - artificially injected voip sub-packets period -->  0: disabled; [1,1024]: 0% to 100% of voip bandwidth produced over a 20-sec window
            'updt' - to pump the voip sub-system in a single-threaded context 
        \endverbatim

        Unhandled selectors are passed through to VoipCommonControl(), and
        further to VoipHeadsetControl() if unhandled there.

    \Version 06/12/2017 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipControl(VoipRefT *pVoip, int32_t iControl, int32_t iValue, void *pValue)
{
    if (iControl == 'psiz')
    {
        NetPrintf(("voipunix: size of artificially injected test voip sub-packets changed from %d bytes to %d bytes\n", pVoip->iDummyVoipSubPacketSize, iValue));
        pVoip->iDummyVoipSubPacketSize = iValue;
        return(0);
    }
    if (iControl == 'prat')
    {
        NetPrintf(("voipunix: artificially injected voip sub-packets period changed from %d/1024 to %d/1024\n", pVoip->uDummyVoipSendPeriod, iValue));
        pVoip->uDummyVoipSendPeriod = iValue;
        return(0);
    }
    if (iControl == 'updt')
    {
        if (VoipGetRef())
        {
            _VoipUpdate(VoipGetRef());
            return(0);
        }
        else
        {
            NetPrintf(("voipunix: module update called when not in a started state\n"));
            return(-1);
        }
    }
    return(VoipCommonControl(&pVoip->Common, iControl, iValue, pValue));
}


