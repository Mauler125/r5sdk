/*H********************************************************************************/
/*!
    \File protohttpserv.c

    \Description
        This module implements an HTTP server that can perform basic transactions
        (get/put) with an HTTP client.  It conforms to but does not fully implement
        the 1.1 HTTP spec (http://www.w3.org/Protocols/rfc2616/rfc2616.html), and
        allows for secure HTTP transactions as well as insecure transactions.

    \Copyright
        Copyright (c) Electronic Arts 2013

    \Version 1.0 12/11/2013 (jbrookes) Initial version, based on HttpServ tester2 module
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h" // NetConnSleep
#include "DirtySDK/proto/protohttputil.h"
#include "DirtySDK/proto/protossl.h"

#include "DirtySDK/proto/protohttpserv.h"

/*** Defines **********************************************************************/

#define HTTPSERV_VERSION                (0x0100)            //!< httpserv revision number (maj.min); update this for major bug fixes or protocol additions/changes
#define HTTPSERV_DISCTIME               (5*1000)            //!< give five seconds after we disconnect before destroying network resources
#define HTTPSERV_CHUNKWAITTEST          (0)                 //!< this tests a corner case in protohttp chunk receive where only one byte of the chunk trailer is available
#define HTTPSERV_IDLETIMEOUT_DEFAULT    (5*60*1000)         //!< default keep-alive timeout
#define HTTPSERV_REQUESTTIMEOUT_DEFAULT (30*1000)           //!< default request timeout
#define HTTPSERV_BUFSIZE_DEFAULT        (16000)             //!< the most user payload we can send in a single SSL packet

/*** Function Prototypes **********************************************************/

/*** Type Definitions *************************************************************/

//! httpserv response table
typedef struct ProtoHttpServRespT
{
    ProtoHttpResponseE eResponse;
    const char *pResponseText;
}ProtoHttpServRespT;

//! httpserv transaction state for a single transaction
typedef struct ProtoHttpServThreadT
{
    struct ProtoHttpServThreadT *pNext;
    ProtoSSLRefT *pProtoSSL;

    struct sockaddr RequestAddr;

    ProtoHttpServRequestT RequestInfo;      //!< user data associated with this thread via request callback (for handing request)
    ProtoHttpServResponseT ResponseInfo;    //!< user data associated with this thread via request callback (for replying)

    char *pBuffer;
    int32_t iBufMax;
    int32_t iBufLen;
    int32_t iBufOff;
    int32_t iChkLen;
    int32_t iChkRcv;
    int32_t iChkOvr;        //!< chunk overhead
    int32_t iIdleTimer;
    int32_t iDisconnectTimer;

    int64_t iContentSent;

    char strUserAgent[256];

    uint8_t bConnected;
    uint8_t bReceivedHeader;
    uint8_t bParsedHeader;
    uint8_t bProcessedRequest;
    uint8_t bReceivedBody;
    uint8_t bFormattedHeader;
    uint8_t bSentHeader;
    uint8_t bSentBody;
    uint8_t bConnectionClose;
    uint8_t bConnectionKeepAlive;
    uint8_t bDisconnecting;
    uint8_t uHttpThreadId;
    uint8_t bHttp1_0;
    uint8_t bChunked;
    uint8_t _pad[2];
} ProtoHttpServThreadT;

//! httpserv module state
struct ProtoHttpServRefT
{
    ProtoSSLRefT *pListenSSL;

    int32_t iMemGroup;
    void *pMemGroupUserData;

    ProtoHttpServRequestCbT *pRequestCb;
    ProtoHttpServReceiveCbT *pReceiveCb;
    ProtoHttpServSendCbT *pSendCb;
    ProtoHttpServHeaderCbT *pHeaderCb;
    ProtoHttpServLogCbT *pLogCb;
    void *pUserData;

    char *pServerCert;
    int32_t iServerCertLen;
    char *pServerKey;
    int32_t iServerKeyLen;
    int32_t iSecure;
    uint16_t uSslVersion;
    uint16_t uSslVersionMin;
    uint32_t uSslCiphers;
    int32_t iClientCertLvl;
    int32_t iIdleTimeout;
    int32_t iRequestTimeout;
    uint16_t uDebugLevel;

    char strServerName[128];
    char strAlpn[128];

    ProtoHttpServThreadT *pThreadListHead;
    ProtoHttpServThreadT *pThreadListTail;
    uint8_t uCurrThreadId;
};

/*** Variables ********************************************************************/

//! http request names
static const char *_ProtoHttpServ_strRequestNames[PROTOHTTP_NUMREQUESTTYPES] =
{
    "HEAD", "GET", "POST", "PUT", "DELETE", "OPTIONS", "PATCH", NULL
};

//! http response name table
static const ProtoHttpServRespT _ProtoHttpServ_Responses[] =
{
    // 1xx - informational reponse
    { PROTOHTTP_RESPONSE_CONTINUE,              "Continue" },                       //!< continue with request, generally ignored
    { PROTOHTTP_RESPONSE_SWITCHPROTO,           "Switching Protocols" },            //!< 101 - OK response to client switch protocol request
    // 2xx - success response
    { PROTOHTTP_RESPONSE_OK,                    "OK" },                             //!< client's request was successfully received, understood, and accepted
    { PROTOHTTP_RESPONSE_CREATED,               "Created" } ,                       //!< new resource has been created
    { PROTOHTTP_RESPONSE_ACCEPTED,              "Accepted" } ,                      //!< request accepted but not complete
    { PROTOHTTP_RESPONSE_NONAUTH,               "Non-Authoritative Information" },  //!< non-authoritative info (ok)
    { PROTOHTTP_RESPONSE_NOCONTENT,             "No Content" } ,                    //!< request fulfilled, no message body
    { PROTOHTTP_RESPONSE_RESETCONTENT,          "Reset Content" } ,                 //!< request success, reset document view
    { PROTOHTTP_RESPONSE_PARTIALCONTENT,        "Partial Content" } ,               //!< server has fulfilled partial GET request
    // 3xx - redirection response
    { PROTOHTTP_RESPONSE_MULTIPLECHOICES,       "Multiple Choices" },               //!< requested resource corresponds to a set of representations
    { PROTOHTTP_RESPONSE_MOVEDPERMANENTLY,      "Moved Permanently" },              //!< requested resource has been moved permanently to new URI
    { PROTOHTTP_RESPONSE_FOUND,                 "Found" },                          //!< requested resources has been moved temporarily to a new URI
    { PROTOHTTP_RESPONSE_SEEOTHER,              "See Other" },                      //!< response can be found under a different URI
    { PROTOHTTP_RESPONSE_NOTMODIFIED,           "Not Modified" },                   //!< response to conditional get when document has not changed
    { PROTOHTTP_RESPONSE_USEPROXY,              "Use Proxy" },                      //!< requested resource must be accessed through proxy
    { PROTOHTTP_RESPONSE_TEMPREDIRECT,          "Temporary Redirect" },             //!< requested resource resides temporarily under a different URI
    // 4xx - client error response
    { PROTOHTTP_RESPONSE_BADREQUEST,            "Bad Request" },                    //!< request could not be understood by server due to malformed syntax
    { PROTOHTTP_RESPONSE_UNAUTHORIZED,          "Unauthorized" },                   //!< request requires user authorization
    { PROTOHTTP_RESPONSE_PAYMENTREQUIRED,       "Payment Required" },               //!< reserved for future user
    { PROTOHTTP_RESPONSE_FORBIDDEN,             "Forbidden" },                      //!< request understood, but server will not fulfill it
    { PROTOHTTP_RESPONSE_NOTFOUND,              "Not Found" },                      //!< Request-URI not found
    { PROTOHTTP_RESPONSE_METHODNOTALLOWED,      "Method Not Allowed" },             //!< method specified in the Request-Line is not allowed
    { PROTOHTTP_RESPONSE_NOTACCEPTABLE,         "Not Acceptable" },                 //!< resource incapable of generating content acceptable according to accept headers in request
    { PROTOHTTP_RESPONSE_PROXYAUTHREQ,          "Proxy Authentication Required" },  //!< client must first authenticate with proxy
    { PROTOHTTP_RESPONSE_REQUESTTIMEOUT,        "Request Timeout" },                //!< client did not produce response within server timeout
    { PROTOHTTP_RESPONSE_CONFLICT,              "Conflict" },                       //!< request could not be completed due to a conflict with current state of the resource
    { PROTOHTTP_RESPONSE_GONE,                  "Gone" },                           //!< requested resource is no longer available and no forwarding address is known
    { PROTOHTTP_RESPONSE_LENGTHREQUIRED,        "Length Required" },                //!< a Content-Length header was not specified and is required
    { PROTOHTTP_RESPONSE_PRECONFAILED,          "Precondition Failed" },            //!< precondition given in request-header field(s) failed
    { PROTOHTTP_RESPONSE_REQENTITYTOOLARGE,     "Request Entity Too Large" },       //!< request entity is larger than the server is able or willing to process
    { PROTOHTTP_RESPONSE_REQURITOOLONG,         "Request-URI Too Long" },           //!< Request-URI is longer than the server is willing to interpret
    { PROTOHTTP_RESPONSE_UNSUPPORTEDMEDIA,      "Unsupported Media Type" },         //!< request entity is in unsupported format
    { PROTOHTTP_RESPONSE_REQUESTRANGE,          "Requested Range Not Satisfiable" },//!< invalid range in Range request header
    { PROTOHTTP_RESPONSE_EXPECTATIONFAILED,     "Expectation Failed" },             //!< expectation in Expect request-header field could not be met by server
    // 5xx - server error response
    { PROTOHTTP_RESPONSE_INTERNALSERVERERROR,   "Internal Server Error" },          //!< an unexpected condition prevented the server from fulfilling the request
    { PROTOHTTP_RESPONSE_NOTIMPLEMENTED,        "Not Implemented" },                //!< the server does not support the functionality required to fulfill the request
    { PROTOHTTP_RESPONSE_BADGATEWAY,            "Bad Gateway" },                    //!< invalid response from gateway server
    { PROTOHTTP_RESPONSE_SERVICEUNAVAILABLE,    "Service Unavailable" },            //!< unable to handle request due to a temporary overloading or maintainance
    { PROTOHTTP_RESPONSE_GATEWAYTIMEOUT,        "Gateway Timeout" },                //!< gateway or DNS server timeout
    { PROTOHTTP_RESPONSE_HTTPVERSUNSUPPORTED,   "HTTP Version Not Supported" },     //!< the server does not support the HTTP protocol version that was used in the request
    { PROTOHTTP_RESPONSE_PENDING,               "Unknown" },                        //!< unknown response code
};

