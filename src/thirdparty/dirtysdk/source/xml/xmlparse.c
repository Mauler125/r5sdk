/*H*************************************************************************************************/
/*!
    \File xmlparse.c

    \Description
        \verbatim
        This is a simple XML parser for use in controlled situations. It should not
        be used with arbitrary XML from uncontrolled hosts (i.e., test its parsing against
        a particular host before using it). This only implement a small subset of full
        XML and while suitable for many applications, it is easily confused by badly
        formed XML or by some of the legitimate but convoluted XML conventions.

        In particular, this parser cannot handle degenerate empty elements (i.e., <BR><BR>
        will confuse it). Empty elements must be of proper XML form <BR/><BR/>. Only the
        predefined entity types (&lt; &gt; &amp; &apos; &quot) are supported. This module
        does not support unicode values.
        \endverbatim

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002.  ALL RIGHTS RESERVED.

    \Version 1.0 01/30/2002 (gschaefer) First Version
*/
/*************************************************************************************************H*/


/*** Include files ********************************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/xml/xmlparse.h"

/*** Type Definitions *****************************************************************************/

/*** Variables ************************************************************************************/

static const unsigned char _Xml_TopDecode[256] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0, 16, 32, 48, 64, 80, 96,112,128,144,  0,  0,  0,  0,  0,  0,
      0,160,176,192,208,224,240,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,160,176,192,208,224,240,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};
static const unsigned char _Xml_BtmDecode[256] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0,  0,  0,  0,  0,  0,
      0, 10, 11, 12, 13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0, 10, 11, 12, 13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

// <![CDATA[
static const uint8_t _Xml_CDataHeader[9] =
    { '<', '!', '[', 'C', 'D', 'A', 'T', 'A', '[' };

// ]]>
static const uint8_t _Xml_CDataTrailer[3] =
{ ']', ']', '>' };

/*** Private Functions ****************************************************************************/

/*F************************************************************************************************/
/*!
    \Function _ParseNumber

    \Description
        Convert a number from string to integer form

    \Input pData    - pointer to source string
    \Input pValue   - pointer to output number

    \Output         - Pointer to next string element

    \Version 04/19/2002 (GWS)
*/
/************************************************************************************************F*/
static const unsigned char *_ParseNumber(const unsigned char *pData, int32_t *pValue)
{
    for (*pValue = 0; (*pData >= '0') && (*pData <= '9'); ++pData)
    {
        *pValue = (*pValue * 10) + (*pData & 15);
    }
    return(pData);
}

/*F************************************************************************************************/
/*!
    \Function _XmlContentChar

    \Description
        Translate an encoded content character to ASCII.

    \Input pXml     - pointer to raw xml source
    \Input pData    - pointer to translated output character

    \Output         - Pointer to next xml source item

    \Version 01/30/2002 (GWS)
*/
/************************************************************************************************F*/
static const unsigned char *_XmlContentChar(const unsigned char *pXml, unsigned char *pData)
{
    uint32_t uNumber;

    // default to bogus
    *pData = '~';

    // coded values
    if (pXml[0] == '#')
    {
        // hex
        if (pXml[1] == 'x')
        {
            // convert hex to integer
            for (uNumber = 0, pXml += 2; (*pXml != '\0') && (_Xml_BtmDecode[*pXml] > 0); ++pXml)
            {
                uNumber = (uNumber << 4) | _Xml_BtmDecode[*pXml];
            }
            // truncate to 8 bits
            *pData = (unsigned char)uNumber;
        }
        // decimal
        else
        {
            // convert decimal to integer
            for (uNumber = 0, pXml += 1; (*pXml >= '0') && (*pXml <= '9'); ++pXml)
            {
                uNumber = (uNumber * 10) + (*pXml & 15);
            }
            // truncate to 8 bits
            *pData = (unsigned char)uNumber;
        }
    }
    // &amp;
    else if ((pXml[0] == 'a') && (pXml[1] == 'm') && (pXml[2] == 'p'))
    {
        pXml += 3;
        *pData = '&';
    }
    // &apos;
    else if ((pXml[0] == 'a') && (pXml[1] == 'p') && (pXml[2] == 'o') && (pXml[3] == 's'))
    {
        pXml += 4;
        *pData = '\'';
    }
    // &quot;
    else if ((pXml[0] == 'q') && (pXml[1] == 'u') && (pXml[2] == 'o') && (pXml[3] == 't'))
    {
        pXml += 4;
        *pData = '"';
    }
    // &lt;
    else if ((pXml[0] == 'l') && (pXml[1] == 't'))
    {
        pXml += 2;
        *pData = '<';
    }
    // &gt;
    else if ((pXml[0] == 'g') && (pXml[1] == 't'))
    {
        pXml += 2;
        *pData = '>';
    }

    // skip the terminator if present
    if (*pXml == ';')
    {
        pXml += 1;
    }

    // return updated pointer
    return(pXml);
}


/*F************************************************************************************************/
/*!
    \Function _XmlContentFind

    \Description
        Locate the content area for the current element

    \Input _pXml    - pointer to xml element (start tag)

    \Output         - Pointer to content area (NULL=no content)

    \Version 01/30/2002 (GWS)
*/
/************************************************************************************************F*/
static const unsigned char *_XmlContentFind(const char *_pXml)
{
    const unsigned char *pXml = (const unsigned char *)_pXml;

    // we should be pointing at an element start
    if ((pXml == NULL) || (*pXml != '<'))
    {
        pXml = NULL;
    }
    // find end of element
    else
    {
        while ((*pXml != 0) && (*pXml != '>'))
        {
            ++pXml;
        }
        if (*pXml != 0)
        {
            pXml = (pXml[-1] == '/' ? NULL : pXml+1);
        }
    }
    // return content pointer
    return(pXml);
}


