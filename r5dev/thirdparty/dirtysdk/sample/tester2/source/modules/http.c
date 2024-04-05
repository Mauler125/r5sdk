/*H********************************************************************************/
/*!
    \File http.c

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
#include <ctype.h>
#include <time.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/proto/protossl.h"

#include "libsample/zlib.h"
#include "libsample/zmem.h"
#include "libsample/zfile.h"

#include "testermodules.h"

/*** Defines **********************************************************************/

#define HTTP_BUFSIZE    (4096)
#define HTTP_RATE       (1)

#define HTTP_XTRAHDR0   ""
#define HTTP_XTRAHDR1   "X-Agent: DirtySock HTTP Tester\r\n"    // test "normal" replacement (replaces Accept: header)
#define HTTP_XTRAHDR2   "User-Agent: DirtySock HTTP Tester\r\n" // test "extended" replacement (replaces User-Agent: and Accept: headers)

//$$ tmp -- special test header used for hard-coded multipart/form-data testing -- this should be removed at some point when we have real multipart/form-data support
#define HTTP_XTRAHDR3   "Content-Type: multipart/form-data; boundary=TeStInG\r\n" \
                        "User-Agent: DirtySock HTTP Tester\r\n" \
                        "Accept: */*\r\n"

#define HTTP_APNDHDR    HTTP_XTRAHDR0

/*** Function Prototypes **********************************************************/

static int32_t _CmdHttpIdleCB(ZContext *argz, int32_t argc, char *argv[]);

/*** Type Definitions *************************************************************/

typedef struct HttpRefT      // module state storage
{
    ProtoHttpRefT *http;
    int64_t show;
    int64_t count;
    int32_t sttime;
    int64_t iDataSize;
    int64_t iSentSize;
    int32_t iSendBufData;
    int32_t iSendBufSent;
    ZFileT iInpFile;
    ZFileT iOutFile;
    int32_t iOutSize;
    char *pOutData;
    char *pClientCert;
    int32_t iCertSize;
    char *pClientKey;
    int32_t iKeySize;
    int32_t iDebugLevel;
    uint8_t bStreaming;
    uint8_t bUseWriteCb;
    uint8_t bRecvAll;
    char strModuleName[32];
    char strHost[128];
    char strCookie[2048];
    char strApndHdr[2048];
    char strFileBuffer[64*1024]; // must be at a minimum 4k or protohttp buffer size, whichever is larger

    // module state
    enum
    {
        IDLE, DNLOAD, UPLOAD
    } state;
} HttpRefT;

/*** Variables ********************************************************************/

