/*H********************************************************************************/
/*!
    \File voipheadsetstub.c

    \Description
        Stubbed VoIP headset manager. To be used on platform where voip connectivity
        is to be exercised but not voice acquisition/playback. Typically needed
        for stress testing voip connectivity with dummy voip traffic.

    \Copyright
        Copyright Electronic Arts 2017.

    \Version 12/06/2017 (mclouatre)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"
#include "voipconnection.h"
#include "voipheadset.h"

/*** Defines **********************************************************************/

/*** Macros ************************************************************************/

/*** Type Definitions **************************************************************/

//! VOIP module state data
struct VoipHeadsetRefT
{
    int32_t bParticipating;             //!< local user is now in "participating" state

    // speaker callback data
    VoipSpkrCallbackT *pSpkrDataCb;
    void *pSpkrCbUserData;
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Public Variables

// Private Variables

/*** Private Functions ************************************************************/


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipHeadsetCreate

    \Description
        Create the headset manager.

    \Input iMaxConduits     - max number of conduits
    \Input *pMicDataCb      - pointer to user callback to trigger when mic data is ready
    \Input *pTextDataCb     - pointer to user callback to trigger when transcribed text is ready
    \Input *pOpaqueDataCb   - pointer to user callback to trigger when opaque data is ready
    \Input *pStatusCb       - pointer to user callback to trigger when headset status changes
    \Input *pCbUserData     - pointer to user callback data
    \Input iData            - platform-specific - unused for PC

    \Output
        VoipHeadsetRefT *   - pointer to module state, or NULL if an error occured

    \Version 06/12/2017 (mclouatre)
*/
/********************************************************************************F*/
VoipHeadsetRefT *VoipHeadsetCreate(int32_t iMaxConduits, VoipHeadsetMicDataCbT *pMicDataCb, VoipHeadsetTextDataCbT *pTextDataCb, VoipHeadsetOpaqueDataCbT *pOpaqueDataCb, VoipHeadsetStatusCbT *pStatusCb, void *pCbUserData, int32_t iData)
{
    VoipHeadsetRefT *pHeadset;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query mem group data
    VoipCommonMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // make sure we don't exceed maxconduits
    if (iMaxConduits > VOIP_MAXCONNECT)
    {
        NetPrintf(("voipheadsetstub: request for %d conduits exceeds max\n", iMaxConduits));
        return(NULL);
    }

    // allocate and clear module state
    if ((pHeadset = DirtyMemAlloc(sizeof(*pHeadset), VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        return(NULL);
    }
    ds_memclr(pHeadset, sizeof(*pHeadset));

    // return module ref to caller
    return(pHeadset);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetDestroy

    \Description
        Destroy the headset manager.

    \Input *pHeadset    - pointer to headset state

    \Version 06/12/2017 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetDestroy(VoipHeadsetRefT *pHeadset)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

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

    \Version 06/12/2017 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetReceiveVoiceDataCb(VoipUserT *pRemoteUsers, int32_t iRemoteUserSize, int32_t iConsoleId, VoipMicrInfoT *pMicrInfo, uint8_t *pPacketData, void *pUserData)
{

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

    \Version 06/12/2017 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetRegisterUserCb(VoipUserT *pRemoteUser, int32_t iConsoleId, uint32_t bRegister, void *pUserData)
{
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetProcess

    \Description
        Headset process function.

    \Input *pHeadset    - pointer to headset state
    \Input uFrameCount  - process iteration counter

    \Version 06/12/2017 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetProcess(VoipHeadsetRefT *pHeadset, uint32_t uFrameCount)
{
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

    \Version 06/12/2017 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetSetVolume(VoipHeadsetRefT *pHeadset, int32_t iPlayVol, uint32_t iRecVol)
{
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetControl

    \Description
        Control function.

    \Input *pHeadset    - headset module state
    \Input iControl     - control selector
    \Input iValue       - control value
    \Input iValue2      - control value
    \Input *pValue      - control value

    \Output
        int32_t         - selector specific, or -1 if no such selector

    \Notes
        iControl can be one of the following:

        \verbatim
        'aloc' - promote/demote to/from participating state
        'cide' - close voip input device
        'code' - close voip output device
        'edev' - enumerate voip input/output devices
        'idev' - select voip input device
        'loop' - enable/disable loopback
        'micr' - enable/disable recording
        'odev' - select voip output device
        'play' - enable/disable playing
        'svol' - changes speaker volume
        \endverbatim

    \Version 06/12/2017 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipHeadsetControl(VoipHeadsetRefT *pHeadset, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'aloc')
    {
        pHeadset->bParticipating = ((iValue2 == 0) ? FALSE : TRUE);

        NetPrintf(("voipheadsetstub: %s participating state\n", pHeadset->bParticipating ? "entering" : "exiting"));

        return(0);
    }

    return(0);
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
            'ruvu' - always return TRUE because MLU is not supported on unix
        \endverbatim

    \Version 06/12/2017 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipHeadsetStatus(VoipHeadsetRefT *pHeadset, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'ruvu')
    {
        return (TRUE);
    }
    // unhandled result
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetSpkrCallback

    \Description
        Set speaker output callback.

    \Input *pHeadset    - headset module state
    \Input *pCallback   - what to call when output data is available
    \Input *pUserData   - user data for callback

    \Version 06/12/2017 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetSpkrCallback(VoipHeadsetRefT *pHeadset, VoipSpkrCallbackT *pCallback, void *pUserData)
{
    pHeadset->pSpkrDataCb = pCallback;
    pHeadset->pSpkrCbUserData = pUserData;
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
