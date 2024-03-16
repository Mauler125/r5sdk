/*H**************************************************************************/
/*!

    \File dirtycert.c

    \Description
        This module defines the CA fallback mechanism which is used by ProtoSSL.

    \Notes
        This module is designed to be thread-safe except the creating/destroying APIs,
        however because some other code (such as CA list) are not thread-safe yet,
        so it's not totally thread-safe right now.
        If two protossl instances are requesting the same CA cert at the same time,
        ref-counting is used and only one CA fetch request will be made to the server.

    \Copyright
        Copyright (c) Electronic Arts 2012.

    \Version 01/23/2012 (szhu)
*/
/***************************************************************************H*/

/*** Include files ***********************************************************/

#include <string.h>

#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/crypt/cryptarc4.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/xml/xmlparse.h"
#include "DirtySDK/util/base64.h"

#include "DirtySDK/dirtysock/dirtycert.h"

/*** Defines ***************************************************************************/

#define DIRTYCERT_MAXREQUESTS       (16)        //!< max concurrent CA requests
#define DIRTYCERT_TIMEOUT           (30*1000)   //!< default dirtycert timeout
#define DIRTYCERT_MAXURL            (2048)      //!< max url length
#define DIRTYCERT_MAXCERTIFICATE    (8*1024)    //!< max certificate size, in bytes
#define DIRTYCERT_MAXRESPONSE       ((DIRTYCERT_MAXCERTIFICATE)*3) //! max response size, in bytes
#define DIRTYCERT_MAXCERTDECODED    ((Base64DecodedSize(DIRTYCERT_MAXCERTIFICATE)+3)&0x7ffc) //!< max certificate size (decoded), in bytes
#define DIRTYCERT_REQUESTID_NONE    (-1)        //!< request id none
#define DIRTYCERT_VERSION           (0x0101)    //!< dirtycert version: update this for major bug fixes or protocol additions/changes

/* dirtycert production url; NOTE: this url is encrypted as stored in the binary to prevent aa
   malicious user from being able to use an easy  string search to find and possibly modify it.
   if the url is changed, _DirtyCertUrlSetup() will complain and spit out new enc/key parameters
   for the updated url, which must be used to replace the current definitions in that function. */
#define DIRTYCERT_URL               "https://gosca18.ea.com:4432%d/redirector"
#define DIRTYCERT_URL_ENCRYPTED     (TRUE)      //!< TRUE if the dirtycert URL is specified in encrypted form; DO NOT CHECK IN DISABLED

#if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
 #define DIRTYCERT_PORT_OFFSET      (6)         //!< xbox platform offset is +1 from other platforms
#else
 #define DIRTYCERT_PORT_OFFSET      (5)         //!< default platform offset is +5 from the base port number of 44320
#endif

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! request type
typedef enum RequestTypeE
{
    RT_ONDEMAND = 0,
    RT_PREFETCH
} RequestTypeE;

//! request status
typedef enum RequestStatusE
{
    RS_NONE = 0,
    RS_NOT_STARTED,
    RS_IN_PROGRESS,
    RS_DONE,
    RS_FAILED,
} RequestStatusE;

//! CA fetch request
typedef struct DirtyCertCARequestT
{
    ProtoSSLCertInfoT CertInfo;     //!< certificate info (on-demand request only)
    char strHost[256];              //!< host we are making an on-demand request for (on-demand request only)
    int32_t iPort;                  //!< port we are making an on-demand request for (on-demand request only)
    RequestTypeE eType;             //!< request type
    RequestStatusE eStatus;         //!< request status
    int32_t iRefCount;              //!< ref count for this request
}DirtyCertCARequestT;

//! module state
struct DirtyCertRefT
{
    // module memory group
    int32_t iMemGroup;              //!< module mem group id
    void *pMemGroupUserData;        //!< user data associated with mem group

    NetCritT crit;                  //!< critical section used to ensure thread safety
    ProtoHttpRefT *pHttp;           //!< http ref

    char strServiceName[DIRTYCERT_SERVICENAME_SIZE];       //!< service name used to identify requester (required)
    char strUrl[DIRTYCERT_MAXURL];  //!< url buffer

    // buffer for response processing
    char aResponseBuf[DIRTYCERT_MAXRESPONSE]; //!< buffer for http response
    char aCertBuf[DIRTYCERT_MAXCERTIFICATE]; //!< buffer for a single certificate
    char aCertDecodedBuf[DIRTYCERT_MAXCERTDECODED]; //!< buffer for a single certificate (decoded)
    uint8_t bPreload;               //!< TRUE if we are to execute preload, else FALSE

    uint32_t uTimeout;              //!< request timeout
    int32_t iRequestId;             //!< current on-going request (==index in request list)
    int32_t iCount;                 //!< count of valid requests
    DirtyCertCARequestT requests[DIRTYCERT_MAXREQUESTS]; //!< request list
};

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

//! module state ref
static DirtyCertRefT *_DirtyCert_pState = NULL;

//! dirtycert url
#if DIRTYCERT_URL_ENCRYPTED
 static char _DirtyCert_strRedirectorUrl[64];
