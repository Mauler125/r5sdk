/*H*************************************************************************************/
/*!
    \File    protoadvt.c

    \Description
        This advertising module provides a relatively simple multi-protocol
        distributed name server architecture utilizing the broadcast capabilities
        of UDP and IPX. Once the module is instantiated, it can be used as both
        an advertiser (server) and watcher (client) simultaneously.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002.  ALL RIGHTS RESERVED.

    \Version  1.0  02/17/99 (GWS) Original version
    \Version  1.1  02/25/99 (GWS) Alpha release
    \Version  1.2  07/27/99 (GWS) Initial release
    \Version  1.3  10/28/99 (GWS) Message queue elimination
    \Version  1.4  07/10/00 (GWS) Windock2 dependency removal
    \Version  1.5  12/11/00 (GWS) Ported to Dirtysock
    \Version  2.0  03/28/03 (GWS) Made more robust with double-broadcast sends
    \Version  2.1  06/16/03 (JLB) Made string comparisons case-insensitive
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/proto/protoadvt.h"

#if defined(DIRTYCODE_PS4)
#include <sys/types.h>
#endif
/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! raw broadcast packet format
typedef struct ProtoAdvtPacketT
{
    //! static packet identifier
    uint8_t ident[3];
    //! single char freq (avoid byte ordering issues)
    uint8_t freq;
    //! advertisement sequence number
    uint8_t seqn[4];
    //! the kind of service
    char kind[32];
    //! the name of this particular service
    char name[32];
    //! misc notes about service
    char note[192];
    //! list of service addresses
    char addr[120];
} ProtoAdvtPacketT;

//! a list of services
typedef struct ServListT
{
    //! actual packet data
    ProtoAdvtPacketT packet;
    //! timeout until item expires or is rebroadcast
    uint32_t timeout;
    //! last internet address associated with service
    char inetdot[16];
    uint32_t inetval;
    uint32_t hostval;
    //! flag to indicate if packet has local origin
    int32_t local;
    //! link to next item in list
    struct ServListT *next;
} ServListT;

//! local module reference
struct ProtoAdvtRef
{
    //! control access to resources
    NetCritT crit;
    //! list of services to announce
    ServListT *announce;
    //! known service list
    ServListT *snoopbuf;
    ServListT *snoopend;
    //! dirtysock memory group
    int32_t memgroup;
    void *memgrpusrdata;
    //! seeding is needed
    int32_t seed;
    //! number of users
    int32_t usage;
    //! active socket
    SocketT *sock;
    //! broadcast address
    struct sockaddr addr;
    //! indicate that snoop buffer changed
    int32_t snoop;
};


/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

static int32_t g_count = 0;
static ProtoAdvtRef *g_ref = NULL;

// Public variables


/*** Private Functions *****************************************************************/


/*F*************************************************************************************/
/*!
    \Function _ProtoAdvtSendPacket

    \Description
        Sends a ProtoAdvt packet.

    \Input *pRef    - module ref
    \Input *pPacket - packet to send

    \Output
        int32_t     - return result from SocketSendto()

    \Version 11/30/2006 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t _ProtoAdvtSendPacket(ProtoAdvtRef *pRef, const ProtoAdvtPacketT *pPacket)
{
    int32_t iPacketSize = sizeof(*pPacket);
    return(SocketSendto(pRef->sock, (const char *)pPacket, iPacketSize, 0, &pRef->addr, sizeof(pRef->addr)));
}

/*F*************************************************************************************/
/*!
    \Function _ProtoAdvtRecvPacket

    \Description
        Receives a ProtoAdvt packet.  Variable-length string fields are unpacked from
        the compacted sent packet to produce the fixed-length result packet.

    \Input *pRef    - module ref
    \Input *pPacket - packet buffer to receive to
    \Input *pFrom   - sender's address

    \Output
        int32_t     - return result from SocketRecvfrom()

    \Version 11/30/2006 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t _ProtoAdvtRecvPacket(ProtoAdvtRef *pRef, ProtoAdvtPacketT *pPacket, struct sockaddr *pFrom)
{
    int32_t iFromLen = sizeof(*pFrom);
    return(SocketRecvfrom(pRef->sock, (char *)pPacket, sizeof(*pPacket), 0, pFrom, &iFromLen));
}

/*F*************************************************************************************/
/*!
    \Function _ProtoAdvtRequestSeed

    \Description
        Send a seed request

    \Input *pRef    - module ref

    \Version 03/28/03 (GWS)
*/
/*************************************************************************************F*/
static void _ProtoAdvtRequestSeed(ProtoAdvtRef *pRef)
{
    ProtoAdvtPacketT Packet;

    // setup a query packet
    ds_memclr(&Packet, sizeof(Packet));
    Packet.ident[0] = ADVERT_PACKET_IDENTIFIER[0];
    Packet.ident[1] = ADVERT_PACKET_IDENTIFIER[1];
    Packet.ident[2] = ADVERT_PACKET_IDENTIFIER[2];
    Packet.kind[0] = '?';

    // do the send
    _ProtoAdvtSendPacket(pRef, &Packet);
}


