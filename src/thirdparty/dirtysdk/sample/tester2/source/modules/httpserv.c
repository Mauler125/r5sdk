/*H********************************************************************************/
/*!
    \File httpserv.c

    \Description
        Implements basic http server using ProtoHttpServ

    \Copyright
        Copyright (c) 2012 Electronic Arts Inc.

    \Version 09/11/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef __linux__
#include <sys/time.h> // gettimeofday
#endif

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/netconn.h" // NetConnSleep
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/proto/protohttpserv.h"

#include "libsample/zlib.h"
#include "libsample/zmem.h"
#include "libsample/zfile.h"

#include "testermodules.h"

/*** Defines **********************************************************************/

#define HTTPSERV_RATE           (1)
#define HTTPSERV_LISTENPORT     (9000)

/*** Function Prototypes **********************************************************/

/*** Type Definitions *************************************************************/

typedef struct HttpServT
{
    ProtoHttpServRefT *pHttpServ;

    char *pServerCert;
    int32_t iServerCertLen;
    char *pServerKey;
    int32_t iServerKeyLen;
    int32_t iChunkLen;

    char strServerName[128];
    char strFileDir[512];
} HttpServT;

/*** Variables ********************************************************************/

static HttpServT _HttpServ_Ref;
static uint8_t   _HttpServ_bInitialized = FALSE;

//! map of filename extensions to content-types
static const char *_ProtoHttpServ_strContentTypes[][2] =
{
    { ".htm",  "text/html"  },
    { ".html", "text/html"  },
    { ".css",  "text/css"   },
    { ".xml",  "text/xml"   },
    { ".jpg",  "image/jpeg" },
    { ".gif",  "image/gif"  },
    { ".png",  "image/png"  },
    { ".mp3",  "audio/mpeg" }
};

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
    \Function _HttpServGetCurrentTime

    \Description
       Gets and parses the current time into components

    \Input *uYear   - pointer to uint32_t var to store 'year' in
    \Input *uMonth  - pointer to uint8_t var to store 'month' in
    \Input *uDay    - pointer to uint8_t var to store 'day' in
    \Input *uHour   - pointer to uint8_t var to store 'hour' in
    \Input *uMin    - pointer to uint8_t var to store 'min' in
    \Input *uSec    - pointer to uint8_t var to store 'sec' in
    \Input *uMillis - pointer to uint32_t var to store 'milliseconds' in

    \Version 10/30/2013 (jbrookes) Borrowed from eafn logger
*/
/********************************************************************************F*/
static void _HttpServGetCurrentTime(uint32_t *uYear, uint8_t *uMonth, uint8_t *uDay, uint8_t *uHour, uint8_t *uMin, uint8_t *uSec, uint32_t *uMillis)
{
#if DIRTYCODE_PC
    SYSTEMTIME SystemTime;
    GetLocalTime(&SystemTime);
    *uYear = SystemTime.wYear;
    *uMonth = SystemTime.wMonth;
    *uDay = SystemTime.wDay;
    *uHour = SystemTime.wHour;
    *uMin = SystemTime.wMinute;
    *uSec = SystemTime.wSecond;
    *uMillis = SystemTime.wMilliseconds;
#else // all non-pc
#if DIRTYCODE_LINUX
    struct timeval tv;
    struct tm *pTime;
    gettimeofday(&tv, NULL);
    pTime = gmtime((time_t *)&tv.tv_sec);
    *uMillis = tv.tv_usec / 1000;
#else
    //$$TODO: plat-time doesn't have anything to get millis
    struct tm TmTime, *pTime;
    pTime = ds_secstotime(&TmTime, ds_timeinsecs());
    *uMillis = 0;
#endif
    *uYear = 1900 + pTime->tm_year;
    *uMonth = pTime->tm_mon + 1;
    *uDay = pTime->tm_mday;
    *uHour = pTime->tm_hour;
    *uMin = pTime->tm_min;
    *uSec = pTime->tm_sec;
#endif // !DIRTYCODE_PC
} 

