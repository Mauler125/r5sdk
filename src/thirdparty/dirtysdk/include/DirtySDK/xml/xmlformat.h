/*H*************************************************************************************/
/*!
    \File xmlformat.h

    \Description
        This module formats simple Xml, in a linear fashion, using a character buffer
        that the client provides.

    \Notes
        \verbatim
        Since this is a simple linear buffer write, without support for a DOM, the following
        rules apply:
            - You cannot set attributes or element values if an element (tag) is not started.
            - If you start an element, (tag), you have to set the attributes before any
              child elements are added.
            - The XmlElemAddXXX calls will create a complete element, complete with start
              and end tags.
            - XmlInit MUST be called on the client buffer, before any api functions are
              called.
            - Once XML output is complete, the client should call XmlFinish().  There is
              some hidden data in the buffer which must be cleared prior to output. Client
              should account for 24 extra bytes in buffer for use by Xml API.

        References:
            1) XML Core: http://www.w3.org/XML/Core/
            2) XML 1.0 4th edition specifications: http://www.w3.org/TR/xml/
        \endverbatim

    \Copyright
        Copyright (c) Electronic Arts 2004-2008.  ALL RIGHTS RESERVED.

    \Version 01/30/2004 (jbertrand) First Version
*/
/*************************************************************************************H*/

#ifndef _xmlformat_h
#define _xmlformat_h

/*!
\Moduledef XmlFormat XmlFormat
\Modulemember Util
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

#define XML_ERR_NONE            0
#define XML_ERR_FULL            -1 //Buffer is full, no space to add attribute or element.
#define XML_ERR_UNINIT          -2 //Did not call XmlInit() prior to writing.
#define XML_ERR_NOT_OPEN        -3 //Attempt to set elem or attrib, but, no tag opened.
#define XML_ERR_ATTR_POSITION   -4 //Attempt to set an attrib, but, child element already added to tag.
#define XML_ERR_INVALID_PARAM   -5 //Invalid parameter passed to function

// Encoding control flags
#define XML_FL_WHITESPACE       1  //!< Include formatting whitespace

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// Init the API -- pass in the character buffer. MUST be called first.
DIRTYCODE_API void XmlInit(char *pBuffer, int32_t iBufLen, uint8_t uFlags);

// Notify the xmlformat API that the buf size was increased
DIRTYCODE_API void XmlBufSizeIncrease(char *pBuffer, int32_t iNewBufLen);

// Finish XML output to this buffer.
DIRTYCODE_API char *XmlFinish(char *pBuffer);

// Start an element to which other elements or attributes may be added.
DIRTYCODE_API int32_t XmlTagStart(char *pBuffer, const char *pName);

// End the current element or tag-- must have an outstanding open tag.
DIRTYCODE_API int32_t XmlTagEnd(char *pBuffer);

// Add a complete, contained text element. Builds start and end tag. Nothing can be appended to this node.
DIRTYCODE_API int32_t XmlElemAddString(char *pBuffer, const char *pElemName, const char *pValue);

// Add a complete, contained integer element. Builds start and end tag. Nothing can be appended to this node.
DIRTYCODE_API int32_t XmlElemAddInt(char *pBuffer, const char *pElemName, int32_t iValue);

// Add a complete, contained decimal element. Format it using formatSpec. Builds start and end tag. Nothing can be appended to this node.
DIRTYCODE_API int32_t XmlElemAddFloat(char *pBuffer, const char *pElemName, const char *pFormatSpec, float fValue);

// Add a complete, contained Date elem, using epoch date. Builds start and end tag. Nothing can be appended to this node.
DIRTYCODE_API int32_t XmlElemAddDate(char *pBuffer, const char *pElemName, uint32_t uEpochDate);

// Set a text attribute for an open element (started tag). Must be called before element text is set.
DIRTYCODE_API int32_t XmlAttrSetString(char *pBuffer, const char *pAttrName, const char *pValue);

// Set a text attribute for an open element (started tag), with no text encoding being performed on the input string. Must be called before element text is set.
DIRTYCODE_API int32_t XmlAttrSetStringRaw(char *pBuffer, const char *pAttr);

// Set an integer attribute for an open element (started tag). Must be called before element text is set.
DIRTYCODE_API int32_t XmlAttrSetInt(char *pBuffer, const char *pAttrName, int32_t iValue);

// Set an IP address attribute (in dot notation)
DIRTYCODE_API int32_t XmlAttrSetAddr(char *pBuffer, const char *pAttrName, uint32_t uAddr);

// Set a decimal attribute for an open element (started tag). Format using formatSpec. Must be called before element text is set.
DIRTYCODE_API int32_t XmlAttrSetFloat(char *pBuffer, const char *pAttrName, const char *pFormatSpec, float fValue);

// Set a date attribute for an open element (started tag). Must be called before element text is set.
DIRTYCODE_API int32_t XmlAttrSetDate(char *pBuffer, const char *pAttrName, uint32_t uEpochDate);

// Set a text value for an open element. Must have an open element (started tag).
DIRTYCODE_API int32_t XmlElemSetString(char *pBuffer, const char *pValue);

// Set a text value for an open element, with no text encoding being performed on the input string. Must have an open element (started tag).
DIRTYCODE_API int32_t XmlElemSetStringRaw(char *pBuffer, const char *pValue);

// Set an integer value for an open element. Must have an open element (started tag).
DIRTYCODE_API int32_t XmlElemSetInt(char *pBuffer, int32_t iValue);

// Set an IP address value for an open element (in dot notation). Must have an open element (started tag).
DIRTYCODE_API int32_t XmlElemSetAddr(char *pBuffer, uint32_t uAddr);

// Set a date for an open element. Must have an open element (started tag).
DIRTYCODE_API int32_t XmlElemSetDate(char *pBuffer, uint32_t uEpoch);

// Build an XML output string from a formatted pattern (like vprintf)
#ifdef va_start
DIRTYCODE_API char *XmlFormatVPrintf(char *pXmlBuff, int32_t iBufLen, const char *pFormat, va_list pFmtArgs);
#endif

// Build an XML output string from a formatted pattern (like printf)
DIRTYCODE_API char *XmlFormatPrintf(char *pXmlBuff, int32_t iBufLen, const char *pFormat, ...);

#ifdef __cplusplus
}
#endif

//@}

#endif // _xmlformat_h
