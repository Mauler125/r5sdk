/*H*************************************************************************************************/
/*!

    \File    commsrp.c

    \Description
        This is CommSRP (Selectively Reliable Protocol), a datagram packet-based
        transport class.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 1999-2003.  ALL RIGHTS RESERVED.

    \Version    0.5        01/03/03 (JLB) Initial Version, based on CommTCP
    \Version    0.7        01/07/03 (JLB) Working unreliable transport, based on CommUDP
    \Version    0.8        01/08/03 (JLB) Working reliable transport.
    \Version    0.9        02/09/03 (JLB) Added support for sending zero-byte packets, and
                                          fixed PS2 alignment issue.
*/
/*************************************************************************************************H*/

/*** Include files *********************************************************************/

#define ESC_CAUSES_LOSS   (0)

#if ESC_CAUSES_LOSS
#include <windows.h>
#endif

#include <stdio.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/comm/commall.h"
#include "DirtySDK/comm/commsrp.h"

/*** Defines ***************************************************************************/

//! enable debug spam
#define COMMSRP_VERBOSE     (COMM_PRINT && FALSE)

//! control packet types
enum
{
    CTRL_PACKET_FIRST = 16,                 //!< 16: beginning of packet code range

    CTRL_PACKET_INIT = CTRL_PACKET_FIRST,   //!< 16: init packet
    CTRL_PACKET_POKE,                       //!< 17: firewall poke packet
    CTRL_PACKET_CONN,                       //!< 18: connection confirmation packet
    CTRL_PACKET_KEEP,                       //!< 19: keep-alive packet
    CTRL_PACKET_DISC,                       //!< 20: connection terminated packet

    CTRL_PACKET_LAST = 63                   //!< 63: end of packet code range
};

//! size of sequence set
#define SEQN_SIZE   (64)
//! sequence set mask
#define SEQN_MASK   (SEQN_SIZE-1)

//! base of unreliable sequence set
#define UNRELSEQN_BASE  (64)
//! base of reliable sequence set
#define RELSEQN_BASE    (128)
//! base of reliable packet reception acknowledgement sequence set
#define RELSEQNACK_BASE (192)

//! rate at which to resend unacknoweldged packets
#define RELIABLE_RESEND_RATE (250)

//! percentage of packets in receive buffer reserved for reliable packets (divisor)
#define RELIABLE_PCT_RESERVED   (8)


/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! raw protocol packet format
/*! Notes:
    The RawSRPPacket code field looks like this:

        value(s)    meaning
        ~~~~~~~~~~~~~~~~~~~~~~
        0           reserved
        1-15        bundled packet count
        16          ctrl packet: init connection (CTRL_PACKET_INIT)
        17          ctrl packet: poke firewall (CTRL_PACKET_POKE)
        18          ctrl packet: connection established (CTRL_PACKET_CONN)
        19          ctrl packet: keep-alive (CTRL_PACKET_KEEP)
        20          ctrl packet: disconnect (CTRL_PACKET_DISC)
        21-63       reserved
        64-127      unreliable packet: sequence number is code-64
        128-191     reliable packet: sequence number is code-128
        192-255     reliable packet acknowledgement: sequence number is code-192
        ~~~~~~~~~~~~~~~~~~~~~~

    If the code field is 1-15, then the value represents the number of packets
    bundled together into the same UDP frame minus one (a value of one is
    unsupported, as it would simply make the single packet bulkier).  The
    bundle format looks as follows:

        offset      value       meaning
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        0           code        bundle count-1 (1-15)

        1           len0-1      length of first bundled packet
        2           code0       code of first packet
        3           data0       data for first packet

        4+len0      len1-1      length of second bundled packet
        5+len0      code1       code of second packet
        6+len0      data1       data for second packet

        [...]
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

*/

typedef struct
{
    //! packet header which is not sent/received
    //! (this data is used internally)
    struct
    {
        uint32_t when;          //!< tick when a packet was received
        uint32_t len;           //!< size of packet
    } head;
    //! packet body which is sent/received
    struct
    {
        unsigned char code;         //!< packet code (see packet codes above)
        unsigned char data[1];      //!< packet user data (variable length)
    } body;
} RawSRPPacketT;


//! private module storage
struct CommSRPRef
{
    //! common header
    CommRef common;
    //! linked list of all instances
    CommSRPRef *link;
    //! comm socket
    SocketT *socket;
    //! peer address
    struct sockaddr peeraddr;

    //! port state
    enum {
        ST_IDLE,    //!< no connection
        ST_CONN,    //!< connecting
        ST_LIST,    //!< listening
        ST_OPEN,    //!< connection established
        ST_CLOSE    //!< connection closed
    } state;

    //! identifier to keep from getting spoofed
    uint32_t connident;

    //! width of receive records (same as width of send)
    int32_t rcvwid;
    //! number of packets reserved for reliable transport
    int32_t rcvrelresv;
    //! length of receive buffer (multiple of rcvwid)
    int32_t rcvlen;
    //! fifo input offset
    int32_t rcvinp;
    //! fifo output offset
    int32_t rcvout;
    //! pointer to buffer storage
    char *rcvbuf;

    //! width of send record (same as width of receive)
    int32_t sndwid;
    //! length of send buffer (multiple of sndwid)
    int32_t sndlen;
    //! fifo input offset
    int32_t sndinp;
    //! fifo output offset
    int32_t sndout;
    //! pointer to buffer storage
    char *sndbuf;

    //! tick at which last packet was sent
    uint32_t sendtick;
    //! tick at which last packet was received
    uint32_t recvtick;

