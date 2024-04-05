/*H*******************************************************************/
/*!
    \File base64.c

    \Description
        This module implements Base-64 encoding/decoding as defined in RFC
        4648: https://tools.ietf.org/html/rfc4648

    \Notes
        No strict compliance check has been made.  Instead it has only
        been confirmed that Decode(Encode(x)) = x for various inputs
        which was all that was required for the original application.

    \Copyright
        Copyright (c) Electronic Arts 2003-2017. ALL RIGHTS RESERVED.

    \Version 1.0 12/11/2003 (SJB) First Version
*/
/*******************************************************************H*/

/*** Include files ***************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/util/base64.h"

/*** Defines *********************************************************/

/*** Type Definitions ************************************************/

/*** Variables *******************************************************/

/*
   The table used in converting an unsigned char -> Base64.
   This is as per the RFC.
*/
static const char _Base64_strEncode[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

/* The table used in converting an unsigned char -> Base64 (URL)
   This is as per the RFC: https://tools.ietf.org/html/rfc4648#section-5 */
static const char _Base64_strEncodeUrl[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789-_";

/*
   The table used in coverting Base64 -> unsigned char.
   This is not strictly needed since this mapping can be determined
   by searching _Base64_Encode, but by having it as a separate table
   it means the decode can run in O(n) time where n is the length of
   the input rather than O(n*m) where m is 64.

   The table is designed such that after subtracting '+' from the
   base 64 value, the corresponding char value can be found by
   indexing into this table.  Since the characters chosen by the
   Base64 designers are not contiguous in ASCII there are some holes
   in the table and these are filled with -1 to indicate an invalid
   value.
*/
static const signed char _Base64_strDecode[] =
{
    62,                                                 // +
    -1, -1, -1,                                         // ,-.
    63,                                                 // /
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61,             // 0-9
    -1, -1, -1, -1, -1, -1, -1,                         // :;<=>?@
    0,   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, // A-
    13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, //  -Z
    -1, -1, -1, -1, -1, -1,                             // [\]^_`
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, // a-
    39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51  //  -z
};

static const signed char _Base64_strDecodeUrl[] =
{
    -1, -1,                                             // +,
    62,                                                 // -
    -1, -1,                                             // ./
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61,             // 0-9
    -1, -1, -1, -1, -1, -1, -1,                         // :;<=>?@
    0,   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, // A-
    13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, //  -Z
    -1, -1, -1, -1,                                     // [\]^
    63,                                                 // _
    -1,                                                 // `
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, // a-
    39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51  //  -z
};

/*** Private functions ***********************************************/

/*F*******************************************************************/
/*!
    \Function _Base64Encode

    \Description
        Base-64 encode a string.

    \Input iInputLen  - the length of the input
    \Input *pInput    - the input
    \Input *pOutput   - the output
    \Input iOutputLen - the output buffer size
    \Input *pEncode   - the table to use for encoding
    \Input bPadded    - do you want padding to be encoded?

    \Output
        int32_t      - encoded length, or -1 on error

    \Notes
        pOutput is NUL terminated.

        It is assumed that pOutput is large enough to hold
        Base64EncodedSize(iInputLen)+1 bytes.

        The following chart should help explain the various bit shifts
        that are in the code.  On the top are the 8-bit bytes of the
        input and on the bottom are the 6-bit bytes of the output :-

     \verbatim
          |       0       |       1       |       2       |  in
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |               |               |               |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |     0     |     1     |       2   |     3     |  out
     \endverbatim

        As the above shows the out0 is the top 6 bits of in0, out1 is
        the bottom two bits of in0 along with the top 4 bits of in1,
        ... etc.

        To speed things up the encoder tries to work on chunks of 3
        input bytes at a time since that produces 4 output bytes
        without having to maintain any inter-byte processing state.

        For any input that is not a multiple of 3 then 1 or 2 padding
        bytes are are added as appropriate as described in the RFC.

    \Version 12/11/2002 (sbevan) First Version
*/
/*******************************************************************F*/
static int32_t _Base64Encode(const char *pInput, int32_t iInputLen, char *pOutput, int32_t iOutputLen, const char *pEncode, uint8_t bPadded)
{
    int32_t iInp, iOut;

    // validate output buffer size, including null termination
    if ((Base64EncodedSize(iInputLen)+1) > iOutputLen)
    {
        return(-1);
    }

    // encode the data
    for (iInp=0, iOut=0; iInputLen >= 3; iOut += 4, iInp += 3, iInputLen -= 3)
    {
        pOutput[iOut+0] = pEncode[(pInput[iInp]>>2)&0x3f];
        pOutput[iOut+1] = pEncode[((pInput[iInp]&0x3)<<4)|((pInput[iInp+1]>>4)&0x0f)];
        pOutput[iOut+2] = pEncode[((pInput[iInp+1]&0xf)<<2)|((pInput[iInp+2]>>6)&0x03)];
        pOutput[iOut+3] = pEncode[(pInput[iInp+2]&0x3f)];
    }
    switch (iInputLen)
    {
    case 1:
        pOutput[iOut+0] = pEncode[(pInput[iInp]>>2)&0x3f];
        pOutput[iOut+1] = pEncode[((pInput[iInp]&0x3)<<4)];

        if (bPadded)
        {
            pOutput[iOut+2] = '=';
            pOutput[iOut+3] = '=';
            iOut += 4;
        }
        else
        {
            iOut += 2;
        }
        break;
    case 2:
        pOutput[iOut+0] = pEncode[(pInput[iInp]>>2)&0x3f];
        pOutput[iOut+1] = pEncode[((pInput[iInp]&0x3)<<4)|((pInput[iInp+1]>>4)&0x0f)];
        pOutput[iOut+2] = pEncode[((pInput[iInp+1]&0xf)<<2)];

        if (bPadded)
        {
            pOutput[iOut+3] = '=';
            iOut += 4;
        }
        else
        {
            iOut += 3;
        }
        break;
    }
    pOutput[iOut] = '\0';
    return(iOut);
}

/*F*******************************************************************/
/*!
    \Function _Base64Decode

    \Description
        Decode a Base-64 encoded string.

    \Input *pInput      - the Base-64 encoded string
    \Input iInputLen    - the length of the encoded input
    \Input *pOutput     - [out] the decoded string, or NULL to calculate output size
    \Input iOutputLen   - size of output buffer
    \Input *pDecode     - the table to use for the decoding

    \Output
        int32_t         - decoded size on success, zero on error.

    \Notes
        The only reasons for failure are if the input base64 sequence does
        not amount to a number of characters comprising an even multiple
        of four characters, or if the input contains a character not in
        the Base64 character set and not a whitespace character.  This
        behavior is mentioned in the RFC:

            "In base64 data, characters other than those in Table 1,
             line breaks, and other white space probably indicate a
             transmission error, about which a warning message or even
             a message rejection might be appropriate under some
             circumstances."

        If pOutput is NULL, the function will return the number of
        characters required to buffer the output, or zero on error.

        The following chart should help explain the various bit shifts
        that are in the code.  On the top are the 6-bit bytes in the
        input and and on the bottom are the 8-bit bytes of the output :-

        \verbatim
          |     0     |     1     |       2   |     3     |   in
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |               |               |               |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |       0       |       1       |       2       |   out
        \endverbatim

        As the above shows the out0 consists of all of in0 and the top
        2 bits of in1, out1 is the bottom 4 bits of in1 and the top 4
        bits of in2, ... etc.

    \Version 12/11/2002 (sbevan)   First Version
    \Version 01/29/2009 (jbrookes) Enhanced to skip whitespace, added decoded length functionality
*/
/*******************************************************************F*/
static int32_t _Base64Decode(const char *pInput, int32_t iInputLen, char *pOutput, int32_t iOutputLen, const signed char *pDecode)
{
    char ci[4];
    int8_t co0, co1, co2, co3;
    int32_t iInputCnt, iInputOff, iOutputOff;

    for (iInputOff = 0, iOutputOff = 0; iInputOff < iInputLen; )
    {
        // scan input
        for (iInputCnt = 0; (iInputCnt < 4) && (iInputOff < iInputLen) && (pInput[iInputOff] != '\0') && (iOutputOff < iOutputLen); iInputOff += 1)
        {
            // ignore whitespace
            if ((pInput[iInputOff] == ' ') || (pInput[iInputOff] == '\t') || (pInput[iInputOff] == '\r') || (pInput[iInputOff] == '\n'))
            {
                continue;
            }
            // basic range validation
            else if ((pInput[iInputOff] < '+') || (pInput[iInputOff] > 'z'))
            {
                return(0);
            }
            // fetch input character
            else
            {
                ci[iInputCnt++] = pInput[iInputOff];
            }
        }
        // if we didn't get anything, we're done
        if (iInputCnt == 0)
        {
            break;
        }
        if (iInputCnt < 4)
        {
            // error if we did not get at least 2
            if (iInputCnt < 2)
            {
                return(0);
            }
            // otherwise for everything under the quad, default to padding
            if (iInputCnt < 3)
            {
                ci[2] = '=';
            }
            ci[3] = '=';
        }
        // decode the sequence
        co0 = pDecode[(int32_t)ci[0]-'+'];
        co1 = pDecode[(int32_t)ci[1]-'+'];
        co2 = pDecode[(int32_t)ci[2]-'+'];
        co3 = pDecode[(int32_t)ci[3]-'+'];

        if ((co0 >= 0) && (co1 >= 0))
        {
            if ((co2 >= 0) && (co3 >= 0))
            {
                if ((pOutput != NULL) && ((iOutputLen - iOutputOff) > 2))
                {
                    pOutput[iOutputOff+0] = (co0<<2)|((co1>>4)&0x3);
                    pOutput[iOutputOff+1] = (co1&0x3f)<<4|((co2>>2)&0x3F);
                    pOutput[iOutputOff+2] = ((co2&0x3)<<6)|co3;
                }
                iOutputOff += 3;
            }
            else if ((co2 >= 0) && (ci[3] == '='))
            {
                if ((pOutput != NULL) && ((iOutputLen - iOutputOff) > 1))
                {
                    pOutput[iOutputOff+0] = (co0<<2)|((co1>>4)&0x3);
                    pOutput[iOutputOff+1] = (co1&0x3f)<<4|((co2>>2)&0x3F);
                }
                iOutputOff += 2;
                // consider input complete
                iInputOff = iInputLen;
            }
            else if ((ci[2] == '=') && (ci[3] == '='))
            {
                if ((pOutput != NULL) && ((iOutputLen - iOutputOff) > 0))
                {
                    pOutput[iOutputOff+0] = (co0<<2)|((co1>>4)&0x3);
                }
                iOutputOff += 1;
                // consider input complete
                iInputOff = iInputLen;
            }
            else
            {
                // illegal input character
                return(0);
            }
        }
        else
        {
            // illegal input character
            return(0);
        }
    }
    // return length of decoded data or zero if decoding failed
    return((iInputOff == iInputLen) ? iOutputOff : 0);
}

/*** Public functions ************************************************/

/*F*******************************************************************/
/*!
    \Function Base64Encode

    \Deprecated
        The calling convention for Base64Encode will be replaced with
        that of Base64Encode2 in a future release.

    \Description
        Base-64 encode a string.

    \Input iInputLen - the length of the input
    \Input *pInput   - the input
    \Input *pOutput  - the output

    \Notes
        pOutput is NUL terminated.

        It is assumed that pOutput is large enough to hold
        Base64EncodedSize(iInputLen)+1 bytes.

    \Version 1.0 12/11/2002 (SJB) First Version
*/
/*******************************************************************F*/
void Base64Encode(int32_t iInputLen, const char *pInput, char *pOutput)
{
    Base64Encode2(pInput, iInputLen, pOutput, 0x7fffffff);
}

/*F*******************************************************************/
/*!
    \Function Base64Encode2

    \Description
        Base-64 encode a string.

    \Input iInputLen  - the length of the input
    \Input *pInput    - the input
    \Input *pOutput   - the output
    \Input iOutputLen - the output buffer size

    \Output
        int32_t      - encoded length, or -1 on error

    \Notes
        pOutput is NUL terminated.

    \Version 12/11/2002 (sbevan) First Version
*/
/*******************************************************************F*/
int32_t Base64Encode2(const char *pInput, int32_t iInputLen, char *pOutput, int32_t iOutputLen)
{
    return(_Base64Encode(pInput, iInputLen, pOutput, iOutputLen, _Base64_strEncode, TRUE));
}

/*F*******************************************************************/
/*!
    \Function Base64Decode

    \Deprecated
        The calling convention for Base64Decode will be replaced with
        that of Base64Decode3 in a future release.

    \Description
        Decode a Base-64 encoded string.

    \Input iInputLen    - the length of the encoded input
    \Input *pInput      - the Base-64 encoded string
    \Input *pOutput     - [out] the decoded string, or NULL to calculate output size

    \Output
        int32_t         - true on success, false on error.

    \Notes
        It is assumed that pOutput is large enough to hold
        Base64DecodedSize(iInputLen) bytes.  If pOutput is NULL,
        the function will return the number of characters required to
        buffer the output or zero on error.

    \Version 12/11/2002 (sbevan)   First Version
    \Version 01/29/2009 (jbrookes) Enhanced to skip whitespace, added decoded length functionality
*/
/*******************************************************************F*/
int32_t Base64Decode(int32_t iInputLen, const char *pInput, char *pOutput)
{
    int32_t iOutputLen = Base64Decode2(iInputLen, pInput, pOutput);
    if (pOutput != NULL)
    {
        iOutputLen = iOutputLen ? TRUE : FALSE;
    }
    return(iOutputLen);
}

/*F*******************************************************************/
/*!
    \Function Base64Decode2

    \Deprecated
        The calling convention for Base64Decode2 will be replaced with
        that of Base64Decode3 in a future release.

    \Description
        Decode a Base-64 encoded string.

    \Input iInputLen    - the length of the encoded input
    \Input *pInput      - the Base-64 encoded string
    \Input *pOutput     - [out[ the decoded string, or NULL to calculate output size

    \Output
        int32_t         - decoded size on success, zero on error.

    \Notes
        It is assumed that pOutput is large enough to hold
        Base64DecodedSize(iInputLen) bytes.  If pOutput is NULL,
        the function will return the number of characters required to
        buffer the output or zero on error.

    \Version 12/11/2002 (sbevan)   First Version
    \Version 01/29/2009 (jbrookes) Enhanced to skip whitespace, added decoded length functionality
*/
/*******************************************************************F*/
int32_t Base64Decode2(int32_t iInputLen, const char *pInput, char *pOutput)
{
    return(Base64Decode3(pInput, iInputLen, pOutput, 0x7fffffff));
}

/*F*******************************************************************/
/*!
    \Function Base64Decode3

    \Description
        Decode a Base-64 encoded string.

    \Input *pInput      - the Base-64 encoded string
    \Input iInputLen    - the length of the encoded input
    \Input *pOutput     - [out] the decoded string, or NULL to calculate output size
    \Input iOutputLen   - size of output buffer

    \Output
        int32_t         - decoded size on success, zero on error.

    \Notes
        If pOutput is NULL, the function will return the number of
        characters required to buffer the output, or zero on error.

    \Version 12/11/2002 (sbevan)   First Version
    \Version 01/29/2009 (jbrookes) Enhanced to skip whitespace, added decoded length functionality
*/
/*******************************************************************F*/
int32_t Base64Decode3(const char *pInput, int32_t iInputLen, char *pOutput, int32_t iOutputLen)
{
    return(_Base64Decode(pInput, iInputLen, pOutput, iOutputLen, _Base64_strDecode));
}

/*F*******************************************************************/
/*!
    \Function Base64EncodeUrl

    \Description
        Base-64 encode a string url.

    \Input *pInput    - the input
    \Input iInputLen  - the length of the input
    \Input *pOutput   - the output
    \Input iOutputLen - the output buffer size

    \Output
        int32_t      - encoded length, or -1 on error

    \Notes
        pOutput is NUL terminated.

    \Version 11/15/2017 (eesponda)
*/
/*******************************************************************F*/
int32_t Base64EncodeUrl(const char *pInput, int32_t iInputLen, char *pOutput, int32_t iOutputLen)
{
    return(Base64EncodeUrl2(pInput, iInputLen, pOutput, iOutputLen, TRUE));
}

/*F*******************************************************************/
/*!
    \Function Base64EncodeUrl2

    \Description
        Base-64 encode a string url with extra options

    \Input *pInput    - the input
    \Input iInputLen  - the length of the input
    \Input *pOutput   - the output
    \Input iOutputLen - the output buffer size
    \Input bPadded    - is the output padded?

    \Output
        int32_t      - encoded length, or -1 on error

    \Notes
        pOutput is NUL terminated.

    \Version 05/20/2020 (eesponda)
*/
/*******************************************************************F*/
int32_t Base64EncodeUrl2(const char *pInput, int32_t iInputLen, char *pOutput, int32_t iOutputLen, uint8_t bPadded)
{
    return(_Base64Encode(pInput, iInputLen, pOutput, iOutputLen, _Base64_strEncodeUrl, bPadded));
}

/*F*******************************************************************/
/*!
    \Function Base64DecodeUrl

    \Description
        Decode a Base-64 encoded url string.

    \Input *pInput      - the Base-64 encoded string
    \Input iInputLen    - the length of the encoded input
    \Input *pOutput     - [out] the decoded string, or NULL to calculate output size
    \Input iOutputLen   - size of output buffer

    \Output
        int32_t         - decoded size on success, zero on error.

    \Notes
        If pOutput is NULL, the function will return the number of
        characters required to buffer the output, or zero on error.

    \Version 11/15/2017 (eesponda)
*/
/*******************************************************************F*/
int32_t Base64DecodeUrl(const char *pInput, int32_t iInputLen, char *pOutput, int32_t iOutputLen)
{
    return(_Base64Decode(pInput, iInputLen, pOutput, iOutputLen, _Base64_strDecodeUrl));
}

