
/*H*************************************************************************************************/
/*!
    \File voipblocklist.h

    \Description
        Allow blocking of voip communication based on account id.

    \Copyright
        Copyright (c) 2019 Electronic Arts Inc.

    \Version 07/03/2019 (cvienneau) First Version
*/
/*************************************************************************************************H*/

#ifndef _voipblocklist_h
#define _voipblocklist_h

/*** Include files ********************************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/voip/voipdef.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/
typedef struct VoipBlockListT VoipBlockListT;

/*** Macros ***************************************************************************************/

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// creates the VoipBlockListT (internal use only by voipcommon)
DIRTYCODE_API VoipBlockListT *VoipBlockListCreate(void);

// destroys the VoipBlockListT (internal use only by voipcommon)
DIRTYCODE_API void VoipBlockListDestroy(VoipRefT *pVoip);

// add a user to be blocked by the local user
DIRTYCODE_API uint8_t VoipBlockListAdd(VoipRefT *pVoip, int32_t iLocalUserIndex, int64_t iMutedAccountId);

// remove a user that was blocked by the local user
DIRTYCODE_API uint8_t VoipBlockListRemove(VoipRefT *pVoip, int32_t iLocalUserIndex, int64_t iMutedAccountId);

// check if a user is blocked by the local user
DIRTYCODE_API uint8_t VoipBlockListIsBlocked(VoipRefT *pVoip, int32_t iLocalUserIndex, int64_t iMutedAccountId);

// clear the blocked list for the local user (-1 for all users)
DIRTYCODE_API uint8_t VoipBlockListClear(VoipRefT *pVoip, int32_t iLocalUserIndex);

#ifdef __cplusplus
}
#endif

#endif // _voipblocklist_h

