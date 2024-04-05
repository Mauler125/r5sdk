/*H********************************************************************************/
/*!
    \File protowebsocket.c

    \Description
        This module implements a WebSocket client interface as documented by
        RFC 6455 (http://tools.ietf.org/html/rfc6455).  The current version of the
        WebSockets protocol implemented here is version 13.  Up to date registry
        information regarding new extensions, protocols, result codes, etc. can be
        found at http://www.iana.org/assignments/websocket/websocket.xml.

    \Notes
        While the standard supports up to a 64-bit frame size, this implementation
        only supports up to 32-bit frame sizes.

    \Todo
        - Sec-WebSocket-Protocols support

    \Copyright
        Copyright (c) 2012 Electronic Arts Inc.

    \Version 11/26/2012 (jbrookes) First Version
    \Version 03/31/2017 (jbrookes) Added frame send/recv capability
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/crypt/cryptrand.h"
#include "DirtySDK/crypt/cryptsha1.h"
#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/util/base64.h"

#include "DirtySDK/proto/protowebsocket.h"

/*** Defines **********************************************************************/

//! ProtoWebSocket revision number (maj.min)
#define PROTOWEBSOCKET_VERSION      (0x0100)          // update this for major bug fixes or protocol additions/changes

//! default ProtoWebSocket timeout
#define PROTOWEBSOCKET_TIMEOUT      (5*30*1000)

//! define WebSocket protocol version
#define PROTOWEBSOCKET_PROTOCOLVERS (13)

//! default protowebsocket buffer size
#define PROTOWEBSOCKET_BUFSIZE_MIN  (4*1024)

// opcode definitions
#define PROTOWEBSOCKET_OPCODE_BASE  (0x0)
#define PROTOWEBSOCKET_OPCODE_CONT  (PROTOWEBSOCKET_OPCODE_BASE+0)
#define PROTOWEBSOCKET_OPCODE_TEXT  (PROTOWEBSOCKET_OPCODE_BASE+1)
#define PROTOWEBSOCKET_OPCODE_BINA  (PROTOWEBSOCKET_OPCODE_BASE+2)
// control frame type definitions
#define PROTOWEBSOCKET_CTRL_BASE    (0x8)
#define PROTOWEBSOCKET_CTRL_CLSE    (PROTOWEBSOCKET_CTRL_BASE+0)
#define PROTOWEBSOCKET_CTRL_PING    (PROTOWEBSOCKET_CTRL_BASE+1)
#define PROTOWEBSOCKET_CTRL_PONG    (PROTOWEBSOCKET_CTRL_BASE+2)
// fragment end
#define PROTOWEBSOCKET_OPCODE_FIN   (0x80)

// max WebSocket protocol send header size
#define PROTOWEBSOCKET_MAXSENDHDR   (12)

//! default size for our temporary input buffer
#define PROTOWEBSOCKET_DEFAULT_INPUTBUFSIZE (256)

/*** Type Definitions *************************************************************/

// state for a websocket connection
struct ProtoWebSocketRefT
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    ProtoSSLRefT *pProtoSSL;

    enum
    {
        ST_DISC,            //!< disconnected
        ST_CONN,            //!< connecting
        ST_SEND,            //!< sending handshake
        ST_RECV,            //!< receiving handshake
        ST_OPEN,            //!< connected
        ST_FAIL             //!< connection failed
    } eState;               //!< current state

    int32_t iVersion;       //!< protocol version

    char *pOutBuf;          //!< output buffer
    int32_t iOutMax;        //!< max amount of data that can be stored in the buffer
    int32_t iOutLen;        //!< current amount of data in buffer
    int32_t iOutOff;        //!< current offset within buffer
    int32_t iRecvRslt;
    int32_t iRecvSize;      //!< used by RecvFrame to track total received size

    char *pInpBuf;          //!< temporary input buffer used for connection establishment
    int32_t iInpMax;        //!< max amount of data that can be stored in the buffer
    int32_t iInpLen;        //!< current amount of data in buffer
    int32_t iInpOff;        //!< current offset within buffer

    int32_t iFrameHdrOff;
    int32_t iFrameHdrLen;
    int32_t iFrameLen;
    int32_t iFrameType;

    int32_t iCtrlDataLen;   //!< length of control packet data
    int32_t iCtrlDataOff;   //!< offset while receiving control packet data

    int32_t iVerbose;
    uint32_t uTimer;
    int32_t iTimeout;
    int32_t iKeepAlive;     //!< if >0, number of seconds before issuing a keep-alive pong
    uint32_t uKeepTimer;
    int32_t iSSLFail;
    int32_t iCloseReason;

    char *pAppendHdr;       //!< append header buffer pointer
    int32_t iAppendLen;     //!< size of append header buffer

    int32_t iPort;          //!< current connection port

    uint8_t bTimeout;
    uint8_t bDoPong;        //!< execute a 'pong' frame at earliest opportunity
    uint8_t bDoClose;       //!< execute a 'close' frame at earliest opportunity; disconnect
    uint8_t bClosing;       //!< have sent a close frame
    uint8_t bRecvCtrlData;  //!< currently receiving control data
    uint8_t bMessageSend;   //!< if true, we're sending a message (FIN bit not sent until message is complete)
    uint8_t bFrameFrag;     //!< if true, we're sending a fragmented frame
    uint8_t bFrameFin;      //!< if true this packet is a fin packet

    char strHost[256];      //!< server name
    char strFrameHdr[PROTOWEBSOCKET_MAXSENDHDR]; //!< cache to receive frame header into

    char strRawKey[32];     //!< generated key sent to server; must hold 16 characters base64 encoded
    char strEncKey[32];     //!< encoded key we expect to get back from server; must hold 16 characters base64 encoded

    char strCtrlData[256];  //!< cached control packet data
};

/*** Variables ********************************************************************/

#if DIRTYCODE_LOGGING
static const char *_ProtoWebSocket_strOpcodeNames[16] =
{
    "cont",
    "text",
    "bina",
    "", "", "", "", "",
    "clse",
    "ping",
    "pong",
    "", "", "", "", ""
};
static const char *_ProtoWebSocket_strCloseReasons[PROTOWEBSOCKET_CLOSEREASON_COUNT] =
{
    "normal",
    "going away",
    "protocol error",
    "unsupported data",
    "reserved",
    "no status received",
    "abnormal closure",
    "invalid frame data",
    "policy violation",
    "message too big",
    "mandatory extension",
    "internal error",
    "service restart",
    "try again later",
    "reserved",
    "tls handshake",
};
#endif

/*** Function Prototypes **********************************************************/

static int32_t _ProtoWebSocketRecvData(ProtoWebSocketRefT *pWebSocket, char *pStrBuf, int32_t iSize);

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketSetAppendHeader

    \Description
        Set given string as append header, allocating memory as required.

    \Input *pWebSocket  - module state
    \Input *pAppendHdr  - append header string

    \Output
        int32_t         - zero=success, else error

    \Version 12/04/2012 (jbrookes) Copied from ProtoHttp
