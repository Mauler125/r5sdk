/*H********************************************************************************/
/*!
    \File http2.c

    \Description
        Test the ProtoHttp2 client

    \Copyright
        Copyright (c) 2016 Electronic Arts Inc.

    \Version 12/01/2016 (eesponda)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protohttp2.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/util/protobufcommon.h"
#include "DirtySDK/util/protobufwrite.h"
#include "DirtySDK/util/protobufread.h"
#include "libsample/zmem.h"

#include "testermodules.h"

/*** Defines **********************************************************************/

//! default address for the grpc server we are testing
#define DEFAULT_GRPC_SERVER ("http://10.14.141.208:50051")

/*** Type Definitions *************************************************************/

//! wrapper for the data payloads used for our tests
typedef struct FixedBufferT
{
    int32_t iSize;
    uint8_t aData[256];
} FixedBufferT;

//! generator function for data
typedef FixedBufferT (GenerateFn)(void);

//! handle response function
typedef const uint8_t *(ResponseFn)(const uint8_t *pBuf, int32_t iBufLen);

//! location used for grpc tests
typedef struct LocationT
{
    uint64_t iLatitude;
    uint64_t iLongitude;
} LocationT;

typedef struct Http2RefT
{
    ProtoHttp2RefT *pHttp;  //!< http2 module ref
    int32_t iStreamId;      //!< main stream identifier

    char strGrpcHost[32];   //!< the default host to run grpc tests

    uint8_t *pBuf;          //!< buffer for reading data
    int32_t iBufSize;       //!< size of the buffer

    FixedBufferT Buffer;    //!< buffer for sending data
    int32_t iDataSent;      //!< how much data we have sent

    GenerateFn *pGen;       //!< streaming generator fn
    ResponseFn *pResp;      //!< response handling fn
    int32_t iIndex;         //!< index of the data for client/bi-directional sends
    int32_t iMaxIndex;      //!< max index of data

    uint32_t uTimer;        //!< timer to gauge operations
    int32_t iCount;         //!< number of bytes downloaded
    int32_t iShow;          //!< current bytes show in logging
} Http2RefT;

/*** Variables ********************************************************************/

// instance for used in testing
static Http2RefT _Http2 = { NULL, 0, DEFAULT_GRPC_SERVER, NULL, 0, { 0, { 0 } }, 0, NULL, NULL, 0, 0, 0, 0, 0 };

