/*H*************************************************************************************/
/*!
    \File protobufwrite.c

    \Description
        Implementation of encoder for the Google Protobuf wire format
        See: https://developers.google.com/protocol-buffers/docs/encoding

    \Copyright
        Copyright (c) Electronic Arts 2017-2018. ALL RIGHTS RESERVED.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************H*/

/*** Includes **************************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/util/protobufcommon.h"
#include "DirtySDK/util/protobufwrite.h"

/*** Defines ***************************************************************************/

//! maximum octets used to encode a varint
#define PROTOBUF_VARINT_MAX (10)

//! max number of levels that we can embed messages
#define PROTOBUF_MAX_EMBEDDED_MSG (100)

/*** Macros ****************************************************************************/

//! encodes the field number and type into a tag
#define PROTOBUF_MakeTag(uField, uType) (((uField) << 3) | (uType))

/*** Type Definitions ******************************************************************/

//! module state
struct ProtobufWriteRefT
{
    uint8_t *pBuffer;   //!< pointer to the start of the buffer
    int32_t iBufLen;    //!< length of the buffer
    int32_t iBufOff;    //!< offset into the buffer

    int32_t iMemGroup;          //!< memory group identifier
    void *pMemGroupUserdata;    //!< memory group data

    uint8_t *aMessagesBegin[PROTOBUF_MAX_EMBEDDED_MSG]; //!< stack of pointers to where each encoded messages begin
    int32_t iNumMessages;                               //!< current number of messages we are embedding

    uint8_t bEncodeSize;    //!< should we encode the size of the message at the beginning of the buffer? (needed for grpc)
    uint8_t _pad[3];
};

/*** Variables *************************************************************************/

// variable to test if something is set
static const uint8_t _aEmptyBuffer[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

/*** Private Functions *****************************************************************/

/*F*************************************************************************************/
/*!
    \Function _ProtobufWriteVarint

    \Description
        Write a varint type to the buffer

    \Input uValue   - varint value to write
    \Input *pBuffer - [out] destination we are writing to
    \Input iBufLen  - size of the output destination
    \Input *pError  - [out] where we write the result of the operation

    \Output
        int32_t     - amount of bytes written to the buffer

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
static int32_t _ProtobufWriteVarint(uint64_t uValue, uint8_t *pBuffer, int32_t iBufLen, int32_t *pError)
{
    int32_t iWrite;

    /* make we can at least write 1 byte for the cases
       where we can fit it in one octet */
    if (iBufLen <= 0)
    {
        *pError = PROTOBUFWRITE_ERROR_FULL;
        return(0);
    }

    for (iWrite = 0; uValue >= 0x80; iWrite += 1)
    {
        // make sure we have space left
        if (iWrite >= (iBufLen - 1))
        {
            *pError = PROTOBUFWRITE_ERROR_FULL;
            return(0);
        }

        // set the lsb of the value and set the msb as a continuation flag
        pBuffer[iWrite] = (uValue % 0x80) + 0x80;
        uValue /= 0x80;
    }
    pBuffer[iWrite++] = (uint8_t)uValue;

    return(iWrite);
}

/*F*************************************************************************************/
/*!
    \Function _ProtobufMemcpy

    \Description
        Copies data to the buffer

    \Input *pDst    - [out] pointer to output
    \Input iDstLen  - size of output
    \Input *pSrc    - pointer to input
    \Input iSrcLen  - size of input
    \Input *pError  - [out] where we write the result of the operation

    \Output
        int32_t     - amount of bytes written to the buffer

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
static int32_t _ProtobufMemcpy(void *pDst, int32_t iDstLen, const void *pSrc, int32_t iSrcLen, int32_t *pError)
{
    if ((iDstLen = DS_MIN(iDstLen, iSrcLen)) > 0)
    {
        ds_memcpy(pDst, pSrc, iDstLen);
    }
    else
    {
        *pError = PROTOBUFWRITE_ERROR_FULL;
    }
    return(iDstLen);
}

/*** Public Functions ******************************************************************/

