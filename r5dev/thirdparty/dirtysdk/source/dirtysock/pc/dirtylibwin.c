/*H********************************************************************************/
/*!

    \File    dirtylibwin.c

    \Description
        Platform specific support library for network code. Suppplies
        simple time, memory, and semaphore functions.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002.  ALL RIGHTS RESERVED.

    \Version    1.0        01/02/02 (GWS) Initial version (ported from PS2 IOP)

*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)

#include "DirtySDK/platform.h"

#if !defined(DIRTYCODE_XBOXONE)
#include <timeapi.h>
#endif

#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>

#include <stdio.h>
#include "DirtySDK/dirtysock/dirtythread.h"
#include "DirtySDK/dirtysock/dirtylib.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Function Prototypes **********************************************************/

static uint32_t _NetLibGetTickCount2(void);

/*** Variables ********************************************************************/

// Private variables

// idle thread state
static volatile int32_t g_idlelife = -1;

// queryperformancecounter frequency (static init so calls to NetTick() before NetLibCreate won't crash)
static LARGE_INTEGER _NetLib_lFreq = { 1 };

#if defined(DIRTYCODE_PC) || defined(DIRTYCODE_GDK)
uint32_t _NetLib_bUseHighResTimer = FALSE;
#else
uint32_t _NetLib_bUseHighResTimer = TRUE;
#endif

// selected timer function
static uint32_t (*_NetLib_pTimerFunc)(void) = _NetLibGetTickCount2;

// Public variables


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _NetLibThread

    \Description
        Thread to handle special library tasks

    \Input _null    -   unused

    \Version 01/02/2002 (gschaefer)
*/
/********************************************************************************F*/
static void _NetLibThread(void *_null)
{
    char strThreadId[32];

    // get the thread id
    DirtyThreadGetThreadId(strThreadId, sizeof(strThreadId));

    // show we are alive
    NetPrintf(("dirtylibwin: idle thread running (thid=%s)\n", strThreadId));
    g_idlelife = 1;

    // run while we have sema
    while (g_idlelife == 1)
    {
        // call idle functions
        NetIdleCall();
        // wait for next tick
        Sleep(50);
    }

    // report termination
    NetPrintf(("dirtylibwin: idle thread exiting\n"));

    // show we are dead
    g_idlelife = 0;
}

/*F********************************************************************************/
/*!
    \Function _NetLibGetTickCount

    \Description
        Millisecond-accurate tick counter.

    \Output
        uint32_t        - millisecond tick counter

    \Version 11/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _NetLibGetTickCount(void)
{
    LARGE_INTEGER lCount;
    QueryPerformanceCounter(&lCount);
    return((uint32_t)((lCount.QuadPart*1000)/_NetLib_lFreq.QuadPart));
}

/*F********************************************************************************/
/*!
    \Function _NetLibGetTickCount2

    \Description
        Millisecond tick counter, with variable precision.

    \Output
        uint32_t        - millisecond tick counter

    \Version 11/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _NetLibGetTickCount2(void)
{
    #if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
    return(_NetLibGetTickCount());  //$$TODO -- better method than querying high performance counter?
    #else
    return(timeGetTime());
    #endif
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function NetLibCreate

    \Description
        Initialize the network library functions

    \Input iThreadPrio        - priority to start the _NetLibThread with
    \Input iThreadStackSize   - stack size to start the _NetLibThread with (in bytes)
    \Input iThreadCpuAffinity - cpu affinity to start the _NetLibThread with

    \Version 01/02/2002 (gschaefer)
*/
/********************************************************************************F*/
void NetLibCreate(int32_t iThreadPrio, int32_t iThreadStackSize, int32_t iThreadCpuAffinity)
{
    DirtyThreadConfigT ThreadConfig;
    int32_t iResult;

    // init common netlib functionality
    NetLibCommonInit();

    // configure thread parameters
    ds_memclr(&ThreadConfig, sizeof(ThreadConfig));
    ThreadConfig.pName = "NetLib";
    ThreadConfig.iPriority = iThreadPrio;
    ThreadConfig.iAffinity = iThreadCpuAffinity;
    ThreadConfig.iVerbosity = 1;

    // create a worker thread
    if ((iResult = DirtyThreadCreate(_NetLibThread, NULL, &ThreadConfig)) != 0)
    {
        NetPrintf(("dirtylibwin: unable to create netidle thread (err=%d)\n", iResult));
        g_idlelife = 0;
    }
    
    // set up high-performance timer, if available
    if (QueryPerformanceFrequency(&_NetLib_lFreq) == 0)
    {
        NetPrintf(("dirtylibwin: high-frequency performance counter not available\n"));
    }
    // use high-performance timer for tick counter?
    if (_NetLib_bUseHighResTimer)
    {
        _NetLib_pTimerFunc = _NetLibGetTickCount;
    }
}

/*F********************************************************************************/
/*!
    \Function NetLibDestroy

    \Description
        Destroy the network lib

    \Input uShutdownFlags   - NET_SHUTDOWN_* flags

    \Version 01/02/02 (gschaefer)
*/
/********************************************************************************F*/
void NetLibDestroy(uint32_t uShutdownFlags)
{
    // if the thread is running
    if (g_idlelife == 1)
    {
        // signal a shutdown
        g_idlelife = 2;

        // wait for thread to terminate
        while (g_idlelife > 0)
        {
            Sleep(1);
        }
    }

    // shut down common functionality
    NetLibCommonShutdown();
}

