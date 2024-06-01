/*H********************************************************************************/
/*!
    \File voipcommon.c

    \Description
        Cross-platform voip data types and functions.

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
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"

#include "DirtySDK/voip/voip.h"
#include "DirtySDK/voip/voipblocklist.h"

/*** Defines **********************************************************************/
#define VOIPCOMMON_CODEDCHAN_ALL (0)            //! Special handling for codeded channel {(0,none),(0,none),(0,none),(0,none)} = talk/listen to everyone
#define VOIPCOMMON_CODEDCHAN_NONE (0x01010101)  //! Special handling for codeded channel {(1,none),(1,none),(1,none),(1,none)} = can't talk or listen to anyone

/*** Type Definitions *************************************************************/


/*** Variables ********************************************************************/

//! pointer to module state
static VoipRefT             *_Voip_pRef = NULL;

//! voip memgroup
static int32_t _Voip_iMemGroup='dflt';

//! voip memgroup userdata
static void *_Voip_pMemGroupUserData=NULL;

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
\Function    _VoipCommonTextDataCb

    \Description
        Callback to handle locally generated transcribed text.

    \Input *pStrUtf8        - pointer to text data
    \Input uLocalUserIndex  - local user index
    \Input *pUserData       - pointer to callback user data

    \Version 10/29/2018 (tcho)
*/
/********************************************************************************F*/
static void _VoipCommonTextDataCb(const char *pStrUtf8, uint32_t uLocalUserIndex, void *pUserData)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pUserData;

    VoipConnectionReliableTranscribedTextMessage(&pVoipCommon->Connectionlist, uLocalUserIndex, pStrUtf8);
    /* We also want locally generated transcribed text shown to its originator if that local user has
       also requested transcribed text from remote users. Motivation: a hearing-impaired person requesting
       transcribed text from other players also wants to see transcribed text for his own speech
       (request from EA's accessibility team). 

       Note on XBoxOne this data will come through _VoipCommonReceiveTextDataCb as it would for remote generated text.
    */
    if (pVoipCommon->Connectionlist.bTranscribedTextRequested[uLocalUserIndex] == TRUE)
    {
        if (pVoipCommon->pDisplayTranscribedTextCb != NULL)
        {
            pVoipCommon->pDisplayTranscribedTextCb(-1, uLocalUserIndex, pStrUtf8, pVoipCommon->pDisplayTranscribedTextUserData);
        }
    }
}

/*F********************************************************************************/
/*!
\Function    _VoipCommonMicDataCb

    \Description
        Callback to handle locally acquired mic data.

    \Input *pVoiceData      - pointer to mic data
    \Input iDataSize        - size of mic data
    \Input *pMetaData       - pointer to platform-specific metadata (can be NULL)
    \Input iMetaDataSize    - size of platform-specifici metadata
    \Input uLocalUserIndex  - local user index
    \Input uSendSeqn        - send sequence
    \Input *pUserData       - pointer to callback user data
    
    \Version 10/29/2018 (tcho)
*/
/********************************************************************************F*/
static void _VoipCommonMicDataCb(const void *pVoiceData, int32_t iDataSize, const void *pMetaData, int32_t iMetaDataSize, uint32_t uLocalUserIndex, uint8_t uSendSeqn, void *pUserData)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pUserData;

    if (pVoipCommon != NULL)
    {
        VoipConnectionSend(&pVoipCommon->Connectionlist, 0xFFFFFFFF, (uint8_t *)pVoiceData, iDataSize, (uint8_t *)pMetaData, iMetaDataSize, uLocalUserIndex, uSendSeqn);
    }
}

/*F********************************************************************************/
/*!
    \Function    _VoipCommonOpaqueDataCb

    \Description
        Callback to handle locally generated opaque data to be sent reliably to other voip peers.

    \Input pOpaqueData      - data buffer
    \Input iOpaqueDataSize  - opaque data size in bytes
    \Input uSendMask        - mask identifying which connections to send the data to
    \Input bReliable        - TRUE if reliable transmission needed, FALSE otherwise
    \Input uSendSeqn        - send sequence
    \Input *pUserData       - user data

    \Version 11/22/2018 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipCommonOpaqueDataCb(const uint8_t *pOpaqueData, int32_t iOpaqueDataSize, uint32_t uSendMask, uint8_t bReliable, uint8_t uSendSeqn, void *pUserData)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pUserData;

    int32_t iConnID;

    if (bReliable)
    {
        for (iConnID = 0; iConnID < pVoipCommon->Connectionlist.iMaxConnections; iConnID++)
        {
            if (uSendMask & (1 << iConnID))
            {
                VoipConnectionReliableSendOpaque(&pVoipCommon->Connectionlist, iConnID, pOpaqueData, iOpaqueDataSize);
            }
        }
    }
    else
    {
        /* The xboxone unreliable date frames are opaque to us. Consequently, we do not know if they contain encode voice from a specific
        local user or from multiple local users. Therfore, we always invoke VoipConnectionSend() with the local user index being
        VOIP_SHARED_USER_INDEX. By doing so, we guarantee that VOIP_REMOTE_USER_RECVVOICE and VOIP_LOCAL_USER_SENDVOICE gets updated
        consistently for all participating users on a given console. We loose the per-user granularity for that flag, but it's
        the best we can do. */
        VoipConnectionSend(&pVoipCommon->Connectionlist, uSendMask, pOpaqueData, iOpaqueDataSize, NULL, 0, VOIP_SHARED_USER_INDEX, uSendSeqn);
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipCommonEncode

    \Description
        Packs the channel selection for sending to a remote machine

    \Input iChannels[]      - the array of channels to encode
    \Input uModes[]         - the array of modes to encode

    \Output
        uint32_t            - the coded channels

    \Version 12/07/2009 (jrainy)
*/
/********************************************************************************F*/
static uint32_t _VoipCommonEncode(int32_t iChannels[], uint32_t uModes[])
{
    uint32_t codedChannel = 0;
    int32_t iIndex;

    for(iIndex = 0; iIndex < VOIP_MAX_CONCURRENT_CHANNEL; iIndex++)
    {
        codedChannel |= (iChannels[iIndex] & 0x3f) << (8 * iIndex);
        codedChannel |= ((uModes[iIndex] & 0x3) << 6) << (8 * iIndex);
    }

    return(codedChannel);
}

