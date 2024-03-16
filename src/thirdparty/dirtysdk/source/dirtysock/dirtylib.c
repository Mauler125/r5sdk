/*H********************************************************************************/
/*!
    \File dirtylib.c

    \Description
        Platform independent routines for support library for network code.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 09/15/1999 (gschaefer) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "DirtySDK/dirtysock.h"

#if defined(DIRTYCODE_PC) || defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
#include <windows.h>
#endif

/*** Defines **********************************************************************/

// max size of line that will be tracked for netprint rate limiting
#define NETPRINT_RATELIMIT_MAXSTRINGLEN (256)

/*** Type Definitions *************************************************************/

// rate limit info
typedef struct NetPrintRateLimitT
{
    NetCritT RateCrit;
    uint32_t uRateLimitCounter;
    uint32_t uPrintTime;
    uint8_t  bRateLimitInProgress;
    char strRateLimitBuffer[NETPRINT_RATELIMIT_MAXSTRINGLEN+32];
} NetPrintRateLimitT;

/*** Variables ********************************************************************/

#if DIRTYCODE_LOGGING
// time stamp functionality
static uint8_t _NetLib_bEnableTimeStamp = TRUE;

// rate limit variables
static NetPrintRateLimitT _NetLib_RateLimit;
static uint8_t _NetLib_bRateLimitInitialized = FALSE;

// debugging hooks
static void *_NetLib_pDebugParm = NULL;
static int32_t (*_NetLib_pDebugHook)(void *pParm, const char *pText) = NULL;
#endif

// idle critical section
static NetCritT _NetLib_IdleCrit;

// idle task list
static struct
{
    void (*pProc)(void *pRef);
    void *pRef;
} _NetLib_IdleList[32];

// number of installed idle task handlers
static int32_t _NetLib_iIdleSize = 0;

//! table for calculating classic CRC-32
static const uint32_t _NetLib_Crc32Table[256] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d 
};


/*** Private Functions ************************************************************/


#if DIRTYCODE_LOGGING
/*F*************************************************************************************************/
/*!
    \Function    _NetPrintRateLimit

    \Description
        Rate limit NetPrintf output; identical lines of fewer than 256 characters that are
        line-terminated (end in a \n) will be suppressed, until a new line of text is output or
        a 100ms timer elapses.  At that point the line will be output, with additional text
        indicating the number of lines that would have been output in total.  A critical section
        guarantees integrity of the buffer and tracking information, but it is only tried to prevent
        any possibility of deadlock.  In the case of a critical section try failure, rate limit
        processing is skipped until the next print statement.

    \Input *pText   - output text to (potentially) rate limit

    \Output
        int32_t     - zero=not rate limited (line should be printed), one=rate limited (do not print)

    \Version 04/05/2016 (jbrookes)
*/
/*************************************************************************************************F*/
static int32_t _NetPrintRateLimit(const char *pText)
{
    NetPrintRateLimitT *pRateLimit = &_NetLib_RateLimit;
    int32_t iTextLen = (int32_t)strlen(pText);
    uint32_t uCurTick = NetTick(), uDiffTick = 0;
    int32_t iStrCmp, iResult = 0;
    /* rate limit IFF:
       - output is relatively small & line-terminated
       - we have initialized rate limiting
       - we are not printing rate-limited text ourselves
       - we can secure access to the critical section (MUST COME LAST!) */
    if ((iTextLen >= NETPRINT_RATELIMIT_MAXSTRINGLEN) || (pText[iTextLen-1] != '\n') || !_NetLib_bRateLimitInitialized || pRateLimit->bRateLimitInProgress || !NetCritTry(&pRateLimit->RateCrit))
    {
        return(iResult);
    }
    // does the string match our current string?
    if ((iStrCmp = strcmp(pText, pRateLimit->strRateLimitBuffer)) == 0)
    {
        // if yes, calculate tick difference between now and when the line was buffered
        uDiffTick = NetTickDiff(uCurTick, pRateLimit->uPrintTime);
    }
    // if we have a line we are rate-limiting, and either the timeout has elapsed or there is a new line
    if ((pRateLimit->uRateLimitCounter > 1) && ((uDiffTick > 100) || (iStrCmp != 0)))
    {
        // print the line, tagging on how many times it was printed
        iTextLen = (int32_t)strlen(pRateLimit->strRateLimitBuffer);
        pRateLimit->strRateLimitBuffer[iTextLen-1] = '\0';
        pRateLimit->bRateLimitInProgress = TRUE;
        NetPrintfCode("%s (%d times)\n", pRateLimit->strRateLimitBuffer, pRateLimit->uRateLimitCounter-1);
        pRateLimit->bRateLimitInProgress = FALSE;
        // set compare to non-matching to force restart below
        iStrCmp = 1;
    }
    // if this line doesn't match our current buffered line, buffer it
    if (iStrCmp != 0)
    {
        ds_strnzcpy(pRateLimit->strRateLimitBuffer, pText, sizeof(pRateLimit->strRateLimitBuffer));
        pRateLimit->uRateLimitCounter = 1;
        pRateLimit->uPrintTime = NetTick();
    }
    else
    {
        // match, so increment rate counter and suppress the line
        pRateLimit->uRateLimitCounter += 1;
        iResult = 1;
    }
    NetCritLeave(&pRateLimit->RateCrit);
    return(iResult);
}
#endif

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function NetTimeStamp

    \Description
        A printf function that returns a date time string for time stamp.
        Only if time stamps is enabled

    \Input *pBuffer - pointer to format time stamp string
    \Input iLen     - length of the supplied buffer

    \Output
        int32_t     - return the length of the Time Stamp String

    \Version 09/10/2013 (tcho)
