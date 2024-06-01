/*H********************************************************************************/
/*!
    \File protohttp.c

    \Description
        This module implements an HTTP client that can perform basic transactions
        (get/put) with an HTTP server.  It conforms to but does not fully implement
        the 1.1 HTTP spec (http://www.w3.org/Protocols/rfc2616/rfc2616.html), and
        allows for secure HTTP transactions as well as insecure transactions.

    \Copyright
        Copyright (c) Electronic Arts 2000-2004. ALL RIGHTS RESERVED.

    \Version 0.5 02/21/2000 (gschaefer) First Version
    \Version 1.0 12/07/2000 (gschaefer) Added PS2/Dirtysock support
    \Version 1.1 03/03/2004 (sbevan)    Rewrote to use ProtoSSL, added limited Post support.
    \Version 1.2 11/18/2004 (jbrookes)  Refactored, updated to HTTP 1.1, added full Post support.
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/proto/protohttp.h"

/*** Defines **********************************************************************/

//! default ProtoHttp timeout
#define PROTOHTTP_TIMEOUT_DEFAULT (30*1000)

//! default maximum allowed redirections
#define PROTOHTTP_MAXREDIRECT   (3)

//! size of "last-received" header cache
#define PROTOHTTP_HDRCACHESIZE  (1024)

//! protohttp revision number (maj.min)
#define PROTOHTTP_VERSION       (0x0103)          // update this for major bug fixes or protocol additions/changes

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! http module state
struct ProtoHttpRefT
{
    ProtoSSLRefT *pSsl;     //!< ssl module

    ProtoHttpCustomHeaderCbT *pCustomHeaderCb;      //!< global callback for modifying request header
    ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb;    //!< global callback for viewing recv header on recepit
    void *pCallbackRef;     //!< user ref for callback

    ProtoHttpWriteCbT *pWriteCb;                    //!< optional data write callback
    ProtoHttpCustomHeaderCbT *pReqCustomHeaderCb;   //!< optional request custom header callback
    ProtoHttpReceiveHeaderCbT *pReqReceiveHeaderCb; //!< optional request receive header callback
    void *pUserData;                                //!< user data for callback

    // module memory group
    int32_t iMemGroup;      //!< module mem group id
    void *pMemGroupUserData;//!< user data associated with mem group

    NetCritT HttpCrit;      //!< critical section for guarding update from send/recv

    ProtoHttpRequestTypeE eRequestType;  //!< request type of current request
    int32_t iPort;          //!< server port
    int32_t iBasePort;      //!< base port (used for partial urls)
    int32_t iProxiedPort;   //!< port of proxied host
    int32_t iSecure;        //!< secure connection
    int32_t iBaseSecure;    //!< base security setting (used for partial urls)
    int32_t iProxiedSecure; //!< true if proxied connection is secure

    enum
    {
        ST_IDLE,            //!< idle
        ST_CONN,            //!< connecting
        ST_SEND,            //!< sending buffered data
        ST_RESP,            //!< waiting for initial response (also sending any data not buffered if POST or PUT)
        ST_HEAD,            //!< getting header
        ST_BODY,            //!< getting body
        ST_DONE,            //!< transaction success
        ST_FAIL             //!< transaction failed
    } eState;               //!< current state

    int32_t iSslFail;       //!< ssl failure code, if any
    int32_t iHresult;       //!< ssl hresult code, if any
    int32_t iHdrCode;       //!< result code
    int32_t iHdrDate;       //!< last modified date

    int32_t iHeadSize;      //!< size of head data
    int64_t iPostSize;      //!< amount of data being sent in a POST or PUT operation
    int64_t iBodySize;      //!< size of body data
    int64_t iBodyRcvd;      //!< size of body data received by caller
    int32_t iRecvSize;      //!< amount of data received by ProtoHttpRecvAll
    int32_t iRecvRslt;      //!< last receive result

    char *pInpBuf;          //!< input buffer
    int32_t iInpMax;        //!< maximum buffer size
    int32_t iInpOff;        //!< offset into buffer
    int32_t iInpLen;        //!< total length in buffer
    int64_t iInpCnt;        //!< ongoing count
    int32_t iInpOvr;        //!< input overflow amount
    int32_t iChkLen;        //!< chunk length (if chunked encoding)
    int32_t iHdrLen;        //!< length of header(s) queued for sending
    int32_t iHdrOff;        //!< temp offset used when receiving header

    char *pInpBufTmp;         //!< temp storage for input buffer pointer when using connect flow
    int32_t iInpLenTmp;       //!< temp storage for input buffer length when using connect flow

    int32_t iNumRedirect;   //!< number of redirections processed
    int32_t iMaxRedirect;   //!< maximum number of redirections allowed

    uint32_t uTimeout;      //!< protocol timeout
    uint32_t uTimer;        //!< timeout timer
    int32_t iKeepAlive;     //!< indicate if we should try to use keep-alive
    int32_t iKeepAliveDflt; //!< keep-alive default (keep-alive will be reset to this value; can be overridden by user)

    char *pAppendHdr;       //!< append header buffer pointer
    int32_t iAppendLen;     //!< size of append header buffer

    char strHdr[PROTOHTTP_HDRCACHESIZE]; //!< storage for most recently received HTTP header
    char strRequestHdr[PROTOHTTP_HDRCACHESIZE]; //!< storage for most recent HTTP request header
    char strConnectHdr[256]; //!< temp storage for connect header when using connect flow
    char strHost[256];      //!< server name
    char strBaseHost[256];  //!< base server name (used for partial urls)
    char strProxy[256];     //!< proxy server name/address (including port)
    char strProxiedHost[256]; //!< hostname of server we are connecting to through proxy

    uint8_t bTimeout;       //!< boolean indicating whether a timeout occurred or not
    uint8_t bChunked;       //!< if TRUE, transfer is chunked
    uint8_t bHeadOnly;      //!< if TRUE, only get header
    uint8_t bCloseHdr;      //!< server wants close after this
    uint8_t bClosed;        //!< connection has been closed
    uint8_t bConnOpen;      //!< connection is open
    uint8_t iVerbose;       //!< debug output verbosity
    uint8_t bVerifyHdr;     //!< perform header type verification
    uint8_t bHttp1_0;       //!< TRUE if HTTP/1.0, else FALSE
    uint8_t bCompactRecv;   //!< compact receive buffer
    uint8_t bInfoHdr;       //!< TRUE if a new informational header has been cached; else FALSE
    uint8_t bNewConnection; //!< TRUE if a new connection should be used, else FALSE (if using keep-alive)
    uint8_t bPipelining;    //!< TRUE if pipelining is enabled, else FALSE
    uint8_t bPipeGetNext;   //!< TRUE if we should proceed to next pipelined result, else FALSE
    int8_t iPipedRequests;  //!< number of pipelined requests
    uint8_t bPipedRequestsLost; //!< TRUE if pipelined requests were lost due to a premature close
    uint8_t bReuseOnPost;   //!< TRUE if reusing a previously established connection on PUT/POST is allowed, else FALSE
    uint8_t bConnProxy;     //!< if true, executing secure proxy connect flow
    uint8_t bUpgradeSSL;    //!< upgrade connection to SSL after connect

};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private variables

// update this when PROTOHTTP_NUMREQUESTTYPES changes
static const char _ProtoHttp_strRequestNames[PROTOHTTP_NUMREQUESTTYPES][16] =
{
    "HEAD", "GET", "POST", "PUT", "DELETE", "OPTIONS", "PATCH", "CONNECT"
};

//! global proxy; if this is set all ProtoHttp refs will use this as their proxy
static char _ProtoHttp_strGlobalProxy[256] = "";


// Public variables


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ProtoHttpApplyBaseUrl

    \Description
        Apply base url elements (if set) to any url elements not specified (relative
        url support).

    \Input *pState      - module state
    \Input *pKind       - parsed http kind ("http" or "https")
    \Input *pHost       - [in/out] parsed URL host
    \Input iHostSize    - size of pHost buffer
    \Input *pPort       - [in/out] parsed port
    \Input *pSecure     - [in/out] parsed security (0 or 1)
    \Input bPortSpecified - TRUE if a port is explicitly specified in the url, else FALSE

    \Output
        uint32_t        - non-zero if changed, else zero

    \Version 02/03/2010 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _ProtoHttpApplyBaseUrl(ProtoHttpRefT *pState, const char *pKind, char *pHost, int32_t iHostSize, int32_t *pPort, int32_t *pSecure, uint8_t bPortSpecified)
{
    uint8_t bChanged = FALSE;
    if ((*pHost == '\0') && (pState->strBaseHost[0] != '\0'))
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp: host not present; setting to %s\n", pState->strBaseHost));
        ds_strnzcpy(pHost, pState->strBaseHost, iHostSize);
        bChanged = TRUE;
    }
    if ((bPortSpecified == FALSE) && (pState->iBasePort != 0))
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp: port not present; setting to %d\n", pState->iBasePort));
        *pPort = pState->iBasePort;
        bChanged = TRUE;
    }
    if (*pKind == '\0')
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp: kind (protocol) not present; setting to %d\n", pState->iBaseSecure));
        *pSecure = pState->iBaseSecure;
        // if our port setting is default and incompatible with our security setting, override it
        if (((*pPort == 80) && (*pSecure == 1)) || ((*pPort == 443) && (*pSecure == 0)))
        {
            *pPort = *pSecure ? 443 : 80;
        }
        bChanged = TRUE;
    }
    return(bChanged);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpClose

    \Description
        Close connection to server, if open.

    \Input *pState      - module state
    \Input *pReason     - reason connection is being closed (for debug output)

    \Output
        None.

    \Version 10/07/2005 (jbrookes) First Version
