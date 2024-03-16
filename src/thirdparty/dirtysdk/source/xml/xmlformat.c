/*H*************************************************************************************/
/*!
    \File xmlformat.c

    \Description
         This module formats simple Xml, in a linear fashion, using a character buffer
         that the client provides.  See header for more information.

    \Copyright
        Copyright (c) Electronic Arts 2004-2008.  ALL RIGHTS RESERVED.

    \Notes
        \verbatim
        When XmlInit() is called, a 24-bytes header is added at the beginning of the
        client-provided buffer. The header is an ascii string that looks like this:
            <AAAAAAAABBBBBBBBCCDD />
                where:
                    AAAAAAAA is the offset field
                    BBBBBBBB is the buffer size field
                    CC is the indent field
                    DD is the flag field
        Those first 24 bytes are replaced by whitespaces when XmlFinish() is called.

        Upon buffer overrun:
        1. _XmlIndent: negative is returned and buffer remain untouched;
        2. _XmlEncodeString: negative is returned and buffer[0] has been reset to '\0' (if length(buffer) > 0);
        3. snzprintf: zero/negative is returned and buffer[0] has been reset to '\0' (if length(buffer) > 0).

        Required buffer capacity for xml = length(xml) + 1.
        That means if length(buffer) <= length(xml), buffer-overrun occurs.
        \endverbatim

    \Version 01/30/2004 (jbertrand) First Version
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/xml/xmlformat.h"

/*** Defines ***************************************************************************/
#define XML_HEADER_LEN       24
#define XML_MAX_TAG_LENGTH   128
#define XML_MAX_FLOAT_LENGTH 128
#define XML_MAX_DATE_LENGTH  32

/*** Type Definitions ******************************************************************/

/*** Variables *************************************************************************/
static const unsigned char _Xml_Hex2AsciiTable[16] = "0123456789abcdef";

