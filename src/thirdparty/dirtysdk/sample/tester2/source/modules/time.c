/*H********************************************************************************/
/*!
    \File time.c

    \Description
        Test the plat-time functionality.

    \Copyright
        Copyright (c) 2012 Electronic Arts

    \Version 07/12/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "libsample/zlib.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct TimeStrToTimeT
{
    char strTime[32];
    uint64_t uTime;
    TimeToStringConversionTypeE eConvType;
} TimeStrToTimeT;

/*** Variables ********************************************************************/

// Variables

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _TimeStrToTime

    \Description
        Execute ds_strtotime/ds_strtotime2 and compare to expected result

    \Output
        int32_t     - 0=success, 1=failed

    \Version 02/27/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _TimeStrToTime(const TimeStrToTimeT *pTime, int32_t iTest)
{
    uint64_t uEpoch = ds_strtotime2(pTime->strTime, pTime->eConvType);
    if (uEpoch != pTime->uTime)
    {
        ZPrintf("time: ds_strtotime2() test %d failed; got=%qd, expected=%qd\n", uEpoch, pTime->uTime);
    }
    return(uEpoch != pTime->uTime);
}

/*F********************************************************************************/
/*!
    \Function _TimeTestStrToTime

    \Description
        Test ds_strtotime/ds_strtotime2

    \Output
        int32_t     - number of test result failures

    \Version 02/27/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _TimeTestStrToTime(void)
{
    static const TimeStrToTimeT _Times[] =
    {
        { "Sun, 06 Nov 1994 08:49:37 GMT",  784111777, TIMETOSTRING_CONVERSION_UNKNOWN },   // RFC 822/1123
        { "Sun Nov  6 08:49:37 1994",       784111777, TIMETOSTRING_CONVERSION_UNKNOWN },   // asctime
        { "1994-11-06T08:49:37Z",           784111777, TIMETOSTRING_CONVERSION_ISO_8601 },
        { "941106084937",                   784111777, TIMETOSTRING_CONVERSION_ASN1_UTCTIME },
        { "641106084937",                   2993186977, TIMETOSTRING_CONVERSION_ASN1_UTCTIME },
        { "701106084937",                   26729377, TIMETOSTRING_CONVERSION_ASN1_UTCTIME },
        { "9411060849",                     784111740, TIMETOSTRING_CONVERSION_ASN1_UTCTIME },
        { "19941106084937",                 784111777, TIMETOSTRING_CONVERSION_ASN1_GENTIME },
        { "19941106084937.123",             784111777, TIMETOSTRING_CONVERSION_ASN1_GENTIME },
        { "20390101000000",                 2177452800, TIMETOSTRING_CONVERSION_ASN1_GENTIME },
        { "20501106084937",                 2551337377, TIMETOSTRING_CONVERSION_ASN1_GENTIME },
        { "21500621143040",                 5695108240, TIMETOSTRING_CONVERSION_ASN1_GENTIME }
    };
    int32_t iTime, iResult = 0;
    for (iResult = 0, iTime = 0; iTime < (int32_t)(sizeof(_Times)/sizeof(_Times[0])); iTime += 1)
    {
        iResult += _TimeStrToTime(&_Times[iTime], iTime);
    }
    return(iResult);
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function CmdTime

    \Description
        Test the time functions

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list

    \Output standard return value

    \Version 07/12/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdTime(ZContext *argz, int32_t argc, char *argv[])
{
    char strBuf[20];
    struct tm curtime;
    uint64_t uTime;
    int32_t iZone;
    void *pPlatTime = NULL;
    int32_t iResult = 0;

    // get current time... this is equivalent to time(0)
    uTime = ds_timeinsecs();
    ZPrintf("time: ds_timeinsecs()=%qd\n", uTime);

    // convert to tm time
    ds_secstotime(&curtime, uTime);
    ZPrintf("time: ds_secstotime()=%d/%d/%d %02d:%02d:%02d\n",
        curtime.tm_mon+1, curtime.tm_mday, curtime.tm_year+1900,
        curtime.tm_hour, curtime.tm_min, curtime.tm_sec);

    // test ISO_8601 conversion
    ZPrintf("time: ds_timetostr(ISO_8601)=%s\n", ds_timetostr(&curtime, TIMETOSTRING_CONVERSION_ISO_8601, 1, strBuf, sizeof(strBuf)));

    // test RFC_0822 conversion
    ZPrintf("time: ds_timetostr(RFC_0822)=%s\n", ds_timetostr(&curtime, TIMETOSTRING_CONVERSION_RFC_0822, 1, strBuf, sizeof(strBuf)));

    // get localtime
    ds_memclr(&curtime, sizeof(curtime));
    ds_localtime(&curtime, uTime);
    ZPrintf("time: ds_localtime()=%d/%d/%d %d:%d:%d\n",
        curtime.tm_mon+1, curtime.tm_mday, curtime.tm_year+1900,
        curtime.tm_hour, curtime.tm_min, curtime.tm_sec);

    // timezone (delta between local and GMT time in seconds)
    iZone = ds_timezone();
    ZPrintf("time: ds_timezone=%ds, %dh\n", iZone, iZone/(60*60));

    // test ds_strtotime()
    iResult += _TimeTestStrToTime();
    ZPrintf("time: %d failed strtotime tests\n", iResult);

    if (pPlatTime != NULL)
    {
        ds_memclr(&curtime, sizeof(curtime));
        ds_plattimetotime(&curtime, pPlatTime);
        ZPrintf("time: ds_plattimetotime()=%d/%d/%d %d:%d:%d\n",
            curtime.tm_mon+1, curtime.tm_mday, curtime.tm_year+1900,
            curtime.tm_hour, curtime.tm_min, curtime.tm_sec);
    }

    return(0);
}


