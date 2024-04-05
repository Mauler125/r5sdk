/*H*************************************************************************************************/
/*!
    \File    dirtynet.c

    \Description
        Platform-independent network related routines.

    \Copyright
        Copyright (c) Electronic Arts 2002-2018

    \Version 1.0 01/02/2002 (gschaefer) First Version
    \Version 1.1 01/27/2003 (jbrookes)  Split from dirtynetwin.c
*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"

#include "dirtynetpriv.h"

/*** Defines ***************************************************************************/

//! 30s hostname cache timeout
#define DIRTYNET_HOSTNAMECACHELIFETIME  (30*1000)

//! verbose logging of dirtynet packet queue operations
#define DIRTYNET_PACKETQUEUEDEBUG       (DIRTYCODE_LOGGING && FALSE)

//! verbose logging of dirtynet rate estimation
#define DIRTYNET_RATEDEBUG              (DIRTYCODE_LOGGING && FALSE)

//! maximum allowable packet queue size
#define DIRTYNET_PACKETQUEUEMAX         (1024)

//! minimum throttle rate supported
#define DIRTYNET_MIN_THROTTLE_RATE      (1460)

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! socket hostname cache entry
typedef struct SocketHostnameCacheEntryT
{
    char strDnsName[256];
    uint32_t uAddress;
    uint32_t uTimer;
} SocketHostnameCacheEntryT;

//! socket hostname cache
struct SocketHostnameCacheT
{
    int32_t iMaxEntries;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    NetCritT Crit;
    SocketHostnameCacheEntryT CacheEntries[1];  //!< variable-length cache entry list
};

//! socket packet queue
struct SocketPacketQueueT
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    int16_t iNumPackets;        //!< number of packets in the queue
    int16_t iMaxPackets;        //!< max queue size
    int16_t iPacketHead;        //!< current packet queue head
    int16_t iPacketTail;        //!< current packet queue tail

    uint32_t uLatency;          //!< simulated packet latency target, in milliseconds
    uint32_t uDeviation;        //!< simulated packet deviation target, in milliseconds
    uint32_t uPacketLoss;       //!< packet loss percentage, 16.16 fractional integer

    uint32_t uPacketDrop;       //!< number of packets overwritten due to queue overflow
    uint32_t uPacketMax;        //!< maximum number of packets in the queue (high water mark)

    uint32_t uLatencyTime;      //!< current amount of latency in the packet queue
    int32_t iDeviationTime;     //!< current deviation

    SocketPacketQueueEntryT aPacketQueue[1]; //!< variable-length queue entry list
};

#ifndef DIRTYCODE_NX
//! socket address map entry
struct SocketAddrMapEntryT
{
    int32_t iRefCount;
    int32_t iVirtualAddress;
    struct sockaddr_in6 SockAddr6;
};
#endif

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables


// Public variables


/*** Private Functions *****************************************************************/

#ifndef DIRTYCODE_NX
/*F********************************************************************************/
/*!
    \Function _SockaddrIn6Identify

    \Description
        Identify IPv6 sockaddr based on specified matching sequence (must come at
        the start of address bytes).

    \Input *pAddr6              - address to identify
    \Input *pCheckVal           - bytes to check
    \Input iValLen              - length of byte sequence to check

    \Output
        uint8_t                 - TRUE if the address matches, else FALSE

    \Version 04/12/2016 (jbrookes)
*/
/********************************************************************************F*/
static uint8_t _SockaddrIn6Identify(const struct sockaddr_in6 *pAddr6, const uint8_t *pCheckVal, int32_t iValLen)
{
    uint8_t bResult = !memcmp(pCheckVal, pAddr6->sin6_addr.s6_addr, iValLen) ? TRUE : FALSE;
    return(bResult);
}

/*F********************************************************************************/
/*!
    \Function _SockaddrIn6IsIPv4

    \Description
        Identify if specified sockaddr_in6 is an IPv4-mapped IPv6 address

    \Input *pAddr6              - address to identify

    \Output
        uint8_t                 - TRUE if the address is IPv4-mapped, else FALSE

    \Version 04/12/2016 (jbrookes)
*/
/********************************************************************************F*/
static uint8_t _SockaddrIn6IsIPv4(const struct sockaddr_in6 *pAddr6)
{
    const uint8_t aIpv4Prefix[]  = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff };
    return(_SockaddrIn6Identify(pAddr6, aIpv4Prefix, sizeof(aIpv4Prefix)));
}

/*F********************************************************************************/
/*!
    \Function _SockaddrIn6IsNAT64

    \Description
        Identify if specified sockaddr_in6 is a NAT64 IPv6 address

    \Input *pAddr6              - address to identify

    \Output
        uint8_t                 - TRUE if the address is NAT64, else FALSE

    \Version 04/12/2016 (jbrookes)
*/
/********************************************************************************F*/
static uint8_t _SockaddrIn6IsNAT64(const struct sockaddr_in6 *pAddr6)
{
    const uint8_t aNat64Prefix[] = { 0x00, 0x64, 0xff, 0x9b };
    return(_SockaddrIn6Identify(pAddr6, aNat64Prefix, sizeof(aNat64Prefix)));
}

/*F********************************************************************************/
/*!
    \Function _SockaddrIn6IsZero

    \Description
        Identify if specified sockaddr_in6 is zero (unspecified)

    \Input *pAddr6              - address to identify

    \Output
        uint8_t                 - TRUE if the address is zero, else FALSE

    \Version 04/22/2016 (jbrookes)
*/
/********************************************************************************F*/
static uint8_t _SockaddrIn6IsZero(const struct sockaddr_in6 *pAddr6)
{
    const uint8_t aIpv6Zero[]  = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    return(_SockaddrIn6Identify(pAddr6, aIpv6Zero, sizeof(aIpv6Zero)));
}

/*F*************************************************************************************************/
/*!
\Function _SockaddrIn6SetAddrText

\Description
Set Internet address component of sockaddr_in6 struct from textural address.

\Input *pAddr6      -  pointer to ipv6 address
\Input *pStr        - pointer to source ipv6 address

\Output
int32_t         - zero=no error, negative=error

\Notes
See _SockaddrIn6GetAddrText for format considerations and references

\Version 08/28/2017 (jbrookes)
*/
/*************************************************************************************************F*/
static int32_t _SockaddrIn6SetAddrText(struct sockaddr_in6 *pAddr6, const char *pStr)
{
    uint16_t aAddrWords[8], *pWords, uWordVal;
    int32_t iWordIdx, iWordCt, iSkipIdx, iWriteIdx;
    const char *pEnd, *pDot;
    uint32_t uAddr32 = 0;

    // see if we have dot notation (e.g. nat64, ipv4-mapped)
    if ((pDot = strchr(pStr, '.')) != NULL)
    {
        // find start of dot notation number
        for (pDot -= 1; isalnum(*pDot) && (pDot > pStr); pDot -= 1)
            ;
        // get 32 bit address encoded in dot notation
        uAddr32 = SocketInTextGetAddr(pDot + 1);
    }

    // init word array
    ds_memclr(aAddrWords, sizeof(aAddrWords));

    // convert address words, remember if we had a skip
    for (iWordIdx = 0, iSkipIdx = -1; (*pStr != '\0') && (iWordIdx < 8); pStr += 1)
    {
        uWordVal = SocketHtons(strtol(pStr, (char **)&pEnd, 16));
        pStr = pEnd;

        if ((*pStr == ':') || (*pStr == '\0'))
        {
            aAddrWords[iWordIdx++] = uWordVal;
        }
        if ((*pStr == ':') && (pStr[1] == ':'))
        {
            iSkipIdx = iWordIdx;
            pStr += 1;
        }
        if ((pStr == pDot) && (uAddr32 != 0))
        {
            aAddrWords[iWordIdx++] = SocketHtons((uint16_t)(uAddr32 >> 16));
            aAddrWords[iWordIdx++] = SocketHtons((uint16_t)(uAddr32 >> 0));
            break;
        }

        if (*pStr == '\0')
        {
            break;
        }
    }

    // copy to sockaddr, with skip
    for (iWordCt = iWordIdx, iWordIdx = iWriteIdx = 0, pWords = (uint16_t *)pAddr6->sin6_addr.s6_addr; iWriteIdx < 8; )
    {
        if (iWriteIdx == iSkipIdx)
        {
            for (iWordCt = 8 - iWordCt; iWordCt > 0; iWordCt -= 1)
            {
                pWords[iWriteIdx++] = 0;
            }
        }
        else
        {
            pWords[iWriteIdx++] = aAddrWords[iWordIdx++];
        }
    }

    return(0);
}
/*F*************************************************************************************************/
/*!
    \Function _SockaddrIn6GetAddrText

    \Description
        Return Internet address component of sockaddr_in6 struct in textual form (see below).

    \Input *pAddr6  - pointer to ipv6 address
    \Input *pStr    - pointer to storage for text address
    \Input iLen     - length of buffer

    \Output
        char *      - returns pStr on success, NULL on failure

    \Notes
        IPv6 textual format guidelines are outlined in https://tools.ietf.org/html/rfc4291#section-2.2
        and later refined in https://tools.ietf.org/html/rfc5952, which narrows the range of
        supported options to standardize are more rigid specification.  This implementation
        follows the more limited set of specifications outlined in rfc5952.  The short version is:

        - Preferred form is x:x:x:x:x:x:x:x, where the 'x's are one of four hexadecimal digits of
        the eight 16-bit pieces of the address
        - Leading zeros within a field MUST be suppressed
        - The use of :: indicates one or more groups of 16 bits of zeros; it can appear only once
        in an address
        - The :: symbole MUST be used to its maximum capability.  It MUST NOT be used to shorten
        just one 16-bit field
        - If there are two or more sets of 16-bit zeros of equal length, the left-most MUST be
        shortened
        - Alphabetic characters in the address MUST be represented in lowercase
        - It is RECOMMENDED to use mixed notation if the address can be distinguished as having
        an IPv4 address embedded in the lower 32 bits solely from the address field through
        the use of a well-known prefix.

    \Version 08/28/2017 (jbrookes)
*/
/*************************************************************************************************F*/
static char *_SockaddrIn6GetAddrText(const struct sockaddr_in6 *pAddr6, char *pStr, int32_t iLen)
{
    uint16_t aAddrWords[8], *pWords;
    int32_t iWord, iOffset;
    int32_t iZeroStart, iZeroCount;
    int32_t iZeroStartTmp, iZeroCountTmp;
    char strAddr32[16] = "";

    // convert to words in host format
    for (iWord = 0, pWords = (uint16_t *)pAddr6->sin6_addr.s6_addr; iWord < 8; iWord += 1)
    {
        aAddrWords[iWord] = SocketNtohs(pWords[iWord]);
    }

    // if this is a mixed-notation address convert the final 32bits to a dot-notation address string
    if (_SockaddrIn6IsIPv4(pAddr6) || _SockaddrIn6IsNAT64(pAddr6))
    {
        uint32_t uAddr32 = (uint32_t)aAddrWords[6] << 16 | (uint32_t)aAddrWords[7];
        SocketInAddrGetText(uAddr32, strAddr32, sizeof(strAddr32));
    }

    // find longest stretch of two or more zeros (if there is one)
    for (iWord = iZeroStart = iZeroCount = iZeroCountTmp = iZeroStartTmp = 0; iWord < 8; iWord += 1)
    {
        if (aAddrWords[iWord] == 0)
        {
            if (iZeroCountTmp == 0)
            {
                iZeroStartTmp = iWord;
            }
            iZeroCountTmp += 1;
        }
        else
        {
            iZeroCountTmp = 0;
        }

        if (iZeroCountTmp > iZeroCount)
        {
            iZeroStart = iZeroStartTmp;
            iZeroCount = iZeroCountTmp;
        }
    }

    // format address string
    for (iWord = 0, iOffset = 0; iWord < 8; iWord += 1)
    {
        if ((iWord != iZeroStart) || (iZeroCount < 2))
        {
            iOffset += ds_snzprintf(pStr + iOffset, iLen - iOffset, "%x", aAddrWords[iWord]);
            if (iWord < 7)
            {
                iOffset += ds_snzprintf(pStr + iOffset, iLen - iOffset, ":");
            }
        }
        else
        {
            iOffset += ds_snzprintf(pStr + iOffset, iLen - iOffset, (iWord == 0) ? "::" : ":");
            iWord += iZeroCount - 1;
        }

        // output dot portion of mixed-notation address, if present
        if ((strAddr32[0] != '\0') && (iWord == 5))
        {
            iOffset += ds_snzprintf(pStr + iOffset, iLen - iOffset, "%s", strAddr32);
            iWord += 2;
        }
    }

    // return to caller
    return(pStr);
}

