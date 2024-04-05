/*H********************************************************************************/
/*!
    \File qoscommon.c

    \Description
        This code implements shared items between client and server of the QOS system.

    \Copyright
        Copyright (c) 2016 Electronic Arts Inc.

*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/misc/qoscommon.h"
#include "DirtySDK/util/binary7.h"
#include "DirtySDK/crypt/crypthash.h"
#include "DirtySDK/crypt/crypthmac.h"

#include "DirtySDK/util/protobufcommon.h"
#include "DirtySDK/util/protobufwrite.h"
#include "DirtySDK/util/protobufread.h"

/*** Defines **********************************************************************/

//!< Limits put on values when decoding rpcs

#define QOS_COMMON_MAX_INIT_SYNC_TIME           (5000)  //!< max time to wait for all the tests to start at the same time, after this time the tests will continue without synchronizing
#define QOS_COMMON_MAX_TIME_BETWEEN_PROBES      (1000)  //!< time between each probe send for a given test
#define QOS_COMMON_MIN_PROBE_COUNT              (1)     //!< the min amount of probes used for a given test, note QOS_COMMON_MAX_PROBE_COUNT is defined in qoscommon.h
#define QOS_COMMON_MAX_RESEND_EXTRA_PROBES      (QOS_COMMON_MAX_PROBE_COUNT)    //!< if we think we lost probes, should we send some extras to reduce the chances of losing some of those too
#define QOS_COMMON_MIN_TIMEOUT                  (7000)  //!< min qosclient request timeout period, the test is considered failed if this much time passed
#define QOS_COMMON_MAX_TIMEOUT                  (20000) //!< max qosclient request timeout period, the test is considered failed if this much time passed
#define QOS_COMMON_MIN_RESEND_TIME              (200)   //!< qosclient time till we decide probes have been lost, so send more probes then initially planned
#define QOS_COMMON_MAX_RESEND_TIME              (QOS_COMMON_MAX_TIMEOUT)        //!< qosclient time till we decide probes have been lost, so send more probes then initially planned
#define QOS_COMMON_MAX_ACCEPTABLE_LOST_PROBES   (QOS_COMMON_MAX_PROBE_COUNT)    //!< don't bother resend new probes if we've only lost x number of probes
#define QOS_COMMON_RPC_HEADER_SIZE              (1) //! one byte is needed to make a grpc header on top of what is done in protobuff 
#define QOS_COMMON_ENABLE_HMAC                  (1) //! be sure that hmac is enabled except for special debug cases

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/
// Note that all of the below enums must be kept in sync with the proto files 
// that belong to the coordinator.

//message ClientRequest
enum 
{
    CLIENT_TO_COORDINATOR_REQUEST_ARESULTS = 1,
    CLIENT_TO_COORDINATOR_REQUEST_PROFILE,
    CLIENT_TO_COORDINATOR_REQUEST_SERVICE_ID,
    CLIENT_TO_COORDINATOR_REQUEST_PROBE_VERSION,
    CLIENT_TO_COORDINATOR_REQUEST_MODIFIERS
}eClientToCoordinatorRequestFields;

//message ClientModifierFields
enum 
{
    CLIENT_MODIFIER_INTERNAL_ADDRESS = 1,
    CLIENT_MODIFIER_PLATFORM,
    CLIENT_MODIFIER_OS_FIREWALL,
    CLIENT_MODIFIER_PACKET_QUEUE_REMAINING,
    CLIENT_MODIFIER_STALL_COUNT_COMM,
    CLIENT_MODIFIER_STALL_DURATION_COMM,
    CLIENT_MODIFIER_STALL_COUNT_PROBE,
    CLIENT_MODIFIER_STALL_DURATION_PROBE,
    CLIENT_MODIFIER_HRESULT
}eQosCommonClientModifiersFields;

//message ClientRawResult
enum 
{
    RAW_RESULTS_ARESULT = 1,
    RAW_RESULTS_CLIENT_ADDRESS_FROM_SERVER,
    RAW_RESULTS_SITE_NAME,
    RAW_RESULTS_TEST_NAME,
    RAW_RESULTS_HRESULT
}eQosCommonRawResultsFields;

//message ProbeResult
enum 
{
    PROBE_RESULT_CLIENT_SEND_TIME = 1,
    PROBE_RESULT_SERVER_RECV_TIME,
    PROBE_RESULT_SERVER_SEND_DELTA,
    PROBE_RESULT_CLIENT_RECV_DELTA
}eQosCommonProbeResultFields;

//message ClientResponse
enum 
{
    COORDINATOR_TO_CLIENT_RESPONSE_STATUS = 1,
    COORDINATOR_TO_CLIENT_RESPONSE_SERVICE_ID,
    COORDINATOR_TO_CLIENT_RESPONSE_CLIENT_ADDR_FROM_COORDINATOR,
    COORDINATOR_TO_CLIENT_RESPONSE_FIREWALL_TYPE,
    COORDINATOR_TO_CLIENT_RESPONSE_TIME_TILL_RETRY,
    COORDINATOR_TO_CLIENT_RESPONSE_CLIENT_EXTERNAL_ADDRESS,
    COORDINATOR_TO_CLIENT_RESPONSE_APINGSITES,
    COORDINATOR_TO_CLIENT_RESPONSE_ACONTROL_CONFIGS,
    COORDINATOR_TO_CLIENT_RESPONSE_ATESTS,
    COORDINATOR_TO_CLIENT_RESPONSE_ASITE_RESULTS
}eCoordinatorToClientResponseFields;

//message PingSite
enum 
{
    SITE_NAME = 1,
    SITE_ADDR,
    SITE_PORT,
    SITE_KEY,
    SITE_PROBE_VERSION
}eQosCommonSiteFields;

//message ControlConfig
enum 
{
    CONTROL_CONFIG_NAME = 1,
    CONTROL_CONFIG_I_VALUE,
    CONTROL_CONFIG_STR_VALUE
}eQosCommonControlConfigFields;

//message QosTest
enum 
{
    TEST_NAME = 1,
    TEST_SITE,
    TEST_PROBE_COUNT_UP,
    TEST_PROBE_COUNT_DOWN,
    TEST_PROBE_SIZE_UP,
    TEST_PROBE_SIZE_DOWN,
    TEST_TIMEOUT,
    TEST_MINTIME_BETWEEN_PROBES,
    TEST_TIME_TILL_RESEND,
    TEST_RESEND_EXTRA_PROBE_COUNT,
    TEST_ACCEPTABLE_PROBE_LOSS,
    TEST_INIT_SYNC_TIME
}eQosCommonTestFields;

//message TestResult
enum 
{
    TEST_RESULT_SITE = 1,
    TEST_RESULT_RTT,
    TEST_RESULT_UP_BPS,
    TEST_RESULT_DOWN_BPS,
    TEST_RESULT_HRESULT
}eQosCommonTestResultFields;

//message ServerRegistrationRequest
enum 
{
    SERVER_TO_COORDINATOR_REQUEST_SITE = 1,
    SERVER_TO_COORDINATOR_REQUEST_ADDR,
    SERVER_TO_COORDINATOR_REQUEST_POOL,
    SERVER_TO_COORDINATOR_REQUEST_KEY,
    SERVER_TO_COORDINATOR_REQUEST_PORT,
    SERVER_TO_COORDINATOR_REQUEST_CAPCAITY_SEC,
    SERVER_TO_COORDINATOR_REQUEST_LAST_LOAD_SEC,
    SERVER_TO_COORDINATOR_REQUEST_PROBE_VERSION,
    SERVER_TO_COORDINATOR_REQUEST_UPDATE_INTERVAL,
    SERVER_TO_COORDINATOR_REQUEST_SHUTTING_DOWN
}eServerToCoordinatorRequestFields;

enum 
{
    COORDINATOR_TO_SERVER_RESPONSE_STATUS = 1,
    COORDINATOR_TO_SERVER_RESPONSE_REGISTRATION_MESSAGE,
    COORDINATOR_TO_SERVER_RESPONSE_MIN_SERVICE_ID
}eCoordinatorToServerResponseFields;

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _QosCommonScrubProbeResults

    \Description
        Time values are kind of meaningless outside of the box they are generated on
        here we try to make them more readable and take less space by trimming them to
        lower values.

    \Input *pIn         - input values
    \Input *pOut        - [out] output values
    \Input uHightWater  - max array index used

    \Version 08/02/2017 (cvienneau)
