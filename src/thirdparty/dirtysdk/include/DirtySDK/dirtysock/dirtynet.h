/*H*************************************************************************************/
/*!
    \File    dirtynet.h

    \Description
        Platform independent interface to network layers. Based on BSD sockets, but with
        performance modifications. Allows truly portable modules to be written and moved
        to different platforms needing only different support wrappers (no change to
        actual network modes).

    \Copyright
        Copyright (c) Electronic Arts 2001-2014

    \Version 0.5 08/01/2001 (gschaefer) First Version
    \Version 1.1 01/02/2002 (gschaefer) First Release
*/
/*************************************************************************************H*/

#ifndef _dirtynet_h
#define _dirtynet_h

/*!
\Moduledef DirtyNet DirtyNet
\Modulemember DirtySock
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

/*
  Include system socket/inet headers, and/or define what we need ourselves
*/
#if defined(DIRTYCODE_NX)
#include <nn/socket/sys/cdefs.h>
#include <nn/socket/netinet/in.h>
#include <nn/socket/sys/types.h>
#include <nn/socket/sys/socket.h>

#elif !defined(DIRTYCODE_XBOXONE) && !defined(DIRTYCODE_PC) && !defined(DIRTYCODE_GDK)

#include <sys/socket.h>
#include <netinet/in.h>         /* struct sockaddr_in */
#include <arpa/inet.h>          /* struct in_addr */

#if defined(DIRTYCODE_PS4)
#define IPPROTO_IPV4    4
#ifdef INADDR_BROADCAST
 #undef INADDR_BROADCAST
#endif
#ifdef INADDR_ANY
 #undef INADDR_ANY
 #define INADDR_ANY 0x00000000
#endif
#ifdef INADDR_LOOPBACK
 #undef INADDR_LOOPBACK
 #define INADDR_LOOPBACK 0x7f000001
#endif

#endif // defined(DIRTYCODE_PS4)

#else // DIRTYCODE_PC or DIRTYCODE_XBOXONE

#if !defined(_WINSOCKAPI_) || defined(EA_FORCE_WINSOCK2_INCLUDE) // if <winsock.h> was included, skip, unless we want to force include it
#include <winsock2.h>
#include <ws2tcpip.h>
#endif // _WINSOCKAPI_

#endif // including system socket/inet headers


/*
    IPv6 definitions for common code for platforms that don't have them,
    until if/when they are available natively
*/

#if defined(DIRTYCODE_PS4)
#if !defined(s6_addr) // not sure what ipv6 definitions will look like but this is a likely definition we can check for
struct in6_addr {
    uint8_t         s6_addr[16];   /* IPv6 address */
};

struct sockaddr_in6 {
    uint16_t        sin6_family;   /* AF_INET6 */
    uint16_t        sin6_port;     /* port number */
    uint32_t        sin6_flowinfo; /* IPv6 flow information */
    struct in6_addr sin6_addr;     /* IPv6 address */
    uint32_t        sin6_scope_id; /* Scope ID (new in 2.4) */
};
#endif // !defined(s6_addr)
#endif // DIRTYCODE_PS4


/*
  DirtyNet specific defines
*/

#define SOCK_NORECV 1           //!< caller does not want to receive more data
#define SOCK_NOSEND 2           //!< caller does not want to send more data

#if !defined (DIRTYCODE_NX)
#ifndef INADDR_BROADCAST
#define INADDR_BROADCAST    0xffffffff
#endif
#endif

#define CALLB_NONE 0            //!< no callback
#define CALLB_SEND 1            //!< call when we can send
#define CALLB_RECV 2            //!< call when we can receive

#define SOCKLOOK_LOCALADDR      "*"

#define SOCKLOOK_FLAGS_ALLOWXDNS    (1)

//! maximum udp packet size we can receive (constrained by Xbox 360 max VDP packet size)
#define SOCKET_MAXUDPRECV       (1264)

//! maximum number of virtual ports that can be specified
#define SOCKET_MAXVIRTUALPORTS  (32)

// Errors

