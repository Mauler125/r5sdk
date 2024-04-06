/*H*************************************************************************************/
/*!
    \File    commsrp.h

    \Description
        This is CommSRP (Selectively Reliable Protocol), a datagram packet-based
        transport class.

    \Copyright
        Copyright Electronic Arts 1999-2003

    \Version 0.5 01/03/03 (jbrookes) Initial Version, based on CommTCP
    \Version 0.7 01/07/03 (jbrookes) Working unreliable transport, based on CommUDP
    \Version 0.8 01/08/03 (jbrookes) Working reliable transport.
    \Version 0.9 02/09/03 (jbrookes) Added support for sending zero-byte packets, and fixed PS2 alignment issue.
*/
/*************************************************************************************H*/

#ifndef _commsrp_h
#define _commsrp_h

/*!
\Moduledef CommSRP CommSRP
\Modulemember Comm
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

// basic reference returned/used by all routines
typedef struct CommSRPRef CommSRPRef;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// construct the class
DIRTYCODE_API  CommSRPRef *CommSRPConstruct(int32_t maxwid, int32_t maxinp, int32_t maxout);

// destruct the class
DIRTYCODE_API  void CommSRPDestroy(CommSRPRef *what);

// resolve an address
DIRTYCODE_API int32_t CommSRPResolve(CommSRPRef *what, const char *addr, char *buf, int32_t len, char div);

// stop the resolver
DIRTYCODE_API void CommSRPUnresolve(CommSRPRef *what);

// listen for a connection
DIRTYCODE_API int32_t CommSRPListen(CommSRPRef *what, const char *addr);

// stop listening
DIRTYCODE_API int32_t CommSRPUnlisten(CommSRPRef *what);

// initiate a connection to a peer
DIRTYCODE_API int32_t CommSRPConnect(CommSRPRef *what, const char *addr);

// terminate a connection
DIRTYCODE_API int32_t CommSRPUnconnect(CommSRPRef *what);

// set event callback hook
DIRTYCODE_API void CommSRPCallback(CommSRPRef *what, void (*callback)(CommRef *ref, int32_t event));

// return current stream status
DIRTYCODE_API int32_t CommSRPStatus(CommSRPRef *what);

// return current clock tick
DIRTYCODE_API uint32_t CommSRPTick(CommSRPRef *what);

// send a packet
DIRTYCODE_API int32_t CommSRPSend(CommSRPRef *what, const void *buffer, int32_t length, uint32_t flags);

// peek at waiting packet
DIRTYCODE_API int32_t CommSRPPeek(CommSRPRef *what, void *target, int32_t length, uint32_t *when);

// receive a packet from the buffer
DIRTYCODE_API int32_t CommSRPRecv(CommSRPRef *what, void *target, int32_t length, uint32_t *when);

#ifdef __cplusplus
}
#endif

//@}

#endif // _commsrp_h






