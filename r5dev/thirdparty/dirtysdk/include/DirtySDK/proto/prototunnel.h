/*H********************************************************************************/
/*!
    \File prototunnel.h

    \Description
        ProtoTunnel creates and manages a Virtual DirtySock Tunnel (VDST)
        connection.  The tunnel transparently bundles data sent from multiple ports
        to a specific remote host into a single send, and optionally encrypts
        portions of that packet.  Only data sent over a UDP socket may be
        tunneled.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 12/02/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _prototunnel_h
#define _prototunnel_h

/*!
\Moduledef ProtoTunnel ProtoTunnel
\Modulemember Proto
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

// prototunnel protocol versions (maj.min)
#define PROTOTUNNEL_VERSION_1_1         (0x0101)
#define PROTOTUNNEL_VERSION             (PROTOTUNNEL_VERSION_1_1)      //!< default protocol version
#define PROTOTUNNEL_VERSION_MIN         (PROTOTUNNEL_VERSION_1_1)      //!< minimum protocol version 
#define PROTOTUNNEL_VERSION_MAX         (PROTOTUNNEL_VERSION_1_1)      //!< maximum protocol version

//! maximum number of ports that can be mapped to a tunnel
#define PROTOTUNNEL_MAXPORTS            (8)

//! port flag indicating data is encrypted
#define PROTOTUNNEL_PORTFLAG_ENCRYPTED  (1)
#define PROTOTUNNEL_PORTFLAG_AUTOFLUSH  (2)

//! max key length
#define PROTOTUNNEL_MAXKEYLEN           (128)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! tunnel event types
typedef enum ProtoTunnelEventE
{
    PROTOTUNNEL_EVENT_RECVNOMATCH,  //!< data received from source with no tunnel mapping

    PROTOTUNNEL_NUMEVENTS           //!< total number of events
} ProtoTunnelEventE;

//! map description
typedef struct ProtoTunnelInfoT
{
    uint32_t uRemoteClientId;                           //!< unique client ID of remote client
    uint32_t uRemoteAddr;                               //!< remote tunnel address
    uint16_t uRemotePort;                               //!< remote tunnel port (if zero, use local port)
    uint16_t aRemotePortList[PROTOTUNNEL_MAXPORTS];     //!< port(s) to map
    uint8_t  aPortFlags[PROTOTUNNEL_MAXPORTS];          //!< port flags
    uint16_t uTunnelVers;                               //!< tunnel protocol version
    uint8_t  _pad[2];
} ProtoTunnelInfoT;

//! tunnel statistics
typedef struct ProtoTunnelStatT
{
    uint32_t uLastPacketTime;           //!< last time packet was snet or received
    uint32_t uUpdateTime;               //!< time when ProtoTunnelStatT was updated
    uint32_t uPrevUpdateTime;           //!< last time ProtoTunnelStatT was updated (can be used to keep track of update interval)
    uint32_t uEfficiency;               //!< send/receive efficiency for the last update interval
    uint32_t uRawBytesPerSecond;        //!< raw byte per second for the last update interval
    uint32_t uBytePerSecond;            //!< data bytes (user data) per second for the last update interval 
    uint32_t uNumBytes;                 //!< total number of bytes
    uint32_t uNumSubpacketBytes;        //!< total number of subpacket bytes (user data)
    uint16_t uNumPackets;               //!< total number of packets
    uint16_t uNumSubpackets;            //!< total number of subpackets
    uint16_t uNumDiscards;              //!< total number of out-of-order packet discards
    uint16_t _pad;
} ProtoTunnelStatT;

//! opaque module ref
typedef struct ProtoTunnelRefT ProtoTunnelRefT;

//! prototunnel event callback
typedef void (ProtoTunnelCallbackT)(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelEventE eEvent, const char *pData, int32_t iDataSize, struct sockaddr *pRecvAddr, void *pUserData);

//! raw inbound data receive callback (shall return 1 if it swallows the data, 0 otherwise)
typedef int32_t (RawRecvCallbackT)(SocketT *pSocket, uint8_t *pData, int32_t iRecvLen, const struct sockaddr *pFrom, int32_t iFromLen, void *pUserData);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module
DIRTYCODE_API ProtoTunnelRefT *ProtoTunnelCreate(int32_t iMaxTunnels, int32_t iTunnelPort);

// destroy the module
DIRTYCODE_API void ProtoTunnelDestroy(ProtoTunnelRefT *pProtoTunnel);

// set event callback
DIRTYCODE_API void ProtoTunnelCallback(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelCallbackT *pCallback, void *pUserData);

// allocate a tunnel
DIRTYCODE_API int32_t ProtoTunnelAlloc(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelInfoT *pInfo, const char *pKey);

// free a tunnel
DIRTYCODE_API uint32_t ProtoTunnelFree(ProtoTunnelRefT *pProtoTunnel, uint32_t uTunnelId, const char *pKey);

// free a tunnel
DIRTYCODE_API uint32_t ProtoTunnelFree2(ProtoTunnelRefT *pProtoTunnel, uint32_t uTunnelId, const char *pKey, uint32_t uAddr);

// update port mapping for specified tunnel
DIRTYCODE_API int32_t ProtoTunnelUpdatePortList(ProtoTunnelRefT *pProtoTunnel, uint32_t uTunnelId, ProtoTunnelInfoT *pInfo);

// get module status
DIRTYCODE_API int32_t ProtoTunnelStatus(ProtoTunnelRefT *pProtoTunnel, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);

// control the module
DIRTYCODE_API int32_t ProtoTunnelControl(ProtoTunnelRefT *pProtoTunnel, int32_t iControl, int32_t iValue, int32_t iValue2, const void *pValue);

// update the module
DIRTYCODE_API void ProtoTunnelUpdate(ProtoTunnelRefT *pProtoTunnel);

// use this function to send raw data to a remote host from the prototunnel socket
DIRTYCODE_API int32_t ProtoTunnelRawSendto(ProtoTunnelRefT *pProtoTunnel, const char *pBuf, int32_t iLen, const struct sockaddr *pTo, int32_t iToLen);

#ifdef __cplusplus
}
#endif

//@}

#endif // _prototunnel_h

