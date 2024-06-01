/*H*************************************************************************************************/
/*!

    \File    commudp.c

    \Description
        This is a reliable UDP transport class optimized for use in a
        controller passing game applications. When the actual data
        bandwidth is low (as is typical with controller passing), it
        sends redundant data in order to quickly recover from any lost
        packets. Overhead is low adding only 8 bytes per packet in
        addition to UDP/IP overhead. This module support mutual
        connects in order to allow connections through firewalls.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 1999-2003.  ALL RIGHTS RESERVED.

    \Version    0.1        02/09/99 (GWS) First Version
    \Version    0.2        02/14/99 (GWS) Revised and enhanced
    \Version    0.5        02/14/99 (GWS) Alpha release
    \Version    1.0        07/30/99 (GWS) Final release
    \Version    2.0        10/27/99 (GWS) Revised to use winsock 1.1/2.0
    \Version    2.1        12/04/99 (GWS) Removed winsock 1.1 support
    \Version    2.2        01/12/00 (GWS) Fixed receive tick bug
    \Version    2.3        06/12/00 (GWS) Added fastack for low-latency nets
    \Version    2.4        12/04/00 (GWS) Added firewall penetrator
    \Version    3.0        12/04/00 (GWS) Reported to dirtysock
    \Version    3.1        11/20/02 (JLB) Added Send() flags parameter
    \Version    3.2        02/18/03 (JLB) Fixes for multiple connection support
    \Version    3.3        05/06/03 (GWS) Allowed poke to come from any IP
    \Version    3.4        09/02/03 (JLB) Added unreliable packet type
    \Version    4.0        09/12/03 (JLB) Per-send optional unreliable transport
    \Version    5.0        07/07/09 (jrainy) Putting meta-data bits over the high bits of the sequence number
*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/comm/commall.h"
#include "DirtySDK/comm/commudp.h"
#include "DirtySDK/comm/commudputil.h"

/*** Defines ***************************************************************************/

#undef COMM_PRINT
#define COMM_PRINT      (0)

#if defined(DIRTYCODE_PC)
    #define ESC_CAUSES_LOSS (DIRTYCODE_DEBUG && 0)
#else
    #define ESC_CAUSES_LOSS (0)
#endif

#if ESC_CAUSES_LOSS
#include <windows.h>
#endif

#define BUSY_KEEPALIVE  (100)
#define IDLE_KEEPALIVE  (2500)
#define PENETRATE_RATE  (1000)
#define UNACK_LIMIT     (2048)

//! max additional space needed by a commudp meta type
#define COMMUDP_MAX_METALEN (8)

#define REDUNDANT_LIMIT_DEFAULT (64)

#define COMMUDP_VERSION_1_0     (0x0100)
#define COMMUDP_VERSION_1_1     (0x0101)
#define COMMUDP_VERSION         (COMMUDP_VERSION_1_1)

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! raw protocol packet format
typedef struct
{
    //! packet header which is not sent/received (this data is used internally)
    struct {
        int32_t iLen;                       //!< variable data len (-1=none) (used int32_t for alignment)
        uint32_t uWhen;                     //!< tick when a packet was received
        uint32_t uMeta;                     //!< four-bit metadata field extracted from seq field
    } head;
    //! packet body which is sent/received
    struct {
        uint32_t uSeq;                      //!< packet type or sequence number
        uint32_t uAck;                      //!< acknowledgement of last packet
        uint8_t  aData[SOCKET_MAXUDPRECV-8];//!< user data
    } body;
} RawUDPPacketT;

//! raw protocol packet header -- used for stack local formatting of handshake packets, and packets with no data
typedef struct
{
    //! packet header which is not sent/received (this data is used internally)
    struct {
        int32_t iLen;           //!< variable data len (-1=none) (used int32_t for alignment)
        uint32_t uWhen;         //!< tick when a packet was received
        uint32_t uMeta;         //!< four-bit metadata field extracted from seq field
    } head;
    //! packet body which is sent/received
    struct {
        uint32_t uSeq;          //!< packet type or seqeunce number
        uint32_t uAck;          //!< acknowledgement of last packet
        uint32_t uCid;          //!< client id (v1.0: in INIT/POKE packets; v1.1+: in INIT/POKE/CONN packets)
        uint8_t  aVers[2];      //!< protocol version (v1.1+ only)
        uint8_t  aData[62];     //!< space for possible metadata included in control packets
    } body;
} RawUDPPacketHeadT;

//! private module storage
struct CommUDPRef
{
    //! common header is first
    CommRef Common;

    //! max amount of unacknowledged data that can be sent in one go (default 2k)
    uint32_t uUnackLimit;

    //! max amount of data that can be sent redundantly (default = REDUNDANT_LIMIT_DEFAULT)
    uint32_t uRedundantLimit;

    //! linked list of all instances
    CommUDPRef *pLink;
    //! comm socket
    SocketT *pSocket;
    //! peer address
    struct sockaddr PeerAddr;

    //! port state
    enum {
        DEAD,       //!< dead
        IDLE,       //!< idle
        CONN,       //!< conn
        LIST,       //!< list
        OPEN,       //!< open
        CLOSE       //!< close
    } eState;

    //! identifier to keep from getting spoofed
    uint32_t uConnIdent;

    //! type of metachunk to include in stream (zero=none)
    uint32_t uMetaType;

    //! protocol version
    uint16_t uVers;
    uint16_t _pad;
    
    //! unique client identifier (used for game server identification)
    uint32_t uClientIdent;
    //! remote client identifier
    uint32_t uRemClientIdent;

    //! width of receive records (same as width of send)
    int32_t iRcvWid;
    //! length of receive buffer (multiple of rcvwid)
    int32_t iRcvLen;
    //! fifo input offset
    int32_t iRcvInp;
    //! fifo output offset
    int32_t iRcvOut;
    //! pointer to buffer storage
    char *pRcvBuf;
    //! next packet expected (sequence number)
    uint32_t uRcvSeq;
    //! next unreliable packet expected
    uint32_t uUnreliableRcvSeq;
    //! last packet we acknowledged
    uint32_t uRcvAck;
    //! number of unacknowledged received bytes
    int32_t iRcvUnack;

    //! width of send record (same as width of receive)
    int32_t iSndWid;
    //! length of send buffer (multiple of sndwid)
    int32_t iSndLen;
    //! fifo input offset
    int32_t iSndInp;
    //! fifo output offset
    int32_t iSndOut;
    //! current output point within fifo
    int32_t iSndNxt;
    //! pointer to buffer storage
    char *pSndBuf;
    //! next packet to send (sequence number)
    uint32_t uSndSeq;
    //! unreliable packet sequence number
    uint32_t uUnreliableSndSeq;
    //! last send result
    uint32_t uSndErr;

    //! tick at which last packet was sent
    uint32_t uSendTick;
    //! tick at which last reliable packet was sent (used for resend tracking)
    uint32_t uSendReliableTick;
    //! tick at which last packet was received
    uint32_t uRecvTick;
    //! tick at which last idle callback made
    uint32_t uIdleTick;

    //! control access during callbacks
    volatile int32_t iCallback;
    //! indicate there is an event pending
    uint32_t uGotEvent;
    //! callback routine pointer
    void (*pCallProc)(void *pRef, int32_t iEvent);
};

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

//! linked list of port objects
static CommUDPRef   *g_link = NULL;

//! semaphore to synchronize thread access
static NetCritT     g_crit;

//! missed event marker
static int32_t      g_missed;

//! variable indicates call to _CommUDPEvent() in progress
static int32_t      g_inevent;

#if DIRTYCODE_LOGGING
static const char  *g_strConnNames[] = { "COMMUDP_RAW_PACKET_INVALID", "COMMUDP_RAW_PACKET_INIT", "COMMUDP_RAW_PACKET_CONN", "COMMUDP_RAW_PACKET_DISC", "COMMUDP_RAW_PACKET_NAK", "COMMUDP_RAW_PACKET_POKE" };
#endif

// Public variables


/*** Private Functions *****************************************************************/

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPSeqnDelta

    \Description
        Compute the sequence number off of uPos by iDelta places
        Can NOT be used for unreliable sequence offsetting

    \Input uPos     - starting position
    \Input iDelta   - offset

    \Output
        uint32_t    - resulting position

    \Version 07/07/09 (jrainy)
