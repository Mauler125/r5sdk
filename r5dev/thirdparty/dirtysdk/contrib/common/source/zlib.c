/*H********************************************************************************/
/*!
    \File zlib.c

    \Description
        A simple console style test library that provides a basic
        notion of processes, output routines and memory allocation.
        Used to implement simple test programs in a unix-style
        command-line environment.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 09/15/1999 (gschaefer) Initial Design
    \Version 11/08/1999 (gschaefer) Cleanup and revision
    \Version 03/17/2005 (jfrank)    Cleanup and revision
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#if defined(_WIN32) && !defined(_XBOX)
#define WIN32_LEAN_AND_MEAN 1

#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "DirtySDK/platform.h"

#include "DirtySDK/dirtydefs.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "zlib.h"
#include "zmem.h"

/*** Defines **********************************************************************/

#if !defined(_WIN32) || defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
#define wsprintf sprintf
#endif

/*** Type Definitions *************************************************************/

struct ZEnviron
{
    ZEnviron *pNext;            //!< list of active process contexts
    ZContext *pContext;         //!< process private context
    ZConsole *pConsole;         //!< console output pointer
    ZCommand *pCommand;         //!< pointer to process code

    uint32_t uRuntime;          //!< number of ms process has run
    uint32_t uRuncount;         //!< number of time slices

    int32_t iPID;               //!< process identifier
    int32_t iStatus;            //!< exit status
    uint32_t uSchedule;         //!< next scheduled tick to run at

    int32_t iArgc;              //!< arg count for program
    char *pArgv[200];           //!< pointer to entry parms
    char strArgb[16*1024];      //!< buffer to hold entry parms
};

/*** Variables ********************************************************************/

static ZEnviron *_pEnvCurr = NULL;   //!< current process
static ZEnviron *_pEnvList = NULL;   //!< list of processes

static void *_Zlib_pPrintfParm = NULL;
static int32_t (*_Zlib_pPrintfHook)(void *pParm, const char *pText) = NULL;

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ZCreate

    \Description
        Create a new process environment. Parses command line
        and gets process ready to run.

    \Input   console - default output console for process
    \Input   cmdline - the entry command line params

    \Output ZEnviron - new process environment

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
ZEnviron *ZCreate(ZConsole *console, const char *cmdline)
{
    char term;
    char *s;
    ZEnviron *env;
    static uint32_t pid = 1;

    // get new environment
    env = (ZEnviron *)ZMemAlloc(sizeof(*env));
    ds_memclr(env, sizeof(*env));

    // assign unique pid
    env->iPID = pid++;

    // save console ref
    env->pConsole = console;

    // copy command line
    ds_strnzcpy(env->strArgb, cmdline, sizeof(env->strArgb));
    s = env->strArgb;

    // parse command line
    for (env->iArgc = 0; env->iArgc < (signed)(sizeof(env->pArgv)/sizeof(env->pArgv[0])); env->iArgc += 1)
    {
        // skip to next token
        while ((*s != 0) && (*s <= ' '))
            ++s;
        // see if anything to save
        if (*s == 0)
            break;
        // see if there is a terminator
        term = ((*s == '"') || (*s == '\'') ? *s++ : 0);
        // record start of token
        env->pArgv[env->iArgc] = s;
        // find end of token
        while ((*s != 0) && (((term == 0) && (*s > ' ')) || ((term != 0) && (*s != term))))
            ++s;
        // terminate token
        if (*s != 0)
            *s++ = 0;
    }

    // set status to terminated
    env->iStatus = 0;
    env->uSchedule = (uint32_t)(-1);

    // put into process list
    env->pNext = _pEnvList;
    _pEnvList = env;

    // return new environment
    return(env);
}