*/
/********************************************************************************F*/
static int32_t _NetTimeStamp(char *pBuffer, int32_t iLen)
{
    int32_t iTimeStampLen = 0;

    if (_NetLib_bEnableTimeStamp == TRUE)
    {
        struct tm tm;
        int32_t imsec;

        ds_plattimetotimems(&tm, &imsec);
        iTimeStampLen = ds_snzprintf(pBuffer, iLen,"%d/%02d/%02d-%02d:%02d:%02d.%03.3d ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, imsec);
    }

    return(iTimeStampLen);
}
#endif

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function NetLibCommonInit

    \Description
        NetLib platform-common initialization

    \Version 04/05/2016 (jbrookes)
*/
/********************************************************************************F*/
void NetLibCommonInit(void)
{
    // reset the idle list
    NetIdleReset();

    // initialize critical sections
    NetCritInit(NULL, "lib-global");
    NetCritInit(&_NetLib_IdleCrit, "lib-idle");

    // set up rate limiting
    #if DIRTYCODE_LOGGING
    ds_memclr(&_NetLib_RateLimit, sizeof(_NetLib_RateLimit));
    NetCritInit(&_NetLib_RateLimit.RateCrit, "lib-rate");
    _NetLib_bRateLimitInitialized = TRUE;
    #endif
}

/*F********************************************************************************/
/*!
    \Function NetLibCommonShutdown

    \Description
        NetLib platform-common shutdown

    \Version 04/05/2016 (jbrookes)
*/
/********************************************************************************F*/
void NetLibCommonShutdown(void)
{
    // kill critical sections
    #if DIRTYCODE_LOGGING
    _NetLib_bRateLimitInitialized = FALSE;
    NetCritKill(&_NetLib_RateLimit.RateCrit);
    #endif
    NetCritKill(&_NetLib_IdleCrit);
    NetCritKill(NULL);
}

/*F********************************************************************************/
/*!
    \Function NetIdleReset

    \Description
        Reset idle function count.

    \Version 06/21/2006 (jbrookes)
*/
/********************************************************************************F*/
void NetIdleReset(void)
{
    _NetLib_iIdleSize = 0;
}

/*F********************************************************************************/
/*!
    \Function NetIdleAdd

    \Description
        Add a function to the idle callback list. The functions are called whenever
        NetIdleCall() is called.

    \Input *pProc   - callback function pointer
    \Input *pRef    - function specific parameter

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
void NetIdleAdd(void (*pProc)(void *pRef), void *pRef)
{
    // make sure proc is valid
    if (pProc == NULL)
    {
        NetPrintf(("dirtylib: attempt to add an invalid idle function\n"));
        return;
    }

    // add item to list
    _NetLib_IdleList[_NetLib_iIdleSize].pProc = pProc;
    _NetLib_IdleList[_NetLib_iIdleSize].pRef = pRef;
    _NetLib_iIdleSize += 1;
}

/*F********************************************************************************/
/*!
    \Function NetIdleDel

    \Description
        Remove a function from the idle callback list.

    \Input *pProc   - callback function pointer
    \Input *pRef    - function specific parameter

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
void NetIdleDel(void (*pProc)(void *pRef), void *pRef)
{
    int32_t iProc;

    // make sure proc is valid
    if (pProc == NULL)
    {
        NetPrintf(("dirtylib: attempt to delete an invalid idle function\n"));
        return;
    }

    // mark item as deleted
    for (iProc = 0; iProc < _NetLib_iIdleSize; ++iProc)
    {
        if ((_NetLib_IdleList[iProc].pProc == pProc) && (_NetLib_IdleList[iProc].pRef == pRef))
        {
            _NetLib_IdleList[iProc].pProc = NULL;
            _NetLib_IdleList[iProc].pRef = NULL;
            break;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function NetIdleDone

    \Description
        Make sure all idle calls have completed

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
void NetIdleDone(void)
{
    NetCritEnter(&_NetLib_IdleCrit);
    NetCritLeave(&_NetLib_IdleCrit);
}

/*F********************************************************************************/
/*!
    \Function NetIdleCall

    \Description
        Call all of the functions in the idle list.

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
void NetIdleCall(void)
{
    int32_t iProc;

    // only do idle call if we have control
    if (NetCritTry(&_NetLib_IdleCrit))
    {
        // walk the table calling routines
        for (iProc = 0; iProc < _NetLib_iIdleSize; ++iProc)
        {
            // get pProc pointer
            void (*pProc)(void *pRef) = _NetLib_IdleList[iProc].pProc;
            void *pRef = _NetLib_IdleList[iProc].pRef;

            /* if pProc is deleted, handle removal here (this
               helps prevent corrupting table in race condition) */
            if (pProc == NULL)
            {
                // swap with final element
                _NetLib_IdleList[iProc].pProc = _NetLib_IdleList[_NetLib_iIdleSize-1].pProc;
                _NetLib_IdleList[iProc].pRef = _NetLib_IdleList[_NetLib_iIdleSize-1].pRef;
                _NetLib_IdleList[_NetLib_iIdleSize-1].pProc = NULL;
                _NetLib_IdleList[_NetLib_iIdleSize-1].pRef = NULL;

                // drop the item count
                _NetLib_iIdleSize -= 1;

                // restart the loop at new current element
                --iProc;
                continue;
            }
            // perform the idle call
            (*pProc)(pRef);
        }
        // done with critical section
        NetCritLeave(&_NetLib_IdleCrit);
    }
}