//! list of locations use for RecordRoute RPC
static const LocationT _aFeatureLocations[] =
{
    { 407838351, (uint64_t)-746143763 },
    { 408122808, (uint64_t)-743999179 },
    { 413628156, (uint64_t)-749015468 },
    { 419999544, (uint64_t)-740371136 },
    { 414008389, (uint64_t)-743951297 },
    { 419611318, (uint64_t)-746524769 },
    { 406109563, (uint64_t)-742186778 },
    { 416802456, (uint64_t)-742370183 },
    { 412950425, (uint64_t)-741077389 },
    { 412144655, (uint64_t)-743949739 },
    { 415736605, (uint64_t)-742847522 },
    { 413843930, (uint64_t)-740501726 },
    { 410873075, (uint64_t)-744459023 },
    { 412346009, (uint64_t)-744026814 },
    { 402948455, (uint64_t)-747903913 },
    { 406337092, (uint64_t)-740122226 },
    { 406421967, (uint64_t)-747727624 },
    { 416318082, (uint64_t)-749677716 },
    { 415301720, (uint64_t)-748416257 },
    { 402647019, (uint64_t)-747071791 },
    { 412567807, (uint64_t)-741058078 },
    { 416855156, (uint64_t)-744420597 },
    { 404663628, (uint64_t)-744820157 },
    { 407113723, (uint64_t)-749746483 },
    { 402133926, (uint64_t)-743613249 },
    { 400273442, (uint64_t)-741220915 },
    { 411236786, (uint64_t)-744070769 },
    { 411633782, (uint64_t)-746784970 },
    { 415830701, (uint64_t)-742952812 },
    { 413447164, (uint64_t)-748712898 },
    { 405047245, (uint64_t)-749800722 },
    { 418858923, (uint64_t)-746156790 },
    { 417951888, (uint64_t)-748484944 },
    { 407033786, (uint64_t)-743977337 },
    { 417548014, (uint64_t)-740075041 },
    { 410395868, (uint64_t)-744972325 },
    { 404615353, (uint64_t)-745129803 },
    { 406589790, (uint64_t)-743560121 },
    { 414653148, (uint64_t)-740477477 },
    { 405957808, (uint64_t)-743255336 },
    { 411733589, (uint64_t)-741648093 },
    { 412676291, (uint64_t)-742606606 },
    { 409224445, (uint64_t)-748286738 },
    { 406523420, (uint64_t)-742135517 },
    { 401827388, (uint64_t)-740294537 },
    { 410564152, (uint64_t)-743685054 },
    { 408472324, (uint64_t)-740726046 },
    { 412452168, (uint64_t)-740214052 },
    { 409146138, (uint64_t)-746188906 },
    { 404701380, (uint64_t)-744781745 },
    { 409642566, (uint64_t)-746017679 },
    { 408031728, (uint64_t)-748645385 },
    { 413700272, (uint64_t)-742135189 },
    { 404310607, (uint64_t)-740282632 },
    { 409319800, (uint64_t)-746201391 },
    { 406685311, (uint64_t)-742108603 },
    { 419018117, (uint64_t)-749142781 },
    { 412856162, (uint64_t)-745148837 },
    { 416560744, (uint64_t)-746721964 },
    { 405314270, (uint64_t)-749836354 },
    { 414219548, (uint64_t)-743327440 },
    { 415534177, (uint64_t)-742900616 },
    { 406898530, (uint64_t)-749127080 },
    { 407586880, (uint64_t)-741670168 },
    { 400106455, (uint64_t)-742870190 },
    { 400066188, (uint64_t)-746793294 },
    { 418803880, (uint64_t)-744102673 },
    { 414204288, (uint64_t)-747895140 },
    { 414777405, (uint64_t)-740615601 },
    { 415464475, (uint64_t)-747175374 },
    { 404062378, (uint64_t)-746376177 },
    { 405688272, (uint64_t)-749285130 },
    { 400342070, (uint64_t)-748788996 },
    { 401809022, (uint64_t)-744157964 },
    { 404226644, (uint64_t)-740517141 },
    { 410322033, (uint64_t)-747871659 },
    { 407100674, (uint64_t)-747742727 },
    { 418811433, (uint64_t)-741718005 },
    { 415034302, (uint64_t)-743850945 },
    { 411349992, (uint64_t)-743694161 },
    { 404839914, (uint64_t)-744759616 },
    { 414638017, (uint64_t)-745957854 },
    { 412127800, (uint64_t)-740173578 },
    { 401263460, (uint64_t)-747964303 },
    { 412843391, (uint64_t)-749086026 },
    { 418512773, (uint64_t)-743067823 },
    { 404318328, (uint64_t)-740835638 },
    { 419020746, (uint64_t)-741172328 },
    { 404080723, (uint64_t)-746119569 },
    { 401012643, (uint64_t)-744035134 },
    { 404306372, (uint64_t)-741079661 },
    { 403966326, (uint64_t)-748519297 },
    { 405002031, (uint64_t)-748407866 },
    { 409532885, (uint64_t)-742200683 },
    { 416851321, (uint64_t)-742674555 },
    { 406411633, (uint64_t)-741722051 },
    { 413069058, (uint64_t)-744597778 },
    { 418465462, (uint64_t)-746859398 },
    { 411733222, (uint64_t)-744228360 },
    { 410248224, (uint64_t)-747127767 }
};

/*** Private functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _CmdHttp2WriteGrpcHeader

    \Description
        Encodes the required size of the message into the buffer

    \Input *pEncoder    - the message encoder
    \Input *pPayload    - the payload the message was encoded to

    \Version 07/05/2017 (eesponda)
*/
/********************************************************************************F*/
static void _CmdHttp2WriteGrpcHeader(ProtobufWriteRefT *pEncoder, FixedBufferT *pPayload)
{
    pPayload->iSize = ProtobufWriteDestroy(pEncoder);
    // add extra for compression byte
    pPayload->iSize += 1;
}

/*F********************************************************************************/
/*!
    \Function _CmdHttp2GetFeature

    \Description
        Encodes the payload for the GetFeature RPC

    \Output
        FixedBufferT    - the buffer we have encoded

    \Version 07/05/2017 (eesponda)
*/
/********************************************************************************F*/
static FixedBufferT _CmdHttp2GetFeature(void)
{
    FixedBufferT Payload;
    ProtobufWriteRefT *pEncoder;

    /*
        rpc GetFeature(Point) returns (Feature) {}

        message Point {
            int32 latitude = 1;
            int32 longitude = 2;
        }
    */

    ds_memclr(&Payload, sizeof(Payload));
    pEncoder = ProtobufWriteCreate(Payload.aData+1, sizeof(Payload.aData)-1, TRUE);

    ProtobufWriteVarint(pEncoder,  409146138, 1);
    ProtobufWriteVarint(pEncoder, (uint64_t)-746188906, 2);

    // write the header
    _CmdHttp2WriteGrpcHeader(pEncoder, &Payload);

    return(Payload);
}