static HttpRefT _Http_Ref;
static uint8_t  _Http_bInitialized = FALSE;

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _GetIntArg

    \Description
        Get fourcc/integer from command-line argument

    \Input *pArg        - pointer to argument

    \Version 10/20/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _GetIntArg(const char *pArg)
{
    int32_t iValue;

    // check for possible fourcc value
    if ((strlen(pArg) == 4) && (isalpha(pArg[0]) || isalpha(pArg[1]) || isalpha(pArg[2]) || isalpha(pArg[3])))
    {
        iValue  = pArg[0] << 24;
        iValue |= pArg[1] << 16;
        iValue |= pArg[2] << 8;
        iValue |= pArg[3];
    }
    else
    {
        iValue = (signed)strtol(pArg, NULL, 10);
    }
    return(iValue);
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpUsage

    \Description
        Display usage information.

    \Input argc         - argument count
    \Input *argv[]      - argument list

    \Version 02/18/2008 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdHttpUsage(int argc, char *argv[])
{
    if (argc == 2)
    {
        ZPrintf("   execute http get or put operations\n");
        ZPrintf("   usage: %s [cclr|cert|cer2|cver|create|ctrl|debug|destroy|get|head|put|post|puts|delete|options|parse|stat|urlparse]", argv[0]);
    }
    else if (argc == 3)
    {
        if (!strcmp(argv[2], "cclr") || !strcmp(argv[2], "cert") || !strcmp(argv[2], "cer2") || !strcmp(argv[2], "cver"))
        {
            ZPrintf("   usage: %s cert|cer2 <certfile> - load certificate file\n", argv[0]);
            ZPrintf("          %s cclr - clear dynamic CA certs\n");
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
        else if (!strcmp(argv[2], "get") || !strcmp(argv[2], "head") || !strcmp(argv[2], "options") || !strcmp(argv[2], "delete"))
        {
            ZPrintf("   usage: %s %s <options> [url] <outfile>\n", argv[0], argv[2]);
        }
        else if (!strcmp(argv[2], "put") || !strcmp(argv[2], "puts") || !strcmp(argv[2], "post"))
        {
            ZPrintf("   usage: %s %s <options> [url] [infile] <outfile>\n", argv[0], argv[2]);
        }
        else if (!strcmp(argv[2], "debug"))
        {
            ZPrintf("   usage: %s debug [debuglevel]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "parse"))
        {
            ZPrintf("   usage: %s parse [url]\n", argv[0]);
        }
    }
}

static int32_t _HttpUrlParseTest(const char *pModuleName, const char *pUrl)
{
    char strKind[16], strHost[256];
    int32_t iPort, iSecure;
    uint8_t bPortSpecified;
    const char *pUri;

    ZPrintf("%s: parsing url %s\n", pModuleName, pUrl);
    pUri = ProtoHttpUrlParse2(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);

    ZPrintf("%s: uri=%s\n", pModuleName, pUri);
    ZPrintf("%s: kind=%s\n", pModuleName, strKind);
    ZPrintf("%s: host=%s\n", pModuleName, strHost);
    ZPrintf("%s: iPort=%d\n", pModuleName, iPort);
    ZPrintf("%s: iSecure=%d\n", pModuleName, iSecure);
    ZPrintf("%s: bPortSpecified=%s\n", pModuleName, bPortSpecified ? "TRUE" : "FALSE");

    return(0);
}

static int32_t _HttpUrlEncodeTest(const char *pModuleName, char **pArgs, int32_t iNumArgs)
{
    char strUrl[128];
    int32_t iArg, iOff;

    ds_snzprintf(strUrl, sizeof(strUrl), "https://urltest.ea.com/");
    for (iArg = 0, iOff = (int32_t)strlen(strUrl); iArg < iNumArgs; iArg += 2)
    {
        iOff += ProtoHttpUrlEncodeStrParm(strUrl+iOff, sizeof(strUrl)-iOff, pArgs[iArg], pArgs[iArg+1]);
    }
    ProtoHttpUrlEncodeIntParm(strUrl, sizeof(strUrl), "&test=", 2918123);
    ZPrintf("%s: url=%s\n", pModuleName, strUrl);

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _HttpLoadCertificate

    \Description
        Display usage information.

    \Input *pFilename   - certificate filename
    \Input *pCertSize   - [out] storage for cert size

    \Output
        char *          - pointer to certificate, or NULL on error

    \Version 10/20/2013 (jbrookes)
*/
/********************************************************************************F*/
static char *_HttpLoadCertificate(const char *pFilename, int32_t *pCertSize)
{
    char *pCertBuf;
    int32_t iFileSize;

    // load certificate file
    if ((pCertBuf = (char *)ZFileLoad(pFilename, &iFileSize, ZFILE_OPENFLAG_RDONLY)) == NULL)
    {
        return(NULL);
    }

    // calculate length
    *pCertSize = iFileSize;
    // return to caller
    return(pCertBuf);
}

/*F********************************************************************************/
/*!
    \Function _HttpReset

    \Description
        Reset transaction state

    \Input *pRef        - module state

    \Version 03/21/2014 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpReset(HttpRefT *pRef)
{
    // clear any previous stats
    pRef->count = 0;
    pRef->show = 0;
    pRef->iDataSize = 0;
    pRef->iSentSize = 0;
    pRef->iSendBufSent = 0;

    // clear previous options
    pRef->bStreaming = FALSE;
    pRef->bUseWriteCb = FALSE;
    pRef->bRecvAll = FALSE;
}

/*F********************************************************************************/
/*!
    \Function _HttpCustomHeaderCallback

    \Description
        ProtoHttp custom header callback.

    \Input *pProtoHttp  - protohttp module state
    \Input *pHeader     - header to be sent
    \Input uHeaderSize  - header size
    \Input *pUserData   - user ref (HttpRefT)

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
    int32_t iOffset;

    NetPrintf(("http: custom header callback, size=%d, data=%d, pData=%p\n", uHeaderSize, iDataLen, pData));

    // find end of header
    iOffset = (int32_t)strlen(pHeader);

    // append a header
    iOffset += ds_snzprintf(pHeader+iOffset, uHeaderSize-iOffset, "X-CustomHeaderCallback: Testing custom header callback\r\n");

    // we can either return the header length or zero; in the latter case protohttp will re-calculate for us
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _HttpRecvHeaderCallback

    \Description
        ProtoHttp recv header callback.

    \Input *pProtoHttp  - protohttp module state
    \Input *pHeader     - received header
    \Input uHeaderSize  - header size
    \Input *pUserData   - user ref (HttpRefT)

    \Output
        int32_t         - zero

    \Version 02/24/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpRecvHeaderCallback(ProtoHttpRefT *pProtoHttp, const char *pHeader, uint32_t uHeaderSize, void *pUserRef)
{
    HttpRefT *pHttp = (HttpRefT *)pUserRef;
    char strBuffer[2048], strName[128];
    const char *pHdrTmp;
    int32_t iLocnSize;

    // check for location header
    if ((iLocnSize = ProtoHttpGetHeaderValue(pProtoHttp, pHeader, "location", NULL, 0, NULL)) > 0)
    {
        ZPrintf("http: location header size=%d\n", iLocnSize);
        if (ProtoHttpGetHeaderValue(pProtoHttp, pHeader, "location", strBuffer, sizeof(strBuffer), NULL) == 0)
        {
            ZPrintf("http: location url='%s'\n", strBuffer);
        }
        else
        {
            ZPrintf("http: error querying location url\n");
        }
    }

    // test ProtoHttpGetNextHeader()
    for (pHdrTmp = pHeader; ProtoHttpGetNextHeader(pProtoHttp, pHdrTmp, strName, sizeof(strName), strBuffer, sizeof(strBuffer), &pHdrTmp) == 0; )
    {
        #if 0
        ZPrintf("http: ===%s: %s\n", strName, strBuffer);
        #endif
    }

    // parse any set-cookie requests
    for (pHdrTmp = pHeader; ProtoHttpGetHeaderValue(pProtoHttp, pHdrTmp, "set-cookie", strBuffer, sizeof(strBuffer), &pHdrTmp) == 0; )
    {
        // print the cookie
        ZPrintf("http: parsed cookie '%s'\n", strBuffer);

        // add field seperator
        if (pHttp->strCookie[0] != '\0')
        {
            ds_strnzcat(pHttp->strCookie, ", ", sizeof(pHttp->strCookie));
        }
        // append to cookie list
        ds_strnzcat(pHttp->strCookie, strBuffer, sizeof(pHttp->strCookie));
    }

    // if this is a redirection update append header with any new cookies
    if ((PROTOHTTP_GetResponseClass(ProtoHttpStatus(pProtoHttp, 'code', NULL, 0)) == PROTOHTTP_RESPONSE_REDIRECTION) && (pHttp->strCookie[0] != '\0'))
    {
        // format append header
        ds_snzprintf(strBuffer, sizeof(strBuffer), "Cookie: %s\r\n%s", pHttp->strCookie, HTTP_APNDHDR);
        ds_strnzcat(strBuffer, pHttp->strApndHdr, sizeof(strBuffer));
        ProtoHttpControl(pProtoHttp, 'apnd', 0, 0, strBuffer);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _HttpReallocBuff

    \Description
        Realloc buffer used for RecvAll operation

    \Input *pRef        - pointer to http ref

    \Version 07/29/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpReallocBuff(HttpRefT *pRef)
{
    char *pNewData;
    int32_t iNewSize;

    // calc new buffer size
    if ((iNewSize = pRef->iOutSize) == 0)
    {
        // try getting body size
        if ((iNewSize = ProtoHttpStatus(pRef->http, 'body', NULL, 0)) > 0)
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
    if (pRef->pOutData != NULL)
    {
        ds_memcpy(pNewData, pRef->pOutData, pRef->iOutSize);
        ZMemFree(pRef->pOutData);
    }
    // save new pointer
    pRef->pOutData = pNewData;
    pRef->iOutSize = iNewSize;
}

/*F********************************************************************************/
/*!
    \Function _HttpDownloadProcessData

    \Description
        Process data received in a download operation

    \Input *pRef    - module state
    \Input iLen     - data length or PROTOHTTP_RECV*

    \Version 05/03/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpDownloadProcessData(HttpRefT *pRef, int32_t iLen)
{
    int32_t iResult;

    // see if we should show progress
    if ((pRef->count/1024) != (pRef->show/1024))
    {
        pRef->show = pRef->count;
        ZPrintf("%s: downloaded %qd bytes\n", pRef->strModuleName, pRef->count);
    }

    // see if we are done
    if ((iLen < 0) && (iLen != PROTOHTTP_RECVWAIT))
    {
        // completed successfully?
        if ((iLen == PROTOHTTP_RECVDONE) || (iLen == PROTOHTTP_RECVHEAD))
        {
            int32_t iDlTime = NetTickDiff(NetTick(), pRef->sttime);
            int64_t iBodySize;
            char strRespHdr[1024];

            ZPrintf("%s: download complete (%qd bytes)\n", pRef->strModuleName, pRef->count);
            ZPrintf("%s: download time: %qd bytes in %.2f seconds (%.3f k/sec)\n", pRef->strModuleName, pRef->count,
                (float)iDlTime/1000.0f, ((float)pRef->count * 1000.0f) / ((float)iDlTime * 1024.0f));

            // make sure we got it all
            if ((iBodySize = ProtoHttpStatus(pRef->http, 'body', NULL, 0)) != pRef->count)
            {
                ZPrintf("%s: WARNING -- mismatch between expected size (%d) and actual size (%d)\n", pRef->strModuleName, iBodySize, pRef->count);
            }

            // display response header
            if (ProtoHttpStatus(pRef->http, 'htxt', strRespHdr, sizeof(strRespHdr)) >= 0)
            {
                ZPrintf("%s response header:\n%s\n", pRef->strModuleName, strRespHdr);
            }

            // display a couple of parsed fields
            if (ProtoHttpStatus(pRef->http, 'head', NULL, 0) > 0)
            {
                time_t tLastMod = ProtoHttpStatus(pRef->http, 'date', NULL, 0);
                const char *pTime;

                if (tLastMod != 0)
                {
                    if ((pTime = ctime(&tLastMod)) != NULL)
                    {
                        ZPrintf("%s: Last-Modified: %s", pRef->strModuleName, pTime);
                    }
                }
                // get content-length; by passing in a 64bit int we support 64bit transfer sizes
                ProtoHttpStatus(pRef->http, 'body', &iBodySize, sizeof(iBodySize));
                ZPrintf("%s: Content-Length: %qd\n", pRef->strModuleName, iBodySize);
            }
        }
        else // failure
        {
            ProtoSSLAlertDescT AlertDesc;
            int32_t iSockErr = ProtoHttpStatus(pRef->http, 'serr', NULL, 0);
            int32_t iSslFail = ProtoHttpStatus(pRef->http, 'essl', NULL, 0);
            int32_t iAlert = ProtoHttpStatus(pRef->http, 'alrt', &AlertDesc, sizeof(AlertDesc));
            ZPrintf("%s: download failed (err=%d, sockerr=%d sslerr=%d)\n", pRef->strModuleName, iLen, iSockErr, iSslFail);
            if ((iSslFail == PROTOSSL_ERROR_CERT_INVALID) || (iSslFail == PROTOSSL_ERROR_CERT_HOST) ||
                (iSslFail == PROTOSSL_ERROR_CERT_NOTRUST) || (iSslFail == PROTOSSL_ERROR_CERT_REQUEST))
            {
                ProtoSSLCertInfoT CertInfo;
                if (ProtoHttpStatus(pRef->http, 'cert', &CertInfo, sizeof(CertInfo)) == 0)
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
            if (iAlert > 0)
            {
                ZPrintf("%s: %s ssl alert %s (%d)\n", pRef->strModuleName, (iAlert == 1) ? "recv" : "sent", AlertDesc.pAlertDesc, AlertDesc.iAlertType);
            }            
        }

        ZPrintf("%s: hResult=0x%08x\n", pRef->strModuleName, ProtoHttpStatus(pRef->http, 'hres', NULL, 0));

        // if file exists, close it
        if (pRef->iOutFile != ZFILE_INVALID)
        {
            if ((iResult = ZFileClose(pRef->iOutFile)) != 0)
            {
                ZPrintf("%s: error %d closing output file\n", pRef->strModuleName, iResult);
            }
            pRef->iOutFile = ZFILE_INVALID;
        }

        // if output buffer exists, free it
        if (pRef->pOutData != NULL)
        {
            pRef->pOutData = NULL;
        }
        pRef->iOutSize = 0;

        // set to idle state
        pRef->state = IDLE;
    }
}

/*F********************************************************************************/
/*!
    \Function _HttpWriteCb

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
static int32_t _HttpWriteCb(ProtoHttpRefT *pState, const ProtoHttpWriteCbInfoT *pWriteInfo, const char *pData, int32_t iDataSize, void *pUserData)
{
    HttpRefT *pRef = (HttpRefT *)pUserData;
    //NetPrintf(("http: writecb (%d,%d) %d bytes\n", pWriteInfo->eRequestType, pWriteInfo->eRequestResponse, iDataSize));

    // if we got data, update count and write to output file if available
    if (iDataSize > 0)
    {
        pRef->count += iDataSize;
        if (pRef->iOutFile != ZFILE_INVALID)
        {
            ZFileWrite(pRef->iOutFile, (void *)pData, iDataSize);
        }
    }

    // process
    _HttpDownloadProcessData(pRef, iDataSize);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _HttpRecvData

    \Description
        Receive data from ProtoHttp using either ProtoHttpRecv() or
        ProtoHttpRecvAll().

    \Input *pRef    - module state
    \Input *pState  - transaction state

    \Version 07/02/2013 (jbrookes) brought over from httpmgr
*/
/********************************************************************************F*/
static int32_t _HttpRecvData(HttpRefT *pRef)
{
    char strBuf[16*1024];
    int32_t iLen;

    // check for data
    if (!pRef->bRecvAll)
    {
        while ((iLen = ProtoHttpRecv(pRef->http, strBuf, 1, sizeof(strBuf))) > 0)
        {
            pRef->count += iLen;
            if (pRef->iOutFile != ZFILE_INVALID)
            {
                ZFileWrite(pRef->iOutFile, strBuf, iLen);
            }
        }
    }
    else
    {
        // receive all the data
        if ((iLen = ProtoHttpRecvAll(pRef->http, pRef->pOutData, pRef->iOutSize)) > 0)
        {
            pRef->count = iLen;
            if (pRef->iOutFile != ZFILE_INVALID)
            {
                ZFileWrite(pRef->iOutFile, pRef->pOutData, iLen);
            }
            iLen = PROTOHTTP_RECVDONE;
        }
        else if (iLen == PROTOHTTP_RECVBUFF)
        {
            // grow the buffer
            _HttpReallocBuff(pRef);
            // swallow error code
            iLen = 0;
        }
    }
    return(iLen);
}

/*F********************************************************************************/
/*!
    \Function _HttpDownloadProcess

    \Description
        Process a download transaction, using polling method

    \Input *pRef    - module state

    \Version 10/28/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpDownloadProcess(HttpRefT *pRef)
{
    int32_t iLen;

    // if we're not doing the write callback thing, poll for data
    for (iLen = 1; (iLen != PROTOHTTP_RECVWAIT) && (iLen != 0) && (pRef->state != IDLE); )
    {
        // receive data
        iLen = _HttpRecvData(pRef);
        // process data
        _HttpDownloadProcessData(pRef, iLen);
    }
}

/*F********************************************************************************/
/*!
    \Function _HttpUploadProcess

    \Description
        Process an upload transaction.

    \Input *pRef    - module state

    \Version 10/28/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpUploadProcess(HttpRefT *pRef)
{
    int32_t iResult;

    // if no input file, nothing to send
    if (pRef->iInpFile == ZFILE_INVALID)
    {
        return;
    }

    // send all the data
    while ((pRef->state == UPLOAD) && (pRef->iSentSize < pRef->iDataSize))
    {
        // do we need more data?
        if (pRef->iSendBufSent == pRef->iSendBufData)
        {
            if ((pRef->iSendBufData = ZFileRead(pRef->iInpFile, pRef->strFileBuffer, sizeof(pRef->strFileBuffer))) > 0)
            {
                ZPrintf("%s: read %d bytes from file\n", pRef->strModuleName, pRef->iSendBufData);
                pRef->iSendBufSent = 0;
            }
            else
            {
                ZPrintf("%s: error %d reading from file\n", pRef->strModuleName, pRef->iSendBufData);
                pRef->state = IDLE;
            }
        }

        // do we have buffered data to send?
        if (pRef->iSendBufSent < pRef->iSendBufData)
        {
            iResult = ProtoHttpSend(pRef->http, pRef->strFileBuffer + pRef->iSendBufSent, pRef->iSendBufData - pRef->iSendBufSent);
            if (iResult > 0)
            {
                pRef->iSentSize += iResult;
                pRef->iSendBufSent += iResult;
                ZPrintf("%s: sent %d bytes (%qd total)\n", pRef->strModuleName, iResult, pRef->iSentSize);
            }
            else if (iResult < 0)
            {
                ZPrintf("%s: ProtoHttpSend() failed; error %d\n", pRef->strModuleName, iResult);
                pRef->state = IDLE;
            }
            else
            {
                break;
            }
        }
    }
    // check for upload completion
    if (pRef->iSentSize == pRef->iDataSize)
    {
        int32_t ultime = NetTick() - pRef->sttime;

        // if streaming upload, signal we are done
        if (pRef->bStreaming == TRUE)
        {
            ProtoHttpSend(pRef->http, NULL, PROTOHTTP_STREAM_END);
            pRef->bStreaming = FALSE;
        }

        // done uploading
        ZPrintf("%s: upload complete (%qd bytes)\n", pRef->strModuleName, pRef->iSentSize);
        ZPrintf("%s: upload time: %d bytes in %.2f seconds (%.3f k/sec)\n", pRef->strModuleName, pRef->iSentSize,
            (float)ultime/1000.0f, ((float)pRef->iSentSize * 1000.0f) / ((float)ultime * 1024.0f));

        // close the file
        ZFileClose(pRef->iInpFile);
        pRef->iInpFile = ZFILE_INVALID;

        // transition to download state to receive server response
        pRef->state = DNLOAD;
    }
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpIdleCB

    \Description
        Callback to process while idle

    \Input *argz    -
    \Input argc     -
    \Input *argv[]  -

    \Output int32_t -

    \Version 09/26/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdHttpIdleCB(ZContext *argz, int32_t argc, char *argv[])
{
    HttpRefT *pRef = &_Http_Ref;

    // shut down?
    if ((argc == 0) || (pRef->http == NULL))
    {
        if (pRef->http != NULL)
        {
            ProtoHttpDestroy(pRef->http);
            pRef->http = NULL;
        }
        return(0);
    }

    // give ref processing time
    ProtoHttpUpdate(pRef->http);

    // download processing (if not using write callback)
    if ((pRef->state == DNLOAD) && (!pRef->bUseWriteCb))
    {
        _HttpDownloadProcess(pRef);
    }
    // upload processing
    if (pRef->state == UPLOAD)
    {
        _HttpUploadProcess(pRef);
    }

    // keep on idling
    return(ZCallback(&_CmdHttpIdleCB, HTTP_RATE));
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdHttp

    \Description
        Initiate an HTTP transaction.

    \Input *argz    -
    \Input argc     -
    \Input *argv[]  -

    \Output int32_t -

    \Version 10/28/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdHttp(ZContext *argz, int32_t argc, char *argv[])
{
    const char *pFileName = NULL, *pUrl;
    HttpRefT *pRef = &_Http_Ref;
    int32_t iResult = 0, iBufSize = HTTP_BUFSIZE;
    char strBuffer[4096];
    int32_t iStartArg = 2; // first arg past get/put/delete/whatever

    if (argc < 2)
    {
        return(0);
    }

    // check for help
    if ((argc >= 2) && !strcmp(argv[1], "help"))
    {
        _CmdHttpUsage(argc, argv);
        return(iResult);
    }

    // check for 'parse' command
    if ((argc == 3) && !strcmp(argv[1], "parse"))
    {
        char strKind[8], strHost[128];
        const char *pUri;
        int32_t iPort, iSecure;
        ds_memclr(strKind, sizeof(strKind));
        ds_memclr(strHost, sizeof(strHost));
        pUri = ProtoHttpUrlParse(argv[2], strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure);
        ZPrintf("parsed url: kind=%s, host=%s, port=%d, secure=%d, uri=%s\n", strKind, strHost, iPort, iSecure, pUri);
        return(0);
    }

    // if not initialized yet, do so now
    if (_Http_bInitialized == FALSE)
    {
        ds_memclr(pRef, sizeof(*pRef));
        ds_strnzcpy(pRef->strModuleName, argv[0], sizeof(pRef->strModuleName));
        pRef->iInpFile = ZFILE_INVALID;
        pRef->iOutFile = ZFILE_INVALID;
        _Http_bInitialized = TRUE;
    }

    // check for explicit destroy
    if ((argc == 2) && !ds_stricmp(argv[1], "destroy"))
    {
        if (pRef->http != NULL)
        {
            ProtoHttpDestroy(pRef->http);
            pRef->http = NULL;
        }
        if (pRef->pClientCert != NULL)
        {
            ZMemFree(pRef->pClientCert);
        }
        if (pRef->pClientKey != NULL)
        {
            ZMemFree(pRef->pClientKey);
        }
        _Http_bInitialized = FALSE;
        return(iResult);
    }

    // check for create request
    if ((argc == 3) && !strcmp(argv[1], "create"))
    {
        iBufSize = (int32_t)strtol(argv[2], NULL, 10);
    }

    // check for request to set a CA certificate
    if ((argc == 3) && ((!strcmp(argv[1], "cert")) || (!strcmp(argv[1], "cer2"))))
    {
        const uint8_t *pFileData;
        int32_t iFileSize;

        // try and open file
        if ((pFileData = (const uint8_t *)ZFileLoad(argv[2], &iFileSize, ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_BINARY)) != NULL)
        {
            if (!strcmp(argv[1], "cert"))
            {
                iResult = ProtoHttpSetCACert(pFileData, iFileSize);
            }
            else
            {
                iResult = ProtoHttpSetCACert2(pFileData, iFileSize);
            }
            ZMemFree((void *)pFileData);
        }
        else
        {
            ZPrintf("%s: unable to load certificate file '%s'\n", argv[0], argv[2]);
        }
        return((iResult >= 0) ? 0 : -1);
    }
    else if (!strcmp(argv[1], "cclr"))
    {
        ZPrintf("%s: clearing dynamic CA certs\n", argv[0]);
        ProtoHttpClrCACerts();
        return(0);
    }
    else if (!strcmp(argv[1], "cver"))
    {
        int32_t iInvalid;
        ZPrintf("%s: verifying dynamic CA certs\n", argv[0]);
        if ((iInvalid = ProtoHttpValidateAllCA()) > 0)
        {
            ZPrintf("%s: could not verify %d CA certs\n", argv[0], iInvalid);
            iResult = -1;
        }
        return(iResult);
    }
    else if ((argc == 3) && !strcmp(argv[1], "scrt"))
    {
        if ((pRef->pClientCert = (char *)_HttpLoadCertificate(argv[2], &pRef->iCertSize)) == NULL)
        {
            ZPrintf("%s: could not load client certificate '%s'\n", argv[0], argv[2]);
            iResult = -1;
        }
        return(iResult);
    }
    else if ((argc == 3) && !strcmp(argv[1], "skey"))
    {
        if ((pRef->pClientKey = (char *)_HttpLoadCertificate(argv[2], &pRef->iKeySize)) == NULL)
        {
            ZPrintf("%s: could not load client private key '%s'\n", argv[0], argv[2]);
            iResult = -1;
        }
        return(iResult);
    }

    // test url parsing
    if ((argc == 3) && !strcmp(argv[1], "urlparse"))
    {
        return(_HttpUrlParseTest(argv[0], argv[2]));
    }

    // test url encoding
    if ((argc >= 3) && !strcmp(argv[1], "urlencode"))
    {
        return(_HttpUrlEncodeTest(argv[0], argv+2, argc-2));
    }

    // create protohttp module if necessary
    if (pRef->http == NULL)
    {
        // disable the secure flags to help with our testing setup
        #if DIRTYCODE_GDK
        ProtoHttpControl(pRef->http, 'secu', 0, 0, NULL);
        #endif

        ZPrintf("%s: creating module with a %dkbyte buffer\n", argv[0], iBufSize);
        pRef->http = ProtoHttpCreate(iBufSize);
        if (pRef->http != NULL)
        {
            ProtoHttpCallback(pRef->http, _HttpCustomHeaderCallback, _HttpRecvHeaderCallback, pRef);
            pRef->iDebugLevel = 1;
        }
    }

    // check for create request -- if so, we're done
    if ((argc <= 3) && !strcmp(argv[1], "create"))
    {
        return(iResult);
    }
    else if ((argc > 2) && (argc < 7) && !strcmp(argv[1], "ctrl"))
    {
        int32_t iCmd, iValue = 0, iValue2 = 0;
        const char *pValue = NULL;

        iCmd  = argv[2][0] << 24;
        iCmd |= argv[2][1] << 16;
        iCmd |= argv[2][2] << 8;
        iCmd |= argv[2][3];

        if (argc > 3)
        {
            iValue = _GetIntArg(argv[3]);
        }

        if (argc > 4)
        {
            iValue2 = _GetIntArg(argv[4]);
        }

        if (argc > 5)
        {
            pValue = argv[5];
        }

        // snoop 'spam'
        if (iCmd == 'spam')
        {
            pRef->iDebugLevel = iValue;
        }

        return(ProtoHttpControl(pRef->http, iCmd, iValue, iValue2, (void *)pValue));
    }
    else if ((argc == 3) && !strcmp(argv[1], "stat"))
    {
        int32_t iCmd;

        iCmd  = argv[2][0] << 24;
        iCmd |= argv[2][1] << 16;
        iCmd |= argv[2][2] << 8;
        iCmd |= argv[2][3];

        iResult = ProtoHttpStatus(pRef->http, iCmd, strBuffer, sizeof(strBuffer));
        ZPrintf("http: ProtoHttpStatus('%s') returned %d\n", argv[2], iResult);
        if (strBuffer[0] != '\0')
        {
            ZPrintf("%s\n", strBuffer);
        }
        return(0);
    }
    // check for setting of base url
    else if ((argc == 3) && !strcmp(argv[1], "base"))
    {
        ProtoHttpSetBaseUrl(pRef->http, argv[2]);
        return(iResult);
    }
    // check for debug setting
    else if (!strcmp(argv[1], "debug") && (argc == 3))
    {
        int32_t iDebugLevel = (int32_t)strtol(argv[2], NULL, 10);
        pRef->iDebugLevel = iDebugLevel;
        return(iResult);
    }
    else if (!ds_stricmp(argv[1], "abrt"))
    {
        ProtoHttpAbort(pRef->http);
        pRef->state = IDLE;
        return(iResult);
    }

    // check for valid get/put request
    else if ((!ds_stricmp(argv[1], "get") || !ds_stricmp(argv[1], "head") || !ds_stricmp(argv[1], "put") || !ds_stricmp(argv[1], "post") ||
              !ds_stricmp(argv[1], "puts") || !ds_stricmp(argv[1], "delete") || !ds_stricmp(argv[1], "options")) &&
            ((argc > 2) || (argc < 6)))
    {
        int32_t iArg;

        // reset to clear any previous stats
        _HttpReset(pRef);

        // init start timer
        pRef->sttime = NetTick();

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

        // set client cert if specified
        if (pRef->pClientCert != NULL)
        {
            ProtoHttpControl(pRef->http, 'scrt', pRef->iCertSize, 0, pRef->pClientCert);
        }
        // set client key if specified
        if (pRef->pClientKey != NULL)
        {
            ProtoHttpControl(pRef->http, 'skey', pRef->iKeySize, 0, pRef->pClientKey);
        }

        // fall-through to code below
    }
    else
    {
        ZPrintf("   unrecognized or badly formatted command '%s'\n", argv[1]);
        _CmdHttpUsage(argc, argv);
        return(iResult);
    }

    // locate url and filename
    pUrl = argv[iStartArg];
    if (argc > iStartArg)
    {
        pFileName = argv[iStartArg+1];
    }

    // do we have a url?
    if (pUrl != NULL)
    {
        const char* pTemp = pUrl; //< Used for parsing the query param
        char strKind[5], strHost[128], strName[32], strValue[32];
        int32_t iPort, iSecure;

        // get url info
        ProtoHttpUrlParse(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure);

        // if the host has changed, reset cookie buffer
        if (ds_stricmp(pRef->strHost, strHost) && (pRef->strCookie[0] != '\0'))
        {
            ZPrintf("http: host change to %s; resetting cookies from %s\n", strHost, pRef->strHost);
            pRef->strCookie[0] = '\0';
        }
        // save host
        ds_strnzcpy(pRef->strHost, strHost, sizeof(strHost));

        while (ProtoHttpGetNextParam(pRef->http, pTemp, strName, sizeof(strName), strValue, sizeof(strValue), &pTemp) == 0)
        {
            ProtoHttpUrlDecodeStrParm(strValue, strValue, sizeof(strValue));

            #if 0
            ZPrintf("http query param: key=%s, value=%s\n", strName, strValue);
            #endif
        }
    }
    else
    {
        return(0);
    }

    // set append header
    strBuffer[0] = '\0';
    if (pRef->strCookie[0] != '\0')
    {
        ds_snzprintf(strBuffer, sizeof(strBuffer), "Cookie: %s\r\n", pRef->strCookie);
    }
    ds_strnzcat(strBuffer, pRef->strApndHdr, sizeof(strBuffer));
    ProtoHttpControl(pRef->http, 'apnd', 0, 0, strBuffer);
    // set debug level
    ProtoHttpControl(pRef->http, 'spam', pRef->iDebugLevel, 0, NULL);

    // if we're uploading, open required input file
    if (!ds_stricmp(argv[1], "put") || !ds_stricmp(argv[1], "post") || !ds_stricmp(argv[1], "puts"))
    {
        ZPrintf("%s: uploading %s to %s\n", argv[0], pFileName, pUrl);

        // assume failure
        iResult = -1;

        // try and open file
        if ((pRef->iInpFile = ZFileOpen(pFileName, ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_BINARY)) != ZFILE_INVALID)
        {
            // get the file size
            if ((pRef->iDataSize = ZFileSize(pRef->iInpFile)) > 0)
            {
                // load data from file
                if ((pRef->iSendBufData = ZFileRead(pRef->iInpFile, pRef->strFileBuffer, sizeof(pRef->strFileBuffer))) > 0)
                {
                    if (ds_stricmp(argv[1], "puts"))
                    {
                        // initiate put/post transaction
                        ZPrintf("%s: uploading %d bytes\n", argv[0], pRef->iDataSize);
                        if ((pRef->iSendBufSent = ProtoHttpPost(pRef->http, pUrl, pRef->strFileBuffer, pRef->iDataSize, !ds_stricmp(argv[1], "put") ? PROTOHTTP_PUT : PROTOHTTP_POST)) < 0)
                        {
                            ZPrintf("%s: error %d initiating send\n", argv[0], pRef->iSendBufSent);
                            iResult = -1;
                        }
                        else if (pRef->iSendBufSent >= 0)
                        {
                            ZPrintf("%s: sent %d bytes\n", argv[0], pRef->iSendBufSent);
                            pRef->iSentSize = pRef->iSendBufSent;
                        }
                    }
                    else
                    {
                        // initiate streaming put
                        if ((iResult = ProtoHttpPost(pRef->http, pUrl, NULL, PROTOHTTP_STREAM_BEGIN, PROTOHTTP_POST)) >= 0)
                        {
                            pRef->bStreaming = TRUE;
                        }
                    }

                    // wait for reply
                    pRef->state = UPLOAD;

                    // locate output file
                    pFileName = (argc > (iStartArg+2)) ? argv[iStartArg+2] : NULL;
                }
                else
                {
                    ZPrintf("%s: error %d reading data from file\n", argv[0], pRef->iSendBufData, pFileName);
                }
            }
            else
            {
                ZPrintf("%s: error %d getting size of file %s\n", argv[0], pRef->iDataSize, pFileName);
            }
        }
        else
        {
            ZPrintf("%s: unable to load file '%s'\n", argv[0], pFileName);
        }
    }

    // open output file?
    if (pFileName != NULL)
    {
        ZPrintf("%s: saving %s data to %s\n", argv[0], pUrl, pFileName);
        if ((pRef->iOutFile = ZFileOpen(pFileName, ZFILE_OPENFLAG_WRONLY|ZFILE_OPENFLAG_BINARY|ZFILE_OPENFLAG_CREATE)) == ZFILE_INVALID)
        {
            ZPrintf("%s: error opening file '%s' for writing\n", argv[0], pFileName);
        }
    }

    // issue request
    if (!ds_stricmp(argv[1], "get") || !ds_stricmp(argv[1], "head") || !ds_stricmp(argv[1], "delete") || !ds_stricmp(argv[1], "options"))
    {
        ProtoHttpRequestTypeE eRequestType;

        ZPrintf("%s: downloading %s\n", argv[0], pUrl);

        // map to protohttp request type
        if (!ds_stricmp(argv[1], "head"))
        {
            eRequestType = PROTOHTTP_REQUESTTYPE_HEAD;
        }
        else if (!ds_stricmp(argv[1], "get"))
        {
            eRequestType = PROTOHTTP_REQUESTTYPE_GET;
        }
        else if (!ds_stricmp(argv[1], "delete"))
        {
            eRequestType = PROTOHTTP_REQUESTTYPE_DELETE;
        }
        else if (!ds_stricmp(argv[1], "options"))
        {
            eRequestType = PROTOHTTP_REQUESTTYPE_OPTIONS;
        }
        else
        {
            ZPrintf("%s: unrecognized request %s\n", argv[0], argv[1]);
            return(-1);
        }

        if (pRef->bUseWriteCb)
        {
            iResult = ProtoHttpRequestCb(pRef->http, pUrl, NULL, 0, eRequestType, _HttpWriteCb, pRef);
        }
        else
        {
            iResult = ProtoHttpRequest(pRef->http, pUrl, NULL, 0, eRequestType);
        }

        if (iResult == 0)
        {
            pRef->state = DNLOAD;
        }
    }

    // set up recurring callback to process transaction
    if (pRef->state != IDLE)
    {
        iResult = ZCallback(_CmdHttpIdleCB, HTTP_RATE);
    }

    return(iResult);
}

