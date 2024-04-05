/*H********************************************************************************/
/*!
    \File voipconnection.c

    \Description
        VoIP virtual connection manager.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Notes
    \verbatim

    None of the packet types used by DirtySDK's voip implementation is reliable, i.e. delivery to the other end of the connection is not guaranteed.
    There exists some sort of reliability during the connection establishment handshaking: voip connect packets (VoipConnPacketT) are being re-sent
    until packets are detected as received by the other end of the connection. This mechanism is specific to the connection establishment and is
    not generic enough for re-use after connection establishment.

    MLU VoIP join-in-progress (local user added to voip console-to-console connection after connection established) requires some
    post-initial-handshaking reliability. For that purpose specifically, a reliability scheme was implemented with protocol components
    piggy-backed on voip micr packets and voip ping packets. Those protocol components are:  DATA, ACK and ACKCNF.

        DATA   (producer->consumer): data to be delivered reliably (tagged with a sequence number)
        ACK    (consumer->producer): acked sequence number
        ACKCNF (producer->consumer): acked sequence number confirmation (can be leveraged by the consumer to stop “acking” and eliminate the associated “acking” overhead)

    From the perspective of a game console producing reliable data, an "outbound reliable data flow" on a given connection looks like this:

        Reliable data producer (local)                  Reliable Data Consumer (remote)

                                 ------    DATA    ----->
                                <------    ACK     -----
                                 ------   ACKCNF   ----->

    From the perspective of a game console consuming reliable data, an "inbound reliable data flow" on a given connection looks like this:

        Reliable data consumer (local)                  Reliable Data Producer (remote)

                                <------    DATA    -----
                                 ------    ACK     ----->
                                <------   ACKCNF   -----

    ACK and ACKCNF protocol components are added, when necessary, as a pair to the beginning of any ping or voip packet. When the packet
    is flagged with VOIP_PACKET_RELIABLE_FLAG_ACK_ACKCNF, then it contains at least one such entry. The exact number of entries is
    captured in the first byte of the packet payload. Then each entry consists of a 4-byte destination client id and a 1-byte field where:
        bit 7       = ACKCNF (1 means "continue acking", 0 means "you can stop acking")
        bits 0-6    = ACK (acked seq number in the 1 to 127 range - 0 means "unused" or "invalid")

    DATA protocol component is also added at the beginning of the voip packet payload (after ACK+ACkNF if any). When the packet
    is flagged with VOIP_PACKET_RELIABLE_FLAG_DATA, then it contains at least one such entry. The exact number of entries is
    captured in the first byte of the packet payload (or in the first byte following the ACK+ACKCNF portion). When a DATA entry
    is created, it is queued with the SAME sequence number in the outbound queue of all connections. The use of the SAME
    sequence number is key in not having to specifiy what destination console the entry is for. For P2P connectivity this is not
    really a concern as packets are exchanged directly between consoles. But for voipserver-based connectivity, the voip micr
    packets are sent once to the voipserver and then rebroadcasted from there to all other peers. Consequently it is important that
    the sequence number that the DATA entries are tagged with are meaningful to all remote peers.
    For voipserver-based voip:
        Have a look at addition comments in _VoipReliableDataOutProcess() describing the logics used to add DATA entries
        to voip micr packets sent to the voipserver.
    \endverbatim

    \Version 1.0 03/17/2004 (jbrookes)  First Version
    \Version 1.1 01/06/2008 (mclouatre) Created VoipConnectionAddRef(), VoipConnectionRemoveRef(), VoipConnectionResetRef(), VoipConnectionGetFallbackAddr()
    \Version 1.2 10/26/2009 (mclouatre) Renamed from xbox/voipconnection.c to xenon/voipconnectionxenon.c
    \Version 1.3 11/13/2009 (jbrookes)  Merged Xenon back into common version
    \Version 1.4 09/29/2014 (mclouatre) Addes support for voip reliable data
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#ifdef _XBOX
#include <xtl.h>
#include <xonline.h>
#endif

#include <stdio.h>
#include <string.h>
#include <wchar.h>  // for wcslen()

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"

#include "voipconnection.h"

/*** Defines **********************************************************************/

#define VOIP_PACKET_VERSION     ('j')           //!< current packet version
#define VOIP_PING_RATE          (500)           //!< send ping packets twice a second
#define VOIP_TIMEOUT            (15*1000)       //!< default voip data timeout
#define VOIP_CONNTIMEOUT        (10*1000)       //!< connection timeout
#define VOIP_MSPERPACKET        (100)           //!< number of ms per network packet (100ms = 10hz)

//! enable for verbose debugging
#define VOIP_CONNECTION_DEBUG   (DIRTYCODE_DEBUG && FALSE)
#define VOIP_RELIABLE_DEBUG     (DIRTYCODE_DEBUG && FALSE)

#define VOIP_IPPROTO        (IPPROTO_IP)

/*** Macros ***********************************************************************/

//! compare packet types
#define VOIP_SamePacketType(_packetHead1, _packetHead2)  \
    (!memcmp((_packetHead1)->aType, (_packetHead2)->aType, sizeof((_packetHead1)->aType)))

//! get connection ID
#define VOIP_ConnID(_pConnectionlist, _pConnection) ((uint32_t)((_pConnection)-(_pConnectionlist)->pConnections))

/*** Type Definitions *************************************************************/

#if DIRTYCODE_LOGGING
const char *_strReliableType[] =
{
    "USERADD",
    "USERREM",
    "OPAQUE ",
    "TEXT   "
};
#endif

/*** Function Prototypes **********************************************************/

static void _VoipConnectionStop(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, int32_t iConnID, int32_t bSendDiscMsg);
static void _VoipEncodeU16(uint8_t *pValue, uint16_t uValue);
static uint16_t _VoipDecodeU16(const uint8_t *pValue);

/*** Variables ********************************************************************/

//! VoIP conn packet header
static VoipPacketHeadT  _Voip_ConnPacket =
{
    { 'C', 'O', VOIP_PACKET_VERSION }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, {0, 0}
};

//! VoIP disc packet header
static VoipPacketHeadT  _Voip_DiscPacket =
{
    { 'D', 'S', 'C' }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, {0, 0}
};

//! VoIP ping packet header
static VoipPacketHeadT  _Voip_PingPacket =
{
    { 'P', 'N', 'G' }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, {0, 0}
};

//! VoIP mic data packet header
static VoipPacketHeadT  _Voip_MicrPacket =
{
    { 'M', 'I', 'C' }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, {0, 0}
};

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function   _VoipEncodeLocalHeadsetStatus

    \Description
        Encode the uLocalUserStatus fields in the packet header

    \Input *pConnectionlist - information read from here to be written to the packet header
    \Input *pHead           - packet header to write to

    \Version 11/20/2008 (cvienneau)
*/
/********************************************************************************F*/
static void _VoipEncodeLocalHeadsetStatus(VoipConnectionlistT *pConnectionlist, VoipPacketHeadT *pHead)
{
    uint16_t uFlags = 0;
    int32_t i = 0;
    for (i = 0; i < VOIP_MAXLOCALUSERS; ++i)
    {
        if (pConnectionlist->uLocalUserStatus[i] & VOIP_LOCAL_USER_HEADSETOK)
        {
            uFlags |= (1 << i);
        }
    }

    // Encode 16 for byte ordering
    _VoipEncodeU16(pHead->aHeadsetStat, uFlags);
}

