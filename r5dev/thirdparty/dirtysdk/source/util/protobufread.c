/*H*************************************************************************************/
/*!
    \File protobufread.c

    \Description
        Implementation of decoder for the Google Protobuf wire format
        See: https://developers.google.com/protocol-buffers/docs/encoding

        This only supports protobuf version 3, if any lesser versions are used
        the result is undefined.

    \Copyright
        Copyright (c) Electronic Arts 2017-2018. ALL RIGHTS RESERVED.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************H*/

/*** Includes **************************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/util/protobufcommon.h"
#include "DirtySDK/util/protobufread.h"

/*** Macros ****************************************************************************/

// helper to read the header information and return the location of the first element
#define PROTOBUF_ReadPackedRepeated(pState, pCurrent, pRepeated) ((ProtobufReadMessage((pState), (pCurrent), (pRepeated)) != NULL) ? (pRepeated)->pBuffer : NULL)

/*** Private Functions *****************************************************************/

/*F*************************************************************************************/
/*!
    \Function _ProtobufReadVarint

    \Description
        Reads varint data from the buffer

    \Input *pBuffer - buffer we are reading from
    \Input iBufLen  - length of the buffer
    \Input *pValue  - [out] data pulled from the buffer

    \Output
        int32_t         - amount of bytes read from the buffer

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
static int32_t _ProtobufReadVarint(const uint8_t *pBuffer, int32_t iBufLen, uint64_t *pValue)
{
    int32_t iRead = 0;
    uint8_t uMSB = 0, uByte;

    // initialize the value to zero
    if (pValue != NULL)
    {
        *pValue = 0;
    }

    // sanity check
    if ((pBuffer == NULL) || (iBufLen <= 0))
    {
        return(0);
    }

    // read until you decode the full integer or reach the end of the buffer
    do
    {
        uByte = pBuffer[iRead++];
        if (pValue != NULL)
        {
            *pValue += (uByte & 0x7f) * ((uint64_t)1 << uMSB);
        }
        uMSB += 7;
    } while (((uByte & 0x80) == 0x80) && (iRead < iBufLen));

    return(iRead);
}

/*** Public Functions ******************************************************************/

