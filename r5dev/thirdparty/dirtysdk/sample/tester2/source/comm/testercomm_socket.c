/*H********************************************************************************/
/*!
    \File testercomm_socket.c

    \Description
        This module provides a communication layer between the host and the client
        using socket I/O.  Typical operations are SendLine() and GetLine(), which
        send and receive lines of text, commands, debug output, etc.  Each platform
        may implement its own way of communicating - through files, debugger API
        calls, etc.

    \Copyright
        Copyright 2011 Electronic Arts Inc.

    \Version 10/17/2011 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/netconn.h"   // for NetConnSleep()
#include "DirtySDK/util/base64.h"
#include "DirtySDK/util/jsonformat.h"
#include "DirtySDK/util/jsonparse.h"
#include "libsample/zmem.h"
#include "libsample/zlib.h"
#include "libsample/zlist.h"
#include "libsample/zfile.h"

#include "testerprofile.h"
#include "testerregistry.h"
#include "testercomm.h"

/*** Defines **********************************************************************/

#define T2COMM_PORT                         (3232) //!< communications port
#define TESTERCOMM_OUTPUTTIMEOUT_DEFAULT    (60*1000) //!< timeout (ms) for a output message

/*** Type Definitions *************************************************************/

//! module state
typedef struct TesterCommSocketT
{
    SocketT *pSocket;       //!< comm socket
    SocketT *pListen;       //!< listen socket used by host
    int32_t iPort;          //!< port to bind to

    char strHostName[TESTERPROFILE_HOSTNAME_SIZEDEFAULT];   //!< host address to connect to
    enum
    {
        ST_INIT,
        ST_CONN,
        ST_LIST,
        ST_OPEN,
        ST_FAIL
    }eState;                    //!< module state
    uint8_t bIsHost;            //!< TRUE if host else FALSE
    char strInputData[16*1024]; //!< input buffer
} TesterCommSocketT;

/*** Private Function Prototypes **************************************************/

