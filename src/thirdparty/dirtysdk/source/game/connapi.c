/*H********************************************************************************/
/*!
    \File connapi.c

    \Description
        ConnApi is a high-level connection manager, that packages the "connect to
        peer" process into a single module.  Both game connections and voice
        connections can be managed.  Multiple peers are supported in a host/client
        model for the game connection, and a peer/peer model for the voice
        connections.

    \Copyright
        Copyright (c) Electronic Arts 2005. ALL RIGHTS RESERVED.

    \Notes
        About ConnApi using ProtoMangle:

            For plaforms other than xboxone:
                * ConnApi uses ProtoMangle to exercice connection demangling logic
                  with a central demangler service.
                * When demangler is enabled, a typical connection state transition is:
                    success:  INIT --> CONN --> ACTV
                    success after demangling:  INIT --> CONN --> MNGL --> INIT --> CONN --> ACTV
                * For each connection, the boolean ConnApiConnInfoT::bDemangling is used to 
                  track "demangling being in progress" across the "MNGL --> INIT --> CONN --> ACTV"
                  state sequence such that ProtoMangleReport() gets called appropriately
                  once post-demangling conn attempt finishes.
                * Additionally, the boolean ConnApiConnInfoT::bDemangling is used to "serialize"
                  demangling attempts as interaction with the demangler is limited to one session
                  at a time.

             For xboxone:
                * The central demangling service is never used, but the secure device association
                  creation is mapped to the protomangle metaphore.
                * A typical connection state transition is:
                    p2p conn (SecureAssoc needed):  MNGL --> INIT --> CONN --> ACTV
                    server conn (SecureAssoc not needed):  INIT --> CONN --> ACTV
                * For each connection, the boolean ConnApiConnInfoT::bDemangling no longer means
                  "demangling being in progress" across states. It's rather used in such way that,
                  for server conns, if the conn attempt fails, then the MGNL state is not entered.


    \Version 01/04/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtynames.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/proto/protomangle.h"
#include "DirtySDK/proto/prototunnel.h"
#include "DirtySDK/voip/voip.h"
#include "DirtySDK/voip/voipgroup.h"

#include "DirtySDK/game/connapi.h"

/*** Defines **********************************************************************/

//! define if we're using Xbox networking
#if defined(DIRTYCODE_XBOXONE) && !defined(DIRTYCODE_GDK)
#define CONNAPI_XBOX_NETWORKING 1
#else
#define CONNAPI_XBOX_NETWORKING 0
#endif

//! default connapi timeout
#define CONNAPI_TIMEOUT_DEFAULT     (15*1000)

//! connapi connection timeout
#define CONNAPI_CONNTIMEOUT_DEFAULT (10*1000)

//! connapi default demangler timeout (per user)
#if CONNAPI_XBOX_NETWORKING
#define CONNAPI_DEMANGLER_TIMEOUT               (2*CONNAPI_CONNTIMEOUT_DEFAULT)  // experimentation showed that creation of security association can be significantly long
#define CONNAPI_DEMANGLER_WITH_FAILOVER_TIMEOUT (CONNAPI_CONNTIMEOUT_DEFAULT)    // however for scenarios involving CC assistance, we don't wait that long
#else
#define CONNAPI_DEMANGLER_TIMEOUT               (CONNAPI_CONNTIMEOUT_DEFAULT)
#define CONNAPI_DEMANGLER_WITH_FAILOVER_TIMEOUT (CONNAPI_DEMANGLER_TIMEOUT)
#endif

//! default GameLink buffer size
#define CONNAPI_LINKBUFDEFAULT      (1024)

//! test demangling
#define CONNAPI_DEMANGLE_TEST       (DIRTYCODE_DEBUG && FALSE)

//! random demangle local port?
#define CONNAPI_RNDLCLDEMANGLEPORT  (0) // used to be enabled for WII revolution platform

//! max number of registered callbacks
#define CONNAPI_MAX_CALLBACKS       (8)

//! connapi client flags
#define CONNAPI_CLIENTFLAG_REMOVE               (1) // remove client from clientlist
#define CONNAPI_CLIENTFLAG_TUNNELPORTDEMANGLED  (2) // tunnel port has been demangled
#define CONNAPI_CLIENTFLAG_SECUREADDRRESOLVED   (4) // secure address has been resolved (xboxone-specific)

//! debug flags
#define CONNAPI_CLIENTFLAG_P2PFAILDBG           (128)  // set to force P2P connections to fail

/*** Type Definitions *************************************************************/
struct ConnApiRefT
{
    //! connapi user callback info
    ConnApiCallbackT        *pCallback[CONNAPI_MAX_CALLBACKS];
    void                    *pUserData[CONNAPI_MAX_CALLBACKS];

    //! dirtymem memory group
    int32_t                 iMemGroup;
    void                    *pMemGroupUserData;

    //! game port to connect on
    uint16_t                uGamePort;

    //! voip port to connect on
    uint16_t                uVoipPort;

    //! game connection flags
    uint16_t                uConnFlags;

    //! netmask, used for external address comparisons
    uint32_t                uNetMask;

    //! game name
    char                    strGameName[32];

    //! game link buffer size
    int32_t                 iLinkBufSize;

    //! master game util ref (used for advertising)
    NetGameUtilRefT         *pGameUtilRef;

    //! protomangle ref
    ProtoMangleRefT         *pProtoMangle;

    //! prototunnel ref
    ProtoTunnelRefT         *pProtoTunnel;

    //! prototunnel port
    int32_t                 iTunnelPort;

    //! do we own tunnel ref?
    int32_t                 bTunnelOwner;

    //! protomangle server name
    char                    strDemanglerServer[48];

    //! voip ref
    VoipRefT                *pVoipRef;

    //! voipgroup ref
    VoipGroupRefT           *pVoipGroupRef;

    //! comm construct function
    CommAllConstructT       *pCommConstruct;

    //! our address
    DirtyAddrT              SelfAddr;

    //! our unique identifier
    uint32_t                uSelfId;

    //! index of ourself in client list
    int32_t                 iSelf;

    //! session identifier
    int32_t                 iSessId;

    //! connection timeout value
    int32_t                 iConnTimeout;

    //! timeout value
    int32_t                 iTimeout;

    //! demangler timeouts
    int32_t                 iConfigMnglTimeout;         // configured timeout for cases where CC assistance are not applicable
    int32_t                 iConfigMnglTimeoutFailover; // configured timeout for cases where CC assistance are applicable
    int32_t                 iCurrentMnglTimeout;        // effective timeout value to be passed down to ProtoMangle

    //! gamelink configuration - input packet queue size
    int32_t                 iGameMinp;

    //! gamelink configuration - output packet queue size
    int32_t                 iGameMout;

    //! gamelink configuration - max packet width
    int32_t                 iGameMwid;

    //! gamelink configuration - unacknowledged packet window
    int32_t                 iGameUnackLimit;

    //! Connection Concierge mode (CONNAPI_CCMODE_*)
    int32_t                 iCcMode;

    //! session information
    char                    strSession[128];

    //! demangler reporting?
    uint8_t                 bReporting;

    //! is demangler enabled?
    uint8_t                 bDemanglerEnabled;

    //! is tunnel enabled?
    uint8_t                 bTunnelEnabled;

    //! inside of a callback?
    uint8_t                 bInCallback;

    //! disc callback on client removal?
    uint8_t                 bRemoveCallback;

    //! auto-update enabled?
    uint8_t                 bAutoUpdate;

    #if CONNAPI_XBOX_NETWORKING
    //! 'exsn', the external session name - passed to the demangler
    char                    strExternalSessionName[128];

    //! 'estn', the external session template name - passed to the demangler
    char                    strExternalSessionTemplateName[128];

    //! 'scid', the service configuration id - passed to the demangler
    char                    strScid[128];
    #endif

    //! 'adve', TRUE if ProtoAdvt advertising enabled
    uint8_t                 bDoAdvertising;

    //! 'meta', TRUE if commudp metadata enabled
    uint8_t                 bCommUdpMetadata;

    //! game socket ref, if available
    uintptr_t               uGameSockRef;

    //! voip socket ref, if available
    uintptr_t               uVoipSockRef;

    //! tunnel socket ref, if available
    uintptr_t               uTunlSockRef;

    //! game host
    int32_t                 iGameHostIndex;
    //! voip host
    int32_t                 iVoipHostIndex;

    #if DIRTYCODE_DEBUG
    //! force direct connection to fail ?
    uint8_t                 bFailP2PConnect;
    #endif

    uint32_t                uGameTunnelFlag;
    uint32_t                uGameTunnelFlagOverride;

    int32_t                 iQosDuration;
    int32_t                 iQosInterval;
    int32_t                 iQosPacketSize;

    //! client state
    enum
    {
        ST_IDLE,            //!< idle
        ST_INGAME           //!< hosting or joined a game
    } eState;

    //! game network topology
    ConnApiGameTopologyE    eGameTopology;
    //! voip network topology
    ConnApiVoipTopologyE    eVoipTopology;

    //! client list - must come last in ref as it is variable length
    ConnApiClientListT      ClientList;
};

/*** Variables ********************************************************************/

//! number of voipgroups we allocate in the manager
static int8_t _ConnApi_iMaxVoipGroups = 8;

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _ConnApiDefaultCallback

    \Description
        Default ConnApi user callback.  On a debug build, displays state transition
        information.

    \Input *pConnApi    - connection manager ref
    \Input *pCbInfo     - connection info
    \Input *pUserData   - user callback data

    \Version 01/17/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiDefaultCallback(ConnApiRefT *pConnApi, ConnApiCbInfoT *pCbInfo, void *pUserData)
{
    #if DIRTYCODE_LOGGING
    static const char *_StateNames[CONNAPI_NUMSTATUSTYPES] =
    {
        "CONNAPI_STATUS_INIT",
        "CONNAPI_STATUS_CONN",
        "CONNAPI_STATUS_MNGL",
        "CONNAPI_STATUS_ACTV",
        "CONNAPI_STATUS_DISC"
    };
    static const char *_TypeNames[CONNAPI_NUMCBTYPES] =
    {
        "CONNAPI_CBTYPE_GAMEEVENT",
        "CONNAPI_CBTYPE_DESTEVENT",
        "CONNAPI_CBTYPE_VOIPEVENT",
    };

    // display state change
    NetPrintf(("connapi: [%p] client %d) [%s] %s -> %s\n", pConnApi, pCbInfo->iClientIndex, _TypeNames[pCbInfo->eType],
        _StateNames[pCbInfo->eOldStatus], _StateNames[pCbInfo->eNewStatus]));
    #endif
}

