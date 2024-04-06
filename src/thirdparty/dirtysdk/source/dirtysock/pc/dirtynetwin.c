/*H*************************************************************************************/
/*!
    \File dirtynetwin.c

    \Description
        Provide a wrapper that translates the Winsock network interface
        into DirtySock calls. In the case of Winsock, little translation
        is needed since it is based off BSD sockets (as is DirtySock).

    \Copyright
        Copyright (c) Electronic Arts 1999-2018.

    \Version 1.0 01/02/2002 (gschaefer) First Version
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1           // avoid windows.h including extra stuff, including winsock.h which we don't want
#endif

#include "DirtySDK/platform.h"

#if defined(DIRTYCODE_XBOXONE)
#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>   // for tcp keep alive definitions
#else
#pragma warning(push,0)
#include <windows.h>
#include <ws2tcpip.h>
#include <mstcpip.h>   // for tcp keep alive definitions
#pragma warning(pop)
#endif

#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtythread.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/dirtyvers.h"

#include "dirtynetpriv.h"       // private include for dirtynet common functions

/*** Defines ***************************************************************************/

#define SOCKET_MAXEVENTS        (64)

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! dirtysock connection socket structure
struct SocketT
{
    SocketT *pNext;             //!< link to next active
    SocketT *pKill;             //!< link to next killed socket

    SOCKET uSocket;             //!< winsock socket ref

    int32_t iFamily;            //!< protocol family
    int32_t iType;              //!< protocol type
    int32_t iProto;             //!< protocol ident

    int8_t iOpened;             //!< negative=error, zero=not open (connecting), positive=open
    uint8_t uImported;          //!< whether socket was imported or not
    uint8_t bVirtual;           //!< if true, socket is virtual
    uint8_t uShutdown;          //!< shutdown flag

    int32_t iLastError;         //!< last socket error

    struct sockaddr LocalAddr;  //!< local address
    struct sockaddr RemoteAddr; //!< remote address

    uint16_t uVirtualPort;      //!< virtual port, if set
    uint8_t bAsyncRecv;         //!< TRUE if async recv is enabled
    int8_t iVerbose;            //!< debug level
    uint8_t bSendCbs;           //!< TRUE if send cbs are enabled, false otherwise
    uint8_t _pad0[3];

    SocketRateT SendRate;       //!< send rate estimation data
    SocketRateT RecvRate;       //!< recv rate estimation data

    uint32_t uCallMask;         //!< valid callback events
    uint32_t uCallLast;         //!< last callback tick
    uint32_t uCallIdle;         //!< ticks between idle calls
    void *pCallRef;             //!< reference calback value
    int32_t (*pCallback)(SocketT *pSocket, int32_t iFlags, void *pRef);

    WSAOVERLAPPED Overlapped;   //!< overlapped i/o structure
    NetCritT RecvCrit;          //!< receive critical section
    int32_t iAddrLen;           //!< storage for async address length write by WSARecv
    uint32_t uRecvFlag;         //!< flags from recv operation
    uint8_t bRecvInp;           //!< if true, a receive operation is in progress  
    uint8_t bInCallback;        //!< in a socket callback
    uint8_t _pad1[2];

    struct sockaddr RecvAddr;   //!< receive address
    struct sockaddr_in6 RecvAddr6;   //!< receive address (ipv6)

    int32_t iPacketQueueResizePending;  // -1 if no resize pending, new size otherwise
    SocketPacketQueueT *pRecvQueue;
    SocketPacketQueueEntryT *pRecvPacket;
};

//! local state
typedef struct SocketStateT
{
    SocketT *pSockList;                 //!< master socket list
    SocketT *pSockKill;                 //!< list of killed sockets
    HostentT *pHostList;                //!< list of ongoing name resolution operations

    uint16_t aVirtualPorts[SOCKET_MAXVIRTUALPORTS]; //!< virtual port list
    int32_t iMaxPacket;                 //!< maximum packet size
    int32_t iFamily;                    //!< family to use for socket operations

    // module memory group
    int32_t iMemGroup;                  //!< module mem group id
    void *pMemGroupUserData;            //!< user data associated with mem group

    int32_t iVersion;                   //!< winsock version
    int32_t iVerbose;                   //!< debug level
    #if defined(DIRTYCODE_XBOXONE)
    uint32_t uLocalAddr;                //!< local ipv4 address
    uint32_t uRandBindPort;
    #else
    uint32_t uAdapterAddress;           //!< local interface used for SocketBind() operations if non-zero
    #endif

    volatile int32_t iRecvLife;         //!< receive thread alive indicator
    WSAEVENT hEvent;                    //!< event used to wake up receive thread
    NetCritT EventCrit;                 //!< event critical section (used when killing events)

    SocketAddrMapT AddrMap;             //!< address map for translating ipv6 addresses to ipv4 virtual addresses and back

    SocketHostnameCacheT *pHostnameCache; //!< hostname cache

    SocketSendCallbackEntryT aSendCbEntries[SOCKET_MAXSENDCALLBACKS]; //!< collection of send callbacks
} SocketStateT;

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

//! module state ref
static SocketStateT *_Socket_pState = NULL;

// Public variables


/*** Private Functions *****************************************************************/


/*F*************************************************************************************/
/*!
    \Function _XlatError0

    \Description
        Translate a winsock error to dirtysock

    \Input iErr     - return value from winsock call
    \Input iWsaErr  - winsock error (from WSAGetLastError())

    \Output
        int32_t     - dirtysock error

    \Version 09/09/2004 (jbrookes)
*/
/************************************************************************************F*/
static int32_t _XlatError0(int32_t iErr, int32_t iWsaErr)
{
    if (iErr < 0)
    {
        iErr = iWsaErr;
        if ((iErr == WSAEWOULDBLOCK) || (iErr == WSA_IO_PENDING))
            iErr = SOCKERR_NONE;
        else if ((iErr == WSAENETUNREACH) || (iErr == WSAEHOSTUNREACH))
            iErr = SOCKERR_UNREACH;
        else if (iErr == WSAENOTCONN)
            iErr = SOCKERR_NOTCONN;
        else if (iErr == WSAECONNREFUSED)
            iErr = SOCKERR_REFUSED;
        else if (iErr == WSAEINVAL)
            iErr = SOCKERR_INVALID;
        else if (iErr == WSAECONNRESET)
            iErr = SOCKERR_CONNRESET;
        else
        {
            NetPrintf(("dirtynetwin: error %s\n", DirtyErrGetName(iErr)));
            iErr = SOCKERR_OTHER;
        }
    }
    return(iErr);
}

/*F*************************************************************************************/
/*!
    \Function _XlatError

    \Description
        Translate the most recent winsock error to dirtysock

    \Input iErr     - return value from winsock call

    \Output
        int32_t     - dirtysock error

    \Version 01/02/2002 (gschaefer)
*/
/************************************************************************************F*/
static int32_t _XlatError(int32_t iErr)
{
    return(_XlatError0(iErr, WSAGetLastError()));
}

