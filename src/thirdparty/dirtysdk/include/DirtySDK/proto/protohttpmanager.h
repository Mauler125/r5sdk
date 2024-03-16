/*H********************************************************************************/
/*!
    \File protohttpmanager.h

    \Description
        High-level module designed to create and manage a pool of ProtoHttp refs.  A
        client application can submit rapid-fire http requests and ProtoHttpManager
        will distribute them efficiently across the ref pool internally, queuing
        them for efficient use of keep-alive and pipelining requests where possible.

    \Notes
        None.

    \Todo
        Pipelining

    \Copyright
        Copyright (c) Electronic Arts 2009.

    \Version 1.0 05/20/2009 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _protohttpmanager_h
#define _protohttpmanager_h

/*!
\Moduledef ProtoHttpManager ProtoHttpManager
\Modulemember Proto
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/proto/protohttp.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! httpmanager stats
typedef struct HttpManagerStatT
{
    uint32_t uNumActiveTransactions;        //!< current number of active transactions
    uint32_t uMaxActiveTransactions;        //!< maximum (highwater) number of active transactions
    uint32_t uNumQueuedTransactions;        //!< current number of queued transactions
    uint32_t uMaxQueuedTransactions;        //!< maximum (highwater) number of queued transactions
    uint32_t uNumTransactions;              //!< total number of transactions issued
    uint32_t uNumKeepAliveTransactions;     //!< total number of keep-alive transactions issued
    uint32_t uNumPipelinedTransactions;     //!< total number of pipelined transactions issued
    uint32_t uSumQueueWaitLatency;          //!< total amount of time spent waiting in queue
    uint32_t uMaxQueueWaitLatency;          //!< max time any single request had to wait
    uint32_t uSumQueueFreeLatency;          //!< total amount of time spent waiting for transaction to freed
    uint32_t uMaxQueueFreeLatency;          //!< max time any single transaction waited to be freed
    uint64_t uTransactionBytes;             //!< total bytes xferred
    uint32_t uTransactionTime;              //!< total transaction time
} HttpManagerStatT;

//! opaque module ref
typedef struct HttpManagerRefT HttpManagerRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// allocate module state and prepare for use
DIRTYCODE_API HttpManagerRefT *HttpManagerCreate(int32_t iHttpBufSize, int32_t iHttpNumRefs);

// set custom header callback
DIRTYCODE_API void HttpManagerCallback(HttpManagerRefT *pHttpManager, ProtoHttpCustomHeaderCbT *pCustomHeaderCb, ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb);

// destroy the module and release its state
DIRTYCODE_API void HttpManagerDestroy(HttpManagerRefT *pHttpManager);

// allocate a new transaction handle
DIRTYCODE_API int32_t HttpManagerAlloc(HttpManagerRefT *pHttpManager);

// release a transaction handle
DIRTYCODE_API void HttpManagerFree(HttpManagerRefT *pHttpManager, int32_t iHandle);

// initiate an HTTP transfer
DIRTYCODE_API int32_t HttpManagerGet(HttpManagerRefT *pHttpManager, int32_t iHandle, const char *pUrl, uint32_t bHeadOnly);

// return the actual url data
DIRTYCODE_API int32_t HttpManagerRecv(HttpManagerRefT *pHttpManager, int32_t iHandle, char *pBuffer, int32_t iBufMin, int32_t iBufMax);

// receive all of the response data
DIRTYCODE_API int32_t HttpManagerRecvAll(HttpManagerRefT *pHttpManager, int32_t iHandle, char *pBuffer, int32_t iBufSize);

// initiate transfer of data to to the server via an HTTP POST command
DIRTYCODE_API int32_t HttpManagerPost(HttpManagerRefT *pHttpManager, int32_t iHandle, const char *pUrl, const char *pData, int64_t iDataSize, uint32_t bDoPut);

// send the actual url data
DIRTYCODE_API int32_t HttpManagerSend(HttpManagerRefT *pHttpManager, int32_t iHandle, const char *pData, int32_t iDataSize);

// make an http request
#define HttpManagerRequest(_pHttpManager, _iHandle, _pUrl, _pData, _iDataSize, _eRequestType) HttpManagerRequestCb2(_pHttpManager, _iHandle, _pUrl, _pData, _iDataSize, _eRequestType, NULL, NULL, NULL, NULL)

// make an http request with write callback
#define HttpManagerRequestCb(_pHttpManager, _iHandle, _pUrl, _pData, _iDataSize, _eRequestType, _pWriteCb, _pWriteCbUserData) HttpManagerRequestCb2(_pHttpManager, _iHandle, _pUrl, _pData, _iDataSize, _eRequestType, _pWriteCb, NULL, NULL, _pWriteCbUserData)

// make an http request with write, custom header and receive header callback
DIRTYCODE_API int32_t HttpManagerRequestCb2(HttpManagerRefT *pHttpManager, int32_t iHandle, const char *pUrl, const char *pData, int64_t iDataSize, ProtoHttpRequestTypeE eRequestType, ProtoHttpWriteCbT *pWriteCb, ProtoHttpCustomHeaderCbT *pCustomHeaderCb, ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb, void *pUserData);

// set base url
DIRTYCODE_API void HttpManagerSetBaseUrl(HttpManagerRefT *pHttpManager, int32_t iHandle, const char *pUrl);

// control function; functionality based on input selector (-1 to apply to all refs)
DIRTYCODE_API int32_t HttpManagerControl(HttpManagerRefT *pHttpManager, int32_t iHandle, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue);

// return module status based on input selector
DIRTYCODE_API int32_t HttpManagerStatus(HttpManagerRefT *pHttpManager, int32_t iHandle, int32_t iSelect, void *pBuffer, int32_t iBufSize);

// give time to module to do its thing (should be called periodically to allow module to perform work)
DIRTYCODE_API void HttpManagerUpdate(HttpManagerRefT *pHttpManager);

#ifdef __cplusplus
}
#endif

//@}

#endif // _protohttpmanager_h

