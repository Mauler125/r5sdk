/*H********************************************************************************/
/*!
    \File protostream.c

    \Description
        Manage streaming of an Internet media resource.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 11/16/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/proto/protohttp.h"

#include "DirtySDK/proto/protostream.h"

/*** Defines **********************************************************************/

#define PROTOSTREAM_MINBUFFER       (32*1024)   //!< minimum buffer configuration
#define PROTOSTREAM_MAXURL          (256)       //!< maximum url length
#define PROTOSTREAM_RESTARTFREQ_MAX (60)        //!< maximum restart increase is one minute
#define PROTOSTREAM_SAMPLE_PERIOD   (2000)      //!< stats sampling period

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

// module state memory
struct ProtoStreamRefT
{
    ProtoHttpRefT *pProtoHttp;          //!< protohttp ref

    // module memory group
    int32_t iMemGroup;                  //!< module mem group id
    void *pMemGroupUserData;            //!< user data associated with mem group

    ProtoStreamCallbackT *pCallback;    //!< user callback
    void *pUserData;                    //!< user callback data
    int32_t iCallbackRate;              //!< callback rate in ms
    uint32_t uLastCallback;             //!< last time callback was triggered

    enum
    {
        ST_IDLE,                        //!< idle state
        ST_OPEN                         //!< stream is active
    } eState;

    int32_t iRestartFreq;               //!< restart frequency (PROTOSTREAM_FREQ_* or restart time in seconds)
    int32_t iRestartTime;               //!< restart timer
    int32_t iRestartIncr;               //!< amount to increase restart frequency by in case of an error
    int32_t iRestartThreshold;          //!< minimum amount of data that must be buffered before playback starts
    int32_t iTimeout;                   //!< current http timeout

    int32_t iLastSize;                  //!< previous stream size
    int32_t iLastModified;              //!< previous last mod time
    int32_t iLastHttpCode;              //!< most recent http result code, saved on stream completion
    int32_t iLastRecvTime;              //!< time data was last received

    int32_t iBufSize;                   //!< streaming buffer size
    int32_t iBufLen;                    //!< amount of data in buffer
    int32_t iBufInp;                    //!< buffer input position
    int32_t iBufOut;                    //!< buffer output position
    int32_t iBufMin;                    //!< min amount of data to provide to caller; -1 if no min

    int32_t iBufAvg;                    //!< measured buffer size average
    int32_t iBufDev;                    //!< measured buffer size deviation

    int32_t iStreamRead;                //!< total amount of data read from stream
    int32_t iStreamTime;                //!< time stream has been open for, in milliseconds

    #if DIRTYCODE_DEBUG
    uint32_t uStarveTime;               //!< time to starve input until, for testing (debug only)
    #endif

    uint8_t *pBufMin;                   //!< buffer to handle min-sized reads

    uint8_t bPrebuffering;              //!< pre-buffering data before giving it to app
    uint8_t bReceivedHeader;            //!< TRUE=received HTTP header, else FALSE
    uint8_t bPaused;                    //!< TRUE if stream is paused, else FALSE
    int8_t  iVerbose;                   //!< verbose debug level (debug only)
    int8_t  iStreamStatus;              //!< stream status: -1=error, 0=in progress, 1=complete

    char    strUrl[PROTOSTREAM_MAXURL]; //!< current url

    char    strError[256];              //!< buffer to store HTTP result text, if any
    uint8_t aBuffer[1];                 //!< variable-sized streaming buffer - must come last
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ProtoStreamDefaultCallback

    \Description
        Default (empty) callback.

    \Input *pProtoStream    - pointer to module state
    \Input eStatus          - callback status (PROTOSTREAM_STATUS_*)
    \Input *pBuffer         - data pointer, or null
    \Input iLength          - data length, or zero
    \Input *pUserData       - user data pointer

