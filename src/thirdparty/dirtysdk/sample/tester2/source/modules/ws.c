/*H********************************************************************************/
/*!
    \File ws.c

    \Description
        Test WebSockets

    \Notes
        Websockets test URL: ws://echo.websocket.org/ (see
        http://www.websocket.org/echo.html).

    \Copyright
        Copyright (c) Electronic Arts 2012.

    \Version 11/27/2012 (jbrookes) First Version
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/proto/protowebsocket.h"

#include "libsample/zlib.h"
#include "libsample/zfile.h"
#include "libsample/zmem.h"
#include "testersubcmd.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct WSAppT
{
    ProtoWebSocketRefT *pWebSocket;
    char *pSendBuf;
    int32_t iSendLen;
    int32_t iSendOff;
    char *pRecvBuf;
    int32_t iRecvBufLen;
    uint8_t bUseMessageApis; // if TRUE, use message semantics for send/recv; specified as send argument
} WSAppT;

/*** Function Prototypes **********************************************************/

static void _WSCreate(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _WSDestroy(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _WSConnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _WSDisconnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _WSControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _WSStatus(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _WSSend(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);

/*** Variables ********************************************************************/

static T2SubCmdT _WS_Commands[] =
{
    { "create",     _WSCreate       },
    { "destroy",    _WSDestroy      },
    { "connect",    _WSConnect      },
    { "disconnect", _WSDisconnect   },
    { "ctrl",       _WSControl      },
    { "stat",       _WSStatus       },
    { "send",       _WSSend         },
    { "",           NULL            },
};

static WSAppT _WS_App = { NULL, NULL, 0, 0, "", 0, 0 };


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _WSResetApp

    \Description
        Reset app, freeing buffers / resetting state

    \Input *pApp    - app state

    \Version 03/31/2017 (jbrookes)
*/
/********************************************************************************F*/
static void _WSResetApp(WSAppT *pApp)
{
    if (pApp->pSendBuf != NULL)
    {
        ZMemFree(pApp->pSendBuf);
        pApp->pSendBuf = NULL;
    }
    pApp->iSendLen = 0;
    pApp->iSendOff = 0;
}

/*F********************************************************************************/
/*!
    \Function _WSDestroyApp

    \Description
        Destroy app, freeing modules.

    \Input *pApp    - app state

    \Version 11/27/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _WSDestroyApp(WSAppT *pApp)
{
    // reset state
    _WSResetApp(pApp);

    // free recv buffer
    if (pApp->pRecvBuf != NULL)
    {
        ZMemFree(pApp->pRecvBuf);
        pApp->pRecvBuf = NULL;
    }
    pApp->iRecvBufLen = 0;

    // destroy module
    if (pApp->pWebSocket != NULL)
    {
        ProtoWebSocketDestroy(pApp->pWebSocket);
    }
    ds_memclr(pApp, sizeof(*pApp));
}

/*

    WS Commands

*/

/*F*************************************************************************************/
/*!
    \Function _WSCreate

    \Description
        WS subcommand - create websocket module

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 11/27/2012 (jbrookes)
*/
/**************************************************************************************F*/
static void _WSCreate(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    WSAppT *pApp = &_WS_App;
    int32_t iBufSize = 4*1024;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s create <bufsize>\n", argv[0]);
        return;
    }

    // if we already have one, skip
    if (pApp->pWebSocket != NULL)
    {
        ZPrintf("%s: websocket module already created\n", argv[0]);
        return;
    }

    // bufsize arg?
    if (argc > 2)
    {
        iBufSize = (int32_t)strtol(argv[2], NULL, 10);
    }

    // create app receive buffer (+1 to hold terminator character)
    if ((pApp->pRecvBuf = ZMemAlloc(iBufSize+1)) == NULL)
    {
        ZPrintf("%s: could not allocate memory for application receive buffer\n", argv[0]);
        _WSDestroyApp(pApp);
        return;
    }
    pApp->iRecvBufLen = iBufSize+1;

    // create a websocket module if it isn't already started
    if ((pApp->pWebSocket = ProtoWebSocketCreate(iBufSize)) == NULL)
    {
        ZPrintf("%s: error creating websocket module\n", argv[0]);
        _WSDestroyApp(pApp);
    }
}

/*F*************************************************************************************/
/*!
    \Function _WSDestroy

    \Description
        WS subcommand - destroy websocket module

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 11/27/2012 (jbrookes)
*/
/**************************************************************************************F*/
static void _WSDestroy(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    WSAppT *pApp = &_WS_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s destroy\n", argv[0]);
        return;
    }

    _WSDestroyApp(pApp);
}

/*F*************************************************************************************/
/*!
    \Function _WSConnect

    \Description
        WS subcommand - connect to server

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 11/27/2012 (jbrookes)
*/
/**************************************************************************************F*/
static void _WSConnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    WSAppT *pApp = &_WS_App;
    int32_t iArg, iStartArg = 2;
    char strApndHdr[1024] = "";

    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("   usage: %s connect <ws-url>\n", argv[0]);
        return;
    }

    // reset app state
    _WSResetApp(pApp);

    // check for connect args
    for (iArg = iStartArg; (iArg < argc) && (argv[iArg][0] == '-'); iArg += 1)
    {
        if (!ds_strnicmp(argv[iArg], "-header=", 8))
        {
            ds_strnzcat(strApndHdr, argv[iArg]+8, sizeof(strApndHdr));
            ds_strnzcat(strApndHdr, "\r\n", sizeof(strApndHdr));
        }
        // skip any option arguments to find url and (optionally) filename
        iStartArg += 1;
    }

    // log connection attempt
    ZPrintf("%s: connecting to '%s'\n", argv[0], argv[iStartArg]);

    // set append header?
    if (strApndHdr[0] != '\0')
    {
        ProtoWebSocketControl(pApp->pWebSocket, 'apnd', 0, 0, strApndHdr);
    }

    // start connect to remote user
    ProtoWebSocketConnect(pApp->pWebSocket, argv[iStartArg]);
}