/*** Private Functions ************************************************************/

static int32_t _ProtoHttpServFormatHeader(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread);
static int32_t _ProtoHttpServUpdateSendHeader(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread);

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServLogPrintf

    \Description
        Log printing for HttpServ server (compiles in all builds, unlike debug output).

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread log entry is for (may be NULL)
    \Input *pFormat     - printf format string
    \Input ...          - variable argument list

    \Version 08/03/2007 (jbrookes) Borrowed from DirtyCast
*/
/********************************************************************************F*/
static void _ProtoHttpServLogPrintf(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread, const char *pFormat, ...)
{
    char strText[2048];
    int32_t iOffset;
    va_list Args;

    // format prefix to output buffer
    if (pHttpThread != NULL)
    {
        iOffset = ds_snzprintf(strText, sizeof(strText), "protohttpserv: [%p][%02x] ", pHttpServ, pHttpThread->uHttpThreadId);
    }
    else
    {
        iOffset = ds_snzprintf(strText, sizeof(strText), "protohttpserv: [%p] ", pHttpServ);
    }

    // format output
    va_start(Args, pFormat);
    ds_vsnprintf(strText+iOffset, sizeof(strText)-iOffset, pFormat, Args);
    va_end(Args);

    // forward to callback, or print if no callback installed
    if ((pHttpServ != NULL) && (pHttpServ->pLogCb != NULL))
    {
        pHttpServ->pLogCb(strText, pHttpServ->pUserData);
    }
    else
    {
        NetPrintf(("%s", strText));
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServLogPrintfVerbose

    \Description
        Log printing for HttpServ server (compiles in all builds, unlike debug output)
        with varying verbosity levels

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread log entry is for (may be NULL)
    \Input uCheckLevel  - the level we are checking our internal uDebugLevel against
    \Input *pFormat     - printf format string
    \Input ...          - variable argument list

    \Version 05/02/2016 (eesponda)
*/
/********************************************************************************F*/
static void _ProtoHttpServLogPrintfVerbose(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread, uint16_t uCheckLevel, const char *pFormat, ...)
{
    char strText[2048];
    va_list Args;

    if (pHttpServ->uDebugLevel > uCheckLevel)
    {
        // format output
        va_start(Args, pFormat);
        ds_vsnprintf(strText, sizeof(strText), pFormat, Args);
        va_end(Args);

        // log this information
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "%s", strText);
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServGetThreadFromId

    \Description
        Get HttpServ thread based on specified thread id

    \Input *pHttpServ       - module state
    \Input uHttpThreadId    - thread id

    \Output
        ProtoHttpServThread *   - requested thread, or null if no match

    \Version 03/28/2018 (jbrookes)
*/
/********************************************************************************F*/
static ProtoHttpServThreadT *_ProtoHttpServGetThreadFromId(ProtoHttpServRefT *pHttpServ, uint32_t uHttpThreadId)
{
    ProtoHttpServThreadT *pHttpThread;
    // find thread with matching id
    for (pHttpThread = pHttpServ->pThreadListHead; (pHttpThread != NULL) && (pHttpThread->uHttpThreadId != uHttpThreadId); pHttpThread = pHttpThread->pNext)
        ;
    // return thead to caller if found, else null
    return(pHttpThread);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServGetResponseText

    \Description
        Return response text for specified response code

    \Input eResponseCode    - code to get text for

    \Output
        const char *        - response text

    \Version 09/13/2012 (jbrookes)
*/
/********************************************************************************F*/
static const char *_ProtoHttpServGetResponseText(ProtoHttpResponseE eResponseCode)
{
    int32_t iResponse;
    for (iResponse = 0; _ProtoHttpServ_Responses[iResponse].eResponse != PROTOHTTP_RESPONSE_PENDING; iResponse += 1)
    {
        if (_ProtoHttpServ_Responses[iResponse].eResponse == eResponseCode)
        {
            break;
        }
    }
    return(_ProtoHttpServ_Responses[iResponse].pResponseText);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServSSLCreate

    \Description
        Create ProtoSSL listen object

    \Input *pHttpServ   - module state
    \Input uPort        - port to bind
    \Input bReuseAddr   - TRUE to set SO_REUSEADDR, else FALSE
    \Input uFlags       - flag field, use PROTOHTTPSERV_FLAG_*

    \Output
        ProtoSSLRefT *  - socket ref, or NULL

    \Version 10/11/2013 (jbrookes)
*/
/********************************************************************************F*/
static ProtoSSLRefT *_ProtoHttpServSSLCreate(ProtoHttpServRefT *pHttpServ, uint16_t uPort, uint8_t bReuseAddr, uint32_t uFlags)
{
    struct sockaddr BindAddr;
    ProtoSSLRefT *pProtoSSL;
    int32_t iResult;

    // create the protossl ref
    if ((pProtoSSL = ProtoSSLCreate()) == NULL)
    {
        return(NULL);
    }

    // enable reuseaddr
    if (bReuseAddr)
    {
        ProtoSSLControl(pProtoSSL, 'radr', 1, 0, NULL);
    }

    // bind ssl to specified port
    SockaddrInit(&BindAddr, AF_INET);
    if (uFlags & PROTOHTTPSERV_FLAG_LOOPBACK)
    {
        SockaddrInSetAddr(&BindAddr, INADDR_LOOPBACK);
    }
    SockaddrInSetPort(&BindAddr, uPort);
    if ((iResult = ProtoSSLBind(pProtoSSL, &BindAddr, sizeof(BindAddr))) != SOCKERR_NONE)
    {
        _ProtoHttpServLogPrintf(pHttpServ, NULL, "error %d binding to port\n", iResult);
        ProtoSSLDestroy(pProtoSSL);
        return(NULL);
    }

    // return ref to caller
    return(pProtoSSL);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServThreadAlloc

    \Description
        Allocate and initialize an HttpThread object

    \Input *pHttpServ       - module state
    \Input *pProtoSSL       - ProtoSSL ref for this thread
    \Input *pRequestAddr    - address connection request was made from

    \Output
        ProtoHttpServThreadT *  - newly allocated HttpThread object, or NULL on failure

    \Version 03/21/2014 (jbrookes)
*/
/********************************************************************************F*/
static ProtoHttpServThreadT *_ProtoHttpServThreadAlloc(ProtoHttpServRefT *pHttpServ, ProtoSSLRefT *pProtoSSL, struct sockaddr *pRequestAddr)
{
    ProtoHttpServThreadT *pHttpThread;
    // allocate and init thread memory
    if ((pHttpThread = DirtyMemAlloc(sizeof(*pHttpThread), HTTPSERV_MEMID, pHttpServ->iMemGroup, pHttpServ->pMemGroupUserData)) == NULL)
    {
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "unable to alloc http thread\n");
        return(NULL);
    }
    ds_memclr(pHttpThread, sizeof(*pHttpThread));
    // allocate http thread streaming buffer
    if ((pHttpThread->pBuffer = DirtyMemAlloc(HTTPSERV_BUFSIZE_DEFAULT, HTTPSERV_MEMID, pHttpServ->iMemGroup, pHttpServ->pMemGroupUserData)) == NULL)
    {
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "unable to alloc http thread buffer\n");
        return(NULL);
    }
    // init thread members
    pHttpThread->pProtoSSL = pProtoSSL;
    pHttpThread->uHttpThreadId = pHttpServ->uCurrThreadId++;
    ds_memcpy_s(&pHttpThread->RequestAddr, sizeof(pHttpThread->RequestAddr), pRequestAddr, sizeof(*pRequestAddr));
    pHttpThread->iBufMax = HTTPSERV_BUFSIZE_DEFAULT;
    // return ref to caller
    return(pHttpThread);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServThreadFree

    \Description
        Free an HttpThread object

    \Input *pHttpServ       - module state
    \Input *pHttpThread     - HttpThread object to free

    \Version 03/21/2014 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpServThreadFree(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 2, "destroying http thread\n");
    if (pHttpThread->pProtoSSL != NULL)
    {
        ProtoSSLDestroy(pHttpThread->pProtoSSL);
    }
    if (pHttpThread->pBuffer != NULL)
    {
        DirtyMemFree(pHttpThread->pBuffer, HTTPSERV_MEMID, pHttpServ->iMemGroup, pHttpServ->pMemGroupUserData);
    }
    DirtyMemFree(pHttpThread, HTTPSERV_MEMID, pHttpServ->iMemGroup, pHttpServ->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServThreadReallocBuf

    \Description
        Reallocate buffer for HttpThread object

    \Input *pHttpServ       - module state
    \Input *pHttpThread     - HttpThread object to realloc buffer for
    \Input iBufSize         - size to reallocate to

    \Version 03/21/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServThreadReallocBuf(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread, int32_t iBufSize)
{
    char *pBuffer;
    if ((pBuffer = DirtyMemAlloc(iBufSize, HTTPMGR_MEMID, pHttpServ->iMemGroup, pHttpServ->pMemGroupUserData)) == NULL)
    {
        return(-1);
    }
    ds_memcpy(pBuffer, pHttpThread->pBuffer+pHttpThread->iBufOff, pHttpThread->iBufLen-pHttpThread->iBufOff);
    DirtyMemFree(pHttpThread->pBuffer, HTTPMGR_MEMID, pHttpServ->iMemGroup, pHttpServ->pMemGroupUserData);
    pHttpThread->pBuffer = pBuffer;
    pHttpThread->iBufLen -= pHttpThread->iBufOff;
    pHttpThread->iBufOff = 0;
    pHttpThread->iBufMax = iBufSize;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServDisconnect

    \Description
        Disconnect from a client

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)
    \Input bGraceful    - TRUE=graceful disconnect, FALSE=hard close

    \Version 10/11/2013 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpServDisconnect(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread, uint8_t bGraceful)
{
    if (bGraceful)
    {
        ProtoSSLDisconnect(pHttpThread->pProtoSSL);
        pHttpThread->iDisconnectTimer = NetTick();
    }
    else
    {
        pHttpThread->bConnected = FALSE;
        pHttpThread->iDisconnectTimer = NetTick() - (HTTPSERV_DISCTIME+1);
    }
    pHttpThread->bDisconnecting = TRUE;
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServResetState

    \Description
        Reset state after a transaction is completed

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Version 10/26/2013 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpServResetState(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    ds_memclr(&pHttpThread->RequestInfo, sizeof(pHttpThread->RequestInfo));
    ds_memclr(&pHttpThread->ResponseInfo, sizeof(pHttpThread->ResponseInfo));

    pHttpThread->bReceivedHeader = FALSE;
    pHttpThread->bReceivedBody = FALSE;
    pHttpThread->bSentBody = FALSE;
    pHttpThread->iContentSent = 0;
    pHttpThread->iBufLen = 0;
    pHttpThread->iBufOff = 0;
    pHttpThread->iChkLen = 0;
    pHttpThread->iChkOvr = 0;
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServParseHeader

    \Description
        Parse a received header

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, negative=failure

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServParseHeader(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    const char *pHdr, *pEnd;
    char strName[128], strValue[4*1024], strMethod[8];
    int32_t iResult;

    // parse http first line for method, url, query, and version
    pHdr = ProtoHttpUtilParseRequest(pHttpThread->RequestInfo.strHeader, strMethod, sizeof(strMethod),
        pHttpThread->RequestInfo.strUrl, sizeof(pHttpThread->RequestInfo.strUrl),
        pHttpThread->RequestInfo.strQuery, sizeof(pHttpThread->RequestInfo.strQuery),
        &pHttpThread->RequestInfo.eRequestType, &pHttpThread->bHttp1_0);
    // make sure it's a valid request
    if (pHdr == NULL)
    {
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "invalid request headers\n");
        pHttpThread->ResponseInfo.eResponseCode = PROTOHTTP_RESPONSE_BADREQUEST;
        return(-1);
    }

    // update address
    pHttpThread->RequestInfo.uAddr = SockaddrInGetAddr(&pHttpThread->RequestAddr);

    // log request info
    _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 1, "url=%s\n", pHttpThread->RequestInfo.strUrl);
    _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 1, "query=%s\n", pHttpThread->RequestInfo.strQuery);

    // detect if it is a 1.0 response; if so we default to closing the connection
    if (pHttpThread->bHttp1_0)
    {
        pHttpThread->bConnectionClose = TRUE;
    }

    // update the header so we dont have the unnecessary information
    memmove(pHttpThread->RequestInfo.strHeader, pHdr, pHdr - pHttpThread->RequestInfo.strHeader);

    // parse header
    for (iResult = 0; iResult >= 0; )
    {
        // get next header name and value from header buffer
        if ((iResult = ProtoHttpGetNextHeader(NULL, pHdr, strName, sizeof(strName), strValue, sizeof(strValue), &pEnd)) == 0)
        {
            // process header
            if (!ds_stricmp(strName, "connection"))
            {
                if (!ds_stricmp(strValue, "close"))
                {
                    pHttpThread->bConnectionClose = TRUE;
                }
                else if (!ds_stricmp(strValue, "keep-alive"))
                {
                    pHttpThread->bConnectionKeepAlive = TRUE;
                }
            }
            else if (!ds_stricmp(strName, "content-length") && !pHttpThread->bChunked)
            {
                pHttpThread->RequestInfo.iContentLength = ds_strtoll(strValue, NULL, 10);
            }
            else if (!ds_stricmp(strName, "content-type"))
            {
                ds_strnzcpy(pHttpThread->RequestInfo.strContentType, strValue, sizeof(pHttpThread->RequestInfo.strContentType));
            }
            else if (!ds_stricmp(strName, "transfer-encoding"))
            {
                if ((pHttpThread->bChunked = !ds_stricmp(strValue, "chunked")) == TRUE)
                {
                    pHttpThread->RequestInfo.iContentLength = -1;
                }
            }
            else if (!ds_stricmp(strName, "user-agent"))
            {
                ds_strnzcpy(pHttpThread->strUserAgent, strValue, sizeof(pHttpThread->strUserAgent));
            }
            else if (!ds_stricmp(strName, "expect"))
            {
                /* 
                RFC-2616 8.2.3 Use of 100 (Continue) Status
                Upon receiving a request which includes an Expect request-header
                field with the 100-continue expectation, an origin server MUST
                either response with 100 (Continue) status and continue to 
                read from the input stream, or respond with a final status code.
                The origin server MUST NOT wait for the request body buffer before
                the 100 (Continue) response. If it responds with a final status
                code, it MAY close the transport connection or it MAY continue
                to read and discard and discard the rest of the request. It
                MUST NOT perform the requested method of it returns a final
                status code
                
                Since we support large payloads we will just respond with 100
                and continue to process the body
                */
                if (!ds_stricmp(strValue, "100-continue"))
                {
                    /* Currently we don't have a way to send a response back in the
                       flow and continue to process. To do this I'm just going to
                       set the response code, format and send the header then reset
                       the state */
                    pHttpThread->ResponseInfo.eResponseCode = PROTOHTTP_RESPONSE_CONTINUE;
                    _ProtoHttpServFormatHeader(pHttpServ, pHttpThread);
                    _ProtoHttpServUpdateSendHeader(pHttpServ, pHttpThread);
                    pHttpThread->bFormattedHeader = FALSE;
                    pHttpThread->bSentHeader = FALSE;
                }
            }
            else
            {
                _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 2, "unprocessed header '%s'='%s'\n", strName, strValue);
            }
        }

        // move to next header
        pHdr = pEnd;
    }

    _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 0, "processing %s %s from %a:%d user-agent=%s\n", 
        _ProtoHttpServ_strRequestNames[pHttpThread->RequestInfo.eRequestType], pHttpThread->RequestInfo.strUrl, 
        SockaddrInGetAddr(&pHttpThread->RequestAddr), SockaddrInGetPort(&pHttpThread->RequestAddr),
        pHttpThread->strUserAgent);

    if ((iResult = pHttpServ->pHeaderCb(&pHttpThread->RequestInfo, &pHttpThread->ResponseInfo, pHttpServ->pUserData)) < 0)
    {
        return(-1);
    }

    pHttpThread->bParsedHeader = TRUE;
    pHttpThread->bProcessedRequest = FALSE;
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServFormatHeader

    \Description
        Format response header

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, negative=failure

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServFormatHeader(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    struct tm TmTime;
    char strTime[64];
    int32_t iBufLen, iBufMax = pHttpThread->iBufMax;

    // format a basic response header
    iBufLen = ds_snzprintf(pHttpThread->pBuffer, iBufMax, "HTTP/1.1 %d %s\r\n", pHttpThread->ResponseInfo.eResponseCode,
        _ProtoHttpServGetResponseText(pHttpThread->ResponseInfo.eResponseCode));

    // date header; specify current GMT time
    ds_secstotime(&TmTime, ds_timeinsecs());
    iBufLen += ds_snzprintf(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, "Date: %s\r\n",
        ds_timetostr(&TmTime, TIMETOSTRING_CONVERSION_RFC_0822, FALSE, strTime, sizeof(strTime)));

    // redirection?
    if (pHttpThread->ResponseInfo.strLocation[0] != '\0')
    {
        iBufLen += ds_snzprintf(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, "Location: %s\r\n", pHttpThread->ResponseInfo.strLocation);
    }
    if (pHttpThread->ResponseInfo.iContentLength > 0)
    {
        // set content type
        if (*pHttpThread->ResponseInfo.strContentType != '\0')
        {
            iBufLen += ds_snzprintf(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, "Content-type: %s\r\n", pHttpThread->ResponseInfo.strContentType);
        }

        // set content length or transfer-encoding
        if (pHttpThread->ResponseInfo.iChunkLength == 0)
        {
            iBufLen += ds_snzprintf(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, "Content-length: %qd\r\n", pHttpThread->ResponseInfo.iContentLength);
        }
        else
        {
            iBufLen += ds_snzprintf(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, "Transfer-Encoding: Chunked\r\n");
        }
    }
    else
    {
        iBufLen += ds_snzprintf(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, "Content-length: 0\r\n");
    }
    iBufLen += ds_snzprintf(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, "Server: %s/ProtoHttpServ %d.%d/DS %d.%d.%d.%d.%d (" DIRTYCODE_PLATNAME ")\r\n",
        pHttpServ->strServerName, (HTTPSERV_VERSION>>8)&0xff, HTTPSERV_VERSION&0xff, DIRTYSDK_VERSION_YEAR, DIRTYSDK_VERSION_SEASON,
        DIRTYSDK_VERSION_MAJOR, DIRTYSDK_VERSION_MINOR, DIRTYSDK_VERSION_PATCH);
    if (pHttpThread->bConnectionClose)
    {
        iBufLen += ds_snzprintf(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, "Connection: Close\r\n");
    }

    iBufLen += ds_strsubzcat(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, pHttpThread->ResponseInfo.strHeader, pHttpThread->ResponseInfo.iHeaderLen);

    // format footer, update thread data
    pHttpThread->iBufLen = ds_snzprintf(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, "\r\n") + iBufLen;
    pHttpThread->iBufOff = 0;
    pHttpThread->bFormattedHeader = TRUE;
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServProcessRequest

    \Description
        Process request

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        uint8_t         - true=request complete, false=request still pending
*/
/********************************************************************************F*/
static uint8_t _ProtoHttpServProcessRequest(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    int32_t iResult;
    
    // make user callback
    if ((iResult = pHttpServ->pRequestCb(&pHttpThread->RequestInfo, &pHttpThread->ResponseInfo, pHttpServ->pUserData)) < 0)
    {
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "error %d processing request\n", iResult);
        pHttpThread->bConnectionClose = TRUE;
        return(TRUE);
    }
    // if the request is not complete return
    else if (iResult != 0)
    {
        return(FALSE);
    }

    // if we are not transferring anymore data and the user doesn't want to keep the connection open, we can close the connection
    if (pHttpThread->ResponseInfo.iContentLength == 0 && !pHttpThread->bConnectionKeepAlive)
    {
        pHttpThread->bConnectionClose = TRUE;
    }

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServUpdateListen

    \Description
        Update HttpServ while listening for a new connection

    \Input *pHttpServ   - module state

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 09/11/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServUpdateListen(ProtoHttpServRefT *pHttpServ)
{
    ProtoSSLRefT *pProtoSSL;
    ProtoHttpServThreadT *pHttpThread;
    struct sockaddr RequestAddr;
    int32_t iAddrLen = sizeof(RequestAddr);

    // don't try Accept() if poll hint says nothing to read
#if 0 //$$ TODO - fix this to work on linux
    if (ProtoSSLStat(pHttpServ->pListenSSL, 'read', NULL, 0) == 0)
    {
        return(0);
    }
#endif
    // accept incoming connection
    SockaddrInit(&RequestAddr, AF_INET);
    if ((pProtoSSL = ProtoSSLAccept(pHttpServ->pListenSSL, pHttpServ->iSecure, &RequestAddr, &iAddrLen)) == NULL)
    {
        return(0);
    }
    // allocate an http thread
    if ((pHttpThread = _ProtoHttpServThreadAlloc(pHttpServ, pProtoSSL, &RequestAddr)) == NULL)
    {
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "unable to alloc http thread\n");
        ProtoSSLDestroy(pProtoSSL);
        return(0);
    }

    // is the thread list empty?
    if (pHttpServ->pThreadListHead == NULL)
    {
        pHttpServ->pThreadListHead = pHttpThread; // insert newly allocated httpthread as the first entry in the list
    }
    else
    {
        pHttpServ->pThreadListTail->pNext = pHttpThread; // enqueue newly allocated httpthread after the current tail entry in the list
    }
    pHttpServ->pThreadListTail = pHttpThread; // mark newly allocated httpthread as then new tail entry in the list

    // log connecting request
    _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 1, "connecting to %a:%d (%s)\n", SockaddrInGetAddr(&pHttpThread->RequestAddr),
        SockaddrInGetPort(&pHttpThread->RequestAddr), pHttpServ->iSecure ? "secure" : "insecure");

    // set server certificate and private key, if specified
    if (pHttpServ->iSecure)
    {
        ProtoSSLControl(pHttpThread->pProtoSSL, 'scrt', pHttpServ->iServerCertLen, 0, pHttpServ->pServerCert);
        ProtoSSLControl(pHttpThread->pProtoSSL, 'skey', pHttpServ->iServerKeyLen, 0, pHttpServ->pServerKey);
        ProtoSSLControl(pHttpThread->pProtoSSL, 'ccrt', pHttpServ->iClientCertLvl, 0, NULL);
        if (pHttpServ->uSslVersionMin != 0)
        {
            ProtoSSLControl(pHttpThread->pProtoSSL, 'vmin', pHttpServ->uSslVersionMin, 0, NULL);
        }
        if (pHttpServ->uSslVersion != 0)
        {
            ProtoSSLControl(pHttpThread->pProtoSSL, 'vers', pHttpServ->uSslVersion, 0, NULL);
        }
        ProtoSSLControl(pHttpThread->pProtoSSL, 'ciph', pHttpServ->uSslCiphers, 0, NULL);
        if (pHttpServ->strAlpn[0] != '\0')
        {
            ProtoSSLControl(pHttpThread->pProtoSSL, 'alpn', 0, 0, pHttpServ->strAlpn);
        }
    }
    pHttpThread->bConnected = FALSE;
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServSocketClose

    \Description
        Handle socket close on connect/send/recv

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)
    \Input iResult      - socket operation result
    \Input *pOperation  - operation type (conn, recv, send)

    \Version 10/31/2013 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpServSocketClose(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread, int32_t iResult, const char *pOperation)
{
    ProtoSSLAlertDescT AlertDesc;
    int32_t iAlert;

    if ((iAlert = ProtoSSLStat(pHttpThread->pProtoSSL, 'alrt', &AlertDesc, sizeof(AlertDesc))) != 0)
    {
        _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 0, "%s ssl alert %s (sslerr=%d, hResult=0x%08x); closing connection\n", (iAlert == 1) ? "recv" : "sent",
            AlertDesc.pAlertDesc, ProtoSSLStat(pHttpThread->pProtoSSL, 'fail', NULL, 0), ProtoSSLStat(pHttpThread->pProtoSSL, 'hres', NULL, 0));
    }
    else
    {
        _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 0, "%s returned %d (sockerr=%d, sslerr=%d, hResult=0x%08x); closing connection\n", pOperation, iResult,
            ProtoSSLStat(pHttpThread->pProtoSSL, 'serr', NULL, 0), ProtoSSLStat(pHttpThread->pProtoSSL, 'fail', NULL, 0), 
            ProtoSSLStat(pHttpThread->pProtoSSL, 'hres', NULL, 0));
    }

    _ProtoHttpServDisconnect(pHttpServ, pHttpThread, 0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServUpdateConnect

    \Description
        Update HttpServ while establishing a new connection

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 09/11/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServUpdateConnect(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    int32_t iStat;

    if ((iStat = ProtoSSLStat(pHttpThread->pProtoSSL, 'stat', NULL, 0)) > 0)
    {
        if (pHttpServ->iSecure)
        {
            char strTlsVersion[16], strCipherSuite[32], strResumed[] = " (resumed)";
            ProtoSSLStat(pHttpThread->pProtoSSL, 'vers', strTlsVersion, sizeof(strTlsVersion));
            ProtoSSLStat(pHttpThread->pProtoSSL, 'ciph', strCipherSuite, sizeof(strCipherSuite));
            _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 1, "connected to %a:%d using %s and %s%s\n",
                SockaddrInGetAddr(&pHttpThread->RequestAddr), SockaddrInGetPort(&pHttpThread->RequestAddr),
                strTlsVersion, strCipherSuite, ProtoSSLStat(pHttpThread->pProtoSSL, 'resu', NULL, 0) ? strResumed : "");
        }
        else
        {
            _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 1, "connected to %a:%d (insecure)\n",
                SockaddrInGetAddr(&pHttpThread->RequestAddr), SockaddrInGetPort(&pHttpThread->RequestAddr));
        }
        pHttpThread->bConnected = TRUE;
        pHttpThread->iIdleTimer = NetTick();
        return(1);
    }
    else if (iStat < 0)
    {
        _ProtoHttpServSocketClose(pHttpServ, pHttpThread, iStat, "conn");
        return(-1);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServUpdateRecvHeader

    \Description
        Update HttpServ while receiving header

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServUpdateRecvHeader(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    int32_t iResult;

    if ((iResult = ProtoSSLRecv(pHttpThread->pProtoSSL, pHttpThread->pBuffer+pHttpThread->iBufLen, pHttpThread->iBufMax-pHttpThread->iBufLen)) > 0)
    {
        char *pHdrEnd;
        int32_t iHdrLen;

        _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 1, "recv %d bytes\n", iResult);

        // update received data size
        pHttpThread->iBufLen += iResult;

        // reset idle timeout
        pHttpThread->iIdleTimer = NetTick();

        // check for header termination
        if ((pHdrEnd = strstr(pHttpThread->pBuffer, "\r\n\r\n")) != NULL)
        {
            // we want to keep the final header EOL
            pHdrEnd += 2;

            // copy header to buffer
            iHdrLen = (int32_t)(pHdrEnd - pHttpThread->pBuffer);
            ds_strsubzcpy(pHttpThread->RequestInfo.strHeader, sizeof(pHttpThread->RequestInfo.strHeader), pHttpThread->pBuffer, iHdrLen);
            if (pHttpServ->uDebugLevel > 1)
            {
                _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "received %d byte header\n", iHdrLen);
                NetPrintWrap(pHttpThread->RequestInfo.strHeader, 132);
            }

            // skip header/body seperator
            pHdrEnd += 2;
            iHdrLen += 2;

            // remove header from buffer
            memmove(pHttpThread->pBuffer, pHdrEnd, pHttpThread->iBufLen - iHdrLen);
            pHttpThread->iBufLen -= iHdrLen;
            pHttpThread->pBuffer[pHttpThread->iBufLen] = '\0';

            // we've received the header
            pHttpThread->bReceivedHeader = TRUE;

            // reset for next state
            pHttpThread->bConnectionClose = FALSE;
            pHttpThread->bParsedHeader = FALSE;
            pHttpThread->bFormattedHeader = FALSE;
            pHttpThread->bSentHeader = FALSE;
        }

        // return whether we have parsed the header or not
        iResult = pHttpThread->bReceivedHeader ? 1 : 0;
    }
    else if (iResult < 0)
    {
        _ProtoHttpServSocketClose(pHttpServ, pHttpThread, iResult, "recv");
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServProcessData

    \Description
        Process received data

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 03/20/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServProcessData(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    int32_t iDataSize;
    if ((iDataSize = pHttpServ->pReceiveCb(&pHttpThread->RequestInfo, pHttpThread->pBuffer+pHttpThread->iBufOff, pHttpThread->iBufLen-pHttpThread->iBufOff, pHttpServ->pUserData)) > 0)
    {
        pHttpThread->RequestInfo.iContentRecv += iDataSize;
        pHttpThread->iBufOff += iDataSize;

        _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 1, "wrote %d bytes to file (%qd total)\n", iDataSize, pHttpThread->RequestInfo.iContentRecv);

        // if we've written all of the data reset buffer pointers
        if (pHttpThread->iBufOff == pHttpThread->iBufLen)
        {
            pHttpThread->iBufOff = 0;
            pHttpThread->iBufLen = 0;
        }
    }
    return(iDataSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServCompactBuffer

    \Description
        Compact the buffer

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Version 12/06/2019 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpServCompactBuffer(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    // compact the buffer
    if (pHttpThread->iBufOff < pHttpThread->iBufLen)
    {
        memmove(pHttpThread->pBuffer, pHttpThread->pBuffer+pHttpThread->iBufOff, pHttpThread->iBufLen-pHttpThread->iBufOff);
    }
    pHttpThread->iBufLen -= pHttpThread->iBufOff;
    pHttpThread->iBufOff = 0;
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServProcessChunkData

    \Description
        Process received chunk data

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 03/20/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServProcessChunkData(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    int32_t iChkLeft, iDataSize;

    // do we need a new chunk header?
    if (pHttpThread->iChkLen == 0)
    {
        char *s = pHttpThread->pBuffer+pHttpThread->iBufOff, *s2;
        char *t = pHttpThread->pBuffer+pHttpThread->iBufLen-1;

        // make sure we have a complete chunk header
        for (s2=s; (s2 < t) && ((s2[0] != '\r') || (s2[1] != '\n')); s2++)
            ;
        if (s2 == t)
        {
            // if we're out of room, compact buffer
            if (pHttpThread->iBufLen == pHttpThread->iBufMax)
            {
                _ProtoHttpServCompactBuffer(pHttpServ, pHttpThread);
            }
            return(0);
        }

        // read chunk header; handle end of input
        if ((pHttpThread->iChkLen = (int32_t)strtol(s, NULL, 16)) == 0)
        {
            _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 1, "processing end chunk\n");
            pHttpThread->iBufOff = 0;
            pHttpThread->iBufLen = 0;
            pHttpThread->RequestInfo.iContentLength = pHttpThread->RequestInfo.iContentRecv;
            return(0);
        }
        _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 1, "processing %d byte chunk\n", pHttpThread->iChkLen);

        // reset chunk read counter
        pHttpThread->iChkRcv = 0;

        // consume chunk header and \r\n trailer
        pHttpThread->iBufOff += s2-s+2;
    }

    // if chunk size plus trailer is bigger than our buffer, realloc our buffer larger to accomodate, so we can buffer an entire chunk in our buffer
    if ((pHttpThread->iChkLen+2) > pHttpThread->iBufMax)
    {
        _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 0, "increasing buffer to handle %d byte chunk\n", pHttpThread->iChkLen);
        if (_ProtoHttpServThreadReallocBuf(pHttpServ, pHttpThread, pHttpThread->iChkLen+2) < 0) // chunk plus trailer
        {
            return(-1);
        }
    }

    // get how much of the chunk we have left to read
    iChkLeft = pHttpThread->iChkLen - pHttpThread->iChkRcv;

    // get data size avaialble; make sure we have all of the remaining data and chunk trailer available
    if ((iDataSize = pHttpThread->iBufLen-pHttpThread->iBufOff) < (iChkLeft+2))
    {
        // compact buffer to make room for more data
        _ProtoHttpServCompactBuffer(pHttpServ, pHttpThread);
        return(0);
    }
    // clamp data size to whatever we have left to read
    iDataSize = iChkLeft;

    // write the data
    if ((iDataSize = pHttpServ->pReceiveCb(&pHttpThread->RequestInfo, pHttpThread->pBuffer+pHttpThread->iBufOff, iDataSize, pHttpServ->pUserData)) > 0)
    {
        _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 1, "wrote %d bytes to file (%qd total)\n", iDataSize, pHttpThread->RequestInfo.iContentRecv+iDataSize);

        pHttpThread->RequestInfo.iContentRecv += iDataSize;
        pHttpThread->iBufOff += iDataSize;
        pHttpThread->iChkRcv += iDataSize;

        // did we consume all of the chunk data?
        if (pHttpThread->iChkRcv == pHttpThread->iChkLen)
        {
            // consume chunk trailer, reset chunk length
            pHttpThread->iBufOff += 2;
            pHttpThread->iChkLen = 0;
        }

        // if we've written all of the data reset buffer pointers
        if (pHttpThread->iBufOff == pHttpThread->iBufLen)
        {
            pHttpThread->iBufOff = 0;
            pHttpThread->iBufLen = 0;
        }
    }
    return(iDataSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServUpdateRecvBody

    \Description
        Update HttpServ while receiving body

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServUpdateRecvBody(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    int32_t iDataSize, iResult;

    while ((pHttpThread->RequestInfo.iContentRecv < pHttpThread->RequestInfo.iContentLength) || (pHttpThread->RequestInfo.iContentLength == -1))
    {
        // need new data?
        while (pHttpThread->iBufLen < (signed)pHttpThread->iBufMax)
        {
            iResult = ProtoSSLRecv(pHttpThread->pProtoSSL, pHttpThread->pBuffer + pHttpThread->iBufLen, pHttpThread->iBufMax - pHttpThread->iBufLen);
            if (iResult > 0)
            {
                pHttpThread->iBufLen += iResult;
                _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 1, "recv %d bytes\n", iResult);
                // reset idle timeout
                pHttpThread->iIdleTimer = NetTick();
            }
            else if ((iResult == SOCKERR_CLOSED) && (pHttpThread->RequestInfo.iContentLength == -1))
            {
                _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "keep-alive connection closed by remote host\n");
                pHttpThread->RequestInfo.iContentLength = pHttpThread->RequestInfo.iContentRecv + pHttpThread->iBufLen;
                break;
            }
            else if (iResult < 0)
            {
                _ProtoHttpServSocketClose(pHttpServ, pHttpThread, iResult, "recv");
                return(-2);
            }
            else
            {
                break;
            }
        }

        // if we have data, write it out
        if (pHttpThread->iBufLen > 0)
        {
            if (!pHttpThread->bChunked)
            {
                // write out as much data as we can
                iDataSize = _ProtoHttpServProcessData(pHttpServ, pHttpThread);
            }
            else
            {
                // write out as many chunks as we have
                while((iDataSize = _ProtoHttpServProcessChunkData(pHttpServ, pHttpThread)) > 0)
                    ;
            }

            if (iDataSize < 0)
            {
                _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "error %d writing to file\n", iDataSize);
                pHttpThread->ResponseInfo.eResponseCode = PROTOHTTP_RESPONSE_INTERNALSERVERERROR;
                return(-1);
            }
        }
        else
        {
            break;
        }
    }

    // check for upload completion
    if (pHttpThread->RequestInfo.iContentRecv == pHttpThread->RequestInfo.iContentLength)
    {
        // done receiving response
        _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 1, "received upload of %qd bytes\n", pHttpThread->RequestInfo.iContentRecv);

        // trigger callback to indicate transaction is complete
        pHttpServ->pReceiveCb(&pHttpThread->RequestInfo, NULL, 0, pHttpServ->pUserData);

        // done with response
        pHttpThread->bReceivedBody = TRUE;
        pHttpThread->ResponseInfo.eResponseCode = PROTOHTTP_RESPONSE_CREATED;
        return(1);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServUpdateSendHeader

    \Description
        Update HttpServ while sending header

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServUpdateSendHeader(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    int32_t iResult;

    // send data to requester
    if ((iResult = ProtoSSLSend(pHttpThread->pProtoSSL, pHttpThread->pBuffer + pHttpThread->iBufOff, pHttpThread->iBufLen - pHttpThread->iBufOff)) > 0)
    {
        pHttpThread->iBufOff += iResult;

        // reset idle timeout
        pHttpThread->iIdleTimer = NetTick();

        // are we done?
        if (pHttpThread->iBufOff == pHttpThread->iBufLen)
        {
            if (pHttpServ->uDebugLevel > 1)
            {
                _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "sent %d byte header\n", pHttpThread->iBufOff);
                NetPrintWrap(pHttpThread->pBuffer, 132);
            }
            if (pHttpThread->ResponseInfo.iContentLength == 0)
            {
                _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 0, "sent %d response (no body)\n", pHttpThread->ResponseInfo.eResponseCode);
            }
            pHttpThread->bSentHeader = TRUE;
            pHttpThread->iBufOff = 0;
            pHttpThread->iBufLen = 0;
            iResult = 1;
        }
        else
        {
            iResult = 0;
        }
    }
    else if (iResult < 0)
    {
        _ProtoHttpServSocketClose(pHttpServ, pHttpThread, iResult, "send");
        pHttpThread->iBufOff = pHttpThread->iBufLen;
        return(-1);
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServUpdateSendBody

    \Description
        Update HttpServ while sending response

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServUpdateSendBody(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    int32_t iResult, iReadOff, iReadMax;
    uint8_t bError;

    // reserve space for chunk header
    if (pHttpThread->ResponseInfo.iChunkLength != 0)
    {
        iReadOff = 10;
        iReadMax = pHttpThread->ResponseInfo.iChunkLength;
    }
    else
    {
        iReadOff = 0;
        iReadMax = pHttpThread->iBufMax;
    }

    // while there is content to be sent
    for (bError = FALSE; pHttpThread->iContentSent < pHttpThread->ResponseInfo.iContentLength; )
    {
        // need new data?
        if (pHttpThread->iBufOff == pHttpThread->iBufLen)
        {
            _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 2, "sent=%qd len=%qd\n", pHttpThread->iContentSent, pHttpThread->ResponseInfo.iContentLength);
            // reset chunk overhead tracker
            pHttpThread->iChkOvr = 0;

            // read the data
            if ((pHttpThread->iBufLen = pHttpServ->pSendCb(&pHttpThread->ResponseInfo, pHttpThread->pBuffer+iReadOff, iReadMax-iReadOff, pHttpServ->pUserData)) > 0)
            {
                pHttpThread->ResponseInfo.iContentRead += pHttpThread->iBufLen;
                pHttpThread->iBufOff = 0;
                _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 2, "read %d bytes from file (%qd total)\n", pHttpThread->iBufLen, pHttpThread->ResponseInfo.iContentRead);

                // if chunked, fill in chunk header based on amount of data read
                if (pHttpThread->ResponseInfo.iChunkLength != 0)
                {
                    char strHeader[11];

                    // format chunk header before data
                    ds_snzprintf(strHeader, sizeof(strHeader), "%08x\r\n", pHttpThread->iBufLen);
                    ds_memcpy(pHttpThread->pBuffer, strHeader, iReadOff);
                    pHttpThread->iBufLen += iReadOff;
                    pHttpThread->iChkOvr += iReadOff;

                    // add chunk trailer after data
                    pHttpThread->pBuffer[pHttpThread->iBufLen+0] = '\r';
                    pHttpThread->pBuffer[pHttpThread->iBufLen+1] = '\n';
                    pHttpThread->iBufLen += 2;
                    pHttpThread->iChkOvr += 2;

                    // if this is the last of the data, add terminating chunk
                    if (pHttpThread->ResponseInfo.iContentRead == pHttpThread->ResponseInfo.iContentLength)
                    {
                        ds_snzprintf(strHeader, sizeof(strHeader), "%08x\r\n", 0);
                        ds_memcpy(pHttpThread->pBuffer+pHttpThread->iBufLen, strHeader, iReadOff);
                        pHttpThread->iBufLen += iReadOff;
                        pHttpThread->iChkOvr += iReadOff;

                        pHttpThread->pBuffer[pHttpThread->iBufLen+0] = '\r';
                        pHttpThread->pBuffer[pHttpThread->iBufLen+1] = '\n';
                        pHttpThread->iBufLen += 2;
                        pHttpThread->iChkOvr += 2;
                    }
                }
            }
            else
            {
                _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "error %d reading from file\n", pHttpThread->iBufLen);
                _ProtoHttpServDisconnect(pHttpServ, pHttpThread, FALSE);
                bError = TRUE;
                break;
            }
        }

        // do we have buffered data to send?
        if (pHttpThread->iBufOff < pHttpThread->iBufLen)
        {
            int32_t iSendLen = pHttpThread->iBufLen - pHttpThread->iBufOff;
            #if HTTPSERV_CHUNKWAITTEST
            if (iSendLen > 1) iSendLen -= 1;
            #endif
            iResult = ProtoSSLSend(pHttpThread->pProtoSSL, pHttpThread->pBuffer + pHttpThread->iBufOff, iSendLen);
            if (iResult > 0)
            {
                pHttpThread->iBufOff += iResult;
                pHttpThread->iContentSent += iResult;
                if (pHttpThread->iChkOvr > 0)
                {
                    pHttpThread->iContentSent -= pHttpThread->iChkOvr;
                    pHttpThread->iChkOvr = 0;
                }
                _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 2, "sent %d bytes (%qd total)\n", iResult, pHttpThread->iContentSent);
                // reset idle timeout
                pHttpThread->iIdleTimer = NetTick();
                #if HTTPSERV_CHUNKWAITTEST
                NetConnSleep(1000);
                #endif
            }
            else if (iResult < 0)
            {
                _ProtoHttpServSocketClose(pHttpServ, pHttpThread, iResult, "send");
                bError = TRUE;
                break;
            }
            else
            {
                break;
            }
        }
    }

    // check for send completion
    if ((pHttpThread->iContentSent == pHttpThread->ResponseInfo.iContentLength) || (bError == TRUE))
    {
        // done sending response
        if (pHttpThread->ResponseInfo.iContentLength != 0)
        {
            _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 0, "sent %d response (%qd bytes)\n", pHttpThread->ResponseInfo.eResponseCode, pHttpThread->iContentSent);
        }

        // trigger callback to indicate transaction is complete
        pHttpServ->pSendCb(&pHttpThread->ResponseInfo, NULL, 0, pHttpServ->pUserData);

        // done with response
        pHttpThread->bSentBody = TRUE;
        return(1);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServUpdateThread

    \Description
        Update an HttpServ thread

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Version 09/11/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpServUpdateThread(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    uint32_t uCurTick = NetTick();
    int32_t iTimeout;

    // if no thread, or we are disconnected, nothing to update
    if (pHttpThread->pProtoSSL == NULL)
    {
        return;
    }

    // update ProtoSSL object
    ProtoSSLUpdate(pHttpThread->pProtoSSL);

    // poll for connection completion
    if ((pHttpThread->bConnected == FALSE) && (_ProtoHttpServUpdateConnect(pHttpServ, pHttpThread) <= 0))
    {
        return;
    }

    // get timeout based on whether we are processing a request or not
    iTimeout = pHttpThread->bReceivedHeader ? pHttpServ->iRequestTimeout : pHttpServ->iIdleTimeout;

    // see if we should timeout the connection
    if (NetTickDiff(uCurTick, pHttpThread->iIdleTimer) > iTimeout)
    {
        if (pHttpThread->bReceivedHeader == TRUE)
        {
            _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "closing connection (request timeout)\n");
        }
        else
        {
            _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 1, "closing connection (idle timeout)\n");
        }
        _ProtoHttpServDisconnect(pHttpServ, pHttpThread, FALSE);
        return;
    }

    // receive the header
    if ((pHttpThread->bReceivedHeader == FALSE) && (_ProtoHttpServUpdateRecvHeader(pHttpServ, pHttpThread) <= 0))
    {
        return;
    }

    // if disconnecting, early out (this is intentionally after recv)
    if (pHttpThread->bDisconnecting)
    {
        return;
    }

    // parse headers
    if ((pHttpThread->bParsedHeader == FALSE) && (_ProtoHttpServParseHeader(pHttpServ, pHttpThread) < 0))
    {
        // If parsing header failed do not continue to do any processing,
        // send a response back and close the connection
        pHttpThread->bParsedHeader = TRUE;
        pHttpThread->bReceivedBody = TRUE;
        pHttpThread->bProcessedRequest = TRUE;
        pHttpThread->bConnectionClose = TRUE;
        return;
    }

    // if data needed to download
    if (pHttpThread->RequestInfo.iContentLength > 0 || pHttpThread->RequestInfo.iContentLength == -1)
    {
        if (pHttpThread->bReceivedBody == FALSE)
        {
            int32_t iResult;
            if ((iResult = _ProtoHttpServUpdateRecvBody(pHttpServ, pHttpThread)) < 0)
            {
                // If receiving/processing the body failed do not continue to receive,
                // send a response back and close the connection
                pHttpThread->bReceivedBody = TRUE;
                pHttpThread->bProcessedRequest = TRUE;
                pHttpThread->bConnectionClose = TRUE;
                return;
            }
            else if (iResult == 0)
            {
                return;
            }
        }
    }

    // process the request
    if (pHttpThread->bProcessedRequest == FALSE)
    {
        if (_ProtoHttpServProcessRequest(pHttpServ, pHttpThread) == FALSE)
        {
            return;
        }
        // mark that we've processed the request
        pHttpThread->bProcessedRequest = TRUE;
    }

    // format response header
    if ((pHttpThread->bFormattedHeader == FALSE) && (_ProtoHttpServFormatHeader(pHttpServ, pHttpThread) < 0))
    {
        return;
    }

    // send response header
    if ((pHttpThread->bSentHeader == FALSE) && (_ProtoHttpServUpdateSendHeader(pHttpServ, pHttpThread) <= 0))
    {
        return;
    }

    // process body data, if any
    if (pHttpThread->ResponseInfo.iContentLength > 0 || pHttpThread->ResponseInfo.iChunkLength > 0)
    {
        if ((pHttpThread->bSentBody == FALSE) && (_ProtoHttpServUpdateSendBody(pHttpServ, pHttpThread) <= 0))
        {
            return;
        }
    }

    // make sure we've sent all of the data
    if ((pHttpThread->pProtoSSL != NULL) && (ProtoSSLStat(pHttpThread->pProtoSSL, 'send', NULL, 0) > 0))
    {
        return;
    }

    // if no keep-alive initiate disconnection
    if (pHttpThread->bConnectionClose == TRUE)
    {
        _ProtoHttpServLogPrintfVerbose(pHttpServ, pHttpThread, 0, "closing connection\n");
        _ProtoHttpServDisconnect(pHttpServ, pHttpThread, TRUE);
    }

    // reset transaction state
    _ProtoHttpServResetState(pHttpServ, pHttpThread);
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoHttpServCreate

    \Description
        Create an HttpServ instance

    \Input iPort            - port to listen on
    \Input iSecure          - TRUE if secure, else FALSE
    \Input *pName           - name of server, used in Server: header

    \Output
        ProtoHttpServRefT * - pointer to state, or null on failure

    \Version 12/11/2013 (jbrookes)
