/*H********************************************************************************/
/*!
    \File voipconduit.c

    \Description
        VoIP conduit management.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 07/29/2004 (jbrookes) Split from voipheadset.c
    \Version 12/01/2009 (jbrookes) voipchannel->voipconduit; avoid name clash with new API
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"
#include "voipconnection.h"
#include "voipmixer.h"
#include "voipdvi.h"

#include "voipconduit.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

//! name of a voip conduit
#define VOIP_ConduitName(_iConduit)         ('a' + (_iConduit))

/*** Type Definitions *************************************************************/
//! headset conduit info
typedef struct VoipConduitT
{
    VoipUserT   PacketUser;
} VoipConduitT;

//! conduit module state
struct VoipConduitRefT
{
    VoipMixerRefT       *apMixers[VOIP_MAXLOCALUSERS];
    VoipConduitPlaybackCbT *pConduitPlayback;
    void                *pUserData;
    int32_t             iNumConduits;
    int32_t             iVerbosity;
    VoipConduitT        Conduits[1];
};


/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Public Variables

// Private Variables

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipConduitCreate

    \Description
        Create voice conduit manager.

    \Input iMaxConduits     - number of conduits to support

    \Output
        VoipConduitRefT *   - pointer to module state, or NULL if an error occurred

    \Version 07/29/2004 (jbrookes) Split from voipheadset.c
*/
/********************************************************************************F*/
VoipConduitRefT *VoipConduitCreate(int32_t iMaxConduits)
{
    VoipConduitRefT *pConduitRef;
    int32_t iRefSize;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // calculate size
    iRefSize = sizeof(VoipConduitRefT) + (sizeof(VoipConduitT) * (iMaxConduits-1));

    // allocate and clear module state
    VoipCommonMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    if ((pConduitRef = DirtyMemAlloc(iRefSize, VOIP_MEMID, iMemGroup, pMemGroupUserData )) == NULL)
    {
        return(NULL);
    }
    ds_memclr(pConduitRef, iRefSize);

    // init ref & return to caller
    pConduitRef->iNumConduits = iMaxConduits;
    return(pConduitRef);
}

/*F********************************************************************************/
/*!
    \Function VoipConduitMixerSet

    \Description
        Assign a mixer to the conduit

    \Input *pConduitRef     - pointer to conduit manager
    \Input *pMixerRef       - pointer to mixer to assign

    \Version 10/26/2011 (jbrookes)
*/
/********************************************************************************F*/
void VoipConduitMixerSet(VoipConduitRefT *pConduitRef, VoipMixerRefT *pMixerRef)
{
    int32_t iMixerIndex;

    // find an empty mixer in the array
    for (iMixerIndex = 0; iMixerIndex < VOIP_MAXLOCALUSERS; ++iMixerIndex)
    {
        if (pConduitRef->apMixers[iMixerIndex] == NULL)
        {
            pConduitRef->apMixers[iMixerIndex] = pMixerRef;
            break;
        }
    }

    if (iMixerIndex == VOIP_MAXLOCALUSERS)
    {
        NetPrintf(("voipconduit: VoipConduitMixerSet() cannot add mixer %p!\n", pMixerRef));
    }
}

/*F********************************************************************************/
/*!
    \Function VoipConduitMixerUnset

    \Description
        Unassign a mixer to the conduit

    \Input *pConduitRef     - pointer to conduit manager
    \Input *pMixerRef       - pointer to mixer to unassign

    \Version 03/12/2019 (tcho)
*/
/********************************************************************************F*/
void VoipConduitMixerUnset(VoipConduitRefT *pConduitRef, VoipMixerRefT *pMixerRef)
{
    int32_t iMixerIndex;

    // find an empty mixer in the array
    for (iMixerIndex = 0; iMixerIndex < VOIP_MAXLOCALUSERS; ++iMixerIndex)
    {
        if (pConduitRef->apMixers[iMixerIndex] == pMixerRef)
        {
            pConduitRef->apMixers[iMixerIndex] = NULL;
            break;
        }
    }

    if (iMixerIndex == VOIP_MAXLOCALUSERS)
    {
        NetPrintf(("voipconduit: VoipConduitMixerUnset() cannot find mixer %p!\n", pMixerRef));
    }
}

