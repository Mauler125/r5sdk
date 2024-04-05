/*H********************************************************************************/
/*!
    \File voippacket.h

    \Description
        VoIP data packet definitions.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 03/17/2004 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _voippacket_h
#define _voippacket_h

/*** Include files ****************************************************************/

#include "DirtySDK/dirtysock/dirtynet.h"

/*** Defines **********************************************************************/

//! maximum voice payload size
#define VOIP_MAXMICRPKTSIZE         (SOCKET_MAXUDPRECV - sizeof(VoipPacketHeadT) - sizeof(VoipMicrInfoT))

//! maximum ping payload size
#define VOIP_MAXPINGPKTSIZE         (SOCKET_MAXUDPRECV - sizeof(VoipPacketHeadT) - 4 /* aRemoteClientId */ - 4 /* aChannelUserIndices */)

//! VoIP Flags bit field included in VoIP packets
#define VOIP_PACKET_STATUS_FLAG_METADATA     (1)     //!< packet contains platform-specific metadata (unreliable)
#define VOIP_PACKET_STATUS_FLAG_STT          (2)     //!< at least one user on the originating side as requested transcribed text to be sent (Speech-to-text accessibility feature)
#define VOIP_PACKET_RELIABLE_FLAG_ACK_ACKCNF (32)    //!< packet contains ACK+ACKCNF protocol components (reliability mechanism)
#define VOIP_PACKET_RELIABLE_FLAG_DATA       (64)    //!< packet contains DATA protocol component (reliability mechanism)

//! range of supported reliable data types
#define VOIP_RELIABLE_TYPE_USERADD          (0)     //!< reliable data contains newly added local user on the originating console (used for voip join-in-progress)
#define VOIP_RELIABLE_TYPE_USERREM          (1)     //!< reliable data contains newly removed local user on the originating console (used for voip leave-in-progress)
#define VOIP_RELIABLE_TYPE_OPAQUE           (2)     //!< reliable data contains platform-specific data blob that needs to be transmitted reliably
#define VOIP_RELIABLE_TYPE_TEXT             (3)     //!< reliable data contains transcribed text (Speech-to-Text) that needs to be sent reliably

//! mask used to manipulate ACKCNF bit for uAckedSeq field of ReliableAckT type
#define VOIP_RELIABLE_ACKCNF_MASK           (0x80)


/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/
//! VoIP data packet header
typedef struct VoipPacketHeadT
{
    uint8_t         aType[3];        //!< type/version info
    uint8_t         uFlags;          //!< flags
    uint8_t         aClientId[4];    //!< source clientId
    uint8_t         aSessionId[4];   //!< session ID
    uint8_t         aHeadsetStat[2]; //!< headset status bitfield (1 bit per local user)
} VoipPacketHeadT;


//! VoIP channel config
typedef struct VoipPacketChanT
{
    uint8_t         aChannelId[4];  //!< packet channelId
} VoipPacketChanT;

//! VoIP user packet
typedef struct VoipUserPacketT
{
    uint8_t aAccountId[8];  //!< the nucleus account id (unique per account)
    uint8_t aPersonaId[8];  //!< the nucleus persona id (globally unique)
    uint8_t aFlags[4];      //!< a bit field contaning VOIPUSER_FLAG_*
    uint8_t aPlatform[4];   //!< what platform is this user running on 
} VoipUserPacketT;

//! VoIP connection packet
typedef struct VoipConnPacketT
{
    VoipPacketHeadT Head;
    uint8_t         aRemoteClientId[4];
    uint8_t         bConnected;
    uint8_t         uNumLocalUsers; // this is the max number of users including the extended user
    VoipUserPacketT LocalUsers[32];
} VoipConnPacketT;

//! VoIP ping packet
typedef struct VoipPingPacketT
{
    VoipPacketHeadT Head;
    uint8_t         aRemoteClientId[4];
    uint8_t         aChannelUserIndices[4];     //!< a bitset that corresponds to the user indices for the channel data in aData
    uint8_t         aData[VOIP_MAXPINGPKTSIZE]; //!< ping packet data (this is where piggy-backed channel & reliable data is added when needed)
} VoipPingPacketT;

//! VoIP disc packet
typedef struct VoipDiscPacketT
{
    VoipPacketHeadT Head;
    uint8_t         aRemoteClientId[4];
} VoipDiscPacketT;

//! VoIP micr packet info
typedef struct VoipMicrInfoT
{
    uint8_t         aSendMask[4];   //!< lsb->msb, one bit per user, used for voip server
    VoipPacketChanT channels;       //!< packet channelId
    uint8_t         uSeqn;          //!< voice packet sequence number
    uint8_t         uNumSubPackets; //!< current number of sub-packets packed in this packet
    uint8_t         uSubPacketSize; //!< size of sub-packets (same fixed-size for all sub-packets)
    uint8_t         uUserIndex;     //!< index of user that generated all the sub-packets in this packet
} VoipMicrInfoT;

//! VoIP data packet
typedef struct VoipMicrPacketT
{
    VoipPacketHeadT Head;           //!< packet header
    VoipMicrInfoT   MicrInfo;       //!< micr packet info
    uint8_t         aData[VOIP_MAXMICRPKTSIZE]; //!< voice packet data (this is where sub-packets are bundled)
} VoipMicrPacketT;

//! VoipPacketBufferT: a buffer that can(does) contain any type of voip packet
typedef union VoipPacketBufferT
{
    VoipPacketHeadT VoipPacketHead;
    VoipConnPacketT VoipConnPacket;
    VoipPingPacketT VoipPingPacket;
    VoipDiscPacketT VoipDiscPacket;
    VoipMicrPacketT VoipMicrPacket;
} VoipPacketBufferT;

//! reliable data info
typedef struct ReliableDataInfoT
{
    uint8_t aRemoteClientId[4]; //!< remote peer to which this data entry is for (required for 1x send to voipserver)
    uint8_t uSeq;               //!< sequence number
    uint8_t uType;              //!< type (VOIP_RELIABLE_TYPE_*)
    uint8_t uSize[2];           //!< size of payload
} ReliableDataInfoT;

//! reliable mechanism - ack entry 
typedef struct ReliableAckT
{
    uint8_t aRemoteClientId[4]; //!< remote peer to which this ack entry is for
    uint8_t uAckedSeq;          //!< bit 7 = ACKCNF flag; bit 0 to 6 = [1,127] seq number (seq nb 0 is invalid - used when only the ACKCNF portion is valid)
} ReliableAckT;

//! maximum size of a single reliable data entry (in bytes) is what can fit in a PING packet (taking into account that there can be up to 32 ReliableAckT entries before it)
//! assumption: very large reliable data entries may not fit in MIC packets (because of prefixed metadata) but will eventually be sent over a PING packet.
#define VOIP_MAXRELIABLEDATA        (VOIP_MAXPINGPKTSIZE - (VOIP_MAXCONNECT * sizeof(ReliableAckT)))

//! reliable mechanism - data entry
typedef struct ReliableDataT
{
    ReliableDataInfoT info;
    uint8_t aData[VOIP_MAXRELIABLEDATA];
} ReliableDataT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#endif // _voippacket_h

