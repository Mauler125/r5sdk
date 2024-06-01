/*H*************************************************************************************/
/*!
    \File netgamepkt.h

    \Description
        Defines netgame packet constants and types.

    \Copyright
        Copyright (c) Electronic Arts 1998-2007.

    \Version 1.0 12/12/1998 (gschaefer) First Version
    \Version 1.1 11/18/2002 (jbrookes) Moved to NetGame hierarchy
    \Version 1.2 02/04/2007 (jbrookes) Cleanup, added NetGameMaxPacketT
*/
/*************************************************************************************H*/

#ifndef _netgamepkt_h
#define _netgamepkt_h

/*!
\Moduledef NetGamePkt NetGamePkt
\Modulemember Game
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

/* sampling period constants
  dont enum these or you will regret it
  (the compiler translates divides by these into logical shifts) */
#define RTT_SAMPLE_PERIOD 1024
#define LATE_SAMPLE_PERIOD 512

//! identifiers of client/server packets
enum
{
    GAME_PACKET_ONE_BEFORE_FIRST =1,//!< enum with value 1 available for future use. If needed just lower ONE_BEFORE_FIRST to zero
    GAME_PACKET_INPUT,              //!< data exchanged by machines using dist, usually controller inputs
    GAME_PACKET_INPUT_MULTI,        //!< same as INPUT but from the game server.
    GAME_PACKET_STATS,              //!< stats sent by the game server.
    GAME_PACKET_USER,               //!< user packet
    GAME_PACKET_USER_UNRELIABLE,    //!< user packet, to be sent unreliably
    GAME_PACKET_USER_BROADCAST,     //!< user packet, to be sent unreliably to broadcast address
    GAME_PACKET_INPUT_FLOW,         //!< dist flow control, sent to indicate readiness
    GAME_PACKET_INPUT_MULTI_FAT,    //!< same as INPUT_MULTI but 16-bit individual packet sizes (to allow 256-1200 sizes).
    GAME_PACKET_INPUT_META,         //!< meta-information on the packet format
    GAME_PACKET_LINK_STRM,          //!< netgamelink stream packet
    GAME_PACKET_ONE_PAST_LAST,      //!< used to mark the valid range. Insert new kinds before
    GAME_PACKET_SYNC = 64           //!< OR this value with packet kind to signal that packet conveys sync info
};

//! default max raw data packet size
#define NETGAME_DATAPKT_DEFSIZE (240)

//! max raw data packet size
#define NETGAME_DATAPKT_MAXSIZE (1200)

//! max size of packet tail (sync+kind)
#define NETGAME_DATAPKT_MAXTAIL (sizeof(NetGamePacketSyncT)+1)

//! max stream packet size
#define NETGAME_STRMPKT_DEFSIZE (200)
#define NETGAME_STRMPKT_MAXSIZE (NETGAME_DATAPKT_MAXSIZE - 12)

//! default max input/output buffers, in packets
#define NETGAME_DATABUF_MAXSIZE (32)

// sync packet flags

//!< indicates repl field is not yet valid (remote side has not received our first sync packet yet)
#define NETGAME_SYNCFLAG_REPLINVALID    (0x01)


/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! packet header (common for every packet)
typedef struct NetGamePacketHeadT
{
    int32_t size;               //!< total size (including header, 4-byte aligned)
    uint32_t when;              //!< reception time in ticks
    uint16_t len;               //!< length of packet body
    uint8_t kind;               //!< packet type (see enum above)
    uint8_t pad;                //!< unused (alignment)
} NetGamePacketHeadT;

//! format of a sync packet (piggybacks onto regular packets)  -- FOR INTERNAL USE ONLY
typedef struct NetGamePacketSyncT
{
    uint32_t repl;
    uint32_t echo;
    int16_t late;
    uint8_t psnt;       //!< number of packets sent since last update
    uint8_t prcvd;      //!< number of packets received since last update
    uint8_t plost;      //!< number of inbound packets lost since last update
    uint8_t nsnt;       //!< number of NAKs sent since last update
    uint8_t flags;      //!< sync packet flags (NETGAME_SYNCFLAG_*)
    uint8_t size;       //!< size of sync packet (MUST COME LAST)
} NetGamePacketSyncT;

//! format of packets which are exchanged by client/server
typedef struct NetGamePacketT
{
    //! packet header (not sent)
    NetGamePacketHeadT head;

    //! game packet body
    union {
        //! stream data
        struct {
            int32_t ident;          //!< stream ident
            int32_t kind;           //!< token
            int32_t size;           //!< packet size
            uint8_t data[NETGAME_STRMPKT_DEFSIZE];//!< binary data
        } strm;
        //! raw data packet
        uint8_t data[NETGAME_DATAPKT_DEFSIZE];
    } body;

    //! space for game packet tail
    uint8_t tail[NETGAME_DATAPKT_MAXTAIL];
} NetGamePacketT;

//! format of a max-sized link packet (may be substituted for a NetGamePacketT)
typedef struct NetGameMaxPacketT
{
    //! packet header (not sent)
    NetGamePacketHeadT head;

    //! game packet body
    union {
        //! stream data
        struct {
            int32_t ident;          //!< stream ident
            int32_t kind;           //!< token
            int32_t size;           //!< packet size
            uint8_t data[NETGAME_STRMPKT_MAXSIZE];//!< binary data
        } strm;
        //! raw data packet
        uint8_t data[NETGAME_DATAPKT_MAXSIZE];
    } body;

    //! space for game packet tail
    uint8_t tail[NETGAME_DATAPKT_MAXTAIL];
} NetGameMaxPacketT;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

//@}

#endif // _netgamepkt_h

