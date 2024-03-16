/*H*************************************************************************************/
/*!
    \File    netgamelink.h

    \Description
        The GameLink class provides the main control interface between the
        individual game controllers and the distributed game-input engine.
        The GameInput and GameComm classes provided the interfaces used by
        this class for real-world communication.

    \Copyright
        Copyright (c) Electronic Arts 1998-2002

    \Version 1.0 12/15/1998 (gschaefer) First Version
    \Version 1.1 12/26/1999 (gschaefer) Revised to add external stats interface
    \Version 1.2 11/18/2002 (jbrookes) Moved to NetGame hierarchy
*/
/*************************************************************************************H*/

#ifndef _netgamelink_h
#define _netgamelink_h

/*!
\Moduledef NetGameLink NetGameLink
\Modulemember Game
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/game/netgamepkt.h"
#include "DirtySDK/game/netgameerr.h"

/*** Defines ***************************************************************************/

#define PING_HISTORY    32      //!< must be a power of 2
#define PING_LENGTH     250     //!< sample every 250 ms
#define PING_DEFAULT    200     //!< default value if no sample available

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! opaque module state
typedef struct NetGameLinkRefT  NetGameLinkRefT;

//! structure providing access to ping history
typedef struct NetGameLinkHistT
{
    uint16_t min;   //!< minimum ping value
    uint16_t max;   //!< maximu ping value
    uint16_t avg;   //!< average ping value
    uint16_t cnt;   //!< number of samples
} NetGameLinkHistT;

//! structure providing access to game/comm stats (in the past, had to be a multiple of 16 bytes to support IOP/EE DMA!)
typedef struct NetGameLinkStatT
{
    volatile uint32_t tick;     //!< most recent tick (used for other tick calculations)
    volatile uint32_t tickseqn; //!< changed every time thread stuff updates the tick

    uint32_t ping;          //!< peer to peer ping
    uint32_t late;          //!< overall latency of game (show this to user)
    uint32_t conn;          //!< when connection established
    uint32_t sent;          //!< number of bytes sent
    uint32_t rcvd;          //!< number of bytes received
    uint32_t sentlast;      //!< when data most recently sent
    uint32_t rcvdlast;      //!< when data most recently received
    uint32_t sentshow;      //!< show send-data indicator
    uint32_t rcvdshow;      //!< show recv-data indicator
    uint32_t senthide;      //!< hide send-data indicator (ignore -- use show)
    uint32_t rcvdhide;      //!< hide recv-data indicator (ignore -- use recv)
    uint32_t outbps;        //!< [outbound] bytes per second (user data)
    uint32_t outrps;        //!< [outbound] raw bytes per second (data sent to network)
    uint32_t outnps;        //!< [outbound] network bytes per second (rps + estimated UDP/Ethernet frame overhead)
    uint32_t outpps;        //!< [outbound] packets per second (user packets)
    uint32_t outrpps;       //!< [outbound] raw packets per second (packets sent to network)
    uint32_t inbps;         //!< [inbound] received bytes per second
    uint32_t inrps;         //!< [inbound] received raw bytes per second
    uint32_t innps;         //!< [inbound] received network bytes per second
    uint32_t inpps;         //!< [inbound] received packets per second
    uint32_t inrpps;        //!< [inbound] received raw packets per second (rps + estimated UDP/Ethernet frame overhead)    
    uint32_t stattick;      //!< the tick at which period stats were updated
    uint32_t lnaksent;      //!< number of NAKs received by peer (from us) since start
    uint32_t lpacksent;     //!< number of packets sent to peer since start (at time = last inbound sync packet)
    uint32_t lpacksent_now; //!< number of packets sent to peer since start (at time = now)
    uint32_t lpackrcvd;     //!< number of packets received from peer since start
    uint32_t lpacklost;     //!< remote->local packets lost: number of packets (from peer) lost since start - not always equal to (rpacksent - lpackrcvd)
    uint32_t lpacksaved;    //!< number of packets recovered by underlying commudp redundancy mechanism
    uint32_t rnaksent;      //!< number of NAKs sent by peer (to us) since start
    uint32_t rpacksent;     //!< number of packets sent by peer (to us) since start
    uint32_t rpackrcvd;     //!< number of packets received by peer (from us) since start
    uint32_t rpacklost;     //!< local->remote packets lost: number of packets (from us) lost by peer since start - not always equal to (lpacksent - rpackrcvd)
    /*  Notes
        Attempting to calculate the number of local->remote packets lost with (lpacksent - rpackrcvd) sometimes
        lead to a boosted packet loss result because some in-flight packets are accounted in the sent counter but not
        in the rcved counter. For a precise local->remote packet loss value, rpacklost is better because
        it is obtained from the remote end directly (from the inbound NetGameLinkSyncT packet)

        Attempting to calculate the number of remote->local packets lost with (rpacksent - lpackrcvd) sometimes
        lead to reduced (or even negative) packet loss result because additional inbound packets are
        received when commudp retransmission mechanisms quick in to guarantee transport reliability. Those
        packet are accounted for in the rcved counter but not in the sent counter. For a precise
        remote->local packet loss value, lpacklost is better because it takes the above phenomenon
        into account.  */

    uint8_t  isconn;        //!< listening or connecting \Deprecated
    uint8_t  isopen;        //!< listening, connecting, or connected
    uint8_t  pad0[2];       //!< pad to four-byte alignment

    uint32_t pingtick;      //!< last tick at which ping stuff updated
    uint32_t pingslot;      //!< the index containing the current ping
    int32_t  pingdev;       //!< ping deviation
    int32_t  pingavg;       //!< ping average

    NetGameLinkHistT pinghist[PING_HISTORY];
} NetGameLinkStatT;

