/*H********************************************************************************/
/*!
    \File protohttputil.h

    \Description
        This module implements HTTP support routines such as URL processing, header
        parsing, etc.

    \Copyright
        Copyright (c) Electronic Arts 2000-2012.

    \Version 1.0 12/18/2012 (jbrookes)  Refactored into separate module from protohttp.
*/
/********************************************************************************H*/

#ifndef _protohttputil_h
#define _protohttputil_h

/*!
\Moduledef ProtoHttpUtil ProtoHttpUtil
\Modulemember Proto
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

//! max supported protocol scheme length
#define PROTOHTTPUTIL_SCHEME_MAX    (32)

/*** Macros ***********************************************************************/

//! get class of response code
#define PROTOHTTP_GetResponseClass(_eError)                 (((_eError)/100) * 100)

/*** Type Definitions *************************************************************/

//! request types
typedef enum ProtoHttpRequestTypeE
{
    PROTOHTTP_REQUESTTYPE_HEAD = 0,     //!< HEAD request - does not return any data
    PROTOHTTP_REQUESTTYPE_GET,          //!< GET request  - get data from a server
    PROTOHTTP_REQUESTTYPE_POST,         //!< POST request - post data to a server
    PROTOHTTP_REQUESTTYPE_PUT,          //!< PUT request - put data on a server
    PROTOHTTP_REQUESTTYPE_DELETE,       //!< DELETE request - delete resource from a server
    PROTOHTTP_REQUESTTYPE_OPTIONS,      //!< OPTIONS request - get server options for specified resource
    PROTOHTTP_REQUESTTYPE_PATCH,        //!< PATCH request - like PUT but updates the ressource instead of overwriting 
    PROTOHTTP_REQUESTTYPE_CONNECT,      //!< CONNECT request - connect to a proxy that can tunnel (internal use only)

    PROTOHTTP_NUMREQUESTTYPES
} ProtoHttpRequestTypeE;

/*! HTTP response codes - see http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10
    for more detailed information. These response codes may be retrieved from ProtoHttp
    by calling ProtoHttpStatus('code').  Note that response codes are not available until
    a response has been received from the server and parsed by ProtoHttp.  If a response
    code is not yet available, -1 will be returned. */
typedef enum ProtoHttpResponseE
{
    PROTOHTTP_RESPONSE_PENDING              = -1,   //!< response has not yet been received

    // 1xx - informational reponse
    PROTOHTTP_RESPONSE_INFORMATIONAL        = 100,  //!< generic name for a 1xx class response
    PROTOHTTP_RESPONSE_CONTINUE             = 100,  //!< continue with request, generally ignored
    PROTOHTTP_RESPONSE_SWITCHPROTO,                 //!< 101 - OK response to client switch protocol request

    // 2xx - success response
    PROTOHTTP_RESPONSE_SUCCESSFUL           = 200,  //!< generic name for a 2xx class response
    PROTOHTTP_RESPONSE_OK                   = 200,  //!< client's request was successfully received, understood, and accepted
    PROTOHTTP_RESPONSE_CREATED,                     //!< new resource has been created
    PROTOHTTP_RESPONSE_ACCEPTED,                    //!< request accepted but not complete
    PROTOHTTP_RESPONSE_NONAUTH,                     //!< non-authoritative info (ok)
    PROTOHTTP_RESPONSE_NOCONTENT,                   //!< request fulfilled, no message body
    PROTOHTTP_RESPONSE_RESETCONTENT,                //!< request success, reset document view
    PROTOHTTP_RESPONSE_PARTIALCONTENT,              //!< server has fulfilled partial GET request

    // 3xx - redirection response
    PROTOHTTP_RESPONSE_REDIRECTION          = 300,  //!< generic name for a 3xx class response
    PROTOHTTP_RESPONSE_MULTIPLECHOICES      = 300,  //!< requested resource corresponds to a set of representations
    PROTOHTTP_RESPONSE_MOVEDPERMANENTLY,            //!< requested resource has been moved permanently to new URI
    PROTOHTTP_RESPONSE_FOUND,                       //!< requested resources has been moved temporarily to a new URI
    PROTOHTTP_RESPONSE_SEEOTHER,                    //!< response can be found under a different URI
    PROTOHTTP_RESPONSE_NOTMODIFIED,                 //!< response to conditional get when document has not changed
    PROTOHTTP_RESPONSE_USEPROXY,                    //!< requested resource must be accessed through proxy
    PROTOHTTP_RESPONSE_RESERVED,                    //!< old response code - reserved
    PROTOHTTP_RESPONSE_TEMPREDIRECT,                //!< requested resource resides temporarily under a different URI

    // 4xx - client error response
    PROTOHTTP_RESPONSE_CLIENTERROR          = 400,  //!< generic name for a 4xx class response
    PROTOHTTP_RESPONSE_BADREQUEST           = 400,  //!< request could not be understood by server due to malformed syntax
    PROTOHTTP_RESPONSE_UNAUTHORIZED,                //!< request requires user authorization
    PROTOHTTP_RESPONSE_PAYMENTREQUIRED,             //!< reserved for future user
    PROTOHTTP_RESPONSE_FORBIDDEN,                   //!< request understood, but server will not fulfill it
    PROTOHTTP_RESPONSE_NOTFOUND,                    //!< Request-URI not found
    PROTOHTTP_RESPONSE_METHODNOTALLOWED,            //!< method specified in the Request-Line is not allowed
    PROTOHTTP_RESPONSE_NOTACCEPTABLE,               //!< resource incapable of generating content acceptable according to accept headers in request
    PROTOHTTP_RESPONSE_PROXYAUTHREQ,                //!< client must first authenticate with proxy
    PROTOHTTP_RESPONSE_REQUESTTIMEOUT,              //!< client did not produce response within server timeout
    PROTOHTTP_RESPONSE_CONFLICT,                    //!< request could not be completed due to a conflict with current state of the resource
    PROTOHTTP_RESPONSE_GONE,                        //!< requested resource is no longer available and no forwarding address is known
    PROTOHTTP_RESPONSE_LENGTHREQUIRED,              //!< a Content-Length header was not specified and is required
    PROTOHTTP_RESPONSE_PRECONFAILED,                //!< precondition given in request-header field(s) failed
    PROTOHTTP_RESPONSE_REQENTITYTOOLARGE,           //!< request entity is larger than the server is able or willing to process
    PROTOHTTP_RESPONSE_REQURITOOLONG,               //!< Request-URI is longer than the server is willing to interpret
    PROTOHTTP_RESPONSE_UNSUPPORTEDMEDIA,            //!< request entity is in unsupported format
    PROTOHTTP_RESPONSE_REQUESTRANGE,                //!< invalid range in Range request header
    PROTOHTTP_RESPONSE_EXPECTATIONFAILED,           //!< expectation in Expect request-header field could not be met by server

    // 5xx - server error response
    PROTOHTTP_RESPONSE_SERVERERROR          = 500,  //!< generic name for a 5xx class response
    PROTOHTTP_RESPONSE_INTERNALSERVERERROR  = 500,  //!< an unexpected condition prevented the server from fulfilling the request
    PROTOHTTP_RESPONSE_NOTIMPLEMENTED,              //!< the server does not support the functionality required to fulfill the request
    PROTOHTTP_RESPONSE_BADGATEWAY,                  //!< invalid response from gateway server
    PROTOHTTP_RESPONSE_SERVICEUNAVAILABLE,          //!< unable to handle request due to a temporary overloading or maintainance
    PROTOHTTP_RESPONSE_GATEWAYTIMEOUT,              //!< gateway or DNS server timeout
    PROTOHTTP_RESPONSE_HTTPVERSUNSUPPORTED          //!< the server does not support the HTTP protocol version that was used in the request
} ProtoHttpResponseE;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// parse HTTP request line from the format as sent on the wire
DIRTYCODE_API const char *ProtoHttpUtilParseRequest(const char *pStrHeader, char *pStrMethod, int32_t iMethodLen, char *pStrUri, int32_t iUriLen, char *pStrQuery, int32_t iQueryLen, ProtoHttpRequestTypeE *pRequestType, uint8_t *pHttp1_0);

