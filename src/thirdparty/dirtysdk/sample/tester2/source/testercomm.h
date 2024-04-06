/*H********************************************************************************/
/*!
    \File testercomm.h

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

#ifndef _testercomm_h
#define _testercomm_h

/*** Include files ****************************************************************/

#include "DirtySDK/dirtysock.h"
#include "libsample/zlist.h"

/*** Defines **********************************************************************/

#define TESTER_MSGTYPE_NONE                 (0)     //!< no message type
#define TESTER_MSGTYPE_CONTROL              (1)     //!< control message - for communication between host/client
#define TESTER_MSGTYPE_COMMAND              (2)     //!< command message - execute host command
#define TESTER_MSGTYPE_STATUS               (3)     //!< status message - for status updates between host/client
#define TESTER_MSGTYPE_CONSOLE              (4)     //!< console output - for display on-screen
#define TESTER_MSGTYPE_MAX                  (8)     //!< max number of message types

#define TESTERCOMM_NUMCOMMANDS_MAX          (512)   //!< number of possible input/output commands outstanding
#define TESTERCOMM_COMMANDSIZE_MAX          (256*1024)  //!< max size of each command line
#define TESTERCOMM_ARGNUM_MAX               (16)    //!< max number of arguments for each command line

//! FILE-SPECIFIC defines
#define TESTERCOMM_CLIENTINPUTFILE          ("clientinput.txt")
#define TESTERCOMM_CLIENTOUTPUTFILE         ("clientoutput.txt")
#define TESTERCOMM_HOSTINPUTFILE            (TESTERCOMM_CLIENTOUTPUTFILE)
#define TESTERCOMM_HOSTOUTPUTFILE           (TESTERCOMM_CLIENTINPUTFILE)

//! MSDM-SPECIFIC defines

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

// the comm module itself
typedef struct TesterCommT TesterCommT;

// message type callback
// Callback prototype used for status updates, if enabled
typedef void (TesterCommMsgCbT)(TesterCommT *pState, const char *pMsg, void *pParam);

// data container for messages to send between host and client
typedef struct TesterCommDataT
{
    int32_t iType;                                  //!< message type
    char strBuffer[TESTERCOMM_COMMANDSIZE_MAX];     //!< command data pointer
} TesterCommDataT;

// interface pointers - filled in by a particular interface adapter function
typedef struct TesterCommInterfaceT
{
    //! connection function
    int32_t   (*CommConnectFunc)   (TesterCommT *pState, const char *pParams, uint32_t bIsHost);
    //! update function
    int32_t   (*CommUpdateFunc)    (TesterCommT *pState);
    //! disconnection function
    int32_t   (*CommDisconnectFunc)(TesterCommT *pState);
    void   *pData;                                                           //!< interface-specific data
} TesterCommInterfaceT;

// module state
struct TesterCommT
{
    // function calls and interface specific stuff
    TesterCommInterfaceT *pInterface;                   //!< interface-specific functions

    // communication stuff
    uint32_t uLastConnectTime;                          //!< the last time a connect was attempted
    uint32_t uLastSendTime;                             //!< last time an output message was sent
    TesterCommMsgCbT *MessageMap[TESTER_MSGTYPE_MAX];   //!< message map array
    void *pMessageMapUserData[TESTER_MSGTYPE_MAX];      //!< user-supplied data to call back with
    uint8_t uSuspended;                                 //!< 1=suspended, 0=awake
    uint8_t bGotInput;                                  //!< TRUE=got input from the other side, else FALSE
    uint8_t uPad[2];                                    //!< pad data

    char strCommand[TESTERCOMM_COMMANDSIZE_MAX];        //!< temporary command processing buffer
    char strResponse[TESTERCOMM_COMMANDSIZE_MAX];       //!< temporary response processing buffer (XENON only)
    TesterCommDataT LineData;                           //!< temporary line data buffer
    TesterCommDataT Message;                            //!< temporary message buffer

    // data lists
    ZListT *pInputData;                                 //!< input commands
    ZListT *pOutputData;                                //!< output commands
};


/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create a tester host client communication module
TesterCommT *TesterCommCreate(void);

// connect the host client communication module
int32_t TesterCommConnect(TesterCommT *pState, const char *pParams, uint32_t bIsHost);

// give the host/client interface some processing time (call this once in a while)
int32_t TesterCommUpdate(TesterCommT *pState);

// send a message to the other side
int32_t TesterCommMessage(TesterCommT *pState, int32_t iMsgType, const char *pMsgText);

// register a callback with a particular message type
int32_t TesterCommRegister(TesterCommT *pState, int32_t iMsgType, TesterCommMsgCbT *pCallback, void *pParam);

// get module status
int32_t TesterCommStatus(TesterCommT *pState, int32_t iSelect, int32_t iValue);

// suspend the comm module, active until a wake call is issued
void TesterCommSuspend(TesterCommT *pState);

// wake the comm module from a suspend
void TesterCommWake(TesterCommT *pState);

// disconnect the host client communication module
int32_t TesterCommDisconnect(TesterCommT *pState);

// destroy a tester host client communication module
void TesterCommDestroy(TesterCommT *pState);

// testercomm_file method to attach to host
void TesterCommAttachFile(TesterCommT *pState);

// testercomm_socket method to attach to host
void TesterCommAttachSocket(TesterCommT *pState);

#ifdef __cplusplus
};
#endif

#endif // _testercomm_h