// stream structure
typedef struct NetGameLinkStreamT NetGameLinkStreamT;

//! dist stream send proc
typedef int32_t (NetGameLinkStreamSendProc)(NetGameLinkStreamT *pStream, int32_t iSubchan, int32_t iKind, void *pBuffer, int32_t iLen);
//! dist stream recv proc
typedef void (NetGameLinkStreamRecvProc)(NetGameLinkStreamT *pStream, int32_t iSubchan, int32_t iKind, void *pBuffer, int32_t iLen);

typedef struct NetGameLinkStreamInpT
{
    char *pInpData;                 //!< private
    int32_t iInpSize;               //!< private
    int32_t iInpProg;               //!< private
    int32_t iInpKind;               //!< private
} NetGameLinkStreamInpT;

//! structure to access network stream with
struct NetGameLinkStreamT
{
    NetGameLinkStreamT *pNext;      //!< private
    NetGameLinkRefT *pClient;       //!< private

    int32_t iIdent;                 //!< private (can read -- stream identifier)
    int32_t iSubchan;               //!< number of subchannels
    int32_t iRefNum;                //!< public (for callers use)
    void *pRefPtr;                  //!< public (for callers use)
    NetGameLinkStreamSendProc *Send;//!< public
    NetGameLinkStreamRecvProc *Recv;//!< public
    int32_t iQueueDepth;            //!< public read only (total bytes in output queue)
    int32_t iQueueLimit;            //!< public read/write (maximum outgoing queue limit)

    int32_t iHighWaterUsed;         //!< public read only
    int32_t iHighWaterNeeded;       //!< public read only

    int32_t iInpMaxm;               //!< private
    NetGameLinkStreamInpT *pInp;    //!< private

    char *pOutData;                  //!< private
    int32_t iOutMaxm;                //!< private
    int32_t iOutSize;                //!< private
    int32_t iOutProg;                //!< private

    char *pSynData;                  //!< private
    int32_t iSynMaxm;                //!< private
    int32_t iSynSize;                //!< private
};

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// construct the game client
DIRTYCODE_API NetGameLinkRefT *NetGameLinkCreate(void *pCommRef, int32_t iOwner, int32_t iBufLen);

// destruct the game client
DIRTYCODE_API void NetGameLinkDestroy(NetGameLinkRefT *pRef);

// register a callback function
DIRTYCODE_API void NetGameLinkCallback(NetGameLinkRefT *pRef, void *pCallData, void (*pCallProc)(void *pCallData, int32_t iKind));

// status function
DIRTYCODE_API int32_t NetGameLinkStatus(NetGameLinkRefT *pRef, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);

// incoming packet stream from upper layer
DIRTYCODE_API int32_t NetGameLinkSend(NetGameLinkRefT *pRef, NetGamePacketT *pPkt, int32_t iLen);

// peek into the buffer
DIRTYCODE_API int32_t NetGameLinkPeek(NetGameLinkRefT *pRef, NetGamePacketT **ppPkt);

// peek into the buffer for specific packet types
DIRTYCODE_API int32_t NetGameLinkPeek2(NetGameLinkRefT *pRef, NetGamePacketT **ppPkt, uint32_t uMask);

// outgoing packet stream to upper layer
DIRTYCODE_API int32_t NetGameLinkRecv(NetGameLinkRefT *pRef, NetGamePacketT *pBuf, int32_t iLen, uint8_t bDist);

// same as NetGameLinkRecv, but takes a mask of which GAME_PACKET types we want
DIRTYCODE_API int32_t NetGameLinkRecv2(NetGameLinkRefT *pRef, NetGamePacketT *pBuf, int32_t iLen, uint32_t uMask);

// control behavior
DIRTYCODE_API int32_t NetGameLinkControl(NetGameLinkRefT *pRef, int32_t iSelect, int32_t iValue, void *pValue);

// dispatch to appropriate updaters
DIRTYCODE_API uint32_t NetGameLinkUpdate(NetGameLinkRefT *pRef);

// create a new network stream
DIRTYCODE_API NetGameLinkStreamT *NetGameLinkCreateStream(NetGameLinkRefT *pRef, int32_t iSubChan, int32_t iIdent, int32_t iInpLen, int32_t iOutLen, int32_t iSynLen);

// destroy an existing network stream
DIRTYCODE_API void NetGameLinkDestroyStream(NetGameLinkRefT *pRef, NetGameLinkStreamT *pStream);

#ifdef __cplusplus
}
#endif

//@}

#endif // _netgamelink_h

