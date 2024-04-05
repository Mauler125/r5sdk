/*H*************************************************************************************************/
/*!

    \File    netgamelink.c

    \Description
        This module provides a packet layer peer-peer interface which utilizes
        a lower layer "comm" module. This module is used either by netgamedist
        (if it is being used) or can be accessed directly if custom online game
        logic is being used. This module also calculates latency (ping) and
        maintains statistics about the connection (number of bytes send/received).
        The direct application interface is currently weak but will be improved.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002.  ALL RIGHTS RESERVED.

    \Version    1.0        12/19/00 (GWS) First Version

*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <string.h>             /* memset/ds_memcpy */

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/game/netgamepkt.h"
#include "DirtySDK/game/netgamelink.h"
#include "DirtySDK/comm/commall.h"

/*** Defines ***************************************************************************/

#define NETGAMELINK_KEEPALIVE_TIME      (500)

//! min/max values for NetGameLink QoS settings
#define NETGAMELINK_QOS_DURATION_MIN    (0)
#define NETGAMELINK_QOS_DURATION_MAX    (10000)
#define NETGAMELINK_QOS_INTERVAL_MIN    (10)
#define NETGAMELINK_QOS_PACKETSIZE_MIN  (50)

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! netgamelink internal state
struct NetGameLinkRefT
{
    //! port used to communicate with server
    CommRef *pPort;
    //! flag to indicate if we own port (if we need to destroy when done)
    int32_t iPortOwner;

    //! module memory group
    int32_t iMemGroup;
    void *pMemGroupUserData;

    //! tick at which last packet was sent to server (in ms)
    uint32_t uLastSend;
    //! tick at which last sync packet was received from server (in ms)
    uint32_t uLastRecv;
    //! tick at which last sync packet contributed to latency average calculation
    uint32_t uLastAvg;
    //! last echo requested by server (used for rtt calc)
    uint32_t uLastEcho;
    //! last time a sync packet was sent
    uint32_t uLastSync;
    //! last time the history slot changed
    uint32_t uLastHist;

    //! the time weighted rtt average
    int32_t iRttAvg;
    //! the time weighted rtt deviation
    int32_t iRttDev;

    //! external status monitoring
    NetGameLinkStatT NetGameLinkStats;

    //! stats over a one second period
    int32_t iInpBytes;      //!< input bytes
    int32_t iInpPackets;    //!< input packets
    int32_t iInpRawBytes;   //!< input raw bytes
    int32_t iInpRawPackets; //!< input raw packets
    int32_t iOverhead;      //!< send overhead
    int32_t iRcvOverhead;   //!< recv overhead
    int32_t iRcvRawBytes;   //!< recv raw bytes
    int32_t iRcvRawPackets; //!< recv raw packet (commudp recvs)
    int32_t iRcvPackets;    //!< recv packets
    int32_t iRcvBytes;      //!< recv bytes

    // track packets sent/received for sending to peer in sync packets

    //! stats - raw packets sent
    int32_t iPackSent;
    //! stats - raw packets received
    int32_t iPackRcvd;
    //! stats - remote->local packets lost
    int32_t iR2LPackLost;
    //! stats - naks sent
    int32_t iNakSent;

    //! point to receive buffer
    char *pBuffer;
    //! length of data in receive buffer
    int32_t iBufLen;
    //! size of receive buffer
    int32_t iBufMax;

    //! protect resources
    NetCritT Crit;
    //! count of missed accessed
    int32_t iProcess;

    //! data for m_callproc
    void *pCallData;
    //! callback for incoming packet notification
    void (*pCallProc)(void *pCalldata, int32_t iKind);
    //! callback pending
    int32_t iCallPending;

    //! count calls to NetGameLinkRecv() to limit callback access
    int32_t iRecvProcCnt;

    //! send enable/disable
    int32_t bSendEnabled;

    //! sync enable/disable
    int32_t bSyncEnabled;

    //! list of active streams
    NetGameLinkStreamT *m_stream;

    int32_t iStrmMaxPktSize;

    //! used for tracking QoS packets
    uint32_t uQosSendInterval;
    uint32_t uQosLastSendTime;
    uint32_t uQosStartTime;
    uint16_t uQosPacketsToSend;
    uint16_t uQosPacketsToRecv;
    uint16_t uQosPacketSize;
    int32_t iQosCumulLate;
    int32_t iQosLateCount;
    int32_t iQosAvgLate;

    //! verbosity
    int32_t iVerbosity;
};

typedef struct GameStreamT
{
    int32_t iKind;          //!< block type
    uint32_t uSync;         //!< sync sequence number
    int32_t iSent;          //!< amount sent so far
    int32_t iSize;          //!< total block size
    int32_t iSubchan;       //!< subchannel index/
} GameStreamT;


/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

// Public variables


/*** Private Functions *****************************************************************/



/*F*************************************************************************************/
/*!
    \Function     _NetGameLinkUpdateLocalPacketLoss

    \Description
        Calculate local packet loss based on a difference of remote packets sent and
        local packets received.

    \Input uLPackRcvd       - count of packets received locally
    \Input uRPackSent       - count of packets sent by remote peer (cumulative psnt
                              values of all received sync packets)
    \Input uOldLPackLost    - old count of packets lost

    \Output
        uint32_t            - updated count of packets lost

    \Notes
        \verbatim
        This function returns the number of packets lost at this end of the connection
        since the beginning. It is obtained by subtracting uRPackSent (the number of
        packets sent by the remote end - cumulative psnt values of all received sync
        packets) from uLPackRcvd (the number of packets received locally).

        The following two scenarios indicate that a sync packet itself suffered packet
        loss, and it relied on the packet resending mechanisms of commudp to finally
        make it to us:
            1 - uLPackRcvd is larger than uRPackSent (subtracting them would return a
                negative value)
            2 - uLPackRcvd is smaller than uRPackSent, but the last saved count of
                packets lost is larger than the new one.

        For both scenarios, we skip updating the packets lost count to avoid a
        possibly negative or under-valued packet loss calculation, and we deal with
        that iteration as if no packet loss was detected.

        The assumption here is that the next sync packet that makes it to us without
        being "resent" will end up updating the packet lost counters with a "coherent"
        value as its "psnt" field will include all packets that were used for packet
        retransmission.  At the end, this is guaranteeing the coherency of the packets
        lost count (non negative value and only increasing over time) by delaying its
        update.
        \endverbatim

    \Version 09/02/14 (mclouatre)
*/
/*************************************************************************************F*/
static uint32_t _NetGameLinkUpdateLocalPacketLoss(uint32_t uLPackRcvd, uint32_t uRPackSent, uint32_t uOldLPackLost)
{
    uint32_t uUpdatedLPackLost = uOldLPackLost;

    // only proceed if packet loss is not negative
    if (uRPackSent > uLPackRcvd)
    {
        // only proceed if packet loss is larger than last calculated value
        if ((uRPackSent - uLPackRcvd) > uOldLPackLost)
        {
            uUpdatedLPackLost = uRPackSent - uLPackRcvd;
        }
     }

    return(uUpdatedLPackLost);
}

