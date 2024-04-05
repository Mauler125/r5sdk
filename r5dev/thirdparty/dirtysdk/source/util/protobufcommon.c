/*H*************************************************************************************/
/*!
    \File protobufcommon.h

    \Description
        Shared definitions and functionality for the protobuf encoder / decoder

    \Copyright
        Copyright (c) Electronic Arts 2017. ALL RIGHTS RESERVED.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************H*/

/*** Include Files *********************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/util/protobufcommon.h"

/*** Varibles **************************************************************************/

//! strings representation for debugging
static const char *_ProtobufCommon_aTypes[] =
{
    "PROTOBUF_TYPE_VARINT",
    "PROTOBUF_TYPE_64BIT",
    "PROTOBUF_TYPE_LENGTH_DELIMITED",
    "PROTOBUF_TYPE_START_GROUP",
    "PROTOBUF_TYPE_END_GROUP",
    "PROTOBUF_TYPE_32BIT"
};

/*** Public Functions ******************************************************************/

/*F*************************************************************************************/
/*!
    \Function ProtobufCommonReadSize

    \Description
        Reads the encoded size of the message from the buffer

    \Input *pBuffer     - buffer we are using to read
    \Input iBufLen      - size of the buffer
    \Input *pResult     - [out] size of the message

    \Output
        const uint8_t * - location past the encoded size or NULL if iBufLen is less than
                          4 bytes or the size of the message

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
const uint8_t *ProtobufCommonReadSize(const uint8_t *pBuffer, int32_t iBufLen, int32_t *pResult)
{
    *pResult = 0;

    /* if the full size of the buffer is less than the size
       we cannot read the message size */
    if (iBufLen < 4)
    {
        return(pBuffer);
    }
    iBufLen -= 4;

    *pResult |= (*pBuffer++ << 24);
    *pResult |= (*pBuffer++ << 16);
    *pResult |= (*pBuffer++ <<  8);
    *pResult |= (*pBuffer++ <<  0);

    // make sure that the size of the message isn't larger than the buffer
    return((*pResult <= iBufLen) ? pBuffer : NULL);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufCommonGetType

    \Description
        Converts the type to its string representation

    \Input eType        - type we are converting

    \Output
        const char *    - string representation or empty string if no mapping found

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
const char *ProtobufCommonGetType(ProtobufTypeE eType)
{
    return((eType >= PROTOBUF_TYPE_VARINT) && (eType <= PROTOBUF_TYPE_32BIT) ? _ProtobufCommon_aTypes[eType] : "");
}

