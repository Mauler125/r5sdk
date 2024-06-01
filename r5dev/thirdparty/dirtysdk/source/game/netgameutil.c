/*H*************************************************************************************************/
/*!

    \File    netgameutil.c

    \Description
        This module provides the setup required to bring peer-peer networking
        online.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2001-2002.  ALL RIGHTS RESERVED.

    \Version    1.0        01/09/01 (GWS) First Version

*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/comm/commall.h"
#include "DirtySDK/comm/commudp.h"
#include "DirtySDK/comm/commsrp.h"
#include "DirtySDK/proto/protoadvt.h"
#include "DirtySDK/game/netgameutil.h"
#include "DirtySDK/game/netgamepkt.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! netgameutil internal state
struct NetGameUtilRefT
{
    //! module memory group
    int32_t memgroup;
    void *memgrpusrdata;

    //! mac->name translation table
    char *table;
    //! class (unique to app)
    char kind[32];
    //! service address list (64-->128 7/25/05 to fix ConnApi overrun GWS)
    char addr[128];

    //! advert ref, for broadcasting hosting info
    ProtoAdvtRef *advt;
    //! advertising ref, for connecting
    ProtoAdvtRef *find;

    //! hosting status: 0=hosting, 1=joining
    int32_t hosting;

    //! host ip
    uint32_t hostip;
    //! host port
    uint32_t hostport;
    //! peer ip
    uint32_t peerip;
    //! peer port
    uint32_t peerport;

    //! socket ref
    SocketT *pSocket;

    //! max packet width
    int32_t maxwid;

    //! size of send buffer in packets
    int32_t maxout;

    //! size of receive buffer in packets
    int32_t maxinp;
    
    //! unacknowledged packet window
    int32_t unacklimit;

    //! advertising frequency, in seconds
    int32_t advtfreq;
    
    //! client identifier (zero == none)
    int32_t clientid;
    
    //! remote client identifier
    int32_t rclientid;
    
    //! metatype
    int32_t metatype;

    //! construct function (used for AUTO mode)
    CommAllConstructT *pConstruct;
    
    //! commref of connection, or NULL if no connection
    CommRef *comm;

    uint8_t uLocalAdvt;
};


/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

// Constants

// Public variables


/*** Private Functions *****************************************************************/


