/*H********************************************************************************/
/*!
    \File protohttpmanager.c

    \Description
        High-level module designed to create and manage a pool of ProtoHttp refs.  A
        client application can submit rapid-fire http requests and ProtoHttpManager
        will distribute them efficiently across the ref pool internally, queuing
        them for efficient use of keep-alive and pipelining requests where possible.

    \Notes
        Pipelining resources:

        http://www.mozilla.org/projects/netlib/http/pipelining-faq.html
        http://www.w3.org/Protocols/HTTP/Performance/Pipeline.html

    \Todo
        Validate POST support
        Pipelining:
            - handle fewer responses than expected (?)

    \Copyright
        Copyright (c) Electronic Arts 2009-2011.

    \Version 1.0 05/20/2009 (jbrookes) First Version
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protohttpmanager.h"

/*** Defines **********************************************************************/

//! maximum number of protohttp refs that can be allocated
#ifdef DIRTYCODE_NX
#define HTTPMANAGER_MAXREFS     (48)
#else
#define HTTPMANAGER_MAXREFS     (64)
#endif

//! maximum number of protohttp commands that can be queued for execution
#define HTTPMANAGER_MAXCMDS     (256)

//! maximum number of protohttp commands that can be queued in a given httpref
#define HTTPMANAGER_MAXREFQUEUE (16)

//! define for final-mode diagnostic printing
#define HTTPMANAGER_FINALDEBUG  (!DIRTYCODE_DEBUG && FALSE)

#if HTTPMANAGER_FINALDEBUG
#if defined(DIRTYCODE_PC)
#include <windows.h> // OutputDebugStringA
#else
#include <stdlib.h>
#endif
#endif

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef enum HttpManagerHttpCmdStateE
{
    HTTPMANAGER_CMDSTATE_IDLE = 0,      //!< unallocated
    HTTPMANAGER_CMDSTATE_WAIT,          //!< queued and waiting to start
    HTTPMANAGER_CMDSTATE_PIPE,          //!< pipelined
    HTTPMANAGER_CMDSTATE_ACTV,          //!< active
    HTTPMANAGER_CMDSTATE_DONE,          //!< complete
    HTTPMANAGER_CMDSTATE_FAIL           //!< queued execution of command failed
} HttpManagerHttpCmdStateE;

typedef enum HttpManagerHttpRefStateE
{
    HTTPMANAGER_REFSTATE_NONE = 0,     //!< no ref available
    HTTPMANAGER_REFSTATE_IDLE,         //!< ref is available
    HTTPMANAGER_REFSTATE_BUSY,         //!< ref is in use
} HttpManagerHttpRefStateE;

//! http ref info
typedef struct HttpManagerHttpRefT
{
    ProtoHttpRefT *pProtoHttp;          //!< http ref
    struct HttpManagerHttpCmdT *HttpCmdQueue[HTTPMANAGER_MAXREFQUEUE];  //!< http command queue
    uint32_t uLastTick;                 //!< last timestamp ref was accessed
    uint8_t uHttpState;                 //!< http ref state
    int8_t iTransactions;               //!< number of transactions queued
    int8_t iCurTransaction;             //!< current transaction counter (used for callbacks)
    int8_t _pad;
} HttpManagerHttpRefT;

//! http cmd info
typedef struct HttpManagerHttpCmdT
{
    HttpManagerRefT *pHttpManager;      //!< HttpManager ref, required by ProtoHttp callback
    HttpManagerHttpRefT *pHttpRef;      //!< pointer to http ref used for this transaction
    int32_t iHttpHandle;                //!< handle associated with this transaction (zero means unallocated)
    int32_t iTimeout;                   //!< timeout to set for http ref (zero=do not set)
    void *pCallbackRef;                 //!< user callback ref
    ProtoHttpWriteCbT *pWriteCb;        //!< write callback
    ProtoHttpCustomHeaderCbT *pCustomHeaderCb;      //!< custom header callback
    ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb;    //!< receive header callback
    void *pUserData;                    //!< callback user data
    const char *pUrl;                   //!< request url
    char *pAppendHdr;                   //!< append header (optional)
    int32_t iResult;                    //!< transaction result
    uint32_t uQueueTick;                //!< tick request was queued at
    uint32_t uIssueTick;                //!< tick request was issued at
    uint32_t uComplTick;                //!< tick request was completed at
    uint64_t uBytesXfer;                //!< bytes transferred
    uint8_t uRequestType;               //!< ProtoHttpRequestTypeE
    uint8_t uState;                     //!< transaction state
    uint8_t bKeepAlive;                 //!< TRUE if this command was queued with keep-alive
    uint8_t bCopiedUrl;                 //!< TRUE if the url was copied
} HttpManagerHttpCmdT;

//! http module state
struct HttpManagerRefT
{
    // module memory group
    int32_t iMemGroup;      //!< module mem group id
    void *pMemGroupUserData;//!< user data associated with mem group

    // user callback info
    ProtoHttpCustomHeaderCbT *pCustomHeaderCb;     //!< callback for modifying request header
    ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb;   //!< callback for viewing recv header on recepit

    //! current handle
    int32_t iHttpHandle;

    //! module debuglevel
    int32_t iVerbose;

    //! transaction stats
    HttpManagerStatT HttpManagerStats;

    // module configuration variables
    uint8_t bPipelining;                    //!< pipelining enable/disable
    uint8_t bKeepalive;                     //!< keep-alive enable/disable
    uint8_t bPipeWithoutKeepAlive;          //!< if TRUE, will pipeline without a prior keep-alive connection
    uint8_t bCopyUrl;                       //!< if TRUE, alloc memory for and copy url; else rely on caller to persist
    int8_t iMaxPipedUrls;                   //!< maximum number of Urls that may be piped in a single request
    uint8_t bAutoUpdate;                    //!< TRUE if auto-update enabled (default), else FALSE
    uint8_t bXhttpEnabled;                  //!< XHTTP enabled (Xbox 360 only)
    uint8_t _pad;

    //! number of available http refs
    int32_t iHttpNumRefs;
    //! protohttp buffer size
    int32_t iHttpBufSize;

    //! global append header (optional)
    char *pAppendHdr;

    //! protohttp module pool
    HttpManagerHttpRefT HttpRefs[HTTPMANAGER_MAXREFS];

