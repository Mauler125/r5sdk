/*H********************************************************************************/
/*!
    \File testermodules.c

    \Description
        Common, platform independent tester modules.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/22/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "libsample/zmem.h"
#include "testercomm.h"
#include "testerregistry.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct TesterModulesCommandT
{
    char strName[TESTERMODULES_COMMANDNAMESIZE_DEFAULT];    //!< name of the command
    ZCommand *pFunc;                                        //!< function to call when dispatched
} TesterModulesCommandT;

struct TesterModulesT
{
    TesterModulesCommandT Commands[TESTERMODULES_NUMCOMMANDS_DEFAULT];  //!< command list
    int32_t iNumCommands;
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function TesterModulesCreate

    \Description
        Create a tester host client communication module.

    \Input None
    
    \Output TesterModulesT * - pointer to allocated module

    \Version 04/01/2005 (jfrank)
*/
/********************************************************************************F*/
TesterModulesT *TesterModulesCreate(void)
{
    TesterModulesT *pState;
    
    pState = ZMemAlloc(sizeof(TesterModulesT));
    ds_memclr(pState, sizeof(TesterModulesT));
    TesterRegistrySetPointer("MODULES", pState);
    
    return(pState);
}

/*F********************************************************************************/
/*!
    \Function TesterModulesRegister

    \Description
        Register a particular command with a function call pointer.

    \Input *pState    - pointer to host client comm module
    \Input *pCommand  - null-terminated string to register the function call with
    \Input *pFunction - function call to associate with the string
    
    \Output int32_t       - 0=success, error code otherwise

    \Version 04/01/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterModulesRegister(TesterModulesT *pState, const char *pCommand, ZCommand *pFunctionPtr)
{
    TesterModulesCommandT *pCmd = NULL;
    uint32_t uCommand;
    int32_t iStrcmp;

    // check for error conditions
    if ((pState == NULL) || (pCommand == NULL) || (pFunctionPtr == NULL))
    {
        return(TESTERMODULES_ERROR_NULLPOINTER);
    }

    // make sure there's room
    if (pState->iNumCommands == TESTERMODULES_NUMCOMMANDS_DEFAULT)
    {
        return(TESTERMODULES_ERROR_COMMANDLISTFULL);
    }

    // find where to insert the command
    for (uCommand = 0; uCommand < TESTERMODULES_NUMCOMMANDS_DEFAULT; uCommand++)
    {
        // ref the command
        pCmd = &pState->Commands[uCommand];

        // empty slot?
        if (pCmd->pFunc == NULL)
        {
            break;
        }

        // compare the commands
        iStrcmp = strcmp(pCommand, pCmd->strName);

        // see if we've already registered this command
        if (iStrcmp == 0)
        {
            if (pCmd->pFunc == pFunctionPtr)
            {
                NetPrintf(("testermodules: warning -- benign redefinition of command %s\n", pCommand));
                return(TESTERMODULES_ERROR_NONE);
            }
            else
            {
                NetPrintf(("testermodules: error -- redefinition of command %s\n", pCommand));
                return(TESTERMODULES_ERROR_REDEFINITON);
            }
        }
        // check to see if we should insert to preserve alphabetical order
        else if (iStrcmp < 0)
        {
            // create a space for the command
            memmove(pCmd+1, pCmd, (pState->iNumCommands-uCommand) * sizeof(*pCmd));
            break;
        }
    }

    // register the command
    ds_strnzcpy(pCmd->strName, pCommand, sizeof(pCmd->strName));
    pCmd->pFunc = pFunctionPtr;
    pState->iNumCommands += 1;

    return(TESTERMODULES_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function TesterModulesGetCommandName

    \Description
        Get a command name in the list at a particular index.

    \Input *pState    - pointer to host client comm module
    \Input  iIndex    - index of command name to retrieve
    \Input *pBuf      - destination string for the command
    \Input  iBufSize  - size of the destination string
    
    \Output int32_t       - 0=success, error code otherwise

    \Version 04/01/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterModulesGetCommandName(TesterModulesT *pState, int32_t iCommandNum, char *pBuf, int32_t iBufSize)
{
    TesterModulesCommandT *pCmd;
    
    // check for error conditions
    if ((pState == NULL) || (pBuf == NULL) || (iBufSize <= 0))
    {
        return(TESTERMODULES_ERROR_NULLPOINTER);
    }

    // check to see if we're even in range
    if ((iCommandNum < 0) || (iCommandNum >= TESTERMODULES_NUMCOMMANDS_DEFAULT))
    {
        return(TESTERMODULES_ERROR_NOSUCHCOMMAND);
    }

    // now get the entry we want, if it's non-null
    pCmd = &(pState->Commands[iCommandNum]);
    if (pCmd->strName[0] != 0)
    {
        ds_strnzcpy(pBuf, pCmd->strName, iBufSize);
    }

    return(TESTERMODULES_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function TesterModulesDispatch

    \Description
        Dispatch a function (if possible) based on the incoming command line.

    \Input *pState       - pointer to host client comm module
    \Input *pCommandLine - standard command line (command arg1 arg2 arg3 ...)
    
    \Output 0=success, error code otherwise

    \Version 04/01/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterModulesDispatch(TesterModulesT *pState, const char *pCommandLine)
{
    char strCommand[256];
    TesterModulesCommandT *pCmd = NULL;
    uint32_t uLoop;
    char *pCommand;
    ZEnviron *pEnv = NULL;

    // helper variables
    char *pListName;
    uint32_t uListNameSize, uCommandNameSize;

    // check for error conditions
    if ((pState == NULL) || (pCommandLine == NULL))
    {
        return(TESTERMODULES_ERROR_NULLPOINTER);
    }

    // find start of strCommand name
    for (pCommand = (char *)pCommandLine; (*pCommand != 0) && (*pCommand <= ' '); pCommand += 1)
        ;

    // copy over strCommand and null-terminate
    ds_strnzcpy(strCommand, pCommand, sizeof(strCommand));

    // terminate command name
    for (pCommand = strCommand; (*pCommand != 0) && (*pCommand > ' '); pCommand += 1)
        ;
    *pCommand = 0;
    uCommandNameSize = (uint32_t)strlen(strCommand);

    // locate corresponding strCommand
    for (uLoop = 0; uLoop < TESTERMODULES_NUMCOMMANDS_DEFAULT; uLoop++)
    {
        // check to see if we match and we want to re-register a function
        // or if we're at the end of the list
        pListName = pState->Commands[uLoop].strName;
        uListNameSize = (uint32_t)strlen(pListName);
        if ((uListNameSize > 0) && (uListNameSize == uCommandNameSize) && (strncmp(strCommand, pListName, uCommandNameSize) == 0))
        {
            pCmd = &(pState->Commands[uLoop]);
            pEnv = ZCreate(NULL, pCommandLine);
            ZInvoke(pEnv, pCmd->pFunc);
            break;
        }
    }

    // show error if no command. or problem with env.
    if (((pCmd == NULL) && (strCommand[0] != 0)) || (pEnv == NULL))
    {
        return(TESTERMODULES_ERROR_NOSUCHCOMMAND);
    }
    else //give the status of the actual command..
    {
        return(ZGetStatus(pEnv));
    }
}

/*F********************************************************************************/
/*!
    \Function TesterModulesHelp

    \Description
        Display help for a command.  Pass NULL to get help for all commands.

    \Input *pState - pointer to host client comm module
    \Input *pCommandLine - standard command line (command arg1 arg2 arg3 ...)
    
    \Output None

    \Version 04/01/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterModulesHelp(TesterModulesT *pState, const char *pCommand)
{
    int32_t iLoop, iFoundHelp=0;

    // check error conditions
    if(pState == NULL)
    {
        return;
    }

    // if NULL, list all commands (do not show help for them!)
    if(pCommand == NULL)
    {
        char strLine[128] = "";

        ZPrintf("tester2 modules\n");

        // create list of modules
        for(iLoop = 0; iLoop < TESTERMODULES_NUMCOMMANDS_DEFAULT; iLoop++)
        {
            if(pState->Commands[iLoop].strName[0] != '\0')
            {
                ds_strnzcat(strLine, pState->Commands[iLoop].strName, sizeof(strLine));
                if (strlen(strLine) > 79)
                {
                    ZPrintf("%s\n", strLine);
                    strLine[0] = '\0';
                }
                else
                {
                    ds_strnzcat(strLine, " ", sizeof(strLine));
                }
            }
        }

        // print last entry?
        if (strLine[0] != '\0')
        {
            ZPrintf("%s\n", strLine);
        }
    }
    // otherwise get help on a specific command
    else
    {
        for(iLoop = 0; iLoop < TESTERMODULES_NUMCOMMANDS_DEFAULT; iLoop++)
        {
            if(strncmp(pCommand, pState->Commands[iLoop].strName, strlen(pCommand)) == 0)
            {
                char *pBuf[1];
                pBuf[0] = (pState->Commands[iLoop]).strName;
                pState->Commands[iLoop].pFunc(NULL, 0, pBuf);
                iFoundHelp = 1;
                break;
            }
        }

        if(iFoundHelp == 0)
        {
            ZPrintf("ERROR: Could not find help for {%s}\n",pCommand);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function TesterModulesDestroy

    \Description
        Destroy a tester host client communication module.

    \Input *pState - pointer to host client comm module
    
    \Output None

    \Version 04/01/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterModulesDestroy(TesterModulesT *pState)
{
    if(pState)
    {
        ZMemFree(pState);
    }
    TesterRegistrySetPointer("MODULES", NULL);
}


