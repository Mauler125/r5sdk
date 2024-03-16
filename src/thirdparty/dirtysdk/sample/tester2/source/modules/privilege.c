/*H*************************************************************************************/
/*!
    \File    privilege.c

    \Description
        Reference application for the privilege api.

    \Copyright
        Copyright (c) Electronic Arts 2014.    ALL RIGHTS RESERVED.

    \Version 18/02/2014 (amakoukji)
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#include <string.h>
#include <stdlib.h>

#include "DirtySDK/platform.h"

#if defined(DIRTYCODE_PS4)
#include <np/np_npid.h>
#include <libsysmodule.h>
#endif

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/misc/privilegeapi.h"

#include "libsample/zlib.h"
#include "libsample/zfile.h"
#include "libsample/zmem.h"
#include "testersubcmd.h"
#include "testermodules.h"

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct PrivaAppT
{
    PrivilegeApiRefT *pPrivApi;
    int32_t iCurrentQueryId;

    uint8_t bStarted;

    unsigned char bZCallback;
} PrivaAppT;

/*** Function Prototypes ***************************************************************/

static void _PrivApiCreate(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _PrivApiDestroy(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _PrivApiCheckPriv(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _PrivApiAbort(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static int32_t _CmdPrivTickCb(ZContext *argz, int32_t argc, char *argv[]);

/*** Variables *************************************************************************/

// Private variables
static T2SubCmdT _Ula_Commands[] =
{
    { "create",              _PrivApiCreate       },
    { "destroy",             _PrivApiDestroy      },
    { "check",               _PrivApiCheckPriv    },
    { "abort",               _PrivApiAbort        },
    { "",                    NULL                 }
};

static PrivaAppT _Priv_App;

// Public variables

/*** Private Functions *****************************************************************/
/*F********************************************************************************/
/*!
    \Function _CmdPrivTickCb

    \Description
        Priv callback, called after command has been issued.

    \Input *argz    - pointer to context
    \Input argc     - number of command-line arguments
    \Input *argv[]   - command-line argument list

    \Output
        int32_t      - result of zcallback, or zero to terminate

    \Version 18/02/2014 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _CmdPrivTickCb(ZContext *argz, int32_t argc, char *argv[])
{
    PrivaAppT *pApp = (PrivaAppT*)&_Priv_App;
    int32_t  iResult = 0;

    // check for kill
    if (argc == 0)
    {
        ZPrintf("%s: killed\n", argv[0]);
        PrivilegeApiDestroy(pApp->pPrivApi);
        return(0);
    }

    // update module
    if (pApp->pPrivApi != NULL && pApp->iCurrentQueryId != -1)
    {
        iResult = PrivilegeApiCheckResult(pApp->pPrivApi, pApp->iCurrentQueryId);

        if (iResult < 0)
        {
            ZPrintf("%s: error in privcheck for %d with code %d. Possibly Aborted? \n", argv[0], pApp->iCurrentQueryId, iResult);
            pApp->iCurrentQueryId = -1;
        }
        else if (iResult & PRIVILEGEAPI_STATUS_IN_PROGRESS)
        {
            
        }
        else
        {
            ZPrintf("%s: Result of priv check %d = %d \n", argv[0], pApp->iCurrentQueryId, iResult);
            PrivilegeApiReleaseRequest(pApp->pPrivApi, pApp->iCurrentQueryId);
            pApp->iCurrentQueryId = -1;
        }
    }

    // keep recurring
    return(ZCallback(&_CmdPrivTickCb, 17));
}

/*F*************************************************************************************/
/*!
    \Function _PrivApiCreate

    \Description
        UserList create

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _PrivApiCreate(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    PrivaAppT *pApp = (PrivaAppT*)&_Priv_App;
#if defined(DIRTYCODE_PS4)
    int32_t iResult = 0;
#endif

    if ((bHelp == TRUE) || (argc != 1 && argc != 2))
    {
        ZPrintf("   usage: %s create \n", argv[0]);
        return;
    }

    if (pApp->bStarted)
    {
        ZPrintf("%s: already created\n", argv[0]);
        return;
    }

    pApp->iCurrentQueryId = -1;
#if defined(DIRTYCODE_PS4)
    if ((iResult = sceSysmoduleLoadModule(SCE_SYSMODULE_NP_COMMERCE)) != SCE_OK)
    {
        ZPrintf("%s: error loading commerce module with code %d\n", argv[0], iResult);
        return;
    }
    if ((iResult = sceSysmoduleLoadModule(SCE_SYSMODULE_MESSAGE_DIALOG)) != SCE_OK)
    {
        ZPrintf("%s: error loading message module with code %d\n", argv[0], iResult);
        return;
    }
#endif

    // allocate ProtoUdp module
    if ((pApp->pPrivApi = PrivilegeApiCreate()) == NULL)
    {
        ZPrintf("%s: unable to create userlist module\n", argv[0]);
        return;
    }

    pApp->bStarted = TRUE;

    // one-time install of periodic callback
    if (pApp->bZCallback == FALSE)
    {
        pApp->bZCallback = TRUE;
        ZCallback(_CmdPrivTickCb, 17);
    }
}

/*F*************************************************************************************/
/*!
    \Function _PrivApiDestroy

    \Description
        UserList destroy

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _PrivApiDestroy(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    PrivaAppT *pApp = (PrivaAppT*)&_Priv_App;

    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s destroy\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: not yet created\n", argv[0]);
        return;
    }

    PrivilegeApiDestroy(pApp->pPrivApi);
    pApp->bStarted = FALSE;
}

/*F*************************************************************************************/
/*!
    \Function _PrivApiCheckPriv

    \Description
        Fetch friends list

    \Input *_pApp   - pointer to app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _PrivApiCheckPriv(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    PrivaAppT *pApp = (PrivaAppT*)&_Priv_App;
    int32_t  iResult;
    int32_t  iUserIndex = 0;
    PrivilegeApiPrivE  ePrivId = PRIVILEGEAPI_PRIV_INVALID;
    int32_t  iHint = 0;
    int32_t  bUseUI = FALSE;

    if ((bHelp == TRUE) || ((argc != 4) && (argc != 5)))
    {
        ZPrintf("   usage: %s checkpriv [user index] [privilege id] [use UI?]\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: not yet created\n", argv[0]);
        return;
    }

    if (pApp->iCurrentQueryId > -1)
    {
        ZPrintf("%s: check busy! Try again later\n", argv[0]);
        return;
    }

    // get arguments
    iUserIndex = (int32_t)strtol(argv[2], NULL, 10);
    ePrivId =    (PrivilegeApiPrivE)strtol(argv[3], NULL, 10);
    if (argc == 5)
    {
        int iUserIn = (int32_t)strtol(argv[4], NULL, 10);
        bUseUI = iUserIn > 0 ? TRUE : FALSE;
    }

    iResult =  PrivilegeApiCheckPrivilegesAsyncWithUi(pApp->pPrivApi, iUserIndex, &ePrivId, 1, &iHint, bUseUI ? "Test" : NULL);

    if (iResult > 0)
    {
        pApp->iCurrentQueryId = iResult;
        ZPrintf("%s check query %d started\n", argv[0], iResult);
    }
    else if (iResult == 0)
    {
        ZPrintf("%s: Privilege Check result  %d\n", argv[0], iHint);
    }
    else
    {
        ZPrintf("%s: PrivilegeApiCheckPrivilegesAsyncWithUi() failed with code  %d\n", argv[0], iResult);
    }
}

/*F*************************************************************************************/
/*!
    \Function _PrivApiAbort

    \Description
        Fetch blocked list

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
static void _PrivApiAbort(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    PrivaAppT *pApp = (PrivaAppT*)&_Priv_App;
    int32_t  iResult;
    int32_t  iId = 0;

    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s abort [query id]\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: priv not yet created\n", argv[0]);
        return;
    }
    // get the receive buffer size (use 0=ANY if not specified)
    iId = (int32_t)strtol(argv[2], NULL, 10);
    iResult = PrivilegeApiAbort(pApp->pPrivApi, iId);
    if (iResult < 0)
    {
        ZPrintf("%s failed with error code\n", argv[0], iResult);
    }
    else
    {
        ZPrintf("%s: query %d aborted\n", argv[0], iId);
    }
}

/*** Public Functions ******************************************************************/

/*F*************************************************************************************/
/*!
    \Function    CmdPriv

    \Description
        Priv command.

    \Input *argz    - unused
    \Input argc     - argument count
    \Input **argv   - argument list

    \Output
        int32_t         - zero

    \Version 18/02/2014 (amakoukji)
*/
/**************************************************************************************F*/
int32_t CmdPriv(ZContext *argz, int32_t argc, char *argv[])
{
    T2SubCmdT *pCmd;
    PrivaAppT *pApp = &_Priv_App;
    unsigned char bHelp;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_Ula_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   test the privilege api module\n");
        T2SubCmdUsage(argv[0], _Ula_Commands);
        return(0);
    }

    // if no ref yet, make one
    if ((pCmd->pFunc != _PrivApiCreate) && (pApp->pPrivApi == NULL))
    {
        char *pCreate = "create";
        ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
        _PrivApiCreate(pApp, 1, &pCreate, bHelp);
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    return(0);
}
