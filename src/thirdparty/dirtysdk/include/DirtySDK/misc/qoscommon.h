/*H********************************************************************************/
/*!
    \File qoscommon.h

    \Description
        This code declares shared items between client and server of the QOS system.

    \Copyright
        Copyright (c) 2017 Electronic Arts Inc.

*/
/********************************************************************************H*/

#ifndef _qoscommon_h
#define _qoscommon_h

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtyaddr.h"
#include "DirtySDK/dirtysock/dirtynet.h"

/*** Defines **********************************************************************/
#define QOS_COMMON_HMAC_TYPE                    (CRYPTHASH_SHA256)              //!< a secure strong hashing algorithm
#define QOS_COMMON_HMAC_SIZE                    (32)                            //!< CRYPTSHA256_HASHSIZE
#define QOS_COMMON_SECURE_KEY_LENGTH            (16)                            //!< length of random data as key, in generation of hmac
#define QOS_COMMON_SIZEOF_PROBE_DATA            (45)                            //!< total byte count of probe, excluding hmac
#define QOS_COMMON_PROBE_PROTOCOL_ID            ('qos2')                        //!< probe packets will all start with this to identify them as being the QOS 2 protocol
#define QOS_COMMON_PROBE_VERSION_MAJOR          (2)                             //!< identifies changes to QosCommonProbePacketT, 2 bytes, first byte indicates api breaking
#define QOS_COMMON_PROBE_VERSION_MINOR          (0)                             //!< second byte compatible bug fixes.
#define QOS_COMMON_PROBE_VERSION                ((QOS_COMMON_PROBE_VERSION_MAJOR << 8) | QOS_COMMON_PROBE_VERSION_MINOR) //!< the combined major, minor version bytes.
#define QOS_COMMON_MIN_PACKET_SIZE (QOS_COMMON_SIZEOF_PROBE_DATA + QOS_COMMON_HMAC_SIZE) //!< size of on wire representation of the probe packet
#define QOS_COMMON_MAX_PACKET_SIZE              (SOCKET_MAXUDPRECV)             //!< maximum probe/packet length
#define QOS_COMMON_MAX_PROBE_COUNT              (32)                            //!< maximum probes per request
#define QOS_COMMON_MAX_SITES                    (32)                            //!< maximum sites we can test against at one time
#define QOS_COMMON_MAX_CONTROL_CONFIGS          (6)                             //!< maximum number of QosClientControl selectors we can include in the request
#define QOS_COMMON_MAX_TESTS                    (4)                             //!< latency, bandwidthUP, bandwidthDown; and extra room for something we haven't considered
#define QOS_COMMON_MAX_RESULTS  (QOS_COMMON_MAX_SITES * QOS_COMMON_MAX_TESTS)   //!< highest number of possible results we can generate
#define QOS_COMMON_MAX_URL_LENGTH               (256)                           //!< size used for URL strings
#define QOS_COMMON_MAX_RPC_STRING               (128)                           //!< size used for typical strings
#define QOS_COMMON_MAX_RPC_STRING_SHORT         (16)                            //!< size used for strings we expect to be very short
#define QOS_COMMON_MAX_RPC_BODY_SIZE            (1024 * 200)                    //!< rpc messages should never exceed this size, raw results are our biggest message
#define QOS_COMMON_DEFAULT_RPC_BODY_SIZE (QOS_COMMON_MAX_RPC_BODY_SIZE / 10)    //!< An amount of space that will usually be enough to receive an rpc message into
#define QOS_COMMON_CONTENT_TYPE                 ("application/grpc")        //!< http content type, everything is protobuff
#define QOS_COMMON_SERVER_URL                   ("/eadp.qoscoordinator.QOSCoordinatorRegistration/registerServer")  //!< used by server to register to the QOS server, and heartbeat to refresh its availability
#define QOS_COMMON_CLIENT_URL                   ("/eadp.qoscoordinator.QOSCoordinator/ClientCall")  //!< used by client to request config and push back final results


// flags contained in probe packets
#define QOS_COMMON_PACKET_FLAG_LAST_PROBE       (1)     //!< set if the client expects this is the last packet in this test

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! just enough space to hold a address family, port, and an v4 or v6 address.  Used for storage or communicating address on the wire, never used to actually send/recv.
typedef struct QosCommonAddrT
{
    union
    {
        union
        {
            uint8_t aBytes[16];
            uint16_t aWords[8];
            uint32_t aDwords[4];
        }v6;        
        uint32_t v4;
    }addr;
    uint16_t  uFamily;
    uint16_t  uPort;
} QosCommonAddrT;

