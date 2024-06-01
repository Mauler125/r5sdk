/*H*************************************************************************************/
/*!
    \File    dirtynetpriv.h

    \Description
        Private include for platform independent interface to network layers.

    \Copyright
        Copyright (c) Electronic Arts 2002-2014

    \Version 1.0 08/07/2014 (jbrookes) First version, split from dirtynet.h
*/
/*************************************************************************************H*/

#ifndef _dirtynetpriv_h
#define _dirtynetpriv_h

/*!
\Moduledef DirtyNetPriv DirtyNetPriv
\Modulemember DirtySock
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtynet.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! socket hostname cache
typedef struct SocketHostnameCacheT SocketHostnameCacheT;

//! socket packet queue
typedef struct SocketPacketQueueT SocketPacketQueueT;

//! socket packet queue entry
typedef struct SocketPacketQueueEntryT
{
    int32_t iPacketSize;                        //!< packet size
    struct sockaddr PacketAddr;                 //!< packet source
    uint32_t uPacketTick;                       //!< tick packet was added to the queue
    uint8_t aPacketData[SOCKET_MAXUDPRECV];     //!< packet data
} SocketPacketQueueEntryT;

//! socket rate estimation
typedef struct SocketRateT
{
    uint32_t uMaxRate;          //!< maximum transfer rate in bytes/sec (zero=no limit), minimum 1460 bytes/sec
    uint32_t uCurRate;          //!< current transfer rate in bytes/sec
    uint32_t uNextRate;         //!< estimated rate at next update
    uint32_t uLastTick;         //!< last update tick
    uint32_t uLastRateTick;     //!< tick count at last rate update
    uint32_t aTickHist[16];     //!< tick history (when update was recorded)
    uint32_t aDataHist[16];     //!< data history (how much data was sent during update)
    uint8_t  aCallHist[16];     //!< call history (how many times we were updated))
    uint8_t  uDataIndex;        //!< current update index
    uint8_t  _pad[3];
} SocketRateT;

// socket address map entry (private)
typedef struct SocketAddrMapEntryT SocketAddrMapEntryT;

//! socket address map
typedef struct SocketAddrMapT
{
    int32_t iNumEntries;
    int32_t iNextVirtAddr;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    SocketAddrMapEntryT *pMapEntries; //!< variable-length array
} SocketAddrMapT;


/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*
    HostName Cache functions
*/

// create hostname cache
SocketHostnameCacheT *SocketHostnameCacheCreate(int32_t iMemGroup, void *pMemGroupUserData);

// destroy hostname cache
void SocketHostnameCacheDestroy(SocketHostnameCacheT *pCache);

// add entry to hostname cache
void SocketHostnameCacheAdd(SocketHostnameCacheT *pCache, const char *pStrHost, uint32_t uAddress, int32_t iVerbose);

// get entry from hostname cache
uint32_t SocketHostnameCacheGet(SocketHostnameCacheT *pCache, const char *pStrHost, int32_t iVerbose);

// del entry from hostname cache
void SocketHostnameCacheDel(SocketHostnameCacheT *pCache, const char *pStrHost, uint32_t uAddress, int32_t iVerbose);

// process cache entries to delete expired
void SocketHostnameCacheProcess(SocketHostnameCacheT *pCache, int32_t iVerbose);

// check for refcounted (in-progress) hostname lookup then addref, or force using a new entry if bUseRef=false
HostentT *SocketHostnameAddRef(HostentT **ppHostList, HostentT *pHost, uint8_t bUseRef);

// process hostname list, delete completed lookups
void SocketHostnameListProcess(HostentT **ppHostList, int32_t iMemGroup, void *pMemGroupUserData);

/*
    Packet Queue functions
*/

// create packet queue
SocketPacketQueueT *SocketPacketQueueCreate(int32_t iMaxPackets, int32_t iMemGroup, void *pMemGroupUserData);

// destroy packet queue
void SocketPacketQueueDestroy(SocketPacketQueueT *pPacketQueue);

// resize a packet queue
SocketPacketQueueT *SocketPacketQueueResize(SocketPacketQueueT *pPacketQueue, int32_t iMaxPackets, int32_t iMemGroup, void *pMemGroupUserData);

