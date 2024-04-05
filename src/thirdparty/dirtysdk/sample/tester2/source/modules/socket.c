/*H*************************************************************************************/
/*!
    \File    socket.c

    \Description
        Reference application for the socket module.

    \Copyright
        Copyright (c) Electronic Arts 2011.    ALL RIGHTS RESERVED.

    \Version 01/20/2011 (mclouatre)
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/
#include <string.h>
#include <stdio.h>   // for sscanf()

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"

#include "libsample/zlib.h"

#include "DirtySDK/dirtysock/dirtynet.h"
#include "testersubcmd.h"
#include "testermodules.h"


/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/
typedef struct SocketAppT
{
    SocketT *pSocket;
    int32_t iSocketType;
    int32_t bSocketBound;
    int32_t iVerbosityLevel;
    int32_t bAverageCalculationCycleStarted;
    uint32_t uAverageCalculationCycleStartTick;

    // reception data
    uint8_t bReceiving;
    uint8_t recvBuffer[64];
    int32_t iUdpDatagramsReceived;
    int32_t iReadIntervalInMs;
    uint32_t uLastRecvTick;
    struct sockaddr source;

    // transmission data
    uint8_t bTransmitting;
    uint8_t xmitBuffer[64];
    int32_t iUdpDatagramsSent;
    int32_t iWriteIntervalInMs;
    uint32_t uLastSentTick;
    struct sockaddr destination;

} SocketAppT;

/*** Function Prototypes ***************************************************************/