#else
 #if 1
  static const char *_DirtyCert_strRedirectorUrl = DIRTYCERT_URL;
 #else // dev redirector, use only for testing, do NOT check in enabled
  #error DO NOT CHECK THIS IN ENABLED!!
  static const char *_DirtyCert_strRedirectorUrl = "http://gosredirector.online.ea.com:42125/redirector";
 #endif
#endif

// Public variables

/*** Private functions ******************************************************/


/*F********************************************************************************/
/*!
    \Function _DirtyCertUrlSetup

    \Description
        Decrypt encrypted DirtyCert URL; also used to generate encrypted URL

    \Output
        int32_t         - 0=success, negative=failure

    \Version 06/04/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _DirtyCertUrlSetup(void)
{
#if DIRTYCERT_URL_ENCRYPTED
    static const uint8_t strKey[] =
    {
        0x67,0x9A,0xE3,0x11, 0x6E,0x4C,0xFD,0xA2, 0x6D,0xC8,0x2D,0xF8, 0x5B,0x22,0xF1,0x40,
        0x40,0xE8,0x92,0x22, 0x8A,0x96,0x75,0x2B, 0x64,0x53,0xC6,0x8D, 0xED,0xA0,0xF9,0x21,
    };
    static const uint8_t strEnc[] =
    {
        0x25,0xEB,0x51,0x91, 0x6C,0x78,0xCB,0xF2, 0x2F,0xEA,0xE7,0x56, 0x37,0x96,0xE2,0x7C,
        0x49,0x8C,0x19,0x75, 0x59,0xBD,0xD8,0x3D, 0x39,0x81,0x91,0xE5, 0x9B,0x6C,0xE7,0x29,
        0x2B,0x72,0xB7,0x0A, 0x76,0x60,0x49,0x04, 0x90,0x7F,0xDB,0x54, 0xFD,0xC1,0x5E,0x2E,
        0xDB,0x37,0xF8,0x77, 0x21,0x06,0xDD,0x4E, 0xCD,0xCB,0xA3,0xF9, 0x87,0x74,0xDE,0x64,
    };
    char strUrlDecrypt[sizeof(_DirtyCert_strRedirectorUrl)];
    ds_memcpy_s(strUrlDecrypt, sizeof(strUrlDecrypt), strEnc, sizeof(strEnc));
    if (CryptArc4StringEncryptStatic(strUrlDecrypt, sizeof(strUrlDecrypt), strKey, sizeof(strKey), DIRTYCERT_URL) < 0)
    {
        return(-1);
    }
    // url has the port offset as a format paramter, so we fill it in here
    ds_snzprintf(_DirtyCert_strRedirectorUrl, sizeof(_DirtyCert_strRedirectorUrl), strUrlDecrypt, DIRTYCERT_PORT_OFFSET);
    return(0);
#else
    return(0);
#endif
}

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function _DirtyCertCAGetStrIdent

    \Description
        Debug-only function to get a string identifier of the request, for debug printing

    \Input *pState          - dirtycert state
    \Input *pRequest        - request to get string identifier for

    \Output
        const char *        - pointer to string ident

    \Version 04/18/2012 (jbrookes)
*/
/********************************************************************************F*/
static const char *_DirtyCertCAGetStrIdent(DirtyCertRefT *pState, DirtyCertCARequestT *pRequest)
{
    static char _strIdent[512];
    if (pRequest->eType == RT_ONDEMAND)
    {
        ds_snzprintf(_strIdent, sizeof(_strIdent), "[%s:%s:%d]", pRequest->CertInfo.Ident.strCommon, pRequest->CertInfo.Ident.strUnit, pRequest->CertInfo.iKeyModSize);
    }
    else
    {
        ds_snzprintf(_strIdent, sizeof(_strIdent), "[%s]", pState->strServiceName);
    }
    return(_strIdent);
}
#endif

