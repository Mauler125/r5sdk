/*H*************************************************************************************/
/*!
    \File    binary7.h

    \Description
        This module provides routines to encode/decode binary7 data to/from a buffer.

    \Copyright
        Copyright (c) Electronic Arts 2009. ALL RIGHTS RESERVED.

    \Version 1.0 11/02/2009 (cadam) First version
*/
/*************************************************************************************H*/

#ifndef _binary7_h
#define _binary7_h

/*!
\Moduledef Binary7 Binary7
\Modulemember Util
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// set a binary field using more efficient encoding
DIRTYCODE_API char *Binary7Encode(unsigned char *pDst, int32_t iDstLen, unsigned const char *pSrc, int32_t iSrcLen, uint32_t bTerminate);

// get binary field contents
DIRTYCODE_API unsigned const char *Binary7Decode(unsigned char *pDst, int32_t iDstLen, unsigned const char *pSrc);

#ifdef __cplusplus
}
#endif

//@}

#endif // _binary7_h