/*F************************************************************************************************/
/*!
    \Function _XmlAttribFind

    \Description
        Locate a named attribute within an element

    \Input _pXml        - pointer to xml element (start tag)
    \Input _pAttrib     - pointer to attribute to find

    \Output
        unsigned char * - pointer to attribute value (NULL=no content)

    \Version 01/30/2002 (GWS)
*/
/************************************************************************************************F*/
static const unsigned char *_XmlAttribFind(const char *_pXml, const char *_pAttrib)
{
    int32_t iIndex;
    unsigned char uTerm;
    const unsigned char *pMatch = NULL;
    const unsigned char *pXml = (const unsigned char *)_pXml;
    const unsigned char *pAttrib = (const unsigned char *)_pAttrib;

    // we should be pointing at an element start
    if ((pXml != NULL) && (*pXml == '<'))
    {
        // skip past element name
        for (; (*pXml != 0) && (*pXml > ' '); ++pXml)
            ;

        // parse all the attributes
        for (;;)
        {
            // skip past the white space
            for (; (*pXml != 0) && (*pXml <= ' '); ++pXml)
                ;

            // see if this is the attribute we want
            for (iIndex = 0; (pXml[iIndex] != 0) && (pXml[iIndex] == pAttrib[iIndex]); ++iIndex)
                ;

            // see if we reached end of attribute name
            if (pAttrib[iIndex] == 0)
            {
                // skip past white space
                for (pXml += iIndex; (*pXml != 0) && (*pXml <= ' '); ++pXml)
                    ;
                // see if we found the matching attribute
                if (*pXml == '=')
                {
                    // find the start of the data
                    for (pMatch = pXml+1; (*pMatch != 0) && (*pMatch <= ' '); ++pMatch)
                        ;
                    break;
                }
            }

            // find end of attribute name
            for (; (*pXml != 0) && (*pXml != '>') && (*pXml != '='); ++pXml)
                ;

            // make sure this is an attribute assignment
            if (*pXml != '=')
            {
                break;
            }

            // skip equals and white space
            for (++pXml; (*pXml != 0) && (*pXml <= ' '); ++pXml)
                ;

            // see if the data is quoted
            if ((*pXml == '"') || (*pXml == '\''))
            {
                // find ending quote
                for (uTerm = *pXml++; (*pXml != 0) && (*pXml != uTerm); ++pXml)
                    ;
                // if we found a quote, skip it
                if (*pXml == uTerm)
                {
                    ++pXml;
                }
            }
            // just skip till next space
            else
            {
                for (; (*pXml != 0) && (*pXml > ' '); ++pXml)
                    ;
            }
        }
    }

    // return the match point
    return(pMatch);
}

/*F************************************************************************************************/
/*!
    \Function _XmlSkipCDataHeader

    \Description
        Determine if the string points to a CDATA header

    \Input pXml      - pointer to an XML content string
    \Input ppContent - pointer to pointer to the start of the content string

    \Output         - TRUE if the string demarks a CDATA header

    \Version 12/14/2006 (kcarpenter)
*/
/************************************************************************************************F*/
static int32_t _XmlSkipCDataHeader(const unsigned char *pXml, const unsigned char **ppContent)
{
    int32_t iIndex;

    // Compare the CDATA header char-by-char
    for (iIndex = 0; iIndex < (signed)sizeof(_Xml_CDataHeader); iIndex++)
    {
        if (pXml[iIndex] != _Xml_CDataHeader[iIndex])
        {
            return(FALSE);
        }
    }

    // Pass out content start pointer, if possible
    if (ppContent != NULL)
    {
        *ppContent = pXml + sizeof(_Xml_CDataHeader);
    }
    return(TRUE);
}

/*F************************************************************************************************/
/*!
    \Function _XmlSkipCDataTrailer

    \Description
        Determine if the string points to a CDATA trailer

    \Input pXml           - pointer to the possible end of an XML content string
    \Input ppAfterContent - pointer to pointer to the data immediately after the trailer

    \Output               - TRUE if the string demarks a CDATA trailer

    \Version 12/14/2006 (kcarpenter)
*/
/************************************************************************************************F*/
static int32_t _XmlSkipCDataTrailer(const unsigned char *pXml, const unsigned char **ppAfterContent)
{
    int32_t iIndex;

    // Compare the CDATA trailer char-by-char
    for (iIndex = 0; iIndex < (signed)sizeof(_Xml_CDataTrailer); iIndex++)
    {
        if (pXml[iIndex] != _Xml_CDataTrailer[iIndex])
        {
            return(FALSE);
        }
    }

    // Pass out pointer to after CDATA trailer, if possible
    if (ppAfterContent != NULL)
    {
        *ppAfterContent = pXml + sizeof(_Xml_CDataTrailer);
    }
    return(TRUE);
}

#if DIRTYCODE_LOGGING
/*F*************************************************************************************/
/*!
    \Function _XmlPrintFmtLine

    \Description
        Print current line buffer to debug output

    \Input *pLine       - line buffer to print
    \Input iLevel       - current xml depth
    \Input *pStrPrefix  - string to prefix each line with

    \Version 09/16/2010 (jbrookes)
*/
/*************************************************************************************F*/
static void _XmlPrintFmtLine(const char *pLine, int32_t iLevel, const char *pStrPrefix)
{
    // declare const string of 64 spaces which gives us 16 levels of indentiation with four spaces per indent
    const char strIndent[] =
        "                "
        "                "
        "                "
        "                ";
    int32_t iOffset;

    // figure out offset into tabs string based on level
    if ((iOffset = ((sizeof(strIndent)-1)/4 - iLevel)) < 0)
    {
        iOffset = 0;
    }
    iOffset *= 4;

    // output the string
    NetPrintf(("%s%s%s\r\n", pStrPrefix, strIndent + iOffset, pLine));
}
#endif