/*F********************************************************************************/
/*!
    \Function ZDestroy

    \Description
        Destroy a process environment

    \Input env - existing process environment

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
void ZDestroy(ZEnviron *env)
{
    ZEnviron **scan;

    // remove from environment list
    for (scan = &_pEnvList; *scan != NULL; scan = &(*scan)->pNext)
    {
        if (*scan == env)
        {
            *scan = env->pNext;
            break;
        }
    }

    // remove from active
    if (_pEnvCurr == env)
        _pEnvCurr = NULL;

    // destroy any attached context
    if (env->pContext != NULL)
        ZMemFree(env->pContext);

    // destroy the environment
    ZMemFree(env);
    return;
}

/*F********************************************************************************/
/*!
    \Function ZInvoke

    \Description
        Execute a process within an existing environment

    \Input env - existing environment
    \Input cmd - process code pointer

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
void ZInvoke(ZEnviron *env, ZCommand *cmd)
{
    uint32_t tick1, tick2;

    // make sure there is a current environment
    if (env == NULL)
        return;
    _pEnvCurr = env;

    // if this is first run
    if (cmd != NULL)
    {
        // save the command
        _pEnvCurr->pCommand = cmd;
        // remove any existing context
        ZContextCreate(0);
    }
    else if (_pEnvCurr->pCommand == NULL)
    {
        return;
    }

    // invoke the command
    tick1 = ZTick();
    _pEnvCurr->uSchedule = (uint32_t)(-1);
    _pEnvCurr->uRuncount += 1;
    _pEnvCurr->iStatus = _pEnvCurr->pCommand(_pEnvCurr->pContext, _pEnvCurr->iArgc, _pEnvCurr->pArgv);
    tick2 = ZTick();
    _pEnvCurr->uRuntime += tick2-tick1;

    // handle callback request
    if ((_pEnvCurr->uSchedule != (uint32_t)(-1)) && (_pEnvCurr->iStatus == ZLIB_STATUS_RUNNING))
        _pEnvCurr->uSchedule += tick2;
}

/*F********************************************************************************/
/*!
    \Function ZSet

    \Description
        Set current environment, based on specified context

    \Input context  - context; environment that owns this context will be made current

    \Version 10/10/2012 (jbrookes)
*/
/********************************************************************************F*/
void ZSet(ZContext *context)
{
    ZEnviron *env;

    // find context
    for (env = _pEnvCurr; env != NULL && env->pContext != context; env = env->pNext)
        ;

    // found it?  make it current
    if (env != NULL)
    {
        _pEnvCurr = env;
    }
}

/*F********************************************************************************/
/*!
    \Function ZGetPid

    \Description
        Get process id of current environment

    \Output
        int32_t     - pid, or zero if no current environment

    \Version 10/10/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t ZGetPid(void)
{
    int32_t iPID = 0;
    if (_pEnvCurr != NULL)
    {
        iPID = _pEnvCurr->iPID;
    }
    return(iPID);
}

/*F********************************************************************************/
/*!
    \Function ZCallback

    \Description
        Let process signal it wants a callback (has not completed)

    \Input   cmd - pointer to callback code
    \Input delay - milliseconds until callback

    \Output  int32_t - special return value that process returns with

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
int32_t ZCallback(ZCommand *cmd, int32_t delay)
{
    _pEnvCurr->pCommand = cmd;
    _pEnvCurr->uSchedule = delay;
    return(ZLIB_STATUS_RUNNING);
}

/*F********************************************************************************/
/*!
    \Function ZTask

    \Description
        Give time to any existing processes that need it.

    \Output int32_t - number of ticks until the next requested update

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
int32_t ZTask(void)
{
    ZEnviron *env;
    uint32_t tick = ZTick();
    uint32_t next = tick+1000;

    // walk environ list and execute processes which are scheduled
    for (env = _pEnvList; env != NULL; env = env->pNext)
    {
        if (tick > env->uSchedule)
            ZInvoke(env, NULL);
        if (next > env->uSchedule)
            next = env->uSchedule;
    }

    // figure time until next tick
    tick = ZTick();
    return((tick > next) ? 0 : next-tick);
}

/*F********************************************************************************/
/*!
    \Function ZCleanup

    \Description
        Remove any environments containing complete processes.

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
void ZCleanup(void)
{
    ZEnviron *env;

    for (env = _pEnvList; env != NULL;)
    {
        if (env->uSchedule == (uint32_t)(-1))
        {
            ZEnviron *kill = env;
            env = env->pNext;
            ZDestroy(kill);
        }
        else
        {
            env = env->pNext;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function ZShutdown

    \Description
        Kill all active processes in preparation for shutdown.

    \Version 02/18/03 (jbrookes)
*/
/********************************************************************************F*/
void ZShutdown(void)
{
    ZEnviron *env;

    for (env = _pEnvList; env != NULL; env = env->pNext)
    {
        // kill the process
        env->uSchedule = (uint32_t)(-1);
        env->iStatus = (env->pCommand)(env->pContext, 0, env->pArgv);
        env->uSchedule = (uint32_t)(-1);
    }

    while(_pEnvList != NULL)
    {
        ZTask();
        ZCleanup();
    }
}

