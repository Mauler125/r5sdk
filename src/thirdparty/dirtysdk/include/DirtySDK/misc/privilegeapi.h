/*H*************************************************************************************/
/*!

    \File    privilegeapi.h

    \Description
        Check first party privileges and parental controls

    \Copyright
        Copyright (c) Electronic Arts 2013.

    \Version 09/02/13 (mcorcoran) First Version
*/
/*************************************************************************************H*/

#ifndef _privilegeapi_h
#define _privilegeapi_h

/*!
\Moduledef PrivilegeApi PrivilegeApi
\Modulemember User
*/
//@{


/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct PrivilegeApiRefT PrivilegeApiRefT;

/*** Variables *************************************************************************/

typedef enum PrivilegeApiPrivE
{
    PRIVILEGEAPI_PRIV_INVALID                          = (0),
    PRIVILEGEAPI_PRIV_COMMUNICATION_VOICE_INGAME       = (-1),
    PRIVILEGEAPI_PRIV_COMMUNICATION_VOICE_SKYPE        = (-2),
    PRIVILEGEAPI_PRIV_VIDEO_COMMUNICATIONS             = (-3),
    PRIVILEGEAPI_PRIV_COMMUNICATIONS                   = (-4),
    PRIVILEGEAPI_PRIV_USER_CREATED_CONTENT             = (-5),
    PRIVILEGEAPI_PRIV_MULTIPLAYER_SESSIONS_REALTIME    = (-6),
    PRIVILEGEAPI_PRIV_DOWNLOAD_FREE_CONTENT            = (-7),
    PRIVILEGEAPI_PRIV_FITNESS_UPLOAD                   = (-8),
    PRIVILEGEAPI_PRIV_VIEW_FRIENDS_LIST                = (-9),
    PRIVILEGEAPI_PRIV_SHARE_KINECT_CONTENT             = (-10),
    PRIVILEGEAPI_PRIV_MULTIPLAYER_PARTIES              = (-11),
    PRIVILEGEAPI_PRIV_CLOUD_GAMING_MANAGE_SESSION      = (-12),
    PRIVILEGEAPI_PRIV_CLOUD_GAMING_JOIN_SESSION        = (-13),
    PRIVILEGEAPI_PRIV_CLOUD_SAVED_GAMES                = (-14),
    PRIVILEGEAPI_PRIV_PREMIUM_CONTENT                  = (-15),
    PRIVILEGEAPI_PRIV_INTERNET_BROWSER                 = (-16),
    PRIVILEGEAPI_PRIV_SUBSCRIPTION_CONTENT             = (-17),
    PRIVILEGEAPI_PRIV_PREMIUM_VIDEO                    = (-18),
    PRIVILEGEAPI_PRIV_GAME_DVR                         = (-19),
    PRIVILEGEAPI_PRIV_SOCIAL_NETWORK_SHARING           = (-20),
    PRIVILEGEAPI_PRIV_PURCHASE_CONTENT                 = (-21),
    PRIVILEGEAPI_PRIV_PROFILE_VIEWING                  = (-22),
    PRIVILEGEAPI_PRIV_PRESENCE                         = (-23),
    PRIVILEGEAPI_PRIV_CONTENT_AUTHOR                   = (-24),
    PRIVILEGEAPI_PRIV_UNSAFE_PROGRAMMING               = (-25),
    PRIVILEGEAPI_PRIV_MULTIPLAYER_SESSIONS_ASYNC       = (-26),
    PRIVILEGEAPI_PRIV_ONLINE_ACCESS                    = (-27),
    PRIVILEGEAPI_PRIV_EA_ACCESS                        = (256)
} PrivilegeApiPrivE;

#define PRIVILEGEAPI_STATUS_NONE                       (0x0000)
#define PRIVILEGEAPI_STATUS_IN_PROGRESS                (0x0001)
#define PRIVILEGEAPI_STATUS_GRANTED                    (0x0002)
#define PRIVILEGEAPI_STATUS_GRANTED_FRIENDS_ONLY       (0x0004)
#define PRIVILEGEAPI_STATUS_RESTRICTED                 (0x0008)
#define PRIVILEGEAPI_STATUS_BANNED                     (0x0010)
#define PRIVILEGEAPI_STATUS_PURCHASE_REQUIRED          (0x0020)
#define PRIVILEGEAPI_STATUS_ABORTED                    (0x0040)
#define PRIVILEGEAPI_STATUS_ERROR                      (0x0080)
#define PRIVILEGEAPI_STATUS_ADDITIONAL_CHECKS_REQUIRED (0x0100)
#define PRIVILEGEAPI_STATUS_UNDERAGE                   (0x0200)

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// creates the module
DIRTYCODE_API PrivilegeApiRefT *PrivilegeApiCreate(void);

// destroys the module
DIRTYCODE_API int32_t PrivilegeApiDestroy(PrivilegeApiRefT *pRef);

// checks for the existance of 1 or more privileges for a given user
DIRTYCODE_API int32_t PrivilegeApiCheckPrivilegesAsync(PrivilegeApiRefT *pRef, int32_t iUserIndex, const PrivilegeApiPrivE *pPrivileges, int32_t iPrivilegeCount, int32_t *pHint);

// checks for the existance of 1 or more privileges for a given user, and show the user a system provided UI in the even tthe user does not have the requested privileges
DIRTYCODE_API int32_t PrivilegeApiCheckPrivilegesAsyncWithUi(PrivilegeApiRefT *pRef, int32_t iUserIndex, const PrivilegeApiPrivE *pPrivileges, int32_t iPrivilegeCount, int32_t *pHint, const char *pUiMessage);

// checks/polls for the result of a given request id returned from the PrivilegeApiCheckPrivilegesAsync() functions
DIRTYCODE_API int32_t PrivilegeApiCheckResult(PrivilegeApiRefT *pRef, int32_t iRequestId);

// frees up a request id once you are finished with it
DIRTYCODE_API int32_t PrivilegeApiReleaseRequest(PrivilegeApiRefT *pRef, int32_t iRequestId);

// aborts a currently executing async request
DIRTYCODE_API int32_t PrivilegeApiAbort(PrivilegeApiRefT *pRef, int32_t iRequestId);

#ifdef __cplusplus
}
#endif

//@}

#endif
