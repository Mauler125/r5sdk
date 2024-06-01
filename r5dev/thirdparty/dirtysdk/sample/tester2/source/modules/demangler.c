/*H********************************************************************************/
/*!
    \File demangler.c

    \Description
        A simple tester of the EA.com demangler service, using ProtoMangle.

    \Copyright
        Copyright (c) 2003-2005 Electronic Arts Inc.

    \Version 04/03/2003 (jbrookes) First Version
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/proto/protomangle.h"
#include "DirtySDK/game/netgameutil.h"
#include "DirtySDK/game/netgamelink.h"
#include "DirtySDK/game/netgamepkt.h"
#include "DirtySDK/comm/commudp.h"

#include "libsample/zlib.h"

#include "testermodules.h"

/*** Defines **********************************************************************/

#define DEMANGLER_CONN_TIMEOUT  (15 * 1000)
#define DEMANGLER_MNGL_TIMEOUT  (15 * 1000)
#define DEMANGLER_SHAREDSOCKETTEST  (FALSE) // note: this test is a hack and is not valid on PS2

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct DemanglerRefT
{
    ProtoMangleRefT *pMangler;
    int32_t         bRequestIssued;
    int32_t         iGamePort;
    int32_t         iPeerAddr;
    int32_t         iPeerPort;
    int32_t         iConnType;
    int32_t         iMnglStart;
    int32_t         iConnStart;
    char            strSessID[32];

    NetGameUtilRefT *pUtilRef;
    NetGameLinkRefT *pLinkRef;

    int32_t         callcnt;

    #if DEMANGLER_SHAREDSOCKETTEST
    SocketT         *pSocket;
    #endif

    // connection state
    enum
    {
        IDLE, INIT, CONNINIT, LISTEN, FAIL, CONN, SHUTDOWN
    } state;
} DemanglerRefT;

/*** Function Prototypes ***************************************************************/


/*** Variables *************************************************************************/


// Private variables

static DemanglerRefT *_pDemanglerRef = NULL;

// Public variables


/*** Private Functions *****************************************************************/

