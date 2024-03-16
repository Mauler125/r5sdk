/*H*************************************************************************************/
/*!
    \File protobufcommon.h

    \Description
        Shared definitions and functionality for the protobuf encoder / decoder

    \Copyright
        Copyright (c) Electronic Arts 2017-2018. ALL RIGHTS RESERVED.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************H*/

#ifndef _protobufcommon_h
#define _protobufcommon_h

/*!
\Moduledef ProtobufCommon ProtobufCommon
\Modulemember Util
*/
//@{

/*** Include Files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Type Definitions ******************************************************************/

//! different field types that are supported by protobuf. note, the order is needed to follow the defined values of the protocol
typedef enum ProtobufTypeE
{
    PROTOBUF_TYPE_VARINT,           //!< int32, int64, uint32, uint64, sint32, sint64, bool, enum
    PROTOBUF_TYPE_64BIT,            //!< fixed64 (uint64_t), sfixed64 (int64_t), double
    PROTOBUF_TYPE_LENGTH_DELIMITED, //!< string, bytes, embedded messages, packed repeated fields
    PROTOBUF_TYPE_START_GROUP,      //!< groups (deprecated)
    PROTOBUF_TYPE_END_GROUP,        //!< groups (deprecated)
    PROTOBUF_TYPE_32BIT             //!< fixed32 (uint32_t), sfixed32 (int32_t), float
} ProtobufTypeE;

/*** Functions *************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

// read the message size out of the payload
DIRTYCODE_API const uint8_t *ProtobufCommonReadSize(const uint8_t *pBuffer, int32_t iBufLen, int32_t *pResult);

// get the string representation of a type
DIRTYCODE_API const char *ProtobufCommonGetType(ProtobufTypeE eType);

#if defined(__cplusplus)
}
#endif

//@}

#endif // _protobufcommon_h
