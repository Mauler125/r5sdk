/*H********************************************************************************/
/*!
    \File voipcommon.h

    \Description
        Cross-platform voip data types and private functions.

    \Copyright
        Copyright (c) 2009 Electronic Arts Inc.

    \Version 12/02/2009 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _voipcommon_h
#define _voipcommon_h

/*** Include files ****************************************************************/

#include "voipconnection.h"
#include "voipheadset.h"
#include "DirtySDK/voip/voip.h"
#include "DirtySDK/voip/voipblocklist.h"


/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/


typedef struct VoipCommonRefT
{
    VoipConnectionlistT Connectionlist;     //!< virtual connection list
    VoipHeadsetRefT     *pHeadset;          //!< voip headset manager
    VoipBlockListT      *pBlockList;        //!< muting based on persona id

    VoipCallbackT       *pCallback;         //!< voip event callback (optional)
    void                *pUserData;         //!< user data for voip event callback

    VoipDisplayTranscribedTextCallbackT *pDisplayTranscribedTextCb; //!< callback invoked when some transcribed text (from a remote user or from a local user) is ready to be displayed locally (optional)
    void                *pDisplayTranscribedTextUserData; //!< user data associated with callback invoked when some transcribed text (from a remote user or from a local user) is ready to be displayed locally (optional)

    NetCritT            ThreadCrit;         //!< critical section used for thread synchronization
    uint8_t             bApplyChannelConfig; //!< apply channel configs on the next voip thread pass
    
    uint32_t            uLastFrom;          //!< most recent from status, used for callback tracking
    uint32_t            uLastHsetStatus;    //!< most recent headset status, used for callback tracking
    uint32_t            uLastLocalStatus[VOIP_MAXLOCALUSERS];   //!< most recent local status, used for callback tracking
    uint32_t            uPortHeadsetStatus; //!< bitfield indicating which ports have headsets
    uint32_t            uLastTtsStatus;     //!< most recent TTS status, used for callback tracking
    uint32_t            uTtsStatus;         //!< current TTS status (bitmask of local user TTS flag)
    uint32_t            uUserSendMask;      //!< current sending status (bitmask of local user send flag)
    
    uint8_t             bUseDefaultSharedChannelConfig; //!< whether we use the default shared config or not (only supported on xbox one)
    uint8_t             bPrivileged;       //!< whether we are communication-privileged or not
    uint8_t             bTextTranscriptionEnabled; //!< whether text transcription is enabled or not locally
    uint8_t             _pad[1];

    uint32_t            uRemoteChannelSelection[VOIP_MAX_LOW_LEVEL_CONNS][VOIP_MAXLOCALUSERS_EXTENDED]; //<! coded, per remote user, channel selection
    uint32_t            uLocalChannelSelection[VOIP_MAXLOCALUSERS_EXTENDED];   //<! coded, per local user, channel selection

    int32_t             iDefaultSharedChannels[VOIP_MAX_CONCURRENT_CHANNEL];   //<! use to store the default shared channel config
    uint32_t            uDefaultSharedModes[VOIP_MAX_CONCURRENT_CHANNEL];      //<! use to store the default shared channel mode
    int32_t             iCustomSharedChannels[VOIP_MAX_CONCURRENT_CHANNEL];    //<! use to store the custom shared channel config
    uint32_t            uCustomSharedModes[VOIP_MAX_CONCURRENT_CHANNEL];       //<! use to store the custom shared channel mode
    int32_t             iLocalChannels[VOIP_MAXLOCALUSERS_EXTENDED][VOIP_MAX_CONCURRENT_CHANNEL];         //<! channels selected locally
    uint32_t            uLocalModes[VOIP_MAXLOCALUSERS_EXTENDED][VOIP_MAX_CONCURRENT_CHANNEL];            //<! channel modes selected locally

    uint32_t            uChanRecvMask;  //<! the recv mask associated with the channel selection
    uint32_t            uChanSendMask;  //<! the send mask associated with the channel selection

    uint32_t            uUserSpkrValue;
    uint32_t            uUserMicrValue;
} VoipCommonRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// get voip ref
VoipRefT *VoipCommonGetRef(void);

// handle common startup functionality
VoipRefT *VoipCommonStartup(int32_t iMaxPeers, int32_t iVoipRefSize, VoipHeadsetStatusCbT *pStatusCb, int32_t iData);

