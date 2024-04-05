/*H*************************************************************************************************/
/*!

    \File    protoname.c

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


/*** Include files *********************************************************************/

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protoname.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

// Public variables


/*** Private Functions *****************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    _ProtoNameSync

    \Description
        Synchronous (blocking) lookup.

    \Input *pName       - pointer to name
    \Input iTimeout     - timeout in millseconds

    \Output
        uint32_t    - 0=failed, else the IP address

    \Version    1.0        03/19/02 (GWS) First Version
*/
/*************************************************************************************************F*/
static uint32_t _ProtoNameSync(const char *pName, int32_t iTimeout)
{
    HostentT *pLookup;
    uint32_t uAddr = 0;

    // start the query
    pLookup = SocketLookup(pName, iTimeout);
    if (pLookup != NULL)
    {
        // wait for completion
        while (!pLookup->Done(pLookup))
        {
            // give away some time
            NetConnSleep(10);
        }

        // get the address
        uAddr = pLookup->addr;
        pLookup->Free(pLookup);
    }

    // return the address
    return(uAddr);
}


/*** Public functions *****************************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    ProtoNameAsync

    \Description
        Async lookup of a domain name. Same as SocketLookup
        but deals with platform specific issues (i.e., this
        works on PS2/EE while calling SocketLookup directly
        would not).

    \Input *pName       - pointer to name
    \Input iTimeout     - timeout in millseconds

    \Output
        HostentT *      - HostentT structure pointer

    \Version    1.0        03/19/02 (GWS) First Version
*/
/*************************************************************************************************F*/
HostentT *ProtoNameAsync(const char *pName, int32_t iTimeout)
{
    return(SocketLookup(pName, iTimeout));
}

/*F*************************************************************************************************/
/*!
    \Function    ProtoNameSync

    \Description
        Synchronous (blocking) lookup of a domain name.

    \Input *pName       - pointer to name
    \Input iTimeout     - timeout in millseconds

    \Output
        uint32_t    - 0=failed, else the IP address

    \Version    1.0        03/19/02 (GWS) First Version
*/
/*************************************************************************************************F*/
uint32_t ProtoNameSync(const char *pName, int32_t iTimeout)
{
    return(_ProtoNameSync(pName, iTimeout));
}