#if DIRTYCODE_LOGGING
/*F*************************************************************************************/
/*!
    \Function _XmlPrintFmtInsertChar

    \Description
        Insert a character into the print buffer.

    \Input **ppLine     - current insertion point
    \Input **ppXml      - current read point
    \Input *pLineStart  - start of line buffer
    \Input iBufSize     - size of line buffer
    \Input iLevel       - current xml depth
    \Input *pStrPrefix  - string to prefix each line with

    \Version 09/16/2010 (jbrookes)
*/
/*************************************************************************************F*/
static void _XmlPrintFmtInsertChar(char **ppLine, const unsigned char **ppXml, char *pLineStart, int32_t iBufSize, int32_t iLevel, const char *pStrPrefix)
{
    // add to line
    **ppLine = **ppXml;
    *ppLine += 1;
    *ppXml += 1;

    // check for full buffer
    if ((*ppLine - pLineStart) == (iBufSize - 1))
    {
        // flush current line and continue
        **ppLine = '\0';
        _XmlPrintFmtLine(pLineStart, iLevel, pStrPrefix);
        pLineStart[0] = ' ';
        *ppLine = pLineStart + 1;
    }
}
#endif

#if DIRTYCODE_LOGGING
/*F*************************************************************************************/
/*!
    \Function _XmlPrintFmt

    \Description
        Takes a as input an XML buffer, and prints out a "nicely" formatted version of
        the XML data to debug output.  Each line of output is appended to text generated
        by the format string and arguments.

    \Input *pXml        - xml to print
    \Input *pStrPrefix  - string to prefix each line with

    \Version 09/16/2010 (jbrookes)
*/
/*************************************************************************************F*/
static void _XmlPrintFmt(const unsigned char *pXml, const char *pStrPrefix)
{
    const char *pTagStart, *pTagEnd, *pTmpBuf;
    int32_t iLevel, iNewLevel;
    char strLine[132], *pLine;

    // skip any leading whitespace
    while ((*pXml == ' ') || (*pXml == '\t'))
    {
        pXml += 1;
    }

    // add xml to buffer
    for (iLevel = 0; *pXml != '\0'; )
    {
        // copy to line buffer until tag end, crlf, or eos
        pLine = strLine;
        pTagStart = (*pXml == '<') ? (const char *)pXml+1 : NULL;
        pTagEnd = NULL;

        // check for close tag
        iNewLevel = ((*pXml == '<') && (*(pXml+1) == '/')) ? iLevel-2 : iLevel;
        for (; (*pXml != '>') && (*pXml != '\r') && (*pXml != '\0'); )
        {
            if ((*pXml == ' ') && (pTagEnd == NULL))
            {
                pTagEnd = (const char *)pXml;
            }
            _XmlPrintFmtInsertChar(&pLine, &pXml, strLine, sizeof(strLine), iLevel, pStrPrefix);
        }
        if (*pXml == '>')
        {
            if ((*(pXml-1) != '/') && (*(pXml-1) != '?'))
            {
                iNewLevel += 1;
            }
            if (pTagEnd == NULL)
            {
                pTagEnd = (const char *)pXml;
            }
            _XmlPrintFmtInsertChar(&pLine, &pXml, strLine, sizeof(strLine), iLevel, pStrPrefix);
        }
        // scan ahead and see if we have a matching end tag
        if (pTagStart != NULL)
        {
            pTmpBuf = (const char *)pXml;
            while ((*pTmpBuf != '>') && (*pTmpBuf != '\r') && (*pTmpBuf != '\0'))
            {
                if ((*pTmpBuf == '<') && (*(pTmpBuf+1) == '/') && (!strncmp((const char *)pTagStart, (const char *)pTmpBuf+2, (size_t)(pTagEnd-pTagStart))))
                {
                    while ((*pXml != '>') && (*pXml != '\r') && (*pXml != '\0'))
                    {
                        _XmlPrintFmtInsertChar(&pLine, &pXml, strLine, sizeof(strLine), iLevel, pStrPrefix);
                    }
                    if (*pXml == '>')
                    {
                        _XmlPrintFmtInsertChar(&pLine, &pXml, strLine, sizeof(strLine), iLevel, pStrPrefix);
                    }
                    iNewLevel--;
                    break;
                }
                pTmpBuf++;
            }
        }
        // skip whitespace
        while ((*pXml == '\r') || (*pXml == '\n') || (*pXml == '\t') || (*pXml == ' '))
        {
            pXml++;
        }
        // null terminate
        *pLine = '\0';

        // indent based on level
        if (iNewLevel < iLevel)
        {
            iLevel = iNewLevel;
        }

        // output line
        _XmlPrintFmtLine(strLine, iLevel, pStrPrefix);

        // index to new level
        iLevel = iNewLevel;
    }
}
#endif