/*F*************************************************************************************/
/*!
    \Function _ProtoAdvtCallback

    \Description
        Main worker callback

    \Input *sock - connection socket pointer
    \Input flags - unused
    \Input *_ref - ProtoAdvtRef pointer

    \Output int32_t - zero

    \Version 11/01/02 (GWS)
*/
/*************************************************************************************F*/
static int32_t _ProtoAdvtCallback(SocketT *sock, int32_t flags, void *_ref)
{
    int32_t len;
    int32_t local;
    struct sockaddr base;
    struct sockaddr from;
    ServListT *list, *item, **link;
    ProtoAdvtPacketT packet;
    ProtoAdvtRef *ref = (ProtoAdvtRef *) _ref;
    uint32_t tick;

    // make sure we own resources
    if (!NetCritTry(&ref->crit))
    {
        NetPrintf(("protoadvt: could not acquire critical section\n"));
        return(0);
    }

    // process all pending packets
    while (ref->sock != NULL)
    {
        // request seeding if needed
        if (ref->seed > 0)
        {
            ref->seed = 0;
            _ProtoAdvtRequestSeed(ref);
        }

        // try and receive a packet
        packet.kind[0] = '\0';
        SockaddrInit(&from, AF_INET);
        len = _ProtoAdvtRecvPacket(ref, &packet, &from);
        if ((len <= 0) || (packet.kind[0] == '\0'))
        {
            break;
        }

        // get local address we would have used
        SockaddrInit(&base, AF_INET);
        SocketHost(&base, sizeof(base), &from, sizeof(from));

        // see if packet was sent locally
        local = 0;
        if ((base.sa_family == from.sa_family) && (from.sa_family == AF_INET))
            local = (memcmp(&from.sa_data, &base.sa_data, 6) == 0);

        // ignore packets without proper identifier
        if ((packet.ident[0] != ADVERT_PACKET_IDENTIFIER[0]) ||
            (packet.ident[1] != ADVERT_PACKET_IDENTIFIER[1]) ||
            (packet.ident[2] != ADVERT_PACKET_IDENTIFIER[2]))
        {
            continue;
        }

        // handle seed request first
        if ((packet.kind[0] == '?') && (packet.kind[1] == 0))
        {
            tick = NetTick() + 1000;
            // reduce timeouts for outgoing packets
            for (list = ref->announce; list != NULL; list = list->next)
            {
                // reduce timeout down to 1 second
                // (helps prevent multiple seed requests from causing storm)
                if (list->timeout > tick)
                    list->timeout = tick;
            }
            // get next packet
            continue;
        }

        // make sure name & kind are set
        if (packet.name[0] == 0)
        {
            continue;
        }

        // process the packet
        item = NULL;
        for (list = ref->snoopbuf; list != ref->snoopend; ++list)
        {
            // check for unused block
            if ((item == NULL) && (list->packet.name[0] == 0))
            {
                item = list;
            }
            // see if we already have this packet
            if ((ds_stricmp(packet.kind, list->packet.kind) == 0) &&
                (ds_stricmp(packet.name, list->packet.name) == 0) &&
                (memcmp(packet.seqn, list->packet.seqn, sizeof(packet.seqn)) == 0))
            {
                break;
            }
        }

        // see if this is a new packet
        if ((list == ref->snoopend) && (item != NULL))
        {
            list = item;
            ds_memclr(list, sizeof(*list));
            // populate with kind/name/seqn (never change)
            ds_memcpy_s(list->packet.seqn, sizeof(list->packet.seqn), packet.seqn, sizeof(packet.seqn));
            ds_strnzcpy(list->packet.kind, packet.kind, sizeof(list->packet.kind));
            ds_strnzcpy(list->packet.name, packet.name, sizeof(list->packet.name));
            ds_strnzcpy(list->packet.note, packet.note, sizeof(list->packet.note));
            // indicate snoop buffer changed
            ref->snoop += 1;
            NetPrintf(("protoadvt: added new advert name=%s freq=%d addr=%a\n", list->packet.name, packet.freq, SockaddrInGetAddr(&from)));
        }

        // update the record (assuming we found/allocated)
        if (list != ref->snoopend)
        {
            ds_strnzcpy(list->packet.addr, packet.addr, sizeof(list->packet.addr));
            ds_strnzcpy(list->packet.note, packet.note, sizeof(list->packet.note));
            list->timeout = NetTick()+2*(packet.freq*1000)+1000;
            list->local = local;
            list->inetval = SockaddrInGetAddr(&from);
            list->hostval = SockaddrInGetAddr(&base);
            SockaddrInGetAddrText(&from, list->inetdot, sizeof(list->inetdot));
            // indicate snoop buffer changed
            ref->snoop += 1;
        }
        // loop checks for another packet
    }

    // check for expired services
    tick = NetTick();
    for (list = ref->snoopbuf; list != ref->snoopend; ++list)
    {
        if (list->packet.name[0] != 0)
        {
            // see if anything has expired
            if ((list->packet.addr[0] == 0) || (list->timeout == 0) || (tick > list->timeout))
            {
                NetPrintf(("protoadvt: expiring advert name=%s\n", list->packet.name));
                list->packet.name[0] = 0;
                list->packet.kind[0] = 0;
                // indicate snoop buffer changed
                ref->snoop += 1;
            }
        }
    }

    // check for expired announcements
    for (link = &ref->announce; (*link) != NULL;)
    {
        // see if it has expired
        if ((*link)->timeout == 0)
        {
            item = *link;
            *link = (*link)->next;
            // send out a cancelation notice
            NetPrintf(("protoadvt: canceling announcement name=%s\n", item->packet.name));
            item->packet.addr[0] = '\0';
            // send out notices
            _ProtoAdvtSendPacket(ref, &item->packet);
            // done with the record
            DirtyMemFree(item, PROTOADVT_MEMID, ref->memgroup, ref->memgrpusrdata);
        }
        else
        {
            // move to next item
            link = &(*link)->next;
        }
    }

    // see if anything needs to be sent
    tick = NetTick();
    for (list = ref->announce; list != NULL; list = list->next)
    {
        // see if its time to send
        if (tick > list->timeout)
        {
            // broadcast packet
            _ProtoAdvtSendPacket(ref, &list->packet);
            // update send time
            list->timeout = tick + list->packet.freq*1000;
        }
    }

    // done with update
    NetCritLeave(&ref->crit);
    return(0);
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function ProtoAdvtAnnounce

    \Description
        Advertise a service as available

    \Input *ref  - reference pointer
    \Input *kind - service class (max 32 characters including NUL)
    \Input *name - service name (usually game name, max 32 characters including NUL)
    \Input *note - service note (max 32 characters including NUL)
    \Input *addr - service address list (max 120 characters, including NUL)
    \Input freq  - announcement frequency, in seconds (can be in the range [2...250])

    \Output
        int32_t  - <0 = error, 0=ok

    \Version 11/01/02 (GWS)
*/
/*************************************************************************************F*/
int32_t ProtoAdvtAnnounce(ProtoAdvtRef *ref, const char *kind, const char *name,
                      const char *note, const char *addr, int32_t freq)
{
    ServListT *list;
    ServListT *item;

    // clamp the freqeuncy to valid range
    if (freq == 0)
        freq = 30;
    if (freq < 2)
        freq = 2;
    if (freq > 250)
        freq = 250;

    // validate input strings
    if ((kind == NULL) || (kind[0] == '\0'))
    {
        NetPrintf(("protoadvt: error, invalid kind\n"));
        return(-1);
    }
    if ((name == NULL) || (name[0] == '\0'))
    {
        NetPrintf(("protoadvt: error, invalid name\n"));
        return(-2);
    }
    if (note == NULL)
    {
        NetPrintf(("protoadvt: error, invalid note\n"));
        return(-3);
    }
    if (addr == NULL)
    {
        NetPrintf(("protoadvt: error, invalid addr\n"));
        return(-4);
    }

    // see if service is already listed
    for (list = ref->announce; list != NULL; list = list->next)
    {
        // check for dupl
        if ((ds_stricmp(kind, list->packet.kind) == 0) &&
            (ds_stricmp(name, list->packet.name) == 0))
        {
            // update the addr field if necessary
            if (ds_stricmp(addr, list->packet.addr) != 0)
            {
                // update address & force immediate broadcast of new info
                ds_strnzcpy(list->packet.addr, addr, sizeof(list->packet.addr));
                list->timeout = NetTick()-1;
            }

            // update the note field if necessary
            if (ds_stricmp(note, list->packet.note) != 0)
            {
                // update note & force immediate broadcast of new info
                ds_strnzcpy(list->packet.note, note, sizeof(list->packet.note));
                list->timeout = NetTick()-1;
            }
            // all done
            return(0);
        }
    }

    // build a packet
    item = DirtyMemAlloc(sizeof(*item), PROTOADVT_MEMID, ref->memgroup, ref->memgrpusrdata);
    ds_memclr(item, sizeof(*item));
    item->timeout = NetTick();
    item->packet.ident[0] = ADVERT_PACKET_IDENTIFIER[0];
    item->packet.ident[1] = ADVERT_PACKET_IDENTIFIER[1];
    item->packet.ident[2] = ADVERT_PACKET_IDENTIFIER[2];
    item->packet.freq = (unsigned char) freq;
    item->packet.seqn[0] = (unsigned char)(item->timeout >> 24);
    item->packet.seqn[1] = (unsigned char)(item->timeout >> 16);
    item->packet.seqn[2] = (unsigned char)(item->timeout >> 8);
    item->packet.seqn[3] = (unsigned char)(item->timeout >> 0);
    ds_strnzcpy(item->packet.kind, kind, sizeof(item->packet.kind));
    ds_strnzcpy(item->packet.name, name, sizeof(item->packet.name));
    ds_strnzcpy(item->packet.addr, addr, sizeof(item->packet.addr));
    ds_strnzcpy(item->packet.note, note, sizeof(item->packet.note));
    NetPrintf(("protoadvt: broadcasting new announcement name=%s freq=%d\n", item->packet.name, item->packet.freq));

    // send an immediate copy
    _ProtoAdvtSendPacket(ref, &item->packet);
    // schedule next send in 250ms
    item->timeout = NetTick()+250;

    // make sure we own resources
    NetCritEnter(&ref->crit);
    // add new item to list
    item->next = ref->announce;
    ref->announce = item;
    // release resources
    NetCritLeave(&ref->crit);
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function ProtoAdvtCancel

    \Description
        Cancel server advertisement

    \Input *ref  - reference pointer
    \Input *kind - service kind
    \Input *name - service name (usually game name, NULL=wildcard)

    \Output
        int32_t  - <0=error, 0=ok

    \Version 11/01/02 (GWS)
*/
/*************************************************************************************F*/
int32_t ProtoAdvtCancel(ProtoAdvtRef *ref, const char *kind, const char *name)
{
    ServListT *list;

    // make sure we own resources
    NetCritEnter(&ref->crit);

    // locate the item in the announcement list
    for (list = ref->announce; list != NULL; list = list->next)
    {
        // see if we got a match
        if (((kind == NULL) || (ds_stricmp(kind, list->packet.kind) == 0)) &&
            ((name == NULL) || (ds_stricmp(name, list->packet.name) == 0)))
        {
            // mark as deleted
            list->timeout = 0;
            break;
        }
    }

    // release resources
    NetCritLeave(&ref->crit);

    // was item found?
    return(list == NULL ? -1 : 0);
}


/*F*************************************************************************************/
/*!
    \Function ProtoAdvtQuery

    \Description
        Query for available services

    \Input *ref    - reference pointer
    \Input *kind   - service kind
    \Input *proto  - protocol
    \Input *buffer - target buffer
    \Input buflen  - target length
    \Input local   - if zero, exclude local lists

    \Output
        int32_t     - number of matches

    \Version 11/01/02 (GWS)
*/
/*************************************************************************************F*/
int32_t ProtoAdvtQuery(ProtoAdvtRef *ref, const char *kind, const char *proto,
                   char *buffer, int32_t buflen, int32_t local)
{
    char *s, *t, *u;
    char addr[256] = "";
    char record[512];
    int32_t count = 0;
    ServListT *list;

    // establish min buffer size
    if (buflen < 5)
        return(-1);

    // default to empty buffer
    buffer[0] = 0;

    // see what matching services we know about
    for (list = ref->snoopbuf; list != ref->snoopend; ++list)
    {
        // skip empty entries
        if (list->packet.name[0] == 0)
            continue;
        // see if the kind matches
        if (ds_stricmp(kind, list->packet.kind) != 0)
            continue;
        // exclude locals if not wanted
        if ((!local) && (list->local))
            continue;
        // parse the address choices
        for (s = list->packet.addr; *s != 0;)
        {
            // parse out the address
            for (t = addr; (*s != 0) && (*s != '\t');)
                *t++ = *s++;
            *t++ = 0;
            if (*s == '\t')
                ++s;
            // make sure address looks valid
            if ((strlen(addr) < 5) || (addr[3] != ':'))
                continue;
            // see if this protocol type is wanted by caller
            addr[3] = 0;
            if ((proto[0] != 0) && (strstr(proto, addr) == NULL))
                continue;
            addr[3] = ':';
            // compile data into a record
            ds_strnzcpy(record, list->packet.name, sizeof(record));
            for (t = record; *t != 0; ++t)
                ;
            *t++ = '\t';
            // copy over notes field
            strcpy(t, list->packet.note);
            while (*t != 0)
                ++t;
            *t++ = '\t';
            // append the address
            for (u = addr; *u != 0; ++u)
            {
                // check for inet substitution
                if ((u[0] == '~') && (u[1] == '1'))
                {
                    // make sure inet address is known
                    if (list->inetdot[0] == 0)
                        break;
                    // copy over the address
                    strcpy(t, list->inetdot);
                    while (*t != 0)
                        ++t;
                    ++u;
                    continue;
                }
                // check for ipx substitution
                if ((u[0] == '~') && (u[1] == '2'))
                {
                    // no ipx support
                    break;
                }
                // raw copy
                *t++ = *u;
            }
            // see if copy was canceled (substitution error)
            if (*u != 0)
                continue;
            // terminate the record
            *t++ = '\n';
            *t = 0;
            // see if the record will fit into output buffer
            if (strlen(record)+5 > (unsigned) buflen)
            {
                // indicate an overflow
                *buffer++ = '.';
                *buffer++ = '.';
                *buffer++ = '.';
                *buffer++ = '\n';
                *buffer = 0;
                return(count);
            }
            // append the record to the buffer
            strcpy(buffer, record);
            buffer += (int32_t)strlen(record);
            buflen -= (int32_t)strlen(record);
            ++count;
        }
    }

    // return count of items
    return(count);
}


/*F*************************************************************************************/
/*!
    \Function ProtoAdvtLocate

    \Description
        Locate a specific advertisement and return advertisers address (UDP only)

    \Input *ref   - reference pointer
    \Input *kind  - service kind (NULL=any)
    \Input *name  - service name (usually game name, NULL=wildcard)
    \Input *host  - pointer to buffer for host (our) address, or NULL
    \Input defval - value returned if module is not set

    \Output
        uint32_t  - defval if ref is NULL, else first matching internet address

    \Version 11/01/02 (GWS)
*/
/*************************************************************************************F*/
uint32_t ProtoAdvtLocate(ProtoAdvtRef *ref, const char *kind, const char *name,
                             uint32_t *host, uint32_t defval)
{
    ServListT *list;

    // just return default if module is not set
    if (ref == NULL)
        return(defval);

    // see what matching services we know about
    for (list = ref->snoopbuf; list != ref->snoopend; ++list)
    {
        // make sure service is valid
        if (list->packet.name[0] == 0)
            continue;
        // exclude locals
        if (list->local)
            continue;
        // see if the kind matches
        if ((kind != NULL) && (kind[0] != 0) && (ds_stricmp(kind, list->packet.kind) != 0))
            continue;
        // make sure the name matches
        if ((name != NULL) && (name[0] != 0) && (ds_stricmp(list->packet.name, name) != 0))
            continue;
        // make sure there is an internet address
        if (list->inetval == 0)
            continue;
        // return host (our) address if they want it
        if (host != NULL)
            *host = list->hostval;
        // return the internet address
        defval = list->inetval;
        break;
    }

    // return default value
    return(defval);
}


/*F*************************************************************************************/
/*!
    \Function ProtoAdvtConstruct

    \Description
        Construct an advertising agent

    \Input limit            - size of snoop buffer (min 4)

    \Output
        ProtoAdvtRef *      - construct ref (passed to other routines)

    \Version 11/01/02 (GWS)
*/
/*************************************************************************************F*/
ProtoAdvtRef *ProtoAdvtConstruct(int32_t limit)
{
    SocketT *sock;
    struct sockaddr bindaddr;
    struct sockaddr peeraddr;
    ProtoAdvtRef *ref;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // see if already allocated
    if (g_ref != NULL)
    {
        ++g_count;
        return(g_ref);
    }

    // allocate class storage
    ref = DirtyMemAlloc(sizeof(*ref), PROTOADVT_MEMID, iMemGroup, pMemGroupUserData);
    if (ref == NULL)
        return(NULL);
    ds_memclr(ref, sizeof(*ref));
    ref->memgroup = iMemGroup;
    ref->memgrpusrdata = pMemGroupUserData;

    // allocate snooped advertisement buffer
    if (limit < 4)
        limit = 4;
    ref->snoopbuf = DirtyMemAlloc(limit*sizeof(ref->snoopbuf[0]), PROTOADVT_MEMID, ref->memgroup, ref->memgrpusrdata);
    if (ref->snoopbuf == NULL)
    {
        DirtyMemFree(ref, PROTOADVT_MEMID, ref->memgroup, ref->memgrpusrdata);
        return(NULL);
    }
    ref->snoopend = ref->snoopbuf+limit;
    ds_memclr(ref->snoopbuf, (int32_t)((char *)ref->snoopend-(char *)ref->snoopbuf));

    // set initial revision value
    ref->snoop = 1;

    // create the actual socket
    sock = SocketOpen(AF_INET, SOCK_DGRAM, 0);
    if (sock == NULL)
    {
        DirtyMemFree(ref, PROTOADVT_MEMID, ref->memgroup, ref->memgrpusrdata);
        return(NULL);
    }

    // mark as available for sharing
    g_ref = ref;
    g_count = 1;

    // need to sync access to data
    NetCritInit(&ref->crit, "protoadvt");
    // limit access during setup
    NetCritEnter(&ref->crit);

    // bind with local address
    SockaddrInit(&bindaddr, AF_INET);
    SockaddrInSetPort(&bindaddr, ADVERT_BROADCAST_PORT_UDP);
    SocketBind(sock, &bindaddr, sizeof(bindaddr));

    // connect to remote address
    SockaddrInit(&peeraddr, AF_INET);
    SockaddrInSetPort(&peeraddr, ADVERT_BROADCAST_PORT_UDP);
    SockaddrInSetAddr(&peeraddr, INADDR_BROADCAST);

    // note: though it would be easier to use connect to bind the peer address
    // to the socket, it will not work because winsock will not deliver an
    // incoming packet to a socket connected to the sending port. therefore,
    // the address must be maintained separately and used with sendto.
    // (logic unchanged for dirtysock since it is already coded this way)
    ds_memcpy_s(&ref->addr, sizeof(ref->addr), &peeraddr, sizeof(peeraddr));

    // broadcast request for available server info (seed database)
    ref->seed = 1;
    // make the socket available
    ref->sock = sock;
    // bind the callback function
    SocketCallback(ref->sock, CALLB_RECV, 100, ref, &_ProtoAdvtCallback);

    // done with setup
    NetCritLeave(&ref->crit);

    // immediately send a seed request
    // (the idle process will send a second within 100ms)
    _ProtoAdvtRequestSeed(ref);
    return(ref);
}


/*F*************************************************************************************/
/*!
    \Function ProtoAdvtDestroy

    \Description
        Destruct an advertising agent

    \Input *ref - construct ref

    \Version 11/01/02 (GWS)
*/
/*************************************************************************************F*/
void ProtoAdvtDestroy(ProtoAdvtRef *ref)
{
    SocketT *sock;

    // make sure what is valid
    if (ref == NULL)
    {
        return;
    }

    // see if we are last
    if (g_count > 1)
    {
        --g_count;
        return;
    }

    // destroy the class
    g_ref = NULL;
    g_count = 0;

    // cancel all announcements
    while (ref->announce != NULL)
    {
        ProtoAdvtPacketT *packet = &ref->announce->packet;
        ProtoAdvtCancel(ref, packet->kind, packet->name);
        _ProtoAdvtCallback(ref->sock, 0, ref);
    }

    // make sure we own resources
    NetCritEnter(&ref->crit);

    // do the shutdown
    sock = ref->sock;
    ref->sock = NULL;

    // release resources
    NetCritLeave(&ref->crit);

    // dispose of socket
    SocketClose(sock);

    // done with semaphore
    NetCritKill(&ref->crit);

    // done with ref
    DirtyMemFree(ref->snoopbuf, PROTOADVT_MEMID, ref->memgroup, ref->memgrpusrdata);
    DirtyMemFree(ref, PROTOADVT_MEMID, ref->memgroup, ref->memgrpusrdata);
}

