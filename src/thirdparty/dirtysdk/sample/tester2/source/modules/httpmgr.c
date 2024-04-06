/*H********************************************************************************/
/*!
    \File httpmgr.c

    \Description
        Implements basic http get and post client.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 10/28/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/proto/protohttpmanager.h"
#include "DirtySDK/proto/protossl.h"

#include "libsample/zlib.h"
#include "libsample/zmem.h"
#include "libsample/zfile.h"

#include "testermodules.h"

/*** Defines **********************************************************************/

#define HTTP_BUFSIZE    (100)
#define HTTP_RATE       (1)
#define HTTP_MAXCMDS    (64)        // max number of in-flight commands

#define HTTP_XTRAHDR0   ""
#define HTTP_XTRAHDR1   "X-Agent: DirtySock HTTP Tester\r\n"    // test "normal" replacement (replaces Accept: header)
#define HTTP_XTRAHDR2   "User-Agent: DirtySock HTTP Tester\r\n" // test "extended" replacement (replaces User-Agent: and Accept: headers)

//$$ tmp -- special test header used for hard-coded multipart/form-data testing -- this should be removed at some point when we have real multipart/form-data support
#define HTTP_XTRAHDR3   "Content-Type: multipart/form-data; boundary=TeStInG\r\n" \
                        "User-Agent: DirtySock HTTP Tester\r\n" \
                        "Accept: */*\r\n"

#define HTTP_APNDHDR    HTTP_XTRAHDR0

/*** Function Prototypes **********************************************************/

static int32_t _CmdHttpMgrIdleCB(ZContext *argz, int32_t argc, char *argv[]);

/*** Type Definitions *************************************************************/

typedef struct HttpStateT    // individual request states
{
    HttpManagerRefT *pHttpManager;
    enum
    {
        IDLE, DNLOAD, UPLOAD, MGET
    } state;
    int32_t iHandle;
    char strCookie[1024];
    int64_t iDataSize;
    int64_t iSentSize;
    int32_t iSendBufData;
    int32_t iSendBufSent;
    ZFileT iInpFile;
    ZFileT iOutFile;
    int32_t iOutSize;
    char *pOutData;
    int64_t show;
    int64_t count;
    int32_t sttime;
    uint8_t bStreaming;
    char strFileBuffer[16*1024];
}HttpStateT;

typedef struct HttpMgrRefT      // module state storage
{
    HttpManagerRefT *pHttpManager;
    int32_t iDebugLevel;
    const char *pMgetBuffer;
    const char *pMgetOffset;
    uint32_t uMgetStart;
    uint32_t uMgetTransactions;

    uint8_t bMgetShowUrlsOnly;
    uint8_t bRecvAll; // use recvall instead of recv
    uint8_t bUseWriteCb;
    uint8_t _pad;

    char strModuleName[32];
    char strMgetFilename[256];
    char strApndHdr[2048];

    // module state
    HttpStateT States[HTTP_MAXCMDS];

} HttpMgrRefT;

/*** Variables ********************************************************************/

static HttpMgrRefT _HttpMgr_Ref;
static uint8_t  _HttpMgr_bInitialized = FALSE;

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _HttpCheckHndlAndHref

    \Description
        Validate 'hndl' and 'href' status selectors

    \Input *pState      - transaction state
    \Input *pProtoHttp  - protohttpref to get handle for
    \Input *pLogStr     - string to identify caller

    \Version 10/03/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpCheckHndlAndHref(HttpStateT *pState, ProtoHttpRefT *pProtoHttp, const char *pLogStr)
{
    HttpMgrRefT *pRef = &_HttpMgr_Ref;
    ProtoHttpRefT *pProtoHttpCheck;
    int32_t iHandle, iResult;

    // get handle from href
    if ((iHandle = HttpManagerStatus(pState->pHttpManager, -1, 'hndl', &pProtoHttp, sizeof(pProtoHttp))) < 0)
    {
        ZPrintf("httpmgr: %s request could not get handle for href %p\n", pLogStr, pProtoHttp);
        return;
    }
    // get href from handle
    if ((iResult = HttpManagerStatus(pState->pHttpManager, iHandle, 'href', &pProtoHttpCheck, sizeof(pProtoHttpCheck))) < 0)
    {
        ZPrintf("httpmgr: %s request could not get href for handle %d\n", pLogStr, iHandle);
        return;
    }
    // make sure we got the right href
    if (pProtoHttp != pProtoHttpCheck)
    {
        ZPrintf("httpmgr: %s request got wrong href %p for handle %d (expected %p)\n", pLogStr, pProtoHttpCheck, iHandle, pProtoHttp);
        return;
    }
    // log success
    if (pRef->iDebugLevel > 1)
    {
        ZPrintf("httpmgr: processing %s request for handle %d (ref %p)\n", pLogStr, iHandle, pProtoHttp);
    }
}

/*F********************************************************************************/
/*!
    \Function _HttpCustomHeaderCallback

    \Description
        ProtoHttp send header callback.

    \Input *pProtoHttp  - protohttp module state
    \Input *pHeader     - received header
    \Input uHeaderSize  - header size
    \Input *pUserData   - user ref (HttpMgrRefT)

    \Output
        int32_t         - zero

    \Notes
        The header returned should be terminated by a *single* CRLF; ProtoHttp will
        append the final CRLF to complete the header.  The callback may return the
        size of the header, or zero, in which case ProtoHttp will calculate the
        headersize using strlen().

    \Version 02/24/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpCustomHeaderCallback(ProtoHttpRefT *pProtoHttp, char *pHeader, uint32_t uHeaderSize, const char *pData, int64_t iDataLen, void *pUserRef)
{
    HttpStateT *pState = (HttpStateT *)pUserRef;
    uint32_t uBufSize;
    char *pAppend;

    // check 'hndl' and 'href' status selectors
    _HttpCheckHndlAndHref(pState, pProtoHttp, "custom header");

    // find append point and calc free header space
    pAppend = pHeader + strlen(pHeader);
    uBufSize = uHeaderSize - (uint32_t)(pAppend - pHeader);

    // append our header info
#if !DIRTYCODE_XBOXONE
    if (pState->strCookie[0] != '\0')
    {
        /* $$todo -- cookies aren't really saved across multiple transactions, so that has
            to be solved before this will work */
        ds_snzprintf(pAppend, uBufSize, "Cookie: %s\r\n%s", pState->strCookie, HTTP_APNDHDR);
    }
    else
