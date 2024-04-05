/*H*************************************************************************************************/
/*!

    \File    protoname.h

    \Description
        This module provides name lookup services via DNS. It is platform indepedent
        and can be used to resolve names for use with other protocol modules. At this
        point, name support is being removed from other protocol modules so it can be
        centralized here.

    \Notes
        None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002.  ALL RIGHTS RESERVED.

    \Version    1.0        03/19/02 (GWS) First Version

*/
/*************************************************************************************************H*/

#ifndef _protoname_h
#define _protoname_h

/*!
\Moduledef ProtoName ProtoName
\Modulemember Proto
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtynet.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// async lookup of a domain name
DIRTYCODE_API HostentT *ProtoNameAsync(const char *pName, int32_t iTimeout);

// synchronous (blocking) lookup of a domain name.
DIRTYCODE_API uint32_t ProtoNameSync(const char *pName, int32_t iTimeout);

#ifdef __cplusplus
}
#endif

//@}

#endif // _protoname_h