/*F********************************************************************************/
/*!
    \Function   _VoipDecodeRemoteHeadsetStatus

    \Description
        Reads aHeadsetStat field from an incoming packet and sets appropriate state 
        about the remote connection.

    \Input *pConnection     - connection to write state to
    \Input *pHead           - packet header to read information from

    \Version 11/20/2008 (cvienneau)
*/
/********************************************************************************F*/
static void _VoipDecodeRemoteHeadsetStatus(VoipConnectionT *pConnection, VoipPacketHeadT *pHead)
{
    uint32_t iIndex;
    uint16_t uFlags = _VoipDecodeU16(pHead->aHeadsetStat);

    for (iIndex = 0; iIndex < VOIP_MAXLOCALUSERS; ++iIndex)
    {
        uint16_t uMask = 1 << iIndex;
        if (uFlags & uMask)
        {
            pConnection->uRemoteUserStatus[iIndex] |= VOIP_REMOTE_USER_HEADSETOK;    //on
        }
        else
        {
            pConnection->uRemoteUserStatus[iIndex] &= ~VOIP_REMOTE_USER_HEADSETOK;   //off
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipDecodeU64

    \Description
        Decode uint64_t from given packet structure.

    \Input *pValue      - network-order data field

    \Output
        uint64_t        - decoded value

    \Version 07/03/2019 (tcho)
*/
/********************************************************************************F*/
static uint64_t _VoipDecodeU64(const uint8_t *pValue)
{
    uint64_t uValue = (uint64_t)pValue[0] << 56;
    uValue |= (uint64_t)pValue[1] << 48;
    uValue |= (uint64_t)pValue[2] << 40;
    uValue |= (uint64_t)pValue[3] << 32;
    uValue |= (uint64_t)pValue[4] << 24;
    uValue |= (uint64_t)pValue[5] << 16;
    uValue |= (uint64_t)pValue[6] << 8;
    uValue |= (uint64_t)pValue[7];
    return(uValue);
}

/*F********************************************************************************/
/*!
    \Function   _VoipDecodeU32

    \Description
        Decode uint32_t from given packet structure.

    \Input *pValue      - network-order data field

    \Output
        uint32_t        - decoded value

    \Version 06/02/2006 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _VoipDecodeU32(const uint8_t *pValue)
{
    uint32_t uValue = pValue[0] << 24;
    uValue |= pValue[1] << 16;
    uValue |= pValue[2] << 8;
    uValue |= pValue[3];
    return(uValue);
}

/*F********************************************************************************/
/*!
    \Function   _VoipDecodeU16

    \Description
        Decode uint16_t from given packet structure.

    \Input *pValue      - network-order data field

    \Output
        uint16_t        - decoded value

    \Version 05/02/2014 (jbrookes)
*/
/********************************************************************************F*/
static uint16_t _VoipDecodeU16(const uint8_t *pValue)
{
    uint16_t uValue = pValue[0] << 8;
    uValue |= pValue[1];
    return(uValue);
}

/*F********************************************************************************/
/*!
    \Function   _VoipDecodeVoipUser

    \Description
        Decode a voip user to a voip user packet

    \Input *pUserPacket     - input voip user packet to decode
    \Input *pUser           - voip user to storge the decoded result

    \Version 07/03/2019 (tcho)
*/
/********************************************************************************F*/
static void _VoipDecodeVoipUser(VoipUserPacketT *pUserPacket, VoipUserT *pUser)
{
    pUser->AccountInfo.iAccountId = (int64_t)_VoipDecodeU64(pUserPacket->aAccountId);
    pUser->AccountInfo.iPersonaId = (int64_t)_VoipDecodeU64(pUserPacket->aPersonaId);
    pUser->uFlags     = _VoipDecodeU32(pUserPacket->aFlags);
    pUser->ePlatform  = _VoipDecodeU32(pUserPacket->aPlatform);
}

/*F********************************************************************************/
/*!
    \Function   _VoipEncodeU64

    \Description
        Encode given uint64_t in network order

    \Input *pValue      - storage for network-order u64
    \Input uValue       - u64 value to encode

    \Version 07/03/2019 (tcho)
*/
/********************************************************************************F*/
static void _VoipEncodeU64(uint8_t *pValue, uint64_t uValue)
{
    pValue[0] = (uint8_t)(uValue >> 56);
    pValue[1] = (uint8_t)(uValue >> 48);
    pValue[2] = (uint8_t)(uValue >> 40);
    pValue[3] = (uint8_t)(uValue >> 32);
    pValue[4] = (uint8_t)(uValue >> 24);
    pValue[5] = (uint8_t)(uValue >> 16);
    pValue[6] = (uint8_t)(uValue >> 8);
    pValue[7] = (uint8_t)(uValue);
}

/*F********************************************************************************/
/*!
    \Function   _VoipEncodeU32

    \Description
        Encode given uint32_t in network order

    \Input *pValue      - storage for network-order u32
    \Input uValue       - u32 value to encode

    \Version 06/02/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipEncodeU32(uint8_t *pValue, uint32_t uValue)
{
    pValue[0] = (uint8_t)(uValue >> 24);
    pValue[1] = (uint8_t)(uValue >> 16);
    pValue[2] = (uint8_t)(uValue >> 8);
    pValue[3] = (uint8_t)uValue;
}

/*F********************************************************************************/
/*!
    \Function   _VoipEncodeU16

    \Description
        Encode given uint16_t in network order

    \Input *pValue      - storage for network-order u32
    \Input uValue       - u16 value to encode

    \Version 05/02/2014 (amakoukji)
*/
/********************************************************************************F*/
static void _VoipEncodeU16(uint8_t *pValue, uint16_t uValue)
{
    pValue[0] = (uint8_t)(uValue >> 8);
    pValue[1] = (uint8_t)uValue;
}

/*F********************************************************************************/
/*!
    \Function   _VoipEncodeVoipUser

    \Description
        Encodes a voip user to a voip user packet

    \Input *pUserPacket     - output voip user packet
    \Input *pUser           - user to encode

    \Version 07/03/2019 (tcho)
*/
/********************************************************************************F*/
static void _VoipEncodeVoipUser(VoipUserPacketT *pUserPacket, VoipUserT *pUser)
{
    _VoipEncodeU64(pUserPacket->aAccountId, (uint64_t) pUser->AccountInfo.iAccountId);
    _VoipEncodeU64(pUserPacket->aPersonaId, (uint64_t) pUser->AccountInfo.iPersonaId);
    _VoipEncodeU32(pUserPacket->aFlags,     pUser->uFlags);
    _VoipEncodeU32(pUserPacket->aPlatform,  pUser->ePlatform);
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionIncrementReliableSeqNb

    \Description
        Return the next seq nb following the specified seq nb.
        Sequence number validity range: [1,127]  (0 = invalid)

    \Input uSeqNb   - input sequence number

    \Output
        uint8_t     - output sequence number

    \Version 09/18/2014 (mclouatre)
*/
/********************************************************************************F*/
static uint8_t _VoipConnectionIncrementReliableSeqNb(uint8_t uSeqNb)
{
    if (uSeqNb == 127)
    {
        return(1);
    }
    else
    {
        return(uSeqNb + 1);
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipReliableDataEnqueue

    \Description
        Add a reliable data entry to the tail of the specified linked list.

    \Input **pListHead  - pointe to head of list
    \Input *pNewEntry   - entry to be added

    \Version 09/14/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipReliableDataEnqueue(LinkedReliableDataT **pListHead, LinkedReliableDataT *pNewEntry)
{
    if (*pListHead == NULL)
    {
        *pListHead = pNewEntry;
    }
    else
    {
        // find tail of list and append
        LinkedReliableDataT *pCurrent = *pListHead;
        while (pCurrent->pNext != NULL)
        {
            pCurrent = pCurrent->pNext;
        }
        pCurrent->pNext = pNewEntry;
    }

    pNewEntry->pNext = NULL;
}

/*F********************************************************************************/
/*!
    \Function   _VoipReliableDataDequeue

    \Description
        Remove a reliable data entry from the tail of the specified linked list.

    \Input **pListHead          - pointer to head of list

    \Output
        LinkedReliableDataT *   - pointer to returned buffer; NULL if error

    \Version 09/14/2014 (mclouatre)
*/
/********************************************************************************F*/
static LinkedReliableDataT *_VoipReliableDataDequeue(LinkedReliableDataT **pListHead)
{
    LinkedReliableDataT *pRemovedEntry = NULL;

    // return head of list to caller
    if (*pListHead)
    {
        pRemovedEntry = *pListHead;
        *pListHead = (*pListHead)->pNext;
        pRemovedEntry->pNext = NULL;
    }

    return(pRemovedEntry);
}

/*F********************************************************************************/
/*!
    \Function   _VoipGetReliableDataBufferFromFreePool

    \Description
        Obtain a free reliable data buffer from the free pool.
        (Extract from head of linked list)

    \Input *pConnectionlist     - connection list
    \Input bAllocIfEmpty        - enable/disable allocating buffer in pool is empty

    \Output
        LinkedReliableDataT *   - pointer to returned buffer; NULL if error

    \Version 09/14/2014 (mclouatre)
*/
/********************************************************************************F*/
static LinkedReliableDataT *_VoipGetReliableDataBufferFromFreePool(VoipConnectionlistT *pConnectionlist, uint8_t bAllocIfEmpty)
{
    LinkedReliableDataT *pReliableDataBuffer;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // query current mem group data
    VoipCommonMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // get a buffer from the pool of free buffer
    pReliableDataBuffer = _VoipReliableDataDequeue(&pConnectionlist->pFreeReliableDataPool);

    // if pool is empty, allocate a new entry
    if ((pReliableDataBuffer == NULL) && (bAllocIfEmpty != FALSE))
    {
        if ((pReliableDataBuffer = (LinkedReliableDataT *)DirtyMemAlloc(sizeof(*pReliableDataBuffer), VOIP_MEMID, iMemGroup, pMemGroupUserData)) != NULL)
        {
            ds_memclr(pReliableDataBuffer, sizeof(*pReliableDataBuffer));

            #if VOIP_RELIABLE_DEBUG
            NetPrintf(("voipconnection: new free reliable data buffer (%p) added to free pool\n", pReliableDataBuffer));
            #endif
        }
        else
        {
            NetPrintf(("voipconnection: error, unable to allocate reliable data buffer\n"));
        }
    }

    #if VOIP_RELIABLE_DEBUG
    if (pReliableDataBuffer)
    {
        NetPrintf(("voipconnection: reliable data buffer (%p) obtained from free pool\n", pReliableDataBuffer));
    }
    else
    {
        NetPrintf(("voipconnection: free pool of reliable data buffers is now empty\n"));
    }
    #endif

    return(pReliableDataBuffer);
}

/*F********************************************************************************/
/*!
    \Function   _VoipReturnReliableDataBufferToFreePool

    \Description
        Return buffer to free pool. (Append to tail of linked list)

    \Input *pConnectionlist     - connection list
    \Input *pFreeBuffer         - buffer to be returned to pool

    \Version 09/14/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipReturnReliableDataBufferToFreePool(VoipConnectionlistT *pConnectionlist, LinkedReliableDataT *pFreeBuffer)
{
    _VoipReliableDataEnqueue(&pConnectionlist->pFreeReliableDataPool, pFreeBuffer);

    #if VOIP_RELIABLE_DEBUG
    NetPrintf(("voipconnection: reliable data buffer (%p) returned to free pool\n", pFreeBuffer));
    #endif
}

/*F********************************************************************************/
/*!
    \Function   _VoipPacketGetWritePtr

    \Description
        Calculate next write position in the specified voip packet

    \Input *pMicrPacket     - packet pointer

    \Output
        uint8_t *           - next write position in the packet

    \Version 07/18/2013 (mclouatre)
*/
/********************************************************************************F*/
static uint8_t *_VoipPacketGetWritePtr(VoipMicrPacketT *pMicrPacket)
{
    uint8_t *pWrite = &pMicrPacket->aData[0];

    // is the packet buffer already filled with ACK+ACKCNF protocol components used for the reliability mechanism?
    if (pMicrPacket->Head.uFlags & VOIP_PACKET_RELIABLE_FLAG_ACK_ACKCNF)
    {
        int32_t iReliableAckEntriesCount = *pWrite++;
        pWrite += (iReliableAckEntriesCount * sizeof(ReliableAckT));
    }

    // is the packet buffer already filled with the DATA protocol component used for the reliability mechanism?
    if (pMicrPacket->Head.uFlags & VOIP_PACKET_RELIABLE_FLAG_DATA)
    {
        int32_t iReliableDataEntriesCount = *pWrite++;
        int32_t iReliableDataEntryIndex;

        for (iReliableDataEntryIndex = 0; iReliableDataEntryIndex < iReliableDataEntriesCount; iReliableDataEntryIndex++)
        {
            ReliableDataT *pCurrent = (ReliableDataT *)pWrite;
            uint32_t uSize = _VoipDecodeU16(&pCurrent->info.uSize[0]);

            pWrite += sizeof(pCurrent->info) + uSize;
        }
    }

    // is the packet buffer already filled with some metadata?
    if (pMicrPacket->Head.uFlags & VOIP_PACKET_STATUS_FLAG_METADATA)
    {
        int32_t iMetaDataSize;
        iMetaDataSize = *pWrite++;
        pWrite += iMetaDataSize;
    }

    // is the packet buffer already filled with some sub-pkts?
    if (pMicrPacket->MicrInfo.uNumSubPackets != 0)
    {
        uint8_t bVariableSubPktLen = ((pMicrPacket->MicrInfo.uSubPacketSize == 0xFF) ? TRUE : FALSE);   // check if sub-pkts in this packet are fixed length or variable length

        // if necessary, jump over sub-pkts already packed in the packet buffer
        if (bVariableSubPktLen)
        {
            int32_t iSubPktIndex, iSubPacketSize;

            // find where we need to write; each sub-packet is prepended (one byte) with its length
            for (iSubPktIndex = 0; iSubPktIndex < pMicrPacket->MicrInfo.uNumSubPackets; iSubPktIndex++)
            {
                iSubPacketSize = *pWrite;
                pWrite += (iSubPacketSize + 1);
            }
        }
        else
        {
            pWrite += (pMicrPacket->MicrInfo.uNumSubPackets * pMicrPacket->MicrInfo.uSubPacketSize);
        }
    }

    return(pWrite);
}


/*F********************************************************************************/
/*!
    \Function   _VoipPacketGetMetaDataPtr

    \Description
        Find position of metadata in the specified voip packet

    \Input *pMicrPacket     - packet pointer

    \Output
        uint8_t *           - ptr to metadata

    \Version 09/25/2014 (mclouatre)
*/
/********************************************************************************F*/
static uint8_t *_VoipPacketGetMetaDataPtr(VoipMicrPacketT *pMicrPacket)
{
    uint8_t *pMetaData = &pMicrPacket->aData[0];

    // is the packet buffer already filled with ACK+ACKCNF protocol components used for the reliability mechanism?
    if (pMicrPacket->Head.uFlags & VOIP_PACKET_RELIABLE_FLAG_ACK_ACKCNF)
    {
        int32_t iReliableAckEntriesCount = *pMetaData++;
        pMetaData += (iReliableAckEntriesCount * sizeof(ReliableAckT));
    }

    // is the packet buffer already filled with the DATA protocol component used for the reliability mechanism?
    if (pMicrPacket->Head.uFlags & VOIP_PACKET_RELIABLE_FLAG_DATA)
    {
        int32_t iReliableDataEntriesCount = *pMetaData++;
        int32_t iReliableDataEntryIndex;

        for (iReliableDataEntryIndex = 0; iReliableDataEntryIndex < iReliableDataEntriesCount; iReliableDataEntryIndex++)
        {
            ReliableDataT *pCurrent = (ReliableDataT *)pMetaData;
            uint32_t uSize = _VoipDecodeU16(&pCurrent->info.uSize[0]);

            pMetaData += sizeof(pCurrent->info) + uSize;
        }
    }

    return(pMetaData);
}

/*F********************************************************************************/
/*!
    \Function   _VoipPrepareVoipServerSendMask

    \Description
        Convert local send mask to voip-server ready send mask

    \Input *pConnectionlist - connection list

    \Output
        uint32_t            - send mask to be packed in voip packet sent to voip server

    \Version 10/27/2006 (mclouatre)
*/
/********************************************************************************F*/
static uint32_t _VoipPrepareVoipServerSendMask(VoipConnectionlistT *pConnectionlist)
{
    int32_t iLocalConnId, iVoipServerConnId;
    uint32_t uLocalSendMask = pConnectionlist->uSendMask;
    uint32_t uVoipServerSendMask = 0;

    for (iLocalConnId = 0; iLocalConnId < pConnectionlist->iMaxConnections; iLocalConnId++)
    {
        iVoipServerConnId = pConnectionlist->pConnections[iLocalConnId].iVoipServerConnId;

        // make sure connection is used by voip server
        if (iVoipServerConnId != VOIP_CONNID_NONE)
        {
            // make sure local connection has send bit set
            if (uLocalSendMask & (1 << iLocalConnId))
            {
                // set the proper bit in the send mask for the voip server
                uVoipServerSendMask |= (1 << iVoipServerConnId);
            }
        }
    }

    return(uVoipServerSendMask);
}


/*F********************************************************************************/
/*!
    \Function   _VoipConnectionIsTranscribedTextRequested

    \Description
        Returns TRUE if at least one local user is requesting text 
        transcription from remote talkers.

    \Input  *pConnectionlist    - pointer to connectionlist

    \Output
        uint32_t                - TRUE or FALSE

    \Version 05/05/2017 (mclouatre)
*/
/********************************************************************************F*/
static uint32_t _VoipConnectionIsTranscribedTextRequested(VoipConnectionlistT *pConnectionlist)
{

    int32_t iUserIndex;

    for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS; iUserIndex++)
    {
        if (pConnectionlist->aIsParticipating[iUserIndex] != FALSE)
        {
            if (pConnectionlist->bTranscribedTextRequested[iUserIndex] != FALSE)
            {
                return(TRUE);
            }
        }
    }

    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionAllocate

    \Description
        Allocate an empty connection for use.

    \Input  *pConnectionlist    - pointer to connectionlist
    \Input  iConnID             - connection to allocate (or VOIP_CONNID_*)

    \Output
        VoipConnectionT *       - pointer to connection to use or NULL

    \Version 03/17/2004 (jbrookes)
*/
/********************************************************************************F*/
static VoipConnectionT *_VoipConnectionAllocate(VoipConnectionlistT *pConnectionlist, int32_t iConnID)
{
    VoipConnectionT *pConnection = NULL;

    // get a connection ID
    pConnection = NULL;
    if (iConnID == VOIP_CONNID_NONE)
    {
        // find a free connection slot
        for (iConnID = 0; iConnID < pConnectionlist->iMaxConnections; iConnID++)
        {
            // if this is an unallocated connection
            if (pConnectionlist->pConnections[iConnID].eState == ST_DISC)
            {
                pConnection = &pConnectionlist->pConnections[iConnID];
                break;
            }
        }

        // make sure we found a connection
        if (pConnection == NULL)
        {
            NetPrintf(("voipconnection: out of connections\n"));
        }
    }
    else if ((iConnID >= 0) && (iConnID < pConnectionlist->iMaxConnections))
    {
        // if we're currently connected, complain
        if (pConnectionlist->pConnections[iConnID].eState != ST_DISC)
        {
            NetPrintf(("voipconnection: [%d] connection not available, currently used for clientId=%d\n", iConnID, pConnectionlist->pConnections[iConnID].uRemoteClientId));
            return(NULL);
        }

        // ref connection
        pConnection = &pConnectionlist->pConnections[iConnID];
    }
    else
    {
        NetPrintf(("voipconnection: %d is an invalid connection id\n", iConnID));
    }

    // return connection ref to caller
    return(pConnection);
}

/*F********************************************************************************/
/*!
    \Function   _VoipSocketSendto

    \Description
        Send data with the given socket.

    \Input *pSocket     - socket to send on
    \Input uClientId    - client identifier
    \Input *pBuf        - data to send
    \Input iLen         - size of data to send
    \Input iFlags       - send flags
    \Input *pTo         - address to send data to
    \Input iToLen       - length of address structure

    \Output
        int32_t         - amount of data sent

    \Version 11/30/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipSocketSendto(SocketT *pSocket, uint32_t uClientId, VoipPacketHeadT *pBuf, int32_t iLen, int32_t iFlags, struct sockaddr *pTo, int32_t iToLen)
{
    // make sure socket exists
    if (pSocket == NULL)
    {
        return(0);
    }

    // set connection identifier (goes in all packets)
    _VoipEncodeU32(pBuf->aClientId, uClientId);

    // send the packet
    return(SocketSendto(pSocket, (char *)pBuf, iLen, iFlags, pTo, iToLen));
}


/*F********************************************************************************/
/*!
    \Function _VoipConnectionMatchSessionId

    \Description
        Check whether specified session ID matchs the voip connection.

    \Input *pConnectionlist - connectionlist
    \Input iConnId          - connection ID
    \Input uSessionId       - session ID

    \Output
        uint8_t             - TRUE for match, FALSE for no match

    \Version 08/30/2011 (mclouatre)
*/
/********************************************************************************F*/
static uint8_t _VoipConnectionMatchSessionId(VoipConnectionlistT *pConnectionlist, int32_t iConnId, uint32_t uSessionId)
{
    int32_t iSessionIndex;
    int32_t bMatch = FALSE;
    VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnId];

    NetCritEnter(&pConnectionlist->NetCrit);

    for (iSessionIndex = 0; iSessionIndex < VOIP_MAXSESSIONIDS; iSessionIndex++)
    {
        if (pConnection->uSessionId[iSessionIndex] == uSessionId)
        {
            bMatch = TRUE;
        }
    }

    NetCritLeave(&pConnectionlist->NetCrit);

    return(bMatch);
}

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function   _VoipConnectionPrintRemoteUsers

    \Description
        Print remote users

    \Input *pConnection     - connection to transition
    \Input iConnID          - index of connection

    \Version 03/11/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipConnectionPrintRemoteUsers(VoipConnectionT *pConnection, int32_t iConnID)
{
    int32_t iIndex;

    for (iIndex = 0; iIndex < pConnection->iMaxRemoteUsers; iIndex++)
    {
        if (!VOIP_NullUser((VoipUserT *)(&pConnection->RemoteUsers[iIndex])))
        {
            NetPrintf(("voipconnection: [%d]     remote user #%d --> %lld\n", iConnID, iIndex, ((VoipUserT *)&pConnection->RemoteUsers[iIndex])->AccountInfo.iPersonaId));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionPrintSessionIds

    \Description
        Prints all session IDs sharing this voip connection

    \Input *pConnectionlist - connectionlist
    \Input iConnId          - connection id

    \Version 08/30/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipConnectionPrintSessionIds(VoipConnectionlistT *pConnectionlist, int32_t iConnId)
{
    int32_t iSessionIndex;
    VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnId];

    NetCritEnter(&pConnectionlist->NetCrit);

    if (pConnection->uSessionId[0] != 0)
    {
        NetPrintf(("voipconnection: [%d] IDs of sessions sharing this voip connection are: ", iConnId));
        for (iSessionIndex = 0; iSessionIndex < VOIP_MAXSESSIONIDS; iSessionIndex++)
        {
            if (pConnection->uSessionId[iSessionIndex] != 0)
            {
                NetPrintf(("0x%08x\n", pConnection->uSessionId[iSessionIndex]));
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        NetPrintf(("voipconnection: [%d] there is no session ID associated with this connection\n", iConnId));
    }

    NetCritLeave(&pConnectionlist->NetCrit);
}
#endif  // DIRTYCODE_LOGGING

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionReliableDataEnqueue

    \Description
        Add a reliable data entry to the outbound queue of the
        specified connection. (Append to tail of linked list)

    \Input *pConnectionlist     - connection list
    \Input iConnID              - connection identifier (VOIP_CONNID_ALL for all)
    \Input *pReliableData       - pointer to reliable data to be enqueued

    \Version 09/14/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipConnectionReliableDataEnqueue(VoipConnectionlistT *pConnectionlist, int32_t iConnID, ReliableDataT *pReliableData)
{
    int32_t iConnectionIndex;
    uint32_t uCurrentTick = NetTick();

    if (iConnID == VOIP_CONNID_ALL)
    {
        iConnectionIndex = 0;
    }
    else
    {
        iConnectionIndex = iConnID;
    }

    while (iConnectionIndex < pConnectionlist->iMaxConnections)
    {
        VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnectionIndex];

        if (pConnection->eState == ST_ACTV || pConnection->eState == ST_CONN)
        {
            LinkedReliableDataT *pNewEntry = _VoipGetReliableDataBufferFromFreePool(pConnectionlist, TRUE);

            if (pNewEntry)
            {
                // fill with input data
                ds_memcpy_s(&pNewEntry->data, sizeof(pNewEntry->data), pReliableData, sizeof(*pReliableData));

                // fill sequence number
                pNewEntry->data.info.uSeq = pConnection->uOutSeqNb;
                pConnection->uOutSeqNb = _VoipConnectionIncrementReliableSeqNb(pConnection->uOutSeqNb);

                // set remote peer id
                _VoipEncodeU32(&pNewEntry->data.info.aRemoteClientId[0], pConnection->uRemoteClientId);

                // fill enqueue tick
                pNewEntry->uEnqueueTick = uCurrentTick;

                // add to oubtound queue
                _VoipReliableDataEnqueue(&pConnection->pOutboundReliableDataQueue, pNewEntry);

                #if VOIP_RELIABLE_DEBUG
                NetPrintf(("voipconnection: [%d] reliable data buffer (%p) added to outbound queue (seq# %03d, %s, tick=%u)\n", iConnectionIndex, pNewEntry,
                    pNewEntry->data.info.uSeq, _strReliableType[pNewEntry->data.info.uType], pNewEntry->uEnqueueTick));
                #endif

                /*
                In the case of voipserver-based topology, to improve responsiveness and increase chances of reliable data
                making it quick to other end, we alter the last send tick on the connection. This should result in a
                ping packets being sent immediately, and help with reliable data reaching other end for voip connections 
                that see their voip traffic squelched by the voip server.
                */
                if (pConnection->iVoipServerConnId != VOIP_CONNID_NONE)
                {
                    #if VOIP_RELIABLE_DEBUG
                    NetPrintf(("voipconnection: [%d] last send tick altered to speed up sending of reliable data via ping packet\n", iConnectionIndex));
                    #endif
                    pConnection->uLastSend = pConnection->uLastSend - VOIP_PING_RATE;
                }
            }
            else
            {
                NetPrintf(("voipconnection: [%d] failed to obtain resources for enqueueing new outbound reliable data\n", iConnectionIndex));
            }
        }

        // break out of while loop if we are dealing with a single connection only
        if (iConnID == VOIP_CONNID_ALL)
        {
            iConnectionIndex++;
        }
        else
        {
            break;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionReliableDataDequeue

    \Description
        Remove reliable data entry from the outbound queue of the
        specified connection. (Remove entry at head of linked list)

    \Input *pConnectionlist     - connection list
    \Input iConnID              - connection identifier (VOIP_CONNID_ALL for all)

    \Version 09/14/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipConnectionReliableDataDequeue(VoipConnectionlistT *pConnectionlist, int32_t iConnID)
{
    LinkedReliableDataT *pRemovedEntry = _VoipReliableDataDequeue(&pConnectionlist->pConnections[iConnID].pOutboundReliableDataQueue);

    #if VOIP_RELIABLE_DEBUG
    NetPrintf(("voipconnection: [%d] reliable data buffer (%p) removed from outbound queue (seq# %03d, %s)\n", iConnID, pRemovedEntry,
        pRemovedEntry->data.info.uSeq, _strReliableType[pRemovedEntry->data.info.uType]));
    #endif

   _VoipReturnReliableDataBufferToFreePool(pConnectionlist, pRemovedEntry);
}

/*F********************************************************************************/
/*!
    \Function   _VoipReliableDataPackACKandACKCNF

    \Description
        Outbound processing of reliablity data - deals with ACK+ACKCNF protocol
        components specifically.

    \Input *pConnectionlist - connectionlist ref
    \Input iConnID          - id of connection to be acked; VOIP_CONNID_ALL to ack all connections
    \Input *pFlags          - pointer to uFlags field of the outbound packet
    \Input bPing            - TRUE if packet is a ping packet, FALSE if packet is a micr packet
    \Input *pWrite          - pointer to next byte to be written in packet payload
    \Input iSpaceLeftInPkt  - amount of space left in packet payload

    \Output
        uint8_t *           - pointer to first byte after the newly added ACK

    \Version 09/14/2014 (mclouatre)
*/
/********************************************************************************F*/
static uint8_t * _VoipReliableDataPackACKandACKCNF(VoipConnectionlistT *pConnectionlist, int32_t iConnID, uint8_t *pFlags, uint32_t bPing, uint8_t *pWrite, int32_t iSpaceLeftInPkt)
{
    uint8_t uEntryCount = 0;
    uint8_t *pEntryCount = pWrite; // remember where the entry count needs to be written if any entry added to packet buffer
    uint32_t uSpaceLeftInPkt = (uint32_t)iSpaceLeftInPkt; // amount of space still available in packet for reliable data
    int32_t iConnectionIndex;

    // skip the byte where the entry count is supposed to be written
    pWrite++;

    if (iConnID == VOIP_CONNID_ALL)
    {
        iConnectionIndex = 0;
    }
    else
    {
        iConnectionIndex = iConnID;
    }

    while (iConnectionIndex < pConnectionlist->iMaxConnections)
    {
        VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnectionIndex];
        uint32_t bAckOrAckCnfNeeded = FALSE;

        // if our outbound queue is not empty or if the peer at the remote end of the connection is still acking us, then
        // add ACK+ACKCNF protocol component to packet.
        if ((pConnectionlist->pConnections[iConnectionIndex].pOutboundReliableDataQueue != NULL) || pConnection->bPeerIsAcking)
        {
            bAckOrAckCnfNeeded = TRUE;
        }

        // make sure there is enough space left in destination packet buffer
        if (sizeof(ReliableAckT) < uSpaceLeftInPkt)
        {
            // do we need to send ACK or ACKCNF to this remote client
            if (pConnection->bPeerNeedsAck || bAckOrAckCnfNeeded)
            {
                ReliableAckT ackEntry;

                // set remote peer id
                _VoipEncodeU32(&ackEntry.aRemoteClientId[0], pConnection->uRemoteClientId);

                // Fill in ACK value
                if (pConnection->bPeerNeedsAck)
                {
                    /* validty range: [1,127]
                       In a scenario where we have no yet received inbound reliable traffic from remote peer, pConnection->uInSeqNb
                       is 0 and we end up initializing ackEntry.uAckedSeq with 0 which means 'ignore this field, no sequence number acked' */
                    ackEntry.uAckedSeq = pConnection->uInSeqNb;
                }
                else
                {
                    // means: 'ignore this field, no sequence number acked'
                    ackEntry.uAckedSeq = 0;
                }

                // set ACKCNF
                if (bAckOrAckCnfNeeded)
                {
                    // if our outbound queue is not empty, flag that we are still expecting acks back
                    if (pConnection->pOutboundReliableDataQueue != NULL)
                    {
                        ackEntry.uAckedSeq |= VOIP_RELIABLE_ACKCNF_MASK;
                    }
                }

                // add entry in the packet
                ds_memcpy_s(pWrite, uSpaceLeftInPkt, &ackEntry, sizeof(ackEntry));
                pWrite += sizeof(ackEntry);

                // removed space used from space left
                uSpaceLeftInPkt -= sizeof(ackEntry);

                // increment entry count
                uEntryCount += 1;

                // flag that packet contains at least one ACK+ACKCNF protocol component entry
                *pFlags |=  VOIP_PACKET_RELIABLE_FLAG_ACK_ACKCNF;

                #if VOIP_RELIABLE_DEBUG
                if (pConnection->bPeerNeedsAck)
                {
                     NetPrintf(("voipconnection: [%d] reliable ACK+ACKCNF (acked seq# %03d, %s) added to outbound %s packet for clientId=0x%08x\n", iConnectionIndex,
                        (ackEntry.uAckedSeq & 0x7F), ((pConnection->pOutboundReliableDataQueue != NULL)?"SendAck":"HaltAck"), (bPing?"PNG":"MIC"), pConnection->uRemoteClientId));
                }
                else
                {
                     NetPrintf(("voipconnection: [%d] reliable ACK+ACKCNF (acked seq# INV, %s) added to outbound %s packet for clientId=0x%08x\n", iConnectionIndex,
                        ((pConnection->pOutboundReliableDataQueue != NULL) ?"SendAck":"HaltAck"), (bPing?"PNG":"MIC"), pConnection->uRemoteClientId));
                }
                #endif
            }
        }
        else
        {
            NetPrintf(("voipconnection: warning - failed to fit all ACK entries in outbound %s packet\n", (bPing?"PNG":"MIC")));
            break;    // no space left in packet, force while loop to exit
        }

        // break out of while loop if we are dealing with a single connection only
        if (iConnID == VOIP_CONNID_ALL)
        {
            iConnectionIndex++;
        }
        else
        {
            break;
        }
    }

    if (uEntryCount == 0)
    {
        // reposition pWrite to its initial value 
        pWrite = pEntryCount;
    }
    else
    {
        // write the entry count in the first byte that the input pWrite was pointing to
        *pEntryCount = uEntryCount;
    }

    return(pWrite);
}

/*F********************************************************************************/
/*!
    \Function   _VoipReliableDataPackDATA

    \Description
        Outbound processing of reliablity data - deals with DATA protocol component specifically.

    \Input *pConnectionlist - connectionlist ref
    \Input iConnID          - id of connection on which the outbound packet is being sent; CANNOT be VOIP_CONNID_ALL
    \Input *pFlags          - pointer to uFlags field of the outbound packet
    \Input bPing            - TRUE if packet is a ping packet, FALSE if packet is a micr packet
    \Input *pWrite          - pointer to next byte to be written in packet payload
    \Input iSpaceLeftInPkt  - amount of space left in packet payload

    \Output
        uint8_t *           - pointer to first byte after the newly added DATA

    \Version 09/14/2014 (mclouatre)
*/
/********************************************************************************F*/
static uint8_t * _VoipReliableDataPackDATA(VoipConnectionlistT *pConnectionlist, int32_t iConnID, uint8_t *pFlags, uint32_t bPing, uint8_t *pWrite, int32_t iSpaceLeftInPkt)
{
    uint8_t uEntryCount = 0;
    uint8_t *pEntryCount = pWrite;  // remember where the entry count needs to be written if any entry added to packet buffer
    uint32_t uSpaceLeftInPkt = (uint32_t)iSpaceLeftInPkt;  // amount of space still available in packet for reliable data
    VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnID];
    LinkedReliableDataT *pCurrent = pConnection->pOutboundReliableDataQueue;

    // skip the byte where the entry count is supposed to be written
    pWrite++;

    // find how many entries from the outbound reliable data queue fit in the outbound packet
    while (pCurrent != NULL)
    {
        uint16_t uSize = _VoipDecodeU16(&pCurrent->data.info.uSize[0]);

        if (uSize < uSpaceLeftInPkt)
        {
            // pack reliable data info
            ds_memcpy(pWrite, &pCurrent->data.info, sizeof(pCurrent->data.info));
            pWrite += sizeof(pCurrent->data.info);

            // pack reliable data payload
            ds_memcpy(pWrite, pCurrent->data.aData, uSize);
            pWrite += uSize;

            // removed space used from space left
            uSpaceLeftInPkt -= uSize;

            // increment the entry count in the packet
            uEntryCount += 1;

            // flag that packet contains at least one DATA protocol component entry
            *pFlags |= VOIP_PACKET_RELIABLE_FLAG_DATA;

            #if VOIP_RELIABLE_DEBUG
            {
                int32_t uClientId = _VoipDecodeU32(&pCurrent->data.info.aRemoteClientId[0]);
                NetPrintf(("voipconnection: [%d] reliable DATA (target: 0x%08x, seq# %03d, %s, %d bytes) added to outbound %s packet\n", iConnID, uClientId, pCurrent->data.info.uSeq,
                    _strReliableType[pCurrent->data.info.uType], uSize, (bPing?"PNG":"MIC")));
            }
            #endif

            // move to next pending entry
            pCurrent = pCurrent->pNext;
        }
        else
        {
            NetPrintf(("voipconnection: [%d] warning - failed to fit all DATA entries in outbound %s packet (last entry size = %d)\n",
                iConnID, (bPing?"PNG":"MIC"), uSize));
            pCurrent = NULL;    // no space left in packet, force while loop to exit
        }
    }

    if (uEntryCount == 0)
    {
        // reposition pWrite to its initial value 
        pWrite = pEntryCount;
    }
    else
    {
        // write the entry count in the first byte that the input pWrite was pointing to
        *pEntryCount = uEntryCount;
    }

    return(pWrite);
}

/*F********************************************************************************/
/*!
    \Function   _VoipReliableDataOutProcess

    \Description
        Outbound processing of reliablity data. To be invoked every time a
        ping packet or a micr packet is sent. Reliability protocol components
        (ACK, ACKCNF, DATA) are going to be added to the packet if necessary.

    \Input *pConnectionlist - connectionlist ref
    \Input iConnID          - connection id; VOIP_CONNID_ALL if packet is being sent to voipserver
    \Input *pPacketHeader   - pointer to header of the outbound packet
    \Input *pPacketPayload  - pointer to first payload byte of outbound packet
    \Input iPayloadCapacity - amount of bytes we can fit in the payload

    \Output
        uint8_t *           - pointer to first byte after reliable data in packet payload

    \Version 09/14/2014 (mclouatre)
*/
/********************************************************************************F*/
static uint8_t * _VoipReliableDataOutProcess(VoipConnectionlistT *pConnectionlist, int32_t iConnID, VoipPacketHeadT *pPacketHeader, uint8_t *pPacketPayload, int32_t iPayloadCapacity)
{
    int32_t iSpaceLeftInPkt = iPayloadCapacity;
    uint32_t bPing = ((pPacketHeader->aType[0] == 'P' && pPacketHeader->aType[1] == 'N' && pPacketHeader->aType[2] == 'G') ? TRUE : FALSE);
    uint8_t *pWrite;

    // add ACK and ACKCNF to packet as necessary
    pWrite = _VoipReliableDataPackACKandACKCNF(pConnectionlist, iConnID, &pPacketHeader->uFlags, bPing, pPacketPayload, iSpaceLeftInPkt);
    iSpaceLeftInPkt -= (pWrite - pPacketPayload);

    if (iConnID == VOIP_CONNID_ALL)
    {
        int32_t iConnectionIndex;
        int32_t iLargestTickDiff = 0;

        /*
        When voip traffic flows through the voip server, we need to pick a local voip connection
        to pack the outbound reliable data entries from. Each connection have its own outbound
        reliable queue. They are all populated with same entries tagged with same sequence numbers.
        But the acking progress on each queue may progress differently. For instance, queue X may
        have entry seq nb 10 acked, but queue Y may still be waiting for entry seq nb 8 to be acked.

        The best we can do is pick the connection that has the oldest pending reliable data. Other
        connections will benefit of this too if there are still waiting for ack of that entry, or 
        they will rely on the next ping packet to go out. 
        */

        for (iConnectionIndex = 0; iConnectionIndex < pConnectionlist->iMaxConnections; iConnectionIndex++)
        {
            VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnectionIndex];

            if ((pConnection->eState == ST_ACTV) && (pConnection->pOutboundReliableDataQueue != NULL))
            {
                int32_t iTickDiff = NetTickDiff(NetTick(), pConnection->pOutboundReliableDataQueue->uEnqueueTick);

                if (iLargestTickDiff < iTickDiff)
                {
                    iLargestTickDiff = iTickDiff;
                    iConnID = iConnectionIndex;
                }
            }
        }
        // if above for loop did not match on any connection, fallback to connection index 0
        if (iConnID == VOIP_CONNID_ALL)
        {
            iConnID = 0;
        }
    }

    pWrite = _VoipReliableDataPackDATA(pConnectionlist, iConnID, &pPacketHeader->uFlags, bPing, pWrite, iSpaceLeftInPkt);

    return(pWrite);
}

/*F********************************************************************************/
/*!
    \Function   _VoipReliableDataInboundUserAddProcess

    \Description
        Inbound processing of reliablity data of type VOIP_RELIABLE_TYPE_USERADD.

    \Input *pConnectionlist - connectionlist ref
    \Input *pConnection     - connection on which the packet was received
    \Input *pReliableData   - pointer to inbound reliable data entry

    \Version 09/14/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipReliableDataInboundUserAddProcess(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, ReliableDataT *pReliableData)
{
    uint8_t uUserIndex = pReliableData->aData[0];    // first byte is user index
    VoipUserT *pRemoteUser = (VoipUserT *)(&pReliableData->aData[0] + 1);

    if (VOIP_NullUser((VoipUserT *)&pConnection->RemoteUsers[uUserIndex]))
    {
        int32_t iConnID = pConnection - &pConnectionlist->pConnections[0];

        // update collection of remote users for this connection
        VOIP_CopyUser(&pConnection->RemoteUsers[uUserIndex], pRemoteUser);

        // register the user
        pConnectionlist->pRegUserCb((VoipUserT *)&pConnection->RemoteUsers[uUserIndex], (uint32_t)iConnID, TRUE, pConnectionlist->pRegUserUserData);

        // force re-application of channel config since we have a new remote player
        ((VoipCommonRefT *)VoipGetRef())->bApplyChannelConfig = TRUE;

#if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
        // reapply player-to-player xone comm relationships
        pConnectionlist->bApplyRelFromMuting = TRUE;
#endif
        // set mute list to be updated
        pConnectionlist->bUpdateMute = TRUE;

        #if DIRTYCODE_LOGGING
        _VoipConnectionPrintRemoteUsers(pConnection, (pConnection - &pConnectionlist->pConnections[0]));
        #endif
    }
    else
    {
        NetPrintf(("voipconnection: [%d] join-in-progress for remote user %lld failed because index %d already used by %lld\n", (pConnection - &pConnectionlist->pConnections[0]),
            ((VoipUserT *)pRemoteUser)->AccountInfo.iPersonaId, uUserIndex, ((VoipUserT *)&pConnection->RemoteUsers[uUserIndex])->AccountInfo.iPersonaId));
    }
}


/*F********************************************************************************/
/*!
    \Function   _VoipReliableDataInboundUserRemProcess

    \Description
        Inbound processing of reliablity data of type VOIP_RELIABLE_TYPE_USERREM.

    \Input *pConnectionlist - connectionlist ref
    \Input *pConnection     - connection on which the packet was received
    \Input *pReliableData   - pointer to inbound reliable data entry

    \Version 09/14/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipReliableDataInboundUserRemProcess(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, ReliableDataT *pReliableData)
{
    uint8_t uUserIndex = pReliableData->aData[0];    // first byte is user index
    VoipUserT *pRemoteUser = (VoipUserT *)(&pReliableData->aData[0] + 1);

    if (!VOIP_NullUser((VoipUserT *)&pConnection->RemoteUsers[uUserIndex]))
    {
        if (memcmp(pRemoteUser, &pConnection->RemoteUsers[uUserIndex], sizeof(*pRemoteUser)) == 0)
        {
            int32_t iConnID = pConnection - &pConnectionlist->pConnections[0];

            // unregister the user
            pConnectionlist->pRegUserCb((VoipUserT *)&pConnection->RemoteUsers[uUserIndex], (uint32_t)iConnID, FALSE, pConnectionlist->pRegUserUserData);

            // force re-application of channel config since we lost a remote player
            ((VoipCommonRefT *)VoipGetRef())->bApplyChannelConfig = TRUE;

#if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
            // reapply player-to-player xone comm relationships
            pConnectionlist->bApplyRelFromMuting = TRUE;
#endif

            // set mute list to be updated
            pConnectionlist->bUpdateMute = TRUE;

            // clear the user
            VOIP_ClearUser((VoipUserT *)&pConnection->RemoteUsers[uUserIndex]);

            #if DIRTYCODE_LOGGING
            _VoipConnectionPrintRemoteUsers(pConnection, (pConnection - &pConnectionlist->pConnections[0]));
            #endif
        }
        else
        {
            NetPrintf(("voipconnection: [%d] leave-in-progress for remote user %lld failed because remote user at index %d rather is %lld\n", (pConnection - &pConnectionlist->pConnections[0]),
                ((VoipUserT *)pRemoteUser)->AccountInfo.iPersonaId, uUserIndex, ((VoipUserT *)&pConnection->RemoteUsers[uUserIndex])->AccountInfo.iPersonaId));
        }
    }
    else
    {
        NetPrintf(("voipconnection: [%d] leave-in-progress for remote user %lld failed because no remote user at index %d\n", (pConnection - &pConnectionlist->pConnections[0]),
            ((VoipUserT *)pRemoteUser)->AccountInfo.iPersonaId, uUserIndex));
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipReliableDataInboundOpaqueProcess

    \Description
        Inbound processing of reliablity data of type VOIP_RELIABLE_TYPE_OPAQUE.

    \Input *pConnectionlist - connectionlist ref
    \Input *pConnection     - connection on which the packet was received
    \Input *pReliableData   - pointer to inbound reliable data entry

    \Version 09/14/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipReliableDataInboundOpaqueProcess(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, ReliableDataT *pReliableData)
{
    uint16_t uDataSize = _VoipDecodeU16(&pReliableData->info.uSize[0]);
    int32_t iConnID = pConnection - &pConnectionlist->pConnections[0];
    pConnectionlist->pOpaqueCb(iConnID, &pReliableData->aData[0], uDataSize, pConnectionlist->pOpaqueUserData);
}

/*F********************************************************************************/
/*!
    \Function   _VoipReliableDataInboundTextProcess

    \Description
        Inbound processing of reliablity data of type VOIP_RELIABLE_TYPE_TEXT.

    \Input *pConnectionlist - connectionlist ref
    \Input *pConnection     - connection on which the packet was received
    \Input *pReliableData   - pointer to inbound reliable data entry

    \Version 05/02/2017 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipReliableDataInboundTextProcess(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, ReliableDataT *pReliableData)
{
    uint8_t uUserIndex = pReliableData->aData[0];    // first byte is user index
    uint16_t uTextSize = _VoipDecodeU16(&pReliableData->info.uSize[0]) - 1;  // text size is payload size minus the byte used for the user index

    if (uTextSize > 0)
    {
        if (_VoipConnectionIsTranscribedTextRequested(pConnectionlist))
        {
            int32_t iConnID = pConnection - &pConnectionlist->pConnections[0];

            if (pConnectionlist->uRecvMask & (1 << iConnID))
            {
                pConnectionlist->pTextCb(iConnID, uUserIndex, (char *)&pReliableData->aData[1], (void *)pConnectionlist->pTextUserData);
            }
            else
            {
                NetPrintf(("voipconnection: inbound transcribed text dropped because connection (%d) is muted\n", iConnID));
            }
        }
        else
        {
            NetPrintf(("voipconnection: inbound transcribed text dropped because none of the local players requested for it (normal when voip is routed through DirytCast voipserver)\n"));
        }
    }
    else
    {
        NetPrintf(("voipconnection: inbound transcribed text dropped because decoding resulted in an empty string\n"));
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipReliableDataUnpackACKandACKCNF

    \Description
        Inbound processing of reliablity data - deals with ACK+ACKCNF protocol
        components specifically.

    \Input *pConnectionlist - connectionlist ref
    \Input uSrcClientId     - client id of originator
    \Input uFlags           - uFlags from the inbound packet
    \Input bPing            - TRUE if packet is a ping packet, FALSE if packet is a micr packet
    \Input *pRead           - pointer to next byte to be read in packet payload

    \Output
        uint8_t *           - pointer to first byte after ACK

    \Version 09/14/2014 (mclouatre)
*/
/********************************************************************************F*/
static uint8_t * _VoipReliableDataUnpackACKandACKCNF(VoipConnectionlistT *pConnectionlist, uint32_t uSrcClientId, uint8_t uFlags, uint32_t bPing, uint8_t *pRead)
{
    int32_t iConnectionIndex;
    VoipConnectionT *pConnection = NULL;
    uint32_t bPeerIsAckingUs = FALSE;

    // find connection matching the source client id
    for (iConnectionIndex = 0; iConnectionIndex < pConnectionlist->iMaxConnections; iConnectionIndex++)
    {
        if (pConnectionlist->pConnections[iConnectionIndex].uRemoteClientId == uSrcClientId)
        {
            pConnection = &pConnectionlist->pConnections[iConnectionIndex];
            break;
        }
    }

    // is the packet buffer already filled with ACK+ACKCNF protocol components used for the reliability mechanism?
    if (uFlags &  VOIP_PACKET_RELIABLE_FLAG_ACK_ACKCNF)
    {
        int32_t iReliableAckEntriesCount = *pRead++;
        int32_t iReliableAckEntryIndex;

        for (iReliableAckEntryIndex = 0; iReliableAckEntryIndex < iReliableAckEntriesCount; iReliableAckEntryIndex++)
        {
            uint32_t uClientId; 
            ReliableAckT *pReliableAck = (ReliableAckT *)pRead;

            pRead += sizeof(ReliableAckT);

            if (pConnection != NULL)
            {
                // determine our local client id from the remote client's point of view
                uint32_t uLocalClientId = pConnection->bIsLocalClientIdValid ? pConnection->uLocalClientId : pConnectionlist->uClientId;

                // extract client id from ACK+ACKCNF protocol component entry
                uClientId = _VoipDecodeU32(&pReliableAck->aRemoteClientId[0]);

                // ignore the entry if it is not for us
                if (uClientId == uLocalClientId)
                {
                    uint8_t uAckedSeq;

                    // deal with ACKCNF
                    pConnection->bPeerNeedsAck = ((pReliableAck->uAckedSeq & VOIP_RELIABLE_ACKCNF_MASK) ? TRUE : FALSE);

                    // deal with ACK
                    uAckedSeq = pReliableAck->uAckedSeq & ~VOIP_RELIABLE_ACKCNF_MASK;
                    if (uAckedSeq != 0)
                    {
                        #if VOIP_RELIABLE_DEBUG
                        NetPrintf(("voipconnection: [%d] reliable ACK+ACKCNF (acked seq# %03d, %s) found in inbound %s packet from clientId=0x%08x\n",
                            iConnectionIndex, uAckedSeq, (pConnection->bPeerNeedsAck ?"SendAck":"HaltAck"), (bPing?"PNG":"MIC"), uSrcClientId));
                        #endif
                        bPeerIsAckingUs = TRUE;

                        while ( (pConnection->pOutboundReliableDataQueue != NULL) && 
                                (uAckedSeq >= pConnection->pOutboundReliableDataQueue->data.info.uSeq) )
                        {
                            _VoipConnectionReliableDataDequeue(pConnectionlist, iConnectionIndex);
                        }
                    }
                    else
                    {
                        #if VOIP_RELIABLE_DEBUG
                        NetPrintf(("voipconnection: [%d] reliable ACK+ACKCNF (acked seq# INV, %s) found in inbound %s packet from clientId=0x%08x\n",
                            iConnectionIndex, (pConnection->bPeerNeedsAck ?"SendAck":"HaltAck"), (bPing?"PNG":"MIC"), uSrcClientId));
                        #endif
                    }
                }
            }
            else
            {
                NetPrintf(("voipconnection: [%d] dropped inbound ACK+ACKCNF - failed to find connection matching originating clientId=0x%08x\n", iConnectionIndex, uSrcClientId));
            }
        }
    }

    if (pConnection != NULL)
    {
        pConnection->bPeerIsAcking = bPeerIsAckingUs;
    }

    return(pRead);
}

/*F********************************************************************************/
/*!
    \Function   _VoipReliableDataUnpackDATA

    \Description
        Inbound processing of reliablity data - deals with DATA protocol component specifically.

    \Input *pConnectionlist - connectionlist ref
    \Input *pConnection     - connection on which the packet was received
    \Input uFlags           - uFlags from the inbound packet
    \Input bPing            - TRUE if packet is a ping packet, FALSE if packet is a micr packet
    \Input *pRead           - pointer to next byte to be read in packet payload

    \Output
        uint8_t *           - pointer to first byte after DATA

    \Version 09/14/2014 (mclouatre)
*/
/********************************************************************************F*/
static uint8_t * _VoipReliableDataUnpackDATA(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, uint8_t uFlags, uint32_t bPing, uint8_t *pRead)
{
    // is the packet buffer already filled with the ACK protocol component used for the reliability mechanism?
    if (uFlags & VOIP_PACKET_RELIABLE_FLAG_DATA)
    {
        int32_t iReliableDataEntriesCount = *pRead++;
        int32_t iReliableDataEntryIndex;

        for (iReliableDataEntryIndex = 0; iReliableDataEntryIndex < iReliableDataEntriesCount; iReliableDataEntryIndex++)
        {
            ReliableDataT *pReliableData = (ReliableDataT *)pRead;
            uint16_t uSize = _VoipDecodeU16(&pReliableData->info.uSize[0]);
            uint32_t uClientId = _VoipDecodeU32(&pReliableData->info.aRemoteClientId[0]);
            uint32_t uLocalClientId = pConnection->bIsLocalClientIdValid ? pConnection->uLocalClientId : pConnectionlist->uClientId;

            // move read pointer ahead
            pRead += sizeof(pReliableData->info) + uSize;

            // make sure this is for us
            if (uClientId == uLocalClientId)
            {
                if (pReliableData->info.uSeq == _VoipConnectionIncrementReliableSeqNb(pConnection->uInSeqNb))
                {
                    // update next expected inbound reliable seq nb
                    pConnection->uInSeqNb = pReliableData->info.uSeq;

                    #if VOIP_RELIABLE_DEBUG
                    NetPrintf(("voipconnection: [%d] reliable DATA (seq# %03d, %s, %d bytes) found in inbound %s packet\n", (pConnection - &pConnectionlist->pConnections[0]),
                        pReliableData->info.uSeq, _strReliableType[pReliableData->info.uType], uSize, (bPing?"PNG":"MIC")));
                    #endif

                    if (pReliableData->info.uType == VOIP_RELIABLE_TYPE_USERADD)
                    {
                        _VoipReliableDataInboundUserAddProcess(pConnectionlist, pConnection, pReliableData);
                    }
                    else if (pReliableData->info.uType == VOIP_RELIABLE_TYPE_USERREM)
                    {
                        _VoipReliableDataInboundUserRemProcess(pConnectionlist, pConnection, pReliableData);
                    }
                    else if (pReliableData->info.uType == VOIP_RELIABLE_TYPE_OPAQUE)
                    {
                        _VoipReliableDataInboundOpaqueProcess(pConnectionlist, pConnection, pReliableData);
                    }
                    else if (pReliableData->info.uType == VOIP_RELIABLE_TYPE_TEXT)
                    {
                        _VoipReliableDataInboundTextProcess(pConnectionlist, pConnection, pReliableData);
                    }
                    else
                    {
                        NetPrintf(("voipconnection: [%d] unsupported inbound reliable data type (%d)\n", (pConnection - &pConnectionlist->pConnections[0]), pReliableData->info.uType));
                    }
                }
                else
                {
                    #if VOIP_RELIABLE_DEBUG
                    NetPrintf(("voipconnection: [%d] dropped inbound reliable data embedded in %s packet - seq nb mismatch (got: %d - expected: %d)\n",
                        (pConnection - &pConnectionlist->pConnections[0]), (bPing ? "PNG" : "MIC"), pReliableData->info.uSeq, _VoipConnectionIncrementReliableSeqNb(pConnection->uInSeqNb)));
                    #endif
                }
            }
            else
            {
                #if VOIP_RELIABLE_DEBUG
                NetPrintf(("voipconnection: [%d] dropped inbound reliable data embedded in %s packet - client id mismatch (target: 0x%08x - local: 0x%08x)\n",
                    (pConnection - &pConnectionlist->pConnections[0]), (bPing ? "PNG" : "MIC"), uClientId, uLocalClientId));
                #endif
            }
        }
    }

    return(pRead);
}

/*F********************************************************************************/
/*!
    \Function   _VoipReliableDataInProcess

    \Description
        Inbound processing of reliablity data. To be invoked every time a
        ping packet or a micr packet is received. Reliability protocol components
        (ACK, ACKCNF, DATA) included in the packet are going to be processed appropriately.

    \Input *pConnectionlist - connectionlist ref
    \Input *pConnection     - connection on which the packet was received
    \Input *pPacketHeader   - pointer to header of the inbound packet
    \Input *pPacketPayload  - pointer to first payload byte of inbound packet

    \Output
        uint8_t             - first byte following reliable data in packet payload

    \Version 09/14/2014 (mclouatre)
*/
/********************************************************************************F*/
static uint8_t * _VoipReliableDataInProcess(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, VoipPacketHeadT *pPacketHeader, uint8_t *pPacketPayload)
{
    // note: for voipserver-based topology, uSrcClientId may not be same as pConnection->uClientId
    uint32_t uSrcClientId = _VoipDecodeU32(&pPacketHeader->aClientId[0]);
    uint32_t bPing = ((pPacketHeader->aType[0] == 'P' && pPacketHeader->aType[1] == 'N' && pPacketHeader->aType[2] == 'G') ? TRUE : FALSE);

    uint8_t *pRead = _VoipReliableDataUnpackACKandACKCNF(pConnectionlist, uSrcClientId, pPacketHeader->uFlags, bPing, pPacketPayload);

    pRead = _VoipReliableDataUnpackDATA(pConnectionlist, pConnection, pPacketHeader->uFlags, bPing, pRead);

    return(pRead);
}

/*F********************************************************************************/
/*!
    \Function   _VoipUpdateUserFlags

    \Description
        Update the flags field for each user just before serialization.

    \Input *pConnectionlist - connectionlist ref

    \Version 10/02/2019 (cvienneau)
*/
/********************************************************************************F*/
static void _VoipUpdateUserFlags(VoipConnectionlistT *pConnectionlist)
{
    int32_t iUserIndex;
    uint8_t bCrossPlay = VoipStatus(VoipGetRef(), 'xply', 0, NULL, 0);

    for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS_EXTENDED; iUserIndex++)
    {
        if (pConnectionlist->aIsParticipating[iUserIndex] == TRUE)
        {
            // is cross play setup for the local users
            if (bCrossPlay)
            {
                pConnectionlist->LocalUsers[iUserIndex].uFlags |= VOIPUSER_FLAG_CROSSPLAY;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionConn

    \Description
        Send a connection packet to peer

    \Input *pConnectionlist - connectionlist ref
    \Input *pConnection     - pointer to connection to connect on
    \Input uTick            - current tick count

    \Version 03/20/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipConnectionConn(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, uint32_t uTick)
{
    VoipConnPacketT ConnPacket;
    int32_t iUserIndex;
    uint32_t uLocalClientId = 0;
    uint32_t uSize = sizeof(ConnPacket) - sizeof(ConnPacket.LocalUsers) + (sizeof(*ConnPacket.LocalUsers) * VOIP_MAXLOCALUSERS_EXTENDED);

    // output diagnostic info
    NetPrintf(("voipconnection: [%d] (sess id 0x%08x) sending connect packet to remote console (%a:%d)\n",
        VOIP_ConnID(pConnectionlist, pConnection), pConnection->uSessionId[0],
        SocketNtohl(pConnection->SendAddr.sin_addr.s_addr), SocketNtohs(pConnection->SendAddr.sin_port)));

    // send a connection packet
    ds_memclr(&ConnPacket, sizeof(VoipConnPacketT));
    ds_memcpy_s(&ConnPacket.Head, sizeof(ConnPacket.Head), &_Voip_ConnPacket, sizeof(_Voip_ConnPacket));
    _VoipEncodeU32(ConnPacket.aRemoteClientId, pConnection->uRemoteClientId);
    _VoipEncodeU32(ConnPacket.Head.aSessionId, pConnection->uSessionId[0]);
    _VoipEncodeLocalHeadsetStatus(pConnectionlist, &(ConnPacket.Head));
    if (_VoipConnectionIsTranscribedTextRequested(pConnectionlist))
    {
        ConnPacket.Head.uFlags |= VOIP_PACKET_STATUS_FLAG_STT;
    }
    _VoipUpdateUserFlags(pConnectionlist);
    for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS_EXTENDED; iUserIndex++)
    {
        if (pConnectionlist->aIsParticipating[iUserIndex] != FALSE)
        {
            _VoipEncodeVoipUser(&ConnPacket.LocalUsers[iUserIndex], &pConnectionlist->LocalUsers[iUserIndex]);
        }
    }

    ConnPacket.bConnected = pConnection->bConnPktRecv;

    // extended local users always have an extra slot allocated and are not the same as maxlocalusers
    ConnPacket.uNumLocalUsers = VOIP_MAXLOCALUSERS_EXTENDED;

    #if VOIP_CONNECTION_DEBUG
    NetPrintMem(&ConnPacket, sizeof(ConnPacket), "connection packet data");
    #endif
    uLocalClientId = pConnection->uLocalClientId == 0 ? pConnectionlist->uClientId : pConnection->uLocalClientId;
    pConnection->iSendResult = _VoipSocketSendto(pConnectionlist->pSocket, uLocalClientId, &ConnPacket.Head, uSize, 0, (struct sockaddr *)&pConnection->SendAddr, sizeof(pConnection->SendAddr));

    // update timestamp
    pConnection->uLastSend = uTick;
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionDisc

    \Description
        Send a disconnection request to peer.

    \Input iConnId          - connection id
    \Input uClientId        - client id
    \Input uRemoteClientId  - remote client id
    \Input uSessionId       - session identifier
    \Input *pSendAddr       - address to send disconnect to
    \Input *pSocket         - socket to send request with

    \Version 03/21/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipConnectionDisc(int32_t iConnId, uint32_t uClientId, uint32_t uRemoteClientId, uint32_t uSessionId, struct sockaddr_in *pSendAddr, SocketT *pSocket)
{
    VoipDiscPacketT DiscPacket;

    // send a disconnect packet
    NetPrintf(("voipconnection: [%d] (sess id 0x%08x) sending disconnect packet to %a:%d\n",
        iConnId, uSessionId, SocketNtohl(pSendAddr->sin_addr.s_addr), SocketNtohs(pSendAddr->sin_port)));

    // set up disc packet
    ds_memcpy_s(&DiscPacket.Head, sizeof(DiscPacket.Head), &_Voip_DiscPacket, sizeof(_Voip_DiscPacket));
    _VoipEncodeU32(DiscPacket.aRemoteClientId, uRemoteClientId);
    _VoipEncodeU32(DiscPacket.Head.aSessionId, uSessionId);

    // send disc packet
    _VoipSocketSendto(pSocket, uClientId, &DiscPacket.Head, sizeof(DiscPacket), 0, (struct sockaddr *)pSendAddr, sizeof(*pSendAddr));
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionInit

    \Description
        Transition from connecting to connected.

    \Input *pConnectionlist - connectionlist connection is a part of
    \Input *pConnection     - connection to transition
    \Input iConnID          - index of connection

    \Version 03/21/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipConnectionInit(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, int32_t iConnID)
{
    int32_t iLocalUserIndex;

    NetPrintf(("voipconnection: [%d] connection established; talking to remote user(s)\n", iConnID));

    #if DIRTYCODE_LOGGING
    _VoipConnectionPrintRemoteUsers(pConnection, iConnID);
    #endif

    // register the new remote users with voipheadset
    VoipConnectionRegisterRemoteTalkers(pConnectionlist, iConnID, TRUE);

    // we want to call this before the connection goes to state ST_ACTV
    // to avoid race conditions where the voipthread would start working with this conn before the channel config is updated
    VoipCommonApplyChannelConfig((VoipCommonRefT *)VoipGetRef());

    // update connection flags
    pConnection->uRemoteConnStatus = VOIP_CONN_CONNECTED;

    // flag that this connection shall now be considered when updating friend status bitmask
    if (pConnectionlist->iFriendConnId == VOIP_CONNID_NONE)
    {
        pConnectionlist->iFriendConnId = iConnID;
    }
    else
    {
        pConnectionlist->iFriendConnId = VOIP_CONNID_ALL;
    }

    // initialize timer used for sending at a fixed 10hz rate
    for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS_EXTENDED; iLocalUserIndex++)
    {
        pConnection->aVoiceSendTimer[iLocalUserIndex] = NetTick();
    }

    /*
    To avoid multi-threading race conditions, it is important to only move the connection state to ST_ACTV at the end of
    the function (after initializing all connection vars and updating channel config) because some code exercised by the
    _VoipThread (like _VoipUpdateRemoteStatus()) may start to use these fields concurrently as soon as the connection state
    becomes ST_ACTV
    */
    pConnection->eState = ST_ACTV;
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionPing

    \Description
        Ping a connected remote peer.

    \Input *pConnectionlist - connectionlist ref
    \Input *pConnection     - pointer to connection to send on
    \Input uTick            - current tick count

    \Version 03/20/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipConnectionPing(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, uint32_t uTick)
{
    VoipPingPacketT PingPacket;
    int32_t iUserIndex;
    uint32_t uChannelUserIndices = 0;
    uint8_t *pWrite = PingPacket.aData;
    const uint8_t *pEnd = pWrite+sizeof(PingPacket.aData);
    uint32_t uLocalClientId = pConnection->bIsLocalClientIdValid ? pConnection->uLocalClientId : pConnectionlist->uClientId;

    #if VOIP_CONNECTION_DEBUG
    NetPrintf(("voipconnection: [%d] (sess id 0x%08x) pinging %a:%d\n",
        VOIP_ConnID(pConnectionlist, pConnection), pConnection->uSessionId[0],
        SocketNtohl(pConnection->SendAddr.sin_addr.s_addr), SocketNtohs(pConnection->SendAddr.sin_port)));
    #endif

    // set up ping packet
    ds_memcpy_s(&PingPacket.Head, sizeof(PingPacket.Head), &_Voip_PingPacket, sizeof(_Voip_PingPacket));
    _VoipEncodeLocalHeadsetStatus(pConnectionlist, &(PingPacket.Head));
    _VoipEncodeU32(PingPacket.aRemoteClientId, pConnection->uRemoteClientId);
    _VoipEncodeU32(PingPacket.Head.aSessionId, pConnection->uSessionId[0]);
    if (_VoipConnectionIsTranscribedTextRequested(pConnectionlist))
    {
        PingPacket.Head.uFlags |= VOIP_PACKET_STATUS_FLAG_STT;
    }
    for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS_EXTENDED; iUserIndex++)
    {
        /* write the channel information for the participating user indices and
           set the bit so the receiving side knows which index it corresponds to */
        if (pConnectionlist->aIsParticipating[iUserIndex])
        {
            uChannelUserIndices |= 1 << iUserIndex;
            _VoipEncodeU32(pWrite, pConnectionlist->aChannels[iUserIndex]);
            pWrite += 4;
        }
    }
    _VoipEncodeU32(PingPacket.aChannelUserIndices, uChannelUserIndices);

    // if needed, add reliable protocol components to this ping packet
    pWrite = _VoipReliableDataOutProcess(pConnectionlist, (pConnection - &pConnectionlist->pConnections[0]), &PingPacket.Head, pWrite, pEnd - pWrite);

    // send a 'ping' packet
    pConnection->iSendResult = _VoipSocketSendto(pConnectionlist->pSocket, uLocalClientId, &PingPacket.Head, (pWrite - (uint8_t *)&PingPacket), 0, (struct sockaddr *)&pConnection->SendAddr, sizeof(pConnection->SendAddr));

    // update send timestamp and local status flag
    pConnection->uLastSend = uTick;
    for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS; ++iUserIndex)
    {
        if (NetTickDiff(uTick, pConnectionlist->uLocalUserLastSend[iUserIndex]) > VOIP_PING_RATE)
        {
            pConnectionlist->uLocalUserStatus[iUserIndex] &= ~VOIP_LOCAL_USER_SENDVOICE;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionIsSendPktReady

    \Description
        Determine if we should send the current packet.

    \Input *pConnectionlist - connectionlist ref
    \Input *pConnection     - connection pointer
    \Input iUserIndex       - index of local user that generated the voice to be sent
    \Input uTick            - current tick count

    \Output
        uint32_t        - TRUE if ready, FALSE otherwise.

    \Version 10/01/2011 (mclouatre)
*/
/********************************************************************************F*/
static uint32_t _VoipConnectionIsSendPktReady(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, int32_t iUserIndex, uint32_t uTick)
{
    VoipMicrPacketT *pVoipPkt = &pConnection->VoipMicrPacket[iUserIndex];
    uint8_t *pWrite = _VoipPacketGetWritePtr(pVoipPkt);
    uint8_t bVariableSubPktLen = ((pVoipPkt->MicrInfo.uSubPacketSize == 0xFF) ? TRUE : FALSE);  // check if sub-pkts in this packet are fixed length or variable length
    int32_t iEstimatedResultingPacketPayloadSize;

    // if an additional sub-packet would cause us to overflow the sub-packet buffer, send packet
    if (bVariableSubPktLen)
    {
        // is next packet going to overflow? 1 byte for prepended size + estimated subpkt size
        iEstimatedResultingPacketPayloadSize = (pWrite + 1 + pConnection->iMaxSubPktSize) - pVoipPkt->aData;
    }
    else
    {
       iEstimatedResultingPacketPayloadSize = (pWrite + pVoipPkt->MicrInfo.uSubPacketSize) - pVoipPkt->aData;
    }

    if (iEstimatedResultingPacketPayloadSize > (signed)sizeof(pVoipPkt->aData))
    {
        return(TRUE);
    }

    // if 10hz timer has elapsed, send packet
    if ((pVoipPkt->MicrInfo.uNumSubPackets > 0) && (NetTickDiff(uTick, pConnection->aVoiceSendTimer[iUserIndex]) >= 0))
    {
        return(TRUE);
    }

    // not yet ready to send
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionSendSingle

    \Description
        Send a single buffered voice packet, if it is time to send it.

    \Input *pConnectionlist - connectionlist ref
    \Input *pConnection     - pointer to connection to send on
    \Input iUserIndex       - index of the local user that generated the voice data to be sent
    \Input uTick            - current tick count
    \Input bFlush           - flush data

    \Version 03/20/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipConnectionSendSingle(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, int32_t iUserIndex, uint32_t uTick, uint32_t bFlush)
{
    // ref current buffered packet
    VoipMicrPacketT *pPacket = &pConnection->VoipMicrPacket[iUserIndex];
    uint32_t bSendPacket = FALSE;
    int32_t iParticipatingUserIndex;
    uint32_t uLocalClientId = pConnection->bIsLocalClientIdValid ? pConnection->uLocalClientId : pConnectionlist->uClientId;

    // should we send?
    if (_VoipConnectionIsSendPktReady(pConnectionlist, pConnection, iUserIndex, uTick) || (bFlush == TRUE))
    {
        bSendPacket = TRUE;
    }

    // send packet?
    if (bSendPacket == TRUE)
    {
        int32_t iPacketSize, iValidPayloadSize;
        uint32_t uVoipServerReadySendMask = 0;

        // set packet sequence
        pPacket->MicrInfo.uSeqn = pConnection->uSendSeqn++;

        // set the user index
        if (iUserIndex == VOIP_SHARED_USER_INDEX)
        {
            pPacket->MicrInfo.uUserIndex = VOIP_SHARED_REMOTE_INDEX;
        }
        else
        {
            pPacket->MicrInfo.uUserIndex = iUserIndex;
        }

        // set the channels
        _VoipEncodeU32(pPacket->MicrInfo.channels.aChannelId, pConnectionlist->aChannels[iUserIndex]);

        // set the session id
        _VoipEncodeU32(pPacket->Head.aSessionId, pConnection->uSessionId[0]);

        // flag whether one of the local users is requesting transcribed text
        if (_VoipConnectionIsTranscribedTextRequested(pConnectionlist))
        {
            pPacket->Head.uFlags |= VOIP_PACKET_STATUS_FLAG_STT;
        }

        // set send mask for VoipServer use
        if (pConnection->iVoipServerConnId != VOIP_CONNID_NONE)
        {
            uVoipServerReadySendMask = _VoipPrepareVoipServerSendMask(pConnectionlist);
        }
        _VoipEncodeU32(pPacket->MicrInfo.aSendMask, uVoipServerReadySendMask);
        _VoipEncodeLocalHeadsetStatus(pConnectionlist, &pPacket->Head);

        #if VOIP_CONNECTION_DEBUG
        // display verbose status (metered to about once every three seconds)
        if ((pConnection->uSendSeqn % 30) == 0)
        {
            char strSubPktSize[32];
            sprintf(strSubPktSize, "size %d (bytes)", pPacket->MicrInfo.uSubPacketSize);
            NetPrintf(("voipconnection: [%d] (sess id 0x%08x) local usr %d sending pkt (seq#:%d) with %d sub-pkt(s) of %s to %a:%d | voip channels:0x%08x\n",
                VOIP_ConnID(pConnectionlist, pConnection), pConnection->uSessionId[0], iUserIndex, pPacket->MicrInfo.uSeqn, pPacket->MicrInfo.uNumSubPackets,
                (pPacket->MicrInfo.uSubPacketSize == 0xFF ? "variable-length" : strSubPktSize), SocketNtohl(pConnection->SendAddr.sin_addr.s_addr), SocketNtohs(pConnection->SendAddr.sin_port),
                pConnectionlist->aChannels[iUserIndex]));
        }
        #endif

        // send voice packet
        iValidPayloadSize = _VoipPacketGetWritePtr(&pConnection->VoipMicrPacket[iUserIndex]) - &pPacket->aData[0];
        iPacketSize = sizeof(pConnection->VoipMicrPacket[iUserIndex]) - sizeof(pPacket->aData) + iValidPayloadSize;
        pConnection->iSendResult = _VoipSocketSendto(pConnectionlist->pSocket, uLocalClientId, &pPacket->Head,
            iPacketSize, 0, (struct sockaddr *)&pConnection->SendAddr, sizeof(pConnection->SendAddr));

        // update send info -- don't update uLastSend in server mode, to force pinging even when voice is being sent
        if (pConnection->iVoipServerConnId == VOIP_CONNID_NONE)
        {
            pConnection->uLastSend = uTick;
        }

        // if packet contains reliable data, don't update aVoiceSendTimer to make sure we retry sending voip sub-packets soon (in case reliable data took all the place in the packet payload)
        if ((pPacket->Head.uFlags & VOIP_PACKET_RELIABLE_FLAG_DATA) == 0)
        {
            pConnection->aVoiceSendTimer[iUserIndex] = uTick + VOIP_MSPERPACKET;
        }

        // if the voice packet is from a shared user set all the participating user's status
        if (iUserIndex == VOIP_SHARED_USER_INDEX)
        {
            for (iParticipatingUserIndex = 0; iParticipatingUserIndex < VOIP_MAXLOCALUSERS; ++iParticipatingUserIndex)
            {
                if (pConnectionlist->aIsParticipating[iParticipatingUserIndex] == TRUE)
                {
                    #if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
                    /* For xboxone, opaque traffic is always sent as if it was originated from the
                       "shared user". We need to query VoipHeadset (via VoipCommon) to find out which 
                       local user is exactly detected as talking by the MS game chat 2's chat manager
                       and consequently involved in generating the current outbound flow of voip packets. */
                    if (VoipCommonStatus((VoipCommonRefT *)VoipGetRef(), 'talk', iParticipatingUserIndex, NULL, 0))
                    #endif
                    {
                        pConnectionlist->uLocalUserStatus[iParticipatingUserIndex] |= VOIP_LOCAL_USER_SENDVOICE;
                        pConnectionlist->uLocalUserLastSend[iParticipatingUserIndex] = uTick;
                    }
                }
            }
        }
        else
        {
            pConnectionlist->uLocalUserStatus[iUserIndex] |= VOIP_LOCAL_USER_SENDVOICE;
            pConnectionlist->uLocalUserLastSend[iUserIndex] = uTick;
        }

        // reset buffer used to build voip micr packet
        ds_memclr(&pPacket->MicrInfo, sizeof(pPacket->MicrInfo));       // clear micr packet info
        pPacket->Head.uFlags = 0;                                       // clear flags in packet header
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionRecvSingle

    \Description
        Handle data incoming on a connection.

    \Input *pConnectionlist - connectionlist connection belongs to
    \Input *pConnection     - pointer to connection to receive on
    \Input *pSendAddr       - remote address data was received from
    \Input *pPacket         - packet data that was received
    \Input iSize            - size of data received

    \Version 03/21/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipConnectionRecvSingle(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, struct sockaddr_in *pSendAddr, VoipPacketBufferT *pPacket, int32_t iSize)
{
    int32_t iConnID = VOIP_ConnID(pConnectionlist, pConnection);
    uint32_t uSessionId = _VoipDecodeU32(pPacket->VoipPacketHead.aSessionId);
    #if DIRTYCODE_LOGGING
    uint32_t uClientId = _VoipDecodeU32(pPacket->VoipPacketHead.aClientId);
    const char *pStrPktType;
    #endif
    uint32_t uTick;
    uint8_t uPktType;
    uint32_t uIndex;

    // note time of arrival
    uTick = NetTick();

    // count the packet
    pConnection->iRecvPackets += 1;
    pConnection->iRecvData += iSize;

    // did port change?
    if (pConnection->SendAddr.sin_port != pSendAddr->sin_port)
    {
        NetPrintf(("voipconnection: [%d] port remap #%d: %d bytes from %d instead of %d\n", iConnID,
            pConnection->iChangePort++, iSize,
            SocketNtohs(pSendAddr->sin_port),
            SocketNtohs(pConnection->SendAddr.sin_port)));

        // send future to remapped port
        pConnection->SendAddr.sin_port = pSendAddr->sin_port;

        // reset counter of packets received
        pConnection->iRecvPackets = 0;
    }

    if (pPacket->VoipPacketHead.uFlags & VOIP_PACKET_STATUS_FLAG_STT)
    {
        #if DIRTYCODE_LOGGING
        if (pConnection->bTranscribedTextRequested == FALSE)
        {
            NetPrintf(("voipconnection: [%d] one or several of the remote peers on that connection are requesting transcribed text\n", iConnID));
        }
        #endif
        pConnection->bTranscribedTextRequested = TRUE;
    }
    else
    {
        #if DIRTYCODE_LOGGING
        if (pConnection->bTranscribedTextRequested != FALSE)
        {
            NetPrintf(("voipconnection: [%d] remote peer(s) are no longer requesting transcribed text\n", iConnID));
        }
        #endif
        pConnection->bTranscribedTextRequested = FALSE;
    }

    // identify type of incoming packet (connect, disconnect, ping, mic or unknown)
    if (VOIP_SamePacketType(&pPacket->VoipPacketHead, &_Voip_ConnPacket))
    {
        uPktType = 'C';  // connection packet
        #if DIRTYCODE_LOGGING
        pStrPktType = "connect";
        #endif
    }
    else if (VOIP_SamePacketType(&pPacket->VoipPacketHead, &_Voip_PingPacket))
    {
        uPktType = 'P';  // ping packet
        #if DIRTYCODE_LOGGING
        pStrPktType = "ping";
        #endif
    }
    else if (VOIP_SamePacketType(&pPacket->VoipPacketHead, &_Voip_MicrPacket))
    {
        uPktType = 'M';  // data packet
        #if DIRTYCODE_LOGGING
        pStrPktType = "data";
        #endif
    }
    else if (VOIP_SamePacketType(&pPacket->VoipPacketHead, &_Voip_DiscPacket))
    {
        uPktType = 'D';  // disconnect packet
        #if DIRTYCODE_LOGGING
        pStrPktType = "disconnect";
        #endif
    }
    else
    {
        NetPrintf(("voipconnection: [%d] received unknown packet type 0x%02x%02x%02x\n", iConnID,
            pPacket->VoipPacketHead.aType[0], pPacket->VoipPacketHead.aType[1], pPacket->VoipPacketHead.aType[2]));
        return;
    }

    // ignore packet if session id does not match
    if (_VoipConnectionMatchSessionId(pConnectionlist, iConnID, uSessionId) == FALSE)
    {
        #if DIRTYCODE_LOGGING
        NetPrintf(("voipconnection: [%d] (sess id 0x%08x) ignoring packet because of session IDs mismatch\n",
            iConnID, uSessionId, pStrPktType));

        // print set of sessions sharing this connection
        _VoipConnectionPrintSessionIds(pConnectionlist, iConnID);

        NetPrintMem(pPacket, iSize, "packet data");
        #endif

        return;
    }

    // handle incoming packet
    switch (uPktType)
    {
        // connect packet
        case 'C':
        {
            VoipConnPacketT *pConnPacket = &pPacket->VoipConnPacket;
            int32_t iRemoteUserIndex;

            // got a connection packet
            NetPrintf(("voipconnection: [%d] (sess id 0x%08x) received conn packet bConnected=%d eState=%d uClientId=0x%08x\n",
                iConnID, uSessionId, pConnPacket->bConnected, pConnection->eState, uClientId));

            // copy remote user's info
            pConnection->iMaxRemoteUsers = pConnPacket->uNumLocalUsers - 1;
            pConnection->iMaxRemoteUsersExt = pConnection->iMaxRemoteUsers;

            for (iRemoteUserIndex = 0; iRemoteUserIndex < pConnection->iMaxRemoteUsersExt; iRemoteUserIndex++)
            {
                _VoipDecodeVoipUser(&pConnPacket->LocalUsers[iRemoteUserIndex], &pConnection->RemoteUsers[iRemoteUserIndex]);
            }

            _VoipDecodeRemoteHeadsetStatus(pConnection, &pConnPacket->Head);

            // flag that we received Conn packet from remote party
            pConnection->bConnPktRecv = TRUE;

            // if they've received data from us, consider the connection established
            if ((pConnPacket->bConnected) && (pConnection->eState == ST_CONN))
            {
                _VoipConnectionInit(pConnectionlist, pConnection, iConnID);
            }

            // if peer no more sees us as connected... go back to ST_CONN state and hope for connection to go back up
            if ((!pConnPacket->bConnected) && (pConnection->eState == ST_ACTV))
            {
                NetPrintf(("voipconnection: [%d] voip connection lost! try to re-establish. connection back to ST_CONN state (from ST_ACTV)... \n", iConnID));
                pConnection->eState = ST_CONN;
            }

            // update last received time
            pConnection->uLastRecv = uTick;
        }
        break;

        // ping packet
        case 'P':
        {
            VoipPingPacketT *pPingPacket = &pPacket->VoipPingPacket;
            int32_t iUserIndex;
            uint32_t uChannelUserIndices;
            const uint8_t *pRead = pPingPacket->aData;

            #if VOIP_CONNECTION_DEBUG
            char strGlobalVoipChannelConfig[80];
            char strUserVoipChannelConfig[20];
            ds_memclr(strGlobalVoipChannelConfig, sizeof(strGlobalVoipChannelConfig));
            ds_memclr(strUserVoipChannelConfig, sizeof(strUserVoipChannelConfig));
            NetPrintf(("voipconnection: [%d] (sess id 0x%08x) received ping packet from clientId=0x%08x\n", iConnID, uSessionId, uClientId));
            #endif

            if (pConnection->bConnPktRecv)
            {
                VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)VoipGetRef();

                // if we receive a ping in connection state, that means we're connected
                if (pConnection->eState == ST_CONN)
                {
                    _VoipConnectionInit(pConnectionlist, pConnection, iConnID);
                }

                // update connection status flags
                _VoipDecodeRemoteHeadsetStatus(pConnection, &pPingPacket->Head);

                // check to see if VOIP_REMOTE_USER_RECVVOICE flag should be deactivated for other users
                for (uIndex = 0; uIndex < VOIP_MAXLOCALUSERS; ++uIndex)
                {
                    if (NetTickDiff(uTick, pConnection->uUserLastRecv[uIndex]) > VOIP_PING_RATE)
                    {
                        pConnection->uRemoteUserStatus[uIndex] &= ~VOIP_REMOTE_USER_RECVVOICE;
                    }
                }

                pConnection->uRemoteConnStatus |= VOIP_CONN_BROADCONN;
                pConnection->uLastRecv = uTick;

                // read the bitset to figure which channel configurations need to be updated
                uChannelUserIndices = _VoipDecodeU32(pPingPacket->aChannelUserIndices);

                // update channel config for each remote user and remote shared user
                for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS_EXTENDED; iUserIndex++)
                {
                    if ((uChannelUserIndices >> iUserIndex) & 1)
                    {
                        pConnection->aRecvChannels[iUserIndex] = _VoipDecodeU32(pRead);
                        pRead += 4;
                        #if VOIP_CONNECTION_DEBUG
                        ds_snzprintf(strUserVoipChannelConfig, sizeof(strUserVoipChannelConfig), "user#%d:0x%08x   ", iUserIndex, pConnection->aRecvChannels[iUserIndex]);
                        ds_strnzcat(strGlobalVoipChannelConfig, strUserVoipChannelConfig, sizeof(strGlobalVoipChannelConfig));
                        #endif
                    }
                }
                #if VOIP_CONNECTION_DEBUG
                NetPrintf(("    voip channels:   %s\n", strGlobalVoipChannelConfig));
                #endif

                // process any reliable protocol components that may be included in this ping packet
                _VoipReliableDataInProcess(pConnectionlist, pConnection, &pPingPacket->Head, (uint8_t *)pRead);

                // update the channel
                if (memcmp(&pConnection->aRecvChannels, &pConnection->aLastRecvChannels, sizeof(pConnection->aRecvChannels)))
                {
                    VoipCommonProcessChannelChange(pVoipCommon, iConnID);
                    ds_memcpy_s(&pConnection->aLastRecvChannels, sizeof(pConnection->aLastRecvChannels), &pConnection->aRecvChannels, sizeof(pConnection->aRecvChannels));
                    VoipCommonUpdateRemoteStatus(pVoipCommon);
                }
            }
            else
            {
                NetPrintf(("voipconnection: [%d] (sess id 0x%08x) ping packet from clientId=0x%08x ignored because conn packet not yet received\n",
                    iConnID, uSessionId, uClientId));
            }
        }
        break;

        // data packet
        case 'M':
        {
            VoipMicrPacketT *pMicrPacket = &pPacket->VoipMicrPacket;
            VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)VoipGetRef();
            
            #if VOIP_CONNECTION_DEBUG
            if ((pConnection->iRecvPackets % 30) == 0)
            {
                char strSubPktSize[32];
                sprintf(strSubPktSize, "size %d (bytes)", pMicrPacket->MicrInfo.uSubPacketSize);
                NetPrintf(("voipconnection: [%d] (sess id 0x%08x) received pkt (seq#:%d) with %d sub-pkt(s) of %s from remote usr idx %d | voip channels:0x%08x\n",
                    iConnID, uSessionId, pMicrPacket->MicrInfo.uSeqn, pMicrPacket->MicrInfo.uNumSubPackets, (pMicrPacket->MicrInfo.uSubPacketSize == 0xFF ? "variable-length" : strSubPktSize),
                    pMicrPacket->MicrInfo.uUserIndex, _VoipDecodeU32(pMicrPacket->MicrInfo.channels.aChannelId)));
                NetPrintMem(pMicrPacket, iSize, "mic packet data");
            }
            #endif

            if (pConnection->bConnPktRecv)
            {
                uint8_t *pRead;
                int32_t iRemoteUserIndex;
                uint32_t uLocalHeadsetStatusAggregate = 0;

                // if we receive voice data in connection state, that means we're connected
                if (pConnection->eState == ST_CONN)
                {
                    _VoipConnectionInit(pConnectionlist, pConnection, iConnID);
                }

                // update connection status flags
                _VoipDecodeRemoteHeadsetStatus(pConnection, &pMicrPacket->Head);
                pConnection->uRemoteConnStatus |= (VOIP_CONN_ACTIVE|VOIP_CONN_BROADCONN);
                if (pMicrPacket->MicrInfo.uUserIndex == VOIP_SHARED_REMOTE_INDEX)
                {
                    pConnection->aRecvChannels[VOIP_SHARED_USER_INDEX] = _VoipDecodeU32(pMicrPacket->MicrInfo.channels.aChannelId);
                }
                else
                {
                    pConnection->aRecvChannels[pMicrPacket->MicrInfo.uUserIndex] = _VoipDecodeU32(pMicrPacket->MicrInfo.channels.aChannelId);
                }

                // update the channel
                if (memcmp(&pConnection->aRecvChannels, &pConnection->aLastRecvChannels, sizeof(pConnection->aRecvChannels)))
                {
                    VoipCommonProcessChannelChange(pVoipCommon, iConnID);
                    ds_memcpy_s(&pConnection->aLastRecvChannels, sizeof(pConnection->aLastRecvChannels), &pConnection->aRecvChannels, sizeof(pConnection->aRecvChannels));
                    VoipCommonUpdateRemoteStatus(pVoipCommon);
                }
                
                pConnection->uLastRecv = uTick;
                
                // validate and update packet sequence number
                if (pMicrPacket->MicrInfo.uSeqn != (uint8_t)(pConnection->uRecvSeqn+1))
                {
                    NetPrintf(("voipconnection: [%d] (sess id 0x%08x) got packet seq # %d while expecting packet seq # %d\n",
                        iConnID, uSessionId, pMicrPacket->MicrInfo.uSeqn, (uint8_t)pConnection->uRecvSeqn+1));
                }
                pConnection->uRecvSeqn = pMicrPacket->MicrInfo.uSeqn;

                // process any reliable protocol components that may be included in this MIC packet
                pRead = _VoipReliableDataInProcess(pConnectionlist, pConnection, &pMicrPacket->Head, &pMicrPacket->aData[0]);

                /* is the connection active, and are we sending to it?
                xboxone: After rebasing VoipHeadsetXboxOne to MS game chat 2, per-connection muting 
                is handled by VoipHeadsetXboxOne (See usage of +snd, -snd, +rcv, -rcv).
                But for crossplay we want the muting to be applied here just like other plaforms.
                Testing has shown that apply te send mask and +snd, -snd, +rcv, -rcv has no negative effect.*/
                if (pConnectionlist->uRecvMask & (1 << iConnID))
                {
                    // if the voice packet is from a remote shared user update all the remote user's status
                    if (pMicrPacket->MicrInfo.uUserIndex == VOIP_SHARED_REMOTE_INDEX)
                    {
                        for (iRemoteUserIndex = 0; iRemoteUserIndex < pConnection->iMaxRemoteUsers; ++iRemoteUserIndex)
                        {
                            VoipUserT *pVoipUser = (VoipUserT *)&pConnection->RemoteUsers[iRemoteUserIndex];

                            if (!VOIP_NullUser(pVoipUser))
                            {
                                #if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
                                /* For xboxone, opaque traffic is always received as if it was originated from the
                                   "shared user". We need to query VoipHeadset (via VoipCommon) to find out which 
                                   remote user is exactly detected as talking by the MS game chat 2's chat manager
                                   and consequently involved in generating the current inbound flow of voip packets. */
                                if (VoipCommonStatus((VoipCommonRefT *)VoipGetRef(), 'talk', 0, pVoipUser, sizeof(*pVoipUser)))
                                #endif
                                {
                                    pConnection->uRemoteUserStatus[iRemoteUserIndex] |= VOIP_REMOTE_USER_RECVVOICE;
                                    pConnection->uUserLastRecv[iRemoteUserIndex] = uTick;
                                }
                            }
                        }
                    }
                    else
                    {
                        pConnection->uUserLastRecv[pMicrPacket->MicrInfo.uUserIndex] = uTick;
                        pConnection->uRemoteUserStatus[pMicrPacket->MicrInfo.uUserIndex] |= VOIP_REMOTE_USER_RECVVOICE;
                    }
                    
                    // if there is at least one headset capable of outputting voice data, forward the packet along, ignore otherwise
                    // we do this to ignore spamming on PC (ticket GOS-30000)
                    for (uIndex = 0; uIndex < VOIP_MAXLOCALUSERS; ++uIndex)
                    {
                        uLocalHeadsetStatusAggregate |= pVoipCommon->Connectionlist.uLocalUserStatus[uIndex];
                    }

                    if (uLocalHeadsetStatusAggregate & VOIP_LOCAL_USER_OUTPUTDEVICEOK)
                    {
                        pConnectionlist->pVoiceCb((VoipUserT *)&pConnection->RemoteUsers[0], pConnection->iMaxRemoteUsers, iConnID, &pMicrPacket->MicrInfo, pRead, pConnectionlist->pVoiceUserData);
                    }
                }

                // check to see if VOIP_REMOTE_USER_RECVVOICE flag should be deactivated for other users
                for (uIndex = 0; uIndex < VOIP_MAXLOCALUSERS; ++uIndex)
                {
                    if (NetTickDiff(uTick, pConnection->uUserLastRecv[uIndex]) > VOIP_PING_RATE)
                    {
                        pConnection->uRemoteUserStatus[uIndex] &= ~VOIP_REMOTE_USER_RECVVOICE;
                    }
                }
            }
            else
            {
                NetPrintf(("voipconnection: [%d] (sess id 0x%08x) voip data packet from clientId=0x%08x ignored because conn packet not yet received\n",
                    iConnID, uSessionId, uClientId));
            }
        }
        break;

        case 'D':
        {
            VoipDiscPacketT *pDiscPacket = &pPacket->VoipDiscPacket;
            uint32_t uRemoteClientId = _VoipDecodeU32(pDiscPacket->aRemoteClientId);
            uint32_t uLocalClientId = pConnection->bIsLocalClientIdValid ? pConnection->uLocalClientId : pConnectionlist->uClientId;
            if (uRemoteClientId == uLocalClientId)
            {
                NetPrintf(("voipconnection: [%d] (sess id 0x%08x) remote peer 0x%08x has disconnected from 0x%08x\n",
                    iConnID, uSessionId, uClientId, uRemoteClientId));

                // In this specific case, we know that remote peer is already in 'disconnected' state.
                // No need to send back a DISC msg. Therefore, set bSendDiscMsg parameter to FALSE.
                _VoipConnectionStop(pConnectionlist, &pConnectionlist->pConnections[iConnID], iConnID, FALSE);
            }
            else
            {
                // this should only occurs with multiple machines on the same PC and voip being routed to the incorrect client
                NetPrintf(("voipconnection: [%d] (sess id 0x%08x) remote peer 0x%08x has disconnected from 0x%08x, but we are 0x%08x\n",
                    iConnID, uSessionId, uClientId, uRemoteClientId, pConnectionlist->uClientId));
            }
        }
        break;

        default:
        {
            NetPrintf(("voipconnection: [%d] critical error! this path can't be taken in _VoipConnectionRecvSing()\n"));
        }
        break;
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionUpdateSingle

    \Description
        Update a virtual connection.

    \Input *pConnectionlist - connectionlist
    \Input *pConnection     - pointer to connection to update
    \Input iConnId          - index of connection to update
    \Input uTick            - current tick count

    \Output
        uint32_t            - nonzero=receiving voice, zero=not receiving voice

    \Version 03/20/2004 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _VoipConnectionUpdateSingle(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, int32_t iConnId, uint32_t uTick)
{
    int32_t iTimeout;
    int32_t iUserIndex;
    uint8_t bIsReceivingVoice = FALSE;

    // make sure it's a valid connection
    if (pConnection->eState == ST_DISC)
    {
        return(0);
    }

    // see if we need to send a conn or ping packet
    if ((uTick - pConnection->uLastSend) > VOIP_PING_RATE)
    {
        // if we're connecting, send connection packet
        if (pConnection->eState == ST_CONN)
        {
            _VoipConnectionConn(pConnectionlist, pConnection, uTick);
        }
        else
        {
            _VoipConnectionPing(pConnectionlist, pConnection, uTick);
        }
    }

    // see if we need to send any buffered voice data
    for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS_EXTENDED; iUserIndex++)
    {
        _VoipConnectionSendSingle(pConnectionlist, pConnection, iUserIndex, uTick, FALSE);
    }

    // see if we need to time out the connection
    iTimeout = (pConnection->eState == ST_ACTV) ? pConnectionlist->iDataTimeout : VOIP_CONNTIMEOUT;
    if (NetTickDiff(uTick, pConnection->uLastRecv) > iTimeout)
    {
        NetPrintf(("voipconnection: [%d] timing out connection due to inactivity\n", iConnId));
        _VoipConnectionStop(pConnectionlist, &pConnectionlist->pConnections[iConnId], iConnId, TRUE);
    }

    // return if we are receiving voice or not
    for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS; iUserIndex++)
    {
        if (pConnection->uRemoteUserStatus[iUserIndex] & VOIP_REMOTE_USER_RECVVOICE)
        {
            bIsReceivingVoice = TRUE;
            break;
        }
    }
    return((bIsReceivingVoice == TRUE) ? (1 << iConnId) : 0);
}

/*F********************************************************************************/
/*!
    \Function _VoipConnectionRecv

    \Description
        Receive data and forward it to the appropriate connection.

    \Input *pConnectionlist - connectionlist

    \Output
        int32_t             - amount of data received

    \Version 03/21/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipConnectionRecv(VoipConnectionlistT *pConnectionlist)
{
    struct sockaddr_in RecvAddr;
    int32_t iAddrLen, iRecvSize;

    VoipPacketBufferT RecvPacket;

    // no socket?  no way to receive.
    if (pConnectionlist->pSocket == NULL)
    {
        return(0);
    }

    // try and receive from a peer
    if ((iRecvSize = SocketRecvfrom(pConnectionlist->pSocket, (char *)&RecvPacket, sizeof(RecvPacket), 0, (struct sockaddr *)&RecvAddr, (iAddrLen=sizeof(RecvAddr),&iAddrLen))) > 0)
    {
        VoipConnectionT *pConnection;
        int32_t iConnID;

        // extract client identifier
        uint32_t uClientId = _VoipDecodeU32(RecvPacket.VoipPacketHead.aClientId);

        // find the matching connection
        for (iConnID = 0, pConnection = NULL; iConnID < pConnectionlist->iMaxConnections; iConnID++)
        {
            pConnection = &pConnectionlist->pConnections[iConnID];

            // if the clientID matches, this is the connection
            if (uClientId == pConnection->uRemoteClientId)
            {
                break;
            }
        }

        // if we found a matching connection
        if (iConnID < pConnectionlist->iMaxConnections)
        {
            // give the data to the connection
            _VoipConnectionRecvSingle(pConnectionlist, pConnection, &RecvAddr, &RecvPacket, iRecvSize);
        }
        else if (!VOIP_SamePacketType(&RecvPacket.VoipPacketHead, &_Voip_ConnPacket) && !VOIP_SamePacketType(&RecvPacket.VoipPacketHead, &_Voip_DiscPacket))
        {
            NetPrintf(("voipconnection: ignoring 0x%02x%02x%02x packet from address %a:%d clientId=0x%08x\n",
                RecvPacket.VoipPacketHead.aType[0], RecvPacket.VoipPacketHead.aType[1], RecvPacket.VoipPacketHead.aType[2],
                SocketNtohl(RecvAddr.sin_addr.s_addr), SocketNtohs(RecvAddr.sin_port),
                uClientId));
            NetPrintMem(&RecvPacket, iRecvSize, "packet data");
        }
    }

    // return data length
    return(iRecvSize);
}

/*F********************************************************************************/
/*!
    \Function _VoipConnectionRecvCallback

    \Description
        Voip socket recv event callback.

    \Input *pSocket - voip socket
    \Input iFlags   - unused
    \Input *pRef    - connectionlist ref

    \Output
        int32_t     - zero

    \Version 12/16/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipConnectionRecvCallback(SocketT *pSocket, int32_t iFlags, void *pRef)
{
    VoipConnectionlistT *pConnectionlist = (VoipConnectionlistT *)pRef;

    // see if we have exclusive access
    if (NetCritTry(&pConnectionlist->NetCrit))
    {
        // try and receive data
        _VoipConnectionRecv(pConnectionlist);

        // free access
        NetCritLeave(&pConnectionlist->NetCrit);
    }
    else
    {
        NetPrintf(("voipconnection: _VoipConnectionRecvCallback() could not acquire connectionlist critical section\n"));
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipConnectionTrySocketClose

    \Description
        Check if the voip socket can be closed. The close will occur when all the
        connections are closed. Moved from the voip loop, to also be done immediately
        upon connection stop.

    \Input *pConnectionlist - connectionlist ref

    \Output
        int32_t     - zero

    \Version 06/18/2009 (jrainy)
*/
/********************************************************************************F*/
static void _VoipConnectionTrySocketClose(VoipConnectionlistT *pConnectionlist)
{
    int32_t iConnId, iNumConnections;

    NetCritEnter(&pConnectionlist->NetCrit);

    // count number of active connections
    for (iConnId = 0, iNumConnections = 0; iConnId < pConnectionlist->iMaxConnections; iConnId++)
    {
        if (pConnectionlist->pConnections[iConnId].eState != ST_DISC)
        {
            iNumConnections++;
        }
    }

    // if no connections left, kill socket
    if ((iNumConnections == 0) && (pConnectionlist->pSocket != NULL))
    {
        NetPrintf(("voipconnection: closing socket in _VoipConnectionTrySocketClose()\n"));
        SocketClose(pConnectionlist->pSocket);
        pConnectionlist->pSocket = NULL;
        pConnectionlist->uBindPort = 0;
    }
    NetCritLeave(&pConnectionlist->NetCrit);
}

/*F********************************************************************************/
/*!
    \Function _VoipConnectionStop

    \Description
        Stop a connection.

    \Input *pConnectionlist - connectionlist ref
    \Input *pConnection     - connection to stop
    \Input iConnID          - index of connection to stop, or VOIP_CONNID_ALL to stop all connections
    \Input bSendDiscMsg     - TRUE - send DISC msg to peer; FALSE - do not send DISC msg to peer

    \Notes
        Loss of the single unreliable disconnect packet sent in this function
        is not critical, as the connection manager will respond to unexpected
        packets that are not connect/disconnect packets (ie pings and voice
        packets) with disconnection requests.

    \Version 05/10/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipConnectionStop(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, int32_t iConnID, int32_t bSendDiscMsg)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)VoipGetRef();
    int32_t i = 0; 

    NetCritEnter(&pConnectionlist->NetCrit);

    // make sure we're not already disconnected
    if (pConnection->eState == ST_DISC)
    {
        NetPrintf(("voipconnection: [%d] disconnection attempt canceled because connection already in state ST_DISC!\n", iConnID));
        NetCritLeave(&pConnectionlist->NetCrit);
        return;
    }

    if (bSendDiscMsg)
    {
        // send a disconnection packet
        uint32_t uLocalClientId = pConnection->bIsLocalClientIdValid ? pConnection->uLocalClientId : pConnectionlist->uClientId;
        _VoipConnectionDisc(iConnID, uLocalClientId, pConnection->uRemoteClientId, pConnection->uSessionId[0], &pConnection->SendAddr, pConnectionlist->pSocket);
    }
    else
    {
        NetPrintf(("voipconnection: [%d] no need to send disconnect message to %a:%d\n", iConnID,
            SocketNtohl(pConnection->SendAddr.sin_addr.s_addr), SocketNtohs(pConnection->SendAddr.sin_port)));
    }

    // set to disconnected before unregister
    pConnection->eState = ST_DISC;
    pConnection->bConnPktRecv = FALSE;

    // subtract connection identifier from send/recv masks
    VoipConnectionSetSendMask(pConnectionlist, pConnectionlist->uSendMask & ~(1 << iConnID));
    VoipConnectionSetRecvMask(pConnectionlist, pConnectionlist->uRecvMask & ~(1 << iConnID));

    // unregister all the remote users on this connection with voipheadset
    VoipConnectionRegisterRemoteTalkers(pConnectionlist, iConnID, FALSE);

    // clear outbound reliable queue
    while (pConnection->pOutboundReliableDataQueue != NULL)
    {
        _VoipConnectionReliableDataDequeue(pConnectionlist, iConnID);
    }

    // clear connection and mark as disconnected
    ds_memclr(pConnection, sizeof(*pConnection));
    pConnection->uRemoteConnStatus = VOIP_CONN_STOPPED;
    pConnection->iVoipServerConnId = VOIP_CONNID_NONE;
    for ( i = 0; i < VOIP_MAXLOCALUSERS; ++i)
    {
        pConnectionlist->uLocalUserStatus[i] &= ~VOIP_LOCAL_USER_SENDVOICE;
        pConnectionlist->uLocalUserLastSend[i] = 0;
    }

    // clear channel config maintained by VoipCommon
    ds_memclr(&pVoipCommon->uRemoteChannelSelection[iConnID], sizeof(pVoipCommon->uRemoteChannelSelection[iConnID]));

    NetCritLeave(&pConnectionlist->NetCrit);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipConnectionStartup

    \Description
        Startup a connectionlist.

    \Input *pConnectionlist - connectionlist to init
    \Input iMaxPeers        - max number of connections to support

    \Output
        int32_t             - zero=success, negative=error

    \Version 03/20/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipConnectionStartup(VoipConnectionlistT *pConnectionlist, int32_t iMaxPeers)
{
    int32_t iConnId;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    //Check to see if Max Peer has exceeded VOIP_MAXCONNECT
    if (iMaxPeers > VOIP_MAXCONNECT)
    {              
        NetPrintf(("voipconnection: error, max peer exceeded VOIP_MAXCONNECT.\n"));
        return(-2);        
    }

    // Query current mem group data
    VoipCommonMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init connectionlist
    if ((pConnectionlist->pConnections = (VoipConnectionT *)DirtyMemAlloc(sizeof(VoipConnectionT) * iMaxPeers, VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipconnection: error, unable to allocate connectionlist\n"));
        return(-1);
    }
    ds_memclr(pConnectionlist->pConnections, sizeof(VoipConnectionT) * iMaxPeers);

    pConnectionlist->iFriendConnId = VOIP_CONNID_NONE;

    // reset local-to-server conn id mappings
    for (iConnId = 0; iConnId < iMaxPeers; iConnId++)
    {
        pConnectionlist->pConnections[iConnId].iVoipServerConnId = VOIP_CONNID_NONE;
    }

    // init critical section
    NetCritInit(&pConnectionlist->NetCrit, "voipconnection");

    // set default timeout
    pConnectionlist->iDataTimeout = VOIP_TIMEOUT;

    // set max peers
    pConnectionlist->iMaxConnections = iMaxPeers;

    NetPrintf(("voipconnection: max connections = %d   max reliable data size = %d   max ping payload size = %d\n",
        pConnectionlist->iMaxConnections, VOIP_MAXRELIABLEDATA, VOIP_MAXPINGPKTSIZE));

    return(0);
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionSetCallbacks

    \Description
        Set required recv voice and reg user callbacks.

    \Input *pConnectionlist     - connectionlist to register callbacks for
    \Input *pVoiceCb            - voice callback to register
    \Input *pVoiceUserData      - user data to be passed to the voice callback
    \Input *pTextCb             - text callback to register
    \Input *pTextUserData       - user data to be passed to the text callback
    \Input *pRegUserCb          - user reg callback to register
    \Input *pRegUserUserData    - user data to be passed to the reguser callback
    \Input *pOpaqueCb           - opaque data callback to register
    \Input *pOpaqueUserData     - user data to be passed to the opaquedata callback

    \Version 03/20/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipConnectionSetCallbacks(VoipConnectionlistT *pConnectionlist, VoipConnectRecvVoiceCbT *pVoiceCb, void *pVoiceUserData,
                                                                      VoipConnectRecvTextCbT *pTextCb, void *pTextUserData, 
                                                                      VoipConnectRegUserCbT *pRegUserCb, void *pRegUserUserData, 
                                                                      VoipConnectRecvOpaqueCbT *pOpaqueCb, void *pOpaqueUserData)
{
    // save callbacks and callback user data
    pConnectionlist->pRegUserCb = pRegUserCb;
    pConnectionlist->pRegUserUserData = pRegUserUserData;
    pConnectionlist->pVoiceCb = pVoiceCb;
    pConnectionlist->pVoiceUserData = pVoiceUserData;
    pConnectionlist->pTextCb = pTextCb;
    pConnectionlist->pTextUserData = pTextUserData;
    pConnectionlist->pOpaqueCb = pOpaqueCb;
    pConnectionlist->pOpaqueUserData = pOpaqueUserData;
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionSetTextCallback

    \Description
        Set required text callbacks for X1 crossplay asscessibility 
        Should be set to null for native mode

    \Input *pConnectionlist     - connectionlist to register callbacks for
    \Input *pTextCb             - text callback to register
    \Input *pTextUserData       - user data to be passed to the text callback

    \Version 04/10/2019 (tcho)
*/
/********************************************************************************F*/
void VoipConnectionSetTextCallback(VoipConnectionlistT *pConnectionlist, VoipConnectRecvTextCbT *pTextCb, void *pTextUserData)
{
    // save callbacks and callback user data
    pConnectionlist->pTextCb = pTextCb;
    pConnectionlist->pTextUserData = pTextUserData;
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionShutdown

    \Description
        Shutdown a connectionlist

    \Input *pConnectionlist - connectionlist to close

    \Version 03/20/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipConnectionShutdown(VoipConnectionlistT *pConnectionlist)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;
    LinkedReliableDataT *pEntry;

    // query current mem group data
    VoipCommonMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // clear free pool of reliable data entries
    while ((pEntry = _VoipGetReliableDataBufferFromFreePool(pConnectionlist, FALSE)) != NULL)
    {
        DirtyMemFree(pEntry, VOIP_MEMID, iMemGroup, pMemGroupUserData);
    }

    // dispose of connectionlist
    DirtyMemFree(pConnectionlist->pConnections, VOIP_MEMID, iMemGroup, pMemGroupUserData);

    // dispose of socket
    if (pConnectionlist->pSocket)
    {
        NetPrintf(("voipconnection: closing socket in VoipConnectionShutdown()\n"));
        SocketClose(pConnectionlist->pSocket);
    }

    // release critical section
    NetCritKill(&pConnectionlist->NetCrit);

    // clear connectionlist
    ds_memclr(pConnectionlist, sizeof(*pConnectionlist));
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionCanAllocate

    \Description
        Check whether a given Connection ID can be allocated in this Connection List

    \Input *pConnectionlist - connection list ref
    \Input iConnID          - connection index

    \Output
        uint8_t             - Whether a connection can be allocated with given ConnID

    \Version 02/19/2008 (jrainy)
*/
/********************************************************************************F*/
uint8_t VoipConnectionCanAllocate(VoipConnectionlistT *pConnectionlist, int32_t iConnID)
{
    if ((iConnID < 0) || (iConnID >= pConnectionlist->iMaxConnections))
    {
        return(FALSE);
    }

    return(pConnectionlist->pConnections[iConnID].eState == ST_DISC);
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionStart

    \Description
        Start a connection to a peer.

    \Input *pConnectionlist - connection list ref
    \Input iConnID          - connection index
    \Input uAddr            - address to connect to
    \Input uConnPort        - connection port
    \Input uBindPort        - local bind port
    \Input uClientId        - id of client to connect to
    \Input uSessionId       - session id (cannot be 0)

    \Output
        int32_t             - connection identifier on success, negative=failure

    \Version 03/18/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipConnectionStart(VoipConnectionlistT *pConnectionlist, int32_t iConnID, uint32_t uAddr, uint32_t uConnPort, uint32_t uBindPort, uint32_t uClientId, uint32_t uSessionId)
{
    VoipConnectionT *pConnection;
    struct sockaddr BindAddr;
    int32_t iUserIndex;

    // sanity check: make sure local and remote clientIDs are different
    if (pConnectionlist->uClientId == uClientId)
    {
        NetPrintf(("voipconnection: local client id (%d) and remote client id (%d) can't be same\n", pConnectionlist->uClientId, uClientId));
        return(-1);
    }

    // if we haven't allocated a socket yet, do so now
    if (pConnectionlist->pSocket == NULL)
    {
        int32_t iResult;

        // open the socket
        if ((pConnectionlist->pSocket = SocketOpen(AF_INET, SOCK_DGRAM, VOIP_IPPROTO)) == NULL)
        {
            NetPrintf(("voipconnection: error creating socket\n"));
            return(-2);
        }

        // bind the socket
        SockaddrInit(&BindAddr, AF_INET);
        SockaddrInSetPort(&BindAddr, uBindPort);
        if ((iResult = SocketBind(pConnectionlist->pSocket, &BindAddr, sizeof(BindAddr))) != SOCKERR_NONE)
        {
            NetPrintf(("voipconnection: error %d binding socket to port %d, trying random\n", iResult, uBindPort));
            SockaddrInSetPort(&BindAddr, 0);
            if ((iResult = SocketBind(pConnectionlist->pSocket, &BindAddr, sizeof(BindAddr))) != SOCKERR_NONE)
            {
                NetPrintf(("voipconnection: error %d binding socket\n", iResult));
                SocketClose(pConnectionlist->pSocket);
                pConnectionlist->pSocket = NULL;
                return(-3);
            }
        }

        // retrieve bound port
        SocketInfo(pConnectionlist->pSocket, 'bind', 0, &BindAddr, sizeof(BindAddr));
        uBindPort = SockaddrInGetPort(&BindAddr);
        NetPrintf(("voipconnection: bound socket to port %d\n", uBindPort));

        // save local port
        pConnectionlist->uBindPort = uBindPort;

        // setup for socket events
        SocketCallback(pConnectionlist->pSocket, CALLB_RECV, 5000, pConnectionlist, &_VoipConnectionRecvCallback);
    }

    // make sure bind matches our previous bind
    if (uBindPort != pConnectionlist->uBindPort)
    {
        NetPrintf(("voipconnection: warning, only one global bind port is currently supported, using previously specified port\n"));
    }

    // convert address and ports to network form
    uAddr = SocketHtonl(uAddr);
    uConnPort = SocketHtons(uConnPort);

    /*
    Guarding the following code with NetCrit proved to be required to protect against
    _VoipConnectionStop() being invoked from the socket async recv thread upon reception
    of a DISC packet (typically belonging to a previous session) on that connection.

    We did not include the above portion of this function within this critical
    usage function, because atomic access to it is guaranteed by the threadcrit
    being lock externally.
    */
    NetCritEnter(&pConnectionlist->NetCrit);

    // allocate a connection
    if ((pConnection = _VoipConnectionAllocate(pConnectionlist, iConnID)) == NULL)
    {
        NetCritLeave(&pConnectionlist->NetCrit);

        NetPrintf(("voipconnection: alloc failed\n"));
        return(-4);
    }

    iConnID = VOIP_ConnID(pConnectionlist, pConnection);

    // init the connection
    ds_memclr(pConnection, sizeof(*pConnection));
    pConnection->iVoipServerConnId = VOIP_CONNID_NONE;
    pConnection->SendAddr.sin_family = AF_INET;
    pConnection->SendAddr.sin_addr.s_addr = uAddr;
    pConnection->SendAddr.sin_port = uConnPort;
    pConnection->uRemoteClientId = uClientId;

    // sequence number for reliable data ranges from 1 to 127
    pConnection->uOutSeqNb = 1;   // use 1 as the first outbound sequence number 
    pConnection->uInSeqNb = 0;    // seq nb of last received inbound message. validity range : [1,127]. 0 means no inbound traffic received yet

    // add specified session ID to the set of sessions sharing this connection
    VoipConnectionAddSessionId(pConnectionlist, iConnID, uSessionId);

    pConnection->uRecvSeqn = 0xFFFFFFFF;   // intialize last receive sequence number with -1.
    pConnection->uRemoteConnStatus = 0;
    for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS; ++iUserIndex)
    {
        pConnection->uRemoteUserStatus[iUserIndex] = 0;
    }
    pConnection->eState = ST_CONN;
    pConnection->uLastRecv = NetTick();

    // set up to transmit mic data
    for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS_EXTENDED; iUserIndex++)
    {
        ds_memcpy_s(&pConnection->VoipMicrPacket[iUserIndex].Head, sizeof(pConnection->VoipMicrPacket[iUserIndex].Head), &_Voip_MicrPacket, sizeof(_Voip_MicrPacket));
    }

    // output connect message
    NetPrintf(("voipconnection: [%d] connecting to %a:%d localId=0x%08x  remoteId=0x%08x\n", iConnID,
        SocketNtohl(pConnection->SendAddr.sin_addr.s_addr), SocketNtohs(pConnection->SendAddr.sin_port),
        pConnection->uLocalClientId, pConnection->uRemoteClientId));

    NetCritLeave(&pConnectionlist->NetCrit);

    // return connection ID to caller
    return(iConnID);
}

/*F********************************************************************************/
/*!
    \Function   VoipConnectionUpdate

    \Description
        Update all connections

    \Input *pConnectionlist - connection list

    \Version 03/18/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipConnectionUpdate(VoipConnectionlistT *pConnectionlist)
{
    int32_t iConnId;
    uint32_t uTick;

    // get sole access
    NetCritEnter(&pConnectionlist->NetCrit);

    // receive any incoming data
    while(_VoipConnectionRecv(pConnectionlist) > 0)
        ;

    _VoipConnectionTrySocketClose(pConnectionlist);

    // relinquish sole access
    NetCritLeave(&pConnectionlist->NetCrit);

    // update connection status for all connections, keeping track of who we are receiving voice from
    for (iConnId = 0, uTick = NetTick(), pConnectionlist->uRecvVoice = 0; iConnId < pConnectionlist->iMaxConnections; iConnId++)
    {
        pConnectionlist->uRecvVoice |= _VoipConnectionUpdateSingle(pConnectionlist, &pConnectionlist->pConnections[iConnId], iConnId, uTick);
    }

    #if !defined(DIRTYCODE_XBOXONE) && !defined(DIRTYCODE_GDK)
    /* check if we need to time out talking status 
       on xbox one this is handled in the voipxbxone _VoipUpdateLocalStatus()
    */
    for (int32_t i = 0; i < VOIP_MAXLOCALUSERS; ++i)
    {
        if ((pConnectionlist->uLocalUserStatus[i] & VOIP_LOCAL_USER_TALKING) && (NetTickDiff(uTick, pConnectionlist->uLastVoiceTime[i]) > VOIP_TALKTIMEOUT))
        {
            pConnectionlist->uLocalUserStatus[i] &= ~VOIP_LOCAL_USER_TALKING;
        }
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionStop

    \Description
        Stop a connection with a peer.

    \Input *pConnectionlist - connection list ref
    \Input iConnID          - connection to stop, or VOIP_CONNID_ALL to stop all connections
    \Input bSendDiscMsg     - TRUE - send DISC msg to peer; FALSE - do not send DISC msg to peer

    \Version 03/18/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipConnectionStop(VoipConnectionlistT *pConnectionlist, int32_t iConnID, int32_t bSendDiscMsg)
{
    if (iConnID == VOIP_CONNID_ALL)
    {
        // disconnect from all current connections
        for (iConnID = 0; iConnID < pConnectionlist->iMaxConnections; iConnID++)
        {
            _VoipConnectionStop(pConnectionlist, &pConnectionlist->pConnections[iConnID], iConnID, bSendDiscMsg);
        }
    }
    else if ((iConnID >= 0) && (iConnID < pConnectionlist->iMaxConnections))
    {
        // disconnect from the given connection
        _VoipConnectionStop(pConnectionlist, &pConnectionlist->pConnections[iConnID], iConnID, bSendDiscMsg);
    }
    else
    {
        NetPrintf(("voipconnection: disconnect with iConnID=%d is invalid\n", iConnID));
    }

    _VoipConnectionTrySocketClose(pConnectionlist);
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionSend

    \Description
        Send data to peer.

    \Input *pConnectionlist - connectionlist to send to
    \Input uSendMask        - mask of connections to send to
    \Input *pVoiceData      - pointer to data to send
    \Input iDataSize        - size of data to send
    \Input *pMetaData       - pointer to metadata to be added to voip packet
    \Input iMetaDataSize    - size of metadata
    \Input uUserIndex       - local user index
    \Input uSendSeqn        - seq nb

    \Version 03/17/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipConnectionSend(VoipConnectionlistT *pConnectionlist, uint32_t uSendMask, const uint8_t *pVoiceData, int32_t iDataSize, const uint8_t *pMetaData, int32_t iMetaDataSize, uint32_t uUserIndex, uint8_t uSendSeqn)
{
    uint32_t uCurTick = NetTick();
    int32_t iConnID;
    int32_t iParticipatingUserIndex;
    uint8_t bSentToVoipServer = FALSE;

    // early exit if metadata is too big
    if ((pMetaData != NULL) && (iMetaDataSize > 256))
    {
        NetPrintf(("voipconnection: critical error! metadata is too big\n"));
        return;
    }

    // early exit if the user index is invalid
    if (uUserIndex >= VOIP_MAXLOCALUSERS_EXTENDED)
    {
        return;
    }

    // if we are dealing with the remote user update all the user's last voice receive time
    if (uUserIndex == VOIP_SHARED_USER_INDEX)
    {
        for (iParticipatingUserIndex = 0; iParticipatingUserIndex < VOIP_MAXLOCALUSERS; ++iParticipatingUserIndex)
        {
            if (pConnectionlist->aIsParticipating[iParticipatingUserIndex] == TRUE)
            {
                pConnectionlist->uLastVoiceTime[iParticipatingUserIndex] = uCurTick;
            }
        }
    }
    else
    {
        pConnectionlist->uLastVoiceTime[uUserIndex] = uCurTick;
    }

    // loop through all connections
    for (iConnID = 0; iConnID < pConnectionlist->iMaxConnections; iConnID++)
    {
        // ref the connection
        VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnID];

        // skip connections that are not set in the sendmask parameter
        if ((uSendMask & (1 << iConnID)) == 0)
        {
            continue;
        }

        // for connections via voip server, send only to the first connection
        // todo: amakoukji, for CCS phase 2 we will need to remember what voip server we already sent the packet to and skip 
        if ((pConnection->iVoipServerConnId != VOIP_CONNID_NONE) && (bSentToVoipServer != FALSE))
        {
            continue;
        }

        /* is the connection active, and are we sending to it?
           xboxone: After rebasing VoipHeadsetXboxOne to MS game chat 2, per-connection muting 
                    is handled by VoipHeadsetXboxOne (See usage of +snd, -snd, +rcv, -rcv).
                    But for crossplay we want the muting to be applied here just like other plaforms.
                    Testing has shown that apply te send mask and +snd, -snd, +rcv, -rcv has no negative effect.*/

        if ((pConnection->eState == ST_ACTV) && (pConnectionlist->uSendMask & (1 << iConnID)))
        {
            uint8_t *pWrite; // write position in voip packet
            VoipMicrPacketT *pMicrPacket = &pConnection->VoipMicrPacket[uUserIndex];
            uint8_t bVariableSubPktLen;
            int32_t iResult;

            // set talking flag
            // if the voice packet is from a shared user set all the participating user status to be talking
            if (uUserIndex == VOIP_SHARED_USER_INDEX)
            {
                for (iParticipatingUserIndex = 0; iParticipatingUserIndex < VOIP_MAXLOCALUSERS; ++iParticipatingUserIndex)
                {
                    if (pConnectionlist->aIsParticipating[iParticipatingUserIndex] == TRUE)
                    {
                        pConnectionlist->uLocalUserStatus[iParticipatingUserIndex] |= VOIP_LOCAL_USER_TALKING;
                    }
                }
            }
            else
            {
                pConnectionlist->uLocalUserStatus[uUserIndex] |= VOIP_LOCAL_USER_TALKING;
            }

            // record maximum outgoing sub-pkt size on this connection
            if (iDataSize > pConnection->iMaxSubPktSize)
            {
                NetPrintf(("voipconnection: [%d] max recorded outbound sub-pkt size increased from %d to %d\n", iConnID, pConnection->iMaxSubPktSize, iDataSize));
                pConnection->iMaxSubPktSize = iDataSize;
            }

            // is new metadata different than metadata already in packet
            if ((pMicrPacket->MicrInfo.uNumSubPackets != 0) && (pMetaData != NULL))
            {
                uint8_t bDifferentMetaData = FALSE;
                uint8_t *pPackedMetaData = _VoipPacketGetMetaDataPtr(pMicrPacket);

                #if DIRTYCODE_LOGGING
                if (!(pMicrPacket->Head.uFlags & VOIP_PACKET_STATUS_FLAG_METADATA))
                {
                    NetPrintf(("voipconnection: [%d] critical error - metadata expected in packet but missing\n", iConnID));
                }
                #endif

                if (iMetaDataSize == *pPackedMetaData)
                {
                    if (memcmp(pMetaData, (pPackedMetaData+1), iMetaDataSize) != 0)
                    {
                        bDifferentMetaData = TRUE;
                    }
                }
                else
                {
                    bDifferentMetaData = TRUE;
                }

                if (bDifferentMetaData)
                {
                    NetPrintf(("voipconnection: [%d] flushing pending voip packet because new sub-packet has different metadata\n", iConnID));
                    _VoipConnectionSendSingle(pConnectionlist, pConnection, uUserIndex, uCurTick, TRUE);
                }
            }

            // if no voice sub-packets are queued yet
            if (pMicrPacket->MicrInfo.uNumSubPackets == 0)
            {
                // if needed, add reliable protocol components to this voip packet
                if (pConnection->iVoipServerConnId != VOIP_CONNID_NONE)
                {
                    // packet routed through voip server
                    pWrite = _VoipReliableDataOutProcess(pConnectionlist, VOIP_CONNID_ALL, &pMicrPacket->Head, &pMicrPacket->aData[0], sizeof(pMicrPacket->aData));
                }
                else
                {
                    // p2p packet
                    pWrite = _VoipReliableDataOutProcess(pConnectionlist, iConnID, &pMicrPacket->Head, &pMicrPacket->aData[0], sizeof(pMicrPacket->aData));
                }

                // if the voice timer has elapsed, reset it
                if (NetTickDiff(uCurTick, pConnection->aVoiceSendTimer[uUserIndex]) >= 0)
                {
                    pConnection->aVoiceSendTimer[uUserIndex] = uCurTick + VOIP_MSPERPACKET;
                }

                // if necessary, add metadata before we start appending the first packet
                if (pMetaData != NULL)
                {
                    // store metadata length in packet buffer
                    *pWrite++ = (unsigned)iMetaDataSize;

                    // copy metadata into packet buffer
                    ds_memcpy(pWrite, pMetaData, iMetaDataSize);
                    pWrite += iMetaDataSize;

                    // set flag signaling that payload starts with metadata size/metadata pair
                    pMicrPacket->Head.uFlags |= VOIP_PACKET_STATUS_FLAG_METADATA;
                }
            }
            else
            {
                pWrite = _VoipPacketGetWritePtr(pMicrPacket);
            }

            // does this platform work with variable length sub-pkts
            if ((iResult = VoipStatus(VoipGetRef(), 'vlen', 0, &bVariableSubPktLen, sizeof(bVariableSubPktLen))) < 0)
            {
                // if selector is not supported, assume fixed length
                bVariableSubPktLen = FALSE;
            }

            if (bVariableSubPktLen)
            {
                // MicrInfo.uSubPacketSize == 0xFF means that sub-pkts may have different sizes 
                pMicrPacket->MicrInfo.uSubPacketSize = 0xFF;
            }
            else
            {
                #if DIRTYCODE_LOGGING
                if (pMicrPacket->MicrInfo.uSubPacketSize != 0 && pMicrPacket->MicrInfo.uSubPacketSize != iDataSize)
                {
                    NetPrintf(("voipconnection: [%d] critical error - sub-packets have different size -> %d vs %d\n", iConnID, pMicrPacket->MicrInfo.uSubPacketSize, iDataSize));
                }
                if (pMicrPacket->MicrInfo.uSubPacketSize == 0xFF)
                {
                    NetPrintf(("voipconnection: [%d] critical error - sub-pkt size conflicting with fixed-length sub-pkt mode\n", iConnID));
                }
                #endif
                pMicrPacket->MicrInfo.uSubPacketSize = (unsigned)iDataSize;
            }

            if (bVariableSubPktLen)
            {
                // store sub-pkt length in packet buffer
                *pWrite = (unsigned)iDataSize;

                // copy data into packet buffer
                ds_memcpy(pWrite+1, pVoiceData, iDataSize);

                #if DIRTYCODE_LOGGING
                if ((pWrite + 1 + iDataSize) >= ((&pConnection->VoipMicrPacket[uUserIndex].aData[0]) + sizeof(pConnection->VoipMicrPacket[uUserIndex].aData)))
                {
                    NetPrintf(("voipconnection: [%d] critical error - sub-packet packing overflowed!\n", iConnID));
                }
                #endif
            }
            else
            {
                // copy data into packet buffer
                ds_memcpy(pWrite, pVoiceData, iDataSize);
            }

            // increment sub-packet count
            pMicrPacket->MicrInfo.uNumSubPackets += 1;

            // identify local user index that generated the voice data
            pMicrPacket->MicrInfo.uUserIndex = uUserIndex;

            // see if we need to send any buffered voice data
            _VoipConnectionSendSingle(pConnectionlist, pConnection, uUserIndex, uCurTick, FALSE);

            if (pConnection->iVoipServerConnId != VOIP_CONNID_NONE)
            {
                // mark game server as served
                bSentToVoipServer = TRUE;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionFlush

    \Description
        Send currently queued voice data, if any.

    \Input *pConnectionlist - connectionlist to send to
    \Input iConnID          - connection ident of connection to flush

    \Output
        int32_t             - number of voice packets flushed

    \Notes
        Currently only useful on Xenon

    \Version 01/04/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipConnectionFlush(VoipConnectionlistT *pConnectionlist, int32_t iConnID)
{
    int32_t iUserIndex;
    int32_t iNumSubPackets = 0;
    int32_t iTotalNumSubPackets = 0;

    // ref the connection
    VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnID];

    for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS_EXTENDED; iUserIndex++)
    {
        iNumSubPackets = (signed)pConnection->VoipMicrPacket[iUserIndex].MicrInfo.uNumSubPackets;

        /* is the connection active, and are we sending to it?
           xboxone: Never "mute" here, i.e. skip applying pConnectionlist->uSendMask.
                    Per-connection muting handling is rather deferred to VoipHeadsetXboxOne (See usage of +snd, -snd, +rcv, -rcv).
                    After rebasing VoipHeadsetXboxOne to MS game chat 2, we found out that the integration
                    with our per-connection muting here was not behaving properly during unmuting: resuming
                    submission of data frames to MS game chat 2 resulted in a ~ 5-sec delay before speech
                    would resume. */
        if ((pConnection->eState == ST_ACTV)  && (iNumSubPackets > 0) 
            #if !defined(DIRTYCODE_XBOXONE) && !defined(DIRTYCODE_GDK)
            && (pConnectionlist->uSendMask & (1 << iConnID))
            #endif
           )
        {
            // see if we need to send any buffered voice data
            _VoipConnectionSendSingle(pConnectionlist, pConnection, iUserIndex, NetTick(), TRUE);

            // increment total number of sub-packets flushed for the connection
            iTotalNumSubPackets += iNumSubPackets;
        }
    }

    // return total number of sub-packets flushed for the connection
    return(iTotalNumSubPackets);
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionRegisterRemoteTalkers

    \Description
        Register/unregister remote users associated with the given connection.

    \Input *pConnectionlist - connectionlist to send to
    \Input iConnID          - connection ident of connection to flush
    \Input bRegister        - if TRUE register talkers, else unregister them

    \Version 01/04/2006 (jbrookes)
*/
/********************************************************************************F*/
void VoipConnectionRegisterRemoteTalkers(VoipConnectionlistT *pConnectionlist, int32_t iConnID, uint32_t bRegister)
{
    VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnID];
    int32_t iUser;

    // register any remote users that are part of the connection
    for (iUser = 0; iUser < pConnection->iMaxRemoteUsersExt; iUser++)
    {
        VoipUserT* pRemoteUser = (VoipUserT *)(&pConnection->RemoteUsers[iUser]);
        // if no user, don't register
        if (VOIP_NullUser(pRemoteUser))
        {
            continue;
        }

        #if DIRTYCODE_LOGGING
        // if the user shouldn't be joining us, complain
        if (pRemoteUser->ePlatform != VOIP_LOCAL_PLATFORM)    // a platform that doesn't match the local platform is joining us
        {
            // is cross play setup properly for the remote user
            if ((pRemoteUser->uFlags & VOIPUSER_FLAG_CROSSPLAY) == FALSE)
            {
                NetPrintf(("voipconnection: [%d] error, platform %d, persona %lld attempted to join, but cross play is not enabled remotely.\n", iConnID, pRemoteUser->ePlatform, pRemoteUser->AccountInfo.iPersonaId));
            }
                
            // is cross play setup properly for the local user
            if (VoipStatus(VoipGetRef(), 'xply', 0, NULL, 0) == FALSE)
            {
                NetPrintf(("voipconnection: [%d] error, platform %d, persona %lld attempted to join, but cross play is not enabled locally.\n", iConnID, pRemoteUser->ePlatform, pRemoteUser->AccountInfo.iPersonaId));
            }
        }
        else if (((pRemoteUser->uFlags & VOIPUSER_FLAG_CROSSPLAY) == TRUE) != (VoipStatus(VoipGetRef(), 'xply', 0, NULL, 0) == TRUE)) // the platform matches, but the cross play settings should match too
        {
            NetPrintf(("voipconnection: [%d] error, local and remote user have mismatched cross play settings, platform %d, persona %lld.\n", iConnID, pRemoteUser->ePlatform, pRemoteUser->AccountInfo.iPersonaId));
        }
        #endif

        // register the user
        pConnectionlist->pRegUserCb((VoipUserT *)&pConnection->RemoteUsers[iUser], iConnID, bRegister, pConnectionlist->pRegUserUserData);

#if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
        // reapply player-to-player xone comm relationships
        pConnectionlist->bApplyRelFromMuting = TRUE;
#endif
    }

    // set mute list to be updated
    pConnectionlist->bUpdateMute = TRUE;
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionSetSendMask

    \Description
        Set connections to send to.

    \Input *pConnectionlist - connectionlist to send to
    \Input uSendMask        - connection send mask

    \Version 03/22/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipConnectionSetSendMask(VoipConnectionlistT *pConnectionlist, uint32_t uSendMask)
{
    if (pConnectionlist->uSendMask != uSendMask)
    {
        VoipCommonSetMask(&pConnectionlist->uSendMask, uSendMask, "sendmask");
    }
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionSetRecvMask

    \Description
        Set connections to receive from.

    \Input *pConnectionlist - connectionlist to send to
    \Input uRecvMask        - connection receive mask

    \Version 03/22/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipConnectionSetRecvMask(VoipConnectionlistT *pConnectionlist, uint32_t uRecvMask)
{
    if (pConnectionlist->uRecvMask != uRecvMask)
    {
        VoipCommonSetMask(&pConnectionlist->uRecvMask, uRecvMask, "recvmask");
    }
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionAddSessionId

    \Description
        Add a session ID the set of higher level sessions sharing the specified VoIP connection.

    \Input *pConnectionlist - connectionlist
    \Input iConnId          - connection ID
    \Input uSessionId       - session ID to be added

    \Output
        int32_t             - 0 for success, negative for failure

    \Notes
        Maintaning a set of session IDs per voip connection is required to support cases
        where two consoles are involved in a P2P game and a pg simultaneously. Both of these
        constructs will have a different session ID over the same shared voip connection.
        Under some specific race conditions affecting the order at which Blaze messages are
        processed by each game client, it is very possible that the pg session id is setup
        first on one side and second on the other side thus leading to VOIP connectivity
        failures if the voip connection construct is not supporting multiple concurrent
        session IDs.

    \Version 08/30/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipConnectionAddSessionId(VoipConnectionlistT *pConnectionlist, int32_t iConnId, uint32_t uSessionId)
{
    int32_t iSessionIndex;
    int32_t iRetCode = -1;
    VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnId];

    NetCritEnter(&pConnectionlist->NetCrit);

    for (iSessionIndex = 0; iSessionIndex < VOIP_MAXSESSIONIDS; iSessionIndex++)
    {
        if (pConnection->uSessionId[iSessionIndex] == 0)
        {
            // free spot, insert session ID here
            pConnection->uSessionId[iSessionIndex] = uSessionId;

            NetPrintf(("voipconnection: [%d] added 0x%08x to set of sessions sharing this voip connection\n", iConnId, uSessionId));

            iRetCode = 0;
            break;
        }
    }

    if (iRetCode != 0)
    {
        NetPrintf(("voipconnection: [%d] warning - 0x%08x could not be added to set of sessions sharing this voip connection because the list is full.\n", iConnId, uSessionId));
    }

    #if DIRTYCODE_LOGGING
    // print set of sessions sharing this connection
    _VoipConnectionPrintSessionIds(pConnectionlist, iConnId);
    #endif

    NetCritLeave(&pConnectionlist->NetCrit);

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionDeleteSessionId

    \Description
        Delete a session ID from the set of sessions sharing this voip connection.

    \Input *pConnectionlist - connectionlist
    \Input iConnId          - connection id
    \Input uSessionId       - session ID to be deleted

    \Output
           int32_t          - 0 for success, negative for failure

    \Version 08/30/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipConnectionDeleteSessionId(VoipConnectionlistT *pConnectionlist, int32_t iConnId, uint32_t uSessionId)
{
    int32_t iSessionIndex;
    int32_t iRetCode = -1;
    VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnId];

    NetCritEnter(&pConnectionlist->NetCrit);

    // now delete this address from the list of fallbacks
    for(iSessionIndex = 0; iSessionIndex < VOIP_MAXSESSIONIDS; iSessionIndex++)
    {
        if (pConnection->uSessionId[iSessionIndex] == uSessionId)
        {
            int32_t iSessionIndex2;

            // move all following session IDs one cell backward in the array
            for(iSessionIndex2 = iSessionIndex; iSessionIndex2 < VOIP_MAXSESSIONIDS; iSessionIndex2++)
            {
                if (iSessionIndex2 == VOIP_MAXSESSIONIDS-1)
                {
                    // last entry, reset to 0
                    pConnection->uSessionId[iSessionIndex2] = 0;
                }
                else
                {
                    pConnection->uSessionId[iSessionIndex2] = pConnection->uSessionId[iSessionIndex2+1];
                }
            }

            NetPrintf(("voipconnection: [%d] removed 0x%08x from set of sessions sharing this voip connection\n", iConnId, uSessionId));

            iRetCode = 0;
            break;
        }
    }

    if (iRetCode != 0)
    {
        NetPrintf(("voipconnection: [%d] warning - 0x%08x not deleted because not found in set of  sessions sharing this voip connection\n", iConnId, uSessionId));
    }

    #if DIRTYCODE_LOGGING
    // print set of sessions sharing this connection
    _VoipConnectionPrintSessionIds(pConnectionlist, iConnId);
    #endif

    NetCritLeave(&pConnectionlist->NetCrit);

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionReliableBroadcastUser

    \Description
        Use this function to broadcast local user join-in-progress/leave-in-progress
        data that needs to be sent reliably on all active connections (or connections in ST_CONN state).

    \Input *pConnectionlist - connectionlist
    \Input uLocalUserIndex  - local user index
    \Input bParticipating   - TRUE if user joined-in-progress, FALSE if user left-in-progress

    \Version 09/18/2014 (mclouatre)
*/
/********************************************************************************F*/
void VoipConnectionReliableBroadcastUser(VoipConnectionlistT *pConnectionlist, uint8_t uLocalUserIndex, uint32_t bParticipating)
{
    ReliableDataT ReliableData;
    uint8_t *pWrite = &ReliableData.aData[0];

    // fill type
    ReliableData.info.uType = (bParticipating ? VOIP_RELIABLE_TYPE_USERADD : VOIP_RELIABLE_TYPE_USERREM);

    // fill payload with user index first
    *pWrite++ = uLocalUserIndex;

    // then append voip user
    ds_memcpy_s(pWrite, sizeof(ReliableData.aData), &pConnectionlist->LocalUsers[uLocalUserIndex], sizeof(pConnectionlist->LocalUsers[uLocalUserIndex]));
    pWrite = pWrite + sizeof(pConnectionlist->LocalUsers[uLocalUserIndex]);

    // fill size
    _VoipEncodeU16(&ReliableData.info.uSize[0], (pWrite - &ReliableData.aData[0]));

    // enqueue for transmission on all connections (will also fill in seq number)
    _VoipConnectionReliableDataEnqueue(pConnectionlist, VOIP_CONNID_ALL, &ReliableData);
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionReliableTranscribedTextMessage

    \Description
        Use this function to reliably broadcast a transcribed text message
        (originated from a local user) on all connections that requested for it.

    \Input *pConnectionlist - connectionlist
    \Input uLocalUserIndex  - local user index
    \Input *pStrUtf8        - pointer to text message to be sent

    \Version 09/18/2014 (mclouatre)
*/
/********************************************************************************F*/
void VoipConnectionReliableTranscribedTextMessage(VoipConnectionlistT *pConnectionlist, uint8_t uLocalUserIndex, const char *pStrUtf8)
{
    int32_t iConnectionIndex;
    ReliableDataT ReliableData;
    uint8_t *pWrite = &ReliableData.aData[0];

    // fill type
    ReliableData.info.uType = VOIP_RELIABLE_TYPE_TEXT;

    // fill payload with user index first
    *pWrite++ = uLocalUserIndex;
    ds_strnzcpy((char *)pWrite, pStrUtf8, VOIP_MAXRELIABLEDATA - 1);
    pWrite += strlen((const char *)pWrite)+1;

    // fill size of payload
    _VoipEncodeU16(&ReliableData.info.uSize[0], (pWrite - &ReliableData.aData[0]));

    // enqueue for transmission only on connections with users that requested for transcribed text
    for (iConnectionIndex = 0; iConnectionIndex < pConnectionlist->iMaxConnections; iConnectionIndex++)
    {
        VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnectionIndex];

        if ((pConnection->eState == ST_ACTV) && pConnection->bTranscribedTextRequested && (pConnectionlist->uSendMask & (1 << iConnectionIndex)))
        {
            _VoipConnectionReliableDataEnqueue(pConnectionlist, iConnectionIndex, &ReliableData);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionReliableSendOpaque

    \Description
        Use this function to send opaque data reliably over specified connection.

    \Input *pConnectionlist - connectionlist
    \Input iConnID          - connection to send the data over
    \Input *pData           - pointer to buffer filled with reliable data to be broadcasted
    \Input uDataSize        - reliable data size (in bytes)

    \Version 09/18/2014 (mclouatre)
*/
/********************************************************************************F*/
void VoipConnectionReliableSendOpaque(VoipConnectionlistT *pConnectionlist, int32_t iConnID, const uint8_t *pData, uint16_t uDataSize)
{
    ReliableDataT ReliableData;

    // fill type
    ReliableData.info.uType = VOIP_RELIABLE_TYPE_OPAQUE;

    // fill size
    _VoipEncodeU16(&ReliableData.info.uSize[0], uDataSize);

    // fill payload
    ds_memcpy_s(&ReliableData.aData[0], sizeof(ReliableData.aData), pData, uDataSize);

    // enqueue for transmission on all connections (will also fill in seq number)
    _VoipConnectionReliableDataEnqueue(pConnectionlist, iConnID, &ReliableData);
}

