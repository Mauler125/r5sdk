/*H********************************************************************************/
/*!
    \File dirtynetunix.c

    \Description
        Provides a wrapper that translates the Unix network interface to the
        DirtySock portable networking interface.

    \Copyright
        Copyright (c) 2010 Electronic Arts Inc.

    \Version 04/05/2010 (jbrookes) First version; a vanilla port to Linux from PS3
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "DirtySDK/platform.h"

#if defined(DIRTYCODE_APPLEIOS) || defined(DIRTYCODE_APPLEOSX)
 #include <ifaddrs.h>
 #include <arpa/inet.h>
 #include <net/if.h>
 #include <net/if_dl.h>
#else
 #include <signal.h>
 #include <net/if.h>             // struct ifreq
 #include <sys/ioctl.h>          // ioctl
#endif

#if defined(DIRTYCODE_APPLEIOS)
 #include <CoreFoundation/CFData.h>
 #include <CoreFoundation/CFStream.h>
 #include <CFNetwork/CFHTTPStream.h>
 #include <CFNetwork/CFHost.h>
#endif

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtythread.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "dirtynetpriv.h"       // private include for dirtynet common functions

/*** Defines **********************************************************************/

#define INVALID_SOCKET      (-1)

#define SOCKET_MAXPOLL      (32)

#define SOCKET_VERBOSE      (DIRTYCODE_DEBUG && FALSE)

/*** Type Definitions *************************************************************/

//! private socketlookup structure containing extra data
typedef struct SocketLookupPrivT
{
    HostentT    Host;      //!< must come first!
} SocketLookupPrivT;

//! dirtysock connection socket structure
struct SocketT
{
    SocketT *pNext;             //!< link to next active
    SocketT *pKill;             //!< link to next killed socket

    int32_t iFamily;            //!< protocol family
    int32_t iType;              //!< protocol type
    int32_t iProto;             //!< protocol ident

    int8_t  iOpened;            //!< negative=error, zero=not open (connecting), positive=open
    uint8_t bImported;          //!< whether socket was imported or not
    uint8_t bVirtual;           //!< if true, socket is virtual
    uint8_t bHasData;           //!< zero if no data, else has data ready to be read
    uint8_t bInCallback;        //!< in a socket callback
    uint8_t uBrokenFlag;        //!< 0 or 1=might not be broken, >1=broken socket
    uint8_t bAsyncRecv;         //!< if true, async recv is enabled
    uint8_t bSendCbs;           //!< TRUE if send cbs are enabled, false otherwise
    int8_t  iVerbose;           //!< debug level
    uint8_t __pad[3];

    int32_t uSocket;            //!< unix socket ref
    int32_t iLastError;         //!< last socket error

    struct sockaddr LocalAddr;  //!< local address
    struct sockaddr RemoteAddr; //!< remote address

    uint16_t uVirtualPort;      //!< virtual port, if set
    uint16_t uPollIdx;          //!< index in blocking poll() operation

    SocketRateT SendRate;       //!< send rate estimation data
    SocketRateT RecvRate;       //!< recv rate estimation data

    int32_t iCallMask;          //!< valid callback events
    uint32_t uCallLast;         //!< last callback tick
    uint32_t uCallIdle;         //!< ticks between idle calls
    void *pCallRef;             //!< reference calback value
    int32_t (*pCallback)(SocketT *pSocket, int32_t iFlags, void *pRef);

    NetCritT RecvCrit;          //!< receive critical section
    int32_t iRecvErr;           //!< last error that occurred
    uint32_t uRecvFlag;         //!< flags from recv operation
    int32_t iRbufSize;          //!< read buffer size (bytes)
    int32_t iSbufSize;          //!< send buffer size (bytes)

    struct sockaddr RecvAddr;   //!< receive address

    struct sockaddr_in6 RecvAddr6;   //!< receive address (ipv6)

    SocketPacketQueueT *pRecvQueue;
    SocketPacketQueueEntryT *pRecvPacket;
};

//! standard ipv4 packet header (see RFC791)
typedef struct HeaderIpv4
{
    uint8_t verslen;      //!< version and length fields (4 bits each)
    uint8_t service;      //!< type of service field
    uint8_t length[2];    //!< total packet length (header+data)
    uint8_t ident[2];     //!< packet sequence number
    uint8_t frag[2];      //!< fragmentation information
    uint8_t time;         //!< time to live (remaining hop count)
    uint8_t proto;        //!< transport protocol number
    uint8_t check[2];     //!< header checksum
    uint8_t srcaddr[4];   //!< source ip address
    uint8_t dstaddr[4];   //!< dest ip address
} HeaderIpv4;

//! local state
typedef struct SocketStateT
{
    SocketT  *pSockList;                //!< master socket list
    SocketT  *pSockKill;                //!< list of killed sockets
    HostentT *pHostList;                //!< list of ongoing name resolution operations

    uint16_t aVirtualPorts[SOCKET_MAXVIRTUALPORTS]; //!< virtual port list

    // module memory group
    int32_t  iMemGroup;                 //!< module mem group id
    void     *pMemGroupUserData;        //!< user data associated with mem group

    uint32_t uConnStatus;               //!< current connection status
    uint32_t uLocalAddr;                //!< local internet address for active interface
    int32_t  iMaxPacket;                //!< maximum packet size

    uint8_t  aMacAddr[6];               //!< MAC address for active interface
    uint8_t  bSingleThreaded;           //!< TRUE if in single-threaded mode
    int8_t   iVerbose;                  //!< debug output verbosity

    volatile int32_t iRecvLife;

    SocketAddrMapT AddrMap;             //!< address map for translating ipv6 addresses to ipv4 virtual addresses and back

    SocketHostnameCacheT *pHostnameCache; //!< hostname cache

    SocketSendCallbackEntryT aSendCbEntries[SOCKET_MAXSENDCALLBACKS]; //!< collection of send callbacks
} SocketStateT;

/*** Variables ********************************************************************/

//! module state ref
static SocketStateT *_Socket_pState = NULL;

#if DIRTYSOCK_ERRORNAMES
//! getaddrinfo() error result table
static const DirtyErrT _GAI_ErrList[] =
{
    DIRTYSOCK_ErrorName(EAI_BADFLAGS),      //  -1; Invalid value for 'ai_flags' field.
    DIRTYSOCK_ErrorName(EAI_NONAME),        //  -2; NAME or SERVICE is unknown.
    DIRTYSOCK_ErrorName(EAI_AGAIN),         //  -3; Temporary failure in name resolution.
    DIRTYSOCK_ErrorName(EAI_FAIL),          //  -4; Non-recoverable failure in name res.
    #if defined(__USE_GNU) || DIRTYCODE_ANDROID
    DIRTYSOCK_ErrorName(EAI_NODATA),        //  -5: No address associated with NAME.
    #endif
    DIRTYSOCK_ErrorName(EAI_FAMILY),        //  -6; 'ai_family' not supported.
    DIRTYSOCK_ErrorName(EAI_SOCKTYPE),      //  -7; 'ai_socktype' not supported.
    DIRTYSOCK_ErrorName(EAI_SERVICE),       //  -8; SERVICE not supported for 'ai_socktype'.
    #if defined(__USE_GNU) || DIRTYCODE_ANDROID
    DIRTYSOCK_ErrorName(EAI_ADDRFAMILY),    //  -9; Address family for NAME not supported.
    #endif
    DIRTYSOCK_ErrorName(EAI_MEMORY),        // -10; Memory allocation failure.
    DIRTYSOCK_ErrorName(EAI_SYSTEM),        // -11; System error returned in `errno'.
    DIRTYSOCK_ErrorName(EAI_OVERFLOW),      // -12; Argument buffer overflow.
    #if DIRTYCODE_LINUX && defined(__USE_GNU)
    DIRTYSOCK_ErrorName(EAI_INPROGRESS),    // -100; Processing request in progress.
    DIRTYSOCK_ErrorName(EAI_CANCELED),      // -101; Request canceled.
    DIRTYSOCK_ErrorName(EAI_NOTCANCELED),   // -102; Request not canceled.
    DIRTYSOCK_ErrorName(EAI_ALLDONE),       // -103; All requests done.
    DIRTYSOCK_ErrorName(EAI_INTR),          // -104; Interrupted by a signal.
    DIRTYSOCK_ErrorName(EAI_IDN_ENCODE),    // -105; IDN encoding failed.
    #endif
    DIRTYSOCK_ListEnd()
};
#endif

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function    _SocketTranslateError2

    \Description
        Translate a BSD error to dirtysock

    \Input iErr     - BSD error code
    \Input iErrno   - errno or override of the value

    \Output
        int32_t     - dirtysock error (SOCKERR_*)

    \Version 06/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _SocketTranslateError2(int32_t iErr, int32_t iErrno)
{
    if (iErr < 0)
    {
        iErr = iErrno;
        if ((iErr == EWOULDBLOCK) || (iErr == EINPROGRESS))
            iErr = SOCKERR_NONE;
        else if (iErr == EHOSTUNREACH)
            iErr = SOCKERR_UNREACH;
        else if (iErr == ENOTCONN)
            iErr = SOCKERR_NOTCONN;
        else if (iErr == ECONNREFUSED)
            iErr = SOCKERR_REFUSED;
        else if (iErr == ECONNRESET)
            iErr = SOCKERR_CONNRESET;
        else if ((iErr == EBADF) || (iErr == EPIPE))
            iErr = SOCKERR_BADPIPE;
        else
            iErr = SOCKERR_OTHER;
    }
    return(iErr);
}

/*F********************************************************************************/
/*!
    \Function    _SocketTranslateError

    \Description
        Translate a BSD error to dirtysock

    \Input iErr     - BSD error code

    \Output
        int32_t     - dirtysock error (SOCKERR_*)

    \Version 06/19/2020 (eesponda)
*/
/********************************************************************************F*/
static int32_t _SocketTranslateError(int32_t iErr)
{
    return(_SocketTranslateError2(iErr, errno));
}

#if defined(DIRTYCODE_LINUX) || defined(DIRTYCODE_APPLEIOS) || defined(DIRTYCODE_ANDROID)
/*F********************************************************************************/
/*!
    \Function    _SocketDisableSigpipe

    \Description
        Disable SIGPIPE signal.

    \Version 12/08/2003 (sbevan)
*/
/********************************************************************************F*/
static void _SocketDisableSigpipe(void)
{
    struct sigaction sa;
    ds_memclr(&sa, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGPIPE, &sa, 0);
}
#endif

/*F********************************************************************************/
/*!
    \Function    _SocketCreateSocket

    \Description
        Create a system level socket.

    \Input iAddrFamily  - address family (AF_INET)
    \Input iType        - socket type (SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, ...)
    \Input iProto       - protocol type for SOCK_RAW (unused by others)

    \Output
        int32_t         - socket handle

    \Version 16/02/2012 (szhu)
*/
/********************************************************************************F*/
static int32_t _SocketCreateSocket(int32_t iAddrFamily, int32_t iType, int32_t iProto)
{
    int32_t iSocket;
    // create socket
    if ((iSocket = socket(iAddrFamily, iType, iProto)) >= 0)
    {
        const uint32_t uTrue = 1, uFalse = 0;
        // if dgram, allow broadcast
        if (iType == SOCK_DGRAM)
        {
            setsockopt(iSocket, SOL_SOCKET, SO_BROADCAST, &uTrue, sizeof(uTrue));
        }
        // if a raw socket, set header include
        if (iType == SOCK_RAW)
        {
            setsockopt(iSocket, IPPROTO_IP, IP_HDRINCL, &uTrue, sizeof(uTrue));
        }
        // set nonblocking operation
        if (fcntl(iSocket, F_SETFL, O_NONBLOCK) < 0)
        {
            NetPrintf(("dirtynetunix: error trying to make socket non-blocking (err=%d)\n", errno));
        }
        // disable IPV6 only
        setsockopt(iSocket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&uFalse, sizeof(uFalse));

        #if defined(DIRTYCODE_APPLEIOS) || defined(DIRTYCODE_APPLEOSX)
        // ignore SIGPIPE; we ignore this signal globally, but that doesn't work on iOS/MacX
        if (setsockopt(iSocket, SOL_SOCKET, SO_NOSIGPIPE, &uTrue, sizeof(uTrue)) < 0)
        {
            NetPrintf(("dirtynetunix: unable to set NOSIGPIPE on socket (err=%s)\n", DirtyErrGetName(errno)));
        }
        #endif
    }
    else
    {
        NetPrintf(("dirtynetunix: socket() failed (err=%s)\n", DirtyErrGetName(errno)));
    }
    return(iSocket);
}

