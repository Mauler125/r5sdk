/*H*************************************************************************************************/
/*!

    \File    utf8.c

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


/*** Include files *********************************************************************/

#include "DirtySDK/util/utf8.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

// Public variables


/*** Private Functions *****************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    _Utf8GetNumBytes

    \Description
        Decode the number of bytes in a UTF-8 encoded sequence.

    \Input cLead    - lead character of UTF-8 sequence

    \Output
        int32_t     - number of bytes in the sequence

    \Version 03/25/03 (JLB)
*/
/*************************************************************************************************F*/
static int32_t _Utf8GetNumBytes(unsigned char cLead)
{
    int32_t iCodeSize;

    if ((cLead & 0x80) == 0x00)
    {
        iCodeSize = 1;
    }
    else if ((cLead & 0xE0) == 0xC0)
    {
        iCodeSize = 2;
    }
    else if ((cLead & 0xF0) == 0xE0)
    {
        iCodeSize = 3;
    }
    else
    {
        iCodeSize = 4;
    }

    return(iCodeSize);
}

/*F*************************************************************************************************/
/*!
    \Function    _Utf8DecodeToUCS2

    \Description
        Decode a UTF-8 sequence into a UCS-2 code point.

    \Input *pOutPtr - pointer to output for decoded UCS-2 value
    \Input *pStr    - pointer to input UTF-8 string

    \Output
        int32_t     - number of input 8bit characters consumed

    \Notes
        UCS-2 range (hex)   UTF-8 octet sequence (binary)
        007F                0xxxxxxx
        07FF                110xxxxx 10xxxxxx
        FFFF                1110xxxx 10xxxxxx 10xxxxxx

    \Version 03/26/03 (JLB)
*/
/*************************************************************************************************F*/
static int32_t _Utf8DecodeToUCS2(uint16_t *pOutPtr, const unsigned char *pStr)
{
    int32_t iCodeSize;

    if ((*pStr & 0x80) == 0x00)
    {
        pOutPtr[0] = (uint16_t)pStr[0];
        iCodeSize = 1;
    }
    else if ((*pStr & 0xE0) == 0xC0)
    {
        pOutPtr[0] = ((pStr[0] & ~0xE0) << 6) | (pStr[1] & ~0xC0);
        iCodeSize = 2;
    }
    else if ((*pStr & 0xF0) == 0xE0)
    {
        pOutPtr[0] = ((pStr[0] & ~0xF0) << 12) | ((pStr[1] & ~0xC0) << 6) | (pStr[2] & ~0xC0);
        iCodeSize = 3;
    }
    else
    {
        iCodeSize = 4;
    }

    return(iCodeSize);
}

