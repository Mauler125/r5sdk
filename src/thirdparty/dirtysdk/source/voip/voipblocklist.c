
/*H********************************************************************************/
/*!
    \File voipblocklist.c

    \Description
        Impliments blocking voip communications based off users accountid

    \Copyright
        Copyright (c) 2019 Electronic Arts Inc.

    \Version 07/02/2019 (cvienneau) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h> // for qsort

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"

#include "DirtySDK/voip/voip.h"
#include "DirtySDK/voip/voipblocklist.h"


/*** Defines **********************************************************************/
#define VOIPBLOCKLIST_DEFAULT_LIST_SIZE     (64)
#define VOIPBLOCKLIST_MAX_LIST_SIZE         (2000)

/*** Type Definitions *************************************************************/
typedef struct VoipBlockListUserT
{
    int64_t *pBlockList;
    uint32_t uCapacity;
    uint32_t uConsumed;
    uint8_t bSorted;
} VoipBlockListUserT;

typedef struct VoipBlockListT
{
    VoipBlockListUserT aUserBlockLists[VOIP_MAXLOCALUSERS];
    int32_t iMemGroup;
    void *pMemGroupUserData;
} VoipBlockListT;

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _VoipBlockListSort

    \Description
        qsort callback used to sort fast lookup array.

    \Input *_pElem0     - pointer to first element to compare
    \Input *_pElem1     - pointer to second element to compare

    \Output
        int32_t         - sort value

    \Version 06/19/2019 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _VoipBlockListSort(const void *_pElem0, const void *_pElem1)
{
    int64_t arg0 = *(const int64_t*)_pElem0;
    int64_t arg1 = *(const int64_t*)_pElem1;

    if (arg0 < arg1) return -1;
    if (arg0 > arg1) return 1;
    return 0;
}

