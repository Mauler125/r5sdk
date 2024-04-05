/*H*************************************************************************************/
/*!
    \File jsonformat.h

    \Description
        This module formats simple Json, in a linear fashion, using a character buffer
        that the client provides.

    \Notes
        References:
            JSON RFC: http://tools.ietf.org/html/rfc4627

    \Copyright
        Copyright (c) Electronic Arts 2012.

    \Version 12/11/2012 (jbrookes) First Version
*/
/*************************************************************************************H*/

#ifndef _jsonformat_h
#define _jsonformat_h

/*!
\Moduledef JsonFormat JsonFormat
\Modulemember Util
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

#define JSON_ERR_NONE           (0)     //!< no error
#define JSON_ERR_FULL           (-1)    //!< buffer is full, no space to add attribute or element.
#define JSON_ERR_UNINIT         (-2)    //!< did not call JsonInit() prior to writing.
#define JSON_ERR_NOT_OPEN       (-3)    //!< attempt to set elem or attrib, but, no tag opened.
#define JSON_ERR_ATTR_POSITION  (-4)    //!< attempt to set an attrib, but, child element already added to tag.
#define JSON_ERR_INVALID_PARAM  (-5)    //!< invalid parameter passed to function

// Encoding control flags
#define JSON_FL_WHITESPACE      (1)     //!< include formatting whitespace

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// init the API -- pass in the character buffer. MUST be called first.
DIRTYCODE_API void JsonInit(char *pBuffer, int32_t iBufLen, uint8_t uFlags);

// notify the API that the buf size was increased
DIRTYCODE_API void JsonBufSizeIncrease(char *pBuffer, int32_t iNewBufLen);

// finish JSON output to this buffer; use returned pointer ref
DIRTYCODE_API char *JsonFinish(char *pBuffer);

// start an object
DIRTYCODE_API int32_t JsonObjectStart(char *pBuffer, const char *pName);

// end the current object -- must have an outstanding open object
DIRTYCODE_API int32_t JsonObjectEnd(char *pBuffer);

// start an array
DIRTYCODE_API int32_t JsonArrayStart(char *pBuffer, const char *pName);

// end the current array -- must have an outstanding open array
DIRTYCODE_API int32_t JsonArrayEnd(char *pBuffer);

// add a string element
DIRTYCODE_API int32_t JsonAddStr(char *pBuffer, const char *pElemName, const char *pValue);

// add a number
DIRTYCODE_API int32_t JsonAddNum(char *pBuffer, const char *pElemName, const char *pFormatSpec, float fValue);

// add an integer element
DIRTYCODE_API int32_t JsonAddInt(char *pBuffer, const char *pElemName, int64_t iValue);

// add a date (will be encoded as a string; use JsonAddInteger for integer encoding)
DIRTYCODE_API int32_t JsonAddDate(char *pBuffer, const char *pElemName, uint32_t uEpochDate);

// build a JSON output string from a formatted pattern (like vprintf) $$TODO
#ifdef va_start
DIRTYCODE_API char *JsonFormatVPrintf(char *pJsonBuff, int32_t iBufLen, const char *pFormat, va_list pFmtArgs);
#endif

// build a JSON output string from a formatted pattern (like printf) $$TODO
DIRTYCODE_API char *JsonFormatPrintf(char *pJsonBuff, int32_t iBufLen, const char *pFormat, ...);

#ifdef __cplusplus
}
#endif

//@}

#endif // _jsonformat_h
