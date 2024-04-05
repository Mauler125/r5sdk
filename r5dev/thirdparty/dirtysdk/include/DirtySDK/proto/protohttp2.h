/*H********************************************************************************/
/*!
    \File    protohttp2.h

    \Description
        This module implements an HTTP/2 client that can perform basic transactions
        with an HTTP/2 server.  It conforms to but does not fully implement
        the 2.0 HTTP spec (https://tools.ietf.org/html/rfc7540), and
        allows for secure HTTP transactions as well as insecure transactions.

    \Copyright
        Copyright (c) Electronic Arts 2016-2018. ALL RIGHTS RESERVED.
*/
/********************************************************************************H*/

#ifndef _protohttp2_h
#define _protohttp2_h

/*!
\Moduledef ProtoHttp2 ProtoHttp2
\Modulemember Proto
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/proto/protohttputil.h"

/*** Defines **********************************************************************/

// define for ProtoHttp2Request iDataSize parameter
#define PROTOHTTP2_STREAM_BEGIN (-1) //!< start streaming upload (size unknown)

// define for ProtoHttp2Send's iDataSize parameter
#define PROTOHTTP2_STREAM_END   (0)  //!< streaming upload operation is complete

// define for ProtoHttp2Status/ProtoHttp2Control iStreamId parameter
#define PROTOHTTP2_INVALID_STREAMID (-1) //!< invalid stream for the functions that allow for invalid

// defines for ProtoHttpRecv's return result
#define PROTOHTTP2_RECVDONE  (-1)    //!< receive operation complete, and all data has been read
#define PROTOHTTP2_RECVFAIL  (-2)    //!< receive operation failed
#define PROTOHTTP2_RECVWAIT  (-3)    //!< waiting for body data
#define PROTOHTTP2_RECVHEAD  (-4)    //!< in headonly mode and header has been received
#define PROTOHTTP2_RECVBUFF  (-5)    //!< recvall did not have enough space in the provided buffer

// generic protohttp errors (do not overlap with RECV* codes)
#define PROTOHTTP2_MINBUFF   (-6)    //!< not enough input buffer space for operation
#define PROTOHTTP2_TIMEOUT   (-7)    //!< did not receive response from server in configured time

/*** Type Definitions *************************************************************/

//! opaque module ref
typedef struct ProtoHttp2RefT ProtoHttp2RefT;

//! write callback info
typedef struct ProtoHttp2WriteCbInfoT
{
    ProtoHttpRequestTypeE eRequestType;
    ProtoHttpResponseE eRequestResponse;
    int32_t iStreamId;
} ProtoHttp2WriteCbInfoT;

/*!
    \Callback ProtoHttp2WriteCbT

    \Description
        Write data callback

    \Input *pState      - module state
    \Input *pCbInfo     - information about about the request this data is for
    \Input *pData       - data we received from the server
    \Input iDataSize    - size of the data
    \Input *pUserData   - user data pointer passed along in the callback

    \Output
        int32_t         - unused
*/
typedef int32_t (ProtoHttp2WriteCbT)(ProtoHttp2RefT *pState, const ProtoHttp2WriteCbInfoT *pCbInfo, const uint8_t *pData, int32_t iDataSize, void *pUserData);

/*!
    \Callback ProtoHttp2CustomHeaderCbT

    \Description
        Callback that may be used to customize request header before sending

    \Input *pState      - module state
    \Input *pHeader     - [out] request headers that we are going to send
    \Input uHeaderSize  - size of the request headers
    \Input *pData       - pointer to data sent in the request. deprecated, will always be NULL
    \Input iDataLen     - size of the data sent in the request. deprecated, will always be 0
    \Input *pUserData   - user data pointer passed along in the callback

    \Output
        int32_t         - negative=error, size of the header=success

    \Deprecated
        pData and iDataLen are deprecated. We will pass NULL and 0 to them respectively.
*/
typedef int32_t (ProtoHttp2CustomHeaderCbT)(ProtoHttp2RefT *pState, char *pHeader, uint32_t uHeaderSize, const uint8_t *pData, int64_t iDataLen, void *pUserData);

/*!
    \Callback ProtoHttp2ReceiveHeaderCbT

    \Description
        Callback that may be used to implement custom header parsing on header receipt

    \Input *pState      - module state
    \Input iStreamId    - stream identifier for request
    \Input *pHeader     - header we received from the server
    \Input uHeaderSize  - size of the response header
    \Input *pUserData   - user data pointer passed along in the callback

    \Output
        int32_t         - unused
*/
typedef int32_t (ProtoHttp2ReceiveHeaderCbT)(ProtoHttp2RefT *pState, int32_t iStreamId, const char *pHeader, uint32_t uHeaderSize, void *pUserData);

