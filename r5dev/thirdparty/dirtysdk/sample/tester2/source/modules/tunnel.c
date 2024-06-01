/*H********************************************************************************/
/*!
    \File tunnel.c

    \Description
        A tester command to test ProtoTunnel

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 12/02/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <stdlib.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/voip/voip.h"
#include "DirtySDK/proto/prototunnel.h"

#include "libsample/zlib.h"
#include "libsample/zmem.h"
#include "testerregistry.h"
#include "testersubcmd.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct TunnelAppT
{
    ProtoTunnelRefT *pProtoTunnel;
    unsigned char bZCallback;
} TunnelAppT;

/*** Function Prototypes ***************************************************************/

static void _TunnelCreate(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _TunnelDestroy(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _TunnelAlloc(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _TunnelFree(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _TunnelControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);

/*** Variables ********************************************************************/

static T2SubCmdT _Tunnel_Commands[] =
{
    { "create",     _TunnelCreate   },
    { "destroy",    _TunnelDestroy  },
    { "alloc",      _TunnelAlloc    },
    { "free",       _TunnelFree     },
    { "ctrl",       _TunnelControl  },
    { "",           NULL            }
};

static TunnelAppT _Tunnel_App = { NULL, FALSE };


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _TunnelCreate
    
    \Description
        Tunnel subcommand - create tunnel module
    
    \Input *pApp    - pointer to tunnel module
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _TunnelCreate(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    TunnelAppT *pApp = &_Tunnel_App;
    int32_t iTunnelPort;
    uint32_t uClientId;

    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("   usage: %s create <clientId> [port]\n", argv[0]);
        return;
    }

    // get the port
    iTunnelPort = (argc == 4) ? (int32_t)strtol(argv[3], NULL, 10) : 9600;

    // create the tunnel
    pApp->pProtoTunnel = ProtoTunnelCreate(4, iTunnelPort);

    // set client id
    uClientId = (uint32_t)strtol(argv[2], NULL, 16);
    ProtoTunnelControl(pApp->pProtoTunnel, 'clid', uClientId, 0, NULL);
}

/*F********************************************************************************/
/*!
    \Function _TunnelDestroy
    
    \Description
        Tunnel subcommand - destroy tunnel module
    
    \Input *pApp    - pointer to tunnel module
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _TunnelDestroy(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    TunnelAppT *pApp = &_Tunnel_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s create\n", argv[0]);
        return;
    }

    if (pApp->pProtoTunnel != NULL)
    {
        ProtoTunnelDestroy(pApp->pProtoTunnel);
        ds_memclr(&pApp, sizeof(pApp));
    }
}

/*F********************************************************************************/
/*!
    \Function _TunnelAlloc
    
    \Description
        Tunnel subcommand - allocate a tunnel
    
    \Input *pApp    - pointer to tunnel module
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 12/07/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _TunnelAlloc(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    TunnelAppT *pApp = &_Tunnel_App;
    ProtoTunnelInfoT Info;
    char *pKey, strKey[] = "24906usdfnasdf1&";

    if ((bHelp == TRUE) || (argc < 5))
    {
        ZPrintf("   usage: %s alloc [clientId] [addr] [port] <key>\n", argv[0]);
        return;
    }

    // ref key
    pKey = (argc == 6) ? argv[5] : strKey;
    
    ds_memclr(&Info, sizeof(Info));
    Info.uRemoteClientId = (uint32_t)strtol(argv[2], 0, 16);
    Info.uRemoteAddr = SocketInTextGetAddr(argv[3]);
    Info.uRemotePort = strtol(argv[4], 0, 10);
    //$$ hardcode for now
    Info.aRemotePortList[0] = 3658;
    Info.aPortFlags[0] = PROTOTUNNEL_PORTFLAG_ENCRYPTED;
    Info.aRemotePortList[1] = VOIP_PORT;
    Info.aPortFlags[1] = PROTOTUNNEL_PORTFLAG_ENCRYPTED;
    
    // allocate a mapping
    ZPrintf("%s: tunnel alloc clientId=0x%08x addr=%a port=%d key=%s\n", argv[0], Info.uRemoteClientId, Info.uRemoteAddr, Info.uRemotePort, pKey);
    ProtoTunnelAlloc(pApp->pProtoTunnel, &Info, pKey);
}

/*F********************************************************************************/
/*!
    \Function _TunnelFree
    
    \Description
        Tunnel subcommand - free a tunnel
    
    \Input *pApp    - pointer to tunnel module
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 12/07/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _TunnelFree(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    TunnelAppT *pApp = &_Tunnel_App;
    uint32_t uTunnelId;
    const char *pKey;

    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("   usage: %s free [tunnelId] <key>\n", argv[0]);
        return;
    }

    // ref key
    pKey = (argc == 4) ? argv[3] : NULL;
    
    // free a mapping
    uTunnelId = (uint32_t)strtol(argv[2], NULL, 16);
    ProtoTunnelFree(pApp->pProtoTunnel, uTunnelId, pKey);
}

/*F********************************************************************************/
/*!
    \Function _TunnelControl
    
    \Description
        Tunnel subcommand - execute a UPnP command
    
    \Input *pApp    - pointer to tunnel module
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _TunnelControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    TunnelAppT *pApp = &_Tunnel_App;
    int32_t iCmd, iValue, iValue2;
    const char *pValue=NULL;

    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("   usage: %s ctrl <command> [parm]...\n", argv[0]);
        return;
    }

    iCmd  = argv[2][0] << 24;
    iCmd |= argv[2][1] << 16;
    iCmd |= argv[2][2] << 8;
    iCmd |= argv[2][3];
    
    iValue = (argc > 3) ? (int32_t)strtol(argv[3], NULL, 10) : 0;
    iValue2 = (argc > 4) ? (int32_t)strtol(argv[4], NULL, 10) : 0;

    ZPrintf("tunnel: executing ProtoTunnelControl(pProtoTunnel, '%s', %d, %d, %s)\n", argv[2], iValue, iValue2, pValue ? pValue : "(null)");
    ProtoTunnelControl(pApp->pProtoTunnel, iCmd, iValue, iValue2, pValue);
}

/*F********************************************************************************/
/*!
    \Function _CmdTunnelCb

    \Description
        Tunnel callback, called after command has been issued.

    \Input *argz    - pointer to context
    \Input argc     - number of command-line arguments
    \Input *argv[]   - command-line argument list
    
    \Output
        int32_t         - result of zcallback, or zero to terminate

    \Version 1.0 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdTunnelCb(ZContext *argz, int32_t argc, char *argv[])
{
    TunnelAppT *pApp = &_Tunnel_App;

    // check for kill
    if (argc == 0)
    {
        ZPrintf("%s: killed\n", argv[0]);
        ProtoTunnelDestroy(pApp->pProtoTunnel);
        return(0);
    }

    // update module
    if (pApp->pProtoTunnel != NULL)
    {
        ProtoTunnelUpdate(pApp->pProtoTunnel);
    }

    // keep recurring
    return(ZCallback(&_CmdTunnelCb, 17));
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdTunnel

    \Description
        Tunnel command.  This command starts the ProtoTunnel module.

    \Input *argz    - pointer to context
    \Input argc     - number of command-line arguments
    \Input *argv[]  - command-line argument list
    
    \Output
        int32_t     - result of zcallback

    \Version 1.0 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdTunnel(ZContext *argz, int32_t argc, char *argv[])
{
    T2SubCmdT *pCmd;
    TunnelAppT *pApp = &_Tunnel_App;
    unsigned char bHelp;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_Tunnel_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   test the prototunnel module\n");
        T2SubCmdUsage(argv[0], _Tunnel_Commands);
        return(0);
    }

    // if no ref yet, make one
    if ((pCmd->pFunc != _TunnelCreate) && (pApp->pProtoTunnel == NULL))
    {
        char *pCreate = "create";
        ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
        _TunnelCreate(pApp, 1, &pCreate, bHelp);
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    // one-time install of periodic callback
    if (pApp->bZCallback == FALSE)
    {
        pApp->bZCallback = TRUE;
        return(ZCallback(_CmdTunnelCb, 17));
    }
    else
    {
        return(0);
    }
}
