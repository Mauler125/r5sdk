/*H********************************************************************************/
/*!
    \File qosclient.c

    \Description
        This module implements the client API for the quality of service.

    \Copyright
        Copyright (c) 2017 Electronic Arts Inc.

    \Version 2.0 06/05/2017 (cvienneau) Re-write of qosapi
*/
/********************************************************************************H*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/misc/qoscommon.h"
#include "DirtySDK/misc/qosclient.h"
#include "DirtySDK/proto/protoname.h"
#include "DirtySDK/proto/protohttp2.h"

/*** Defines **********************************************************************/
//!< qosclient default listen port
#if !DIRTYCODE_XBOXONE
#define QOS_CLIENT_DEFAULT_LISTENPORT   (7673)
#else
#define QOS_CLIENT_DEFAULT_LISTENPORT   (0)
#endif

#define QOS_CLIENT_SIMULATE_PACKET_LOSS (0)         //!< for testing it can often be handy to lose some probes

//!< passed to SocketControl 'rbuf'
#define QOS_CLIENT_DEFAULT_RECEIVE_BUFF_SIZE        (16*1024)

//!< passed to SocketControl 'sbuf'
#define QOS_CLIENT_DEFAULT_SEND_BUFF_SIZE           (16*1024)

/*! passed to SocketControl 'pque'
    By default non-virtual sockets have a 1-deep packet queue. When _QosClientRecvCB() fails
    to enter pQosClient->ThreadCrit we need space to buffer the packet. If extracting packets 
    from the socket receive buffer is delayed, then rtt time calculated from those probe replies 
    ends up being incorrectly higher than it should be. 
       
    Testing has shown that with 6 ping sites providing ~equal ping times a packet queue of 2 is sometimes required.
    If pQosClient->ThreadCrit fails to enter 50% of the time, approximately a 'pque' of 6 is required.
    With pQosClient->ThreadCrit failing 100% of the time, an 11 deep 'pque' successfully allows the updating via NetConnIdle to handle it.*/
#define QOS_CLIENT_DEFAULT_PACKET_QUEUE_SIZE        (12)

//!< qosclient timeout for socket idle callback (0 to disable the idle callback)
#define QOS_CLIENT_SOCKET_IDLE_RATE                  (0)

//!< the minimum amount of time to have passed since the last update for us to consider our updates to have been stalled
#define QOS_CLIENT_DEFAULT_STALL_WINDOW             (500)

//!< the maximum amount of time to have passed since we started the qos process before we just bail out
#define QOS_CLIENT_DEFAULT_MAX_QOS_PROCESS_TIME     (60000)

//!< easily switch from https to http for testing, prod will always be https
#define QOS_CLIENT_DEFAULT_USE_HTTPS    (TRUE)

//!< every module state transition in DIRTYAPI_QOS should have a unique entry here and later can be found in the hResult
enum
{
    QOS_CLIENT_MODULE_STATUS_INIT = 2000,
    QOS_CLIENT_MODULE_STATUS_PROCESS_STARTED,
    QOS_CLIENT_MODULE_STATUS_TEST_CONFIG_RECIEVED,
    QOS_CLIENT_MODULE_STATUS_RAW_RESULTS_READY,
    QOS_CLIENT_MODULE_STATUS_RESULTS_RECEIVED,
    QOS_CLIENT_MODULE_STATUS_REPORT_COMPLETE,

    QOS_CLIENT_MODULE_STATUS_UNSET = 0,

    QOS_CLIENT_MODULE_STATUS_ERROR_TEST_CONFIG_UNUSABLE = -2000,
    QOS_CLIENT_MODULE_STATUS_ERROR_RPC_DECODE,
    QOS_CLIENT_MODULE_STATUS_ERROR_RPC_ENCODE,
    QOS_CLIENT_MODULE_STATUS_ERROR_UNKOWN_STATE,
    QOS_CLIENT_MODULE_STATUS_ERROR_TEST_CONFIG_PRODUCED_NO_REQUESTS,
    QOS_CLIENT_MODULE_STATUS_ERROR_FAILED_ALLOC_RECV_BUFFER,
    QOS_CLIENT_MODULE_STATUS_ERROR_RECV_BUFFER_NEEDED_TOO_LARGE,
    QOS_CLIENT_MODULE_STATUS_ERROR_FAILED_ALLOC_SEND_BUFFER,
    QOS_CLIENT_MODULE_STATUS_ERROR_FAILED_ALLOC_SEND_BUFFER_FATAL,
    QOS_CLIENT_MODULE_STATUS_ERROR_SEND_BUFFER_NEEDED_TOO_LARGE,
    QOS_CLIENT_MODULE_STATUS_ERROR_SERIALIZED_PACKET_UNEXPECTED_SIZE,
    QOS_CLIENT_MODULE_STATUS_ERROR_TIMEOUT,
    QOS_CLIENT_MODULE_STATUS_ERROR_UNRECOVERED_ERROR

} eQosClientModuleStatus;

//!< every request state transition in DIRTYAPI_QOS should have a unique entry here and later can be found in the hResult
enum
{
    QOS_CLIENT_REQUEST_STATUS_INIT = 1000,
    QOS_CLIENT_REQUEST_STATUS_SOCKET_LOOKUP_SUCCESS,
    QOS_CLIENT_REQUEST_STATUS_INIT_SYNC_SUCCESS,
    QOS_CLIENT_REQUEST_STATUS_SEND_NEXT_PROBE,
    QOS_CLIENT_REQUEST_STATUS_SEND_LOST_PROBES,
    QOS_CLIENT_REQUEST_STATUS_SEND_SUCCESS,
    QOS_CLIENT_REQUEST_STATUS_SEND_TRY_AGAIN,
    QOS_CLIENT_REQUEST_STATUS_COMPLETE_ACCEPTABLE,
    QOS_CLIENT_REQUEST_STATUS_COMPLETE,
    //add new positive states here
    
    QOS_CLIENT_REQUEST_STATUS_UNSET = 0,    //unused valued
    
    QOS_CLIENT_REQUEST_STATUS_ERROR_SOCKET_LOOKUP_ALLOC = -1000,
    QOS_CLIENT_REQUEST_STATUS_ERROR_SOCKET_LOOKUP_UPDATE,
    QOS_CLIENT_REQUEST_STATUS_ERROR_TIMEOUT,
    QOS_CLIENT_REQUEST_STATUS_ERROR_SENT_MAX_PROBES,
    QOS_CLIENT_REQUEST_STATUS_ERROR_SITE_INVALID,
    QOS_CLIENT_REQUEST_STATUS_ERROR_TEST_INVALID,
    QOS_CLIENT_REQUEST_STATUS_ERROR_EXTERNAL_ADDRESS_MISMATCH,
    QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_VERSION,
    QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_UP_TOO_SMALL,
    QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_DOWN_TOO_SMALL,
    QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_UP_TOO_LARGE,
    QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_DOWN_TOO_LARGE,
    QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_MY_ADDRESS,
    QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_SEVER_TIME,
    QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_DOWN_TOO_MANY,
    QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_UP_TOO_MANY,
    QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_SERVICE_REQUEST_ID,
    QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_ALREADY_COMPLETE,
    QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_ALREADY_RECEIVED,
    QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_HMAC,
    QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_PROTOCOL,
    QOS_CLIENT_REQUEST_STATUS_ERROR_TIMEOUT_PARTIAL
    //add new negative states here
    
} eQosClientRequestStatus;

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef enum QosClientRequestStateE
{
    REQUEST_STATE_INIT = 0,                         //!< prepare configuration, usually DNS resolve the server address
    REQUEST_STATE_INIT_SYNC,                        //!< attempt to start all tests at the same time, wait for other tests to finish their init before proceeding to the send state
    REQUEST_STATE_SEND,                             //!< send a series of probes to the target
    REQUEST_STATE_RECEIVE,                          //!< wait for probes/response from target
    REQUEST_STATE_COMPLETE,                         //!< succeeded or failed, callback the user and clean up
    REQUEST_STATE_ERROR                             //!< equivalent to REQUEST_STATE_COMPLETE, but sets failure hResult error code bit
} QosClientRequestStateE;                                        

typedef enum QosClientModuleStateE
{
    MODULE_STATE_IDLE = 0,                          //!< do nothing
    MODULE_STATE_INIT_COORDINATOR_COMM,             //!< send a message to the coordinator announcing who we are, and any results we have so far
    MODULE_STATE_UPDATE_COORDINATOR_COMM,           //!< wait for response from the coordinator
    MODULE_STATE_PROBE,                             //!< perform the tests that the coordinator has requested, once tests are completed typically communicate with the coordinator again to provide results and get next steps
    MODULE_STATE_REPORT,                            //!< the coordinator is done with us, report results to the client
    MODULE_STATE_ERROR,                             //!< an error has happened preventing us from doing our tests, report to the coordinator immediately
    MODULE_STATE_FATAL                              //!< an unrecoverable error has happened, such as coordinator communication failure, halt all further actions
} QosClientModuleStateE;

//! A QOS test is used to gather various metrics for a given ping site
typedef struct QosClientRequestT
{
    struct QosClientRequestT *pNext;                //!< link to the next record

    //info about the target the test is being performed against
    QosCommonSiteT site;                            //!< site we want to send the QOS probes to
    HostentT *pQosServerHost;                       //!< host name lookup for the QOS server
    struct sockaddr serverAddr;                     //!< address we want to send the QOS probes to
    
    // info about how the test is to be performed
    QosCommonTestT test;                            //!< parameters describing the test
    uint32_t uServiceRequestID;                     //!< id from the server to distinguish different clients
    uint16_t uClientRequestID;                      //!< id generated by this client to distinguish different requests

    //storage for the results
    QosCommonRawResultsT rawResults;                //!< we'll store all the info we are going to send back to the coordinator here
    QosCommonAddrT probeExternalAddr;               //!< clients external address, and the location the server will be sending probes to. Start with to clientAddressFromCoordinator, but update to ClientAddressFromServer if available, they will likely be the same.

    //request status
    QosClientRequestStateE eState;                  //!< the current state the request is in
    uint32_t uProbesToSend;                         //!< the number of probes we will send total, including retries
    uint32_t uWhenStarted;                          //!< time we started working on this request
    uint32_t uWhenLastSent;                         //!< the last time we sent a probe for this request
    uint32_t uWhenCompleted;                        //!< time we finished working on this request
    uint32_t uCreationCount;                        //!< typically 0, but if a request generates a new request we increment
    int16_t iLastProbeValidationError;              //!< what was the last thing to go wrong when receiving probes for this request
} QosClientRequestT;

//! current module state
struct QosClientRefT
{
    // module memory group
    int32_t iMemGroup;                                      //!< module memory group id
    void *pMemGroupUserData;                                //!< user data associated with memory group
    
    // return results to the user of QosClient
    QosClientCallbackT *pCallback;                          //!< callback function to call to when results are available
    void *pUserData;                                        //!< user specified data to pass to the callback

    // QosCoordinator communication
    char strCoordinatorAddr[QOS_COMMON_MAX_URL_LENGTH];     //!< the url for the coordinator
    QosCommonClientToCoordinatorRequestT rawResultStore;    //!< contains the data that needs to be provided to the coordinator
    QosCommonProcessedResultsT finalResults;                //!< contains the result data which is ready for consumption by the client
    QosCommonAddrT clientAddressFromCoordinator;            //!< external address from the coordinators prospective
    ProtoHttp2RefT *pProtoHttp;                             //!< http module, used for service requests to the QOS server
    int32_t iHttpStreamId;                                  //!< stream identifier generated on post
    uint8_t *pHttpRecvBuff;                                 //!< buffer we will receive http responses into, this need to be a member since sometimes the whole message can't come in one call (PROTOHTTP_RECVWAIT)
    uint8_t *pHttpSendBuff;                                 //!< buffer we will send http messages from, typically it will be in one shot, but if the message is particularly large multiple ProtoHttpSend calls may be needed
    uint8_t *pHttpSendProgress;                             //!< pointer into the send buffer where rpc data is currently being sent from
    uint32_t uHttpRecvBuffSize;                             //!< the current allocated size of pHttpRecvBuff
    uint32_t uCurrentHttpRecvSize;                          //!< the amount of data stored in pHttpRecvBuff
    uint32_t uHttpSendBuffSize;                             //!< the current allocated size of pHttpSendBuff
    uint32_t uHttpBytesToSend;                              //!< total rpc body size to send to the coordinator
    int32_t iHttpSoLinger;                                  //!< passed down to the socket as a SO_LINGER option if its >= 0, a 0 value will abort the socket on disconnection with no TIME_WAIT state
    uint16_t uCoordinatorPort;                              //!< port of the coordinator
    uint8_t bUseHttps;                                      //!< true if communication is done via https otherwise http
    uint8_t bIgnoreSSLErrors;                               //!< if true ssl cert errors will be ignored

    // udp probe socket management
    SocketT *pUdpSocket;                                    //!< socket pointer
    QosCommonAddrT localUdpAddrInfo;                        //!< information about the local address we bound to
    uint32_t uUdpReceiveBuffSize;                           //!< passed to SocketControl 'rbuf'
    uint32_t uUdpSendBuffSize;                              //!< passed to SocketControl 'sbuf'
    uint32_t uUdpPacketQueueSize;                           //!< passed to SocketControl 'pque' defaults QOS_CLIENT_DEFAULT_PACKET_QUEUE_SIZE (12)
    uint16_t uUdpRequestedListenPort;                       //!< port we want to listen on, we receive probes from the server here
    uint16_t uUdpCurrentListenPort;                         //!< port we are currently listening on (we may not get the port we wanted to listen on)
    int8_t bDeferredRecv;                                   //!< data available for reading, _QosClientUpdate should take care of it

    // request management
    QosClientRequestT *pRequestQueue;                       //!< linked list of requests pending
    NetCritT ThreadCrit;                                    //!< critical section, we lock receive thread, update, and public APIs to ensure safety with one accessors to the module at a time, since everything access the pRequestQueue
    QosClientModuleStateE eModuleState;                     //!< what stage of the QOS procedure are we currently in
    uint32_t uInitSyncTime;                                 //!< the time when a series of tests was initialized, used a base time to see if initialization is taking a long time
    uint32_t uFinalResultsRecvTime;                         //!< the time when the coordinator processed results arrived on this console, used to time if we want to retry based off uTimeTillRetry
    uint32_t uErrorMessageCount;                            //!< the number of consecutive messages we received from the coordinator that resulted in an error state.
    uint16_t uNextClientRequestID;                          //!< current request id, incremented every time a new request is made

    // update management
    uint32_t uStallWindow;                                  //!< the amount of time to pass between updates before we consider it stalled
    uint32_t uQosProcessStartTime;                          //!< the amount of time to have passed since calling QosClientStart
    uint32_t uMaxQosProcessTime;                            //!< the maximum amount of time to let qos run before bailing out completely
    uint32_t uLastUpdateTime;                               //!< NetTick of the last time update was called for this module