/*F********************************************************************************/
/*!
    \Function _CmdHttp2GetFeatureResponse

    \Description
        Handles the response for the GetFeature RPC

    \Input *pBuffer     - payload
    \Input iBufLen      - size of payload

    \Output
        const uint8_t * - new buffer location past the response

    \Version 07/05/2017 (eesponda)
*/
/********************************************************************************F*/
static const uint8_t *_CmdHttp2GetFeatureResponse(const uint8_t *pBuffer, int32_t iBufLen)
{
    int32_t iMsgSize;
    ProtobufReadT Reader, Msg;
    struct {
        char strName[256];
        LocationT Point;
    } Response;

    ds_memset(&Response, -1, sizeof(Response));

    /*
        rpc GetFeature(Point) returns (Feature) {}

        message Point {
            int32 latitude = 1;
            int32 longitude = 2;
        }

        message Feature {
            string name = 1;
            Point location = 2;
        }
    */

    // get message size (skipping compression)
    pBuffer = ProtobufCommonReadSize(pBuffer+1, iBufLen-1, &iMsgSize);
    ProtobufReadInit(&Reader, pBuffer, iMsgSize);

    ProtobufReadString(&Reader, ProtobufReadFind(&Reader, 1), Response.strName, sizeof(Response.strName));
    if (ProtobufReadMessage(&Reader, ProtobufReadFind(&Reader, 2), &Msg) != NULL)
    {
        Response.Point.iLatitude = ProtobufReadVarint(&Msg, ProtobufReadFind(&Msg, 1));
        Response.Point.iLongitude = ProtobufReadVarint(&Msg, ProtobufReadFind(&Msg, 2));
    }

    ZPrintf("http2: name (%s), location (%d/%d)\n", Response.strName, Response.Point.iLatitude, Response.Point.iLongitude);
    return(pBuffer+iMsgSize);
}

/*F********************************************************************************/
/*!
    \Function _CmdHttp2ListFeatures

    \Description
        Encodes the payload for the ListFeatures RPC

    \Output
        FixedBufferT    - the buffer we have encoded

    \Version 07/05/2017 (eesponda)
*/
/********************************************************************************F*/
static FixedBufferT _CmdHttp2ListFeatures(void)
{
    /*
        rpc ListFeatures(Rectangle) returns (stream Feature) {}

        message Rectangle {
            Point lo = 1;
            Point hi = 2;
        }
    */

    FixedBufferT Payload;
    ProtobufWriteRefT *pEncoder;

    ds_memclr(&Payload, sizeof(Payload));
    pEncoder = ProtobufWriteCreate(Payload.aData+1, sizeof(Payload.aData)-1, TRUE);

    ProtobufWriteMessageBegin(pEncoder, 1);
    ProtobufWriteVarint(pEncoder, 400000000, 1);
    ProtobufWriteVarint(pEncoder, (uint64_t)-750000000, 2);
    ProtobufWriteMessageEnd(pEncoder);

    ProtobufWriteMessageBegin(pEncoder, 2);
    ProtobufWriteVarint(pEncoder, 420000000, 1);
    ProtobufWriteVarint(pEncoder, (uint64_t)-730000000, 2);
    ProtobufWriteMessageEnd(pEncoder);

    // write the header
    _CmdHttp2WriteGrpcHeader(pEncoder, &Payload);

    return(Payload);
}

/*F********************************************************************************/
/*!
    \Function _CmdHttp2ListFeaturesResponse

    \Description
        Handles the response for the ListFeatures RPC

    \Input *pBuffer     - payload
    \Input iBufLen      - size of payload

    \Output
        const uint8_t * - new buffer location past the response

    \Version 07/05/2017 (eesponda)
*/
/********************************************************************************F*/
static const uint8_t *_CmdHttp2ListFeaturesResponse(const uint8_t *pBuffer, int32_t iBufLen)
{
    int32_t iMsgSize;
    ProtobufReadT Reader, Msg;

    struct {
        char strName[256];
        LocationT Point;
    } Response;

    /*
        rpc ListFeatures(Rectangle) returns (stream Feature) {}

        message Point {
            int32 latitude = 1;
            int32 longitude = 2;
        }

        message Feature {
            string name = 1;
            Point location = 2;
        }
    */

    // get message size (skipping compression)
    pBuffer = ProtobufCommonReadSize(pBuffer+1, iBufLen-1, &iMsgSize);
    ProtobufReadInit(&Reader, pBuffer, iMsgSize);

    ProtobufReadString(&Reader, ProtobufReadFind(&Reader, 1), Response.strName, sizeof(Response.strName));
    if (ProtobufReadMessage(&Reader, ProtobufReadFind(&Reader, 2), &Msg) != NULL)
    {
        Response.Point.iLatitude = ProtobufReadVarint(&Msg, ProtobufReadFind(&Msg, 1));
        Response.Point.iLongitude = ProtobufReadVarint(&Msg, ProtobufReadFind(&Msg, 2));
    }

    ZPrintf("http2: name (%s), location (%d/%d)\n", Response.strName, Response.Point.iLatitude, Response.Point.iLongitude);
    return(pBuffer+iMsgSize);
}

