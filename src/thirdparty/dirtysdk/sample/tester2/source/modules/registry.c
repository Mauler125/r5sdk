/*H********************************************************************************/
/*!
    \File registry.c

    \Description
        Handles registry for tester2.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 04/11/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/platform.h"

#include "libsample/zlib.h"

#include "testerregistry.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdRegistry

    \Description
        Do some registry operations

    \Input  *argz   - environment
    \Input   argc   - number of args
    \Input *argv[]   - argument list
    
    \Output int32_t     - standard return code

    \Version 04/11/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t CmdRegistry(ZContext *argz, int32_t argc, char *argv[])
{
    if(argc < 1)
    {
        ZPrintf("   print registry\n");
        ZPrintf("   usage: %s\n", argv[0]);
    }
    else
    {
        ZPrintf("registry: printing registry:\n");
        TesterRegistryPrint();
        ZPrintf("registry: done\n");
    }
    return(0);
}
