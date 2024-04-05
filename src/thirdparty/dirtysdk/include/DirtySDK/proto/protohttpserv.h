/*H********************************************************************************/
/*!
    \File protohttpserv.h

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

#ifndef _protohttpserv_h
#define _protohttpserv_h

/*!
\Moduledef ProtoHttpServ ProtoHttpServ
\Modulemember Proto
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/proto/protohttputil.h"

/*** Defines **********************************************************************/

#define PROTOHTTPSERV_FLAG_LOOPBACK (0x01) //!< when set, only traffic from the same host will be accepted on the listen port

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct ProtoHttpServRequestT
{
    ProtoHttpRequestTypeE eRequestType; //!< The HTTP method
    uint32_t uAddr;                     //!< Address of where the request originated from
    int64_t iContentLength;             //!< `Content-Length` header value, how big is the request
    int64_t iContentRecv;               //!< Amount of data received when processing request
    char strContentType[64];            //!< `Content-Type` header value, what type of data is being received
    char strUrl[128];                   //!< The path / resource (when a RESTful API) that is being accessed. Example: /person/{id}
    char strQuery[128];                 //!< Query strings passed into the request. Example: ?name="foobar"
    char strHeader[16*1024];            //!< HTTP headers sent with the request. Use ProtoHttpUtil functions for parsing
    void *pData;                        //!< User Specific data (store any data to help handle the request)
} ProtoHttpServRequestT;

typedef struct ProtoHttpServResponseT
{
    ProtoHttpResponseE eResponseCode;   //!< The HTTP code (200, 404, etc)
    char strContentType[64];            //!< `Content-Type` header value, what type of data is being sent
    int64_t iContentLength;             //!< `Content-Length` header value, how much data is being sent.
    int64_t iContentRead;               //!< Amount of data read that is being sent
    char strHeader[1*1024];             //!< Custom headers being sent
    int32_t iHeaderLen;                 //!< Size of the custom headers
    int32_t iChunkLength;               //!< Use `Transfer-Encoding: chunked` if greater than zero
    char strLocation[1 * 1024];         //!< Redirect Location 
    void *pData;                        //!< User Specific data (store any data to help respond)
} ProtoHttpServResponseT;

/*! 
    \Callback ProtoHttpServRequestCbT
 
    \Description
        Invoked when a request is ready for processing
 
    \Input *pRequest    - Information that allows for processing of request
    \Input *pResponse   - [out] Information that ProtoHttpServ used in responding to the request
    \Input *pUserData   - User Specific Data that was passed into ProtoHttpServCallback
   
    \Output
        int32_t         - zero=complete, positive=in-progress, negative=error
*/
typedef int32_t (ProtoHttpServRequestCbT)(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData);

/*!
    \Callback ProtoHttpServReceiveCbT

    \Description
        Invoked when new incoming data is received for a request

    \Input *pRequest    - [in/out] Information about the request
    \Input *pBuffer     - The data that was received (NULL when complete)
    \Input iBufSize     - The size of the data received (zero when complete)
    \Input *pUserData   - User Specific Data that was passed into ProtoHttpServCallback

    \Output
        int32_t     - zero=no data processed, positive=amount of data processed, negative=error
*/
typedef int32_t (ProtoHttpServReceiveCbT)(ProtoHttpServRequestT *pRequest, const char *pBuffer, int32_t iBufSize, void *pUserData);

/*!
    \Callback ProtoHttpServSendCbT

    \Description
        Invoked when outgoing data needs to be sent for the response

    \Input *pResponse   - Information about the response we are sending
    \Input *pBuffer     - The data we are writting to for the response (NULL when complete)
    \Input iBufSize     - The size of the outgoing buffer (zero when complete)
    \Input *pUserData   - User Specific Data that was passed into ProtoHttpServCallback

    \Output
        int32_t - positive=amount of data written, other=error
*/
typedef int32_t (ProtoHttpServSendCbT)(ProtoHttpServResponseT *pResponse, char *pBuffer, int32_t iBufSize, void *pUserData);

/*!
    \Callback ProtoHttpServHeaderCbT

    \Description
        Invoked when a new request header is received

    \Input *pRequest    - [in/out] Information about the request we are going to process
    \Input *pResponse   - [out] Used for handling the response, this allows us to error if header is malformed
    \Input *pUserData   - User Specific Data that was passed into ProtoHttpServCallback

    \Output
        int32_t - positive=success, negative=error
*/
typedef int32_t (ProtoHttpServHeaderCbT)(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData);

//! logging function type
typedef void (ProtoHttpServLogCbT)(const char *pText, void *pUserData);

//! opaque module ref
typedef struct ProtoHttpServRefT ProtoHttpServRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create an httpserv module
DIRTYCODE_API ProtoHttpServRefT *ProtoHttpServCreate(int32_t iPort, int32_t iSecure, const char *pName);

// create an httpserv module
DIRTYCODE_API ProtoHttpServRefT *ProtoHttpServCreate2(int32_t iPort, int32_t iSecure, const char *pName, uint32_t uFlags);

// destroy an httpserv module
DIRTYCODE_API void ProtoHttpServDestroy(ProtoHttpServRefT *pHttpServ);

// set httpserv callback function handlers (required)
DIRTYCODE_API void ProtoHttpServCallback(ProtoHttpServRefT *pHttpServ, ProtoHttpServRequestCbT *pRequestCb, ProtoHttpServReceiveCbT *pReceiveCb, ProtoHttpServSendCbT *pSendCb, ProtoHttpServHeaderCbT *pHeaderCb, ProtoHttpServLogCbT *pLogCb, void *pUserData);

// control function; functionality based on input selector
DIRTYCODE_API int32_t ProtoHttpServControl(ProtoHttpServRefT *pHttpServ, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue);

// control function allowing thread-specific control; functionality based on input selector
DIRTYCODE_API int32_t ProtoHttpServControl2(ProtoHttpServRefT *pHttpServ, int32_t iThread, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue);

// return module status based on input selector
DIRTYCODE_API int32_t ProtoHttpServStatus(ProtoHttpServRefT *pHttpServ, int32_t iSelect, void *pBuffer, int32_t iBufSize);

// update the module
DIRTYCODE_API void ProtoHttpServUpdate(ProtoHttpServRefT *pHttpServ);

// updates the backlog setting on the listen socket, not required to be called unless you need to update the backlog
DIRTYCODE_API int32_t ProtoHttpServListen(ProtoHttpServRefT *pHttpServ, int32_t iBacklog);

#ifdef __cplusplus
}
#endif

//@}

#endif // _protohttpserv_h