/*F********************************************************************************/
/*!
    \Function NetHash

    \Description
        Calculate a unique 32-bit hash based on the given input string.

    \Input  *pString    - pointer to string to calc hash of

    \Output
        int32_t         - resultant 32bit hash

    \Version 2.0 07/26/2011 (jrainy) rewrite to lower collision rate
*/
/********************************************************************************F*/
int32_t NetHash(const char *pString)
{
    return(NetHashBin(pString, (uint32_t)strlen(pString)));
}

/*F********************************************************************************/
/*!
    \Function NetHashBin

    \Description
        Calculate a unique 32-bit hash based on the given buffer.

    \Input  *pBuffer    - pointer to buffer to calc hash of
    \Input  *uLength    - length of buffer to calc hash of

    \Output
        int32_t         - resultant 32bit hash

    \Version 1.0 02/14/2011 (jrainy) First Version
*/
/********************************************************************************F*/
int32_t NetHashBin(const void *pBuffer, uint32_t uLength)
{
    // prime factor to multiply by at each 16-char block boundary
    static uint32_t uShift = 436481627;

    // prime factors to multiply individual characters by
    static uint32_t uFactors[16] = {
        682050377,  933939593,  169587707,  131017121,
        926940523,  102453581,  543947221,  775968049,
        129461173,  793216343,  870352919,  455044847,
        747808279,  727551509,  431178773,  519827743};

    // running hash
    uint32_t uSum = 0;
    uint32_t uChar;

    for (uChar = 0; uChar != uLength; uChar++)
    {
        // at each 16-byte boundary, multiply the running hash by fixed factor
        if ((uChar & 0xf) == 0)
        {
            uSum *= uShift;
        }
        // sum up the value of the char at position iChar by prime factor iChar%16
        uSum += ((uint8_t)((char*)pBuffer)[uChar]) * uFactors[uChar & 0xf];
    }

    return((int32_t)uSum);
}


