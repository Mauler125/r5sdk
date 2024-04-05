/*H********************************************************************************/
/*!
    \File plat-str.c

    \Description
        This file provides platform independent versions of some standard-C
        functions that are not "standard" across platforms and/or fixes
        implementation problems with the standard versions (such as consistent
        termination).

    \Copyright
        Copyright (c) 2005-2011 Electronic Arts Inc.

    \Version 01/11/2005 (gschaefer) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>      // tolower

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtynet.h" // sockaddr

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

//! lowercase hex translation table
static const char _ds_strhexlower[16] = "0123456789abcdef";
//! uppercase hex translation table
static const char _ds_strhexupper[16] = "0123456789ABCDEF";

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function _ds_writechar

    \Description
        Write a character to output buffer, if there is room

    \Input *pBuffer     - [out] output buffer
    \Input iLength      - size of output buffer
    \Input cWrite       - character to write to output
    \Input iOutput      - offset in output buffer to write to

    \Output
        int32_t         - iOutput+1

    \Version 02/14/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ds_writechar(char *pBuffer, int32_t iLength, char cWrite, int32_t iOutput)
{
    if (iOutput < iLength)
    {
        pBuffer[iOutput] = cWrite;
    }
    return(iOutput+1);
}

/*F********************************************************************************/
/*!
    \Function _ds_writestr

    \Description
        Write string to output buffer, supporting a source of chars or wchars and a
        destination of chars. As per C99 specifications, if the output does not fit in
        the given buffer, this function will return the number of characters that would
        have been written if the buffer were big enough, but will truncate the string to
        preserve buffer integrity.

    \Input *pBuffer     - [out] output buffer
    \Input iLength      - size of output buffer
    \Input *pString     - string to print
    \Input iOutput      - offset in output buffer to print to
    \Input bWideChar    - if TRUE, input string uses wide (wchar_t) chars

    \Output
        int32_t         - number of characters written to output buffer

    \Version 12/14/2011 (jbrookes) Extracted from _ds_printstr(), added wchar support
*/
/********************************************************************************F*/
static int32_t _ds_writestr(char *pBuffer, int32_t iLength, const char *pString, int32_t iOutput, uint8_t bWideChar)
{
    const wchar_t *pWideStr;
    int32_t iIndex;

    if (!bWideChar)
    {
        for (iIndex = 0; pString[iIndex] != '\0'; iIndex += 1)
        {
            iOutput = _ds_writechar(pBuffer, iLength, pString[iIndex], iOutput);
        }
    }
    else
    {
        for (iIndex = 0, pWideStr = (const wchar_t *)pString; pWideStr[iIndex] != '\0\0'; iIndex += 1)
        {
            iOutput = _ds_writechar(pBuffer, iLength, (char)pWideStr[iIndex], iOutput);
        }
    }
    return(iOutput);
}

/*F********************************************************************************/
/*!
    \Function _ds_printstr

    \Description
        Print string to output buffer, with leading character/justification/padding.
        As per C99 specifications, if the output does not fit in the given buffer,
        this function will return the number of characters that would have been written
        if the buffer were big enough, but will truncate the string to preserve buffer
        integrity.

    \Input *pBuffer     - [out] output buffer
    \Input iLength      - size of output buffer
    \Input *pString     - string to print
    \Input iOutput      - offset in output buffer to print to
    \Input iWidth       - field width
    \Input bRightJust   - if TRUE, right-justify output
    \Input bWideChar    - if TRUE, input string uses wide (wchar_t) chars
    \Input cLead        - leading character or zero if none
    \Input cPad         - character to pad output with if width is larger than string length

    \Output
        int32_t         - number of characters written to output buffer

    \Version 10/28/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ds_printstr(char *pBuffer, int32_t iLength, const char *pString, int32_t iOutput, int32_t iWidth, uint8_t bRightJust, uint8_t bWideChar, char cPad, char cLead)
{
    int32_t iStrLen;
    // allow null pointer for source string
    if (pString == NULL)
    {
        pString = "(null)";
    }
    // calculate field width
    if (iWidth > 0)
    {
        iStrLen = (int32_t) strlen(pString);
        iWidth = (iStrLen < iWidth) ? iWidth - iStrLen : 0;
    }
    // handle right justification
    if (bRightJust)
    {
        // handle lead character
        if (cLead != 0)
        {
            // only add the lead character if we are not padding with spaces
            if (cPad != ' ')
            {
                iOutput = _ds_writechar(pBuffer, iLength, cLead, iOutput);
                cLead = 0;
            }
            // always decrement width
            if (iWidth > 0)
            {
                iWidth -= 1;
            }
        }
        for ( ; iWidth > 0; iWidth -= 1)
        {
            iOutput = _ds_writechar(pBuffer, iLength, cPad, iOutput);
        }
    }
    // handle lead character
    if (cLead != 0)
    {
        iOutput = _ds_writechar(pBuffer, iLength, cLead, iOutput);
        // only decrement width if we are not right justfied, in which case we've already done it
        if ((bRightJust == FALSE) && (iWidth > 0))
        {
            iWidth -= 1;
        }
    }
    // write string to buffer
    iOutput = _ds_writestr(pBuffer, iLength, pString, iOutput, bWideChar);
    // handle left justification
    for ( ; iWidth > 0; iWidth -= 1)
    {
        iOutput = _ds_writechar(pBuffer, iLength, cPad, iOutput);
    }
    return(iOutput);
}

/*F********************************************************************************/
/*!
    \Function _ds_uinttostr

    \Description
        Convert an input unsigned integer into a string.

    \Input *pBuffer     - [out] output buffer
    \Input iBufSize     - size of output buffer
    \Input uInteger     - unsigned integer to print
    \Input uBase        - output integer base
    \Input *pHexTbl     - pointer to hex conversion table (upper or lower case)

    \Output
        char *          - pointer to output string

    \Version 10/28/2009 (jbrookes)
*/
/********************************************************************************F*/
static char *_ds_uinttostr(char *pBuffer, int32_t iBufSize, uint64_t uInteger, uint32_t uBase, const char *pHexTbl)
{
    int32_t iIndex = iBufSize-1;
    uint32_t uDigit;

    pBuffer[iIndex--] = '\0';

    if (uInteger != 0)
    {
        for ( ; ; iIndex -= 1)
        {
            uDigit = uInteger % uBase;
            pBuffer[iIndex] = pHexTbl[uDigit];
            if ((uInteger /= uBase) == 0)
            {
                break;
            }
        }
    }
    else
    {
        pBuffer[iIndex] = '0';
    }

    return(pBuffer+iIndex);
}

