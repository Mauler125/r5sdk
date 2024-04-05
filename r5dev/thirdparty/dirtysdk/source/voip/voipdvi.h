/*H********************************************************************************/
/*!
    \File voipdvi.h

    \Description
        Table based 16:3 ADPCM compression originally based off EAC SIMEX code,
        modified by Greg Schaefer.

    \Copyright
        Copyright (c) Electronic Arts 2003-2004. ALL RIGHTS RESERVED.

    \Version 1.0 11/01/2002 (ischmidt)  First version (based on SIMEX by Dave Mercier)
    \Version 2.0 05/13/2003 (gschaefer) Rewrite to 16:3 (from 16:4)
*/
/********************************************************************************H*/

#ifndef _voipdvi_h
#define _voipdvi_h

/*** Include files ****************************************************************/

#include "DirtySDK/voip/voipcodec.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

extern const VoipCodecDefT   VoipDVI_CodecDef;

#if defined(__cplusplus)
};
#endif

/*** Functions ********************************************************************/

#endif // _voipdvi_h