    //! unreliable sequence number
    uint32_t unrelseqn;
    //! unreliable sequence number
    uint32_t unrelrecvseqn;
    //! reliable sequence number
    uint32_t relseqn;
    //! reliable received sequence number
    uint32_t relrecvseqn;

    //! allow for dns lookup
    uint32_t dnsid;
    char dnsquery[256];
    char *dnsbuf;
    int32_t dnslen;
    char dnsdiv;

    //! semaphore to synchronize thread access
    NetCritT crit;
    //! callback synchronization
    volatile int32_t callback;
    //! indcate pending event
    int32_t gotevent;
    //! callback routine pointer
    void (*callproc)(CommRef *ref, int32_t event);
};

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

// Public variables


/*** Private Functions *****************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    _CommSRPSetAddrInfo

    \Description
        Sets peer/host addr/port info in common ref.

    \Input *ref     - reference pointer
    \Input sin      - address pointer

    \Version 04/16/04 (JLB)
*/
/*************************************************************************************************F*/
static void _CommSRPSetAddrInfo(CommSRPRef *ref, struct sockaddr *sin)
{
    struct sockaddr SockAddr;

    // save peer addr/port info in common ref
    ref->common.peerip = SockaddrInGetAddr(sin);
    ref->common.peerport = SockaddrInGetPort(sin);

    // save host addr/port info in common ref
    SocketInfo(ref->socket, 'bind', 0, &SockAddr, sizeof(SockAddr));
    ref->common.hostip = SocketGetLocalAddr();
    ref->common.hostport = SockaddrInGetPort(&SockAddr);

    // debug output
    NetPrintf(("commsrp: peer=0x%08x:%d, host=0x%08x:%d\n",
        ref->common.peerip,
        ref->common.peerport,
        ref->common.hostip,
        ref->common.hostport));
}

/*F*************************************************************************************************/
/*!
    \Function    _CommSRPSetSocket

    \Description
        Sets socket in ref and socketref in common portion of ref.

    \Input *pRef    - reference pointer
    \Input *pSocket - socket to set

    \Version 08/24/04 (JLB)
*/
/*************************************************************************************************F*/
static void _CommSRPSetSocket(CommSRPRef *pRef, SocketT *pSocket)
{
    pRef->socket = pSocket;
    pRef->common.sockptr = pSocket;
}

/*F*************************************************************************************************/
/*!
    \Function    _CommSRPResetTransfer

    \Description
        Reset the transfer state

    \Input *ref    - reference pointer

    \Version 01/03/03 (JLB)
*/
/*************************************************************************************************F*/
static void _CommSRPResetTransfer(CommSRPRef *ref)
{
    // reset the send queue
    ref->sndinp = 0;
    ref->sndout = 0;

    // reset the receive queue
    ref->rcvinp = 0;
    ref->rcvout = 0;

    // make sendtick really old (in protocol terms)
    ref->sendtick = NetTick()-5000;

    // start reliable received sequence at an invalid sequence number
    ref->relrecvseqn = (uint32_t)-1;
}

