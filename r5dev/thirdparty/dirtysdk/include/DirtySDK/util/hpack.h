/*H********************************************************************************/
/*!
    \File    hpack.h

    \Description
        This module implements a decode/encoder based on the HPACK spec
        (https://tools.ietf.org/html/rfc7541). Which is used for encoding/decoding
        the HEADERS frame in the HTTP/2 protocol.

    \Copyright
        Copyright (c) Electronic Arts 2016. ALL RIGHTS RESERVED.
*/
/********************************************************************************H*/

#ifndef _hpack_h
#define _hpack_h

/*!
\Moduledef Hpack Hpack
\Modulemember Util
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Type Definitions *************************************************************/

//! opaque module ref
typedef struct HpackRefT HpackRefT;

/*** Functions ********************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

// create the module
DIRTYCODE_API HpackRefT *HpackCreate(uint32_t uTableMax, uint8_t bDecoder);

// destroy the module
DIRTYCODE_API void HpackDestroy(HpackRefT *pRef);

// unpack the header
DIRTYCODE_API int32_t HpackDecode(HpackRefT *pRef, const uint8_t *pInput, int32_t iInpSize, char *pOutput, int32_t iOutSize);

// encode the header
DIRTYCODE_API int32_t HpackEncode(HpackRefT *pRef, const char *pInput, uint8_t *pOutput, int32_t iOutSize, uint8_t bHuffman);

// clear the dynamic table
DIRTYCODE_API void HpackClear(HpackRefT *pRef);

// resize the dynamic table
DIRTYCODE_API void HpackResize(HpackRefT *pRef, uint32_t uTableSize, uint8_t bSendUpdate);

#if defined(__cplusplus)
}
#endif

//@}

#endif // _hpack_h
