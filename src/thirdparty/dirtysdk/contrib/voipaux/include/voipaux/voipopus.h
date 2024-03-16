/*H*************************************************************************************/
/*!
    \File voipopus.c

    \Description
        PC Audio Encoder / Decoder using Opus

    \Copyright
        Copyright (c) Electronic Arts 2017. ALL RIGHTS RESERVED.

    \Version 07/03/2017 (eesponda)
*/
/*************************************************************************************H*/

#ifndef _voipopus_h
#define _voipopus_h

/*** Includes **************************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/voip/voipcodec.h"

/*** Variables *************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

// opus codec definition
DIRTYCODE_API extern const VoipCodecDefT VoipOpus_CodecDef;

#if defined(__cplusplus)
}
#endif

#endif // _voipopus_h