/*F********************************************************************************/
/*!
    \Function    _SocketOpen

    \Description
        Create a new transfer endpoint. A socket endpoint is required for any
        data transfer operation.  If iSocket != -1 then used existing socket.

    \Input iSocket      - Socket descriptor to use or -1 to create new
    \Input iAddrFamily  - address family (AF_INET)
    \Input iType        - socket type (SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, ...)
    \Input iProto       - protocol type for SOCK_RAW (unused by others)
    \Input iOpened      - 0=not open (connecting), 1=open

    \Output
        SocketT *       - socket reference

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static SocketT *_SocketOpen(int32_t iSocket, int32_t iAddrFamily, int32_t iType, int32_t iProto, int32_t iOpened)
{
    SocketStateT *pState = _Socket_pState;
    const int32_t iQueueSize = (iType != SOCK_STREAM) ? 1 : 8;
    SocketT *pSocket;

    // allocate memory
    if ((pSocket = DirtyMemAlloc(sizeof(*pSocket), SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetunix: unable to allocate memory for socket\n"));
        return(NULL);
    }
    ds_memclr(pSocket, sizeof(*pSocket));

    // open a socket (force to AF_INET6 if we are opening it)
    if ((iSocket == -1) && ((iSocket = _SocketCreateSocket(iAddrFamily = AF_INET6, iType, iProto)) < 0))
    {
        NetPrintf(("dirtynetunix: error %s creating socket\n", DirtyErrGetName(iSocket)));
        DirtyMemFree(pSocket, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(NULL);
    }

    // create packet queue
    if ((pSocket->pRecvQueue = SocketPacketQueueCreate(iQueueSize, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetunix: failed to create socket queue for socket\n"));
        close(iSocket);
        DirtyMemFree(pSocket, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(NULL);
    }

    // set family/proto info
    pSocket->iFamily = iAddrFamily;
    pSocket->iType = iType;
    pSocket->iProto = iProto;
    pSocket->uSocket = iSocket;
    pSocket->iOpened = iOpened;
    pSocket->iLastError = SOCKERR_NONE;
    pSocket->bAsyncRecv = ((pState->bSingleThreaded == FALSE) && ((iType == SOCK_DGRAM) || (iType == SOCK_RAW))) ? TRUE : FALSE;
    pSocket->bSendCbs = TRUE;
    pSocket->iVerbose = 1;

    // inititalize critical section
    NetCritInit(&pSocket->RecvCrit, "inet-recv");
    
    // install into list
    NetCritEnter(NULL);
    pSocket->pNext = pState->pSockList;
    pState->pSockList = pSocket;
    NetCritLeave(NULL);

    // return the socket
    return(pSocket);
}

/*F********************************************************************************/
/*!
    \Function    _SocketReopen

    \Description
        Recreate a socket endpoint.

    \Input *pSocket     - socket ref

    \Output
        SocketT *       - socket ref on success, NULL for error

    \Notes
        This function should not be called from the async/idle thread,
        so socket recreation cannot happen in SocketRecvfrom.

    \Version 16/02/2012 (szhu)
*/
/********************************************************************************F*/
static SocketT *_SocketReopen(SocketT *pSocket)
{
    // we need recvcrit to prevent the socket from being used by the async/idle thread
    // for non-virtual non-tcp sockets only
    if (!pSocket->bVirtual && ((pSocket->iType == SOCK_DGRAM) || (pSocket->iType == SOCK_RAW)))
    {
        SocketT *pResult = NULL;
        // acquire socket receive critical section
        NetCritEnter(&pSocket->RecvCrit);
        NetPrintf(("dirtynetunix: trying to recreate the socket handle for %x->%d\n", pSocket, pSocket->uSocket));

        // close existing socket handle
        if (pSocket->uSocket != INVALID_SOCKET)
        {
            close(pSocket->uSocket);
            pSocket->uSocket = INVALID_SOCKET;
        }

        // create socket
        if ((pSocket->uSocket = _SocketCreateSocket(pSocket->iFamily, pSocket->iType, pSocket->iProto)) >= 0)
        {
            // set socket buffer size
            if (pSocket->iRbufSize > 0)
            {
                SocketControl(pSocket, 'rbuf', pSocket->iRbufSize, NULL, 0);
            }
            if (pSocket->iSbufSize > 0)
            {
                SocketControl(pSocket, 'sbuf', pSocket->iSbufSize, NULL, 0);
            }
            // bind if previous socket was bound to a specific port
            if (SockaddrInGetPort(&pSocket->LocalAddr) != 0)
            {
                int32_t iResult;
                // always set reuseaddr flag for socket recreation
                SocketControl(pSocket, 'radr', 1, NULL, 0);
                // we don't call SocketBind() here (to avoid virtual socket translation)
                if ((iResult = bind(pSocket->uSocket, &pSocket->LocalAddr, sizeof(pSocket->LocalAddr))) < 0)
                {
                    pSocket->iLastError = _SocketTranslateError(iResult);
                    NetPrintf(("dirtynetunix: bind() to %a:%d failed (err=%s)\n",
                               SockaddrInGetAddr(&pSocket->LocalAddr),
                               SockaddrInGetPort(&pSocket->LocalAddr),
                               DirtyErrGetName(errno)));
                }
            }
            // connect if previous socket was connected to a specific remote
            if (SockaddrInGetPort(&pSocket->RemoteAddr) != 0)
            {
                struct sockaddr SockAddr;
                // make a copy of remote
                ds_memcpy_s(&SockAddr, sizeof(SockAddr), &pSocket->RemoteAddr, sizeof(pSocket->RemoteAddr));
                SocketConnect(pSocket, &SockAddr, sizeof(SockAddr));
            }
            // success
            pSocket->uBrokenFlag = 0;
            pResult = pSocket;
            NetPrintf(("dirtynetunix: socket recreation (%x->%d) succeeded.\n", pSocket, pSocket->uSocket));
        }
        else
        {
            pSocket->iLastError = _SocketTranslateError(pSocket->uSocket);
            NetPrintf(("dirtynetunix: socket recreation (%x) failed.\n", pSocket));
        }

        // release socket receive critical section
        NetCritLeave(&pSocket->RecvCrit);
        return(pResult);
    }
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function _SocketClose

    \Description
        Disposes of a SocketT, including disposal of the SocketT allocated memory.  Does
        NOT dispose of the unix socket ref.

    \Input *pSocket - socket to close

    \Output
        int32_t     - negative=error, else zero

    \Version 06/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _SocketClose(SocketT *pSocket)
{
    SocketStateT *pState = _Socket_pState;
    uint32_t bSockInList;
    SocketT **ppSocket;

    // remove sock from linked list
    NetCritEnter(NULL);
    for (ppSocket = &pState->pSockList, bSockInList = FALSE; *ppSocket != NULL; ppSocket = &(*ppSocket)->pNext)
    {
        if (*ppSocket == pSocket)
        {
            *ppSocket = pSocket->pNext;
            bSockInList = TRUE;
            break;
        }
    }
    NetCritLeave(NULL);

    // make sure the socket is in the socket list (and therefore valid)
    if (bSockInList == FALSE)
    {
        NetPrintf(("dirtynetunix: warning, trying to close socket 0x%08x that is not in the socket list\n", (intptr_t)pSocket));
        return(-1);
    }

    // finish any idle call
    NetIdleDone();

    // mark as closed
    pSocket->uSocket = INVALID_SOCKET;
    pSocket->iOpened = FALSE;

    // kill critical section
    NetCritKill(&pSocket->RecvCrit);

    // destroy packet queue
    if (pSocket->pRecvQueue != NULL)
    {
        SocketPacketQueueDestroy(pSocket->pRecvQueue);
    }

    // put into killed list
    NetCritEnter(NULL);
    pSocket->pKill = pState->pSockKill;
    pState->pSockKill = pSocket;
    NetCritLeave(NULL);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function    _SocketIdle

    \Description
        Call idle processing code to give connections time.

    \Input *pData   - pointer to socket state

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _SocketIdle(void *pData)
{
    SocketStateT *pState = (SocketStateT *)pData;
    SocketT *pSocket;
    uint32_t uTick;

    // for access to g_socklist and g_sockkill
    NetCritEnter(NULL);

    // get current tick
    uTick = NetTick();

    // walk socket list and perform any callbacks
    for (pSocket = pState->pSockList; pSocket != NULL; pSocket = pSocket->pNext)
    {
        // see if we should do callback
        if ((pSocket->uCallIdle != 0) &&
            (pSocket->pCallback != NULL) &&
            (!pSocket->bInCallback) &&
            (NetTickDiff(uTick, pSocket->uCallLast) > (signed)pSocket->uCallIdle))
        {
            pSocket->bInCallback = TRUE;
            (pSocket->pCallback)(pSocket, 0, pSocket->pCallRef);
            pSocket->bInCallback = FALSE;
            pSocket->uCallLast = uTick = NetTick();
        }
    }

    // delete any killed sockets
    while ((pSocket = pState->pSockKill) != NULL)
    {
        pState->pSockKill = pSocket->pKill;
        DirtyMemFree(pSocket, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }

    // process dns cache list, delete expired entries
    SocketHostnameCacheProcess(pState->pHostnameCache, pState->iVerbose);

    // process hostname list, delete completed lookup requests
    SocketHostnameListProcess(&pState->pHostList, pState->iMemGroup, pState->pMemGroupUserData);

    // for access to g_socklist and g_sockkill
    NetCritLeave(NULL);
}

/*F*************************************************************************************/
/*!
    \Function    _SocketRecvfromPacketQueue

    \Description
        Check if there is a pending inbound packet in the socket packet queue.

    \Input *pSocket     - pointer to socket
    \Input *pBuf        - [out] buffer to receive data
    \Input iLen         - length of recv buffer
    \Input *pFrom       - [out] address data was received from (NULL=ignore)
    \Input *pFromLen    - [out] length of address

    \Output
        int32_t         - size of packet extracted from queue, 0 if no packet

    \Version 04/20/2016 (mclouatre)
*/
/************************************************************************************F*/
static int32_t _SocketRecvfromPacketQueue(SocketT *pSocket, const char *pBuf, int32_t iLen, struct sockaddr *pFrom, int32_t *pFromLen)
{
    int32_t iResult = 0;

    // make sure destination buffer is valid
    if ((iLen > 0) && (pBuf != NULL))
    {
        // get a packet
        if (pSocket->iType != SOCK_STREAM)
        {
            iResult = SocketPacketQueueRem(pSocket->pRecvQueue, (uint8_t *)pBuf, iLen, &pSocket->RecvAddr);
        }
        else
        {
            iResult = SocketPacketQueueRemStream(pSocket->pRecvQueue, (uint8_t *)pBuf, iLen);
        }

        if (iResult > 0)
        {
            if (pFrom != NULL)
            {
                ds_memcpy_s(pFrom, sizeof(*pFrom), &pSocket->RecvAddr, sizeof(pSocket->RecvAddr));
                *pFromLen = sizeof(*pFrom);
            }
        }
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function    _SocketPoll

    \Description
        Execute a blocking poll on input of all sockets.

    \Input pState       - pointer to module state
    \Input uPollNsec    - maximum number of nanoseconds to block in poll

    \Output
        int32_t         - result of the ppoll call

    \Version 04/02/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _SocketPoll(SocketStateT *pState, uint32_t uPollNsec)
{
    struct pollfd aPollFds[1024];
    #if defined(DIRTYCODE_APPLEOSX) || defined(DIRTYCODE_APPLEIOS)
    int32_t iPollTime = (int32_t)(uPollNsec/1000000);
    #else
    struct timespec PollTime = { .tv_sec = uPollNsec/1000000000, .tv_nsec = uPollNsec%1000000000 };
    #endif
    int32_t iPoll, iMaxPoll, iResult;
    SocketT *pSocket;

    // for access to socket list
    NetCritEnter(NULL);

    // walk socket list and find matching socket
    for (pSocket = pState->pSockList, iPoll = 0, iMaxPoll = 1024; (pSocket != NULL) && (iPoll < iMaxPoll); pSocket = pSocket->pNext)
    {
        // skip invalid sockets or sockets we have an error on
        if ((pSocket->uSocket == INVALID_SOCKET) || (pSocket->bHasData & 0x80))
        {
            pSocket->uPollIdx = iMaxPoll;   // mark the socket as 'not in the poll array'
            continue;
        }

        // add socket to poll array
        aPollFds[iPoll].fd = pSocket->uSocket;
        aPollFds[iPoll].events = POLLIN;
        aPollFds[iPoll].revents = 0;

        // remember poll index
        pSocket->uPollIdx = iPoll++;
    }

    // release critical section
    NetCritLeave(NULL);

    // execute the poll
    #if defined(DIRTYCODE_APPLEOSX) || defined(DIRTYCODE_APPLEIOS)
    iResult = poll(aPollFds, iPoll, iPollTime);
    #else
    iResult = ppoll(aPollFds, iPoll, &PollTime, NULL);
    #endif

    // if any sockets have pending data, figure out which ones
    if (iResult > 0)
    {
        uint32_t uCurTick;

        // re-acquire critical section
        NetCritEnter(NULL);

        // update sockets if there is data to be read or not
        for (pSocket = pState->pSockList, uCurTick = NetTick(); pSocket != NULL; pSocket = pSocket->pNext)
        {
            /* skip socket if it was not in poll array submitted to poll()/ppoll()
               only sockets explicitely added to the poll array have uPollIdx in the valid range [0, iMaxPoll[ */
            if (pSocket->uPollIdx >= iMaxPoll)
            {
                continue;
            }

            pSocket->bHasData += aPollFds[pSocket->uPollIdx].revents & POLLIN ? 1 : 0;
            if (pSocket->bHasData
                && !pSocket->bInCallback
                && (pSocket->pCallback != NULL)
                && (pSocket->iCallMask & CALLB_RECV))
            {
                pSocket->bInCallback = TRUE;
                pSocket->pCallback(pSocket, 0, pSocket->pCallRef);
                pSocket->bInCallback = FALSE;
                pSocket->uCallLast = uCurTick;
            }

            // if we have a socket error, remove from future poll events
            if (aPollFds[pSocket->uPollIdx].revents & (POLLERR|POLLHUP))
            {
                pSocket->bHasData |= 0x80;
            }
        }

        // release the critical section
        NetCritLeave(NULL);
    }
    else if (iResult < 0)
    {
        NetPrintf(("dirtynetunix: poll() failed (err=%s)\n", DirtyErrGetName(errno)));
    }

    // return number of file descriptors with pending data
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _SocketLookupDone

    \Description
        Callback to determine if gethostbyname is complete.

    \Input *pHost   - pointer to host lookup record

    \Output
        int32_t     - zero=in progess, neg=done w/error, pos=done w/success

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _SocketLookupDone(HostentT *pHost)
{
    // return current status
    return(pHost->done);
}

/*F********************************************************************************/
/*!
    \Function _SocketLookupFree

    \Description
        Release resources used by SocketLookup()

    \Input *pHost   - pointer to host lookup record

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _SocketLookupFree(HostentT *pHost)
{
    // release resource
    pHost->refcount -= 1;
}

/*F********************************************************************************/
/*!
    \Function    _SocketLookupThread

    \Description
        Socket lookup thread

    \Input *_pRef       - thread argument (hostent record)

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _SocketLookupThread(void *_pRef)
{
    SocketStateT *pState = _Socket_pState;
    HostentT *pHost = (HostentT *)_pRef;
    int32_t iResult;
    struct addrinfo Hints, *pList;
    int32_t iPreferredFamily;
    char strThreadId[32];

    // if state is null the module has been shut down
    if (pState == NULL)
    {
        return;
    }

    // get the thread id
    DirtyThreadGetThreadId(strThreadId, sizeof(strThreadId));

    // setup lookup hints
    ds_memclr(&Hints, sizeof(Hints));
    Hints.ai_family = AF_UNSPEC;
    Hints.ai_socktype = SOCK_STREAM; // set specific socktype to limit to one result per type
    Hints.ai_protocol = IPPROTO_TCP; // set specific proto to limit to one result per type
    Hints.ai_flags = AI_ADDRCONFIG;

    #if !defined(DIRTYCODE_ANDROID)
    Hints.ai_flags |= AI_V4MAPPED;
    #endif

    #if defined(DIRTYCODE_APPLEIOS) || defined(DIRTYCODE_APPLEOSX)
    iPreferredFamily = AF_INET6;
    #else
    iPreferredFamily = AF_INET;
    #endif

    // start lookup
    NetPrintf(("dirtynetunix: lookup thread start; name=%s (thid=%s)\n", pHost->name, strThreadId));
    if ((iResult = getaddrinfo(pHost->name, NULL, &Hints, &pList)) == 0)
    {
        struct addrinfo *pAddrInfo;

        // first loop we look for addresses matching our preferred address family
        for (pAddrInfo = pList; pAddrInfo != NULL; pAddrInfo = pAddrInfo->ai_next)
        {
            // verbose logging of address info if spam settings warrant
            NetPrintfVerbose((pState->iVerbose, 1, "dirtynetunix: addr=%A\n", pAddrInfo->ai_addr));
            NetPrintfVerbose((pState->iVerbose, 2, "dirtynetunix:   ai_flags=0x%08x\n", pAddrInfo->ai_flags));
            NetPrintfVerbose((pState->iVerbose, 2, "dirtynetunix:   ai_family=%d\n", pAddrInfo->ai_family));
            NetPrintfVerbose((pState->iVerbose, 2, "dirtynetunix:   ai_socktype=%d\n", pAddrInfo->ai_socktype));
            NetPrintfVerbose((pState->iVerbose, 2, "dirtynetunix:   ai_protocol=%d\n", pAddrInfo->ai_protocol));
            NetPrintfVerbose((pState->iVerbose, 2, "dirtynetunix:   name=%s\n", pAddrInfo->ai_canonname));

            // extract first IPv6 address we come across
            if ((pHost->addr == 0) && (pAddrInfo->ai_family == iPreferredFamily))
            {
                pHost->addr = SocketAddrMapAddress(&pState->AddrMap, pAddrInfo->ai_addr, (int32_t)pAddrInfo->ai_addrlen);
            }
        }

        // if we haven't found one yet, look for any address, and pick the first one we come across
        for (pAddrInfo = pList; (pAddrInfo != NULL) && (pHost->addr == 0); pAddrInfo = pAddrInfo->ai_next)
        {
            pHost->addr = SocketAddrMapAddress(&pState->AddrMap, pAddrInfo->ai_addr, (int32_t)pAddrInfo->ai_addrlen);
        }

        // print selected address
        NetPrintf(("dirtynetunix: %s=%a\n", pHost->name, pHost->addr));

        // mark success
        pHost->done = 1;

        // add hostname to cache
        SocketHostnameCacheAdd(pState->pHostnameCache, pHost->name, pHost->addr, pState->iVerbose);

       // release memory
       freeaddrinfo(pList);
    }
    else
    {
        // unsuccessful
        NetPrintf(("dirtynetunix: getaddrinfo('%s', ...) failed err=%s\n", pHost->name, DirtyErrGetNameList(iResult, _GAI_ErrList)));
        pHost->done = -1;
    }

    // note thread completion
    pHost->thread = 1;

    NetPrintf(("dirtynetunix: lookup thread exit; name=%s (thid=%s)\n", pHost->name, strThreadId));

    // release thread-allocated refcount on hostname resource
    pHost->refcount -= 1;
}

/*F********************************************************************************/
/*!
    \Function    _SocketRecvfrom

    \Description
        Receive data from a remote host on a datagram socket.

    \Input *pSocket     - socket reference
    \Input *pBuf        - buffer to receive data
    \Input iLen         - length of recv buffer
    \Input *pFrom       - address data was received from (NULL=ignore)
    \Input *pFromLen    - length of address

    \Output
        int32_t         - positive=data bytes received, else error

    \Version 09/10/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _SocketRecvfrom(SocketT *pSocket, char *pBuf, int32_t iLen, struct sockaddr *pFrom, int32_t *pFromLen)
{
    int32_t iResult;

    // make sure socket ref is valid
    if (pSocket->uSocket == INVALID_SOCKET)
    {
        pSocket->iLastError = SOCKERR_INVALID;
        return(pSocket->iLastError);
    }

    if (pFrom != NULL)
    {
        // do the receive
        struct sockaddr_in6 SockAddr6;
        SockaddrInit6(&SockAddr6, AF_INET6);
        *pFromLen = sizeof(SockAddr6);
        SockaddrInit(pFrom, AF_INET);
        if ((iResult = (int32_t)recvfrom(pSocket->uSocket, pBuf, iLen, 0, (struct sockaddr *)&SockAddr6, (socklen_t *)pFromLen)) > 0)
        {
            SocketAddrMapTranslate(&_Socket_pState->AddrMap, pFrom, (struct sockaddr *)&SockAddr6, pFromLen);
            SockaddrInSetMisc(pFrom, NetTick());
        }
    }
    else
    {
        iResult = (int32_t)recv(pSocket->uSocket, pBuf, iLen, 0);
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function    _SocketRecvToPacketQueue

    \Description
        Attempt to receive data from the given socket and to push it directly
        in the packet queue.

    \Input *pState  - pointer to module state
    \Input *pSocket - pointer to socket to read from

    \Output
        int32_t     - receive result

    \Version 10/21/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _SocketRecvToPacketQueue(SocketStateT *pState, SocketT *pSocket)
{
    int32_t iRecvResult;

    // early exit if packet queue is full or this is a virtual socket
    if (SocketPacketQueueStatus(pSocket->pRecvQueue, 'pful') || (pSocket->bVirtual == TRUE))
    {
        return;
    }

    // get a packet queue entry to receive into
    pSocket->pRecvPacket = SocketPacketQueueAlloc(pSocket->pRecvQueue);

    // try and receive some data
    if ((pSocket->iType == SOCK_DGRAM) || (pSocket->iType == SOCK_RAW))
    {
        int32_t iFromLen = sizeof(pSocket->RecvAddr);
        iRecvResult = _SocketRecvfrom(pSocket, (char *)pSocket->pRecvPacket->aPacketData, sizeof(pSocket->pRecvPacket->aPacketData), &pSocket->RecvAddr, &iFromLen);
    }
    else
    {
        iRecvResult = _SocketRecvfrom(pSocket, (char *)pSocket->pRecvPacket->aPacketData, sizeof(pSocket->pRecvPacket->aPacketData), NULL, 0);
    }

    // if the read completed successfully, save the originator address, packet size and reception time; forward data to socket callback if needed
    if (iRecvResult > 0)
    {
        pSocket->pRecvPacket->iPacketSize = iRecvResult;
        pSocket->pRecvPacket->uPacketTick = NetTick();
        ds_memcpy_s(&pSocket->pRecvPacket->PacketAddr, sizeof(pSocket->pRecvPacket->PacketAddr), &pSocket->RecvAddr, sizeof(pSocket->RecvAddr));

        // see if we should issue callback
        if ((pSocket->uCallLast != (unsigned)-1) && (pSocket->pCallback != NULL) && (pSocket->iCallMask & CALLB_RECV))
        {
            pSocket->uCallLast = (unsigned)-1;
            (pSocket->pCallback)(pSocket, 0, pSocket->pCallRef);
            pSocket->uCallLast = NetTick();
        }
    }
    else
    {
        if (errno != EAGAIN)
        {
            // if we are using a TCP socket and we didn't receive positive bytes, we are closed
            if ((pSocket->iType == SOCK_STREAM) && (iRecvResult <= 0))
            {
                NetPrintfVerbose((pSocket->iVerbose, 0, "dirtynetunix: [%p] connection %s\n", pSocket, (iRecvResult == 0) ? "closed" : "failed"));
                pSocket->iOpened = -1;
            }
            else
            {
                NetPrintf(("dirtynetunix: [%p] _SocketRecvfrom() to packet queue returned %d (err=%s)\n", pSocket, iRecvResult, DirtyErrGetName(errno)));
            }
        }

        // clean up resources that were reserved for the receive operation
        SocketPacketQueueAllocUndo(pSocket->pRecvQueue);
    }
}

/*F********************************************************************************/
/*!
    \Function    _SocketRecvThread

    \Description
        Wait for incoming data and deliver it immediately to the socket callback,
        if registered.

    \Input  *pArg    - pointer to Socket module state

    \Version 10/21/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _SocketRecvThread(void *pArg)
{
    typedef struct PollListT
    {
        SocketT *aSockets[SOCKET_MAXPOLL];
        struct pollfd aPollFds[SOCKET_MAXPOLL];
        int32_t iCount;
    } PollListT;

    PollListT pollList, previousPollList;
    SocketT *pSocket;
    int32_t iListIndex, iResult;
    SocketStateT *pState = (SocketStateT *)pArg;
    char strThreadId[32];

    // get the thread id
    DirtyThreadGetThreadId(strThreadId, sizeof(strThreadId));

    // show we are alive
    pState->iRecvLife = 1;
    NetPrintf(("dirtynetunix: recv thread running (thid=%s)\n", strThreadId));

    // reset contents of pollList
    ds_memclr(&pollList, sizeof(pollList));

    // loop until done
    while(pState->iRecvLife == 1)
    {
        // reset contents of previousPollList
        ds_memclr(&previousPollList, sizeof(previousPollList));

        // make a copy of the poll list used for the last poll() call
        for (iListIndex = 0; iListIndex < pollList.iCount; iListIndex++)
        {
            // copy entry from pollList to previousPollList
            previousPollList.aSockets[iListIndex] = pollList.aSockets[iListIndex];
            previousPollList.aPollFds[iListIndex] = pollList.aPollFds[iListIndex];
        }
        previousPollList.iCount = pollList.iCount;

        // reset contents of pollList in preparation for the next poll() call
        ds_memclr(&pollList, sizeof(pollList));

        // acquire global critical section for access to socket list
        NetCritEnter(NULL);

        // walk the socket list and do two things:
        //    1- if the socket is ready for reading, perform the read operation
        //    2- if the buffer in which inbound data is saved is empty, initiate a new low-level read operation for that socket
        for (pSocket = pState->pSockList; (pSocket != NULL) && (pollList.iCount < SOCKET_MAXPOLL); pSocket = pSocket->pNext)
        {
            // only handle non-virtual sockets with asyncrecv enabled
            if ((pSocket->bVirtual == FALSE) && (pSocket->uSocket != INVALID_SOCKET) && (pSocket->bAsyncRecv == TRUE))
            {
                // acquire socket critical section
                NetCritEnter(&pSocket->RecvCrit);

                // was this socket in the poll list of the previous poll() call
                for (iListIndex = 0; iListIndex < previousPollList.iCount; iListIndex++)
                {
                    if (previousPollList.aSockets[iListIndex] == pSocket)
                    {
                        // socket was in previous poll list!
                        // now check if poll() notified that this socket is ready for reading
                        if (previousPollList.aPollFds[iListIndex].revents & POLLIN)
                        {
                            /*
                            Note:
                            The poll() doc states that some error codes returned by the function
                            may only apply to one of the sockets in the poll list. For this reason,
                            we check the polling result for all entries in the list regardless
                            of the return value of poll().
                            */

                            // ready for reading, so go ahead and read
                            _SocketRecvToPacketQueue(pState, previousPollList.aSockets[iListIndex]);
                        }
                        /*
                            POLLNVAL: The file descriptor is not open. we need to exclude this socket
                            handle from being added to the poll list, otherwise the poll() will keep
                            returning 1 at once.

                            Due to the race-condition (the main thread might have recreated the socket
                            before we reach here), 'brokenflag' is accumulated rather simply set to TRUE, and
                            the socket will be excluded from the poll list only if it's marked as broken
                            twice or more ('brokenflag' is reset to 0 when recreating the socket, so actually
                            the max possible value of 'brokenflag' is 2).
                        */
                        else if (previousPollList.aPollFds[iListIndex].revents & POLLNVAL)
                        {
                            NetPrintf(("dirtynetunix: marking socket (%x->%d) as broken upon POLLNVAL\n", pSocket, pSocket->uSocket));
                            pSocket->uBrokenFlag++;
                        }
                        break;
                    }
                }

                /* if the socket is not virtual, the socket is open (TCP) and if there is room in the recv queue,
                   then add this socket to the poll list to be used by the next poll() call */
                if (!SocketPacketQueueStatus(pSocket->pRecvQueue, 'pful') && (pSocket->uSocket != INVALID_SOCKET) && (pSocket->uBrokenFlag <= 1) && ((pSocket->iType != SOCK_STREAM) || (pSocket->iOpened > 0)))
                {
                    // add socket to poll list
                    pollList.aSockets[pollList.iCount] = pSocket;
                    pollList.aPollFds[pollList.iCount].fd = pSocket->uSocket;
                    pollList.aPollFds[pollList.iCount].events = POLLIN;
                    pollList.iCount += 1;
                }

                // release socket critical section
                NetCritLeave(&pSocket->RecvCrit);
            }
        }

        // release global critical section
        NetCritLeave(NULL);

        // any sockets?
        if (pollList.iCount > 0)
        {
            // poll for data (wait up to 50ms)
            iResult = poll(pollList.aPollFds, pollList.iCount, 50);

            if (iResult < 0)
            {
                NetPrintf(("dirtynetunix: poll() failed (err=%s)\n", DirtyErrGetName(errno)));

                // stall for 50ms because experiment shows that next call to poll() may not block
                // internally if a socket is alreay in error.
                usleep(50*1000);
            }
        }
        else
        {
            // no sockets, so stall for 50ms
            usleep(50*1000);
        }
    }

    // indicate we are done
    NetPrintf(("dirtynetunix: receive thread exit\n"));
    pState->iRecvLife = 0;
}