/*F************************************************************************************************/
/*!
    \Function _XmlSkip

    \Description
        Skip to the next xml element (used to enumerate lists)

    \Input *pXml    - pointer to xml element (start tag)
    \Input *pValid  - [optional, out] TRUE if element being skipped is complete, else FALSE

    \Output         - Pointer to next element at same level

    \Version 01/30/2002 (GWS)
*/
/************************************************************************************************F*/
static const unsigned char *_XmlSkip(const unsigned char *pXml, uint32_t *pValid)
{
    int32_t iIndex;
    uint32_t uValid;

    // allow NULL pValid
    if (pValid == NULL)
    {
        pValid = &uValid;
    }

    // assume invalid
    *pValid = FALSE;

    // make sure reference is valid
    if ((pXml == NULL) || (*pXml != '<'))
    {
        return(NULL);
    }

    // handle xml decl
    if (pXml[1] == '?')
    {
        // find the end sequence
        for (pXml += 2; (pXml[0] != 0) && (pXml[0] != '?') && (pXml[1] != '>'); ++pXml)
            ;
        // skip the terminator
        if (pXml[0] != 0)
        {
            pXml += 2;
        }
    }

    // handle doctype/comment
    else if ((pXml[0] == '<') && (pXml[1] == '!'))
    {
        // count past the < and >
        for (iIndex = 1, ++pXml; (pXml[0] != 0) && (iIndex > 0); ++pXml)
        {
            if (pXml[0] == '>')
            {
                iIndex -= 1;
            }
            if (pXml[0] == '<')
            {
                iIndex += 1;
            }
        }
    }

    // handle regular element
    else
    {
        // count past < and >
        for (iIndex = 1, ++pXml; (iIndex > 0) && (pXml[0] != 0);)
        {
            // if we found an element start
            if (pXml[0] == '<')
            {
                // see if this is CDATA, and if so, skip it
                if (_XmlSkipCDataHeader(pXml, &pXml))
                {
                    // skip over the CDATA section
                    while (TRUE)
                    {
                        if (*pXml == 0)
                        {
                            break;
                        }
                        if ((*pXml == ']') && _XmlSkipCDataTrailer(pXml, &pXml))
                        {
                            break;
                        }
                        ++pXml;
                    }
                    continue;
                }
                // if we hit a doctype/comment, then skip it
                if ((pXml[0] == '<') && (pXml[1] == '!'))
                {
                    if ((pXml = _XmlSkip(pXml, NULL)) == NULL)
                    {
                        return(pXml);
                    }
                    else
                    {
                        continue;
                    }
                }

                // adjust the indent level
                iIndex += (pXml[1] != '/' ? 1 : -1);
                // find the end of the element
                for (++pXml; (pXml[0] != 0) && (pXml[0] != '>'); ++pXml)
                    ;
                // dont gobble /> but do skip >
                if (pXml[-1] == '/')
                {
                    --pXml;
                }
                else if (pXml[0] == '>')
                {
                    ++pXml;
                }
            }
            // this is the end of an empty element
            else if ((pXml[0] == '/') && (pXml[1] == '>'))
            {
                iIndex -= 1;
                pXml += 2;
            }
            else
            {
                ++pXml;
            }
        }

        // valid?
        if (*pXml != '\0')
        {
            *pValid = 1;
        }
        else if ((iIndex <= 1) && (pXml[-1] == '>'))
        {
            *pValid = 1;
        }
    }

    // skip white space
    for (; (*pXml != 0) && (*pXml <= ' '); ++pXml)
        ;

    // see if we ran out of data
    if (*pXml == 0)
    {
        pXml = NULL;
    }

    // return new position
    return(pXml);
}


/*** Public functions *****************************************************************************/


/*F************************************************************************************************/
/*!
    \Function XmlSkip

    \Description
        Skip to the next xml element (used to enumerate lists)

    \Input *pXml    - pointer to xml element (start tag)

    \Output         - Pointer to next element at same level

    \Version 01/30/2002 (GWS)
*/
/************************************************************************************************F*/
const char *XmlSkip(const char *pXml)
{
    return((const char *)_XmlSkip((const unsigned char *)pXml, NULL));
}

/*F************************************************************************************************/
/*!
    \Function XmlComplete

    \Description
        Determines if the xml element pointed to is complete

    \Input *pXml    - pointer to xml element (start tag)

    \Output
        uint32_t    - TRUE if the element is complete, else FALSE

    \Version 09/24/2010 (jbrookes)
*/
/************************************************************************************************F*/
uint32_t XmlComplete(const char *pXml)
{
    uint32_t uValid;
    _XmlSkip((const unsigned char *)pXml, &uValid);
    return(uValid);
}

/*F************************************************************************************************/
/*!
    \Function XmlFind

    \Description
        Find an element within an xml document

    \Input _pXml    - pointer to xml document
    \Input _pName   - element name (x.y.z notation)

    \Output         - Pointer to named element start

    \Version 01/30/2002 (GWS)
*/
/************************************************************************************************F*/
const char *XmlFind(const char *_pXml, const char *_pName)
{
    int32_t iXmlIndex, iMatchIndex;
    const unsigned char *pMatch = NULL;
    const unsigned char *pXml = (const unsigned char *)_pXml;
    const unsigned char *pName = (const unsigned char *)_pName;

    // make sure record is valid
    if ((pXml == NULL) || (pXml[0] == 0))
    {
        return(NULL);
    }

    // make sure name is valid
    if ((pName == NULL) || (pName[0] == 0))
    {
        return(NULL);
    }

    // allow "." to specify current tag
    if ((*pName == '.') && (*pXml == '<'))
    {
        pName += 1;
        pXml += 1;
    }

    // scan the data
    while (pXml != NULL)
    {
        // locate the first marker
        for (; (pXml[0] != '<') && (pXml[0] != 0); ++pXml)
            ;

        // if we hit an xml decl then skip it
        if ((pXml[0] == '<') && (pXml[1] == '?'))
        {
            pXml = _XmlSkip(pXml, NULL);
            continue;
        }

        // if we hit a doctype/comment, then skip it
        if ((pXml[0] == '<') && (pXml[1] == '!'))
        {
            pXml = _XmlSkip(pXml, NULL);
            continue;
        }

        // if we hit an end tag, then search must be over
        if ((pXml[0] == '<') && (pXml[1] == '/'))
        {
            break;
        }

        // skip past leading <
        if (pXml[0] != 0)
        {
            ++pXml;
        }

        // see if we found the matching element
        for (iXmlIndex = iMatchIndex = 0; (pXml[iXmlIndex] != '\0') && (pName[iMatchIndex] != '\0'); ++iXmlIndex, ++iMatchIndex)
        {
            // wildcard match?
            if ((pName[iMatchIndex] == '%') && (pName[iMatchIndex+1] == '*'))
            {
                // consume all input until the subsequent character
                for (iMatchIndex += 2; (pXml[iXmlIndex] != '\0') && (pXml[iXmlIndex] != pName[iMatchIndex]); ++iXmlIndex)
                    ;
            }
            else if (pXml[iXmlIndex] != pName[iMatchIndex])
            {
                break;
            }
        }

        // handle end of data case
        if (pXml[iXmlIndex] == '\0')
        {
            break;
        }

        // see if we matched
        if ((pXml[iXmlIndex] <= ' ') || (pXml[iXmlIndex] == '>') || (pXml[iXmlIndex] == '/'))
        {
            // handle leaf match
            if (pName[iMatchIndex] == '\0')
            {
                pMatch = pXml-1;
                break;
            }

            // handle node match
            if ((pName[iMatchIndex] == '.') && (pXml[iXmlIndex] != '/'))
            {
                // locate the end of the start tag
                for (; (pXml[0] != '\0') && (pXml[0] != '>'); ++pXml)
                    ;
                // see if this is the stop point
                if (pName[iMatchIndex+1] == '\0')
                {
                    pMatch = (*pXml ? pXml+1 : NULL);
                    break;
                }

                // search within this area recursively
                if (pXml[0] != '\0')
                {
                    pMatch = (const unsigned char *)XmlFind((const char *)pXml+1, (const char *)pName+iMatchIndex+1);
                }
                break;
            }

        }

        // skip past mismatch element
        pXml = _XmlSkip(pXml-1, NULL);
    }

    // point to matching data
    return((const char *)pMatch);
}