/*F*************************************************************************************/
/*!
    \Function     _NetGameLinkGetSync

    \Description
        Extract sync packet from buffer, and return a pointer to the sync packet location in the buffer

    \Input *pPacket     - pointer to packet structure
    \Input *pPacketData - pointer to packet data (used instead of packet buffer ref to allow split use)
    \Input *pSync       - [out] output buffer for extracted sync packet

    \Output
        NetGamePacketSyncT * - pointer to start of sync packet in buffer, or NULL if invalid

    \Version 09/09/11 (jbrookes)
*/
/*************************************************************************************F*/
static NetGamePacketSyncT *_NetGameLinkGetSync(NetGamePacketT *pPacket, uint8_t *pPacketData, NetGamePacketSyncT *pSync)
{
    uint32_t uSyncSize=0;
    // validate sync length byte is available
    if (pPacket->head.len < 1)
    {
        NetPrintf(("netgamelink: received a sync packet with no data\n"));
        return(NULL);
    }
    // validate sync size
    if ((uSyncSize = (uint32_t)pPacketData[pPacket->head.len-1]) != sizeof(*pSync))
    {
        NetPrintf(("netgamelink: received a sync with an invalid size (got=%d, expected=%d)\n", uSyncSize, sizeof(*pSync)));
        return(NULL);
    }
    // validate packet is large enough to hold sync
    if (pPacket->head.len < uSyncSize)
    {
        NetPrintf(("netgamelink: received a sync too large for packet (len=%d)\n", pPacket->head.len));
        return(NULL);
    }
    // locate sync at end of packet & subtract from packet length, copy to output
    pPacket->head.len -= uSyncSize;
    ds_memcpy(pSync, pPacketData+pPacket->head.len, uSyncSize);
    // return sync packet pointer to caller
    return((NetGamePacketSyncT *)(pPacketData+pPacket->head.len));
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkSendPacket

    \Description
        Send a packet to the server

    \Input *pRef    - reference pointer
    \Input *pPacket - pointer to completed packet
    \Input uCurrTick- current tick

    \Output
        int32_t     - bad error, zero=unable to send now, positive=sent

    \Notes
        Adds timestamp, rtt and duplex information to packet

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
static int32_t _NetGameLinkSendPacket(NetGameLinkRefT *pRef, NetGamePacketT *pPacket, uint32_t uCurrTick)
{
    int32_t iResult;
    int32_t iSynced = 0;
    NetGamePacketSyncT sync;
    uint32_t uPacketFlags, uPacketType;
    uint32_t uPackSent=0, uPackRcvd=0, uLPktLost=0, uNakSent=0;

    // sending enabled?
    if (pRef->bSendEnabled == FALSE)
    {
        NetPrintf(("netgamelink: warning -- trying to send packet over a sending-disabled link (%d bytes)\n", pPacket->head.len));
        return(1);
    }

    // return error for oversized packets
    if ((pPacket->head.len + NETGAME_DATAPKT_MAXTAIL) > pRef->pPort->maxwid)
    {
        NetPrintf(("netgamelink: oversized packet send (%d bytes)\n", pPacket->head.len));
        return(-1);
    }

    /* see if we should add sync info:
       if _NetGameLinkSendPacket was called explicitly to send a sync packet there is no need to make any further checks
       otherwise if a reliable packet game was being sent and it is time to send a sync packet */
    if (((pPacket->head.kind & GAME_PACKET_SYNC) || ((pPacket->head.kind != GAME_PACKET_USER_UNRELIABLE) && (uCurrTick-pRef->uLastSync > NETGAMELINK_KEEPALIVE_TIME/2))) && (pRef->bSyncEnabled == TRUE))
    {
        // build the sync packet
        ds_memclr(&sync, sizeof(sync));

        sync.size = sizeof(sync);
        sync.echo = SocketHtonl(uCurrTick);
        sync.repl = SocketHtonl(pRef->uLastEcho+(uCurrTick-pRef->uLastRecv));
        sync.late = SocketHtons((int16_t)((pRef->iRttAvg+pRef->iRttDev+1)/2));

        uLPktLost = _NetGameLinkUpdateLocalPacketLoss(pRef->NetGameLinkStats.lpackrcvd, pRef->NetGameLinkStats.rpacksent, (unsigned)pRef->iR2LPackLost);
        uPackSent = pRef->pPort->packsent;
        uNakSent = pRef->pPort->naksent;
        uPackRcvd = pRef->pPort->packrcvd;

        sync.plost = (uint8_t)(uLPktLost - pRef->iR2LPackLost);
        sync.psnt = (uint8_t)(uPackSent - pRef->iPackSent);
        sync.nsnt = (uint8_t)(uNakSent - pRef->iNakSent);
        sync.prcvd = (uint8_t)(uPackRcvd - pRef->iPackRcvd);

        // if we have not received a sync packet from the remote peer, our repl field is not valid, so we tell the remote peer to ignore it
        if (pRef->NetGameLinkStats.rpacksent == 0)
        {
            sync.flags |= NETGAME_SYNCFLAG_REPLINVALID;
        }

        // piggyback on existing packet
        ds_memcpy(pPacket->body.data+pPacket->head.len, &sync, sizeof(sync));
        pPacket->head.len += sizeof(sync);
        pPacket->head.kind |= GAME_PACKET_SYNC;
        iSynced = 1;
    }

    // put type as last byte
    pPacket->body.data[pPacket->head.len] = pPacket->head.kind;
    // determine packet flags
    uPacketType = pPacket->head.kind & ~GAME_PACKET_SYNC;
    if (((uPacketType <= GAME_PACKET_ONE_BEFORE_FIRST) || (uPacketType >= GAME_PACKET_ONE_PAST_LAST)) && (pPacket->head.kind != GAME_PACKET_SYNC))
    {
        NetPrintf(("netgamelink: warning -- send unrecognized packet kind %d\n", pPacket->head.kind));
    }
    if (uPacketType == GAME_PACKET_USER_UNRELIABLE)
    {
        uPacketFlags = COMM_FLAGS_UNRELIABLE;
    }
    else if (uPacketType == GAME_PACKET_USER_BROADCAST)
    {
        uPacketFlags = COMM_FLAGS_UNRELIABLE|COMM_FLAGS_BROADCAST;
    }
    else
    {
        uPacketFlags = COMM_FLAGS_RELIABLE;
    }

    // send the packet
    iResult = pRef->pPort->Send(pRef->pPort, pPacket->body.data, pPacket->head.len + 1, uPacketFlags);

    // if we added sync info, remove it now
    if (iSynced)
    {
        pPacket->head.len -= sizeof(sync);
        pPacket->head.kind ^= GAME_PACKET_SYNC;
    }

    // make sure send succeeded
    if (iResult > 0)
    {
        // record the send time
        pRef->uLastSend = uCurrTick;
        // if it was a sync packet, update sync info
        if (iSynced)
        {
            pRef->uLastSync = uCurrTick;
            pRef->iPackSent = uPackSent;
            pRef->iNakSent = uNakSent;
            pRef->iPackRcvd = uPackRcvd;
            pRef->iR2LPackLost = uLPktLost;
        }

        // update the stats
        pRef->iInpBytes += pPacket->head.len;
        pRef->iInpPackets += 1;
        pRef->NetGameLinkStats.sent += pPacket->head.len;
        pRef->NetGameLinkStats.sentlast = pRef->uLastSend;

        // see if we should turn off send light
        if ((pRef->NetGameLinkStats.sentshow != 0) && (pRef->uLastSend -pRef->NetGameLinkStats.sentshow > 100))
        {
            pRef->NetGameLinkStats.sentshow = 0;
            pRef->NetGameLinkStats.senthide = pRef->uLastSend;
        }

        // see if we should turn on send light
        if ((pRef->NetGameLinkStats.senthide != 0) && (pRef->uLastSend -pRef->NetGameLinkStats.senthide > 100))
        {
            pRef->NetGameLinkStats.senthide = 0;
            pRef->NetGameLinkStats.sentshow = pRef->uLastSend;
        }
    }
    else
    {
//        NetPrintf(("GmLink: send failed! (kind=%d, len=%d, synced=%d)\n",
//            packet->head.kind, packet->head.len, iSynced));
    }

    // return the result code
    return(iResult);
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkRecvPacket0

    \Description
        Process incoming data packet

    \Input *pRef    - reference pointer
    \Input uCurrTick- current tick

    \Output
        int32_t     - zero=no packet processed, positive=packet processed

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
static int32_t _NetGameLinkRecvPacket0(NetGameLinkRefT *pRef, uint32_t uCurrTick)
{
    int32_t iSize;
    int16_t iDelta;
    uint32_t uWhen, uKind;
    NetGamePacketT *pPacket;
    NetGameLinkHistT *pHistory;

    // calculate buffer space free, making sure to include packet header overhead (not sent, but queued)
    if ((iSize = pRef->iBufMax -pRef->iBufLen -sizeof(pPacket->head)) <= 0)
    {
        return(0);
    }
    else if (iSize > pRef->pPort->maxwid)
    {
        iSize = pRef->pPort->maxwid;
    }

    // setup packet buffer
    pPacket = (NetGamePacketT *)(pRef->pBuffer +pRef->iBufLen);

    // see if packet is available
    iSize = pRef->pPort->Recv(pRef->pPort, &pPacket->body, iSize, &uWhen);
    if ((iSize <= 0) || (iSize > pRef->pPort->maxwid))
    {
        #if DIRTYCODE_LOGGING
        if (iSize != COMM_NODATA)
        {
            NetPrintf(("netgamelink: Recv() returned %d\n", iSize));
        }
        #endif
        return(0);
    }
    pPacket->head.len = iSize;
    pPacket->head.size = 0;
    pPacket->head.when = uWhen;

    pRef->NetGameLinkStats.tick = uCurrTick;
    pRef->NetGameLinkStats.tickseqn += 1;

    // update the stats
    pRef->NetGameLinkStats.rcvdlast = uCurrTick;
    pRef->NetGameLinkStats.rcvd += pPacket->head.len;
    pRef->iRcvBytes += pPacket->head.len;
    pRef->iRcvPackets += 1;

    // see if we should turn off receive light
    if ((pRef->NetGameLinkStats.rcvdshow != 0) && (uCurrTick-pRef->NetGameLinkStats.rcvdshow > 100))
    {
        pRef->NetGameLinkStats.rcvdshow = 0;
        pRef->NetGameLinkStats.rcvdhide = uCurrTick;
    }

    // see if we should turn on receive light
    if ((pRef->NetGameLinkStats.rcvdhide != 0) && (uCurrTick-pRef->NetGameLinkStats.rcvdhide > 100))
    {
        pRef->NetGameLinkStats.rcvdhide = 0;
        pRef->NetGameLinkStats.rcvdshow = uCurrTick;
    }

    // extract the kind field and fix the length
    pPacket->head.len -= 1;
    pPacket->head.kind = pPacket->body.data[pPacket->head.len];

    // get packet kind
    uKind = pPacket->head.kind & ~GAME_PACKET_SYNC;

    // warn if kind is invalid
    if (((uKind <= GAME_PACKET_ONE_BEFORE_FIRST) || (uKind >= GAME_PACKET_ONE_PAST_LAST)) && (pPacket->head.kind != GAME_PACKET_SYNC))
    {
        NetPrintf(("netgamelink: warning -- recv unrecognized packet kind %d\n", pPacket->head.kind));
        return(1);
    }

    // see if this packet contains timing info
    if (pPacket->head.kind & GAME_PACKET_SYNC)
    {
        // get sync packet info
        NetGamePacketSyncT Sync;
        if (_NetGameLinkGetSync(pPacket, pPacket->body.data, &Sync) == NULL)
        {
            return(1);
        }

        // remove sync bit
        pPacket->head.kind ^= GAME_PACKET_SYNC;

        // update the latency stat
        pRef->NetGameLinkStats.late = SocketNtohs(Sync.late);

        // calculate instantaneous rtt
        if ((Sync.flags & NETGAME_SYNCFLAG_REPLINVALID) == 0)
        {
            /* delta = recvtime - sendtime + 1
               The +1 is needed because the peer may think the packet stayed in
               its queue for 1ms while we may think it was returned within the
               same millisecond. This happens because the clocks are not precisely
               synchronized. Adding 1 avoids the issue without compromising precision. */
            iDelta = (int16_t)(uWhen - SocketNtohl(Sync.repl) + 1);
        }
        else
        {
            // remote peer has indicated their repl field is invalid, so we ignore sync latency info
            iDelta = -1;
        }
        if ((iDelta >= 0) && (iDelta <= 2500))
        {
            // figure out elapsed time since last packet
            int32_t iElapsed = uWhen-pRef->uLastAvg;
            if (iElapsed < 10)
            {
                iElapsed = 10;
            }
            pRef->uLastAvg = uWhen;

            // perform rtt calc using weighted time average
            if (iElapsed < RTT_SAMPLE_PERIOD)
            {
                // figure out weight of existing data
                int32_t iWeight = RTT_SAMPLE_PERIOD-iElapsed;
                // figure deviation first since it uses average
                int32_t iDeviate = iDelta - pRef->iRttAvg;
                if (iDeviate < 0)
                    iDeviate = -iDeviate;
                // calc weighted deviation
                pRef->iRttDev = (iWeight*pRef->iRttDev+iElapsed*iDeviate)/RTT_SAMPLE_PERIOD;
                // calc weighted average
                pRef->iRttAvg = (iWeight*pRef->iRttAvg+iElapsed*iDelta)/RTT_SAMPLE_PERIOD;
            }
            else
            {
                // if more than our scale has elapsed, use this data
                pRef->iRttAvg = iDelta;
                pRef->iRttDev = 0;
            }

            // save copy of ping in stats table
            pRef->NetGameLinkStats.ping = pRef->iRttAvg+pRef->iRttDev;
            pRef->NetGameLinkStats.pingavg = pRef->iRttAvg;
            pRef->NetGameLinkStats.pingdev = pRef->iRttDev;

            // see if this is a new recod
            if (uWhen-pRef->uLastHist >= PING_LENGTH)
            {
                // remember the update time
                pRef->uLastHist = uWhen;
                // advance to next ping slot
                pRef->NetGameLinkStats.pingslot = (pRef->NetGameLinkStats.pingslot + 1) % PING_HISTORY;
                pHistory = pRef->NetGameLinkStats.pinghist + pRef->NetGameLinkStats.pingslot;

                // save the information
                pHistory->min = iDelta;
                pHistory->max = iDelta;
                pHistory->avg = iDelta;
                pHistory->cnt = 1;
            }
            else
            {
                // update the information
                pHistory = pRef->NetGameLinkStats.pinghist + pRef->NetGameLinkStats.pingslot;
                if (iDelta < pHistory->min)
                    pHistory->min = iDelta;
                if (iDelta > pHistory->max)
                    pHistory->max = iDelta;
                pHistory->avg = ((pHistory->avg * pHistory->cnt) + iDelta) / (pHistory->cnt+1);
                pHistory->cnt += 1;
            }

            // save this update time
            pRef->NetGameLinkStats.pingtick = uWhen;
        }
        else if ((Sync.flags & NETGAME_SYNCFLAG_REPLINVALID) == 0)
        {
            NetPrintf(("netgamelink: sync delta %d is out of range (when=0x%08x,sync.repl=0x%08x)\n", iDelta, uWhen, SocketNtohl(Sync.repl) ));
        }

        // extract timing information
        pRef->uLastRecv = uWhen;
        pRef->uLastEcho = SocketNtohl(Sync.echo);

        // update remote peer's packets sent/recv stat tracker
        if (pRef->NetGameLinkStats.rpacksent == 0)
        {
            /*
               bias initial update by one to account for the commdup packet conveying this NetGameLinkSyncT packet not being in the count (but it should really be)
               no need to do so afterwards because the count will include the previous packet for which the count is already biased
            */
            pRef->NetGameLinkStats.rpacksent += 1;
        }
        pRef->NetGameLinkStats.rpacksent += Sync.psnt;
        pRef->NetGameLinkStats.rnaksent += Sync.nsnt;
        pRef->NetGameLinkStats.rpackrcvd += Sync.prcvd;
        pRef->NetGameLinkStats.rpacklost += Sync.plost;

        // update packets received, packets saved, packets sent and packets lost; this is done here to keep in sync with rpacksent and rpackrcvd
        pRef->NetGameLinkStats.lnaksent = pRef->pPort->naksent;
        pRef->NetGameLinkStats.lpackrcvd = pRef->pPort->packrcvd;
        pRef->NetGameLinkStats.lpacksaved = pRef->pPort->packsaved;
        pRef->NetGameLinkStats.lpacksent = pRef->pPort->packsent;
        pRef->NetGameLinkStats.lpacklost = _NetGameLinkUpdateLocalPacketLoss(pRef->NetGameLinkStats.lpackrcvd, pRef->NetGameLinkStats.rpacksent, pRef->NetGameLinkStats.lpacklost);
    }

    // save packet if something remains (was not just sync)
    if (pPacket->head.kind != 0)
    {
        pPacket->head.size = (sizeof(pPacket->head)+pPacket->head.len+3) & 0x7ffc;
        pRef->iBufLen += pPacket->head.size;
    }
    return(1);
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkRecvPacket

    \Description
        Call _NetGameLinkRecvPacket0 if we haven't already

    \Input *pRef    - reference pointer
    \Input uCurrTick- current tick

    \Output
        int32_t     - zero=no packet processed, positive=packet processed

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
static int32_t _NetGameLinkRecvPacket(NetGameLinkRefT *pRef, uint32_t uCurrTick)
{
    int32_t iRetVal;

    // limit call depth to prevent a callback from calling us more than once
    if (pRef->iRecvProcCnt > 0)
    {
        #if DIRTYCODE_LOGGING && 0
        NetPrintf(("netgamelink: m_recvprocct=%d, unable to call _NetGameLinkRecvPacket0\n", pRef->m_recvprocct));
        #endif
        return(0);
    }

    pRef->iRecvProcCnt = 1;
    iRetVal = _NetGameLinkRecvPacket0(pRef, uCurrTick);
    pRef->iRecvProcCnt = 0;

    return(iRetVal);
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkProcess

    \Description
        Main send/receive processing loop

    \Input *pRef    - reference pointer
    \Input uCurrTick- current tick

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
static void _NetGameLinkProcess(NetGameLinkRefT *pRef, uint32_t uCurrTick)
{
    uint32_t uRange;

    // grab any pending packets
    while (_NetGameLinkRecvPacket(pRef, uCurrTick) > 0)
    {
        pRef->iCallPending += 1;
    }

    // mark as processing complete
    pRef->iProcess = 0;

    // handle stats update (once per second)
    pRef->NetGameLinkStats.tick = uCurrTick;
    pRef->NetGameLinkStats.tickseqn += 1;
    uRange = uCurrTick - pRef->NetGameLinkStats.stattick;
    if (uRange >= 1000)
    {
        // calc bytes per second, raw bytes per second, and network bytes per second
        pRef->NetGameLinkStats.outbps = (pRef->iInpBytes *1000)/uRange;
        pRef->NetGameLinkStats.outrps = (pRef->pPort->datasent-pRef->iInpRawBytes)*1000/uRange;
        pRef->NetGameLinkStats.outnps = ((pRef->pPort->datasent-pRef->iInpRawBytes)+(pRef->pPort->overhead-pRef->iOverhead))*1000/uRange;
        pRef->NetGameLinkStats.inrps = ((pRef->pPort->datarcvd-pRef->iRcvRawBytes)*1000)/uRange;
        pRef->NetGameLinkStats.inbps = (pRef->iRcvBytes * 1000)/uRange;
        pRef->NetGameLinkStats.innps = ((pRef->pPort->datarcvd-pRef->iRcvRawBytes)+(pRef->pPort->rcvoverhead-pRef->iRcvOverhead))*1000/uRange;
        // calculate packets per second and raw packets per second
        pRef->NetGameLinkStats.outpps = ((pRef->iInpPackets *1000)+500)/uRange;
        pRef->NetGameLinkStats.outrpps = ((pRef->pPort->packsent-pRef->iInpRawPackets)*1000+500)/uRange;
        pRef->NetGameLinkStats.inpps = ((pRef->iRcvPackets *1000)+500)/uRange;
        pRef->NetGameLinkStats.inrpps = ((pRef->pPort->packrcvd - pRef->iRcvRawPackets)*1000+500)/uRange;
        // reset tracking variables
        pRef->iInpBytes = pRef->iInpPackets = 0;
        pRef->iRcvBytes = pRef->iRcvPackets = 0;
        pRef->iInpRawBytes = pRef->pPort->datasent;
        pRef->iInpRawPackets = pRef->pPort->packsent;
        pRef->iRcvRawPackets = pRef->pPort->packrcvd;
        pRef->iRcvRawBytes = pRef->pPort->datarcvd;
        pRef->iOverhead = pRef->pPort->overhead;
        pRef->iRcvOverhead = pRef->pPort->rcvoverhead;

        // remember stat update time
        pRef->NetGameLinkStats.stattick = uCurrTick;
    }

    // see if we should turn off send light
    if ((pRef->NetGameLinkStats.sentshow != 0) && (uCurrTick-pRef->NetGameLinkStats.sentshow > 100))
    {
        pRef->NetGameLinkStats.sentshow = 0;
        pRef->NetGameLinkStats.senthide = uCurrTick;
    }

    // see if we should turn on send light
    if ((pRef->NetGameLinkStats.senthide != 0) && (uCurrTick-pRef->NetGameLinkStats.senthide > 100) && (uCurrTick-pRef->NetGameLinkStats.sentlast < 50))
    {
        pRef->NetGameLinkStats.senthide = 0;
        pRef->NetGameLinkStats.sentshow = uCurrTick;
    }

    // see if we should turn off receive light
    if ((pRef->NetGameLinkStats.rcvdshow != 0) && (uCurrTick-pRef->NetGameLinkStats.rcvdshow > 100))
    {
        pRef->NetGameLinkStats.rcvdshow = 0;
        pRef->NetGameLinkStats.rcvdhide = uCurrTick;
    }

    // see if we should turn on receive light
    if ((pRef->NetGameLinkStats.rcvdhide != 0) && (uCurrTick-pRef->NetGameLinkStats.rcvdhide > 100) && (uCurrTick-pRef->NetGameLinkStats.rcvdlast < 50))
    {
        pRef->NetGameLinkStats.rcvdhide = 0;
        pRef->NetGameLinkStats.rcvdshow = uCurrTick;
    }

    // send keepalive/sync if needed
    if ((uCurrTick-pRef->uLastSend > NETGAMELINK_KEEPALIVE_TIME) && (pRef->bSyncEnabled == TRUE))
    {
        // make sure we do not overflow output queue
        int32_t iQueue = pRef->pPort->Send(pRef->pPort, NULL, 0, COMM_FLAGS_RELIABLE);
        // the exact buffer limit is unimportant, but it needs something
        // to avoid overrun during a semi-persistant failure
        if ((iQueue > 0) && (iQueue < (pRef->pPort->maxout/4)))
        {
            // send sync packet
            NetGamePacketT spacket;
            spacket.head.kind = GAME_PACKET_SYNC;
            spacket.head.len = 0;
            _NetGameLinkSendPacket(pRef, &spacket, uCurrTick);
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkPrintStats

    \Description
        Prints the current NetGameLink stats.

    \Input *pRef     - reference pointer

    \Version 09/03/14 (mcorcoran)
*/
/*************************************************************************************************F*/
static void _NetGameLinkPrintStats(NetGameLinkRefT *pRef)
{
    NetGameLinkStatT NetGameLinkStats;
    NetGameLinkStatus(pRef, 'stat', 0, &NetGameLinkStats, sizeof(NetGameLinkStats));

    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: QoS Results ------------------------------------------------------------------------------\n"));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: latency over QoS period                                                          %d\n", pRef->iQosAvgLate));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: latency over last sampling period                                                %d (sampl. prd = %d ms)\n", NetGameLinkStats.late, RTT_SAMPLE_PERIOD));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: when connection established                                                      %d\n", NetGameLinkStats.conn));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: when data most recently sent                                                     %d\n", NetGameLinkStats.sentlast));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: when data most recently received                                                 %d\n", NetGameLinkStats.rcvdlast));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: ping deviation                                                                   %d\n", NetGameLinkStats.pingdev));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: ping average                                                                     %d\n", NetGameLinkStats.pingavg));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: number of bytes sent                                                             %d\n", NetGameLinkStats.sent));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: number of bytes received                                                         %d\n", NetGameLinkStats.rcvd));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: number of packets sent to peer since start (at time = last inbound sync pkt)     %d\n", NetGameLinkStats.lpacksent));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: number of packets sent to peer since start (at time = now)                       %d\n", NetGameLinkStats.lpacksent_now));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: number of packets received from peer since start                                 %d\n", NetGameLinkStats.lpackrcvd));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: number of packets sent by peer (to us) since start                               %d\n", NetGameLinkStats.rpacksent));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: number of packets received by peer (from us) since start                         %d\n", NetGameLinkStats.rpackrcvd));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: local->remote packets lost: number of packets (from us) lost by peer since start %d\n", NetGameLinkStats.rpacklost));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: packets saved: number of packets recovered by via redundancy mechanisms          %d\n", NetGameLinkStats.lpacksaved));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: number of NAKs sent by peer (to us) since start                                  %d\n", NetGameLinkStats.rnaksent));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: packets per sec sent (user packets)                                              %d\n", NetGameLinkStats.outpps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: raw packets per sec sent (packets sent to network)                               %d\n", NetGameLinkStats.outrpps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: bytes per sec sent (user data)                                                   %d\n", NetGameLinkStats.outbps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: raw bytes per sec sent (data sent to network)                                    %d\n", NetGameLinkStats.outrps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: network bytes per sec sent (inrps + estimated UDP/Eth frame overhead)            %d\n", NetGameLinkStats.outnps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: packets per sec received (user packets)                                          %d\n", NetGameLinkStats.inpps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: raw packets per sec received (packets sent to network)                           %d\n", NetGameLinkStats.inrpps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: bytes per sec received (user data)                                               %d\n", NetGameLinkStats.inbps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: raw bytes per sec received (data sent to network)                                %d\n", NetGameLinkStats.inrps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: network bytes per sec received (inrps + estimated UDP/Eth frame overhead)        %d\n", NetGameLinkStats.innps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: ------------------------------------------------------------------------------------------\n"));
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkNotify

    \Description
        Main notification from lower layer

    \Input *pCommRef- generic comm pointer
    \Input iEvent   - event type

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
static void _NetGameLinkNotify(CommRef *pCommRef, int32_t iEvent)
{
    NetGameLinkRefT *pRef = (NetGameLinkRefT *)pCommRef->refptr;

    // make sure we are exclusive
    if (NetCritTry(&pRef->Crit))
    {
        // do the processing
        _NetGameLinkProcess(pRef, NetTick());

        // do callback if needed
        if ((pRef->iBufLen > 0) && (pRef->pCallProc != NULL) && (pRef->iCallPending > 0))
        {
            pRef->iCallPending = 0;
            (pRef->pCallProc)(pRef->pCallData, 1);
        }

        // free access
        NetCritLeave(&pRef->Crit);
    }
    else
    {
        // count the miss
        pRef->iProcess += 1;
        if (pRef->iProcess > 0)
        {
//            NetPrintf(("netgamelink: missed %d events\n", pRef->iProcess));
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function _NetGameLinkSendCallback

    \Description
        Handle notification of send (or re-send) of data from comm layer.

    \Input *pComm       - pointer to comm ref
    \Input *pPacket     - packet data that is being sent
    \Input iPacketLen   - packet length
    \Input uCurrTick    - current timestamp

    \Version 09/09/11 (jbrookes)
*/
/*************************************************************************************************F*/
static void _NetGameLinkSendCallback(CommRef *pComm, void *pPacket, int32_t iPacketLen, uint32_t uCurrTick)
{
    uint8_t *pPacketData = (uint8_t *)pPacket;
    NetGamePacketSyncT Sync, *pSync;
    NetGamePacketHeadT Head;
    uint32_t uTickDiff;

    // does this packet include a sync packet?
    if ((pPacketData[iPacketLen-1] & GAME_PACKET_SYNC) == 0)
    {
        return;
    }

    // extract sync packet
    Head.kind = pPacketData[iPacketLen-1];
    Head.len = iPacketLen-1;
    if ((pSync = _NetGameLinkGetSync((NetGamePacketT *)&Head, pPacketData, &Sync)) != NULL)
    {
        // compare currtick with current tick
        uTickDiff = NetTickDiff(uCurrTick, SocketNtohl(Sync.echo));

        // refresh echo and repl and write back sync packet
        Sync.echo = SocketHtonl(uCurrTick);
        Sync.repl = SocketHtonl((SocketNtohl(Sync.repl) + uTickDiff));
        ds_memcpy(pSync, &Sync, sizeof(Sync));

        /* Note:
           Unlike echo and repl, other sync packet fields (psnt, plost, nsnt, prcvd,...) are intentionally not
           refreshed here because they convey update of counts between two sync packets (they are delta values).
           Any update in the values of these counters not included in this sync packet (regardless of this
           packet being resent or not) will be safely covered by the next sync packet. */
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkPollStream

    \Description
        Poll to see if we should send stream data

    \Input *pRef    - reference pointer

    \Output
        int32_t     - send count

    \Version 01/11/10 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _NetGameLinkPollStream(NetGameLinkRefT *pRef)
{
    int32_t iCount = 0, iSize, iResult;
    NetGameMaxPacketT Packet;
    GameStreamT Block;
    NetGameLinkStreamT *pStream;

    // see if any stream has pending data
    for (pStream = pRef->m_stream; pStream != NULL; pStream = pStream->pNext)
    {
        // see if anything to send
        while (pStream->iOutProg < pStream->iOutSize)
        {
            int32_t iLimit;

            // get data block
            ds_memcpy(&Block, pStream->pOutData+pStream->iOutProg, sizeof(Block));
            // setup data packet
            Packet.head.kind = GAME_PACKET_LINK_STRM;
            // set up packet 'size' field, which is size | subchannel
            iSize = (Block.iSize & ~0xff000000) | ((Block.iSubchan & 0xff) << 24);
            // store stream header fields in network order
            Packet.body.strm.ident = SocketHtonl(pStream->iIdent);
            Packet.body.strm.kind = SocketHtonl(Block.iKind);
            Packet.body.strm.size = SocketHtonl(iSize);
            iSize = Block.iSize -Block.iSent;

            // don't overflow our buffer
            iLimit = (signed)sizeof(Packet.body.strm.data);
            if (iSize > iLimit)
            {
                iSize = iLimit;
            }

            // don't overflow the underlying layer max packet size
            iLimit = pRef->iStrmMaxPktSize;
            if (iSize > iLimit)
            {
                iSize = iLimit;
            }

            ds_memcpy(Packet.body.strm.data, pStream->pOutData+pStream->iOutProg+sizeof(Block)+Block.iSent, iSize);
            Packet.head.len = (uint16_t)(sizeof(Packet.body.strm)-sizeof(Packet.body.strm.data)+iSize);
            // try and queue/send the packet
            iResult = NetGameLinkSend(pRef, (NetGamePacketT *)&Packet, 1);
            if (iResult < 0)
            {
                NetPrintf(("netgamelink: [%p] stream send failed for stream 0x%08x!\n", pRef, pStream));
                break;
            }
            else if (iResult == 0)
            {
                break;
            }
            // advance the sent count
            Block.iSent += iSize;
            if (Block.iSent != Block.iSize)
            {
                // save back updated block for next time (could just update .sent, but this is easier due to alignment)
                ds_memcpy(pStream->pOutData+pStream->iOutProg, &Block, sizeof(Block));
            }
            else
            {
                pStream->iOutProg += sizeof(Block)+Block.iSize;
                // see if we should shift the buffer (more than 75% full and shift would gain >33%)
                if ((pStream->iOutProg < pStream->iOutSize) &&
                    (pStream->iOutSize*4 > pStream->iOutMaxm*3) &&
                    (pStream->iOutProg*3 > pStream->iOutMaxm*1))
                {
                    // do the shift
                    memmove(pStream->pOutData, pStream->pOutData+pStream->iOutProg, pStream->iOutSize-pStream->iOutProg);
                    pStream->iOutSize -= pStream->iOutProg;
                    pStream->iOutProg = 0;
                }
            }
            // count the send
            ++iCount;
        }
    }

    // return send count
    return(iCount);
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkSendStream

    \Description
        Let user queue up a buffer to send

    \Input *pStream - stream to send on
    \Input iSubChan - subchannel stream is to be received on
    \Input iKind    - kind of data (used to determine if sync)
    \Input *pBuffer - pointer to data to send
    \Input iLength  - length of data to send

    \Output
        int32_t     - negative=error, zero=busy (send again later), positive=sent

    \Version 01/11/10 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _NetGameLinkSendStream(NetGameLinkStreamT *pStream, int32_t iSubChan, int32_t iKind, void *pBuffer, int32_t iLength)
{
    GameStreamT Block;

    // if this is an orphaned stream, return an error
    if (pStream->pClient == NULL)
    {
        return(-1); //todo
    }

    // allow negative length to mean strlen(data)+1
    if (iLength < 0)
    {
        for (iLength = 0; ((char *)pBuffer)[iLength] != '\0'; ++iLength)
            ;
        ++iLength;
    }

    // make sure send buffer does not exceed total input buffer
    // (do this AFTER negative length check)
    if (iLength >= pStream->iInpMaxm)
    {
        return(-1);
    }

    // see if we can reset buffer pointers
    if (pStream->iOutProg >= pStream->iOutSize)
    {
        pStream->iOutProg = pStream->iOutSize = 0;
    }

    // make sure send buffer does not exceed current output buffer
    if (iLength+sizeof(GameStreamT) >= (unsigned)pStream->iOutMaxm-pStream->iOutSize)
    {
        return(0);
    }

    // if this is a sync, make sure we have space in sync buffer
    if ((iKind > '~   ') && (iLength+sizeof(GameStreamT) >= (unsigned)pStream->iSynMaxm-pStream->iSynSize))
    {
        return(0);
    }

    // see if this is a length query
    if (pBuffer == NULL)
    {
        // calc main buffer space
        iLength = pStream->iOutMaxm-pStream->iOutSize;
        // if this is sync, limit based on sync buffer
        if ((iKind > '~   ') && (iLength > pStream->iSynMaxm-pStream->iSynSize))
        {
            iLength = pStream->iSynMaxm-pStream->iSynSize;
        }
        // subtract packet header size
        iLength -= sizeof(GameStreamT);
        if (iLength < 0)
        {
            iLength = 0;
        }
        // return available size
        return(iLength);
    }

    // setup the header block
    Block.iKind = iKind;
    Block.uSync = 0;
    Block.iSent = 0;
    Block.iSize = iLength;
    Block.iSubchan = iSubChan;

    // if sync, copy into sync buffer
    if (iKind > '~   ')
    {
        // set sync sequence number
        Block.uSync = 0;
        // copy into sync byffer
        ds_memcpy(pStream->pSynData+pStream->iSynSize, &Block, sizeof(Block));
        ds_memcpy(pStream->pSynData+pStream->iSynSize+sizeof(Block), pBuffer, iLength);
        pStream->iSynSize += sizeof(Block)+iLength;
    }

    // do a memmove just in case data source is already in this buffer
    // (this is an optimization used for database access)
    memmove(pStream->pOutData+pStream->iOutSize+sizeof(Block), pBuffer, iLength);
    ds_memcpy(pStream->pOutData+pStream->iOutSize, &Block, sizeof(Block));
    pStream->iOutSize += sizeof(Block)+iLength;

    if (pStream->iOutSize > pStream->iHighWaterUsed)
    {
        pStream->iHighWaterUsed = pStream->iOutSize;
    }
    if ((pStream->iOutSize - pStream->iOutProg) > pStream->iHighWaterNeeded)
    {
        pStream->iHighWaterNeeded = pStream->iOutSize - pStream->iOutProg;
    }

    // attempt immediate send
    _NetGameLinkPollStream(pStream->pClient);

    // return send status
    return(iLength);
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkRecvStream

    \Description
        Process a received stream packet

    \Input *pRef    - reference pointer
    \Input *pPacket - packet to process

    \Output
        int32_t     - -1=no match, -2=overflow error, 0=valid packet received

    \Version 01/11/10 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _NetGameLinkRecvStream(NetGameLinkRefT *pRef, NetGamePacketT *pPacket)
{
    int32_t iRes = -1, iSize;
    NetGameLinkStreamT *pStream;
    NetGameLinkStreamInpT *pInp;
    int32_t iSubChannel;

    // see if this packet matches any of the streams
    for (pStream = pRef->m_stream; pStream != NULL; pStream = pStream->pNext)
    {
        // see if this is the matching stream
        if (pStream->iIdent == pPacket->body.strm.ident)
        {
            // extract size/subchannel from 'size' member
            iSize = pPacket->body.strm.size & ~0xff000000;
            iSubChannel = (unsigned)pPacket->body.strm.size >> 24;
            if ((iSubChannel < 0) || (iSubChannel >= (pStream->iSubchan+1)))
            {
                NetPrintf(("netgamelink: [%p] warning, received packet on stream '%C' with invalid packet subchannel %d\n", pRef, pPacket->body.strm.ident, iSubChannel));
                return(-1);
            }
            // ref channel
            pInp = pStream->pInp + iSubChannel;
            // default to overflow error
            iRes = GMDIST_OVERFLOW;
            // validate the packet size
            if (iSize <= pStream->iInpMaxm)
            {
                /* auto-clear packet if anything looks inconsistant -- note that this is expected behavior
                   for the first packet fragment in a sequence of packet fragments */
                if ((pPacket->body.strm.kind != pInp->iInpKind) || (iSize != pInp->iInpSize))
                {
                    // reset progress
                    pInp->iInpProg = 0;
                    // save basic packet header
                    pInp->iInpKind = pPacket->body.strm.kind;
                    pInp->iInpSize = iSize;
                }
                // figure out size of data in packet
                iSize = pPacket->head.len-(sizeof(pPacket->body.strm)-sizeof(pPacket->body.strm.data));
                if (iSize > pInp->iInpSize-pInp->iInpProg)
                {
                    NetPrintf(("netgamelink: [%p] clamped stream packet size from %d to %d on stream '%C'\n", pRef, iSize, pInp->iInpSize-pInp->iInpProg, pPacket->body.strm.ident));
                    iSize = pInp->iInpSize-pInp->iInpProg;
                }
                // append to existing packet
                ds_memcpy(pInp->pInpData+pInp->iInpProg, pPacket->body.strm.data, iSize);
                pInp->iInpProg += iSize;
                // see if packet is complete
                if (pInp->iInpProg == pInp->iInpSize)
                {
                    // deliver packet
                    if (pStream->Recv != NULL)
                    {
                        pStream->Recv(pStream, iSubChannel, pInp->iInpKind, pInp->pInpData, pInp->iInpSize);
                    }
                    else
                    {
                        NetPrintf(("netgamelink: [%p] no registered stream recv handler on stream '%C'\n", pRef, pPacket->body.strm.ident));
                    }
                    // clear from buffer
                    pInp->iInpProg = 0;
                }
                // packet was valid
                iRes = 0;
            }
            else
            {
                NetPrintf(("netgamelink: [%p] invalid stream packet size (%d >= %d) on stream '%C'\n",
                    pRef, pPacket->body.strm.size, pStream->iInpMaxm, pPacket->body.strm.ident));
            }
            return(iRes);
        }
    }

    // didn't find the stream
    NetPrintf(("netgamelink: [%p] could not find stream for stream packet with iIdent '%C'\n", pRef, pPacket->body.strm.ident));

    // return result
    return(iRes);
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkUpdateQos

    \Description
        Send and receive QoS packets.

    \Input *pRef    - reference pointer

    \Version 09/03/14 (mcorcoran)
*/
/*************************************************************************************************F*/
static void _NetGameLinkUpdateQos(NetGameLinkRefT *pRef)
{
    NetGameMaxPacketT QosPacket;
    uint32_t uNow = NetTick();

    // save QoS start time
    if (pRef->uQosStartTime == 0)
    {
        pRef->uQosStartTime = uNow;
    }

    // init QoS send time
    if ((pRef->uQosPacketsToSend > 0) && (pRef->uQosLastSendTime == 0))
    {
        /* We are now expecting QoS packet from the other end of the link.
           Make this value non-zero to satisfy the while() condition below. */
        pRef->uQosPacketsToRecv = 1;
        pRef->uQosLastSendTime = uNow - (pRef->uQosSendInterval+1);
    }

    // check if it's time to send a QoS packet.
    if ((pRef->uQosPacketsToSend > 0) && (NetTickDiff(uNow, pRef->uQosLastSendTime) > (signed)pRef->uQosSendInterval))
    {
        // set qos last send time (reserve zero for uninitialized)
        pRef->uQosLastSendTime = (uNow != 0) ? uNow : uNow-1;
        pRef->uQosPacketsToSend -= 1;

        // create and send the packet
        QosPacket.head.kind = GAME_PACKET_USER;
        QosPacket.head.len = pRef->uQosPacketSize;

        ds_memclr(QosPacket.body.data, QosPacket.head.len);

        // the other end of the link needs to know how many more packets to expect.
        QosPacket.body.data[0] = (uint8_t)(pRef->uQosPacketsToSend >> 8);
        QosPacket.body.data[1] = (uint8_t)(pRef->uQosPacketsToSend >> 0);

        NetGameLinkSend(pRef, (NetGamePacketT*)&QosPacket, 1);
        NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: [%p] sent QoS packet, size(%d), remaining packets to send (%d)\n", pRef, QosPacket.head.len, pRef->uQosPacketsToSend));
    }

    while ((pRef->uQosPacketsToRecv > 0) && NetGameLinkRecv(pRef, (NetGamePacketT*)&QosPacket, 1, FALSE))
    {
        // the other end of the link tells us how many more packets to expect (wait for before transitioning to active)
        pRef->uQosPacketsToRecv =
            (QosPacket.body.data[0] << 8) |
            (QosPacket.body.data[1] << 0);

        NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: [%p] received QoS packet, size(%d), remaining packets to recv (%d)\n", pRef, QosPacket.head.len, pRef->uQosPacketsToRecv));
    }

    // have we finished sending and receiving everything yet?
    if ((pRef->uQosPacketsToSend == 0) && (pRef->uQosPacketsToRecv == 0))
    {
        // we're done, print available stats
        _NetGameLinkPrintStats(pRef);
    }

    // only start averaging latency when 1 sample period is complete
    if (NetTickDiff(uNow, pRef->uQosStartTime) > RTT_SAMPLE_PERIOD)
    {
        pRef->iQosCumulLate += pRef->NetGameLinkStats.late;
        pRef->iQosLateCount++;
        pRef->iQosAvgLate = pRef->iQosCumulLate / pRef->iQosLateCount;
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkDrainRecvStream

    \Description
        Need for streams handling.  Received all data on the stream.

    \Input *pRef    - reference pointer

    \Version 09/03/14 (mcorcoran)
*/
/*************************************************************************************************F*/
static void _NetGameLinkDrainRecvStream(NetGameLinkRefT *pRef)
{
    NetGameMaxPacketT Packet;

    while (NetGameLinkRecv2(pRef, (NetGamePacketT*)&Packet, 1, 1 << GAME_PACKET_LINK_STRM))
    {
        // convert stream header from network to host order
        Packet.body.strm.ident = SocketNtohl(Packet.body.strm.ident);
        Packet.body.strm.kind = SocketNtohl(Packet.body.strm.kind);
        Packet.body.strm.size = SocketNtohl(Packet.body.strm.size);

        // process the packet
        _NetGameLinkRecvStream(pRef, (NetGamePacketT *)&Packet);
    }
}

/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkCreate

    \Description
        Construct the game client

    \Input *pCommRef        - the connection from NetGameUtilComplete()
    \Input iOwner           - if TRUE, NetGameLink will assume ownership of the port (ie, delete it when done)
    \Input iBufLen          - length of input buffer

    \Output
        NetGameLinkRefT *   - pointer to new NetGameLinkRef

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
NetGameLinkRefT *NetGameLinkCreate(void *pCommRef, int32_t iOwner, int32_t iBufLen)
{
    int32_t iIndex;
    uint32_t uTick;
    CommRef *pPort = pCommRef;
    NetGameLinkRefT *pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    NetGameMaxPacketT Packet;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pRef = DirtyMemAlloc(sizeof(*pRef), NETGAMELINK_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netgamelink: unable to allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pRef, sizeof(*pRef));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;

    // assign port info
    pRef->pPort = pPort;
    pRef->iPortOwner = iOwner;

    // allocate input buffer
    if (iBufLen < 4096)
    {
        iBufLen = 4096;
    }
    pRef->pBuffer = DirtyMemAlloc(iBufLen, NETGAMELINK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    pRef->iBufMax = iBufLen;

    // reset the timing info
    pRef->iRttAvg = 0;
    pRef->iRttDev = 0;

    // setup connection time for reference
    uTick = NetTick();
    pRef->NetGameLinkStats.conn = uTick;
    pRef->NetGameLinkStats.senthide = uTick;
    pRef->NetGameLinkStats.rcvdhide = uTick;
    pRef->NetGameLinkStats.rcvdlast = uTick;
    pRef->NetGameLinkStats.sentlast = uTick;
    pRef->NetGameLinkStats.stattick = uTick;
    pRef->NetGameLinkStats.isconn = FALSE;
    pRef->NetGameLinkStats.isopen = FALSE;

    // fill ping history with bogus starting data
    for (iIndex = 0; iIndex < PING_HISTORY; ++iIndex)
    {
        pRef->NetGameLinkStats.pinghist[iIndex].min = PING_DEFAULT;
        pRef->NetGameLinkStats.pinghist[iIndex].max = PING_DEFAULT;
        pRef->NetGameLinkStats.pinghist[iIndex].avg = PING_DEFAULT;
        pRef->NetGameLinkStats.pinghist[iIndex].cnt = 1;
    }

    // setup critical section
    NetCritInit(&pRef->Crit, "netgamelink");

    // setup for callbacks
    pPort->refptr = pRef;
    pPort->Callback(pPort, _NetGameLinkNotify);
    pPort->SendCallback = _NetGameLinkSendCallback;

    // default sending and syncs to enabled
    pRef->bSendEnabled = TRUE;
    pRef->bSyncEnabled = TRUE;

    // calculate the max packet size based on the cudp max width (minus the stream overhead)
    pRef->iStrmMaxPktSize = pPort->maxwid - (sizeof(Packet.body.strm) - sizeof(Packet.body.strm.data));

    // other QoS items memset() to zero above
    pRef->uQosSendInterval = NETGAMELINK_QOS_INTERVAL_MIN;
    pRef->uQosPacketSize = NETGAME_DATAPKT_MAXSIZE;

    pRef->iVerbosity = 1;

    return(pRef);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkDestroy

    \Description
        Destruct the game client

    \Input *pRef    - reference pointer

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
void NetGameLinkDestroy(NetGameLinkRefT *pRef)
{
    // dont need callback
    pRef->pPort->Callback(pRef->pPort, NULL);

    // we own the port -- get rid of it
    if (pRef->iPortOwner)
        pRef->pPort->Destroy(pRef->pPort);

    // free receive buffer
    DirtyMemFree(pRef->pBuffer, NETGAMELINK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);

    // free critical section
    NetCritKill(&pRef->Crit);

    // free our memory
    DirtyMemFree(pRef, NETGAMELINK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkCallback

    \Description
        Register a callback function

    \Input *pRef        - reference pointer
    \Input *pCallData   - caller reference data
    \Input *pCallProc   - callback function

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
void NetGameLinkCallback(NetGameLinkRefT *pRef, void *pCallData, void (*pCallProc)(void *pCallData, int32_t iKind))
{
    pRef->pCallData = pCallData;
    pRef->pCallProc = pCallProc;
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkStatus

    \Description
        Return current link status

    \Input *pRef        - reference pointer
    \Input iSelect      - selector
    \Input iValue       - input value
    \Input pBuf         - output buffer
    \Input iBufSize     - output buffer size

    \Output
        int32_t         - selector specific return value

    \Notes
        iSelect can be one of the following:

        \verbatim
            'mwid' - returns the commudp packet max width field
            'qlat' - average latency calculated during the initial qos phase
            'sinf' - returns SocketInfo() with iValue being the status selector (iData param is unsupported)
            'slen' - returns current length of send queue
            'sque' - returns TRUE if send queue is empty, else FALSE
            'stat' - Fills out a NetGameLinkStatT with QoS info. pBuf=&NetGameLinkStatT, iBufSize=sizeof(NetGameLinkStatT)
        \endverbatim

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
int32_t NetGameLinkStatus(NetGameLinkRefT *pRef, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    // read commudp mwid field
    if (iSelect == 'mwid')
    {
        return(pRef->pPort->maxwid);
    }
    if (iSelect == 'qlat')
    {
        return(pRef->iQosAvgLate);
    }
    if ((iSelect == 'sinf') && (pRef->pPort != NULL) && (pRef->pPort->sockptr != NULL))
    {
        return(SocketInfo(pRef->pPort->sockptr, iValue, 0, pBuf, iBufSize));
    }
    if ((iSelect == 'sque') || (iSelect == 'slen'))
    {
        // get output queue position
        int32_t iQueue = pRef->pPort->Send(pRef->pPort, NULL, 0, COMM_FLAGS_RELIABLE);

        if (iSelect == 'sque')
        {
            // if output queue position is one send queue is empty (assume error=empty)
            iQueue = (iQueue > 0) ? (iQueue == 1) : TRUE;
        }
        return(iQueue);
    }
    if (iSelect == 'stat')
    {
        volatile uint32_t uSeqn;
        uint32_t uPortStat;

        do
        {
            // remember pre-update ticks
            uSeqn = pRef->NetGameLinkStats.tickseqn;

            // update tick is same as movement tick
            pRef->NetGameLinkStats.tick = NetTick();

        // if things changed during out assignment, do it again
        } while (pRef->NetGameLinkStats.tickseqn != uSeqn);

        // update port status
        uPortStat = pRef->pPort->Status(pRef->pPort);
        pRef->NetGameLinkStats.isopen = ((uPortStat == COMM_CONNECTING) || (uPortStat == COMM_ONLINE)) ? TRUE : FALSE;
        pRef->NetGameLinkStats.isopen = pRef->NetGameLinkStats.isopen && (pRef->uQosPacketsToSend == 0) && (pRef->uQosPacketsToRecv == 0);

        // update packet sent counter
        pRef->NetGameLinkStats.lpacksent_now = pRef->pPort->packsent;

        // make sure user-provided buffer is large enough to receive a pointer
        if ((pBuf != NULL) && (iBufSize >= (int32_t)sizeof(NetGameLinkStatT)))
        {
            ds_memcpy(pBuf, &pRef->NetGameLinkStats, sizeof(NetGameLinkStatT));
            return(0);
        }
        else
        {
            // unhandled
            return(-1);
        }
    }
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkSend

    \Description
        Handle incoming packet stream from upper layer

    \Input *pRef    - reference pointer
    \Input *pPkt    - packet list (1 or more)
    \Input iLen     - size packet list (1=one packet)

    \Output
        int32_t     - negative=bad error, zero=unable to send now (overflow), positive=bytes sent

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
int32_t NetGameLinkSend(NetGameLinkRefT *pRef, NetGamePacketT *pPkt, int32_t iLen)
{
    int32_t iCnt = 0, iErr;
    uint32_t uCurrTick = NetTick();

    // get exclusive access
    NetCritEnter(&pRef->Crit);

    // see if we need to handle missed event
    if (pRef->iProcess > 0)
    {
        NetPrintf(("netgamelink: processing missed event\n"));
        _NetGameLinkProcess(pRef, uCurrTick);
    }

    // walk the packet list
    while (iLen > 0)
    {
        if ((iErr = _NetGameLinkSendPacket(pRef, pPkt, uCurrTick)) <= 0)
        {
            // don't spam on overflow
            if (iErr != 0)
            {
                NetPrintf(("netgamelink: error %d sending packet\n", iErr));
            }
            // should _NetGameLinkSendPacket fail at first send attempt, return err to caller.
            if (iCnt == 0)
            {
                iCnt = iErr;
            }
            break;
        }

        // calc packet size for them
        pPkt->head.size = (sizeof(pPkt->head)+pPkt->head.len+sizeof(NetGamePacketSyncT)+3)&0x7ffc;
        // count the packet size
        iCnt += pPkt->head.size;
        // stop if this was single packet
        if (iLen == 1)
        {
            break;
        }

        // skip to next packet
        iLen -= pPkt->head.size;
        pPkt = (NetGamePacketT *)(((char *)pPkt)+pPkt->head.size);
    }

    // release exclusive access
    NetCritLeave(&pRef->Crit);

    // return bytes sent
    return(iCnt);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkPeek2

    \Description
        Peek into the buffer, for the first packet type matching mask

    \Input *pRef    - reference pointer
    \Input **ppPkt  - (optional) storage for pointer to current packet data
    \Input uMask    - which packet types we want. bitmask, addressed by GAME_PACKET values

    \Output
        int32_t     - buffer size

    \Version 08/08/11 (jrainy)
*/
/*************************************************************************************************F*/
int32_t NetGameLinkPeek2(NetGameLinkRefT *pRef, NetGamePacketT **ppPkt, uint32_t uMask)
{
    int32_t iBufLen = pRef->iBufLen;
    int32_t iIndex, iKind;
    NetGamePacketT *pPkt0;
    int32_t iPktSize = 0;

    // I do not understand the reason for && (iBufLen > 0).
    // Kept to maintain current behaviour
    // TODO: investigate
    if ((ppPkt != NULL) && (iBufLen > 0))
    {
        // while going through our buffer of received packets
        for (iIndex = 0; iIndex < pRef->iBufLen;)
        {
            // extract size of current head packet
            pPkt0 = (NetGamePacketT *)(pRef->pBuffer + iIndex);
            iPktSize = pPkt0->head.size;

            // if the current packet matches what we want

            iKind = (((NetGamePacketT *)(pRef->pBuffer + iIndex))->head.kind) & ~GAME_PACKET_SYNC;

            if (uMask & (1 << iKind))
            {
                *ppPkt = pPkt0;
                break;
            }
            else
            {
                // skip over the non-matching packets.
                iIndex += iPktSize;
            }
        }
    }
    else if (ppPkt != NULL)
    {
        *ppPkt = NULL;
    }


    // return buffer size
    return(iBufLen);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkPeek

    \Description
        Peek into the buffer

    \Input *pRef    - reference pointer
    \Input **ppPkt  - (optional) storage for pointer to current packet data

    \Output
        int32_t     - buffer size

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
int32_t NetGameLinkPeek(NetGameLinkRefT *pRef, NetGamePacketT **ppPkt)
{
    return(NetGameLinkPeek2(pRef, ppPkt, (unsigned)~0));
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkRecv2

    \Description
        Outgoing packet stream to upper layer

    \Input *pRef    - reference pointer
    \Input *pBuf    - storage for packet list (1 or more)
    \Input iLen     - size packet list (1=one packet)
    \Input uMask    - which packet types we wanted. bitmask, addressed by GAME_PACKET values

    \Output
        int32_t     - 0=no data available, else number of packets received

    \Version 01/11/10 (jrainy)
*/
/*************************************************************************************************F*/
int32_t NetGameLinkRecv2(NetGameLinkRefT *pRef, NetGamePacketT *pBuf, int32_t iLen, uint32_t uMask)
{
    NetGamePacketT *pPkt;
    uint32_t uCurrTick = NetTick();
    int32_t iIndex, iKind;
    int32_t iLenRead = 0;
    int32_t iPktSize = 0;

    // disable callback
    NetCritEnter(&pRef->Crit);

    // if no data in queue, check with lower layer
    if (pRef->iBufLen == 0)
    {
        while (_NetGameLinkRecvPacket(pRef, uCurrTick) > 0)
            ;
    }

    // see if there is anything to process
    if (pRef->iBufLen > 0)
    {
        // while going through our buffer of received packets
        for (iIndex = 0; iIndex < pRef->iBufLen;)
        {
            // extract size of current head packet
            pPkt = (NetGamePacketT *)(pRef->pBuffer + iIndex);
            iPktSize = pPkt->head.size;

            // don't access pkt past here, as the memmove will overwrite it

            // if the current packet matches what we want

            iKind = (((NetGamePacketT *)(pRef->pBuffer + iIndex))->head.kind) & ~GAME_PACKET_SYNC;

            if (uMask & (1 << iKind))
            {
                // if we can fit it in
                if ((iPktSize <= iLen) || (iLen == 1))
                {
                    // copy the packet to return buffer
                    ds_memcpy(pBuf, pRef->pBuffer + iIndex, iPktSize);
                    iLen -= iPktSize;
                    pBuf = (NetGamePacketT *)(((char*)pBuf) + iPktSize);
                    iLenRead += iPktSize;

                    // remove from our list of received packets
                    memmove(pRef->pBuffer + iIndex, pRef->pBuffer + iIndex + iPktSize, pRef->iBufLen - iIndex - iPktSize);
                    pRef->iBufLen -= iPktSize;

                    // if len was 1, it's now negative, we got our packet, bail out
                    if (iLen < 0)
                    {
                        break;
                    }
                }
                else
                {
                    // bail out, we got all we could
                    break;
                }
            }
            else
            {
                // skip over the non-matching packets.
                iIndex += iPktSize;
            }
        }
    }

    // enable access
    NetCritLeave(&pRef->Crit);

    // return length
    return(iLenRead);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkRecv

    \Description
        Outgoing packet stream to upper layer

    \Input *pRef    - reference pointer
    \Input *pBuf    - storage for packet list (1 or more)
    \Input iLen     - size packet list (1=one packet)
    \Input bDist    - whether dist packets are to be received. Use FALSE.

    \Output
        int32_t     - 0=no data available, else number of packets received

    \Version 01/11/10 (jrainy)
*/
/*************************************************************************************************F*/
int32_t NetGameLinkRecv(NetGameLinkRefT *pRef, NetGamePacketT *pBuf, int32_t iLen, uint8_t bDist)
{
    uint32_t uDistMask =    (1 << GAME_PACKET_INPUT) |
                            (1 << GAME_PACKET_INPUT_MULTI) |
                            (1 << GAME_PACKET_INPUT_MULTI_FAT) |
                            (1 << GAME_PACKET_STATS) |
                            (1 << GAME_PACKET_INPUT_FLOW) |
                            (1 << GAME_PACKET_INPUT_META);

    return(NetGameLinkRecv2(pRef, pBuf, iLen, (bDist ? uDistMask : ~uDistMask) & ~(1 << GAME_PACKET_LINK_STRM)));
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkControl

    \Description
        NetGameLink control function.  Different selectors control different behaviors.

    \Input *pRef    - reference pointer
    \Input iControl - control selector
    \Input iValue   - selector-specific value
    \Input pValue   - selector-specific pointer

    \Output
        int32_t     - selector specific, or negative if failure

    \Notes
        iControl can be one of the following:

        \verbatim
            lqos: set individual QoS packet size
            rlmt: set redundant packet size limit
            send: enable/disable sending
            spam: set debug verbosity level
            sqos: set QoS duration and interval
            sync: enable/disable sync packets
        \endverbatim

        Unhandled selectors are passed through to the underlying comm module

    \Version 07/07/03 (jbrookes)
*/
/*************************************************************************************************F*/
int32_t NetGameLinkControl(NetGameLinkRefT *pRef, int32_t iControl, int32_t iValue, void *pValue)
{
    // set individual QoS packet size
    if (iControl == 'lqos')
    {
        if ((iValue < NETGAMELINK_QOS_PACKETSIZE_MIN) || (iValue > NETGAME_DATAPKT_MAXSIZE))
        {
            NetPrintf(("netgamelink: [%p] invalid QoS packet size specified (%d). QoS packets must be >= %d and <= %d\n", pRef, iValue, NETGAMELINK_QOS_PACKETSIZE_MIN, NETGAME_DATAPKT_MAXSIZE));
            iValue = (iValue < NETGAMELINK_QOS_PACKETSIZE_MIN ? NETGAMELINK_QOS_PACKETSIZE_MIN : iValue);
            iValue = (iValue > NETGAME_DATAPKT_MAXSIZE ? NETGAME_DATAPKT_MAXSIZE : iValue);
        }
        NetPrintf(("netgamelink: [%p] QoS packet size set to %d\n", pRef, iValue));
        pRef->uQosPacketSize = iValue;
        return(0);
    }
    // set redundant packet size limit
    if (iControl == 'rlmt')
    {
        NetPrintf(("netgamelink: [%p] updating redundant packet size limit for underlying comm (%d)\n", pRef, iValue));
        return(pRef->pPort->Control(pRef->pPort, iControl, iValue, pValue));
    }
    // set send/recv value
    if (iControl == 'send')
    {
        pRef->bSendEnabled = iValue;
        return(1);
    }
    // set debug verbosity level
    if (iControl == 'spam')
    {
        if (iValue >= 0 && iValue <= 5)
        {
            pRef->iVerbosity = iValue;
            return(1);
        }
        return(0);
    }
    // set QoS duration and interval
    if (iControl == 'sqos')
    {
        int32_t iValue2 = *(int32_t*)pValue;

        if (pRef->uQosLastSendTime != 0)
        {
            NetPrintf(("netgamelink: [%p] cannot change QoS duration or interval while QoS is in progress or is already finished.", pRef));
            return(-1);
        }

        if ((iValue < NETGAMELINK_QOS_DURATION_MIN) || (iValue > NETGAMELINK_QOS_DURATION_MAX))
        {
            NetPrintf(("netgamelink: [%p] invalid QoS duration period provided (%d). QoS duration must be >= %d and <= %s ms\n", pRef, iValue, NETGAMELINK_QOS_DURATION_MIN, NETGAMELINK_QOS_DURATION_MAX));
            iValue = (iValue < NETGAMELINK_QOS_DURATION_MIN ? NETGAMELINK_QOS_DURATION_MIN : iValue);
            iValue = (iValue > NETGAMELINK_QOS_DURATION_MAX ? NETGAMELINK_QOS_DURATION_MAX : iValue);
        }
        if ((iValue != 0) && ((iValue2 < NETGAMELINK_QOS_INTERVAL_MIN) || (iValue2 > iValue)))
        {
            NetPrintf(("netgamelink: [%p] invalid QoS interval provided (%d). QoS interval must be >= %d and <= 'QoS duration period'\n", pRef, iValue2, NETGAMELINK_QOS_INTERVAL_MIN));
            iValue2 = (iValue2 < NETGAMELINK_QOS_INTERVAL_MIN ? NETGAMELINK_QOS_INTERVAL_MIN : iValue2);
            iValue2 = (iValue2 > iValue ? iValue : iValue2);
        }
        NetPrintf(("netgamelink: [%p] QoS duration period set to %d ms %s\n", pRef, iValue, ((iValue == 0) ? "(QoS disabled)" : "")));

        pRef->uQosPacketsToSend = (iValue == 0 ? 0 : iValue / iValue2);
        NetPrintf(("netgamelink: [%p] QoS packet count set to %d packets %s\n", pRef, pRef->uQosPacketsToSend, ((pRef->uQosPacketsToSend == 0) ? "(QoS disabled)" : "")));

        pRef->uQosSendInterval = iValue2;
        NetPrintf(("netgamelink: [%p] QoS interval set to %d ms\n", pRef, pRef->uQosSendInterval));

        return(0);
    }
    // queue checks (Deprecated use netgamelinkstatus versions instead) $$todo Remove in future release
    if ((iControl == 'sque') || (iControl == 'slen'))
    {
        // get output queue position
        int32_t iQueue = pRef->pPort->Send(pRef->pPort, NULL, 0, COMM_FLAGS_RELIABLE);

        if (iControl == 'sque')
        {
            // if output queue position is one send queue is empty (assume error=empty)
            iQueue = (iQueue > 0) ? (iQueue == 1) : TRUE;
        }
        return(iQueue);
    }
    // enable/disable sync packets
    if (iControl == 'sync')
    {
        pRef->bSyncEnabled = iValue;
        return(0);
    }
    // unhandled; pass through to comm module if available
    return((pRef->pPort != NULL) ? pRef->pPort->Control(pRef->pPort, iControl, iValue, pValue) : -1);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkUpdate

    \Description
        Provides NetGameLink with time, at regular interval. Need for streams handling.
        Also handles QoS stage when during link startup.

    \Input *pRef    - reference pointer

    \Output
        uint32_t    - zero

    \Version 01/11/10 (jrainy)
*/
/*************************************************************************************************F*/
uint32_t NetGameLinkUpdate(NetGameLinkRefT *pRef)
{
    // Are we currently in the QoS phase?
    if ((pRef->uQosPacketsToSend > 0) || (pRef->uQosPacketsToRecv > 0))
    {
        // Send and receive QoS packets
        _NetGameLinkUpdateQos(pRef);
    }
    else
    {
        // Handle normal link data
        _NetGameLinkDrainRecvStream(pRef);

        _NetGameLinkPollStream(pRef);
    }

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkCreateStream

    \Description
        Allocate a stream

    \Input *pRef            - module state reference
    \Input iSubChan         - number of subchannels (zero for a normal stream)
    \Input iIdent           - unique stream iIdent
    \Input iInpLen          - size of input buffer
    \Input iOutLen          - size of output buffer
    \Input iSynLen          - size of sync buffer

    \Output
        NetGameLinkStreamT * - new stream, or NULL if the iIdent was not unique

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
NetGameLinkStreamT *NetGameLinkCreateStream(NetGameLinkRefT *pRef, int32_t iSubChan, int32_t iIdent, int32_t iInpLen, int32_t iOutLen, int32_t iSynLen)
{
    NetGameLinkStreamT *pStream;
    int32_t iInpSize;
    char *pInpData;

    // make sure the pipe identifier is unique
    for (pStream = pRef->m_stream; pStream != NULL; pStream = pStream->pNext)
    {
        // dont create a duplicate stream
        if (pStream->iIdent == iIdent)
        {
            NetPrintf(("netgamelink: [%p] error -- attempting to create duplicate stream '%c%c%c%c'\n",
                pRef, (uint8_t)(iIdent>>24), (uint8_t)(iIdent>>16), (uint8_t)(iIdent>>8), (uint8_t)iIdent));
            return(NULL);
        }
    }

    // allocate a pipe record
    if ((pStream = (NetGameLinkStreamT *) DirtyMemAlloc(sizeof(*pStream), NETGAMELINK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netgamelink: [%p] unable to allocate stream\n", pRef));
        return(NULL);
    }
    ds_memclr(pStream, sizeof(*pStream));

    // setup the data fields
    pStream->pClient = pRef;
    pStream->iIdent = iIdent;
    pStream->iSubchan = iSubChan;
    pStream->Send = _NetGameLinkSendStream;
    pStream->Recv = NULL;

    // allocate input buffer plus input buffer tracking structure(s)
    pStream->iInpMaxm = iInpLen;
    iInpSize = sizeof(*pStream->pInp) * (pStream->iSubchan + 1);
    pStream->pInp = DirtyMemAlloc(iInpSize + (iInpLen * (pStream->iSubchan + 1)), NETGAMELINK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    ds_memclr(pStream->pInp, iInpSize);
    for (iSubChan = 0, pInpData = (char *)pStream->pInp + iInpSize; iSubChan < pStream->iSubchan+1; iSubChan++)
    {
        pStream->pInp[iSubChan].pInpData = pInpData;
        pInpData += iInpLen;
    }

    // allocate output buffer
    if (iOutLen < iInpLen)
    {
        iOutLen = iInpLen;
    }
    pStream->iOutMaxm = iOutLen;
    pStream->pOutData = DirtyMemAlloc(iOutLen, NETGAMELINK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);

    // allocate sync buffer
    pStream->iSynMaxm = iSynLen;
    if (iSynLen > 0)
    {
        pStream->pSynData = DirtyMemAlloc(iSynLen, NETGAMELINK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    }

    // put into list
    pStream->pNext = pRef->m_stream;
    pRef->m_stream = pStream;
    return(pStream);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkDestroyStream

    \Description
        Destroy a stream

    \Input *pRef    - reference pointer
    \Input *pStream - pointer to stream to destroy

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
void NetGameLinkDestroyStream(NetGameLinkRefT *pRef, NetGameLinkStreamT *pStream)
{
    NetGameLinkStreamT **ppLink;

    // make sure stream is valid
    if (pStream != NULL)
    {
        // if stream is active, remove the link
        if (pStream->pClient != NULL)
        {
            // locate the stream in the list for removal
            for (ppLink = &pStream->pClient->m_stream; *ppLink != pStream; ppLink = &((*ppLink)->pNext))
            {
                // see if at end of list
                if (*ppLink == NULL)
                {
                    return;
                }
            }
            // remove stream from list
            *ppLink = pStream->pNext;
        }

        // now the tricky part-- make sure nobody is still processing this stream

        // release the resources
        if (pStream->pSynData != NULL)
        {
            DirtyMemFree(pStream->pSynData, NETGAMELINK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        }
        DirtyMemFree(pStream->pInp, NETGAMELINK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        DirtyMemFree(pStream->pOutData, NETGAMELINK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        DirtyMemFree(pStream, NETGAMELINK_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    }
}