/*F*************************************************************************************/
/*!
    \Function ProtobufWriteCreate

    \Description
        Create and initialize the writer module ref

    \Input *pBuffer             - where we are writing to
    \Input iBufLen              - the size of the buffer we are writing to
    \Input bEncodeSize          - encode the size of the message at the beginning of the buffer?

    \Output
        ProtobufWriteRefT *    - pointer to module ref or NULL if failure

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
ProtobufWriteRefT *ProtobufWriteCreate(uint8_t *pBuffer, int32_t iBufLen, uint8_t bEncodeSize)
{
    ProtobufWriteRefT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserdata;

    // query memory group information
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserdata);

    // allocate module state
    if ((pState = (ProtobufWriteRefT *)DirtyMemAlloc(sizeof(*pState), PROTOBUF_MEMID, iMemGroup, pMemGroupUserdata)) == NULL)
    {
        NetPrintf(("protobufwriter: cannot allocate writer module state\n"));
        return(NULL);
    }
    ds_memclr(pState, sizeof(*pState));
    pState->pBuffer = pBuffer;
    pState->iBufLen = iBufLen;
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserdata = pMemGroupUserdata;

    // assign the encode size setting, if true save space at the beginning for encoding size
    if ((pState->bEncodeSize = bEncodeSize) == TRUE)
    {
        pState->iBufOff += 4;
    }

    return(pState);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufWriteVarint

    \Description
        Write the non-zigzag encoded (sint32, sint64) versions of the varint types

    \Input *pState      - module state
    \Input uValue       - value we are writing
    \Input uField       - field identifier

    \Output
        int32_t         - 0 (PROTOBUFWRITE_ERROR_OK) on success, >0 on error

    \Notes
        This function supports the following types: int32, uint32, uint64, bool and
        enum.

        If you pass in 0 for uField, nothing will be written to the buffer for the tag
        which you only want to do when writing packed repeated fields.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
int32_t ProtobufWriteVarint(ProtobufWriteRefT *pState, uint64_t uValue, uint32_t uField)
{
    int32_t iError = PROTOBUFWRITE_ERROR_OK;

    // don't write the defaults to save space when not in a repeated field
    if ((uValue != 0) || (uField == 0))
    {
        if (uField > 0)
        {
            pState->iBufOff += _ProtobufWriteVarint(PROTOBUF_MakeTag(uField, PROTOBUF_TYPE_VARINT), pState->pBuffer+pState->iBufOff, pState->iBufLen-pState->iBufOff, &iError);
        }
        pState->iBufOff += _ProtobufWriteVarint(uValue, pState->pBuffer+pState->iBufOff, pState->iBufLen-pState->iBufOff, &iError);
    }
    return(iError);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufWriteRepeatedVarint

    \Description
        Write the non-zigzag encoded (sint32, sint64) versions of the varint types
        repeated

    \Input *pState      - module state
    \Input *pValues     - array of values we are writing
    \Input iCount       - number of entries in the array
    \Input uField       - field identifier

    \Output
        int32_t         - 0 (PROTOBUFWRITE_ERROR_OK) on success, >0 on error

    \Notes
        This function supports the following types: int32, uint32, uint64, bool and
        enum.

    \Version 12/05/2017 (eesponda)
*/
/*************************************************************************************F*/
int32_t ProtobufWriteRepeatedVarint(ProtobufWriteRefT *pState, uint64_t *pValues, int32_t iCount, uint32_t uField)
{
    int32_t iIndex, iError;

    ProtobufWriteMessageBegin(pState, uField);
    for (iIndex = 0, iError = PROTOBUFWRITE_ERROR_OK; (iIndex < iCount) && (iError == PROTOBUFWRITE_ERROR_OK); iIndex += 1)
    {
        iError = ProtobufWriteVarint(pState, pValues[iIndex], 0);
    }
    ProtobufWriteMessageEnd(pState);

    return(iError);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufWriteSint32

    \Description
        Write the zigzag encoded sint32 type

    \Input *pState      - module state
    \Input iValue       - value we are writing
    \Input uField       - field identifier

    \Output
        int32_t         - 0 (PROTOBUFWRITE_ERROR_OK) on success, >0 on error

    \Notes
        If you pass in 0 for uField, nothing will be written for the tag
        which you only want to do when writing packed repeated fields.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
int32_t ProtobufWriteSint32(ProtobufWriteRefT *pState, int32_t iValue, uint32_t uField)
{
    int32_t iError = PROTOBUFWRITE_ERROR_OK;
    // zigzag encode the value
    const uint64_t uValue = (iValue << 1) ^ (iValue >> 31);

    // don't write the defaults to save space when not in a repeated field
    if ((uValue != 0) || (uField == 0))
    {
        if (uField > 0)
        {
            pState->iBufOff += _ProtobufWriteVarint(PROTOBUF_MakeTag(uField, PROTOBUF_TYPE_VARINT), pState->pBuffer+pState->iBufOff, pState->iBufLen-pState->iBufOff, &iError);
        }
        pState->iBufOff += _ProtobufWriteVarint(uValue, pState->pBuffer+pState->iBufOff, pState->iBufLen-pState->iBufOff, &iError);
    }
    return(iError);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufWriteRepeatedSint32

    \Description
        Write the zigzag encoded sint32 type repeated

    \Input *pState      - module state
    \Input *pValues     - array of values we are writing
    \Input iCount       - number of entries in the array
    \Input uField       - field identifier

    \Output
        int32_t         - 0 (PROTOBUFWRITE_ERROR_OK) on success, >0 on error

    \Version 12/05/2017 (eesponda)
*/
/*************************************************************************************F*/
int32_t ProtobufWriteRepeatedSint32(ProtobufWriteRefT *pState, int32_t *pValues, int32_t iCount, uint32_t uField)
{
    int32_t iIndex, iError;

    ProtobufWriteMessageBegin(pState, uField);
    for (iIndex = 0, iError = PROTOBUFWRITE_ERROR_OK; (iIndex < iCount) && (iError == PROTOBUFWRITE_ERROR_OK); iIndex += 1)
    {
        iError = ProtobufWriteSint32(pState, pValues[iIndex], 0);
    }
    ProtobufWriteMessageEnd(pState);

    return(iError);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufWriteSint64

    \Description
        Write the zigzag encoded sint64 type

    \Input *pState      - module state
    \Input iValue       - value we are writing
    \Input uField       - field identifier

    \Output
        int32_t         - 0 (PROTOBUFWRITE_ERROR_OK) on success, >0 on error

    \Notes
        If you pass in 0 for uField, nothing will be written for the tag
        which you only want to do when writing packed repeated fields.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
int32_t ProtobufWriteSint64(ProtobufWriteRefT *pState, int64_t iValue, uint32_t uField)
{
    int32_t iError = PROTOBUFWRITE_ERROR_OK;
    // zigzag encode the value
    const uint64_t uValue = (iValue << 1) ^ (iValue >> 63);

    // don't write the defaults to save space when not in a repeated field
    if ((uValue != 0) || (uField == 0))
    {
        if (uField > 0)
        {
            pState->iBufOff += _ProtobufWriteVarint(PROTOBUF_MakeTag(uField, PROTOBUF_TYPE_VARINT), pState->pBuffer+pState->iBufOff, pState->iBufLen-pState->iBufOff, &iError);
        }
        pState->iBufOff += _ProtobufWriteVarint(uValue, pState->pBuffer+pState->iBufOff, pState->iBufLen-pState->iBufOff, &iError);
    }
    return(iError);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufWriteRepeatedSint64

    \Description
        Write the zigzag encoded sint64 type repeated

    \Input *pState      - module state
    \Input *pValues     - array of values we are writing
    \Input iCount       - number of entries in the array
    \Input uField       - field identifier

    \Output
        int32_t         - 0 (PROTOBUFWRITE_ERROR_OK) on success, >0 on error

    \Version 12/05/2017 (eesponda)
*/
/*************************************************************************************F*/
int32_t ProtobufWriteRepeatedSint64(ProtobufWriteRefT *pState, int64_t *pValues, int32_t iCount, uint32_t uField)
{
    int32_t iIndex, iError;

    ProtobufWriteMessageBegin(pState, uField);
    for (iIndex = 0, iError = PROTOBUFWRITE_ERROR_OK; (iIndex < iCount) && (iError == PROTOBUFWRITE_ERROR_OK); iIndex += 1)
    {
        iError = ProtobufWriteSint64(pState, pValues[iIndex], 0);
    }
    ProtobufWriteMessageEnd(pState);

    return(iError);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufWriteFixed32

    \Description
        Write the fixed32, float or sfixed32 type to the buffer

    \Input *pState      - module state
    \Input *pValue      - value we are writing
    \Input uField       - field identifier

    \Output
        int32_t         - 0 (PROTOBUFWRITE_ERROR_OK) on success, >0 on error

    \Notes
        The fixed types are written in little endian so we use memcpy.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
int32_t ProtobufWriteFixed32(ProtobufWriteRefT *pState, const void *pValue, uint32_t uField)
{
    int32_t iError = PROTOBUFWRITE_ERROR_OK;

    // don't write the defaults to save space
    if (memcmp(pValue, _aEmptyBuffer, 4) != 0)
    {
        pState->iBufOff += _ProtobufWriteVarint(PROTOBUF_MakeTag(uField, PROTOBUF_TYPE_32BIT), pState->pBuffer+pState->iBufOff, pState->iBufLen-pState->iBufOff, &iError);
        pState->iBufOff += _ProtobufMemcpy(pState->pBuffer+pState->iBufOff, pState->iBufLen-pState->iBufOff, pValue, 4, &iError);
    }
    return(iError);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufWriteRepeatedFixed32

    \Description
        Write the repeated fixed32, float or sfixed32 type to the buffer

    \Input *pState      - module state
    \Input *pInput      - array of values we are writing
    \Input iInpLen      - size (in bytes) of the array
    \Input uField       - field identifier

    \Output
        int32_t         - 0 (PROTOBUFWRITE_ERROR_OK) on success, >0 on error

    \Version 12/05/2017 (eesponda)
*/
/*************************************************************************************F*/
int32_t ProtobufWriteRepeatedFixed32(ProtobufWriteRefT *pState, const void *pInput, int32_t iInpLen, uint32_t uField)
{
    int32_t iError = PROTOBUFWRITE_ERROR_OK;

    ProtobufWriteMessageBegin(pState, uField);
    pState->iBufOff += _ProtobufMemcpy(pState->pBuffer+pState->iBufOff, pState->iBufLen-pState->iBufOff, pInput, iInpLen, &iError);
    ProtobufWriteMessageEnd(pState);

    return(iError);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufWriteFixed64

    \Description
        Write the fixed64, double or sfixed64 type to the buffer

    \Input *pState      - module state
    \Input *pValue      - value we are writing
    \Input uField       - field identifier

    \Output
        int32_t         - 0 (PROTOBUFWRITE_ERROR_OK) on success, >0 on error

    \Notes
        The fixed types are written in little endian so we use memcpy.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
int32_t ProtobufWriteFixed64(ProtobufWriteRefT *pState, const void *pValue, uint32_t uField)
{
    int32_t iError = PROTOBUFWRITE_ERROR_OK;

    // don't write the defaults to save space
    if (memcmp(pValue, _aEmptyBuffer, 8) != 0)
    {
        pState->iBufOff += _ProtobufWriteVarint(PROTOBUF_MakeTag(uField, PROTOBUF_TYPE_64BIT), pState->pBuffer+pState->iBufOff, pState->iBufLen-pState->iBufOff, &iError);
        pState->iBufOff += _ProtobufMemcpy(pState->pBuffer+pState->iBufOff, pState->iBufLen-pState->iBufOff, pValue, 8, &iError);
    }
    return(iError);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufWriteRepeatedFixed64

    \Description
        Write the repeated fixed64, double or sfixed64 type to the buffer

    \Input *pState      - module state
    \Input *pInput      - array of values we are writing
    \Input iInpLen      - size (in bytes) of the array
    \Input uField       - field identifier

    \Output
        int32_t         - 0 (PROTOBUFWRITE_ERROR_OK) on success, >0 on error

    \Version 12/05/2017 (eesponda)
*/
/*************************************************************************************F*/
int32_t ProtobufWriteRepeatedFixed64(ProtobufWriteRefT *pState, const void *pInput, int32_t iInpLen, uint32_t uField)
{
    int32_t iError = PROTOBUFWRITE_ERROR_OK;

    ProtobufWriteMessageBegin(pState, uField);
    pState->iBufOff += _ProtobufMemcpy(pState->pBuffer+pState->iBufOff, pState->iBufLen-pState->iBufOff, pInput, iInpLen, &iError);
    ProtobufWriteMessageEnd(pState);

    return(iError);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufWriteLengthDelimited

    \Description
        Write the length delimited types

    \Input *pState      - module state
    \Input *pValue      - value we are writing
    \Input iLength      - size of the value
    \Input uField       - field identifier

    \Output
        int32_t         - 0 (PROTOBUFWRITE_ERROR_OK) on success, >0 on error

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
int32_t ProtobufWriteLengthDelimited(ProtobufWriteRefT *pState, const void *pValue, int32_t iLength, uint32_t uField)
{
    int32_t iError = PROTOBUFWRITE_ERROR_OK;

    // don't write empty data to save space when not in a repeated field
    if ((pValue != NULL) && (iLength > 0))
    {
        pState->iBufOff += _ProtobufWriteVarint(PROTOBUF_MakeTag(uField, PROTOBUF_TYPE_LENGTH_DELIMITED), pState->pBuffer+pState->iBufOff, pState->iBufLen-pState->iBufOff, &iError);
        pState->iBufOff += _ProtobufWriteVarint(iLength, pState->pBuffer+pState->iBufOff, pState->iBufLen-pState->iBufOff, &iError);
        pState->iBufOff += _ProtobufMemcpy(pState->pBuffer+pState->iBufOff, pState->iBufLen-pState->iBufOff, pValue, iLength, &iError);
    }
    return(iError);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufWriteMessageBegin

    \Description
        Begin encoding an embedded message in place

    \Input *pState      - module state
    \Input uField       - field identifier

    \Output
        int32_t         - 0 (PROTOBUFWRITE_ERROR_OK) on success, >0 on error

    \Notes
        Embedded messages work like a stack (FILO) when nesting more messages.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
int32_t ProtobufWriteMessageBegin(ProtobufWriteRefT *pState, uint32_t uField)
{
    int32_t iError = PROTOBUFWRITE_ERROR_OK;

    // make sure that we have not reached our maximum
    if (pState->iNumMessages == PROTOBUF_MAX_EMBEDDED_MSG)
    {
        return(PROTOBUFWRITE_ERROR_EMBED);
    }

    pState->iBufOff += _ProtobufWriteVarint(PROTOBUF_MakeTag(uField, PROTOBUF_TYPE_LENGTH_DELIMITED), pState->pBuffer+pState->iBufOff, pState->iBufLen-pState->iBufOff, &iError);
    pState->aMessagesBegin[pState->iNumMessages++] = pState->pBuffer+pState->iBufOff;

    if ((pState->iBufLen-pState->iBufOff) > PROTOBUF_VARINT_MAX)
    {
        pState->iBufOff += PROTOBUF_VARINT_MAX;
    }
    else
    {
        iError = PROTOBUFWRITE_ERROR_FULL;
    }

    return(iError);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufWriteMessageEnd

    \Description
        Finish encoding an embedded message in place

    \Input *pState      - module state

    \Output
        int32_t         - 0 (PROTOBUFWRITE_ERROR_OK) on success, >0 on error

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
int32_t ProtobufWriteMessageEnd(ProtobufWriteRefT *pState)
{
    int32_t iError = PROTOBUFWRITE_ERROR_OK, iInpLen, iBufOff;
    uint8_t *pMsgBegin;

    // make sure that we still have messages in the stack
    if (pState->iNumMessages > 0)
    {
        pMsgBegin = pState->aMessagesBegin[--pState->iNumMessages];
    }
    else
    {
        return(PROTOBUFWRITE_ERROR_EMBED);
    }

    // calculate the size of the message and encode it to the buffer
    iInpLen = (int32_t)((pState->pBuffer+pState->iBufOff)-pMsgBegin-PROTOBUF_VARINT_MAX);
    iBufOff = _ProtobufWriteVarint(iInpLen, pMsgBegin, PROTOBUF_VARINT_MAX, &iError);

    // fill the gap (if any)
    memmove(pMsgBegin+iBufOff, pMsgBegin+PROTOBUF_VARINT_MAX, iInpLen);
    pState->iBufOff -= (PROTOBUF_VARINT_MAX-iBufOff);

    return(iError);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufWriteDestroy

    \Description
        Finish writing to the buffer and return the amount of data written

    \Input *pState      - module state

    \Output
        int32_t         - amount of bytes written to the buffer

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
int32_t ProtobufWriteDestroy(ProtobufWriteRefT *pState)
{
    const int32_t iResult = pState->iBufOff;

    // encode the size if needed
    if (pState->bEncodeSize == TRUE)
    {
        // calculate the size of the message without accounting for encoded size
        const int32_t iMessageSize = SocketNtohl(iResult-4);
        ds_memcpy(pState->pBuffer, (const void *)&iMessageSize, sizeof(iMessageSize));
    }

    DirtyMemFree(pState, PROTOBUF_MEMID, pState->iMemGroup, pState->pMemGroupUserdata);
    return(iResult);
}
