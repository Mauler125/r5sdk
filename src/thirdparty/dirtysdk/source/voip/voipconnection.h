/*H********************************************************************************/
/*!
    \File voipconnection.h

    \Description
        VoIP virtual connection manager.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 03/17/2004 (jbrookes) First Version
    \Version 1.1 12/10/2008 (mclouatre) Added ref count to VOIP connections.
    \Version 1.2 01/06/2009 (mclouatre) Added field RemoteAddrFallbacks to VoipConnectionT structure. Create related functions
    \Version 1.3 10/26/2009 (mclouatre) Renamed from xbox/voipconnection.h to xenon/voipconnectionxenon.h
*/
/********************************************************************************H*/

#ifndef _voipconnection_h
#define _voipconnection_h

/*** Include files ****************************************************************/

#include "voippacket.h"

/*** Defines **********************************************************************/
//! maximum number of session IDs per VOIP connection
#define VOIP_MAXSESSIONIDS  (8)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! voice receive callback
typedef void (VoipConnectRecvVoiceCbT)(VoipUserT *pRemoteUsers, int32_t iRemoteUserSize, int32_t iConnId, VoipMicrInfoT *pMicrInfo, uint8_t *pPacketData, void *pUserData);

//! transcribed text recv callback
typedef void (VoipConnectRecvOpaqueCbT)(int32_t iConnId, const uint8_t *pOpaqueData, int32_t iOpaqueDataSize, void *pUserData);

//! transcribed text recv callback
typedef void (VoipConnectRecvTextCbT)(int32_t iConnId, int32_t iRemoteUserIndex, const char *pStrUtf8, void *pUserData);

//! new remote user register callback
typedef void (VoipConnectRegUserCbT)(VoipUserT *pRemoteUser, int32_t iConnId, uint32_t bRegister, void *pUserData);

typedef struct SendAddrFallbackT
{
    uint32_t        SendAddr;           //!< send address
    uint8_t         bReachable;         //!< FALSE if voip detected the IP addr as being unreachable
} SendAddrFallbackT;

//! type used to create linked list of reliable data entries
typedef struct LinkedReliableDataT
{
    struct LinkedReliableDataT *pNext;  //<! pointer to next entry
    uint32_t uEnqueueTick;              //<! time at which entry was enqueue
    ReliableDataT data;                 //<! reliable data buffe
} LinkedReliableDataT;

//! connection structure
typedef struct VoipConnectionT
{
    struct sockaddr_in  SendAddr;       //!< address we send to
    uint32_t        uSessionId[VOIP_MAXSESSIONIDS]; //!< list of IDs representing the set of sessions sharing this connection
    int32_t         iMaxRemoteUsers;    //!< max number users (without shared slot)
    int32_t         iMaxRemoteUsersExt; //!< max number of users (with shared slot)
    VoipUserT       RemoteUsers[32]; //!< remote user(s) we're connecting/connected to
    uint32_t        uLocalClientId;     //!< local client id associated with the high-level connection (only valid when bIsLocalClientIdValid is TRUE)
    uint32_t        bIsLocalClientIdValid; //!< true if uLocalClientID is valid

    uint32_t        uRemoteClientId;    //!< clientId of remote user
    uint32_t        uRemoteUserStatus[VOIP_MAXLOCALUSERS];      //!< VOIP_REMOTE_*
    uint32_t        uRemoteConnStatus;  //!< VOIP_CONN_*
    int32_t         iVoipServerConnId;  //!< conn id used by the voip server to identify this connection, contains VOIP_CONNID_NONE if unknown by voip server
    uint32_t        uUserLastRecv[VOIP_MAXLOCALUSERS]; 

    enum
    {
        ST_DISC,
        ST_CONN,
        ST_ACTV
    } eState;                           //!< connection status

    LinkedReliableDataT *pOutboundReliableDataQueue;                //!< linked list of outbound reliable data entries

    VoipMicrPacketT VoipMicrPacket[VOIP_MAXLOCALUSERS_EXTENDED];    //!< buffered voip packet (for sending), one per local user

    uint32_t        aVoiceSendTimer[VOIP_MAXLOCALUSERS_EXTENDED];   //!< timer used to rate limit voice packets to ten per second
    uint32_t        aRecvChannels[VOIP_MAXLOCALUSERS_EXTENDED];     //!< id of the channels to which remote users on this connection are subscribed to
    uint32_t        aLastRecvChannels[VOIP_MAXLOCALUSERS_EXTENDED]; //!< last value of aRecvChannels

    uint32_t        uLastSend;          //!< last time data was sent
    uint32_t        uLastRecv;          //!< last time data was received
    uint32_t        uSendSeqn;          //!< packet send sequence
    uint32_t        uRecvSeqn;          //!< sequence number of last received voice packet

    int32_t         iChangePort;        //!< number of port changes
    int32_t         iChangeAddr;        //!< number of address changes
    int32_t         iRecvPackets;       //!< number of valid packets received
    int32_t         iRecvData;          //!< total amount of data received
    int32_t         iSendResult;        //!< previous socket send result
    int32_t         iMaxSubPktSize;     //!< max outbound sub-pkt size encountered during the lifetime of this connection

    //! variables used to implement reliable DATA flow
    uint8_t         uOutSeqNb;          //!< seq nb to be assigned to next outbound reliable data entry on this connection. validity range: [1,127].
    uint8_t         uInSeqNb;           //!< last successfully received seq nb from peer. validity range: [1,127]. 0 means no inbound traffic received yet
    uint8_t         bPeerNeedsAck;      //!< TRUE if peer still expects acked seq number in packets
    uint8_t         bPeerIsAcking;      //!< TRUE if peer is still acking us

    //! misc
    uint8_t         bMuted;             //!< true if muted, else false
    uint8_t         bConnPktRecv;       //!< true if Conn packet received from other party, else false
    uint8_t         bAutoMuted;         //!< true if auto muting has already been performed we should only apply auto mute once per connection (only used on Xbox One)
    uint8_t         bTranscribedTextRequested; //!< true if at least one remote user as requested transcribed text
} VoipConnectionT;

