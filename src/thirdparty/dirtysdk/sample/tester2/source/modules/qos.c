/*H*************************************************************************************/
/*!
    \File    qos.c

    \Description
        Reference application for QosClient.

    \Copyright
        Copyright (c) Electronic Arts 2008.    ALL RIGHTS RESERVED.

    \Version    1.0        05/25/2008 (cadam) First Version
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/proto/protoname.h"
#include "DirtySDK/misc/qosclient.h"

#include "libsample/zlib.h"

#include "testerregistry.h"
#include "testersubcmd.h"
#include "testermodules.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct QosAppT
{
    QosClientRefT *pQosClient;

    unsigned char bZCallback;
} QosAppT;

/*** Function Prototypes ***************************************************************/

static void _QosCreate(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
static void _QosDestroy(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
static void _QosStart(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
static void _QosControl(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
static void _QosStatus(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);

/*** Variables *************************************************************************/

static QosAppT _Qos_App = { NULL, FALSE };

static T2SubCmdT _Qos_Commands[] =
{
    { "create",      _QosCreate          },
    { "destroy",     _QosDestroy         },
    { "start",       _QosStart           },
    { "control",     _QosControl         },
    { "status",      _QosStatus          },
    { "",            NULL                }
};

/*** Private Functions *****************************************************************/


/*F********************************************************************************/
/*!
    \Function _CmdQosUsage

    \Description
        Display usage information.

    \Input argc         - argument count
    \Input *argv[]      - argument list
    
    \Output
        None.

    \Version 04/25/2008 (cadam)
*/
/********************************************************************************F*/
/*static void _CmdQosUsage(int argc, char *argv[])
{
    if (argc == 2)
    {
        ZPrintf("   listen for, request, and/or control and get the status of current requests\n");
        ZPrintf("   usage: %s [create|destroy|listen|request|service|cancel|control|status|nattype]", argv[0]);
    }
    else if (argc == 3)
    {
        if (!strcmp(argv[2], "create"))
        {
            ZPrintf("   usage: %s create [serviceport]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "destroy"))
        {
            ZPrintf("   usage: %s destroy\n", argv[0]);
        }
        else if (!strcmp(argv[2], "listen"))
        {
            ZPrintf("   usage: %s listen [response]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "request"))
        {
            ZPrintf("   usage: %s request [address] [probes]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "service"))
        {
            ZPrintf("   usage: %s service [address] [serviceid] [probes]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "cancel"))
        {
            ZPrintf("   usage: %s cancel [requestid]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "control"))
        {
            ZPrintf("   usage: %s control [control] [value] [pvalue]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "status"))
        {
            ZPrintf("   usage: %s status [select] [data]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "nattype"))
        {
            ZPrintf("   usage: %s nattype\n", argv[0]);
        }
    }
}
*/

/*F*************************************************************************************/
/*!
    \Function _QosCreate
    
    \Description
        Qos subcommand - create qosclient reference
    
    \Input *pCmdRef
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 04/25/2008 (cadam) First Version
*/
/**************************************************************************************F*/
static void _QosCreate(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    QosAppT *pApp = &_Qos_App;

    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("   usage: %s create [serviceport]\n", argv[0]);
        return;
    }

    if (pApp->pQosClient != NULL)
    {
        ZPrintf("   %s: ref has already been created\n", argv[0]);
        return;
    }

    pApp->pQosClient = QosClientCreate(NULL, "", (int32_t)strtol(argv[2], NULL, 10));
}


/*F*************************************************************************************/
/*!
    \Function _QosDestroy
    
    \Description
        Qos subcommand - destroy qosclient reference
    
    \Input *pCmdRef
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 04/25/2008 (cadam) First Version
*/
/**************************************************************************************F*/
static void _QosDestroy(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    QosAppT *pApp = &_Qos_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s destroy\n", argv[0]);
        return;
    }

    QosClientDestroy(pApp->pQosClient);
    pApp->pQosClient = NULL;
}

/*F*************************************************************************************/
/*!
    \Function _QosStart
    
    \Description
        Qos subcommand - issue a QoS service request
    
    \Input *pCmdRef
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 04/25/2008 (cadam) First Version
*/
/**************************************************************************************F*/
static void _QosStart(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    QosAppT *pApp = &_Qos_App;

    if ((bHelp == TRUE) || (argc < 5))
    {
        ZPrintf("   usage: %s service [address] [probeport] [profile name]\n", argv[0]);
        return;
    }

    QosClientStart(pApp->pQosClient, argv[2], (int32_t)strtol(argv[3], NULL, 10), argv[4]);
}

/*F*************************************************************************************/
/*!
    \Function _QosControl
    
    \Description
        Qos subcommand - QoS control function
    
    \Input *pCmdRef
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 04/25/2008 (cadam) First Version
*/
/**************************************************************************************F*/
static void _QosControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    int32_t iCtrl;
    QosAppT *pApp = &_Qos_App;

    if ((bHelp == TRUE) || (argc < 5))
    {
        ZPrintf("   usage: %s control [control] [value] [pvalue]\n", argv[0]);
        return;
    }

    iCtrl  = argv[2][0] << 24;
    iCtrl |= argv[2][1] << 16;
    iCtrl |= argv[2][2] << 8;
    iCtrl |= argv[2][3];

    QosClientControl(pApp->pQosClient, iCtrl, (int32_t)strtol(argv[3], NULL, 10), argv[4]);
}


/*F*************************************************************************************/
/*!
    \Function _QosStatus
    
    \Description
        Qos subcommand - QoS status function
    
    \Input *pCmdRef
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 04/25/2008 (cadam) First Version
*/
/**************************************************************************************F*/
static void _QosStatus(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    int32_t iSelect;
    QosAppT *pApp = &_Qos_App;
    char strBuffer[256];

    if ((bHelp == TRUE) || (argc < 4))
    {
        ZPrintf("   usage: %s status [select] [data]\n", argv[0]);
        return;
    }

    iSelect  = argv[2][0] << 24;
    iSelect |= argv[2][1] << 16;
    iSelect |= argv[2][2] << 8;
    iSelect |= argv[2][3];

    QosClientStatus(pApp->pQosClient, iSelect, (int32_t)strtol(argv[3], NULL, 10), &strBuffer, sizeof(strBuffer));
}

/*F*************************************************************************************/
/*!
    \Function _CmdQosCb
    
    \Description
        Qos idle callback.
    
    \Input *argz    - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list
    
    \Output
        int32_t         - zero
            
    \Version 1.0 04/25/2008 (cadam) First Version
*/
/**************************************************************************************F*/
static int32_t _CmdQosCb(ZContext *argz, int32_t argc, char *argv[])
{
    QosAppT *pApp = &_Qos_App;

    // check for kill
    if (argc == 0)
    {
        ZPrintf("%s: killed\n", argv[0]);
        QosClientDestroy(pApp->pQosClient);
        return(0);
    }

    // update at fastest rate
    return(ZCallback(_CmdQosCb, 1000));
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function    CmdQos
    
    \Description
        QoS command.
    
    \Input *argz    - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list
    
    \Output
        int32_t         - zero
            
    \Version 1.0 04/25/2008 (cadam) First Version
*/
/**************************************************************************************F*/
int32_t CmdQos(ZContext *argz, int32_t argc, char *argv[])
{
    T2SubCmdT *pCmd;
    QosAppT *pApp = &_Qos_App;
    unsigned char bHelp;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_Qos_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   %s: issue a qos request, get the nat information, or get the status of or control a request\n", argv[0]);
        T2SubCmdUsage(argv[0], _Qos_Commands);
        return(0);
    }

    // if no ref yet, make one
    if ((pCmd->pFunc != _QosCreate) && (pApp->pQosClient == NULL))
    {
        char *pCreate = "create 11204";
        ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
        _QosCreate(pApp, 1, &pCreate, bHelp);
        if (pApp->pQosClient == NULL)
        {
            ZPrintf("   %s: error creating module\n", argv[0]);
            return(0);
        }
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    // one-time install of periodic callback
    if (pApp->bZCallback == FALSE)
    {
        pApp->bZCallback = TRUE;
        return(ZCallback(_CmdQosCb, 1000));
    }
    else
    {
        return(0);
    }
}

