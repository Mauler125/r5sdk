/*H********************************************************************************/
/*!
    \File voippcm.h

    \Description
        Table based 16:3 ADPCM compression originally based off EAC SIMEX code,
        modified by Greg Schaefer.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 08/04/2004 (jbrookes) First version
*/
/********************************************************************************H*/

#ifndef _voippcm_h
#define _voippcm_h

/*** Include files ****************************************************************/

#include "DirtySDK/voip/voipcodec.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

extern const VoipCodecDefT   VoipPCM_CodecDef;

#ifdef __cplusplus
};
#endif

/*** Functions ********************************************************************/

#endif // _voippcm_h