/*F************************************************************************************************/
/*!
    \Function XmlNext

    \Description
        Skip to next element with same name as current

    \Input _pXml    - pointer to element start

    \Output         - Pointer to next element start

    \Version 01/30/2002 (GWS)
*/
/************************************************************************************************F*/
const char *XmlNext(const char *_pXml)
{
    int32_t iIndex;
    const unsigned char *pMatch;
    const unsigned char *pXml = (const unsigned char *)_pXml;

    // make sure we are at element start
    for (; (*pXml != 0) && (*pXml != '<'); ++pXml)
        ;

    // locate next tag that matches this one
    for (pMatch = _XmlSkip(pXml, NULL); pMatch != NULL; pMatch = _XmlSkip(pMatch, NULL))
    {
        // skip to the element start
        for (; (*pMatch != 0) && (*pMatch != '<'); ++pMatch)
            ;

        // see if we found next element of same name
        for (iIndex = 0; (pMatch[iIndex] > ' ') && (pMatch[iIndex] != '>') && (pXml[iIndex] > ' ') && (pXml[iIndex] != '>') && (pMatch[iIndex] == pXml[iIndex]); ++iIndex)
            ;

        // check for complete match
        if (((pMatch[iIndex] <= ' ') || (pMatch[iIndex] == '>')) && ((pXml[iIndex] <= ' ') || (pXml[iIndex] == '>')))
        {
            break;
        }
    }

    // return the match
    return((const char *)pMatch);
}


/*F************************************************************************************************/
/*!
    \Function XmlStep

    \Description
        Step over tag (regardless of the tab being a start tag or a end tag).

    \Input *_pXml       - pointer to tag opening character

    \Output
        const char *    - pointer to byte following tag closing character or NULL if at end of document

    \Version 09/16/2010 (jbrookes)
*/
/************************************************************************************************F*/
const char *XmlStep(const char *_pXml)
{
    const unsigned char *pXml = (const unsigned char *)_pXml;

    // make sure we are at tag opening character
    for (; (*pXml != 0) && (*pXml != '<'); ++pXml)
        ;

    // step to tag closing character
    for (; (*pXml != '\0') && (*pXml != '>'); ++pXml)
        ;

    // skip past end, return NULL if at end of document
    return(((*pXml != '\0') && (*(pXml+1) != '\0')) ? (const char *)pXml + 1 : NULL);
}

/*F************************************************************************************************/
/*!
    \Function XmlContentGetString

    \Description
        Return element contents as a string

    \Input pXml    - pointer to xml document
    \Input _pBuffer - string output buffer
    \Input iLength  - length of output buffer
    \Input pDefault - default value if (pXml == NULL)

    \Output
        int32_t     - length of string (-1=nothing copied)

    \Version 01/30/2002 (GWS)
*/
/************************************************************************************************F*/
int32_t XmlContentGetString(const char *pXml, char *_pBuffer, int32_t iLength, const char *pDefault)
{
    int32_t iLen;
    const unsigned char *pData;
    unsigned char *pBuffer = (unsigned char *)_pBuffer;
    int32_t bInCDataSection;

    // validate buffer
    if ((pBuffer == NULL) || (iLength < 1))
    {
        return(-1);
    }

    // locate the content area
    pData = _XmlContentFind(pXml);
    if (pData != NULL)
    {
        // skip leading white space
        while ((*pData != 0) && (*pData <= ' '))
        {
            ++pData;
        }

        // Skip CDATA header, if any
        bInCDataSection = _XmlSkipCDataHeader(pData, &pData);

        // convert the string
        for (iLen = 1; (iLen < iLength) && (*pData != 0); ++iLen)
        {
            if (bInCDataSection)
            {
                // Just copy the data until we reach the end trailer marker
                if (_XmlSkipCDataTrailer(pData, NULL))
                {
                    break;
                }

                *pBuffer++ = *pData++;
            }
            else
            {
                if (*pData == '<')
                {
                    break;
                }

                if (*pData == '&')
                {
                    pData = _XmlContentChar(pData+1, pBuffer++);
                }
                else
                {
                    *pBuffer++ = *pData++;
                }
            }
        }
        // remove trailing white space
        while ((iLen > 1) && (pBuffer[-1] <= ' '))
        {
            --iLen;
            --pBuffer;
        }
        // terminate buffer
        *pBuffer = 0;
    }
    else if (pDefault != NULL)
    {
        // copy over the string
        for (iLen = 1; (iLen < iLength) && (*pDefault != 0); ++iLen)
        {
            *pBuffer++ = *pDefault++;
        }
        // terminate buffer
        *pBuffer = 0;
    }
    else
    {
        // leave the old string
        iLen = 0;
    }

    // return length (without terminator)
    return(iLen-1);
}


/*F************************************************************************************************/
/*!
    \Function XmlContentGetInteger

    \Description
        Return element contents as an integer

    \Input pXml     - pointer to xml document
    \Input iDefault - default value if (pXml == NULL)

    \Output
        int32_t     - element contents as integer

    \Version 01/30/2002 (GWS)
*/
/************************************************************************************************F*/
int32_t XmlContentGetInteger(const char *pXml, int32_t iDefault)
{
    return((int32_t)XmlContentGetInteger64(pXml, iDefault));
}