/*F********************************************************************************/
/*!
    \Function    _SocketGetMacAddress

    \Description
        Attempt to retreive MAC address of the system.

    \Input *pState  - pointer to module state

    \Output
        uint8_t     - TRUE if MAC address found, FALSE otherwise

    \Notes
        Usage of getifaddrs() is preferred over usage of ioctl() with a socket to save
        the socket creation step. However, not all platforms support the AF_LINK address
        family. In those cases, usage of ioctl() can't be avoided.

    \Version 05/12/2004 (mclouatre)
*/
/********************************************************************************F*/
static uint8_t _SocketGetMacAddress(SocketStateT *pState)
{
    int32_t iResult;
    uint8_t bFound =  FALSE;

#if defined(DIRTYCODE_APPLEIOS) || defined(DIRTYCODE_APPLEOSX)
    struct ifaddrs* pIntfList = NULL;
    struct ifaddrs* pIntf = NULL;

    // retrieve the current interfaces - returns 0 on success
    iResult = getifaddrs(&pIntfList);
    if (iResult == 0)
    {
        // loop through linked list of interfaces
        pIntf = pIntfList;
        while(pIntf != NULL)
        {
            // "en0" is the name of the wifi adapter on the iPhone
            if ((pIntf->ifa_addr->sa_family == AF_LINK) && strcmp(pIntf->ifa_name, "en0") == 0)
            {
                struct sockaddr_dl* pDataLinkSockAddr = (struct sockaddr_dl *)(pIntf->ifa_addr);

                if (pDataLinkSockAddr && pDataLinkSockAddr->sdl_alen == 6)
                {
                    ds_memcpy(pState->aMacAddr, LLADDR(pDataLinkSockAddr), 6);

                    NetPrintf(("dirtynetunix: mac address - %X:%X:%X:%X:%X:%X\n",
                              (uint32_t)pState->aMacAddr[0], (uint32_t)pState->aMacAddr[1], (uint32_t)pState->aMacAddr[2],
                              (uint32_t)pState->aMacAddr[3], (uint32_t)pState->aMacAddr[4], (uint32_t)pState->aMacAddr[5]));

                    bFound = TRUE;

                    break;
                }
            }

            pIntf = pIntf->ifa_next;
        }

        // free interface list returned by getifaddrs()
        freeifaddrs(pIntfList);
    }
    else
    {
        NetPrintf(("dirtynetunix: getifaddrs() returned nonzero status: %d\n", iResult));
    }
#else
    struct ifreq req;
    int32_t fd;
    int32_t iIfIndex;

    const char *aIfrName[] =
    {
        "eth0",
        "eth1",
        "wlan0",
        "end-of-interface-array"
    };

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
    {
        for (iIfIndex = 0; strcmp(aIfrName[iIfIndex],"end-of-interface-array"); iIfIndex++)
        {
            strncpy(req.ifr_name, aIfrName[iIfIndex], IFNAMSIZ);
            if ((iResult = ioctl(fd, SIOCGIFHWADDR, &req)) >= 0)
            {
                ds_memcpy(pState->aMacAddr, req.ifr_hwaddr.sa_data, 6);
                bFound = TRUE;
                break;
            }
            else
            {
                NetPrintf(("dirtynetunix: (%s) failed to query MAC address - SIOCGIFHWADDR ioctl failure %d\n", aIfrName[iIfIndex], errno));
            }
        }

        close(fd);
    }
    else
    {
        NetPrintf(("dirtynetunix: can't open socket %d for MAC address query with ioctl(SIOCGIFHWADDR)\n", errno));
    }
#endif

    return(bFound);
}