/*F********************************************************************************/
/*!
    \Function _HttpServGetContentType

    \Description
        Get content-type based on target url

    \Input *strUrl   - full url including file

    \Output
        const char *    - content type

    \Version 07/09/2013 (jbrookes)
*/
/********************************************************************************F*/
static const char *_HttpServGetContentType(const char *strUrl)
{
    const char *pContentType = _ProtoHttpServ_strContentTypes[0][1];
    int32_t iType;

    for (iType = 0; iType < (signed)(sizeof(_ProtoHttpServ_strContentTypes)/sizeof(_ProtoHttpServ_strContentTypes[0])); iType += 1)
    {
        if (ds_stristr(strUrl, _ProtoHttpServ_strContentTypes[iType][0]))
        {
            pContentType = _ProtoHttpServ_strContentTypes[iType][1];
            break;
        }
    }

    return(pContentType);
}

/*F********************************************************************************/
/*!
    \Function _HttpServLoadCertificate

    \Description
        Load pem certificate file and trim begin/end text.

    \Input *pFilename       - name of certificate file to open
    \Input *pCertSize       - [out] storage for size of certificate

    \Output
        const char *        - certificate data

    \Version 10/11/2013 (jbrookes)
*/
/********************************************************************************F*/
static char *_HttpServLoadCertificate(const char *pFilename, int32_t *pCertSize)
{
    char *pCertBuf;
    int32_t iFileSize;

    // load certificate file
    if ((pCertBuf = (char *)ZFileLoad(pFilename, &iFileSize, ZFILE_OPENFLAG_RDONLY)) == NULL)
    {
        return(NULL);
    }

    // set size and return buffer to caller
    *pCertSize = iFileSize;
    return(pCertBuf);
}

