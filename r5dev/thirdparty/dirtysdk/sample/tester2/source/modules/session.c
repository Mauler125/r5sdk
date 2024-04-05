/*H********************************************************************************/
/*!
    \File session.c

    \Description
        Test sessions

    \Notes
        Test framework largely borrowed from ws.c by James Brookes.

    \Copyright
        Copyright (c) Electronic Arts 2013.

    \Version 03/26/2013 (cvienneau) First Version
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtysessionmanager.h"
#include "DirtySDK/dirtysock/dirtyuser.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "libsample/zlib.h"
#include "libsample/zfile.h"
#include "libsample/zmem.h"
#include "testersubcmd.h"
#include "testermodules.h"

#if defined(DIRTYCODE_PS4) && !defined(DIRTYCODE_PS5)
#include <np/np_npid.h>
#include <np/np_common.h>
#endif

/*** Defines **********************************************************************/
#if defined(DIRTYCODE_PS4) && !defined(DIRTYCODE_PS5)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct SessionAppT
{
    DirtySessionManagerRefT *pDirtySessionManager;
} SessionAppT;

/*** Function Prototypes **********************************************************/
static void _SessionCreate(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _SessionDestroy(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _SessionControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _SessionStatus(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _SessionMaxUsers(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _SessionImage(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _SessionSetup(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _SessionInvite(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _SessionInviteNoDialog(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _SessionAccept(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _SessionAcceptDialog(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _SessionAbortRequest(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);

/*** Variables ********************************************************************/

static T2SubCmdT _Session_Commands[] =
{
    { "create",     _SessionCreate       },
    { "destroy",    _SessionDestroy      },
    { "ctrl",       _SessionControl      },
    { "stat",       _SessionStatus       },
    { "maxusers",   _SessionMaxUsers     },
    { "image",      _SessionImage        },
    { "setup",      _SessionSetup        },
    { "invite",     _SessionInvite       },
    { "inviteNoDialog", _SessionInviteNoDialog},
    { "accept",     _SessionAccept       },
    { "acceptdialog",_SessionAcceptDialog},
    { "abort",      _SessionAbortRequest },
    { "",           NULL                 },
};

static SessionAppT _Session_App = { NULL };


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _SessionDestroyApp

    \Description
        Destroy app, freeing modules.

    \Input *pApp    - app state

    \Version 11/27/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _SessionDestroyApp(SessionAppT *pApp)
{
    if (pApp->pDirtySessionManager != NULL)
    {
        DirtySessionManagerDestroy(pApp->pDirtySessionManager);
    }
    ds_memclr(pApp, sizeof(*pApp));
}

/*

    Session Commands

*/

/*F*************************************************************************************/
/*!
    \Function _SessionCreate

    \Description
        Session subcommand - create websocket module

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 11/27/2012 (jbrookes)
*/
/**************************************************************************************F*/
static void _SessionCreate(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    SessionAppT *pApp = &_Session_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s create\n", argv[0]);
        return;
    }

    // create a websocket module if it isn't already started
    if ((pApp->pDirtySessionManager = DirtySessionManagerCreate()) == NULL)
    {
        ZPrintf("%s: error creating DirtySessionManager ref.\n", argv[0]);
        return;
    }
}

/*F*************************************************************************************/
/*!
    \Function _SessionDestroy

    \Description
        Session subcommand - destroy websocket module

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 11/27/2012 (jbrookes)
*/
/**************************************************************************************F*/
static void _SessionDestroy(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    SessionAppT *pApp = &_Session_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s destroy\n", argv[0]);
        return;
    }

    _SessionDestroyApp(pApp);
}

/*F*************************************************************************************/
/*!
    \Function _SessionControl

    \Description
        Session control subcommand - set control options

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 11/27/2012 (jbrookes)
*/
/**************************************************************************************F*/
static void _SessionControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    SessionAppT *pApp = &_Session_App;
    int32_t iCmd, iValue = 0, iValue2 = 0;
    void *pValue = NULL;

    if ((bHelp == TRUE) || (argc < 3) || (argc > 6))
    {
        ZPrintf("   usage: %s ctrl [cmd] <iValue> <iValue2> <pValue>\n", argv[0]);
        return;
    }

    // get the command
    iCmd  = ZGetIntArg(argv[2]);

    // get optional arguments
    if (argc > 3)
    {
        iValue = ZGetIntArg(argv[3]);
    }
    if (argc > 4)
    {
        iValue2 = ZGetIntArg(argv[4]);
    }
    if (argc > 5)
    {
        pValue = argv[5];
    }

    // issue the control call
    DirtySessionManagerControl(pApp->pDirtySessionManager, iCmd, iValue, iValue2, pValue);
}

/*F*************************************************************************************/
/*!
    \Function _SessionStatus

    \Description
        Session status subcommand - query module status

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 11/27/2012 (jbrookes)
*/
/**************************************************************************************F*/
static void _SessionStatus(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    SessionAppT *pApp = &_Session_App;
    int32_t iCmd, iResult, iValue = 0, iValue2 = 0, iValue3 = 0; 
    char strBuffer[512] = "";

    if ((bHelp == TRUE) || (argc < 3) || (argc > 7))
    {
        ZPrintf("   usage: %s stat <cmd> <arg>\n", argv[0]);
        return;
    }

    // get the command
    iCmd  = ZGetIntArg(argv[2]);

    // get optional arguments
    if (argc > 3)
    {
        iValue = ZGetIntArg(argv[3]);
    }
    if (argc > 4)
    {
        iValue2 = ZGetIntArg(argv[4]);
    }
    if (argc > 5)
    {
        iValue3 = ZGetIntArg(argv[5]);
    }

    // issue the status call
    iResult = DirtySessionManagerStatus2(pApp->pDirtySessionManager, iCmd, iValue, iValue2, iValue3, strBuffer, sizeof(strBuffer));

    // report result
    ZPrintf("Session: DirtySessionManagerStatus('%C') returned %d (\"%s\")\n", iCmd, iResult, strBuffer);
}

/*F*************************************************************************************/
/*!
    \Function _SessionMaxUsers

    \Description
        Wrapper for 'smau'

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 12/09/2013 (mclouatre)
*/
/**************************************************************************************F*/
static void _SessionMaxUsers(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    SessionAppT *pApp = &_Session_App;
    int32_t iResult;
    int32_t iMaxUsers;

    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s <max users count>\n", argv[0]);
        return;
    }

    sscanf(argv[2], "%d", &iMaxUsers);

    if ((iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'smau', 0, iMaxUsers, NULL)) < 0)
    {
        ZPrintf("Session: max users update ('smau') failed with err %d\n", iResult);
        return;
    }
    if ((iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'umau', 0, 0, NULL)) < 0)
    {
        ZPrintf("Session: max users update ('umau') failed with err %d\n", iResult);
        return;
    }

    // report result
    ZPrintf("Session: successfully initiated session max users update to %d\n", iMaxUsers);
}

/*F*************************************************************************************/
/*!
    \Function _SessionImage

    \Description
        Wrapper for 'simg'

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 6/6/2013 (cvienneau)
*/
/**************************************************************************************F*/
static void _SessionImage(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    SessionAppT *pApp = &_Session_App;
    int32_t iResult;

    // T2 image Blue
    uint8_t img_buf[] = {255, 216, 255, 224, 0, 16, 74, 70, 73, 70, 0, 1, 1, 1, 0, 96, 0, 96, 0, 0, 255, 225, 0, 104, 69, 120, 105, 102, 0, 0, 77, 77, 0, 42, 0, 0, 0, 8, 0, 4, 1, 26, 0, 5, 0, 0, 0, 1, 0, 0, 0, 62, 1, 27, 0, 5, 0, 0, 0, 1, 0, 0, 0, 70, 1, 40, 0, 3, 0, 0, 0, 1, 0, 2, 0, 0, 1, 49, 0, 2, 0, 0, 0, 18, 0, 0, 0, 78, 0, 0, 0, 0, 0, 0, 0, 96, 0, 0, 0, 1, 0, 0, 0, 96, 0, 0, 0, 1, 80, 97, 105, 110, 116, 46, 78, 69, 84, 32, 118, 51, 46, 53, 46, 49, 48, 0, 255, 219, 0, 67, 0, 2, 1, 1, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 5, 3, 3, 3, 3, 3, 6, 4, 4, 3, 5, 7, 6, 7, 7, 7, 6, 7, 7, 8, 9, 11, 9, 8, 8, 10, 8, 7, 7, 10, 13, 10, 10, 11, 12, 12, 12, 12, 7, 9, 14, 15, 13, 12, 14, 11, 12, 12, 12, 255, 219, 0, 67, 1, 2, 2, 2, 3, 3, 3, 6, 3, 3, 6, 12, 8, 7, 8, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 255, 192, 0, 17, 8, 0, 36, 0, 64, 3, 1, 34, 0, 2, 17, 1, 3, 17, 1, 255, 196, 0, 31, 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 255, 196, 0, 181, 16, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125, 1, 2, 3, 0, 4, 17, 5, 18, 33, 49, 65, 6, 19, 81, 97, 7, 34, 113, 20, 50, 129, 145, 161, 8, 35, 66, 177, 193, 21, 82, 209, 240, 36, 51, 98, 114, 130, 9, 10, 22, 23, 24, 25, 26, 37, 38, 39, 40, 41, 42, 52, 53, 54, 55, 56, 57, 58, 67, 68, 69, 70, 71, 72, 73, 74, 83, 84, 85, 86, 87, 88, 89, 90, 99, 100, 101, 102, 103, 104, 105, 106, 115, 116, 117, 118, 119, 120, 121, 122, 131, 132, 133, 134, 135, 136, 137, 138, 146, 147, 148, 149, 150, 151, 152, 153, 154, 162, 163, 164, 165, 166, 167, 168, 169, 170, 178, 179, 180, 181, 182, 183, 184, 185, 186, 194, 195, 196, 197, 198, 199, 200, 201, 202, 210, 211, 212, 213, 214, 215, 216, 217, 218, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 255, 196, 0, 31, 1, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 255, 196, 0, 181, 17, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 119, 0, 1, 2, 3, 17, 4, 5, 33, 49, 6, 18, 65, 81, 7, 97, 113, 19, 34, 50, 129, 8, 20, 66, 145, 161, 177, 193, 9, 35, 51, 82, 240, 21, 98, 114, 209, 10, 22, 36, 52, 225, 37, 241, 23, 24, 25, 26, 38, 39, 40, 41, 42, 53, 54, 55, 56, 57, 58, 67, 68, 69, 70, 71, 72, 73, 74, 83, 84, 85, 86, 87, 88, 89, 90, 99, 100, 101, 102, 103, 104, 105, 106, 115, 116, 117, 118, 119, 120, 121, 122, 130, 131, 132, 133, 134, 135, 136, 137, 138, 146, 147, 148, 149, 150, 151, 152, 153, 154, 162, 163, 164, 165, 166, 167, 168, 169, 170, 178, 179, 180, 181, 182, 183, 184, 185, 186, 194, 195, 196, 197, 198, 199, 200, 201, 202, 210, 211, 212, 213, 214, 215, 216, 217, 218, 226, 227, 228, 229, 230, 231, 232, 233, 234, 242, 243, 244, 245, 246, 247, 248, 249, 250, 255, 218, 0, 12, 3, 1, 0, 2, 17, 3, 17, 0, 63, 0, 252, 195, 162, 138, 43, 253, 208, 62, 76, 40, 175, 217, 223, 216, 183, 224, 15, 252, 19, 115, 246, 162, 189, 240, 55, 129, 116, 189, 39, 251, 115, 226, 118, 185, 165, 199, 246, 139, 31, 181, 120, 162, 215, 207, 187, 138, 204, 207, 117, 251, 198, 116, 129, 112, 34, 153, 184, 96, 167, 110, 23, 57, 0, 247, 191, 181, 71, 236, 101, 255, 0, 4, 221, 253, 137, 252, 97, 167, 104, 31, 19, 188, 55, 255, 0, 8, 206, 173, 171, 89, 255, 0, 104, 90, 193, 253, 161, 226, 139, 223, 54, 13, 237, 30, 253, 214, 242, 200, 163, 230, 70, 24, 36, 30, 58, 98, 191, 2, 196, 253, 32, 178, 250, 25, 143, 246, 68, 242, 156, 127, 214, 26, 114, 80, 250, 188, 121, 165, 20, 218, 230, 140, 125, 167, 51, 142, 143, 222, 74, 218, 62, 199, 74, 195, 54, 185, 185, 149, 189, 79, 194, 90, 43, 245, 43, 254, 9, 173, 255, 0, 4, 184, 248, 1, 251, 68, 233, 127, 22, 62, 48, 248, 227, 88, 150, 231, 225, 135, 134, 117, 205, 84, 232, 26, 13, 165, 244, 246, 237, 253, 145, 109, 230, 74, 46, 174, 57, 251, 89, 79, 36, 97, 23, 42, 231, 201, 114, 196, 158, 43, 211, 190, 16, 255, 0, 193, 51, 63, 100, 95, 248, 42, 199, 236, 217, 226, 173, 119, 224, 23, 135, 124, 103, 240, 203, 196, 62, 30, 185, 147, 79, 130, 77, 78, 246, 226, 85, 107, 161, 16, 146, 47, 58, 57, 110, 46, 81, 160, 125, 195, 38, 55, 87, 24, 57, 3, 128, 222, 174, 107, 227, 166, 67, 151, 226, 170, 208, 196, 80, 175, 236, 232, 202, 17, 171, 85, 83, 94, 206, 148, 167, 180, 102, 220, 148, 174, 182, 124, 177, 149, 158, 154, 147, 28, 60, 154, 186, 177, 248, 207, 69, 62, 226, 6, 182, 157, 227, 112, 3, 198, 197, 88, 2, 14, 8, 224, 242, 41, 149, 251, 66, 102, 1, 69, 20, 80, 7, 216, 191, 240, 64, 111, 249, 75, 103, 194, 111, 251, 140, 127, 233, 154, 250, 189, 243, 254, 14, 162, 255, 0, 147, 192, 248, 119, 255, 0, 98, 112, 255, 0, 210, 219, 154, 248, 35, 246, 66, 253, 169, 124, 65, 251, 22, 126, 209, 62, 30, 248, 153, 225, 107, 61, 26, 255, 0, 94, 240, 215, 218, 126, 203, 6, 171, 20, 146, 218, 73, 231, 219, 75, 108, 251, 214, 57, 35, 115, 132, 153, 136, 195, 143, 152, 12, 228, 100, 30, 187, 246, 253, 255, 0, 130, 134, 120, 211, 254, 10, 53, 241, 35, 70, 241, 71, 141, 244, 191, 11, 233, 90, 134, 135, 166, 255, 0, 101, 65, 30, 135, 109, 60, 16, 188, 94, 107, 203, 185, 132, 211, 74, 75, 110, 144, 242, 8, 24, 3, 142, 245, 248, 222, 103, 193, 57, 157, 127, 18, 240, 156, 81, 77, 71, 234, 212, 240, 206, 148, 157, 253, 238, 119, 42, 175, 225, 237, 239, 173, 77, 213, 68, 169, 56, 117, 185, 232, 223, 240, 73, 191, 248, 36, 223, 136, 63, 224, 164, 223, 16, 110, 174, 174, 174, 238, 60, 57, 240, 223, 195, 147, 34, 107, 90, 194, 32, 105, 167, 144, 128, 194, 210, 212, 48, 218, 102, 43, 130, 88, 229, 98, 86, 12, 193, 137, 68, 127, 209, 31, 219, 119, 226, 255, 0, 140, 63, 98, 255, 0, 128, 55, 223, 179, 223, 236, 157, 251, 62, 252, 86, 91, 84, 142, 91, 61, 75, 197, 150, 222, 15, 212, 102, 182, 83, 34, 237, 154, 75, 105, 188, 162, 215, 87, 47, 208, 220, 147, 177, 66, 175, 151, 184, 5, 41, 240, 7, 236, 89, 255, 0, 5, 203, 248, 167, 251, 8, 124, 5, 177, 248, 121, 224, 191, 9, 124, 49, 185, 210, 108, 174, 103, 188, 123, 189, 79, 78, 190, 150, 242, 242, 105, 164, 44, 207, 43, 71, 119, 26, 18, 6, 212, 24, 65, 242, 198, 160, 228, 130, 79, 171, 255, 0, 196, 82, 159, 180, 15, 253, 9, 255, 0, 7, 63, 240, 85, 169, 127, 242, 125, 124, 7, 28, 112, 167, 30, 231, 92, 73, 245, 186, 216, 74, 88, 140, 13, 9, 94, 141, 25, 86, 229, 131, 107, 106, 149, 34, 149, 231, 39, 171, 81, 147, 180, 83, 181, 183, 190, 180, 231, 78, 48, 178, 118, 103, 230, 221, 205, 180, 150, 119, 18, 67, 52, 111, 20, 177, 49, 71, 71, 82, 172, 140, 14, 8, 32, 244, 32, 211, 42, 206, 181, 170, 201, 174, 235, 55, 119, 211, 42, 44, 183, 147, 60, 238, 16, 16, 161, 153, 139, 16, 51, 158, 50, 106, 181, 127, 80, 198, 252, 171, 155, 115, 140, 40, 162, 138, 160, 10, 40, 162, 128, 10, 40, 162, 128, 10, 40, 162, 128, 63, 255, 217};

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s image\n", argv[0]);
        return;
    }
    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'simg', 0, sizeof(img_buf), img_buf);
    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'uimg', 0, 0, NULL);

    // report result
    ZPrintf("Session: new image attached\n");
}


/*F*************************************************************************************/
/*!
    \Function _SessionSetup

    \Description
        Wrapper for creating a session with common defaults

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 6/6/2013 (cvienneau)
*/
/**************************************************************************************F*/
static void _SessionSetup(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    SessionAppT *pApp = &_Session_App;
    int32_t iResult;

    // T2 image Green
    uint8_t img_buf[] = {255, 216, 255, 224, 0, 16, 74, 70, 73, 70, 0, 1, 1, 1, 0, 96, 0, 96, 0, 0, 255, 225, 0, 104, 69, 120, 105, 102, 0, 0, 77, 77, 0, 42, 0, 0, 0, 8, 0, 4, 1, 26, 0, 5, 0, 0, 0, 1, 0, 0, 0, 62, 1, 27, 0, 5, 0, 0, 0, 1, 0, 0, 0, 70, 1, 40, 0, 3, 0, 0, 0, 1, 0, 2, 0, 0, 1, 49, 0, 2, 0, 0, 0, 18, 0, 0, 0, 78, 0, 0, 0, 0, 0, 0, 0, 96, 0, 0, 0, 1, 0, 0, 0, 96, 0, 0, 0, 1, 80, 97, 105, 110, 116, 46, 78, 69, 84, 32, 118, 51, 46, 53, 46, 49, 48, 0, 255, 219, 0, 67, 0, 2, 1, 1, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 5, 3, 3, 3, 3, 3, 6, 4, 4, 3, 5, 7, 6, 7, 7, 7, 6, 7, 7, 8, 9, 11, 9, 8, 8, 10, 8, 7, 7, 10, 13, 10, 10, 11, 12, 12, 12, 12, 7, 9, 14, 15, 13, 12, 14, 11, 12, 12, 12, 255, 219, 0, 67, 1, 2, 2, 2, 3, 3, 3, 6, 3, 3, 6, 12, 8, 7, 8, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 255, 192, 0, 17, 8, 0, 36, 0, 64, 3, 1, 34, 0, 2, 17, 1, 3, 17, 1, 255, 196, 0, 31, 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 255, 196, 0, 181, 16, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125, 1, 2, 3, 0, 4, 17, 5, 18, 33, 49, 65, 6, 19, 81, 97, 7, 34, 113, 20, 50, 129, 145, 161, 8, 35, 66, 177, 193, 21, 82, 209, 240, 36, 51, 98, 114, 130, 9, 10, 22, 23, 24, 25, 26, 37, 38, 39, 40, 41, 42, 52, 53, 54, 55, 56, 57, 58, 67, 68, 69, 70, 71, 72, 73, 74, 83, 84, 85, 86, 87, 88, 89, 90, 99, 100, 101, 102, 103, 104, 105, 106, 115, 116, 117, 118, 119, 120, 121, 122, 131, 132, 133, 134, 135, 136, 137, 138, 146, 147, 148, 149, 150, 151, 152, 153, 154, 162, 163, 164, 165, 166, 167, 168, 169, 170, 178, 179, 180, 181, 182, 183, 184, 185, 186, 194, 195, 196, 197, 198, 199, 200, 201, 202, 210, 211, 212, 213, 214, 215, 216, 217, 218, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 255, 196, 0, 31, 1, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 255, 196, 0, 181, 17, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 119, 0, 1, 2, 3, 17, 4, 5, 33, 49, 6, 18, 65, 81, 7, 97, 113, 19, 34, 50, 129, 8, 20, 66, 145, 161, 177, 193, 9, 35, 51, 82, 240, 21, 98, 114, 209, 10, 22, 36, 52, 225, 37, 241, 23, 24, 25, 26, 38, 39, 40, 41, 42, 53, 54, 55, 56, 57, 58, 67, 68, 69, 70, 71, 72, 73, 74, 83, 84, 85, 86, 87, 88, 89, 90, 99, 100, 101, 102, 103, 104, 105, 106, 115, 116, 117, 118, 119, 120, 121, 122, 130, 131, 132, 133, 134, 135, 136, 137, 138, 146, 147, 148, 149, 150, 151, 152, 153, 154, 162, 163, 164, 165, 166, 167, 168, 169, 170, 178, 179, 180, 181, 182, 183, 184, 185, 186, 194, 195, 196, 197, 198, 199, 200, 201, 202, 210, 211, 212, 213, 214, 215, 216, 217, 218, 226, 227, 228, 229, 230, 231, 232, 233, 234, 242, 243, 244, 245, 246, 247, 248, 249, 250, 255, 218, 0, 12, 3, 1, 0, 2, 17, 3, 17, 0, 63, 0, 240, 122, 40, 171, 254, 21, 176, 181, 213, 124, 81, 166, 218, 223, 77, 246, 107, 43, 155, 168, 162, 184, 155, 120, 79, 42, 54, 112, 25, 183, 30, 6, 1, 39, 39, 129, 138, 252, 145, 43, 187, 31, 198, 145, 87, 105, 34, 133, 21, 250, 175, 240, 247, 254, 8, 243, 251, 44, 124, 91, 214, 165, 211, 124, 41, 241, 131, 93, 241, 62, 163, 4, 6, 230, 75, 93, 39, 197, 122, 69, 236, 209, 196, 25, 84, 200, 82, 59, 102, 96, 161, 157, 70, 226, 49, 150, 3, 184, 172, 255, 0, 138, 31, 240, 73, 127, 217, 87, 225, 61, 214, 161, 167, 107, 127, 25, 117, 109, 19, 94, 178, 183, 50, 255, 0, 103, 106, 94, 45, 209, 237, 174, 1, 41, 185, 55, 68, 246, 234, 248, 97, 130, 56, 228, 30, 43, 215, 121, 30, 37, 71, 154, 241, 183, 170, 62, 213, 240, 6, 102, 169, 251, 87, 42, 124, 189, 249, 213, 190, 243, 242, 226, 138, 251, 147, 246, 53, 255, 0, 130, 83, 248, 91, 226, 31, 236, 201, 47, 198, 47, 139, 222, 51, 212, 60, 37, 224, 214, 134, 91, 152, 98, 211, 213, 22, 117, 183, 141, 204, 126, 115, 200, 233, 39, 222, 117, 33, 99, 88, 217, 155, 43, 131, 150, 11, 90, 223, 28, 191, 224, 147, 159, 15, 188, 71, 251, 42, 106, 31, 22, 254, 5, 120, 247, 88, 241, 86, 137, 164, 90, 207, 125, 53, 182, 171, 18, 151, 185, 134, 2, 124, 253, 172, 34, 133, 163, 116, 85, 118, 216, 241, 229, 128, 224, 140, 140, 227, 28, 171, 16, 233, 251, 68, 150, 215, 181, 213, 237, 222, 199, 4, 56, 67, 50, 158, 29, 98, 20, 86, 177, 231, 81, 230, 92, 238, 63, 205, 203, 123, 216, 248, 22, 138, 40, 175, 52, 249, 128, 162, 138, 40, 3, 238, 207, 248, 55, 199, 254, 79, 55, 196, 223, 246, 37, 221, 127, 233, 117, 133, 112, 159, 240, 91, 31, 249, 72, 111, 139, 63, 235, 203, 78, 255, 0, 210, 56, 171, 132, 255, 0, 130, 124, 126, 218, 191, 240, 194, 63, 25, 245, 63, 23, 127, 194, 53, 255, 0, 9, 87, 246, 142, 139, 46, 143, 246, 79, 237, 31, 176, 249, 123, 231, 130, 111, 51, 127, 149, 38, 113, 228, 99, 110, 209, 247, 179, 158, 48, 112, 127, 109, 159, 218, 131, 254, 27, 19, 246, 135, 213, 188, 123, 253, 135, 255, 0, 8, 239, 246, 164, 22, 240, 253, 135, 237, 191, 108, 242, 188, 168, 86, 60, 249, 158, 92, 121, 206, 220, 253, 209, 140, 227, 158, 181, 235, 75, 21, 73, 229, 202, 130, 126, 247, 53, 237, 229, 175, 200, 251, 42, 185, 174, 22, 92, 51, 12, 189, 79, 247, 170, 167, 53, 172, 246, 179, 214, 246, 183, 94, 247, 61, 131, 254, 9, 231, 251, 30, 252, 85, 253, 185, 52, 219, 189, 21, 124, 99, 226, 111, 15, 124, 35, 178, 145, 96, 213, 157, 175, 230, 107, 75, 150, 82, 142, 45, 161, 183, 221, 229, 201, 32, 219, 27, 18, 70, 216, 240, 172, 114, 118, 43, 123, 95, 237, 193, 251, 122, 252, 56, 253, 155, 191, 102, 235, 159, 217, 243, 224, 88, 75, 235, 115, 107, 46, 151, 171, 107, 17, 200, 100, 134, 20, 114, 69, 194, 172, 191, 242, 222, 121, 114, 193, 221, 127, 118, 161, 200, 92, 240, 19, 152, 253, 151, 255, 0, 224, 183, 154, 127, 236, 191, 240, 19, 195, 62, 5, 211, 126, 16, 139, 184, 124, 63, 104, 33, 150, 235, 254, 18, 113, 9, 188, 157, 137, 121, 102, 41, 246, 70, 219, 190, 70, 102, 219, 185, 176, 8, 25, 56, 205, 116, 63, 20, 191, 224, 224, 79, 248, 89, 95, 12, 188, 71, 225, 207, 248, 84, 159, 98, 254, 223, 210, 238, 116, 223, 180, 127, 194, 81, 230, 121, 30, 116, 77, 30, 253, 191, 100, 27, 177, 187, 56, 200, 206, 58, 138, 239, 163, 91, 9, 79, 13, 203, 10, 214, 155, 86, 111, 150, 77, 250, 46, 200, 250, 28, 22, 51, 38, 195, 101, 158, 203, 15, 140, 229, 175, 56, 218, 114, 116, 231, 39, 103, 246, 34, 244, 81, 87, 210, 250, 223, 126, 214, 252, 226, 162, 138, 43, 230, 143, 203, 2, 138, 40, 160, 2, 138, 40, 160, 2, 138, 40, 160, 2, 138, 40, 160, 15, 255, 217};

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s setup\n", argv[0]);
        return;
    }

    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'simg', 0, sizeof(img_buf), img_buf);
    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'smau', 0, 22, NULL);
    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'slid', 0, 8, "12345678");

    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'snam', 0, 0, "Default Name");
    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'snam', 0, 'enUS', "enUS Name");
    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'snam', 0, 'frFR', "frFR Name");

    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'ssta', 0, 0, "Default Status");
    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'ssta', 0, 'enUS', "enUS Status");
    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'ssta', 0, 'frFR', "frFR Status");

    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'scre', 0, 0, NULL);

    // report result
    ZPrintf("Session: Setup Complete\n");
}

