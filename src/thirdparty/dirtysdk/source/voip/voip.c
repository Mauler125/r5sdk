/*H********************************************************************************/
/*!
    \File voip.c

    \Description
        Cross-platform public voip functions.

    \Copyright
        Copyright (c) 2009 Electronic Arts Inc.

    \Version 12/02/2009 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#ifdef _XBOX
#include <xtl.h>
#include <xonline.h>
#endif

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"
#include "DirtySDK/voip/voip.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function   VoipLocalUserStatus

    \Description
        Return information about local hardware state

    \Input *pVoip           - module state from VoipStartup
    \Input  iLocalUserIndex - local index of the user
    
    \Output
        int32_t         - VOIP_LOCAL* flags
        
    \Version 1.0 03/02/2004 (jbrookes) First Version
    \Version 2.0 04/25/2014 (amakoukji) Refactored for MLU
*/
/********************************************************************************F*/
int32_t VoipLocalUserStatus(VoipRefT *pVoip, int32_t iLocalUserIndex)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;
    uint32_t uLocalStatus = pVoipCommon->Connectionlist.uLocalUserStatus[iLocalUserIndex];
    
    return(uLocalStatus);
}

/*F********************************************************************************/
/*!
    \Function   VoipGetRef

    \Description
        Return current module reference.

    \Output
        VoipRefT *      - reference pointer, or NULL if the module is not active

    \Version 1.0 03/08/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
VoipRefT *VoipGetRef(void)
{
    return(VoipCommonGetRef());
}

/*F*************************************************************************************************/
/*!
    \Function VoipSpkrCallback
    
    \Description
        Set speaker data callback (not supported on all platforms).
            
    \Input *pVoip       - voip module state
    \Input *pCallback   - data callback
    \Input *pUserData   - user data
    
    \Version 12/12/2005 (jbrookes)
*/
/*************************************************************************************************F*/
void VoipSpkrCallback(VoipRefT *pVoip, VoipSpkrCallbackT *pCallback, void *pUserData)
{
    #if defined(DIRTYCODE_PC) || defined(DIRTYCODE_STADIA)
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;
    VoipHeadsetSpkrCallback(pVoipCommon->pHeadset, pCallback, pUserData);
    #endif
}

/*F********************************************************************************/
/*!
    \Function   VoipSelectChannel

    \Description
        Select the mode(send/recv) of a given channel.

    \Input *pVoip       - voip module state
    \Input iUserIndex   - local user index
    \Input iChannel     - Channel ID (valid range: [0,63])
    \Input eMode        - The mode, combination of VOIP_CHANSEND, VOIP_CHANRECV

    \Output
        int32_t         - number of channels remaining that this console could join

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
int32_t VoipSelectChannel(VoipRefT *pVoip, int32_t iUserIndex, int32_t iChannel, VoipChanModeE eMode)
{
    return(VoipCommonSelectChannel((VoipCommonRefT*)pVoip, iUserIndex, iChannel, eMode));
}

/*F********************************************************************************/
/*!
    \Function   VoipResetChannels

    \Description
        Resets the channels selection to defaults. Sends and receives to all

    \Input *pVoip           - module ref
    \Input iUserIndex       - local user index

    \Version 12/07/2009 (jrainy)
*/
/********************************************************************************F*/
void VoipResetChannels(VoipRefT *pVoip, int32_t iUserIndex)
{
    VoipCommonResetChannels((VoipCommonRefT*)pVoip, iUserIndex);
}

/*F********************************************************************************/
/*!
    \Function   VoipConfigTranscription

    \Description
        Configure voice transcription

    \Input *pVoip       - module ref
    \Input uProfile     - transcribe profile
    \Input *pUrl        - transcribe provider url
    \Input *pKey        - transcribe key

    \Version 11/06/2018 (tcho)
*/
/********************************************************************************F*/
void VoipConfigTranscription(VoipRefT *pVoip, uint32_t uProfile, const char *pUrl, const char *pKey)
{
    #if defined(DIRTYCODE_PC) || defined(DIRTYCODE_PS4) || defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK) || defined(DIRTYCODE_STADIA)
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;
    VoipHeadsetConfigTranscription(pVoipCommon->pHeadset, uProfile, pUrl, pKey);
    #endif
}

/*F********************************************************************************/
/*!
    \Function   VoipConfigNarration

    \Description
        Configure voice transcription

    \Input *pVoip       - module ref
    \Input uProvider    - narration profile
    \Input *pUrl        - narration provider url
    \Input *pKey        - narration key

    \Version 11/06/2018 (tcho)
*/
/********************************************************************************F*/
void VoipConfigNarration(VoipRefT *pVoip, uint32_t uProvider, const char *pUrl, const char *pKey)
{
    #if defined(DIRTYCODE_PS4) || defined(DIRTYCODE_STADIA)
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;
    VoipHeadsetConfigNarration(pVoipCommon->pHeadset, uProvider, pUrl, pKey);
    #endif
}

/*F********************************************************************************/
/*!
    \Function   VoipRegisterFirstPartyIdCallback

    \Description
        Register first party id callback

    \Input *pVoip       - module ref 
    \Input *pCallback   - first party id callback
    \Input *pUserData   - callback user data

    \Version 05/08/2019 (tcho)
*/
/********************************************************************************F*/
void VoipRegisterFirstPartyIdCallback(VoipRefT *pVoip, VoipFirstPartyIdCallbackCbT *pCallback, void *pUserData)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;
    VoipHeadsetSetFirstPartyIdCallback(pVoipCommon->pHeadset, pCallback, pUserData);
}
