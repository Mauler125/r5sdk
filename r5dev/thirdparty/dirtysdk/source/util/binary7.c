/*H*************************************************************************************/
/*!
    \File    binary7.c

    \Description
        This module provides routines to encode/decode binary7 data to/from a buffer.

    \Copyright
        Copyright (c) Electronic Arts 2009. ALL RIGHTS RESERVED.

    \Version 1.0 11/02/2009 (cadam) First version
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

// include platform types and functions
#include "DirtySDK/platform.h"

// self-include
#include "DirtySDK/util/binary7.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

/*** Private Functions *****************************************************************/

/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function    Binary7Encode

    \Description
        Encode binary data to fit in 7-bit high-ASCII characters.  All of the requested
        data will be encoded in a reversible form that is slightly larger than the
        original data.

    \Input *pDst    - destination buffer (will hold encoded data)
    \Input iDstLen  - destination buffer max size (in bytes)
    \Input *pSrc    - source buffer
    \Input iSrcLen  - source buffer length (in bytes)
    \Input bTerminate - TRUE to terminate the buffer, else FALSE.

    \Output
        int32_t     - < 0 indicates error; >= 0 indicates bytes in converted data

    \Notes
        If there is insufficient space to store all the encoded data, an error is
        returned immediately; no data is encoded or written.
        Storage requirements are (slen*8+6)/7 bytes.

    \Version 01/20/2004 (djones)
*/
/*************************************************************************************F*/
char *Binary7Encode(unsigned char *pDst, int32_t iDstLen, unsigned const char *pSrc, int32_t iSrcLen, uint32_t bTerminate)
{
    int32_t iBytesLeft = iSrcLen;
    uint32_t uBuffer = 0;           // buffer for holding leftover bits
    int32_t iBits = 0;              // number of bits currently in buffer

    // verify destination buffer is big enough to hold encoded data.
    if (iDstLen < ((((iSrcLen * 8) + 6) / 7) + (signed)bTerminate))
    {
        return(NULL);
    }

    // encode all bytes.
    while (iBytesLeft-- > 0)
    {
        uBuffer |= (*pSrc++) << iBits;          // merge new bits up above leftover bits
        iBits += 8;
        while (iBits >= 7)                      // have enough bits to write out an encoded byte
        {
            *pDst++ = 0x80 | (uBuffer & 0x7F);  // write encoded byte
            uBuffer >>= 7;                      // discard written bits
            iBits -= 7;
        }
    }

    // a few bits left over?
    if (iBits > 0)
    {
        *pDst++ = 0x80 | uBuffer;               // write leftover bits
    }

    // terminate?
    if (bTerminate)
    {
        *pDst = '\0';
    }

    // return pointer past end of dest buffer
    return((char *)pDst);
}

/*F*************************************************************************************/
/*!
    \Function    Binary7Decode

    \Description
        Decode binary data from 7-bit high-ASCII characters to 8-bit data.

    \Input *pDst    - destination buffer (will hold decoded data)
    \Input iDstLen  - destination buffer max size (in bytes)
    \Input *pSrc`   - source buffer

    \Output
        int32_t     - < 0 indicates error; >= 0 indicates bytes in converted data

    \Notes
        Decoding will stop at the first low-ASCII character, or until dlen bytes have
        been written, whichever comes first.  Storage requirements are slen*7/8 bytes.
        
    \Version 01/20/2004 (djones)
*/
/*************************************************************************************F*/
unsigned const char *Binary7Decode(unsigned char *pDst, int32_t iDstLen, unsigned const char *pSrc)
{
    uint32_t uBuffer = 0;            // buffer for holding leftover bits
    int32_t iBits = 0;               // number of bits currently in buffer

    // Begin decoding bytes.
    while ((*pSrc & 0x80) && (iDstLen > 0))
    {
        uBuffer |= (*pSrc++ & 0x7F) << iBits;   // merge new bits up above leftover bits
        iBits += 7;
        if (iBits >= 8)                         // have enough bits to write out a decoded byte
        {
            *pDst++ = uBuffer & 0xFF;           // write decoded byte
            uBuffer >>= 8;                      // discard written bits
            iBits -= 8;
            iDstLen -= 1;
        }
    }

    // Unlike encoding, where all source bits must be converted,
    // in decoding, extra bits are discarded, so no special
    // handling is required for leftover bits here.

    // return pointer past end of dest buffer
    return(pSrc);
}