// packet queue control function
int32_t SocketPacketQueueControl(SocketPacketQueueT *pPacketQueue, int32_t iControl, int32_t iValue);

// packet queue status function
int32_t SocketPacketQueueStatus(SocketPacketQueueT *pPacketQueue, int32_t iStatus);

// add to packet queue
int32_t SocketPacketQueueAdd(SocketPacketQueueT *pPacketQueue, const uint8_t *pPacketData, int32_t iPacketSize, struct sockaddr *pPacketAddr);
int32_t SocketPacketQueueAdd2(SocketPacketQueueT *pPacketQueue, const uint8_t *pPacketData, int32_t iPacketSize, struct sockaddr *pPacketAddr, uint32_t bPartialAllowed);

// alloc packet queue entry
SocketPacketQueueEntryT *SocketPacketQueueAlloc(SocketPacketQueueT *pPacketQueue);

// undo previous call to SocketPacketQueueAlloc()
void SocketPacketQueueAllocUndo(SocketPacketQueueT *pPacketQueue);

// remove from packet queue
int32_t SocketPacketQueueRem(SocketPacketQueueT *pPacketQueue, uint8_t *pPacketData, int32_t iPacketSize, struct sockaddr *pPacketAddr);

// remove stream packet from packet queue
int32_t SocketPacketQueueRemStream(SocketPacketQueueT *pPacketQueue, uint8_t *pPacketData, int32_t iPacketSize);

// get pointers to data associated with queue head
int32_t SocketPacketQueueGetHead(SocketPacketQueueT *pPacketQueue, uint8_t **ppPacketData, int32_t *pPacketSize, struct sockaddr **ppPacketAddr);

// update queue's head entry such that it no longer includes successfully consumed portion of data
int32_t SocketPacketQueueTouchHead(SocketPacketQueueT *pPacketQueue, int32_t iConsumedSize);

/*
    Rate functions
*/

// update socket data rate calculations
void SocketRateUpdate(SocketRateT *pRate, int32_t iData, const char *pOpName);

// throttle based on socket data rate calcuations and configured max rate
int32_t SocketRateThrottle(SocketRateT *pRate, int32_t iSockType, int32_t iData, const char *pOpName);

/*
    Send Callback functions
*/

// register a new socket send callback
int32_t SocketSendCallbackAdd(SocketSendCallbackEntryT aCbList[], SocketSendCallbackEntryT *pCbEntry);

// removes a socket send callback previously registered
int32_t SocketSendCallbackRem(SocketSendCallbackEntryT aCbList[], SocketSendCallbackEntryT *pCbEntry);

/*

   AddrMap functions

*/

// initialize a socket address map
void SocketAddrMapInit(SocketAddrMapT *pAddrMap, int32_t iMemGroup, void *pMemGroupUserData);

// cleanup a socket address map
void SocketAddrMapShutdown(SocketAddrMapT *pAddrMap);

// check if source address is in address map, and if so return mapped address
struct sockaddr *SocketAddrMapGet(const SocketAddrMapT *pAddrMap, struct sockaddr *pResult, const struct sockaddr *pSource, int32_t *pNameLen);

// translate between IPv4, vIPv4, and IPv6 addresses
struct sockaddr *SocketAddrMapTranslate(const SocketAddrMapT *pAddrMap, struct sockaddr *pResult, const struct sockaddr *pSource, int32_t *pNameLen);

// map an IPv6 address and return a vIPv4 address that can be used to reference it
int32_t SocketAddrMapAddress(SocketAddrMapT *pAddrMap, const struct sockaddr *pAddr, int32_t iAddrSize);

// remap an existing IPv6 address and return a vIPv4 address that can be used to reference it
int32_t SocketAddrRemapAddress(SocketAddrMapT *pAddrMap, const struct sockaddr *pOldAddr, const struct sockaddr *pNewAddr, int32_t iAddrSize);

// removes an addres mapping from the mapping table
int32_t SocketAddrUnmapAddress(SocketAddrMapT *pAddrMap, const struct sockaddr *pAddr, int32_t iAddrSize);


#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtynetpriv_h