#endif
    {
        ds_strnzcpy(pAppend, "X-Append: Custom append test\r\n", uBufSize);
    }

    // recalc header size
    uHeaderSize = (uint32_t)strlen(pHeader);

    return(uHeaderSize);
}

/*F********************************************************************************/
/*!
    \Function _HttpRecvHeaderCallback

    \Description
        ProtoHttp recv header callback.

    \Input *pProtoHttp  - protohttp module state
    \Input *pHeader     - received header
    \Input uHeaderSize  - header size
    \Input *pUserData   - user ref (HttpMgrRefT)

    \Output
        None.

    \Version 02/24/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpRecvHeaderCallback(ProtoHttpRefT *pProtoHttp, const char *pHeader, uint32_t uHeaderSize, void *pUserRef)
{
    HttpMgrRefT *pRef = &_HttpMgr_Ref;
    HttpStateT *pState = (HttpStateT *)pUserRef;
    char strBuffer[1024], strName[128];
    const char *pHdrTmp;
    int32_t iLocnSize;

    // check 'hndl' and 'href' status selectors
    _HttpCheckHndlAndHref(pState, pProtoHttp, "receive header");

    // check for location header
    if ((iLocnSize = ProtoHttpGetHeaderValue(pProtoHttp, pHeader, "location", NULL, 0, NULL)) > 0)
    {
        if (pRef->iDebugLevel > 1)
        {
            ZPrintf("httpmgr: location header size=%d\n", iLocnSize);
        }
        if (ProtoHttpGetHeaderValue(pProtoHttp, pHeader, "location", strBuffer, sizeof(strBuffer), NULL) == 0)
        {
            if (pRef->iDebugLevel > 1)
            {
                ZPrintf("httpmgr: location url='%s'\n", strBuffer);
            }
        }
        else
        {
            ZPrintf("httpmgr: error querying location url\n");
        }
    }

    // test ProtoHttpGetNextHeader()
    for (pHdrTmp = pHeader; ProtoHttpGetNextHeader(pProtoHttp, pHdrTmp, strName, sizeof(strName), strBuffer, sizeof(strBuffer), &pHdrTmp) == 0; )
    {
        #if 0
        ZPrintf("httpmgr: ===%s: %s\n", strName, strBuffer);
        #endif
    }

    // parse any set-cookie requests
    for (pHdrTmp = pHeader; ProtoHttpGetHeaderValue(pProtoHttp, pHdrTmp, "set-cookie", strBuffer, sizeof(strBuffer), &pHdrTmp) == 0; )
    {
        // print the cookie
        if (pRef->iDebugLevel > 1)
        {
            ZPrintf("httpmgr: parsed cookie '%s'\n", strBuffer);
        }

        // add field seperator
        if (pState->strCookie[0] != '\0')
        {
            ds_strnzcat(pState->strCookie, ", ", sizeof(pState->strCookie));
        }
        // append to cookie list
        ds_strnzcat(pState->strCookie, strBuffer, sizeof(pState->strCookie));
    }

    // if we have any cookies, set them here for inclusion in next request
    if (pState->strCookie[0] != '\0')
    {
        // format append header
        ds_snzprintf(strBuffer, sizeof(strBuffer), "Cookie: %s\r\n%s", pState->strCookie, HTTP_APNDHDR);
        ProtoHttpControl(pProtoHttp, 'apnd', 0, 0, strBuffer);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _HttpMgrAllocState

    \Description
        Allocate an HttpStateT ref for tracking a transaction.

    \Input *pRef        - pointer to httpmgr ref

    \Output
        HttpStateT *    - allocated state, or NULL on failure

    \Version 02/15/2011 (jbrookes)
*/
/********************************************************************************F*/
static HttpStateT *_HttpMgrAllocState(HttpMgrRefT *pRef)
{
    HttpStateT *pState;
    int32_t iState;

    for (iState = 0; iState < HTTP_MAXCMDS; iState += 1)
    {
        pState = &pRef->States[iState];
        if (pState->iHandle == 0)
        {
            // clear any previous stats
            pState->count = 0;
            pState->show = 0;
            pState->iDataSize = 0;
            pState->iSentSize = 0;
            pState->bStreaming = FALSE;
            pState->iHandle = HttpManagerAlloc(pRef->pHttpManager);
            pState->pHttpManager = pRef->pHttpManager;

            // set callback user data pointer
            HttpManagerControl(pRef->pHttpManager, pState->iHandle, 'cbup', 0, 0, pState);

            // init start timer
            pState->sttime = NetTick();

            // return initialized state to caller
            return(pState);
        }
    }
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function _HttpMgrFreeState

    \Description
        Free an allocated HttpStateT ref.

    \Input *pRef        - pointer to httpmgr ref
    \Input *pState      - pointer to state to free


    \Version 02/18/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpMgrFreeState(HttpMgrRefT *pRef, HttpStateT *pState)
{
    // free handle
    HttpManagerFree(pRef->pHttpManager, pState->iHandle);
    // reset state memory
    ds_memclr(pState, sizeof(*pState));
}

/*F********************************************************************************/
/*!
    \Function _HttpMgrReallocBuff

    \Description
        Free an allocated HttpStateT ref.

    \Input *pRef        - pointer to httpmgr ref
    \Input *pState      - pointer to transaction state

    \Version 07/29/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpMgrReallocBuff(HttpMgrRefT *pRef, HttpStateT *pState)
{
    char *pNewData;
    int32_t iNewSize;

    // calc new buffer size
    if ((iNewSize = pState->iOutSize) == 0)
    {
        // try getting body size
        if ((iNewSize = HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'body', NULL, 0)) > 0)
        {
            // bump up buffer size for recvall null terminator
            //$$ TODO V9 -- why 2 required, not 1??
            iNewSize += 2;
        }
        else
        {
            // assign a fixed size, since we didn't get a body size
            iNewSize = 4096;
        }
    }
    else
    {
        iNewSize *= 2;
    }

    // allocate new buffer
    if ((pNewData = ZMemAlloc(iNewSize)) == NULL)
    {
        return;
    }
    // if realloc, copy old data and free old pointer
    if (pState->pOutData != NULL)
    {
        ds_memcpy(pNewData, pState->pOutData, pState->iOutSize);
        ZMemFree(pState->pOutData);
    }
    // save new pointer
    pState->pOutData = pNewData;
    pState->iOutSize = iNewSize;
}

/*F********************************************************************************/
/*!
    \Function _HttpMgrCheckComplete

    \Description
        See if the HTTP transaction is complete.

    \Input *pRef        - pointer to http ref
    \Input *pCmdName    - module name

    \Output
        int32_t         - completion status from ProtoHttpStatus() 'done' selector

    \Version 10/28/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpMgrCheckComplete(HttpManagerRefT *pHttpManager, HttpStateT *pState, const char *pCmdName)
{
    ProtoHttpResponseE eResponse;
    int32_t iResult;

    // wait for ref to be assigned before checking ref status
    if ((iResult = HttpManagerStatus(pHttpManager, pState->iHandle, 'href', NULL, 0)) < 0)
    {
        ZPrintf("httpmgr: waiting for httpref to be assigned\n");
        return(0);
    }
    // wait for header response
    if ((iResult = HttpManagerStatus(pHttpManager, pState->iHandle, 'head', NULL, 0)) == -2)
    {
        ZPrintf("httpmgr: waiting for head response (result=%d)\n", iResult);
        return(0);
    }
    // check for completion
    if ((iResult = HttpManagerStatus(pHttpManager, pState->iHandle, 'done', NULL, 0)) == 0)
    {
        ZPrintf("httpmgr: waiting for completion (result=%d)\n", iResult);
        return(0);
    }

    // get completion result
    eResponse = (ProtoHttpResponseE)HttpManagerStatus(pHttpManager, pState->iHandle, 'code', NULL, 0);
    switch (PROTOHTTP_GetResponseClass(eResponse))
    {
        case PROTOHTTP_RESPONSE_SUCCESSFUL:
            ZPrintf("%s: success (%d)\n", pCmdName, eResponse);
            break;

        case PROTOHTTP_RESPONSE_CLIENTERROR:
            ZPrintf("%s: client error %d\n", pCmdName, eResponse);
            break;

        case PROTOHTTP_RESPONSE_SERVERERROR:
            ZPrintf("%s: server error %d\n", pCmdName, eResponse);
            break;

        default:
            ZPrintf("%s: unexpected result code %d\n", pCmdName, eResponse);
            break;
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _HttpMgrMget

    \Description
        Process an mget request

    \Input *pRef        - module state

    \Version 02/16/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpMgrMget(HttpMgrRefT *pRef)
{
    const char *pHref = NULL, *pHref2, *pEndQuote;
    char strUrl[1024], strFile[1024];
    char *strArgs[4] = { "httpmgr", "get", "", "" };

    if (pRef->pMgetOffset != NULL)
    {
        // look for hrefs
        for (pHref = strstr(pRef->pMgetOffset, "href=\""); pHref != NULL; pHref = strstr(pHref2, "href=\""))
        {
            // skip href text
            pHref2 = pHref + 6;

            // find trailing quote
            if ((pEndQuote = strchr(pHref2, '"')) == NULL)
            {
                // skip it
                continue;
            }

            // copy the url
            ds_strsubzcpy(strUrl, sizeof(strUrl), pHref2, (int32_t)(pEndQuote-pHref2));

            // make sure it's a full URL
            if (strncmp(strUrl, "http://", 7) && strncmp(strUrl, "https://", 8))
            {
                // skip it
                continue;
            }

            // make a filename for the file, skipping http ref
            ds_snzprintf(strFile, sizeof(strFile), "%s-data\\%s", pRef->strMgetFilename, strUrl+7);

            // issue an http command
            strArgs[2] = strUrl;
            strArgs[3] = "";//strFile;

            if (!pRef->bMgetShowUrlsOnly)
            {
                if (CmdHttpMgr(NULL, 3, strArgs) != 0)
                {
                    // safe current url for next time around
                    pRef->pMgetOffset = pHref2;
                    break;
                }
                else
                {
                    pRef->uMgetTransactions += 1;
                }
            }
            else
            {
                ZPrintf("%s %s %s %s\n", strArgs[0], strArgs[1], strArgs[2], strArgs[3]);
            }
        }
    }

    // update HttpManager ($$note -- should we need to have this here?)
    HttpManagerUpdate(pRef->pHttpManager);

    // have we parsed the whole buffer?
    if (pHref == NULL)
    {
        // mark that we have completed parsing the buffer
        if (pRef->pMgetOffset != NULL)
        {
            pRef->pMgetOffset = NULL;
        }

        // are all of our transactions complete?
        if (HttpManagerStatus(pRef->pHttpManager, -1, 'busy', NULL, 0) == 0)
        {
            HttpManagerStatT MgetStats;

            // report time taken for mget request
            ZPrintf("httpmgr: mget completed in %dms (%d transactions)\n", NetTickDiff(NetTick(), pRef->uMgetStart), pRef->uMgetTransactions);

            // get and display stats
            if (HttpManagerStatus(pRef->pHttpManager, -1, 'stat', &MgetStats, sizeof(MgetStats)) == 0)
            {
                // display stats
                ZPrintf("httpmgr: mget transactions: %d\n", MgetStats.uNumTransactions);
                if (MgetStats.uNumTransactions > 0)
                {
                    ZPrintf("httpmgr: keepalive transactions: %d\n", MgetStats.uNumKeepAliveTransactions);
                    ZPrintf("httpmgr: pipelined transactions: %d\n", MgetStats.uNumPipelinedTransactions);
                    ZPrintf("httpmgr: max active transactions: %d\n", MgetStats.uMaxActiveTransactions);
                    ZPrintf("httpmgr: max queued transactions: %d\n", MgetStats.uMaxQueuedTransactions);
                    ZPrintf("httpmgr: sum queue wait time: %dms\n", MgetStats.uSumQueueWaitLatency);
                    ZPrintf("httpmgr: avg queue wait time: %dms\n", MgetStats.uSumQueueWaitLatency/MgetStats.uNumTransactions);
                    ZPrintf("httpmgr: max queue wait time: %dms\n", MgetStats.uMaxQueueWaitLatency);
                    ZPrintf("httpmgr: sum queue free time: %dms\n", MgetStats.uSumQueueFreeLatency);
                    ZPrintf("httpmgr: avg queue free time: %dms\n", MgetStats.uSumQueueFreeLatency/MgetStats.uNumTransactions);
                    ZPrintf("httpmgr: max queue free time: %dms\n", MgetStats.uMaxQueueFreeLatency);
                    ZPrintf("httpmgr: total bytes transferred: %d\n", MgetStats.uTransactionBytes);
                    ZPrintf("httpmgr: total transaction time: %d\n", MgetStats.uTransactionTime);
                    ZPrintf("httpmgr: avg bytes per second %.2f\n", (float)MgetStats.uTransactionBytes*1000.0f/(float)MgetStats.uTransactionTime);
                    ZPrintf("httpmgr: avg transaction size %.2f\n", (float)MgetStats.uTransactionBytes/(float)MgetStats.uNumTransactions);
                }
            }
            else
            {
                ZPrintf("%s: could not get stats\n", pRef->strModuleName);
            }

            // reset stats
            ZPrintf("httpmgr: resetting stats\n");
            HttpManagerControl(pRef->pHttpManager, -1, 'stcl', 0, 0, NULL);

            // dispose of mget buffer
            ZMemFree((void *)pRef->pMgetBuffer);
            pRef->pMgetBuffer = NULL;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _HttpMgrDownloadProcessData

    \Description
        Process data for a download transaction.

    \Input *pRef    - httpmanager state
    \Input *pState  - transaction state
    \Input iLen     - recv response

    \Version 07/02/2012 (jbrookes) split from _HttpMgrDownloadProcess()
*/
/********************************************************************************F*/
static void _HttpMgrDownloadProcessData(HttpMgrRefT *pRef, HttpStateT *pState, int32_t iLen)
{
    char strBuf[1024];

    // see if we should show progress
    if ((pState->count/1024) != (pState->show/1024))
    {
        pState->show = pState->count;
        if (pRef->iDebugLevel > 1)
        {
            ZPrintf("%s: downloaded %qd bytes\n", pRef->strModuleName, pState->count);
        }
    }

    // see if we are done
    if ((iLen < 0) && (iLen != PROTOHTTP_RECVWAIT))
    {
        // get the url we issued
        HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'urls', strBuf, sizeof(strBuf));

        // completed successfully?
        if ((iLen == PROTOHTTP_RECVDONE) || (iLen == PROTOHTTP_RECVHEAD))
        {
            if (pRef->iDebugLevel > 1)
            {
                int32_t iDlTime = NetTickDiff(NetTick(), pState->sttime);
                int32_t iHdrCode = HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'code', NULL, 0);

                ZPrintf("%s: %s download done (%d): %qd bytes in %.2f seconds (%.3f k/sec)\n", pRef->strModuleName, strBuf, iHdrCode, pState->count,
                    (float)iDlTime/1000.0f, ((float)pState->count * 1000.0f) / ((float)iDlTime * 1024.0f));
            }

            // display some header info (suppress if it's an mget, unless our debuglevel is high)
            if ((pRef->pMgetBuffer == NULL) || (pRef->iDebugLevel > 1))
            {
                if (HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'htxt', strBuf, sizeof(strBuf)) >= 0)
                {
                    ZPrintf("%s response header:\n%s\n", pRef->strModuleName, strBuf);
                }

                // display a couple of parsed fields
                if (HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'head', NULL, 0) > 0)
                {
                    time_t tLastMod = HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'date', NULL, 0);
                    const char *pTime;
                    int64_t iBodySize;

                    if (tLastMod != 0)
                    {
                        if ((pTime = ctime(&tLastMod)) != NULL)
                        {
                            ZPrintf("%s: Last-Modified: %s", pRef->strModuleName, pTime);
                        }
                    }
                    HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'body', &iBodySize, sizeof(iBodySize));
                    ZPrintf("%s: Content-Length: %qd\n", pRef->strModuleName, iBodySize);
                }
            }
        }
        else
        {
            int32_t iSslFail = HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'essl', NULL, 0);
            ZPrintf("%s: download failed (err=%d, sslerr=%d)\n", pRef->strModuleName, iLen, iSslFail);
            if ((iSslFail == PROTOSSL_ERROR_CERT_INVALID) || (iSslFail == PROTOSSL_ERROR_CERT_HOST) || (iSslFail == PROTOSSL_ERROR_CERT_NOTRUST))
            {
                ProtoSSLCertInfoT CertInfo;
                if (HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'cert', &CertInfo, sizeof(CertInfo)) == 0)
                {
                    ZPrintf("%s: cert failure (%d): (C=%s, ST=%s, L=%s, O=%s, OU=%s, CN=%s)\n", pRef->strModuleName, iSslFail,
                        CertInfo.Ident.strCountry, CertInfo.Ident.strState, CertInfo.Ident.strCity,
                        CertInfo.Ident.strOrg, CertInfo.Ident.strUnit, CertInfo.Ident.strCommon);
                }
                else
                {
                    ZPrintf("%s: could not get cert info\n", pRef->strModuleName);
                }
            }
        }

        // if file exists, close it
        if (pState->iOutFile > 0)
        {
            ZFileClose(pState->iOutFile);
        }
        pState->iOutFile = 0;

        // if output buffer exists, free it
        if (pState->pOutData != NULL)
        {
            pState->pOutData = NULL;
        }
        pState->iOutSize = 0;

        // free state tracking
        _HttpMgrFreeState(pRef, pState);
    }
}