/*F*************************************************************************************/
/*!
    \Function _SocketOpen

    \Description
        Allocates a SocketT.  If uSocket is INVALID_SOCKET, a WinSock socket ref is
        created, otherwise uSocket is used.

    \Input uSocket  - socket to use, or INVALID_SOCKET
    \Input iFamily  - address family
    \Input iType    - type (SOCK_DGRAM, SOCK_STREAM, ...)
    \Input iProto   - protocol

    \Output
        SocketT *   - pointer to new socket, or NULL

    \Version 03/03/2005 (jbrookes)
*/
/************************************************************************************F*/
static SocketT *_SocketOpen(SOCKET uSocket, int32_t iFamily, int32_t iType, int32_t iProto)
{
    SocketStateT *pState = _Socket_pState;
    const uint32_t uTrue = 1, uFalse = 0;
    const int32_t iQueueSize = (iType != SOCK_STREAM) ? 1 : 8;
    SocketT *pSocket;

    // allocate memory
    if ((pSocket = (SocketT *)DirtyMemAlloc(sizeof(*pSocket), SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetwin: unable to allocate memory for socket\n"));
        return(NULL);
    }
    ds_memclr(pSocket, sizeof(*pSocket));

    // open a winsock socket
    if ((uSocket == INVALID_SOCKET) && ((uSocket = WSASocketW(iFamily, iType, iProto, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET))
    {
        NetPrintf(("dirtynetwin: error %d creating socket\n", WSAGetLastError()));
        DirtyMemFree(pSocket, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(NULL);
    }

    // create packet queue
    if ((pSocket->pRecvQueue = SocketPacketQueueCreate(iQueueSize, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetwin: failed to create socket queue for socket\n"));
        closesocket(uSocket);
        DirtyMemFree(pSocket, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(NULL);
    }
    pSocket->iPacketQueueResizePending = -1;

    // save the socket
    pSocket->uSocket = uSocket;
    pSocket->iLastError = SOCKERR_NONE;
    pSocket->iVerbose = 1;

    // set to non blocking
    ioctlsocket(uSocket, FIONBIO, (u_long *)&uTrue);

    // if udp, allow broadcast
    if (iType == SOCK_DGRAM)
    {
        setsockopt(uSocket, SOL_SOCKET, SO_BROADCAST, (char *)&uTrue, sizeof(uTrue));
    }
    // if raw, set hdrincl
    if (iType == SOCK_RAW)
    {
        setsockopt(uSocket, IPPROTO_IP, IP_HDRINCL, (char *)&uTrue, sizeof(uTrue));
    }
    // disable IPv6 only (allow IPv4 use)
    if (iFamily == AF_INET6)
    {
        setsockopt(uSocket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&uFalse, sizeof(uFalse));
    }

    // set family/proto info
    pSocket->iFamily = iFamily;
    pSocket->iType = iType;
    pSocket->iProto = iProto;
    pSocket->bAsyncRecv = ((iType == SOCK_DGRAM) || (iType == SOCK_RAW)) ? TRUE : FALSE;
    pSocket->bSendCbs = TRUE;

    // create overlapped i/o event object
    ds_memclr(&pSocket->Overlapped, sizeof(pSocket->Overlapped));
    pSocket->Overlapped.hEvent = WSACreateEvent();

    // initialize receive critical section
    NetCritInit(&pSocket->RecvCrit, "recvthread");

    // install into list
    NetCritEnter(NULL);
    pSocket->pNext = pState->pSockList;
    pState->pSockList = pSocket;
    NetCritLeave(NULL);

    // return the socket
    return(pSocket);
}

/*F*************************************************************************************/
/*!
    \Function _SocketClose

    \Description
        Disposes of a SocketT, including removal from the global socket list and
        disposal of the SocketT allocated memory.  Does NOT dispose of the winsock
        socket ref.

    \Input *pSocket     - socket to close
    \Input bShutdown    - if TRUE, shutdown and close socket ref

    \Output
        int32_t         - negative=failure, zero=success

    \Version 01/14/2005 (jbrookes)
*/
/************************************************************************************F*/
static int32_t _SocketClose(SocketT *pSocket, uint32_t bShutdown)
{
    SocketStateT *pState = _Socket_pState;
    uint8_t bSockInList = FALSE;
    SocketT **ppSocket;

    // for access to socket list
    NetCritEnter(NULL);

    // remove sock from linked list
    for (ppSocket = &pState->pSockList; *ppSocket != NULL; ppSocket = &(*ppSocket)->pNext)
    {
        if (*ppSocket == pSocket)
        {
            *ppSocket = pSocket->pNext;
            bSockInList = TRUE;
            break;
        }
    }

    // release before NetIdleDone
    NetCritLeave(NULL);

    // make sure the socket is in the socket list (and therefore valid)
    if (!bSockInList)
    {
        NetPrintf(("dirtynetwin: warning, trying to close socket 0x%08x that is not in the socket list\n", (uintptr_t)pSocket));
        return(-1);
    }

    // finish any idle call
    NetIdleDone();

    // acquire global critical section
    NetCritEnter(NULL);

    // wake up out of WaitForMultipleEvents()
    WSASetEvent(pState->hEvent);

    // acquire event critical section
    NetCritEnter(&pState->EventCrit);

    // close event
    WSACloseEvent(pSocket->Overlapped.hEvent);
    pSocket->Overlapped.hEvent = WSA_INVALID_EVENT;

    // release event critical section
    NetCritLeave(&pState->EventCrit);

    // release global critical section
    NetCritLeave(NULL);

    // destroy packet queue
    if (pSocket->pRecvQueue != NULL)
    {
        SocketPacketQueueDestroy(pSocket->pRecvQueue);
    }

    // mark as closed
    if (bShutdown && (pSocket->uSocket != INVALID_SOCKET))
    {
        // close winsock socket
        shutdown(pSocket->uSocket, 2);
        closesocket(pSocket->uSocket);
    }
    pSocket->uSocket = INVALID_SOCKET;
    pSocket->iOpened = 0;

    /* Put into killed list:
       Usage of a kill list allows for postponing two things
        * destruction of RecvCrit
        * release of socket data structure memory
      This ensures that RecvCrit is not freed while in-use by a running thread.
      Such a scenario can occur when the receive callback invoked by _SocketRecvThread()
      (while RecvCrit is entered) calls _SocketClose() */
    NetCritEnter(NULL);
    pSocket->pKill = pState->pSockKill;
    pState->pSockKill = pSocket;
    NetCritLeave(NULL);

    return(0);
}

/*F*************************************************************************************/
/*!
    \Function _SocketIdle

    \Description
        Call idle processing code to give connections time.

    \Input *_pState  - module state

    \Version 10/15/1999 (gschaefer)
*/
/************************************************************************************F*/
static void _SocketIdle(void *_pState)
{
    SocketStateT *pState = (SocketStateT *)_pState;
    SocketT *pSocket;
    uint32_t uTick = NetTick();

    // for access to socket list and kill list
    NetCritEnter(NULL);

    // walk socket list and perform any callbacks
    for (pSocket = pState->pSockList; pSocket != NULL; pSocket = pSocket->pNext)
    {
        // see if we should do callback
        if ((pSocket->uCallIdle != 0) && (pSocket->pCallback != NULL) && (!pSocket->bInCallback) && (NetTickDiff(uTick, pSocket->uCallLast) > pSocket->uCallIdle))
        {
            pSocket->bInCallback = TRUE;
            pSocket->pCallback(pSocket, 0, pSocket->pCallRef);
            pSocket->bInCallback = FALSE;
            pSocket->uCallLast = uTick = NetTick();
        }
    }

    // delete any killed sockets
    while ((pSocket = pState->pSockKill) != NULL)
    {
        pState->pSockKill = pSocket->pKill;

        // release the socket's receive critical section
        NetCritKill(&pSocket->RecvCrit);

        // free the socket memory
        DirtyMemFree(pSocket, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }

    // process dns cache list, delete expired entries
    SocketHostnameCacheProcess(pState->pHostnameCache, pState->iVerbose);

    // process hostname list, delete completed lookup requests
    SocketHostnameListProcess(&pState->pHostList, pState->iMemGroup, pState->pMemGroupUserData);

    // release access to socket list and kill list
    NetCritLeave(NULL);
}

/*F*************************************************************************************/
/*!
    \Function _SocketRecvfromPacketQueue

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

/*F*************************************************************************************/
/*!
    \Function _SocketRecvfrom

    \Description
        Check if there is a pending inbound packet in the receive buffer of the
        system socket

    \Input *pSocket     - pointer to socket
    \Input *pBuf        - [out] buffer to receive data
    \Input iLen         - length of recv buffer
    \Input *pFrom       - [out] address data was received from (NULL=ignore)
    \Input *pFromLen    - [out] length of address
    \Input *pRecvErr    - [out] pointer to variable to be filled with recv err code

    \Output
        int32_t         - positive=data bytes received, else standard error code

    \Version 04/20/2016 (mclouatre)
*/
/************************************************************************************F*/
static int32_t _SocketRecvfrom(SocketT *pSocket, const char *pBuf, int32_t iLen, struct sockaddr *pFrom, int32_t *pFromLen, int32_t *pRecvErr)
{
    int32_t iResult = 0;

    // make sure socket ref is valid
    if (pSocket->uSocket == INVALID_SOCKET)
    {
        pSocket->iLastError = SOCKERR_INVALID;
        return(pSocket->iLastError);
    }

    if (pFrom != NULL)
    {
        if (pSocket->iFamily == AF_INET)
        {
            iResult = recvfrom(pSocket->uSocket, (char *)pBuf, iLen, 0, pFrom, pFromLen);
        }
        if (pSocket->iFamily == AF_INET6)
        {
            struct sockaddr_in6 SockAddr6;
            int32_t iFromLen6 = sizeof(SockAddr6);
            SockAddr6.sin6_family = AF_INET;
            if ((iResult = recvfrom(pSocket->uSocket, (char *)pBuf, iLen, 0, (struct sockaddr *)&SockAddr6, &iFromLen6)) > 0)
            {
                SocketAddrMapTranslate(&_Socket_pState->AddrMap, pFrom, (struct sockaddr *)&SockAddr6, pFromLen);
            }
        }
        SockaddrInSetMisc(pFrom, NetTick());
    }
    else
    {
        iResult = recv(pSocket->uSocket, (char *)pBuf, iLen, 0);
        pFrom = &pSocket->RemoteAddr;
    }

    // get most recent socket error
    if ((*pRecvErr = WSAGetLastError()) == WSAEMSGSIZE)
    {
        /* if there was a message truncation, simply return the truncated size.  this matches what we
           do with the async recv thread version and also the linux behavior of recvfrom() */
        iResult = iLen;
    }

    return(iResult);
}

/*F*************************************************************************************/
/*!
    \Function _SocketProcessQueueResize

    \Description
        Resize socket packet queue if needed.

    \Input *pSocket - pointer to socket

    \Notes
        This function is meant to be called from the _SocketRecvThread() only.
        Do not use this function in code paths exercised by other threads.

    \Version 04/20/2016 (mclouatre)
*/
/************************************************************************************F*/
static void _SocketProcessQueueResize(SocketT *pSocket)
{
    SocketStateT *pState = _Socket_pState;
    if ((pSocket->iPacketQueueResizePending != -1))
    {
        pSocket->pRecvQueue = SocketPacketQueueResize(pSocket->pRecvQueue, pSocket->iPacketQueueResizePending, pState->iMemGroup, pState->pMemGroupUserData);
        pSocket->iPacketQueueResizePending = -1;
    }
}

/*F*************************************************************************************/
/*!
    \Function    _SocketRecvfromAsyncComplete

    \Description
        Called when data is received by _SocketRecvThread(). If there is a callback
        registered, then the socket is passed to that callback so the data may be
        consumed.

    \Input *pSocket       - pointer to socket that has new data
    \Input iBytesReceived - number of bytes received

    \Notes
        This function is meant to be called from the _SocketRecvThread() only.
        Do not use this function in code paths exercised by other threads.

    \Version 08/30/2004 (jbrookes)
*/
/************************************************************************************F*/
static void _SocketRecvfromAsyncComplete(SocketT *pSocket, int32_t iBytesReceived)
{
    // translate IPv6->IPv4, save receive timestamp
    if (pSocket->iType != SOCK_STREAM)
    {
        if (pSocket->iFamily == AF_INET6)
        {
            int32_t iNameLen = sizeof(pSocket->RecvAddr);
            SockaddrInit(&pSocket->RecvAddr, AF_INET);
            SocketAddrMapTranslate(&_Socket_pState->AddrMap, &pSocket->RecvAddr, (struct sockaddr *)&pSocket->RecvAddr6, &iNameLen);
        }
        SockaddrInSetMisc(&pSocket->RecvAddr, NetTick());
    }

    // complete setup of packet queue entry (already has data) by updating the size and reception time
    pSocket->pRecvPacket->iPacketSize = iBytesReceived;
    pSocket->pRecvPacket->uPacketTick = NetTick();
    ds_memcpy_s(&pSocket->pRecvPacket->PacketAddr, sizeof(pSocket->pRecvPacket->PacketAddr), &pSocket->RecvAddr, sizeof(pSocket->RecvAddr));

    // we are done with populating that specific packet queue entry, kill reference into packet queue
    pSocket->pRecvPacket = NULL;

    /* This is a safe place to deal with pending packet queue resize because the recvcrit is guaranteed to be locked
       and there is no async recv operation in progress on the queue (which implicitly means that pSocket->pRecvPacket is NOT
       pointing to a buffer in the queue.

       It is important to execute this after pSocket->pRecvPacket (which points to an entry in the queue) is fully initialized
       and befor the recv callback is invoked (because we want to resize to happen before SocketRecvfrom being potentially
       invoked from the callback. */
    _SocketProcessQueueResize(pSocket);

    // see if we should issue callback
    if ((!pSocket->bInCallback) && (pSocket->pCallback != NULL) && (pSocket->uCallMask & CALLB_RECV))
    {
        pSocket->bInCallback = TRUE;
        (pSocket->pCallback)(pSocket, 0, pSocket->pCallRef);
        pSocket->bInCallback = FALSE;
        pSocket->uCallLast = NetTick();
    }
}

/*F*************************************************************************************/
/*!
    \Function _SocketRecvfromAsync

    \Description
        Issue an overlapped recv call on the given socket.

    \Input *pSocket - pointer to socket to read from

    \Notes
        This function is meant to be called from the _SocketRecvThread() only.
        Do not use this function in code paths exercised by other threads.

    \Version 08/30/2004 (jbrookes)
*/
/************************************************************************************F*/
static void _SocketRecvfromAsync(SocketT *pSocket)
{
    int32_t iResult = 0;
    int32_t iRecvErr;
    WSABUF RecvBuf;

    /* Mark the operation as in progress.
    For scenarios where the recv call returned 0 (meaning immediate completion), we enforce an execution
    path similar to recv call not completing immediately. We asssume two things:
    * _SocketRecvThread() will not wait on the next WSAWaitForMultipleEvents() because the event for this socket is signaled.
    * The following call to WSAGetOverlappedResult() will then detect completion of the recv operation. */
    pSocket->bRecvInp = TRUE;

    // get a packet queue entry to receive into
    pSocket->pRecvPacket = SocketPacketQueueAlloc(pSocket->pRecvQueue);

    // set up for recv call
    RecvBuf.buf = (CHAR *)pSocket->pRecvPacket->aPacketData;
    RecvBuf.len = sizeof(pSocket->pRecvPacket->aPacketData);
    pSocket->uRecvFlag = 0;

    // try and receive some data
    if (pSocket->iType == SOCK_DGRAM)
    {
        if (pSocket->iFamily == AF_INET)
        {
            pSocket->iAddrLen = sizeof(pSocket->RecvAddr);
            iResult = WSARecvFrom(pSocket->uSocket, &RecvBuf, 1, NULL, (LPDWORD)&pSocket->uRecvFlag, (struct sockaddr *)&pSocket->RecvAddr, (LPINT)&pSocket->iAddrLen, &pSocket->Overlapped, NULL);
        }
        if (pSocket->iFamily == AF_INET6)
        {
            pSocket->iAddrLen = sizeof(pSocket->RecvAddr6);
            iResult = WSARecvFrom(pSocket->uSocket, &RecvBuf, 1, NULL, (LPDWORD)&pSocket->uRecvFlag, (struct sockaddr *)&pSocket->RecvAddr6, (LPINT)&pSocket->iAddrLen, &pSocket->Overlapped, NULL);
        }
    }
    else // pSocket->iType == SOCK_RAW
    {
        iResult = WSARecv(pSocket->uSocket, &RecvBuf, 1, NULL, (LPDWORD)&pSocket->uRecvFlag, &pSocket->Overlapped, NULL);
    }

    // error?
    if ((iResult == SOCKET_ERROR) && ((iRecvErr = WSAGetLastError()) != WSA_IO_PENDING))
    {
        if (pSocket->iType != SOCK_STREAM)
        {
            NetPrintf(("dirtynetwin: [%p] error %s when trying to initiate async receive on socket\n", pSocket, DirtyErrGetName(iRecvErr)));
        }
        else
        {
            // in the testing done, winsock2 returns WSAECONNABORTED when the FIN comes in for the socket. in reviewing the error codes there was no other more suitable check to make
            NetPrintf(("dirtynetwin: [%p] connection %s\n", pSocket, (iRecvErr == WSAECONNABORTED) ? "closed" : "failed"));
            pSocket->iOpened = -1;
        }

        // clean up resources that were reserved when the async recv was initiated
        SocketPacketQueueAllocUndo(pSocket->pRecvQueue);

        // mark that receive operation is no longer in progress
        pSocket->bRecvInp = FALSE;
    }
}

/*F*************************************************************************************/
/*!
    \Function _SocketRecvThread

    \Description
        Wait for incoming data and deliver it immediately to the registered socket callback,
        if any.

    \Input  pUnused - unused

    \Version 08/30/2004 (jbrookes)
*/
/************************************************************************************F*/
static void _SocketRecvThread(void *pUnused)
{
    SocketStateT *pState = _Socket_pState;
    WSAEVENT aEventList[SOCKET_MAXEVENTS];
    int32_t iNumEvents, iResult;
    SocketT *pSocket;
    char strThreadId[32];

    // get the thread id
    DirtyThreadGetThreadId(strThreadId, sizeof(strThreadId));

    // show we are alive
    NetPrintfVerbose((pState->iVerbose, 0, "dirtynetwin: receive thread started (thid=%s)\n", strThreadId));

    // clear event list
    ds_memclr(aEventList, sizeof(aEventList));

    // loop until done
    while (pState->iRecvLife == 1)
    {
        // acquire global critical section for access to g_socklist
        NetCritEnter(NULL);
    
        // add global event to list
        iNumEvents = 0;
        aEventList[iNumEvents++] = pState->hEvent;
    
        /* Walk the socket list and for each eligible socket do:
                1- If an async recv has recently completed, then finalize the recv operation and invoke the recv callback if registered.
                Then mark the async recv operatoin has no longer in progress.
                2- If an async recv is not in progress, initiate one if n-deep packet queue is not full. Then mark the async recv operation
                as in progress.
                3- If an async revc is in progress (old or newly initiated), then add the socket's event to the event list that the
                thread later blocks on when calling WSAWaitForMultipleEvents(). */
        for (pSocket = pState->pSockList; (pSocket != NULL) && (iNumEvents < SOCKET_MAXEVENTS); pSocket = pSocket->pNext)
        {
            // only handle non-virtual sockets with asyncrecv true
            if ((!pSocket->bVirtual) && (pSocket->uSocket != INVALID_SOCKET) && pSocket->bAsyncRecv)
            {
                // acquire socket critical section
                NetCritEnter(&pSocket->RecvCrit);

                // is an asynchronous recv in progress on this socket?
                if (pSocket->bRecvInp)
                {
                    int32_t iRecvResult;

                    // check for overlapped read completion
                    if (WSAGetOverlappedResult(pSocket->uSocket, &pSocket->Overlapped, (LPDWORD)&iRecvResult, FALSE, (LPDWORD)&pSocket->uRecvFlag) == TRUE)
                    {
                        // mark that receive operation is no longer in progress
                        pSocket->bRecvInp = FALSE;

                        _SocketRecvfromAsyncComplete(pSocket, iRecvResult);
                    }
                    else if ((iResult = WSAGetLastError()) != WSA_IO_INCOMPLETE)
                    {
                        // clean up resources that were reserved when the async recv was initiated
                        SocketPacketQueueAllocUndo(pSocket->pRecvQueue);

                        #if DIRTYCODE_LOGGING
                        if (iResult != WSAECONNRESET)
                        {
                            NetPrintf(("dirtynetwin: [%p] WSAGetOverlappedResult error %d\n", pSocket, iResult));
                        }
                        #endif

                        // mark that receive operation is no longer in progress
                        pSocket->bRecvInp = FALSE;

                        /* This is a safe place to deal with pending packet queue resize because the recvcrit is guaranteed to be locked
                            and there is no async read operation in progress on the queue (which implicitly means that pSocket->pRecvPacket is NOT
                            pointing to a buffer in the queue. */
                        _SocketProcessQueueResize(pSocket);
                    }
                }

                /* Before proceeding, make sure that the user callback invoked in _SocketRecvThreadFinishAsyncRead() did not close the socket
                    with SocketClose(). We know that SocketClose() function resets the value of pSocket->socket to INVALID_SOCKET. Also, we
                    know that it does not destroy pSocket but it queues it in the global kill list. Since this list cannot be processed before
                    the code below, as it also runs in the context of the global critical section, the following code is thread-safe. */
                if ((pSocket->uSocket != INVALID_SOCKET) && ((pSocket->iType != SOCK_STREAM) || (pSocket->iOpened > 0)))
                {
                    // should we initiate a new asynchronous recv on this socket?
                    if (!pSocket->bRecvInp && !SocketPacketQueueStatus(pSocket->pRecvQueue, 'pful'))
                    {
                        _SocketRecvfromAsync(pSocket);
                    }

                    // if an asynchronous recv (old or newly initiated) is in progress, then add the associated event to event list
                    if (pSocket->bRecvInp)
                    {
                        aEventList[iNumEvents++] = pSocket->Overlapped.hEvent;
                    }
                }

                // release socket critical section
                NetCritLeave(&pSocket->RecvCrit);
            }
        }

        // protect against events being deleted
        NetCritEnter(&pState->EventCrit);

        // release global critical section
        NetCritLeave(NULL);

        // wait for an event to trigger
        iResult = WSAWaitForMultipleEvents(iNumEvents, aEventList, FALSE, WSA_INFINITE, FALSE) - WSA_WAIT_EVENT_0;

        // reset the signaled event
        if ((iResult >= 0) && (iResult < iNumEvents))
        {
            WSAResetEvent(aEventList[iResult]);
        }

        // leave event protected section
        NetCritLeave(&pState->EventCrit);
    }

    // indicate we are done
    NetPrintfVerbose((pState->iVerbose, 0, "dirtynetwin: receive thread exit\n"));
    pState->iRecvLife = 0;
}

/*F*************************************************************************************/
/*!
    \Function _SocketPoll

    \Description
        Perform a blocking poll in nanoseconds

    \Input *pState          - pointer to module state
    \Input uPollUsec        - time to perform the poll (select) for
    \Input *ppSocketList    - socket list or NULL

    \Output
        int32_t             - result of the select call

    \Version 04/17/2019 (eesponda)
*/
/************************************************************************************F*/
static int32_t _SocketPoll(SocketStateT *pState, uint32_t uPollUsec, SocketT **ppSocketList)
{
    fd_set FdRead, FdExcept;
    struct timeval TimeVal;
    SocketT *pTempSocket, **ppSocket;
    int32_t iSocket, iResult;
    int32_t iMaxSocket = 0;

    FD_ZERO(&FdRead);
    FD_ZERO(&FdExcept);
    TimeVal.tv_sec = uPollUsec/1000000;
    TimeVal.tv_usec = uPollUsec%1000000;

    // if socket list specified, use it
    if (ppSocketList != NULL)
    {
        // add sockets to select list (FD_SETSIZE is 64)
        for (ppSocket = ppSocketList, iSocket = 0; (*ppSocket != NULL) && (iSocket < FD_SETSIZE); ppSocket++, iSocket++)
        {
            FD_SET((*ppSocket)->uSocket, &FdRead);
            FD_SET((*ppSocket)->uSocket, &FdExcept);
            iMaxSocket = DS_MAX((int32_t)((*ppSocket)->uSocket), iMaxSocket);
        }
    }
    else
    {
        // get exclusive access to socket list
        NetCritEnter(NULL);
        // walk socket list and add all sockets
        for (pTempSocket = pState->pSockList, iSocket = 0; (pTempSocket != NULL) && (iSocket < FD_SETSIZE); pTempSocket = pTempSocket->pNext, iSocket++)
        {
            if (pTempSocket->uSocket != INVALID_SOCKET)
            {
                FD_SET(pTempSocket->uSocket, &FdRead);
                FD_SET(pTempSocket->uSocket, &FdExcept);
                iMaxSocket = DS_MAX((int32_t)(pTempSocket->uSocket), iMaxSocket);
            }
        }
        // for access to g_socklist and g_sockkill
        NetCritLeave(NULL);
    }

    // wait for input on the socket list for up to iData1 milliseconds
    iResult = select(iMaxSocket + 1, &FdRead, NULL, &FdExcept, &TimeVal);

    // if any sockets have pending data, figure out which ones
    if (iResult > 0)
    {
        // re-acquire critical section
        NetCritEnter(NULL);

        // update sockets if there is data to be read or not
        for (pTempSocket = pState->pSockList; pTempSocket != NULL; pTempSocket = pTempSocket->pNext)
        {
            if ((pTempSocket->uSocket != INVALID_SOCKET) &&
                (FD_ISSET(pTempSocket->uSocket, &FdRead)) &&
                (pTempSocket->pCallback != NULL) &&
                (pTempSocket->uCallMask & CALLB_RECV))
            {
                pTempSocket->pCallback(pTempSocket, 0, pTempSocket->pCallRef);
                pTempSocket->uCallLast = NetTick();
            }
        }

        // release the critical section
        NetCritLeave(NULL);
    }

    // return number of file descriptors with pending data
    return(iResult);
}

/*F*************************************************************************************/
/*!
    \Function _SocketInfoGlobal

    \Description
        Return information about global state

    \Input iInfo    - selector for desired information
    \Input iData    - selector specific
    \Input *pBuf    - [out] return buffer
    \Input iLen     - buffer length

    \Output
        int32_t     - size of returned data or error code (negative value)

    \Notes
        These selectors need to be documented in SocketInfo() to allow our
        documentation generation to pick them up.

    \Version 03/31/2017 (eesponda)
*/
/************************************************************************************F*/
static int32_t _SocketInfoGlobal(int32_t iInfo, int32_t iData, void *pBuf, int32_t iLen)
{
    SocketStateT *pState = _Socket_pState;

    if (iInfo == 'addr')
    {
        #if defined(DIRTYCODE_XBOXONE)
        // force new acquisition of address?
        if (iData == 1)
        {
            pState->uLocalAddr = 0;
        }
        #endif
        return(SocketGetLocalAddr());
    }
    // get socket bound to given port
    if ( (iInfo == 'bind') || (iInfo == 'bndu') )
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
            if ( (iInfo == 'bind') || ((iInfo == 'bndu') && (pSocket->iType == SOCK_DGRAM)) )
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

    #if !defined(DIRTYCODE_XBOXONE)
    // get local address previously specified by user for subsequent SocketBind() operations
    if (iInfo == 'ladr')
    {
        // If 'ladr' had not been set previously, address field of output sockaddr buffer
        // will just be filled with 0.
        SockaddrInSetAddr((struct sockaddr *)pBuf, pState->uAdapterAddress);
        return(0);
    }
    #endif
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

        NetPrintf(("dirtynetwin: 'sdcf' selector used with invalid paramaters\n"));
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

        NetPrintf(("dirtynetwin: 'sdcu' selector used with invalid paramaters\n"));
        return(-1);
    }
    // return global debug output level
    if (iInfo == 'spam')
    {
        return(pState->iVerbose);
    }
    // unhandled
    NetPrintf(("dirtynetwin: unhandled global SocketInfo() selector '%C'\n", iInfo));
    return(-1);
}

/*F*************************************************************************************/
/*!
    \Function _SocketControlGlobal

    \Description
        Process a global control message (type specific operation)

    \Input iOption  - the option to pass
    \Input iData1   - message specific parm
    \Input *pData2  - message specific parm
    \Input *pData3  - message specific parm

    \Output
        int32_t     - message specific result (-1=unsupported message, -2=no such module)

    \Notes
        These selectors need to be documented in SocketControl() to allow our
        documentation generation to pick them up.

    \Version 03/31/2017 (eesponda)
*/
/************************************************************************************F*/
static int32_t _SocketControlGlobal(int32_t iOption, int32_t iData1, void *pData2, void *pData3)
{
    SocketStateT *pState = _Socket_pState;

    // set address family to use for socket operations
    if (iOption == 'afam')
    {
        if ((iData1 != AF_INET) && (iData1 != AF_INET6))
        {
            return(-1);
        }
        pState->iFamily = iData1;
        NetPrintf(("dirtynetwin: address family used for socket operations set to %s\n", pState->iFamily == AF_INET ? "AF_INET" : "AF_INET6"));
        return(0);
    }
    // handle connect message
    if (iOption == 'conn')
    {
        return(0);
    }
    // handle disconnect message
    if (iOption == 'disc')
    {
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
    #if !defined(DIRTYCODE_XBOXONE)
    // set local address? (used to select between multiple network interfaces)
    if (iOption == 'ladr')
    {
        pState->uAdapterAddress = (unsigned)iData1;
        return(0);
    }
    #endif
    // set max udp packet size
    if (iOption == 'maxp')
    {
        NetPrintf(("dirtynetwin: setting max udp packet size to %d\n", iData1));
        pState->iMaxPacket = iData1;
        return(0);
    }
    // block waiting on input from socket list in milliseconds
    if (iOption == 'poll')
    {
        return(_SocketPoll(pState, (unsigned)iData1*1000, pData2));
    }
    if (iOption == 'poln')
    {
        return(_SocketPoll(pState, (unsigned)iData1, pData2));
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
    // set debug level
    if (iOption == 'spam')
    {
        // set module level debug level
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
            NetPrintfVerbose((pState->iVerbose, 1, "dirtynetwin: added port %d to virtual port list\n", iData1));
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
            NetPrintfVerbose((pState->iVerbose, 1, "dirtynetwin: removed port %d from virtual port list\n", iData1));
            pState->aVirtualPorts[iPort] = 0;
            return(0);
        }
    }
    // unhandled
    NetPrintf(("dirtynetwin: unhandled global SocketControl() option '%C'\n", iOption));
    return(-1);
}

/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function SocketCreate

    \Description
        Create new instance of socket interface module.  Initializes all global
        resources and makes module ready for use.

    \Input iThreadPrio        - priority to start threads with
    \Input iThreadStackSize   - stack size to start threads with (in bytes)
    \Input iThreadCpuAffinity - cpu affinity to start threads with

    \Output
        int32_t               - negative=error, zero=success

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketCreate(int32_t iThreadPrio, int32_t iThreadStackSize, int32_t iThreadCpuAffinity)
{
    SocketStateT *pState = _Socket_pState;
    WSADATA WSAData;
    int32_t iResult;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    DirtyThreadConfigT ThreadConfig;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // error if already started
    if (pState != NULL)
    {
        NetPrintfVerbose((pState->iVerbose, 0, "dirtynetwin: SocketCreate() called while module is already active\n"));
        return(-1);
    }

    // print version info
    NetPrintf(("dirtynetwin: DirtySDK v%d.%d.%d.%d.%d\n", DIRTYSDK_VERSION_YEAR, DIRTYSDK_VERSION_SEASON, DIRTYSDK_VERSION_MAJOR, DIRTYSDK_VERSION_MINOR, DIRTYSDK_VERSION_PATCH));

    // alloc and init state ref
    if ((pState = (SocketStateT *)DirtyMemAlloc(sizeof(*pState), SOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetwin: unable to allocate module state\n"));
        return(-2);
    }
    ds_memclr(pState, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->iMaxPacket = SOCKET_MAXUDPRECV;
    pState->iFamily = AF_INET6;
    pState->iVerbose = 1;

    // save global module ref
    _Socket_pState = pState;

    // startup network libs
    NetLibCreate(iThreadPrio, iThreadStackSize, iThreadCpuAffinity);

    // start winsock
    ds_memclr(&WSAData, sizeof(WSAData));
    iResult = WSAStartup(MAKEWORD(2,2), &WSAData);
    if (iResult != 0)
    {
        NetPrintf(("dirtynetwin: error %d loading winsock library\n", iResult));
        SocketDestroy((uint32_t)(-1));
        return(-3);
    }

    // save the available version
    pState->iVersion = (LOBYTE(WSAData.wVersion)<<8)|(HIBYTE(WSAData.wVersion)<<0);

    // create hostname cache
    if ((pState->pHostnameCache = SocketHostnameCacheCreate(iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetwin: unable to create hostname cache\n"));
        SocketDestroy((uint32_t)(-1));
        return(-4);
    }

    // add our idle handler
    NetIdleAdd(&_SocketIdle, pState);

    // create a global event to notify recv thread of newly available sockets
    pState->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (pState->hEvent == NULL)
    {
        NetPrintf(("dirtynetwin: error %d creating state event\n", GetLastError()));
        SocketDestroy(0);
        return(-5);
    }

    // initialize event critical section
    NetCritInit(&pState->EventCrit, "recv-event-crit");

    /* if recvthread has been created but has no chance to run (upon failure or shutdown
       immediately), there will be a problem.  by setting iRecvLife to non-zero before
       thread creation we ensure SocketDestroy work properly. */
    pState->iRecvLife = 1; // see _SocketRecvThread for more details

    // configure thread parameters
    ds_memclr(&ThreadConfig, sizeof(ThreadConfig));
    ThreadConfig.pName = "SocketRecv";
    ThreadConfig.iPriority = iThreadPrio;
    ThreadConfig.iAffinity = iThreadCpuAffinity;
    ThreadConfig.iVerbosity = pState->iVerbose;

    // start up socket receive thread
    if ((iResult = DirtyThreadCreate(_SocketRecvThread, NULL, &ThreadConfig)) != 0)
    {
        pState->iRecvLife = 0; // no recvthread was created, reset to 0
        NetPrintf(("dirtynetwin: error %d creating socket receive thread\n", iResult));
        SocketDestroy(0);
        return(-6);
    }

    // init socket address map
    SocketAddrMapInit(&pState->AddrMap, pState->iMemGroup, pState->pMemGroupUserData);

    // return success
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function SocketDestroy

    \Description
        Release resources and destroy module.

    \Input uShutdownFlags   - shutdown flags

    \Output
        int32_t             - negative=error, zero=success

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketDestroy(uint32_t uShutdownFlags)
{
    SocketStateT *pState = _Socket_pState;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtynetwin: SocketDestroy() called while module is not active\n"));
        return(-1);
    }

    NetPrintf(("dirtynetwin: shutting down\n"));

    // $$TODO- why don't we do this on XONE too?
    #if !defined(DIRTYCODE_XBOXONE)
    // wait until all lookup threads are done
    while (pState->pHostList != NULL)
    {
        volatile HostentT **ppHost;
        int32_t iSocketLookups;

        // check for lookup threads that are still active
        for (ppHost = &pState->pHostList, iSocketLookups = 0; *ppHost != NULL; ppHost = (volatile HostentT **)&(*ppHost)->pNext)
        {
            iSocketLookups += (*ppHost)->thread ? 0 : 1;
        }
        // if no ongoing socket lookups, we're done
        if (iSocketLookups == 0)
        {
            break;
        }
        Sleep(1);
    }
    #endif

    // kill idle callbacks
    NetIdleDel(&_SocketIdle, pState);

    // let any idle event finish
    NetIdleDone();

    // tell receive thread to quit and wake it up (if running)
    if (pState->iRecvLife == 1)
    {
        pState->iRecvLife = 2;
        if (pState->hEvent != NULL)
        {
            WSASetEvent(pState->hEvent);
        }

        // wait for thread to terminate
        while (pState->iRecvLife > 0)
        {
            Sleep(1);
        }
    }

    // cleanup addr map, if allocated
    SocketAddrMapShutdown(&pState->AddrMap);

    // close all sockets
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

    // we rely on the Handle having been created to know if the critical section was created.
    if (pState->hEvent != NULL)
    {
        // destroy event critical section
        NetCritKill(&pState->EventCrit);
        // delete global event
        CloseHandle(pState->hEvent);
    }

    // free the memory and clear global module ref
    _Socket_pState = NULL;
    DirtyMemFree(pState, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);

    // shut down network libs
    NetLibDestroy(0);

    // shutdown winsock, unless told otherwise
    if (uShutdownFlags == 0)
    {
        WSACleanup();
    }

    NetPrintf(("dirtynetwin: shutdown complete\n"));

    return(0);
}

/*F*************************************************************************************/
/*!
    \Function SocketOpen

    \Description
        Create a new transfer endpoint. A socket endpoint is required for any
        data transfer operation.

    \Input iFamily  - address family (AF_INET) [IGNORED]
    \Input iType    - socket type (SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, ...)
    \Input iProto   - protocol type for SOCK_RAW (unused by others)

    \Output
        SocketT     - socket reference

    \Notes
        The address family specified here is ignored; instead it is dictated based on
        the internal configuration setting which defaults to AF_INET6, but can be
        overridden using the 'fam' SocketControl() selector.

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
SocketT *SocketOpen(int32_t iFamily, int32_t iType, int32_t iProto)
{
    return(_SocketOpen(INVALID_SOCKET, _Socket_pState->iFamily, iType, iProto));
}

/*F*************************************************************************************/
/*!
    \Function SocketClose

    \Description
        Close a socket. Performs a graceful shutdown of connection oriented protocols.

    \Input *pSocket - socket reference

    \Output
        int32_t     - zero

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketClose(SocketT *pSocket)
{
    return(_SocketClose(pSocket, TRUE));
}

/*F*************************************************************************************/
/*!
    \Function SocketImport

    \Description
        Import a socket.  The given socket ref may be a SocketT, in which case a
        SocketT pointer to the ref is returned, or it can be an actual Sony socket ref,
        in which case a SocketT is created for the Sony socket ref.

    \Input uSockRef     - socket reference

    \Output
        SocketT *       - pointer to imported socket, or NULL

    \Version 01/14/2005 (jbrookes)
*/
/************************************************************************************F*/
SocketT *SocketImport(intptr_t uSockRef)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iProto, iProtoSize;
    SocketT *pSocket;

    // see if this socket is already in our socket list
    NetCritEnter(NULL);
    for (pSocket = pState->pSockList; pSocket != NULL; pSocket = pSocket->pNext)
    {
        if (pSocket == (SocketT *)uSockRef)
        {
            break;
        }
    }
    NetCritLeave(NULL);

    // if socket is in socket list, just return it
    if (pSocket != NULL)
    {
        return(pSocket);
    }

    //$$ TODO - this assumes AF_INET

    // get info from socket ref
    iProtoSize = sizeof(iProto);
    if (getsockopt((SOCKET)uSockRef, 0, SO_TYPE, (char *)&iProto, &iProtoSize) != SOCKET_ERROR)
    {
        // create the socket (note: winsock socket types directly map to dirtysock socket types)
        pSocket = _SocketOpen(uSockRef, pSocket->iFamily, iProto, 0);

        // update local and remote addresses
        SocketInfo(pSocket, 'bind', 0, &pSocket->LocalAddr, sizeof(pSocket->LocalAddr));
        SocketInfo(pSocket, 'peer', 0, &pSocket->RemoteAddr, sizeof(pSocket->RemoteAddr));

        // mark it as imported
        pSocket->uImported = 1;
    }

    return(pSocket);
}

/*F*************************************************************************************/
/*!
    \Function SocketRelease

    \Description
        Release an imported socket.

    \Input *pSocket   - pointer to socket

    \Version 01/14/2005 (jbrookes)
*/
/************************************************************************************F*/
void SocketRelease(SocketT *pSocket)
{
    // if it wasn't imported, nothing to do
    if (!pSocket->uImported)
    {
        return;
    }

    // dispose of SocketT, but leave the sockref alone
    _SocketClose(pSocket, FALSE);
}

/*F*************************************************************************************/
/*!
    \Function SocketShutdown

    \Description
        Perform partial/complete shutdown of socket indicating that either sending
        and/or receiving is complete.

    \Input *pSocket - socket reference
    \Input iHow     - SOCK_NOSEND and/or SOCK_NORECV

    \Output
        int32_t     - zero

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketShutdown(SocketT *pSocket, int32_t iHow)
{
    // no shutdown for an invalid socket
    if (pSocket->uSocket == INVALID_SOCKET)
    {
        return(0);
    }

    // translate how
    if (iHow == SOCK_NOSEND)
    {
        iHow = SD_SEND;
    }
    else if (iHow == SOCK_NORECV)
    {
        iHow = SD_RECEIVE;
    }
    else if (iHow == (SOCK_NOSEND|SOCK_NORECV))
    {
        iHow = SD_BOTH;
    }

    pSocket->uShutdown |= iHow;
    shutdown(pSocket->uSocket, iHow);

    return(0);
}

/*F*************************************************************************************/
/*!
    \Function SocketBind

    \Description
        Bind a local address/port to a socket.

    \Input *pSocket - socket reference
    \Input *pName   - local address/port
    \Input iNameLen - length of name

    \Output
        int32_t     - standard network error code (SOCKERR_xxx)

    \Notes
        If either address or port is zero, then they are filled in automatically.

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketBind(SocketT *pSocket, const struct sockaddr *pName, int32_t iNameLen)
{
    SocketStateT *pState = _Socket_pState;
    struct sockaddr_in6 SockAddr6;
    int32_t iResult;

    #if !defined(DIRTYCODE_XBOXONE)
    struct sockaddr BindAddr;

    // bind to specific address?
    if ((SockaddrInGetAddr(pName) == 0) && (pState->uAdapterAddress != 0))
    {
        ds_memcpy_s(&BindAddr, sizeof(BindAddr), pName, sizeof(*pName));
        SockaddrInSetAddr(&BindAddr, pState->uAdapterAddress);
        pName = &BindAddr;
    }
    #endif

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
                    NetPrintf(("dirtynetwin: [%p] failed to bind socket to %u which was already bound to port %u virtual\n", pSocket, uPort, pSocket->uVirtualPort));
                    NetCritLeave(&pSocket->RecvCrit);
                    return(pSocket->iLastError = SOCKERR_INVALID);
                }

                // close winsock socket
                NetPrintf(("dirtynetwin: [%p] making socket bound to port %d virtual\n", pSocket, uPort));
                if (pSocket->uSocket != INVALID_SOCKET)
                {
                    shutdown(pSocket->uSocket, SOCK_NOSEND);
                    closesocket(pSocket->uSocket);
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

    #if defined(DIRTYCODE_XBOXONE)
    // set up IPv6 address (do not use an IPv4-mapped IPv6 here as it will disallow IPv6 address use)
    ds_memclr(&SockAddr6, sizeof(SockAddr6));
    SockAddr6.sin6_family = AF_INET6;
    SockAddr6.sin6_port = SocketNtohs(SockaddrInGetPort(pName));
    pName = (const struct sockaddr *)&SockAddr6;
    iNameLen = sizeof(SockAddr6);
    #else
    // translate IPv4 -> IPv6 address (if needed)
    if ((pSocket->iFamily == AF_INET6) && (pName->sa_family != AF_INET6))
    {
        ds_memclr(&SockAddr6, sizeof(SockAddr6));
        SockAddr6.sin6_family = AF_INET6;
        SockAddr6.sin6_port = SocketNtohs(SockaddrInGetPort(pName));
        pName = SocketAddrMapTranslate(&_Socket_pState->AddrMap, (struct sockaddr *)&SockAddr6, pName, &iNameLen);
    }
    #endif

    // execute the bind
    iResult = _XlatError(bind(pSocket->uSocket, pName, iNameLen));

    // notify read thread that socket is ready to be read from
    if ((iResult == SOCKERR_NONE) && pSocket->bAsyncRecv)
    {
        WSASetEvent(pState->hEvent);
    }

    // return result to caller
    pSocket->iLastError = iResult;
    return(pSocket->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function SocketConnect

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

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketConnect(SocketT *pSocket, struct sockaddr *pName, int32_t iNameLen)
{
    struct sockaddr_in6 SockAddr6;
    struct sockaddr SockAddr, *pSockAddr = NULL;
    int32_t iResult, iSockAddrLen = 0;

    // mark as not open
    pSocket->iOpened = 0;

    // save connect address
    ds_memcpy_s(&pSocket->RemoteAddr, sizeof(pSocket->RemoteAddr), pName, sizeof(*pName));

    // init sockaddr
    if (pSocket->iFamily == AF_INET)
    {
        SockaddrInit(&SockAddr, AF_INET);
        pSockAddr = &SockAddr;
        iSockAddrLen = sizeof(SockAddr);
    }
    else if (pSocket->iFamily == AF_INET6)
    {
        // initialize family of SockAddr6
        SockaddrInit6(&SockAddr6, AF_INET6);
        pSockAddr = (struct sockaddr *)&SockAddr6;
        iSockAddrLen = sizeof(SockAddr6);
    }

    #if !defined(DIRTYCODE_XBOXONE)
    /* execute an explicit bind - this allows us to specify a non-zero local address
       or port (see SocketBind()).  a SOCKERR_INVALID result here means the socket has
       already been bound, so we ignore that particular error */
    if (((iResult = SocketBind(pSocket, pSockAddr, iSockAddrLen)) < 0) && (iResult != SOCKERR_INVALID))
    {
        pSocket->iLastError = iResult;
        return(pSocket->iLastError);
    }
    #endif

    // translate to IPv6 if required
    if (pSocket->iFamily == AF_INET6)
    {
        pName = SocketAddrMapTranslate(&_Socket_pState->AddrMap, pSockAddr, pName, &iNameLen);
    }

    // execute the connect
    NetPrintfVerbose((pSocket->iVerbose, 0, "dirtynetwin: connecting to %A\n", pName));
    iResult = _XlatError(connect(pSocket->uSocket, pName, iNameLen));

    // notify read thread that socket is ready to be read from
    if ((iResult == SOCKERR_NONE) && pSocket->bAsyncRecv)
    {
        WSASetEvent(_Socket_pState->hEvent);
    }

    // return result to caller
    pSocket->iLastError = iResult;
    return(pSocket->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function SocketListen

    \Description
        Start listening for an incoming connection on the socket.  The socket must already
        be bound and a stream oriented connection must be in use.

    \Input *pSocket - socket reference to bound socket (see SocketBind())
    \Input iBackLog - number of pending connections allowed

    \Output
        int32_t         - standard network error code (SOCKERR_xxx)

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketListen(SocketT *pSocket, int32_t iBackLog)
{
    // do the listen
    pSocket->iLastError = _XlatError(listen(pSocket->uSocket, iBackLog));
    return(pSocket->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function SocketAccept

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

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
SocketT *SocketAccept(SocketT *pSocket, struct sockaddr *pAddr, int32_t *pAddrLen)
{
    SocketT *pOpen = NULL;
    SOCKET iIncoming;

    pSocket->iLastError = SOCKERR_INVALID;

    // see if already connected
    if (pSocket->uSocket == INVALID_SOCKET)
    {
        return(NULL);
    }

    // make sure turn parm is valid
    if ((pAddr != NULL) && (*pAddrLen < sizeof(struct sockaddr)))
    {
        return(NULL);
    }

    #if !defined(DIRTYCODE_XBOXONE)
    // perform inet accept
    if (pSocket->iFamily == AF_INET)
    {
        iIncoming = accept(pSocket->uSocket, pAddr, pAddrLen);
        if (iIncoming != INVALID_SOCKET)
        {
            pOpen = _SocketOpen(iIncoming, pSocket->iFamily, pSocket->iType, pSocket->iProto);
            pSocket->iLastError = SOCKERR_NONE;
        }
        else
        {
            // use a negative error code to force _XlatError to internally call WSAGetLastError() and translate the obtained result
            pSocket->iLastError = _XlatError(-99);

            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                NetPrintf(("dirtynetwin: accept() failed err=%d\n", WSAGetLastError()));
            }
        }
    }
    #endif

    // perform inet6 accept
    if (pSocket->iFamily == AF_INET6)
    {
        struct sockaddr_in6 SockAddr6;
        int32_t iAddrLen;

        SockaddrInit6(&SockAddr6, AF_INET6);
        iAddrLen = sizeof(SockAddr6);
        iIncoming = accept(pSocket->uSocket, (struct sockaddr *)&SockAddr6, &iAddrLen);
        if (iIncoming != INVALID_SOCKET)
        {
            pOpen = _SocketOpen(iIncoming, pSocket->iFamily, pSocket->iType, pSocket->iProto);
            pSocket->iLastError = SOCKERR_NONE;
            // translate ipv6 to ipv4 virtual address
            SocketAddrMapAddress(&_Socket_pState->AddrMap, (struct sockaddr *)&SockAddr6, sizeof(SockAddr6));

            // save translated connecting info for caller
            SockaddrInit(pAddr, AF_INET);
            SocketAddrMapTranslate(&_Socket_pState->AddrMap, pAddr, (struct sockaddr *) &SockAddr6, pAddrLen);
        }
        else
        {
            // use a negative error code to force _XlatError to internally call WSAGetLastError() and translate the obtained result
            pSocket->iLastError = _XlatError(-99);

            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                NetPrintf(("dirtynetwin: accept() failed err=%d\n", WSAGetLastError()));
            }
        }
    }

    // return the socket
    return(pOpen);
}

/*F*************************************************************************************/
/*!
    \Function SocketSendto

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

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketSendto(SocketT *pSocket, const char *pBuf, int32_t iLen, int32_t iFlags, const struct sockaddr *pTo, int32_t iToLen)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iResult;

    if (pSocket->bSendCbs)
    {
        // if installed, give socket callback right of first refusal
        if ((iResult = SocketSendCallbackInvoke(&pState->aSendCbEntries[0], pSocket, pSocket->iType, pBuf, iLen, pTo)) > 0)
        {
            return(iResult);
        }
    }

    // make sure socket ref is valid
    if (pSocket->uSocket == INVALID_SOCKET)
    {
        #if DIRTYCODE_LOGGING
        uint32_t uAddr = 0, uPort = 0;
        if (pTo)
        {
            uAddr = SockaddrInGetAddr(pTo);
            uPort = SockaddrInGetPort(pTo);
        }
        NetPrintf(("dirtynetwin: attempting to send to %a:%d on invalid socket\n", uAddr, uPort));
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
        iResult = send(pSocket->uSocket, pBuf, iLen, 0);
        pTo = &pSocket->RemoteAddr;
    }
    else
    {
        struct sockaddr_in6 SockAddr6;
        if (pSocket->iFamily == AF_INET6)
        {
            SockaddrInit6(&SockAddr6, AF_INET6);
            iToLen = sizeof(SockAddr6);
            SocketAddrMapTranslate(&pState->AddrMap, (struct sockaddr *)&SockAddr6, pTo, &iToLen);
            pTo = (struct sockaddr *)&SockAddr6;
        }
        iResult = sendto(pSocket->uSocket, pBuf, iLen, 0, pTo, iToLen);
    }

    // update data rate estimation
    SocketRateUpdate(&pSocket->SendRate, iResult, "send");

    // return bytes sent
    pSocket->iLastError = _XlatError(iResult);
    return(pSocket->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function SocketRecvfrom

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

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketRecvfrom(SocketT *pSocket, char *pBuf, int32_t iLen, int32_t iFlags, struct sockaddr *pFrom, int32_t *pFromLen)
{
    int32_t iRecv = 0, iRecvErr = 0;

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

    // sockets marked for async recv had actual receive operation take place in the thread
    if (pSocket->bAsyncRecv)
    {
        uint32_t bSocketPacketQueueFull;

        // acquire socket receive critical section
        NetCritEnter(&pSocket->RecvCrit);

        // remember if packet queue was full prior to calling _SocketRecvfromPacketQueue()
        bSocketPacketQueueFull = SocketPacketQueueStatus(pSocket->pRecvQueue, 'pful');

        /* given the socket could be either a TCP or UDP socket we handle the no data condition the same.
           this is due to the below, when we do error conversion we override the system error with EWOULDBLOCK because
           with TCP zero would be mean closed. if we are doing direct recv calls then the translation will
           convert based on the errno returned after the call */
        if ((iRecv = _SocketRecvfromPacketQueue(pSocket, pBuf, iLen, pFrom, pFromLen)) == 0)
        {
            iRecv = -1;
            iRecvErr = WSAEWOULDBLOCK;
        }

        // when data is obtained from the packet queue, we lose visibility on system socket errors
        pSocket->iLastError = SOCKERR_NONE;

        // release socket receive critical section
        NetCritLeave(&pSocket->RecvCrit);

        /* If packet queue had reached "full" state, then _SocketRecvThread had not been
           adding that socket to the event list anymore. Consequently, if _SocketRecvThread
           is currently blocked on WSAWaitForMultipleEvents(), it may not wake up if there is
           inbound traffic on this socket. To work around this, we wake it up explicitly here. */
        if (bSocketPacketQueueFull)
        {
            WSASetEvent(_Socket_pState->hEvent);
        }
    }
    else
    {
        iRecv = _SocketRecvfrom(pSocket, pBuf, iLen, pFrom, pFromLen, &iRecvErr);
    }

    // do error conversion
    iRecv = (iRecv == 0) ? SOCKERR_CLOSED : _XlatError0(iRecv, iRecvErr);

    // update data rate estimation
    SocketRateUpdate(&pSocket->RecvRate, pSocket->iLastError, "recv");

    // return the error code
    pSocket->iLastError = iRecv;
    return(pSocket->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function SocketInfo

    \Description
        Return information about an existing socket.

    \Input *pSocket - socket reference
    \Input iInfo    - selector for desired information
    \Input iData    - selector specific
    \Input *pBuf    - [out] return buffer
    \Input iLen     - buffer length

    \Output
        int32_t     - size of returned data or error code (negative value)

    \Notes
        iInfo can be one of the following:

        \verbatim
            'addr' - return local address
            'conn' - who we are connecting to
            'bind' - return bind data (if pSocket == NULL, get socket bound to given port)
            'bndu' - return bind data (only with pSocket=NULL, get SOCK_DGRAM socket bound to given port)
            '?ip6' - return TRUE if ipv4 address specified in iData is virtual, and fill in pBuf with ipv6 address if not NULL
            'ladr' - get local address previously specified by user for subsequent SocketBind() operations (pc only)
            'maxp' - return configured max packet size
            'maxr' - return configured max recv rate (bytes/sec; zero=uncapped)
            'maxs' - return configured max send rate (bytes/sec; zero=uncapped)
            'pdrp' - return socket packet queue number of packets dropped
            'peer' - peer info (only valid if connected)
            'pmax' - return socket packet queue max depth
            'pnum' - return socket packet queue current depth
            'ratr' - return current recv rate estimation (bytes/sec)
            'rats' - return current send rate estimation (bytes/sec)
            'sdcf' - get installed send callback function pointer (iData specifies index in array)
            'sdcu' - get installed send callback userdata pointer (iData specifies index in array)
            'serr' - last socket error
            'psiz' - return socket packet queue max size
            'sock' - return windows socket associated with the specified DirtySock socket
            'spam' - return debug level for debug output
            'stat' - TRUE if connected, else FALSE
            'virt' - TRUE if socket is virtual, else FALSE
        \endverbatim

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketInfo(SocketT *pSocket, int32_t iInfo, int32_t iData, void *pBuf, int32_t iLen)
{
    SocketStateT *pState = _Socket_pState;

    // always zero results by default
    #if defined(DIRTYCODE_XBOXONE)
    if (pBuf != NULL)
    #else
    if ((pBuf != NULL) && (iInfo != 'ladr'))
    #endif
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
        if (pSocket->bVirtual)
        {
            SockaddrInit((struct sockaddr *)pBuf, AF_INET);
            SockaddrInSetPort((struct sockaddr *)pBuf, pSocket->uVirtualPort);
            iResult = 0;
        }
        else if (pSocket->uSocket != INVALID_SOCKET)
        {
            struct sockaddr_in6 SockAddr6;
            iLen = sizeof(SockAddr6);
            if ((iResult = getsockname(pSocket->uSocket, (struct sockaddr *)&SockAddr6, &iLen)) == 0)
            {
                SockaddrInit((struct sockaddr *)pBuf, AF_INET);
                SockaddrInSetPort((struct sockaddr *)pBuf, SocketHtons(SockAddr6.sin6_port));
                SockaddrInSetAddr((struct sockaddr *)pBuf, SocketAddrMapAddress(&pState->AddrMap, (struct sockaddr *)&SockAddr6, sizeof(SockAddr6)));
            }
            iResult = _XlatError(iResult);
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

    // return socket protocol
    if (iInfo == 'prot')
    {
        return(pSocket->iProto);
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

    // return peer info (only valid if connected)
    if ((iInfo == 'conn') || (iInfo == 'peer'))
    {
        getpeername(pSocket->uSocket, (struct sockaddr *)pBuf, &iLen);
        //$$TODO this needs to be IPv6, translate to virtual address for output
        return(0);
    }
    // get packet queue info
    if ((iInfo == 'pdrp') || (iInfo == 'pmax') || (iInfo == 'psiz'))
    {
        int32_t iResult;
        // acquire socket receive critical section
        NetCritEnter(&pSocket->RecvCrit);
        // get packet queue status
        iResult = SocketPacketQueueStatus(pSocket->pRecvQueue, iInfo);
        // release socket receive critical section
        NetCritLeave(&pSocket->RecvCrit);
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

    // return last socket error
    if (iInfo == 'serr')
    {
        return(pSocket->iLastError);
    }

    // return windows socket identifier
    if (iInfo == 'sock')
    {
        if (pBuf != NULL)
        {
            if (iLen == (int32_t)sizeof(pSocket->uSocket))
            {
                ds_memcpy(pBuf, &pSocket->uSocket, sizeof(pSocket->uSocket));
            }
        }
        return((int32_t)pSocket->uSocket);
    }

    // return socket status
    if (iInfo == 'stat')
    {
        fd_set FdRead, FdWrite, FdExcept;
        struct timeval TimeVal;

        // if not a connected socket, return TRUE
        if (pSocket->iType != SOCK_STREAM)
        {
            return(1);
        }

        // if not connected, use select to determine connect
        if (pSocket->iOpened == 0)
        {
            // setup write/exception lists so we can select against the socket
            FD_ZERO(&FdWrite);
            FD_ZERO(&FdExcept);
            FD_SET(pSocket->uSocket, &FdWrite);
            FD_SET(pSocket->uSocket, &FdExcept);
            TimeVal.tv_sec = TimeVal.tv_usec = 0;
            if (select(pSocket->uSocket + 1, NULL, &FdWrite, &FdExcept, &TimeVal) > 0)
            {
                // if we got an exception, that means connect failed
                if (FdExcept.fd_count > 0)
                {
                    NetPrintfVerbose((pSocket->iVerbose, 0, "dirtynetwin: connection failed\n"));
                    pSocket->iOpened = -1;
                }
                // if socket is writable, that means connect succeeded
                else if (FdWrite.fd_count > 0)
                {
                    NetPrintfVerbose((pSocket->iVerbose, 0, "dirtynetwin: connection open\n"));
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
            FD_ZERO(&FdRead);
            FD_ZERO(&FdExcept);
            FD_SET(pSocket->uSocket, &FdRead);
            FD_SET(pSocket->uSocket, &FdExcept);
            TimeVal.tv_sec = TimeVal.tv_usec = 0;
            if (select(pSocket->uSocket + 1, &FdRead, NULL, &FdExcept, &TimeVal) > 0)
            {
                // if we got an exception, that means connect failed (usually closed by remote peer)
                if (FdExcept.fd_count > 0)
                {
                    NetPrintfVerbose((pSocket->iVerbose, 0, "dirtynetwin: connection failure\n"));
                    pSocket->iOpened = -1;
                }
                else if (FdRead.fd_count > 0)
                {
                    u_long uAvailBytes = 1; // u_long is required by ioctlsocket
                    // if socket is readable but there's no data available, connect was closed
                    // uAvailBytes might be less than actual bytes, so it can only be used for zero-test
                    if ((ioctlsocket(pSocket->uSocket, FIONREAD, &uAvailBytes) != 0) || (uAvailBytes == 0))
                    {
                        pSocket->iLastError = SOCKERR_CLOSED;
                        NetPrintfVerbose((pSocket->iVerbose, 0, "dirtynetwin: connection closed (wsaerr=%d)\n", (uAvailBytes==0) ? WSAECONNRESET : WSAGetLastError()));
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

    return(-1);
}

/*F*************************************************************************************/
/*!
    \Function SocketCallback

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

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
#pragma optimize("", off) // make sure this block of code is not reordered
int32_t SocketCallback(SocketT *pSocket, int32_t iMask, int32_t iIdle, void *pRef, int32_t (*pProc)(SocketT *pSocket, int32_t iFlags, void *pRef))
{
    pSocket->uCallIdle = iIdle;
    pSocket->uCallMask = iMask;
    pSocket->pCallRef = pRef;
    pSocket->pCallback = pProc;
    return(0);
}
#pragma optimize("", on)

/*F*************************************************************************************/
/*!
    \Function SocketControl

    \Description
        Process a control message (type specific operation)

    \Input *pSocket - socket to control, or NULL for module-level option
    \Input iOption  - the option to pass
    \Input iData1   - message specific parm
    \Input *pData2  - message specific parm
    \Input *pData3  - message specific parm

    \Output
        int32_t     - message specific result (-1=unsupported message, -2=no such module)

    \Notes
        iOption can be one of the following:

        \verbatim
            'afam' - set internet address family to use for socket operations (defaults to AF_INET6, AF_INET is also supported))
            'arcv' - set async receive enable/disable (default enabled for DGRAM/RAW, disabled for TCP)
            'conn' - handle connect message
            'disc' - handle disconnect message
            '+ip6' - add an IPv6 address into the mapping table and return a virtual IPv4 address to reference it
            '-ip6' - del an IPv6 address from the mapping table
            '~ip6' - remap an existing IPv6 address in the mapping table
            'keep' - set TCP keep-alive settings on PC (iData1=enable/disable, iData2=keep-alive time, iData3=keep-alive interval)
            'ladr' - set local address for subsequent SocketBind() operations (pc only)
            'maxp' - set max udp packet size
            'maxr' - set max recv rate (bytes/sec; zero=uncapped)
            'maxs' - set max send rate (bytes/sec; zero=uncapped)
            'nbio' - set nonblocking/blocking mode (TCP only, iData1=TRUE (nonblocking) or FALSE (blocking))
            'ndly' - set TCP_NODELAY state for given stream socket (iData1=zero or one)
            'pdev' - set simulated packet deviation
            'plat' - set simulated packet latency
            'plos' - set simulated packet loss
            'poll' - execute blocking wait on given socket list (pData2) or all sockets (pData2=NULL), 64 max sockets in milliseconds
            'poln' - execute blocking wait on given socket list (pData2) or all sockets (pData2=NULL), 64 max sockets in microseconds
            'pque' - set socket packet queue depth
            'push' - push data into given socket (iData1=size, pData2=data ptr, pData3=sockaddr ptr)
            'rbuf' - set socket recv buffer size
            'sbuf' - set socket send buffer size
            'scbk' - enable/disable "send callbacks usage" on specified socket (defaults to enable)
            'sdcb' - set/unset send callback (iData1=TRUE for set - FALSE for unset, pData2=callback, pData3=callref)
            'soli' - set SO_LINGER On the specified socket, iData1 is timeout in seconds
            'spam' - set debug level for debug output
            'vadd' - add a port to virtual port list
            'vdel' - del a port from virtual port list
        \endverbatim

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketControl(SocketT *pSocket, int32_t iOption, int32_t iData1, void *pData2, void *pData3)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iResult;

    // handle global control
    if (pSocket == NULL)
    {
        return(_SocketControlGlobal(iOption, iData1, pData2, pData3));
    }

    // set async recv enable
    if (iOption == 'arcv')
    {
        // set socket async recv flag
        pSocket->bAsyncRecv = iData1 ? TRUE : FALSE;
        // wake up recvthread to update socket polling
        WSASetEvent(pState->hEvent);
        return(0);
    }
    // set max recv rate
    if (iOption == 'maxr')
    {
        NetPrintf(("dirtynetwin: setting max recv rate to %d bytes/sec\n", iData1));
        pSocket->RecvRate.uMaxRate = iData1;
        return(0);
    }
    // set max send rate
    if (iOption == 'maxs')
    {
        NetPrintf(("dirtynetwin: setting max send rate to %d bytes/sec\n", iData1));
        pSocket->SendRate.uMaxRate = iData1;
        return(0);
    }
    // enable/disable "send callbacks usage" on specified socket (defaults to enable)
    if (iOption == 'scbk')
    {
        if (pSocket->bSendCbs != (iData1?TRUE:FALSE))
        {
            NetPrintf(("dirtynetwin: send callbacks usage changed from %s to %s for socket ref %p\n", (pSocket->bSendCbs?"ON":"OFF"), (iData1?"ON":"OFF"), pSocket));
            pSocket->bSendCbs = (iData1?TRUE:FALSE);
        }
        return(0);
    }
    // set debug level
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

    #if !defined(DIRTYCODE_XBOXONE)
    // configure TCP keep-alive
    if (iOption == 'keep')
    {
        struct tcp_keepalive TcpKeepAlive;
        DWORD dwBytesReturned;

        if (pSocket->iType != SOCK_STREAM)
        {
            NetPrintf(("dirtynetwin: [%p] 'keep' selector can only be used on a SOCK_STREAM socket\n", pSocket));
            return(-1);
        }
        if (pSocket->uSocket == INVALID_SOCKET)
        {
            NetPrintf(("dirtynetwin: [%p] 'keep' selector used with an invalid socket\n", pSocket));
            return(-1);
        }

        // initialize tcpkeep alive structure
        ds_memclr(&TcpKeepAlive, sizeof(TcpKeepAlive));

        TcpKeepAlive.onoff = (uint8_t)iData1;              // on/off
        if (TcpKeepAlive.onoff)
        {
            TcpKeepAlive.keepalivetime = *(uint32_t*)pData2;     // timeout in ms
            TcpKeepAlive.keepaliveinterval = *(uint32_t*)pData3; // interval in ms
        }
        else
        {
            TcpKeepAlive.keepalivetime = 0;     // timeout in ms
            TcpKeepAlive.keepaliveinterval = 0; // interval in ms
        }

        if ((iResult = WSAIoctl(pSocket->uSocket, SIO_KEEPALIVE_VALS, (void *)&TcpKeepAlive, sizeof(TcpKeepAlive), NULL, 0, &dwBytesReturned, NULL, NULL)) == 0)
        {
            pSocket->iLastError = SOCKERR_NONE;
            NetPrintfVerbose((pSocket->iVerbose, 1, "dirtynetwin: [%p] successfully %s the TCP keep-alive (timeout=%dms, interval=%dms)\n", pSocket,
                iData1?"enabled":"disabled", TcpKeepAlive.keepalivetime, TcpKeepAlive.keepaliveinterval));
        }
        else
        {
            pSocket->iLastError = _XlatError(iResult);
            NetPrintf(("dirtynetwin: [%p] failed to %s TCP keep-alive for low-level socket (err = %d)\n", pSocket,
                iData1?"enable":"disable", WSAGetLastError()));
        }

        return(pSocket->iLastError);
    }
    #endif
    // if a stream socket, set nonblocking/blocking mode
    if ((iOption == 'nbio') && (pSocket->iType == SOCK_STREAM))
    {
        uint32_t uNbio = (uint32_t)iData1;
        iResult = ioctlsocket(pSocket->uSocket, FIONBIO, (u_long *)&uNbio);
        pSocket->iLastError = _XlatError(iResult);
        NetPrintf(("dirtynetwin: setting socket 0x%p to %s mode %s (LastError=%d).\n", pSocket, iData1 ? "nonblocking" : "blocking", iResult ? "failed" : "succeeded", pSocket->iLastError));
        return(pSocket->iLastError);
    }
    // if a stream socket, set TCP_NODELAY state
    if ((iOption == 'ndly') && (pSocket->iType == SOCK_STREAM))
    {
        iResult = setsockopt(pSocket->uSocket, IPPROTO_TCP, TCP_NODELAY, (const char *)&iData1, sizeof(iData1));
        pSocket->iLastError = _XlatError(iResult);
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
        if (pSocket->bAsyncRecv)
        {
            pSocket->iPacketQueueResizePending = iData1;
        }
        else
        {
            // acquire socket receive critical section
            NetCritEnter(&pSocket->RecvCrit);
            // resize the queue
            pSocket->pRecvQueue = SocketPacketQueueResize(pSocket->pRecvQueue, iData1, pState->iMemGroup, pState->pMemGroupUserData);
            // release socket receive critical section
            NetCritLeave(&pSocket->RecvCrit);
        }
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
            NetPrintf(("dirtynetwin: request to push %d bytes of data discarded (max=%d)\n", iData1, SOCKET_MAXUDPRECV));
            NetCritLeave(&pSocket->RecvCrit);
            return(-1);
        }

        // add packet to queue
        SocketPacketQueueAdd(pSocket->pRecvQueue, (uint8_t *)pData2, iData1, (struct sockaddr *)pData3);

        // release socket critical section
        NetCritLeave(&pSocket->RecvCrit);

        // see if we should issue callback
        if ((pSocket->pCallback != NULL) && (pSocket->uCallMask & CALLB_RECV))
        {
            pSocket->pCallback(pSocket, 0, pSocket->pCallRef);
        }
        return(0);
    }
    // set REUSEADDR
    if (iOption == 'radr')
    {
        iResult = setsockopt(pSocket->uSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&iData1, sizeof(iData1));
        pSocket->iLastError = _XlatError(iResult);
        return(pSocket->iLastError);
    }
    // set socket receive buffer size
    if ((iOption == 'rbuf') || (iOption == 'sbuf'))
    {
        int32_t iOldSize, iNewSize, iOptLen=4;
        int32_t iSockOpt = (iOption == 'rbuf') ? SO_RCVBUF : SO_SNDBUF;

        // get current buffer size
        getsockopt(pSocket->uSocket, SOL_SOCKET, iSockOpt, (char *)&iOldSize, &iOptLen);

        // set new size
        iResult = setsockopt(pSocket->uSocket, SOL_SOCKET, iSockOpt, (const char *)&iData1, sizeof(iData1));
        pSocket->iLastError = _XlatError(iResult);

        // get new size
        getsockopt(pSocket->uSocket, SOL_SOCKET, iSockOpt, (char *)&iNewSize, &iOptLen);
        NetPrintf(("dirtynetwin: setsockopt(%s) changed buffer size from %d to %d\n", (iOption == 'rbuf') ? "SO_RCVBUF" : "SO_SNDBUF",
            iOldSize, iNewSize));

        return(pSocket->iLastError);
    }
    // set SO_LINGER
    if (iOption == 'soli')
    {
        struct linger lingerOptions;
        lingerOptions.l_onoff = TRUE;
        lingerOptions.l_linger = iData1;
        iResult = setsockopt(pSocket->uSocket, SOL_SOCKET, SO_LINGER, (const char *)&lingerOptions, sizeof(lingerOptions));
        pSocket->iLastError = _XlatError(iResult);
        return(pSocket->iLastError);
    }
    // unhandled
    return(-1);
}

/*F*************************************************************************************/
/*!
    \Function SocketGetLocalAddr

    \Description
        Returns the "external" local address (ie, the address as a machine "out on
        the Internet" would see as the local machine's address).

    \Output
        uint32_t        - local address

    \Version 07/28/2003 (jbrookes)
*/
/************************************************************************************F*/
uint32_t SocketGetLocalAddr(void)
{
    #if defined(DIRTYCODE_XBOXONE)
    SocketStateT *pState = _Socket_pState;
    struct sockaddr InetAddr, HostAddr;

    if ((pState->uLocalAddr == 0) || (pState->uLocalAddr == 0x7f000001))
    {
        // create a remote internet address
        ds_memclr(&InetAddr, sizeof(InetAddr));
        InetAddr.sa_family = AF_INET;
        SockaddrInSetPort(&InetAddr, 79);
        SockaddrInSetAddr(&InetAddr, 0x9f990000);

        // ask socket to give us local address that can connect to it
        ds_memclr(&HostAddr, sizeof(HostAddr));
        SocketHost(&HostAddr, sizeof(HostAddr), &InetAddr, sizeof(InetAddr));

        pState->uLocalAddr = SockaddrInGetAddr(&HostAddr);
    }
    return(pState->uLocalAddr);
    #else
    struct sockaddr InetAddr, HostAddr;

    // create a remote internet address
    ds_memclr(&InetAddr, sizeof(InetAddr));
    InetAddr.sa_family = AF_INET;
    SockaddrInSetPort(&InetAddr, 79);
    SockaddrInSetAddr(&InetAddr, 0x9f990000);

    // ask socket to give us local address that can connect to it
    ds_memclr(&HostAddr, sizeof(HostAddr));
    SocketHost(&HostAddr, sizeof(HostAddr), &InetAddr, sizeof(InetAddr));

    return(SockaddrInGetAddr(&HostAddr));
    #endif
}

/*F*************************************************************************************/
/*!
    \Function _SocketLookupDone

    \Description
        Callback to determine if gethostbyname is complete.

    \Input *pHost   - pointer to host lookup record

    \Output
        int32_t     - zero=in progess, neg=done w/error, pos=done w/success

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
static int32_t _SocketLookupDone(HostentT *pHost)
{
    // return current status
    return(pHost->done);
}

/*F*************************************************************************************/
/*!
    \Function _SocketLookupFree

    \Description
        Release resources used by gethostbyname.

    \Input *pHost   - pointer to host lookup record

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
static void _SocketLookupFree(HostentT *pHost)
{
    // release resource
    pHost->refcount -= 1;
}

/*F*************************************************************************************/
/*!
    \Function _SocketLookupThread

    \Description
        Socket lookup thread

    \Input *pUserData   - pointer to host lookup record

    \Output
        int32_t     - zero

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
static void _SocketLookupThread(void *pUserData)
{
    SocketStateT *pState = _Socket_pState;
    HostentT *pHost = (HostentT *)pUserData;
    struct addrinfo Hints, *pList = NULL;
    int32_t iResult;

    // setup lookup hints
    ds_memclr(&Hints, sizeof(Hints));
    Hints.ai_family = AF_UNSPEC;
    Hints.ai_socktype = SOCK_STREAM; // set specific socktype to limit to one result per type
    Hints.ai_protocol = IPPROTO_TCP; // set specific proto to limit to one result per type
    Hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;

    // perform the blocking dns query
    if ((iResult = getaddrinfo(pHost->name, NULL, &Hints, &pList)) == 0)
    {
        struct addrinfo *pAddrInfo;

        // first loop we look for IPv4 addresses (prefer IPv4 to IPv6)
        for (pAddrInfo = pList; pAddrInfo != NULL; pAddrInfo = pAddrInfo->ai_next)
        {
            // verbose logging of address info if spam settings warrant
            NetPrintfVerbose((pState->iVerbose, 1, "dirtynetwin: addr=%A\n", pAddrInfo->ai_addr));
            NetPrintfVerbose((pState->iVerbose, 2, "dirtynetwin:   ai_flags=0x%08x\n", pAddrInfo->ai_flags));
            NetPrintfVerbose((pState->iVerbose, 2, "dirtynetwin:   ai_family=%d\n", pAddrInfo->ai_family));
            NetPrintfVerbose((pState->iVerbose, 2, "dirtynetwin:   ai_socktype=%d\n", pAddrInfo->ai_socktype));
            NetPrintfVerbose((pState->iVerbose, 2, "dirtynetwin:   ai_protocol=%d\n", pAddrInfo->ai_protocol));
            NetPrintfVerbose((pState->iVerbose, 2, "dirtynetwin:   name=%s\n", pAddrInfo->ai_canonname));

            // extract first IPv4 address we come across
            if ((pHost->addr == 0) && (pAddrInfo->ai_family == AF_INET))
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
        NetPrintfVerbose((pState->iVerbose, 0, "dirtynetwin: %s=%a\n", pHost->name, pHost->addr));

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
        NetPrintf(("dirtynetwin: getaddrinfo('%s', ...) failed err=%d\n", pHost->name, iResult));
        pHost->done = -1;
    }

    #if !defined(DIRTYCODE_XBOXONE)
    // note thread completion
    pHost->thread = 1;
    #endif
    // release thread-allocated refcount on hostname resource
    pHost->refcount -= 1;
}

/*F*************************************************************************************/
/*!
    \Function SocketLookup

    \Description
        Lookup a host by name and return the corresponding Internet address. Uses
        a callback/polling system since the socket library does not allow blocking.

    \Input *pText   - pointer to null terminated address string
    \Input iTimeout - number of milliseconds to wait for completion

    \Output
        HostentT *  - hostent struct that includes callback vectors

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
HostentT *SocketLookup(const char *pText, int32_t iTimeout)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iAddr, iResult;
    HostentT *pHost, *pHostRef;
    DirtyThreadConfigT ThreadConfig;

    NetPrintf(("dirtynetwin: looking up address for host '%s'\n", pText));

    // dont allow negative timeouts
    if (iTimeout < 0)
    {
        return(NULL);
    }

    // create new structure
    pHost = DirtyMemAlloc(sizeof(*pHost), SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    ds_memclr(pHost, sizeof(*pHost));

    // setup callbacks
    pHost->Done = &_SocketLookupDone;
    pHost->Free = &_SocketLookupFree;
    // copy over the target address
    ds_strnzcpy(pHost->name, pText, sizeof(pHost->name));

    // check for refcounted lookup
    if ((pHostRef = SocketHostnameAddRef(&pState->pHostList, pHost, TRUE)) != NULL)
    {
        DirtyMemFree(pHost, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(pHostRef);
    }

    // check for dot notation, then check hostname cache
    if (((iAddr = SocketInTextGetAddr(pText)) != 0) || ((iAddr = SocketHostnameCacheGet(pState->pHostnameCache, pText, pState->iVerbose)) != 0))
    {
        // save the data
        pHost->addr = iAddr;
        pHost->done = 1;
        // mark as "thread complete" so _SocketLookupFree will release memory
        pHost->thread = 1;
        // return completed record
        return(pHost);
    }

    /* add an extra refcount for the thread; this ensures the host structure survives until the thread
       is done with it.  this must be done before thread creation. */
    pHost->refcount += 1;

    // configure dns thread parameters
    ds_memclr(&ThreadConfig, sizeof(ThreadConfig));
    ThreadConfig.pName = "SocketLookup";
    ThreadConfig.iAffinity = NetConnStatus('affn', 0, NULL, 0);
    ThreadConfig.iVerbosity = pState->iVerbose-1;

    // create dns lookup thread
    if ((iResult = DirtyThreadCreate(_SocketLookupThread, pHost, &ThreadConfig)) != 0)
    {
        NetPrintf(("dirtynetwin: unable to create socket lookup thread (err=%d)\n", iResult));
        pHost->done = -1;
        // remove refcount we just added
        pHost->refcount -= 1;
    }

    // return the host reference
    return(pHost);
}

/*F*************************************************************************************/
/*!
    \Function SocketHost

    \Description
        Return the host address that would be used in order to communicate with the given
        destination address.

    \Input *pHost    - local sockaddr struct
    \Input iHostLen  - length of structure (sizeof(host))
    \Input *pDest    - remote sockaddr struct
    \Input iDestLen  - length of structure (sizeof(dest))

    \Output
        int32_t      - zero=success, negative=error

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketHost(struct sockaddr *pHost, int32_t iHostLen, const struct sockaddr *pDest, int32_t iDestLen)
{
    SocketStateT *pState = _Socket_pState;
    struct sockaddr SockAddr;
    char strHostName[256];
    int32_t iErr;
    SOCKET iSock;

    // must be same kind of addresses
    if (iHostLen != iDestLen)
    {
        return(-1);
    }

    // do family specific lookup
    if (pDest->sa_family == AF_INET)
    {
        // make them match initially
        ds_memcpy(pHost, pDest, iHostLen);

        #if !defined(DIRTYCODE_XBOXONE)
        // respect adapter override
        if (pState->uAdapterAddress != 0)
        {
            SockaddrInSetAddr(pHost, pState->uAdapterAddress);
            return(0);
        }
        #endif

        // zero the address portion
        pHost->sa_data[2] = pHost->sa_data[3] = pHost->sa_data[4] = pHost->sa_data[5] = 0;

        // create a temp socket (must be datagram)
        iSock = socket(AF_INET, SOCK_DGRAM, 0);
        if (iSock != INVALID_SOCKET)
        {
            // use routing check if winsock 2.0
            if (pState->iVersion >= 0x200)
            {
                // get interface that would be used for this dest
                ds_memclr(&SockAddr, sizeof(SockAddr));
                if (WSAIoctl(iSock, SIO_ROUTING_INTERFACE_QUERY, (void *)pDest, iDestLen, &SockAddr, sizeof(SockAddr), (LPDWORD)&iHostLen, NULL, NULL) < 0)
                {
                    iErr = WSAGetLastError();
                    NetPrintf(("dirtynetwin: WSAIoctl(SIO_ROUTING_INTERFACE_QUERY) returned %d\n", iErr));
                }
                else
                {
                    // copy over the result
                    ds_memcpy(pHost->sa_data+2, SockAddr.sa_data+2, 4);
                }

                // convert loopback to requested address
                if (SockaddrInGetAddr(pHost) == INADDR_LOOPBACK)
                {
                    ds_memcpy(pHost->sa_data+2, pDest->sa_data+2, 4);
                }
            }

            // if no result, try connecting to socket then reading
            // (this only works on some non-microsoft stacks)
            if (SockaddrInGetAddr(pHost) == 0)
            {
                // connect to remote address
                if (connect(iSock, pDest, iDestLen) == 0)
                {
                    // query the host address
                    if (getsockname(iSock, &SockAddr, &iHostLen) == 0)
                    {
                        // just copy over the address portion
                        ds_memcpy(pHost->sa_data+2, SockAddr.sa_data+2, 4);
                    }
                }
            }

            // $$TODO evaluate possibility of unifying - (keep PC version)
            #if defined(DIRTYCODE_XBOXONE) && !defined(DIRTYCODE_GDK)
            // if still no result, use classic gethosthame/gethostbyname
            // (works for microsoft, not for non-microsoft stacks)
            if (SockaddrInGetAddr(pHost) == 0)
            {
                struct hostent *pHostent;

                // get machine name
                gethostname(strHostName, sizeof(strHostName));
                // lookup ip info
                pHostent = gethostbyname(strHostName);
                if (pHostent != NULL)
                {
                    ds_memcpy(pHost->sa_data+2, pHostent->h_addr_list[0], 4);
                }
            }
            #else
            // if still no result, use classic gethosthame/getaddrinfo
            // (works for microsoft, not for non-microsoft stacks)
            if (SockaddrInGetAddr(pHost) == 0)
            {
                int32_t iResult;
                struct addrinfo Hints, *pList = NULL;

                // get machine name
                ds_memclr(&Hints, sizeof(Hints));
                Hints.ai_family = AF_INET;
                Hints.ai_socktype = SOCK_STREAM;
                Hints.ai_protocol = IPPROTO_TCP;
                gethostname(strHostName, sizeof(strHostName));
                // lookup ip info
                if ((iResult = getaddrinfo(strHostName, NULL, &Hints, &pList)) == 0)
                {
                    ds_memcpy(pHost->sa_data+2, pList->ai_addr->sa_data+2, 4);
                }
                else
                {
                    NetPrintf(("dirtynetwin: getaddrinfo('%s', ...) failed err=%d\n", strHostName, iResult));
                }

                // release memory
                freeaddrinfo(pList);
            }
            #endif

            // close the socket
            closesocket(iSock);
        }
        return(0);
    }

    // unsupported family
    ds_memclr(pHost, iHostLen);
    return(-3);
}
