/*H********************************************************************************/
/*!
    \File testerhostcore.h

    \Description
        Main control module for the Tester2 host application.

    \Notes
        This is the main host module for the tester2 host application.  
        It contains mostly global-variable type objects and operations, similar 
        to LobbyAPI. TesterhostCore is responsible for starting up all the 
        necessary child modules like TesterConsole, TesterHostClientComm, etc.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/17/2005 (jfrank) First Version
*/
/********************************************************************************H*/
#ifndef _testerhostcore_h
#define _testerhostcore_h

/*** Include files ****************************************************************/

#include "libsample/zlib.h"
#include "testerprofile.h"
#include "testerconsole.h"

/*** Defines **********************************************************************/

#define TESTERHOSTCORE_ERROR_NONE                (0)    //!< no error (success)
#define TESTERHOSTCORE_ERROR_NULLPOINTER        (-1)    //!< a null pointer ref was used

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

// module state object
typedef struct TesterHostCoreT TesterHostCoreT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create a testerhostcore module
TesterHostCoreT *TesterHostCoreCreate(const char *pParams);

// connect the core module
int32_t TesterHostCoreConnect(TesterHostCoreT *pState, const char *pNetParams);

// idle function - pump this to make networking, etc. happy
void TesterHostCoreIdle(TesterHostCoreT *pState);

// register function to flush output to the display
void TesterHostCoreDisplayFunc(TesterHostCoreT *pState, TesterConsoleDisplayCbT *pProc, int32_t iRefcon, void *pRefptr);

// dispatch a command from an incoming command line - local version
int32_t TesterHostCoreDispatch(TesterHostCoreT *pCore, const char *pCommandLine);

// get local history info
const char *TesterHostCoreGetHistory(TesterHostCoreT *pState, int32_t iPrevNext, char *pBuf, int32_t iSize);

// update function - pump this to get data flowing, sending a pointer to the core module as data
void TesterHostCoreUpdate(void *pData, uint32_t uTick);

// register a module
int32_t TesterHostCoreRegister(TesterHostCoreT *pState, const char *pCommand, ZCommand *pFunctionPtr);

// start writting log messages in logfile
int32_t TesterHostCoreStartSavingLog(TesterHostCoreT *pState, const char * strLogfileName);

// stop writting log messages in logfile
int32_t TesterHostCoreStopSavingLog(TesterHostCoreT *pState);

// signal for a shutdown of all running tester modules
void TesterHostCoreShutdown(TesterHostCoreT *pState);

// disconnect the core module
int32_t TesterHostCoreDisconnect(TesterHostCoreT *pState);

// destroy a testerhostcore module
void TesterHostCoreDestroy(TesterHostCoreT *pState);

// -------------------- COMMUNICATION FUNCTIONS ------------------

// send status back to the client
int32_t TesterHostCoreSendStatus(TesterHostCoreT *pState, const char *pData);

// send console output back to the client
int32_t TesterHostCoreSendConsole(TesterHostCoreT *pState, const char *pData);

#ifdef __cplusplus
};
#endif

#endif // _testerhostcore_h