/*F********************************************************************************/
/*!
    \Function _CmdHttp2RecordRoute

    \Description
        Encodes the payload for the RecordRoute RPC

    \Output
        FixedBufferT    - the buffer we have encoded

    \Version 07/05/2017 (eesponda)
*/
/********************************************************************************F*/
static FixedBufferT _CmdHttp2RecordRoute(void)
{
    /*
        rpc RecordRoute(stream Point) returns (RouteSummary) {}

        message Point {
            int32 latitude = 1;
            int32 longitude = 2;
        }
    */

    FixedBufferT Payload;
    ProtobufWriteRefT *pEncoder;
    const int32_t iIndex = rand() % (sizeof(_aFeatureLocations)/sizeof(_aFeatureLocations[0]));

    ds_memclr(&Payload, sizeof(Payload));
    pEncoder = ProtobufWriteCreate(Payload.aData+1, sizeof(Payload.aData)-1, TRUE);
    ProtobufWriteVarint(pEncoder, _aFeatureLocations[iIndex].iLatitude, 1);
    ProtobufWriteVarint(pEncoder, _aFeatureLocations[iIndex].iLongitude, 2);

    // write the header
    _CmdHttp2WriteGrpcHeader(pEncoder, &Payload);

    return(Payload);
}

/*F********************************************************************************/
/*!
    \Function _CmdHttp2RecordRouteResponse

    \Description
        Handles the response for the RecordRoute RPC

    \Input *pBuffer     - payload
    \Input iBufLen      - size of payload

    \Output
        const uint8_t * - new buffer location past the response

    \Version 07/05/2017 (eesponda)
*/
/********************************************************************************F*/
static const uint8_t *_CmdHttp2RecordRouteResponse(const uint8_t *pBuffer, int32_t iBufLen)
{
    int32_t iMsgSize;
    ProtobufReadT Reader;

    struct {
        int32_t iPointCount;
        int32_t iFeatureCount;
        int32_t iDistance;
        int32_t iElapsedTime;
    } Response;

    /*
        rpc RecordRoute(stream Point) returns (RouteSummary) {}

        message RouteSummary {
            int32 point_count = 1;
            int32 feature_count = 2;
            int32 distance = 3;
            int32 elapsed_time = 4;
        }
    */

    // get message size (skipping compression)
    pBuffer = ProtobufCommonReadSize(pBuffer+1, iBufLen-1, &iMsgSize);
    ProtobufReadInit(&Reader, pBuffer, iMsgSize);

    Response.iPointCount = (int32_t)ProtobufReadVarint(&Reader, ProtobufReadFind(&Reader, 1));
    Response.iFeatureCount = (int32_t)ProtobufReadVarint(&Reader, ProtobufReadFind(&Reader, 2));
    Response.iDistance = (int32_t)ProtobufReadVarint(&Reader, ProtobufReadFind(&Reader, 3));
    Response.iElapsedTime = (int32_t)ProtobufReadVarint(&Reader, ProtobufReadFind(&Reader, 4));

    ZPrintf("http2: pointcount %d, featurecount %d, distance %d, elapsedtime %d\n", Response.iPointCount, Response.iFeatureCount, Response.iDistance, Response.iElapsedTime);
    return(pBuffer+iMsgSize);
}