    int16_t iSpamValue;                                     //!< the current logging level
};

/*** Function Prototypes **********************************************************/

static int32_t _QosClientRecvCB(SocketT *pSocket, int32_t iFlags, void *pData);
static uint8_t* _QosClientAllocHttpRecvBuff(QosClientRefT *pQosClient, uint32_t uSize);
static uint8_t* _QosClientAllocHttpSendBuff(QosClientRefT *pQosClient, uint32_t uSize);
static int32_t _QosClientDestroyRequest(QosClientRefT *pQosClient, uint32_t uRequestId);

/*** Variables ********************************************************************/
#if DIRTYCODE_LOGGING
const char *_strQosClientRequestState[] =              //!< keep in sync with QosClientRequestStateE
{
    "REQUEST_STATE_INIT     ",                         
    "REQUEST_STATE_INIT_SYNC",                         
    "REQUEST_STATE_SEND     ",                         
    "REQUEST_STATE_RECEIVE  ",                       
    "REQUEST_STATE_COMPLETE ",                    
    "REQUEST_STATE_ERROR    "                    
};

const char *_strQosClientModuleState[] =              //!< keep in sync with QosClientModuleStateE
{
    "MODULE_STATE_IDLE  ",
    "MODULE_STATE_INIT_COORDINATOR_COMM ",
    "MODULE_STATE_UPDATE_COORDINATOR_COMM ",
    "MODULE_STATE_PROBE ",
    "MODULE_STATE_REPORT",
    "MODULE_STATE_ERROR ",
    "MODULE_STATE_FATAL "
};

