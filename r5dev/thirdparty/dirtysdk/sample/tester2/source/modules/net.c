/*H*************************************************************************************/
/*!
    \File    lobby.c

    \Description
        Reference application for the NetApi module.

    \Notes
        Base code from locker test function by jbrookes.

    \Copyright
        Copyright (c) Electronic Arts 2004.    ALL RIGHTS RESERVED.

    \Version    1.0        04/12/2005 (jfrank) First Version
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if defined (__CELLOS_LV2__)
#include <ppu_thread.h>
#include <np.h>
#include <netex/net.h>
#endif

#ifdef _XBOX
#include <xtl.h>
#endif

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

#include "libsample/zlib.h"
#include "libsample/zmem.h"

#include "testerregistry.h"
#include "testerhostcore.h"
#include "testersubcmd.h"
#include "testermodules.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct NetAppT
{
    int32_t                     iNetUp;
    int8_t    bExternalCleanupComplete;
} NetAppT;

/*** Function Prototypes ***************************************************************/

static void _NetStartup(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetQuery(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
#if defined(DIRTYCODE_PS4)
static void _NetTicket(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
#endif
static void _NetConnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetId(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetDisconnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetShutdown(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetStatus(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetPorts(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);

/*** Variables *************************************************************************/

static T2SubCmdT _Net_Commands[] =
{
    { "startup",        _NetStartup       },
    { "start",          _NetStartup       },
    { "query",          _NetQuery         },
    { "ctrl",           _NetControl       },
    { "id",             _NetId            },
    { "status",         _NetStatus        },
    { "ports",          _NetPorts         },
#if defined(DIRTYCODE_PS4)
    { "ticket",         _NetTicket        },
#endif
    { "connect",        _NetConnect       },
    { "disconnect",     _NetDisconnect    },
    { "shutdown",       _NetShutdown      },
    { "stop",           _NetShutdown      },
    { "",               NULL              }
};

static NetAppT _Net_App =
{ 0,
  0
};

/*** Private Functions *****************************************************************/

/*F*************************************************************************************************/
/*!
    \Function _NetExtCleanupCallback

    \Description
        To test external cleanup mechanism

    \Input *pCallbackData   - pointer to platform-specific dataspace

    \Output
        int32_t             - zero=success; -1=try again; other negative=error

    \Version 10/01/2011 (mclouatre)
*/
/*************************************************************************************************F*/
static int32_t _NetExtCleanupCallback(void *pCallbackData)
{
    NetAppT *pApp = (NetAppT *)pCallbackData;

    if (pApp->bExternalCleanupComplete)
    {
        return(0);  // complete
    }
    else
    {
        return(-1);  // try again
    }
}

/*F*************************************************************************************/
/*!
    \Function _NetStartup

    \Description
        Net subcommand - start dirtysock

    \Input *pApp    - pointer to lobby app
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 04/12/2005 (jfrank)
*/
/**************************************************************************************F*/
static void _NetStartup(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    NetAppT *pApp = (NetAppT *)pCmdRef;
    char strBuf[256] = "-servicename=tester2";
    int32_t iLoop;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s [start|startup] \"<startup parameters>\"\n", argv[0], argv[1]);
        return;
    }

    for (iLoop = 2; iLoop < argc; iLoop++)
    {
        ds_strnzcat(strBuf, " ", (int32_t)(sizeof(strBuf) - strlen(strBuf) - 1));
        ds_strnzcat(strBuf, argv[iLoop], (int32_t)(sizeof(strBuf) - strlen(strBuf) - 1));
    }

    ZPrintf("NET: Starting dirtysock with params {%s}.\n", strBuf);
    NetConnStartup(strBuf);

    pApp->iNetUp = 1;
}

/*F*************************************************************************************/
/*!
    \Function _NetQuery

    \Description
        Net subcommand - issue a NetConnQuery call

    \Input *pApp    - pointer to lobby app
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 05/12/2005 (jbrookes)
*/
/**************************************************************************************F*/
static void _NetQuery(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    if ((bHelp == TRUE) || (argc > 3))
    {
        ZPrintf("   usage: %s [query] \"<startup parameters>\"\n", argv[0], argv[1]);
        return;
    }

    if (argc == 2)
    {
        NetConnQuery(NULL, NULL, 0);
    }
    else if (argc == 3)
    {
        NetConnQuery(argv[2], NULL, 0);
    }
}

#if defined(DIRTYCODE_PS4)
/*F*************************************************************************************/
/*!
    \Function _NetTicket

    \Description
        Net subcommand - test ps3 ticketing system

    \Input *pApp    - pointer to lobby app
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 10/09/2009 (jbrookes)
*/
/**************************************************************************************F*/
static void _NetTicket(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    uint8_t aTicketBuf[1024];
    int32_t iResult;

    // acquire ticket
    if ((iResult = NetConnStatus('tick', 0, aTicketBuf, sizeof(aTicketBuf))) < 1)
    {
        NetPrintf(("net: NetConnStatus('tick') returned %d\n", iResult));
        return;
    }
    // display environment
    if ((iResult = NetConnStatus('envi', 0, NULL, 0)) < 1)
    {
        NetPrintf(("net: NetConnStatus('envi') returned %d\n", iResult));
        return;
    }
    NetPrintf(("net: platform environment is %d\n"));
}
#endif //defined(DIRTYCODE_PS4)

/*F*************************************************************************************/
/*!
    \Function _NetConnect

    \Description
        Net subcommand - connect the networking

    \Input *pApp    - pointer to lobby app
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 04/12/2005 (jfrank) First Version
*/
/**************************************************************************************F*/
static void _NetConnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    if ((bHelp == TRUE) || (argc < 2) || (argc > 4))
    {
        ZPrintf("   usage: %s connect \"<startup parameters>\"\n", argv[0]);
        return;
    }

    if (argc == 2)
    {
        NetConnConnect(NULL, NULL, 0);
    }
    else if (argc == 3)
    {
        NetConnConnect((const NetConfigRecT *)argv[2], NULL, 0);
    }
    else if (argv[2][0] == '-')
    {
        NetConnConnect(NULL, argv[3], 0);
    }
    else
    {
        NetConnConnect((const NetConfigRecT *)argv[2], argv[3], 0);
    }
}

/*F*************************************************************************************/
/*!
    \Function _NetId

    \Description
        Net subcommand - set up accountId and personaId

    \Input *pApp    - pointer to lobby app
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 07/31/2019 (mgallant) 
*/
/**************************************************************************************F*/
static void _NetId(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    if ((bHelp == TRUE) || (argc != 4))
    {
        ZPrintf("   usage: %s id [accountId] [personaId]\n", argv[0]);
        return;
    }

    int64_t iAccountId = (int64_t) strtol(argv[2], NULL, 10);
    int64_t iPersonaId = (int64_t) strtol(argv[3], NULL, 10);

    ZPrintf("net: executing NetConnControl('%s', %d, %d, %s, %s)\n", "acid", 0, sizeof(iAccountId), "<ptr>", NULL);
    NetConnControl('acid', 0, sizeof(iAccountId), &iAccountId, NULL);

    ZPrintf("net: executing NetConnControl('%s', %d, %d, %s, %s)\n", "peid", 0, sizeof(iPersonaId), "<ptr>", NULL);
    NetConnControl('peid', 0, sizeof(iPersonaId), &iPersonaId, NULL);

    return;

}

/*F*************************************************************************************/
/*!
    \Function _NetDisconnect

    \Description
        Net subcommand - connect the networking

    \Input *pApp    - pointer to lobby app
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 04/12/2005 (jfrank) First Version
*/
/**************************************************************************************F*/
static void _NetDisconnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s disconnect\n", argv[0]);
        return;
    }

    NetConnDisconnect();
}

/*F*************************************************************************************/
/*!
    \Function _NetShutdown

    \Description
        Net subcommand - shut down dirtysock

    \Input *pApp    - pointer to lobby app
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 04/12/2005 (jfrank) First Version
*/
/**************************************************************************************F*/
static void _NetShutdown(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    NetAppT *pApp = (NetAppT *)pCmdRef;
    TesterHostCoreT *pCore;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s [stop|shutdown]\n", argv[0]);
        return;
    }

    pApp->iNetUp = 0;

    // signal to the core that we want to shut everything down
    if ((pCore = (TesterHostCoreT *)TesterRegistryGetPointer("CORE")) != NULL)
    {
        TesterHostCoreShutdown(pCore);
    }
}

