/*H*************************************************************************************/
/*!
    \File dirtysim.c

    \Description
        Lightweight network simulator intended to allow exercise of various codepaths
        in network code that handle adverse network conditions (latency, packet loss,
        out of order packets).  Is *not* intended to accurately simulate real network
        conditions.

    \Copyright
        Copyright (c) Electronic Arts 2018.

    \Version 01/25/2018 (jbrookes) First Version
*/
/*************************************************************************************H*/

/*** Example Usage *********************************************************************/

/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// dirtysock includes
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/proto/protossl.h"

// zlib includes
#include "libsample/zlib.h"
#include "libsample/zmem.h"
#include "libsample/zfile.h"

/*** Defines ***************************************************************************/

// PACKET_CHANCE percentage chance packet is lost, out of order, or corrupted
#define PACKET_CHANCE_PCT (10)
#define PACKET_CHANCE ((65536*PACKET_CHANCE_PCT)/100)

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct DirtyPacketT
{
    uint32_t uPacketSize;
    uint32_t uPacketTick;
    uint8_t  aPacketData[SOCKET_MAXUDPRECV];
} DirtyPacketT;

typedef struct DirtyPacketQueueT
{
    uint32_t uNumPackets;
    uint32_t uFirstPacket;
    uint32_t uLastPacket;
    DirtyPacketT PacketBuf[128];
} DirtyPacketQueueT;

typedef struct DirtyBridgeT
{
    struct sockaddr ClientAddrA;
    DirtyPacketQueueT ClientQueueA;
    struct sockaddr ClientAddrB;
    DirtyPacketQueueT ClientQueueB;
} DirtyBridgeT;

typedef struct DirtysimOptionsT
{
    uint32_t uTemp;
} DirtysimOptionsT;

typedef struct DirtysimStateT
{
    SocketT *pSocket;
    DirtyBridgeT Bridge;
} DirtysimStateT;

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

// Public variables


/*** Private Functions ******************************************************************/

/*F*************************************************************************************/
/*!
    \Function dirtysim_zprintf_hook

    \Description
        Hook up debug output for ZPrintf where appropriate

    \Input *pParm       - unused
    \Input *pText       - text to print

    \Output
        int32_t         - one

    \Version 01/25/2018 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t dirtysim_zprintf_hook(void *pParm, const char *pText)
{
    // on PC we want output to console in addition to debug output
    #if defined(DIRTYCODE_PC)
    printf("%s", pText);
    #endif
    // don't suppress debug output
    return(1);
}

/*F*************************************************************************************/
/*!
    \Function network_startup

    \Description
        Start up required DirtySDK networking

    \Input *pParm       - unused
    \Input *pText       - text to print

    \Output
        int32_t         - one

    \Version 01/25/2018 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t network_startup(void)
{
    int32_t iResult=0, iStatus, iTimeout;

    // start network
    NetConnStartup("-servicename=dirtysim");

    // bring up the interface
    NetConnConnect(NULL, NULL, 0);

    // wait for network interface activation
    for (iTimeout = NetTick() + 15*1000; ; )
    {
        // update network
        NetConnIdle();

        // get current status
        iStatus = NetConnStatus('conn', 0, NULL, 0);
        if ((iStatus == '+onl') || ((iStatus >> 24) == '-'))
        {
            break;
        }

        // check for timeout
        if (iTimeout < (signed)NetTick())
        {
            ZPrintf("dirtysim: timeout waiting for interface activation\n");
            break;
        }

        // give time to other threads
        NetConnSleep(500);
    }

    // check result code
    if ((iStatus = NetConnStatus('conn', 0, NULL, 0)) == '+onl')
    {
        ZPrintf("dirtysim: interface active\n");
        iResult = 1;
    }
    else if ((iStatus >> 24) == '-')
    {
        ZPrintf("dirtysim: error %C bringing up interface\n", iStatus);
    }

    // return result to caller
    return(iResult);
}

/*F*************************************************************************************/
/*!
    \Function process_args

    \Description
        Process command-line arguments

    \Input iArgc        - arg count
    \Input *pArgv[]     - arg list
    \Input *pOptions    - [out] parsed options

    \Version 01/25/2018 (jbrookes)
*/
/*************************************************************************************F*/
static void process_args(int32_t iArgc, const char *pArgv[], DirtysimOptionsT *pOptions)
{
    int32_t iArg;

    // echo options
    for (iArg = 0; iArg < iArgc; iArg += 1)
    {
        ZPrintf("%s ", pArgv[iArg]);
    }
    ZPrintf("\n");

    ds_memclr(pOptions, sizeof(*pOptions));

    // init default options

    // pick off command-line options
    for (iArg = 1; (iArg < iArgc) && (pArgv[iArg][0] == '-'); iArg += 1)
    {
        #if 0 // example processing
        if (!strcmp(pArgv[iArg], "-someopt") && ((iArg+1) < iArgc))
        {
            // process option, skip additional value if present (e.g. -someopt value)
            iArg += 1;
        }
        #endif
    }
}

