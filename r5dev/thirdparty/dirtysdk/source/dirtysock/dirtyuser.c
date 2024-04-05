/*H********************************************************************************/
/*!

    \File    dirtyuser.c

    \Description
        Generic user functions. Given that all our supported platforms use
        uint64_t, we can combine all our functions.

    \Copyright
        Copyright (c) Electronic Arts 2020.    ALL RIGHTS RESERVED.

    \Version 03/05/20 (eesponda)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyuser.h"
#include "DirtySDK/util/binary7.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function DirtyUserToNativeUser

    \Description
        Convert a DirtyUserT to native format.

    \Input *pOutput     - [out] storage for native format user
    \Input iBufLen      - length of output buffer
    \Input *pUser       - source user to convert

    \Output
        uint32_t        - TRUE if successful, else FALSE

    \Version 03/05/2020 (eesponda)
*/
/********************************************************************************F*/
uint32_t DirtyUserToNativeUser(void *pOutput, int32_t iBufLen, const DirtyUserT *pUser)
{
    // make sure output buffer is big enough
    if (iBufLen < (signed)sizeof(uint64_t))
    {
        return(FALSE);
    }
    // make sure the encoding is correct
    if (*pUser->strNativeUser != '^')
    {
        return(FALSE);
    }

    // decode contents of pUser
    Binary7Decode((uint8_t *)pOutput, iBufLen, (const uint8_t *)pUser->strNativeUser+1);

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function DirtyUserFromNativeUser

    \Description
        Convert native format user data to a DirtyUserT.

    \Input *pUser       - [out] storage for output DirtyUserT
    \Input *pInput      - pointer to native format user

    \Output
        uint32_t        - TRUE if successful, else FALSE

    \Version 03/05/2020 (eesponda)
*/
/********************************************************************************F*/
uint32_t DirtyUserFromNativeUser(DirtyUserT *pUser, const void *pInput)
{
    uint64_t *pPlayerId = (uint64_t *)pInput;
    // clear output data
    ds_memclr(pUser, sizeof(*pUser));

    // encode input SceNpAccountId into pUser
    pUser->strNativeUser[0] = '^';
    Binary7Encode((uint8_t *)pUser->strNativeUser+1, sizeof(*pUser)-1, (uint8_t *)pPlayerId, sizeof(*pPlayerId), TRUE);

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function DirtyUserCompare

    \Description
        Compare two DirtyUserT users for equality.

    \Input *pUser1 - user 1
    \Input *pUser2 - user 2

    \Output
        int32_t    - TRUE if successful, else FALSE

    \Version 03/05/2020 (eesponda)
*/
/********************************************************************************F*/
int32_t DirtyUserCompare(DirtyUserT *pUser1, DirtyUserT *pUser2)
{
    return(strcmp(pUser1->strNativeUser, pUser2->strNativeUser) == 0);
}