/*F********************************************************************************/
/*!
    \Function _NetControl

    \Description
        Execute NetConnControl()

    \Input *pApp    - pointer to upnp module
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false

    \Version 09/27/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _NetControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    NetAppT *pApp = (NetAppT *)pCmdRef;
    int32_t iCmd, iValue=0, iValue2=0;
    void *pValue = NULL;
    void *pValue2 = NULL;

    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("   usage: %s ctrl <cmd> [val1] [val2] [strVal] [strVal]\n", argv[0]);
        return;
    }

    iCmd  = argv[2][0] << 24;
    iCmd |= argv[2][1] << 16;
    iCmd |= argv[2][2] << 8;
    iCmd |= argv[2][3];

    if (argc > 3)
    {
        iValue = (int32_t)strtol(argv[3], NULL, 10);
    }
    if (argc > 4)
    {
        iValue2 = (int32_t)strtol(argv[4], NULL, 10);
    }
    if (argc > 5)
    {
        pValue = argv[5];
    }
    if (argc > 6)
    {
        pValue2 = argv[6];
    }

    if (strcmp("recu", argv[2]) == 0)
    {
        pApp->bExternalCleanupComplete = FALSE;
        pValue = (void *)_NetExtCleanupCallback;
        pValue2 = (void *)pApp;
    }

    ZPrintf("net: executing NetConnControl('%s', %d, %d, %s, %s)\n", argv[2], iValue, iValue2, pValue ? "<ptr>" : "(null)", pValue2 ? "<ptr>" : "(null)");
    NetConnControl(iCmd, iValue, iValue2, pValue, pValue2);
}

/*F*************************************************************************************/
/*!
    \Function _NetStatus

    \Description
        Net subcommand - Execute NetConnStatus()

    \Input *pApp    - pointer to lobby app
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 04/12/2005 (jfrank) First Version
*/
/**************************************************************************************F*/
static void _NetStatus(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    unsigned char *pToken;
    uint32_t uResult, uToken, iData;

    if ((bHelp == TRUE) || (argc != 4))
    {
        ZPrintf("   usage: %s status <4-char status token to get> <iData>\n", argv[0]);
        return;
    }

    pToken = (unsigned char *)argv[2];
    uToken = (pToken[0] << 24) | (pToken[1] << 16) | (pToken[2] << 8) | pToken[3];
    iData = (int32_t)strtol(argv[3], NULL, 10);

#if defined(DIRTYCODE_XBOXONE)
    if (strcmp("tick", argv[2]) == 0)
    {
        char strToken[16 * 1024];
        uResult = NetConnStatus(uToken, iData, (void *)strToken, sizeof(strToken));
    }
    else if (strcmp("xadd", argv[2]) == 0)
    {
        uint8_t aSecureDeviceAddressBlob[256];

        uResult = NetConnStatus(uToken, iData, (void *)aSecureDeviceAddressBlob, sizeof(aSecureDeviceAddressBlob));
    }
    else
#endif
    {
        uResult = NetConnStatus(uToken, iData, NULL, 0);
    }

    // if printable, display result as text
    if (isprint((uResult >> 24) & 0xff) && isprint((uResult >> 16) & 0xff) &&
        isprint((uResult >>  8) & 0xff) && isprint((uResult >>  0) & 0xff))
    {
        ZPrintf("%s: status of ('%C', %d) is {'%C')\n", argv[0], uToken, iData, uResult);
    }
    else // display result as decimal and hex
    {
        ZPrintf("%s: status of ('%C', %d) is {%d/0x%08X)\n", argv[0], uToken, iData, uResult, uResult);
    }
}