*/
/*************************************************************************************************F*/
static uint32_t _CommUDPSeqnDelta(uint32_t uPos, int32_t iDelta)
{
    return(((uPos + iDelta + COMMUDP_RAW_PACKET_DATA_WINDOW - COMMUDP_RAW_PACKET_DATA) % COMMUDP_RAW_PACKET_DATA_WINDOW) + COMMUDP_RAW_PACKET_DATA);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPSeqnDiff

    \Description
        Compute the difference in position between two sequence numbers
        Can NOT be used to compute unreliable sequence differences

    \Input uPos1    - source position
    \Input uPos2    - target position

    \Output
        int32_t     - signed difference in position

    \Version 07/07/09 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _CommUDPSeqnDiff(uint32_t uPos1, uint32_t uPos2)
{
    return((((uPos1 - uPos2) + (3 * COMMUDP_RAW_PACKET_DATA_WINDOW / 2)) % COMMUDP_RAW_PACKET_DATA_WINDOW) - (COMMUDP_RAW_PACKET_DATA_WINDOW / 2));
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPSetAddrInfo

    \Description
        Sets peer/host addr/port info in common ref.

    \Input *pRef    - reference pointer
    \Input *pSin    - address pointer

    \Version 04/16/04 (JLB)
*/
/*************************************************************************************************F*/
static void _CommUDPSetAddrInfo(CommUDPRef *pRef, struct sockaddr *pSin)
{
    // save peer addr/port info in common ref
    pRef->Common.peerip = SockaddrInGetAddr(pSin);
    pRef->Common.peerport = SockaddrInGetPort(pSin);
    NetPrintf(("commudp: [%p] peer=%a:%d\n", pRef, pRef->Common.peerip, pRef->Common.peerport));
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPSetSocket

    \Description
        Sets socket in ref and socketref in common portion of ref.

    \Input *pRef    - reference pointer
    \Input *pSocket - socket to set

    \Version 08/24/04 (JLB)
*/
/*************************************************************************************************F*/
static void _CommUDPSetSocket(CommUDPRef *pRef, SocketT *pSocket)
{
    pRef->pSocket = pSocket;
    pRef->Common.sockptr = pSocket;
    if (pSocket != NULL)
    {
        struct sockaddr SockAddr;
        // save host addr/port info in common ref
        SocketInfo(pRef->pSocket, 'bind', 0, &SockAddr, sizeof(SockAddr));
        pRef->Common.hostip = SocketGetLocalAddr();
        pRef->Common.hostport = SockaddrInGetPort(&SockAddr);
        NetPrintf(("commudp: [%p] host=%a:%d\n", pRef, pRef->Common.hostip, pRef->Common.hostport));
    }
    else
    {
        pRef->Common.hostip = 0;
        pRef->Common.hostport = 0;
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPSetConnID

    \Description
        Sets connident to the 32bit hash of the specified connection identifier string, if any.

    \Input *pRef        - reference pointer
    \Input *pStrConn    - pointer to user-specified connection string

    \Version 06/16/04 (JLB)
*/
/*************************************************************************************************F*/
static void _CommUDPSetConnID(CommUDPRef *pRef, const char *pStrConn)
{
    const char *pConnID = strchr(pStrConn, '#');
    if (pConnID != NULL)
    {
        pRef->uConnIdent = NetHash(pConnID+1);
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPResetTransfer

    \Description
        Reset the transfer state

    \Input *pRef    - reference pointer

    \Version 12/04/00 (GWS)
*/
/*************************************************************************************************F*/
static void _CommUDPResetTransfer(CommUDPRef *pRef)
{
    // reset packet received bool
    pRef->Common.bpackrcvd = FALSE;
    // reset the send queue
    pRef->iSndInp = 0;
    pRef->iSndOut = 0;
    pRef->iSndNxt = 0;
    // reset the sequence number
    pRef->uSndSeq = COMMUDP_RAW_PACKET_DATA;
    pRef->uUnreliableSndSeq = COMMUDP_RAW_PACKET_UNREL;
    // reset the receive queue
    pRef->iRcvInp = 0;
    pRef->iRcvOut = 0;
    // reset the packet sequence number
    pRef->uRcvSeq = COMMUDP_RAW_PACKET_DATA;
    pRef->uUnreliableRcvSeq = COMMUDP_RAW_PACKET_UNREL;

    // no unack data
    pRef->iRcvUnack = 0;

    // make sendtick really old (in protocol terms)
    pRef->uSendTick = pRef->uSendReliableTick = NetTick()-5000;
    // recvtick must be older than tick because the first
    // packet that arrives may come in moments before this
    // initialization takes place and without this adjustment
    // code can compute an elapsed receive time of 0xffffffff]
    pRef->uRecvTick = NetTick()-5000;
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPOverhead

    \Description
        Computes the bandwidth overhead associated with a packet of length packetLength

    \Input *pRef        - module reference
    \Input iPktLen      - length of the packet we are about to send

    \Output
        int32_t         - the associated overhead. 28 on most platforms, but higher on xbox360.

    \Version 01/08/07 (JRainy)

*/
/*************************************************************************************************F*/
static int32_t _CommUDPOverhead(CommUDPRef *pRef, int32_t iPktLen)
{
    // start with basic IP+UDP header size
    int32_t iOverhead = 28;
    return(iOverhead);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPWrite

    \Description
        Send a packet to the peer

    \Input *pRef        - reference pointer
    \Input *pPacket     - packet pointer
    \Input *pPeerAddr   - address of peer to send to
    \Input uCurrTick    - current tick

    \Output
        int32_t         - negative=error, zero=ok

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
static int32_t _CommUDPWrite(CommUDPRef *pRef, RawUDPPacketT *pPacket, struct sockaddr *pPeerAddr, uint32_t uCurrTick)
{
    int32_t iErr;
    int32_t iLen;

    // figure full packet length (nak/ack fields + variable data)
    iLen = sizeof(pPacket->body)-sizeof(pPacket->body.aData)+pPacket->head.iLen;

    // fill in metatype info    
    if (pRef->uMetaType == 1)
    {
        int32_t iMetaOffset = ((pPacket->body.uSeq == COMMUDP_RAW_PACKET_INIT) || (pPacket->body.uSeq == COMMUDP_RAW_PACKET_POKE)) ? 4 : 0;
        // make room for metadata
        memmove(&pPacket->body.aData[COMMUDP_RAW_METATYPE1_SIZE+iMetaOffset], &pPacket->body.aData[0+iMetaOffset], iLen-iMetaOffset);
        iLen += COMMUDP_RAW_METATYPE1_SIZE;
        // metatype1 src clientident
        pPacket->body.aData[0+iMetaOffset] = (uint8_t)(pRef->uClientIdent >> 24);
        pPacket->body.aData[1+iMetaOffset] = (uint8_t)(pRef->uClientIdent >> 16);
        pPacket->body.aData[2+iMetaOffset] = (uint8_t)(pRef->uClientIdent >> 8);
        pPacket->body.aData[3+iMetaOffset] = (uint8_t)(pRef->uClientIdent);
        // metatype1 dst clientident
        pPacket->body.aData[4+iMetaOffset] = (uint8_t)(pRef->uRemClientIdent >> 24);
        pPacket->body.aData[5+iMetaOffset] = (uint8_t)(pRef->uRemClientIdent >> 16);
        pPacket->body.aData[6+iMetaOffset] = (uint8_t)(pRef->uRemClientIdent >> 8);
        pPacket->body.aData[7+iMetaOffset] = (uint8_t)(pRef->uRemClientIdent);
        // set metatype header
        pPacket->body.uSeq |= (pRef->uMetaType & 0xf) << COMMUDP_SEQ_META_SHIFT;
    }

    #if COMM_PRINT > 1
    NetPrintf(("commudp: [%p] seq:0x%08x ack:0x%08x  send %d bytes to %a:%d\n", pRef, pPacket->body.uSeq, pPacket->body.uAck, iLen, SockaddrInGetAddr(pPeerAddr), SockaddrInGetPort(pPeerAddr)));
    #endif
    #if COMM_PRINT > 2
    NetPrintMem(&pPacket->body, iLen, "cudp-send");
    #endif

    // translate seq and ack to network order for send
    pPacket->body.uSeq = SocketHtonl(pPacket->body.uSeq);
    pPacket->body.uAck = SocketHtonl(pPacket->body.uAck);

    // store send time in misc field of sockaddr
    SockaddrInSetMisc(pPeerAddr, uCurrTick);

    #if ESC_CAUSES_LOSS
    // lose packets when escape is pressed
    if (GetAsyncKeyState(VK_ESCAPE) < 0)
    {
        NetPrintf(("commudp: [%p] dropping packet to simulate packet loss (seq=0x%08x)\n", pRef, SocketNtohl(pPacket->body.uSeq)));
        iErr = iLen;
    }
    else
    {
        // send the packet
        iErr = SocketSendto(pRef->pSocket, (char *)&pPacket->body, iLen, 0, pPeerAddr, sizeof(*pPeerAddr));
    }
    #else
    // send the packet
    iErr = SocketSendto(pRef->pSocket, (char *)&pPacket->body, iLen, 0, pPeerAddr, sizeof(*pPeerAddr));
    #endif


    // translate seq and ack back to host order
    pPacket->body.uSeq = SocketNtohl(pPacket->body.uSeq);
    pPacket->body.uAck = SocketNtohl(pPacket->body.uAck);

    // check for success
    if (iErr == iLen)
    {
        // update last send time
        pRef->uSendTick = uCurrTick;

        // do stats
        if (pRef->eState == OPEN)
        {
            pRef->Common.datasent += iLen;
            pRef->Common.packsent += 1;
        }

        pRef->Common.overhead += _CommUDPOverhead(pRef, iLen);

        // is the packet reliable?
        if ((pPacket->body.uSeq & COMMUDP_SEQ_MASK) >= COMMUDP_RAW_PACKET_DATA)
        {
            // we assume any reliable send includes an up to date ack value
            // which means that we can reset the unacked data count to zero
            pRef->iRcvUnack = 0;
            // update reliable send timer
            pRef->uSendReliableTick = uCurrTick;
        }
    }
    else
    {
        NetPrintf(("commudp: [%p] SocketSendto() returned %d\n", pRef, iErr));
        pRef->uSndErr = iErr;
        iErr = -1;
    }

    return(iErr);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPClose

    \Description
        Close the connection

    \Input *pRef    - reference pointer
    \Input uCurrTick- current tick

    \Output
        int32_t     - negative=error, zero=ok

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
static int32_t _CommUDPClose(CommUDPRef *pRef, uint32_t uCurrTick)
{
    RawUDPPacketHeadT Packet;

    // if we're open, shut down connection
    if (pRef->eState == OPEN)
    {
        // see if any output data pending
        if (pRef->iSndNxt != pRef->iSndInp)
        {
            NetPrintf(("commudp: [%p] unsent data pending\n", pRef));
        }
        else if (pRef->iSndOut != pRef->iSndInp)
        {
            NetPrintf(("commudp: [%p] unacked data pending\n", pRef));
        }

        // send a disconnect message
        Packet.head.iLen = 0;
        Packet.body.uSeq = COMMUDP_RAW_PACKET_DISC;
        Packet.body.uAck = pRef->uConnIdent;
        _CommUDPWrite(pRef, (RawUDPPacketT *)&Packet, &pRef->PeerAddr, uCurrTick);
    }

    // set to disconnect state
    NetPrintf(("commudp: [%p] closed connection\n", pRef));
    pRef->uConnIdent = 0;
    pRef->eState = CLOSE;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPFormatHandshake

    \Description
        Format handshake packet

    \Input *pRef    - reference pointer
    \Input *pPacket - [out] buffer to format handshake packet into
    \Input uKind    - packet kind (COMMUDP_RAW_PACKET_INIT, COMMUDP_RAW_PACKET_POKE, COMMUDP_RAW_PACKET_CONN)

    \Version 10/29/2015 (jbrookes)
*/
/*************************************************************************************************F*/
static void _CommUDPFormatHandshake(CommUDPRef *pRef, RawUDPPacketHeadT *pPacket, uint32_t uKind)
{
    pPacket->body.uSeq = uKind;
    pPacket->body.uAck = pRef->uConnIdent;
    #if COMMUDP_VERSION > COMMUDP_VERSION_1_0
    pPacket->body.uCid = SocketHtonl(pRef->uClientIdent);
    pPacket->body.aVers[0] = COMMUDP_VERSION>>8;
    pPacket->body.aVers[1] = COMMUDP_VERSION&0x0f;
    pPacket->head.iLen = 6;
    #else
    if ((kind == COMMUDP_RAW_PACKET_INIT) || (kind == COMMUDP_RAW_PACKET_POKE))
    {
        pPacket->body.cid = SocketHtonl(pRef->clientident);
        pPacket->head.iLen = 4;
    }
    else
    {
        pPacket->head.iLen = 0;
    }
    #endif
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPProcessSetup

    \Description
        Process a setup/teardown request

    \Input *pRef    - reference pointer
    \Input *pPacket - requesting packet
    \Input *pSin    - address
    \Input uCurrTick- current tick

    \Version 12/04/00 (GWS)
*/
/*************************************************************************************************F*/
static void _CommUDPProcessSetup(CommUDPRef *pRef, const RawUDPPacketT *pPacket, struct sockaddr *pSin, uint32_t uCurrTick)
{
    // make sure the session identifier matches
    if (pPacket->body.uAck != pRef->uConnIdent)
    {
        NetPrintf(("commudp: [%p] warning - connident mismatch (expected=0x%08x got=0x%08x)\n", pRef, pRef->uConnIdent, pPacket->body.uAck));
        // an init packet with a different session identifier
        // indicates that the old session has closed
        if (pPacket->body.uSeq == COMMUDP_RAW_PACKET_INIT)
        {
            pRef->eState = CLOSE;
        }
        return;
    }
    
    // update valid receive time -- must put into past to avoid race condition
    pRef->uRecvTick = uCurrTick-1000;

    // version identification
    #if COMMUDP_VERSION > COMMUDP_VERSION_1_0
    if ((pRef->uVers == 0) && ((pPacket->body.uSeq == COMMUDP_RAW_PACKET_INIT) || (pPacket->body.uSeq == COMMUDP_RAW_PACKET_CONN) || (pPacket->body.uSeq == COMMUDP_RAW_PACKET_POKE)))
    {
        RawUDPPacketHeadT *hshk = (RawUDPPacketHeadT *)pPacket;
        pRef->uVers = (pPacket->head.iLen > 4) ? (hshk->body.aVers[0] << 8) | hshk->body.aVers[1] : COMMUDP_VERSION_1_0;
        NetPrintf(("commudp: [%p] vers=%d.%d\n", pRef, pRef->uVers >> 8, pRef->uVers & 0xff));
    }
    #else
    pRef->uVers = COMMUDP_VERSION_1_0;
    #endif

    // response to connection/poke query
    if ((pPacket->body.uSeq == COMMUDP_RAW_PACKET_INIT) || (pPacket->body.uSeq == COMMUDP_RAW_PACKET_POKE))
    {
        RawUDPPacketHeadT connpacket;
        
        // set host/peer addr/port info
        _CommUDPSetAddrInfo(pRef, pSin);

        // send CONN in response to INIT/POKE
        NetPrintf(("commudp: [%p] sending CONN packet to %a:%d connident=0x%08x\n", pRef, SockaddrInGetAddr(&pRef->PeerAddr),
            SockaddrInGetPort(&pRef->PeerAddr), pRef->uConnIdent));
        _CommUDPFormatHandshake(pRef, &connpacket, COMMUDP_RAW_PACKET_CONN);
        _CommUDPWrite(pRef, (RawUDPPacketT *)&connpacket, &pRef->PeerAddr, uCurrTick);
        return;
    }

    // response to a connect confirmation
    if (pPacket->body.uSeq == COMMUDP_RAW_PACKET_CONN)
    {
        // change to open if not already there
        if (pRef->eState == CONN)
        {
            // set host/peer addr/port info
            _CommUDPSetAddrInfo(pRef, pSin);
            NetPrintf(("commudp: [%p] transitioning to OPEN state due to received COMMUDP_RAW_PACKET_CONN\n", pRef));
            pRef->eState = OPEN;
        }
        return;
    }

    // response to disconnect message
    if (pPacket->body.uSeq == COMMUDP_RAW_PACKET_DISC)
    {
        // close the connection
        if (pRef->eState == OPEN)
        {
            pRef->eState = CLOSE;
        }
        NetPrintf(("commudp: [%p] received DISC packet\n", pRef));
    }

    // should not get here
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPProcessInit

    \Description
        Initiate a connection

    \Input *pRef    - reference pointer
    \Input uCurrTick- current tick

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
static void _CommUDPProcessInit(CommUDPRef *pRef, uint32_t uCurrTick)
{
    RawUDPPacketHeadT Packet;

    // send INIT to peer
    NetPrintf(("commudp: [%p] sending INIT packet to %a:%d connident=0x%08x clientident=0x%08x\n", pRef, SockaddrInGetAddr(&pRef->PeerAddr),
        SockaddrInGetPort(&pRef->PeerAddr), pRef->uConnIdent, pRef->uClientIdent));
    _CommUDPFormatHandshake(pRef, &Packet, COMMUDP_RAW_PACKET_INIT);
    _CommUDPWrite(pRef, (RawUDPPacketT *)&Packet, &pRef->PeerAddr, uCurrTick);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPProcessOutput

    \Description
        Send data packet(s)

    \Input *pRef    - reference pointer
    \Input uCurrTick- current tick

    \Version 12/04/00 (GWS)
*/
/*************************************************************************************************F*/
static void _CommUDPProcessOutput(CommUDPRef *pRef, uint32_t uCurrTick)
{
    int32_t iIndex, iCount;
    int32_t iTLimit = pRef->uUnackLimit, iPLimit;
    static RawUDPPacketT Multi;
    RawUDPPacketT *pBuffer;
    static uint32_t uMLimit = 0x20000000;
    int32_t iMetaLen = CommUDPUtilGetMetaSize(pRef->uMetaType);
    int32_t iSubpacketLimit = (pRef->uVers > COMMUDP_VERSION_1_0) ? 1530 : 250;

    // figure unacked data length
    for (iIndex = pRef->iSndOut; iIndex != pRef->iSndNxt; iIndex = (iIndex+pRef->iSndWid)%pRef->iSndLen) {
        pBuffer = (RawUDPPacketT *) &pRef->pSndBuf[iIndex];
        // count down the limit
        iTLimit -= pBuffer->head.iLen;
    }

    // allow a minimum send rate (256 bytes per 250 ms)
    if ((iTLimit < 256) && (uCurrTick-pRef->uSendTick > 250))
        iTLimit = 256;

    // send as much data as limit allows
    while (iTLimit > 0) {

        // limit size of forward packet consolidation
        iPLimit = SocketInfo(NULL, 'maxp', 0, NULL, 0) - iMetaLen - sizeof(Multi.body) + sizeof(Multi.body.aData);
        
        if (iPLimit > iTLimit)
            iPLimit = iTLimit;

        // attempt forward consolidation of packets
        for (iCount = 0; (iCount < 8) && (iPLimit > 0) && (pRef->iSndNxt != pRef->iSndInp); ++iCount) {
            pBuffer = (RawUDPPacketT *) &pRef->pSndBuf[pRef->iSndNxt];
            iPLimit -= pBuffer->head.iLen;
            iPLimit -= CommUDPUtilEncodeSubpacketSize(NULL, pBuffer->head.iLen);
            // if not the first packet, then we must be careful about size
            if ((iCount > 0) && (iPLimit <= 0))
                break;
            pRef->iSndNxt = (pRef->iSndNxt+pRef->iSndWid) % pRef->iSndLen;
            // if packet is too large for subpacket encoding, it must be final packet (first in multisend)
            if (pBuffer->head.iLen > iSubpacketLimit) {
                ++iCount;
                break;
            }
        }
        if (iCount == 0)
            return;

        // setup main packet
        iIndex = (pRef->iSndNxt+pRef->iSndLen-pRef->iSndWid)%pRef->iSndLen;
        pBuffer = (RawUDPPacketT *) &pRef->pSndBuf[iIndex];
        // if they want a callback, do it now
        if (pRef->Common.SendCallback != NULL)
            pRef->Common.SendCallback((CommRef *)pRef, pBuffer->body.aData, pBuffer->head.iLen, uCurrTick);
        ds_memcpy(&Multi, pBuffer, sizeof(pBuffer->head)+8+pBuffer->head.iLen);
        iCount -= 1;

        // add in required preceding packets
        for (; iCount > 0; --iCount) {
            // move to preceding packet
            iIndex = (iIndex+pRef->iSndLen-pRef->iSndWid)%pRef->iSndLen;
            // point to potential piggyback packet
            pBuffer = (RawUDPPacketT *) &pRef->pSndBuf[iIndex];
            // if they want a callback, do it now
            if (pRef->Common.SendCallback != NULL)
                pRef->Common.SendCallback((CommRef *)pRef, pBuffer->body.aData, pBuffer->head.iLen, uCurrTick);
            // combine the packets
            Multi.body.uSeq += COMMUDP_SEQ_MULTI_INC;
            ds_memcpy(Multi.body.aData+Multi.head.iLen, pBuffer->body.aData, pBuffer->head.iLen);
            Multi.head.iLen += pBuffer->head.iLen;
            Multi.head.iLen += CommUDPUtilEncodeSubpacketSize(Multi.body.aData+Multi.head.iLen, pBuffer->head.iLen);
        }

        // add in optional redundant packets
        while ((iIndex != pRef->iSndOut) && (Multi.body.uSeq <= uMLimit)) {
            // move to preceding packet
            iIndex = (iIndex+pRef->iSndLen-pRef->iSndWid)%pRef->iSndLen;
            // point to potential piggyback packet
            pBuffer = (RawUDPPacketT *) &pRef->pSndBuf[iIndex];
            // if packet is too large for subpacket encoding, we cannot multi-send it
            if (pBuffer->head.iLen > iSubpacketLimit)
                break;
            // see if combined length would be a problem
            if ((Multi.head.iLen + pBuffer->head.iLen + 1) > (signed)pRef->uRedundantLimit)
                break;
            // if they want a callback, do it now
            if (pRef->Common.SendCallback != NULL)
                pRef->Common.SendCallback((CommRef *)pRef, pBuffer->body.aData, pBuffer->head.iLen, uCurrTick);
            // combine the packets
            Multi.body.uSeq += COMMUDP_SEQ_MULTI_INC;
            ds_memcpy(Multi.body.aData+Multi.head.iLen, pBuffer->body.aData, pBuffer->head.iLen);
            Multi.head.iLen += pBuffer->head.iLen;
            Multi.head.iLen += CommUDPUtilEncodeSubpacketSize(Multi.body.aData+Multi.head.iLen, pBuffer->head.iLen);
        }

        // adjust the max redundancy
        if (iIndex == pRef->iSndOut) {
            uMLimit = 0x20000000;
        } else {
            uMLimit = (uMLimit < 0x80000000 ? uMLimit*2 : 0xf0000000);
        }

        // update the ack value (one less than one we are waiting for)
        pRef->uRcvAck = pRef->uRcvSeq;
        Multi.body.uAck = _CommUDPSeqnDelta(pRef->uRcvSeq, -1);
        // go ahead and send the packet
        if (_CommUDPWrite(pRef, &Multi, &pRef->PeerAddr, uCurrTick) < 0)
        {
            return;
        }
        // count the bandwidth for this packet
        iTLimit -= Multi.head.iLen;
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPProcessFlow

    \Description
        Perform flow control based on ack/nak packets

    \Input *pRef    - reference pointer
    \Input *pPacket - incoming packet header
    \Input uCurrTick- current tick

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
static void _CommUDPProcessFlow(CommUDPRef *pRef, RawUDPPacketHeadT *pPacket, uint32_t uCurrTick)
{
    int32_t iNak;
    uint32_t uAck;

    // grab the ack point and nak flag
    iNak = (pPacket->body.uSeq == COMMUDP_RAW_PACKET_NAK);

    if (pPacket->body.uAck < COMMUDP_RAW_PACKET_DATA)
    {
        uAck = (iNak ? pPacket->body.uAck-1 : pPacket->body.uAck);
    }
    else
    {
        uAck = (iNak ? _CommUDPSeqnDelta(pPacket->body.uAck, -1) : pPacket->body.uAck);
    }

    // advance ack point
    while (pRef->iSndOut != pRef->iSndInp)
    {
        RawUDPPacketT *pBuffer = (RawUDPPacketT *) &pRef->pSndBuf[pRef->iSndOut];
        // see if this packet has been acked
        if (((uAck & COMMUDP_SEQ_MASK) < COMMUDP_RAW_PACKET_DATA) ||
            (_CommUDPSeqnDiff(uAck, pBuffer->body.uSeq) < 0))
        {
            break;
        }

        // if about to send this packet, skip to next
        if (pRef->iSndNxt == pRef->iSndOut)
            pRef->iSndNxt = (pRef->iSndNxt+pRef->iSndWid) % pRef->iSndLen;
        // remove the packet from the queue
        pRef->iSndOut = (pRef->iSndOut+pRef->iSndWid) % pRef->iSndLen;
    }

    // reset send point for nak
    if (iNak)
    {
        #if COMM_PRINT
        NetPrintf(("commudp: [%p] got NAK for packet %d\n", pRef, uAck+1));
        #endif
        // reset send point
        pRef->iSndNxt = pRef->iSndOut;
        // immediate restart
        _CommUDPProcessOutput(pRef, uCurrTick);
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPProcessInput

    \Description
        Process incoming data packet

    \Input *pRef    - module state
    \Input *pPacket - incoming packet header
    \Input *pData   - pointer to packet data
    \Input uCurrTick- current network tick

    \Output
        int32_t     - -1=nak, 0=old, 1=new, 2=buffer full

    \Version 12/04/2000 (gschaefer)
*/
/*************************************************************************************************F*/
static int32_t _CommUDPProcessInput(CommUDPRef *pRef, RawUDPPacketHeadT *pPacket, uint8_t *pData, uint32_t uCurrTick)
{
    RawUDPPacketT *pBuffer;

    // see if room in buffer for packet
    if ((pRef->iRcvInp+pRef->iRcvWid)%pRef->iRcvLen == pRef->iRcvOut)
    {
        // no room in buffer -- just drop packet
        // could nak, but would generate lots of
        // network activity with no result
        NetPrintf(("commudp: [%p] input buffer overflow\n", pRef));
        return(2);
    }

    // handle unreliable receive
    if (((pPacket->body.uSeq & COMMUDP_SEQ_MASK) >= COMMUDP_RAW_PACKET_UNREL) && ((pPacket->body.uSeq & COMMUDP_SEQ_MASK) < COMMUDP_RAW_PACKET_DATA))
    {
        // calculate the number of free packets in pRcvBuf
        int32_t queuepos = ((pRef->iRcvInp+pRef->iRcvLen)-pRef->iRcvOut)%pRef->iRcvLen;
        int32_t pktsfree = (pRef->iRcvLen - queuepos)/pRef->iRcvWid;

        // calculate delta between received sequence and expected sequence, accounting for wrapping
        int32_t delta = (pPacket->body.uSeq - pRef->uUnreliableRcvSeq) & (COMMUDP_RAW_PACKET_UNREL - 1);

        // update lost packet count
        if ((pPacket->body.uSeq > pRef->uUnreliableRcvSeq) || (delta < COMMUDP_RAW_PACKET_UNREL/4))
        {
            pRef->Common.packlost += delta;
        }

        // calculate new sequence number
        if ((pRef->uUnreliableRcvSeq = (pPacket->body.uSeq + 1)) >= COMMUDP_RAW_PACKET_DATA)
        {
            pRef->uUnreliableRcvSeq = COMMUDP_RAW_PACKET_UNREL + (pRef->uUnreliableRcvSeq - COMMUDP_RAW_PACKET_DATA);
        }

        // see if there is room to buffer (leave room for reliable packets)
        if (pktsfree <= 4)
        {
            return(2);
        }
    }
    else
    {
        // ignore old packets
        if (_CommUDPSeqnDiff(pPacket->body.uSeq, pRef->uRcvSeq) < 0)
        {
            return(0);
        }

        // immediate nak for missing packets
        if (_CommUDPSeqnDiff(pPacket->body.uSeq, pRef->uRcvSeq) > 0)
        {
            // update lost packet count
            pRef->Common.packlost += _CommUDPSeqnDiff(pPacket->body.uSeq, pRef->uRcvSeq);
            
            // send a nak packet
            #if COMM_PRINT
            NetPrintf(("commudp: [%p] sending a NAK of packet %d (tick=%u)\n", pRef, pRef->uRcvSeq, NetTick()));
            #endif
            pPacket->body.uSeq = COMMUDP_RAW_PACKET_NAK;
            pPacket->body.uAck = pRef->uRcvSeq;
            pPacket->head.iLen = 0;
            _CommUDPWrite(pRef, (RawUDPPacketT *)pPacket, &pRef->PeerAddr, uCurrTick);

            // update number of NAKs sent
            pRef->Common.naksent += 1;

            return(-1);
        }

        // no further processing for empty (ack) packets
        if (pPacket->head.iLen == 0)
        {
            return(0);
        }
    }

    // add the packet to the buffer
    pBuffer = (RawUDPPacketT *) &pRef->pRcvBuf[pRef->iRcvInp];
    // copy the packet
    ds_memcpy_s(pBuffer, sizeof(*pBuffer), pPacket, sizeof(*pPacket));
    ds_memcpy(pBuffer->body.aData, pData, pPacket->head.iLen);

    // limit receive access for callbacks
    pRef->iCallback += 1;
    // add item to receive buffer
    pRef->iRcvInp = (pRef->iRcvInp+pRef->iRcvWid) % pRef->iRcvLen;

    // reliable specific processing
    if ((pPacket->body.uSeq & COMMUDP_SEQ_MASK) >= COMMUDP_RAW_PACKET_DATA)
    {
        pRef->uRcvSeq = _CommUDPSeqnDelta(pRef->uRcvSeq, 1);
        // add to unacknowledged byte count
        pRef->iRcvUnack += pBuffer->head.iLen;
    }

    // indicate we got an event
    pRef->uGotEvent |= 1;
    // let the callback process it
    if (pRef->Common.RecvCallback != NULL)
        pRef->Common.RecvCallback((CommRef *)pRef, pBuffer->body.aData, pBuffer->head.iLen, pBuffer->head.uWhen);
    // release access to receive
    pRef->iCallback -= 1;
    return(1);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPProcessPoke

    \Description
        Penetrate firewall with poke packet

    \Input *pRef    - reference pointer
    \Input uCurrTick- current tick

    \Version 07/07/00 (GWS)

*/
/*************************************************************************************************F*/
static void _CommUDPProcessPoke(CommUDPRef *pRef, uint32_t uCurrTick)
{
    RawUDPPacketHeadT Packet;

    // send POKE to peer
    NetPrintf(("commudp: [%p] sending POKE packet to %a:%d connident=0x%08x clientident=0x%08x\n", pRef, SockaddrInGetAddr(&pRef->PeerAddr),
        SockaddrInGetPort(&pRef->PeerAddr), pRef->uConnIdent, pRef->uClientIdent));
    _CommUDPFormatHandshake(pRef, &Packet, COMMUDP_RAW_PACKET_POKE);
    _CommUDPWrite(pRef, (RawUDPPacketT *)&Packet, &pRef->PeerAddr, uCurrTick);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPProcessAlive

    \Description
        Send a keepalive packet

    \Input *pRef    - reference pointer
    \Input uCurrTick- current tick

    \Version 12/04/00 (GWS)
*/
/*************************************************************************************************F*/
static void _CommUDPProcessAlive(CommUDPRef *pRef, uint32_t uCurrTick)
{
    RawUDPPacketHeadT Packet;

    // see if we should resend most recent packet
    if ((pRef->iSndOut != pRef->iSndInp) && (pRef->iSndNxt == pRef->iSndInp)) {
        // this shound result in a multi-send
        pRef->iSndNxt = (pRef->iSndNxt+pRef->iSndLen-pRef->iSndWid)%pRef->iSndLen;
        _CommUDPProcessOutput(pRef, uCurrTick);
        return;
    }

    // set our packet number
    Packet.body.uSeq = pRef->uSndSeq;
    // acknowledge packet prior to one we are waiting for
    pRef->uRcvAck = pRef->uRcvSeq;
    Packet.body.uAck = _CommUDPSeqnDelta(pRef->uRcvSeq, -1);

    // no data means keepalive
    Packet.head.iLen = 0;
    // send it
    #if COMM_PRINT
    NetPrintf(("commudp: [%p] sending keep-alive\n", pRef));
    #endif
    _CommUDPWrite(pRef, (RawUDPPacketT *)&Packet, &pRef->PeerAddr, uCurrTick);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPFlush

    \Description
        Flush output buffer

    \Input *pRef    - reference pointer
    \Input uLimit   - timeout in milliseconds
    \Input uCurrTick- current tick

    \Output
        uint32_t    - number of packets in buffer

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
static uint32_t _CommUDPFlush(CommUDPRef *pRef, uint32_t uLimit, uint32_t uCurrTick)
{
    int32_t iNumPackets;
    int32_t iIndex;
    RawUDPPacketT *pBuffer;

    iNumPackets = (((pRef->iSndInp+pRef->iSndLen)-pRef->iSndOut)%pRef->iSndLen)/pRef->iSndWid;
    NetPrintf(("commudp: [%p] flushing %d packets in send queue\n", pRef, iNumPackets));

    for (iIndex = pRef->iSndOut; iIndex != pRef->iSndNxt; iIndex = (iIndex+pRef->iSndWid)%pRef->iSndLen)
    {
        pBuffer = (RawUDPPacketT *) &pRef->pSndBuf[iIndex];
        _CommUDPWrite(pRef, pBuffer, &pRef->PeerAddr, uCurrTick);
    }

    return(iNumPackets);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPThreadData

    \Description
        Take care of data processing

    \Input uCurrTick- current tick

    \Output
        int32_t     - number of packets received

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
static int32_t _CommUDPThreadData(uint32_t uCurrTick)
{
    int32_t iLen;
    int32_t iCount = 0;
    CommUDPRef *pRef;
    CommUDPRef *pListen = NULL;
    struct sockaddr Sin;
    static RawUDPPacketT Packet;
    SocketT *pSocket;
    uint32_t bConnIdentMatch=0;
    uint32_t uLClientIdent=0, uRClientIdent=0;

    // init sockaddr (else unused bytes cause mismatch during ident)
    SockaddrInit(&Sin, AF_INET);

    // scan sockets for incoming packet
    Packet.head.iLen = -1;
    pSocket = NULL;
    for (pRef = g_link; pRef != NULL; pRef = pRef->pLink)
    {
        // make sure we need to scan this one
        if ((pRef->pSocket != NULL) && (pRef->pSocket != pSocket))
        {
            // see if data is pending
            pSocket = pRef->pSocket;
            iLen = sizeof(Sin);
            iLen = SocketRecvfrom(pSocket, (char *)&Packet.body, sizeof(Packet.body), 0, &Sin, &iLen);
            if (iLen > 0)
            {
                // track recieve overhead
                pRef->Common.rcvoverhead += sizeof(Packet.body)-sizeof(Packet.body.aData) + _CommUDPOverhead(pRef, iLen);
                pRef->Common.rcvoverhead += CommUDPUtilGetMetaSize(pRef->uMetaType);
                // get length and timestamp the packet
                Packet.head.iLen = iLen-(sizeof(Packet.body)-sizeof(Packet.body.aData));
                Packet.head.uWhen = SockaddrInGetMisc(&Sin);
                // translate seq and ack to host order
                Packet.body.uSeq = SocketNtohl(Packet.body.uSeq);
                Packet.body.uAck = SocketNtohl(Packet.body.uAck);
                // extract and clear meta bits
                Packet.head.uMeta = CommUDPUtilGetMetaTypeFromSeq(Packet.body.uSeq);
                Packet.body.uSeq &= ~(0x0f << COMMUDP_SEQ_META_SHIFT);

                #if COMM_PRINT > 1
                NetPrintf(("commudp: [%p] seq:0x%08x ack:0x%08x recv %d bytes from %a:%d (head.iLen=%d, meta=%d) at tick=%d\n",
                    pRef, Packet.body.uSeq, Packet.body.uAck, iLen, SockaddrInGetAddr(&Sin), SockaddrInGetPort(&Sin), Packet.head.iLen, Packet.head.uMeta, Packet.head.uWhen));
                #endif
                #if COMM_PRINT > 2
                NetPrintMem(&Packet.body, iLen, "cudp-recv");
                #endif
                
                if (Packet.body.uSeq == COMMUDP_RAW_PACKET_CONN || (Packet.body.uSeq == COMMUDP_RAW_PACKET_INIT) || (Packet.body.uSeq == COMMUDP_RAW_PACKET_POKE))
                {
                    NetPrintf(("commudp: [%p] got %s connident=0x%08x len=%d\n", pRef, g_strConnNames[Packet.body.uSeq], Packet.body.uAck, Packet.head.iLen));
                }
                // count the packet
                ++iCount;

                break;
            }
        }
    }

    // if we have meta chunk use that for matching with our connection ref
    if ((Packet.head.iLen >= COMMUDP_RAW_METATYPE1_SIZE) && (Packet.head.uMeta == 1))
    {
        int32_t iMetaOffset = ((Packet.body.uSeq == COMMUDP_RAW_PACKET_INIT) || (Packet.body.uSeq == COMMUDP_RAW_PACKET_POKE)) ? 4 : 0;
        
        // read meta type one chunk data
        uRClientIdent = ((uint32_t)Packet.body.aData[0+iMetaOffset])<<24 | ((uint32_t)Packet.body.aData[1+iMetaOffset])<<16 | ((uint32_t)Packet.body.aData[2+iMetaOffset])<<8 | (uint32_t)Packet.body.aData[3+iMetaOffset];
        uLClientIdent = ((uint32_t)Packet.body.aData[4+iMetaOffset])<<24 | ((uint32_t)Packet.body.aData[5+iMetaOffset])<<16 | ((uint32_t)Packet.body.aData[6+iMetaOffset])<<8 | (uint32_t)Packet.body.aData[7+iMetaOffset];
        
        // remove metadata from packet
        Packet.head.iLen -= COMMUDP_RAW_METATYPE1_SIZE;
        if ((Packet.head.iLen-iMetaOffset) > 0)
        {
            memmove(&Packet.body.aData[0+iMetaOffset], &Packet.body.aData[COMMUDP_RAW_METATYPE1_SIZE+iMetaOffset], Packet.head.iLen-iMetaOffset);
        }
        #if COMM_PRINT > 2
        NetPrintf(("commudp: [%p] processed metadata chunk\n", pRef));
        #endif
    }

    // walk port list and handle processing
    for (pRef = g_link; pRef != NULL; pRef = pRef->pLink)
    {
        // get latest tick
        uCurrTick = NetTick();

        // identify port to handle connection request
        if ((pListen == NULL) && (pSocket == pRef->pSocket) && (pRef->eState == LIST) && (Packet.head.iLen >= 0) &&
            ((Packet.body.uSeq == COMMUDP_RAW_PACKET_INIT) || (Packet.body.uSeq == COMMUDP_RAW_PACKET_CONN)) && (pRef->uConnIdent == Packet.body.uAck))
        {
            pListen = pRef;
        }

        if (Packet.head.iLen >= 0)
        {
            if (Packet.head.uMeta == 1)
            {
                bConnIdentMatch = (uRClientIdent == pRef->uRemClientIdent) && (uLClientIdent == pRef->uClientIdent);
                #if COMM_PRINT > 2
                NetPrintf(("commudp: [%p] matching with metadata: src=0x%08x(0x%08x) dst=0x%08x(0x%08x) match=%d\n", pRef,
                    uRClientIdent, pRef->uRemClientIdent, uLClientIdent, pRef->uClientIdent, bConnIdentMatch));
                #endif
            }
            else
            {
                // if this is an INIT or POKE, make sure the connident matches
                bConnIdentMatch = ((Packet.body.uSeq == COMMUDP_RAW_PACKET_INIT) || (Packet.body.uSeq == COMMUDP_RAW_PACKET_POKE)) ? (Packet.body.uAck == pRef->uConnIdent) : 1;
            }
        }

        // see if packet belongs to someone
        if ((Packet.head.iLen >= 0) && (pRef->eState != LIST) && (pRef->eState != CLOSE) && (pSocket == pRef->pSocket) &&
            (SockaddrCompare(&pRef->PeerAddr, &Sin) == 0) && (bConnIdentMatch))
        {
            // we got a packet.
            pRef->Common.bpackrcvd = TRUE;

            // transition to open state if we're getting data from peer
            if ((pRef->eState == CONN) && ((Packet.body.uSeq == COMMUDP_RAW_PACKET_UNREL) || (Packet.body.uSeq == COMMUDP_RAW_PACKET_DATA)))
            {
                NetPrintf(("commudp: [%p] transitioning to OPEN state due to received data from peer\n", pRef));
                pRef->eState = OPEN;
            }

            // do stats
            if (pRef->eState == OPEN)
            {
                pRef->Common.datarcvd += Packet.head.iLen;
                pRef->Common.packrcvd += 1;
            }

            // process an incoming packet
            if ((Packet.body.uSeq == COMMUDP_RAW_PACKET_INIT) ||
                (Packet.body.uSeq == COMMUDP_RAW_PACKET_POKE) ||
                (Packet.body.uSeq == COMMUDP_RAW_PACKET_CONN) ||
                (Packet.body.uSeq == COMMUDP_RAW_PACKET_DISC))
            {
                // handle connection setup/teardown
                _CommUDPProcessSetup(pRef, &Packet, &Sin, uCurrTick);
            }
            else if (pRef->eState != OPEN)
            {
                // ignore the packet
                continue;
            }
            else if (Packet.body.uSeq == COMMUDP_RAW_PACKET_NAK)
            {
                // remember we got something
                pRef->uRecvTick = Packet.head.uWhen;
                // resend the missing data
                _CommUDPProcessFlow(pRef, (RawUDPPacketHeadT *)&Packet, uCurrTick);
            }
            else if (Packet.body.uSeq > COMMUDP_SEQ_MULTI_INC)
            {
                RawUDPPacketHeadT Multi;
                uint32_t uOldSeq = pRef->uRcvSeq;
                int32_t iExtraSubPktCount = CommUDPUtilGetExtraSubPktCountFromSeq(Packet.body.uSeq);
                Multi.head.uWhen = Packet.head.uWhen;
                Multi.body.uSeq = _CommUDPSeqnDelta((Packet.body.uSeq & COMMUDP_SEQ_MASK), -iExtraSubPktCount);
                Multi.body.uAck = Packet.body.uAck;
                // remember we got something
                pRef->uRecvTick = Packet.head.uWhen;
                // process all the packets
                for (; iExtraSubPktCount >= 0; --iExtraSubPktCount)
                {
                    if (iExtraSubPktCount > 0)
                    {
                        Packet.head.iLen -= CommUDPUtilDecodeSubpacketSize(Packet.body.aData+Packet.head.iLen-1, &Multi.head.iLen);
                    }
                    else
                    {
                        Multi.head.iLen = Packet.head.iLen;
                    }
                    Packet.head.iLen -= Multi.head.iLen;

                    if (Packet.head.iLen < 0)
                    {
                        // we just received corrupt data, bail out
                        break;
                    }

                    // process the ack info
                    _CommUDPProcessFlow(pRef, &Multi, uCurrTick);
                    // process the input data and check for missing data (nak)
                    // (if nak, stop processing so we only send one nak packet)
                    if (_CommUDPProcessInput(pRef, &Multi, Packet.body.aData+ Packet.head.iLen, uCurrTick) < 0)
                    {
                        iExtraSubPktCount = 0;
                    }
                    // see if packets were saved
                    if ((iExtraSubPktCount > 0) && (uOldSeq != pRef->uRcvSeq))
                    {
                        pRef->Common.packsaved += 1;
                        #if COMM_PRINT
                        NetPrintf(("commudp: [%p] redundant packet %d prevented loss of packet %d\n", pRef, Packet.body.uSeq & COMMUDP_SEQ_MASK, uOldSeq));
                        #endif
                        uOldSeq = pRef->uRcvSeq;
                    }
                    // advance the packet sequence number

                    // if we sent a nak, multi.body.seq got wiped and count2 set to 0, so don't increment it.
                    if (Multi.body.uSeq >= COMMUDP_RAW_PACKET_UNREL)
                    {
                        Multi.body.uSeq = _CommUDPSeqnDelta(Multi.body.uSeq, 1);
                    }
                }
            }
            else
            {
                // remember we got something
                pRef->uRecvTick = Packet.head.uWhen;
                // process the ack info
                _CommUDPProcessFlow(pRef, (RawUDPPacketHeadT *)&Packet, uCurrTick);
                // save the data
                _CommUDPProcessInput(pRef, (RawUDPPacketHeadT *)&Packet, Packet.body.aData, uCurrTick);
            }
            // mark packet as processed
            Packet.head.iLen = -1;
        }

        // see if we are trying to connect
        if ((pRef->eState == CONN) && (uCurrTick-pRef->uSendTick > 1000))
        {
            _CommUDPProcessInit(pRef, uCurrTick);
        }

        // see if any output for this port
        if ((pRef->eState == OPEN) && (pRef->iSndNxt != pRef->iSndInp))
        {
            _CommUDPProcessOutput(pRef, uCurrTick);
        }

        // check for connection timeout
        if ((pRef->eState == OPEN) && (NetTickDiff(uCurrTick, pRef->uRecvTick) > 120*1000) && (NetTickDiff(uCurrTick, pRef->uSendTick) < 2000))
        {
            NetPrintf(("commudp: [%p] closing connection due to timeout\n", pRef));
            NetPrintf(("commudp: [%p] tick=%d, rtick=%d, stick=%d\n", pRef, uCurrTick, pRef->uRecvTick, pRef->uSendTick));
            _CommUDPClose(pRef, uCurrTick);
        }

        // see if we should run the penetrator
        if ((pRef->eState == LIST) && (pRef->PeerAddr.sa_family == AF_INET) &&
            (uCurrTick > pRef->uSendTick+PENETRATE_RATE))
        {
            // penetrate the firewall
            _CommUDPProcessPoke(pRef, uCurrTick);
        }

        // see if callback needs an idle tick
        if ((pRef->uGotEvent == 0) && (uCurrTick > pRef->uIdleTick+250))
        {
            pRef->uIdleTick = uCurrTick;
            pRef->uGotEvent |= 4;
        }

        // do callback if needed
        if ((pRef->iCallback == 0) && (pRef->uGotEvent != 0))
        {
            // limit callback access
            pRef->iCallback += 1;
            // callback the handler
            if (pRef->pCallProc != NULL)
            {
                pRef->pCallProc((CommRef *)pRef, pRef->uGotEvent);
            }
            else if (pRef->eState == OPEN)
            {
                #if COMM_PRINT
                NetPrintf(("commudp: [%p] no upper layer callback\n", pRef));
                #endif
            }
            // limit callback access
            pRef->iCallback -= 1;
            // reset event count
            pRef->uGotEvent = 0;
        }

        // see if we need a keepalive
        // do this after callback since callback often generates a
        // packet send that eliminates the need for the idle packet
        if ((pRef->eState == OPEN) && (pRef->iSndNxt == pRef->iSndInp))
        {
            int32_t timeout = NetTickDiff(uCurrTick, pRef->uSendTick);
            int32_t reliabletimeout = NetTickDiff(uCurrTick, pRef->uSendReliableTick);
            if (((reliabletimeout > BUSY_KEEPALIVE) && (pRef->uRcvAck != pRef->uRcvSeq)) ||
                ((reliabletimeout > BUSY_KEEPALIVE) && (pRef->iSndInp != pRef->iSndOut)) ||
                 (timeout > IDLE_KEEPALIVE) ||
                 (pRef->iRcvUnack >= UNACK_LIMIT))
            {
                // force reset of unack count just in case
                pRef->iRcvUnack = 0;
                // send a keepalive
                _CommUDPProcessAlive(pRef, uCurrTick);
            }
        }
    }

    // check for unclaimed connection packet (port mangling)
    if ((Packet.head.iLen >= 0) && (Sin.sa_family == AF_INET) &&
        ((Packet.body.uSeq == COMMUDP_RAW_PACKET_POKE) ||
         (Packet.body.uSeq == COMMUDP_RAW_PACKET_INIT) ||
         (Packet.body.uSeq == COMMUDP_RAW_PACKET_CONN)))
    {
        uint32_t bIgnored = FALSE;
        NetPrintf(("commudp: [%p] received %s packet from %a:%d\n", pRef, g_strConnNames[Packet.body.uSeq], SockaddrInGetAddr(&Sin), SockaddrInGetPort(&Sin)));
        
        // look for matching reference
        for (pRef = g_link; pRef != NULL; pRef = pRef->pLink)
        {
            if (((pRef->eState == CONN) || (pRef->eState == LIST)) && (pRef->PeerAddr.sa_family == AF_INET))
            {
                if (pRef->uConnIdent == Packet.body.uAck)
                {
                    // see if they are trying to connect to poke source but port is mangled
                    if ((pSocket == pRef->pSocket) && (SockaddrInGetAddr(&pRef->PeerAddr) == SockaddrInGetAddr(&Sin)) && (SockaddrInGetPort(&pRef->PeerAddr) != SockaddrInGetPort(&Sin)))
                    {
                        // assume poke source is masq, change port number to correspond
                        NetPrintf(("commudp: [%p] changing peer to %a:%d (was expecting %a:%d)\n", pRef, SockaddrInGetAddr(&Sin), SockaddrInGetPort(&Sin),
                            SockaddrInGetAddr(&pRef->PeerAddr), SockaddrInGetPort(&pRef->PeerAddr)));
                        ds_memcpy_s(&pRef->PeerAddr, sizeof(pRef->PeerAddr), &Sin, sizeof(Sin));
                    }
                    break;
                }
                else // remember we ignored a packet with non-matching connident
                {
                    bIgnored = TRUE;
                }
            }
        }
        if ((pRef == NULL) && (bIgnored == TRUE))
        {
            NetPrintf(("commudp: [%p] ignoring %s packet with connident 0x%08x\n", pRef, g_strConnNames[Packet.body.uSeq], Packet.body.uAck));
        }
    }

    // see if this was an initial connection request
    if ((pListen != NULL) && (Packet.head.iLen >= 0) && ((Packet.body.uSeq == COMMUDP_RAW_PACKET_INIT) || (Packet.body.uSeq == COMMUDP_RAW_PACKET_CONN)))
    {
        int32_t iPeerAddr = SockaddrInGetAddr(&pListen->PeerAddr);
        pRef = pListen;
        
        if ((pRef->uConnIdent == Packet.body.uAck) && ((iPeerAddr == 0) || (iPeerAddr == SockaddrInGetAddr(&Sin))))
        {
            NetPrintf(("commudp: [%p] transitioning to CONN state after receiving %s packet\n", pRef, g_strConnNames[Packet.body.uSeq]));
            // change to connecting state
            pRef->eState = CONN;
            // if we were listening without a peer address, set it now
            if (iPeerAddr == 0)
            {
                ds_memcpy_s(&pRef->PeerAddr, sizeof(pRef->PeerAddr), &Sin, sizeof(Sin));
            }
            // process init packet
            _CommUDPProcessSetup(pRef, &Packet, &Sin, uCurrTick);
        }
    }

    return(iCount);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPEvent

    \Description
        Main event function

    \Input *pSock   - socket
    \Input iFlags    - flags
    \Input *_ref    - to be completed

    \Output
        int32_t     - 0

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
static int32_t _CommUDPEvent(SocketT *pSock, int32_t iFlags, void *_ref)
{
    // remember we're in an event call
    g_inevent = 1;

    // see if we have exclusive access
    if (NetCritTry(&g_crit))
    {
        uint32_t uCurrTick = NetTick();
        // clear event counter before calling _CommUDPProcessThreadData to prevent possible unwanted recursion
        g_missed = 0;
        // process data as long as we get something
        while (_CommUDPThreadData(uCurrTick) > 0)
            ;
        // free access
        NetCritLeave(&g_crit);
    }
    else
    {
        g_missed += 1;
        #if COMM_PRINT
        NetPrintf(("commudp: missed %d events\n", g_missed));
        #endif
    }

    // leaving event call
    g_inevent = 0;

    // done for now
    return(0);
}


/*F*************************************************************************************************/
/*!
    \Function    _CommUDPEnlistRef

    \Description
        Add the given ref to the global linked list of references.

    \Input *pRef    - ref to add

    \Notes
        This also handles initialization of the global critical section global
        missed event counter.

    \Version 02/18/03 (JLB)
*/
/*************************************************************************************************F*/
static void _CommUDPEnlistRef(CommUDPRef *pRef)
{
    // if first ref, init global critical section
    if (g_link == NULL)
    {
        NetCritInit(&g_crit, "commudp-global");
        g_missed = 0;
        g_inevent = 0;
    }

    // add to linked list of ports
    NetCritEnter(&g_crit);
    pRef->pLink = g_link;
    g_link = pRef;
    NetCritLeave(&g_crit);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPDelistRef

    \Description
        Remove the given ref from the global linked list of references.    

    \Input *pRef    - ref to remove

    \Notes
        This also handles destruction of the global critical section.

    \Version 02/18/03 (JLB)
*/
/*************************************************************************************************F*/
static void _CommUDPDelistRef(CommUDPRef *pRef)
{
    CommUDPRef **ppLink;

    // remove from linked list of ports
    NetCritEnter(&g_crit);
    for (ppLink = &(g_link); *ppLink != pRef; ppLink = &((*ppLink)->pLink))
        ;
    *ppLink = pRef->pLink;
    NetCritLeave(&g_crit);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPListen0

    \Description
        Listen for a connection (private version)

    \Input *pRef        - reference pointer
    \Input *pSock       - fallback socket
    \Input *pBindAddr   - local port

    \Output
        int32_t         - negative=error, zero=ok

    \Version 12/04/00 (GWS)
*/
/*************************************************************************************************F*/
static int32_t _CommUDPListen0(CommUDPRef *pRef, SocketT *pSock, struct sockaddr *pBindAddr)
{
    int32_t iErr;
    CommUDPRef *pFind;
    struct sockaddr GlueAddr;

    // make sure in valid state
    if (pRef->eState != IDLE)
    {
        SocketClose(pSock);
        return(COMM_BADSTATE);
    }

    // reset to unresolved
    _CommUDPSetSocket(pRef, NULL);
    _CommUDPResetTransfer(pRef);

    // setup bind points
    ds_memclr(&GlueAddr, sizeof(GlueAddr));
    ds_memclr(&pRef->PeerAddr, sizeof(pRef->PeerAddr));

#if defined(DIRTYCODE_PC)
    /*
    the following line will fill in bindaddr with the IP addr previously set with
    SocketControl(... 'ladr' ...). If that selector was never used, then the IP
    address field of bindaddr will just be filled with 0.
    note: only required for multi-homed system (PC)
    */
    SocketInfo(NULL, 'ladr', 0, pBindAddr, sizeof(*pBindAddr));
#endif

    // see if there is an existing socket bound to this port
    for (pFind = g_link; pFind != NULL; pFind = pFind->pLink)
    {
        // dont check ourselves or unbound sockets
        if ((pFind == pRef) || (pFind->pSocket == NULL))
            continue;

        // see where this endpoint is bound
        if (SocketInfo(pFind->pSocket, 'bind', 0, &GlueAddr, sizeof(GlueAddr)) < 0)
            continue;

        /*
        see if the socket can be reused
            if the endpoint is virtual: we only compare the ports
            if the endpoint is not virtual:
                if bindaddr (specifying what we want to bind to) has ipaddr=0, we only compare the ports
                otherwise we compare the full sockaddr structs (i.e. family, port, addr)
        */
        if (SockaddrInGetPort(pBindAddr) == SockaddrInGetPort(&GlueAddr))
        {
            if ((SocketInfo(pFind->pSocket, 'virt', 0, NULL, 0) == TRUE) ||
                (SockaddrInGetAddr(pBindAddr) == 0) ||
                (SockaddrCompare(pBindAddr, &GlueAddr) == 0))
            {
                // share the socket
                _CommUDPSetSocket(pRef, pFind->pSocket);

                // dont need supplied socket
                SocketClose(pSock);
                break;
            }
        }
    }

    // create socket if no existing one
    if (pFind == NULL)
    {
        // bind the address to the socket
        iErr = SocketBind(pSock, pBindAddr, sizeof(*pBindAddr));
        if (iErr < 0)
        {
            NetPrintf(("commudp: [%p] bind to %d failed with %d\n", pRef, SockaddrInGetPort(pBindAddr), iErr));
            SockaddrInSetPort(pBindAddr, 0);
            if ((iErr = SocketBind(pSock, pBindAddr, sizeof(*pBindAddr))) < 0)
            {
                NetPrintf(("commudp: [%p] bind to 0 failed with result %d\n", pRef, iErr));
                pRef->eState = DEAD;
                SocketClose(pSock);
                return((iErr == SOCKERR_INVALID) ? COMM_PORTBOUND : COMM_UNEXPECTED);
            }
        }
        // use the supplied socket
        _CommUDPSetSocket(pRef, pSock);

        // setup for socket events
        SocketCallback(pSock, CALLB_RECV, 100, NULL, &_CommUDPEvent);
    }

    // put into listen mode
    pRef->eState = LIST;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPConnect0

    \Description
        Initiate a connection to a peer (private version)

    \Input *pRef        - reference pointer
    \Input *pSock       - socket
    \Input *pPeerAddr   - peer address

    \Output
        int32_t         - negative=error, zero=ok

    \Notes
        Does not currently perform dns translation

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
static int32_t _CommUDPConnect0(CommUDPRef *pRef, SocketT *pSock, struct sockaddr *pPeerAddr)
{
    // make sure in valid state
    if (pRef->eState != IDLE) {
        SocketClose(pSock);
        return(COMM_BADSTATE);
    }

    // reset to unresolved
    _CommUDPSetSocket(pRef, NULL);
    _CommUDPResetTransfer(pRef);

    // setup target info
    ds_memcpy_s(&pRef->PeerAddr, sizeof(pRef->PeerAddr), pPeerAddr, sizeof(*pPeerAddr));

    // save the socket
    _CommUDPSetSocket(pRef, pSock);

    // setup for callbacks
    SocketCallback(pSock, CALLB_RECV, 100, NULL, &_CommUDPEvent);

    // change the state
    pRef->eState = CONN;
    return(0);
}

/*** Public Functions ******************************************************************/

/*F*************************************************************************************************/
/*!
    \Function    CommUDPConstruct

    \Description
        Construct the class

    \Input iMaxWid  - max record width
    \Input iMaxInp  - input packet buffer size
    \Input iMaxOut  - output packet buffer size

    \Output
        CommUDPRef  - construct pointer

    \Notes
        Initialized winsock for first class. also creates linked
        list of all current instances of the class and worker thread
        to do most udp stuff.

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
CommUDPRef *CommUDPConstruct(int32_t iMaxWid, int32_t iMaxInp, int32_t iMaxOut)
{
    CommUDPRef *pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate class storage
    pRef = DirtyMemAlloc(sizeof(*pRef), COMMUDP_MEMID, iMemGroup, pMemGroupUserData);
    if (pRef == NULL)
        return(NULL);
    ds_memclr(pRef, sizeof(*pRef));
    pRef->Common.memgroup = iMemGroup;
    pRef->Common.memgrpusrdata = pMemGroupUserData;

    // initialize the callback routines
    pRef->Common.Construct = (CommAllConstructT *)CommUDPConstruct;
    pRef->Common.Destroy = (CommAllDestroyT *)CommUDPDestroy;
    pRef->Common.Resolve = (CommAllResolveT *)CommUDPResolve;
    pRef->Common.Unresolve = (CommAllUnresolveT *)CommUDPUnresolve;
    pRef->Common.Listen = (CommAllListenT *)CommUDPListen;
    pRef->Common.Unlisten = (CommAllUnlistenT *)CommUDPUnlisten;
    pRef->Common.Connect = (CommAllConnectT *)CommUDPConnect;
    pRef->Common.Unconnect = (CommAllUnconnectT *)CommUDPUnconnect;
    pRef->Common.Callback = (CommAllCallbackT *)CommUDPCallback;
    pRef->Common.Control = (CommAllControlT *)CommUDPControl;
    pRef->Common.Status = (CommAllStatusT *)CommUDPStatus;
    pRef->Common.Tick = (CommAllTickT *)CommUDPTick;
    pRef->Common.Send = (CommAllSendT *)CommUDPSend;
    pRef->Common.Peek = (CommAllPeekT *)CommUDPPeek;
    pRef->Common.Recv = (CommAllRecvT *)CommUDPRecv;

    // remember max packet width
    pRef->Common.maxwid = iMaxWid;
    pRef->Common.maxinp = iMaxInp;
    pRef->Common.maxout = iMaxOut;

    // allocate the buffers
    pRef->iRcvWid = sizeof(RawUDPPacketT)-sizeof(((RawUDPPacketT *)0)->body.aData)+iMaxWid+COMMUDP_MAX_METALEN;
    pRef->iRcvWid = (pRef->iRcvWid +3) & 0x7ffc;
    pRef->iRcvLen = pRef->iRcvWid * iMaxInp;
    pRef->pRcvBuf = (char *)DirtyMemAlloc(pRef->iRcvLen, COMMUDP_MEMID, pRef->Common.memgroup, pRef->Common.memgrpusrdata);
    pRef->iSndWid = sizeof(RawUDPPacketT)-sizeof(((RawUDPPacketT *)0)->body.aData)+iMaxWid+COMMUDP_MAX_METALEN;
    pRef->iSndWid = (pRef->iSndWid+3) & 0x7ffc;
    pRef->iSndLen = pRef->iSndWid * iMaxOut;
    pRef->pSndBuf = (char *)DirtyMemAlloc(pRef->iSndLen, COMMUDP_MEMID, pRef->Common.memgroup, pRef->Common.memgrpusrdata);

    // reset the socket
    _CommUDPSetSocket(pRef, NULL);
    
    // reset peer address
    ds_memclr(&pRef->PeerAddr, sizeof(pRef->PeerAddr));

    // reset the state
    pRef->eState = IDLE;
    pRef->uConnIdent = 0;
    pRef->uUnackLimit = UNACK_LIMIT;
    pRef->uRedundantLimit = REDUNDANT_LIMIT_DEFAULT;

    // add to port list
    _CommUDPEnlistRef(pRef);

    return(pRef);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPDestroy

    \Description
        Destruct the class

    \Input *pRef    - reference pointer

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
void CommUDPDestroy(CommUDPRef *pRef)
{
    CommUDPRef *pFind;
    uint32_t uCurrTick = NetTick();

    // flush final data
    _CommUDPFlush(pRef, 200, uCurrTick);

    // remove from port list
    _CommUDPDelistRef(pRef);

    // if port is open, close it
    if (pRef->eState == OPEN)
        _CommUDPClose(pRef, uCurrTick);

    // kill the socket
    if (pRef->pSocket != NULL) {
        // see if socket shared
        for (pFind = g_link; pFind != NULL; pFind = pFind->pLink) {
            if (pFind->pSocket == pRef->pSocket)
                break;
        }
        // if we are only user of this socket
        if (pFind == NULL) {
            SocketClose(pRef->pSocket);
            _CommUDPSetSocket(pRef, NULL);
        }
    }

    // if last ref, destroy global critical section
    if (g_link == NULL)
    {
        NetCritKill(&g_crit);
    }

    // release resources
    DirtyMemFree(pRef->pRcvBuf, COMMUDP_MEMID, pRef->Common.memgroup, pRef->Common.memgrpusrdata);
    DirtyMemFree(pRef->pSndBuf, COMMUDP_MEMID, pRef->Common.memgroup, pRef->Common.memgrpusrdata);
    DirtyMemFree(pRef, COMMUDP_MEMID, pRef->Common.memgroup, pRef->Common.memgrpusrdata);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPCallback

    \Description
        Set upper layer callback

    \Input *pRef        - reference pointer
    \Input *pCallback    - socket generating callback

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
void CommUDPCallback(CommUDPRef *pRef, void (*pCallback)(void *pRef, int32_t iEvent))
{
    NetCritEnter(&g_crit);
    pRef->pCallProc = pCallback;
    pRef->uGotEvent |= 2;
    NetCritLeave(&g_crit);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPResolve

    \Description
        Resolve an address

    \Input *pRef    - endpoint
    \Input *pAddr   - resolve address
    \Input *pBuf    - target buffer
    \Input iLen     - target length (min 64 bytes)
    \Input cDiv     - divider char

    \Output
        int32_t         - <0=error, 0=complete (COMM_NOERROR), >0=in progress (COMM_PENDING)

    \Notes
        Target list is always double null terminated allowing null
        to be used as the divider character if desired. when COMM_PENDING
        is returned, target buffer is set to "~" until completion.

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
int32_t CommUDPResolve(CommUDPRef *pRef, const char *pAddr, char *pBuf, int32_t iLen, char cDiv)
{
    NetPrintf(("commudp: [%p] resolve functionality not supported\n", pRef));
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPUnresolve

    \Description
        Stop the resolver

    \Input *pRef    - reference pointer

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
void CommUDPUnresolve(CommUDPRef *pRef)
{
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPUnlisten

    \Description
        Stop listening

    \Input *pRef    - reference pointer

    \Output
        int32_t     - negative=error, zero=ok

    \Version 12/04/00 (GWS)
*/
/*************************************************************************************************F*/
int32_t CommUDPUnlisten(CommUDPRef *pRef)
{
    uint32_t uCurrTick = NetTick();

    // flush final data
    _CommUDPFlush(pRef, 200, uCurrTick);

    // never close listening socket (it is shared)
    if (pRef->eState == LIST)
    {
        _CommUDPSetSocket(pRef, NULL);
    }

    // get rid of socket if presernt
    if (pRef->pSocket != NULL)
    {
        // attempt to close socket
        _CommUDPClose(pRef, uCurrTick);
        // done with socket
        SocketClose(pRef->pSocket);
        _CommUDPSetSocket(pRef, NULL);
    }

    // return to idle mode
    pRef->eState = IDLE;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPUnconnect

    \Description
        Terminate a connection

    \Input *pRef    - reference pointer

    \Output
        int32_t     - negative=error, zero=ok


    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
int32_t CommUDPUnconnect(CommUDPRef *pRef)
{
    uint32_t uCurrTick = NetTick();

    // flush final data
    _CommUDPFlush(pRef, 200, uCurrTick);

    // never close listening socket (it is shared)
    if (pRef->eState == LIST)
    {
        _CommUDPSetSocket(pRef, NULL);
    }

    // get rid of socket if presernt
    if (pRef->pSocket != NULL)
    {
        // attempt to close socket
        _CommUDPClose(pRef, uCurrTick);
        // done with socket
        SocketClose(pRef->pSocket);
        _CommUDPSetSocket(pRef, NULL);
    }

    // return to idle mode
    pRef->eState = IDLE;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPStatus

    \Description
        Return current stream status

    \Input *pRef    - reference pointer

    \Output
        int32_t     - CONNECTING, OFFLINE, ONLINE or FAILURE

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
int32_t CommUDPStatus(CommUDPRef *pRef)
{
    // return state
    if ((pRef->eState == CONN) || (pRef->eState == LIST))
        return(COMM_CONNECTING);
    if ((pRef->eState == IDLE) || (pRef->eState == CLOSE))
        return(COMM_OFFLINE);
    if (pRef->eState == OPEN)
        return(COMM_ONLINE);
    return(COMM_FAILURE);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPControl

    \Description
        Set connection behavior.

    \Input *pRef     - reference pointer
    \Input iControl - control selector
    \Input iValue   - selector specfic
    \Input *pValue  - selector specific

    \Output
        int32_t     - negative=error, else selector result

    \Notes
        iControl can be one of the following:

        \verbatim
            'clid' - client identifier
            'meta' - set metatype (default=0)
            'rcid' - remote client identifier
            'rlmt' - redundant packet size limit (default = REDUNDANT_LIMIT_DEFAULT)
            'ulmt' - unacknowledged packet limit (2k default)
        \endverbatim

    \Version 02/20/2007 (jbrookes)
*/
/*************************************************************************************************F*/
int32_t CommUDPControl(CommUDPRef *pRef, int32_t iControl, int32_t iValue, void *pValue)
{
    if (iControl == 'clid')
    {
        pRef->uClientIdent = iValue;
        return(0);
    }
    if (iControl == 'meta')
    {
        NetPrintf(("commudp: [%p] setting metatype=%d\n", pRef, iValue));
        pRef->uMetaType = iValue;
        return(0);
    }
    if (iControl == 'rcid')
    {
        pRef->uRemClientIdent = iValue;
        return(0);
    }
    if (iControl == 'rlmt')
    {
        const int32_t iMaxLimit = sizeof(((RawUDPPacketT *)0)->body.aData);
        if (iValue == 0)
        {
            iValue = REDUNDANT_LIMIT_DEFAULT;
        }
        if (iValue >= iMaxLimit)
        {
            iValue = iMaxLimit;
        }
        NetPrintf(("commudp: [%p] redundant limit changed from %d bytes to %d bytes\n", pRef, pRef->uRedundantLimit, iValue));
        pRef->uRedundantLimit = iValue;
        return(0);
    }
    if (iControl == 'ulmt')
    {
        pRef->uUnackLimit = iValue;
        NetPrintf(("commudp: [%p] setting ulimit to %d bytes\n", pRef, pRef->uUnackLimit));
        return(0);
    }
    // unhandled; pass through to socket module if we have a socket
    if (pRef->pSocket != NULL)
    {
        return(SocketControl(pRef->pSocket, iControl, iValue, pValue, NULL));
    }
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPTick

    \Description
        Return current tick

    \Input *pRef    - reference pointer

    \Output
        uint32_t    - elaped milliseconds

    \Version 12/04/00 (GWS)
*/
/*************************************************************************************************F*/
uint32_t CommUDPTick(CommUDPRef *pRef)
{
    return(NetTick());
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPSend

    \Description
        Send a packet

    \Input *pRef    - reference pointer
    \Input *pBuffer - pointer to data
    \Input iLength  - length of data
    \Input uFlags   - COMM_FLAGS_*

    \Output
        int32_t     - negative=error, zero=buffer full (temp fail), positive=queue position (ok)

    \Notes
        Zero length packets may not be sent (they are used for buffer query)

    \Version 12/04/00 (GWS)
*/
/*************************************************************************************************F*/
int32_t CommUDPSend(CommUDPRef *pRef, const void *pBuffer, int32_t iLength, uint32_t uFlags)
{
    RawUDPPacketT PacketBuffer, *pPacket;
    uint32_t uCurrTick = NetTick();
    int32_t iPos, iMetaLen, iResult=1; // result=1 is default (min queue) result

    // make sure port is open
    if (pRef->eState != OPEN)
    {
        return(COMM_BADSTATE);
    }

    // if metachunk enabled, adjust size 
    iMetaLen = CommUDPUtilGetMetaSize(pRef->uMetaType);

    // return error for oversized packets
    if ((iLength+iMetaLen) > (signed)(pRef->iSndWid-(sizeof(RawUDPPacketT)-sizeof(((RawUDPPacketT *)0)->body.aData))))
    {
        NetPrintf(("commudp: [%p] oversized packet send (%d bytes)\n", pRef, iLength));
        return(COMM_MINBUFFER);
    }

    /* check for zero-length packets, which cannot be sent (they are used for acks); instead, treat them as successful,
       which means the queue position is returned */
    if (iLength == 0)
    {
        iPos = (((pRef->iSndInp+pRef->iSndLen)-pRef->iSndOut)%pRef->iSndLen)/pRef->iSndWid;
        return(iPos+1);
    }

    // get packet buffer
    if (uFlags & COMM_FLAGS_UNRELIABLE)
    {
        // unreliable packets are staged locally and flushed immediately
        pPacket = &PacketBuffer;
    }
    else
    {
        // make sure output buffer isn't full
        if ((pRef->iSndInp + pRef->iSndWid) % pRef->iSndLen == pRef->iSndOut)
        {
            NetPrintfVerbose((COMM_PRINT, 0, "commudp: [%p] send overflow (connident=0x%08x)\n", pRef, pRef->uConnIdent));
            return(0);
        }
        // reference packet buffer in output queue
        pPacket = (RawUDPPacketT *) &(pRef->pSndBuf[pRef->iSndInp]);
    }

    // copy the packet to the buffer
    ds_memcpy(pPacket->body.aData, pBuffer, iLength);
    pPacket->head.iLen = iLength;
    // set the send time
    pPacket->head.uWhen = uCurrTick;

    // handle unreliable send
    if (uFlags & COMM_FLAGS_UNRELIABLE)
    {
        int32_t iErr = -1;
        NetCritEnter(&g_crit);
        
        // set up and send an unreliable packet
        pPacket->body.uSeq = pRef->uUnreliableSndSeq;
        pPacket->body.uAck = _CommUDPSeqnDelta(pRef->uRcvSeq, -1);
        if (uFlags & COMM_FLAGS_BROADCAST)
        {
            struct sockaddr PeerAddr;
            ds_memcpy_s(&PeerAddr, sizeof(PeerAddr), &pRef->PeerAddr, sizeof(pRef->PeerAddr));
            SockaddrInSetAddr(&PeerAddr, 0xffffffff);
            iErr = _CommUDPWrite(pRef, pPacket, &PeerAddr, uCurrTick);
        }
        else
        {
            iErr = _CommUDPWrite(pRef, pPacket, &pRef->PeerAddr, uCurrTick);
        }

        // calculate new sequence number
        if (++pRef->uUnreliableSndSeq >= COMMUDP_RAW_PACKET_DATA)
        {
            pRef->uUnreliableSndSeq = COMMUDP_RAW_PACKET_UNREL;
        }
        
        NetCritLeave(&g_crit);
        // if send failed, return buffer-full(0) or error
        if (iErr < 0)
        {
            iResult = (pRef->uSndErr == SOCKERR_NONE) ? 0 : iErr;
        }
    }
    else
    {
        // set the data fields
        pPacket->body.uSeq = pRef->uSndSeq;
        pRef->uSndSeq = _CommUDPSeqnDelta(pRef->uSndSeq, 1);

        pPacket->body.uAck = _CommUDPSeqnDelta(pRef->uRcvSeq, -1);

        // add the packet to the queue
        pRef->iSndInp = (pRef->iSndInp+pRef->iSndWid) % pRef->iSndLen;
        iPos = (((pRef->iSndInp+pRef->iSndLen)-pRef->iSndOut)%pRef->iSndLen)/pRef->iSndWid;

        // try to send packet immediately if buffer is at least half empty
        if (iPos < (pRef->Common.maxout/2))
        {
            NetCritEnter(&g_crit);
            _CommUDPProcessOutput(pRef, uCurrTick);

            // process incoming if we missed event
            if (g_missed != 0)
            {
                // clear event counter before calling _CommUDPProcessThreadData to prevent possible unwanted recursion
                g_missed = 0;
                // process data as long as we get something
                while (_CommUDPThreadData(uCurrTick) > 0)
                    ;
                NetPrintfVerbose((COMM_PRINT, 0, "commudp: [%p] processing %d after send\n", pRef, g_missed));
            }
            NetCritLeave(&g_crit);
        }
        // return buffer depth
        if (iPos > 0)
        {
            iResult = iPos;
        }
    }

    return(iResult);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPPeek

    \Description
        Peek at waiting packet

    \Input *pRef    - reference pointer
    \Input *pTarget - target buffer
    \Input iLength  - buffer length
    \Input *pWhen   - tick received at

    \Output
        int32_t         - negative=nothing pending, else packet length

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
int32_t CommUDPPeek(CommUDPRef *pRef, void *pTarget, int32_t iLength, uint32_t *pWhen)
{
    RawUDPPacketT *pPacket;

    // see if a packet is available
    if (pRef->iRcvOut == pRef->iRcvInp)
        return(COMM_NODATA);

    // point to the packet
    pPacket = (RawUDPPacketT *) &(pRef->pRcvBuf[pRef->iRcvOut]);

    // copy data?
    if (iLength > 0)
    {
        // make sure enough space is available
        if (iLength < pPacket->head.iLen)
            return(COMM_MINBUFFER);
       
        // copy over the data portion
        ds_memcpy(pTarget, pPacket->body.aData, pPacket->head.iLen);
    }
        
    // get the timestamp
    if (pWhen != NULL)
        *pWhen = pPacket->head.uWhen;
    
    // return packet data length
    return(pPacket->head.iLen);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPRecv

    \Description
        Receive a packet from the buffer

    \Input *pRef    - reference pointer
    \Input *pTarget - target buffer
    \Input iLength  - buffer length
    \Input *pWhen   - tick received at

    \Output
        int32_t     - negative=error, else packet length

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
int32_t CommUDPRecv(CommUDPRef *pRef, void *pTarget, int32_t iLength, uint32_t *pWhen)
{
    // use peek to remove the data
    int32_t iLen = CommUDPPeek(pRef, pTarget, iLength, pWhen);
    if (iLen >= 0)
        pRef->iRcvOut = (pRef->iRcvOut+pRef->iRcvWid)%pRef->iRcvLen;
    // all done
    return(iLen);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPListen

    \Description
        Listen for a connection

    \Input *pRef    - reference pointer
    \Input *pAddr   - port to listen on (only :port portion used)

    \Output
        int32_t     - negative=error, zero=ok

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
int32_t CommUDPListen(CommUDPRef *pRef, const char *pAddr)
{
    int32_t iErr, iListenPort, iConnPort;
    uint32_t uPoke;
    SocketT *pSock;
    struct sockaddr BindAddr;

    // setup bind points
    SockaddrInit(&BindAddr, AF_INET);

    // parse at least port
    if ((SockaddrInParse2(&uPoke, &iListenPort, &iConnPort, pAddr) & 0x2) != 0x2)
    {
        return(COMM_BADADDRESS);
    }
    SockaddrInSetPort(&BindAddr, iListenPort);

    // create socket in case its needed
    pSock = SocketOpen(AF_INET, SOCK_DGRAM, 0);
    if (pSock == NULL)
    {
        return(COMM_NORESOURCE);
    }

    // let common code finish up
    iErr = _CommUDPListen0(pRef, pSock, &BindAddr);

    // set connection identifier
    _CommUDPSetConnID(pRef, pAddr);

    NetPrintf(("commudp: [%p] listen err=%d, bind=%d, connident=0x%08x\n", pRef, iErr, iListenPort, pRef->uConnIdent));

    // see if we should setup peer address
    if ((iErr == 0) && (uPoke != 0))
    {
        if (iConnPort == 0)
        {
            iConnPort = iListenPort+1;
        }

        NetPrintf(("commudp: [%p] poke address=%08x:%d\n", pRef, uPoke, iConnPort));
        SockaddrInit(&pRef->PeerAddr, AF_INET);
        SockaddrInSetAddr(&pRef->PeerAddr, uPoke);
        SockaddrInSetPort(&pRef->PeerAddr, iConnPort);
    }

    // clear any previous receive errors
    pRef->uSndErr = 0;
    return(iErr);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPConnect

    \Description
        Initiate a connection to a peer

    \Input *pRef    - reference pointer
    \Input *pAddr   - address in ip-address:port form

    \Output
        int32_t     - negative=error, zero=ok

    \Notes
        Does not currently perform dns translation

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
int32_t CommUDPConnect(CommUDPRef *pRef, const char *pAddr)
{
    int32_t iErr, iConnPort, iListenPort;
    uint32_t uAddr;
    CommUDPRef *pFind;
    SocketT *sock;
    struct sockaddr BindAddr, GlueAddr, PeerAddr;

    // setup target info
    SockaddrInit(&PeerAddr, AF_INET);
    SockaddrInit(&BindAddr, AF_INET);

    // parse addr - make sure we have at least an address and port
    iErr = SockaddrInParse2(&uAddr, &iListenPort, &iConnPort, pAddr);
    if ((iErr & 3) != 3)
    {
        return(COMM_BADADDRESS);
    }

    // if we don't have an alternate connect port, connect=listen
    if (iConnPort == 0)
    {
        iConnPort = iListenPort++;
    }

    // set listen port
    SockaddrInSetPort(&BindAddr, iListenPort);

    // set connection identifier
    _CommUDPSetConnID(pRef, pAddr);
    NetPrintf(("commudp: [%p] connect addr=%08x, bind=%d, peer=%d connident=0x%08x\n",
        pRef, uAddr, iListenPort, iConnPort, pRef->uConnIdent));

#if defined(DIRTYCODE_PC)
    /*
    the following line will fill in bindaddr with the IP addr previously set with
    SocketControl(... 'ladr' ...). If that selector was never used, then the IP
    address field of bindaddr will just be filled with 0.
    note: only required for multi-homed system (PC)
    */
    SocketInfo(NULL, 'ladr', 0, &BindAddr, sizeof(BindAddr));
#endif

    // see if there is an existing socket bound to this port
    for (pFind = g_link, sock = NULL; pFind != NULL; pFind = pFind->pLink)
    {
        // dont check ourselves or unbound sockets
        if ((pFind == pRef) || (pFind->pSocket == NULL))
            continue;

        // see where this endpoint is bound
        if (SocketInfo(pFind->pSocket, 'bind', 0, &GlueAddr, sizeof(GlueAddr)) < 0)
            continue;

        /*
        see if the socket can be reused
            if the endpoint is virtual: we only compare the ports
            if the endpoint is not virtual:
                if bindaddr (specifying what we want to bind to) has ipaddr=0, we only compare the ports
                otherwise we compare the full sockaddr structs (i.e. family, port, addr)
        */
        if (SockaddrInGetPort(&BindAddr) == SockaddrInGetPort(&GlueAddr))
        {
            if ((SocketInfo(pFind->pSocket, 'virt', 0, NULL, 0) == TRUE) ||
                (SockaddrInGetAddr(&BindAddr) == 0) ||
                (SockaddrCompare(&BindAddr, &GlueAddr) == 0))
            {
                // share the socket
                sock = pFind->pSocket;
                break;
            }
        }
    }

    // create the actual socket
    if (sock == NULL)
    {
        sock = SocketOpen(AF_INET, SOCK_DGRAM, 0);
        if (sock == NULL)
        {
            return(COMM_NORESOURCE);
        }
        
        // bind socket
        if ((iErr = SocketBind(sock, &BindAddr, sizeof(BindAddr))) < 0)
        {
            NetPrintf(("commudp: [%p] bind to %d failed with %d\n", pRef, iListenPort, iErr));
            SockaddrInSetPort(&BindAddr, 0);
            if ((iErr = SocketBind(sock, &BindAddr, sizeof(BindAddr))) < 0)
            {
                NetPrintf(("commudp: [%p] bind to 0 failed with result %d\n", pRef, iErr));
                SocketClose(sock);
                return(COMM_UNEXPECTED);
            }
            else
            {
                SocketInfo(sock, 'bind', 0, &BindAddr, sizeof(BindAddr));
                NetPrintf(("commudp: [%p] bound socket to port %d\n", pRef, SockaddrInGetPort(&BindAddr)));
            }
        }
    }

    // set connect sockaddr
    SockaddrInSetAddr(&PeerAddr, uAddr);
    SockaddrInSetPort(&PeerAddr, iConnPort);

    // clear any previous receive errors
    pRef->uSndErr = 0;

    // pass to common handler
    return(_CommUDPConnect0(pRef, sock, &PeerAddr));
}

