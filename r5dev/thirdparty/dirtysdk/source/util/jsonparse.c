/*H*************************************************************************************/
/*!
    \File jsonparse.c

    \Description
        Simple JSON parser.

    \Copyright
        Copyright (c) Electronic Arts 2012.

    \Notes
        Written by Greg Schaefer outside of EA for a personal project, but donated
        back for a good cause.

        The parse buffer contains a list of objects, consisting of an offset and size.
        The offset is the offset of the object in the JSON buffer.  The size is the
        size of the object in the JSON buffer, except for arrays and objects, where it
        is the size of the object in the parse buffer (used when skipping past an array
        or object).  Values are written as uint16_t, with progressive encoding employed
        when required.

        UNICODE support is limited to extracting ASCII characters that are encoded
        as UNICODE.

    \Todo
        Implement support for getting real numbers        

    \Version 12/11/2012 (jbrookes) Added to DirtySDK, added some new functionality
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/util/jsonparse.h"

/*** Defines ***************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Variables *************************************************************************/

//! decode table to classify characters as whitespace, number, letter, bracket, brace, or invalid
static const uint8_t _strDecode[256] =
{
    0,' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',    // 00..0f
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',    // 10..1f
  ' ',  0,'"',  0,  0,  0,  0,  0,  0,  0,  0,  0,',','1',  0,  0,    // 20..2f
  '1','1','1','1','1','1','1','1','1','1',':',  0,  0,  0,  0,  0,    // 30..3f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 40..4f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,'[','\\',']', 0,  0,    // 50..5f
  ' ','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',    // 60..6f
  'a','a','a','a','a','a','a','a','a','a','a','{',0  ,'}',  0,  0,    // 70..7f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 80..8f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 90..9f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // a0..af
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // b0..bf
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // c0..cf
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // d0..df
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // e0..ef
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0     // f0..ff
};

//! characters that need to be escaped
static const uint8_t _strEscape[256] =
{
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 00..0f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 10..1f
    0,  0,'"',  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,'/',    // 20..2f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 30..3f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 40..4f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,'\\', 0,  0,  0,    // 50..5f
    0,  0,  8,  0,  0,  0, 12,  0,  0,  0,  0,  0,  0,  0, 10,  0,    // 60..6f
    0,  0,  0,  0,  0,'u',  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 70..7f
    0,  0, 13,  0,  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 80..8f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 90..9f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // a0..af
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // b0..bf
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // c0..cf
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // d0..df
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // e0..ef
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0     // f0..ff
};

//! hex->int decode table
static const uint8_t _HexDecode[256] =
{
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,128,128,128,128,128,128,
    128, 10, 11, 12, 13, 14, 15,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128, 10, 11, 12, 13, 14, 15,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128
};


/*** Private Functions *****************************************************************/


/*F*************************************************************************************/
/*!
    \Function _JsonGetQuoted

    \Description
        Get text from a quoted string

    \Input *pDst    - [out] storage for text
    \Input iLim     - size of output buffer
    \Input *pSrc    - pointer to quoted string
    \Input iSkip    - output buffer increment when writing (zero for length check)

    \Output
        int32_t     - returns string length

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t _JsonGetQuoted(uint8_t *pDst, int32_t iLim, const uint8_t *pSrc, int32_t iSkip)
{
    int32_t iLen = 0;
    uint8_t c;

    // make room for terminator
    iLim -= 1;

    // make sure its a quoted string
    if (*pSrc == '"')
    {
        for (++pSrc; (iLen < iLim) && (c = *pSrc) != '"'; ++pSrc)
        {
            // handle escape sequence
            if ((c == '\\') && (pSrc[1] > ' '))
            {
                c = _strEscape[*++pSrc];
                if ((c == 'u') && (pSrc[1] != '"') && (pSrc[2] != '"') && (pSrc[3] != '"') && (pSrc[4] != '"'))
                {
                    c = (_HexDecode[pSrc[3]]<<4)|(_HexDecode[pSrc[4]]<<0), pSrc += 4;
                }
            }
            // conditional save (to use as length check)
            *pDst = c;
            pDst += iSkip;
            ++iLen;
        }
        *pDst = '\0';
    }

    return(iLen);
}

/*F*************************************************************************************/
/*!
    \Function _JsonParseWrite

    \Description
        Write a value to the parse buffer.  Depending on size, the value will be encoded
        in one or two 16 bit values using a progressive encoding scheme.

    \Input *pParse  - [out] buffer for parse value
    \Input *pMax    - limit of parse buffer
    \Input uValue   - value to write to parse buffer
    \Input bWide    - if TRUE, write using wide encoding

    \Output
        uint16_t *  - pointer to next value

    \Notes
        Values from zero to 0x7fff are written as-is using a uint16_t.  Larger values
        are written with the lower 15 bits or'd with 0x8000, followed by bits 15-31 in
        the following uint16_t.  The largest number that can be encoded this way is
        0x7fffffff, as one bit is sacrificed for the progressive bit.

    \Version 02/02/2016 (jbrookes)
*/
/*************************************************************************************F*/
static uint16_t *_JsonParseWrite(uint16_t *pParse, uint16_t *pMax, uint32_t uValue, uint8_t bWide)
{
    if ((uValue > 0x7fff) || (bWide == TRUE))
    {
        if (pParse < pMax)
        {
            *pParse = (uint16_t)uValue | 0x8000;
        }
        uValue >>= 15;
        pParse += 1;
    }
    if (pParse < pMax)
    {
        *pParse = (uint16_t)uValue;
    }
    pParse += 1;
    return(pParse);
}

