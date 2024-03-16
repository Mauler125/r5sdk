/*H*************************************************************************************************/
/*!

    \File    userapipriv.h

    \Description
        Expose first party player information

    \Notes
         None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2001-2013.    ALL RIGHTS RESERVED.

    \Version 05/10/2013 (mcorcoran) First Version

*/
/*************************************************************************************************H*/

#ifndef _userapipriv_h
#define _userapipriv_h

/*** Include files ********************************************************************************/

#include "DirtySDK/misc/userapi.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/netconn.h"

/*** Defines **************************************************************************************/

#define USERAPI_MAX_QUEUED_NOTIFICATIONS (100)

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

typedef struct UserApiUserContextT
{
    int32_t               iTotalRequested;   //!< the total number of user profiles that are being looked up
    int32_t               iTotalReceived;    //!< the number of user profiles received
    int32_t               iTotalErrors;      //!< the number erros that have occured
} UserApiUserContextT;

typedef struct UserApiUserContextRMPT
{
    int32_t            iTotalRequested;   //!< the total number of user profiles that are being looked up
    int32_t            iTotalReceived;    //!< the number of user profiles received
    int32_t            iTotalErrors;      //!< the number erros that have occured
    UserApiPostCallbackT  *pUserCallback;     //!< callback to user code that will be called when data is available during a call to UserApiUpdate()
    void              *pUserData;         //!< user data for the callback
} UserApiUserContextRMPT;

typedef struct UserApiNotificationT
{
    UserApiUpdateCallbackT *pCallback;         //!< function address
    void                   *pUserData;         //!< user data to return
    uint32_t                uUserIndex;        //!< user index of the requester
} UserApiNotificationT;

typedef struct UserApiNotifyEventT
{
    UserApiNotifyDataT    *pNotificationData;
    UserApiNotificationT (*pNotificationList)[];
    UserApiNotifyTypeE     pNotificationType;
    uint32_t               uUserIndex;
} UserApiNotifyEventT;

typedef struct UserApiPlatformDataT UserApiPlatformDataT;

struct UserApiRefT
{
    int32_t                iMemGroup;                                 //!< dirtymem memory group
    void                  *pMemGroupUserData;                         //!< dirtymem memory group user data
    NetCritT               crit;                                      //!< sychronize shared data between the threads for profiles
    NetCritT               postCrit;                                  //!< sychronize shared data between the threads for POSTing data
    uint8_t                bShuttingDown;

    UserApiUserContextT    UserContextList[NETCONN_MAXLOCALUSERS];    //!< per local user data for profile requests
    UserApiUserContextT    UserPresenceList[NETCONN_MAXLOCALUSERS];   //!< per local user data for presence requests
    UserApiUserContextT    UserRichPresenceList[NETCONN_MAXLOCALUSERS];   //!< per local user data for presence requests
    UserApiUserContextT    UserRmpList[NETCONN_MAXLOCALUSERS];        //!< per local user data for recently met player requests
    UserApiCallbackT      *pUserCallback[NETCONN_MAXLOCALUSERS];      //!< callback to user code that will be called when data is available during a call to UserApiUpdate()
    UserApiPostCallbackT  *pPostCallback[NETCONN_MAXLOCALUSERS];      //!< callback to user code that will be called when data is available for POSTs
    void                  *pUserData[NETCONN_MAXLOCALUSERS];          //!< user data for the callback
    void                  *pUserDataPost[NETCONN_MAXLOCALUSERS];      //!< user data for the callback for POSTs
    uint32_t               uUserDataMask[NETCONN_MAXLOCALUSERS];      //!> request mask

    volatile uint8_t       bAvailableDataIndex[NETCONN_MAXLOCALUSERS];             //!< mask denoting which user has data waiting to pick up
    volatile uint8_t       bAvailableDataIndexPresence[NETCONN_MAXLOCALUSERS];     //!< mask denoting which user has data waiting to pick up
    volatile uint8_t       bAvailableDataIndexRichPresence[NETCONN_MAXLOCALUSERS]; //!< mask denoting which user has data waiting to pick up
    volatile uint8_t       bAvailableDataIndexRMP[NETCONN_MAXLOCALUSERS];          //!< mask denoting which user has data waiting to pick up
    UserApiPlatformDataT  *pPlatformData;