const char *_strQosCommonFirewallType[] =              //!< keep in sync with QosCommonFirewallTypeE
{
    "QOS_COMMON_FIREWALL_UNKNOWN  ",
    "QOS_COMMON_FIREWALL_OPEN ",
    "QOS_COMMON_FIREWALL_MODERATE ",
    "QOS_COMMON_FIREWALL_STRICT ",
    "QOS_COMMON_FIREWALL_NUMNATTYPES"
};
#endif

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _QosClientRequestStateTransition

    \Description
        Update the requests state, status and current error code.
    
    \Input *pQosClient  - module state
    \Input *pRequest    - pointer to the request
    \Input eState       - new state to transition to, REQUEST_STATE_*
    \Input uModule      - module producing the hResult code, DIRTYAPI_*
    \Input iCode        - status/error code to use in hResult
    \Input iLogLevel    - the log level this state transition should be logged at, equivalent to NetPrintfVerbose((pQosClient->iSpamValue, iLogLevel, ...

    \Version 09/19/2014 (cvienneau)
*/
/********************************************************************************F*/
static void _QosClientRequestStateTransition(QosClientRefT *pQosClient, QosClientRequestT *pRequest, QosClientRequestStateE eState, uint32_t uModule, int16_t iCode, int32_t iLogLevel)
{
    pRequest->rawResults.hResult = DirtyErrGetHResult(uModule, iCode, eState == REQUEST_STATE_ERROR);
    NetPrintfVerbose((pQosClient->iSpamValue, iLogLevel, "qosclient: request[%02d] transitioning from %s-> %s (0x%08x, %s, %s)\n", pRequest->uClientRequestID, _strQosClientRequestState[pRequest->eState], _strQosClientRequestState[eState], pRequest->rawResults.hResult, pRequest->site.strSiteName, pRequest->test.strTestName));
    
    if (eState == REQUEST_STATE_ERROR)
    {
        pRequest->eState = REQUEST_STATE_COMPLETE;
    }
    else
    {
        pRequest->eState = eState;
    }
}

/*F********************************************************************************/
/*!
    \Function _QosClientModuleStateTransition

    \Description
        Update the modules hResult, and transition to another state.
    
    \Input *pQosClient  - module state
    \Input eState       - new state to transition to, MODULE_STATE_*
    \Input uModule      - module producing the hResult code, DIRTYAPI_*
    \Input iCode        - status/error code to use in hResult
    \Input iLogLevel    - the log level this state transition should be logged at, equivalent to NetPrintfVerbose((pQosClient->iSpamValue, iLogLevel, ...

    \Version 09/19/2014 (cvienneau)
*/
/********************************************************************************F*/
static void _QosClientModuleStateTransition(QosClientRefT *pQosClient, QosClientModuleStateE eState, uint32_t uModule, int16_t iCode, int32_t iLogLevel)
{
    pQosClient->rawResultStore.clientModifiers.hResult = DirtyErrGetHResult(uModule, iCode, ((eState == MODULE_STATE_ERROR) || (eState == MODULE_STATE_FATAL)));
    NetPrintfVerbose((pQosClient->iSpamValue, iLogLevel, "qosclient: process transitioning from %s-> %s (0x%08x)\n", _strQosClientModuleState[pQosClient->eModuleState], _strQosClientModuleState[eState], pQosClient->rawResultStore.clientModifiers.hResult, pQosClient->rawResultStore.clientModifiers.hResult));
    
    // if we were sending to the coordinator, and we aren't anymore, we no longer need the send buffer
    if ((pQosClient->eModuleState == MODULE_STATE_INIT_COORDINATOR_COMM) && (eState != MODULE_STATE_INIT_COORDINATOR_COMM))
    {
        _QosClientAllocHttpSendBuff(pQosClient, 0);
    }    
    
    // if we were receiving/reporting results from the coordinator, and we aren't anymore, we no longer need the receive buffer
    if ((pQosClient->eModuleState == MODULE_STATE_REPORT) && (eState != MODULE_STATE_REPORT))
    {
        _QosClientAllocHttpRecvBuff(pQosClient, 0);  
    }

    // if we are no longer active, give the port back to the system, if we do qos again we'll attempt to obtain it again
    if ((pQosClient->eModuleState != MODULE_STATE_IDLE) && (eState == MODULE_STATE_IDLE))
    {
        if (pQosClient->pUdpSocket != NULL)
        {
            SocketClose(pQosClient->pUdpSocket);
            pQosClient->pUdpSocket = NULL;
        }
    }

    // fatal errors clean up and call the callback, we can't talk to the coordinator anymore
    if (eState == MODULE_STATE_FATAL)
    {
        //free any of our buffers, they would likely need to be re-created anyways
        _QosClientAllocHttpSendBuff(pQosClient, 0);
        _QosClientAllocHttpRecvBuff(pQosClient, 0);

        //if the module has gone into an error state we probably don't have any requests (because most module level errors happen before we would)
        // but if we did we should get rid of them since the only course of action would be to start the qos process again.
        while (pQosClient->pRequestQueue != NULL)
        {
            _QosClientDestroyRequest(pQosClient, pQosClient->pRequestQueue->uClientRequestID);
        }
        pQosClient->eModuleState = MODULE_STATE_REPORT;
        return;
    }

    // errors tell the coordinator about it right away, keep whatever results we can and let the coordinator figgure out what to do
    if (eState == MODULE_STATE_ERROR)
    {
        pQosClient->eModuleState = MODULE_STATE_INIT_COORDINATOR_COMM;
        return;
    }

    //if nothing else has returned set the new state
    pQosClient->eModuleState = eState;
}

/*F********************************************************************************/
/*!
    \Function _QosClientUpdateLocalAddress

    \Description
        Update the local address used for udp communication.

    \Input *pQosClient  - pointer to module state

    \Version 09/06/2017 (cvienneau)
*/
/********************************************************************************F*/
static void _QosClientUpdateLocalAddress(QosClientRefT *pQosClient)
{
#if DIRTYCODE_LOGGING
    char addrBuff[128];
#endif    
    // if we haven't set the local address information, do so now
    if (pQosClient->localUdpAddrInfo.uFamily == 0)
    {
        //xbox and ps4 http don't have the api to read the local addr from protohttp, but they also don't support ipv6 or have multiple nics so we can do something simpler
        #if defined(DIRTYCODE_PS4) || defined(DIRTYCODE_XBOXONE)
            pQosClient->localUdpAddrInfo.uFamily = AF_INET;
            pQosClient->localUdpAddrInfo.addr.v4 = SocketGetLocalAddr();
        #else
            struct sockaddr LocalAddr;
            if (ProtoHttp2Status(pQosClient->pProtoHttp, 0, 'ladd', &LocalAddr, sizeof(LocalAddr)) == 0)
            {
                //if the address family of the cached value is 0, it hasn't actually been cached yet
                if (LocalAddr.sa_family != 0)
                {
                    // cache off the info about the address we are bound to, this will be sent to the coordinator as our pQosClient->rawResultStore.clientAddressInternal
                    // it needs caching since we will clear the rawResultStore if multiple tests are done, but we only bind the socket once
                    // technically this is the address used for the http communication, but it can be difficult to get the udp address so we will presume they will be the same.
                    QosCommonConvertAddr(&pQosClient->localUdpAddrInfo, &LocalAddr);
                }
            }
            else
            {
                NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: localUdpAddrInfo failed to get cached local address.\n"));
            }
        #endif
        
        // change the info to be shared if we have updated the local address info
        if (pQosClient->localUdpAddrInfo.uFamily != 0)
        {
            pQosClient->localUdpAddrInfo.uPort = pQosClient->uUdpCurrentListenPort;             //override the port to the one we know the udp address was bound to
            pQosClient->rawResultStore.clientModifiers.clientAddressInternal = pQosClient->localUdpAddrInfo;    //set the local address information to be sent on next coordinator communication 
            NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: localUdpAddrInfo address updated to %s\n", QosCommonAddrToString(&pQosClient->localUdpAddrInfo, addrBuff, sizeof(addrBuff))));
        }
    }

    // if the port from the udp socket doesn't match what we have cached update it
    if (pQosClient->localUdpAddrInfo.uPort != pQosClient->uUdpCurrentListenPort)
    {
        pQosClient->localUdpAddrInfo.uPort = pQosClient->uUdpCurrentListenPort;             //override the port to the one we know the udp address was bound to
        pQosClient->rawResultStore.clientModifiers.clientAddressInternal = pQosClient->localUdpAddrInfo;    //set the local address information to be sent on next coordinator communication 
        NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: localUdpAddrInfo port updated to %s\n", QosCommonAddrToString(&pQosClient->localUdpAddrInfo, addrBuff, sizeof(addrBuff))));
    }
}

/*F********************************************************************************/
/*!
    \Function _QosClientSocketOpen

    \Description
        Open QosClient socket

    \Input *pQosClient  - pointer to module state

    \Output 
        SocketT *       - pointer to socket

    \Version 09/13/2013 (jbrookes)
*/
/********************************************************************************F*/
static SocketT *_QosClientSocketOpen(QosClientRefT *pQosClient)
{
    struct sockaddr LocalAddr;
    int32_t iResult;
    SocketT *pSocket;

    // set the port to the default value if 0
    if (pQosClient->uUdpRequestedListenPort == 0)
    {
        pQosClient->uUdpRequestedListenPort = QOS_CLIENT_DEFAULT_LISTENPORT;
    }

    // create the socket
    if ((pSocket = SocketOpen(AF_INET, SOCK_DGRAM, 0)) == NULL)
    {
        NetPrintf(("qosclient: could not allocate socket\n"));
        return(NULL);
    }

    // make sure socket receive buffer is large enough to queue up
    // multiple probe responses (worst case: 10 probes * 1200 bytes)
    SocketControl(pSocket, 'rbuf', pQosClient->uUdpReceiveBuffSize, NULL, NULL);
    SocketControl(pSocket, 'sbuf', pQosClient->uUdpSendBuffSize, NULL, NULL);
    SocketControl(pSocket, 'pque', pQosClient->uUdpPacketQueueSize, NULL, NULL);

    SockaddrInit(&LocalAddr, AF_INET);
    SockaddrInSetPort(&LocalAddr, pQosClient->uUdpRequestedListenPort);

    // bind the socket
    if ((iResult = SocketBind(pSocket, &LocalAddr, sizeof(LocalAddr))) != SOCKERR_NONE)
    {
        NetPrintf(("qosclient: error %d binding socket to port %d, trying random\n", iResult, pQosClient->uUdpRequestedListenPort));
        SockaddrInSetPort(&LocalAddr, 0);
        if ((iResult = SocketBind(pSocket, &LocalAddr, sizeof(LocalAddr))) != SOCKERR_NONE)
        {
            NetPrintf(("qosclient: error %d binding socket to listen\n", iResult));
            SocketClose(pSocket);
            return(NULL);
        }
    }

    // set the current listen port
    SocketInfo(pSocket, 'bind', 0, &LocalAddr, sizeof(LocalAddr));
    pQosClient->uUdpCurrentListenPort = SockaddrInGetPort(&LocalAddr);
    NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: bound probe socket to port %u\n", pQosClient->uUdpCurrentListenPort));

    //update the clients local address, as new info becomes available (we just discovered the port)
    _QosClientUpdateLocalAddress(pQosClient);

    // set the callback
    SocketCallback(pSocket, CALLB_RECV, QOS_CLIENT_SOCKET_IDLE_RATE, pQosClient, &_QosClientRecvCB);
    // return to caller
    return(pSocket);
}

/*F********************************************************************************/
/*!
    \Function _QosClientDestroyRequest

    \Description
        Remove request from queue and free any memory associated with the request.

    \Input *pQosClient  - pointer to module state
    \Input uRequestId   - id of the request to destroy

    \Output 
        int32_t - 0 if successful, negative otherwise

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static int32_t _QosClientDestroyRequest(QosClientRefT *pQosClient, uint32_t uRequestId)
{
    QosClientRequestT *pQosClientRequest, **ppQosClientRequest;

    // find request in queue
    for (ppQosClientRequest = &pQosClient->pRequestQueue; *ppQosClientRequest != NULL; ppQosClientRequest = &(*ppQosClientRequest)->pNext)
    {
        // found the request to destroy?
        if ((*ppQosClientRequest)->uClientRequestID == uRequestId)
        {
            // set the request
            pQosClientRequest = *ppQosClientRequest;

            // dequeue
            *ppQosClientRequest = (*ppQosClientRequest)->pNext;

            // destroy host name lookup if we have one in progress
            if (pQosClientRequest->pQosServerHost != NULL)
            {
                pQosClientRequest->pQosServerHost->Free(pQosClientRequest->pQosServerHost);
                pQosClientRequest->pQosServerHost = NULL;
            }

            // free memory
            DirtyMemFree(pQosClientRequest, QOS_CLIENT_MEMID, pQosClient->iMemGroup, pQosClient->pMemGroupUserData);

            return(0);
        }
    }

    // specified request not found
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _QosClientGetRequest

    \Description
        Get a request by request ID.

    \Input *pQosClient      - pointer to module state
    \Input uRequestId       - id of the request to get

    \Output
        QosClientRequestT *    -   pointer to request or NULL if not found

    \Version 08/16/2011 (jbrookes)
*/
/********************************************************************************F*/
static QosClientRequestT *_QosClientGetRequest(QosClientRefT *pQosClient, uint32_t uRequestId)
{
    QosClientRequestT *pRequest;

    // find request in queue
    for (pRequest = pQosClient->pRequestQueue; pRequest != NULL; pRequest = pRequest->pNext)
    {
        // found the request?
        if (pRequest->uClientRequestID == uRequestId)
        {
            return(pRequest);
        }
    }
    // did not find the request
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function _QosClientValidateTest

    \Description
        Validate that a test contains all the required information.

    \Input *pQosClient  - pointer to module state
    \Input *pTest       - pointer test to be validated

    \Output
        int32_t         - 0 if valid test, negative otherwise

    \Version 06/05/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosClientValidateTest(QosClientRefT *pQosClient, QosCommonTestT *pTest)
{
    if ((strcmp(pTest->strSiteName, "") == 0) || (strcmp(pTest->strTestName, "") == 0))
    {
        NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: test %s skipped, incomplete information, strSiteName=%s.\n", pTest->strTestName, pTest->strSiteName));
        return(-1);
    } 

    if (!((pTest->uProbeCountUp == 1) || (pTest->uProbeCountDown == 1)))
    {
        NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: test %s skipped, uProbeCountUp(%d) or uProbeCountDown(%d) must be 1, we do not support many to many probes on this client.\n", pTest->strTestName, pTest->uProbeCountUp, pTest->uProbeCountDown));
        return(-2);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _QosClientValidateSite

    \Description
        Validate that a site contains all the required information.

    \Input *pQosClient  - pointer to module state
    \Input *pSite       - pointer site to be validated

    \Output
        int32_t         - 0 if valid site, negative otherwise

    \Version 06/05/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosClientValidateSite(QosClientRefT *pQosClient, QosCommonSiteT *pSite)
{
    // validate that the site has all the info we require
    if ((pSite->uProbePort == 0) || (strcmp(pSite->strSiteName, "") == 0) || (strcmp(pSite->strProbeAddr, "") == 0))
    {
        NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: site skipped, did not have complete information; site:%s, addr:%s:%d\n", pSite->strSiteName, pSite->strProbeAddr, pSite->uProbePort));
        return(-1);
    }

    if (!QosCommonIsCompatibleProbeVersion(pSite->uProbeVersion))
    {
        NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: site provided doesn't have a compatible version. %d, vs %d.\n", pSite->uProbeVersion, QOS_COMMON_PROBE_VERSION));
        return(-1);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _QosClientControlStrToControlInt

    \Description
        Convert a string to a DirtySDK control selector, ie "abcd" = 'abcd'.

    \Input *pStr    - string to convert

    \Output
        int32_t     - the integer value of the four character code

    \Version 06/05/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosClientControlStrToControlInt(const char* pStr)
{
    //control configs must be four characters long
    if (strlen(pStr) == 4)
    {
        int32_t controlInt = 0;
        // extract the type
        controlInt = pStr[3];
        controlInt |= pStr[2] << 8;
        controlInt |= pStr[1] << 16;
        controlInt |= pStr[0] << 24;
        return(controlInt);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _QosClientApplyControlConfigs

    \Description
        Apply any QosClientControl selector overrides that coordinator has requested.

    \Input *pQosClient  - pointer to module state
    \Input *pConfig     - collection of config overrides provided by the coordinator

    \Output
        int32_t         - number of config overrides applied.

    \Version 06/05/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosClientApplyControlConfigs(QosClientRefT *pQosClient, QosCommonClientConfigT *pConfig)
{
    int32_t i;
    for (i = 0; i < pConfig->uNumControlConfigs; i++)
    {
        uint32_t controlInt = _QosClientControlStrToControlInt(pConfig->aControlConfigs[i].strControl);
        QosClientControl(pQosClient, controlInt, pConfig->aControlConfigs[i].iValue, pConfig->aControlConfigs[i].strValue);
    }
    return(i);
}

/*F********************************************************************************/
/*!
    \Function _QosClientCreateRequest

    \Description
        Enqueue a test to be performed against a ping site

    \Input *pQosClient          - module state
    \Input *pSite               - information on what ping site to perform the test against
    \Input *pTest               - information on what settings to use while doing the test
    \Input uServiceRequestID    - id for a batch of requests, provided by the coordinator

    \Output
        int32_t             - request id, negative on failure.

    \Version 03/31/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosClientCreateRequest(QosClientRefT *pQosClient, QosCommonSiteT *pSite, QosCommonTestT *pTest, uint32_t uServiceRequestID)
{
    QosClientRequestT *pRequest, **ppQosClientRequest;

    NetCritEnter(&pQosClient->ThreadCrit);
    // if the module's socket hasn't yet been allocated (do this a bit lazy so we can possibly configure the socket after creation)
    if ((pQosClient->pUdpSocket == NULL) && ((pQosClient->pUdpSocket = _QosClientSocketOpen(pQosClient)) == NULL))
    {
        NetPrintf(("qosclient: error opening listening socket.\n"));
        NetCritLeave(&pQosClient->ThreadCrit);
        return(-1);
    }

    // allocate a request
    if ((pRequest = (QosClientRequestT*)DirtyMemAlloc(sizeof(*pRequest), QOS_CLIENT_MEMID, pQosClient->iMemGroup, pQosClient->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("qosclient: error allocating request.\n"));
        NetCritLeave(&pQosClient->ThreadCrit);
        return(-1);
    }
    ds_memclr(pRequest, sizeof(*pRequest));

    // find end of queue and append
    for (ppQosClientRequest = &pQosClient->pRequestQueue; *ppQosClientRequest != NULL; ppQosClientRequest = &(*ppQosClientRequest)->pNext)
    {
    }
    *ppQosClientRequest = pRequest;

    // set the request values
    pRequest->uServiceRequestID = uServiceRequestID;
    pRequest->uClientRequestID = pQosClient->uNextClientRequestID++;

    pRequest->site = *pSite;
    pRequest->test = *pTest;

    pRequest->uProbesToSend = pRequest->test.uProbeCountUp;
    pRequest->uWhenStarted = NetTick();
    ds_strnzcpy(pRequest->rawResults.strSiteName, pRequest->site.strSiteName, sizeof(pRequest->rawResults.strSiteName));
    ds_strnzcpy(pRequest->rawResults.strTestName, pRequest->test.strTestName, sizeof(pRequest->rawResults.strTestName));

    if (_QosClientValidateSite(pQosClient, &pRequest->site) < 0)
    {
        _QosClientRequestStateTransition(pQosClient, pRequest, REQUEST_STATE_ERROR, DIRTYAPI_QOS, QOS_CLIENT_REQUEST_STATUS_ERROR_SITE_INVALID, 0);
    }
    else if (_QosClientValidateTest(pQosClient, &pRequest->test) < 0)
    {
        _QosClientRequestStateTransition(pQosClient, pRequest, REQUEST_STATE_ERROR, DIRTYAPI_QOS, QOS_CLIENT_REQUEST_STATUS_ERROR_TEST_INVALID, 0);
    }
    else
    {
        _QosClientRequestStateTransition(pQosClient, pRequest, REQUEST_STATE_INIT, DIRTYAPI_QOS, QOS_CLIENT_REQUEST_STATUS_INIT, 0);
    }

    NetCritLeave(&pQosClient->ThreadCrit);
    return(pRequest->uClientRequestID);
}

/*F********************************************************************************/
/*!
    \Function _QosClientGetOsFirewallType

    \Description
        Queries DirtySDK to see if there is a NAT override to use in place of the value obtained by QOS

    \Output
        QosCommonFirewallTypeE - NAT type.

    \Version 06/05/2017 (cvienneau)
*/
/********************************************************************************F*/
static QosCommonFirewallTypeE _QosClientGetOsFirewallType(void)
{
    #if defined(DIRTYCODE_XBOXONE) && !defined(DIRTYCODE_GDK)
    int32_t iResult = 0;
    int32_t iNatType;

    iResult = NetConnStatus('natt', 0, &iNatType, sizeof(iNatType));
    if (iResult < 0)
    {
        NetPrintf(("qosclient: _QosClientGetOsFirewallType() error retrieving NAT type\n"));
        return(QOS_COMMON_FIREWALL_UNKNOWN);
    }

    if (iNatType == 0)
    {
        return(QOS_COMMON_FIREWALL_OPEN);
    }
    else if (iNatType == 1)
    {
        return(QOS_COMMON_FIREWALL_MODERATE);
    }
    else if (iNatType == 2)
    {
        return(QOS_COMMON_FIREWALL_STRICT);
    }

    NetPrintf(("qosclient: _QosClientGetOsFirewallType() unknown error\n"));
    return(QOS_COMMON_FIREWALL_UNKNOWN);
    #else
    return(QOS_COMMON_FIREWALL_UNKNOWN);
    #endif
}

/*F********************************************************************************/
/*!
    \Function _QosClientOverrideFinalResults

    \Description
        Give the QosClient an opportunity to do final tweaks to the results before 
        providing the results to the game client.  Currently not used.

    \Input *pQosClient          - module state

    \Version 03/31/2017 (cvienneau)
*/
/********************************************************************************F*/
static void _QosClientOverrideFinalResults(QosClientRefT *pQosClient)
{
}

/*F********************************************************************************/
/*!
    \Function _QosClientPurgeCurrentResults

    \Description
        Clear any raw results we might be hanging on to

    \Input *pQosClient  - pointer to module state

    \Version 03/16/2018 (cvienneau)
*/
/********************************************************************************F*/
static void _QosClientPurgeCurrentResults(QosClientRefT *pQosClient)
{
    pQosClient->rawResultStore.uNumResults = 0;
    ds_memclr(pQosClient->rawResultStore.aResults, sizeof(pQosClient->rawResultStore.aResults));
    pQosClient->rawResultStore.clientModifiers.uStallCountCoordinator = 0;
    pQosClient->rawResultStore.clientModifiers.uStallCountProbe = 0;
    pQosClient->rawResultStore.clientModifiers.uStallDurationCoordinator = 0;
    pQosClient->rawResultStore.clientModifiers.uStallDurationProbe = 0;
}

/*F********************************************************************************/
/*!
    \Function _QosClientProcessCoordinatorResponse

    \Description
        Parses the rpc response from the coordinator.

    \Input *pQosClient  - pointer to module state

    \Output 
        int32_t      - negative on error, 0 on success

    \Version 03/31/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosClientProcessCoordinatorResponse(QosClientRefT *pQosClient)
{
    int32_t iRet = 0;
    QosCommonCoordinatorToClientResponseT response;

    NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: <-- http response:\n"));
    #if DIRTYCODE_LOGGING
        if (pQosClient->iSpamValue >= 2)
        {
            NetPrintMem(pQosClient->pHttpRecvBuff, pQosClient->uCurrentHttpRecvSize, "CoordinatorToClientResponse");
        }
    #endif

    if (QosCommonCoordinatorToClientResponseDecode(&response, pQosClient->pHttpRecvBuff, pQosClient->uCurrentHttpRecvSize) == 0)
    {
        int32_t testIdx, siteIdx;
        pQosClient->uInitSyncTime = NetTick();

        // detailed print
        QosCommonCoordinatorToClientPrint(&response, pQosClient->iSpamValue);

        //validate the service id is what we expect it to be
        if ((pQosClient->rawResultStore.uServiceRequestID != 0) && (pQosClient->rawResultStore.uServiceRequestID != response.uServiceRequestID))
        {
            NetPrintf(("qosclient: warning, expected uServiceRequestID %d, but got %d.\n", pQosClient->rawResultStore.uServiceRequestID, response.uServiceRequestID));
        }
        pQosClient->rawResultStore.uServiceRequestID = response.uServiceRequestID;
        pQosClient->clientAddressFromCoordinator = response.configuration.clientAddressFromCoordinator;

        //apply the client configs if there are any, this may change many behaviors
        if (response.configuration.uNumControlConfigs > 0)
        {
            _QosClientApplyControlConfigs(pQosClient, &response.configuration);
        }        
        
        if (response.results.uNumResults > 0)
        {
            //read our results and complete the QOS process
            pQosClient->finalResults = response.results;
            pQosClient->uFinalResultsRecvTime = NetTick();
            _QosClientOverrideFinalResults(pQosClient);
            _QosClientModuleStateTransition(pQosClient, MODULE_STATE_REPORT, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_RESULTS_RECEIVED, 0);
        }
        else if ((response.configuration.uNumQosTests > 0) && (response.configuration.uNumSites > 0))
        {
            // get rid of existing raw results, so when the new tests finish we don't send the old raw results to the coordinator again
            if (pQosClient->rawResultStore.uNumResults > 0)
            {
                NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: purging %d existing raw results, continuing with multi-phased qos test.\n", pQosClient->rawResultStore.uNumResults));
                _QosClientPurgeCurrentResults(pQosClient);
            }

            //convert this collection of information we have into a bunch of discrete request
            for (testIdx = 0; testIdx  < response.configuration.uNumQosTests; testIdx++)
            {
                for (siteIdx = 0; siteIdx < response.configuration.uNumSites; siteIdx++)
                {
                    // if the test is against all sties, or if the site is contained in the list of sites the test should be performed against
                    if ((strcmp(response.configuration.aQosTests[testIdx].strSiteName, "ALL") == 0) || (strstr(response.configuration.aQosTests[testIdx].strSiteName, response.configuration.aSites[siteIdx].strSiteName) != NULL))
                    {
                        _QosClientCreateRequest(pQosClient, &response.configuration.aSites[siteIdx], &response.configuration.aQosTests[testIdx], response.uServiceRequestID);
                    }
                }
            }
            if (pQosClient->pRequestQueue != NULL)
            {
                _QosClientModuleStateTransition(pQosClient, MODULE_STATE_PROBE, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_TEST_CONFIG_RECIEVED, 0);
            }
            else
            {
                NetPrintf(("qosclient: error, message from QosCoordinator did not result in any actionable requests.\n"));
                _QosClientModuleStateTransition(pQosClient, MODULE_STATE_ERROR, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_ERROR_TEST_CONFIG_PRODUCED_NO_REQUESTS, 0);
                iRet = -1;
            }
        }
        else
        {
            NetPrintf(("qosclient: error, message from QosCoordinator was not actionable; uNumQosTests:%d, uNumSites:%d.\n", response.configuration.uNumQosTests, response.configuration.uNumSites));
            _QosClientModuleStateTransition(pQosClient, MODULE_STATE_ERROR, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_ERROR_TEST_CONFIG_UNUSABLE, 0);
            iRet = -1;
        }
    }
    else
    {
        NetPrintf(("qosclient: error, QosCommonCoordinatorToClientResponseDecode failed.\n"));
        _QosClientModuleStateTransition(pQosClient, MODULE_STATE_ERROR, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_ERROR_RPC_DECODE, 0);
        iRet = -1;
    }
    return(iRet);
}

/*F********************************************************************************/
/*!
    \Function _QosClientRequestInit

    \Description
        Do any preparations such as the dns resolve of the QOS server before beginning 
        to send probes.

    \Input *pQosClient  - pointer to module state
    \Input *pRequest    - pointer to the request which contains the site info

    \Version 03/31/2017 (cvienneau)
*/
/********************************************************************************F*/
static void _QosClientRequestInit(QosClientRefT *pQosClient, QosClientRequestT *pRequest)
{
    // check if we need to do a lookup of the address
    if (pRequest->pQosServerHost == 0)
    {
        if ((pRequest->pQosServerHost = SocketLookup(pRequest->site.strProbeAddr, 30*1000)) == NULL)
        {
            _QosClientRequestStateTransition(pQosClient, pRequest, REQUEST_STATE_ERROR, DIRTYAPI_PROTO_HTTP, QOS_CLIENT_REQUEST_STATUS_ERROR_SOCKET_LOOKUP_ALLOC, 0);
        }
    }
    else // we are in the process of looking up the address
    {
        // check if we are done resolving the host name
        if (pRequest->pQosServerHost->Done(pRequest->pQosServerHost) == TRUE)
        {
            if (pRequest->pQosServerHost->addr != 0) 
            {
                // we finished successfully, update the address from the lookup
                SockaddrInit(&pRequest->serverAddr, AF_INET);
                SockaddrInSetAddr(&pRequest->serverAddr, pRequest->pQosServerHost->addr);
                SockaddrInSetPort(&pRequest->serverAddr, pRequest->site.uProbePort);
                _QosClientRequestStateTransition(pQosClient, pRequest, REQUEST_STATE_INIT_SYNC, DIRTYAPI_QOS, QOS_CLIENT_REQUEST_STATUS_SOCKET_LOOKUP_SUCCESS, 0);
            }
            else 
            {
                // we finished but we don't have a valid address
                _QosClientRequestStateTransition(pQosClient, pRequest, REQUEST_STATE_ERROR, DIRTYAPI_PROTO_HTTP, QOS_CLIENT_REQUEST_STATUS_ERROR_SOCKET_LOOKUP_UPDATE, 0);
            }

            // we are done with the address lookup, free the resources
            pRequest->pQosServerHost->Free(pRequest->pQosServerHost);
            pRequest->pQosServerHost = NULL;
        }
    }

}

/*F********************************************************************************/
/*!
    \Function _QosClientRequestWaitForInit

    \Description
        Transition to the send state if no other request are in the init state.
        We prefer to start all tests at the same time because if the users connection 
        suffers from periodic lag spikes, this will produce a more "fair" result.  Otherwise what 
        might be the best ping site can suffer from a random occurrence, and a less ideal ping 
        site which was lucky enough to avoid the lag spike will come out as the best ping site.
        Now all ping sites should avoid or suffer from the same lag spike since they are occurring
        at the same time.

    \Input *pQosClient  - pointer to module state
    \Input *pRequest    - pointer to the request

    \Version 08/11/2016 (cvienneau)
*/
/********************************************************************************F*/
static void _QosClientRequestWaitForInit(QosClientRefT *pQosClient, QosClientRequestT *pRequest)
{
    QosClientRequestT *pRequestToCheck;

    if (NetTickDiff(NetTick(), pQosClient->uInitSyncTime) > pRequest->test.uInitSyncTimeout) //too much time passed
    {
        NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: request[%02d] giving up on waiting for init, %dms have already passed.\n", pRequest->uClientRequestID, pRequest->test.uInitSyncTimeout));
    }
    else
    {
        for (pRequestToCheck = pQosClient->pRequestQueue; pRequestToCheck != NULL; pRequestToCheck = pRequestToCheck->pNext)
        {
            if (pRequestToCheck->eState == REQUEST_STATE_INIT)
            {
                return;//one of the requests is still waiting to resolve the target address, lets continue to wait so that we can all measure connection metrics at the same time
            }
        }
    }
    _QosClientRequestStateTransition(pQosClient, pRequest, REQUEST_STATE_SEND, DIRTYAPI_QOS, QOS_CLIENT_REQUEST_STATUS_INIT_SYNC_SUCCESS, 0);
}

/*F********************************************************************************/
/*!
    \Function _QosClientRequestSendProbes

    \Description
        Sends probes to the request's target as necessary.

    \Input *pQosClient  - pointer to module state
    \Input *pRequest    - pointer to the request

    \Version 11/07/2014 (cvienneau)
*/
/********************************************************************************F*/
static void _QosClientRequestSendProbes(QosClientRefT *pQosClient, QosClientRequestT *pRequest)
{
    uint32_t uSendCounter;
    int32_t iResult = 0;
    uint32_t uProbeCountToSend = 1;        //send probes one at a time, unless uMinTimeBetwenProbes == 0
    uint8_t aSendBuff[QOS_COMMON_MAX_PACKET_SIZE];
    QosCommonProbePacketT probe;

    // figure out what external address best describes where the server will be sending its probes to, for security 
    // server will verify the probes are coming from this address, we switch to address from server if its available
    if (pRequest->rawResults.clientAddressFromServer.uFamily == 0)
    {
        pRequest->probeExternalAddr = pQosClient->clientAddressFromCoordinator;
    }
    else
    {
        pRequest->probeExternalAddr = pRequest->rawResults.clientAddressFromServer;
    }    
    
    ds_memclr(&probe, sizeof(probe));
    ds_memclr(aSendBuff, sizeof(aSendBuff));

    probe.uProtocol = QOS_COMMON_PROBE_PROTOCOL_ID;
    probe.uVersion = QOS_COMMON_PROBE_VERSION;
    probe.uServiceRequestId = pRequest->uServiceRequestID;
    //probe.uServerReceiveTime; server use only
    //probe.uServerSendDuration; server use only
    probe.uProbeSizeUp = pRequest->test.uProbeSizeUp;
    probe.uProbeSizeDown = pRequest->test.uProbeSizeDown; 
    //probe.uProbeCountUp = pRequest->test.uProbeCountUp;  //modified for each probe in loop below
    probe.uProbeCountDown = pRequest->test.uProbeCountDown;
    probe.uClientRequestId = pRequest->uClientRequestID;
    probe.clientAddressFromService = pRequest->probeExternalAddr;
    probe.uExpectedProbeCountUp = pRequest->test.uProbeCountUp;
    //probe.additionalBytes filled with memset send buffer

    // should we send all the probes in a burst?
    if (pRequest->test.uMinTimeBetwenProbes == 0)
    {
        uProbeCountToSend = pRequest->uProbesToSend - pRequest->rawResults.uProbeCountUp;  //target number of probes to send - the number of probes we already sent
    }

    // send probes
    for (uSendCounter = 0; uSendCounter < uProbeCountToSend; uSendCounter++)
    {
        if (pRequest->rawResults.uProbeCountUp >= QOS_COMMON_MAX_PROBE_COUNT)
        {
            //this can happen in a BW up test since we send a burst, but if we do lets bail
            NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: request[%02d] max probes have already been sent, not sending additional probes.\n", pRequest->uClientRequestID, &pRequest->serverAddr));
            //we don't really want to error immediately, better let it timeout so we can have a chance to receive any incoming probes (though likely is going to be a timeout scenario)
            _QosClientRequestStateTransition(pQosClient, pRequest, REQUEST_STATE_RECEIVE, DIRTYAPI_QOS, QOS_CLIENT_REQUEST_STATUS_ERROR_SENT_MAX_PROBES, 0);
            pRequest->uWhenLastSent = NetTick();
            return;
        }        
    
        probe.uProbeCountUp = pRequest->rawResults.uProbeCountUp;
        
        //probe is now complete, serialize it and sign it, into the send buffer (a place where there will be extra space for the additional bandwidth bytes)
        if (QosCommonSerializeProbePacket(aSendBuff, sizeof(aSendBuff), &probe, pRequest->site.aSecureKey) != QOS_COMMON_MIN_PACKET_SIZE)
        {
            NetPrintf(("qosclient: error serialized packet is not the expected size.\n"));
            _QosClientModuleStateTransition(pQosClient, MODULE_STATE_ERROR, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_ERROR_SERIALIZED_PACKET_UNEXPECTED_SIZE, 0);
            return;
        }

        iResult = SocketSendto(pQosClient->pUdpSocket, (const char *)aSendBuff, probe.uProbeSizeUp, 0, &pRequest->serverAddr, sizeof(pRequest->serverAddr));
        NetPrintfVerbose((pQosClient->iSpamValue, 2, "qosclient: request[%02d] sent probe %d to %A (result=%d)\n", pRequest->uClientRequestID, pRequest->rawResults.uProbeCountUp, &pRequest->serverAddr, iResult));

        if (iResult == probe.uProbeSizeUp)    //check to see that we were successful in our last send
        {
            pRequest->rawResults.aProbeResult[pRequest->rawResults.uProbeCountUp].uClientSendTime = NetTick();
            pRequest->rawResults.uProbeCountUp++;
            if (pRequest->rawResults.uProbeCountUp >= QOS_COMMON_MAX_PROBE_COUNT)
            {
                NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: request[%02d] max probes have now been sent, no additional probes will be sent.\n", pRequest->uClientRequestID, &pRequest->serverAddr));
            }        
        }
        else
        {
            break;          //some failure deal with it below
        }
    }

    if (iResult == probe.uProbeSizeUp)    //check to see that we were successful in our last send
    {
        NetPrintfVerbose((pQosClient->iSpamValue, 1, "qosclient: request[%02d] sent %d probes to address %A\n", pRequest->uClientRequestID, uProbeCountToSend, &pRequest->serverAddr));
        _QosClientRequestStateTransition(pQosClient, pRequest, REQUEST_STATE_RECEIVE, DIRTYAPI_QOS, QOS_CLIENT_REQUEST_STATUS_SEND_SUCCESS, 1);
        pRequest->uWhenLastSent = NetTick();
    }
    else if (iResult == 0)  // need to try again
    {
        NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: request[%02d] needs to try again to send probe to %A\n", pRequest->uClientRequestID, &pRequest->serverAddr));
        // transition anyways, let the re-issue logic handle the missing probes
        _QosClientRequestStateTransition(pQosClient, pRequest, REQUEST_STATE_RECEIVE, DIRTYAPI_QOS, QOS_CLIENT_REQUEST_STATUS_SEND_TRY_AGAIN, 0);
        pRequest->uWhenLastSent = NetTick();
    }    
    else if (iResult < 0)   // an error
    {
        NetPrintf(("qosclient: request[%02d] failed to send probe %d to address %A (err=%d)\n", pRequest->uClientRequestID, pRequest->rawResults.uProbeCountUp, &pRequest->serverAddr, iResult));
        _QosClientRequestStateTransition(pQosClient, pRequest, REQUEST_STATE_ERROR, DIRTYAPI_SOCKET, iResult, 0);
    }
    else                    // unexpected result
    {
        NetPrintf(("qosclient: request[%02d] unexpected result for probe %d to address %A (err=%d)\n", pRequest->uClientRequestID, pRequest->rawResults.uProbeCountUp, &pRequest->serverAddr, iResult));
        _QosClientRequestStateTransition(pQosClient, pRequest, REQUEST_STATE_ERROR, DIRTYAPI_SOCKET, iResult, 0);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosClientValidatePacketQueue

    \Description
        Ensure the packet queue 'pque' is in a healthy state, if its not print warnings.

    \Input *pQosClient  - pointer to module state

    \Output
        uint32_t        - amount of space left in packet queue

    \Version 03/31/2017 (cvienneau)
*/
/********************************************************************************F*/
static uint32_t _QosClientValidatePacketQueue(QosClientRefT *pQosClient)
{
    if (pQosClient->pUdpSocket != NULL)
    {
        //validate that the packet queue is healthy, it can be difficult to detect so we have special logging to check
        int32_t iQueueSize = SocketInfo(pQosClient->pUdpSocket, 'psiz', 0, NULL, 0);
        int32_t iQueueHighWater = SocketInfo(pQosClient->pUdpSocket, 'pmax', 0, NULL, 0);

#if DIRTYCODE_LOGGING
        int32_t iPacketDrop = SocketInfo(pQosClient->pUdpSocket, 'pdrp', 0, NULL, 0);

        //if we are getting close to using all of the packet queue complain always
        if (iQueueHighWater >= (iQueueSize - 2))
        {
            NetPrintf(("qosclient: warning %d/%d packet queue high watermark, lost packets %d.\n", iQueueHighWater, iQueueSize, iPacketDrop));
        }
        else
        {
            NetPrintfVerbose((pQosClient->iSpamValue, 2, "qosclient: %d/%d packet queue high watermark, lost packets %d.\n", iQueueHighWater, iQueueSize, iPacketDrop));
        }
#endif
        return(iQueueSize - iQueueHighWater);
    }

    //if we don't have a socket, just return the default
    return(QOS_CLIENT_DEFAULT_PACKET_QUEUE_SIZE);
}

/*F********************************************************************************/
/*!
    \Function _QosClientAllocHttpSendBuff

    \Description
        Allocate a send buffer of the specified size.

    \Input *pQosClient  - module state
    \Input uSize        - the size the send buffer should be

    \Output
        char*           - success, pointer to pQosClient->pHttpSendBuff; fail, NULL

    \Version 11/07/2014 (cvienneau)
*/
/********************************************************************************F*/
static uint8_t* _QosClientAllocHttpSendBuff(QosClientRefT *pQosClient, uint32_t uSize)
{
    DirtyMemGroupEnter(pQosClient->iMemGroup, pQosClient->pMemGroupUserData);

    // if the user is asking for a 0 size buffer they really just want to deallocate it
    if (uSize == 0)
    {
        if (pQosClient->pHttpSendBuff != NULL)
        {
            DirtyMemFree(pQosClient->pHttpSendBuff, QOS_CLIENT_MEMID, pQosClient->iMemGroup, pQosClient->pMemGroupUserData);
            pQosClient->pHttpSendBuff = NULL;
        }
        DirtyMemGroupLeave();
        pQosClient->uHttpSendBuffSize = 0;
        pQosClient->uHttpBytesToSend = 0;
        return(NULL);
    }

    // if the current size is good enough just use it
    if (pQosClient->uHttpSendBuffSize >= uSize)
    {
        DirtyMemGroupLeave();
        return(pQosClient->pHttpSendBuff);
    }

    // if we are going to allocate something, deallocate what we already have, if we had something
    _QosClientAllocHttpSendBuff(pQosClient, 0);  //this doesn't move the old data to the new buffer, but that isn't behavior we need right now

    // check to see the amount of space being asked for is sane
    if (uSize > QOS_COMMON_MAX_RPC_BODY_SIZE)
    {
        NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: purging %d existing raw results, the required send buffer is too large.\n", pQosClient->rawResultStore.uNumResults));
        _QosClientPurgeCurrentResults(pQosClient);
        _QosClientModuleStateTransition(pQosClient, MODULE_STATE_ERROR, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_ERROR_SEND_BUFFER_NEEDED_TOO_LARGE, 0);
        DirtyMemGroupLeave();
        return(NULL);
    }

    // allocate the amount of space that was asked for
    if ((pQosClient->pHttpSendBuff = DirtyMemAlloc(uSize, QOS_CLIENT_MEMID, pQosClient->iMemGroup, pQosClient->pMemGroupUserData)) == NULL)
    {
        //try again with no results
        if (pQosClient->rawResultStore.uNumResults > 0)
        {
            NetPrintf(("qosclient: purging %d existing raw results, error failed to allocate send buffer(%d)\n", pQosClient->rawResultStore.uNumResults, uSize));
            _QosClientPurgeCurrentResults(pQosClient);
            _QosClientModuleStateTransition(pQosClient, MODULE_STATE_ERROR, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_ERROR_FAILED_ALLOC_SEND_BUFFER, 0);
        }
        else // if we can't allocate a buffer with no resutls, its fatal
        {
            NetPrintf(("qosclient: error failed to allocate send buffer(%d)\n", uSize));
            _QosClientModuleStateTransition(pQosClient, MODULE_STATE_FATAL, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_ERROR_FAILED_ALLOC_SEND_BUFFER_FATAL, 0);
        }
        DirtyMemGroupLeave();
        return(NULL);
    }
    pQosClient->uHttpSendBuffSize = uSize;
    pQosClient->uHttpBytesToSend = 0;
    DirtyMemGroupLeave();
    return(pQosClient->pHttpSendBuff);
}


/*F********************************************************************************/
/*!
    \Function _QosClientAutoStart

    \Description
        The coordinator may have told us to run a tests again after some delay,
        if it wasn't satisfied with the results it generated.

    \Input *pQosClient  - pointer to module state

    \Version 08/02/2017 (cvienneau)
*/
/********************************************************************************F*/
static void _QosClientAutoStart(QosClientRefT *pQosClient)
{
    // see if the coordinator has told us to start a QOS process ourselves
    if (pQosClient->finalResults.uTimeTillRetry != 0)
    {
        if (NetTickDiff(NetTick(), pQosClient->uFinalResultsRecvTime) > (int32_t)pQosClient->finalResults.uTimeTillRetry)
        {
            //save off the last profile name to pass back in
            char strQosProfile[QOS_COMMON_MAX_RPC_STRING];
            ds_snzprintf(strQosProfile, sizeof(strQosProfile), "%s", pQosClient->rawResultStore.strQosProfile);
            QosClientStart(pQosClient, pQosClient->strCoordinatorAddr, pQosClient->uCoordinatorPort, strQosProfile);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _QosClientMessageCoordinator

    \Description
        Send message to the coordinator letting it know the current state of the 
        QoS process.

    \Input *pQosClient  - pointer to module state

    \Version 03/31/2017 (cvienneau)
*/
/********************************************************************************F*/
static void _QosClientMessageCoordinator(QosClientRefT *pQosClient)
{
    char strUrl[QOS_COMMON_MAX_URL_LENGTH];

    ProtoHttp2Update(pQosClient->pProtoHttp);

    // we don't have a send in progress yet
    if (pQosClient->uHttpBytesToSend == 0)
    {
        // get a send buffer
        if (_QosClientAllocHttpSendBuff(pQosClient, QosCommonClientToCoordinatorEstimateEncodedSize(&pQosClient->rawResultStore)) == NULL)
        {
            //_QosClientAllocHttpSendBuff will have state transitioned if it failed
            return;
        }
        
        // add any last minute data
        pQosClient->rawResultStore.clientModifiers.uPacketQueueRemaining = _QosClientValidatePacketQueue(pQosClient);   // check if packet queue sizes are becoming a problem
        pQosClient->rawResultStore.clientModifiers.eOSFirewallType = _QosClientGetOsFirewallType(); // tell the coordinator what the client thinks about the firewall

        // encode the rpc into the send buffer
        if ((pQosClient->pHttpSendProgress = QosCommonClientToCoordinatorRequestEncode(&pQosClient->rawResultStore, pQosClient->pHttpSendBuff, pQosClient->uHttpSendBuffSize, &pQosClient->uHttpBytesToSend)) != NULL)
        {
            int32_t iResult;
            const char *pStrHttpMode = "https";
            if (pQosClient->bUseHttps != TRUE)
            {
                pStrHttpMode = "http";
            }
            ds_snzprintf(strUrl, sizeof(strUrl), "%s://%s:%u%s", pStrHttpMode, pQosClient->strCoordinatorAddr, pQosClient->uCoordinatorPort, QOS_COMMON_CLIENT_URL);
            NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: --> sending http request:\n", strUrl));
            #if DIRTYCODE_LOGGING
                if (pQosClient->iSpamValue >= 2)
                {
                    NetPrintMem(pQosClient->pHttpSendBuff, pQosClient->uHttpBytesToSend, "ClientToCoordinatorRequest");
                }
                QosCommonClientToCoordinatorPrint(&pQosClient->rawResultStore, pQosClient->iSpamValue);
            #endif

            // we shouldn't have an active stream right now, but its possible through some error paths that it hasn't been cleaned up
            if (pQosClient->iHttpStreamId != 0)
            {
                NetPrintf(("qosclient: discarding stream %d.\n", pQosClient->iHttpStreamId));
                ProtoHttp2StreamFree(pQosClient->pProtoHttp, pQosClient->iHttpStreamId);
                pQosClient->iHttpStreamId = 0;
            }

            // start the send
            if ((iResult = ProtoHttp2Request(pQosClient->pProtoHttp, strUrl, NULL, PROTOHTTP2_STREAM_BEGIN, PROTOHTTP_REQUESTTYPE_POST, &pQosClient->iHttpStreamId)) >= 0)
            {
                pQosClient->pHttpSendProgress += iResult;
                pQosClient->uHttpBytesToSend -= iResult;
                //if uHttpBytesToSend is 0 we are done, otherwise there is still more data that needs to be sent, so stay in this state to do more ProtoHttpSends
                if (pQosClient->uHttpBytesToSend == 0)
                {
                    _QosClientModuleStateTransition(pQosClient, MODULE_STATE_UPDATE_COORDINATOR_COMM, DIRTYAPI_PROTO_HTTP, iResult, 0);
                }
            }
            else
            {
                _QosClientModuleStateTransition(pQosClient, MODULE_STATE_FATAL, DIRTYAPI_PROTO_HTTP, iResult, 0);
            }
        }
        else
        {
            NetPrintf(("qosclient: error, unable to QosCommonClientToCoordinatorRequestEncode.\n"));
            _QosClientModuleStateTransition(pQosClient, MODULE_STATE_FATAL, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_ERROR_RPC_ENCODE, 0);
        }
    }
    //proceed with sending of stream
    else
    {
        int32_t iResult;
        
        if ((iResult = ProtoHttp2Send(pQosClient->pProtoHttp, pQosClient->iHttpStreamId, pQosClient->pHttpSendProgress, pQosClient->uHttpBytesToSend)) >= 0)
        {
            pQosClient->pHttpSendProgress += iResult;
            pQosClient->uHttpBytesToSend -= iResult;
            //if uHttpBytesToSend is 0 we are done, otherwise there is still more data that needs to be sent, so stay in this state to do more ProtoHttpSends
            if (pQosClient->uHttpBytesToSend == 0)
            {
                ProtoHttp2Send(pQosClient->pProtoHttp, pQosClient->iHttpStreamId, NULL, PROTOHTTP2_STREAM_END);
                _QosClientModuleStateTransition(pQosClient, MODULE_STATE_UPDATE_COORDINATOR_COMM, DIRTYAPI_PROTO_HTTP, iResult, 0);
            }        
        }
        else
        {
            _QosClientModuleStateTransition(pQosClient, MODULE_STATE_FATAL, DIRTYAPI_PROTO_HTTP, iResult, 0);
        }        
    }
}

/*F********************************************************************************/
/*!
    \Function _QosClientValidateProbe

    \Description
        Validate that a probe packet meets all the requirements to be processed.

    \Input *pQosClient  - pointer to module state
    \Input *pProbe      - pointer probe to be validated

    \Output
        QosClientRequestT * - the request the probe belongs to, or NULL if validation fails.

    \Version 06/05/2017 (cvienneau)
*/
/********************************************************************************F*/
static QosClientRequestT* _QosClientValidateProbe(QosClientRefT *pQosClient, QosCommonProbePacketT* pProbe)
{
    QosClientRequestT *pRequest = NULL;

    // get request matching id
    if ((pRequest = _QosClientGetRequest(pQosClient, pProbe->uClientRequestId)) == NULL)
    {
        NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: request[%02d] could not be found, ignoring.\n", pProbe->uClientRequestId));
        return(NULL);
    }

    if (pProbe->uProtocol != QOS_COMMON_PROBE_PROTOCOL_ID)
    {
        NetPrintf(("qosclient: error probe packet not expected protocol (%d).\n", pProbe->uProtocol));
        pRequest->iLastProbeValidationError = QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_PROTOCOL;
        return(NULL);
    }

    if (!QosCommonIsCompatibleProbeVersion(pProbe->uVersion))
    {
        NetPrintf(("qosclient: error probe packet not compatible version (%d).\n", pProbe->uVersion));
        pRequest->iLastProbeValidationError = QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_VERSION;
        return(NULL);
    }

    // make sure the probe isn't smaller than could be possible
    if (pProbe->uProbeSizeUp < QOS_COMMON_MIN_PACKET_SIZE)
    {
        NetPrintf(("qosclient: error uProbeSizeUp=%d, is smaller than QOS_COMMON_MIN_PACKET_SIZE=%d\n", pProbe->uProbeSizeUp, QOS_COMMON_MIN_PACKET_SIZE));
        pRequest->iLastProbeValidationError = QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_UP_TOO_SMALL;
        return(NULL);
    }

    if (pProbe->uProbeSizeDown < QOS_COMMON_MIN_PACKET_SIZE)
    {
        NetPrintf(("qosclient: error uProbeSizeDown=%d, is smaller than QOS_COMMON_MIN_PACKET_SIZE=%d\n", pProbe->uProbeSizeDown, QOS_COMMON_MIN_PACKET_SIZE));
        pRequest->iLastProbeValidationError = QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_DOWN_TOO_SMALL;
        return(NULL);
    }

    // make sure the probe isn't larger than could be possible
    if (pProbe->uProbeSizeUp > QOS_COMMON_MAX_PACKET_SIZE)
    {
        NetPrintf(("qosclient: error uProbeSizeUp=%d, is larger than QOS_COMMON_MAX_PACKET_SIZE=%d\n", pProbe->uProbeSizeUp, QOS_COMMON_MAX_PACKET_SIZE));
        pRequest->iLastProbeValidationError = QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_UP_TOO_LARGE;
        return(NULL);
    }

    if (pProbe->uProbeSizeDown > QOS_COMMON_MAX_PACKET_SIZE)
    {
        NetPrintf(("qosclient: error uProbeSizeDown=%d, is larger than QOS_COMMON_MAX_PACKET_SIZE=%d\n", pProbe->uProbeSizeDown, QOS_COMMON_MAX_PACKET_SIZE));
        pRequest->iLastProbeValidationError = QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_DOWN_TOO_LARGE;
        return(NULL);
    }

    // make sure the incoming probe doesn't contain values we don't expect
    if (!(pProbe->clientAddressFromService.uFamily == AF_INET || pProbe->clientAddressFromService.uFamily == AF_INET6))
    {
        NetPrintf(("qosclient: error unexpected externalAddress.\n"));
        pRequest->iLastProbeValidationError = QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_MY_ADDRESS;
        return(NULL);
    }

    if (pProbe->uServerReceiveTime == 0)
    {
        NetPrintf(("qosclient: error, expected incoming probe expected uServerReceiveTime(%d), to be set.\n", pProbe->uServerReceiveTime));
        pRequest->iLastProbeValidationError = QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_SEVER_TIME;
        return(NULL);
    }

    // make sure the probe counts are within expected limits
    if (pProbe->uProbeCountDown > QOS_COMMON_MAX_PROBE_COUNT)
    {
        NetPrintf(("qosclient: error, uProbeCountDown(%d) higher than expected.\n", pProbe->uProbeCountDown));
        pRequest->iLastProbeValidationError = QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_DOWN_TOO_MANY;
        return(NULL);
    }

    if (pProbe->uProbeCountUp > QOS_COMMON_MAX_PROBE_COUNT)
    {
        NetPrintf(("qosclient: error, uProbeCountUp(%d) higher than expected.\n", pProbe->uProbeCountUp));
        pRequest->iLastProbeValidationError = QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_UP_TOO_MANY;
        return(NULL);
    }

    // validate that this request is part of the right service request
    if (pRequest->uServiceRequestID != pProbe->uServiceRequestId)
    {
        NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: request[%02d] uServiceRequestId's did not match request(%d), probe(%d).\n", pProbe->uClientRequestId));
        pRequest->iLastProbeValidationError = QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_SERVICE_REQUEST_ID;
        return(NULL);
    }

    // make sure it's not failed or timed out
    if (pRequest->eState == REQUEST_STATE_COMPLETE)
    {
        NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: request[%02d] response received but ignored, since the request is already completed.\n", pRequest->uClientRequestID));
        pRequest->iLastProbeValidationError = QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_ALREADY_COMPLETE;
        return(NULL);
    }

    // validate we haven't already received the probe for this slot (we will only know the server receive time if we have received it here on the client)
    if (pRequest->rawResults.aProbeResult[pProbe->uProbeCountDown].uServerReceiveTime != 0)
    {
        NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: request[%02d] response received but ignored, because we already received a probe to match the one sent uProbeCount(%d).\n", pRequest->uClientRequestID, pProbe->uProbeCountDown));
        pRequest->iLastProbeValidationError = QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_ALREADY_RECEIVED;
        return(NULL);
    }

    return(pRequest);
}

/*F********************************************************************************/
/*!
    \Function _QosClientSaveProbeResults

    \Description
        Store the results from receiving a probe, when all probes have been received the
        request is done.  Once all requests are done results will be shared with the 
        coordinator.

    \Input *pQosClient  - module state
    \Input *pRequest    - request associated to the probe
    \Input *pProbe      - incoming probe
    \Input *pRecvAddr   - address the probe came from (QOS server's address)

    \Version 06/05/2017 (cvienneau)
*/
/********************************************************************************F*/
static void _QosClientSaveProbeResults(QosClientRefT *pQosClient, QosClientRequestT *pRequest, QosCommonProbePacketT *pProbe, struct sockaddr *pRecvAddr)
{
    //update the result of the request
    pRequest->rawResults.aProbeResult[pProbe->uProbeCountDown].uServerReceiveTime = pProbe->uServerReceiveTime;
    pRequest->rawResults.aProbeResult[pProbe->uProbeCountDown].uServerSendDelta = pProbe->uServerSendDelta;

    // we didn't actually send a probe to match this receive, so use the send time of the first probe
    if (pRequest->test.uProbeCountUp == 1)
    {
        pRequest->rawResults.aProbeResult[pProbe->uProbeCountDown].uClientSendTime = pRequest->rawResults.aProbeResult[0].uClientSendTime;
        pRequest->rawResults.aProbeResult[pProbe->uProbeCountDown].uClientReceiveDelta = NetTickDiff(SockaddrInGetMisc(pRecvAddr), pRequest->rawResults.aProbeResult[0].uClientSendTime);
    }
    else
    {
        pRequest->rawResults.aProbeResult[pProbe->uProbeCountDown].uClientReceiveDelta = NetTickDiff(SockaddrInGetMisc(pRecvAddr), pRequest->rawResults.aProbeResult[pProbe->uProbeCountDown].uClientSendTime);
    }

    pRequest->rawResults.clientAddressFromServer = pProbe->clientAddressFromService;    //this would be the same from every probe for a given requests

    pRequest->rawResults.uProbeCountDown++;
    if (pRequest->rawResults.uProbeCountHighWater < pProbe->uProbeCountDown)
    {
        pRequest->rawResults.uProbeCountHighWater = pProbe->uProbeCountDown;
    }

    if (QosCommonIsAddrEqual(&pRequest->rawResults.clientAddressFromServer, &pRequest->probeExternalAddr) == FALSE)
    {
#if DIRTYCODE_LOGGING
        char addr1Buff[128];
        char addr2Buff[128];
#endif
        NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: request[%02d] QOS server does not agree that client external address is %s, it should be %s.\n", pRequest->uClientRequestID, QosCommonAddrToString(&pRequest->rawResults.clientAddressFromServer, addr1Buff, sizeof(addr1Buff)), QosCommonAddrToString(&pRequest->probeExternalAddr, addr2Buff, sizeof(addr2Buff))));
        // the coordinator and the QOS server disagree on what the external address of the client is, this will be ok for many test types
        // however the QOS server will not take part in sending multiple packets or large packets to a target that it isn't confident it has the right address 
        // in case of ip/spoofing packet replay is attempting to generate a DDOS attack
        // if we would like to do something like check our down bandwidth we must update our external address to agree with the QOS server who is performing the test
        if ((pRequest->test.uProbeCountDown > 1) || (pRequest->test.uProbeSizeDown > QOS_COMMON_MIN_PACKET_SIZE))
        {
            // if we want to update to the correct address, lets just make a whole new request so that we don't potentially have older probes on the wire interfering with our new results
            // but in case there is some real world internet scenario where the address never match up and we get caught in a loop creating requests, we should only do this max once per request
            if (pRequest->uCreationCount == 0)
            {
                NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: request[%02d] creating new request with updated external address(%s->%s).\n", pRequest->uClientRequestID, QosCommonAddrToString(&pRequest->rawResults.clientAddressFromServer, addr1Buff, sizeof(addr1Buff)), QosCommonAddrToString(&pRequest->probeExternalAddr, addr2Buff, sizeof(addr2Buff))));

                // mostly the new request is a copy of the old request
                int32_t iRequestID = _QosClientCreateRequest(pQosClient, &pRequest->site, &pRequest->test, pRequest->uServiceRequestID);
                QosClientRequestT *pNewRequest = _QosClientGetRequest(pQosClient, iRequestID);
                
                //the probes will be sent containing the address the QOS server expects to see, probeExternalAddr selects clientAddressFromServer over clientAddressFromCoordinator
                pNewRequest->rawResults.clientAddressFromServer = pRequest->rawResults.clientAddressFromServer;
                
                // so we don't do this again if it fails again for some reason
                pNewRequest->uCreationCount++;     
            }

            // let the old request complete early, this should prevent us from processing more of the probes from the old request
            _QosClientRequestStateTransition(pQosClient, pRequest, REQUEST_STATE_ERROR, DIRTYAPI_QOS, QOS_CLIENT_REQUEST_STATUS_ERROR_EXTERNAL_ADDRESS_MISMATCH, 0);
        }
    }

    //now that we've processed the probe, determine if we are ready to move on to the next state
    if (pRequest->rawResults.uProbeCountDown >= (pRequest->test.uProbeCountDown * pRequest->test.uProbeCountUp))
    {
        _QosClientRequestStateTransition(pQosClient, pRequest, REQUEST_STATE_COMPLETE, DIRTYAPI_QOS, QOS_CLIENT_REQUEST_STATUS_COMPLETE, 0);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosClientRecvResponse

    \Description
        Process a receive response.

    \Input *pQosClient  - module state
    \Input *pProbe      - received probe
    \Input *pRecvAddr   - source of packet data

    \Version 08/15/2011 (jbrookes) re-factored from _QosClientRecv()
*/
/********************************************************************************F*/
static void _QosClientRecvResponse(QosClientRefT *pQosClient, QosCommonProbePacketT *pProbe, struct sockaddr *pRecvAddr)
{
    QosClientRequestT *pRequest;

#if QOS_CLIENT_SIMULATE_PACKET_LOSS
    static uint32_t uTestCount=0;
    uTestCount++;
    if ((uTestCount%4) == 0)
    {
        NetPrintf(("qosclient: request[%02d] pretending probe %d was lost.\n", pProbe->uClientRequestId, pProbe->uProbeCountDown));
        return;
    }
#endif

    // make sure the probe is valid and get matching request
    if ((pRequest = _QosClientValidateProbe(pQosClient, pProbe)) == NULL)
    {
        //Validation function would have printed
        return;
    }

    //save the timing info from the probe and state transition if necessary
    _QosClientSaveProbeResults(pQosClient, pRequest, pProbe, pRecvAddr);
}

/*F********************************************************************************/
/*!
    \Function _QosClientRecv

    \Description
        Process QosClient socket data.

    \Input *pSocket - pointer to module socket
    \Input *pData   - pointer to module state

    \Output 
        int32_t - zero

    \Version 08/08/2011 (szhu)
*/
/********************************************************************************F*/
static int32_t _QosClientRecv(SocketT *pSocket, void *pData)
{
    QosClientRefT *pQosClient = (QosClientRefT *)pData;
    struct sockaddr_storage recvAddr;
    int32_t iAddrLen, iRecvLen;
    uint8_t aRecvBuff[QOS_COMMON_MAX_PACKET_SIZE];
    SockaddrInit((struct sockaddr*)&recvAddr, AF_INET);

    NetCritEnter(&pQosClient->ThreadCrit);
    while ((iRecvLen = SocketRecvfrom(pSocket, (char*)aRecvBuff, sizeof(aRecvBuff), 0, (struct sockaddr*)&recvAddr, &iAddrLen)) > 0)
    {
        if (iRecvLen >= QOS_COMMON_MIN_PACKET_SIZE)
        {
            QosClientRequestT *pRequest = NULL;
            QosCommonProbePacketT probe;
            uint16_t uClientRequestID = QosCommonDeserializeClientRequestId(aRecvBuff); //we need to get the client request id from the packet so we can look up the secure key to authenticate the rest of the probe
            NetPrintfVerbose((pQosClient->iSpamValue, 2, "qosclient: request[%02d] received probe (iRecvLen=%d).\n", uClientRequestID, iRecvLen));

            if ((pRequest = _QosClientGetRequest(pQosClient, uClientRequestID)) != NULL)
            {
                if (QosCommonDeserializeProbePacket(&probe, aRecvBuff, pRequest->site.aSecureKey, NULL) == 0)
                {
                    _QosClientRecvResponse(pQosClient, &probe, (struct sockaddr*)&recvAddr);
                }
                else
                {
                    NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: request[%02d] hmac test failed.\n", pRequest->uClientRequestID));
                    pRequest->iLastProbeValidationError = QOS_CLIENT_REQUEST_STATUS_ERROR_PROBE_HMAC;
                }
            }
            else
            {
                NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: request[%02d] QosCommonDeserializeClientRequestId() could not be found, ignoring.\n", uClientRequestID));
            }
        }
        else
        {
            NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: Discarding probe packet that was too small, %d bytes.\n", iRecvLen));
        }
    }
    NetCritLeave(&pQosClient->ThreadCrit);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _QosClientRecvCB

    \Description
        QosClient socket callback.

    \Input *pSocket - pointer to module socket
    \Input iFlags   - ignored
    \Input *pData   - pointer to module state

    \Output
        int32_t     - zero

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static int32_t _QosClientRecvCB(SocketT *pSocket, int32_t iFlags, void *pData)
{
    QosClientRefT *pQosClient = (QosClientRefT *)pData;
    // make sure we own resources
    if (NetCritTry(&pQosClient->ThreadCrit))
    {
        _QosClientRecv(pSocket, pData);
        // release resources
        NetCritLeave(&pQosClient->ThreadCrit);
    }
    else
    {
        pQosClient->bDeferredRecv = TRUE;
    }
    return(0);
}


/*F********************************************************************************/
/*!
    \Function _QosClientUpdateUdpProbes

    \Description
        Receive any incoming probes

    \Input *pQosClient     - module state

    \Version 09/19/2014 (cvienneau)
*/
/********************************************************************************F*/
static void _QosClientUpdateUdpProbes(QosClientRefT *pQosClient)
{
    /*
    check if we need to poll for data. we need to do this if:
        1. this platform does not implement an async receive thread
        2. there's a deferred receive call issued by async receive thread
    */
    if (pQosClient->pUdpSocket != NULL)
    {
        if (pQosClient->bDeferredRecv)
        {
            pQosClient->bDeferredRecv = FALSE;
            _QosClientRecv(pQosClient->pUdpSocket, pQosClient);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _QosClientAllocHttpRecvBuff

    \Description
        Allocate a receive buffer of the specified size.

    \Input *pQosClient  - module state
    \Input uSize        - the size the receive buffer should be
    
    \Output
        char*           - success, pointer to pQosClient->pHttpRecvBuff; fail, NULL

    \Version 11/07/2014 (cvienneau)
*/
/********************************************************************************F*/
static uint8_t* _QosClientAllocHttpRecvBuff(QosClientRefT *pQosClient, uint32_t uSize)
{
    uint8_t *pTemp;
    DirtyMemGroupEnter(pQosClient->iMemGroup, pQosClient->pMemGroupUserData);

    // if the user is asking for a 0 size buffer they really just want to deallocate it
    if (uSize == 0) 
    {
        if (pQosClient->pHttpRecvBuff != NULL)
        {
            DirtyMemFree(pQosClient->pHttpRecvBuff, QOS_CLIENT_MEMID, pQosClient->iMemGroup, pQosClient->pMemGroupUserData);
            pQosClient->pHttpRecvBuff = NULL;
        }
        DirtyMemGroupLeave();
        pQosClient->uHttpRecvBuffSize = 0;
        pQosClient->uCurrentHttpRecvSize = 0;
        return(NULL);
    }

    // if the current size is good enough just use it
    if (pQosClient->uHttpRecvBuffSize >= uSize)
    {
        DirtyMemGroupLeave();
        return(pQosClient->pHttpRecvBuff);
    }

    // check to see the amount of space being asked for is sane
    if (uSize > QOS_COMMON_MAX_RPC_BODY_SIZE)
    {
        _QosClientModuleStateTransition(pQosClient, MODULE_STATE_ERROR, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_ERROR_RECV_BUFFER_NEEDED_TOO_LARGE, 0);
        DirtyMemGroupLeave();
        return(NULL);
    }


    // allocate the amount of space that was asked for to a temporary variable
    if ((pTemp = DirtyMemAlloc(uSize, QOS_CLIENT_MEMID, pQosClient->iMemGroup, pQosClient->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("qosclient: error failed to allocate receive buffer(%d)\n", uSize));
        _QosClientModuleStateTransition(pQosClient, MODULE_STATE_FATAL, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_ERROR_FAILED_ALLOC_RECV_BUFFER, 0);
        DirtyMemGroupLeave();
        return(NULL);
    }

    // move old data to the new buffer, and get rid of the old buffer (if there was one)
    if (pQosClient->pHttpRecvBuff)
    {
        ds_memcpy(pTemp, pQosClient->pHttpRecvBuff, pQosClient->uHttpRecvBuffSize);
        _QosClientAllocHttpRecvBuff(pQosClient, 0);
    }

    pQosClient->pHttpRecvBuff = pTemp;
    pQosClient->uHttpRecvBuffSize = uSize;
    DirtyMemGroupLeave();
    return(pQosClient->pHttpRecvBuff);
}

/*F********************************************************************************/
/*!
    \Function _QosClientUpdateHttp

    \Description
        Receive any incoming http responses

    \Input *pQosClient     - module state

    \Version 11/07/2014 (cvienneau)
*/
/********************************************************************************F*/
static void _QosClientUpdateHttp(QosClientRefT *pQosClient)
{
    int32_t iResult;
    

    //service http module since we are expecting bytes to come in
    ProtoHttp2Update(pQosClient->pProtoHttp);
    
    //update the clients local address, as new info becomes available
    _QosClientUpdateLocalAddress(pQosClient);

    if (_QosClientAllocHttpRecvBuff(pQosClient, QOS_COMMON_DEFAULT_RPC_BODY_SIZE) == NULL)
    {
        // _QosClientAllocHttpRecvBuff will state transition on an error
        return;
    }

    if ((iResult = ProtoHttp2RecvAll(pQosClient->pProtoHttp, pQosClient->iHttpStreamId, pQosClient->pHttpRecvBuff, pQosClient->uHttpRecvBuffSize)) >= 0)
    {
        int32_t iHttpStatusCode = ProtoHttp2Status(pQosClient->pProtoHttp, pQosClient->iHttpStreamId, 'code', NULL, 0);
        if (iHttpStatusCode == PROTOHTTP_RESPONSE_SUCCESSFUL)
        {
            pQosClient->uCurrentHttpRecvSize = iResult;
            // parse the http response
            if (_QosClientProcessCoordinatorResponse(pQosClient) >= 0)
            {
                //we state transition inside _QosClientProcessCoordinatorResponse
                ProtoHttp2StreamFree(pQosClient->pProtoHttp, pQosClient->iHttpStreamId);
                pQosClient->iHttpStreamId = 0;
                pQosClient->uErrorMessageCount = 0;
            }
            else
            {
                pQosClient->uErrorMessageCount++;
                if (pQosClient->uErrorMessageCount > 1)
                {
                    NetPrintf(("qosclient: error we have had %d consecutive errors from the coordinator, going fatal.\n", pQosClient->uErrorMessageCount));
                    _QosClientModuleStateTransition(pQosClient, MODULE_STATE_FATAL, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_ERROR_UNRECOVERED_ERROR, 0);
                }
            }
        }
        else
        {
            NetPrintf(("qosclient: error http status %d\n", iHttpStatusCode));
            _QosClientModuleStateTransition(pQosClient, MODULE_STATE_FATAL, DIRTYAPI_PROTO_HTTP, iHttpStatusCode, 0);
        }
    }
    else if (iResult == PROTOHTTP2_RECVWAIT)
    {
        NetPrintfVerbose((pQosClient->iSpamValue, 1, "qosclient: [%p] Waiting for complete http response to arrive.\n", pQosClient));
    }
    else if (iResult == PROTOHTTP2_RECVBUFF)
    {
        uint32_t uNewBuffSize = 2 * pQosClient->uHttpRecvBuffSize;
        NetPrintfVerbose((pQosClient->iSpamValue, 1, "qosclient: [%p] Insufficient buffer space to receive, increasing space from %u to %u.\n", pQosClient, pQosClient->uHttpRecvBuffSize, uNewBuffSize));
        _QosClientAllocHttpRecvBuff(pQosClient, uNewBuffSize);
    }
    else if (iResult < 0)
    {
        int32_t hResult = ProtoHttp2Status(pQosClient->pProtoHttp, pQosClient->iHttpStreamId, 'hres', NULL, 0);
        if (hResult >= 0)  //in this case there was no socket or ssl error
        {
            NetPrintf(("qosclient: error ProtoHttpRecvAll failed iResult=%d\n", iResult));
            _QosClientModuleStateTransition(pQosClient, MODULE_STATE_FATAL, DIRTYAPI_PROTO_HTTP, iResult, 0);
        }
        else
        {
            int16_t iCode = 0;

            //we just want to get the error code from the hResult so we can include it in the hResult generated in _QosClientModuleStateTransition
            DirtyErrDecodeHResult(hResult, NULL, &iCode, NULL, NULL);

            NetPrintf(("qosclient: error ProtoHttpRecvAll failed hResult=%p\n", hResult));
            _QosClientModuleStateTransition(pQosClient, MODULE_STATE_FATAL, DIRTYAPI_PROTO_HTTP, iCode, 0);
        }
    }
}


/*F********************************************************************************/
/*!
    \Function _QosClientRequestReceiveUpdate

    \Description
        Check to see if we should send a probe or we need to re-send any probes.
        If so transition back into the send state.

    \Input *pQosClient  - module state
    \Input *pRequest    - request being processes

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static void _QosClientRequestReceiveUpdate(QosClientRefT *pQosClient, QosClientRequestT *pRequest)
{
    // don't bother sending anything more if we already have sent all we can, unless the responses are already on the wire this will likely timeout
    if (pRequest->rawResults.uProbeCountUp < QOS_COMMON_MAX_PROBE_COUNT)
    {
        uint32_t currentTime = NetTick();
        // there are two reasons why we might transition back to REQUEST_STATE_SEND
        // 1 the common case is that enough time has lapsed in between probes
        if ((pRequest->rawResults.uProbeCountUp < pRequest->uProbesToSend) &&      //we haven't sent all probes yet
            (NetTickDiff(currentTime, pRequest->uWhenLastSent) > (int32_t)pRequest->test.uMinTimeBetwenProbes) //too much time passed
            )
        {
            _QosClientRequestStateTransition(pQosClient, pRequest, REQUEST_STATE_SEND, DIRTYAPI_QOS, QOS_CLIENT_REQUEST_STATUS_SEND_NEXT_PROBE, 1);
        }
        // 2 too much time may have laps before receiving responses in which case we want to send more probes then initially planned (we likely lost probes on the wire)
        if ((pRequest->rawResults.uProbeCountDown < (pRequest->test.uProbeCountUp * pRequest->test.uProbeCountDown)) &&      //we haven't received the number of responses we want, and
            (NetTickDiff(currentTime, pRequest->uWhenLastSent) > (int32_t)pRequest->test.uTimeTillResend) //too much time passed
            )
        {
            int32_t iMissingProbes = ((pRequest->test.uProbeCountUp * pRequest->test.uProbeCountDown) - pRequest->rawResults.uProbeCountDown);
            if (iMissingProbes > pRequest->test.uAcceptableLostProbeCount)
            {
                int32_t iAdditionalProbes = (iMissingProbes / pRequest->test.uProbeCountDown); // we expect uProbeCountDown per uProbesToSend
                if (iAdditionalProbes <= 0)  //there needs to be at least one missing probe
                {
                    iAdditionalProbes = 1;
                }
                pRequest->uProbesToSend += (iAdditionalProbes + pRequest->test.uResendExtraProbeCount);  //the test may say to send even more probes if missing probes are found
                _QosClientRequestStateTransition(pQosClient, pRequest, REQUEST_STATE_SEND, DIRTYAPI_QOS, QOS_CLIENT_REQUEST_STATUS_SEND_LOST_PROBES, 0);
                NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: request[%02d] re-sending %d new probes.\n", pRequest->uClientRequestID, iAdditionalProbes));
            }
            else
            {
                _QosClientRequestStateTransition(pQosClient, pRequest, REQUEST_STATE_COMPLETE, DIRTYAPI_QOS, QOS_CLIENT_REQUEST_STATUS_COMPLETE_ACCEPTABLE, 0);
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _QosClientRequestComplete

    \Description
        Finish the request; call back the user, and delete the request.

    \Input *pQosClient  - module ref
    \Input *pRequest    - request being completed

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static void _QosClientRequestComplete(QosClientRefT *pQosClient, QosClientRequestT *pRequest)
{
    pRequest->uWhenCompleted = NetTick();
    pQosClient->rawResultStore.aResults[pQosClient->rawResultStore.uNumResults] = pRequest->rawResults;
    pQosClient->rawResultStore.uNumResults++;

    _QosClientDestroyRequest(pQosClient, pRequest->uClientRequestID);

    // see if there are currently no requests left, if so tell the coordinator about what happened
    if (pQosClient->pRequestQueue == NULL)
    {
        _QosClientModuleStateTransition(pQosClient, MODULE_STATE_INIT_COORDINATOR_COMM, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_RAW_RESULTS_READY, 0);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosClientCheckRequestForTimeout

    \Description
        Check to see if the request should be completed due to timeout, if so transition it to timeout.

    \Input *pQosClient  - module ref
    \Input *pRequest    - request being checked

    \Version 11/07/2014 (cvienneau)
*/
/********************************************************************************F*/
static void _QosClientCheckRequestForTimeout(QosClientRefT *pQosClient, QosClientRequestT *pRequest)
{
    //check to see if the request has timed out
    if (NetTickDiff(NetTick(), pRequest->uWhenStarted) > (int32_t)pRequest->test.uTimeout)
    {
        int16_t iErrorCode = QOS_CLIENT_REQUEST_STATUS_ERROR_TIMEOUT;
        if (pRequest->rawResults.uProbeCountDown > 0)
        {
            iErrorCode = QOS_CLIENT_REQUEST_STATUS_ERROR_TIMEOUT_PARTIAL;
        }
        if (pRequest->iLastProbeValidationError != 0)
        {
            iErrorCode = pRequest->iLastProbeValidationError;
        }
        NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: request[%02d] timed out (%d ms), possible cause %d.\n", pRequest->uClientRequestID, NetTickDiff(NetTick(), pRequest->uWhenStarted), iErrorCode));
        _QosClientRequestStateTransition(pQosClient, pRequest, REQUEST_STATE_ERROR, DIRTYAPI_QOS, iErrorCode, 0);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosClientReport

    \Description
        Display a summary report and pass the info on to the client

    \Input *pQosClient  - module ref

    \Version 09/15/2017 (cvienneau)
*/
/********************************************************************************F*/
static void _QosClientReport(QosClientRefT *pQosClient)
{
    //save the hResult that was generated while doing this request to pass to the user
    pQosClient->finalResults.hResult = pQosClient->rawResultStore.clientModifiers.hResult;
    #if DIRTYCODE_LOGGING
        uint32_t uSiteIndex;
        char addrBuff[128];

        uint32_t uProcessTime = NetTickDiff(NetTick(), pQosClient->uQosProcessStartTime);
        NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: CLIENT(hResult:0x%08x, external addr:%s, fw:%s, time:%ums)\n",
            pQosClient->finalResults.hResult,
            QosCommonAddrToString(&pQosClient->finalResults.clientExternalAddress, addrBuff, sizeof(addrBuff)),
            _strQosCommonFirewallType[pQosClient->finalResults.eFirewallType],
            uProcessTime));
        
        for (uSiteIndex = 0; uSiteIndex < pQosClient->finalResults.uNumResults; uSiteIndex++)
        {
            NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: SITE(%d, %8s) RESULT(hResult:0x%08x, upbps:%9u, downbps:%9u, rtt:%11u)\n",
                uSiteIndex,
                pQosClient->finalResults.aTestResults[uSiteIndex].strSiteName,
                pQosClient->finalResults.aTestResults[uSiteIndex].hResult,
                pQosClient->finalResults.aTestResults[uSiteIndex].uUpbps,
                pQosClient->finalResults.aTestResults[uSiteIndex].uDownbps,
                pQosClient->finalResults.aTestResults[uSiteIndex].uMinRTT
            ));
        }
    #endif
    
    pQosClient->uQosProcessStartTime = 0;
    if (pQosClient->pCallback)
    {
        pQosClient->pCallback(pQosClient, &pQosClient->finalResults, pQosClient->pUserData);
    }
    _QosClientModuleStateTransition(pQosClient, MODULE_STATE_IDLE, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_REPORT_COMPLETE, 0);
}

/*F********************************************************************************/
/*!
    \Function _QosClientUpdateRequest

    \Description
        Process the request for any given state.

    \Input *pQosClient  - module ref
    \Input *pRequest    - request being updated
    
    \Output 
        int8_t          - TRUE if the request has been destroyed.
        
    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static uint8_t _QosClientUpdateRequest(QosClientRefT *pQosClient, QosClientRequestT *pRequest)
{
    if (pRequest->eState == REQUEST_STATE_INIT)
    {
        _QosClientRequestInit(pQosClient, pRequest);
    }
    else if (pRequest->eState == REQUEST_STATE_INIT_SYNC)
    {
        _QosClientRequestWaitForInit(pQosClient, pRequest);
    }
    else if (pRequest->eState == REQUEST_STATE_SEND)
    {
        _QosClientRequestSendProbes(pQosClient, pRequest);
    }
    else if (pRequest->eState == REQUEST_STATE_RECEIVE)
    {
        _QosClientRequestReceiveUpdate(pQosClient, pRequest);
    }
    else if (pRequest->eState == REQUEST_STATE_COMPLETE)
    {
        _QosClientRequestComplete(pQosClient, pRequest);
        return(TRUE);
    }
    else
    {
        NetPrintf(("qosclient: request[%02d] error request in unexpected state.\n", pRequest->uClientRequestID));
    }
    
    _QosClientCheckRequestForTimeout(pQosClient, pRequest);

    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function _QosClientCheckUpdateTimings

    \Description
        See if we are being pumped frequently enough, if we aren't it can adversely
        effect the timeouts.

    \Input *pQosClient  - module ref

    \Version 11/28/2017 (cvienneau)
*/
/********************************************************************************F*/
static void _QosClientCheckUpdateTimings(QosClientRefT *pQosClient)
{
    uint32_t uCurrentTime = NetTick();
    
    //check to see if we are being updated at a reasonable rate
    uint32_t uUpateElapsedTime = NetTickDiff(uCurrentTime, pQosClient->uLastUpdateTime);
    if (uUpateElapsedTime >= pQosClient->uStallWindow)
    {
        if ((pQosClient->eModuleState == MODULE_STATE_INIT_COORDINATOR_COMM) || (pQosClient->eModuleState == MODULE_STATE_UPDATE_COORDINATOR_COMM))
        {
            pQosClient->rawResultStore.clientModifiers.uStallCountCoordinator++;
            pQosClient->rawResultStore.clientModifiers.uStallDurationCoordinator += uUpateElapsedTime;

        }
        else if (pQosClient->eModuleState == MODULE_STATE_PROBE)
        {
            pQosClient->rawResultStore.clientModifiers.uStallCountProbe++;
            pQosClient->rawResultStore.clientModifiers.uStallDurationProbe += uUpateElapsedTime;
        }
        NetPrintfVerbose((pQosClient->iSpamValue, 1, "qosclient: an update stall of %dms was detected.\n", uUpateElapsedTime));
    }
    pQosClient->uLastUpdateTime = uCurrentTime;

    //check for overall timeout
    if (pQosClient->uQosProcessStartTime != 0)
    {
        if (NetTickDiff(uCurrentTime, pQosClient->uQosProcessStartTime) > (int32_t)pQosClient->uMaxQosProcessTime)
        {
            NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: the qos process has timed out after %ums.\n", pQosClient->uMaxQosProcessTime));
            _QosClientModuleStateTransition(pQosClient, MODULE_STATE_FATAL, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_ERROR_TIMEOUT, 0);
        }
    }
}


/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function QosClientUpdate

    \Description
        Allow the module to do processing.

    \Input *pQosClient - point to module state

    \Version 06/21/2017 (cvienneau)
*/
/********************************************************************************F*/
void QosClientUpdate(QosClientRefT *pQosClient)
{
    QosClientRequestT *pRequest;

    if (!NetCritTry(&pQosClient->ThreadCrit))
    {
        return;
    }

    _QosClientCheckUpdateTimings(pQosClient);

    if (pQosClient->eModuleState == MODULE_STATE_IDLE)
    {
        _QosClientAutoStart(pQosClient);
    }
    else if (pQosClient->eModuleState == MODULE_STATE_INIT_COORDINATOR_COMM)
    {
        //tell the coordinator where we are at, this could be just who we are, or may include the results we have so far
        _QosClientMessageCoordinator(pQosClient);
    }
    else if (pQosClient->eModuleState == MODULE_STATE_UPDATE_COORDINATOR_COMM)
    {
        // if there is any pending http action process it now
        _QosClientUpdateHttp(pQosClient);
    }
    else if (pQosClient->eModuleState == MODULE_STATE_PROBE)
    {
        // receive data for any incoming probes
        // note that probes may also be received by the socket receive thread
        _QosClientUpdateUdpProbes(pQosClient);

        //update the state of all the request
        for (pRequest = pQosClient->pRequestQueue; pRequest != NULL; pRequest = pRequest->pNext)
        {
            // update based off recent communication udp or http
            if(_QosClientUpdateRequest(pQosClient, pRequest) == TRUE) 
            {
                break;//on TRUE a request has been destroyed the list is no longer valid
            }
        }    
    }
    else if (pQosClient->eModuleState == MODULE_STATE_REPORT)
    {
        _QosClientReport(pQosClient);
    }
    else
    {
        NetPrintf(("qosclient: error unknown eModuleState state.\n"));
        _QosClientModuleStateTransition(pQosClient, MODULE_STATE_ERROR, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_ERROR_UNKOWN_STATE, 0);
    }

    NetCritLeave(&pQosClient->ThreadCrit);
}

/*F********************************************************************************/
/*!
    \Function QosClientCreate

    \Description
        Create the QosClient module.

    \Input *pCallback   - callback function to use when a status change is detected
    \Input *pUserData   - use data to be set for the callback
    \Input uListenPort  - set the port incoming probes will go to, pass 0 to use defaults

    \Output 
        QosClientRefT * - state pointer on success, NULL on failure

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
QosClientRefT *QosClientCreate(QosClientCallbackT *pCallback, void *pUserData, uint16_t uListenPort)
{
    QosClientRefT *pQosClient;
    void *pMemGroupUserData;
    int32_t iMemGroup;

    // Query current memory group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    DirtyMemGroupEnter(iMemGroup, pMemGroupUserData);

    // allocate and init module state
    if ((pQosClient = (QosClientRefT*)DirtyMemAlloc(sizeof(*pQosClient), QOS_CLIENT_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        DirtyMemGroupLeave();
        NetPrintf(("qosclient: could not allocate module state\n"));
        return(NULL);
    }

    //init member variables
    ds_memclr(pQosClient, sizeof(*pQosClient));
    pQosClient->iMemGroup = iMemGroup;
    pQosClient->pMemGroupUserData = pMemGroupUserData;
    pQosClient->iSpamValue = 1;
    pQosClient->uUdpRequestedListenPort = uListenPort;
    pQosClient->iHttpSoLinger = -1;
    pQosClient->bUseHttps = QOS_CLIENT_DEFAULT_USE_HTTPS;
    pQosClient->pCallback = pCallback;
    pQosClient->pUserData = pUserData;
    pQosClient->uUdpReceiveBuffSize = QOS_CLIENT_DEFAULT_RECEIVE_BUFF_SIZE;
    pQosClient->uUdpSendBuffSize = QOS_CLIENT_DEFAULT_SEND_BUFF_SIZE;
    pQosClient->uUdpPacketQueueSize = QOS_CLIENT_DEFAULT_PACKET_QUEUE_SIZE;
    pQosClient->uStallWindow = QOS_CLIENT_DEFAULT_STALL_WINDOW;
    pQosClient->uMaxQosProcessTime = QOS_CLIENT_DEFAULT_MAX_QOS_PROCESS_TIME;
    pQosClient->uLastUpdateTime = NetTick();
    //transition to starting state
    _QosClientModuleStateTransition(pQosClient, MODULE_STATE_IDLE, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_INIT, 0);

    //setup protoHttp for communication with the coordinator
    if ((pQosClient->pProtoHttp = ProtoHttp2Create(4096)) == NULL)  //we shouldn't need much space
    {
        DirtyMemGroupLeave();
        NetPrintf(("qosclient: could not create the protohttp module.\n"));
        QosClientDestroy(pQosClient);
        return(NULL);
    }
    DirtyMemGroupLeave();
    ProtoHttp2Control(pQosClient->pProtoHttp, pQosClient->iHttpStreamId, 'apnd', 0, 0, NULL);
    ProtoHttp2Control(pQosClient->pProtoHttp, pQosClient->iHttpStreamId, 'apnd', 0, 0, (void *)"te: trailers\r\ncontent-type: application/grpc\r\n");

    // need to sync access to data
    NetCritInit(&pQosClient->ThreadCrit, "qosclient");

    // return module state to caller
    return(pQosClient);
}

/*F********************************************************************************/
/*!
    \Function QosClientStart

    \Description
        Request that the QOS coordinator provide setting to the client, the client will
        then perform tests based off those settings.  Results will be provided to game code
        via the callback set in QosClientCreate.

    \Input *pQosClient          - module ref
    \Input *pStrCoordinatorAddr - url of the QOS coordinator
    \Input uPort                - port of the QOS coordinator
    \Input *pStrQosProfile      - what profile should the QOS coordinator use for settings, likely a service name like "fifa-pc-20147"

    \Version 11/07/2014 (cvienneau)
*/
/********************************************************************************F*/
void QosClientStart(QosClientRefT *pQosClient, const char *pStrCoordinatorAddr, uint16_t uPort, const char *pStrQosProfile)
{
    NetCritEnter(&pQosClient->ThreadCrit);

    //only start if we haven't already got something on the go
    if (pQosClient->eModuleState == MODULE_STATE_IDLE)
    {
        //clear out any existing state
        ds_memclr(&pQosClient->finalResults, sizeof(pQosClient->finalResults));
        ds_memclr(&pQosClient->rawResultStore, sizeof(pQosClient->rawResultStore));
        pQosClient->uHttpBytesToSend = 0;
        pQosClient->uErrorMessageCount = 0;
        pQosClient->uQosProcessStartTime = NetTick();

        //set info about the client
        ds_snzprintf(pQosClient->rawResultStore.strQosProfile, sizeof(pQosClient->rawResultStore.strQosProfile), pStrQosProfile);
        pQosClient->rawResultStore.uProbeVersion = QOS_COMMON_PROBE_VERSION;
        pQosClient->rawResultStore.clientModifiers.clientAddressInternal = pQosClient->localUdpAddrInfo;
        ds_snzprintf(pQosClient->rawResultStore.clientModifiers.strPlatform, sizeof(pQosClient->rawResultStore.clientModifiers.strPlatform), DIRTYCODE_PLATNAME_SHORT);

        //set info about the coordinator
        ds_snzprintf(pQosClient->strCoordinatorAddr, sizeof(pQosClient->strCoordinatorAddr), pStrCoordinatorAddr);
        pQosClient->uCoordinatorPort = uPort;

        //make use of any control selectors that were set before we start
        ProtoHttp2Control(pQosClient->pProtoHttp, pQosClient->iHttpStreamId, 'spam', pQosClient->iSpamValue + 1, 0, NULL);
        ProtoHttp2Control(pQosClient->pProtoHttp, pQosClient->iHttpStreamId, 'ncrt', pQosClient->bIgnoreSSLErrors, 0, NULL);

        //start communication with the coordinator
        _QosClientModuleStateTransition(pQosClient, MODULE_STATE_INIT_COORDINATOR_COMM, DIRTYAPI_QOS, QOS_CLIENT_MODULE_STATUS_PROCESS_STARTED, 0);
    }
    else
    {
        NetPrintf(("qosclient: QosClientStart() skipped because the QosClient is already busy.\n"));
    }
    NetCritLeave(&pQosClient->ThreadCrit);
}

/*F********************************************************************************/
/*!
    \Function QosClientControl

    \Description
        QosClient control function.  Different selectors control different behaviors.

    \Input *pQosClient - module state
    \Input iControl - control selector
    \Input iValue   - selector specific data
    \Input pValue   - selector specific data

    \Output 
        int32_t - control specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'cbfp' - set callback function pointer - pValue=pCallback
            'lprt' - set the port to use for the QoS listen port (must be set before calling listen/request), same as the value passed to QosClientCreate
            'maxt' - set the amount of ms before we give up on the current qos process
            'ncrt' - passed to ProtoHttpControl, if TRUE will proceed even if ssl cert errors are detected (defaults FALSE)
            'pque' - passed to SocketControl 'pque' if not 0, otherwise the number of packets will be passed to SocketControl 'pque'
            'rbuf' - passed to SocketControl 'rbuf'
            'sbuf' - passed to SocketControl 'sbuf'
            'soli' - passed down to the socket as a SO_LINGER option if its >= 0, a 0 value will abort the socket on disconnection with no TIME_WAIT state
            'spam' - set the verbosity of the module, default 1 (0 errors, 1 debug info, 2 extended debug info, 3 per probe info)
            'stwi' - set the amount of ms before a stall is counted
            'ussl' - if TRUE use https else http for setup communication (defaults TRUE)

        \endverbatim

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
int32_t QosClientControl(QosClientRefT *pQosClient, int32_t iControl, int32_t iValue, void *pValue)
{
    int32_t iRet = 0;
    NetPrintfVerbose((pQosClient->iSpamValue, 0, "qosclient: QosClientControl('%C'), (%d)\n", iControl, iValue));
    
    NetCritEnter(&pQosClient->ThreadCrit);

    if (iControl == 'cbfp')
    {
        // set callback function pointer
        pQosClient->pCallback = (QosClientCallbackT *)pValue;
    }
    else if (iControl == 'lprt')
    {
        pQosClient->uUdpRequestedListenPort = iValue;
    }
    else if (iControl == 'maxt')
    {
        pQosClient->uMaxQosProcessTime = iValue;
    }
    else if (iControl == 'ncrt')
    {
        pQosClient->bIgnoreSSLErrors = iValue;
    }    
    else if (iControl == 'pque')
    {
        pQosClient->uUdpPacketQueueSize = iValue;
    }  
    else if (iControl == 'rbuf')
    {
        pQosClient->uUdpReceiveBuffSize = iValue;
    }
    else if (iControl == 'sbuf')
    {
        pQosClient->uUdpSendBuffSize = iValue;
    }
    else if (iControl == 'soli')
    {
        pQosClient->iHttpSoLinger = iValue;
    }
    else if (iControl == 'spam')
    {
        pQosClient->iSpamValue = iValue;
    }
    else if (iControl == 'stwi')
    {
        pQosClient->uStallWindow = iValue;
    }
    else if (iControl == 'ussl')
    {
        pQosClient->bUseHttps = iValue;
    }
    else
    {
        iRet = -1;
    }

    NetCritLeave(&pQosClient->ThreadCrit);
    return(iRet);
}

/*F********************************************************************************/
/*!
    \Function QosClientStatus

    \Description
        Return quality of service information.

    \Input *pQosClient - module state
    \Input iSelect  - output selector
    \Input iData    - selector specific
    \Input *pBuf    - [out] pointer to output buffer
    \Input iBufSize - size of output buffer

    \Output 
        int32_t - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'clpt' - get the current listen port of the QoS socket
            'hres' - get the current hResult value for the module
            'lprt' - get the port to use for the QoS listen port
            'maxt' - get the maximum time we will allow the qos process to run
            'ncrt' - if TRUE we will ignore ssl errors for setup communication
            'pque' - get the value passed to SocketControl 'pque'
            'rbuf' - get the value passed to SocketControl 'rbuf'
            'rpro' - get the processed results, the same results that are returned in the callback, pass in a QosCommonProcessedResultsT
            'rraw' - get the raw results, the results that are analyzed by the coordinator, pass in a QosCommonClientToCoordinatorRequestT
            'rrpc' - get the result rpc data, the processed results from the coordinator, only valid during the completion callback. returns length of data, and populates pBuf if a buffer of enough space is provided.
            'sbuf' - get the value passed to SocketControl 'sbuf'
            'soli' - get the value passed to ProtoHttp 'soli'
            'spam' - get the verbosity of the module
            'stwi' - get the duration of time to pass, to consider a updating to have been stalled
            'ussl' - if TRUE use https else http for setup communication

        \endverbatim

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
int32_t QosClientStatus(QosClientRefT *pQosClient, int32_t iSelect, int32_t iData, void *pBuf, int32_t iBufSize)
{
    int32_t iRet = 0;
    NetPrintfVerbose((pQosClient->iSpamValue, 1, "qosclient: QosClientStatus('%C'), (%d)\n", iSelect, iData));

    NetCritEnter(&pQosClient->ThreadCrit);

    if (iSelect == 'clpt')
    {
        iRet = pQosClient->localUdpAddrInfo.uPort;
    }
    if (iSelect == 'hres')
    {
        iRet = pQosClient->rawResultStore.clientModifiers.hResult;
    }
    else if (iSelect == 'lprt')
    {
        iRet = pQosClient->uUdpRequestedListenPort;
    }
    else if (iSelect == 'maxt')
    {
        iRet = pQosClient->uMaxQosProcessTime;
    }
    else if (iSelect == 'ncrt')
    {
        iRet = pQosClient->bIgnoreSSLErrors;
    }
    else if (iSelect == 'pque')
    {
        iRet = pQosClient->uUdpPacketQueueSize;
    }  
    else if (iSelect == 'rbuf')
    {
        iRet = pQosClient->uUdpReceiveBuffSize;
    }   
    else if ((iSelect == 'rpro') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(QosCommonProcessedResultsT)))
    {
        ds_memcpy(pBuf, &pQosClient->finalResults, sizeof(QosCommonProcessedResultsT));
        iRet = 1;
    }     
    else if ((iSelect == 'rraw') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(QosCommonClientToCoordinatorRequestT)))
    {
        ds_memcpy(pBuf, &pQosClient->rawResultStore, sizeof(QosCommonClientToCoordinatorRequestT));
        iRet = 1;
    }         
    else if (iSelect == 'rrpc')
    {
        iRet = 0;
        if (pQosClient->pHttpRecvBuff)
        {
            iRet = (int32_t)pQosClient->uCurrentHttpRecvSize;
            if ((pBuf != NULL) && (iBufSize >= iRet))
            {
                ds_memcpy(pBuf, pQosClient->pHttpRecvBuff, iRet);
            }
        }
    }
    else if (iSelect == 'sbuf')
    {
        iRet = pQosClient->uUdpSendBuffSize;
    }
    else if (iSelect == 'soli')
    {
        iRet = pQosClient->iHttpSoLinger;
    }
    else if (iSelect == 'spam')
    {
        iRet = pQosClient->iSpamValue;
    }
    else if (iSelect == 'stwi')
    {
        iRet = pQosClient->uStallWindow;
    }
    else if (iSelect == 'ussl')
    {
        iRet = pQosClient->bUseHttps;
    }
    else
    {
        iRet = -1;
    }

    NetCritLeave(&pQosClient->ThreadCrit);
    return(iRet);
}

/*F********************************************************************************/
/*!
    \Function QosClientDestroy

    \Description
        Destroy the QosClient module.

    \Input *pQosClient - module state

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
void QosClientDestroy(QosClientRefT *pQosClient)
{
    NetCritEnter(&pQosClient->ThreadCrit);

    if (pQosClient->pUdpSocket != NULL)
    {
        SocketClose(pQosClient->pUdpSocket);
        pQosClient->pUdpSocket = NULL;
    }

    if (pQosClient->pProtoHttp != NULL)
    {
        // close the socket asap when destroyed (necessary for stress testing to not leak resources too long)
        if (pQosClient->iHttpSoLinger >= 0)
        {
            ProtoHttp2Control(pQosClient->pProtoHttp, pQosClient->iHttpStreamId, 'soli', pQosClient->iHttpSoLinger, 0, NULL);
        }
        ProtoHttp2Destroy(pQosClient->pProtoHttp);
        pQosClient->pProtoHttp = NULL;

        _QosClientAllocHttpRecvBuff(pQosClient, 0);
        _QosClientAllocHttpSendBuff(pQosClient, 0);
    }

    while (pQosClient->pRequestQueue != NULL)
    {
        _QosClientDestroyRequest(pQosClient, pQosClient->pRequestQueue->uClientRequestID);
    }

    NetCritLeave(&pQosClient->ThreadCrit);
    NetCritKill(&pQosClient->ThreadCrit);

    // release module memory
    DirtyMemFree(pQosClient, QOS_CLIENT_MEMID, pQosClient->iMemGroup, pQosClient->pMemGroupUserData);
}

