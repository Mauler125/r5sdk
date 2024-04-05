/*H*************************************************************************************************/
/*!

    \File    xmlparse.h

    \Description
    \verbatim
        This is a simple XML parser for use in controlled situations. It should not
        be used with arbitrary XML from uncontrolled hosts (i.e., test its parsing against
        a particular host before using it). This only implement a small subset of full
        XML and while suitable for many applications, it is easily confused by badly
        formed XML or by some of the ligitimate but convoluated XML conventions.

        In particular, this parser cannot handle degenerate empty elements (i.e., <BR><BR>
        will confuse it). Empty elements must be of proper XML form <BR/><BR/>. Only the
        predefined entity types (&lt; &gt; &amp; &apos; &quot) are supported. This module
        does not support unicode values.
    \endverbatim

    \Notes
        None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002.  ALL RIGHTS RESERVED.

    \Version    1.0        01/30/2002 (GWS) First Version

*/
/*************************************************************************************************H*/

#ifndef _xmlparse_h
#define _xmlparse_h

/*!
\Moduledef XmlParse XmlParse
\Modulemember Util
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

// debug printing routines
#if DIRTYCODE_LOGGING
 #define XmlPrintFmt(_x) XmlPrintFmtCode _x
#else
 #define XmlPrintFmt(_x) { }
#endif

/*** Type Definitions ******************************************************************/

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// skip to the next xml entity (used to enumerate lists)
DIRTYCODE_API const char *XmlSkip(const char *strXml);

// find an entity within an xml document
DIRTYCODE_API const char *XmlFind(const char *strXml, const char *strName);

// skip to next entity with same name as current
DIRTYCODE_API const char *XmlNext(const char *strXml);

// step to next xml element
DIRTYCODE_API const char *XmlStep(const char *strXml);

// determines if the xml entity pointed to is complete
DIRTYCODE_API uint32_t XmlComplete(const char *pXml);

// return entity contents as a string
DIRTYCODE_API int32_t XmlContentGetString(const char *strXml, char *strBuffer, int32_t iLength, const char *strDefault);

// return entity contents as an integer
DIRTYCODE_API int32_t XmlContentGetInteger(const char *strXml, int32_t iDefault);

// return entity contents as a 64-bit integer
DIRTYCODE_API int64_t XmlContentGetInteger64(const char *strXml, int64_t iDefault);

// return entity contents as a token (a packed sequence of characters)
DIRTYCODE_API int32_t XmlContentGetToken(const char *pXml, int32_t iDefault);

// return epoch seconds for a date
DIRTYCODE_API uint32_t XmlContentGetDate(const char *pXml, uint32_t uDefault);

// parse entity contents as an internet dot-notation address
DIRTYCODE_API int32_t XmlContentGetAddress(const char *pXml, int32_t iDefault);

// return binary encoded data
DIRTYCODE_API int32_t XmlContentGetBinary(const char *pXml, char *pBuffer, int32_t iLength);

// return entity attribute as a string
DIRTYCODE_API int32_t XmlAttribGetString(const char *strXml, const char *strAttrib, char *strBuffer, int32_t iLength, const char *strDefault);

// return entity attribute as an integer
DIRTYCODE_API int32_t XmlAttribGetInteger(const char *strXml, const char *strAttrib, int32_t iDefault);

// return entity attribute as an 64-bit integer
DIRTYCODE_API int64_t XmlAttribGetInteger64(const char *strXml, const char *strAttrib, int64_t iDefault);

// return entity attribute as a token
DIRTYCODE_API int32_t XmlAttribGetToken(const char *pXml, const char *pAttrib, int32_t iDefault);

// return epoch seconds for a date
DIRTYCODE_API uint32_t XmlAttribGetDate(const char *pXml, const char *pAttrib, uint32_t uDefault);

// convert epoch to date components
DIRTYCODE_API int32_t XmlConvEpoch2Date(uint32_t uEpoch, int32_t *pYear, int32_t *pMonth, int32_t *pDay, int32_t *pHour, int32_t *pMinute, int32_t *pSecond);

// debug output of XML reformatted to look 'nicer' - use macro wrapper XmlPrintFmt(), do not call directly
#if DIRTYCODE_LOGGING
DIRTYCODE_API void XmlPrintFmtCode(const char *pXml, const char *pFormat, ...);
#endif

#ifdef __cplusplus
}
#endif

//@}

#endif // _xmlparse_h
