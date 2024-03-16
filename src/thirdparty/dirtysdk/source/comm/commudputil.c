/*H*************************************************************************************************/
/*!

    \File    commudputil.c

    \Description
        CommUdp knowledge to be shared with gameserver implementation.

     \Copyright
        Copyright (c) 2006-2017 Electronic Arts Inc.

    \Version    1.0  09/01/2017 (mclouatre) First Version
*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include "DirtySDK/comm/commudputil.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

/*** Private Functions *****************************************************************/

/*** Public Functions ******************************************************************/

/*F*************************************************************************************************/
/*!
    \Function    CommUDPUtilEncodeSubpacketSize

    \Description
        Encodes a subpacket size field.  A size of 0-250 is encoded in a single byte, while a size
        of 251-1530 requires two.  1530 bytes is the maximum allowed size of a subpacket.

    \Input *pBuf    - buffer holding encoded subpacket size (NULL to just calculate encoded size)
    \Input uVal     - subpacket size

    \Output
        uint32_t    - number of bytes that encoded size consumed (1 or 2)

    \Notes
        CommUDP uses a special encoding scheme to encode subpacket sizes in one or two bytes.  The
        original version of the protocol stored subpacket sizes up to 250 (the maximum previously
        allowed) in a single byte at the end of the subpacket data, and the newer scheme was
        designed to retain compatibility with that method.  Subpacket sizes up to 250 bytes are
        therefore still encoded in a single byte that immediately follows the subpacket data.  A
        value of 251 through 255 is used to represent larger subpacket sizes, which are encoded
        with an additional byte, stored immediately previous to the final byte.
        
        The following table shows how the subpacket size is encoded, with the first column
        containing the value of the final byte B0, and B1 representing the previous byte, if
        present.

        \verbatim
                           base10 equation to   equivalent expression in format:       resulting
            val of <B0>    get sub-pkt size     base10 + base2_prefix [base16 range]   sub-pkt size 
            -----------    ------------------   ------------------------------------   ------------ 
            0x00-0xFA   ->              <B0>  =           [0x00-0xFA]                =    0 -  250
            0xFB        -> 251 +    0 + <B1>  = 251 +  00 [0x00-0xFF]                =  251 -  506
            0xFC        -> 251 +  256 + <B1>  = 251 +  01 [0x00-0xFF]                =  507 -  762
            0xFD        -> 251 +  512 + <B1>  = 251 +  10 [0x00-0xFF]                =  763 - 1018
            0xFE        -> 251 +  768 + <B1>  = 251 +  11 [0x00-0xFF]                = 1019 - 1274
            0xFF        -> 251 + 1024 + <B1>  = 251 + 100 [0x00-0xFF]                = 1275 - 1530
        \endverbatim

    \Version 10/30/2015 (jbrookes)
*/
/*************************************************************************************************F*/
uint32_t CommUDPUtilEncodeSubpacketSize(uint8_t *pBuf, uint32_t uVal)
{
    uint32_t uOffset, uTemp;
    if (uVal > 250)
    {
        if (pBuf != NULL)
        {
            uTemp = uVal - 251;
            pBuf[1] = (uTemp >> 8) + 251;
            pBuf[0] = (uint8_t)(uTemp & 0xff);
        }
        uOffset = 2;
    }
    else
    {
        if (pBuf != NULL)
        {
            pBuf[0] = (uint8_t)uVal;
        }
        uOffset = 1;
    }
    return(uOffset);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPUtilDecodeSubpacketSize

    \Description
        Decodes a subpacket size field (see _CommUDPEncodeSubpacketSize for notes)

    \Input *pBuf    - buffer holding encoded subpacket size
    \Input *pVal    - [out] storage for decoded subpacket size

    \Output
        uint32_t    - number of bytes encoded size consumed (1 or 2)

    \Version 10/30/2015 (jbrookes)
*/
/*************************************************************************************************F*/
uint32_t CommUDPUtilDecodeSubpacketSize(const uint8_t *pBuf, int32_t *pVal)
{
    uint32_t uOffset, uTemp;
    if (pBuf[0] > 250)
    {
        pBuf -= 1;
        uTemp = pBuf[1] - 251;
        *pVal = 251 + (pBuf[0] | (uTemp << 8));
        uOffset = 2;
    }
    else
    {
        *pVal = pBuf[0];
        uOffset = 1;
    }
    return(uOffset);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPUtilGetMetaTypeFromSeq

    \Description
        Extracts metatype from host-ordered commupd header seq field (RawUDPPacketT.body.uSeq)

    \Input uSeq     - host-ordered commupd header seq field

    \Output
        uint32_t    - extracted meta type value

    \Version 09/01/2017 (mclouatre)
*/
/*************************************************************************************************F*/
uint32_t CommUDPUtilGetMetaTypeFromSeq(uint32_t uSeq)
{
    return((uSeq >> COMMUDP_SEQ_META_SHIFT) & 0xf);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPUtilGetMetaSize

    \Description
        Returns the metadata size for a given metadata type.

    \Input uMetaType    - metadata type

    \Output
        uint32_t        - metadata size

    \Version 09/01/2017 (mclouatre)
*/
/*************************************************************************************************F*/
uint32_t CommUDPUtilGetMetaSize(uint32_t uMetaType)
{
    return(uMetaType ? COMMUDP_RAW_METATYPE1_SIZE : 0);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPUtilGetExtraSubPktCountFromSeq

    \Description
        Extracts the number of extra subpackets (excluding the main subpacket) from host-ordered
        commupd header seq field (RawUDPPacketT.body.uSeq)

    \Input uSeq     - host-ordered commupd header seq field

    \Output
        uint32_t    - extracted extra subpackets count (does not include the main subpacket)

    \Version 09/01/2017 (mclouatre)
*/
/*************************************************************************************************F*/
uint32_t CommUDPUtilGetExtraSubPktCountFromSeq(uint32_t uSeq)
{
    return(uSeq >> COMMUDP_SEQ_MULTI_SHIFT);
}