    \Version 11/16/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoStreamDefaultCallback(ProtoStreamRefT *pProtoStream, ProtoStreamStatusE eStatus, const uint8_t *pBuffer, int32_t iLength, void *pUserData)
{
    // no data consumed
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoStreamDone

    \Description
        Called when stream is complete.

    \Input *pProtoStream    - pointer to module state

    \Output
        None.

    \Version 11/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoStreamDone(ProtoStreamRefT *pProtoStream)
{
    NetPrintf(("protostream: stream done\n"));

    // notify user of stream completion
    pProtoStream->pCallback(pProtoStream, PROTOSTREAM_STATUS_DONE, NULL, 0, pProtoStream->pUserData);

    // set up for restart if appropriate
    if (pProtoStream->iRestartFreq != PROTOSTREAM_FREQ_ONCE)
    {
        int32_t iRestartFreq = pProtoStream->iRestartFreq;

        // if the most recent http response code was an error, increase restart time
        if (pProtoStream->iLastHttpCode != PROTOHTTP_RESPONSE_SUCCESSFUL)
        {
            iRestartFreq += pProtoStream->iRestartIncr;
            if (iRestartFreq > PROTOSTREAM_RESTARTFREQ_MAX)
            {
                iRestartFreq = PROTOSTREAM_RESTARTFREQ_MAX;
            }
            else
            {
                pProtoStream->iRestartIncr *= 2;
            }
            NetPrintf(("protostream: setting restart frequency to %d due to http error %d\n",
                iRestartFreq, pProtoStream->iLastHttpCode));
        }
        else
        {
            pProtoStream->iRestartIncr = 1;
        }

        // set restart time
        pProtoStream->iRestartTime = NetTick() + iRestartFreq * 1000;
    }

    // go to idle state
    pProtoStream->eState = ST_IDLE;
}

/*F********************************************************************************/
/*!
    \Function _ProtoStreamReset

    \Description
        Reset the module state.

    \Input *pProtoStream    - pointer to module state

    \Version 11/16/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoStreamReset(ProtoStreamRefT *pProtoStream)
{
    // abort any ongoing stream
    ProtoHttpAbort(pProtoStream->pProtoHttp);

    // reset buffering state
    pProtoStream->iBufLen = 0;
    pProtoStream->iBufInp = 0;
    pProtoStream->iBufOut = 0;

    // reset other misc stuff
    pProtoStream->uLastCallback = 0;
    pProtoStream->eState = ST_IDLE;
    pProtoStream->bReceivedHeader = FALSE;
    pProtoStream->bPrebuffering = TRUE;
    pProtoStream->iStreamStatus = 0;
    pProtoStream->strUrl[0] = '\0';
    pProtoStream->iLastHttpCode = 0;
    pProtoStream->iStreamTime = 0;
    pProtoStream->iStreamRead = 0;
    pProtoStream->iRestartThreshold = pProtoStream->iBufSize/2;
}

/*F********************************************************************************/
/*!
    \Function _ProtoStreamUpdateStats

    \Description
        Update stream statistics

    \Input *pProtoStream    - pointer to module state
    \Input iDataRead        - amount of data read

    \Version 11/22/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoStreamUpdateStats(ProtoStreamRefT *pProtoStream, int32_t iDataRead)
{
    int32_t iRecvTime, iRecvElapsed;

    // get current time
    iRecvTime = NetTick();

    // prime lastrecvtime
    if (pProtoStream->iLastRecvTime == 0)
    {
        pProtoStream->iLastRecvTime = iRecvTime;
    }

    // calculate elapsed time since last receive and update last received time
    iRecvElapsed = iRecvTime - pProtoStream->iLastRecvTime;
    pProtoStream->iLastRecvTime = iRecvTime;

    // update overall time and size info
    pProtoStream->iStreamTime += iRecvElapsed;
    pProtoStream->iStreamRead += iDataRead;

    // perform latency calc using weighted time average
    if ((iRecvElapsed >= 0) && (iRecvElapsed < PROTOSTREAM_SAMPLE_PERIOD))
    {
        // figure out weight of existing data
        int32_t iWeight = PROTOSTREAM_SAMPLE_PERIOD - iRecvElapsed;
        // figure deviation first since it uses average
        int32_t iChange = pProtoStream->iBufLen - pProtoStream->iBufAvg;
        if (iChange < 0)
        {
            iChange = -iChange;
        }
        // calc weighted deviation
        pProtoStream->iBufDev = ((iWeight*pProtoStream->iBufDev) + (iRecvElapsed*iChange))/PROTOSTREAM_SAMPLE_PERIOD;
        // calc weighted average
        pProtoStream->iBufAvg = ((iWeight*pProtoStream->iBufAvg) + (iRecvElapsed*pProtoStream->iBufLen))/PROTOSTREAM_SAMPLE_PERIOD;
        // calc restart threshold, if we're not prebuffering
        if (pProtoStream->bPrebuffering == FALSE)
        {
            pProtoStream->iRestartThreshold = pProtoStream->iBufSize - pProtoStream->iBufAvg + pProtoStream->iBufDev;
            // clamp restart threshold to [0.5 ... 1.0]
            if (pProtoStream->iRestartThreshold < pProtoStream->iBufSize/2)
            {
                pProtoStream->iRestartThreshold = pProtoStream->iBufSize/2;
            }
            else if (pProtoStream->iRestartThreshold > pProtoStream->iBufSize)
            {
                pProtoStream->iRestartThreshold = pProtoStream->iBufSize;
            }
        }
        NetPrintfVerbose((pProtoStream->iVerbose, 1, "protostream: dev=%5d avg=%5d len=%5d thr=%d\n",
            pProtoStream->iBufDev, pProtoStream->iBufAvg, pProtoStream->iBufLen, pProtoStream->iRestartThreshold));
    }
    else
    {
        // if more than our scale has elapsed, use this data
        pProtoStream->iBufDev = 0;
        pProtoStream->iBufAvg = 0;
        pProtoStream->iRestartThreshold = pProtoStream->iBufSize/2;
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoStreamChanged

    \Description
        Returns whether stream has changed since previous stream or not

    \Input *pProtoStream    - pointer to module state

    \Output
        uint32_t            - TRUE if stream changed, else FALSE

    \Version 11/16/2005 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _ProtoStreamChanged(ProtoStreamRefT *pProtoStream)
{
    int32_t iLast, iSize;
    uint32_t bChanged = FALSE;

    // get size
    iSize = ProtoHttpStatus(pProtoStream->pProtoHttp, 'body', NULL, 0);

    // if size has changed or is unspecified, stream has changed
    if ((iSize <= 0) || (iSize != pProtoStream->iLastSize))
    {
        pProtoStream->iLastSize = iSize;
        bChanged = TRUE;
    }

    // get last modified time
    iLast = ProtoHttpStatus(pProtoStream->pProtoHttp, 'date', NULL, 0);

    // if last modified time has changed or is unspecified, stream has changed
    if ((iLast == 0) || (iLast != pProtoStream->iLastModified))
    {
        pProtoStream->iLastModified = iLast;
        bChanged = TRUE;
    }

    // return changed status
    return(bChanged);
}

/*F********************************************************************************/
/*!
    \Function _ProtoStreamRecv

    \Description
        Try and receive data if there is room

    \Input *pProtoStream    - pointer to module state

    \Output
        int32_t             - bytes received, or negative if error

    \Version 11/16/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoStreamRecv(ProtoStreamRefT *pProtoStream)
{
    int32_t iRecvMax, iRecvResult;

    // if the stream is done, don't try and receive any more
    if (pProtoStream->iStreamStatus != 0)
    {
        return(0);
    }

    #if DIRTYCODE_DEBUG
    if (pProtoStream->uStarveTime != 0)
    {
        if (NetTickDiff(NetTick(), pProtoStream->uStarveTime) > 0)
        {
            NetPrintf(("protostream: stream starvation complete\n"));
            pProtoStream->uStarveTime = 0;
        }
        else
        {
            return(0);
        }
    }
    #endif

    // no room in buffer?
    if (pProtoStream->iBufLen == pProtoStream->iBufSize)
    {
        return(0);
    }

    // get max size we can receive
    iRecvMax = pProtoStream->iBufSize - pProtoStream->iBufLen;
    if (iRecvMax > (pProtoStream->iBufSize - pProtoStream->iBufInp))
    {
        iRecvMax = pProtoStream->iBufSize - pProtoStream->iBufInp;
    }

    // have we received the header yet?
    if (pProtoStream->bReceivedHeader == FALSE)
    {
        // wait until we've received the header
        if (ProtoHttpStatus(pProtoStream->pProtoHttp, 'head', NULL, 0) > 0)
        {
            // if the stream hasn't changed since last time, skip it
            if (!_ProtoStreamChanged(pProtoStream))
            {
                // mark stream as complete
                NetPrintf(("protostream: stream unchanged from last play -- skipping\n"));
                ProtoHttpAbort(pProtoStream->pProtoHttp);
                pProtoStream->iStreamStatus = 1;
                return(0);
            }
            // if the response isn't ok, disable prebuffering so client code isn't stuck waiting
            if (ProtoHttpStatus(pProtoStream->pProtoHttp, 'code', NULL, 0) != PROTOHTTP_RESPONSE_OK)
            {
                pProtoStream->bPrebuffering = FALSE;
            }
            // mark header as received
            pProtoStream->bReceivedHeader = TRUE;
        }
    }

    // try and get some data
    if ((iRecvResult = ProtoHttpRecv(pProtoStream->pProtoHttp, (char *)pProtoStream->aBuffer + pProtoStream->iBufInp, 1, iRecvMax)) > 0)
    {
        #if DIRTYCODE_LOGGING
        if (pProtoStream->iVerbose > 1)
        {
            int32_t iStreamBps=0;
            if (pProtoStream->iStreamTime != 0)
            {
                iStreamBps = (int32_t)(((int64_t)pProtoStream->iStreamRead*8*1000)/(int64_t)pProtoStream->iStreamTime);
            }
            NetPrintf(("protostream: recv [0x%04x,0x%04x] len=%d bps=%d clk=%.2f\n", pProtoStream->iBufInp,
                pProtoStream->iBufInp+iRecvResult, pProtoStream->iBufLen+iRecvResult, iStreamBps,
                (float)pProtoStream->iStreamTime/1000.0f));
        }
        #endif

        // update buffer pointers
        pProtoStream->iBufLen += iRecvResult;
        pProtoStream->iBufInp += iRecvResult;
        if (pProtoStream->iBufInp == pProtoStream->iBufSize)
        {
            pProtoStream->iBufInp = 0;
        }
    }
    else if (iRecvResult == PROTOHTTP_RECVDONE)
    {
        // mark stream as complete
        NetPrintf(("protostream: stream data transfer complete\n"));
        pProtoStream->iLastHttpCode = ProtoHttpStatus(pProtoStream->pProtoHttp, 'code', NULL, 0);
        pProtoStream->iStreamStatus = (pProtoStream->iLastHttpCode == PROTOHTTP_RESPONSE_OK) ? 1 : -1;
        if (pProtoStream->iStreamStatus < 0)
        {
            ds_strsubzcpy(pProtoStream->strError, sizeof(pProtoStream->strError), (char *)pProtoStream->aBuffer+pProtoStream->iBufOut, pProtoStream->iBufLen-pProtoStream->iBufOut);
        }
    }
    else if ((iRecvResult < 0) && (iRecvResult != PROTOHTTP_RECVWAIT))
    {
        NetPrintf(("protostream: error %d receiving http response\n", iRecvResult));
        pProtoStream->iStreamStatus = -1;
        pProtoStream->iLastSize = -1;
    }

    // return result to caller
    return(iRecvResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoStreamDataCallback

    \Description
        Determine amount of data that can be read from buffer, and call
        user callback with a pointer to the beginning of the data in the queue
        and the amount of data that can be read.

    \Input *pProtoStream    - pointer to module state
    \Input eStatus          - current status

    \Output
        int32_t             - amount of data read

    \Version 01/25/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoStreamDataCallback(ProtoStreamRefT *pProtoStream, ProtoStreamStatusE eStatus)
{
    int32_t iRead, iReadMax;

    // determine max amount of data that can be read
    iReadMax = pProtoStream->iBufSize - pProtoStream->iBufOut;
    if (iReadMax > pProtoStream->iBufLen)
    {
        iReadMax = pProtoStream->iBufLen;
    }

    // if buffer minimum conditions do not apply, allow the user to grab some data
    if ((iReadMax > pProtoStream->iBufMin) || ((pProtoStream->iBufMin >= pProtoStream->iBufLen) && (pProtoStream->iStreamStatus == 1)))
    {
        if ((iRead = pProtoStream->pCallback(pProtoStream, eStatus, pProtoStream->aBuffer + pProtoStream->iBufOut, iReadMax, pProtoStream->pUserData)) > 0)
        {
            NetPrintfVerbose((pProtoStream->iVerbose, 1, "protostream: read [0x%04x,0x%04x]\n", pProtoStream->iBufOut, pProtoStream->iBufOut+iRead));

            // if they consumed too much data, print a diagnostic warning and clamp
            if (iRead > iReadMax)
            {
                NetPrintf(("protostream: warning; tried to read %d bytes when max was %d\n", iRead, iReadMax));
                iRead = iReadMax;
            }

            // update buffer status
            pProtoStream->iBufOut += iRead;
            if (pProtoStream->iBufOut == pProtoStream->iBufSize)
            {
                pProtoStream->iBufOut = 0;
            }
            pProtoStream->iBufLen -= iRead;
        }
        else if (iRead < 0)
        {
            NetPrintf(("protostream: stream error - aborting\n"));
            pProtoStream->eState = ST_IDLE;
        }
    }
    else // enforce minimum data amount, if specified
    {
        // make sure we have enough data
        if (pProtoStream->iBufLen < pProtoStream->iBufMin)
        {
            return(0);
        }
        // read minimum amount of data
        if ((iReadMax = ProtoStreamRead(pProtoStream, (char *)pProtoStream->pBufMin, pProtoStream->iBufMin, pProtoStream->iBufMin)) != pProtoStream->iBufMin)
        {
            NetPrintf(("protostream: stream error; minbuf read failed err=%d\n", iReadMax));
            return(0);
        }
        // pass data on to caller
        iRead = pProtoStream->pCallback(pProtoStream, eStatus, pProtoStream->pBufMin, iReadMax, pProtoStream->pUserData);
    }

    return(iRead);
}

/*F********************************************************************************/
/*!
    \Function _ProtoStreamDataCallbackProcess

    \Description
        Do data callback processing.

    \Input *pProtoStream    - pointer to module state

    \Output
        None.

    \Version 11/16/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoStreamDataCallbackProcess(ProtoStreamRefT *pProtoStream)
{
    ProtoStreamStatusE eStatus;
    uint32_t uTick = NetTick();
    int32_t iRead;

    // if we're prebuffering, wait until there is enough data or the stream is complete
    if (pProtoStream->bPrebuffering == TRUE)
    {
        if ((pProtoStream->iBufLen < pProtoStream->iRestartThreshold) && (pProtoStream->iStreamStatus != 1))
        {
            return;
        }
        NetPrintf(("protostream: prebuffered %d bytes\n", pProtoStream->iBufLen));
        pProtoStream->bPrebuffering = FALSE;
    }

    // determine callback status type
    if (pProtoStream->uLastCallback != 0)
    {
        // recurring data callback
        eStatus = PROTOSTREAM_STATUS_DATA;

        // only update if update rate exceeded and not paused
        if ((NetTickDiff(uTick, pProtoStream->uLastCallback) < pProtoStream->iCallbackRate) || (pProtoStream->bPaused == TRUE))
        {
            return;
        }
    }
    else
    {
        // start of stream
        eStatus = PROTOSTREAM_STATUS_BEGIN;
    }

    // update callback timer
    pProtoStream->uLastCallback = uTick;

    // do the callback
    iRead = _ProtoStreamDataCallback(pProtoStream, eStatus);
    if ((iRead > 0) && (pProtoStream->iBufOut == 0) && (pProtoStream->iBufLen > 0))
    {
        /* if they consumed all of the data, and we wrapped,
           and there is still data to be consumed, give them
           another shot at the data */
        _ProtoStreamDataCallback(pProtoStream, eStatus);
    }

