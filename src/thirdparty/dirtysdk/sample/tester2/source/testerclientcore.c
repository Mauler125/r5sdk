/*H********************************************************************************/
/*!
    \File testerclientcore.c

    \Description
        Main control module for the Tester2 Client application.

    \Notes
        This is the main host module for the tester2 client application.
        It contains mostly global-variable type objects and operations, similar
        to LobbyAPI. TesterClientCore is responsible for starting up all the
        necessary child modules like TesterConsole, TesterComm, etc.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/17/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>
#include <string.h>
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/util/jsonparse.h"

#include "libsample/zmem.h"
#include "libsample/zfile.h"

#include "testerprofile.h"
#include "testercomm.h"
#include "testerconsole.h"
#include "testerhistory.h"
#include "testermodules.h"
#include "testerregistry.h"
#include "testerclientcore.h"

/*** Defines **********************************************************************/

#define TESTERCLIENTCORE_ROOTPATH_DEFAULT       ".\\T2Client"
#define TESTERCLIENTCORE_PROFILEFILE_DEFAULT    "profiles.txt"
#define TESTERCLIENTCORE_CONSOLESIZE_DEFAULT    16384

/*** Type Definitions *************************************************************/

struct TesterClientCoreT
{
    TesterProfileT              *pProfile;          //!< profile manager module
    TesterCommT                 *pComm;             //!< host/client communication module
    TesterConsoleT              *pConsole;          //!< console for managing output
    TesterHistoryT              *pHistory;          //!< command history module
    TesterModulesT              *pModules;          //!< client side command modules
    TesterProfileEntryT         CmdLineProfile;     //!< the command line as passed
    int iLogFile;                                   //!< Log File identifier
    TesterConsoleDisplayCbT     *pDisplayProc;      //!< procedure for displaying output
};

/*** Variables ********************************************************************/

/*** External Functions ***********************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _TesterClientCoreMsgControl

    \Description
        Handle incoming messages from TesterComm

    \Input *pState - Module state
    \Input *pMsg   - Message text
    \Input *pParam - user supplied data

    \Output None

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
static void _TesterClientCoreMsgControl(TesterCommT *pState, const char *pMsg, void *pParam)
{
    TesterClientCoreT *pCore = (TesterClientCoreT *)pParam;
    TesterConsoleOutput(pCore->pConsole, TESTER_MSGTYPE_CONTROL, pMsg);
}

/*F********************************************************************************/
/*!
    \Function _TesterClientCoreMsgCommand

    \Description
        Handle incoming messages from TesterComm

    \Input *pState - Module state
    \Input *pMsg   - Message text
    \Input *pParam - user supplied data

    \Output None

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
static void _TesterClientCoreMsgCommand(TesterCommT *pState, const char *pMsg, void *pParam)
{
    TesterClientCoreT *pCore = (TesterClientCoreT *)pParam;
    TesterConsoleOutput(pCore->pConsole, TESTER_MSGTYPE_COMMAND, "> ");
    TesterConsoleOutput(pCore->pConsole, TESTER_MSGTYPE_COMMAND, pMsg);
    TesterConsoleOutput(pCore->pConsole, TESTER_MSGTYPE_COMMAND, "\n");
}

/*F********************************************************************************/
/*!
    \Function _TesterClientCoreMsgStatus

    \Description
        Handle incoming messages from TesterComm

    \Input *pState - Module state
    \Input *pMsg   - Message text
    \Input *pParam - user supplied data

    \Output None

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
static void _TesterClientCoreMsgStatus(TesterCommT *pState, const char *pMsg, void *pParam)
{
    //TesterClientCoreT *pCore = (TesterClientCoreT *)pParam;
    //TesterConsoleOutput(pCore->pConsole, TESTER_MSGTYPE_STATUS, pMsg);
}

/*F********************************************************************************/
/*!
    \Function _TesterClientCoreMsgConsole

    \Description
        Handle incoming messages from TesterComm

    \Input *pState - Module state
    \Input *pMsg   - Message text
    \Input *pParam - user supplied data

    \Output None

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
static void _TesterClientCoreMsgConsole(TesterCommT *pState, const char *pMsg, void *pParam)
{
    TesterClientCoreT *pCore = (TesterClientCoreT *)pParam;
    TesterConsoleOutput(pCore->pConsole, TESTER_MSGTYPE_CONSOLE, pMsg);
}


/*F********************************************************************************/
/*!
    \Function _TesterClientCorePrintf

    \Description
        Tester Printf function.

    \Input *pParm  - Module state
    \Input *pText  - Message text

    \Output 0=success

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
#if DIRTYCODE_LOGGING
static int32_t _TesterClientCorePrintf(void *pParm, const char *pText)
{
    TesterClientCoreT *pCore = pParm;
    TesterConsoleOutput(pCore->pConsole, TESTER_MSGTYPE_CONSOLE, pText);
    return(0);
}
#endif


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function TesterClientCoreCreate

    \Description
        Create a TesterClientCore module and return the pointer to it

    \Input None

    \Output TesterClientCoreT * - newly allocated and created TesterClientCoreT

    \Version 03/21/2005 (jfrank)
*/
/********************************************************************************F*/
TesterClientCoreT *TesterClientCoreCreate(char *strCmdLine)
{
    TesterClientCoreT *pState;

    // create and wipe clean the module
    pState = (TesterClientCoreT *)ZMemAlloc(sizeof(TesterClientCoreT));
    ds_memclr((void *)pState, sizeof(TesterClientCoreT));

    // start up the registry
    TesterRegistryCreate(-1);
    TesterRegistrySetPointer("CORE", pState);
    TesterRegistrySetString("ROOT", TESTERCLIENTCORE_ROOTPATH_DEFAULT);

    // now create the children modules
    pState->pProfile    = TesterProfileCreate();
    pState->pComm       = TesterCommCreate();
    pState->pConsole    = TesterConsoleCreate(TESTERCLIENTCORE_CONSOLESIZE_DEFAULT, TRUE);
    pState->pModules    = TesterModulesCreate();
    pState->pHistory    = TesterHistoryCreate(-1);

#if DIRTYCODE_LOGGING
    // hook in the netprintf to the console
    NetPrintfHook(_TesterClientCorePrintf, pState);
#endif

    // start up profile earlier so we can get data
    TesterProfileConnect(pState->pProfile, TESTERCLIENTCORE_ROOTPATH_DEFAULT "\\" TESTERCLIENTCORE_PROFILEFILE_DEFAULT);

    // register client commands
    TesterModulesRegisterClientCommands(pState->pModules);

    // done
    return(pState);
}