*/
/********************************************************************************F*/
ProtoHttpServRefT *ProtoHttpServCreate(int32_t iPort, int32_t iSecure, const char *pName)
{
    return(ProtoHttpServCreate2(iPort, iSecure, pName, 0));
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpServCreate2

    \Description
        Create an HttpServ instance

    \Input iPort            - port to listen on
    \Input iSecure          - TRUE if secure, else FALSE
    \Input *pName           - name of server, used in Server: header
    \Input uFlags           - flag field, use PROTOHTTPSERV_FLAG_*

    \Output
        ProtoHttpServRefT * - pointer to state, or null on failure

    \Version 02/22/2016 (amakoukji)
*/
/********************************************************************************F*/
ProtoHttpServRefT *ProtoHttpServCreate2(int32_t iPort, int32_t iSecure, const char *pName, uint32_t uFlags)
{
    ProtoHttpServRefT *pHttpServ;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate module state
    if ((pHttpServ = DirtyMemAlloc(sizeof(*pHttpServ), HTTPSERV_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        _ProtoHttpServLogPrintf(NULL, NULL, "unable to allocate module state\n");
        return(NULL);
    }
    ds_memclr(pHttpServ, sizeof(*pHttpServ));
    // save memgroup (will be used in ProtoHttpDestroy)
    pHttpServ->iMemGroup = iMemGroup;
    pHttpServ->pMemGroupUserData = pMemGroupUserData;
    pHttpServ->uDebugLevel = 1;

    // create a protossl listen ref
    if ((pHttpServ->pListenSSL = _ProtoHttpServSSLCreate(pHttpServ, iPort, TRUE, uFlags)) == NULL)
    {
        _ProtoHttpServLogPrintf(pHttpServ, NULL, "could not create ssl ref on port %d\n", iPort);
        ProtoHttpServDestroy(pHttpServ);
        return(NULL);
    }
    // start listening
    if (ProtoHttpServListen(pHttpServ, 2) != SOCKERR_NONE)
    {
        ProtoHttpServDestroy(pHttpServ);
        return(NULL);
    }

    // save settings
    pHttpServ->iSecure = iSecure;
    ds_strnzcpy(pHttpServ->strServerName, pName, sizeof(pHttpServ->strServerName));

    // initialize default values
    pHttpServ->uCurrThreadId = 1;
    pHttpServ->uSslCiphers = PROTOSSL_CIPHER_ALL;
    pHttpServ->iIdleTimeout = HTTPSERV_IDLETIMEOUT_DEFAULT;
    pHttpServ->iRequestTimeout = HTTPSERV_REQUESTTIMEOUT_DEFAULT;

    // log success
    _ProtoHttpServLogPrintfVerbose(pHttpServ, NULL, 0, "listening on port %d (%s)\n", iPort, iSecure ? "secure" : "insecure");

    // return new ref to caller
    return(pHttpServ);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpServListen

    \Description
        Listens on the listen socket with the specified settings

    \Input *pHttpServ   - module state
    \Input iBacklog     - listen backlog setting

    \Output
        int32_t         - result of the listen call

    \Notes
        This function is not required to be called for the server to work.
        The create functions call it on your behalf with our default backlog,
        this is only required for users that want to update settings.

    \Version 05/02/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t ProtoHttpServListen(ProtoHttpServRefT *pHttpServ, int32_t iBacklog)
{
    int32_t iResult;

    // start listening
    if ((iResult = ProtoSSLListen(pHttpServ->pListenSSL, iBacklog)) != SOCKERR_NONE)
    {
        _ProtoHttpServLogPrintf(pHttpServ, NULL, "error listening on socket (err=%d)\n", iResult);
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpServDestroy

    \Description
        Destroy an HttpServ instance

    \Input *pHttpServ       - module state to destroy

    \Version 12/11/2013 (jbrookes)
*/
/********************************************************************************F*/
void ProtoHttpServDestroy(ProtoHttpServRefT *pHttpServ)
{
    ProtoHttpServThreadT *pHttpThread, *pHttpThreadNext;

    if (pHttpServ->pListenSSL != NULL)
    {
        ProtoSSLDestroy(pHttpServ->pListenSSL);
    }

    for (pHttpThread = pHttpServ->pThreadListHead; pHttpThread != NULL; pHttpThread = pHttpThreadNext)
    {
        pHttpThreadNext = pHttpThread->pNext;
        _ProtoHttpServThreadFree(pHttpServ, pHttpThread);
    }

    DirtyMemFree(pHttpServ, HTTPSERV_MEMID, pHttpServ->iMemGroup, pHttpServ->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpServCallback

    \Description
        Set required callback functions for HttpServ to use.

    \Input *pHttpServ   - module state
    \Input *pRequestCb  - request handler callback
    \Input *pReceiveCb  - inbound data handler callback
    \Input *pSendCb     - outbound data handler callback
    \Input *pHeaderCb   - inbound header callback
    \Input *pLogCb      - logging function to use (optional)
    \Input *pUserData   - user data for callbacks

    \Version 12/11/2013 (jbrookes)
*/
/********************************************************************************F*/
void ProtoHttpServCallback(ProtoHttpServRefT *pHttpServ, ProtoHttpServRequestCbT *pRequestCb, ProtoHttpServReceiveCbT *pReceiveCb, ProtoHttpServSendCbT *pSendCb, ProtoHttpServHeaderCbT *pHeaderCb, ProtoHttpServLogCbT *pLogCb, void *pUserData)
{
    pHttpServ->pRequestCb = pRequestCb;
    pHttpServ->pReceiveCb = pReceiveCb;
    pHttpServ->pSendCb = pSendCb;
    pHttpServ->pHeaderCb = pHeaderCb;
    pHttpServ->pLogCb = pLogCb;
    pHttpServ->pUserData = pUserData;
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpServControl

    \Description
        Set behavior of module, based on selector input

    \Input *pHttpServ   - reference pointer
    \Input iSelect      - control selector
    \Input iValue       - selector specific
    \Input iValue2      - selector specific
    \Input *pValue      - selector specific

    \Output
        int32_t         - selector specific

    \Notes
        Selectors are:

        \verbatim
            SELECTOR    DESCRIPTION
            'alpn'      Set alpn string (pValue=string)
            'ccrt'      Set client certificate level (0=disabled, 1=requested, 2=required)
            'ciph'      Set supported cipher suites (iValue=PROTOSSL_CIPHER_*; default=PROTOSSL_CIPHER_ALL)
            'idle'      Set idle timeout (iValue=timeout; default=5 minutes)
            'scrt'      Set certificate (pValue=cert, iValue=len)
            'skey'      Set private key (pValue=key, iValue=len)
            'spam'      Set debug level (iValue=level)
            'time'      Set request timeout in milliseconds (iValue=timeout; default=30 seconds)
            'vers'      Set server-supported maximum SSL version (iValue=version; default=ProtoSSL default)
            'vmin'      Set server-required minimum SSL version (iValue=version; default=ProtoSSL default)
        \endverbatim

    \Version 12/12/2013 (jbrookes)
*/
/*******************************************************************************F*/
int32_t ProtoHttpServControl(ProtoHttpServRefT *pHttpServ, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iSelect == 'alpn')
    {
        ds_strnzcpy(pHttpServ->strAlpn, (const char *)pValue, sizeof(pHttpServ->strAlpn));
        return(0);
    }
    if (iSelect == 'ccrt')
    {
        pHttpServ->iClientCertLvl = iValue;
        return(0);
    }
    if (iSelect == 'ciph')
    {
        pHttpServ->uSslCiphers = iValue;
        return(0);
    }
    if (iSelect == 'idle')
    {
        pHttpServ->iIdleTimeout = iValue;
        return(0);
    }
    if (iSelect == 'scrt')
    {
        pHttpServ->pServerCert = (char *)pValue;
        pHttpServ->iServerCertLen = iValue;
        return(0);
    }
    if (iSelect == 'skey')
    {
        pHttpServ->pServerKey = (char *)pValue;
        pHttpServ->iServerKeyLen = iValue;
        return(0);
    }
    if (iSelect == 'spam')
    {
        pHttpServ->uDebugLevel = (uint16_t)iValue;
        return(0);
    }
    if (iSelect == 'time')
    {
        pHttpServ->iRequestTimeout = iValue;
        return(0);
    }
    if (iSelect == 'vers')
    {
        pHttpServ->uSslVersion = iValue;
        return(0);
    }
    if (iSelect == 'vmin')
    {
        pHttpServ->uSslVersionMin = iValue;
        return(0);
    }
    // unsupported
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpServControl2

    \Description
        Set behavior of module, based on selector input

    \Input *pHttpServ   - reference pointer
    \Input iThread      - thread to apply control to, or -1 for a global setting
    \Input iSelect      - control selector
    \Input iValue       - selector specific
    \Input iValue2      - selector specific
    \Input *pValue      - selector specific

    \Output
        int32_t         - selector specific

    \Notes
        See ProtoHttpServControl for global settings.  Thread-relative control
        selectors are passed through to ProtoSSL.

    \Version 03/28/2018 (jbrookes)
*/
/*******************************************************************************F*/
int32_t ProtoHttpServControl2(ProtoHttpServRefT *pHttpServ, int32_t iThread, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue)
{
    ProtoHttpServThreadT *pHttpThread;

    if (iThread == -1)
    {
        return(ProtoHttpServControl(pHttpServ, iSelect, iValue, iValue2, pValue));
    }

    if ((pHttpThread = _ProtoHttpServGetThreadFromId(pHttpServ, iThread)) == NULL)
    {
        _ProtoHttpServLogPrintf(pHttpServ, NULL, "invalid thread id %d\n", iThread);
        return(-1);
    }

    return(ProtoSSLControl(pHttpThread->pProtoSSL, iSelect, iValue, iValue2, pValue));
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpServStatus

    \Description
        Return status of module, based on selector input

    \Input *pHttpServ   - module state
    \Input iSelect      - info selector (see Notes)
    \Input *pBuffer     - [out] storage for selector-specific output
    \Input iBufSize     - size of buffer

    \Output
        int32_t         - selector specific

    \Notes
        Selectors are:

    \verbatim
        SELECTOR    RETURN RESULT
    \endverbatim

    \Version 12/12/2013 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpServStatus(ProtoHttpServRefT *pHttpServ, int32_t iSelect, void *pBuffer, int32_t iBufSize)
{
    // unsupported selector
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpServUpdate

    \Description
        Update HttpServ module

    \Input *pHttpServ   - module state

    \Version 10/30/2013 (jbrookes)
*/
/********************************************************************************F*/
void ProtoHttpServUpdate(ProtoHttpServRefT *pHttpServ)
{
    ProtoHttpServThreadT *pHttpThread, *pHttpThreadNext, *pHttpThreadPrevious = NULL, **ppHttpThread;

    // if we don't have a listen object don't do anything at all
    if (pHttpServ->pListenSSL == NULL)
    {
        return;
    }

    // listen for a response
    _ProtoHttpServUpdateListen(pHttpServ);

    // update http threads
    for (pHttpThread = pHttpServ->pThreadListHead; pHttpThread != NULL; pHttpThread = pHttpThread->pNext)
    {
        _ProtoHttpServUpdateThread(pHttpServ, pHttpThread);
    }

    // clean up expired threads
    for (ppHttpThread = &pHttpServ->pThreadListHead; *ppHttpThread != NULL; )
    {
        pHttpThread = *ppHttpThread;
        if (pHttpThread->bDisconnecting && (NetTickDiff(NetTick(), pHttpThread->iDisconnectTimer) > HTTPSERV_DISCTIME))
        {
            pHttpThreadNext = pHttpThread->pNext;

            // if dealing with tail of list, move tail pointer to previous entry (or NULL it if tail is last entry in list)
            if (pHttpThread == pHttpServ->pThreadListTail)
            {
                pHttpServ->pThreadListTail = pHttpThreadPrevious;
                if (pHttpServ->pThreadListTail != NULL)
                {
                    pHttpServ->pThreadListTail->pNext = NULL;
                }
            }

            _ProtoHttpServThreadFree(pHttpServ, pHttpThread);
            *ppHttpThread = pHttpThreadNext;
        }
        else
        {
            pHttpThreadPrevious = pHttpThread;
            ppHttpThread = &(*ppHttpThread)->pNext;
        }
    }
}