/*F********************************************************************************/
/*!
    \Function _CmdHttp2RouteChat

    \Description
        Encodes the payload for the RouteChat RPC

    \Output
        FixedBufferT    - the buffer we have encoded

    \Version 07/05/2017 (eesponda)
*/
/********************************************************************************F*/
static FixedBufferT _CmdHttp2RouteChat(void)
{
    /*
        rpc RouteChat(stream RouteNote) returns (stream RouteNote) {}

        message RouteNote {
            Point location = 1;
            string message = 2;
        }
    */

    FixedBufferT Payload = { 0, { 0 } };
    int32_t iIndex;
    const char *aMessages[] = { "First message", "Second message", "Third message", "Fourth message" };
    LocationT aLocations[] = { { 0, 0 }, { 0, 1 }, { 1, 0 }, { 0, 0 } };

    for (iIndex = 0; iIndex < 4; iIndex += 1)
    {
        FixedBufferT Message = { 0, { 0 } };
        ProtobufWriteRefT *pEncoder;

        pEncoder = ProtobufWriteCreate(Message.aData+1, sizeof(Message.aData)-1, TRUE);
        ProtobufWriteMessageBegin(pEncoder, 1);
        ProtobufWriteVarint(pEncoder, aLocations[iIndex].iLatitude, 1);
        ProtobufWriteVarint(pEncoder, aLocations[iIndex].iLongitude, 2);
        ProtobufWriteMessageEnd(pEncoder);
        ProtobufWriteString(pEncoder, aMessages[iIndex], (signed)strlen(aMessages[iIndex]), 2);

        _CmdHttp2WriteGrpcHeader(pEncoder, &Message);

        ds_memcpy(Payload.aData+Payload.iSize, Message.aData, Message.iSize);
        Payload.iSize += Message.iSize;
    }

    return(Payload);
}

/*F********************************************************************************/
/*!
    \Function _CmdHttp2RouteChatResponse

    \Description
        Handles the response for the RouteChat RPC

    \Input *pBuffer     - payload
    \Input iBufLen      - size of payload

    \Output
        const uint8_t * - new buffer location past the response

    \Version 07/05/2017 (eesponda)
*/
/********************************************************************************F*/
static const uint8_t *_CmdHttp2RouteChatResponse(const uint8_t *pBuffer, int32_t iBufLen)
{
    int32_t iMsgSize;
    ProtobufReadT Reader, Msg;

    struct {
        LocationT Point;
        char strMessage[256];
    } Response;

    /*
        rpc RouteChat(stream RouteNote) returns (stream RouteNote) {}

        message RouteNote {
            Point location = 1;
            string message = 2;
        }
    */

    // get message size (skipping compression)
    pBuffer = ProtobufCommonReadSize(pBuffer+1, iBufLen-1, &iMsgSize);
    ProtobufReadInit(&Reader, pBuffer, iMsgSize);

    if (ProtobufReadMessage(&Reader, ProtobufReadFind(&Reader, 1), &Msg) != NULL)
    {
        Response.Point.iLatitude = ProtobufReadVarint(&Msg, ProtobufReadFind(&Msg, 1));
        Response.Point.iLongitude = ProtobufReadVarint(&Msg, ProtobufReadFind(&Msg, 2));
    }
    ProtobufReadString(&Reader, ProtobufReadFind(&Reader, 2), Response.strMessage, sizeof(Response.strMessage));

    ZPrintf("http2: location (%d/%d), message (%s)\n", Response.Point.iLatitude, Response.Point.iLongitude, Response.strMessage);
    return(pBuffer+iMsgSize);
}

/*F********************************************************************************/
/*!
    \Function _CmdHttp2CleaupSend

        Cleans up the module ref for a request send

    \Input *pHttp2  - module ref for this test to cleanup

    \Version 12/01/2016 (eesponda)
*/
/********************************************************************************F*/
static void _CmdHttp2CleaupSend(Http2RefT *pHttp2)
{
    ds_memclr(&pHttp2->Buffer, sizeof(pHttp2->Buffer));
    pHttp2->iDataSent = 0;
    pHttp2->pGen = NULL;
    pHttp2->iMaxIndex = 0;
    pHttp2->iIndex = 0;
}