/*F********************************************************************************/
/*!
    \Function    _SocketInfoGlobal

    \Description
        Return information about global state

    \Input iInfo    - selector for desired information
    \Input iData    - selector specific
    \Input *pBuf    - return buffer
    \Input iLen     - buffer length

    \Output
        int32_t     - selector-specific

    \Notes
        These selectors need to be documented in SocketInfo() to allow our
        documentation generation to pick them up.

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _SocketInfoGlobal(int32_t iInfo, int32_t iData, void *pBuf, int32_t iLen)
{
    SocketStateT *pState = _Socket_pState;

    if (iInfo == 'addr')
    {
        #if defined(DIRTYCODE_APPLEIOS) //$$TODO -- evaluate
        if (pState->uLocalAddr == 0)
        {
            // get local address here, or possibly at network startup
            struct ifaddrs* interfaces = NULL;

            NetPrintf(("dirtynetunix: querying interfaces\n"));

            int error = getifaddrs(&interfaces);
            if (error == 0)
            {
                struct ifaddrs *currentAddress;
                for (currentAddress = interfaces; currentAddress; currentAddress = currentAddress->ifa_next)
                {
                    // only consider live inet interfaces and return first valid non-loopback address
                    if (((currentAddress->ifa_flags & (IFF_LOOPBACK | IFF_UP)) == IFF_UP))
                    {
                        struct sockaddr *pHostAddr = (struct sockaddr *)currentAddress->ifa_addr;
                        if ((currentAddress->ifa_addr->sa_family == AF_INET) && (pState->uLocalAddr == 0))
                        {
                            pState->uLocalAddr = SockaddrInGetAddr(pHostAddr);
                            NetPrintf(("dirtynetunix: found local address %a\n", pState->uLocalAddr));
                        }
                        else if (currentAddress->ifa_addr->sa_family == AF_INET6)
                        {
                            NetPrintf(("dirtynetunix: found interface %A\n", (struct sockaddr *)pHostAddr));
                        }
                    }
                }

                freeifaddrs(interfaces);
            }
            else
            {
                NetPrintf(("dirtynetunix: error %d querying interfaces\n", errno));
            }
        }
        return(pState->uLocalAddr);
        #else
        struct sockaddr HostAddr, DestAddr;
        SockaddrInit(&DestAddr, AF_INET);
        SockaddrInSetAddr(&DestAddr, (uint32_t)iData);
        if (SocketHost(&HostAddr, sizeof(HostAddr), &DestAddr, sizeof(DestAddr)) != -1)
        {
            return(SockaddrInGetAddr(&HostAddr));
        }
        else
        {
            return(-1);
        }
        #endif
    }
    // get socket bound to given port
    if ((iInfo == 'bind') || (iInfo == 'bndu'))
    {
        SocketT *pSocket;
        struct sockaddr BindAddr;
        int32_t iFound = -1;

        // for access to socket list
        NetCritEnter(NULL);

        // walk socket list and find matching socket
        for (pSocket = pState->pSockList; pSocket != NULL; pSocket = pSocket->pNext)
        {
            // if iInfo is 'bndu', only consider sockets of type SOCK_DGRAM
            // note: 'bndu' stands for "bind udp"
            if ((iInfo == 'bind') || ((iInfo == 'bndu') && (pSocket->iType == SOCK_DGRAM)))
            {
                // get socket info
                SocketInfo(pSocket, 'bind', 0, &BindAddr, sizeof(BindAddr));
                if (SockaddrInGetPort(&BindAddr) == iData)
                {
                    *(SocketT **)pBuf = pSocket;
                    iFound = 0;
                    break;
                }
            }
        }

        // for access to g_socklist and g_sockkill
        NetCritLeave(NULL);
        return(iFound);
    }

    if (iInfo == 'conn')
    {
        return(pState->uConnStatus);
    }

    #ifdef DIRTYCODE_ANDROID
    if (iInfo == 'eth0' || iInfo == 'wan0')
    {
        int32_t iRet = -1;
        int32_t fd = socket(AF_INET, SOCK_DGRAM, 0);
        struct ifreq ifr;

        if (fd == -1)
        {
            NetPrintf(("dirtynetunix: SocketInfo('eth0/wan0') cannot create socket descriptor.\n"));
            iRet = -2;
        }
        else
        {
            if (iInfo == 'eth0')
            {
                strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
            }
            else
            {
                strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ);
            }

            if (ioctl(fd, SIOCGIFADDR, &ifr) == -1)
            {
                close(fd);
                NetPrintf(("dirtynetunix: SocketInfo('%c') cannot find an IP address for device %s. Errno %s\n", iInfo, ifr.ifr_name, strerror(errno)));
                iRet = -3;
            }
            else
            {
                close(fd);
                struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifr.ifr_addr;
                if (ipaddr->sin_addr.s_addr != 0)
                {
                    NetPrintf(("dirtynetunix: SocketInfo('%c') network address found for %s.\n", iInfo, ifr.ifr_name));
                    iRet = 0;
                }
                else
                {
                    NetPrintf(("dirtynetunix: SocketInfo('%c') cannot find an IP address for device %s.\n", iInfo, ifr.ifr_name));
                    iRet = -4;
                }
            }
        }

        return(iRet);
    }
    #endif

    // get MAC address
    if ((iInfo == 'ethr') || (iInfo == 'macx'))
    {
        uint8_t aZeros[6] = { 0, 0, 0, 0, 0, 0 };
        uint8_t bFound = TRUE;

        // early exit if user-provided buffer not correct
        if ((pBuf == NULL) && (iLen < (signed)sizeof(pState->aMacAddr)))
        {
            return(-1);
        }

        // try to get mac address if we don't already have it
        if (!memcmp(pState->aMacAddr, aZeros, sizeof(pState->aMacAddr)))
        {
            bFound = _SocketGetMacAddress(pState);
        }

        if (bFound)
        {
            // copy MAC address in user-provided buffer and signal success
            ds_memcpy(pBuf, &pState->aMacAddr, sizeof(pState->aMacAddr));
            return(0);
        }

        // signal failure - no MAC address found
        return(-1);
    }

    // check if specified ipv4 address is virtual and return associated ipv6 address if so
    if (iInfo == '?ip6')
    {
        int32_t iResult = -1;
        struct sockaddr_in6 SockAddr6, *pSockAddr6;
        struct sockaddr SockAddr;
        int32_t iNameLen;

        SockaddrInit(&SockAddr, AF_INET);
        SockaddrInSetAddr(&SockAddr, iData);
        
        SockaddrInit6(&SockAddr6, AF_INET6);
        pSockAddr6 = ((pBuf != NULL) && (iLen == sizeof(SockAddr6))) ? (struct sockaddr_in6 *)pBuf : &SockAddr6;
        iNameLen = sizeof(SockAddr6);

        iResult = SocketAddrMapGet(&pState->AddrMap, (struct sockaddr *)pSockAddr6, &SockAddr, &iNameLen) ? 1 : 0;
        return(iResult);
    }

    // return max packet size
    if (iInfo == 'maxp')
    {
        return(pState->iMaxPacket);
    }

    // get send callback function pointer (iData specifies index in array)
    if (iInfo == 'sdcf')
    {
        if ((pBuf != NULL) && (iLen == sizeof(pState->aSendCbEntries[iData].pSendCallback)))
        {
            ds_memcpy(pBuf, &pState->aSendCbEntries[iData].pSendCallback, sizeof(pState->aSendCbEntries[iData].pSendCallback));
            return(0);
        }

        NetPrintf(("dirtynetunix: 'sdcf' selector used with invalid paramaters\n"));
        return(-1);
    }
    // get send callback user data pointer (iData specifies index in array)
    if (iInfo == 'sdcu')
    {
        if ((pBuf != NULL) && (iLen == sizeof(pState->aSendCbEntries[iData].pSendCallref)))
        {
            ds_memcpy(pBuf, &pState->aSendCbEntries[iData].pSendCallref, sizeof(pState->aSendCbEntries[iData].pSendCallref));
            return(0);
        }

        NetPrintf(("dirtynetunix: 'sdcu' selector used with invalid paramaters\n"));
        return(-1);
    }
    // return global debug output level
    if (iInfo == 'spam')
    {
        return(pState->iVerbose);
    }

    NetPrintf(("dirtynetunix: unhandled global SocketInfo() selector '%C'\n", iInfo));
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function    _SocketControlGlobal

    \Description
        Process a global control message (type specific operation)

    \Input iOption  - the option to pass
    \Input iData1   - message specific parm
    \Input *pData2  - message specific parm
    \Input *pData3  - message specific parm

    \Output
        int32_t     - message specific result (-1=unsupported message)

    \Notes
        These selectors need to be documented in SocketControl() to allow our
        documentation generation to pick them up.

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _SocketControlGlobal(int32_t iOption, int32_t iData1, void *pData2, void *pData3)
{
    SocketStateT *pState = _Socket_pState;

    // init network stack and bring up interface
    if (iOption == 'conn')
    {
        #if defined(DIRTYCODE_APPLEIOS) //$$TODO - evaluate
        CFStringRef URLString = CFStringCreateWithCString(kCFAllocatorDefault, "http://gos.ea.com/util/test.jsp", kCFStringEncodingASCII);
        CFStringRef getString = CFStringCreateWithCString(kCFAllocatorDefault, "GET", kCFStringEncodingASCII);
        CFURLRef baseURL = NULL;
        CFURLRef url = CFURLCreateWithString(kCFAllocatorDefault, URLString, baseURL);
        CFHTTPMessageRef request = CFHTTPMessageCreateRequest(NULL, getString, url, kCFHTTPVersion1_1);
        CFMutableDataRef data = CFDataCreateMutable(NULL, 0);
        CFReadStreamRef readStream = CFReadStreamCreateForHTTPRequest(NULL, request);

        if (CFReadStreamOpen(readStream))
        {
            char done = FALSE;
            do
            {
                const int BUFSIZE = 4096;
                unsigned char buf[BUFSIZE];
                int bytesRead = (int)CFReadStreamRead(readStream, buf, BUFSIZE);
                if (bytesRead > 0)
                {
                    CFDataAppendBytes(data, buf, bytesRead);
                }
                else if (bytesRead == 0)
                {
                    done = TRUE;
                }
                else
                {
                    done = TRUE;
                }
            } while (!done);
        }

        CFReadStreamClose(readStream);
        CFRelease(readStream);
        readStream = nil;

        CFRelease(url);
        url = NULL;

        CFRelease(data);
        data = NULL;

        CFRelease(URLString);
        URLString = NULL;

        CFRelease(getString);
        getString = NULL;
        #endif

        pState->uConnStatus = '+onl';
        return(0);
    }
    // bring down interface
    if (iOption == 'disc')
    {
        NetPrintf(("dirtynetunix: disconnecting from network\n"));
        pState->uConnStatus = '-off';
        return(0);
    }
    // set an ipv6 address into the mapping table
    if (iOption == '+ip6')
    {
        return(SocketAddrMapAddress(&pState->AddrMap, (const struct sockaddr *)pData2, iData1));
    }
    // del an ipv6 address from the mapping table
    if (iOption == '-ip6')
    {
        return(SocketAddrUnmapAddress(&pState->AddrMap, (const struct sockaddr *)pData2, iData1));
    }
    // remap an existing ipv6 address in the mapping table
    if (iOption == '~ip6')
    {
        return(SocketAddrRemapAddress(&pState->AddrMap, (const struct sockaddr *)pData2, (const struct sockaddr *)pData3, iData1));
    }
    // handle any idle processing required
    if (iOption == 'idle')
    {
        // in single-threaded mode, we have to give life to the network idle process
        if (pState->bSingleThreaded)
        {
            NetIdleCall();
        }
        return(0);
    }
    // set max udp packet size
    if (iOption == 'maxp')
    {
        NetPrintf(("dirtynetunix: setting max udp packet size to %d\n", iData1));
        pState->iMaxPacket = iData1;
        return(0);
    }
    // block waiting on input from socket list
    if (iOption == 'poll')
    {
        return(_SocketPoll(pState, (unsigned)iData1*1000000));
    }
    if (iOption == 'poln')
    {
        return(_SocketPoll(pState, (unsigned)iData1*1000));
    }
    // set/unset send callback (iData1=TRUE for set - FALSE for unset, pData2=callback, pData3=callref)
    if (iOption == 'sdcb')
    {
        SocketSendCallbackEntryT sendCbEntry;
        sendCbEntry.pSendCallback = (SocketSendCallbackT *)pData2;
        sendCbEntry.pSendCallref = pData3;

        if (iData1)
        {
            return(SocketSendCallbackAdd(&pState->aSendCbEntries[0], &sendCbEntry));
        }
        else
        {
            return(SocketSendCallbackRem(&pState->aSendCbEntries[0], &sendCbEntry));
        }
    }
    // set debug spam level
    if (iOption == 'spam')
    {
        // module level debug level
        pState->iVerbose = iData1;
        return(0);
    }
    // mark a port as virtual
    if (iOption == 'vadd')
    {
        int32_t iPort;

        // find a slot to add virtual port
        for (iPort = 0; pState->aVirtualPorts[iPort] != 0; iPort++)
            ;
        if (iPort < SOCKET_MAXVIRTUALPORTS)
        {
            NetPrintfVerbose((pState->iVerbose, 1, "dirtynetunix: added port %d to virtual port list\n", iData1));
            pState->aVirtualPorts[iPort] = (uint16_t)iData1;
            return(0);
        }
    }
    // remove port from virtual port list
    if (iOption == 'vdel')
    {
        int32_t iPort;

        // find virtual port in list
        for (iPort = 0; (iPort < SOCKET_MAXVIRTUALPORTS) && (pState->aVirtualPorts[iPort] != (uint16_t)iData1); iPort++)
            ;
        if (iPort < SOCKET_MAXVIRTUALPORTS)
        {
            NetPrintfVerbose((pState->iVerbose, 1, "dirtynetunix: removed port %d from virtual port list\n", iData1));
            pState->aVirtualPorts[iPort] = 0;
            return(0);
        }
    }
    // unhandled
    NetPrintf(("dirtynetunix: unhandled global SocketControl() option '%C'\n", iOption));
    return(-1);
}

/*** Public Functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function    SocketCreate

    \Description
        Create new instance of socket interface module.  Initializes all global
        resources and makes module ready for use.

    \Input iThreadPrio        - priority to start threads with
    \Input iThreadStackSize   - stack size to start threads with (in bytes)
    \Input iThreadCpuAffinity - cpu affinity to start threads with

    \Output
        int32_t               - negative=error, zero=success

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketCreate(int32_t iThreadPrio, int32_t iThreadStackSize, int32_t iThreadCpuAffinity)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    int32_t iResult;

    // Query mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // error if already started
    if (pState != NULL)
    {
        NetPrintf(("dirtynetunix: SocketCreate() called while module is already active\n"));
        return(-1);
    }

    // print version info
    NetPrintf(("dirtynetunix: DirtySDK v%d.%d.%d.%d.%d\n", DIRTYSDK_VERSION_YEAR, DIRTYSDK_VERSION_SEASON, DIRTYSDK_VERSION_MAJOR, DIRTYSDK_VERSION_MINOR, DIRTYSDK_VERSION_PATCH));

    // alloc and init state ref
    if ((pState = DirtyMemAlloc(sizeof(*pState), SOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetunix: unable to allocate module state\n"));
        return(-2);
    }
    ds_memclr(pState, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->iMaxPacket = SOCKET_MAXUDPRECV;
    pState->iVerbose = 1;

    if (iThreadPrio < 0)
    {
        pState->bSingleThreaded = TRUE;
    }

    // disable SIGPIPE (Linux-based system only)
    #if defined(DIRTYCODE_LINUX) || defined(DIRTYCODE_APPLEIOS) || defined(DIRTYCODE_ANDROID)
    _SocketDisableSigpipe();
    #endif

    // startup network libs
    NetLibCreate(iThreadPrio, iThreadStackSize, iThreadCpuAffinity);

    // create hostname cache
    if ((pState->pHostnameCache = SocketHostnameCacheCreate(iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetunix: unable to create hostname cache\n"));
        SocketDestroy((uint32_t)(-1));
        return(-3);
    }

    // add our idle handler
    NetIdleAdd(&_SocketIdle, pState);

    // create high-priority receive thread
    if (!pState->bSingleThreaded)
    {
        DirtyThreadConfigT ThreadConfig;

        // configure threading
        ds_memclr(&ThreadConfig, sizeof(ThreadConfig));
        ThreadConfig.pName = "SocketRecv";
        ThreadConfig.iAffinity = iThreadCpuAffinity;
        ThreadConfig.iPriority = iThreadPrio;
        ThreadConfig.iVerbosity = pState->iVerbose;

        if ((iResult = DirtyThreadCreate(_SocketRecvThread, pState, &ThreadConfig)) == 0)
        {
            // wait for receive thread startup
            while (pState->iRecvLife == 0)
            {
                usleep(100);
            }
        }
        else
        {
            NetPrintf(("dirtynetunix: unable to create recv thread (err=%d)\n", iResult));
            pState->iRecvLife = 0;
        }
    }

    // init socket address map
    SocketAddrMapInit(&pState->AddrMap, pState->iMemGroup, pState->pMemGroupUserData);

    // save state
    _Socket_pState = pState;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function    SocketDestroy

    \Description
        Release resources and destroy module.

    \Input uShutdownFlags   - shutdown flags

    \Output
        int32_t             - negative=error, zero=success

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketDestroy(uint32_t uShutdownFlags)
{
    SocketStateT *pState = _Socket_pState;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtynetunix: SocketDestroy() called while module is not active\n"));
        return(-1);
    }

    NetPrintf(("dirtynetunix: shutting down\n"));

    // wait until all lookup threads are done
    while (pState->pHostList != NULL)
    {
        volatile HostentT **ppHost;
        int32_t iSocketLookups;

        // check for lookup threads that are still active
        for (ppHost = (volatile HostentT **)&pState->pHostList, iSocketLookups = 0; *ppHost != NULL; ppHost = (volatile HostentT **)&(*ppHost)->pNext)
        {
            iSocketLookups += (*ppHost)->thread ? 0 : 1;
        }
        // if no ongoing socket lookups, we're done
        if (iSocketLookups == 0)
        {
            break;
        }
        NetConnSleep(1);
    }

    // kill idle callbacks
    NetIdleDel(&_SocketIdle, pState);

    // let any idle event finish
    NetIdleDone();

    if ((!pState->bSingleThreaded) && (pState->iRecvLife == 1))
    {
        // tell receive thread to quit
        pState->iRecvLife = 2;
        // wait for thread to terminate
        while (pState->iRecvLife > 0)
        {
            usleep(1*1000);
        }
    }

    // cleanup addr map, if allocated
    SocketAddrMapShutdown(&pState->AddrMap);

    // close any remaining sockets
    NetCritEnter(NULL);
    while (pState->pSockList != NULL)
    {
        SocketClose(pState->pSockList);
    }
    NetCritLeave(NULL);

    // clear the kill list
    _SocketIdle(pState);

    // destroy hostname cache
    if (pState->pHostnameCache != NULL)
    {
        SocketHostnameCacheDestroy(pState->pHostnameCache);
    }

    // shut down network libs
    NetLibDestroy(0);

    // dispose of state
    DirtyMemFree(pState, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    _Socket_pState = NULL;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function    SocketOpen

    \Description
        Create a new transfer endpoint. A socket endpoint is required for any
        data transfer operation.

    \Input iAddrFamily  - address family (AF_INET)
    \Input iType        - socket type (SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, ...)
    \Input iProto       - protocol type for SOCK_RAW (unused by others)

    \Output
        SocketT *       - socket reference

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
SocketT *SocketOpen(int32_t iAddrFamily, int32_t iType, int32_t iProto)
{
    return(_SocketOpen(-1, iAddrFamily, iType, iProto, 0));
}

/*F********************************************************************************/
/*!
    \Function    SocketClose

    \Description
        Close a socket. Performs a graceful shutdown of connection oriented protocols.

    \Input *pSocket     - socket reference

    \Output
        int32_t         - negative=error, else zero

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketClose(SocketT *pSocket)
{
    int32_t iSocket = pSocket->uSocket;

    // stop sending
    SocketShutdown(pSocket, SOCK_NOSEND);

    // dispose of SocketT
    if (_SocketClose(pSocket) < 0)
    {
        return(-1);
    }

    // close unix socket if allocated
    if (iSocket >= 0)
    {
        // close socket
        if (close(iSocket) < 0)
        {
            NetPrintf(("dirtynetunix: close() failed (err=%s)\n", DirtyErrGetName(errno)));
        }
    }

    // success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function SocketImport

    \Description
        Import a socket.  The given socket ref may be a SocketT, in which case a
        SocketT pointer to the ref is returned, or it can be an actual unix socket ref,
        in which case a SocketT is created for the unix socket ref.

    \Input uSockRef - socket reference

    \Output
        SocketT *   - pointer to imported socket, or NULL

    \Version 01/14/2005 (jbrookes)
*/
/********************************************************************************F*/
SocketT *SocketImport(intptr_t uSockRef)
{
    SocketStateT *pState = _Socket_pState;
    socklen_t iProtoSize;
    int32_t iProto;
    SocketT *pSock;

    // see if this socket is already in our socket list
    NetCritEnter(NULL);
    for (pSock = pState->pSockList; pSock != NULL; pSock = pSock->pNext)
    {
        if (pSock == (SocketT *)uSockRef)
        {
            break;
        }
    }
    NetCritLeave(NULL);

    // if socket is in socket list, just return it
    if (pSock != NULL)
    {
        return(pSock);
    }

    // get info from socket ref
    iProtoSize = sizeof(iProto);
    if (getsockopt((int32_t)uSockRef, SOL_SOCKET, SO_TYPE, &iProto, &iProtoSize) == 0)
    {
        // create the socket
        pSock = _SocketOpen((int32_t)uSockRef, AF_INET, iProto, 0, 0);

        // update local and remote addresses
        SocketInfo(pSock, 'bind', 0, &pSock->LocalAddr, sizeof(pSock->LocalAddr));
        SocketInfo(pSock, 'peer', 0, &pSock->RemoteAddr, sizeof(pSock->RemoteAddr));

        // mark it as imported
        pSock->bImported = TRUE;
    }
    else
    {
        NetPrintf(("dirtynetunix: getsockopt(SO_TYPE) failed (err=%s)\n", DirtyErrGetName(errno)));
    }

    return(pSock);
}

