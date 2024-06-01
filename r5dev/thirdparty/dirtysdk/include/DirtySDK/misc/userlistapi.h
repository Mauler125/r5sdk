/*H*************************************************************************************************/
/*!
    \File    userlistapi.h

    \Description
        The UserListApi module allow the user to fetche friends, blocked users and
        muted users from first parties.

    \Notes
         This API currently only supports Gen 4 platforms (XBox One and PS4).

    \Copyright
        Copyright (c) Electronic Arts 2013.

    \Version 04/17/13 (amakoukji)
    \Version 12/02/14 (amakoukji) API revamped
*/
/*************************************************************************************************H*/

#ifndef _userlistapi_h
#define _userlistapi_h

/*!
\Moduledef UserList UserList
\Modulemember User
*/
//@{

/*** Include files ********************************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtyuser.h"
#include "DirtySDK/misc/userapi.h"

#if defined (DIRTYCODE_XBOXONE) || defined (DIRTYCODE_GDK)
#include "DirtySDK/misc/xboxone/userlistapixboxone.h"
#elif defined (DIRTYCODE_PS5)
#include "DirtySDK/misc/ps5/userlistapips5.h"
#elif defined (DIRTYCODE_PS4)
#include "DirtySDK/misc/ps4/userlistapips4.h"
#endif

/*** Defines **************************************************************************************/
  
#define USERLISTAPI_ERROR_UNSUPPORTED       (-1)
#define USERLISTAPI_ERROR_INPROGRESS        (-2)
#define USERLISTAPI_ERROR_BAD_PARAM         (-3)
#define USERLISTAPI_ERROR_NO_USER           (-4)
#define USERLISTAPI_ERROR_TIMEOUT           (-5)
#define USERLISTAPI_ERROR_GUEST             (-6)
#define USERLISTAPI_ERROR_INVALID_USER      (-7)
#define USERLISTAPI_ERROR_USER_INDEX_RANGE  (-8)
#define USERLISTAPI_ERROR_RATE_LIMITED      (-9)
#define USERLISTAPI_ERROR_UNKNOWN           (-99)

#define USERLISTAPI_MASK_PROFILE          (0x01)    
#define USERLISTAPI_MASK_PRESENCE         (0x02)  
#define USERLISTAPI_MASK_RICH_PRESENCE    (0x04) 

#define USERLISTAPI_NOTIFY_LIST_MAX_SIZE  (50)
  
typedef enum UserListApiTypeE
{
    USERLISTAPI_TYPE_FRIENDS,
    USERLISTAPI_TYPE_BLOCKED,
    USERLISTAPI_TYPE_FRIENDS_OF_FRIEND,
    USERLISTAPI_TYPE_MUTED
} UserListApiTypeE;

typedef enum UserListApiIfATypeE
{
    USERLISTAPI_IS_FRIENDS,
    USERLISTAPI_IS_BLOCKED
} UserListApiIfATypeE;

typedef enum UserListApiReturnTypeE
{
    TYPE_USER_DATA,
    TYPE_LIST_END
} UserListApiReturnTypeE;

typedef enum UserListApiIsAReturnTypeE
{
    USERLISTAPI_IS_OF_TYPE,
    USERLISTAPI_IS_NOT_OF_TYPE,
    USERLISTAPI_IS_MYSELF,
    USERLISTAPI_IS_TYPE_UNKNOWN
} UserListApiIsAReturnTypeE;

typedef enum UserListApiNotifyTypeE
{
    USERLISTAPI_NOTIFY_FRIENDLIST_UPDATE = 0,
    USERLISTAPI_NOTIFY_BLOCKEDLIST_UPDATE
} 
UserListApiNotifyTypeE;

typedef enum UserListApiUpdateTypeE 
{
    USERLISTAPI_UPDATE_TYPE_UNKNOWN = 0,
    USERLISTAPI_UPDATE_TYPE_REMOVED,
    USERLISTAPI_UPDATE_TYPE_ADD
} UserListApiUpdateTypeE;
 
/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

//!< opaque module ref
typedef struct UserListApiRefT UserListApiRefT;

//!< search filters ref, this will be further defined in platforms specific headers
typedef struct UserListApiFiltersT UserListApiFiltersT;

//!< response structure for user data
typedef struct UserListApiUserDataT
{
    DirtyUserT             DirtyUser;                                    // user identifier
    uint8_t                bIsMutualFriend;                              // TRUE if the user is a mutual friend, FALSE otherwise
    uint8_t                bIsFavorite;                                  // TRUE if the user is a favorite friend, FALSE otherwise
    int32_t                iUserProfileError;                            // >= 0 means the profile is valid, otherwise a meaningful error code
    UserApiUserDataT       ExtendedUserData;                             // user details containing profile, presence and rich presence
    UserListApiTypeE       eRequestType;                                 // type of user lists, friends, recently met, etc 
    uint32_t               uUserIndex;                                   // index of local user index
    UserListApiFiltersT   *pUserListFilters;                             // filters used for this query, first Party Specific, only valid inside the UserListApiCallbackT
    void                  *pRawData;                                     // pointer to 1st party friend data
} UserListApiUserDataT;

