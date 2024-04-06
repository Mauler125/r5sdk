/*H********************************************************************************/
/*!
    \File <filename>.c

    \Description
        Helper functions for modules to implement sub-commands.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 05/10/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "libsample/zlib.h"
#include "testersubcmd.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function T2SubCmdUsage

    \Description
        Display basic usage for the given subcommand.

    \Input *pCmdName    - name of command
    \Input *pCmdList    - list of subcommands
    
    \Output
        None.

    \Version 05/10/2005 (jbrookes)
*/
/********************************************************************************F*/
void T2SubCmdUsage(const char *pCmdName, T2SubCmdT *pCmdList)
{
    char strUsage[1024] = "help|";
    int32_t iCmd;

    for (iCmd = 0; pCmdList[iCmd].pFunc != NULL; iCmd++)
    {
        strcat(strUsage, pCmdList[iCmd].strName);
        if (pCmdList[iCmd+1].pFunc != NULL)
        {
            strcat(strUsage, "|");
        }
    }
    ZPrintf("   usage: %s [%s]\n", pCmdName, strUsage);
}

/*F********************************************************************************/
/*!
    \Function T2SubCmdParse

    \Description
        Parse commandline for subcommand, and return matching command or NULL
        if not found.

    \Input *pCmdList    - pointer to command list to match against
    \Input argc         - argument count
    \Input *argv[]      - argument list
    \Input *pHelp       - [out] storage for whether help is requested or not
    
    \Output
        T2SubCmdT *     - pointer to parsed subcommand, or NULL if no match

    \Version 05/10/2005 (jbrookes)
*/
/********************************************************************************F*/
T2SubCmdT *T2SubCmdParse(T2SubCmdT *pCmdList, int32_t argc, char *argv[], unsigned char *pHelp)
{
    T2SubCmdT *pCmd = NULL;
    int32_t iArgIdx;

    if (argc > 1)
    {
        iArgIdx = (!strcmp(argv[1], "help")) ? 2 : 1;

        if (iArgIdx < argc)
        {
            *pHelp = (iArgIdx == 2) ? 1 : 0;

            for (pCmd = pCmdList; pCmd->pFunc != NULL; pCmd++)
            {
                if (!strcmp(argv[iArgIdx], pCmd->strName))
                {
                    break;
                }
            }

            if (pCmd->pFunc == NULL)
            {
                pCmd = NULL;
            }
        }
    }

    return(pCmd);
}