/*F*************************************************************************************/
/*!
    \Function _SessionInvite

    \Description
        Wrapper for opening invite dialog

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 6/6/2013 (cvienneau)
*/
/**************************************************************************************F*/
static void _SessionInvite(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    SessionAppT *pApp = &_Session_App;
    int32_t iResult;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s invite\n", argv[0]);
        return;
    }

    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'imus', 16, 0, NULL);
    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'imsg', 0, 0, "Play T2 with me");
    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'iued', 1, 0, NULL);
    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'osid', 0, 0, NULL);

    // report result
    ZPrintf("Session: Invite Complete\n");
}



/*F*************************************************************************************/
/*!
    \Function _SessionInviteNoDialog

    \Description
        Wrapper for _SessionInviteNoDialog

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 11/4/2013 (tcho)
*/
/**************************************************************************************F*/
static void _SessionInviteNoDialog(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    #if defined(DIRTYCODE_PS4)

    SessionAppT *pApp = &_Session_App;
    int32_t iResult;

    if (bHelp == TRUE)
    {
        ZPrintf("usage: %s inviteNoDialog <list of onlineId to send invite to>\n", argv[0]);
        return;
    }

    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'imus', 16, 0, NULL);
    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'imsg', 0, 0, "Play T2 with me");

    for(int32_t index = 2; index < argc; ++index)
    {
        iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'ianp', 0, 0, argv[index]);
    }

    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'sind', 1, 0, NULL);
    iResult = DirtySessionManagerControl(pApp->pDirtySessionManager, 'icnp', 0, 0, NULL);
    
    // report result
    ZPrintf("Session: Invite Complete\n");

    #else
    
    ZPrintf("Session: Invite without dialog not support\n");
    
    #endif
}

