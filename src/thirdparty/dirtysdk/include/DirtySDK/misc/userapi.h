/*H*************************************************************************************************/
/*!
    \File    userapi.h

    \Description
        Expose first party player information

    \Notes
         This API currently only supports Gen 4 platforms (XBox One and PS4).

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2001-2013.    ALL RIGHTS RESERVED.

    \Version 05/12/13 (mcorcoran) First Version
    \Version 12/02/14 (amakoukji) API revamped
*/
/*************************************************************************************************H*/

#ifndef _userapi_h
#define _userapi_h

/*!
\Moduledef UserApi UserApi
\Modulemember User
*/
//@{

/*** Include files ********************************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtyuser.h"

#if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
#include "DirtySDK/misc/xboxone/userapixboxone.h"
#endif

/*** Defines **************************************************************************************/

//!< Maximum sizes of various strings
#define USERAPI_NOTIFY_LIST_MAX_SIZE  (50)
#define USERAPI_GAMERTAG_MAXLEN       (128)
#define USERAPI_AVATAR_URL_MAXLEN     (256)
#define USERAPI_DISPLAYNAME_MAXLEN    (256)
#define USERAPI_PLATFORM_NAME_MAXLEN  (64)
#define USERAPI_TITLE_NAME_MAXLEN     (128)
#define USERAPI_TITLE_ID_MAXLEN       (64)
#define USERAPI_RICH_PRES_MAXLEN      (512)
#define USERAPI_GAME_DATA_MAXLEN      (172)

#define USERAPI_MASK_PROFILE          (0x01)
#define USERAPI_MASK_PRESENCE         (0x02)
#define USERAPI_MASK_RICH_PRESENCE    (0x04)
#define USERAPI_MASK_PROFILES         (0x08)

/*** Macros ***************************************************************************************/

/*** Definitions *****************************************************************************/

//!< UserApi Error types
typedef enum UserApiEventErrorE
{
    USERAPI_ERROR_OK = 0,                       // Success code

    USERAPI_ERROR_REQUEST_FAILED = -100,        // Failed to retrieve id
    USERAPI_ERROR_REQUEST_NOT_FOUND,            // User wasn't found
    USERAPI_ERROR_REQUEST_FORBIDDEN,            // local user does not have access to requested user information
    USERAPI_ERROR_REQUEST_SERVER_ERROR,         // Internal server error
    USERAPI_ERROR_REQUEST_UNAVAILABLE,          // Service unavailable
    USERAPI_ERROR_REQUEST_TIMEOUT,              // Profile response was not received on time
    USERAPI_ERROR_UNSUPPORTED,
    USERAPI_ERROR_INPROGRESS,
    USERAPI_ERROR_REQUEST_PARSE_FAILED,
    USERAPI_ERROR_NO_USER,
    USERAPI_ERROR_FULL
} UserApiEventErrorE;

//!< Type of first party notifications
// Note that minimal information is returned by the notifications themselves and a follow up query to the first party will often be necessary to update the internal state
typedef enum UserApiNotifyTypeE
{
    USERAPI_NOTIFY_PRESENCE_UPDATE = 0,
    USERAPI_NOTIFY_TITLE_UPDATE,
    USERAPI_NOTIFY_RICH_PRESENCE_UPDATE,
    USERAPI_NOTIFY_PROFILE_UPDATE
} UserApiNotifyTypeE;

//!< Categories of data for UserApi callbacks
typedef enum UserApiEventTypeE
{
    USERAPI_EVENT_DATA = 0,      // The current event has user data available
    USERAPI_EVENT_END_OF_LIST    // The current event has completion data available through EventDetails.EndOfList
} UserApiEventTypeE;

//!< List of possible online statuses
typedef enum UserApiOnlineStatusE
{
    USERAPI_PRESENCE_UNKNOWN = 0,
    USERAPI_PRESENCE_OFFLINE,
    USERAPI_PRESENCE_ONLINE,
    USERAPI_PRESENCE_AWAY
} UserApiOnlineStatusE;

//!< Rich presence response data structure
typedef struct UserApiRichPresenceT
{
    char  strData[USERAPI_RICH_PRES_MAXLEN + 1];     // Rich presence string
    char  strGameData[USERAPI_GAME_DATA_MAXLEN + 1]; // GameData Base64 string
    void *pRawData;
} UserApiRichPresenceT;