    // if we have emptied the buffer, and the stream is not done, rebuffer
    if ((pProtoStream->iBufLen == 0) && (pProtoStream->iStreamStatus == 0))
    {
        NetPrintf(("protostream: exhausted buffer; prebuffering %d bytes\n", pProtoStream->iRestartThreshold));
        pProtoStream->bPrebuffering = TRUE;
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoStreamOpen

    \Description
        Begin streaming an Internet media source

    \Input *pProtoStream    - pointer to module state
    \Input *pUrl            - resource to stream
    \Input *pReq            - request body, if POST
    \Input iFreq            - restart frequency in seconds, or PROTOSTREAM_FREQ_*

    \Output
        int32_t             - negative=failure, else success

    \Version 11/16/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoStreamOpen(ProtoStreamRefT *pProtoStream, const char *pUrl, const char *pReq, int32_t iFreq)
{
    int32_t iResult;

    // reset state
    _ProtoStreamReset(pProtoStream);

    // cache info
    pProtoStream->iRestartFreq = iFreq;
    ds_strnzcpy(pProtoStream->strUrl, pUrl, sizeof(pProtoStream->strUrl));

    // open stream
    if ((iResult = ProtoHttpRequest(pProtoStream->pProtoHttp, pUrl, pReq, -1, pReq != NULL ? PROTOHTTP_REQUESTTYPE_POST : PROTOHTTP_REQUESTTYPE_GET)) >= 0)
    {
        pProtoStream->eState = ST_OPEN;
    }
    else
    {
        NetPrintf(("protostream: error opening stream '%s'\n", pUrl));
    }

    // return result code to caller
    return(iResult);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoStreamCreate

    \Description
        Create the stream module

    \Input iBufSize         - size of streaming buffer (at least PROTOSTREAM_MINBUFFER)

    \Output
        ProtoStreamRefT *   - new module state, or NULL

    \Version 11/16/2005 (jbrookes)
*/
/********************************************************************************F*/
ProtoStreamRefT *ProtoStreamCreate(int32_t iBufSize)
{
    ProtoStreamRefT *pProtoStream;
    int32_t iRefSize;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // enforce minimum buffer size
    if (iBufSize < PROTOSTREAM_MINBUFFER)
    {
        iBufSize = PROTOSTREAM_MINBUFFER;
    }

    // calc ref size
    iRefSize = sizeof(*pProtoStream) - sizeof(pProtoStream->aBuffer) + iBufSize;

    // allocate and init module state
    if ((pProtoStream = DirtyMemAlloc(iRefSize, PROTOSTREAM_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protostream: could not allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pProtoStream, iRefSize);
    pProtoStream->iMemGroup = iMemGroup;
    pProtoStream->pMemGroupUserData = pMemGroupUserData;
    pProtoStream->pCallback = _ProtoStreamDefaultCallback;

    // create http ref
    if ((pProtoStream->pProtoHttp = ProtoHttpCreate(4096)) == NULL)
    {
        NetPrintf(("protostream: could not allocate http module\n"));
        DirtyMemFree(pProtoStream, PROTOSTREAM_MEMID, pProtoStream->iMemGroup, pProtoStream->pMemGroupUserData);
        return(NULL);
    }
    
    // init other state variables
    pProtoStream->iBufSize = iBufSize;
    pProtoStream->iBufMin = -1;
    pProtoStream->iVerbose = 1;
    pProtoStream->iTimeout = 60*1000;
    ProtoHttpControl(pProtoStream->pProtoHttp, 'time', pProtoStream->iTimeout, 0, NULL);

    // return ref to caller
    return(pProtoStream);
}

/*F********************************************************************************/
/*!
    \Function ProtoStreamDestroy

    \Description
        Destroy the ProtoStream module

    \Input *pProtoStream    - pointer to module state

    \Version 11/16/2005 (jbrookes)
*/
/********************************************************************************F*/
void ProtoStreamDestroy(ProtoStreamRefT *pProtoStream)
{
    // free minbuf buffer, if allocated
    if (pProtoStream->pBufMin != NULL)
    {
        DirtyMemFree(pProtoStream->pBufMin, PROTOSTREAM_MEMID, pProtoStream->iMemGroup, pProtoStream->pMemGroupUserData);
    }

    // dispose of http module
    ProtoHttpDestroy(pProtoStream->pProtoHttp);

    // dispose of module memory
    DirtyMemFree(pProtoStream, PROTOSTREAM_MEMID, pProtoStream->iMemGroup, pProtoStream->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function ProtoStreamSetCallback

    \Description
        Set recurring ProtoStream callback.

    \Input *pProtoStream    - pointer to module state
    \Input iRate            - callback rate in ms
    \Input *pCallback       - data callback
    \Input *pUserData       - user data for callback

    \Version 11/16/2005 (jbrookes)
*/
/********************************************************************************F*/
void ProtoStreamSetCallback(ProtoStreamRefT *pProtoStream, int32_t iRate, ProtoStreamCallbackT *pCallback, void *pUserData)
{
    pProtoStream->pCallback = (pCallback != NULL) ? pCallback : _ProtoStreamDefaultCallback;
    pProtoStream->pUserData = pUserData;
    pProtoStream->iCallbackRate = iRate;
}

/*F********************************************************************************/
/*!
    \Function ProtoStreamSetHttpCallback

    \Description
        Set custom callbacks for ProtoHttp ref managed by ProtoStream.

    \Input *pProtoStream    - pointer to module state
    \Input *pCustomHeaderCb - pointer to custom send header callback (may be NULL)
    \Input *pReceiveHeaderCb- pointer to recv header callback (may be NULL)
    \Input *pUserData       - user-supplied callback ref (may be NULL)

    \Version 10/26/2011 (jbrookes)
*/
/********************************************************************************F*/
void ProtoStreamSetHttpCallback(ProtoStreamRefT *pProtoStream, ProtoHttpCustomHeaderCbT *pCustomHeaderCb, ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb, void *pUserData)
{
    ProtoHttpCallback(pProtoStream->pProtoHttp, pCustomHeaderCb, pReceiveHeaderCb, pUserData);
}

/*F********************************************************************************/
/*!
    \Function ProtoStreamOpen

    \Description
        Begin streaming an Internet media source

    \Input *pProtoStream    - pointer to module state
    \Input *pUrl            - resource to stream
    \Input iFreq            - restart frequency in seconds, or PROTOSTREAM_FREQ_*

    \Output
        int32_t             - negative=failure, else success

    \Version 11/16/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoStreamOpen(ProtoStreamRefT *pProtoStream, const char *pUrl, int32_t iFreq)
{
    // reset restart values
    pProtoStream->iRestartIncr = 1;
    pProtoStream->iRestartThreshold = pProtoStream->iBufSize/2;

    // reset previous stream info
    pProtoStream->iLastSize = 0;
    pProtoStream->iLastModified = 0;

    // open stream
    return(_ProtoStreamOpen(pProtoStream, pUrl, NULL, iFreq));
}

/*F********************************************************************************/
/*!
    \Function ProtoStreamOpen2

    \Description
        Begin streaming an Internet media source using POST request

    \Input *pProtoStream    - pointer to module state
    \Input *pUrl            - resource to stream
    \Input *pReq            - request body
    \Input iFreq            - restart frequency in seconds, or PROTOSTREAM_FREQ_*

    \Output
        int32_t             - negative=failure, else success

    \Version 10/30/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoStreamOpen2(ProtoStreamRefT *pProtoStream, const char *pUrl, const char *pReq, int32_t iFreq)
{
    // reset restart values
    pProtoStream->iRestartIncr = 1;
    pProtoStream->iRestartThreshold = pProtoStream->iBufSize/2;

    // reset previous stream info
    pProtoStream->iLastSize = 0;
    pProtoStream->iLastModified = 0;

    // open stream
    return(_ProtoStreamOpen(pProtoStream, pUrl, pReq, iFreq));
}

/*F********************************************************************************/
/*!
    \Function ProtoStreamRead

    \Description
        Read at least iMinLen bytes from stream into user buffer.

    \Input *pProtoStream    - pointer to module state
    \Input *pBuffer         - [out] output buffer
    \Input iBufLen          - size of output buffer
    \Input iMinLen          - minimum amount of data to copy

    \Output
        int32_t             - number of bytes copied; negative=stream not open

    \Version 11/21/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoStreamRead(ProtoStreamRefT *pProtoStream, char *pBuffer, int32_t iBufLen, int32_t iMinLen)
{
    char *pBufStart = pBuffer;
    int32_t iBufCopy;

    // make sure we're open
    if (pProtoStream->eState != ST_OPEN)
    {
        NetPrintf(("protostream: no stream open\n"));
        return(-1);
    }

    // make sure we're not prebuffering, have enough data, and are not paused
    if ((pProtoStream->bPrebuffering == TRUE) || (pProtoStream->iBufLen < iMinLen) || (pProtoStream->bPaused == TRUE))
    {
        return(0);
    }

    // determine amount of data to copy
    iBufCopy = (pProtoStream->iBufLen < iBufLen) ? pProtoStream->iBufLen : iBufLen;

    // does copy span tail end of buffer?
    if ((pProtoStream->iBufOut + iBufCopy) > pProtoStream->iBufSize)
    {
        // copy tail end of data to buffer
        int32_t iBufCopy2 = pProtoStream->iBufSize - pProtoStream->iBufOut;
        ds_memcpy(pBuffer, &pProtoStream->aBuffer[pProtoStream->iBufOut], iBufCopy2);

        NetPrintfVerbose((pProtoStream->iVerbose, 1, "protostream: read [0x%04x,0x%04x]\n", pProtoStream->iBufOut, pProtoStream->iBufOut+iBufCopy2));

        // adjust buffer parameters
        pProtoStream->iBufOut = 0;
        pProtoStream->iBufLen -= iBufCopy2;

        // adjust current copy parameters
        pBuffer += iBufCopy2;
        iBufCopy -= iBufCopy2;
    }

    // copy data to output buffer
    NetPrintfVerbose((pProtoStream->iVerbose, 1, "protostream: read [0x%04x,0x%04x]\n", pProtoStream->iBufOut, pProtoStream->iBufOut+iBufCopy));
    ds_memcpy(pBuffer, &pProtoStream->aBuffer[pProtoStream->iBufOut], iBufCopy);
    pBuffer += iBufCopy;

    // adjust buffer parameters
    pProtoStream->iBufOut += iBufCopy;
    if (pProtoStream->iBufOut == pProtoStream->iBufSize)
    {
        pProtoStream->iBufOut = 0;
    }
    pProtoStream->iBufLen -= iBufCopy;

    // return amount copied to caller
    return((int32_t)(pBuffer - pBufStart));
}

/*F********************************************************************************/
/*!
    \Function ProtoStreamPause

    \Description
        Pause/unpause stream.

    \Input *pProtoStream    - pointer to module state
    \Input bPause           - TRUE to pause, else FALSE

    \Version 01/25/2012 (jbrookes)
*/
/********************************************************************************F*/
void ProtoStreamPause(ProtoStreamRefT *pProtoStream, uint8_t bPause)
{
    NetPrintf(("protostream: pause %s (was %s)\n", bPause ? "enabled" : "disabled", pProtoStream->bPaused ? "enabled" : "disabled"));
    if ((pProtoStream->bPaused = bPause) == TRUE)
    {
        ProtoHttpControl(pProtoStream->pProtoHttp, 'time', 0x7fffffff, 0, NULL);
    }
    else
    {
        ProtoHttpControl(pProtoStream->pProtoHttp, 'time', pProtoStream->iTimeout, 0, NULL);
    }
}

/*F********************************************************************************/
/*!
    \Function ProtoStreamClose

    \Description
        Stop streaming

    \Input *pProtoStream    - pointer to module state

    \Version 11/16/2005 (jbrookes)
*/
/********************************************************************************F*/
void ProtoStreamClose(ProtoStreamRefT *pProtoStream)
{
    _ProtoStreamReset(pProtoStream);
}

/*F********************************************************************************/
/*!
    \Function ProtoStreamStatus

    \Description
        Get module status.

    \Input *pProtoStream    - pointer to module state
    \Input iStatus          - status selector
    \Input *pBuffer         - selector specific
    \Input iBufSize         - selector specific

    \Output
        int32_t             - selector specific

    \Notes
        iStatus can be one of the following:

        \verbatim
            'bufl' - amount of data currently buffered
            'bufs' - size of current stream buffer
            'code' - most recent http result code
            'done' - stream status: -1=error, 0=in progress, 1=complete
            'serr' - copies http error result string to pBuffer
        \endverbatim

        Unrecognized codes are passed through to ProtoHttpStatus().

    \Version 11/16/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoStreamStatus(ProtoStreamRefT *pProtoStream, int32_t iStatus, void *pBuffer, int32_t iBufSize)
{
    // return current amount of data in buffer
    if (iStatus == 'bufl')
    {
        return(pProtoStream->iBufLen);
    }
    // return current size of stream buffer
    if (iStatus == 'bufs')
    {
        return(pProtoStream->iBufSize);
    }
    // return most recent http result code
    if (iStatus == 'code')
    {
        return(pProtoStream->iLastHttpCode);
    }
    // return completion status
    if (iStatus == 'done')
    {
        return(pProtoStream->iStreamStatus);
    }
    // return error string, if set
    if (iStatus == 'serr')
    {
        ds_strnzcpy(pBuffer, pProtoStream->strError, iBufSize);
        return(0);
    }
    // if not handled, let ProtoHttp take a stab at it
    return(ProtoHttpStatus(pProtoStream->pProtoHttp, iStatus, pBuffer, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function ProtoStreamControl

    \Description
        Set control options

    \Input *pProtoStream    - pointer to module state
    \Input iControl         - control selector
    \Input iValue           - selector specific
    \Input iValue2          - selector specific
    \Input *pValue          - selector specific

    \Output
        int32_t             - selector specific

    \Notes
        iStatus can be one of the following:

        \verbatim
            'minb' - set minimum data amount of data for data callback (note; not enforced at end of stream)
            'play' - set whether playing or not (client timeout is disabled if not playing)
            'spam' - set verbose debug level (debug only)
            'strv' - set number of seconds to starve input (debug only)
        \endverbatim

        Unhandled codes are passed through to ProtoHttpControl().

    \Version 11/16/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoStreamControl(ProtoStreamRefT *pProtoStream, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    // set minimum amount of data caller needs in a callback
    if (iControl == 'minb')
    {
        if ((pProtoStream->iBufMin != iValue) && (pProtoStream->pBufMin != NULL))
        {
            DirtyMemFree(pProtoStream->pBufMin, PROTOSTREAM_MEMID, pProtoStream->iMemGroup, pProtoStream->pMemGroupUserData);
        }
        if ((iValue > 0) && ((pProtoStream->pBufMin = DirtyMemAlloc(iValue, PROTOSTREAM_MEMID, pProtoStream->iMemGroup, pProtoStream->pMemGroupUserData)) != NULL))
        {
            pProtoStream->iBufMin = iValue;
        }
        else
        {
            NetPrintf(("protostream: could not allocate %d bytes for minbuffer\n", iValue));
            pProtoStream->iBufMin = -1;
        }
        return(0);
    }
    // set http timeout based on if application is playing back or not
    if (iControl == 'play')
    {
        // set parameters to pass through to ProtoHttp
        iControl = 'time';
        iValue = (iValue != 0) ? pProtoStream->iTimeout : 0x40000000; // 'disabled' == very large timeout value
    }
    #if DIRTYCODE_LOGGING
    // set verbosity for us and pass through to protohttp as well
    if (iControl == 'spam')
    {
        pProtoStream->iVerbose = iValue;
    }
    #endif
    #if DIRTYCODE_DEBUG
    // set to starve for a period of time (debug only)
    if (iControl == 'strv')
    {
        NetPrintf(("protostream: starving input for %d seconds\n", iValue));
        pProtoStream->uStarveTime = NetTick() + (iValue * 1000);
        return(0);
    }
    #endif
    if (iControl == 'time')
    {
        // remember most recent timeout value, and pass through to protohttp
        pProtoStream->iTimeout = iValue;
    }

    // if not handled, let ProtoHttp take a stab at it
    return(ProtoHttpControl(pProtoStream->pProtoHttp, iControl, iValue, iValue2, pValue));
}

/*F********************************************************************************/
/*!
    \Function ProtoStreamUpdate

    \Description
        Update the ProtoStream module

    \Input *pProtoStream    - pointer to module state

    \Version 11/16/2005 (jbrookes)
*/
/********************************************************************************F*/
void ProtoStreamUpdate(ProtoStreamRefT *pProtoStream)
{
    // update module in idle state
    if (pProtoStream->eState == ST_IDLE)
    {
        // if we have a url and are in restart mode
        if ((pProtoStream->strUrl[0] != '\0') && (pProtoStream->iRestartFreq != PROTOSTREAM_FREQ_ONCE))
        {
            // time to restart?
            if (NetTickDiff(NetTick(), pProtoStream->iRestartTime) > 0)
            {
                char strUrl[PROTOSTREAM_MAXURL];
                ds_strnzcpy(strUrl, pProtoStream->strUrl, sizeof(strUrl));
                _ProtoStreamOpen(pProtoStream, strUrl, NULL, pProtoStream->iRestartFreq);
            }
        }
    }

    // update stream in open state
    if (pProtoStream->eState == ST_OPEN)
    {
        int32_t iRecvResult;

        // give time to http module
        ProtoHttpUpdate(pProtoStream->pProtoHttp);

        // try and receive data
        for ( iRecvResult = 1; iRecvResult > 0; )
        {
            if ((iRecvResult = _ProtoStreamRecv(pProtoStream)) >= 0)
            {
                _ProtoStreamUpdateStats(pProtoStream, iRecvResult);
            }
        }

        // user data callback processing
        _ProtoStreamDataCallbackProcess(pProtoStream);

        // check for stream completion
        if ((pProtoStream->iStreamStatus != 0) && (pProtoStream->iBufLen == 0))
        {
            _ProtoStreamDone(pProtoStream);
        }
    }
}