/*F*************************************************************************************************/
/*!
    \Function    Utf8EncodeFromUCS2CodePt

    \Description
        Encode a single  UCS-2 code point ("char") into a UTF-8 sequence.

    \Input uCodePt  - input UCS-2 code point
    \Input *pOutPtr - pointer to output for encoded UTF-8 sequence.

    \Output
        int32_t     - number of 8bit characters output

    \Notes
        See notes for _Utf8DecodeToUCS2()

    \Version 03/26/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t Utf8EncodeFromUCS2CodePt(char *pOutPtr, uint16_t uCodePt)
{
    int32_t iCodeSize;

    if (uCodePt < 0x0080)
    {
        pOutPtr[0] = (char)uCodePt;
        iCodeSize = 1;
    }
    else if (uCodePt < 0x800)
    {
        pOutPtr[0] = 0xC0 | (uCodePt >> 6);
        pOutPtr[1] = 0x80 | (uCodePt & 0x3F); 
        iCodeSize = 2;
    }
    else
    {
        pOutPtr[0] = 0xE0 | (uCodePt >> 12);
        pOutPtr[1] = 0x80 | ((uCodePt >> 6) & 0x3F);
        pOutPtr[2] = 0x80 | (uCodePt & 0x3F);
        iCodeSize = 3;
    }

    return(iCodeSize);
}

/*F*************************************************************************************************/
/*!
    \Function    _Utf8Translate

    \Description
        Look through translation subtables and translate uCodePt

    \Input *pOutBuf     - output buffer to store 8bit translated output
    \Input *pTransTbl   - translation table to translate with
    \Input uCodePt      - UCS-2 code point to translate
    \Input cReplace     - character to replace untranslatable characters with (or null-termination char to strip)

    \Output
        int32_t         - number of ascii characters output

    \Version 03/26/03 (JLB)
*/
/*************************************************************************************************F*/
static int32_t _Utf8Translate(char *pOutBuf, const Utf8TransTblT *pTransTbl, uint16_t uCodePt, char cReplace)
{
    unsigned char cCode;
    char *pOldBuf = pOutBuf;
    int32_t bFound;

    // look through subtables
    for (bFound = FALSE; pTransTbl->uRangeEnd != 0; pTransTbl++)
    {
        // are we in range?
        if ((uCodePt >= pTransTbl->uRangeBegin) && (uCodePt <= (pTransTbl->uRangeEnd)))
        {
            // dereference table
            uCodePt -= pTransTbl->uRangeBegin;
            cCode = (unsigned char)pTransTbl->pCodeTbl[uCodePt];

            if ((cCode == 0xFF) && (cReplace != '\0'))
            {
                // untranslatable - replace
                *(pOutBuf++) = cReplace;
            }
            else
            {
                // translate
                *(pOutBuf++) = cCode;
            }

            bFound = TRUE;
            break;
        }
    }

    // not found and replacing?
    if ((bFound == FALSE) && (cReplace != '\0'))
    {
        // replace
        *(pOutBuf++) = cReplace;
    }

    // return number of characters output
    return((int32_t)(pOutBuf-pOldBuf));
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    Utf8Strip

    \Description
        Strip non-ASCII UTF-8 encoded data.

    \Input *pOutStr - pointer to output buffer (may be same as input buffer)
    \Input iBufSize - size of output buffer in ASCII units (char)
    \Input *pInStr  - pointer to source string

    \Output
        int32_t     - number of characters in new string, or zero if no utf8 data was stripped

    \Version 03/25/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t Utf8Strip(char *pOutStr, int32_t iBufSize, const char *pInStr)
{
    return(Utf8Replace(pOutStr, iBufSize, pInStr, '\0'));
}

/*F*************************************************************************************************/
/*!
    \Function    Utf8Replace

    \Description
        Replace non-ASCII UTF-8 encoded data with the given character.

    \Input *pOutStr - pointer to output buffer (may be same as input buffer)
    \Input iBufSize - size of output buffer in ASCII units (char)
    \Input *pInStr  - pointer to source string
    \Input cReplace - character to replace non-ASCII UTF-8 characters with

    \Output
        int32_t     - number of characters in new string, or zero if no UTF-8 data was replaced

    \Version 03/25/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t Utf8Replace(char *pOutStr, int32_t iBufSize, const char *pInStr, char cReplace)
{
    int32_t iSrcIdx, iDstIdx = 0;

    // fast scan to find any utf8 encoded data
    for (iSrcIdx = 0; ((pInStr[iSrcIdx] & 0x80) == 0) && (pInStr[iSrcIdx] != '\0'); iSrcIdx++)
    {
    }

    // did we find any?
    if (pInStr[iSrcIdx] != '\0')
    {
        // yes, so replace/strip it
        for (iDstIdx = iSrcIdx; pInStr[iSrcIdx] != '\0' && iDstIdx < iBufSize; )
        {
            // do we have utf8 data?
            if (pInStr[iSrcIdx] & 0x80)
            {
                // figure out how many bytes of utf8 data we have
                int32_t iNumBytes = _Utf8GetNumBytes(pInStr[iSrcIdx]);

                // skip them
                iSrcIdx += iNumBytes;

                // replace with cReplace
                if (cReplace != '\0')
                {
                    pOutStr[iDstIdx++] = cReplace;
                }
            }
            else
            {
                // normal string data - copy it
                pOutStr[iDstIdx++] = pInStr[iSrcIdx++];
            }
        }

        // terminate
        pOutStr[iDstIdx++] = '\0';
    }
    
    return(iDstIdx);
}

/*F*************************************************************************************************/
/*!
    \Function    Utf8StrLen

    \Description
        Returns the number of code points in a UTF-8 encoded string.

    \Input *pStr    - pointer to string to get string length of

    \Output
        int32_t     - number of code points in string

    \Version 03/26/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t Utf8StrLen(const char *pStr)
{
    int32_t iSrcIdx, iStrLen;

    for (iSrcIdx = iStrLen = 0; pStr[iSrcIdx] != '\0'; iStrLen++)
    {
        if (pStr[iSrcIdx] & 0x80)
        {
            // figure out how many bytes of utf8 data we have
            int32_t iNumBytes = _Utf8GetNumBytes(pStr[iSrcIdx]);

            // skip them
            iSrcIdx += iNumBytes;
        }
        else
        {
            iSrcIdx++;
        }
    }

    return(iStrLen);
}

/*F*************************************************************************************************/
/*!
    \Function    Utf8EncodeFromUCS2

    \Description
        Convert a UCS-2 code point sequence into a UTF-8 code point sequence.

    \Input *pOutStr - pointer to buffer to encode string to
    \Input iBufLen  - length of output buffer, in char units
    \Input *pInStr  - pointer to string to encode

    \Output
        int32_t     - output string length, in char units 

    \Version 03/27/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t Utf8EncodeFromUCS2(char *pOutStr, int32_t iBufLen, const uint16_t *pInStr)
{
    int32_t iStrLen;

    // a UCS-2 encoded string can generate up to three chars.
    iBufLen -= 2;

    // encode
    for (iStrLen = 0; (*pInStr != 0x0000) && (iStrLen < iBufLen); pInStr++)
    {
        iStrLen += Utf8EncodeFromUCS2CodePt(&pOutStr[iStrLen], *pInStr);
    }    

    // NULL terminate & return length to caller
    pOutStr[iStrLen++] = '\0';
    return(iStrLen);
}

/*F*************************************************************************************************/
/*!
    \Function    Utf8DecodeToUCS2

    \Description
        Convert a UTF-8 code point sequence into a UCS-2 code point sequence.

    \Input *pOutStr - pointer to buffer to decode string to
    \Input iBufLen  - length of output buffer, in UCS-2 units (int16_t)
    \Input *pInStr  - pointer to string to decode

    \Output
        int32_t     - output string length, in code points 

    \Version 03/26/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t Utf8DecodeToUCS2(uint16_t *pOutStr, int32_t iBufLen, const char *pInStr)
{
    int32_t iSrcIdx, iStrLen, iCodeSize;

    // ensure room for NULL terminator
    iBufLen--;

    // decode string
    for (iSrcIdx = iStrLen = 0; (pInStr[iSrcIdx] != '\0') && (iStrLen < iBufLen); )
    {
        if ((iCodeSize = _Utf8DecodeToUCS2(&pOutStr[iStrLen], (const unsigned char *)&pInStr[iSrcIdx])) <= 3)
        {
            iStrLen++;
        }

        iSrcIdx += iCodeSize;
    }    

    // NULL terminate & return length to caller
    pOutStr[iStrLen++] = 0x0000;
    return(iStrLen);
}

/*F*************************************************************************************************/
/*!
    \Function    Utf8EncodeFrom8Bit

    \Description
        Encode the given 8bit input string to UTF-8, based on the input translation table

    \Input *pOutStr     - pointer to output UTF-8 string buffer
    \Input iBufLen      - length of buffer
    \Input *pInStr      - pointer to input 8bit string
    \Input *pEncodeTbl  - pointer to translation table to map 8bit string to UCS-2

    \Output
        int32_t         - length of output UCS-8 string

    \Version 03/28/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t Utf8EncodeFrom8Bit(char *pOutStr, int32_t iBufLen, const char *pInStr, const Utf8EncodeTblT *pEncodeTbl)
{
    int32_t iStrLen;
    uint16_t uCodePt;

    // a UCS-2 encoded value can generate up to three chars.
    iBufLen -= 2;

    // encode
    for (iStrLen = 0; (*pInStr != 0x0000) && (iStrLen < iBufLen); pInStr++)
    {
        uCodePt = pEncodeTbl->uCodeTbl[*(const unsigned char *)pInStr];
        iStrLen += Utf8EncodeFromUCS2CodePt(&pOutStr[iStrLen], uCodePt);
    }

    // NULL terminate & return length to caller
    pOutStr[iStrLen++] = '\0';
    return(iStrLen);
}

/*F*************************************************************************************************/
/*!
    \Function    Utf8TranslateTo8Bit

    \Description
        Translates the given UTF-8 sequence based on the input translation table.

    \Input *pOutStr     - pointer to buffer to decode string to
    \Input iBufLen      - length of output buffer, in ASCII units (char)
    \Input *pInStr      - pointer to string to decode
    \Input cReplace     - \verbatim character to replace code point with if untranslateable ('\0' to strip) \endverbatim
    \Input *pTransTbl   - pointer to NULL-terminated translation table array

    \Output
        int32_t         - length of output string in ASCII characters (8bit)

    \Notes
        'pTransTbl' is expected to be a NULL-terminated array of Utf8TransTblT structures that
        represent a sparse translation table from 16bit UCS-2 space to 8bit game font space.

    \Version 03/26/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t Utf8TranslateTo8Bit(char *pOutStr, int32_t iBufLen, const char *pInStr, char cReplace, const Utf8TransTblT *pTransTbl)
{
    int32_t iSrcIdx, iDstIdx;
    uint16_t uCodePt = 0;

    // ensure room for NULL terminator
    iBufLen--;

    // translate string
    for (iSrcIdx = iDstIdx = 0; (pInStr[iSrcIdx] != '\0') && (iDstIdx < iBufLen); )
    {
        iSrcIdx += _Utf8DecodeToUCS2(&uCodePt, (const unsigned char *)&pInStr[iSrcIdx]);
        iDstIdx += _Utf8Translate(&pOutStr[iDstIdx], pTransTbl, uCodePt, cReplace);
    }

    // NULL terminate & return length to caller
    pOutStr[iDstIdx++] = '\0';
    return(iDstIdx);
}
