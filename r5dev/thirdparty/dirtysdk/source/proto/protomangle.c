/*H*************************************************************************************************/
/*!
    \File    protomangle.c

    \Description
        This module encapsulates client services for use of the EA.Com Demangler service.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2003.  ALL RIGHTS RESERVED.

    \Version    1.0        04/03/03 (JLB) First Version
*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtycert.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/proto/protomangle.h"

/*** Defines ***************************************************************************/

#define PROTOMANGLE_VERBOSE         (DIRTYCODE_DEBUG && TRUE)

#define PROTOMANGLE_RAND_PORTBASE   (2000)

//! range used for random port generation (used to be 8000 but that proved to conflict with Sony port range restriction on PS4)
#define PROTOMANGLE_RAND_PORTRANGE  (6000)

#define PROTOMANGLE_HTTP_BUFSIZE    (1024)

#define PROTOMANGLE_HTTP_TIMEOUT    (60 * 1000)

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef enum ProtoMangleResponseE
{
    PROTOMANGLE_RESP_INVALID = -1,
    PROTOMANGLE_RESP_SUCCESS,
    PROTOMANGLE_RESP_PROBE,
    PROTOMANGLE_RESP_FAILURE,

    PROTOMANGLE_NUMRESP
} ProtoMangleResponseE;

//! parsed probe request data
typedef struct ProtoMangleProbeT
{
    int32_t  iCurCnt;           //!< current probe iteration
    int32_t  iSndCnt;           //!< number of probes to send
    int32_t  iAddr;             //!< peer address
    int32_t  iPort;             //!< peer port
    int32_t  iSrcPort;          //!< local port used to bind (if -1, bind a random port)
    int32_t  iId;               //!< transaction ID
    char strTag[64];        //!< response tag
} ProtoMangleProbeT;

//! internal module state
struct ProtoMangleRefT      //!< module state
{
    ProtoHttpRefT *pHttp;   //!< http module
    
    SocketT *pProbeSock;     //!< socket for sending UDP probes
    SocketT *pSharedProbeSock; //!< shared socket, if any

    //! module memory group
    int32_t iMemGroup;
    void *pMemGroupUserData;

    int32_t iHostAddr;          //!< local address
    int32_t iGamePort;          //!< game port

    int32_t iPeerAddr;          //!< peer address (when eMangleState==ST_MNGL_DONE)
    int32_t iPeerPort;          //!< peer port to connect to (when eMangleState==ST_MNGL_DONE)

    char strCookie[PROTOMANGLE_STRCOOKIE_MAX];  //!< session cookie
    char strGameID[PROTOMANGLE_STRGAMEID_MAX];  //!< game feature ID
    char strLKey[PROTOMANGLE_STRLKEY_MAX];      //!< user LKey
    char strServer[PROTOMANGLE_STRSERVER_MAX];  //!< server name
    int32_t iServerPort;        //!< server port

    ProtoMangleResponseE eResponse;  //!< most recent server response

    int32_t iRandPort;          //!< 'random' port to bind if no local port requested

    enum
    {
        ST_MNGL_IDLE,       //!< created
        ST_MNGL_WAIT,       //!< waiting for response from HTTP server
        ST_MNGL_DONE,       //!< successful completion
        ST_MNGL_FAIL,       //!< failure condition
        ST_MNGL_REPT        //!< reporting
    } eMangleState;
    
    int32_t iRecvSize;          //!< current receive size
    char strBuf[PROTOMANGLE_HTTP_BUFSIZE];
};

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

//! response codes
static const char *_ProtoMangle_pResponseTable[PROTOMANGLE_NUMRESP] =
{
    "success",              // PROTOMANGLE_RESP_SUCCESS
    "probe",                // PROTOMANGLE_RESP_PROBE
    "failure"               // PROTOMANGLE_RESP_FAILURE
};

//! report codes
static const char *_ProtoMangle_pReportTable[PROTOMANGLE_NUMSTATUS] =
{
    "connected",            // PROTOMANGLE_STATUS_CONNECTED
    "failed"                // PROTOMANGLE_STATUS_FAILED
};

// Public variables