static int32_t _TesterCommDisconnect(TesterCommT *pState);
static int32_t _TesterCommCheckInput(TesterCommT *pState);

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _TesterCommConnect2

    \Description
        Connect the host client communication module.

    \Input *pSocketState    - module state

    \Output
        int32_t             - 0=success, else failure

    \Version 10/17/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _TesterCommConnect2(TesterCommSocketT *pSocketState)
{
    struct sockaddr SockAddr;
    uint32_t uHostAddr;
    int32_t iResult;

    // set up sockaddr
    SockaddrInit(&SockAddr, AF_INET);
    SockaddrInSetPort(&SockAddr, pSocketState->iPort);

    // if we have somewhere to connect to, do it
    if (pSocketState->strHostName[0] != '\0')
    {
        // create the socket
        if ((pSocketState->pSocket = SocketOpen(AF_INET, SOCK_STREAM, 0)) == NULL)
        {
            ZPrintf("testercomm_socket: cannot create socket\n");
            pSocketState->eState = ST_FAIL;
            return(-1);
        }

        // check for invalid addr
        if ((uHostAddr = SocketInTextGetAddr(pSocketState->strHostName)) == 0)
        {
            ZPrintf("testercomm_socket: connect got invalid host address %s\n", pSocketState->strHostName);
            pSocketState->eState = ST_FAIL;
            return(-2);
        }

        // initiate connection
        SockaddrInSetAddr(&SockAddr, uHostAddr);
        if ((iResult = SocketConnect(pSocketState->pSocket, &SockAddr, sizeof(SockAddr))) != SOCKERR_NONE)
        {
            ZPrintf("testercomm_socket: error %d initiating connection to %s\n", iResult, pSocketState->strHostName);
            SocketClose(pSocketState->pSocket);
            pSocketState->pSocket = NULL;
            pSocketState->eState = ST_FAIL;
            return(-3);
        }
        pSocketState->eState = ST_CONN;
        ZPrintf("testercomm_socket: connect hostname=%a\n", uHostAddr);
    }
    else
    {
        struct sockaddr BindAddr;

        /* only create the listen socket the first time, if the peer disconnects we will
           just go back into the listen state to accept the new connection */
        if (pSocketState->pListen == NULL)
        {
            // create the socket
            if ((pSocketState->pListen = SocketOpen(AF_INET, SOCK_STREAM, 0)) == NULL)
            {
                ZPrintf("testercomm_socket: cannot create socket\n");
                pSocketState->eState = ST_FAIL;
                return(-4);
            }

            // set SO_LINGER to zero on listen socket so it doesn't linger and cause errors in testing
            SocketControl(pSocketState->pListen, 'soli', 0, NULL, NULL);
            // set to reuse addr incase it is still bound from previous session
            SocketControl(pSocketState->pListen, 'radr', 1, NULL, NULL);

            // bind the socket
            if ((iResult = SocketBind(pSocketState->pListen, &SockAddr, sizeof(SockAddr))) != SOCKERR_NONE)
            {
                ZPrintf("testercomm_socket: error %d binding to port %d\n", iResult, pSocketState->iPort);
                SocketClose(pSocketState->pListen);
                pSocketState->pListen = NULL;
                pSocketState->eState = ST_FAIL;
                ZPrintf("testercomm_socket: next bind will use a random port\n");
                pSocketState->iPort = 0;
                return(-5);
            }
            // listen on the socket
            if ((iResult = SocketListen(pSocketState->pListen, 2)) != SOCKERR_NONE)
            {
                ZPrintf("testercomm_socket: error %d listening on socket\n", iResult);
                SocketClose(pSocketState->pListen);
                pSocketState->pListen = NULL;
                pSocketState->eState = ST_FAIL;
                return(-6);
            }
        }

        pSocketState->eState = ST_LIST;
        SocketInfo(pSocketState->pListen, 'bind', 0, &BindAddr, sizeof(BindAddr));
        ZPrintf("testercomm_socket: waiting for connection on port %d\n", SockaddrInGetPort(&BindAddr));
    }

    // done for now
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _TesterCommDisconnect2

    \Description
        Disconnect the host client communication module.

    \Input *pSocketState    - pointer to host client comm module

    \Output
        int32_t             - 0=success, error code otherwise

    \Version 10/17/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _TesterCommDisconnect2(TesterCommSocketT *pSocketState)
{
    ZPrintf("testercomm_socket: disconnect\n");
    // dispose of socket(s)
    if (pSocketState->pSocket != NULL)
    {
        SocketClose(pSocketState->pSocket);
        pSocketState->pSocket = NULL;
    }

    // return success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _TesterCommCheckConnect

    \Description
        Check to see if a connection has been established

    \Input *pState      - module state

    \Output
        int32_t         - 1=open, 0=connecting, negative=error

    \Version 10/17/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _TesterCommCheckConnect(TesterCommT *pState)
{
    TesterCommSocketT *pSocketState = pState->pInterface->pData;

    // if already open nothing to do
    if (pSocketState->eState == ST_OPEN)
    {
        return(1);
    }

    if (pSocketState->eState == ST_LIST)
    {
        struct sockaddr RequestAddr;
        int32_t iAddrLen = sizeof(RequestAddr);

        // accept incoming connection
        SockaddrInit(&RequestAddr, AF_INET);
        if ((pSocketState->pSocket = SocketAccept(pSocketState->pListen, &RequestAddr, &iAddrLen)) != NULL)
        {
            ZPrintf("testercomm_socket: accepted incoming connection request from %A\n", &RequestAddr);
            pSocketState->eState = ST_OPEN;
        }
    }
    else if (pSocketState->eState == ST_CONN)
    {
        int32_t iResult;

        // waiting for connection to be complete
        if ((iResult = SocketInfo(pSocketState->pSocket, 'stat', 0, NULL, 0)) > 0)
        {
            ZPrintf("testercomm_socket: connection complete\n");
            pSocketState->eState = ST_OPEN;
        }
        else if (iResult < 0)
        {
            pSocketState->eState = ST_FAIL;
        }
    }
    else if (pSocketState->eState == ST_FAIL)
    {
        _TesterCommDisconnect2(pSocketState);

        // dump all the messages
        ZListClear(pState->pOutputData);

        // set that we haven't received any input
        pState->bGotInput = FALSE;

        _TesterCommConnect2(pSocketState);
    }

    if (pSocketState->eState == ST_OPEN)
    {
        // set the tick time so we don't automatically get a timeout
        pState->uLastSendTime = ZTick();
        return(1);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _TesterCommCheckInput

    \Description
        Check for data coming from the other side (host or client) and pull
        any data into our internal buffer, if possible.

    \Input *pState - pointer to host client comm module

    \Output
        int32_t     - 0 = no data, >0 = data, error code otherwise

    \Version 10/17/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _TesterCommCheckInput(TesterCommT *pState)
{
    TesterCommSocketT *pSocketState = pState->pInterface->pData;
    int32_t iBytesRecv = 0, iLinesRecv, iResult;
    const char *pInputLoop;
    uint16_t *pJson;

    // don't do anything if we have already closed the socket
    if (pSocketState->eState != ST_OPEN)
    {
        return(-1);
    }

    // try to get as much data as possible
    while ((iResult = SocketRecv(pSocketState->pSocket, pSocketState->strInputData+iBytesRecv, sizeof(pSocketState->strInputData)-iBytesRecv, 0)) != 0)
    {
        if (iResult > 0)
        {
            iBytesRecv += iResult;
            pState->bGotInput = TRUE;
        }
        else if (iResult < 0)
        {
            // an error occurred
            ZPrintf("testercomm_socket: error %d receiving data\n", iResult);
            pSocketState->eState = ST_FAIL;
            return(-1);
        }
    }
    if (iBytesRecv == 0)
    {
        return(0);
    }

    pJson = JsonParse2(pSocketState->strInputData, -1, 0, 0, NULL);

    // push each line onto the list
    iLinesRecv = 0;
    while ((pInputLoop = JsonFind2(pJson, NULL, "msg[", iLinesRecv)) != NULL)
    {
        char strBuffer[sizeof(pState->LineData.strBuffer)];
        int32_t iBufLen;

        ds_memclr(&pState->LineData, sizeof(pState->LineData));
        pState->LineData.iType = (int32_t)JsonGetInteger(JsonFind2(pJson, pInputLoop, ".TYPE", iLinesRecv), 0);
        iBufLen = JsonGetString(JsonFind2(pJson, pInputLoop, ".TEXT", iLinesRecv), strBuffer, sizeof(strBuffer), "");
        Base64Decode3(strBuffer, iBufLen, pState->LineData.strBuffer, sizeof(pState->LineData.strBuffer));

        // add to the back of the list; remember to add one for the terminator
        ZListPushBack(pState->pInputData, &pState->LineData);

        iLinesRecv += 1;
    }

    ZMemFree(pJson);

    // done - return how much we got from the file
    return(iLinesRecv);
}

/*F********************************************************************************/
/*!
    \Function _TesterCommCheckOutput

    \Description
        Check and send data from the output buffer, if possible.

    \Input *pState  - pointer to host client comm module

    \Output
        int32_t     - 0=success, error code otherwise

    \Version 10/17/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _TesterCommCheckOutput(TesterCommT *pState)
{
    TesterCommSocketT *pSocketState = pState->pInterface->pData;
    int32_t iLineLength, iBytesSent, iResult, iTickDiff;
    char *pWrite, *pBuffer;

    // see if there's any data to send
    if (ZListPeekFront(pState->pOutputData) == NULL)
    {
        // reset the last send time - we're not timing out because there's no data
        pState->uLastSendTime = ZTick();
        return(0);
    }

    // see if we've timed out
    if ((iTickDiff = NetTickDiff(ZTick(), pState->uLastSendTime)) > TESTERCOMM_OUTPUTTIMEOUT_DEFAULT)
    {
        // timeout occurred - dump all pending messages and pop a warning
        ZListClear(pState->pOutputData);
        
        ZPrintf("testercomm_socket: TIMEOUT in sending data (%dms).  List cleared.  Comms Lost?\n", iTickDiff);
        pSocketState->eState = ST_FAIL;
    }

    // push output queue stuff by sending commands
    JsonInit(pState->strCommand, sizeof(pState->strCommand), 0);
    JsonArrayStart(pState->strCommand, "msg");

    while (ZListPeekFront(pState->pOutputData) != NULL)
    {
        char strBuffer[sizeof(pState->LineData.strBuffer)];

        // snag the data into the local buffer
        ZListPopFront(pState->pOutputData, &pState->LineData);

        // create the stuff to write
        JsonObjectStart(pState->strCommand, NULL);
        JsonAddInt(pState->strCommand, "TYPE", pState->LineData.iType);
        Base64Encode2(pState->LineData.strBuffer, (int32_t)strlen(pState->LineData.strBuffer), strBuffer, sizeof(strBuffer));
        if ((iResult = JsonAddStr(pState->strCommand, "TEXT", strBuffer)) == JSON_ERR_FULL)
        {
            // Sometimes, the text we get in will be too big.  Make sure this is an easy error to diagnose.
            ZPrintf("testercomm_socket: text too large to send in TESTERCOMM_COMMANDSIZE_DEFAULT.  Discarding.\n");
        }
        JsonObjectEnd(pState->strCommand);
    }
    JsonArrayEnd(pState->strCommand);
    pBuffer = JsonFinish(pState->strCommand);
    iLineLength = (int32_t)strlen(pBuffer);

    // send it
    for (pWrite = pBuffer, iBytesSent = 0; iLineLength != 0; )
    {
        if ((iResult = SocketSend(pSocketState->pSocket, pWrite, iLineLength, 0)) < 0)
        {
            ZPrintf("testercomm_socket: error %d sending command to remote target\n", iResult);
            pSocketState->eState = ST_FAIL;
            return(-1);
        }
        else
        {
            iLineLength -= iResult;
            pWrite += iResult;
            iBytesSent += iResult;
        }

        if (iLineLength != 0)
        {
            NetConnSleep(5);
        }
    }

    // mark the last send time
    pState->uLastSendTime = ZTick();
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _TesterCommConnect

    \Description
        Connect the host client communication module.

    \Input *pState  - pointer to host client comm module
    \Input *pParams - startup parameters
    \Input bIsHost  - TRUE=host, FALSE=client

    \Output
        int32_t     - 0 for success, error code otherwise

    \Version 10/17/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _TesterCommConnect(TesterCommT *pState, const char *pParams, uint32_t bIsHost)
{
    TesterCommSocketT *pSocketState = pState->pInterface->pData;
    char *pUserPort;
    uint16_t aJson[512];

    // remember host status
    pSocketState->bIsHost = (uint8_t)bIsHost;
    pSocketState->iPort = T2COMM_PORT;

    // get startup parameters
    JsonParse(aJson, sizeof(aJson)/sizeof(*aJson), pParams, -1);
    JsonGetString(JsonFind(aJson, "HOSTNAME"), pSocketState->strHostName, sizeof(pSocketState->strHostName), "");

    // check for a user specified port
    if ((pUserPort = strchr(pSocketState->strHostName, ':')) != NULL )
    {
        pSocketState->iPort = atoi(pUserPort+1);
    }

    // do the connect
    return(_TesterCommConnect2(pSocketState));
}

/*F********************************************************************************/
/*!
    \Function _TesterCommUpdate

    \Description
        Give the host/client interface module some processor time.  Call this
        once in a while to pump the input and output pipes.

    \Input *pState  - module state

    \Output
        int32_t     - 0 for success, error code otherwise

    \Version 10/17/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _TesterCommUpdate(TesterCommT *pState)
{
    int32_t iResult;

    // quit if we are suspended (don't do any more commands)
    if (pState->uSuspended)
    {
        return(0);
    }

    // check for accepted connection
    if (_TesterCommCheckConnect(pState) <= 0)
    {
        return(0);
    }

    // check for outgoing and incoming data
    _TesterCommCheckInput(pState);
    _TesterCommCheckOutput(pState);

    // now call the callbacks for incoming messages
    while ((iResult  = ZListPopFront(pState->pInputData, &pState->LineData)) > 0)
    {
        // try to access the message map
        if ((pState->LineData.iType >= 0) && (pState->MessageMap[pState->LineData.iType] != NULL))
        {
            // protect against recursion by suspending commands until this one completes
            TesterCommSuspend(pState);
            (pState->MessageMap[pState->LineData.iType])(pState, pState->LineData.strBuffer, pState->pMessageMapUserData[pState->LineData.iType]);
            TesterCommWake(pState);
        }
    }

    // done
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _TesterCommDisconnect

    \Description
        Disconnect the host client communication module.

    \Input *pState  - pointer to host client comm module

    \Output
        int32_t     - 0=success, error code otherwise

    \Version 10/17/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _TesterCommDisconnect(TesterCommT *pState)
{
    TesterCommSocketT *pSocketState = (TesterCommSocketT *)pState->pInterface->pData;

    _TesterCommDisconnect2(pSocketState);
    if (pSocketState->pListen != NULL)
    {
        SocketClose(pSocketState->pListen);
        pSocketState->pListen = NULL;
    }

    return(0);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function TesterCommAttachSocket

    \Description
        Attach file module function pointers to a tester comm module.

    \Input *pState      - pointer to host client comm module

    \Version 10/17/2011 (jbrookes)
*/
/********************************************************************************F*/
void TesterCommAttachSocket(TesterCommT *pState)
{
    ZPrintf("testercomm_socket: attaching socket interface methods\n");
    pState->pInterface->CommConnectFunc = &_TesterCommConnect;
    pState->pInterface->CommUpdateFunc = &_TesterCommUpdate;
    pState->pInterface->CommDisconnectFunc = &_TesterCommDisconnect;
    pState->pInterface->pData = (TesterCommSocketT *)ZMemAlloc(sizeof(TesterCommSocketT));
    ds_memclr(pState->pInterface->pData, sizeof(TesterCommSocketT));
}