#define SOCKERR_NONE 0          //!< no error
#define SOCKERR_CLOSED -1       //!< the socket is closed
#define SOCKERR_NOTCONN -2      //!< the socket is not connected
#define SOCKERR_BLOCKED -3      //!< operation would result in blocking
#define SOCKERR_ADDRESS -4      //!< the address is invalid
#define SOCKERR_UNREACH -5      //!< network cannot be accessed by this host
#define SOCKERR_REFUSED -6      //!< connection refused by the recipient
#define SOCKERR_OTHER   -7      //!< unclassified error
#define SOCKERR_NOMEM   -8      //!< out of memory
#define SOCKERR_NORSRC  -9      //!< out of resources
#define SOCKERR_UNSUPPORT -10   //!< unsupported operation
#define SOCKERR_INVALID -11     //!< resource or operation is invalid
#define SOCKERR_ADDRINUSE -12   //!< address already in use
#define SOCKERR_CONNRESET -13   //!< connection has been reset
#define SOCKERR_BADPIPE -14     //!< EBADF or EPIPE

//! error occured when trying to map address using SocketAddrMapAddress (+ip6)/SocketAddrRemapAddress (~ip6)
#define SOCKMAP_ERROR           (-1)

//! maximum number of send callbacks supported (internal use only - needs to be public for DirtyCast stress tester)
#define SOCKET_MAXSENDCALLBACKS (8)


/*** Macros ****************************************************************************/

/*
Macros used to write into a sockaddr structure take into account the fact that the sa_data field is defined
as an array of signed char on Windows and an array of unsigned char on other platforms. Doing this is required
to avoid C4365 warning occurring when these macros are used in .cpp files. (The same warning would not occur
when using these macros in .c files. The windows compiler (cl.exe) only warns about C4365 with .cpp files.
*/
#if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_PC)
#define SA_DATA_TYPE char
#else
#define SA_DATA_TYPE unsigned char
#endif

//! init a sockaddr to zero and set its family type
#define SockaddrInit(addr,fam)      { (addr)->sa_family = (fam); ds_memclr((addr)->sa_data, sizeof((addr)->sa_data)); }

//! get the port in host format from sockaddr
#define SockaddrInGetPort(addr)     ((((unsigned char)(addr)->sa_data[0])<<8)|(((unsigned char)(addr)->sa_data[1])<<0))

//! set the port in host format in a sockaddr
#define SockaddrInSetPort(addr,val) { (addr)->sa_data[0] = (SA_DATA_TYPE)(((val)>>8)&0xff); (addr)->sa_data[1] = (SA_DATA_TYPE)((val)&0xff); }

//! get the address in host format from sockaddr
#define SockaddrInGetAddr(addr)     (((((((unsigned char)((addr)->sa_data[2])<<8)|(unsigned char)((addr)->sa_data[3]))<<8)|(unsigned char)((addr)->sa_data[4]))<<8)|(unsigned char)((addr)->sa_data[5]))

//! set the address in host format in a sockaddr
#define SockaddrInSetAddr(addr,val) { uint32_t val2 = (val); (addr)->sa_data[5] = (SA_DATA_TYPE)val2; val2 >>= 8; (addr)->sa_data[4] = (SA_DATA_TYPE)val2; val2 >>= 8; (addr)->sa_data[3] = (SA_DATA_TYPE)val2; val2 >>= 8; (addr)->sa_data[2] = (SA_DATA_TYPE)val2; }

//! get the misc field in host format from sockaddr
#define SockaddrInGetMisc(addr)     (((((((unsigned char)((addr)->sa_data[6])<<8)|(unsigned char)((addr)->sa_data[7]))<<8)|(unsigned char)((addr)->sa_data[8]))<<8)|(unsigned char)((addr)->sa_data[9]))

//! set the misc field in host format in a sockaddr
#define SockaddrInSetMisc(addr,val) { uint32_t val2 = (val); (addr)->sa_data[9] = (SA_DATA_TYPE)val2; val2 >>= 8; (addr)->sa_data[8] = (SA_DATA_TYPE)val2; val2 >>= 8; (addr)->sa_data[7] = (SA_DATA_TYPE)val2; val2 >>= 8; (addr)->sa_data[6] = (SA_DATA_TYPE)val2; }