/*F********************************************************************************/
/*!
    \Function SocketRelease

    \Description
        Release an imported socket.

    \Input *pSocket - pointer to socket

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
void SocketRelease(SocketT *pSocket)
{
    // if it wasn't imported, nothing to do
    if (pSocket->bImported == FALSE)
    {
        return;
    }

    // dispose of SocketT, but leave the sockref alone
    _SocketClose(pSocket);
}

/*F********************************************************************************/
/*!
    \Function    SocketShutdown

    \Description
        Perform partial/complete shutdown of socket indicating that either sending
        and/or receiving is complete.

    \Input *pSocket - socket reference
    \Input iHow     - SOCK_NOSEND and/or SOCK_NORECV

    \Output
        int32_t     - negative=error, else zero

    \Version 09/10/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketShutdown(SocketT *pSocket, int32_t iHow)
{
    int32_t iResult=0;

    // only shutdown a connected socket
    if (pSocket->iType != SOCK_STREAM)
    {
        pSocket->iLastError = SOCKERR_NONE;
        return(pSocket->iLastError);
    }

    // make sure socket ref is valid
    if (pSocket->uSocket == INVALID_SOCKET)
    {
        pSocket->iLastError = SOCKERR_NONE;
        return(pSocket->iLastError);
    }

    // translate how
    if (iHow == SOCK_NOSEND)
    {
        iHow = SHUT_WR;
    }
    else if (iHow == SOCK_NORECV)
    {
        iHow = SHUT_RD;
    }
    else if (iHow == (SOCK_NOSEND|SOCK_NORECV))
    {
        iHow = SHUT_RDWR;
    }

    // do the shutdown
    if (shutdown(pSocket->uSocket, iHow) < 0)
    {
        iResult = errno;

        // log only useful messages
        if (iResult != ENOTCONN)
        {
            NetPrintf(("dirtynetunix: shutdown() failed (err=%s)\n", DirtyErrGetName(iResult)));
        }
    }

    pSocket->iLastError = _SocketTranslateError(iResult);
    return(pSocket->iLastError);
}

/*F********************************************************************************/
/*!
    \Function    SocketBind

    \Description
        Bind a local address/port to a socket.

    \Input *pSocket - socket reference
    \Input *pName   - local address/port
    \Input iNameLen - length of name

    \Output
        int32_t     - standard network error code (SOCKERR_xxx)

    \Notes
        If either address or port is zero, then they are filled in automatically.

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketBind(SocketT *pSocket, const struct sockaddr *pName, int32_t iNameLen)
{
    SocketStateT *pState = _Socket_pState;
    struct sockaddr_in6 SockAddr6;
    int32_t iResult;

    // make sure socket is valid
    if (pSocket->uSocket < 0)
    {
        NetPrintf(("dirtynetunix: attempt to bind invalid socket\n"));
        pSocket->iLastError = SOCKERR_INVALID;
        return(pSocket->iLastError);
    }

    // save local address
    ds_memcpy_s(&pSocket->LocalAddr, sizeof(pSocket->LocalAddr), pName, sizeof(*pName));

    // is the bind port a virtual port?
    if (pSocket->iType == SOCK_DGRAM)
    {
        int32_t iPort;
        uint16_t uPort;

        if ((uPort = SockaddrInGetPort(pName)) != 0)
        {
            // find virtual port in list
            for (iPort = 0; (iPort < SOCKET_MAXVIRTUALPORTS) && (pState->aVirtualPorts[iPort] != uPort); iPort++)
                ;
            if (iPort < SOCKET_MAXVIRTUALPORTS)
            {
                // acquire socket critical section
                NetCritEnter(&pSocket->RecvCrit);

                // check to see if the socket is bound
                if (pSocket->bVirtual && (pSocket->uVirtualPort != 0))
                {
                    NetPrintf(("dirtynetunix: [%p] failed to bind socket to %u which was already bound to port %u virtual\n", pSocket, uPort, pSocket->uVirtualPort));
                    NetCritLeave(&pSocket->RecvCrit);
                    return(pSocket->iLastError = SOCKERR_INVALID);
                }

                // close winsock socket
                NetPrintf(("dirtynetunix: [%p] making socket bound to port %d virtual\n", pSocket, uPort));
                if (pSocket->uSocket != INVALID_SOCKET)
                {
                    shutdown(pSocket->uSocket, SOCK_NOSEND);
                    close(pSocket->uSocket);
                    pSocket->uSocket = INVALID_SOCKET;
                }
                /* increase socket queue size; this protects virtual sockets from having data pushed into
                   them and overwriting previous data that hasn't been read yet */
                pSocket->pRecvQueue = SocketPacketQueueResize(pSocket->pRecvQueue, 4, pState->iMemGroup, pState->pMemGroupUserData);
                // mark socket as virtual
                pSocket->uVirtualPort = uPort;
                pSocket->bVirtual = TRUE;
                
                // release socket critical section
                NetCritLeave(&pSocket->RecvCrit);
                return(0);
            }
        }
    }

    // translate IPv4 -> IPv6 address (if needed)
    if (pName->sa_family != AF_INET6)
    {
        ds_memclr(&SockAddr6, sizeof(SockAddr6));
        SockAddr6.sin6_family = AF_INET6;
        SockAddr6.sin6_port = SocketHtons(SockaddrInGetPort(pName));
        pName = SocketAddrMapTranslate(&pState->AddrMap, (struct sockaddr *)&SockAddr6, pName, &iNameLen);
    }

    // do the bind
    if ((iResult = bind(pSocket->uSocket, pName, iNameLen)) < 0)
    {
        NetPrintf(("dirtynetunix: bind() to port %d failed (err=%s)\n", SockaddrInGetPort(pName), DirtyErrGetName(errno)));
    }
    else if (SockaddrInGetPort(&pSocket->LocalAddr) == 0)
    {
        iNameLen = sizeof(pSocket->LocalAddr);
        iResult = getsockname(pSocket->uSocket, &pSocket->LocalAddr, (socklen_t *)&iNameLen);
        NetPrintf(("dirtynetunix: bind(port=0) succeeded, local address=%a:%d.\n",
                   SockaddrInGetAddr(&pSocket->LocalAddr),
                   SockaddrInGetPort(&pSocket->LocalAddr)));
    }

    pSocket->iLastError = _SocketTranslateError(iResult);
    return(pSocket->iLastError);
}