//!< response structure for metadata
typedef struct UserListApiEndDataT
{
    UserListApiTypeE       eRequestType;      // type of user lists, friends, recently met, etc 
    UserListApiFiltersT   *pUserListFilters;  // filters used for this query, first Party Specific, only valid inside the UserListApiCallbackT
    uint32_t               uUserIndex;        // index of local user index
    uint32_t               uTypeMask;         // type mask
    int64_t                iLimit;            // maximum number of users to return
    int64_t                iOffset;           // index of first user from 1st party query
    int64_t                iTotalFriendCount; // total number of users that match the query, will become valid
    int32_t                Error;             // error code returned by the function
    int32_t                iRateLimRetrySecs; // if Error is USERLISTAPI_ERROR_RATE_LIMITED then this is the number of seconds to wait before retrying
} UserListApiEndDataT;

//!< IsA style request response
typedef struct UserListApiIsADataT
{
    DirtyUserT                       DirtyUser;         // user identifier 
    uint32_t                         uUserIndex;        // index of local user index
    UserListApiIsAReturnTypeE        eIsaType;          // result (yes, no, unknown or myself)
    uint8_t                          bIsMutualFriend;   // TRUE if the user is a mutual friend, FALSE otherwise
    uint8_t                          bIsFavorite;       // TRUE if the user is a favorite friend, FALSE otherwise
    int32_t                          Error;             // error code returned by the function
    int32_t                          iRateLimRetrySecs; // if Error is USERLISTAPI_ERROR_RATE_LIMITED then this is the number of seconds to wait before retrying
} UserListApiIsADataT;

//!< container union for generic response
typedef union UserListApiEventDataT
{
    struct UserListApiUserDataT     UserData;          // user data for eResponseType = TYPE_USER_DATA
    struct UserListApiEndDataT      ListEndData;       // meta data for eResponseType = TYPE_LIST_END
} UserListApiEventDataT;

//!< presence update notification data
typedef struct UserListApiNotifyPresenceUpdateData
{
  DirtyUserT DirtyUser;                             // user identifier 
} UserListApiNotifyPresenceUpdateData;

//!< rich presence update notification data
typedef struct UserListApiNotifyRichPresenceUpdateData 
{
  DirtyUserT DirtyUser;                             // user identifier 
} UserListApiNotifyRichPresenceUpdateData;

//!< friend list update notification data
typedef struct UserListApiNotifyFriendListUpdate
{
  DirtyUserT DirtyUser;                             // user identifier 
  UserListApiUpdateTypeE eType;
  uint32_t uUserIndex;  // user index for whom this notification belongs
} UserListApiNotifyFriendListUpdate;

//!< blocked list update notification data
typedef struct UserListApiNotifyBlockedListUpdate
{
  DirtyUserT DirtyUser;                             // user identifier 
  UserListApiUpdateTypeE eType;
  uint32_t uUserIndex;  // user index for whom this notification belongs
} UserListApiNotifyBlockedListUpdate;


//!< container union for notification results
typedef union UserListApiNotifyDataT
{
    UserListApiNotifyFriendListUpdate       FriendListData;
    UserListApiNotifyBlockedListUpdate      BlockedListData;
} UserListApiNotifyDataT;

//<! list query callback
typedef void (UserListApiCallbackT)(UserListApiRefT *pRef, UserListApiReturnTypeE eResponseType, UserListApiEventDataT *pUserApiEventData, void *pUserData);

//<! "is a" query callback
typedef void (UserListApiIsAQueryCallbackT)(UserListApiRefT *pRef, UserListApiIfATypeE eResponseType, UserListApiIsADataT *pUserApiEventData, void *pUserData);

//<! notification callback
typedef void (UserListApiUpdateCallbackT)(UserListApiRefT *pRef, UserListApiNotifyTypeE eType, UserListApiNotifyDataT *pData, void *pUserData);

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

//!< create the user list api object
DIRTYCODE_API UserListApiRefT *UserListApiCreate(uint32_t uMaxResponseSize);

//!< set the callback used for registering userlist update event
DIRTYCODE_API int32_t UserListApiRegisterUpdateEvent(UserListApiRefT *pRef, uint32_t uUserIndex, UserListApiNotifyTypeE eType, UserListApiUpdateCallbackT *pNotifyCb, void *pUserData);

//!< give module time to perform periodic processing
DIRTYCODE_API void UserListApiUpdate(UserListApiRefT *pRef);

//!< get status information
DIRTYCODE_API int32_t UserListApiStatus(UserListApiRefT *pRef, int32_t iSelect, void *pBuf, int32_t iBufSize);

//!< control function
DIRTYCODE_API int32_t UserListApiControl(UserListApiRefT *pRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

//!< destroy the user list api object
DIRTYCODE_API void UserListApiDestroy(UserListApiRefT *pRef);

//!< gets the list of users of the specified type
DIRTYCODE_API int32_t UserListApiGetListAsync(UserListApiRefT *pRef, uint32_t uUserIndex, UserListApiTypeE eType, int32_t iLimit, int32_t iOffset, UserListApiFiltersT *filter, UserListApiCallbackT *pNotifyCb, uint32_t uTypeMask, void *pUserData);

//!< Is target user a friend
DIRTYCODE_API int32_t UserListApiIsFriendAsync(UserListApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pUser, UserListApiIsAQueryCallbackT *pNotifyCb, void *pUserData);

//!< Is target user blocked
DIRTYCODE_API int32_t UserListApiIsBlockedAsync(UserListApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pUser, UserListApiIsAQueryCallbackT *pNotifyCb, void *pUserData);

//!< cancels all pending transactions
DIRTYCODE_API void UserListApiCancelAll(UserListApiRefT *pRef);

#ifdef __cplusplus
}
#endif
  
//@}

#endif // _userlistapi_h