    DirtyUserT            *aLookupUsers[NETCONN_MAXLOCALUSERS];
    int32_t                iLookupUsersLength[NETCONN_MAXLOCALUSERS];
    int32_t                iLookupUserIndex[NETCONN_MAXLOCALUSERS];
    uint8_t                bLookupUserAvailable[NETCONN_MAXLOCALUSERS];
    int32_t                iLookupsSent[NETCONN_MAXLOCALUSERS];
    uint8_t                bLookupRmpAvailable[NETCONN_MAXLOCALUSERS];

    // Callbacks
    UserApiNotificationT PresenceNotification[USERAPI_NOTIFY_LIST_MAX_SIZE];
    UserApiNotificationT TitleNotification[USERAPI_NOTIFY_LIST_MAX_SIZE];
    UserApiNotificationT RichPresenceNotification[USERAPI_NOTIFY_LIST_MAX_SIZE];
    UserApiNotificationT ProfileUpdateNotification[USERAPI_NOTIFY_LIST_MAX_SIZE];
    uint8_t bPresenceNotificationStarted;
    uint8_t bTitleNotificationStarted;
    uint8_t bRichPresenceNotificationStarted;
    uint8_t bProfileUpdateNotificationStarted;
    UserApiNotifyEventT UserApiNotifyEvent[USERAPI_MAX_QUEUED_NOTIFICATIONS];
};

/*** Function Prototypes **************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

UserApiPlatformDataT *UserApiPlatCreateData(UserApiRefT *pRef);
int32_t UserApiPlatDestroyData(UserApiRefT *pRef, UserApiPlatformDataT *pPlatformData);
int32_t UserApiPlatRequestProfile(UserApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pLookupUsers, int32_t iLookupUsersLength);
int32_t UserApiPlatAbortRequests(UserApiRefT *pRef, uint32_t uUserIndex);
int32_t UserApiPlatRequestPresence(UserApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pLookupUsers);
int32_t UserApiPlatRequestRichPresence(UserApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pLookupUser);
int32_t UserApiPlatRequestRecentlyMet(UserApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pPlayerMet, void *pAdditionalInfo);
int32_t UserApiPlatRequestPostRichPresence(UserApiRefT *pRef, uint32_t uUserIndex, UserApiRichPresenceT *pData);

int32_t _UserApiPlatAbortPostRequests(UserApiRefT *pRef, uint32_t uUserIndex);

int32_t _UserApiProcessProfileResponse(UserApiRefT *pRef, int32_t uUserIndex, uint8_t bBatch, UserApiProfileT *ProfileData, UserApiUserDataT *pUserData);
int32_t _UserApiProcessPresenceResponse(UserApiRefT *pRef, int32_t uUserIndex, UserApiPresenceT *pPresenceData, UserApiUserDataT *pUserData);
int32_t _UserApiProcessRichPresenceResponse(UserApiRefT *pRef, int32_t uUserIndex, UserApiRichPresenceT *pRichPresenceData, UserApiUserDataT *pUserData);
int32_t _UserApiProcessRmpResponse(UserApiRefT *pRef, uint32_t uUserIndex);

void _UserApiTriggerCallback(UserApiRefT *pRef, uint32_t uUserIndex, UserApiEventErrorE eError, UserApiEventTypeE eType, UserApiUserDataT *pUserData);
void _UserApiTriggerPostCallback(UserApiRefT *pRef, uint32_t uUserIndex);

int32_t UserApiPlatRegisterUpdateEvent(UserApiRefT *pRef, uint32_t uUserIndex, UserApiNotifyTypeE eType);

//!< Control behavior of module
int32_t UserApiPlatControl(UserApiRefT *pRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

int32_t UserApiPlatUpdate(UserApiRefT *pRef);

#ifdef __cplusplus
}
#endif

#endif
