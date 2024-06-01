/*H********************************************************************************/
/*!
    \File help.c

    \Description
        Handles help for tester2.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 04/11/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"

#include "libsample/zlib.h"

#include "testerregistry.h"
#include "testermodules.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdHelp

    \Description
        Do some registry operations

    \Input  *argz   - environment
    \Input   argc   - number of args
    \Input **argv   - argument list
    
    \Output int32_t     - standard return code

    \Version 04/11/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t CmdHelp(ZContext *argz, int32_t argc, char **argv)
{
    TesterModulesT *pModules;

    // get the modules pointer from the registry, if available
    pModules = (TesterModulesT *)TesterRegistryGetPointer("MODULES");

    // as part of the help function, the help command is called to get help on help.
    // stop the recusion by not calling the TesterModulesHelp function for help(NULL)
    if (pModules == NULL)
    {
        // no modules created
        ZPrintf("HELP: No module help available\n");
    }    
    else if (argc < 1)
    {
        // get help on help
        ZPrintf("   get help on modules\n");
        ZPrintf("   usage: %s <command>\n", argv[0]);
    }
    else if (argc == 2)
    {
        // get help on a specific command
        TesterModulesHelp(pModules, argv[1]);
    }
    else
    {
        // get help on all commands by default
        TesterModulesHelp(pModules, NULL);
    }
    return(0);
}
