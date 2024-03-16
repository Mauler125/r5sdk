/*H********************************************************************************/
/*!
    \File upnp.c

    \Description
        A tester command to test ProtoUpnp

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/23/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <stdlib.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/proto/protoupnp.h"

#include "libsample/zlib.h"
#include "libsample/zmem.h"
#include "libsample/zfile.h"

#include "testerregistry.h"
#include "testersubcmd.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct UpnpAppT
{
    ProtoUpnpRefT *pProtoUpnp;

    unsigned char bZCallback;
} UpnpAppT;

/*** Function Prototypes ***************************************************************/

static void _UpnpCreate(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _UpnpDestroy(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _UpnpControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _UpnpFakeResponse(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);

/*** Variables ********************************************************************/

static T2SubCmdT _Upnp_Commands[] =
{
    { "create",  _UpnpCreate         },
    { "destroy", _UpnpDestroy        },
    { "ctrl",    _UpnpControl        },
    { "fake",    _UpnpFakeResponse   },
    { "",        NULL                }
};

static UpnpAppT _Upnp_App = { NULL, FALSE };

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _UpnpCreate
    
    \Description
        Upnp subcommand - create upnp module
    
    \Input *pApp    - pointer to upnp module
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 09/27/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _UpnpCreate(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    UpnpAppT *pApp = &_Upnp_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s create\n", argv[0]);
        return;
    }

    pApp->pProtoUpnp = ProtoUpnpCreate();
}

/*F********************************************************************************/
/*!
    \Function _UpnpDestroy
    
    \Description
        Upnp subcommand - destroy upnp module
    
    \Input *pApp    - pointer to upnp module
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 09/27/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _UpnpDestroy(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    UpnpAppT *pApp = &_Upnp_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s destroy\n", argv[0]);
        return;
    }

    ProtoUpnpDestroy(pApp->pProtoUpnp);
}

/*F********************************************************************************/
/*!
    \Function _UpnpControl
    
    \Description
        Upnp subcommand - execute a UPnP command
    
    \Input *pApp    - pointer to upnp module
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 09/27/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _UpnpControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    UpnpAppT *pApp = &_Upnp_App;
    int32_t iCmd, iValue=0, iValue2=0;
    const char *pValue=NULL;

    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("   usage: %s command <command>\n", argv[0]);
        return;
    }

    iCmd  = argv[2][0] << 24;
    iCmd |= argv[2][1] << 16;
    iCmd |= argv[2][2] << 8;
    iCmd |= argv[2][3];
    
    if (argc == 4)
    {
        if (iCmd == 'macr')
        {
            iValue  = argv[3][0] << 24;
            iValue |= argv[3][1] << 16;
            iValue |= argv[3][2] << 8;
            iValue |= argv[3][3];
        }
        else if (iCmd == 'gvar')
        {
            pValue = argv[3];
        }
        else
        {
            iValue = (int32_t)strtol(argv[3], NULL, 10);
        }
    }

    ZPrintf("upnp: executing ProtoUpnpControl(pProtoUpnp, '%s', %d, %d, %s)\n", argv[2], iValue, iValue2, pValue ? pValue : "(null)");
    ProtoUpnpControl(pApp->pProtoUpnp, iCmd, iValue, iValue2, pValue);
}

/*F********************************************************************************/
/*!
    \Function _UpnpFakeResponse
    
    \Description
        Upnp subcommand - fake a upnp response
    
    \Input *pApp    - pointer to upnp module
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 09/27/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _UpnpFakeResponse(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    UpnpAppT *pApp = &_Upnp_App;
    char *pResponseFile;
    int32_t iCmd, iFileSize;

    if ((bHelp == TRUE) || (argc != 4))
    {
        ZPrintf("   usage: %s fake <cmd> <responsefile>\n", argv[0]);
        return;
    }

    // assemble command
    iCmd  = argv[2][0] << 24;
    iCmd |= argv[2][1] << 16;
    iCmd |= argv[2][2] << 8;
    iCmd |= argv[2][3];

    // load response file
    if ((pResponseFile = ZFileLoad(argv[3], &iFileSize, ZFILE_OPENFLAG_RDONLY)) == NULL)
    {
        ZPrintf("%s: unable to open response file '%s'\n", argv[3]);
        return;
    }

    // fake the command
    ProtoUpnpControl(pApp->pProtoUpnp, 'fake', iCmd, 0, pResponseFile);

    // unload the file
    ZMemFree(pResponseFile);
}

/*F********************************************************************************/
/*!
    \Function _CmdUpnpCb

    \Description
        Upnp callback, called after command has been issued.

    \Input *argz    - pointer to context
    \Input argc     - number of command-line arguments
    \Input *argv[]   - command-line argument list
    
    \Output
        int32_t         - result of zcallback, or zero to terminate

    \Version 03/23/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdUpnpCb(ZContext *argz, int32_t argc, char *argv[])
{
    UpnpAppT *pApp = &_Upnp_App;

    // check for kill
    if (argc == 0)
    {
        ZPrintf("%s: killed\n", argv[0]);
        ProtoUpnpDestroy(pApp->pProtoUpnp);
        return(0);
    }

    // update module
    ProtoUpnpUpdate(pApp->pProtoUpnp);

    // keep recurring
    return(ZCallback(&_CmdUpnpCb, 17));
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdUpnp

    \Description
        Upnp command.  This command starts the ProtoUpnp module.

    \Input *argz    - pointer to context
    \Input argc     - number of command-line arguments
    \Input *argv[]  - command-line argument list
    
    \Output
        int32_t     - result of zcallback

    \Version 03/23/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdUpnp(ZContext *argz, int32_t argc, char *argv[])
{
    T2SubCmdT *pCmd;
    UpnpAppT *pApp = &_Upnp_App;
    unsigned char bHelp;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_Upnp_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   test the protoupnp module\n");
        T2SubCmdUsage(argv[0], _Upnp_Commands);
        return(0);
    }

    // if no ref yet, make one
    if ((pCmd->pFunc != _UpnpCreate) && (pApp->pProtoUpnp == NULL))
    {
        char *pCreate = "create";
        ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
        _UpnpCreate(pApp, 1, &pCreate, bHelp);
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    // one-time install of periodic callback
    if (pApp->bZCallback == FALSE)
    {
        pApp->bZCallback = TRUE;
        return(ZCallback(_CmdUpnpCb, 17));
    }
    else
    {
        return(0);
    }
}