/*F*************************************************************************************/
/*!
    \Function ProtobufReadInit

    \Description
        Initializes the reading structure based on buffer

    \Input *pState  - state we are initializing
    \Input *pBuffer - buffer we are using to read
    \Input iBufLen  - size of the buffer

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
void ProtobufReadInit(ProtobufReadT *pState, const uint8_t *pBuffer, int32_t iBufLen)
{
    ds_memclr(pState, sizeof(*pState));
    pState->pBuffer = pBuffer;
    pState->pBufEnd = pBuffer+iBufLen;
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadFind

    \Description
        Finds a field in the buffer

    \Input *pState      - reader state
    \Input uField       - field we are looking for

    \Output
        const uint8_t * - pointer to the location of the field or NULL if not found

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
const uint8_t *ProtobufReadFind(const ProtobufReadT *pState, uint32_t uField)
{
    const uint8_t *pBuffer = pState->pBuffer;

    for (;;)
    {
        uint32_t uField2;
        uint8_t uType;

        // read the tag information, if we didn't read anything then we are done
        if ((pBuffer = ProtobufReadHeader(pState, pBuffer, &uField2, &uType)) == NULL)
        {
            break;
        }

        // check to see if it is the field you are looking for
        if (uField == uField2)
        {
            return(pBuffer);
        }
        // try to read past
        else if ((pBuffer = ProtobufReadSkip(pState, pBuffer, uType)) == NULL)
        {
            break;
        }
    }
    return(NULL);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadFind2

    \Description
        Finds a field in the buffer allowing to continue from a point

    \Input *pState      - reader state
    \Input uField       - field we are looking for
    \Input *pCurrent    - where to continue from or NULL if to start from the beginning

    \Output
        const uint8_t * - pointer to the location of the field or NULL if not found / end
                          of buffer

    \Notes
        This function is useful when reading repeated length delimited types (not packed).
        Since the reading happens procedurally, we need to continue finding repeated elements
        past the previous one that was read.

        For this reason, when passing data into pCurrent make sure this is past the element
        that you have read otherwise parsing will fail.

        \code{.c}
        char strText[256];
        const uint8_t *pCurrent = NULL;

        while ((pCurrent = ProtobufReadString(&Read, ProtobufReadFind2(&Read, 1, pCurrent), strText, sizeof(strText))) != NULL)
        {
            ...
        }
        \endcode

    \Version 12/05/2017 (eesponda)
*/
/*************************************************************************************F*/
const uint8_t *ProtobufReadFind2(const ProtobufReadT *pState, uint32_t uField, const uint8_t *pCurrent)
{
    // assign the buffer to the beginning if we are not continuing
    if (pCurrent == NULL)
    {
        pCurrent = pState->pBuffer;
    }

    for (;;)
    {
        uint32_t uField2;
        uint8_t uType;

        // read the tag information, if we didn't read anything then we are done
        if ((pCurrent = ProtobufReadHeader(pState, pCurrent, &uField2, &uType)) == NULL)
        {
            break;
        }

        // check to see if it is the field you are looking for
        if (uField == uField2)
        {
            return(pCurrent);
        }

        // try read past
        if ((pCurrent = ProtobufReadSkip(pState, pCurrent, uType)) == NULL)
        {
            break;
        }
    }
    return(NULL);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadHeader

    \Description
        Reads the tag header information from the buffer

    \Input *pState      - reader state
    \Input *pCurrent    - location in the buffer or NULL
    \Input *pField      - [out] field identifier
    \Input *pType       - [out] type identifier (equates to ProtobufTypeE)

    \Output
        const uint8_t * - pointer to the location past the tag header or NULL if the end

    \Notes
        This function is for more advanced use where speed in critical to prevent
        scanning the entire buffer when using ProtobufReadFind. When in doubt do
        not use this and ProtobufReadSkip directly.

        \code{.c}
        uint32_t uField;
        uint8_t uType;
        const uint8_t *pCurrent = Read.pBuffer;

        while ((pCurrent = ProtobufReadHeader(&Read, pCurrent, &uField, &uType)) != NULL)
        {
            // check for correct field / type combo you expect, for instance in a simple case
            // note: normally you would check all your known type/field combos
            if ((uType == PROTOBUF_TYPE_VARINT) && (uField == 1))
            {
                uint64_t uResult;
                pCurrent = ProtobufReadVarint2(&Read, pCurrent, &uResult);
            }
            // otherwise skip
            else
            {
                pCurrent = ProtobufReadSkip(&Read, pCurrent, uType);
            }
        }
        \endcode

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
const uint8_t *ProtobufReadHeader(const ProtobufReadT *pState, const uint8_t *pCurrent, uint32_t *pField, uint8_t *pType)
{
    int32_t iRead;
    uint64_t uTag;

    // read the tag information, if we didn't read anything then we have reached the end
    if ((iRead = _ProtobufReadVarint(pCurrent, (int32_t)(pState->pBufEnd-pCurrent), &uTag)) <= 0)
    {
        return(NULL);
    }
    pCurrent += iRead;

    // parse the tag information and return current location
    *pField = (uint32_t)(uTag >> 3);
    *pType = uTag & 7;
    return(pCurrent);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadSkip

    \Description
        Read over an element in the buffer

    \Input *pState      - reader state
    \Input *pCurrent    - location in the buffer or NULL
    \Input uType        - type identifier (equates to ProtobufTypeE)

    \Output
        const uint8_t * - location past the element or NULL if unhandled type / end of buffer

    \Notes
        See notes in ProtobufReadHeader

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
const uint8_t *ProtobufReadSkip(const ProtobufReadT *pState, const uint8_t *pCurrent, uint8_t uType)
{
    switch (uType)
    {
        case PROTOBUF_TYPE_VARINT:
        {
            pCurrent += _ProtobufReadVarint(pCurrent, (int32_t)(pState->pBufEnd-pCurrent), NULL);
            break;
        }

        case PROTOBUF_TYPE_64BIT:
        {
            pCurrent += DS_MIN((uint32_t)(pState->pBufEnd-pCurrent), 8);
            break;
        }

        case PROTOBUF_TYPE_LENGTH_DELIMITED:
        {
            uint64_t uValue;
            pCurrent += _ProtobufReadVarint(pCurrent, (int32_t)(pState->pBufEnd-pCurrent), &uValue);
            pCurrent += DS_MIN((uint32_t)(pState->pBufEnd-pCurrent), uValue);
            break;
        }

        case PROTOBUF_TYPE_32BIT:
        {
            pCurrent += DS_MIN((uint32_t)(pState->pBufEnd-pCurrent), 4);
            break;
        }

        case PROTOBUF_TYPE_START_GROUP:
        case PROTOBUF_TYPE_END_GROUP:
        default:
            // not sure how to read these so return an error so we can stop trying to read
            NetPrintf(("protobufreader: found '%s' type that we cannot handle\n", ProtobufCommonGetType(uType)));
            return(NULL);
    }
    return(pCurrent);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadVarint

    \Description
        Reads a varint from the buffer

    \Input *pState      - reader state
    \Input *pCurrent    - location in the buffer or NULL

    \Output
        uint64_t        - value read from the buffer, zero if pCurrent is NULL

    \Notes
        This function is different than many of the other read functions where the output
        is a pointer output parameter, due to the nature of the varint we read it out in
        an uint64_t. Since this will cause some annoyances in using it for most cases we
        opted to use a special function in this case. This version of the function will
        not work when reading repeated fields so please use ProtobufReadVarint2 in that
        case.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
uint64_t ProtobufReadVarint(const ProtobufReadT *pState, const uint8_t *pCurrent)
{
    uint64_t uResult;
    _ProtobufReadVarint(pCurrent, (int32_t)(pState->pBufEnd-pCurrent), &uResult);
    return(uResult);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadVarint2

    \Description
        Reads a varint from the buffer

    \Input *pState      - reader state
    \Input *pCurrent    - location in the buffer or NULL
    \Input *pResult     - [out] result of reading

    \Output
        const uint8_t * - location past the element in the buffer or NULL if end is reached

    \Notes
        The result of this function is only important when reading repeated fields where
        we want to stop reading when there are no more elements in the field to read.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
const uint8_t *ProtobufReadVarint2(const ProtobufReadT *pState, const uint8_t *pCurrent, uint64_t *pResult)
{
    int32_t iRead = _ProtobufReadVarint(pCurrent, (int32_t)(pState->pBufEnd-pCurrent), pResult);
    return((iRead > 0) ? (pCurrent+iRead) : NULL);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadRepeatedVarint

    \Description
        Reads a repeated varint from the buffer into an array

    \Input *pState      - reader state
    \Input *pCurrent    - location in the buffer or NULL
    \Input *pResult     - [out] output array
    \Input iCount       - number of elements that can be written

    \Output
        const uint8_t * - location past the element in the buffer or NULL if end is reached

    \Notes
        If you need to figure out the amount of elements for the repeated field, you
        can call the ProtobufReadNumRepeatedElements function. Otherwise, you should
        pass in an array large enough to hold the elements you expect.

    \Version 01/02/2018 (eesponda)
*/
/*************************************************************************************F*/
const uint8_t *ProtobufReadRepeatedVarint(const ProtobufReadT *pState, const uint8_t *pCurrent, uint64_t *pResult, int32_t iCount)
{
    ProtobufReadT Field;
    int32_t iIndex = 0;
    ds_memclr(pResult, sizeof(*pResult) * iCount);

    // read the header for the packed repeated field
    if ((pCurrent = PROTOBUF_ReadPackedRepeated(pState, pCurrent, &Field)) == NULL)
    {
        return(pCurrent);
    }
    // write as many elements as possible from the buffer
    while ((pCurrent = ProtobufReadVarint2(&Field, pCurrent, (iIndex < iCount) ? &pResult[iIndex++] : NULL)) != NULL)
        ;
    return((Field.pBufEnd < pState->pBufEnd) ? (Field.pBufEnd+1) : NULL);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadSint32

    \Description
        Reads a sint32 from the buffer

    \Input *pState      - reader state
    \Input *pCurrent    - location in the buffer or NULL
    \Input *pResult     - [out] result of reading

    \Output
        const uint8_t * - location past the element in the buffer or NULL if end is reached

    \Notes
        The result of this function is only important when reading repeated fields where
        we want to stop reading when there are no more elements in the field to read.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
const uint8_t *ProtobufReadSint32(const ProtobufReadT *pState, const uint8_t *pCurrent, int32_t *pResult)
{
    uint64_t uResult;
    pCurrent = ProtobufReadVarint2(pState, pCurrent, &uResult);
    *pResult = (int32_t)(((uResult >> 1) ^ -(int32_t)(uResult & 1)));
    return(pCurrent);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadRepeatedSint32

    \Description
        Reads a repeated varint sint32 from the buffer into an array

    \Input *pState      - reader state
    \Input *pCurrent    - location in the buffer or NULL
    \Input *pResult     - [out] output array
    \Input iCount       - number of elements that can be written

    \Output
        const uint8_t * - location past the element in the buffer or NULL if end is reached

    \Notes
        If you need to figure out the amount of elements for the repeated field, you
        can call the ProtobufReadNumRepeatedElements function. Otherwise, you should
        pass in an array large enough to hold the elements you expect.

    \Version 01/02/2018 (eesponda)
*/
/*************************************************************************************F*/
const uint8_t *ProtobufReadRepeatedSint32(const ProtobufReadT *pState, const uint8_t *pCurrent, int32_t *pResult, int32_t iCount)
{
    ProtobufReadT Field;
    int32_t iIndex = 0;
    ds_memclr(pResult, sizeof(*pResult) * iCount);

    // read the header for the packed repeated field
    if ((pCurrent = PROTOBUF_ReadPackedRepeated(pState, pCurrent, &Field)) == NULL)
    {
        return(pCurrent);
    }
    // write as many elements as possible from the buffer
    while ((pCurrent = ProtobufReadSint32(&Field, pCurrent, (iIndex < iCount) ? &pResult[iIndex++] : NULL)) != NULL)
        ;
    return((Field.pBufEnd < pState->pBufEnd) ? (Field.pBufEnd+1) : NULL);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadSint64

    \Description
        Reads a sint64 from the buffer

    \Input *pState      - reader state
    \Input *pCurrent    - location in the buffer or NULL
    \Input *pResult     - [out] result of reading

    \Output
        const uint8_t * - location past the element in the buffer or NULL if end is reached

    \Notes
        The result of this function is only important when reading repeated fields where
        we want to stop reading when there are no more elements in the field to read.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
const uint8_t *ProtobufReadSint64(const ProtobufReadT *pState, const uint8_t *pCurrent, int64_t *pResult)
{
    uint64_t uResult;
    pCurrent = ProtobufReadVarint2(pState, pCurrent, &uResult);
    *pResult = ((uResult >> 1) ^ -(int64_t)(uResult & 1));
    return(pCurrent);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadRepeatedSint64

    \Description
        Reads a repeated varint sint64 from the buffer into an array

    \Input *pState      - reader state
    \Input *pCurrent    - location in the buffer or NULL
    \Input *pResult     - [out] output array
    \Input iCount       - number of elements that can be written

    \Output
        const uint8_t * - location past the element in the buffer or NULL if end is reached

    \Notes
        If you need to figure out the amount of elements for the repeated field, you
        can call the ProtobufReadNumRepeatedElements function. Otherwise, you should
        pass in an array large enough to hold the elements you expect.

    \Version 01/02/2018 (eesponda)
*/
/*************************************************************************************F*/
const uint8_t *ProtobufReadRepeatedSint64(const ProtobufReadT *pState, const uint8_t *pCurrent, int64_t *pResult, int32_t iCount)
{
    ProtobufReadT Field;
    int32_t iIndex = 0;
    ds_memclr(pResult, sizeof(*pResult) * iCount);

    // read the header for the packed repeated field
    if ((pCurrent = PROTOBUF_ReadPackedRepeated(pState, pCurrent, &Field)) == NULL)
    {
        return(pCurrent);
    }
    // write as many elements as possible from the buffer
    while ((pCurrent = ProtobufReadSint64(&Field, pCurrent, (iIndex < iCount) ? &pResult[iIndex++] : NULL)) != NULL)
        ;
    return((Field.pBufEnd < pState->pBufEnd) ? (Field.pBufEnd+1) : NULL);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadFixed32

    \Description
        Reads a fixed32, float or sfixed32 from the buffer

    \Input *pState      - reader state
    \Input *pCurrent    - location in the buffer or NULL
    \Input *pOutput     - [out] output of reading

    \Output
        const uint8_t * - location past the element in the buffer or NULL if end is reached

    \Notes
        The result of this function is only important when reading repeated fields where
        we want to stop reading when there are no more elements in the field to read.
        We use void * for reading to allow you to pass in different 32 bit types, if the
        wrong type is used a max of 4 bytes is always read.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
const uint8_t *ProtobufReadFixed32(const ProtobufReadT *pState, const uint8_t *pCurrent, void *pOutput)
{
    uint32_t uLength = (pCurrent != NULL) ? DS_MIN((uint32_t)(pState->pBufEnd-pCurrent), 4) : 0;
    if (pOutput != NULL)
    {
        ds_memcpy_s(pOutput, 4, pCurrent, uLength);
    }
    return((uLength > 0) ? (pCurrent+uLength) : NULL);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadRepeatedFixed32

    \Description
        Reads a repeated fixed32, float or sfixed32 from the buffer into an array

    \Input *pState      - reader state
    \Input *pCurrent    - location in the buffer or NULL
    \Input *pOutput     - [out] output array
    \Input iOutLen      - size of the output

    \Output
        const uint8_t * - location past the element in the buffer or NULL if end is reached

    \Notes
        Since fixed types can be read as a block, we can memcpy the entire repeated field
        straight into the array. To achieve this you need to make sure to pass in the size
        in bytes of the array and not the number of elements.

        If you need to figure out the amount of elements for the repeated field, you
        can call the ProtobufReadNumRepeatedElements function. Otherwise, you should
        pass in an array large enough to hold the elements you expect.

    \Version 01/02/2018 (eesponda)
*/
/*************************************************************************************F*/
const uint8_t *ProtobufReadRepeatedFixed32(const ProtobufReadT *pState, const uint8_t *pCurrent, void *pOutput, int32_t iOutLen)
{
    ProtobufReadT Field;

    // sanity check to try to weed out incorrect usage
    if ((iOutLen % 4) != 0)
    {
        NetPrintf(("protobufwrite: [%p] fixed64 repeated output length is not a multiple of 8, please check to make sure you are sending size instead of count\n", pState));
        return(NULL);
    }
    ds_memclr(pOutput, iOutLen);

    // read the header for the packed repeated field
    if ((pCurrent = PROTOBUF_ReadPackedRepeated(pState, pCurrent, &Field)) == NULL)
    {
        return(pCurrent);
    }

    // write as many elements as possible from the buffer
    ds_memcpy_s(pOutput, iOutLen, pCurrent, (uint32_t)(Field.pBufEnd-pCurrent));
    return((Field.pBufEnd < pState->pBufEnd) ? (Field.pBufEnd+1) : NULL);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadFixed64

    \Description
        Reads a fixed64, double or sfixed64 from the buffer

    \Input *pState      - reader state
    \Input *pCurrent    - location in the buffer or NULL
    \Input *pOutput     - [out] output of reading

    \Output
        const uint8_t * - location past the element in the buffer or NULL if end is reached

    \Notes
        The result of this function is only important when reading repeated fields where
        we want to stop reading when there are no more elements in the field to read.
        We use void * for reading to allow you to pass in different 64 bit types, if the
        wrong type is used a max of 8 bytes is always read.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
const uint8_t *ProtobufReadFixed64(const ProtobufReadT *pState, const uint8_t *pCurrent, void *pOutput)
{
    uint32_t uLength = (pCurrent != NULL) ? DS_MIN((uint32_t)(pState->pBufEnd-pCurrent), 8) : 0;
    if (pOutput != NULL)
    {
        ds_memcpy_s(pOutput, 8, pCurrent, uLength);
    }
    return((uLength > 0) ? (pCurrent+uLength) : NULL);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadRepeatedFixed64

    \Description
        Reads a repeated fixed64, double or sfixed64 from the buffer into an array

    \Input *pState      - reader state
    \Input *pCurrent    - location in the buffer or NULL
    \Input *pOutput     - [out] output array
    \Input iOutLen      - size of the output

    \Output
        const uint8_t * - location past the element in the buffer or NULL if end is reached

    \Notes
        Since fixed types can be read as a block, we can memcpy the entire repeated field
        straight into the array. To achieve this you need to make sure to pass in the size
        in bytes of the array and not the number of elements.

        If you need to figure out the amount of elements for the repeated field, you
        can call the ProtobufReadNumRepeatedElements function. Otherwise, you should
        pass in an array large enough to hold the elements you expect.

    \Version 01/02/2018 (eesponda)
*/
/*************************************************************************************F*/
const uint8_t *ProtobufReadRepeatedFixed64(const ProtobufReadT *pState, const uint8_t *pCurrent, void *pOutput, int32_t iOutLen)
{
    ProtobufReadT Field;

    // sanity check to try to weed out incorrect usage
    if ((iOutLen % 8) != 0)
    {
        NetPrintf(("protobufwrite: [%p] fixed64 repeated output length is not a multiple of 8, please check to make sure you are sending size instead of count\n", pState));
        return(NULL);
    }
    ds_memclr(pOutput, iOutLen);

    // read the header for the packed repeated field
    if ((pCurrent = PROTOBUF_ReadPackedRepeated(pState, pCurrent, &Field)) == NULL)
    {
        return(pCurrent);
    }

    // write as many elements as possible from the buffer
    ds_memcpy_s(pOutput, iOutLen, pCurrent, (uint32_t)(Field.pBufEnd-pCurrent));
    return((Field.pBufEnd < pState->pBufEnd) ? (Field.pBufEnd+1) : NULL);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadBytes

    \Description
        Reads the bytes type from the buffer

    \Input *pState      - reader state
    \Input *pCurrent    - location in the buffer or NULL
    \Input *pOutput     - [out] output of reading
    \Input iOutLen      - size of the output

    \Output
        const uint8_t * - location past the element in the buffer or NULL if end is reached

    \Notes
        The result of this function is only important when reading repeated fields where
        we want to stop reading when there are no more elements in the field to read.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
const uint8_t *ProtobufReadBytes(const ProtobufReadT *pState, const uint8_t *pCurrent, uint8_t *pOutput, int32_t iOutLen)
{
    uint64_t uLength;
    int32_t iRead;

    // parse the length and update to not pass the end
    if ((iRead = _ProtobufReadVarint(pCurrent, (int32_t)(pState->pBufEnd-pCurrent), &uLength)) <= 0)
    {
        if (pOutput != NULL)
        {
            ds_memclr(pOutput, iOutLen);
        }
        return(NULL);
    }
    pCurrent += iRead;
    uLength = DS_MIN((uint32_t)(pState->pBufEnd-pCurrent), uLength);

    ds_memcpy_s(pOutput, iOutLen, pCurrent, (int32_t)(uLength));
    return(pCurrent+uLength);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadString

    \Description
        Reads a string from the buffer

    \Input *pState      - reader state
    \Input *pCurrent    - location in the buffer or NULL
    \Input *pOutput     - [out] output of reading
    \Input iOutLen      - size of the output

    \Output
        const uint8_t * - location past the element in the buffer or NULL if end is reached

    \Notes
        The result of this function is only important when reading repeated fields where
        we want to stop reading when there are no more elements in the field to read.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
const uint8_t *ProtobufReadString(const ProtobufReadT *pState, const uint8_t *pCurrent, char *pOutput, int32_t iOutLen)
{
    uint64_t uLength;
    int32_t iRead;

    // parse the length and update to not pass the end
    if ((iRead = _ProtobufReadVarint(pCurrent, (int32_t)(pState->pBufEnd-pCurrent), &uLength)) <= 0)
    {
        if (pOutput != NULL)
        {
            *pOutput = '\0';
        }
        return(NULL);
    }
    pCurrent += iRead;
    uLength = DS_MIN((uint32_t)(pState->pBufEnd-pCurrent), uLength);

    ds_strsubzcpy(pOutput, iOutLen, (const char *)pCurrent, (int32_t)(uLength));
    return(pCurrent+uLength);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadMessage

    \Description
        Reads an embedded message from the buffer

    \Input *pState      - reader state
    \Input *pCurrent    - location in the buffer or NULL
    \Input *pMsg        - [out] new reader state used for reading embedded message

    \Output
        const uint8_t * - location past the element in the buffer or NULL if end is reached

    \Notes
        The result of this function is only important when reading repeated fields where
        we want to stop reading when there are no more elements in the field to read.

    \Version 07/05/2017 (eesponda)
*/
/*************************************************************************************F*/
const uint8_t *ProtobufReadMessage(const ProtobufReadT *pState, const uint8_t *pCurrent, ProtobufReadT *pMsg)
{
    uint64_t uLength;
    int32_t iRead;

    if ((iRead = _ProtobufReadVarint(pCurrent, (int32_t)(pState->pBufEnd-pCurrent), &uLength)) <= 0)
    {
        ProtobufReadInit(pMsg, NULL, 0);
        return(NULL);
    }
    pCurrent += iRead;

    // update length to not read past the end
    uLength = DS_MIN((uint32_t)(pState->pBufEnd-pCurrent), uLength);

    ProtobufReadInit(pMsg, pCurrent, (int32_t)(uLength));
    return(pCurrent+uLength);
}

/*F*************************************************************************************/
/*!
    \Function ProtobufReadNumRepeatedElements

    \Description
        Gets the number of elements in a repeated field

    \Input *pState      - reader state
    \Input uField       - field identifier of the repeated field
    \Input uType        - types encoded in the repeated field (they are not sent over the wire)

    \Output
        int32_t         - number of elements in the repeated field

    \Notes
        This function is available as an utility. If you don't need to get the number
        of elements don't call it as it saves from relooping over the repeated field.

    \Version 08/14/2017 (eesponda)
*/
/*************************************************************************************F*/
int32_t ProtobufReadNumRepeatedElements(const ProtobufReadT *pState, uint32_t uField, uint8_t uType)
{
    int32_t iNumElements = 0;

    if ((uType == PROTOBUF_TYPE_VARINT) || (uType == PROTOBUF_TYPE_64BIT) || (uType == PROTOBUF_TYPE_32BIT))
    {
        ProtobufReadT Repeated;
        const uint8_t *pCurrent = PROTOBUF_ReadPackedRepeated(pState, ProtobufReadFind(pState, uField), &Repeated);

        if (uType == PROTOBUF_TYPE_VARINT)
        {
            while ((pCurrent = ProtobufReadVarint2(&Repeated, pCurrent, NULL)) != NULL)
            {
                iNumElements += 1;
            }
        }
        else if (uType == PROTOBUF_TYPE_64BIT)
        {
            iNumElements = (int32_t)(Repeated.pBufEnd-Repeated.pBuffer) / 8;
        }
        else if (uType == PROTOBUF_TYPE_32BIT)
        {
            iNumElements = (int32_t)(Repeated.pBufEnd-Repeated.pBuffer) / 4;
        }
    }
    else if (uType == PROTOBUF_TYPE_LENGTH_DELIMITED)
    {
        const uint8_t *pCurrent = NULL;
        while ((pCurrent = ProtobufReadFind2(pState, uField, pCurrent)) != NULL)
        {
            pCurrent = ProtobufReadSkip(pState, pCurrent, uType);
            iNumElements += 1;
        }
    }

    return(iNumElements);
}