/*F********************************************************************************/
/*!
    \Function   VoipConduitDestroy

    \Description
        Destroy voice conduit manager.

    \Input *pConduitRef     - pointer to conduit manager

    \Version 07/29/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipConduitDestroy(VoipConduitRefT *pConduitRef)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    VoipCommonMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    DirtyMemFree(pConduitRef, VOIP_MEMID, iMemGroup, pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function VoipConduitReceiveVoiceData

    \Description
        Receive voice data, and send it to the mixer.

    \Input *pConduitRef     - pointer to conduit manager
    \Input *pRemoteUser     - user data came from
    \Input *pData           - incoming voice data packet
    \Input iDataSize        - voice data size

    \Version 03/21/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipConduitReceiveVoiceData(VoipConduitRefT *pConduitRef, VoipUserT *pRemoteUser, const uint8_t *pData, int32_t iDataSize)
{
    VoipConduitT *pConduit;
    int32_t iConduit;
    int32_t iMixerIndex;

    // copy to matching slot in queue
    for (iConduit = 0; iConduit < pConduitRef->iNumConduits; iConduit++)
    {
        pConduit = &pConduitRef->Conduits[iConduit];
        if (VOIP_SameUser(&pConduit->PacketUser, pRemoteUser))
        {
            int32_t iConduitMask = 1 << iConduit;

            for (iMixerIndex = 0; iMixerIndex < VOIP_MAXLOCALUSERS; ++iMixerIndex)
            {
                // decompress the frame and accumulate to the current mixbuffer
                if (pConduitRef->apMixers[iMixerIndex] != NULL)
                {
                    uint8_t bPlayback = TRUE;

                    if (pConduitRef->pConduitPlayback != NULL)
                    {
                        bPlayback = pConduitRef->pConduitPlayback(pConduitRef->apMixers[iMixerIndex], pRemoteUser, pConduitRef->pUserData);
                    }
                    
                    if (bPlayback)
                    {
                        if (VoipMixerAccumulate(pConduitRef->apMixers[iMixerIndex], (uint8_t *)pData, iDataSize, iConduitMask, iConduit) < 0)
                        {
                            NetPrintfVerbose((pConduitRef->iVerbosity, 3, "voipconduit: [%c] index[%d], discarding packet due to mixbuffer overflow\n", VOIP_ConduitName(iConduit), iMixerIndex));
                        }
                    }
                }
            }
            break;
        }
    }

    #if DIRTYCODE_LOGGING
    if (iConduit == pConduitRef->iNumConduits)
    {
        NetPrintf(("voipconduit: could not find a conduit for voice packet from user '%lld'\n", pRemoteUser->AccountInfo.iPersonaId));
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function VoipConduitRegisterUser

    \Description
        Register/unregister a remote user with the conduit manager.

    \Input *pConduitRef     - pointer to conduit manager
    \Input *pRemoteUser     - remote user
    \Input bRegister        - if TRUE, register user, else unregister user.

    \Version 03/21/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipConduitRegisterUser(VoipConduitRefT *pConduitRef, VoipUserT *pRemoteUser, uint32_t bRegister)
{
    VoipUserT *pConduitUser;
    int32_t iConduit;

    // find an open slot
    for (iConduit = 0; iConduit < pConduitRef->iNumConduits; iConduit++)
    {
        // ref conduit user
        pConduitUser = &pConduitRef->Conduits[iConduit].PacketUser;

        if (bRegister)
        {
            if (VOIP_NullUser(pConduitUser))
            {
                NetPrintf(("voipconduit: [%c] registering user '%lld'\n", VOIP_ConduitName(iConduit), pRemoteUser->AccountInfo.iPersonaId));
                VOIP_CopyUser(pConduitUser, pRemoteUser);
                break;
            }
        }
        else
        {
            if (VOIP_SameUser(pConduitUser, pRemoteUser))
            {
                NetPrintf(("voipconduit: [%c] unregistering user '%lld'\n", VOIP_ConduitName(iConduit), pRemoteUser->AccountInfo.iPersonaId));
                VOIP_ClearUser(pConduitUser);
                break;
            }
        }
    }

    #if DIRTYCODE_LOGGING
    if (iConduit >= pConduitRef->iNumConduits)
    {
        NetPrintf(("voipconduit: could not %s user '%lld'\n",
            (bRegister) ? "register" : "unregister",
            pRemoteUser->AccountInfo.iPersonaId));
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function VoipConduitRegisterPlaybackCb

    \Description
        Register/unregister a remote user with the conduit manager.

    \Input *pConduitRef           - pointer to conduit manager
    \Input *pPlaybackCallback     - playback callback
    \Input *pUserData             - callback user data

    \Version 06/24/2019 (tcho)
*/
/********************************************************************************F*/
void VoipConduitRegisterPlaybackCb(VoipConduitRefT *pConduitRef, VoipConduitPlaybackCbT *pPlaybackCallback, void *pUserData)
{
    pConduitRef->pConduitPlayback = pPlaybackCallback;
    pConduitRef->pUserData = pUserData;
}


/*F********************************************************************************/
/*!
    \Function VoipConduitControl

    \Description
        Control function.

    \Input *pConduitRef   - pointer to conduit manager
    \Input iControl       - control selector
    \Input iValue         - control value
    \Input *pValue        - control value

    \Output
        int32_t         - selector specific, or -1 if no such selector

    \Notes
        iControl can be one of the following:

    \verbatim
        'spam' - set the debug level
        'rmcb' - register playback callback
        'rmcu' - register playback call back user data
    \endverbatim

    \Version 06/03/2016 (amakoukji)
*/
/********************************************************************************F*/
int32_t VoipConduitControl(VoipConduitRefT *pConduitRef, int32_t iControl, int32_t iValue, void *pValue)
{
    int32_t iReturn = -1;
    if (iControl == 'spam')
    {
        pConduitRef->iVerbosity = iValue;
        iReturn = 0;
    }
    return(iReturn);
}
