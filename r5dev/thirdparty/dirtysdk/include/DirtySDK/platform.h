/*H********************************************************************************/
/*!
    \File platform.h

    \Description
        A platform wide header that performs environment identification and replaces
        some standard lib functions with "safe" alternatives (such an snprintf that
        always terminates the buffer).

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 01/25/2005 (gschaefer) First Version
*/
/********************************************************************************H*/

#ifndef _platform_h
#define _platform_h

/*!
\Moduledef Platform Platform
\Modulemember DirtySock
*/
//@{

/*** Include files ****************************************************************/

#include <EABase/eabase.h>

/*** Defines **********************************************************************/

// identify platform if not yet known
#if defined(DIRTYCODE_PC) || defined(DIRTYCODE_LINUX)
// do nothing
#elif defined(EA_PLATFORM_XBSX)
    #define DIRTYCODE_GDK           1
    #define DIRTYCODE_XBSX          1
#elif defined(EA_PLATFORM_XBOX_GDK)
    #define DIRTYCODE_GDK           1
    #define DIRTYCODE_XBOXONE       1
#elif defined(EA_PLATFORM_CAPILANO)
    #define DIRTYCODE_XBOXONE       1
#elif defined(EA_PLATFORM_MICROSOFT)
    #define DIRTYCODE_PC            1
#elif defined(EA_PLATFORM_PS4)
    #define DIRTYCODE_PS4           1
#elif defined(EA_PLATFORM_PS5)
   #define DIRTYCODE_PS5            1
   #define DIRTYCODE_PS4            1
#elif defined(EA_PLATFORM_IPHONE) || defined(EA_PLATFORM_IPHONE_SIMULATOR)
    #define DIRTYCODE_APPLEIOS      1
#elif defined(EA_PLATFORM_OSX)
    #define DIRTYCODE_APPLEOSX      1
#elif defined(EA_PLATFORM_ANDROID)
    #define DIRTYCODE_ANDROID       1
#elif defined(EA_PLATFORM_STADIA)
    #define DIRTYCODE_LINUX         1
    #define DIRTYCODE_STADIA        1
#elif defined(EA_PLATFORM_LINUX)
    #define DIRTYCODE_LINUX         1
#elif defined(EA_PLATFORM_NX)
    #define DIRTYCODE_NX            1
#endif

// define platform name
#if defined(DIRTYCODE_ANDROID)
#define DIRTYCODE_PLATNAME "Android"
#define DIRTYCODE_PLATNAME_SHORT "android"
#elif defined(DIRTYCODE_APPLEIOS)
#define DIRTYCODE_PLATNAME "AppleIOS"
#define DIRTYCODE_PLATNAME_SHORT "ios"
#elif defined(DIRTYCODE_APPLEOSX)
#define DIRTYCODE_PLATNAME "AppleOSX"
#define DIRTYCODE_PLATNAME_SHORT "osx"
#elif defined(DIRTYCODE_XBSX)
#define DIRTYCODE_PLATNAME "XboxSeriesX"
#define DIRTYCODE_PLATNAME_SHORT "xbsx"
#elif defined(DIRTYCODE_XBOXONE)
#define DIRTYCODE_PLATNAME "XboxOne"
#define DIRTYCODE_PLATNAME_SHORT "xone"
#elif defined(DIRTYCODE_PS5)
#define DIRTYCODE_PLATNAME "PlayStation5"
#define DIRTYCODE_PLATNAME_SHORT "ps5"
#elif defined(DIRTYCODE_PS4)
#define DIRTYCODE_PLATNAME "PlayStation4"
#define DIRTYCODE_PLATNAME_SHORT "ps4"
#elif defined(DIRTYCODE_STADIA)
#define DIRTYCODE_PLATNAME "Stadia"
#define DIRTYCODE_PLATNAME_SHORT "stadia"
#elif defined(DIRTYCODE_LINUX)
#define DIRTYCODE_PLATNAME "Linux"
#define DIRTYCODE_PLATNAME_SHORT "linux"
#elif defined(DIRTYCODE_PC)
#define DIRTYCODE_PLATNAME "Windows"
#define DIRTYCODE_PLATNAME_SHORT "pc"
#elif defined(DIRTYCODE_NX)
#define DIRTYCODE_PLATNAME "Switch"
#define DIRTYCODE_PLATNAME_SHORT "nx"
#else
#error The platform was not predefined and could not be auto-determined!
#endif