*/
/********************************************************************************F*/
static void _ProtoHttpClose(ProtoHttpRefT *pState, const char *pReason)
{
    if (pState->bClosed)
    {
        // already issued disconnect, don't need to do it again
        return;
    }

    NetPrintfVerbose((pState->iVerbose, 0, "protohttp: [%p] closing connection: %s\n", pState, pReason));
    ProtoSSLDisconnect(pState->pSsl);
    pState->bCloseHdr = FALSE;
    pState->bConnOpen = FALSE;
    pState->bClosed = TRUE;
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpReset

    \Description
        Reset state before a transaction request.

    \Input  *pState     - reference pointer

    \Output
        None.

    \Version 11/22/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static void _ProtoHttpReset(ProtoHttpRefT *pState)
{
    ds_memclr(pState->strHdr, sizeof(pState->strHdr));
    ds_memclr(pState->strRequestHdr, sizeof(pState->strRequestHdr));
    pState->eState = ST_IDLE;
    pState->iSslFail = 0;
    pState->iHresult = 0;
    pState->iHdrCode = -1;
    pState->iHdrDate = 0;
    pState->iHeadSize = 0;
    pState->iBodySize = pState->iBodyRcvd = 0;
    pState->iRecvSize = 0;
    pState->iInpOff = 0;
    pState->iInpLen = 0;
    pState->iInpOvr = 0;
    pState->iChkLen = 0;
    pState->bTimeout = FALSE;
    pState->bChunked = FALSE;
    pState->bClosed = FALSE;
    pState->bHeadOnly = FALSE;
    pState->bPipeGetNext = FALSE;
    pState->bPipedRequestsLost = FALSE;
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpFreeInputBuf

    \Description
        Free input buffer

    \Input  *pState     - reference pointer

    \Version 06/20/2017 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpFreeInputBuf(ProtoHttpRefT *pState)
{
    char *pInpBuf = (pState->pInpBuf != pState->strConnectHdr) ? pState->pInpBuf : pState->pInpBufTmp;
    DirtyMemFree(pInpBuf, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpSetAppendHeader

    \Description
        Set given string as append header, allocating memory as required.

    \Input *pState      - reference pointer
    \Input *pAppendHdr  - append header string

    \Output
        int32_t         - zero=success, else error

    \Version 11/11/2004 (jbrookes) Split/combined from ProtoHttpGet() and ProtoHttpPost()
*/
/********************************************************************************F*/
static int32_t _ProtoHttpSetAppendHeader(ProtoHttpRefT *pState, const char *pAppendHdr)
{
    int32_t iAppendBufLen, iAppendStrLen;

    // check for empty append string, in which case we free the buffer
    if ((pAppendHdr == NULL) || (*pAppendHdr == '\0'))
    {
        if (pState->pAppendHdr != NULL)
        {
            DirtyMemFree(pState->pAppendHdr, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            pState->pAppendHdr = NULL;
        }
        pState->iAppendLen = 0;
        return(0);
    }

    // check to see if append header is already set
    if ((pState->pAppendHdr != NULL) && (!strcmp(pAppendHdr, pState->pAppendHdr)))
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp: ignoring set of append header '%s' that is already set\n", pAppendHdr));
        return(0);
    }

    // get append header length
    iAppendStrLen = (int32_t)strlen(pAppendHdr);
    // append buffer size includes null and space for \r\n if not included by submitter
    iAppendBufLen = iAppendStrLen + 3;

    // see if we need to allocate a new buffer
    if (iAppendBufLen > pState->iAppendLen)
    {
        if (pState->pAppendHdr != NULL)
        {
            DirtyMemFree(pState->pAppendHdr, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        }
        if ((pState->pAppendHdr = DirtyMemAlloc(iAppendBufLen, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) != NULL)
        {
            pState->iAppendLen = iAppendBufLen;
        }
        else
        {
            NetPrintf(("protohttp: could not allocate %d byte buffer for append header\n", iAppendBufLen));
            pState->iAppendLen = 0;
            return(-1);
        }
    }

    // copy append header
    ds_strnzcpy(pState->pAppendHdr, pAppendHdr, iAppendStrLen+1);

    // if append header is not \r\n terminated, do it here
    if (pAppendHdr[iAppendStrLen-2] != '\r' || pAppendHdr[iAppendStrLen-1] != '\n')
    {
        ds_strnzcat(pState->pAppendHdr, "\r\n", pState->iAppendLen);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpFormatRequestHeader

    \Description
        Format a request header based on given input data.

    \Input *pState      - reference pointer
    \Input *pUrl        - pointer to user-supplied url
    \Input *pHost       - pointer to hostname
    \Input iPort        - port, or zero if unspecified
    \Input iSecure      - 1=enabled, 0=disabled
    \Input *pRequest    - pointer to request type ("GET", "HEAD", "POST", "PUT")
    \Input iDataLen     - size of included data; zero if none, negative if streaming put/post

    \Output
        int32_t         - zero=success, else error

    \Version 11/11/2004 (jbrookes) Split/combined from ProtoHttpGet() and ProtoHttpPost()
*/
/********************************************************************************F*/
static int32_t _ProtoHttpFormatRequestHeader(ProtoHttpRefT *pState, const char *pUrl, const char *pHost, int32_t iPort, int32_t iSecure, const char *pRequest, int64_t iDataLen)
{
    int32_t iInpMax, iOffset = 0;
    const char *pUrlSlash;
    char *pInpBuf;
    ProtoHttpCustomHeaderCbT *pCustomHeaderCb;
    void *pUserData;

    // if url is empty or isn't preceded by a slash, put one in
    pUrlSlash = (*pUrl != '/') ? "/" : "";

    // set up for header formatting
    pInpBuf = pState->pInpBuf + pState->iInpLen;
    iInpMax = pState->iInpMax - pState->iInpLen;
    if (pState->iInpLen != 0)
    {
        pState->iPipedRequests += 1;
    }

    // format request header
    iOffset += ds_snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "%s %s%s HTTP/1.1\r\n", pRequest, pUrlSlash, pUrl);
    if ((iSecure && (iPort == 443)) || (iPort == 80))
    {
        iOffset += ds_snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "Host: %s\r\n", pHost);
    }
    else
    {
        iOffset += ds_snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "Host: %s:%d\r\n", pHost, iPort);
    }
    if (iDataLen == -1)
    {
        iOffset += ds_snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "Transfer-Encoding: chunked\r\n");
    }
    else if ((iDataLen > 0) || (pState->eRequestType == PROTOHTTP_REQUESTTYPE_PUT) || (pState->eRequestType == PROTOHTTP_REQUESTTYPE_PATCH) || (pState->eRequestType == PROTOHTTP_REQUESTTYPE_POST))
    {
        iOffset += ds_snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "Content-Length: %qd\r\n", iDataLen);
    }
    if (pState->iKeepAlive == 0)
    {
        iOffset += ds_snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "Connection: Close\r\n");
    }
    if ((pState->pAppendHdr == NULL) || !ds_stristr(pState->pAppendHdr, "User-Agent:"))
    {
        iOffset += ds_snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "User-Agent: ProtoHttp %d.%d/DS %d.%d.%d.%d.%d (" DIRTYCODE_PLATNAME ")\r\n",
            (PROTOHTTP_VERSION>>8)&0xff, PROTOHTTP_VERSION&0xff, DIRTYSDK_VERSION_YEAR, DIRTYSDK_VERSION_SEASON, DIRTYSDK_VERSION_MAJOR, DIRTYSDK_VERSION_MINOR, DIRTYSDK_VERSION_PATCH);
    }
    if ((pState->pAppendHdr == NULL) || (pState->pAppendHdr[0] == '\0'))
    {
        iOffset += ds_snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "Accept: */*\r\n");
    }
    else
    {
        iOffset += ds_snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "%s", pState->pAppendHdr);
    }

    // request level callback takes priority to global
    if ((pCustomHeaderCb = pState->pReqCustomHeaderCb) != NULL)
    {
        pUserData = pState->pUserData;
    }
    else
    {
        pCustomHeaderCb = pState->pCustomHeaderCb;
        pUserData = pState->pCallbackRef;
    }

    // call custom header format callback, if specified
    if (pCustomHeaderCb != NULL)
    {
        if ((iOffset = pCustomHeaderCb(pState, pInpBuf, iInpMax, NULL, 0, pUserData)) < 0)
        {
            NetPrintfVerbose((pState->iVerbose, 0, "protohttp: custom header callback error %d\n", iOffset));
            return(iOffset);
        }
        if (iOffset == 0)
        {
            iOffset = (int32_t)strlen(pInpBuf);
        }
    }

    // append header terminator and return header length
    iOffset += ds_snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "\r\n");

    // make sure we were able to complete the header
    if (iOffset > iInpMax)
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttp: not enough buffer to format request header (have %d, need %d)\n", iInpMax, iOffset));
        pState->iInpOvr = iOffset;
        return(PROTOHTTP_MINBUFF);
    }

    // save a copy of the header
    ds_strnzcpy(pState->strRequestHdr, pInpBuf, sizeof(pState->strRequestHdr));

    // update buffer size
    pState->iInpLen += iOffset;

    // save updated header size
    pState->iHdrLen = pState->iInpLen;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpFormatConnectHeader

    \Description
        Format a proxy connect request header as per
        https://tools.ietf.org/html/rfc7231#section-4.3.6.

    \Input *pState      - reference pointer
    \Input *pStrHost    - pointer to proxy hostname
    \Input iPort        - proxy port

    \Version 05/30/2017 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpFormatConnectHeader(ProtoHttpRefT *pState, const char *pStrHost, int32_t iPort)
{
    int32_t iOffset = 0, iInpMax;

    // save current input buffer info
    pState->pInpBufTmp = pState->pInpBuf;
    pState->iInpLenTmp = pState->iInpLen;

    // point to temp connect header
    pState->pInpBuf = pState->strConnectHdr;
    iInpMax = sizeof(pState->strConnectHdr);

    // format request header
    iOffset += ds_snzprintf(pState->pInpBuf + iOffset, iInpMax - iOffset, "%s %s:%d HTTP/1.1\r\n", _ProtoHttp_strRequestNames[PROTOHTTP_REQUESTTYPE_CONNECT], pStrHost, iPort);
    iOffset += ds_snzprintf(pState->pInpBuf + iOffset, iInpMax - iOffset, "Host: %s:%d\r\n", pStrHost, iPort);
    // append header terminator and return header length
    iOffset += ds_snzprintf(pState->pInpBuf + iOffset, iInpMax - iOffset, "\r\n");
    // update buffer size
    pState->iInpLen = iOffset;
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpFormatRequest

    \Description
        Format a request into the local buffer.

    \Input *pState      - reference pointer
    \Input *pUrl        - pointer to user-supplied url
    \Input *pData       - pointer to data to include with request, or NULL
    \Input iDataLen     - size of data pointed to by pData, or zero if no data
    \Input eRequestType - type of request (PROTOHTTP_REQUESTTYPE_*)

    \Output
        int32_t         - bytes of userdata included in request

    \Version 10/07/2005 (jbrookes) Split/combined from ProtoHttpGet() and ProtoHttpPost()
*/
/********************************************************************************F*/
static int32_t _ProtoHttpFormatRequest(ProtoHttpRefT *pState, const char *pUrl, const char *pData, int64_t iDataLen, ProtoHttpRequestTypeE eRequestType)
{
    char strHost[sizeof(pState->strHost)], strKind[8];
    int32_t iPort, iResult, iSecure;
    int32_t eState = pState->eState;
    uint8_t bPortSpecified;
    const char *pStrProxy;

    NetPrintfVerbose((pState->iVerbose, 0, "protohttp: [%p] %s %s\n", pState, _ProtoHttp_strRequestNames[eRequestType], pUrl));
    pState->eRequestType = eRequestType;

    // reset various state
    if (pState->eState != ST_IDLE)
    {
        _ProtoHttpReset(pState);
    }

    // restore input buffer, if set up for proxy
    if (pState->pInpBuf == pState->strConnectHdr)
    {
        pState->pInpBuf = pState->pInpBufTmp;
    }

    // use global proxy if set, otherwise use state-local proxy
    pStrProxy = (_ProtoHttp_strGlobalProxy[0] != '\0') ? _ProtoHttp_strGlobalProxy : pState->strProxy;

    // assume we don't want a new connection to start with (if this is a pipelined request, don't override the original selection)
    if (pState->iInpLen == 0)
    {
        pState->bNewConnection = FALSE;
    }

    // parse the url for kind, host, and port
    if (pStrProxy[0] == '\0')
    {
        pUrl = ProtoHttpUrlParse2(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);
    }
    else
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttp: using proxy server %s\n", pStrProxy));
        ProtoHttpUrlParse2(pStrProxy, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);
    }

    // fill in any missing info (relative url) if available
    if (_ProtoHttpApplyBaseUrl(pState, strKind, strHost, sizeof(strHost), &iPort, &iSecure, bPortSpecified) != 0)
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttp: [%p] %s %s://%s:%d%s\n", pState, _ProtoHttp_strRequestNames[eRequestType],
            iSecure ? "https" : "http", strHost, iPort, pUrl));
    }

    // determine if host, port, or security settings have changed since the previous request
    if ((iSecure != pState->iSecure) || (ds_stricmp(strHost, pState->strHost) != 0) || (iPort != pState->iPort))
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] requesting new connection -- url change to %s\n", pState, strHost));

        // reset keep-alive
        pState->iKeepAlive = pState->iKeepAliveDflt;

        // save new server/port/security state
        ds_strnzcpy(pState->strHost, strHost, sizeof(pState->strHost));
        pState->iPort = iPort;
        pState->iSecure = iSecure;

        // make sure we use a new connection
        pState->bNewConnection = TRUE;
    }

    // check to see if previous connection (if any) is still active
    if ((pState->bNewConnection == FALSE) && (ProtoSSLStat(pState->pSsl, 'stat', NULL, 0) < 0))
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] requesting new connection -- previous connection was closed\n", pState));
        pState->bNewConnection = TRUE;
    }

    // check to make sure we are in a known valid state
    if ((pState->bNewConnection == FALSE) && (eState != ST_IDLE) && (eState != ST_DONE))
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] requesting new connection -- current state of %d does not allow connection reuse\n", pState, eState));
        pState->bNewConnection = TRUE;
    }

    // if executing put/post, check to see if connection reuse on request is allowed
    if ((pState->bNewConnection == FALSE) && (pState->bReuseOnPost == FALSE) && ((eRequestType == PROTOHTTP_REQUESTTYPE_PUT) || (eRequestType == PROTOHTTP_REQUESTTYPE_PATCH) || (eRequestType == PROTOHTTP_REQUESTTYPE_POST)))
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] requesting new connection -- reuse on put/post disabled\n", pState));
        pState->bNewConnection = TRUE;
    }

    // if using a proxy server, parse original url to get target host and port for Host header
    if (pStrProxy[0] != '\0')
    {
        pUrl = ProtoHttpUrlParse2(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);
        if ((ds_stricmp(pState->strProxiedHost, strHost)) || (pState->iProxiedPort != iPort) || (pState->iProxiedSecure != iSecure))
        {
            NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] requesting new connection -- proxy change\n", pState));
            pState->bNewConnection = TRUE;
        }
        ds_strnzcpy(pState->strProxiedHost, strHost, sizeof(pState->strProxiedHost));
        pState->iProxiedPort = iPort;
        pState->iProxiedSecure = iSecure;
    }
    else
    {
        pState->strProxiedHost[0] = '\0';
    }

    // format the request header
    if ((iResult = _ProtoHttpFormatRequestHeader(pState, pUrl, strHost, iPort, iSecure, _ProtoHttp_strRequestNames[eRequestType], iDataLen)) < 0)
    {
        return(iResult);
    }

    // append data to header?
    if ((pData != NULL) && (iDataLen > 0))
    {
        // see how much data will fit into the buffer
        if (iDataLen > (pState->iInpMax - pState->iInpLen))
        {
            iDataLen = (pState->iInpMax - pState->iInpLen);
        }

        // copy data into buffer (must happen after _ProtoHttpFormatRequestHeader())
        ds_memcpy(pState->pInpBuf + pState->iInpLen, pData, (int32_t)iDataLen);
        pState->iInpLen += iDataLen;
    }
    else if (iDataLen < 0)
    {
        // for a streaming post, return no data written
        iDataLen = 0;
    }

    // set headonly status
    pState->bHeadOnly = (eRequestType == PROTOHTTP_REQUESTTYPE_HEAD) ? TRUE : FALSE;

    // handle connect flow when using a secure proxy
    if ((pStrProxy[0] != '\0') && iSecure)
    {
        _ProtoHttpFormatConnectHeader(pState, strHost, iPort);
        pState->bConnProxy = TRUE;
    }
    else
    {
        pState->bConnProxy = FALSE;
    }

    return((int32_t)iDataLen);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpSendRequest

    \Description
        Send a request (already formatted in buffer) to the server.

    \Input *pState      - reference pointer

    \Output
        None.

    \Version 05/19/2009 (jbrookes) Split from _ProtoHttpFormatRequest()
