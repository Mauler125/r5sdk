/*H********************************************************************************/
/*!
    \File dirtyuser.h

    \Description
        Definition for portable user type.

    \Copyright
        Copyright (c) Electronic Arts 2013

    \Version 04/25/13 (mclouatre) First Version
*/
/********************************************************************************H*/

#ifndef _dirtyuser_h
#define _dirtyuser_h

/*!
\Moduledef DirtyUser DirtyUser
\Modulemember DirtySock
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

#define DIRTYUSER_NATIVEUSER_MAXLEN (64)
#define DIRTYUSER_NATIVEUSER_MAXSIZE (DIRTYUSER_NATIVEUSER_MAXLEN + 1)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! opaque user type
typedef struct DirtyUserT
{
    char strNativeUser[DIRTYUSER_NATIVEUSER_MAXSIZE];
} DirtyUserT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

//! convert a DirtyUserT to native format
DIRTYCODE_API uint32_t DirtyUserToNativeUser(void *pOutput, int32_t iBufLen, const DirtyUserT *pUser);

//! convert a native format to a DirtyUserT
DIRTYCODE_API uint32_t DirtyUserFromNativeUser(DirtyUserT *pUser, const void *pInput);

//! compare two opaque users for equality  (same=TRUE, different=FALSE)
DIRTYCODE_API int32_t DirtyUserCompare(DirtyUserT *pUser1, DirtyUserT *pUser2);

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtyuser_h
