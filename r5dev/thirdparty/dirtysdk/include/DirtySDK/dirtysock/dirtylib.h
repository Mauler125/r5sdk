/*H**************************************************************************************/
/*!
    \File    dirtylib.h

    \Description
        Provide basic library functions for use by network layer code.
        This is needed because the network code is platform/project
        independent and needs to rely on a certain set of basic
        functions.

    \Copyright
        Copyright (c) Electronic Arts 2001-2018

    \Version    0.5        08/01/01 (GWS) First Version
    \Version    1.0        12/31/01 (GWS) Redesigned for Tiburon environment
*/
/**************************************************************************************H*/

#ifndef _dirtylib_h
#define _dirtylib_h

/*!
\Moduledef DirtyLib DirtyLib
\Modulemember DirtySock
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

// define platform-specific options
#ifndef DIRTYCODE_LOGGING
 #if DIRTYCODE_DEBUG
  //in debug mode logging is defaulted to on
  #define DIRTYCODE_LOGGING (1)
 #else
  //if its not specified then turn it off
  #define DIRTYCODE_LOGGING (0)
 #endif
#endif

//! define NetCrit options
#define NETCRIT_OPTION_NONE               (0)   //!< default settings
#define NETCRIT_OPTION_SINGLETHREADENABLE (1)   //!< enable the crit even when in single-threaded mode

// debug printing routines
#if DIRTYCODE_LOGGING
 #define NetPrintf(_x) NetPrintfCode _x
 #define NetPrintfVerbose(_x) NetPrintfVerboseCode _x
 #define NetPrintArray(_pMem, _iSize, _pTitle) NetPrintArrayCode(_pMem, _iSize, _pTitle)
 #define NetPrintMem(_pMem, _iSize, _pTitle) NetPrintMemCode(_pMem, _iSize, _pTitle)
 #define NetPrintWrap(_pString, _iWrapCol) NetPrintWrapCode(_pString, _iWrapCol)
 #define NetTimeStampEnable(_bEnableTimeStamp) NetTimeStampEnableCode(_bEnableTimeStamp)
#else
 #define NetPrintf(_x) { }
 #define NetPrintfVerbose(_x) { }
 #define NetPrintArray(_pMem, _iSize, _pTitle) { }
 #define NetPrintMem(_pMem, _iSize, _pTitle) { }
 #define NetPrintWrap(_pString, _iWrapCol) { }
 #define NetTimeStampEnable(_bEnableTimeStamp) { }
#endif

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct NetCritPrivT NetCritPrivT;

//! critical section definition
typedef struct NetCritT
{
    NetCritPrivT *pData;
} NetCritT;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*
 Portable routines implemented in dirtynet.c
*/

// platform-common create (called internally by NetLibCreate)
DIRTYCODE_API void NetLibCommonInit(void);

// platform-common shutdown (called internally by NetLibDestroy)
DIRTYCODE_API void NetLibCommonShutdown(void);

// reset net idle list
DIRTYCODE_API void NetIdleReset(void);

// remove a function to the idle callback list.
DIRTYCODE_API void NetIdleAdd(void (*proc)(void *ref), void *ref);

// call all the functions in the idle list.
DIRTYCODE_API void NetIdleDel(void (*proc)(void *ref), void *ref);

// make sure all idle calls have completed
DIRTYCODE_API void NetIdleDone(void);

// add a function to the idle callback list
DIRTYCODE_API void NetIdleCall(void);

// return 32-bit hash from given input string
DIRTYCODE_API int32_t NetHash(const char *pString);

// return 32-bit hash from given buffer
DIRTYCODE_API int32_t NetHashBin(const void *pBuffer, uint32_t uLength);

// return 32-bit CRC from given buffer
DIRTYCODE_API int32_t NetCrc32(const uint8_t *pBuffer, int32_t iBufLen, const uint32_t *pCrcTable);

// A simple psuedo-random sequence generator
DIRTYCODE_API uint32_t NetRand(uint32_t uLimit);

// return time
DIRTYCODE_API uint64_t NetTime(void);

// enable logging time stamp
DIRTYCODE_API void NetTimeStampEnableCode(uint8_t bEnableTimeStamp);

// hook into debug output
DIRTYCODE_API void NetPrintfHook(int32_t (*pPrintfDebugHook)(void *pParm, const char *pText), void *pParm);

// diagnostic output routine (do not call directly, use NetPrintf() wrapper
DIRTYCODE_API int32_t NetPrintfCode(const char *fmt, ...);