/*F********************************************************************************/
/*!
    \Function _HttpMgrRecvData

    \Description
        Receive data from HttpManager using eithe HttpManagerRecv() or
        HttpManagerRecvAll().

    \Input *pRef    - httpmanager state
    \Input *pState  - transaction state

    \Version 07/02/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpMgrRecvData(HttpMgrRefT *pRef, HttpStateT *pState)
{
    char strBuf[16*1024];
    int32_t iLen;

    // check for data
    if (!pRef->bRecvAll)
    {
        while ((iLen = HttpManagerRecv(pRef->pHttpManager, pState->iHandle, strBuf, 1, sizeof(strBuf))) > 0)
        {
            pState->count += iLen;
            if (pState->iOutFile != 0)
            {
                ZFileWrite(pState->iOutFile, strBuf, iLen);
            }
        }
    }
    else
    {
        // receive all the data
        if ((iLen = HttpManagerRecvAll(pRef->pHttpManager, pState->iHandle, pState->pOutData, pState->iOutSize)) > 0)
        {
            pState->count = iLen;
            if (pState->iOutFile != 0)
            {
                ZFileWrite(pState->iOutFile, pState->pOutData, iLen);
            }
            iLen = PROTOHTTP_RECVDONE;
        }
        else if (iLen == PROTOHTTP_RECVBUFF)
        {
            // grow the buffer
            _HttpMgrReallocBuff(pRef, pState);
            // swallow error code
            iLen = 0;
        }
    }
    return(iLen);
}

/*F********************************************************************************/
/*!
    \Function _HttpMgrDownloadProcess

    \Description
        Process a download transaction.

    \Input *pRef    - httpmanager state
    \Input *pState  - transaction state

    \Version 10/28/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpMgrDownloadProcess(HttpMgrRefT *pRef, HttpStateT *pState)
{
    int32_t iLen;

    // if we're not doing the write callback thing, poll for data
    for (iLen = 1; (iLen != PROTOHTTP_RECVWAIT) && (iLen != 0) && (pState->state != IDLE); )
        {
            // receive data
        iLen = _HttpMgrRecvData(pRef, pState);
            // process data
        _HttpMgrDownloadProcessData(pRef, pState, iLen);
    }
}

/*F********************************************************************************/
/*!
    \Function _HttpMgrWriteCb

    \Description
        Implementation of ProtoHttp write callback

    \Input *pState      - http module state
    \Input *pWriteInfo  - callback info
    \Input *pData       - transaction data pointer
    \Input iDataSize    - size of data
    \Input *pUserData   - user callback data

    \Output
        int32_t         - zero

    \Version 05/03/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpMgrWriteCb(ProtoHttpRefT *pProtoHttp, const ProtoHttpWriteCbInfoT *pWriteInfo, const char *pData, int32_t iDataSize, void *pUserData)
{
    HttpStateT *pState = (HttpStateT *)pUserData;
#if 0
    static const char *_strRequestNames[] =
    {
        "PROTOHTTP_REQUESTTYPE_HEAD", "PROTOHTTP_REQUESTTYPE_GET", "PROTOHTTP_REQUESTTYPE_POST",
        "PROTOHTTP_REQUESTTYPE_PUT", "PROTOHTTP_REQUESTTYPE_DELETE", "PROTOHTTP_REQUESTTYPE_OPTIONS"
    };
    ZPrintf("httpmgr: writecb (%s,%d) recv=%d\n", _strRequestNames[pWriteInfo->eRequestType], pWriteInfo->eRequestResponse, iDataSize);
#endif

    // detect minbuff error
    if (iDataSize == PROTOHTTP_RECVBUFF)
    {
        // grow the buffer and return
        _HttpMgrReallocBuff(&_HttpMgr_Ref, pState);
        return(0);
    }

    // update count and write to output file if available
    pState->count += iDataSize;
    if (pState->iOutFile != ZFILE_INVALID)
    {
        ZFileWrite(pState->iOutFile, (void *)pData, iDataSize);
    }

    // update/completion procesing
    _HttpMgrDownloadProcessData(&_HttpMgr_Ref, pState, iDataSize);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _HttpMgrUploadProcess

    \Description
        Process an upload transaction.

    \Input *pRef    - module state

    \Output
        None.

    \Version 10/28/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpMgrUploadProcess(HttpMgrRefT *pRef, HttpStateT *pState)
{
    char strResponse[1024];
    int32_t iCode, iResult;

    // read data?
    if (pState->iInpFile != ZFILE_INVALID)
    {
        while (pState->iSentSize < pState->iDataSize)
        {
            // do we need more data?
            if (pState->iSendBufSent == pState->iSendBufData)
            {
                if ((pState->iSendBufData = ZFileRead(pState->iInpFile, pState->strFileBuffer, sizeof(pState->strFileBuffer))) > 0)
                {
                    ZPrintf("%s: read %d bytes from file\n", pRef->strModuleName, pState->iSendBufData);
                    pState->iSendBufSent = 0;
                }
                else
                {
                    ZPrintf("%s: error %d reading from file\n", pRef->strModuleName, pState->iSendBufData);
                    pState->state = IDLE;
                }
            }

            // do we have buffered data to send?
            if (pState->iSendBufSent < pState->iSendBufData)
            {
                iResult = HttpManagerSend(pRef->pHttpManager, pState->iHandle, pState->strFileBuffer + pState->iSendBufSent, pState->iSendBufData - pState->iSendBufSent);
                if (iResult > 0)
                {
                    pState->iSentSize += iResult;
                    pState->iSendBufSent += iResult;
                    ZPrintf("%s: sent %d bytes\n", pRef->strModuleName, iResult);
                }
                else if (iResult < 0)
                {
                    ZPrintf("%s: HttpManagerSend() failed; error %d\n", pRef->strModuleName, iResult);
                    pState->state = IDLE;
                }
                else
                {
                    break;
                }
            }
        }
        // check for upload completion
        if (pState->iSentSize == pState->iDataSize)
        {
            // if streaming upload, signal we are done
            if (pState->bStreaming == TRUE)
            {
                HttpManagerSend(pRef->pHttpManager, pState->iHandle, NULL, PROTOHTTP_STREAM_END);
                pState->bStreaming = FALSE;
            }

            // done uploading
            ZPrintf("%s: uploaded %qd bytes\n", pRef->strModuleName, pState->iSentSize);

            // close the file
            ZFileClose(pState->iInpFile);
            pState->iInpFile = ZFILE_INVALID;
        }
    }

    // give it time
    HttpManagerUpdate(pRef->pHttpManager);

    // see if we've received an HTTP 1xx (INFORMATIONAL) header
    iCode = HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'info', strResponse, sizeof(strResponse));
    if (PROTOHTTP_GetResponseClass(iCode) == PROTOHTTP_RESPONSE_INFORMATIONAL)
    {
        // got response header, so print it
        NetPrintf(("httpmgr: received %d response header\n----------------------------------\n%s----------------------------------\n", iCode, strResponse));
    }

    // done?
    if ((iResult = _HttpMgrCheckComplete(pRef->pHttpManager, pState, pRef->strModuleName)) != 0)
    {
        if (iResult > 0)
        {
            int32_t ultime = NetTickDiff(NetTick(), pState->sttime);
            ZPrintf("%s: upload complete (%qd bytes)\n", pRef->strModuleName, pState->iSentSize);
            ZPrintf("upload time: %qd bytes in %.2f seconds (%.3f k/sec)\n", pState->iSentSize, (float)ultime/1000.0f,
                ((float)pState->iSentSize * 1000.0f) / ((float)ultime * 1024.0f));

            ds_memclr(strResponse, sizeof(strResponse));
            iResult = HttpManagerRecv(pRef->pHttpManager, pState->iHandle, strResponse, 1, sizeof(strResponse));
            if (iResult > 0)
            {
                ZPrintf("http result:\n%s", strResponse);
            }
        }

        _HttpMgrFreeState(pRef, pState);
    }
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpMgrIdleCB

    \Description
        Callback to process while idle

    \Input *argz    -
    \Input argc     -
    \Input *argv[]  -

    \Output int32_t -

    \Version 09/26/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdHttpMgrIdleCB(ZContext *argz, int32_t argc, char *argv[])
{
    HttpMgrRefT *pRef = &_HttpMgr_Ref;
    int32_t iState;

    // shut down?
    if ((argc == 0) || (pRef->pHttpManager == NULL))
    {
        if (pRef->pHttpManager != NULL)
        {
            HttpManagerDestroy(pRef->pHttpManager);
            pRef->pHttpManager = NULL;
        }
        return(0);
    }

    // update httpmanager
    if (pRef->pHttpManager != NULL)
    {
        HttpManagerUpdate(pRef->pHttpManager);
    }

    // look for active transactions to process
    for (iState = 0; iState < HTTP_MAXCMDS; iState += 1)
    {
        // process a download (if not using write callback)
        if ((pRef->States[iState].state == DNLOAD) && (!pRef->bUseWriteCb))
        {
            _HttpMgrDownloadProcess(pRef, &pRef->States[iState]);
        }

        // process an upload
        if (pRef->States[iState].state == UPLOAD)
        {
            _HttpMgrUploadProcess(pRef, &pRef->States[iState]);
        }
    }

    // process mget request
    if (pRef->pMgetBuffer != NULL)
    {
        _HttpMgrMget(pRef);
    }

    // keep on idling
    return(ZCallback(&_CmdHttpMgrIdleCB, HTTP_RATE));
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpMgrUsage

    \Description
        Display usage information.

    \Input argc         - argument count
    \Input *argv[]      - argument list

    \Output
        None.

    \Version 02/18/2008 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdHttpMgrUsage(int argc, char *argv[])
{
    if (argc == 2)
    {
        ZPrintf("   execute http get or put operations\n");
        ZPrintf("   usage: %s [cclr|cert|cer2|cver|create|ctrl|destroy|free|get|mget|put|puts|parse|stat]", argv[0]);
    }
    else if (argc == 3)
    {
        if (!strcmp(argv[2], "cclr") || !strcmp(argv[2], "cert") || !strcmp(argv[2], "cer2") || !strcmp(argv[2], "cver"))
        {
            ZPrintf("   usage: %s cert|cer2 <certfile> - load certificate file\n", argv[0]);
            ZPrintf("          %s cclr - clear dynamic certificates\n");
            ZPrintf("          %s cver - verify dynamic CA certs\n");
        }
        else if (!strcmp(argv[2], "create"))
        {
            ZPrintf("   usage: %s create <bufsize>\n", argv[0]);
        }
        else if (!strcmp(argv[2], "destroy"))
        {
            ZPrintf("   usage: %s destroy\n", argv[0]);
        }
        else if (!strcmp(argv[2], "free"))
        {
            ZPrintf("   usage: %s get <iHandle>\n", argv[0]);
        }
        else if (!strcmp(argv[2], "get"))
        {
            ZPrintf("   usage: %s get [url] <file>\n", argv[0]);
        }
        else if (!strcmp(argv[2], "mget"))
        {
            ZPrintf("   usage: %s mget <file>\n", argv[0]);
        }
        else if (!strcmp(argv[2], "put"))
        {
            ZPrintf("   usage: %s put [url] [file]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "puts"))
        {
            ZPrintf("   usage: %s puts [url] [file]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "parse"))
        {
            ZPrintf("   usage: %s parse [url]\n", argv[0]);
        }
    }
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdHttpMgr

    \Description
        Initiate an HTTP transaction.

    \Input *argz    -
    \Input argc     -
    \Input *argv[]  -

    \Output int32_t -

    \Version 10/28/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdHttpMgr(ZContext *argz, int32_t argc, char *argv[])
{
    int32_t iResult = 0, iBufSize = HTTP_BUFSIZE, iNumRefs = 4;
    const char *pFileName = NULL, *pUrl;
    HttpMgrRefT *pRef = &_HttpMgr_Ref;
    HttpStateT *pState = NULL;
    const char *pFileData;
    int32_t iArg, iStartArg = 2; // first arg past get/put/delete/whatever
    int32_t iFileSize;

    if (argc < 2)
    {
        return(0);
    }

    // check for help
    if ((argc >= 2) && !strcmp(argv[1], "help"))
    {
        _CmdHttpMgrUsage(argc, argv);
        return(iResult);
    }

    // check for 'parse' command
    if ((argc == 3) && !strcmp(argv[1], "parse"))
    {
        char strKind[5], strHost[128];
        const char *pUri;
        int32_t iPort, iSecure;
        ds_memclr(strKind, sizeof(strKind));
        ds_memclr(strHost, sizeof(strHost));
        pUri = ProtoHttpUrlParse(argv[2], strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure);
        ZPrintf("parsed url: kind=%s, host=%s, port=%d, secure=%d, uri=%s\n", strKind, strHost, iPort, iSecure, pUri);
        return(0);
    }

    // if not initialized yet, do so now
    if (_HttpMgr_bInitialized == FALSE)
    {
        ds_memclr(pRef, sizeof(*pRef));
        _HttpMgr_bInitialized = TRUE;
    }

    // check for explicit destroy
    if ((argc == 2) && !ds_stricmp(argv[1], "destroy"))
    {
        if (pRef->pHttpManager != NULL)
        {
            HttpManagerDestroy(pRef->pHttpManager);
            pRef->pHttpManager = NULL;
        }
        return(iResult);
    }

    // check for request to set a certificate
    if ((argc == 3) && ((!strcmp(argv[1], "cert")) || (!strcmp(argv[1], "cer2"))))
    {
        const uint8_t *pCertData;
        int32_t iCertSize;

        // try and open file
        if ((pCertData = (const uint8_t *)ZFileLoad(argv[2], &iCertSize, ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_BINARY)) != NULL)
        {
            if (!strcmp(argv[1], "cert"))
            {
                iResult = ProtoHttpSetCACert(pCertData, iCertSize);
            }
            else
            {
                iResult = ProtoHttpSetCACert2(pCertData, iCertSize);
            }
            ZMemFree((void *)pCertData);
        }
        else
        {
            ZPrintf("%s: unable to load certificate file '%s'\n", argv[0], argv[2]);
        }
        return((iResult > 0) ? 0 : -1);
    }
    else if (!strcmp(argv[1], "cclr"))
    {
        ZPrintf("%s: clearing dynamic certs\n", argv[0]);
        ProtoHttpClrCACerts();
        return(0);
    }
    else if (!strcmp(argv[1], "cver"))
    {
        int32_t iInvalid;
        ZPrintf("%s: verifying dynamic CA certs\n", argv[0]);
        if ((iInvalid = ProtoHttpValidateAllCA()) > 0)
        {
            ZPrintf("%s: could not verify %d CA certs\n", iInvalid);
            iResult = -1;
        }
        return(iResult);
    }

    // check for create request
    if ((argc >= 2) && !strcmp(argv[1], "create"))
    {
        if (argc >= 3)
        {
            iBufSize = (int32_t)strtol(argv[2], NULL, 10);
        }
        if (argc >= 4)
        {
            iNumRefs = (int32_t)strtol(argv[3], NULL, 10);
        }
    }

    // create httpmanager module if necessary
    if (pRef->pHttpManager == NULL)
    {
        ZPrintf("%s: creating module with a %d refs and %dkbyte buffer\n", argv[0], iNumRefs, iBufSize);
        ds_memclr(pRef, sizeof(*pRef));
        ds_strnzcpy(pRef->strModuleName, argv[0], sizeof(pRef->strModuleName));
        pRef->pHttpManager = HttpManagerCreate(iBufSize, iNumRefs);
        if (pRef->pHttpManager != NULL)
        {
            HttpManagerCallback(pRef->pHttpManager, _HttpCustomHeaderCallback, _HttpRecvHeaderCallback);
            pRef->iDebugLevel = 1;
            HttpManagerControl(pRef->pHttpManager, -1, 'spam', pRef->iDebugLevel, 0, NULL);
        }
    }

    // check for create request -- if so, we're done
    if ((argc >= 2) && !strcmp(argv[1], "create"))
    {
        return(iResult);
    }
    else if ((argc > 2) && (argc < 6) && !strcmp(argv[1], "ctrl"))
    {
        int32_t iCmd, iValue = 0, iValue2 = 0;

        iCmd  = argv[2][0] << 24;
        iCmd |= argv[2][1] << 16;
        iCmd |= argv[2][2] << 8;
        iCmd |= argv[2][3];

        if (argc > 3)
        {
            iValue = (int32_t)strtol(argv[3], NULL, 10);
        }

        if (argc > 4)
        {
            iValue2 = (int32_t)strtol(argv[4], NULL, 10);
        }

        // snoop 'spam'
        if (iCmd == 'spam')
        {
            pRef->iDebugLevel = iValue;
        }

        return(HttpManagerControl(pRef->pHttpManager, /*iHandle*/ -1, iCmd, iValue, iValue2, NULL));
    }
    else if ((argc == 3) && !strcmp(argv[1], "stat"))
    {
        int32_t iCmd;
        char strBufferTemp[1024] = "";

        iCmd  = argv[2][0] << 24;
        iCmd |= argv[2][1] << 16;
        iCmd |= argv[2][2] << 8;
        iCmd |= argv[2][3];

        iResult = HttpManagerStatus(pRef->pHttpManager, /*iHandle*/ -1, iCmd, strBufferTemp, sizeof(strBufferTemp));
        ZPrintf("%s: ProtoHttpStatus('%s') returned %d\n", argv[0], argv[2], iResult);
        if (strBufferTemp[0] != '\0')
        {
            ZPrintf("%s\n", strBufferTemp);
        }
        return(0);
    }
    // check for setting of base url
    else if ((argc == 3) && !strcmp(argv[1], "base"))
    {
        HttpManagerSetBaseUrl(pRef->pHttpManager, /*iHandle*/ -1, argv[2]);
        return(iResult);
    }
    // check for valid get/put request
    else if ((!ds_stricmp(argv[1], "get") || !ds_stricmp(argv[1], "head") || !ds_stricmp(argv[1], "put") || !ds_stricmp(argv[1], "post") ||
              !ds_stricmp(argv[1], "puts") || !ds_stricmp(argv[1], "delete") || !ds_stricmp(argv[1], "options")) &&
            ((argc > 2) || (argc < 5)))
    {
        // allocate a new state record
        pState = _HttpMgrAllocState(pRef);
        if (pState == NULL)
        {
            // if we could not allocate state, return error so upper layer can deal with it
            return(-1);
        }

        // fall-through to code below
    }
    else if (!ds_stricmp(argv[1], "mget") || !ds_stricmp(argv[1], "free"))
    {
        // do nothing, fall through
    }
    else
    {
        ZPrintf("   unrecognized or badly formatted command '%s'\n", argv[1]);
        _CmdHttpMgrUsage(argc, argv);
        return(-1);
    }

    // clear previous options
    pRef->bUseWriteCb = FALSE;
    pRef->bRecvAll = FALSE;

    // set up append header
    ds_strnzcpy(pRef->strApndHdr, HTTP_APNDHDR, sizeof(pRef->strApndHdr));

    // check for args
    for (iArg = 2; (iArg < argc) && (argv[iArg][0] == '-'); iArg += 1)
    {
        if (!ds_strnicmp(argv[iArg], "-header=", 8))
        {
            ds_strnzcat(pRef->strApndHdr, argv[iArg]+8, sizeof(pRef->strApndHdr));
            ds_strnzcat(pRef->strApndHdr, "\r\n", sizeof(pRef->strApndHdr));
        }
        if (!ds_strnicmp(argv[iArg], "-writecb", 8))
        {
            pRef->bUseWriteCb = TRUE;
        }
        if (!ds_strnicmp(argv[iArg], "-recvall", 8))
        {
            pRef->bRecvAll = TRUE;
        }
        // skip any option arguments to find url and (optionally) filename
        iStartArg += 1;
    }

    // locate url and filename
    pUrl = argv[iStartArg];
    if (argc > (iStartArg+1))
    {
        pFileName = argv[iStartArg+1];
    }

    if (!pUrl)
    {
        ZPrintf("%s: no url specified\n", argv[0]);
        return(-1);
    }

    // set append header
    if ((pState != NULL) || (!ds_stricmp(argv[1], "mget")))
    {
        char strBuffer[1024] = "\0";
        int32_t iHandle = (pState != NULL) ? pState->iHandle : -1;
        //if (pRef->strCookie[0] != '\0')
        //{
        //    ds_snzprintf(strBuffer, sizeof(strBuffer), "Cookie: %s\r\n", pRef->strCookie);
        //}
        ds_strnzcat(strBuffer, pRef->strApndHdr, sizeof(strBuffer));
        HttpManagerControl(pRef->pHttpManager, iHandle, 'apnd', 0, 0, strBuffer);
    }

    // see if we're uploading or downloading
    if (!ds_stricmp(argv[1], "put") || !ds_stricmp(argv[1], "post") || !ds_stricmp(argv[1], "puts"))
    {
        ZPrintf("%s: uploading %s to %s\n", argv[0], pFileName, pUrl);

        // assume failure
        iResult = -1;

        // try and open file
        if ((pState->iInpFile = ZFileOpen(pFileName, ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_BINARY)) != ZFILE_INVALID)
        {
            // get the file size
            if ((pState->iDataSize = ZFileSize(pState->iInpFile)) > 0)
            {
                // load data from file
                if ((pState->iSendBufData = ZFileRead(pState->iInpFile, pState->strFileBuffer, sizeof(pState->strFileBuffer))) > 0)
                {
                    if (ds_stricmp(argv[1], "puts"))
                    {
                        // initiate put/post transaction
                        ZPrintf("%s: uploading %qd bytes\n", argv[0], pState->iDataSize);
                        if ((pState->iSendBufSent = HttpManagerPost(pRef->pHttpManager, pState->iHandle, pUrl, pState->strFileBuffer, pState->iDataSize, !ds_stricmp(argv[1], "put") ? PROTOHTTP_PUT : PROTOHTTP_POST)) < 0)
                        {
                            ZPrintf("%s: error %d initiating send\n", pState->iSendBufSent);
                            iResult = -1;
                        }
                        else if (pState->iSendBufSent > 0)
                        {
                            ZPrintf("%s: sent %d bytes\n", argv[0], pState->iSendBufSent);
                            pState->iSentSize = pState->iSendBufSent;
                        }
                    }
                    else
                    {
                        // initiate streaming put
                        if ((iResult = HttpManagerPost(pRef->pHttpManager, pState->iHandle, pUrl, NULL, PROTOHTTP_STREAM_BEGIN, PROTOHTTP_PUT)) == 0)
                        {
                            pState->bStreaming = TRUE;
                        }
                    }

                    // wait for reply
                    pState->state = UPLOAD;
                    iResult = 0;
                }
                else
                {
                    ZPrintf("%s: error %d reading data from file\n", argv[0], pState->iSendBufData, pFileName);
                }
            }
            else
            {
                ZPrintf("%s: error %d getting size of file %s\n", argv[0], pState->iDataSize, pFileName);
            }
        }
        else
        {
            ZPrintf("%s: unable to load file '%s'\n", argv[0], pFileName);
        }
    }
    else if (!ds_stricmp(argv[1], "head") || !ds_stricmp(argv[1], "get"))
    {
        ProtoHttpRequestTypeE eRequestType = !ds_stricmp(argv[1], "head") ? PROTOHTTP_REQUESTTYPE_HEAD : PROTOHTTP_REQUESTTYPE_GET;

        if (pFileName != NULL)
        {
            if (pRef->iDebugLevel > 1)
            {
                pState->iOutFile = ZFileOpen(pFileName, ZFILE_OPENFLAG_WRONLY|ZFILE_OPENFLAG_BINARY|ZFILE_OPENFLAG_CREATE);
            }
        }
        else if (pRef->iDebugLevel > 1)
        {
            ZPrintf("%s: downloading %s\n", argv[0], pUrl);
        }

        // initiate transaction
        if (pRef->bUseWriteCb)
        {
            iResult = HttpManagerRequestCb(pRef->pHttpManager, pState->iHandle, pUrl, NULL, 0, eRequestType, _HttpMgrWriteCb, pState);
        }
        else
        {
            iResult = HttpManagerRequest(pRef->pHttpManager, pState->iHandle, pUrl, NULL, 0, eRequestType);
        }
        if (iResult == 0)
        {
            pState->state = DNLOAD;
        }
    }
    else if (!ds_stricmp(argv[1], "mget") && (pRef->pMgetBuffer == NULL))
    {
        if ((pFileData = ZFileLoad(argv[iStartArg], &iFileSize, ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_BINARY)) != NULL)
        {
            // set up for mget process
            pRef->pMgetBuffer = pRef->pMgetOffset = pFileData;
            pRef->uMgetStart = NetTick();
            ds_strnzcpy(pRef->strMgetFilename, argv[iStartArg], sizeof(pRef->strMgetFilename));
        }
    }
    else if (!ds_stricmp(argv[1], "delete"))
    {
        if ((iResult = HttpManagerRequest(pRef->pHttpManager, pState->iHandle, pUrl, NULL, 0, PROTOHTTP_REQUESTTYPE_DELETE)) == 0)
        {
            pState->state = DNLOAD;
        }
    }
    else if (!ds_stricmp(argv[1], "options"))
    {
        if ((iResult = HttpManagerRequest(pRef->pHttpManager, pState->iHandle, pUrl, NULL, 0, PROTOHTTP_REQUESTTYPE_DELETE)) == 0)
        {
            pState->state = DNLOAD;
        }
    }
    else if (!ds_stricmp(argv[1], "free"))
    {
        int32_t iHandle = atoi(argv[2]);
        ZPrintf("HttpManagerFree %d\n", iHandle);
        HttpManagerFree(pRef->pHttpManager, iHandle);
    }
    else
    {
        ZPrintf("%s: unrecognized request %s\n", argv[0], argv[1]);
        iResult = -1;
    }

    // set up recurring callback to process transaction, if any
    if (((pState != NULL) && (pState->state != IDLE) && (pRef->pMgetBuffer == NULL)) || ((pState == NULL) && (pRef->pMgetBuffer != NULL)))
    {
        iResult = ZCallback(_CmdHttpMgrIdleCB, HTTP_RATE);
    }
    return(iResult);
}

