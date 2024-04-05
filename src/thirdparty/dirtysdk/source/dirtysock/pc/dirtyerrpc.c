/*H********************************************************************************/
/*!
    \File dirtyerrpc.c

    \Description
        Dirtysock debug error routines.

    \Copyright
        Copyright (c) 2014 Electronic Arts Inc.

    \Version 07/01/2014 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

#if DIRTYSOCK_ERRORNAMES

static DirtyErrT _DirtyErr_List[] =
{
    /*
        WinError.h"
    */
    DIRTYSOCK_ErrorName(ERROR_INVALID_FUNCTION),    // 1
    DIRTYSOCK_ErrorName(ERROR_ACCESS_DENIED),       // 5
    DIRTYSOCK_ErrorName(ERROR_NO_MORE_FILES),       // 18
    DIRTYSOCK_ErrorName(ERROR_ALREADY_ASSIGNED),    // 85
    DIRTYSOCK_ErrorName(ERROR_INVALID_PARAMETER),   // 87
    DIRTYSOCK_ErrorName(ERROR_INSUFFICIENT_BUFFER), // 122
    DIRTYSOCK_ErrorName(ERROR_ALREADY_EXISTS),      // 183
    DIRTYSOCK_ErrorName(ERROR_NO_DATA),             // 232
    DIRTYSOCK_ErrorName(ERROR_INVALID_PORT_ATTRIBUTES), // 545
    DIRTYSOCK_ErrorName(ERROR_IO_INCOMPLETE),       // 996
    DIRTYSOCK_ErrorName(ERROR_IO_PENDING),          // 997
    DIRTYSOCK_ErrorName(ERROR_NOT_FOUND),           // 1168
    DIRTYSOCK_ErrorName(ERROR_NO_MATCH),            // 1169
    DIRTYSOCK_ErrorName(ERROR_CONNECTION_INVALID),  // 1229
    DIRTYSOCK_ErrorName(ERROR_SERVICE_NOT_FOUND),   // 1243
    DIRTYSOCK_ErrorName(ERROR_FUNCTION_FAILED),     // 1627

    DIRTYSOCK_ErrorName(RPC_S_CALL_FAILED),         // 1726L
    DIRTYSOCK_ErrorName(RPC_S_CALL_FAILED_DNE),     // 1727L

    DIRTYSOCK_ErrorName(OR_INVALID_OXID),           // 1910L -- the object exporter specified was not found

    // Misc other errors
    DIRTYSOCK_ErrorName(ERROR_INVALID_STATE),       // 5023L - The group or resource is not in the correct state to perform the requested operation

    /*
        WinSock2.h
    */
    // WinSock error codes from 10000-11999
    DIRTYSOCK_ErrorName(WSAEINTR),              // 10004L - A blocking operation was interrupted by a call to WSACancelBlockingCall.
    DIRTYSOCK_ErrorName(WSAEBADF),              // 10009L - The file handle supplied is not valid.
    DIRTYSOCK_ErrorName(WSAEACCES),             // 10013L - An attempt was made to access a socket in a way forbidden by its access permissions.
    DIRTYSOCK_ErrorName(WSAEFAULT),             // 10014L - The system detected an invalid pointer address in attempting to use a pointer argument in a call.
    DIRTYSOCK_ErrorName(WSAEINVAL),             // 10022L - An invalid argument was supplied.
    DIRTYSOCK_ErrorName(WSAEMFILE),             // 10024L - Too many open sockets.
    DIRTYSOCK_ErrorName(WSAEWOULDBLOCK),        // 10035L - A non-blocking socket operation could not be completed immediately.
    DIRTYSOCK_ErrorName(WSAEINPROGRESS),        // 10036L - A blocking operation is currently executing.
    DIRTYSOCK_ErrorName(WSAEALREADY),           // 10037L - An operation was attempted on a non-blocking socket that already had an operation in progress.
    DIRTYSOCK_ErrorName(WSAENOTSOCK),           // 10038L - An operation was attempted on something that is not a socket.
    DIRTYSOCK_ErrorName(WSAEDESTADDRREQ),       // 10039L - A required address was omitted from an operation on a socket.
    DIRTYSOCK_ErrorName(WSAEMSGSIZE),           // 10040L - A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram into was smaller than the datagram itself.
    DIRTYSOCK_ErrorName(WSAEPROTOTYPE),         // 10041L - A protocol was specified in the socket function call that does not support the semantics of the socket type requested.
    DIRTYSOCK_ErrorName(WSAENOPROTOOPT),        // 10042L - An unknown, invalid, or unsupported option or level was specified in a getsockopt or setsockopt call.
    DIRTYSOCK_ErrorName(WSAEPROTONOSUPPORT),    // 10043L - The requested protocol has not been configured into the system, or no implementation for it exists.
    DIRTYSOCK_ErrorName(WSAESOCKTNOSUPPORT),    // 10044L - The support for the specified socket type does not exist in this address family.
    DIRTYSOCK_ErrorName(WSAEOPNOTSUPP),         // 10045L - The attempted operation is not supported for the type of object referenced.
    DIRTYSOCK_ErrorName(WSAEPFNOSUPPORT),       // 10046L - The protocol family has not been configured into the system or no implementation for it exists.
    DIRTYSOCK_ErrorName(WSAEAFNOSUPPORT),       // 10047L - An address incompatible with the requested protocol was used.
    DIRTYSOCK_ErrorName(WSAEADDRINUSE),         // 10048L - Only one usage of each socket address (protocol/network address/port) is normally permitted.
    DIRTYSOCK_ErrorName(WSAEADDRNOTAVAIL),      // 10049L - The requested address is not valid in its context.
    DIRTYSOCK_ErrorName(WSAENETDOWN),           // 10050L - A socket operation encountered a dead network.
    DIRTYSOCK_ErrorName(WSAENETUNREACH),        // 10051L - A socket operation was attempted to an unreachable network.
    DIRTYSOCK_ErrorName(WSAENETRESET),          // 10052L - The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress.
    DIRTYSOCK_ErrorName(WSAECONNABORTED),       // 10053L - An established connection was aborted by the software in your host machine.
    DIRTYSOCK_ErrorName(WSAECONNRESET),         // 10054L - An existing connection was forcibly closed by the remote host.
    DIRTYSOCK_ErrorName(WSAENOBUFS),            // 10055L - An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.
    DIRTYSOCK_ErrorName(WSAEISCONN),            // 10056L - A connect request was made on an already connected socket.
    DIRTYSOCK_ErrorName(WSAENOTCONN),           // 10057L - A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using a sendto call) no address was supplied.
    DIRTYSOCK_ErrorName(WSAESHUTDOWN),          // 10058L - A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call.
    DIRTYSOCK_ErrorName(WSAETOOMANYREFS),       // 10059L - Too many references to some kernel object.
    DIRTYSOCK_ErrorName(WSAETIMEDOUT),          // 10060L - A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond.
    DIRTYSOCK_ErrorName(WSAECONNREFUSED),       // 10061L - No connection could be made because the target machine actively refused it.
    DIRTYSOCK_ErrorName(WSAELOOP),              // 10062L - Cannot translate name.
    DIRTYSOCK_ErrorName(WSAENAMETOOLONG),       // 10063L - Name component or name was too long.
    DIRTYSOCK_ErrorName(WSAEHOSTDOWN),          // 10064L - A socket operation failed because the destination host was down.
    DIRTYSOCK_ErrorName(WSAEHOSTUNREACH),       // 10065L - A socket operation was attempted to an unreachable host.
    DIRTYSOCK_ErrorName(WSAENOTEMPTY),          // 10066L - Cannot remove a directory that is not empty.
    DIRTYSOCK_ErrorName(WSAEPROCLIM),           // 10067L - A Windows Sockets implementation may have a limit on the number of applications that may use it simultaneously.
    DIRTYSOCK_ErrorName(WSAEUSERS),             // 10068L - Ran out of quota.
    DIRTYSOCK_ErrorName(WSAEDQUOT),             // 10069L - Ran out of disk quota.
    DIRTYSOCK_ErrorName(WSAESTALE),             // 10070L - File handle reference is no longer available.
    DIRTYSOCK_ErrorName(WSAEREMOTE),            // 10071L - Item is not available locally.
    DIRTYSOCK_ErrorName(WSASYSNOTREADY),        // 10091L - WSAStartup cannot function at this time because the underlying system it uses to provide network services is currently unavailable.
    DIRTYSOCK_ErrorName(WSAVERNOTSUPPORTED),    // 10092L - The Windows Sockets version requested is not supported.
    DIRTYSOCK_ErrorName(WSANOTINITIALISED),     // 10093L - Either the application has not called WSAStartup, or WSAStartup failed.
    DIRTYSOCK_ErrorName(WSAEDISCON),            // 10101L - Returned by WSARecv or WSARecvFrom to indicate the remote party has initiated a graceful shutdown sequence.
    DIRTYSOCK_ErrorName(WSAENOMORE),            // 10102L - No more results can be returned by WSALookupServiceNext.
    DIRTYSOCK_ErrorName(WSAECANCELLED),         // 10103L - A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.
    DIRTYSOCK_ErrorName(WSAEINVALIDPROCTABLE),  // 10104L - The procedure call table is invalid.
    DIRTYSOCK_ErrorName(WSAEINVALIDPROVIDER),   // 10105L - The requested service provider is invalid.
    DIRTYSOCK_ErrorName(WSAEPROVIDERFAILEDINIT),// 10106L - The requested service provider could not be loaded or initialized.
    DIRTYSOCK_ErrorName(WSASYSCALLFAILURE),     // 10107L - A system call that should never fail has failed.
    DIRTYSOCK_ErrorName(WSASERVICE_NOT_FOUND),  // 10108L - No such service is known. The service cannot be found in the specified name space.
    DIRTYSOCK_ErrorName(WSATYPE_NOT_FOUND),     // 10109L - The specified class was not found.
    DIRTYSOCK_ErrorName(WSA_E_NO_MORE),         // 10110L - No more results can be returned by WSALookupServiceNext.
    DIRTYSOCK_ErrorName(WSA_E_CANCELLED),       // 10111L - A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.
    DIRTYSOCK_ErrorName(WSAEREFUSED),           // 10112L - A database query failed because it was actively refused.
    DIRTYSOCK_ErrorName(WSAHOST_NOT_FOUND),     // 11001L - No such host is known.

    // NULL terminate
    DIRTYSOCK_ListEnd()
};

