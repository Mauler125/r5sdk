/*H********************************************************************************/
/*!

    \File   dirtydefs.h

    \Description:
        DirtySock platform independent definitions and enumerations.

        [DEPRECATE]

    \Copyright
        Copyright (c) Electronic Arts 1999-2005

    \Version    1.0        02/03/99 (JLB) First Version
    \Version    1.1        04/01/99 (MDB) Added Endian types
    \Version    1.2        03/27/02 (GWS) Made NULL C++ friendly, added DIRTYCODE_IOP, CODE_UNIX
    \Version    1.3        02/22/05 (JEF) Moved CODE_XXX to DIRTYCODE_XXX to avoid conflicts
    \Version    1.4        03/22/05 (GWS) Replaced with include of platform.h
*/
/********************************************************************************H*/

#ifndef _dirtydefs_h
#define _dirtydefs_h

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

// Microsoft Facilty codes when building a HRESTULT go up to 81, so we'll just start at 128
// we have 11 bits to work with giving us a max value of 2047
#define DIRTYAPI_SOCKET            (128)
#define DIRTYAPI_PROTO_HTTP        (129)
#define DIRTYAPI_PROTO_SSL         (130)
#define DIRTYAPI_QOS               (131)
#define DIRTYAPI_MAX               (2047)

#define DIRTYAPI_SOCKET_ERR_ALREADY_ACTIVE         (-1)
#define DIRTYAPI_SOCKET_ERR_NO_MEMORY              (-2)
#define DIRTYAPI_SOCKET_ERR_HOST_NAME_CACHE        (-3)
#define DIRTYAPI_SOCKET_ERR_PLATFORM_SPECIFIC      (-4)
/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

#endif // _dirtydefs_h