/*F********************************************************************************/
/*!
    \Function NetCrc32

    \Description
        Calculate CRC32 of specified data.  If no table is specified, the default
        table is used.

    \Input *pBuffer     - buffer to calculate crc32
    \Input iBufLen      - length of buffer
    \Input *pCrcTable   - crc32 table to use, or NULL for the default

    \Output
        int32_t         - CRC32 of input data

    \Version 01/17/2019 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetCrc32(const uint8_t *pBuffer, int32_t iBufLen, const uint32_t *pCrcTable)
{
    uint32_t uCrc32;
    // use default table if none specified
    if (pCrcTable == NULL)
    {
        pCrcTable = _NetLib_Crc32Table;
    }
    // calculate crc
    for (uCrc32 = 0xffffffff; iBufLen > 0; iBufLen -= 1)
    {
        uCrc32 = (uCrc32 >> 8) ^ pCrcTable[(uCrc32 ^ *pBuffer++) & 0xff];
    }
    // return to caller
    return(uCrc32 ^ 0xffffffff);
}

/*F*************************************************************************************************/
/*!
    \Function NetRand

    \Description
        A simple pseudo-random sequence generator. The sequence is implicitly seeded in the first
        call with the millisecond tick count at the time of the call

    \Input uLimit   - upper bound of pseudo-random number output

    \Output
        uint32_t     - pseudo-random number from [0...(uLimit - 1)]

    \Version 06/25/2009 (jbrookes)
*/
/*************************************************************************************************F*/
uint32_t NetRand(uint32_t uLimit)
{
    static uint32_t _aRand = 0;
    if (_aRand == 0)
    {
        _aRand = NetTick();
    }
    if (uLimit == 0)
    {
        return(0);
    }
    _aRand = (_aRand * 125) % 2796203;
    return(_aRand % uLimit);
}

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function NetTimeStampEnableCode

    \Description
        Enables Time Stamp in Logging

    \Input bEnableTimeStamp  - TRUE to enable Time Stamp in Logging

    \Version 9/11/2014 (tcho)
*/
/********************************************************************************F*/
void NetTimeStampEnableCode(uint8_t bEnableTimeStamp)
{
    _NetLib_bEnableTimeStamp = bEnableTimeStamp;
}
#endif

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function NetPrintfHook

    \Description
        Hook into debug output.

    \Input *pPrintfDebugHook    - pointer to function to call with debug output
    \Input *pParm               - user parameter

    \Version 03/29/2005 (jbrookes)
