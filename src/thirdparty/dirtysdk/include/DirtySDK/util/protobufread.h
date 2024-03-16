/*H*************************************************************************************/
/*!
    \File protobufread.h

    \Description
        Interface of decoder for the Google Protobuf wire format
        See: https://developers.google.com/protocol-buffers/docs/encoding

        This module only supports protobuf version 3, if any lesser versions are used
        the result is undefined.

    \Copyright
        Copyright (c) Electronic Arts 2017-2018. ALL RIGHTS RESERVED.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************H*/

#ifndef _protobufread_h
#define _protobufread_h

/*!
\Moduledef ProtobufRead ProtobufRead
\Modulemember Util
*/
//@{

/*** Includes **************************************************************************/

#include "DirtySDK/platform.h"

/*** Type Definitions ******************************************************************/

//! struct for ease of use for the API
typedef struct ProtobufReadT
{
    const uint8_t *pBuffer; //!< pointer to the start of the buffer
    const uint8_t *pBufEnd; //!< pointer to the end of the buffer
} ProtobufReadT;

/*** Functions *************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

// initialize the state
DIRTYCODE_API void ProtobufReadInit(ProtobufReadT *pState, const uint8_t *pBuffer, int32_t iBufLen);

// find a field by identifier
DIRTYCODE_API const uint8_t *ProtobufReadFind(const ProtobufReadT *pState, uint32_t uField);

// find a field by identifier that allows continuation for non-packed repeated fields
DIRTYCODE_API const uint8_t *ProtobufReadFind2(const ProtobufReadT *pState, uint32_t uField, const uint8_t *pBuffer);

// read the current field header
DIRTYCODE_API const uint8_t *ProtobufReadHeader(const ProtobufReadT *pState, const uint8_t *pCurrent, uint32_t *pField, uint8_t *pType);

// skip over a field
DIRTYCODE_API const uint8_t *ProtobufReadSkip(const ProtobufReadT *pState, const uint8_t *pCurrent, uint8_t uType);

// read the varint int32, int64, uint32, uint64, bool or enum from the buffer
DIRTYCODE_API uint64_t ProtobufReadVarint(const ProtobufReadT *pState, const uint8_t *pCurrent);

// alternative version used in conjunction with repeated field reading
DIRTYCODE_API const uint8_t *ProtobufReadVarint2(const ProtobufReadT *pState, const uint8_t *pCurrent, uint64_t *pResult);

// read the repeated varint int32, int64, uint32, uint64, bool or enum from the buffer
DIRTYCODE_API const uint8_t *ProtobufReadRepeatedVarint(const ProtobufReadT *pState, const uint8_t *pCurrent, uint64_t *pResult, int32_t iCount);

// read the varint sint32 from the buffer (zigzag decoded)
DIRTYCODE_API const uint8_t *ProtobufReadSint32(const ProtobufReadT *pState, const uint8_t *pCurrent, int32_t *pResult);

// read the repeated varint sint32 from the buffer (zigzag decoded)
DIRTYCODE_API const uint8_t *ProtobufReadRepeatedSint32(const ProtobufReadT *pState, const uint8_t *pCurrent, int32_t *pResult, int32_t iCount);

// read the varint sint64 from the buffer (zigzag decoded)
DIRTYCODE_API const uint8_t *ProtobufReadSint64(const ProtobufReadT *pState, const uint8_t *pCurrent, int64_t *pResult);

// read the repeated varint sint64 from the buffer (zigzag decoded)
DIRTYCODE_API const uint8_t *ProtobufReadRepeatedSint64(const ProtobufReadT *pState, const uint8_t *pCurrent, int64_t *pResult, int32_t iCount);

// read fixed32, float or sfixed32 from the buffer
DIRTYCODE_API const uint8_t *ProtobufReadFixed32(const ProtobufReadT *pState, const uint8_t *pCurrent, void *pOutput);

// read repeated fixed32, float or sfixed32 from the buffer
DIRTYCODE_API const uint8_t *ProtobufReadRepeatedFixed32(const ProtobufReadT *pState, const uint8_t *pCurrent, void *pOutput, int32_t iOutLen);

// read fixed64, double or sfixed64 from the buffer
DIRTYCODE_API const uint8_t *ProtobufReadFixed64(const ProtobufReadT *pState, const uint8_t *pCurrent, void *pOutput);

// read repeated fixed64, double or sfixed64 from the buffer
DIRTYCODE_API const uint8_t *ProtobufReadRepeatedFixed64(const ProtobufReadT *pState, const uint8_t *pCurrent, void *pOutput, int32_t iOutLen);

// read the bytes from the buffer
DIRTYCODE_API const uint8_t *ProtobufReadBytes(const ProtobufReadT *pState, const uint8_t *pCurrent, uint8_t *pOutput, int32_t iOutLen);

// read the string from the buffer
DIRTYCODE_API const uint8_t *ProtobufReadString(const ProtobufReadT *pState, const uint8_t *pCurrent, char *pOutput, int32_t iOutLen);

// read an embedded message information from the buffer
DIRTYCODE_API const uint8_t *ProtobufReadMessage(const ProtobufReadT *pState, const uint8_t *pCurrent, ProtobufReadT *pMsg);

// read the number of elements in the repeated field
DIRTYCODE_API int32_t ProtobufReadNumRepeatedElements(const ProtobufReadT *pState, uint32_t uField, uint8_t uType);

#if defined(__cplusplus)
}
#endif

//@}

#endif // _protobufread_h
