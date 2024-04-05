/*H*************************************************************************************************/
/*!
    \File    userapi.c

    \Description
        Expose first party players' information

    \Copyright
        Copyright (c) Electronic Arts 2001-2013

    \Version 05/10/2013 (mcorcoran) First Version
*/
/*************************************************************************************************H*/


/*** Include files ********************************************************************************/

#include <string.h>

#include "DirtySDK/misc/userapi.h"
#include "userapipriv.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtymem.h"

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Function Prototypes **************************************************************************/

static void _ClearContext(UserApiRefT *pRef, int32_t iUserIndex);

/*** Variables ************************************************************************************/

/*** Private Functions ****************************************************************************/

/*F*************************************************************************************************/
/*!
    \Function _UserApiFreeCallback

    \Description
        Callback registered with the netconn external cleanup mechanism. It proceeds with destroying
        the UserApi instance only when there is no longer an internal async operation in progress
        and the memory can safely be freed. 

    \Input *pMem     - pointer to the UserApi memory buffer

    \Output
        int32_t       - zero=success; -1=try again; other negative=error

    \Version 09/25/2013 (amakoukji)
*/
/*************************************************************************************************F*/
static int32_t _UserApiFreeCallback(void *pMem)
{
    UserApiRefT *pRef = (UserApiRefT*)pMem;
                    
    if ((pRef->pPlatformData != NULL) && (UserApiPlatDestroyData(pRef, pRef->pPlatformData) < 0))
    {
        return(-1);
    }

    NetCritKill(&pRef->crit);
    NetCritKill(&pRef->postCrit);
    DirtyMemFree(pRef, USERAPI_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    _ClearContext

    \Description
        Clears a user context

    \Input *pRef      - pointer to module ref if successful, NULL otherwise.
    \Input iUserIndex - user index

    \Version 12/11/2013 (amakoukji) - First version
*/
/*************************************************************************************************F*/
void _ClearContext(UserApiRefT *pRef, int32_t iUserIndex)
{
    NetCritEnter(&pRef->crit);
    pRef->UserContextList[iUserIndex].iTotalRequested = 0;
    pRef->UserContextList[iUserIndex].iTotalReceived = 0;
    pRef->UserContextList[iUserIndex].iTotalErrors = 0;

    pRef->UserPresenceList[iUserIndex].iTotalRequested = 0;
    pRef->UserPresenceList[iUserIndex].iTotalReceived = 0;
    pRef->UserPresenceList[iUserIndex].iTotalErrors = 0;

    pRef->UserRichPresenceList[iUserIndex].iTotalRequested = 0;
    pRef->UserRichPresenceList[iUserIndex].iTotalReceived = 0;
    pRef->UserRichPresenceList[iUserIndex].iTotalErrors = 0;

    pRef->iLookupUsersLength[iUserIndex] = -1;
    pRef->iLookupUserIndex[iUserIndex] = -1;
    pRef->iLookupsSent[iUserIndex] = 0;
    pRef->bLookupUserAvailable[iUserIndex] = TRUE;

    pRef->bAvailableDataIndex[iUserIndex] = FALSE;
    pRef->bAvailableDataIndexPresence[iUserIndex] = FALSE;
    pRef->bAvailableDataIndexRichPresence[iUserIndex] = FALSE;

    if (pRef->aLookupUsers[iUserIndex])
    {
        DirtyMemFree(pRef->aLookupUsers[iUserIndex], USERAPI_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData); 
        pRef->aLookupUsers[iUserIndex] = NULL;
    }
    NetCritLeave(&pRef->crit);
}

/*F*************************************************************************************************/
/*!
    \Function    _UserApiTriggerCallback

    \Description
        Called by update function in the platform specific UserApi modules when a profile
        or error is ready. 

    \Input *pRef          - pointer to UserApiT module reference.
    \Input *uUserIndex    - The index of the user associated with this profile request.
    \Input *eError        - A UserApiEventErrorE indicating success for the failure reason.
    \Input  eType         - type of callback
    \Input *pUserData     - pointer to a populated UserApiUserDataT

    \Output
        void

    \Version 05/10/2013 (mcorcoran) - First version
    \Version 12/10/2013 (amakoukji) - Second version for UserApi second pass
*/
/*************************************************************************************************F*/
void _UserApiTriggerCallback(UserApiRefT *pRef, uint32_t uUserIndex, UserApiEventErrorE eError, UserApiEventTypeE eType, UserApiUserDataT *pUserData)
{
    UserApiEventDataT EventData;
    NetCritT *pCrit = &pRef->crit;

    NetCritEnter(pCrit);

    EventData.eError = eError;
    EventData.eEventType = eType;
    EventData.uUserIndex = (uint32_t)uUserIndex;
    ds_memcpy(&EventData.EventDetails.UserData, pUserData, sizeof(UserApiUserDataT));

    if (eType == USERAPI_EVENT_END_OF_LIST)
    {
        EventData.EventDetails.EndOfList.iTotalRequested = pRef->UserContextList[uUserIndex].iTotalRequested + pRef->UserPresenceList[uUserIndex].iTotalRequested + pRef->UserRichPresenceList[uUserIndex].iTotalRequested;
        EventData.EventDetails.EndOfList.iTotalReceived  = pRef->UserContextList[uUserIndex].iTotalReceived  + pRef->UserPresenceList[uUserIndex].iTotalReceived  + pRef->UserRichPresenceList[uUserIndex].iTotalReceived;
        EventData.EventDetails.EndOfList.iTotalErrors    = pRef->UserContextList[uUserIndex].iTotalErrors    + pRef->UserPresenceList[uUserIndex].iTotalErrors    + pRef->UserRichPresenceList[uUserIndex].iTotalErrors;

        if (EventData.EventDetails.EndOfList.iTotalErrors > 0)
        {
            EventData.eError = USERAPI_ERROR_REQUEST_FAILED;
        }
    }
    
    // the actual callback
    if (pRef->pUserCallback[uUserIndex] != NULL)
    {
        pRef->pUserCallback[uUserIndex](pRef, &EventData, pRef->pUserData[uUserIndex]);
    }

    NetCritLeave(pCrit);
}


/*** Public Functions *****************************************************************************/

/*F*************************************************************************************************/
/*!
    \Function    UserApiCreate

    \Description
        Starts UserApi.

    \Output
        UserApiRefT*  - pointer to module ref if successful, NULL otherwise.

    \Version 05/10/2013 (mcorcoran) - First version
*/
/*************************************************************************************************F*/
UserApiRefT * UserApiCreate(void)
{
    UserApiRefT *pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    uint32_t iIndex;

    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pRef = (UserApiRefT*)DirtyMemAlloc(sizeof(*pRef), USERAPI_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("userapi: [%p] failed to allocate module state.\n", pRef));
        return(NULL);
    }
    ds_memclr(pRef, sizeof(*pRef));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;

    pRef->bShuttingDown = FALSE;

    for (iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
    {
        pRef->iLookupUsersLength[iIndex] = -1;
        pRef->iLookupUserIndex[iIndex] = -1;
        pRef->bLookupUserAvailable[iIndex] = TRUE;
        pRef->aLookupUsers[iIndex] = NULL;

        pRef->pUserCallback[iIndex] = NULL;
        pRef->pPostCallback[iIndex] = NULL; 
        pRef->pUserData[iIndex] = NULL;    
        pRef->pUserDataPost[iIndex] = NULL; 
        pRef->uUserDataMask[iIndex] = 0;
        pRef->bAvailableDataIndex[iIndex] = 0;
        pRef->bAvailableDataIndexPresence[iIndex] = 0;
        pRef->bAvailableDataIndexRichPresence[iIndex] = 0;
        pRef->bAvailableDataIndexRMP[iIndex] = FALSE;
    }

    for (iIndex = 0; iIndex < USERAPI_NOTIFY_LIST_MAX_SIZE; ++iIndex)
    {
        pRef->PresenceNotification[iIndex].pCallback = NULL;
        pRef->PresenceNotification[iIndex].pUserData = NULL;
        pRef->TitleNotification[iIndex].pCallback = NULL;
        pRef->TitleNotification[iIndex].pUserData = NULL;
        pRef->RichPresenceNotification[iIndex].pCallback = NULL;
        pRef->RichPresenceNotification[iIndex].pUserData = NULL;
        pRef->ProfileUpdateNotification[iIndex].pCallback = NULL;
        pRef->ProfileUpdateNotification[iIndex].pUserData = NULL;
    }
    pRef->bPresenceNotificationStarted = FALSE;
    pRef->bTitleNotificationStarted = FALSE;
    pRef->bRichPresenceNotificationStarted = FALSE;
    pRef->bProfileUpdateNotificationStarted = FALSE;

    NetCritInit(&pRef->crit, "UserApi");
    NetCritInit(&pRef->postCrit, "UserApiPost");

    pRef->pPlatformData = UserApiPlatCreateData(pRef);
    if (pRef->pPlatformData == NULL)
    {
        NetPrintf(("userapi: [%p] failed to create platform data.\n", pRef));
        UserApiDestroy(pRef);
        return(NULL);
    }

    // return the module ref
    return(pRef);
}

/*F*************************************************************************************************/
/*!
    \Function    UserApiDestroy

    \Description
        Starts shutting down module. Module will not accept new requests, and will abort ongoing ones. Also, 
        registered UserApiCallbackT callback will not be called even if there is data available for processing.

    \Input *pRef - pointer to UserApiT module reference

    \Output
        int32_t  - 0 if successful. Otherwise -1

    \Version 05/10/2013 (mcorcoran) - First version
*/
/*************************************************************************************************F*/
int32_t UserApiDestroy(UserApiRefT *pRef)
{
    int32_t iIndex = 0;
    pRef->bShuttingDown = TRUE;
    NetCritEnter(&pRef->crit);

    for (; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
    {
        pRef->bAvailableDataIndex[iIndex] = 0;
        pRef->bAvailableDataIndexPresence[iIndex] = 0;
        pRef->bAvailableDataIndexRichPresence[iIndex] = 0;
        pRef->bAvailableDataIndexRMP[iIndex] = FALSE;
    }

    NetCritLeave(&pRef->crit);

    if (_UserApiFreeCallback((void*)pRef) < 0)
    {
        NetPrintf(("userapi: [%p] destroy deferred to netconn due to pending async operation.\n", pRef));
        NetConnControl('recu', 0, 0, (void *)_UserApiFreeCallback, pRef);
    }

    // $todo: change return type to void and remove the return statement
    return(0);
}

/*F********************************************************************************/
/*!
    \Function UserApiStatus

    \Description
        Get status information.

    \Input *pRef    - pointer to module state
    \Input iSelect  - status selector
    \Input *pBuf    - [out] storage for selector-specific output
    \Input iBufSize - size of output buffer

    \Output
        int32_t     - Return 0 if successful. Otherwise a selector specific error

    \Notes
        There is currently nothing to query with this module.  This is a placeholder for future implementations.

        \verbatim
        \endverbatim

    \Version 05/10/2013 (mcorcoran) - First version
*/
/********************************************************************************F*/
int32_t UserApiStatus(UserApiRefT *pRef, int32_t iSelect, void *pBuf, int32_t iBufSize)
{
    NetPrintf(("userapi: unhandled status selector '%C'\n", iSelect));
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function UserApiControl

    \Description
        Control behavior of module.

    \Input *pRef    - pointer to module state
    \Input iControl - status selector
    \Input iValue   - control value
    \Input iValue2  - control value
    \Input *pValue  - control value

    \Output
        int32_t     - Return 0 if successful. Otherwise a selector specific error

    \Notes
        iControl can be one of the following:

        \verbatim
            'abrt' - Abort the current request associated with the user at index iValue
            'avsz' - (PS4 only) Set which avatar size will be retrieved. iValue = 's' for small, 'm' for medium and 'l' for big. 's' is the default value, and this is just functional for PS4.
        \endverbatim

    \Version 05/10/2013 (mcorcoran) - First version
*/
/********************************************************************************F*/
int32_t UserApiControl(UserApiRefT *pRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'abrt')
    {
        if ((iValue < 0) || (iValue >= NETCONN_MAXLOCALUSERS))
        {
            NetPrintf(("userapi: [%p] iValue(%d) is not a valid user index\n", pRef, iValue));
            return(-1);
        }

        pRef->pUserCallback[iValue] = NULL;
        pRef->pUserData[iValue] = NULL;

        UserApiPlatAbortRequests(pRef, iValue);

        pRef->UserContextList[iValue].iTotalRequested = 0;
        pRef->UserContextList[iValue].iTotalReceived = 0;
        pRef->UserContextList[iValue].iTotalErrors = 0;

        pRef->iLookupUsersLength[iValue] = -1;
        pRef->iLookupUserIndex[iValue] = -1;
        pRef->bLookupUserAvailable[iValue] = TRUE;
        if (pRef->aLookupUsers[iValue])
        {
            DirtyMemFree(pRef->aLookupUsers[iValue], USERAPI_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData); 
            pRef->aLookupUsers[iValue] = NULL;
        }

        return(0);
    }
    else
    {
        int32_t iRet;
        if ((iRet = UserApiPlatControl(pRef, iControl, iValue, iValue2, pValue)) < 0)
        {
            NetPrintf(("userapi: unhandled control selector '%C'\n", iControl));
        }

        return(iRet);
    }
}

/*F*************************************************************************************************/
/*!
    \Function    UserApiRequestProfilesAsync

    \Description
        Starts the process to retrieve user information of players. pLookupUsers is a pointer to the first DirtyUserT, and iLookupUsersLength is the number of DirtyUserTs.

    \Input *pRef                - pointer to UserApiT module reference
    \Input  uUserIndex          - Index used to specify which local user owns the request
    \Input *pLookupUsers        - Pointer to first DityUserT in an array of iLookupUsersLength elements
    \Input  iLookupUsersLength  - Number of elements in the pLookupUsers array
    \Input *pCallback           - Callback that is going to be called when responses for these requests are received
    \Input *pUserData           - This pointer is going to be passed to the callback when it is called. This parameter can be NULL

    \Output
         int32_t                - Return 0 if successful, -1 otherwise.

    \Version 05/10/2013 (mcorcoran) - First version
    \Version 09/12/2013 (amakoukji) - Second pass with presence
*/
/*************************************************************************************************F*/
int32_t UserApiRequestProfilesAsync(UserApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pLookupUsers, int32_t iLookupUsersLength, UserApiCallbackT *pCallback, void *pUserData)
{
    int32_t iRet;

    if (pLookupUsers == NULL || iLookupUsersLength <= 0)
    {
        NetPrintf(("userapi: [%p] invalid pointer to DirtyUserT list\n", pRef));
        return(-1);
    }

    if (pCallback == NULL)
    {
        NetPrintf(("userapi: [%p] invalid pointer to callback\n", pRef));
        return(-3);
    }

    NetCritEnter(&pRef->crit);

    if (pRef->UserContextList[uUserIndex].iTotalRequested > 0 || pRef->UserPresenceList[uUserIndex].iTotalRequested > 0)
    {
        NetPrintf(("userapi: [%p] module is already handling a request for user (%u)\n", pRef, uUserIndex));
        NetCritLeave(&pRef->crit);
        return(-2);
    }

    pRef->UserContextList[uUserIndex].iTotalRequested = iLookupUsersLength;
    pRef->UserContextList[uUserIndex].iTotalReceived = 0;
    pRef->UserContextList[uUserIndex].iTotalErrors = 0;
    pRef->pUserCallback[uUserIndex] = pCallback;
    pRef->pPostCallback[uUserIndex] = NULL;
    pRef->pUserData[uUserIndex] = pUserData;
    pRef->uUserDataMask[uUserIndex] = USERAPI_MASK_PROFILES; // Mark as batch

    iRet = UserApiPlatRequestProfile(pRef, uUserIndex, pLookupUsers, iLookupUsersLength);

    // clear the context if the request was not successful
    if (iRet < 0)
    {
        pRef->UserContextList[uUserIndex].iTotalRequested = 0;
        pRef->UserContextList[uUserIndex].iTotalReceived = 0;
        pRef->UserContextList[uUserIndex].iTotalErrors = 0;
        pRef->pUserCallback[uUserIndex] = NULL;
        pRef->pPostCallback[uUserIndex] = NULL;
        pRef->pUserData[uUserIndex] = NULL;
        pRef->uUserDataMask[uUserIndex] = 0;
    }

    NetCritLeave(&pRef->crit);

    return(iRet);
}

/*F*************************************************************************************************/
/*!
    \Function    UserApiRequestProfileAsync

    \Description
        Starts the process to retrieve profile information of players. pLookupUser is a pointer to DirtyUserT.

    \Input *pRef                - pointer to UserApiT module reference
    \Input  uUserIndex          - Index used to specify which local user owns the request
    \Input *pLookupUser         - Pointer to DityUserT
    \Input *pCallback           - Callback that is going to be called when responses for these requests are received
    \Input  uUserDataMask       - A mask value defining which elements are needed; USERAPI_MASK_*
    \Input *pUserData           - This pointer is going to be passed to the callback when it is called. This parameter can be NULL

    \Output
         int32_t                - Return 0 if successful, -1 otherwise.

    \Version 09/12/2013 (amakoukji) - First version
*/
/*************************************************************************************************F*/
int32_t UserApiRequestProfileAsync(UserApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pLookupUser, UserApiCallbackT *pCallback, uint32_t uUserDataMask, void *pUserData)
{
    return(UserApiRequestPresenceAsync(pRef, uUserIndex, pLookupUser, pCallback, uUserDataMask, pUserData));
}

/*F*************************************************************************************************/
/*!
    \Function    UserApiRequestPresenceAsync

    \Description
        Starts the process to retrieve profile information of players. pLookupUser is a pointer to DirtyUserT.

    \Input *pRef                - pointer to UserApiT module reference
    \Input  uUserIndex          - Index used to specify which local user owns the request
    \Input *pLookupUser         - Pointer to DityUserT
    \Input *pCallback           - Callback that is going to be called when responses for these requests are received
    \Input  uUserDataMask       - A mask value defining which elements are needed; USERAPI_MASK_*
    \Input *pUserData           - This pointer is going to be passed to the callback when it is called. This parameter can be NULL

    \Output
         int32_t                - Return 0 if successful, -1 otherwise.

    \Version 09/12/2013 (amakoukji) - First version
*/
/*************************************************************************************************F*/
int32_t UserApiRequestPresenceAsync(UserApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pLookupUser, UserApiCallbackT *pCallback, uint32_t uUserDataMask, void *pUserData)
{
    // Start 2 seperate queries, one for a single profile and another for presence / rich presence if necessary
    int32_t iReturn = 0;
    if (uUserDataMask == 0)
    {
        // nothing to do
        NetPrintf(("userapi: [%p] query without a mask value submitted (user %u)\n", pRef, uUserIndex));
        return(-4);
    }

    if (pLookupUser == NULL)
    {
        NetPrintf(("userapi: [%p] invalid pointer to DirtyUserT list\n", pRef));
        return(-5);
    }

    if (pCallback == NULL)
    {
        NetPrintf(("userapi: [%p] invalid pointer to callback\n", pRef));
        return(-3);
    }


    NetCritEnter(&pRef->crit);

    if (pRef->UserContextList[uUserIndex].iTotalRequested > 0 || pRef->UserPresenceList[uUserIndex].iTotalRequested > 0)
    {
        NetPrintf(("userapi: [%p] module is already handling a request for user (%u)\n", pRef, uUserIndex));
        NetCritLeave(&pRef->crit);
        return(-2);
    }
        
    pRef->pUserCallback[uUserIndex] = pCallback;
    pRef->pPostCallback[uUserIndex] = NULL; 
    pRef->pUserData[uUserIndex] = pUserData;
    pRef->uUserDataMask[uUserIndex] = uUserDataMask;

    if ( uUserDataMask & USERAPI_MASK_PROFILE)
    {
        pRef->UserContextList[uUserIndex].iTotalRequested = 1;
        pRef->UserContextList[uUserIndex].iTotalReceived = 0;
        pRef->UserContextList[uUserIndex].iTotalErrors = 0;

        if ((iReturn = UserApiPlatRequestProfile(pRef, uUserIndex, pLookupUser, 1)) < 0)
        {
            _ClearContext(pRef, (int32_t)uUserIndex);
            NetCritLeave(&pRef->crit);
            return(iReturn);
        }
    }

    if ( (uUserDataMask & USERAPI_MASK_PRESENCE))
    {
        pRef->UserPresenceList[uUserIndex].iTotalRequested = 1;
        pRef->UserPresenceList[uUserIndex].iTotalReceived = 0;
        pRef->UserPresenceList[uUserIndex].iTotalErrors = 0;

        if ((iReturn = UserApiPlatRequestPresence(pRef, uUserIndex, pLookupUser)) < 0)
        {
            _ClearContext(pRef, (int32_t)uUserIndex);
            NetCritLeave(&pRef->crit);
            return(iReturn);
        }
    }

    if ((uUserDataMask & USERAPI_MASK_RICH_PRESENCE))
    {
        pRef->UserRichPresenceList[uUserIndex].iTotalRequested = 1;
        pRef->UserRichPresenceList[uUserIndex].iTotalReceived = 0;
        pRef->UserRichPresenceList[uUserIndex].iTotalErrors = 0;

        if ((iReturn = UserApiPlatRequestRichPresence(pRef, uUserIndex, pLookupUser)) < 0)
        {
            _ClearContext(pRef, (int32_t)uUserIndex);
            NetCritLeave(&pRef->crit);
            return(iReturn);
        }
    }

    NetCritLeave(&pRef->crit);

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    UserApiRequestRichPresenceAsync

    \Description
        Starts the process to retrieve rich presence information of players.

    \Input *pRef                - pointer to UserApiT module reference
    \Input  uUserIndex          - Index used to specify which local user owns the request
    \Input *pLookupUser         - Pointer to DirtyUserT
    \Input *pCallback           - Callback that is going to be called when responses for these requests are received
    \Input uUserDataMask        - A mask value defining which elements are needed; USERAPI_MASK_*
    \Input *pUserData           - This pointer is going to be passed to the callback when it is called. This parameter can be NULL

    \Output
         int32_t                - Return 0 if successful, -1 otherwise.

    \Notes
        For PS4:
            strData contains GameStatus
            pData == NULL means delete the rich presence
        For XB1:
            strData contains the Rich Presence String as defined in your Service Config Workbook
            More complex rich presence requests for XB1 will need to be set through their API directly

    \Version 09/12/2013 (amakoukji) - First version
*/
/*************************************************************************************************F*/
int32_t UserApiRequestRichPresenceAsync(UserApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pLookupUser, UserApiCallbackT *pCallback, uint32_t uUserDataMask, void *pUserData)
{ 
    return(UserApiRequestProfileAsync(pRef, uUserIndex, pLookupUser, pCallback, uUserDataMask , pUserData));
}

/*F*************************************************************************************************/
/*!
    \Function    UserApiPostRecentlyMetAsync

    \Description
        Starts the process to post that a certain player was recently encountered.

    \Input *pRef                - pointer to UserApiT module reference
    \Input  uUserIndex          - Index used to specify which local user owns the request
    \Input *pPlayerMet          - Pointer to the DityUserT to add to the recently met players list
    \Input *pAdditionalInfo     - Pointer to additional info container which a platform may require
    \Input *pCallback           - Callback that is going to be called when responses for these requests are received
    \Input *pUserData           - This pointer is going to be passed to the callback when it is called. This parameter can be NULL

    \Output
         int32_t                - Return 0 if successful, -1 otherwise.

    \Version 06/16/2013 (amakoukji) - First version
*/
/*************************************************************************************************F*/
int32_t UserApiPostRecentlyMetAsync(UserApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pPlayerMet, void *pAdditionalInfo, UserApiPostCallbackT *pCallback, void *pUserData)
{
    int32_t result = 0;
    if (pRef == NULL)
    {
        NetPrintf(("userapi: UserApiRecentlyMetAsync() invalid reference pointer\n"));
        return(-1);
    }

    if (uUserIndex >= NETCONN_MAXLOCALUSERS)
    {
        NetPrintf(("userapi: [%p] module, invalid user index (%d)\n", pRef, uUserIndex));
        return(-1);
    }

    if (pPlayerMet == NULL)
    {
        NetPrintf(("userapi: [%p] module, UserApiRecentlyMetAsync() invalid DirtyUserT pointer [%p]\n", pRef, pPlayerMet));
        return(-1);
    }

    NetCritEnter(&pRef->postCrit);

    if (pRef->UserRmpList[uUserIndex].iTotalRequested > 0)
    {
        NetPrintf(("userapi: [%p] module is already handling a recently met player request for user (%d)\n", pRef, uUserIndex));
        NetCritLeave(&pRef->postCrit);
        return(-2);
    }

    pRef->UserRmpList[uUserIndex].iTotalRequested = 1;
    pRef->UserRmpList[uUserIndex].iTotalReceived = 0;
    pRef->UserRmpList[uUserIndex].iTotalErrors = 0;
    pRef->pPostCallback[uUserIndex] = pCallback;
    pRef->pUserDataPost[uUserIndex] = pUserData;

    result = UserApiPlatRequestRecentlyMet(pRef, (int32_t)uUserIndex, pPlayerMet, pAdditionalInfo);

    if (result < 0)
    {
        // An error occured, reset
        pRef->UserRmpList[uUserIndex].iTotalRequested = 0;
        pRef->UserRmpList[uUserIndex].iTotalReceived = 0;
        pRef->UserRmpList[uUserIndex].iTotalErrors = 0;
        pRef->pPostCallback[uUserIndex] = NULL;
        pRef->pUserDataPost[uUserIndex] = NULL;
    }

    NetCritLeave(&pRef->postCrit);

    return(result);
}

/*F*************************************************************************************************/
/*!
    \Function    UserApiPostRichPresenceAsync

    \Description
        Starts the process to post that a certain player was recently encountered.

    \Input *pRef                - pointer to UserApiT module reference
    \Input  uUserIndex          - Index used to specify which local user owns the request
    \Input *pData               - Rich Presence to post
    \Input *pCallback           - Callback that is going to be called when responses for these requests are received
    \Input *pUserData           - This pointer is going to be passed to the callback when it is called. This parameter can be NULL

    \Output
         int32_t                - Return 0 if successful, -1 otherwise.

    \Version 06/16/2013 (amakoukji) - First version
*/
/*************************************************************************************************F*/
int32_t UserApiPostRichPresenceAsync(UserApiRefT *pRef, uint32_t uUserIndex, UserApiRichPresenceT *pData, UserApiPostCallbackT *pCallback, void *pUserData)
{
    int32_t result = 0;
    if (pRef == NULL)
    {
        NetPrintf(("userapi: UserApiPostRichPresenceAsync() invalid reference pointer\n"));
        return(-1);
    }

    if (uUserIndex >= NETCONN_MAXLOCALUSERS)
    {
        NetPrintf(("userapi: [%p] module, invalid user index (%d)\n", pRef, uUserIndex));
        return(-2);
    }

    NetCritEnter(&pRef->postCrit);

    if (pRef->UserRmpList[uUserIndex].iTotalRequested > 0)
    {
        NetPrintf(("userapi: [%p] module is already handling a recently met player request for user (%d)\n", pRef, uUserIndex));
        NetCritLeave(&pRef->postCrit);
        return(-2);
    }

    pRef->UserRmpList[uUserIndex].iTotalRequested = 1;
    pRef->UserRmpList[uUserIndex].iTotalReceived = 0;
    pRef->UserRmpList[uUserIndex].iTotalErrors = 0;
    pRef->pPostCallback[uUserIndex] = pCallback;
    pRef->pUserDataPost[uUserIndex] = pUserData;

    result = UserApiPlatRequestPostRichPresence(pRef, (int32_t)uUserIndex, pData);

    NetCritLeave(&pRef->postCrit);

    return(result);
}

/*F*************************************************************************************************/
/*!
    \Function    UserApiRegisterUpdateEvent

    \Description
        Register for notifications from first part

    \Input *pRef                - pointer to UserApiT module reference
    \Input  uUserIndex          - Index used to specify which local user owns the request
    \Input  eType               - type of event to register for
    \Input *pNotifyCb           - Callback that is going to be called when responses for these requests are received
    \Input *pUserData           - This pointer is going to be passed to the callback when it is called. This parameter can be NULL

    \Output
         int32_t                - Return 0 if successful, -1 otherwise.

    \Version 06/16/2013 (amakoukji) - First version
*/
/*************************************************************************************************F*/
int32_t UserApiRegisterUpdateEvent(UserApiRefT *pRef, uint32_t uUserIndex, UserApiNotifyTypeE eType, UserApiUpdateCallbackT *pNotifyCb, void *pUserData)
{
    int32_t iReturn = USERAPI_ERROR_OK;
    UserApiNotificationT (*pList)[USERAPI_NOTIFY_LIST_MAX_SIZE] = NULL;
    uint8_t bNeedsInit = TRUE;
    int32_t i = 0;

    if (eType == USERAPI_NOTIFY_PRESENCE_UPDATE)
    {
        pList = &pRef->PresenceNotification;
        if (pRef->bPresenceNotificationStarted)
        {
            bNeedsInit = FALSE;
        }
        pRef->bPresenceNotificationStarted = TRUE;
    }
    else if (eType == USERAPI_NOTIFY_TITLE_UPDATE)
    {
        pList = &pRef->TitleNotification;
        if (pRef->bTitleNotificationStarted)
        {
            bNeedsInit = FALSE;
        }
        pRef->bTitleNotificationStarted = TRUE;
    }
    else if (eType == USERAPI_NOTIFY_RICH_PRESENCE_UPDATE)
    {
        pList = &pRef->RichPresenceNotification;
        if (pRef->bRichPresenceNotificationStarted)
        {
            bNeedsInit = FALSE;
        }
        pRef->bRichPresenceNotificationStarted = TRUE;
    }
    else if (eType == USERAPI_NOTIFY_PROFILE_UPDATE)
    {
        pList = &pRef->ProfileUpdateNotification;
        if (pRef->bProfileUpdateNotificationStarted)
        {
            bNeedsInit = FALSE;
        }
        pRef->bProfileUpdateNotificationStarted = TRUE;
    }

    else
    {
        return(USERAPI_ERROR_UNSUPPORTED);
    }

    if (pList == NULL)
    {
        return(USERAPI_ERROR_FULL);
    }

    if (bNeedsInit)
    {
        iReturn = UserApiPlatRegisterUpdateEvent(pRef, uUserIndex, eType);
    }

    if (iReturn >= 0)
    {
        for (i = 0; i < USERAPI_NOTIFY_LIST_MAX_SIZE; ++i)
        {
            if ((*pList)[i].pCallback == pNotifyCb && (*pList)[i].pUserData == pUserData && (*pList)[i].uUserIndex == uUserIndex)
            {
                // callback already setup 
                break;
            }

            if ((*pList)[i].pCallback == NULL)
            {
                (*pList)[i].pCallback = pNotifyCb;
                (*pList)[i].pUserData = pUserData;
                (*pList)[i].uUserIndex = uUserIndex;
                break;
            }
        }
    }
    
    return(iReturn);
}

/*F*************************************************************************************************/
/*!
    \Function    UserApiCancel

    \Description
        Cancel all queries to the 1st party. 

    \Input *pRef        - pointer to UserApiT module reference
    \Input uUserIndex   - index of user associated with this request

    \Output
        int32_t         - result of abort request; see UserApiPlatAbortRequests()

    \Version 05/10/2013 (mcorcoran) - First version
*/
/*************************************************************************************************F*/
int32_t UserApiCancel(UserApiRefT *pRef, uint32_t uUserIndex)
{
    _ClearContext(pRef, (int32_t)uUserIndex);
    pRef->pUserCallback[uUserIndex] = NULL;
    pRef->pPostCallback[uUserIndex] = NULL; 
    pRef->pUserDataPost[uUserIndex] = NULL; 
    return(UserApiPlatAbortRequests(pRef, (int32_t)uUserIndex));
}

/*F*************************************************************************************************/
/*!
    \Function    UserApiUpdate

    \Description
        Update the internal state of the module, and call registered UserApiCallbackT callback if there are GamerCard/Profile responses available. This function should be called periodically.

    \Input *pRef - pointer to UserApiT module reference

    \Version 05/10/2013 (mcorcoran) - First version
*/
/*************************************************************************************************F*/
void UserApiUpdate(UserApiRefT *pRef)
{
    UserApiUserDataT      UserData;
    UserApiProfileT      *pProfileData = &UserData.Profile;
    UserApiPresenceT     *pPresenceData = &UserData.Presence; 
    UserApiRichPresenceT *pRichPresenceData = &UserData.RichPresence;
    int32_t               iProcessingResult = 0;
    int32_t               i = 0;
    int32_t               iInnerLoop = 0;

    if (pRef == NULL)
    {
        NetPrintf(("userapi: invalid reference pointer\n"));
        return;
    }

    UserApiPlatUpdate(pRef);

    for (i = 0; i < NETCONN_MAXLOCALUSERS; ++i)
    {
        NetCritEnter(&pRef->crit);

        ds_memclr(&UserData, sizeof(UserData));
        UserData.uUserDataMask = pRef->uUserDataMask[i];

        // Check for batch profile fetch.
        // This is treated seperately because it never needs to wait for other 1st party requests to finish
        if (pRef->uUserDataMask[i] == USERAPI_MASK_PROFILES)
        {
            if (pRef->bAvailableDataIndex[i] > 0)
            {
                iProcessingResult = _UserApiProcessProfileResponse(pRef, i, TRUE, pProfileData, &UserData);

                // If the processing failed report it and cancel all further 1st party requests
                if (iProcessingResult < 0)
                {
                    _UserApiTriggerCallback(pRef, i, iProcessingResult, USERAPI_EVENT_END_OF_LIST, &UserData);
                    _ClearContext(pRef, i);
                }
                // If all the requested results have been accounted for send the "list end" callback
                else if (pRef->UserContextList[i].iTotalRequested == (pRef->UserContextList[i].iTotalReceived + pRef->UserContextList[i].iTotalErrors))
                {
                    _UserApiTriggerCallback(pRef, i, USERAPI_ERROR_OK, USERAPI_EVENT_END_OF_LIST, &UserData);
                    _ClearContext(pRef, i);
                }
                // If all requests are not complete for the user, mark the lookup available as true in order to continue
                else 
                {
                    pRef->bLookupUserAvailable[i] = TRUE;
                }

                pRef->bAvailableDataIndex[i] = FALSE;    
                pRef->bAvailableDataIndexPresence[i] = FALSE;  
                pRef->bAvailableDataIndexRichPresence[i] = FALSE;  
            }
        }

        // then check individual requests which may need to wait for several 1st party queries to finish
        else if ((pRef->uUserDataMask[i] & USERAPI_MASK_PROFILE) || (pRef->uUserDataMask[i] & USERAPI_MASK_PRESENCE) || (pRef->uUserDataMask[i] & USERAPI_MASK_RICH_PRESENCE))
        {
            if ( ((pRef->bAvailableDataIndex[i] > 0         && (pRef->uUserDataMask[i] & USERAPI_MASK_PROFILE))  || !(pRef->uUserDataMask[i] & USERAPI_MASK_PROFILE))  
            &&   ((pRef->bAvailableDataIndexPresence[i] > 0 && (pRef->uUserDataMask[i] & USERAPI_MASK_PRESENCE)) || !(pRef->uUserDataMask[i] & USERAPI_MASK_PRESENCE)) 
            &&   ((pRef->bAvailableDataIndexRichPresence[i] > 0 && (pRef->uUserDataMask[i] & USERAPI_MASK_RICH_PRESENCE)) || !(pRef->uUserDataMask[i] & USERAPI_MASK_RICH_PRESENCE)) )
            {
                // At this point all 1st party queries are complete
                // Parse the data, report it to the user and reset
                iProcessingResult = 0;

                if (pRef->uUserDataMask[i] & USERAPI_MASK_PROFILE)
                {
                    iProcessingResult = _UserApiProcessProfileResponse(pRef, i, FALSE, pProfileData, &UserData);
                }

                if (pRef->uUserDataMask[i] & USERAPI_MASK_PRESENCE && iProcessingResult >= 0)
                {
                    iProcessingResult = _UserApiProcessPresenceResponse(pRef, i, pPresenceData, &UserData);
                }

                if (pRef->uUserDataMask[i] & USERAPI_MASK_RICH_PRESENCE && iProcessingResult >= 0)
                {
                    iProcessingResult = _UserApiProcessRichPresenceResponse(pRef, i, pRichPresenceData, &UserData);
                }

                // Callback
                _UserApiTriggerCallback(pRef, i, (iProcessingResult >= 0) ? USERAPI_ERROR_OK : iProcessingResult, 
                                        (iProcessingResult >= 0) ? USERAPI_EVENT_DATA : USERAPI_EVENT_END_OF_LIST, &UserData);

                // Clean up
                _ClearContext(pRef, i);
                pRef->bAvailableDataIndex[i] = FALSE;    
                pRef->bAvailableDataIndexPresence[i] = FALSE;  
                pRef->bAvailableDataIndexRichPresence[i] = FALSE;  
            }
        }

        NetCritLeave(&pRef->crit);
    }

    // Handle Posted responses
    NetCritEnter(&pRef->postCrit);
    for (i = 0; i < NETCONN_MAXLOCALUSERS; ++i)
    {

        if (pRef->bAvailableDataIndexRMP[i] != FALSE)
        {
            _UserApiTriggerPostCallback(pRef, i);

            // Clear context
            pRef->UserRmpList[i].iTotalRequested = 0;
            pRef->UserRmpList[i].iTotalReceived = 0;
            pRef->UserRmpList[i].iTotalErrors = 0;
            pRef->bAvailableDataIndexRMP[i] = FALSE;
        }
    }

    // finally process notifications from Sony
    if (pRef->UserApiNotifyEvent[0].pNotificationData != NULL)
    {
        int32_t iOuterLoop = 0;
        // at least one notification is in the list, do processing
        for (; iOuterLoop < USERAPI_MAX_QUEUED_NOTIFICATIONS; ++iOuterLoop)
        {
            if (pRef->UserApiNotifyEvent[iOuterLoop].pNotificationData != NULL)
            {
                uint32_t uNotficationUserId = pRef->UserApiNotifyEvent[iOuterLoop].uUserIndex;
                // dispatch notifications
                for (iInnerLoop = 0; iInnerLoop < USERAPI_NOTIFY_LIST_MAX_SIZE; ++iInnerLoop)
                {
                    if ((*(pRef->UserApiNotifyEvent[iOuterLoop].pNotificationList))[iInnerLoop].pCallback != NULL)
                    {
                        // if the notification is for the requestor of the callback
                        if (uNotficationUserId == (*(pRef->UserApiNotifyEvent[iOuterLoop].pNotificationList))[iInnerLoop].uUserIndex)
                        {
                            (*(pRef->UserApiNotifyEvent[iOuterLoop].pNotificationList))[iInnerLoop].pCallback(pRef, 
                                                                                                              pRef->UserApiNotifyEvent[iOuterLoop].pNotificationType, 
                                                                                                              pRef->UserApiNotifyEvent[iOuterLoop].pNotificationData, 
                                                                                                              (*(pRef->UserApiNotifyEvent[iOuterLoop].pNotificationList))[iInnerLoop].pUserData);
                        }
                    }
                }

                // clean up
                DirtyMemFree(pRef->UserApiNotifyEvent[iOuterLoop].pNotificationData, USERLISTAPI_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
                pRef->UserApiNotifyEvent[iOuterLoop].pNotificationData = NULL;
                pRef->UserApiNotifyEvent[iOuterLoop].pNotificationList = NULL;
            }
        }
    }

    NetCritLeave(&pRef->postCrit);

}