/*F********************************************************************************/
/*!
    \Function _CmdDemanglerCB

    \Description
        Update demangling process.

    \Input *argz    -
    \Input argc     -
    \Input **argv   -
    
    \Output int32_t -

    \Version 04/03/2003 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdDemanglerCB(ZContext *argz, int32_t argc, char **argv)
{
    DemanglerRefT *pRef = (DemanglerRefT *)argz;
    int32_t iResult;

    // check for kill
    if (argc == 0)
    {
        if (pRef->pMangler != NULL)
        {
            ProtoMangleDestroy(pRef->pMangler);
        }
        if (pRef->pLinkRef != NULL)
        {
            NetGameLinkDestroy(pRef->pLinkRef);
        }
        if (pRef->pUtilRef != NULL)
        {
            NetGameUtilDestroy(pRef->pUtilRef);
        }
        return(0);
    }

    // update demangler client
    if (pRef->pMangler != NULL)
    {
        ProtoMangleUpdate(pRef->pMangler);
    }

    if (pRef->state == INIT)
    {
        if (pRef->bRequestIssued == FALSE)
        {
            #if DEMANGLER_SHAREDSOCKETTEST
            // create socket and bind it to game port
            if (pRef->pSocket == NULL)
            {
                struct sockaddr bindaddr;

                pRef->pSocket = SocketOpen(AF_INET, SOCK_DGRAM, 0);
                SockaddrInit(&bindaddr, AF_INET);
                SockaddrInSetPort(&bindaddr, pRef->iGamePort);
                SocketBind(pRef->pSocket, &bindaddr, sizeof(bindaddr));
            }
            ProtoMangleConnectSocket(pRef->pMangler, (uint32_t)pRef->pSocket, pRef->strSessID);
            #else
            ProtoMangleConnect(pRef->pMangler, pRef->iGamePort, pRef->strSessID);
            #endif
            pRef->bRequestIssued = TRUE;
            pRef->iMnglStart = NetTick();
            ZPrintf("demangler: connect issued at %d\n", NetTick());
        }

        if ((iResult = ProtoMangleComplete(pRef->pMangler, &pRef->iPeerAddr, &pRef->iPeerPort)) > 0)
        {
            ZPrintf("Demangler successful:\n");
            ZPrintf("  iPeerAddr: %a\n", pRef->iPeerAddr);
            ZPrintf("  iPeerPort: %d\n", pRef->iPeerPort);

            pRef->state = CONNINIT;
        }
        else if (iResult < 0)
        {
            if (ProtoMangleStatus(pRef->pMangler, 'time', NULL, 0) == TRUE)
            {
                ZPrintf("demangler: timeout at %d\n", NetTick());
            }
            else
            {
                ZPrintf("demangler: ProtoMangleComplete()=%d\n", iResult);
            }
            pRef->state = IDLE;
        }
    }

    if (pRef->state == CONNINIT)
    {
        char strAddr[48];

        // create utilref
        pRef->pUtilRef = NetGameUtilCreate();
        ds_snzprintf(strAddr, sizeof(strAddr), "%a:%d:%d", pRef->iPeerAddr, pRef->iGamePort, pRef->iPeerPort);
        NetGameUtilConnect(pRef->pUtilRef, pRef->iConnType, strAddr, (CommAllConstructT *)CommUDPConstruct);

        pRef->iConnStart = NetTick();
        pRef->state = LISTEN;
    }

    if (pRef->state == LISTEN)
    {
        void *pCommRef;

        // check for connect
        pCommRef = NetGameUtilComplete(pRef->pUtilRef);
        if (pCommRef != NULL)
        {
            // got a connect -- startup link
            ZPrintf("%s: comm established\n", argv[0]);
            pRef->pLinkRef = NetGameLinkCreate(pCommRef, FALSE, 8192);
            ZPrintf("%s: link running\n", argv[0]);
            pRef->state = CONN;

            // report success to protomangle server
            ProtoMangleReport(pRef->pMangler, PROTOMANGLE_STATUS_CONNECTED, -1);
        }

        // check for timeout
        if ((NetTick() - pRef->iConnStart) > DEMANGLER_CONN_TIMEOUT)
        {
            // report failure to protomangle server
            ProtoMangleReport(pRef->pMangler, PROTOMANGLE_STATUS_FAILED, -1);
            pRef->state = FAIL;
        }
    }

    if (pRef->state == CONN)
    {
        NetGameLinkStatT Stats;

        // check connection status
        NetGameLinkStatus(pRef->pLinkRef, 'stat', 0, &Stats, sizeof(NetGameLinkStatT));
        if (Stats.isopen)
        {
            NetGamePacketT TestPacket;

            // send an unreliable packet
            TestPacket.head.kind = GAME_PACKET_USER_UNRELIABLE;
            TestPacket.head.len = NETGAME_DATAPKT_DEFSIZE;
            sprintf((char *)TestPacket.body.data, "demangler test packet %d", pRef->callcnt);
            if (NetGameLinkSend(pRef->pLinkRef,&TestPacket,1) > 0)
            {
                pRef->callcnt++;
            }

            // check for receive
            while (NetGameLinkRecv(pRef->pLinkRef,&TestPacket,1,FALSE) > 0)
            {
                // sanity checks that the data got here OK
                if (TestPacket.head.len != NETGAME_DATAPKT_DEFSIZE)
                {
                    ZPrintf("packet size invalid\n");
                }
                if ((TestPacket.head.kind != GAME_PACKET_USER_UNRELIABLE) && (TestPacket.head.kind != GAME_PACKET_USER))
                {
                    ZPrintf("packet type invalid\n");
                }

                // output data
                ZPrintf("%s\n",(char *)TestPacket.body.data);
            }
        }
    }

    if ((pRef->state == CONN) || (pRef->state == FAIL))
    {
        // update ProtoMangle to send response to server
        if (pRef->pMangler != NULL)
        {
            if (ProtoMangleComplete(pRef->pMangler, NULL, NULL) != 0)
            {
                ZPrintf("posted result to server\n");
            }
        }
    }

    return(ZCallback(&_CmdDemanglerCB, 100));
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdDemangler
        A simple tester of the EA.com demangler service, using ProtoMangle.
    
    \Description
    
    \Input *argz    -
    \Input argc     -
    \Input **argv   -
    
    \Output
        int32_t         -
            
    \Version 04/03/2003 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdDemangler(ZContext *argz, int32_t argc, char **argv)
{
    const char *pServer, *pSessId;
    uint8_t bReuseRef = TRUE;
    DemanglerRefT *pRef;

    // usage
    if ((argc < 3) || (argc > 4) || (strcmp(argv[1], "conn") && strcmp(argv[1], "list") && strcmp(argv[1], "disc") && strcmp(argv[1], "dest")))
    {
        ZPrintf("   connect to a peer using demangler server\n");
        ZPrintf("   usage: %s [conn|list|disc|dest] <server> [sessID]\n", argv[0]);
        return(0);
    }

    // destroy?
    if (!strcmp(argv[1], "dest"))
    {
        if (_pDemanglerRef != NULL)
        {
            _CmdDemanglerCB((ZContext *)_pDemanglerRef, 0, NULL);
            _pDemanglerRef = NULL;
        }
        else
        {
            ZPrintf("   %s: no module to destroy\n", argv[0]);
        }
        return(0);
    }

    // disconnect?
    if (!strcmp(argv[1], "disc"))
    {
        if (_pDemanglerRef != NULL)
        {
            if (_pDemanglerRef->pLinkRef != NULL)
            {
                NetGameLinkDestroy(_pDemanglerRef->pLinkRef);
            }
            if (_pDemanglerRef->pUtilRef != NULL)
            {
                NetGameUtilDestroy(_pDemanglerRef->pUtilRef);
            }
        }
        return(0);
    }

    // resolve server and session identifer
    if (argc == 4)
    {
        pServer = argv[2];
        pSessId = argv[3];
    }
    else
    {
        pServer = PROTOMANGLE_SERVER;
        pSessId = argv[2];
    }

    // allocate context?
    if ((pRef = _pDemanglerRef) == NULL)
    {
        pRef = (DemanglerRefT *) ZContextCreate(sizeof(*pRef));
        ds_memclr(pRef, sizeof(*pRef));

        pRef->pMangler = ProtoMangleCreate(pServer, PROTOMANGLE_PORT, "mangletest-pc-2004", "");
        ProtoMangleControl(pRef->pMangler, 'time', DEMANGLER_MNGL_TIMEOUT, 0, NULL);

        _pDemanglerRef = pRef;
        bReuseRef = FALSE;
    }

    strcpy(pRef->strSessID, pSessId);
    pRef->iConnType = (!strcmp(argv[1],"conn")) ? NETGAME_CONN_CONNECT : NETGAME_CONN_LISTEN;
    pRef->state = INIT;
    pRef->iGamePort = 10000;
    pRef->bRequestIssued = FALSE;

    return(bReuseRef ? 0 : ZCallback(&_CmdDemanglerCB, 100));
}