*/
/********************************************************************************F*/
static void _QosCommonScrubProbeResults(const QosCommonProbeResultT* pIn, QosCommonProbeResultT* pOut, uint32_t uHightWater)
{
    //scan data for lowest client side time and lowest server side time
    uint32_t uMinClientTime = (uint32_t)-1;
    uint32_t uMinServerTime = (uint32_t)-1;
    uint32_t uProbeIndex;
    for (uProbeIndex = 0; uProbeIndex <= uHightWater; uProbeIndex++)
    {
        if ((pIn[uProbeIndex].uClientSendTime != 0) && (pIn[uProbeIndex].uClientSendTime < uMinClientTime))
        {
            uMinClientTime = pIn[uProbeIndex].uClientSendTime;
        }

        if ((pIn[uProbeIndex].uServerReceiveTime != 0) && (pIn[uProbeIndex].uServerReceiveTime < uMinServerTime))
        {
            uMinServerTime = pIn[uProbeIndex].uServerReceiveTime;
        }
    }

    //zeros are used when probes are missing, so we want to use 1 instead.
    uMinClientTime--;
    uMinServerTime--;

    //pass over data again to reduce values
    for (uProbeIndex = 0; uProbeIndex <= uHightWater; uProbeIndex++)
    {
        if (pIn[uProbeIndex].uClientSendTime != 0)
        {
            pOut[uProbeIndex].uClientSendTime = pIn[uProbeIndex].uClientSendTime - uMinClientTime;
        }
        else
        {
            pOut[uProbeIndex].uClientSendTime = UINT32_MAX;
        }

        if (pIn[uProbeIndex].uServerReceiveTime != 0)
        {
            pOut[uProbeIndex].uServerReceiveTime = pIn[uProbeIndex].uServerReceiveTime - uMinServerTime;
            //copy the delta values out of the original array deltas don't need re-basing 
            pOut[uProbeIndex].uClientReceiveDelta = pIn[uProbeIndex].uClientReceiveDelta;
            pOut[uProbeIndex].uServerSendDelta = pIn[uProbeIndex].uServerSendDelta;
        }
        else
        {
            //if the server didn't receive the probe put error sentinel values in for everything
            pOut[uProbeIndex].uServerReceiveTime = UINT32_MAX;
            pOut[uProbeIndex].uClientReceiveDelta = UINT16_MAX;
            pOut[uProbeIndex].uServerSendDelta = UINT16_MAX;
        }

    }
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function QosCommonClientToCoordinatorEstimateEncodedSize

    \Description
    Estimate how much space will be needed to encode the QosCommonClientToCoordinatorRequestT.

    \Input *pClientToCoordinatorRequest - structure to read values from

    \Output
        int32_t           - negative on error or, size on success

    \Version 12/09/2016 (cvienneau)
*/
/********************************************************************************F*/
int32_t QosCommonClientToCoordinatorEstimateEncodedSize(const QosCommonClientToCoordinatorRequestT *pClientToCoordinatorRequest)
{
    // the basic introduction message with no results fits under 200 bytes, lets say 1k as a min anyways
    int32_t iEstimatedSize = 1024;
    // my own local test shows ~600 bytes for 3 results, approximately 200 bytes per result, but if the tests used more probes (i used 10) 
    // i could see this easily be 600b, lets just say 1k per result for plenty of wiggle room 
    iEstimatedSize = iEstimatedSize + (1024 * pClientToCoordinatorRequest->uNumResults);

    return(iEstimatedSize);
}

/*F********************************************************************************/
/*!
    \Function QosCommonClientToCoordinatorRequestEncode

    \Description
        Encode the QosCommonClientToCoordinatorRequestT into a buffer, which
        gives the coordinator information on who the client is and what test 
        results it might already have.

    \Input *pClientToCoordinatorRequest - structure to read values from
    \Input *pBuffer                     - [out] buffer we are writing to
    \Input uBuffSize                    - max size of buffer
    \Input *pOutSize                    - [out] output size of encoded data

    \Output
        uint8_t*           - NULL on error or, pointer to filled buffer on success

    \Version 12/09/2016 (cvienneau)
*/
/********************************************************************************F*/
uint8_t* QosCommonClientToCoordinatorRequestEncode(const QosCommonClientToCoordinatorRequestT *pClientToCoordinatorRequest, uint8_t *pBuffer, uint32_t uBuffSize, uint32_t *pOutSize)
{
    char strTemp[QOS_COMMON_MAX_RPC_STRING];
    uint32_t uIndex, uIndexProbe;
    int32_t iError = 0;

    ds_memclr(pBuffer, QOS_COMMON_RPC_HEADER_SIZE);
    ProtobufWriteRefT *pEncoder = ProtobufWriteCreate(pBuffer + QOS_COMMON_RPC_HEADER_SIZE, uBuffSize - QOS_COMMON_RPC_HEADER_SIZE, TRUE);

    if (pEncoder)
    {
        iError |= ProtobufWriteString(pEncoder, pClientToCoordinatorRequest->strQosProfile, (int32_t)strlen(pClientToCoordinatorRequest->strQosProfile), CLIENT_TO_COORDINATOR_REQUEST_PROFILE);
        iError |= ProtobufWriteVarint(pEncoder, pClientToCoordinatorRequest->uServiceRequestID, CLIENT_TO_COORDINATOR_REQUEST_SERVICE_ID);
        iError |= ProtobufWriteVarint(pEncoder, pClientToCoordinatorRequest->uProbeVersion, CLIENT_TO_COORDINATOR_REQUEST_PROBE_VERSION);

        iError |= ProtobufWriteMessageBegin(pEncoder, CLIENT_TO_COORDINATOR_REQUEST_MODIFIERS);
        QosCommonAddrToString(&pClientToCoordinatorRequest->clientModifiers.clientAddressInternal, strTemp, sizeof(strTemp));
        iError |= ProtobufWriteString(pEncoder, strTemp, (int32_t)strlen(strTemp), CLIENT_MODIFIER_INTERNAL_ADDRESS);
        iError |= ProtobufWriteString(pEncoder, pClientToCoordinatorRequest->clientModifiers.strPlatform, (int32_t)strlen(pClientToCoordinatorRequest->clientModifiers.strPlatform), CLIENT_MODIFIER_PLATFORM);
        iError |= ProtobufWriteVarint(pEncoder, pClientToCoordinatorRequest->clientModifiers.eOSFirewallType, CLIENT_MODIFIER_OS_FIREWALL);
        iError |= ProtobufWriteVarint(pEncoder, pClientToCoordinatorRequest->clientModifiers.uPacketQueueRemaining, CLIENT_MODIFIER_PACKET_QUEUE_REMAINING);
        iError |= ProtobufWriteVarint(pEncoder, pClientToCoordinatorRequest->clientModifiers.uStallCountCoordinator, CLIENT_MODIFIER_STALL_COUNT_COMM);
        iError |= ProtobufWriteVarint(pEncoder, pClientToCoordinatorRequest->clientModifiers.uStallCountProbe, CLIENT_MODIFIER_STALL_COUNT_PROBE);
        iError |= ProtobufWriteVarint(pEncoder, pClientToCoordinatorRequest->clientModifiers.uStallDurationCoordinator, CLIENT_MODIFIER_STALL_DURATION_COMM);
        iError |= ProtobufWriteVarint(pEncoder, pClientToCoordinatorRequest->clientModifiers.uStallDurationProbe, CLIENT_MODIFIER_STALL_DURATION_PROBE);
        iError |= ProtobufWriteVarint(pEncoder, pClientToCoordinatorRequest->clientModifiers.hResult, CLIENT_MODIFIER_HRESULT);
        iError |= ProtobufWriteMessageEnd(pEncoder);    //end of CLIENT_TO_COORDINATOR_REQUEST_MODIFIERS

                                                        // add all current results
        if ((iError == 0) && (pClientToCoordinatorRequest->uNumResults > 0))
        {
            for (uIndex = 0; uIndex < pClientToCoordinatorRequest->uNumResults; uIndex++)
            {
                iError |= ProtobufWriteMessageBegin(pEncoder, CLIENT_TO_COORDINATOR_REQUEST_ARESULTS);
                iError |= ProtobufWriteVarint(pEncoder, pClientToCoordinatorRequest->aResults[uIndex].hResult, RAW_RESULTS_HRESULT);
                iError |= ProtobufWriteString(pEncoder, pClientToCoordinatorRequest->aResults[uIndex].strSiteName, (int32_t)strlen(pClientToCoordinatorRequest->aResults[uIndex].strSiteName), RAW_RESULTS_SITE_NAME);
                iError |= ProtobufWriteString(pEncoder, pClientToCoordinatorRequest->aResults[uIndex].strTestName, (int32_t)strlen(pClientToCoordinatorRequest->aResults[uIndex].strTestName), RAW_RESULTS_TEST_NAME);
                QosCommonAddrToString(&pClientToCoordinatorRequest->aResults[uIndex].clientAddressFromServer, strTemp, sizeof(strTemp));
                iError |= ProtobufWriteString(pEncoder, strTemp, (int32_t)strlen(strTemp), RAW_RESULTS_CLIENT_ADDRESS_FROM_SERVER);

                if ((iError == 0) && (pClientToCoordinatorRequest->aResults[uIndex].uProbeCountDown > 0))
                {
                    //make an array the same size as the one contained in the struct
                    QosCommonProbeResultT aScrubedProbeResults[(sizeof(pClientToCoordinatorRequest->aResults[uIndex].aProbeResult) / sizeof(pClientToCoordinatorRequest->aResults[uIndex].aProbeResult[0]))];

                    //lower the values used to something more human readable
                    _QosCommonScrubProbeResults(pClientToCoordinatorRequest->aResults[uIndex].aProbeResult, aScrubedProbeResults, pClientToCoordinatorRequest->aResults[uIndex].uProbeCountHighWater);
                    // info on individual probes
                    for (uIndexProbe = 0; uIndexProbe <= pClientToCoordinatorRequest->aResults[uIndex].uProbeCountHighWater; uIndexProbe++)  //this list may have spaces if some probes were not received 
                    {
                        iError |= ProtobufWriteMessageBegin(pEncoder, RAW_RESULTS_ARESULT);
                        iError |= ProtobufWriteVarint(pEncoder, aScrubedProbeResults[uIndexProbe].uClientSendTime, PROBE_RESULT_CLIENT_SEND_TIME);
                        iError |= ProtobufWriteVarint(pEncoder, aScrubedProbeResults[uIndexProbe].uServerReceiveTime, PROBE_RESULT_SERVER_RECV_TIME);
                        iError |= ProtobufWriteVarint(pEncoder, aScrubedProbeResults[uIndexProbe].uServerSendDelta, PROBE_RESULT_SERVER_SEND_DELTA);
                        iError |= ProtobufWriteVarint(pEncoder, aScrubedProbeResults[uIndexProbe].uClientReceiveDelta, PROBE_RESULT_CLIENT_RECV_DELTA);
                        iError |= ProtobufWriteMessageEnd(pEncoder);    //end of probe result
                    }
                }
                iError |= ProtobufWriteMessageEnd(pEncoder);    //end of result
            }
        }
        *pOutSize = ProtobufWriteDestroy(pEncoder) + QOS_COMMON_RPC_HEADER_SIZE;
    }
    else
    {
        iError |= -1;
        *pOutSize = 0;
    }
    return((iError == 0) ? pBuffer : NULL);
}

/*F********************************************************************************/
/*!
    \Function QosCommonClientToCoordinatorPrint

    \Description
        Print the QosCommonClientToCoordinatorRequestT to tty.

    \Input *pClientToCoordinatorRequest - structure to print
    \Input uLogLevel                    - log level to control spam

    \Version 03/03/2018 (cvienneau)
*/
/********************************************************************************F*/
void QosCommonClientToCoordinatorPrint(const QosCommonClientToCoordinatorRequestT *pClientToCoordinatorRequest, uint32_t uLogLevel)
{
#if DIRTYCODE_LOGGING
    char strTemp[QOS_COMMON_MAX_RPC_STRING];
    uint32_t uIndex, uIndexProbe;

    NetPrintfVerbose((uLogLevel, 0, "qoscommon: cl2co.strQosProfile:                %s\n", pClientToCoordinatorRequest->strQosProfile));
    NetPrintfVerbose((uLogLevel, 0, "qoscommon: cl2co.uServiceRequestID:            %u\n", pClientToCoordinatorRequest->uServiceRequestID));
    NetPrintfVerbose((uLogLevel, 1, "qoscommon: cl2co.uProbeVersion:                %u\n", pClientToCoordinatorRequest->uProbeVersion));

    QosCommonAddrToString(&pClientToCoordinatorRequest->clientModifiers.clientAddressInternal, strTemp, sizeof(strTemp));
    NetPrintfVerbose((uLogLevel, 0, "qoscommon: cl2co.cm.clientAddressInternal:     %s\n", strTemp));
    NetPrintfVerbose((uLogLevel, 1, "qoscommon: cl2co.cm.strPlatform:               %s\n",  pClientToCoordinatorRequest->clientModifiers.strPlatform));
    NetPrintfVerbose((uLogLevel, 0, "qoscommon: cl2co.cm.eOSFirewallType:           %u\n", pClientToCoordinatorRequest->clientModifiers.eOSFirewallType));
    NetPrintfVerbose((uLogLevel, 0, "qoscommon: cl2co.cm.uPacketQueueRemaining:     %u\n", pClientToCoordinatorRequest->clientModifiers.uPacketQueueRemaining));
    NetPrintfVerbose((uLogLevel, 1, "qoscommon: cl2co.cm.uStallCountCoordinator:    %u\n", pClientToCoordinatorRequest->clientModifiers.uStallCountCoordinator));
    NetPrintfVerbose((uLogLevel, 1, "qoscommon: cl2co.cm.uStallCountProbe:          %u\n", pClientToCoordinatorRequest->clientModifiers.uStallCountProbe));
    NetPrintfVerbose((uLogLevel, 0, "qoscommon: cl2co.cm.uStallDurationCoordinator: %u\n", pClientToCoordinatorRequest->clientModifiers.uStallDurationCoordinator));
    NetPrintfVerbose((uLogLevel, 0, "qoscommon: cl2co.cm.uStallDurationProbe:       %u\n", pClientToCoordinatorRequest->clientModifiers.uStallDurationProbe));
    NetPrintfVerbose((uLogLevel, 0, "qoscommon: cl2co.cm.hResult:                   0x%08x\n", pClientToCoordinatorRequest->clientModifiers.hResult));
    
    // add all current results
    for (uIndex = 0; uIndex < pClientToCoordinatorRequest->uNumResults; uIndex++)
    {
        NetPrintfVerbose((uLogLevel, 0, "qoscommon: cl2co.aResults[%u] site: %s, test: %s, hresult: 0x%08x, up: %u, down: %u, high: %u, addr: %s\n", 
            uIndex, 
            pClientToCoordinatorRequest->aResults[uIndex].strSiteName,
            pClientToCoordinatorRequest->aResults[uIndex].strTestName,
            pClientToCoordinatorRequest->aResults[uIndex].hResult,
            pClientToCoordinatorRequest->aResults[uIndex].uProbeCountUp,
            pClientToCoordinatorRequest->aResults[uIndex].uProbeCountDown,
            pClientToCoordinatorRequest->aResults[uIndex].uProbeCountHighWater,
            QosCommonAddrToString(&pClientToCoordinatorRequest->aResults[uIndex].clientAddressFromServer, strTemp, sizeof(strTemp))
            ));

        if (pClientToCoordinatorRequest->aResults[uIndex].uProbeCountDown > 0)
        {
            //make an array the same size as the one contained in the struct
            QosCommonProbeResultT aScrubedProbeResults[(sizeof(pClientToCoordinatorRequest->aResults[uIndex].aProbeResult) / sizeof(pClientToCoordinatorRequest->aResults[uIndex].aProbeResult[0]))];

            //lower the values used to something more human readable
            _QosCommonScrubProbeResults(pClientToCoordinatorRequest->aResults[uIndex].aProbeResult, aScrubedProbeResults, pClientToCoordinatorRequest->aResults[uIndex].uProbeCountHighWater);
            // info on individual probes
            for (uIndexProbe = 0; uIndexProbe <= pClientToCoordinatorRequest->aResults[uIndex].uProbeCountHighWater; uIndexProbe++)  //this list may have spaces if some probes were not received 
            {
                // only print the first probe, unless we have higher logging level set.
                int32_t uLocalLogLevel = 0;
                if (uIndexProbe > 0)
                {
                    uLocalLogLevel = 1;
                }
                NetPrintfVerbose((uLogLevel, uLocalLogLevel, "qoscommon: cl2co.aResults[%d].aProbeResult[%d] csend: %u, srcv: %u, sdelta: %u, cdelta: %u\n", uIndex, uIndexProbe,
                    aScrubedProbeResults[uIndexProbe].uClientSendTime,
                    aScrubedProbeResults[uIndexProbe].uServerReceiveTime,
                    aScrubedProbeResults[uIndexProbe].uServerSendDelta,
                    aScrubedProbeResults[uIndexProbe].uClientReceiveDelta));
            }
        }
    }
#endif
}


/*F********************************************************************************/
/*!
    \Function QosCommonCoordinatorToClientResponseDecode

    \Description
        Decodes the buffer, into a QosCommonCoordinatorToClientResponseT, which
        gives the client instructions on what it should be doing next.

    \Input *pResponse   - [out] structure to write values to
    \Input *pBuffer     - buffer we are reading from
    \Input uBuffSize    - max size of pBuffer

    \Output
        int32_t         - -1 on error 0 otherwise

    \Version 12/09/2016 (cvienneau)
*/
/********************************************************************************F*/
int32_t QosCommonCoordinatorToClientResponseDecode(QosCommonCoordinatorToClientResponseT *pResponse, const uint8_t *pBuffer, uint32_t uBuffSize)
{
    int32_t iMsgSize;
    uint8_t bError = FALSE;

    ds_memclr(pResponse, sizeof(QosCommonCoordinatorToClientResponseT));
    pBuffer = ProtobufCommonReadSize(pBuffer + QOS_COMMON_RPC_HEADER_SIZE, uBuffSize - QOS_COMMON_RPC_HEADER_SIZE, &iMsgSize);

    if (pBuffer)
    {
        ProtobufReadT Reader, ReaderValue;
        const uint8_t *pCurrentRepeat;
        char strTempAddr[QOS_COMMON_MAX_RPC_STRING];
        ProtobufReadInit(&Reader, pBuffer, iMsgSize);

        pResponse->uServiceRequestID = (uint32_t)ProtobufReadVarint(&Reader, ProtobufReadFind(&Reader, COORDINATOR_TO_CLIENT_RESPONSE_SERVICE_ID));
        ProtobufReadString(&Reader, ProtobufReadFind(&Reader, COORDINATOR_TO_CLIENT_RESPONSE_CLIENT_ADDR_FROM_COORDINATOR), strTempAddr, sizeof(strTempAddr));
        QosCommonStringToAddr(strTempAddr, &pResponse->configuration.clientAddressFromCoordinator);

        //most will only be valid if results have been calculated, but we don't know what magic the coordinator might use
        pResponse->results.eFirewallType = (QosCommonFirewallTypeE)ProtobufReadVarint(&Reader, ProtobufReadFind(&Reader, COORDINATOR_TO_CLIENT_RESPONSE_FIREWALL_TYPE));
        pResponse->results.uTimeTillRetry = (uint32_t)ProtobufReadVarint(&Reader, ProtobufReadFind(&Reader, COORDINATOR_TO_CLIENT_RESPONSE_TIME_TILL_RETRY));
        ProtobufReadString(&Reader, ProtobufReadFind(&Reader, COORDINATOR_TO_CLIENT_RESPONSE_CLIENT_EXTERNAL_ADDRESS), strTempAddr, sizeof(strTempAddr));
        QosCommonStringToAddr(strTempAddr, &pResponse->results.clientExternalAddress);

        pResponse->configuration.uNumSites = 0;
        pCurrentRepeat = NULL;
        while ((pCurrentRepeat = ProtobufReadFind2(&Reader, COORDINATOR_TO_CLIENT_RESPONSE_APINGSITES, pCurrentRepeat)) != NULL)
        {
            if (pResponse->configuration.uNumSites >= QOS_COMMON_MAX_SITES)
            {
                bError = TRUE;
                break;
            }
            pCurrentRepeat = ProtobufReadMessage(&Reader, pCurrentRepeat, &ReaderValue);
            ProtobufReadString(&ReaderValue, ProtobufReadFind(&ReaderValue, SITE_NAME), pResponse->configuration.aSites[pResponse->configuration.uNumSites].strSiteName, sizeof(pResponse->configuration.aSites[pResponse->configuration.uNumSites].strSiteName));
            ProtobufReadString(&ReaderValue, ProtobufReadFind(&ReaderValue, SITE_ADDR), pResponse->configuration.aSites[pResponse->configuration.uNumSites].strProbeAddr, sizeof(pResponse->configuration.aSites[pResponse->configuration.uNumSites].strProbeAddr));
            ProtobufReadBytes(&ReaderValue, ProtobufReadFind(&ReaderValue, SITE_KEY), pResponse->configuration.aSites[pResponse->configuration.uNumSites].aSecureKey, sizeof(pResponse->configuration.aSites[pResponse->configuration.uNumSites].aSecureKey));
            pResponse->configuration.aSites[pResponse->configuration.uNumSites].uProbePort = ProtobufReadVarint(&ReaderValue, ProtobufReadFind(&ReaderValue, SITE_PORT));
            pResponse->configuration.aSites[pResponse->configuration.uNumSites].uProbeVersion = ProtobufReadVarint(&ReaderValue, ProtobufReadFind(&ReaderValue, SITE_PROBE_VERSION));
            pResponse->configuration.uNumSites++;
        }

        pResponse->configuration.uNumControlConfigs = 0;
        pCurrentRepeat = NULL;
        while ((pCurrentRepeat = ProtobufReadFind2(&Reader, COORDINATOR_TO_CLIENT_RESPONSE_ACONTROL_CONFIGS, pCurrentRepeat)) != NULL)
        {
            if (pResponse->configuration.uNumControlConfigs >= QOS_COMMON_MAX_CONTROL_CONFIGS)
            {
                bError = TRUE;
                break;
            }
            pCurrentRepeat = ProtobufReadMessage(&Reader, pCurrentRepeat, &ReaderValue);
            ProtobufReadString(&ReaderValue, ProtobufReadFind(&ReaderValue, CONTROL_CONFIG_NAME), pResponse->configuration.aControlConfigs[pResponse->configuration.uNumControlConfigs].strControl, sizeof(pResponse->configuration.aControlConfigs[pResponse->configuration.uNumControlConfigs].strControl));
            pResponse->configuration.aControlConfigs[pResponse->configuration.uNumControlConfigs].iValue = (int32_t)ProtobufReadVarint(&ReaderValue, ProtobufReadFind(&ReaderValue, CONTROL_CONFIG_I_VALUE));
            ProtobufReadString(&ReaderValue, ProtobufReadFind(&ReaderValue, CONTROL_CONFIG_STR_VALUE), pResponse->configuration.aControlConfigs[pResponse->configuration.uNumControlConfigs].strValue, sizeof(pResponse->configuration.aControlConfigs[pResponse->configuration.uNumControlConfigs].strValue));
            pResponse->configuration.uNumControlConfigs++;
        }

        pResponse->configuration.uNumQosTests = 0;
        pCurrentRepeat = NULL;
        while ((pCurrentRepeat = ProtobufReadFind2(&Reader, COORDINATOR_TO_CLIENT_RESPONSE_ATESTS, pCurrentRepeat)) != NULL)
        {
            if (pResponse->configuration.uNumQosTests >= QOS_COMMON_MAX_TESTS)
            {
                bError = TRUE;
                break;
            }
            pCurrentRepeat = ProtobufReadMessage(&Reader, pCurrentRepeat, &ReaderValue);
            ProtobufReadString(&ReaderValue, ProtobufReadFind(&ReaderValue, TEST_NAME), pResponse->configuration.aQosTests[pResponse->configuration.uNumQosTests].strTestName, sizeof(pResponse->configuration.aQosTests[pResponse->configuration.uNumQosTests].strTestName));
            ProtobufReadString(&ReaderValue, ProtobufReadFind(&ReaderValue, TEST_SITE), pResponse->configuration.aQosTests[pResponse->configuration.uNumQosTests].strSiteName, sizeof(pResponse->configuration.aQosTests[pResponse->configuration.uNumQosTests].strSiteName));
            pResponse->configuration.aQosTests[pResponse->configuration.uNumQosTests].uProbeCountUp = DS_CLAMP(ProtobufReadVarint(&ReaderValue, ProtobufReadFind(&ReaderValue, TEST_PROBE_COUNT_UP)), QOS_COMMON_MIN_PROBE_COUNT, QOS_COMMON_MAX_PROBE_COUNT);
            pResponse->configuration.aQosTests[pResponse->configuration.uNumQosTests].uProbeCountDown = DS_CLAMP(ProtobufReadVarint(&ReaderValue, ProtobufReadFind(&ReaderValue, TEST_PROBE_COUNT_DOWN)), QOS_COMMON_MIN_PROBE_COUNT, QOS_COMMON_MAX_PROBE_COUNT);
            pResponse->configuration.aQosTests[pResponse->configuration.uNumQosTests].uProbeSizeUp = DS_CLAMP(ProtobufReadVarint(&ReaderValue, ProtobufReadFind(&ReaderValue, TEST_PROBE_SIZE_UP)), QOS_COMMON_MIN_PACKET_SIZE, QOS_COMMON_MAX_PACKET_SIZE);
            pResponse->configuration.aQosTests[pResponse->configuration.uNumQosTests].uProbeSizeDown = DS_CLAMP(ProtobufReadVarint(&ReaderValue, ProtobufReadFind(&ReaderValue, TEST_PROBE_SIZE_DOWN)), QOS_COMMON_MIN_PACKET_SIZE, QOS_COMMON_MAX_PACKET_SIZE);
            pResponse->configuration.aQosTests[pResponse->configuration.uNumQosTests].uTimeout = DS_CLAMP(ProtobufReadVarint(&ReaderValue, ProtobufReadFind(&ReaderValue, TEST_TIMEOUT)), QOS_COMMON_MIN_TIMEOUT, QOS_COMMON_MAX_TIMEOUT);
            pResponse->configuration.aQosTests[pResponse->configuration.uNumQosTests].uMinTimeBetwenProbes = DS_MIN(ProtobufReadVarint(&ReaderValue, ProtobufReadFind(&ReaderValue, TEST_MINTIME_BETWEEN_PROBES)), QOS_COMMON_MAX_TIME_BETWEEN_PROBES);
            pResponse->configuration.aQosTests[pResponse->configuration.uNumQosTests].uTimeTillResend = DS_CLAMP(ProtobufReadVarint(&ReaderValue, ProtobufReadFind(&ReaderValue, TEST_TIME_TILL_RESEND)), QOS_COMMON_MIN_RESEND_TIME, QOS_COMMON_MAX_RESEND_TIME);
            pResponse->configuration.aQosTests[pResponse->configuration.uNumQosTests].uResendExtraProbeCount = DS_MIN(ProtobufReadVarint(&ReaderValue, ProtobufReadFind(&ReaderValue, TEST_RESEND_EXTRA_PROBE_COUNT)), QOS_COMMON_MAX_RESEND_EXTRA_PROBES);
            pResponse->configuration.aQosTests[pResponse->configuration.uNumQosTests].uAcceptableLostProbeCount = DS_MIN(ProtobufReadVarint(&ReaderValue, ProtobufReadFind(&ReaderValue, TEST_ACCEPTABLE_PROBE_LOSS)), QOS_COMMON_MAX_ACCEPTABLE_LOST_PROBES);
            pResponse->configuration.aQosTests[pResponse->configuration.uNumQosTests].uInitSyncTimeout = DS_MIN(ProtobufReadVarint(&ReaderValue, ProtobufReadFind(&ReaderValue, TEST_INIT_SYNC_TIME)), QOS_COMMON_MAX_INIT_SYNC_TIME);
            pResponse->configuration.uNumQosTests++;
        }

        pResponse->results.uNumResults = 0;
        pCurrentRepeat = NULL;
        while ((pCurrentRepeat = ProtobufReadFind2(&Reader, COORDINATOR_TO_CLIENT_RESPONSE_ASITE_RESULTS, pCurrentRepeat)) != NULL)
        {
            if (pResponse->results.uNumResults >= QOS_COMMON_MAX_SITES)
            {
                bError = TRUE;
                break;
            }
            pCurrentRepeat = ProtobufReadMessage(&Reader, pCurrentRepeat, &ReaderValue);
            ProtobufReadString(&ReaderValue, ProtobufReadFind(&ReaderValue, TEST_RESULT_SITE), pResponse->results.aTestResults[pResponse->results.uNumResults].strSiteName, sizeof(pResponse->results.aTestResults[pResponse->results.uNumResults].strSiteName));
            pResponse->results.aTestResults[pResponse->results.uNumResults].uMinRTT = (uint32_t)ProtobufReadVarint(&ReaderValue, ProtobufReadFind(&ReaderValue, TEST_RESULT_RTT));
            pResponse->results.aTestResults[pResponse->results.uNumResults].uUpbps = (uint32_t)ProtobufReadVarint(&ReaderValue, ProtobufReadFind(&ReaderValue, TEST_RESULT_UP_BPS));
            pResponse->results.aTestResults[pResponse->results.uNumResults].uDownbps = (uint32_t)ProtobufReadVarint(&ReaderValue, ProtobufReadFind(&ReaderValue, TEST_RESULT_DOWN_BPS));
            pResponse->results.aTestResults[pResponse->results.uNumResults].hResult = (uint32_t)ProtobufReadVarint(&ReaderValue, ProtobufReadFind(&ReaderValue, TEST_RESULT_HRESULT));
            pResponse->results.uNumResults++;
        }
    }
    else
    {
        bError = TRUE;
    }

    return(bError ? -1 : 0);
}

/*F********************************************************************************/
/*!
    \Function QosCommonCoordinatorToClientPrint

    \Description
        Print the QosCommonCoordinatorToClientResponseT to tty.

    \Input *pResponse       - structure to print
    \Input uLogLevel        - log level to control spam

    \Version 03/03/2018 (cvienneau)
*/
/********************************************************************************F*/
void QosCommonCoordinatorToClientPrint(const QosCommonCoordinatorToClientResponseT *pResponse, uint32_t uLogLevel)
{
#if DIRTYCODE_LOGGING
    char strTemp[QOS_COMMON_MAX_RPC_STRING];
    uint32_t uIndex;

    //todo set proper log levels of these prints

    NetPrintfVerbose((uLogLevel, 0, "qoscommon: co2cl.uServiceRequestID:                          %u\n", pResponse->uServiceRequestID));
    NetPrintfVerbose((uLogLevel, 0, "qoscommon: co2cl.configuration.clientAddressFromCoordinator: %s\n", QosCommonAddrToString(&pResponse->configuration.clientAddressFromCoordinator, strTemp, sizeof(strTemp))));
    NetPrintfVerbose((uLogLevel, 1, "qoscommon: co2cl.results.eFirewallType:                      %u\n", pResponse->results.eFirewallType));
    NetPrintfVerbose((uLogLevel, 0, "qoscommon: co2cl.results.uTimeTillRetry:                     %u\n", pResponse->results.uTimeTillRetry));
    NetPrintfVerbose((uLogLevel, 1, "qoscommon: co2cl.results.clientExternalAddress:              %s\n", QosCommonAddrToString(&pResponse->results.clientExternalAddress, strTemp, sizeof(strTemp))));

    for (uIndex = 0; uIndex < pResponse->configuration.uNumSites; uIndex++)
    {
        NetPrintfVerbose((uLogLevel, 0, "qoscommon: co2cl.configuration.aSites[%d] %s, %s:%u, %u\n", 
            uIndex,
            pResponse->configuration.aSites[uIndex].strSiteName, 
            pResponse->configuration.aSites[uIndex].strProbeAddr, 
            pResponse->configuration.aSites[uIndex].uProbePort,
            pResponse->configuration.aSites[uIndex].uProbeVersion));
        //NetPrintfVerbose((uLogLevel, 0, "qoscommon: co2cl.configuration.aSites[%d].aSecureKey:      X\n", pResponse->configuration.aSites[uIndex].aSecureKey));
    }

    for (uIndex = 0; uIndex < pResponse->configuration.uNumControlConfigs; uIndex++)
    {
        NetPrintfVerbose((uLogLevel, 0, "qoscommon: co2cl.configuration.aControlConfigs[%u]: '%s', %d, %s\n", 
            uIndex,
            pResponse->configuration.aControlConfigs[uIndex].strControl, 
            pResponse->configuration.aControlConfigs[uIndex].iValue, 
            pResponse->configuration.aControlConfigs[uIndex].strValue));
    }

    for (uIndex = 0; uIndex < pResponse->configuration.uNumQosTests; uIndex++)
    {
        NetPrintfVerbose((uLogLevel, 0, "qoscommon: co2cl.configuration.aQosTests[%u]:  %s, %s, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u\n", 
            uIndex,
            pResponse->configuration.aQosTests[uIndex].strTestName, 
            pResponse->configuration.aQosTests[uIndex].strSiteName, 
            pResponse->configuration.aQosTests[uIndex].uProbeCountUp, 
            pResponse->configuration.aQosTests[uIndex].uProbeCountDown,
            pResponse->configuration.aQosTests[uIndex].uProbeSizeUp,
            pResponse->configuration.aQosTests[uIndex].uProbeSizeDown,
            pResponse->configuration.aQosTests[uIndex].uTimeout,
            pResponse->configuration.aQosTests[uIndex].uMinTimeBetwenProbes,
            pResponse->configuration.aQosTests[uIndex].uTimeTillResend,
            pResponse->configuration.aQosTests[uIndex].uResendExtraProbeCount,
            pResponse->configuration.aQosTests[uIndex].uAcceptableLostProbeCount,
            pResponse->configuration.aQosTests[uIndex].uInitSyncTimeout));
    }

    for (uIndex = 0; uIndex < pResponse->results.uNumResults; uIndex++)
    {
        NetPrintfVerbose((uLogLevel, 1, "qoscommon: co2cl.results.aTestResults[%d]:  %s, %u, %u, %u, 0x%08x\n",
            uIndex,
            pResponse->results.aTestResults[uIndex].strSiteName,
            pResponse->results.aTestResults[uIndex].uMinRTT,
            pResponse->results.aTestResults[uIndex].uUpbps,
            pResponse->results.aTestResults[uIndex].uDownbps,
            pResponse->results.aTestResults[uIndex].hResult));
    }
#endif
}


/*F********************************************************************************/
/*!
    \Function QosCommonServerToCoordinatorRequestEncode

    \Description
        Encode QosCommonServerToCoordinatorRequestT into a buffer, which either
        tells the coordinator we are a new server to be added to the available pool
        or as a heartbeat and update of the secure key.

    \Input *pServerRegistrationRequest  - structure to read values from
    \Input *pBuffer                     - [out] buffer we are writing to
    \Input uBuffSize                    - size of buffer
    \Input *pOutSize                    - [out] output size of encoded data

    \Output
        uint8_t*   - NULL on error, pointer to filled buffer on success

    \Version 12/09/2016 (cvienneau)
*/
/********************************************************************************F*/
uint8_t* QosCommonServerToCoordinatorRequestEncode(const QosCommonServerToCoordinatorRequestT *pServerRegistrationRequest, uint8_t *pBuffer, uint32_t uBuffSize, uint32_t *pOutSize)
{
    int32_t iError = 0;

    //first byte is grpc header
    ds_memclr(pBuffer, QOS_COMMON_RPC_HEADER_SIZE);
    ProtobufWriteRefT *pEncoder = ProtobufWriteCreate(pBuffer + QOS_COMMON_RPC_HEADER_SIZE, uBuffSize - QOS_COMMON_RPC_HEADER_SIZE, TRUE);

    if (pEncoder)
    {
        iError |= ProtobufWriteString(pEncoder, pServerRegistrationRequest->strSiteName, (int32_t)strlen(pServerRegistrationRequest->strSiteName), SERVER_TO_COORDINATOR_REQUEST_SITE);
        iError |= ProtobufWriteString(pEncoder, pServerRegistrationRequest->strPool, (int32_t)strlen(pServerRegistrationRequest->strPool), SERVER_TO_COORDINATOR_REQUEST_POOL);
        iError |= ProtobufWriteString(pEncoder, pServerRegistrationRequest->strAddr, (int32_t)strlen(pServerRegistrationRequest->strAddr), SERVER_TO_COORDINATOR_REQUEST_ADDR);
        iError |= ProtobufWriteBytes(pEncoder, pServerRegistrationRequest->aSecureKey, sizeof(pServerRegistrationRequest->aSecureKey), SERVER_TO_COORDINATOR_REQUEST_KEY);
        iError |= ProtobufWriteVarint(pEncoder, pServerRegistrationRequest->uPort, SERVER_TO_COORDINATOR_REQUEST_PORT);
        iError |= ProtobufWriteVarint(pEncoder, pServerRegistrationRequest->uCapacityPerSec, SERVER_TO_COORDINATOR_REQUEST_CAPCAITY_SEC);
        iError |= ProtobufWriteVarint(pEncoder, pServerRegistrationRequest->uLastLoadPerSec, SERVER_TO_COORDINATOR_REQUEST_LAST_LOAD_SEC);
        iError |= ProtobufWriteVarint(pEncoder, pServerRegistrationRequest->uProbeVersion, SERVER_TO_COORDINATOR_REQUEST_PROBE_VERSION);
        iError |= ProtobufWriteVarint(pEncoder, pServerRegistrationRequest->uUpdateInterval, SERVER_TO_COORDINATOR_REQUEST_UPDATE_INTERVAL);
        iError |= ProtobufWriteVarint(pEncoder, pServerRegistrationRequest->bShuttingDown, SERVER_TO_COORDINATOR_REQUEST_SHUTTING_DOWN);
        *pOutSize = ProtobufWriteDestroy(pEncoder) + QOS_COMMON_RPC_HEADER_SIZE;
    }
    else
    {
        iError |= -1;
        *pOutSize = 0;
    }
    return((iError == 0) ? pBuffer : NULL);
}

/*F********************************************************************************/
/*!
    \Function QosCommonCoordinatorToServerResponseDecode

    \Description
        Decode buffer into a  QosCommonCoordinatorToServerResponseT, which
        tells the server any registration status.

    \Input *pServerRegistrationResponse - [out] structure to write values to
    \Input *pBuffer                     - buffer we are reading from
    \Input uBuffSize                    - size of buffer

    \Output
        int32_t                         - -1 on err, 0 on success

    \Version 12/09/2016 (cvienneau)
*/
/********************************************************************************F*/
int32_t QosCommonCoordinatorToServerResponseDecode(QosCommonCoordinatorToServerResponseT *pServerRegistrationResponse, const uint8_t *pBuffer, uint32_t uBuffSize)
{
    int32_t iMsgSize;
    uint8_t bError = FALSE;

    ds_memclr(pServerRegistrationResponse, sizeof(QosCommonCoordinatorToServerResponseT));
    pBuffer = ProtobufCommonReadSize(pBuffer + QOS_COMMON_RPC_HEADER_SIZE, uBuffSize - QOS_COMMON_RPC_HEADER_SIZE, &iMsgSize);

    if (pBuffer)
    {
        ProtobufReadT Reader;
        ProtobufReadInit(&Reader, pBuffer, iMsgSize);
        //note we don't do anything with COORDINATOR_TO_SERVER_RESPONSE_STATUS
        ProtobufReadString(&Reader, ProtobufReadFind(&Reader, COORDINATOR_TO_SERVER_RESPONSE_REGISTRATION_MESSAGE), pServerRegistrationResponse->strRegistrationMessage, sizeof(pServerRegistrationResponse->strRegistrationMessage));
        pServerRegistrationResponse->uMinServiceRequestID = (uint32_t)ProtobufReadVarint(&Reader, ProtobufReadFind(&Reader, COORDINATOR_TO_SERVER_RESPONSE_MIN_SERVICE_ID));
    }
    else
    {
        bError = TRUE;
    }

    return(bError ? -1 : 0);
}


/*F********************************************************************************/
/*!
    \Function QosCommonAddrToString

    \Description
        Convert a address into a human readable easily pars-able string.

    \Input *pAddr       - address to convert
    \Input *pBuffer     - [out] buffer to write to
    \Input iBufSize     - size of buffer

    \Output
        char*           - the pBuffer pointer that was written to    

    \Version 12/09/2016 (cvienneau)
*/
/********************************************************************************F*/
char* QosCommonAddrToString(const QosCommonAddrT *pAddr, char *pBuffer, int32_t iBufSize)
{
    //we build a sockaddr, just so we can use our library print function, which saves us some headache
    // if we wanted more details take a look at _ds_sockaddrtostr()
    if (pAddr->uFamily == AF_INET)
    {
        struct sockaddr sockAddr;
        SockaddrInit(&sockAddr, AF_INET);
        SockaddrInSetAddr(&sockAddr, pAddr->addr.v4);
        SockaddrInSetPort(&sockAddr, pAddr->uPort);
        ds_snzprintf(pBuffer, iBufSize, "v4%A", &sockAddr);
    }
    #ifndef DIRTYCODE_NX
    else if (pAddr->uFamily == AF_INET6)
    {
        struct sockaddr_in6 sockAddr6;
        SockaddrInit6(&sockAddr6, AF_INET6);
        ds_memcpy(&sockAddr6.sin6_addr, &(pAddr->addr.v6), sizeof(sockAddr6.sin6_addr));
        sockAddr6.sin6_port = pAddr->uPort;
        ds_snzprintf(pBuffer, iBufSize, "v6%A", &sockAddr6);
    }
    #endif
    else
    {
        ds_snzprintf(pBuffer, iBufSize, "na[0]:0");
    }
    return(pBuffer);
}

/*F********************************************************************************/
/*!
    \Function QosCommonStringToAddr

    \Description
        Convert a string generated by QosCommonAddrToString into a QosCommonAddrT

    \Input *pStrIn      - text to convert
    \Input *pOutAddr    - [out] address to write to

    \Output
        int32_t   - -1 on err, 0 on success

    \Version 12/09/2016 (cvienneau)
*/
/********************************************************************************F*/
int32_t QosCommonStringToAddr(char *pStrIn, QosCommonAddrT *pOutAddr)
{
    int32_t iRet = -1;
    int32_t iCurrentToken = 0;
    char *pSave = NULL;
    char *pTokenTemp = NULL;
    char *pToken0 = NULL;
    char *pToken1 = NULL;
    char *pToken2 = NULL;

    ds_memclr(pOutAddr, sizeof(QosCommonAddrT));
    pTokenTemp = ds_strtok_r(pStrIn, "[]", &pSave);
    while (pTokenTemp != NULL)
    {
        //process the token into a variable based off the token count
        if (iCurrentToken == 0)
        {
            pToken0 = pTokenTemp;
        }
        else if (iCurrentToken == 1)
        {
            pToken1 = pTokenTemp;
        }
        else if (iCurrentToken == 2)
        {
            pToken2 = pTokenTemp;
        }
        pTokenTemp = ds_strtok_r(NULL, "[]", &pSave);
        iCurrentToken++;
    }

    if ((pToken0 != NULL) && (ds_strnicmp(pToken0, "v4", 2) == 0))
    {   
        pOutAddr->uFamily = AF_INET;
        if (pToken1 != NULL)
        {
            struct sockaddr tempAddr;  //doing this so i can make use of SockaddrInSetAddrText
            SockaddrInit(&tempAddr, AF_INET);
            SockaddrInSetAddrText(&tempAddr, pToken1);
            pOutAddr->addr.v4 = SockaddrInGetAddr(&tempAddr);
            iRet = 0;
        }
        if (pToken2 != NULL)
        {
            pOutAddr->uPort = atoi(pToken2 + 1);   // +1 to move past the ':' character
        }
    }
    #ifndef DIRTYCODE_NX
    else if ((pToken0 != NULL) && (ds_strnicmp(pToken0, "v6", 2) == 0))
    {
        pOutAddr->uFamily = AF_INET6;
        if (pToken1 != NULL)
        {
            struct sockaddr_in6 tempAddr;  //doing this so i can make use of SockaddrInSetAddrText
            SockaddrInit6(&tempAddr, AF_INET6);
            SockaddrInSetAddrText((struct sockaddr *)&tempAddr, pToken1);    //this will translate the string into the bytes it needs to be
            ds_memcpy(&(pOutAddr->addr.v6), &tempAddr.sin6_addr, sizeof(pOutAddr->addr.v6));    //write the bytes into our structure
            iRet = 0;
        }
        if (pToken2 != NULL)
        {
            pOutAddr->uPort = atoi(pToken2 + 1);   // +1 to move past the ':' character
        }
    }
    #endif
    return(iRet);
}
/*F********************************************************************************/
/*!
    \Function QosCommonConvertAddr

    \Description
        Determine if pSourceAddr is remapped, if so fetch the real info and 
        copy the common fields from pSourceAddr to pTargetAddr.

    \Input *pTargetAddr  - [out] structure to store the fields we are interested in 
    \Input *pSourceAddr  - address to copy from

    \Version 06/27/2017 (cvienneau)
*/
/********************************************************************************F*/
void QosCommonConvertAddr(QosCommonAddrT *pTargetAddr, struct sockaddr *pSourceAddr)
{
    #ifndef DIRTYCODE_NX
    struct sockaddr_in6 SockAddr6;
    
    //check to see if the ipv4 address coming in is a remapped ipv6 addr, if it is get the real ipv6 info
    if (pSourceAddr->sa_family == AF_INET)
    {
        uint32_t uAddr = SockaddrInGetAddr(pSourceAddr);
        if (SocketInfo(NULL, '?ip6', uAddr, &SockAddr6, sizeof(SockAddr6)) == 1)
        {
            pSourceAddr = (struct sockaddr *)&SockAddr6; //it is an ipv6 remapped addr, use the ipv6 info below.
        }
    }
    #endif

    pTargetAddr->uFamily = pSourceAddr->sa_family;

    if (pTargetAddr->uFamily == AF_INET)
    {
        pTargetAddr->addr.v4 = SockaddrInGetAddr(pSourceAddr);
        pTargetAddr->uPort = SockaddrInGetPort(pSourceAddr);
    }
    
    #ifndef DIRTYCODE_NX
    else if (pTargetAddr->uFamily == AF_INET6)
    {
        struct sockaddr_in6 * pAddr6 = (struct sockaddr_in6 *)pSourceAddr;
        ds_memcpy(&(pTargetAddr->addr.v6), &pAddr6->sin6_addr, sizeof(pTargetAddr->addr.v6));
        pTargetAddr->uPort = pAddr6->sin6_port;
    }
    #endif
}


/*F********************************************************************************/
/*!
    \Function QosCommonIsAddrEqual

    \Description
        Compare if two addresses belong to the same machine (ignore port).

    \Input *pAddr1  - first address to compare
    \Input *pAddr2  - second address to compare

    \Output
        uint8_t     - TRUE if addresses are equal

    \Version 12/09/2016 (cvienneau)
*/
/********************************************************************************F*/
uint8_t QosCommonIsAddrEqual(QosCommonAddrT *pAddr1, QosCommonAddrT *pAddr2)
{
    if (pAddr1->uFamily == pAddr2->uFamily)
    {
        if (pAddr1->uFamily == AF_INET)
        {
            if (pAddr1->addr.v4 == pAddr2->addr.v4)
            {
                return(TRUE);
            }
        }
        #ifndef DIRTYCODE_NX
        else if (pAddr1->uFamily == AF_INET6)
        {
            //just compare the address portion of the struct
            if (memcmp(&pAddr1->addr.v6,
                       &pAddr2->addr.v6,
                 sizeof(pAddr1->addr.v6)) == 0)
            {
                return(TRUE);
            }
        }
        #endif
    }
      
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function QosCommonIsRemappedAddrEqual

    \Description
        Retrieve the address remap for pAddr1 if it exists (would be the case for ipv6)
        and compare it to the v4 or already remapped pAddr2 to see if they belong to the same
        machine.

    \Input *pAddr1  - first address to compare
    \Input *pAddr2  - second address to compare

    \Output
        uint8_t     - TRUE if addresses are equal

    \Version 12/09/2016 (cvienneau)
*/
/********************************************************************************F*/
uint8_t QosCommonIsRemappedAddrEqual(QosCommonAddrT *pAddr1, struct sockaddr *pAddr2)
{
    uint32_t uAddr1v4;
    uint32_t uAddr2v4;

    if (pAddr2->sa_family != AF_INET)
    {
        return(FALSE); //we expect this address to be ipv4 or a remapped ipv6, so we shouldn't see anything other than AF_INET
    }
    uAddr2v4 = SockaddrInGetAddr(pAddr2);

    if (pAddr1->uFamily == AF_INET)
    {
        uAddr1v4 = pAddr1->addr.v4;
    }
    #ifndef DIRTYCODE_NX
    else if (pAddr1->uFamily == AF_INET6)
    {
        struct sockaddr_in6 addr6;
        SockaddrInit6(&addr6, AF_INET6);
        ds_memcpy(&addr6.sin6_addr, &(pAddr1->addr.v6), sizeof(addr6.sin6_addr));
        addr6.sin6_port = pAddr1->uPort;
        uAddr1v4 = SocketControl(NULL, '+ip6', sizeof(addr6), &addr6, NULL);
        //todo when will things be removed from the address map?
    }
    #endif
    else
    {
        return(FALSE); // we don't know the type, they're not equal
    }

    if (uAddr1v4 == uAddr2v4)
    {
        return(TRUE);
    }

    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function QosCommonSerializeProbePacket

    \Description
        Write the contents of a QosCommonProbePacketT struct to a buffer, sign it for authenticity
        in preparation for sending it on the wire.

    \Input *pOutBuff  - [out] buffer the packet will be written to
    \Input uBuffSize  - size of the buffer the packet is being written to, must be at least QOS_COMMON_MIN_PACKET_SIZE
    \Input *pInPacket - the structure containing the probe packet information
    \Input *pSecureKey- the secure key used to sign the probe as being authentic

    \Output
        uint8_t     - number of bytes written to the buffer

    \Version 07/06/2017 (cvienneau)
*/
/********************************************************************************F*/
uint8_t QosCommonSerializeProbePacket(uint8_t *pOutBuff, uint32_t uBuffSize, const QosCommonProbePacketT *pInPacket, uint8_t *pSecureKey)
{
    /*
    This is the order we are going to serialize in
    uint32_t uProtocol;                 //!< QOS 2.0 packets will always contain 'qos2' for easy identification.
    uint16_t uVersion;                  //!< uniquely identifies protocol
    uint32_t uServiceRequestId;         //!< provided by the QosCoordinator, unique to the client doing multiple QOS actions, used to identify which server resources this client is using
    uint16_t uClientRequestId;          //!< provided by the client, unique to a particular QOS action, used to pair request and responses together
    uint32_t uServerReceiveTime;        //!< time the server received this probe from the client
    uint16_t uServerSendDelta;          //!< delta before the server sent this probe response
    uint16_t uProbeSizeUp;              //!< indicates how big this probe is, including any padding for bandwidth
    uint16_t uProbeSizeDown;            //!< indicates how big this probe is, including any padding for bandwidth
    uint8_t uProbeCountUp;              //!< count index of this probe
    uint8_t uProbeCountDown;            //!< count index of this probe
    QosCommonAddrT clientAddressFromService;  //!< initially the client address from coordinator, however address from server prospective takes precedence, used to authenticate the packet is coming from the address that generated it
    uint8_t aHmac[QOS_COMMON_HMAC_SIZE];//!< when combined with the secure key identifies this packet as coming from a real QOS client
    */
    if (uBuffSize >= QOS_COMMON_MIN_PACKET_SIZE)
    {
        uint32_t uTemp32;
        uint16_t uTemp16;
        uint8_t *pWrite = pOutBuff;

        uTemp32 = SocketHtonl(pInPacket->uProtocol);                                    ds_memcpy(pWrite, &uTemp32, sizeof(uint32_t));  pWrite += sizeof(uint32_t);     //4
        uTemp16 = SocketHtons(pInPacket->uVersion);                                     ds_memcpy(pWrite, &uTemp16, sizeof(uint16_t));  pWrite += sizeof(uint16_t);     //2
        uTemp32 = SocketHtonl(pInPacket->uServiceRequestId);                            ds_memcpy(pWrite, &uTemp32, sizeof(uint32_t));  pWrite += sizeof(uint32_t);     //4
        uTemp16 = SocketHtons(pInPacket->uClientRequestId);                             ds_memcpy(pWrite, &uTemp16, sizeof(uint16_t));  pWrite += sizeof(uint16_t);     //2
        uTemp32 = SocketHtonl(pInPacket->uServerReceiveTime);                           ds_memcpy(pWrite, &uTemp32, sizeof(uint32_t));  pWrite += sizeof(uint32_t);     //4
        uTemp16 = SocketHtons(pInPacket->uServerSendDelta);                             ds_memcpy(pWrite, &uTemp16, sizeof(uint16_t));  pWrite += sizeof(uint16_t);     //2
        uTemp16 = SocketHtons(pInPacket->uProbeSizeUp);                                 ds_memcpy(pWrite, &uTemp16, sizeof(uint16_t));  pWrite += sizeof(uint16_t);     //2
        uTemp16 = SocketHtons(pInPacket->uProbeSizeDown);                               ds_memcpy(pWrite, &uTemp16, sizeof(uint16_t));  pWrite += sizeof(uint16_t);     //2
        /*pInPacket->uProbeCountUp;*/                                   ds_memcpy(pWrite, &pInPacket->uProbeCountUp, sizeof(uint8_t));  pWrite += sizeof(uint8_t);      //1
        /*pInPacket->uProbeCountDown;*/                               ds_memcpy(pWrite, &pInPacket->uProbeCountDown, sizeof(uint8_t));  pWrite += sizeof(uint8_t);      //1   
        /*pInPacket->uExpectedProbeCountUp;*/                   ds_memcpy(pWrite, &pInPacket->uExpectedProbeCountUp, sizeof(uint8_t));  pWrite += sizeof(uint8_t);      //1   
        uTemp16 = SocketHtons(pInPacket->clientAddressFromService.uFamily);             ds_memcpy(pWrite, &uTemp16, sizeof(uint16_t));  pWrite += sizeof(uint16_t);     //2
        uTemp16 = SocketHtons(pInPacket->clientAddressFromService.uPort);               ds_memcpy(pWrite, &uTemp16, sizeof(uint16_t));  pWrite += sizeof(uint16_t);     //2
        uTemp32 = SocketHtonl(pInPacket->clientAddressFromService.addr.v6.aDwords[0]);  ds_memcpy(pWrite, &uTemp32, sizeof(uint32_t));  pWrite += sizeof(uint32_t);     //4
        uTemp32 = SocketHtonl(pInPacket->clientAddressFromService.addr.v6.aDwords[1]);  ds_memcpy(pWrite, &uTemp32, sizeof(uint32_t));  pWrite += sizeof(uint32_t);     //4
        uTemp32 = SocketHtonl(pInPacket->clientAddressFromService.addr.v6.aDwords[2]);  ds_memcpy(pWrite, &uTemp32, sizeof(uint32_t));  pWrite += sizeof(uint32_t);     //4
        uTemp32 = SocketHtonl(pInPacket->clientAddressFromService.addr.v6.aDwords[3]);  ds_memcpy(pWrite, &uTemp32, sizeof(uint32_t));  pWrite += sizeof(uint32_t);     //4
                                                                                                                                                                        //total 45 bytes == QOS_COMMON_SIZEOF_PROBE_DATA
        //generate hmac, it is just bytes, we don't need to hton them     
        //hash the probe values with the secure key to sign the probe as being authentic
        CryptHmacCalc(pWrite, QOS_COMMON_HMAC_SIZE, pOutBuff, QOS_COMMON_SIZEOF_PROBE_DATA, pSecureKey, QOS_COMMON_SECURE_KEY_LENGTH, QOS_COMMON_HMAC_TYPE);
        pWrite += QOS_COMMON_HMAC_SIZE;
        return(pWrite - pOutBuff);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function QosCommonDeserializeClientRequestId

    \Description
        Read the uClientRequestId from the packet without doing any validation.

    \Input *pInBuff  - buffer the packet will be read from 

    \Output
        uint16_t     - the uClientRequestId field from a serialized QosCommonProbePacketT

    \Version 07/06/2017 (cvienneau)
    */
/********************************************************************************F*/
uint16_t QosCommonDeserializeClientRequestId(uint8_t *pInBuff)
{
    //uClientRequestId is the 10'th byte in the serialized packet
    return(SocketNtohs(*(uint16_t*)(pInBuff+10)));
}

/*F********************************************************************************/
/*!
    \Function QosCommonDeserializeServiceRequestId

    \Description
        Read the uServiceRequestId from the packet without doing any validation.

    \Input *pInBuff  - buffer the packet will be read from to

    \Output
        uint32_t     - the uServiceRequestId field from a serialized QosCommonProbePacketT

    \Version 07/06/2017 (cvienneau)
*/
/********************************************************************************F*/
uint32_t QosCommonDeserializeServiceRequestId(uint8_t *pInBuff)
{
    //uServiceRequestId is the 6'th byte in the serialized packet
    return(SocketNtohl(*(uint32_t*)(pInBuff + 6)));
}

/*F********************************************************************************/
/*!
    \Function QosCommonDeserializeProbePacket

    \Description
        Authenticate and read a QosCommonProbePacketT from the provided buffer.

    \Input *pOutPacket  - [out] struct to contain the deserailzed probe
    \Input *pInBuff     - buffer the probe is read from
    \Input *pSecureKey1 - secure key used to authenticate the probe data
    \Input *pSecureKey2 - optional alternative, secure key used to authenticate the probe data

    \Output
        uint32_t     - 0=first secure key succeeded, 1=second secure key succeeded, else failure

    \Version 07/06/2017 (cvienneau)
*/
/********************************************************************************F*/
uint8_t QosCommonDeserializeProbePacket(QosCommonProbePacketT *pOutPacket, uint8_t *pInBuff, uint8_t *pSecureKey1, uint8_t *pSecureKey2)
{
    uint8_t uRet = 0;

#if QOS_COMMON_ENABLE_HMAC
    uint8_t aHmac[QOS_COMMON_HMAC_SIZE];

    //validate the hmac, which is at the end of the packet
    //generate what we think the hmac should be
    CryptHmacCalc(aHmac, QOS_COMMON_HMAC_SIZE, pInBuff, QOS_COMMON_SIZEOF_PROBE_DATA, pSecureKey1, QOS_COMMON_SECURE_KEY_LENGTH, QOS_COMMON_HMAC_TYPE);
    if (memcmp(aHmac, pInBuff + QOS_COMMON_SIZEOF_PROBE_DATA, QOS_COMMON_HMAC_SIZE) != 0)
    {
        //first secure key failed, try with the backup secure key if one was provided
        if (pSecureKey2 != NULL)
        {
            CryptHmacCalc(aHmac, QOS_COMMON_HMAC_SIZE, pInBuff, QOS_COMMON_SIZEOF_PROBE_DATA, pSecureKey2, QOS_COMMON_SECURE_KEY_LENGTH, QOS_COMMON_HMAC_TYPE);
            if (memcmp(aHmac, pInBuff + QOS_COMMON_SIZEOF_PROBE_DATA, QOS_COMMON_HMAC_SIZE) != 0)
            {
                return(2);  //failed authentication
            }
            else
            {
                uRet = 1;   //first hmac didn't succeed but second one did
            }
        }
        else
        {
            return(2);  //failed authentication
        }
    }
#endif

    QosCommonDeserializeProbePacketInsecure(pOutPacket, pInBuff);
    return(uRet);
}

/*F********************************************************************************/
/*!
    \Function QosCommonDeserializeProbePacketInsecure

    \Description
        Read a QosCommonProbePacketT from the provided buffer.

    \Input *pOutPacket  - [out] struct to contain the deserailzed probe
    \Input *pInBuff     - buffer the probe is read from

    \Version 07/06/2017 (cvienneau)
*/
/********************************************************************************F*/
void QosCommonDeserializeProbePacketInsecure(QosCommonProbePacketT *pOutPacket, uint8_t *pInBuff)
{
    /*
    This is the order we are going to deserialize from
    uint32_t uProtocol;                 //!< QOS 2.0 packets will always contain 'qos2' for easy identification.
    uint16_t uVersion;                  //!< uniquely identifies protocol
    uint32_t uServiceRequestId;         //!< provided by the QosCoordinator, unique to the client doing multiple QOS actions, used to identify which server resources this client is using
    uint16_t uClientRequestId;          //!< provided by the client, unique to a particular QOS action, used to pair request and responses together
    uint32_t uServerReceiveTime;        //!< time the server received this probe from the client
    uint32_t uServerSendDelta;          //!< delta before the server sent this probe response
    uint16_t uProbeSizeUp;              //!< indicates how big this probe is, including any padding for bandwidth
    uint16_t uProbeSizeDown;            //!< indicates how big this probe is, including any padding for bandwidth
    uint8_t uProbeCountUp;              //!< count index of this probe
    uint8_t uProbeCountDown;            //!< count index of this probe
    QosCommonAddrT clientAddressFromService;  //!< initially the client address from coordinator, however address from server prospective takes precedence, used to authenticate the packet is coming from the address that generated it
    uint8_t aHmac[QOS_COMMON_HMAC_SIZE];//!< when combined with the secure key identifies this packet as coming from a real QOS client
    */
    pOutPacket->uProtocol = SocketNtohl(*(uint32_t*)pInBuff);                                       pInBuff += sizeof(uint32_t);    //4
    pOutPacket->uVersion = SocketNtohs(*(uint16_t*)pInBuff);                                        pInBuff += sizeof(uint16_t);    //2
    pOutPacket->uServiceRequestId = SocketNtohl(*(uint32_t*)pInBuff);                               pInBuff += sizeof(uint32_t);    //4
    pOutPacket->uClientRequestId = SocketNtohs(*(uint16_t*)pInBuff);                                pInBuff += sizeof(uint16_t);    //2
    pOutPacket->uServerReceiveTime = SocketNtohl(*(uint32_t*)pInBuff);                              pInBuff += sizeof(uint32_t);    //4
    pOutPacket->uServerSendDelta = SocketNtohl(*(uint16_t*)pInBuff);                                pInBuff += sizeof(uint16_t);    //2
    pOutPacket->uProbeSizeUp = SocketNtohs(*(uint16_t*)pInBuff);                                    pInBuff += sizeof(uint16_t);    //2
    pOutPacket->uProbeSizeDown = SocketNtohs(*(uint16_t*)pInBuff);                                  pInBuff += sizeof(uint16_t);    //2
    pOutPacket->uProbeCountUp = *pInBuff;                                                           pInBuff += sizeof(uint8_t);     //1
    pOutPacket->uProbeCountDown = *pInBuff;                                                         pInBuff += sizeof(uint8_t);     //1
    pOutPacket->uExpectedProbeCountUp = *pInBuff;                                                   pInBuff += sizeof(uint8_t);     //1
    pOutPacket->clientAddressFromService.uFamily = SocketNtohs(*(uint16_t*)pInBuff);                pInBuff += sizeof(uint16_t);    //2
    pOutPacket->clientAddressFromService.uPort = SocketNtohs(*(uint16_t*)pInBuff);                  pInBuff += sizeof(uint16_t);    //2
    pOutPacket->clientAddressFromService.addr.v6.aDwords[0] = SocketNtohl(*(uint32_t*)pInBuff);     pInBuff += sizeof(uint32_t);    //4
    pOutPacket->clientAddressFromService.addr.v6.aDwords[1] = SocketNtohl(*(uint32_t*)pInBuff);     pInBuff += sizeof(uint32_t);    //4
    pOutPacket->clientAddressFromService.addr.v6.aDwords[2] = SocketNtohl(*(uint32_t*)pInBuff);     pInBuff += sizeof(uint32_t);    //4
    pOutPacket->clientAddressFromService.addr.v6.aDwords[3] = SocketNtohl(*(uint32_t*)pInBuff);     pInBuff += sizeof(uint32_t);    //4
}

/*F********************************************************************************/
/*!
    \Function QosCommonMakeVersion

    \Description
        Make a version number out of a major and minor version parts

    \Input uMajor  - major byte, indicates non-backwards compatible changes
    \Input uMinor  - minor byte, indicates bug fixes

    \Output
        uint16_t    - 2 bytes representing a version.

    \Version 12/09/2016 (cvienneau)
*/
/********************************************************************************F*/
uint16_t QosCommonMakeVersion(uint8_t uMajor, uint8_t uMinor)
{
    uint16_t uVersion = (uMajor << 8) + uMinor;
    return(uVersion);
}

/*F********************************************************************************/
/*!
    \Function QosCommonGetVersion

    \Description
        Split a version number into major and minor version parts

    \Input uVersion - full version, to be split into major and minor components
    \Input *uMajor  - [out] major byte, indicates non-backwards compatible changes
    \Input *uMinor  - [out] minor byte, indicates bug fixes

    \Version 12/09/2016 (cvienneau)
*/
/********************************************************************************F*/
void QosCommonGetVersion(uint16_t uVersion, uint8_t *uMajor, uint8_t *uMinor)
{
    *uMajor = (uVersion >> 8);
    *uMinor = (uVersion & 0x00FF);
}

/*F********************************************************************************/
/*!
    \Function QosCommonIsCompatibleVersion

    \Description
        Compare the major portion of two version number to see if they are compatible.

    \Input uVersion1   - first version
    \Input uVersion2   - second version

    \Output
        uint8_t         - TRUE if they are compatible

    \Version 12/09/2016 (cvienneau)
*/
/********************************************************************************F*/
uint8_t QosCommonIsCompatibleVersion(uint16_t uVersion1, uint16_t uVersion2)
{
     uint8_t uMajor1 = (uVersion1 >> 8);
     uint8_t uMajor2 = (uVersion2 >> 8);
     if (uMajor1 == uMajor2)
     {
         return(TRUE);
     }
     return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function QosCommonIsCompatibleProbeVersion

    \Description
        Compare if probes of the passed in version are compatible with our version of code.

    \Input uVersion - version of probe

    \Output
        uint8_t    - TRUE if they are compatible

    \Version 12/09/2016 (cvienneau)
*/
/********************************************************************************F*/
uint8_t QosCommonIsCompatibleProbeVersion(uint16_t uVersion)
{
    uint8_t uMajor = (uVersion >> 8);
    if (uMajor == QOS_COMMON_PROBE_VERSION_MAJOR)
    {
        return(TRUE);
    }
    return(FALSE);
}
