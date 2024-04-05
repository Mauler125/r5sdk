/*H*************************************************************************************/
/*!
    \File    userlist.c

    \Description
        Reference application for the userlistapi.

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
#include "DirtySDK/misc/userlistapi.h"

#include "libsample/zlib.h"
#include "libsample/zfile.h"
#include "libsample/zmem.h"
#include "testersubcmd.h"
#include "testermodules.h"

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct UlaAppT
{
    UserListApiRefT *pUla;
    char aRecvBuf[2048];
    char aSendBuf[2048];

    uint8_t bStarted;

    unsigned char bZCallback;
} UlaAppT;

/*** Function Prototypes ***************************************************************/

static void _UlaCreate(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _UlaDestroy(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _UlaFriends(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _UlaBlocked(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _UlaRegister(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _UlaIsFriend(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _UlaIsBlocked(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _CmdUlaCb(UserListApiRefT *pRef, UserListApiReturnTypeE eResponseType, UserListApiEventDataT *pUserApiEventData, void *pUserData);
static void _CmdUlaIsACb(UserListApiRefT *pRef, UserListApiIfATypeE eResponseType, UserListApiIsADataT *pUserApiEventData, void *pUserData);
static void _CmdUlaUpdateCb(UserListApiRefT *pRef, UserListApiNotifyTypeE eType, UserListApiNotifyDataT *pData, void *pUserData);
static int32_t _CmdUlaTickCb(ZContext *argz, int32_t argc, char *argv[]);

/*** Variables *************************************************************************/

// Private variables
static T2SubCmdT _Ula_Commands[] =
{
    { "create",              _UlaCreate           },
    { "destroy",             _UlaDestroy          },
    { "getfriends",          _UlaFriends          },
    { "getblocked",          _UlaBlocked          },
    { "register",            _UlaRegister         },
    { "isfriend",            _UlaIsFriend         },
    { "isblocked",           _UlaIsBlocked        },
    { "",                    NULL                 }
};

static UlaAppT _Ula_App;

// Public variables

/*** Private Functions *****************************************************************/

/*F********************************************************************************/
/*!
    \Function _CmdUlaTickCb

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
static int32_t _CmdUlaTickCb(ZContext *argz, int32_t argc, char *argv[])
{
    UlaAppT *pApp = &_Ula_App;

    if (pApp->bStarted != FALSE)
    {       
        // check for kill
        if (argc == 0)
        {
            ZPrintf("%s: killed\n", argv[0]);
            UserListApiDestroy(pApp->pUla);
            return(0);
        }

        // update module
        if (pApp->pUla != NULL)
        {
            UserListApiUpdate(pApp->pUla);
        }
        
        // keep recurring
        return(ZCallback(&_CmdUlaTickCb, 17));
    }
    
    return(ZLIB_STATUS_UNKNOWN);
}

/*F********************************************************************************/
/*!
    \Function _CmdUlaCb

    \Description
        userlist callback

    \Input *pRef              - UserListApiRefT reference
    \Input  eResponseType     - type of query
    \Input *pUserApiEventData - result data struct
    \Input  pUserData         - NULL


    \Version 18/02/2014 (amakoukji)
*/
/********************************************************************************F*/
static void _CmdUlaCb(UserListApiRefT *pRef, UserListApiReturnTypeE eResponseType, UserListApiEventDataT *pUserApiEventData, void *pUserData)
{
    uint64_t uUserId;

    if (eResponseType == TYPE_USER_DATA)
    {
        DirtyUserToNativeUser(&uUserId, sizeof(uUserId), &pUserApiEventData->UserData.DirtyUser);

        if (pUserApiEventData->UserData.ExtendedUserData.uUserDataMask != 0)
        {
            ZPrintf("Friend: userid: %llu, gamertag %s\n", uUserId, pUserApiEventData->UserData.ExtendedUserData.Profile.strGamertag);
        }
        else
        {
            ZPrintf("Friend: userid: %llu\n", uUserId);
        }
    }
    else if (eResponseType == TYPE_LIST_END)
    {
        ZPrintf("Friend: END OF LIST\n");
    }

}

/*F********************************************************************************/
/*!
    \Function _CmdUlaIsACb

    \Description
        userlist callback

    \Input *pRef              - UserListApiRefT reference
    \Input  eResponseType     - type of query
    \Input *pUserApiEventData - result data struct
    \Input  pUserData         - NULL


    \Version 18/02/2014 (amakoukji)
*/
/********************************************************************************F*/
static void _CmdUlaIsACb(UserListApiRefT *pRef, UserListApiIfATypeE eResponseType, UserListApiIsADataT *pUserApiEventData, void *pUserData)
{
    ZPrintf("%s: %s %s\n", eResponseType == USERLISTAPI_IS_FRIENDS ? "IsFriend" : "IsBlocked",
                           pUserApiEventData->eIsaType == USERLISTAPI_IS_OF_TYPE ? "is a" : "is NOT a",
                           eResponseType == USERLISTAPI_IS_FRIENDS ? "friend" : "blocked user");
}

/*F********************************************************************************/
/*!
    \Function _CmdUlaUpdateCb

    \Description
        userlist callback

    \Input *pRef      - UserListApiRefT reference
    \Input  eType     - type of query
    \Input *pData     - result data struct
    \Input  pUserData - NULL


    \Version 18/02/2014 (amakoukji)
*/
/********************************************************************************F*/
static void _CmdUlaUpdateCb(UserListApiRefT *pRef, UserListApiNotifyTypeE eType, UserListApiNotifyDataT *pData, void *pUserData)
{
    if (eType == USERLISTAPI_NOTIFY_FRIENDLIST_UPDATE)
    {
        ZPrintf("userlist: Friend list update notification received!\n");
    }
    else if (eType == USERLISTAPI_NOTIFY_BLOCKEDLIST_UPDATE)
    {
        ZPrintf("userlist: Blocked list update notification received!\n");
    }
    else
    {
        ZPrintf("userlist: Unknown first party notification received!\n");
    }
}

/*F*************************************************************************************/
/*!
    \Function _UlaCreate

    \Description
        UserList create

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _UlaCreate(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UlaAppT *pApp = (UlaAppT *)_pApp;
    int32_t iReceiveBufferSize;

    if ((bHelp == TRUE) || (argc != 2 && argc != 3))
    {
        ZPrintf("   usage: %s create [receive buffer size]\n", argv[0]);
        return;
    }

    if (pApp->bStarted)
    {
        ZPrintf("%s: userlist already created, re-creating\n", argv[0]);
        UserListApiDestroy(pApp->pUla);
    }

    // get the receive buffer size (use 0=ANY if not specified)
    iReceiveBufferSize = (argc == 3) ? strtol(argv[2], NULL, 10) : 0;

    // allocate ProtoUdp module
    if ((pApp->pUla = UserListApiCreate(iReceiveBufferSize)) == NULL)
    {
        ZPrintf("%s: unable to create userlist module\n", argv[0]);
        return;
    }

    pApp->bStarted = TRUE;

    // one-time install of periodic callback
    if (pApp->bZCallback == FALSE)
    {
        pApp->bZCallback = TRUE;
        ZCallback(_CmdUlaTickCb, 17);
    }
}

/*F*************************************************************************************/
/*!
    \Function _UlaDestroy

    \Description
        UserList destroy

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _UlaDestroy(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UlaAppT *pApp = (UlaAppT *)_pApp;

    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s destroy\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: userlist not yet created\n", argv[0]);
        return;
    }

    UserListApiDestroy(pApp->pUla);
    pApp->bZCallback = FALSE;
    pApp->bStarted = FALSE;
}

/*F*************************************************************************************/
/*!
    \Function _UlaFriends

    \Description
        Fetch friends list

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _UlaFriends(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UlaAppT  *pApp = (UlaAppT *)_pApp;
    int32_t  iResult;
    uint32_t uUserIndex = 0;

    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s getfriends [user index]\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: userlist not yet created\n", argv[0]);
        return;
    }
    // get the receive buffer size (use 0=ANY if not specified)
    uUserIndex = (uint32_t)strtol(argv[2], NULL, 10);
    iResult = UserListApiGetListAsync(pApp->pUla, uUserIndex, USERLISTAPI_TYPE_FRIENDS, 1000, 0, NULL, &_CmdUlaCb, USERLISTAPI_MASK_PROFILE | USERLISTAPI_MASK_PRESENCE, NULL);
    if (iResult == USERLISTAPI_ERROR_UNSUPPORTED)
    {
        ZPrintf("%s is not supported on this platform\n", argv[0], iResult);
    }
    else
    {
        ZPrintf("%s: UserListApiGetListAsync() returned %d\n", argv[0], iResult);
    }
}

/*F*************************************************************************************/
/*!
    \Function _UlaBlocked

    \Description
        Fetch blocked list

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _UlaBlocked(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UlaAppT  *pApp = (UlaAppT *)_pApp;
    int32_t  iResult;
    uint32_t uUserIndex = 0;

    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s getblocked [user index]\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: userlist not yet created\n", argv[0]);
        return;
    }
    // get the receive buffer size (use 0=ANY if not specified)
    uUserIndex = (uint32_t)strtol(argv[2], NULL, 10);
    iResult = UserListApiGetListAsync(pApp->pUla, uUserIndex, USERLISTAPI_TYPE_BLOCKED, 50, 0, NULL, &_CmdUlaCb, USERLISTAPI_MASK_PROFILE, NULL);
    if (iResult == USERLISTAPI_ERROR_UNSUPPORTED)
    {
        ZPrintf("%s is not supported on this platform\n", argv[0], iResult);
    }
    else
    {
        ZPrintf("%s: UserListApiGetListAsync() returned %d\n", argv[0], iResult);
    }
}

/*F*************************************************************************************/
/*!
    \Function _UlaIsFriend

    \Description
        Check if a user is a friend

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _UlaIsFriend(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UlaAppT  *pApp = (UlaAppT *)_pApp;
    int32_t  iResult;
    uint32_t uUserIndex = 0;
    DirtyUserT User;
    uint64_t uUserId;

    if ((bHelp == TRUE) || (argc != 4))
    {
        ZPrintf("   usage: %s isfriend [user index] [userid]\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: userlist not yet created\n", argv[0]);
        return;
    }
    // get the receive buffer size (use 0=ANY if not specified)
    uUserIndex = (uint32_t)strtol(argv[2], NULL, 10);

    uUserId = ds_strtoull(argv[3], NULL, 10);
    DirtyUserFromNativeUser(&User, &uUserId);

    iResult = UserListApiIsFriendAsync(pApp->pUla, uUserIndex, &User, &_CmdUlaIsACb, NULL);
    if (iResult == USERLISTAPI_ERROR_UNSUPPORTED)
    {
        ZPrintf("%s is not supported on this platform\n", argv[0], iResult);
    }
    else
    {
        ZPrintf("%s: UserListApiIsFriendAsync() returned %d\n", argv[0], iResult);
    }
}

/*F*************************************************************************************/
/*!
    \Function _UlaIsBlocked

    \Description
        Check if a user is blocked

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _UlaIsBlocked(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UlaAppT  *pApp = (UlaAppT *)_pApp;
    int32_t  iResult;
    uint32_t uUserIndex = 0;
    DirtyUserT User;
    uint64_t uUserId;

    if ((bHelp == TRUE) || (argc != 4))
    {
        ZPrintf("   usage: %s isblocked [user index] [userid]\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: userlist not yet created\n", argv[0]);
        return;
    }
    // get the receive buffer size (use 0=ANY if not specified)
    uUserIndex = (uint32_t)strtol(argv[2], NULL, 10);

    uUserId = ds_strtoull(argv[3], NULL, 10);
    DirtyUserFromNativeUser(&User, &uUserId);

    iResult = UserListApiIsBlockedAsync(pApp->pUla, uUserIndex, &User, &_CmdUlaIsACb, NULL);
    if (iResult == USERLISTAPI_ERROR_UNSUPPORTED)
    {
        ZPrintf("%s is not supported on this platform\n", argv[0], iResult);
    }
    else
    {
        ZPrintf("%s: UserListApiIsBlockedAsync() returned %d\n", argv[0], iResult);
    }
}

/*F*************************************************************************************/
/*!
    \Function _UlaRegister

    \Description
        Check if a user is blocked

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _UlaRegister(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UlaAppT  *pApp = (UlaAppT *)_pApp;
    int32_t  iResult;
    uint32_t uUserIndex = 0;
    DirtyUserT User;
    uint8_t bTypeFriends = TRUE;
    uint64_t uUserId;

    if ((bHelp == TRUE) || (argc != 4))
    {
        ZPrintf("   usage: %s register [user index] [friend|blocked]\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: userlist not yet created\n", argv[0]);
        return;
    }
    // get the receive buffer size (use 0=ANY if not specified)
    uUserIndex = (uint32_t)strtol(argv[2], NULL, 10);

    uUserId = ds_strtoull(argv[3], NULL, 10);
    DirtyUserFromNativeUser(&User, &uUserId);

    if (ds_stricmp(argv[3], "friend") == 0)
    {
        bTypeFriends = TRUE;
    }
    else if (ds_stricmp(argv[3], "blocked") == 0)
    {
        bTypeFriends = FALSE;
    }
    else
    {
        ZPrintf("   usage: %s register [user index] [friend|blocked]\n", argv[0]);
        return;
    }

    iResult = UserListApiRegisterUpdateEvent(pApp->pUla, uUserIndex, bTypeFriends == TRUE ? USERLISTAPI_NOTIFY_FRIENDLIST_UPDATE : USERLISTAPI_NOTIFY_BLOCKEDLIST_UPDATE, &_CmdUlaUpdateCb, NULL);
    if (iResult == USERLISTAPI_ERROR_UNSUPPORTED)
    {
        ZPrintf("%s is not supported on this platform\n", argv[0], iResult);
    }
    else
    {
        ZPrintf("%s: UserListApiRegisterUpdateEvent() returned %d\n", argv[0], iResult);
    }
}

/*** Public Functions ******************************************************************/

/*F*************************************************************************************/
/*!
    \Function    CmdUserList

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
int32_t CmdUserList(ZContext *argz, int32_t argc, char *argv[])
{
    T2SubCmdT *pCmd;
    UlaAppT *pApp = &_Ula_App;
    unsigned char bHelp;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_Ula_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   test the userlist module\n");
        T2SubCmdUsage(argv[0], _Ula_Commands);
        return(0);
    }

    // if no ref yet, make one
    if ((pCmd->pFunc != _UlaCreate) && (pApp->pUla == NULL))
    {
        char *pCreate = "create";
        ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
        _UlaCreate(pApp, 1, &pCreate, bHelp);
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    return(0);
}
