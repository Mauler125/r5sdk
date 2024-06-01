/*H*************************************************************************************/
/*!
    \File jsonformat.c

    \Description
         This module formats simple JSON, in a linear fashion, using a character buffer
         that the client provides.

    \Copyright
        Copyright (c) Electronic Arts 2012.

    \Notes
        \verbatim
        When JsonInit() is called, a 24-bytes header is added at the beginning of the
        client-provided buffer. The header is an ascii string that looks like this:
            <AAAAAAAABBBBBBBBCCDD />
                where:
                    AAAAAAAA is the offset field
                    BBBBBBBB is the buffer size field
                    CC is the indent field
                    DD is the flag field
        Those first 24 bytes are replaced by whitespaces when JsonFinish() is called.

        Required buffer capacity for json = ??? length(JSON) + 1.
        That means if length(buffer) <= length(JSON), buffer-overrun occurs.
        \endverbatim

    \Version 12/11/2012 (jbrookes) First Version, based on XmlFormat
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/util/jsonformat.h"

/*** Defines ***************************************************************************/

#define JSON_HEADER_LEN       (22)
#define JSON_MAX_TAG_LENGTH   (128)
#define JSON_MAX_FLOAT_LENGTH (128)
#define JSON_MAX_DATE_LENGTH  (32)

/*** Type Definitions ******************************************************************/

/*** Variables *************************************************************************/

static const uint8_t _Json_Hex2AsciiTable[16] = "0123456789abcdef";

// ascii->hex conversion table for 7bit ASCII characters
static const uint8_t _Json_Ascii2HexTable[128] =
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

/*! 7bit ASCII characters that should be encoded ([0..31], ", ?, \)
    characters that are represented with a 1 are escaped in long form
    (u00xy) whereas characters represented with a 2 are escaped with
    a single backslash */
static const uint8_t _Json_EncodeStringTable[128] =
{
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  1,  1,  // 00-0F
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 10-1F
    0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 20-2F
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  // 30-3F
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 40-4F
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 50-5F
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 60-6F
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  // 70-7F
};


static int32_t _iLastIndent; //$$TODO - clean this up (put it in table?)



/*** Private Functions *****************************************************************/


