/*H********************************************************************************/
/*!
    \File voipheadset.h

    \Description
        VoIP headset manager.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 03/31/2004 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _voipheadset_h
#define _voipheadset_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! voip headset update status
typedef enum VoipHeadsetStatusUpdateE
{
    VOIP_HEADSET_STATUS_INPUT = 1,
    VOIP_HEADSET_STATUS_OUTPUT,
    VOIP_HEADSET_STATUS_INOUT
} VoipHeadsetStatusUpdateE;

//! opaque headset module ref
typedef struct VoipHeadsetRefT VoipHeadsetRefT;

//! mic data ready callback function prototype
typedef void (VoipHeadsetMicDataCbT)(const void *pVoiceData, int32_t iDataSize, const void *pMetaData, int32_t iMetaDataSize, uint32_t uLocalUserIndex, uint8_t uSendSeqn, void *pUserData);

//! locally generated transcribed text ready callback function prototype
typedef void (VoipHeadsetTextDataCbT)(const char *pTranscribedTextUtf8, uint32_t uLocalUserIndex, void *pUserData);

#if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
//! remote-generated transcribed text received callback function prototype
typedef void (VoipHeadsetTranscribedTextReceivedCbT)(VoipUserT *pUser, const char *pText, void *pUserData);
#endif

//! opaque data ready callback function prototype
typedef void (VoipHeadsetOpaqueDataCbT)(const uint8_t *pOpaqueData, int32_t iOpaqueDataSize, uint32_t uSendMask, uint8_t bReliable, uint8_t uSendSeqn, void *pUserData);

#if defined(DIRTYCODE_PS4) 
//! remote-generated transcribed text received callback function prototype
typedef void (VoipHeadsetTranscribedTextReceivedCbT)(const VoipUserT *pUser, const char *pText, void *pUserData);
#endif

//! headset status change callback
typedef void (VoipHeadsetStatusCbT)(int32_t iLocalUserIndex, uint32_t bStatus, VoipHeadsetStatusUpdateE eUpdate, void *pUserData);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create headset module
VoipHeadsetRefT *VoipHeadsetCreate(int32_t iMaxChannels, VoipHeadsetMicDataCbT *pMicDataCb, VoipHeadsetTextDataCbT *pTextDataCb, VoipHeadsetOpaqueDataCbT *pOpaqueDataCb, VoipHeadsetStatusCbT *pStatusCb, void *pCbUserData, int32_t iData);

// destroy headset module
void VoipHeadsetDestroy(VoipHeadsetRefT *pHeadset);

// callback to handle receiving voice data
void VoipHeadsetReceiveVoiceDataCb(VoipUserT *pRemoteUsers, int32_t iRemoteUserSize, int32_t iConsoleId, VoipMicrInfoT *pMicrInfo, uint8_t *pPacketData, void *pUserData);

// callback to handle receiving opaque data
void VoipHeadsetReceiveOpaqueDataCb(int32_t iConsoleId, const uint8_t *pOpaqueData, int32_t iOpaqueDataSize, void *pUserData);

// callback to handle registering of a new user
void VoipHeadsetRegisterUserCb(VoipUserT *pRemoteUser, int32_t iConsoleID, uint32_t bRegister, void *pUserData);

// process function to update headset(s)
void VoipHeadsetProcess(VoipHeadsetRefT *pHeadset, uint32_t uFrameCount);

// set play/rec volumes (-1 to keep current setting)
void VoipHeadsetSetVolume(VoipHeadsetRefT *pHeadset, int32_t iPlayVol, uint32_t iRecVol);

// control function
int32_t VoipHeadsetControl(VoipHeadsetRefT *pHeadset, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// status function
int32_t VoipHeadsetStatus(VoipHeadsetRefT *pHeadset, int32_t iSelect, int32_t iData, void *pBuf, int32_t iBufSize);

// set speaker output callback (only available on some platforms)
void VoipHeadsetSpkrCallback(VoipHeadsetRefT *pHeadset, VoipSpkrCallbackT *pCallback, void *pUserData);

// configure transcribe
void VoipHeadsetConfigTranscription(VoipHeadsetRefT *pHeadset, uint32_t uProfile, const char *pUrl, const char *pKey);

// configure narrate
void VoipHeadsetConfigNarration(VoipHeadsetRefT *pHeadset, uint32_t uProvider, const char *pUrl, const char *pKey);

#if defined(DIRTYCODE_PS4)
// validate communication permissions before display text from a remote user
void VoipHeadsetTranscribedTextPermissionCheck(VoipHeadsetRefT *pHeadset, VoipUserT *pOriginator, const char *pStrUtf8);
#endif

#if defined(DIRTYCODE_XBOXONE)|| defined(DIRTYCODE_PS4) || defined(DIRTYCODE_GDK)
// set callback to be invoked by voipheadsetxboxone when inbound transcribed text has passed communication permission check
void VoipHeadsetSetTranscribedTextReceivedCallback(VoipHeadsetRefT *pHeadset, VoipHeadsetTranscribedTextReceivedCbT *pCallback, void *pUserData);
#endif

// callback to tell dirtysdk what the first party id is for this user
void VoipHeadsetSetFirstPartyIdCallback(VoipHeadsetRefT *pHeadset, VoipFirstPartyIdCallbackCbT *pPlaybackCallback, void *pUserData);

#ifdef __cplusplus
};
#endif

#endif // _voipheadset_h