/*F*************************************************************************************/
/*!
    \Function _JsonParseRead

    \Description
        Read a progressively-encoded value from the parse buffer.

    \Input *pParse      - parse buffer
    \Input *pValue      - [out] storage for value read from parse buffer
    \Input *pParseMax   - end of parse buffer

    \Output
        uint16_t *      - pointer to next value

    \Notes
        See _JsonParseWrite for a description of the progressive encoding scheme.

    \Version 02/02/2016 (jbrookes)
*/
/*************************************************************************************F*/
static const uint16_t *_JsonParseRead(const uint16_t *pParse, uint32_t *pValue, const uint16_t *pParseMax)
{
    *pValue = 0;
    if (pParse < pParseMax)
    {
        *pValue = *pParse;
    }
    pParse += 1;
    if (*pValue & 0x8000)
    {
        *pValue &= ~0x8000;
        if (pParse < pParseMax)
        {
            *pValue |= (uint32_t)*pParse << 15;
        }
        pParse += 1;
    }
    return(pParse);
}

/*F*************************************************************************************/
/*!
    \Function _JsonParseReadOffsetAndSize

    \Description
        Read offset and size values.  If offset exceeds parse buffer size, an offset and
        size of zero are returned.

    \Input *pParse      - parse buffer
    \Input *pOffset     - [out] storage for offset
    \Input *pSize       - [out] storage for size
    \Input *pParseMax   - end of parse buffer

    \Output
        uint16_t *      - pointer to next parse object

    \Notes
        See _JsonParseWrite for a description of the progressive encoding scheme.

    \Version 02/02/2016 (jbrookes)
*/
/*************************************************************************************F*/
static const uint16_t *_JsonParseReadOffsetAndSize(const uint16_t *pParse, uint32_t *pOffset, uint32_t *pSize, const uint16_t *pParseMax)
{
    pParse = _JsonParseRead(pParse, pOffset, pParseMax);
    pParse = _JsonParseRead(pParse, pSize, pParseMax);
    if (pParse > pParseMax)
    {
        *pOffset = 0;
        *pSize = 0;
    }
    return(pParse);
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function JsonParse

    \Description
        Pre-parse a JSON buffer for fast parsing.  Must be called on a buffer before
        any Find() or Get() functions are used.  Pass pDst=NULL and iMax=0 to query
        the required table size.

    \Input *pDst    - [out] storage for lookup table, two elements per object
    \Input iMax     - number of uint16_t elements in output table
    \Input *_pSrc   - pointer to JSON buffer
    \Input iLen     - length of JSON buffer; if -1 a strlen is performed

    \Output
        int32_t     - zero=table too small, else size of table in bytes

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
int32_t JsonParse(uint16_t *pDst, int32_t iMax, const char *_pSrc, int32_t iLen)
{
    uint8_t c;
    int32_t iArray = 0;
    int32_t iObject = 0;
    uint16_t *pArray[16];
    uint16_t *pObject[16];
    const uint8_t *pSrc = (const uint8_t *)_pSrc;
    const uint8_t *pOff = pSrc;
    const uint8_t *pEnd;
    uint16_t *pBeg = pDst;
    uint16_t *pMax = pDst+iMax;
    uint32_t uStart;
    uint8_t bTruncated = FALSE;
    int32_t iBufSize;

    // easy string length
    if (iLen < 0)
    {
        iLen = (int32_t)strlen(_pSrc);
    }
    pEnd = pSrc+iLen;

    // check size and save json pointer in parse buffer
    if (iMax != 0)
    {
        // make sure we have enough space to store json pointer plus at least one max-sized json parse object
        if (iMax < (int32_t)(sizeof(pSrc)/sizeof(uint16_t)) + 4 + 4)
        {
            return(0);
        }
        // save json pointer at start of parse buffer
        ds_memcpy(pDst, &pSrc, sizeof(pSrc));
        // save max size
        ds_memcpy(pDst + (sizeof(pSrc)/sizeof(*pDst)), &iMax, sizeof(iMax));
    }
    // add space for pointer
    pDst += sizeof(pSrc)/sizeof(*pDst);
    // add space for max size
    pDst += sizeof(iMax)/sizeof(*pDst);

    // find the tokens
    for (; pSrc != pEnd; ++pSrc)
    {
        c = _strDecode[*pSrc];

        // handle end of decoding
        if (c == '\0')
        {
            break;
        }
        // handle white space
        if (c == ' ')
        {
            continue;
        }
        // save start of token
        pDst = _JsonParseWrite(pDst, pMax, uStart = (uint32_t)(pSrc-pOff), FALSE);

        // handle number
        if (c == '1')
        {
            // skip past number
            for (++pSrc; (pSrc != pEnd) && (*pSrc >= '0') && (*pSrc <= '9'); ++pSrc)
                ;
            // handle decimal ending
            if ((pSrc != pEnd) && (*pSrc == '.'))
            {
                for (++pSrc; (pSrc != pEnd) && (*pSrc >= '0') && (*pSrc <= '9'); ++pSrc)
                    ;
            }
            // handle exponent
            if ((pSrc != pEnd) && ((*pSrc|32) == 'e'))
            {
                // gobble extension after exponent
                ++pSrc;
                if ((pSrc != pEnd) && ((*pSrc == '+') || (*pSrc == '-')))
                {
                    ++pSrc;
                }
                for (; (pSrc != pEnd) && (*pSrc >= '0') && (*pSrc <= '9'); ++pSrc)
                    ;
            }
            --pSrc;
            pDst = _JsonParseWrite(pDst, pMax, (uint32_t)((pSrc-pOff)-uStart+1), FALSE);
        }
        // handle string
        else if (c == '"')
        {
            // walk the string
            for (++pSrc; (pSrc != pEnd) && (*pSrc != '"'); ++pSrc)
            {
                if ((*pSrc == '\\') && (pSrc[1] > ' '))
                {
                    ++pSrc;
                }
            }
            pDst = _JsonParseWrite(pDst, pMax, (uint32_t)((pSrc-pOff)-uStart+1), FALSE);
        }
        // handle token
        else if (c == 'a')
        {
            for (++pSrc; (pSrc != pEnd) && (_strDecode[*pSrc] == 'a'); ++pSrc)
                ;
            --pSrc;
            pDst = _JsonParseWrite(pDst, pMax, (uint32_t)((pSrc-pOff)-uStart+1), FALSE);
        }
        // handle object open
        else if ((c == '{') && (iObject < (int32_t)(sizeof(pObject)/sizeof(pObject[0]))))
        {
            pObject[iObject++] = pDst;
            pDst = _JsonParseWrite(pDst, pMax, 0, TRUE);
        }
        // handle object close
        else if ((c == '}') && (iObject > 0))
        {
            --iObject;
            _JsonParseWrite(pObject[iObject], pMax, (uint32_t)(pDst-pObject[iObject]), TRUE);
            pDst = _JsonParseWrite(pDst, pMax, 1, FALSE);
        }
        // handle array open
        else if ((c == '[') && (iArray < (int32_t)(sizeof(pArray)/sizeof(pArray[0]))))
        {
            pArray[iArray++] = pDst;
            pDst = _JsonParseWrite(pDst, pMax, 0, TRUE);
        }
        // handle array close
        else if ((c == ']') && (iArray > 0))
        {
            --iArray;
            _JsonParseWrite(pArray[iArray], pMax, (uint32_t)(pDst-pArray[iArray]), TRUE);
            pDst = _JsonParseWrite(pDst, pMax, 1, FALSE);
        }
        else
        {
            pDst = _JsonParseWrite(pDst, pMax, 1, FALSE);
        }
    }

    // close any remaining objects -- invalid JSON?
    while (iObject-- > 0)
    {
        _JsonParseWrite(pObject[iObject], pMax, (uint32_t)(pDst-pObject[iObject]), TRUE);
    }
    while (iArray-- > 0)
    {
        _JsonParseWrite(pArray[iArray], pMax, (uint32_t)(pDst-pArray[iArray]), TRUE);
    }

    // make sure we always terminate output buffer
    if ((iMax != 0) && (pDst > pMax-2))
    {
        pDst = pMax-2;
        bTruncated = TRUE;
    }
    // terminate output buffer
    pDst = _JsonParseWrite(pDst, pMax, 0, FALSE);
    pDst = _JsonParseWrite(pDst, pMax, 0, FALSE);

    // calculate size of buffer in bytes
    iBufSize = (int32_t)(((uint8_t *)pDst) - ((uint8_t *)pBeg));

    // return size in bytes, or zero if the table was truncated
    return(bTruncated ? 0 : iBufSize);
}

/*F*************************************************************************************/
/*!
\Function JsonParse2

    \Description
        Pre-parse a JSON buffer for fast parsing.  Must be called on a buffer before
        any Find() or Get() functions are used.  This version allocates the required
        amount of memory internally rather than taking the buffer as input.

        \Input *pSrc    - pointer to JSON buffer
        \Input iLen     - length of JSON buffer; if -1 a strlen is performed
        \Input iMemModule - memory module passed to DirtyMemAlloc
        \Input iMemGroup - memory group passed to DirtyMemAlloc   
        \Input *pMemGroupUserData - memory group user data passed to DirtyMemAlloc

    \Output
        uint16_t *      - allocated parse table, or NULL on error

    \Version 12/05/2016 (jbrookes)
*/
/*************************************************************************************F*/
uint16_t *JsonParse2(const char *pSrc, int32_t iLen, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    uint16_t *pBuff;
    int32_t iSize;

    // get buffer size
    if ((iSize = JsonParse(NULL, 0, pSrc, iLen)) == 0)
    {
        return(NULL);
    }
    // allocate buffer space
    if ((pBuff = DirtyMemAlloc(iSize, iMemModule, iMemGroup, pMemGroupUserData)) == NULL)
    {
        return(NULL);
    }
    // parse the buffer
    if (JsonParse(pBuff, iSize, pSrc, iLen) != iSize)
    {
        DirtyMemFree(pBuff, iMemModule, iMemGroup, pMemGroupUserData);
        pBuff = NULL;
    }
    return(pBuff);
}

/*F*************************************************************************************/
/*!
    \Function JsonFind

    \Description
        Locate a JSON element

    \Input *pParse      - pre-parsed lookup from JsonParse
    \Input *pName       - name of element to locate

    \Output
        const char *    - pointer to element, or NULL if not found

    \Notes
        A name can include a period character '.' which indicates an object reference,
        or an open bracket '[' which indicates an array reference.  A bracket must be
        the last character in the name.

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
const char *JsonFind(const uint16_t *pParse, const char *pName)
{
    return(JsonFind2(pParse, NULL, pName, 0));
}

/*F*************************************************************************************/
/*!
    \Function JsonFind2

    \Description
        Locate a JSON element, starting from an offset, with an optional array index

    \Input *pParse      - pre-parsed lookup from JsonParse
    \Input *_pJsonOff   - offset into JSON buffer to begin search
    \Input *pName       - name of element to locate
    \Input iIndex       - [optional] index of array element to return

    \Output
        const uint8_t * - pointer to element, or NULL if not found

    \Notes
        This version of Find is useful for parsing arrays of scalars or objects, or
        for starting a parse at an offset within the JSON document.

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
const char *JsonFind2(const uint16_t *pParse, const char *_pJsonOff, const char *pName, int32_t iIndex)
{
    const uint8_t *pJsonOff = (const uint8_t *)_pJsonOff;
    const uint8_t *pJson;
    const char *pLast, *pElem;
    const uint16_t *pParseMax, *pTemp, *pTemp2;
    int32_t iLen, iMax;
    uint32_t uOffset, uOffsetNext, uSize, uSizeNext, uTemp;
    uint8_t c;

    // save pointer for parse max
    pParseMax = pParse;

    // get json data pointer; note cast to avoid invalid vc warning -- thinks &pJson is const whereas it points to something const
    ds_memcpy((uint8_t *)&pJson, pParse, sizeof(pJson));
    pParse += sizeof(pJson)/sizeof(*pParse);
    // get parse buffer max size
    ds_memcpy(&iMax, pParse, sizeof(iMax));
    pParse += sizeof(iMax)/sizeof(*pParse);

    // point to end of parse buffer
    pParseMax += iMax;

    // if json has outer object, then skip it
    pTemp = _JsonParseRead(pParse, &uOffset, pParseMax);
    if (pJson[uOffset] == '{')
    {
        pParse = _JsonParseRead(pTemp, &uTemp, pParseMax);
    }

    // if they gave us an offset, move the parse forward to match
    if (pJsonOff != NULL)
    {
        uint32_t uJsonOff = (uint32_t)(pJsonOff - pJson);
        while (1)
        {
            pTemp = _JsonParseReadOffsetAndSize(pParse, &uOffset, &uSize, pParseMax);
            if ((uOffset >= uJsonOff) || (uSize == 0))
            {
                break;
            }
            pParse = pTemp;
        }
    }

    // parse the find string
    for (uSize = 1; (*pName != 0) && (uSize != 0); )
    {
        pTemp = _JsonParseReadOffsetAndSize(pParse, &uOffset, &uSize, pParseMax);

        // handle traversing into an object
        if (*pName == '.')
        {
            // if not an object, then its an error
            if (pJson[uOffset] != '{')
            {
                break;
            }
            // move into object and keep parsing
            pParse = pTemp;
            ++pName;
            continue;
        }
        // handle traversing into an array
        else if (*pName == '[')
        {
            // if not an array, then its an error
            if (pJson[uOffset] != '[')
            {
                break;
            }
            // skip to nth element
            for (pParse = pTemp, uSize = 1; (iIndex > 0) && (uSize != 0); iIndex -= 1)
            {
                pTemp = _JsonParseReadOffsetAndSize(pParse, &uOffset, &uSize, pParseMax);
                c = pJson[uOffset];
                if (c == ']')
                {
                    break;
                }
                else if (c == '{')
                {
                    // skip to end of object
                    pParse += uSize; 
                    pTemp = pParse;
                 }
                // skip object
                if (uSize != 0)
                {
                    pTemp = _JsonParseReadOffsetAndSize(pParse, &uOffset, &uSize, pParseMax);
                }
                // skip trailing comma?
                pTemp = _JsonParseReadOffsetAndSize(pTemp, &uOffset, &uSize, pParseMax);
                if ((uSize != 0) && (pJson[uOffset] == ','))
                {
                    pParse = pTemp;
                }
            }
            // if we didn't end on a close bracket we've found our element
            if ((uSize != 0) && (pJson[uOffset] != ']')) // must re-check because we could end exactly on the close bracket
            {
                pName = "\0";
            }
            break;
        }
        // check to see if object name matches
        else if (((*pName|32) >= 'a') && ((*pName|32) <= 'z'))
        {
            // figure out length
            for (pLast = pName; (*pLast != 0) && (*pLast != '.') && (*pLast != '['); ++pLast)
                ;
            iLen = (int32_t)(pLast-pName);

            // search for this name
            for (uSize = 1; uSize != 0; pParse = pTemp)
            {
                pTemp = _JsonParseReadOffsetAndSize(pParse, &uOffset, &uSize, pParseMax);
                c = pJson[uOffset];

                // skip arrays & objects
                if ((c == '[') || (c == '{'))
                {
                    pTemp = _JsonParseReadOffsetAndSize(pParse+uSize, &uOffset, &uSize, pParseMax);
                    if (uSize == 0)
                    {
                        break;
                    }
                    continue;
                }

                // end of array/object is terminator, or we hit the end of the parse buffer
                if ((c == ']') || (c == '}') || (uSize == 0))
                {
                    return(NULL);
                }

                // look for a label
                pTemp2 = _JsonParseReadOffsetAndSize(pTemp, &uOffsetNext, &uSizeNext, pParseMax);
                if ((c == '"') && (pJson[uOffsetNext] == ':') && (iLen+2 == (signed)uSize))
                {
                    if (memcmp(pJson+uOffset+1, pName, iLen) == 0)
                    {
                        // skip the label
                        pParse = pTemp2;
                        uOffset = uOffsetNext;
                        uSize = uSizeNext;
                        pName = pLast;
                        break;
                    }
                }
            }
        }
        else
        {
            // invalid pattern
            return(NULL);
        }
    }

    // get current offset and size from parse
    _JsonParseReadOffsetAndSize(pParse, &uOffset, &uSize, pParseMax);

    /* point to element; validate the name matches and that we are pointing at a valid element.  both checks
       are required, as we could have found the name successfully, but then failed to find the element */
    if ((*pName == '\0') && (uSize != 0))
    {
        pElem = (const char *)(pJson+uOffset);
    }
    else
    {
        pElem = NULL;
    }
    return(pElem);
}