//! connection list
typedef struct VoipConnectionlistT
{
    SocketT         *pSocket;           //!< master socket, used to send/recv all data
    NetCritT        NetCrit;            //!< critical section

    VoipConnectRegUserCbT   *pRegUserCb;//!< callback to call when a new user is registered
    void            *pRegUserUserData;  //!< user data to be sent to callback
    VoipConnectRecvVoiceCbT *pVoiceCb;  //!< callback to call when voice data is received
    void            *pVoiceUserData;    //!< user data to be sent to callback
    VoipConnectRecvTextCbT  *pTextCb;   //!< callback to call when transcribed text is received
    void            *pTextUserData;     //!< user data to be sent to callback
    VoipConnectRecvOpaqueCbT *pOpaqueCb;//!< callback to call when opaque data is received
    void            *pOpaqueUserData;   //!< user data to be sent to callback

    int32_t         iMaxConnections;    //!< maximum number of connections
    VoipConnectionT *pConnections;      //!< connection array

    //! memory group used to build the pre-allocated pFreeReliableDataPool
    int32_t         iMemGroup;          //!< module memgroup id
    void            *pMemGroupUserData; //!< user data associated with memgroup

    LinkedReliableDataT *pFreeReliableDataPool; //!< linked list of free pre-allocated reliable data buffers

    uint32_t        uUserSendMask;      //!< application-specified send mask
    uint32_t        uSendMask;          //!< bitmask of who we are sending voice data to
    uint32_t        uUserRecvMask;      //!< application-specified recv mask
    uint32_t        uRecvMask;          //!< bitmask of who we are accepting voice data from
    uint32_t        uRecvVoice;         //!< bitmask of who we are currently receiving voice from
    uint32_t        uLocalUserStatus[VOIP_MAXLOCALUSERS];    //!< current local status (VOIP_LOCAL_*)
    uint32_t        uLocalUserLastSend[VOIP_MAXLOCALUSERS];  //!< current local user send time
    uint32_t        uFriendsMask;       //!< Computed mask specifying which channels are to friends

    uint32_t        uLastVoiceTime[VOIP_MAXLOCALUSERS];     //!< used to time out VOIP_LOCAL_USER_TALKING status

    VoipUserT       LocalUsers[VOIP_MAXLOCALUSERS_EXTENDED]; //!< Local Users

    uint8_t         aIsParticipating[VOIP_MAXLOCALUSERS_EXTENDED]; //!< TRUE if user was moved to "participating" state with VoipCommonActivateLocalUser(); FALSE otherwise

    uint32_t        aChannels[VOIP_MAXLOCALUSERS_EXTENDED]; //!< ids of the voip channels that local users are subscribed to

    uint8_t         bTranscribedTextRequested[VOIP_MAXLOCALUSERS]; //!< TRUE if user wants remote peer to send transcribed text.

    uint32_t        uClientId;          //!< clientId sent in every packet

    uint32_t        uBindPort;          //!< port socket is bound to for receive

    int32_t         iDataTimeout;       //!< connection data timeout in milliseconds

    uint8_t         bSentVoiceData;     //!< current voice data has been sent (used in server mode)
    uint8_t         bUpdateMute;        //!< TRUE if the mute list needs to be updated, else FALSE
    int8_t          iFriendConnId;
    #if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
    int8_t          bApplyRelFromMuting; //!< TRUE if comm relationship needs to be reapplied because of a change in muting
    #else
    uint8_t         _pad;
    #endif
} VoipConnectionlistT;


