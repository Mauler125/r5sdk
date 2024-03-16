/*H********************************************************************************/
/*!
    \File zlib.h

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

#ifndef _zlib_h
#define _zlib_h

/*** Include files ****************************************************************/

#include "DirtySDK/dirtysock/dirtylib.h"

/*** Defines **********************************************************************/

#define ZLIB_STATUS_RUNNING ((signed)0x80000000)
#define ZLIB_STATUS_UNKNOWN ((signed)0x80000001)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct ZEnviron ZEnviron;
typedef struct ZContext ZContext;
typedef struct ZConsole ZConsole;
typedef int32_t (ZCommand)(ZContext *pArgz, int32_t iArgc, char **pArgv);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create a new process environment. Parses command line and gets process ready to run.
DIRTYCODE_API ZEnviron *ZCreate(ZConsole *pConsole, const char *pCmdline);

// destroy process environment
DIRTYCODE_API void ZDestroy(ZEnviron *pEnv);

// execute a process within an existing environment.
DIRTYCODE_API void ZInvoke(ZEnviron *pEnv, ZCommand *pCmd);

// get process id of current process
DIRTYCODE_API int32_t ZGetPid(void);

// set current environment from context
DIRTYCODE_API void ZSet(ZContext *context);

// let process signal it wants a callback (has not completed).
DIRTYCODE_API int32_t ZCallback(ZCommand *pCmd, int32_t iDelay);

// give time to any existing processes that need it.
DIRTYCODE_API int32_t ZTask(void);

// remove any environments containing complete processes.
DIRTYCODE_API void ZCleanup(void);

// kill all active processes in preparation for shutdown
DIRTYCODE_API void ZShutdown(void);

// allow a process to allocate persistant private context.
DIRTYCODE_API ZContext *ZContextCreate(int32_t iSize);

// return tick count in milliseconds.
DIRTYCODE_API uint32_t ZTick(void);

// put process to sleep for some period of time
DIRTYCODE_API void ZSleep(uint32_t uMSecs);

// display output using standard printf semantics.
DIRTYCODE_API void ZPrintf(const char *pFmt, ...);

// display output using standard printf semantics (no hook)
DIRTYCODE_API void ZPrintf2(const char *pFmt, ...);

#if DIRTYCODE_LOGGING
 #define ZPrintfDbg(_x) ZPrintf _x
#else
 #define ZPrintfDbg(_x) { }
#endif

// set zprint callback
DIRTYCODE_API void ZPrintfHook(int32_t (*pPrintfHook)(void *pParm, const char *pText), void *pParm);

// show list of all process environments.
DIRTYCODE_API int32_t ZCmdPS(ZContext *pArgz, int32_t iArgc, char **pArgv);

// kill an existing process.
DIRTYCODE_API int32_t ZCmdKill(ZContext *pArgz, int32_t iArgc, char **pArgv);

// return status of current env, command.
DIRTYCODE_API int32_t ZGetStatus(ZEnviron *pEnv);

// return status of process with specified pid
DIRTYCODE_API int32_t ZGetStatusPid(int32_t iPid);

// get fourcc/integer from command-line argument
DIRTYCODE_API int32_t ZGetIntArg(const char *pArg);

#ifdef __cplusplus
};
#endif

#endif // _zlib_h


