/*H********************************************************************************/
/*!
    \File prototunnel.c

    \Description

        ProtoTunnel creates and manages a Virtual DirtySock Tunnel connection.  The
        tunnel transparently bundles data sent from multiple ports to a specific
        remote host into a single send, optionally encrypting packets based on the
        tunnel mappings set up by the caller.  Only data sent over a UDP socket may
        be tunneled.  ProtoTunnel does not itself send packets unsolicited, it merely
        encapsulates packets being sent by other API's like CommUDP or VoIP.
        Therefore, ProtoTunnel packets always include user data.

    \Notes
    \verbatim
        Encryption Key

            ProtoTunnel assumes that the encryption key used is provided by the
            caller creating the tunnel.  It does not provide any mechanism for
            secure key exchange; this is left to the caller.  A good random
            encryption key of at least sixty-four characters is strongly recommended
            to provide adequate security.

        Packet format

            ProtoTunnel bundles multiple packets bound for different ports on the
            same destination address into a single packet.  There are two types of
            packets, handshake packets and regular packets.  A handshake packet is
            identical in layout to a regular packet, with the addition of handshake
            data.  Packets contain the following data elements:

                - Two-byte Ident field
                - Two-byte tunnel encryption header
                - Handshake info (handshake packets only)
                - Encrypted list of one or more two-byte sub-packet headers
                - Encrypted HMAC message digest
                - Encrypted packet data (if any)
                - Unencrypted packet data (if any)

                0              8             15
                -------------------------------
                |            Ident            |     Ident field
                -------------------------------
                |         Encryption          |     Encryption field
                -------------------------------
                |                             |
                |          Handshake          |     Handshake field (handshake packets only)
                |                             |
                -------------------------------
                |     Packet size      | PIdx |     Packet #1 header
                -------------------------------
                |     Packet size      | PIdx |     Packet #2 header
                -------------------------------
                                .
                                .
                                .
                -------------------------------
                |     Packet size      | PIdx |     Packet #n header
                -------------------------------
                |                             |
                |            HMAC             |     HMAC message digest (length dependent on HMAC type and size, may be zero)
                |                             |
                -------------------------------
                |                             |
                |    Encrypted Packet Data    |     Encrypted Packet Data
                |                             |
                -------------------------------
                |                             |
                |   Unencrypted Packet Data   |     Unencrypted Packet Data
                |                             |
                -------------------------------

            Ident field

            The Ident field contains one of two values; handshake packets are identified
            by the high bit being set (0x8000) and otherwise contain a two byte protocol
            version identifier.  For example, an Ident field of 0x8101 would indicate a
            handshake packet using protocol version 1.1.  Non-handshake packets do not
            include the high bit, with the lower 15 bits used to indicate the remote
            tunnel index on an active connection.  The ident field is authenticated but
            not encrypted, so it may be processed before the packet is decrypted.  During
            handshaking, an endpoint receiving packets with a lesser version of the
            protocol specified will downgrade to that version.  (Note that downgrading
            to versions older than 1.1 is not supported).  The ident field is the only
            field that is required to exist in any given ProtoTunnel protocol version.

            Encryption field

            The Encryption field contains a 16bit stream index used to synchronize
            the stream cipher in the event of lost packets.  The high bit (0x8000)
            is reserved to indicate that the packet is encrypted.  The remaining
            15bit value is shifted up by three bits to give a total window size of 256k.
            Because of the shift, the stream cipher encrypts in blocks of eight bytes.
            This requires the stream cipher to be iterated over the amount
            of data that is encrypted, and then advanced by the number of bytes
            required to round up to the next multiple of eight bytes.  In the event
            one or more tunnel packets are lost, the receiving side will advance the
            stream cipher by the amount of data lost to remain synchronized with the
            sender.  A packet received that has a negative stream offset relative to
            the current offset is assumed to be a previous packet.  ProtoTunnel will
            make an attempt to decrypt such a packet with a copy of the previous
            state.  If that fails, the packet is discarded.  All packets contain some
            encrypted data, and all packet data is authenticated (see HMAC below).

            Handshake field

            Handshake data is sent until a connection is determined to be active. The
            format of the field is as follows:

                0              8             15
                -------------------------------
                |        Client Ident         |     Client Ident field
                |                             |
                -------------------------------
                |      Connection Ident       |     Connection Ident field
                -------------------------------
                |    Active    |XXXXXXXXXXXXXX|     Active field
                -------------------------------

            The Client Ident is used on the receiving side to match incoming handshake
            packets to the specific tunnel endpoint configured to receive them.  This
            is necessary when the packets arrive from an unexpected address and/or port,
            usually due to NAT.  Because the Handshake data is unencrypted, this can
            be trivially done, without having to potentially try decryption against
            many endpoints.  This is an important consideration for servers that many
            have hundreds or even thousands of endpoints.

            The Connection Ident field is used to specify the Ident that non-handshake
            packets from the remote endpoint will use to identify the connection they
            belong to.  This ident is simply the index of the tunnel endpoint in the
            tunnel list.  Because the non-handshake Ident field is 15 bits, this limits
            the maximum number of tunnel endpoints to 32,767.  The Ident present in
            non-handshake packets is useful when a port and/or address change takes
            place.  There are multiple scenarios where this type of behavior can occur;
            for example a connection where data is not sent for a period of time can
            have its route expired by a NAT device.  When data resumes being sent,
            it likely will originate from a different port.  Another example is a
            corporate firewall, which may aggressively reallocate ports or even
            addresses dynamically.  Including the Ident in unencrypted form non-
            handshake packets allows trivial rematching of incoming packet data without
            needing to decrypt the packet data.

            The Active field is how ProtoTunnel determines if a connection is active.
            Initially, it is set to zero.  When an endpoint successfully receives
            data from a remote endpoint, the Active field in the handshake packets
            it sends is set to one.  When an endpoint receives a handshake packet
            from a remote endpoint with the Active field set to one, the connection
            is considered to be complete, and handshake information is no longer
            sent.

            Packet Headers

            Packet headers consist of a twelve-bit size field and a four-bit port
            index.  This means the maximum bundled packet size is 4k and the maximum
            number of port mappings in a tunnel is 16.  ProtoTunnel restricts the
            actual maximum number of port mappings to eight.  ProtoTunnel also
            reserves the last port to use for embedded control data (see ProtoTunnel
            Control Info below), so this leaves seven user-allocatable ports.  It
            also means that tunnels are required to be created with the same port
            mappings on each side of the tunnel, otherwise the bundled tunnel
            packets will not be forwarded to the correct ports.  The tunnel
            demultiplexer iterates through the packet headers until the aggregate
            size of packet data plus packet headers plus encryption header equals
            the size of the received packet.  When the packet format is encrypted,
            packet headers are decrypted one at a time until all of the headers are
            successfully decrypted.  If there are any discrepencies (e.g. invalid
            port mappings or aggregate sizes don't match) the packet is considered
            to be invalid and is discarded.

            Encrypted HMAC message digest

            The HMAC message digest provides message authentication; in the event
            that packet data is modified on the wire, the HMAC calculation on the
            receiving side will not match the expected result, which will indicate
            that the data has been modified, and the packet will be discarded.  Any
            supported HMAC algorithm maybe be chosen, and any amount of truncation
            is supported up to 1/2 the hash size.  The maximum size of the HMAC is
            24 bytes.  A NULL HMAC may be chosen, in which case no space is consumed
            in the packet body.  It is assumed that both sides are configured with
            the same HMAC type and size, otherwise a connection is not be possible.

            Encrypted Packet Data

            Encrypted packets, if any, immediately follow the final header.  All
            encrypted packet data is decrypted together in one pass.

            Unencrypted Packet Data

            Unencrypted packets, if any, immediately follow encrypted packets, or
            packet headers if there are no encrypted packets.

        Matching packets to an endpoint

            When ProtoTunnel is running in a configuration where multiple endpoints
            are using the same socket, an incoming packet must be matched to a specific
            tunnel endpoint for it to be properly received.  Two examples of this type
            of scenario are a dedicated server, or a peer-mesh game with more than
            two clients.  Due to NAT behavior we may not always know where a packet
            will be coming from at any given time.

            ProtoTunnel matches differently depending on whether the tunnel is active
            or not.  If the tunnel is not yet active, the ClientId field in the handshake
            data is used to match the packet to the appropriate tunnel.  During this
            phase of handshaking, ProtoTunnel will update the address and port of the
            remote endpoint based on where the data originates from, assuming the
            packet can be decrypted and authenticated.  When a tunnel is active,
            ProtoTunnel uses the Ident field to directly route the packet to the proper
            endpoint.  If the source addr and/or port change while a tunnel is active,
            the updated port will be detected and used to match future packets.

    \endverbatim

    \Copyright
        Copyright (c) 2005-2015 Electronic Arts Inc.

    \Version 12/02/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/crypt/cryptarc4.h"
#include "DirtySDK/crypt/crypthash.h"
#include "DirtySDK/crypt/crypthmac.h"

#include "DirtySDK/proto/prototunnel.h"

/*** Defines **********************************************************************/

#define PROTOTUNNEL_HMAC_DEBUG      (FALSE)
#define PROTOTUNNEL_HMAC_MAXSIZE    (16)                    //!< maximum supported HMAC size
#define PROTOTUNNEL_HMAC_DEFSIZE    (12)                    //!< default supported HMAC size

#define PROTOTUNNEL_PKTHDRSIZE      (2)
#define PROTOTUNNEL_MAXHDROFFS      (sizeof(ProtoTunnelHandshakeT))
#define PROTOTUNNEL_MAXPACKETS      (8)
#define PROTOTUNNEL_MAXHDRSIZE      (PROTOTUNNEL_PKTHDRSIZE * PROTOTUNNEL_MAXPACKETS)
#define PROTOTUNNEL_PACKETBUFFER    (SOCKET_MAXUDPRECV)
#define PROTOTUNNEL_MAXPACKET       (PROTOTUNNEL_PACKETBUFFER - PROTOTUNNEL_MAXHDROFFS - PROTOTUNNEL_PKTHDRSIZE - PROTOTUNNEL_HMAC_MAXSIZE)

#define PROTOTUNNEL_CRYPTBITS       (3) // number of bits added to extend window
#define PROTOTUNNEL_CRYPTALGN       (1 << PROTOTUNNEL_CRYPTBITS)
#define PROTOTUNNEL_CRYPTMASK       (PROTOTUNNEL_CRYPTALGN-1)

#define PROTOTUNNEL_CRYPTARC4_ITER  (12) // improved security by skipping slightly less secure first output data (3k bytes), this data is not transmitted so does not produce overhead on the connection

#define PROTOTUNNEL_CONTROL_PORTIDX (PROTOTUNNEL_MAXPORTS-1)

#define PROTOTUNNEL_IDENT_HANDSHAKE (0x8000)

#define PROTOTUNNEL_IPPROTO         (IPPROTO_IP)

#define PROTOTUNNEL_MAXKEYS         (PROTOTUNNEL_MAXPORTS)

// comma delimited set of support versions
// IMPORTANT: make sure to add/delete when min/current version is changed
#define PROTOTUNNEL_VERSIONS        ("1.1")

typedef enum ProtoTunnelControlE
{
    PROTOTUNNEL_CTRL_INIT
} ProtoTunnelControlE;

typedef enum ProtoTunnelPortListFuncE
{
    PROTOTUNNEL_PORTLIST_ADD,
    PROTOTUNNEL_PORTLIST_DEL,
    PROTOTUNNEL_PORTLIST_CHK
} ProtoTunnelPortListFuncE;

// packet processing return codes
#define PROTOTUNNEL_PACKETRECVFAIL_VALIDATE     (-1)   //!< decrypt or validate failed
#define PROTOTUNNEL_PACKETRECVFAIL_NOMATCH      (-2)   //!< could not match packet to a tunnel
#define PROTOTUNNEL_PACKETRECVFAIL_VERSION      (-3)   //!< fatal prototunnel protocol version mismatch
#define PROTOTUNNEL_PACKETRECVFAIL_OUTOFORDER   (-4)   //!< out-of-order packet discard; packet is previous to our current window offset
#define PROTOTUNNEL_PACKETRECVFAIL_TRYAGAIN     (-5)   //!< packet decrypt failed but rematch found new key/offset; try again

/*** Macros ***********************************************************************/

#define PROTOTUNNEL_GetIdentFromPacket(__pPacketData) (((__pPacketData[0])<<8) | (__pPacketData)[1])
#define PROTOTUNNEL_GetEncryptionFromPacket(__uTunnelVers, __pPacketData) (((__pPacketData[2])<<8) | (__pPacketData)[3])
#define PROTOTUNNEL_GetStreamOffsetFromPacket(__uTunnelVers, __pPacketData) (PROTOTUNNEL_GetEncryptionFromPacket(__uTunnelVers, __pPacketData) & 0x7fff)

/*** Type Definitions *************************************************************/

//! handshake packet format for version 1.1 of the protocol
typedef struct ProtoTunnelHandshake_1_1_T
{
    uint8_t aCrypt[2];              //!< stream cipher offset
    uint8_t aClientIdent[4];        //!< client identifier, used to tag handshake packets for initial matching of packet to tunnel
    uint8_t aConnIdent[2];          //!< connection identifier, used to tag regular packets for re-matching
    uint8_t uActive;                //!< is connection active?
} ProtoTunnelHandshake_1_1_T;

//! prototunnel handshake packet format
typedef struct ProtoTunnelHandshakeT
{
    uint8_t aIdent[2];
    union
    {
        ProtoTunnelHandshake_1_1_T V1_1;
    } Version;
} ProtoTunnelHandshakeT;

//! handshake control info
typedef struct ProtoTunnelControlT
{
    uint8_t uPacketType;        //!< PROTOTUNNEL_CTRL_*
    uint8_t aClientId[4];       //!< source clientId
    uint8_t aProtocolVers[2];   //!< protocol version
    uint8_t uActive;            //!< is connection active?
} ProtoTunnelControlT;

typedef struct ProtoTunnelT
{
    ProtoTunnelInfoT    Info;               //!< mapping info
    uint32_t            uVirtualAddr;       //!< virtual address, used for NAT support
    uint32_t            uLocalClientId;     //!< tunnel-specific local clientId (can override ProtoTunnelT if non-zero)
    int16_t             iBuffSize;          //!< current next open position in packet data buffer
    int16_t             iDataSize;          //!< current amount of data queued
    int8_t              iNumPackets;        //!< number of packets queued
    uint8_t             uHandshakeSize;     //!< size of handshake header, determined by protocol version
    uint8_t             uConnIdent[2];      //!< connection identifier, determined in handshaking (version 1.1+ only)

    NetCritT            PacketCrit;         //!< packet buffer critical section

    uint16_t            uSendOffset;        //!< stream send offset
    uint16_t            uRecvOffset;        //!< stream recv offset
    uint16_t            uRecvOOPOffset;     //!< stream recv offset for OOPState
    uint16_t            _pad2;

    CryptArc4T          CryptSendState;     //!< crypto state for encrypting data
    CryptArc4T          CryptRecvState;     //!< crypto state for decrypting data
    CryptArc4T          CryptRecvOOPState;  //!< state for receiving out-of-order packets

    #if DIRTYCODE_LOGGING
    uint32_t            uLastStatUpdate;    //!< last time stat update was printed
    ProtoTunnelStatT    LastSendStat;       //!< previous send stat
    #endif

    uint32_t            uLastTunnelSend;    //!< last time data was sent on this tunnel

    ProtoTunnelStatT    SendStat;           //!< cumulative send statistics
    ProtoTunnelStatT    RecvStat;           //!< cumulative recv statistics

    uint32_t            uLastSendNumBytes;  //!< tracking variable for send bytes per second calculation
    uint32_t            uLastSendNumSubpacketBytes; //!< tracking variable for send number of subpacket bytes per second calculation
    uint32_t            uLastRecvNumBytes;  //!< tracking variable for receive bytes per second calculation
    uint32_t            uLastRecvNumSubpacketBytes; //!< tracking variable for receive number of subpacket bytes per second calculation

    char                aKeyList[PROTOTUNNEL_MAXKEYS][128];        //!< crypto key(s)
    char                aHmacKey[64];       //!< storage for HMAC key (HMAC IV encrypted by tunnel key)
    uint8_t             uActive;            //!< "active" if we've received data from remote peer
    uint8_t             uRefCount;          //!< tunnel ref count
    uint8_t             uSendKey;           //!< index of key we are using to send with
    uint8_t             uRecvKey;           //!< index of key we are using to recv with
    uint8_t             bSendCtrlInfo;      //!< whether to send control info or not
    uint8_t             aForceCrypt[PROTOTUNNEL_MAXPACKETS];    //!< force crypt override (per sub-packet)
    uint8_t             aPacketData[PROTOTUNNEL_PACKETBUFFER+PROTOTUNNEL_MAXHDRSIZE+2];  //!< packet aggregator
} ProtoTunnelT;

struct ProtoTunnelRefT
{
    // module memory group
    int32_t         iMemGroup;              //!< module mem group id
    void            *pMemGroupUserData;     //!< user data associated with mem group