/*F*************************************************************************************/
/*!
    \Function _WSDisconnect

    \Description
        WS subcommand - disconnect from server

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 11/30/2012 (jbrookes)
*/
/**************************************************************************************F*/
static void _WSDisconnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    WSAppT *pApp = &_WS_App;

    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s disconnect\n", argv[0]);
        return;
    }

    // log connection attempt
    ZPrintf("%s: disconnecting from server\n", argv[0], argv[2]);

    // start connect to remote user
    ProtoWebSocketDisconnect(pApp->pWebSocket);
}

/*F*************************************************************************************/
/*!
    \Function _WSControl

    \Description
        WS control subcommand - set control options

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 11/27/2012 (jbrookes)
*/
/**************************************************************************************F*/
static void _WSControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    WSAppT *pApp = &_WS_App;
    int32_t iCmd, iValue = 0, iValue2 = 0;
    void *pValue = NULL;

    if ((bHelp == TRUE) || (argc < 3) || (argc > 6))
    {
        ZPrintf("   usage: %s ctrl [cmd] <iValue> <iValue2> <pValue>\n", argv[0]);
        return;
    }

    // get the command
    iCmd  = ZGetIntArg(argv[2]);

    // get optional arguments
    if (argc > 3)
    {
        iValue = ZGetIntArg(argv[3]);
    }
    if (argc > 4)
    {
        iValue2 = ZGetIntArg(argv[4]);
    }
    if (argc > 5)
    {
        pValue = argv[5];
    }

    // issue the control call
    ProtoWebSocketControl(pApp->pWebSocket, iCmd, iValue, iValue2, pValue);
}

/*F*************************************************************************************/
/*!
    \Function _WSStatus

    \Description
        WS status subcommand - query module status

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 11/27/2012 (jbrookes)
*/
/**************************************************************************************F*/
static void _WSStatus(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    WSAppT *pApp = &_WS_App;
    int32_t iCmd, iResult;
    char strBuffer[256] = "";

    if ((bHelp == TRUE) || (argc < 3) || (argc > 5))
    {
        ZPrintf("   usage: %s stat <cmd> <arg>\n", argv[0]);
        return;
    }

    // get the command
    iCmd  = ZGetIntArg(argv[2]);

    // issue the status call
    iResult = ProtoWebSocketStatus(pApp->pWebSocket, iCmd, strBuffer, sizeof(strBuffer));

    // report result
    ZPrintf("ws: ProtoWebSocketStatus('%C') returned %d (\"%s\")\n", iCmd, iResult, strBuffer);
}