//! detect loopback address (family independent)
#define SockaddrIsLoopback(addr)   (( ((addr)->sa_family == AF_INET) && ((addr)->sa_data[0] == 127) && ((addr)->sa_data[1] == 0) && ((addr)->sa_data[2] == 0) && ((addr)->sa_data[3] == 1) ))

/*
  sockaddr_v6 helpers
*/

//! init a sockaddr6 to zero and set its family type
#define SockaddrInit6(addr,fam)     { ds_memclr(addr, sizeof(*(addr))); (addr)->sin6_family = (fam); }

//! get IPv6 address from IPv4-mapped IPv6 address, also works for NAT64
#define SockaddrIn6GetAddr4(addr)   (((((((uint8_t)((addr)->sin6_addr.s6_addr[12])<<8)|(uint8_t)((addr)->sin6_addr.s6_addr[13]))<<8)|(uint8_t)((addr)->sin6_addr.s6_addr[14]))<<8)|(uint8_t)((addr)->sin6_addr.s6_addr[15]))

/*** Type Definitions ******************************************************************/


// basic socket type is a pointer
typedef struct SocketT SocketT;

//! a host lookup structure -- uses a callback
//! system to determine when lookup has finished
typedef struct HostentT
{
    int32_t done;                       //!< public: indicates when lookup is complete
    uint32_t addr;                      //!< public: resolved host address

    int32_t (*Done)(struct HostentT *); //!< public: callback to indicate completion status
    void (*Free)(struct HostentT *);    //!< public: callback to release Hostent structure

    char name[256];                     //!< private   (the maximum DNS name length is 253 characters)
    int32_t sema;                       //!< private
    int32_t thread;                     //!< private
    void* pData;                        //!< private
    uint32_t timeout;                   //!< private
    struct HostentT *pNext;             //!< private
    int32_t refcount;                   //!< private
} HostentT;

//! global socket send callback
typedef int32_t (SocketSendCallbackT)(SocketT *pSocket, int32_t iType, const uint8_t *pData, int32_t iDataSize, const struct sockaddr *pTo, void *pCallref);

//! socket send callback entry (internal use only - needs to be public for DirtyCast stress tester)
typedef struct SocketSendCallbackEntryT
{
    SocketSendCallbackT *pSendCallback; //!< global send callback
    void                *pSendCallref;  //!< user callback data
} SocketSendCallbackEntryT;


/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*
    platform independent functions, implemented in dirtynet.c
*/

// compare two sockaddr structures
DIRTYCODE_API int32_t SockaddrCompare(const struct sockaddr *pAddr1, const struct sockaddr *pAddr2);

// set the address in text format in a sockaddr
DIRTYCODE_API int32_t SockaddrInSetAddrText(struct sockaddr *pAddr, const char *pStr);

// get the address in text format from sockaddr
DIRTYCODE_API char *SockaddrInGetAddrText(const struct sockaddr *pAddr, char *pStr, int32_t iLen);

// parse address:port combination
DIRTYCODE_API int32_t SockaddrInParse(struct sockaddr *pAddr, const char *pParse);

// parse address:port:port combination into separate components
DIRTYCODE_API int32_t SockaddrInParse2(uint32_t *pAddr, int32_t *pPort, int32_t *pPort2, const char *pParse);

// convert 32-bit internet address into textual form
DIRTYCODE_API char *SocketInAddrGetText(uint32_t uAddr, char *pStr, int32_t iLen);

// convert textual internet address into 32-bit integer form
DIRTYCODE_API int32_t SocketInTextGetAddr(const char *pAddrText);

// convert int16_t from host to network byte order
DIRTYCODE_API uint16_t SocketHtons(uint16_t uAddr);

// convert int32_t from host to network byte order
DIRTYCODE_API uint32_t SocketHtonl(uint32_t uAddr);

// convert int16_t from network to host byte order
DIRTYCODE_API uint16_t SocketNtohs(uint16_t uAddr);