/*F*************************************************************************************/
/*!
    \Function JsonGetString

    \Description
        Extract a JSON string element.  Pass pBuffer=NULL for length check.

    \Input *pJson       - JSON buffer
    \Input *pBuffer     - [out] storage for string (may be NULL for length query)
    \Input iLength      - length of output buffer
    \Input *pDefault    - default value to use, if string could not be extracted

    \Output
        int32_t         - length of string, or negative on error

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
int32_t JsonGetString(const char *pJson, char *pBuffer, int32_t iLength, const char *pDefault)
{
    return(JsonGetString2(pJson, pBuffer, iLength, pDefault, NULL));
}

/*F*************************************************************************************/
/*!
    \Function JsonGetString2

    \Description
        Extract a JSON string element.  Pass pBuffer=NULL for length check.  Error
        accumulating version.

    \Input *pJson       - JSON buffer
    \Input *pBuffer     - [out] storage for string (may be NULL for length query)
    \Input iLength      - length of output buffer
    \Input *pDefault    - default value to use, if string could not be extracted
    \Input *pError      - [in/out] accumulation flag for parse errors; 1=error, 0=no error

    \Output
        int32_t         - length of string, or negative on error

    \Version 12/05/2016 (jbrookes)
*/
/*************************************************************************************F*/
int32_t JsonGetString2(const char *pJson, char *pBuffer, int32_t iLength, const char *pDefault, uint8_t *pError)
{
    uint8_t c;

    // handle default value
    if (pJson == NULL)
    {
        // error if no default string
        if (pDefault == NULL)
        {
            if (pError != NULL)
            {
                *pError = 1;
            }
            return(-1);
        }
        // copy string if it isn't a length-only query
        if (pBuffer != NULL)
        {
            ds_strnzcpy(pBuffer, pDefault, iLength);
            pDefault = pBuffer;
        }
        // return the length
        return((int32_t)strlen(pDefault));
    }

    // handle length query
    if (pBuffer == NULL)
    {
        return(_JsonGetQuoted(&c, INT32_MAX, (const uint8_t *)pJson, 0));
    }

    // make sure return buffer has terminator length
    if (iLength < 1)
    {
        if (pError != NULL)
        {
            *pError = 1;
        }
        return(-1);
    }

    // copy the string
    return(_JsonGetQuoted((uint8_t *)pBuffer, iLength, (const uint8_t *)pJson, 1));
}