/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// init a connectionlist
int32_t  VoipConnectionStartup(VoipConnectionlistT *pConnectionlist, int32_t iMaxPeers);

// set callbacks
void VoipConnectionSetCallbacks(VoipConnectionlistT *pConnectionlist, VoipConnectRecvVoiceCbT *pVoiceCb, void *pVoiceUserData,
                                                                      VoipConnectRecvTextCbT *pTextCb, void *pTextUserData,
                                                                      VoipConnectRegUserCbT *pRegUserCb, void *pRegUserUserData,
                                                                      VoipConnectRecvOpaqueCbT *pOpaqueCb, void *pOpaqueUserData);

// set text callbacks (this is needed on x1 crossplay)
void VoipConnectionSetTextCallback(VoipConnectionlistT *pConnectionlist, VoipConnectRecvTextCbT *pTextCb, void *pTextUserData);

// close a connectionlist
void VoipConnectionShutdown(VoipConnectionlistT *pConnectionlist);

// Check whether a given Connection ID can be allocated in this Connection List
uint8_t  VoipConnectionCanAllocate(VoipConnectionlistT *pConnectionlist, int32_t iConnID);

// start a virtual connection with peer
int32_t  VoipConnectionStart(VoipConnectionlistT *pConnectionlist, int32_t iConnID, uint32_t uAddr, uint32_t uConnPort, uint32_t uBindPort, uint32_t uClientId, uint32_t uSessionId);

// update virtual connections
void VoipConnectionUpdate(VoipConnectionlistT *pConnectionlist);

// stop a virtual connection
void VoipConnectionStop(VoipConnectionlistT *pConnectionlist, int32_t iConnID, int32_t bSendDiscMsg);

// flush any currently queued voice data on given connection (currently only does anything on Xenon)
int32_t VoipConnectionFlush(VoipConnectionlistT *pConnectionlist, int32_t iConnID);

// send voice data on connection(s)
void VoipConnectionSend(VoipConnectionlistT *pConnectionlist, uint32_t uSendMask, const uint8_t *pVoiceData, int32_t iDataSize, const uint8_t *pMetaData, int32_t iMetaDataSize, uint32_t uLocalIndex, uint8_t uSendSeqn);

// set connections to send to
void VoipConnectionSetSendMask(VoipConnectionlistT *pConnectionlist, uint32_t uConnMask);

// set connections to receive from
void VoipConnectionSetRecvMask(VoipConnectionlistT *pConnectionlist, uint32_t uRecvMask);

// register/unregister remote talkers associated with the given connectionID
void VoipConnectionRegisterRemoteTalkers(VoipConnectionlistT *pConnectionlist, int32_t iConnID, uint32_t bRegister);

// add a session ID to the set of sessions sharing the voip connection
int32_t VoipConnectionAddSessionId(VoipConnectionlistT *pConnectionlist, int32_t iConnId, uint32_t uSessionId);

// delete a session ID to the set of sessions sharing the voip connection
int32_t VoipConnectionDeleteSessionId(VoipConnectionlistT *pConnectionlist, int32_t iConnId, uint32_t uSessionId);

// broadcast local user join-in-progress data that needs to be sent reliably on all active connections
void VoipConnectionReliableBroadcastUser(VoipConnectionlistT *pConnectionlist, uint8_t uLocalUserIndex, uint32_t bParticipating);

// reliably broadcast a transcribed text message (originated from a local user) on all connections that requested for it.
void VoipConnectionReliableTranscribedTextMessage(VoipConnectionlistT *pConnectionlist, uint8_t uLocalUserIndex, const char *pStrUtf8);

// send opaque data reliably over specified connection
void VoipConnectionReliableSendOpaque(VoipConnectionlistT *pConnectionlist, int32_t iConnID, const uint8_t *pData, uint16_t uDataSize);

#ifdef __cplusplus
};
#endif

#endif // _voipconnection_h

