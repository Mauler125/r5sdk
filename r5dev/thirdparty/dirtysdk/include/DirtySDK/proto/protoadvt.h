/*H*************************************************************************************/
/*!

    \File    protoadvt.h

    \Description
        This advertising module provides a relatively simple multi-protocol
        distributed name server architecture utilizing the broadcast capabilities
        of UDP and IPX. Once the module is instantiated, it can be used as both
        an advertiser (server) and watcher (client) simultaneously.

    \Notes
        None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002.  ALL RIGHTS RESERVED.

    \Version    1.0        02/17/99 (GWS) Original version
    \Version    1.1        02/25/99 (GWS) Alpha release
    \Version    1.2        08/10/99 (GWS) Final release
    \Version    1.3        12/04/00 (GWS) Revised for Dirtysock

*/
/*************************************************************************************H*/

#ifndef _protoadvt_h
#define _protoadvt_h

/*!
\Moduledef ProtoAdvt ProtoAdvt
\Modulemember Proto
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

//! define the ports used for service broadcasts
#define ADVERT_BROADCAST_PORT_UDP 9999
#define ADVERT_BROADCAST_PORT_IPX 9999

//! define the static packet header identifier
#define ADVERT_PACKET_IDENTIFIER "gEA"

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! module reference -- passed as first arg to all module functions
typedef struct ProtoAdvtRef ProtoAdvtRef;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// construct an advertising agent
DIRTYCODE_API ProtoAdvtRef *ProtoAdvtConstruct(int32_t buffer);

// destruct an advertising agent
DIRTYCODE_API void ProtoAdvtDestroy(ProtoAdvtRef *what);

// query for available services
DIRTYCODE_API int32_t ProtoAdvtQuery(ProtoAdvtRef *what, const char *kind, const char *proto,
                                     char *buffer, int32_t buflen, int32_t local);

// locate a specific advertisement and return advertisers address (UDP only)
DIRTYCODE_API uint32_t ProtoAdvtLocate(ProtoAdvtRef *ref, const char *kind, const char *name,
                             uint32_t *host, uint32_t defval);

// advertise a service as available
DIRTYCODE_API int32_t ProtoAdvtAnnounce(ProtoAdvtRef *what, const char *kind, const char *name,
                                        const char *addr, const char *note, int32_t freq);

// cancel server advertisement
DIRTYCODE_API int32_t ProtoAdvtCancel(ProtoAdvtRef *what, const char *kind, const char *name);

#ifdef __cplusplus
}
#endif

//@}

#endif // _protoadvt_h