//!< Presence data response structure
typedef struct UserApiPresenceT
{
    UserApiOnlineStatusE ePresenceStatus;               // online, offline, away or unknown
    char  strPlatform[USERAPI_PLATFORM_NAME_MAXLEN+1];  // name of the console the user is on
    char  strTitleName[USERAPI_TITLE_NAME_MAXLEN+1];    // title name
    char  strTitleId[USERAPI_TITLE_ID_MAXLEN+1];        // title identifier
    uint8_t bIsPlayingSameTitle;
    void *pRawData;
} UserApiPresenceT;

//!< Profile data response structure
typedef struct UserApiProfileT
{
    char        strDisplayName[USERAPI_DISPLAYNAME_MAXLEN + 1];       // The users real name. This string will contain strGamerTag if the real name is not available
    char        strGamertag   [USERAPI_GAMERTAG_MAXLEN    + 1];       // The users gamertag for XBox One or onlineId for PS4
    char        strAvatarUrl  [USERAPI_AVATAR_URL_MAXLEN  + 1];       // URI pointing to the avatar image for XBox One and PS4
    uint32_t    uLocale;                                              // The users location and language encoded using the LOBBYAPI macros in dirtylang.h
    #if defined(DIRTYCODE_XBOXONE) && !defined(DIRTYCODE_GDK)
    UserApiAccessibilityT Accessibility;                              // Contains the accessibility settings
    #endif
    void       *pRawData;                                             // pointer to object that contain all first part data received
} UserApiProfileT;

//!< General response structure
typedef struct UserApiUserDataT
{
    DirtyUserT            DirtyUser;     // A DirtySDK representation or the native user on the current platform
    uint32_t              uUserDataMask; // A mask value to determine which values below are valid
    UserApiProfileT       Profile;      // Valid when uUserDataMask & USERAPI_MASK_PROFILE > 0
    UserApiPresenceT      Presence;     // Valid when uUserDataMask & USERAPI_MASK_PRESENCE > 0
    UserApiRichPresenceT  RichPresence; // Valid when uUserDataMask & USERAPI_MASK_RICH_PRESENCE > 0
} UserApiUserDataT;

//!< Request metadata response structure
typedef struct UserApiEndOfListT
{
    int32_t iTotalRequested;
    int32_t iTotalReceived;
    int32_t iTotalErrors;
} UserApiEndOfListT;

//!< Container union for responses
typedef union UserApiEventDetailsT
{
    UserApiUserDataT      UserData;      // Valid when eEventType == USERAPI_EVENT_DATA
    UserApiEndOfListT     EndOfList;     // Valid when eEventType == USERAPI_EVENT_END_OF_LIST
} UserApiEventDetailsT;

//!< User metadata fpr USERAPI_EVENT_DATA
typedef struct UserApiEventDataT
{
    UserApiEventErrorE   eError;       // Error code associated with this event
    UserApiEventTypeE    eEventType;   // Event type used to specify the type of data in EventDetails
    uint32_t             uUserIndex;   // Index of the user that made the request
    UserApiEventDetailsT EventDetails; // Contains event details specific for the eEventType event type
} UserApiEventDataT;

//!< Response from data post style request
typedef struct UserApiPostResponseT
{
    uint32_t            uUserIndex; // Index of the user that made the request
    UserApiEventErrorE  eError;     // HTTP Error code associated with this event
    const char         *pMessage;   // Message if applicable
} UserApiPostResponseT;

//!< opaque module ref
typedef struct UserApiRefT UserApiRefT;

//!< Title change notification data
typedef struct UserApiNotifyTitleDataT
{
    DirtyUserT DirtyUser;  // user who's presence was updated
    uint32_t   uTitleId;   // title that was updated
    uint32_t   uUserIndex; // user the notification is for
} UserApiNotifyTitleDataT;

//!< Presence change notification data, note that this remains incomplete since minimal data is provided by first parties
typedef struct UserApiNotifyPresenceDataT
{
    DirtyUserT DirtyUser;  // user who's presence was updated
    uint32_t   uUserIndex; // user the notification is for
} UserApiNotifyPresenceDataT;

//!< Rich presence change notification data, note that this remains incomplete since minimal data is provided by first parties
typedef struct UserApiNotifyRichPresenceDataT
{
    DirtyUserT DirtyUser;  // user who's presence was updated
    uint32_t   uUserIndex; // user the notification is for
} UserApiNotifyRichPresenceDataT;

//!< Rich presence change notification data, note that this remains incomplete since minimal data is provided by first parties
typedef struct UserApiNotifyProfileUpdateDataT
{
    DirtyUserT DirtyUser;  // user who's presence was updated
    uint32_t   uUserIndex; // user the notification is for
} UserApiNotifyProfileUpdateDataT;