/*F************************************************************************************************/
/*!
    \Function XmlContentGetInteger64

    \Description
        Return element contents as an integer (64-bit)

    \Input pXml     - pointer to xml document
    \Input iDefault - default value if (pXml == NULL)

    \Output
        int64_t     - element contents as integer
*/
/************************************************************************************************F*/
int64_t XmlContentGetInteger64(const char *pXml, int64_t iDefault)
{
    int32_t iSign = 1;
    uint64_t uNumber;

    const uint8_t *pData;

    // locate the content area
    if ((pData = _XmlContentFind(pXml)) == NULL)
    {
        return(iDefault);
    }

    // skip leading whitespace
    while ((*pData != 0) && (*pData <= ' '))
    {
        ++pData;
    }

    // check for a sign value
    if (*pData == '+')
    {
        iSign = 1;
        ++pData;
    }
    if (*pData == '-')
    {
        iSign = -1;
        ++pData;
    }
    
    // parse the number
    for (uNumber = 0; (*pData >= '0') && (*pData <= '9'); ++pData)
    {
        uNumber = (uNumber * 10) + (*pData & 15);
    }

    // return final number
    return (iSign*uNumber);
}

/*F************************************************************************************************/
/*!
    \Function XmlContentGetToken

    \Description
        Return element contents as a token (a packed sequence of characters)

    \Input pXml     - pointer to xml document
    \Input iDefault - default value if (pXml == NULL)

    \Output
        int32_t     - element contents as token

    \Version 01/30/2002 (GWS)
*/
/************************************************************************************************F*/
int32_t XmlContentGetToken(const char *pXml, int32_t iDefault)
{
    int32_t iToken = '    ';
    const unsigned char *pData;

    // locate the content area
    if ((pData = _XmlContentFind(pXml)) == NULL)
    {
        return(iDefault);
    }

    // skip leading white space
    while ((*pData != 0) && (*pData <= ' '))
    {
        ++pData;
    }

    // parse the number
    while ((*pData > ' ') && (*pData != '<'))
    {
        iToken = (iToken << 8) | *pData++;
    }

    // return the token
    return(iToken);
}


/*F************************************************************************************************/
/*!
    \Function XmlContentGetDate

    \Description
        Return epoch seconds for a date

    \Input pXml     - pointer to xml document
    \Input uDefault - default value if (pXml == NULL)

    \Output
        uint32_t    - element contents as epoch seconds

    \Version 01/30/2002 (GWS)
*/
/************************************************************************************************F*/
uint32_t XmlContentGetDate(const char *pXml, uint32_t uDefault)
{
    struct tm tm;
    const unsigned char *pData;

    // locate the content area
    if ((pData = _XmlContentFind(pXml)) == NULL)
    {
        return(uDefault);
    }

    // skip leading white space
    while ((*pData != 0) && (*pData <= ' '))
    {
        ++pData;
    }

    // set the unused fields
    tm.tm_isdst = -1;
    tm.tm_wday = 0;
    tm.tm_yday = 0;

    // extract the date
    pData = _ParseNumber(pData, &tm.tm_year);
    if ((*pData == '.') || (*pData == '-'))
        ++pData;
    pData = _ParseNumber(pData, &tm.tm_mon);
    if ((*pData == '.') || (*pData == '-'))
        ++pData;
    pData = _ParseNumber(pData, &tm.tm_mday);
    if ((*pData == ' ') || (*pData == 'T'))
        ++pData;
    pData = _ParseNumber(pData, &tm.tm_hour);
    if (*pData == ':')
        ++pData;
    pData = _ParseNumber(pData, &tm.tm_min);
    if (*pData == ':')
        ++pData;
    _ParseNumber(pData, &tm.tm_sec);

    // validate the fields
    if ((tm.tm_year < 1970) || (tm.tm_year > 2099) || (tm.tm_mon < 1) || (tm.tm_mon > 12) || (tm.tm_mday < 1) || (tm.tm_mday > 31))
    {
        return(uDefault);
    }
    if ((tm.tm_hour < 0) || (tm.tm_hour > 23) || (tm.tm_min < 0) || (tm.tm_min > 59) || (tm.tm_sec < 0) || (tm.tm_sec > 61))
    {
        return(uDefault);
    }

    // return epoch time
    tm.tm_mon -= 1;
    tm.tm_year -= 1900;
    return((uint32_t)ds_timetosecs(&tm));
}

/*F************************************************************************************************/
/*!
    \Function XmlContentGetAddress

    \Description
        Parse element contents as an internet dot-notation address and return as an integer.

    \Input pXml     - pointer to xml document
    \Input iDefault - default value if (pXml == NULL)

    \Output
        int32_t     - element contents as integer

    \Version 10/07/2005 (JLB)
*/
/************************************************************************************************F*/
int32_t XmlContentGetAddress(const char *pXml, int32_t iDefault)
{
    const unsigned char *pData;
    uint32_t iAddr, iQuad, iValue;

    // locate the content area
    if ((pData = _XmlContentFind(pXml)) == NULL)
    {
        return(iDefault);
    }

    // parse address
    for (iAddr = iQuad = 0; iQuad < 4; iQuad++, pData++)
    {
        // parse current digit
        for (iValue = 0; (*pData >= '0') && (*pData <= '9'); pData++)
        {
            iValue = (iValue*10) + (*pData & 15);
        }
        // verify digit
        if ((iQuad < 3) && (*pData != '.'))
        {
            iAddr = iDefault;
            break;
        }
        // accumulate digit in address
        iAddr <<= 8;
        iAddr |= iValue;
    }

    // return to caller
    return(iAddr);
}