/*F*************************************************************************************************/
/*!
    \Function    _CommSRPSendImmediate

    \Description
        Send an packet to the peer.

    \Input *ref     - reference pointer
    \Input *packet  - packet pointer

    \Output
        int32_t         - negative=error, else amount of data sent

    \Version 01/07/03 (JLB)
*/
/*************************************************************************************************F*/
static int32_t _CommSRPSendImmediate(CommSRPRef *ref, RawSRPPacketT *packet)
{
    int32_t len, err;

    #if ESC_CAUSES_LOSS
    // lose packets when escape is pressed
    if (GetAsyncKeyState(VK_ESCAPE) < 0)
    {
        ref->sendtick = NetTick();
        return(0);
    }
    #endif

    // figure out amount to send
    len = sizeof(packet->body)-sizeof(packet->body.data);   // packet framing
    len += packet->head.len;                                // variable data

    #if COMMSRP_VERBOSE
    {
        char addrbuf[32];

        SockaddrInGetAddrText(&ref->peeraddr,addrbuf,sizeof(addrbuf)),
        NetPrintf(("_CommSRPSendImmediate: Sending %d bytes to %s:%d\n",len, addrbuf,
            SockaddrInGetPort(&ref->peeraddr)));
    }
    #endif

    // send some data
    err = SocketSendto(ref->socket,(char *)&packet->body,len,0,
        &ref->peeraddr,sizeof(ref->peeraddr));

    // check for send failure
    if (err != len)
    {
        NetPrintf(("_CommSRPSendImmediate: SocketSendto returned %d\n", err));
        return(-1);
    }

    // update last send time
    ref->sendtick = NetTick();
    ref->common.datasent += len;
    ref->common.packsent += 1;

    return(len);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommSRPSendCtrl

    \Description
        Send a control packet.

    \Input *ref    - reference pointer
    \Input code    - control code to send

    \Notes
        Control packets are sent unreliably.

    \Version 01/08/03 (JLB)
*/
/*************************************************************************************************F*/
static void _CommSRPSendCtrl(CommSRPRef *ref, unsigned char code)
{
    RawSRPPacketT packet;

    #if COMMSRP_VERBOSE
    NetPrintf(("_CommSRPSendCtrl: Sending control packet id %d\n", code));
    #endif

    packet.head.len = 0;
    packet.body.code = code;

    _CommSRPSendImmediate(ref, &packet);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommSRPProcessInitRequest

    \Description
        Send an INIT control packet if we know our peer.

    \Input *ref    - reference pointer

    \Version 01/08/03 (JLB)
*/
/*************************************************************************************************F*/
static void _CommSRPProcessInitRequest(CommSRPRef *ref)
{
    // see if we know our peer
    if ((SockaddrInGetAddr(&ref->peeraddr) == 0) || (SockaddrInGetPort(&ref->peeraddr) == 0))
    {
        return;
    }

    // send connect message once a second
    if ((NetTick() - ref->sendtick) >= 1000)
    {
        if (ref->state == ST_CONN)
        {
            // send connection initiation packet
            _CommSRPSendCtrl(ref, CTRL_PACKET_INIT);
        }
        else if (ref->state == ST_LIST)
        {
            // send poke packet
            _CommSRPSendCtrl(ref, CTRL_PACKET_POKE);
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _CommSRPProcessACK

    \Description
        Process a reliable packet acknowledgement

    \Input *ref     - reference pointer
    \Input *packet  - ack packet

    \Version 01/08/03 (JLB)
*/
/*************************************************************************************************F*/
static void _CommSRPProcessACK(CommSRPRef *ref, RawSRPPacketT *packet)
{
    uint32_t ackseqnid, refseqnid;
    RawSRPPacketT *refpkt;

    // get a pointer to packet this should be an ack for
    refpkt = (RawSRPPacketT *)&ref->sndbuf[ref->sndout];

    // decode sequence ids
    ackseqnid = packet->body.code - RELSEQNACK_BASE;
    refseqnid = refpkt->body.code - RELSEQN_BASE;

    // compare sequence ids
    if (ackseqnid == refseqnid)
    {
        // packet was successfully transmitted, so dequeue it
        ref->sndout = (ref->sndout+ref->sndwid)%ref->sndlen;

        #if COMMSRP_VERBOSE
        NetPrintf(("_CommSRPProcessACK: Success: confirmed delivery of packet %d\n",
            refseqnid));
        #endif
    }
    else
    {
        #if COMMSRP_VERBOSE
        NetPrintf(("_CommSRPProcessACK: Got old ack %d, wanted ack %d\n",
            ackseqnid,refseqnid));
        #endif
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _CommSRPProcessCtrl

    \Description
        Process an incoming control message.

    \Input *ref     - reference pointer
    \Input *packet  - control packet to process
    \Input *pFrom   - sender's socket address

    \Version 01/07/03 (JLB)
*/
/*************************************************************************************************F*/
static void _CommSRPProcessCtrl(CommSRPRef *ref, RawSRPPacketT *packet, struct sockaddr *pFrom)
{
    // update valid receive time
    // must put into past to avoid race condition
    ref->recvtick = NetTick()-1000;

    switch(packet->body.code)
    {
        // response to connection request packet
        case CTRL_PACKET_INIT:
        {
            #if COMMSRP_VERBOSE
            NetPrintf(("_CommSRPProcessSetup: Received INIT\n"));
            #endif

            if (ref->state == ST_LIST)
            {
                // set the peer
                ds_memcpy_s(&ref->peeraddr, sizeof(ref->peeraddr), pFrom, sizeof(*pFrom));

                // set peer/host addr/port info
                _CommSRPSetAddrInfo(ref, pFrom);

                // update state
                ref->state = ST_OPEN;
            }

            // always send a response
            _CommSRPSendCtrl(ref, CTRL_PACKET_CONN);
        }
        break;

        // response to poke packet
        case CTRL_PACKET_POKE:
        {
            #if COMMSRP_VERBOSE
            NetPrintf(("_CommSRPProcessSetup: Received POKE\n"));
            #endif

            if (ref->state == ST_CONN)
            {
                // set the peer
                ds_memcpy_s(&ref->peeraddr, sizeof(ref->peeraddr), pFrom, sizeof(*pFrom));
            }
        }
        break;

        // response to a connect confirmation
        case CTRL_PACKET_CONN:
        {
            #if COMMSRP_VERBOSE
            NetPrintf(("_CommSRPProcessSetup: Received CONN\n"));
            #endif

            // change to open if not already there
            if (ref->state == ST_CONN)
            {
                // set peer/host addr/port info
                _CommSRPSetAddrInfo(ref, pFrom);

                ref->state = ST_OPEN;
            }
        }
        break;

        // response to disconnect message
        case CTRL_PACKET_DISC:
        {
            #if COMMSRP_VERBOSE
            NetPrintf(("_CommSRPProcessSetup: Received DISC\n"));
            #endif

            // close the connection
            if (ref->state == ST_OPEN)
            {
                ref->state = ST_CLOSE;
            }
        }
        break;

        // response to keepalive message
        case CTRL_PACKET_KEEP:
        {
            #if COMMSRP_VERBOSE
            NetPrintf(("_CommSRPProcessSetup: Received KEEP\n"));
            #endif
        }
        break;

        // this case should not happen
        default:
        {
            NetPrintf(("_CommSRPProcessSetup: Unrecognized control packet type %d\n",packet->body.code));
        }
        break;
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _CommSRPProcessData

    \Description
        Process incoming data packet.

    \Input *ref     - reference pointer
    \Input *packet  - incoming packet

    \Output
        int32_t     - <0 = error, 0 = packet discarded (duplicate resend), 1 = packet added to buffer

    \Version 01/07/03 (JLB)
*/
/*************************************************************************************************F*/
static int32_t _CommSRPProcessData(CommSRPRef *ref, RawSRPPacketT *packet)
{
    RawSRPPacketT *buffer;

    if ((packet->body.code >= UNRELSEQN_BASE) && (packet->body.code < RELSEQN_BASE))
    {
        int32_t queuepos, pktsfree;

        // calculate the number of free packets in rcvbuf
        queuepos = ((ref->rcvinp+ref->rcvlen)-ref->rcvout)%ref->rcvlen;
        pktsfree = (ref->rcvlen - queuepos)/ref->rcvwid;

        // see if there is room in buffer for packet (leave extra space for reliable packets)
        if (pktsfree <= ref->rcvrelresv)
        {
            // no room in buffer -- just drop packet
            #if COMMSRP_VERBOSE // (we expect lots of these, so don't spam)
            NetPrintf(("_CommSRPProcessData: Unreliable packet %d discarded due to input buffer overrun.\n",ref->relrecvseqn));
            #endif
            return(-1);
        }

        #if COMMSRP_VERBOSE
        NetPrintf(("_CommSRPProcessData: Received packet %d\n",packet->body.code - UNRELSEQN_BASE));
        #endif

        // update unreliably received sequence number
        ref->unrelrecvseqn = packet->body.code - UNRELSEQN_BASE;
    }
    else
    {
        uint32_t uPacketId;

        // see if room in buffer for packet
        if ((ref->rcvinp+ref->rcvwid)%ref->rcvlen == ref->rcvout)
        {
            // no room in buffer -- just drop packet
            NetPrintf(("_CommSRPProcessData: Reliable packet discarded due to input buffer overrun\n"));
            return(-1);
        }

        // get packet ID
        uPacketId = (uint32_t)(packet->body.code - RELSEQN_BASE);

        // is this the packet we're expecting?
        if (uPacketId == ((ref->relrecvseqn+1) % SEQN_SIZE))
        {
            // update reliably received sequence number
            ref->relrecvseqn = uPacketId;

            #if COMMSRP_VERBOSE
            NetPrintf(("_CommSRPProcessData: Received packet %d\n", ref->relrecvseqn));
            #endif
        }
        else if (uPacketId == ref->relrecvseqn)
        {
            // this is the previously received packet - return TRUE to ack it in case our last ack got lost
            return(1);
        }
        else
        {
            #if COMMSRP_VERBOSE
            NetPrintf(("_CommSRPProcessData: Discarding duplicate packet %d\n",ref->relrecvseqn));
            #endif
            return(0);
        }
    }

    // add the packet to the buffer
    buffer = (RawSRPPacketT *) &ref->rcvbuf[ref->rcvinp];
    ds_memcpy(buffer, packet, ref->rcvwid);

    // limit receive access for callbacks
    ref->callback += 1;
    // add item to receive buffer
    ref->rcvinp = (ref->rcvinp+ref->rcvwid) % ref->rcvlen;
    // indicate we got an event
    ref->gotevent |= 1;
    // let the callback process it
    if (ref->common.RecvCallback != NULL)
    {
        ref->common.RecvCallback((CommRef *)ref, buffer->body.data, buffer->head.len, buffer->head.when);
    }
    // release access to receive
    ref->callback -= 1;
    return(1);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommSRPProcessAlive

    \Description
        Send out a keep-alive packet (CTRL_PACKET_KEEP) periodically

    \Input *ref     - reference pointer

    \Version 01/07/03 (JLB)
*/
/*************************************************************************************************F*/
static void _CommSRPProcessAlive(CommSRPRef *ref)
{
    if ((ref->state == ST_OPEN) && (ref->sndinp == ref->sndout))
    {
        // see if time has elapsed
        if (NetTick() > ref->sendtick+5000)
        {
            // queue up a keepalive packet
            _CommSRPSendCtrl(ref, CTRL_PACKET_KEEP);
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _CommSRPProcessRecvQueue

    \Description
        Attempt to read data from peer and add it to the receive queue.

    \Input *ref     - reference pointer

    \Version 01/08/03 (JLB)
*/
/*************************************************************************************************F*/
static void _CommSRPProcessRecvQueue(CommSRPRef *ref)
{
    int32_t len, sinlen, iResult = 0;
    char *buffer;
    struct sockaddr sin;
    RawSRPPacketT *packet;

    while((ref->state >= ST_CONN) && (ref->state <= ST_OPEN) && (iResult >= 0))
    {
        // get pointer to recv queue
        packet = (RawSRPPacketT *) &ref->rcvbuf[ref->rcvinp];
        buffer = (char *) &packet->body;

        // attempt to get a packet
        sinlen = sizeof(sin);
        len = SocketRecvfrom(ref->socket, buffer, ref->rcvwid, 0, &sin, &sinlen);

        // if we got data add it to recv queue
        if (len > 0)
        {
            #if COMMSRP_VERBOSE
            NetPrintf(("_CommSRPProcessRecvQueue: Received %d bytes of data\n",len));
            #endif

            packet->head.len = len;
            packet->head.when = SockaddrInGetMisc(&sin);

            // process packet
            if ((packet->body.code >= CTRL_PACKET_FIRST) && (packet->body.code <= CTRL_PACKET_LAST))
            {
                // handle control packets
                _CommSRPProcessCtrl(ref, packet, &sin);
            }
            else if (packet->body.code >= RELSEQNACK_BASE)
            {
                // handle ack of reliable packet
                _CommSRPProcessACK(ref, packet);
            }
            else
            {
                // handle data packets
                iResult = _CommSRPProcessData(ref, packet);
                if (iResult == 1)
                {
                    // check to see if this is a reliable packet and requires an acknowledgement
                    if ((packet->body.code >= RELSEQN_BASE) && (packet->body.code < RELSEQNACK_BASE))
                    {
                        // send an ack (seqid +64)
                        #if COMMSRP_VERBOSE
                        NetPrintf(("_CommSRPProcessRecvQueue: Sending ack for packet %d\n",packet->body.code));
                        #endif

                        _CommSRPSendCtrl(ref,packet->body.code + SEQN_SIZE);
                    }
                }
            }
        }
        else if (len < 0)
        {
            // error in recv - close connection
            NetPrintf(("_CommSRPProcessRecvQueue: Error %d - closing connection\n",len));
            ref->state = ST_CLOSE;
            break;
        }
        else
        {
            // no data to receive
            break;
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _CommSRPProcessSendQueue

    \Description
        Send data to peer from send queue.

    \Input *ref     - reference pointer

    \Version 01/08/03 (JLB)
*/
/*************************************************************************************************F*/
static void _CommSRPProcessSendQueue(CommSRPRef *ref)
{
    RawSRPPacketT *packet;

    if ((ref->state == ST_OPEN) && (ref->sndinp != ref->sndout))
    {
        // ref next packet to send
        packet = (RawSRPPacketT *) &ref->sndbuf[ref->sndout];

        // send packet
        if ((packet->head.when == 0) || ((NetTick() - packet->head.when) >= RELIABLE_RESEND_RATE))
        {
            #if COMMSRP_VERBOSE
            if (packet->head.when != 0)
            {
                NetPrintf(("_CommSRPProcessSendQueue: Resending sequence id %d\n",
                    packet->body.code - RELSEQN_BASE));
            }
            #endif

            #if COMMSRP_VERBOSE
            if (packet->head.when == 0)
            {
                NetPrintf(("_CommSRPProcessSendQueue: Sending sequence id %d\n",
                    packet->body.code - RELSEQN_BASE));
            }
            #endif

            // send the packet
            if (_CommSRPSendImmediate(ref, packet) < 0)
            {
                return;
            }

            // update sent tick
            packet->head.when = NetTick();
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _CommSRPClose

    \Description
        Close the connection

    \Input *ref     - reference pointer

    \Output
        int32_t     - zero

    \Version 01/03/03 (JLB)
*/
/*************************************************************************************************F*/
static int32_t _CommSRPClose(CommSRPRef *ref)
{
    // see if we are even connected
    if (ref->state != ST_OPEN)
    {
        return(0);
    }

    // send a disconnect message
    _CommSRPSendCtrl(ref, CTRL_PACKET_DISC);

    // set to disconnect state
    ref->connident = 0;
    ref->state = ST_CLOSE;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommSRPEvent0

    \Description
        Private socket callback function for CommSRP event processing.

    \Input *ref    - reference pointer

    \Notes
        This function should only be called by _CommSRPEvent()

    \Version 01/08/03 (JLB)
*/
/*************************************************************************************************F*/
static void _CommSRPEvent0(CommSRPRef *ref)
{
    // send init request if we're connecting
    if ((ref->state == ST_CONN) || (ref->state == ST_LIST))
    {
        _CommSRPProcessInitRequest(ref);
    }

    // receive data into recv queue
    _CommSRPProcessRecvQueue(ref);

    // periodically send a keep-alive
    _CommSRPProcessAlive(ref);

    // send all queued data
    _CommSRPProcessSendQueue(ref);

    // do callback if needed
    if ((ref->callback == 0) && (ref->gotevent != 0))
    {
        // limit callback access
        ref->callback += 1;
        // notify user
        if (ref->callproc != NULL)
        {
            ref->callproc((CommRef *)ref, ref->gotevent);
        }
        // limit callback access
        ref->callback -= 1;
        // done with event
        ref->gotevent = 0;
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _CommSRPEvent

    \Description
        Socket callback function for CommSRP event processing, protected
        with critical section.

    \Input *sock    - unused
    \Input flags    - unused
    \Input *_ref    - reference pointer

    \Output
        int32_t     - zero

    \Version 01/08/03 (JLB)
*/
/*************************************************************************************************F*/
static int32_t _CommSRPEvent(SocketT *sock, int32_t flags, void *_ref)
{
    CommSRPRef *ref = _ref;

    // see if we have exclusive access
    if (NetCritTry(&ref->crit))
    {
        // update
        _CommSRPEvent0(ref);

        // free access
        NetCritLeave(&ref->crit);
    }

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommSRPSendQueued

    \Description
        Add packet to send queue.

    \Input *ref     - reference pointer
    \Input *packet  - packet to add to send queue

    \Output
        int32_t     - buffer depth

    \Notes
        Only reliable packets should be added to the send queue; use _CommSRPSendImmediate() for
        unreliable packet data.

    \Version 01/08/03 (JLB)
*/
/*************************************************************************************************F*/
static int32_t _CommSRPSendQueued(CommSRPRef *ref, RawSRPPacketT *packet)
{
    int32_t pos;

    // mark timestamp=0 for immediate send
    packet->head.when = 0;

    // add the packet to the queue
    ref->sndinp = (ref->sndinp+ref->sndwid) % ref->sndlen;

    // try and send immediately
    _CommSRPEvent(ref->socket, 0, ref);

    // return buffer depth
    pos = (((ref->sndinp+ref->sndlen)-ref->sndout)%ref->sndlen)/ref->sndwid;
    return(pos);
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    CommSRPConstruct

    \Description
        Construct the class

    \Input maxwid   - max record width
    \Input maxinp   - input packet buffer size
    \Input maxout   - output packet buffer size

    \Output
        CommSRPRef  - reference pointer

    \Notes
        Initialized winsock for first class. also creates linked
        list of all current instances of the class and worker thread
        to do most udp stuff.

    \Version 01/03/03 (JLB)
*/
/*************************************************************************************************F*/
CommSRPRef *CommSRPConstruct(int32_t maxwid, int32_t maxinp, int32_t maxout)
{
    CommSRPRef *ref;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate class storage
    ref = DirtyMemAlloc(sizeof(*ref), COMMSRP_MEMID, iMemGroup, pMemGroupUserData);
    if (ref == NULL)
        return(NULL);
    ds_memclr(ref, sizeof(*ref));
    ref->common.memgroup = iMemGroup;
    ref->common.memgrpusrdata = pMemGroupUserData;

    // initialize the callback routines
    ref->common.Construct = (CommAllConstructT *)CommSRPConstruct;
    ref->common.Destroy = (CommAllDestroyT *)CommSRPDestroy;
    ref->common.Resolve = (CommAllResolveT *)CommSRPResolve;
    ref->common.Unresolve = (CommAllUnresolveT *)CommSRPUnresolve;
    ref->common.Listen = (CommAllListenT *)CommSRPListen;
    ref->common.Unlisten = (CommAllUnlistenT *)CommSRPUnlisten;
    ref->common.Connect = (CommAllConnectT *)CommSRPConnect;
    ref->common.Unconnect = (CommAllUnconnectT *)CommSRPUnconnect;
    ref->common.Callback = (CommAllCallbackT *)CommSRPCallback;
    ref->common.Status = (CommAllStatusT *)CommSRPStatus;
    ref->common.Tick = (CommAllTickT *)CommSRPTick;
    ref->common.Send = (CommAllSendT *)CommSRPSend;
    ref->common.Peek = (CommAllPeekT *)CommSRPPeek;
    ref->common.Recv = (CommAllRecvT *)CommSRPRecv;

    // remember max packet width
    ref->common.maxwid = maxwid;
    ref->common.maxinp = maxinp;
    ref->common.maxout = maxout;

    // reset access to shared resources
    NetCritInit(&ref->crit, "commsrp");

    // allocate the buffers
    ref->rcvwid = sizeof(RawSRPPacketT)-sizeof(((RawSRPPacketT *)0)->body.data)+maxwid;
    ref->rcvwid = (ref->rcvwid+3) & 0x7ffc;
    ref->rcvlen = ref->rcvwid * maxinp;
    ref->rcvbuf = DirtyMemAlloc(ref->rcvlen, COMMSRP_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);
    ref->sndwid = sizeof(RawSRPPacketT)-sizeof(((RawSRPPacketT *)0)->body.data)+maxwid;
    ref->sndwid = (ref->sndwid+3) & 0x7ffc;
    ref->sndlen = ref->sndwid * maxout;
    ref->sndbuf = DirtyMemAlloc(ref->sndlen, COMMSRP_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);

    // calculate number of packets reserved for reliable transport
    ref->rcvrelresv = maxinp/RELIABLE_PCT_RESERVED;

    // reset the socket
    _CommSRPSetSocket(ref, NULL);

    // reset the state
    ref->state = ST_IDLE;
    ref->connident = 0;

    // return the reference
    return(ref);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSRPDestroy

    \Description
        Destruct the class

    \Input *ref     - reference pointer

    \Version 01/03/03 (JLB)
*/
/*************************************************************************************************F*/
void CommSRPDestroy(CommSRPRef *ref)
{
    // if port is open, close it
    if (ref->state == ST_OPEN)
    {
        _CommSRPClose(ref);
    }

    // kill the socket
    if (ref->socket != NULL)
    {
        SocketClose(ref->socket);
        _CommSRPSetSocket(ref, NULL);
    }

    // get rid of sockets critical section
    NetCritKill(&ref->crit);

    // release resources
    DirtyMemFree(ref->rcvbuf, COMMSRP_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);
    DirtyMemFree(ref->sndbuf, COMMSRP_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);
    DirtyMemFree(ref, COMMSRP_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSRPCallback

    \Description
        Set upper layer callback

    \Input *ref         - reference pointer
    \Input *callback    - socket generating callback

    \Version 01/03/03 (JLB)
*/
/*************************************************************************************************F*/
void CommSRPCallback(CommSRPRef *ref, void (*callback)(CommRef *ref, int32_t event))
{
    ref->callproc = callback;
    ref->gotevent |= 2;
}

/*F*************************************************************************************************/
/*!
    \Function    CommSRPResolve

    \Description
        Resolve an address (unimplemented)

    \Input *ref     - endpoint
    \Input *addr    - resolve address
    \Input *buf     - target buffer
    \Input len      - target length (min 64 bytes)
    \Input div      - divider char

    \Output
        int32_t         - <0=error, 0=complete (COMM_NOERROR), >0=in progress (COMM_PENDING)

    \Notes
        Target list is always double null terminated allowing null
        to be used as the divider character if desired. when COMM_PENDING
        is returned, target buffer is set to "~" until completion

    \Version 01/03/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t CommSRPResolve(CommSRPRef *ref, const char *addr, char *buf, int32_t len, char div)
{
    NetPrintf(("commsrp: resolve functionality not supported\n"));
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSRPUnresolve

    \Description
        Stop the resolver

    \Input *ref     - endpoint ref

    \Version 01/03/03 (JLB)
*/
/*************************************************************************************************F*/
void CommSRPUnresolve(CommSRPRef *ref)
{
    return;
}

/*F*************************************************************************************************/
/*!
    \Function    CommSRPUnlisten

    \Description
        Stop listening.

    \Input *ref     - reference pointer

    \Output
        int32_t     - negative=error, zero=ok

    \Version 01/03/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t CommSRPUnlisten(CommSRPRef *ref)
{
    // get rid of socket if presernt
    if (ref->socket != NULL)
    {
        SocketClose(ref->socket);
        _CommSRPSetSocket(ref, NULL);
    }

    // return to idle mode
    ref->state = ST_IDLE;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSRPUnconnect

    \Description
        Terminate a connection

    \Input *ref     - reference pointer

    \Output
        int32_t     - negative=error, zero=ok

    \Version 01/03/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t CommSRPUnconnect(CommSRPRef *ref)
{
    // get rid of socket if presernt
    if (ref->socket != NULL)
    {
        SocketClose(ref->socket);
        _CommSRPSetSocket(ref, NULL);
    }

    // return to idle mode
    ref->state = ST_IDLE;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSRPStatus

    \Description
        Return current stream status

    \Input *ref     - reference pointer

    \Output
        int32_t     - COMM_CONNECTING, COMM_OFFLINE, COMM_ONLINE or COMM_FAILURE

    \Version 01/03/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t CommSRPStatus(CommSRPRef *ref)
{
    // return state
    if ((ref->state == ST_CONN) || (ref->state == ST_LIST))
        return(COMM_CONNECTING);
    if ((ref->state == ST_IDLE) || (ref->state == ST_CLOSE))
        return(COMM_OFFLINE);
    if (ref->state == ST_OPEN)
        return(COMM_ONLINE);
    return(COMM_FAILURE);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSRPTick

    \Description
        Return current tick

    \Input *ref     - reference pointer

    \Output
        uint32_t    - elapsed milliseconds

    \Version 01/03/03 (JLB)
*/
/*************************************************************************************************F*/
uint32_t CommSRPTick(CommSRPRef *ref)
{
    return(NetTick());
}

/*F*************************************************************************************************/
/*!
    \Function    CommSRPSend

    \Description
        Send a packet

    \Input *ref     - reference pointer
    \Input *buffer  - pointer to data
    \Input length   - length of data
    \Input flags    - COMM_FLAGS_*

    \Output
        int32_t     - negative=error, zero=buffer full (temp fail), positive=queue position (ok)

    \Version 01/03/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t CommSRPSend(CommSRPRef *ref, const void *buffer, int32_t length, uint32_t flags)
{
    RawSRPPacketT *packet;
    int32_t pos;

    // make sure port is open
    if (ref->state != ST_OPEN)
    {
        return(COMM_BADSTATE);
    }

    // see if input queue full
    if ((ref->sndinp+ref->sndwid)%ref->sndlen == ref->sndout)
    {
        NetPrintf(("commsrp: input queue full\n"));
        return(0);
    }

    // return error for oversized packets
    if (length > (signed)(ref->sndwid-(sizeof(RawSRPPacketT)-sizeof((RawSRPPacketT *)0)->body.data)))
    {
        NetPrintf(("commsrp: oversized packet send (%d bytes)\n", length));
        return(COMM_MINBUFFER);
    }

    // zero sized packet cannot be sent (they are used for acks)
    // instead, treat them as successful which means the queue
    // position is returned
    if (length == 0)
    {
        pos = (((ref->sndinp+ref->sndlen)-ref->sndout)%ref->sndlen)/ref->sndwid;
        return(pos+1);
    }

    // add packet to send queue
    packet = (RawSRPPacketT *) &(ref->sndbuf[ref->sndinp]);

    // set packet length
    packet->head.len = length;

    // copy user data into queue
    ds_memcpy(packet->body.data, buffer, length);

    // send reliable or unreliable?
    if (flags & COMM_FLAGS_UNRELIABLE)
    {
        // unreliable sequence tracking
        packet->body.code = (unsigned char)ref->unrelseqn++ + UNRELSEQN_BASE;
        ref->unrelseqn &= SEQN_MASK;

        #if COMMSRP_VERBOSE
        NetPrintf(("commsrp: sending unreliable sequence number %d (len=%d)\n",packet->body.code-UNRELSEQN_BASE,length));
        #endif

        // unreliable packets are sent immediately
        if ((pos = _CommSRPSendImmediate(ref,packet)) > 0)
        {
            pos = 1;
        }
    }
    else
    {
        // reliable sequence tracking
        packet->body.code = (unsigned char)ref->relseqn++ + RELSEQN_BASE;
        ref->relseqn &= SEQN_MASK;

        #if COMMSRP_VERBOSE
        NetPrintf(("CommSRPSend: Sending reliable sequence number %d (len=%d)\n",packet->body.code-RELSEQN_BASE,length));
        #endif

        // reliable packets are added to the send queue
        pos = _CommSRPSendQueued(ref,packet);
    }

    // return buffer depth
    return((pos > 0) ? pos : 1);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSRPPeek

    \Description
        Peek at waiting packet, and copy to target buffer if present.

    \Input *ref     - reference pointer
    \Input *target  - target buffer
    \Input length   - buffer length
    \Input *when    - tick received at

    \Output
        int32_t     - negative=nothing pending, else packet length

    \Version 01/03/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t CommSRPPeek(CommSRPRef *ref, void *target, int32_t length, uint32_t *when)
{
    RawSRPPacketT *packet;
    int32_t packetlen;

    // see if a packet is available
    if (ref->rcvout == ref->rcvinp)
    {
        return(COMM_NODATA);
    }

    // point to the packet
    packet = (RawSRPPacketT *) &(ref->rcvbuf[ref->rcvout]);

    #if COMMSRP_VERBOSE
    NetPrintf(("commsrp: received packet, sequence number %d (len=%d)\n",packet->body.code,packet->head.len));
    #endif

    // packet length minus code byte
    packetlen = packet->head.len - (sizeof(packet->body)-sizeof(packet->body.data));

    // copy over the data portion
    ds_memcpy(target, packet->body.data, (packetlen < length ? packetlen : length));

    // get the timestamp
    if (when != NULL)
    {
        *when = packet->head.when;
    }

    // return packet data length
    return(packetlen);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSRPRecv

    \Description
        Receive a packet from the buffer

    \Input *ref     - reference pointer
    \Input *target  - target buffer
    \Input length   - buffer length
    \Input *when    - tick received at

    \Output
        int32_t     - negative=error, else packet length

    \Version 01/03/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t CommSRPRecv(CommSRPRef *ref, void *target, int32_t length, uint32_t *when)
{
    // use peek to remove the data
    int32_t len = CommSRPPeek(ref, target, length, when);

    if (len >= 0)
    {
        ref->rcvout = (ref->rcvout+ref->rcvwid)%ref->rcvlen;
    }

    // all done
    return(len);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSRPListen

    \Description
        Listen for a connection

    \Input *ref     - reference pointer
    \Input *addr    - port to listen on (only :port portion used)

    \Output
        int32_t     - negative=error, zero=ok

    \Version 01/03/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t CommSRPListen(CommSRPRef *ref, const char *addr)
{
    int32_t err, iListenPort, iConnPort;
    uint32_t uPokeAddr;
    struct sockaddr bindaddr;
    SocketT *sock;

    // make sure in valid state
    if ((ref->state != ST_IDLE) || (ref->socket != NULL))
    {
        return(COMM_BADSTATE);
    }

    // setup bind points
    SockaddrInit(&bindaddr, AF_INET);

    // parse at least port
    if ((SockaddrInParse2(&uPokeAddr, &iListenPort, &iConnPort, addr) & 0x2) != 0x2)
    {
        return(COMM_BADADDRESS);
    }
    SockaddrInSetPort(&bindaddr, iListenPort);

    // reset to unresolved
    _CommSRPResetTransfer(ref);

    // create socket in case its needed
    sock = SocketOpen(AF_INET, SOCK_DGRAM, 0);
    _CommSRPSetSocket(ref, sock);
    if (ref->socket == NULL)
    {
        return(COMM_NORESOURCE);
    }

    // bind locally
    #if COMMSRP_VERBOSE
    NetPrintf(("commsrp: binding to port %d\n", SockaddrInGetPort(&bindaddr)));
    #endif
    if ((err = SocketBind(ref->socket, &bindaddr, sizeof(bindaddr))) < 0)
    {
        NetPrintf(("commsrp: error %d binding socket\n", err));
        SocketClose(ref->socket);
        _CommSRPSetSocket(ref, NULL);
        return(COMM_UNEXPECTED);
    }

    // setup for callbacks
    SocketCallback(ref->socket, CALLB_RECV, 100, ref, &_CommSRPEvent);

    // see if we should setup peer address
    if (uPokeAddr != 0)
    {
        if (iConnPort == 0)
        {
            iConnPort = iListenPort+1;
        }

        SockaddrInit(&ref->peeraddr, AF_INET);
        SockaddrInSetAddr(&ref->peeraddr, uPokeAddr);
        SockaddrInSetPort(&ref->peeraddr, iConnPort);
    }

    // put into init mode
    ref->state = ST_LIST;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSRPConnect

    \Description
        Initiate a connection to a peer

    \Input *ref     - reference pointer
    \Input *pAddr   - address in ip-address:port form

    \Output
        int32_t     - negative=error, zero=ok

    \Notes
        Does not perform dns translation

    \Version 01/03/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t CommSRPConnect(CommSRPRef *ref, const char *pAddr)
{
    struct sockaddr bindaddr;
    int32_t iErr, iConnPort, iListenPort;
    uint32_t uAddr;
    SocketT *sock;

    // make sure in valid state
    if ((ref->state != ST_IDLE) || (ref->socket != NULL))
    {
        return(COMM_BADSTATE);
    }

    // parse address and port from addr string
    if ((SockaddrInParse2(&uAddr, &iListenPort, &iConnPort, pAddr) & 0x3) != 0x3)
    {
        return(COMM_BADADDRESS);
    }

    // if we don't have an alternate connect port: connect=listen, listen=listen+1
    if (iConnPort == 0)
    {
        iConnPort = iListenPort++;
    }

    // reset to unresolved
    _CommSRPResetTransfer(ref);

    // create the actual socket
    sock = SocketOpen(AF_INET, SOCK_DGRAM, 0);
    _CommSRPSetSocket(ref, sock);
    if (ref->socket == NULL)
    {
        return(COMM_NORESOURCE);
    }

    // set listen port
    SockaddrInit(&bindaddr, AF_INET);
    SockaddrInSetPort(&bindaddr, iListenPort);

    // bind to port
    #if COMMSRP_VERBOSE
    NetPrintf(("commsrp: binding to port %d\n", iListenPort));
    #endif
    if ((iErr = SocketBind(ref->socket, &bindaddr, sizeof(bindaddr))) < 0)
    {
        NetPrintf(("commsrp: error %d binding socket\n", iErr));
        return(COMM_UNEXPECTED);
    }

    // setup target info
    SockaddrInit(&ref->peeraddr, AF_INET);
    SockaddrInSetAddr(&ref->peeraddr, uAddr);
    SockaddrInSetPort(&ref->peeraddr, iConnPort);

    // setup for callbacks
    SocketCallback(ref->socket, CALLB_RECV, 100, ref, &_CommSRPEvent);

    // change the state
    ref->state = ST_CONN;
    return(0);
}