/*F********************************************************************************/
/*!
    \Function _NetPorts

    \Description
        Like "netstat" on unix

    \Input *pApp    - pointer to upnp module
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false

    \Version 09/25/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _NetPorts(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    SocketT *pSocket;
    int32_t iResult;
    uint32_t uPort;
    struct sockaddr SockName;

    SockaddrInit(&SockName, AF_INET);

    // find ports that are bound
    for (uPort = 1024; uPort < 65536; uPort++)
    {
        // create a UDP socket
        if ((pSocket = SocketOpen(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == NULL)
        {
            NetPrintf(("net: error -- could not allocate socket resource\n"));
            return;
        }

        // bind the socket
        SockaddrInSetPort(&SockName, uPort);
        iResult = SocketBind(pSocket, &SockName, sizeof(SockName));
        if (iResult == SOCKERR_ADDRINUSE)
        {
            ZPrintf("net: %5s %5d\n", "UDP", uPort);
        }

        // close the socket
        if (SocketClose(pSocket) != 0)
        {
            NetPrintf(("net: error -- could not free socket resource\n"));
            return;
        }

        // give some time to network stack
        NetConnSleep(1);
        NetConnIdle();
    }
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function    CmdNet

    \Description
        Net command.

    \Input *argz    - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output
        int32_t     - zero

    \Version 11/24/04 (jbrookes)
*/
/**************************************************************************************F*/
int32_t CmdNet(ZContext *argz, int32_t argc, char *argv[])
{
    void *pCmdRef = &_Net_App;
    unsigned char bHelp;
    T2SubCmdT *pCmd;

    // handle shutdown
    if(argc == 0)
    {
        // nothing to do
        return(0);
    }
    // handle basic help
    else if ((argc < 2) || (((pCmd = T2SubCmdParse(_Net_Commands, argc, argv, &bHelp)) == NULL)))
    {
        T2SubCmdUsage(argv[0], _Net_Commands);
        return(0);
    }

    // hand off to command
    pCmd->pFunc(pCmdRef, argc, argv, bHelp);
    return(0);
}