/*F********************************************************************************/
/*!
    \Function   _VoipCommonDecode

    \Description
        Unpacks the channel selection received by a remote machine

    \Input uCodedChannel    - the coded channels
    \Input iChannels[]      - the array of channels to fill up
    \Input uModes[]         - the array of modes to fill up

    \Version 12/07/2009 (jrainy)
*/
/********************************************************************************F*/
static void _VoipCommonDecode(uint32_t uCodedChannel, int32_t iChannels[], uint32_t uModes[])
{
    int32_t iIndex;
    for(iIndex = 0; iIndex < VOIP_MAX_CONCURRENT_CHANNEL; iIndex++)
    {
        iChannels[iIndex] = (uCodedChannel >> (8 * iIndex)) & 0x3f;
        uModes[iIndex] = ((uCodedChannel >> (8 * iIndex)) >> 6) & 0x3;
    }
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonProcessChannelChange

    \Description
        Unpacks the channel selection received by a remote machine

    \Input pVoipCommon   - voip common state
    \Input iConnId       - connection id

    \Version 12/07/2009 (jrainy)
*/
/********************************************************************************F*/
void VoipCommonProcessChannelChange(VoipCommonRefT *pVoipCommon, int32_t iConnId)
{
    int32_t iUserIndex;

    for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS_EXTENDED; iUserIndex++)
    {
        uint32_t uInputOuputVar;
        #if DIRTYCODE_LOGGING
        uint32_t uNewChannelConfig, uOldChannelConfig;
        #endif

        uInputOuputVar = (uint32_t)iUserIndex;
        VoipStatus(VoipGetRef(), 'rchn', iConnId, &uInputOuputVar, sizeof(uInputOuputVar));

        #if DIRTYCODE_LOGGING
        uNewChannelConfig = uInputOuputVar;
        uOldChannelConfig = pVoipCommon->uRemoteChannelSelection[iConnId][iUserIndex];

        NetPrintf(("voipcommon: got remote channels 0x%08x (old config: 0x%08x)for user index %d on low-level conn id %d\n",
            uNewChannelConfig, uOldChannelConfig, iUserIndex, iConnId));
        #endif

        pVoipCommon->uRemoteChannelSelection[iConnId][iUserIndex] = uInputOuputVar;
    }

    VoipCommonApplyChannelConfig(pVoipCommon);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipIdle

    \Description
        NetConn idle function to update the Voip module.

        This function is designed to handle issuing user callbacks based on events such as
        headset insertion and removal.  It is implemented as a NetConn idle function instead
        of as a part of the Voip thread so that callbacks will be generated from the same
        thread that DirtySock operates in.

    \Input *pData   - pointer to module state
    \Input uTick    - current tick count

    \Notes
        This function is installed as a NetConn Idle function.  NetConnIdle()
        must be regularly polled for this function to be called.

    \Version 02/10/2006 (jbrookes)
*/
/*************************************************************************************************F*/
static void _VoipIdle(void *pData, uint32_t uTick)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pData;
    int32_t iIndex;

    // check for state changes and trigger callback
    if (pVoipCommon->uLastHsetStatus != pVoipCommon->uPortHeadsetStatus)
    {
        pVoipCommon->pCallback((VoipRefT *)pData, VOIP_CBTYPE_HSETEVENT, pVoipCommon->uPortHeadsetStatus, pVoipCommon->pUserData);
        pVoipCommon->uLastHsetStatus = pVoipCommon->uPortHeadsetStatus;
    }

    if (pVoipCommon->uLastFrom != pVoipCommon->Connectionlist.uRecvVoice)
    {
        pVoipCommon->pCallback((VoipRefT *)pData, VOIP_CBTYPE_FROMEVENT, pVoipCommon->Connectionlist.uRecvVoice, pVoipCommon->pUserData);
        pVoipCommon->uLastFrom = pVoipCommon->Connectionlist.uRecvVoice;
    }

    if (pVoipCommon->uLastTtsStatus != pVoipCommon->uTtsStatus)
    {
        pVoipCommon->pCallback((VoipRefT *)pData, VOIP_CBTYPE_TTOSEVENT, pVoipCommon->uTtsStatus, pVoipCommon->pUserData);
        pVoipCommon->uLastTtsStatus = pVoipCommon->uTtsStatus;
    }

    for (iIndex = 0; iIndex < VOIP_MAXLOCALUSERS; ++iIndex)
    {
        if (pVoipCommon->uLastLocalStatus[iIndex] != pVoipCommon->Connectionlist.uLocalUserStatus[iIndex])
        {
            if ((pVoipCommon->uLastLocalStatus[iIndex] ^ pVoipCommon->Connectionlist.uLocalUserStatus[iIndex]) & VOIP_LOCAL_USER_SENDVOICE)
            {
                pVoipCommon->pCallback((VoipRefT *)pData, VOIP_CBTYPE_SENDEVENT, pVoipCommon->Connectionlist.uLocalUserStatus[iIndex] & VOIP_LOCAL_USER_SENDVOICE, pVoipCommon->pUserData);
            }
            pVoipCommon->uLastLocalStatus[iIndex] = pVoipCommon->Connectionlist.uLocalUserStatus[iIndex];
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipCommonChannelMatch

    \Description
        Returns whether a given channel selection is to be received and/or sent to

    \Input *pVoipCommon         - voip common state
    \Input iUserIndex           - local user index
    \Input uCodedChannel        - coded channels of remote user

    \Output
        uint32_t                - the coded channels

    \Version 12/07/2009 (jrainy)
*/
/********************************************************************************F*/
static VoipChanModeE _VoipCommonChannelMatch(VoipCommonRefT *pVoipCommon, int32_t iUserIndex, uint32_t uCodedChannel)
{
    int32_t iIndexRemote;
    int32_t iIndexLocal;
    VoipChanModeE eMode = 0;
    int32_t iChannels[VOIP_MAX_CONCURRENT_CHANNEL];
    uint32_t uModes[VOIP_MAX_CONCURRENT_CHANNEL];

    // Voip traffic sent on channels {(0,none),(0,none),(0,none),(0,none)} = talk/listen to everyone = VOIPCOMMON_CODEDCHAN_ALL
    // Voip traffic blocked on channels {(1,none),(1,none),(1,none),(1,none)} = can't talk or listen = VOIPCOMMON_CODEDCHAN_NONE
    // is assumed to be meant for everybody.
    // enables default behaviour for teams not using channels.
    if ((uCodedChannel == VOIPCOMMON_CODEDCHAN_ALL) || (pVoipCommon->uLocalChannelSelection[iUserIndex] == VOIPCOMMON_CODEDCHAN_ALL))
    {
        return(VOIP_CHANSEND|VOIP_CHANRECV);
    }
    else if ((uCodedChannel == VOIPCOMMON_CODEDCHAN_NONE) || (pVoipCommon->uLocalChannelSelection[iUserIndex] == VOIPCOMMON_CODEDCHAN_NONE))
    {
        return(VOIP_CHANNONE);
    }

    _VoipCommonDecode(uCodedChannel, iChannels, uModes);

    // for all our channels, and all the channels of the remote party
    for(iIndexRemote = 0; iIndexRemote < VOIP_MAX_CONCURRENT_CHANNEL; iIndexRemote++)
    {
        for(iIndexLocal = 0; iIndexLocal < VOIP_MAX_CONCURRENT_CHANNEL; iIndexLocal++)
        {
            // if their channel numbers match 
            if (pVoipCommon->iLocalChannels[iUserIndex][iIndexLocal] == iChannels[iIndexRemote])
            {
                // if we're sending on it and they're receiving on it
                if ((pVoipCommon->uLocalModes[iUserIndex][iIndexLocal] & VOIP_CHANSEND) && (uModes[iIndexRemote] & VOIP_CHANRECV))
                {
                    // let's send
                    eMode |= VOIP_CHANSEND;
                }
                // if we're receiving on it and they're send on it
                if ((pVoipCommon->uLocalModes[iUserIndex][iIndexLocal] & VOIP_CHANRECV) && (uModes[iIndexRemote] & VOIP_CHANSEND))
                {
                    // let's receive
                    eMode |= VOIP_CHANRECV;
                }
            }
        }
    }

    return(eMode);
}


/*F********************************************************************************/
/*!
    \Function   _VoipCommonComputeEffectiveUserSendMask

    \Description
        Compute effective user send mask from two other masks: user-selected
        send mask and mask obtained from voip channel config

    \Input *pVoipCommon - voip common state

    \Version 12/07/2009 (jrainy)
*/
/********************************************************************************F*/
static void _VoipCommonComputeEffectiveUserSendMask(VoipCommonRefT *pVoipCommon)
{
    VoipCommonSetMask(&pVoipCommon->Connectionlist.uUserSendMask, pVoipCommon->uUserMicrValue & pVoipCommon->uChanSendMask, "usendmask");
    #if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
    pVoipCommon->Connectionlist.bApplyRelFromMuting = TRUE; // to reflect these changes in the communication relationship
    #endif
}

/*F********************************************************************************/
/*!
    \Function   _VoipCommonComputeDefaultSharedChannelConfig

    \Description
        Computes the default shared channel config. This is the logical "and" of all 
        local user's channel modes

    \Input *pVoipCommon - voip common state

    \Notes
        Loops through all the channels and "and" the channel mode of all the valid users on the
        that particular channel together. Only if everyone is subcribed to the channel do we set
        the mode for the shared user.

    \Version 12/11/2014 (tcho)
*/
/********************************************************************************F*/
static void _VoipCommonComputeDefaultSharedChannelConfig(VoipCommonRefT *pVoipCommon)
{
    // only bother to do any of this if we are in fact using the "default shared channel config", which is more of a behavior than a static thing 
    if (pVoipCommon->bUseDefaultSharedChannelConfig)
    {
        int32_t iLocalUserIndex;
        int32_t iChannel;
        int32_t iChannelSlot;
        int32_t iEmptySlot = 0;

        _VoipCommonDecode(VOIPCOMMON_CODEDCHAN_NONE, pVoipCommon->iDefaultSharedChannels, pVoipCommon->uDefaultSharedModes);

        // loop through all the channel from 1 to 63
        for (iChannel = 1; iChannel < VOIP_MAXLOCALCHANNEL; ++iChannel)
        {
            int32_t iValidUser = 0;
            int32_t iChannelSubscriber = 0;
            int32_t iChannelAllUser = 0;
            uint32_t uSharedChannelMode = VOIP_CHANSENDRECV;

            // only compute the shared channel config if we still have a free slot
            if (iEmptySlot != VOIP_MAX_CONCURRENT_CHANNEL)
            {
                // loop through all the local users excluding the shared user
                for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS; ++iLocalUserIndex)
                {
                    if (!VOIP_NullUser((VoipUserT *)&pVoipCommon->Connectionlist.LocalUsers[iLocalUserIndex]))
                    {
                        ++iValidUser;

                        // the user is valid loop through his subscribed channels
                        for (iChannelSlot = 0; iChannelSlot < VOIP_MAX_CONCURRENT_CHANNEL; ++iChannelSlot)
                        {
                            // check to see if we are in an VOIPCOMMON_CODEDCHAN_ALL state, in which case consider treating the shared user as a VOIPCOMMON_CODEDCHAN_ALL
                            if (_VoipCommonEncode(pVoipCommon->iLocalChannels[iLocalUserIndex], pVoipCommon->uLocalModes[iLocalUserIndex]) == VOIPCOMMON_CODEDCHAN_ALL)
                            {
                                ++iChannelAllUser;
                                break;
                            }
                            // check to see if the user is actually a member of the channel, in which case consider using this channel for the shared user too
                            else if (pVoipCommon->iLocalChannels[iLocalUserIndex][iChannelSlot] == iChannel)
                            {
                                ++iChannelSubscriber;
                                uSharedChannelMode &= pVoipCommon->uLocalModes[iLocalUserIndex][iChannelSlot];
                                break;
                            }
                        }
                    }
                }

                // if everyone is all open, then the shared channel is all open too
                if ((iValidUser == iChannelAllUser) && (iChannelAllUser != 0))
                {
                    _VoipCommonDecode(VOIPCOMMON_CODEDCHAN_ALL, pVoipCommon->iDefaultSharedChannels, pVoipCommon->uDefaultSharedModes);
                }
                // if everyone is subscribed to the channel (or all open) then the uSharedChannelMode is valid
                else if ((iValidUser == (iChannelSubscriber + iChannelAllUser)) && (iChannelSubscriber != 0))
                {
                    // if we were set to VOIPCOMMON_CODEDCHAN_NONE but now we are going to make changes, clear out to VOIPCOMMON_CODEDCHAN_ALL first
                    if (_VoipCommonEncode(pVoipCommon->iDefaultSharedChannels, pVoipCommon->uDefaultSharedModes) == VOIPCOMMON_CODEDCHAN_NONE)
                    {
                        _VoipCommonDecode(VOIPCOMMON_CODEDCHAN_ALL, pVoipCommon->iDefaultSharedChannels, pVoipCommon->uDefaultSharedModes);
                    }
                    pVoipCommon->iDefaultSharedChannels[iEmptySlot] = iChannel;
                    pVoipCommon->uDefaultSharedModes[iEmptySlot] = uSharedChannelMode;
                    ++iEmptySlot;
                }
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipCommonComputeEffectiveUserRecvMask

    \Description
        Compute effective user recv mask from two other masks: user-selected
        recv mask and mask obtained from voip channel config

    \Input *pVoipCommon - voip common state

    \Version 12/07/2009 (jrainy)
*/
/********************************************************************************F*/
static void _VoipCommonComputeEffectiveUserRecvMask(VoipCommonRefT *pVoipCommon)
{
    VoipCommonSetMask(&pVoipCommon->Connectionlist.uUserRecvMask, pVoipCommon->uUserSpkrValue & pVoipCommon->uChanRecvMask, "urecvmask");
    #if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
    pVoipCommon->Connectionlist.bApplyRelFromMuting = TRUE; // to reflect these changes in the communication relationship
    #endif
}

/*F********************************************************************************/
/*!
    \Function   _VoipCommonSetChannels

    \Description
        Set the active voip channel (coded value of channels and modes)

    \Input *pVoipCommon - voip common state
    \Input uChannels    - channels value we are setting for the user
    \Input iUserIndex   - index of user we are performing the operation on

    \Output
        int32_t         - zero=success, negative=failure

    \Version 01/30/2019 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipCommonSetChannels(VoipCommonRefT *pVoipCommon, uint32_t uChannels, int32_t iUserIndex)
{
    if ((iUserIndex >= 0) && (iUserIndex < VOIP_MAXLOCALUSERS))
    {
        NetPrintf(("voipcommon: setting voip channels (0x%08x) for local user index %d\n", uChannels, iUserIndex));
        pVoipCommon->Connectionlist.aChannels[iUserIndex] = uChannels;
        return(0);
    }
    else if (iUserIndex == VOIP_SHARED_USER_INDEX)
    {
        NetPrintf(("voipcommon: setting voip channels (0x%08x) for local shared user\n", uChannels));
        pVoipCommon->Connectionlist.aChannels[iUserIndex] = uChannels;
        return(0);
    }
    else
    {
        NetPrintf(("voipcommon: warning - setting voip channels for invalid local user index %d\n", iUserIndex));
        return(-1);
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipCommonSetSharedUserChannelConfig

    \Description
        Set the shared user channel config to custom or default based on the 
        bUseDefaultSharedChannelConfig flag.

    \Input *pVoipCommon - voip common state

    \Version 12/16/2014 (tcho)
*/
/********************************************************************************F*/
static void _VoipCommonSetSharedUserChannelConfig(VoipCommonRefT *pVoipCommon)
{
    int32_t iMaxConcurrentChannel;
    int32_t iSharedUserIndex = VOIP_SHARED_USER_INDEX;

    for (iMaxConcurrentChannel = 0; iMaxConcurrentChannel < VOIP_MAX_CONCURRENT_CHANNEL; ++iMaxConcurrentChannel)
    {
        if (pVoipCommon->bUseDefaultSharedChannelConfig)
        {
            pVoipCommon->iLocalChannels[iSharedUserIndex][iMaxConcurrentChannel] = pVoipCommon->iDefaultSharedChannels[iMaxConcurrentChannel];
            pVoipCommon->uLocalModes[iSharedUserIndex][iMaxConcurrentChannel] = pVoipCommon->uDefaultSharedModes[iMaxConcurrentChannel];
        }
        else
        {
            pVoipCommon->iLocalChannels[iSharedUserIndex][iMaxConcurrentChannel] = pVoipCommon->iCustomSharedChannels[iMaxConcurrentChannel];
            pVoipCommon->uLocalModes[iSharedUserIndex][iMaxConcurrentChannel] = pVoipCommon->uCustomSharedModes[iMaxConcurrentChannel];
        }
    }

    // encdoe the channel selection and set the active voip channels
    pVoipCommon->uLocalChannelSelection[iSharedUserIndex] = _VoipCommonEncode(pVoipCommon->iLocalChannels[iSharedUserIndex], pVoipCommon->uLocalModes[iSharedUserIndex]);
    _VoipCommonSetChannels(pVoipCommon, pVoipCommon->uLocalChannelSelection[iSharedUserIndex], iSharedUserIndex);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipCommonCallback
    
    \Description
        Default (empty) user callback function.
        
    \Input *pVoip       - voip module state
    \Input eCbType      - type of event
    \Input iValue       - event-specific information
    \Input *pUserData   - callback user data
    
    \Output
        None.

    \Version 02/10/2006 (jbrookes)
*/
/*************************************************************************************************F*/
static void _VoipCommonCallback(VoipRefT *pVoip, VoipCbTypeE eCbType, int32_t iValue, void *pUserData)
{
}

#if defined(DIRTYCODE_PS4)
/*F********************************************************************************/
/*!
    \Function   _VoipCommonDisplayRemoteTextDataCb

    \Description
       Callback to handle receiving a transcribed text packet from a remote peer.
       (once permission check has been validated in voipheadset)

    \Input *pRemoteUser      - transcribed text's originator
    \Input *pStrUtf          - transcribed text
    \Input *pUserData        - user data

    \Version 04/04/2019 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipCommonDisplayRemoteTextDataCb(const VoipUserT *pRemoteUser, const char *pStrUtf8, void *pUserData)
{
    int32_t iConnectionIndex, iRemoteUserIndex = 0;
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pUserData;
    uint32_t bFound = FALSE;

    for (iConnectionIndex = 0; iConnectionIndex < pVoipCommon->Connectionlist.iMaxConnections; iConnectionIndex++)
    {
        VoipConnectionT *pConnection = &pVoipCommon->Connectionlist.pConnections[iConnectionIndex];

        for (iRemoteUserIndex = 0; iRemoteUserIndex < VOIP_MAXLOCALUSERS_EXTENDED; iRemoteUserIndex++)
        {
            if (VOIP_SameUser(pRemoteUser, (VoipUserT *)&pConnection->RemoteUsers[iRemoteUserIndex]))
            {
                bFound = TRUE;
                break;
            }
        }

        if (bFound == TRUE)
        {
            break;
        }
    }

    if (bFound == TRUE)
    {
        if (pVoipCommon->pDisplayTranscribedTextCb != NULL)
        { 
            pVoipCommon->pDisplayTranscribedTextCb(iConnectionIndex, iRemoteUserIndex, pStrUtf8, pVoipCommon->pDisplayTranscribedTextUserData);
        }
    }
    else
    {
        NetPrintf(("voipcommon: failed to find matching remote user for originator of transcribed text\n"));
    }
}
#endif

#if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
/*F********************************************************************************/
/*!
    \Function   _VoipCommonReceiveTextDataCb

    \Description
       Callback to handle receiving a transcribed text packet from a remote peer.

       \Input *pSourceUser  - transcribed text's originator
       \Input *pStrUtf      - transcribed text
       \Input *pUserData    - user data

    \Version 05/02/2017 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipCommonReceiveTextDataNativeCb(const VoipUserT *pSourceUser, const char *pStrUtf8, void *pUserData)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pUserData;

    if (pVoipCommon->pDisplayTranscribedTextCb != NULL)
    {
        int32_t iConnectionIndex, iUserIndex;

        // see if a local user sent this text (seeing what they said)
        for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS; iUserIndex++)
        {
            int64_t iLocalPersonaId;
            if (NetConnStatus('peid', iUserIndex, &iLocalPersonaId, sizeof(iLocalPersonaId)) == 0)
            {
                if (iLocalPersonaId == pSourceUser->AccountInfo.iPersonaId)
                {
                    pVoipCommon->pDisplayTranscribedTextCb(-1, iUserIndex, pStrUtf8, pVoipCommon->pDisplayTranscribedTextUserData);
                    return;
                }
            }
        }

        // search for a remote user who sent this text
        for (iConnectionIndex = 0; iConnectionIndex < pVoipCommon->Connectionlist.iMaxConnections; iConnectionIndex++)
        {
            VoipConnectionT *pConnection = &pVoipCommon->Connectionlist.pConnections[iConnectionIndex];

            for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS_EXTENDED; iUserIndex++)
            {
                if (VOIP_SameUser(pSourceUser, &pConnection->RemoteUsers[iUserIndex]))
                {
                    pVoipCommon->pDisplayTranscribedTextCb(iConnectionIndex, iUserIndex, pStrUtf8, pVoipCommon->pDisplayTranscribedTextUserData);
                    return;
                }
            }
        }

        // we didn't find a local or remote user who sent this text, just drop it but complain
        NetPrintf(("voipcommon: failed to find matching originator of transcribed text; user: %lld, text: %s\n", pSourceUser->AccountInfo.iPersonaId, pStrUtf8));
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function   _VoipCommonReceiveTextDataCb

    \Description
       Callback to handle receiving a transcribed text packet from a remote peer.

       \Input iConnId           - connection index
       \Input iRemoteUserIndex  - index of remote user
       \Input *pStrUtf8         - pointer to beginning of text
       \Input *pUserData        - user data

    \Version 05/02/2017 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipCommonReceiveTextDataCb(int32_t iConnId, int32_t iRemoteUserIndex, const char *pStrUtf8, void *pUserData)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pUserData;

    if (pVoipCommon->pDisplayTranscribedTextCb != NULL)
    {
        #if defined(DIRTYCODE_PS4)
        VoipUserT VoipUser;
        uint32_t uConnUserPair;

        // initialize variable used with VoipStatus('rcvu'). most-significant 16 bits = remote user index, least-significant 16 bits = conn index
        uConnUserPair = iConnId;
        uConnUserPair |= (iRemoteUserIndex << 16);

        // find out if there is a valid remote user for that connId/userId pair
        if (VoipStatus(VoipGetRef(), 'rcvu', (int32_t)uConnUserPair, &VoipUser, sizeof(VoipUser)) == 0)
        {
            VoipHeadsetTranscribedTextPermissionCheck(pVoipCommon->pHeadset, &VoipUser, pStrUtf8);
        }
        else
        {
            NetPrintf(("voipcommon: 'transcribed text received' event dropped because voipheadset can't find remote user index %d on connection %d\n", iRemoteUserIndex, iConnId));
        }
        #else
        pVoipCommon->pDisplayTranscribedTextCb(iConnId, iRemoteUserIndex, pStrUtf8, pVoipCommon->pDisplayTranscribedTextUserData);
        #endif

    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function   VoipCommonGetRef

    \Description
        Return current module reference.

    \Output
        VoipRefT *      - reference pointer, or NULL if the module is not active

    \Version 03/08/2004 (jbrookes)
*/
/********************************************************************************F*/
VoipRefT *VoipCommonGetRef(void)
{
    // return pointer to module state
    return(_Voip_pRef);
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
    int32_t iMemGroup;
    void *pMemGroupUserData;
    VoipCommonRefT *pVoipCommon;
    int32_t i;

    // make sure we're not already started
    if (_Voip_pRef != NULL)
    {
        NetPrintf(("voipcommon: module startup called when not in a shutdown state\n"));
        return(NULL);
    }

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // create and initialize module state
    if ((pVoipCommon = (VoipCommonRefT *)DirtyMemAlloc(iVoipRefSize, VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipcommon: unable to allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pVoipCommon, iVoipRefSize);

    // set voip memgroup
    VoipCommonMemGroupSet(iMemGroup, pMemGroupUserData);

    // by default, user-selected micr and spkr flag always default to ON
    pVoipCommon->uUserMicrValue = 0xFFFFFFFF;
    pVoipCommon->uUserSpkrValue = 0xFFFFFFFF;

    // create thread critical section
    NetCritInit(&pVoipCommon->ThreadCrit, "voip");
    
    // create block list
    if ((pVoipCommon->pBlockList = VoipBlockListCreate()) == NULL)
    {
        NetPrintf(("voipcommon: unable to allocate block list\n"));
        VoipCommonShutdown(pVoipCommon);
        return(NULL);
    }

    // create connection list
    if (VoipConnectionStartup(&pVoipCommon->Connectionlist, iMaxPeers) < 0)
    {
        NetPrintf(("voipcommon: unable to allocate connectionlist\n"));
        VoipCommonShutdown(pVoipCommon);
        return(NULL);
    }

    // create headset module
    if ((pVoipCommon->pHeadset = VoipHeadsetCreate(iMaxPeers, &_VoipCommonMicDataCb, &_VoipCommonTextDataCb, &_VoipCommonOpaqueDataCb, pStatusCb, pVoipCommon, iData)) == NULL)
    {
        NetPrintf(("voipcommon: unable to create VoipHeadset layer\n"));
        VoipCommonShutdown(pVoipCommon);
        return(NULL);
    }

    // turn on default shared channel config by default
    pVoipCommon->bUseDefaultSharedChannelConfig = TRUE;

    // set up connectionlist callback interface
#if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
    VoipConnectionSetCallbacks(&pVoipCommon->Connectionlist, &VoipHeadsetReceiveVoiceDataCb, (void *)pVoipCommon->pHeadset, 
        NULL, NULL,
        &VoipHeadsetRegisterUserCb, (void *)pVoipCommon->pHeadset,
        &VoipHeadsetReceiveOpaqueDataCb, (void *)pVoipCommon->pHeadset);

    VoipHeadsetSetTranscribedTextReceivedCallback(pVoipCommon->pHeadset, &_VoipCommonReceiveTextDataNativeCb, (void *)pVoipCommon);
#else
    #if defined(DIRTYCODE_PS4)
    VoipHeadsetSetTranscribedTextReceivedCallback(pVoipCommon->pHeadset, &_VoipCommonDisplayRemoteTextDataCb, (void *)pVoipCommon);
    #endif

    VoipConnectionSetCallbacks(&pVoipCommon->Connectionlist, &VoipHeadsetReceiveVoiceDataCb, (void *)pVoipCommon->pHeadset,
        &_VoipCommonReceiveTextDataCb, (void *)pVoipCommon,
        &VoipHeadsetRegisterUserCb, (void *)pVoipCommon->pHeadset,
        NULL, (void *)pVoipCommon->pHeadset);
#endif

    // add callback idle function
    VoipCommonSetEventCallback(pVoipCommon, NULL, NULL);
    pVoipCommon->uLastFrom = pVoipCommon->Connectionlist.uRecvVoice;
    for (i = 0; i < VOIP_MAXLOCALUSERS; ++i)
    {
        pVoipCommon->uLastLocalStatus[i] = pVoipCommon->Connectionlist.uLocalUserStatus[i];
    }
    NetConnIdleAdd(_VoipIdle, pVoipCommon);

    // save ref
    _Voip_pRef = (VoipRefT *)pVoipCommon;
    return(_Voip_pRef);
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
    void *pMemGroupUserData;
    int32_t iMemGroup;

    // del callback idle function
    if (_Voip_pRef)
    {
        NetConnIdleDel(_VoipIdle, pVoipCommon);
    }

    // destroy headset module
    if (pVoipCommon->pHeadset)
    {
        VoipHeadsetDestroy(pVoipCommon->pHeadset);
    }

    // shut down connectionlist
    if (pVoipCommon->Connectionlist.pConnections)
    {
        VoipConnectionShutdown(&pVoipCommon->Connectionlist);
    }

    // destroy the block list
    if (pVoipCommon->pBlockList)
    {
        VoipBlockListDestroy((VoipRefT *)pVoipCommon);
    }

    // destroy critical section
    NetCritKill(&pVoipCommon->ThreadCrit);

    // free module memory
    VoipCommonMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    DirtyMemFree(pVoipCommon, VOIP_MEMID, iMemGroup, pMemGroupUserData);

    // clear pointer to module state
    _Voip_pRef = NULL;
}

/*F********************************************************************************/
/*!
    \Function VoipCommonMemGroupQuery

    \Description
        Get VoIP mem group data.

    \Input *pMemGroup            - [OUT param] pointer to variable to be filled with mem group id
    \Input **ppMemGroupUserData  - [OUT param] pointer to variable to be filled with pointer to user data
    
    \Version 1.0 11/11/2005 (jbrookes) First Version
    \Version 1.1 11/18/2008 (mclouatre) returned values now passed in [OUT] parameters
*/
/********************************************************************************F*/
void VoipCommonMemGroupQuery(int32_t *pMemGroup, void **ppMemGroupUserData)
{
    *pMemGroup = _Voip_iMemGroup;
    *ppMemGroupUserData = _Voip_pMemGroupUserData;
}

/*F********************************************************************************/
/*!
    \Function VoipCommonMemGroupSet

    \Description
        Set VoIP mem group data.

    \Input iMemGroup             - mem group to set
    \Input *pMemGroupUserData    - user data associated with mem group
    
    \Version 1.0 11/11/2005 (jbrookes) First Version
    \Version 1.1 11/18/2008 (mclouatre) Adding second parameter (user data)
*/
/********************************************************************************F*/
void VoipCommonMemGroupSet(int32_t iMemGroup, void *pMemGroupUserData)
{
    _Voip_iMemGroup = iMemGroup;
    _Voip_pMemGroupUserData = pMemGroupUserData;
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
    uint32_t uSendMask, uRecvMask, uChannelMask, bEnabled;
    int32_t iConnection;
    int32_t bTextTranscriptionEnabled = FALSE;
    int32_t iLocalUserIndex;

    // process all active channels
    for (iConnection = 0; iConnection < pVoipCommon->Connectionlist.iMaxConnections; iConnection++)
    {
        VoipConnectionT *pConnection = &pVoipCommon->Connectionlist.pConnections[iConnection];

        // if not active, don't process
        if (pConnection->eState != ST_ACTV)
        {
            continue;
        }

        // calculate channel mask
        uChannelMask = 1 << iConnection;

        // decide whether this channel should be enabled or not
        bEnabled = pVoipCommon->bPrivileged;

        // set send/recv masks based on enable + user send override
        if (bEnabled && ((pVoipCommon->Connectionlist.uUserSendMask & uChannelMask) != 0))
        {
            uSendMask = pVoipCommon->Connectionlist.uSendMask | uChannelMask;
        }
        else
        {
            uSendMask = pVoipCommon->Connectionlist.uSendMask & ~uChannelMask;
        }

        // set send/recv masks based on enable + user recv override
        if (bEnabled && ((pVoipCommon->Connectionlist.uUserRecvMask & uChannelMask) != 0))
        {
            uRecvMask = pVoipCommon->Connectionlist.uRecvMask | uChannelMask;
        }
        else
        {
            uRecvMask = pVoipCommon->Connectionlist.uRecvMask & ~uChannelMask;
        }

        // set send/recv masks and priority
        VoipConnectionSetSendMask(&pVoipCommon->Connectionlist, uSendMask);
        VoipConnectionSetRecvMask(&pVoipCommon->Connectionlist, uRecvMask);

        if ((pConnection->bTranscribedTextRequested) && (pVoipCommon->Connectionlist.uSendMask & (1 << iConnection)))
        {
            // enable local text transcription because at least one remote user requested transcribed text
            bTextTranscriptionEnabled = TRUE;
        }
    }

    /* We also want text transcription locally enabled if any local user is requesting text transcription
    from remote users. Motivation: a hearing-impaired person requesting transcribed text from other
    players also wants to see transcribed text for his own speech (request from EA's accessibility team). */
    for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS; iLocalUserIndex++)
    {
        if (pVoipCommon->Connectionlist.bTranscribedTextRequested[iLocalUserIndex] == TRUE)
        {
            bTextTranscriptionEnabled = TRUE;
        }
    }

    if (bTextTranscriptionEnabled != pVoipCommon->bTextTranscriptionEnabled)
    {
        NetPrintf(("voipcommon: %s text transcription locally\n", bTextTranscriptionEnabled?"enabling":"disabling"));
        pVoipCommon->bTextTranscriptionEnabled = bTextTranscriptionEnabled;
        VoipHeadsetControl(pVoipCommon->pHeadset, 'tran', bTextTranscriptionEnabled, 0, NULL);
    }
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

    \Notes
        Allowed selectors:

        \verbatim
            'avlb' - whether a given ConnID is available
            'chan' - retrieve the active voip channel (coded value of channels and modes)
            'chnc' - return the count of channel slots used by the voip group manager
            'chnl' - return the channel id and channel mode associated with specified channel slot
            'from' - bitfield indicating which peers are talking
            'hset' - bitfield indicating which ports have headsets
            'lprt' - return local socket port
            'luvu' - return voipuser of specified local user
            'maxc' - return max connections
            'mgrp' - voip memgroup user id
            'mgud' - voip memgroup user data
            'micr' - who we're sending voice to
            'rchn' - retrieve the active voip channel of a remote peer (coded value of channels and modes)
            'rcvu' - return voipuser of specified remote user (connection and user index) 
            'shch' - return bUseDefaultSharedChannelConfig which indicates if the default shared channel config is used
            'sock' - voip socket ref
            'spkr' - who we're accepting voice from
            'umic' - user-specified microphone send list
            'uspk' - user-specified microphone recv list
        \endverbatim

    \Version 12/02/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipCommonStatus(VoipCommonRefT *pVoipCommon, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'avlb')
    {
        return(VoipConnectionCanAllocate(&pVoipCommon->Connectionlist, iValue));
    }
    if (iSelect == 'chan')
    {
        return(pVoipCommon->Connectionlist.aChannels[iValue]);
    }
    if (iSelect == 'chnc')
    {
       return(VOIP_MAX_CONCURRENT_CHANNEL);
    }
    if (iSelect == 'chnl')
    {
        int32_t iUserIndex = iValue & 0xFFFF;
        int32_t iChannelSlot = iValue >> 16;

        if (!pBuf)
        {
            NetPrintf(("voipcommon: pBuf must be valid with the 'chnl' selector\n"));
            return(-1);
        }

        if ((unsigned)iBufSize < sizeof(VoipChanModeE))
        {
            NetPrintf(("voipcommon: user buffer used with the 'chnl' selector is too small (expected size = %d; current size =%d)\n",
                sizeof(VoipChanModeE), iBufSize));
            return(-2);
        }

        if ((iChannelSlot < 0) || (iChannelSlot >= VOIP_MAX_CONCURRENT_CHANNEL))
        {
            NetPrintf(("voipcommon: invalid slot id (%d) used with 'chnl' selector; valid range is [0,%d]\n", iChannelSlot, VOIP_MAX_CONCURRENT_CHANNEL-1));
            return(-3);
        }

        *(VoipChanModeE *)pBuf = pVoipCommon->uLocalModes[iUserIndex][iChannelSlot];
        return(pVoipCommon->iLocalChannels[iUserIndex][iChannelSlot]);
    }
    if (iSelect == 'from')
    {
        return(pVoipCommon->Connectionlist.uRecvVoice);
    }
    if (iSelect == 'hset')
    {
        uint32_t uPortHeadsetStatus = pVoipCommon->uPortHeadsetStatus;
        return(uPortHeadsetStatus);
    }
    if (iSelect == 'lprt')
    {
        return(pVoipCommon->Connectionlist.uBindPort);
    }
    if (iSelect == 'luvu')
    {
        if (pBuf)
        {
            if (iBufSize >= (signed)sizeof(VoipUserT))
            {
                if ((iValue < VOIP_MAXLOCALUSERS_EXTENDED) && (!VOIP_NullUser((VoipUserT *)(&pVoipCommon->Connectionlist.LocalUsers[iValue]))))
                {
                    // copy user id in caller-provided buffer
                    ds_memcpy(pBuf, &pVoipCommon->Connectionlist.LocalUsers[iValue], sizeof(VoipUserT));
                    return(0);
                }
            }
            else
            {
                NetPrintf(("voipcommon: VoipCommonStatus('luvu') error -->  iBufSize (%d) not big enough (needs to be at least %d bytes)\n",
                    iBufSize, sizeof(VoipUserT)));
            }
        }
        else
        {
            NetPrintf(("voipcommon: VoipCommonStatus('luvu') error --> pBuf cannot be NULL\n"));
        }

        return(-1);
    }
    if (iSelect == 'maxc')
    {
        return(pVoipCommon->Connectionlist.iMaxConnections);
    }
    if (iSelect == 'mgrp')
    {
        int32_t iMemGroup;
        void *pMemGroupUserData;

        // Query mem group data
        VoipCommonMemGroupQuery(&iMemGroup, &pMemGroupUserData);

        // return mem group id
        return(iMemGroup);
    }
    if (iSelect == 'mgud')
    {
        // make sure user-provided buffer is large enough to receive a pointer
        if ((pBuf != NULL) && (iBufSize >= (signed)sizeof(void *)))
        {
            int32_t iMemGroup;
            void *pMemGroupUserData;

            // Query mem group data
            VoipCommonMemGroupQuery(&iMemGroup, &pMemGroupUserData);

            // fill [out] parameter with pointer to mem group user data
            *((void **)pBuf) = pMemGroupUserData;
            return(0);
        }
        else
        {
            // unhandled
            return(-1);
        }
    }
    if (iSelect == 'micr')
    {
        return(pVoipCommon->Connectionlist.uSendMask);
    }
    if (iSelect == 'rchn')
    {
        if (pBuf)
        {
            if (iBufSize == sizeof(uint32_t))
            {
                // pBut is used as an input and an output parameter
                // input -> user index
                // output -> returned channels
                int32_t iUserIndex = *(uint32_t *)pBuf;
                *(uint32_t *)pBuf = pVoipCommon->Connectionlist.pConnections[iValue].aRecvChannels[iUserIndex];
                return(0);
            }
            else
            {
                NetPrintf(("voipcommon: VoipCommonStatus('rchn') error for conn id %d -->  iBufSize (%d) does not match expected size (%d)\n",
                    iValue, iBufSize, sizeof(int32_t)));
            }
        }
        else
        {
            NetPrintf(("voipcommon: VoipCommonStatus('rchn') error for conn id %d --> pBuf cannot be NULL\n", iValue));
        }

        return(-1);
    }
    if (iSelect == 'rcvu')
    {
        int32_t iConnIndex = iValue & 0xFFFF;
        int32_t iRemoteUserIndex = iValue >> 16;

        if (pBuf)
        {
            if (iBufSize >= (signed)sizeof(VoipUserT))
            {
                if (pVoipCommon->Connectionlist.pConnections[iConnIndex].eState != ST_DISC)
                {
                    if ((iRemoteUserIndex < VOIP_MAXLOCALUSERS_EXTENDED) && (!VOIP_NullUser((VoipUserT *)(&pVoipCommon->Connectionlist.pConnections[iConnIndex].RemoteUsers[iRemoteUserIndex]))))
                    {
                        // copy user id in caller-provided buffer
                        ds_memcpy(pBuf, &pVoipCommon->Connectionlist.pConnections[iConnIndex].RemoteUsers[iRemoteUserIndex], sizeof(VoipUserT));
                        return(0);
                    }
                }
                else
                {
                    NetPrintf(("voipcommon: VoipCommonStatus('rcvu') error for conn id %d and remote user index %d --> connection is not active\n",
                            iConnIndex, iRemoteUserIndex));
                }
            }
            else
            {
                NetPrintf(("voipcommon: VoipCommonStatus('rcvu') error for conn id %d and remote user index %d --> iBufSize (%d) not big enough (needs to be at least %d bytes)\n",
                    iConnIndex, iRemoteUserIndex, iBufSize, sizeof(VoipUserT)));
            }
        }
        else
        {
            NetPrintf(("voipcommon: VoipCommonStatus('rcvu') error for conn id %d and remote user index %d --> pBuf cannot be NULL\n", iConnIndex, iRemoteUserIndex));
        }

        return(-1);
    }
    if (iSelect == 'shch')
    {
        return(pVoipCommon->bUseDefaultSharedChannelConfig);
    }
    if (iSelect == 'sock')
    {
        if (pBuf != NULL)
        {
            if (iBufSize >= (signed)sizeof(pVoipCommon->Connectionlist.pSocket))
            {
                ds_memcpy(pBuf, &pVoipCommon->Connectionlist.pSocket, sizeof(pVoipCommon->Connectionlist.pSocket));
            }
            else
            {
                NetPrintf(("voipcommon: socket reference cannot be copied because user buffer is not large enough\n"));
                return(-1);
            }
        }
        else
        {
            NetPrintf(("voipcommon: socket reference cannot be copied because user buffer pointer is NULL\n"));
            return(-1);
        }
        return(0);
    }
    if (iSelect == 'spkr')
    {
        return(pVoipCommon->Connectionlist.uRecvMask);
    }
    if (iSelect == 'umic')
    {
        return(pVoipCommon->uUserMicrValue);
    }
    if (iSelect == 'uspk')
    {
        return(pVoipCommon->uUserSpkrValue);
    }
    // unrecognized selector, so pass through to voipheadset
    return(VoipHeadsetStatus(pVoipCommon->pHeadset, iSelect, iValue, pBuf, iBufSize));
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

    \Notes
        iControl can be one of the following:

        \verbatim
            'clid' - set local client id
            'flsh' - send currently queued voice data immediately (iValue=connection id)
            'shch' - turn on or the of the default shared channel config. iValue = TRUE to turn, FALSE to turn off
            'stot' - turn on/off (*pValue) STT for local user specified with iValue
            'time' - set data timeout in milliseconds
            'xply' - toggle cross play
        \endverbatim

        Unhandled selectors are passed through to VoipHeadsetControl()

    \Version 03/02/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipCommonControl(VoipCommonRefT *pVoipCommon, int32_t iControl, int32_t iValue, void *pValue)
{
    if (iControl == 'clid')
    {
        if ((pVoipCommon->Connectionlist.uClientId != 0) && (pVoipCommon->Connectionlist.uClientId != (unsigned)iValue))
        {
            NetPrintf(("voipcommon: warning - local client id is being changed from %d to %d\n", pVoipCommon->Connectionlist.uClientId, iValue));
        }
        pVoipCommon->Connectionlist.uClientId = (unsigned)iValue;
        return(0);
    }
    if (iControl == 'flsh')
    {
        return(VoipConnectionFlush(&pVoipCommon->Connectionlist, iValue));
    }
    if (iControl == 'shch')
    {
        pVoipCommon->bUseDefaultSharedChannelConfig = iValue;
        pVoipCommon->bApplyChannelConfig = TRUE;
        NetPrintf(("voipcommon: VoipCommonControl('shch') default shared channel config is %s\n", pVoipCommon->bUseDefaultSharedChannelConfig ? "on" : "off"));
        return(0);
    }
    if (iControl == 'stot')
    {
        uint32_t bEnabled = TRUE;  // default to 'enabled'
        int32_t iUserLocalIndex = iValue;

        if (pValue != NULL)
        { 
            bEnabled = *(uint32_t *)pValue;
        }

        if ((iUserLocalIndex >= 0) && (iUserLocalIndex < VOIP_MAXLOCALUSERS))
        {
            NetPrintf(("voipcommon: %s STT for local user index %d\n", (bEnabled ?"enabling":"disabling"), iUserLocalIndex));
            pVoipCommon->Connectionlist.bTranscribedTextRequested[iUserLocalIndex] = bEnabled;
            return(0);
        }
        else
        {
            NetPrintf(("voipcommon: warning invalid local user index %d used with VoipControl('stot')\n", iUserLocalIndex));
            return(-1);
        }
    }
    if (iControl == 'time')
    {
        pVoipCommon->Connectionlist.iDataTimeout = iValue;
        return(0);
    }
    if (iControl == 'xply')
    {
        int32_t iRet;
        NetCritEnter(&pVoipCommon->ThreadCrit);
        #if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
        if (iValue == TRUE)
        {
            VoipConnectionSetTextCallback(&pVoipCommon->Connectionlist, &_VoipCommonReceiveTextDataCb, (void *)pVoipCommon);
        }
        else
        {
            VoipConnectionSetTextCallback(&pVoipCommon->Connectionlist, NULL, NULL);
        }
        #endif
        iRet = VoipHeadsetControl(pVoipCommon->pHeadset, 'xply', iValue, 0, pValue);
        NetCritLeave(&pVoipCommon->ThreadCrit);

        return(iRet);
    }

    // unhandled selectors are passed through to voipheadset
    if (pVoipCommon)
    {
        return(VoipHeadsetControl(pVoipCommon->pHeadset, iControl, iValue, 0, pValue));
    }
    else
    {
        // some voipheadset control selectors can be used before a call to VoipStartup()
        return(VoipHeadsetControl(NULL, iControl, iValue, 0, pValue));
    }
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
    VoipCommonSetMask(pMask, *pMask | uAddMask, pMaskName);
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
    VoipCommonSetMask(pMask, *pMask & ~uDelMask, pMaskName);
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
    NetPrintf(("voipcommon: %s update: 0x%08x->0x%08x\n", pMaskName, *pMask, uNewMask));
    *pMask = uNewMask;
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
    int32_t iIndex;
    int32_t iSlot = VOIP_MAX_CONCURRENT_CHANNEL;
    int32_t iCount = 0;
 
    // if the shared user index is 0xff, the current platform does not support shared channel config feature
    if (iUserIndex != VOIP_SHARED_USER_INDEX)
    {
        if ((iUserIndex < 0) || (iUserIndex >= VOIP_MAXLOCALUSERS))
        {
            NetPrintf(("voipcommon: [channel] warning - attempt to set invalid local channels index %d\n", iUserIndex));
            return(-3);
        }
    }

    // enforcing the valid ranges
    eMode &= VOIP_CHANSENDRECV;
    iChannel &= 0x3f;

    // find the slot to store the specified channel and count the slots remaining.
    for(iIndex = 0; iIndex < VOIP_MAX_CONCURRENT_CHANNEL; iIndex++)
    {
        // remember either the slot with the channel we want to set, or if we have found nothing, an empty slot
        if ( ((pVoipCommon->uLocalModes[iUserIndex][iIndex] != VOIP_CHANNONE) && (pVoipCommon->iLocalChannels[iUserIndex][iIndex] == iChannel)) ||
             ((pVoipCommon->uLocalModes[iUserIndex][iIndex] == VOIP_CHANNONE) && (iSlot == VOIP_MAX_CONCURRENT_CHANNEL)) )
        {
            iSlot = iIndex;
        }
        // and take the opportunity to count the free slots
        if (pVoipCommon->uLocalModes[iUserIndex][iIndex] == VOIP_CHANNONE)
        {
            iCount++;
        }
    }

    // no more slots to store the channel selection or
    // the given channel doesn't exist
    if (iSlot == VOIP_MAX_CONCURRENT_CHANNEL)
    {
        return(-1);
    }

    //count if we're taking a spot.
    if ((pVoipCommon->uLocalModes[iUserIndex][iSlot] == VOIP_CHANNONE) && (eMode != VOIP_CHANNONE))
    {
        iCount--;
    }
    //count if we're freeing a spot.
    if ((pVoipCommon->uLocalModes[iUserIndex][iSlot] != VOIP_CHANNONE) && (eMode == VOIP_CHANNONE))
    {
        iCount++;
        iChannel = 0; // when freeing a channel reset it back to the 0 to bring us towards the default state
    }

    // if we are trying to set the shared channel 
    if (iUserIndex == VOIP_SHARED_USER_INDEX)
    {
        pVoipCommon->iCustomSharedChannels[iSlot] = iChannel;
        pVoipCommon->uCustomSharedModes[iSlot] = eMode;
    }
    // set the channel and mode in selected slot
    else
    {
        pVoipCommon->iLocalChannels[iUserIndex][iSlot] = iChannel;
        pVoipCommon->uLocalModes[iUserIndex][iSlot] = eMode;

        NetPrintf(("voipcommon: [channel] set local channel at slot %d: channelId=%d, channelMode=%u for local user index %d\n",
        iSlot, pVoipCommon->iLocalChannels[iUserIndex][iSlot], pVoipCommon->uLocalModes[iUserIndex][iSlot], iUserIndex));
        pVoipCommon->uLocalChannelSelection[iUserIndex] = _VoipCommonEncode(pVoipCommon->iLocalChannels[iUserIndex], pVoipCommon->uLocalModes[iUserIndex]);
        NetPrintf(("voipcommon: [channel] set local channels 0x%08x for local user index %d\n", pVoipCommon->uLocalChannelSelection[iUserIndex], iUserIndex));
    }

    _VoipCommonSetChannels(pVoipCommon, pVoipCommon->uLocalChannelSelection[iUserIndex], iUserIndex);
    pVoipCommon->bApplyChannelConfig = TRUE;

    return(iCount);

}

/*F********************************************************************************/
/*!
    \Function   VoipCommonApplyChannelConfig

    \Description
        Setup user muting flags based on channel config

    \Input *pVoipCommon - voip module state

    \Todo
        Rename this function to indicate that we are applying the peer to peer
        mutes instead of just applying channel configurations. The name we have
        now no longer makes sense.

    \Version 12/02/2009 (jrainy)
*/
/********************************************************************************F*/
void VoipCommonApplyChannelConfig(VoipCommonRefT *pVoipCommon)
{
    static uint32_t uLastChanSendMask = 0;
    static uint32_t uLastChanRecvMask = 0;
    int32_t iConnIndex, iLocalUserIndex, iRemoteUserIndex;
    uint32_t uConnUserPair;
    uint8_t bSocialBlocked = FALSE;
    VoipChanModeE eMatch;
    VoipUserT LocalUser;

    pVoipCommon->uChanRecvMask = pVoipCommon->uChanSendMask = 0;
    pVoipCommon->uUserSendMask = 0;

    // setup the shared channel, to reflect the channel changes that may have occurred of the local users
    _VoipCommonComputeDefaultSharedChannelConfig(pVoipCommon);
    _VoipCommonSetSharedUserChannelConfig(pVoipCommon);

    for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS_EXTENDED; iLocalUserIndex++)
    {
        // find out if there is a local user for that local user index
        if (VoipStatus(VoipGetRef(), 'luvu', iLocalUserIndex, &LocalUser, sizeof(LocalUser)) == 0)
        {
            for(iConnIndex = 0; iConnIndex < VoipStatus(VoipGetRef(), 'maxc', 0, NULL, 0); iConnIndex++)
            {
                if (pVoipCommon->Connectionlist.pConnections[iConnIndex].eState != ST_DISC)
                {
                    for (iRemoteUserIndex = 0; iRemoteUserIndex < VOIP_MAXLOCALUSERS_EXTENDED; iRemoteUserIndex++)
                    {
                        // initialize variable used with VoipStatus('rcvu'). most-significant 16 bits = remote user index, least-significant 16 bits = conn index
                        VoipUserT RemoteUser;
                        uConnUserPair = iConnIndex;
                        uConnUserPair |= (iRemoteUserIndex << 16);

                        // find out if there is a valid remote user for that connId/userId pair
                        if (VoipStatus(VoipGetRef(), 'rcvu', (int32_t)uConnUserPair, &RemoteUser, sizeof(RemoteUser)) == 0)
                        {
                            uint8_t bFirstPartyBlocked = FALSE;
                            eMatch = _VoipCommonChannelMatch(pVoipCommon, iLocalUserIndex, pVoipCommon->uRemoteChannelSelection[iConnIndex][iRemoteUserIndex]);

                            // does the local user have that remote user blocked, if so do not alow voip between them?
                            bSocialBlocked = VoipBlockListIsBlocked((VoipRefT *)pVoipCommon, iLocalUserIndex, RemoteUser.AccountInfo.iAccountId);
 
                            // apply first party mute list block
                            if (VoipHeadsetStatus(pVoipCommon->pHeadset, 'fpml', iLocalUserIndex, &RemoteUser, sizeof(RemoteUser)) > 0)
                            {
                                bFirstPartyBlocked = TRUE;
                            }

                            // update send mask
                            if (!bFirstPartyBlocked && !bSocialBlocked && (eMatch & VOIP_CHANSEND))
                            {
                                pVoipCommon->uChanSendMask |= (1 << iConnIndex);
                                pVoipCommon->uUserSendMask |= (1 << iLocalUserIndex);
                            }

                            // update receive mask and user-specific playback config
                            if (!bFirstPartyBlocked && !bSocialBlocked && (eMatch & VOIP_CHANRECV))
                            {
                                pVoipCommon->uChanRecvMask |= (1 << iConnIndex);

                                // remote users exist in the connection list before they are registered with voipheadset later in the voip thread
                                // therefore we must make sure the remote user is registered with voip headset before applying voice playback
                                if (VoipHeadsetStatus(pVoipCommon->pHeadset, 'ruvu', 0, &RemoteUser, sizeof(RemoteUser)))
                                {
                                    // enable playback of the remote user's voice for the specified local user
                                    VoipControl(VoipGetRef(), '+pbk', iLocalUserIndex, &RemoteUser);
                                }
                            }
                            else
                            {
                                // remote users exist in the connection list before they are registered with voipheadset later in the voip thread
                                // therefore we must make sure the remote user is registered with voip headset before applying voice playback
                                if (VoipHeadsetStatus(pVoipCommon->pHeadset, 'ruvu', 0, &RemoteUser, sizeof(RemoteUser)))
                                {
                                    // disable playback of the remote user's voice for the specified local user
                                    VoipControl(VoipGetRef(), '-pbk', iLocalUserIndex, &RemoteUser);
                                }
                            }
                        } // if
                    } // for
                } // if
            } // for
        } // if
    } // for

    if (uLastChanSendMask != pVoipCommon->uChanSendMask)
    {
        NetPrintf(("voipcommon: uChanSendMask is now 0x%08x\n", pVoipCommon->uChanSendMask));
        uLastChanSendMask = pVoipCommon->uChanSendMask;
    }

    if (uLastChanRecvMask != pVoipCommon->uChanRecvMask)
    {
        NetPrintf(("voipcommon: uChanRecvMask is now 0x%08x\n", pVoipCommon->uChanRecvMask));
        uLastChanRecvMask = pVoipCommon->uChanRecvMask;
    }

    _VoipCommonComputeEffectiveUserSendMask(pVoipCommon);
    _VoipCommonComputeEffectiveUserRecvMask(pVoipCommon);
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
    // if the default config is on ignore the channel reset on the shared user
    if ((pVoipCommon->bUseDefaultSharedChannelConfig) && (iUserIndex == VOIP_SHARED_USER_INDEX))
    {
        return;
    }

    if ((iUserIndex < 0 || iUserIndex >= VOIP_MAXLOCALUSERS) && iUserIndex != VOIP_SHARED_USER_INDEX)
    {
        NetPrintf(("voipcommon: [channel] warning - attempt to reset invalid local channels index %d\n", iUserIndex));
        return;
    }

    _VoipCommonDecode(VOIPCOMMON_CODEDCHAN_ALL, pVoipCommon->iLocalChannels[iUserIndex], pVoipCommon->uLocalModes[iUserIndex]);

    pVoipCommon->uLocalChannelSelection[iUserIndex] = _VoipCommonEncode(pVoipCommon->iLocalChannels[iUserIndex], pVoipCommon->uLocalModes[iUserIndex]);
    NetPrintf(("voipcommon: [channel] set local channels 0x%08x\n", pVoipCommon->uLocalChannelSelection[iUserIndex]));

    if (VoipGetRef() != NULL)
    {
        _VoipCommonSetChannels(pVoipCommon, pVoipCommon->uLocalChannelSelection[iUserIndex], iUserIndex);
        pVoipCommon->bApplyChannelConfig = TRUE;
    }
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
    NetPrintf(("voipcommon: uUserMicrValue changed from 0x%08x to 0x%08x\n", pVoipCommon->uUserMicrValue, uUserMicrValue));
    pVoipCommon->uUserMicrValue = uUserMicrValue;
    _VoipCommonComputeEffectiveUserSendMask(pVoipCommon);
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
    NetPrintf(("voipcommon: uUserSpkrValue changed from 0x%08x to 0x%08x\n", pVoipCommon->uUserSpkrValue, uUserSpkrValue));
    pVoipCommon->uUserSpkrValue = uUserSpkrValue;
    _VoipCommonComputeEffectiveUserRecvMask(pVoipCommon);
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
    int32_t iRetCode;

    // acquire critical section to modify ConnectionList
    NetCritEnter(&pVoipCommon->ThreadCrit);

    iRetCode = VoipConnectionAddSessionId(&pVoipCommon->Connectionlist, iConnId, uSessionId);

    // release critical section to modify ConnectionList
    NetCritLeave(&pVoipCommon->ThreadCrit);

    return(iRetCode);
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
    int32_t iRetCode;

    // acquire critical section to modify ConnectionList
    NetCritEnter(&pVoipCommon->ThreadCrit);

    iRetCode = VoipConnectionDeleteSessionId(&pVoipCommon->Connectionlist, iConnId, uSessionId);

    // release critical section to modify ConnectionList
    NetCritLeave(&pVoipCommon->ThreadCrit);

    return(iRetCode);
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
    if (pVoipCommon->Connectionlist.pConnections[iLocalConnId].eState != ST_DISC)
    {
        NetPrintf(("voipcommon: mapping local conn id %d to voipserver conn id %d\n", iLocalConnId, iVoipServerConnId));
        pVoipCommon->Connectionlist.pConnections[iLocalConnId].iVoipServerConnId = iVoipServerConnId;
        return(0);
    }
    else
    {
        NetPrintf(("voipcommon: warning - mapping local conn id to voipserver conn id ignored because connection state is ST_DISC\n"));
        return(-1);
    }
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
    if (pVoipCommon->Connectionlist.pConnections[iConnId].bIsLocalClientIdValid)
    {
        NetPrintf(("voipcommon: warning - local client id 0x%08x is being replaced with 0x%08x for conn id %d\n",
            pVoipCommon->Connectionlist.pConnections[iConnId].uLocalClientId, uLocalClientId, iConnId));
    }
    else
    {
        NetPrintf(("voipcommon: assigning local client id 0x%08x to conn id %d\n", uLocalClientId, iConnId));
    }
    pVoipCommon->Connectionlist.pConnections[iConnId].uLocalClientId = uLocalClientId;
    pVoipCommon->Connectionlist.pConnections[iConnId].bIsLocalClientIdValid = TRUE;
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
    // acquire critical section
    NetCritEnter(&pVoipCommon->ThreadCrit);

    if (pCallback == NULL)
    {
        pVoipCommon->pDisplayTranscribedTextCb = NULL;
        pVoipCommon->pDisplayTranscribedTextUserData = NULL;
    }
    else
    {
        pVoipCommon->pDisplayTranscribedTextCb = pCallback;
        pVoipCommon->pDisplayTranscribedTextUserData = pUserData;
    }

    // release critical section
    NetCritLeave(&pVoipCommon->ThreadCrit);
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
    // acquire critical section
    NetCritEnter(&pVoipCommon->ThreadCrit);

    if (pCallback == NULL)
    {
        pVoipCommon->pCallback = _VoipCommonCallback;
        pVoipCommon->pUserData = NULL;
    }
    else
    {
        pVoipCommon->pCallback = pCallback;
        pVoipCommon->pUserData = pUserData;
    }

    // release critical section
    NetCritLeave(&pVoipCommon->ThreadCrit);
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
    // acquire critical section to modify ConnectionList
    NetCritEnter(&pVoipCommon->ThreadCrit);

    // initiate connection
    iConnID = VoipConnectionStart(&pVoipCommon->Connectionlist, iConnID, uAddress, uManglePort, uGamePort, uClientId, uSessionId);

    // release critical section
    NetCritLeave(&pVoipCommon->ThreadCrit);

    // return connection ID to caller
    return(iConnID);
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
    // acquire critical section to modify ConnectionList
    NetCritEnter(&pVoipCommon->ThreadCrit);

    // shut down connection
    VoipConnectionStop(&pVoipCommon->Connectionlist, iConnID, bSendDiscMsg);

    // release critical section
    NetCritLeave(&pVoipCommon->ThreadCrit);
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
    int32_t iRemoteStatus = VOIP_FLAG_INVALID;

    if (pVoipCommon != NULL && pVoipCommon->Connectionlist.pConnections != NULL)
    {
        if (iConnID == VOIP_CONNID_ALL)
        {
            for (iConnID = 0, iRemoteStatus = 0; iConnID < pVoipCommon->Connectionlist.iMaxConnections; iConnID++)
            {
                iRemoteStatus |= (int32_t)pVoipCommon->Connectionlist.pConnections[iConnID].uRemoteUserStatus[iRemoteUserIndex];
            }
        }
        else if ((iConnID >= 0) && (iConnID < pVoipCommon->Connectionlist.iMaxConnections))
        {
            iRemoteStatus = pVoipCommon->Connectionlist.pConnections[iConnID].uRemoteUserStatus[iRemoteUserIndex];
        }
    }

    return(iRemoteStatus);
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
    int32_t iRemoteStatus = VOIP_FLAG_INVALID;

    if (pVoipCommon != NULL && pVoipCommon->Connectionlist.pConnections != NULL)
    {
        if (iConnID == VOIP_CONNID_ALL)
        {
            for (iConnID = 0, iRemoteStatus = 0; iConnID < pVoipCommon->Connectionlist.iMaxConnections; iConnID++)
            {
                iRemoteStatus |= (int32_t)pVoipCommon->Connectionlist.pConnections[iConnID].uRemoteConnStatus;
            }
        }
        else if ((iConnID >= 0) && (iConnID < pVoipCommon->Connectionlist.iMaxConnections))
        {
            iRemoteStatus = pVoipCommon->Connectionlist.pConnections[iConnID].uRemoteConnStatus;
        }
    }

    return(iRemoteStatus);
}