/*F********************************************************************************/
/*!
    \Function TesterClientCoreConnect

    \Description
        Connect the core module and all its children

    \Input *pState              - TesterClientCoreT module to connect
    \Input *pParams             - startup parameters

    \Output int32_t - 0=success, error code otherwise

    \Version 03/21/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterClientCoreConnect(TesterClientCoreT *pState, const char *pParams)
{
    TesterProfileEntryT CurProfile;
    char strHostname[64], strProfilename[64], strMessage[128], strControlDir[2];
    uint16_t aJson[512];

    ds_memclr(&CurProfile, sizeof(TesterProfileEntryT));

    // check the state
    if (pState == NULL)
    {
        return(TESTERCLIENTCORE_ERROR_NULLPOINTER);
    }
    // check the children
    if ((pState->pComm == NULL) || (pState->pProfile == NULL))
    {
        return(TESTERCLIENTCORE_ERROR_NULLPOINTER);
    }

    JsonParse(aJson, sizeof(aJson)/sizeof(*aJson), pParams, -1);

    // attach an interface method
    JsonGetString(JsonFind(aJson, "CONTROLDIR"), strControlDir, sizeof(strControlDir), "");
    if (strControlDir[0] != '\0')
    {
        TesterCommAttachFile(pState->pComm);
    }
    else
    {
        TesterCommAttachSocket(pState->pComm);
    }

    // connect the children modules
    TesterCommConnect(pState->pComm, pParams, FALSE);

    // register callbacks for all the types of messages we'll see
    TesterCommRegister(pState->pComm, TESTER_MSGTYPE_CONTROL, _TesterClientCoreMsgControl, (void *)pState);
    TesterCommRegister(pState->pComm, TESTER_MSGTYPE_COMMAND, _TesterClientCoreMsgCommand, (void *)pState);
    TesterCommRegister(pState->pComm, TESTER_MSGTYPE_STATUS,  _TesterClientCoreMsgStatus , (void *)pState);
    TesterCommRegister(pState->pComm, TESTER_MSGTYPE_CONSOLE, _TesterClientCoreMsgConsole, (void *)pState);

    // send connected message to the host
    JsonGetString(JsonFind(aJson, "HOSTNAME"), strHostname, sizeof(strHostname), "");
    JsonGetString(JsonFind(aJson, "PROFILENAME"), strProfilename, sizeof(strProfilename), "");
    ds_snzprintf(strMessage, sizeof(strMessage), "t2client: client '%s' connected to host %s\n", strProfilename, strHostname);
    TesterCommMessage(pState->pComm, TESTER_MSGTYPE_CONSOLE, strMessage);

    // load historical commands
    TesterProfileGet(pState->pProfile, JsonGetInteger(JsonFind(aJson, "PROFILENUM"), -2), &CurProfile);
    TesterHistoryLoad(pState->pHistory, CurProfile.strHistoryFile);

    // now just wait for something
    return(TESTERCLIENTCORE_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function TesterClientCoreIdle

    \Description
        Idle function - pump this to make networking, etc. happy.

    \Input *pState - TesterClientCoreT module to service

    \Output None

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterClientCoreIdle(TesterClientCoreT *pState)
{
    if(pState == NULL)
        return;

    if(pState->pComm)
    {
        // pump the comm update function
        TesterCommUpdate(pState->pComm);
    }

    // now flush any debug output
    if((pState->pDisplayProc) && (pState->pConsole))
    {
        TesterConsoleFlush(pState->pConsole, pState->pDisplayProc);
    }
}


/*F********************************************************************************/
/*!
    \Function TesterClientCoreSendCommand

    \Description
        Send a command to the host - should comes from the client GUI command line.

    \Input *pState - TesterClientCoreT module
    \Input *pData  - command to send

    \Output 0=success, error code otherwise

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterClientCoreSendCommand(TesterClientCoreT *pState, const char *pData)
{
    int32_t iResult;

    // echo it locally
    _TesterClientCoreMsgCommand(pState->pComm, pData, pState);

    // add it to the history
    TesterHistoryAdd(pState->pHistory, pData);

    // check to see if its a local command
    iResult = TesterModulesDispatch(pState->pModules, pData);

    // did it work locally?
    if(iResult == 0)
    {
        return(TESTERCLIENTCORE_ERROR_NONE);
    }
    // not a local command - send it to the host to deal with
    else
    {
        return(TesterCommMessage(pState->pComm, TESTER_MSGTYPE_COMMAND, pData));
    }
}

/*F********************************************************************************/
/*!
    \Function TesterClientCoreProfileGet

    \Description
        Return a tagfield with a specified profile's data

    \Input *pState - TesterClientCoreT module state
    \Input  iIndex - 0-based index of the profile to get, -1 for default entry
    \Input *pDest  - destination entry for the profile

    \Output int32_t - 0=success, error code otherwise

    \Version 03/21/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterClientCoreProfileGet(TesterClientCoreT *pState, int32_t iIndex, TesterProfileEntryT *pDest)
{
    // check for error conditions
    if ((pState == NULL) || (pDest == NULL))
        return(TESTERCLIENTCORE_ERROR_NULLPOINTER);

    // get the specified profile
    return(TesterProfileGet(pState->pProfile, iIndex, pDest));
}

/*F********************************************************************************/
/*!
    \Function TesterClientCoreRegister

    \Description
        Register a command

    \Input *pState       - TesterClientCoreT module state
    \Input *pCommand     - command name to register with
    \Input *pFunctionPtr - the ZCommand module to call

    \Output int32_t - 0=success, error code otherwise

    \Version 03/21/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterClientCoreRegister(TesterClientCoreT *pState, const char *pCommand, ZCommand *pFunctionPtr)
{
    if(pState == NULL)
        return(TESTERCLIENTCORE_ERROR_NULLPOINTER);

    return(TesterModulesRegister(pState->pModules, pCommand, pFunctionPtr));
}


/*F********************************************************************************/
/*!
    \Function TesterClientCoreProfileAdd

    \Description
        Add a profile to the list

    \Input *pState          - module state
    \Input *pEntry          - entry to add

    \Output int32_t - 0 for success, error code otherwise

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterClientCoreProfileAdd(TesterClientCoreT *pState, TesterProfileEntryT *pEntry)
{
    int32_t iResult;

    // add the profile
    iResult = TesterProfileAdd(pState->pProfile, pEntry);
    if(iResult != 0)
        return(iResult);

    // trigger a save automatically
    iResult = TesterProfileSave(pState->pProfile);
    return(iResult);
}


/*F********************************************************************************/
/*!
    \Function TesterClientCoreProfileDelete

    \Description
        Remove a profile from the list

    \Input *pState       - module state
    \Input *pName        - profile to nuke

    \Output int32_t - 0 for success, error code otherwise

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterClientCoreProfileDelete(TesterClientCoreT *pState, const char *pName)
{
    return(TesterProfileDelete(pState->pProfile, pName));
}


/*F********************************************************************************/
/*!
    \Function TesterClientCoreProfileDefault

    \Description
        Remove a profile from the list

    \Input *pState       - module state
    \Input *pName        - profile to set as default

    \Output int32_t - 0 for success, error code otherwise

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterClientCoreProfileDefault(TesterClientCoreT *pState, const char *pName)
{
    return(TesterProfileSetDefaultName(pState->pProfile, pName));
}

/*F********************************************************************************/
/*!
    \Function TesterClientCoreDisplayFunc

    \Description
        Register a display function to show stuff on the screen.

    \Input *pState      - TesterClientCoreT module state
    \Input *pProc       - function pointer to display procedure
    \Input iRefcon      - int32_t value to pass to the display function when called
    \Input *pDisplayRef - ref value to pass to the display function when called (hDlg)

    \Output None

    \Version 03/28/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterClientCoreDisplayFunc(TesterClientCoreT *pState,  TesterConsoleDisplayCbT *pProc, int32_t iRefcon, void *pRefptr)
{
    pState->pDisplayProc = pProc;
    TesterConsoleConnect(pState->pConsole, iRefcon, pRefptr);
}


/*F********************************************************************************/
/*!
    \Function TesterClientCoreGetHistoricalCommand

    \Description
        Get a historical command from the history module.

    \Input *pState              - TesterClientCoreT module state
    \Input *pOffsetFromCurrent  - pointer to the offset, may be clamped if necessary
    \Input *pBuf                - destination buffer (may be NULL)
    \Input  iSize               - size of destination buffer

    \Output
        const char *            - history text, or null

    \Version 04/06/2005 (jfrank)
*/
/********************************************************************************F*/
const char *TesterClientCoreGetHistoricalCommand(TesterClientCoreT *pState, int32_t *pOffsetFromCurrent, char *pBuf, int32_t iSize)
{
    int32_t iHeadCount, iTailCount;
    int32_t iOffsetFromCurrent;
    const char *pText = NULL;
    
    // check for errors
    if (pState == NULL)
    {
        return(NULL);
    }

    // get the current head and tail
    TesterHistoryHeadTailCount(pState->pHistory, &iHeadCount, &iTailCount);

    // clamp the incoming value
    iOffsetFromCurrent = *pOffsetFromCurrent;
    if (iOffsetFromCurrent > 1)
    {
        iOffsetFromCurrent = 1;
    }
    if ((iHeadCount - iTailCount) > iOffsetFromCurrent)
    {
        iOffsetFromCurrent = (iHeadCount - iTailCount);
    }
    *pOffsetFromCurrent = iOffsetFromCurrent;

    // get the command
    if ((iOffsetFromCurrent == 1) && (pBuf != NULL))
    {
        // set a blank entry
        ds_memclr(pBuf, iSize);
    }
    else
    {
        pText = TesterHistoryGet(pState->pHistory, (iTailCount + iOffsetFromCurrent), pBuf, iSize);
    }
    return(pText);
}