/*F*************************************************************************************/
/*!
    \Function _SessionAccept

    \Description
        Examin current invites and join one of them 
        Must be done after DirtySessionManagerControl('ginv') has completed

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 6/6/2013 (cvienneau)
*/
/**************************************************************************************F*/
static void _SessionAccept(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    SessionAppT *pApp = &_Session_App;
    int32_t i, iInviteCount, iUserIndex = 0;
    DirtyUserT user;
    char strUserName[32];
    char strMessage[512];
    char strSessionId[64];
    strUserName[0] = 0;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s accept <user index>\n", argv[0]);
        return;
    }

    // get optional arguments
    if (argc > 3)
    {
        iUserIndex = ZGetIntArg(argv[3]);
    }

    iInviteCount = DirtySessionManagerStatus2(pApp->pDirtySessionManager, 'ginv', iUserIndex, 0, 0, NULL, 0);

    ZPrintf("   Current Invitations: %d\n", iInviteCount);

    //print out info from all the invites
    for (i = 0; i < iInviteCount; i++)
    {
        DirtySessionManagerStatus2(pApp->pDirtySessionManager, 'gims', iUserIndex, i, 0, strMessage, sizeof(strMessage));
        DirtySessionManagerStatus2(pApp->pDirtySessionManager, 'gisi', iUserIndex, i, 0, strSessionId, sizeof(strSessionId));
        DirtySessionManagerStatus2(pApp->pDirtySessionManager, 'giun', iUserIndex, i, 0, &user, sizeof(user));
        #if defined(DIRTYCODE_PS4)
            SceNpAccountId accountId;
            DirtyUserToNativeUser(&accountId, sizeof(accountId), &user);
            ds_snzprintf(strUserName, sizeof(strUserName), "%llu",  accountId);
        #endif
        ZPrintf("  * %d %s %llu %s\n", iInviteCount, strSessionId, strUserName, strMessage);

    }

    if (iInviteCount > 0)
    {
        ZPrintf("   Joining: %s\n", strSessionId);
        //join what ever last invite was processes
        DirtySessionManagerControl(pApp->pDirtySessionManager, 'sjoi', 0, 0, strSessionId);
        DirtySessionManagerControl(pApp->pDirtySessionManager, 'uinv', 0, 0, NULL);
    }

    // report result
    ZPrintf("Session: Accept Complete\n");
}