// finds the header field value for the specified header field
DIRTYCODE_API const char *ProtoHttpFindHeaderValue(const char *pHdrBuf, const char *pHeaderText);

// extracts the header field value for the specified header field entry
DIRTYCODE_API int32_t ProtoHttpExtractHeaderValue(const char *pHdrField, char *pOutBuf, int32_t iOutLen, const char **ppHdrEnd);

// parse header response code from HTTP header
DIRTYCODE_API int32_t ProtoHttpParseHeaderCode(const char *pHdrBuf);

// extract a header value from the specified header text
DIRTYCODE_API int32_t ProtoHttpGetHeaderValue(void *pState, const char *pHdrBuf, const char *pName, char *pBuffer, int32_t iBufSize, const char **pHdrEnd);

// get next header name and value from header buffer
DIRTYCODE_API int32_t ProtoHttpGetNextHeader(void *pState, const char *pHdrBuf, char *pName, int32_t iNameSize, char *pValue, int32_t iValSize, const char **pHdrEnd);

// url-encode a integer parameter
DIRTYCODE_API int32_t ProtoHttpUrlEncodeIntParm(char *pBuffer, int32_t iLength, const char *pParm, int32_t iValue);

// url-encode a string parameter
DIRTYCODE_API int32_t ProtoHttpUrlEncodeStrParm(char *pBuffer, int32_t iLength, const char *pParm, const char *pData);

// url-encode a string parameter with custom safe table
DIRTYCODE_API int32_t ProtoHttpUrlEncodeStrParm2(char *pBuffer, int32_t iLength, const char *pParm, const char *pData, const char *pStrSafe);

// url-decode a string parameter
DIRTYCODE_API int32_t ProtoHttpUrlDecodeStrParm(const char *pBuffer, char *pData, int32_t iDataSize);

// parse a url into component parts
DIRTYCODE_API const char *ProtoHttpUrlParse(const char *pUrl, char *pKind, int32_t iKindSize, char *pHost, int32_t iHostSize, int32_t *pPort, int32_t *pSecure);

// parse a url into component parts
DIRTYCODE_API const char *ProtoHttpUrlParse2(const char *pUrl, char *pKind, int32_t iKindSize, char *pHost, int32_t iHostSize, int32_t *pPort, int32_t *pSecure, uint8_t *bPortSpecified);

// get next param name and value from URI buffer
DIRTYCODE_API int32_t ProtoHttpGetNextParam(void *pState, const char *pUriBuf, char *pName, int32_t iNameSize, char *pValue, int32_t iValSize, const char **pUriEnd);

#ifdef __cplusplus
}
#endif

//@}

#endif // _protohttp_h

