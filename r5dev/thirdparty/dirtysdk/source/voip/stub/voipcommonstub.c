/*H********************************************************************************/
/*!
    \File voipcommonstub.c

    \Description
        A stub for Voipcommon APIs

    \Copyright
        Copyright (c) 2019 Electronic Arts Inc.

    \Version 02/21/2019 (tcho) First Version
*/
/********************************************************************************H*/
#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/voip/voipdef.h"
#include "DirtySDK/voip/voip.h"

#include "voippriv.h"
#include "voipcommon.h"

/*F********************************************************************************/
/*!
    \Function   VoipCommonGetRef

    \Description
        Return current module reference.

    \Output
        VoipRefT *      - reference pointer, or NULL if the module is not active

    \Version 02/21/2019 (tcho)
*/
/********************************************************************************F*/
VoipRefT *VoipCommonGetRef(void)
{
    return(NULL);
}

/*F********************************************************************************/
/*!
\Function   VoipCommonStartup

    \Description
        Start up common functionality

    \Input iMaxPeers    - maximum number of peers supported (up to VOIP_MAXCONNECT)
    \Input iVoipRefSize - size of voip ref to allocate
    \Input *pStatusCb   - headset status callback
    \Input iData        - platform-specific

    \Output
        VoipRefT *      - voip ref if successful; else NULL

    \Version 12/02/2009 (jbrookes)
*/
/********************************************************************************F*/
VoipRefT *VoipCommonStartup(int32_t iMaxPeers, int32_t iVoipRefSize, VoipHeadsetStatusCbT *pStatusCb, int32_t iData)
{
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonShutdown

    \Description
        Shutdown common functionality

    \Input *pVoipCommon - common module state

    \Version 12/02/2009 (jbrookes)
*/
/********************************************************************************F*/
void VoipCommonShutdown(VoipCommonRefT *pVoipCommon)
{
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonUpdateRemoteStatus

    \Description
        Process mute list, and set appropriate flags/priority for each remote user.

    \Input *pVoipCommon - pointer to module state

    \Version 08/23/2005 (jbrookes)
*/
/********************************************************************************F*/
void VoipCommonUpdateRemoteStatus(VoipCommonRefT *pVoipCommon)
{
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonStatus

    \Description
        Return status.

    \Input *pVoipCommon - voip common state
    \Input iSelect      - status selector
    \Input iValue       - selector-specific
    \Input *pBuf        - [out] storage for selector-specific output
    \Input iBufSize     - size of output buffer

    \Output
        int32_t         - selector-specific data

    \Version 12/02/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipCommonStatus(VoipCommonRefT *pVoipCommon, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonControl

    \Description
        Set control options.

    \Input *pVoipCommon - voip common state
    \Input iControl     - control selector
    \Input iValue       - selector-specific input
    \Input *pValue      - selector-specific input

    \Output
        int32_t         - selector-specific output

    \Version 03/02/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipCommonControl(VoipCommonRefT *pVoipCommon, int32_t iControl, int32_t iValue, void *pValue)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonAddMask

    \Description
        Add (OR) uAddMask into *pMask

    \Input *pMask       - mask to add into
    \Input uAddMask     - mask to add (OR)
    \Input *pMaskName   - name of mask (for debug logging)

    \Version 12/03/2009 (jbrookes)
*/
/********************************************************************************F*/
void VoipCommonAddMask(uint32_t *pMask, uint32_t uAddMask, const char *pMaskName)
{
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonDelMask

    \Description
        Del (&~) uDelMask from *pMask

    \Input *pMask       - mask to del from
    \Input uDelMask     - mask to del (&~)
    \Input *pMaskName   - name of mask (for debug logging)

    \Version 12/03/2009 (jbrookes)
*/
/********************************************************************************F*/
void VoipCommonDelMask(uint32_t *pMask, uint32_t uDelMask, const char *pMaskName)
{
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonSetMask

    \Description
        Set value of mask (with logging).

    \Input *pMask       - mask to write to
    \Input uNewMask     - new mask value
    \Input *pMaskName   - name of mask (for debug logging)

    \Version 12/03/2009 (jbrookes)
*/
/********************************************************************************F*/
void VoipCommonSetMask(uint32_t *pMask, uint32_t uNewMask, const char *pMaskName)
{
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonSelectChannel

    \Description
        Select the mode(send/recv) of a given channel.

    \Input *pVoipCommon - common module state
    \Input iUserIndex   - local user index
    \Input iChannel     - Channel ID (valid range: [0,63])
    \Input eMode        - The mode, combination of VOIP_CHANSEND, VOIP_CHANRECV

    \Output
    int32_t         - number of channels remaining that this console could join

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
int32_t VoipCommonSelectChannel(VoipCommonRefT *pVoipCommon, int32_t iUserIndex, int32_t iChannel, VoipChanModeE eMode)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonApplyChannelConfig

    \Description
        Setup user muting flags based on channel config

    \Input *pVoipCommon - voip module state

    \Version 12/02/2009 (jrainy)
*/
/********************************************************************************F*/
void VoipCommonApplyChannelConfig(VoipCommonRefT *pVoipCommon)
{
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonResetChannels

    \Description
        Resets the channels selection to defaults. Sends and receives to all

    \Input *pVoipCommon     - voip common state
    \Input iUserIndex       - local user index

    \Version 12/07/2009 (jrainy)
*/
/********************************************************************************F*/
void VoipCommonResetChannels(VoipCommonRefT *pVoipCommon, int32_t iUserIndex)
{
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonMicrophone

    \Description
        Select which peers to send voice to

    \Input *pVoipCommon     - voip common state
    \Input uUserMicrValue   - microphone bit values

    \Version 01/30/2019 (eesponda)
*/
/********************************************************************************F*/
void VoipCommonMicrophone(VoipCommonRefT *pVoipCommon, uint32_t uUserMicrValue)
{
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonSpeaker

    \Description
        Select which peers to accept voice from

    \Input *pVoipCommon     - voip common state
    \Input uUserSpkrValue   - speaker bit values

    \Version 01/30/2019 (eesponda)
*/
/********************************************************************************F*/
void VoipCommonSpeaker(VoipCommonRefT *pVoipCommon, uint32_t uUserSpkrValue)
{
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonConnectionSharingAddSession

    \Description
        Add session id to share a specified voip connection

    \Input *pVoipCommon - voip common state
    \Input iConnId      - connection id
    \Input uSessionId   - session id we are adding

    \Output
    int32_t         - zero=success, negative=failure

    \Version 01/30/2019 (eesponda)
*/
/********************************************************************************F*/
int32_t VoipCommonConnectionSharingAddSession(VoipCommonRefT *pVoipCommon, int32_t iConnId, uint32_t uSessionId)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonConnectionSharingDelSession

    \Description
    Remove session id from sharing a specified voip connection

    \Input *pVoipCommon - voip common state
    \Input iConnId      - connection id
    \Input uSessionId   - session id we are removing

    \Output
    int32_t         - zero=success, negative=failure

    \Version 01/30/2019 (eesponda)
*/
/********************************************************************************F*/
int32_t VoipCommonConnectionSharingDelSession(VoipCommonRefT* pVoipCommon, int32_t iConnId, uint32_t uSessionId)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonMapVoipServerId

    \Description
        For server-based voip, maps a local conn id to a voipserver conn id

    \Input *pVoipCommon         - voip common state
    \Input iLocalConnId         - local connection id
    \Input iVoipServerConnId    - voipserver connection id

    \Output
    int32_t                 - zero=success, negative=failure

    \Version 01/30/2019 (eesponda)
*/
/********************************************************************************F*/
int32_t VoipCommonMapVoipServerId(VoipCommonRefT *pVoipCommon, int32_t iLocalConnId, int32_t iVoipServerConnId)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonSetLocalClientId

    \Description
        Set local client id for connection

    \Input *pVoipCommon     - voip common state
    \Input iConnId          - connection id
    \Input uLocalClientId   - local client id

    \Version 01/30/2019 (eesponda)
*/
/********************************************************************************F*/
void VoipCommonSetLocalClientId(VoipCommonRefT *pVoipCommon, int32_t iConnId, uint32_t uLocalClientId)
{
}

/*F*************************************************************************************************/
/*!
    \Function VoipCommonSetDisplayTranscribedTextCallback

    \Description
    Set callback to be invoked when transcribed text (from local user or remote user)
    is ready to be displayed locally.

    \Input *pVoipCommon - voip common state
    \Input *pCallback   - notification handler
    \Input *pUserData   - user data for handler

    \Version 05/03/2017 (mclouatre)
*/
/*************************************************************************************************F*/
void VoipCommonSetDisplayTranscribedTextCallback(VoipCommonRefT *pVoipCommon, VoipDisplayTranscribedTextCallbackT *pCallback, void *pUserData)
{
}

/*F*************************************************************************************************/
/*!
    \Function VoipCommonSetEventCallback

    \Description
        Set voip event notification handler.

    \Input *pVoipCommon - voip common state
    \Input *pCallback   - event notification handler
    \Input *pUserData   - user data for handler

    \Version 02/10/2006 (jbrookes)
*/
/*************************************************************************************************F*/
void VoipCommonSetEventCallback(VoipCommonRefT *pVoipCommon, VoipCallbackT *pCallback, void *pUserData)
{
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonConnect

    \Description
        Connect to a peer.

    \Input *pVoipCommon - voip common state
    \Input iConnID      - [zero, iMaxPeers-1] for an explicit slot or VOIP_CONNID_NONE to auto-allocate
    \Input uAddress     - remote peer address
    \Input uManglePort  - port from demangler
    \Input uGamePort    - port to connect on
    \Input uClientId    - remote clientId to connect to (cannot be 0)
    \Input uSessionId   - session identifier (optional)

    \Output
        int32_t         - connection identifier (negative=error)

    \Version 1.0 03/02/2004 (jbrookes) first version
    \Version 1.1 10/26/2009 (mclouatre) uClientId is no longer optional
*/
/********************************************************************************F*/
int32_t VoipCommonConnect(VoipCommonRefT *pVoipCommon, int32_t iConnID, uint32_t uAddress, uint32_t uManglePort, uint32_t uGamePort, uint32_t uClientId, uint32_t uSessionId)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonDisconnect

    \Description
        Disconnect from peer.

    \Input *pVoipCommon - voip common state
    \Input iConnID      - which connection to disconnect (VOIP_CONNID_ALL for all)
    \Input bSendDiscMsg - TRUE if a voip disc pkt needs to be sent, FALSE otherwise

    \Todo
        Multiple connection support.

    \Version 15/01/2014 (mclouatre)
*/
/********************************************************************************F*/
void VoipCommonDisconnect(VoipCommonRefT *pVoipCommon, int32_t iConnID, int32_t bSendDiscMsg)
{
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonConnStatus

    \Description
        Return information about peer connection.

    \Input *pVoipCommon - voip common state
    \Input  iConnID     - which connection to get remote info for, or VOIP_CONNID_ALL

    \Output
        int32_t         - VOIP_CONN* flags, or VOIP_FLAG_INVALID if iConnID is invalid

    \Version 05/06/2014 (amakoukji)
*/
/********************************************************************************F*/
int32_t VoipCommonConnStatus(VoipCommonRefT *pVoipCommon, int32_t iConnID)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonRemoteUserStatus

    \Description
        Return information about remote peer.

    \Input *pVoipCommon      - voip common state
    \Input  iConnID          - which connection to get remote info for, or VOIP_CONNID_ALL
    \Input  iRemoteUserIndex - user index at the connection iConnID

    \Output
        int32_t         - VOIP_REMOTE* flags, or VOIP_FLAG_INVALID if iConnID is invalid

    \Version 05/06/2014 (amakoukji)
*/
/********************************************************************************F*/
int32_t VoipCommonRemoteUserStatus(VoipCommonRefT *pVoipCommon, int32_t iConnID, int32_t iRemoteUserIndex)
{
    return(0);
}