*/
/********************************************************************************F*/
static void _ProtoHttpSendRequest(ProtoHttpRefT *pState)
{
    int32_t iResult;
    char cTest;

    /* if we still want to reuse the current connection, try and receive on it and
       make sure it is in a valid state (not an error state and no data to be read) */
    if (pState->bNewConnection == FALSE)
    {
        if ((iResult = ProtoSSLRecv(pState->pSsl, &cTest, sizeof(cTest))) > 0)
        {
            NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] requesting new connection -- receive on previous connection returned data (0x%02x)\n", pState, (uint8_t)cTest));
            pState->bNewConnection = TRUE;
        }
        else if (iResult < 0)
        {
            NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] requesting new connection -- received %d error response from previous connection\n", pState, iResult));
            pState->bNewConnection = TRUE;
        }
    }

    // handle proxy connections
    if (pState->bConnProxy)
    {
        // restore user request if we're in the proxy connect flow and we're already connected
        if (!pState->bNewConnection)
        {
            NetPrintf(("protohttp: [%p] bypassing proxy connect (already connected)\n", pState));
            pState->pInpBuf = pState->pInpBufTmp;
            pState->iInpLen = pState->iInpLenTmp;
            pState->bUpgradeSSL = FALSE;
        }
        else
        {
            // if a new proxy connection, mark for SSL upgrade
            NetPrintf(("protohttp: [%p] new proxy connection\n", pState));
            pState->bUpgradeSSL = TRUE;
        }
    }

    // set connection timeout
    pState->uTimer = NetTick() + pState->uTimeout;

    // see if we need a new connection
    if (pState->bNewConnection == TRUE)
    {
        // close the existing connection, if not already closed
        _ProtoHttpClose(pState, "new connection");

        // start connect
        NetPrintfVerbose((pState->iVerbose, 2, "protohttp: [%p] connect start (tick=%u)\n", pState, NetTick()));
        ProtoSSLConnect(pState->pSsl, pState->iSecure, pState->strHost, 0, pState->iPort);
        pState->eState = ST_CONN;
        pState->bClosed = FALSE;
    }
    else
    {
        // advance state
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] reusing previous connection (keep-alive)\n", pState));
        pState->eState = ST_SEND;
    }

    // if we requested a connection close, the server may not tell us, so remember it here
    if (pState->iKeepAlive == 0)
    {
        pState->bCloseHdr = TRUE;
    }

    // count the attempt
    pState->iKeepAlive += 1;

    // call the update routine just in case operation can complete
    ProtoHttpUpdate(pState);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpRetrySendRequest

    \Description
        If the connection was a keep-alive connection and the request method was
        idempotent (see http://www.w3.org/Protocols/rfc2616/rfc2616-sec9.html#sec9.1.2
        for a definition of idempotent), we automatically re-issue the request one
        time on a fresh connection.

    \Input *pState      - reference pointer

    \Output
        uint32_t        - zero=did not reissue request, else we did

    \Version 07/14/2009 (jbrookes) Split from ProtoHttpUpdate()