//!< container union for notification data
typedef union UserApiNotifyDataT
{
    UserApiNotifyTitleDataT         TitleData;           // Valid when eNotifyType == USERAPI_NOTIFY_TITLE_UPDATE
    UserApiNotifyPresenceDataT      PresenceData;        // Valid when eNotifyType == USERAPI_NOTIFY_PRESENCE
    UserApiNotifyRichPresenceDataT  RichPresenceData;    // Valid when eNotifyType == USERAPI_NOTIFY_RICH_PRESENCE
    UserApiNotifyProfileUpdateDataT ProfileUpdateData;   // Valid When eNotifyType == USERAPI_NOTIFY_PROFILE_UPDATE
} UserApiNotifyDataT;

//!< callback types
typedef void (UserApiCallbackT)      (UserApiRefT *pRef, UserApiEventDataT *pUserApiEventData, void *pUserData);
typedef void (UserApiPostCallbackT)  (UserApiRefT *pRef, UserApiPostResponseT *pResponse, void *pUserData);
typedef void (UserApiUpdateCallbackT)(UserApiRefT *pRef, UserApiNotifyTypeE eNotifyType, UserApiNotifyDataT *pData, void *pUserData);

/*** Function Prototypes **************************************************************************/

/*** Variables ************************************************************************************/

/*** Private Functions ****************************************************************************/

/*** Public Functions *****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// Starts UserApi.
DIRTYCODE_API UserApiRefT *UserApiCreate(void);

// Starts shutting down module. Module will not accept new requests, and will abort ongoing ones. Also, registered UserApiCallbackT callback will not be called even if there is data available for processing
DIRTYCODE_API int32_t UserApiDestroy(UserApiRefT *pRef);

// Get status information.
DIRTYCODE_API int32_t UserApiStatus(UserApiRefT *pRef, int32_t iSelect, void *pBuf, int32_t iBufSize);

// Control behavior of module
DIRTYCODE_API int32_t UserApiControl(UserApiRefT *pRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// Starts the process to retrieve a batch of user information for players. pLookupUsers is a pointer to the first DirtyUserT, and iLookupUsersLength is the number of DirtyUserTs.
DIRTYCODE_API int32_t UserApiRequestProfilesAsync(UserApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pLookupUsers, int32_t iLookupUsersLength, UserApiCallbackT *pCallback, void *pUserData);

// Starts the process to retrieve user information of a single user. pLookupUsers is a pointer to the DirtyUserT
DIRTYCODE_API int32_t UserApiRequestProfileAsync(UserApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pLookupUser, UserApiCallbackT *pCallback, uint32_t uUserDataMask, void *pUserData);

// Starts the process to retrieve Presence information of a single user. pLookupUsers is a pointer to the DirtyUserT
DIRTYCODE_API int32_t UserApiRequestPresenceAsync(UserApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pLookupUser, UserApiCallbackT *pCallback, uint32_t uUserDataMask, void *pUserData);

// Starts the process to retrieve Rich Presence information of a single user. pLookupUsers is a pointer to the DirtyUserT
DIRTYCODE_API int32_t UserApiRequestRichPresenceAsync(UserApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pLookupUser, UserApiCallbackT *pCallback, uint32_t uUserDataMask, void *pUserData);

// Log that a local user recently interacted with another
DIRTYCODE_API int32_t UserApiPostRecentlyMetAsync(UserApiRefT *pRef, uint32_t uUserIndex, DirtyUserT *pPlayerMet, void *pAdditionalInfo, UserApiPostCallbackT *pCallback, void *pUserData);

// Log new rich presence info; usage of this is very dependant on the first party's method of rich presence
DIRTYCODE_API int32_t UserApiPostRichPresenceAsync(UserApiRefT *pRef, uint32_t uUserIndex, UserApiRichPresenceT *pData, UserApiPostCallbackT *pCallback, void *pUserData);

// Register for callbacks
DIRTYCODE_API int32_t UserApiRegisterUpdateEvent(UserApiRefT *pRef, uint32_t uUserIndex, UserApiNotifyTypeE eType, UserApiUpdateCallbackT *pNotifyCb, void *pUserData);

// Update the internal state of the module, and call registered UserApiCallbackT callback if there are GamerCard/Profile responses available. This function should be called periodically.
DIRTYCODE_API void UserApiUpdate(UserApiRefT *pRef);

// cancels all pending transactions
DIRTYCODE_API int32_t UserApiCancel(UserApiRefT *pRef, uint32_t uUserIndex);

#ifdef __cplusplus
}
#endif

//@}

#endif // _userapi_h
