/*H********************************************************************************/
/*!
    \File weblog.c

    \Description
        Captures DirtySDK debug output and posts it to a webserver where the output
        can be retrieved.  This is useful when debugging on a system with no
        debugging capability or in a "clean room" environment, for example.  Two
        basic mechanisms are employed; the first is a NetPrint debug hook to capture
        all debug output, the second a stand-alone WebOfferPrintf() function that
        can be used more selectively.

    \Notes
        A small local buffer is required to store the output before it is submitted
        to ProtoHttp for sending.  This is to avoid reentrancy issues where debug
        output from ProtoHttp or lower-level modules (ProtoSSL, Crypt*, etc) would
        put us into an infinite recursion.  Instead the text is simply buffered and
        flushed at regular intervals by the WebLogUpdate() function.

    \Copyright
        Copyright (c) 2008 Electronic Arts Inc.

    \Version 05/06/2008 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/misc/weblog.h"
#include "DirtySDK/proto/protohttp.h"


/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

struct WebLogRefT
{
    // module memory group
    int32_t iMemGroup;          //!< module mem group id
    void *pMemGroupUserData;    //!< user data associated with mem group
    
    ProtoHttpRefT *pProtoHttp;  //!< http module ref
    NetCritT WebCrit;           //!< critical section to guard weblog buffer
    int32_t iBufLen;            //!< string buffer length
    char strWebHost[128];       //!< webhost to post to
    char strWebUrl[128];        //!< weburl for post

    uint8_t bLogging;           //!< TRUE if logging is enabled, else FALSE
    uint8_t bPosting;           //!< TRUE if in posting state, else FALSE
    uint8_t bUpdating;          //!< TRUE if in WebLogUpdate(), else FALSE
    uint8_t _pad;
    
    char strText[1];            //!< variable-length string buffer (must come last!)
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _WebLogFlush

    \Description
        Flush weblog text to ProtoHttp.

    \Input *pWebLog     - module state
        
    \Output
        None.

    \Version 05/06/2008 (jbrookes)
*/
/********************************************************************************F*/
static void _WebLogFlush(WebLogRefT *pWebLog)
{
    int32_t iSendSize = (int32_t)strlen(pWebLog->strText), iSentSize;

    // if nothing to do, don't send
    if (iSendSize == 0)
    {
        return;
    }

    // if logging is enabled but we aren't posting yet, start posting
    if ((pWebLog->bLogging == TRUE) && (pWebLog->bPosting == FALSE))
    {
        char strUrl[256];
        // format url
        ds_snzprintf(strUrl, sizeof(strUrl), "http://%s%s", pWebLog->strWebHost, pWebLog->strWebUrl);
        // make client timeout very long (one hour)
        ProtoHttpControl(pWebLog->pProtoHttp, 'time', 60*60*1000, 0, NULL);
        //$$ hack -- set keepalive to 2 so we don't get a Connection: Close header
        ProtoHttpControl(pWebLog->pProtoHttp, 'keep', 2, 0, NULL);
        // start the post request
        ProtoHttpPost(pWebLog->pProtoHttp, strUrl, NULL, -1, FALSE);
        pWebLog->bPosting = TRUE;
    }

    // send the data
    iSentSize = ProtoHttpSend(pWebLog->pProtoHttp, pWebLog->strText, iSendSize);
    
    // remove sent data from buffer
    if (iSentSize > 0)
    {
        if (iSentSize == iSendSize)
        {
            // clear the buffer
            pWebLog->strText[0] = '\0';
            
            // if we aren't logging, end the transaction
            if (pWebLog->bLogging == FALSE)
            {
                NetPrintf(("weblog: ending streaming transmission\n"));
                ProtoHttpSend(pWebLog->pProtoHttp, NULL, 0);
            }
        }
        else
        {
            // contract buffer (include null character)
            memmove(pWebLog->strText, pWebLog->strText + iSentSize, iSendSize - iSentSize + 1);
        }
    }
    else if (iSentSize < 0)
    {
        NetPrintf(("weblog: error %d trying to send; resetting state to try a new post operation\n", iSentSize));
        pWebLog->bPosting = FALSE;
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function WebLogCreate

    \Description
        Create the WebLog module.

    \Input iBufSize     - local text buffer size
    
    \Output
        WebLogRefT *    - pointer to module state, or NULL

    \Version 05/06/2008 (jbrookes)
*/
/********************************************************************************F*/
WebLogRefT *WebLogCreate(int32_t iBufSize)
{
    WebLogRefT *pWebLog;
    int32_t iModuleSize;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    
    // enforce minimum buffer size
    if (iBufSize < 4096)
    {
        iBufSize = 4096;
    }
    
    // allocate and init module state
    iModuleSize = sizeof(*pWebLog) + iBufSize - 1;
    if ((pWebLog = DirtyMemAlloc(iModuleSize, WEBLOG_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("weblog: could not allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pWebLog, iModuleSize);
    pWebLog->iMemGroup = iMemGroup;
    pWebLog->pMemGroupUserData = pMemGroupUserData;
    pWebLog->iBufLen = iBufSize;
    
    /* Set default webhost -- easo.stest.ea.com is the VIP address for the EAWS stest web cluster
       and should be accessible from any environment.  Port 8080 on the VIP is configured to forward
       directly to port 8001, which hits Tomcat directly (and bypasses Apache).  This is required
       because the EAWS version of Apache has a bug when proxying a chunked upload that causes the
       chunked encoding to be stripped but a Content-Length: not applied to the forwarded upload */
    ds_strnzcpy(pWebLog->strWebHost, "easo.stest.ea.com:8080", sizeof(pWebLog->strWebHost));
    #if 0
    // stesteasoweb01 bypasses the VIP but is an internal address
    ds_strnzcpy(pWebLog->strWebHost, "stesteasoweb01.pt.abn-iad.ea.com:8001", sizeof(pWebLog->strWebHost));
    // eggplant is the dev EAWS server
    ds_strnzcpy(pWebLog->strWebHost, "eggplant.online.ea.com", sizeof(pWebLog->strWebHost));
    // eggplant:8001 bypasses Apache and goes directly to Tomcat
    ds_strnzcpy(pWebLog->strWebHost, "eggplant.online.ea.com:8001", sizeof(pWebLog->strWebHost));
    #endif
    
    // set default URL
    ds_strnzcpy(pWebLog->strWebUrl, "/tool/easo/logrequest.jsp", sizeof(pWebLog->strWebUrl));

    // init critical section
    NetCritInit(&pWebLog->WebCrit, "WebLog");

    // create ProtoHttp ref
    if ((pWebLog->pProtoHttp = ProtoHttpCreate(4*1024)) == NULL)
    {
        NetPrintf(("weblog: could not allocate http ref\n"));
        WebLogDestroy(pWebLog);
        return(NULL);
    }
    
    // return module state to caller
    return(pWebLog);
}

/*F********************************************************************************/
/*!
    \Function WebLogConfigure

    \Description
        Configure weblog parameters

    \Input *pWebLog - weblog module state
    \Input *pServer - server to post log to (NULL to retain current value)
    \Input *pUrl    - url to post log to (NULL to retain current value)

    \Output
        None.    

    \Version 05/14/2008 (jbrookes)
*/
/********************************************************************************F*/
void WebLogConfigure(WebLogRefT *pWebLog, const char *pServer, const char *pUrl)
{
    if (pServer != NULL)
    {
        ds_strnzcpy(pWebLog->strWebHost, pServer, sizeof(pWebLog->strWebHost));
    }
    if (pUrl != NULL)
    {
        ds_strnzcpy(pWebLog->strWebUrl, pUrl, sizeof(pWebLog->strWebUrl));
    }
}

/*F********************************************************************************/
/*!
    \Function WebLogStart

    \Description
        Starts logging

    \Input *pWebLog - weblog module state

    \Output
        None.    

    \Version 05/06/2008 (jbrookes)
*/
/********************************************************************************F*/
void WebLogStart(WebLogRefT *pWebLog)
{
    // see if we are already started
    if (pWebLog->bLogging)
    {
        NetPrintf(("weblog: start called when already started\n"));
        return;
    }
    
    // start the logging
    pWebLog->bLogging = TRUE;
    NetPrintf(("weblog: starting log operation\n"));
}

/*F********************************************************************************/
/*!
    \Function WebLogStop

    \Description
        Stop logging and close close the current WebLog transaction (if any)

    \Input *pWebLog - weblog module state

    \Output
        None.    

    \Version 05/06/2008 (jbrookes)
*/
/********************************************************************************F*/
void WebLogStop(WebLogRefT *pWebLog)
{
    // see if we are already stopped
    if (!pWebLog->bLogging)
    {
        NetPrintf(("weblog: stop called when already stopped\n"));
        return;
    }
    
    // stop the logging
    NetPrintf(("weblog: stopping log operation\n"));
    pWebLog->bLogging = FALSE;
}

/*F********************************************************************************/
/*!
    \Function WebLogDestroy

    \Description
        Destroy the WebLog module.

    \Input *pWebLog - pointer to weblog module to destroy

    \Output
        None.    

    \Version 05/06/2008 (jbrookes)
*/
/********************************************************************************F*/
void WebLogDestroy(WebLogRefT *pWebLog)
{
    NetCritKill(&pWebLog->WebCrit);
    if (pWebLog->pProtoHttp != NULL)
    {
        ProtoHttpDestroy(pWebLog->pProtoHttp);
    }
    DirtyMemFree(pWebLog, WEBLOG_MEMID, pWebLog->iMemGroup, pWebLog->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function WebLogDebugHook

    \Description
        WebLog NetPrintf debug hook.

    \Input *pUserData   - user data
    \Input *pText       - debug text

    \Output
        int32_t         - zero to suppress debug output, else do not suppress 

    \Version 05/06/2008 (jbrookes)
*/
/********************************************************************************F*/
int32_t WebLogDebugHook(void *pUserData, const char *pText)
{
    WebLogRefT *pWebLog = (WebLogRefT *)pUserData;
    WebLogPrintf(pWebLog, "%s", pText);
    return(1);
}

/*F********************************************************************************/
/*!
    \Function WebLogPrintf

    \Description
        Print into the weblog buffer.

    \Input *pWebLog - weblog module state
    \Input *pFormat - format string
    \Input ...      - variable argument listing

    \Output
        None.    

    \Version 05/06/2008 (jbrookes)
*/
/********************************************************************************F*/
void WebLogPrintf(WebLogRefT *pWebLog, const char *pFormat, ...)
{
    static char strText[4096];
    va_list pFmtArgs;

    // ensure serial access to text buffer and weblog buffer
    NetCritEnter(&pWebLog->WebCrit);
    
    // format text
    va_start(pFmtArgs, pFormat);
    ds_vsnzprintf(strText, sizeof(strText), pFormat, pFmtArgs);
    va_end(pFmtArgs);
    
    // queue the text for sending if we are logging
    if (pWebLog->bLogging && !pWebLog->bUpdating)
    {
        ds_strnzcat(pWebLog->strText, strText, pWebLog->iBufLen);
    }

    // release mutex
    NetCritLeave(&pWebLog->WebCrit);
}

/*F********************************************************************************/
/*!
    \Function WebLogControl

    \Description
        Control weblog behavior

    \Input *pWebLog - weblog module state
    \Input iSelect  - control selector
    \Input iValue   - selector specific
    \Input iValue2  - selector specific
    \Input *pValue  - selector specific

    \Output
        int32_t     - selector specific
        
    \Notes
        Unhandled selectors are passed through to ProtoHttpControl()

    \Version 05/13/2008 (jbrookes)
*/
/********************************************************************************F*/
int32_t WebLogControl(WebLogRefT *pWebLog, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue)
{
    return(ProtoHttpControl(pWebLog->pProtoHttp, iSelect, iValue, iValue2, pValue));
}

/*F********************************************************************************/
/*!
    \Function WebLogUpdate

    \Description
        Update the WebLog module

    \Input *pWebLog - weblog module state

    \Output
        None.    

    \Version 05/06/2008 (jbrookes)
*/
/********************************************************************************F*/
void WebLogUpdate(WebLogRefT *pWebLog)
{
    NetCritEnter(&pWebLog->WebCrit);
    // remember we are updating so weblog doesn't log itself
    pWebLog->bUpdating = TRUE;
    // flush data to protohttp
    _WebLogFlush(pWebLog);
    // update ProtoHttp
    ProtoHttpUpdate(pWebLog->pProtoHttp);
    // no longer updating
    pWebLog->bUpdating = FALSE;
    NetCritLeave(&pWebLog->WebCrit);
}