/*F********************************************************************************/
/*!
    \Function _ds_inttostr

    \Description
        Convert an input integer into a string.

    \Input *pBuffer     - [out] output buffer
    \Input iBufSize     - size of output buffer
    \Input iInteger     - integer to print
    \Input uBase        - output integer base
    \Input *pHexTbl     - pointer to hex conversion table (upper or lower case)
    \Input bPrintPlus   - if TRUE, include + character on positive integers
    \Input *pLead       - [out] leading character to print (+/- or none)

    \Output
        char *          - pointer to output string

    \Version 10/28/2009 (jbrookes)
*/
/********************************************************************************F*/
static char *_ds_inttostr(char *pBuffer, int32_t iBufSize, int64_t iInteger, uint32_t uBase, const char *pHexTbl, uint8_t bPrintPlus, char *pLead)
{
    uint8_t bNegative = FALSE;
    if (iInteger < 0)
    {
        bNegative = TRUE;
        iInteger = -iInteger;
    }
    pBuffer = _ds_uinttostr(pBuffer, iBufSize, (uint64_t)iInteger, uBase, pHexTbl);
    if (bNegative)
    {
        *pLead = '-';
    }
    else if (bPrintPlus)
    {
        *pLead = '+';
    }
    return(pBuffer);
}

/*F********************************************************************************/
/*!
    \Function _ds_fcvt

    \Description
        Convert a double into a string (like fcvt()).

    \Input *pBuffer     - [out] output buffer
    \Input iBufSize     - size of output buffer
    \Input fDouble      - double to print
    \Input iPrecision   - number of digits to include in mantissa
    \Input *pDecimalPos - [out] storage for decimal point offset
    \Input *pSign       - [out] 0=positive, 1=negative

    \Output
        char *          - pointer to output string

    \Notes
            Format of a double
            seeeeeee eeeeffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff (1 11 52)

    \Version 05/03/2010 (jbrookes)
*/
/********************************************************************************F*/
static char *_ds_fcvt(char *pBuffer, int32_t iBufSize, double fDouble, int32_t iPrecision, int32_t *pDecimalPos, int32_t *pSign)
{
    char strFloat[128] = "0", strInt[128], *pStrInt, *pStrFloat, *pStrFloatRnd, *pStrFloatStrt;
    uint64_t uDouble, uMantissa, uDivisor, uDigit = 0;
    int32_t iExponent, iFloatBufSize = sizeof(strFloat) - 1, iShift;
    uint32_t uSign, uUpper;

    ds_memcpy_s(&uDouble, sizeof(uDouble), &fDouble, sizeof(fDouble));
    uUpper = (uint32_t)(uDouble >> 32);
    iExponent = (int32_t)((uUpper >> 20) & 0x7ff) - 1023;
    uMantissa = ((uDouble << 12) >> 12) | ((uint64_t)1 << 52);
    uSign = uUpper >> 31;

    *pSign = uSign;
    *pDecimalPos = 0;
    pStrFloat = pStrFloatStrt = &strFloat[1];

    // right-bias mantissa
    for (iShift = 0; (uMantissa & 1) != 1 && (52-iExponent-iShift) > 0; iShift += 1)
    {
        uMantissa >>= 1;
    }

    // give us a little more room to handle very large numbers
    if (iExponent > 52)
    {
        if ((iShift = iExponent - 52) > 11)
        {
            // too big to fit in a 64bit integer
            ds_strnzcpy(pBuffer, "(BIG)", iBufSize);
            return(pBuffer);
        }
        // left-bias mantissa to reduce exponent
        uMantissa <<= iShift;
        iExponent -= iShift;
        iShift = 0;
    }

    // for very small numbers; right-shift (losing precision) until our integer divisor is within range
    for ( ; (52-iExponent-iShift) > 62; iShift += 1)
    {
        uMantissa >>= 1;
    }

    uDivisor = (uint64_t)1 << (52-iExponent-iShift);

    // integer part
    if (uMantissa >= uDivisor)
    {
        uDigit = uMantissa / uDivisor;
        pStrInt = _ds_uinttostr(strInt, sizeof(strInt), uDigit, 10, _ds_strhexlower);
        ds_strnzcpy(pStrFloat, pStrInt, iFloatBufSize);
        *pDecimalPos = (int32_t)strlen(pStrInt);
        pStrFloat += *pDecimalPos;
        uMantissa -= uDigit*uDivisor;
        uDigit = 0;
    }

    // fractional part
    for ( ; uMantissa > 0; iPrecision -= 1)
    {
        uMantissa = uMantissa * 10;
        uDigit = uMantissa / uDivisor;
        if (iPrecision == 0)
        {
            break;
        }
        *pStrFloat++ = (uint8_t)(uDigit + '0');
        uMantissa -= uDigit*uDivisor;
    }

    // round?
    if ((iPrecision == 0) && (uDigit >= 5))
    {
        // null-terminate
        *pStrFloat = '\0';

        for (pStrFloatRnd = pStrFloat - 1; ; pStrFloatRnd -= 1)
        {
            *pStrFloatRnd += 1;
            if (*pStrFloatRnd <= '9')
            {
                break;
            }
            *pStrFloatRnd = '0';
        }

        if (pStrFloatRnd == strFloat)
        {
            pStrFloatStrt = strFloat;
            *pDecimalPos += 1;
        }
    }
    else
    {
        // pad with zeros
        for ( ; iPrecision > 0; iPrecision -= 1)
        {
            *pStrFloat++ = '0';
        }

        // null-terminate
        *pStrFloat++ = '\0';
    }

    // copy to output and return to caller
    ds_strnzcpy(pBuffer, pStrFloatStrt, iBufSize);
    return(pBuffer);
}

