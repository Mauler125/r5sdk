/*H*************************************************************************************************/
/*!

    \File    utf8.h

    \Description
        This module implements routines for converting to and from UTF-8.

    \Notes
        This code only decodes the first three octets of UTF-8, thus it only handles UCS-2 codes,
        not UCS-4 codes.  It also does not handle UTF-16 (and surrogate pairs), and is therefore
        limited to encoding to/decoding from the basic reference plane.

        Helpful references:

            http://www.utf-8.com/                                   - links
            http://www.cis.ohio-state.edu/cgi-bin/rfc/rfc2279.html  - RFC 2279
            http://www.unicode.org/charts/                          - UNICODE character charts
            http://www-106.ibm.com/developerworks/library/utfencodingforms/ - UNICODE primer
            http://www.columbia.edu/kermit/utf8.html                - UTF-8 samples

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2003.  ALL RIGHTS RESERVED.

    \Version    1.0        03/25/03 (JLB) First version.

*/
/*************************************************************************************************H*/

#ifndef _utf8_h
#define _utf8_h

/*!
\Moduledef Utf8 Utf8
\Modulemember Util
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! UTF-8 to 8bit translation table
typedef struct Utf8TransTblT
{
    uint32_t uRangeBegin;
    uint32_t uRangeEnd;
    unsigned char *pCodeTbl;
} Utf8TransTblT;

//! 8bit to UTF-8 translation table
typedef struct Utf8EncodeTblT
{
    uint16_t uCodeTbl[256];
} Utf8EncodeTblT;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// strip non-ASCII characters from a UTF-8 encoded string
DIRTYCODE_API int32_t Utf8Strip(char *pOutStr, int32_t iBufSize, const char *pInStr);

// replace non-ASCII characters in a UTF-8 encoded string with 'cReplace'
DIRTYCODE_API int32_t Utf8Replace(char *pOutStr, int32_t iBufSize, const char *pInStr, char cReplace);

// get code point length of UTF-8 encoded string
DIRTYCODE_API int32_t Utf8StrLen(const char *pStr);

// encode a UCS-2 string to UTF-8
DIRTYCODE_API int32_t Utf8EncodeFromUCS2(char *pOutStr, int32_t iBufLen, const uint16_t *pInStr);

// encode a single UCS-2 "char" to UTF-8 string.
DIRTYCODE_API int32_t Utf8EncodeFromUCS2CodePt(char *pOutPtr, uint16_t uCodePt);

// decode a UTF-8 encoded string into UCS-2
DIRTYCODE_API int32_t Utf8DecodeToUCS2(uint16_t *pOutStr, int32_t iBufLen, const char *pInStr);

// encode the given 8bit input string to UTF-8, based on the input translation table
DIRTYCODE_API int32_t Utf8EncodeFrom8Bit(char *pOutStr, int32_t iBufLen, const char *pInStr, const Utf8EncodeTblT *pEncodeTbl);

// translate the given UTF-8 sequence based on the NULL-terminated array of given tables
DIRTYCODE_API int32_t Utf8TranslateTo8Bit(char *pOutStr, int32_t iBufLen, const char *pInStr, char cReplace, const Utf8TransTblT *pTransTbl);

#ifdef __cplusplus
}
#endif

//@}

#endif // _utf8_h