*/
/********************************************************************************F*/
void NetPrintfHook(int32_t (*pPrintfDebugHook)(void *pParm, const char *pText), void *pParm)
{
    _NetLib_pDebugHook = pPrintfDebugHook;
    _NetLib_pDebugParm = pParm;
}
#endif

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function NetPrintfCode

    \Description
        Debug formatted output

    \Input *pFormat - pointer to format string
    \Input ...      - variable argument list

    \Output
        int32_t     - zero

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
int32_t NetPrintfCode(const char *pFormat, ...)
{
    va_list pFmtArgs;
    char strText[4096];
    const char *pText = strText;
    int32_t iOutput = 1;
    int32_t iTimeStampLen = 0;

    // init the string buffer (not done at instantiation so as to avoid having to clear the entire array, for perf)
    strText[0] = '\0';

    // only returns time stamp if time stamp is enabled
    iTimeStampLen = _NetTimeStamp(strText, sizeof(strText));

    // format the text
    va_start(pFmtArgs, pFormat);
    if ((pFormat[0] == '%') && (pFormat[1] == 's') && (pFormat[2] == '\0'))
    {
        ds_strnzcat(strText + iTimeStampLen, va_arg(pFmtArgs, const char *), sizeof(strText) - iTimeStampLen);
    }
    else
    {
        ds_vsnprintf(strText + iTimeStampLen, sizeof(strText) - iTimeStampLen, pFormat, pFmtArgs);
    }
    va_end(pFmtArgs);

    // check for rate limit (omit timestamp from consideration)
    if (_NetPrintRateLimit(strText+iTimeStampLen))
    {
        return(0);
    }

    // forward to debug hook, if defined
    if (_NetLib_pDebugHook != NULL)
    {
       iOutput = _NetLib_pDebugHook(_NetLib_pDebugParm, pText);
    }

    // output to debug output, unless suppressed by debug hook
    if (iOutput != 0)
    {
        #if defined(DIRTYCODE_PC) || defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
        OutputDebugStringA(pText);
        #else
        printf("%s", pText);
        #endif
    }

    return(0);
}
#endif

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function NetPrintfVerboseCode

    \Description
        Display input data if iVerbosityLevel is > iCheckLevel

    \Input iVerbosityLevel  - current verbosity level
    \Input iCheckLevel      - level to check against
    \Input *pFormat         - format string

    \Version 12/20/2005 (jbrookes)
*/
/********************************************************************************F*/
void NetPrintfVerboseCode(int32_t iVerbosityLevel, int32_t iCheckLevel, const char *pFormat, ...)
{
    va_list Args;
    char strText[1024];

    if (iVerbosityLevel > iCheckLevel)
    {
        va_start(Args, pFormat);
        ds_vsnprintf(strText, sizeof(strText), pFormat, Args);
        va_end(Args);

        NetPrintf(("%s", strText));
    }
}
#endif

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function NetPrintWrapCode

    \Description
        Display input data with wrapping.

    \Input *pString - pointer to packet data to display
    \Input iWrapCol - number of columns to wrap at

    \Version 09/15/2005 (jbrookes)
*/
/********************************************************************************F*/
void NetPrintWrapCode(const char *pString, int32_t iWrapCol)
{
    const char *pTemp, *pEnd, *pEqual, *pSpace;
    char strTemp[256] = "   ";
    uint32_t bDone;
    int32_t iLen;

    // loop through whole packet
    for (bDone = FALSE; bDone == FALSE; )
    {
        // scan forward, tracking whitespace, linefeeds, and equal signs
        for (pTemp=pString, pEnd=pTemp+iWrapCol, pSpace=NULL, pEqual=NULL; (pTemp < pEnd); pTemp++)
        {
            // remember most recent whitespace
            if ((*pTemp == ' ') || (*pTemp == '\t'))
            {
                pSpace = pTemp;
            }

            // remember most recent equal sign
            if (*pTemp == '=')
            {
                pEqual = pTemp;
            }

            // if eol or eos, break here
            if ((*pTemp == '\n') || (*pTemp == '\0'))
            {
                break;
            }
        }

        // scanned an entire line?
        if (pTemp == pEnd)
        {
            // see if we have whitespace to break on
            if (pSpace != NULL)
            {
                pTemp = pSpace;
            }
            // see if we have an equals to break on
            else if (pEqual != NULL)
            {
                pTemp = pEqual;
            }
        }

        // format string for output
        iLen = (int32_t)(pTemp - pString + 1);
        strncpy(strTemp + 3, pString, iLen);
        if (*pTemp == '\0')
        {
            strTemp[iLen+2] = '\n';
            strTemp[iLen+3] = '\0';
            bDone = TRUE;
        }
        else if ((*pTemp != '\n') && (*pTemp != '\r'))
        {
            strTemp[iLen+3] = '\n';
            strTemp[iLen+4] = '\0';
        }
        else
        {
            strTemp[iLen+3] = '\0';
        }

        // print it out
        NetPrintf(("%s", strTemp));

        // increment to next line
        pString += iLen;
    }
}
#endif // #if DIRTYCODE_LOGGING