static void _SocketOpen(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _SocketBind(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _SocketClose(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _SocketRecvFrom(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _SocketSendTo(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _SetDebugVerbosity(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);



/*** Variables *************************************************************************/

// Private variables
static T2SubCmdT _Socket_Commands[] =
{
    { "open",       _SocketOpen                     },
    { "bind",       _SocketBind                     },
    { "close",      _SocketClose                    },
    { "sendto",     _SocketSendTo                   },
    { "recvfrom",   _SocketRecvFrom                 },
    { "verbose",    _SetDebugVerbosity              },
    { "",           NULL                            }
};

static SocketAppT _Socket_App;
uint8_t _SocketApp_bInitialized;

/*** Private Functions *****************************************************************/

/*F*************************************************************************************/
/*!
    \Function _CmdSocketCb

    \Description
        Socket idle callback.

    \Input *argz    - unused
    \Input argc     - argument count
    \Input **argv   - argument list

    \Output
        int32_t         - zero

    \Version 01/20/2011 (mclouatre)
*/
/**************************************************************************************F*/
static int32_t _CmdSocketCb(ZContext *argz, int32_t argc, char **argv)
{
    SocketAppT *pApp = &_Socket_App;
    int32_t iInterval;

    // if socket no more exists, stop invoking this callback
    if (!pApp->pSocket)
    {
        return(0);
    }

    // display send/recv average every 5 sec
    if (pApp->bReceiving || pApp->bTransmitting)
    {
        if (pApp->bAverageCalculationCycleStarted)
        {
            // check if cycle completed
            if (NetTickDiff(NetTick(), pApp->uAverageCalculationCycleStartTick) > 10000)
            {
                uint32_t uSendingAverage; 
                uint32_t uReceivingAverage;

                pApp->bAverageCalculationCycleStarted = FALSE;

                uSendingAverage = pApp->iUdpDatagramsSent / 10;

                uReceivingAverage = pApp->iUdpDatagramsReceived / 10;

                ZPrintf("   LAST 10 SECONDS -> dgrams sent=%d | dgrams rcved=%d | avg dgrams sent=%d/sec | avg dgrams rcved=%d/sec\n",
                    pApp->iUdpDatagramsSent, pApp->iUdpDatagramsReceived, uSendingAverage, uReceivingAverage);
            }
        }

        if (!pApp->bAverageCalculationCycleStarted)
        {
            // mark cycle as started
            pApp->bAverageCalculationCycleStarted = TRUE;

            // reset cycle start tick
            pApp->uAverageCalculationCycleStartTick = NetTick();

            // reset counters
            pApp->iUdpDatagramsSent = 0;
            pApp->iUdpDatagramsReceived = 0;
        }
    }

    if (pApp->iSocketType == SOCK_DGRAM)
    {
        // is receiving enabled?
        if (pApp->bReceiving)
        {
            uint32_t uCurrentTick = NetTick();
            int32_t iRecvLen;
            int32_t iSourceLen;
            int32_t iSourcePort;
            char strSourceAddrText[16];
            iRecvLen = SocketRecvfrom(pApp->pSocket, (char *)&pApp->recvBuffer, sizeof(pApp->recvBuffer), 0, &pApp->source, &iSourceLen);
            if (iRecvLen > 0)
            {
                pApp->iUdpDatagramsReceived++;
                SockaddrInGetAddrText(&pApp->source, strSourceAddrText, sizeof(strSourceAddrText));
                iSourcePort = SockaddrInGetPort(&pApp->source);
                if (pApp->iVerbosityLevel > 3)
                {
                    ZPrintf("   received 1 datagram (size %d) from %s:%d at time %d (delta = %d ms)\n",
                        iRecvLen, strSourceAddrText, iSourcePort, uCurrentTick, NetTickDiff(uCurrentTick, pApp->uLastRecvTick));
                }
                pApp->uLastRecvTick = uCurrentTick;
            }
            else
            {
                if (pApp->iVerbosityLevel > 3)
                {
                    ZPrintf("   SocketRecvfrom() returned %d at time %d\n", iRecvLen, uCurrentTick);
                }
            }
        }

        // is transmitting enabled?
        if (pApp->bTransmitting)
        {
            uint32_t uCurrentTick = NetTick();
            int32_t iSentLen;
            int32_t iDestinationPort;
            char strDestAddrText[16];
            iSentLen = SocketSendto(pApp->pSocket, (char *)&pApp->xmitBuffer, sizeof(pApp->recvBuffer), 0, &pApp->destination, sizeof(pApp->destination));
            if (iSentLen > 0)
            {
                pApp->iUdpDatagramsSent++;
                SockaddrInGetAddrText(&pApp->destination, strDestAddrText, sizeof(strDestAddrText));
                iDestinationPort = SockaddrInGetPort(&pApp->destination);
                if (pApp->iVerbosityLevel > 3)
                {
                    ZPrintf("   sent 1 datagram (size %d) to %s:%d at time %d (delta =%d ms)\n",
                        iSentLen, strDestAddrText, iDestinationPort, uCurrentTick, NetTickDiff(uCurrentTick, pApp->uLastSentTick));
                }
                pApp->uLastSentTick = uCurrentTick;
            }
            else
            {
                if (pApp->iVerbosityLevel > 3)
                {
                    ZPrintf("   SocketSendto() returned %d at time %d\n", iSentLen, uCurrentTick);
                }
            }
        }
    }
    else
    {
        ZPrintf("   send/recv processing not implemented for raw or tcp sockets (current socket type = %d)\n", pApp->iSocketType);
        return(0);
    }

    if (pApp->iReadIntervalInMs < pApp->iWriteIntervalInMs)
    {
        iInterval = pApp->iReadIntervalInMs;
    }
    else
    {
        iInterval = pApp->iWriteIntervalInMs;
    }
    return(ZCallback(_CmdSocketCb, iInterval));
}

/*F*************************************************************************************/
/*!
    \Function _SocketOpen

    \Description
        Socket subcommand - open a socket

    \Input *pApp    - pointer to Socket app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 01/20/2011 (mclouatre)
*/
/**************************************************************************************F*/
static void _SocketOpen(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    SocketAppT *pApp = (SocketAppT *)_pApp;

    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s %s <raw|udp|tcp>\n", argv[0], argv[1]);
        return;
    }

    if (strcmp(argv[2], "raw") == 0)
    {
        pApp->iSocketType = SOCK_RAW;
    }
    else if (strcmp(argv[2], "udp") == 0)
    {
        pApp->iSocketType = SOCK_DGRAM;
    }
    else if (strcmp(argv[2], "tcp") == 0)
    {
        pApp->iSocketType = SOCK_STREAM;
    }
    else
    {
        ZPrintf("   invalid socket type\n", argv[2]);
        ZPrintf("   usage: %s %s <raw|udp|tcp>\n", argv[0], argv[1]);
        return;
    }

    // make sure we don't already have a socket
    if (pApp->pSocket != NULL)
    {
        ZPrintf("   %s: there is already a socket opened\n", argv[0]);
        return;
    }

    pApp->pSocket = SocketOpen(AF_INET, pApp->iSocketType, 0);
    if (pApp->pSocket)
    {
        ZPrintf("   %s: opened %s socket (ref=%p)\n", argv[0], argv[2], pApp->pSocket);
    }
    else
    {
        ZPrintf("   %s: failed to open socket\n", argv[0]);
        return;
    }
    pApp->bSocketBound = FALSE;

    // read and write interval defaul to 1 sec until recv or send is initiated
    pApp->iReadIntervalInMs = 1000;
    pApp->iWriteIntervalInMs = 1000;

    // register periodic callback
    ZCallback(_CmdSocketCb, pApp->iReadIntervalInMs);
}

/*F*************************************************************************************/
/*!
    \Function _SocketBind

    \Description
        Socket subcommand - bind a socket to a local IP address and port

    \Input *pApp    - pointer to Socket app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 01/20/2011 (mclouatre)
*/
/**************************************************************************************F*/
static void _SocketBind(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    SocketAppT *pApp = (SocketAppT *)_pApp;
    int32_t iResult;
    struct sockaddr bindAddr;

    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s %s <ipaddress:port>\n", argv[0], argv[1]);
        return;
    }

    if (!pApp->pSocket)
    {
        ZPrintf("   %s: socket is not yet opened\n", argv[0]);
        return;
    }

    if (pApp->bSocketBound)
    {
        ZPrintf("   %s: socket (ref=%p) is already bound\n", argv[0], pApp->pSocket);
        return;
    }

    // initialize bindAddr from input <ipaddress:port> string
    if ((SockaddrInParse(&bindAddr, argv[2]) & 0x3) != 0x3)
    {
        ZPrintf("   %s: badly formatted <ipaddress:port> parameter\n", argv[0]);
        return;
    }

    iResult = SocketBind(pApp->pSocket, &bindAddr, sizeof(bindAddr));
    if (iResult < 0)
    {
        ZPrintf("   %s: socket (ref=%p) failed to bind to %s -  err = %d\n", argv[0], pApp->pSocket, argv[2], iResult);
    }
    else
    {
        SocketInfo(pApp->pSocket, 'bind', 0, &bindAddr, sizeof(bindAddr));
        ZPrintf("   %s: socket (ref=%p) bound to %A\n", argv[0], pApp->pSocket, &bindAddr);
        pApp->bSocketBound = TRUE;
    }
}

/*F*************************************************************************************/
/*!
    \Function _SocketClose

    \Description
        Socket subcommand - close a socket

    \Input *pApp    - pointer to Socket app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 01/20/2011 (mclouatre)
*/
/**************************************************************************************F*/
static void _SocketClose(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    SocketAppT *pApp = (SocketAppT *)_pApp;

    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s %s\n", argv[0], argv[1]);
        return;
    }

    if (!pApp->pSocket)
    {
        ZPrintf("   %s: socket is not yet opened\n", argv[0]);
        return;
    }

    SocketClose(pApp->pSocket);
    pApp->bSocketBound = FALSE;
    ZPrintf("   %s: closed socket (ref=%p)\n", argv[0], pApp->pSocket);
    pApp->pSocket = NULL;
}

/*F*************************************************************************************/
/*!
    \Function _SocketRecvFrom

    \Description
        Socket subcommand - recvfrom operation on previously created socket

    \Input *pApp    - pointer to Socket app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 01/20/2011 (mclouatre)
*/
/**************************************************************************************F*/
static void _SocketRecvFrom(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    SocketAppT *pApp = (SocketAppT *)_pApp;

    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s %s <read rate (ms)>\n", argv[0], argv[1]);
        return;
    }

    if (!pApp->pSocket)
    {
        ZPrintf("   %s: socket is not yet opened\n", argv[0]);
        return;
    }

    if (!pApp->bSocketBound)
    {
        ZPrintf("   %s: socket (ref=%p) is not yet bound\n", argv[0], pApp->pSocket);
        return;
    }

    // get read rate
    sscanf(argv[2], "%d", &pApp->iReadIntervalInMs);

    ZPrintf("   %s: data reception enabled on socket (ref=%p) - read rate is: 1 read operation every %d ms\n", argv[0], pApp->pSocket, pApp->iReadIntervalInMs);
    pApp->bReceiving = TRUE;
}

/*F*************************************************************************************/
/*!
    \Function _SocketSendTo

    \Description
        Socket subcommand - sendto operation on previously created socket

    \Input *pApp    - pointer to Socket app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 01/20/2011 (mclouatre)
*/
/**************************************************************************************F*/
static void _SocketSendTo(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    SocketAppT *pApp = (SocketAppT *)_pApp;


    if ((bHelp == TRUE) || (argc != 4))
    {
        ZPrintf("   usage: %s %s <ipaddress:port> <write rate (ms)>\n", argv[0], argv[1]);
        return;
    }

    if (!pApp->pSocket)
    {
        ZPrintf("   %s: socket is not yet opened\n", argv[0]);
        return;
    }

    // initialize destination from input <ipaddress:port> string
    if ((SockaddrInParse(&pApp->destination, argv[2]) & 0x3) != 0x3)
    {
        ZPrintf("   %s: badly formatted <ipaddress:port> parameter\n", argv[0]);
        return;
    }

    if (!pApp->bSocketBound)
    {
        ZPrintf("   %s: socket (ref=%p) is not yet bound\n", argv[0], pApp->pSocket);
        return;
    }

    // get write rate
    sscanf(argv[3], "%d", &pApp->iWriteIntervalInMs);

    ZPrintf("   %s: data transmission enabled on socket (ref=%p) - write rate is: 1 write operation every %d ms\n", argv[0], pApp->pSocket, pApp->iWriteIntervalInMs);

    pApp->bTransmitting = TRUE;
}

/*F*************************************************************************************/
/*!
    \Function _SetDebugVerbosity

    \Description
        Socket subcommand - sendto operation on previously created socket

    \Input *pApp    - pointer to Socket app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 01/20/2011 (mclouatre)
*/
/**************************************************************************************F*/
static void _SetDebugVerbosity(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    SocketAppT *pApp = (SocketAppT *)_pApp;

    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s %s <verbositylevel>\n", argv[0], argv[1]);
        return;
    }

    // get write rate
    sscanf(argv[2], "%d", &pApp->iVerbosityLevel);
}

/*** Public Functions ******************************************************************/

/*F*************************************************************************************/
/*!
    \Function    CmdSocket

    \Description
        Socket command.

    \Input *argz    - unused
    \Input argc     - argument count
    \Input **argv   - argument list

    \Output
        int32_t         - zero

    \Version 01/20/2011 (mclouatre) First Version
*/
/**************************************************************************************F*/
int32_t CmdSocket(ZContext *argz, int32_t argc, char **argv)
{
    T2SubCmdT *pCmd;
    SocketAppT *pApp = &_Socket_App;
    unsigned char bHelp;

    // initialize state
    if (!_SocketApp_bInitialized)
    {
        ds_memclr(pApp, sizeof(*pApp));
        _SocketApp_bInitialized = TRUE;
    }

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_Socket_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   exercises DirtySock socket API\n");
        T2SubCmdUsage(argv[0], _Socket_Commands);
        return(0);
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    return(0);
}
