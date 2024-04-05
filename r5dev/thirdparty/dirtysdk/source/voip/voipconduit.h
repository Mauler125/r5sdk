/*H********************************************************************************/
/*!
    \File voipconduit.h

    \Description
        VoIP data packet definitions.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 07/29/2004 (jbrookes) Split from voipheadset
    \Version 12/01/2009 (jbrookes) voipchannel->voipconduit; avoid name clash with new API
*/
/********************************************************************************H*/

#ifndef _voipconduit_h
#define _voipconduit_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/
//! playback callback
typedef uint8_t(VoipConduitPlaybackCbT)(VoipMixerRefT *pMixer, VoipUserT *pRemoteUser, void *pUserData);

//! conduit manager module state
typedef struct VoipConduitRefT VoipConduitRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create conduit manager
VoipConduitRefT *VoipConduitCreate(int32_t iMaxConduits);

// set conduit mixer
void VoipConduitMixerSet(VoipConduitRefT *pConduitRef, VoipMixerRefT *pMixerRef);

// unset a conduit mixer
void VoipConduitMixerUnset(VoipConduitRefT *pConduitRef, VoipMixerRefT *pMixerRef);

// destroy conduit manager
void VoipConduitDestroy(VoipConduitRefT *pConduitRef);

// receive voice data
void VoipConduitReceiveVoiceData(VoipConduitRefT *pConduitRef, VoipUserT *pRemoteUser, const uint8_t *pData, int32_t iDataSize);

// register a remote user
void VoipConduitRegisterUser(VoipConduitRefT *pConduitRef, VoipUserT *pRemoteUser, uint32_t bRegister);

// register playback callback
void VoipConduitRegisterPlaybackCb(VoipConduitRefT *pConduitRef, VoipConduitPlaybackCbT *pPlaybackCallback, void *pUserData);

// control setters
int32_t VoipConduitControl(VoipConduitRefT *pConduitRef, int32_t iControl, int32_t iValue, void *pValue);

#ifdef __cplusplus
};
#endif

#endif // _voipconduit_h

