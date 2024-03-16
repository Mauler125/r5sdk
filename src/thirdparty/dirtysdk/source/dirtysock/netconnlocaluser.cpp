/*H********************************************************************************/
/*!
    \File netconnlocaluser.cpp

    \Description
        Cross-platform netconn data types and functions.

    \Copyright
        Copyright (c) 2014 Electronic Arts Inc.

    \Version 05/21/2014 (mclouatre)  First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "netconnlocaluser.h"

#ifndef DIRTYCODE_PS5
#include "IEAUser/IEAUser.h"
#endif

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

static NetConnLocalUserRefT  *_NetConnLocalUser_pRef = NULL;          //!< module state pointer

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function   _NetConnLocalUserEnqueueIEAUserEvent

    \Description
        Add the specified entry at the head of the specified list.

    \Input *pUserEventEntry - event entry to be enqueued
    \Input **pList          - list to add the event to

    \Version 05/09/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnLocalUserEnqueueIEAUserEvent(NetConnIEAUserEventT *pUserEventEntry, NetConnIEAUserEventT **pList)
{
    if (*pList != NULL)
    {
        pUserEventEntry->pNext = *pList;
    }

    *pList = pUserEventEntry;
}

/*F********************************************************************************/
/*!
    \Function   _NetConnLocalUserDequeueIEAUserEvent

    \Description
        Get an event entry from the tail fo the specified list.

    \Input **pList              - list to dequeue from

    \Output
        NetConnIEAUserEventT *  - pointer to free IEAUserEvent entry

    \Version 05/09/2014 (mclouatre)
*/
/********************************************************************************F*/
static NetConnIEAUserEventT * _NetConnLocalUserDequeueIEAUserEvent(NetConnIEAUserEventT **pList)
{
    NetConnIEAUserEventT *pUserEventEntry = NULL;

    // find tail
    if (*pList != NULL)
    {
        NetConnIEAUserEventT *pPrevious = NULL;
        for (pUserEventEntry = *pList; pUserEventEntry->pNext != NULL; pUserEventEntry = pUserEventEntry->pNext)
        {
            pPrevious = pUserEventEntry;
        }

        if (pPrevious)
        {
            pPrevious->pNext = NULL;
        }
        else
        {
           *pList = NULL;
        }
    }

    return(pUserEventEntry);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnLocalUserGetFreeIEAUserEvent

    \Description
        Get a free IEAUserEvent entry from the free list.

    \Input *pLocalUserRef          - netconn module state

    \Output
        NetConnIEAUserEventT *  - pointer to free IEAUserEvent entry

    \Version 05/09/2014 (mclouatre)
*/
/********************************************************************************F*/
static NetConnIEAUserEventT * _NetConnLocalUserGetFreeIEAUserEvent(NetConnLocalUserRefT *pLocalUserRef)
{
    NetConnIEAUserEventT *pUserEventEntry;

    // check if free list is empty
    if (pLocalUserRef->pIEAUserFreeEventList == NULL)
    {
        int32_t iLoop = 0;

        // add 4 entries to the free list
        for (iLoop = 0; iLoop < 4; iLoop++)
        {
            pUserEventEntry = (NetConnIEAUserEventT *) DirtyMemAlloc(sizeof(NetConnIEAUserEventT), NETCONN_MEMID, pLocalUserRef->iMemGroup, pLocalUserRef->pMemGroupUserData);

            if (pUserEventEntry)
            {
                ds_memclr(pUserEventEntry, sizeof(*pUserEventEntry));

                _NetConnLocalUserEnqueueIEAUserEvent(pUserEventEntry, &pLocalUserRef->pIEAUserFreeEventList);

                NetPrintfVerbose((pLocalUserRef->iDebugLevel, 0, "netconnlocaluser: [%p] allocated a new free user event entry\n", pUserEventEntry));
            }
            else
            {
                NetPrintf(("netconnlocaluser: failed to allocate a new user event entry\n"));
            }
        }
    }

    pUserEventEntry = _NetConnLocalUserDequeueIEAUserEvent(&pLocalUserRef->pIEAUserFreeEventList);

    return(pUserEventEntry);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnLocalUserAddIEAUserEvent

    \Description
        Add an entry to the list of IEAUserEvents

    \Input *pLocalUserRef   - netconn module state
    \Input iLocalUserIndex  - local user index
    \Input *pIEAUser        - pointer to IEAUser object
    \Input eEvent           - event type

    \Output
        int32_t             - 0 for success; negative for error

    \Version 05/09/2014 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _NetConnLocalUserAddIEAUserEvent(NetConnLocalUserRefT *pLocalUserRef, int32_t iLocalUserIndex, const EA::User::IEAUser *pIEAUser, NetConnIEAUserEventTypeE eEvent)
{
    int32_t iResult = 0;

    NetCritEnter(&pLocalUserRef->crit);

    NetConnIEAUserEventT *pUserEventEntry = _NetConnLocalUserGetFreeIEAUserEvent(pLocalUserRef);

    if (pUserEventEntry)
    {
        pUserEventEntry->eEvent = eEvent;
        pUserEventEntry->iLocalUserIndex = iLocalUserIndex;
        pUserEventEntry->pIEAUser = pIEAUser;
#ifndef DIRTYCODE_PS5
        pUserEventEntry->pIEAUser->AddRef();
#endif

        _NetConnLocalUserEnqueueIEAUserEvent(pUserEventEntry, &pLocalUserRef->pIEAUserEventList);

#ifndef DIRTYCODE_PS5
        NetPrintfVerbose((pLocalUserRef->iDebugLevel, 0, "netconnlocaluser: [%p] IEAUser event queued (local user index = %d, local user id = 0x%08x, event = %s)\n",
            pUserEventEntry, iLocalUserIndex, (int32_t)pIEAUser->GetUserID(), (eEvent==NETCONN_EVENT_IEAUSER_ADDED?"added":"removed")));
#endif
    }
    else
    {
        iResult = -1;
    }

    NetCritLeave(&pLocalUserRef->crit);

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnLocalUserClearIEAUserEventList

    \Description
        Clear list of pending IEAUser events.

    \Input *pLocalUserRef  - netconn module state

    \Version 05/09/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnLocalUserClearIEAUserEventList(NetConnLocalUserRefT *pLocalUserRef)
{
    NetConnIEAUserEventT *pUserEventEntry;

    NetCritEnter(&pLocalUserRef->crit);

    while ((pUserEventEntry = _NetConnLocalUserDequeueIEAUserEvent(&pLocalUserRef->pIEAUserEventList)) != NULL)
    {
        // return event entry to free list
#ifndef DIRTYCODE_PS5
        pUserEventEntry->pIEAUser->Release();
#endif
        ds_memclr(pUserEventEntry, sizeof(*pUserEventEntry));
        _NetConnLocalUserEnqueueIEAUserEvent(pUserEventEntry, &pLocalUserRef->pIEAUserFreeEventList);
    }

    NetCritLeave(&pLocalUserRef->crit);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnLocalUserClearIEAUserFreeEventList

    \Description
        Clear list of free IEAUser events.

    \Input *pLocalUserRef  - netconn module state

    \Version 05/09/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnLocalUserClearIEAUserFreeEventList(NetConnLocalUserRefT *pLocalUserRef)
{
    NetConnIEAUserEventT *pUserEventEntry;

    NetCritEnter(&pLocalUserRef->crit);

    while ((pUserEventEntry = _NetConnLocalUserDequeueIEAUserEvent(&pLocalUserRef->pIEAUserFreeEventList)) != NULL)
    {
        DirtyMemFree(pUserEventEntry, NETCONN_MEMID, pLocalUserRef->iMemGroup, pLocalUserRef->pMemGroupUserData);

        NetPrintfVerbose((pLocalUserRef->iDebugLevel, 0, "netconnlocaluser: [%p] freed user event entry\n", pUserEventEntry));
    }

    NetCritLeave(&pLocalUserRef->crit);
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function    NetConnLocalUserInit

    \Description
        Start up local user functionality.

    \Input *pNetConn        - parent NetConn reference
    \Input *pAddUserCb      - added user event handler
    \Input *pRemoveUserCb   - removed user event handler

    \Output
        NetConnLocalUserRefT - local user reference on success; NULL on error

    \Version 10/24/2017 (amakoukji)
*/
/********************************************************************************F*/
NetConnLocalUserRefT* NetConnLocalUserInit(NetConnRefT *pNetConn, NetConnAddLocalUserCallbackT *pAddUserCb, NetConnRemoveLocalUserCallbackT *pRemoveUserCb)
{
    NetConnLocalUserRefT *pLocalUserRef = _NetConnLocalUser_pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // refcount if already started
    if (pLocalUserRef != NULL)
    {
        NetPrintf(("netconnlocaluser: NetConnLocalUserInit() called while module is already active\n"));
        return(NULL);
    }

    // alloc and init ref
    if ((pLocalUserRef = (NetConnLocalUserRefT*)DirtyMemAlloc(sizeof(NetConnLocalUserRefT), NETCONN_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netconnlocaluser: unable to allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pLocalUserRef, sizeof(NetConnLocalUserRefT));
    pLocalUserRef->iMemGroup = iMemGroup;
    pLocalUserRef->pMemGroupUserData = pMemGroupUserData;
    pLocalUserRef->pAddUserCb = pAddUserCb;
    pLocalUserRef->pRemoveUserCb = pRemoveUserCb;
    pLocalUserRef->pNetConn = pNetConn;

    NetCritInit(&pLocalUserRef->crit, "netconnlocaluserCrit");

    // save ref
    _NetConnLocalUser_pRef = pLocalUserRef;

    // successful completion
    return(pLocalUserRef);
}

/*F********************************************************************************/
/*!
    \Function    NetConnLocalUserDestroy

    \Description
        Shutdown common functionality.

    \Input *pLocalUserRef - module state

    \Version 10/24/2017 (amakoukji)
*/
/********************************************************************************F*/
void NetConnLocalUserDestroy(NetConnLocalUserRefT *pLocalUserRef)
{
    _NetConnLocalUserClearIEAUserEventList(pLocalUserRef);
    _NetConnLocalUserClearIEAUserFreeEventList(pLocalUserRef);

    NetCritKill(&pLocalUserRef->crit);
    
    // dispose of ref
    DirtyMemFree(pLocalUserRef, NETCONN_MEMID, pLocalUserRef->iMemGroup, pLocalUserRef->pMemGroupUserData);
    _NetConnLocalUser_pRef = NULL;
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnLocalUserAdd

    \Description
        Use this function to tell netconn about a newly detected local user on the local console.

    \Input iLocalUserIndex  - index at which DirtySDK needs to insert this user it its internal user array
    \Input pLocalUser       - pointer to associated IEAUser

    \Output
        int32_t             - 0 for success; negative for error

    \Version 01/16/2014 (mclouatre)
*/
/*************************************************************************************************F*/
int32_t NetConnLocalUserAdd(int32_t iLocalUserIndex, const EA::User::IEAUser *pLocalUser)
{
    NetConnLocalUserRefT *pLocalUserRef = _NetConnLocalUser_pRef;
    int32_t iRetCode = 0;   // default to success

    if ((iLocalUserIndex >= 0) && (iLocalUserIndex < NETCONN_MAXLOCALUSERS))
    {
        iRetCode = _NetConnLocalUserAddIEAUserEvent(pLocalUserRef, iLocalUserIndex, pLocalUser, NETCONN_EVENT_IEAUSER_ADDED);
    }
    else
    {
        NetPrintf(("netconnlocaluser: NetConnLocalUserAddLocalUser() called with an invalid index (%d)\n", iLocalUserIndex));
        iRetCode = -1;
    }

    return(iRetCode);
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnLocalUserRemove

    \Description
        Use this function to tell netconn about a local user that no longer exists

    \Input iLocalUserIndex  - index in the internal DirtySDK user array at which the user needs to be cleared
                              pass -1 when the index is unknown and a lookup will be done
    \Input pLocalUser       - pointer to associated IEAUser

    \Output
        int32_t             - 0 for success; negative for error

    \Version 01/16/2014 (mclouatre)
*/
/*************************************************************************************************F*/
int32_t NetConnLocalUserRemove(int32_t iLocalUserIndex, const EA::User::IEAUser *pLocalUser)
{
    NetConnLocalUserRefT *pLocalUserRef = _NetConnLocalUser_pRef;
    int32_t iRetCode = 0;   // default to success

    if (iLocalUserIndex == -1)
    {
        NetCritEnter(&pLocalUserRef->crit);
        iLocalUserIndex = NetConnStatus('usri', 0, (void *)pLocalUser, 0);
        NetCritLeave(&pLocalUserRef->crit);
    }

    if ((iLocalUserIndex >= 0) && (iLocalUserIndex < NETCONN_MAXLOCALUSERS))
    {
        iRetCode = _NetConnLocalUserAddIEAUserEvent(pLocalUserRef, iLocalUserIndex, pLocalUser, NETCONN_EVENT_IEAUSER_REMOVED);
    }
    else
    {
        NetPrintf(("netconnlocaluser: NetConnLocalUserRemoveLocalUser() called with an invalid index (%d)\n", iLocalUserIndex));
        iRetCode = -1;
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function   NetConnLocalUserUpdate

    \Description
        Update our internally maintained array of NetConnUsers from the array
        of IEAUsers

    \Input *pLocalUserRef  - module reference

    \Version 01/18/2014 (mclouatre)
*/
/********************************************************************************F*/
void NetConnLocalUserUpdate(NetConnLocalUserRefT *pLocalUserRef)
{
    NetConnIEAUserEventT *pUserEventEntry;

    if (pLocalUserRef != NULL)
    {
        NetCritEnter(&pLocalUserRef->crit);

        // process events
        while ((pUserEventEntry = _NetConnLocalUserDequeueIEAUserEvent(&pLocalUserRef->pIEAUserEventList)) != NULL)
        {
            if (pUserEventEntry->eEvent == NETCONN_EVENT_IEAUSER_ADDED)
            {
                if (pLocalUserRef->pAddUserCb != NULL)
                {
                    pLocalUserRef->pAddUserCb(pLocalUserRef, pUserEventEntry->iLocalUserIndex, pUserEventEntry->pIEAUser);
                }
            }
            else
            {
                if (pLocalUserRef->pRemoveUserCb != NULL)
                {
                    pLocalUserRef->pRemoveUserCb(pLocalUserRef, pUserEventEntry->iLocalUserIndex, pUserEventEntry->pIEAUser);
                }
            }

#ifndef DIRTYCODE_PS5
            NetPrintfVerbose((pLocalUserRef->iDebugLevel, 0, "netconnlocaluser: [%p] IEAUser event dequeued (local user index = %d, local user id = 0x%08x, event = %s)\n",
                pUserEventEntry, pUserEventEntry->iLocalUserIndex, (int32_t)pUserEventEntry->pIEAUser->GetUserID(), (pUserEventEntry->eEvent == NETCONN_EVENT_IEAUSER_ADDED ? "added" : "removed")));

            // return event entry to free list
            pUserEventEntry->pIEAUser->Release();
#endif
            ds_memclr(pUserEventEntry, sizeof(*pUserEventEntry));
            _NetConnLocalUserEnqueueIEAUserEvent(pUserEventEntry, &pLocalUserRef->pIEAUserFreeEventList);
        }

        NetCritLeave(&pLocalUserRef->crit);
    }
}