//! describes the configuration for a particular QOS test
typedef struct QosCommonTestT
{
    char strTestName[QOS_COMMON_MAX_RPC_STRING_SHORT];   //!< name of the test
    char strSiteName[QOS_COMMON_MAX_RPC_STRING_SHORT];   //!< alias of target QOS server
    uint16_t uProbeSizeUp;              //!< minimum size of the probe, the probe will be padded with extra data to reach this size
    uint16_t uProbeSizeDown;            //!< minimum size of the probe, the probe will be padded with extra data to reach this size
    uint16_t uTimeout;                  //!< maximum amount of time before giving up on completing the test
    uint16_t uMinTimeBetwenProbes;      //!< minimum amount of time between each probe sent from the client
    uint16_t uTimeTillResend;           //!< if we haven't got the expected number of responses back in this much time, we will send more probes
    uint16_t uInitSyncTimeout;          //!< the amount of time we might wait to synchronize all requests together
    uint8_t uProbeCountUp;              //!< number of probes to send to the server
    uint8_t uProbeCountDown;            //!< number of probes there server should respond with for each probe received
    uint8_t uResendExtraProbeCount;     //!< in the case of finding packet loss, we request the lost packets again with a few extras, to reduce the chance that those are lost too
    uint8_t uAcceptableLostProbeCount;  //!< typically we would receive (uProbeCountUp * uProbeCountDown) probes, this indicates how many less probes we can receive without triggering a re-send
} QosCommonTestT;

//! describes QosClientControl overrides that should be executed before doing any QosTests
typedef struct QosCommonControlConfigT
{
    char strValue[QOS_COMMON_MAX_RPC_STRING];   //!< string value, such as QosClientControl(mQosClient, 'sprt', 0, strValue);
    char strControl[5];                         //!< dirty sdks four character code identifier, usually in the form of 'wxzy'
    int32_t iValue;                             //!< integer value, such as QosClientControl(mQosClient, 'lprt', iValue, NULL);
} QosCommonControlConfigT;

//! describes a site, QOS Tests are performed against a QOS/ping Site
typedef struct QosCommonSiteT
{
    char strSiteName[QOS_COMMON_MAX_RPC_STRING_SHORT];  //!< identifies where a site is, such as bio-sjc
    char strProbeAddr[QOS_COMMON_MAX_RPC_STRING];       //!< url to communicate with the server
    uint8_t aSecureKey[QOS_COMMON_SECURE_KEY_LENGTH];   //!< key used in generating hmac
    uint16_t uProbePort;                                //!< port to use when communicating
    uint16_t uProbeVersion;                             //!< the version the site is operating with
} QosCommonSiteT;

//! a collection of sites, control configs and tests to describe what actions the client should be taking
typedef struct QosCommonClientConfigT
{
    QosCommonSiteT aSites[QOS_COMMON_MAX_SITES];    //!< list of sites that are potential targets
    QosCommonControlConfigT aControlConfigs[QOS_COMMON_MAX_CONTROL_CONFIGS]; //!< list of control settings to be run before doing tests
    QosCommonTestT aQosTests[QOS_COMMON_MAX_TESTS]; //!< list tests to be done
    QosCommonAddrT clientAddressFromCoordinator;    //!< the client must report its external address to the QOS server, the coordinator will initialize this to what it sees the clients address as
    uint8_t uNumSites;                              //!< number of QosCommonSiteT contained in aSites
    uint8_t uNumControlConfigs;                     //!< number of QosCommonControlConfigT contained in aControlConfigs
    uint8_t uNumQosTests;                           //!< number of QosCommonTestT contained aQosTests
} QosCommonClientConfigT;

//!< a firewall or NAT (network address translation) indicates how difficult it is to make a connection with this location 
typedef enum QosCommonFirewallTypeE
{
    QOS_COMMON_FIREWALL_UNKNOWN,
    QOS_COMMON_FIREWALL_OPEN,
    QOS_COMMON_FIREWALL_MODERATE,
    QOS_COMMON_FIREWALL_STRICT,
    QOS_COMMON_FIREWALL_NUMNATTYPES          //!< max number of NAT types, must be at the end
} QosCommonFirewallTypeE;