/*F********************************************************************************/
/*!
    \Function _CmdHttp2IdleCb

    \Description
        Update for the http2 testing module

    \Input *pArgz   - environment
    \Input iArgc    - standard number of arguments
    \Input *pArgv[] - standard arg list

    \Version 12/01/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _CmdHttp2IdleCb(ZContext *pArgz, int32_t iArgc, char *pArgv[])
{
    int32_t iResult;
    Http2RefT *pHttp2 = &_Http2;

    // clean up the ref if needed
    if (iArgc == 0)
    {
        if (pHttp2->pHttp != NULL)
        {
            ProtoHttp2Destroy(pHttp2->pHttp);
            ProtoSSLClrCACerts();
            pHttp2->pHttp = NULL;
        }
        return(0);
    }

    // try to send data
    if (pHttp2->Buffer.iSize > 0)
    {
        if (pHttp2->iDataSent < pHttp2->Buffer.iSize)
        {
            int32_t iDataSent;
            if ((iDataSent = ProtoHttp2Send(pHttp2->pHttp, pHttp2->iStreamId, pHttp2->Buffer.aData+pHttp2->iDataSent, pHttp2->Buffer.iSize-pHttp2->iDataSent)) < 0)
            {
                return(1);
            }
            pHttp2->iDataSent += iDataSent;
        }
        else if (pHttp2->iIndex < pHttp2->iMaxIndex)
        {
            pHttp2->Buffer = pHttp2->pGen();
            pHttp2->iDataSent = 0;
            pHttp2->iIndex += 1;
        }
        else
        {
            _CmdHttp2CleaupSend(pHttp2);
            ProtoHttp2Send(pHttp2->pHttp, pHttp2->iStreamId, NULL, PROTOHTTP2_STREAM_END);
        }
    }

    // try to read data
    if (pHttp2->iStreamId != 0)
    {
        ProtoHttpRequestTypeE eRequestType = ProtoHttp2Status(pHttp2->pHttp, pHttp2->iStreamId, 'rtyp', NULL, 0);

        /* for straight gets we want to use this to measure how fast our downloads are
           other types can be handled as before */
        if (eRequestType == PROTOHTTP_REQUESTTYPE_GET)
        {
            uint8_t strBuf[16*1024];
            while ((iResult = ProtoHttp2Recv(pHttp2->pHttp, pHttp2->iStreamId, strBuf, 1, sizeof(strBuf))) > 0)
            {
                ProtoHttp2Update(pHttp2->pHttp);
                pHttp2->iCount += iResult;
            }

            if (pHttp2->iCount != pHttp2->iShow)
            {
                ZPrintf("http2: downloaded %d bytes\n", pHttp2->iCount);
                pHttp2->iShow = pHttp2->iCount;
            }
            if (iResult == PROTOHTTP2_RECVDONE || iResult == PROTOHTTP2_RECVHEAD)
            {
                int32_t iTickDiff = NetTickDiff(NetTick(), pHttp2->uTimer);

                iResult = ProtoHttp2Status(pHttp2->pHttp, pHttp2->iStreamId, 'head', NULL, 0);
                ZPrintf("http2: 'head' %d\n", iResult);

                iResult = ProtoHttp2Status(pHttp2->pHttp, pHttp2->iStreamId, 'body', NULL, 0);
                ZPrintf("http2: 'body' %d in %.2f seconds (%.3f k/sec)\n", iResult, (float)iTickDiff/1000.0f,
                        ((float)iResult * 1000.0f) / ((float)iTickDiff * 1024.0f));

                iResult = ProtoHttp2Status(pHttp2->pHttp, pHttp2->iStreamId, 'done', NULL, 0);
                ZPrintf("http2: 'done' %s\n", iResult ? "TRUE" : "FALSE");

                ProtoHttp2StreamFree(pHttp2->pHttp, pHttp2->iStreamId);
                pHttp2->iStreamId = 0;
                pHttp2->iCount = pHttp2->iShow = 0;
            }
        }
        else
        {
            uint8_t bDone;
            iResult = ProtoHttp2RecvAll(pHttp2->pHttp, pHttp2->iStreamId, pHttp2->pBuf, pHttp2->iBufSize);
            bDone = iResult != PROTOHTTP2_RECVWAIT && iResult != PROTOHTTP2_RECVBUFF;

            // handle finished
            if (iResult > 0)
            {
                int32_t iBody;
                int32_t iTickDiff = NetTickDiff(NetTick(), pHttp2->uTimer);

                iResult = ProtoHttp2Status(pHttp2->pHttp, pHttp2->iStreamId, 'head', NULL, 0);
                ZPrintf("http2: 'head' %d\n", iResult);

                iBody = ProtoHttp2Status(pHttp2->pHttp, pHttp2->iStreamId, 'body', NULL, 0);
                ZPrintf("http2: 'body' %d in %.2f seconds (%.3f k/sec)\n", iBody, (float)iTickDiff/1000.0f,
                        ((float)iBody * 1000.0f) / ((float)iTickDiff * 1024.0f));

                iResult = ProtoHttp2Status(pHttp2->pHttp, pHttp2->iStreamId, 'done', NULL, 0);
                ZPrintf("http2: 'done' %s\n", iResult ? "TRUE" : "FALSE");

                iResult = ProtoHttp2Status(pHttp2->pHttp, pHttp2->iStreamId, 'hres', NULL, 0);
                ZPrintf("http2: 'hres' 0x%08x\n", iResult);

                // we have received the full response, now we can handle it
                if (pHttp2->pResp != NULL)
                {
                    const uint8_t *pCur = pHttp2->pBuf;
                    const uint8_t *pEnd = pHttp2->pBuf+iBody;
                    while (pCur < pEnd)
                    {
                        pCur = pHttp2->pResp(pCur, (int32_t)(pEnd-pCur));
                    }
                }
            }
            // increase size if needed
            else if (iResult == PROTOHTTP2_RECVBUFF)
            {
                if (pHttp2->iBufSize == 0)
                {
                    pHttp2->pBuf = ZMemAlloc(4096);
                    pHttp2->iBufSize = 4096;
                }
                else
                {
                    uint8_t *pNewBuf = ZMemAlloc(pHttp2->iBufSize * 2);
                    ds_memcpy(pNewBuf, pHttp2->pBuf, pHttp2->iBufSize);

                    ZMemFree(pHttp2->pBuf);
                    pHttp2->pBuf = pNewBuf;
                    pHttp2->iBufSize *= 2;
                }

                ZPrintf("http2: allocated larger buffer: new size %d\n", pHttp2->iBufSize);
            }
            else if ((iResult == PROTOHTTP2_RECVFAIL) || (iResult == PROTOHTTP2_TIMEOUT))
            {
                iResult = ProtoHttp2Status(pHttp2->pHttp, pHttp2->iStreamId, 'hres', NULL, 0);
                ZPrintf("http2: receive failed (0x%08x)\n", iResult);
            }

            // cleanup now that we are done
            if (bDone == TRUE)
            {
                ProtoHttp2StreamFree(pHttp2->pHttp, pHttp2->iStreamId);
                pHttp2->iStreamId = 0;

                ZMemFree(pHttp2->pBuf);
                pHttp2->pBuf = NULL;
                pHttp2->iBufSize = 0;
                pHttp2->pResp = NULL;
            }
        }
    }

    // update the ref
    ProtoHttp2Update(pHttp2->pHttp);

    return(ZCallback(_CmdHttp2IdleCb, 1)); // slow enough to allow us to test abort
}