    SocketT         *pSocket;               //!< physical socket tunnel uses
    SocketT         *pServerSocket;         //!< server socket (xboxone only)

    uint16_t        aServerRemotePortList[16]; //!< remote ports for server socket
    uint16_t        uTunnelPort;            //!< port socket tunnel is bound to
    uint32_t        uLocalClientId;         //!< local client identifier

    uint16_t        uVersion;               //!< protocol version
    uint8_t         uHmacType;              //!< HMAC hash type
    uint8_t         uHmacSize;              //!< HMAC size

    int32_t         iMaxTunnels;            //!< maximum number of tunnels
    int32_t         iMaxRecv;               //!< maximum number of receive calls that may be made in one execution of _ProtoTunnelRecvCallback
    int32_t         iIdleCbRate;            //!< rate that socket callback is called by idle thread in milliseconds
    int32_t         iVerbosity;             //!< verbosity level

    uint32_t        uVirtualAddr;           //!< next free virtual address
    uint32_t        uFlushRate;             //!< rate at which packets are flushed by update

    #if DIRTYCODE_DEBUG
    int32_t         iPacketDrop;            //!< drop the next n packets
    #endif

    uint32_t        uNumRecvCalls;          //!< number of receive calls
    uint32_t        uNumPktsRecvd;          //!< number of packets received
    uint32_t        uNumSubPktsRecvd;       //!< number of sub-packets received
    uint32_t        uNumPktsDiscard;        //!< number of out-of-order packet discards

    ProtoTunnelCallbackT *pCallback;        //!< prototunnel event callback
    void            *pUserData;             //!< callback user data

    RawRecvCallbackT *pRawRecvCallback;     //!< raw inbound data filtering function
    void            *pRawRecvUserData;      //!< callback user data

    NetCritT        TunnelsCritS;           //!< "send thread" critical section
    NetCritT        TunnelsCritR;           //!< "recv thread" critical section
    ProtoTunnelT    Tunnels[1];             //!< variable-length tunnel array - must be last
};

/*** Variables ********************************************************************/

// hmac init vector; borrowed from SHA512 initial hash
static const uint8_t _ProtoTunnel_aHmacInitVec[64] =
{
    0x6a, 0x09, 0xe6, 0x67, 0xf3, 0xbc, 0xc9, 0x08,
    0xbb, 0x67, 0xae, 0x85, 0x84, 0xca, 0xa7, 0x3b,
    0x3c, 0x6e, 0xf3, 0x72, 0xfe, 0x94, 0xf8, 0x2b,
    0xa5, 0x4f, 0xf5, 0x3a, 0x5f, 0x1d, 0x36, 0xf1,
    0x51, 0x0e, 0x52, 0x7f, 0xad, 0xe6, 0x82, 0xd1,
    0x9b, 0x05, 0x68, 0x8c, 0x2b, 0x3e, 0x6c, 0x1f,
    0x1f, 0x83, 0xd9, 0xab, 0xfb, 0x41, 0xbd, 0x6b,
    0x5b, 0xe0, 0xcd, 0x19, 0x13, 0x7e, 0x21, 0x79
};

static uint8_t _ProtoTunnel_bBaseAddrUsed[256];

//! used to control behavior for _ProtoTunnelSocketOpen
static uint8_t _ProtoTunnel_bRetryRandomOnFailure = TRUE;

/*** Private Functions ************************************************************/

int32_t ProtoTunnelValidatePacket(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelT *pTunnel, uint8_t *pOutputData, const uint8_t *pPacketData, int32_t iPacketSize, const char *pKey);


