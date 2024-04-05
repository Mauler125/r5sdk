/*H********************************************************************************/
/*!
    \File testercomm.c

    \Description
        This module provides a communication layer between the host and the client.  
        Typical operations are SendLine() and GetLine(), which send and receive 
        lines of text, commands, debug output, etc.  Each platform will implement 
        its own way of communicating – through files, debugger API calls, etc.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/23/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>
#include "DirtySDK/dirtysock.h"
#include "libsample/zmem.h"
#include "libsample/zlib.h"
#include "libsample/zlist.h"
#include "testerregistry.h"
#include "testercomm.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function TesterCommCreate

    \Description
        Create a tester host client communication module.

    \Input None
    
    \Output TesterCommT * - pointer to allocated module

    \Version 03/23/2005 (jfrank)
*/
/********************************************************************************F*/
TesterCommT *TesterCommCreate(void)
{
    TesterCommT *pState;

    // create the module
    pState = (TesterCommT *)ZMemAlloc(sizeof(TesterCommT));
    ds_memclr((void *)pState, sizeof(TesterCommT));
    pState->pInterface = (TesterCommInterfaceT *)ZMemAlloc(sizeof(TesterCommInterfaceT));
    ds_memclr((void *)pState->pInterface, sizeof(TesterCommInterfaceT));

    // create the lists
    pState->pInputData  = ZListCreate(TESTERCOMM_NUMCOMMANDS_MAX, sizeof(TesterCommDataT));
    pState->pOutputData = ZListCreate(TESTERCOMM_NUMCOMMANDS_MAX, sizeof(TesterCommDataT));

    // add this module to the registry
    TesterRegistrySetPointer("COMM", pState);

    // return the pointer
    return(pState);
}


/*F********************************************************************************/
/*!
    \Function TesterCommMessage

    \Description
        Send a message to the other side

    \Input *pState   - module state
    \Input  iMsgType - type of message to send (TESTER_MSGTYPE_XXX)
    \Input *pMsgText - text of the message to send

    \Output int32_t - 0 for success, error code otherwise

    \Version 03/28/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterCommMessage(TesterCommT *pState, int32_t iMsgType, const char *pMsgText)
{
    // check for error conditions
    if ((pState == NULL) || 
        (iMsgType <= 0) || 
        (iMsgType >= TESTER_MSGTYPE_MAX) || 
        (pMsgText == NULL))
    {
        return(-1);
    }

    ds_strnzcpy(pState->Message.strBuffer, pMsgText, sizeof(pState->Message.strBuffer));
    pState->Message.iType = iMsgType;
    return(ZListPushBack(pState->pOutputData, &pState->Message));
}


/*F********************************************************************************/
/*!
    \Function TesterCommRegister

    \Description
        Register a callback for a particular type of message

    \Input *pState    - module state
    \Input  iMsgType  - type of message register for (TESTER_MSGTYPE_XXX)
    \Input *pCallback - callback to call when the message comes in
    \Input *pParam    - user pointer to pass to the callback

    \Output int32_t - 0 for success, error code otherwise

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterCommRegister(TesterCommT *pState, int32_t iMsgType, TesterCommMsgCbT *pCallback, void *pParam)
{
    // check error conditions
    if ((pState == NULL) || 
        (iMsgType <= 0) || 
        (iMsgType >= TESTER_MSGTYPE_MAX))
    {
        return(-1);
    }

    // now register the callback
    pState->MessageMap[iMsgType] = pCallback;
    pState->pMessageMapUserData[iMsgType] = pParam;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function TesterCommStatus

    \Description
        Get module status.

    \Input *pState  - module state
    \Input iSelect  - status selector
    \Input iValue   - selector specific

    \Output int32_t - selector specific

    \Version 10/31/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t TesterCommStatus(TesterCommT *pState, int32_t iSelect, int32_t iValue)
{
    if (iSelect == 'inpt')
    {
        return(pState->bGotInput);
    }
    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function TesterCommSuspend

    \Description
        Suspend the comm module.  Service the files but don't
        execute any commands until the Wake function is called,

    \Input *pState - pointer to host client comm module
    
    \Output None

    \Version 04/07/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterCommSuspend(TesterCommT *pState)
{
    if(pState)
        pState->uSuspended = 1;
}


/*F********************************************************************************/
/*!
    \Function TesterCommWake

    \Description
        Wake the comm module to begin processing commands again.

    \Input *pState - pointer to host client comm module
    
    \Output None

    \Version 04/07/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterCommWake(TesterCommT *pState)
{
    if(pState)
        pState->uSuspended = 0;
}


/*F********************************************************************************/
/*!
    \Function TesterCommDestroy

    \Description
        Destroy a tester host client communication module.

    \Input *pState - pointer to host client comm module
    
    \Output None

    \Version 03/23/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterCommDestroy(TesterCommT *pState)
{
    if (pState)
    {
        // disconnect first, just in case
        TesterCommDisconnect(pState);
        ZListDestroy(pState->pInputData);
        ZListDestroy(pState->pOutputData);

        // delete this item from the registry
        TesterRegistrySetPointer("COMM", NULL);

        // now dump the memory
        ZMemFree(pState->pInterface->pData);
        ZMemFree(pState->pInterface);
        ZMemFree(pState);
    }
}


/*F********************************************************************************/
/*!
    \Function TesterCommConnect

    \Description
        Connect the host client communication module.

    \Input *pState              - pointer to host client comm module
    \Input *pParams             - startup parameters
    \Input bIsHost              - TRUE=host, FALSE=client
    
    \Output int32_t - 0 for success, error code otherwise

    \Version 05/02/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterCommConnect(TesterCommT *pState, const char *pParams, uint32_t bIsHost)
{
    // check for errors
    if (pState == NULL)
        return(-1);
    if (pState->pInterface == NULL)
        return(-1);
    if (pState->pInterface->CommConnectFunc == NULL)
        return(-1);

    return(pState->pInterface->CommConnectFunc(pState, pParams, bIsHost));
}

/*F********************************************************************************/
/*!
    \Function TesterCommUpdate

    \Description
        Give the host/client interface module some processor time.  Call this
        once in a while to pump the input and output pipes.

    \Input *pState - module state

    \Output    int32_t - 0 for success, error code otherwise

    \Version 05/02/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterCommUpdate(TesterCommT *pState)
{
    // check for errors
    if (pState == NULL)
        return(-1);
    if (pState->pInterface == NULL)
        return(-1);
    if (pState->pInterface->CommUpdateFunc == NULL)
        return(-1);

    return(pState->pInterface->CommUpdateFunc(pState));
}


/*F********************************************************************************/
/*!
    \Function TesterCommDisconnect

    \Description
        Disconnect the host client communication module.

    \Input *pState - pointer to host client comm module
    
    \Output int32_t - 0=success, error code otherwise

    \Version 05/02/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterCommDisconnect(TesterCommT *pState)
{
    // check for errors
    if (pState == NULL)
        return(-1);
    if (pState->pInterface == NULL)
        return(-1);
    if (pState->pInterface->CommDisconnectFunc == NULL)
        return(-1);

    return(pState->pInterface->CommDisconnectFunc(pState));
}