//For Building as a dll
#if defined(EA_DLL)
 #ifndef DIRTYCODE_DLL
    #define DIRTYCODE_DLL
 #endif
 #ifndef DIRTYCODE_API
    #define DIRTYCODE_API __declspec(dllimport)
 #endif
#else
 #define DIRTYCODE_API
#endif

// define 32-bit or 64-bit pointers
#if (EA_PLATFORM_PTR_SIZE == 8)
 #define DIRTYCODE_64BITPTR (1)
#else
 #define DIRTYCODE_64BITPTR (0)
#endif

// we need va_list to be a universal type
#include <stdarg.h>
#include <wchar.h>
#include <time.h>

/*** Defines **********************************************************************/

// force our definition of TRUE/FALSE
#ifdef  TRUE
#undef  TRUE
#undef  FALSE
#endif

#define FALSE (0)
#define TRUE  (1)

// map common debug code definitions to our debug code definition
#ifndef DIRTYCODE_DEBUG
 #if defined(EA_DEBUG)
  #define DIRTYCODE_DEBUG (1)
 #else
  #define DIRTYCODE_DEBUG (0)
 #endif
#endif

/*** Macros ***********************************************************************/

//! min of _x and _y
#define DS_MIN(_x, _y)              (((_x) < (_y)) ? (_x) : (_y))

//! max of _x and _y
#define DS_MAX(_x, _y)              (((_x) > (_y)) ? (_x) : (_y))

//! abs of _x
#define DS_ABS(_x)                  (((_x) < 0) ? -(_x) : (_x))

//! clamp _x between _min and _max; if _low < _high result is undefined
#define DS_CLAMP(_x, _min, _max)    (((_x) > (_max)) ? (_max) : (((_x) < (_min)) ? (_min) : (_x)))

/*** Type Definitions *************************************************************/

//! time-to-string conversion type
typedef enum TimeToStringConversionTypeE
{
    TIMETOSTRING_CONVERSION_ISO_8601,       //!< ISO 8601 standard:  yyyy-MM-ddTHH:mmZ where Z means 0 UTC offset, and no Z means local time zone
    TIMETOSTRING_CONVERSION_ISO_8601_BASIC, //!< ISO 8601 basic format (no hyphens or colon) : https://en.wikipedia.org/wiki/ISO_8601#cite_ref-15
    TIMETOSTRING_CONVERSION_RFC_0822,       //!< RFC 0822 format, updated by RFC1123: day, dd mon yyyy hh:mm:ss zzz
    TIMETOSTRING_CONVERSION_ASN1_UTCTIME,   //!< ASN.1 UTCTime format
    TIMETOSTRING_CONVERSION_ASN1_GENTIME,   //!< ASN.1 GeneralizedTime format
    TIMETOSTRING_CONVERSION_UNKNOWN         //!< unknown source format type; try to auto-determine if possible
} TimeToStringConversionTypeE;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// replacement time functions
DIRTYCODE_API uint64_t ds_timeinsecs(void);
DIRTYCODE_API int32_t ds_timezone(void);
DIRTYCODE_API struct tm *ds_localtime(struct tm *pTm, uint64_t elap);
DIRTYCODE_API struct tm *ds_secstotime(struct tm *pTm, uint64_t elap);
DIRTYCODE_API uint64_t ds_timetosecs(const struct tm *pTm);
DIRTYCODE_API struct tm *ds_plattimetotime(struct tm *pTm, void *pPlatTime);
DIRTYCODE_API struct tm *ds_plattimetotimems(struct tm *pTm, int32_t *pImSec);
DIRTYCODE_API char *ds_timetostr(const struct tm *pTm, TimeToStringConversionTypeE eConvType, uint8_t bLocalTime, char *pStr, int32_t iBufSize);
DIRTYCODE_API char *ds_secstostr(uint64_t elap, TimeToStringConversionTypeE eConvType, uint8_t bLocalTime, char *pStr, int32_t iBufSize);
DIRTYCODE_API uint64_t ds_strtotime(const char *str);
DIRTYCODE_API uint64_t ds_strtotime2(const char *pStr, TimeToStringConversionTypeE eConvType);