/*F********************************************************************************/
/*!
    \Function _ProtoTunnelSetVersion

    \Description
        Set protocol version for the specified tunnel

    \Input *pProtoTunnel    - module state
    \Input *pTunnel         - tunnel to set version for
    \Input uTunnelVers      - version to set (PROTOTUNNEL_VERSION_*)

    \Version 03/26/2015 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoTunnelSetVersion(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelT *pTunnel, uint32_t uTunnelVers)
{
    uint32_t uHandshakeSize = 2; // all handshake sizes are a minimum of two bytes

    // calculate size of header
    switch (uTunnelVers)
    {
        case PROTOTUNNEL_VERSION_1_1:
            uHandshakeSize += sizeof(ProtoTunnelHandshake_1_1_T);
            break;
        default:
            NetPrintf(("prototunnel: [%p] trying to set unknown version %d.%d; assuming 1.1 format\n", pProtoTunnel, uTunnelVers>>8, uTunnelVers&0xff));
            uHandshakeSize += sizeof(ProtoTunnelHandshake_1_1_T);
            break;
    }

    pTunnel->Info.uTunnelVers = uTunnelVers;
    pTunnel->uHandshakeSize = uHandshakeSize;
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelGetBaseAddress

    \Description
        Assign a free base address to the specified prototunnel instance

    \Input *pProtoTunnel    - module state

    \Output
        int32_t             - 0 for success, -1 if no address available

    \Version 08/22/2014 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelGetBaseAddress(ProtoTunnelRefT *pProtoTunnel)
{
    int32_t iRetCode = -1;    // default to error
    int32_t iIndex;

    // skip index=0 because we don't want to create base addresses with format 0.x.x.x
    for (iIndex = 1; iIndex < 256; iIndex++)
    {
        if (_ProtoTunnel_bBaseAddrUsed[iIndex] == FALSE)
        {
            _ProtoTunnel_bBaseAddrUsed[iIndex] = TRUE;
            pProtoTunnel->uVirtualAddr = iIndex << 24;

            NetPrintf(("prototunnel: [%p] base virtual address set to %a\n", pProtoTunnel, pProtoTunnel->uVirtualAddr));

            iRetCode = 0; // signal success
            break;
        }
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelReleaseBaseAddress

    \Description
        Return a free base address to be assigned to a new protoutunnel instance

    \Input *pProtoTunnel    - module state

    \Version 08/22/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _ProtoTunnelReleaseBaseAddress(ProtoTunnelRefT *pProtoTunnel)
{
     NetPrintf(("prototunnel: [%p] releasing base virtual address %a\n", pProtoTunnel, (pProtoTunnel->uVirtualAddr&0xFF000000)));

    _ProtoTunnel_bBaseAddrUsed[pProtoTunnel->uVirtualAddr >> 24] = FALSE;
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelIndexFromId

    \Description
        Return tunnel index of tunnel with specified Id, or -1 if there is no
        tunnel that matches.

    \Input *pProtoTunnel    - module state
    \Input uTunnelId        - tunnel ident

    \Output
        int32_t             - tunnel index, or -1 if not found

    \Version 02/20/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelIndexFromId(ProtoTunnelRefT *pProtoTunnel, uint32_t uTunnelId)
{
    int32_t iTunnel;

    // find tunnel id
    for (iTunnel = 0; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel += 1)
    {
        // found it?
        if (pProtoTunnel->Tunnels[iTunnel].uVirtualAddr == uTunnelId)
        {
            return(iTunnel);
        }
    }
    // not found
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelStreamAdvance

    \Description
        Advance stream offset, and advance crypt state if we need to for padding
        purposes.

    \Input *pCryptState - pointer to crypt state to advance
    \Input *pOffset     - pointer to current offset (16bit in crypt data units)
    \Input uOffset      - amount to advance by (in bytes)

    \Version 12/08/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoTunnelStreamAdvance(CryptArc4T *pCryptState, uint16_t *pOffset, uint32_t uOffset)
{
    uint32_t uCryptAlign;

    // convert from number of bytes to number of crypt data units
    *pOffset += (uint16_t)(uOffset>>PROTOTUNNEL_CRYPTBITS);

    // if we have 'left-over' bytes, advance the crypt state and add a data unit
    if ((uCryptAlign = (uOffset & PROTOTUNNEL_CRYPTMASK)) != 0)
    {
        uCryptAlign = PROTOTUNNEL_CRYPTALGN - uCryptAlign;
        CryptArc4Advance(pCryptState, uCryptAlign);
        *pOffset += 1;
    }
    // handle 15 bit wrapping
    *pOffset &= 0x7fff;
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelVirtualToPhysical

    \Description
        Convert a virtual address into its corresponding physical address.

    \Input *pProtoTunnel    - pointer to module state
    \Input uVirtualAddr     - virtual address
    \Input *pBuf            - [out] storage for sockaddr (optional)
    \Input iBufSize         - size of buffer

    \Output
        int32_t             - physical address, or zero if no match

    \Version 03/31/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelVirtualToPhysical(ProtoTunnelRefT *pProtoTunnel, uint32_t uVirtualAddr, char *pBuf, int32_t iBufSize)
{
    ProtoTunnelT *pTunnel;
    uint32_t uRemoteAddr;
    int32_t iTunnel;

    // acquire exclusive access to tunnel list
    NetCritEnter(&pProtoTunnel->TunnelsCritS);
    NetCritEnter(&pProtoTunnel->TunnelsCritR);

    // find tunnel virtual address is bound to
    for (iTunnel = 0, uRemoteAddr = 0; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel++)
    {
        pTunnel = &pProtoTunnel->Tunnels[iTunnel];
        if (pTunnel->uVirtualAddr == uVirtualAddr)
        {
            struct sockaddr SockAddr;
            if ((pBuf != NULL) && (iBufSize >= (signed)sizeof(SockAddr)))
            {
                SockaddrInit(&SockAddr, AF_INET);
                SockaddrInSetAddr(&SockAddr, pTunnel->Info.uRemoteAddr);
                SockaddrInSetPort(&SockAddr, pTunnel->Info.uRemotePort);
                ds_memcpy(pBuf, &SockAddr, sizeof(SockAddr));
            }
            uRemoteAddr = pTunnel->Info.uRemoteAddr;
            break;
        }
    }

    // release exclusive access to tunnel list
    NetCritLeave(&pProtoTunnel->TunnelsCritR);
    NetCritLeave(&pProtoTunnel->TunnelsCritS);

    // return addr to caller
    return(uRemoteAddr);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelServerPortListFunc

    \Description
        Perform a function (add, del, check) on server socket port list

    \Input *pProtoTunnel    - pointer to module state
    \Input uPort            - port value
    \Input ePortOp          - function type (PORTLIST_*)

    \Output
        uint32_t            - true=success, else false

    \Version 10/04/2017 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _ProtoTunnelServerPortListFunc(ProtoTunnelRefT *pProtoTunnel, uint16_t uPort, ProtoTunnelPortListFuncE ePortOp)
{
    const int32_t iNumPorts = sizeof(pProtoTunnel->aServerRemotePortList) / sizeof(pProtoTunnel->aServerRemotePortList[0]);
    uint16_t uCheckVal = (ePortOp != PROTOTUNNEL_PORTLIST_ADD) ? uPort : 0;
    uint32_t bSuccess = FALSE;
    int32_t iPortIdx;

    // find entry
    for (iPortIdx = 0; iPortIdx < iNumPorts; iPortIdx += 1)
    {
        if (pProtoTunnel->aServerRemotePortList[iPortIdx] == uCheckVal)
        {
            break;
        }
    }
    // operate on entry
    if (iPortIdx < iNumPorts)
    {
        switch (ePortOp)
        {
            case PROTOTUNNEL_PORTLIST_DEL:
                uPort = 0;
            case PROTOTUNNEL_PORTLIST_ADD:
                pProtoTunnel->aServerRemotePortList[iPortIdx] = uPort;
            case PROTOTUNNEL_PORTLIST_CHK:
                bSuccess = TRUE;
            default:
                break;
        }
    }
    // return result to caller
    return(bSuccess);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelSocketSendto

    \Description
        Convert a virtual address into its corresponding physical address.

    \Input *pProtoTunnel    - pointer to module state
    \Input *pBuf            - data to send
    \Input iLen             - length of data to send
    \Input iFlags           - flags
    \Input *pTo             - address to send to
    \Input iToLen           - length of address

    \Output
        int32_t             - SocketSendto() result, or SOCKERR_INVALID if no socket

    \Version 10/01/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelSocketSendto(ProtoTunnelRefT *pProtoTunnel, const char *pBuf, int32_t iLen, int32_t iFlags, const struct sockaddr *pTo, int32_t iToLen)
{
    SocketT *pSocket = _ProtoTunnelServerPortListFunc(pProtoTunnel, SockaddrInGetPort(pTo), PROTOTUNNEL_PORTLIST_CHK) ? pProtoTunnel->pServerSocket : pProtoTunnel->pSocket;
    if (pSocket != NULL)
    {
        return(SocketSendto(pSocket, pBuf, iLen, iFlags, pTo, iToLen));
    }
    else
    {
        return(SOCKERR_INVALID);
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelBufferCollect

    \Description
        Collect buffered data in final packet form.

    \Input *pProtoTunnel    - pointer to module state
    \Input *pTunnel         - tunnel to send data on
    \Input iHeadSize        - size of packet headers
    \Input *pPacketData     - [out] buffer for finalized packet
    \Input **ppHeaderDst    - pointer to current header output in finalized packet
    \Input **ppPacketDst    - pointer to current packet data output in finalized packet
    \Input uPortFlag        - flag indicating whether we are collecting encrypted or
                              unencrypted packet data
    \Input *pLimit          - end of buffer, used for debug overflow check

    \Output
        int32_t             - size of packet data added to output buffer

    \Version 12/08/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelBufferCollect(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelT *pTunnel, int32_t iHeadSize, uint8_t *pPacketData, uint8_t **ppHeaderDst, uint8_t **ppPacketDst, uint32_t uPortFlag, uint8_t *pLimit)
{
    int32_t iDataSize, iPacket, iPacketSize;
    uint8_t *pHeaderSrc, *pPacketSrc;
    uint32_t bEnabled;

    // point to packet headers and data
    pHeaderSrc = pTunnel->aPacketData + PROTOTUNNEL_MAXHDROFFS;
    pPacketSrc = pTunnel->aPacketData + PROTOTUNNEL_MAXHDRSIZE + PROTOTUNNEL_MAXHDROFFS;
    iDataSize = 0;

    // collect packets
    for (iPacket = 0; iPacket < pTunnel->iNumPackets; iPacket++)
    {
        // get packet size
        iPacketSize = (pHeaderSrc[0] << 4) | (pHeaderSrc[1] >> 4);

        // if packet encryption matches that specified by the caller, copy it and its header
        bEnabled = (((pTunnel->Info.aPortFlags[pHeaderSrc[1] & 0xf] & (unsigned)PROTOTUNNEL_PORTFLAG_ENCRYPTED) == uPortFlag));
        if (pTunnel->aForceCrypt[iPacket] != 0)
        {
            bEnabled = !bEnabled;
        }
        if (bEnabled)
        {
            // make sure we can fit the packet; this shouldn't happen, because we only buffer packets we can fit in ProtoTunnelBufferData()
            if ((((*ppPacketDst) + iPacketSize) - pLimit) > 0)
            {
                NetPrintf(("prototunnel: [%p][%04d] collect buffer overflow by %d bytes; packet of size %d will be discarded\n",
                    pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), (((*ppPacketDst) + iPacketSize) - pLimit), iPacketSize));
                break;
            }

            // copy packet data and add it to packet buffer
            ds_memcpy(*ppPacketDst, pPacketSrc, iPacketSize);
            (*ppHeaderDst)[0] = pHeaderSrc[0];
            (*ppHeaderDst)[1] = pHeaderSrc[1];
            iDataSize += iPacketSize;
            *ppPacketDst += iPacketSize;
            *ppHeaderDst += PROTOTUNNEL_PKTHDRSIZE;
        }

        // increment to next packet header and packet data
        pHeaderSrc += PROTOTUNNEL_PKTHDRSIZE;
        pPacketSrc += iPacketSize;
    }

    // return size to caller
    return(iDataSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelBufferSend

    \Description
        Send buffered data

    \Input *pProtoTunnel    - pointer to module state
    \Input *pTunnel         - tunnel to send data on
    \Input uCurTick         - current tick count

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoTunnelBufferSend(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelT *pTunnel, uint32_t uCurTick)
{
    uint8_t aPacketData[PROTOTUNNEL_PACKETBUFFER];
    int32_t iCryptSize, iDataSize, iHeadSize, iHshkSize, iResult;
    uint8_t *pHeaderDst, *pPacketDst, *pCrypt;
    uint8_t *pHmac;
    uint16_t uSendHeader = pTunnel->uSendOffset | 0x8000; // add GENERIC bit so we know the protocol type
    struct sockaddr SendAddr;
    #if DIRTYCODE_LOGGING
    int32_t iNumPackets;
    #endif

    // get sole access to packet buffer
    NetCritEnter(&pTunnel->PacketCrit);

    // no data to send or socket to send it on?
    if ((pProtoTunnel->pSocket == NULL) || (pTunnel->iDataSize <= 0))
    {
        NetCritLeave(&pTunnel->PacketCrit);
        return;
    }

    // create send addr
    SockaddrInit(&SendAddr, AF_INET);
    SockaddrInSetAddr(&SendAddr, pTunnel->Info.uRemoteAddr);
    SockaddrInSetPort(&SendAddr, pTunnel->Info.uRemotePort);

    // format packet in local buffer
    ds_memclr(aPacketData, sizeof(aPacketData));

    // get handshake header size and total header size (including sub-packet headers)
    iHshkSize = (pTunnel->bSendCtrlInfo) ? pTunnel->uHandshakeSize : 4;
    iHeadSize = (pTunnel->iNumPackets * PROTOTUNNEL_PKTHDRSIZE) + iHshkSize;

    // set up for copy
    pHeaderDst = aPacketData + iHshkSize;
    pPacketDst = aPacketData + iHeadSize;

    // reserve space for the HMAC; we put it here so it will be encrypted and easy to locate
    pHmac = aPacketData + iHeadSize;
    iCryptSize = pProtoTunnel->uHmacSize;
    ds_memclr(pHmac, iCryptSize);
    pPacketDst += iCryptSize;

    // collect encrypted packets
    iCryptSize += _ProtoTunnelBufferCollect(pProtoTunnel, pTunnel, iHeadSize, aPacketData, &pHeaderDst, &pPacketDst, PROTOTUNNEL_PORTFLAG_ENCRYPTED, aPacketData + PROTOTUNNEL_PACKETBUFFER);

    // collect unencrypted packets
    iDataSize = _ProtoTunnelBufferCollect(pProtoTunnel, pTunnel, iHeadSize, aPacketData, &pHeaderDst, &pPacketDst, 0, aPacketData + PROTOTUNNEL_PACKETBUFFER) + iCryptSize;

    // calculate total encrypted size (encrypted headers + encrypted data, but skip the first two bytes)
    iCryptSize += iHeadSize - iHshkSize;
    // total size of packet
    iDataSize += iHeadSize;

    // write protocol header
    if (pTunnel->bSendCtrlInfo)
    {
        ProtoTunnelHandshakeT Handshake;
        uint32_t uLocalClientId = (pTunnel->uLocalClientId != 0) ? pTunnel->uLocalClientId : pProtoTunnel->uLocalClientId;
        uint16_t uTunnelIndex = pTunnel - pProtoTunnel->Tunnels;
        uint16_t uIdent = pTunnel->Info.uTunnelVers | PROTOTUNNEL_IDENT_HANDSHAKE;

        Handshake.aIdent[0] = (uint8_t)(uIdent >> 8);
        Handshake.aIdent[1] = (uint8_t)(uIdent);
        Handshake.Version.V1_1.aCrypt[0] = 0;
        Handshake.Version.V1_1.aCrypt[1] = 0;
        Handshake.Version.V1_1.aClientIdent[0] = (uint8_t)(uLocalClientId >> 24);
        Handshake.Version.V1_1.aClientIdent[1] = (uint8_t)(uLocalClientId >> 16);
        Handshake.Version.V1_1.aClientIdent[2] = (uint8_t)(uLocalClientId >> 8);
        Handshake.Version.V1_1.aClientIdent[3] = (uint8_t)(uLocalClientId);
        Handshake.Version.V1_1.aConnIdent[0] = (uint8_t)(uTunnelIndex >> 8);
        Handshake.Version.V1_1.aConnIdent[1] = (uint8_t)(uTunnelIndex);
        Handshake.Version.V1_1.uActive = pTunnel->uActive;
        ds_memcpy(aPacketData, &Handshake, sizeof(Handshake));

        pCrypt = ((ProtoTunnelHandshakeT *)aPacketData)->Version.V1_1.aCrypt;
    }
    else
    {
        aPacketData[0] = pTunnel->uConnIdent[0];
        aPacketData[1] = pTunnel->uConnIdent[1];
        pCrypt = aPacketData+2;
    }

    // set tunnel encryption header
    pCrypt[0] = (uint8_t)(uSendHeader >> 8);
    pCrypt[1] = (uint8_t)(uSendHeader >> 0);

    // calculate the hmac and write it into the space reserved for it
    if (pProtoTunnel->uHmacType != CRYPTHASH_NULL)
    {
        CryptHmacCalc(pHmac, pProtoTunnel->uHmacSize, aPacketData, iDataSize, (uint8_t *)pTunnel->aHmacKey, sizeof(pTunnel->aHmacKey), (CryptHashTypeE)pProtoTunnel->uHmacType);
        #if PROTOTUNNEL_HMAC_DEBUG
        NetPrintMem(aPacketData, iDataSize, "send-hmac-data");
        NetPrintMem(pTunnel->aHmacKey, sizeof(pTunnel->aHmacKey), "send-hmac-key");
        NetPrintMem(pHmac, pProtoTunnel->uHmacSize, "send-hmac");
        #endif
    }

    // verbose-only writing of unencrypted payload
    if (pProtoTunnel->iVerbosity > 3)
    {
        NetPrintMem(aPacketData, (iDataSize < 64) ? iDataSize : 64, "prototunnel-send-nocrypt");
    }
    // encrypt payload and advance stream if necessary
    CryptArc4Apply(&pTunnel->CryptSendState, aPacketData + iHshkSize, iCryptSize);
    _ProtoTunnelStreamAdvance(&pTunnel->CryptSendState, &pTunnel->uSendOffset, iCryptSize);

    // accumulate stats
    pTunnel->SendStat.uLastPacketTime = uCurTick;
    pTunnel->SendStat.uNumBytes += iDataSize;
    pTunnel->SendStat.uNumSubpacketBytes += iDataSize - iHeadSize - pProtoTunnel->uHmacSize;
    pTunnel->SendStat.uNumPackets += 1;
    pTunnel->SendStat.uNumSubpackets += pTunnel->iNumPackets + pTunnel->bSendCtrlInfo;

    #if DIRTYCODE_LOGGING
    iNumPackets = pTunnel->iNumPackets + pTunnel->bSendCtrlInfo;
    #endif

    // mark data as sent
    pTunnel->iBuffSize = 0;
    pTunnel->iDataSize = 0;
    pTunnel->iNumPackets = 0;
    ds_memclr(pTunnel->aForceCrypt, sizeof(pTunnel->aForceCrypt));

    // release packet buffer critical section
    NetCritLeave(&pTunnel->PacketCrit);

    #if DIRTYCODE_LOGGING
    if (NetTickDiff(uCurTick, pTunnel->uLastStatUpdate) > 5000)
    {
        ProtoTunnelStatT DiffStat;
        DiffStat.uNumBytes = pTunnel->SendStat.uNumBytes - pTunnel->LastSendStat.uNumBytes;
        DiffStat.uNumSubpacketBytes = pTunnel->SendStat.uNumSubpacketBytes - pTunnel->LastSendStat.uNumSubpacketBytes;
        DiffStat.uNumPackets = pTunnel->SendStat.uNumPackets - pTunnel->LastSendStat.uNumPackets;
        DiffStat.uNumSubpackets = pTunnel->SendStat.uNumSubpackets - pTunnel->LastSendStat.uNumSubpackets;
        ds_memcpy_s(&pTunnel->LastSendStat, sizeof(pTunnel->LastSendStat), &pTunnel->SendStat, sizeof(pTunnel->SendStat));
        NetPrintfVerbose((pProtoTunnel->iVerbosity, 2, "prototunnel: [%p][%04d] pkts: %d eff: %.2f sent: %d eff: %.2f\n",
            pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels),
            DiffStat.uNumPackets, (float)DiffStat.uNumSubpackets / (float)DiffStat.uNumPackets,
            DiffStat.uNumBytes, (float)DiffStat.uNumSubpacketBytes / (float)DiffStat.uNumBytes));
        pTunnel->uLastStatUpdate = uCurTick;
    }
    if (pProtoTunnel->iVerbosity > 3)
    {
        NetPrintMem(aPacketData, (iDataSize < 64) ? iDataSize : 64, "prototunnel-send");
    }
    #endif

    // send the data
    if ((iResult = _ProtoTunnelSocketSendto(pProtoTunnel, (char *)aPacketData, iDataSize, 0, &SendAddr, sizeof(SendAddr))) < 0)
    {
        NetPrintf(("prototunnel: [%p][%04d] send - error %d sending buffered packet to %a:%d\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels),
            iResult, SockaddrInGetAddr(&SendAddr), SockaddrInGetPort(&SendAddr)));
    }
    else
    {
        NetPrintfVerbose((pProtoTunnel->iVerbosity, 2, "prototunnel: [%p][%04d] send - sent %d bytes [%d packets] to %a:%d\n",
            pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), iDataSize, iNumPackets, SockaddrInGetAddr(&SendAddr), SockaddrInGetPort(&SendAddr)));
    }

    pTunnel->uLastTunnelSend = NetTick();
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelBufferData

    \Description
        Buffer a send

    \Input *pProtoTunnel    - pointer to module state
    \Input *pTunnel         - tunnel to buffer data on
    \Input *pData           - data to be sent
    \Input uSize            - size of data to send
    \Input uPortIdx         - index of port in tunnel map
    \Input bForceCrypt      - TRUE to force encryption of packet, else FALSE

    \Output
        int32_t             - amount of data buffered, or socket error code (SOCKERR_XXX)

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelBufferData(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelT *pTunnel, const uint8_t *pData, uint32_t uSize, uint32_t uPortIdx, uint8_t bForceCrypt)
{
    uint32_t uCurTick = NetTick();
    uint8_t *pBuffer;

    // make sure data is within size limits
    if ((uSize == 0) || (uSize > PROTOTUNNEL_MAXPACKET))
    {
        NetPrintf(("prototunnel: [%p][%04d] packet of size %d will not be tunneled (max size = %d)\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), uSize, PROTOTUNNEL_MAXPACKET));
        NetPrintMem(pData, 64, "prototunnel-nobuf");
        return(SOCKERR_NORSRC);
    }

    // acquire packet buffer critical section
    NetCritEnter(&pTunnel->PacketCrit);

    // if buffer is full, or we've already buffered max packets, flush buffer
    if ((((unsigned)pTunnel->iDataSize + uSize + PROTOTUNNEL_PKTHDRSIZE) > PROTOTUNNEL_PACKETBUFFER) ||
         (pTunnel->iNumPackets == PROTOTUNNEL_MAXPACKETS))
    {
        // flush current packet
        _ProtoTunnelBufferSend(pProtoTunnel, pTunnel, uCurTick);
    }

    // if buffer is empty, reserve space for max header data (1.1 handshake + max sub-packet headers)
    if (pTunnel->iBuffSize == 0)
    {
        pTunnel->iBuffSize = PROTOTUNNEL_MAXHDROFFS + PROTOTUNNEL_MAXHDRSIZE;
        pTunnel->iDataSize = PROTOTUNNEL_MAXHDROFFS + pProtoTunnel->uHmacSize;
        ds_memclr(pTunnel->aPacketData, pTunnel->iBuffSize);
    }

    // store packet header
    pBuffer = pTunnel->aPacketData + (pTunnel->iNumPackets * PROTOTUNNEL_PKTHDRSIZE) + PROTOTUNNEL_MAXHDROFFS;
    pBuffer[0] = (uint8_t)(uSize >> 4);
    pBuffer[1] = (uint8_t)((uSize << 4) | uPortIdx);

    // store packet data
    pBuffer = pTunnel->aPacketData + pTunnel->iBuffSize;
    ds_memcpy(pBuffer, pData, uSize);
    pTunnel->iBuffSize += uSize;
    pTunnel->iDataSize += uSize + PROTOTUNNEL_PKTHDRSIZE;
    pTunnel->aForceCrypt[(uint32_t)pTunnel->iNumPackets] = bForceCrypt;

    // update overall count
    pTunnel->iNumPackets += 1;

    if (pTunnel->Info.aPortFlags[uPortIdx] & PROTOTUNNEL_PORTFLAG_AUTOFLUSH)
    {
        _ProtoTunnelBufferSend(pProtoTunnel, pTunnel, uCurTick);
    }

    // release packet buffer critical section
    NetCritLeave(&pTunnel->PacketCrit);

    // return buffered size to caller
    return(uSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelSendCallback

    \Description
        Global send callback handler.

    \Input *pSocket     - socket to send on
    \Input iType        - socket type
    \Input *pData       - data to be sent
    \Input iDataSize    - size of data to send
    \Input *pTo         - destination address
    \Input *pCallref    - prototunnel ref

    \Output
        int32_t         - 0 = send not handled; >0 = send successfully handled (bytes sent); <0 = send handled but failed (SOCKERR_XXX)

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelSendCallback(SocketT *pSocket, int32_t iType, const uint8_t *pData, int32_t iDataSize, const struct sockaddr *pTo, void *pCallref)
{
    ProtoTunnelRefT *pProtoTunnel = (ProtoTunnelRefT *)pCallref;
    ProtoTunnelT *pTunnel = NULL;
    uint32_t uAddr, uPort;
    int32_t iPort = 0, iTunnel;
    int32_t iResult = 0; // default to "not handled"
    uint32_t bFound, bForceCrypt=0;
    #if DIRTYCODE_LOGGING
    uint32_t bFoundAddr = FALSE;
    #endif
    struct sockaddr SockAddr;

    // only handle dgram sockets, and don't handle our own socket
    if ((iType != SOCK_DGRAM) || (pProtoTunnel->pSocket == pSocket) || (pProtoTunnel->pServerSocket == pSocket))
    {
        return(0);  // not handled
    }

    // if a destination address is not specified, get it from the socket
    if (pTo == NULL)
    {
        SocketInfo(pSocket, 'peer', 0, &SockAddr, sizeof(SockAddr));
        pTo = &SockAddr;
    }

    // get destination address and port
    uAddr = SockaddrInGetAddr(pTo);
    uPort = SockaddrInGetPort(pTo);

    // ignore sends to an invalid destination
    if ((uAddr == 0) || (uPort == 0))
    {
        return(0);  // not handled
    }

    // get exclusive access to tunnel list
    NetCritEnter(&pProtoTunnel->TunnelsCritS);

    // see if we have a match
    for (iTunnel = 0, bFound = FALSE; (iTunnel < pProtoTunnel->iMaxTunnels) && (bFound == FALSE); iTunnel++)
    {
        pTunnel = &pProtoTunnel->Tunnels[iTunnel];

        // does virtual address match?
        if (pTunnel->uVirtualAddr != uAddr)
        {
            continue;
        }
        #if DIRTYCODE_LOGGING
        // remember if we have a tunnel to this address
        bFoundAddr = TRUE;
        #endif
        // see if we have a port match or are at the end of our port list
        for (iPort = 0; iPort < PROTOTUNNEL_MAXPORTS; iPort++)
        {
            uint32_t uPort2 = pTunnel->Info.aRemotePortList[iPort];
            if (uPort2 == uPort)
            {
                bFound = TRUE;
                break;
            }
        }
    }

    // found a match?
    if (bFound == TRUE)
    {
        // if we have a match, queue data
        iResult = _ProtoTunnelBufferData(pProtoTunnel, pTunnel, pData, iDataSize, iPort, bForceCrypt);
    }
    else
    {
        NetPrintfVerbose((pProtoTunnel->iVerbosity, 1, "prototunnel: [%p] send - no match for data sent to %a:%d (%s mismatch)\n", pProtoTunnel, uAddr, uPort, bFoundAddr ? "port" : "addr"));
    }

    // release exclusive access to tunnel list and return result
    NetCritLeave(&pProtoTunnel->TunnelsCritS);
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelDecryptAndValidatePacket2

    \Description
        Decrypt the tunnel packet

    \Input *pProtoTunnel - pointer to module state
    \Input *pTunnel      - tunnel
    \Input *pHeadOffset  - [out] offset of sub-packet header data
    \Input *pPacketData  - pointer to tunnel packet
    \Input iRecvLen      - size of tunnel packet
    \Input bUpdateState  - TRUE to update crypto state, else FALSE
    \Input bCurrentState - TRUE when decrypting against current state, FALSE against oop state

    \Output
        int32_t          - PROTOTUNNEL_PACKETRECVFAIL_* on error, else number of subpackets decoded

    \Version 12/07/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelDecryptAndValidatePacket2(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelT *pTunnel, uint32_t *pHeadOffset, uint8_t *pPacketData, int32_t iRecvLen, uint8_t bUpdateState, uint8_t bCurrentState)
{
    int32_t iEncryptedSize, iNumPackets, iPacketOffset, iPacketSize, iHdrCopySize;
    uint8_t aPacketHeader[PROTOTUNNEL_MAXHDRSIZE+PROTOTUNNEL_MAXHDROFFS];
    uint8_t *pPacketStart = pPacketData;
    uint32_t uRecvOffset, uRecvOffsetState;
    CryptArc4T CryptRecvState, *pCryptRecvState;
    uint16_t *pRecvOffsetState;
    uint16_t uIdent, uVersion;
    uint8_t bSynced = FALSE;
    uint32_t uHeadOffset;

    // make a copy of packet header before doing anything else, in case we need to restore it after a decrypt failure
    iHdrCopySize = DS_MIN(sizeof(aPacketHeader), (unsigned)iRecvLen);
    ds_memcpy(aPacketHeader, pPacketData, iHdrCopySize);

    // get reference to crypt state based on whether we are decoding the current state or oop state
    pCryptRecvState = (bCurrentState) ? &pTunnel->CryptRecvState : &pTunnel->CryptRecvOOPState;
    pRecvOffsetState = (bCurrentState) ? &pTunnel->uRecvOffset : &pTunnel->uRecvOOPOffset;

    // get stream offset from packet header - this tells us where we need to be to be able to decrypt the packet
    uRecvOffset = PROTOTUNNEL_GetStreamOffsetFromPacket(pTunnel->Info.uTunnelVers, pPacketData);

    // work on a local copy of the crypt state and stream offset, in case we need to throw this packet out
    ds_memcpy_s(&CryptRecvState, sizeof(CryptRecvState), pCryptRecvState, sizeof(*pCryptRecvState));
    uRecvOffsetState = *pRecvOffsetState;

    // lost data?  sync the cipher
    if (uRecvOffset != uRecvOffsetState)
    {
        // calc how many data units we've missed
        int16_t iRecvOffset = (signed)uRecvOffset;
        int16_t iTunlOffset = (signed)uRecvOffsetState;
        // calc 15bit offset
        int16_t iRecvDiff = (iRecvOffset - iTunlOffset) & 0x7fff;
        /* sign extend our 15bit offset - we do it this way because the simpler way of shifting left and
           back again right relies on the right shift sign-extending the value, however in the C standard
           it is implementation-dependent whether the shift is treated as signed or unsigned */
        iRecvDiff |= (iRecvDiff & 16384) << 1;

        /* check to see if packet is previous to our current window offset; we call this an out-of-order packet and handle it
           differently from a skipped packet, which is ahead of our current window offset.  this distinction is made because
           it is trivial to advance the state, but it cannot be rolled back */
        if (iRecvDiff < 0)
        {
            NetPrintf(("prototunnel: [%p][%04d] received out of order packet (off=%d)\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), iRecvDiff));
            return(PROTOTUNNEL_PACKETRECVFAIL_OUTOFORDER);
        }

        // advance the cipher to resync
        NetPrintf(("prototunnel: [%p][%04d] stream skip %d bytes\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), iRecvDiff<<PROTOTUNNEL_CRYPTBITS));
        CryptArc4Advance(&CryptRecvState, iRecvDiff<<PROTOTUNNEL_CRYPTBITS);
        uRecvOffsetState = uRecvOffset;
        // remember we synced the state
        bSynced = TRUE;
    }

    // reset offset to count bytes added from this packet
    uRecvOffset = 0;

    // get ident
    uIdent = PROTOTUNNEL_GetIdentFromPacket(pPacketData);

    // does the packet include handshake data?
    if (uIdent & PROTOTUNNEL_IDENT_HANDSHAKE)
    {
        ProtoTunnelHandshakeT Handshake;

        uVersion = uIdent & ~PROTOTUNNEL_IDENT_HANDSHAKE;
        NetPrintf(("prototunnel: [%p][%04d] handshake packet version %d.%d\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), uVersion>>8, uVersion&0xff));
        if (uVersion != pTunnel->Info.uTunnelVers)
        {
            NetPrintf(("prototunnel: [%p][%04d] version mismatch; received %d.%d but expected %d.%d\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), uVersion>>8, uVersion&0xff,
                pTunnel->Info.uTunnelVers>>8, pTunnel->Info.uTunnelVers&0xff));
            if ((uVersion > pTunnel->Info.uTunnelVers) || (uVersion < PROTOTUNNEL_VERSION_1_1))
            {
                NetPrintf(("prototunnel: [%p][%04d] can't connect with protocol version %d.%d\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), uVersion>>8, uVersion&0xff));
                return(PROTOTUNNEL_PACKETRECVFAIL_VERSION);
            }
        }

        ds_memcpy(&Handshake, pPacketData, sizeof(Handshake));
        pTunnel->uConnIdent[0] = Handshake.Version.V1_1.aConnIdent[0];
        pTunnel->uConnIdent[1] = Handshake.Version.V1_1.aConnIdent[1];
        NetPrintf(("prototunnel: [%p][%04d] setting connection ident to %d\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), (pTunnel->uConnIdent[0] << 8) | pTunnel->uConnIdent[1]));

        uHeadOffset = pTunnel->uHandshakeSize;
    }
    else
    {
        uVersion = pTunnel->Info.uTunnelVers;
        uHeadOffset = 4;
    }

    // skip header
    pPacketData += uHeadOffset;
    iPacketOffset = uHeadOffset;
    // save sub-packet header offset for later use
    *pHeadOffset = uHeadOffset;

    /* subtract out hmac size while we decrypt packet headers and check overall
        packet size, as hmac is not included in that calculation */
    iRecvLen -= pProtoTunnel->uHmacSize;

    // iterate through packet headers
    for (iNumPackets = 0, iEncryptedSize = 0; iPacketOffset < iRecvLen; iNumPackets++)
    {
        // decrypt the packet header
        CryptArc4Apply(&CryptRecvState, pPacketData, PROTOTUNNEL_PKTHDRSIZE);
        uRecvOffset += PROTOTUNNEL_PKTHDRSIZE;

        // extract size and port info
        iPacketSize = (pPacketData[0] << 4) | (pPacketData[1] >> 4);
        iPacketOffset += iPacketSize + PROTOTUNNEL_PKTHDRSIZE;
        if (pTunnel->Info.aPortFlags[pPacketData[1] & 0xf] & PROTOTUNNEL_PORTFLAG_ENCRYPTED)
        {
            iEncryptedSize += iPacketSize;
        }

        // increment to next packet header
        pPacketData += PROTOTUNNEL_PKTHDRSIZE;
    }

    // make sure size matches
    if (iPacketOffset != iRecvLen)
    {
        NetPrintf(("prototunnel: [%p][%04d] recv - mismatched size in received packet (%d received, %d decoded)\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), iRecvLen+pProtoTunnel->uHmacSize, iPacketOffset));
        // restore original packet header
        ds_memcpy(pPacketStart, aPacketHeader, iHdrCopySize);
        return(PROTOTUNNEL_PACKETRECVFAIL_VALIDATE);
    }

    // restore hmac to packet size, and add hmac to size of data to be decrypted
    iRecvLen += pProtoTunnel->uHmacSize;
    iEncryptedSize += pProtoTunnel->uHmacSize;

    // decrypt encrypted packet data
    if (iEncryptedSize > 0)
    {
        CryptArc4Apply(&CryptRecvState, pPacketData, iEncryptedSize);
        uRecvOffset += iEncryptedSize;
    }

    // calculate HMAC and compare with what we received
    if (pProtoTunnel->uHmacType != CRYPTHASH_NULL)
    {
        uint8_t aPackHmac[PROTOTUNNEL_HMAC_MAXSIZE], aCalcHmac[PROTOTUNNEL_HMAC_MAXSIZE];
        // make a copy of HMAC
        ds_memcpy_s(aPackHmac, sizeof(aPackHmac), pPacketData, pProtoTunnel->uHmacSize);
        // clear HMAC payload in packet to zero so we can calculate the HMAC ourselves
        ds_memclr(pPacketData, pProtoTunnel->uHmacSize);
        // calculate HMAC
        CryptHmacCalc(aCalcHmac, pProtoTunnel->uHmacSize, pPacketStart, iRecvLen, (uint8_t *)pTunnel->aHmacKey, sizeof(pTunnel->aHmacKey), (CryptHashTypeE)pProtoTunnel->uHmacType);
        // validate
        if (memcmp(aPackHmac, aCalcHmac, pProtoTunnel->uHmacSize))
        {
            NetPrintf(("prototunnel: [%p][%04d] bad HMAC!\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels)));
            #if PROTOTUNNEL_HMAC_DEBUG
            NetPrintMem(pPacketData, pProtoTunnel->uHmacSize, "recv-hmac");
            NetPrintMem(pPacketStart, iRecvLen, "recv-hmac-data");
            NetPrintMem(pTunnel->aHmacKey, sizeof(pTunnel->aHmacKey), "recv-hmac-key");
            #endif
            return(PROTOTUNNEL_PACKETRECVFAIL_VALIDATE);
        }
        else
        {
            #if PROTOTUNNEL_HMAC_DEBUG
            NetPrintf(("prototunnel: [%p][%04d] recv - hmac validated\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels)));
            #endif
        }
    }

    // update crypt state and stream offset with working temp copies
    if (bUpdateState)
    {
        // if we had to sync the cipher, save current state to oop state before advancing for possible future out-of-order packet decoding
        if (bCurrentState && bSynced)
        {
            NetPrintf(("prototunnel: [%p][%04d] saving out-of-order state\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels)));
            ds_memcpy_s(&pTunnel->CryptRecvOOPState, sizeof(pTunnel->CryptRecvOOPState), pCryptRecvState, sizeof(*pCryptRecvState));
            pTunnel->uRecvOOPOffset = *pRecvOffsetState;
        }
        // now save advanced state
        ds_memcpy_s(pCryptRecvState, sizeof(*pCryptRecvState), &CryptRecvState, sizeof(CryptRecvState));
        *pRecvOffsetState = (uint16_t)uRecvOffsetState;
        // advance stream by received encrypted packet data size, plus padding if necessary
        _ProtoTunnelStreamAdvance(pCryptRecvState, pRecvOffsetState, uRecvOffset);
    }

    // if we downgraded protocol version, update here
    if (pTunnel->Info.uTunnelVers != uVersion)
    {
        NetPrintf(("prototunnel: [%p][%04d] downgrading to version %d.%d\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), uVersion>>8, uVersion&0xff));
        _ProtoTunnelSetVersion(pProtoTunnel, pTunnel, uVersion);
    }

    // return number of packets decoded
    return(iNumPackets);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelDecryptAndValidatePacket

    \Description
        Decrypt the tunnel packet, with limited out-of-order packet recovery

    \Input *pProtoTunnel - pointer to module state
    \Input *pTunnel      - tunnel
    \Input *pHeadOffset  - [out] offset of sub-packet header data
    \Input *pPacketData  - pointer to tunnel packet
    \Input iRecvLen      - size of tunnel packet
    \Input bUpdateState  - TRUE to update crypt state, else FALSE

    \Output
        int32_t          - PROTOTUNNEL_PACKETRECVFAIL_* on error, else number of subpackets decoded

    \Notes
        If an out-of-order packet is detected against the current tunnel decrypt
        state, a single attempt is made to recover the packet using the out-of-order
        decrypt state.

    \Version 02/21/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelDecryptAndValidatePacket(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelT *pTunnel, uint32_t *pHeadOffset, uint8_t *pPacketData, int32_t iRecvLen, uint8_t bUpdateState)
{
    int32_t iResult;
    // try decrypting packet with current state
    if ((iResult = _ProtoTunnelDecryptAndValidatePacket2(pProtoTunnel, pTunnel, pHeadOffset, pPacketData, iRecvLen, bUpdateState, TRUE)) == PROTOTUNNEL_PACKETRECVFAIL_OUTOFORDER)
    {
        // received out-of-order packet prior to our current state; try again with out-of-order packet state
        if ((iResult = _ProtoTunnelDecryptAndValidatePacket2(pProtoTunnel, pTunnel, pHeadOffset, pPacketData, iRecvLen, bUpdateState, FALSE)) > 0)
        {
            NetPrintf(("prototunnel: [%p][%04d] out of order packet recovered by oop state\n", pProtoTunnel, pTunnel - pProtoTunnel->Tunnels));
        }
    }
    // return result
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelGetControlInfo

    \Description
        Get control info from handshake

    \Input *pTunnel                 - tunnel packet was received on
    \Input *pControlInfo            - [out] control info from packet data
    \Input *pPacketData             - source data

    \Output
        uint32_t                    - non-zero if control info was found, else zero

    \Version 03/26/2015 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _ProtoTunnelGetControlInfo(ProtoTunnelT *pTunnel, ProtoTunnelControlT *pControlInfo, const uint8_t *pPacketData)
{
    uint32_t bFound = FALSE;

    // get ident
    uint16_t uIdent = PROTOTUNNEL_GetIdentFromPacket(pPacketData);

    // get control info, if present
    if (uIdent & PROTOTUNNEL_IDENT_HANDSHAKE)
    {
        ProtoTunnelHandshakeT *pHandshake = (ProtoTunnelHandshakeT *)pPacketData;
        pControlInfo->uPacketType = PROTOTUNNEL_CTRL_INIT;
        pControlInfo->aProtocolVers[0] = pHandshake->aIdent[0] & ~0x80;
        pControlInfo->aProtocolVers[1] = pHandshake->aIdent[1];
        pControlInfo->aClientId[0] = pHandshake->Version.V1_1.aClientIdent[0];
        pControlInfo->aClientId[1] = pHandshake->Version.V1_1.aClientIdent[1];
        pControlInfo->aClientId[2] = pHandshake->Version.V1_1.aClientIdent[2];
        pControlInfo->aClientId[3] = pHandshake->Version.V1_1.aClientIdent[3];
        pControlInfo->uActive = pHandshake->Version.V1_1.uActive;
        bFound = TRUE;
    }
    else
    {
        ds_memclr(pControlInfo, sizeof(*pControlInfo));
    }

    return(bFound);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelCryptSetup

    \Description
        Setup crypto state for key index iKey

    \Input *pTunnel         - tunnel to setup crypto state for
    \Input iKey             - index of key to use to setup state
    \Input bForceSetup      - force setup

    \Version 02/24/2016 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoTunnelCryptSetup(ProtoTunnelT *pTunnel, int32_t iKey, uint8_t bForceSetup)
{
    uint8_t *pKey = (uint8_t *)pTunnel->aKeyList[iKey];
    int32_t iKeyLen = (int32_t)strlen((char *)pKey);

    // only setup if we haven't already, or if forced
    if ((pTunnel->uSendKey == (uint8_t)iKey) && (bForceSetup != TRUE))
    {
        return;
    }

    // initialize HMAC key (shared for both send and recv) by running IV through RC4 initialized with tunnel key
    CryptArc4Init(&pTunnel->CryptSendState, pKey, iKeyLen, PROTOTUNNEL_CRYPTARC4_ITER);
    ds_memcpy_s(pTunnel->aHmacKey, sizeof(pTunnel->aHmacKey), _ProtoTunnel_aHmacInitVec, sizeof(_ProtoTunnel_aHmacInitVec));
    CryptArc4Apply(&pTunnel->CryptSendState, (uint8_t *)pTunnel->aHmacKey, sizeof(pTunnel->aHmacKey));

    // initialize send/recv crypto state
    CryptArc4Init(&pTunnel->CryptSendState, pKey, iKeyLen, PROTOTUNNEL_CRYPTARC4_ITER);
    CryptArc4Init(&pTunnel->CryptRecvState, pKey, iKeyLen, PROTOTUNNEL_CRYPTARC4_ITER);
    pTunnel->uSendKey = pTunnel->uRecvKey = (uint8_t)iKey;
    pTunnel->uSendOffset = pTunnel->uRecvOffset = 0;
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelMatchTunnel

    \Description
        Attempt to decrypt incoming data against a tunnel, and check for clientId match

    \Input *pProtoTunnel    - module state
    \Input *pTunnel         - tunnel to check data against
    \Input *pPacketData     - source data
    \Input iPacketSize      - size of source data

    \Output
        int32_t             - FALSE=no match, TRUE=match

    \Version 06/26/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelMatchTunnel(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelT *pTunnel, const uint8_t *pPacketData, int32_t iPacketSize)
{
    uint8_t aPacketData[PROTOTUNNEL_PACKETBUFFER];
    int32_t iNumPackets, iRecvKey;
    ProtoTunnelControlT ControlInfo;
    uint32_t uClientId;

    // try any non-empty keys in this tunnel
    for (iNumPackets = -1, iRecvKey = 0; iRecvKey < PROTOTUNNEL_MAXKEYS; iRecvKey += 1)
    {
        // skip empty keys
        if (pTunnel->aKeyList[iRecvKey][0] == '\0')
        {
            continue;
        }

        // try and validate packet against this key with a zero stream offset
        if ((iNumPackets = ProtoTunnelValidatePacket(pProtoTunnel, pTunnel, aPacketData, pPacketData, iPacketSize, pTunnel->aKeyList[iRecvKey])) > 0)
        {
            break;
        }
    }

    // if we couldn't successfully decode the packet, we return no match
    if (iNumPackets < 0)
    {
        return(FALSE);
    }

    // if we're rematching an active tunnel, we're done
    if (pTunnel->uActive != 0)
    {
        // reset crypt state, matching what ProtoTunnelValidatePacket did to successfully decrypt the packet
        _ProtoTunnelCryptSetup(pTunnel, iRecvKey, TRUE);
        return(TRUE);
    }

    // get packet control info
    if (_ProtoTunnelGetControlInfo(pTunnel, &ControlInfo, aPacketData) != 0)
    {
        uClientId  = ControlInfo.aClientId[0] << 24;
        uClientId |= ControlInfo.aClientId[1] << 16;
        uClientId |= ControlInfo.aClientId[2] << 8;
        uClientId |= ControlInfo.aClientId[3];
    }
    else
    {
        NetPrintf(("prototunnel: [%p][%04d] no control data included in packet on inactive tunnel\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels)));
        return(FALSE);
    }

    // if we found the matching tunnel, we're done
    if (uClientId == pTunnel->Info.uRemoteClientId)
    {
        #if DIRTYCODE_LOGGING
        if (iRecvKey != 0)
        {
            NetPrintf(("prototunnel: [%p][%04d] matched key with index=%d\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), iRecvKey));
        }
        #endif
        _ProtoTunnelCryptSetup(pTunnel, iRecvKey, FALSE);
        return(TRUE);
    }
    else
    {
        NetPrintf(("prototunnel: [%p][%04d] packet clientId 0x%08x does not match tunnel clientId 0x%08x\n",
            pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), uClientId, pTunnel->Info.uRemoteClientId));
        if (pProtoTunnel->iVerbosity > 3)
        {
            NetPrintMem(aPacketData, (iPacketSize < 64) ? iPacketSize : 64, "prototunnel-recv-decrypt");
        }
    }

    // no match
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelFindTunnel

    \Description
        Find the tunnel endpoint for incoming packet

    \Input *pProtoTunnel    - module state
    \Input *pPacketData     - packet data
    \Input iRecvLen         - packet size
    \Input uRecvAddr        - source addr of packet
    \Input uRecvPort        - source port of packet

    \Output
        int32_t             - matching tunnel index; iMaxTunnels means no match

    \Version 03/26/2015 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelFindTunnel(ProtoTunnelRefT *pProtoTunnel, uint8_t *pPacketData, int32_t iRecvLen, uint32_t uRecvAddr, uint16_t uRecvPort)
{
    int32_t iTunnel = pProtoTunnel->iMaxTunnels;
    ProtoTunnelT *pTunnel = NULL;
    uint32_t uClientId;
    uint16_t uIdent = PROTOTUNNEL_GetIdentFromPacket(pPacketData);

    if (uIdent & PROTOTUNNEL_IDENT_HANDSHAKE)
    {
        ProtoTunnelHandshakeT *pHandshake = (ProtoTunnelHandshakeT *)pPacketData;
        uClientId  = pHandshake->Version.V1_1.aClientIdent[0] << 24;
        uClientId |= pHandshake->Version.V1_1.aClientIdent[1] << 16;
        uClientId |= pHandshake->Version.V1_1.aClientIdent[2] << 8;
        uClientId |= pHandshake->Version.V1_1.aClientIdent[3];

        // find tunnel with matching clientId
        for (iTunnel = 0; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel += 1)
        {
            pTunnel = &pProtoTunnel->Tunnels[iTunnel];
            if (pTunnel->Info.uRemoteClientId == uClientId)
            {
                if (pTunnel->uActive == 0)
                {
                    // decrypt and validate packet
                    if (!_ProtoTunnelMatchTunnel(pProtoTunnel, pTunnel, pPacketData, iRecvLen))
                    {
                        NetPrintf(("prototunnel: [%p][%04d] error; validate failed on match of uClientId=0x%08x to inactive tunnel\n", pProtoTunnel, iTunnel, uClientId));
                        iTunnel = pProtoTunnel->iMaxTunnels;
                    }
                }
                else
                {
                    NetPrintf(("prototunnel: [%p][%04d] warning; matching uClientId=0x%08x to active tunnel\n", pProtoTunnel, iTunnel, uClientId));
                }
                break;
            }
        }
    }
    else if (uIdent < (unsigned)pProtoTunnel->iMaxTunnels)
    {
        // a non-handshake ident field contains the local tunnel index exchanged in handshaking
        pTunnel = &pProtoTunnel->Tunnels[uIdent];
        if (pTunnel->uActive == 1)
        {
            // decrypt and validate packet
            if (_ProtoTunnelMatchTunnel(pProtoTunnel, pTunnel, pPacketData, iRecvLen) != 0)
            {
                // found tunnel to match incoming data with
                iTunnel = (signed)uIdent;
            }
        }
    }

    // handle port change
    if ((iTunnel < pProtoTunnel->iMaxTunnels) && (pTunnel != NULL) && (pTunnel->Info.uRemotePort != uRecvPort))
    {
        NetPrintf(("prototunnel: [%p][%04d] detected port change; updating remote port from %d to %d\n", pProtoTunnel, iTunnel, pTunnel->Info.uRemotePort, uRecvPort));
        pTunnel->Info.uRemotePort = uRecvPort;
    }

    // couldn't match?
    if (iTunnel == pProtoTunnel->iMaxTunnels)
    {
        NetPrintf(("prototunnel: [%p] could not match packet from %a:%d (ident=0x%04x)\n", pProtoTunnel, uRecvAddr, uRecvPort, uIdent));
    }
    return(iTunnel);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelRecvData

    \Description
        Match incoming data with a tunnel, and if source address is mapped,
        decode the number of subpackets and decrypt packet data.

    \Input *pProtoTunnel    - module state
    \Input *pRemotePortList - [out] storage for remote port list of matching tunnel
    \Input iPortListSize    - size of port list
    \Input *pHeadOffset     - [out] storage for sub-packet header data offset
    \Input *pPacketData     - source data
    \Input iRecvLen         - size of source data
    \Input *pRecvAddr       - source address

    \Output
        int32_t             - PROTOTUNNEL_PACKETRECVFAIL_*, else number of decoded packets

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelRecvData(ProtoTunnelRefT *pProtoTunnel, uint16_t *pRemotePortList, int32_t iPortListSize, uint32_t *pHeadOffset, uint8_t *pPacketData, int32_t iRecvLen, struct sockaddr *pRecvAddr)
{
    uint32_t uRecvAddr, uRecvPort;
    int32_t iNumPackets, iTunnel;
    ProtoTunnelT *pTunnel;
    uint32_t uCurTick = NetTick();

    // get exclusive access to tunnel list
    NetCritEnter(&pProtoTunnel->TunnelsCritR);

    // find tunnel with matching remote address, port, and mode
    for (iTunnel = 0, uRecvAddr = SockaddrInGetAddr(pRecvAddr), uRecvPort = SockaddrInGetPort(pRecvAddr); iTunnel < pProtoTunnel->iMaxTunnels; iTunnel++)
    {
        pTunnel = &pProtoTunnel->Tunnels[iTunnel];
        if ((pTunnel->Info.uRemoteAddr == uRecvAddr) && (pTunnel->Info.uRemotePort == uRecvPort))
        {
            if (pTunnel->uActive == 0)
            {
                // decrypt packet and verify clientId matches
                if (_ProtoTunnelMatchTunnel(pProtoTunnel, pTunnel, pPacketData, iRecvLen))
                {
                    // found tunnel to match incoming data with
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    // if no matching tunnel found, use packet data to match to a tunnel
    if (iTunnel == pProtoTunnel->iMaxTunnels)
    {
        iTunnel = _ProtoTunnelFindTunnel(pProtoTunnel, pPacketData, iRecvLen, uRecvAddr, uRecvPort);
    }

    // did we find a tunnel?
    if (iTunnel < pProtoTunnel->iMaxTunnels)
    {
        pTunnel = &pProtoTunnel->Tunnels[iTunnel];

        // if the tunnel isn't active yet, bind to it and mark it as active
        if (pTunnel->uActive == 0)
        {
            if ((pTunnel->Info.uRemoteAddr != uRecvAddr) || (pTunnel->Info.uRemotePort != uRecvPort))
            {
                NetPrintf(("prototunnel: [%p][%04d] updating remote address from %a:%d to %a:%d\n",
                    pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), pTunnel->Info.uRemoteAddr, pTunnel->Info.uRemotePort, uRecvAddr, uRecvPort));
            }

            // update tunnel to addr/port combo
            pTunnel->Info.uRemoteAddr = uRecvAddr;
            pTunnel->Info.uRemotePort = uRecvPort;

            // mark that we're now active
            pTunnel->uActive = 1;

            NetPrintf(("prototunnel: [%p][%04d] matched incoming data from %a:%d to tunnelId 0x%08x / clientId 0x%08x\n",
                pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), uRecvAddr, uRecvPort, pTunnel->uVirtualAddr, pTunnel->Info.uRemoteClientId));
        }

        // make a copy of port list for later use
        ds_memcpy(pRemotePortList, pTunnel->Info.aRemotePortList, iPortListSize);

        // display recv info
        NetPrintfVerbose((pProtoTunnel->iVerbosity, 2, "prototunnel: [%p][%04d] recv - received %d bytes on tunnel 0x%08x from %a:%d\n",
            pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), iRecvLen, pTunnel->uVirtualAddr, uRecvAddr, uRecvPort));

        iNumPackets = _ProtoTunnelDecryptAndValidatePacket(pProtoTunnel, pTunnel, pHeadOffset, pPacketData, iRecvLen, TRUE);

        if (iNumPackets > 0)
        {
            ProtoTunnelControlT ControlInfo;

            // accumulate receive stats
            pTunnel->RecvStat.uLastPacketTime = uCurTick;
            pTunnel->RecvStat.uNumBytes += iRecvLen;
            pTunnel->RecvStat.uNumSubpacketBytes += iRecvLen - (iNumPackets * PROTOTUNNEL_PKTHDRSIZE) - 2 - pProtoTunnel->uHmacSize;
            pTunnel->RecvStat.uNumPackets += 1;
            pTunnel->RecvStat.uNumSubpackets += iNumPackets;
            pProtoTunnel->uNumSubPktsRecvd += (unsigned)iNumPackets;

            // process any control info
            if (_ProtoTunnelGetControlInfo(pTunnel, &ControlInfo, pPacketData) != 0)
            {
                if (ControlInfo.uActive == 1)
                {
                    NetPrintf(("prototunnel: [%p][%04d] connection is active; disabling control packets\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels)));
                    pTunnel->bSendCtrlInfo = FALSE;
                }
                else if (!pTunnel->bSendCtrlInfo)
                {
                    /* this path is typically exercised when one side of the connection ends up refcounting a tunnel and the other side
                       ends up destroying/recreating the tunnel because both consoles get Blaze messages with different timing. Under such
                       conditions, the prototunnel handshaking (sending of ctrl messages) needs to occur again. */
                    NetPrintf(("prototunnel: [%p][%04d] peer no longer sees connection as active; resume sending control packets\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels)));
                    pTunnel->bSendCtrlInfo = TRUE;
                }
            }
            else if ((pTunnel->bSendCtrlInfo) && (pTunnel->uActive == 1))
            {
                NetPrintf(("prototunnel: [%p][%04d] connection is active; disabling control packets\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels)));
                pTunnel->bSendCtrlInfo = FALSE;
            }

            // rewrite address to virtual address
            SockaddrInSetAddr(pRecvAddr, pTunnel->uVirtualAddr);
        }
        else if (iNumPackets == PROTOTUNNEL_PACKETRECVFAIL_VALIDATE)
        {
            /* attempt rematch with a different key and/or stream offset of zero; if rematch is successful we return TRYAGAIN,
               as we need the caller to call us again following a successful rematch.  if rematch fails here, we do not want to
               try again, so we directly return the error code */
            NetPrintf(("prototunnel: [%p][%04d] attempting tunnel rematch after VALIDATE error\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels)));
            if (_ProtoTunnelMatchTunnel(pProtoTunnel, pTunnel, pPacketData, iRecvLen))
            {
                NetPrintf(("prototunnel: [%p][%04d] tunnel rematch succeeded\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels)));
                iNumPackets = PROTOTUNNEL_PACKETRECVFAIL_TRYAGAIN;
            }
            else
            {
                NetPrintf(("prototunnel: [%p][%04d] tunnel rematch failed\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels)));
            }
        }
        else if (iNumPackets == PROTOTUNNEL_PACKETRECVFAIL_OUTOFORDER)
        {
            NetPrintf(("prototunnel: [%p][%04d] discarding out of order packet\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels)));
            pTunnel->SendStat.uNumDiscards += 1;
            pProtoTunnel->uNumPktsDiscard += 1;
        }
    }
    else
    {
        // tell calling code there is no matching tunnel
        iNumPackets = PROTOTUNNEL_PACKETRECVFAIL_NOMATCH;
    }

    // release exclusive access
    NetCritLeave(&pProtoTunnel->TunnelsCritR);

    // return number of packets decoded
    return(iNumPackets);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelRecv

    \Description
        Callback to handle idle and recv callbacks on a tunnel socket.

    \Input *pProtoTunnel    - module ref
    \Input pPacketData      - packet data
    \Input iRecvLen         - receive length
    \Input *pRecvAddr       - sender's address

    \Output
        int32_t             - zero

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelRecv(ProtoTunnelRefT *pProtoTunnel, uint8_t *pPacketData, int32_t iRecvLen, struct sockaddr *pRecvAddr)
{
    int32_t iPacket, iNumPackets;
    uint16_t aRemotePortList[PROTOTUNNEL_MAXPORTS];
    uint8_t *pPacketHead;
    uint32_t uHeadOffset;

    // find matching tunnel and decode packets
    if ((iNumPackets = _ProtoTunnelRecvData(pProtoTunnel, aRemotePortList, sizeof(aRemotePortList), &uHeadOffset, pPacketData, iRecvLen, pRecvAddr)) == PROTOTUNNEL_PACKETRECVFAIL_TRYAGAIN)
    {
        // we were able to match a new key/stream offset; run it through again
        iNumPackets = _ProtoTunnelRecvData(pProtoTunnel, aRemotePortList, sizeof(aRemotePortList), &uHeadOffset, pPacketData, iRecvLen, pRecvAddr);
    }

    // if no packets
    if (iNumPackets <= 0)
    {
        NetPrintf(("prototunnel: [%p] recv - received unhandled %d bytes from %a:%d (%d)\n", pProtoTunnel, iRecvLen,
            SockaddrInGetAddr(pRecvAddr), SockaddrInGetPort(pRecvAddr), iNumPackets));

        // if event callback is set, call it
        if ((pProtoTunnel->pCallback != NULL) && (iNumPackets == PROTOTUNNEL_PACKETRECVFAIL_NOMATCH))
        {
            pProtoTunnel->pCallback(pProtoTunnel, PROTOTUNNEL_EVENT_RECVNOMATCH, (char *)pPacketData, iRecvLen, pRecvAddr, pProtoTunnel->pUserData);
        }

        // display recv info
        if (pProtoTunnel->iVerbosity > 3)
        {
            NetPrintMem(pPacketData, (iRecvLen < 64) ? iRecvLen : 64, "prototunnel-recv-nomatch");
        }
        return(0);
    }

    // display decrypted packet data
    #if DIRTYCODE_LOGGING
    if (pProtoTunnel->iVerbosity > 3)
    {
        NetPrintMem(pPacketData, (iRecvLen < 64) ? iRecvLen : 64, "prototunnel-recv");
    }
    #endif

    // demultiplex aggregate packet data and push into the appropriate sockets
    for (iPacket = 0, pPacketHead = pPacketData+uHeadOffset, pPacketData = pPacketData+uHeadOffset+(iNumPackets*PROTOTUNNEL_PKTHDRSIZE)+pProtoTunnel->uHmacSize; iPacket < iNumPackets; iPacket++)
    {
        // extract size and port info
        uint32_t uPktHead = (pPacketHead[0] << 8) | pPacketHead[1];
        uint32_t uPktSize = uPktHead >> 4;
        uint32_t uPortIdx = uPktHead & 0xf;
        uint32_t uPort = aRemotePortList[uPortIdx];

        // if it is a control packet, skip it
        if (uPortIdx != PROTOTUNNEL_CONTROL_PORTIDX)
        {
            SocketT *pSocket;

            // find SOCK_DGRAM socket bound to this port
            if (SocketInfo(NULL, 'bndu', uPort, &pSocket, sizeof(pSocket)) == 0)
            {
                // rewrite port to match what socket will be expecting
                SockaddrInSetPort(pRecvAddr, uPort);

                // display tunneled receive
                NetPrintfVerbose((pProtoTunnel->iVerbosity, 2, "prototunnel: [%p] recv - %db->%d\n", pProtoTunnel, uPktSize, uPort));

                // forward data on to caller
                SocketControl(pSocket, 'push', uPktSize, pPacketData, pRecvAddr);
            }
            else
            {
                NetPrintf(("prototunnel: [%p] recv - warning, got data for port %d with no socket bound to that port\n", pProtoTunnel, uPort));
            }
        }
        else
        {
            NetPrintf(("prototunnel: [%p] recv - received control packet\n", pProtoTunnel));
        }

        // skip to next packet
        pPacketHead += PROTOTUNNEL_PKTHDRSIZE;
        pPacketData += uPktSize;
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelRecvCallback

    \Description
        Callback to handle idle and recv callbacks on a tunnel socket.

    \Input *pSocket - socket ref
    \Input iFlags   - unused
    \Input *_pRef   - tunnel map ref

    \Output
        int32_t     - zero

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelRecvCallback(SocketT *pSocket, int32_t iFlags, void *_pRef)
{
    ProtoTunnelRefT *pProtoTunnel = (ProtoTunnelRefT *)_pRef;
    struct sockaddr RecvAddr;
    int32_t iRecvAddrLen = sizeof(RecvAddr), iRecv, iRecvLen;
    uint8_t aPacketData[PROTOTUNNEL_PACKETBUFFER];

    SockaddrInit(&RecvAddr, AF_INET);

    // got any input?
    for (iRecv = 0; iRecv < pProtoTunnel->iMaxRecv; iRecv += 1)
    {
        if ((iRecvLen = SocketRecvfrom(pSocket, (char *)aPacketData, sizeof(aPacketData), 0, &RecvAddr, &iRecvAddrLen)) <= 0)
        {
            break;
        }

        // drop packet for easy testing of lost packet recovery
        #if DIRTYCODE_DEBUG
        if (pProtoTunnel->iPacketDrop > 0)
        {
            pProtoTunnel->iPacketDrop -= 1;
            NetPrintf(("prototunnel: [%p] dropping received packet\n", pProtoTunnel));
            continue;
        }
        #endif

        // forward to any registered raw inbound data filter
        if (pProtoTunnel->pRawRecvCallback)
        {
            if (pProtoTunnel->pRawRecvCallback(pSocket, aPacketData, iRecvLen, &RecvAddr, sizeof(RecvAddr), pProtoTunnel->pRawRecvUserData) > 0)
            {
                // inbound raw data was swallowed by the filter, this packet is to be ignored by ProtoTunnel
                continue;
            }
        }

        // process the packet
        _ProtoTunnelRecv(pProtoTunnel, aPacketData, iRecvLen, &RecvAddr);
    }

    // update stats
    pProtoTunnel->uNumRecvCalls += iRecv > 0;
    pProtoTunnel->uNumPktsRecvd += (unsigned)iRecv;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelSocketConfig

    \Description
        Configure tunnel socket

    \Input *pProtoTunnel    - pointer to module state
    \Input *pSocket         - socket to configure

    \Output
        uint32_t            - local port socket is bound to

    \Version 09/18/2013 (jbrookes) split from _ProtoTunnelSocketOpen
*/
/********************************************************************************F*/
static uint32_t _ProtoTunnelSocketConfig(ProtoTunnelRefT *pProtoTunnel, SocketT *pSocket)
{
    struct sockaddr BindAddr;
    uint32_t uPort;

    // retrieve bound port
    SocketInfo(pSocket, 'bind', 0, &BindAddr, sizeof(BindAddr));

    // reference local port
    uPort = SockaddrInGetPort(&BindAddr);
    NetPrintf(("prototunnel: [%p] bound socket to port %d\n", pProtoTunnel, uPort));

    // set up for socket callback events
    SocketCallback(pSocket, CALLB_RECV, pProtoTunnel->iIdleCbRate, pProtoTunnel, &_ProtoTunnelRecvCallback);

    return(uPort);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelSocketOpen

    \Description
        Open a tunnel socket

    \Input *pProtoTunnel    - pointer to module state
    \Input iPort            - port tunnel will go over

    \Output
        SocketT *           - new socket or NULL on failure

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static SocketT *_ProtoTunnelSocketOpen(ProtoTunnelRefT *pProtoTunnel, int32_t iPort)
{
    struct sockaddr BindAddr;
    SocketT *pSocket;
    int32_t iResult;

    // open the socket
    if ((pSocket = SocketOpen(AF_INET, SOCK_DGRAM, PROTOTUNNEL_IPPROTO)) == NULL)
    {
        NetPrintf(("prototunnel: [%p] unable to open socket\n", pProtoTunnel));
        return(NULL);
    }

    // bind socket to specified port
    SockaddrInit(&BindAddr, AF_INET);
    SockaddrInSetPort(&BindAddr, iPort);
    if ((iResult = SocketBind(pSocket, &BindAddr, sizeof(BindAddr))) != SOCKERR_NONE)
    {
        if (_ProtoTunnel_bRetryRandomOnFailure)
        {
            NetPrintf(("prototunnel: [%p] error %d binding to port %d, trying random\n", pProtoTunnel, iResult, iPort));
            SockaddrInSetPort(&BindAddr, 0);
            if ((iResult = SocketBind(pSocket, &BindAddr, sizeof(BindAddr))) != SOCKERR_NONE)
            {
                NetPrintf(("prototunnel: [%p] error %d binding to port\n", pProtoTunnel, iResult));
                SocketClose(pSocket);
                return(NULL);
            }
        }
        else
        {
            NetPrintf(("prototunnel: [%p] error %d binding to port %d\n", pProtoTunnel, iResult, iPort));
            SocketClose(pSocket);
            return(NULL);
        }
    }

    // return socket to caller
    return(pSocket);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelStatCalc

    \Description
        Calculates ProtoTunnel Stats

    \Input *pTunnel          - Tunnel that calculations are to be performed on.
    \Input iSelect           - 'rcvs' for receive stats and 'snds' for send stats

    \Version 09/17/2014 (tcho)
*/
/********************************************************************************F*/
static void _ProtoTunnelStatCalc(ProtoTunnelT *pTunnel, int32_t iSelect)
{
    uint32_t uCurTick = NetTick();
    uint32_t uRange = 0;

    if (iSelect == 'rcvs')
    {
        if (pTunnel->RecvStat.uUpdateTime == 0)
        {
            pTunnel->RecvStat.uBytePerSecond = 0;
            pTunnel->RecvStat.uRawBytesPerSecond = 0;
            pTunnel->RecvStat.uEfficiency = 0;
        }
        else
        {
            uRange = NetTickDiff(uCurTick, pTunnel->RecvStat.uUpdateTime);
            pTunnel->RecvStat.uRawBytesPerSecond = (pTunnel->RecvStat.uNumBytes - pTunnel->uLastRecvNumBytes) * 1000 / uRange;
            pTunnel->RecvStat.uBytePerSecond = (pTunnel->RecvStat.uNumSubpacketBytes - pTunnel->uLastRecvNumSubpacketBytes) * 1000 / uRange;
            pTunnel->RecvStat.uEfficiency = (uint32_t)(((float)pTunnel->RecvStat.uBytePerSecond/(float)pTunnel->RecvStat.uRawBytesPerSecond) * 100.0f);
        }

        // update tracking variable
        pTunnel->RecvStat.uPrevUpdateTime = pTunnel->RecvStat.uUpdateTime;
        pTunnel->RecvStat.uUpdateTime = uCurTick;
        pTunnel->uLastRecvNumBytes = pTunnel->RecvStat.uNumBytes;
        pTunnel->uLastRecvNumSubpacketBytes = pTunnel->RecvStat.uNumSubpacketBytes;
    }
    else if (iSelect == 'snds')
    {
        if (pTunnel->SendStat.uUpdateTime == 0)
        {
            pTunnel->SendStat.uBytePerSecond = 0;
            pTunnel->SendStat.uRawBytesPerSecond = 0;
            pTunnel->SendStat.uEfficiency = 0;
        }
        else
        {
            uRange = NetTickDiff(uCurTick, pTunnel->SendStat.uUpdateTime);
            pTunnel->SendStat.uRawBytesPerSecond = (pTunnel->SendStat.uNumBytes - pTunnel->uLastSendNumBytes) * 1000 / uRange;
            pTunnel->SendStat.uBytePerSecond = (pTunnel->SendStat.uNumSubpacketBytes - pTunnel->uLastSendNumSubpacketBytes) * 1000 / uRange;
            pTunnel->SendStat.uEfficiency = (uint32_t)(((float)pTunnel->SendStat.uBytePerSecond/(float)pTunnel->SendStat.uRawBytesPerSecond) * 100.0f);
        }

        // update tracking variable
        pTunnel->SendStat.uPrevUpdateTime = pTunnel->SendStat.uUpdateTime;
        pTunnel->SendStat.uUpdateTime = uCurTick;
        pTunnel->uLastSendNumBytes = pTunnel->SendStat.uNumBytes;
        pTunnel->uLastSendNumSubpacketBytes = pTunnel->SendStat.uNumSubpacketBytes;
    }

}
/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoTunnelCreate

    \Description
        Create the ProtoTunnel module.

    \Input iMaxTunnels      - maximum number of tunnels module can allocate
    \Input iTunnelPort      - local port for socket all tunnels will use

    \Output
        ProtoTunnelRefT *   - pointer to new module, or NULL

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
ProtoTunnelRefT *ProtoTunnelCreate(int32_t iMaxTunnels, int32_t iTunnelPort)
{
    ProtoTunnelRefT *pProtoTunnel;
    int32_t iRefSize = sizeof(*pProtoTunnel) + ((iMaxTunnels-1) * sizeof(ProtoTunnelT));
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // maximum of 32k tunnels due to size of ident field
    if (iMaxTunnels > 32767)
    {
        NetPrintf(("prototunnel: clamping requested %d tunnels to max of 32767\n", iMaxTunnels));
        iMaxTunnels = 32767;
    }

    // query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pProtoTunnel = DirtyMemAlloc(iRefSize, PROTOTUNNEL_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("prototunnel: could not allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pProtoTunnel, iRefSize);
    pProtoTunnel->iMemGroup = iMemGroup;
    pProtoTunnel->pMemGroupUserData = pMemGroupUserData;
    pProtoTunnel->iMaxTunnels = iMaxTunnels;
    pProtoTunnel->iMaxRecv = 64;
    pProtoTunnel->iIdleCbRate = 100;
    pProtoTunnel->uTunnelPort = iTunnelPort;
    pProtoTunnel->uHmacType = CRYPTHASH_MURMUR3;
    pProtoTunnel->uHmacSize = PROTOTUNNEL_HMAC_DEFSIZE;
    pProtoTunnel->uVersion = PROTOTUNNEL_VERSION;

    if (_ProtoTunnelGetBaseAddress(pProtoTunnel) != 0)
    {
        DirtyMemFree(pProtoTunnel, PROTOTUNNEL_MEMID, pProtoTunnel->iMemGroup, pProtoTunnel->pMemGroupUserData);
        return(NULL);
    }

    // create the tunnel socket
    if ((pProtoTunnel->pSocket = _ProtoTunnelSocketOpen(pProtoTunnel, iTunnelPort)) == NULL)
    {
        _ProtoTunnelReleaseBaseAddress(pProtoTunnel);
        DirtyMemFree(pProtoTunnel, PROTOTUNNEL_MEMID, pProtoTunnel->iMemGroup, pProtoTunnel->pMemGroupUserData);
        return(NULL);
    }
    pProtoTunnel->uTunnelPort = _ProtoTunnelSocketConfig(pProtoTunnel, pProtoTunnel->pSocket);

    // initialize critical sections
    NetCritInit(&pProtoTunnel->TunnelsCritS, "prototunnel-global-send");
    NetCritInit(&pProtoTunnel->TunnelsCritR, "prototunnel-global-recv");

    // restrict max udp packet size
    SocketControl(NULL, 'maxp', PROTOTUNNEL_MAXPACKET, NULL, NULL);

    // hook into global socket send hook
    SocketControl(NULL, 'sdcb', TRUE, (void *)_ProtoTunnelSendCallback, pProtoTunnel);

    // configure prototunnel socket to NOT call socket send callbacks
    SocketControl(pProtoTunnel->pSocket, 'scbk', FALSE, NULL, NULL);
    
    // return ref to caller
    return(pProtoTunnel);
}

/*F********************************************************************************/
/*!
    \Function ProtoTunnelDestroy

    \Description
        Destroy the ProtoTunnel module.

    \Input *pProtoTunnel    - pointer to module state

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
void ProtoTunnelDestroy(ProtoTunnelRefT *pProtoTunnel)
{
    // clear global socket send hook
    SocketControl(NULL, 'sdcb', FALSE, (void *)_ProtoTunnelSendCallback, pProtoTunnel);

    // close tunnel socket
    if (pProtoTunnel->pSocket != NULL)
    {
        SocketClose(pProtoTunnel->pSocket);
    }

    // close server socket, if allocated
    if (pProtoTunnel->pServerSocket != NULL)
    {
        SocketClose(pProtoTunnel->pServerSocket);
    }

    // dispose of critical sections
    NetCritKill(&pProtoTunnel->TunnelsCritR);
    NetCritKill(&pProtoTunnel->TunnelsCritS);

    _ProtoTunnelReleaseBaseAddress(pProtoTunnel);

    // dispose of module memory
    DirtyMemFree(pProtoTunnel, PROTOTUNNEL_MEMID, pProtoTunnel->iMemGroup, pProtoTunnel->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function ProtoTunnelCallback

    \Description
        Set event callback

    \Input *pProtoTunnel    - pointer to module state
    \Input *pCallback       - callback pointer
    \Input *pUserData       - callback data

    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
void ProtoTunnelCallback(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelCallbackT *pCallback, void *pUserData)
{
    pProtoTunnel->pCallback = pCallback;
    pProtoTunnel->pUserData = pUserData;
}

/*F********************************************************************************/
/*!
    \Function ProtoTunnelAlloc

    \Description
        Allocate a tunnel.

    \Input *pProtoTunnel    - pointer to module state
    \Input *pInfo           - tunnel info
    \Input *pKey            - encryption key for tunnel

    \Output
        int32_t             - negative=error, else allocated tunnel id

    \Version 12/07/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoTunnelAlloc(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelInfoT *pInfo, const char *pKey)
{
    ProtoTunnelT *pTunnel;
    int32_t iTunnel;

    // get exclusive access to tunnel list
    NetCritEnter(&pProtoTunnel->TunnelsCritS);
    NetCritEnter(&pProtoTunnel->TunnelsCritR);

    // see if we already have a tunnel with this clientId
    for (iTunnel = 0; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel++)
    {
        pTunnel = &pProtoTunnel->Tunnels[iTunnel];
        if (pTunnel->Info.uRemoteClientId == pInfo->uRemoteClientId)
        {
            int32_t iKey, iKeySlot, iResult = (signed)pTunnel->uVirtualAddr;

            // refcount the tunnel
            NetPrintf(("prototunnel: [%p][%04d] refcounting tunnel with id=0x%08x key=%s clientId=0x%08x remote address=%a\n",
                pProtoTunnel, iTunnel, pTunnel->uVirtualAddr, pKey, pInfo->uRemoteClientId, pInfo->uRemoteAddr));
            pTunnel->uRefCount += 1;

            // append key to key list
            for (iKey = 0, iKeySlot = -1; iKey < PROTOTUNNEL_MAXKEYS; iKey++)
            {
                #if DIRTYCODE_LOGGING
                if (!strcmp(pKey, pTunnel->aKeyList[iKey]))
                {
                    NetPrintf(("prototunnel: [%p][%04d] warning - duplicate key %s on alloc\n", pProtoTunnel, iTunnel, pKey));
                }
                #endif
                if ((iKeySlot == -1) && (pTunnel->aKeyList[iKey][0] == '\0'))
                {
                    iKeySlot = iKey;
                }
            }
            if (iKeySlot != -1)
            {
                ds_strnzcpy(pTunnel->aKeyList[iKeySlot], pKey, sizeof(pTunnel->aKeyList[iKeySlot]));
            }
            else
            {
                NetPrintf(("prototunnel: [%p][%04d] error - key overflow on alloc\n", pProtoTunnel, iTunnel));
                iResult = -1;
            }

            NetCritLeave(&pProtoTunnel->TunnelsCritR);
            NetCritLeave(&pProtoTunnel->TunnelsCritS);

            return(iResult);
        }
    }

    // find an unallocated tunnel
    for (iTunnel = 0; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel++)
    {
        if (pProtoTunnel->Tunnels[iTunnel].uVirtualAddr == 0)
        {
            break;
        }
    }
    // make sure we found room
    if (iTunnel == pProtoTunnel->iMaxTunnels)
    {
        NetPrintf(("prototunnel: [%p] could not allocate a new tunnel\n", pProtoTunnel));
        NetCritLeave(&pProtoTunnel->TunnelsCritR);
        NetCritLeave(&pProtoTunnel->TunnelsCritS);
        return(-1);
    }

    // ref and init tunnel
    pTunnel = &pProtoTunnel->Tunnels[iTunnel];
    ds_memclr(pTunnel, sizeof(*pTunnel));
    ds_memcpy_s(&pTunnel->Info, sizeof(pTunnel->Info), pInfo, sizeof(*pInfo));
    NetCritInit(&pTunnel->PacketCrit, "prototunnel-tunnel");
    ds_strnzcpy(pTunnel->aKeyList[0], pKey, sizeof(pTunnel->aKeyList[0]));
    pTunnel->uRefCount = 1;

    pTunnel->uLastTunnelSend = NetTick();

    // init tunnel crypto state
    _ProtoTunnelCryptSetup(pTunnel, 0, TRUE);

    // port index seven is reserved by ProtoTunnel and always encrypted
    pTunnel->Info.aPortFlags[PROTOTUNNEL_CONTROL_PORTIDX] = PROTOTUNNEL_PORTFLAG_ENCRYPTED;

    // initialize to send connect info
    pTunnel->bSendCtrlInfo = TRUE;

    // assign a virtual address/id
    pTunnel->uVirtualAddr = pProtoTunnel->uVirtualAddr++;

    // if unspecified, set remote port
    if (pTunnel->Info.uRemotePort == 0)
    {
        pTunnel->Info.uRemotePort = pProtoTunnel->uTunnelPort;
    }

    // set default tunnel protocol version
    _ProtoTunnelSetVersion(pProtoTunnel, pTunnel, pProtoTunnel->uVersion);

    #if DIRTYCODE_LOGGING
    pTunnel->uLastStatUpdate = NetTick();
    #endif

    // release exclusive access to tunnel list
    NetCritLeave(&pProtoTunnel->TunnelsCritR);
    NetCritLeave(&pProtoTunnel->TunnelsCritS);

    // debug spam
    #if DIRTYCODE_LOGGING
    {
        int32_t iPort;
        NetPrintf(("prototunnel: [%p][%04d] creating map to remote client %a:%d (id=0x%08x)\n",
            pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), pTunnel->Info.uRemoteAddr, pTunnel->Info.uRemotePort, pTunnel->Info.uRemoteClientId));
        for (iPort = 0; (iPort < PROTOTUNNEL_MAXPORTS) && (pTunnel->Info.aRemotePortList[iPort] != 0); iPort++)
        {
            NetPrintf(("prototunnel: [%p][%04d] [%d] %d\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), iPort, pTunnel->Info.aRemotePortList[iPort]));
        }
    }
    #endif

    // return addr/id to caller
    NetPrintf(("prototunnel: [%p][%04d] allocated tunnel with id 0x%08x key=%s\n",
        pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), pTunnel->uVirtualAddr, pTunnel->aKeyList[0]));
    return(pTunnel->uVirtualAddr);
}

/*F********************************************************************************/
/*!
    \Function ProtoTunnelFree

    \Description
        Free a tunnel.

    \Input *pProtoTunnel    - pointer to module state
    \Input uTunnelId        - id of tunnel to free
    \Input *pKey            - key tunnel was allocated with

    \Notes
        pKey is only required when tunnel refcounting is used.  Otherwise, pKey
        may be specified as NULL.

    \Version 12/07/2005 (jbrookes)
*/
/********************************************************************************F*/
uint32_t ProtoTunnelFree(ProtoTunnelRefT *pProtoTunnel, uint32_t uTunnelId, const char *pKey)
{
    return(ProtoTunnelFree2(pProtoTunnel, uTunnelId, pKey, 0));
}

/*F********************************************************************************/
/*!
    \Function ProtoTunnelFree2

    \Description
        Free a tunnel. Same as ProtoTunnelFree but takes an IP address that is
        no longer used.  Consider for deprecation in the future.

    \Input *pProtoTunnel    - pointer to module state
    \Input uTunnelId        - id of tunnel to free
    \Input *pKey            - key tunnel was allocated with
    \Input uAddr            - address of peer that is being freed

    \Notes
        pKey is only required when tunnel refcounting is used.  Otherwise, pKey
        may be specified as NULL.

    \Version 03/15/2010 (jrainy)
*/
/********************************************************************************F*/
uint32_t ProtoTunnelFree2(ProtoTunnelRefT *pProtoTunnel, uint32_t uTunnelId, const char *pKey, uint32_t uAddr)
{
    ProtoTunnelT *pTunnel;
    int32_t iTunnel;
    uint32_t uRet = (uint32_t)-1;

    #if DIRTYCODE_LOGGING
    uint32_t bFound = FALSE;
    #endif

    // get exclusive access to tunnel list
    NetCritEnter(&pProtoTunnel->TunnelsCritS);
    NetCritEnter(&pProtoTunnel->TunnelsCritR);

    // find tunnel id
    for (iTunnel = 0, pTunnel = &pProtoTunnel->Tunnels[0]; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel++, pTunnel++)
    {
        // found it?
        if (pTunnel->uVirtualAddr == uTunnelId)
        {
            #if DIRTYCODE_LOGGING
            bFound = TRUE;
            #endif

            // deallocate the tunnel
            if (pTunnel->uRefCount == 1)
            {
                NetPrintf(("prototunnel: [%p][%04d] freeing tunnel 0x%08x\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), uTunnelId));

                // flush buffer before destroy
                _ProtoTunnelBufferSend(pProtoTunnel, pTunnel, NetTick());

                // dispose of critical section
                NetCritKill(&pTunnel->PacketCrit);

                ds_memclr(pTunnel, sizeof(*pTunnel));

                uRet = 0;
            }
            else
            {
                int32_t iKey;

                NetPrintf(("prototunnel: [%p][%04d] decrementing refcount of tunnel 0x%08x (clientId=0x%08x)\n",
                    pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), uTunnelId, pTunnel->Info.uRemoteClientId));

                // remove key from key list
                for (iKey = 0; iKey < PROTOTUNNEL_MAXKEYS; iKey++)
                {
                    if (!strcmp(pKey, pTunnel->aKeyList[iKey]))
                    {
                        ds_memclr(pTunnel->aKeyList[iKey], sizeof(pTunnel->aKeyList[iKey]));
                        break;
                    }
                }

                // did we blow away our send key?
                if (pTunnel->uSendKey == (uint8_t)iKey)
                {
                    NetPrintf(("prototunnel: [%p][%04d] free of current send key; picking another\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels)));
                    for (iKey = 0; iKey < PROTOTUNNEL_MAXKEYS; iKey++)
                    {
                        if (pTunnel->aKeyList[iKey][0] != '\0')
                        {
                            NetPrintf(("prototunnel: [%p][%04d] picking key %d (%s) offset=%d\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), iKey, pTunnel->aKeyList[iKey], pTunnel->uSendOffset));
                            // init tunnel crypto state for new key
                            _ProtoTunnelCryptSetup(pTunnel, iKey, FALSE);
                            break;
                        }
                    }
                }
                else if (iKey == PROTOTUNNEL_MAXKEYS)
                {
                    NetPrintf(("prototunnel: [%p][%04d] could not find key %s in key list on free\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), pKey));
                }

                pTunnel->uRefCount -= 1;
                uRet = pTunnel->uRefCount;

                NetPrintf(("prototunnel: [%p][%04d] refcounting down tunnel with id=0x%08x key=%s clientId=0x%08x remote address=%a\n",
                    pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), pTunnel->uVirtualAddr, pKey, pTunnel->Info.uRemoteClientId, pTunnel->Info.uRemoteAddr));
            }

            // done
            break;
        }
    }

    #if DIRTYCODE_LOGGING
    if (bFound == FALSE)
    {
        NetPrintf(("prototunnel: [%p][%04d] could not find tunnel with id 0x%08x to free\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), uTunnelId));
    }
    #endif

    // release exclusive access to tunnel list
    NetCritLeave(&pProtoTunnel->TunnelsCritR);
    NetCritLeave(&pProtoTunnel->TunnelsCritS);
    return(uRet);
}

/*F********************************************************************************/
/*!
    \Function ProtoTunnelUpdatePortList

    \Description
        Updates the port list for the given tunnel.  Port and port flag info is
        copied over from the specified info structure if the port value is non-
        zero.

    \Input *pProtoTunnel    - pointer to module state
    \Input uTunnelId        - id of tunnel to update port mapping for
    \Input *pInfo           - structure containing updated port info

    \Output
        int32_t             - zero=success, else could not find tunnel with given id

    \Version 06/12/2008 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoTunnelUpdatePortList(ProtoTunnelRefT *pProtoTunnel, uint32_t uTunnelId, ProtoTunnelInfoT *pInfo)
{
    ProtoTunnelT *pTunnel;
    int32_t iTunnel, iResult = -1;
    int32_t iPort;

    // get exclusive access to tunnel list
    NetCritEnter(&pProtoTunnel->TunnelsCritS);
    NetCritEnter(&pProtoTunnel->TunnelsCritR);

    // find tunnel id
    for (iTunnel = 0, pTunnel = &pProtoTunnel->Tunnels[0]; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel++, pTunnel++)
    {
        // found it?
        if (pTunnel->uVirtualAddr == uTunnelId)
        {
            // copy portlist items that should be updated
            for (iPort = 0; iPort < PROTOTUNNEL_MAXPORTS; iPort += 1)
            {
                if (pInfo->aRemotePortList[iPort] != 0)
                {
                    NetPrintf(("prototunnel: [%p][%04d] updating port mapping %d for tunnel=0x%08x from (%d,%d) to (%d,%d)\n",
                        pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), iPort, uTunnelId,
                        pTunnel->Info.aRemotePortList[iPort], pTunnel->Info.aPortFlags[iPort],
                        pInfo->aRemotePortList[iPort], pInfo->aPortFlags[iPort]));
                    pTunnel->Info.aRemotePortList[iPort] = pInfo->aRemotePortList[iPort];
                    pTunnel->Info.aPortFlags[iPort] = pInfo->aPortFlags[iPort];
                }
            }
            iResult = 0;
            break;
        }
    }

    // release exclusive access to tunnel list
    NetCritLeave(&pProtoTunnel->TunnelsCritR);
    NetCritLeave(&pProtoTunnel->TunnelsCritS);

    // return result code to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoTunnelValidatePacket

    \Description
        Validate key against packet, a tunnel has not been allocated yet

    \Input *pProtoTunnel- pointer to module state
    \Input *pTunnel     - pointer to tunnel
    \Input *pOutputData - [out] pointer to buffer to store decrypted data (may be NULL)
    \Input *pPacketData - pointer to tunnel packet
    \Input iPacketSize  - size of tunnel packet
    \Input *pKey        - encryption key for tunnel

    \Output
        int32_t         - PROTOTUNNEL_PACKETRECVFAIL_* on error, else number of subpackets decoded

    \Version 06/06/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoTunnelValidatePacket(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelT *pTunnel, uint8_t *pOutputData, const uint8_t *pPacketData, int32_t iPacketSize, const char *pKey)
{
    uint8_t aPacketData[PROTOTUNNEL_PACKETBUFFER];
    int32_t iNumPackets;
    uint32_t uHeadOffset;

    CryptArc4T CryptTemp;
    uint32_t uRecvOffsetTemp;
    uint8_t aHmacKeyTemp[64];

    // copy packet data to temp buffer
    ds_memcpy_s(aPacketData, sizeof(aPacketData), pPacketData, iPacketSize);

    // save current hmac key
    ds_memcpy_s(aHmacKeyTemp, sizeof(aHmacKeyTemp), pTunnel->aHmacKey, sizeof(pTunnel->aHmacKey));

    // init HMAC key by running IV through RC4 initialized with tunnel key
    CryptArc4Init(&CryptTemp, (const unsigned char *)pKey, (int32_t)strlen(pKey), PROTOTUNNEL_CRYPTARC4_ITER);
    ds_memcpy_s(pTunnel->aHmacKey, sizeof(pTunnel->aHmacKey), _ProtoTunnel_aHmacInitVec, sizeof(_ProtoTunnel_aHmacInitVec));
    CryptArc4Apply(&CryptTemp, (uint8_t *)pTunnel->aHmacKey, sizeof(pTunnel->aHmacKey));

    // save current recv state
    ds_memcpy_s(&CryptTemp, sizeof(CryptTemp), &pTunnel->CryptRecvState, sizeof(pTunnel->CryptRecvState));
    uRecvOffsetTemp = pTunnel->uRecvOffset;

    // init recv state with specified key & reset offset
    CryptArc4Init(&pTunnel->CryptRecvState, (unsigned char *)pKey, (int32_t)strlen(pKey), PROTOTUNNEL_CRYPTARC4_ITER);
    pTunnel->uRecvOffset = 0;

    // decrypt and validate packet data
    iNumPackets = _ProtoTunnelDecryptAndValidatePacket(pProtoTunnel, pTunnel, &uHeadOffset, aPacketData, iPacketSize, FALSE);

    // restore previous recv state
    ds_memcpy(&pTunnel->CryptRecvState, &CryptTemp, sizeof(CryptTemp));
    pTunnel->uRecvOffset = uRecvOffsetTemp;

    // restore previous HMAC key
    ds_memcpy_s(pTunnel->aHmacKey, sizeof(pTunnel->aHmacKey), aHmacKeyTemp, sizeof(aHmacKeyTemp));

    // copy decrypted data to output buffer, if available
    if ((iNumPackets > 0) && (pOutputData != NULL))
    {
        ds_memcpy_s(pOutputData, iPacketSize, aPacketData, sizeof(aPacketData));
    }

    // return number of packets decoded
    return(iNumPackets);
}

/*F********************************************************************************/
/*!
    \Function ProtoTunnelStatus

    \Description
        Get module status

    \Input *pProtoTunnel    - pointer to module state
    \Input iSelect          - status selector
    \Input iValue           - selector specific
    \Input *pBuf            - [out] - selector specific
    \Input iBufSize         - size of output buffer

    \Output
        int32_t             - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'actv' - return number of active tunnels based on version (via iValue)
            'bnds' - return if the specified port (iValue) is part of the server port list
            'dpkt' - return number of out-of-order packet discards
            'hmac' - return HMAC type/size
            'idle' - return idle callback rate
            'lprt' - return local socket port
            'maxp' - return maximum size of packet that can be tunneled (NOTE: pProtoTunnel can be NULL)
            'rcvs' - copy receive statistics to pBuf
            'rcal' - return number of recv calls
            'rmax' - return maximum number of recv calls allowed in _ProtoTunnelRecvCallback
            'rprt' - remote game port for the specificed tunnel id
            'rsub' - return number of sub-packets received
            'rtot' - return number of packets received
            'snds' - copy send statistics to pBuf
            'sock' - copy socket ref to pBuf
            'vers' - return protocol version used with this tunnel id (specified with iValue) 
            'vset' - return set of supported versions as a comma delimited string (via pBuf)
            'vtop' - convert virtual address to physical address (pBuf == sockaddr, optional)
        \endverbatim

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoTunnelStatus(ProtoTunnelRefT *pProtoTunnel, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'actv')
    {
        int32_t iTunnel;
        int32_t iNumActive = 0;

        // acquire exclusive access to tunnel list
        NetCritEnter(&pProtoTunnel->TunnelsCritS);
        NetCritEnter(&pProtoTunnel->TunnelsCritR);

        // get active tunnels by version
        for (iTunnel = 0; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel += 1)
        {
            const ProtoTunnelT *pTunnel = &pProtoTunnel->Tunnels[iTunnel];
            if (pTunnel->uActive == 0)
            {
                continue;
            }
            if (pTunnel->Info.uTunnelVers == (uint16_t)iValue)
            {
                iNumActive += 1;
            }
        }

        // release exclusive access to tunnel list
        NetCritLeave(&pProtoTunnel->TunnelsCritR);
        NetCritLeave(&pProtoTunnel->TunnelsCritS);

        return(iNumActive);
    }
    if (iSelect == 'bnds')
    {
        return(_ProtoTunnelServerPortListFunc(pProtoTunnel, (uint16_t)iValue, PROTOTUNNEL_PORTLIST_CHK));
    }
    if (iSelect == 'dpkt')
    {
        return(pProtoTunnel->uNumPktsDiscard);
    }
    if (iSelect == 'hmac')
    {
        return((pProtoTunnel->uHmacType << 4) | pProtoTunnel->uHmacSize);
    }
    if (iSelect == 'idle')
    {
        return(pProtoTunnel->iIdleCbRate);
    }
    if (iSelect == 'lprt')
    {
        return(pProtoTunnel->uTunnelPort);
    }
    if (iSelect == 'maxp')
    {
        return(PROTOTUNNEL_MAXPACKET);
    }
    if (iSelect == 'rcal')
    {
        return(pProtoTunnel->uNumRecvCalls);
    }
    if (iSelect == 'rmax')
    {
        return(pProtoTunnel->iMaxRecv);
    }
    if (iSelect == 'rprt')
    {
        if ((pBuf != NULL) && (iBufSize == sizeof(uint16_t)))
        {
            int32_t iTunnel = _ProtoTunnelIndexFromId(pProtoTunnel, (uint32_t)iValue);
            if (iTunnel != -1)
            {
                *(uint16_t *)pBuf = pProtoTunnel->Tunnels[iTunnel].Info.uRemotePort;
                return(0);
            }
            else
            {
                NetPrintf(("prototunnel: [%p] 'rprt' status selector called with unknown tunnelid=0x%08x\n", pProtoTunnel, iValue));
                return(-1);
            }
        }
    }
    if (iSelect == 'rsub')
    {
        return(pProtoTunnel->uNumSubPktsRecvd);
    }
    if (iSelect == 'rtot')
    {
        return(pProtoTunnel->uNumPktsRecvd);
    }
    if (iSelect == 'rcvs')
    {
        if ((pBuf != NULL) && (iBufSize == sizeof(ProtoTunnelStatT)) && (iValue < pProtoTunnel->iMaxTunnels))
        {
            _ProtoTunnelStatCalc(&pProtoTunnel->Tunnels[iValue], iSelect);
            ds_memcpy(pBuf, &pProtoTunnel->Tunnels[iValue].RecvStat, sizeof(ProtoTunnelStatT));
            return(0);
        }
    }
    if (iSelect == 'snds')
    {
        if ((pBuf != NULL) && (iBufSize == sizeof(ProtoTunnelStatT)) && (iValue < pProtoTunnel->iMaxTunnels))
        {
            _ProtoTunnelStatCalc(&pProtoTunnel->Tunnels[iValue], iSelect);
            ds_memcpy(pBuf, &pProtoTunnel->Tunnels[iValue].SendStat, sizeof(ProtoTunnelStatT));
            return(0);
        }
    }
    if (iSelect == 'sock')
    {
        ds_memcpy(pBuf, &pProtoTunnel->pSocket, iBufSize);
        return(0);
    }
    if (iSelect == 'vers')
    {
        int32_t iResult = -1;
        int32_t iTunnel = _ProtoTunnelIndexFromId(pProtoTunnel, (uint32_t)iValue);
        if (iTunnel != -1)
        {
            iResult = (int32_t)pProtoTunnel->Tunnels[iTunnel].Info.uTunnelVers;
        }
        else
        {
            NetPrintf(("prototunnel: [%p] 'vers' status selector called with unknown tunnelid=0x%08x\n", pProtoTunnel, iValue));
        }
        return(iResult);
    }
    if (iSelect == 'vset')
    {
        if (pBuf != NULL && iBufSize > 0)
        {
            ds_strnzcpy(pBuf, PROTOTUNNEL_VERSIONS, iBufSize);
            return(0);
        }
    }
    if (iSelect == 'vtop')
    {
        return(_ProtoTunnelVirtualToPhysical(pProtoTunnel, iValue, pBuf, iBufSize));
    }
    // selector not supported
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ProtoTunnelControl

    \Description
        Control the module

    \Input *pProtoTunnel    - pointer to module state
    \Input iControl         - control selector
    \Input iValue           - selector specific
    \Input iValue2          - selector specific
    \Input *pValue          - selector specific

    \Output
        int32_t             - selector result

    \Notes
        iControl can be one of the following:

        \verbatim
            'bind' - recreate tunnel socket bound to specified port (iValue is port to bind to)
            'bndr' - remove a server tunnel port mapping from list (iValue is port to remove)
            'bnds' - recreate server tunnel socket bound to ephemeral port (iValue is ignored, iValue2 is remote server port)
            'clid' - set local clientId for all tunnels
            'drop' - drop next iValue packets (debug only; used to test crypt recovery)
            'flsh' - flush the specified tunnelId
            'hmac' - set hmac type (iValue) and size (iValue2)
            'idle' - set idle callback rate in milliseconds (iValue; default is 100)
            'rand' - set behavior to retry random port on bind failure via iValue, call before ProtoTunnelCreate
            'rate' - set flush rate in milliseconds; defaults to 16ms
            'rmax' - set maximum receive calls per call to _ProtoTunnelRecvCallback (iValue; default is 64)
            'rprt' - set specified tunnel's remote port
            'rrcb' - set raw receive callback
            'rrud' - set raw receive user data
            'sock' - set socket ref
            'spam' - set verbosity level (debug only)
            'tcid' - set per-tunnel local clientId override for specific tunnel
        \endverbatim

        Unrecognized selectors are passed through to SocketControl()

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoTunnelControl(ProtoTunnelRefT *pProtoTunnel, int32_t iControl, int32_t iValue, int32_t iValue2, const void *pValue)
{
    if (iControl == 'bind')
    {
        SocketT *pOldSocket = NULL, *pNewSocket = NULL;
        NetPrintf(("prototunnel: [%p] recreating tunnel socket bound to port %d\n", pProtoTunnel, iValue));

        // early out if we already have socket bound to this port
        if (pProtoTunnel->uTunnelPort == (unsigned)iValue)
        {
            NetPrintf(("prototunnel: [%p] already have socket bound to port %d\n", pProtoTunnel, pProtoTunnel->uTunnelPort));
            return(0);
        }

        // recreate tunnel socket bound to new port
        if ((pNewSocket = _ProtoTunnelSocketOpen(pProtoTunnel, iValue)) == NULL)
        {
            NetPrintf(("prototunnel: [%p] could not recreate tunnel socket\n", pProtoTunnel));
            return(-1);
        }

        // acquire exclusive access to tunnel list
        NetCritEnter(&pProtoTunnel->TunnelsCritS);
        NetCritEnter(&pProtoTunnel->TunnelsCritR);

        // save old tunnel socket and replace with new socket
        if (pProtoTunnel->pSocket != NULL)
        {
            pOldSocket = pProtoTunnel->pSocket;
        }
        pProtoTunnel->pSocket = pNewSocket;

        // release exclusive access to tunnel list
        NetCritLeave(&pProtoTunnel->TunnelsCritR);
        NetCritLeave(&pProtoTunnel->TunnelsCritS);

        // reconfigure new tunnel socket
        pProtoTunnel->uTunnelPort = _ProtoTunnelSocketConfig(pProtoTunnel, pProtoTunnel->pSocket);

        // close old socket
        if (pOldSocket != NULL)
        {
            SocketClose(pOldSocket);
        }
        return(0);
    }
    if (iControl == 'bndr')
    {
        // del remote port from list
        if (!_ProtoTunnelServerPortListFunc(pProtoTunnel, (uint16_t)iValue, PROTOTUNNEL_PORTLIST_DEL))
        {
            NetPrintf(("prototunnel: [%p] could not del port %d from server port list\n", pProtoTunnel, iValue));
        }
        return(0);
    }
    if (iControl == 'bnds')
    {
        if (pProtoTunnel->pServerSocket == NULL)
        {
            SocketT *pSocket = NULL;

            // create tunnel socket (binding to an ephemeral port)
            NetPrintf(("prototunnel: [%p] creating server tunnel socket (binding to system selected ephemeral port) - uServerRemotePort = %d\n", pProtoTunnel, iValue));
            if ((pSocket = _ProtoTunnelSocketOpen(pProtoTunnel, 0)) == NULL)
            {
                NetPrintf(("prototunnel: [%p] could not create server tunnel socket\n", pProtoTunnel));
                return(-1);
            }

            // acquire exclusive access to tunnel list
            NetCritEnter(&pProtoTunnel->TunnelsCritS);
            NetCritEnter(&pProtoTunnel->TunnelsCritR);

            // set new socket
            pProtoTunnel->pServerSocket = pSocket;

            // release exclusive access to tunnel list
            NetCritLeave(&pProtoTunnel->TunnelsCritR);
            NetCritLeave(&pProtoTunnel->TunnelsCritS);

            // reconfigure new tunnel socket
            _ProtoTunnelSocketConfig(pProtoTunnel, pProtoTunnel->pServerSocket);
        }
        else
        {
            NetPrintf(("prototunnel: --- reusing server socket ---\n"));
        }

        // add remote port to list
        if (!_ProtoTunnelServerPortListFunc(pProtoTunnel, (uint16_t)iValue, PROTOTUNNEL_PORTLIST_ADD))
        {
            NetPrintf(("prototunnel: [%p] could not add port to server port list\n", pProtoTunnel));
        }
        return(0);
    }
    if (iControl == 'clid')
    {
        NetPrintf(("prototunnel: [%p] setting local clientId=0x%08x\n", pProtoTunnel, iValue));
        pProtoTunnel->uLocalClientId = iValue;
        return(0);
    }
    #if DIRTYCODE_DEBUG
    if (iControl == 'drop')
    {
        pProtoTunnel->iPacketDrop = iValue;
        return(0);
    }
    #endif
    if ((iControl == 'flsh') || (iControl == 'rprt'))
    {
        ProtoTunnelT *pTunnel;
        int32_t iTunnel;
        uint32_t uCurTick = NetTick();

        // acquire exclusive access to tunnel list
        NetCritEnter(&pProtoTunnel->TunnelsCritS);
        NetCritEnter(&pProtoTunnel->TunnelsCritR);

        // flush specified tunnel
        for (iTunnel = 0; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel++)
        {
            pTunnel = &pProtoTunnel->Tunnels[iTunnel];
            if (pTunnel->uVirtualAddr == (unsigned)iValue)
            {
                if (iControl == 'flsh')
                {
                    NetPrintf(("prototunnel: [%p][%04d] explicitly flushing tunnel 0x%08x\n", pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), pTunnel->uVirtualAddr));
                    _ProtoTunnelBufferSend(pProtoTunnel, pTunnel, uCurTick);
                }
                if (iControl == 'rprt')
                {
                    NetPrintf(("prototunnel: [%p][%04d] updating remote port for tunnel 0x%08x to %d\n",
                        pProtoTunnel, (pTunnel - pProtoTunnel->Tunnels), pTunnel->uVirtualAddr, iValue2));
                    pTunnel->Info.uRemotePort = (unsigned)iValue2;
                }
                break;
            }
        }

        // release exclusive access to tunnel list
        NetCritLeave(&pProtoTunnel->TunnelsCritR);
        NetCritLeave(&pProtoTunnel->TunnelsCritS);

        // if we didn't find the tunnel
        if (iTunnel == pProtoTunnel->iMaxTunnels)
        {
            NetPrintf(("prototunnel: [%p] unable to find tunnel 0x%08x for '%c%c%c%c' operation\n",
                pProtoTunnel, iValue, (uint8_t)(iControl>>24), (uint8_t)(iControl>>16), (uint8_t)(iControl>>8), (uint8_t)iControl));
            return(-1);
        }
        return(0);
    }
    if (iControl == 'hmac')
    {
        int32_t iHashSize;
        // validate hmac type
        if ((iValue <= CRYPTHASH_NULL) || (iValue >= CRYPTHASH_NUMHASHES))
        {
            NetPrintf(("prototunnel: [%p] ignoring attempt to set invalid hmactype %d\n", pProtoTunnel, iValue));
            return(-1);
        }
        // validate hmac size
        iHashSize = CryptHashGetSize((CryptHashTypeE)iValue);
        if (iValue2 > iHashSize)
        {
            NetPrintf(("prototunnel: [%p] hmacsize %d is larger than max hmactype size %d; truncating\n", pProtoTunnel, iValue2, iHashSize));
            iValue2 = iHashSize;
        }
        if (iValue2 > PROTOTUNNEL_HMAC_MAXSIZE)
        {
            NetPrintf(("prototunnel: [%p] hmacsize %d is too large; truncating\n", pProtoTunnel, iValue2));
            iValue2 = PROTOTUNNEL_HMAC_MAXSIZE;
        }
        NetPrintf(("prototunnel: [%p] setting hmactype=%d and hmacsize=%d\n", pProtoTunnel, iValue, iValue2));
        pProtoTunnel->uHmacType = (uint8_t)iValue;
        pProtoTunnel->uHmacSize = (uint8_t)iValue2;
        return(0);
    }
    if (iControl == 'idle')
    {
        pProtoTunnel->iIdleCbRate = iValue;
        SocketCallback(pProtoTunnel->pSocket, CALLB_RECV, pProtoTunnel->iIdleCbRate, pProtoTunnel, &_ProtoTunnelRecvCallback);
        return(0);
    }
    if (iControl == 'rand')
    {
        NetPrintf(("prototunnel: setting retry random port on bind failure to %s\n", (uint8_t)iValue ? "TRUE" : "FALSE"));
        _ProtoTunnel_bRetryRandomOnFailure = (uint8_t)iValue;
        return(0);
    }
    if (iControl == 'rate')
    {
        pProtoTunnel->uFlushRate = iValue;
        return(0);
    }
    if (iControl == 'rmax')
    {
        pProtoTunnel->iMaxRecv = iValue;
        return(0);
    }
    if (iControl == 'rrcb')
    {
        NetPrintf(("prototunnel: [%p] 'rrcb' selector used to change raw recv callback from %p to %p\n", pProtoTunnel, pProtoTunnel->pRawRecvCallback, pValue));
        pProtoTunnel->pRawRecvCallback = (RawRecvCallbackT *)pValue;
        return(0);
    }
    if (iControl == 'rrud')
    {
        NetPrintf(("prototunnel: [%p] 'rrud' selector used to change raw recv callback user data from %p to %p\n", pProtoTunnel, pProtoTunnel->pRawRecvUserData, pValue));
        pProtoTunnel->pRawRecvUserData = (void *)pValue;
        return(0);
    }
    if (iControl == 'sock')
    {
        NetPrintf(("prototunnel: [%p] replacing tunnel socket\n", pProtoTunnel));

        // acquire exclusive access to tunnel list
        NetCritEnter(&pProtoTunnel->TunnelsCritS);
        NetCritEnter(&pProtoTunnel->TunnelsCritR);

        // close current tunnel socket
        if (pProtoTunnel->pSocket != NULL)
        {
            SocketClose(pProtoTunnel->pSocket);
            pProtoTunnel->pSocket = NULL;
        }
        // write in the new socket and configure it
        pProtoTunnel->pSocket = (SocketT *)pValue;
        _ProtoTunnelSocketConfig(pProtoTunnel, pProtoTunnel->pSocket);

        // release exclusive access to tunnel list
        NetCritLeave(&pProtoTunnel->TunnelsCritR);
        NetCritLeave(&pProtoTunnel->TunnelsCritS);
        return(0);
    }
    if (iControl == 'spam')
    {
        pProtoTunnel->iVerbosity = iValue;
        return(0);
    }
    if (iControl == 'tcid')
    {
        int32_t iTunnel = _ProtoTunnelIndexFromId(pProtoTunnel, (uint32_t)iValue);
        if (iTunnel != -1)
        {
            NetPrintf(("prototunnel: [%p][%04d] changing local client id from 0x%08x to 0x%08x\n", pProtoTunnel, iTunnel, pProtoTunnel->Tunnels[iTunnel].uLocalClientId, iValue2));
            pProtoTunnel->Tunnels[iTunnel].uLocalClientId = (uint32_t)iValue2;
            return(0);
        }
        else
        {
            NetPrintf(("prototunnel: [%p] 'tcid' control selector called with unknown tunnelid=0x%08x\n", pProtoTunnel, iValue));
            return(-1);
        }
    }
    if (iControl == 'vers')
    {
        if (iValue < PROTOTUNNEL_VERSION_MIN)
        {
            NetPrintf(("prototunnel [%p] ignoring attempt to set invalid version %d.%d\n", pProtoTunnel, iValue>>8, iValue&0xff));
            return(-1);
        }
        NetPrintf(("prototunnel: [%p] setting version to %d.%d\n", pProtoTunnel, iValue>>8, iValue&0xff));
        pProtoTunnel->uVersion = (uint16_t)iValue;
        return(0);
    }
    // pass-through to SocketControl()
    return(SocketControl(pProtoTunnel->pSocket, iControl, iValue, NULL, NULL));
}

/*F********************************************************************************/
/*!

    \Function ProtoTunnelUpdate

    \Description
        Update the module

    \Input *pProtoTunnel    - pointer to module state

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
void ProtoTunnelUpdate(ProtoTunnelRefT *pProtoTunnel)
{
    uint32_t uCurTick = NetTick();
    int32_t iTunnel;

    // time to flush?

    // acquire exclusive access to tunnel list
    NetCritEnter(&pProtoTunnel->TunnelsCritS);
    NetCritEnter(&pProtoTunnel->TunnelsCritR);

    // flush all tunnels
    // check individual flush timers, which are reset each send
    for (iTunnel = 0; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel++)
    {
        ProtoTunnelT *pTunnel = &(pProtoTunnel->Tunnels[iTunnel]);
        if (pTunnel->uVirtualAddr
            && (NetTickDiff(uCurTick, pTunnel->uLastTunnelSend) > (signed)pProtoTunnel->uFlushRate))
        {
            _ProtoTunnelBufferSend(pProtoTunnel, pTunnel, uCurTick);
        }
    }

    // release exclusive access to tunnel list
    NetCritLeave(&pProtoTunnel->TunnelsCritR);
    NetCritLeave(&pProtoTunnel->TunnelsCritS);
}

/*F*************************************************************************************/
/*!
    \Function    ProtoTunnelRawSendto

    \Description
        Send data to a remote host over the prototunnel socket. The destination address
        is supplied along with the data. Important: remote host shall not be expecting
        tunneled data because that function bypasses all the tunneling logic on the sending
        side.

    \Input *pProtoTunnel    - pointer to module state
    \Input *pBuf            - the data to be sent
    \Input iLen             - size of data
    \Input *pTo             - the address to send to
    \Input iToLen           - length of address

    \Output
        int32_t             - number of bytes sent or standard network error code (SOCKERR_xxx)

    \Version 07/12/2011 (mclouatre)
*/
/************************************************************************************F*/
int32_t ProtoTunnelRawSendto(ProtoTunnelRefT *pProtoTunnel, const char *pBuf, int32_t iLen, const struct sockaddr *pTo, int32_t iToLen)
{
    return(_ProtoTunnelSocketSendto(pProtoTunnel, pBuf, iLen, 0, pTo, iToLen));
}