/*F*************************************************************************************/
/*!
    \Function JsonGetInteger

    \Description
        Extract a JSON integer number element.

    \Input *pJson       - JSON buffer
    \Input iDefault     - default value to use, if number could not be extracted

    \Output
        int64_t         - integer, or default value

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
int64_t JsonGetInteger(const char *pJson, int64_t iDefault)
{
    return(JsonGetInteger2(pJson, iDefault, INT64_MIN, INT64_MAX, NULL));
}

/*F*************************************************************************************/
/*!
    \Function JsonGetInteger2

    \Description
        Extract a JSON integer number element with range checking and error accumulation.
        Pass zero for both iMin and iMax to disable range checking.

    \Input *pJson       - JSON buffer
    \Input iDefault     - default value to use, if number could not be extracted
    \Input iMin         - min value for range checking
    \Input iMax         - max value for range checking
    \Input *pError      - [in/out] accumulation flag for parse errors; 1=error, 0=no error

    \Output
        int64_t         - integer, or default value

    \Version 12/05/2016 (jbrookes)
*/
/*************************************************************************************F*/
int64_t JsonGetInteger2(const char *pJson, int64_t iDefault, int64_t iMin, int64_t iMax, uint8_t *pError)
{
    int64_t iSign = 1;
    int64_t iValue;

    // handle default
    if (pJson == NULL)
    {
        if (pError != NULL)
        {
            *pError = 1;
        }
        return(iDefault);
    }

    // handle negative
    if (*pJson == '-')
    {
        ++pJson;
        iSign = -1;
    }

    // parse the value
    for (iValue = 0; (*pJson >= '0') && (*pJson <= '9'); ++pJson)
    {
        iValue = (iValue * 10) + (*pJson & 15);
    }

    // range check
    if ((iMin != 0) || (iMax != 0))
    {
        iValue = DS_CLAMP(iValue, iMin, iMax);
    }    

    // return with sign
    return(iSign*iValue);
}