#endif
/*F*************************************************************************************************/
/*!
    \Function _SockaddrIn4SetAddrText

    \Description
        Set Internet address component of sockaddr struct from textual address (a.b.c.d).

    \Input *pAddr   - sockaddr structure
    \Input *pStr    - textual address

    \Output
        int32_t     - zero=no error, negative=error

    \Version 10/04/1999 (gschaefer)
*/
/*************************************************************************************************F*/
static int32_t _SockaddrIn4SetAddrText(struct sockaddr *pAddr, const char *pStr)
{
    uint8_t *pIpAddr = (uint8_t *)(pAddr->sa_data+2);
    int32_t iOctet;

    for (iOctet = 0; iOctet < 4; iOctet += 1, pStr += 1)
    {
        pIpAddr[iOctet] = '\0';
        while ((*pStr >= '0') && (*pStr <= '9'))
        {
            pIpAddr[iOctet] = (pIpAddr[iOctet]*10) + (*pStr++ & 15);
        }
        if ((iOctet < 3) && (*pStr != '.'))
        {
            pIpAddr[0] = pIpAddr[1] = pIpAddr[2] = pIpAddr[3] = '\0';
            return(-1);
        }
    }

    return(0);
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function SockaddrCompare

    \Description
        Compare two sockaddr structs and see if address is the same. This is different
        from simple binary compare because only relevent fields are checked.

    \Input *pAddr1  - address #1
    \Input *pAddr2  - address to compare with address #1

    \Output
        int32_t         - zero=no error, negative=error

    \Version 08/30/1999 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t SockaddrCompare(const struct sockaddr *pAddr1, const struct sockaddr *pAddr2)
{
    int32_t len = sizeof(*pAddr1)-sizeof(pAddr1->sa_family);

    // make sure address family matches
    if (pAddr1->sa_family != pAddr2->sa_family)
    {
        return(pAddr1->sa_family- pAddr2->sa_family);
    }

    // do type specific comparison
    if (pAddr1->sa_family == AF_INET)
    {
        // compare port and address
        len = 2 + 4;
    }
    else if (pAddr1->sa_family == AF_INET6)
    {
        // compare port, flow info, and address
        len = 2 + 4 + 8;
    }

    // binary compare of address data
    return(memcmp(pAddr1->sa_data, pAddr2->sa_data, len));
}

/*F*************************************************************************************************/
/*!
    \Function SockaddrInSetAddrText

    \Description
        Set Internet address component of sockaddr struct from textual address.
        Important: Only works with AF_INET addresses for nx

    \Input *pAddr   - sockaddr structure
    \Input *pStr    - textual address

    \Output
        int32_t     - zero=no error, negative=error

    \Version 10/04/1999 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t SockaddrInSetAddrText(struct sockaddr *pAddr, const char *pStr)
{
    int32_t iResult = -1;
    if (pAddr->sa_family == AF_INET)
    {
        iResult = _SockaddrIn4SetAddrText(pAddr, pStr);
    }
    #ifndef DIRTYCODE_NX
    else if (pAddr->sa_family == AF_INET6)
    {
        iResult = _SockaddrIn6SetAddrText((struct sockaddr_in6 *)pAddr, pStr);
    }
    #endif
    return(iResult);
}

/*F*************************************************************************************************/
/*!
    \Function SockaddrInGetAddrText

    \Description
        Convert a sockaddr into textual form based on address family

    \Input *pAddr   - sockaddr struct
    \Input *pStr    - address buffer
    \Input iLen     - address length

    \Output
        char *      - returns str on success, NULL on failure

    \Version 10/04/1999 (gschaefer)
*/
/*************************************************************************************************F*/
char *SockaddrInGetAddrText(const struct sockaddr *pAddr, char *pStr, int32_t iLen)
{
    char *pResult = NULL;
    if (pAddr->sa_family == AF_INET)
    {
        pResult = SocketInAddrGetText(SockaddrInGetAddr(pAddr), pStr, iLen);
    }
    #ifndef DIRTYCODE_NX
    else if (pAddr->sa_family == AF_INET6)
    {
        pResult = _SockaddrIn6GetAddrText((const struct sockaddr_in6 *)pAddr, pStr, iLen);
    }
    #endif
    return(pResult);
}

/*F*************************************************************************************************/
/*!
    \Function    SockaddrInParse

    \Description
        Convert textual internet address:port into sockaddr structure

    \Input *pAddr   - sockaddr to fill in
    \Input *pParse  - textual address

    \Output
        int32_t         - flags:
            0=parsed nothing
            1=parsed addr
            2=parsed port
            3=parsed addr+port

    \Version 11/23/2002 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t SockaddrInParse(struct sockaddr *pAddr, const char *pParse)
{
    int32_t iReturn = 0, iPort = 0;
    uint32_t uAddr = 0;

    // init the address
    SockaddrInit(pAddr, AF_INET);

    // parse addr:port
    iReturn = SockaddrInParse2(&uAddr, &iPort, NULL, pParse);

    // set addr:port in sockaddr
    SockaddrInSetAddr(pAddr, uAddr);
    SockaddrInSetPort(pAddr, iPort);

    // return parse info
    return(iReturn);
}

/*F*************************************************************************************************/
/*!
    \Function    SockaddrInParse2

    \Description
        Convert textual internet address:port into sockaddr structure

        If the textual internet address:port is followed by a second :port, the second port
        is optionally parsed into pPort2, if not NULL.

    \Input *pAddr   - address to fill in
    \Input *pPort   - port to fill in
    \Input *pPort2  - second port to fill in
    \Input *pParse  - textual address

    \Output
        int32_t         - flags:
            0=parsed nothing
            1=parsed addr
            2=parsed port
            3=parsed addr+port
            4=parsed port2

    \Version 11/23/02 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t SockaddrInParse2(uint32_t *pAddr, int32_t *pPort, int32_t *pPort2, const char *pParse)
{
    int32_t iReturn = 0;
    uint32_t uVal;

    // skip embedded white-space
    while ((*pParse > 0) && (*pParse <= ' '))
    {
        ++pParse;
    }

    // parse the address (no dns for listen)
    for (uVal = 0; ((*pParse >= '0') && (*pParse <= '9')) || (*pParse == '.'); ++pParse)
    {
        // either add or shift
        if (*pParse != '.')
        {
            uVal = (uVal - (uVal & 255)) + ((uVal & 255) * 10) + (*pParse & 15);
        }
        else
        {
            uVal <<= 8;
        }
    }
    if ((*pAddr = uVal) != 0)
    {
        iReturn |= 1;
    }

    // skip non-port info
    while ((*pParse != ':') && (*pParse != 0))
    {
        ++pParse;
    }

    // parse the port
    uVal = 0;
    if (*pParse == ':')
    {
        for (++pParse; (*pParse >= '0') && (*pParse <= '9'); ++pParse)
        {
            uVal = (uVal * 10) + (*pParse & 15);
        }
        iReturn |= 2;
    }
    *pPort = (int32_t)uVal;

    // parse port2 (optional)
    if (pPort2 != NULL)
    {
        uVal = 0;
        if (*pParse == ':')
        {
            for (++pParse; (*pParse >= '0') && (*pParse <= '9'); ++pParse)
            {
                uVal = (uVal * 10) + (*pParse & 15);
            }
            iReturn |= 4;
        }
        *pPort2 = (int32_t)uVal;
    }

    // return the address
    return(iReturn);
}

/*F*************************************************************************************************/
/*!
    \Function SocketInAddrGetText

    \Description
        Convert 32-bit internet address into textual form.

    \Input uAddr    - address
    \Input *pStr    - [out] address buffer
    \Input iLen     - address length

    \Output
        char *      - returns str on success, NULL on failure

    \Version 06/17/2009 (jbrookes)
*/
/*************************************************************************************************F*/
char *SocketInAddrGetText(uint32_t uAddr, char *pStr, int32_t iLen)
{
    uint8_t uAddrByte[4];
    int32_t iIndex;
    char *pStrStart = pStr;

    uAddrByte[0] = (uint8_t)(uAddr>>24);
    uAddrByte[1] = (uint8_t)(uAddr>>16);
    uAddrByte[2] = (uint8_t)(uAddr>>8);
    uAddrByte[3] = (uint8_t)(uAddr>>0);

    for (iIndex = 0; iIndex < 4; iIndex += 1)
    {
        uint32_t uNumber = uAddrByte[iIndex];
        if (uNumber > 99)
        {
            *pStr++ = (char)('0' + (uNumber / 100));
            uNumber %= 100;
            *pStr++ = (char)('0' + (uNumber / 10));
            uNumber %= 10;
        }
        if (uNumber > 9)
        {
            *pStr++ = (char)('0' + (uNumber / 10));
            uNumber %= 10;
        }
        *pStr++ = (char)('0' + uNumber);
        if (iIndex < 3)
        {
            *pStr++ = '.';
        }
    }
    *pStr = '\0';
    return(pStrStart);
}

/*F*************************************************************************************************/
/*!
    \Function    SocketInTextGetAddr

    \Description
        Convert textual internet address into 32-bit integer form

    \Input *pAddrText   - textual address

    \Output
        int32_t         - integer form

    \Version 11/23/02 (JLB) First Version

*/
/*************************************************************************************************F*/
int32_t SocketInTextGetAddr(const char *pAddrText)
{
    struct sockaddr SockAddr;
    int32_t iAddr = 0;

    SockaddrInit(&SockAddr, AF_INET);
    if (SockaddrInSetAddrText(&SockAddr, pAddrText) == 0)
    {
        iAddr = SockaddrInGetAddr(&SockAddr);
    }
    return(iAddr);
}

/*F*************************************************************************************************/
/*!
    \Function    SocketHtons

    \Description
        Convert uint16_t from host to network byte order

    \Input uAddr    - value to convert

    \Output
        uint16_t    - converted value

    \Version 10/04/1999 (gschaefer)
*/
/*************************************************************************************************F*/
uint16_t SocketHtons(uint16_t uAddr)
{
    uint8_t uNetw[2];
    ds_memcpy_s(uNetw, sizeof(uNetw), &uAddr, sizeof(uAddr));
    return((uNetw[0]<<8)|(uNetw[1]<<0));
}

/*F*************************************************************************************************/
/*!
    \Function    SocketHtonl

    \Description
        Convert uint32_t from host to network byte order.

    \Input uAddr    - value to convert

    \Output
        uint32_t    - converted value

    \Version 10/04/1999 (gschaefer)
*/
/*************************************************************************************************F*/
uint32_t SocketHtonl(uint32_t uAddr)
{
    uint8_t uNetw[4];
    ds_memcpy_s(uNetw, sizeof(uNetw), &uAddr, sizeof(uAddr));
    return((((((uNetw[0]<<8)|uNetw[1])<<8)|uNetw[2])<<8)|uNetw[3]);
}

/*F*************************************************************************************************/
/*!
    \Function    SocketNtohs

    \Description
        Convert uint16_t from network to host byte order.

    \Input uAddr  - value to convert

    \Output
        uint16_t  - converted value

    \Version 10/0/99 (GWS) First Version

*/
/*************************************************************************************************F*/
uint16_t SocketNtohs(uint16_t uAddr)
{
    uint8_t uNetw[2];
    uNetw[1] = (uint8_t)uAddr;
    uAddr >>= 8;
    uNetw[0] = (uint8_t)uAddr;
    ds_memcpy_s(&uAddr, sizeof(uAddr), uNetw, sizeof(uNetw));
    return(uAddr);
}

/*F*************************************************************************************************/
/*!
    \Function    SocketNtohl

    \Description
        Convert uint32_t from network to host byte order.

    \Input uAddr     - value to convert

    \Output
        uint32_t    - converted value

    \Version 10/04/1999 (gschaefer)
*/
/*************************************************************************************************F*/
uint32_t SocketNtohl(uint32_t uAddr)
{
    uint8_t uNetw[4];
    uNetw[3] = (uint8_t)uAddr;
    uAddr >>= 8;
    uNetw[2] = (uint8_t)uAddr;
    uAddr >>= 8;
    uNetw[1] = (uint8_t)uAddr;
    uAddr >>= 8;
    uNetw[0] = (uint8_t)uAddr;
    ds_memcpy_s(&uAddr, sizeof(uAddr), uNetw, sizeof(uNetw));
    return(uAddr);
}

/*
    HostName Cache functions
*/

/*F********************************************************************************/
/*!
    \Function SocketHostnameCacheCreate

    \Description
        Create short-term hostname (DNS) cache

    \Input iMemGroup            - memgroup to alloc/free with
    \Input *pMemGroupUserData   - memgroup user data to alloc/free with

    \Output
        SocketHostnameCacheT *  - hostname cache or NULL on failure

    \Version 10/09/2013 (jbrookes)
*/
/********************************************************************************F*/
SocketHostnameCacheT *SocketHostnameCacheCreate(int32_t iMemGroup, void *pMemGroupUserData)
{
    const int32_t iMaxEntries = 16;
    int32_t iCacheSize = sizeof(SocketHostnameCacheT) + (iMaxEntries * sizeof(SocketHostnameCacheEntryT));
    SocketHostnameCacheT *pCache;

    // alloc and init cache
    if ((pCache = DirtyMemAlloc(iCacheSize, SOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynet: could not alloc %d bytes for hostname cache\n", iCacheSize));
        return(NULL);
    }
    ds_memclr(pCache, iCacheSize);

    // set base info
    pCache->iMaxEntries = iMaxEntries;
    pCache->iMemGroup = iMemGroup;
    pCache->pMemGroupUserData = pMemGroupUserData;

    // initialize crit
    NetCritInit2(&pCache->Crit, "HostnameCache", NETCRIT_OPTION_SINGLETHREADENABLE);

    // return to caller
    return(pCache);
}

/*F********************************************************************************/
/*!
    \Function SocketHostnameCacheDestroy

    \Description
        Destroy short-term hostname (DNS) cache

    \Input *pCache      - hostname cache

    \Version 10/09/2013 (jbrookes)
*/
/********************************************************************************F*/
void SocketHostnameCacheDestroy(SocketHostnameCacheT *pCache)
{
    NetCritKill(&pCache->Crit);
    DirtyMemFree(pCache, SOCKET_MEMID, pCache->iMemGroup, pCache->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function SocketHostnameCacheAdd

    \Description
        Add hostname and address to hostname cache

    \Input *pCache      - hostname cache
    \Input *pStrHost    - hostname to add
    \Input uAddress     - address of hostname
    \Input iVerbose     - debug level

    \Version 10/09/2013 (jbrookes)
*/
/********************************************************************************F*/
void SocketHostnameCacheAdd(SocketHostnameCacheT *pCache, const char *pStrHost, uint32_t uAddress, int32_t iVerbose)
{
    SocketHostnameCacheEntryT *pCacheEntry;
    int32_t iCacheIdx;

    // if we're already in the cache, bail
    if (SocketHostnameCacheGet(pCache, pStrHost, 0) != 0)
    {
        return;
    }

    // scan cache for an open entry
    NetCritEnter(&pCache->Crit);
    for (iCacheIdx = 0; iCacheIdx < pCache->iMaxEntries; iCacheIdx += 1)
    {
        pCacheEntry = &pCache->CacheEntries[iCacheIdx];
        if (pCacheEntry->uAddress == 0)
        {
            NetPrintfVerbose((iVerbose, 1, "dirtynet: adding hostname cache entry %s/%a\n", pStrHost, uAddress));
            ds_strnzcpy(pCacheEntry->strDnsName, pStrHost, sizeof(pCacheEntry->strDnsName));
            pCacheEntry->uAddress = uAddress;
            pCacheEntry->uTimer = NetTick();
            break;
        }
    }
    NetCritLeave(&pCache->Crit);
}

/*F********************************************************************************/
/*!
    \Function SocketHostnameCacheGet

    \Description
        Get address for hostname from cache, if available.

    \Input *pCache      - hostname cache
    \Input *pStrHost    - hostname to add
    \Input iVerbose     - debug level

    \Output
        uint32_t        - address for hostname, or zero if not in cache

    \Version 10/09/2013 (jbrookes)
*/
/********************************************************************************F*/
uint32_t SocketHostnameCacheGet(SocketHostnameCacheT *pCache, const char *pStrHost, int32_t iVerbose)
{
    SocketHostnameCacheEntryT *pCacheEntry;
    uint32_t uAddress;
    int32_t iCacheIdx;

    // scan cache for dns entry
    NetCritEnter(&pCache->Crit);
    for (iCacheIdx = 0, uAddress = 0; iCacheIdx < pCache->iMaxEntries; iCacheIdx += 1)
    {
        pCacheEntry = &pCache->CacheEntries[iCacheIdx];
        // skip empty entries
        if (pCacheEntry->strDnsName[0] == '\0')
        {
            continue;
        }
        // check for entry we want
        if (!strcmp(pCacheEntry->strDnsName, pStrHost))
        {
            NetPrintfVerbose((iVerbose, 0, "dirtynet: %s=%a [cache]\n", pCacheEntry->strDnsName, pCacheEntry->uAddress));
            uAddress = pCache->CacheEntries[iCacheIdx].uAddress;
            break;
        }
    }
    NetCritLeave(&pCache->Crit);
    return(uAddress);
}

/*F********************************************************************************/
/*!
    \Function SocketHostnameCacheDel

    \Description
        Remove hostname cache entry from the cache, if it exists.  pStrHost is
        checked if non-NULL and uAddress is checked if non-zero (note that both
        can be checked if desired).

    \Input *pCache      - hostname cache
    \Input *pStrHost    - hostname of cache entry to delete, or NULL
    \Input uAddress     - address of cache entry to delete, or zero
    \Input iVerbose     - debug level

    \Version 09/23/2016 (jbrookes)
*/
/********************************************************************************F*/
void SocketHostnameCacheDel(SocketHostnameCacheT *pCache, const char *pStrHost, uint32_t uAddress, int32_t iVerbose)
{
    SocketHostnameCacheEntryT *pCacheEntry;
    int32_t iCacheIdx;

    // scan cache for dns entry
    NetCritEnter(&pCache->Crit);
    for (iCacheIdx = 0; iCacheIdx < pCache->iMaxEntries; iCacheIdx += 1)
    {
        pCacheEntry = &pCache->CacheEntries[iCacheIdx];
        // skip empty entries
        if (pCacheEntry->strDnsName[0] == '\0')
        {
            continue;
        }
        // check for hostname match
        if ((pStrHost != NULL) && strcmp(pCacheEntry->strDnsName, pStrHost))
        {
            continue;
        }
        // check for address match
        if ((uAddress != 0) && (pCacheEntry->uAddress != uAddress))
        {
            continue;
        }
        // found a match; delete the entry
        NetPrintfVerbose((iVerbose, 1, "dirtynet: deleting hostname cache entry %s/%a\n", pCacheEntry->strDnsName, pCacheEntry->uAddress));
        ds_memclr(pCacheEntry, sizeof(*pCacheEntry));
        break;
    }
    NetCritLeave(&pCache->Crit);
}

/*F********************************************************************************/
/*!
    \Function SocketHostnameCacheProcess

    \Description
        Process the hostname cache, clear expired cache entries

    \Input *pCache      - hostname cache
    \Input iVerbose     - debug level

    \Version 04/23/2019 (eesponda)
*/
/********************************************************************************F*/
void SocketHostnameCacheProcess(SocketHostnameCacheT *pCache, int32_t iVerbose)
{
    SocketHostnameCacheEntryT *pCacheEntry;
    uint32_t uCurTick;
    int32_t iCacheIdx;

    if (!NetCritTry(&pCache->Crit))
    {
        return;
    }

    // scan cache for dns entry
    for (iCacheIdx = 0, uCurTick = NetTick(); iCacheIdx < pCache->iMaxEntries; iCacheIdx += 1)
    {
        pCacheEntry = &pCache->CacheEntries[iCacheIdx];
        // skip empty entries
        if (pCacheEntry->strDnsName[0] == '\0')
        {
            continue;
        }
        // check for expiration
        if (NetTickDiff(uCurTick, pCacheEntry->uTimer) > DIRTYNET_HOSTNAMECACHELIFETIME)
        {
            NetPrintfVerbose((iVerbose, 1, "dirtynet: expiring hostname cache entry %s/%a\n", pCacheEntry->strDnsName, pCacheEntry->uAddress));
            ds_memclr(pCacheEntry, sizeof(*pCacheEntry));
        }
    }
    NetCritLeave(&pCache->Crit);
}

/*F********************************************************************************/
/*!
    \Function SocketHostnameAddRef

    \Description
        Check for in-progress DNS requests we can piggyback on, instead of issuing
        a new request.

    \Input **ppHostList - list of active lookups
    \Input *pHost       - current lookup
    \Input bUseRef      - force using a new entry if bUseRef=FALSE (should normally be TRUE)

    \Output
        HostentT *      - Pre-existing DNS request we have refcounted, or NULL

    \Version 01/16/2014 (jbrookes)
*/
/********************************************************************************F*/
HostentT *SocketHostnameAddRef(HostentT **ppHostList, HostentT *pHost, uint8_t bUseRef)
{
    HostentT *pHost2 = NULL;

    // look for an in-progress refcounted lookup
    NetCritEnter(NULL);
    if (bUseRef == TRUE)
    {
        for (pHost2 = *ppHostList; pHost2 != NULL; pHost2 = pHost2->pNext)
        {
            if (!strcmp(pHost2->name, pHost->name) && !pHost2->done)
            {
                break;
            }
        }
    }

    // new lookup, so add to list
    if (pHost2 == NULL)
    {
        pHost->refcount = 1;
        pHost->pNext = *ppHostList;
        *ppHostList = pHost;
        pHost = NULL;
    }
    else // found an in-progress lookup, so piggyback on it
    {
        pHost = pHost2;
        pHost->refcount += 1;
        NetPrintfVerbose((SocketInfo(NULL, 'spam', 0, NULL, 0), 0, "dirtynet: %s lookup refcounted (%d refs)\n", pHost->name, pHost->refcount));
    }

    NetCritLeave(NULL);
    return(pHost);
}

/*F********************************************************************************/
/*!
    \Function SocketHostnameListProcess

    \Description
        Process list of in-progress DNS requests, disposing of those that are
        completed and no longer referenced.

    \Input **ppHostList - list of active lookups
    \Input iMemGroup    - memgroup hostname lookup records are allocated with
    \Input *pMemGroupUserData - memgroup userdata hostname lookup records are allocated with

    \Notes
        This function is called from the SocketIdle thread, which is already guarded
        by the global critical section.  It is therefore assumed that it does not
        need to be explicitly guarded here.

    \Version 01/16/2014 (jbrookes)
*/
/********************************************************************************F*/
void SocketHostnameListProcess(HostentT **ppHostList, int32_t iMemGroup, void *pMemGroupUserData)
{
    HostentT **ppHost;
    for (ppHost = ppHostList; *ppHost != NULL;)
    {
        if ((*ppHost)->refcount == 0)
        {
            HostentT *pHost = *ppHost;
            *ppHost = (*ppHost)->pNext;
            DirtyMemFree(pHost, SOCKET_MEMID, iMemGroup, pMemGroupUserData);
        }
        else
        {
            ppHost = &(*ppHost)->pNext;
        }
    }
}

/*
    Packet Queue functions
*/

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueCreate

    \Description
        Create a packet queue

    \Input iMaxPackets          - size of queue, in packets (max 127)
    \Input iMemGroup            - memgroup to alloc/free with
    \Input *pMemGroupUserData   - memgroup user data to alloc/free with

    \Output
        SocketPacketQueueT *    - packet queue, or NULL on failure

    \Version 02/21/2014 (jbrookes)
*/
/********************************************************************************F*/
SocketPacketQueueT *SocketPacketQueueCreate(int32_t iMaxPackets, int32_t iMemGroup, void *pMemGroupUserData)
{
    SocketPacketQueueT *pPacketQueue;
    int32_t iQueueSize;

    // enforce min/max queue sizes
    iMaxPackets = DS_CLAMP(iMaxPackets, 1, DIRTYNET_PACKETQUEUEMAX);

    // calculate memory required for queue
    iQueueSize = sizeof(*pPacketQueue) + ((iMaxPackets-1) * sizeof(pPacketQueue->aPacketQueue[0]));

    // alloc and init queue
    if ((pPacketQueue = DirtyMemAlloc(iQueueSize, SOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynet: could not alloc %d bytes for packet queue\n", iQueueSize));
        return(NULL);
    }
    ds_memclr(pPacketQueue, iQueueSize);

    // set base info
    pPacketQueue->iNumPackets = 0;
    pPacketQueue->iMaxPackets = iMaxPackets;
    pPacketQueue->iMemGroup = iMemGroup;
    pPacketQueue->pMemGroupUserData = pMemGroupUserData;

    // latency/packet loss simulation setup
    pPacketQueue->uLatencyTime = NetTick();

    //$$temp - testing
    //pPacketQueue->uLatency = 100;
    //pPacketQueue->uDeviation = 5;
    //pPacketQueue->uPacketLoss = 5*65536;

    // return queue to caller
    return(pPacketQueue);
}

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueDestroy

    \Description
        Destroy packet queue

    \Input *pPacketQueue    - packet queue to destroy

    \Version 02/21/2014 (jbrookes)
*/
/********************************************************************************F*/
void SocketPacketQueueDestroy(SocketPacketQueueT *pPacketQueue)
{
    DirtyMemFree(pPacketQueue, SOCKET_MEMID, pPacketQueue->iMemGroup, pPacketQueue->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueResize

    \Description
        Resize a packet queue, if the max packet size is different

    \Input *pPacketQueue        - packet queue to resize (may be null)
    \Input iMaxPackets          - new size of queue, in packets (max 127)
    \Input iMemGroup            - memgroup to alloc/free with
    \Input *pMemGroupUserData   - memgroup user data to alloc/free with

    \Output
        SocketPacketQueueT * - pointer to resized packet queue

    \Notes
        If the new max queue size is less than the number of packets in the current
        queue, packets will be overwritten in the usual manner (older discarded in
        favor of newer).

    \Version 05/30/2014 (jbrookes)
*/
/********************************************************************************F*/
SocketPacketQueueT *SocketPacketQueueResize(SocketPacketQueueT *pPacketQueue, int32_t iMaxPackets, int32_t iMemGroup, void *pMemGroupUserData)
{
    uint8_t aPacketData[SOCKET_MAXUDPRECV];
    SocketPacketQueueT *pNewPacketQueue;
    struct sockaddr PacketAddr;
    int32_t iPacketSize;

    // enforce min/max queue sizes
    iMaxPackets = DS_CLAMP(iMaxPackets, 1, DIRTYNET_PACKETQUEUEMAX);

    // if we have a queue and it's already the right size, return it
    if ((pPacketQueue != NULL) && (pPacketQueue->iMaxPackets == iMaxPackets))
    {
        return(pPacketQueue);
    }

    // create new queue
    NetPrintf(("dirtynet: [%p] re-creating socket packet queue with %d max packets\n", pPacketQueue, iMaxPackets));
    if ((pNewPacketQueue = SocketPacketQueueCreate(iMaxPackets, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynet: could not allocate new packet queue\n"));
        return(pPacketQueue);
    }

    // copy old data (if any) over, and destroy the old packet queue
    if (pPacketQueue != NULL)
    {
        while ((iPacketSize = SocketPacketQueueRem(pPacketQueue, aPacketData, sizeof(aPacketData), &PacketAddr)) > 0)
        {
            SocketPacketQueueAdd(pNewPacketQueue, aPacketData, iPacketSize, &PacketAddr);
        }
        SocketPacketQueueDestroy(pPacketQueue);
    }

    // return resized queue to caller
    return(pNewPacketQueue);
}

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueAdd

    \Description
        Add a packet to packet queue

    \Input *pPacketQueue    - packet queue to add to
    \Input *pPacketData     - packet data to add to queue
    \Input iPacketSize      - size of packet data
    \Input *pPacketAddr     - remote address associated with packet

    \Output
        int32_t             - >=0: number of bytes buffered, -1: packet too large

    \Version 07/28/2020 (mclouatre)
*/
/********************************************************************************F*/
int32_t SocketPacketQueueAdd(SocketPacketQueueT *pPacketQueue, const uint8_t *pPacketData, int32_t iPacketSize, struct sockaddr *pPacketAddr)
{
    return(SocketPacketQueueAdd2(pPacketQueue, pPacketData, iPacketSize, pPacketAddr, FALSE));
}

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueAdd2

    \Description
        Add a packet to packet queue

    \Input *pPacketQueue    - packet queue to add to
    \Input *pPacketData     - packet data to add to queue
    \Input iPacketSize      - size of packet data
    \Input *pPacketAddr     - remote address associated with packet
    \Input bPartialAllowed  - allow consuming only a portion of the submitted data (NX only)

    \Output
        int32_t             - >=0: number of bytes buffered, -1: packet too large

    \Version 02/21/2014 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketPacketQueueAdd2(SocketPacketQueueT *pPacketQueue, const uint8_t *pPacketData, int32_t iPacketSize, struct sockaddr *pPacketAddr, uint32_t bPartialAllowed)
{
    SocketPacketQueueEntryT *pQueueEntry;

    if (bPartialAllowed == FALSE)
    {
        // reject if packet data is too large
        if (iPacketSize > SOCKET_MAXUDPRECV)
        {
            NetPrintf(("dirtynet: [%p] packet too large to add to queue\n", pPacketQueue));
            return(-1);
        }
    }

    // if queue is full, overwrite oldest member
    if (pPacketQueue->iNumPackets == pPacketQueue->iMaxPackets)
    {
        NetPrintf(("dirtynet: [%p] add to full queue; oldest entry will be overwritten\n", pPacketQueue));
        pPacketQueue->iPacketHead = (pPacketQueue->iPacketHead + 1) % pPacketQueue->iMaxPackets;
        pPacketQueue->uPacketDrop += 1;
    }
    else
    {
        pPacketQueue->iNumPackets += 1;
        if (pPacketQueue->uPacketMax < (unsigned)pPacketQueue->iNumPackets)
        {
            pPacketQueue->uPacketMax = (unsigned)pPacketQueue->iNumPackets;
        }
    }
    // set packet entry
    NetPrintfVerbose((DIRTYNET_PACKETQUEUEDEBUG, 0, "dirtynet: [%p] [%d] packet queue entry added (%d entries)\n", pPacketQueue, pPacketQueue->iPacketTail, pPacketQueue->iNumPackets));
    pQueueEntry = &pPacketQueue->aPacketQueue[pPacketQueue->iPacketTail];
    
    if (iPacketSize > (signed)sizeof(pQueueEntry->aPacketData))
    {
        iPacketSize = sizeof(pQueueEntry->aPacketData);
    }
    ds_memcpy_s(pQueueEntry->aPacketData, sizeof(pQueueEntry->aPacketData), pPacketData, iPacketSize);

    if (pPacketAddr)
    { 
        ds_memcpy(&pQueueEntry->PacketAddr, pPacketAddr, sizeof(pQueueEntry->PacketAddr));
    }
    else
    {
        ds_memclr(&pQueueEntry->PacketAddr, sizeof(pQueueEntry->PacketAddr));
        pQueueEntry->PacketAddr.sa_family = AF_UNSPEC;
    }

    pQueueEntry->iPacketSize = iPacketSize;
    pQueueEntry->uPacketTick = NetTick();
    // add to queue
    pPacketQueue->iPacketTail = (pPacketQueue->iPacketTail + 1) % pPacketQueue->iMaxPackets;
    NetPrintfVerbose((DIRTYNET_PACKETQUEUEDEBUG, 0, "dirtynet: [%p] head=%d tail=%d\n", pPacketQueue, pPacketQueue->iPacketHead, pPacketQueue->iPacketTail));
    // return number of bytes buffered
    return(pQueueEntry->iPacketSize);
}

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueAlloc

    \Description
        Alloc a packet queue entry.  This is used when receiving data directly into
        the packet queue data buffer.

    \Input *pPacketQueue    - packet queue to alloc entry from

    \Output
        SocketPacketQueueEntryT * - packet queue entry

    \Version 02/24/2014 (jbrookes)
*/
/********************************************************************************F*/
SocketPacketQueueEntryT *SocketPacketQueueAlloc(SocketPacketQueueT *pPacketQueue)
{
    SocketPacketQueueEntryT *pQueueEntry;
    // if queue is full, alloc over oldest member
    if (pPacketQueue->iNumPackets == pPacketQueue->iMaxPackets)
    {
        // print a warning if we're altering a slot that is in use
        if (pPacketQueue->aPacketQueue[pPacketQueue->iPacketTail].iPacketSize != -1)
        {
            NetPrintf(("dirtynet: [%p] alloc to full queue; oldest entry will be overwritten\n", pPacketQueue));
            pPacketQueue->uPacketDrop += 1;
        }
        pPacketQueue->iPacketHead = (pPacketQueue->iPacketHead + 1) % pPacketQueue->iMaxPackets;
    }
    else
    {
        pPacketQueue->iNumPackets += 1;
        if (pPacketQueue->uPacketMax < (unsigned)pPacketQueue->iNumPackets)
        {
            pPacketQueue->uPacketMax = (unsigned)pPacketQueue->iNumPackets;
        }
    }
    // allocate queue entry
    pQueueEntry = &pPacketQueue->aPacketQueue[pPacketQueue->iPacketTail];
    NetPrintfVerbose((DIRTYNET_PACKETQUEUEDEBUG, 0, "dirtynet: [%p] [%d] packet queue entry alloc (%d entries)\n", pPacketQueue, pPacketQueue->iPacketTail, pPacketQueue->iNumPackets));
    // mark packet as allocated
    pQueueEntry->iPacketSize = -1;
    // add to queue
    pPacketQueue->iPacketTail = (pPacketQueue->iPacketTail + 1) % pPacketQueue->iMaxPackets;
    NetPrintfVerbose((DIRTYNET_PACKETQUEUEDEBUG, 0, "dirtynet: [%p] head=%d tail=%d\n", pPacketQueue, pPacketQueue->iPacketHead, pPacketQueue->iPacketTail));
    return(pQueueEntry);
}

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueAllocUndo

    \Description
        Undo a recent call to SocketPacketQueueAlloc().  Cannot be used if packet
        queue has been altered in between calling SocketPacketQueueAlloc() and
        SocketPacketQueueAllocUndo()

    \Input *pPacketQueue    - packet queue to undo alloc entry from

    \Version 09/04/2018 (mclouatre)
*/
/********************************************************************************F*/
void SocketPacketQueueAllocUndo(SocketPacketQueueT *pPacketQueue)
{
    pPacketQueue->iNumPackets -= 1;
    NetPrintfVerbose((DIRTYNET_PACKETQUEUEDEBUG, 0, "dirtynet: [%p] [%d] recent alloc undone from tail (%d entries)\n", pPacketQueue, pPacketQueue->iPacketTail, pPacketQueue->iNumPackets));

    // modular arithmetic is not used here to avoid complications when the substraction underflows
    pPacketQueue->iPacketTail -= 1;
    if (pPacketQueue->iPacketTail < 0)
    {
        pPacketQueue->iPacketTail = pPacketQueue->iMaxPackets - 1;
    }

    NetPrintfVerbose((DIRTYNET_PACKETQUEUEDEBUG, 0, "dirtynet: [%p] head=%d tail=%d\n", pPacketQueue, pPacketQueue->iPacketHead, pPacketQueue->iPacketTail));
}

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueRem

    \Description
        Remove a packet from packet queue

    \Input *pPacketQueue    - packet queue to remove from
    \Input *pPacketData     - [out] storage for packet data or NULL
    \Input iPacketSize      - size of packet output data buffer
    \Input *pPacketAddr     - [out] storage for packet addr or NULL

    \Output
        int32_t             - positive=size of packet, zero=no packet, negative=failure

    \Notes
        The packet data and address outputs are optional if you are just
        trying to remove the entry at the head.

    \Version 02/21/2014 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketPacketQueueRem(SocketPacketQueueT *pPacketQueue, uint8_t *pPacketData, int32_t iPacketSize, struct sockaddr *pPacketAddr)
{
    SocketPacketQueueEntryT *pQueueEntry;
    uint32_t uCurTick = NetTick();

    // nothing to do if queue is empty
    if (pPacketQueue->iNumPackets == 0)
    {
        return(0);
    }
    // get head packet
    pQueueEntry = &pPacketQueue->aPacketQueue[pPacketQueue->iPacketHead];
    // nothing to do if packet was allocated and has not been filled
    if (pQueueEntry->iPacketSize < 0)
    {
        return(0);
    }

    // apply simulated latency, if enabled
    if (pPacketQueue->uLatency != 0)
    {
        // compare to current latency
        if (NetTickDiff(uCurTick, pQueueEntry->uPacketTick) < (signed)pPacketQueue->uLatency + pPacketQueue->iDeviationTime)
        {
            NetPrintfVerbose((DIRTYNET_PACKETQUEUEDEBUG, 0, "dirtynet: [%p] latency=%d, deviation=%d\n", pPacketQueue, NetTickDiff(uCurTick, pPacketQueue->uLatencyTime), pPacketQueue->iDeviationTime));
            return(0);
        }

        // update packet receive timestamp
        SockaddrInSetMisc(&pQueueEntry->PacketAddr, uCurTick);

        // recalculate deviation
        pPacketQueue->iDeviationTime = (signed)NetRand(pPacketQueue->uDeviation*2) - (signed)pPacketQueue->uDeviation;
    }

    // get packet size to copy (will truncate if output buffer is too small)
    if (iPacketSize > pQueueEntry->iPacketSize)
    {
        iPacketSize = pQueueEntry->iPacketSize;
    }
    // copy out packet data and source
    if (pPacketData != NULL)
    {
        ds_memcpy(pPacketData, pQueueEntry->aPacketData, iPacketSize);
    }
    if (pPacketAddr != NULL)
    {
        ds_memcpy(pPacketAddr, &pQueueEntry->PacketAddr, sizeof(pQueueEntry->PacketAddr));
    }
    // remove packet from queue
    pPacketQueue->iNumPackets -= 1;
    NetPrintfVerbose((DIRTYNET_PACKETQUEUEDEBUG, 0, "dirtynet: [%p] [%d] packet queue entry removed from head in %dms (%d entries)\n", pPacketQueue, pPacketQueue->iPacketHead,
        NetTickDiff(NetTick(), pQueueEntry->uPacketTick), pPacketQueue->iNumPackets));
    pPacketQueue->iPacketHead = (pPacketQueue->iPacketHead + 1) % pPacketQueue->iMaxPackets;
    NetPrintfVerbose((DIRTYNET_PACKETQUEUEDEBUG, 0, "dirtynet: [%p] head=%d tail=%d\n", pPacketQueue, pPacketQueue->iPacketHead, pPacketQueue->iPacketTail));

    // simulate packet loss
    if (pPacketQueue->uPacketLoss != 0)
    {
        uint32_t uRand = NetRand(100*65536);
        if (uRand < pPacketQueue->uPacketLoss)
        {
            NetPrintf(("dirtynet: [%p] lost packet (rand=%d, comp=%d)!\n", pPacketQueue, uRand, pPacketQueue->uPacketLoss));
            return(0);
        }
    }

    // return success
    return(pQueueEntry->iPacketSize);
}

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueRemStream

    \Description
        Remove a stream packet from packet queue

    \Input *pPacketQueue    - packet queue to add to
    \Input *pPacketData     - [out] storage for packet data or NULL
    \Input iPacketSize      - size of packet output data buffer

    \Output
        int32_t             - positive=amount of data read, zero=no packet, negative=failure

    \Notes
        The packet data output is optional if you are just trying to remove the entry
        at the head.

    \Version 11/20/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t SocketPacketQueueRemStream(SocketPacketQueueT *pPacketQueue, uint8_t *pPacketData, int32_t iPacketSize)
{
    SocketPacketQueueEntryT *pQueueEntry;
    int32_t iPacketRead;

    // nothing to do if queue is empty
    if (pPacketQueue->iNumPackets == 0)
    {
        return(0);
    }
    // get head packet
    pQueueEntry = &pPacketQueue->aPacketQueue[pPacketQueue->iPacketHead];
    // nothing to do if packet was allocated and has not been filled
    if (pQueueEntry->iPacketSize < 0)
    {
        return(0);
    }

    // get packet size to copy
    iPacketRead = DS_MIN(iPacketSize, pQueueEntry->iPacketSize);

    // copy the packet data and offset by amount read
    if (pPacketData != NULL)
    {
        ds_memcpy(pPacketData, pQueueEntry->aPacketData, iPacketRead);

        pPacketData += iPacketRead;
        iPacketSize -= iPacketRead;
    }

    // if we copied less than the size of the entry adjust the entry as needed, otherwise remove packet from queue
    if ((iPacketRead > 0) && (iPacketRead < pQueueEntry->iPacketSize))
    {
        memmove(pQueueEntry->aPacketData, pQueueEntry->aPacketData+iPacketRead, pQueueEntry->iPacketSize-iPacketRead);
        pQueueEntry->iPacketSize -= iPacketRead;
    }
    else
    {
        pPacketQueue->iNumPackets -= 1;
        pPacketQueue->iPacketHead = (pPacketQueue->iPacketHead + 1) % pPacketQueue->iMaxPackets;
    }

    // continue to copy if we still have space in the buffer
    if (iPacketSize > 0)
    {
        iPacketRead += SocketPacketQueueRemStream(pPacketQueue, pPacketData, iPacketSize);
    }
    return(iPacketRead);
}

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueGetHead

    \Description
        Get pointers to data associated with queue head.

    \Input *pPacketQueue    - packet queue to remove from
    \Input **ppPacketData   - [out] to be filled with pointer to packet data (can be NULL)
    \Input *pPacketSize     - [out] to be filled with size of packet data (can be NULL)
    \Input **ppPacketAddr   - [out] to be filled with pointer to packet addr (can be NULL)

    \Output
        int32_t             - positive=size of packet, zero=no packet

    \Version 08/07/2020 (mclouatre)
*/
/********************************************************************************F*/
int32_t SocketPacketQueueGetHead(SocketPacketQueueT *pPacketQueue, uint8_t **ppPacketData, int32_t *pPacketSize, struct sockaddr **ppPacketAddr)
{
    SocketPacketQueueEntryT *pQueueEntry;

    // nothing to do if queue is empty
    if (pPacketQueue->iNumPackets == 0)
    {
        return(0);
    }

    // get head packet
    pQueueEntry = &pPacketQueue->aPacketQueue[pPacketQueue->iPacketHead];
    // nothing to do if packet was allocated and has not been filled
    if (pQueueEntry->iPacketSize < 0)
    {
        return(0);
    }

    // fill output variables
    if (ppPacketData != NULL)
    {
        *ppPacketData = pQueueEntry->aPacketData;
    }
    if (pPacketSize != NULL)
    {
        *pPacketSize = pQueueEntry->iPacketSize;
    }
    if (ppPacketAddr != NULL)
    {
        *ppPacketAddr = &pQueueEntry->PacketAddr;
    }

    // return success
    return(pQueueEntry->iPacketSize);
}

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueTouchHead

    \Description
        Update queue's head entry such that it no longer includes successfully
        consumed portion of data.

    \Input *pPacketQueue    - packet queue to remove from
    \Input iConsumedSize    - amount of data consumed from head entry (in bytes)

    \Output
        int32_t             - 0 success, negative: error

    \Version 08/07/2020 (mclouatre)
*/
/********************************************************************************F*/
int32_t SocketPacketQueueTouchHead(SocketPacketQueueT *pPacketQueue, int32_t iConsumedSize)
{
    SocketPacketQueueEntryT *pQueueEntry;

    // nothing to do if queue is empty
    if (pPacketQueue->iNumPackets == 0)
    {
        return(-1);
    }

    // get head packet
    pQueueEntry = &pPacketQueue->aPacketQueue[pPacketQueue->iPacketHead];
    // nothing to do if packet was allocated and has not been filled
    if (pQueueEntry->iPacketSize < 0)
    {
        return(-2);
    }

    // update size of packet queue entry
    pQueueEntry->iPacketSize -= iConsumedSize;

    // remove consumed data from data buffer
    memmove(pQueueEntry->aPacketData, pQueueEntry->aPacketData + iConsumedSize, pQueueEntry->iPacketSize);

    // return success
    return(0);
}


/*F********************************************************************************/
/*!
    \Function SocketPacketQueueControl

    \Description
        Control socket packet queue options

    \Input *pPacketQueue    - packet queue control function; different selectors
                              control different behaviors
    \Input iControl         - control selector
    \Input iValue           - selector specific

    \Output
        int32_t             - selector result

    \Notes
        iControl can be one of the following:

        \verbatim
            'pdev' - set simulated packet deviation
            'plat' - set simulated packet latency
            'plos' - set simulated packet loss
        \endverbatim

    \Version 10/07/2014 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketPacketQueueControl(SocketPacketQueueT *pPacketQueue, int32_t iControl, int32_t iValue)
{
    if (iControl == 'pdev')
    {
        NetPrintf(("dirtynet: setting simulated packet deviation=%dms\n", iValue));
        pPacketQueue->uDeviation = iValue;
        return(0);
    }
    if (iControl == 'plat')
    {
        NetPrintf(("dirtynet: setting simulated packet latency=%dms\n", iValue));
        pPacketQueue->uLatency = iValue;
        return(0);
    }
    if (iControl == 'plos')
    {
        NetPrintf(("dirtynet: setting simulated packet loss to %d.%d\n", iValue >> 16, iValue & 0xffff));
        pPacketQueue->uPacketLoss = iValue;
        return(0);
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueStatus

    \Description
        Get status of socket packet queue

    \Input *pPacketQueue    - packet queue to get status of
    \Input iStatus          - status selector

    \Output
        int32_t             - selector result

    \Notes
        iStatus can be one of the following:

        \verbatim
            'pful' - TRUE if queue is full, FALSE otherwise
            'pdrp' - number of packets overwritten due to queue overflow
            'pmax' - maximum number of packets in queue (high water)
            'pnum' - number of packets in queue
            'psiz' - queue size (in packets)
        \endverbatim

    \Version 10/20/2015 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketPacketQueueStatus(SocketPacketQueueT *pPacketQueue, int32_t iStatus)
{
    if (iStatus == 'pful')
    {
        return(pPacketQueue->iMaxPackets == pPacketQueue->iNumPackets ? TRUE : FALSE);
    }
    if (iStatus == 'pdrp')
    {
        return((signed)pPacketQueue->uPacketDrop);
    }
    if (iStatus == 'pmax')
    {
        return((signed)pPacketQueue->uPacketMax);
    }
    if (iStatus == 'pnum')
    {
        return(pPacketQueue->iNumPackets);
    }
    if (iStatus == 'psiz')
    {
        return(pPacketQueue->iMaxPackets);
    }
    // invalid selector
    return(-1);
}

/*
    Rate functions
*/

/*F********************************************************************************/
/*!
    \Function SocketRateUpdate

    \Description
        Update socket data rate estimation

    \Input *pRate           - state used to store rate estimation data
    \Input iData            - amount of data being sent/recv
    \Input *pOpName         - indicates send or recv (for debug use only)

    \Notes
        Rate estimation is based on a rolling 16-deep history of ~100ms samples
        of data rate (the actual period may vary slightly based on update rate
        and tick resolution) covering a total of ~1600-1700ms.  We also keep track
        of the rate we are called at (excluding multiple calls within the same
        tick) so we can estimate how much data we need to have sent by the next
        time we are called.  This is important because we will always be shooting
        lower than our cap if we don't consider this factor, and the amount we
        are shooting lower by increases the slower the update rate.

    \Version 08/20/2014 (jbrookes)
*/
/********************************************************************************F*/
void SocketRateUpdate(SocketRateT *pRate, int32_t iData, const char *pOpName)
{
    const uint32_t uMaxIndex = (unsigned)(sizeof(pRate->aDataHist)/sizeof(pRate->aDataHist[0]));
    uint32_t uCurTick = NetTick(), uOldTick;
    uint32_t uIndex, uCallRate, uCallSum, uDataSum;
    int32_t iTickDiff;

    // reserve tick=0 as uninitialized
    if (uCurTick == 0)
    {
        uCurTick = 1;
    }
    // initialize update tick counters
    if (pRate->uLastTick == 0)
    {
        pRate->uLastTick = NetTickDiff(uCurTick, 2);
        // initialize rate tick counter to current-100 so as to force immediate history update
        pRate->uLastRateTick = NetTickDiff(uCurTick, 100);
        // start off at max rate
        pRate->uCurRate = pRate->uNextRate = pRate->uMaxRate;
    }
    // exclude error results
    if (iData < 0)
    {
        return;
    }

    // update the data
    pRate->aDataHist[pRate->uDataIndex] += iData;
    /* update the call count, only only if time has elapsed since our last call, since we
       want the true update rate) */
    if (NetTickDiff(uCurTick, pRate->uLastTick) > 1)
    {
        pRate->aCallHist[pRate->uDataIndex] += 1;
    }
    // update last update tick
    pRate->uLastTick = uCurTick;
    /* update timestamp, but only if it hasn't been updated already.  we do this so we can
       correctly update the rate calculation continuously with the current sample */
    if (pRate->aTickHist[pRate->uDataIndex] == 0)
    {
        pRate->aTickHist[pRate->uDataIndex] = uCurTick;
    }

    // get oldest tick & sum recorded data & callcounts
    for (uIndex = (pRate->uDataIndex + 1) % uMaxIndex, uOldTick = 0, uCallSum = 0, uDataSum = 0; ; uIndex = (uIndex + 1) % uMaxIndex)
    {
        // skip uninitialized tick values
        if ((uOldTick == 0) && (pRate->aTickHist[uIndex] != 0))
        {
            uOldTick = pRate->aTickHist[uIndex];
        }
        // update call sum
        uCallSum += pRate->aCallHist[uIndex];
        // update data sum
        uDataSum += pRate->aDataHist[uIndex];
        // quit when we've hit every entry
        if (uIndex == pRate->uDataIndex)
        {
            break;
        }
    }

    // update rate estimation
    if ((iTickDiff = NetTickDiff(uCurTick, uOldTick)) > 0)
    {
        // calculate call rate
        uCallRate = (uCallSum > 0) ? iTickDiff/uCallSum : 0;
        // update current rate estimation
        pRate->uCurRate = (uDataSum * 1000) / iTickDiff;
        // update next rate estimation (fudge slightly by 2x as it gives us better tracking to our desired rate)
        pRate->uNextRate = (uDataSum * 1000) / (iTickDiff+(uCallRate*2));

        #if DIRTYNET_RATEDEBUG
        if (pRate->uCurRate != 0)
        {
            NetPrintf(("dirtynet: [%p] rate=%4.2fkb nextrate=%4.2f callrate=%dms tickdiff=%d tick=%d\n", pRate, ((float)pRate->uCurRate)/1024.0f, ((float)pRate->uNextRate)/1024.0f, uCallRate, iTickDiff, uCurTick));
        }
        #endif
    }

    // move to next slot in history every 100ms
    if (NetTickDiff(uCurTick, pRate->uLastRateTick) >= 100)
    {
        pRate->uDataIndex = (pRate->uDataIndex + 1) % uMaxIndex;
        pRate->aDataHist[pRate->uDataIndex] = 0;
        pRate->aTickHist[pRate->uDataIndex] = 0;
        pRate->aCallHist[pRate->uDataIndex] = 0;
        pRate->uLastRateTick = uCurTick;

        #if DIRTYNET_RATEDEBUG
        if (pRate->uCurRate != 0)
        {
            NetPrintf(("dirtynet: [%p] %s=%5d rate=%4.2fkb/s indx=%2d tick=%08x diff=%d\n", pRate, pOpName, uDataSum, ((float)pRate->uCurRate)/1024.0f, pRate->uDataIndex, uCurTick, iTickDiff));
        }
        #endif
    }
}

/*F********************************************************************************/
/*!
    \Function SocketRateThrottle

    \Description
        Throttles data size to send or recv based on calculated data rate and
        configured max rate.

    \Input *pRate           - state used to calculate rate
    \Input iSockType        - socket type (SOCK_DGRAM, SOCK_STREAM, etc)
    \Input iData            - amount of data being sent/recv
    \Input *pOpName         - indicates send or recv (for debug use only)

    \Output
        int32_t             - amount of data to send/recv

    \Version 08/20/2014 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketRateThrottle(SocketRateT *pRate, int32_t iSockType, int32_t iData, const char *pOpName)
{
    int32_t iRateDiff;

    // max rate of zero means rate throttling is disabled
    if ((pRate->uMaxRate == 0) || (iSockType != SOCK_STREAM))
    {
        return(iData);
    }

    // enforce min max update rate
    if (pRate->uMaxRate < DIRTYNET_MIN_THROTTLE_RATE)
    {
        NetPrintf(("dirtynet: [%p] clamping max rate throttle of %d to %d", pRate->uMaxRate, DIRTYNET_MIN_THROTTLE_RATE));
        pRate->uMaxRate = DIRTYNET_MIN_THROTTLE_RATE;
    }

    // if we're exceeding the max rate, update rate estimation and return no data
    if ((iRateDiff = pRate->uMaxRate - pRate->uNextRate) <= 0)
    {
        NetPrintfVerbose((DIRTYNET_RATEDEBUG, 0, "dirtynet: [%p] exceeding max rate; clamping to zero\n", pRate));
        iData = 0;
    }
    else if (iData > iRateDiff) // return a limited amount of data so as not to exceed max rate
    {
        /* clamp in multiples of the typical TCP maximum segment size, so as not to generate fragmented packets
           note that the TCP maximum segment size is equal to our minimum throttle rate, otherwise setting the
           uMaxRate to less than this value would result in iData always being equal to 0. */
        iData = (iRateDiff / DIRTYNET_MIN_THROTTLE_RATE) * DIRTYNET_MIN_THROTTLE_RATE;
        NetPrintfVerbose((DIRTYNET_RATEDEBUG, 0, "dirtynet: [%p] exceeding max rate; clamping to %d bytes from %d bytes\n", pRate, iRateDiff, pRate->uMaxRate - pRate->uNextRate));
    }

    // if we are returning no data, update the rate as the caller won't
    if (iData == 0)
    {
        SocketRateUpdate(pRate, 0, pOpName);
    }

    return(iData);
}

/*
    Send Callback functions
*/

/*F********************************************************************************/
/*!
    \Function SocketSendCallbackAdd

    \Description
        Register a new socket send callback

    \Input aCbEntries   - collection of callbacks to add to
    \Input *pCbEntry    - entry to be added

    \Output
        int32_t         - zero=success; negative=failure

    \Version 07/18/2014 (mclouatre)
*/
/********************************************************************************F*/
int32_t SocketSendCallbackAdd(SocketSendCallbackEntryT aCbEntries[], SocketSendCallbackEntryT *pCbEntry)
{
    int32_t iRetCode = -1; // default to failure
    int32_t iEntryIndex;

    for(iEntryIndex = 0; iEntryIndex < SOCKET_MAXSENDCALLBACKS; iEntryIndex++)
    {
        if (aCbEntries[iEntryIndex].pSendCallback == NULL)
        {
            aCbEntries[iEntryIndex].pSendCallback = pCbEntry->pSendCallback;
            aCbEntries[iEntryIndex].pSendCallref = pCbEntry->pSendCallref;

            NetPrintf(("dirtynet: adding send callback (%p, %p)\n", pCbEntry->pSendCallback, pCbEntry->pSendCallref));

            iRetCode = 0;  // success

            break;
        }
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function SocketSendCallbackRem

    \Description
        Unregister a socket send callback that was already registered.

    \Input aCbEntries   - collection of callbacks to remove from
    \Input *pCbEntry    - entry to be removed

    \Output
        int32_t         - zero=success; negative=failure

    \Version 07/18/2014 (mclouatre)
*/
/********************************************************************************F*/
int32_t SocketSendCallbackRem(SocketSendCallbackEntryT aCbEntries[], SocketSendCallbackEntryT *pCbEntry)
{
    int32_t iRetCode = -1; // default to failure
    int32_t iEntryIndex;

    for(iEntryIndex = 0; iEntryIndex < SOCKET_MAXSENDCALLBACKS; iEntryIndex++)
    {
        if (aCbEntries[iEntryIndex].pSendCallback == pCbEntry->pSendCallback && aCbEntries[iEntryIndex].pSendCallref == pCbEntry->pSendCallref)
        {
            NetPrintf(("dirtynet: removing send callback (%p, %p)\n", aCbEntries[iEntryIndex].pSendCallback,  aCbEntries[iEntryIndex].pSendCallref));

            aCbEntries[iEntryIndex].pSendCallback = NULL;
            aCbEntries[iEntryIndex].pSendCallref = NULL;

            iRetCode = 0;  // success

            break;
        }
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function SocketSendCallbackInvoke

    \Description
        Invoke all send callbacks in specified collection. Only one of the callback
        is supposed to return "handled"... warn if not the case.

    \Input aCbEntries   - collection of callbacks to be invoked
    \Input pSocket      - socket reference
    \Input iType        - socket type
    \Input *pBuf        - the data to be sent
    \Input iLen         - size of data
    \Input *pTo         - the address to send to (NULL=use connection address)

    \Output
        int32_t         - 0 = send not handled; >0 = send successfully handled (bytes sent); <0 = send handled but failed (SOCKERR_XXX)

    \Version 07/18/2014 (mclouatre)
*/
/********************************************************************************F*/
int32_t SocketSendCallbackInvoke(SocketSendCallbackEntryT aCbEntries[], SocketT *pSocket, int32_t iType, const char *pBuf, int32_t iLen, const struct sockaddr *pTo)
{
    int32_t iRetCode = 0;  // default to send not handled
    int32_t iResult;
    int32_t iEntryIndex;

    for (iEntryIndex = 0; iEntryIndex < SOCKET_MAXSENDCALLBACKS; iEntryIndex++)
    {
        // we expect zero or one send callback to handle the send, more than one indicates an invalid condition
        if (aCbEntries[iEntryIndex].pSendCallback != NULL)
        {
            if((iResult = aCbEntries[iEntryIndex].pSendCallback(pSocket, iType, (const uint8_t *)pBuf, iLen, pTo, aCbEntries[iEntryIndex].pSendCallref)) != 0)
            {
                if (iRetCode == 0)
                {
                    iRetCode = iResult;
                }
                else
                {
                    NetPrintf(("dirtynet: critical error - send handled by more than one callback  (iEntryIndex = %d, iResult = %d)\n", iEntryIndex, iResult));
                }
            }
        }
    }

    return(iRetCode);
}

/*

    Addr map functions

*/
#ifndef DIRTYCODE_NX
/*F*************************************************************************************/
/*!
    \Function _Sockaddr6SetAddrV4Mapped

    \Description
        Set a V4-mapped IPv6 in6_addr

    \Input *pAddr6      - [out] storage for IPv4-mapped IPv6 address
    \Input uAddr4       - IPv4 address to map

    \Version 03/01/2016 (jbrookes)
*/
/************************************************************************************F*/
static void _Sockaddr6SetAddrV4Mapped(struct in6_addr *pAddr6, uint32_t uAddr4)
{
    ds_memclr(pAddr6, sizeof(*pAddr6));
    pAddr6->s6_addr[10] = 0xff;
    pAddr6->s6_addr[11] = 0xff;
    pAddr6->s6_addr[12] = (uint8_t)(uAddr4 >> 24);
    pAddr6->s6_addr[13] = (uint8_t)(uAddr4 >> 16);
    pAddr6->s6_addr[14] = (uint8_t)(uAddr4 >> 8);
    pAddr6->s6_addr[15] = (uint8_t)(uAddr4);
}

/*F*************************************************************************************/
/*!
    \Function _Sockaddr6SetV4Mapped

    \Description
        Set a V4-mapped IPv6 sockaddr6

    \Input *pResult6    - [out] storage for IPv4-mapped IPv6 sockaddr
    \Input *pSource4    - IPv4 address to map

    \Version 03/01/2016 (jbrookes)
*/
/************************************************************************************F*/
static void _Sockaddr6SetV4Mapped(struct sockaddr_in6 *pResult6, const struct sockaddr *pSource4)
{
    pResult6->sin6_family = AF_INET6;
    pResult6->sin6_port = SocketNtohs(SockaddrInGetPort(pSource4));
    pResult6->sin6_flowinfo = 0;
    _Sockaddr6SetAddrV4Mapped(&pResult6->sin6_addr, SockaddrInGetAddr(pSource4));
    pResult6->sin6_scope_id = 0;
}

/*F*************************************************************************************/
/*!
    \Function _SocketAddrMapAlloc

    \Description
        Alloc (or re-alloc) address map entry list

    \Input *pAddrMap            - address map
    \Input iNumEntries          - new entry list size
    \Input iMemGroup            - memory group
    \Input *pMemGroupUserData   - memory group user data

    \Output
        int32_t         - negative=error, zero=success

    \Version 04/17/2013 (jbrookes)
*/
/************************************************************************************F*/
static int32_t _SocketAddrMapAlloc(SocketAddrMapT *pAddrMap, int32_t iNumEntries, int32_t iMemGroup, void *pMemGroupUserData)
{
    int32_t iOldEntryListSize = 0, iNewEntryListSize = iNumEntries*sizeof(SocketAddrMapEntryT);
    SocketAddrMapEntryT *pNewEntries;

    // allocate new map memory
    if ((pNewEntries = (SocketAddrMapEntryT *)DirtyMemAlloc(iNewEntryListSize, SOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        return(-1);
    }
    // clear new map memory
    ds_memclr(pNewEntries, iNewEntryListSize);
    // copy previous map data
    if (pAddrMap->pMapEntries != NULL)
    {
        iOldEntryListSize = pAddrMap->iNumEntries*sizeof(SocketAddrMapEntryT);
        ds_memcpy(pNewEntries, pAddrMap->pMapEntries, iOldEntryListSize);
        DirtyMemFree(pAddrMap->pMapEntries, SOCKET_MEMID, iMemGroup, pMemGroupUserData);
    }
    // update state
    pAddrMap->iNumEntries = iNumEntries;
    pAddrMap->pMapEntries = pNewEntries;
    pAddrMap->iMemGroup = iMemGroup;
    pAddrMap->pMemGroupUserData = pMemGroupUserData;
    // return success
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function _SocketAddrMapGet

    \Description
        Find a map entry, based on specified address

    \Input *pAddrMap    - address map
    \Input *pAddr       - address to get entry for
    \Input iAddrSize    - size of address

    \Output
        SockAddrMapEntryT * - pointer to map entry or NULL if not found

    \Version 04/17/2013 (jbrookes)
*/
/************************************************************************************F*/
static SocketAddrMapEntryT *_SocketAddrMapGet(const SocketAddrMapT *pAddrMap, const struct sockaddr *pAddr, int32_t iAddrSize)
{
    const struct sockaddr_in6 *pAddr6 = (const struct sockaddr_in6 *)pAddr;
    SocketAddrMapEntryT *pMapEntry;
    int32_t iMapEntry;

    for (iMapEntry = 0; iMapEntry < pAddrMap->iNumEntries; iMapEntry += 1)
    {
        pMapEntry = &pAddrMap->pMapEntries[iMapEntry];
        if ((pAddr->sa_family == AF_INET) && (pMapEntry->iVirtualAddress == SockaddrInGetAddr(pAddr)))
        {
            return(pMapEntry);
        }
        if ((pAddr->sa_family == AF_INET6) && (!memcmp(&pAddr6->sin6_addr, &pMapEntry->SockAddr6.sin6_addr, sizeof(pAddr6->sin6_addr))))
        {
            return(pMapEntry);
        }
    }
    return(NULL);
}

/*F*************************************************************************************/
/*!
    \Function _SocketAddrMapSet

    \Description
        Initialize a map entry

    \Input *pAddrMap    - address map
    \Input *pMapEntry   - entry to set
    \Input *pAddr6      - IPv6 address to set
    \Input iAddrSize    - size of address

    \Version 04/17/2013 (jbrookes)
*/
/************************************************************************************F*/
static void _SocketAddrMapSet(SocketAddrMapT *pAddrMap, SocketAddrMapEntryT *pMapEntry, const struct sockaddr_in6 *pAddr6, int32_t iAddrSize)
{
    pMapEntry->iRefCount = 1;
    pMapEntry->iVirtualAddress = pAddrMap->iNextVirtAddr;
    pAddrMap->iNextVirtAddr = (pAddrMap->iNextVirtAddr + 1) & 0x00ffffff;
    ds_memcpy(&pMapEntry->SockAddr6, pAddr6, iAddrSize);
    NetPrintfVerbose((SocketInfo(NULL, 'spam', 0, NULL, 0), 1, "dirtynet: [%d] add map %A to %a\n", pMapEntry - pAddrMap->pMapEntries, pAddr6, pMapEntry->iVirtualAddress));
}

/*F*************************************************************************************/
/*!
    \Function _SocketAddrMapRemap

    \Description
        Alter the value of an initialized map entry

    \Input *pAddrMap    - address map
    \Input *pMapEntry   - entry to set
    \Input *pAddr6      - IPv6 address to set
    \Input iAddrSize    - size of address

    \Version 12/03/2015 (amakoukji)
*/
/************************************************************************************F*/
static void _SocketAddrMapRemap(const SocketAddrMapT *pAddrMap, SocketAddrMapEntryT *pMapEntry, const struct sockaddr_in6 *pAddr6, int32_t iAddrSize)
{
    if (memcmp(&pMapEntry->SockAddr6, pAddr6, iAddrSize) == 0)
    {
        NetPrintf(("dirtynet: [%d] attempt to remap %a to identical IPv6 address %A\n", pMapEntry - pAddrMap->pMapEntries, pMapEntry->iVirtualAddress, pAddr6));
    }
    NetPrintfVerbose((SocketInfo(NULL, 'spam', 0, NULL, 0), 1, "dirtynet: [%d] remapped %A -> %A for virtual address %a\n", pMapEntry - pAddrMap->pMapEntries, &pMapEntry->SockAddr6, pAddr6, pMapEntry->iVirtualAddress));
    ds_memcpy(&pMapEntry->SockAddr6, pAddr6, iAddrSize);
}

/*F*************************************************************************************/
/*!
    \Function _SocketAddrMapDel

    \Description
        Dereference (and clear, if no more references) a map entry

    \Input *pAddrMap    - address map
    \Input *pMapEntry   - entry to del

    \Version 04/17/2013 (jbrookes)
*/
/************************************************************************************F*/
static void _SocketAddrMapDel(SocketAddrMapT *pAddrMap, SocketAddrMapEntryT *pMapEntry)
{
    NetPrintfVerbose((SocketInfo(NULL, 'spam', 0, NULL, 0), 1, "dirtynet: [%d] del map %A to %a (decremented refcount to %d)\n",
        pMapEntry - pAddrMap->pMapEntries, &pMapEntry->SockAddr6, pMapEntry->iVirtualAddress, pMapEntry->iRefCount-1));
    if (--pMapEntry->iRefCount == 0)
    {
        ds_memclr(pMapEntry, sizeof(*pMapEntry));
    }
}

/*F*************************************************************************************/
/*!
    \Function _SocketAddrMapAdd

    \Description
        Add an IPv6 address to the address mapping table

    \Input *pAddrMap    - address map
    \Input *pAddr6      - address to add to the mapping table
    \Input iAddrSize    - size of address

    \Output
        int32_t         - SOCKMAP_ERROR=error, else virtual IPv4 address for newly mapped IPv6 address

    \Version 04/17/2013 (jbrookes)
*/
/************************************************************************************F*/
static int32_t _SocketAddrMapAdd(SocketAddrMapT *pAddrMap, const struct sockaddr_in6 *pAddr6, int32_t iAddrSize)
{
    int32_t iMapEntry;

    // find an empty slot
    for (iMapEntry = 0; iMapEntry < pAddrMap->iNumEntries; iMapEntry += 1)
    {
        if (pAddrMap->pMapEntries[iMapEntry].iVirtualAddress == 0)
        {
            _SocketAddrMapSet(pAddrMap, &pAddrMap->pMapEntries[iMapEntry], pAddr6, iAddrSize);
            return(pAddrMap->pMapEntries[iMapEntry].iVirtualAddress);
        }
    }

    // if no empty slot, realloc the array
    if ((_SocketAddrMapAlloc(pAddrMap, pAddrMap->iNumEntries+8, pAddrMap->iMemGroup, pAddrMap->pMemGroupUserData)) < 0)
    {
        return(SOCKMAP_ERROR);
    }
    // try the add again
    return(_SocketAddrMapAdd(pAddrMap, pAddr6, iAddrSize));
}

/*F*************************************************************************************/
/*!
    \Function SocketAddrMapInit

    \Description
        Initialize a Socket Address Map

    \Input *pAddrMap    - address map
    \Input iMemGroup    - memory group
    \Input *pMemGroupUserData - memory group user data

    \Version 03/03/2016 (jbrookes)
*/
/************************************************************************************F*/
void SocketAddrMapInit(SocketAddrMapT *pAddrMap, int32_t iMemGroup, void *pMemGroupUserData)
{
    // set up address map
    ds_memclr(pAddrMap, sizeof(*pAddrMap));
    pAddrMap->iMemGroup = iMemGroup;
    pAddrMap->pMemGroupUserData = pMemGroupUserData;
    // init ipv6 map virtual address
    pAddrMap->iNextVirtAddr = NetTick() & 0x00ffffff;
}

/*F*************************************************************************************/
/*!
    \Function SocketAddrMapShutdown

    \Description
        Clean up a Socket Address Map

    \Input *pAddrMap    - address map

    \Version 03/03/2016 (jbrookes)
*/
/************************************************************************************F*/
void SocketAddrMapShutdown(SocketAddrMapT *pAddrMap)
{
    if (pAddrMap->pMapEntries != NULL)
    {
        DirtyMemFree(pAddrMap->pMapEntries, SOCKET_MEMID, pAddrMap->iMemGroup, pAddrMap->pMemGroupUserData);
    }
}

/*F*************************************************************************************/
/*!
    \Function SocketAddrMapAddress

    \Description
        Map an IPv6 address and return a virtual IPv4 address that can be used to
        reference it.

    \Input *pAddrMap    - address map
    \Input *pAddr       - address to add to the mapping table
    \Input iAddrSize    - size of address

    \Output
        int32_t         - SOCKMAP_ERROR=error, else virtual IPv4 address for newly mapped IPv6 address

    \Version 04/17/2013 (jbrookes)
*/
/************************************************************************************F*/
int32_t SocketAddrMapAddress(SocketAddrMapT *pAddrMap, const struct sockaddr *pAddr, int32_t iAddrSize)
{
    const struct sockaddr_in6 *pAddr6 = (const struct sockaddr_in6 *)pAddr;
    SocketAddrMapEntryT *pMapEntry;

    // if IPv4, just return the IPv4 address directly, without mapping
    if (pAddr->sa_family == AF_INET)
    {
        return(SockaddrInGetAddr(pAddr));
    }
    // we only map ipv6 addresses
    if ((pAddr->sa_family != AF_INET6) || (iAddrSize < (signed)sizeof(struct sockaddr_in6)))
    {
        NetPrintf(("dirtynet: can't map address of type %d size=%d\n", pAddr->sa_family, iAddrSize));
        return(SOCKMAP_ERROR);
    }
    // force address size to in6 (is this necessary??)
    iAddrSize = sizeof(struct sockaddr_in6);
    // if it's an IPv4-mapped IPv6 address, return the IPv4 address
    if (_SockaddrIn6IsIPv4(pAddr6) || _SockaddrIn6IsZero(pAddr6))
    {
        return(SockaddrIn6GetAddr4(pAddr6));
    }
    // see if this address is already mapped
    if ((pMapEntry = _SocketAddrMapGet(pAddrMap, pAddr, iAddrSize)) != NULL)
    {
        pMapEntry->iRefCount += 1;
        NetPrintfVerbose((SocketInfo(NULL, 'spam', 0, NULL, 0), 1, "dirtynet: [%d] map %A to %a (incremented refcount to %d)\n", pMapEntry - pAddrMap->pMapEntries, &pMapEntry->SockAddr6, pMapEntry->iVirtualAddress, pMapEntry->iRefCount));
        return(pMapEntry->iVirtualAddress);
    }
    // add it to the map
    return(_SocketAddrMapAdd(pAddrMap, pAddr6, iAddrSize));
}

/*F*************************************************************************************/
/*!
    \Function SocketAddrRemapAddress

    \Description
        Remap an existing IPv6 address and return a virtual IPv4 address that can be used to
        reference it.

    \Input *pAddrMap    - address map
    \Input *pOldAddr    - address that should be in mapping table
    \Input *pNewAddr    - address to update to the mapping table
    \Input iAddrSize    - size of address

    \Output
        int32_t         - SOCKMAP_ERROR=error, else virtual IPv4 address for remapped IPv6 address

    \Version 12/03/2015 (amakoukji)
*/
/************************************************************************************F*/
int32_t SocketAddrRemapAddress(SocketAddrMapT *pAddrMap, const struct sockaddr *pOldAddr, const struct sockaddr *pNewAddr, int32_t iAddrSize)
{
    SocketAddrMapEntryT *pMapEntry;

    // if IPv4, just return the IPv4 address directly, without mapping
    if (pNewAddr->sa_family == AF_INET)
    {
        return(SockaddrInGetAddr(pNewAddr));
    }
    // we only map ipv6 addresses
    if ((pNewAddr->sa_family != AF_INET6) || (iAddrSize < (signed)sizeof(struct sockaddr_in6)))
    {
        NetPrintf(("dirtynet: can't remap address of type %d size=%d\n", pNewAddr->sa_family, iAddrSize));
        return(SOCKMAP_ERROR);
    }
    iAddrSize = sizeof(struct sockaddr_in6);
    // see if this address is already mapped
    if ((pMapEntry = _SocketAddrMapGet(pAddrMap, (const struct sockaddr *)pOldAddr, iAddrSize)) != NULL)
    {
        NetPrintf(("dirtynet: attempting to remap virtual address %a, from %A to %A\n", pMapEntry->iVirtualAddress, pOldAddr, pNewAddr));
        _SocketAddrMapRemap(pAddrMap, pMapEntry, (const struct sockaddr_in6 *)pNewAddr, iAddrSize);
        return(pMapEntry->iVirtualAddress);
    }

    // did not find the address
    NetPrintf(("dirtynet: attempt to remap unmapped address %A, attempting to add mapping instead\n", pNewAddr));
    return(SocketAddrMapAddress(pAddrMap, (const struct sockaddr *)pNewAddr, iAddrSize));
}

/*F*************************************************************************************/
/*!
    \Function SocketAddrUnmapAddress

    \Description
        Removes an address mapping from the mapping table.

    \Input *pAddrMap    - address map
    \Input *pAddr       - address to remove from the mapping table
    \Input iAddrSize    - size of address

    \Output
        int32_t         - negative=error, else zero

    \Version 04/18/2013 (jbrookes)
*/
/************************************************************************************F*/
int32_t SocketAddrUnmapAddress(SocketAddrMapT *pAddrMap, const struct sockaddr *pAddr, int32_t iAddrSize)
{
    SocketAddrMapEntryT *pMapEntry;

    // we only map ipv6 addresses
    if ((pAddr->sa_family != AF_INET6) || (iAddrSize < (signed)sizeof(struct sockaddr_in6)))
    {
        NetPrintf(("dirtynet: can't unmap address of type %d size=%d\n", pAddr->sa_family, iAddrSize));
        return(-1);
    }
    iAddrSize = sizeof(struct sockaddr_in6);
    // get the map entry for this address
    if ((pMapEntry = _SocketAddrMapGet(pAddrMap, (const struct sockaddr *)pAddr, iAddrSize)) == NULL)
    {
        NetPrintf(("dirtynet: address unmap operation on an address not in the table\n"));
        return(-2);
    }
    // unmap it
    _SocketAddrMapDel(pAddrMap, pMapEntry);
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function SocketAddrMapGet

    \Description
        Check if an address is in the address map, and if so return its mapped equivalent

    \Input *pAddrMap    - address map
    \Input *pResult     - [out] storage for result
    \Input *pSource     - source address
    \Input *pNameLen    - [out] storage for result length

    \Output
        struct sockaddr * - pointer to result address, or NULL if not found

    \Version 04/18/2013 (jbrookes)
*/
/************************************************************************************F*/
struct sockaddr *SocketAddrMapGet(const SocketAddrMapT *pAddrMap, struct sockaddr *pResult, const struct sockaddr *pSource, int32_t *pNameLen)
{
    SocketAddrMapEntryT *pMapEntry = _SocketAddrMapGet(pAddrMap, pSource, *pNameLen);
    struct sockaddr *pReturn = NULL;

    if (pMapEntry != NULL)
    {
        if (pResult->sa_family == AF_INET)
        {
            SockaddrInit(pResult, AF_INET);
            SockaddrInSetAddr(pResult, pMapEntry->iVirtualAddress);
            pReturn = pResult;
        }
        else if (pResult->sa_family == AF_INET6)
        {
            ds_memcpy_s(pResult, sizeof(*pResult), &pMapEntry->SockAddr6, sizeof(pMapEntry->SockAddr6));
            pReturn = pResult;
        }
    }

    return(pReturn);
}

/*F*************************************************************************************/
/*!
    \Function SocketAddrMapTranslate

    \Description
        Translate the following:
        - IPv4 real address to IPV4-mapped IPv6 address
        - IPv4 virtual address to IPv6 address
        - IPv6 address to IPv4 address

    \Input *pAddrMap    - address map
    \Input *pResult     - [out] storage for translated result
    \Input *pSource     - source address
    \Input *pNameLen    - [out] storage for result length

    \Output
        struct sockaddr * - pointer to resulting IPv6 address

    \Version 04/15/2013 (jbrookes)
*/
/************************************************************************************F*/
struct sockaddr *SocketAddrMapTranslate(const SocketAddrMapT *pAddrMap, struct sockaddr *pResult, const struct sockaddr *pSource, int32_t *pNameLen)
{
    SocketAddrMapEntryT *pMapEntry;
    struct sockaddr *pReturn = NULL;

    // handle broken use cases where the family is not specified
    if ((pSource->sa_family != AF_INET) && (pSource->sa_family != AF_INET6))
    {
        NetPrintf(("dirtynet: unsupported source family %d in SocketAddrMapTranslate(); assuming AF_INET\n", pSource->sa_family));
        ((struct sockaddr *)pSource)->sa_family = AF_INET;
    }
    if ((pResult->sa_family != AF_INET) && (pResult->sa_family != AF_INET6))
    {
        NetPrintf(("dirtynet: unsupported result family %d in SocketAddrMapTranslate(); assuming AF_INET\n", pResult->sa_family));
        pResult->sa_family = AF_INET;
    }

    // handle IPv4->IPv6
    if ((pSource->sa_family == AF_INET) && (pResult->sa_family == AF_INET6))
    {
        struct sockaddr_in6 *pResult6 = (struct sockaddr_in6 *)pResult;
        uint32_t uAddr = SockaddrInGetAddr(pSource);
        // handle a regular IPv4 address
        if ((uAddr == 0) || ((uAddr >> 24) != 0))
        {
            ds_memclr(pResult, sizeof(*pResult));
            _Sockaddr6SetV4Mapped(pResult6, pSource);
            *pNameLen = sizeof(*pResult6);
            pReturn = pResult;
        }
        // get IPv6 address from virtual IPv4
        else if ((pMapEntry = _SocketAddrMapGet(pAddrMap, pSource, *pNameLen)) != NULL)
        {
            ds_memcpy_s(pResult6, sizeof(*pResult6), &pMapEntry->SockAddr6, sizeof(pMapEntry->SockAddr6));
            pResult6->sin6_port = SocketNtohs(SockaddrInGetPort(pSource));
            *pNameLen = sizeof(*pResult6);
            pReturn = pResult;
        }
        else
        {
            NetPrintf(("dirtynet: could not find address for virtual address %a\n", SockaddrInGetAddr(pSource)));
        }
    }
    // handle IPv6->IPv4
    else if ((pSource->sa_family == AF_INET6) && (pResult->sa_family == AF_INET))
    {
        struct sockaddr_in6 *pSource6 = (struct sockaddr_in6 *)pSource;
        // translate IPv6 to virtual IPv4 address
        if ((pMapEntry = _SocketAddrMapGet(pAddrMap, pSource, *pNameLen)) != NULL)
        {
            struct sockaddr *pResult4 = (struct sockaddr *)pResult;
            SockaddrInit(pResult4, AF_INET);
            SockaddrInSetAddr(pResult4, pMapEntry->iVirtualAddress);
            SockaddrInSetPort(pResult4, SocketHtons(pSource6->sin6_port));
            *pNameLen = sizeof(*pResult4);
            pReturn = pResult;
        }
        // is this an IPv4-mapped IPv6 address?
        else if (_SockaddrIn6IsIPv4(pSource6))
        {
            struct sockaddr *pResult4 = (struct sockaddr *)pResult;
            uint32_t uAddress = SockaddrIn6GetAddr4(pSource6);
            SockaddrInit(pResult4, AF_INET);
            SockaddrInSetAddr(pResult4, uAddress);
            SockaddrInSetPort(pResult4, SocketHtons(pSource6->sin6_port));
            *pNameLen = sizeof(*pResult4);
            pReturn = pResult;
        }
    }
    else
    {
        *pNameLen = (pResult->sa_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
        ds_memcpy_s(pResult, *pNameLen, pSource, *pNameLen);
        pReturn = pResult;
    }
    if (pReturn != NULL)
    {
        NetPrintfVerbose((SocketInfo(NULL, 'spam', 0, NULL, 0), 2, "dirtynet: map translate %A->%A\n", pSource, pReturn));
    }
    else
    {
        NetPrintf(("dirtynet: map translate %A failed\n", pSource));
        pReturn = (struct sockaddr *)pSource;
        *pNameLen = sizeof(*pSource);
    }
    return(pReturn);
}
#endif