/*** Private Functions *****************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    _ProtoMangleSockClose

    \Description
        Safe socket close of a probe socket

    \Input *pRef    - module state
    \Input bRelease - whether shared probe socket should be release or not

    \Version 01/14/05 (JLB)
*/
/*************************************************************************************************F*/
static void _ProtoMangleSockClose(ProtoMangleRefT *pRef, uint32_t bRelease)
{
    if (pRef->pProbeSock != NULL)
    {
        if (pRef->pProbeSock != pRef->pSharedProbeSock)
        {
            NetPrintf(("protomangle: [%p] closing probe socket %p\n", pRef, pRef->pProbeSock));
            SocketShutdown(pRef->pProbeSock, SOCK_NOSEND);
            SocketClose(pRef->pProbeSock);
            pRef->pProbeSock = NULL;
        }
        else if (bRelease)
        {
            NetPrintf(("protomangle: [%p] releasing shared probe socket %p\n", pRef, pRef->pProbeSock));
            SocketRelease(pRef->pProbeSock);
            pRef->pProbeSock = pRef->pSharedProbeSock = NULL;
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _ProtoMangleHttpPost

    \Description
        Send a POST request to HTTP server.

    \Input *pRef    - module state
    \Input *pUrl    - url to post to
    \Input *pCookie - session cookie
    \Input *pData   - POST data

    \Output
        int32_t         - zero=success, negative=failure

    \Version 07/18/03 (JLB)
*/
/*************************************************************************************************F*/
static int32_t _ProtoMangleHttpPost(ProtoMangleRefT *pRef, const char *pUrl, const char *pCookie, const char *pData)
{
    char strBuffer[256];

    // setup the extra header fields
    ds_snzprintf(strBuffer, sizeof(strBuffer),
        "Cookie: sessionID=%s\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n",
        pCookie);

    // set extra header fields
    ProtoHttpControl(pRef->pHttp, 'apnd', 0, 0, strBuffer);

    // setup the url
    ds_snzprintf(strBuffer, sizeof(strBuffer), "http://%s:%d/%s", pRef->strServer, pRef->iServerPort, pUrl);

    // execute the post
    return(ProtoHttpPost(pRef->pHttp, strBuffer, pData, (int32_t)strlen(pData), FALSE));
}

/*F*************************************************************************************************/
/*!
    \Function    _ProtoMangleHttpRequest

    \Description
        Issue HTTP request to the demangler server.

    \Input *pRef    - module state

    \Version    1.0        04/08/03 (JLB) First Version
*/
/*************************************************************************************************F*/
static void _ProtoMangleHttpRequest(ProtoMangleRefT *pRef)
{
    char strAppend[256], strUrl[256 + (3 * DIRTYCERT_SERVICENAME_SIZE)], strAddrText[20], strServiceName[DIRTYCERT_SERVICENAME_SIZE];
    int32_t iFormatted = 0;

    if (DirtyCertStatus('snam', strServiceName, DIRTYCERT_SERVICENAME_SIZE) < 0)
    {
        ds_snzprintf(strServiceName, sizeof(strServiceName), "invalid");
    }

    // setup the url
    iFormatted = ds_snzprintf(strUrl, sizeof(strUrl), "http://%s:%d/getPeerAddress?myIP=%s&myPort=%d&version=1.0",
        pRef->strServer, pRef->iServerPort, SocketInAddrGetText(pRef->iHostAddr, strAddrText, sizeof(strAddrText)), pRef->iGamePort);

    // add service name
    ProtoHttpUrlEncodeStrParm(strUrl + iFormatted, sizeof(strUrl) - iFormatted, "&gameID=", strServiceName);

    // setup the extra header fields
    ds_snzprintf(strAppend, sizeof(strAppend), "Cookie: sessionID=%s\r\n", pRef->strCookie);

    // set extra header fields
    ProtoHttpControl(pRef->pHttp, 'apnd', 0, 0, strAppend);

    // send the request
    ProtoHttpGet(pRef->pHttp, strUrl, FALSE);

    // set state
    pRef->iRecvSize = 0;
    pRef->eMangleState = ST_MNGL_WAIT;
}

/*F*************************************************************************************************/
/*!
    \Function    _ProtoMangleGetParm
    
    \Description
        Parse a parameter from a demangler server response.
    
    \Input *pDst        - pointer to buffer for parsed parm
    \Input iBufLen      - length of destination buffer
    \Input *pSrc        - pointer to server reseponse
    \Input *pParmName   - name of parameter to parse
    \Input bParseCount  - if TRUE, parse response count
    
    \Output
        int32_t             - response count (one if bParseCount==FALSE), or negative if parmname not found
            
    \Version    1.0        04/08/03 (JLB) First Version
*/
/*************************************************************************************************F*/
static int32_t _ProtoMangleGetParm(char *pDst, int32_t iBufLen, const char *pSrc, const char *pParmName, int32_t bParseCount)
{
    const char *pParm;
    int32_t iStr, iId = -1;

    // clear the destination buffer
    pDst[0] = '\0';

    // make sure we have room for terminating NULL character
    iBufLen--;

    // look for parm
    if ((pParm = strstr(pSrc, pParmName)) != NULL)
    {
        // parse count?
        if (bParseCount == TRUE)
        {
            char *pId;

            if ((pId = strchr(pParm, '-')) != NULL)
            {
                iId = pId[1] - '0';
                if ((iId < 0) || (iId > 9))
                {
                    iId = -1;
                }
            }
        }
        else
        {
            iId = TRUE;
        }

        // look for value
        if ((pParm = strchr(pParm, '=')) != NULL)
        {
            // found string; make a copy of it
            for (iStr = 0, pParm++; (iStr < iBufLen) && (pParm[iStr] != '\r') && (pParm[iStr] != '\n'); iStr++)
            {
                pDst[iStr] = pParm[iStr];
            }

            // NULL terminate
            pDst[iStr++] = '\0';
        }
    }

    return(iId);
}

/*F*************************************************************************************************/
/*!
    \Function    _ProtoMangleParseAdvance

    \Description
        Advance past current parm block

    \Input *pSrc        - pointer to current block
    \Input *pTerminator - termination character

    \Output
        const char*     - pointer to next block, or NULL if there is none

    \Notes
        Parm blocks are terminated by double LFs.

    \Version    1.0        04/09/03 (JLB) First Version
*/
/*************************************************************************************************F*/
static const char *_ProtoMangleParseAdvance(const char *pSrc, const char *pTerminator)
{
    if ((pSrc = strstr(pSrc, pTerminator)) != NULL)
    {
        while(*pSrc == '\n')
        {
            pSrc++;
        }
    }

    return(pSrc);
}

/*F*************************************************************************************************/
/*!
    \Function    _ProtoMangleParseResponseType
    
    \Description
        Determine type of demangler server response.
    
    \Input *pSrc        - module state
    \Input *pResponse   - storage for parsed response type
    
    \Output
        const char *    - pointer to next entry
            
    \Version    1.0        04/08/03 (JLB) First Version
*/
/*************************************************************************************************F*/
static const char *_ProtoMangleParseResponseType(const char *pSrc, ProtoMangleResponseE *pResponse)
{
    char strCmd[32];
    int32_t  iResp;

    *pResponse = PROTOMANGLE_RESP_INVALID;
    if (_ProtoMangleGetParm(strCmd, sizeof(strCmd), pSrc, "status", FALSE))
    {
        for (iResp = 0; iResp < PROTOMANGLE_NUMRESP; iResp++)
        {
            if (!strncmp(strCmd, _ProtoMangle_pResponseTable[iResp], strlen(_ProtoMangle_pResponseTable[iResp])))
            {
                *pResponse = (ProtoMangleResponseE)iResp;
                break;
            }
        }
    }

    return(_ProtoMangleParseAdvance(pSrc, "\n"));
}

/*F*************************************************************************************************/
/*!
    \Function    _ProtoMangleParseSuccess
    
    \Description
        Parse demangler server 'success' response.
    
    \Input *pRef    - module state
    \Input *pStr    - server response string
    
    \Output
        int32_t         - TRUE if successfully parsed, else FALSE
            
    \Version    1.0        04/09/03 (JLB) First Version
*/
/*************************************************************************************************F*/
static int32_t _ProtoMangleParseSuccess(ProtoMangleRefT *pRef, const char *pStr)
{
    char strParm[32];
    int32_t  bLinesParsed = 0;

    if ((_ProtoMangleGetParm(strParm, sizeof(strParm), pStr, "peerIP", FALSE)) >= 0)
    {
        if ((pRef->iPeerAddr = SocketInTextGetAddr(strParm)) != 0)
        {
            bLinesParsed++;
        }
    }

    if ((_ProtoMangleGetParm(strParm, sizeof(strParm), pStr, "peerPort", FALSE)) >= 0)
    {
        pRef->iPeerPort = atoi(strParm);
        bLinesParsed++;
    }

    return(bLinesParsed == 2);
}

/*F*************************************************************************************************/
/*!
    \Function    _ProtoMangleParseProbeRequest
    
    \Description
        Parse a 'probe' demangler server request.
    
    \Input *pProbe  - pointer to probe structure to fill in
    \Input *pRef    - module ref
    \Input *pSrc    - server response to parse
    
    \Output
        int32_t         - TRUE if response parsed, else FALSE
            
    \Version    1.0        04/08/03 (JLB) First Version
*/
/*************************************************************************************************F*/
static int32_t _ProtoMangleParseProbeRequest(ProtoMangleProbeT *pProbe, ProtoMangleRefT *pRef, const char *pSrc)
{
    int32_t bLinesParsed = 0, iParm;
    char strParm[64];

    if (pSrc == NULL)
    {
        return(FALSE);
    }

    // init
    pProbe->iCurCnt = 0;
    pProbe->iSrcPort = -1;
    pProbe->iId = -1;

    // parse target address
    if ((pProbe->iId = _ProtoMangleGetParm(strParm, sizeof(strParm), pSrc, "targetIP", TRUE)) >= 0)
    {
        if ((pProbe->iAddr =  SocketInTextGetAddr(strParm)) != 0)
        {
            bLinesParsed++;
        }
    }

    if (pProbe->iId < 0)
    {
        return(FALSE);
    }

    // parse target port
    if (_ProtoMangleGetParm(strParm, sizeof(strParm), pSrc, "targetPort", TRUE) == pProbe->iId)
    {
        if ((pProbe->iPort = atoi(strParm)) > 0)
        {
            bLinesParsed++;
        }
    }

    // parse request tag
    if (_ProtoMangleGetParm(strParm, sizeof(strParm), pSrc, "tag", TRUE) == pProbe->iId)
    {
        ds_strnzcpy(pProbe->strTag, strParm, sizeof(pProbe->strTag));
        bLinesParsed++;
    }

    // parse send count
    if (_ProtoMangleGetParm(strParm, sizeof(strParm), pSrc, "sendCount", TRUE) == pProbe->iId)
    {
        if (((iParm = atoi(strParm)) > 1) && (iParm < 32))
        {
            pProbe->iSndCnt = iParm;
            bLinesParsed++;
        }
    }

    // parse source port (optional)
    if (_ProtoMangleGetParm(strParm, sizeof(strParm), pSrc, "sourcePort", TRUE) == pProbe->iId)
    {
        if ((iParm = atoi(strParm)) > 0)
        {
            #ifdef DIRTYCODE_PS4
            if (iParm > 32767) 
            {
                int32_t iParmOriginal = iParm;
                iParm &= 36863;
                if (iParmOriginal != iParm)
                {
                    NetPrintf(("protomangle: using port %d instead of %d.\n", iParm, iParmOriginal));
                }
            }
            #endif
            pProbe->iSrcPort = iParm;
        }
    }

    return(bLinesParsed == 4);
}

/*F*************************************************************************************************/
/*!
    \Function    _ProtoMangleSetupProbeSocket
    
    \Description
        Bind probe socket.

    \Input *pRef    - module state
    \Input *pProbe  - probe parameters
    
    \Output
        int32_t         - local port bound to socket, or negative on error
            
    \Version    1.0        04/09/03 (JLB) First Version
*/
/*************************************************************************************************F*/
static int32_t _ProtoMangleSetupProbeSocket(ProtoMangleRefT *pRef, ProtoMangleProbeT *pProbe)
{
    struct sockaddr BindAddr;

    // tear down previous socket if we're not using shared socket
    _ProtoMangleSockClose(pRef, FALSE);

    // if we have a shared socket and this is a request for the shared socket port, set it and return
    if ((pRef->pSharedProbeSock != NULL) && (pProbe->iSrcPort == pRef->iGamePort))
    {
        NetPrintf(("protomangle: [%p] using shared socket ref %p to probe port %d\n", pRef, pRef->pSharedProbeSock, pRef->iGamePort));
        pRef->pProbeSock = pRef->pSharedProbeSock;
    }
    else
    {
        // recreate probe socket
        if ((pRef->pProbeSock = SocketOpen(AF_INET, SOCK_DGRAM, 0)) == NULL)
        {
            NetPrintf(("protomangle: [%p] error creating probe socket\n", pRef));
            return(-1);
        }

        // set up local binding
        SockaddrInit(&BindAddr, AF_INET);
        if (pProbe->iSrcPort != -1)
        {
            SockaddrInSetPort(&BindAddr, pProbe->iSrcPort);
        }
        else
        {
            SockaddrInSetPort(&BindAddr, pRef->iRandPort++);
        }

        // bind locally
        if (SocketBind(pRef->pProbeSock, &BindAddr, sizeof(BindAddr)) == SOCKERR_NONE)
        {
            NetPrintf(("protomangle: [%p] created probe socket %p bound to port %d\n", pRef, pRef->pProbeSock, SockaddrInGetPort(&BindAddr)));
        }
        else
        {
            NetPrintf(("protomangle: [%p] error binding probe socket %p to port %d\n", pRef, pRef->pProbeSock, SockaddrInGetPort(&BindAddr)));
            // tear down socket since binding failed
            _ProtoMangleSockClose(pRef, FALSE);
            return(-1);
        }
    }

    // return local port to caller
    SocketInfo(pRef->pProbeSock, 'bind', 0, &BindAddr, sizeof(BindAddr));
    return(SockaddrInGetPort(&BindAddr));
}

/*F*************************************************************************************************/
/*!
    \Function    _ProtoMangleSendProbe

    \Description
        Send UDP probe packet(s) to demangler server.

    \Input *pRef        - module state
    \Input *pProbe      - probe parameters
    \Input iSrcPort     - source port
    \Input iProbeReq    - probe request count from current request list

    \Version 04/08/03 (JLB)
*/
/*************************************************************************************************F*/
static void _ProtoMangleSendProbe(ProtoMangleRefT *pRef, ProtoMangleProbeT *pProbe, int32_t iSrcPort, int32_t iProbeReq)
{
    struct sockaddr PeerAddr;
    char strMesg[128], strAddrText[20];
    int32_t iErr, iMsgSize;

    // set up target
    SockaddrInit(&PeerAddr, AF_INET);
    SockaddrInSetAddr(&PeerAddr, pProbe->iAddr);
    SockaddrInSetPort(&PeerAddr, pProbe->iPort);

    // format probe package
    ds_snzprintf(strMesg, sizeof(strMesg), "sourceIP=%s\r\nsourcePort=%d\r\ntag=%s\r\nsendCount=%d\r\n",
        SocketInAddrGetText(pRef->iHostAddr, strAddrText, sizeof(strAddrText)), iSrcPort, pProbe->strTag, pProbe->iCurCnt);
    iMsgSize = (int32_t)strlen(strMesg)+1;

    // send probe
    iErr = SocketSendto(pRef->pProbeSock, strMesg, iMsgSize, 0, &PeerAddr, sizeof(PeerAddr));

    #if PROTOMANGLE_VERBOSE
    if (iErr == iMsgSize)
    {
        NetPrintf(("protomangle: [%p] success ", pRef));
    }
    else
    {
        NetPrintf(("protomangle: [%p] error %d ", pRef, iErr));
    }
    NetPrintf(("sending probe %d/%d (%d) from port %d\n", pProbe->iCurCnt, pProbe->iSndCnt, iProbeReq, iSrcPort));
    #else
    (void)iErr;
    #endif
}

/*F*************************************************************************************************/
/*!
    \Function    _ProtoMangleHandleProbeRequest
    
    \Description
        Parse a probe request block, and issue any probes requested.
    
    \Input *pRef    - module state
    \Input *pSrc    - probe request string
    
    \Output
        int32_t         - zero on success, negative on failure
            
    \Version    1.0        04/08/03 (JLB) First Version
*/
/*************************************************************************************************F*/
static int32_t _ProtoMangleHandleProbeRequest(ProtoMangleRefT *pRef, const char *pSrc)
{
    ProtoMangleProbeT Probe;
    int32_t iProbeReq, iSrcPort;

    // parse probe request list, 
    for (iProbeReq = 1; _ProtoMangleParseProbeRequest(&Probe, pRef, pSrc) != FALSE; iProbeReq++)
    {
        // make sure sequence IDs match
        if (iProbeReq != Probe.iId)
        {
            NetPrintf(("protomangle: [%p] warning, probe sequence mismatch\n", pRef));
        }

        // init probe socket
        if ((iSrcPort = _ProtoMangleSetupProbeSocket(pRef, &Probe)) < 0)
        {
            return(-1);
        }

        // send iSndCnt probes
        for (Probe.iCurCnt = 0; Probe.iCurCnt < Probe.iSndCnt; Probe.iCurCnt++)
        {
            _ProtoMangleSendProbe(pRef, &Probe, iSrcPort, iProbeReq);
        }

        // advance to next response block
        pSrc = _ProtoMangleParseAdvance(pSrc,"\n\n");
    }

    return(0);
}


/*** Public Functions ******************************************************************/



/*F*************************************************************************************************/
/*!
    \Function    ProtoMangleCreate
    
    \Description
        Allocate module state and prepare for use
    
    \Input *pServer         - pointer to server name
    \Input iPort            - server port
    \Input *pGameID         - pointer to game feature ID string
    \Input *pLKey           - pointer to user LKey
    
    \Output
        ProtoMangleRefT *   - reference pointer (must be passed to all other functions)
            
    \Version    1.0        04/03/03 (JLB) First Version
    \Version    1.1        06/16/03 (JLB) Added server parms as arguments
*/
/*************************************************************************************************F*/
ProtoMangleRefT *ProtoMangleCreate(const char *pServer, int32_t iPort, const char *pGameID, const char *pLKey)
{
    ProtoMangleRefT *pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    NetPrintf(("protomangle: server:%s port:%d gameid:%s lkey:%s\n", pServer, iPort, pGameID, pLKey));

    // allocate and init module state
    if ((pRef = DirtyMemAlloc(sizeof(*pRef), PROTOMANGLE_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protomangle: could not allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pRef, sizeof(*pRef));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;

    // create http module
    if ((pRef->pHttp = ProtoHttpCreate(PROTOMANGLE_HTTP_BUFSIZE)) == NULL)
    {
        DirtyMemFree(pRef, PROTOMANGLE_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        return(NULL);
    }

    // since we know we are going to have an ongoing transaction, enable keep-alive from the beginning
    ProtoHttpControl(pRef->pHttp, 'keep', 1, 0, NULL);

    // set default http timeout
    ProtoHttpControl(pRef->pHttp, 'time', PROTOMANGLE_HTTP_TIMEOUT, 0, NULL);

    // generate 'random' port in range
    pRef->iRandPort = (NetTick() % PROTOMANGLE_RAND_PORTRANGE) + PROTOMANGLE_RAND_PORTBASE;

    // save server parms, gameID, LKey
    ds_strnzcpy(pRef->strServer, pServer, sizeof(pRef->strServer));
    pRef->iServerPort = iPort;
    ds_strnzcpy(pRef->strGameID, pGameID, sizeof(pRef->strGameID));
    ds_strnzcpy(pRef->strLKey, pLKey, sizeof(pRef->strLKey));

    // return ref to caller
    return(pRef);
}

/*F*************************************************************************************************/
/*!
    \Function    ProtoMangleDestroy

    \Description
        Destroy the module and release its state

    \Input *pRef    - reference pointer

    \Version    1.0        04/03/03 (JLB) First Version
*/
/*************************************************************************************************F*/
void ProtoMangleDestroy(ProtoMangleRefT *pRef)
{
    NetPrintf(("protomangle: [%p] shutting down\n", pRef));

    // close probe socket, if we own it, else release it
    _ProtoMangleSockClose(pRef, TRUE);

    // shut down http module
    ProtoHttpDestroy(pRef->pHttp);

    // release ref
    DirtyMemFree(pRef, PROTOMANGLE_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
}

/*F*************************************************************************************************/
/*!
    \Function    ProtoMangleConnect2

    \Description
        Initiate transaction with demangler service

    \Input *pRef        - module state
    \Input iMyPort      - game port
    \Input *pSessID     - pointer to unique session ID (must be identical between clients)
    \Input iSessIDLen   - lenght of SessID string

    \Version 01/16/13 mclouatre
*/
/*************************************************************************************************F*/
void ProtoMangleConnect2(ProtoMangleRefT *pRef, int32_t iMyPort, const char *pSessID, int32_t iSessIDLen)
{
    // save port/addr
    pRef->iHostAddr = SocketGetLocalAddr();
    pRef->iGamePort = iMyPort;

    // save cookie
    ds_strnzcpy(pRef->strCookie, pSessID, sizeof(pRef->strCookie));
    
    /* close previous http socket to prevent http connections spanning
       sessions, which doesn't work with the demangler load balancer */
    ProtoHttpControl(pRef->pHttp, 'disc', 0, 0, NULL);

    // fire off initial request
    _ProtoMangleHttpRequest(pRef);
}

/*F*************************************************************************************************/
/*!
    \Function    ProtoMangleConnect

    \Description
        Initiate transaction with demangler service

    \Input *pRef    - module state
    \Input iMyPort  - game port
    \Input *pSessID - pointer to unique session ID (must be identical between clients)

    \Version 04/07/03 jbrookes
*/
/*************************************************************************************************F*/
void ProtoMangleConnect(ProtoMangleRefT *pRef, int32_t iMyPort, const char *pSessID)
{
    ProtoMangleConnect2(pRef, iMyPort, pSessID, (int32_t)strlen(pSessID));
}

/*F*************************************************************************************************/
/*!
    \Function    ProtoMangleConnectSocket

    \Description
        Initiate transaction with demangler service, using the given shared socket ref for
        UDP probes that are to be sent out over the game port.

    \Input *pRef    - module state
    \Input uSockRef - socket reference to use for UDP probes sent over game port
    \Input *pSessID - pointer to unique session ID (must be identical between clients)

    \Version    1.0        08/24/04 (JLB) First Version
*/
/*************************************************************************************************F*/
void ProtoMangleConnectSocket(ProtoMangleRefT *pRef, intptr_t uSockRef, const char *pSessID)
{
    struct sockaddr SockAddr;

    // import socket
    pRef->pSharedProbeSock = SocketImport(uSockRef);
    if (pRef->pSharedProbeSock == NULL)
    {
        NetPrintf(("protomangle: [%p] unable to import sockref 0x%08x\n", pRef, uSockRef));
        return;
    }

    // save addr and socket ref
    pRef->iHostAddr = SocketGetLocalAddr();

    // look up and save bind port
    SocketInfo(pRef->pSharedProbeSock, 'bind', 0, &SockAddr, sizeof(SockAddr));
    pRef->iGamePort = SockaddrInGetPort(&SockAddr);

    NetPrintf(("protomangle: [%p] created shared socket using platform socket ref %p bound to port %d\n", pRef, pRef->pSharedProbeSock, pRef->iGamePort));

    // save cookie
    ds_strnzcpy(pRef->strCookie, pSessID, sizeof(pRef->strCookie));

    /* close previous http socket to prevent http connections spanning
       sessions, which doesn't work with the demangler load balancer */
    ProtoHttpControl(pRef->pHttp, 'disc', 0, 0, NULL);

    // fire off initial reques
    _ProtoMangleHttpRequest(pRef);
}

/*F*************************************************************************************************/
/*!
    \Function    ProtoMangleUpdate

    \Description
        Update module state

    \Input *pRef    - module reference

    \Version 04/07/03 (JLB)
*/
/*************************************************************************************************F*/
void ProtoMangleUpdate(ProtoMangleRefT *pRef)
{
    // handle updates to connect process
    if (pRef->eMangleState == ST_MNGL_WAIT)
    {
        int32_t iRecvLen;

        // update HTTP request
        ProtoHttpUpdate(pRef->pHttp);

        // poll for completion
        if ((iRecvLen = ProtoHttpRecvAll(pRef->pHttp, pRef->strBuf, sizeof(pRef->strBuf))) >= 0)
        {
            const char *pStr;

            // get data
            NetPrintf(("protomangle: [%p] server response\n", pRef));
            NetPrintWrap(pRef->strBuf, 80);

            // parse response type
            pStr = _ProtoMangleParseResponseType(pRef->strBuf, &pRef->eResponse);
            switch(pRef->eResponse)
            {
                case PROTOMANGLE_RESP_SUCCESS:
                    // parse success result
                    pRef->eMangleState = _ProtoMangleParseSuccess(pRef, pStr) ? ST_MNGL_DONE : ST_MNGL_FAIL;
                    break;
                case PROTOMANGLE_RESP_PROBE:
                    // parse and issue probes
                    if (_ProtoMangleHandleProbeRequest(pRef, pStr) >= 0)
                    {
                        // request server update
                        _ProtoMangleHttpRequest(pRef);
                        break;
                    }
                    // intentional fall-through in failure condition
                case PROTOMANGLE_RESP_FAILURE:
                case PROTOMANGLE_RESP_INVALID:
                default:
                    pRef->eMangleState = ST_MNGL_FAIL;
                    break;
            }
        }
        else if ((iRecvLen < 0) && (iRecvLen != PROTOHTTP_RECVWAIT))
        {
            pRef->eMangleState = ST_MNGL_FAIL;
        }
    }

    // handle response to report
    if (pRef->eMangleState == ST_MNGL_REPT)
    {
        // update HTTP request
        ProtoHttpUpdate(pRef->pHttp);

        // done?        
        if (ProtoHttpStatus(pRef->pHttp, 'done', NULL, 0) != 0)
        {
            pRef->eMangleState = (ProtoHttpStatus(pRef->pHttp, 'code', NULL, 0) == 200) ? ST_MNGL_DONE : ST_MNGL_FAIL;
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    ProtoMangleComplete
    
    \Description
        Returns TRUE and fills in address and port if Demangler has finished operation successfully.
    
    \Input *pRef    - module state
    \Input *pAddr   - storage for address (result==TRUE)
    \Input *pPort   - storage for port (result==TRUE)
    
    \Output
        int32_t         - TRUE if complete, else FALSE, or <0 to indicate an error
            
    \Version    1.0        04/09/03 (JLB) First Version
*/
/*************************************************************************************************F*/
int32_t ProtoMangleComplete(ProtoMangleRefT *pRef, int32_t *pAddr, int32_t *pPort)
{
    if (pRef->eMangleState == ST_MNGL_DONE)
    {
        // close probe socket
        _ProtoMangleSockClose(pRef, TRUE);

        // return address and port to caller
        if ((pAddr != NULL) && (pPort != NULL))
        {
            *pAddr = pRef->iPeerAddr;
            *pPort = pRef->iPeerPort;
        }

        // reset to IDLE state
        pRef->eMangleState = ST_MNGL_IDLE;
        return(TRUE);
    }
    else if ((pRef->eMangleState == ST_MNGL_WAIT) || (pRef->eMangleState == ST_MNGL_REPT))
    {
        return(FALSE);
    }
    else
    {
        pRef->eMangleState = ST_MNGL_IDLE;
        return(-1);
    }
}

/*F*************************************************************************************************/
/*!
    \Function    ProtoMangleReport
    
    \Description
        Post result report to server.
    
    \Input *pRef    - mangle ref
    \Input eStatus  - connection status
    \Input iLatency - peer latency (optional, -1 to not send)
    
    \Output
        int32_t         - zero=success, negative=failure

    \Notes
        Report is of the form:

            POST http://demangler.ea.com:3658/connectionStatus
            Cookie: sessionID=6ba7b810-9dad-11d1-80b4-00c04fd430c8 
            myIP=10.14.221.1&myPort=3250&version=1.0&status=connected&gameFeatureID=madden-ps2-2004&latency=80
            
    \Version    1.0        07/18/03 (JLB) First Version
*/
/*************************************************************************************************F*/
int32_t ProtoMangleReport(ProtoMangleRefT *pRef, ProtoMangleStatusE eStatus, int32_t iLatency)
{
    char strData[128 + (3 * DIRTYCERT_SERVICENAME_SIZE)], strAddrText[20], strServiceName[DIRTYCERT_SERVICENAME_SIZE];
    int32_t iFormatted = 0;

    // make sure status is valid
    if (eStatus > PROTOMANGLE_STATUS_FAILED)
    {
        return(-1);
    }

    if (DirtyCertStatus('snam', strServiceName, DIRTYCERT_SERVICENAME_SIZE) < 0)
    {
        ds_snzprintf(strServiceName, sizeof(strServiceName), "invalid");
    }

    // format response to demangler server
    iFormatted = ds_snzprintf(strData, sizeof(strData), "myIP=%s&myPort=%d&version=1.0&status=%s&gameFeatureID=%s\n",
        SocketInAddrGetText(pRef->iHostAddr, strAddrText, sizeof(strAddrText)), pRef->iPeerPort,
        _ProtoMangle_pReportTable[eStatus], pRef->strGameID);

    // add service name
    ProtoHttpUrlEncodeStrParm(strData + iFormatted, sizeof(strData) - iFormatted, "&gameID=", strServiceName);

    // add latency info, if included
    if (iLatency >= 0)
    {
        char strData2[32];
        ds_snzprintf(strData2, sizeof(strData2), "&latency=%d", iLatency);
        ds_strnzcat(strData, strData2, sizeof(strData));
    }

    // post result to demangler server
    _ProtoMangleHttpPost(pRef, "connectionStatus", pRef->strCookie, strData);

    // set state & return success
    pRef->eMangleState = ST_MNGL_REPT;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    ProtoMangleControl

    \Description
        ProtoMangle control function.

    \Input *pRef    - mangle ref
    \Input iControl - control selector
    \Input iValue   - selector specific
    \Input iValue2  - selector specific
    \Input *pValue  - selector specific

    \Version 05/12/05 (JLB)
*/
/*************************************************************************************************F*/
int32_t ProtoMangleControl(ProtoMangleRefT *pRef, int32_t iControl, int32_t iValue, int32_t iValue2, const void *pValue)
{
    if (iControl == 'abrt')
    {
        // close probe socket
        _ProtoMangleSockClose(pRef, TRUE);

        // reset to IDLE state
        pRef->eMangleState = ST_MNGL_IDLE;
        return(0);
    }
    if (iControl == 'time')
    {
        ProtoHttpControl(pRef->pHttp, 'time', iValue, 0, NULL);
        return(0);
    }
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function    ProtoMangleStatus

    \Description
        Return module status based on input selector.

    \Input *pRef    - mangle ref
    \Input iSelect  - status selector
    \Input *pBuf    - [out] storage for output data
    \Input iBufSize - size of output buffer

    \Version 1.0 01/13/05 (JLB)
*/
/*************************************************************************************************F*/
int32_t ProtoMangleStatus(ProtoMangleRefT *pRef, int32_t iSelect, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'idle')
    {
        return(pRef->eMangleState == ST_MNGL_IDLE);
    }
    if (iSelect == 'time')
    {
        return(ProtoHttpStatus(pRef->pHttp, 'time', NULL, 0));
    }
    // unhandled
    return(-1);
}