/*F********************************************************************************/
/*!
    \Function _DirtyCertUrlEncodeStrParm

    \Description
        Wrapper for ProtoHttpUrlEncodeStrParm() that won't encode a null string.

    \Input *pBuffer         - output buffer to encode into
    \Input iLength          - length of output buffer
    \Input *pParm           - parameter name (not encoded)
    \Input *pData           - parameter data

    \Version 04/18/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyCertUrlEncodeStrParm(char *pBuffer, int32_t iLength, const char *pParm, const char *pData)
{
    if (*pData != '\0')
    {
        ProtoHttpUrlEncodeStrParm(pBuffer, iLength, pParm, pData);
    }
}

/*F********************************************************************************/
/*!
    \Function _DirtyCertCompareCertInfo

    \Description
        Compare two cert infos.

    \Input *pCertInfo1    - the cert info to be compared
    \Input *pCertInfo2    - the cert info to be compared

    \Output int32_t       - zero=match, non-zero=no-match

    \Version 01/23/2012 (szhu)
*/
/********************************************************************************F*/
static int32_t _DirtyCertCompareCertInfo(const ProtoSSLCertInfoT *pCertInfo1, const ProtoSSLCertInfoT *pCertInfo2)
{
    if ((pCertInfo1->iKeyModSize != pCertInfo2->iKeyModSize)
        || strcmp(pCertInfo1->Ident.strCountry, pCertInfo2->Ident.strCountry)
        || strcmp(pCertInfo1->Ident.strState, pCertInfo2->Ident.strState)
        || strcmp(pCertInfo1->Ident.strCity, pCertInfo2->Ident.strCity)
        || strcmp(pCertInfo1->Ident.strOrg, pCertInfo2->Ident.strOrg)
        || strcmp(pCertInfo1->Ident.strCommon, pCertInfo2->Ident.strCommon)
        || strcmp(pCertInfo1->Ident.strUnit, pCertInfo2->Ident.strUnit))
    {
        return(1);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _DirtyCertValidateServiceName

    \Description
        Validate that specified servicename is valid.

    \Input *pServiceName    - servicename to validate

    \Output
        int32_t             - negative=invalid, else valid

    \Version 03/15/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _DirtyCertValidateServiceName(const char *pServiceName)
{
    if (*pServiceName == '\0')
    {
        return(-1);
    }
    else
    {
        return(0);
    }
}

/*F********************************************************************************/
/*!
    \Function _DirtyCertSetServiceName

    \Description
        Format a request into provided buffer.

    \Input *pState      - module ref
    \Input *pName       - service name to set (game-year-platform or game)

    \Version 03/19/2013 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyCertSetServiceName(DirtyCertRefT *pState, const char *pName)
{
    char strServiceName[sizeof(pState->strServiceName)];
    if (!strchr(pName, '-'))
    {
        ds_snzprintf(strServiceName, sizeof(strServiceName), "%s-%d-%s", pName, 2000+DIRTYSDK_VERSION_YEAR, DIRTYCODE_PLATNAME_SHORT);
    }
    else
    {
        ds_strnzcpy(strServiceName, pName, sizeof(strServiceName));
    }
    if (strcmp(pState->strServiceName, strServiceName))
    {
        ds_strnzcpy(pState->strServiceName, strServiceName, sizeof(pState->strServiceName));
        NetPrintf(("dirtycert: servicename set to '%s'\n", pState->strServiceName));
    }
}

/*F********************************************************************************/
/*!
    \Function _DirtyCertFormatRequestUrl

    \Description
        Format a request into provided buffer.

    \Input *pState      - module ref
    \Input *pRequest    - CA request
    \Input *pBuf        - user-supplied url buffer
    \Input iBufLen      - size of buffer pointed to by pBuf

    \Version 01/23/2012 (szhu)
*/
/********************************************************************************F*/
static void _DirtyCertFormatRequestUrl(DirtyCertRefT *pState, DirtyCertCARequestT *pRequest, char *pBuf, int32_t iBufLen)
{
    // table of "safe" characters
    // 0=hex encode, non-zero=direct encode, @-O=valid hex digit (&15 to get value)
    static char _DirtyCert_strSafe[256] =
        "0000000000000000" "0000000000000000"   // 0x00 - 0x1f
        "0000000000000110" "@ABCDEFGHI000000"   // 0x20 - 0x3f (allow period, dash, and digits)
        "0JKLMNO111111111" "1111111111100000"   // 0x40 - 0x5f (allow uppercase)
        "0JKLMNO111111111" "1111111111100000"   // 0x60 - 0x7f (allow lowercase)
        "0000000000000000" "0000000000000000"   // 0x80 - 0x9f
        "0000000000000000" "0000000000000000"   // 0xa0 - 0xbf
        "0000000000000000" "0000000000000000"   // 0xc0 - 0xdf
        "0000000000000000" "0000000000000000";  // 0xe0 - 0xff}
    char strTemp[32];

    // create URL and attach DirtySDK version
    ds_snzprintf(pBuf, iBufLen, "%s/%s", _DirtyCert_strRedirectorUrl, (pRequest->eType == RT_ONDEMAND) ? "findCACertificates" : "getCACertificates");

    // append dirtysdk version
    ds_snzprintf(strTemp, sizeof(strTemp), "%d.%d.%d.%d.%d", DIRTYSDK_VERSION_YEAR, DIRTYSDK_VERSION_SEASON, DIRTYSDK_VERSION_MAJOR, DIRTYSDK_VERSION_MINOR, DIRTYSDK_VERSION_PATCH);
    ProtoHttpUrlEncodeStrParm2(pBuf, iBufLen, "?v=", strTemp, _DirtyCert_strSafe);

    // append dirtycert version
    ProtoHttpUrlEncodeIntParm(pBuf, iBufLen, "&vers=", DIRTYCERT_VERSION);

    // append servicename
    ProtoHttpUrlEncodeStrParm2(pBuf, iBufLen, "&name=", pState->strServiceName, _DirtyCert_strSafe);

    /* add additional on-demand parameters; for params name and format, refer to:
       blazeserver\dev3\component\redirector\gen\redirectortypes.tdf */
    if (pRequest->eType == RT_ONDEMAND)
    {
        // append required parameters
        ProtoHttpUrlEncodeStrParm2(pBuf, iBufLen, "&host=", pRequest->strHost, _DirtyCert_strSafe);
        ProtoHttpUrlEncodeIntParm(pBuf, iBufLen, "&port=", pRequest->iPort);
        ProtoHttpUrlEncodeIntParm(pBuf, iBufLen, "&bits=", pRequest->CertInfo.iKeyModSize * 8);
        // append parameters identifying required CA (empty fields are not included)
        _DirtyCertUrlEncodeStrParm(pBuf, iBufLen, "&entr|CN=", pRequest->CertInfo.Ident.strCommon);
        _DirtyCertUrlEncodeStrParm(pBuf, iBufLen, "&entr|C=", pRequest->CertInfo.Ident.strCountry);
        _DirtyCertUrlEncodeStrParm(pBuf, iBufLen, "&entr|O=", pRequest->CertInfo.Ident.strOrg);
        _DirtyCertUrlEncodeStrParm(pBuf, iBufLen, "&entr|OU=", pRequest->CertInfo.Ident.strUnit);
        _DirtyCertUrlEncodeStrParm(pBuf, iBufLen, "&entr|L=", pRequest->CertInfo.Ident.strCity);
        _DirtyCertUrlEncodeStrParm(pBuf, iBufLen, "&entr|ST=", pRequest->CertInfo.Ident.strState);
    }
}

/*F********************************************************************************/
/*!
    \Function _DirtyCertCreateRequest

    \Description
        Issue a CA fetch request.

    \Input *pState      - module ref
    \Input iRequestId   - CA request id
    \Input *pRequest    - CA request

    \Output int32_t     - 0 for success

    \Version 01/23/2012 (szhu)
*/
/********************************************************************************F*/
static int32_t _DirtyCertCreateRequest(DirtyCertRefT *pState, int32_t iRequestId, DirtyCertCARequestT *pRequest)
{
    // if we're not busy
    if (pState->iRequestId == DIRTYCERT_REQUESTID_NONE)
    {
        ds_memclr(pState->strUrl, sizeof(pState->strUrl));
        // format request url
        _DirtyCertFormatRequestUrl(pState, pRequest, pState->strUrl, sizeof(pState->strUrl));
        // set timeout
        ProtoHttpControl(pState->pHttp, 'time', pState->uTimeout, 0, NULL);
        // if pre-load set keep-alive
        if (pRequest->eType == RT_PREFETCH)
        {
            ProtoHttpControl(pState->pHttp, 'keep', 1, 0, NULL);
        }
        // http GET
        NetPrintf(("dirtycert: making CA request: %s\n", pState->strUrl));
        if (ProtoHttpGet(pState->pHttp, pState->strUrl, 0) >= 0)
        {
            pState->iRequestId = iRequestId;
            pRequest->eStatus = RS_IN_PROGRESS;
        }
        else
        {
            // failed?
            pRequest->eStatus = RS_FAILED;
            NetPrintf(("dirtycert: ProtoHttpGet(%p, %s) failed.\n", pState->pHttp, pState->strUrl));
        }
    }

    return(0);
}

/*F*************************************************************************************/
/*!
    \Function _DirtyCertCARequestFree

    \Description
        Release resources used by a CA fetch request.

    \Input *pState      - module state
    \Input *pRequest    - request ptr
    \Input iRequestId   - request id

    \Output
        int32_t         - negative=error, else success

    \Version 04/18/2012 (jbrookes)
*/
/************************************************************************************F*/
static int32_t _DirtyCertCARequestFree(DirtyCertRefT *pState, DirtyCertCARequestT *pRequest, int32_t iRequestId)
{
    int32_t iResult = 0;

    if (pRequest->iRefCount <= 0)
    {
        // error request id?
        iResult = -3;
    }
    else if (--pRequest->iRefCount == 0)
    {
        // if this is an active request, abort it
        if ((pState->iRequestId == iRequestId) && (pState->iRequestId != -1))
        {
            ProtoHttpAbort(pState->pHttp);
            pState->iRequestId = DIRTYCERT_REQUESTID_NONE;
        }
        NetPrintf(("dirtycert: freeing CA fetch request for %s\n", _DirtyCertCAGetStrIdent(pState, pRequest)));
        ds_memclr(pRequest, sizeof(*pRequest));
        pRequest->eStatus = RS_NONE;
        // if no queued requests, close connection
        if (--pState->iCount == 0)
        {
            ProtoHttpControl(pState->pHttp, 'disc', 0, 0, NULL);
        }
        // success
        iResult = 1;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _DirtyCertProcessResponse

    \Description
        Process response.

    \Input *pState      - module ref
    \Input *pResponse   - http response
    \Input iLen         - length of response
    \Input *pFailed     - [out] storage for number of CA certificate loads that failed

    \Output int32_t     - positive=installed certs, 0 or negative=error

    \Version 01/24/2012 (szhu)
*/
/********************************************************************************F*/
static int32_t _DirtyCertProcessResponse(DirtyCertRefT *pState, const char *pResponse, int32_t iLen, int32_t *pFailed)
{
    int32_t iCount = 0, iCertLen, iResult;
    const char *pCert;

    *pFailed = 0;

    // for response format, refer to: blazeserver\dev3\component\redirector\gen\redirectortypes.tdf
    pCert = XmlFind(pResponse, "cacertificate.certificatelist.certificatelist");
    while (pCert)
    {
        ds_memclr(pState->aCertBuf, sizeof(pState->aCertBuf));
        if ((iCertLen = XmlContentGetString(pCert, pState->aCertBuf, sizeof(pState->aCertBuf), "")) > 0)
        {
            // the cert response might be base64 encoded
            char sEncoding[32];
            ds_memclr(sEncoding, sizeof(sEncoding));
            XmlAttribGetString(pCert, "enc", sEncoding, sizeof(sEncoding), "");
            // base64 encoded, decode and install the cert
            if (ds_stricmp(sEncoding, "base64") == 0)
            {
                ds_memclr(pState->aCertDecodedBuf, sizeof(pState->aCertDecodedBuf));
                if (Base64Decode(iCertLen, pState->aCertBuf, pState->aCertDecodedBuf))
                {
                    if ((iResult = ProtoSSLSetCACert((uint8_t *)pState->aCertDecodedBuf, (int32_t)strlen((const char*)pState->aCertDecodedBuf))) >= 0)
                    {
                        iCount += iResult;
                    }
                    else
                    {
                        *pFailed += 1;
                    }
                }
            }
            else
            {
                // plain, install the cert
                if ((iResult = ProtoSSLSetCACert((uint8_t *)pState->aCertBuf, iCertLen)) > 0)
                {
                    iCount += iResult;
                }
                else
                {
                    *pFailed += 1;
                }
            }
        }
        pCert = XmlNext(pCert);
    }
    return(iCount);
}

/*F********************************************************************************/
/*!
    \Function _DirtyCertUpdate

    \Description
        Update status of DirtyCert module.

    \Input *pData   - pointer to DirtyCert module ref

    \Notes
        This is be the only private 'static' function that needs to acquire crit
        because it's actually called from NetIdle thread.

    \Version 01/23/2012 (szhu)
*/
/********************************************************************************F*/
static void _DirtyCertUpdate(void *pData)
{
    DirtyCertRefT *pState = (DirtyCertRefT*)pData;
    DirtyCertCARequestT *pRequest;

    /* attempt to acquire critical section. there is dependency between this crit
       and the idle crit, we want to prevent a deadlock in those conditions */
    if (!NetCritTry(&pState->crit))
    {
        return;
    }

    if (pState->iRequestId != DIRTYCERT_REQUESTID_NONE)
    {
        pRequest = &pState->requests[pState->iRequestId];

        if (pRequest->eStatus == RS_IN_PROGRESS)
        {
            int32_t iHttpStatus;
            // update http
            ProtoHttpUpdate(pState->pHttp);
            // if we've done with current http request
            if ((iHttpStatus = ProtoHttpStatus(pState->pHttp, 'done', NULL, 0)) > 0)
            {
                // check http response code, we only deal with "20X OK"
                if ((ProtoHttpStatus(pState->pHttp, 'code', NULL, 0) / 100) == 2)
                {
                    int32_t iRecvLen, iFailed;
                    // zero buf
                    ds_memclr(pState->aResponseBuf, sizeof(pState->aResponseBuf));
                    if ((iRecvLen = ProtoHttpRecvAll(pState->pHttp, (char*)pState->aResponseBuf, sizeof(pState->aResponseBuf))) > 0)
                    {
                        // install received certs
                        iRecvLen = _DirtyCertProcessResponse(pState, pState->aResponseBuf, iRecvLen, &iFailed);
                        pRequest->eStatus = RS_DONE;
                        pState->iRequestId = DIRTYCERT_REQUESTID_NONE;
                        NetPrintf(("dirtycert: %d cert(s) installed for %s (%d failed)\n", iRecvLen,
                            _DirtyCertCAGetStrIdent(pState, pRequest), iFailed));
                    }
                }

                // unknown error? (ex. http 500 error)
                if (pRequest->eStatus != RS_DONE)
                {
                    iHttpStatus = -1;
                    NetPrintf(("dirtycert: CA fetch request failed (httpcode=%d)\n", ProtoHttpStatus(pState->pHttp, 'code', NULL, 0)));
                }
            }
            else if (iHttpStatus == 0)
            {
                // if we're missing CA cert for our http ref, we fail.
                if (ProtoHttpStatus(pState->pHttp, 'cfip', NULL, 0) > 0)
                {
                    // mark request as failed but don't reset pState->iRequestId otherwise a new request may be issued
                    pRequest->eStatus = RS_FAILED;
                    NetPrintf(("dirtycert: CA fetch request failed because of no CA available for redirector: %s\n", _DirtyCert_strRedirectorUrl));
                }
            }

            // if we failed (or timed-out)
            if (iHttpStatus < 0)
            {
                pRequest->eStatus = RS_FAILED;
                pState->iRequestId = DIRTYCERT_REQUESTID_NONE;
                NetPrintf(("dirtycert: CA fetch request for %s %s\n", _DirtyCertCAGetStrIdent(pState, pRequest),
                           ProtoHttpStatus(pState->pHttp, 'time', NULL, 0) ? "timeout" : "failed"));
            }

            // prefetch requests are executed only once, and automatically clean up after themselves upon completion
            if ((pRequest->eType == RT_PREFETCH) && (iHttpStatus != 0))
            {
                pState->bPreload = FALSE;
                _DirtyCertCARequestFree(pState, pRequest, pState->iRequestId);
            }
        }
    }

    // if we need to issue a new request?
    if ((pState->iRequestId == DIRTYCERT_REQUESTID_NONE) && (pState->iCount > 0))
    {
        int32_t iRequestId;
        for (iRequestId = 0; iRequestId < DIRTYCERT_MAXREQUESTS; iRequestId++)
        {
            pRequest = &pState->requests[iRequestId];
            if ((pRequest->iRefCount > 0) && (pRequest->eStatus == RS_NOT_STARTED))
            {
                if (_DirtyCertCreateRequest(pState, iRequestId, pRequest) == 0)
                {
                    break;
                }
            }
        }
    }

    // release critical section
    NetCritLeave(&pState->crit);
}


/*** Public functions ********************************************************/


/*F*************************************************************************************************/
/*!
    \Function DirtyCertCreate

    \Description
        Startup DirtyCert module.

    \Output int32_t     - 0 for success, negative for error

    \Version 01/23/2012 (szhu)
*/
/*************************************************************************************************F*/
int32_t DirtyCertCreate(void)
{
    DirtyCertRefT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    if (_DirtyCert_pState != NULL)
    {
        NetPrintf(("dirtycert: DirtyCertCreate() called while module is already active\n"));
        return(-1);
    }

    // decrypt url
    if (_DirtyCertUrlSetup() < 0)
    {
        NetPrintf(("dirtycert: error setting up url; dirtycert startup failure\n"));
        return(-2);
    }

    // query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pState = DirtyMemAlloc(sizeof(*pState), DIRTYCERT_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtycert: could not allocate module state\n"));
        return(-3);
    }
    ds_memclr(pState, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->bPreload = TRUE;

    // create http ref
    if ((pState->pHttp = ProtoHttpCreate(DIRTYCERT_MAXRESPONSE)) == NULL)
    {
        DirtyMemFree(pState, DIRTYCERT_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(-4);
    }
    // no request
    pState->iRequestId = DIRTYCERT_REQUESTID_NONE;

    // set request timeout
    pState->uTimeout = DIRTYCERT_TIMEOUT;

    // init crit
    NetCritInit(&pState->crit, "DirtyCert");

    // add update function to netidle handler
    NetIdleAdd(_DirtyCertUpdate, pState);

    // save module ref
    _DirtyCert_pState = pState;

    NetPrintf(("dirtycert: created dirtycert 0x%p (pHttp=%p)\n", pState, pState->pHttp));
    // return module state
    return(0);
}

/*F************************************************************************/
/*!
    \Function DirtyCertDestroy

    \Description
        Destroy the module and release its state.

    \Output int32_t     - 0 for success, negative for error

    \Version 01/23/2012 (szhu)
*/
/*************************************************************************F*/
int32_t DirtyCertDestroy(void)
{
    DirtyCertRefT *pState = _DirtyCert_pState;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtycert: DirtyCertDestroy() called while module is not active\n"));
        return(-1);
    }

    // remove idle handler
    NetIdleDel(_DirtyCertUpdate, pState);

    // clear global module ref
    _DirtyCert_pState = NULL;

    // destroy http ref
    ProtoHttpDestroy(pState->pHttp);

    // destroy crit
    NetCritKill(&pState->crit);

    // free the memory
    DirtyMemFree(pState, DIRTYCERT_MEMID, pState->iMemGroup, pState->pMemGroupUserData);

    NetPrintf(("dirtycert: freed dirtycert %p\n", pState));
    return(0);
}

/*F********************************************************************************/
/*!
    \Function DirtyCertCARequestCert

    \Description
        Initialize a CA fetch request.

    \Input *pCertInfo   - CA Cert Info
    \Input *pHost       - host we are fetching CA for
    \Input iPort        - port we are fetching CA for

    \Output
        int32_t         - positive=id of request, negative=error

    \Version 01/23/2012 (szhu)
*/
/********************************************************************************F*/
int32_t DirtyCertCARequestCert(const ProtoSSLCertInfoT *pCertInfo, const char *pHost, int32_t iPort)
{
    DirtyCertRefT *pState = _DirtyCert_pState;
    int32_t i, iSlot = -1;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtycert: DirtyCertCARequestCert() called while module is not active\n"));
        return(-1);
    }

    // acquire critical section
    NetCritEnter(&pState->crit);

    // require a valid service name
    if (_DirtyCertValidateServiceName(pState->strServiceName) < 0)
    {
        NetCritLeave(&pState->crit);
        return(-2);
    }

    // if we haven't successfully executed pre-load yet, slot in a pre-load request first
    if (pState->bPreload)
    {
        DirtyCertCAPreloadCerts(pState->strServiceName);
    }

    // check if it matches any in-progress request
    for (i = 0; i < DIRTYCERT_MAXREQUESTS; i++)
    {
        // if this is an empty slot
        if (pState->requests[i].iRefCount <= 0)
        {
            // mark it
            if (iSlot < 0)
            {
                iSlot = i;
            }
            continue;
        }

        // we found an in-progress request with the same cert info
        if (_DirtyCertCompareCertInfo(pCertInfo, &pState->requests[i].CertInfo) == 0)
        {
            iSlot = i;
            break;
        }
    }

    if (iSlot >= 0)
    {
        DirtyCertCARequestT *pRequest = &pState->requests[iSlot];
        // a CA request for this cert is already queued
        if (pRequest->iRefCount > 0)
        {
            // inc refcount
            pRequest->iRefCount++;
            NetPrintf(("dirtycert: CA fetch request %s already queued (status=%d), updated refcount to %d.\n",
                _DirtyCertCAGetStrIdent(pState, pRequest), (int32_t)pRequest->eStatus, pRequest->iRefCount));
        }
        // no matching request, create a new one
        else
        {
            ds_memclr(pRequest, sizeof(*pRequest));
            // set request type and status
            pRequest->eType = RT_ONDEMAND;
            pRequest->eStatus = RS_NOT_STARTED;
            // save host info
            ds_strnzcpy(pRequest->strHost, pHost, sizeof(pRequest->strHost));
            pRequest->iPort = iPort;
            // save cert info
            ds_memcpy_s(&pRequest->CertInfo, sizeof(pRequest->CertInfo), pCertInfo, sizeof(*pCertInfo));
            // set refcount
            pRequest->iRefCount = 1;
            pState->iCount++;
            NetPrintf(("dirtycert: queued CA fetch request for %s\n", _DirtyCertCAGetStrIdent(pState, pRequest)));
            _DirtyCertCreateRequest(pState, iSlot, pRequest);
        }
        // public slot reference is +1 to reserve zero
        iSlot += 1;
    }
    else
    {
        NetPrintf(("dirtycert: too many CA fetch requests.\n"));
    }

    // release critical section
    NetCritLeave(&pState->crit);

    // return slot ref to caller
    return(iSlot);
}

/*F********************************************************************************/
/*!
    \Function DirtyCertCAPreloadCerts

    \Description
        Initialize a CA preload request

    \Input *pServiceName    - servicename to identify CA preload set ("gamename-gameyear-platform")

    \Notes
        Unlike the explicit CA load, a preload request is fire and forget, and
        will clean up after itself when complete.

    \Version 04/18/2012 (jbrookes)
*/
/********************************************************************************F*/
void DirtyCertCAPreloadCerts(const char *pServiceName)
{
    DirtyCertRefT *pState = _DirtyCert_pState;
    int32_t i, iSlot = -1;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtycert: DirtyCertCARequestCert() called while module is not active\n"));
        return;
    }

    // require a valid servicename
    if (_DirtyCertValidateServiceName(pServiceName) < 0)
    {
        return;
    }

    // acquire critical section
    NetCritEnter(&pState->crit);

    // if there is already a prefetch queued, ignore this request
    for (i = 0; i < DIRTYCERT_MAXREQUESTS; i++)
    {
        if ((pState->requests[i].iRefCount > 0) && (pState->requests[i].eType == RT_PREFETCH))
        {
            NetPrintf(("dirtycert: CA prefetch request already queued; request ignored\n"));
            NetCritLeave(&pState->crit);
            return;
        }
    }

    // save service name (required for all request types)
    _DirtyCertSetServiceName(pState, pServiceName);

    // find an empty slot
    for (i = 0; i < DIRTYCERT_MAXREQUESTS; i++)
    {
        // if this is an empty slot
        if (pState->requests[i].iRefCount <= 0)
        {
            // mark it
            if (iSlot < 0)
            {
                iSlot = i;
                break;
            }
        }
    }

    if (iSlot >= 0)
    {
        DirtyCertCARequestT *pRequest = &pState->requests[iSlot];

        ds_memclr(pRequest, sizeof(*pRequest));
        // set request type and status
        pRequest->eType = RT_PREFETCH;
        pRequest->eStatus = RS_NOT_STARTED;
        // set refcount
        pRequest->iRefCount = 1;
        pState->iCount++;

        NetPrintf(("dirtycert: queued CA prefetch request for %s\n", _DirtyCertCAGetStrIdent(pState, pRequest)));
        _DirtyCertCreateRequest(pState, iSlot, pRequest);
    }
    else
    {
        NetPrintf(("dirtycert: too many CA fetch requests.\n"));
    }

    // release critical section
    NetCritLeave(&pState->crit);
}

/*F*************************************************************************************/
/*!
    \Function    DirtyCertCARequestDone

    \Description
        Determine if a CA fetch request is complete.

    \Input iRequestId   - id of CA request

    \Output
        int32_t         - zero=in progess, neg=done w/error, pos=done w/success

    \Version 01/23/2012 (szhu)
*/
/************************************************************************************F*/
int32_t DirtyCertCARequestDone(int32_t iRequestId)
{
    DirtyCertRefT *pState = _DirtyCert_pState;
    DirtyCertCARequestT *pRequest;
    int32_t iResult = 0;

    // internal request id is public id - 1
    iRequestId -= 1;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtycert: DirtyCertCARequestDone() called while module is not active\n"));
        return(-1);
    }
    // if it's a valid request id
    if ((iRequestId < 0) || (iRequestId >= DIRTYCERT_MAXREQUESTS))
    {
        NetPrintf(("dirtycert: invalid request id (%d)\n", iRequestId));
        return(-2);
    }

    // acquire critical section
    NetCritEnter(&pState->crit);

    pRequest = &pState->requests[iRequestId];
    if (pRequest->iRefCount <= 0)
    {
        // error request id?
        iResult = -3;
    }
    else if (pRequest->eStatus == RS_FAILED)
    {
        iResult = -4;
    }
    else if (pRequest->eStatus == RS_DONE)
    {
        iResult = 1;
    }

    // release critical section
    NetCritLeave(&pState->crit);

    return(iResult);
}

/*F*************************************************************************************/
/*!
    \Function    DirtyCertCARequestFree

    \Description
        Release resources used by a CA fetch request.

    \Input iRequestId    - id of CA request

    \Output int32_t      - negative=error, else success

    \Version 01/23/2012 (szhu)
*/
/************************************************************************************F*/
int32_t DirtyCertCARequestFree(int32_t iRequestId)
{
    DirtyCertRefT *pState = _DirtyCert_pState;
    int32_t iResult;

    // internal request id is public id - 1
    iRequestId -= 1;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtycert: DirtyCertCARequestFree() called while module is not active\n"));
        return(-1);
    }
    // if it's a valid request id
    if ((iRequestId < 0) || (iRequestId >= DIRTYCERT_MAXREQUESTS))
    {
        NetPrintf(("dirtycert: invalid request id (%d)\n", iRequestId));
        return(-2);
    }

    // acquire critical section
    NetCritEnter(&pState->crit);

    // free the request
    iResult = _DirtyCertCARequestFree(pState, &pState->requests[iRequestId], iRequestId);

    // release critical section
    NetCritLeave(&pState->crit);

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function DirtyCertControl

    \Description
        Set module behavior based on input selector.

    \Input  iControl    - input selector
    \Input  iValue      - selector input
    \Input  iValue2     - selector input
    \Input *pValue      - selector input

    \Output
        int32_t         - selector result

    \Notes
        iControl can be one of the following:

        \verbatim
            SELECTOR    DESCRIPTION
            'prld'      Enables/disables preload
            'snam'      Sets service name to use for on-demand requests
            'time'      Sets CA request timeout in milliseconds (default=30s)
        \endverbatim

    \Version 01/24/2012 (szhu)
*/
/********************************************************************************F*/
int32_t DirtyCertControl(int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    DirtyCertRefT *pState = _DirtyCert_pState;
    int32_t iResult = -100;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtycert: DirtyCertControl() called while module is not active\n"));
        return(-1);
    }

    // acquire critical section
    NetCritEnter(&pState->crit);

    // set preload enable/disable
    if (iControl == 'prld')
    {
        NetPrintf(("dirtycert: %s CA preload\n", (iValue != 0) ? "enabling" : "disabling"));
        pState->bPreload = (iValue != 0) ? TRUE : FALSE;
        iResult = 0;
    }
    // set service name
    if (iControl == 'snam')
    {
        _DirtyCertSetServiceName(pState, (const char *)pValue);
        iResult = 0;
    }
    // set timeout
    if (iControl == 'time')
    {
        NetPrintf(("dirtycert: setting timeout to %d ms\n", iValue));
        pState->uTimeout = (unsigned)iValue;
        iResult = 0;
    }

    // release critical section
    NetCritLeave(&pState->crit);

    // unhandled?
    if (iResult == -100)
    {
        NetPrintf(("dirtycert: unhandled control option '%C'\n", iControl));
        iResult = -1;
    }

    // return control result to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function DirtyCertStatus

    \Description
        Get module status

    \Input iStatus  - info selector (see notes)
    \Input *pBuffer - [out] storage for selector-specific output
    \Input iBufSize - size of output buffer

    \Output
        int32_t     - selector result

    \Notes
        iStatus can be one of the following:

        \verbatim
            SELECTOR    DESCRIPTION
            'snam'      Stores service name in output buffer (if not NULL); returns validity
        \endverbatim

    \Version 03/25/2013 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyCertStatus(int32_t iStatus, void *pBuffer, int32_t iBufSize)
{
    DirtyCertRefT *pState = _DirtyCert_pState;
    int32_t iResult = -100;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtycert: DirtyCertStatus() called while module is not active\n"));
        return(-1);
    }

    // acquire critical section
    NetCritEnter(&pState->crit);

    // get service name, return whether it is valid or not
    if (iStatus == 'snam')
    {
        if (pBuffer != NULL)
        {
            ds_strnzcpy(pBuffer, pState->strServiceName, iBufSize);
        }
        iResult = _DirtyCertValidateServiceName(pState->strServiceName);
    }

    // release critical section
    NetCritLeave(&pState->crit);

    // unhandled?
    if (iResult == -100)
    {
        NetPrintf(("dirtycert: unhandled status option '%C'\n", iStatus));
        iResult = -1;
    }

    // return control result to caller
    return(iResult);
}