// replacement string functions
DIRTYCODE_API int32_t ds_strnlen(const char *pBuffer, int32_t iLength);
DIRTYCODE_API int32_t ds_vsnprintf(char *pBuffer, int32_t iLength, const char *pFormat, va_list Args);
DIRTYCODE_API int32_t ds_vsnzprintf(char *pBuffer, int32_t iLength, const char *pFormat, va_list Args);
DIRTYCODE_API int32_t ds_snzprintf(char *pBuffer, int32_t iLength, const char *pFormat, ...);
DIRTYCODE_API char *ds_strnzcpy(char *pDest, const char *pSource, int32_t iCount);
DIRTYCODE_API int32_t ds_strnzcat(char *pDst, const char *pSrc, int32_t iDstLen);
DIRTYCODE_API int32_t ds_stricmp(const char *pString1, const char *pString2);
DIRTYCODE_API int32_t ds_strnicmp(const char *pString1, const char *pString2, uint32_t uCount);
DIRTYCODE_API char *ds_stristr(const char *pHaystack, const char *pNeedle);
DIRTYCODE_API char* ds_strtok_r(char *pStr, const char *pDelim, char **ppSavePtr);

#if defined(_WIN32) || defined(_WIN64)
 #define ds_strtoll(_buf, _outp, _radix) _strtoi64(_buf, _outp, _radix)
 #define ds_strtoull(_buf, _outp, _radix) _strtoui64(_buf, _outp, _radix)
#else
 #define ds_strtoll(_buf, _outp, _radix) strtoll(_buf, _outp, _radix)
 #define ds_strtoull(_buf, _outp, _radix) strtoull(_buf, _outp, _radix)
#endif

// 'original' string functions
DIRTYCODE_API char *ds_fmtoctstring(char *pOutput, int32_t iOutLen, const uint8_t *pInput, int32_t iInpLen);
DIRTYCODE_API char *ds_fmtoctstring_lc(char *pOutput, int32_t iOutLen, const uint8_t *pInput, int32_t iInpLen);
DIRTYCODE_API char *ds_strtolower(char *pString);
DIRTYCODE_API char *ds_strtoupper(char *pString);
DIRTYCODE_API char *ds_strtrim(char *pString);
DIRTYCODE_API int32_t ds_strlistinsert(char *pStrList, int32_t iListBufLen, const char *pString, char cTerm);
DIRTYCODE_API int32_t ds_strsubzcpy(char *pDst, int32_t iDstLen, const char *pSrc, int32_t iSrcLen);
DIRTYCODE_API int32_t ds_strsubzcat(char *pDst, int32_t iDstLen, const char *pSrc, int32_t iSrcLen);
DIRTYCODE_API int32_t ds_strcmpwc(const char *pString1, const char *pStrWild);
DIRTYCODE_API int32_t ds_stricmpwc(const char *pString1, const char *pStrWild);
DIRTYCODE_API int32_t ds_strsplit(const char *pSrc, char cDelimiter, char *pDst, int32_t iDstSize, const char **pNewSrc);

// mem functions
DIRTYCODE_API void ds_memcpy(void *pDst, const void *pSrc, int32_t iDstLen);
DIRTYCODE_API void ds_memcpy_s(void *pDst, int32_t iDstLen, const void *pSrc, int32_t iSrcLen);
DIRTYCODE_API void ds_memclr(void *pMem, int32_t iCount);
DIRTYCODE_API void ds_memset(void *pMem, int32_t iValue, int32_t iCount);

#ifdef __cplusplus
}
#endif

//@}

#endif // _platform_h