/*F********************************************************************************/
/*!
    \Function _VoipBlockListFindAccountId

    \Description
        Sorts the given users Blocked list, then finds the index of the BlockedAccountId

    \Input pVoip                - voip state
    \Input iLocalUserIndex      - the index of the local user
    \Input iBlockedAccountId    - the account id of the user to be found

    \Output
        int32_t - the index of the blocked account id, or -1 if not found

    \Version 06/20/2019 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _VoipBlockListFindAccountId(VoipRefT *pVoip, int32_t iLocalUserIndex, int64_t iBlockedAccountId)
{
    int32_t iCheck, iLow, iHigh;
    int64_t iCheckId;
    VoipBlockListT *pBlockedList = ((VoipCommonRefT *)pVoip)->pBlockList;

    if (pBlockedList->aUserBlockLists[iLocalUserIndex].bSorted == FALSE)
    {
        qsort(pBlockedList->aUserBlockLists[iLocalUserIndex].pBlockList, pBlockedList->aUserBlockLists[iLocalUserIndex].uConsumed, sizeof(pBlockedList->aUserBlockLists[iLocalUserIndex].pBlockList[0]), _VoipBlockListSort);
        pBlockedList->aUserBlockLists[iLocalUserIndex].bSorted = TRUE;
    }

    // execute binary search on sorted lookup table
    for (iLow = 0, iHigh = pBlockedList->aUserBlockLists[iLocalUserIndex].uConsumed - 1; iLow <= iHigh; )
    {
        iCheck = iLow + ((iHigh - iLow) / 2);
        if ((iCheckId = pBlockedList->aUserBlockLists[iLocalUserIndex].pBlockList[iCheck]) > iBlockedAccountId)
        {
            iHigh = iCheck - 1;
        }
        else if (iCheckId < iBlockedAccountId)
        {
            iLow = iCheck + 1;
        }
        else
        {
            return(iCheck);
        }
    }
    // not found
    return(-1);
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function   VoipBlockListCreate

    \Description
        Creates the VoipBlockListT.

    \Output
        VoipBlockListT * - The Blocked List state

    \Version 05/02/2019 (cvienneau)
*/
/********************************************************************************F*/
VoipBlockListT *VoipBlockListCreate(void)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;
    VoipBlockListT *pBlockedList = NULL;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    if ((pBlockedList = DirtyMemAlloc(sizeof(*pBlockedList), VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipblocklist: could not allocate block list state.\n"));
        return(NULL);
    }
    ds_memclr(pBlockedList, sizeof(*pBlockedList));
    pBlockedList->iMemGroup = iMemGroup;
    pBlockedList->pMemGroupUserData = pMemGroupUserData;

    return(pBlockedList);
}

/*F********************************************************************************/
/*!
    \Function   VoipBlockListDestroy

    \Description
        Free the voip block list

    \Input pVoip                - voip state

    \Version 05/07/2019 (cvienneau)
*/
/********************************************************************************F*/
void VoipBlockListDestroy(VoipRefT *pVoip)
{
    VoipBlockListT *pBlockedList = ((VoipCommonRefT *)pVoip)->pBlockList;

    // to free memory used by each user
    VoipBlockListClear(pVoip, -1);

    DirtyMemFree(pBlockedList, VOIP_MEMID, pBlockedList->iMemGroup, pBlockedList->pMemGroupUserData);
    ((VoipCommonRefT *)pVoip)->pBlockList = NULL;
}

/*F********************************************************************************/
/*!
    \Function   VoipBlockListAdd

    \Description
        Add a user to be blocked by the local user

    \Input pVoip                - voip state
    \Input iLocalUserIndex      - the index of the local user
    \Input iBlockedAccountId    - the account id of the user to be blocked

    \Output
        uint8_t         - TRUE if the user was successfully blocked

    \Version 05/07/2019 (cvienneau)
*/
/********************************************************************************F*/
uint8_t VoipBlockListAdd(VoipRefT *pVoip, int32_t iLocalUserIndex, int64_t iBlockedAccountId)
{
    VoipBlockListT *pBlockedList = ((VoipCommonRefT *)pVoip)->pBlockList;

    // early out if parameters are bad
    if (iLocalUserIndex < 0 || iLocalUserIndex >= VOIP_MAXLOCALUSERS)
    {
        NetPrintf(("voipblocklist: error, user index %d passed to VoipBlockListAdd is not valid.\n", iLocalUserIndex));
        return(FALSE);
    }

    // create/resize the blocked list as needed
    if (pBlockedList->aUserBlockLists[iLocalUserIndex].uCapacity < (pBlockedList->aUserBlockLists[iLocalUserIndex].uConsumed + 1))
    {
        int64_t *pOldList = pBlockedList->aUserBlockLists[iLocalUserIndex].pBlockList;
        uint32_t uNewCapacity = VOIPBLOCKLIST_DEFAULT_LIST_SIZE;

        // figgure out the size of the new blocked list
        if (pBlockedList->aUserBlockLists[iLocalUserIndex].uCapacity != 0)
        {
            if (pBlockedList->aUserBlockLists[iLocalUserIndex].uCapacity == VOIPBLOCKLIST_MAX_LIST_SIZE)
            {
                NetPrintf(("voipblocklist: error, user index %d's has no room in blocked list to add %lld.\n", iBlockedAccountId));
                return(FALSE);
            }
            uNewCapacity = 2 * pBlockedList->aUserBlockLists[iLocalUserIndex].uCapacity;

            if (uNewCapacity > VOIPBLOCKLIST_MAX_LIST_SIZE)
            {
                uNewCapacity = VOIPBLOCKLIST_MAX_LIST_SIZE;
            }
        }

        // allocate the new list
        if ((pBlockedList->aUserBlockLists[iLocalUserIndex].pBlockList = DirtyMemAlloc(sizeof(int64_t) * uNewCapacity, VOIP_MEMID, pBlockedList->iMemGroup, pBlockedList->pMemGroupUserData)) == NULL)
        {
            NetPrintf(("voipblocklist: could not allocate block list for user %d.\n", iLocalUserIndex));
            return(FALSE);
        }
        ds_memclr(pBlockedList->aUserBlockLists[iLocalUserIndex].pBlockList, sizeof(int64_t) * uNewCapacity);
        pBlockedList->aUserBlockLists[iLocalUserIndex].uCapacity = uNewCapacity;

        // copy any contents of the old list to the new list and free it
        if (pOldList != NULL)
        {
            ds_memcpy(pBlockedList->aUserBlockLists[iLocalUserIndex].pBlockList, pOldList, sizeof(int64_t) * pBlockedList->aUserBlockLists[iLocalUserIndex].uConsumed);
            DirtyMemFree(pOldList, VOIP_MEMID, pBlockedList->iMemGroup, pBlockedList->pMemGroupUserData);
        }
    }

    // fill the last slot of the blocked list with the new value
    pBlockedList->aUserBlockLists[iLocalUserIndex].pBlockList[pBlockedList->aUserBlockLists[iLocalUserIndex].uConsumed] = iBlockedAccountId;
    pBlockedList->aUserBlockLists[iLocalUserIndex].uConsumed += 1;
    pBlockedList->aUserBlockLists[iLocalUserIndex].bSorted = FALSE;
    if (VoipGetRef() != NULL)
    {
        ((VoipCommonRefT *)pVoip)->bApplyChannelConfig = TRUE;
    }
    NetPrintf(("voipblocklist: user index %d blocked %lld.\n", iLocalUserIndex, iBlockedAccountId));
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function   VoipBlockListRemove

    \Description
        Remove a user that was blocked by the local user

    \Input pVoip                - voip state
    \Input iLocalUserIndex      - the index of the local user
    \Input iBlockedAccountId    - the account id of the user who was blocked

    \Output
        uint8_t         - TRUE if the user was successfully un-blocked

    \Version 05/07/2019 (cvienneau)
*/
/********************************************************************************F*/
uint8_t VoipBlockListRemove(VoipRefT *pVoip, int32_t iLocalUserIndex, int64_t iBlockedAccountId)
{
    uint32_t uBlockedIndex;
    VoipBlockListT *pBlockedList = ((VoipCommonRefT *)pVoip)->pBlockList;

    // early out if parameters are bad
    if (iLocalUserIndex < 0 || iLocalUserIndex >= VOIP_MAXLOCALUSERS)
    {
        NetPrintf(("voipblocklist: error, user index %d passed to VoipBlockListRemove is not valid.\n", iLocalUserIndex));
        return(FALSE);
    }

    // look for the blocked user
    // note we don't use _VoipBlockListFindAccountId here since if the calling pattern is to call Remove several times in a row 
    // we don't want to sort it over and over and removing no longer ensuers its sorted
    for (uBlockedIndex = 0; uBlockedIndex < pBlockedList->aUserBlockLists[iLocalUserIndex].uConsumed; uBlockedIndex++)
    {
        if (pBlockedList->aUserBlockLists[iLocalUserIndex].pBlockList[uBlockedIndex] == iBlockedAccountId)
        {
            // copy the last entry over the to be removed entry
            pBlockedList->aUserBlockLists[iLocalUserIndex].pBlockList[uBlockedIndex] = pBlockedList->aUserBlockLists[iLocalUserIndex].pBlockList[pBlockedList->aUserBlockLists[iLocalUserIndex].uConsumed - 1];

            //remove the last entry
            pBlockedList->aUserBlockLists[iLocalUserIndex].pBlockList[pBlockedList->aUserBlockLists[iLocalUserIndex].uConsumed - 1] = 0;
            pBlockedList->aUserBlockLists[iLocalUserIndex].uConsumed -= 1;
            pBlockedList->aUserBlockLists[iLocalUserIndex].bSorted = FALSE;

            if (VoipGetRef() != NULL)
            {
                ((VoipCommonRefT *)pVoip)->bApplyChannelConfig = TRUE;
            }
            NetPrintf(("voipblocklist: BlockList, user index %d un-blocked %lld.\n", iLocalUserIndex, iBlockedAccountId));
            return(TRUE);
        }
    }
    NetPrintf(("voipblocklist: BlockList warning, user index %d could not find %lld in blocked list to remove it.\n", iLocalUserIndex, iBlockedAccountId));
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function   VoipBlockListIsBlocked

    \Description
        Check if a user is blocked by the local user

    \Input pVoip                - voip state
    \Input iLocalUserIndex      - the index of the local user
    \Input iBlockedAccountId    - the account id of the user who was blocked

    \Output
        uint8_t         - TRUE if the user is blocked

    \Version 05/07/2019 (cvienneau)
*/
/********************************************************************************F*/
uint8_t VoipBlockListIsBlocked(VoipRefT *pVoip, int32_t iLocalUserIndex, int64_t iBlockedAccountId)
{
    // if its the shared local user index then we really need to test all the local users
    if (iLocalUserIndex == VOIP_SHARED_USER_INDEX)
    {
        for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS; iLocalUserIndex++)
        {
            if (VoipBlockListIsBlocked(pVoip, iLocalUserIndex, iBlockedAccountId) == TRUE)
            {
                return(TRUE);
            }
        }
    }
    // deal with individual users
    else
    {
        // early out if parameters are bad
        if (iLocalUserIndex < 0 || iLocalUserIndex >= VOIP_MAXLOCALUSERS)
        {
            NetPrintf(("voipblocklist: error, user index %d passed to VoipBlockListIsBlocked is not valid.\n", iLocalUserIndex));
            return(FALSE);
        }

        // look for the blocked user
        if (_VoipBlockListFindAccountId(pVoip, iLocalUserIndex, iBlockedAccountId) >= 0)
        {
            return(TRUE);
        }
    }
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function   VoipBlockListClear

    \Description
        Clear the blocked list for the local user

    \Input pVoip                - voip state
    \Input iLocalUserIndex      - the index of the local user (-1 for all users)

    \Output
        uint8_t         - TRUE if the list was cleared

    \Version 05/07/2019 (cvienneau)
*/
/********************************************************************************F*/
uint8_t VoipBlockListClear(VoipRefT *pVoip, int32_t iLocalUserIndex)
{
    VoipBlockListT *pBlockedList = ((VoipCommonRefT *)pVoip)->pBlockList;

    if (iLocalUserIndex == -1)
    {
        int32_t iUserLoopIndex;
        for (iUserLoopIndex = 0; iUserLoopIndex < VOIP_MAXLOCALUSERS; iUserLoopIndex++)
        {
            VoipBlockListClear(pVoip, iUserLoopIndex);
        }
    }
    else
    {
        // early out if parameters are bad
        if ((iLocalUserIndex < 0) || (iLocalUserIndex >= VOIP_MAXLOCALUSERS))
        {
            NetPrintf(("voipblocklist: error, user index %d passed to VoipBlockListClear is not valid.\n", iLocalUserIndex));
            return(FALSE);
        }

        // deallocate
        DirtyMemFree(pBlockedList->aUserBlockLists[iLocalUserIndex].pBlockList, VOIP_MEMID, pBlockedList->iMemGroup, pBlockedList->pMemGroupUserData);
        ds_memclr(&pBlockedList->aUserBlockLists[iLocalUserIndex], sizeof(pBlockedList->aUserBlockLists[iLocalUserIndex]));
        NetPrintf(("voipblocklist: BlockList, cleared blocked list for user index %d.\n", iLocalUserIndex));
    }

    if (VoipGetRef() != NULL)
    {
        ((VoipCommonRefT *)pVoip)->bApplyChannelConfig = TRUE;
    }
    return(TRUE);
}