//! contains latency and bandwidth results for a given site
typedef struct QosCommonTestResultT 
{
    char strSiteName[QOS_COMMON_MAX_RPC_STRING_SHORT];
    uint32_t uMinRTT;
    uint32_t uUpbps;
    uint32_t uDownbps;
    uint32_t hResult;
} QosCommonTestResultT;

//! the coordinator will examine a QosCommonRawResultsT and produce usable QosCommonProcessedResultsT
typedef struct QosCommonProcessedResultsT
{
    QosCommonTestResultT aTestResults[QOS_COMMON_MAX_SITES];    //!< an ordered list (by "best" site, coordinator's decision) of all test results
    QosCommonAddrT clientExternalAddress;                       //!< this is the address the coordinator recommends using for communication to this client
    QosCommonFirewallTypeE eFirewallType;                       //!< this is what the coordinator classifies the firewall type to be
    uint32_t uNumResults;                                       //!< number of results found in aTestResults
    uint32_t uTimeTillRetry;                                    //!< if this is != 0, the QOS server wants us to check back in this many ms for a new test
    uint32_t hResult;                                           //!< holds the error/status code of the request
} QosCommonProcessedResultsT;

//! the coordinator responds with configuration or results
typedef struct QosCommonCoordinatorToClientResponseT
{
    QosCommonClientConfigT configuration;       //!< what does the coordinator want the client to do
    QosCommonProcessedResultsT results;         //!< contains the processed data from the coordinator, this will be provided if there coordinator doesn't have more tests for the client to do
    uint32_t uServiceRequestID;                 //!< number uniquely identifies this transaction taking place between client and coordinator
} QosCommonCoordinatorToClientResponseT;

//! sent to and from the QOS server as a udp probe, note QosCommonSerializeProbePacket and QosCommonDeserializeProbePacket determines on wire packing and order
typedef struct QosCommonProbePacketT 
{
    QosCommonAddrT clientAddressFromService;  //!< initially the client address from coordinator, however address from server prospective takes precedence, used to authenticate the packet is coming from the address that generated it
    uint32_t uProtocol;                 //!< QOS 2.0 packets will always contain 'qos2' for easy identification.
    uint32_t uServiceRequestId;         //!< provided by the QosCoordinator, unique to the client doing multiple QOS actions, used to identify which server resources this client is using
    uint32_t uServerReceiveTime;        //!< time the server received this probe from the client
    uint16_t uServerSendDelta;          //!< duration the server held onto the packet before sending the response probe, this is latency added by the server process
    uint16_t uVersion;                  //!< uniquely identifies protocol
    uint16_t uProbeSizeUp;              //!< indicates how big this probe is, including any padding for bandwidth
    uint16_t uProbeSizeDown;            //!< indicates how big this probe is, including any padding for bandwidth
    uint16_t uClientRequestId;          //!< provided by the client, unique to a particular QOS action, used to pair request and responses together
    uint8_t uProbeCountUp;              //!< count index of this probe
    uint8_t uProbeCountDown;            //!< from client (number of probes there server should respond with for each probe received) from server (count index of this probe)
    uint8_t uExpectedProbeCountUp;      //!< number of probes the client expects to send to the sever in this test (assuming no packet loss)
} QosCommonProbePacketT;

//! a single QOS probe result information
typedef struct QosCommonProbeResultT 
{
    uint32_t uClientSendTime;           //!< time the client initially sent this QOS probe
    uint32_t uServerReceiveTime;        //!< time the server received this probe from the client (note that server and client time are not in sync)
    uint16_t uServerSendDelta;          //!< duration the server held onto the packet before sending the response probe, this is latency added by the server process
    uint16_t uClientReceiveDelta;       //!< duration since sending before the client received response from the QOS server for this probe
} QosCommonProbeResultT;