/*F********************************************************************************/
/*!
    \Function    SocketConnect

    \Description
        Initiate a connection attempt to a remote host.

    \Input *pSocket - socket reference
    \Input *pName   - pointer to name of socket to connect to
    \Input iNameLen - length of name

    \Output
        int32_t     - standard network error code (SOCKERR_xxx)

    \Notes
        Only has real meaning for stream protocols. For a datagram protocol, this
        just sets the default remote host.

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketConnect(SocketT *pSocket, struct sockaddr *pName, int32_t iNameLen)
{
    struct sockaddr_in6 SockAddr6;
    int32_t iResult;

    // initialize family of Sockaddr6
    SockaddrInit6(&SockAddr6, AF_INET6);

    // translate to IPv6 if required
    pName = SocketAddrMapTranslate(&_Socket_pState->AddrMap, (struct sockaddr *)&SockAddr6, pName, &iNameLen);
    NetPrintfVerbose((pSocket->iVerbose, 0, "dirtynetunix: connecting to %A\n", pName));

    // do the connect
    pSocket->iOpened = 0;
    if ((iResult = connect(pSocket->uSocket, pName, iNameLen)) == 0)
    {
        // if connect succeeded (usually for udp sockets) or attempting to establish a non-blocking connection save correct address
        ds_memcpy_s(&pSocket->RemoteAddr, sizeof(pSocket->RemoteAddr), pName, sizeof(*pName));
    }
    else if (errno == EHOSTUNREACH)
    {
        /* if host is unreachable, purge from hostname cache (if present).  this is helpful in
           situations where the addressing scheme has changed; for example if a device switches
           from an IPv4 hosted connection to an IPv6 connection.  in such a case the old (now
           invalid) address is purged from the cache more quickly than if we waited for it to
           expire from the normal cache timeout */
        SocketHostnameCacheDel(_Socket_pState->pHostnameCache, NULL, SockaddrInGetAddr(pName), _Socket_pState->iVerbose);
    }
    else if (errno != EINPROGRESS)
    {
        NetPrintf(("dirtynetunix: connect() failed (err=%s)\n", DirtyErrGetName(errno)));
    }

    pSocket->iLastError = _SocketTranslateError(iResult);
    return(pSocket->iLastError);
}

/*F********************************************************************************/
/*!
    \Function    SocketListen

    \Description
        Start listening for an incoming connection on the socket.  The socket must already
        be bound and a stream oriented connection must be in use.

    \Input *pSocket - socket reference to bound socket (see SocketBind())
    \Input iBacklog - number of pending connections allowed

    \Output
        int32_t     - standard network error code (SOCKERR_xxx)

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketListen(SocketT *pSocket, int32_t iBacklog)
{
    int32_t iResult;

    // do the listen
    if ((iResult = listen(pSocket->uSocket, iBacklog)) < 0)
    {
        NetPrintf(("dirtynetunix: listen() failed (err=%s)\n", DirtyErrGetName(errno)));
    }

    pSocket->iLastError = _SocketTranslateError(iResult);
    return(pSocket->iLastError);
}

/*F********************************************************************************/
/*!
    \Function    SocketAccept

    \Description
        Accept an incoming connection attempt on a socket.

    \Input *pSocket     - socket reference to socket in listening state (see SocketListen())
    \Input *pAddr       - pointer to storage for address of the connecting entity, or NULL
    \Input *pAddrLen    - pointer to storage for length of address, or NULL

    \Output
        SocketT *       - the accepted socket, or NULL if not available

    \Notes
        The integer pointed to by addrlen should on input contain the number of characters
        in the buffer addr.  On exit it will contain the number of characters in the
        output address.

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
SocketT *SocketAccept(SocketT *pSocket, struct sockaddr *pAddr, int32_t *pAddrLen)
{
    SocketT *pOpen = NULL;
    int32_t iIncoming;

    pSocket->iLastError = SOCKERR_INVALID;

    // make sure we have a socket
    if (pSocket->uSocket == INVALID_SOCKET)
    {
        NetPrintf(("dirtynetunix: accept() called on invalid socket\n"));
        return(NULL);
    }

    // make sure turn parm is valid
    if ((pAddr != NULL) && (*pAddrLen < (signed)sizeof(struct sockaddr)))
    {
        NetPrintf(("dirtynetunix: accept() called with invalid address\n"));
        return(NULL);
    }

    // perform inet6 accept
    if (pSocket->iFamily == AF_INET6)
    {
        struct sockaddr_in6 SockAddr6;
        socklen_t iAddrLen;

        SockaddrInit6(&SockAddr6, AF_INET6);
        SockAddr6.sin6_port   = SocketNtohs(SockaddrInGetPort(pAddr));
        SockAddr6.sin6_addr   = in6addr_any;
        iAddrLen = sizeof(SockAddr6);

        iIncoming = accept(pSocket->uSocket, (struct sockaddr *)&SockAddr6, &iAddrLen);
        if (iIncoming != -1)
        {
            // Allocate socket structure and install in list
            pOpen = _SocketOpen(iIncoming, pSocket->iFamily, pSocket->iType, pSocket->iProto, 1);
            pSocket->iLastError = SOCKERR_NONE;

            #if defined(DIRTYCODE_ANDROID) || defined(DIRTYCODE_LINUX)
            /* http://linux.die.net/man/2/accept:
               On Linux, the new socket returned by accept() does not inherit file status flags
               such as O_NONBLOCK and O_ASYNC from the listening socket. This behaviour differs
               from the canonical BSD sockets implementation. */
            // set nonblocking operation
            if (fcntl(iIncoming, F_SETFL, O_NONBLOCK) < 0)
            {
                    NetPrintf(("dirtynetunix: error trying to make socket non-blocking (err=%d)\n", errno));
            }
            #endif
            // translate ipv6 to ipv4 virtual address
            SocketAddrMapAddress(&_Socket_pState->AddrMap, (const struct sockaddr *)&SockAddr6, sizeof(SockAddr6));
            // save translated connecting info for caller
            SockaddrInit(pAddr, AF_INET);
            SocketAddrMapTranslate(&_Socket_pState->AddrMap, pAddr, (struct sockaddr *)&SockAddr6, pAddrLen);
        }
        else
        {
            pSocket->iLastError = _SocketTranslateError(iIncoming);
            if (errno != EWOULDBLOCK)
            {
                NetPrintf(("dirtynetunix: accept() failed (err=%s)\n", DirtyErrGetName(errno)));
            }
        }
    }

    // return the socket
    return(pOpen);
}

/*F********************************************************************************/
/*!
    \Function    SocketSendto

    \Description
        Send data to a remote host. The destination address is supplied along with
        the data. Should only be used with datagram sockets as stream sockets always
        send to the connected peer.

    \Input *pSocket - socket reference
    \Input *pBuf    - the data to be sent
    \Input iLen     - size of data
    \Input iFlags   - unused
    \Input *pTo     - the address to send to (NULL=use connection address)
    \Input iToLen   - length of address

    \Output
        int32_t     - standard network error code (SOCKERR_xxx)

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketSendto(SocketT *pSocket, const char *pBuf, int32_t iLen, int32_t iFlags, const struct sockaddr *pTo, int32_t iToLen)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iResult = -1;

    if (pSocket->bSendCbs)
    {
        // if installed, give socket callback right of first refusal
        if ((iResult = SocketSendCallbackInvoke(&pState->aSendCbEntries[0], pSocket, pSocket->iType, pBuf, iLen, pTo)) > 0)
        {
            return(iResult);
        }
    }

    // make sure socket ref is valid
    if (pSocket->uSocket < 0)
    {
        #if DIRTYCODE_LOGGING
        uint32_t uAddr = 0, uPort = 0;
        if (pTo)
        {
            uAddr = SockaddrInGetAddr(pTo);
            uPort = SockaddrInGetPort(pTo);
        }
        NetPrintf(("dirtynetunix: attempting to send to %a:%d on invalid socket\n", uAddr, uPort));
        #endif
        pSocket->iLastError = SOCKERR_INVALID;
        return(pSocket->iLastError);
    }

    // handle optional data rate throttling
    if ((iLen = SocketRateThrottle(&pSocket->SendRate, pSocket->iType, iLen, "send")) == 0)
    {
        return(0);
    }

    // use appropriate version
    if (pTo == NULL)
    {
        if ((iResult = (int32_t)send(pSocket->uSocket, pBuf, iLen, 0)) < 0)
        {
            if (errno != EWOULDBLOCK)
                NetPrintf(("dirtynetunix: send() failed (err=%s)\n", DirtyErrGetName(errno)));
        }
    }
    else
    {
        struct sockaddr_in6 SockAddr6;
        struct sockaddr *pTo6;

        // do the send
        #if SOCKET_VERBOSE
        NetPrintf(("dirtynetunix: sending %d bytes to %a:%d\n", iLen, SockaddrInGetAddr(pTo), SockaddrInGetPort(pTo)));
        #endif

        SockaddrInit6(&SockAddr6, AF_INET6);
        iToLen = sizeof(SockAddr6);
        pTo6 = SocketAddrMapTranslate(&pState->AddrMap, (struct sockaddr *)&SockAddr6, pTo, &iToLen);
        if ((iResult = (int32_t)sendto(pSocket->uSocket, pBuf, iLen, 0, pTo6, iToLen)) < 0)
        {
            NetPrintf(("dirtynetunix: sendto(%A) failed (err=%s)\n", pTo6, DirtyErrGetName(errno)));
        }
    }
    // translate error
    pSocket->iLastError = iResult = _SocketTranslateError(iResult);
    if (iResult == SOCKERR_BADPIPE)
    {
        // recreate socket (iLastError will be set in _SocketReopen)
        if (_SocketReopen(pSocket))
        {
            // success, re-send (should either work or error out)
            return(SocketSendto(pSocket, pBuf, iLen, iFlags, pTo, iToLen));
        }
        // failed to recreate the socket, error
    }

    // update data rate estimation
    SocketRateUpdate(&pSocket->SendRate, iResult, "send");
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function    SocketRecvfrom

    \Description
        Receive data from a remote host. If socket is a connected stream, then data can
        only come from that source. A datagram socket can receive from any remote host.

    \Input *pSocket     - socket reference
    \Input *pBuf        - buffer to receive data
    \Input iLen         - length of recv buffer
    \Input iFlags       - unused
    \Input *pFrom       - address data was received from (NULL=ignore)
    \Input *pFromLen    - length of address

    \Output
        int32_t         - positive=data bytes received, else standard error code

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketRecvfrom(SocketT *pSocket, char *pBuf, int32_t iLen, int32_t iFlags, struct sockaddr *pFrom, int32_t *pFromLen)
{
    int32_t iRecv = -1, iErrno = 0;

    // clear "hasdata" hint
    pSocket->bHasData = 0;

    // handle rate throttling, if enabled
    if ((iLen = SocketRateThrottle(&pSocket->RecvRate, pSocket->iType, iLen, "recv")) == 0)
    {
        return(0);
    }
    // handle if the socket was killed
    if (pSocket->pKill != NULL)
    {
        pSocket->iLastError = SOCKERR_INVALID;
        return(pSocket->iLastError);
    }

    /* sockets marked for async recv had actual receive operation take place in the thread. sockets marked as virtual have the
       packet pushed into them (specific to unix as we disable async receive for singlethreaded mode). */
    if ((pSocket->bAsyncRecv == TRUE) || (pSocket->bVirtual == TRUE))
    {
        // acquire socket receive critical section
        NetCritEnter(&pSocket->RecvCrit);

        /* given the socket could be either a TCP or UDP socket we handle the no data condition the same.
           this is due to the below, when we do error conversion we override the system error with EWOULDBLOCK because
           with TCP zero would be mean closed. if we are doing direct recv calls then the translation will
           convert based on the errno returned after the call */
        if ((iRecv = _SocketRecvfromPacketQueue(pSocket, pBuf, iLen, pFrom, pFromLen)) == 0)
        {
            iRecv = -1;
            iErrno = EWOULDBLOCK;
        }

        // when data is obtained from the packet queue, we lose visibility on system socket errors
        pSocket->iLastError = SOCKERR_NONE;

        // release socket receive critical section
        NetCritLeave(&pSocket->RecvCrit);
    }
    else // non-async recvthread socket
    {
        // do direct recv call
        if (((iRecv = _SocketRecvfrom(pSocket, pBuf, iLen, pFrom, pFromLen)) < 0) && ((iErrno = errno) != EAGAIN))
        {
            NetPrintf(("dirtynetunix: _SocketRecvfrom() failed on a SOCK_STREAM socket (err=%s)\n", DirtyErrGetName(iErrno)));
        }
    }

    // do error conversion
    iRecv = (iRecv == 0) ? SOCKERR_CLOSED : _SocketTranslateError2(iRecv, iErrno);

    // update data rate estimation
    SocketRateUpdate(&pSocket->RecvRate, iRecv, "recv");

    // return the error code
    pSocket->iLastError = iRecv;
    return(iRecv);
}

