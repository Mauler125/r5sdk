/*H********************************************************************************/
/*!
    \File plat-time.c

    \Description
        This module implements variations on the standard library time functions.
        The originals had a number of problems with internal static storage,
        bizarre parameter passing (pointers to input values instead of the
        actual value) and time_t which is different on different platforms.

        All these functions work with uint32_t values which provide a time
        range of 1970-2107.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 01/11/2005 (gschaefer) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"

#if defined(DIRTYCODE_PS4)
#include <kernel.h>
#elif defined(DIRTYCODE_PC) || defined(DIRTYCODE_XBOXONE)
#include <windows.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#elif defined (DIRTYCODE_LINUX) || defined(DIRTYCODE_ANDROID) || defined(DIRTYCODE_APPLEIOS) || defined(DIRTYCODE_APPLEOSX)
#include <sys/time.h>
#endif

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

static const char *_PlatTime_strWday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", NULL };
static const char *_PlatTime_strMonth[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL };

/*** Private functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ds_strtoint

    \Description
        Converts a string number to an integer.  Does not support sign.

    \Input *pStr        - pointer to int to read
    \Input *pValue      - [out] storage for result
    \Input iMaxDigits   - max number of digits to convert

    \Output
        const char *    - pointer past end of number

    \Version 12/13/2012 (jbrookes)
*/
/********************************************************************************F*/
static const char *_ds_strtoint(const char *pStr, int32_t *pValue, int32_t iMaxDigits)
{
    int32_t iNum, iDigit;
    for (iNum = 0, iDigit = 0; ((*pStr >= '0') && (*pStr <= '9') && (iDigit < iMaxDigits)); pStr += 1, iDigit += 1)
    {
        iNum = (iNum * 10) + (*pStr & 15);
    }
    *pValue = iNum;
    return(pStr);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ds_timeinsecs

    \Description
        Return time in seconds, or zero if not available

    \Output uint64_t - number of elapsed seconds since Jan 1, 1970

    \Version 01/12/2005 (gschaefer)
*/
/********************************************************************************F*/
uint64_t ds_timeinsecs(void)
{
    return(NetTime());
}

/*F********************************************************************************/
/*!
    \Function ds_timezone

    \Description
        This function returns the current system timezone as its offset from GMT in
        seconds. There is no direct equivilent function in the standard C libraries.

    \Output int32_t - local timezone offset from GMT in seconds

    \Notes
        The intent is to determine the timezone, so the offset intentionally does 
        not take daylight savings into account.  
        Do not use for GMT time determination.

    \Version 11/06/2002 (jbrookes)
*/
/********************************************************************************F*/
int32_t ds_timezone(void)
{
    time_t iGmt, iLoc;
    static int32_t iZone = -1;

    // just calc the timezone one time
    if (iZone == -1)
    {
        time_t uTime = time(0);
        struct tm *pTm, TmTime;

        // convert to gmt time
        pTm = gmtime(&uTime);
        iGmt = (uint32_t)mktime(pTm);

        // convert to local time
        pTm = ds_localtime(&TmTime,(uint32_t)uTime);
        iLoc = (uint32_t)mktime(pTm);

        // calculate timezone difference
        iZone = (int32_t)(iLoc - iGmt);
    }

    // return the timezone offset in seconds
    return(iZone);
}

/*F********************************************************************************/
/*!
    \Function ds_localtime

    \Description
        This converts the input GMT time to the local time as specified by the
        system clock.  This function follows the re-entrant localtime_r function
        signature.

    \Input *pTm         - storage for localtime output
    \Input elap         - GMT time

    \Output
        struct tm *     - pointer to localtime result

    \Version 04/23/2008 (jbrookes)
*/
/********************************************************************************F*/
struct tm *ds_localtime(struct tm *pTm, uint64_t elap)
{
    return(NetLocalTime(pTm, elap));
}

/*F********************************************************************************/
/*!
    \Function ds_secstotime

    \Description
        Convert elapsed seconds to discrete time components. This is essentially a
        ds_localtime() replacement with better syntax that is available on all platforms.

    \Input *pTm  - target component record
    \Input elap  - epoch time input

    \Output struct tm * - returns tm if successful, NULL if failed

    \Version 01/23/2000 (gschaefer)
*/
/********************************************************************************F*/
struct tm *ds_secstotime(struct tm *pTm, uint64_t elap)
{
    int32_t year, leap, next, days, secs;
    const int32_t *mon;

    // table to find days per month
    static const int32_t dayspermonth[24] = {
        31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, // leap
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31  // norm
    };
    // table to find day of the week
    static const int32_t wday[] = {
        0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4
    };

    // divide out secs within day and days
    // (we ignore leap-seconds cause it requires tables and nasty stuff)
    days = (int32_t)(elap / (24*60*60));
    secs = (int32_t)(elap % (24*60*60));

    // store the time info
    pTm->tm_sec = secs % 60;
    secs /= 60;
    pTm->tm_min = secs % 60;
    secs /= 60;
    pTm->tm_hour = secs;

    // determine what year we are in
    for (year = 1970;; year = next) {
        // calc the length of the year in days
        leap = (((year & 3) == 0) && (((year % 100 != 0) || ((year % 400) == 0))) ? 366 : 365);
        // see if date is within this year
        if (days < leap)
            break;

        // estimate target year assuming every year is a leap year
        // (this may underestimate the year, but will never overestimate)
        next = year + (days / 366);

        // make sure year got changed and force if it did not
        /// (can happen on dec 31 of non-leap year)
        if (next == year)
            ++next;

        // subtract out normal days/year
        days -= (next - year) * 365;
        // add in leap years from previous guess
        days += ((year-1)/4 - (year-1)/100 + (year-1)/400);
        // subtract out leap years since new guess
        days -= ((next-1)/4 - (next-1)/100 + (next-1)/400);
    }

    // save the year and day within year
    pTm->tm_year = year - 1900;
    pTm->tm_yday = days;
    // calc the month
    mon = dayspermonth + 12*(leap&1);
    for (pTm->tm_mon = 0; days >= *mon; pTm->tm_mon += 1)
        days -= *mon++;
    // save the days
    pTm->tm_mday = days + 1;
    // calculate weekday using Sakamoto's algorithm, adjusted for m=[0...11]
    year -= pTm->tm_mon < 2;
    pTm->tm_wday = (year + year/4 - year/100 + year/400 + wday[pTm->tm_mon] + pTm->tm_mday) % 7;
    // clear dst
    pTm->tm_isdst = 0;

    // return pointer to argument to make it closer to ds_localtime()
    return(pTm);
}

/*F********************************************************************************/
/*!
    \Function ds_timetosecs

    \Description
        Convert discrete components to elapsed time.

    \Input *pTm      - source component record

    \Output
        uint32_t    - zero=failure, else epoch time

    \Version 01/23/2000 (gschaefer)
*/
/********************************************************************************F*/
uint64_t ds_timetosecs(const struct tm *pTm)
{
    uint64_t min, max, mid;
    struct tm cmp;
    int32_t res;

    /* do a binary search using ds_secstotime to encode prospective time_t values.
       though iterative, it requires a max of 32 iterations which is actually pretty
       good considering the complexity of the calculation (this allows all the
       time nastiness to remain in a single function). */

    /* perform binary search (set max to 1/1/3000 to reduce 64-bit search space
       and prevent overflow in 32bit days/secs calculations in ds_secstotime */
    for (min = 0, mid = 0, res = 0, max = 32503680000; min <= max; )
    {
        /* test at midpoint -- since these are large unsigned values, they can overflow
           in the (min+max)/2 case hense the individual divide and lost bit recovery */
        mid = (min/2)+(max/2)+(min&max&1);
        // do the time conversion
        ds_secstotime(&cmp, mid);
        // do the compare
        if ((res = (cmp.tm_year - pTm->tm_year)) == 0) {
            if ((res = (cmp.tm_mon - pTm->tm_mon)) == 0) {
                if ((res = (cmp.tm_mday - pTm->tm_mday)) == 0) {
                    if ((res = (cmp.tm_hour - pTm->tm_hour)) == 0) {
                        if ((res = (cmp.tm_min - pTm->tm_min)) == 0) {
                            if ((res = cmp.tm_sec - pTm->tm_sec) == 0) {
                                // got an exact match!
                                break;
                            }
                        }
                    }
                }
            }
        }

        // force break once min/max converge (cannot do this within for condition as res will not be setup correctly)
        if (min == max)
            break;

        // narrow the search range
        if (res > 0)
            max = mid-1;
        else
            min = mid+1;
    }

    // return converted time or zero if failed
    return((res == 0) ? mid : 0);
}

/*F********************************************************************************/
/*!
    \Function ds_plattimetotime

    \Description
        This converts the input platform-specific time data structure to the
        generic time data structure.

    \Input *pTm         - generic time data structure to be filled by the function
    \Input *pPlatTime   - pointer to the platform-specific data structure

    \Output
        struct tm *     - NULL=failure; else pointer to user-provided generic time data structure

    \Notes
        See NetPlattimeToTime() for input data format

    \Version 05/08/2010 (mclouatre)
*/
/********************************************************************************F*/
struct tm *ds_plattimetotime(struct tm *pTm, void *pPlatTime)
{
    return(NetPlattimeToTime(pTm, pPlatTime));
}

/*F********************************************************************************/
/*!
    \Function ds_plattimetotimems
 
    \Description
        This function retrieves the current date time and fills in the
        generic time data structure prrovided. It has the option of returning millisecond
        which is not part of the generic time data structure

    \Input *pTm         - generic time data structure to be filled by the function
    \Input *pImSec      - output param for milisecond to be filled by the function (optional can be NULL)

    \Output
        struct tm *     - NULL=failure; else pointer to user-provided generic time data structure

    \Version 09/16/2014 (tcho)
*/
/********************************************************************************F*/
struct tm *ds_plattimetotimems(struct tm *pTm , int32_t *pImSec)
{
    return(NetPlattimeToTimeMs(pTm, pImSec));
}

/*F********************************************************************************/
/*!
    \Function ds_timetostr

    \Description
        Converts a date formatted in a number of common Unix and Internet formats
        and convert to a struct tm.

    \Input *pTm       - input time stucture to be converted to string
    \Input eConvType  - user-selected conversion type
    \Input bLocalTime - whether input time is local time or UTC 0 offset time.
    \Input *pStrBuf   - user-provided buffer to be filled with datetime string
    \Input iBufSize   - size of output buffer (must be at least 18 bytes to receive null-terminated yyyy-MM-ddTHH:mm:ssZ

    \Output
        char *        - zero=failure, else epoch time

    \Version 07/12/2012 (jbrookes)
*/
/********************************************************************************F*/
char *ds_timetostr(const struct tm *pTm, TimeToStringConversionTypeE eConvType, uint8_t bLocalTime, char *pStrBuf, int32_t iBufSize)
{
    switch(eConvType)
    {
        case TIMETOSTRING_CONVERSION_ISO_8601:
            ds_snzprintf(pStrBuf, iBufSize, "%04d-%02d-%02dT%02d:%02d:%02d%s",
                pTm->tm_year+1900, pTm->tm_mon+1, pTm->tm_mday,
                pTm->tm_hour, pTm->tm_min, pTm->tm_sec,
                !bLocalTime ? "Z" : "");
            break;
        case TIMETOSTRING_CONVERSION_ISO_8601_BASIC:
            ds_snzprintf(pStrBuf, iBufSize, "%04d%02d%02dT%02d%02d%02d%s",
                pTm->tm_year+1900, pTm->tm_mon+1, pTm->tm_mday,
                pTm->tm_hour, pTm->tm_min, pTm->tm_sec,
                !bLocalTime ? "Z" : "");
            break;
        case TIMETOSTRING_CONVERSION_RFC_0822: // e.g.  Wed, 15 Nov 1995 04:58:08 GMT
        {
            uint32_t tm_wday = ((unsigned)pTm->tm_wday < 7) ? pTm->tm_wday : 7;
            uint32_t tm_mon = ((unsigned)pTm->tm_mon < 12) ? pTm->tm_mon : 12;
            ds_snzprintf(pStrBuf, iBufSize, "%s, %2d %s %4d %02d:%02d:%02d GMT", _PlatTime_strWday[tm_wday], pTm->tm_mday,
                _PlatTime_strMonth[tm_mon], pTm->tm_year+1900, pTm->tm_hour, pTm->tm_min, pTm->tm_sec);
            break;
        }
        default:
            // unsupported conversion type
            pStrBuf = NULL;
            break;
    }

    return(pStrBuf);
}

/*F********************************************************************************/
/*!
    \Function ds_secstostr

    \Description
        Converts a datetime in epoch format to string

    \Input elap         - epoch time to convert to string
    \Input eConvType    - user-selected conversion type
    \Input bLocalTime   - whether input time is local time or UTC 0 offset time.
    \Input *pStrBuf     - user-provided buffer to be filled with datetime string
    \Input iBufSize     - size of output buffer (must be at least 18 bytes to receive null-terminated yyyy-MM-ddTHH:mm:ssZ

    \Output
        char *          - zero=failure, else epoch time

    \Version 02/26/2014 (jbrookes)
*/
/********************************************************************************F*/
char *ds_secstostr(uint64_t elap, TimeToStringConversionTypeE eConvType, uint8_t bLocalTime, char *pStrBuf, int32_t iBufSize)
{
    struct tm TmTime;
    return(ds_timetostr(ds_secstotime(&TmTime, elap), eConvType, bLocalTime, pStrBuf, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function ds_strtotime

    \Description
        Converts a date formatted in a number of common Unix and Internet formats
        and convert to a struct tm.

    \Input *pStr - textual date string

    \Output uint32_t - zero=failure, else epoch time

    \Notes
        \verbatim
        The following time formats are supported:
            Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
            Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format

        The following format is mentioned as needing to be readable by HTTP
        but is not currently supported by this function:
            Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
        \endverbatim

    \Version 11/01/2002 (gschaefer)
*/
/********************************************************************************F*/
uint64_t ds_strtotime(const char *pStr)
{
    int32_t i;
    const char *s;
    struct tm tm;

    // reset the fields
    ds_memset(&tm, -1, sizeof(tm));

    // skip past any white space
    while ((*pStr != 0) && (*pStr <= ' '))
    {
        pStr++;
    }

    // see if starts with day of week (RFC 822/1123, asctime)
    for (i = 0; (s=_PlatTime_strWday[i]) != 0; ++i)
    {
        if ((pStr[0] == s[0]) && (pStr[1] == s[1]) && (pStr[2] == s[2]))
        {
            tm.tm_wday = i;
            // skip name of weekday
            while ((*pStr != ',') && (*pStr != ' ') && (*pStr != 0))
                ++pStr;
            // skip the divider
            while ((*pStr == ',') || (*pStr == ' '))
                ++pStr;
            break;
        }
    }

    // check for mmm dd (asctime)
    if ((*pStr < '0') || (*pStr > '9'))
    {
        for (i = 0; (s=_PlatTime_strMonth[i]) != 0; ++i)
        {
            // look for a match
            if ((pStr[0] != s[0]) || (pStr[1] != s[1]) || (pStr[2] != s[2]))
                continue;
            // found the month
            tm.tm_mon = i;
            // skip to the digits
            while ((*pStr != 0) && ((*pStr < '0') || (*pStr > '9')))
                ++pStr;
            // get the day of month
            for (i = 0; ((*pStr >= '0') && (*pStr <= '9')); ++pStr)
                i = (i * 10) + (*pStr & 15);
            if (i > 0)
                tm.tm_mday = i;
            break;
        }
    }

    // check for dd mmm (RFC 822/1123)
    if ((tm.tm_mon < 0) && (pStr[0] >= '0') && (pStr[0] <= '9') &&
        ((pStr[1] > '@') || (pStr[2] > '@') || (pStr[3] > '@')))
    {
        // get the day
        for (i = 0; ((*pStr >= '0') && (*pStr <= '9')); ++pStr)
            i = (i * 10) + (*pStr & 15);
        tm.tm_mday = i;
        while (*pStr < '@')
            ++pStr;
        // get the month
        for (i = 0; (s=_PlatTime_strMonth[i]) != 0; ++i)
        {
            // look for a match
            if ((pStr[0] != s[0]) || (pStr[1] != s[1]) || (pStr[2] != s[2]))
                continue;
            tm.tm_mon = i;
            while ((*pStr != 0) && (*pStr != ' '))
                ++pStr;
            break;
        }
    }

    // check for xx/xx or xx/xx/xx (???)
    if ((*pStr >= '0') && (*pStr <= '9') && (tm.tm_mon < 0))
    {
        // get the month
        for (i = 0; ((*pStr >= '0') && (*pStr <= '9')); ++pStr)
            i = (i * 10) + (*pStr & 15);
        tm.tm_mon = i - 1;
        if (*pStr != 0)
            ++pStr;
        // get the day
        for (i = 0; ((*pStr >= '0') && (*pStr <= '9')); ++pStr)
            i = (i * 10) + (*pStr & 15);
        tm.tm_mday = i;
        if (*pStr != 0)
            ++pStr;
    }

    // check for year (RFC 822/1123)
    while ((*pStr != 0) && ((*pStr < '0') || (*pStr > '9')))
        ++pStr;
    // see if the year is here
    if ((pStr[0] >= '0') && (pStr[0] <= '9') && (pStr[1] != ':') && (pStr[2] != ':'))
    {
        for (i = 0; ((*pStr >= '0') && (*pStr <= '9')); ++pStr)
            i = (i * 10) + (*pStr & 15);
        if (i > 999)
            tm.tm_year = i;
        else if (i >= 50)
            tm.tm_year = 1900+i;
        else
            tm.tm_year = 2000+i;
        // find next digit sequence
        while ((*pStr != 0) && ((*pStr < '0') || (*pStr > '9')))
            ++pStr;
    }

    // save the hour (RFC 822/1123, asctime)
    if ((*pStr >= '0') && (*pStr <= '9'))
    {
        i = (*pStr++ & 15);
        if ((*pStr >= '0') && (*pStr <= '9'))
            i = (i * 10) + (*pStr++ & 15);
        tm.tm_hour = i;
        if (*pStr == ':')
            ++pStr;
    }

    // save the minute (RFC 822/1123, asctime)
    if ((*pStr >= '0') && (*pStr <= '9'))
    {
        i = (*pStr++ & 15);
        if ((*pStr >= '0') && (*pStr <= '9'))
            i = (i * 10) + (*pStr++ & 15);
        tm.tm_min = i;
        if (*pStr == ':')
            ++pStr;
    }

    // save the second (if present) (RFC 822/1123, asctime)
    if ((*pStr >= '0') && (*pStr <= '9'))
    {
        i = (*pStr++ & 15);
        if ((*pStr >= '0') && (*pStr <= '9'))
            i = (i * 10) + (*pStr++ & 15);
        tm.tm_sec = i;
    }

    // see if year is still remaining (asctime)
    if (tm.tm_year < 0)
    {
        // see if any digits left
        while ((*pStr != 0) && ((*pStr < '0') || (*pStr > '9')))
            ++pStr;
        for (i = 0; ((*pStr >= '0') && (*pStr <= '9')); ++pStr)
            i = (i * 10) + (*pStr & 15);
        if (i > 999)
            tm.tm_year = i;
    }

    // make year relative to 1900 (really dumb)
    if (tm.tm_year > 1900)
        tm.tm_year -= 1900;

    // convert from struct tm to uint32_t and return to caller
    return(ds_timetosecs(&tm));
}

/*F********************************************************************************/
/*!
    \Function ds_strtotime2

    \Description
        Converts a date formatted in a number of common Unix and Internet formats
        and convert to a struct tm.

    \Input *pStr        - textual date string
    \Input eConvType    - time format to convert from

    \Output
        uint32_t        - zero=failure, else epoch time

    \Notes
        For supported conversion types other than ISO_8601, see documentation
        for ds_strtotime().

    \Version 12/13/2012 (jbrookes)
*/
/********************************************************************************F*/
uint64_t ds_strtotime2(const char *pStr, TimeToStringConversionTypeE eConvType)
{
    uint64_t uTime = 0;
    struct tm tm;

    if (eConvType == TIMETOSTRING_CONVERSION_ISO_8601)
    {
        // format: YYYY-MM-DDTHH:MM:SSZ
        if (strlen(pStr) < 19) // 'Z' is optional and ignored
        {
            return(0);
        }
        // read the date/time
        pStr = _ds_strtoint(pStr, &tm.tm_year, 4) + 1;  // get the year, skip hyphen
        pStr = _ds_strtoint(pStr, &tm.tm_mon, 2) + 1;   // get the month, skip hyphen
        pStr = _ds_strtoint(pStr, &tm.tm_mday, 2) + 1;  // get the day, skip 'T'
        pStr = _ds_strtoint(pStr, &tm.tm_hour, 2) + 1;  // get the hour, skip ':'
        pStr = _ds_strtoint(pStr, &tm.tm_min, 2) + 1;   // get the minute, skip ':'
        _ds_strtoint(pStr, &tm.tm_sec, 2);              // get the second
        // adjust year and month
        tm.tm_year -= 1900;
        tm.tm_mon -= 1;
        // convert from struct tm to uint32_t and return to caller
        uTime = ds_timetosecs(&tm);
    }
    else if ((eConvType == TIMETOSTRING_CONVERSION_ASN1_UTCTIME) || (eConvType == TIMETOSTRING_CONVERSION_ASN1_GENTIME))
    {
        /* GeneralizedTime: .fff is optional
            Local Time: YYYYMMDDHHMMSS.fff
            UTC Time: YYYYMMDDHHMMSS.fffZ
            Difference Time: YYYYMMDDHHMMSS.fff+-HHMM
           UTCTime: like GeneralizedTime, but accuracy is to one minute or one second (no .fff, SS is optional, Z is implicit and not included).
            Local Time: YYYYMMDDHHMMSS
            UTC Time: YYMMDDHHMMSS
            Difference Time: YYYYMMDDHHMMSS+-HHMM
           Note: Fractional time, difference time, and UTC character, if present, are ignored */
        if (eConvType == TIMETOSTRING_CONVERSION_ASN1_UTCTIME)
        {
            pStr = _ds_strtoint(pStr, &tm.tm_year, 2);
            tm.tm_year += (tm.tm_year < 70) ? 2000 : 1900; // 2-year UTC time represents from 1970 to 2069
        }
        else
        {
            pStr = _ds_strtoint(pStr, &tm.tm_year, 4);
        }
        pStr = _ds_strtoint(pStr, &tm.tm_mon, 2);
        pStr = _ds_strtoint(pStr, &tm.tm_mday, 2);
        pStr = _ds_strtoint(pStr, &tm.tm_hour, 2);
        pStr = _ds_strtoint(pStr, &tm.tm_min, 2);
        _ds_strtoint(pStr, &tm.tm_sec, 2);
        // adjust year and month
        tm.tm_year -= 1900;
        tm.tm_mon -= 1;
        // convert from struct tm to uint32_t and return to caller
        uTime = ds_timetosecs(&tm);
    }
    else
    {
        uTime = ds_strtotime(pStr);
    }
    return(uTime);
}