*/
/********************************************************************************F*/
static uint32_t _ProtoHttpRetrySendRequest(ProtoHttpRefT *pState)
{
    // if this was not a keep-alive connection, we do not retry
    if (pState->bNewConnection == TRUE)
    {
        return(0);
    }
    // if this was a POST request, we do not retry as the method is not idempotent
    if (pState->eRequestType == PROTOHTTP_REQUESTTYPE_POST)
    {
        NetPrintf(("protohttp: cannot execute automatic retry of post request on keep-alive connection\n"));
        return(0);
    }

    // retry the connection
    NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] request failure on keep-alive connection; retrying\n", pState));
    _ProtoHttpClose(pState, "retry");

    // rewind buffer pointers to resend header
    pState->iInpLen = pState->iHdrLen;
    pState->iInpOff = 0;

    /* set keep-alive so we don't try another reconnect attempt, but we do
       request keep-alive on any further requests if this one succeeds */
    pState->iKeepAlive = 1;

    // reconnect
    ProtoSSLConnect(pState->pSsl, pState->iSecure, pState->strHost, 0, pState->iPort);
    pState->eState = ST_CONN;
    pState->bClosed = FALSE;
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpResizeBuffer

    \Description
        Resize the buffer

    \Input *pState      - reference pointer
    \Input iBufMax      - new buffer size

    \Output
        int32_t         - zero=success, else failure

    \Version 02/21/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpResizeBuffer(ProtoHttpRefT *pState, int32_t iBufMax)
{
    int32_t iCopySize;
    char *pInpBuf;

    NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] resizing input buffer from %d to %d bytes\n", pState, pState->iInpMax, iBufMax));
    if ((pInpBuf = DirtyMemAlloc(iBufMax, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttp: [%p] could not resize input buffer\n", pState));
        return(-1);
    }

    // calculate size of data to copy from old buffer to new
    if ((iCopySize = pState->iInpLen - pState->iInpOff) > iBufMax)
    {
        NetPrintf(("protohttp: [%p] warning; resize of input buffer is losing %d bytes of data\n", pState, iCopySize - iBufMax));
        iCopySize = iBufMax;
    }
    // copy valid contents of input buffer, if any, to new buffer
    ds_memcpy(pInpBuf, pState->pInpBuf+pState->iInpOff, iCopySize);

    // dispose of input buffer
    _ProtoHttpFreeInputBuf(pState);

    // update buffer variables
    pState->pInpBuf = pInpBuf;
    pState->iInpOff = 0;
    pState->iInpLen = iCopySize;
    pState->iInpMax = iBufMax;

    // clear overflow count
    pState->iInpOvr = 0;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpCompactBuffer

    \Description
        Compact the buffer

    \Input *pState      - reference pointer

    \Output
        int32_t         - amount of space freed by compaction

    \Version 07/02/2009 (jbrookes) Extracted from ProtoHttpRecv()
*/
/********************************************************************************F*/
static int32_t _ProtoHttpCompactBuffer(ProtoHttpRefT *pState)
{
    int32_t iCompacted = 0;
    // make sure it needs compacting
    if (pState->iInpOff > 0)
    {
        // compact the buffer
        if (pState->iInpOff < pState->iInpLen)
        {
            memmove(pState->pInpBuf, pState->pInpBuf+pState->iInpOff, pState->iInpLen-pState->iInpOff);
            iCompacted = pState->iInpOff;
        }
        pState->iInpLen -= pState->iInpOff;
        pState->iInpOff = 0;
        pState->bCompactRecv = FALSE;
    }
    return(iCompacted);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpParseHeader

    \Description
        Parse incoming HTTP header.  The HTTP header is presumed to be at the
        beginning of the input buffer.

    \Input  *pState     - reference pointer

    \Output
        int32_t         - negative=not ready or error, else success

    \Version 11/12/2004 (jbrookes) First Version.
*/
/********************************************************************************F*/
static int32_t _ProtoHttpParseHeader(ProtoHttpRefT *pState)
{
    char *s = pState->pInpBuf;
    char *t = pState->pInpBuf+pState->iInpLen-3;
    char strTemp[128];
    ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb;
    void *pUserData;

    // scan for blank line marking body start
    while ((s != t) && ((s[0] != '\r') || (s[1] != '\n') || (s[2] != '\r') || (s[3] != '\n')))
    {
        s++;
    }
    if (s == t)
    {
        // header is incomplete
        return(-1);
    }

    // save the head size
    pState->iHeadSize = (int32_t)(s+4-pState->pInpBuf);
    // terminate header for easy parsing
    s[2] = s[3] = 0;

    // make sure the header is valid
    if (pState->bVerifyHdr)
    {
        if (strncmp(pState->pInpBuf, "HTTP", 4))
        {
            // header is invalid
            NetPrintf(("protohttp: [%p] invalid result type\n", pState));
            pState->eState = ST_FAIL;
            return(-2);
        }
    }

    // detect if it is a 1.0 response
    pState->bHttp1_0 = !strncmp(pState->pInpBuf, "HTTP/1.0", 8);

    // parse header code
    pState->iHdrCode = ProtoHttpParseHeaderCode(pState->pInpBuf);

    #if DIRTYCODE_LOGGING
    NetPrintfVerbose((pState->iVerbose, 0, "protohttp: [%p] received %d response (%d bytes)\n", pState, pState->iHdrCode, pState->iHeadSize));
    if (pState->iVerbose > 1)
    {
        NetPrintWrap(pState->pInpBuf, 80);
    }
    #endif

    // parse content-length field
    if (ProtoHttpGetHeaderValue(pState, pState->pInpBuf, "content-length", strTemp, sizeof(strTemp), NULL) != -1)
    {
        pState->iBodySize = ds_strtoll(strTemp, NULL, 10);
        pState->bChunked = FALSE;
    }
    else
    {
        pState->iBodySize = -1;
    }

    // parse last-modified header
    if (ProtoHttpGetHeaderValue(pState, pState->pInpBuf, "last-modified", strTemp, sizeof(strTemp), NULL) != -1)
    {
        pState->iHdrDate = (int32_t)ds_strtotime(strTemp);
    }
    else
    {
        pState->iHdrDate = 0;
    }

    // parse transfer-encoding header
    if (ProtoHttpGetHeaderValue(pState, pState->pInpBuf, "transfer-encoding", strTemp, sizeof(strTemp), NULL) != -1)
    {
        pState->bChunked = !ds_stricmp(strTemp, "chunked");
    }

    // parse connection header
    if (pState->bCloseHdr == FALSE)
    {
        ProtoHttpGetHeaderValue(pState, pState->pInpBuf, "connection", strTemp, sizeof(strTemp), NULL);
        pState->bCloseHdr = !ds_stricmp(strTemp, "close");
        // if server is closing the connection and we are expecting subsequent piped results, we should not expect to get them
        if (pState->bCloseHdr && (pState->iPipedRequests > 0))
        {
            NetPrintfVerbose((pState->iVerbose, 0, "protohttp: [%p] lost %d piped requests due to server connection-close request\n", pState, pState->iPipedRequests));
            pState->iPipedRequests = 0;
            pState->bPipedRequestsLost = TRUE;
        }
    }

    // note if it is an informational header
    pState->bInfoHdr = PROTOHTTP_GetResponseClass(pState->iHdrCode) == PROTOHTTP_RESPONSE_INFORMATIONAL;

    // copy header to header cache
    ds_strnzcpy(pState->strHdr, pState->pInpBuf, sizeof(pState->strHdr));

    // request level callback takes priority to global
    if ((pReceiveHeaderCb = pState->pReqReceiveHeaderCb) != NULL)
    {
        pUserData = pState->pUserData;
    }
    else
    {
        pReceiveHeaderCb = pState->pReceiveHeaderCb;
        pUserData = pState->pCallbackRef;
    }

    // trigger recv header user callback, if specified (and if this wasn't a proxy connect request)
    if ((pReceiveHeaderCb != NULL) && (!pState->bConnProxy || (pState->pInpBufTmp == NULL)))
    {
        pReceiveHeaderCb(pState, pState->pInpBuf, (uint32_t)strlen(pState->pInpBuf), pUserData);
    }

    // remove header from input buffer
    pState->iInpOff = pState->iHeadSize;
    pState->iInpCnt = pState->iInpLen - pState->iHeadSize;

    // header successfully parsed
    return(0);
}

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function _ProtoHttpDisplayHeader

    \Description
        Display header to debug output [DEBUG ONLY]

    \Input  *pState     - reference pointer

    \Version 05/03/2010 (jbrookes) Refactored out of ProtoHttpUpdate()
*/
/********************************************************************************F*/
static void _ProtoHttpDisplayHeader(ProtoHttpRefT *pState)
{
    // if we just sent a header, display header to debug output
    if (pState->iVerbose > 1)
    {
        int32_t iRequestType;
        for (iRequestType = 0; iRequestType < PROTOHTTP_NUMREQUESTTYPES; iRequestType += 1)
        {
            if (!strncmp(pState->pInpBuf, _ProtoHttp_strRequestNames[iRequestType], strlen(_ProtoHttp_strRequestNames[iRequestType])))
            {
                char *pHdrEnd = pState->pInpBuf + pState->iHdrLen;
                char cHdrChr = *pHdrEnd;
                *pHdrEnd = '\0';
                NetPrintf(("protohttp: [%p] sent request:\n", pState));
                NetPrintfVerbose((pState->iVerbose, 2, "protohttp: [%p] tick=%u\n", pState, NetTick()));
                NetPrintWrap(pState->pInpBuf, 80);
                *pHdrEnd = cHdrChr;
                break;
            }
        }
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function _ProtoHttpProcessInfoHeader

    \Description
        Handles an informational response header (response code=1xx)

    \Input  *pState     - reference pointer

    \Output
        None.

    \Version 05/15/2008 (jbrookes) First Version.
*/
/********************************************************************************F*/
static void _ProtoHttpProcessInfoHeader(ProtoHttpRefT *pState)
{
    // ignore the response
    NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] ignoring %d response from server\n", pState, pState->iHdrCode));

    // remove header from input buffer
    memmove(pState->pInpBuf, pState->pInpBuf+pState->iInpOff, pState->iInpLen-pState->iInpOff);
    pState->iInpLen -= pState->iInpOff;
    // reset processing offset
    pState->iInpOff = 0;

    // reset state to process next header
    pState->eState = ST_HEAD;
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpProcessRedirect

    \Description
        Handle redirection header (response code=3xx)

    \Input *pState      - reference pointer

    \Notes
        A maximum of PROTOHTTP_MAXREDIRECT redirections is allowed.  Any further
        redirection attempts will result in a failure state.  A redirection
        Location url is limited based on the size of the http receive buffer.

        Auto-redirection is implemented as specified by RFC:
        (http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.3);
        if auto-redirection is not performed, processing ends and it is the
        responsibility of the application to recognize the 3xx result code
        and handle it accordingly.

        Auto-redirection can be disabled by setting the maximum number of
        redirections allowed to zero.

    \Version 11/12/2004 (jbrookes) First Version.
*/
/********************************************************************************F*/
static void _ProtoHttpProcessRedirect(ProtoHttpRefT *pState)
{
    char strHost[sizeof(pState->strHost)], strKind[PROTOHTTPUTIL_SCHEME_MAX];
    int32_t iPort, iResult, iSecure, iUrlLen;
    char *pUrlBuf;

    // do not auto-redirect multiplechoices or notmodified responses
    if ((pState->iHdrCode == PROTOHTTP_RESPONSE_MULTIPLECHOICES) || (pState->iHdrCode == PROTOHTTP_RESPONSE_NOTMODIFIED))
    {
        return;
    }
    // do not auto-redirect responses that are not head or get requests, and are not a SEEOTHER response
    if ((pState->eRequestType != PROTOHTTP_REQUESTTYPE_GET) && (pState->eRequestType != PROTOHTTP_REQUESTTYPE_HEAD))
    {
        /* As per HTTP 1.1 RFC, 303 SEEOTHER POST requests may be auto-redirected to a GET requset.  302 FOUND responses
           to a POST request are not supposed to be auto-redirected; however, there is a note in the RFC indicating that
           this is a common client behavior, and it is additionally a common server behavior to use 302 even when automatic
           redirection is intended, as some clients do not support 303 SEEOTHER.  Therefore, we perform auto-redirection
           on 302 FOUND responses to POST requests with a GET request for compatibility with servers that choose this
           behavior */
        if ((pState->iHdrCode == PROTOHTTP_RESPONSE_FOUND) || (pState->iHdrCode == PROTOHTTP_RESPONSE_SEEOTHER))
        {
            pState->eRequestType = PROTOHTTP_REQUESTTYPE_GET;
            pState->iPostSize = 0;
        }
        else
        {
            return;
        }
    }

    // get size of location header
    if ((iUrlLen = ProtoHttpGetLocationHeader(pState, pState->pInpBuf, NULL, 0, NULL)) <= 0)
    {
        NetPrintf(("protohttp: [%p] no location included in redirect header\n", pState));
        pState->eState = ST_FAIL;
        return;
    }

    // get url at the end of input buffer
    pUrlBuf = pState->pInpBuf + pState->iInpMax - iUrlLen;
    if (ProtoHttpGetLocationHeader(pState, pState->pInpBuf, pUrlBuf, iUrlLen, NULL) != 0)
    {
        NetPrintf(("protohttp: [%p] failed to get location header url", pState));
        pState->eState = ST_FAIL;
        return;
    }

    // parse url for protocol
    ProtoHttpUrlParse(pUrlBuf, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure);
    // only auto-redirect if http/s protocol
    if ((ds_stricmp(strKind, "https") != 0) && (ds_stricmp(strKind, "http") != 0))
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttp: [%p] skipping auto-redirection of non-http protocol '%s'\n", pState, strKind));
        return;
    }

    // process based on max redirections allowed; zero=disabled
    if (pState->iMaxRedirect == 0)
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttp: [%p] auto-redirection disabled\n", pState));
        return;
    }
    else if (++pState->iNumRedirect > pState->iMaxRedirect)
    {
        NetPrintf(("protohttp: [%p] maximum number of redirections (%d) exceeded\n", pState, pState->iMaxRedirect));
        pState->eState = ST_FAIL;
        return;
    }

    // close connection?
    if (pState->bCloseHdr)
    {
        _ProtoHttpClose(pState, "server request");
    }

    // clear piped result count
    pState->iPipedRequests = 0;
    pState->bPipedRequestsLost = FALSE;

    // format redirection request
    if ((iResult = _ProtoHttpFormatRequest(pState, pUrlBuf, NULL, 0, pState->eRequestType)) < 0)
    {
        NetPrintf(("protohttp: redirect header format request failed\n"));
        pState->eState = ST_FAIL;
        return;
    }
    // send redirection request
    _ProtoHttpSendRequest(pState);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpChunkProcess

    \Description
        Process output if chunked encoding.

    \Input *pState      - reference pointer
    \Input iBufMax      - maximum number of bytes to return (buffer size)

    \Output
        int32_t         - number of bytes available

    \Notes
        Does not support anything but chunked encoding.  Does not support optional
        end-transfer header (anything past the terminating 0 chunk is discarded).

    \Version 04/05/2005 (jbrookes) First Version.
*/
/********************************************************************************F*/
static int32_t _ProtoHttpChunkProcess(ProtoHttpRefT *pState, int32_t iBufMax)
{
    int32_t iChkSize, iLen, iLenInt;

    // if no new data, bail
    if (pState->iInpLen == pState->iInpOff)
    {
        return(0);
    }

    // see if we are starting a new chunk
    if (pState->iChkLen == 0)
    {
        char *s = pState->pInpBuf+pState->iInpOff, *s2;
        char *t = pState->pInpBuf+pState->iInpLen-1;

        // make sure we have a complete chunk header
        for (s2=s; (s2 < t) && ((s2[0] != '\r') || (s2[1] != '\n')); s2++)
            ;
        if (s2 == t)
        {
            if (pState->iInpLen == pState->iInpMax)
            {
                // tell ProtoHttpRecv() to compact recv buffer next time around
                pState->bCompactRecv = TRUE;
            }
            return(0);
        }

        // get the chunk length
        if ((pState->iChkLen = (int32_t)strtol(s, NULL, 16)) == 0)
        {
            // terminating chunk - clear the buffer and set state to done
            NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] parsed end chunk\n", pState));
            pState->iInpOff += s2-s+4; // remove chunk header plus terminating crlf
            pState->iBodySize = pState->iBodyRcvd;
            pState->eState = ST_DONE;

            // return no data
            return(0);
        }
        else
        {
            NetPrintfVerbose((pState->iVerbose, 2, "protohttp: [%p] parsed chunk size=%d\n", pState, pState->iChkLen));
        }

        // remove header from input
        pState->iInpOff += s2-s+2;
    }

    // calculate length
    iLenInt = pState->iInpLen - pState->iInpOff;
    iLen = (iLenInt > iBufMax) ? iBufMax : iLenInt;

    // have we got all the data?
    if (iLen >= pState->iChkLen)
    {
        /* got chunk and trailer, so consume it.  we use the internal data size rather than the data size
           that is constrained by the user buffer size as the user isn't going to read the chunk data and
           therefore must not be limited by a requirement to read it.  not doing so results in the user not
           being able to consume the whole chunk if their buffer does not have room for the chunk trailer */
        if (iLenInt >= (pState->iChkLen+2))
        {
            // reset chunk length
            iChkSize = pState->iChkLen;
            pState->iChkLen = 0;
        }
        else
        {
            if (pState->iChkLen > 1)
            {
                /* consume data and compact recv buffer to make room for the trailer; however we leave one
                   byte unread to cover the case where the chunk trailer spans a recv boundary, and where the
                   next byte is not available to read from the network.  in such a case we wait until the chunk
                   trailer is completely available before finishing the chunk, otherwise we end up reading the
                   one byte of chunk trailer as if it were the next chunk size */
                iChkSize = pState->iChkLen-1;
                pState->iChkLen -= iChkSize;
            }
            else
            {
                NetPrintfVerbose((pState->iVerbose, 0, "protohttp: [%p] waiting for chunk trailer\n", pState));
                iChkSize = 0;
            }
            // tell ProtoHttpRecv() to compact recv buffer next time around
            pState->bCompactRecv = TRUE;
        }
    }
    else
    {
        iChkSize = iLen;
        pState->iChkLen -= iChkSize;
    }

    return(iChkSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpSend

    \Description
        Try and send some data.  If data is sent, the timout value is updated.

    \Input  *pState     - reference pointer
    \Input  *pStrBuf    - pointer to buffer to send from
    \Input  iSize       - amount of data to try and send

    \Output
        int32_t         - negative=error, else number of bytes sent

    \Version 11/18/2004 (jbrookes) First Version.
*/
/********************************************************************************F*/
static int32_t _ProtoHttpSend(ProtoHttpRefT *pState, const char *pStrBuf, int32_t iSize)
{
    int32_t iResult;

    // try and send some data
    if ((iResult = ProtoSSLSend(pState->pSsl, pStrBuf, iSize)) > 0)
    {
        #if DIRTYCODE_LOGGING
        if (pState->iVerbose > 2)
        {
            NetPrintf(("protohttp: [%p] sent %d bytes\n", pState, iResult));
        }
        if (pState->iVerbose > 3)
        {
            NetPrintMem(pStrBuf, iResult, "http-send");
        }
        #endif

        // sent data, so update timeout
        pState->uTimer = NetTick() + pState->uTimeout;
    }
    else if (iResult < 0)
    {
        NetPrintf(("protohttp: [%p] error %d sending %d bytes\n", pState, iResult, iSize));
        pState->eState = ST_FAIL;
        pState->iSslFail = ProtoSSLStat(pState->pSsl, 'fail', NULL, 0);
        pState->iHresult = ProtoSSLStat(pState->pSsl, 'hres', NULL, 0);
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpSendBuff

    \Description
        Send data queued in buffer.

    \Input  *pState     - reference pointer

    \Output
        int32_t         - negative=error, else number of bytes sent

    \Version 01/29/2010 (jbrookes) First Version.
*/
/********************************************************************************F*/
static int32_t _ProtoHttpSendBuff(ProtoHttpRefT *pState)
{
    int32_t iResult, iSentSize = 0;
    if ((iResult = _ProtoHttpSend(pState, pState->pInpBuf+pState->iInpOff, pState->iInpLen)) > 0)
    {
        pState->iInpOff += iResult;
        pState->iInpLen -= iResult;
        if (pState->iInpLen == 0)
        {
            pState->iInpOff = 0;
        }
        iSentSize = iResult;
    }
    else if (iResult < 0)
    {
        NetPrintf(("protohttp: [%p] failed to send request data (err=%d)\n", pState, iResult));
        pState->iInpLen = 0;
        pState->eState = ST_FAIL;
        iSentSize = -1;
    }
    return(iSentSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpFormatChunk

    \Description
        Format source data into chunked format, ready to be sent.

    \Input  *pState     - reference pointer
    \Input  *pStrBuf    - pointer to buffer to send from
    \Input  iSize       - amount of data to try and send (zero for stream completion)

    \Output
        int32_t         - negative=space required, else number of bytes of user data sent

    \Notes
        Space is reserved for an end chunk to be buffered so the ProtoHttpSend()
        call indicating the stream is complete does not ever need to be retried.

    \Version 03/21/2014 (jbrookes) Refactored from _ProtoHttpSendChunk()
*/
/********************************************************************************F*/
static int32_t _ProtoHttpFormatChunk(ProtoHttpRefT *pState, const char *pStrBuf, int32_t iSize)
{
    char *pInpBuf = pState->pInpBuf + pState->iInpLen + pState->iInpOff;
    int32_t iInpMax = pState->iInpMax - pState->iInpLen - pState->iInpOff;
    int32_t iSendSize, iSentSize;

    // make sure we have enough room for a max chunk header, chunk data, *and* possible end chunk
    if ((iSendSize = iSize) > 0)
    {
        if (iSendSize > (iInpMax - (6+2+2 + 1+2+2)))
        {
            iSendSize = (iInpMax - (6+2+2 + 1+2+2));
        }
    }
    else
    {
        pState->iPostSize = 0;
    }

    // if we have room to buffer chunk data, or this is the end chunk, do it
    if ((iSendSize > 0) || (iSize == 0))
    {
        // format chunk into buffer
        iSentSize = ds_snzprintf(pInpBuf, iInpMax, "%x\r\n", iSendSize);
        if (iSendSize > 0)
        {
            ds_memcpy(pInpBuf+iSentSize, pStrBuf, iSendSize);
            iSentSize += iSendSize;
        }
        iSentSize += ds_snzprintf(pInpBuf+iSentSize, iInpMax, "\r\n");

        // add chunk to length
        pState->iInpLen += iSentSize;
    }
    else
    {
        iSendSize = 0;
    }
    // return size of data buffered to caller
    return(iSendSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpSendChunk

    \Description
        Send the specified data using chunked transfer encoding.

    \Input  *pState     - reference pointer
    \Input  *pStrBuf    - pointer to buffer to send from
    \Input  iSize       - amount of data to try and send (zero for stream completion)

    \Output
        int32_t         - negative=error, else number of bytes of user data sent

    \Notes
        $$TODO - update
        Unlike _ProtoHttpSend(), which simply passes the data to ProtoSSL and returns
        the amount of data that was accepted, this function buffers one or more chunks
        of data, up to the buffer limit.  It tries to send the buffered data immediately,
        however if the band the data is sent by ProtoHttpUpdate().
        This guarantees that a chunk will be sent correctly even if a partial send
        occurs.

    \Version 05/07/2008 (jbrookes) First Version.
*/
/********************************************************************************F*/
static int32_t _ProtoHttpSendChunk(ProtoHttpRefT *pState, const char *pStrBuf, int32_t iSize)
{
    int32_t iBuffSize, iCompSize;

    // try to buffer chunk data
    if ((iBuffSize = _ProtoHttpFormatChunk(pState, pStrBuf, iSize)) < 0)
    {
        // try compacting send buffer to make room for more data
        if ((iCompSize = _ProtoHttpCompactBuffer(pState)) > 0)
        {
            // buffer was compacted, retry
            NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] compacted send buffer (%d bytes)\n", pState, iCompSize));
            // try to buffer chunk data again
            iBuffSize = _ProtoHttpFormatChunk(pState, pStrBuf, iSize);
        }
    }

    // try to send any buffered data
    _ProtoHttpSendBuff(pState);

    // return buffered data size to caller
    return((iBuffSize > 0) ? iBuffSize : 0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpRecvData

    \Description
        Try and receive some data.  If data is received, the timout value is
        updated.

    \Input *pState      - reference pointer
    \Input *pStrBuf     - [out] pointer to buffer to receive into
    \Input iSize        - amount of data to try and receive

    \Output
        int32_t         - negative=error, else success

    \Version 11/12/2004 (jbrookes) First Version.
*/
/********************************************************************************F*/
static int32_t _ProtoHttpRecvData(ProtoHttpRefT *pState, char *pStrBuf, int32_t iSize)
{
    // if we have no buffer space, don't try to receive
    if (iSize == 0)
    {
        return(0);
    }

    // try and receive some data
    if ((pState->iRecvRslt = ProtoSSLRecv(pState->pSsl, pStrBuf, iSize)) > 0)
    {
        #if DIRTYCODE_LOGGING
        if (pState->iVerbose > 2)
        {
            NetPrintf(("protohttp: [%p] recv %d bytes\n", pState, pState->iRecvRslt));
        }
        if (pState->iVerbose > 3)
        {
            NetPrintMem(pStrBuf, pState->iRecvRslt, "http-recv");
        }
        #endif

        // received data, so update timeout
        pState->uTimer = NetTick() + pState->uTimeout;
    }
    return(pState->iRecvRslt);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpHeaderRecvFirstLine

    \Description
        Try and receive the first line of the response header.  We receive into
        the header cache buffer directly so we can avoid conflicting with possible
        use of the input buffer sending streaming data.  This allows us to receive
        while we are sending, which is useful in detecting situations where the
        server responsds with an error in the middle of a streaming post transaction.

    \Input *pState          - reference pointer

    \Output
        int32_t             - negative=error, zero=pending, else success

    \Version 01/13/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpHeaderRecvFirstLine(ProtoHttpRefT *pState)
{
    int32_t iResult;
    for (iResult = 1; (iResult == 1) && (pState->iHdrOff < 64);  ) // hard-coded max limit in case this is not http
    {
        if ((iResult = _ProtoHttpRecvData(pState, pState->strHdr+pState->iHdrOff, 1)) == 1)
        {
            pState->iHdrOff += 1;
            if ((pState->strHdr[pState->iHdrOff-2] == '\r') && (pState->strHdr[pState->iHdrOff-1] == '\n'))
            {
                // we've received the first line of the response header... get response code
                int32_t iHdrCode = ProtoHttpParseHeaderCode(pState->strHdr);

                /* if this is a streaming post, check the response code.  we do this because a server might
                   abort a streaming upload prematurely if the file size is too large and this allows the
                   client to stop sending gracefully; typically the server will issue a forceful reset if
                   the client keeps sending data after being sent notification */
                if ((pState->iPostSize == -1) && (iHdrCode != PROTOHTTP_RESPONSE_CONTINUE) && (PROTOHTTP_GetResponseClass(iHdrCode) != PROTOHTTP_RESPONSE_SUCCESSFUL))
                {
                    NetPrintf(("protohttp [%p] got unexpected response %d during streaming post; aborting send\n", pState, iHdrCode));
                    pState->iPostSize = 0;
                }
                break;
            }
        }
    }
    // return result unless we are in the middle of a streaming post
    return((pState->iPostSize != -1) ? iResult : 0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpHeaderProcess

    \Description
        Check for header completion and process header data

    \Input *pState          - reference pointer

    \Version 05/03/2012 (jbrookes) Refactored out of ProtoHttpUpdate()
*/
/********************************************************************************F*/
static void _ProtoHttpHeaderProcess(ProtoHttpRefT *pState)
{
    // try parsing header
    if (_ProtoHttpParseHeader(pState) < 0)
    {
        // was there a prior SOCKERR_CLOSED error?
        if (pState->iRecvRslt < 0)
        {
            NetPrintf(("protohttp: [%p] ST_HEAD append got ST_FAIL (err=%d, len=%d)\n", pState, pState->iRecvRslt, pState->iInpLen));
            pState->eState = ST_FAIL;
        }
        // if the buffer is full, we don't have enough room to receive the header
        if (pState->iInpLen == pState->iInpMax)
        {
            if (pState->iInpOvr == 0)
            {
                NetPrintfVerbose((pState->iVerbose, 0, "protohttp: [%p] input buffer too small for response header\n", pState));
            }
            pState->iInpOvr = pState->iInpLen+1;
        }
        return;
    }

    /* workaround for non-compliant 1.0 servers - some 1.0 servers specify a
       Content Length of zero invalidly.  if the response is a 1.0 response
       and the Content Length is zero, and we've gotten body data, we set
       the Content Length to -1 (indeterminate) */
    if ((pState->bHttp1_0 == TRUE) && (pState->iBodySize == 0) && (pState->iInpCnt > 0))
    {
        pState->iBodySize = -1;
    }

    // handle final processing
    if ((pState->bHeadOnly == TRUE) || (pState->iHdrCode == PROTOHTTP_RESPONSE_NOCONTENT) || (pState->iHdrCode == PROTOHTTP_RESPONSE_NOTMODIFIED))
    {
        // only needed the header (or response did not include a body; see HTTP RFC 1.1 section 4.4) -- all done
        pState->eState = ST_DONE;
    }
    else if ((pState->iBodySize >= 0) && (pState->iInpCnt >= pState->iBodySize))
    {
        // we got entire body with header -- all done
        pState->eState = ST_DONE;
    }
    else
    {
        // wait for rest of body
        pState->eState = ST_BODY;
    }

    // handle response code?
    if (PROTOHTTP_GetResponseClass(pState->iHdrCode) == PROTOHTTP_RESPONSE_INFORMATIONAL)
    {
        _ProtoHttpProcessInfoHeader(pState);
    }
    else if (PROTOHTTP_GetResponseClass(pState->iHdrCode) == PROTOHTTP_RESPONSE_REDIRECTION)
    {
        _ProtoHttpProcessRedirect(pState);
    }

    /* handle upgrade to ssl after connect; note we have to check state because a redirection processed immediately
       above can issue a new proxy connection requiring an upgrade, but it won't be in the right state yet */
    if (pState->bUpgradeSSL && (pState->eState == ST_BODY))
    {
        if (ProtoSSLControl(pState->pSsl, 'secu', 0, 0, NULL) < 0)
        {
            NetPrintf(("protossl: failed to upgrade connection to SSL\n"));
            pState->eState = ST_FAIL;
            return;
        }
        
        NetPrintf(("protossl: upgrading connection to SSL\n"));
        ProtoSSLControl(pState->pSsl, 'host', 0, 0, pState->strProxiedHost);

        // restore and send user request
        pState->pInpBuf = pState->pInpBufTmp;
        pState->iInpLen = pState->iInpLenTmp;
        pState->iInpOff = 0;
        pState->pInpBufTmp = NULL;
        pState->iInpLenTmp = 0;

        // send the request
        pState->bNewConnection = FALSE;
        pState->eState = ST_CONN;

        // upgrade completed
        pState->bUpgradeSSL = FALSE;
        // set keep-alive
        pState->iKeepAlive += 1;
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpRecvBody

    \Description
        Attempt to recevive and process body data

    \Input *pState          - reference pointer

    \Output
        int32_t             - zero=no data available

    \Version 05/03/2012 (jbrookes) Refactored out of ProtoHttpUpdate()
*/
/********************************************************************************F*/
static int32_t _ProtoHttpRecvBody(ProtoHttpRefT *pState)
{
    int32_t iResult;

    // reset pointers if needed
    if ((pState->iInpLen > 0) && (pState->iInpOff == pState->iInpLen))
    {
        pState->iInpOff = pState->iInpLen = 0;
    }

    // check for data
    iResult = pState->iInpMax - pState->iInpLen;
    if (iResult <= 0)
    {
        // always return zero bytes since buffer is full
        iResult = 0;
    }
    else
    {
        // try to add to buffer
        iResult = _ProtoHttpRecvData(pState, pState->pInpBuf+pState->iInpLen, iResult);
    }
    if (iResult == 0)
    {
        return(0);
    }

    // check for connection close
    if ((iResult == SOCKERR_CLOSED) && ((pState->iBodySize == -1) || (pState->iBodySize == pState->iInpCnt)))
    {
        if (!pState->bChunked)
        {
            pState->iBodySize = pState->iInpCnt;
        }
        pState->bCloseHdr = TRUE;
        pState->eState = ST_DONE;
    }
    else if (iResult > 0)
    {
        // add to buffer
        pState->iInpLen += iResult;
        pState->iInpCnt += iResult;

        // check for end of body
        if ((pState->iBodySize >= 0) && (pState->iInpCnt >= pState->iBodySize))
        {
            pState->eState = ST_DONE;
        }
    }
    else if (iResult < 0)
    {
        NetPrintf(("protohttp: [%p] ST_FAIL (err=%d)\n", (uintptr_t)pState, iResult));
        pState->eState = ST_FAIL;
        pState->iSslFail = ProtoSSLStat(pState->pSsl, 'fail', NULL, 0);
        pState->iHresult = ProtoSSLStat(pState->pSsl, 'hres', NULL, 0);
    }

    return(1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpRecv

    \Description
        Return the actual url data.

    \Input *pState  - reference pointer
    \Input *pBuffer - buffer to store data in
    \Input iBufMin  - minimum number of bytes to return (returns zero if this number is not available)
    \Input iBufMax  - maximum number of bytes to return (buffer size)

    \Output
        int32_t     - negative=error, zero=no data available or bufmax <= 0, positive=number of bytes read

    \Version 04/12/2000 (gschaefer) First Version
*/
/********************************************************************************F*/
static int32_t _ProtoHttpRecv(ProtoHttpRefT *pState, char *pBuffer, int32_t iBufMin, int32_t iBufMax)
{
    int32_t iLen;

    // early out for failure result
    if (pState->eState == ST_FAIL)
    {
        int32_t iResult = PROTOHTTP_RECVFAIL;
        if (pState->iNumRedirect > pState->iMaxRedirect)
        {
            iResult = PROTOHTTP_RECVRDIR;
        }
        else if (pState->bTimeout)
        {
            iResult = PROTOHTTP_TIMEOUT;
        }
        return(iResult);
    }
    // check for input buffer too small for header
    if (pState->iInpOvr > 0)
        return(PROTOHTTP_MINBUFF);
    // waiting for data
    if ((pState->eState != ST_BODY) && (pState->eState != ST_DONE))
        return(PROTOHTTP_RECVWAIT);

    // if they only wanted head, thats all they get
    if (pState->bHeadOnly == TRUE)
        return(PROTOHTTP_RECVHEAD);

    // if they are querying only for done state when no more data is available to be read
    if((iBufMax == 0) && ((pState->eState == ST_DONE) && (pState->iBodyRcvd == pState->iBodySize)))
        return(PROTOHTTP_RECVDONE);

    // make sure range is valid
    if (iBufMax < 1)
        return(0);

    // clamp the range
    if (iBufMin < 1)
        iBufMin = 1;
    if (iBufMax < iBufMin)
        iBufMax = iBufMin;
    if (iBufMin > pState->iInpMax)
        iBufMin = pState->iInpMax;
    if (iBufMax > pState->iInpMax)
        iBufMax = pState->iInpMax;

    // see if we need to shift buffer
    if ((iBufMin > pState->iInpMax-pState->iInpOff) || (pState->bCompactRecv == TRUE))
    {
        // compact receive buffer
        _ProtoHttpCompactBuffer(pState);
        // give chance to get more data
        _ProtoHttpRecvBody(pState);
    }

    // figure out how much data is available
    if (pState->bChunked == TRUE)
    {
        iLen = _ProtoHttpChunkProcess(pState, iBufMax);
    }
    else if ((iLen = (pState->iInpLen - pState->iInpOff)) > iBufMax)
    {
        iLen = iBufMax;
    }

    // check for end of data
    if ((iLen == 0) && (pState->eState == ST_DONE))
    {
        return(PROTOHTTP_RECVDONE);
    }

    // special check for responses with trailing piped data
    if (pState->iPipedRequests > 0)
    {
        // check for end of this transaction
        if (pState->iBodyRcvd == pState->iBodySize)
        {
            return(PROTOHTTP_RECVDONE);
        }
        // clamp available data to body size
        if ((pState->iBodySize != -1) && (iLen > (int32_t)(pState->iBodySize - pState->iBodyRcvd)))
        {
            iLen = (int32_t)(pState->iBodySize - pState->iBodyRcvd);
        }
    }

    // see if there is enough to return
    if ((iLen >= iBufMin) || (pState->iInpCnt == pState->iBodySize))
    {
        // return data to caller
        if (pBuffer != NULL)
        {
            ds_memcpy(pBuffer, pState->pInpBuf+pState->iInpOff, iLen);

            #if DIRTYCODE_LOGGING
            if (pState->iVerbose > 3)
            {
                NetPrintf(("protohttp: [%p] read %d bytes\n", pState, iLen));
            }
            if (pState->iVerbose > 4)
            {
                NetPrintMem(pBuffer, iLen, "http-read");
            }
            #endif
        }
        pState->iInpOff += iLen;
        pState->iBodyRcvd += iLen;

        // if we're at the end of a chunk, skip trailing crlf
        if ((pState->bChunked == TRUE) && (pState->iChkLen == 0))
        {
            pState->iInpOff += 2;
        }

        // return bytes read
        return(iLen);
    }

    // nothing available
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpWriteCbProcess

    \Description
        User write callback processing, if write callback is set

    \Input *pState          - reference pointer

    \Version 05/03/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpWriteCbProcess(ProtoHttpRefT *pState)
{
    ProtoHttpWriteCbInfoT WriteCbInfo;
    int32_t iResult;

    ds_memclr(&WriteCbInfo, sizeof(WriteCbInfo));
    WriteCbInfo.eRequestType = pState->eRequestType;
    WriteCbInfo.eRequestResponse = (ProtoHttpResponseE)pState->iHdrCode;

    /* download more data when the following are true: we are in the body state or we are in the done state but we haven't received
       everything. note, the body size is negative for chunked transfers when we haven't processed the end chunk. */
    if ((pState->eState == ST_BODY) || ((pState->eState == ST_DONE) && ((pState->iBodySize < 0) || (pState->iBodyRcvd < pState->iBodySize))))
    {
        char strTempRecv[1024];
        while ((iResult = _ProtoHttpRecv(pState, strTempRecv, 1, sizeof(strTempRecv))) > 0)
        {
            pState->pWriteCb(pState, &WriteCbInfo, strTempRecv, iResult, pState->pUserData);
        }
    }
    
    if (pState->eState > ST_BODY)
    {
        if (pState->eState == ST_DONE)
        {
            pState->pWriteCb(pState, &WriteCbInfo, "", pState->bHeadOnly ? PROTOHTTP_HEADONLY : PROTOHTTP_RECVDONE, pState->pUserData);
        }
        if (pState->eState == ST_FAIL)
        {
            pState->pWriteCb(pState, &WriteCbInfo, "", pState->bTimeout ? PROTOHTTP_TIMEOUT : PROTOHTTP_RECVFAIL, pState->pUserData);
        }
        pState->pWriteCb = NULL;
        pState->pReqCustomHeaderCb = NULL;
        pState->pReqReceiveHeaderCb = NULL;
        pState->pUserData = NULL;
    }
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoHttpCreate

    \Description
        Allocate module state and prepare for use

    \Input iBufSize     - length of receive buffer

    \Output
        ProtoHttpRefT * - pointer to module state, or NULL

    \Version 04/12/2000 (gschaefer) First Version
*/
/********************************************************************************F*/
ProtoHttpRefT *ProtoHttpCreate(int32_t iBufSize)
{
    ProtoHttpRefT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // clamp the buffer size
    if (iBufSize < 4096)
    {
        iBufSize = 4096;
    }

    // allocate the resources
    if ((pState = DirtyMemAlloc(sizeof(*pState), PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttp: unable to allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pState, sizeof(*pState));
    // save memgroup (will be used in ProtoHttpDestroy)
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;

    // allocate ssl module
    if ((pState->pSsl = ProtoSSLCreate()) == NULL)
    {
        NetPrintf(("protohttp: [%p] unable to allocate ssl module\n", pState));
        ProtoHttpDestroy(pState);
        return(NULL);
    }
    if ((pState->pInpBuf = DirtyMemAlloc(iBufSize, PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttp: [%p] unable to allocate protohttp buffer\n", pState));
        ProtoHttpDestroy(pState);
        return(NULL);
    }

    // init crit
    NetCritInit(&pState->HttpCrit, "ProtoHttp");

    // save parms & set defaults
    pState->eState = ST_IDLE;
    pState->iInpMax = iBufSize;
    pState->uTimeout = PROTOHTTP_TIMEOUT_DEFAULT;
    pState->bVerifyHdr = TRUE;
    pState->iVerbose = 1;
    pState->iMaxRedirect = PROTOHTTP_MAXREDIRECT;

    // return the state
    return(pState);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpCallback

    \Description
        Set header callbacks.

    \Input *pState          - module state
    \Input *pCustomHeaderCb - pointer to custom send header callback (may be NULL)
    \Input *pReceiveHeaderCb- pointer to recv header callback (may be NULL)
    \Input *pCallbackRef    - user-supplied callback ref (may be NULL)

    \Notes
        The ProtHttpCustomHeaderCbt callback is used to allow customization of
        the HTTP header before sending.  It is more powerful than the append
        header functionality, allowing to make changes to any part of the header
        before it is sent.  The callback should return a negative code if an error
        occurred, zero can be returned if the application wants ProtoHttp to
        calculate the header size, or the size of the header can be returned if
        the application has already calculated it.  The header should *not* be
        terminated with the double \\r\\n that indicates the end of the entire
        header, as protohttp appends itself.

        The ProtoHttpReceiveHeaderCbT callback is used to view the header
        immediately on reception.  It can be used when the built-in header
        cache (retrieved with ProtoHttpStatus('htxt') is too small to hold
        the entire header received.  It is also possible with this method
        to view redirection response headers that cannot be retrieved
        normally.  This can be important if, for example, the application
        wishes to attach new cookies to a redirection response.  The
        custom response header and custom header callback can be used in
        conjunction to implement this type of functionality.

    \Version 1.0 02/16/2007 (jbrookes) First Version
*/
/********************************************************************************F*/
void ProtoHttpCallback(ProtoHttpRefT *pState, ProtoHttpCustomHeaderCbT *pCustomHeaderCb, ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb, void *pCallbackRef)
{
    pState->pCustomHeaderCb = pCustomHeaderCb;
    pState->pReceiveHeaderCb = pReceiveHeaderCb;
    pState->pCallbackRef = pCallbackRef;
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpDestroy

    \Description
        Destroy the module and release its state

    \Input *pState      - reference pointer

    \Output
        None.

    \Version 04/12/2000 (gschaefer) First Version
*/
/********************************************************************************F*/
void ProtoHttpDestroy(ProtoHttpRefT *pState)
{
    if (pState->pSsl != NULL)
    {
        ProtoSSLDestroy(pState->pSsl);
    }
    _ProtoHttpFreeInputBuf(pState);
    if (pState->pAppendHdr != NULL)
    {
        DirtyMemFree(pState->pAppendHdr, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }
    NetCritKill(&pState->HttpCrit);
    DirtyMemFree(pState, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpGet

    \Description
        Initiate an HTTP transfer. Pass in a URL and the module starts a transfer
        from the appropriate server.

    \Input *pState      - reference pointer
    \Input *pUrl        - the url to retrieve
    \Input bHeadOnly    - if TRUE only get header

    \Output
        int32_t         - negative=failure, else success

    \Version 04/12/2000 (gschaefer) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpGet(ProtoHttpRefT *pState, const char *pUrl, uint32_t bHeadOnly)
{
    int32_t iResult;

    // reset redirection count
    pState->iNumRedirect = 0;

    // format the request
    if (pUrl != NULL)
    {
        if ((iResult = _ProtoHttpFormatRequest(pState, pUrl, NULL, 0, bHeadOnly ? PROTOHTTP_REQUESTTYPE_HEAD : PROTOHTTP_REQUESTTYPE_GET)) < 0)
        {
            return(iResult);
        }
    }
    // issue the request
    if (!pState->bPipelining || (pUrl == NULL))
    {
        _ProtoHttpSendRequest(pState);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpRecv

    \Description
        Return the actual url data.

    \Input *pState  - reference pointer
    \Input *pBuffer - buffer to store data in
    \Input iBufMin  - minimum number of bytes to return (returns zero if this number is not available)
    \Input iBufMax  - maximum number of bytes to return (buffer size)

    \Output
        int32_t     - negative=error, zero=no data available or bufmax <= 0, positive=number of bytes read

    \Version 04/12/2000 (gschaefer) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpRecv(ProtoHttpRefT *pState, char *pBuffer, int32_t iBufMin, int32_t iBufMax)
{
    int32_t iResult;

    NetCritEnter(&pState->HttpCrit);
    iResult = _ProtoHttpRecv(pState, pBuffer, iBufMin, iBufMax);
    NetCritLeave(&pState->HttpCrit);

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpRecvAll

    \Description
        Return all of the url data.

    \Input *pState  - reference pointer
    \Input *pBuffer - buffer to store data in
    \Input iBufSize - size of buffer

    \Output
        int32_t     - PROTOHTTP_RECV*, or positive=bytes in response

    \Version 12/14/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpRecvAll(ProtoHttpRefT *pState, char *pBuffer, int32_t iBufSize)
{
    int32_t iRecvMax, iRecvResult;

    // try to read as much data as possible (leave one byte for null termination)
    for (iRecvMax = iBufSize-1; (iRecvResult = ProtoHttpRecv(pState, pBuffer + pState->iRecvSize, 1, iRecvMax - pState->iRecvSize)) > 0; )
    {
        // add to amount received
        pState->iRecvSize += iRecvResult;
    }

    // check response code
    if (iRecvResult == PROTOHTTP_RECVDONE)
    {
        // null-terminate response & return completion success
        if ((pBuffer != NULL) && (iBufSize > 0))
        {
            pBuffer[pState->iRecvSize] = '\0';
        }
        iRecvResult = pState->iRecvSize;
    }
    else if ((iRecvResult < 0) && (iRecvResult != PROTOHTTP_RECVWAIT))
    {
        // an error occurred
        NetPrintf(("protohttp: [%p] error %d receiving response\n", pState, iRecvResult));
    }
    else if (iRecvResult == 0)
    {
        iRecvResult = (pState->iRecvSize < iRecvMax) ? PROTOHTTP_RECVWAIT : PROTOHTTP_RECVBUFF;
    }

    // return result to caller
    return(iRecvResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpPost

    \Description
        Initiate transfer of data to to the server via a HTTP POST command.

    \Input *pState      - reference pointer
    \Input *pUrl        - the URL that identifies the POST action.
    \Input *pData       - pointer to URL data (optional, may be NULL)
    \Input iDataSize    - size of data being uploaded (see Notes below)
    \Input bDoPut       - if TRUE, do a PUT instead of a POST

    \Output
        int32_t         - negative=failure, else number of data bytes sent

    \Notes
        Any data that is not sent as part of the Post transaction should be sent
        with ProtoHttpSend().  ProtoHttpSend() should be called at poll rate (i.e.
        similar to how often ProtoHttpUpdate() is called) until all of the data has
        been sent.  The amount of data bytes actually sent is returned by the
        function.

        If pData is not NULL and iDataSize is less than or equal to zero, iDataSize
        will be recalculated as the string length of pData, to allow for easy string
        sending.

        If pData is NULL and iDataSize is negative, the transaction is assumed to
        be a streaming transaction and a chunked transfer will be performed.  A
        subsequent call to ProtoHttpSend() with iDataSize equal to zero will end
        the transaction.

    \Version 03/03/2004 (sbevan) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpPost(ProtoHttpRefT *pState, const char *pUrl, const char *pData, int64_t iDataSize, uint32_t bDoPut)
{
    int32_t iDataSent;
    // allow for easy string sending
    if ((pData != NULL) && (iDataSize <= 0))
    {
        iDataSize = (int32_t)strlen(pData);
    }
    // save post size (or -1 to indicate that this is a variable-length stream)
    pState->iPostSize = iDataSize;
    // make the request
    if ((iDataSent = _ProtoHttpFormatRequest(pState, pUrl, pData, iDataSize, bDoPut ? PROTOHTTP_REQUESTTYPE_PUT : PROTOHTTP_REQUESTTYPE_POST)) >= 0)
    {
        // send the request
        _ProtoHttpSendRequest(pState);
    }
    return(iDataSent);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpSend

    \Description
        Send data during an ongoing Post transaction.

    \Input *pState      - reference pointer
    \Input *pData       - pointer to data to send
    \Input iDataSize    - size of data being sent

    \Output
        int32_t         - negative=failure, else number of data bytes sent

    \Version 11/18/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpSend(ProtoHttpRefT *pState, const char *pData, int32_t iDataSize)
{
    int32_t iResult;

    // make sure we are in a sending state
    if (pState->eState < ST_RESP)
    {
        // not ready to send data yet
        return(0);
    }
    else if (pState->eState != ST_RESP)
    {
        // if we're past ST_RESP, an error occurred.
        return(-1);
    }

    /* clamp to max ProtoHttp buffer size - even though
       we don't queue the data in the ProtoHttp buffer, this
       gives us a reasonable max size to send in one chunk */
    if (iDataSize > pState->iInpMax)
    {
        iDataSize = pState->iInpMax;
    }

    // get sole access to httpcrit
    NetCritEnter(&pState->HttpCrit);
    // send the data
    iResult = (pState->iPostSize < 0) ? _ProtoHttpSendChunk(pState, pData, iDataSize) : _ProtoHttpSend(pState, pData, iDataSize);
    // release access to httpcrit
    NetCritLeave(&pState->HttpCrit);

    // return result
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpDelete

    \Description
        Request deletion of a server-based resource.

    \Input *pState      - reference pointer
    \Input *pUrl        - Url describing reference to delete

    \Output
        int32_t         - negative=failure, zero=success

    \Version 06/01/2009 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpDelete(ProtoHttpRefT *pState, const char *pUrl)
{
    int32_t iResult;

    // reset redirection count
    pState->iNumRedirect = 0;

    // format the request
    if ((iResult = _ProtoHttpFormatRequest(pState, pUrl, NULL, 0, PROTOHTTP_REQUESTTYPE_DELETE)) >= 0)
    {
        // issue the request
        _ProtoHttpSendRequest(pState);
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpOptions

    \Description
        Request options from a server.

    \Input *pState      - reference pointer
    \Input *pUrl        - Url describing reference to get options on

    \Output
        int32_t         - negative=failure, zero=success

    \Version 07/14/2010 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpOptions(ProtoHttpRefT *pState, const char *pUrl)
{
    int32_t iResult;

    // reset redirection count
    pState->iNumRedirect = 0;

    // format the request
    if ((iResult = _ProtoHttpFormatRequest(pState, pUrl, NULL, 0, PROTOHTTP_REQUESTTYPE_OPTIONS)) >= 0)
    {
        // issue the request
        _ProtoHttpSendRequest(pState);
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpPatch

    \Description
        Initiate transfer of data to to the server via a HTTP PATCH command.

    \Input *pState      - reference pointer
    \Input *pUrl        - the URL that identifies the POST action.
    \Input *pData       - pointer to URL data (optional, may be NULL)
    \Input iDataSize    - size of data being uploaded (see Notes below)

    \Output
        int32_t         - negative=failure, else number of data bytes sent

    \Notes
        Any data that is not sent as part of the Patch transaction should be sent
        with ProtoHttpSend().  ProtoHttpSend() should be called at poll rate (i.e.
        similar to how often ProtoHttpUpdate() is called) until all of the data has
        been sent.  The amount of data bytes actually sent is returned by the
        function.

        If pData is not NULL and iDataSize is less than or equal to zero, iDataSize
        will be recalculated as the string length of pData, to allow for easy string
        sending.

        If pData is NULL and iDataSize is negative, the transaction is assumed to
        be a streaming transaction and a chunked transfer will be performed.  A
        subsequent call to ProtoHttpSend() with iDataSize equal to zero will end
        the transaction.

    \Version 01/01/2017 (amakoukji) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpPatch(ProtoHttpRefT *pState, const char *pUrl, const char *pData, int64_t iDataSize)
{
    int32_t iDataSent;
    // allow for easy string sending
    if ((pData != NULL) && (iDataSize <= 0))
    {
        iDataSize = (int32_t)strlen(pData);
    }
    // save post size (or -1 to indicate that this is a variable-length stream)
    pState->iPostSize = iDataSize;
    // make the request
    if ((iDataSent = _ProtoHttpFormatRequest(pState, pUrl, pData, iDataSize, PROTOHTTP_REQUESTTYPE_PATCH)) >= 0)
    {
        // send the request
        _ProtoHttpSendRequest(pState);
    }
    return(iDataSent);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpRequestCb2

    \Description
        Initiate an HTTP transfer. Pass in a URL and the module starts a transfer
        from the appropriate server.  Use ProtoHttpRequest() macro wrapper if
        callbacks are not required.

    \Input *pState              - reference pointer
    \Input *pUrl                - the url to retrieve
    \Input *pData               - user data to send to server (PUT and POST only)
    \Input iDataSize            - size of user data to send to server (PUT and POST only)
    \Input eRequestType         - request type to make
    \Input *pWriteCb            - write callback (optional)
    \Input *pCustomHeaderCb     - custom header callback (optional)
    \Input *pReceiveHeaderCb    - receive header callback (optional)
    \Input *pUserData           - callback user data (optional)

    \Output
        int32_t                 - negative=failure, zero=success, >0=number of data bytes sent (PUT and POST only)

    \Version 09/11/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t ProtoHttpRequestCb2(ProtoHttpRefT *pState, const char *pUrl, const char *pData, int64_t iDataSize, ProtoHttpRequestTypeE eRequestType, ProtoHttpWriteCbT *pWriteCb, ProtoHttpCustomHeaderCbT *pCustomHeaderCb, ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb, void *pUserData)
{
    int32_t iResult;

    // save callbacks
    pState->pWriteCb = pWriteCb;
    pState->pReqCustomHeaderCb = pCustomHeaderCb;
    pState->pReqReceiveHeaderCb = pReceiveHeaderCb;
    pState->pUserData = pUserData;

    // make the request
    if ((eRequestType == PROTOHTTP_REQUESTTYPE_GET) || (eRequestType == PROTOHTTP_REQUESTTYPE_HEAD))
    {
        iResult = ProtoHttpGet(pState, pUrl, eRequestType == PROTOHTTP_REQUESTTYPE_HEAD);
    }
    else if ((eRequestType == PROTOHTTP_REQUESTTYPE_PUT) || (eRequestType == PROTOHTTP_REQUESTTYPE_POST))
    {
        iResult = ProtoHttpPost(pState, pUrl, pData, iDataSize, eRequestType == PROTOHTTP_REQUESTTYPE_PUT);
    }
    else if (eRequestType == PROTOHTTP_REQUESTTYPE_DELETE)
    {
        iResult = ProtoHttpDelete(pState, pUrl);
    }
    else if (eRequestType == PROTOHTTP_REQUESTTYPE_OPTIONS)
    {
        iResult = ProtoHttpOptions(pState, pUrl);
    }
    else if (eRequestType == PROTOHTTP_REQUESTTYPE_PATCH)
    {
        iResult = ProtoHttpPatch(pState, pUrl, pData, iDataSize);
    }
    else
    {
        NetPrintf(("protohttp: [%p] unrecognized request type %d\n", pState, eRequestType));
        iResult = -1;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpAbort

    \Description
        Abort current operation, if any.

    \Input *pState      - reference pointer

    \Output
        None.

    \Version 12/01/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void ProtoHttpAbort(ProtoHttpRefT *pState)
{
    // acquire sole access to http crit
    NetCritEnter(&pState->HttpCrit);

    // terminate current connection, if any
    _ProtoHttpClose(pState, "abort");

    // reset state
    _ProtoHttpReset(pState);

    // release sole access to http crit
    NetCritLeave(&pState->HttpCrit);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpSetBaseUrl

    \Description
        Set base url that will be used for any relative url references.

    \Input *pState      - module state
    \Input *pUrl        - base url

    \Output
        None

    \Version 02/03/2010 (jbrookes)
*/
/********************************************************************************F*/
void ProtoHttpSetBaseUrl(ProtoHttpRefT *pState, const char *pUrl)
{
    char strHost[sizeof(pState->strHost)], strKind[8];
    int32_t iPort, iSecure;
    uint8_t bPortSpecified;

    // parse the url for kind, host, and port
    ProtoHttpUrlParse2(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);

    // set base info
    NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] setting base url to %s://%s:%d\n", pState, iSecure ? "https" : "http", strHost, iPort));
    ds_strnzcpy(pState->strBaseHost, strHost, sizeof(pState->strBaseHost));
    pState->iBasePort = iPort;
    pState->iBaseSecure = iSecure;
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpGetLocationHeader

    \Description
        Get location header from the input buffer.  The Location header requires
        some special processing.

    \Input *pState  - reference pointer
    \Input *pInpBuf - buffer holding header text
    \Input *pBuffer - [out] output buffer for parsed location header, null for size request
    \Input iBufSize - size of output buffer, zero for size request
    \Input **pHdrEnd- [out] pointer past end of parsed header (optional)

    \Output
        int32_t     - negative=not found or not enough space, zero=success, positive=size query result

    \Version 03/24/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpGetLocationHeader(ProtoHttpRefT *pState, const char *pInpBuf, char *pBuffer, int32_t iBufSize, const char **pHdrEnd)
{
    const char *pLocHdr;
    int32_t iLocLen, iLocPreLen=0;

    // get a pointer to header
    if ((pLocHdr = ProtoHttpFindHeaderValue(pInpBuf, "location")) == NULL)
    {
        return(-1);
    }

    /* according to RFC location headers should be absolute, but some webservers respond with relative
       URL's.  we assume any url that does not include "://" is a relative url, and if we find one, we
       assume the request keeps the same security, port, and host as the previous request */
    if ((pState != NULL) && (!strstr(pLocHdr, "://")))
    {
        char strTemp[288]; // space for max DNS name (253 chars) plus max http url prefix
        int32_t iPort, iSecure;
        char *pStrHost;

        // get host info; if we're using a proxy we need to look at the proxied* fields
        if (pState->strProxiedHost[0] != '\0')
        {
            pStrHost = pState->strProxiedHost;
            iPort = pState->iProxiedPort;
            iSecure = pState->iProxiedSecure;
        }
        else
        {
            pStrHost = pState->strHost;
            iPort = pState->iPort;
            iSecure = pState->iSecure;
        }

        // format http url prefix
        if ((pState->iSecure && (iPort == 443)) || (iPort == 80))
        {
            ds_snzprintf(strTemp, sizeof(strTemp), "%s://%s", iSecure ? "https" : "http", pStrHost);
        }
        else
        {
            ds_snzprintf(strTemp, sizeof(strTemp), "%s://%s:%d", iSecure ? "https" : "http", pStrHost, iPort);
        }

        // make sure relative URL includes '/' as the first character, required when we format the redirection url
        if (*pLocHdr != '/')
        {
            ds_strnzcat(strTemp, "/", sizeof(strTemp));
        }

        // calculate url prefix length
        iLocPreLen = (int32_t)strlen(strTemp);

        // copy url prefix text if a buffer is specified
        if (pBuffer != NULL)
        {
            ds_strnzcpy(pBuffer, strTemp, iBufSize);
            pBuffer = (char *)((uint8_t *)pBuffer + iLocPreLen);
            iBufSize -= iLocPreLen;
        }
    }

    // extract location header and return size
    iLocLen = ProtoHttpExtractHeaderValue(pLocHdr, pBuffer, iBufSize, pHdrEnd);
    // if it's a size request add in (possible) url header length
    if ((pBuffer == NULL) && (iBufSize == 0))
    {
        iLocLen += iLocPreLen;
    }

    // return to caller
    return(iLocLen);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpControl

    \Description
        ProtoHttp control function.  Different selectors control different behaviors.

    \Input *pState  - reference pointer
    \Input iSelect  - control selector ('time')
    \Input iValue   - selector specific
    \Input iValue2  - selector specific
    \Input *pValue  - selector specific

    \Output
        int32_t     - selector specific

    \Notes
        Selectors are:

        \verbatim
            SELECTOR    DESCRIPTION
            'apnd'      The given buffer will be appended to future headers sent
                        by ProtoHttp.  Note that the User-Agent and Accept lines
                        in the default header will be replaced, so if these lines
                        are desired, they should be supplied in the append header.
            'disc'      Close current connection, if any.
            'hver'      Sets header type verification: zero=disabled, else enabled
            'ires'      Resize input buffer
            'keep'      Sets keep-alive; zero=disabled, else enabled
            'gpxy'      Set global proxy server to use for all protohttp refs
            'pipe'      Sets pipelining; zero=disabled, else enabled
            'pnxt'      Call to proceed to next piped result
            'prxy'      Set proxy server to use for this ref
            'rmax'      Sets maximum number of redirections (default=3; 0=disable)
            'rput'      Sets connection-reuse on put/post (TRUE=enabled, default FALSE)
            'spam'      Sets debug output verbosity (0...n)
            'time'      Sets ProtoHttp client timeout in milliseconds (default=30s)
        \endverbatim

        Unhandled selectors are passed on to ProtoSSL.

    \Version 11/12/2004 (jbrookes) First Version
*/
/*******************************************************************************F*/
int32_t ProtoHttpControl(ProtoHttpRefT *pState, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iSelect == 'apnd')
    {
        return(_ProtoHttpSetAppendHeader(pState, (const char *)pValue));
    }
    if (iSelect == 'disc')
    {
        _ProtoHttpClose(pState, "user request");
        return(0);
    }
    if (iSelect == 'hver')
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] header type verification %s\n", pState, iValue ? "enabled" : "disabled"));
        pState->bVerifyHdr = iValue;
    }
    if (iSelect == 'ires')
    {
        return(_ProtoHttpResizeBuffer(pState, iValue));
    }
    if (iSelect == 'keep')
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] setting keep-alive to %d\n", pState, iValue));
        pState->iKeepAlive = pState->iKeepAliveDflt = iValue;
        return(0);
    }
    if (iSelect == 'gpxy')
    {
        NetPrintf(("protohttp: setting %s as global proxy server\n", pValue));
        ds_strnzcpy(_ProtoHttp_strGlobalProxy, pValue, sizeof(_ProtoHttp_strGlobalProxy));
        return(0);
    }
    if (iSelect == 'pipe')
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] pipelining %s\n", pState, iValue ? "enabled" : "disabled"));
        pState->bPipelining = iValue ? TRUE : FALSE;
        return(0);
    }
    if (iSelect == 'pnxt')
    {
        NetPrintfVerbose((pState->iVerbose, 2, "protohttp: [%p] proceeding to next pipeline result\n", pState));
        pState->bPipeGetNext = TRUE;
        return(0);
    }
    if (iSelect == 'prxy')
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttp: [%p] setting %s as proxy server\n", pState, pValue));
        ds_strnzcpy(pState->strProxy, pValue, sizeof(pState->strProxy));
        return(0);
    }
    if (iSelect == 'rmax')
    {
        pState->iMaxRedirect = iValue;
        return(0);
    }
    if (iSelect == 'rput')
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] connection reuse on put/post %s\n", pState, iValue ? "enabled" : "disabled"));
        pState->bReuseOnPost = iValue ? TRUE : FALSE;
        return(0);
    }
    if (iSelect == 'spam')
    {
        pState->iVerbose = iValue;
        // fall through to protossl
    }
    if (iSelect == 'time')
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] setting timeout to %d ms\n", pState, iValue));
        pState->uTimeout = (unsigned)iValue;
        return(0);
    }
    // unhandled -- pass on to ProtoSSL
    return(ProtoSSLControl(pState->pSsl, iSelect, iValue, iValue2, pValue));
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpStatus

    \Description
        Return status of current HTTP transfer.  Status type depends on selector.

    \Input *pState  - reference pointer
    \Input iSelect  - info selector (see Notes)
    \Input *pBuffer - [out] storage for selector-specific output
    \Input iBufSize - size of buffer

    \Output
        int32_t     - selector specific

    \Notes
        Selectors are:

    \verbatim
        SELECTOR    RETURN RESULT
        'body'      negative=failed or pending, else size of body (for 64bit size, get via pBuffer)
        'code'      negative=no response, else server response code (ProtoHttpResponseE)
        'data'      negative=failed, zero=pending, positive=amnt of data ready
        'date'      last-modified date, if available
        'done'      negative=failed, zero=pending, positive=done
        'essl'      returns protossl error state
        'head'      negative=failed or pending, else size of header
        'host'      current host copied to output buffer
        'hres'      return hResult containing either the socket error or ssl error or the http status code
        'htxt'      current received http header text copied to output buffer
        'info'      copies most recent info header received, if any, to output buffer (one time only)
        'imax'      size of input buffer
        'iovr'      returns input buffer overflow size (valid on PROTOHTTP_MINBUFF result code)
        'plst'      return whether any piped requests were lost due to a premature close
        'port'      current port
        'rtxt'      most recent http request header text copied to output buffer
        'rmax'      returns currently configured max redirection count
        'time'      TRUE if the client timed out the connection, else FALSE
    \endverbatim

        Unhandled selectors are passed on to ProtoSSL.

    \Version 04/12/2000 (gschaefer) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpStatus(ProtoHttpRefT *pState, int32_t iSelect, void *pBuffer, int32_t iBufSize)
{
    // return protossl error state (we cache this since we reset the state when we disconnect on error)
    if (iSelect == 'essl')
    {
        return(pState->iSslFail);
    }

    // return current host
    if (iSelect == 'host')
    {
        ds_strnzcpy(pBuffer, pState->strHost, iBufSize);
        return(0);
    }

    // return hResult containing either the socket error or ssl error or the http status code
    if (iSelect == 'hres')
    {
        if (pState->iHdrCode > 0)
        {
            return(DirtyErrGetHResult(DIRTYAPI_PROTO_HTTP, pState->iHdrCode, (pState->iHdrCode >= PROTOHTTP_RESPONSE_CLIENTERROR) ? TRUE : FALSE));
        }
        else
        {
            return(pState->iHresult);
        }
    }

    // return size of input buffer
    if (iSelect == 'imax')
    {
        return(pState->iInpMax);
    }

    // return input overflow amount (valid after PROTOHTTP_MINBUFF result code)
    if (iSelect == 'iovr')
    {
        return(pState->iInpOvr);
    }

    // return piped requests lost status
    if (iSelect == 'plst')
    {
        return(pState->bPipedRequestsLost);
    }

    // return current port
    if (iSelect == 'port')
    {
        return(pState->iPort);
    }

    // return current redirection max
    if (iSelect == 'rmax')
    {
        return(pState->iMaxRedirect);
    }

    // return most recent http request header text
    if (iSelect == 'rtxt')
    {
        ds_strnzcpy(pBuffer, pState->strRequestHdr, iBufSize);
        return(0);
    }

    // done check: negative=failed, zero=pending, positive=done
    if (iSelect == 'done')
    {
        if (pState->eState == ST_FAIL)
            return(-1);
        if (pState->eState == ST_DONE)
            return(1);
        return(0);
    }

    // data check: negative=failed, zero=pending, positive=data ready
    if (iSelect == 'data')
    {
        if (pState->eState == ST_FAIL)
            return(-1);
        if ((pState->eState == ST_BODY) || (pState->eState == ST_DONE))
            return(pState->iInpLen);
        return(0);
    }

    // return response code
    if (iSelect == 'code')
        return(pState->iHdrCode);

    // return timeout indicator
    if (iSelect == 'time')
        return(pState->bTimeout);

    // copies info header (if available) to output buffer
    if (iSelect == 'info')
    {
        if (pState->bInfoHdr)
        {
            if (pBuffer != NULL)
            {
                ds_strnzcpy(pBuffer, pState->strHdr, iBufSize);
            }
            pState->bInfoHdr = FALSE;
            return(pState->iHdrCode);
        }
        return(0);
    }

    // the following selectors early-out with errors in failure states
    if ((iSelect == 'body') || (iSelect == 'date') || (iSelect == 'head') || (iSelect == 'htxt'))
    {
        // check the state
        if (pState->eState == ST_FAIL)
            return(-1);
        if ((pState->eState != ST_BODY) && (pState->eState != ST_DONE))
            return(-2);

        // parse the tokens
        if (iSelect == 'head')
        {
            return(pState->iHeadSize);
        }
        if (iSelect == 'body')
        {
            if ((pBuffer != NULL) && (iBufSize == sizeof(pState->iBodySize)))
            {
                ds_memcpy(pBuffer, &pState->iBodySize, iBufSize);
            }
            return((int32_t)pState->iBodySize);
        }
        if (iSelect == 'date')
        {
            return(pState->iHdrDate);
        }
        if (iSelect == 'htxt')
        {
            ds_strnzcpy(pBuffer, pState->strHdr, iBufSize);
            return(0);
        }
    }

    // pass down to unhandled selectors to ProtoSSL
    return(ProtoSSLStat(pState->pSsl, iSelect, pBuffer, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpCheckKeepAlive

    \Description
        Checks whether a request for the given Url would be a keep-alive transaction.

    \Input *pState      - reference pointer
    \Input *pUrl        - Url to check

    \Output
        int32_t         - TRUE if a request to this Url would use keep-alive, else FALSE

    \Version 05/12/2009 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpCheckKeepAlive(ProtoHttpRefT *pState, const char *pUrl)
{
    char strHost[sizeof(pState->strHost)], strKind[8];
    int32_t iPort, iSecure;
    uint8_t bPortSpecified;

    // parse the url
    ProtoHttpUrlParse2(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);

    // refresh open status
    if (pState->bConnOpen && (ProtoSSLStat(pState->pSsl, 'stat', NULL, 0) <= 0))
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttp: [%p] check for keep-alive detected connection close\n", pState));
        pState->bConnOpen = FALSE;
    }

    // see if a request for this url would use keep-alive
    if (pState->bConnOpen && (pState->iKeepAlive > 0) && (pState->iPort == iPort) && (pState->iSecure == iSecure) && !ds_stricmp(pState->strHost, strHost))
    {
        return(1);
    }
    else
    {
        return(0);
    }
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpUpdate

    \Description
        Give time to module to do its thing (should be called periodically to
        allow module to perform work)

    \Input *pState      - reference pointer

    \Output
        None.

    \Version 04/12/2000 (gschaefer) First Version
*/
/********************************************************************************F*/
void ProtoHttpUpdate(ProtoHttpRefT *pState)
{
    int32_t iResult;

    // give time to comm module
    ProtoSSLUpdate(pState->pSsl);

    // acquire sole access to http crit
    NetCritEnter(&pState->HttpCrit);

    // check for timeout
    if ((pState->eState != ST_IDLE) && (pState->eState != ST_DONE) && (pState->eState != ST_FAIL))
    {
        if (NetTickDiff(NetTick(), pState->uTimer) >= 0)
        {
            NetPrintf(("protohttp: [%p] server timeout (state=%d)\n", pState, pState->eState));
            pState->eState = ST_FAIL;
            pState->bTimeout = TRUE;
        }
    }

    // see if connection is complete
    if (pState->eState == ST_CONN)
    {
        iResult = ProtoSSLStat(pState->pSsl, 'stat', NULL, 0);
        if (iResult > 0)
        {
            pState->uTimer = NetTick() + pState->uTimeout;
            pState->eState = ST_SEND;
            pState->bConnOpen = TRUE;
        }
        if (iResult < 0)
        {
            NetPrintf(("protohttp: [%p] ST_CONN got ST_FAIL (err=%d)\n", pState, iResult));
            pState->eState = ST_FAIL;
            pState->iSslFail = ProtoSSLStat(pState->pSsl, 'fail', NULL, 0);
            pState->iHresult = ProtoSSLStat(pState->pSsl, 'hres', NULL, 0);
        }
    }

    // send buffered header+data to webserver
    if (pState->eState == ST_SEND)
    {
        if (((iResult = _ProtoHttpSendBuff(pState)) > 0) && (pState->iInpLen == 0))
        {
            #if DIRTYCODE_LOGGING
            _ProtoHttpDisplayHeader(pState);
            #endif
            pState->iInpOff = 0;
            pState->iHdrOff = 0;
            pState->eState = ST_RESP;
        }
    }

    // wait for initial response
    if (pState->eState == ST_RESP)
    {
        // try to send any remaining buffered data
        _ProtoHttpSendBuff(pState);

        // try for the first line of http response
        if ((iResult = _ProtoHttpHeaderRecvFirstLine(pState)) > 0)
        {
            // copy first line of header to input buffer and proceed
            ds_strnzcpy(pState->pInpBuf, pState->strHdr, pState->iHdrOff+1);
            pState->iInpLen = pState->iHdrOff;
            pState->eState = ST_HEAD;
        }
        else if ((iResult < 0) && !_ProtoHttpRetrySendRequest(pState))
        {
            NetPrintf(("protohttp: [%p] ST_HEAD byte0 got ST_FAIL (err=%d)\n", pState, iResult));
            pState->iInpLen = 0;
            pState->eState = ST_FAIL;
        }
    }

    // try to receive header data
    if (pState->eState == ST_HEAD)
    {
        // append data to buffer
        if ((iResult = _ProtoHttpRecvData(pState, pState->pInpBuf+pState->iInpLen, pState->iInpMax - pState->iInpLen)) > 0)
        {
            pState->iInpLen += iResult;
        }
        // deal with network error (defer handling closed state to next block, to allow pipelined receives of already buffered transactions)
        if ((iResult < 0) && ((iResult != SOCKERR_CLOSED) || (pState->iInpLen <= 4)))
        {
            NetPrintf(("protohttp: [%p] ST_HEAD append got ST_FAIL (err=%d, len=%d)\n", pState, iResult, pState->iInpLen));
            pState->eState = ST_FAIL;
            pState->iSslFail = ProtoSSLStat(pState->pSsl, 'fail', NULL, 0);
            pState->iHresult = ProtoSSLStat(pState->pSsl, 'hres', NULL, 0);
        }
    }

    // check for header completion, process it
    if ((pState->eState == ST_HEAD) && (pState->iInpLen > 4))
    {
        _ProtoHttpHeaderProcess(pState);
    }

    // parse incoming body data
    while ((pState->eState == ST_BODY) && (_ProtoHttpRecvBody(pState) != 0))
        ;

    // write callback processing
    if (pState->pWriteCb != NULL)
    {
        _ProtoHttpWriteCbProcess(pState);
    }

    // force disconnect in failure state
    if (pState->eState == ST_FAIL)
    {
        _ProtoHttpClose(pState, "error");
    }

    // handle completion
    if (pState->eState == ST_DONE)
    {
        if (pState->bPipelining && (pState->iPipedRequests > 0))
        {
            // are we ready to proceed?
            if ((pState->iBodyRcvd == pState->iBodySize) && pState->bPipeGetNext)
            {
                NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [%p] handling piped request\n", pState));
                _ProtoHttpCompactBuffer(pState);
                pState->eState = ST_HEAD;
                pState->iHeadSize = pState->iBodySize = pState->iBodyRcvd = 0;
                pState->iPipedRequests -= 1;
                pState->bPipeGetNext = FALSE;
            }
        }
        else if (pState->bCloseHdr)
        {
            _ProtoHttpClose(pState, "server request");
        }

        // check for keep-alive connection close by server
        if (pState->bConnOpen && (ProtoSSLStat(pState->pSsl, 'stat', NULL, 0) <= 0))
        {
            _ProtoHttpClose(pState, "server close");
        }
    }

    // release access to httpcrit
    NetCritLeave(&pState->HttpCrit);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpSetCACert

    \Description
        Add one or more X.509 CA certificates to the trusted list. A
        certificate added will be available to all HTTP instances for
        the lifetime of the application. This function can add one or more
        PEM certificates or a single DER certificate.

    \Input *pCACert - pointer to certificate data
    \Input iCertSize- certificate size in bytes

    \Output
        int32_t     - negative=failure, zero=success

    \Version 01/13/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpSetCACert(const uint8_t *pCACert, int32_t iCertSize)
{
    return(ProtoSSLSetCACert(pCACert, iCertSize));
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpSetCACert2

    \Description
        Add one or more X.509 CA certificates to the trusted list. A
        certificate added will be available to all HTTP instances for
        the lifetime of the application. This function can add one or more
        PEM certificates or a single DER certificate.

        This version of the function does not validate the CA at load time.
        The X509 certificate data will be copied and retained until the CA
        is validated, either by use of ProtoHttpValidateAllCA() or by the CA
        being used to validate a certificate.

    \Input *pCACert - pointer to certificate data
    \Input iCertSize- certificate size in bytes

    \Output
        int32_t     - negative=failure, zero=success

    \Version 04/21/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpSetCACert2(const uint8_t *pCACert, int32_t iCertSize)
{
    return(ProtoSSLSetCACert2(pCACert, iCertSize));
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpValidateAllCA

    \Description
        Validate all CA that have been added but not yet been validated.  Validation
        is a one-time process and disposes of the X509 certificate that is retained
        until validation.

    \Output
        int32_t     - negative=failure, zero=success

    \Version 04/21/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpValidateAllCA(void)
{
    return(ProtoSSLValidateAllCA());
}

/*F*************************************************************************/
/*!
    \Function ProtoHttpClrCACerts

    \Description
        Clears all dynamic CA certs from the list.

    \Version 01/14/2009 (jbrookes)
*/
/**************************************************************************F*/
void ProtoHttpClrCACerts(void)
{
    ProtoSSLClrCACerts();
}
