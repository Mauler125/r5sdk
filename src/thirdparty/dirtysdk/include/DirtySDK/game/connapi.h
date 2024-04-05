/*H********************************************************************************/
/*!
    \File connapi.h

    \Description
        ConnApi is a high-level connection manager, that packages the "connect to
        peer" process into a single module.  Both game connections and voice
        connections can be managed.  Multiple peers are supported in a host/client
        model for the game connection, and a peer/peer model for the voice
        connections.

    \Copyright
        Copyright (c) Electronic Arts 2005. ALL RIGHTS RESERVED.

    \Version 01/04/2005 (jbrookes) first version
*/
/********************************************************************************H*/

#ifndef _connapi_h
#define _connapi_h

/*!
\Moduledef ConnApi ConnApi
\Modulemember Game
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtyaddr.h"
#include "DirtySDK/game/netgameutil.h"
#include "DirtySDK/game/netgamelink.h"
#include "DirtySDK/game/netgamedist.h"
#include "DirtySDK/comm/commudp.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/prototunnel.h"

/*** Defines **********************************************************************/


// connection flags
#define CONNAPI_CONNFLAG_GAMECONN       (1)     //!< game connection supported
#define CONNAPI_CONNFLAG_VOIPCONN       (2)     //!< voip connection supported
#define CONNAPI_CONNFLAG_GAMEVOIP       (3)     //!< game and voip connections supported

// connection status flags
#define CONNAPI_CONNFLAG_CONNECTED      (4)     //!< set if connection succeeded
#define CONNAPI_CONNFLAG_DEMANGLED      (16)    //!< set if demangler was attempted
#define CONNAPI_CONNFLAG_PKTRECEIVED    (32)    //!< set if packets are received (for game connection only)

// error codes
#define CONNAPI_ERROR_INVALID_STATE     (-1)    //!< connapi is in an invalid state
#define CONNAPI_ERROR_CLIENTLIST_FULL   (-2)    //!< client list is full
#define CONNAPI_ERROR_SLOT_USED         (-3)    //!< selected slot already in use
#define CONNAPI_ERROR_SLOT_OUT_OF_RANGE (-4)    //!< selected slot is not an valid index into the client array
#define CONNAPI_CALLBACKS_FULL          (-5)    //!< maximum number of callbacks registered
#define CONNAPI_CALLBACK_NOT_FOUND      (-6)    //!< callback not found

// supported connection concierge mode  (to be used with 'ccmd' control selector)
#define CONNAPI_CCMODE_PEERONLY         (0)     //!< peer connections only
#define CONNAPI_CCMODE_HOSTEDONLY       (1)     //!< ccs connections only
#define CONNAPI_CCMODE_HOSTEDFALLBACK   (2)     //!< peer connections fallback to ccs on failure

//! ConnApi callback types
typedef enum ConnApiCbTypeE
{
    CONNAPI_CBTYPE_GAMEEVENT,                   //!< a game event occurred
    CONNAPI_CBTYPE_DESTEVENT,                   //!< link/util ref destruction event
    CONNAPI_CBTYPE_VOIPEVENT,                   //!< a voip event occurred
    CONNAPI_NUMCBTYPES                          //!< number of callback types
} ConnApiCbTypeE;

//! connection status
typedef enum ConnApiConnStatusE
{
    CONNAPI_STATUS_INIT,                        //!< initialization state
    CONNAPI_STATUS_CONN,                        //!< connecting to peer
    CONNAPI_STATUS_MNGL,                        //!< demangling
    CONNAPI_STATUS_ACTV,                        //!< connection established
    CONNAPI_STATUS_DISC,                        //!< disconnected
    CONNAPI_NUMSTATUSTYPES                      //!< max number of status types
} ConnApiConnStatusE;

//! game topology types
typedef enum ConnApiGameTopologyE
{
    CONNAPI_GAMETOPOLOGY_DISABLED,              //!< no game traffic
    CONNAPI_GAMETOPOLOGY_PEERWEB,               //!< peer to peer full mesh
    CONNAPI_GAMETOPOLOGY_PEERHOSTED,            //!< hosted by peer
    CONNAPI_GAMETOPOLOGY_SERVERHOSTED           //!< hosted by server
} ConnApiGameTopologyE;

