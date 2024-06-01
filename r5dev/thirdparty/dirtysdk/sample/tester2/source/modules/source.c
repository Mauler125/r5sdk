/*H********************************************************************************/
/*!
    \File source.c

    \Description
        A tester command to implement 'source' (like unix) command

    \Copyright
        Copyright (c) 2012 Electronic Arts Inc.

    \Version 10/10/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <stdlib.h>

#include "DirtySDK/platform.h"

#include "libsample/zlib.h"
#include "libsample/zmem.h"
#include "libsample/zfile.h"

#include "testerregistry.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct SourceRefT
{
    TesterModulesT *pModulesState;
    char *pScriptData;
    char *pScriptLine;
    int32_t iFileSize;
    int32_t iCurrProc;
    int32_t iCurrRslt;
    int32_t iCurrStat;
    int32_t iNumCmds;
    int32_t iNumCmdsFailed;
} SourceRefT;

/*** Function Prototypes ***************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _SourceExecuteCmd

    \Description
        'source' callback, called after command has been issued.

    \Input *pRef        - command module state
    \Input *pScriptLine - current line of source script
    
    \Output
        char *          - pointer to next line of script, or NULL

    \Version 10/10/2012 (jbrookes)
*/
/********************************************************************************F*/
static char *_SourceExecuteCmd(SourceRefT *pRef, char *pScriptLine)
{
    char *pScriptEnd;

    // find end of command
    for (pScriptEnd = pScriptLine; (*pScriptEnd != '\r') && (*pScriptEnd != '\n') && (*pScriptEnd != '\0'); pScriptEnd += 1)
        ;

    // terminate and skip to next command start
    if (*pScriptEnd != '\0')
    {
        for (*pScriptEnd++ = '\0';  ((*pScriptEnd == '\r') || (*pScriptEnd == '\n')) && (*pScriptEnd != '\0'); pScriptEnd += 1)
            ;
    }

    // execute the line
    ZPrintf("source: executing '%s'\n", pScriptLine);
    pRef->iCurrStat = TesterModulesDispatch(pRef->pModulesState, pScriptLine);
    pRef->iNumCmds += 1;
    // get process id of process we just executed
    pRef->iCurrProc = ZGetPid();

    /* restore current environment... required because TexterModulesDispatch()/ZInvoke() leaves the dispatched
       command as the current environment, which messes up our subsequent call to ZCallback() */
    ZSet((ZContext *)pRef);

    // return pointer to next command, or NULL if no more commands
    return((*pScriptEnd != '\0') ? pScriptEnd : NULL);
}

/*F********************************************************************************/
/*!
    \Function _CmdSourceCb

    \Description
        'source' callback, called after command has been issued.

    \Input *argz    - pointer to context
    \Input argc     - number of command-line arguments
    \Input *argv[]  - command-line argument list
    
    \Output
        int32_t     - result of zcallback, or zero to terminate

    \Version 10/10/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdSourceCb(ZContext *argz, int32_t argc, char *argv[])
{
    SourceRefT *pRef = (SourceRefT *)argz;\
    int32_t iStatus;

    // check for kill
    if (argc == 0)
    {
        ZPrintf("%s: killed\n", argv[0]);
        return(0);
    }

    // if we have a current process running, check it
    if (pRef->iCurrProc != -1)
    {
        /* $$ TODO - this works to determine the command is no longer running, but getting the final status/return code
        doesn't work; it is not available since the command is no more */
        if ((iStatus = ZGetStatusPid(pRef->iCurrProc)) != ZLIB_STATUS_RUNNING)
        {
            // determine process result -- first check for immediate-exit result
            if (pRef->iCurrStat != ZLIB_STATUS_RUNNING)
            {
                iStatus = pRef->iCurrStat;
            }
            else if (iStatus == ZLIB_STATUS_UNKNOWN)
            {
                iStatus = 0; // any 'async' command will do this until we can implement a way to get the exit code
            }
            ZPrintf("%s: process %d complete (result=%d)\n", argv[0], pRef->iCurrProc, iStatus);
            // track completion status
            pRef->iNumCmdsFailed += iStatus != 0;
            // clear current command
            pRef->iCurrProc = -1;
        }
    }

    // issue a new command?
    if (pRef->iCurrProc == -1)
    {
        // if we have more commands, start a new one
        if (pRef->pScriptLine != NULL)
        {
            pRef->pScriptLine = _SourceExecuteCmd(pRef, pRef->pScriptLine);
        }
        else
        {
            // we're done, time to quit
            ZMemFree(pRef->pScriptData);
            ZPrintf("%s: done (%d commands, %d failed)\n", argv[0], pRef->iNumCmds, pRef->iNumCmdsFailed);
            return(0);
        }
    }

    // keep recurring
    return(ZCallback(&_CmdSourceCb, 16));
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdSource

    \Description
        Upnp command.  This command starts the ProtoUpnp module.

    \Input *argz    - pointer to context
    \Input argc     - number of command-line arguments
    \Input *argv[]  - command-line argument list
    
    \Output
        int32_t     - result of zcallback

    \Version 03/23/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdSource(ZContext *argz, int32_t argc, char *argv[])
{
    SourceRefT *pRef;

    // handle basic help
    if (argc != 2)
    {
        ZPrintf("usage: %s <file>\n", argv[0]);
        return(0);
    }

    // otherwise run the script
    ZPrintf("%s: running script '%s'\n", argv[0], argv[1]);

    // allocate context
    if ((pRef = (SourceRefT *)ZContextCreate(sizeof(*pRef))) == NULL)
    {
        ZPrintf("%s: could not allocate state\n", argv[0]);
        return(-1);
    }
    ds_memclr(pRef, sizeof(*pRef));

    // get modules state
    if ((pRef->pModulesState = (TesterModulesT *)TesterRegistryGetPointer("MODULES")) == NULL)
    {
        ZPrintf("%s: could not get module state for dispatch\n", argv[0]);
        return(-2);
    }

    // load the script
    if ((pRef->pScriptData = ZFileLoad(argv[1], &pRef->iFileSize, 0)) == NULL)
    {
        ZPrintf("%s: failed to load file {%s}\n", argv[0], argv[1]);
        return(-3);
    }
    pRef->pScriptLine = pRef->pScriptData;
    pRef->iCurrProc = -1;

    // set up recurring callback
    return(ZCallback(_CmdSourceCb, 16));
}

