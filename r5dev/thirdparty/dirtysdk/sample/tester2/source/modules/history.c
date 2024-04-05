/*H********************************************************************************/
/*!
    \File history.c

    \Description
        Handles the history command for Tester2

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 04/11/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"

#include "libsample/zmem.h"
#include "libsample/zlib.h"

#include "testercomm.h"
#include "testerregistry.h"
#include "testermodules.h"
#include "testerhistory.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdHistory

    \Description
        Handle the history command

    \Input  *argz   - environment
    \Input   argc   - number of args
    \Input **argv   - argument list
    
    \Output int32_t     - standard return code

    \Version 04/11/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t CmdHistory(ZContext *argz, int32_t argc, char **argv)
{
    int32_t iNum, iHeadCount, iTailCount;
    TesterModulesT *pModules;
    TesterHistoryT *pHistory;
    TesterCommT *pComm;
    const char *pText;

    // get the history module (if we can)
    if ((pHistory = (TesterHistoryT *)TesterRegistryGetPointer("HISTORY")) == NULL)
    {
        ZPrintf("history: could not get information about history - no history module accessible.\n");
        return(-1);
    }

    // history help?
    if (argc < 1)
    {
        ZPrintf("   see previously executed command lines\n");
        ZPrintf("   usage: %s <execcommand>", argv[0]);
        return(0);
    }

    // otherwise, get some history info
    TesterHistoryHeadTailCount(pHistory, &iHeadCount, &iTailCount);

    // see if we just want to dump the history
    if (argc == 1)
    {
        for (iNum = iHeadCount; iNum <= iTailCount; iNum++)
        {
            if ((pText = TesterHistoryGet(pHistory, iNum, NULL, 0)) != NULL)
            {
                ZPrintf("history: - [%5d] {%s}\n",iNum, pText);
            }
        }
    }

    // see if we want to execute a command
    if (argc == 2)
    {
        iNum = atoi(argv[1]);
        pText = TesterHistoryGet(pHistory, iNum, NULL, 0);
        // don't recursively execute the history command itself
        if ((pText != NULL) && (iNum < iTailCount))
        {
            // execute it locally
            if ((pModules = TesterRegistryGetPointer("MODULES")) == NULL)
            {
                ZPrintf("history: could not dispatch historical command locally {%s}\n", pText);
            }
            else
            {
                TesterModulesDispatch(pModules, pText);
            }

            // send it to the other side for execution
            if ((pComm = (TesterCommT *)TesterRegistryGetPointer("COMM")) == NULL)
            {
                ZPrintf("history: could not dispatch historical command remotely {%s}\n", pText);
            }
            else
            {
                TesterCommMessage(pComm, TESTER_MSGTYPE_COMMAND, pText);
            }
        }
        else
        {
            ZPrintf("history: - [%5d] error getting historical command number [%s].\n", iNum, argv[1]);
        }
    }

    return(0);
}