/*F********************************************************************************/
/*!
    \Function    SocketInfo

    \Description
        Return information about an existing socket.

    \Input *pSocket - socket reference
    \Input iInfo    - selector for desired information
    \Input iData    - selector specific
    \Input *pBuf    - return buffer
    \Input iLen     - buffer length

    \Output
        int32_t     - selector-specific

    \Notes
        iInfo can be one of the following:

        \verbatim
            'addr' - returns interface address; iData=destination address for routing
            'bind' - return bind data (if pSocket == NULL, get socket bound to given port)
            'bndu' - return bind data (only with pSocket=NULL, get SOCK_DGRAM socket bound to given port)
            'conn' - connection status
            'eth0' - returns 0 if we have a valid ip for eth0 device. negative if error or no address found (Android Only)
            'ethr'/'macx' - local ethernet address (returned in pBuf), 0=success, negative=error
            '?ip6' - return TRUE if ipv4 address specified in iData is virtual, and fill in pBuf with ipv6 address if not NULL
            'maxp' - return configured max packet size
            'maxr' - return configured max recv rate (bytes/sec; zero=uncapped)
            'maxs' - return configured max send rate (bytes/sec; zero=uncapped)
            'pdrp' - return socket packet queue number of packets dropped
            'peer' - peer info (only valid if connected)
            'pmax' - return socket packet queue max depth
            'pnum' - return socket packet queue current depth
            'ratr' - return current recv rate estimation (bytes/sec)
            'rats' - return current send rate estimation (bytes/sec)
            'read' - return if socket has data available for reading
            'sdcf' - get installed send callback function pointer (iData specifies index in array)
            'sdcu' - get installed send callback userdata pointer (iData specifies index in array)
            'serr' - last socket error
            'psiz' - return socket packet queue max size
            'sock' - return socket associated with the specified DirtySock socket
            'spam' - return debug level for debug output
            'stat' - socket status
            'virt' - TRUE if socket is virtual, else FALSE
            'wan0' - returns true if we have a valid ip for wlan0 device. Negative if error or no address found (Android Only)
        \endverbatim

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketInfo(SocketT *pSocket, int32_t iInfo, int32_t iData, void *pBuf, int32_t iLen)
{
    SocketStateT *pState = _Socket_pState;

    // always zero results by default
    if (pBuf != NULL)
    {
        ds_memclr(pBuf, iLen);
    }

    // handle global socket options
    if (pSocket == NULL)
    {
        return(_SocketInfoGlobal(iInfo, iData, pBuf, iLen));
    }

    // return local bind data
    if (iInfo == 'bind')
    {
        int32_t iResult = -1;
        if (pSocket->bVirtual == TRUE)
        {
            SockaddrInit((struct sockaddr *)pBuf, AF_INET);
            SockaddrInSetPort((struct sockaddr *)pBuf, pSocket->uVirtualPort);
            iResult = 0;
        }
        else if (pSocket->uSocket != INVALID_SOCKET)
        {
            struct sockaddr_in6 SockAddr6;
            iLen = sizeof(SockAddr6);
            if ((iResult = getsockname(pSocket->uSocket, (struct sockaddr *)&SockAddr6, (socklen_t *)&iLen)) == 0)
            {
                SockaddrInit((struct sockaddr *)pBuf, AF_INET);
                SockaddrInSetPort((struct sockaddr *)pBuf, SocketHtons(SockAddr6.sin6_port));
                SockaddrInSetAddr((struct sockaddr *)pBuf, SocketAddrMapAddress(&pState->AddrMap, (struct sockaddr *)&SockAddr6, sizeof(SockAddr6)));
            }
            iResult = _SocketTranslateError(iResult);
        }
        return(iResult);
    }

    // return configured max recv rate
    if (iInfo == 'maxr')
    {
        return(pSocket->RecvRate.uMaxRate);
    }

    // return configured max send rate
    if (iInfo == 'maxs')
    {
        return(pSocket->SendRate.uMaxRate);
    }

    // return whether the socket is virtual or not
    if (iInfo == 'virt')
    {
        return(pSocket->bVirtual);
    }

    /* 
       make sure the socket is alive
       ** AFTER THIS POINT WE ENSURE THE SOCKET DESCRIPTOR AND PACKET QUEUE ARE VALID **
    */
    if (pSocket->pKill != NULL)
    {
        pSocket->iLastError = SOCKERR_INVALID;
        return(pSocket->iLastError);
    }

    // return local peer data
    if ((iInfo == 'conn') || (iInfo == 'peer'))
    {
        if (iLen >= (signed)sizeof(pSocket->LocalAddr))
        {
            getpeername(pSocket->uSocket, pBuf, (socklen_t *)&iLen);
        }
        return(0);
    }

    // get packet queue info
    if ((iInfo == 'pdrp') || (iInfo == 'pmax') || (iInfo == 'pnum') || (iInfo == 'psiz'))
    {
        int32_t iResult;
        // acquire socket receive critical section
        NetCritEnter(&pSocket->RecvCrit);
        // get packet queue status
        iResult = SocketPacketQueueStatus(pSocket->pRecvQueue, iInfo);
        // release socket receive critical section
        NetCritLeave(&pSocket->RecvCrit);
        // return success
        return(iResult);
    }

    // return current recv rate estimation
    if (iInfo == 'ratr')
    {
        return(pSocket->RecvRate.uCurRate);
    }

    // return current send rate estimation
    if (iInfo == 'rats')
    {
        return(pSocket->SendRate.uCurRate);
    }

    // return if socket has data
    if (iInfo == 'read')
    {
        return(pSocket->bHasData);
    }

    // return last socket error
    if (iInfo == 'serr')
    {
        return(pSocket->iLastError);
    }

    // return unix socket ref
    if (iInfo == 'sock')
    {
        return(pSocket->uSocket);
    }

    // return socket status
    if (iInfo == 'stat')
    {
        struct pollfd PollFd;

        // if not a connected socket, return TRUE
        if (pSocket->iType != SOCK_STREAM)
        {
            return(1);
        }

        // if not connected, use poll to determine connect
        if (pSocket->iOpened == 0)
        {
            ds_memclr(&PollFd, sizeof(PollFd));
            PollFd.fd = pSocket->uSocket;
            PollFd.events = POLLOUT;
            if (poll(&PollFd, 1, 0) != 0)
            {
                /*
                    Experimentation shows that on connect failed:
                    Android: (only) POLLERR is returned.
                    iOS5: (only) POLLHUP is returned.
                    Linux: both POLLERR and POLLHUP are returned (POLLERR|POLLHUP).

                    To make the code work on all platforms, we test if any of POLLERR and POLLHUP was set.
                */
                if ((PollFd.revents & POLLERR) || (PollFd.revents & POLLHUP))
                {
                    NetPrintfVerbose((pSocket->iVerbose, 0, "dirtynetunix: read exception on connect\n"));
                    pSocket->iOpened = -1;
                }
                // if socket is writable, that means connect succeeded
                else if (PollFd.revents & POLLOUT)
                {
                    NetPrintfVerbose((pSocket->iVerbose, 0, "dirtynetunix: connection open\n"));
                    pSocket->iOpened = 1;
                }
            }
        }

        /* if previously connected, make sure connect still valid. we only do this when not doing async receive for two
           reasons
           1. there is a race condition between the poll waking up and querying the bytes available. if the bytes are
              read on the receive thread between the poll and ioctl then it would think the socket is closed because the
              socket has already been drained
           2. our reasoning behind using the async receive thread could be the cost of recv, plus other calls may be
              expensive as well. the async receive thread will already set the correct state on the socket thus we can
              skip the query and return iOpened back to the user */
        if (!pSocket->bAsyncRecv && (pSocket->iOpened > 0))
        {
            ds_memclr(&PollFd, sizeof(PollFd));
            PollFd.fd = pSocket->uSocket;
            PollFd.events = POLLIN;
            if (poll(&PollFd, 1, 0) != 0)
            {
                // if we got an exception, that means connect failed (usually closed by remote peer)
                if ((PollFd.revents & POLLERR) || (PollFd.revents & POLLHUP))
                {
                    NetPrintfVerbose((pSocket->iVerbose, 0, "dirtynetunix: connection failure\n"));
                    pSocket->iOpened = -1;
                }
                else if (PollFd.revents & POLLIN)
                {
                    int32_t iAvailBytes = 1;
                    // get number of bytes for read (might be less than actual bytes, so it can only be used for zero-test)
                    if (ioctl(pSocket->uSocket, FIONREAD, &iAvailBytes) != 0)
                    {
                        NetPrintfVerbose((pSocket->iVerbose, 0, "dirtynetunix: ioctl(FIONREAD) failed (err=%s).\n", DirtyErrGetName(errno)));
                    }
                    // if socket is readable but there's no data available for read, connect was closed
                    else if (iAvailBytes == 0)
                    {
                        pSocket->iLastError = SOCKERR_CLOSED;
                        NetPrintfVerbose((pSocket->iVerbose, 0, "dirtynetunix: connection closed\n"));
                        pSocket->iOpened = -1;
                    }
                }
            }
        }
        /* if we still have packets in the queue, tell the caller that we are still open. this makes sure they read the
           complete stream of data */
        else if (pSocket->bAsyncRecv && (pSocket->iOpened < 0) && (SocketInfo(pSocket, 'pnum', 0, NULL, 0) != 0))
        {
            return(1);
        }

        // return connect status
        return(pSocket->iOpened);
    }

    // unhandled option?
    NetPrintf(("dirtynetunix: unhandled SocketInfo() option '%C'\n", iInfo));
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function    SocketCallback

    \Description
        Register a callback routine for notification of socket events.  Also includes
        timeout support.

    \Input *pSocket - socket reference
    \Input iMask    - valid callback events (CALLB_NONE, CALLB_SEND, CALLB_RECV)
    \Input iIdle    - if nonzero, specifies the number of ticks between idle calls
    \Input *pRef    - user data to be passed to proc
    \Input *pProc   - user callback

    \Output
        int32_t     - zero

    \Notes
        A callback will reset the idle timer, so when specifying a callback and an
        idle processing time, the idle processing time represents the maximum elapsed
        time between calls.

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketCallback(SocketT *pSocket, int32_t iMask, int32_t iIdle, void *pRef, int32_t (*pProc)(SocketT *pSock, int32_t iFlags, void *pRef))
{
    pSocket->uCallIdle = iIdle;
    pSocket->iCallMask = iMask;
    pSocket->pCallRef = pRef;
    pSocket->pCallback = pProc;
    pSocket->uCallLast = NetTick() - iIdle;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function    SocketControl

    \Description
        Process a control message (type specific operation)

    \Input *pSocket - socket to control, or NULL for module-level option
    \Input iOption  - the option to pass
    \Input iData1   - message specific parm
    \Input *pData2  - message specific parm
    \Input *pData3  - message specific parm

    \Output
        int32_t     - message specific result (-1=unsupported message)

    \Notes
        iOption can be one of the following:

        \verbatim
            'arcv' - set async receive enable/disable (default enabled for DGRAM/RAW, disabled for TCP)
            'conn' - init network stack
            'disc' - bring down network stack
            'idle' - perform any network connection related processing
            '+ip6' - add an IPv6 address into the mapping table and return a virtual IPv4 address to reference it
            '-ip6' - del an IPv6 address from the mapping table
            '~ip6' - remap an existing IPv6 address in the mapping table
            'keep' - set TCP keep-alive settings on Linux (iData1=enable/disable, iData2=keep-alive time, iData3=keep-alive interval)
            'maxp' - set max udp packet size
            'maxr' - set max recv rate (bytes/sec; zero=uncapped)
            'maxs' - set max send rate (bytes/sec; zero=uncapped)
            'nbio' - set nonblocking/blocking mode (TCP only, iData1=TRUE (nonblocking) or FALSE (blocking))
            'ndly' - set TCP_NODELAY state for given stream socket (iData1=zero or one)
            'pdev' - set simulated packet deviation
            'plat' - set simulated packet latency
            'plos' - set simulated packet loss
            'poll' - block waiting on input from socket list (iData1=ms to block)
            'poln' - block waiting on input from socket list (iData1=us to block), unsupported on apple platforms fallback to 'poll' behavior
            'pque' - set socket packet queue depth
            'push' - push data into given socket receive buffer (iData1=size, pData2=data ptr, pData3=sockaddr ptr)
            'radr' - set SO_REUSEADDR On the specified socket
            'rbuf' - set socket recv buffer size
            'sbuf' - set socket send buffer size
            'scbk' - enable/disable "send callbacks usage" on specified socket (defaults to enable)
            'sdcb' - set/unset send callback (iData1=TRUE for set - FALSE for unset, pData2=callback, pData3=callref)
            'soli' - set SO_LINGER On the specified socket, iData1 is timeout in seconds
            'spam' - set debug level for debug output
            'vadd' - add a port to virtual port list
            'vdel' - del a port from virtual port list
        \endverbatim

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketControl(SocketT *pSocket, int32_t iOption, int32_t iData1, void *pData2, void *pData3)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iResult;

    // handle global controls
    if (pSocket == NULL)
    {
        return(_SocketControlGlobal(iOption, iData1, pData2, pData3));
    }

    // set async recv enable
    if (iOption == 'arcv')
    {
        // set socket async recv flag
        pSocket->bAsyncRecv = iData1 ? TRUE : FALSE;
        return(0);
    }

    // set max recv rate
    if (iOption == 'maxr')
    {
        NetPrintf(("dirtynetunix: setting max recv rate to %d bytes/sec\n", iData1));
        pSocket->RecvRate.uMaxRate = iData1;
        return(0);
    }
    // set max send rate
    if (iOption == 'maxs')
    {
        NetPrintf(("dirtynetunix: setting max send rate to %d bytes/sec\n", iData1));
        pSocket->SendRate.uMaxRate = iData1;
        return(0);
    }
    // enable/disable "send callbacks usage" on specified socket (defaults to enable)
    if (iOption == 'scbk')
    {
        if (pSocket->bSendCbs != (iData1?TRUE:FALSE))
        {
            NetPrintf(("dirtynetunix: send callbacks usage changed from %s to %s for socket ref %p\n", (pSocket->bSendCbs?"ON":"OFF"), (iData1?"ON":"OFF"), pSocket));
            pSocket->bSendCbs = (iData1?TRUE:FALSE);
        }
        return(0);
    }
    // set debug spam level
    if (iOption == 'spam')
    {
        // per-socket debug level
        pSocket->iVerbose = iData1;
        return(0);
    }

    /* 
       make sure the socket is alive
       ** AFTER THIS POINT WE ENSURE THE SOCKET DESCRIPTOR AND PACKET QUEUE ARE VALID **
    */
    if (pSocket->pKill != NULL)
    {
        pSocket->iLastError = SOCKERR_INVALID;
        return(pSocket->iLastError);
    }

#if defined(DIRTYCODE_LINUX)
    // configure TCP keep-alive
    if (iOption == 'keep')
    {
        uint32_t bKeepAlive, uKeepAliveTime, uKeepAliveInterval;

        if (pSocket->iType != SOCK_STREAM)
        {
            NetPrintf(("dirtynetunix: [%p] 'keep' control can only be used on a SOCK_STREAM socket\n", pSocket));
            return(-1);
        }

        bKeepAlive = (uint8_t)iData1;                                       //!< enable/disable keep-alive option
        uKeepAliveTime = bKeepAlive ? *(uint32_t *)pData2 / 1000 : 0;       //!< get keep-alive time and convert to seconds
        uKeepAliveInterval = bKeepAlive ? *(uint32_t *)pData3 / 1000 : 0;   //!< get keep-alive interval and convert to seconds

        if ((iResult = setsockopt(pSocket->uSocket, SOL_SOCKET, SO_KEEPALIVE, &bKeepAlive, sizeof(bKeepAlive))) != 0)
        {
            pSocket->iLastError = _SocketTranslateError(iResult);
            NetPrintf(("dirtynetunix: [%p] failed to set SO_KEEPALIVE to %s (err=%d)\n", pSocket, bKeepAlive ? "true" : "false", pSocket->iLastError));
        }
        else if ((iResult = setsockopt(pSocket->uSocket, SOL_TCP, TCP_KEEPIDLE, &uKeepAliveTime, sizeof(uKeepAliveTime))) != 0)
        {
            pSocket->iLastError = _SocketTranslateError(iResult);
            NetPrintf(("dirtynetunix: [%p] failed to set TCP_KEEPIDLE to %ums (err=%d)\n", pSocket, uKeepAliveTime*1000, pSocket->iLastError));
        }
        else if ((iResult = setsockopt(pSocket->uSocket, SOL_TCP, TCP_KEEPINTVL, &uKeepAliveInterval, sizeof(uKeepAliveInterval))) != 0)
        {
            pSocket->iLastError = _SocketTranslateError(iResult);
            NetPrintf(("dirtynetunix: [%p] failed to set TCP_KEEPINTVL to %ums (err=%d)\n", pSocket, uKeepAliveInterval*1000, pSocket->iLastError));
        }
        else
        {
            pSocket->iLastError = SOCKERR_NONE;

            NetPrintfVerbose((pState->iVerbose, 1, "dirtynetunix: [%p] successfully set the TCP keep-alive options (enabled=%s, timeout=%ums, interval=%ums)\n",
                pSocket, bKeepAlive ? "true" : "false", uKeepAliveTime*1000, uKeepAliveInterval*1000));
        }

        return(pSocket->iLastError);
    }
#endif
    // if a stream socket, set nonblocking/blocking mode
    if ((iOption == 'nbio') && (pSocket->iType == SOCK_STREAM))
    {
        int32_t iVal = fcntl(pSocket->uSocket, F_GETFL, O_NONBLOCK);
        iVal = iData1 ? (iVal | O_NONBLOCK) : (iVal & ~O_NONBLOCK);
        iResult = fcntl(pSocket->uSocket, F_SETFL, iVal);
        pSocket->iLastError = _SocketTranslateError(iResult);
        NetPrintf(("dirtynetunix: setting socket:0x%x to %s mode %s (LastError=%d).\n", pSocket, iData1 ? "nonblocking" : "blocking", iResult ? "failed" : "succeeded", pSocket->iLastError));
        return(pSocket->iLastError);
    }
    // if a stream socket, set TCP_NODELAY state
    if ((iOption == 'ndly') && (pSocket->iType == SOCK_STREAM))
    {
        iResult = setsockopt(pSocket->uSocket, IPPROTO_TCP, TCP_NODELAY, &iData1, sizeof(iData1));
        pSocket->iLastError = _SocketTranslateError(iResult);
        return(pSocket->iLastError);
    }
    // set simulated packet loss or packet latency
    if ((iOption == 'pdev') || (iOption == 'plat') || (iOption == 'plos'))
    {
        // acquire socket receive critical section
        NetCritEnter(&pSocket->RecvCrit);
        // forward selector to packet queue
        iResult = SocketPacketQueueControl(pSocket->pRecvQueue, iOption, iData1);
        // release socket receive critical section
        NetCritLeave(&pSocket->RecvCrit);
        return(iResult);
    }

    // change packet queue size
    if (iOption == 'pque')
    {
        // acquire socket receive critical section
        NetCritEnter(&pSocket->RecvCrit);
        // resize the queue
        pSocket->pRecvQueue = SocketPacketQueueResize(pSocket->pRecvQueue, iData1, pState->iMemGroup, pState->pMemGroupUserData);
        // release socket receive critical section
        NetCritLeave(&pSocket->RecvCrit);
        // return success
        return(0);
    }

    // push data into receive buffer
    if (iOption == 'push')
    {
        // acquire socket critical section
        NetCritEnter(&pSocket->RecvCrit);

        // don't allow data that is too large (for the buffer) to be pushed
        if (iData1 > SOCKET_MAXUDPRECV)
        {
            NetPrintf(("dirtynetunix: request to push %d bytes of data discarded (max=%d)\n", iData1, SOCKET_MAXUDPRECV));
            NetCritLeave(&pSocket->RecvCrit);
            return(-1);
        }

        // add packet to queue
        SocketPacketQueueAdd(pSocket->pRecvQueue, (uint8_t *)pData2, iData1, (struct sockaddr *)pData3);
        // remember we have data
        pSocket->bHasData = 1;

        // release socket critical section
        NetCritLeave(&pSocket->RecvCrit);

        // see if we should issue callback
        if ((pSocket->pCallback != NULL) && (pSocket->iCallMask & CALLB_RECV))
        {
            pSocket->pCallback(pSocket, 0, pSocket->pCallRef);
        }
        return(0);
    }
    // set SO_REUSEADDR
    if (iOption == 'radr')
    {
        iResult = setsockopt(pSocket->uSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&iData1, sizeof(iData1));
        pSocket->iLastError = _SocketTranslateError(iResult);
        return(pSocket->iLastError);
    }
    // set socket receive buffer size
    if ((iOption == 'rbuf') || (iOption == 'sbuf'))
    {
        int32_t iOldSize, iNewSize;
        int32_t iSockOpt = (iOption == 'rbuf') ? SO_RCVBUF : SO_SNDBUF;
        socklen_t uOptLen = 4;

        // get current buffer size
        getsockopt(pSocket->uSocket, SOL_SOCKET, iSockOpt, (char *)&iOldSize, &uOptLen);

        // set new size
        iResult = setsockopt(pSocket->uSocket, SOL_SOCKET, iSockOpt, (const char *)&iData1, sizeof(iData1));
        if ((pSocket->iLastError = _SocketTranslateError(iResult)) == SOCKERR_NONE)
        {
            // save new buffer size
            if (iOption == 'rbuf')
            {
                pSocket->iRbufSize = iData1;
            }
            else
            {
                pSocket->iSbufSize = iData1;
            }
        }

        // get new size
        getsockopt(pSocket->uSocket, SOL_SOCKET, iSockOpt, (char *)&iNewSize, &uOptLen);
        #if defined(DIRTYCODE_LINUX)
        /* as per SO_RCVBUF/SO_SNDBUF documentation: "The kernel doubles the value (to allow space for bookkeeping
           overhead) when it is set using setsockopt(), and this doubled value is returned by getsockopt()."  To
           account for this we halve the getsockopt() value so it matches what we requested. */
        iNewSize /= 2;
        #endif
        NetPrintf(("dirtynetunix: setsockopt(%s) changed buffer size from %d to %d\n", (iOption == 'rbuf') ? "SO_RCVBUF" : "SO_SNDBUF",
            iOldSize, iNewSize));

        return(pSocket->iLastError);
    }
    // set SO_LINGER
    if (iOption == 'soli')
    {
        struct linger lingerOptions;
        lingerOptions.l_onoff = TRUE;
        lingerOptions.l_linger = iData1;
        iResult = setsockopt(pSocket->uSocket, SOL_SOCKET, SO_LINGER, &lingerOptions, sizeof(lingerOptions));
        pSocket->iLastError = _SocketTranslateError(iResult);
        return(pSocket->iLastError);
    }
    // unhandled
    NetPrintf(("dirtynetunix: unhandled control option '%C'\n", iOption));
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function    SocketGetLocalAddr

    \Description
        Returns the "external" local address (ie, the address as a machine "out on
        the Internet" would see as the local machine's address).

    \Output
        uint32_t        - local address

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
uint32_t SocketGetLocalAddr(void)
{
    SocketStateT *pState = _Socket_pState;
    if (pState->uLocalAddr == 0)
    {
        pState->uLocalAddr = SocketInfo(NULL, 'addr', 0, NULL, 0);
    }
    return(pState->uLocalAddr);
}

/*F********************************************************************************/
/*!
    \Function    SocketLookup

    \Description
        Lookup a host by name and return the corresponding Internet address. Uses
        a callback/polling system since the socket library does not allow blocking.

    \Input *pText   - pointer to null terminated address string
    \Input iTimeout - number of milliseconds to wait for completion

    \Output
        HostentT *  - hostent struct that includes callback vectors

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
HostentT *SocketLookup(const char *pText, int32_t iTimeout)
{
    SocketStateT *pState = _Socket_pState;
    SocketLookupPrivT *pPriv;
    int32_t iAddr, iResult;
    HostentT *pHost, *pHostRef;
    DirtyThreadConfigT ThreadConfig;

    NetPrintf(("dirtynetunix: looking up address for host '%s'\n", pText));

    // dont allow negative timeouts
    if (iTimeout < 0)
    {
        return(NULL);
    }

    // create new structure
    pPriv = DirtyMemAlloc(sizeof(*pPriv), SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    ds_memclr(pPriv, sizeof(*pPriv));
    pHost = &pPriv->Host;

    // setup callbacks
    pHost->Done = &_SocketLookupDone;
    pHost->Free = &_SocketLookupFree;
    // copy over the target address
    ds_strnzcpy(pHost->name, pText, sizeof(pHost->name));

    // look for refcounted lookup
    if ((pHostRef = SocketHostnameAddRef(&pState->pHostList, &pPriv->Host, TRUE)) != NULL)
    {
        DirtyMemFree(pPriv, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(pHostRef);
    }

    // check for dot notation, then check hostname cache
    if (((iAddr = SocketInTextGetAddr(pText)) != 0) || ((iAddr = SocketHostnameCacheGet(pState->pHostnameCache, pText, pState->iVerbose)) != 0))
    {
        // we've got a dot-notation address
        pHost->addr = iAddr;
        pHost->done = 1;
        // return completed record
        return(pHost);
    }

    /* add an extra refcount for the thread; this ensures the host structure survives until the thread
       is done with it.  this must be done before thread creation. */
    pHost->refcount += 1;

    // configure threading
    ds_memclr(&ThreadConfig, sizeof(ThreadConfig));
    ThreadConfig.pName = "SocketLookup";
    ThreadConfig.iAffinity = NetConnStatus('affn', 0, NULL, 0);
    ThreadConfig.iVerbosity = pState->iVerbose-1;

    // create dns lookup thread
    if ((iResult = DirtyThreadCreate(_SocketLookupThread, pPriv, &ThreadConfig)) < 0)
    {
        NetPrintf(("dirtynetunix: failed to create lookup thread (err=%d)\n", iResult));
        pPriv->Host.done = -1;
        // remove refcount we just added
        pHost->refcount -= 1;
    }

    // return the host reference
    return(pHost);
}

/*F********************************************************************************/
/*!
    \Function    SocketHost

    \Description
        Return the host address that would be used in order to communicate with
        the given destination address.

    \Input *pHost   - [out] local sockaddr struct
    \Input iHostlen - length of structure (sizeof(host))
    \Input *pDest   - remote sockaddr struct
    \Input iDestlen - length of structure (sizeof(dest))

    \Output
        int32_t     - zero=success, negative=error

    \Version 12/12/2003 (sbevan)
*/
/********************************************************************************F*/
int32_t SocketHost(struct sockaddr *pHost, int32_t iHostlen, const struct sockaddr *pDest, int32_t iDestlen)
{
#if defined(DIRTYCODE_APPLEIOS)
    SocketStateT *pState = _Socket_pState;

    // must be same kind of addresses
    if (iHostlen != iDestlen)
    {
        return(-1);
    }

    // do family specific lookup
    if (pDest->sa_family == AF_INET)
    {
        // special case destination of zero or loopback to return self
        if ((SockaddrInGetAddr(pDest) == 0) || (SockaddrInGetAddr(pDest) == 0x7f000000))
        {
            ds_memcpy(pHost, pDest, iHostlen);
            return(0);
        }
        else
        {
            ds_memclr(pHost, iHostlen);
            pHost->sa_family = AF_INET;
            SockaddrInSetAddr(pHost, pState->uLocalAddr);
            return(0);
        }
    }

    // unsupported family
    ds_memclr(pHost, iHostlen);
    return(-3);
#elif defined(DIRTYCODE_LINUX) || defined(DIRTYCODE_ANDROID)
    struct sockaddr_in HostAddr;
    struct sockaddr_in DestAddr;
    uint32_t uSource = 0, uTarget;
    int32_t iSocket;
#if DIRTYCODE_LOGGING
    SocketStateT *pState = _Socket_pState;
#endif
    // get target address
    uTarget = SockaddrInGetAddr(pDest);

    // create a temp socket (must be datagram)
    iSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSocket != INVALID_SOCKET)
    {
        int32_t iIndex;
        int32_t iCount;
        struct ifreq EndpRec[16];
        struct ifconf EndpList;
        uint32_t uAddr;
        uint32_t uMask;

        // request list of interfaces
        ds_memclr(&EndpList, sizeof(EndpList));
        EndpList.ifc_req = EndpRec;
        EndpList.ifc_len = sizeof(EndpRec);
        if (ioctl(iSocket, SIOCGIFCONF, &EndpList) >= 0)
        {
            // figure out number and walk the list
            iCount = EndpList.ifc_len / sizeof(EndpRec[0]);
            for (iIndex = 0; iIndex < iCount; ++iIndex)
            {
                // extract the individual fields
                ds_memcpy(&HostAddr, &EndpRec[iIndex].ifr_addr, sizeof(HostAddr));
                uAddr = ntohl(HostAddr.sin_addr.s_addr);
                ioctl(iSocket, SIOCGIFNETMASK, &EndpRec[iIndex]);
                ds_memcpy(&DestAddr, &EndpRec[iIndex].ifr_broadaddr, sizeof(DestAddr));
                uMask = ntohl(DestAddr.sin_addr.s_addr);
                ioctl(iSocket, SIOCGIFFLAGS, &EndpRec[iIndex]);

                NetPrintfVerbose((pState->iVerbose, 1, "dirtynetunix: checking interface name=%s, fam=%d, flags=%04x, addr=%08x, mask=%08x\n",
                    EndpRec[iIndex].ifr_name, HostAddr.sin_family,
                    EndpRec[iIndex].ifr_flags, uAddr, uMask));

                // only consider live inet interfaces
                if ((HostAddr.sin_family == AF_INET) && ((EndpRec[iIndex].ifr_flags & (IFF_LOOPBACK+IFF_UP)) == (IFF_UP)))
                {
                    // if target is within address range, must be hit
                    if ((uAddr & uMask) == (uTarget & uMask))
                    {
                        uSource = uAddr;
                        break;
                    }
                    // if in a private address space and nothing else found
                    if (((uAddr & 0xff000000) == 0x0a000000) || ((uAddr & 0xffff0000) == 0xc0a80000))
                    {
                        if (uSource == 0)
                        {
                            uSource = uAddr;
                        }
                    }
                    // always take a public address
                    else
                    {
                        uSource = uAddr;
                    }
                }
            }
        }
        // close the socket
        close(iSocket);
    }

    // populate dest addr
    SockaddrInit(pHost, AF_INET);
    SockaddrInSetAddr(pHost, uSource);

    // return result
    return((uSource != 0) ? 0 : -1);
#else
    return(-1);
#endif
}

