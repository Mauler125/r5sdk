/*H*************************************************************************************************/
/*!

    \File    commudputil.h

    \Description
        CommUdp knowledge to be shared with gameserver implementation.

     \Copyright
        Copyright (c) 2006-2017 Electronic Arts Inc.

     \Version    1.0  09/01/2017 (mclouatre) First Version

*/
/*************************************************************************************************H*/


#ifndef _commudputil_h
#define _commudputil_h

/*!
\Moduledef CommUDP CommUDP
\Modulemember Comm
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

//! define protocol packet types
enum {
    COMMUDP_RAW_PACKET_INIT = 1,        // initiate a connection
    COMMUDP_RAW_PACKET_CONN,            // confirm a connection
    COMMUDP_RAW_PACKET_DISC,            // terminate a connection
    COMMUDP_RAW_PACKET_NAK,             // force resend of lost data
    COMMUDP_RAW_PACKET_POKE,            // try and poke through firewall

    COMMUDP_RAW_PACKET_UNREL = 128,     // unreliable packet send (must be power of two)
                                        // 128-255 reserved for unreliable packet sequence
    COMMUDP_RAW_PACKET_DATA = 256,      // initial data packet sequence number (must be power of two)

    /*  Width of the sequence window, can be anything provided
        RAW_PACKET_DATA + RAW_PACKET_DATA_WINDOW
        doesn't overlap the meta-data bits. */
    COMMUDP_RAW_PACKET_DATA_WINDOW = (1 << 24) - 256
};

#define COMMUDP_RAW_METATYPE1_SIZE  (8)
#define COMMUDP_SEQ_META_SHIFT      (28 - 4)
#define COMMUDP_SEQ_MULTI_SHIFT     (28)
#define COMMUDP_SEQ_MULTI_INC       (1 << COMMUDP_SEQ_MULTI_SHIFT)
#define COMMUDP_SEQ_MASK            (0x00ffffff)


/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// Encodes a subpacket size field.
DIRTYCODE_API uint32_t CommUDPUtilEncodeSubpacketSize(uint8_t *pBuf, uint32_t uVal);

// Decodes a subpacket size field (see _CommUDPEncodeSubpacketSize for notes)
DIRTYCODE_API uint32_t CommUDPUtilDecodeSubpacketSize(const uint8_t *pBuf, int32_t *pVal);

// Extracts metatype from host-ordered commupd header seq field (RawUDPPacketT.body.uSeq)
DIRTYCODE_API uint32_t CommUDPUtilGetMetaTypeFromSeq(uint32_t uSeq);

// Returns the metadata size for a given metadata type.
DIRTYCODE_API uint32_t CommUDPUtilGetMetaSize(uint32_t uMetaType);

// Extracts the number of extra subpackets (excluding the main subpacket) from host - ordered commupd header seq field(RawUDPPacketT.body.uSeq)
DIRTYCODE_API uint32_t CommUDPUtilGetExtraSubPktCountFromSeq(uint32_t uSeq);

#ifdef __cplusplus
}
#endif

//@}

#endif // _commudputil_h

