/*H********************************************************************************/
/*!
    \File gamelink.c

    \Description
        Test NetGameLink

    \Copyright
        Copyright (c) Electronic Arts 2018.

    \Version 02/02/2018 (jbrookes)  First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#ifdef _WIN32
 #pragma warning(push,0)
 #include <windows.h>
 #pragma warning(pop)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/game/netgameutil.h"
#include "DirtySDK/game/netgamelink.h"
#include "DirtySDK/comm/commudp.h"

#if defined(DIRTYCODE_XBOXONE)
#include "DirtySDK/DirtySock/dirtyaddr.h"
#endif

#include "libsample/zlib.h"
#include "libsample/zfile.h"
#include "libsample/zmem.h"
#include "testersubcmd.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

//! gamelink states
enum
{
    GAMELINK_STATUS_DISC,
    GAMELINK_STATUS_CONN,
    GAMELINK_STATUS_OPEN,
    GAMELINK_STATUS_ACTV
};

//! default gamelink port
#define GAMELINK_PORT (3658)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct GameLinkAppT
{
    NetGameUtilRefT *pGameUtil;
    NetGameLinkRefT *pGameLink;
    uint32_t uLoclAddr;
    uint8_t bZCallback;
    uint8_t uLinkStatus;
    uint8_t uPackSendVal;
    uint8_t _pad[2];
} GameLinkAppT;

/*** Function Prototypes **********************************************************/

