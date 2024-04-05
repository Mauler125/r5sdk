/*H*************************************************************************************/
/*!
    \File dirtysock.h

    \Description
        Platform independent interface to network layers. Based on
        BSD sockets, but with performance modifications. Allows truly
        portable modules to be written and moved to different platforms
        needing only different support wrappers (no change to actual
        network modes).

    \Copyright
        Copyright (c) Electronic Arts 2001-2014

    \Version 1.0 08/01/2001 (gschaefer) First Version
*/
/*************************************************************************************H*/

#ifndef _dirtysock_h
#define _dirtysock_h

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

#ifndef DIRTYSOCK
#define DIRTYSOCK (TRUE)
#include "DirtySDK/dirtydefs.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#endif

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#endif // _dirtysock_h