/*F*************************************************************************************/
/*!
    \Function _SessionAcceptDialog

    \Description
        Join the session the user selected from the invitation dialog 'osrd'

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 6/6/2013 (cvienneau)
*/
/**************************************************************************************F*/
static void _SessionAcceptDialog(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    SessionAppT *pApp = &_Session_App;
    char strSessionId[64];

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s acceptdialog\n", argv[0]);
        return;
    }

    //check to see that we do have a session id
    DirtySessionManagerStatus(pApp->pDirtySessionManager, 'gssi', strSessionId, sizeof(strSessionId));

    if (strSessionId[0] == '\0')
    {
        //it doen't look the there is a selected session
        ZPrintf("Session: no session slected to join\n");
    }
    else
    {
        ZPrintf("Session: joining selected session\n");
        DirtySessionManagerControl(pApp->pDirtySessionManager, 'sjoi', 0, 0, strSessionId);
        DirtySessionManagerControl(pApp->pDirtySessionManager, 'cssi', 0, 0, NULL);
    }

    // report result
    ZPrintf("Session: AcceptDialog Complete\n");
}

/*F*************************************************************************************/
/*!
    \Function _SessionAbortRequest

    \Description
        Create several requests then abort them.

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 6/6/2013 (cvienneau)
*/
/**************************************************************************************F*/
static void _SessionAbortRequest(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    SessionAppT *pApp = &_Session_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s abort\n", argv[0]);
        return;
    }

    //que getting invites for every index
    DirtySessionManagerControl(pApp->pDirtySessionManager, 'ginv', 0, 0, NULL);
    DirtySessionManagerControl(pApp->pDirtySessionManager, 'ginv', 0, 0, NULL);
    DirtySessionManagerControl(pApp->pDirtySessionManager, 'ginv', 0, 0, NULL);
    DirtySessionManagerControl(pApp->pDirtySessionManager, 'ginv', 0, 0, NULL);

    //give the items a chance to process just a bit
    //NetConnSleep(500);

    //abort all outsanding requests
    DirtySessionManagerControl(pApp->pDirtySessionManager, 'abrt', 0, 0, NULL);

    // report result
    ZPrintf("Session: Abort Test Complete\n");
}