// handle common shutdown functionality
void VoipCommonShutdown(VoipCommonRefT *pVoipCommon);

// update status flags
void VoipCommonUpdateRemoteStatus(VoipCommonRefT *pVoipCommon);

// activate/deactivate specified local user (when activated, the user becomes a "participating" user and voice acquisition/playback is enabled for that user)
void VoipCommonActivateLocalUser(VoipCommonRefT *pVoipCommon, int32_t iLocalUserIndex, uint8_t bActivate);

// get current memgroup
void VoipCommonMemGroupQuery(int32_t *pMemGroup, void **ppMemGroupUserData);

// set current memgroup
void VoipCommonMemGroupSet(int32_t iMemGroup, void *pMemGroupUserData);

// handle common status selectors
int32_t VoipCommonStatus(VoipCommonRefT *pVoipCommon, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);

// handle common control selectors
int32_t VoipCommonControl(VoipCommonRefT *pVoipCommon, int32_t iControl, int32_t iValue, void *pValue);

// add (OR) uAddMask into *pMask
void VoipCommonAddMask(uint32_t *pMask, uint32_t uAddMask, const char *pMaskName);

// del (&~) uDelMask from *pMask
void VoipCommonDelMask(uint32_t *pMask, uint32_t uDelMask, const char *pMaskName);

// set *pMask = uNewMask
void VoipCommonSetMask(uint32_t *pMask, uint32_t uNewMask, const char *pMaskName);

// select which peers to send voice to
void VoipCommonMicrophone(VoipCommonRefT *pVoipCommon, uint32_t uUserMicrValue);

// select which peers to accept voice from
void VoipCommonSpeaker(VoipCommonRefT *pVoipCommon, uint32_t uUserSpkrValue);

// add session id for connection sharing
int32_t VoipCommonConnectionSharingAddSession(VoipCommonRefT *pVoipCommon, int32_t iConnId, uint32_t uSessionId);

// remove session id for connection sharing
int32_t VoipCommonConnectionSharingDelSession(VoipCommonRefT* pVoipCommon, int32_t iConnId, uint32_t uSessionId);

// for voip via server, maps a local conn id to a voip server conn id
int32_t VoipCommonMapVoipServerId(VoipCommonRefT *pVoipCommon, int32_t iLocalConnId, int32_t iVoipServerConnId);

// set local client id for conn id
void VoipCommonSetLocalClientId(VoipCommonRefT *pVoipCommon, int32_t iConnId, uint32_t uLocalClientId);

// set callback to be invoked when some transcribed text (from a remote user or from a local user) is ready to be displayed locally
void VoipCommonSetDisplayTranscribedTextCallback(VoipCommonRefT *pVoipCommon, VoipDisplayTranscribedTextCallbackT *pCallback, void *pUserData);

// set event notification callback
void VoipCommonSetEventCallback(VoipCommonRefT *pVoipCommon, VoipCallbackT *pCallback, void *pUserData);

// connect to a peer
int32_t VoipCommonConnect(VoipCommonRefT *pVoipCommon, int32_t iConnID, uint32_t uAddress, uint32_t uManglePort, uint32_t uGamePort, uint32_t uClientId, uint32_t uSessionId);

// disconnect from peer
void VoipCommonDisconnect(VoipCommonRefT *pVoipCommon, int32_t iConnID, int32_t bSendDiscMsg);

// return information about peer connection
int32_t VoipCommonConnStatus(VoipCommonRefT *pVoipCommon, int32_t iConnID);

// return information about remote peer
int32_t VoipCommonRemoteUserStatus(VoipCommonRefT *pVoipCommon, int32_t iConnID, int32_t iRemoteUserIndex);

int32_t VoipCommonSelectChannel(VoipCommonRefT *pVoip, int32_t iUserIndex, int32_t iChannel, VoipChanModeE eMode);

void VoipCommonResetChannels(VoipCommonRefT *pVoip, int32_t iUserIndex);

void VoipCommonApplyChannelConfig(VoipCommonRefT *pVoip);

void VoipCommonProcessChannelChange(VoipCommonRefT *pVoipCommon, int32_t iConnID);

#ifdef __cplusplus
}
#endif

#endif // _voipcommon_h