#define DIRTYERR_NUMERRORS (sizeof(_DirtyErr_List) / sizeof(_DirtyErr_List[0]))

#endif // #if DIRTYSOCK_ERRORNAMES

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function DirtyErrNameList

    \Description
        This function takes as input a system-specific error code, and either
        resolves it to its define name if it is recognized, or formats it as a hex
        number if not.

    \Input *pBuffer - [out] pointer to output buffer to store result
    \Input iBufSize - size of output buffer
    \Input uError   - error code to format
    \Input *pList   - error list to use

    \Version 06/13/2005 (jbrookes)
*/
/********************************************************************************F*/
void DirtyErrNameList(char *pBuffer, int32_t iBufSize, uint32_t uError, const DirtyErrT *pList)
{
    static char strUnknown[16];

    #if DIRTYSOCK_ERRORNAMES
    int32_t iErr;

    // first try to match exactly
    for (iErr = 0; pList[iErr].uError != DIRTYSOCK_LISTTERM; iErr++)
    {
        if ((pList[iErr].uError & ~0x80000000) == (uError & ~0x80000000))
        {
            ds_snzprintf(pBuffer, iBufSize, "%s/0x%08x", pList[iErr].pErrorName, uError);
            return;
        }
    }

    // if not found, try to match lower 16 bits of error code
    for (iErr = 0; pList[iErr].uError != DIRTYSOCK_LISTTERM; iErr++)
    {
        if ((pList[iErr].uError & ~0xffff0000) == (uError & ~0xffff0000))
        {
            ds_snzprintf(pBuffer, iBufSize, "%s/0x%08x", pList[iErr].pErrorName, uError);
            return;
        }
    }
    #endif

    ds_snzprintf(pBuffer, iBufSize, "0x%08x", uError);
}