/*F********************************************************************************/
/*!
    \Function _ds_floattostr

    \Description
        Convert an 32bit floating point number into a string.

    \Input *pBuffer     - [out] output buffer
    \Input iBufSize     - size of output buffer
    \Input fDouble      - double to print
    \Input iPrecision   - number of digits to include in mantissa
    \Input bPrintPlus   - if TRUE, include + character on positive integers
    \Input *pLead       - [out] leading character to print (+/- or none)

    \Output
        char *          - pointer to output string

    \Version 05/03/2010 (jbrookes)
*/
/********************************************************************************F*/
static char *_ds_floattostr(char *pBuffer, int32_t iBufSize, double fDouble, int32_t iPrecision, uint32_t bPrintPlus, char *pLead)
{
    int32_t iDecimalPos, iSign, iIndex = 0;
    char strFloat[128], strFmtFloat[128];

    // default precision
    if (iPrecision < 0)
    {
        iPrecision = 6; //c99 default
    }

    // float to string
#if 0
    ds_strnzcpy(strFloat, fcvt(fDouble, iPrecision, &iDecimalPos, &iSign), sizeof(strFloat));
#else
    _ds_fcvt(strFloat, sizeof(strFloat), fDouble, iPrecision, &iDecimalPos, &iSign);
#endif

    // handle sign
    if (iSign != 0)
    {
        *pLead = '-';
    }
    else if (bPrintPlus)
    {
        *pLead = '+';
    }

    // copy integer portion
    if (iDecimalPos > 0)
    {
        ds_strsubzcpy(&strFmtFloat[iIndex], sizeof(strFmtFloat) - iIndex, strFloat, iDecimalPos);
        iIndex += iDecimalPos;
    }
    else
    {
        strFmtFloat[iIndex++] = '0';
    }

    // add decimal point and fractional portion
    if (strFloat[iDecimalPos] != '\0')
    {
        strFmtFloat[iIndex++] = '.';
    }
    for ( ; iDecimalPos < 0 && iPrecision > 0; iDecimalPos += 1, iPrecision -= 1)
    {
        strFmtFloat[iIndex++] = '0';
    }
    if (iDecimalPos >= 0)
    {
        ds_strnzcpy(&strFmtFloat[iIndex], &strFloat[iDecimalPos], sizeof(strFmtFloat)-iIndex);
    }
    else
    {
        strFmtFloat[iIndex++] = '\0';
    }

    // copy to buffer
    ds_strnzcpy(pBuffer, strFmtFloat, iBufSize);
    return(pBuffer);
}

/*F********************************************************************************/
/*!
    \Function _ds_ptrtostr

    \Description
        Convert an input integer into a string.

    \Input *pBuffer     - [out] output buffer
    \Input iBufSize     - size of output buffer
    \Input *pPointer    - pointer value to print

    \Output
        char *          - pointer to output string

    \Version 10/28/2009 (jbrookes)
*/
/********************************************************************************F*/
static char *_ds_ptrtostr(char *pBuffer, int32_t iBufSize, void *pPointer)
{
    if (pPointer == NULL)
    {
        ds_strnzcpy(pBuffer, "(null)", iBufSize);
        return(pBuffer);
    }
    pBuffer = _ds_uinttostr(pBuffer, iBufSize, (uintptr_t)pPointer, 16, _ds_strhexlower);
    return(pBuffer);
}

/*F********************************************************************************/
/*!
    \Function _ds_sockaddrtostr

    \Description
        Convert an input sockaddr into string format (supports AF_INET and AF_INET6)
        Important on the nx only AF_INET addresses arew supported

    \Input *pBuffer     - [out] output buffer
    \Input iBufSize     - size of output buffer
    \Input *pSockAddr   - address to print

    \Output
        char *          - pointer to output string

    \Notes
        Reference: http://tools.ietf.org/html/rfc5952#section-4

    \Version 04/24/2013 (jbrookes)
*/
/********************************************************************************F*/
static char *_ds_sockaddrtostr(char *pBuffer, int32_t iBufSize, struct sockaddr *pSockAddr)
{
    char *pBufStart = pBuffer;
    char strPort[16] = "", strAddr[48];
    uint16_t uPort = 0;

    // format port string, if non-zero
    #ifdef DIRTYCODE_NX
    uPort = SockaddrInGetPort(pSockAddr);
    #else
    struct sockaddr_in6 *pSockAddr6 = (struct sockaddr_in6 *)pSockAddr;
    uPort = (pSockAddr->sa_family == AF_INET) ? SockaddrInGetPort(pSockAddr) : SocketHtons(pSockAddr6->sin6_port);
    #endif
    
    if (uPort != 0)
    {
        ds_snzprintf(strPort, sizeof(strPort), ":%d", uPort);
    }
    // format the address
    ds_snzprintf(pBuffer, iBufSize, "[%s]%s", SockaddrInGetAddrText(pSockAddr, strAddr, sizeof(strAddr)), strPort);

    // return buffer to caller
    return(pBufStart);
}


/*F********************************************************************************/
/*!
    \Function _ds_fourcctostr

    \Description
        Convert an input fourcc code into a string.  Unprintable characters
        are converted to asterisks ('*').

    \Input *pBuffer     - [out] output buffer
    \Input iBufSize     - size of output buffer
    \Input uFourCC      - fourcc code to print

    \Output
        char *          - pointer to output string

    \Version 10/28/2009 (jbrookes)
*/
/********************************************************************************F*/
static char *_ds_fourcctostr(char *pBuffer, int32_t iBufSize, uint32_t uFourCC)
{
    int32_t iCode;
    for (iCode = 3; iCode >= 0; iCode -= 1)
    {
        pBuffer[iCode] = (char)(uFourCC & 0xff);
        uFourCC >>= 8;
        if (!isprint((uint8_t)pBuffer[iCode]))
        {
            pBuffer[iCode] = '*';
        }
    }
    pBuffer[4] = '\0';
    return(pBuffer);
}

