/*H********************************************************************************/
/*!
    \File testerconsole.h

    \Description
        This module buffers console output for the tester application.
        In essense, it is just a large FIFO which knows how to handle
        newline characters as expected.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 09/15/1999 (gschaefer) First Version
    \Version 11/08/1999 (gschaefer) Cleanup and revision
    \Version 03/29/2005 (jfrank)    Update for Tester2
*/
/********************************************************************************H*/

#ifndef _testerconsole_h
#define _testerconsole_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

// module state
typedef struct TesterConsoleT TesterConsoleT;

// display callback
typedef void (TesterConsoleDisplayCbT)(const char *pBuf, int32_t iLen, int32_t iRefcon, void *pRefptr);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// Create instance of a console buffer.
TesterConsoleT *TesterConsoleCreate(int32_t iSize, int32_t iWrap);

// Connect a console to a particular ref set.
void TesterConsoleConnect(TesterConsoleT *pRef, int32_t iRefcon, void *pRefptr);

// Clear the console
void TesterConsoleClear(TesterConsoleT *pRef);

// Add text to console buffer.
void TesterConsoleOutput(TesterConsoleT *pRef, int32_t iMsgType, const char *pText);

// Flush buffer data to output handler (calls output handler)
void TesterConsoleFlush(TesterConsoleT *pRef, TesterConsoleDisplayCbT *pProc);

// Release resources and destroy console module.
void TesterConsoleDestroy(TesterConsoleT *pRef);

#ifdef __cplusplus
};
#endif

#endif // _testerconsole_h