/*F********************************************************************************/
/*!
    \Function DirtyErrName

    \Description
        This function takes as input a system-specific error code, and either
        resolves it to its define name if it is recognized or formats it as a hex
        number if not.

    \Input *pBuffer - [out] pointer to output buffer to store result
    \Input iBufSize - size of output buffer
    \Input uError   - error code to format

    \Version 06/13/2005 (jbrookes)
*/
/********************************************************************************F*/
void DirtyErrName(char *pBuffer, int32_t iBufSize, uint32_t uError)
{
    #if DIRTYSOCK_ERRORNAMES
    DirtyErrNameList(pBuffer, iBufSize, uError, _DirtyErr_List);
    #endif
}

/*F********************************************************************************/
/*!
    \Function DirtyErrGetNameList

    \Description
        This function takes as input a system-specific error code, and either
        resolves it to its define name if it is recognized or formats it as a hex
        number if not.

    \Input uError   - error code to format
    \Input *pList   - error list to use

    \Output
        const char *- pointer to error name or error formatted in hex

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
const char *DirtyErrGetNameList(uint32_t uError, const DirtyErrT *pList)
{
    static char strName[8][64];
    static uint8_t uLast = 0;
    char *pName = strName[uLast++];
    if (uLast > 7) uLast = 0;
    DirtyErrNameList(pName, sizeof(strName[0]), uError, pList);
    return(pName);
}

/*F********************************************************************************/
/*!
    \Function DirtyErrGetName

    \Description
        This function takes as input a system-specific error code, and either
        resolves it to its define name if it is recognized or formats it as a hex
        number if not.

    \Input uError   - error code to format

    \Output
        const char *- pointer to error name or error formatted in hex

    \Version 06/13/2005 (jbrookes)
*/
/********************************************************************************F*/
const char *DirtyErrGetName(uint32_t uError)
{
    static char strName[128];
    DirtyErrName(strName, sizeof(strName), uError);
    return(strName);
}