// convert int32_t from network to host byte order.
DIRTYCODE_API uint32_t SocketNtohl(uint32_t uAddr);

/*
    platform dependent functions, implemented in dirtynet<platform>.c
*/

// create new instance of socket interface module
DIRTYCODE_API int32_t SocketCreate(int32_t iThreadPrio, int32_t iThreadStackSize, int32_t iThreadCpuAffinity);

// release resources and destroy module.
DIRTYCODE_API int32_t SocketDestroy(uint32_t uShutdownFlags);

// create a new transfer endpoint
DIRTYCODE_API SocketT *SocketOpen(int32_t af, int32_t type, int32_t protocol);

// perform partial/complete shutdown of socket
DIRTYCODE_API int32_t SocketShutdown(SocketT *pSocket, int32_t how);

// close a socket
DIRTYCODE_API int32_t SocketClose(SocketT *pSocket);

// import a socket - may be SocketT pointer or sony socket ref
DIRTYCODE_API SocketT *SocketImport(intptr_t uSockRef);

// release an imported socket
DIRTYCODE_API void SocketRelease(SocketT *pSocket);

// bind a local address/port to a socket
DIRTYCODE_API int32_t SocketBind(SocketT *pSocket, const struct sockaddr *name, int32_t namelen);

// return information about an existing socket.
DIRTYCODE_API int32_t SocketInfo(SocketT *pSocket, int32_t iInfo, int32_t iData, void *pBuf, int32_t iLen);

// send a control message to the dirtysock layer
DIRTYCODE_API int32_t SocketControl(SocketT *pSocket, int32_t option, int32_t data1, void *data2, void *data3);

// start listening for an incoming connection on the socket
DIRTYCODE_API int32_t SocketListen(SocketT *pSocket, int32_t backlog);

// attempt to accept an incoming connection from a
DIRTYCODE_API SocketT *SocketAccept(SocketT *pSocket, struct sockaddr *addr, int32_t *addrlen);

// initiate a connection attempt to a remote host
DIRTYCODE_API int32_t SocketConnect(SocketT *pSocket, struct sockaddr *name, int32_t namelen);

// send data to a remote host
DIRTYCODE_API int32_t SocketSendto(SocketT *pSocket, const char *buf, int32_t len, int32_t flags, const struct sockaddr *to, int32_t tolen);

// same as SocketSendto() with "to" set to NULL
#define SocketSend(_pSocket, _pBuf, iLen, iFlags)   SocketSendto(_pSocket, _pBuf, iLen, iFlags, NULL, 0)

// receive data from a remote host
DIRTYCODE_API int32_t SocketRecvfrom(SocketT *pSocket, char *buf, int32_t len, int32_t flags, struct sockaddr *from, int32_t *fromlen);

// same as SocketRecvfrom() with "from" set to NULL.
#define SocketRecv(_pSocket, pBuf, iLen, iFlags)   SocketRecvfrom(_pSocket, pBuf, iLen, iFlags, NULL, 0)

// register a callback routine for notification of socket events
DIRTYCODE_API int32_t SocketCallback(SocketT *pSocket, int32_t flags, int32_t timeout, void *ref, int32_t (*proc)(SocketT *pSocket, int32_t flags, void *ref));

// return the host address that would be used in order to communicate with the given destination address.
DIRTYCODE_API int32_t SocketHost(struct sockaddr *host, int32_t hostlen, const struct sockaddr *dest, int32_t destlen);

// lookup a host by name and return the corresponding Internet address
DIRTYCODE_API HostentT *SocketLookup(const char *text, int32_t timeout);

// lookup a host by name and return the corresponding Internet address, with flags
#define SocketLookup2(_text, _timeout, _flags) SocketLookup(_text, _timeout)

// invoke all registered send callbacks (internal use only - needs to be public for DirtyCast stress tester)
int32_t SocketSendCallbackInvoke(SocketSendCallbackEntryT aCbEntries[], SocketT *pSocket, int32_t iType, const char *pBuf, int32_t iLen, const struct sockaddr *pTo);


// return "external" local address
DIRTYCODE_API uint32_t SocketGetLocalAddr(void);

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtynet_h