/*F********************************************************************************/
/*!
    \Function _CmdHttp2WriteCb

    \Description
        Callback when we receive data (if registered)

    \Input *pState      - module state
    \Input *pCbInfo     - information about the request
    \Input *pData       - data we are receiving
    \Input iDataSize    - size of the data
    \Input *pUserData   - user specific data

    \Output
        int32_t         - result of the operation

    \Version 12/01/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _CmdHttp2WriteCb(ProtoHttp2RefT *pState, const ProtoHttp2WriteCbInfoT *pCbInfo, const uint8_t *pData, int32_t iDataSize, void *pUserData)
{
    Http2RefT *pHttp2 = (Http2RefT *)pUserData;
    ZPrintf("received %d\n", iDataSize);

    // if the request is complete then cleanup the stream
    if ((iDataSize == PROTOHTTP2_RECVDONE) || (iDataSize == PROTOHTTP2_RECVFAIL) || (iDataSize == PROTOHTTP2_TIMEOUT))
    {
        ProtoHttp2StreamFree(pState, pCbInfo->iStreamId);
        pHttp2->iStreamId = 0;
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _CmdHttp2CreateGrpcRequest

    \Description
        Wrapper to create grpc request

    \Input *pHttp2      - module state
    \Input *pUri        - the path of the request
    \Input *pPayloadFn  - pointer to function to generate payload data
    \Input iNumPayload  - number of entries in array
    \Input *pResponseFn - pointer to function to handle response data

    \Version 12/01/2016 (eesponda)
*/
/********************************************************************************F*/
static void _CmdHttp2CreateGrpcRequest(Http2RefT *pHttp2, const char *pUri, GenerateFn *pPayloadFn, int32_t iNumPayloads, ResponseFn *pResponseFn)
{
    char strUrl[128];

    // setup url
    ds_snzprintf(strUrl, sizeof(strUrl), "%s/%s", pHttp2->strGrpcHost, pUri);

    // update headers, we set null first to clear the previous headers
    ProtoHttp2Control(pHttp2->pHttp, 0, 'apnd', 0, 0, NULL);
    ProtoHttp2Control(pHttp2->pHttp, 0, 'apnd', 0, 0, (void *)"te: trailers\r\ncontent-type: application/grpc\r\n");

    pHttp2->uTimer = NetTick();

    // create the request
    if (ProtoHttp2Request(pHttp2->pHttp, strUrl, NULL, PROTOHTTP2_STREAM_BEGIN, PROTOHTTP_REQUESTTYPE_POST, &pHttp2->iStreamId) < 0)
    {
        ZPrintf("http2: failed to create request %s\n", pUri);
        return;
    }
    // set the array and num indexies
    pHttp2->pGen = pPayloadFn;
    pHttp2->pResp = pResponseFn;
    pHttp2->iMaxIndex = iNumPayloads;
    // set index to 1 as we always read first entry
    pHttp2->iIndex = 1;
    // read the first entry
    pHttp2->Buffer = pPayloadFn();
}