//! voip topology types
typedef enum ConnApiVoipTopologyE
{
    CONNAPI_VOIPTOPOLOGY_DISABLED,              //!< no voip traffic
    CONNAPI_VOIPTOPOLOGY_PEERWEB,               //!< peer to peer full mesh
    CONNAPI_VOIPTOPOLOGY_SERVERHOSTED           //!< peer to peer routed via server
} ConnApiVoipTopologyE;

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/
//! connection timers
typedef struct ConnApiConnTimersT
{
    uint32_t            uCreateSATime;          //!< time it takes to resolve secure association (xbox one)
    uint32_t            uConnectTime;           //!< time it takes for intial connection attempt 
    uint32_t            uDemangleTime;          //!< time it takes to attempt demangling on the connection
    uint32_t            uDemangleConnectTime;   //!< time it takes to attempt a connection after demangling
} ConnApiConnTimersT;

//! connection info
typedef struct ConnApiConnInfoT
{
    uint16_t            uLocalPort;             //!< local (bind) port
    uint16_t            uMnglPort;              //!< demangled port, if any
    uint8_t             bDemangling;            //!< is demangling process ongoing? (internal use only - see notes in connapi.c file header)
    uint8_t             uConnFlags;             //!< connection status flags (CONNAPI_CONNSTAT_*)
    uint8_t             _pad[2];
    ConnApiConnStatusE  eStatus;                //!< status of connection (CONNAPI_STATUS_*)
    uint32_t            iConnStart;             //!< NetTick() recorded at connection or demangling start (internal use only)
    ConnApiConnTimersT  ConnTimers;             //!< connection timers 
} ConnApiConnInfoT;

//! client info
typedef struct ConnApiClientInfoT
{
    uint32_t            uId;                    //!< unique connapi client id, uId should never be 0. A client with uId of 0 will be consider invalid.
    uint32_t            uAddr;                  //!< external internet address of user
    uint8_t             bIsConnectivityHosted;  //!< flag for whether the connectivity to this client is direct or server-hosted
    uint32_t            uRemoteClientId;        //!< remote client id (used when setting up tunnel, game connection and voip connection with this client)
    uint32_t            uLocalClientId;         //!< local client id (used when setting up tunnel, game connection and voip connection with this client)
    uint32_t            uHostingServerId;       //!< id of the hosting server or 0 if none
    uint32_t            uLocalAddr;             //!< internal address of user
    uint16_t            uGamePort;              //!< external (send) port to use for game connection, or zero to use global port
    uint16_t            uVoipPort;              //!< external (send) port to use for voip connection, or zero to use global port
    uint16_t            uLocalGamePort;         //!< local (bind) port to use for game connection, or zero to use global port
    uint16_t            uLocalVoipPort;         //!< local (bind) port to use for voip connection, or zero to use global port
    uint16_t            uTunnelPort;            //!< user's tunnel port, or zero to use default
    uint16_t            uLocalTunnelPort;       //!< user's local tunnel port
    uint8_t             bEnableQos;             //!< enable QoS for this client, call ConnApiControl() with 'sqos' and 'lqos' to configure QoS settings
    uint8_t             _pad[3];
    DirtyAddrT          DirtyAddr;              //!< dirtyaddr address of client
    char                strTunnelKey[PROTOTUNNEL_MAXKEYLEN]; //!< tunnel key
} ConnApiClientInfoT;

//! connection type
typedef struct ConnApiClientT
{
    ConnApiClientInfoT  ClientInfo;
    ConnApiConnInfoT    GameInfo;               //!< info about game connection
    ConnApiConnInfoT    VoipInfo;               //!< info about voip connection
    NetGameUtilRefT     *pGameUtilRef;          //!< util ref for connection
    NetGameLinkRefT     *pGameLinkRef;          //!< link ref for connection
    NetGameDistRefT     *pGameDistRef;          //!< dist ref for connection (for app use; not managed by ConnApi)
    int32_t             iTunnelId;              //!< tunnel identifier (if any)
    uint16_t            uConnFlags;             //!< CONNAPI_CONNFLAG_* describing the connection type (read-only)
    int16_t             iVoipConnId;            //!< voip connection identifier
    uint16_t            uFlags;                 //!< internal client flags
    uint8_t             bAllocated;             //!< TRUE if this slot is allocated, else FALSE
    uint8_t             bEstablishVoip;         //!< used to establish voip when enable QoS is set (after QoS has been validated), call ConnApiControl() with 'estv' to configure
} ConnApiClientT;

