/*H********************************************************************************/
/*!
    \File testerhistory.h

    \Description
        Maintains command history for a particular user.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 04/05/2005 (jfrank) First Version
*/
/********************************************************************************H*/

#ifndef _testerhistory_h
#define _testerhistory_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

#define TESTERHISTORY_ERROR_NONE             (0)    //!< no error
#define TESTERHISTORY_ERROR_NULLPOINTER     (-1)    //!< null pointer sent (invalid)
#define TESTERHISTORY_ERROR_NOSUCHENTRY     (-2)    //!< entry requested doesn't exist

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct TesterHistoryT TesterHistoryT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create a tester history module
TesterHistoryT *TesterHistoryCreate(int32_t iSize);

// get an entry in the history
const char *TesterHistoryGet(TesterHistoryT *pState, int32_t iNum, char *pBuf, int32_t iSize);

// add an entry to the history
int32_t TesterHistoryAdd(TesterHistoryT *pState, const char *pBuf);

// return the head and tail history numbers
int32_t TesterHistoryHeadTailCount(TesterHistoryT *pState, int32_t *pHeadNum, int32_t *pTailNum);

// save historical commands to a file
int32_t TesterHistorySave(TesterHistoryT *pState, const char *pFilename);

// load historical commands from a file
int32_t TesterHistoryLoad(TesterHistoryT *pState, const char *pFilename);

// destroy a tester history object
void TesterHistoryDestroy(TesterHistoryT *pState);

#ifdef __cplusplus
};
#endif

#endif // _testerhistory_h