static void _GameLinkCreate(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _GameLinkDestroy(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _GameLinkConnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _GameLinkControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _GameLinkSend(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);

/*** Variables ********************************************************************/

static T2SubCmdT _GameLink_Commands[] =
{
    { "create",     _GameLinkCreate         },
    { "destroy",    _GameLinkDestroy        },
    { "connect",    _GameLinkConnect        },
    { "ctrl",       _GameLinkControl        },
    { "send",       _GameLinkSend           },
    { "",           NULL                    },
};

static GameLinkAppT _GameLink_App = { 0 };

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _GetIntArg

    \Description
        Get fourcc/integer from command-line argument

    \Input *pArg        - pointer to argument

    \Version 10/20/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _GetIntArg(const char *pArg)
{
    int32_t iValue;

    // check for possible fourcc value
    if ((strlen(pArg) == 4) && (isalpha(pArg[0]) || isalpha(pArg[1]) || isalpha(pArg[2]) || isalpha(pArg[3])))
    {
        iValue  = pArg[0] << 24;
        iValue |= pArg[1] << 16;
        iValue |= pArg[2] << 8;
        iValue |= pArg[3];
    }
    else
    {
        iValue = (signed)strtol(pArg, NULL, 10);
    }
    return(iValue);
}

/*F********************************************************************************/
/*!
    \Function _GetConnectParms

    \Description
        Create connection string for gamelink connection

    \Input *pConnName   - [out] connection string
    \Input iNameSize    - size of connname buffer
    \Input uLoclAddr    - local address
    \Input uLoclPort    - local port
    \Input uConnAddr    - peer address
    \Input uConnPort    - peer port

    \Output
        int32_t         - connection flags

    \Version 02/02/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _GetConnectParms(char *pConnName, int32_t iNameSize, uint32_t uLoclAddr, uint16_t uLoclPort, uint32_t uConnAddr, uint16_t uConnPort)
{
    char strLoclAddr[32], strConnAddr[32];
    int32_t iConnFlags = NETGAME_CONN_CONNECT;
    uint32_t uAddrTemp;
    uint8_t bHosting;

    ds_snzprintf(strLoclAddr, sizeof(strLoclAddr), "%a:%d", uLoclAddr, uLoclPort);
    ds_snzprintf(strConnAddr, sizeof(strConnAddr), "%a:%d", uConnAddr, uConnPort);

    bHosting = (strcmp(strLoclAddr, strConnAddr) < 0) ? TRUE : FALSE;

    /* set up parms based on whether we are "hosting" or not.  the connection name is the
       unique address of the "host" concatenated with the unique address of the "client" */
    if (bHosting == TRUE)
    {
        // swap names
        uAddrTemp = uConnAddr;
        uConnAddr = uLoclAddr;
        uLoclAddr = uAddrTemp;
        // set conn flags to listen
        iConnFlags = NETGAME_CONN_LISTEN;
    }

    // format connection name and return connection flags
    ds_snzprintf(pConnName, iNameSize, "%x-%x", uLoclAddr, uConnAddr);
    return(iConnFlags);
}

/*F********************************************************************************/
/*!
    \Function _GameUtilConnect

    \Description
        Connect to peer

    \Input *pApp        - module state
    \Input uLoclAddr    - local address
    \Input uLoclPort    - local port
    \Input uConnAddr    - peer address
    \Input uConnPort    - peer port

    \Output
        int32_t         - result of NetGameUtilConnect, or -1 on failure

    \Version 02/02/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _GameUtilConnect(GameLinkAppT *pApp, uint32_t uLoclAddr, uint16_t uLoclPort, uint32_t uConnAddr, uint16_t uConnPort)
{
    char strConn[128], strConnName[64];
    int32_t iConnFlags;

    if ((pApp->pGameUtil == NULL) && ((pApp->pGameUtil = NetGameUtilCreate()) == NULL))
    {
        ZPrintf("gamelink: could not create util ref\n");
        return(-1);
    }

    // set game link options
#if 0
    _SetGamelinkOpt(pClient->pGameUtilRef, 'minp', pConnApi->iGameMinp);
    _SetGamelinkOpt(pClient->pGameUtilRef, 'mout', pConnApi->iGameMout);
    _SetGamelinkOpt(pClient->pGameUtilRef, 'mwid', pConnApi->iGameMwid);
    if (pConnApi->iGameUnackLimit != 0)
    {
        _SetGamelinkOpt(pClient->pGameUtilRef, 'ulmt', pConnApi->iGameUnackLimit);
    }

    // set our client id (used by gameserver to uniquely identify us)
    NetGameUtilControl(pClient->pGameUtilRef, 'clid', pClient->ClientInfo.uLocalClientId);
    NetGameUtilControl(pClient->pGameUtilRef, 'rcid', pClient->ClientInfo.uRemoteClientId);
#endif

    // determine connection parameters
    iConnFlags = _GetConnectParms(strConnName, sizeof(strConnName), uLoclAddr, uLoclPort, uConnAddr, uConnPort);

    // format connect string
    ds_snzprintf(strConn, sizeof(strConn), "%a:%d:%d#%s", uConnAddr, uLoclPort, uConnPort, strConnName);

    // start the connection attempt
    return(NetGameUtilConnect(pApp->pGameUtil, iConnFlags, strConn, (CommAllConstructT *)CommUDPConstruct));
}

/*F********************************************************************************/
/*!
    \Function _GameLinkDestroyApp

    \Description
        Destroy app, clearing state

    \Input *pApp    - app state

    \Version 02/02/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _GameLinkDestroyApp(GameLinkAppT *pApp)
{
    ds_memclr(pApp, sizeof(*pApp));
}

/*

    GameLink Commands

*/

/*F*************************************************************************************/
/*!
    \Function _GameLinkCreate

    \Description
        GameLink subcommand - create gamelink module

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - TRUE if usage request

    \Version 02/02/2018 (jbrookes)
*/
/**************************************************************************************F*/
static void _GameLinkCreate(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    GameLinkAppT *pApp = &_GameLink_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s create\n", argv[0]);
        return;
    }

    pApp->uLoclAddr = SocketInfo(NULL, 'addr', 0, NULL, 0);
}

/*F*************************************************************************************/
/*!
    \Function _GameLinkDestroy

    \Description
        GameLink subcommand - destroy gamelink module

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - TRUE if usage request

    \Version 02/02/2018 (jbrookes)
*/
/**************************************************************************************F*/
static void _GameLinkDestroy(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    GameLinkAppT *pApp = &_GameLink_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s destroy\n", argv[0]);
        return;
    }

    _GameLinkDestroyApp(pApp);
}