/*F*************************************************************************************/
/*!
    \Function _JsonGet8

    \Description
        Get 8bit value from given offset in buffer

    \Input *pJson   - pointer to start of buffer
    \Input iOffset  - offset within buffer to get value from

    \Output
        int32_t     - value

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t _JsonGet8(const char *pJson, int32_t iOffset)
{
    return((_Json_Ascii2HexTable[(int32_t)pJson[iOffset+0]] << 4) |
            _Json_Ascii2HexTable[(int32_t)pJson[iOffset+1]]);
}

/*F*************************************************************************************/
/*!
    \Function _JsonSet8

    \Description
        Save 8bit value in buffer

    \Input *pJson   - pointer to start of buffer
    \Input iOffset  - offset to save at
    \Input iValue   - value to save

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
static void _JsonSet8(char *pJson, int32_t iOffset, int32_t iValue)
{
    pJson[iOffset+0] = _Json_Hex2AsciiTable[(iValue >> 4) & 15];
    pJson[iOffset+1] = _Json_Hex2AsciiTable[(iValue >> 0) & 15];
}

/*F*************************************************************************************/
/*!
    \Function _JsonGet32

    \Description
        Get 32bit value from given offset in buffer

    \Input *pJson   - pointer to start of buffer
    \Input iOffset  - offset within buffer to get value from

    \Output
        int32_t     - value

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t _JsonGet32(char *pJson, int32_t iOffset)
{
    return((_Json_Ascii2HexTable[(int32_t)pJson[iOffset+0]] << 28) |
           (_Json_Ascii2HexTable[(int32_t)pJson[iOffset+1]] << 24) |
           (_Json_Ascii2HexTable[(int32_t)pJson[iOffset+2]] << 20) |
           (_Json_Ascii2HexTable[(int32_t)pJson[iOffset+3]] << 16) |
           (_Json_Ascii2HexTable[(int32_t)pJson[iOffset+4]] << 12) |
           (_Json_Ascii2HexTable[(int32_t)pJson[iOffset+5]] << 8) |
           (_Json_Ascii2HexTable[(int32_t)pJson[iOffset+6]] << 4) |
            _Json_Ascii2HexTable[(int32_t)pJson[iOffset+7]]);
}

/*F*************************************************************************************/
/*!
    \Function _JsonSet32

    \Description
        Save 32-bit value at given offset in buffer

    \Input *pJson   - pointer to start of buffer
    \Input iOffset  - offset into buffer to save
    \Input iValue   - value to save into buffer

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
static void _JsonSet32(char *pJson, int32_t iOffset, int32_t iValue)
{
    pJson[iOffset+0] = _Json_Hex2AsciiTable[(iValue>>28)&15];
    pJson[iOffset+1] = _Json_Hex2AsciiTable[(iValue>>24)&15];
    pJson[iOffset+2] = _Json_Hex2AsciiTable[(iValue>>20)&15];
    pJson[iOffset+3] = _Json_Hex2AsciiTable[(iValue>>16)&15];
    pJson[iOffset+4] = _Json_Hex2AsciiTable[(iValue>>12)&15];
    pJson[iOffset+5] = _Json_Hex2AsciiTable[(iValue>>8)&15];
    pJson[iOffset+6] = _Json_Hex2AsciiTable[(iValue>>4)&15];
    pJson[iOffset+7] = _Json_Hex2AsciiTable[(iValue>>0)&15];
}

/*F*************************************************************************************/
/*!
    \Function _JsonGetOffset

    \Description
        Get offset from buffer

    \Input *pJson   - pointer to start of buffer

    \Output
        int32_t     - offset

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t _JsonGetOffset(char *pJson)
{
    return(_JsonGet32(pJson, 1));
}

/*F*************************************************************************************/
/*!
    \Function _JsonGetLength

    \Description
        Get length from buffer

    \Input *pJson   - pointer to start of buffer

    \Output
        int32_t     - length

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t _JsonGetLength(char *pJson)
{
    return(_JsonGet32(pJson, 9));
}

/*F*************************************************************************************/
/*!
    \Function _JsonGetIndent

    \Description
        Get indent level from buffer

    \Input *pJson   - pointer to start of buffer

    \Output
        int32_t     - indent

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t _JsonGetIndent(char *pJson)
{
    return(_JsonGet8(pJson, 17));
}

/*F*************************************************************************************/
/*!
    \Function _JsonGetFlags

    \Description
        Get flags from buffer

    \Input *pJson   - pointer to start of buffer

    \Output
        int32_t     - flags

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t _JsonGetFlags(const char *pJson)
{
    return(_JsonGet8(pJson, 19));
}

/*F*************************************************************************************/
/*!
    \Function _JsonSetOffset

    \Description
        Save offset in buffer

    \Input *pJson   - pointer to start of buffer
    \Input iOffset  - offset to save in buffer

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
static void _JsonSetOffset(char *pJson, int32_t iOffset)
{
    _JsonSet32(pJson, 1, iOffset);
}

/*F*************************************************************************************/
/*!
    \Function _JsonSetLength

    \Description
        Save length in buffer

    \Input *pJson   - pointer to start of buffer
    \Input iLength  - length to save in buffer

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
static void _JsonSetLength(char *pJson, int32_t iLength)
{
    _JsonSet32(pJson, 9, iLength);
}

/*F*************************************************************************************/
/*!
    \Function _JsonSetIndent

    \Description
        Save indent level in buffer

    \Input *pJson   - pointer to start of buffer
    \Input iIndent  - indent level to save

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
static void _JsonSetIndent(char *pJson, int32_t iIndent)
{
    _JsonSet8(pJson, 17, iIndent);
}

/*F*************************************************************************************/
/*!
    \Function _JsonSetFlags

    \Description
        Save flags in buffer

    \Input *pJson   - pointer to start of buffer
    \Input iFlags   - flags

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
static void _JsonSetFlags(char *pJson, int32_t iFlags)
{
    _JsonSet8(pJson, 19, iFlags);
}

/*F*************************************************************************************/
/*!
    \Function _JsonValidHeader

    \Description
        Check whether buffer is initialized with a valid header.

    \Input *pBuffer - buffer to which JSON will be written

    \Output
        int32_t     - returns 1 if there is a valid header, 0 otherwise

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t _JsonValidHeader(const char *pBuffer)
{
    return((pBuffer[0] != '{') ? 0 : 1);
}

/*F*************************************************************************************/
/*!
    \Function _JsonOpenTag

    \Description
        Check whether there is an open tag in the buffer.

    \Input *pBuffer - buffer to which JSON will be written
    \Input iOffset  - current offset within the buffer

    \Output
        int32_t     - returns 1 if an open tag exists in the buffer, 0 otherwise

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t _JsonOpenTag(const char *pBuffer, int32_t iOffset)
{
    int32_t iBackPos;
    int32_t iBeginCount = 0;
    int32_t iEndCount = 0;

    for (iBackPos = (iOffset-1); iBackPos >= JSON_HEADER_LEN; iBackPos--)
    {
        // Count all closing tags (ended tag with no children will be counted as starting and closing tag)
        if (pBuffer[iBackPos] == '}')
        {
            iEndCount++;
            iBackPos--;
        }
        // Count all starting tags
        else if (pBuffer[iBackPos] == '{')
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
    \Function _JsonUpdateHeader

    \Description
        Update the JSON header to reflect the number of characters written. If the buffer
        was full, this function will update the header to indicate the full buffer and will
        return the JSON_ERR_FULL error code.

    \Input *pBuffer     - buffer to which json will be written
    \Input iNumChars    - number of characters written, -1 for buffer filled
    \Input iLength      - length of the buffer
    \Input iOffset      - current offset within the buffer
    \Input iIndent      - current indent level

    \Output
        int32_t         - returns JSON_ERR_FULL if buffer was filled, JSON_ERR_NONE otherwise

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t _JsonUpdateHeader(char *pBuffer, int32_t iNumChars, int32_t iLength, int32_t iOffset, int32_t iIndent)
{
    iOffset += iNumChars;

    // leave room for null character
    iLength -= 1;

    if ((iNumChars < 0) || (iOffset > iLength))
    {
        pBuffer[iLength] = '\0';
        iOffset = iLength;
        _JsonSetOffset(pBuffer, iOffset);
        return(JSON_ERR_FULL);
    }
    else
    {
        _iLastIndent = _JsonGetIndent(pBuffer);
        _JsonSetOffset(pBuffer, iOffset);
        _JsonSetIndent(pBuffer, iIndent);
        return(JSON_ERR_NONE);
    }
}

/*F****************************************************************************/
/*!
    \Function _JsonIndent

    \Description
        Add the appropriate level of whitespace to indent the current tag if
        the whitespace flag is enabled for the buffer.

    \Input *pBuffer     - buffer to which JSON will be written
    \Input iLength      - length of the buffer
    \Input iOffset      - current offset within the buffer
    \Input iIndent      - current indent level
    \Input iFlags       - flags

    \Output
        int32_t         - negative=buffer overrun, zero/positive=number of characters written

    \Version 12/11/2012 (jbrookes)
*/
/****************************************************************************F*/
static int32_t _JsonIndent(char *pBuffer, int32_t iLength, int32_t iOffset, int32_t iIndent, int32_t iFlags)
{
    int32_t iNumChars = -1;

    #if DIRTYCODE_LOGGING
    if (iIndent < 0)
    {
        NetPrintf(("jsonformat: warning -- invalid indent level (%d) in _JsonIndent\n", iIndent));
    }
    #endif

    if ((iFlags & JSON_FL_WHITESPACE) == 0)
    {
        iNumChars = 0;
    }
    else if (iIndent <= 0)
    {
        if ((iLength - iOffset) > 1)
        {
            iNumChars = ds_snzprintf(pBuffer + iOffset, iLength - iOffset, "\n");
        }
    }
    else
    {
        if ((iLength - iOffset) > ((iIndent * 2) + 1))
        {
            iNumChars = ds_snzprintf(pBuffer + iOffset, iLength - iOffset, "\n%*c", iIndent * 2, ' ');
        }
    }

    return(iNumChars);
}