/*F*************************************************************************************/
/*!
    \Function _WSSend

    \Description
        WS status subcommand - send data

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 11/29/2012 (jbrookes)
*/
/**************************************************************************************F*/
static void _WSSend(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    WSAppT *pApp = &_WS_App;
    int32_t iMsgLen, iStartArg=2, iMinArgs=4;
    char *pMessage;

    if ((bHelp == TRUE) || (argc < iMinArgs))
    {
        ZPrintf("   usage: %s send [-msg] -f <file> -m \"message\"\n", argv[0]);
        return;
    }

    if (pApp->pSendBuf != NULL)
    {
        ZPrintf("%s: already sending data, please try later\n", argv[0]);
        return;
    }

    if (!strcmp(argv[iStartArg], "-msg"))
    {
        pApp->bUseMessageApis = TRUE;
        iStartArg += 1;
        iMinArgs += 1;
    }

    if (!strcmp(argv[iStartArg], "-f") && (argc == iMinArgs))
    {
        char *pFileData;
        int32_t iFileSize; 

        // try to open file
        if ((pFileData = ZFileLoad(argv[iStartArg+1], &iFileSize, ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_BINARY)) != NULL)
        {
            pMessage = pFileData;
            iMsgLen = iFileSize;
        }
        else
        {
            ZPrintf("%s: could not open file '%s' for sending\n", argv[0], argv[iStartArg+1]);
            return;
        }
    }
    else if (!strcmp(argv[iStartArg], "-m"))
    {
        if ((iMsgLen = (int32_t)strlen(argv[iStartArg+1])) > 0)
        {
            pMessage = ZMemAlloc(iMsgLen+1);
            ds_strnzcpy(pMessage, argv[iStartArg+1], iMsgLen+1);
        }
        else
        {
            ZPrintf("%s: will not send empty message\n", argv[0]);
            return;
        }
    }
    else
    {
        ZPrintf("%s: unknown send option '%s'\n", argv[0], argv[iStartArg]);
        return;
    }

    // set up send params
    pApp->pSendBuf = pMessage;
    pApp->iSendLen = iMsgLen;
    pApp->iSendOff = 0;
}

/*F*************************************************************************************/
/*!
    \Function _CmdWSSend

    \Description
        Wraps ProtoWebSocketSend/Message

    \Input *pApp    - WS app ref
    \Input *pBuffer - send data
    \Input iLength  - length of send data

    \Version 04/11/2017 (jbrookes)
*/
/**************************************************************************************F*/
static int32_t _CmdWSSend(WSAppT *pApp, const char *pBuffer, int32_t iLength)
{
    return ((pApp->bUseMessageApis) ? ProtoWebSocketSendMessage(pApp->pWebSocket, pBuffer, iLength) : ProtoWebSocketSend(pApp->pWebSocket, pBuffer, iLength));
}

/*F*************************************************************************************/
/*!
    \Function _CmdWSRecv

    \Description
        Wraps ProtoWebSocketRecv/Message

    \Input *pApp    - WS app ref
    \Input *pBuffer - [out] recv buffer
    \Input iLength  - length of recv buffer

    \Version 04/11/2017 (jbrookes)
*/
/**************************************************************************************F*/
static int32_t _CmdWSRecv(WSAppT *pApp, char *pBuffer, int32_t iLength)
{
    return ((pApp->bUseMessageApis) ? ProtoWebSocketRecvMessage(pApp->pWebSocket, pBuffer, iLength) : ProtoWebSocketRecv(pApp->pWebSocket, pBuffer, iLength));
}