/*F********************************************************************************/
/*!
    \Function _HttpServProcessGet

    \Description
        Process GET/HEAD request

    \Input *pServerState    - module state
    \Input *pRequest        - request information
    \Input *pResponse       - [out] response information 

    \Output
        int32_t         - response code

    \Version 12/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServProcessGet(HttpServT *pServerState, const ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse)
{
    const char* pHdr = pRequest->strHeader;
    char strFilePath[4096], strName[128], strValue[4*1024];
    ZFileStatT FileStat;
    int32_t iFileLen, iResult;
    ZFileT iFileId;
    uint32_t uModifiedSince = 0, uUnmodifiedSince = 0;
    struct tm TmTime;
    char strTime[64];

    // if empty url, substitute default
    if (*(pRequest->strUrl+1) == '\0')
    {
        ds_snzprintf(strFilePath, sizeof(strFilePath), "%s/index.html", pServerState->strFileDir, pRequest->strUrl + 1);
    }
    else
    {
        ds_snzprintf(strFilePath, sizeof(strFilePath), "%s/%s", pServerState->strFileDir, pRequest->strUrl + 1);
    }

    // stat the file
    if ((iResult = ZFileStat(strFilePath, &FileStat)) != ZFILE_ERROR_NONE)
    {
        // no file
        ZPrintf("httpserv: could not stat file '%s'\n", strFilePath);
        pResponse->eResponseCode = PROTOHTTP_RESPONSE_NOTFOUND;
        return(-1);
    }

    // see if url refers to a file
    if ((iFileId = ZFileOpen(strFilePath, ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_BINARY)) == ZFILE_INVALID)
    {
        ZPrintf("httpserv: could not open file '%s' for reading\n", strFilePath);
        pResponse->eResponseCode = PROTOHTTP_RESPONSE_INTERNALSERVERERROR;
        return(-2);
    }
    // get the file size
    if ((iFileLen = (int32_t)ZFileSize(iFileId)) < 0)
    {
        ZPrintf("httpserv: error %d getting file size\n", iFileLen);
        pResponse->eResponseCode = PROTOHTTP_RESPONSE_INTERNALSERVERERROR;
        return(-3);
    }

    // parse the header for request specific data
    while (ProtoHttpGetNextHeader(NULL, pHdr, strName, sizeof(strName), strValue, sizeof(strValue), &pHdr) == 0)
    {
        if (ds_stricmp(strName, "if-modified-since") == 0)
        {
            uModifiedSince = (uint32_t)ds_strtotime(strValue);
        }
        else if (ds_stricmp(strName, "if-unmodified-since") == 0)
        {
            uUnmodifiedSince = (uint32_t)ds_strtotime(strValue);
        }
    }

    if (uModifiedSince != 0 && ((int32_t)(FileStat.uTimeModify-uModifiedSince) <= 0))
    {
        pResponse->eResponseCode = PROTOHTTP_RESPONSE_NOTMODIFIED;
        ZPrintf("httpserv: file not modified (%d-%d=%d)\n", uModifiedSince, FileStat.uTimeModify,
            (int32_t)(FileStat.uTimeModify-uModifiedSince));
        return(0);
    }
    if (uUnmodifiedSince != 0 && ((int32_t)(FileStat.uTimeModify-uUnmodifiedSince) > 0))
    {
        ZPrintf("httpserv: file modified since (%d-%d=%d)\n", uUnmodifiedSince, FileStat.uTimeModify,
            (int32_t)(FileStat.uTimeModify-uUnmodifiedSince));
        pResponse->eResponseCode = PROTOHTTP_RESPONSE_PRECONFAILED;
        return(-4);
    }

    // set last modified time
    ds_secstotime(&TmTime, FileStat.uTimeModify);
    pResponse->iHeaderLen += ds_snzprintf(pResponse->strHeader, sizeof(pResponse->strHeader)-pResponse->iHeaderLen, "Last-Modified: %s\r\n",
        ds_timetostr(&TmTime, TIMETOSTRING_CONVERSION_RFC_0822, FALSE, strTime, sizeof(strTime)));

    // set content-type
    ds_strnzcpy(pResponse->strContentType, _HttpServGetContentType(pRequest->strUrl), sizeof(pResponse->strContentType));

    // set request info
    pResponse->iContentLength = iFileLen;
    pResponse->iChunkLength = pServerState->iChunkLen;
    pResponse->pData = (void *)iFileId;
    pResponse->eResponseCode = PROTOHTTP_RESPONSE_OK;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _HttpServProcessPut

    \Description
        Process PUT/POST request

    \Input *pServerState   - module state
    \Input *pRequest       - request information
    \Input *pResponse      - [out] response information

    \Output
        int32_t         - response code

    \Version 12/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServProcessPut(HttpServT *pServerState, const ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse)
{
    ZFileT iFileId = (ZFileT)(uintptr_t)pRequest->pData;
    if (iFileId == 0 || iFileId == ZFILE_INVALID)
    {
        pResponse->eResponseCode = PROTOHTTP_RESPONSE_INTERNALSERVERERROR;
        return(-1);
    }

    // we've processed the request
    pResponse->pData = (void *)(uintptr_t)iFileId;
    pResponse->eResponseCode = PROTOHTTP_RESPONSE_CREATED;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _HttpServRequestCb

    \Description
        ProtoHttpServ request callback handler

    \Input *pRequest    - request information
    \Input *pResponse   - [out] response information
    \Input *pUserData   - callback user data

    \Output
        int32_t         - response code

    \Version 12/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServRequestCb(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData)
{
    HttpServT *pServerState = (HttpServT *)pUserData;
    int32_t iResult = -1;

    // init default response values
    pResponse->eResponseCode = PROTOHTTP_RESPONSE_NOTIMPLEMENTED;

    // handle the request
    if ((pRequest->eRequestType == PROTOHTTP_REQUESTTYPE_GET) || (pRequest->eRequestType == PROTOHTTP_REQUESTTYPE_HEAD))
    {
        iResult = _HttpServProcessGet(pServerState, pRequest, pResponse);
    }
    if ((pRequest->eRequestType == PROTOHTTP_REQUESTTYPE_PUT) || (pRequest->eRequestType == PROTOHTTP_REQUESTTYPE_PATCH) || (pRequest->eRequestType == PROTOHTTP_REQUESTTYPE_POST))
    {
        iResult = _HttpServProcessPut(pServerState, pRequest, pResponse);
    }

    // return result to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _HttpServReceiveCb

    \Description
        ProtoHttpServ inbound data callback handler

    \Input *pServerState- module state
    \Input *pBuffer     - data to write
    \Input iBufSize     - size of the data
    \Input *pUserData   - user data

    \Output
        int32_t         - negative=failure, else bytes written

    \Version 12/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServReceiveCb(ProtoHttpServRequestT *pRequest, const char *pBuffer, int32_t iBufSize, void *pUserData)
{
    ZFileT iFileId = (ZFileT)(uintptr_t)pRequest->pData;
    int32_t iResult = 0;

    // if the file failed to load previously or
    // not the correct request type
    // then early out
    if (iFileId == ZFILE_INVALID)
    {
        return(-1);
    }
    if (pRequest->eRequestType != PROTOHTTP_REQUESTTYPE_POST && pRequest->eRequestType != PROTOHTTP_REQUESTTYPE_PUT && pRequest->eRequestType != PROTOHTTP_REQUESTTYPE_PATCH)
    {
        return(iBufSize);
    }

    // check for upload completion
    if (pBuffer == NULL)
    {
        ZFileClose(iFileId);
    }
    else if ((iResult = ZFileWrite(iFileId, (void *)pBuffer, iBufSize)) < 0)
    {
        NetPrintf(("httpserv: error %d writing to file\n", iResult));
    }
    // return result
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _HttpServHeaderCb

    \Description
        ProtoHttpServ inbound data callback handler

    \Input *pRequest    - [in/out] data used to process the request
    \Input *pResponse   - [out] data used to send the response
    \Input *pUserData   - user data

    \Output
        int32_t         - negative=failure, zero=success
*/
/********************************************************************************F*/
static int32_t _HttpServHeaderCb(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData)
{
    HttpServT *pServerState = (HttpServT *)pUserData;
    char strFilePath[1024];
    ZFileT iFileId = ZFILE_INVALID;

    if (pRequest->eRequestType == PROTOHTTP_REQUESTTYPE_POST ||
        pRequest->eRequestType == PROTOHTTP_REQUESTTYPE_PUT  ||
        pRequest->eRequestType == PROTOHTTP_REQUESTTYPE_PATCH)
    {
        // create filepath
        ds_snzprintf(strFilePath, sizeof(strFilePath), "%s\\%s", pServerState->strFileDir, pRequest->strUrl + 1);

        // try to open the file
        if ((iFileId = ZFileOpen(strFilePath, ZFILE_OPENFLAG_WRONLY | ZFILE_OPENFLAG_BINARY)) == ZFILE_INVALID)
        {
            ZPrintf("httpserv: could not open file '%s' for writing\n", strFilePath);
            pResponse->eResponseCode = PROTOHTTP_RESPONSE_INTERNALSERVERERROR;
            return(-1);
        }
    }

    // set the file id for use during receiving/sending
    pRequest->pData = (void *)(uintptr_t)iFileId;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _HttpServSendCb

    \Description
        ProtoHttpServ outbound callback handler

    \Input *pServerState    - module state
    \Input *pBuffer         - data to write to
    \Input iBufSize         - size of the data
    \Input *pUserData       - user data

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 12/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServSendCb(ProtoHttpServResponseT *pResponse, char *pBuffer, int32_t iBufSize, void *pUserData)
{
    //HttpServT *pServerState = (HttpServT *)pUserData;
    ZFileT iFileId = (ZFileT)(uintptr_t)pResponse->pData;
    int32_t iResult = 0;

    if (iFileId == ZFILE_INVALID)
    {
        return(0);
    }

    // check for download completion
    if (pBuffer == NULL)
    {
        ZFileClose(iFileId);
    }
    else if ((iResult = ZFileRead(iFileId, pBuffer, iBufSize)) < 0)
    {
        ZPrintf("httpserv: error %d reading from file\n", iResult);
    }
    // return result
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _HttpServLogCb

    \Description
        ProtoHttpServ logging function

    \Input *pText       - text to print
    \Input *pUserData   - user data (module state)

    \Output
        int32_t         - zero

    \Version 12/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpServLogCb(const char *pText, void *pUserData)
{
    uint32_t uYear, uMillis;
    uint8_t uMonth, uDay, uHour, uMin, uSec;

    // format prefix to output buffer
    _HttpServGetCurrentTime(&uYear, &uMonth, &uDay, &uHour, &uMin, &uSec, &uMillis);

    ZPrintf("%02u/%02u/%02u %02u:%02u:%02u.%03u %s", uYear, uMonth, uDay, uHour, uMin, uSec, uMillis, pText);
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpServIdleCB

    \Description
        Callback to process while idle

    \Input *argz    -
    \Input argc     -
    \Input *argv[]  -

    \Output int32_t -

    \Version 09/26/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdHttpServIdleCB(ZContext *argz, int32_t argc, char *argv[])
{
    HttpServT *pRef = &_HttpServ_Ref;

    // shut down?
    if (argc == 0)
    {
        if (pRef->pHttpServ != NULL)
        {
            ProtoHttpServDestroy(pRef->pHttpServ);
            pRef->pHttpServ = NULL;
        }
        return(0);
    }
    // httpserv destroyed?
    if (pRef->pHttpServ == NULL)
    {
        return(0);
    }

    // update protohttpserv module
    ProtoHttpServUpdate(pRef->pHttpServ);

    // keep on idling
    return(ZCallback(&_CmdHttpServIdleCB, HTTPSERV_RATE));
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpServUsage

    \Description
        Display usage information.

    \Input argc         - argument count
    \Input *argv[]      - argument list

    \Version 09/13/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdHttpServUsage(int argc, char *argv[])
{
    if (argc <= 2)
    {
        ZPrintf("httpserv:    basic http server\n");
        ZPrintf("httpserv:    usage: %s [alpn|cert|chunked|ciph|filedir|listen|setcert|setkey|stop|vers|vmin]", argv[0]);
    }
    else if (argc == 3)
    {
        if (!strcmp(argv[2], "alpn"))
        {
            ZPrintf("httpserv:    usage: %s alpn [alpnstr]\n", argv[0]);
        }
        if (!strcmp(argv[2], "ccrt"))
        {
            ZPrintf("httpserv:    usage: %s ccrt [level]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "cert"))
        {
            ZPrintf("httpserv:    usage: %s cert [cacertfile]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "chunked"))
        {
            ZPrintf("httpserv:    usage: %s chunked [chunklen]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "ciph"))
        {
            ZPrintf("httpserv:    usage: %s ciph [ciphers]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "filedir"))
        {
            ZPrintf("httpserv:    usage: %s filedir <directory>\n", argv[0]);
        }
        else if (!strcmp(argv[2], "listen"))
        {
            ZPrintf("httpserv:    usage: %s listen [listenport] <secure>\n", argv[0]);
        }
        else if (!strcmp(argv[2], "setcert"))
        {
            ZPrintf("httpserv:    usage: %s setcert [certfile]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "setkey"))
        {
            ZPrintf("httpserv:    usage: %s setkey [certkey]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "stop"))
        {
            ZPrintf("httpserv:    usage: %s stop\n", argv[0]);
        }
        else if (!strcmp(argv[2], "vers"))
        {
            ZPrintf("httpserv:    usage: %s vers [version]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "vmin"))
        {
            ZPrintf("httpserv:    usage: %s vmin [version]\n", argv[0]);
        }
    }
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdHttpServ

    \Description
        Simple HTTP server

    \Input *argz    - context
    \Input argc     - command count
    \Input *argv[]  - command arguments

    \Output int32_t -

    \Version 09/11/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdHttpServ(ZContext *argz, int32_t argc, char *argv[])
{
    HttpServT *pServerState = &_HttpServ_Ref;
    int32_t iResult = 0;

    if ((argc < 2) || !ds_stricmp(argv[1], "help"))
    {
        _CmdHttpServUsage(argc, argv);
        return(iResult);
    }

    // if not initialized yet, do so now
    if (_HttpServ_bInitialized == FALSE)
    {
        ds_memclr(pServerState, sizeof(*pServerState));
        ds_strnzcpy(pServerState->strServerName, "HttpServ", sizeof(pServerState->strServerName));
        ds_strnzcpy(pServerState->strFileDir, "c:\\temp\\httpserv", sizeof(pServerState->strFileDir));
        _HttpServ_bInitialized = TRUE;
    }

    // check for filedir set
    if ((argc == 3) && !ds_stricmp(argv[1], "filedir"))
    {
        ds_strnzcpy(pServerState->strFileDir, argv[2], sizeof(pServerState->strFileDir));
        return(iResult);
    }

    // check for listen
    if ((argc >= 2) && (argc < 5) && !ds_stricmp(argv[1], "listen"))
    {
        int32_t iPort = HTTPSERV_LISTENPORT;
        int32_t iSecure = 0; // insecure by default

        if ((argc > 3) && !ds_stricmp(argv[3], "secure"))
        {
            iSecure = 1;
        }
        if (argc > 2)
        {
            iPort = (int32_t)strtol(argv[2], NULL, 10);
            if ((iPort < 1) || (iPort > 65535))
            {
                ZPrintf("httpserv: invalid port %d specified in listen request\n", iPort);
                return(-1);
            }
        }

        // destroy previous httpserv ref, if any
        if (pServerState->pHttpServ != NULL)
        {
            ProtoHttpServDestroy(pServerState->pHttpServ);
        }

        // create new httpserv ref
        if ((pServerState->pHttpServ = ProtoHttpServCreate(iPort, iSecure, pServerState->strServerName)) == NULL)
        {
            ZPrintf("httpserv: could not create httpserv state on port %d\n", iPort);
            return(-2);
        }

        // set up httpserv callbacks
        ProtoHttpServCallback(pServerState->pHttpServ, _HttpServRequestCb, _HttpServReceiveCb, _HttpServSendCb, _HttpServHeaderCb, _HttpServLogCb, pServerState);

        // install recurring update
        iResult = ZCallback(_CmdHttpServIdleCB, HTTPSERV_RATE);
    }

    // the following functions require http serv state
    if (pServerState->pHttpServ == NULL)
    {
        ZPrintf("%s: '%s' requires httpserv creation (use 'listen' command)\n", argv[0], argv[1]);
        return(iResult);
    }

    // check for server stop
    if ((argc == 2) && !ds_stricmp(argv[1], "stop"))
    {
        ProtoHttpServDestroy(pServerState->pHttpServ);
        pServerState->pHttpServ = NULL;

        ZPrintf("httpserv: protohttpserv stopped listening\n");
    }

    // check for setting alpn
    if ((argc == 3) && !ds_stricmp(argv[1], "alpn"))
    {
        return(ProtoHttpServControl(pServerState->pHttpServ, 'alpn', 0, 0, argv[2]));
    }

    // check for client cert level specification
    if ((argc == 3) && !ds_stricmp(argv[1], "ccrt"))
    {
        return(ProtoHttpServControl(pServerState->pHttpServ, 'ccrt', (int32_t)strtol(argv[2], NULL, 10), 0, NULL));
    }

    // check for cacert load
    if ((argc == 3) && !ds_stricmp(argv[1], "cert"))
    {
        const uint8_t *pFileData;
        int32_t iFileSize;

        // try and open file
        if ((pFileData = (const uint8_t *)ZFileLoad(argv[2], &iFileSize, ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_BINARY)) != NULL)
        {
            iResult = ProtoSSLSetCACert(pFileData, iFileSize);
            ZMemFree((void *)pFileData);
        }
        else
        {
            ZPrintf("%s: unable to load certificate file '%s'\n", argv[0], argv[2]);
        }
        return((iResult > 0) ? 0 : -1);
    }

    // check for chunked transfer (download) enable
    if ((argc >= 2) && !ds_stricmp(argv[1], "chunked"))
    {
        int32_t iChunkLen = 4096;
        if (argc > 2)
        {
            iChunkLen = (int32_t)strtol(argv[2], NULL, 10);
        }
        ZPrintf("httpserv: enabling chunked transfers and setting chunk size to %d\n", iChunkLen);
        pServerState->iChunkLen = iChunkLen;
        return(iResult);
    }

    // check for cipher set
    if ((argc == 3) && !ds_stricmp(argv[1], "ciph"))
    {
        return(ProtoHttpServControl(pServerState->pHttpServ, 'ciph', (int32_t)strtol(argv[2], NULL, 16), 0, NULL));
    }

    // check for server certificate
    if ((argc == 3) && !ds_stricmp(argv[1], "setcert"))
    {
        if ((pServerState->pServerCert = _HttpServLoadCertificate(argv[2], &pServerState->iServerCertLen)) != NULL)
        {
            iResult = ProtoHttpServControl(pServerState->pHttpServ, 'scrt', pServerState->iServerCertLen, 0, pServerState->pServerCert);
        }
        else
        {
            ZPrintf("%s: could not load certificate '%s'\n", argv[0], argv[2]);
            iResult = -1;
        }
        return(iResult);
    }

    // check for server private key
    if ((argc == 3) && !ds_stricmp(argv[1], "setkey"))
    {
        if ((pServerState->pServerKey = _HttpServLoadCertificate(argv[2], &pServerState->iServerKeyLen)) != NULL)
        {
            iResult = ProtoHttpServControl(pServerState->pHttpServ, 'skey', pServerState->iServerKeyLen, 0, pServerState->pServerKey);
        }
        else
        {
            ZPrintf("httpserv: could not load private key '%s'\n", argv[2]);
            iResult = -1;
        }
        return(iResult);
    }

    // check for version set
    if ((argc == 3) && !ds_stricmp(argv[1], "vers"))
    {
        return(ProtoHttpServControl(pServerState->pHttpServ, 'vers', (int32_t)strtol(argv[2], NULL, 16), 0, NULL));
    }

    // check for min version set
    if ((argc == 3) && !ds_stricmp(argv[1], "vmin"))
    {
        return(ProtoHttpServControl(pServerState->pHttpServ, 'vmin', (int32_t)strtol(argv[2], NULL, 16), 0, NULL));
    }

    // check for control 
    if ((argc > 3) && (!ds_stricmp(argv[1], "ctrl")))
    {
        int32_t iCmd, iThread, iValue = 0, iValue2 = 0;
        const char *pValue = NULL;

        iCmd = _GetIntArg(argv[2]);
        iThread = _GetIntArg(argv[3]);

        if (argc > 4)
        {
            iValue = _GetIntArg(argv[3]);
        }
        if (argc > 5)
        {
            iValue2 = _GetIntArg(argv[4]);
        }
        if (argc > 6)
        {
            pValue = argv[5];
        }

        return(ProtoHttpServControl2(pServerState->pHttpServ, iThread, iCmd, iValue, iValue2, (void *)pValue));
    }

    return(iResult);
}