//!< after the client performs the tests, raw results are provided to the coordinator for analysis
//! Its expected each QosCommonRawResultsT can be used to generate latency values, bandwidth values, with two QosCommonRawResultsT firewall values can be calculated
typedef struct QosCommonRawResultsT 
{
    QosCommonProbeResultT aProbeResult[QOS_COMMON_MAX_PROBE_COUNT]; //!< a list of individual probe timings
    QosCommonAddrT clientAddressFromServer; //!< clients address from prospective of the QOS server  
    char strSiteName[QOS_COMMON_MAX_RPC_STRING_SHORT]; //!< identifies where a site is, such as bio-sjc 
    char strTestName[QOS_COMMON_MAX_RPC_STRING_SHORT]; //!< identifies which test, such as latency, bandwidthUp, bandwidthDown
    uint32_t hResult;                       //!< holds the error/status code of the request
    uint8_t uProbeCountUp;                  //!< number of probes the request sent when all retrying and everything is done (usually same as uProbeCountRequestedUp if no probes were lost, otherwise we send more)
    uint8_t uProbeCountDown;                //!< number of valid probes from this request that came back from the server (usually same as uProbeCountRequestedDown at the end unless an error or probes lost)
    uint8_t uProbeCountHighWater;           //!< the highest count of probe we receive back
                                            //note, uProbeCountHighWater, uProbeCountDown, and uProbeCountUp aren't shared with the coordinator
} QosCommonRawResultsT ;

//! data the client will tell the coordinator about itself (aside from test results) to help it make its decisions
typedef struct QosCommonClientModifiersT
{
    QosCommonAddrT clientAddressInternal;                   //!< address the client believes it is using
    char strPlatform[QOS_COMMON_MAX_RPC_STRING_SHORT];      //!< a string describing which platform the client is
    QosCommonFirewallTypeE eOSFirewallType;                 //!< the firewall type that the client OS suggests it might be
    uint32_t uPacketQueueRemaining;                         //!< number of spots left in the packet queue after the client has done its tests, a 0 value indicates a problem
    uint32_t uStallCountCoordinator;                        //!< number of stall events encountered when attempting to communicate with the coordinator
    uint32_t uStallDurationCoordinator;                     //!< total duration of time spent stalled while communicating with the coordinator
    uint32_t uStallCountProbe;                              //!< number of stall events encountered when attempting to send probes to the qos server
    uint32_t uStallDurationProbe;                           //!< total duration of time spent stalled while communicating with the qos server
    uint32_t hResult;                                       //!< current error status of the QosClient module, can be used to report errors to the coordinator
} QosCommonClientModifiersT;

//! the client informs the coordinator of who it is and any results it has gathered in order to get configuration or processed "final" results
typedef struct QosCommonClientToCoordinatorRequestT
{
    QosCommonRawResultsT aResults[QOS_COMMON_MAX_RESULTS];  //!< a set of raw results for each site, for each test performed
    char strQosProfile[QOS_COMMON_MAX_RPC_STRING];          //!< a string describing which setting the coordinator should use for this title. (likely service name, like fifa-pc-2017)    
    uint32_t uServiceRequestID;                             //!< number uniquely identifies this transaction taking place between client and coordinator
    uint32_t uNumResults;                                   //!< number results in aResults, if the client has results to report
    uint16_t uProbeVersion;                                 //!< uniquely identifies protocol
    QosCommonClientModifiersT clientModifiers;              //!< a set of fields the client wants to share with the coordinator such as health information to use in decision process
} QosCommonClientToCoordinatorRequestT;

//! the QOS server will request to be registered with the coordinator
typedef struct QosCommonServerToCoordinatorRequestT
{
    char strAddr[QOS_COMMON_MAX_RPC_STRING];            //!< url to communicate with the server
    char strSiteName[QOS_COMMON_MAX_RPC_STRING_SHORT];  //!< location where this server resides ie bio-sjc
    char strPool[QOS_COMMON_MAX_RPC_STRING];            //!< segregate servers at a given location, such as only fifa is using this server or only for special qa tests
    uint8_t aSecureKey[QOS_COMMON_SECURE_KEY_LENGTH];   //!< key used to generate hmac
    uint16_t uPort;                             //!< port to use when communicating
    uint16_t uCapacityPerSec;                   //!< maximum number of clients the coordinator should send to the server per second
    uint16_t uLastLoadPerSec;                   //!< number of qos tests started last second
    uint16_t uProbeVersion;                     //!< the probe packet version this server will operate with
    uint16_t uUpdateInterval;                   //!< amount of time in ms that the coordinator should wait before expecting the next heartbeat
    uint8_t bShuttingDown;                      //!< indicates if this server has begun to shutdown, if so no more clients should be sent to this server
} QosCommonServerToCoordinatorRequestT;