/*F********************************************************************************/
/*!
    \Function ZCreateContext

    \Description
        Allow a process to allocate persistent private context.

    \Input        size - size of needed context

    \Output ZContext * - context of requested size

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
ZContext *ZContextCreate(int32_t size)
{
    if (_pEnvCurr == NULL)
        return(NULL);

    if (_pEnvCurr->pContext != NULL)
        ZMemFree(_pEnvCurr->pContext);

    _pEnvCurr->pContext = ((size > 0) ? (ZContext *)ZMemAlloc(size) : NULL);
    return(_pEnvCurr->pContext);
}

/*F********************************************************************************/
/*!
    \Function ZPrintf

    \Description
        Display output using standard printf semantics.

    \Input Standard printf inputs.

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
void ZPrintf(const char *fmt, ...)
{
    char text[4096];
    va_list args;
    int32_t iOutput=1;

    // parse the data
    va_start(args, fmt);
    ds_vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);

    // send to debug hook if set
    if (_Zlib_pPrintfHook != NULL)
    {
        iOutput = _Zlib_pPrintfHook(_Zlib_pPrintfParm, text);
    }

    // if debug hook didn't override output, print here
    if (iOutput != 0)
    {
        #if defined(DIRTYCODE_PC) || defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
        OutputDebugStringA(text);
        #else
        printf("%s", text);
        #endif
    }
}

/*F********************************************************************************/
/*!
    \Function ZPrintf2

    \Description
        Display output using standard printf semantics (no hook).

    \Input Standard printf inputs.

    \Version 10/18/2011 (jbrookes)
*/
/********************************************************************************F*/
void ZPrintf2(const char *fmt, ...)
{
    char text[4096];
    va_list args;

    // parse the data
    va_start(args, fmt);
    ds_vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);

    // print here
    #if defined(DIRTYCODE_PC) || defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
    OutputDebugStringA(text);
    #else
    printf("%s", text);
    #endif
}

/*F********************************************************************************/
/*!
    \Function ZPrintfHook

    \Description
        Hook into output.

    \Input *pPrintfHook - pointer to function to call with output
    \Input *pParm       - user parameter

    \Output
        None.

    \Version 03/23/2006 (jbrookes)
*/
/********************************************************************************F*/
void ZPrintfHook(int32_t (*pPrintfHook)(void *pParm, const char *pText), void *pParm)
{
    _Zlib_pPrintfHook = pPrintfHook;
    _Zlib_pPrintfParm = pParm;
}