#if DIRTYCODE_LOGGING
/*F*************************************************************************************************/
/*!
    \Function    NetPrintMemCode

    \Description
        Dump memory to debug output

    \Input *pMem    - pointer to memory to dump
    \Input iSize    - size of memory block to dump
    \Input *pTitle  - pointer to title of memory block

    \Version    1.0        05/17/05 (jbrookes) First Version
*/
/*************************************************************************************************F*/
void NetPrintMemCode(const void *pMem, int32_t iSize, const char *pTitle)
{
    static const char _hex[] = "0123456789ABCDEF";
    char strOutput[128];
    int32_t iBytes, iOutput = 2;

    ds_memset(strOutput, ' ', sizeof(strOutput)-1);
    strOutput[sizeof(strOutput)-1] = '\0';

    NetPrintf(("dirtylib: dumping memory for object %s (%d bytes)\n", pTitle, iSize));

    for (iBytes = 0; iBytes < iSize; iBytes++, iOutput += 2)
    {
        unsigned char cByte = ((unsigned char *)pMem)[iBytes];
        strOutput[iOutput]   = _hex[cByte>>4];
        strOutput[iOutput+1] = _hex[cByte&0xf];
        strOutput[(iOutput/2)+40] = isprint(cByte) ? cByte : '.';
        if (iBytes > 0)
        {
            if (((iBytes+1) % 16) == 0)
            {
                strOutput[(iOutput/2)+40+1] = '\0';
                NetPrintf(("%s\n", strOutput));
                ds_memset(strOutput, ' ', sizeof(strOutput)-1);
                strOutput[sizeof(strOutput)-1] = '\0';
                iOutput = 0;
            }
            else if (((iBytes+1) % 4) == 0)
            {
                iOutput++;
            }
        }
    }

    if ((iBytes % 16) != 0)
    {
        strOutput[(iOutput/2)+40+1] = '\0';
        NetPrintf(("%s\n", strOutput));
    }
}
#endif

#if DIRTYCODE_LOGGING
/*F*************************************************************************************************/
/*!
    \Function    NetPrintArrayCode

    \Description
        Dump memory to debug output in the form of a c-style array declaration

    \Input *pMem    - pointer to memory to dump
    \Input iSize    - size of memory block to dump
    \Input *pTitle  - pointer to title of memory block

    \Version 06/05/2014 (jbrookes)
*/
/*************************************************************************************************F*/
void NetPrintArrayCode(const void *pMem, int32_t iSize, const char *pTitle)
{
    static const char _hex[] = "0123456789ABCDEF";
    char strOutput[128];
    int32_t iBytes, iOutput = 4;

    ds_memset(strOutput, ' ', sizeof(strOutput)-1);
    strOutput[sizeof(strOutput)-1] = '\0';

    NetPrintf(("dirtylib: dumping declaration for object %s (%d bytes)\n", pTitle, iSize));
    NetPrintf(("static const uint8_t %s[] =\n", pTitle));
    NetPrintf(("{\n"));

    for (iBytes = 0; iBytes < iSize; iBytes++)
    {
        uint8_t cByte = ((uint8_t *)pMem)[iBytes];
        strOutput[iOutput+0] = '0';
        strOutput[iOutput+1] = 'x';
        strOutput[iOutput+2] = _hex[cByte>>4];
        strOutput[iOutput+3] = _hex[cByte&0xf];
        strOutput[iOutput+4] = ',';
        iOutput += 5;
        if (iBytes > 0)
        {
            if (((iBytes+1) % 16) == 0)
            {
                strOutput[iOutput] = '\0';
                NetPrintf(("%s\n", strOutput));
                ds_memset(strOutput, ' ', sizeof(strOutput)-1);
                strOutput[sizeof(strOutput)-1] = '\0';
                iOutput = 4;
            }
            else if (((iBytes+1) % 4) == 0)
            {
                iOutput++;
            }
        }
    }

    if ((iBytes % 16) != 0)
    {
        strOutput[iOutput] = '\0';
        NetPrintf(("%s\n", strOutput));
    }

    NetPrintf(("};\n"));
}
#endif