/*F*************************************************************************************/
/*!
    \Function add_packet_to_queue

    \Description
        Add a packet to packet queue

    \Input *pPacketQueue    - packet queue to add to
    \Input iPacketData      - packet data to add
    \Input iPacketSize      - size of packet data

    \Version 01/25/2018 (jbrookes)
*/
/*************************************************************************************F*/
static void add_packet_to_queue(DirtyPacketQueueT *pPacketQueue, const uint8_t *pPacketData, int32_t iPacketSize)
{
    const uint32_t uQueueSize = sizeof(pPacketQueue->PacketBuf)/sizeof(pPacketQueue->PacketBuf[0]);
    DirtyPacketT *pPacket;

    if (pPacketQueue->uNumPackets == uQueueSize)
    {
        ZPrintf("dirtysim: packet queue full\n");
        return;
    }

    // allocate packet
    pPacket = &pPacketQueue->PacketBuf[pPacketQueue->uNumPackets++];
    // copy data
    ds_memcpy_s(pPacket->aPacketData, sizeof(pPacket->aPacketData), pPacketData, iPacketSize);
    pPacket->uPacketSize = iPacketSize;
    // timestamp receive
    pPacket->uPacketTick = NetTick();
}

/*F*************************************************************************************/
/*!
    \Function get_packet_from_queue

    \Description
        Get a packet from packet queue

    \Input *pPacketQueue    - packet queue to get packet from
    \Input iPacketData      - [out] packet buffer
    \Input iPacketSize      - size of packet buffer
    \Input uMinQueueSize    - min queue size to allow packet reordering

    \Version 01/25/2018 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t get_packet_from_queue(DirtyPacketQueueT *pPacketQueue, uint8_t *pPacketData, int32_t iPacketSize, uint32_t uMinQueueSize)
{
    DirtyPacketT *pPacket;

    // make sure we buffer at least one packet to be able to do out-of-order
    if (pPacketQueue->uNumPackets <= uMinQueueSize)
    {
        return(0);
    }

    // deallocate packet
    pPacket = &pPacketQueue->PacketBuf[--pPacketQueue->uNumPackets];
    // copy data
    ds_memcpy_s(pPacketData, iPacketSize, pPacket->aPacketData, pPacket->uPacketSize);
    // return
    return(pPacket->uPacketSize);
}

/*F*************************************************************************************/
/*!
    \Function packet_queue_process

    \Description
        Process packet queue, introducing possible packet loss, out of order,
        or corruption.

    \Input *pState          - application state
    \Input *pPacketQueue    - packet queue to process
    \Input *pSrcAddr        - bridge src addr
    \Input *pDstAddr        - bridge dst addr

    \Version 01/25/2018 (jbrookes)
*/
/*************************************************************************************F*/
static void packet_queue_process(DirtysimStateT *pState, DirtyPacketQueueT *pPacketQueue, struct sockaddr *pSrcAddr, struct sockaddr *pDstAddr)
{
    uint8_t aPacketBuf[SOCKET_MAXUDPRECV];
    int32_t iAddrLen = sizeof(*pDstAddr), iPacketLen;
    uint32_t uRandom;

    // get packet; give us a buffer to allow out-of-order packets
    if ((iPacketLen = get_packet_from_queue(pPacketQueue, aPacketBuf, sizeof(aPacketBuf), 2)) == 0)
    {
        return;
    }

    // get a random 16bit number
    uRandom = NetRand(0xffff);

    // see if we should do something
    if (uRandom < PACKET_CHANCE)
    {
        ZPrintf("dirtysim: %A->%A (%d bytes) (depth=%d) [LOST]\n", pSrcAddr, pDstAddr, iPacketLen, pPacketQueue->uNumPackets);
        // packet lost; don't forward
        return;
    }
    else if (uRandom < (PACKET_CHANCE*2))
    {
        uint8_t aPacketBuf2[SOCKET_MAXUDPRECV];
        int32_t iPacketLen2;

        // packet out of order - get next packet and send it first
        if ((iPacketLen2 = get_packet_from_queue(pPacketQueue, aPacketBuf2, sizeof(aPacketBuf2), 1)) > 0)
        {
            ZPrintf("dirtysim: %A->%A (%d bytes) (depth=%d) [OUTOFORDER]\n", pSrcAddr, pDstAddr, iPacketLen2, pPacketQueue->uNumPackets);
            SocketSendto(pState->pSocket, (char *)aPacketBuf2, iPacketLen2, 0, pDstAddr, iAddrLen);
        }
    }
    else if (uRandom < (PACKET_CHANCE*3))
    {
        // corrupt the packet; we pick a random byte and set it to the index value
        uRandom = NetRand(iPacketLen-1);
        aPacketBuf[uRandom] = (uint8_t)(uRandom&0xff);
        ZPrintf("dirtysim: %A->%A (%d bytes) (depth=%d) [CORRUPTED]\n", pSrcAddr, pDstAddr, iPacketLen, pPacketQueue->uNumPackets);
    }
    else
    {
        ZPrintf("dirtysim: %A->%A (%d bytes) (depth=%d)\n", pSrcAddr, pDstAddr, iPacketLen, pPacketQueue->uNumPackets);
    }
    
    // forward the packet
    SocketSendto(pState->pSocket, (char *)aPacketBuf, iPacketLen, 0, pDstAddr, iAddrLen);
}