    //! protohttp command pool
    HttpManagerHttpCmdT HttpCmds[HTTPMANAGER_MAXCMDS];
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private variables

#if HTTPMANAGER_FINALDEBUG
int32_t _HttpManager_iStartTick = 0;
#endif

// Public variables


/*** Private Functions ************************************************************/

#if HTTPMANAGER_FINALDEBUG
/*F********************************************************************************/
/*!
    \Function _HttpManagerPrintfCode

    \Description
        Special final-mode PC debugging hook to display netprintf debug output.

    \Input *pFormat         - format string
    \Input ...              - variable-length arg list

    \Output
        int32_t             - zero

    \Version 05/22/2009 (jbrookes)
*/
/********************************************************************************F*/
#undef NetPrintf
#define NetPrintf(_x) _HttpManagerPrintfCode _x
static int32_t _HttpManagerPrintfCode(const char *pFormat, ...)
{
    va_list pFmtArgs;
    char strText[4096];
    char strTick[16];
    const char *pText = strText;

    va_start(pFmtArgs, pFormat);
    // check for simple formatting
    if ((pFormat[0] == '%') && (pFormat[1] == 's') && (pFormat[2] == 0))
    {
        pText = va_arg(pFmtArgs, const char *);
    }
    else
    {
        vsprintf(strText, pFormat, pFmtArgs);
    }
    va_end(pFmtArgs);

    ds_snzprintf(strTick, sizeof(strTick), "[%d] ", NetTick()-_HttpManager_iStartTick);
    #if defined(DIRTYCODE_PC)
    OutputDebugStringA(strTick);
    OutputDebugStringA(pText);
    #else
    printf("%s%s", strTick, pText);
    #endif
    return(0);
}
#endif

#if HTTPMANAGER_FINALDEBUG
/*F********************************************************************************/
/*!
    \Function _HttpManagerPrintfVerboseCode

    \Description
        Special final-mode PC debugging hook to display netprintfverbose debug output.

    \Input iVerbosityLevel  - module verbosity level
    \Input iCheckLevel      - verbosity to check against for this statement
    \Input *pFormat         - format string
    \Input ...              - variable-length arg list

    \Output
        int32_t             - zero

    \Version 05/26/2009 (jbrookes)
*/
/********************************************************************************F*/
#undef NetPrintfVerbose
#define NetPrintfVerbose(_x) _HttpManagerPrintfVerboseCode _x
static int32_t _HttpManagerPrintfVerboseCode(int32_t iVerbosityLevel, int32_t iCheckLevel, const char *pFormat, ...)
{
    va_list pFmtArgs;
    char strText[4096];
    char strTick[16];
    const char *pText = strText;

    va_start(pFmtArgs, pFormat);
    // check for simple formatting
    if ((pFormat[0] == '%') && (pFormat[1] == 's') && (pFormat[2] == 0))
    {
        pText = va_arg(pFmtArgs, const char *);
    }
    else
    {
        vsprintf(strText, pFormat, pFmtArgs);
    }
    va_end(pFmtArgs);

    if (iCheckLevel < iVerbosityLevel)
    {
        ds_snzprintf(strTick, sizeof(strTick), "[%d] ", NetTick()-_HttpManager_iStartTick);
        #if defined(DIRTYCODE_PC)
        OutputDebugStringA(strTick);
        OutputDebugStringA(pText);
        #else
        printf("%s%s", strTick, pText);
        #endif
    }
    return(0);
}
#endif

#if DIRTYCODE_LOGGING || HTTPMANAGER_FINALDEBUG
/*F********************************************************************************/
/*!
    \Function _HttpManagerDisplayStats

    \Description
        Display transfer stats for module.

    \Input *pHttpManager    - httpmanager state

    \Version 02/21/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpManagerDisplayStats(HttpManagerRefT *pHttpManager)
{
    // display stats
    NetPrintf(("httpmanager: transactions: %d\n", pHttpManager->HttpManagerStats.uNumTransactions));
    if (pHttpManager->HttpManagerStats.uNumTransactions > 0)
    {
        NetPrintf(("httpmanager: keepalive transactions: %d\n", pHttpManager->HttpManagerStats.uNumKeepAliveTransactions));
        NetPrintf(("httpmanager: pipelined transactions: %d\n", pHttpManager->HttpManagerStats.uNumPipelinedTransactions));
        NetPrintf(("httpmanager: max active transactions: %d\n", pHttpManager->HttpManagerStats.uMaxActiveTransactions));
        NetPrintf(("httpmanager: max queued transactions: %d\n", pHttpManager->HttpManagerStats.uMaxQueuedTransactions));
        NetPrintf(("httpmanager: sum queue wait time: %dms\n", pHttpManager->HttpManagerStats.uSumQueueWaitLatency));
        NetPrintf(("httpmanager: avg queue wait time: %dms\n", pHttpManager->HttpManagerStats.uSumQueueWaitLatency/pHttpManager->HttpManagerStats.uNumTransactions));
        NetPrintf(("httpmanager: max queue wait time: %dms\n", pHttpManager->HttpManagerStats.uMaxQueueWaitLatency));
        NetPrintf(("httpmanager: sum queue free time: %dms\n", pHttpManager->HttpManagerStats.uSumQueueFreeLatency));
        NetPrintf(("httpmanager: avg queue free time: %dms\n", pHttpManager->HttpManagerStats.uSumQueueFreeLatency/pHttpManager->HttpManagerStats.uNumTransactions));
        NetPrintf(("httpmanager: max queue free time: %dms\n", pHttpManager->HttpManagerStats.uMaxQueueFreeLatency));
        NetPrintf(("httpmanager: total bytes transferred: %qd\n", pHttpManager->HttpManagerStats.uTransactionBytes));
        NetPrintf(("httpmanager: total transaction time: %d\n", pHttpManager->HttpManagerStats.uTransactionTime));
        NetPrintf(("httpmanager: avg bytes per second %.2f\n", (float)pHttpManager->HttpManagerStats.uTransactionBytes*1000.0f/(float)pHttpManager->HttpManagerStats.uTransactionTime));
        NetPrintf(("httpmanager: avg transaction size %.2f\n", (float)pHttpManager->HttpManagerStats.uTransactionBytes/(float)pHttpManager->HttpManagerStats.uNumTransactions));
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function _HttpManagerResetStats

    \Description
        Reset transfer stats for module.

    \Input *pHttpManager    - httpmanager state

    \Version 07/26/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpManagerResetStats(HttpManagerRefT *pHttpManager)
{
    ds_memclr(&pHttpManager->HttpManagerStats, sizeof(pHttpManager->HttpManagerStats));
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerUrlCompare

    \Description
        Compares base elements of given Urls (kind, host, port, security) and
        returns whether they are identical or not.

    \Input *pUrlA       - Url to compare
    \Input *pUrlB       - Url to compare

    \Output
        uint32_t        - zero=same, else different

    \Version 07/01/2009 (jbrookes) First Version
*/
/********************************************************************************F*/
static uint32_t _HttpManagerUrlCompare(const char *pUrlA, const char *pUrlB)
{
    char strKindA[16], strKindB[16], strHostA[128], strHostB[128];
    int32_t iPortA, iPortB, iSecureA, iSecureB;

    ProtoHttpUrlParse(pUrlA, strKindA, sizeof(strKindA), strHostA, sizeof(strHostA), &iPortA, &iSecureA);
    ProtoHttpUrlParse(pUrlB, strKindB, sizeof(strKindB), strHostB, sizeof(strHostB), &iPortB, &iSecureB);

    return(!((ds_stricmp(strKindA, strKindB) == 0) && (ds_stricmp(strHostA, strHostB) == 0) && (iPortA == iPortB) && (iSecureA == iSecureB)));
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerPipelineCheck

    \Description
        See if specified requests will pipeline.

    \Input *pHttpCmd        - previously issued command
    \Input *pHttpCmd2       - command to check for piping with previous command

    \Output
        int32_t             - zero=will not pipe; else will pipe

    \Version 07/15/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpManagerPipelineCheck(HttpManagerHttpCmdT *pHttpCmd, HttpManagerHttpCmdT *pHttpCmd2)
{
    // make sure request can be pipelined
    if ((pHttpCmd2->uRequestType != PROTOHTTP_REQUESTTYPE_GET) && (pHttpCmd2->uRequestType != PROTOHTTP_REQUESTTYPE_HEAD))
    {
        return(0);
    }

    // make sure urls will pipe
    return(_HttpManagerUrlCompare(pHttpCmd->pUrl, pHttpCmd2->pUrl) == 0);
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerPipelineReset

    \Description
        Reset pipe following a request failure of some kind or another with subsequent
        piped results that need to be reissued.

    \Input *pHttpManager    - reference pointer
    \Input *pHttpRef        - http ref piped transactions we need to consider for reset are queued on
    \Input iHttpCmdFirst    - first command to reset

    \Version 07/20/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpManagerPipelineReset(HttpManagerRefT *pHttpManager, HttpManagerHttpRefT *pHttpRef, int32_t iHttpCmdFirst)
{
    HttpManagerHttpCmdT *pHttpCmd = NULL;
    int32_t iHttpCmd;

    // reset commands for reissue
    for (iHttpCmd = iHttpCmdFirst ; iHttpCmd < pHttpRef->iTransactions; iHttpCmd += 1)
    {
        pHttpCmd = pHttpRef->HttpCmdQueue[iHttpCmd];
        if ((pHttpCmd->uState == HTTPMANAGER_CMDSTATE_ACTV) || (pHttpCmd->uState == HTTPMANAGER_CMDSTATE_PIPE))
        {
            NetPrintfVerbose((pHttpManager->iVerbose, 1, "httpmanager: reset handle %d for re-issue\n", pHttpCmd->iHttpHandle));
            pHttpCmd->uState = HTTPMANAGER_CMDSTATE_WAIT;
            pHttpCmd->bKeepAlive = FALSE;
        }
        else
        {
            break;
        }
    }
    if ((iHttpCmd - iHttpCmdFirst) > 0)
    {
        NetPrintfVerbose((pHttpManager->iVerbose, 1, "httpmanager: reset %d requests on ref %2d for re-issue\n", iHttpCmd - iHttpCmdFirst, pHttpRef - pHttpCmd->pHttpManager->HttpRefs));
    }
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerCustomHeaderCb

    \Description
        Handle ProtoHttp custom header send callback, and pass it on to caller-registered
        callback handler with handle-specific callback user data pointer.

    \Input *pState          - protohttp state
    \Input *pHeader         - request header
    \Input uHeaderSize      - amount of space available for header (including current header text)
    \Input *pData           - data to be appended to header
    \Input iDataLen         - total size of data
    \Input *pUserRef        - HttpManagerHttpRefT for this request

    \Output
        int32_t             - user callback result

    \Version 05/20/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpManagerCustomHeaderCb(ProtoHttpRefT *pState, char *pHeader, uint32_t uHeaderSize, const char *pData, int64_t iDataLen, void *pUserRef)
{
    HttpManagerHttpRefT *pHttpRef = (HttpManagerHttpRefT *)pUserRef;
    HttpManagerHttpCmdT *pHttpCmd = pHttpRef->HttpCmdQueue[pHttpRef->iCurTransaction];
    NetPrintfVerbose((pHttpCmd->pHttpManager->iVerbose, 1, "httpmanager: calling custom header callback with userref 0x%08x for handle %d\n", (uintptr_t)pHttpCmd->pCallbackRef, pHttpCmd->iHttpHandle));
    return(pHttpCmd->pHttpManager->pCustomHeaderCb(pState, pHeader, uHeaderSize, pData, iDataLen, pHttpCmd->pCallbackRef));
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerReceiveHeaderCb

    \Description
        Handle ProtoHttp header receive callback, and pass it on to caller-registered
        callback handler with handle-specific callback user data pointer.

    \Input *pState          - protohttp state
    \Input *pHeader         - response header
    \Input uHeaderSize      - size of response header
    \Input *pUserRef        - HttpManagerHttpRefT for this request

    \Output
        int32_t             - user callback result

    \Version 05/20/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpManagerReceiveHeaderCb(ProtoHttpRefT *pState, const char *pHeader, uint32_t uHeaderSize, void *pUserRef)
{
    HttpManagerHttpRefT *pHttpRef = (HttpManagerHttpRefT *)pUserRef;
    HttpManagerHttpCmdT *pHttpCmd = pHttpRef->HttpCmdQueue[pHttpRef->iCurTransaction];
    HttpManagerRefT *pHttpManager = pHttpCmd->pHttpManager;
    int32_t iResult, iHttpCmd = -1;
    ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb;
    void *pUserData;

    /* early out if pHttpManager is null; fifa encountred an issue 
       In theory, if HttpManagerAlloc/HttpManagerFree are being called from different thread
       than ProtoHttpUpdate.  When HttpManagerFree is called it clears out a structure HttpManagerHttpCmdT that contains the 
       pointer to pHttpManager, it is then a race condition on when the header callback is called causing a crash. */
    if (pHttpManager == NULL)
    {
        NetPrintf(("httpmanager: skipping _HttpManagerReceiveHeaderCb because pHttpManager is null\n"));
        return(0);
    }

    // check for an error response, and handle if it will affect piped commands
    iResult = ProtoHttpStatus(pState, 'code', NULL, 0);
    if (PROTOHTTP_GetResponseClass(iResult) != PROTOHTTP_RESPONSE_SUCCESSFUL)
    {
        if (PROTOHTTP_GetResponseClass(iResult) == PROTOHTTP_RESPONSE_REDIRECTION)
        {
            char strLocation[1024];
            ProtoHttpGetLocationHeader(pState, pHeader, strLocation, sizeof(strLocation), NULL);
            NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: [%d] redirect ref %d url=%s (%d)\n", pHttpCmd->iHttpHandle,
                pHttpCmd->pHttpRef - pHttpManager->HttpRefs, strLocation, iResult));
        }
        else
        {
            NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: [%d] redirect ref %d (%d)\n", pHttpCmd->iHttpHandle,
                pHttpCmd->pHttpRef - pHttpManager->HttpRefs, iResult));
        }

        /* if this transaction timed out (from a keep-alive connection that was closed) mark it for reissue,
           otherwise, we just look for subsequent piped results that need to be reissued */
        if (iResult == PROTOHTTP_RESPONSE_REQUESTTIMEOUT)
        {
            iHttpCmd = 0;
        }
        if (PROTOHTTP_GetResponseClass(iResult) == PROTOHTTP_RESPONSE_REDIRECTION)
        {
            iHttpCmd = 1;
        }
    }

    // check for loss of piped commands due to premature close
    if (ProtoHttpStatus(pState, 'plst', NULL, 0) == TRUE)
    {
        NetPrintfVerbose((pHttpManager->iVerbose, 1, "httpmanager: piped commands lost due to premature close on handle %d\n", pHttpCmd->iHttpHandle));
        iHttpCmd = 1;

        // downgrade our pipelining support $$todo$$ - this should be tracked per site
        if ((PROTOHTTP_GetResponseClass(iResult) != PROTOHTTP_RESPONSE_REDIRECTION) && (pHttpManager->bPipeWithoutKeepAlive))
        {
            NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager; disabling pipelining without keep-alive\n"));
            pHttpManager->bPipeWithoutKeepAlive = FALSE;
        }
    }

    // reset pipe if necessary
    if (iHttpCmd != -1)
    {
        _HttpManagerPipelineReset(pHttpManager, pHttpRef, iHttpCmd);
    }

    // if we have a 408 timeout response, reset ref to idle state, so we can reissue the request
    if (iResult == PROTOHTTP_RESPONSE_REQUESTTIMEOUT)
    {
        pHttpRef->uHttpState = HTTPMANAGER_REFSTATE_IDLE;
        // since we are going to re-issue this request, we do not forward the 408 response on to the application
        return(0);
    }

    // request level callback takes priority to global
    if ((pReceiveHeaderCb = pHttpCmd->pReceiveHeaderCb) != NULL)
    {
        pUserData = pHttpCmd->pUserData;
    }
    else
    {
        pReceiveHeaderCb = pHttpManager->pReceiveHeaderCb;
        pUserData = pHttpCmd->pCallbackRef;
    }

    // forward to caller's receive callback, if installed
    if (pReceiveHeaderCb != NULL)
    {
        NetPrintfVerbose((pHttpManager->iVerbose, 1, "httpmanager: calling receive header callback with userref 0x%08x for handle %d\n", (uintptr_t)pUserData, pHttpCmd->iHttpHandle));
        return(pReceiveHeaderCb(pState, pHeader, uHeaderSize, pUserData));
    }
    else
    {
        return(0);
    }
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerWriteCbProcess

    \Description
        User write callback processing, if write callback is set

    \Input *pHttpManager    - reference pointer
    \Input *pHttpCmd        - http cmd

    \Version 07/30/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpManagerWriteCbProcess(HttpManagerRefT *pHttpManager, HttpManagerHttpCmdT *pHttpCmd)
{
    ProtoHttpWriteCbInfoT WriteCbInfo;
    char strTempRecv[1024];
    int32_t iResult;

    ds_memclr(&WriteCbInfo, sizeof(WriteCbInfo));
    WriteCbInfo.eRequestType = (ProtoHttpRequestTypeE)pHttpCmd->uRequestType;
    WriteCbInfo.eRequestResponse = PROTOHTTP_RESPONSE_PENDING;

    while ((iResult = HttpManagerRecv(pHttpManager, pHttpCmd->iHttpHandle, strTempRecv, 1, sizeof (strTempRecv))) > 0)
    {
        pHttpCmd->pWriteCb(pHttpCmd->pHttpRef->pProtoHttp, &WriteCbInfo, strTempRecv, iResult, pHttpCmd->pUserData);
    }

    if ((iResult == PROTOHTTP_RECVDONE) || (iResult == PROTOHTTP_RECVHEAD) || (iResult == PROTOHTTP_RECVFAIL))
    {
        if (iResult != PROTOHTTP_RECVFAIL)
        {
            WriteCbInfo.eRequestResponse = (ProtoHttpResponseE)ProtoHttpStatus(pHttpCmd->pHttpRef->pProtoHttp, 'code', NULL, 0);
            pHttpCmd->pWriteCb(pHttpCmd->pHttpRef->pProtoHttp, &WriteCbInfo, "", iResult, pHttpCmd->pUserData);
        }
        else
        {
            pHttpCmd->pWriteCb(pHttpCmd->pHttpRef->pProtoHttp, &WriteCbInfo, "", PROTOHTTP_RECVFAIL, pHttpCmd->pUserData);
        }
        pHttpCmd->pWriteCb = NULL;
        pHttpCmd->pCustomHeaderCb = NULL;
        pHttpCmd->pReceiveHeaderCb = NULL;
        pHttpCmd->pUserData = NULL;
    }


}

/*F********************************************************************************/
/*!
    \Function _HttpManagerSetAppendHeader

    \Description
        Set append header, either on a global basis (pHttpCmd=NULL) or for a
        specific HTTP command.

    \Input *pHttpManager    - reference pointer
    \Input *pHttpCmd        - cmd to set append header for, or NULL to set global append header
    \Input *pAppendHdr      - append header to set

    \Output
        int32_t             - negative=failure, else success

    \Version 07/26/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpManagerSetAppendHeader(HttpManagerRefT *pHttpManager, HttpManagerHttpCmdT *pHttpCmd, const char *pAppendHdr)
{
    char **ppAppendHdr;

    // point to append header we are operating on
    ppAppendHdr = (pHttpCmd != NULL) ? &pHttpCmd->pAppendHdr : &pHttpManager->pAppendHdr;

    // if there is a previous append header, free it
    if (*ppAppendHdr != NULL)
    {
        DirtyMemFree(*ppAppendHdr, HTTPMGR_MEMID, pHttpManager->iMemGroup, pHttpManager->pMemGroupUserData);
        *ppAppendHdr = NULL;
    }
    // set new append header, if specified
    if ((pAppendHdr != NULL) && (*pAppendHdr != '\0'))
    {
        int32_t iHdrLen = (int32_t)strlen(pAppendHdr);
        // allocate memory to buffer append header
        if ((*ppAppendHdr = DirtyMemAlloc(iHdrLen+1, HTTPMGR_MEMID, pHttpManager->iMemGroup, pHttpManager->pMemGroupUserData)) == NULL)
        {
            NetPrintf(("httpmanager: could not allocate %d bytes for append header for handle %d\n", iHdrLen+1, (pHttpCmd != NULL) ? pHttpCmd->iHttpHandle: -1));
            return(-1);
        }
        // copy to append header
        ds_strnzcpy(*ppAppendHdr, pAppendHdr, iHdrLen+1);
    }
    // return success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerApplyAppendHeader

    \Description
        Applies append header to the specified command.  The append header set will
        be the command-specific header if set, else it will be the global append
        header if set.  If neither is set, the append header is cleared.

    \Input *pHttpManager    - reference pointer
    \Input *pHttpCmd        - cmd to get append header for

    \Version 07/26/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpManagerApplyAppendHeader(HttpManagerRefT *pHttpManager, HttpManagerHttpCmdT *pHttpCmd)
{
    char *pAppendHdr = pHttpCmd->pAppendHdr;
    if (pAppendHdr == NULL)
    {
        pAppendHdr = pHttpManager->pAppendHdr;
    }
    ProtoHttpControl(pHttpCmd->pHttpRef->pProtoHttp, 'apnd', 0, 0, pAppendHdr);
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerAllocHandle

    \Description
        Allocate a new handle.  Valid handles range from [1...7ffffff].

    \Input *pHttpManager    - reference pointer

    \Output
        int32_t             - new handle

    \Version 05/20/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpManagerAllocHandle(HttpManagerRefT *pHttpManager)
{
    int32_t iHttpHandle = pHttpManager->iHttpHandle;
    pHttpManager->iHttpHandle = (pHttpManager->iHttpHandle + 1) & 0x7fffffff;
    return(iHttpHandle);
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerAllocCmd

    \Description
        Allocate a new command.  A command represents a single HTTP transaction request.

    \Input *pHttpManager    - reference pointer

    \Output
        HttpManagerHttpCmdT *   - new command

    \Version 05/20/2009 (jbrookes)
*/
/********************************************************************************F*/
static HttpManagerHttpCmdT *_HttpManagerAllocCmd(HttpManagerRefT *pHttpManager)
{
    HttpManagerHttpCmdT *pHttpCmd;
    int32_t iHttpCmd;

    for (iHttpCmd = 0, pHttpCmd = NULL; iHttpCmd < HTTPMANAGER_MAXCMDS; iHttpCmd += 1)
    {
        if (pHttpManager->HttpCmds[iHttpCmd].iHttpHandle == 0)
        {
            pHttpCmd = &pHttpManager->HttpCmds[iHttpCmd];
            ds_memclr(pHttpCmd, sizeof(*pHttpCmd));
            pHttpCmd->pHttpManager = pHttpManager;
            pHttpCmd->iHttpHandle = _HttpManagerAllocHandle(pHttpManager);
            break;
        }
    }
    return(pHttpCmd);
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerAllocRef

    \Description
        Allocate an http ref.  An http ref represents a single ProtoHttp module.  In
        HttpManager, all ProtoHttp refs are created at startup; this function simply
        finds an appropriate ref to use for the specified transaction request.

    \Input *pHttpManager        - reference pointer
    \Input *pHttpCmd            - command

    \Output
        HttpManagerHttpRefT *   - new command

    \Version 05/20/2009 (jbrookes)
*/
/********************************************************************************F*/
static HttpManagerHttpRefT *_HttpManagerAllocRef(HttpManagerRefT *pHttpManager, HttpManagerHttpCmdT *pHttpCmd)
{
    HttpManagerHttpRefT *pHttpRef, *pAllocRef = NULL;
    HttpManagerHttpCmdT *pHttpPipeCmd;
    int32_t iHttpCmd, iHttpRef;
    char strKind[16], strHost[128];
    int32_t iPort, iSecure;

    // parse Url into component parts
    ProtoHttpUrlParse(pHttpCmd->pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure);

    // first pass, we try to find a ref we can pipeline this request on
    if (pHttpManager->bPipelining)
    {
        for (iHttpRef = 0; iHttpRef < pHttpManager->iHttpNumRefs; iHttpRef += 1)
        {
            // get ref
            pHttpRef = &pHttpManager->HttpRefs[iHttpRef];

            // iterate through queued transactions, if any
            for (iHttpCmd = 0; iHttpCmd < pHttpRef->iTransactions; iHttpCmd += 1)
            {
                // ref http command we might be able to pipe against
                pHttpPipeCmd = pHttpRef->HttpCmdQueue[iHttpCmd];
                if ((pHttpPipeCmd->uState == HTTPMANAGER_CMDSTATE_WAIT) && ((pHttpPipeCmd->bKeepAlive == TRUE) || (pHttpManager->bPipeWithoutKeepAlive == TRUE)))
                {
                    if (_HttpManagerPipelineCheck(pHttpPipeCmd, pHttpCmd) && (pHttpRef->iTransactions < HTTPMANAGER_MAXREFQUEUE))
                    {
                        if (iHttpCmd == (pHttpRef->iTransactions - 1))
                        {
                            pAllocRef = pHttpRef;
                            pHttpCmd->bKeepAlive = TRUE;
                            NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: [%d] queued   ref %d count=%d pipe=1 url=%s\n", pHttpCmd->iHttpHandle, iHttpRef, pAllocRef->iTransactions, pHttpCmd->pUrl));
                            break;
                        }
                    }
                }
            }
        }
    }

    // if not pipelined
    if (pAllocRef == NULL)
    {
        uint8_t aKeepAliveState[HTTPMANAGER_MAXREFS];
        int32_t iBestMatchIdx, iBestMatchTick, iHttpRefTick, iCurTick;
        int32_t iQueueDepth;

        // build list of keep-alive matches (whether a ref will try keep-alive with the specified Url) for all http refs
        ds_memclr(aKeepAliveState, sizeof(aKeepAliveState));
        for (iHttpRef = 0; iHttpRef < pHttpManager->iHttpNumRefs; iHttpRef += 1)
        {
            pHttpRef = &pHttpManager->HttpRefs[iHttpRef];
            if ((pHttpRef->uHttpState != HTTPMANAGER_REFSTATE_NONE) && ProtoHttpCheckKeepAlive(pHttpRef->pProtoHttp, pHttpCmd->pUrl))
            {
                aKeepAliveState[iHttpRef] = TRUE;
            }
        }

        // iterate through ref list in order from least heavily loaded (queue size) to most heavily loaded
        for (iQueueDepth = 0; (pAllocRef == NULL) && (iQueueDepth < HTTPMANAGER_MAXREFQUEUE); iQueueDepth += 1)
        {
            // first, try to find a ref at this depth with keep-alive potential
            for (iHttpRef = 0; iHttpRef < pHttpManager->iHttpNumRefs; iHttpRef += 1)
            {
                pHttpRef = &pHttpManager->HttpRefs[iHttpRef];
                if ((pHttpRef->iTransactions == iQueueDepth) && (aKeepAliveState[iHttpRef] == TRUE))
                {
                    // we have a match
                    pAllocRef = pHttpRef;
                    pHttpCmd->bKeepAlive = TRUE;
                    NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: [%d] queued   ref %d count=%d keep=1 url=%s\n", pHttpCmd->iHttpHandle, iHttpRef, pAllocRef->iTransactions, pHttpCmd->pUrl));
                    break;
                }
            }
            // did we find one?
            if (pAllocRef != NULL)
            {
                break;
            }

            // second pass, just look for the least recently used ref at this depth
            for (iHttpRef = 0, iBestMatchIdx = -1, iBestMatchTick = -1, iCurTick = NetTick(); iHttpRef < pHttpManager->iHttpNumRefs; iHttpRef += 1)
            {
                pHttpRef = &pHttpManager->HttpRefs[iHttpRef];
                iHttpRefTick = NetTickDiff(iCurTick, pHttpRef->uLastTick);
                if ((pHttpRef->iTransactions == iQueueDepth) && (iHttpRefTick > iBestMatchTick))
                {
                    // consider this for a match
                    iBestMatchTick = iHttpRefTick;
                    iBestMatchIdx  = iHttpRef;
                }
            }
            // if we found a ref, use it
            if (iBestMatchIdx >= 0)
            {
                pAllocRef = &pHttpManager->HttpRefs[iBestMatchIdx];
                pHttpCmd->bKeepAlive = FALSE;
                NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: [%d] queued   ref %d count=%d keep=0 url=%s\n", pHttpCmd->iHttpHandle, iBestMatchIdx, pAllocRef->iTransactions, pHttpCmd->pUrl));
                break;
            }
        }
    }

    // if we allocated a ref
    if (pAllocRef != NULL)
    {
        // update transaction stats
        if (pAllocRef->iTransactions > 0)
        {
            pHttpManager->HttpManagerStats.uNumQueuedTransactions += 1;
            if (pHttpManager->HttpManagerStats.uMaxQueuedTransactions < pHttpManager->HttpManagerStats.uNumQueuedTransactions)
            {
                pHttpManager->HttpManagerStats.uMaxQueuedTransactions = pHttpManager->HttpManagerStats.uNumQueuedTransactions;
            }
        }
        if (pHttpCmd->bKeepAlive)
        {
            pHttpManager->HttpManagerStats.uNumKeepAliveTransactions += 1;
        }

        // bind ref to cmd
        pHttpCmd->pHttpRef = pAllocRef;
        // add command to ref queue
        pHttpCmd->pHttpRef->HttpCmdQueue[pHttpCmd->pHttpRef->iTransactions++] = pHttpCmd;
        // update last access time
        pHttpCmd->pHttpRef->uLastTick = pHttpCmd->uQueueTick;
    }
    return(pAllocRef);
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerGetCmd

    \Description
        Find http command referenced by specified handle.

    \Input *pHttpManager    - reference pointer
    \Input iHandle          - transaction handle

    \Output
        HttpManagerHttpCmdT *   - requested command

    \Version 05/20/2009 (jbrookes)
*/
/********************************************************************************F*/
static HttpManagerHttpCmdT *_HttpManagerGetCmd(HttpManagerRefT *pHttpManager, int32_t iHandle)
{
    HttpManagerHttpCmdT *pHttpCmd;
    int32_t iHttpCmd;

    // walk cmd list and find transaction with given handle
    for (iHttpCmd = 0, pHttpCmd = NULL; iHttpCmd < HTTPMANAGER_MAXCMDS; iHttpCmd += 1)
    {
        if (pHttpManager->HttpCmds[iHttpCmd].iHttpHandle == iHandle)
        {
            pHttpCmd = &pHttpManager->HttpCmds[iHttpCmd];
            break;
        }
    }
    return(pHttpCmd);
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerUpdateTransactionStats

    \Description
        Update stats based on a single transaction request.

    \Input *pHttpManager    - reference pointer
    \Input *pHttpCmd        - transaction command

    \Version 05/25/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpManagerUpdateTransactionStats(HttpManagerRefT *pHttpManager, HttpManagerHttpCmdT *pHttpCmd)
{
    uint32_t uLatency;

    pHttpManager->HttpManagerStats.uNumActiveTransactions += 1;
    if (pHttpManager->HttpManagerStats.uMaxActiveTransactions < pHttpManager->HttpManagerStats.uNumActiveTransactions)
    {
        pHttpManager->HttpManagerStats.uMaxActiveTransactions = pHttpManager->HttpManagerStats.uNumActiveTransactions;
    }
    pHttpCmd->uIssueTick = NetTick();
    uLatency = NetTickDiff(pHttpCmd->uIssueTick, pHttpCmd->uQueueTick);
    if (pHttpManager->HttpManagerStats.uMaxQueueWaitLatency < uLatency)
    {
        pHttpManager->HttpManagerStats.uMaxQueueWaitLatency = uLatency;
    }
    pHttpManager->HttpManagerStats.uSumQueueWaitLatency += uLatency;
    pHttpManager->HttpManagerStats.uNumTransactions += 1;
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerResizeInputBuffer

    \Description
        After PROTOHTTP_MINBUFF response, increase the input buffer size for the given
        ProtoHttp ref to allow the request buffer space to be reissued successfully.

    \Input *pHttpManager    - reference pointer
    \Input *pHttpRef        - http ref to resize

    \Output
        int32_t             - zero=success, else failure

    \Version 02/15/20011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpManagerResizeInputBuffer(HttpManagerRefT *pHttpManager, HttpManagerHttpRefT *pHttpRef)
{
    int32_t iBuffSize, iCurrSize, iReqdSize;

    // get current and required sizes
    if ((iCurrSize = ProtoHttpStatus(pHttpRef->pProtoHttp, 'imax', NULL, 0)) <= 0)
    {
        // something wrong with HTTP ref... can happen if HTTP ref is tromped, so we bail out here to prevent infinite loop below
        NetPrintf(("httpmanager: ProtoHttpStats(0x%08x, 'imax') returned %d\n", (uintptr_t)pHttpRef->pProtoHttp, iCurrSize));
        return(-1);
    }
    iReqdSize = ProtoHttpStatus(pHttpRef->pProtoHttp, 'iovr', NULL, 0);

    // calc new buffer size in increments of original buffer size
    for (iBuffSize = iCurrSize; iBuffSize < iReqdSize; iBuffSize += iCurrSize)
        ;

    // resize the input buffer
    return(ProtoHttpControl(pHttpRef->pProtoHttp, 'ires', iBuffSize, 0, NULL));
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerRequestIssue

    \Description
        Issue an HTTP request

    \Input *pHttpManager    - reference pointer
    \Input *pHttpCmd        - transaction command
    \Input *pData           - data associated with command, or NULL if none
    \Input iDataSize        - size of data associated with command, or zero if none

    \Output
        int32_t             - ProtoHttpRequest() result

    \Version 05/25/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpManagerRequestIssue(HttpManagerRefT *pHttpManager, HttpManagerHttpCmdT *pHttpCmd, const char *pData, int64_t iDataSize)
{
    int32_t iResult;

    // initiate the request, we only specify the custom header callback as the rest are implemented within this module
    if ((iResult = ProtoHttpRequestCb2(pHttpCmd->pHttpRef->pProtoHttp, pHttpCmd->pUrl, pData, iDataSize, (ProtoHttpRequestTypeE)pHttpCmd->uRequestType, NULL, pHttpCmd->pCustomHeaderCb, NULL, pHttpCmd->pUserData)) < 0)
    {
        return(iResult);
    }

    NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: [%d] started  ref %d url=%s\n", pHttpCmd->iHttpHandle, pHttpCmd->pHttpRef - pHttpManager->HttpRefs, pHttpCmd->pUrl));

    // if pipelining is enabled, a ProtoHttpGet request with NULL Url is required to flush the previous request
    if (pHttpManager->bPipelining && ((pHttpCmd->uRequestType == PROTOHTTP_REQUESTTYPE_HEAD) || (pHttpCmd->uRequestType == PROTOHTTP_REQUESTTYPE_GET)))
    {
        ProtoHttpGet(pHttpCmd->pHttpRef->pProtoHttp, NULL, 0);
    }

    // update stats
    _HttpManagerUpdateTransactionStats(pHttpManager, pHttpCmd);

    // update status
    pHttpCmd->uState = HTTPMANAGER_CMDSTATE_ACTV;
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerRequestStart

    \Description
        Set up any initial transaction-specific settings and issue a GET or HEAD
        request.

    \Input *pHttpManager    - reference pointer
    \Input *pHttpCmd        - transaction command
    \Input *pData           - pointer to URL data (optional, may be NULL)
    \Input iDataSize        - size of data being uploaded (see Notes)

    \Output
        int32_t             - ProtoHttpRequest() result

    \Version 05/21/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpManagerRequestStart(HttpManagerRefT *pHttpManager, HttpManagerHttpCmdT *pHttpCmd, const char *pData, int64_t iDataSize)
{
    int32_t iResult;

    // make sure ref is idle
    #if DIRTYCODE_LOGGING
    if (pHttpCmd->pHttpRef->uHttpState != HTTPMANAGER_REFSTATE_IDLE)
    {
        NetPrintf(("httpmanager: error; get request issued on non-idle ref\n"));
    }
    #endif

    // explicitly disable keep-alive if global setting has it unavailable
    if (pHttpManager->bKeepalive == FALSE)
    {
        ProtoHttpControl(pHttpCmd->pHttpRef->pProtoHttp, 'keep', 0, 0, NULL);
    }
    // set timeout if requested
    if (pHttpCmd->iTimeout != 0)
    {
        ProtoHttpControl(pHttpCmd->pHttpRef->pProtoHttp, 'time', pHttpCmd->iTimeout, 0, NULL);
    }
    // apply append header
    _HttpManagerApplyAppendHeader(pHttpManager, pHttpCmd);

    // update httpref status
    pHttpCmd->pHttpRef->uHttpState = HTTPMANAGER_REFSTATE_BUSY;
    pHttpCmd->pHttpRef->iCurTransaction = 0;

    // initiate the request
    if ((iResult = _HttpManagerRequestIssue(pHttpManager, pHttpCmd, pData, iDataSize)) < 0)
    {
        // if there wasn't enough input buffer space to store the request, realloc the buffer and try again
        if ((iResult == PROTOHTTP_MINBUFF) && (_HttpManagerResizeInputBuffer(pHttpManager, pHttpCmd->pHttpRef) == 0))
        {
            // resized input buffer; try to issue the request again
            iResult = _HttpManagerRequestIssue(pHttpManager, pHttpCmd, pData, iDataSize);
        }

        // fail on error
        if (iResult < 0)
        {
            NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: ProtoHttpRequest() returned %d on handle %d url=%s\n", iResult, pHttpCmd->iHttpHandle, pHttpCmd->pUrl));
            pHttpCmd->uState = HTTPMANAGER_CMDSTATE_FAIL;
        }
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerRequestCheckCompletion

    \Description
        Check for transaction completion, and update state and stats accordingly.

    \Input *pHttpManager    - reference pointer
    \Input *pHttpCmd        - transaction command

    \Version 07/03/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpManagerRequestCheckCompletion(HttpManagerRefT *pHttpManager, HttpManagerHttpCmdT *pHttpCmd)
{
    HttpManagerHttpRefT *pHttpRef = pHttpCmd->pHttpRef;
    int32_t iHeadSize, iStatus;
    #if DIRTYCODE_LOGGING
    int32_t iRsltCode = 0;
    #endif

    // are we done yet?
    if ((iStatus = ProtoHttpStatus(pHttpRef->pProtoHttp, 'done', NULL, 0)) == 0)
    {
        // not done yet
        return;
    }
    else if (iStatus == 1)
    {
        int64_t iBodySize;
        ProtoHttpStatus(pHttpRef->pProtoHttp, 'body', &iBodySize, sizeof(iBodySize));
        if (iBodySize != (signed)pHttpCmd->uBytesXfer)
        {
            // transaction is complete, but data has not all been received by caller
            return;
        }

        #if DIRTYCODE_LOGGING
        iRsltCode = ProtoHttpStatus(pHttpRef->pProtoHttp, 'code', NULL, 0);
        #endif
    }

    // remember completion time and mark transaction as complete
    pHttpCmd->uComplTick = NetTick();
    pHttpCmd->uState = HTTPMANAGER_CMDSTATE_DONE;

    // get size of header
    if ((iHeadSize = ProtoHttpStatus(pHttpRef->pProtoHttp, 'head', NULL, 0)) < 0)
    {
        NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: could not get header size for stat tracking (err=%d)\n", iHeadSize));

        // downgrade our pipelining support $$todo$$ - this should be tracked per site
        if (pHttpManager->bPipeWithoutKeepAlive)
        {
            /*$$todo: this can happen for other reasons (e.g. cert untrusted on a secure connection)
                but we should only be downgrading pipe without keep-alive if we get a generic socket
                failure or a premature Connection: Close */
            NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager; disabling pipelining without keep-alive\n"));
            pHttpManager->bPipeWithoutKeepAlive = FALSE;
        }

        // reset the pipeline
        _HttpManagerPipelineReset(pHttpManager, pHttpRef, 1);

        iHeadSize = 0;
    }

    // update transaction stats
    pHttpCmd->uBytesXfer += (unsigned)iHeadSize;
    pHttpManager->HttpManagerStats.uTransactionBytes  += pHttpCmd->uBytesXfer;
    pHttpManager->HttpManagerStats.uTransactionTime   += NetTickDiff(pHttpCmd->uComplTick, pHttpCmd->uQueueTick);

    // log transaction results
    NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: [%d] %s ref %d url=%s (%d): %qd bytes in %dms, %.2fk/sec)\n", pHttpCmd->iHttpHandle,
        (iStatus == 1) ? "complete" : "failed  ", pHttpRef - pHttpManager->HttpRefs, pHttpCmd->pUrl, iRsltCode, pHttpCmd->uBytesXfer, NetTickDiff(pHttpCmd->uComplTick, pHttpCmd->uIssueTick),
        ((float)pHttpCmd->uBytesXfer * 1000.0f) / (float)(NetTickDiff(pHttpCmd->uComplTick, pHttpCmd->uIssueTick)*1024)));
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerRequestGetCmdQueueIdx

    \Description
        Get index of given command in httpref command queue.

    \Input *pHttpManager    - reference pointer
    \Input *pHttpRef        - http ref
    \Input *pHttpCmd        - transaction command

    \Output
        int32_t             - index of command in command queue; -1 if not in queue

    \Version 07/03/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpManagerRequestGetCmdQueueIdx(HttpManagerRefT *pHttpManager, HttpManagerHttpRefT *pHttpRef, HttpManagerHttpCmdT *pHttpCmd)
{
    int32_t iHttpCmd;
    for (iHttpCmd = 0; (iHttpCmd < HTTPMANAGER_MAXREFQUEUE) && (pHttpRef->HttpCmdQueue[iHttpCmd] != pHttpCmd); iHttpCmd += 1)
        ;
    if (iHttpCmd == HTTPMANAGER_MAXREFQUEUE)
    {
        NetPrintf(("httpmanager: could not find handle %d in ref %d command queue!\n", pHttpCmd->iHttpHandle, pHttpRef-pHttpManager->HttpRefs));
        return(-1);
    }
    return(iHttpCmd);
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerRequestDelCmdFromQueue

    \Description
        Remove command from command queue and update ref state accordingly

    \Input *pHttpManager    - reference pointer
    \Input *pHttpRef        - httpref pointer
    \Input *pHttpCmd        - transaction command

    \Todo
        Possible issues that need to be considered:
        1) out-of-order command deletion may not be handled correctly

    \Version 07/03/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpManagerRequestDelCmdFromQueue(HttpManagerRefT *pHttpManager, HttpManagerHttpRefT *pHttpRef, HttpManagerHttpCmdT *pHttpCmd)
{
    int32_t iHttpCmd;

    // update transaction count
    pHttpRef->iTransactions -= 1;

    // update ref state
    if ((pHttpRef->iTransactions == 0) || (pHttpRef->HttpCmdQueue[1]->uState == HTTPMANAGER_CMDSTATE_WAIT))
    {
        pHttpRef->uHttpState = HTTPMANAGER_REFSTATE_IDLE;
    }

    // update stat tracking
    if (pHttpRef->iTransactions > 0)
    {
        pHttpManager->HttpManagerStats.uNumQueuedTransactions -= 1;
    }
    if (pHttpManager->HttpManagerStats.uNumActiveTransactions > 0)
    {
        pHttpManager->HttpManagerStats.uNumActiveTransactions -= 1;
    }

    // sanity check -- should never have an iCurTransaction != 0 here
    if (pHttpRef->iCurTransaction != 0)
    {
        NetPrintf(("httpmanager: warning -- current transaction index is not zero!\n", pHttpRef->iCurTransaction));
    }

    // find ourselves in the HttpRef queue
    if ((iHttpCmd = _HttpManagerRequestGetCmdQueueIdx(pHttpManager, pHttpRef, pHttpCmd)) < 0)
    {
        NetPrintf(("httpmanager: error -- could not find handle %d in ref %d queue\n", pHttpCmd->iHttpHandle, pHttpRef - pHttpManager->HttpRefs));
        return;
    }

    // remove command from ref command queue
    if (iHttpCmd < pHttpRef->iTransactions)
    {
        NetPrintfVerbose((pHttpManager->iVerbose, 1, "httpmanager: contracting transaction queue for ref %d\n", pHttpCmd->pHttpRef - pHttpManager->HttpRefs));
        memmove(&pHttpRef->HttpCmdQueue[iHttpCmd], &pHttpRef->HttpCmdQueue[iHttpCmd+1], sizeof(pHttpRef->HttpCmdQueue[0]) * (pHttpRef->iTransactions - iHttpCmd));
    }
    pHttpRef->HttpCmdQueue[pHttpRef->iTransactions] = NULL;
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerRequestPipe

    \Description
        Pipeline the given request.

    \Input *pHttpManager    - reference pointer
    \Input *pHttpRef        - http ref to use for transaction
    \Input *pHttpCmd        - transaction command

    \Output
        int32_t             - ProtoHttpRequest() result

    \Version 07/01/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpManagerRequestPipe(HttpManagerRefT *pHttpManager, HttpManagerHttpRefT *pHttpRef, HttpManagerHttpCmdT *pHttpCmd)
{
    const char *pUrl;
    uint32_t uRequestType;
    int32_t iResult;

    // set up Url and request type
    if (pHttpCmd != NULL)
    {
        pUrl = pHttpCmd->pUrl;
        uRequestType = pHttpCmd->uRequestType;
    }
    else
    {
        pUrl = NULL;
        uRequestType = 0;
    }

    // explicitly disable keep-alive if global setting has it unavailable
    if (pHttpManager->bKeepalive == FALSE)
    {
        ProtoHttpControl(pHttpRef->pProtoHttp, 'keep', 0, 0, NULL);
    }

    /* set current transaction index.  we have to do this before ProtoHttpRequest() as that
       will call into the custom header callback, which relies on the index being accurate. */
    if ((pHttpRef->uHttpState == HTTPMANAGER_REFSTATE_IDLE) || (pUrl == NULL))
    {
        pHttpRef->iCurTransaction = 0;
    }
    else
    {
        pHttpRef->iCurTransaction += 1;
    }

    // set timeout if requested (only for the first request, since this is a global setting)
    if ((pHttpCmd != NULL) && (pHttpCmd->iTimeout != 0) && (pHttpRef->iCurTransaction == 0))
    {
        ProtoHttpControl(pHttpCmd->pHttpRef->pProtoHttp, 'time', pHttpCmd->iTimeout, 0, NULL);
    }

    // apply append header as long as this isn't a pipeline flush
    if (pHttpCmd != NULL)
    {
        _HttpManagerApplyAppendHeader(pHttpManager, pHttpCmd);
    }

    // initiate the request
    if ((iResult = ProtoHttpRequest(pHttpRef->pProtoHttp, pUrl, NULL, 0, (ProtoHttpRequestTypeE)uRequestType)) >= 0)
    {
        if (pHttpCmd != NULL)
        {
            // update stats
            _HttpManagerUpdateTransactionStats(pHttpManager, pHttpCmd);

            // if this is the first request on this ref, mark it as busy and set up the first transaction handler
            if (pHttpRef->uHttpState == HTTPMANAGER_REFSTATE_IDLE)
            {
                pHttpRef->uHttpState = HTTPMANAGER_REFSTATE_BUSY;
                pHttpCmd->uState = HTTPMANAGER_CMDSTATE_ACTV;
            }
            else
            {
                pHttpCmd->uState = HTTPMANAGER_CMDSTATE_PIPE;
                pHttpManager->HttpManagerStats.uNumPipelinedTransactions += 1;
            }
        }
        /* if this was a flush command, reset current command, now that all of the headers
           have been formatted, so received header callbacks will get correct ref */
        else
        {
            pHttpRef->iCurTransaction = 0;
        }
    }
    else if (pHttpRef->iCurTransaction > 0)
    {
        pHttpRef->iCurTransaction -= 1;
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerRequestPipeUpdateQueue

    \Description
        Update queue upon completion of a piped request.

    \Input *pHttpManager    - reference pointer
    \Input *pHttpRef        - http ref to use for transaction
    \Input *pHttpCmd        - transaction command

    \Version 07/03/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpManagerRequestPipeUpdateQueue(HttpManagerRefT *pHttpManager, HttpManagerHttpRefT *pHttpRef, HttpManagerHttpCmdT *pHttpCmd)
{
    if ((pHttpRef->iTransactions > 0) && (pHttpRef->HttpCmdQueue[pHttpRef->iCurTransaction]->uState == HTTPMANAGER_CMDSTATE_PIPE))
    {
        // if the previous command completed successfully, set state of next piped transaction to ACTV
        if (pHttpCmd->uState == HTTPMANAGER_CMDSTATE_DONE)
        {
            // set next handle to active state
            NetPrintfVerbose((pHttpManager->iVerbose, 1, "httpmanager: handle %d set to active state\n", pHttpRef->HttpCmdQueue[pHttpRef->iCurTransaction]->iHttpHandle));
            pHttpRef->HttpCmdQueue[pHttpRef->iCurTransaction]->uState = HTTPMANAGER_CMDSTATE_ACTV;
            // allow protohttp to get the next piped transaction
            ProtoHttpControl(pHttpRef->pProtoHttp, 'pnxt', 0, 0,NULL);
        }
        else // previous command did not complete; so we have to reset the pipe
        {
            int32_t iHttpCmd;
            /* if this transaction was in a sequence of pipelined transactions, we need
               to reset the state of the following transactions so they will be reissued */
            NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: resetting state of piped requests following deletion of handle %d in state %d\n",
                pHttpCmd->iHttpHandle, pHttpCmd->uState));
            for (iHttpCmd = pHttpRef->iCurTransaction; iHttpCmd < pHttpRef->iTransactions; iHttpCmd += 1)
            {
                NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: handle %d reset from state %d to state %d\n", pHttpRef->HttpCmdQueue[iHttpCmd]->iHttpHandle,
                    pHttpRef->HttpCmdQueue[iHttpCmd]->uState, HTTPMANAGER_CMDSTATE_WAIT));
                pHttpRef->HttpCmdQueue[iHttpCmd]->uState = HTTPMANAGER_CMDSTATE_WAIT;
            }
            // reset httpref to idle status
            pHttpRef->uHttpState = HTTPMANAGER_REFSTATE_IDLE;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerCmdReleaseRef

    \Description
        Releases an HttpRef from the HttpCmd it is associated with.  This allows
        any queued or piped transactions waiting on this ref to proceed.

    \Input *pHttpManager    - reference pointer
    \Input *pHttpCmd        - http command

    \Version 07/10/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpManagerCmdReleaseRef(HttpManagerRefT *pHttpManager, HttpManagerHttpCmdT *pHttpCmd)
{
    uint32_t uFreeLatency;

    // no ref allocated?
    if (pHttpCmd->pHttpRef == NULL)
    {
        return;
    }

    // remove command from command queue
    _HttpManagerRequestDelCmdFromQueue(pHttpManager, pHttpCmd->pHttpRef, pHttpCmd);

    // update queue for piped operations
    _HttpManagerRequestPipeUpdateQueue(pHttpManager, pHttpCmd->pHttpRef, pHttpCmd);

    // if our request is currently active, abort it
    if (ProtoHttpStatus(pHttpCmd->pHttpRef->pProtoHttp, 'done', NULL, 0) == 0)
    {
        ProtoHttpAbort(pHttpCmd->pHttpRef->pProtoHttp);
    }

    // update time spent waiting to be freed
    if ((pHttpCmd->uComplTick != 0) && (pHttpCmd->uState != HTTPMANAGER_CMDSTATE_FAIL))
    {
        uFreeLatency = NetTickDiff(NetTick(), pHttpCmd->uComplTick);
    }
    else
    {
        NetPrintfVerbose((pHttpManager->iVerbose, 1, "httpmanager: warning -- no completion timestamp for handle %d being deleted (%dms since queued)\n",
            pHttpCmd->iHttpHandle, NetTickDiff(NetTick(), pHttpCmd->uQueueTick)));
        NetPrintfVerbose((pHttpManager->iVerbose, 1, "httpmanager:    url=%s\n", pHttpCmd->pUrl));
        NetPrintfVerbose((pHttpManager->iVerbose, 1, "httpmanager:    state=%d\n", pHttpCmd->uState));
        NetPrintfVerbose((pHttpManager->iVerbose, 1, "httpmanager:    data=%qd\n", pHttpCmd->uBytesXfer));
        uFreeLatency = 0;
    }
    pHttpManager->HttpManagerStats.uSumQueueFreeLatency += uFreeLatency;
    if (pHttpManager->HttpManagerStats.uMaxQueueFreeLatency < uFreeLatency)
    {
        pHttpManager->HttpManagerStats.uMaxQueueFreeLatency = uFreeLatency;
    }
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerPipeline

    \Description
        Try to pipeline a set of requests.

    \Input *pHttpManager    - reference pointer
    \Input *pHttpRef        - ref to execute commands on
    \Input *pHttpCmd        - first command in ref command queue

    \Output
        int32_t             - zero=no requests issued

    \Version 06/08/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpManagerPipeline(HttpManagerRefT *pHttpManager, HttpManagerHttpRefT *pHttpRef, HttpManagerHttpCmdT *pHttpCmd)
{
    HttpManagerHttpCmdT *pHttpCmd2;
    int32_t iHttpCmd, iResult;

    // not pipelining?
    if (!pHttpManager->bPipelining)
    {
        return(0);
    }

    // make sure request type can be pipelined
    if ((pHttpCmd->uRequestType != PROTOHTTP_REQUESTTYPE_GET) && (pHttpCmd->uRequestType != PROTOHTTP_REQUESTTYPE_HEAD))
    {
        return(0);
    }

    // if we require keep-alive to pipeline, check keep-alive status
    if (!pHttpManager->bPipeWithoutKeepAlive && (ProtoHttpCheckKeepAlive(pHttpRef->pProtoHttp, pHttpCmd->pUrl) == 0))
    {
        return(0);
    }

    // execute the first transaction
    if ((iResult = _HttpManagerRequestPipe(pHttpManager, pHttpRef, pHttpCmd)) >= 0)
    {
        NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: [%d] started  ref %d url=%s\n", pHttpCmd->iHttpHandle, pHttpRef - pHttpManager->HttpRefs, pHttpCmd->pUrl));

        // iterate through transaction queue and issue any pending requests that are queued for pipelining
        for (iHttpCmd = 1; (iHttpCmd < pHttpRef->iTransactions) && (iHttpCmd < pHttpManager->iMaxPipedUrls); iHttpCmd += 1)
        {
            // ref next transaction in queue
            pHttpCmd2 = pHttpRef->HttpCmdQueue[iHttpCmd];

            // check to see requests pipeline, and if so try and pipeline it
            if (_HttpManagerPipelineCheck(pHttpCmd, pHttpCmd2) == 0)
            {
                break;
            }
            if ((iResult = _HttpManagerRequestPipe(pHttpManager, pHttpRef, pHttpCmd2)) < 0)
            {
                NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: could not pipe handle %d; request will be retried on a new ref\n", pHttpCmd2->iHttpHandle));
                break;
            }
            NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: [%d] piped    ref %d url=%s\n", pHttpCmd2->iHttpHandle, pHttpRef - pHttpManager->HttpRefs, pHttpCmd2->pUrl));

            // update trailing ref
            pHttpCmd = pHttpCmd2;
        }

        // issue queued request(s)
        _HttpManagerRequestPipe(pHttpManager, pHttpRef, NULL);
    }
    else if ((iResult == PROTOHTTP_MINBUFF) && (_HttpManagerResizeInputBuffer(pHttpManager, pHttpRef) == 0))
    {
        // resized input buffer, let _HttpManagerRequestCb() re-issue it for us without pipelining
        return(0);
    }
    else
    {
        NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: ProtoHttpRequest() returned %d on handle %d url=%s\n", iResult, pHttpCmd->iHttpHandle, pHttpCmd->pUrl));
        pHttpRef->uHttpState = HTTPMANAGER_REFSTATE_BUSY;
        pHttpRef->iCurTransaction = 0;
        pHttpCmd->uState = HTTPMANAGER_CMDSTATE_FAIL;
    }
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerRequestCb

    \Description
        Handle an HTTP request.

    \Input *pHttpManager    - reference pointer
    \Input iHandle          - transaction handle
    \Input *pUrl            - the URL that identifies the POST action.
    \Input *pData           - pointer to URL data (optional, may be NULL)
    \Input iDataSize        - size of data being uploaded
    \Input eRequestType     - http request type
    \Input *pWriteCb        - write callback (optional)
    \Input *pCustomHeaderCb - custom header callback (optional)
    \Input *pReceiveHeaderCb- receive header callback (optional)
    \Input *pUserData       - user data for callbacks (optional)

    \Output
        int32_t             - negative=error, else number of bytes written

    \Version 06/08/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpManagerRequestCb(HttpManagerRefT *pHttpManager, int32_t iHandle, const char *pUrl, const char *pData, int64_t iDataSize, ProtoHttpRequestTypeE eRequestType, ProtoHttpWriteCbT *pWriteCb, ProtoHttpCustomHeaderCbT *pCustomHeaderCb, ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb, void *pUserData)
{
    HttpManagerHttpCmdT *pHttpCmd;

    // get referenced transaction
    if ((pHttpCmd = _HttpManagerGetCmd(pHttpManager, iHandle)) == NULL)
    {
        NetPrintf(("httpmanager: unrecognized transaction %d in request\n", iHandle));
        return(-1);
    }
    // remember request timestamp
    pHttpCmd->uQueueTick = NetTick();
    // store request info
    pHttpCmd->uRequestType = (uint8_t)eRequestType;
    pHttpCmd->pWriteCb = pWriteCb;
    pHttpCmd->pCustomHeaderCb = pCustomHeaderCb;
    pHttpCmd->pReceiveHeaderCb = pReceiveHeaderCb;
    pHttpCmd->pUserData = pUserData;

    // make a copy of url?
    if (pHttpManager->bCopyUrl)
    {
        int32_t iStrLen = (int32_t)strlen(pUrl);
        pHttpCmd->pUrl = DirtyMemAlloc(iStrLen+1, HTTPMGR_MEMID, pHttpManager->iMemGroup, pHttpManager->pMemGroupUserData);
        ds_strnzcpy((char *)pHttpCmd->pUrl, pUrl, iStrLen+1);
        pHttpCmd->bCopiedUrl = TRUE;
    }
    else
    {
        pHttpCmd->pUrl = pUrl;
        pHttpCmd->bCopiedUrl = FALSE;
    }

    // allocate a ref for the transaction
    if (_HttpManagerAllocRef(pHttpManager, pHttpCmd) == NULL)
    {
        // unable to allocate ref, but HttpManager will try again
        return(0);
    }

    /* if this is the only transaction queued and we are not pipelining, or the request type
       is not a GET or a HEAD (the only request types we pipeline) */
    if (((pHttpCmd->pHttpRef->iTransactions == 1) && !pHttpManager->bPipelining) ||
        ((eRequestType != PROTOHTTP_REQUESTTYPE_GET) && (eRequestType != PROTOHTTP_REQUESTTYPE_HEAD)))
    {
        // issue the request
        return(_HttpManagerRequestStart(pHttpManager, pHttpCmd, pData, iDataSize));
    }
    else
    {
        pHttpCmd->uState = HTTPMANAGER_CMDSTATE_WAIT;
        return(0);
    }
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerIdle

    \Description
        NetConn idle function to update the HttpManager module.

    \Input *pData   - pointer to module state
    \Input uTick    - current tick count

    \Notes
        This function is installed as a NetConn Idle function.  NetConnIdle()
        must be regularly polled for this function to be called.

    \Version 1.0 07/23/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpManagerIdle(void *pData, uint32_t uTick)
{
    HttpManagerRefT *pHttpManager = (HttpManagerRefT *)pData;
    if (pHttpManager->bAutoUpdate)
    {
        HttpManagerUpdate(pHttpManager);
    }
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerCreateRef

    \Description
        Create a new http ref

    \Input *pHttpManager    - module state
    \Input iHttpRef         - index of ref to create

    \Output
        int32_t             - zero=success, negative=failure

    \Version 01/31/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpManagerCreateRef(HttpManagerRefT *pHttpManager, int32_t iHttpRef)
{
    HttpManagerHttpRefT *pHttpRef = &pHttpManager->HttpRefs[iHttpRef];

    // create protohttp ref
    if ((pHttpRef->pProtoHttp = ProtoHttpCreate(pHttpManager->iHttpBufSize)) == NULL)
    {
        NetPrintf(("httpmanager: could not allocate http ref %d\n", iHttpRef));
        return(-1);
    }
    // temporarily snuff debug output
    ProtoHttpControl(pHttpRef->pProtoHttp, 'spam', 0, 0, NULL);

    // default keep-alive to enabled
    ProtoHttpControl(pHttpRef->pProtoHttp, 'keep', 1, 0, NULL);
    // set pipelining enable/disable
    ProtoHttpControl(pHttpRef->pProtoHttp, 'pipe', pHttpManager->bPipelining, 0, NULL);
    // set up callbacks
    ProtoHttpCallback(pHttpRef->pProtoHttp, pHttpManager->pCustomHeaderCb ? _HttpManagerCustomHeaderCb : NULL, _HttpManagerReceiveHeaderCb, pHttpRef);
    // set default debug level
    ProtoHttpControl(pHttpRef->pProtoHttp, 'spam', pHttpManager->iVerbose, 0, NULL);

    // init to idle state
    pHttpRef->uHttpState = HTTPMANAGER_REFSTATE_IDLE;
    // init last access time
    pHttpRef->uLastTick = NetTick();
    // return success to caller
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerDestroyRef

    \Description
        Destroy an allocated http ref

    \Input *pHttpManager    - module state
    \Input iHttpRef         - index of ref to destroy

    \Version 01/31/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpManagerDestroyRef(HttpManagerRefT *pHttpManager, int32_t iHttpRef)
{
    HttpManagerHttpRefT *pHttpRef = &pHttpManager->HttpRefs[iHttpRef];
    HttpManagerHttpCmdT *pHttpCmd;
    int32_t iTransaction;

    // early out if ref is null
    if (pHttpRef->pProtoHttp == NULL)
    {
        return;
    }

    // invalidate any commands that referenced this ref
    for (iTransaction = 0; iTransaction < pHttpRef->iTransactions; iTransaction += 1)
    {
        pHttpCmd = pHttpRef->HttpCmdQueue[iTransaction];
        if (pHttpCmd->pHttpRef == pHttpRef)
        {
            NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: destroying active ref %2d; handle %d is being terminated\n", iHttpRef, pHttpCmd->iHttpHandle));
            pHttpCmd->pHttpRef = NULL;
            pHttpCmd->uState = HTTPMANAGER_CMDSTATE_FAIL;
        }
    }

    // destroy ref
    NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: destroying http ref %d\n", iHttpRef));
    ProtoHttpDestroy(pHttpRef->pProtoHttp);
    // clear state
    ds_memclr(pHttpRef, sizeof(*pHttpRef));
}

/*F********************************************************************************/
/*!
    \Function _HttpManagerSizeRefPool

    \Description
        Create or destroy ProtoHttp refs based on current pool size and new pool size

    \Input *pHttpManager    - module state
    \Input iHttpNumRefs     - number of desired protohttp refs

    \Output
        int32_t             - zero=success, negative=failure

    \Version 01/31/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpManagerSizeRefPool(HttpManagerRefT *pHttpManager, int32_t iHttpNumRefs)
{
    int32_t iHttpRef;

    // validate request size
    if (iHttpNumRefs > HTTPMANAGER_MAXREFS)
    {
        NetPrintf(("httpmanager: clamping 'pool' request to max %d refs\n", HTTPMANAGER_MAXREFS));
        iHttpNumRefs = HTTPMANAGER_MAXREFS;
    }
    else if (iHttpNumRefs < 1)
    {
        NetPrintf(("httpmanager: clamping 'pool' request to min of one ref\n"));
        iHttpNumRefs = 1;
    }

    // increase size of ref pool?
    if (pHttpManager->iHttpNumRefs < iHttpNumRefs)
    {
        int32_t iResult;
        DirtyMemGroupEnter(pHttpManager->iMemGroup, pHttpManager->pMemGroupUserData);
        for (iHttpRef = pHttpManager->iHttpNumRefs, iResult = 0; iHttpRef < iHttpNumRefs; iHttpRef += 1)
        {
            if ((iResult = _HttpManagerCreateRef(pHttpManager, iHttpRef)) < 0)
            {
                break;
            }
        }
        DirtyMemGroupLeave();
        if (iResult < 0)
        {
            // could not allocate http ref
            return(-1);
        }
    }
    else if (pHttpManager->iHttpNumRefs > iHttpNumRefs) // decrease size of ref pool
    {
        for (iHttpRef = pHttpManager->iHttpNumRefs - 1; iHttpRef >= iHttpNumRefs; iHttpRef -= 1)
        {
            _HttpManagerDestroyRef(pHttpManager, iHttpRef);
        }
    }

    // save new ref count, return success
    pHttpManager->iHttpNumRefs = iHttpNumRefs;
    return(0);
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function HttpManagerCreate

    \Description
        Allocate module state and prepare for use

    \Input iHttpBufSize     - length of receive buffer for each protohttp ref
    \Input iHttpNumRefs     - number of protohttp modules to allocate

    \Output
        HttpManagerRefT *   - pointer to module state, or NULL

    \Version 05/20/2009 (jbrookes)
*/
/********************************************************************************F*/
HttpManagerRefT *HttpManagerCreate(int32_t iHttpBufSize, int32_t iHttpNumRefs)
{
    HttpManagerRefT *pHttpManager;
    void *pMemGroupUserData;
    int32_t iMemGroup;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // clamp refcount
    if (iHttpNumRefs > HTTPMANAGER_MAXREFS)
    {
        NetPrintf(("httpmanager: %d requested refs exceeds max of %d; clamping\n", iHttpNumRefs, HTTPMANAGER_MAXREFS));
        iHttpNumRefs = HTTPMANAGER_MAXREFS;
    }

    // allocate the module state
    if ((pHttpManager = DirtyMemAlloc(sizeof(*pHttpManager), HTTPMGR_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("httpmanager: unable to allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pHttpManager, sizeof(*pHttpManager));

    // save parms & set defaults
    pHttpManager->iMemGroup = iMemGroup;
    pHttpManager->pMemGroupUserData = pMemGroupUserData;
    pHttpManager->iHttpBufSize = iHttpBufSize;
    pHttpManager->iHttpHandle = 1;  // zero is reserved as invalid
#if !defined(DIRTYCODE_XBOXONE) && !defined(DIRTYCODE_PS4) && !defined(DIRTYCODE_GDK)
    pHttpManager->bPipelining = TRUE;
#endif
    pHttpManager->bKeepalive = TRUE;
    pHttpManager->bPipeWithoutKeepAlive = TRUE;
    pHttpManager->iMaxPipedUrls = 4;
    pHttpManager->bCopyUrl = TRUE;
    pHttpManager->bAutoUpdate = TRUE;
    pHttpManager->iVerbose = 1;

    // allocate protohttp refs
    if (_HttpManagerSizeRefPool(pHttpManager, iHttpNumRefs) < 0)
    {
        NetPrintf(("httpmanager: could not allocate ref pool\n"));
        HttpManagerDestroy(pHttpManager);
        return(NULL);
    }

    // add httpmanager task handle
    NetConnIdleAdd(_HttpManagerIdle, pHttpManager);

    #if HTTPMANAGER_FINALDEBUG
    _HttpManager_iStartTick = NetTick();
    #endif

    // return new module state
    return(pHttpManager);
}

/*F********************************************************************************/
/*!
    \Function HttpManagerCallback

    \Description
        Set header callbacks.

    \Input *pHttpManager    - module state
    \Input *pCustomHeaderCb - pointer to custom send header callback
    \Input *pReceiveHeaderCb- pointer to recv header callback

    \Notes
        See ProtoHttpCallback documentation for a description of the callbacks
        specified here.

    \Version 05/20/2009 (jbrookes)
*/
/********************************************************************************F*/
void HttpManagerCallback(HttpManagerRefT *pHttpManager, ProtoHttpCustomHeaderCbT *pCustomHeaderCb, ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb)
{
    HttpManagerHttpRefT *pHttpRef;
    int32_t iHttpRef;

    // save callback info
    pHttpManager->pCustomHeaderCb = pCustomHeaderCb;
    pHttpManager->pReceiveHeaderCb = pReceiveHeaderCb;

    // update protohttp refs with callback info
    for (iHttpRef = 0; iHttpRef < pHttpManager->iHttpNumRefs; iHttpRef += 1)
    {
        pHttpRef = &pHttpManager->HttpRefs[iHttpRef];
        ProtoHttpCallback(pHttpRef->pProtoHttp, pHttpManager->pCustomHeaderCb ? _HttpManagerCustomHeaderCb : NULL, _HttpManagerReceiveHeaderCb, pHttpRef);
    }
}

/*F********************************************************************************/
/*!
    \Function HttpManagerDestroy

    \Description
        Destroy the module and release its state

    \Input *pHttpManager    - reference pointer

    \Version 05/20/2009 (jbrookes)
*/
/********************************************************************************F*/
void HttpManagerDestroy(HttpManagerRefT *pHttpManager)
{
    HttpManagerHttpCmdT *pHttpCmd;
    int32_t iHttpCmd, iHttpRef;

    // del httpmanager task handle
    NetConnIdleDel(_HttpManagerIdle, pHttpManager);

    // if append header is set, free it
    if (pHttpManager->pAppendHdr != NULL)
    {
        DirtyMemFree(pHttpManager->pAppendHdr, HTTPMGR_MEMID, pHttpManager->iMemGroup, pHttpManager->pMemGroupUserData);
    }

    // destroy protohttp modules
    for (iHttpRef = 0; iHttpRef < pHttpManager->iHttpNumRefs; iHttpRef += 1)
    {
        _HttpManagerDestroyRef(pHttpManager, iHttpRef);
    }

    // clean up command list
    for (iHttpCmd = 0; iHttpCmd < HTTPMANAGER_MAXCMDS; iHttpCmd += 1)
    {
        pHttpCmd = &pHttpManager->HttpCmds[iHttpCmd];
        if (pHttpCmd->iHttpHandle != 0)
        {
            NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: warning; handle %d not cleaned up\n", pHttpCmd->iHttpHandle));
            HttpManagerFree(pHttpManager, pHttpCmd->iHttpHandle);
        }
    }

    // display stats
    #if DIRTYCODE_LOGGING || HTTPMANAGER_FINALDEBUG
    if (pHttpManager->iVerbose > 0)
    {
        _HttpManagerDisplayStats(pHttpManager);
    }
    #endif

    // destroy module state
    DirtyMemFree(pHttpManager, HTTPMGR_MEMID, pHttpManager->iMemGroup, pHttpManager->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function HttpManagerAlloc

    \Description
        Allocate a new transaction handle

    \Input *pHttpManager    - reference pointer

    \Output
        int32_t             - negative=failure, else transaction handle

    \Version 05/20/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t HttpManagerAlloc(HttpManagerRefT *pHttpManager)
{
    HttpManagerHttpCmdT *pHttpCmd;

    // get an unallocated ProtoHttp ref
    if ((pHttpCmd = _HttpManagerAllocCmd(pHttpManager)) == NULL)
    {
        NetPrintf(("httpmanager: could not allocate new http transaction for HttpManagerGet() request\n"));
        return(-1);
    }
    // return transaction handle to caller
    NetPrintfVerbose((pHttpManager->iVerbose, 2, "httpmanager: allocated handle %d\n", pHttpCmd->iHttpHandle));
    return(pHttpCmd->iHttpHandle);
}

/*F********************************************************************************/
/*!
    \Function HttpManagerFree

    \Description
        Release a new transaction handle

    \Input *pHttpManager    - reference pointer
    \Input iHandle          - transaction handle

    \Version 05/20/2009 (jbrookes)
*/
/********************************************************************************F*/
void HttpManagerFree(HttpManagerRefT *pHttpManager, int32_t iHandle)
{
    HttpManagerHttpCmdT *pHttpCmd;

    // get referenced transaction
    if ((pHttpCmd = _HttpManagerGetCmd(pHttpManager, iHandle)) == NULL)
    {
        NetPrintf(("httpmanager: unrecognized transaction %d in HttpManagerFree()\n", iHandle));
        return;
    }
    NetPrintfVerbose((pHttpManager->iVerbose, 2, "httpmanager: releasing handle %d\n", iHandle));

    // release the ref
    _HttpManagerCmdReleaseRef(pHttpManager, pHttpCmd);
    // free url, if it was copied
    if (pHttpCmd->bCopiedUrl == TRUE)
    {
        if (pHttpCmd->pUrl != NULL)
        {
            DirtyMemFree((void *)pHttpCmd->pUrl, HTTPMGR_MEMID, pHttpManager->iMemGroup, pHttpManager->pMemGroupUserData);
        }
        else
        {
            NetPrintf(("httpmanager: warning; copied url null before free\n"));
        }
    }
    // free append buffer, if present
    if (pHttpCmd->pAppendHdr != NULL)
    {
        DirtyMemFree(pHttpCmd->pAppendHdr, HTTPMGR_MEMID, pHttpManager->iMemGroup, pHttpManager->pMemGroupUserData);
    }

    // clear ref
    ds_memclr(pHttpCmd, sizeof(*pHttpCmd));
}

/*F********************************************************************************/
/*!
    \Function HttpManagerGet

    \Description
        Initiate an HTTP transaction. Pass in a URL and the module starts a transfer
        from the appropriate server.

    \Input *pHttpManager    - module state
    \Input iHandle          - transaction handle
    \Input *pUrl            - the url to retrieve
    \Input bHeadOnly        - if TRUE only get header

    \Output
        int32_t             - negative=failure, else success

    \Version 05/20/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t HttpManagerGet(HttpManagerRefT *pHttpManager, int32_t iHandle, const char *pUrl, uint32_t bHeadOnly)
{
    return(_HttpManagerRequestCb(pHttpManager, iHandle, pUrl, NULL, 0, bHeadOnly ? PROTOHTTP_REQUESTTYPE_HEAD : PROTOHTTP_REQUESTTYPE_GET, NULL, NULL, NULL, NULL));
}

/*F********************************************************************************/
/*!
    \Function HttpManagerRecv

    \Description
        Return the actual url data.

    \Input *pHttpManager    - reference pointer
    \Input iHandle          - transaction handle (returned by HttpManagerGet())
    \Input *pBuffer         - buffer to store data in
    \Input iBufMin          - minimum number of bytes to return (returns zero if this number is not available)
    \Input iBufMax          - maximum number of bytes to return (buffer size)

    \Output
        int32_t             - negative=error, zero=no data available or bufmax <= 0, positive=number of bytes read

    \Version 05/20/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t HttpManagerRecv(HttpManagerRefT *pHttpManager, int32_t iHandle, char *pBuffer, int32_t iBufMin, int32_t iBufMax)
{
    HttpManagerHttpCmdT *pHttpCmd;

    // get referenced transaction
    if ((pHttpCmd = _HttpManagerGetCmd(pHttpManager, iHandle)) == NULL)
    {
        NetPrintf(("httpmanager: unrecognized transaction %d in HttpManagerRecv()\n", iHandle));
        return(-1);
    }

    // wait until active
    if (pHttpCmd->uState < HTTPMANAGER_CMDSTATE_ACTV)
    {
        return(0);
    }

    // early-out if queued execution of request failed
    if (pHttpCmd->uState == HTTPMANAGER_CMDSTATE_FAIL)
    {
        return(PROTOHTTP_RECVFAIL);
    }

    // update the ref
    ProtoHttpUpdate(pHttpCmd->pHttpRef->pProtoHttp);

    // execute the receive
    if ((pHttpCmd->iResult = ProtoHttpRecv(pHttpCmd->pHttpRef->pProtoHttp, pBuffer, iBufMin, iBufMax)) > 0)
    {
        // track bytes received
        pHttpCmd->uBytesXfer += (unsigned)pHttpCmd->iResult;
    }
    else if ((pHttpCmd->iResult == PROTOHTTP_MINBUFF) && (_HttpManagerResizeInputBuffer(pHttpManager, pHttpCmd->pHttpRef) == 0))
    {
        // resized input buffer; swallow the error result and try again next go-around
        pHttpCmd->iResult = 0;
    }

    // check for transaction completion
    if (pHttpCmd->uState == HTTPMANAGER_CMDSTATE_ACTV)
    {
        _HttpManagerRequestCheckCompletion(pHttpManager, pHttpCmd);
    }

    // update last access time
    {
        uint32_t uCurTick = NetTick();
        if (NetTickDiff(uCurTick, pHttpCmd->pHttpRef->uLastTick) > 1000)
        {
            NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: warning -- ref %d last updated %dms previous (receive)\n", pHttpCmd->pHttpRef - pHttpManager->HttpRefs, NetTickDiff(uCurTick, pHttpCmd->pHttpRef->uLastTick)));
        }
        pHttpCmd->pHttpRef->uLastTick = uCurTick;
    }

    // return result to caller
    return(pHttpCmd->iResult);
}

/*F********************************************************************************/
/*!
    \Function HttpManagerRecvAll

    \Description
        Return all of the url data.

    \Input *pHttpManager    - reference pointer
    \Input iHandle          - transaction handle (returned by HttpManagerGet())
    \Input *pBuffer         - buffer to store data in
    \Input iBufSize         - size of buffer

    \Output
        int32_t             - PROTOHTTP_RECV*, or positive=bytes in response

    \Version 05/20/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t HttpManagerRecvAll(HttpManagerRefT *pHttpManager, int32_t iHandle, char *pBuffer, int32_t iBufSize)
{
    HttpManagerHttpCmdT *pHttpCmd;

    // get referenced transaction
    if ((pHttpCmd = _HttpManagerGetCmd(pHttpManager, iHandle)) == NULL)
    {
        NetPrintf(("httpmanager: unrecognized transaction %d in HttpManagerRecv()\n", iHandle));
        return(-1);
    }

    // if transaction is not active, return no data available
    if (pHttpCmd->uState != HTTPMANAGER_CMDSTATE_ACTV)
    {
        return(0);
    }

    // pass through to http ref
    if ((pHttpCmd->iResult =  ProtoHttpRecvAll(pHttpCmd->pHttpRef->pProtoHttp, pBuffer, iBufSize)) == PROTOHTTP_MINBUFF)
    {
        // increase the input buffer size, for next go-around
        if (_HttpManagerResizeInputBuffer(pHttpManager, pHttpCmd->pHttpRef) == 0)
        {
            // swallow the error result and try again next go-around
            pHttpCmd->iResult = 0;
        }
    }
    // return result to caller
    return(pHttpCmd->iResult);
}

/*F********************************************************************************/
/*!
    \Function HttpManagerPost

    \Description
        Initiate transfer of data to to the server via a HTTP POST command.

    \Input *pHttpManager    - reference pointer
    \Input iHandle          - transaction handle
    \Input *pUrl            - the URL that identifies the POST action.
    \Input *pData           - pointer to URL data (optional, may be NULL)
    \Input iDataSize        - size of data being uploaded (see Notes)
    \Input bDoPut           - if TRUE, do a PUT instead of a POST

    \Output
        int32_t             - negative=failure, else number of data bytes sent

    \Notes
        See ProtoHttpPost() documentation.

    \Version 05/20/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t HttpManagerPost(HttpManagerRefT *pHttpManager, int32_t iHandle, const char *pUrl, const char *pData, int64_t iDataSize, uint32_t bDoPut)
{
    return(_HttpManagerRequestCb(pHttpManager, iHandle, pUrl, pData, iDataSize, bDoPut ? PROTOHTTP_REQUESTTYPE_PUT : PROTOHTTP_REQUESTTYPE_POST, NULL, NULL, NULL, NULL));
}

/*F********************************************************************************/
/*!
    \Function HttpManagerSend

    \Description
        Send data during an ongoing Post transaction.

    \Input *pHttpManager    - reference pointer
    \Input iHandle          - transaction handle
    \Input *pData           - pointer to data to send
    \Input iDataSize        - size of data being sent

    \Output
        int32_t             - negative=failure, else number of data bytes sent

    \Version 05/20/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t HttpManagerSend(HttpManagerRefT *pHttpManager, int32_t iHandle, const char *pData, int32_t iDataSize)
{
    HttpManagerHttpCmdT *pHttpCmd;

    // get referenced transaction
    if ((pHttpCmd = _HttpManagerGetCmd(pHttpManager, iHandle)) == NULL)
    {
        NetPrintf(("httpmanager: unrecognized transaction %d in HttpManagerRecv()\n", iHandle));
        return(-1);
    }

    // if transaction is not active, return no data available
    if (pHttpCmd->uState != HTTPMANAGER_CMDSTATE_ACTV)
    {
        return(0);
    }

    // pass through to http ref
    return(ProtoHttpSend(pHttpCmd->pHttpRef->pProtoHttp, pData, iDataSize));
}

/*F********************************************************************************/
/*!
    \Function HttpManagerRequestCb2

    \Description
        Make an HTTP request.

    \Input *pHttpManager    - reference pointer
    \Input iHandle          - transaction handle
    \Input *pUrl            - the URL that identifies the POST action.
    \Input *pData           - pointer to URL data (optional, may be NULL)
    \Input iDataSize        - size of data being uploaded
    \Input eRequestType     - http request type
    \Input *pWriteCb        - write callback (optional)
    \Input *pCustomHeaderCb - custom header callback (optional)
    \Input *pReceiveHeaderCb- receive header callback (optional)
    \Input *pUserData       - user data for write callback

    \Output
        int32_t             - negative=error, else number of bytes written

    \Version 09/11/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t HttpManagerRequestCb2(HttpManagerRefT *pHttpManager, int32_t iHandle, const char *pUrl, const char *pData, int64_t iDataSize, ProtoHttpRequestTypeE eRequestType, ProtoHttpWriteCbT *pWriteCb, ProtoHttpCustomHeaderCbT *pCustomHeaderCb, ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb, void *pUserData)
{
    return(_HttpManagerRequestCb(pHttpManager, iHandle, pUrl, pData, iDataSize, eRequestType, pWriteCb, pCustomHeaderCb, pReceiveHeaderCb, pUserData));
}

/*F********************************************************************************/
/*!
    \Function HttpManagerSetBaseUrl

    \Description
        Set base url that will be used for any relative url references.

    \Input *pHttpManager    - reference pointer
    \Input iHandle          - handle to set base url for
    \Input *pUrl            - base url

    \Version 02/03/2010 (jbrookes)
*/
/********************************************************************************F*/
void HttpManagerSetBaseUrl(HttpManagerRefT *pHttpManager, int32_t iHandle, const char *pUrl)
{
    HttpManagerHttpCmdT *pHttpCmd;
    // get referenced transaction
    if ((pHttpCmd = _HttpManagerGetCmd(pHttpManager, iHandle)) == NULL)
    {
        NetPrintf(("httpmanager: unrecognized transaction %d in HttpManagerSetBaseUrl()\n", iHandle));
        return;
    }
    // pass through to http ref
    if ((pHttpCmd->pHttpRef != NULL) && (pHttpCmd->pHttpRef->pProtoHttp != NULL))
    {
        ProtoHttpSetBaseUrl(pHttpCmd->pHttpRef->pProtoHttp, pUrl);
    }
}

/*F********************************************************************************/
/*!
    \Function HttpManagerControl

    \Description
        HttpManager control function.  Different selectors control different
        behaviors.

    \Input *pHttpManager    - reference pointer
    \Input iHandle          - transaction handle
    \Input iSelect          - control selector
    \Input iValue           - selector specific
    \Input iValue2          - selector specific
    \Input *pValue          - selector specific

    \Output
        int32_t             - selector specific

    \Notes
        If a handle is specified (iHandle > 0), HttpManagerControl() will forward
        the call to ProtoHttpControl() for the appropriate ref.  HttpManager control
        selectors are as follows:

        \verbatim
            SELECTOR    DESCRIPTION
            'apnd'      Header append feature (see ProtoHttp doc for details).  If a
                        handle is specified the scope is command-specific, otherwise
                        it is global.  A command-specific append header takes
                        precedence over a global append header.
            'auto'      enable/disable auto-update (polling of HttpManagerUpdate by
                        NetConnIdle(); default enabled)
            'cbup'      sets callback user data pointer (pValue=callback)
            'copy'      sets if urls are copied internally or not (default=true)
            'maxp'      sets max pipeline depth (default 4)
            'pipe'      pipelining enable/disable
            'pwka'      pipelining without keep-alive enable/disable
            'spam'      manager-level debug verbosity
            'stcl'      clear httpmanager stats.
        \endverbatim

    \Version 05/20/2009 (jbrookes)
*/
/*******************************************************************************F*/
int32_t HttpManagerControl(HttpManagerRefT *pHttpManager, int32_t iHandle, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue)
{
    HttpManagerHttpCmdT *pHttpCmd = NULL;
    int32_t iHttpRef;

    // if we are given a handle, resolve http cmd
    if ((iHandle > 0) && ((pHttpCmd = _HttpManagerGetCmd(pHttpManager, iHandle)) == NULL))
    {
        NetPrintf(("httpmanager: HttpManagerControl(%d, '%C') failed; unrecognized handle\n", iHandle, iSelect));
        return(-1);
    }

    // commands that work with or without a cmd

    // set append header
    if (iSelect == 'apnd')
    {
        return(_HttpManagerSetAppendHeader(pHttpManager, pHttpCmd, pValue));
    }

    // command-specific selector
    if (pHttpCmd != NULL)
    {
        // put httpmanager control selectors here
        if (pHttpCmd->uState == HTTPMANAGER_CMDSTATE_IDLE)
        {
            // set callback user data pointer
            if (iSelect == 'cbup')
            {
                NetPrintfVerbose((pHttpManager->iVerbose, 2, "httpmanager: setting callback for handle %d to 0x%08x\n", iHandle, pValue));
                pHttpCmd->pCallbackRef = pValue;
                return(0);
            }
            // set timeout
            if (iSelect == 'time')
            {
                pHttpCmd->iTimeout = iValue;
                return(0);
            }
        }

        // if it's not an httpmanager control selector, and we have a ProtoHttp ref, pass it through to ProtoHttp
        if ((pHttpCmd->pHttpRef != NULL) && (pHttpCmd->pHttpRef->pProtoHttp != NULL))
        {
            return(ProtoHttpControl(pHttpCmd->pHttpRef->pProtoHttp, iSelect, iValue, iValue2, pValue));
        }
    }
    else // global (not command-specific) selectors
    {
        // enable/disable auto-update (polling of HttpManagerUpdate by NetConnIdle())
        if (iSelect == 'auto')
        {
            NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: auto-update %s\n", iValue ? "enabled" : "disabled"));
            pHttpManager->bAutoUpdate = iValue ? TRUE : FALSE;
            return(0);
        }
        // copy url setting enable/disable
        if (iSelect == 'copy')
        {
            NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: urlcopy %s\n", iValue ? "enabled" : "disabled"));
            pHttpManager->bCopyUrl = iValue ? TRUE : FALSE;
            return(0);
        }
        // set max pipeline depth
        if (iSelect == 'maxp')
        {
            NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: setting max pipeline depth to %d\n", iValue));
            pHttpManager->iMaxPipedUrls = (int8_t)iValue;
            return(0);
        }
        // set pipelining setting
        #if !defined(DIRTYCODE_XBOXONE) && !defined(DIRTYCODE_PS4) && !defined(DIRTYCODE_GDK)
        if (iSelect == 'pipe')
        {
            NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: pipelining %s\n", iValue ? "enabled" : "disabled"));
            pHttpManager->bPipelining = iValue ? TRUE : FALSE;
            // intentional fall-through to pass down to protohttp refs
        }
        #endif
        if (iSelect == 'pool')
        {
            return(_HttpManagerSizeRefPool(pHttpManager, iValue));
        }
        // set pipelining without keep-alive
        if (iSelect == 'pwka')
        {
            NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: pipelining without keep-alive %s\n", iValue ? "enabled" : "disabled"));
            pHttpManager->bPipeWithoutKeepAlive = iValue ? TRUE : FALSE;
            return(0);
        }
        // manager-level debug verbosity
        if (iSelect == 'spam')
        {
            pHttpManager->iVerbose = iValue;
            if (iValue > 0)
            {
                // set protohttp spam level one lower than httpmanager's
                iValue -= 1;
            }
            // intentional fall-through to pass down to protohttp refs
        }
        // clear stats
        if (iSelect == 'stcl')
        {
            _HttpManagerResetStats(pHttpManager);
            return(0);
        }
        
        // if we haven't gotten it by now, it's an option to be applied to all http refs
        for (iHttpRef = 0; iHttpRef < pHttpManager->iHttpNumRefs; iHttpRef += 1)
        {
            if (pHttpManager->HttpRefs[iHttpRef].pProtoHttp != NULL)
            {
                ProtoHttpControl(pHttpManager->HttpRefs[iHttpRef].pProtoHttp, iSelect, iValue, iValue2, pValue);
            }
        }
        return(0);
    }
    // unhandled
    NetPrintf(("httpmanager: HttpManagerControl(%d, '%C') unhandled\n", iHandle, iSelect));
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function HttpManagerStatus

    \Description
        Return status of current HTTP transfer.  Status type depends on selector.

    \Input *pHttpManager    - reference pointer
    \Input iHandle          - transaction handle
    \Input iSelect          - info selector (see Notes)
    \Input *pBuffer         - [in/out] input and/or storage for selector-specific output
    \Input iBufSize         - size of buffer

    \Output
        int32_t             - selector specific

    \Notes
        If a handle is specified, HttpManagerStatus() will call ProtoHttpStatus() for
        the appropriate ref.  HttpManager status selectors are as follows:

        \verbatim
            SELECTOR    DESCRIPTION
            'busy'      Returns number of busy refs
            'hndl'      Get HttpManager handle from ProtoHttp ref (passed in pBuffer)
            'href'      Get ProtoHttp ref from HttpManager handle (returned in pBuffer, if specified)
            'stat'      If pBuffer is NULL, display httpmanager stats (debug only); else copy stats to output buffer
            'urls'      Copies url from specified command into output buffer
        \endverbatim

    \Version 05/20/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t HttpManagerStatus(HttpManagerRefT *pHttpManager, int32_t iHandle, int32_t iSelect, void *pBuffer, int32_t iBufSize)
{
    // if the user wants status of a specific transaction?
    if (iHandle > 0)
    {
        HttpManagerHttpCmdT *pHttpCmd;

        // get referenced transaction
        if ((pHttpCmd = _HttpManagerGetCmd(pHttpManager, iHandle)) == NULL)
        {
            NetPrintf(("httpmanager: HttpManagerStatus(%d, '%C') failed; unrecognized handle\n", iHandle, iSelect));
            return(-1);
        }

        // get href from handle
        if (iSelect == 'href')
        {
            if ((pHttpCmd->pHttpRef != NULL) && (pHttpCmd->pHttpRef->pProtoHttp != NULL))
            {
                if ((pBuffer != NULL) && (iBufSize == sizeof(void *)))
                {
                    ds_memcpy(pBuffer, &pHttpCmd->pHttpRef->pProtoHttp, sizeof(void *));
                }
                return(0);
            }
            return(-1);
        }

        // copy url to status buffer
        if (iSelect == 'urls')
        {
            ds_strnzcpy(pBuffer, pHttpCmd->pUrl, iBufSize);
            return(0);
        }

        // only allow status of active transactions (otherwise this transaction does not own the ref)
        if (pHttpCmd->uState < HTTPMANAGER_CMDSTATE_ACTV)
        {
            if ((iSelect == 'done') || (iSelect == 'data') || (iSelect == 'time'))
            {
                return(0);
            }
            if ((iSelect == 'body') || (iSelect == 'code') || (iSelect == 'htxt'))
            {
                return(-1);
            }
            NetPrintf(("httpmanager: HttpManagerStatus(%d, '%C') failed; handle not active or done (state=%d)\n", iHandle, iSelect, pHttpCmd->uState));
            return(-1);
        }

        // if it's not an httpmanager status selector, and we have a ProtoHttp ref, pass it through to ProtoHttp
        if ((pHttpCmd->pHttpRef != NULL) && (pHttpCmd->pHttpRef->pProtoHttp != NULL))
        {
            return(ProtoHttpStatus(pHttpCmd->pHttpRef->pProtoHttp, iSelect, pBuffer, iBufSize));
        }
    }
    else
    {
        // return count of the number of busy refs
        if (iSelect == 'busy')
        {
            HttpManagerHttpRefT *pHttpRef;
            int32_t iHttpRef, iBusyRefs;

            for (iHttpRef = 0, iBusyRefs = 0; iHttpRef < pHttpManager->iHttpNumRefs; iHttpRef += 1)
            {
                pHttpRef = &pHttpManager->HttpRefs[iHttpRef];
                if (pHttpRef->uHttpState == HTTPMANAGER_REFSTATE_BUSY)
                {
                    iBusyRefs += 1;
                }
            }
            return(iBusyRefs);
        }
        // get handle from href
        if (iSelect == 'hndl')
        {
            if ((pBuffer != NULL) && (iBufSize == sizeof(void *)))
            {
                HttpManagerHttpRefT *pHttpRef;
                ProtoHttpRefT *pProtoHttpFind;
                int32_t iHttpRef;

                // get httpref from buffer
                ds_memcpy(&pProtoHttpFind, pBuffer, sizeof(void *));

                // find it
                for (iHttpRef = 0; iHttpRef < pHttpManager->iHttpNumRefs; iHttpRef += 1)
                {
                    pHttpRef = &pHttpManager->HttpRefs[iHttpRef];
                    if ((pHttpRef->pProtoHttp == pProtoHttpFind) && (pHttpRef->HttpCmdQueue[pHttpRef->iCurTransaction] != NULL))
                    {
                        // return active handle
                        return(pHttpRef->HttpCmdQueue[pHttpRef->iCurTransaction]->iHttpHandle);
                    }
                }
            }
            // not found
            return(-1);
        }
        // display/fetch stats
        if (iSelect == 'stat')
        {
            if (pBuffer != NULL)
            {
                if (iBufSize == sizeof(pHttpManager->HttpManagerStats))
                {
                    ds_memcpy(pBuffer, &pHttpManager->HttpManagerStats, sizeof(pHttpManager->HttpManagerStats));
                    return(0);
                }
                else
                {
                    return(-1);
                }
            }
            #if DIRTYCODE_LOGGING || HTTPMANAGER_FINALDEBUG
            else
            {
                _HttpManagerDisplayStats(pHttpManager);
            }
            #endif
            return(0);
        }
    }
    // unhandled
    NetPrintf(("httpmanager: HttpManagerStatus(%d, '%C') unhandled\n", iHandle, iSelect));
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function HttpManagerUpdate

    \Description
        Give time to module to do its thing (should be called periodically to
        allow module to perform work)

    \Input *pHttpManager    - reference pointer

    \Version 05/20/2009 (jbrookes)
*/
/********************************************************************************F*/
void HttpManagerUpdate(HttpManagerRefT *pHttpManager)
{
    HttpManagerHttpRefT *pHttpRef;
    HttpManagerHttpCmdT *pHttpCmd;
    int32_t iHttpCmd, iHttpRef;

    // look for unassigned commands (these happen when we make a request, if we have filled up our available HttpRef command slots)
    for (iHttpCmd = 0; iHttpCmd < HTTPMANAGER_MAXCMDS; iHttpCmd += 1)
    {
        pHttpCmd = &pHttpManager->HttpCmds[iHttpCmd];
        if ((pHttpCmd->pUrl != NULL) && (pHttpCmd->uState == HTTPMANAGER_CMDSTATE_IDLE) && (pHttpCmd->pHttpRef == NULL))
        {
            // allocate a ref for the transaction
            if (_HttpManagerAllocRef(pHttpManager, pHttpCmd) == NULL)
            {
                break;
            }
            NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: deferred alloc for handle %d\n", pHttpCmd->iHttpHandle));
            // promote to wait state
            pHttpCmd->uState = HTTPMANAGER_CMDSTATE_WAIT;
        }
    }

    // update protohttp modules
    for (iHttpRef = 0; iHttpRef < pHttpManager->iHttpNumRefs; iHttpRef += 1)
    {
        uint32_t uCurTick;
        pHttpRef = &pHttpManager->HttpRefs[iHttpRef];

        // check for idle ref that has one or more transactions queued
        if ((pHttpRef->uHttpState == HTTPMANAGER_REFSTATE_IDLE) && (pHttpRef->iTransactions > 0))
        {
            // reference first queued transaction
            pHttpCmd = pHttpRef->HttpCmdQueue[0];

            // try to pipeline the command; if we can't, start the request
            if (_HttpManagerPipeline(pHttpManager, pHttpRef, pHttpCmd) == 0)
            {
                // execute first queued transaction
                _HttpManagerRequestStart(pHttpManager, pHttpCmd, NULL, 0);
            }
        }

        // update refs
        uCurTick = NetTick();
        ProtoHttpUpdate(pHttpRef->pProtoHttp);

        if (NetTickDiff(uCurTick, pHttpRef->uLastTick) > 1000)
        {
            NetPrintfVerbose((pHttpManager->iVerbose, 0, "httpmanager: warning -- ref %d last updated %dms previous (update)\n", iHttpRef, NetTickDiff(uCurTick, pHttpRef->uLastTick)));
        }
        pHttpRef->uLastTick = uCurTick;
    }

    // process any refs with write callbacks to update
    for (iHttpCmd = 0; iHttpCmd < HTTPMANAGER_MAXCMDS; iHttpCmd += 1)
    {
        pHttpCmd = &pHttpManager->HttpCmds[iHttpCmd];
        if ((pHttpCmd->pWriteCb != NULL) && (pHttpCmd->uState == HTTPMANAGER_CMDSTATE_ACTV))
        {
            _HttpManagerWriteCbProcess(pHttpManager, pHttpCmd);
        }
    }

}