// diagnostic output routine (do not call directly, use NetPrintf() wrapper
DIRTYCODE_API void NetPrintfVerboseCode(int32_t iVerbosityLevel, int32_t iCheckLevel, const char *pFormat, ...);

// print input buffer with wrapping (do not call directly; use NetPrintWrap() wrapper)
DIRTYCODE_API void NetPrintWrapCode(const char *pData, int32_t iWrapCol);

// print memory as hex (do not call directly; use NetPrintMem() wrapper)
DIRTYCODE_API void NetPrintMemCode(const void *pMem, int32_t iSize, const char *pTitle);

// print memory as a c-style array (do not call directly; use NetPrintArray() wrapper)
DIRTYCODE_API void NetPrintArrayCode(const void *pMem, int32_t iSize, const char *pTitle);

/*
 Platform-specific routines implemented in dirtynet<platform>.c
*/

// initialize the network library functions.
DIRTYCODE_API void NetLibCreate(int32_t iThreadPrio, int32_t iThreadStackSize, int32_t iThreadCpuAffinity);

// shutdown the network library.
DIRTYCODE_API void NetLibDestroy(uint32_t uShutdownFlags);

// return an increasing tick count with millisecond scale
DIRTYCODE_API uint32_t NetTick(void);

// return microsecond timer, intended for debug timing purposes only
DIRTYCODE_API uint64_t NetTickUsec(void);


/*
The NetTickDiff() macro implies 2 steps.

The first step consists in substracting 2 unsigned values. When working with unsigned
types, modular arithmetic (aka "wrap around" behavior) is taking place. It is similar
to reading a clock.
    Adding clockwise:               9 + 4 = 1  (13 mod 12)
    Substracting counterclockwise:  1 - 4 = 9  (-3 mod 12)
Obviously the value range here is [0,0xFFFFFFFF] and not [0,11].
By the virtue of modular arithmetic, the difference between _uNewTime and _uOldTime is 
always valid, even in scenarios where one (or both) of the two values has just
"wrapped around".

The second step consists in casting the unsigned result of step 1 into a signed
integer. The result of that second step is the final outcome of the macro, i.e. a
value ranging between 
    -2147483648 (2's complement notation: 0x80000000) and 
     2147483647 (2's complement notation: 0x7FFFFFFF)

Consequently, the maximum time difference (positive or negative) that can be calculated
between _uNewTime and _uOldTime is 2147483647 ms, i.e. approximately 596,8 hours (24,9 days). 

Any longer period of time captured with an initial call to NetTick() and a final 
call to NetTick() and then calculated by feeding both values to NetTickDiff() would
incorrectly result in a time difference much shorter than reality.

If _uNewTime is more recent than _uOldTime (by not more than 596,8 hours), then
the returned time difference will be positive.

If _uOldTime is more recent than _uNewTime (by not more than 596,8 hours), then
the returned time difference will be negative.
*/

// return signed difference between new tick count and old tick count (new - old)
#define NetTickDiff(_uNewTime, _uOldTime) ((signed)((_uNewTime) - (_uOldTime)))

// return localtime
DIRTYCODE_API struct tm *NetLocalTime(struct tm *pTm, uint64_t uElap);

// convert a platform-specific time format to generic time format
DIRTYCODE_API struct tm *NetPlattimeToTime(struct tm *pTm, void *pPlatTime);

// convert a platform-specific time format to generic time format, with milliseconds
DIRTYCODE_API struct tm *NetPlattimeToTimeMs(struct tm *pTm, int32_t *pImSec);

// initialize a critical section for use -- includes name for verbose debugging on some platforms
DIRTYCODE_API int32_t NetCritInit(NetCritT *pCrit, const char *pCritName);

// initialize a critical section with the ability to set options (NETCRIT_OPTION_*)
DIRTYCODE_API int32_t NetCritInit2(NetCritT *pCrit, const char *pCritName, uint32_t uFlags);

// release resources and destroy critical section
DIRTYCODE_API void NetCritKill(NetCritT *pCrit);

// attempt to gain access to critical section
DIRTYCODE_API int32_t NetCritTry(NetCritT *pCrit);

// enter a critical section, blocking if needed
DIRTYCODE_API void NetCritEnter(NetCritT *pCrit);

// leave a critical section
DIRTYCODE_API void NetCritLeave(NetCritT *pCrit);

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtylib_h

