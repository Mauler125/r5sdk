/*H*************************************************************************************/
/*!
    \File protobufwrite.h

    \Description
        Interface of encoder for the Google Protobuf wire format
        See: https://developers.google.com/protocol-buffers/docs/encoding

    \Copyright
        Copyright (c) Electronic Arts 2017-2018. ALL RIGHTS RESERVED.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************H*/

#ifndef _protobufwrite_h
#define _protobufwrite_h

/*!
\Moduledef ProtobufWrite ProtobufWrite
\Modulemember Util
*/
//@{

/*** Includes **************************************************************************/

#include "DirtySDK/platform.h"

/*** Type Definitions ******************************************************************/

//! used for result for the write functions
typedef enum ProtobufWriteErrorE
{
    PROTOBUFWRITE_ERROR_OK,              //!< successful operation
    PROTOBUFWRITE_ERROR_FULL,            //!< the buffer is full
    PROTOBUFWRITE_ERROR_EMBED,           //!< called begin when out of slots or calling end when begin hasn't been called
} ProtobufWriteErrorE;

//! opaque module ref
typedef struct ProtobufWriteRefT ProtobufWriteRefT;

/*** Functions *************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

// initialize the encoder, must be called first
DIRTYCODE_API ProtobufWriteRefT *ProtobufWriteCreate(uint8_t *pBuffer, int32_t iBufLen, uint8_t bEncodeSize);

// write the varint int32, int64, uint32, uint64, bool or enum to the buffer
DIRTYCODE_API int32_t ProtobufWriteVarint(ProtobufWriteRefT *pState, uint64_t uValue, uint32_t uField);

// write repeated varint to the buffer
DIRTYCODE_API int32_t ProtobufWriteRepeatedVarint(ProtobufWriteRefT *pState, uint64_t *pValues, int32_t iCount, uint32_t uField);

// write the varint sint32 to the buffer (zigzag encoded)
DIRTYCODE_API int32_t ProtobufWriteSint32(ProtobufWriteRefT *pState, int32_t iValue, uint32_t uField);

// write repeated varint sint32 to the buffer (zigzag encoded)
DIRTYCODE_API int32_t ProtobufWriteRepeatedSint32(ProtobufWriteRefT *pState, int32_t *pValues, int32_t iCount, uint32_t uField);

// write the varint sint64 to the buffer (zigzag encoded)
DIRTYCODE_API int32_t ProtobufWriteSint64(ProtobufWriteRefT *pState, int64_t iValue, uint32_t uField);

// write repeated varint sint64 to the buffer (zigzag encoded)
DIRTYCODE_API int32_t ProtobufWriteRepeatedSint64(ProtobufWriteRefT *pState, int64_t *pValues, int32_t iCount, uint32_t uField);

// write fixed32, float or sfixed32 to the buffer
DIRTYCODE_API int32_t ProtobufWriteFixed32(ProtobufWriteRefT *pState, const void *pValue, uint32_t uField);

// write repeated fixed32, float or sfixed32 to the buffer
DIRTYCODE_API int32_t ProtobufWriteRepeatedFixed32(ProtobufWriteRefT *pState, const void *pInput, int32_t iInpLen, uint32_t uField);

// write fixed64, double or sfixed64 to the buffer
DIRTYCODE_API int32_t ProtobufWriteFixed64(ProtobufWriteRefT *pState, const void *pValue, uint32_t uField);

// write repeated fixed64, double or sfixed64 to the buffer
DIRTYCODE_API int32_t ProtobufWriteRepeatedFixed64(ProtobufWriteRefT *pState, const void *pInput, int32_t iInpLen, uint32_t uField);

// write the length delimited string, bytes or embedded message/packed repeated fields (if already encoded in pValue) to the buffer
DIRTYCODE_API int32_t ProtobufWriteLengthDelimited(ProtobufWriteRefT *pState, const void *pValue, int32_t iLength, uint32_t uField);

// alias function to write string
#define ProtobufWriteString(pState, pValue, iLength, uField) ProtobufWriteLengthDelimited(pState, pValue, iLength, uField)

// alias function to write bytes
#define ProtobufWriteBytes(pState, pValue, iLength, uField) ProtobufWriteLengthDelimited(pState, pValue, iLength, uField)

// begin an embedded message/packed repeated fields
DIRTYCODE_API int32_t ProtobufWriteMessageBegin(ProtobufWriteRefT *pState, uint32_t uField);

// end an embedded message/packed repeated fields
DIRTYCODE_API int32_t ProtobufWriteMessageEnd(ProtobufWriteRefT *pState);

// finish the writing of the message, cleanup the module state and return the size of the payload
DIRTYCODE_API int32_t ProtobufWriteDestroy(ProtobufWriteRefT *pState);

#if defined(__cplusplus)
}
#endif

//@}

#endif // _protobufwrite_h