/*F*************************************************************************************/
/*!
    \Function JsonGetDate

    \Description
        Extract a JSON date element

    \Input *pJson       - JSON buffer
    \Input uDefault     - default date value to use

    \Output
        uint32_t        - extracted date, or default

    \Notes
        Date is assumed to be in ISO_8601 format

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
uint32_t JsonGetDate(const char *pJson, uint32_t uDefault)
{
    return(JsonGetDate2(pJson, uDefault, NULL));
}

/*F*************************************************************************************/
/*!
    \Function JsonGetDate2

    \Description
        Extract a JSON date element

    \Input *pJson       - JSON buffer
    \Input uDefault     - default date value to use
    \Input *pError      - [in/out] accumulation flag for parse errors; 1=error, 0=no error

    \Output
        uint32_t        - extracted date, or default

    \Notes
        Date is assumed to be in ISO_8601 format

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
uint32_t JsonGetDate2(const char *pJson, uint32_t uDefault, uint8_t *pError)
{
    uint32_t uDate = uDefault;
    char strValue[128];

    JsonGetString(pJson, strValue, sizeof(strValue), "");
    if ((uDate = (uint32_t)ds_strtotime2(strValue, TIMETOSTRING_CONVERSION_ISO_8601)) == 0)
    {
        uDate = uDefault;
        if (pError != NULL)
        {
            *pError = 1;
        }
    }
    return(uDate);
}

/*F*************************************************************************************/
/*!
    \Function JsonGetBoolean

    \Description
        Extract a JSON boolean

    \Input *pJson       - JSON buffer
    \Input bDefault     - default boolean value to use

    \Output
        uint8_t         - extracted bool, or default

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
uint8_t JsonGetBoolean(const char *pJson, uint8_t bDefault)
{
    return(JsonGetBoolean2(pJson, bDefault, NULL));
}

/*F*************************************************************************************/
/*!
    \Function JsonGetBoolean2

    \Description
        Extract a JSON boolean, with accumulating error

    \Input *pJson       - JSON buffer
    \Input bDefault     - default boolean value to use
    \Input *pError      - [in/out] accumulation flag for parse errors; 1=error, 0=no error

    \Output
        uint8_t         - extracted bool, or default

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
uint8_t JsonGetBoolean2(const char *pJson, uint8_t bDefault, uint8_t *pError)
{
    uint8_t bValue = bDefault;

    // handle default
    if (pJson == NULL)
    {
        if (pError != NULL)
        {
            *pError = 1;
        }
        return(bDefault);
    }
    // check for true
    if (strncmp((const char *)pJson, "true", 4) == 0)
    {
        bValue = TRUE;
    }
    else if (strncmp((const char *)pJson, "false", 4) == 0)
    {
        bValue = FALSE;
    }
    else if (pError != NULL)
    {
        *pError = 1;
    }
    // return result
    return(bValue);
}

/*F*************************************************************************************/
/*!
    \Function JsonGetEnum

    \Description
        Extract a JSON enumerated value.  The source type is assumed to be a string,
        and pEnumArray contains a NULL-terminated list of strings that may be converted
        to an enum value.

    \Input *pJson       - JSON buffer
    \Input *pEnumArray  - NULL-terminated list of enum strings
    \Input iDefault     - default enum value to use if no match is found

    \Output
        int32_t         - enum value; or default

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
int32_t JsonGetEnum(const char *pJson, const char *pEnumArray[], int32_t iDefault)
{
    return(JsonGetEnum2(pJson, pEnumArray, iDefault, NULL));
}

/*F*************************************************************************************/
/*!
    \Function JsonGetEnum2

    \Description
        Extract a JSON enumerated value, with accumulating error.  The source type is
        assumed to be a string, and pEnumArray contains a NULL-terminated list of strings
        that may be converted to an enum value.

    \Input *pJson       - JSON buffer
    \Input *pEnumArray  - NULL-terminated list of enum strings
    \Input iDefault     - default enum value to use if no match is found
    \Input *pError      - [in/out] accumulation flag for parse errors; 1=error, 0=no error

    \Output
        int32_t         - enum value; or default

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
int32_t JsonGetEnum2(const char *pJson, const char *pEnumArray[], int32_t iDefault, uint8_t *pError)
{
    int32_t iEnum = iDefault;
    char strValue[128];

    JsonGetString(pJson, strValue, sizeof(strValue), "");
    for (iEnum = 0; pEnumArray[iEnum] != NULL; iEnum += 1)
    {
        if (!strcmp(pEnumArray[iEnum], strValue))
        {
            return(iEnum);
        }
    }
    if (pError != NULL)
    {
        *pError = 1;
    }
    return(iDefault);
}

/*F*************************************************************************************************/
/*!
    \Function   JsonSeekObjectEnd

    \Description
        Seek to the end of an object within the JSON text buffer

    \Input *pList       - pointer to the beginning of object

    \Output
        const char *    - pointer to the end of the object, NULL for error

    \Version 04/17/2013 (amakoukji)
*/
/*************************************************************************************************F*/
const char *JsonSeekObjectEnd(const char *pList)
{
    const char *pCurrent = pList;
    const char *pEnd = pList + (int32_t)strlen(pList);
    uint32_t uDepth = 0;
    char c;

    while (pCurrent < pEnd)
    {
        c = *pCurrent;

        switch (c)
        {
            case '"':
            {
                // walk the string
                for (++pCurrent; (pCurrent != pEnd) && (*pCurrent != '"'); ++pCurrent)
                {
                    if ((*pCurrent == '\\') && (pCurrent[1] > ' '))
                    {
                        ++pCurrent;
                    }
                }
            }
            break;

            case '{':
            {
                uDepth += 1;
            }
            break;

            case '}':
            {
                uDepth -= 1;
            }
            break;

            case ',':
            {
                if (uDepth == 0)
                {
                    return(pCurrent);
                }
            }
            break;

            case ']':
            {
                if (uDepth == 0)
                {
                    return(pCurrent);
                }
            }
            break;
        }

        pCurrent += 1;
    }

    // error
    return(NULL);
}

/*F*************************************************************************************************/
/*!
    \Function JsonSeekValue

    \Description
        Given a pointer to the beginning of a simple JSON key value pair, seeks the 
        start of the value
        
    \Input *pKey        - module ref

    \Output
        int32_t         - pointer to the start of the value, NULL for error
        
    \Version 11/28/2013 (amakoukji)
*/
/*************************************************************************************************F*/
const char *JsonSeekValue(const char *pKey)
{
    const char *pTemp = pKey;

    if (pKey == NULL)
    {
        return(NULL);
    }

    // seek to the ':'
    while (*pTemp != ':')
    {
        if ((*pTemp == '\0') || (*pTemp == '\n'))
        {
            return(NULL);
        }
        pTemp += 1;
    }
    pTemp += 1;

    // seek to non-whitespace
    while ((*pTemp == ' ') || (*pTemp == '\t'))
    {
        if ((*pTemp == '\0') || (*pTemp == '\n'))
        {
            return(NULL);
        }
        pTemp += 1;
    }

    return(pTemp);
}

