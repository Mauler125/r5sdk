/*H*************************************************************************************************/
/*!

    \File    commudp.h

    \Description
        This is a simple UDP transport class which incorporations the notions of virtual
        connections and error free transfer. The protocol is optimized for use in a real-time
        environment where latency is more important than bandwidth. Overhead is low with
        the protocol adding only 8 bytes per packet on top of that required by UDP itself.

    \Notes
        None.

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
    \Version    2.4        07/07/00 (GWS) Added firewall penetrator
    \Version    3.0        12/04/00 (GWS) Reported to dirtysock
    \Version    3.1        11/20/02 (JLB) Added Send() flags parameter
    \Version    3.2        02/18/03 (JLB) Fixes for multiple connection support
    \Version    3.3        05/06/03 (GWS) Allowed poke to come from any IP
    \Version    3.4        09/02/03 (JLB) Added unreliable packet type
    \Version    4.0        09/12/03 (JLB) Per-send optional unreliable transport

*/
/*************************************************************************************************H*/


#ifndef _commudp_h
#define _commudp_h

/*!
\Moduledef CommUDP CommUDP
\Modulemember Comm
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

// basic reference returned/used by all routines
typedef struct CommUDPRef CommUDPRef;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// construct the class
DIRTYCODE_API CommUDPRef *CommUDPConstruct(int32_t iMaxWid, int32_t iMaxInp, int32_t iMaxOut);

// destruct the class
DIRTYCODE_API void CommUDPDestroy(CommUDPRef *pRef);

// resolve an address
DIRTYCODE_API int32_t CommUDPResolve(CommUDPRef *pRef, const char *pAddr, char *pBuf, int32_t iLen, char cDiv);

// resolve an address
DIRTYCODE_API void CommUDPUnresolve(CommUDPRef *pRef);

// listen for a connection
DIRTYCODE_API int32_t CommUDPListen(CommUDPRef *pRef, const char *pAddr);

// stop listening
DIRTYCODE_API int32_t CommUDPUnlisten(CommUDPRef *pRef);

// initiate a connection to a peer
DIRTYCODE_API int32_t CommUDPConnect(CommUDPRef *pRef, const char *pAddr);

// terminate a connection
DIRTYCODE_API int32_t CommUDPUnconnect(CommUDPRef *pRef);

// set event callback hook
DIRTYCODE_API void CommUDPCallback(CommUDPRef *pRef, void (*pCallback)(void *pRef, int32_t iEvent));

// return current stream status
DIRTYCODE_API int32_t CommUDPStatus(CommUDPRef *pRef);

// control connection behavior
DIRTYCODE_API int32_t CommUDPControl(CommUDPRef *pRef, int32_t iControl, int32_t iValue, void *pValue);

// return current clock tick
DIRTYCODE_API uint32_t CommUDPTick(CommUDPRef *pRef);

// send a packet
DIRTYCODE_API int32_t CommUDPSend(CommUDPRef *pRef, const void *pBuffer, int32_t iLength, uint32_t uFlags);

// peek at waiting packet
DIRTYCODE_API int32_t CommUDPPeek(CommUDPRef *pRef, void *pTarget, int32_t iLength, uint32_t *pWhen);

// receive a packet from the buffer
DIRTYCODE_API int32_t CommUDPRecv(CommUDPRef *pRef, void *pTarget, int32_t iLength, uint32_t *pWhen);

#ifdef __cplusplus
}
#endif

//@}

#endif // _commudp_h