/*F*************************************************************************************/
/*!
    \Function _GameLinkConnect

    \Description
        GameLink subcommand - connect to peer

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - TRUE if usage request

    \Version 02/02/2018 (jbrookes)
*/
/**************************************************************************************F*/
static void _GameLinkConnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    GameLinkAppT *pApp = &_GameLink_App;
    uint32_t uConnAddr, uConnPort, uLoclPort, uRemoteClientId;

    if ((bHelp == TRUE) || (argc > 4))
    {
        ZPrintf("   usage: %s connect <address[:port:port]> [rclientid]\n", argv[0]);
        return;
    }

    // get remote address, port, and local port (if specified) in addr:port:port2 format
    SockaddrInParse2(&uConnAddr, (int32_t *)&uConnPort, (int32_t *)&uLoclPort, argv[2]);
    if (uConnPort == 0)
    {
        uConnPort = GAMELINK_PORT;
    }
    if (uLoclPort == 0)
    {
        uLoclPort = GAMELINK_PORT;
    }

    // get remote client id, or use remote address if unspecified
    uRemoteClientId = (argc == 4) ? (uint32_t)strtol(argv[3], NULL, 16) : uConnAddr;

    // start the connect
    if (_GameUtilConnect(pApp, pApp->uLoclAddr, uLoclPort, uConnAddr, uConnPort) < 0)
    {
        ZPrintf("%s: error trying to connect\n");
        return;
    }
    // log connection attempt, uptdate status
    ZPrintf("%s: connecting to %a:%u:%u (clientId=0x%08x)\n", argv[0], uConnAddr, uConnPort, uLoclPort, uRemoteClientId);
    pApp->uLinkStatus = GAMELINK_STATUS_CONN;
}

/*F*************************************************************************************/
/*!
    \Function _GameLinkControl

    \Description
        GameLink control subcommand - set control options

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - TRUE if usage request

    \Version 02/02/2018 (jbrookes)
*/
/**************************************************************************************F*/
static void _GameLinkControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    GameLinkAppT *pApp = &_GameLink_App;
    int32_t iCmd, iValue = 0;

    if ((argc < 3) && (argc > 4))
    {
        ZPrintf("%s: invalid ctrl command\n", argv[0]);
        bHelp = TRUE;
    }
    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s ctrl <cmd> <arg>\n", argv[0]);
        return;
    }

    // get control command
    iCmd  = argv[2][0] << 24;
    iCmd |= argv[2][1] << 16;
    iCmd |= argv[2][2] << 8;
    iCmd |= argv[2][3];

    // get control argument, if specified
    if (argc > 3)
    {
        iValue = _GetIntArg(argv[3]);
    }

    // pass it down
    NetGameLinkControl(pApp->pGameLink, iCmd, iValue, NULL);
}

/*F*************************************************************************************/
/*!
    \Function _GameLinkSend

    \Description
        GameLink subcommand - send data

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - TRUE if usage request

    \Version 02/02/2018 (jbrookes)
*/
/**************************************************************************************F*/
static void _GameLinkSend(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    GameLinkAppT *pApp = &_GameLink_App;
    NetGameMaxPacketT Packet;
    int32_t iSize, iResult;

    if (argc != 3)
    {
        ZPrintf("%s: invalid send command\n", argv[0]);
        bHelp = TRUE;
    }
    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s send <size>\n", argv[0]);
        return;
    }

    // get size
    if (((iSize = _GetIntArg(argv[2])) < 0) || (iSize > NETGAME_DATAPKT_MAXSIZE))
    {
        ZPrintf("%s: size %d invalid, setting size to %d\n", NETGAME_DATAPKT_MAXSIZE);
        iSize = NETGAME_DATAPKT_MAXSIZE;
    }

    // format packet head
    ds_memclr(&Packet.head, sizeof(Packet.head));
    Packet.head.kind = GAME_PACKET_USER;
    Packet.head.len = iSize;

    // format packet data
    ds_memset(&Packet.body.data, pApp->uPackSendVal++, iSize);

    // send the packet
    iResult = NetGameLinkSend(pApp->pGameLink, (NetGamePacketT *)&Packet, 1);
    ZPrintf("%s: sent %d bytes\n", argv[0], iResult);
}