/*F********************************************************************************/
/*!
    \Function _CmdSessionCb

    \Description
        Update Session command

    \Input *argz    - environment
    \Input argc     - standard number of arguments
    \Input *argv[]  - standard arg list

    \Output
        int32_T     -standard return value

    \Version 11/27/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdSessionCb(ZContext *argz, int32_t argc, char *argv[])
{
    SessionAppT *pApp = &_Session_App;

    // check for kill
    if (argc == 0)
    {
        _SessionDestroyApp(pApp);
        ZPrintf("%s: killed\n", argv[0]);
        return(0);
    }

    // give life to the module
    if (pApp->pDirtySessionManager != NULL)
    {
        // update the module
        DirtySessionManagerUpdate(pApp->pDirtySessionManager);
    }

    // keep running
    return(ZCallback(&_CmdSessionCb, 16));
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdSession

    \Description
        Session (WebSocket) command

    \Input *argz    - environment
    \Input argc     - standard number of arguments
    \Input *argv[]  - standard arg list

    \Output
        int32_t     - standard return value

    \Version 11/27/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdSession(ZContext *argz, int32_t argc, char *argv[])
{
    T2SubCmdT *pCmd;
    SessionAppT *pApp = &_Session_App;
    int32_t iResult = 0;
    uint8_t bHelp;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_Session_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   test the DirtySessionManager module\n");
        T2SubCmdUsage(argv[0], _Session_Commands);
        return(0);
    }

    // if no ref yet, make one
    if ((pCmd->pFunc != _SessionCreate) && (pApp->pDirtySessionManager == NULL))
    {
        char *pCreate = "create";
        ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
        _SessionCreate(pApp, 1, &pCreate, bHelp);
        iResult = ZCallback(_CmdSessionCb, 16);
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    // one-time install of periodic callback
    if (pCmd->pFunc == _SessionCreate)
    {
        iResult = ZCallback(_CmdSessionCb, 16);
    }
    return(iResult);
}

#endif  //defined(DIRTYCODE_PS4)