//! state of the stream (see https://tools.ietf.org/html/rfc7540#section-5.1 for reference)
typedef enum ProtoHttp2StreamStateE
{
    STREAMSTATE_IDLE,
    STREAMSTATE_OPEN,
    STREAMSTATE_HALF_CLOSED_LOCAL,
    STREAMSTATE_HALF_CLOSED_REMOTE,
    STREAMSTATE_CLOSED
} ProtoHttp2StreamStateE;

/*** Functions ********************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

// allocate the module state and prepare for use
DIRTYCODE_API ProtoHttp2RefT *ProtoHttp2Create(int32_t iRecvBuf);

// give time to module to do its thing
DIRTYCODE_API void ProtoHttp2Update(ProtoHttp2RefT *pState);

// destroy the module and release its state
DIRTYCODE_API void ProtoHttp2Destroy(ProtoHttp2RefT *pState);

// set the custom header callbacks
DIRTYCODE_API void ProtoHttp2Callback(ProtoHttp2RefT *pState, ProtoHttp2CustomHeaderCbT *pCustomHeaderCb, ProtoHttp2ReceiveHeaderCbT *pReceiveHeaderCb, void *pUserData);

// initiate an HTTP request
DIRTYCODE_API int32_t ProtoHttp2Request(ProtoHttp2RefT *pState, const char *pUrl, const uint8_t *pData, int32_t iDataSize, ProtoHttpRequestTypeE eRequestType, int32_t *pStreamId);

// initiate an HTTP request with a write callback
DIRTYCODE_API int32_t ProtoHttp2RequestCb(ProtoHttp2RefT *pState, const char *pUrl, const uint8_t *pData, int32_t iDataSize, ProtoHttpRequestTypeE eRequestType, int32_t *pStreamId, ProtoHttp2WriteCbT *pWriteCb, void *pUserData);

// initiate an HTTP request with a write, custom header and receive header callback
DIRTYCODE_API int32_t ProtoHttp2RequestCb2(ProtoHttp2RefT *pState, const char *pUrl, const uint8_t *pData, int32_t iDataSize, ProtoHttpRequestTypeE eRequestType, int32_t *pStreamId, ProtoHttp2WriteCbT *pWriteCb, ProtoHttp2CustomHeaderCbT *pCustomHeaderCb, ProtoHttp2ReceiveHeaderCbT *pReceiveHeaderCb, void *pUserData);

// send data for a specific stream, NULL data ends the stream
DIRTYCODE_API int32_t ProtoHttp2Send(ProtoHttp2RefT *pState, int32_t iStreamId, const uint8_t *pData, int32_t iDataSize);

// receive the actual url data
DIRTYCODE_API int32_t ProtoHttp2Recv(ProtoHttp2RefT *pState, int32_t iStreamId, uint8_t *pBuffer, int32_t iBufMin, int32_t iBufMax);

// receive all of the response data for the given stream
DIRTYCODE_API int32_t ProtoHttp2RecvAll(ProtoHttp2RefT *pState, int32_t iStreamId, uint8_t *pBuffer, int32_t iBufSize);

// abort the passed in stream's transaction
DIRTYCODE_API void ProtoHttp2Abort(ProtoHttp2RefT *pState, int32_t iStreamId);

// close the connection to the server
DIRTYCODE_API void ProtoHttp2Close(ProtoHttp2RefT *pState);

// return module status based on input selector
DIRTYCODE_API int32_t ProtoHttp2Status(ProtoHttp2RefT *pState, int32_t iStreamId, int32_t iSelect, void *pBuffer, int32_t iBufSize);

// control function; functionality based on input selector
DIRTYCODE_API int32_t ProtoHttp2Control(ProtoHttp2RefT *pState, int32_t iStreamId, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// set base url (used in relative url support)
DIRTYCODE_API void ProtoHttp2SetBaseUrl(ProtoHttp2RefT *pState, const char *pUrl);

// get location header (requires state and special processing for relative urls)
DIRTYCODE_API int32_t ProtoHttp2GetLocationHeader(ProtoHttp2RefT *pState, const char *pInpBuf, char *pBuffer, int32_t iBufSize, const char **pHdrEnd);

// release resources for the stream identified by id
DIRTYCODE_API void ProtoHttp2StreamFree(ProtoHttp2RefT *pState, int32_t iStreamId);

#if defined(__cplusplus)
}
#endif

//@}

#endif // _protohttp2_h