/*F************************************************************************************************/
/*!
    \Function XmlContentGetBinary

    \Description
        Return binary encoded data

    \Input pXml     - pointer to xml document
    \Input pBuffer  - pointer to buffer to copy binary data to
    \Input iLength  - length of buffer pointed to by pBuffer

    \Output
        int32_t     - negative if error, length otherwise

    \Version 01/30/2002 (GWS)
*/
/************************************************************************************************F*/
int32_t XmlContentGetBinary(const char *pXml, char *pBuffer, int32_t iLength)
{
    int32_t iCount;
    const unsigned char *pData;

    // locate the content area
    if ((pData = _XmlContentFind(pXml)) == NULL)
    {
        return(0);
    }

    // skip leading white space
    while ((*pData != 0) && (*pData <= ' '))
    {
        ++pData;
    }

    // see if they just want the length
    if (pBuffer == NULL)
    {
        for (iCount = 0; (pData[0] >= '0') && (pData[1] >= '0'); pData += 2)
        {
            ++iCount;
        }
        return(iCount);
    }

    // make sure buffer has a valid length
    if (iLength < 0)
    {
        return(-1);
    }

    // attempt to copy over data
    for (iCount = 0; (iCount < iLength) && (pData[0] >= '0') && (pData[1] >= '0'); ++iCount)
    {
        *pBuffer++ = _Xml_TopDecode[pData[0]] | _Xml_BtmDecode[pData[1]];
        pData += 2;
    }

    // return the length
    return(iCount);
}

/*F************************************************************************************************/
/*!
    \Function XmlAttribGetString

    \Description
        Return element attribute as a string

    \Input pXml     - pointer to xml element
    \Input pAttrib  - name of attribute
    \Input pBuffer  - output string buffer
    \Input iLength  - length of string buffer
    \Input pDefault - default string if (pXml == NULL || pAttrib not found)

    \Output
        int32_t     - length of result string (-1=nothing copied)

    \Version 01/30/2002 (GWS)
*/
/************************************************************************************************F*/
int32_t XmlAttribGetString(const char *pXml, const char *pAttrib, char *pBuffer, int32_t iLength, const char *pDefault)
{
    int32_t iLen;
    unsigned char uTerm;
    const unsigned char *pData;

    // validate buffer
    if ((pBuffer == NULL) || (iLength < 1))
    {
        return(-1);
    }

    // locate the content area
    pData = _XmlAttribFind(pXml, pAttrib);
    if (pData != NULL)
    {
        // skip leading white space
        while ((*pData != 0) && (*pData <= ' '))
        {
            ++pData;
        }

        // see if there is a terminator
        if ((*pData == '"') || (*pData == '\''))
        {
            uTerm = *pData++;
        }
        else
        {
            uTerm = 0;
        }

        // convert the string
        for (iLen = 1; (iLen < iLength) && (*pData != uTerm) && (*pData != 0) && (*pData != '>'); ++iLen)
        {
            if (*pData == '&')
            {
                pData = _XmlContentChar(pData+1, (unsigned char *)pBuffer++);
            }
            else
            {
                *pBuffer++ = *pData++;
            }
        }
        // terminate buffer
        *pBuffer = 0;
    }
    else if (pDefault != NULL)
    {
        // copy over the string
        for (iLen = 1; (iLen < iLength) && (*pDefault != 0); ++iLen)
        {
            *pBuffer++ = *pDefault++;
        }
        // terminate buffer
        *pBuffer = 0;
    }
    else
    {
        // leave the old value
        iLen = 0;
    }

    // return length (without terminator)
    return(iLen-1);
}


/*F************************************************************************************************/
/*!
    \Function XmlAttribGetInteger

    \Description
        Return element attribute as an integer

    \Input pXml     - pointer to xml element
    \Input pAttrib  - name of attribute
    \Input iDefault - default value if (pXml == NULL || pAttrib not found)

    \Output
        int32_t     - attibute value as integer

    \Version 01/30/2002 (GWS)
*/
/************************************************************************************************F*/
int32_t XmlAttribGetInteger(const char *pXml, const char *pAttrib, int32_t iDefault)
{
    return((int32_t)XmlAttribGetInteger64(pXml, pAttrib, iDefault));
}

/*F************************************************************************************************/
/*!
    \Function XmlAttribGetInteger64

    \Description
        Return element attribute as an integer (64-bit)

    \Input pXml     - pointer to xml element
    \Input pAttrib  - name of attribute
    \Input iDefault - default value if (pXml == NULL || pAttrib not found)

    \Output
        int64_t     - attibute value as integer
*/
/************************************************************************************************F*/
int64_t XmlAttribGetInteger64(const char *pXml, const char *pAttrib, int64_t iDefault)
{
    int32_t iSign = 1;
    uint64_t uNumber;
    const unsigned char *pData;

    // locate the content area
    pData = _XmlAttribFind(pXml, pAttrib);
    if (pData == NULL)
    {
        return(iDefault);
    }

    // skip leading white space
    while ((*pData != 0) && (*pData <= ' '))
    {
        ++pData;
    }

    // skip terminator if present
    if ((*pData == '"') || (*pData == '\''))
    {
        ++pData;
    }

    // check for a sign value
    if (*pData == '+')
    {
        iSign = 1;
        ++pData;
    }
    if (*pData == '-')
    {
        iSign = -1;
        ++pData;
    }

    // parse the number
    for (uNumber = 0; (*pData >= '0') && (*pData <= '9'); ++pData)
    {
        uNumber = (uNumber * 10) + (*pData & 15);
    }

    // check for symbol true
    if (((pData[0]|32) == 't') && ((pData[1]|32) == 'r') && ((pData[2]|32) == 'u') && ((pData[3]|32) == 'e'))
    {
        iSign = 1;
        uNumber = 1;
    }

    // check for symbol false
    if (((pData[0]|32) == 'f') && ((pData[1]|32) == 'a') && ((pData[2]|32) == 'l') && ((pData[3]|32) == 's') && ((pData[4]|32) == 'e'))
    {
        iSign = 1;
        uNumber = 0;
    }

    // return final value
    return(iSign*uNumber);
}