*/
/********************************************************************************F*/
static int32_t _ProtoWebSocketSetAppendHeader(ProtoWebSocketRefT *pWebSocket, const char *pAppendHdr)
{
    int32_t iAppendBufLen, iAppendStrLen;

    // check for empty append string, in which case we free the buffer
    if ((pAppendHdr == NULL) || (*pAppendHdr == '\0'))
    {
        if (pWebSocket->pAppendHdr != NULL)
        {
            DirtyMemFree(pWebSocket->pAppendHdr, PROTOHTTP_MEMID, pWebSocket->iMemGroup, pWebSocket->pMemGroupUserData);
            pWebSocket->pAppendHdr = NULL;
        }
        pWebSocket->iAppendLen = 0;
        return(0);
    }

    // check to see if append header is already set
    if ((pWebSocket->pAppendHdr != NULL) && (!strcmp(pAppendHdr, pWebSocket->pAppendHdr)))
    {
        NetPrintfVerbose((pWebSocket->iVerbose, 1, "protowebsocket: [%p] ignoring set of append header '%s' that is already set\n", pWebSocket, pAppendHdr));
        return(0);
    }

    // get append header length
    iAppendStrLen = (int32_t)strlen(pAppendHdr);
    // append buffer size includes null and space for \r\n if not included by submitter
    iAppendBufLen = iAppendStrLen + 3;

    // see if we need to allocate a new buffer
    if (iAppendBufLen > pWebSocket->iAppendLen)
    {
        if (pWebSocket->pAppendHdr != NULL)
        {
            DirtyMemFree(pWebSocket->pAppendHdr, PROTOHTTP_MEMID, pWebSocket->iMemGroup, pWebSocket->pMemGroupUserData);
        }
        if ((pWebSocket->pAppendHdr = DirtyMemAlloc(iAppendBufLen, PROTOHTTP_MEMID, pWebSocket->iMemGroup, pWebSocket->pMemGroupUserData)) != NULL)
        {
            pWebSocket->iAppendLen = iAppendBufLen;
        }
        else
        {
            NetPrintf(("protowebsocket: [%p] could not allocate %d byte buffer for append header\n", pWebSocket, iAppendBufLen));
            pWebSocket->iAppendLen = 0;
            return(-1);
        }
    }

    // copy append header
    ds_strnzcpy(pWebSocket->pAppendHdr, pAppendHdr, iAppendStrLen+1);

    // if append header is not \r\n terminated, do it here
    if (pAppendHdr[iAppendStrLen-2] != '\r' || pAppendHdr[iAppendStrLen-1] != '\n')
    {
        ds_strnzcat(pWebSocket->pAppendHdr, "\r\n", pWebSocket->iAppendLen);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketGenerateKey

    \Description
        Generate random base64-encoded key to be sent to server in initial
        handshake, and base64-encoded SHA1 hash of the key that we expect
        from the server in response.

    \Input *pWebSocket  - module state

    \Version 11/27/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoWebSocketGenerateKey(ProtoWebSocketRefT *pWebSocket)
{
    char strAccept[128];
    uint8_t aSha1Data[CRYPTSHA1_HASHSIZE];
    CryptSha1T Sha1State;
    uint8_t aRandom[16];

    // get 16-digit random number
    CryptRandGet(aRandom, sizeof(aRandom));

    // base64 encode string
    Base64Encode2((char *)aRandom, sizeof(aRandom), pWebSocket->strRawKey, sizeof(pWebSocket->strRawKey));

    // now, generate accept header we must receive to validate response

    // concatentate key with hardcoded GUID as per spec
    ds_strnzcpy(strAccept, pWebSocket->strRawKey, sizeof(strAccept));
    ds_strnzcat(strAccept, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", sizeof(strAccept));

    // hash the key with SHA1
    CryptSha1Init(&Sha1State);
    CryptSha1Update(&Sha1State, (uint8_t *)strAccept, (uint32_t)strlen(strAccept));
    CryptSha1Final(&Sha1State, aSha1Data, CRYPTSHA1_HASHSIZE);

    // base64 encode for final accept key
    Base64Encode2((char *)aSha1Data, CRYPTSHA1_HASHSIZE, pWebSocket->strEncKey, sizeof(pWebSocket->strEncKey));

    NetPrintfVerbose((pWebSocket->iVerbose, 2, "protowebsocket: [%p] raw key=%s\n", pWebSocket, pWebSocket->strRawKey));
    NetPrintfVerbose((pWebSocket->iVerbose, 2, "protowebsocket: [%p] enc key=%s\n", pWebSocket, pWebSocket->strEncKey));
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketFormatHeader

    \Description
        Format websocket handshake header

    \Input *pWebSocket  - module state
    \Input *pHost       - websocket host
    \Input *pUrl        - websocket url
    \Input iPort        - protocol port
    \Input iSecure      - true=secure, else false

    \Output
        int32_t         - positive=success, negative=failure

    \Version 11/27/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoWebSocketFormatHeader(ProtoWebSocketRefT *pWebSocket, const char *pHost, const char *pUrl, int32_t iPort, int32_t iSecure)
{
    int32_t iBufSize = pWebSocket->iOutMax, iBufLen = 0;

    // if no url specified use the minimum
    if (*pUrl == '\0')
    {
        pUrl = "/";
    }

    iBufLen += ds_snzprintf(pWebSocket->pOutBuf+iBufLen, iBufSize-iBufLen, "GET %s HTTP/1.1\r\n", pUrl);
    if ((iSecure && (iPort == 443)) || (iPort == 80))
    {
        iBufLen += ds_snzprintf(pWebSocket->pOutBuf+iBufLen, iBufSize-iBufLen, "Host: %s\r\n", pHost);
    }
    else
    {
        iBufLen += ds_snzprintf(pWebSocket->pOutBuf+iBufLen, iBufSize-iBufLen, "Host: %s:%d\r\n", pHost, iPort);
    }
    iBufLen += ds_snzprintf(pWebSocket->pOutBuf+iBufLen, iBufSize-iBufLen, "Upgrade: websocket\r\n");
    iBufLen += ds_snzprintf(pWebSocket->pOutBuf+iBufLen, iBufSize-iBufLen, "Connection: upgrade\r\n");
    iBufLen += ds_snzprintf(pWebSocket->pOutBuf+iBufLen, iBufSize-iBufLen, "Sec-WebSocket-Key: %s\r\n", pWebSocket->strRawKey);
    iBufLen += ds_snzprintf(pWebSocket->pOutBuf+iBufLen, iBufSize-iBufLen, "Sec-WebSocket-Version: %d\r\n", pWebSocket->iVersion);
    if ((pWebSocket->pAppendHdr == NULL) || !ds_stristr(pWebSocket->pAppendHdr, "User-Agent:"))
    {
        iBufLen += ds_snzprintf(pWebSocket->pOutBuf+iBufLen, iBufSize-iBufLen, "User-Agent: ProtoWebSocket %d.%d/DS %d.%d.%d.%d.%d (" DIRTYCODE_PLATNAME ")\r\n",
            (PROTOWEBSOCKET_VERSION>>8)&0xff, PROTOWEBSOCKET_VERSION&0xff, DIRTYSDK_VERSION_YEAR, DIRTYSDK_VERSION_SEASON,
            DIRTYSDK_VERSION_MAJOR, DIRTYSDK_VERSION_MINOR, DIRTYSDK_VERSION_PATCH);
    }
    if ((pWebSocket->pAppendHdr == NULL) || (pWebSocket->pAppendHdr[0] == '\0'))
    {
        iBufLen += ds_snzprintf(pWebSocket->pOutBuf+iBufLen, iBufSize-iBufLen, "Accept: */*\r\n");
    }
    else
    {
        iBufLen += ds_snzprintf(pWebSocket->pOutBuf+iBufLen, iBufSize-iBufLen, "%s", pWebSocket->pAppendHdr);
    }
    iBufLen += ds_snzprintf(pWebSocket->pOutBuf+iBufLen, iBufSize-iBufLen, "\r\n");
    pWebSocket->iOutLen = iBufLen;
    pWebSocket->iOutOff = 0;
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketDisconnect

    \Description
        Disconnect from server

    \Input *pWebSocket  - module state
    \Input iNewState    - new state to set (fail or disc)

    \Version 11/30/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoWebSocketDisconnect(ProtoWebSocketRefT *pWebSocket, int32_t iNewState)
{
    NetPrintfVerbose((pWebSocket->iVerbose, 0, "protowebsocket: [%p] disconnecting\n", pWebSocket));
    pWebSocket->eState = iNewState;
    if (iNewState == ST_FAIL)
    {
        pWebSocket->iSSLFail = ProtoSSLStat(pWebSocket->pProtoSSL, 'fail', NULL, 0);
    }
    return(ProtoSSLDisconnect(pWebSocket->pProtoSSL));
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketReset

    \Description
        Reset module state

    \Input *pWebSocket  - module state

    \Version 11/30/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoWebSocketReset(ProtoWebSocketRefT *pWebSocket)
{
    // if we're not already disconnected, disconnect now
    if (pWebSocket->eState != ST_DISC && pWebSocket->eState != ST_FAIL)
    {
        _ProtoWebSocketDisconnect(pWebSocket, ST_DISC);
    }

    // reset state
    pWebSocket->iOutLen = 0;
    pWebSocket->iOutOff = 0;
    pWebSocket->iInpLen = 0;
    pWebSocket->iInpOff = 0;
    pWebSocket->iRecvRslt = 0;
    pWebSocket->iRecvSize = 0;
    pWebSocket->iFrameHdrOff = 0;
    pWebSocket->iFrameHdrLen = 2; // minimal header size
    pWebSocket->iFrameLen = 0;
    pWebSocket->iFrameType = 0;
    pWebSocket->iCtrlDataLen = 0;
    pWebSocket->iSSLFail = 0;
    pWebSocket->bTimeout = FALSE;
    pWebSocket->bDoPong = FALSE;
    pWebSocket->bDoClose = FALSE;
    pWebSocket->bClosing = FALSE;
    pWebSocket->bFrameFrag = FALSE;
    pWebSocket->bFrameFin = FALSE;
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketFail

    \Description
        Handle a socket failure

    \Input *pWebSocket  - module state
    \Input iResult      - result from a socket operation
    \Input *pStrDebug   - debug text indicating failure context

    \Version 11/27/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoWebSocketFail(ProtoWebSocketRefT *pWebSocket, int32_t iResult, const char *pStrDebug)
{
    if (pWebSocket->eState == ST_FAIL)
    {
        return;
    }
    NetPrintf(("protowebsocket: [%p] failure: %s (err=%d)\n", pWebSocket, pStrDebug, iResult));
    _ProtoWebSocketDisconnect(pWebSocket, ST_FAIL);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketValidateHeader

    \Description
        Validate server response header

    \Input *pWebSocket  - module state
    \Input *pBuffer     - buffer holding received header
    \Input *pHeader     - name of header to validate
    \Input *pValue      - header value to validate

    \Output
        int32_t         - zero=success, negative=failure

    \Version 11/27/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoWebSocketValidateHeader(ProtoWebSocketRefT *pWebSocket, const char *pBuffer, const char *pHeader, const char *pValue)
{
    char strDebug[256];
    char strHdrVal[128];

    if (ProtoHttpGetHeaderValue(NULL, pBuffer, pHeader, strHdrVal, sizeof(strHdrVal), NULL) < 0)
    {
        ds_snzprintf(strDebug, sizeof(strDebug), "missing %s header", pHeader);
        _ProtoWebSocketFail(pWebSocket, -1, strDebug);
        return(-1);
    }
    else if (ds_stricmp(strHdrVal, pValue))
    {
        ds_snzprintf(strDebug, sizeof(strDebug), "invalid %s header value '%s'", pHeader, pValue);
        _ProtoWebSocketFail(pWebSocket, -1, strDebug);
        return(-1);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketProcessHeader

    \Description
        Process websocket handshake server response

    \Input *pWebSocket  - module state
    \Input *pBuffer     - buffer holding received header

    \Output
        int32_t         - positive=success, negative=failure

    \Version 11/27/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoWebSocketProcessHeader(ProtoWebSocketRefT *pWebSocket, const char *pBuffer)
{
    int32_t iHdrCode;

    NetPrintfVerbose((pWebSocket->iVerbose, 1, "protowebsocket: [%p] handshake response received\n", pWebSocket));
    #if DIRTYCODE_LOGGING
    if (pWebSocket->iVerbose > 2)
    {
        NetPrintWrap(pBuffer, 80);
    }
    #endif

    /* as per https://www.w3.org/TR/websockets/ 4.8: When the user agent validates the server's response during
       the "establish a WebSocket connection" algorithm, if the status code received from the server is not 101
       (e.g. it is a redirect), the user agent must fail the WebSocket connection. */

    // validate 101 response
    iHdrCode = ProtoHttpParseHeaderCode(pBuffer);
    if (iHdrCode != 101)
    {
        _ProtoWebSocketFail(pWebSocket, iHdrCode, "invalid http response code");
        return(-1);
    }
    // validate "upgrade: websocket" header
    if (_ProtoWebSocketValidateHeader(pWebSocket, pBuffer, "upgrade", "websocket") < 0)
    {
        return(-2);
    }
    // validate "connection: upgrade" header
    if (_ProtoWebSocketValidateHeader(pWebSocket, pBuffer, "connection", "upgrade") < 0)
    {
        return(-3);
    }
    // validate accept header
    if (_ProtoWebSocketValidateHeader(pWebSocket, pBuffer, "sec-websocket-accept", pWebSocket->strEncKey) < 0)
    {
        return(-4);
    }

    //$$TODO - support Sec-WebSocket-Extensions?
    //$$TODO - support Sec-WebSocket-Protocol?

    pWebSocket->eState = ST_OPEN;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketSend

    \Description
        Try and send some data.  If data is sent, the timout value is updated.

    \Input *pWebSocket  - module state
    \Input *pStrBuf     - pointer to buffer to send from
    \Input iSize        - amount of data to try and send

    \Output
        int32_t         - negative=error, else number of bytes sent

    \Version 11/27/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoWebSocketSend(ProtoWebSocketRefT *pWebSocket, const char *pStrBuf, int32_t iSize)
{
    int32_t iResult;

    // try and send some data
    if ((iResult = ProtoSSLSend(pWebSocket->pProtoSSL, pStrBuf, iSize)) > 0)
    {
        NetPrintfVerbose((pWebSocket->iVerbose, 1, "protowebsocket: [%p] sent %d bytes\n", pWebSocket, iResult));
        #if DIRTYCODE_LOGGING
        if (pWebSocket->iVerbose > 3)
        {
            NetPrintMem(pStrBuf, iResult, "ws-send");
        }
        #endif

        // sent data, so update timer
        pWebSocket->uTimer = NetTick();
    }
    else if (iResult < 0)
    {
        NetPrintf(("protowebsocket: [%p] error %d sending %d bytes\n", pWebSocket, iResult, iSize));
        pWebSocket->eState = ST_FAIL;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketSendBuff

    \Description
        Send buffered data

    \Input *pWebSocket  - module state

    \Output
        int32_t         - zero=in progress, positive=done, negative=failure

    \Version 11/29/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoWebSocketSendBuff(ProtoWebSocketRefT *pWebSocket)
{
    int32_t iResult;
    if ((iResult = _ProtoWebSocketSend(pWebSocket, pWebSocket->pOutBuf+pWebSocket->iOutOff, pWebSocket->iOutLen-pWebSocket->iOutOff)) > 0)
    {
        pWebSocket->iOutOff += iResult;
        if (pWebSocket->iOutOff == pWebSocket->iOutLen)
        {
            iResult = pWebSocket->iOutLen;
            pWebSocket->iOutLen = pWebSocket->iOutOff = 0;
        }
        else
        {
            iResult = 0;
        }
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketSetLength

    \Description
        Endode message length in the format websocket protocol reqires

    \Input *pWebSocket  - module state
    \Input *pBuffer     - buffer to encode to
    \Input iMask        - true if data is masked, else false
    \Input iLength      - length to set

    \Output
        uint8_t *       - pointer past end of encoded length

    \Notes
        While the standard supports up to a 64-bit frame size, this implementation
        only supports up to 32-bit frame sizes.

    \Version 11/29/2012 (jbrookes)
*/
/********************************************************************************F*/
static uint8_t *_ProtoWebSocketSetLength(ProtoWebSocketRefT *pWebSocket, uint8_t *pBuffer, int32_t iMask, int32_t iLength)
{
    iMask = iMask ? 0x80 : 0;
    if (iLength < 126)
    {
        *pBuffer++ = (uint8_t)iLength | (uint8_t)iMask;
    }
    else if (iLength < 65536)
    {
        *pBuffer++ = (uint8_t)126 | (uint8_t)iMask;
        *pBuffer++ = (uint8_t)(iLength >> 8);
        *pBuffer++ = (uint8_t)(iLength >> 0);
    }
    else
    {
        *pBuffer++ = (uint8_t)127 | (uint8_t)iMask;
        *pBuffer++ = 0;
        *pBuffer++ = 0;
        *pBuffer++ = 0;
        *pBuffer++ = 0;
        *pBuffer++ = (uint8_t)(iLength >> 24);
        *pBuffer++ = (uint8_t)(iLength >> 16);
        *pBuffer++ = (uint8_t)(iLength >> 8);
        *pBuffer++ = (uint8_t)(iLength >> 0);
    }
    return(pBuffer);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketSetMask

    \Description
        Generate and encode mask; required for sending data from client to server

    \Input *pWebSocket  - module state
    \Input *pBuffer     - [out] buffer to write mask to
    \Input **ppMask     - [out] storage for pointer to mask

    \Output
        uint8_t *       - pointer past end of encoded mask

    \Version 11/29/2012 (jbrookes)
*/
/********************************************************************************F*/
static uint8_t *_ProtoWebSocketSetMask(ProtoWebSocketRefT *pWebSocket, uint8_t *pBuffer, uint8_t **ppMask)
{
    // generate four-byte random mask
    CryptRandGet(pBuffer, 4);
    // save pointer to mask for caller
    *ppMask = pBuffer;
    // print mask for extended diagnostics
    NetPrintfVerbose((pWebSocket->iVerbose, 2, "protowebsocket: [%p] mask=0x%02x%02x%02x%02x\n", pWebSocket,
        pBuffer[0], pBuffer[1], pBuffer[2], pBuffer[3]));
    return(pBuffer+4);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketSendData

    \Description
        Send masked data from client to server.

    \Input *pWebSocket  - module state
    \Input *pBuffer     - data to send
    \Input iLength      - length of data to send
    \Input uOpcode      - frame type

    \Output
        int32_t         - number of data bytes sent

    \Version 11/29/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoWebSocketSendData(ProtoWebSocketRefT *pWebSocket, const char *pBuffer, int32_t iLength, uint8_t uOpcode)
{
    uint8_t *pPacket = (uint8_t *)pWebSocket->pOutBuf, *pMask;
    int32_t iBufIdx, iOutMax, iResult;
    uint8_t uFrameFin = PROTOWEBSOCKET_OPCODE_FIN;

    // only allow if there's not a send in progress
    if (pWebSocket->iOutLen != 0)
    {
        return(0);
    }

    // opcode is only set for the first packet of a fragmented message
    if (pWebSocket->bMessageSend && pWebSocket->bFrameFrag)
    {
        uOpcode = 0;
    }

    // calculate available space, while reserving max header size
    iOutMax = pWebSocket->iOutMax - PROTOWEBSOCKET_MAXSENDHDR;
    if (iLength > iOutMax)
    {
        // for message sending, clear FIN bit when it isn't the final frame
        if (pWebSocket->bMessageSend)
        {
            uFrameFin = 0;
            pWebSocket->bFrameFrag = TRUE;
        }
        // constrain send length to max available space
        iLength = iOutMax;
    }
    else if (pWebSocket->bMessageSend)
    {
        // reset fragmentation flag on final frame of message
        pWebSocket->bFrameFrag = FALSE;
    }

    // set packet opcode
    *pPacket++ = uOpcode|uFrameFin;
    // set packet length
    pPacket = _ProtoWebSocketSetLength(pWebSocket, pPacket, 1, iLength);
    // set packet mask
    pPacket = _ProtoWebSocketSetMask(pWebSocket, pPacket, &pMask);

    // mask the data
    for (iBufIdx = 0; iBufIdx < iLength; iBufIdx += 1)
    {
        *pPacket++ = *pBuffer++ ^ pMask[iBufIdx&0x3];
    }

    // set buffer parms
    pWebSocket->iOutOff = 0;
    pWebSocket->iOutLen = (int32_t)(pPacket-(uint8_t *)pWebSocket->pOutBuf);

    // try to send
    if ((iResult = _ProtoWebSocketSendBuff(pWebSocket)) < 0)
    {
        return(iResult);
    }

    #if DIRTYCODE_LOGGING
    if (iResult > 0)
    {
        NetPrintfVerbose((pWebSocket->iVerbose, 1, "protowebsocket: [%p] sent frame fin=%d typ=%s len=%d\n", pWebSocket, ((uint8_t *)pWebSocket->pOutBuf)[0]>>7,
            _ProtoWebSocket_strOpcodeNames[((uint8_t *)pWebSocket->pOutBuf)[0]&0xf], iResult));
    }
    #endif


    // return amount of data buffered
    return(iLength);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketSendUserData

    \Description
        Send masked data from client to server making, wrapping the state error
        checking

    \Input *pWebSocket  - module state
    \Input *pBuffer     - data to send
    \Input iLength      - length of data to send
    \Input uOpcode      - frame type

    \Output
        int32_t         - number of data bytes sent

    \Version 08/30/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoWebSocketSendUserData(ProtoWebSocketRefT *pWebSocket, const char *pBuffer, int32_t iLength, uint8_t uOpcode)
{
    // handle not connected states
    if ((pWebSocket->eState == ST_DISC) || (pWebSocket->eState == ST_FAIL))
    {
        return(SOCKERR_NOTCONN);
    }
    if (pWebSocket->eState != ST_OPEN)
    {
        return(0);
    }
    return(_ProtoWebSocketSendData(pWebSocket, pBuffer, iLength, uOpcode));
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketSendClose

    \Description
        Send a close notification to server (or queue it if we are in the middle
        of another send).

    \Input *pWebSocket  - module state
    \Input iReason      - (optional) close reason (PROTOWEBSOCKET_CLOSEREASON_*)
    \Input *pStrReason  - (optional) string close reason

    \Output
        int32_t         - number of bytes data sent

    \Version 11/30/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoWebSocketSendClose(ProtoWebSocketRefT *pWebSocket, int32_t iReason, const char *pStrReason)
{
    int32_t iDataLen, iResult;

    // if we've already sent a close frame, we don't want to send another
    if (pWebSocket->bClosing)
    {
        return(0);
    }
    pWebSocket->bClosing = TRUE;

    // format reason data
    if (iReason != 0)
    {
        iDataLen = 2;
        pWebSocket->strCtrlData[0] = (iReason>>8)&0xff;
        pWebSocket->strCtrlData[1] = iReason&0xff;
        if (pStrReason != NULL)
        {
            ds_strnzcpy(pWebSocket->strCtrlData+2, pStrReason, sizeof(pWebSocket->strCtrlData)-2);
            iDataLen += (int32_t)strlen(pWebSocket->strCtrlData+2);
        }
    }
    else
    {
        pWebSocket->strCtrlData[0] = '\0';
        iDataLen = 0;
    }

    // try immediate send
    NetPrintfVerbose((pWebSocket->iVerbose, 0, "protowebsocket: [%p] closing connection (code=%d, text=%s)\n", pWebSocket, iReason, pStrReason));
    if ((iResult = _ProtoWebSocketSendData(pWebSocket, pWebSocket->strCtrlData, iDataLen, PROTOWEBSOCKET_CTRL_CLSE)) == 0)
    {
        // couldn't buffer for sending, so set up to try again
        pWebSocket->bDoClose = TRUE;
        pWebSocket->iCtrlDataLen = iDataLen;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketSendPing

    \Description
        Send a ping notification to server -- unlike close, we don't queue for
        later sending if we can't send immediately.

    \Input *pWebSocket  - module state
    \Input *pStrData    - (optional) string ping data
    \Input iType        - PING or PONG

    \Output
        int32_t         - number of bytes data sent

    \Version 11/30/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoWebSocketSendPing(ProtoWebSocketRefT *pWebSocket, const char *pStrData, int32_t iType)
{
    int32_t iDataLen = 0;
    if (pStrData != NULL)
    {
        iDataLen = (int32_t)strlen(pStrData);
    }
    return(_ProtoWebSocketSendData(pWebSocket, pStrData, iDataLen, iType));
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketRecv

    \Description
        Try and receive some data.  If data is received, the timout value is
        updated.

    \Input *pWebSocket  - module state
    \Input *pStrBuf     - [out] pointer to buffer to receive into
    \Input iSize        - amount of data to try and receive

    \Output
        int32_t         - negative=error, else success

    \Version 11/27/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoWebSocketRecv(ProtoWebSocketRefT *pWebSocket, char *pStrBuf, int32_t iSize)
{
    // if we have no buffer space, don't try to receive
    if (iSize == 0)
    {
        return(0);
    }

    /* if we are receiving when in the RECV state or if we have no data already waiting
       pull from the TCP buffer. when we are in the RECV state we need to pull into the temp
       input buffer, hence why we need the state check */
    if ((pWebSocket->eState == ST_RECV) || (pWebSocket->iInpLen == 0))
    {
        pWebSocket->iRecvRslt = ProtoSSLRecv(pWebSocket->pProtoSSL, pStrBuf, iSize);
    }
    // otherwise pull from the temp input buffer
    else
    {
        iSize = DS_MIN(iSize, pWebSocket->iInpLen-pWebSocket->iInpOff);
        ds_memcpy(pStrBuf, pWebSocket->pInpBuf+pWebSocket->iInpOff, iSize);
        pWebSocket->iInpOff += iSize;

        // if we read the entire buffer, reset the variables
        if (pWebSocket->iInpLen == pWebSocket->iInpOff)
        {
            pWebSocket->iInpLen = pWebSocket->iInpOff = 0;
        }
        pWebSocket->iRecvRslt = iSize;
    }

    // handle the result
    if (pWebSocket->iRecvRslt > 0)
    {
        NetPrintfVerbose((pWebSocket->iVerbose, 1, "protowebsocket: [%p] recv %d bytes\n", pWebSocket, pWebSocket->iRecvRslt));
        #if DIRTYCODE_LOGGING
        if (pWebSocket->iVerbose > 3)
        {
            NetPrintMem(pStrBuf, pWebSocket->iRecvRslt, "ws-recv");
        }
        #endif

        // received data, so update timer
        pWebSocket->uTimer = NetTick();
    }
    return(pWebSocket->iRecvRslt);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketCheckCtrl

    \Description
        Check a control frame received from the server

    \Input *pWebSocket  - module state
    \Input *pStrBuf     - pointer to buffer containing received data from server
    \Input iSize        - amount of data to received from the server

    \Version 11/29/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoWebSocketCheckCtrl(ProtoWebSocketRefT *pWebSocket, char *pStrBuf, int32_t iSize)
{
    if (pWebSocket->iFrameType == PROTOWEBSOCKET_CTRL_PING)
    {
        NetPrintfVerbose((pWebSocket->iVerbose, 1, "protowebsocket: [%p] got a ping with %d bytes of data\n", pWebSocket, iSize));
        // set to respond with a pong
        pWebSocket->bDoPong = TRUE;
    }
    else if (pWebSocket->iFrameType == PROTOWEBSOCKET_CTRL_PONG)
    {
        NetPrintfVerbose((pWebSocket->iVerbose, 1, "protowebsocket: [%p] got a pong with %d bytes of data\n", pWebSocket, iSize));
    }
    else if (pWebSocket->iFrameType == PROTOWEBSOCKET_CTRL_CLSE)
    {
        int32_t iCloseReason = ((uint8_t)pStrBuf[0]) << 8 | (uint8_t)pStrBuf[1];
        if ((iCloseReason >= PROTOWEBSOCKET_CLOSEREASON_BASE) && (iCloseReason <= PROTOWEBSOCKET_CLOSEREASON_MAX))
        {
            NetPrintfVerbose((pWebSocket->iVerbose, 0, "protowebsocket: [%p] got a close with reason '%s' (len=%d)\n", pWebSocket,
                _ProtoWebSocket_strCloseReasons[iCloseReason-PROTOWEBSOCKET_CLOSEREASON_BASE], iSize));
            pWebSocket->iCloseReason = iCloseReason;
        }
        else
        {
            NetPrintfVerbose((pWebSocket->iVerbose, 0, "protowebsocket: [%p] got a close with unknown reason %d (len=%d)\n", pWebSocket,
                iCloseReason, iSize));
            pWebSocket->iCloseReason = PROTOWEBSOCKET_CLOSEREASON_UNKNOWN;
        }
        // disconnect
        _ProtoWebSocketDisconnect(pWebSocket, ST_DISC);
    }
    else
    {
        NetPrintf(("protowebsocket: [%p] ignoring unknown control type %d (len=%d)\n", pWebSocket, pWebSocket->iFrameType, iSize));
        // ignore unknown frame type data?
    }

    // done with control data, reset frame header info; this is only necessary for control messages with no associated data
    pWebSocket->iFrameHdrOff = 0;
    pWebSocket->iFrameHdrLen = 2; // minimum header size
    pWebSocket->bFrameFin = FALSE;
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketCheckHeader

    \Description
        Check a server->client header for completion

    \Input *pWebSocket  - module state
    \Input *pStrBuf     - pointer to buffer containing received data from server
    \Input iSize        - amount of data received from the server

    \Output
        int32_t         - zero=in progress, else complete

    \Notes
        While the standard supports up to a 64-bit frame size, this implementation
        only supports up to 32-bit frame sizes.

    \Version 01/09/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoWebSocketCheckHeader(ProtoWebSocketRefT *pWebSocket, char *pStrBuf, int32_t iSize)
{
    // check for minimum frame size
    if (iSize < 2)
    {
        return(0);
    }
    // decode frame info
    pWebSocket->bFrameFin = ((uint8_t *)pStrBuf)[0] >> 7;
    pWebSocket->iFrameType = pStrBuf[0]&0x0f;
    pWebSocket->iFrameLen = pStrBuf[1];
    // handle extended frame lengths
    if (pWebSocket->iFrameLen == 126)
    {
        pWebSocket->iFrameHdrLen = 4;
    }
    else if (pWebSocket->iFrameLen == 127)
    {
        pWebSocket->iFrameHdrLen = 10;
    }
    // check extended header size
    if (iSize < pWebSocket->iFrameHdrLen)
    {
        return(0);
    }
    // decode extended frame lengths
    if (pWebSocket->iFrameLen == 126)
    {
        pWebSocket->iFrameLen = (((uint8_t *)pStrBuf)[2] << 8) | ((uint8_t *)pStrBuf)[3];
    }
    else if (pWebSocket->iFrameLen == 127)
    {
        pWebSocket->iFrameLen = (((uint8_t *)pStrBuf)[6] << 24) | (((uint8_t *)pStrBuf)[7] << 16) | (((uint8_t *)pStrBuf)[8] << 8) | ((uint8_t *)pStrBuf)[9];
    }
    // if it's a control frame, set up to process it
    if (pWebSocket->iFrameType & PROTOWEBSOCKET_CTRL_BASE)
    {
        pWebSocket->iCtrlDataLen = pWebSocket->iFrameLen;
        pWebSocket->iCtrlDataOff = 0;
    }
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketRecvHeader

    \Description
        Receive a server->client frame header.  As headers are variable length
        and we receive data directly into the user-supplied buffer, we process
        the header separately from the data.

    \Input *pWebSocket  - module state

    \Output
        int32_t         - negative=error, zero=in progress, positive=done

    \Version 01/09/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoWebSocketRecvHeader(ProtoWebSocketRefT *pWebSocket)
{
    int32_t iResult;

    // receive first two header bytes
    while ((iResult = _ProtoWebSocketRecv(pWebSocket, pWebSocket->strFrameHdr+pWebSocket->iFrameHdrOff, pWebSocket->iFrameHdrLen-pWebSocket->iFrameHdrOff)) > 0)
    {
        // accumulate result
        pWebSocket->iFrameHdrOff += iResult;
        // see if we have a valid header
        if ((iResult = _ProtoWebSocketCheckHeader(pWebSocket, pWebSocket->strFrameHdr, pWebSocket->iFrameHdrOff)) > 0)
        {
            break;
        }
    }
    #if DIRTYCODE_LOGGING
    if (iResult > 0)
    {
        NetPrintfVerbose((pWebSocket->iVerbose, 1, "protowebsocket: [%p] recv frame fin=%d typ=%s len=%d\n", pWebSocket, ((uint8_t *)pWebSocket->strFrameHdr)[0]>>7,
            _ProtoWebSocket_strOpcodeNames[pWebSocket->iFrameType], pWebSocket->iFrameLen));
    }
    #endif
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketRecvCtrl

    \Description
        Receive a control packet into internal buffer.  Any data that doesn't
        fit in the internal buffer is discarded.

    \Input *pWebSocket  - module state

    \Output
        int32_t         - zero

    \Version 02/01/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoWebSocketRecvCtrl(ProtoWebSocketRefT *pWebSocket)
{
    char strTempBuf[256], *pStrBuf;
    int32_t iSize;

    // receive into control data cache buffer (if we have enough room)
    if (pWebSocket->iCtrlDataOff < (signed)sizeof(pWebSocket->strCtrlData))
    {
        pStrBuf = pWebSocket->strCtrlData + pWebSocket->iCtrlDataOff;
        iSize = (signed)sizeof(pWebSocket->strCtrlData) - pWebSocket->iCtrlDataOff;
    }
    else // no room, so just receive into stack buffer and throw out
    {
        pStrBuf = strTempBuf;
        iSize = (signed)sizeof(strTempBuf);
    }
    pWebSocket->bRecvCtrlData = TRUE;
    pWebSocket->iCtrlDataOff += _ProtoWebSocketRecvData(pWebSocket, pStrBuf, iSize);
    pWebSocket->bRecvCtrlData = FALSE;

    // process if we've received the whole control frame
    if ((pWebSocket->iCtrlDataOff == pWebSocket->iCtrlDataLen) && (pStrBuf != strTempBuf))
    {
        _ProtoWebSocketCheckCtrl(pWebSocket, pStrBuf, pWebSocket->iCtrlDataLen);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketRecvData

    \Description
        Try and receive some data.

    \Input *pWebSocket  - module state
    \Input *pStrBuf     - [out] pointer to buffer to receive into
    \Input iSize        - amount of data to try and receive

    \Output
        int32_t         - negative=error, else success

    \Version 11/27/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoWebSocketRecvData(ProtoWebSocketRefT *pWebSocket, char *pStrBuf, int32_t iSize)
{
    int32_t iResult;

    // receive header
    if ((pWebSocket->iFrameLen == 0) && ((iResult = _ProtoWebSocketRecvHeader(pWebSocket)) <= 0))
    {
        return(iResult);
    }

    // receive control frame data
    if ((pWebSocket->iFrameType & PROTOWEBSOCKET_CTRL_BASE) && (!pWebSocket->bRecvCtrlData))
    {
        return(_ProtoWebSocketRecvCtrl(pWebSocket));
    }

    // limit read maximum to frame len
    if (iSize > pWebSocket->iFrameLen)
    {
        iSize = pWebSocket->iFrameLen;
    }

    // receive some data
    if ((iResult = _ProtoWebSocketRecv(pWebSocket, pStrBuf, iSize)) > 0)
    {
        // decrement frame size by amount of user data received
        pWebSocket->iFrameLen -= iResult;
        // if we've received all of the data, reset header info
        if (pWebSocket->iFrameLen == 0)
        {
            pWebSocket->iFrameHdrOff = 0;
            pWebSocket->iFrameHdrLen = 2; // minimum header size
        }
    }
    else if (iResult < 0)
    {
        _ProtoWebSocketFail(pWebSocket, iResult, "receiving data");
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketProcessSendPong

    \Description
        Process a pending pong request

    \Input *pWebSocket  - module state

    \Version 11/30/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoWebSocketProcessSendPong(ProtoWebSocketRefT *pWebSocket)
{
    int32_t iResult;
    if ((pWebSocket->bDoPong != TRUE) || (pWebSocket->iOutLen > 0))
    {
        return;
    }
    if ((iResult = _ProtoWebSocketSendPing(pWebSocket, pWebSocket->strCtrlData, PROTOWEBSOCKET_CTRL_PONG)) >= 0)
    {
        pWebSocket->bDoPong = FALSE;
    }
    else if (iResult < 0)
    {
        _ProtoWebSocketFail(pWebSocket, iResult, "sending pong");
        pWebSocket->bDoPong = FALSE;
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketProcessSendClose

    \Description
        Process a pending close request.

    \Input *pWebSocket  - module state

    \Version 11/30/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoWebSocketProcessSendClose(ProtoWebSocketRefT *pWebSocket)
{
    int32_t iResult;
    if ((pWebSocket->bDoClose != TRUE) || (pWebSocket->iOutLen > 0))
    {
        return;
    }
    if ((iResult = _ProtoWebSocketSendData(pWebSocket, pWebSocket->strCtrlData, pWebSocket->iCtrlDataLen, PROTOWEBSOCKET_CTRL_CLSE)) == 0)
    {
        pWebSocket->bDoClose = FALSE;
    }
    else if (iResult < 0)
    {
        _ProtoWebSocketFail(pWebSocket, iResult, "sending close");
        pWebSocket->bDoClose = FALSE;
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketProcessSend

    \Description
        Process an in-progress send from the output buffer.

    \Input *pWebSocket  - module state

    \Version 11/30/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoWebSocketProcessSend(ProtoWebSocketRefT *pWebSocket)
{
    int32_t iResult;
    if (pWebSocket->iOutLen == 0)
    {
        return;
    }
    if ((iResult = _ProtoWebSocketSendBuff(pWebSocket)) < 0)
    {
        _ProtoWebSocketFail(pWebSocket, iResult, "sending data");
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoWebSocketCreate

    \Description
        Allocate a WebSocket connection and prepare for use

    \Input iBufSize             - websocket buffer size (determines max message size)

    \Output
        ProtoWebSocketRefT *    - module state, or NULL on failure

    \Version 11/26/2012 (jbrookes)
*/
/********************************************************************************F*/
ProtoWebSocketRefT *ProtoWebSocketCreate(int32_t iBufSize)
{
    ProtoWebSocketRefT *pWebSocket;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // clamp the buffer size
    if (iBufSize < PROTOWEBSOCKET_BUFSIZE_MIN)
    {
        iBufSize = PROTOWEBSOCKET_BUFSIZE_MIN;
    }
    iBufSize += PROTOWEBSOCKET_MAXSENDHDR;

    // allocate and init module state
    if ((pWebSocket = DirtyMemAlloc(sizeof(*pWebSocket), PROTOWEBSOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protowebsocket: could not allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pWebSocket, sizeof(*pWebSocket));
    pWebSocket->iMemGroup = iMemGroup;
    pWebSocket->pMemGroupUserData = pMemGroupUserData;
    pWebSocket->iTimeout = PROTOWEBSOCKET_TIMEOUT;
    pWebSocket->iVersion = PROTOWEBSOCKET_PROTOCOLVERS;
    pWebSocket->iVerbose = 1;

    // allocate ssl module
    if ((pWebSocket->pProtoSSL = ProtoSSLCreate()) == NULL)
    {
        NetPrintf(("protowebsocket: [%p] unable to allocate ssl module\n", pWebSocket));
        ProtoWebSocketDestroy(pWebSocket);
        return(NULL);
    }
    // allocate websocket buffer
    if ((pWebSocket->pOutBuf = DirtyMemAlloc(iBufSize, PROTOWEBSOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protowebsocket: [%p] unable to allocate websocket buffer\n", pWebSocket));
        ProtoWebSocketDestroy(pWebSocket);
        return(NULL);
    }
    pWebSocket->iOutMax = iBufSize;

    // allocate temp input buffer
    pWebSocket->iInpMax = PROTOWEBSOCKET_DEFAULT_INPUTBUFSIZE;
    if ((pWebSocket->pInpBuf = (char *)DirtyMemAlloc(pWebSocket->iInpMax, PROTOWEBSOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protowebsocket: [%p] unable to allocate temporary input buffer\n", pWebSocket));
        ProtoWebSocketDestroy(pWebSocket);
        return(NULL);
    }

    // return module state to caller
    return(pWebSocket);
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketCreate2

    \Description
        Allocate a WebSocket connection and prepare for use: ProtoSSL function
        signature compatible version.

    \Output
        ProtoWebSocketRefT *    - module state, or NULL on failure

    \Version 03/08/2013 (jbrookes)
*/
/********************************************************************************F*/
ProtoWebSocketRefT *ProtoWebSocketCreate2(void)
{
    return(ProtoWebSocketCreate(PROTOWEBSOCKET_BUFSIZE_MIN));
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketDestroy

    \Description
        Destroy a connection and release state

    \Input *pWebSocket  - websocket module state

    \Version 11/26/2012 (jbrookes)
*/
/********************************************************************************F*/
void ProtoWebSocketDestroy(ProtoWebSocketRefT *pWebSocket)
{
    if (pWebSocket->pAppendHdr != NULL)
    {
        DirtyMemFree(pWebSocket->pAppendHdr, PROTOWEBSOCKET_MEMID, pWebSocket->iMemGroup, pWebSocket->pMemGroupUserData);
    }
    if (pWebSocket->pInpBuf != NULL)
    {
        DirtyMemFree(pWebSocket->pInpBuf, PROTOWEBSOCKET_MEMID, pWebSocket->iMemGroup, pWebSocket->pMemGroupUserData);
    }
    if (pWebSocket->pOutBuf != NULL)
    {
        DirtyMemFree(pWebSocket->pOutBuf, PROTOWEBSOCKET_MEMID, pWebSocket->iMemGroup, pWebSocket->pMemGroupUserData);
    }
    if (pWebSocket->pProtoSSL != NULL)
    {
        ProtoSSLDestroy(pWebSocket->pProtoSSL);
    }
    DirtyMemFree(pWebSocket, PROTOWEBSOCKET_MEMID, pWebSocket->iMemGroup, pWebSocket->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketConnect

    \Description
        Initiate a connection to a server

    \Input *pWebSocket          - websocket module state
    \Input *pUrl                - url containing host information; ws:// or wss://

    \Output
        int32_t                 - zero=success, else failure

    \Version 11/26/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoWebSocketConnect(ProtoWebSocketRefT *pWebSocket, const char *pUrl)
{
    int32_t iSecure, iResult;
    char strKind[16];

    // reset module state
    _ProtoWebSocketReset(pWebSocket);

    // parse out the url
    pUrl = ProtoHttpUrlParse(pUrl, strKind, sizeof(strKind), pWebSocket->strHost, sizeof(pWebSocket->strHost), &pWebSocket->iPort, &iSecure);

    // do our own secure determination
    iSecure = ds_stricmp(strKind, "wss") ? 0 : 1;
    if (iSecure && (pWebSocket->iPort == 80))
    {
        pWebSocket->iPort = 443;
    }

    // create key we send and encded key we must validate in server response
    _ProtoWebSocketGenerateKey(pWebSocket);

    // format the request header for sending once connected
    _ProtoWebSocketFormatHeader(pWebSocket, pWebSocket->strHost, pUrl, pWebSocket->iPort, iSecure);

    // start the connect
    NetPrintfVerbose((pWebSocket->iVerbose, 0, "protowebsocket: [%p] connecting to %s:%d\n", pWebSocket, pWebSocket->strHost, pWebSocket->iPort));
    iResult = ProtoSSLConnect(pWebSocket->pProtoSSL, iSecure, pWebSocket->strHost, 0, pWebSocket->iPort);
    pWebSocket->eState = (iResult == SOCKERR_NONE) ? ST_CONN : ST_FAIL;
    pWebSocket->uTimer = NetTick();
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketDisconnect

    \Description
        Disconnect from the server.  Note that this is an asynchronous operation.
        Connection status can be polled with the 'conn' status selector

    \Input *pWebSocket          - websocket module state

    \Output
        int32_t                 - zero=success, else failure

    \Version 11/26/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoWebSocketDisconnect(ProtoWebSocketRefT *pWebSocket)
{
    // ignore request in states where we are already disconnected
    if ((pWebSocket->eState == ST_DISC) || (pWebSocket->eState == ST_FAIL))
    {
        return(0);
    }
    // if we're in handshaking just do an immediate close
    if (pWebSocket->eState != ST_OPEN)
    {
        return(_ProtoWebSocketDisconnect(pWebSocket, ST_DISC));
    }
    // send close notification
    return(_ProtoWebSocketSendClose(pWebSocket, PROTOWEBSOCKET_CLOSEREASON_NORMAL, NULL));
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketSend

    \Description
        Send data to a server over a WebSocket connection.  Stream-type send, no
        framing is performed by ProtoWebSocket.

    \Input *pWebSocket          - websocket module state
    \Input *pBuffer             - data to send
    \Input iLength              - amount of data to send

    \Output
        int32_t                 - negative=failure, else number of bytes of data sent

    \Version 11/26/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoWebSocketSend(ProtoWebSocketRefT *pWebSocket, const char *pBuffer, int32_t iLength)
{
    return(_ProtoWebSocketSendUserData(pWebSocket, pBuffer, iLength, PROTOWEBSOCKET_OPCODE_BINA));
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketSendText

    \Description
        Send text data to a server over a WebSocket connection.  Stream-type send, no
        framing is performed by ProtoWebSocket.

    \Input *pWebSocket          - websocket module state
    \Input *pBuffer             - data to send (NULL terminated C-style string)

    \Output
        int32_t                 - negative=failure, else number of bytes of data sent

    \Version 08/30/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t ProtoWebSocketSendText(ProtoWebSocketRefT *pWebSocket, const char *pBuffer)
{
    return(_ProtoWebSocketSendUserData(pWebSocket, pBuffer, (int32_t)strlen(pBuffer), PROTOWEBSOCKET_OPCODE_TEXT));
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketSendMessage

    \Description
        Send a message to a server over a WebSocket connection.  If the message
        size is larger than the ProtoWebSocket buffer size, the message will be
        sent fragmented.

    \Input *pWebSocket          - websocket module state
    \Input *pBuffer             - data to send
    \Input iLength              - amount of data to send

    \Output
        int32_t                 - negative=failure, else number of bytes of data sent

    \Version 03/31/2017 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoWebSocketSendMessage(ProtoWebSocketRefT *pWebSocket, const char *pBuffer, int32_t iLength)
{
    int32_t iResult;
    pWebSocket->bMessageSend = TRUE;
    iResult = _ProtoWebSocketSendUserData(pWebSocket, pBuffer, iLength, PROTOWEBSOCKET_OPCODE_BINA);
    pWebSocket->bMessageSend = FALSE;
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketSendMessageText

    \Description
        Send a text message to a server over a WebSocket connection.  If the message
        size is larger than the ProtoWebSocket buffer size, the message will be
        sent fragmented.

    \Input *pWebSocket          - websocket module state
    \Input *pBuffer             - data to send (NULL terminated C-style string)

    \Output
        int32_t                 - negative=failure, else number of bytes of data sent

    \Version 08/30/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t ProtoWebSocketSendMessageText(ProtoWebSocketRefT *pWebSocket, const char *pBuffer)
{
    int32_t iResult;
    pWebSocket->bMessageSend = TRUE;
    iResult = _ProtoWebSocketSendUserData(pWebSocket, pBuffer, (int32_t)strlen(pBuffer), PROTOWEBSOCKET_OPCODE_TEXT);
    pWebSocket->bMessageSend = FALSE;
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketRecv

    \Description
        Receive data from a server over a WebSocket connection.  Stream-type recv,
        framing bits are ignored.

    \Input *pWebSocket          - websocket module state
    \Input *pBuffer             - [out] buffer to receive data
    \Input iLength              - size of buffer

    \Output
        int32_t                 - negative=failure, else number of bytes of data received

    \Version 11/26/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoWebSocketRecv(ProtoWebSocketRefT *pWebSocket, char *pBuffer, int32_t iLength)
{
    // handle not connected states
    if ((pWebSocket->eState == ST_DISC) || (pWebSocket->eState == ST_FAIL))
    {
        return(SOCKERR_NOTCONN);
    }
    if (pWebSocket->eState != ST_OPEN)
    {
        return(0);
    }

    // connected, do the receive
    return(_ProtoWebSocketRecvData(pWebSocket, pBuffer, iLength));
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketRecvMessage

    \Description
        Receive a message from a server over a WebSocket connection.  Note that
        the user buffer is expected to persist and be capable of receiving the
        entire message.

    \Input *pWebSocket          - websocket module state
    \Input *pBuffer             - [out] buffer to receive data
    \Input iLength              - size of buffer

    \Output
        int32_t                 - negative=failure, SOCKERR_NOMEM if the user buffer
                                  is too small, zero=pending, else frame size received

    \Version 03/30/2017 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoWebSocketRecvMessage(ProtoWebSocketRefT *pWebSocket, char *pBuffer, int32_t iLength)
{
    int32_t iResult;

    // see if we need more buffer
    if (pWebSocket->iRecvSize == iLength)
    {
        return(SOCKERR_NOMEM);
    }

    // do the receive
    if ((iResult = ProtoWebSocketRecv(pWebSocket, pBuffer+pWebSocket->iRecvSize, iLength-pWebSocket->iRecvSize)) > 0)
    {
        // accumulate receive size
        pWebSocket->iRecvSize += iResult;

        // return accumulated size if final frame and completely read, else return zero
        if ((pWebSocket->bFrameFin) && (pWebSocket->iFrameLen == 0))
        {
            iResult = pWebSocket->iRecvSize;
            pWebSocket->iRecvSize = 0;
            pWebSocket->bFrameFin = FALSE;
        }
        else
        {
            iResult = 0;
        }
    }

    // return result to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketStatus

    \Description
        Get module status

    \Input *pWebSocket          - websocket module state
    \Input iSelect              - status selector
    \Input *pBuffer             - [inp/out] buffer, selector specific
    \Input iBufSize             - size of buffer

    \Output
        int32_t                 - selector specific

    \Notes
        Selectors are:

    \verbatim
        SELECTOR    RETURN RESULT
        'crsn'      returns close reason (PROTOWEBSOCKET_CLOSEREASON_*)
        'essl'      returns protossl error state
        'host'      current host copied to output buffer
        'port'      current port
        'stat'      connection status (-1=not connected, 0=connection in progress, 1=connected)
        'time'      TRUE if the client timed out the connection, else FALSE
    \endverbatim

    \Version 11/26/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoWebSocketStatus(ProtoWebSocketRefT *pWebSocket, int32_t iSelect, void *pBuffer, int32_t iBufSize)
{
    // return close reason
    if (iSelect == 'crsn')
    {
        if ((pBuffer != NULL) && (pWebSocket->iCtrlDataLen > 0))
        {
            int32_t iCopyLen = (iBufSize < pWebSocket->iCtrlDataLen) ? iBufSize : pWebSocket->iCtrlDataLen;
            ds_memcpy(pBuffer, pWebSocket->strCtrlData, iCopyLen);
        }
        return(pWebSocket->iCloseReason);
    }

    // return protossl error state (we cache this since the state is reset when we disconnect on error)
    if (iSelect == 'essl')
    {
        return(pWebSocket->iSSLFail);
    }

    // return current host
    if (iSelect == 'host')
    {
        ds_strnzcpy(pBuffer, pWebSocket->strHost, iBufSize);
        return(0);
    }

    // return current port
    if (iSelect == 'port')
    {
        return(pWebSocket->iPort);
    }

    // check connection status
    if (iSelect == 'stat')
    {
        if ((pWebSocket->eState == ST_DISC) || (pWebSocket->eState == ST_FAIL))
        {
            return(-1);
        }
        else if (pWebSocket->eState != ST_OPEN)
        {
            return(0);
        }
        // if we're connected (ST_OPEN) fall through to protossl connectivity check
    }

    // return timeout indicator
    if (iSelect == 'time')
    {
        return(pWebSocket->bTimeout);
    }

    // pass through unhandled selectors to protossl
    return(ProtoSSLStat(pWebSocket->pProtoSSL, iSelect, pBuffer, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketControl

    \Description
        Control module behavior

    \Input *pWebSocket          - websocket module state
    \Input iSelect              - control selector
    \Input iValue               - selector specific
    \Input iValue2              - selector specific
    \Input *pValue              - selector specific

    \Output
        int32_t                 - selector specific

    \Notes
        Selectors are:

        \verbatim
            SELECTOR    DESCRIPTION
            'apnd'      The given buffer will be appended to future headers sent by
                        ProtoWebSocket.  The Accept header in the default header will
                        be omitted.  If there is a User-Agent header included in the
                        append header, it will replace the default User-Agent header.
            'clse'      Send a close message with optional reason (iValue) and data
                        (string passed in pValue)
            'keep'      Set keep-alive timer, in seconds; client sends a 'pong' at this
                        rate to keep connection alive (0 disables; disabled by default)
            'ires'      Resize the input buffer
            'ping'      Send a ping message with optional data (string passed in pValue)
            'pong'      Send a pong message with optional data (string passed in pValue)
            'spam'      Sets debug output verbosity (0...n)
            'time'      Sets ProtoWebSocket client timeout in seconds (default=300s)
        \endverbatim

        Unhandled selectors are passed on to ProtoSSL.

    \Version 11/30/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoWebSocketControl(ProtoWebSocketRefT *pWebSocket, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iSelect == 'apnd')
    {
        return(_ProtoWebSocketSetAppendHeader(pWebSocket, (const char *)pValue));
    }
    if (iSelect == 'clse')
    {
        return(_ProtoWebSocketSendClose(pWebSocket, iValue, (const char *)pValue));
    }
    if (iSelect == 'keep')
    {
        // enforce a minimum keep-alive interval of 30s
        if (iValue < 30)
        {
            NetPrintf(("protowebsocket: [%p] warning: enforcing a minimum keep-alive interval (%ds->30s)\n", pWebSocket, iValue));
            iValue = 30;
        }
        pWebSocket->iKeepAlive = iValue*1000;
        return(0);
    }
    if (iSelect == 'ires')
    {
        char *pInpBuf;

        // make sure that we don't truncate any data already in the buffer, this assumes that you don't resize multiple times
        iValue = DS_MAX(iValue, PROTOWEBSOCKET_DEFAULT_INPUTBUFSIZE);

        // allocate buffer and reassign
        if ((pInpBuf = (char *)DirtyMemAlloc(iValue, PROTOWEBSOCKET_MEMID, pWebSocket->iMemGroup, pWebSocket->pMemGroupUserData)) == NULL)
        {
            NetPrintf(("protowebsocket: [%p] unable to allocate buffer for resizing input buffer\n", pWebSocket));
            return(-1);
        }
        ds_memcpy(pInpBuf, pWebSocket->pInpBuf, pWebSocket->iInpLen);

        DirtyMemFree(pWebSocket->pInpBuf, PROTOWEBSOCKET_MEMID, pWebSocket->iMemGroup, pWebSocket->pMemGroupUserData);
        pWebSocket->pInpBuf = pInpBuf;
        pWebSocket->iInpMax = iValue;

        return(0);
    }
    if ((iSelect == 'ping') || (iSelect == 'pong'))
    {
        return(_ProtoWebSocketSendPing(pWebSocket, (const char *)pValue, (iSelect == 'ping') ? PROTOWEBSOCKET_CTRL_PING : PROTOWEBSOCKET_CTRL_PONG));
    }
    if (iSelect == 'spam')
    {
        pWebSocket->iVerbose = iValue;
        return(0);
    }
    if (iSelect == 'time')
    {
        pWebSocket->iTimeout = iValue*1000;
        return(0);
    }
    // pass unhandled selectors through to ProtoSSL
    return(ProtoSSLControl(pWebSocket->pProtoSSL, iSelect, iValue, iValue2, pValue));
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketUpdate

    \Description
        Give life to module (should be called periodically to allow module to
        perform work).

    \Input *pWebSocket          - websocket module state

    \Version 11/26/2012 (jbrookes)
*/
/********************************************************************************F*/
void ProtoWebSocketUpdate(ProtoWebSocketRefT *pWebSocket)
{
    uint32_t uCurTick = NetTick();
    int32_t iResult;
    char *pStart, *pEnd;

    // give time to comm module
    ProtoSSLUpdate(pWebSocket->pProtoSSL);

    // check for timeout
    if ((pWebSocket->eState != ST_DISC) && (pWebSocket->eState != ST_FAIL))
    {
        if (NetTickDiff(uCurTick, pWebSocket->uTimer) > pWebSocket->iTimeout)
        {
            NetPrintf(("protowebsocket: [%p] inactivity timeout (state=%d)\n", pWebSocket, pWebSocket->eState));
            pWebSocket->eState = ST_FAIL;
            pWebSocket->bTimeout = TRUE;
        }
    }

    // see if network connection is complete
    if (pWebSocket->eState == ST_CONN)
    {
        if ((iResult = ProtoSSLStat(pWebSocket->pProtoSSL, 'stat', NULL, 0)) > 0)
        {
            pWebSocket->uTimer = uCurTick;
            pWebSocket->eState = ST_SEND;
        }
        else if (iResult < 0)
        {
            _ProtoWebSocketFail(pWebSocket, iResult, "connecting");
        }
    }

    // sending handshake
    if (pWebSocket->eState == ST_SEND)
    {
        if ((iResult = _ProtoWebSocketSendBuff(pWebSocket)) > 0)
        {
            NetPrintfVerbose((pWebSocket->iVerbose, 1, "protowebsocket: [%p] sent handshake request\n", pWebSocket));
            #if DIRTYCODE_LOGGING
            if (pWebSocket->iVerbose > 2)
            {
                NetPrintWrap(pWebSocket->pOutBuf, 80);
            }
            #endif
            pWebSocket->eState = ST_RECV;
        }
        else if (iResult < 0)
        {
            _ProtoWebSocketFail(pWebSocket, iResult, "sending handshake request");
        }
    }

    // receive server handshake response: note that due to serialized nature of handshaking
    if (pWebSocket->eState == ST_RECV)
    {
        if ((iResult = _ProtoWebSocketRecv(pWebSocket, pWebSocket->pInpBuf+pWebSocket->iInpLen, pWebSocket->iInpMax-pWebSocket->iInpLen)) > 0)
        {
            pWebSocket->iInpLen += iResult;
            // see if we have the complete header
            pStart = pWebSocket->pInpBuf;
            pEnd = pWebSocket->pInpBuf+pWebSocket->iInpLen-3;

            // scan for blank line marking body start
            while ((pStart != pEnd) && ((pStart[0] != '\r') || (pStart[1] != '\n') || (pStart[2] != '\r') || (pStart[3] != '\n')))
            {
                pStart += 1;
            }

            if (pStart != pEnd)
            {
                // process the header
                if ((iResult = _ProtoWebSocketProcessHeader(pWebSocket, pWebSocket->pInpBuf)) >= 0)
                {
                    NetPrintf(("protowebsocket: [%p] connection open\n", pWebSocket));
                    pWebSocket->eState = ST_OPEN;
                }
                else
                {
                    _ProtoWebSocketFail(pWebSocket, iResult, "processing handshake response");
                }
                // remove header from buffer
                pWebSocket->iInpOff = (int32_t)(pStart+4-pWebSocket->pInpBuf);
            }
        }
        else if (iResult < 0)
        {
            _ProtoWebSocketFail(pWebSocket, iResult, "receiving handshake response");
        }
    }

    // process in open state
    if (pWebSocket->eState == ST_OPEN)
    {
        // issue a keep-alive?
        if ((pWebSocket->iKeepAlive > 0) && (NetTickDiff(uCurTick, pWebSocket->uTimer) > pWebSocket->iKeepAlive))
        {
            NetPrintfVerbose((pWebSocket->iVerbose, 0, "protowebsocket: [%p] sending keep-alive at %u\n", pWebSocket, uCurTick));
            pWebSocket->bDoPong = TRUE;
            pWebSocket->strCtrlData[0] = '\0';
            pWebSocket->iCtrlDataLen = 0;
        }

        // process pong request
        _ProtoWebSocketProcessSendPong(pWebSocket);

        // process close request
        _ProtoWebSocketProcessSendClose(pWebSocket);

        // process send request
        _ProtoWebSocketProcessSend(pWebSocket);
    }
}