//! registration message is for debug purpose only
typedef struct QosCommonCoordinatorToServerResponseT
{
    char strRegistrationMessage[QOS_COMMON_MAX_RPC_STRING]; //!< message provided by the coordinator for this server, likely explaining any error status
    uint32_t uMinServiceRequestID;                          //!< don't accept service request id's older than this
} QosCommonCoordinatorToServerResponseT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// QosCommonClientToCoordinatorRequestT helpers
DIRTYCODE_API uint8_t* QosCommonClientToCoordinatorRequestEncode(const QosCommonClientToCoordinatorRequestT *pClientToCoordinatorRequest, uint8_t *pBuffer, uint32_t uBuffSize, uint32_t *pOutSize);
DIRTYCODE_API int32_t QosCommonClientToCoordinatorEstimateEncodedSize(const QosCommonClientToCoordinatorRequestT *pClientToCoordinatorRequest);
DIRTYCODE_API void QosCommonClientToCoordinatorPrint(const QosCommonClientToCoordinatorRequestT *pClientToCoordinatorRequest, uint32_t uLogLevel);

// QosCommonCoordinatorToClientResponseT helpers
DIRTYCODE_API int32_t QosCommonCoordinatorToClientResponseDecode(QosCommonCoordinatorToClientResponseT *pResponse, const uint8_t *pBuffer, uint32_t uBuffSize);
DIRTYCODE_API void QosCommonCoordinatorToClientPrint(const QosCommonCoordinatorToClientResponseT *pResponse, uint32_t uLogLevel);

// QosCommonServerToCoordinatorRequestT helpers
DIRTYCODE_API uint8_t* QosCommonServerToCoordinatorRequestEncode(const QosCommonServerToCoordinatorRequestT *pServerRegistrationRequest, uint8_t *pBuffer, uint32_t uBuffSize, uint32_t *pOutSize);

// QosCommonCoordinatorToServerResponseT helpers
DIRTYCODE_API int32_t QosCommonCoordinatorToServerResponseDecode(QosCommonCoordinatorToServerResponseT *pServerRegistrationResponse, const uint8_t *pBuffer, uint32_t uBuffSize);

// helpers for address
DIRTYCODE_API char* QosCommonAddrToString(const QosCommonAddrT *pAddr, char *pBuffer, int32_t iBufSize);
DIRTYCODE_API int32_t QosCommonStringToAddr(char *pStrIn, QosCommonAddrT *pOutAddr);
DIRTYCODE_API uint8_t QosCommonIsAddrEqual(QosCommonAddrT *pAddr1, QosCommonAddrT *pAddr2);
DIRTYCODE_API uint8_t QosCommonIsRemappedAddrEqual(QosCommonAddrT *pAddr1, struct sockaddr *pAddr2);
DIRTYCODE_API void QosCommonConvertAddr(QosCommonAddrT *pTargetAddr, struct sockaddr *pSourceAddr);

// helpers for serializing and deserializing packets
DIRTYCODE_API uint8_t QosCommonSerializeProbePacket(uint8_t *pOutBuff, uint32_t uBuffSize, const QosCommonProbePacketT *pInPacket, uint8_t *pSecureKey);
DIRTYCODE_API uint8_t QosCommonDeserializeProbePacket(QosCommonProbePacketT *pOutPacket, uint8_t *pInBuff, uint8_t *pSecureKey1, uint8_t *pSecureKey2);
DIRTYCODE_API void QosCommonDeserializeProbePacketInsecure(QosCommonProbePacketT *pOutPacket, uint8_t *pInBuff);
DIRTYCODE_API uint16_t QosCommonDeserializeClientRequestId(uint8_t *pInBuff);
DIRTYCODE_API uint32_t QosCommonDeserializeServiceRequestId(uint8_t *pInBuff);

// helpers for version numbering
DIRTYCODE_API uint16_t QosCommonMakeVersion(uint8_t uMajor, uint8_t uMinor);
DIRTYCODE_API void QosCommonGetVersion(uint16_t uVersion, uint8_t *uMajor, uint8_t *uMinor);
DIRTYCODE_API uint8_t QosCommonIsCompatibleVersion(uint16_t uVersion1, uint16_t uVersion2);
DIRTYCODE_API uint8_t QosCommonIsCompatibleProbeVersion(uint16_t uVersion);

#ifdef __cplusplus
}
#endif

//@}

#endif // _qoscommon_h