/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdHttp2

    \Description
        Entrypoint for the http2 testing module

    \Input *pArgz   - environment
    \Input iArgc    - standard number of arguments
    \Input *pArgv[] - standard arg list

    \Output
        int32_t     - standard return value

    \Version 12/01/2016 (eesponda)
*/
/********************************************************************************F*/
int32_t CmdHttp2(ZContext *pArgz, int32_t iArgc, char *pArgv[])
{
    Http2RefT *pHttp2 = &_Http2;

    // allocate the module if needed
    if (pHttp2->pHttp == NULL)
    {
        if ((pHttp2->pHttp = ProtoHttp2Create(0)) == NULL)
        {
            ZPrintf("http2: could not allocate module state\n");
            return(1);
        }
    }

    // make sure there are enough parameters
    if (iArgc < 2)
    {
        return(0);
    }

    // close the connection
    if (ds_stricmp(pArgv[1], "close") == 0)
    {
        ProtoHttp2Close(pHttp2->pHttp);
    }
    // update the logging
    else if (ds_stricmp(pArgv[1], "spam") == 0)
    {
        ProtoHttp2Control(pHttp2->pHttp, PROTOHTTP2_INVALID_STREAMID, 'spam', atoi(pArgv[2]), 0, NULL);
    }
    else if (ds_stricmp(pArgv[1], "time") == 0)
    {
        ProtoHttp2Control(pHttp2->pHttp, PROTOHTTP2_INVALID_STREAMID, 'time', atoi(pArgv[2]), 0, NULL);
    }
    else if (ds_stricmp(pArgv[1], "grpc") == 0)
    {
        ds_strnzcpy(pHttp2->strGrpcHost, pArgv[2], sizeof(pHttp2->strGrpcHost));
    }

    if (pHttp2->iStreamId == 0)
    {
        // handle the normal get/head/options
        if ((ds_stricmp(pArgv[1], "get") == 0) || (ds_stricmp(pArgv[1], "head") == 0) || (ds_stricmp(pArgv[1], "options") == 0))
        {
            // a bit nasty but gets the job done
            ProtoHttpRequestTypeE eRequestType = *pArgv[1] == 'g' ? PROTOHTTP_REQUESTTYPE_GET : *pArgv[1] == 'h' ? PROTOHTTP_REQUESTTYPE_HEAD : PROTOHTTP_REQUESTTYPE_OPTIONS;

            pHttp2->uTimer = NetTick();

            if ((iArgc == 4) && (ds_stricmp(pArgv[3], "-cb") == 0))
            {
                ProtoHttp2RequestCb(pHttp2->pHttp, pArgv[2], NULL, 0, eRequestType, &pHttp2->iStreamId, _CmdHttp2WriteCb, &_Http2);
            }
            else
            {
                ProtoHttp2Request(pHttp2->pHttp, pArgv[2], NULL, 0, eRequestType, &pHttp2->iStreamId);
            }
        }
        else if (ds_stricmp(pArgv[1], "test1") == 0)
        {
            _CmdHttp2CreateGrpcRequest(pHttp2, "routeguide.RouteGuide/GetFeature", &_CmdHttp2GetFeature, 0, &_CmdHttp2GetFeatureResponse);
        }
        else if (ds_stricmp(pArgv[1], "test2") == 0)
        {
            _CmdHttp2CreateGrpcRequest(pHttp2, "routeguide.RouteGuide/ListFeatures", &_CmdHttp2ListFeatures, 0, &_CmdHttp2ListFeaturesResponse);
        }
        else if (ds_stricmp(pArgv[1], "test3") == 0)
        {
            _CmdHttp2CreateGrpcRequest(pHttp2, "routeguide.RouteGuide/RecordRoute", &_CmdHttp2RecordRoute, 10, _CmdHttp2RecordRouteResponse);
        }
        else if (ds_stricmp(pArgv[1], "test4") == 0)
        {
            _CmdHttp2CreateGrpcRequest(pHttp2, "routeguide.RouteGuide/RouteChat", &_CmdHttp2RouteChat, 5, _CmdHttp2RouteChatResponse);
        }
    }
    else
    {
        if (ds_stricmp(pArgv[1], "abort") == 0)
        {
            // abort the request
            ProtoHttp2Abort(pHttp2->pHttp, pHttp2->iStreamId);

            // cleanup send data
            _CmdHttp2CleaupSend(pHttp2);

            // cleanup stream
            ProtoHttp2StreamFree(pHttp2->pHttp, pHttp2->iStreamId);
            pHttp2->iStreamId = 0;
        }
    }

    return(ZCallback(_CmdHttp2IdleCb, 1));
}