/*F************************************************************************************************/
/*!
    \Function XmlAttribGetToken

    \Description
        Return element attribute as a token

    \Input pXml     - pointer to xml element
    \Input pAttrib  - name of attribute
    \Input iDefault - default value if (pXml == NULL || pAttrib not found)

    \Output
        int32_t     - attribute value as a token

    \Version 01/30/2002 (GWS)
*/
/************************************************************************************************F*/
int32_t XmlAttribGetToken(const char *pXml, const char *pAttrib, int32_t iDefault)
{
    int32_t iToken = '    ';
    unsigned char uTerm = 0;
    const unsigned char *pData;

    // locate the content area
    pData = _XmlAttribFind(pXml, pAttrib);
    if (pData == NULL)
    {
        return(iDefault);
    }

    // skip leading white space
    while ((*pData != 0) && (*pData <= ' '))
    {
        ++pData;
    }

    // skip terminator if present
    if ((*pData == '"') || (*pData == '\''))
    {
        uTerm = *pData++;
    }

    // parse the number
    while ((*pData > ' ') && (*pData != uTerm) && (*pData != '>') && (*pData != 0))
    {
        iToken = (iToken << 8) | *pData++;
    }

    // return final value
    return(iToken);
}


/*F************************************************************************************************/
/*!
    \Function XmlAttribGetDate

    \Description
        Return epoch seconds for a date

    \Input pXml     - pointer to xml element
    \Input pAttrib  - name of attribute
    \Input uDefault - default value if (pXml == NULL || pAttrib not found)

    \Output
        uint32_t    - attribute value as epoch seconds

    \Version 01/30/2002 (GWS)
*/
/************************************************************************************************F*/
uint32_t XmlAttribGetDate(const char *pXml, const char *pAttrib, uint32_t uDefault)
{
    struct tm tm;
    const unsigned char *pData;

    // locate the content area
    pData = _XmlAttribFind(pXml, pAttrib);
    if (pData == NULL)
    {
        return(uDefault);
    }

    // skip leading white space
    while ((*pData != 0) && (*pData <= ' '))
    {
        ++pData;
    }

    // skip terminator if present
    if ((*pData == '"') || (*pData == '\''))
    {
        ++pData;
    }

    // set the unused fields
    tm.tm_isdst = -1;
    tm.tm_wday = 0;
    tm.tm_yday = 0;

    // extract the date
    pData = _ParseNumber(pData, &tm.tm_year);
    if ((*pData == '.') || (*pData == '-'))
        ++pData;
    pData = _ParseNumber(pData, &tm.tm_mon);
    if ((*pData == '.') || (*pData == '-'))
        ++pData;
    pData = _ParseNumber(pData, &tm.tm_mday);
    if ((*pData == ' ') || (*pData == 'T'))
        ++pData;
    pData = _ParseNumber(pData, &tm.tm_hour);
    if (*pData == ':')
        ++pData;
    pData = _ParseNumber(pData, &tm.tm_min);
    if (*pData == ':')
        ++pData;
    _ParseNumber(pData, &tm.tm_sec);

    // validate the fields
    if ((tm.tm_year < 1970) || (tm.tm_year > 2099) || (tm.tm_mon < 1) || (tm.tm_mon > 12) || (tm.tm_mday < 1) || (tm.tm_mday > 31))
    {
        return(uDefault);
    }
    if ((tm.tm_hour < 0) || (tm.tm_hour > 23) || (tm.tm_min < 0) || (tm.tm_min > 59) || (tm.tm_sec < 0) || (tm.tm_sec > 61))
    {
        return(uDefault);
    }

    // return epoch time
    tm.tm_mon -= 1;
    tm.tm_year -= 1900;
    return((uint32_t)ds_timetosecs(&tm));
}


//////////////////////////////////////////////////////////////////////////////////////////


/*F************************************************************************************************/
/*!
    \Function XmlConvEpoch2Date

    \Description
        Convert epoch to date components

    \Input uEpoch   - epoch to convert to date components
    \Input pYear    - pointer to storage for year
    \Input pMonth   - pointer to storage for month
    \Input pDay     - pointer to storage for day
    \Input pHour    - pointer to storage for hour
    \Input pMinute  - pointer to storage for minute
    \Input pSecond  - pointer to storage for second

    \Output
        int32_t     - negative=error, zero=success

    \Version 01/30/2002 (GWS)
*/
/************************************************************************************************F*/
int32_t XmlConvEpoch2Date(uint32_t uEpoch, int32_t *pYear, int32_t *pMonth, int32_t *pDay, int32_t *pHour, int32_t *pMinute, int32_t *pSecond)
{
    int32_t iResult = -1;
    struct tm tm;

    // convert to calendar time
    if (ds_secstotime(&tm, uEpoch) != NULL)
    {
        // return the fields
        if (pYear != NULL)
            *pYear = tm.tm_year;
        if (pMonth != NULL)
            *pMonth = tm.tm_mon;
        if (pDay != NULL)
            *pDay = tm.tm_mday;
        if (pHour != NULL)
            *pHour = tm.tm_hour;
        if (pMinute != NULL)
            *pMinute = tm.tm_min;
        if (pSecond != NULL)
            *pSecond = tm.tm_sec;
        // return success
        iResult = 0;
    }

    return(iResult);
}

#if DIRTYCODE_LOGGING
/*F*************************************************************************************/
/*!
    \Function XmlPrintFmtCode

    \Description
        Takes a as input an XML buffer, and prints out a "nicely" formatted version of
        the XML data to debug output.  Each line of output is appended to text generated
        by the format string and arguments.

    \Input *pXml        - xml to print
    \Input *pFormat     - printf style format string
    \Input ...          - variable-argument list

    \Version 09/15/2010 (jbrookes)
*/
/*************************************************************************************F*/
void XmlPrintFmtCode(const char *pXml, const char *pFormat, ...)
{
    char strPrefix[128];
    va_list pFmtArgs;

    // format prefix string
    va_start(pFmtArgs, pFormat);
    ds_vsnzprintf(strPrefix, sizeof(strPrefix), pFormat, pFmtArgs);
    va_end(pFmtArgs);

    _XmlPrintFmt((const unsigned char *)pXml, strPrefix);
}
#endif