/*** Public Functions ******************************************************************/

// dll-friendly DirtyMemAlloc
#if !defined(DIRTYCODE_DLL)
void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
#else
void *DirtyMemAlloc2(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
#endif
{
    return(malloc(iSize));
}

// dll-friendly DirtyMemFree
#if !defined(DIRTYCODE_DLL)
void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
#else
void DirtyMemFree2(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
#endif
{
    free(pMem);
}


/*F*************************************************************************************/
/*!
    \Function main

    \Description
        Main processing for DirtySim

    \Input argc             - input argument count
    \Input *argv[]          - input argument list

    \Output
        int32_t             - negative on failure, else zero

    \Version 01/25/2018 (jbrookes)
*/
/*************************************************************************************F*/
int main(int32_t argc, const char *argv[])
{
    int32_t iCount, iLen, iTimeout;
    DirtysimOptionsT DirtysimOptions;
    DirtysimStateT DirtysimState;
    uint8_t aPacketBuf[SOCKET_MAXUDPRECV];
    struct sockaddr BindAddr, RecvAddr;
    int32_t iAddrLen = sizeof(RecvAddr), iRecvLen;

    #if defined(DIRTYCODE_DLL)
    DirtyMemFuncSet(&DirtyMemAlloc2, &DirtyMemFree2);
    #endif

    // hook into zprintf output
    ZPrintfHook(dirtysim_zprintf_hook, NULL);

    // get options
    process_args(argc, argv, &DirtysimOptions);

    // init state

    // start dirtysock
    if (!network_startup())
    {
        return(-1);
    }

    // init bridge
    ds_memclr(&DirtysimState.Bridge, sizeof(DirtysimState.Bridge));

    // client a
    SockaddrInit(&DirtysimState.Bridge.ClientAddrA, AF_INET);
    SockaddrInSetAddr(&DirtysimState.Bridge.ClientAddrA, SocketInTextGetAddr("10.8.13.194"));
    SockaddrInSetPort(&DirtysimState.Bridge.ClientAddrA, 8010);
    // client b
    SockaddrInit(&DirtysimState.Bridge.ClientAddrB, AF_INET);
    SockaddrInSetAddr(&DirtysimState.Bridge.ClientAddrB, SocketInTextGetAddr("10.8.13.194"));
    SockaddrInSetPort(&DirtysimState.Bridge.ClientAddrB, 8020);

    // create & bind DirtySim socket
    DirtysimState.pSocket = SocketOpen(AF_INET, SOCK_DGRAM, 0);
    SockaddrInit(&BindAddr, AF_INET);
    SockaddrInSetPort(&BindAddr, 8000);
    SocketBind(DirtysimState.pSocket, &BindAddr, iAddrLen);

    // just keep working
    for (iCount = 0, iLen = -1, iTimeout = NetTick()-1; ; )
    {
        // receive a packet
        iRecvLen = SocketRecvfrom(DirtysimState.pSocket, (char *)aPacketBuf, sizeof(aPacketBuf), 0, &RecvAddr, &iAddrLen);

        // if we got something, add it to the packet queue
        if (iRecvLen > 0)
        {
            //ZPrintf("dirtysim: read %d byte packet from %A\n", iRecvLen, &RecvAddr);
            // ClientA->ClientB
            if (!SockaddrCompare(&RecvAddr, &DirtysimState.Bridge.ClientAddrA))
            {
                add_packet_to_queue(&DirtysimState.Bridge.ClientQueueA, aPacketBuf, iRecvLen);
            }
            // ClientB->ClientA
            else if (!SockaddrCompare(&RecvAddr, &DirtysimState.Bridge.ClientAddrB))
            {
                add_packet_to_queue(&DirtysimState.Bridge.ClientQueueB, aPacketBuf, iRecvLen);
            }
        }

        // process the packet queue; if there's a packet to send, send it
        packet_queue_process(&DirtysimState, &DirtysimState.Bridge.ClientQueueA, &DirtysimState.Bridge.ClientAddrA, &DirtysimState.Bridge.ClientAddrB);
        packet_queue_process(&DirtysimState, &DirtysimState.Bridge.ClientQueueB, &DirtysimState.Bridge.ClientAddrB, &DirtysimState.Bridge.ClientAddrA);

        // if we got something, try again
        if (iRecvLen > 0)
        {
            continue;
        }

        // sleep a bit
        NetConnSleep(10);
    }

    //ZPrintf("dirtysim: done\n");

    // disconnect from the network%
    //NetConnDisconnect();

    // shutdown the network connections && destroy the dirtysock code
    //NetConnShutdown(FALSE);
    //return(0);
}