#if 0 //UNUSED?
/*F*************************************************************************************/
/*!
    \Function _JsonFormatInsert

    \Description
        Insert a preformatted item

    \Input *pBuffer - the JSON buffer
    \Input *pValue  - raw string to insert (must be correctly preformatted)

    \Output
        int32_t     - return code.

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t _JsonFormatInsert(char *pBuffer, const char *pValue)
{
    int32_t iWidth, iCount;
    int32_t iOffset, iLength, iIndent, iFlags;

    // make sure there is a header and extract fields
    if (!_JsonValidHeader(pBuffer))
        return(JSON_ERR_UNINIT);

    iOffset = _JsonGetOffset(pBuffer);
    iLength = _JsonGetLength(pBuffer);
    iIndent = _JsonGetIndent(pBuffer);
    iFlags = _JsonGetFlags(pBuffer);

    // there must be an open tag in the buffer
    if (!_JsonOpenTag(pBuffer, iOffset))
    {
        return(JSON_ERR_NOT_OPEN);
    }

    // check if there's enough room for the insertion to complete successfully
    iWidth = (int32_t)strlen(pValue);
    // figure out indent size
    iWidth += ((pValue[0] == '{') && (iFlags & JSON_FL_WHITESPACE)) ? (1 + iIndent * 2) : 0;
    // we must be able to insert completely
    if ((iLength - iOffset) <= iWidth)
    {
        return(JSON_ERR_FULL);
    }

    // buffer is good, now start to determine the insertion type
    // 1. <elem>value<elem>, handle indent if necessary
    if (pValue[0] == '{')
    {
        if (iFlags & JSON_FL_WHITESPACE)
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
        if (!_JsonOpenForAttr(pBuffer, iOffset))
        {
            return(JSON_ERR_ATTR_POSITION);
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
    _JsonSetOffset(pBuffer, iOffset);
    return(JSON_ERR_NONE);
}
#endif

/*F*************************************************************************************/
/*!
    \Function _JsonEncodeString

    \Description
        Encode a string as needed for reserved characters

    \Input *pBuffer - the JSON buffer
    \Input iLength  - number of eight-bit characters available in buffer
    \Input *_pSrc   - source string to add to buffer

    \Output
        int32_t     - negative=not enough buffer, zero/positive=encoded length.

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t _JsonEncodeString(char *pBuffer, int32_t iLength, const char *_pSrc)
{
    char * const pBackup = pBuffer;
    const uint8_t *pSource;
    int32_t iRemain, iEncodeType;

    if (_pSrc == NULL)
    {
        _pSrc = "null";
    }
    pSource = (const uint8_t *)_pSrc;

    // encode the string
    for (iRemain = iLength; (iRemain > 1) && (*pSource != '\0'); ++pSource)
    {
        if ((*pSource < 128) && ((iEncodeType = _Json_EncodeStringTable[*pSource]) != 0))
        {
            if ((iEncodeType == 2) && (iRemain > 6))
            {
                // hex encode all out of range values
                *pBuffer++ = '\\';
                *pBuffer++ = 'u';
                *pBuffer++ = '0';
                *pBuffer++ = '0';
                *pBuffer++ = _Json_Hex2AsciiTable[(*pSource)>>4];
                *pBuffer++ = _Json_Hex2AsciiTable[(*pSource)&0xF];
                iRemain -= 6;
            }
            else if ((iEncodeType == 1) && (iRemain > 1))
            {
                *pBuffer++ = '\\';
                *pBuffer++ = *pSource;
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

/*F*************************************************************************************/
/*!
    \Function _JsonAddStr

    \Description
        Adds a string element

    \Input *pBuffer     - JSON buffer
    \Input *pElemName   - name of the element (may be null)
    \Input *pValue      - value of the element
    \Input bQuote       - TRUE if value should be quoted

    \Output
        int32_t       - JSON_ERR_*

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t _JsonAddStr(char *pBuffer, const char *pElemName, const char *pValue, uint8_t bQuote)
{
    int32_t iOffset, iLength, iNumChars, iIndent, iFlags, iTempLength;

    if (!_JsonValidHeader(pBuffer))
    {
        return(JSON_ERR_UNINIT);
    }

    iOffset = _JsonGetOffset(pBuffer);
    iLength = _JsonGetLength(pBuffer);
    iIndent = _JsonGetIndent(pBuffer);
    iFlags = _JsonGetFlags(pBuffer);

    // there must be an open tag in the buffer
    if (_JsonOpenTag(pBuffer, iOffset))
    {
        iNumChars = 0;
        if (iIndent <= _iLastIndent)
        {
            if ((iTempLength = ds_snzprintf(pBuffer+iOffset, iLength-iOffset, ",")) <= 0)
            {
                pBuffer[iOffset] = '\0'; // reset all changes
                return(JSON_ERR_FULL);
            }
            iNumChars = iTempLength;
        }

        if ((iTempLength = _JsonIndent(pBuffer+iNumChars, iLength-iNumChars, iOffset, iIndent, iFlags)) < 0)
        {
            return(JSON_ERR_FULL);
        }
        iNumChars += iTempLength;

        // add element name
        if (pElemName != NULL)
        {
            if ((iTempLength = ds_snzprintf(pBuffer+iOffset+iNumChars, iLength-iOffset-iNumChars, "\"%s\":", pElemName)) <= 0)
            {
                pBuffer[iOffset] = '\0'; // reset all changes
                return(JSON_ERR_FULL);
            }
            iNumChars += iTempLength;
        }

        // add leading quote
        if (bQuote)
        {
            if ((iTempLength = ds_snzprintf(pBuffer+iOffset+iNumChars, iLength-iOffset-iNumChars, "\"")) <= 0)
            {
                pBuffer[iOffset] = '\0'; // reset all changes
                return(JSON_ERR_FULL);
            }
            iNumChars += iTempLength;
        }

        // add element value
        if ((iTempLength = _JsonEncodeString(pBuffer+iOffset+iNumChars, iLength-iOffset-iNumChars, pValue)) < 0)
        {
            pBuffer[iOffset] = '\0'; // reset all changes
            return(JSON_ERR_FULL);
        }
        iNumChars += iTempLength;

        // add trailing quote
        if (bQuote)
        {
            if ((iTempLength = ds_snzprintf(pBuffer+iOffset+iNumChars, iLength-iOffset-iNumChars, "\"")) <= 0)
            {
                pBuffer[iOffset] = '\0'; // reset all changes
                return(JSON_ERR_FULL);
            }
            iNumChars += iTempLength;
        }

        // all done successfully
        return(_JsonUpdateHeader(pBuffer, iNumChars, iLength, iOffset, iIndent));
    }
    else
    {
        return(JSON_ERR_NOT_OPEN);
    }
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function JsonInit

    \Description
        Initialize the Json Buffer. This MUST be called prior to any other calls.
        Client can allocate the buffer on stack, but should not access it directly.
        (see JsonData()).

    \Input *pBuffer - buffer to which JSON will be written.
    \Input iBufLen  – size of buffer passed in.
    \Input uFlags   - encoding flags (JSONFORMAT_FL_xxx)

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
void JsonInit(char *pBuffer, int32_t iBufLen, unsigned char uFlags)
{
    int32_t iOffset;

    ds_memclr(pBuffer, iBufLen);
    if (iBufLen > JSON_HEADER_LEN)
    {
        iOffset = ds_snzprintf(pBuffer, iBufLen - 1, "{00000000000000000000}{");
        _JsonSetOffset(pBuffer, iOffset);
        _JsonSetLength(pBuffer, iBufLen);
        _JsonSetFlags(pBuffer, uFlags);
        _JsonSetIndent(pBuffer, 1);
        _iLastIndent = 0;
    }
}

/*F*************************************************************************************/
/*!
    \Function JsonBufSizeIncrease

    \Description
        Notify the jsonformat API that the buffer size was increased.

    \Input *pBuffer     - buffer to which JSON is being written.
    \Input iNewBufLen   – new size for that buffer

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
void JsonBufSizeIncrease(char *pBuffer, int32_t iNewBufLen)
{
    _JsonSetLength(pBuffer, iNewBufLen);
}

/*F*************************************************************************************/
/*!
    \Function JsonFinish

    \Description
        Signal completion of output to this buffer.  Clear the JSON API hidden
        data.

    \Input *pBuffer – buffer to which JSON was written.

    \Output char*   - pointer to real JSON data (skipping past header)

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
char *JsonFinish(char *pBuffer)
{
    if (_JsonValidHeader(pBuffer))
    {
        int32_t iOffset = _JsonGetOffset(pBuffer);
        int32_t iBufLen = _JsonGetLength(pBuffer);
        int32_t iFlags  = _JsonGetFlags(pBuffer);

        // add final close tag
        if (iFlags == JSON_FL_WHITESPACE)
        {
            ds_snzprintf(pBuffer + iOffset, iBufLen - iOffset, "\n}");
        }
        else
        {
            ds_snzprintf(pBuffer + iOffset, iBufLen - iOffset, "}");
        }

        // remove header
        ds_memset(pBuffer, ' ', JSON_HEADER_LEN);
        pBuffer += JSON_HEADER_LEN;
    }
    return(pBuffer);
}

/*F*************************************************************************************/
/*!
    \Function JsonObjectStart

    \Description
        Start a JSON object, to which you can add other objects or elements.  Must be
        ended with JsonObjectEnd.

    \Input *pBuffer - JSON buffer
    \Input *pName   - name of the object (may be null)

    \Output
        int32_t     - JSON_ERR_*

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
int32_t JsonObjectStart(char *pBuffer, const char *pName)
{
    int32_t iOffset, iLength, iNumChars, iIndent, iFlags;

    if (!_JsonValidHeader(pBuffer))
    {
        return(JSON_ERR_UNINIT);
    }

    iOffset = _JsonGetOffset(pBuffer);
    iLength = _JsonGetLength(pBuffer);
    iIndent = _JsonGetIndent(pBuffer);
    iFlags = _JsonGetFlags(pBuffer);

    if (iIndent <= _iLastIndent)
    {
        if ((iNumChars = ds_snzprintf(pBuffer+iOffset, iLength-iOffset, ",")) <= 0)
        {
            pBuffer[iOffset] = '\0'; // reset all changes
            return(JSON_ERR_FULL);
        }
        iOffset += iNumChars;
    }

    if ((iNumChars = _JsonIndent(pBuffer, iLength, iOffset, iIndent, iFlags)) < 0)
    {
        return(JSON_ERR_FULL);
    }
    iOffset += iNumChars;
    
    if (pName != NULL)
    {
        if ((iNumChars = ds_snzprintf(pBuffer + iOffset, iLength - iOffset, "\"%s\":", pName)) < 0)
        {
            return(JSON_ERR_FULL);
        }
        iOffset += iNumChars;
    }

    if ((iNumChars = _JsonIndent(pBuffer, iLength, iOffset, iIndent++, iFlags)) < 0)
    {
        return(JSON_ERR_FULL);
    }
    iOffset += iNumChars;

    if ((iNumChars = ds_snzprintf(pBuffer + iOffset, iLength - iOffset, "{")) < 0)
    {
        return(JSON_ERR_FULL);
    }
    return(_JsonUpdateHeader(pBuffer, iNumChars, iLength, iOffset, iIndent));
}

/*F*************************************************************************************/
/*!
    \Function JsonObjectEnd

    \Description
        End the current object -- must have an outstanding open tag.

    \Input *pBuffer - JSON buffer.

    \Output
        int32_t     - JSON_ERR_*

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
int32_t JsonObjectEnd(char *pBuffer)
{
    int32_t iOffset, iLength, iNumChars;
    int32_t iIndent, iFlags;

    if (!_JsonValidHeader(pBuffer))
    {
        return(JSON_ERR_UNINIT);
    }

    iOffset = _JsonGetOffset(pBuffer);
    iLength = _JsonGetLength(pBuffer);
    iFlags  = _JsonGetFlags(pBuffer);
    iIndent = _JsonGetIndent(pBuffer) - 1;

    if ((iNumChars = _JsonIndent(pBuffer, iLength, iOffset, iIndent, iFlags)) < 0)
    {
        return(JSON_ERR_FULL);
    }
    iOffset += iNumChars;

    iNumChars = ds_snzprintf(pBuffer+iOffset, iLength-iOffset, "}");
    return(_JsonUpdateHeader(pBuffer, iNumChars, iLength, iOffset, iIndent));
}

/*F*************************************************************************************/
/*!
    \Function JsonArrayStart

    \Description
        Start a JSON array, to which you can add other objects or elements.  Must be
        ended with JsonArrayEnd.

    \Input *pBuffer - JSON buffer
    \Input *pName   - name of the array (may be null)

    \Output
        int32_t     - JSON_ERR_*

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
int32_t JsonArrayStart(char *pBuffer, const char *pName)
{
    int32_t iOffset, iLength, iNumChars, iIndent, iFlags;

    if (!_JsonValidHeader(pBuffer))
    {
        return(JSON_ERR_UNINIT);
    }

    iOffset = _JsonGetOffset(pBuffer);
    iLength = _JsonGetLength(pBuffer);
    iIndent = _JsonGetIndent(pBuffer);
    iFlags = _JsonGetFlags(pBuffer);

    iNumChars = 0;
    if (iIndent <= _iLastIndent)
    {
        int32_t iTempLength;
        if ((iTempLength = ds_snzprintf(pBuffer+iOffset, iLength-iOffset, ",")) <= 0)
        {
            pBuffer[iOffset] = '\0'; // reset all changes
            return(JSON_ERR_FULL);
        }
        iNumChars = iTempLength;
    }
    iOffset += iNumChars;
    if ((iNumChars = _JsonIndent(pBuffer, iLength, iOffset, iIndent, iFlags)) < 0)
    {
        return(JSON_ERR_FULL);
    }
    iOffset += iNumChars;
    
    if (pName != NULL)
    {
        if ((iNumChars = ds_snzprintf(pBuffer + iOffset, iLength - iOffset, "\"%s\":", pName)) < 0)
        {
            return(JSON_ERR_FULL);
        }
        iOffset += iNumChars;
    }

    if ((iNumChars = _JsonIndent(pBuffer, iLength, iOffset, iIndent++, iFlags)) < 0)
    {
        return(JSON_ERR_FULL);
    }
    iOffset += iNumChars;

    if ((iNumChars = ds_snzprintf(pBuffer + iOffset, iLength - iOffset, "[")) < 0)
    {
        return(JSON_ERR_FULL);
    }
    return(_JsonUpdateHeader(pBuffer, iNumChars, iLength, iOffset, iIndent));
}

/*F*************************************************************************************/
/*!
    \Function JsonArrayEnd

    \Description
        End the current array -- must have an outstanding open array

    \Input *pBuffer - JSON buffer

    \Output
        int32_t     - JSON_ERR_*

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
int32_t JsonArrayEnd(char *pBuffer)
{
    int32_t iOffset, iLength, iNumChars;
    int32_t iIndent, iFlags;

    if (!_JsonValidHeader(pBuffer))
    {
        return(JSON_ERR_UNINIT);
    }

    iOffset = _JsonGetOffset(pBuffer);
    iLength = _JsonGetLength(pBuffer);
    iFlags  = _JsonGetFlags(pBuffer);
    iIndent = _JsonGetIndent(pBuffer) - 1;

    if ((iNumChars = _JsonIndent(pBuffer, iLength, iOffset, iIndent, iFlags)) < 0)
    {
        return(JSON_ERR_FULL);
    }
    iOffset += iNumChars;

    iNumChars = ds_snzprintf(pBuffer+iOffset, iLength-iOffset, "]");
    return(_JsonUpdateHeader(pBuffer, iNumChars, iLength, iOffset, iIndent));
}

/*F*************************************************************************************/
/*!
    \Function JsonAddStr

    \Description
        Adds a string element

    \Input *pBuffer   - JSON buffer
    \Input *pElemName - name of the element (may be null)
    \Input *pValue    - value of the element

    \Output
        int32_t       - JSON_ERR_*

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
int32_t JsonAddStr(char *pBuffer, const char *pElemName, const char *pValue)
{
    return(_JsonAddStr(pBuffer, pElemName, pValue, TRUE));
}

/*F*************************************************************************************/
/*!
    \Function JsonAddInt

    \Description
        Add an integer element

    \Input *pBuffer   - JSON buffer
    \Input *pElemName - name of the element
    \Input iValue     - integer value of the element

    \Output
        int32_t       - JSON_ERR_*

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
int32_t JsonAddInt(char *pBuffer, const char *pElemName, int64_t iValue)
{
    char strInt[64];
    ds_snzprintf(strInt, sizeof(strInt), "%lld", iValue);
    return(_JsonAddStr(pBuffer, pElemName, strInt, FALSE));
}

/*F*************************************************************************************/
/*!
    \Function JsonAddNum

    \Description
        Add a complete, contained decimal element. Builds start and end tag.
        Nothing can be appended to this tag. Format it using formatSpec.

    \Input *pBuffer     - JSON buffer
    \Input *pElemName   - name of the element
    \Input *pFormatSpec - format spec for formatting the float (see printf)
    \Input fValue       - float value of the element

    \Output
        int32_t         - JSON_ERR_*

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
int32_t JsonAddNum(char *pBuffer, const char *pElemName, const char *pFormatSpec, float fValue)
{
    char strFloat[JSON_MAX_FLOAT_LENGTH];

    if (ds_snzprintf(strFloat, sizeof(strFloat), pFormatSpec, fValue) <= 0)
    {
        return(JSON_ERR_INVALID_PARAM);
    }
    return(_JsonAddStr(pBuffer, pElemName, strFloat, FALSE));
}

/*F*************************************************************************************/
/*!
    \Function  JsonAddDate

    \Description
       Add a date (will be encoded as a string; use JsonAddInteger for integer encoding)

    \Input *pBuffer     - JSON buffer
    \Input *pElemName   - name of the element
    \Input uEpochDate   - date/time since epoch

    \Output
        int32_t         - JSON_ERR_*

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
int32_t JsonAddDate(char *pBuffer, const char *pElemName, uint32_t uEpochDate)
{
    struct tm *pTime, Time;
    char strDate[JSON_MAX_DATE_LENGTH];

    if ((pTime = ds_secstotime(&Time, uEpochDate)) == NULL)
    {
        return(JSON_ERR_INVALID_PARAM);
    }

    ds_timetostr(pTime, TIMETOSTRING_CONVERSION_ISO_8601, FALSE, strDate, sizeof(strDate));
    return(_JsonAddStr(pBuffer, pElemName, strDate, FALSE));
}

/*F*************************************************************************************/
/*!
    \Function JsonFormatVPrintf

    \Description
        Takes a format string and variable parameter list and formats into syntax
        correct JSON.  The "v" (variable args) version can be called by other
        functions that use "..." in the their prototype.

    \Input *pJsonBuff   - JSON output buffer
    \Input iBufLen      - length of Json output buffer (negative=append to existing buffer)
    \Input *pFormat     - printf style format string
    \Input pFmtArgs     - variable argument list (use va_start / va_end to get)

    \Output
        char *          - pointer to formatted buffer

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
char *JsonFormatVPrintf(char *pJsonBuff, int32_t iBufLen, const char *pFormat, va_list pFmtArgs)
{
#if 0
    int32_t iToken;
    int32_t iIndex;
    char strName[65];
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
        uFlags |= JSON_FL_WHITESPACE;
        ++pFormat;
    }

    // start the formatting
    if (iBufLen >= 0)
        JsonInit(pJsonBuff, iBufLen, uFlags);

    // parse the format string
    for (pParse = pFormat; *pParse != 0; ++pParse)
    {
        // look for tag open
        if (pParse[0] == '{')
        {
            for (iIndex = 0; (iIndex < (signed)(sizeof(strName) - 1)) && (pParse[iIndex+1] > 0) && (pParse[iIndex+1] < 127); ++iIndex)
            {
                if (_ConvName[(int32_t)pParse[iIndex+1]] <= ' ')
                    break;
                strName[iIndex] = pParse[iIndex+1];
            }
            strName[iIndex] = 0;
            if (iIndex > 0)
            {
                JsonObjStart(pJsonBuff, strName);
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
            strName[iIndex] = 0;
        }

        // grab next 3 characters as a token
        iToken = ' ';
        iToken = (iToken << 8) | pParse[0];
        iToken = (iToken << 8) | pParse[1];
        iToken = (iToken << 8) | pParse[2];

        // handle format tokens
        if (pParse[0] == '}')
        {
            JsonObjEnd(pJsonBuff);
        }
        else if (iToken == ' >%s')
        {
            const char *pString = va_arg(pFmtArgs, const char *);
            JsonElemSetString(pJsonBuff, pString);
        }
        else if (iToken == ' >%d')
        {
            int32_t iInteger = va_arg(pFmtArgs, int32_t);
            JsonElemSetInt(pJsonBuff, iInteger);
        }
        else if (iToken == ' >%e')
        {
            uint32_t uEpoch = va_arg(pFmtArgs, uint32_t);
            if (uEpoch == 0)
                uEpoch = (uint32_t)ds_timeinsecs();
            JsonElemSetDate(pJsonBuff, uEpoch);
        }
        else if (iToken == ' >%a')
        {
            uint32_t uAddr = va_arg(pFmtArgs, uint32_t);
            JsonElemSetAddr(pJsonBuff, uAddr);
        }
        else if (strName[0] == 0)
        {
        }
        else if (iToken == ' =%s')
        {
            const char *pString = va_arg(pFmtArgs, const char *);
            JsonAttrSetString(pJsonBuff, strName, pString);
        }
        else if (iToken == ' =%d')
        {
            int32_t iInteger = va_arg(pFmtArgs, int32_t);
            JsonAttrSetInt(pJsonBuff, strName, iInteger);
        }
        else if (iToken == ' =%e')
        {
            uint32_t uEpoch = va_arg(pFmtArgs, uint32_t);
            if (uEpoch == 0)
                uEpoch = (uint32_t)ds_timeinsecs();
            JsonAttrSetDate(pJsonBuff, strName, uEpoch);
        }
        else if (iToken == ' =%a')
        {
            uint32_t uAddr = va_arg(pFmtArgs, uint32_t);
            JsonAttrSetAddr(pJsonBuff, strName, uAddr);
        }
    }

    // finish up and return data pointer
    if (iBufLen >= 0)
        pJsonBuff = JsonFinish(pJsonBuff);
    return(pJsonBuff);
    #else
    return(NULL);
    #endif
}

/*F*************************************************************************************/
/*!
    \Function JsonFormatPrintf

    \Description
        Takes a format string and variable parameter list and formats into syntax
        correct JSON.

    \Input *pJsonBuff   - JSON output buffer
    \Input iBufLen      - length of Json output buffer (negative=append to existing buffer)
    \Input *pFormat     - printf style format string

    \Output
        char *          - pointer to formatted buffer

    \Version 12/11/2012 (jbrookes)
*/
/*************************************************************************************F*/
char *JsonFormatPrintf(char *pJsonBuff, int32_t iBufLen, const char *pFormat, ...)
{
    char *pResult;
    va_list pFmtArgs;

    // setup varargs and call real function
    va_start(pFmtArgs, pFormat);
    pResult = JsonFormatVPrintf(pJsonBuff, iBufLen, pFormat, pFmtArgs);
    va_end(pFmtArgs);

    // return pointer to start of valid JSON data (skips leading spaces)
    return(pResult);
}