#if CONNAPI_RNDLCLDEMANGLEPORT
/*F********************************************************************************/
/*!
    \Function _ConnApiGenerateDemanglePort

    \Description
        Generate a "random" demangle port to use, actually generated based on the
        two lowest bytes of the MAC address and multiplied by a small prime number,
        and then modded into a small range of ports.  The intent here is to generate
        ports that will both not be reused on successive attempts and also be unlikely
        to collide with other client choices, in case there are other clients operating
        the same code behind the same NAT device.

    \Input *pConnApi    - connection manager ref

    \Output
        uint16_t        - generated demangle port to use

    \Version 03/02/2009 (jbrookes)
*/
/********************************************************************************F*/
static uint16_t _ConnApiGenerateDemanglePort(ConnApiRefT *pConnApi)
{
    static const uint16_t _uPortBase = 10000;
    static const uint16_t _uPortRange = 1000;
    static uint16_t _uPortOffset = 0xffff;

    // initialize port offset based on mac address to avoid collisions with other clients behind the same NAT
    if (_uPortOffset == 0xffff)
    {
        uint8_t aMacAddr[6];
        if (NetConnStatus('macx', 0, aMacAddr, sizeof(aMacAddr)) >= 0)
        {
            // generate mac-based port within specified range
            NetPrintf(("connapi: [%p] generating port offset based on MAC addr '%s'\n", pConnApi, NetConnMAC()));
            _uPortOffset = aMacAddr[4];
            _uPortOffset = (_uPortOffset << 8) + aMacAddr[5];
            _uPortOffset *= 7;
        }
        else
        {
            NetPrintf(("connapi: [%p] unable to acquire mac address\n", pConnApi));
            _uPortOffset = 0;
        }
    }

    // generate new port
    _uPortOffset = (_uPortOffset + 1) % _uPortRange;
    return(_uPortBase + _uPortOffset);
}
#endif

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function _ConnApiDisplayClientInfo

    \Description
        Debug-only function to print the given client info to debug output.

    \Input *pClient - pointer to client to display

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiDisplayClientInfo(ConnApiClientT *pClient, int32_t iClient)
{
    if (pClient->bAllocated)
    {
        NetPrintf(("connapi:   %d) id:0x%08x lid:0x%08x rid:0x%08x ip:%a hosted:%s qos:%s dirtyaddr:%s\n",
            iClient, pClient->ClientInfo.uId, pClient->ClientInfo.uLocalClientId, pClient->ClientInfo.uRemoteClientId,
            pClient->ClientInfo.uAddr, pClient->ClientInfo.bIsConnectivityHosted ? "TRUE" : "FALSE",
            pClient->ClientInfo.bEnableQos ? "TRUE" : "FALSE", pClient->ClientInfo.DirtyAddr.strMachineAddr));
    }
    else
    {
        NetPrintf(("connapi:   %d) empty\n", iClient));
    }
}
#endif

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function _ConnApiDisplayClientList

    \Description
        Debug-only function to print the given client list to debug output.

    \Input *pClientList - pointer to client list to display

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiDisplayClientList(ConnApiClientListT *pClientList)
{
    int32_t iClient;

    NetPrintf(("connapi: clientlist display\n"));
    for (iClient = 0; iClient < pClientList->iMaxClients; iClient++)
    {
        _ConnApiDisplayClientInfo(&pClientList->Clients[iClient], iClient);
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function _ConnApiClientReleaseProtoMangleResources

    \Description
        When both voip and game are in DISC state, let ProtoMangle know that it
        can release resources associated with this client.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to init

    \Notes
        1- Really needed for Xbox One only. Ends up being a no-op on other platform.

        2- For dirtycast-based scenarios, the client entry for the gameserver always has
           pClient->VoipInfo.eStatus = CONNAPI_STATUS_INIT, so the call to ProtoMangleContro()
           is never exercised.

    \Version 09/13/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _ConnApiClientReleaseProtoMangleResources(ConnApiRefT *pConnApi, ConnApiClientT *pClient)
{
    if ((pConnApi->bDemanglerEnabled == TRUE) && (pClient->GameInfo.eStatus == CONNAPI_STATUS_DISC) && (pClient->VoipInfo.eStatus == CONNAPI_STATUS_DISC))
    {
        ProtoMangleControl(pConnApi->pProtoMangle, 'remv', (int32_t)(pClient - pConnApi->ClientList.Clients), 0, NULL);
    }
}


#if CONNAPI_XBOX_NETWORKING
/*F********************************************************************************/
/*!
    \Function _ConnApiInitClientConnectionState

    \Description
        Initialize client's game and voip connection states based on selected game and voip topology.

    \Input *pConnApi    - connection manager ref
    \Input *pClient     - pointer to client to init
    \Input iClientIndex - index of client
    \Input uConnMode    - bit mask to specify what connection state needs to be initialized (CONNAPI_CONNFLAG_XXXCONN)

    \Version 11/08/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _ConnApiInitClientConnectionState(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, uint32_t uConnMode)
{
    /*
    $todo - revisit this logics to make it more explicit
    --> p2p game connections that do not need to be demangled are skipped in _ConnApiUpdateConnections() because
        they don't have the CONNAPI_CONNFLAG_GAMECONN flag set
    --> the same trick is not feasible with p2p voip connections because they need to reach the CONNAPI_STATUS_ACTV state
        even if routed through a server; so we make sure they do not get demangled by defaulting straight
        to CONNAPI_STATUS_INIT here.
    */

    if (uConnMode & CONNAPI_CONNFLAG_GAMECONN)
    {
        // if we're a xboxone client in phxc mode and we are currently dealing with the dedicated server client entry or we are connectivity hosted,
        // then make sure we do not attempt to demangle with protomanglexboxone
        if (!pConnApi->bDemanglerEnabled || pClient->ClientInfo.bIsConnectivityHosted || (iClientIndex == pConnApi->iSelf) ||
            ((pConnApi->eGameTopology == CONNAPI_GAMETOPOLOGY_SERVERHOSTED) && (iClientIndex == pConnApi->iGameHostIndex)))
        {
            pClient->GameInfo.eStatus = CONNAPI_STATUS_INIT;
            pClient->GameInfo.bDemangling = TRUE; // make sure demangling will not be attempted if first connection attempt to server fails
        }
        else
        {
            pClient->GameInfo.eStatus = CONNAPI_STATUS_MNGL;

            // if secure address is already known, don't clear it - it will be reused
            if ((pClient->uFlags & CONNAPI_CLIENTFLAG_SECUREADDRRESOLVED) == 0)
            {
                pClient->ClientInfo.uAddr = 0;
                pClient->ClientInfo.uLocalAddr = 0;
            }
        }
    }

    if (uConnMode & CONNAPI_CONNFLAG_VOIPCONN)
    {
        // if voip is routed through a dedicated server, game is routed through a dedicated server but voip is not or when connectivity hosted, then make sure we do
        // not attempt to demangle voip connections with protomanglexboxone. There is no need to check to see if the client is a voip topology host (host will never have conn mode CONNAPI_CONNFLAG_VOIPCONN)
        if (!pConnApi->bDemanglerEnabled || pClient->ClientInfo.bIsConnectivityHosted || (iClientIndex == pConnApi->iSelf) ||
           (pConnApi->eVoipTopology == CONNAPI_VOIPTOPOLOGY_SERVERHOSTED) ||
           ((pConnApi->eGameTopology == CONNAPI_GAMETOPOLOGY_SERVERHOSTED) && (iClientIndex == pConnApi->iGameHostIndex)))
        {
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_INIT;
            pClient->VoipInfo.bDemangling = TRUE; // make sure demangling will not be attempted if first connection attempt to server fails
        }
        else
        {
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_MNGL;

            // if secure address is already known, don't clear it - it will be reused
            if ((pClient->uFlags & CONNAPI_CLIENTFLAG_SECUREADDRRESOLVED) == 0)
            {
                pClient->ClientInfo.uAddr = 0;
                pClient->ClientInfo.uLocalAddr = 0;
            }
        }
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function _ConnApiInitClient

    \Description
        Initialize a single client based on input user info.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to init
    \Input *pClientInfo - pointer to user info to init client with
    \Input iClientIndex - index of client

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiInitClient(ConnApiRefT *pConnApi, ConnApiClientT *pClient, ConnApiClientInfoT *pClientInfo, int32_t iClientIndex)
{
    // initialize new client structure and save input user info
    ds_memclr(pClient, sizeof(*pClient));
    ds_memcpy_s(&pClient->ClientInfo, sizeof(pClient->ClientInfo),  pClientInfo, sizeof(*pClientInfo));

    // initialize default voip values
    pClient->iVoipConnId = VOIP_CONNID_NONE;

    // set up remote (connect) port info
    pClient->GameInfo.uMnglPort = (pClient->ClientInfo.uGamePort == 0) ? pConnApi->uGamePort : pClient->ClientInfo.uGamePort;
    pClient->VoipInfo.uMnglPort = (pClient->ClientInfo.uVoipPort == 0) ? pConnApi->uVoipPort : pClient->ClientInfo.uVoipPort;

    // set up local (bind) port info
    pClient->GameInfo.uLocalPort = ((pClient->ClientInfo.uLocalGamePort == 0) || (pConnApi->bTunnelEnabled == TRUE)) ? pConnApi->uGamePort : pClient->ClientInfo.uLocalGamePort;
    pClient->VoipInfo.uLocalPort = ((pClient->ClientInfo.uLocalVoipPort == 0) || (pConnApi->bTunnelEnabled == TRUE)) ? pConnApi->uVoipPort : pClient->ClientInfo.uLocalVoipPort;

    // set unique client identifier if not already supplied
    if (pClient->ClientInfo.uId == 0)
    {
        pClient->ClientInfo.uId = (uint32_t)iClientIndex + 1;
    }

    if ((pClient->ClientInfo.bEnableQos == TRUE) && (iClientIndex != pConnApi->iSelf))
    {
        NetPrintf(("connapi: [%p] delaying voip connection for player %d because of QoS validation\n", pConnApi, iClientIndex));
    }
    else
    {
        // Voip is not delayed when bEnableQos is FALSE
        pClient->bEstablishVoip = TRUE;
    }

    #if CONNAPI_XBOX_NETWORKING
    _ConnApiInitClientConnectionState(pConnApi, pClient, iClientIndex, CONNAPI_CONNFLAG_GAMECONN|CONNAPI_CONNFLAG_VOIPCONN);
    #endif

    // mark as allocated
    pClient->bAllocated = TRUE;
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateClientFlags

    \Description
        Update client flags based on game mode and game flags.

    \Input *pConnApi    - pointer to module state

    \Version 05/16/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiUpdateClientFlags(ConnApiRefT *pConnApi)
{
    ConnApiClientT *pClient;
    int32_t iClient;

    for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
    {
        pClient = &pConnApi->ClientList.Clients[iClient];
        if (!pClient->bAllocated || (iClient == pConnApi->iSelf))
        {
            continue;
        }
        pClient->uConnFlags = pConnApi->uConnFlags;

        // when not in a peer web game topology, only establish a game connection to the host
        if ((iClient != pConnApi->iGameHostIndex) && (pConnApi->iGameHostIndex != pConnApi->iSelf) && (pConnApi->eGameTopology != CONNAPI_GAMETOPOLOGY_PEERWEB))
        {
            pClient->uConnFlags &= ~CONNAPI_CONNFLAG_GAMECONN;
        }
        if (pConnApi->eVoipTopology == CONNAPI_VOIPTOPOLOGY_SERVERHOSTED)
        {
            // when in a server hosted voip topology, don't establish a voip connection to the host
            if (iClient == pConnApi->iVoipHostIndex)
            {
                pClient->uConnFlags &= ~CONNAPI_CONNFLAG_VOIPCONN;
            }
        }
        else
        {
            /* if we are in a game with a server hosted and the voip topology is not server hosted, don't establish
                connections to that host */
            if ((pConnApi->eGameTopology == CONNAPI_GAMETOPOLOGY_SERVERHOSTED) && (iClient == pConnApi->iGameHostIndex))
            {
                pClient->uConnFlags &= ~CONNAPI_CONNFLAG_VOIPCONN;
            }
        }
        // if voip needs to be disabled while we do QoS measurements, update the conn flags accordingly
        if (pClient->bEstablishVoip == FALSE)
        {
            pClient->uConnFlags &= ~CONNAPI_CONNFLAG_VOIPCONN;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiGenerateSessionKey

    \Description
        Generate session key for demangling session.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to peer
    \Input iClientIndex - index of peer
    \Input *pSess       - [out] pointer to session buffer
    \Input iSessSize    - size of session buffer
    \Input *pSessType   - type of session - "game" or "voip"

    \Version 01/13/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiGenerateSessionKey(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, char *pSess, int32_t iSessSize, const char *pSessType)
{
    uint32_t uIdA, uIdB, uTemp;

    uIdA = pClient->ClientInfo.uLocalClientId;
    uIdB = pClient->ClientInfo.uRemoteClientId;
    if (uIdB < uIdA)
    {
        uTemp = uIdA;
        uIdA = uIdB;
        uIdB = uTemp;
    }

    ds_snzprintf(pSess, iSessSize, "%08x-%08x-%s-%08x", uIdA, uIdB, pSessType, pConnApi->iSessId);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiTunnelAlloc

    \Description
        Allocate a tunnel for the given client.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - client to connect to
    \Input iClientIndex - index of client
    \Input uRemoteAddr  - remote address to tunnel to
    \Input bLocalAddr   - TRUE if we are using local address, else FALSE

    \Version 12/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiTunnelAlloc(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, uint32_t uRemoteAddr, uint8_t bLocalAddr)
{
    ProtoTunnelInfoT TunnelInfo;

    // if connectivity is hosted and if the tunnel to the hosting server was already created in the connection flow of another client
    // then skip tunnel creation and reuse that tunnel
    if (pClient->ClientInfo.bIsConnectivityHosted)
    {
        int32_t iClient;

        for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
        {
            if ( (pClient != &pConnApi->ClientList.Clients[iClient]) &&
                 (pClient->ClientInfo.uHostingServerId == pConnApi->ClientList.Clients[iClient].ClientInfo.uHostingServerId) &&
                 (pConnApi->ClientList.Clients[iClient].iTunnelId != 0) )
            {
                pClient->iTunnelId = pConnApi->ClientList.Clients[iClient].iTunnelId;
                NetPrintf(("connapi: [%p] client %d is reusing tunnel 0x%08x to hosting server 0x%08x\n", pConnApi, iClientIndex, pClient->iTunnelId, pClient->ClientInfo.uHostingServerId));
                return;
            }
        }
    }

    // set up tunnel info
    ds_memclr(&TunnelInfo, sizeof(TunnelInfo));

    TunnelInfo.uRemoteClientId =  pClient->ClientInfo.bIsConnectivityHosted ? pClient->ClientInfo.uHostingServerId : pClient->ClientInfo.uRemoteClientId;
    TunnelInfo.uRemoteAddr = uRemoteAddr;
    TunnelInfo.uRemotePort = bLocalAddr ? pClient->ClientInfo.uLocalTunnelPort : pClient->ClientInfo.uTunnelPort;
    TunnelInfo.aRemotePortList[0] = pClient->GameInfo.uMnglPort;
    TunnelInfo.aRemotePortList[1] = pClient->VoipInfo.uMnglPort;
    TunnelInfo.aPortFlags[0] = (pConnApi->uGameTunnelFlagOverride) ? (pConnApi->uGameTunnelFlag) : (PROTOTUNNEL_PORTFLAG_ENCRYPTED|PROTOTUNNEL_PORTFLAG_AUTOFLUSH);
    TunnelInfo.aPortFlags[1] = PROTOTUNNEL_PORTFLAG_ENCRYPTED;

    NetPrintf(("connapi: [%p] setting client %d clientId=0x%08x TunnelInfo.uRemotePort %d %s\n", pConnApi, iClientIndex, TunnelInfo.uRemoteClientId, TunnelInfo.uRemotePort,
               pClient->ClientInfo.bIsConnectivityHosted ? "(hosted connection)" : "(direct connection)"));

    // allocate tunnel, set the local client id and return to caller
    pClient->iTunnelId = ProtoTunnelAlloc(pConnApi->pProtoTunnel, &TunnelInfo, pClient->ClientInfo.strTunnelKey);
    if (pClient->iTunnelId >= 0)
    {
        ProtoTunnelControl(pConnApi->pProtoTunnel, 'tcid', pClient->iTunnelId, (int32_t)pClient->ClientInfo.uLocalClientId, NULL);
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiVoipTunnelAlloc

    \Description
        Allocate a tunnel for voip for the given client depending on voip settings.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - client to connect to
    \Input iClientIndex - index of client
    \Input uRemoteAddr  - remote address to tunnel to
    \Input bLocalAddr   - TRUE if we are using local address, else FALSE

    \Version 12/23/2008 (jrainy)
*/
/********************************************************************************F*/
static void _ConnApiVoipTunnelAlloc(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, uint32_t uRemoteAddr, uint8_t bLocalAddr)
{
    int32_t iTunnelId = 0;

    // if doing redirect via host, check for previously created tunnel for re-use
    if (pConnApi->eVoipTopology == CONNAPI_VOIPTOPOLOGY_SERVERHOSTED)
    {
        iTunnelId = pConnApi->ClientList.Clients[pConnApi->iVoipHostIndex].iTunnelId;
    }

    // if no reused tunnel, create one
    if (iTunnelId == 0)
    {
        _ConnApiTunnelAlloc(pConnApi, pClient, iClientIndex, uRemoteAddr, bLocalAddr);
    }
    else
    {
        pClient->iTunnelId = iTunnelId;
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiTunnelFree

    \Description
        Free the given client's tunnel.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to allocate tunnel for

    \Version 12/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiTunnelFree(ConnApiRefT *pConnApi, ConnApiClientT *pClient)
{
    // tunnel active?
    if (!pConnApi->bTunnelEnabled)
    {
        return;
    }

    //  if voip to this client is redirected via the host in a C/S game,
    //  and we're trying to free a client, but not the host itself, skip tunnel destruction.
    if (pConnApi->eVoipTopology == CONNAPI_VOIPTOPOLOGY_SERVERHOSTED)
    {
        if ((pConnApi->ClientList.Clients[pConnApi->iVoipHostIndex].iTunnelId == pClient->iTunnelId) &&
            (&pConnApi->ClientList.Clients[pConnApi->iVoipHostIndex] != pClient))
        {
            pClient->iTunnelId = 0;
            return;
        }
    }

    // if connectivity is hosted and if the tunnel to the hosting server is still used for another client
    // then skip tunnel destruction
    if (pClient->ClientInfo.bIsConnectivityHosted)
    {
        int32_t iClient;

        for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
        {
            if ( (pClient != &pConnApi->ClientList.Clients[iClient]) &&
                 (pClient->ClientInfo.uHostingServerId == pConnApi->ClientList.Clients[iClient].ClientInfo.uHostingServerId) &&
                 (pClient->iTunnelId == pConnApi->ClientList.Clients[iClient].iTunnelId) )
            {
                #if DIRTYCODE_LOGGING
                ConnApiClientT *pFirstClient = &pConnApi->ClientList.Clients[0];
                NetPrintf(("connapi: [%p] freeing tunnel 0x%08x to hosting server 0x%08x is skipped for client %d because tunnel still used by at least client %d\n",
                    pConnApi, pClient->iTunnelId, pClient->ClientInfo.uHostingServerId, (pClient - pFirstClient), iClient));
                #endif
                pClient->iTunnelId = 0;
                return;
            }
        }
    }

    if (pClient->iTunnelId != 0)
    {
        ProtoTunnelFree2(pConnApi->pProtoTunnel, pClient->iTunnelId, pClient->ClientInfo.strTunnelKey, pClient->ClientInfo.uAddr);
        pClient->iTunnelId = 0;
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiGetConnectAddr

    \Description
        Selects between internal and internal address to use for connection,
        based on whether the external addresses are equal or not.

    \Input *pConnApi        - pointer to module state
    \Input *pClient         - pointer to client to connect to
    \Input *pLocalAddr      - [out] storage for boolean indicating whether local address was used or not
    \Input uConnMode        - game conn or voip conn
    \Input **pClientUsedRet - the pointer to use to return the client actually used

    \Output
        int32_t             - ip address to be used to connnect to the specified client

    \Version 01/11/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ConnApiGetConnectAddr(ConnApiRefT *pConnApi, ConnApiClientT *pClient, uint8_t *pLocalAddr, const uint32_t uConnMode, ConnApiClientT **pClientUsedRet)
{
    ConnApiClientT *pSelf;
    int32_t uAddr;
    uint8_t bLocalAddr = FALSE;
    ConnApiClientT *pClientUsed = NULL;

    #if DIRTYCODE_LOGGING
    const char *pConn = (uConnMode == CONNAPI_CONNFLAG_GAMECONN) ? "game" : "voip";
    #endif

    // ref local client info
    pSelf = &pConnApi->ClientList.Clients[pConnApi->iSelf];

    if ((uConnMode == CONNAPI_CONNFLAG_VOIPCONN) && (pConnApi->eVoipTopology == CONNAPI_VOIPTOPOLOGY_SERVERHOSTED))
    {
        uAddr = pConnApi->ClientList.Clients[pConnApi->iVoipHostIndex].ClientInfo.uAddr;
        pClientUsed = &pConnApi->ClientList.Clients[pConnApi->iVoipHostIndex];
        NetPrintf(("connapi: [%s] using host address to connect to (or disconnect from) client 0x%08x\n", pConn, pClient->ClientInfo.uId));
    }
    else
    {
        if (pClient->ClientInfo.uAddr == 0)
        {
            /* we get into passive mode when we don't have an address for our peer and we are expecting the incoming traffic to help us choose the correct remote address.
               for clarity of how this works, we can look at the behavior of commudp, given that the game traffic is what comes first in the establishing the connections.
               when commudp sees that the address is 0.0.0.0, it will normally not send POKE packets to the peer. when the INIT packets come in the remote address gets updated based
               on the source of the traffic. after which point we can complete the connection with our peer having the updated address.

               NOTE: when we introduced prototunnel to the equation the passive functionality was broken because it using a virtual address, not understanding what the real address is.
               this means that we will send invalid POKE packets to 0.0.0.0 via the tunnel which can be seen as errors in the logs. when we receive incoming traffic the connection
               can still complete successfully using the same logic explained above.

               this behavior is only expected in a connection between an xbox client (using xbox secure networking) and dedicated server (from the standpoint of the server).
               if this behavior is seen anywhere else there is a problem in the assigning of address to connapi. dirtysock will recover as long as one side has a correct address.
               if both sides have invalid addresses you could expect the connection to fail */
            NetPrintf(("connapi: [%s] using %a address to connect to (or disconnect from) client 0x%08x in passive mode\n", pConn, pClient->ClientInfo.uAddr, pClient->ClientInfo.uId));
            uAddr = pClient->ClientInfo.uAddr;
            pClientUsed = pClient;
        }
        // if external addresses match, use local address
        else if ((pSelf->ClientInfo.uAddr & pConnApi->uNetMask) == (pClient->ClientInfo.uAddr & pConnApi->uNetMask))
        {
            NetPrintf(("connapi: [%s] using local address to connect to (or disconnect from) client 0x%08x\n", pConn, pClient->ClientInfo.uId));
            uAddr = pClient->ClientInfo.uLocalAddr;
            pClientUsed = pClient;
            bLocalAddr = TRUE;
        }
        else
        {
            NetPrintf(("connapi: [%s] using peer address to connect to (or disconnect from) client 0x%08x\n", pConn, pClient->ClientInfo.uId));
            uAddr = pClient->ClientInfo.uAddr;

            pClientUsed = pClient;
        }

        #if DIRTYCODE_DEBUG
        if (pClient->ClientInfo.bIsConnectivityHosted == FALSE)
        {
            if (pConnApi->bFailP2PConnect)
            {
                // global P2P fail flag is set
                NetPrintf(("connapi: [%s] !! P2P CONNECTION FAILURE TRICK !! - global P2P fail - peer address for client 0x%08x replaced with unreachable value\n", pConn, pClient->ClientInfo.uId));
                uAddr = 0;
            }
            else if (pClient->uFlags & CONNAPI_CLIENTFLAG_P2PFAILDBG)
            {
                NetPrintf(("connapi: [%s] !! P2P CONNECTION FAILURE TRICK !! - remote P2P fail flag - peer address for client 0x%08x replaced with unreachable value\n", pConn, pClient->ClientInfo.uId));
                uAddr = 0;
            }
            else if (pConnApi->ClientList.Clients[pConnApi->iSelf].uFlags & CONNAPI_CLIENTFLAG_P2PFAILDBG)
            {
                NetPrintf(("connapi: [%s] !! P2P CONNECTION FAILURE TRICK !! - self P2P fail flag - peer address for client 0x%08x replaced with unreachable value\n", pConn, pClient->ClientInfo.uId));
                uAddr = 0;
            }
        }
        #endif
    }

    *pLocalAddr = bLocalAddr;

    if (pClientUsedRet)
    {
        *pClientUsedRet = pClientUsed;
    }

    return(uAddr);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiVoipGroupConnSharingCallback

    \Description
        Use to invoke VoipGroupResume() and VoipGroupSuspend when notified
        by the VoipGroup module.

    \Input *pVoipGroup  - voip group ref
    \Input eCbType      - event identifier
    \Input iConnId      - connection ID
    \Input *pUserData   - user callback data
    \Input bSending     - client sending flag
    \Input bReceiving   - client receiving flag

    \Version 11/11/2009 (mclouatre)
*/
/********************************************************************************F*/
static void _ConnApiVoipGroupConnSharingCallback(VoipGroupRefT *pVoipGroup, ConnSharingCbTypeE eCbType, int32_t iConnId, void *pUserData, uint8_t bSending, uint8_t bReceiving)
{
    ConnApiRefT *pConnApi = (ConnApiRefT *)pUserData;
    ConnApiClientT *pClient = &pConnApi->ClientList.Clients[iConnId];
    ConnApiClientT *pClientUsed;
    int32_t iConnectAddr;
    uint8_t bLocalAddr;

    iConnectAddr = _ConnApiGetConnectAddr(pConnApi, pClient, &bLocalAddr, CONNAPI_CONNFLAG_VOIPCONN, &pClientUsed);

    if (eCbType == VOIPGROUP_CBTYPE_CONNSUSPEND)
    {
        NetPrintf(("connapi: [%p] suspending voip connection to client 0x%08x:%a at %d\n", pConnApi, pClient->ClientInfo.uId, iConnectAddr, NetTick()));
        VoipGroupSuspend(pVoipGroup, iConnId);
    }
    else if (eCbType == VOIPGROUP_CBTYPE_CONNRESUME)
    {
         // do we have a tunnel to this client?
        if (pClientUsed->iTunnelId > 0)
        {
            NetPrintf(("connapi: [%p] we have a tunnel for client %d; using virtual address %a\n", pConnApi,
                iConnId, pClientUsed->iTunnelId));
            iConnectAddr = pClientUsed->iTunnelId;
        }

        NetPrintf(("connapi: [%p] resuming voip connection to client 0x%08x:%a at %d\n", pConnApi, pClient->ClientInfo.uId, iConnectAddr, NetTick()));
        VoipGroupResume(pVoipGroup, iConnId, iConnectAddr, pClient->VoipInfo.uMnglPort, pClient->VoipInfo.uLocalPort, pClient->ClientInfo.uId, pConnApi->iSessId, pClient->ClientInfo.bIsConnectivityHosted);
        #if DIRTYCODE_LOGGING
        if (bSending != bReceiving)
        {
            NetPrintf(("connapi: [%p] warning - send and receive mute flags are different for client 0x%08x:%a\n", pConnApi, pClient->ClientInfo.uId, iConnectAddr));
        }
        #endif
        VoipGroupMuteByConnId(pVoipGroup, iConnId, bSending);
    }
    else
    {
        NetPrintf(("connapi: [%p] critical error - unknown connection sharing event type\n", pConnApi));
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiGetConnectParms

    \Description
        Gets connection parameters.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to connect to
    \Input *pConnName   - [out] storage for connection name
    \Input iNameSize    - size of output buffer

    \Output
        int32_t             - connection flags (NEGAME_CONN_*)

    \Version 04/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ConnApiGetConnectParms(ConnApiRefT *pConnApi, ConnApiClientT *pClient, char *pConnName, int32_t iNameSize)
{
    uint32_t uAddrA, uAddrB, uAddrT;
    uint32_t bHosting = TRUE;
    int32_t iConnFlags;

    // reference unique address strings
    uAddrA = pClient->ClientInfo.uRemoteClientId;
    uAddrB = pClient->ClientInfo.uLocalClientId;

    /* determine if we are "hosting" or not, in NetGame terms (one peer
       must listen and one peer must connect for each connection) */
    if (pConnApi->eGameTopology != CONNAPI_GAMETOPOLOGY_PEERWEB)
    {
        // if we're client/server, server listens and clients connect
        bHosting = (pConnApi->iGameHostIndex == pConnApi->iSelf);
    }
    else
    {
        // if we're peer-web, compare addresses to choose listener and connector
        bHosting = uAddrA < uAddrB;
    }

    /* set up parms based on whether we are "hosting" or not.  the connection name is the
       unique address of the "host" concatenated with the unique address of the "client" */
    if (bHosting == TRUE)
    {
        // swap names
        uAddrT = uAddrB;
        uAddrB = uAddrA;
        uAddrA = uAddrT;

        // set conn flags
        iConnFlags = NETGAME_CONN_LISTEN;
    }
    else
    {
        iConnFlags = NETGAME_CONN_CONNECT;
    }

    // format connection name and return connection flags
    ds_snzprintf(pConnName, iNameSize, "%x-%x", uAddrA, uAddrB);
    return(iConnFlags);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateCallback

    \Description
        Trigger user callback if the state has changed.

    \Input *pConnApi    - pointer to module state
    \Input iClientIndex - index of client
    \Input eType        - type of connection (CONNAPI_CBTYPE_*)
    \Input eOldStatus   - previous status
    \Input eNewStatus   - current status

    \Output
        int32_t         - one=active, zero=disconnected

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiUpdateCallback(ConnApiRefT *pConnApi, int32_t iClientIndex, ConnApiCbTypeE eType, ConnApiConnStatusE eOldStatus, ConnApiConnStatusE eNewStatus)
{
    ConnApiCbInfoT CbInfo;
    ConnApiConnInfoT *pConnInfo = NULL;
    int32_t iIndex;

    // if no change, no callback
    if (eOldStatus == eNewStatus)
    {
        return;
    }

    // otherwise, fire off a callback
    CbInfo.iClientIndex = iClientIndex;
    CbInfo.eType = eType;
    CbInfo.eOldStatus = eOldStatus;
    CbInfo.eNewStatus = eNewStatus;
    CbInfo.pClient = &pConnApi->ClientList.Clients[iClientIndex];

    if (eType == CONNAPI_CBTYPE_GAMEEVENT)
    {
        pConnInfo = (ConnApiConnInfoT *)&CbInfo.pClient->GameInfo;
    }
    else if (eType == CONNAPI_CBTYPE_VOIPEVENT)
    {
        pConnInfo = (ConnApiConnInfoT *)&CbInfo.pClient->VoipInfo;
    }

    // update connection timers
    if (pConnInfo != NULL)
    {
        // finished demangling
        if (eOldStatus == CONNAPI_STATUS_MNGL)
        {
            if (pConnInfo->iConnStart != 0)
            {
                #if CONNAPI_XBOX_NETWORKING
                pConnInfo->ConnTimers.uCreateSATime = NetTickDiff(NetTick(), pConnInfo->iConnStart);
                #else
                pConnInfo->ConnTimers.uDemangleTime = NetTickDiff(NetTick(), pConnInfo->iConnStart);
                #endif
            }
            else
            {
                #if CONNAPI_XBOX_NETWORKING
                pConnInfo->ConnTimers.uCreateSATime = 0;
                #else
                pConnInfo->ConnTimers.uDemangleTime = 0;
                #endif
            }
        }
        // it went from some state to active or disconnect log connect time
        else if ((eOldStatus == CONNAPI_STATUS_CONN) && ((eNewStatus == CONNAPI_STATUS_ACTV) || (eNewStatus == CONNAPI_STATUS_DISC)))
        {
            if (pConnInfo->uConnFlags & CONNAPI_CONNFLAG_DEMANGLED)
            {
                if (pConnInfo->iConnStart != 0)
                {
                    pConnInfo->ConnTimers.uDemangleConnectTime = NetTickDiff(NetTick(), pConnInfo->iConnStart);
                }
                else
                {
                    pConnInfo->ConnTimers.uDemangleConnectTime = 0;
                }
            }
            else
            {
                if (pConnInfo->iConnStart != 0)
                {
                    pConnInfo->ConnTimers.uConnectTime = NetTickDiff(NetTick(), pConnInfo->iConnStart);
                }
                else
                {
                    pConnInfo->ConnTimers.uConnectTime = 0;
                }
            }
        }
    }

    // call the callback
    pConnApi->bInCallback = TRUE;
    _ConnApiDefaultCallback(pConnApi, &CbInfo, NULL);
    for(iIndex = 0; iIndex < CONNAPI_MAX_CALLBACKS; iIndex++)
    {
        if (pConnApi->pCallback[iIndex] != NULL)
        {
            pConnApi->pCallback[iIndex](pConnApi, &CbInfo, pConnApi->pUserData[iIndex]);
        }
    }

    pConnApi->bInCallback = FALSE;
}


/*F********************************************************************************/
/*!
    \Function _ConnApiDestroyGameConnection

    \Description
        Destroy game link to given client.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to close game connection for
    \Input iClientIndex - index of client in client array
    \Input *pReason     - reason connection is being closed (for debug use)

    \Version 01/12/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiDestroyGameConnection(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, const char *pReason)
{
    // if refs are about to be destroyed, notify application
    if ((pClient->pGameDistRef != NULL) || (pClient->pGameLinkRef != NULL))
    {
        _ConnApiUpdateCallback(pConnApi, iClientIndex, CONNAPI_CBTYPE_DESTEVENT, CONNAPI_STATUS_ACTV, CONNAPI_STATUS_DISC);
    }

    // destroy the refs
    NetPrintf(("connapi: [%p] destroying game connection to client 0x%08x: %s at %d\n", pConnApi, pClient->ClientInfo.uId, pReason, NetTick()));
    if (pClient->pGameLinkRef != NULL)
    {
        NetGameLinkDestroy(pClient->pGameLinkRef);
        pClient->pGameLinkRef = NULL;
    }
    if (pClient->pGameUtilRef != NULL)
    {
        NetGameUtilDestroy(pClient->pGameUtilRef);
        pClient->pGameUtilRef = NULL;
    }

    pClient->GameInfo.eStatus = CONNAPI_STATUS_DISC;
    _ConnApiClientReleaseProtoMangleResources(pConnApi, pClient);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiDestroyVoipConnection

    \Description
        Destroy voip link to given client.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to close game connection for
    \Input *pReason     - reason connection is being closed (for debug use)

    \Version 01/12/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiDestroyVoipConnection(ConnApiRefT *pConnApi, ConnApiClientT *pClient, const char *pReason)
{
    if (pClient->iVoipConnId >= 0)
    {
        NetPrintf(("connapi: destroying voip connection to client 0x%08x: %s at %d\n", pClient->ClientInfo.uId, pReason, NetTick()));
        VoipGroupDisconnect(pConnApi->pVoipGroupRef, pClient->iVoipConnId);
        pClient->iVoipConnId = VOIP_CONNID_NONE;
    }
    pClient->VoipInfo.eStatus = CONNAPI_STATUS_DISC;

    _ConnApiClientReleaseProtoMangleResources(pConnApi, pClient);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiDisconnectClient

    \Description
        Disconnect a client.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to disconnect
    \Input iClientIndex - client index
    \Input *pReason     - reason for the close

    \Version 01/12/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiDisconnectClient(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, const char *pReason)
{
    _ConnApiDestroyGameConnection(pConnApi, pClient, iClientIndex, pReason);
    _ConnApiDestroyVoipConnection(pConnApi, pClient, pReason);
    _ConnApiTunnelFree(pConnApi, pClient);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiInitClientList

    \Description
        Initialize client list based on input client list.

    \Input *pConnApi    - pointer to module state
    \Input *pClientList - list of client addresses
    \Input iNumClients  - number of clients in list

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiInitClientList(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientList, int32_t iNumClients)
{
    ConnApiClientT *pClient;
    int32_t iClient;

    // make sure client count is below max
    if (iNumClients > pConnApi->ClientList.iMaxClients)
    {
        NetPrintf(("connapi: [%p] cannot host %d clients; clamping to %d\n", pConnApi, iNumClients, pConnApi->ClientList.iMaxClients));
        iNumClients = pConnApi->ClientList.iMaxClients;
    }

    // find our index
    pConnApi->iSelf = -1;   // init so we can check after setup to make sure we're in the list
    for (iClient = 0, pConnApi->ClientList.iNumClients = 0; iClient < iNumClients; iClient++)
    {
        // remember our index in list
        if (pClientList[iClient].uId == pConnApi->uSelfId)
        {
            pConnApi->iSelf = iClient;
        }
    }

    // copy input client list
    for (iClient = 0, pConnApi->ClientList.iNumClients = 0; iClient < iNumClients; iClient++)
    {
        // ref client structure
        pClient = &pConnApi->ClientList.Clients[iClient];

        // need to check to see if the client passed it is valid.
        if (pClientList[iClient].uId != 0)
        {
            // init client structure and copy user info
            _ConnApiInitClient(pConnApi, pClient, &pClientList[iClient], iClient);

            // if us, update dirtyaddr
            if (iClient == pConnApi->iSelf)
            {
                // update dirtyaddr and save ref
                ds_memcpy_s(&pConnApi->SelfAddr, sizeof(pConnApi->SelfAddr), &pClient->ClientInfo.DirtyAddr, sizeof(pClient->ClientInfo.DirtyAddr));
            }

            // increment client count
            pConnApi->ClientList.iNumClients += 1;
        }
    }

    // make sure iSelf is valid before we continue
    if (pConnApi->iSelf >= 0)
    {
        // ref local client
        pClient = &pConnApi->ClientList.Clients[pConnApi->iSelf];

        // set local user
        if (pConnApi->pVoipRef != NULL)
        {
            VoipGroupControl(pConnApi->pVoipGroupRef, 'clid', pClient->ClientInfo.uId, 0, NULL);
        }

        // set prototunnel user
        if (pConnApi->pProtoTunnel != NULL)
        {
            ProtoTunnelControl(pConnApi->pProtoTunnel, 'clid', pClient->ClientInfo.uId, 0, NULL);
        }

        // if the voip is server hosted don't ever establish voip from the host
        if ((pConnApi->eVoipTopology == CONNAPI_VOIPTOPOLOGY_SERVERHOSTED) && (pConnApi->iVoipHostIndex == pConnApi->iSelf))
        {
            pConnApi->uConnFlags &= ~CONNAPI_CONNFLAG_VOIPCONN;
        }
    }

    // set initial client flags
    _ConnApiUpdateClientFlags(pConnApi);

    #if DIRTYCODE_LOGGING
    // make sure we're in the list
    if (pConnApi->iSelf == -1)
    {
        NetPrintf(("connapi: local client 0x%08x not found in client list\n", pConnApi->uSelfId));
    }
    // display client list
    _ConnApiDisplayClientList(&pConnApi->ClientList);
    #endif
}

/*F********************************************************************************/
/*!
    \Function _ConnApiParseAdvertField

    \Description
        Parse a field from an advertisement.

    \Input *pOutBuf     - output buffer for field
    \Input iOutSize     - size of output buffer
    \Input *pInpBuf     - pointer to start of field in advertisement buffer
    \Input cTerminator  - field termination character

    \Output
        char *          - pointer to next field

    \Version 01/10/2005 (jbrookes)
*/
/********************************************************************************F*/
static char *_ConnApiParseAdvertField(char *pOutBuf, int32_t iOutSize, char *pInpBuf, char cTerminator)
{
    char *pEndPtr;

    pEndPtr = strchr(pInpBuf, cTerminator);
    *pEndPtr = '\0';

    ds_strnzcpy(pOutBuf, pInpBuf, iOutSize);
    return(pEndPtr+1);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiCheckAdvert

    \Description
        Scan current adverts for any adverts that are broadcast by clients we are
        connecting to.

    \Input *pConnApi    - pointer to module state

    \Version 01/10/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiCheckAdvert(ConnApiRefT *pConnApi)
{
    char strAdvtList[512], *pAdvt, *pTemp;
    char strName[32], strNote[32], strAddr[32];
    ConnApiClientT *pClient;
    int32_t iAdvt, iNumAdvt, iClient;
    uint32_t uAdvtId;
    uint32_t uLocalAddr;

    // see if there are any advertisements
    iNumAdvt = NetGameUtilQuery(pConnApi->pGameUtilRef, pConnApi->strGameName, strAdvtList, sizeof(strAdvtList));
    NetPrintf(("connapi: [%p] found %d advertisements\n", pConnApi, iNumAdvt));

    // parse any advertisements
    for (pAdvt = strAdvtList, iAdvt = 0; iAdvt < iNumAdvt; iAdvt++)
    {
        // extract info from advertisement
        pAdvt = _ConnApiParseAdvertField(strName, sizeof(strName), pAdvt, '\t');
        pAdvt = _ConnApiParseAdvertField(strNote, sizeof(strNote), pAdvt, '\t');
        pAdvt = _ConnApiParseAdvertField(strAddr, sizeof(strAddr), pAdvt+4, '\n');

        sscanf(strName, "%u", &uAdvtId);

        // extract address from addr field
        pTemp = strchr(strAddr, ':');
        *pTemp = '\0';
        uLocalAddr = SocketInTextGetAddr(strAddr);

        // does the name match one of our client's names?
        for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
        {
            pClient = &pConnApi->ClientList.Clients[iClient];

            if ((uAdvtId == pClient->ClientInfo.uId) && (pClient->ClientInfo.uLocalAddr != uLocalAddr))
            {
                NetPrintf(("connapi: updating local address of machine Id %u from %a to %a\n", uAdvtId, pClient->ClientInfo.uAddr, uLocalAddr));
                pClient->ClientInfo.uLocalAddr = uLocalAddr;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiSetGamelinkOpt

    \Description
        Set a gamelink option, if it isn't the default.

    \Input *pUtilRef    - pointer to util ref for game link
    \Input iOpt         - gamelink option to set
    \Input iValue       - value to set

    \Version 06/03/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiSetGamelinkOpt(NetGameUtilRefT *pUtilRef, int32_t iOpt, int32_t iValue)
{
    if (iValue != 0)
    {
        NetGameUtilControl(pUtilRef, iOpt, iValue);
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiDemangleReport

    \Description
        Initiate a demangler report to indicate connection success or failure.

    \Input *pConnApi    - pointer to module state
    \Input *pInfo       - connection info
    \Input eStatus      - connection result (success/fail)

    \Version 01/17/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiDemangleReport(ConnApiRefT *pConnApi, ConnApiConnInfoT *pInfo, ProtoMangleStatusE eStatus)
{
    if (pInfo->bDemangling != TRUE)
    {
        // not demangling, nothing to report
        return;
    }

    ProtoMangleReport(pConnApi->pProtoMangle, eStatus, -1);
    pConnApi->bReporting = TRUE;
    pInfo->bDemangling = FALSE;
}

/*F********************************************************************************/
/*!
    \Function _ConnApiRemoveClient

    \Description
        Remove a current client from a game.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to remove
    \Input iClientIndex - index of client to remove

    \Version 04/08/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiRemoveClient(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex)
{
    ConnApiConnStatusE eGameStatus = pClient->GameInfo.eStatus;
    ConnApiConnStatusE eVoipStatus = pClient->VoipInfo.eStatus;
    ConnApiConnStatusE eNewGameStatus;
    ConnApiConnStatusE eNewVoipStatus;

    // disconnect them
    _ConnApiDisconnectClient(pConnApi, pClient, iClientIndex, "removal");

    eNewGameStatus = pClient->GameInfo.eStatus;
    eNewVoipStatus = pClient->VoipInfo.eStatus;

    // if we were demangling them, abort demangling
    #if CONNAPI_XBOX_NETWORKING
    if ((pClient->GameInfo.eStatus == CONNAPI_STATUS_MNGL) || (pClient->VoipInfo.eStatus == CONNAPI_STATUS_MNGL))
    #else
    if ((pClient->GameInfo.bDemangling == TRUE) || (pClient->VoipInfo.bDemangling == TRUE))
    #endif
    {
        NetPrintf(("connapi: aborting demangle of client 0x%08x - being removed from client list\n", pClient->ClientInfo.uId));
        ProtoMangleControl(pConnApi->pProtoMangle, 'abrt', iClientIndex, 0, NULL);
    }

    // decrement overall count
    pConnApi->ClientList.iNumClients -= 1;

    ds_memclr(&pConnApi->ClientList.Clients[iClientIndex], sizeof(ConnApiClientT));

    // send a disconnect event
    if (pConnApi->bRemoveCallback == TRUE)
    {
        if ((pConnApi->uConnFlags & CONNAPI_CONNFLAG_GAMECONN) != 0)
        {
            _ConnApiUpdateCallback(pConnApi, iClientIndex, CONNAPI_CBTYPE_GAMEEVENT, eGameStatus, eNewGameStatus);
        }
        if ((pConnApi->uConnFlags & CONNAPI_CONNFLAG_VOIPCONN) != 0)
        {
            _ConnApiUpdateCallback(pConnApi, iClientIndex, CONNAPI_CBTYPE_VOIPEVENT, eVoipStatus, eNewVoipStatus);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateGameClient

    \Description
        Process game connection associated with this client.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to update
    \Input iClientIndex - index of client

    \Output
        int32_t         - one=active, zero=disconnected

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ConnApiUpdateGameClient(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex)
{
    ConnApiConnStatusE eStatus = pClient->GameInfo.eStatus;
    uint8_t bLocalAddr;
    int32_t iConnTimeout;

    // are game connections disabled?
    if ((pConnApi->uConnFlags & CONNAPI_CONNFLAG_GAMECONN) == 0)
    {
        // if we're not connected, just bail
        if ((pClient->GameInfo.eStatus == CONNAPI_STATUS_INIT) || (pClient->GameInfo.eStatus == CONNAPI_STATUS_DISC))
        {
            return(0);
        }

        // if we're voip only and not already disconnected, kill the connection
        _ConnApiDestroyGameConnection(pConnApi, pClient, iClientIndex, "connection closed (voiponly)");
    }

    // handle initial connection state
    if (pClient->GameInfo.eStatus == CONNAPI_STATUS_INIT)
    {
        ConnApiClientT *pClientUsed;

        // get address to connect with
        int32_t iConnectAddr = _ConnApiGetConnectAddr(pConnApi, pClient, &bLocalAddr, CONNAPI_CONNFLAG_GAMECONN, &pClientUsed);

        NetPrintf(("connapi: [%p] establishing game connection to client 0x%08x:%a at %d\n", pConnApi, pClient->ClientInfo.uId, iConnectAddr, NetTick()));

        // create tunnel?
        if ((pConnApi->bTunnelEnabled) && (pClientUsed->iTunnelId == 0))
        {
            _ConnApiTunnelAlloc(pConnApi, pClientUsed, iClientIndex, iConnectAddr, bLocalAddr);
        }

        // do we have a tunnel to this client?
        if (pClientUsed->iTunnelId > 0)
        {
            NetPrintf(("connapi: [%p] tunnel allocated for client %d (local id 0x%08x, remote id 0x%08x); switching to use virtual address %a\n", pConnApi,
                iClientIndex, pClient->ClientInfo.uLocalClientId,
                (pClient->ClientInfo.bIsConnectivityHosted ? pClient->ClientInfo.uHostingServerId : pClient->ClientInfo.uRemoteClientId), pClientUsed->iTunnelId));
            iConnectAddr = pClientUsed->iTunnelId;
        }

        // try to create game connection
        DirtyMemGroupEnter(pConnApi->iMemGroup, pConnApi->pMemGroupUserData);
        pClient->pGameUtilRef = NetGameUtilCreate();
        DirtyMemGroupLeave();
        if (pClient->pGameUtilRef != NULL)
        {
            char strConn[128], strConnName[64];
            int32_t iConnFlags;

            // set game link options
            _ConnApiSetGamelinkOpt(pClient->pGameUtilRef, 'minp', pConnApi->iGameMinp);
            _ConnApiSetGamelinkOpt(pClient->pGameUtilRef, 'mout', pConnApi->iGameMout);
            _ConnApiSetGamelinkOpt(pClient->pGameUtilRef, 'mwid', pConnApi->iGameMwid);
            if (pConnApi->iGameUnackLimit != 0)
            {
                _ConnApiSetGamelinkOpt(pClient->pGameUtilRef, 'ulmt', pConnApi->iGameUnackLimit);
            }

            // set our client id (used by gameserver to uniquely identify us)
            NetGameUtilControl(pClient->pGameUtilRef, 'clid', pClient->ClientInfo.uLocalClientId);
            NetGameUtilControl(pClient->pGameUtilRef, 'rcid', pClient->ClientInfo.uRemoteClientId);

            // determine connection parameters
            iConnFlags = _ConnApiGetConnectParms(pConnApi, pClient, strConnName, sizeof(strConnName));

            // format connect string
            ds_snzprintf(strConn, sizeof(strConn), "%a:%d:%d#%s", iConnectAddr,
                pClient->GameInfo.uLocalPort, pClient->GameInfo.uMnglPort, strConnName);

            // start the connection attempt
            NetGameUtilConnect(pClient->pGameUtilRef, iConnFlags, strConn, pConnApi->pCommConstruct);
            pClient->GameInfo.eStatus = CONNAPI_STATUS_CONN;

            // remember connection start time
            pClient->GameInfo.iConnStart = NetTick();

            NetGameUtilControl(pClient->pGameUtilRef, 'meta', (pConnApi->bCommUdpMetadata || (pClient->ClientInfo.bIsConnectivityHosted)) ? 1 : 0);
        }
        else
        {
            NetPrintf(("connapi: unable to allocate util ref for connection %d to client 0x%08x\n", iClientIndex, pClient->ClientInfo.uId));
            pClient->GameInfo.eStatus = CONNAPI_STATUS_DISC;
            _ConnApiClientReleaseProtoMangleResources(pConnApi, pClient);
        }
    }

    // waiting for connection
    if (pClient->GameInfo.eStatus == CONNAPI_STATUS_CONN)
    {
        void *pCommRef;

        if (pClient->pGameLinkRef == NULL)
        {
            // check for established connection
            if ((pCommRef = NetGameUtilComplete(pClient->pGameUtilRef)) != NULL)
            {
                DirtyMemGroupEnter(pConnApi->iMemGroup, pConnApi->pMemGroupUserData);
                pClient->pGameLinkRef = NetGameLinkCreate(pCommRef, FALSE, pConnApi->iLinkBufSize);
                DirtyMemGroupLeave();
                if (pClient->pGameLinkRef != NULL)
                {
                    NetPrintf(("connapi: game connection %d to client 0x%08x established at %d\n", iClientIndex, pClient->ClientInfo.uId, NetTick()));

                    if (pClient->ClientInfo.bEnableQos)
                    {
                        NetPrintf(("connapi: enabling QoS over NetGameLink on connection %d to client 0x%08x\n", iClientIndex, pClient->ClientInfo.uId));
                        NetGameLinkControl(pClient->pGameLinkRef, 'sqos', pConnApi->iQosDuration, &pConnApi->iQosInterval);
                        NetGameLinkControl(pClient->pGameLinkRef, 'lqos', pConnApi->iQosPacketSize, NULL);
                    }

                    // if we were demangling, report success
                    _ConnApiDemangleReport(pConnApi, &pClient->GameInfo, PROTOMANGLE_STATUS_CONNECTED);

                    // save socket ref for multi-demangle if we need it
                    if (!pConnApi->bTunnelEnabled)
                    {
                        NetGameUtilStatus(pClient->pGameUtilRef, 'sock', &pConnApi->uGameSockRef, sizeof(pConnApi->uGameSockRef));
                    }

                    // indicate we've connected
                    pClient->GameInfo.uConnFlags |= CONNAPI_CONNFLAG_CONNECTED;
                }
                else
                {
                    NetPrintf(("connapi: unable to allocate link ref for connection %d to client 0x%08x\n", iClientIndex, pClient->ClientInfo.uId));
                    pClient->GameInfo.eStatus = CONNAPI_STATUS_DISC;
                    _ConnApiClientReleaseProtoMangleResources(pConnApi, pClient);
                }
            }
        }

        // check for gamelink saying we're connected
        if (pClient->pGameLinkRef != NULL)
        {
            NetGameLinkStatT Stat;

            // give time to NetGameLink to run any connection-related processes
            NetGameLinkUpdate(pClient->pGameLinkRef);

            // get link stats
            NetGameLinkStatus(pClient->pGameLinkRef, 'stat', 0, &Stat, sizeof(NetGameLinkStatT));

            // see if we're open
            if (Stat.isopen == TRUE)
            {
                // mark as active
                NetPrintf(("connapi: game connection %d to client 0x%08x is active at %d\n", iClientIndex, pClient->ClientInfo.uId, NetTick()));
                pClient->GameInfo.eStatus = CONNAPI_STATUS_ACTV;
            }
        }

        // check for connection timeout
        iConnTimeout = pConnApi->iConnTimeout;

        // check for connection timeout
        // The connection timeout, and a subsequent disconnection, should only occur if we still have not connected to the peer.
        // If we have a pGameLinkRef, then that means we MUST have established a connection to the peer, but are still doing QoS.
        if ((pClient->GameInfo.eStatus == CONNAPI_STATUS_CONN) && (pClient->pGameLinkRef == NULL) && (NetTickDiff(NetTick(), pClient->GameInfo.iConnStart) > iConnTimeout))
        {
            _ConnApiDestroyGameConnection(pConnApi, pClient, iClientIndex, "connection timeout");

            // on xboxone, we never want to go back to MNGL state from CONN state
            #if CONNAPI_XBOX_NETWORKING
            NetPrintf(("connapi: game connection to client 0x%08x failed\n", pClient->ClientInfo.uId));
            if (pClient->GameInfo.bDemangling)
            {
                _ConnApiDemangleReport(pConnApi, &pClient->GameInfo, PROTOMANGLE_STATUS_FAILED);
            }

            pClient->GameInfo.eStatus = CONNAPI_STATUS_DISC;
            #else
            // initial attempt to connect failed
            if (pClient->GameInfo.bDemangling == FALSE)
            {
                if ((pConnApi->bDemanglerEnabled) && (pClient->ClientInfo.bIsConnectivityHosted == FALSE) && (pConnApi->eGameTopology != CONNAPI_GAMETOPOLOGY_SERVERHOSTED))
                {
                    NetPrintf(("connapi: game status=mngl for connection %d to client 0x%08x\n", iClientIndex, pClient->ClientInfo.uId));
                    pClient->GameInfo.eStatus = CONNAPI_STATUS_MNGL;
                }
                else
                {
                    if (pConnApi->bDemanglerEnabled && pClient->ClientInfo.bIsConnectivityHosted == TRUE)
                    {
                        NetPrintf(("connapi: demangling skipped for connection %d to client 0x%08x because the client is connectivity hosted\n", iClientIndex, pClient->ClientInfo.uId));
                    }

                    pClient->GameInfo.eStatus = CONNAPI_STATUS_DISC;
                }
            }
            else
            {
                NetPrintf(("connapi: game connection to client 0x%08x after demangle failed\n", pClient->ClientInfo.uId));
                _ConnApiDemangleReport(pConnApi, &pClient->GameInfo, PROTOMANGLE_STATUS_FAILED);

                pClient->GameInfo.eStatus = CONNAPI_STATUS_DISC;
            }
            #endif
        } // timeout

        // set the packet receive flag if a packet was received
        if ((pClient->pGameUtilRef != NULL) && (NetGameUtilStatus(pClient->pGameUtilRef, 'pkrc', NULL, 0) == TRUE))
        {
            pClient->GameInfo.uConnFlags |= CONNAPI_CONNFLAG_PKTRECEIVED;
        }
    } // waiting for connection

    // update connection status during active phase
    if (pClient->GameInfo.eStatus == CONNAPI_STATUS_ACTV)
    {
        // get link stats
        NetGameLinkStatT Stat;
        NetGameLinkStatus(pClient->pGameLinkRef, 'stat', 0, &Stat, sizeof(NetGameLinkStatT));

        // make sure connection is still open
        if (Stat.isopen == FALSE)
        {
            _ConnApiDestroyGameConnection(pConnApi, pClient, iClientIndex, "connection closed");
        }
        // see if we've timed out
        else if (NetTickDiff(Stat.tick, Stat.rcvdlast) > pConnApi->iTimeout)
        {
            _ConnApiDestroyGameConnection(pConnApi, pClient, iClientIndex, "connection timed out");
        }
    }

    // trigger callback if state change
    _ConnApiUpdateCallback(pConnApi, iClientIndex, CONNAPI_CBTYPE_GAMEEVENT, eStatus, (ConnApiConnStatusE)pClient->GameInfo.eStatus);

    // return active or inactive
    return(pClient->GameInfo.eStatus != CONNAPI_STATUS_DISC);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateVoipClient

    \Description
        Process voip connection associated with this connection.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to connection to update
    \Input iClientIndex - index of connection

    \Output
        int32_t         - one=active, zero=disconnected

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ConnApiUpdateVoipClient(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex)
{
    ConnApiConnStatusE eStatus = pClient->VoipInfo.eStatus;
    int32_t iVoipConnId, iVoipStatus = 0;
    uint8_t bLocalAddr;

    // are voip connections disabled?
    if ((pConnApi->uConnFlags & CONNAPI_CONNFLAG_VOIPCONN) == 0)
    {
        // if we're not connected, just bail
        if ((pClient->VoipInfo.eStatus == CONNAPI_STATUS_INIT) || (pClient->VoipInfo.eStatus == CONNAPI_STATUS_DISC))
        {
            return(0);
        }

        // if we're game only and not already disconnected, close the connection
        _ConnApiDestroyVoipConnection(pConnApi, pClient, "connection closed (gameonly)");
    }

    // handle initial connection state
    if (pClient->VoipInfo.eStatus == CONNAPI_STATUS_INIT)
    {
        ConnApiClientT *pClientUsed;
        int32_t iAdjustedVoipClientIndex = (pConnApi->eVoipTopology == CONNAPI_VOIPTOPOLOGY_SERVERHOSTED) ? iClientIndex - 1 : iClientIndex;

        // get address to connect with
        int32_t iConnectAddr = _ConnApiGetConnectAddr(pConnApi, pClient, &bLocalAddr, CONNAPI_CONNFLAG_VOIPCONN, &pClientUsed);

        NetPrintf(("connapi: [%p] establishing voip connection to client 0x%08x:%a at %d\n", pConnApi,
            pClient->ClientInfo.uId, iConnectAddr, NetTick()));

        // create tunnel?
        if ((pConnApi->bTunnelEnabled) && (pClientUsed->iTunnelId == 0))
        {
            _ConnApiVoipTunnelAlloc(pConnApi, pClientUsed, iClientIndex, iConnectAddr, bLocalAddr);
        }

        // do we have a tunnel to this client?
        if (pClientUsed->iTunnelId > 0)
        {
            NetPrintf(("connapi: [%p] tunnel allocated for client %d; switching to use virtual address %a\n", pConnApi,
                iClientIndex, pClientUsed->iTunnelId));
            iConnectAddr = pClientUsed->iTunnelId;
        }

        // initiate connection attempt
        // $$todo - deprecate 'vcid' and add a new uLocalClientId parameter to VoipGroupConnect() - the current implementation can be problematic if 
        // the return conn id does not correspond to the one set with 'vcid'
        VoipGroupControl(pConnApi->pVoipGroupRef, 'vcid', iAdjustedVoipClientIndex, 0, &pClient->ClientInfo.uLocalClientId);
        iVoipConnId = VoipGroupConnect(pConnApi->pVoipGroupRef, iAdjustedVoipClientIndex, iConnectAddr, pClient->VoipInfo.uMnglPort, pClient->VoipInfo.uLocalPort,
                                       pClient->ClientInfo.uId, pConnApi->iSessId, pClient->ClientInfo.bIsConnectivityHosted, pClient->ClientInfo.uRemoteClientId);

        if (iVoipConnId >= 0)
        {
            pClient->iVoipConnId = iVoipConnId;
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_CONN;
            pClient->VoipInfo.iConnStart = NetTick();
        }
        else
        {
            NetPrintf(("connapi: unable to init voip for client index %d (client id: 0x%08x)\n", iClientIndex, pClient->ClientInfo.uId));
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_DISC;
            _ConnApiClientReleaseProtoMangleResources(pConnApi, pClient);
        }
    }

    // get connection status
    if ((pClient->VoipInfo.eStatus == CONNAPI_STATUS_CONN) || (pClient->VoipInfo.eStatus == CONNAPI_STATUS_ACTV))
    {
        iVoipStatus = VoipGroupConnStatus(pConnApi->pVoipGroupRef, pClient->iVoipConnId);
    }

    // for both cases below, CONNAPI_STATUS_CONN and CONNAPI_STATUS_ACTV, when we detect a disconnection,
    // we won't set the iVoipConnId to NONE nor trigger a disconnection. This is because the voipgroup code,
    // in order to provide correct voip connection sharing, needs to be told only of authoritative "connect"
    // and "disconnect" by the higher-level lobby (plasma, lobbysdk, blazesdk, ...) techonology.

    // update connection status during connect phase
    if (pClient->VoipInfo.eStatus == CONNAPI_STATUS_CONN)
    {
        // check for established connection
        if (iVoipStatus & VOIP_CONN_CONNECTED)
        {
            NetPrintf(("connapi: voip connection for client index %d (client id: 0x%08x) established at %d\n", iClientIndex, pClient->ClientInfo.uId, NetTick()));

            // if we were demangling, report success
            _ConnApiDemangleReport(pConnApi, &pClient->VoipInfo, PROTOMANGLE_STATUS_CONNECTED);

            // save socket ref for multi-demangle if we need it
            if (!pConnApi->bTunnelEnabled)
            {
                VoipGroupStatus(pConnApi->pVoipGroupRef, 'sock', 0, &pConnApi->uVoipSockRef, sizeof(pConnApi->uVoipSockRef));
            }

            // mark as active
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_ACTV;
            pClient->VoipInfo.uConnFlags |= CONNAPI_CONNFLAG_CONNECTED;
        }
        else if (iVoipStatus & VOIP_CONN_STOPPED)
        {
            NetPrintf(("connapi: voip connection attempt to client 0x%08x failed at %d\n", pClient->ClientInfo.uId, NetTick()));

            // on xboxone, we never want to go back to MNGL state from CONN state
            #if CONNAPI_XBOX_NETWORKING
            NetPrintf(("connapi: voip connection attempt to client 0x%08x failed\n", pClient->ClientInfo.uId));
            if (pClient->VoipInfo.bDemangling == TRUE)
            {
                _ConnApiDemangleReport(pConnApi, &pClient->VoipInfo, PROTOMANGLE_STATUS_FAILED);
            }

            pClient->VoipInfo.eStatus = CONNAPI_STATUS_DISC;
            #else
            if (pClient->VoipInfo.bDemangling == TRUE)
            {
                NetPrintf(("connapi: voip connection attempt to client 0x%08x after demangle failed\n", pClient->ClientInfo.uId));
                _ConnApiDemangleReport(pConnApi, &pClient->VoipInfo, PROTOMANGLE_STATUS_FAILED);

                pClient->VoipInfo.eStatus = CONNAPI_STATUS_DISC;
            }
            else
            {
                if (pConnApi->bDemanglerEnabled && pClient->ClientInfo.bIsConnectivityHosted == FALSE)
                {
                    NetPrintf(("connapi: voip status=mngl for client index %d (client id: 0x%08x)\n", iClientIndex, pClient->ClientInfo.uId));
                    pClient->VoipInfo.eStatus = CONNAPI_STATUS_MNGL;
                }
                else
                {
                    if (pConnApi->bDemanglerEnabled && pClient->ClientInfo.bIsConnectivityHosted == TRUE)
                    {
                        NetPrintf(("connapi: voip connection demangling skipped for client index %d (client id: 0x%08x) because the client is connectivity hosted\n", iClientIndex, pClient->ClientInfo.uId));
                    }

                    pClient->VoipInfo.eStatus = CONNAPI_STATUS_DISC;
                }
            } // bDemangling = FALSE
            #endif
        } // REMOTE_DISCONNECTED
    } // STATUS_CONN

    // update client in active state
    if (pClient->VoipInfo.eStatus == CONNAPI_STATUS_ACTV)
    {
        if (iVoipStatus & VOIP_CONN_STOPPED)
        {
            NetPrintf(("connapi: voip connection to client 0x%08x terminated at %d\n", pClient->ClientInfo.uId, NetTick()));
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_DISC;
            _ConnApiClientReleaseProtoMangleResources(pConnApi, pClient);
        }
    }

    // trigger callback if state change
    _ConnApiUpdateCallback(pConnApi, iClientIndex, CONNAPI_CBTYPE_VOIPEVENT, eStatus, (ConnApiConnStatusE)pClient->VoipInfo.eStatus);

    // return active or inactive
    return(pClient->VoipInfo.eStatus != CONNAPI_STATUS_DISC);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateDemangleReport

    \Description
        Update demangler during the Report phase.

    \Input *pConnApi    - pointer to module state

    \Output
        uint32_t        - TRUE if reporting is in progress, FALSE otherwise

    \Version 01/17/2005 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _ConnApiUpdateDemangleReport(ConnApiRefT *pConnApi)
{
    // if not reporting, don't process
    if (pConnApi->bReporting != FALSE)
    {
        #if CONNAPI_XBOX_NETWORKING
        // there is no reporting phase on xbox one... so we alway fake that reporting is complete
        pConnApi->bReporting = FALSE;
        #else
        // update client
        ProtoMangleUpdate(pConnApi->pProtoMangle);

        // check for completion
        if (ProtoMangleComplete(pConnApi->pProtoMangle, NULL, NULL) != 0)
        {
            pConnApi->bReporting = FALSE;
        }
        #endif
    }

    return(pConnApi->bReporting);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateDemangle

    \Description
        Update client connection in CONNAPI_STATUS_MNGL state.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to update
    \Input iClientIndex - index of client
    \Input *pConnInfo   - pointer to connection info (game or voip)
    \Input iType        - type of connection (zero=game, one=voip)

    \Version 01/13/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiUpdateDemangle(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, ConnApiConnInfoT *pConnInfo, int32_t iType)
{
    static const char _Types[2][5] = { "game", "voip" };
    static uint32_t _Flags[2] = { CONNAPI_CONNFLAG_GAMECONN, CONNAPI_CONNFLAG_VOIPCONN };
    ConnApiCbTypeE eCbType;

    #if !CONNAPI_XBOX_NETWORKING
    int32_t iClient;
    #endif

    // initialize eType
    if (iType == 0)
    {
        eCbType = CONNAPI_CBTYPE_GAMEEVENT;
    }
    else
    {
        eCbType = CONNAPI_CBTYPE_VOIPEVENT;
    }

    // ignore game/voip demangle if we're not doing game/voip connections
    if ((_Flags[iType] & pConnApi->uConnFlags) == 0)
    {
        return;
    }

    #if !CONNAPI_XBOX_NETWORKING
    // if anyone is in a connecting state, wait to demangle
    for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
    {
        if ((pConnApi->ClientList.Clients[iClient].GameInfo.eStatus == CONNAPI_STATUS_CONN) ||
            (pConnApi->ClientList.Clients[iClient].VoipInfo.eStatus == CONNAPI_STATUS_CONN))
        {
            NetPrintf(("connapi: [%p] deferring demangle until there are no ongoing connection attempts\n", pConnApi));
            return;
        }
    }
    #endif

    // tunnel-specific processing
    if (pConnApi->bTunnelEnabled)
    {
        // if we've already demangled the tunnel port, use previous demangle result
        if (pClient->uFlags & CONNAPI_CLIENTFLAG_TUNNELPORTDEMANGLED)
        {
            ConnApiConnStatusE eStatus = pConnInfo->eStatus;
            pConnInfo->eStatus = CONNAPI_STATUS_INIT;
            pConnInfo->uConnFlags |= CONNAPI_CONNFLAG_DEMANGLED;

            NetPrintf(("connapi: [%p] reusing previously demangled tunnel port\n", pConnApi));

            // trigger callback if state change
            _ConnApiUpdateCallback(pConnApi, iClientIndex, eCbType, eStatus, (ConnApiConnStatusE)pConnInfo->eStatus);
            return;
        }
    }
    else
    {
        // if we've already resolved the secure address, use it
        if (pClient->uFlags & CONNAPI_CLIENTFLAG_SECUREADDRRESOLVED)
        {
            ConnApiConnStatusE eStatus = pConnInfo->eStatus;
            pConnInfo->eStatus = CONNAPI_STATUS_INIT;
            pConnInfo->uConnFlags |= CONNAPI_CONNFLAG_DEMANGLED;

            NetPrintf(("connapi: [%p] reusing previously resolved secure address\n", pConnApi));

            // trigger callback if state change
            _ConnApiUpdateCallback(pConnApi, iClientIndex, eCbType, eStatus, (ConnApiConnStatusE)pConnInfo->eStatus);
            return;
        }
    }

    // are we in an idle state?
    if (ProtoMangleStatus(pConnApi->pProtoMangle, 'idle', NULL, iClientIndex))
    {
        char strSess[64];
        intptr_t uSockRef;
        uint32_t uDemanglePort = 0;

        // show we're demangling
        pConnInfo->bDemangling = TRUE;

        // create session id
        _ConnApiGenerateSessionKey(pConnApi, pClient, iClientIndex, strSess, sizeof(strSess), _Types[iType]);

        // get socket ref
        if (pConnApi->bTunnelEnabled)
        {
            uSockRef = pConnApi->uTunlSockRef;
        }
        else if (iType == 0)
        {
            uSockRef = pConnApi->uGameSockRef;
        }
        else
        {
            uSockRef = pConnApi->uVoipSockRef;
        }

        // if socket ref is null, connect normally
        if (uSockRef == 0)
        {
            #if !CONNAPI_RNDLCLDEMANGLEPORT
            uDemanglePort = (iType == 0) ? pConnApi->uGamePort : pConnApi->uVoipPort;
            uDemanglePort = (pConnApi->bTunnelEnabled) ? (unsigned)pConnApi->iTunnelPort : uDemanglePort;
            #else
            uDemanglePort = _ConnApiGenerateDemanglePort(pConnApi);
            if (pConnApi->bTunnelEnabled)
            {
                // if we're tunneling, we need to recreate the tunnel socket bound to the new port
                ProtoTunnelControl(pConnApi->pProtoTunnel, 'bind', uDemanglePort, 0, NULL);
            }
            else
            {
                // if not tunneling, we need to update the local port info with the new local (bind) demangle port
                pConnInfo->uLocalPort = (uint16_t)uDemanglePort;
            }
            #endif
            // if tunneling we always want to use the tunnel socket ref for demangling
            if (pConnApi->bTunnelEnabled)
            {
                ProtoTunnelStatus(pConnApi->pProtoTunnel, 'sock', 0, &pConnApi->uTunlSockRef, sizeof(pConnApi->uTunlSockRef));
                uSockRef = pConnApi->uTunlSockRef;
            }
        }

        if (iType == 0)
        {
            pClient->GameInfo.iConnStart = NetTick();
        }
        else
        {
            pClient->VoipInfo.iConnStart = NetTick();
        }

        #if CONNAPI_XBOX_NETWORKING
        {
            // since the SecureDeviceAddr fits in the DirtyAddrT, it is safe to use a buffer as large as a DirtyAddrT
            char aSecureDeviceAddressBlob[DIRTYADDR_MACHINEADDR_MAXLEN];
            int32_t iBlobSize;

            if (DirtyAddrGetInfoXboxOne(&pConnApi->ClientList.Clients[iClientIndex].ClientInfo.DirtyAddr, NULL, aSecureDeviceAddressBlob, &iBlobSize))
            {
                NetPrintf(("connapi: initiating %s secure address resolution of client 0x%08x at %d\n", _Types[iType], pClient->ClientInfo.uId, NetTick()));
                ProtoMangleConnect2(pConnApi->pProtoMangle, iClientIndex, aSecureDeviceAddressBlob, iBlobSize);
            }
            else
            {
                ConnApiConnStatusE eStatus = pConnInfo->eStatus;
                NetPrintf(("connapi: failed to initiate the %s secure address resolution of client 0x%08x due to device address being invalid\n", _Types[iType], pClient->ClientInfo.uId));
                pConnInfo->eStatus = CONNAPI_STATUS_DISC;
                _ConnApiUpdateCallback(pConnApi, iClientIndex, eCbType, eStatus, (ConnApiConnStatusE)pConnInfo->eStatus);
                return;
            }
        }
        #else
        // before demangling, flush the tunnel to make sure there are no buffered packets
        if (pConnApi->bTunnelEnabled)
        {
            ProtoTunnelControl(pConnApi->pProtoTunnel, 'flsh', pClient->iTunnelId, 0, NULL);
        }

        // kick off demangling process
        if (uSockRef == 0)
        {
            NetPrintf(("connapi: initiating %s:%d demangle of client 0x%08x at %d\n", _Types[iType], uDemanglePort, pClient->ClientInfo.uId, NetTick()));
            ProtoMangleConnect(pConnApi->pProtoMangle, uDemanglePort, strSess);

        }
        else
        {
            NetPrintf(("connapi: initiating %s demangle of client 0x%08x using sockref 0x%08x at %d\n", _Types[iType], pClient->ClientInfo.uId, uSockRef, NetTick()));
            ProtoMangleConnectSocket(pConnApi->pProtoMangle, uSockRef, strSess);
        }
        #endif
    }
    else
    {
        if (pConnInfo->bDemangling != FALSE)
        {
            int32_t iAddr, iPort, iResult;

            // update client
            ProtoMangleUpdate(pConnApi->pProtoMangle);

            // check for completion
            #if CONNAPI_XBOX_NETWORKING
            iPort = iClientIndex;
            #endif
            if ((iResult = ProtoMangleComplete(pConnApi->pProtoMangle, &iAddr, &iPort)) != 0)
            {
                ConnApiConnStatusE eStatus = pConnInfo->eStatus;

                if (eStatus != CONNAPI_STATUS_ACTV)
                {
                    if (iResult > 0)
                    {
    #if CONNAPI_XBOX_NETWORKING
                        NetPrintf(("connapi: %s secure address resolution for user 0x%08x is successful ipaddr=%a at %d\n",
                            _Types[iType], pClient->ClientInfo.uId, iAddr, NetTick()));
                        iPort = pConnInfo->uMnglPort;
    #else
                        NetPrintf(("connapi: %s demangle of client 0x%08x successful port=%d at %d\n", _Types[iType], pClient->ClientInfo.uId, iPort, NetTick()));
    #endif

                        pClient->ClientInfo.uAddr = pClient->ClientInfo.uLocalAddr = iAddr;
                        if (pConnApi->bTunnelEnabled)
                        {
                            // for xboxone, clients do not yet have a valid tunnel id at this point because client enters to MNGL state before INIT state (unlike other platforms)
                            #if !CONNAPI_XBOX_NETWORKING
                            ProtoTunnelControl(pConnApi->pProtoTunnel, 'rprt', pClient->iTunnelId, iPort, NULL);
                            #endif

                            pClient->uFlags |= CONNAPI_CLIENTFLAG_TUNNELPORTDEMANGLED;
                        }
                        else
                        {
                            pConnInfo->uMnglPort = (uint16_t)iPort;

                            #if CONNAPI_XBOX_NETWORKING
                            pClient->uFlags |= CONNAPI_CLIENTFLAG_SECUREADDRRESOLVED;
                            #endif
                        }
                        pConnInfo->eStatus = CONNAPI_STATUS_INIT;
                        pConnInfo->uConnFlags |= CONNAPI_CONNFLAG_DEMANGLED;
                    }
                    else
                    {
                        NetPrintf(("connapi: %s demangle of client 0x%08x failed (timeout=%s)\n", _Types[iType], pClient->ClientInfo.uId,
                            ProtoMangleStatus(pConnApi->pProtoMangle, 'time', NULL, iClientIndex) ? "true" : "false"));
                        pConnInfo->eStatus = CONNAPI_STATUS_DISC;
                    }
                }
                else
                {
                    NetPrintf(("connapi: [%p] %s demangle of client 0x%08x finished with %d but ignored because connection aleady active (timeout=%s)\n", pConnApi,
                        _Types[iType], pClient->ClientInfo.uId, iResult, ProtoMangleStatus(pConnApi->pProtoMangle, 'time', NULL, iClientIndex) ? "true" : "false"));
                }

                // trigger callback if state change
                _ConnApiUpdateCallback(pConnApi, iClientIndex, eCbType, eStatus, (ConnApiConnStatusE)pConnInfo->eStatus);
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateRemoval

    \Description
        Scan through client list and remove clients marked for removal.

    \Input *pConnApi    - pointer to module state

    \Version 04/11/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiUpdateRemoval(ConnApiRefT *pConnApi)
{
    ConnApiClientT *pClient;
    int32_t iClientIndex;

    for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        // ref client
        pClient = &pConnApi->ClientList.Clients[iClientIndex];

        // if client needs to be removed, remove them
        if (pClient->uFlags & CONNAPI_CLIENTFLAG_REMOVE)
        {
            _ConnApiRemoveClient(pConnApi, pClient, iClientIndex);
        }
    }
}


/*F********************************************************************************/
/*!
    \Function _ConnApiRemoveClientSetup

    \Description
        Set up a client for removal from the game.

    \Input *pConnApi    - pointer to module state
    \Input iClientIndex - index of client to remove (used if pClientName is NULL)
    \Input uFlags       - client removal flags

    \Notes
        If this function is called inside of a ConnApi callback, the removal will
        be deferred until the next time NetConnIdle() is called.  Otherwise, the
        removal will happen immediately.

    \Version 04/08/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiRemoveClientSetup(ConnApiRefT *pConnApi, int32_t iClientIndex, uint16_t uFlags)
{
    ConnApiClientT *pClient;

    // don't allow self removal
    if (iClientIndex == pConnApi->iSelf)
    {
        NetPrintf(("connapi: [%p] can't remove self from game\n", pConnApi));
        return;
    }

    // ref the client and mark them for removal
    pClient = &pConnApi->ClientList.Clients[iClientIndex];
    pClient->uFlags |= uFlags;

    // if we're not in a callback, do the removal immediately
    if (pConnApi->bInCallback == FALSE)
    {
        _ConnApiUpdateRemoval(pConnApi);
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateConnections

    \Description
        Update ConnApi connections.

    \Input *pConnApi    - pointer to module state

    \Output
        int32_t         - number of connections that are not in the DISC state

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ConnApiUpdateConnections(ConnApiRefT *pConnApi)
{
    ConnApiClientT *pClient;
    int32_t iActive, iClientIndex;
    uint32_t bDemangling;

    // update game connections
    for (iActive = 0, iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        // ref connection
        pClient = &pConnApi->ClientList.Clients[iClientIndex];

        // don't update if iClientIndex is us or unallocated
        if (!pClient->bAllocated)
        {
            continue;
        }

        if (pClient->uConnFlags & CONNAPI_CONNFLAG_GAMECONN)
        {
            // process game connection
            iActive += _ConnApiUpdateGameClient(pConnApi, pClient, iClientIndex);
        }
    }

    // update voip connections
    if (pConnApi->pVoipRef != NULL)
    {
        for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
        {
            // ref connection
            pClient = &pConnApi->ClientList.Clients[iClientIndex];

            // don't update if iClientIndex is us or unallocated
            if (!pClient->bAllocated)
            {
                continue;
            }

            if (pClient->uConnFlags & CONNAPI_CONNFLAG_VOIPCONN)
            {
                // process voip connection
                iActive += _ConnApiUpdateVoipClient(pConnApi, pClient, iClientIndex);
            }
        }
    }

    // update reporting
    bDemangling = _ConnApiUpdateDemangleReport(pConnApi);

    // update game demangling
    for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        // ref connection
        pClient = &pConnApi->ClientList.Clients[iClientIndex];

        // don't update if iClientIndex is us or unallocated
        if ((iClientIndex == pConnApi->iSelf) || !pClient->bAllocated)
        {
            continue;
        }

        // demangle game connection?
#if CONNAPI_XBOX_NETWORKING
        // on xboxone, we don't serialize demangling of different clients, we want them occurring in parallel
        if ((pClient->GameInfo.eStatus == CONNAPI_STATUS_MNGL) && (pClient->uConnFlags & CONNAPI_CONNFLAG_GAMECONN))
#else
        if ((pClient->GameInfo.eStatus == CONNAPI_STATUS_MNGL) && (pClient->uConnFlags & CONNAPI_CONNFLAG_GAMECONN) && (bDemangling == FALSE))
#endif

        {
            _ConnApiUpdateDemangle(pConnApi, pClient, iClientIndex, &pClient->GameInfo, 0);
        }
        bDemangling |= pClient->GameInfo.bDemangling;
    }

    // update voip demangling
    for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        // ref connection
        pClient = &pConnApi->ClientList.Clients[iClientIndex];

        // don't update if iClientIndex is us or unallocated
        if ((iClientIndex == pConnApi->iSelf) || !pClient->bAllocated)
        {
            continue;
        }

        // demangle voip connection?
#if CONNAPI_XBOX_NETWORKING
        // on xboxone, we don't serialize demangling of different clients, we want them occurring in parallel
        if ((pClient->VoipInfo.eStatus == CONNAPI_STATUS_MNGL))
#else
        if ((pClient->VoipInfo.eStatus == CONNAPI_STATUS_MNGL) && (bDemangling == FALSE))
#endif

        {
            _ConnApiUpdateDemangle(pConnApi, pClient, iClientIndex, &pClient->VoipInfo, 1);
        }
    }

    // update tunnel
    if ((pConnApi->bTunnelEnabled != 0) && (pConnApi->pProtoTunnel != NULL))
    {
        ProtoTunnelUpdate(pConnApi->pProtoTunnel);
    }

    return(iActive);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiIdle

    \Description
        NetConn idle function to update the ConnApi module.

    \Input *pData   - pointer to module state
    \Input uTick    - current tick count

    \Notes
        This function is installed as a NetConn Idle function.  NetConnIdle()
        must be regularly polled for this function to be called.

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiIdle(void *pData, uint32_t uTick)
{
    ConnApiRefT *pConnApi = (ConnApiRefT *)pData;

    if (pConnApi->bAutoUpdate == TRUE)
    {
        ConnApiUpdate(pConnApi);
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ConnApiCreate2

    \Description
        Create the module state.

    \Input iGamePort    - game connection port
    \Input iMaxClients  - maximum number of clients allowed
    \Input *pCallback   - pointer to user callback
    \Input *pUserData   - pointer to user data
    \Input *pConstruct  - comm construct function

    \Output
        ConnApiRefT *   - pointer to module state, or NULL

    \Version 01/04/2005 (jbrookes)
*/
/********************************************************************************F*/
ConnApiRefT *ConnApiCreate2(int32_t iGamePort, int32_t iMaxClients, ConnApiCallbackT *pCallback, void *pUserData, CommAllConstructT *pConstruct)
{
    ConnApiRefT *pConnApi;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    int32_t iSize;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // calculate size of module state
    iSize = sizeof(*pConnApi) + (sizeof(ConnApiClientT) * (iMaxClients - 1));

    // allocate and init module state
    if ((pConnApi = DirtyMemAlloc(iSize, CONNAPI_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("connapi: could not allocate module state... connapi initialization aborted!\n"));
        return(NULL);
    }
    ds_memclr(pConnApi, iSize);
    pConnApi->iMemGroup = iMemGroup;
    pConnApi->pMemGroupUserData = pMemGroupUserData;

    if ((pConnApi->pVoipGroupRef = VoipGroupCreate(_ConnApi_iMaxVoipGroups)) == NULL)
    {
        // release module memory
        DirtyMemFree(pConnApi, CONNAPI_MEMID, pConnApi->iMemGroup, pConnApi->pMemGroupUserData);

        NetPrintf(("connapi: [%p] no more voip groups available... connapi initialization aborted!\n", pConnApi));
        return(NULL);
    }

    // register connection sharing callback with underlying voip group instance
    VoipGroupSetConnSharingEventCallback(pConnApi->pVoipGroupRef, _ConnApiVoipGroupConnSharingCallback, pConnApi);

    // save info
    pConnApi->uGamePort = (uint16_t)iGamePort;
    pConnApi->uVoipPort = VOIP_PORT;
    pConnApi->ClientList.iMaxClients = iMaxClients;
    pConnApi->pCallback[0] = (pCallback != NULL) ? pCallback :  _ConnApiDefaultCallback;
    pConnApi->pUserData[0] = pUserData;
    pConnApi->pCommConstruct = pConstruct;

    // set default values
    pConnApi->uConnFlags = CONNAPI_CONNFLAG_GAMEVOIP;
    pConnApi->iLinkBufSize = CONNAPI_LINKBUFDEFAULT;
    pConnApi->iConnTimeout = CONNAPI_CONNTIMEOUT_DEFAULT;
    pConnApi->iTimeout = CONNAPI_TIMEOUT_DEFAULT;
    pConnApi->iConfigMnglTimeout = CONNAPI_DEMANGLER_TIMEOUT;
    pConnApi->iConfigMnglTimeoutFailover = CONNAPI_DEMANGLER_WITH_FAILOVER_TIMEOUT;
    pConnApi->iCurrentMnglTimeout = 0;
    pConnApi->iTunnelPort = 3658;
    pConnApi->bDemanglerEnabled = TRUE;
    pConnApi->bAutoUpdate = TRUE;
#if !CONNAPI_XBOX_NETWORKING
    pConnApi->bDoAdvertising = TRUE;
#endif

    pConnApi->uNetMask = 0xffffffff;

    pConnApi->iQosDuration = 0; // QoS is disabled by default
    pConnApi->iQosInterval = 0;
    pConnApi->iQosPacketSize = 0;
    pConnApi->iGameHostIndex = -1;
    pConnApi->iVoipHostIndex = -1;

    // set default demangler server and create demangler
    ds_strnzcpy(pConnApi->strDemanglerServer, PROTOMANGLE_SERVER, sizeof(pConnApi->strDemanglerServer));

    // add update function to netconn idle handler
    NetConnIdleAdd(_ConnApiIdle, pConnApi);

    // return module state to caller
    return(pConnApi);
}

/*F********************************************************************************/
/*!
    \Function ConnApiOnline

    \Description
        This function should be called once the user has logged on and the input
        parameters are available

    \Input *pConnApi    - pointer to module state
    \Input *pGameName   - pointer to game resource string (eg cso/NCAA-2006/na)
    \Input uSelfId      - unique identifier for the local connapi client
    \Input eGameTopology- type of game
    \Input eVoipTopology- type of voip

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
void ConnApiOnline(ConnApiRefT *pConnApi, const char *pGameName, uint32_t uSelfId, ConnApiGameTopologyE eGameTopology, ConnApiVoipTopologyE eVoipTopology)
{
    char strAdvt[32];

    NetPrintf(("connapi: [%p] ConnApiOnline() invoked with uSelfId=0x%08x and pGameName=%s\n", pConnApi, uSelfId, pGameName));

    // save info
    ds_strnzcpy(pConnApi->strGameName, pGameName, sizeof(pConnApi->strGameName));
    pConnApi->uSelfId = uSelfId;

    // if voip was disabled before online was called change the topology to disabled
    if ((pConnApi->uConnFlags & CONNAPI_CONNFLAG_VOIPCONN) == 0)
    {
        NetPrintf(("connapi: [%p] requested voip topology ignored because voip globally disabled\n", pConnApi));
        eVoipTopology = CONNAPI_VOIPTOPOLOGY_DISABLED;
    }

    // save the topology and set the connection flags accordingly
    if ((pConnApi->eGameTopology = eGameTopology) == CONNAPI_GAMETOPOLOGY_DISABLED)
    {
        pConnApi->uConnFlags &= ~CONNAPI_CONNFLAG_GAMECONN;
    }
    if ((pConnApi->eVoipTopology = eVoipTopology) == CONNAPI_VOIPTOPOLOGY_DISABLED)
    {
        pConnApi->uConnFlags &= ~CONNAPI_CONNFLAG_VOIPCONN;
    }

    // get VoIP ref
    if ((pConnApi->eVoipTopology != CONNAPI_VOIPTOPOLOGY_DISABLED) && (pConnApi->pVoipRef == NULL))
    {
        if ((pConnApi->pVoipRef = VoipGetRef()) == NULL)
        {
            NetPrintf(("connapi: [%p] critical error! ConnApiOnline() is invoked on a voip-enabled ConnApi before VoipStartup() was called externally.\n", pConnApi));
            return;
        }
    }

    // set memory grouping, this requires DirtyMemGroupLeave() to be called before return
    DirtyMemGroupEnter(pConnApi->iMemGroup, pConnApi->pMemGroupUserData);

    // create util ref for subnet advertising
    if (pConnApi->bDoAdvertising)
    {
        NetPrintf(("connapi: [%p] creating NetGameUtil ref used for advertising purposes\n", pConnApi));
        if (pConnApi->pGameUtilRef == NULL)
        {
            if ((pConnApi->pGameUtilRef = NetGameUtilCreate()) == NULL)
            {
                NetPrintf(("connapi: [%p] failed to create the NetGameUtil ref used for advertising purposes\n", pConnApi));
                DirtyMemGroupLeave();
                return;
            }
        }
        else
        {
            NetPrintf(("connapi: [%p] can't create the NetGameUtil ref used for advertising purposes because there already exists one\n", pConnApi));
            DirtyMemGroupLeave();
            return;
        }
    }
    else
    {
        NetPrintf(("connapi: [%p] skipped creation of the NetGameUtil ref used for advertising purposes\n", pConnApi));
    }

    // on non-XboxOne platforms handle the cases where we should disable the demangler
    #if !CONNAPI_XBOX_NETWORKING
    if ((pConnApi->eGameTopology == CONNAPI_GAMETOPOLOGY_SERVERHOSTED) && (pConnApi->eVoipTopology != CONNAPI_VOIPTOPOLOGY_PEERWEB))
    {
        NetPrintf(("connapi: [%p] internally disabling demangler for mesh involving server-based game/voip topologies\n", pConnApi));
        pConnApi->bDemanglerEnabled = FALSE;
    }
    /* for CC-assisted scenarios, we don't allow demangling for meshes with potentially
       more than 2 players because connapi serializes the demangling attempts internally, and multiple
       back-to-back failing demangling attempts can induce a long wait time that will result in
       some clients not even having a chance to attempt CC-assisted path. */
    else if ((pConnApi->ClientList.iMaxClients > 2) && (pConnApi->iCcMode != CONNAPI_CCMODE_PEERONLY))
    {
        NetPrintf(("connapi: [%p] internally disabling demangler for CC-assisted mesh with a max player count (%d) larger than 2\n", pConnApi, pConnApi->ClientList.iMaxClients));
        pConnApi->bDemanglerEnabled = FALSE;
    }
    #endif

    // create demangler
    if ((pConnApi->bDemanglerEnabled) && (pConnApi->pProtoMangle == NULL))
    {
        #if CONNAPI_XBOX_NETWORKING
        NetPrintf(("connapi: [%p] creating demangler ref with max clients = %d\n", pConnApi, pConnApi->ClientList.iMaxClients));
        if ((pConnApi->pProtoMangle = ProtoMangleCreate(pConnApi->strDemanglerServer, pConnApi->ClientList.iMaxClients, pConnApi->strGameName, "")) == NULL)
        #else
        NetPrintf(("connapi: [%p] creating demangler ref with gamename=%s and server=%s\n", pConnApi, pConnApi->strGameName, pConnApi->strDemanglerServer));
        if ((pConnApi->pProtoMangle = ProtoMangleCreate(pConnApi->strDemanglerServer, PROTOMANGLE_PORT, pConnApi->strGameName, "")) == NULL)
        #endif
        {
            NetPrintf(("connapi: [%p] unable to create ProtoMangle module\n", pConnApi));
            pConnApi->bDemanglerEnabled = FALSE;
        }
        else
        {
            // now that we have a valid ProtoMangle, immediately make sure that ConnApi-driven demangler timeout is passed down to it
            ConnApiControl(pConnApi, 'dtim', pConnApi->iConfigMnglTimeout, 0, NULL);
            ConnApiControl(pConnApi, 'dtif', pConnApi->iConfigMnglTimeoutFailover, 0, NULL);
        }
    }

    // create tunnel module
    if ((pConnApi->bTunnelEnabled) && (pConnApi->pProtoTunnel == NULL))
    {
        if ((pConnApi->pProtoTunnel = ProtoTunnelCreate(pConnApi->ClientList.iMaxClients-1, pConnApi->iTunnelPort)) == NULL)
        {
            // unable to create, so disable the tunnel
            pConnApi->bTunnelEnabled = FALSE;
        }
        else
        {
            // we own the tunnel
            pConnApi->bTunnelOwner = TRUE;
        }
    }

    // set voip/gamelink timeouts
    ConnApiControl(pConnApi, 'time', pConnApi->iTimeout, 0, NULL);

    #if CONNAPI_XBOX_NETWORKING
    // set external session name and scid (calls into ProtoMangle)
    ConnApiControl(pConnApi, 'exsn', 0, 0, pConnApi->strExternalSessionName);
    ConnApiControl(pConnApi, 'exst', 0, 0, pConnApi->strExternalSessionTemplateName);
    ConnApiControl(pConnApi, 'scid', 0, 0, pConnApi->strScid);
    #endif

    ds_snzprintf(strAdvt, sizeof(strAdvt), "%u", pConnApi->uSelfId);

    // start advertising
    if (pConnApi->pGameUtilRef != NULL)
    {
        NetGameUtilAdvert(pConnApi->pGameUtilRef, pConnApi->strGameName, strAdvt, "");
    }

    // leave memory group
    DirtyMemGroupLeave();
}

/*F********************************************************************************/
/*!
    \Function ConnApiDestroy

    \Description
        Destroy the module state.

    \Input *pConnApi    - pointer to module state

    \Version 01/04/2005 (jbrookes)
*/
/********************************************************************************F*/
void ConnApiDestroy(ConnApiRefT *pConnApi)
{
    // disconnect
    ConnApiDisconnect(pConnApi);

    // remove idle handler
    NetConnIdleDel(_ConnApiIdle, pConnApi);

    VoipGroupDestroy(pConnApi->pVoipGroupRef);

    // destroy advertising gameutil ref
    if (pConnApi->pGameUtilRef != NULL)
    {
        NetGameUtilDestroy(pConnApi->pGameUtilRef);
    }

    // destroy tunnel, if present and we are the owner
    if ((pConnApi->pProtoTunnel != NULL) && (pConnApi->bTunnelOwner == TRUE))
    {
        ProtoTunnelDestroy(pConnApi->pProtoTunnel);
    }

    // destroy demangler
    if (pConnApi->pProtoMangle != NULL)
    {
        ProtoMangleDestroy(pConnApi->pProtoMangle);
    }

    // release module memory
    DirtyMemFree(pConnApi, CONNAPI_MEMID, pConnApi->iMemGroup, pConnApi->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function ConnApiConnect

    \Description
        Connect to a game.

    \Input *pConnApi            - pointer to module state
    \Input *pClientList         - list of clients in game session
    \Input iClientListSize      - number of clients in list
    \Input iGameHostIndex       - index in the client list who will be serving as game host
    \Input iVoipHostIndex       - index in the clinet list who will be serving as voip host
    \Input iSessId              - unique session identifier

    \Notes
        ConnApi supports invalid entries in the client list. Invalid clients are detected with a DirtyAddr that is zeroed out

    \Version 09/29/2009 (cvienneau)
*/
/********************************************************************************F*/
void ConnApiConnect(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientList, int32_t iClientListSize, int32_t iGameHostIndex, int32_t iVoipHostIndex, int32_t iSessId)
{
    NetPrintf(("connapi: [%p] ConnApiConnect() called with listSize=%d, gamehost=%d, voiphost=%d, sessionId=%d\n",
        pConnApi, iClientListSize, iGameHostIndex, iVoipHostIndex, iSessId));

    // make sure we're idle
    if (pConnApi->eState != ST_IDLE)
    {
        NetPrintf(("connapi: [%p] can't host or connect to a game when not in idle state\n", pConnApi));
        return;
    }

    // save session identifier
    pConnApi->iSessId = iSessId;

    // virtualize ports if tunneling
    if (pConnApi->bTunnelEnabled == TRUE)
    {
        NetConnControl('vadd', pConnApi->uGamePort, 0, NULL, NULL);

        if (pConnApi->eVoipTopology != CONNAPI_VOIPTOPOLOGY_DISABLED)
        {
            NetConnControl('vadd', pConnApi->uVoipPort, 0, NULL, NULL);
        }
    }

    // initialize the game / voip host index only in topologies they apply
    if ((pConnApi->eGameTopology == CONNAPI_GAMETOPOLOGY_PEERHOSTED) || (pConnApi->eGameTopology == CONNAPI_GAMETOPOLOGY_SERVERHOSTED))
    {
        pConnApi->iGameHostIndex = iGameHostIndex;
    }
    if (pConnApi->eVoipTopology == CONNAPI_VOIPTOPOLOGY_SERVERHOSTED)
    {
        pConnApi->iVoipHostIndex = iVoipHostIndex;
    }

    // init client list
    _ConnApiInitClientList(pConnApi, pClientList, iClientListSize);

    // let the voipgroup know if we are hosting or not
    VoipGroupControl(pConnApi->pVoipGroupRef, 'serv', (pConnApi->eVoipTopology == CONNAPI_VOIPTOPOLOGY_SERVERHOSTED), 0, NULL);
    
    #if CONNAPI_XBOX_NETWORKING
    if (pConnApi->bDemanglerEnabled == TRUE)
    {
        // let protomanglexboxone know what our index is
        ProtoMangleControl(pConnApi->pProtoMangle, 'self', pConnApi->iSelf, 0, NULL);
    }
    #endif

    // check for advertisement if necessary
    if (pConnApi->pGameUtilRef != NULL)
    {
        _ConnApiCheckAdvert(pConnApi);
    }

    pConnApi->eState = ST_INGAME;
}

/*F********************************************************************************/
/*!
    \Function ConnApiMigrateGameHost

    \Description
        Reopen all connections, using the host specified.
        This is for host migration to a different host in non-peerweb, needing new
        connections for everyone.

    \Input *pConnApi            - pointer to module state
    \Input iNewGameHostIndex    - index of the new game host

    \Version 09/21/2007 (jrainy)
*/
/********************************************************************************F*/
void ConnApiMigrateGameHost(ConnApiRefT *pConnApi, int32_t iNewGameHostIndex)
{
    ConnApiClientT *pClient;
    int32_t iClientIndex;
    ConnApiConnStatusE eStatus;

    pConnApi->iGameHostIndex = iNewGameHostIndex;
    pConnApi->eState = ST_INGAME;

    for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        pClient = &pConnApi->ClientList.Clients[iClientIndex];
        if (pClient->bAllocated && (pClient->GameInfo.eStatus != CONNAPI_STATUS_ACTV))
        {
            eStatus = pClient->GameInfo.eStatus;

            #if CONNAPI_XBOX_NETWORKING
            _ConnApiInitClientConnectionState(pConnApi, pClient, iClientIndex, CONNAPI_CONNFLAG_GAMECONN);
            #else
            pClient->GameInfo.eStatus = CONNAPI_STATUS_INIT;
            #endif

            _ConnApiUpdateCallback(pConnApi, iClientIndex, CONNAPI_CBTYPE_GAMEEVENT, eStatus, (ConnApiConnStatusE)pClient->GameInfo.eStatus);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function ConnApiAddClient

    \Description
        Add a new client to a pre-existing game at the specified index.

    \Input *pConnApi    - pointer to module state
    \Input *pClientInfo - info on joining user
    \Input iClientIndex - index to add client to

    \Output
        0 if successful, error code otherwise.

    \Notes
        This function should be called by all current members of a game while
        ConnApiConnect() is called by the joining client.

    \Version 06/16/2008 (jbrookes)
*/
/********************************************************************************F*/
int32_t ConnApiAddClient(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientInfo, int32_t iClientIndex)
{
    ConnApiClientT *pClient;

    // make sure we're not idle
    if (pConnApi->eState == ST_IDLE)
    {
        NetPrintf(("connapi: [%p] can't add a connection to a game in idle state\n", pConnApi));
        return(CONNAPI_ERROR_INVALID_STATE);
    }

    // make sure there is room
    if (pConnApi->ClientList.iNumClients == pConnApi->ClientList.iMaxClients)
    {
        NetPrintf(("connapi: [%p] can't add a connection to the game because it is full\n", pConnApi));
        return(CONNAPI_ERROR_CLIENTLIST_FULL);
    }

    // make sure the selected slot is valid
    if ((iClientIndex < 0) || (iClientIndex >= pConnApi->ClientList.iMaxClients))
    {
        NetPrintf(("connapi: [%p] can't add a connection to the game in slot %d because valid slot range 0-%d\n", pConnApi, iClientIndex, pConnApi->ClientList.iMaxClients-1));
        return(CONNAPI_ERROR_SLOT_OUT_OF_RANGE);
    }

    // get pointer to new client structure to fill in, and increment client count
    pClient = &pConnApi->ClientList.Clients[iClientIndex];

    // check slot and make sure it is uninitialized
    if (pClient->bAllocated == TRUE)
    {
        NetPrintf(("connapi: [%p] slot %d already allocated; cannot add a new client in this slot\n", pConnApi, iClientIndex));
        return(CONNAPI_ERROR_SLOT_USED);
    }

    // add client to list
    _ConnApiInitClient(pConnApi, pClient, pClientInfo, iClientIndex);

    // display client info
    #if DIRTYCODE_LOGGING
    NetPrintf(("connapi: [%p] adding client to clientlist\n", pConnApi));
    _ConnApiDisplayClientInfo(&pConnApi->ClientList.Clients[iClientIndex], iClientIndex);
    #endif

    // increment client count
    pConnApi->ClientList.iNumClients += 1;

    // check for advertisement if necessary
    if (pConnApi->pGameUtilRef != NULL)
    {
        _ConnApiCheckAdvert(pConnApi);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function ConnApiFindClient

    \Description
        Returns the ConnApiClientT of a given client, if found by id.

    \Input *pConnApi    - pointer to module state
    \Input *pClientInfo - info on searched user
    \Input *pOutClient  - used to return the ClientT structure of the client

    \Output
        uint8_t         - TRUE if the client is found, FALSE otherwise

    \Version 06/05/2008 (jbrookes)
*/
/********************************************************************************F*/
uint8_t ConnApiFindClient(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientInfo, ConnApiClientT *pOutClient)
{
    int32_t iClient;

    for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
    {
        if (pConnApi->ClientList.Clients[iClient].ClientInfo.uId == pClientInfo->uId)
        {
            ds_memcpy_s(pOutClient, sizeof(*pOutClient), &pConnApi->ClientList.Clients[iClient], sizeof(pConnApi->ClientList.Clients[iClient]));
            return(TRUE);
        }
    }
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function ConnApiRemoveClient

    \Description
        Remove a current client from a game.

    \Input *pConnApi    - pointer to module state
    \Input iClientIndex - index of client to remove (used if pClientName is NULL)

    \Notes
        If this function is called inside of a ConnApi callback, the removal will
        be deferred until the next time NetConnIdle() is called.  Otherwise, the
        removal will happen immediately.

    \Version 06/14/2008 (jbrookes)
*/
/********************************************************************************F*/
void ConnApiRemoveClient(ConnApiRefT *pConnApi, int32_t iClientIndex)
{
    // make sure the select slot is valid
    if ((iClientIndex < 0) || (iClientIndex >= pConnApi->ClientList.iMaxClients))
    {
        NetPrintf(("connapi: [%p] can't remove a connection from the game in slot %d because valid slot range is 0-%d\n", pConnApi, iClientIndex, pConnApi->ClientList.iMaxClients-1));
        return;
    }

    _ConnApiRemoveClientSetup(pConnApi, iClientIndex, CONNAPI_CLIENTFLAG_REMOVE);
}

/*F********************************************************************************/
/*!
    \Function ConnApiDisconnect

    \Description
        Stop game, disconnect from clients, and reset client list.

    \Input *pConnApi    - pointer to module state

    \Notes
        Any NetGameDistRefs created by the application that references a NetGameUtil/
        NeGameLink combination created by ConnApi must destroy the DistRef(s) before
        calling this function.

    \Version 01/04/2005 (jbrookes)
*/
/********************************************************************************F*/
void ConnApiDisconnect(ConnApiRefT *pConnApi)
{
    ConnApiClientT *pClient;
    int32_t iClient;

    NetPrintf(("connapi: [%p] disconnecting\n", pConnApi));

    // make sure we're not idle
    if (pConnApi->eState == ST_IDLE)
    {
        NetPrintf(("connapi: [%p] can't disconnect when in idle state\n", pConnApi));
        return;
    }

    // walk client list
    for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
    {
        // ref client
        pClient = &pConnApi->ClientList.Clients[iClient];

        // if it's not us and was allocated, disconnect from them
        if ((iClient != pConnApi->iSelf) && pClient->bAllocated)
        {
            _ConnApiDisconnectClient(pConnApi, &pConnApi->ClientList.Clients[iClient], iClient, "disconnect");
        }
    }

    // reset client list
    pConnApi->ClientList.iNumClients = 0;
    ds_memclr(&pConnApi->ClientList.Clients, pConnApi->ClientList.iMaxClients * sizeof(ConnApiClientT));

    // devirtualize ports if tunneling is enabled
    if (pConnApi->bTunnelEnabled == TRUE)
    {
        NetConnControl('vdel', pConnApi->uGamePort, 0, NULL, NULL);

        if (pConnApi->eVoipTopology != CONNAPI_VOIPTOPOLOGY_DISABLED)
        {
            NetConnControl('vdel', pConnApi->uVoipPort, 0, NULL, NULL);
        }
    }

    // clear socket refs
    pConnApi->uTunlSockRef = 0;
    pConnApi->uGameSockRef = 0;
    pConnApi->uVoipSockRef = 0;

    // go to idle state
    pConnApi->eState = ST_IDLE;
}

/*F********************************************************************************/
/*!
    \Function ConnApiGetClientList

    \Description
        Get a list of current connections.

    \Input *pConnApi            - pointer to module state

    \Output
        ConnApiClientListT *    - pointer to client list

    \Version 01/04/2005 (jbrookes)
*/
/********************************************************************************F*/
const ConnApiClientListT *ConnApiGetClientList(ConnApiRefT *pConnApi)
{
    return(&pConnApi->ClientList);
}

/*F********************************************************************************/
/*!
    \Function ConnApiStatus

    \Description
        Get status information.

    \Input *pConnApi    - pointer to module state
    \Input iSelect      - status selector
    \Input *pBuf        - [out] storage for selector-specific output
    \Input iBufSize     - size of output buffer

    \Output
        int32_t         - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'cbfp' - return current callback function pointer in output buffer
            'cbup' - return current callback data pointer in output buffer
            'ctim' - returns the connection timeout
            'dtim' - get effective demangler timeout (in milliseconds)
            'ghst' - return the the host index ConnApiClientT (via pBuf) for the game host in peer hosted and server hosted games
            'gprt' - return game port
            'gsrv' - return whether the game is server hosted (ConnApiClientT returned via pBuf)
            'ingm' - currently 'in game' (connecting to or connected to one or more peers)
            'lbuf' - returns GameLink buffer allocation size
            'lclt' - returns one past the index of the last allocated player
            'lcon' - returns the number of consoles (excluding the game server) allocated in the client list
            'minp' - returns GameLink input buffer queue length (zero=default)
            'mngl' - returns whether demangler is enabled or not
            'mout' - returns GameLink output buffer queue length (zero=default)
            'mplr' - *deprecated - replaced by 'lclt'* returns one past the index of the last allocated player
            'mwid' - returns GameLink max packet size (zero=default)
            'nmsk' - returns current netmask
            'peer' - returns game conn peer-web enable/disable status
            'self' - returns index of local user in client list
            'sock' - copy socket ref to pBuf
            'sess' - copies session information into output buffer
            'time' - returns the timeout
            'tprt' - returns port tunnel has been bound to (if available)
            'tref' - returns the prototunnel ref used
            'tunl' - returns whether tunnel is enabled or not
            'type' - returns current connection type (CONNAPI_CONNFLAG_*)
            'vhst' - return the host index and ConnApiClientT (via pBuf) for the voip host in server hosted voip
            'vprt' - return voip port
        \endverbatim

    \Version 01/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ConnApiStatus(ConnApiRefT *pConnApi, int32_t iSelect, void *pBuf, int32_t iBufSize)
{
    return(ConnApiStatus2(pConnApi, iSelect, NULL, pBuf, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function ConnApiStatus2

    \Description
        Get status information.

    \Input *pConnApi    - pointer to module state
    \Input iSelect      - status selector
    \Input *pData       - input data
    \Input *pBuf        - [out] storage for selector-specific output
    \Input iBufSize     - size of output buffer

    \Output
        int32_t         - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'cadr' - stands for Connection Address, ip address used at the platform socket level to reach this client (regardless of tunneling being used or not)
            'cbfp' - return current callback function pointer in output buffer
            'cbup' - return current callback data pointer in output buffer
            'cprt' - stands for Connection Port, UDP port used at the platform socket level to reach this client
            'ctim' - returns the connection timeout
            'dtim' - get effective demangler timeout (in milliseconds)
            'ghst' - return the the host index ConnApiClientT (via pBuf) for the game host in peer hosted and server hosted games
            'gprt' - return game port
            'gsrv' - return whether the game is server hosted (ConnApiClientT returned via pBuf)
            'host' - returns whether hosting or not, plus copies host name to buffer
            'ingm' - currently 'in game' (connecting to or connected to one or more peers)
            'lbuf' - returns GameLink buffer allocation size
            'lclt' - returns one past the index of the last allocated player
            'lcon' - returns the number of consoles (excluding the game server) allocated in the client list
            'minp' - returns GameLink input buffer queue length (zero=default)
            'mngl' - returns whether demangler is enabled or not
            'mout' - returns GameLink output buffer queue length (zero=default)
            'mplr' - *deprecated - replaced by 'lclt'* returns one past the index of the last allocated player
            'mvtm' - return true if multiple virtual machine mode is active
            'mwid' - returns GameLink max packet size (zero=default)
            'nmsk' - returns current netmask
            'peer' - returns game conn peer-web enable/disable status
            'self' - returns index of local user in client list
            'sock' - copy socket ref to pBuf
            'sess' - copies session information into output buffer
            'time' - returns the timeout
            'tprt' - returns port tunnel has been bound to (if available)
            'tref' - returns the prototunnel ref used
            'tunl' - returns whether tunnel is enabled or not
            'tunr' - returns receive protunnel stats as a ProtoTunnelStatT in pBuf for a given client pData
            'tuns' - returns send prototunnel stats as a ProtoTunnelStatT in pBuf for a given client pData
            'type' - returns current connection type (CONNAPI_CONNFLAG_*)
            'vgrp' - returns voipgroup pointer in pBuf
            'vhst' - return the host index and ConnApiClientT (via pBuf) for the voip host in server hosted voip
            'vprt' - return voip port
        \endverbatim

    \Version 01/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ConnApiStatus2(ConnApiRefT *pConnApi, int32_t iSelect, void *pData, void *pBuf, int32_t iBufSize)
{
    if ((iSelect == 'cadr') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(uint32_t)))
    {
        uint8_t bLocalAddr;
        ConnApiClientT *pClientUsed;
        ConnApiClientT *pClient = (ConnApiClientT *)pData;
        struct sockaddr addr;

        *(uint32_t*)pBuf = _ConnApiGetConnectAddr(pConnApi, pClient, &bLocalAddr, CONNAPI_CONNFLAG_GAMECONN, &pClientUsed);

        // if we are using the tunnel return the tunnel address
        if (pConnApi->pProtoTunnel != NULL)
        {
            ProtoTunnelStatus(pConnApi->pProtoTunnel, 'vtop', pClient->iTunnelId, &addr, sizeof(addr));
             *(uint32_t*)pBuf = SockaddrInGetAddr(&addr);
        }

        if (pClientUsed->GameInfo.eStatus != CONNAPI_STATUS_ACTV)
        {
            return(-1);
        }

        return(0);
    }
    if ((iSelect == 'cbfp') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(pConnApi->pCallback[0])))
    {
        ds_memcpy(pBuf, &(pConnApi->pCallback[0]), sizeof(pConnApi->pCallback[0]));
        return(0);
    }
    if ((iSelect == 'cbup') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(pConnApi->pUserData[0])))
    {
        ds_memcpy(pBuf, &(pConnApi->pUserData[0]), sizeof(pConnApi->pUserData[0]));
        return(0);
    }
    if ((iSelect == 'cprt') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(uint16_t)))
    {
        uint8_t bLocalAddr;
        ConnApiClientT *pClientUsed;
        ConnApiClientT *pClient = (ConnApiClientT *)pData;

        _ConnApiGetConnectAddr(pConnApi, pClient, &bLocalAddr, CONNAPI_CONNFLAG_GAMECONN, &pClientUsed);

        if (pClientUsed->GameInfo.eStatus != CONNAPI_STATUS_ACTV)
        {
            return(-1);
        }

        if (pClientUsed->iTunnelId > 0)
        {
            ProtoTunnelStatus(pConnApi->pProtoTunnel, 'rprt', pClientUsed->iTunnelId, pBuf, iBufSize);
        }
        else
        {
            *(uint16_t*)pBuf = pClientUsed->GameInfo.uMnglPort;
        }
        return(0);
    }
    if (iSelect == 'ctim')
    {
        return(pConnApi->iConnTimeout);
    }
    if (iSelect == 'dtim')
    {
        return(pConnApi->iCurrentMnglTimeout);
    }
    if ((iSelect == 'ghst') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(ConnApiClientT)))
    {
        if ((pConnApi->eGameTopology == CONNAPI_GAMETOPOLOGY_PEERHOSTED) || (pConnApi->eGameTopology == CONNAPI_GAMETOPOLOGY_SERVERHOSTED))
        {
            ds_memcpy(pBuf, &pConnApi->ClientList.Clients[pConnApi->iGameHostIndex], sizeof(ConnApiClientT));
            return(pConnApi->iGameHostIndex);
        }
        return(-1);
    }
    if (iSelect == 'gprt')
    {
        return(pConnApi->uGamePort);
    }
    if (iSelect == 'gsrv')
    {
        uint8_t bHostIsGameServer;
        if ((bHostIsGameServer = (pConnApi->eGameTopology == CONNAPI_GAMETOPOLOGY_SERVERHOSTED)) == TRUE)
        {
            if ((pBuf != NULL) && (iBufSize >= (signed)sizeof(ConnApiClientT)))
            {
                ds_memcpy(pBuf, &pConnApi->ClientList.Clients[pConnApi->iGameHostIndex], sizeof(ConnApiClientT));
            }
        }
        return(bHostIsGameServer);
    }
    if (iSelect == 'ingm')
    {
        return(pConnApi->eState != ST_IDLE);
    }
    if (iSelect == 'lbuf')
    {
        return(pConnApi->iLinkBufSize);
    }
    if (iSelect == 'lcon')
    {
        int32_t iNumberPlayers = ConnApiStatus2(pConnApi, 'lclt', pData, pBuf, iBufSize);

        // if the game is server hosted we do not want to include the server
        if (ConnApiStatus(pConnApi, 'gsrv', NULL, 0))
        {
            --iNumberPlayers;
        }

        return(iNumberPlayers);
    }
    if (iSelect == 'minp')
    {
        return(pConnApi->iGameMinp);
    }
    if (iSelect == 'mngl')
    {
        return(pConnApi->bDemanglerEnabled);
    }
    if (iSelect == 'mout')
    {
        return(pConnApi->iGameMout);
    }
    if ((iSelect == 'mplr') || (iSelect == 'lclt'))
    {
        int32_t iClient;
        int32_t iNumberPlayers = 0;
        for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
        {
            if (pConnApi->ClientList.Clients[iClient].bAllocated)
            {
                iNumberPlayers = iClient + 1;
            }
        }
        return(iNumberPlayers);
    }
    if (iSelect == 'mwid')
    {
        return(pConnApi->iGameMwid);
    }
    if (iSelect == 'nmsk')
    {
        return(pConnApi->uNetMask);
    }
    if (iSelect == 'peer')
    {
        return(pConnApi->eGameTopology == CONNAPI_GAMETOPOLOGY_PEERWEB);
    }
    if (iSelect == 'self')
    {
        return(pConnApi->iSelf);
    }
    if (iSelect == 'sess')
    {
        ds_strnzcpy((char *)pBuf, pConnApi->strSession, iBufSize);
        return(0);
    }
    if (iSelect == 'sock')
    {
        if (iBufSize >= (signed)sizeof(intptr_t))
        {
            if (pConnApi->bTunnelEnabled)
            {
                ProtoTunnelStatus(pConnApi->pProtoTunnel, 'sock', 0, pBuf, iBufSize);
            }
            else if ((pConnApi->iGameHostIndex >= 0) && (pConnApi->ClientList.Clients[pConnApi->iGameHostIndex].pGameUtilRef != NULL))
            {
                NetGameUtilStatus(pConnApi->ClientList.Clients[pConnApi->iGameHostIndex].pGameUtilRef, 'sock', pBuf, iBufSize);
            }
            else
            {
                NetPrintf(("connapi: [%p] ConnApiStatus('sock') failed because game socket not yet available (tunnel enabled = %s)\n", pConnApi,
                    pConnApi->bTunnelEnabled?"true":"false"));
                return(-2);
            }
            return(0);
        }
        else
        {
            NetPrintf(("connapi: [%p] ConnApiStatus('sock') failed because size (%d) of user-provided buffer is too small (required: %d)\n", pConnApi,
                iBufSize, sizeof(intptr_t)));
            return(-1);
        }
    }
    if (iSelect == 'time')
    {
        return(pConnApi->iTimeout);
    }
    if ((iSelect == 'tprt') && (pConnApi->pProtoTunnel != NULL))
    {
        return(ProtoTunnelStatus(pConnApi->pProtoTunnel, 'lprt', 0, NULL, 0));
    }
    if (iSelect == 'tref' && (pConnApi->pProtoTunnel != NULL))
    {
        if (iBufSize >= (signed)sizeof(ProtoTunnelRefT*))
        {
            ds_memcpy(pBuf, &pConnApi->pProtoTunnel, sizeof(ProtoTunnelRefT*));
            return(0);
        }
        else
        {
            NetPrintf(("connapi: [%p] ConnApiStatus('tref') failed because size (%d) of user-provided buffer is too small (required: %d)\n", pConnApi,
                iBufSize, sizeof(ProtoTunnelRefT*)));
            return(-1);
        }
    }
    if (iSelect == 'tunl')
    {
        return(pConnApi->bTunnelEnabled);
    }
    if (iSelect == 'tunr')
    {
        if ((pData != NULL) && (pBuf != NULL) && (iBufSize == sizeof(ProtoTunnelStatT)))
        {
            ConnApiClientT *pClient = (ConnApiClientT *)pData;
            ProtoTunnelStatus(pConnApi->pProtoTunnel, 'rcvs', pClient->iTunnelId, pBuf, iBufSize);
            return(0);
        }
        else
        {
            NetPrintf(("connapi: [%p] ConnApiStatus('tunr') failed due to invalid arguments\n", pConnApi));
            return(-1);
        }
    }
    if (iSelect == 'tuns')
    {
        if ((pData != NULL) && (pBuf != NULL) && (iBufSize == sizeof(ProtoTunnelStatT)))
        {
            ConnApiClientT *pClient = (ConnApiClientT *)pData;
            ProtoTunnelStatus(pConnApi->pProtoTunnel, 'snds', pClient->iTunnelId, pBuf, iBufSize);
            return(0);
        }
        else
        {
            NetPrintf(("connapi: [%p] ConnApiStatus('tuns') failed due to invalid arguments\n", pConnApi));
            return(-1);
        }
    }
    if (iSelect == 'type')
    {
        return(pConnApi->uConnFlags);
    }
    if (iSelect == 'ulmt')
    {
        return(pConnApi->iGameUnackLimit);
    }
    if (iSelect == 'vgrp')
    {
        if (iBufSize >= (int32_t)sizeof(pConnApi->pVoipGroupRef))
        {
            ds_memcpy(pBuf, &pConnApi->pVoipGroupRef, sizeof(pConnApi->pVoipGroupRef));
            return(0);
        }
        return(-1);
    }
    if ((iSelect == 'vhst') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(ConnApiClientT)))
    {
        if (pConnApi->eVoipTopology == CONNAPI_VOIPTOPOLOGY_SERVERHOSTED)
        {
            ds_memcpy(pBuf, &pConnApi->ClientList.Clients[pConnApi->iVoipHostIndex], sizeof(ConnApiClientT));
            return(pConnApi->iVoipHostIndex);
        }
        return(-1);
    }
    if (iSelect == 'vprt')
    {
        return(pConnApi->uVoipPort);
    }
    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ConnApiControl

    \Description
        Control behavior of module.

    \Input *pConnApi    - pointer to module state
    \Input iControl     - status selector
    \Input iValue       - control value
    \Input iValue2      - control value
    \Input *pValue      - control value

    \Output
        int32_t             - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'adve' - set to enable ProtoAdvt advertising
            'auto' - set auto-update enable/disable - iValue=TRUE or FALSE (default TRUE)
            'cbfp' - set callback function pointer - pValue=pCallback
            'cbup' - set callback user data pointer - pValue=pUserData
            'ccmd' - set the CC mode  (CONNAPI_CCMODE_*)
            'ctim' - set connection timeout - iValue=timeout (minimum & default 10000 ms)
            'dist' - set dist ref - iValue=index of client or 'gsrv' for host user, pValue=dist ref
            'dsrv' - set demangler server - pValue=pointer to demangler server name (default demangler.ea.com)
            'dtif' - set demangler timeout for scenarios involving or CC assistance - iValue=timeout in milliseconds
            'dtim' - set demangler timeout - iValue=timeout in milliseconds
            'exsn' - set the external session name for the MultiplayerSessionReference (xbox one only)
            'exst' - set the external session template name for the MultiplayerSessionReference (xbox one only)
            'estv' - enable flag to establish voip for a client after it was delayed, iValue=client index
            'gprt' - set game port to use - iValue=port
            'lbuf' - set game link buffer size - iValue=size (default 1024)
            'lqos' - set QoS packet size used when creating NetGameLinks. iValue=packet size in bytes
            'maxg' - set maximum number of voipgroups we support
            'meta' - enable/disable commudp metadata
            'minp' - set GameLink input buffer queue length - iValue=length (default 32)
            'mngl' - set demangler enable/disable - iValue=TRUE/FALSE (default TRUE)
            'mout' - set GameLink output buffer queue length - iValue=length (default 32)
            'mvtm' - set multiple virtual machine mode enable/disable - iValue=TRUE/FALSE (default FALSE)
            'mwid' - set GameLink max packet size - iValue=size (default NETGAME_DATAPKT_DEFSIZE, max NETGAME_DATAPKT_MAXSIZE)
            'nmsk' - set netmask used for external address comparisons - iValue=mask (default 0xffffffff)
            'rcbk' - set enable of disc callback on removal - iValue=TRUE/FALSE (default FALSE)
            'scid' - set the service configuration id for the MultiplayerSessionReference (xbox one only)
            'sqos' - set QoS settings used when creating NetGameLinks. iValue=QoS duration (0 disables QoS), iValue2=QoS packet interval
            'stun' - set prototunnel ref
            'tctl' - set prototunnel control data
            'time' - set timeout - iValue=timeout in ms (default 15 seconds)
            'tgam' - enable the override of game tunnel flags - iValue=falgs, iValue2=boolean(overrides or not)
            'tunl' - set tunnel parms:
                         iValue = TRUE/FALSE to enable/disable or negative to ignore
                         iValue2 = tunnel port, or negative to ignore
            'type' - set connection type - iValue = CONNAPI_CONNFLAG_*
            'voig' - pass-through to VoipGroupControl() - 'getr' VoipGroupControl selector has been deprecated please switch to ConnApiStatus('vgrp') instead
            'voip' - pass-through to VoipControl() - iValue=VoIP iControl, iValue2= VoIP iValue
            'vprt' - set voip port to use - iValue=port
            'vset' - set voip enable/disable (default=TRUE; call before ConnApiOnline())
            '!res' - force secure address resolution to fail, to simulate p2p connection failure.
                         iValue  = TRUE/FALSE to enable/disable
                         iValue2 = index of user to force fail or -1 for all users or local index for all users
        \endverbatim

    \Version 01/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ConnApiControl(ConnApiRefT *pConnApi, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'adve')
    {
        #if CONNAPI_XBOX_NETWORKING
        NetPrintf(("connapi: ProtoAdvt advertising cannot be enabled on Xbox One\n"));
        #else
        NetPrintf(("connapi: [%p] ProtoAdvt advertising %s\n", pConnApi, (iValue?"enabled":"disabled")));
        pConnApi->bDoAdvertising = iValue;
        // if disabling advertising and we already have an advertising ref, kill it
        if ((pConnApi->bDoAdvertising == FALSE) && (pConnApi->pGameUtilRef != NULL))
        {
            NetGameUtilDestroy(pConnApi->pGameUtilRef);
            pConnApi->pGameUtilRef = NULL;
        }
        #endif
        return(0);
    }
    if (iControl == 'auto')
    {
        pConnApi->bAutoUpdate = iValue;
        return(0);
    }
    if (iControl == 'ccmd')
    {
        #if DIRTYCODE_LOGGING
        static const char *_ConnApiCcModeNames[] =
        {
            "CONNAPI_CCMODE_PEERONLY",
            "CONNAPI_CCMODE_HOSTEDONLY",
            "CONNAPI_CCMODE_HOSTEDFALLBACK"
        };
        #endif

        switch (iValue)
        {
            case (CONNAPI_CCMODE_PEERONLY):
                VoipGroupControl(pConnApi->pVoipGroupRef, 'ccmd', VOIPGROUP_CCMODE_PEERONLY, 0, NULL);
                break;
            case (CONNAPI_CCMODE_HOSTEDONLY):
                VoipGroupControl(pConnApi->pVoipGroupRef, 'ccmd', VOIPGROUP_CCMODE_HOSTEDONLY, 0, NULL);
                break;
            case (CONNAPI_CCMODE_HOSTEDFALLBACK):
                VoipGroupControl(pConnApi->pVoipGroupRef, 'ccmd', VOIPGROUP_CCMODE_HOSTEDFALLBACK, 0, NULL);
                break;
            default:
                NetPrintf(("connapi: [%p] unsupported CC mode %d\n", pConnApi, iValue));
                return(-1);
        }
        NetPrintf(("connapi: [%p] CC mode = %d (%s)\n", pConnApi, iValue, _ConnApiCcModeNames[iValue]));
        pConnApi->iCcMode = iValue;
        return(0);
    }
    if (iControl == 'cbfp')
    {
        // set callback function pointer
        pConnApi->pCallback[0] = ((pValue != NULL) ? (ConnApiCallbackT *)pValue : _ConnApiDefaultCallback);
        return(0);
    }
    if (iControl == 'cbup')
    {
        // set callback user data pointer
        pConnApi->pUserData[0] = pValue;
        return(0);
    }
    if ((iControl == 'ctim') && (iValue >= CONNAPI_CONNTIMEOUT_DEFAULT))
    {
        NetPrintf(("connapi: [%p] setting connection timeout to %d\n", pConnApi, iValue));
        pConnApi->iConnTimeout = iValue;
        return(0);
    }
    if (iControl == 'dist')
    {
        // set dist ref for specified client

        // special value to signify the host
        if (iValue == 'gsrv')
        {
            iValue = pConnApi->iGameHostIndex;
        }

        if ((iValue >= 0) && (iValue < pConnApi->ClientList.iMaxClients))
        {
            pConnApi->ClientList.Clients[iValue].pGameDistRef = (NetGameDistRefT *)pValue;
            return(0);
        }
    }
    if (iControl == 'dsrv')
    {
        // set demangler server
        ds_strnzcpy(pConnApi->strDemanglerServer, (const char *)pValue, sizeof(pConnApi->strDemanglerServer));
        return(0);
    }
    if ((iControl == 'dtim') || (iControl == 'dtif'))
    {
        // set demangler timeout
        int32_t iNewMnglTimeout;
        #if DIRTYCODE_LOGGING
        uint8_t bIsFailoverPossible = FALSE;
        #endif

        // special case to allow using a different timeout value for connapi involving CC assistance
        if (iControl == 'dtif')
        {
            NetPrintf(("connapi: [%p] changing demangler_with_failover timeout config (%d ms --> %d ms)\n", pConnApi, pConnApi->iConfigMnglTimeoutFailover, iValue));
            pConnApi->iConfigMnglTimeoutFailover = iValue;
        }
        else
        {
            NetPrintf(("connapi: [%p] changing demangler timeout config (%d ms --> %d ms)\n", pConnApi, pConnApi->iConfigMnglTimeout, iValue));
            pConnApi->iConfigMnglTimeout = iValue;
        }

        // check if the special timeout value for cc scenarios is applicable
        if (pConnApi->iCcMode != CONNAPI_CCMODE_PEERONLY)
        {
            #if DIRTYCODE_LOGGING
            bIsFailoverPossible = TRUE;
            #endif
            iNewMnglTimeout = pConnApi->iConfigMnglTimeoutFailover;
        }
        else
        {
            iNewMnglTimeout = pConnApi->iConfigMnglTimeout;
        }

        // pass new effective demangler timeout value down to ProtoMangle
        if ((pConnApi->pProtoMangle != NULL) && (iNewMnglTimeout != pConnApi->iCurrentMnglTimeout))
        {
            NetPrintf(("connapi: [%p] applying new demangler timeout (%d ms --> %d ms) for a demangling scenario %s\n",
                pConnApi, pConnApi->iCurrentMnglTimeout, iNewMnglTimeout, (bIsFailoverPossible?"with":"without")));
            pConnApi->iCurrentMnglTimeout = iNewMnglTimeout;
            ProtoMangleControl(pConnApi->pProtoMangle, 'time', pConnApi->iCurrentMnglTimeout, 0, NULL);
        }

        return(0);
    }
    #if CONNAPI_XBOX_NETWORKING
    if (iControl == 'exsn')
    {
        if ((pValue == NULL) || (*(char*)pValue == '\0'))
        {
            NetPrintf(("connapi: [%p] 'exsn', invalid external session name\n", pConnApi));
        }
        else
        {
            if (pConnApi->strExternalSessionName != pValue)
            {
                ds_strnzcpy(pConnApi->strExternalSessionName, (char*)pValue, sizeof(pConnApi->strExternalSessionName));
                NetPrintf(("connapi: [%p] 'exsn', external session name saved as (%s)\n", pConnApi, pConnApi->strExternalSessionName));
            }
            if (pConnApi->pProtoMangle != NULL)
            {
                return(ProtoMangleControl(pConnApi->pProtoMangle, 'exsn', 0, 0, pConnApi->strExternalSessionName));
            }
            return(0);
        }
    }
    if (iControl == 'exst')
    {
        if ((pValue == NULL) || (*(char*)pValue == '\0'))
        {
            NetPrintf(("connapi: [%p] 'exst', invalid external session template name\n", pConnApi));
        }
        else
        {
            if (pConnApi->strExternalSessionTemplateName != pValue)
            {
                ds_strnzcpy(pConnApi->strExternalSessionTemplateName, (char*)pValue, sizeof(pConnApi->strExternalSessionTemplateName));
                NetPrintf(("connapi: [%p] 'exst', external session template name saved as (%s)\n", pConnApi, pConnApi->strExternalSessionTemplateName));
            }
            if (pConnApi->pProtoMangle != NULL)
            {
                return(ProtoMangleControl(pConnApi->pProtoMangle, 'exst', 0, 0, pConnApi->strExternalSessionTemplateName));
            }
            return(0);
        }
    }
    #endif
    if (iControl == 'gprt')
    {
        NetPrintf(("connapi: [%p] using game port %d\n", pConnApi, iValue));
        pConnApi->uGamePort = (uint16_t)iValue;
        return(0);
    }
    if (iControl == 'lbuf')
    {
        // set game link buffer size
        pConnApi->iLinkBufSize = iValue;
        return(0);
    }
    if (iControl == 'maxg')
    {
        // set maximum number of voipgroups
        NetPrintf(("connapi: changing maximum number of voipgroups from %d to %d\n", _ConnApi_iMaxVoipGroups, iValue));
        _ConnApi_iMaxVoipGroups = (int8_t)iValue;
        return(0);
    }
    if (iControl == 'meta')
    {
        // enable/disable commudp metadata
        NetPrintf(("connapi: [%p] commudp metadata %s\n", pConnApi, iValue ? "enabled" : "disabled"));
        pConnApi->bCommUdpMetadata = iValue;
        return(0);
    }
    if (iControl == 'minp')
    {
        // set gamelink input packet queue length
        pConnApi->iGameMinp = iValue;
        return(0);
    }
    if (iControl == 'mngl')
    {
        #if CONNAPI_XBOX_NETWORKING
        NetPrintf(("connapi: [%p] demangler cannot be disabled on Xbox One\n", pConnApi));
        #else
        // set demangler enable/disable
        NetPrintf(("connapi: [%p] demangling %s\n", pConnApi, iValue ? "enabled" : "disabled"));
        pConnApi->bDemanglerEnabled = iValue;
        #endif
        return(0);
    }
    if (iControl == 'mout')
    {
        // set gamelink output packet queue length
        pConnApi->iGameMout = iValue;
        return(0);
    }
    if (iControl == 'mwid')
    {
        // set gamelink packet length
        pConnApi->iGameMwid = iValue;
        return(0);
    }
    if (iControl == 'nmsk')
    {
        // set netmask
        pConnApi->uNetMask = (unsigned)iValue;
        return(0);
    }
    if (iControl == 'rcbk')
    {
        // enable disc callback on removal
        pConnApi->bRemoveCallback = iValue;
        return(0);
    }
    #if CONNAPI_XBOX_NETWORKING
    if (iControl == 'scid')
    {
        if ((pValue == NULL) || (*(char*)pValue == '\0'))
        {
            NetPrintf(("connapi: [%p] 'scid', invalid service configuration id\n", pConnApi));
        }
        else
        {
            if (pConnApi->strScid != pValue)
            {
                ds_strnzcpy(pConnApi->strScid, (char*)pValue, sizeof(pConnApi->strScid));
                NetPrintf(("connapi: [%p] 'scid', service configuration name saved as (%s)\n", pConnApi, pConnApi->strScid));
            }
            if (pConnApi->pProtoMangle != NULL)
            {
                return(ProtoMangleControl(pConnApi->pProtoMangle, 'scid', 0, 0, pConnApi->strScid));
            }
            return(0);
        }
    }
    #endif
    if ((iControl == 'stun') && (pConnApi->pProtoTunnel == NULL) && (pValue != NULL))
    {
        // set prototunnel ref
        pConnApi->pProtoTunnel = (ProtoTunnelRefT *)pValue;
        return(0);
    }
    if (iControl == 'time')
    {
        // set timeout
        pConnApi->iTimeout = iValue;
        VoipGroupControl(pConnApi->pVoipGroupRef, 'time', iValue, 0, NULL);
        return(0);
    }
    if ((iControl == 'tctl') && (pConnApi->pProtoTunnel != NULL))
    {
        return(ProtoTunnelControl(pConnApi->pProtoTunnel, iValue, iValue2, 0, pValue));
    }
    if (iControl == 'tgam')
    {
        pConnApi->uGameTunnelFlag = iValue;
        pConnApi->uGameTunnelFlagOverride = iValue2;
    }
    if (iControl == 'tunl')
    {
        // set tunnel status
        if (iValue >= 0)
        {
            pConnApi->bTunnelEnabled = iValue;
            VoipGroupControl(pConnApi->pVoipGroupRef, 'tunl', iValue, 0, NULL);
        }
        if (iValue2 > 0)
        {
            pConnApi->iTunnelPort = iValue2;
        }
        return(0);
    }
    if (iControl == 'type')
    {
        // set connection flags (CONNAPI_CONNFLAG_*)
        NetPrintf(("connapi: [%p] connflag change from 0x%02x to 0x%02x\n", pConnApi, pConnApi->uConnFlags, iValue));
        pConnApi->uConnFlags = (uint16_t)iValue;
        return(0);
    }
    if (iControl == 'ulmt')
    {
        // set gamelink unack window size
        pConnApi->iGameUnackLimit = iValue;
        return(0);
    }
    if (iControl == 'voig')
    {
        if (iValue == 'getr')   //$$todo remove 'getr' support in future release
        {
            return(ConnApiStatus(pConnApi, 'vgrp', pValue, sizeof(pConnApi->pVoipGroupRef)));
        }
        VoipGroupControl(pConnApi->pVoipGroupRef, iValue, iValue2, 0, pValue);
        return(0);
    }
    if (iControl == 'voip')
    {
        if(pConnApi->pVoipRef != NULL)
        {
            VoipControl(pConnApi->pVoipRef, iValue, iValue2, pValue);
            return(0);
        }

        NetPrintf(("connapi: [%p] - WARNING - ConnApiControl(): processing of 'voip' selector failed because of an uninitialized VOIP module reference!\n", pConnApi));
    }
    if (iControl == 'vprt')
    {
        NetPrintf(("connapi: [%p] using voip port %d\n", pConnApi, iValue));
        pConnApi->uVoipPort = (uint16_t)iValue;
        return(0);
    }
    if (iControl == 'vset')
    {
        uint8_t bVoipEnabled;
        // if disabling VoIP, set game flags appropriately
        if ((bVoipEnabled = iValue) == FALSE)
        {
            NetPrintf(("connapi: [%p] 'vset' used to globally disable voip\n", pConnApi));
            pConnApi->uConnFlags &= ~CONNAPI_CONNFLAG_VOIPCONN;
        }
        return(0);
    }
    #if DIRTYCODE_DEBUG
    if (iControl == '!res')
    {
        ConnApiClientT *pClient;
        if (iValue2 >= 0 && iValue2 < pConnApi->ClientList.iMaxClients)
        {
            pClient = &pConnApi->ClientList.Clients[iValue2];
            if (pClient->bAllocated == TRUE)
            {
                if (iValue > 0)
                {
                    NetPrintf(("connapi: [%p] setting debug FailP2P flag for user at index %d\n", pConnApi, iValue2));
                    pConnApi->ClientList.Clients[iValue2].uFlags |= CONNAPI_CLIENTFLAG_P2PFAILDBG;
                    if (iValue2 == pConnApi->iSelf)
                    {
                        NetPrintf(("connapi: [%p] setting debug FailP2P flag with our own index, disabling P2P for all users for all users\n", pConnApi));
                    }
                }
                else
                {
                    NetPrintf(("connapi: [%p] resetting debug FailP2P flag for user at index %d\n", pConnApi, iValue2));
                    pConnApi->ClientList.Clients[iValue2].uFlags &= ~CONNAPI_CLIENTFLAG_P2PFAILDBG;
                }
            }
            else
            {
                NetPrintf(("connapi: [%p] failed to set debug FailP2P flag for user at index %d\n", pConnApi, iValue2));
                return(-1);
            }
        }
        else if (iValue2 < 0)
        {
            pConnApi->bFailP2PConnect = iValue;
            NetPrintf(("connapi: [%p] setting debug FailP2P flag to %d for all users\n", pConnApi, iValue));
        }
        else
        {
            NetPrintf(("connapi: [%p] cannot set debug FailP2P flag to %d for user at index %d\n", pConnApi, iValue, iValue2));
        }

        return(0);
    }
    #endif
    if (iControl == 'sqos')
    {
        pConnApi->iQosDuration = iValue;
        NetPrintf(("connapi: [%p] total duration of QoS characterization over netgamelinks --> %d ms %s\n",
            pConnApi, pConnApi->iQosDuration, ((pConnApi->iQosDuration == 0) ? "(QoS over NetGameLink disabled)" : "")));

        pConnApi->iQosInterval = iValue2;
        NetPrintf(("connapi: [%p] send interval used for QoS characterization over netgamelinks --> %d ms\n", pConnApi, pConnApi->iQosInterval));

        return(0);
    }
    if (iControl == 'lqos')
    {
        NetPrintf(("connapi: [%p] packet size used for QoS characterization over netgamelinks --> %d bytes\n", pConnApi, iValue));
        pConnApi->iQosPacketSize = iValue;
        return(0);
    }
    if (iControl == 'estv')
    {
        ConnApiClientT *pClient;
        if (iValue < pConnApi->ClientList.iMaxClients)
        {
            pClient = &pConnApi->ClientList.Clients[iValue];
            if (pClient->bAllocated == TRUE)
            {
                // no-op if it is already established
                // in the case of the local user, he will always have bEstablishVoip == TRUE so let's not spam with extra logs about it
                if (pClient->bEstablishVoip == FALSE)
                {
                    NetPrintf(("connapi: [%p] activating delayed voip for client %d\n", pConnApi, iValue));
                    pClient->bEstablishVoip = TRUE;
                    return(0);
                }
            }
            else
            {
                NetPrintf(("connapi: [%p] activating delayed voip failed because client %d not allocated\n", pConnApi, iValue));
                return(-1);
            }
        }
        else
        {
            NetPrintf(("connapi: [%p] activating delayed voip failed because of client index of range\n", pConnApi));
            return(-1);
        }
    }

    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ConnApiUpdate

    \Description
        Update the ConnApi module (must be called directly if auto-update is disabled)

    \Input *pConnApi    - pointer to module state

    \Notes
        By default, ConnApiUpdate() is called internally via a NetConnIdle() callback
        (auto-update).  If auto-update is disabled via ConnApiControl('auto'),
        ConnApiUpdate must be polled by the application instead.

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
void ConnApiUpdate(ConnApiRefT *pConnApi)
{
    // update client flags
    _ConnApiUpdateClientFlags(pConnApi);

    // update connapi connections
    _ConnApiUpdateConnections(pConnApi);

    #if CONNAPI_XBOX_NETWORKING
    /*
       update protomangle outside the demangling phase context
       required for protomangle to be pumped after a call to ProtoMangleControl('remv')
    */
    if (pConnApi->bDemanglerEnabled == TRUE)
    {
        ProtoMangleUpdate(pConnApi->pProtoMangle);
    }
    #endif

    // handle removal of clients from client list, if requested
    _ConnApiUpdateRemoval(pConnApi);
}

/*F********************************************************************************/
/*!
    \Function ConnApiAddCallback

    \Description
        Register a new callback

    \Input *pConnApi    - pointer to module state
    \Input *pCallback   - the callback to add
    \Input *pUserData   - the user data that will be passed back

    \Output
        int32_t         - negative means error. 0 or greater: slot used

    \Version 09/18/2008 (jrainy)
*/
/********************************************************************************F*/
int32_t ConnApiAddCallback(ConnApiRefT *pConnApi, ConnApiCallbackT *pCallback, void *pUserData)
{
    int32_t iIndex;

    // skip the first (0th, which is reserved for 'cbfp' and 'cbup' backward compatibility.
    for(iIndex = 1; iIndex < CONNAPI_MAX_CALLBACKS; iIndex++)
    {
        if (pConnApi->pCallback[iIndex] == NULL)
        {
            pConnApi->pCallback[iIndex] = pCallback;
            pConnApi->pUserData[iIndex] = pUserData;

            return(iIndex);
        }
    }
    return(CONNAPI_CALLBACKS_FULL);
}

/*F********************************************************************************/
/*!
    \Function ConnApiRemoveCallback

    \Description
        Unregister a callback

    \Input *pConnApi    - pointer to module state
    \Input *pCallback   - the callback to remove
    \Input *pUserData   - the user data that was originally passed in

    \Output
        int32_t         - negative means error. 0 or greater: slot freed

    \Version 09/18/2008 (jrainy)
*/
/********************************************************************************F*/
int32_t ConnApiRemoveCallback(ConnApiRefT *pConnApi, ConnApiCallbackT *pCallback, void *pUserData)
{
    int32_t iIndex;

    // skip the first (0th, which is reserved for 'cbfp' and 'cbup' backward compatibility.
    for(iIndex = 1; iIndex < CONNAPI_MAX_CALLBACKS; iIndex++)
    {
        if ((pConnApi->pCallback[iIndex] == pCallback) && (pConnApi->pUserData[iIndex] == pUserData))
        {
            pConnApi->pCallback[iIndex] = NULL;
            pConnApi->pUserData[iIndex] = NULL;
            return(iIndex);
        }
    }
    return(CONNAPI_CALLBACK_NOT_FOUND);
}