/*F*************************************************************************************/
/*!
    \Function _NetGameUtilAdvtConstruct

    \Description
        Create the ProtoAdvt module.

    \Input *pRef        - pointer to module state
    \Input iSize        - ProtoAdvtConstruct() parameter
    \Input bConnect     - TRUE if constructing advt ref for connecting, FALSE if constructing advt ref for broadcasting hosting info

    \Output
        ProtoAdvtRefT * - pointer to new advertising module

    \Version 10/13/2005 (jbrookes)
*/
/*************************************************************************************F*/
static ProtoAdvtRef *_NetGameUtilAdvtConstruct(NetGameUtilRefT *pRef, int32_t iSize, uint32_t bConnect)
{
    ProtoAdvtRef **ppAdvt = bConnect ? &pRef->find : &pRef->advt;

    if (*ppAdvt == NULL)
    {
        DirtyMemGroupEnter(pRef->memgroup, pRef->memgrpusrdata);
        *ppAdvt = ProtoAdvtConstruct(iSize);
        DirtyMemGroupLeave();
    }
    return(*ppAdvt);
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function   NetGameUtilCreate

    \Description
        Construct the game setup module

    \Output
        NetGameUtilRefT *   - reference pointer

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
NetGameUtilRefT *NetGameUtilCreate(void)
{
    NetGameUtilRefT *pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pRef = DirtyMemAlloc(sizeof(*pRef), NETGAMEUTIL_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netgameutil: unable to allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pRef, sizeof(*pRef));
    pRef->memgroup = iMemGroup;
    pRef->memgrpusrdata = pMemGroupUserData;

    // set default comm buffer parameters
    NetGameUtilControl(pRef, 'mwid', NETGAME_DATAPKT_DEFSIZE);
    NetGameUtilControl(pRef, 'minp', NETGAME_DATABUF_MAXSIZE);
    NetGameUtilControl(pRef, 'mout', NETGAME_DATABUF_MAXSIZE);
    
    // return state
    return(pRef);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilDestroy

    \Description
        Destroy the game setup module

    \Input *ref    - reference pointer

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
void NetGameUtilDestroy(NetGameUtilRefT *ref)
{
    // reset
    NetGameUtilReset(ref);
    // done with local state
    DirtyMemFree(ref, NETGAMEUTIL_MEMID, ref->memgroup, ref->memgrpusrdata);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilReset

    \Description
        Reset the game setup module

    \Input *ref    - reference pointer

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
void NetGameUtilReset(NetGameUtilRefT *ref)
{
    // release comm module
    if (ref->comm != NULL)
    {
        ref->comm->Destroy(ref->comm);
        ref->comm = NULL;
    }
    // kill advertisements
    if (ref->find != NULL)
    {
        ProtoAdvtDestroy(ref->find);
        ref->find = NULL;
    }
    if (ref->advt != NULL)
    {
        ProtoAdvtDestroy(ref->advt);
        ref->advt = NULL;
    }
    
    // clear construct ref, if any
    ref->pConstruct = NULL;
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilControl

    \Description
        Set internal GameUtil parameters.

    \Input *pRef    - reference pointer
    \Input iKind    - selector
    \Input iValue   - value to set

    \Notes
        Selectors:

        \verbatim
            'advf': set advertising frequency, in seconds (call before calling NetGameUtilAdvert)
            'clid': set client identifier* (for use with gameservers)
            'locl': enable or disable query of local adverts
            'meta': set metatype (default=0)
            'minp': set receive buffer size, in packets*
            'mout': set send buffer size, in packets*
            'mwid': set maximum packet width (must be <= NETGAME_DATAPKT_MAXSIZE)*
                * must be set before NetGameLinkConnect() is called to be effective.
        \endverbatim

    \Version    11/12/03 (JLB)
*/
/*************************************************************************************************F*/
void NetGameUtilControl(NetGameUtilRefT *pRef, int32_t iKind, int32_t iValue)
{
    if (iKind == 'clid')
    {
        pRef->clientid = iValue;
        NetPrintf(("netgameutil: setting clid=0x%08x\n", pRef->clientid));
    }
    if (iKind == 'rcid')
    {
        pRef->rclientid = iValue;
        NetPrintf(("netgameutil: setting rcid=0x%08x\n", pRef->rclientid));
    }
    if (iKind == 'locl')
    {
        pRef->uLocalAdvt = iValue;
        NetPrintf(("netgameutil: setting uLocalAdvt=%d\n", pRef->uLocalAdvt));
    }
    if (iKind == 'meta')
    {
        pRef->metatype = iValue;
        NetPrintf(("netgameutil: setting meta=0x%08x\n", pRef->metatype));
    }
    if (iKind == 'mwid')
    {
        if (iValue <= NETGAME_DATAPKT_MAXSIZE)
        {
            pRef->maxwid = iValue+NETGAME_DATAPKT_MAXTAIL;
            NetPrintf(("netgameutil: setting mwid=%d\n", pRef->maxwid));
        }
        else
        {
            NetPrintf(("netgameutil: mwid value of %d is too large\n", iValue));
        }
    }
    if (iKind == 'minp')
    {
        pRef->maxinp = iValue;
        NetPrintf(("netgameutil: setting minp=%d\n", pRef->maxinp));
    }
    if (iKind == 'mout')
    {
        pRef->maxout = iValue;
        NetPrintf(("netgameutil: setting mout=%d\n", pRef->maxout));
    }
    if (iKind == 'ulmt')
    {
        pRef->unacklimit = iValue;
        NetPrintf(("netgameutil: setting unacklimit=%d\n", pRef->unacklimit));
    }

    if (iKind == 'advf')
    {
        pRef->advtfreq = iValue;
        NetPrintf(("netgameutil: setting advf=%d\n", pRef->advtfreq));
    }
    
    // if selector is unhandled, and a comm func is available, pass it on down
    if ((pRef->comm != NULL) && (pRef->comm->Control != NULL))
    {
        pRef->comm->Control(pRef->comm, iKind, iValue, NULL);
    }
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilConnect

    \Description
        Establish a connection (connect/listen)

    \Input *ref         - reference pointer
    \Input conn         - connect mode (NETGAME_CONN_*) | comm type (NETGAME_CONN_*)
    \Input *addr        - service address list
    \Input *pConstruct  - comm construct function

    \Output
        int32_t         - zero=success, negative=failure 

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
int32_t NetGameUtilConnect(NetGameUtilRefT *ref, int32_t conn, const char *addr, CommAllConstructT *pConstruct)
{
    int32_t iErr = 0;

    // make sure user specified connect and/or listen, and a valid protocol
    if (((conn & NETGAME_CONN_AUTO) == 0) || (pConstruct == NULL))
    {
        NetPrintf(("netgameutil: invalid conn param\n"));
        return(-100);   // using hundred range to not conflic with COMM_* error codes that iErr can be assigned with.
    }

    // save the address for later
    ds_strnzcpy(ref->addr, addr, sizeof(ref->addr));
    // save host/join mode
    ref->hosting = ((conn & NETGAME_CONN_CONNECT) ? 1 : 0);

    NetPrintf(("netgameutil: connect %d %s\n", conn, addr));

    // release previous comm module
    if (ref->comm != NULL)
    {
        ref->comm->Destroy(ref->comm);
    }

    // handle auto mode
    if ((conn & NETGAME_CONN_AUTO) == NETGAME_CONN_AUTO)
    {
        // save construct ref
        ref->pConstruct = pConstruct;
        // make sure advertising is running
        if (_NetGameUtilAdvtConstruct(ref, 8, TRUE) == NULL)
        {
            NetPrintf(("netgameutil: NetGameUtilConnect() failed to create the ProtoAdvt module.\n"));
            return(-101);  // using hundred range to not conflic with COMM_* error codes that iErr can be assigned with.
        }
        ProtoAdvtAnnounce(ref->find, "GmUtil", addr, "", "TCP:~1:1024\tUDP:~1:1024", 0);
        return(0);
    }

    // mark modules as created by us with our memgroup
    DirtyMemGroupEnter(ref->memgroup, ref->memgrpusrdata);

    // create comm module
    ref->comm = pConstruct(ref->maxwid, ref->maxinp, ref->maxout);

    // start connect/listen
    if (ref->comm != NULL)
    {
        if (ref->comm->Control != NULL)
        {
            ref->comm->Control(ref->comm, 'clid', ref->clientid, NULL);
            ref->comm->Control(ref->comm, 'rcid', ref->rclientid, NULL);
            ref->comm->Control(ref->comm, 'meta', ref->metatype, NULL);

            if (ref->unacklimit != 0)
            {
                ref->comm->Control(ref->comm, 'ulmt', ref->unacklimit, NULL);
            }
        }
        if (conn & NETGAME_CONN_CONNECT)
        {
            iErr = ref->comm->Connect(ref->comm, addr);
        }
        else if (conn & NETGAME_CONN_LISTEN)
        {
            iErr = ref->comm->Listen(ref->comm, addr);
        }
        ref->pSocket = ref->comm->sockptr;
        // get host ip/port info from commref
        ref->hostip = ref->comm->hostip;
        ref->hostport = ref->comm->hostport;
    }

    DirtyMemGroupLeave();
    return(iErr);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilComplete

    \Description
        Check for connection complete

    \Input *ref    - reference pointer

    \Output
        void *     - connection pointer (NULL is no connection)

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
void *NetGameUtilComplete(NetGameUtilRefT *ref)
{
    // see if we are in find mode
    if (ref->find != NULL)
    {
        // if not connecting, see if we can locate someone
        if (ref->comm == NULL)
        {
            char text[256];
            uint32_t peer, host;
            peer = ProtoAdvtLocate(ref->find, "GmUtil", ref->addr, &host, 0);
            if (peer != 0)
            {
                NetPrintf(("netgameutil: located peer=%08x, host=%08x\n", peer, host));
                if (peer > host)
                {
                    ref->hostip = peer;
                    ref->peerip = host;
                    ds_snzprintf(text, sizeof(text), "%d.%d.%d.%d%s",
                        (unsigned char)(peer>>24), (unsigned char)(peer>>16),
                        (unsigned char)(peer>> 8), (unsigned char)(peer>>0),
                        ref->addr);
                    NetGameUtilConnect(ref, NETGAME_CONN_CONNECT, text, ref->pConstruct);
                }
                else
                {
                    ref->hostip = host;
                    ref->peerip = peer;
                    ds_strnzcpy(text, ref->addr, sizeof(text));
                    NetGameUtilConnect(ref, NETGAME_CONN_LISTEN, text, ref->pConstruct);
                }
            }
        }
    }

    // check for a connect
    if ((ref->comm != NULL) && (ref->comm->Status(ref->comm) == COMM_ONLINE))
    {
        // get peer ip/port info from commref
        ref->peerip = ref->comm->peerip;
        ref->peerport = ref->comm->peerport;

        // stop any advertising
        if (ref->advt != NULL)
        {
            ProtoAdvtDestroy(ref->advt);
            ref->advt = NULL;
        }
        if (ref->find != NULL)
        {
            ProtoAdvtDestroy(ref->find);
            ref->find = NULL;
        }
        
        NetPrintf(("netgameutil: connection complete\n"));
        return(ref->comm);
    }
    else
    {
        return(NULL);
    }
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilStatus

    \Description
        Return status info

    \Input *ref     - reference pointer
    \Input iSelect  - info selector
    \Input *pBuf    - [out] output buffer
    \Input iBufSize - size of output buffer

    \Output
        int32_t     - status info

    \Notes
        iSelect can be one of the following:

        \verbatim
            'host' - TRUE if hosting, else FALSE
            'join' - TRUE if joining, else FALSE
            'hoip' - host ip
            'hprt' - host port
            'peip' - peer ip
            'pprt' - peer port
            'pkrc' - packet received
            'sock' - SocketT socket pointer (in pBuf)
        \endverbatim

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
int32_t NetGameUtilStatus(NetGameUtilRefT *ref, int32_t iSelect, void *pBuf, int32_t iBufSize)
{
    // return host status
    if (iSelect == 'host')
    {
        return(ref->hosting == 0);
    }
    if (iSelect == 'join')
    {
        return(ref->hosting == 1);
    }
    if (iSelect == 'hoip')
    {
        return(ref->hostip);
    }
    if (iSelect == 'hprt')
    {
        return(ref->hostport);
    }
    if (iSelect == 'pkrc')
    {
        return(ref->comm->bpackrcvd);
    }
    if (iSelect == 'peip')
    {
        return(ref->peerip);
    }
    if (iSelect == 'pprt')
    {
        return(ref->peerport);
    }
    if ((iSelect == 'sock') && (iBufSize == (signed)sizeof(ref->pSocket)))
    {
        ds_memcpy(pBuf, &ref->pSocket, sizeof(ref->pSocket));
        return(sizeof(ref->pSocket));
    }
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilAdvert

    \Description
        Send out an advertisement

    \Input *ref     - reference pointer
    \Input *kind    - class (unique to app)
    \Input *name    - name to broadcast
    \Input *note    - notes

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
void NetGameUtilAdvert(NetGameUtilRefT *ref, const char *kind, const char *name, const char *note)
{
    // see if we need to create module
    if (_NetGameUtilAdvtConstruct(ref, 16, FALSE) == NULL)
    {
        NetPrintf(("netgameutil: NetGameUtilAdvert() failed to create the ProtoAdvt module.\n"));
        return;
    }

    // save the kind for future queries
    ds_strnzcpy(ref->kind, kind, sizeof(ref->kind));

    // start advertising
    ProtoAdvtAnnounce(ref->advt, kind, name, note, "TCP:~1:1024\tUDP:~1:1024", ref->advtfreq);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilWithdraw

    \Description
        Withdraw given advertisement

    \Input *ref     - reference pointer
    \Input *kind    - advert kind
    \Input *name    - advert name

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
void NetGameUtilWithdraw(NetGameUtilRefT *ref, const char *kind, const char *name)
{
    if (ref->advt != NULL)
    {
        ProtoAdvtCancel(ref->advt, kind, name);
    }
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilLocate

    \Description
        Find ip address of a specific advertisement

    \Input *ref     - reference pointer
    \Input *kind    - class (unique to app)
    \Input *name    - advertisement to look for

    \Output
        uint32_t    - ip address of advertiser or zero if no match

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
uint32_t NetGameUtilLocate(NetGameUtilRefT *ref, const char *kind, const char *name)
{
    // auto-create the advert module if needed
    if (_NetGameUtilAdvtConstruct(ref, 16, FALSE) == NULL)
    {
        NetPrintf(("netgameutil: NetGameUtilLocate() failed to create the ProtoAdvt module.\n"));
        return(0);
    }

    // allow use of default kind
    if (kind == NULL)
    {
        kind = ref->kind;
    }

    // pass to advertising module
    return(ProtoAdvtLocate(ref->advt, kind, name, NULL, 0));
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilQuery

    \Description
        Return a list of all advertisements

    \Input *ref     - reference pointer
    \Input *kind    - class (unique to app)
    \Input *buf     - target buffer
    \Input max      - target buffer length

    \Output
        int32_t         - number of matching ads

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
int32_t NetGameUtilQuery(NetGameUtilRefT *ref, const char *kind, char *buf, int32_t max)
{
    // auto-create the advert module if needed
    if (_NetGameUtilAdvtConstruct(ref, 16, FALSE) == NULL)
    {
        NetPrintf(("netgameutil: NetGameUtilQuery() failed to create the ProtoAdvt module.\n"));
        return(0);
    }

    // allow use of default kind
    if (kind == NULL)
    {
        kind = ref->kind;
    }

    // pass to advert module
    return(ProtoAdvtQuery(ref->advt, kind, "", buf, max, ref->uLocalAdvt));
}