/*F********************************************************************************/
/*!
    \Function _CmdGameLinkCb

    \Description
        Update gamelink command

    \Input *argz    - environment
    \Input argc     - standard number of arguments
    \Input *argv[]  - standard arg list

    \Output
        int32_t     - standard return value

    \Version 02/02/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdGameLinkCb(ZContext *argz, int32_t argc, char *argv[])
{
    GameLinkAppT *pApp = &_GameLink_App;

    // check for kill
    if (argc == 0)
    {
        _GameLinkDestroyApp(pApp);
        ZPrintf("%s: killed\n", argv[0]);
        return(0);
    }

    // update in connecting state
    if (pApp->uLinkStatus == GAMELINK_STATUS_CONN)
    {
        CommRef *pCommRef;

        // check for established connection
        if ((pCommRef = NetGameUtilComplete(pApp->pGameUtil)) != NULL)
        {
            if ((pApp->pGameLink = NetGameLinkCreate(pCommRef, FALSE, 8192)) != NULL)
            {
                ZPrintf("%s: game connection established\n", argv[0]);
                // indicate we've connected
                pApp->uLinkStatus = GAMELINK_STATUS_OPEN;
            }
            else
            {
                ZPrintf("%s: game connection failed\n", argv[0]);
                pApp->uLinkStatus = GAMELINK_STATUS_DISC;
            }
        }
    }

    // update in open state
    if (pApp->uLinkStatus == GAMELINK_STATUS_OPEN)
    {
        NetGameLinkStatT Stat;

        // give time to NetGameLink to run any connection-related processes
        NetGameLinkUpdate(pApp->pGameLink);

        // get link stats
        NetGameLinkStatus(pApp->pGameLink, 'stat', 0, &Stat, sizeof(NetGameLinkStatT));

        // see if we're open
        if (Stat.isopen == TRUE)
        {
            // mark as active
            ZPrintf("%s: game connection is active\n", argv[0]);
            pApp->uLinkStatus = GAMELINK_STATUS_ACTV;
        }
    }

    // update in active state
    if (pApp->uLinkStatus == GAMELINK_STATUS_ACTV)
    {
        NetGameMaxPacketT Packet;
        NetGameLinkStatT Stat;

        // get link stats
        NetGameLinkStatus(pApp->pGameLink, 'stat', 0, &Stat, sizeof(NetGameLinkStatT));

        // make sure connection is still open
        if (Stat.isopen == FALSE)
        {
            ZPrintf("%s: game connection closed\n", argv[0]);
            pApp->uLinkStatus = GAMELINK_STATUS_DISC;
        }
        // see if we've timed out
        else if (NetTickDiff(Stat.tick, Stat.rcvdlast) > 20000)
        {
            ZPrintf("%s: game connection timed out\n", argv[0]);   
            pApp->uLinkStatus = GAMELINK_STATUS_DISC;
        }

        // try and receive something; if we get something echo it to output
        if (NetGameLinkRecv(pApp->pGameLink, (NetGamePacketT *)&Packet, sizeof(Packet.body.data), FALSE) > 0)
        {
            // cap max size to echo
            #if DIRTYCODE_DEBUG
            int32_t iSize = DS_MIN(Packet.head.len, 64);
            NetPrintMem(Packet.body.data, iSize, "packet data");
            #endif
        }
    }

    // keep running
    return(ZCallback(&_CmdGameLinkCb, 16));
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdGameLink

    \Description
        Initiate GameLink connection.

    \Input *argz    - environment
    \Input argc     - standard number of arguments
    \Input *argv[]  - standard arg list

    \Output
        int32_t     - standard return value

    \Version 02/02/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdGameLink(ZContext *argz, int32_t argc, char *argv[])
{
    GameLinkAppT *pApp = &_GameLink_App;
    T2SubCmdT *pCmd;
    uint8_t bHelp;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_GameLink_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   test the gamelink module\n");
        T2SubCmdUsage(argv[0], _GameLink_Commands);
        return(0);
    }

    // if no ref yet, make one
    if ((pCmd->pFunc != _GameLinkCreate) && (pApp->pGameLink == NULL))
    {
        char *pCreate = "create";
        ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
        _GameLinkCreate(pApp, 1, &pCreate, bHelp);
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    // one-time install of periodic callback
    if (pApp->bZCallback == FALSE)
    {
        pApp->bZCallback = TRUE;
        return(ZCallback(_CmdGameLinkCb, 16));
    }
    else
    {
        return(0);
    }
}
