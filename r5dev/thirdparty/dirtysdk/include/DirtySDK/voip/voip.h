/*H*************************************************************************************************/
/*!

    \File    voip.h

    \Description
        Main include for Voice Over IP module.

    \Notes
        MLU:
           Every Local User should be registered with the voip subsytem in order to get headset status info with VoipSetLocalUser().
           In order to get fully functional voip a user must be further activated which enrolls them as a participating user with VoipGroupActivateLocalUser().

           Basically any users that are being pulled into the game should be a participating user.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002-2004.    ALL RIGHTS RESERVED.

    \Version    1.0        11/19/02 (IRS) First Version
    \Version    2.0        05/13/03 (GWS) Rewrite to fix misc bugs and improve network connection.
    \Version    3.0        11/03/03 (JLB) Implemented unloadable support, major code cleanup and reorganization.
    \Version    3.5        03/02/04 (JLB) VoIP 2.0 - API changes for future multiple channel support.
    \Version    3.6        11/19/08 (mclouatre) Modified synopsis of VoipStatus()
*/
/*************************************************************************************************H*/

#ifndef _voip_h
#define _voip_h

/*!
\Moduledef VoipApi VoipApi
\Modulemember Voip
*/
//@{

/*** Include files ********************************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/voip/voipdef.h"

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// prepare voip for use
DIRTYCODE_API VoipRefT *VoipStartup(int32_t iMaxPeers, int32_t iData);

// return pointer to current module state
DIRTYCODE_API VoipRefT *VoipGetRef(void);

// release all voip resources
DIRTYCODE_API void VoipShutdown(VoipRefT *pVoip, uint32_t uShutdownFlags);

// register/unregister specified local user with the voip sub-system  (allows for local headset status query, but not voice acquisition/playback)
DIRTYCODE_API void VoipSetLocalUser(VoipRefT *pVoip, int32_t iLocalUserIndex, uint32_t bRegister);

// return information about local hardware state on a per-user basis
DIRTYCODE_API int32_t VoipLocalUserStatus(VoipRefT *pVoip, int32_t iLocalUserIndex);

// return status
DIRTYCODE_API int32_t VoipStatus(VoipRefT *pVoip, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);   //do any of the controls need user index?

// set control options
DIRTYCODE_API int32_t VoipControl(VoipRefT *pVoip, int32_t iControl, int32_t iValue, void *pValue);                 //do any of the controls need user index?

// set speaker output callback (only available on some platforms)
DIRTYCODE_API void VoipSpkrCallback(VoipRefT *pVoip, VoipSpkrCallbackT *pCallback, void *pUserData);

// pick a channel
DIRTYCODE_API int32_t VoipSelectChannel(VoipRefT *pVoip, int32_t iUserIndex, int32_t iChannel, VoipChanModeE eMode);

// go back to not using channels
DIRTYCODE_API void VoipResetChannels(VoipRefT *pVoip, int32_t iUserIndex);

// config voice transcriptions (for default config set uProfile to -1)
DIRTYCODE_API void VoipConfigTranscription(VoipRefT *pVoip, uint32_t uProfile, const char *pUrl, const char *pKey);

// config text narration (for the default config set uProvider to -1)
DIRTYCODE_API void VoipConfigNarration(VoipRefT *pVoip, uint32_t uProvider, const char *pUrl, const char *pKey);

// set first party id callback
DIRTYCODE_API void VoipRegisterFirstPartyIdCallback(VoipRefT *pVoip, VoipFirstPartyIdCallbackCbT *pCallback, void *pUserData);
#ifdef __cplusplus
}
#endif

//@}

#endif // _voip_h

