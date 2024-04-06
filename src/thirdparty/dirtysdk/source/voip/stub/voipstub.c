/*H********************************************************************************/
/*!
    \File voipstub.c

    \Description
        Voip library interface.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 0.5 03/02/2004 (jbrookes)  Implemented stubbed interface.
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/voip/voip.h"
#include "DirtySDK/voip/voipcodec.h"
#include "voippriv.h"
#include "voipcommon.h"

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! VoIP module state
struct VoipRefT
{
    //! module memory group
    int32_t         iMemGroup;
    void            *pMemGroupUserData;
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private Variables

//! pointer to module state
static VoipRefT             *_Voip_pRef = NULL;

// Public Variables


/*** Private Functions ************************************************************/


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function   VoipStartup

    \Description
        Prepare VoIP for use.

    \Input iMaxPeers    - maximum number of peers supported (up to 32)
    \Input iData        - platform-specific - unused for voipstub

    \Output
        VoipRefT        - state reference that is passed to all other functions

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
VoipRefT *VoipStartup(int32_t iMaxPeers, int32_t iData)
{
    VoipRefT *pVoip;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // make sure we're not already started
    if (_Voip_pRef != NULL)
    {
        NetPrintf(("voip: module startup called when not in a shutdown state\n"));
        return(NULL);
    }

    // create and initialize module state
    if ((pVoip = (VoipRefT *)DirtyMemAlloc(sizeof(*pVoip), VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voip: unable to allocate module state\n"));
        return(NULL);
    }
    pVoip->iMemGroup = iMemGroup;
    pVoip->pMemGroupUserData = pMemGroupUserData;

    // save ref and return to caller
    _Voip_pRef = pVoip;
    return(pVoip);
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
    // return pointer to module state
    return(_Voip_pRef);
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
    if (_Voip_pRef == NULL)
    {
        NetPrintf(("voip: module shutdown called when not in a started state\n"));
        return;
    }

    // free module memory
    DirtyMemFree(pVoip, VOIP_MEMID, pVoip->iMemGroup, pVoip->pMemGroupUserData);

    // clear pointer to module state
    _Voip_pRef = NULL;
}


/*F********************************************************************************/
/*!
    \Function   VoipSetLocalUser

    \Description
        Register/unregister specified local user with the voip sub-system.

    \Input *pVoip           - module state from VoipStartup
    \Input iLocalUserIndex  - local user index (ignored because MLU not supported on PC)
    \Input bRegister        - ignored

    \Version 04/15/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipSetLocalUser(VoipRefT *pVoip, int32_t iLocalUserIndex, uint32_t bRegister)
{

}

/*F********************************************************************************/
/*!
    \Function   VoipCommonActivateLocalUser

    \Description
        Promote/demote specified local user to/from "participating" state

    \Input *pVoipCommon     - voip common state
    \Input iLocalUserIndex  - local user index (ignored because MLU not supported on PC)
    \Input bActivate        - TRUE to activate, FALSE to deactivate

    \Version 04/25/2013 (tcho)
*/
/********************************************************************************F*/
void VoipCommonActivateLocalUser(VoipCommonRefT *pVoipCommon, int32_t iLocalUserIndex, uint8_t bActivate)
{

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

    \Version 1.0 03/02/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipStatus(VoipRefT *pVoip, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'avlb')
    {
        return(TRUE);
    }
    if (iSelect == 'from')
    {
        return(0);
    }
    if (iSelect == 'micr')
    {
        return(0);
    }
    if (iSelect == 'sock')
    {
        return(0);
    }
    if (iSelect == 'spkr')
    {
        return(0);
    }
    // unsupported selector
    return(-1);
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

    \Output
        int32_t         - selector-specific data

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t VoipControl(VoipRefT *pVoip, int32_t iControl, int32_t iValue, void *pValue)
{
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function VoipSpkrCallback

    \Description
        Set speaker data callback.

    \Input *pVoip       - voip module state
    \Input *pCallback   - data callback
    \Input *pUserData   - user data

    \Version 1.0 12/12/2005 (jbrookes) First Version
*/
/*************************************************************************************************F*/
void VoipSpkrCallback(VoipRefT *pVoip, VoipSpkrCallbackT *pCallback, void *pUserData)
{
}

/*F*************************************************************************************************/
/*!
    \Function VoipCodecControl

    \Description
        Stub for codec control

    \Input iCodecIdent - codec identifier, or VOIP_CODEC_ACTIVE
    \Input iControl - control selector
    \Input iValue   - selector specific
    \Input iValue2  - selector specific
    \Input *pValue  - selector specific

    \Output
        int32_t     - selector specific

    \Version 1.0 10/11/2011 (jbrookes) First Version
*/
/*************************************************************************************************F*/
int32_t VoipCodecControl(int32_t iCodecIdent, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function   VoipSelectChannel

    \Description
        Stub for VoipSelectChannel

    \Input *pVoip       - voip module state
    \Input iUserIndex   - local user index
    \Input iChannel     - Channel ID (valid range: [0,63])
    \Input eMode        - The mode, combination of VOIPGROUP_CHANSEND, VOIPGROUP_CHANRECV

    \Output
        int32_t         - number of channels remaining that this console could join

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
int32_t VoipSelectChannel(VoipRefT *pVoip, int32_t iUserIndex, int32_t iChannel, VoipChanModeE eMode)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function   VoipResetChannels

    \Description
        Stub for VoipResetChannels

    \Input *pVoip           - module ref
    \Input iUserIndex       - local user index

    \Version 12/07/2009 (jrainy)
*/
/********************************************************************************F*/
void VoipResetChannels(VoipRefT *pVoip, int32_t iUserIndex)
{
}

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
    return(0);
}

/*F********************************************************************************/
/*!
    \Function   VoipBlockListCreate

    \Description
        Creates the VoipBlockListT.

    \Output
        VoipBlockListT * - The Blocked List state

    \Version 05/02/2019 (cvienneau)
*/
/********************************************************************************F*/
VoipBlockListT *VoipBlockListCreate(void)
{
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function   VoipBlockListDestroy

    \Description
        Free the voip block list

    \Input pVoip                - voip state

    \Version 05/07/2019 (cvienneau)
*/
/********************************************************************************F*/
void VoipBlockListDestroy(VoipRefT *pVoip)
{
}

/*F********************************************************************************/
/*!
    \Function   VoipBlockListAdd

    \Description
        Add a user to be blocked by the local user

    \Input pVoip                - voip state
    \Input iLocalUserIndex      - the index of the local user
    \Input iBlockedAccountId    - the account id of the user to be blocked

    \Output
        uint8_t         - TRUE if the user was successfully blocked

    \Version 05/07/2019 (cvienneau)
*/
/********************************************************************************F*/
uint8_t VoipBlockListAdd(VoipRefT *pVoip, int32_t iLocalUserIndex, int64_t iBlockedAccountId)
{
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function   VoipBlockListRemove

    \Description
        Remove a user that was blocked by the local user

    \Input pVoip                - voip state
    \Input iLocalUserIndex      - the index of the local user
    \Input iBlockedAccountId    - the account id of the user who was blocked

    \Output
        uint8_t         - TRUE if the user was successfully un-blocked

    \Version 05/07/2019 (cvienneau)
*/
/********************************************************************************F*/
uint8_t VoipBlockListRemove(VoipRefT *pVoip, int32_t iLocalUserIndex, int64_t iBlockedAccountId)
{
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function   VoipBlockListIsBlocked

    \Description
        Check if a user is blocked by the local user

    \Input pVoip                - voip state
    \Input iLocalUserIndex      - the index of the local user
    \Input iBlockedAccountId    - the account id of the user who was blocked

    \Output
        uint8_t         - TRUE if the user is blocked

    \Version 05/07/2019 (cvienneau)
*/
/********************************************************************************F*/
uint8_t VoipBlockListIsBlocked(VoipRefT *pVoip, int32_t iLocalUserIndex, int64_t iBlockedAccountId)
{
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function   VoipBlockListClear

    \Description
        Clear the blocked list for the local user

    \Input pVoip                - voip state
    \Input iLocalUserIndex      - the index of the local user (-1 for all users)

    \Output
        uint8_t         - TRUE if the list was cleared

    \Version 05/07/2019 (cvienneau)
*/
/********************************************************************************F*/
uint8_t VoipBlockListClear(VoipRefT *pVoip, int32_t iLocalUserIndex)
{
    return(FALSE);
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
}