/*F********************************************************************************/
/*!
    \Function TesterClientCoreDisconnect

    \Description
        Disconnect the core module and all its children

    \Input *pState - TesterClientCoreT module to disconnect

    \Output int32_t - 0=success, error code otherwise

    \Version 03/21/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterClientCoreDisconnect(TesterClientCoreT *pState)
{
    // check the state
    if (pState == NULL)
        return(TESTERCLIENTCORE_ERROR_NULLPOINTER);
    // check the children
    if ((pState->pComm == NULL) || (pState->pProfile == NULL))
        return(TESTERCLIENTCORE_ERROR_NULLPOINTER);

    // disconnect the children modules
    TesterCommDisconnect(pState->pComm);

    return(TESTERCLIENTCORE_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function TesterClientCoreDestroy

    \Description
        Destroy a TesterClientCoreT module and all its children

    \Input *pState - TesterClientCoreT module to destroy

    \Output None

    \Version 03/21/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterClientCoreDestroy(TesterClientCoreT *pState)
{
    TesterProfileEntryT CurProfile;

    // check the state
    if (pState == NULL)
        return;

    TesterProfileGet(pState->pProfile, -1, &CurProfile);
    TesterHistorySave(pState->pHistory, CurProfile.strHistoryFile);

    // shut down profile late so we can save settings if need be
    TesterProfileDisconnect(pState->pProfile);

    // destoy all the children modules
    TesterProfileDestroy(pState->pProfile);
    TesterCommDestroy(pState->pComm);
    TesterConsoleDestroy(pState->pConsole);
    TesterModulesDestroy(pState->pModules);
    TesterHistoryDestroy(pState->pHistory);
    TesterRegistryDestroy();

#if DIRTYCODE_LOGGING
    // unhook debug output
    NetPrintfHook(NULL, NULL);
#endif

    // wipe the pointers clean
    pState->pComm = NULL;
    pState->pProfile = NULL;
    pState->pConsole = NULL;
    pState->pHistory = NULL;

    // now destroy this module
    ZMemFree((void *)pState);
}

