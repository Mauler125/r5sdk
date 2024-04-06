/*H********************************************************************************/
/*!
    \File runscript.c

    \Description
        Run a script in Tester2.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 04/11/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"

#include "libsample/zmem.h"
#include "libsample/zfile.h"

#include "testerregistry.h"
#include "testermodules.h"
#include "testercomm.h"
#include "testerclientcore.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _dispatchCommand

    \Description
        Dispatch a single command line, locally and remotely.

    \Input  *pCmd   - pointer to this command eg. run.
    \Input  *bClientOnly   - if true, only run on client side.
    \Input  *pLine   - the command line to be run.
    
    \Output  int32_t - return code; 0 if command ran, -1 if empty or some error

    \Version 30/11/2005 (TE)
*/
/********************************************************************************F*/
int32_t _dispatchCommand(char* pCmd, int bClientOnly, char * pLine)
{
    TesterModulesT *pModules;
    TesterCommT *pComm = NULL;
    int32_t iResult = -1;

    if ((strlen(pLine) == 0) || (pLine[0] == '#'))
    {
        return(iResult);
    }
    // execute it locally
    if ((pModules = (TesterModulesT *)TesterRegistryGetPointer("MODULES")) == NULL)
    {
        ZPrintf("%s: could not dispatch script command locally {%s}\n", pCmd, pLine);
    }
    else
    {
        ZPrintf("%s: {%s}\n", pCmd, pLine);
        // check to see if its a local command
        TesterModulesDispatch(pModules, pLine);
        iResult = 0;
    }

    // try host too...
    if (!bClientOnly)
    {
        // send it to the other side for execution
        if ((pComm = TesterRegistryGetPointer("COMM")) == NULL)
        {
            ZPrintf("%s: could not dispatch  command remotely {%s}\n", pCmd, pLine);
        }
        else
        {
            TesterCommMessage(pComm, TESTER_MSGTYPE_COMMAND, pLine);
            iResult = 0;
        }
    }
    return(iResult);
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdRunScript

    \Description
        Run a script

    \Input  *argz   - environment
    \Input   argc   - number of args
    \Input *argv[]  - argument list
    
    \Output int32_t     - standard return code

    \Version 04/11/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t CmdRunScript(ZContext *argz, int32_t argc, char *argv[])
{
    int32_t iFileSize;
    char *pFile = NULL, *pLine = NULL, *pEnd = NULL;
    TesterClientCoreT *pState = NULL;
    char strSleep[100]; //to format a sleep command
    static int iSleepTimeStatic =2; //default sleep is 2 secs between commands

    // check usage
    if (argc < 2)
    {
        ZPrintf( "usage: %s <filename> - run a script\n", argv[0]);
        ZPrintf("usage: %s -s <sleeptime> -set sleep secs between script commands\n", argv[0]);
        return(-1);
    }
    if (strcmp (argv[1],"-s") == 0)
    { //set the sleep time..
        if (argc < 3)
        {
            ZPrintf(  "usage: %s -s <sleeptime> \n", argv[0]);  
            return(-1);
        }
        iSleepTimeStatic = atoi(argv[2]);
        ZPrintf("%s: sleep between commands set to: %d secs \n", argv[0], iSleepTimeStatic);  
        return(0);
    }

    // otherwise run the script
    ZPrintf("%s: running script {%s}\n", argv[0], argv[1]);

    // load the data in
    pFile = ZFileLoad(argv[1], &iFileSize, 0);
    if(pFile == NULL)
    {
        ZPrintf("%s: failed to load file {%s}\n", argv[0], argv[1]);
        return(0);
    }
    pState = (TesterClientCoreT *)TesterRegistryGetPointer("CORE");
    
    // fire off each line
    pLine = pFile;
    //NO strtok() --it is static and i/o uses it too!!
    if ( (pEnd = strstr(pFile, "\n")) != NULL)
    {
        *pEnd++ = '\0';
    }

    while(pLine)
    {
        if ((_dispatchCommand(argv[0], 0, pLine) == 0) && (iSleepTimeStatic > 0))
        { //did something.. so sleep
            //format sleep command and sleep to catch up
            sprintf (strSleep, "sleep %d", iSleepTimeStatic);
            _dispatchCommand( argv[0], 1, strSleep); //clientOnly=1
        }       
        if (pState)
        { // pump i/o between commands..
            TesterClientCoreIdle(pState);
        }

        // try to process the next line
        pLine = pEnd;
        if ( (pLine) &&((pEnd = strstr(pLine, "\n")) != NULL))
        {
            *pEnd++ = '\0';
        }
    }

    ZPrintf("%s:  ran scripts in {%s}\n", argv[0],  argv[1]);
   
    // kill the memory allocated by loadfile
    ZMemFree(pFile);
    return(0);
}