/*F********************************************************************************/
/*!
    \Function NetTick

    \Description
        Return some kind of increasing tick count with millisecond scale (does
        not need to have millisecond precision, but higher precision is better).

    \Output
        uint32_t    - millisecond tick count

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
uint32_t NetTick(void)
{
    return(_NetLib_pTimerFunc());
}

/*F********************************************************************************/
/*!
    \Function NetTickUsec

    \Description
        Return increasing tick count in microseconds.  Used for performance timing
        purposes.

    \Output
        uint64_t    - microsecond tick count

    \Version 01/30/2015 (jbrookes)
*/
/********************************************************************************F*/
uint64_t NetTickUsec(void)
{
    LARGE_INTEGER lCount;
    QueryPerformanceCounter(&lCount);
    return((uint64_t)((lCount.QuadPart*1000000)/_NetLib_lFreq.QuadPart));
}

/*F********************************************************************************/
/*!
    \Function NetLocalTime

    \Description
        This converts the input GMT time to the local time as specified by the
        system clock.  This function follows the re-entrant localtime_r function
        signature.

    \Input *pTm         - storage for localtime output
    \Input uElap        - GMT time

    \Output
        struct tm *     - pointer to localtime result

    \Version 04/23/2008 (jbrookes)
*/
/********************************************************************************F*/
struct tm *NetLocalTime(struct tm *pTm, uint64_t uElap)
{
    time_t uTimeT = (time_t)uElap;
    localtime_s(pTm, &uTimeT);
    return(pTm);
}

/*F********************************************************************************/
/*!
    \Function NetPlattimeToTime

    \Description
        This converts the input platform-specific time data structure to the
        generic time data structure.

    \Input *pTm         - generic time data structure to be filled by the function
    \Input *pPlatTime   - pointer to the platform-specific data structure

    \Output
        struct tm *     - NULL=failure; else pointer to user-provided generic time data structure

    \Notes
        pPlatTime is expected to point to a __timeb64 on PC platforms, and a
        SYSTEMTIME on Xbox One.

    \Version 05/08/2010 (mclouatre)
*/
/********************************************************************************F*/
struct tm *NetPlattimeToTime(struct tm *pTm, void *pPlatTime)
{
    #if defined(DIRTYCODE_PC)
    struct __timeb64 timebuffer = *(struct __timeb64 *)pPlatTime;
    struct tm resultTm = *(_localtime64(&(timebuffer.time)));

    pTm->tm_sec = resultTm.tm_sec;
    pTm->tm_min = resultTm.tm_min;
    pTm->tm_hour = resultTm.tm_hour;
    pTm->tm_mday = resultTm.tm_mday;
    pTm->tm_mon = resultTm.tm_mon;
    pTm->tm_year = resultTm.tm_year;
    pTm->tm_wday = resultTm.tm_wday;
    pTm->tm_yday = resultTm.tm_yday;
    pTm->tm_isdst =resultTm.tm_isdst;
    #else  // XBOXONE
    SYSTEMTIME systemTime = *(SYSTEMTIME *)pPlatTime;

    pTm->tm_sec = systemTime.wSecond;
    pTm->tm_min = systemTime.wMinute;
    pTm->tm_hour = systemTime.wHour;
    pTm->tm_mday = systemTime.wDay;
    pTm->tm_mon = systemTime.wMonth - 1;
    pTm->tm_year = systemTime.wYear - 1900;
    pTm->tm_wday = systemTime.wDayOfWeek;
    pTm->tm_yday = 0;
    pTm->tm_isdst = 0;
    #endif

    return(pTm);
}

/*F********************************************************************************/
/*!
    \Function NetPlattimeToTimeMs
 
    \Description
        This function retrieves the current date time and fills in the
        generic time data structure prrovided. It has the option of returning millisecond
        which is not part of the generic time data structure

    \Input *pTm         - generic time data structure to be filled by the function
    \Input *pImsec      - output param for milisecond to be filled by the function (optional can be NULL)

    \Output
        struct tm *     - NULL=failure; else pointer to user-provided generic time data structure

    \Version 09/16/2014 (tcho)
*/
/********************************************************************************F*/
struct tm *NetPlattimeToTimeMs(struct tm *pTm , int32_t *pImsec)
{
    void *pPlatTime;
    int32_t iMsec;
    
    #if defined(DIRTYCODE_PC)
    struct __timeb64 timebuffer;
    _ftime64_s(&timebuffer);
    iMsec = timebuffer.millitm;
    pPlatTime = (void *)&timebuffer;
    #else // XBOXONE
    SYSTEMTIME systemTime;
    GetLocalTime( &systemTime );
    iMsec = systemTime.wMilliseconds;
    pPlatTime = (void *)&systemTime;
    #endif

    if (pImsec != NULL)
    {
        *pImsec = iMsec;
    }
   
    if (pTm == NULL)
    {
        return(NULL);
    }

    return(NetPlattimeToTime(pTm, pPlatTime));
}

/*F********************************************************************************/
/*!
    \Function NetTime

    \Description
        This function replaces the standard library time() function. Main
        differences are the missing pointer parameter (not needed) and the uint64_t
        return value. The function returns 0 on unsupported platforms vs time which
        returns -1.

    \Output
        uint64_t    - number of elapsed seconds since Jan 1, 1970.

    \Version 01/12/2005 (gschaefer)
*/
/********************************************************************************F*/
uint64_t NetTime(void)
{
    return((uint64_t)time(NULL));
}