/*F********************************************************************************/
/*!
    \Function _ds_vsnprintf

    \Description
        Replacement for vsnprintf, with some modifications.

    \Input *pBuffer - output buffer
    \Input iLength  - size of output buffer
    \Input *pFormat - format string
    \Input Args     - variable argument list

    \Output
        int32_t     - number of characters formatted; if > than the length, output
                      is truncated and the buffer is null-terminated unless iLength<=0

    \Notes
        Unlike a standard implementation of vsnprintf(), this version always
        null-terminates unless the buffer size is zero.  Overflow semantics follow
        the C99 standard.

        \verbatim
            Most ISO printf functionality is supported, with the following exceptions:

                - Floating point is supported with the following limitations:
                    - All floating-point formatters behave like 'f'
                    - Very large (> 2^63) floating point numbers will return (BIG)
                - The # flag is not supported, although it is consumed.
                - Length is not supported, although length specifiers are consumed.

            Additionally, the following extension types replace their normal meaning:

                - %a: print 32-bit address in dot notation (32-bit integer)
                - %A: print a sockaddr (IPv4 or IPv6)
                - %C: print fourcc code (32-bit integer)
        \endverbatim

    \Version 10/28/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ds_vsnprintf(char *pBuffer, int32_t iLength, const char *pFormat, va_list Args)
{
    int32_t iInput, iOutput, iTerm;
    char strTempBuf[64], *pString;
    char cInput;

    // handle format list
    for (iInput = 0, iOutput = 0; ; )
    {
        // read input character
        if ((cInput = pFormat[iInput++]) == '\0')
        {
            break;
        }

        // process a format specifier
        if (cInput == '%')
        {
            int32_t iPrecision = -1, iSize = 4, iWidth = 0;
            uint8_t bRightJust = TRUE, bPrintPlus = FALSE, bWideChar = FALSE;
            char cPad = ' ', cLead = 0;

            // handle end of string
            if (pFormat[iInput] == '\0')
            {
                break;
            }

            // process %%
            if (pFormat[iInput] == '%')
            {
                iOutput = _ds_writechar(pBuffer, iLength, pFormat[iInput], iOutput);
                iInput += 1;
                continue;
            }
            // process flags
            if (pFormat[iInput] == '-')
            {
                iInput += 1;
                bRightJust = FALSE;
            }
            if (pFormat[iInput] == '+')
            {
                iInput += 1;
                bPrintPlus = TRUE;
            }
            if (pFormat[iInput] == '#')
            {
                // unsupported
                iInput += 1;
            }
            if (pFormat[iInput] == '0')
            {
                iInput += 1;
                cPad = '0';
            }
            // process width
            if (pFormat[iInput] == '*')
            {
                iInput += 1;
                iWidth = va_arg(Args, int);
            }
            for ( ; (pFormat[iInput] >= '0') && (pFormat[iInput] <= '9'); iInput += 1)
            {
                iWidth *= 10;
                iWidth += pFormat[iInput] - '0';
            }

            // precision specifier?
            if (pFormat[iInput] == '.')
            {
                // process period and precision width
                for (iPrecision = 0, iInput += 1; (pFormat[iInput] >= '0') && (pFormat[iInput] <= '9'); iInput += 1)
                {
                    iPrecision *= 10;
                    iPrecision += pFormat[iInput] - '0';
                }

                // process asterisk (arg width) if present
                if (pFormat[iInput] == '*')
                {
                    iInput += 1;
                    iWidth = va_arg(Args, int);
                }
            }

            // check for length specifier
            cInput = pFormat[iInput++];
            if ((cInput == 'h') || (cInput == 'l') || (cInput == 'L') || (cInput == 'z') || (cInput == 'j') || (cInput == 't') || (cInput == 'q') || (cInput == 'I'))
            {
                if (cInput == 'h')
                {
                    iSize = 2;
                }
                if (cInput == 'q')
                {
                    iSize = 8;
                }
                if ((cInput == 'l') && (pFormat[iInput] == 's'))
                {
                    iSize = 8;
                }
                if (cInput == 'I')
                {
                    if ((pFormat[iInput] == '6') && (pFormat[iInput+1] == '4'))
                    {
                        iSize = 8;
                    }
                    iInput += 2;
                }
                cInput = pFormat[iInput++];
                if ((cInput == 'h') || (cInput == 'l'))
                {
                    if (cInput == 'h')
                    {
                        iSize = 1;
                    }
                    if (cInput == 'l')
                    {
                        iSize = 8;
                    }
                    cInput = pFormat[iInput++];
                }
            }

            // process type specifier
            switch(cInput)
            {
                case 'a':
                {
                    uint32_t uAddress = va_arg(Args, unsigned int);
                    pString = SocketInAddrGetText(uAddress, strTempBuf, sizeof(strTempBuf));
                    iOutput = _ds_printstr(pBuffer, iLength, pString, iOutput, iWidth, bRightJust, bWideChar, cPad, 0);
                }
                break;

                case 'A':
                {
                    struct sockaddr *pSockAddr = va_arg(Args, struct sockaddr *);
                    pString = _ds_sockaddrtostr(strTempBuf, sizeof(strTempBuf), pSockAddr);
                    iOutput = _ds_printstr(pBuffer, iLength, pString, iOutput, iWidth, bRightJust, bWideChar, cPad, 0);
                }
                break;

                case 'c':
                {
                    strTempBuf[0] = (char)va_arg(Args, int);
                    strTempBuf[1] = '\0';
                    iOutput = _ds_printstr(pBuffer, iLength, strTempBuf, iOutput, iWidth, bRightJust, bWideChar, cPad, 0);
                }
                break;

                case 'C':
                {
                    uint32_t uFourCC = va_arg(Args, unsigned int);
                    pString = _ds_fourcctostr(strTempBuf, sizeof(strTempBuf), uFourCC);
                    iOutput = _ds_printstr(pBuffer, iLength, pString, iOutput, iWidth, bRightJust, bWideChar, cPad, 0);
                }
                break;

                case 'd':
                case 'i':
                {
                    int64_t iInteger = (iSize <= 4) ? va_arg(Args, int) : va_arg(Args, long long);
                    pString = _ds_inttostr(strTempBuf, sizeof(strTempBuf), iInteger, 10, _ds_strhexlower, bPrintPlus, &cLead);
                    iOutput = _ds_printstr(pBuffer, iLength, pString, iOutput, iWidth, bRightJust, bWideChar, cPad, cLead);
                }
                break;

                case 'n':
                {
                    int *pInteger = va_arg(Args, int *);
                    *pInteger = iOutput;
                }
                break;

                case 'o':
                {
                    uint64_t uInteger = (iSize <= 4) ? va_arg(Args, unsigned int) : va_arg(Args, unsigned long long);
                    pString = _ds_uinttostr(strTempBuf, sizeof(strTempBuf), uInteger, 8, _ds_strhexlower);
                    iOutput = _ds_printstr(pBuffer, iLength, pString, iOutput, iWidth, bRightJust, bWideChar, cPad, 0);
                }
                break;

                case 'p':
                {
                    void *pPointer = va_arg(Args, void *);
                    if (pPointer != NULL)
                    {
                        iWidth = sizeof(void *) * 2 + 1;    // always print full width (+1 for the leading '$')
                        cLead = '$';
                    }
                    pString = _ds_ptrtostr(strTempBuf, sizeof(strTempBuf), pPointer);
                    iOutput = _ds_printstr(pBuffer, iLength, pString, iOutput, iWidth, bRightJust, bWideChar, '0', cLead); // always print leading zeros
                }
                break;

                case 'S':
                    iSize = 8;
                case 's':
                {
                    pString = va_arg(Args, char *);
                    bWideChar = (iSize == 8) ? TRUE : FALSE;
                    iOutput = _ds_printstr(pBuffer, iLength, pString, iOutput, iWidth, bRightJust, bWideChar, cPad, 0);
                }
                break;

                case 'u':
                {
                    uint64_t uInteger = (iSize <= 4) ? va_arg(Args, unsigned int) : va_arg(Args, unsigned long long);
                    pString = _ds_uinttostr(strTempBuf, sizeof(strTempBuf), uInteger, 10, _ds_strhexlower);
                    iOutput = _ds_printstr(pBuffer, iLength, pString, iOutput, iWidth, bRightJust, bWideChar, cPad, 0);
                }
                break;

                case 'x':
                case 'X':
                {
                    uint64_t uInteger = (iSize <= 4) ? va_arg(Args, unsigned int) : va_arg(Args, unsigned long long);
                    pString = _ds_uinttostr(strTempBuf, sizeof(strTempBuf), uInteger, 16, cInput == 'x' ? _ds_strhexlower : _ds_strhexupper);
                    iOutput = _ds_printstr(pBuffer, iLength, pString, iOutput, iWidth, bRightJust, bWideChar, cPad, 0);
                }
                break;

                // all floating-point handled like 'f'
                case 'e':
                case 'E':
                case 'f':
                case 'F':
                case 'g':
                case 'G':
                {
                    double fDouble = va_arg(Args, double);
                    pString = _ds_floattostr(strTempBuf, sizeof(strTempBuf), fDouble, iPrecision, bPrintPlus, &cLead);
                    iOutput = _ds_printstr(pBuffer, iLength, pString, iOutput, iWidth, bRightJust, bWideChar, cPad, cLead);
                }
                break;

                default:
                {
                    // should not hit this; unsupported type specifier?
                }
                break;
            }
        }
        else
        {
            iOutput = _ds_writechar(pBuffer, iLength, cInput, iOutput);
        }
    }

    // locate terminating null
    iTerm = (iOutput < iLength) ? iOutput : iLength-1;

    // terminate if possible
    if (iLength > 0)
    {
        pBuffer[iTerm] = '\0';
    }
    // return length of output to caller
    return(iOutput);
}

/*F********************************************************************************/
/*!
    \Function _ds_strcmpwc

    \Description
        String compare with wildcard matching ('*' only).

    \Input *pString1    - string to match
    \Input *pStrWild    - wildcard string
    \Input bNoCase      - TRUE if a case-insensitive comparison should be performed

    \Output
        int32_t         - see stricmp

    \Version 09/18/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ds_strcmpwc(const char *pString1, const char *pStrWild, uint8_t bNoCase)
{
    int32_t r;
    char c1, c2;

    do {
        c1 = *pString1++;
        c2 = *pStrWild;
        if (bNoCase)
        {
            if ((c1 >= 'A') && (c1 <= 'Z'))
            {
                c1 ^= 32;
            }
            if ((c2 >= 'A') && (c2 <= 'Z'))
            {
                c2 ^= 32;
            }
        }
        if ((c2 == '*') && (c1 != '\0'))
        {
            if ((r = _ds_strcmpwc(pString1, pStrWild+1, bNoCase)) == 0)
            {
                break;
            }
            r = 0;
        }
        else
        {
            pStrWild += 1;
            r = c1-c2;
        }
    } while ((c1 != '\0') && (c2 != '\0') && (r == 0));

    return(r);
}

/*F********************************************************************************/
/*!
    \Function _ds_fmtoctstring

    \Description
        Format a binary blob as hexadecmial text

    \Input *pOutput - [out] outbuffer for text
    \Input iOutLen  - size of output buffer
    \Input *pInput  - input binary data
    \Input iInpLen  - size of input data
    \Input *pHexTbl - hex table to use for translation

    \Output
        char *      - pointer to output buffer

    \Version 04/11/2017 (jbrookes)
*/
/********************************************************************************F*/
static char *_ds_fmtoctstring(char *pOutput, int32_t iOutLen, const uint8_t *pInput, int32_t iInpLen, const char *pHexTbl)
{
    int32_t iInput, iOutput;

    for (iInput = 0, iOutput = 0, iOutLen -= 1; (iInput < iInpLen) && (iOutput < (iOutLen-1)); iInput += 1, iOutput += 2)
    {
        uint8_t cByte = pInput[iInput];
        pOutput[iOutput+0] = pHexTbl[cByte >> 4];
        pOutput[iOutput+1] = pHexTbl[cByte & 0xf];
    }
    pOutput[iOutput] = '\0';

    return(pOutput);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ds_strnlen

    \Description
        Replacement for strnlen.

    \Input *pBuffer - buffer
    \Input iLength  - size of buffer

    \Output
        int32_t     - number of characters or iLength if no '\0' is present.

    \Version 10/28/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t ds_strnlen(const char *pBuffer, int32_t iLength)
{
    int32_t iLen = 0;

    while (iLen < iLength && *(pBuffer+iLen) != '\0')
    {
        ++iLen;
    }

    return(iLen);
}

/*F********************************************************************************/
/*!
    \Function ds_vsnprintf

    \Description
        Replacement for vsnprintf, with some modifications.

    \Input *pBuffer - output buffer
    \Input iLength  - size of output buffer
    \Input *pFormat - format string
    \Input Args     - variable argument list

    \Output
        int32_t     - number of characters formatted; if > than the length, output
                      is truncated and the buffer is null-terminated unless iLength<=0.

    \Notes
        Unlike standard vsnprintf, this version always null-terminates if the buffer
        is not zero-sized.

    \Version 10/28/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t ds_vsnprintf(char *pBuffer, int32_t iLength, const char *pFormat, va_list Args)
{
    return(_ds_vsnprintf(pBuffer, iLength, pFormat, Args));
}

/*F********************************************************************************/
/*!
    \Function ds_vsnzprintf

    \Description
        Replacement for vsnprintf that always includes a terminator at the end of
        the string.

    \Input *pBuffer - output buffer
    \Input iLength  - size of output buffer
    \Input *pFormat - format string
    \Input Args     - variable argument list

    \Output
        int32_t     - number of characters formatted; if > than the length, output
                      is truncated and the buffer is null-terminated unless iLength<=0.

    \Version 02/15/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ds_vsnzprintf(char *pBuffer, int32_t iLength, const char *pFormat, va_list Args)
{
    return(_ds_vsnprintf(pBuffer, iLength, pFormat, Args));
}

/*F********************************************************************************/
/*!
    \Function ds_snzprintf

    \Description
        Replacement for snprintf that always includes a terminator at the end of
        the string.

    \Input *pBuffer - output buffer
    \Input iLength  - size of output buffer
    \Input *pFormat - format string
    \Input ...      - variable argument list

    \Output
        int32_t     - number of characters formatted; if > than the length, output
                      is truncated and the buffer is null-terminated unless iLength<=0.

    \Version 02/15/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ds_snzprintf(char *pBuffer, int32_t iLength, const char *pFormat, ...)
{
    int32_t iResult;
    va_list args;

    // format the text
    va_start(args, pFormat);
    iResult = _ds_vsnprintf(pBuffer, iLength, pFormat, args);
    va_end(args);

    // return result to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ds_fmtoctstring

    \Description
        Format a binary blob as hexadecmial text

    \Input *pOutput - [out] outbuffer for text
    \Input iOutLen  - size of output buffer
    \Input *pInput  - input binary data
    \Input iInpLen  - size of input data

    \Output
        char *      - pointer to output buffer

    \Version 04/11/2017 (jbrookes)
*/
/********************************************************************************F*/
char *ds_fmtoctstring(char *pOutput, int32_t iOutLen, const uint8_t *pInput, int32_t iInpLen)
{
    return(_ds_fmtoctstring(pOutput, iOutLen, pInput, iInpLen, "0123456789ABCDEF"));
}

/*F********************************************************************************/
/*!
    \Function ds_fmtoctstring_lc

    \Description
        Format a binary blob as hexadecmial text, with the alphabetic characters
        being lowercase

    \Input *pOutput - [out] outbuffer for text
    \Input iOutLen  - size of output buffer
    \Input *pInput  - input binary data
    \Input iInpLen  - size of input data

    \Output
        char *      - pointer to output buffer

    \Version 04/11/2017 (jbrookes)
*/
/********************************************************************************F*/
char *ds_fmtoctstring_lc(char *pOutput, int32_t iOutLen, const uint8_t *pInput, int32_t iInpLen)
{
    return(_ds_fmtoctstring(pOutput, iOutLen, pInput, iInpLen, "0123456789abcdef"));
}

/*F********************************************************************************/
/*!
    \Function ds_strtolower

    \Description
        Convert given string to lower case

    \Input *pString     - string to convert

    \Output
        char *          - pointer to string

    \Version 12/28/2019 (jbrookes)
*/
/********************************************************************************F*/
char *ds_strtolower(char *pString)
{
    char *pStrStart = pString;
    for (; *pString != '\0'; pString += 1)
    {
        if ((*pString >= 'A') && (*pString <= 'Z'))
        {
            *pString += 0x20;
        }
    }
    return(pStrStart);
}

/*F********************************************************************************/
/*!
    \Function ds_strtoupper

    \Description
        Convert given string to upper case

    \Input *pString     - string to convert

    \Output
        char *          - pointer to string

    \Version 12/28/2019 (jbrookes)
*/
/********************************************************************************F*/
char *ds_strtoupper(char *pString)
{
    char *pStrStart = pString;
    for (; *pString != '\0'; pString += 1)
    {
        if ((*pString >= 'a') && (*pString <= 'z'))
        {
            *pString -= 0x20;
        }
    }
    return(pStrStart);
}

/*F********************************************************************************/
/*!
    \Function ds_strtrim

    \Description
        Removes excess white space before and after values, and converts sequential
        spaces to a single space.

    \Input *pString         - [out] string to trim

    \Output
        char *              - pointer to string

    \Version 12/28/2018 (jbrookes)
*/
/********************************************************************************F*/
char *ds_strtrim(char *pString)
{
    char *pStrStart = pString, *pStrTrim;
    // skip leading whitespace
    for (; *pString == ' '; pString += 1)
        ;
    // copy characters, converting sequential spaces into a single space
    for (pStrTrim = pString; *pString != '\0'; )
    {
        *pStrTrim++ = *pString;
        if (*pString == ' ')
        {
            // eat multiple spaces
            for (; *pString == ' '; pString += 1)
                ;
        }
        else
        {
            pString += 1;
        }
    }
    // terminate string
    *pString = '\0';
    // return start of string
    return(pStrStart);
}

/*F********************************************************************************/
/*!
    \Function ds_strlistinsert

    \Description
        Insert string into string list buffer in sorted order, terminated by
        the specified terminator.  The list terminator may not be nul and it must
        be a character that does not appear in strings inserted in the list.

    \Input *pStrList    - [in/out] buffer to hold string list
    \Input iListBufLen  - length of list buffer
    \Input *pString     - string to insert in string list
    \Input cTerm        - string terminator

    \Output
        int32_t         - positive on success, zero if string could not be inserted

    \Version 12/29/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t ds_strlistinsert(char *pStrList, int32_t iListBufLen, const char *pString, char cTerm)
{
    char *pStrInsert, *pStrSkip;
    int32_t iStrLen, iListLen;

    // get length of string we're inserting
    iStrLen = (int32_t)strlen(pString);
    // get length of strlist
    iListLen = (int32_t)strlen(pStrList);
    // make sure we have room to insert
    if (iListBufLen < (iListLen+iStrLen+1))
    {
        return(0);
    }

    // find insertion point
    for (pStrInsert = pStrSkip = pStrList; pStrSkip != NULL; )
    {
        if (ds_stricmp(pStrInsert, pString) >= 0)
        {
            break;
        }
        pStrInsert = pStrSkip;
        if ((pStrSkip = strchr(pStrSkip, cTerm)) != NULL)
        {
            pStrSkip += 1;
        }
    }

    // make room for formatted header if necessary
    if (*pStrInsert != '\0')
    {
        // get length of list from here to end of the buffer
        iListLen  = (int32_t)strlen(pStrInsert);
        // make room for string, plus terminator character
        memmove(pStrInsert+iStrLen+1, pStrInsert, iListLen);
        // point to insertion point
        iListBufLen -= pStrInsert-pStrList;
        pStrList = pStrInsert;
    }
    else
    {
        // end of list, so null terminate
        pStrInsert[iStrLen+1] = '\0';
    }

    // copy formatted header into list and terminate it
    ds_memcpy_s(pStrInsert, iListBufLen, pString, iStrLen);
    pStrInsert[iStrLen] = cTerm;
    return(1);
}

/*F********************************************************************************/
/*!
    \Function ds_stristr

    \Description
        Case insensitive substring search

    \Input pHaystack    - see stristr
    \Input pNeedle      - see stristr

    \Output see stristr

    \Version 01/25/2005 (gschaefer)
*/
/********************************************************************************F*/
char *ds_stristr(const char *pHaystack, const char *pNeedle)
{
    int32_t iFirst;
    int32_t iIndex;

    // make sure strings are valid
    if ((pHaystack != NULL) && (*pHaystack != 0) && (pNeedle != NULL) && (*pNeedle != 0))
    {
        iFirst = tolower((unsigned char)*pNeedle);
        for (; *pHaystack != 0; ++pHaystack)
        {
            // small optimization on first character search
            if (tolower((unsigned char)*pHaystack) == iFirst)
            {
                for (iIndex = 1;; ++iIndex)
                {
                    if (pNeedle[iIndex] == 0)
                    {
                        return((char *)pHaystack);
                    }
                    if (pHaystack[iIndex] == 0)
                    {
                        break;
                    }
                    if (tolower((unsigned char)pHaystack[iIndex]) != tolower((unsigned char)pNeedle[iIndex]))
                    {
                        break;
                    }
                }
            }
        }
    }
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function ds_stricmp

    \Description
        Case insensitive string compare

    \Input *pString1    - see stricmp
    \Input *pString2    - see stricmp

    \Output see stricmp

    \Version 01/25/2005 (gschaefer)
*/
/********************************************************************************F*/
int32_t ds_stricmp(const char *pString1, const char *pString2)
{
    int32_t r;
    char c1, c2;

    do {
        c1 = *pString1++;
        if ((c1 >= 'A') && (c1 <= 'Z'))
            c1 ^= 32;
        c2 = *pString2++;
        if ((c2 >= 'A') && (c2 <= 'Z'))
            c2 ^= 32;
        r = c1-c2;
    } while ((c1 != 0) && (r == 0));

    return(r);
}

/*F********************************************************************************/
/*!
    \Function ds_strnicmp

    \Description
        Case insensitive string compare of first N characters

    \Input *pString1    - see strnicmp
    \Input *pString2    - see strnicmp
    \Input uCount       - see strnicmp

    \Output see strnicmp

    \Version 01/25/2005 (gschaefer)
*/
/********************************************************************************F*/
int32_t ds_strnicmp(const char *pString1, const char *pString2, uint32_t uCount)
{
    int32_t r;
    char c1, c2;
    uint32_t uPos;

    if (uCount == 0)
        return(0);

    uPos = 0;
    do {
        c1 = *pString1++;
        if ((c1 >= 'A') && (c1 <= 'Z'))
            c1 ^= 32;
        c2 = *pString2++;
        if ((c2 >= 'A') && (c2 <= 'Z'))
            c2 ^= 32;
        r = c1-c2;
        uPos++;
    } while ((c1 != 0) && (r == 0) && (uPos < uCount));

    return(r);
}

/*F********************************************************************************/
/*!
    \Function ds_strcmpwc

    \Description
        String compare with wildcard matching ('*' only).

    \Input *pString1    - see stricmp
    \Input *pStrWild    - match string including wildcard(s)

    \Output see stricmp

    \Version 09/18/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t ds_strcmpwc(const char *pString1, const char *pStrWild)
{
    return(_ds_strcmpwc(pString1, pStrWild, FALSE));
}

/*F********************************************************************************/
/*!
    \Function ds_stricmpwc

    \Description
        String compare with wildcard matching ('*' only).

    \Input *pString1    - see stricmp
    \Input *pStrWild    - match string including wildcard(s)

    \Output see stricmp

    \Version 09/18/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t ds_stricmpwc(const char *pString1, const char *pStrWild)
{
    return(_ds_strcmpwc(pString1, pStrWild, TRUE));
}

/*F********************************************************************************/
/*!
    \Function ds_strnzcpy

    \Description
        Always terminated strncpy function

    \Input *pDest   - [out] output for new string
    \Input *pSource - pointer to input string to copy from
    \Input iCount   - length of output buffer

    \Output
        char *      - pointer to output buffer

    \Notes
        Unlike strncpy(), the destination buffer is not filled with zeros if the
        string being copied does not fill it.

    \Version 01/11/2017 (jbrookes)
*/
/********************************************************************************F*/
char *ds_strnzcpy(char *pDest, const char *pSource, int32_t iCount)
{
    int32_t iIndex;

    // make sure buffer has enough room
    if (--iCount < 0)
    {
        return(0);
    }

    // copy the string
    for (iIndex = 0; (iIndex < iCount) && (pSource[iIndex] != '\0'); iIndex += 1)
    {
        pDest[iIndex] = pSource[iIndex];
    }

    // write null terminator and return number of bytes written
    pDest[iIndex] = '\0';
    return(pDest);
}

/*F********************************************************************************/
/*!
    \Function ds_strsubzcpy

    \Description
        Copy a substring from pSrc into the output buffer.  The output string
        is guaranteed to be null-terminated.

    \Input *pDst    - [out] output for new string
    \Input iDstLen  - length of output buffer
    \Input *pSrc    - pointer to input string to copy from
    \Input iSrcLen  - number of characters to copy from input string

    \Output
        int32_t     - number of characters written, excluding null character

    \Version 09/27/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ds_strsubzcpy(char *pDst, int32_t iDstLen, const char *pSrc, int32_t iSrcLen)
{
    int32_t iIndex;

    // make sure buffer has enough room
    if (--iDstLen < 0)
    {
        return(0);
    }

    // copy the string
    for (iIndex = 0; (iIndex < iSrcLen) && (iIndex < iDstLen) && (pSrc[iIndex] != '\0'); iIndex++)
    {
        pDst[iIndex] = pSrc[iIndex];
    }

    // write null terminator and return number of bytes written
    pDst[iIndex] = '\0';
    return(iIndex);
}

/*F********************************************************************************/
/*!
    \Function ds_strnzcat

    \Description
        Concatenate the string pointed to by pSrc to the string pointed to by pDst.
        A maximum of iDstLen-1 characters are copied, and the resulting string is
        guaranteed to be null-terminated.

    \Input *pDst    - [out] output for new string
    \Input *pSrc    - pointer to input string to copy from
    \Input iDstLen  - size of output buffer

    \Output
        int32_t         - number of characters in pDst, excluding null character

    \Version 09/27/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ds_strnzcat(char *pDst, const char *pSrc, int32_t iDstLen)
{
    int32_t iDst, iSrc;

    // make sure buffer has enough room
    if (--iDstLen < 0)
    {
        return(0);
    }

    // find end of string
    for (iDst = 0; (iDst < iDstLen) && (pDst[iDst] != '\0'); iDst++)
        ;

    // copy the string
    for (iSrc = 0; (iDst < iDstLen) && (pSrc[iSrc] != '\0'); iSrc++, iDst++)
    {
        pDst[iDst] = pSrc[iSrc];
    }

    // write null terminator and return updated length of string
    pDst[iDst] = '\0';
    return(iDst);
}

/*F********************************************************************************/
/*!
    \Function ds_strsubzcat

    \Description
        Concatenate the substring pointed to by pSrc and with a length of iSrcLen
        to the string pointed to by pDst.  A maximum of iDstLen-1 or iSrcLen
        characters are copied, whichever is smaller, and the resulting string is
        guaranteed to be null-terminated.

    \Input *pDst    - [out] output for new string
    \Input iDstLen  - size of output buffer
    \Input *pSrc    - pointer to input string to copy from
    \Input iSrcLen  - size of substring pointed to by pSrc.

    \Output
        int32_t         - number of characters in pDst, excluding null character

    \Version 09/27/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ds_strsubzcat(char *pDst, int32_t iDstLen, const char *pSrc, int32_t iSrcLen)
{
    int32_t iDst, iSrc;

    // make sure buffer has enough room
    if (--iDstLen < 0)
    {
        return(0);
    }

    // find end of string
    for (iDst = 0; (iDst < iDstLen) && (pDst[iDst] != '\0'); iDst++)
        ;

    // copy the string
    for (iSrc = 0; (iDst < iDstLen) && (iSrc < iSrcLen) && (pSrc[iSrc] != '\0'); iSrc++, iDst++)
    {
        pDst[iDst] = pSrc[iSrc];
    }

    // write null terminator and return updated length of string
    pDst[iDst] = '\0';
    return(iDst);

}

/*F********************************************************************************/
/*!
    \Function ds_strsplit

    \Description
        Splits string on a delimiter

    \Input *pSrc        - the string we are splitting
    \Input cDelimiter   - delimited to split on
    \Input *pDst        - where we are writing the data split
    \Input iDstSize     - size of dest string
    \Input **pNewSrc    - used to save position of iteration

    \Output
        int32_t         - length of destination or zero if nothing parsed

    \Version 08/04/2016 (eesponda)
*/
/********************************************************************************F*/
int32_t ds_strsplit(const char *pSrc, char cDelimiter, char *pDst, int32_t iDstSize, const char **pNewSrc)
{
    const char *pLocation;
    int32_t iSrcSize;

    // check for validity of inputs
    if ((pSrc == NULL) || (*pSrc == '\0'))
    {
        return(0);
    }
    iSrcSize = (int32_t)strlen(pSrc);

    /* if we start with delimiter skip it
       this happens when you do multiple parses */
    if (pSrc[0] == cDelimiter)
    {
        pSrc += 1;
        iSrcSize -= 1;
        (*pNewSrc) += 1;
    }

    // terminate the destination
    if (pDst != NULL && iDstSize > 0)
    {
        pDst[0] = '\0';
    }

    /* write the value to the destination (if not null)
       when length is less than max, add 1 for nul terminator */
    if ((pLocation = strchr(pSrc, cDelimiter)) != NULL)
    {
        const int32_t iCount = (int32_t)(pLocation - pSrc);
        (*pNewSrc) += iCount;
        if (pDst != NULL)
        {
            ds_strnzcpy(pDst, pSrc, DS_MIN(iDstSize, iCount+1));
        }
        return(iCount);
    }
    else
    {
        (*pNewSrc) += iSrcSize;
        if (pDst != NULL)
        {
            ds_strnzcpy(pDst, pSrc, DS_MIN(iDstSize, iSrcSize+1));
        }
        return(iSrcSize);
    }
}

/*F********************************************************************************/
/*!
    \Function ds_strtok_r

    \Description
        Reentrant version of strtok

    \Input *pStr        - input string
    \Input *pDelim      - delimiter
    \Input **ppSavePtr  - variable used to save down the state

    \Output
        char*       - reutrns the pointer to the next token

    \Version 09/07/2018 (tcho)
*/
/********************************************************************************F*/
char* ds_strtok_r(char *pStr, const char *pDelim, char **ppSavePtr)
{
    #if defined(DIRTYCODE_NX)
    char *pEnd;

    if (pStr == NULL)
    {
        pStr = *ppSavePtr;
    }

    if (*pStr == '\0')
    {
        *ppSavePtr = pStr;
        return(NULL);
    }

    // scan leading character
    pStr += strspn(pStr, pDelim);
    if (*pStr == '\0')
    {
        *ppSavePtr = pStr;
        return(NULL);
    }

    // find end of token
    pEnd = pStr + strcspn(pStr, pDelim);
    if (*pEnd == '\0')
    {
        *ppSavePtr = pEnd;
        return(pStr);
    }

    *pEnd = '\0';
    *ppSavePtr = pEnd + 1;
    return pStr;
    #elif defined(DIRTYCODE_PC) || defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
    return strtok_s(pStr, pDelim, ppSavePtr);
    #else
    return strtok_r(pStr, pDelim, ppSavePtr);
    #endif
}
/*F********************************************************************************/
/*!
    \Function ds_memcpy

    \Description
        Replacement for memcpy.

    \Input *pDst    - [out] output for new string
    \Input *pSrc    - pointer to input string to copy from
    \Input iDstLen  - size of substring pointed to by pSrc.
    
    \Version 07/15/2014 (amakoukji)
*/
/********************************************************************************F*/
void ds_memcpy(void *pDst, const void *pSrc, int32_t iDstLen)
{
    memcpy(pDst, pSrc, iDstLen);
}

/*F********************************************************************************/
/*!
    \Function ds_memcpy_s

    \Description
        Replacement for memcpy_s.

    \Input *pDst    - [out] output for new string
    \Input iDstLen  - size of output buffer
    \Input *pSrc    - pointer to input string to copy from
    \Input iSrcLen  - size of substring pointed to by pSrc.
    
    \Version 07/15/2014 (amakoukji)
*/
/********************************************************************************F*/
void ds_memcpy_s(void *pDst, int32_t iDstLen, const void *pSrc, int32_t iSrcLen)
{
    iDstLen = DS_MIN(iDstLen, iSrcLen);
    memcpy(pDst, pSrc, iDstLen);
}

/*F********************************************************************************/
/*!
    \Function ds_memclr

    \Description
        Memset that always clears the memory with zero

    \Input *pMem    - memory that is being cleared
    \Input iCount   - the number of bytes to clear
    
    \Version 01/06/2017 (eesponda)
*/
/********************************************************************************F*/
void ds_memclr(void *pMem, int32_t iCount)
{
    memset(pMem, 0, iCount);
}

/*F********************************************************************************/
/*!
    \Function ds_memset

    \Description
        Replacement for memset

    \Input *pMem    - memory that is being cleared
    \Input iValue   - value to clear the memory with
    \Input iCount   - the number of bytes to clear
    
    \Version 01/06/2017 (eesponda)
*/
/********************************************************************************F*/
void ds_memset(void *pMem, int32_t iValue, int32_t iCount)
{
    memset(pMem, iValue, iCount);
}