/*F********************************************************************************/
/*!
    \Function ZCmdPS

    \Description
        Show list of all process environments.

    \Input    *argz - context
    \Input     argc - arg count
    \Input     argz - arg strings

    \Output     int32_t - exit code

    \Version 10/04/1999 (gschaefer)
*/
/********************************************************************************F*/
int32_t ZCmdPS(ZContext *argz, int32_t argc, char **pArgv)
{
    ZEnviron *env;
    uint32_t tick = ZTick();

    // handle help
    if (argc == 0) {
        ZPrintf("%s - show process status list\r\n", pArgv[0]);
        return(0);
    }

    // show process status list
    ZPrintf("  PID STATUS ITER   TIME COMMAND\r\n");
    for (env = _pEnvList; env != NULL; env = env->pNext) {
        int32_t i;
        char text[256];
        char *s = text;

        // dont show ourselves
        if (env == _pEnvCurr)
            continue;

        // put in pid
        s += wsprintf(s, "%5d", env->iPID);

        // add in state/time till next
        if (env->uSchedule == (uint32_t)(-1))
        {
            s += wsprintf(s, " F %4d", env->iStatus);
        }
        else
        {
            int32_t timeout = env->uSchedule-tick;
            if (timeout < 0)
                timeout = 0;
            if (timeout > 9999)
                timeout = 9999;
            s += wsprintf(s, " S %4d", timeout);
        }

        // show run count
        s += wsprintf(s, " %4d", (env->uRuncount < 9999 ? env->uRuncount : 9999));

        // show time used
        s += wsprintf(s, " %2d.%03d", env->uRuntime/1000, env->uRuntime%1000);

        // show command name
        s += wsprintf(s, " %s", env->pArgv[0]);

        // show command parms
        for (i = 1; i < env->iArgc; ++i)
            s += wsprintf(s, " '%s'", env->pArgv[i]);

        // end of line
        *s++ = '\r';
        *s++ = '\n';
        *s = 0;
        ZPrintf("%s", text);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function ZCmdKill

    \Description
        Kill an existing process.

    \Input    *argz - context
    \Input     argc - arg count
    \Input     argz - arg strings

    \Output     int32_t - exit code

    \Version 10/04/1999 (gschaefer)
*/
/********************************************************************************F*/
int32_t ZCmdKill(ZContext *argz, int32_t argc, char **pArgv)
{
    int32_t pid;
    char *s, *d;
    ZEnviron *env;

    // handle help
    if (argc == 0) {
        ZPrintf("%s pid|name - kill a running command\r\n", pArgv[0]);
        return(0);
    }

    // check usage
    if (argc != 2) {
        ZPrintf("usage: %s pid|name\r\n", pArgv[0]);
        return(-1);
    }

    // get the pid
    pid = 0;
    for (s = pArgv[1]; (*s >= '0') && (*s <= '9'); ++s)
        pid = (pid * 10) + (*s & 15);

    // if its zero, see if the name matches
    for (env = _pEnvList; env != NULL; env = env->pNext)
    {
        for (s = pArgv[1], d = env->pArgv[0]; *s != 0; ++s, ++d)
        {
            if (*s != *d)
                break;
        }
        if (*s == 0)
        {
            pid = env->iPID;
            break;
        }
    }

    // make sure we got something
    if (pid <= 0)
    {
        ZPrintf("%s: invalid pid: %s\r\n", pArgv[0], pArgv[1]);
        return(-2);
    }

    // search process list for match
    for (env = _pEnvList; env != NULL; env = env->pNext) {
        if ((env != _pEnvCurr) && (env->iPID == pid))
            break;
    }

    // error if no matching process
    if (env == NULL)
    {
        ZPrintf("%s: no such process %d\r\n", pArgv[0], pid);
        return(-3);
    }

    // if already dead
    if (env->uSchedule == (uint32_t)(-1))
    {
        ZPrintf("%s: process %d already dead\r\n", pArgv[0], pid);
        return(-4);
    }

    // kill the process
    env->uSchedule = (uint32_t)(-1);
    env->iStatus = (env->pCommand)(env->pContext, 0, env->pArgv);
    env->uSchedule = (uint32_t)(-1);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ZGetStatus

    \Description
        Return status of current command. Returns -1 if pEnv is Null.

    \Input  pEnv -pointer to current env, command.

    \Output Status of current command. Returns -1 if pEnv is Null.

    \Version 29/11/2005 (TE)
*/
/********************************************************************************F*/
int32_t ZGetStatus(ZEnviron *pEnv)
{
    return(pEnv ? pEnv->iStatus :-1);
}

/*F********************************************************************************/
/*!
    \Function ZGetStatusPid

    \Description
        Get status of the process specified by pid

    \Output
        int32_t     - status, or ZLIB_STATUS_UNKNOWN if process is not found

    \Version 10/10/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t ZGetStatusPid(int32_t iPID)
{
    ZEnviron *env;

    // show process status list
    for (env = _pEnvList; env != NULL; env = env->pNext)
    {
        if (env->iPID == iPID)
        {
            return(env->iStatus);
        }
    }
    return(ZLIB_STATUS_UNKNOWN);
}

/*F********************************************************************************/
/*!
    \Function ZGetIntArg

    \Description
        Get fourcc/integer from command-line argument

    \Input *pArg        - pointer to argument

    \Version 11/26/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t ZGetIntArg(const char *pArg)
{
    int32_t iValue;

    // check for possible fourcc value
    if ((strlen(pArg) == 4) && (isalpha(pArg[0]) || isalpha(pArg[1]) || isalpha(pArg[2]) || isalpha(pArg[3])))
    {
        iValue  = pArg[0] << 24;
        iValue |= pArg[1] << 16;
        iValue |= pArg[2] << 8;
        iValue |= pArg[3];
    }
    else
    {
        iValue = (signed)strtol(pArg, NULL, 10);
    }
    return(iValue);
}

/*F*************************************************************************************/
/*!
    \Function    ZTick

    \Description
        Return some kind of increasing tick count with millisecond scale (does
        not need to have millisecond precision, but higher precision is better).

    \Output
        uint32_t    - millisecond tick count

    \Version 1.0 05/06/2005 (jfrank) First Version
*/
/*************************************************************************************F*/
uint32_t ZTick(void)
{
    return(NetTick());
}


/*F********************************************************************************/
/*!
    \Function ZSleep

    \Description
        Put process to sleep for some period of time

    \Input uMSecs - Number of milliseconds to sleep for.
    
    \Version 05/06/2005 (jfrank)
*/
/********************************************************************************F*/
void ZSleep(uint32_t uMSecs)
{
    NetConnSleep(uMSecs);
}



