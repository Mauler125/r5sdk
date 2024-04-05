/*H*************************************************************************************/
/*!
    \File    user.c

    \Description
        Reference application for the userapi.

    \Copyright
        Copyright (c) Electronic Arts 2014.    ALL RIGHTS RESERVED.

    \Version 18/02/2014 (amakoukji)
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#include <string.h>
#include <stdlib.h>

#include "DirtySDK/platform.h"

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/misc/userapi.h"

#include "libsample/zlib.h"
#include "libsample/zfile.h"
#include "libsample/zmem.h"
#include "testersubcmd.h"
#include "testermodules.h"

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct UaAppT
{
    UserApiRefT *pUa;

    uint8_t bStarted;

    unsigned char bZCallback;
} UaAppT;

/*** Function Prototypes ***************************************************************/

static void _UaCreate(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _UaControl(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _UaDestroy(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _UaGetProfiles(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _UaGetProfile(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _UaRegister(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _UaRecentlyMet(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _UaSetRichPresence(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static int32_t _CmdUaTickCb(ZContext *argz, int32_t argc, char *argv[]);
static void _CmdUaCb(UserApiRefT *pRef, UserApiEventDataT *pUserApiEventData, void *pUserData);
static void _CmdUaUpdateCb(UserApiRefT *pRef, UserApiNotifyTypeE eNotifyType, UserApiNotifyDataT *pData, void *pUserData);
static void _CmdUaPostCb(UserApiRefT *pRef, UserApiPostResponseT *pResponse, void *pUserData);

/*** Variables *************************************************************************/

// Private variables
static T2SubCmdT _Ua_Commands[] =
{
    { "create",              _UaCreate           },
    { "control",             _UaControl          },
    { "destroy",             _UaDestroy          },
    { "getprofiles",         _UaGetProfiles      },
    { "getprofile",          _UaGetProfile       },
    { "register",            _UaRegister         },
    { "recentlymet",         _UaRecentlyMet      },
    { "setrichpresence",     _UaSetRichPresence  },
    { "",                    NULL                }
};

static UaAppT _Ua_App;

// Public variables

/*** Private Functions *****************************************************************/

/*F********************************************************************************/
/*!
    \Function _CmdUaTickCb

    \Description
        UserList callback, called after command has been issued.

    \Input *argz    - pointer to context
    \Input argc     - number of command-line arguments
    \Input *argv[]   - command-line argument list

    \Output
        int32_t      - result of zcallback, or zero to terminate

    \Version 18/02/2014 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _CmdUaTickCb(ZContext *argz, int32_t argc, char *argv[])
{
    UaAppT *pApp = &_Ua_App;

    if (pApp->bStarted != FALSE)
    {
        // check for kill
        if (argc == 0)
        {
            ZPrintf("%s: killed\n", argv[0]);
            UserApiDestroy(pApp->pUa);
            return(0);
        }

        // update module
        if (pApp->pUa != NULL)
        {
            UserApiUpdate(pApp->pUa);
        }

        // keep recurring
        return(ZCallback(&_CmdUaTickCb, 17));
    }

    return(ZLIB_STATUS_UNKNOWN);
}

/*F********************************************************************************/
/*!
    \Function _CmdUaCb

    \Description
        user callback

    \Input *pRef              - UserListApiRefT reference
    \Input *pUserApiEventData - result data struct
    \Input  pUserData         - NULL


    \Version 18/02/2014 (amakoukji)
*/
/********************************************************************************F*/
static void _CmdUaCb(UserApiRefT *pRef, UserApiEventDataT *pUserApiEventData, void *pUserData)
{
    uint64_t uUserId;

    if (pUserApiEventData->eEventType == USERAPI_EVENT_DATA)
    {
        DirtyUserToNativeUser(&uUserId, sizeof(uUserId), &(pUserApiEventData->EventDetails.UserData.DirtyUser));
        ZPrintf("user: profile data \n\t id %llu ", uUserId);

        if (pUserApiEventData->EventDetails.UserData.uUserDataMask & USERAPI_MASK_PROFILE)
        {
            ZPrintf("\n\twith gamertag %s and avatar at %s ", pUserApiEventData->EventDetails.UserData.Profile.strGamertag, pUserApiEventData->EventDetails.UserData.Profile.strAvatarUrl);
        }

        if (pUserApiEventData->EventDetails.UserData.uUserDataMask & USERAPI_MASK_PRESENCE)
        {
            ZPrintf("\n\t%s ", (pUserApiEventData->EventDetails.UserData.Presence.ePresenceStatus == USERAPI_PRESENCE_ONLINE) ? "is online" :
                           (pUserApiEventData->EventDetails.UserData.Presence.ePresenceStatus == USERAPI_PRESENCE_AWAY) ? "is away" : "is offline");
            if (pUserApiEventData->EventDetails.UserData.Presence.ePresenceStatus == USERAPI_PRESENCE_ONLINE)
            {
                ZPrintf("\n\tin title %s on plaform %s", pUserApiEventData->EventDetails.UserData.Presence.strTitleName, pUserApiEventData->EventDetails.UserData.Presence.strPlatform);
            }
        }

        if (pUserApiEventData->EventDetails.UserData.uUserDataMask & USERAPI_MASK_RICH_PRESENCE)
        {
            ZPrintf("\n\twith rich presence \"%s\"", pUserApiEventData->EventDetails.UserData.RichPresence.strData);
        }
        ZPrintf("\n");
    }
    else if (pUserApiEventData->eEventType == USERAPI_EVENT_END_OF_LIST)
    {
        ZPrintf("user: End of profiles report\n \tTotal Requested : %d\n\tTotal Received : %d\n\tTotalErrors : %d\n", pUserApiEventData->EventDetails.EndOfList.iTotalRequested,
                                                                                                                  pUserApiEventData->EventDetails.EndOfList.iTotalReceived,
                                                                                                                  pUserApiEventData->EventDetails.EndOfList.iTotalErrors);
    }
}

/*F********************************************************************************/
/*!
    \Function _CmdUaUpdateCb

    \Description
        user callback for 1st party notifications

    \Input *pRef              - UserListApiRefT reference
    \Input  eNotifyType       - type of callback
    \Input *pUserApiEventData - result data struct
    \Input  pUserData         - NULL


    \Version 18/02/2014 (amakoukji)
*/
/********************************************************************************F*/
static void _CmdUaUpdateCb(UserApiRefT *pRef, UserApiNotifyTypeE eNotifyType, UserApiNotifyDataT *pData, void *pUserData)
{
    uint64_t uUserId;

    if (eNotifyType == USERAPI_NOTIFY_PRESENCE_UPDATE)
    {
        DirtyUserToNativeUser(&uUserId, sizeof(uUserId), &(pData->PresenceData.DirtyUser));
        ZPrintf("user: presence update for id %llu\n", uUserId);
    }
    else if  (eNotifyType == USERAPI_NOTIFY_TITLE_UPDATE)
    {
        DirtyUserToNativeUser(&uUserId, sizeof(uUserId), &(pData->TitleData.DirtyUser));
        ZPrintf("user: title update for id %llu\n", uUserId);
    }
    else if  (eNotifyType == USERAPI_NOTIFY_RICH_PRESENCE_UPDATE)
    {
        DirtyUserToNativeUser(&uUserId, sizeof(uUserId), &(pData->RichPresenceData.DirtyUser));
        ZPrintf("user: rich presence update for id %llu\n", uUserId);
    }


}

/*F********************************************************************************/
/*!
    \Function _CmdUaPostCb

    \Description
        user callback for posted data

    \Input *pRef              - UserListApiRefT reference
    \Input *pResponse         - result data struct
    \Input  pUserData         - NULL


    \Version 18/02/2014 (amakoukji)
*/
/********************************************************************************F*/
static void _CmdUaPostCb(UserApiRefT *pRef, UserApiPostResponseT *pResponse, void *pUserData)
{
    ZPrintf("user: response from data push, error code=%d, message=\"%s\"\n", pResponse->eError, pResponse->pMessage);
}

/*F*************************************************************************************/
/*!
    \Function _UaCreate

    \Description
        Udp subcommand - create udp socket

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _UaCreate(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UaAppT *pApp = (UaAppT *)_pApp;

    if ((bHelp == TRUE) || (argc != 2 ))
    {
        ZPrintf("   usage: %s create\n", argv[0]);
        return;
    }

    if (pApp->bStarted)
    {
        ZPrintf("%s: user already created\n", argv[0]);
        return;
    }

    // allocate UserApi module
    if ((pApp->pUa = UserApiCreate()) == NULL)
    {
        ZPrintf("%s: unable to create protoudp module\n", argv[0]);
        return;
    }

    pApp->bStarted = TRUE;

    // one-time install of periodic callback
    if (pApp->bZCallback == FALSE)
    {
        pApp->bZCallback = TRUE;
        ZCallback(_CmdUaTickCb, 17);
    }
}

/*F*************************************************************************************/
/*!
    \Function _UaControl

    \Description
        Call UserApiControl

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _UaControl(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UaAppT  *pApp = (UaAppT *)_pApp;
    int32_t  iResult;
    int32_t  iControl = 0;
    int32_t  iValue = 0;
    int32_t  iValue2 = 0;

    if ((bHelp == TRUE) || (argc != 6)  || strlen(argv[2]) != 4)
    {
        ZPrintf("   usage: %s control [4 character selector] [iValue] [iValue2] [pValue]\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: user not yet created\n", argv[0]);
        return;
    }

    iControl +=  argv[2][0] << 24;
    iControl +=  argv[2][1] << 16;
    iControl +=  argv[2][2] << 8;
    iControl +=  argv[2][3];
    iValue  = strtol(argv[3], NULL, 10);
    iValue2 = strtol(argv[4], NULL, 10);
    iResult = UserApiControl(pApp->pUa, iControl, iValue, iValue2, argv[5]);
    ZPrintf("control result for %s\n", argv[2], iResult);
}

/*F*************************************************************************************/
/*!
    \Function _UaDestroy

    \Description
        Destroy UserApi

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _UaDestroy(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UaAppT *pApp = (UaAppT *)_pApp;

    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s destroy\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: user not yet created\n", argv[0]);
        return;
    }

    UserApiDestroy(pApp->pUa);
    pApp->bZCallback = FALSE;
    pApp->bStarted = FALSE;
}

/*F*************************************************************************************/
/*!
    \Function _UaGetProfiles

    \Description
        Batch profile request. Cannot get presence concurrently. Max 100

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _UaGetProfiles(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UaAppT  *pApp = (UaAppT *)_pApp;
    int32_t  iResult;
    uint32_t uUserIndex = 0;
    char    *pch;
    uint32_t uNumUsers = 0;
    DirtyUserT aDirtyUsers[100];

    if ((bHelp == TRUE) || (argc != 4))
    {
        ZPrintf("   usage: %s getprofiles [user index] [comma seperated user user ids]\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: user not yet created\n", argv[0]);
        return;
    }

    uUserIndex = (uint32_t)strtol(argv[2], NULL, 10);
    ds_memclr(aDirtyUsers, sizeof(aDirtyUsers));

    // parse list
    pch = strtok (argv[3],",");
    while (pch != NULL)
    {
        uint64_t uUserId = ds_strtoull(pch, NULL, 10);
        DirtyUserFromNativeUser(&aDirtyUsers[uNumUsers++], &uUserId);
        pch = strtok (NULL, ",");
    }

    iResult = UserApiRequestProfilesAsync(pApp->pUa, uUserIndex, aDirtyUsers, uNumUsers, &_CmdUaCb, NULL);
    if (iResult == USERAPI_ERROR_UNSUPPORTED)
    {
        ZPrintf("getprofiles is not supported on this platform\n", iResult);
    }
    else
    {
        ZPrintf("getprofiles: UserApiRequestProfilesAsync() returned %d\n", iResult);
    }
}

/*F*************************************************************************************/
/*!
    \Function _UaGetProfile

    \Description
        Profile request. Can get presence concurrently

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _UaGetProfile(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UaAppT  *pApp = (UaAppT *)_pApp;
    int32_t  iResult;
    uint32_t uUserIndex = 0;
    char    *pCh;
    uint32_t uUserDataMask = 0;
    DirtyUserT DirtyUser;
    uint64_t uUserId;
    int32_t iBase;

    if ((bHelp == TRUE) || (argc != 5))
    {
        ZPrintf("   usage: %s getprofile [user index] [user ids] [comma seperated flags in profile,presence,richpresence]\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: user not yet created\n", argv[0]);
        return;
    }

    uUserIndex = (uint32_t)strtol(argv[2], NULL, 10);

    pCh = argv[3];
    iBase = 10;
    if ((pCh[0] == '0') && (pCh[1] == 'x'))
    {
        pCh += 2;
        iBase = 16;
    }
    uUserId = ds_strtoull(pCh, NULL, iBase);
    DirtyUserFromNativeUser(&DirtyUser, &uUserId);

    // parse list
    pCh = strtok(argv[4], ",");
    while (pCh != NULL)
    {
        if (ds_stricmp(pCh, "profile") == 0)
        {
            uUserDataMask |= USERAPI_MASK_PROFILE;
        }
        else if (ds_stricmp(pCh, "presence") == 0)
        {
            uUserDataMask |= USERAPI_MASK_PRESENCE;
        }
        else if (ds_stricmp(pCh, "richpresence") == 0)
        {
            uUserDataMask |= USERAPI_MASK_RICH_PRESENCE;
        }

        pCh = strtok (NULL, ",");
    }

    iResult = UserApiRequestProfileAsync(pApp->pUa, uUserIndex, &DirtyUser, &_CmdUaCb, uUserDataMask, NULL);
    if (iResult == USERAPI_ERROR_UNSUPPORTED)
    {
        ZPrintf("getprofiles is not supported on this platform\n", iResult);
    }
    else
    {
        ZPrintf("getprofiles: UserListApiGetListAsync() returned %d\n", iResult);
    }
}

/*F*************************************************************************************/
/*!
    \Function _UaRegister

    \Description
        Register for 1st party notifications

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _UaRegister(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UaAppT  *pApp = (UaAppT *)_pApp;
    int32_t  iResult;
    uint32_t uUserIndex = 0;
    UserApiNotifyTypeE eType = USERAPI_NOTIFY_PRESENCE_UPDATE;

    if ((bHelp == TRUE) || (argc != 4))
    {
        ZPrintf("   usage: %s register [user index] [presence|title|richpresence]\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: userlist not yet created\n", argv[0]);
        return;
    }
    // get the receive buffer size (use 0=ANY if not specified)
    uUserIndex = (uint32_t)strtol(argv[2], NULL, 10);

    if (ds_stricmp(argv[3], "presence") == 0)
    {
        eType = USERAPI_NOTIFY_PRESENCE_UPDATE;
    }
    else if (ds_stricmp(argv[3], "title") == 0)
    {
        eType = USERAPI_NOTIFY_TITLE_UPDATE;
    }
    else if (ds_stricmp(argv[3], "richpresence") == 0)
    {
        eType = USERAPI_NOTIFY_RICH_PRESENCE_UPDATE;
    }
    else
    {
        ZPrintf("   usage: %s register [user index] [presence|title|richpresence]\n", argv[0]);
        return;
    }

    iResult = UserApiRegisterUpdateEvent(pApp->pUa, uUserIndex, eType, &_CmdUaUpdateCb, NULL);
    if (iResult == USERAPI_ERROR_UNSUPPORTED)
    {
        ZPrintf("%s is not supported on this platform\n", argv[0], iResult);
    }
    else
    {
        ZPrintf("%s: UserApiRegisterUpdateEvent() returned %d\n", argv[0], iResult);
    }
}

/*F*************************************************************************************/
/*!
    \Function _UaRecentlyMet

    \Description
        Submit a recently met player report

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _UaRecentlyMet(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
#if !defined(DIRTYCODE_XBOXONE) && !defined(DIRTYCODE_GDK)
    UaAppT  *pApp = (UaAppT *)_pApp;
    int32_t iResult = 0;
    uint32_t uUserIndex = 0;
    DirtyUserT DirtyUser;
    uint64_t AcountId;
    int32_t iBase = 0;
    char *pCh;

    if ((bHelp == TRUE) || (argc != 4))
    {
        ZPrintf("   usage: %s recentlymet [user index] [user online ids]\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: userlist not yet created\n", argv[0]);
        return;
    }

    // get the receive buffer size (use 0=ANY if not specified)
    uUserIndex = (uint32_t)strtol(argv[2], NULL, 10);

    pCh = argv[3];
    iBase = 10;
    if ((pCh[0] == '0') && (pCh[1] == 'x'))
    {
        pCh += 2;
        iBase = 16;
    }
    AcountId = ds_strtoll(pCh, NULL, iBase);
    DirtyUserFromNativeUser(&DirtyUser, &AcountId);

    iResult = UserApiPostRecentlyMetAsync(pApp->pUa, uUserIndex, &DirtyUser, NULL, &_CmdUaPostCb, NULL);
    if (iResult == USERAPI_ERROR_UNSUPPORTED)
    {
        ZPrintf("recentlymet is not supported on this platform\n", iResult);
    }
    else
    {
        ZPrintf("recentlymet: UserApiPostRecentlyMetAsync() returned %d\n", iResult);
    }
#else
    ZPrintf("   %s recentlymet is not supported on XBox\n", argv[0]);
#endif
}

/*F*************************************************************************************/
/*!
    \Function _UaSetRichPresence

    \Description
        Set Rich presence

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _UaSetRichPresence(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UaAppT  *pApp = (UaAppT *)_pApp;
    int32_t iResult = 0;
    uint32_t uUserIndex = 0;
    UserApiRichPresenceT Data;

    if ((bHelp == TRUE) || (argc != 4))
    {
        ZPrintf("   usage: %s setrichpresence [user index] [rich presence string]\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: userlist not yet created\n", argv[0]);
        return;
    }

    // get the receive buffer size (use 0=ANY if not specified)
    uUserIndex = (uint32_t)strtol(argv[2], NULL, 10);

    ds_memclr(&Data, sizeof(UserApiRichPresenceT));
    ds_strnzcpy(Data.strData, argv[3], (int32_t)sizeof(Data.strData));

    iResult = UserApiPostRichPresenceAsync(pApp->pUa, uUserIndex, &Data, &_CmdUaPostCb, NULL);
    if (iResult == USERAPI_ERROR_UNSUPPORTED)
    {
        ZPrintf("setrichpresence is not supported on this platform\n", iResult);
    }
    else
    {
        ZPrintf("recentlymet: UserApiPostRichPresenceAsync() returned %d\n", iResult);
    }
}


/*** Public Functions ******************************************************************/

/*F*************************************************************************************/
/*!
    \Function    CmdUdp

    \Description
        Udp command.

    \Input *argz    - unused
    \Input argc     - argument count
    \Input **argv   - argument list

    \Output
        int32_t         - zero

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
int32_t CmdUser(ZContext *argz, int32_t argc, char *argv[])
{
    T2SubCmdT *pCmd;
    UaAppT *pApp = &_Ua_App;
    unsigned char bHelp;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_Ua_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   test the user module\n");
        T2SubCmdUsage(argv[0], _Ua_Commands);
        return(0);
    }

    // if no ref yet, make one
    if ((pCmd->pFunc != _UaCreate) && (pApp->pUa == NULL))
    {
        char *pCreate = "create";
        ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
        _UaCreate(pApp, 1, &pCreate, bHelp);
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    return(0);
}
