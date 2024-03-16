/*H*************************************************************************************************/
/*!

    \File    voipgroup.h

    \Description
        A module that handles logical grouping of voip connections. Each module that wants to deal
        with a block of voip connections uses a voip group. This reduces the singleton nature
        of voip.h

    \Copyright
        Copyright (c) 2007 Electronic Arts Inc.

    \Version 12/18/07 (jrainy) First Version
*/
/*************************************************************************************************H*/

#ifndef _voipgroup_h
#define _voipgroup_h

/*!
\Moduledef VoipGroup VoipGroup
\Modulemember Voip
*/
//@{

/*** Include files ********************************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/voip/voip.h"

/*** Defines **************************************************************************************/
#define VOIPGROUP_INBOUND_MUTE  (0x1)
#define VOIPGROUP_OUTBOUND_MUTE (0x2)
#define VOIPGROUP_TWOWAY_MUTE   (VOIPGROUP_INBOUND_MUTE | VOIPGROUP_OUTBOUND_MUTE)

// supported connection concierge mode  (to be used with 'ccmd' control selector)
#define VOIPGROUP_CCMODE_PEERONLY       (0)
#define VOIPGROUP_CCMODE_HOSTEDONLY     (1)
#define VOIPGROUP_CCMODE_HOSTEDFALLBACK (2)

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

//! callback event types
typedef enum ConnSharingCbTypeE
{
    VOIPGROUP_CBTYPE_CONNSUSPEND,    //<! it is time to suspend the connection with VoipGroupSuspend()
    VOIPGROUP_CBTYPE_CONNRESUME      //<! it is time to resume the connection with VoipGroupResume()
} ConnSharingCbTypeE;

//! opaque module ref
typedef struct VoipGroupRefT VoipGroupRefT;

//! event callback function prototype
typedef void (ConnSharingCallbackT)(VoipGroupRefT *pVoip, ConnSharingCbTypeE eCbType, int32_t iConnId, void *pUserData, uint8_t bSending, uint8_t bReceiving);

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// allocates a VoipGroup
DIRTYCODE_API VoipGroupRefT *VoipGroupCreate(int8_t iMaxGroups);

// deallocates a VoipGroup
DIRTYCODE_API void VoipGroupDestroy(VoipGroupRefT *pRef);

// set event notification callback
DIRTYCODE_API void VoipGroupSetEventCallback(VoipGroupRefT *pVoipGroup, VoipCallbackT *pCallback, void *pUserData);

// set callback to be invoked when some transcribed text (from a remote user or from a local user) is ready to be displayed locally
DIRTYCODE_API void VoipGroupSetDisplayTranscribedTextCallback(VoipGroupRefT *pVoipGroup, VoipDisplayTranscribedTextCallbackT *pCallback, void *pUserData);

// set connection sharing event notification callback - only use if for scenarios involving several voip groups concurrently
DIRTYCODE_API void VoipGroupSetConnSharingEventCallback(VoipGroupRefT *pVoipGroup, ConnSharingCallbackT *pCallback, void *pUserData);

// connect to a peer
DIRTYCODE_API int32_t VoipGroupConnect(VoipGroupRefT *pVoipGroup, int32_t iConnId, uint32_t uAddress, uint32_t uManglePort, uint32_t uGamePort, uint32_t uClientId, uint32_t uSessionId, uint8_t bIsConnectivityHosted, uint32_t uLowLevelConnectivityId);

// resume a connection that is in supended state at the voip group level
DIRTYCODE_API void VoipGroupResume(VoipGroupRefT *pVoipGroup, int32_t iConnId, uint32_t uAddress, uint32_t uManglePort, uint32_t uGamePort, uint32_t uClientId, uint32_t uSessionId, uint8_t bIsConnectivityHosted);

// disconnect from peer
DIRTYCODE_API void VoipGroupDisconnect(VoipGroupRefT *pVoipGroup, int32_t iConnID);

// suspend an active connection while we rely on another low-level voip connection belonging to another voip collection
DIRTYCODE_API void VoipGroupSuspend(VoipGroupRefT *pVoipGroup, int32_t iConnId);

// mute all users (2-way) or restore previous mute settings, this functionality stacks with VoipGroupMute functionality
DIRTYCODE_API void VoipGroupMuteAll(VoipGroupRefT *pVoipGroup, uint8_t bActive);

// mute/unmute a client, specified by ConnId
DIRTYCODE_API int32_t VoipGroupMuteByConnId(VoipGroupRefT *pVoipGroup, uint32_t iConnId, uint8_t uMute);

// mute/unmute a client, specified by ClientId
DIRTYCODE_API int32_t VoipGroupMuteByClientId(VoipGroupRefT *pVoipGroup, uint32_t uClientId, uint8_t uMute);

// whether a client is muted, by ConnId
DIRTYCODE_API uint8_t VoipGroupIsMutedByConnId(VoipGroupRefT *pVoipGroup, uint32_t iConnId);

// whether a client is muted, by ClientId
DIRTYCODE_API uint8_t VoipGroupIsMutedByClientId(VoipGroupRefT *pVoipGroup, uint32_t uClientId);

// mute/unmute a client, specified by ConnId and the mute flag
DIRTYCODE_API int32_t VoipGroupMuteByConnId2(VoipGroupRefT *pVoipGroup, uint32_t iConnId, uint8_t bMute, uint32_t uMuteFlag);

// mute/unmute a client, specified by ClientId, returns the mute flag
DIRTYCODE_API int32_t VoipGroupMuteByClientId2(VoipGroupRefT *pVoipGroup, uint32_t iClientId, uint8_t bMute, uint32_t uMuteFlag);

// mute/unmute a client, specified by ClientId, returns the mute flag
DIRTYCODE_API int32_t VoipGroupMuteByClientId3(uint32_t uClientId, uint8_t bMute, uint32_t uMuteFlag);

// whether a client is muted, by ConnId, returns the mute flag
DIRTYCODE_API uint32_t VoipGroupIsMutedByConnId2(VoipGroupRefT *pVoipGroup, uint32_t iConnId);

// whether a client is muted, by ClientId, returns the mute flags
DIRTYCODE_API uint32_t VoipGroupIsMutedByClientId2(VoipGroupRefT *pVoipGroup, uint32_t iClientId);

// return information about peer connection
DIRTYCODE_API int32_t VoipGroupConnStatus(VoipGroupRefT *pVoipGroup, int32_t iConnID);

// return information about remote peer
DIRTYCODE_API int32_t VoipGroupRemoteUserStatus(VoipGroupRefT *pVoipGroup, int32_t iConnID, int32_t iRemoteUserIndex);

// VoipLocalUserStatus, through a VoipGroup
DIRTYCODE_API int32_t VoipGroupLocalUserStatus(VoipGroupRefT *pVoipGroup, int32_t iLocalUserIndex);

// return status
DIRTYCODE_API int32_t VoipGroupStatus(VoipGroupRefT *pVoipGroup, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);

// set control options
DIRTYCODE_API int32_t VoipGroupControl(VoipGroupRefT *pVoipGroup, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// activate/deactivate specified local user (when activated, the user becomes a "participating" user and voice acquisition/playback is enabled for that user)
DIRTYCODE_API void VoipGroupActivateLocalUser(VoipGroupRefT *pVoipGroup, int32_t iLocalUserIndex, uint8_t bActivate);

#ifdef __cplusplus
}
#endif

//@}

#endif // _voipgroup_h