// ascii->hex conversion table for 7bit ASCII characters
static const unsigned char _Xml_Ascii2HexTable[128] =
{
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0,  0,  0,  0,  0,  0,
    0, 10, 11, 12, 13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0, 10, 11, 12, 13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

// 7bit ASCII characters that should be encoded ([1..31 minus tab, cr, lf], <, =, >, &, ", and DEL)
static const unsigned char _Xml_EncodeStringTable[128] =
{
    0,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  1,  1,  0,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,
};

/*** Private Functions *****************************************************************/
static int32_t _XmlGetOffset(char *pXml)
{
    return((_Xml_Ascii2HexTable[(int32_t)pXml[1]] << 28) |
           (_Xml_Ascii2HexTable[(int32_t)pXml[2]] << 24) |
           (_Xml_Ascii2HexTable[(int32_t)pXml[3]] << 20) |
           (_Xml_Ascii2HexTable[(int32_t)pXml[4]] << 16) |
           (_Xml_Ascii2HexTable[(int32_t)pXml[5]] << 12) |
           (_Xml_Ascii2HexTable[(int32_t)pXml[6]] << 8) |
           (_Xml_Ascii2HexTable[(int32_t)pXml[7]] << 4) |
           _Xml_Ascii2HexTable[(int32_t)pXml[8]]);
}

static int32_t _XmlGetLength(char *pXml)
{
    return((_Xml_Ascii2HexTable[(int32_t)pXml[9]] << 28) |
           (_Xml_Ascii2HexTable[(int32_t)pXml[10]] << 24) |
           (_Xml_Ascii2HexTable[(int32_t)pXml[11]] << 20) |
           (_Xml_Ascii2HexTable[(int32_t)pXml[12]] << 16) |
           (_Xml_Ascii2HexTable[(int32_t)pXml[13]] << 12) |
           (_Xml_Ascii2HexTable[(int32_t)pXml[14]] << 8) |
           (_Xml_Ascii2HexTable[(int32_t)pXml[15]] << 4) |
           _Xml_Ascii2HexTable[(int32_t)pXml[16]]);
}

static int32_t _XmlGetIndent(char *pXml)
{
    return((_Xml_Ascii2HexTable[(int32_t)pXml[17]] << 4) |
            _Xml_Ascii2HexTable[(int32_t)pXml[18]]);
}

static int32_t _XmlGetFlags(char *pXml)
{
    return((_Xml_Ascii2HexTable[(int32_t)pXml[19]] << 4) |
            _Xml_Ascii2HexTable[(int32_t)pXml[20]]);
}

static void _XmlSetOffset(char *pXml, int32_t iOff)
{
    pXml[1] = _Xml_Hex2AsciiTable[(iOff>>28)&15];
    pXml[2] = _Xml_Hex2AsciiTable[(iOff>>24)&15];
    pXml[3] = _Xml_Hex2AsciiTable[(iOff>>20)&15];
    pXml[4] = _Xml_Hex2AsciiTable[(iOff>>16)&15];
    pXml[5] = _Xml_Hex2AsciiTable[(iOff>>12)&15];
    pXml[6] = _Xml_Hex2AsciiTable[(iOff>>8)&15];
    pXml[7] = _Xml_Hex2AsciiTable[(iOff>>4)&15];
    pXml[8] = _Xml_Hex2AsciiTable[(iOff>>0)&15];
}

static void _XmlSetLength(char *pXml, int32_t iLen)
{
    pXml[9] = _Xml_Hex2AsciiTable[(iLen>>28)&15];
    pXml[10] = _Xml_Hex2AsciiTable[(iLen>>24)&15];
    pXml[11] = _Xml_Hex2AsciiTable[(iLen>>20)&15];
    pXml[12] = _Xml_Hex2AsciiTable[(iLen>>16)&15];
    pXml[13] = _Xml_Hex2AsciiTable[(iLen>>12)&15];
    pXml[14] = _Xml_Hex2AsciiTable[(iLen>>8)&15];
    pXml[15] = _Xml_Hex2AsciiTable[(iLen>>4)&15];
    pXml[16] = _Xml_Hex2AsciiTable[(iLen>>0)&15];
}

static void _XmlSetIndent(char *pXml, int32_t iIndent)
{
    pXml[17] = _Xml_Hex2AsciiTable[(iIndent >> 4) & 15];
    pXml[18] = _Xml_Hex2AsciiTable[(iIndent >> 0) & 15];
}

static void _XmlSetFlags(char *pXml, int32_t iFlags)
{
    pXml[19] = _Xml_Hex2AsciiTable[(iFlags >> 4) & 15];
    pXml[20] = _Xml_Hex2AsciiTable[(iFlags >> 0) & 15];
}

/*F*************************************************************************************/
/*!
    \Function   _XmlValidHeader

    \Description
        Check whether buffer is initialized with a valid header.

    \Input *pBuffer - buffer to which xml will be written

    \Output
        int32_t     - returns 1 if there is a valid header, 0 otherwise
*/
/*************************************************************************************F*/
static int32_t _XmlValidHeader(const char *pBuffer)
{
    // Make sure all the header components exist
    if ((pBuffer[0] != '<') || (pBuffer[XML_HEADER_LEN-3] != ' ') || (pBuffer[XML_HEADER_LEN-2] != '/') || (pBuffer[XML_HEADER_LEN-1] != '>'))
        return(0);
    else
        return(1);
}

/*F*************************************************************************************/
/*!
    \Function   _XmlOpenTag

    \Description
        Check whether there is an open tag in the buffer.

    \Input *pBuffer - buffer to which xml will be written
    \Input iOffset  - current offset within the buffer

    \Output
        int32_t     - returns 1 if an open tag exists in the buffer, 0 otherwise
*/
/*************************************************************************************F*/
static int32_t _XmlOpenTag(const char *pBuffer, int32_t iOffset)
{
    int32_t iBackPos;
    int32_t iBeginCount = 0;
    int32_t iEndCount = 0;

    for (iBackPos = (iOffset-1); iBackPos >= XML_HEADER_LEN; iBackPos--)
    {
        // Count all closing tags (ended tag with no children will be counted as starting and closing tag)
        if (   ((pBuffer[iBackPos] == '>') && (pBuffer[iBackPos-1] == '/'))
            || ((pBuffer[iBackPos] == '/') && (pBuffer[iBackPos-1] == '<')))
        {
            iEndCount++;
            iBackPos--;
        }
        // Count all starting tags
        else if (pBuffer[iBackPos] == '<')
        {
            iBeginCount++;
            // If there are more open tags than close tags, return true
            if (iBeginCount > iEndCount)
            {
                return(1);
            }
        }
    }
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function   _XmlOpenForAttr

    \Description
        Check whether the previous tag can still have attributes written.

    \Input *pBuffer - buffer to which xml will be written
    \Input iOffset  - current offset within the buffer

    \Output
        int32_t     - returns 1 if the previous tag can still have attributes written, 0 otherwise
*/
/*************************************************************************************F*/
static int32_t _XmlOpenForAttr(const char *pBuffer, int32_t iOffset)
{
    int32_t iBackPos = iOffset-1;

    if ( iOffset < 1 )
        return(0);

    // Buffer does not end with a tag, child text exists
    if (pBuffer[iBackPos] != '>')
        return(0);

    while (iBackPos >= XML_HEADER_LEN)
    {
        // Last tag in buffer is already closed
        if ((pBuffer[iBackPos] == '>') && (pBuffer[iBackPos-1] == '/'))
            return(0);
        // Buffer ends with an open tag and no children
        else if (pBuffer[iBackPos] == '<')
            return(1);
        else
            iBackPos--;
    }
    // No tags found in buffer
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function   _XmlUpdateHeader

    \Description
        Update the XML header to reflect the number of characters written. If the buffer
        was full, this function will update the header to indicate the full buffer and will
        return the XML_ERR_FULL error code.

    \Input *pBuffer     - buffer to which xml will be written
    \Input iNumChars    - number of characters written, -1 for buffer filled
    \Input iLength      - length of the buffer
    \Input iOffset      - current offset within the buffer
    \Input iIndent      - current indent level

    \Output
        int32_t         - returns XML_ERR_FULL if buffer was filled, XML_ERR_NONE otherwise
*/
/*************************************************************************************F*/
static int32_t _XmlUpdateHeader(char *pBuffer, int32_t iNumChars, int32_t iLength, int32_t iOffset, int32_t iIndent)
{
    if (iNumChars < 0)
    {
        pBuffer[iLength-1] = 0;
        iOffset = iLength;
        _XmlSetOffset(pBuffer, iOffset);
        return XML_ERR_FULL;
    }
    else
    {
        iOffset += iNumChars;
        _XmlSetOffset(pBuffer, iOffset);
        _XmlSetIndent(pBuffer, iIndent);
        return XML_ERR_NONE;
    }
}

/*F****************************************************************************/
/*!
    \Function   _XmlIndent

    \Description
        Add the appropriate level of whitespace to indent the current tag if
        the whitespace flag is enabled for the buffer.

    \Input *pBuffer     - buffer to which xml will be written
    \Input iLength      - length of the buffer
    \Input iOffset      - current offset within the buffer
    \Input iIndent      - current indent level
    \Input iFlags       - flags

    \Output
        int32_t         - negative=buffer overrun, zero/positive=number of characters written
*/
/****************************************************************************F*/
static int32_t _XmlIndent(char *pBuffer, int32_t iLength, int32_t iOffset, int32_t iIndent, int32_t iFlags)
{
    int32_t iNumChars = 0;

#if DIRTYCODE_LOGGING
    if (iIndent < 0)
    {
        NetPrintf(("xmlformat: warning -- invalid indent level (%d) in _XmlIndent\n", iIndent));
    }
#endif

    if ((iFlags & XML_FL_WHITESPACE) == 0)
        iNumChars = 0;
    else if (iIndent <= 0)
    {
        if ((iLength - iOffset) > 1)
        {
            iNumChars = ds_snzprintf(pBuffer + iOffset, iLength - iOffset, "\n");
        }
        else
        {
            iNumChars = -1;
        }
    }
    else
    {
        if ((iLength - iOffset) > ((iIndent * 2) + 1))
        {
            iNumChars = ds_snzprintf(pBuffer + iOffset, iLength - iOffset, "\n%*c", iIndent * 2, ' ');
        }
        else
        {
            iNumChars = -1;
        }
    }

    return(iNumChars);
}

/*F*************************************************************************************/
/*!
    \Function   _XmlFormatInsert

    \Description
        Insert a preformatted item

    \Input *pBuffer - the xml buffer
    \Input *pValue  - raw string to insert (must be correctly preformatted)

    \Output int32_t - return code.
*/
/*************************************************************************************F*/
static int32_t _XmlFormatInsert(char *pBuffer, const char *pValue)
{
    int32_t iWidth, iCount;
    int32_t iOffset, iLength, iIndent, iFlags;

    // make sure there is a header and extract fields
    if (!_XmlValidHeader(pBuffer))
        return(XML_ERR_UNINIT);

    iOffset = _XmlGetOffset(pBuffer);
    iLength = _XmlGetLength(pBuffer);
    iIndent = _XmlGetIndent(pBuffer);
    iFlags = _XmlGetFlags(pBuffer);

    // there must be an open tag in the buffer
    if (!_XmlOpenTag(pBuffer, iOffset))
    {
        return(XML_ERR_NOT_OPEN);
    }

    // check if there's enough room for the insertion to complete successfully
    iWidth = (int32_t)strlen(pValue);
    // figure out indent size
    iWidth += ((pValue[0] == '<') && (iFlags & XML_FL_WHITESPACE)) ? (1 + iIndent * 2) : 0;
    // we must be able to insert completely
    if ((iLength - iOffset) <= iWidth)
    {
        return(XML_ERR_FULL);
    }

    // buffer is good, now start to determine the insertion type
    // 1. <elem>value<elem>, handle indent if necessary
    if (pValue[0] == '<')
    {
        if (iFlags & XML_FL_WHITESPACE)
        {
            pBuffer[iOffset++] = '\n';
            for (iCount = 0; iCount < iIndent; ++iCount)
            {
                pBuffer[iOffset++] = ' ';
                pBuffer[iOffset++] = ' ';
            }
        }
    }
    // 2. attr="value">
    else if (strchr(pValue, '"') != NULL)
    {
        // the tag must be open for accepting attributes (no children and no element text set)
        if (!_XmlOpenForAttr(pBuffer, iOffset))
        {
            return(XML_ERR_ATTR_POSITION);
        }
        // replace the close tag '>' to ' ' for input (pValue[] must look like: attr="value">)
        pBuffer[iOffset - 1] = ' ';
    }

    // for all insertion types, insert the string
    for (iCount = 0; pValue[iCount] != 0; ++iCount)
    {
        pBuffer[iOffset++] = pValue[iCount];
    }
    pBuffer[iOffset] = 0;

    // update offset and return
    _XmlSetOffset(pBuffer, iOffset);
    return(XML_ERR_NONE);
}

/*F*************************************************************************************/
/*!
    \Function   _XmlEncodeString

    \Description
        Encode a string with %xx as needed for reserved characters

    \Input *pBuffer - the xml buffer
    \Input iLength  - number of eight-bit characters available in buffer
    \Input *_pSrc   - source string to add to buffer

    \Output int32_t - negative=not enough buffer, zero/positive=encoded length.
*/
/*************************************************************************************F*/
static int32_t _XmlEncodeString(char *pBuffer, int32_t iLength, const char *_pSrc)
{
    char * const pBackup = pBuffer;
    const uint8_t *pSource = (const uint8_t *)_pSrc;
    int32_t iRemain;

    // encode the string
    for (iRemain = iLength; (iRemain > 1) && (*pSource != '\0'); ++pSource)
    {
        if ((*pSource < 128) && (_Xml_EncodeStringTable[*pSource]))
        {
            // hex encode all out of range values
            if (iRemain > 6)
            {
                *pBuffer++ = '&';
                *pBuffer++ = '#';
                *pBuffer++ = 'x';
                *pBuffer++ = _Xml_Hex2AsciiTable[(*pSource)>>4];
                *pBuffer++ = _Xml_Hex2AsciiTable[(*pSource)&0xF];
                *pBuffer++ = ';';
                iRemain -= 6;
            }
            else // buffer overrun
            {
                break;
            }
        }
        else
        {
            // normal encoding
            *pBuffer++ = *pSource;
            --iRemain;
        }
    }

    // make sure space for terminator
    if (iRemain > 0)
    {
        *pBuffer = 0;
    }

    // if character(s) remains, return negative to indicate buffer-overrun.
    if (*pSource != '\0')
    {
        // if buffer has been changed, reset the change(s)
        if (pBackup != pBuffer)
        {
            *pBackup = '\0';
        }
        return(-1);
    }

    // return encoded size
    return(iLength-iRemain);
}

/*** Public Functions ******************************************************************/

/*F*************************************************************************************/
/*!
    \Function   XmlInit

    \Description
        Initialize the Xml Buffer. This MUST be called prior to any other calls.
        Client can allocate the buffer on stack, but should not access it directly. ( see XmlData() ).

    \Input *pBuffer - buffer to which xml will be written.
    \Input iBufLen  – size of buffer passed in.
    \Input uFlags   - encoding flags (XMLFORMAT_FL_xxx)
*/
/*************************************************************************************F*/
void XmlInit(char *pBuffer, int32_t iBufLen, unsigned char uFlags)
{
    int32_t iOffset;

    ds_memclr(pBuffer, iBufLen);
    if (iBufLen > XML_HEADER_LEN) {
        iOffset = ds_snzprintf(pBuffer, iBufLen - 1, "<00000000000000000000 />");
        _XmlSetOffset(pBuffer, iOffset);
        _XmlSetLength(pBuffer, iBufLen);
        _XmlSetFlags(pBuffer, uFlags);
    }
}

/*F*************************************************************************************/
/*!
    \Function   XmlBufSizeIncrease

    \Description
        Notify the xmlformat API that the buffer size was increased.

    \Input *pBuffer     - buffer to which xml is being written.
    \Input iNewBufLen   – new size for that buffer

    \Version 03/30/2010 (mclouatre)
*/
/*************************************************************************************F*/
void XmlBufSizeIncrease(char *pBuffer, int32_t iNewBufLen)
{
    _XmlSetLength(pBuffer, iNewBufLen);
}

/*F*************************************************************************************/
/*!
    \Function   XmlFinish

    \Description
        Signal completion of output to this buffer.  Clear the XML API hidden
        data.

    \Input *pBuffer – buffer to which xml was written.

    \Output char*   - pointer to real XML data (skipping past header)
*/
/*************************************************************************************F*/
char *XmlFinish(char *pBuffer)
{
    if (_XmlValidHeader(pBuffer))
    {
        ds_memset(pBuffer, ' ', XML_HEADER_LEN);
        pBuffer += XML_HEADER_LEN;
    }
    return(pBuffer);
}

/*F*************************************************************************************/
/*!
    \Function   XmlTagStart

    \Description
        Start an xml element (node), to which you can add other elements or attributes.
        Must be ended with XmlTagEnd.

    \Input *pBuffer - xml buffer.
    \Input *pName   - Name of the tag.

    \Output
        int32_t     - return code.

    \Todo
        Consider using  var-args for attributes?
*/
/*************************************************************************************F*/
int32_t XmlTagStart(char *pBuffer, const char *pName)
{
    int32_t iOffset, iLength, iNumChars, iIndent, iFlags, iTempLength;

    if (!_XmlValidHeader(pBuffer))
        return(XML_ERR_UNINIT);

    iOffset = _XmlGetOffset(pBuffer);
    iLength = _XmlGetLength(pBuffer);
    iIndent = _XmlGetIndent(pBuffer);
    iFlags = _XmlGetFlags(pBuffer);

    if ((iNumChars = _XmlIndent(pBuffer, iLength, iOffset, iIndent++, iFlags)) < 0)
    {
        return(XML_ERR_FULL);
    }

    if ((iTempLength = ds_snzprintf(pBuffer + iOffset + iNumChars, iLength - iOffset - iNumChars, "<%s>", pName)) <= 0)
    {
        pBuffer[iOffset] = '\0'; // reset all changes
        return(XML_ERR_FULL);
    }
    iNumChars += iTempLength;

    return _XmlUpdateHeader(pBuffer, iNumChars, iLength, iOffset, iIndent);
}

/*F*************************************************************************************/
/*!
    \Function   XmlTagEnd

    \Description
        End the current element or tag -- must have an outstanding open tag.

    \Input *pBuffer - xml buffer.

    \Output
        int32_t     - return code.
*/
/*************************************************************************************F*/
int32_t XmlTagEnd(char *pBuffer)
{
    int32_t iOffset, iLength, iNumChars;
    int32_t iBackPos;
    int32_t iEndCount = 0;
    int32_t iBeginCount = 0;
    int32_t iIndent;
    int32_t iFlags;

    if (!_XmlValidHeader(pBuffer))
        return(XML_ERR_UNINIT);

    iOffset = _XmlGetOffset(pBuffer);
    iLength = _XmlGetLength(pBuffer);
    iIndent = _XmlGetIndent(pBuffer);
    iFlags = _XmlGetFlags(pBuffer);
    iIndent--;

    for (iBackPos = (iOffset-1); iBackPos >= XML_HEADER_LEN; iBackPos--)
    {
        // Count all closing tags (ended tag with no children will be counted as starting and closing tag)
        if (   ((pBuffer[iBackPos] == '>') && (pBuffer[iBackPos-1] == '/'))
            || ((pBuffer[iBackPos] == '/') && (pBuffer[iBackPos-1] == '<')))
        {
            iEndCount++;
            iBackPos--;
        }
        // Count all starting tags, if there are more starting than closing tags, we have found the tag to close
        else if (pBuffer[iBackPos] == '<')
        {
            iBeginCount++;
            if (iBeginCount > iEndCount)
            {
                if ((iLength - iOffset) < 3) // "/>"
                {
                    // nothing changed, return(XML_ERR_FULL) directly
                    return(XML_ERR_FULL);
                }

                // If the last tag in the buffer is open and has no children, close it
                if ((iEndCount == 0) && (pBuffer[iOffset-1] == '>'))
                {
                    pBuffer[iOffset-1] = ' ';
                    iNumChars = ds_snzprintf(pBuffer+iOffset, iLength-iOffset, "/>");
                    return(_XmlUpdateHeader(pBuffer, iNumChars, iLength, iOffset, iIndent));
                }
                // The open tag has children, so create a new closing tag
                else
                {
                    char tagBuffer[XML_MAX_TAG_LENGTH + 4];
                    int32_t iTagLen = 0;
                    tagBuffer[iTagLen++] = '<';
                    tagBuffer[iTagLen++] = '/';
                    tagBuffer[iTagLen] = '0';
                    // Scan forward to obtain the name for the closing tag
                    do
                    {
                        iBackPos++;
                        tagBuffer[iTagLen] = pBuffer[iBackPos];
                        iTagLen++;
                    } while ((iBackPos < iLength) && (pBuffer[iBackPos+1] != ' ') && (pBuffer[iBackPos+1] != '>') && (iTagLen < XML_MAX_TAG_LENGTH-1));
                    tagBuffer[iTagLen++] = '>';
                    tagBuffer[iTagLen] = 0;
                    if (((iNumChars = _XmlIndent(pBuffer, iLength, iOffset, iIndent, iFlags)) < 0)
                        || ((iLength - iOffset) <= (iTagLen + iNumChars)))
                    {
                        pBuffer[iOffset] = '\0'; // reset all changes
                        return(XML_ERR_FULL);
                    }
                    iNumChars += ds_snzprintf(pBuffer+iOffset+iNumChars, iLength-iOffset-iNumChars, "%s", tagBuffer);
                    return(_XmlUpdateHeader(pBuffer, iNumChars, iLength, iOffset, iIndent));
                }
            }
        }
    }
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function   XmlElemAddString

    \Description
        Add a complete, contained text element. Builds start and end tag.
        Nothing can be appended to this tag.

    \Input *pBuffer   - xml buffer.
    \Input *pElemName - name of the element (tag).
    \Input *pValue    - value of the element.

    \Output
        int32_t       - return code.
*/
/*************************************************************************************F*/
int32_t XmlElemAddString(char *pBuffer, const char *pElemName, const char *pValue)
{
    int32_t iOffset, iLength, iNumChars, iIndent, iFlags, iTempLength;

    if (!_XmlValidHeader(pBuffer))
        return(XML_ERR_UNINIT);

    iOffset = _XmlGetOffset(pBuffer);
    iLength = _XmlGetLength(pBuffer);
    iIndent = _XmlGetIndent(pBuffer);
    iFlags = _XmlGetFlags(pBuffer);

    // there must be an open tag in the buffer
    if (_XmlOpenTag(pBuffer, iOffset))
    {
        if ((iNumChars = _XmlIndent(pBuffer, iLength, iOffset, iIndent, iFlags)) < 0)
        {
            return(XML_ERR_FULL);
        }

        // <ElemName>
        if ((iTempLength = ds_snzprintf(pBuffer+iOffset+iNumChars, iLength-iOffset-iNumChars, "<%s>", pElemName)) <= 0)
        {
            pBuffer[iOffset] = '\0'; // reset all changes
            return(XML_ERR_FULL);
        }
        iNumChars += iTempLength;

        // Value
        if ((iTempLength = _XmlEncodeString(pBuffer+iOffset+iNumChars, iLength-iOffset-iNumChars, pValue)) < 0)
        {
            pBuffer[iOffset] = '\0'; // reset all changes
            return(XML_ERR_FULL);
        }
        iNumChars += iTempLength;

        // </ElemName>
        if ((iTempLength = ds_snzprintf(pBuffer+iOffset+iNumChars, iLength-iOffset-iNumChars, "</%s>", pElemName)) <= 0)
        {
            pBuffer[iOffset] = '\0'; // reset all changes
            return(XML_ERR_FULL);
        }
        iNumChars += iTempLength;

        // all done successfully
        return(_XmlUpdateHeader(pBuffer, iNumChars, iLength, iOffset, iIndent));
    }
    else
    {
        return(XML_ERR_NOT_OPEN);
    }
}

/*F*************************************************************************************/
/*!
    \Function   XmlElemAddInt

    \Description
        Add a complete, contained integer element. Builds start and end tag.
        Nothing can be appended to this tag.

    \Input *pBuffer   - xml buffer.
    \Input *pElemName - name of the element (tag).
    \Input iValue     - integer value of the element.

    \Output
        int32_t       - return code.
*/
/*************************************************************************************F*/
int32_t XmlElemAddInt(char *pBuffer, const char *pElemName, int32_t iValue)
{
    char strInt[32]; // [32] is large enough for int32_t string

    ds_snzprintf(strInt, sizeof(strInt), "%d", iValue);
    return(XmlElemAddString(pBuffer, pElemName, strInt));
}

/*F*************************************************************************************/
/*!
    \Function   XmlElemAddFloat

    \Description
        Add a complete, contained decimal element. Builds start and end tag.
        Nothing can be appended to this tag. Format it using formatSpec.

    \Input *pBuffer     - xml buffer.
    \Input *pElemName   - name of the element (tag).
    \Input *pFormatSpec - Spec for formatting the float (see sprintf).
    \Input fValue       - float value of the element.

    \Output
        int32_t         - return code.
*/
/*************************************************************************************F*/
int32_t XmlElemAddFloat(char *pBuffer, const char *pElemName, const char *pFormatSpec, float fValue)
{
    char strFloat[XML_MAX_FLOAT_LENGTH];

    if (ds_snzprintf(strFloat, sizeof(strFloat), pFormatSpec, fValue) <= 0)
    {
        return(XML_ERR_INVALID_PARAM);
    }
    return(XmlElemAddString(pBuffer, pElemName, strFloat));
}

/*F*************************************************************************************/
/*!
    \Function    XmlElemAddDate

    \Description
        Add a complete, contained Date element, using epoch date. Builds start and end tag.
        Nothing can be appended to this tag.

    \Input *pBuffer     - xml buffer.
    \Input *pElemName   - name of the element (tag).
    \Input uEpochDate   - date/time since epoch.

    \Output
        int32_t         - return code.
*/
/*************************************************************************************F*/
int32_t XmlElemAddDate(char *pBuffer, const char *pElemName, uint32_t uEpochDate)
{
    struct tm *time1, time2;
    char strDate[XML_MAX_DATE_LENGTH];

    if ((time1 = ds_secstotime(&time2, uEpochDate)) == NULL)
    {
        return(XML_ERR_INVALID_PARAM);
    }

    ds_snzprintf(strDate,
              sizeof(strDate),
              "%04d-%02d-%02dT%02d:%02d:%02dZ",
              time1->tm_year + 1900,
              time1->tm_mon + 1,
              time1->tm_mday,
              time1->tm_hour,
              time1->tm_min,
              time1->tm_sec);
    return(XmlElemAddString(pBuffer, pElemName, strDate));
}

/*F*************************************************************************************/
/*!
    \Function   XmlAttrSetString

    \Description
        Set a text attribute for an open element (started tag).
        Must be called before Element text is set.

    \Input *pBuffer     - xml buffer.
    \Input *pAttrName   - name of the attribute to be added.
    \Input *pValue      - string value of the element.

    \Output
        int32_t         - return code.
*/
/*************************************************************************************F*/
int32_t XmlAttrSetString(char *pBuffer, const char *pAttrName, const char *pValue)
{
    int32_t iOffset, iLength, iNumChars, iIndent;
    int32_t iInsert;

    if (!_XmlValidHeader(pBuffer))
        return(XML_ERR_UNINIT);

    iOffset = iInsert = _XmlGetOffset(pBuffer);
    iLength = _XmlGetLength(pBuffer);
    iIndent = _XmlGetIndent(pBuffer);

    // there must be an open tag in the buffer
    if (_XmlOpenTag(pBuffer, iOffset))
    {
        // the tag must be open for accepting attributes (no children and no element text set)
        if (_XmlOpenForAttr(pBuffer, iOffset))
        {
            // insert the tag
            if ((iNumChars = ds_snzprintf(pBuffer+iInsert, iLength-iInsert, "%s=\"", pAttrName)) <= 0)
            {
                pBuffer[iOffset] = '\0'; // reset all changes
                return(XML_ERR_FULL);
            }
            iInsert += iNumChars;

            // insert the value
            if ((iNumChars = _XmlEncodeString(pBuffer+iInsert, iLength-iInsert, pValue)) < 0)
            {
                pBuffer[iOffset] = '\0'; // reset all changes
                return(XML_ERR_FULL);
            }
            iInsert += iNumChars;

            // see if there is room for final terminator -- if so, include entire field
            if ((iInsert + 3) > iLength)
            {
                pBuffer[iOffset] = '\0'; // reset all changes
                return(XML_ERR_FULL);
            }
            // <name> --> <name>attr="value --> <name attr="value">
            pBuffer[iOffset-1] = ' ';
            pBuffer[iInsert+0] = '"';
            pBuffer[iInsert+1] = '>';
            pBuffer[iInsert+2] = '\0';

            // update the header
            return(_XmlUpdateHeader(pBuffer, iInsert-iOffset+2, iLength, iOffset, iIndent));
        }
        else
            return(XML_ERR_ATTR_POSITION);
    }
    else
    {
        return(XML_ERR_NOT_OPEN);
    }
}

/*F*************************************************************************************/
/*!
    \Function XmlAttrSetStringRaw

    \Description
        Set a text attribute for an open element (started tag) with no text encoding
        being performed on the input string.  Must be called before Element text is set.

    \Input *pBuffer     - xml buffer.
    \Input *pAttr       - attribute to be added (expected format: attrName="attrValue").

    \Output
        int32_t         - return code.
*/
/*************************************************************************************F*/
int32_t XmlAttrSetStringRaw(char *pBuffer, const char *pAttr)
{
    char strStringRaw[256];

    if (ds_snzprintf(strStringRaw, sizeof(strStringRaw), "%s>", pAttr) <= 0)
    {
        return(XML_ERR_INVALID_PARAM);
    }
    return(_XmlFormatInsert(pBuffer, strStringRaw));
}

/*F*************************************************************************************/
/*!
    \Function   XmlAttrSetInt

    \Description
        Set an integer attribute for an open element (started tag).
        Must be called before Element text is set.

    \Input *pBuffer     - xml buffer.
    \Input *pAttrName   - name of the attribute to be added.
    \Input iValue       - integer value to be set.

    \Output
        int32_t         - return code.
*/
/*************************************************************************************F*/
int32_t XmlAttrSetInt(char *pBuffer, const char *pAttrName, int32_t iValue)
{
    char strInt[32]; // [32] is large enough for int32_t string

    ds_snzprintf(strInt, sizeof(strInt), "%d", iValue);
    return(XmlAttrSetString(pBuffer, pAttrName, strInt));
}

/*F*************************************************************************************/
/*!
    \Function   XmlAttrSetFloat

    \Description
        Set a decimal attribute for an open element (started tag).
        Must be called before Element text is set. Format using formatSpec.

    \Input *pBuffer     - xml buffer.
    \Input *pAttrName   - name of the attribute (tag).
    \Input *pFormatSpec - Spec for formatting the float (see sprintf).
    \Input fValue       - float value of the attribute.

    \Output
        int32_t         - return code.
*/
/*************************************************************************************F*/
int32_t XmlAttrSetFloat(char *pBuffer, const char *pAttrName, const char *pFormatSpec, float fValue)
{
    char strFloat[XML_MAX_FLOAT_LENGTH];

    if (ds_snzprintf(strFloat, sizeof(strFloat), pFormatSpec, fValue) <= 0)
    {
        return(XML_ERR_INVALID_PARAM);
    }
    return(XmlAttrSetString(pBuffer, pAttrName, strFloat));
}

/*F*************************************************************************************/
/*!
    \Function   XmlAttrSetDate

    \Description
        Set a date attribute for an open element (started tag).
        Must be called before Element text is set.

    \Input *pBuffer     - xml buffer.
    \Input *pAttrName   - name of the attribute to be added.
    \Input uEpochDate   - date value to be set.

    \Output
        int32_t         - return code.
*/
/*************************************************************************************F*/
int32_t XmlAttrSetDate(char *pBuffer, const char *pAttrName, uint32_t uEpochDate)
{
    struct tm *time1, time2;
    char strDate[XML_MAX_DATE_LENGTH];

    if ((time1 = ds_secstotime(&time2, uEpochDate)) == NULL)
    {
        return(XML_ERR_INVALID_PARAM);
    }

    ds_snzprintf(strDate,
              sizeof(strDate),
              "%04d-%02d-%02dT%02d:%02d:%02dZ",
              time1->tm_year + 1900,
              time1->tm_mon + 1,
              time1->tm_mday,
              time1->tm_hour,
              time1->tm_min,
              time1->tm_sec);
    return(XmlAttrSetString(pBuffer, pAttrName, strDate));
}

/*F*************************************************************************************/
/*!
    \Function   XmlAttrSetAddr

    \Description
        Set an IP address attribute (in dot notation)

    \Input *pBuffer     - xml buffer.
    \Input *pAttrName   - name of the attribute to be added.
    \Input uAddr        - address value to be set.

    \Output int32_t     - return code.
*/
/*************************************************************************************F*/
int32_t XmlAttrSetAddr(char *pBuffer, const char *pAttrName, uint32_t uAddr)
{
    char strAddr[32]; //[32] is large enough for xxx.xxx.xxx.xxx

    ds_snzprintf(strAddr, sizeof(strAddr), "%a", uAddr);
    return(XmlAttrSetString(pBuffer, pAttrName, strAddr));
}

/*F*************************************************************************************/
/*!
    \Function   XmlElemSetString

    \Description
        Set text value for an open element (started tag).
        Must have an open or started tag, uses element from previous start tag call.
        To be used if element is simple text node with some attributes.

    \Input *pBuffer     - xml buffer.
    \Input *pValue      - value of the element.

    \Output
        int32_t         - return code.
*/
/*************************************************************************************F*/
int32_t XmlElemSetString(char *pBuffer, const char *pValue)
{
    int32_t iOffset, iLength, iNumChars, iIndent, iInsert;

    if (!_XmlValidHeader(pBuffer))
        return(XML_ERR_UNINIT);

    iOffset = iInsert = _XmlGetOffset(pBuffer);
    iLength = _XmlGetLength(pBuffer);
    iIndent = _XmlGetIndent(pBuffer);

    // there must be an open tag in the buffer
    if (_XmlOpenTag(pBuffer, iOffset))
    {
        // insert the value
        if ((iNumChars = _XmlEncodeString(pBuffer + iInsert, iLength - iInsert, pValue)) < 0)
        {
            pBuffer[iOffset] = '\0'; // reset all changes
            return(XML_ERR_FULL);
        }
        iInsert += iNumChars;

        // update the header
        return(_XmlUpdateHeader(pBuffer, iInsert-iOffset, iLength, iOffset, iIndent));
    }
    else
        return(XML_ERR_NOT_OPEN);
}

/*F*************************************************************************************/
/*!
    \Function   XmlElemSetStringRaw

    \Description
        Set text value for an open element (started tag), with no encoding being
        performed on the input string.  Must have an open or started tag, uses element
        from previous start tag call. To be used if element is simple text node with some
        attributes.

    \Input *pBuffer     - xml buffer.
    \Input *pValue      - value of the element.

    \Output
        int32_t         - return code.
*/
/*************************************************************************************F*/
int32_t XmlElemSetStringRaw(char *pBuffer, const char *pValue)
{
    return(_XmlFormatInsert(pBuffer, pValue));
}

/*F*************************************************************************************/
/*!
    \Function   XmlElemSetInt

    \Description
        Set integer value for an open element (started tag).
        Must have an open or started tag, uses element from previous start tag call.
        To be used if element is a simple node with some attributes.

    \Input *pBuffer - xml buffer.
    \Input iValue   - integer value of the element.

    \Output
        int32_t     - return code.
*/
/*************************************************************************************F*/
int32_t XmlElemSetInt(char *pBuffer, int32_t iValue)
{
    char strInt[32];

    ds_snzprintf(strInt, sizeof(strInt), "%d", iValue);
    return(XmlElemSetString(pBuffer, strInt));
}

/*F*************************************************************************************/
/*!
    \Function   XmlElemSetAddr

    \Description
        Set an IP address value (in dot notation) for an open element¸.

    \Input *pBuffer - xml buffer.
    \Input uAddr    - address to set

    \Output
        int32_t     - return code.
*/
/*************************************************************************************F*/
int32_t XmlElemSetAddr(char *pBuffer, uint32_t uAddr)
{
    char strAddr[32];

    ds_snzprintf(strAddr, sizeof(strAddr), "%a", uAddr);
    return(XmlElemSetString(pBuffer, strAddr));
}


/*F*************************************************************************************/
/*!
    \Function   XmlElemSetDate

    \Description
        Set a date for an open element. Must have an open element (started tag).

    \Input *pBuffer - xml buffer.
    \Input uEpoch   - date to set

    \Output
        int32_t     - return code.
*/
/*************************************************************************************F*/
int32_t XmlElemSetDate(char *pBuffer, uint32_t uEpoch)
{
    struct tm Date;
    char strDate[XML_MAX_DATE_LENGTH];

    if (ds_secstotime(&Date, uEpoch) == NULL)
    {
        return(XML_ERR_INVALID_PARAM);
    }
    ds_snzprintf(strDate,
              sizeof(strDate),
              "%04d-%02d-%02dT%02d:%02d:%02dZ",
              Date.tm_year + 1900,
              Date.tm_mon + 1,
              Date.tm_mday,
              Date.tm_hour,
              Date.tm_min,
              Date.tm_sec);
    return(XmlElemSetString(pBuffer, strDate));
}

/*F*************************************************************************************/
/*!
    \Function   XmlFormatVPrintf

    \Description
    \verbatim
        Takes a format string and variable parameter list and formats into syntax
        correct XML doing optimizations like removing empty tags (i.e., <x></x>
        becomes <x />). The "v" (variable args) version can be called by other
        functions that use "..." in the their prototype.
    \endverbatim

    \Input *pXmlBuff - xml output buffer
    \Input iBufLen   - length of Xml output buffer (negative=append to existing buffer)
    \Input *pFormat  - printf style format string
    \Input pFmtArgs  - variable argument list (use va_start / va_end to get)

    \Output char*    - pointer to formatted buffer
*/
/*************************************************************************************F*/
char *XmlFormatVPrintf(char *pXmlBuff, int32_t iBufLen, const char *pFormat, va_list pFmtArgs)
{
    int32_t iToken;
    int32_t iIndex;
    char strName[65] = "";
    const char *pName;
    const char *pParse;
    unsigned char uFlags = 0;
    /* table of allowed name characters.  see http://www.w3.org/TR/xml/#charsets
       for information on name characters allowed by the XML 1.0 specification */
    static char _ConvName[128] =
        "                " "                "
        "             -. " "0123456789:     "
        " ABCDEFGHIJKLMNO" "PQRSTUVWXYZ    _"
        " abcdefghijklmno" "pqrstuvwxyz     ";

    // determine if whitespace indenting is desired
    if (*pFormat == ' ')
    {
        uFlags |= XML_FL_WHITESPACE;
        ++pFormat;
    }

    // start the formatting
    if (iBufLen >= 0)
        XmlInit(pXmlBuff, iBufLen, uFlags);

    // parse the format string
    for (pParse = pFormat; *pParse != 0; ++pParse)
    {
        // look for tag open
        if (pParse[0] == '<')
        {
            for (iIndex = 0; (iIndex < (signed)(sizeof(strName) - 1)) && (pParse[iIndex+1] > 0) && (pParse[iIndex+1] < 127); ++iIndex)
            {
                if (_ConvName[(int32_t)pParse[iIndex+1]] <= ' ')
                    break;
                strName[iIndex] = pParse[iIndex+1];
            }
            strName[iIndex] = '\0';
            if (iIndex > 0)
            {
                XmlTagStart(pXmlBuff, strName);
            }
        }

        // parse name for assignments
        if (pParse[0] == '=')
        {
            // find start of name
            for (pName = pParse; (pName != pFormat) && (pName[-1] > 0) && (pName[-1] < 127); --pName)
            {
                if (_ConvName[pName[-1]&127] <= ' ')
                    break;
            }
            // copy and format name in private buffer
            for (iIndex = 0; (iIndex < (signed)(sizeof(strName) - 1)) && (pName+iIndex != pParse); ++iIndex)
            {
                strName[iIndex] = pName[iIndex];
            }
            strName[iIndex] = '\0';
        }

        // grab next 3 characters as a token
        iToken = ' ';
        iToken = (iToken << 8) | pParse[0];
        iToken = (iToken << 8) | pParse[1];
        iToken = (iToken << 8) | pParse[2];

        // handle format tokens
        if ((pParse[0] == '/') && (pParse[1] == '>'))
        {
            XmlTagEnd(pXmlBuff);
        }
        else if (iToken == ' >%s')
        {
            const char *pString = va_arg(pFmtArgs, const char *);
            XmlElemSetString(pXmlBuff, pString);
        }
        else if (iToken == ' >%d')
        {
            int32_t iInteger = va_arg(pFmtArgs, int32_t);
            XmlElemSetInt(pXmlBuff, iInteger);
        }
        else if (iToken == ' >%e')
        {
            uint32_t uEpoch = va_arg(pFmtArgs, uint32_t);
            if (uEpoch == 0)
                uEpoch = (uint32_t)ds_timeinsecs();
            XmlElemSetDate(pXmlBuff, uEpoch);
        }
        else if (iToken == ' >%a')
        {
            uint32_t uAddr = va_arg(pFmtArgs, uint32_t);
            XmlElemSetAddr(pXmlBuff, uAddr);
        }
        else if (strName[0] == '\0')
        {
        }
        else if (iToken == ' =%s')
        {
            const char *pString = va_arg(pFmtArgs, const char *);
            XmlAttrSetString(pXmlBuff, strName, pString);
        }
        else if (iToken == ' =%d')
        {
            int32_t iInteger = va_arg(pFmtArgs, int32_t);
            XmlAttrSetInt(pXmlBuff, strName, iInteger);
        }
        else if (iToken == ' =%e')
        {
            uint32_t uEpoch = va_arg(pFmtArgs, uint32_t);
            if (uEpoch == 0)
                uEpoch = (uint32_t)ds_timeinsecs();
            XmlAttrSetDate(pXmlBuff, strName, uEpoch);
        }
        else if (iToken == ' =%a')
        {
            uint32_t uAddr = va_arg(pFmtArgs, uint32_t);
            XmlAttrSetAddr(pXmlBuff, strName, uAddr);
        }
    }

    // finish up and return data pointer
    if (iBufLen >= 0)
        pXmlBuff = XmlFinish(pXmlBuff);
    return(pXmlBuff);
}

/*F*************************************************************************************/
/*!
    \Function   XmlFormatPrintf

    \Description
    \verbatim
        Takes a format string and variable parameter list and formats into syntax
        correct XML doing optimizations like removing empty tags (i.e., <x></x>
        becomes <x />).
    \endverbatim

    \Input *pXmlBuff - xml output buffer
    \Input iBufLen   - length of Xml output buffer (negative=append to existing buffer)
    \Input *pFormat  - printf style format string

    \Output char*    - pointer to formatted buffer
*/
/*************************************************************************************F*/
char *XmlFormatPrintf(char *pXmlBuff, int32_t iBufLen, const char *pFormat, ...)
{
    char *pResult;
    va_list pFmtArgs;

    // setup varargs and call real function
    va_start(pFmtArgs, pFormat);
    pResult = XmlFormatVPrintf(pXmlBuff, iBufLen, pFormat, pFmtArgs);
    va_end(pFmtArgs);

    // return pointer to start of valid XML data (skips leading spaces)
    return(pResult);
}
