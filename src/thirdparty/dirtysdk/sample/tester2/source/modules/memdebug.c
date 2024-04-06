/*H********************************************************************************/
/*!
    \File memdebug.c

    \Description
        Enable/Disable DirtySock memory debugging.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 11/01/2005 (jbookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "libsample/zlib.h"

#include "testermemory.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdMemDebug

    \Description
        Enable/Disable DirtySock memory debugging.

    \Input  *argz   - environment
    \Input   argc   - number of args
    \Input **argv   - argument list
    
    \Output int32_t     - standard return code

    \Version 11/01/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t CmdMemDebug(ZContext *argz, int32_t argc, char **argv)
{
    uint32_t bEnable;

    // check usage
    if (argc != 2)
    {
        ZPrintf("   control memory auditing features\n");
        ZPrintf("   usage: %s [0|1] - disable/enable memory debugging\n", argv[0]);
        return(0);
    }

    // parse the arg
    bEnable = (int32_t)strtol(argv[1], NULL, 10);
    TesterMemorySetDebug(bEnable);
    ZPrintf("   %s: memory debugging %s\n", argv[0], bEnable ? "enabled" : "disabled");

    // done
    return(0);
}