//! connection list type
typedef struct ConnApiClientListT
{
    int32_t             iNumClients;            //!< number of clients in list
    int32_t             iMaxClients;            //!< max number of clients
    ConnApiClientT      Clients[1];             //!< client array (variable length)
} ConnApiClientListT;

//! callback info
typedef struct ConnApiCbInfoT
{
    int32_t             iClientIndex;           //!< index of client event is for
    uint32_t            eType;                  //!< type of event (CONNAPI_CBTYPE_*)
    uint32_t            eOldStatus;             //!< old status (CONNAPI_STATUS_*)
    uint32_t            eNewStatus;             //!< new status (CONNAPI_STATUS_*)
    const ConnApiClientT* pClient;              //!< pointer to the corresponding client structure
} ConnApiCbInfoT;

//! opaque module ref
typedef struct ConnApiRefT ConnApiRefT;

/*!
    \Callback ConnApiCallbackT

    \Description
        Callback fired when a connection event happens identified by the information
        in pCbInfo

    \Input *pConnApi    - module state
    \Input *pCbInfo     - callback information
    \Input *pUserData   - user information passed along with the callback
*/
typedef void (ConnApiCallbackT)(ConnApiRefT *pConnApi, ConnApiCbInfoT *pCbInfo, void *pUserData);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

//! create the module state
#define ConnApiCreate(_iGamePort, _iMaxClients, _pCallback, _pUserData) ConnApiCreate2(_iGamePort, _iMaxClients, _pCallback, _pUserData, (CommAllConstructT *)CommUDPConstruct)

// create the module state
DIRTYCODE_API ConnApiRefT *ConnApiCreate2(int32_t iGamePort, int32_t iMaxClients, ConnApiCallbackT *pCallback, void *pUserData, CommAllConstructT *pConstruct);

// this function should be called once the user has logged on and the input parameters are available
DIRTYCODE_API void ConnApiOnline(ConnApiRefT *pConnApi, const char *pGameName, uint32_t uSelfId, ConnApiGameTopologyE eGameTopology, ConnApiVoipTopologyE eVoipTopology);

// destroy the module state
DIRTYCODE_API void ConnApiDestroy(ConnApiRefT *pConnApi);

// host or connect to a game / voip, with the possibility to import connection.
DIRTYCODE_API void ConnApiConnect(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientList, int32_t iClientListSize, int32_t iGameHostIndex, int32_t iVoipHostIndex, int32_t iSessId);

// add a new client to a pre-existing game in the specified index.
DIRTYCODE_API int32_t ConnApiAddClient(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientInfo, int32_t iClientIdx);

// return the ConnApiClientT for the specified client (by id)
DIRTYCODE_API uint8_t ConnApiFindClient(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientInfo, ConnApiClientT *pOutClient);

// remove a current client from a game
DIRTYCODE_API void ConnApiRemoveClient(ConnApiRefT *pConnApi, int32_t iClientIdx);

// redo all connections, using the host specified
DIRTYCODE_API void ConnApiMigrateGameHost(ConnApiRefT *pConnApi, int32_t iNewGameHostIndex);

// disconnect from game
DIRTYCODE_API void ConnApiDisconnect(ConnApiRefT *pConnApi);

// get list of current connections
DIRTYCODE_API const ConnApiClientListT *ConnApiGetClientList(ConnApiRefT *pConnApi);

// connapi status
DIRTYCODE_API int32_t ConnApiStatus(ConnApiRefT *pConnApi, int32_t iSelect, void *pBuf, int32_t iBufSize);

// connapi status (version 2)
DIRTYCODE_API int32_t ConnApiStatus2(ConnApiRefT *pConnApi, int32_t iSelect, void *pData, void *pBuf, int32_t iBufSize);

// connapi control
DIRTYCODE_API int32_t ConnApiControl(ConnApiRefT *pConnApi, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// update connapi module (must be called directly if auto-update is disabled)
DIRTYCODE_API void ConnApiUpdate(ConnApiRefT *pConnApi);

// Register a new callback
DIRTYCODE_API int32_t ConnApiAddCallback(ConnApiRefT *pConnApi, ConnApiCallbackT *pCallback, void *pUserData);

// Removes a callback previously registered
DIRTYCODE_API int32_t ConnApiRemoveCallback(ConnApiRefT *pConnApi, ConnApiCallbackT *pCallback, void *pUserData);

#ifdef __cplusplus
};
#endif

//@}

#endif // _connapi_h