/*F********************************************************************************/
/*!
    \Function _CmdWSCb

    \Description
        Update WS command

    \Input *argz    - environment
    \Input argc     - standard number of arguments
    \Input *argv[]  - standard arg list

    \Output
        int32_T     -standard return value

    \Version 11/27/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdWSCb(ZContext *argz, int32_t argc, char *argv[])
{
    WSAppT *pApp = &_WS_App;
    int32_t iResult;

    // check for kill
    if (argc == 0)
    {
        _WSDestroyApp(pApp);
        ZPrintf("%s: killed\n", argv[0]);
        return(0);
    }

    // give life to the module
    if (pApp->pWebSocket != NULL)
    {
        // update the module
        ProtoWebSocketUpdate(pApp->pWebSocket);

        // processing, if we're connected
        if (ProtoWebSocketStatus(pApp->pWebSocket, 'stat', NULL, 0) == 1)
        {
            // try and receive some data
            if ((iResult = _CmdWSRecv(pApp, pApp->pRecvBuf, pApp->iRecvBufLen - 1)) > 0)
            {
                NetPrintf(("%s: received %d byte server frame\n", argv[0], iResult));
                // null terminate it so we can print it
                pApp->pRecvBuf[iResult] = '\0';
                // print it
                NetPrintWrap(pApp->pRecvBuf, 80);
                // clear message flag
                pApp->bUseMessageApis = FALSE;
            }
            else if (iResult == SOCKERR_NOMEM)
            {
                // our buffer is too small, so double it
                int32_t iRecvBufLen = pApp->iRecvBufLen * 2;
                char *pRecvBuf = ZMemAlloc(iRecvBufLen);
                if (pRecvBuf != NULL)
                {
                    ZPrintf("%s: increasing receive buffer to %d bytes\n", argv[0], iRecvBufLen);
                    ds_memcpy_s(pRecvBuf, iRecvBufLen, pApp->pRecvBuf, pApp->iRecvBufLen);
                    ZMemFree(pApp->pRecvBuf);
                    pApp->pRecvBuf = pRecvBuf;
                    pApp->iRecvBufLen = iRecvBufLen;
                }
                else
                {
                    ZPrintf("%s: error allocating %d bytes of memory for receive buffer\n", iRecvBufLen);
                }
            }
            else if (iResult < 0)
            {
                ZPrintf("%s: error %d receiving\n", argv[0], iResult);
                // clear message flag
                pApp->bUseMessageApis = FALSE;
                // keep running
                return(ZCallback(&_CmdWSCb, 16));
            }

            // try and send some data
            if (pApp->pSendBuf != NULL)
            {
                if ((iResult = _CmdWSSend(pApp, pApp->pSendBuf + pApp->iSendOff, pApp->iSendLen - pApp->iSendOff)) > 0)
                {
                    pApp->iSendOff += iResult;
                    if (pApp->iSendOff == pApp->iSendLen)
                    {
                        ZPrintf("%s: sent %d byte message\n", argv[0], pApp->iSendLen);
                        ZMemFree(pApp->pSendBuf);
                        pApp->pSendBuf = NULL;
                    }
                }
                else if (iResult < 0)
                {
                    ZPrintf("%s: error %d sending message\n", argv[0], iResult);
                }
            }
        }
    }

    // keep running
    return(ZCallback(&_CmdWSCb, 16));
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdWS

    \Description
        WS (WebSocket) command

    \Input *argz    - environment
    \Input argc     - standard number of arguments
    \Input *argv[]  - standard arg list

    \Output
        int32_t     - standard return value

    \Version 11/27/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdWS(ZContext *argz, int32_t argc, char *argv[])
{
    T2SubCmdT *pCmd;
    WSAppT *pApp = &_WS_App;
    int32_t iResult = 0;
    uint8_t bHelp;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_WS_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   test the websocket module\n");
        T2SubCmdUsage(argv[0], _WS_Commands);
        return(0);
    }

    // if no ref yet, make one
    if ((pCmd->pFunc != _WSCreate) && (pApp->pWebSocket == NULL))
    {
        char *pCreate = "create";
        ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
        _WSCreate(pApp, 1, &pCreate, bHelp);
        iResult = ZCallback(_CmdWSCb, 16);
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    // one-time install of periodic callback
    if (pCmd->pFunc == _WSCreate)
    {
        iResult = ZCallback(_CmdWSCb, 16);
    }
    return(iResult);
}

