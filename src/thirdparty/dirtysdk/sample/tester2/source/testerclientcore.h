/*H********************************************************************************/
/*!
    \File testerclientcore.h

    \Description
        Main control module for the Tester2 Client application.

    \Notes
        This is the main host module for the tester2 client application.  
        It contains mostly global-variable type objects and operations, similar 
        to LobbyAPI. TesterClientCore is responsible for starting up all the 
        necessary child modules like TesterConsole, TesterHostClientComm, etc.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/17/2005 (jfrank) First Version
*/
/********************************************************************************H*/
#ifndef _testerclientcore_h
#define _testerclientcore_h

/*** Include files ****************************************************************/

#include "testerconsole.h"
#include "testerprofile.h"

/*** Defines **********************************************************************/

#define TESTERCLIENTCORE_ERROR_NONE              (0)    //!< no error (success)
#define TESTERCLIENTCORE_ERROR_NULLPOINTER      (-1)    //!< a null pointer ref was used

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

// module state object
typedef struct TesterClientCoreT TesterClientCoreT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// -------------------- FRAMEWORK --------------------------------

// create a testerclientcore module
TesterClientCoreT *TesterClientCoreCreate(char* strCmdLine);

// connect the core module
int32_t TesterClientCoreConnect(TesterClientCoreT *pState, const char *pParams);

// register function to flush output to the display
void TesterClientCoreDisplayFunc(TesterClientCoreT *pState, 
                                 TesterConsoleDisplayCbT *pProc, 
                                 int32_t iRefcon, void *pRefptr);

// idle function - pump this to make networking, etc. happy
void TesterClientCoreIdle(TesterClientCoreT *pState);

// register a module
int32_t TesterClientCoreRegister(TesterClientCoreT *pState, const char *pCommand, ZCommand *pFunctionPtr);

// disconnect the core module
int32_t TesterClientCoreDisconnect(TesterClientCoreT *pState);

// destroy a testerclientcore module
void TesterClientCoreDestroy(TesterClientCoreT *pState);

// get a historical command
const char *TesterClientCoreGetHistoricalCommand(TesterClientCoreT *pState, int32_t *pOffsetFromCurrent, char *pBuf, int32_t iSize);

// -------------------- COMMUNICATION FUNCTIONS ------------------

// send a command (from GUI command line)
int32_t TesterClientCoreSendCommand(TesterClientCoreT *pState, const char *pData);

// -------------------- PROFILE FUNCTIONS ------------------------

// return a tagfield with a specified profile's data
int32_t TesterClientCoreProfileGet(TesterClientCoreT *pState, int32_t iIndex, TesterProfileEntryT *pDest);

// add a profile
int32_t TesterClientCoreProfileAdd(TesterClientCoreT *pState, TesterProfileEntryT *pEntry);

// delete a profile
int32_t TesterClientCoreProfileDelete(TesterClientCoreT *pState, const char *pName);

// set a profile as the default
int32_t TesterClientCoreProfileDefault(TesterClientCoreT *pState, const char *pName);

#ifdef __cplusplus
};
#endif

#endif // _testerclientcore_h

