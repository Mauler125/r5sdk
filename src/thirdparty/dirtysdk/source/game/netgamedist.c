/*H*************************************************************************************************/
/*!

    \File    netgamedist.c

    \Description
        This file provides some upper layer protocol abstractions such as controller packet
        buffering and exchange logic.

    \Notes
    \verbatim

    From a "client" perspective:

        pRef->InpBufData is the inbound queue where inbound multipackets from server are accumulated.
            * pRef->InpBufData.iBeg identifies the oldest entry in that queue 
              (i.e. the next entry to be consumed locally)
            * pRef->InpBufData.iEnd identifies the next free entry in that queue
              (i.e. the next spot to write into when we read from the socket)
            * An overflow in that queue is always detected by comparing pRef->InpBufData.iBeg and pRef->InpBufData.iEnd.

        pRef->OutBufData is the outbound queue where outbound packets are accumulated pending transmission to the server.
            * pRef->OutBufData.iBeg identifies the oldest entry in that queue
              (i.e. the next entry to be sent over the network)
            * pRef->OutBufData.iEnd identifies the next free entry in that queue
              (i.e. the next spot to write into when user submits data)
            * SPECIFICITY: When pRef->OutBufData.iBeg is advanced, the entry that it used to point to is NOT YET invalidated
              because it still needs to be surfaced up to the user later when paired with a inbound bundle from the server. 
              To track those "pending entries" located before the position of pRef->OutBufData.iBeg, this third pointer
              is used: "pRef->InpBufData.iBeg+pRef->iIOOffset".  Notice that it consists of a pointer tracking the 
              other queue + an adjustment offset.
            * An overflow in that queue is always detected by comparing "pRef->InpBufData.iBeg+pRef->iIOOffset"
              and pRef->OutBufData.iEnd.

    From a "server" perspective:

        pRef->InpBufData is the inbound queue where inbound packets from a specific client are accumulated.
            * pRef->InpBufData.iBeg identifies the oldest entry in that queue
                (i.e. the next entry to be consumed locally)
            * pRef->InpBufData.iEnd identifies the next free entry in that queue
                (i.e. the next spot to write into when we read from the socket)
            * An overflow in that queue is always detected by comparing pRef->InpBufData.iBeg and pRef->InpBufData.iEnd.

        pRef->OutBufData is the outbound queue where outbound multipackets are accumulated pending transmission to a specific client.
            * pRef->OutBufData.iBeg identifies the oldest entry in that queue
                (i.e. the next entry to be sent over the network)
            * pRef->OutBufData.iEnd identifies the next free entry in that queue
              (i.e. the next spot to write into when the server has a bundle of paired inputs to submit for transmission)
            * An overflow in that queue is always detected by comparing pRef->OutBufData.iBeg and pRef->OutBufData.iEnd

    \endverbatim

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2000-2018.  ALL RIGHTS RESERVED.

    \Version    1.0        12/20/00 (GWS) Based on split of GmClient.c
    \Version    1.1        12/31/01 (GWS) Cleaned up and made really platform independent
    \Version    1.2        12/03/09 (mclouatre) Added configurable run-time verbosity
*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <string.h>
#include <stdio.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/game/netgamepkt.h"
#include "DirtySDK/game/netgamelink.h"
#include "DirtySDK/game/netgamedist.h"

/*** Defines ***************************************************************************/

#define NETGAMEDIST_VERBOSITY (2)

#define TIMING_DEBUG                    (0)
#define PING_DEBUG                      (0)
#define INPUTCHECK_LOGGING_DELAY        (15)    // 15 msec
#define GMDIST_META_ARRAY_SIZE          (32)    // how many past versions of sparse multipacket to keep

// PACKET_WINDOW can be overriden at build time with the nant global property called dirtysdk-distpktwindow-size
#ifndef PACKET_WINDOW
#define PACKET_WINDOW                   (64)
#endif

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! Describe an entry in the flat buffer.
typedef struct GameBufferLookupT
{
    uint32_t uInsertTime;   //!< time when packet was queued
    uint32_t uPos;          //!< indexes into OutBufData.pControllerIO or InpBufData.pControllerIO
    uint16_t uLen;
    uint16_t uLenSize;
} GameBufferLookupT;

//! Flat buffer structure. Implements a wrapping queue of packets.
typedef struct GameBufferDataT
{
    //! incoming and outgoing controller packets
    unsigned char *pControllerIO;
    //! length of the io buffer
    uint32_t uBufLen;
    // input/output lookup, addresses into pControllerIO
    GameBufferLookupT IOLookUp[PACKET_WINDOW];
    //! index of first packet to send/process in pControllerIO
    int32_t iBeg;
    //! index of last packet to send/process in pControllerIO
    int32_t iEnd;
} GameBufferDataT;

//! Describes one version of multipacket.
typedef struct NetGameDistMetaInfoT
{
    //! which entries are used
    uint32_t uMask;
    //! number of players used
    uint8_t uPlayerCount;
    //! version number
    uint8_t uVer;
} NetGameDistMetaInfoT;

//! netgamedist internal state
struct NetGameDistRefT
{
    //! module memory group
    int32_t iMemGroup;
    void *pMemGroupUserData;

    //! output buffer for packets and lookup table
    GameBufferDataT OutBufData;
    //! input buffer for packets and lookup table
    GameBufferDataT InpBufData;

    //! offset between the input and output queues
    int32_t iIOOffset;

    //! local sequence number
    uint32_t uLocalSeq;
    //! global sequence number
    uint32_t uGlobalSeq;

    //! external status monitoring
    NetGameLinkStatT NetGameLinkStats;

    //! current exchange rate
    int32_t iInputRate;
    //! input exchange window
    int32_t iInputWind;
    //! clamp the min window size
    int32_t iInputMini;
    //! clamp the max window size
    int32_t iInputMaxi;

    //! when to recalc window
    uint32_t uInpCalc;
    //! when packet was last send
    uint32_t uInpNext;

    //! netgamelink ref
    void *pNetGameLinkRef;
    //! netgamelink stat func
    NetGameDistStatProc *pStatProc;
    //! netgame send function
    NetGameDistSendProc *pSendProc;
    //! netgame recv function
    NetGameDistRecvProc *pRecvProc;

    NetGameDistDropProc *pDropProc;
    NetGameDistPeekProc *pPeekProc;

    NetGameDistLinkCtrlProc *pLinkCtrlProc;

    //! when bActAsServer is true the index is of the player that owns the dist 
    //! if bActAsServer is false the index is of the local player in the dist game
    uint32_t uDistIndex;

    //! the total number of players
    uint32_t uTotalPlyrs;

    //! true if we are receiving multi packets from dirtycast in OTP mode
    //! false if when inbound packets originated from the client in a 2 player mode
    uint32_t bRecvMulti;

    //! whether we are acting as the server.
    uint32_t bActAsServer;

    //! a max packet for use by input
    NetGameMaxPacketT MaxPkt;

    //! a max packet for use by input
    char aMultiBuf[NETGAME_DATAPKT_MAXSIZE];

    //! the number of writes to a position in the input queue, (dropproc)
    int32_t aPacketId[PACKET_WINDOW];

    //! latest stats received by each pClient
    NetGameDistStatT aRecvStats[GMDIST_MAX_CLIENTS];

    //! Error condition. Set during calls like update, if an error occurs.
    int32_t iErrCond;
    int32_t iLastSentDelta;
    uint32_t uSkippedInputCheckLogCount;
    uint32_t uLastInputCheckLogTick;
    
    int32_t iInboundDropPktCnt;
    int32_t iInboundPktCnt;
    int32_t iOutboundPktCnt;

    //! total wait time in input queue
    int32_t iWaitTimeTotal;

    //! total Input deqeued count
    int32_t iInboundDequeueCnt;

    //! total dist processing time
    int32_t iDistProcTimeTotal;

    //! total dist inputs processed
    int32_t iDistProcCnt;

    char strErrorText[GMDIST_ERROR_SIZE];

    //! whether we must update the flow control flags to the game server
    uint8_t bUpdateFlowCtrl;

    //! the range of meta info we must update the
    uint8_t uUpdateMetaInfoBeg;
    uint8_t uUpdateMetaInfoEnd;

    //! whether we are ready to send or not
    uint8_t bRdySend;

    //! whether we are ready to receive or not
    uint8_t bRdyRecv;

    //! whether remote is ready to send or not
    uint8_t bRdySendRemote;

    //! whether remote is ready to receive or not
    uint8_t bRdyRecvRemote;

    uint32_t uLocalCRC;
    uint8_t bLocalCRCValid;

    uint32_t uRemoteCRC;
    uint8_t bRemoteCRCValid;

    //! debug output verbosity
    uint8_t uVerbose;

    //! boolean indicating whether we want to surface CRC cahllenges from the GS
    uint8_t bCRCChallenges;

    //! boolean indicating whether we received meta information on the packet layout
    uint8_t bGotMetaInfo;

    //! boolean indicating whether we are sending sparse multi-packets
    uint8_t bSparse;

    //! Meta information about sparse multi-packets to send
    NetGameDistMetaInfoT aMetaInfoToSend[GMDIST_META_ARRAY_SIZE];

    //! Received meta information about sparse multi-packets (wraps around)
    NetGameDistMetaInfoT aMetaInfo[GMDIST_META_ARRAY_SIZE];

    //! The version meta information from the last peeked or queried packet
    uint32_t uLastQueriedVersion;
};

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

#if DIRTYCODE_LOGGING
// The following is meant to be indexed with the corresponding constants defined in netgamedist.h.
// Array contents need to be tailored if constants are removed, modified or added.
static char _strNetGameDistDataTypes[6][32] =
    {"INVALID",
     "GMDIST_DATA_NONE",
     "GMDIST_DATA_INPUT",
     "GMDIST_DATA_INPUT_DROPPABLE",
     "GMDIST_DATA_DISCONNECT",
     "GMDIST_DATA_NODATA"};
#endif

// Public variables


/*** Private Functions *****************************************************************/


#if PING_DEBUG
/*F*************************************************************************************************/
/*!
    \Function    _PingHistory

    \Description
        Display the uPing history [DEBUG only]

    \Input *pStats      - pointer to NetGameLinkStat struct

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
static void _PingHistory(const NetGameLinkStatT *pStats)
{
    int32_t iPing;
    int32_t iIndex;
    char strMin[64];
    char strMax[64];
    char strAvg[64];
    char strCnt[64];
    const NetGameLinkHistT *pHist;
    static uint32_t uPrev = 0;

    // see if its time
    if (pStats->pingslot == uPrev)
    {
        return;
    }
    uPrev = pStats->pingslot;

    iPing = 0;

    for (iIndex = 0; iIndex < PING_HISTORY; ++iIndex)
    {
        pHist = pStats->pinghist + ((pStats->pingslot - iIndex) & (PING_HISTORY-1));
        strMin[iIndex] = '0'+pHist->min/50;
        strMax[iIndex] = '0'+pHist->max/50;
        strAvg[iIndex] = '0'+pHist->avg/50;
        strCnt[iIndex] = '0'+pHist->cnt;

        iPing += pHist->avg;
    }
    strMin[iIndex] = 0;
    strMax[iIndex] = 0;
    strAvg[iIndex] = 0;
    strCnt[iIndex] = 0;

    iPing /= PING_HISTORY;

    NetPrintf(("history(%d/%d): ping=%d, late=%d, calc=%d\n", pStats->pingslot, pStats->pingtick, pStats->ping, pStats->late, iPing));
    NetPrintf((" %s\n", strMin));
    NetPrintf((" %s\n", strMax));
    NetPrintf((" %s\n", strAvg));
    NetPrintf((" %s\n", strCnt));
}
#endif

/*F*************************************************************************************************/
/*!
    \Function    _SetDataPtrTry

    \Description
        returns the position in the input or output buffer for addition of a packet of length uLength.

    \Input *pRef    - reference pointer
    \Input *pBuffer - buffer pointer
    \Input uLength  - length of the next packet to be stored

    \Output
        int32_t     - pos if a space was found. -1 if the buffer cannot accomodate uLength

    \Version 02/08/07 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _SetDataPtrTry(NetGameDistRefT *pRef, GameBufferDataT *pBuffer, uint16_t uLength)
{
    uint32_t uIndexA, uIndexB;
    uint32_t uPosA;

    if (uLength > pBuffer->uBufLen)
    {
        ds_snzprintf(pRef->strErrorText, sizeof(pRef->strErrorText), "overflow in _SetDataPtrTry. requested length (%d) > buffer size (%d), ", uLength, pBuffer->uBufLen);
        NetPrintf(("netgamedist: [%p] critical failure in _SetDataPtrTry()\n", pRef));
        return(-1);
    }

    // identify the IOLookup index that points to the last-written entry (uIndexA), and find out where the corresponding next free byte exactly is (uPosA)
    uIndexA = GMDIST_Modulo(pBuffer->iEnd - 1, PACKET_WINDOW);
    uPosA = pBuffer->IOLookUp[uIndexA].uPos + pBuffer->IOLookUp[uIndexA].uLen;
    uPosA = ((uPosA + 3)&~3); // aligns posA to the next 4-byte boundary

    // identify the IOLookup index that points to the oldest valid entry (uIndexB)
    if ((pBuffer == &pRef->InpBufData) || pRef->bActAsServer)
    {
        /* for the input queue (inbound data from either server or client) or the server output queue (outbound data to client)
           the iBeg index identifies the oldest valid data */
        uIndexB = pBuffer->iBeg;
    }
    else
    {
        /* For the client output queue (outbound data to server), the iBeg index identifies the next packet to be
        sent to the server not the oldest valid data. The oldest valid data, i.e. pending data to be
        notified as paired by the game server, is rather always identified using a combination of
        the "inbound" iBeg and the iOOffset. */
        uIndexB = GMDIST_Modulo(pRef->InpBufData.iBeg + pRef->iIOOffset, PACKET_WINDOW);
    }

    // if the buffer is empty then we can just start filling it in from the beginning
    if (uIndexB == (unsigned)pBuffer->iEnd)
    {
        return(0);
    }
    else
    {
        /* note: uIndexB can be used to access valid data in IOLookup[] only if uIndexB != pBuffer->iEnd
                 because pBuffer->iEnd points to the next "free" entry... so it contains no valid data yet. */

        // find out where the last valid byte exactly is (uPosB)
        uint32_t uPosB = pBuffer->IOLookUp[uIndexB].uPos;

        if (uPosA >= uPosB)
        {
            // if we can't fit at the end, let's retry from the beginning
            if ((uPosA + uLength) > pBuffer->uBufLen)
            {
                uPosA = 0;
                // fall through to the next 'if'
            }
            else
            {
                return(uPosA);
            }
        }
        if (uPosB >= uPosA)
        {
            if ((uPosA + uLength) >= uPosB)
            {
                ds_snzprintf(pRef->strErrorText, sizeof(pRef->strErrorText), "overflow in _SetDataPtrTry. indexA was %d, indexB was %d, posA was %d, posB was %d, ", uIndexA, uIndexB, pBuffer->IOLookUp[uIndexA].uPos + pBuffer->IOLookUp[uIndexA].uLen, uPosB);

                NetPrintf(("netgamedist: [%p] %s buffer full.\n", pRef, ((pBuffer == &pRef->InpBufData)?"input":"output")));
                return(-1);
            }
            else
            {
                return(uPosA);
            }
        }
    }

    // unreachable code. If we get here something went terribly wrong.
    ds_snzprintf(pRef->strErrorText, sizeof(pRef->strErrorText), "_SetDataPtrTry reached unreachable code.");
    NetPrintf(("netgamedist: [%p] critical failure in _SetDataPtrTry() - unreachable code\n", pRef));

    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function    _SetDataPtr

    \Description
        Prepares the input or output buffer for addition of a packet of length uLength.

    \Input *pRef    - reference pointer
    \Input *pBuffer - buffer pointer
    \Input uLength  - length of the next packet to be stored

    \Output
        uint8_t     - TRUE if a space was found. FALSE if the buffer cannot accomodate uLength

    \Version 02/08/07 (jrainy)
*/
/*************************************************************************************************F*/
static uint8_t _SetDataPtr(NetGameDistRefT *pRef, GameBufferDataT *pBuffer, uint16_t uLength)
{
    int32_t uPos = _SetDataPtrTry(pRef, pBuffer, uLength);

    if (uPos != -1)
    {
        pBuffer->IOLookUp[pBuffer->iEnd].uPos = uPos;
        return(TRUE);
    }

    return(FALSE);
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameDistCheckWindow

    \Description
        Handle the window stretching. If we have too many unacknowledged packets, we start sending less often

    \Input *pRef    - The NetGameDist ref
    \Input iRemain  - Remaining frame time (before window stretching)
    \Input uTick    - Current tick

    \Version 12/21/09 (jrainy)
*/
/*************************************************************************************************F*/
static void _NetGameDistCheckWindow(NetGameDistRefT *pRef, int32_t iRemain, uint32_t uTick)
{
    int32_t iQueue;

    if (!pRef->bRecvMulti)
    {
        // stretch cycle if we are over window
        iQueue = GMDIST_Modulo(pRef->OutBufData.iEnd - GMDIST_Modulo(pRef->InpBufData.iBeg + pRef->iIOOffset, PACKET_WINDOW), PACKET_WINDOW);
        if ((iQueue > pRef->iInputWind) && (iRemain < pRef->iInputRate /2))
        {
            #if TIMING_DEBUG
            // dont show single cycle adjustments
            if (uTick+pRef->iInputRate/2-pRef->uInpNext != 1)
            {
                NetPrintf(("netgamedist: [%p] stretching cycle (que=%d, win=%d, tick=%d, add=%d)\n",
                     pRef, iQueue, pRef->iInputWind, uTick, (uTick+pRef->iInputRate/2)-pRef->uInpNext));
            }
            #endif
            // stretch the next send
            pRef->uInpNext = uTick+pRef->iInputRate /2;
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameDistUpdateSendTime

    \Description
        Update the next send time after sending each packet. In p2p mode, clamp the next send to
        no earlier than half a frame after *now* and no later than two frames after *now*

    \Input *pRef    - The NetGameDist ref

    \Version 12/21/09 (jrainy)
*/
/*************************************************************************************************F*/
static void _NetGameDistUpdateSendTime(NetGameDistRefT *pRef)
{
    uint32_t uTick;
    int32_t iNext;
    // get current time

    uTick = NetTick();
    // record the send time so we can schedule next packet
    pRef->uInpNext += pRef->iInputRate;

    if (!pRef->bRecvMulti)
    {
        // figure time till next send
        iNext = pRef->uInpNext - uTick;
        // clamp the time range to half/double rate
        if (iNext < pRef->iInputRate / 2)
        {
            #if TIMING_DEBUG
            NetPrintf(("send clamping to half (was %d)\n", iNext));
            #endif
            pRef->uInpNext = uTick + pRef->iInputRate /2;
        }
        if (iNext > pRef->iInputRate * 2)
        {
            #if TIMING_DEBUG
            NetPrintf(("send clamping to double (was %d)\n", iNext));
            #endif
            pRef->uInpNext = uTick + pRef->iInputRate *2;
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameDistSendInput

    \Description
        Provide local input data

    \Input *pRef        - reference pointer
    \Input *pBuffer     - controller data
    \Input iLength      - data length
    \Input iLengthSize  - size of length buffer

    \Output
        int32_t         - negative=error (including GMDIST_OVERFLOW=overflow), positive=packet successfully sent or saved to NetGameDist send buffer

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
static int32_t _NetGameDistSendInput(NetGameDistRefT *pRef, void *pBuffer, int32_t iLength, int32_t iLengthSize)
{
    int32_t iNext, iResult, iBeg;
    unsigned char *pData;

    // add packet to queue
    if (pBuffer != NULL)
    {
        // verify length is valid
        if (iLength < 0)
        {
            ds_snzprintf(pRef->strErrorText, sizeof(pRef->strErrorText), "netgamedist: _NetGameDistSendInput with iLength %d.", iLength);
            NetPrintf(("netgamedist: invalid buffer length passed to _NetGameDistSendInput()!\n"));
            return(GMDIST_INVALID);
        }

        // see if room to buffer packet
        iNext = GMDIST_Modulo(pRef->OutBufData.iEnd+1, PACKET_WINDOW);
        // the - 1 steals a spot but prevents:
        // input local prechanging iooffset triggering this
        iBeg = GMDIST_Modulo(pRef->InpBufData.iBeg + pRef->iIOOffset - 1, PACKET_WINDOW);
        if (!pRef->bActAsServer && (iNext == iBeg))
        {
            ds_snzprintf(pRef->strErrorText, sizeof(pRef->strErrorText), "netgamedist: _NetGameDistSendInput with iNext %d and iBeg %d.", iNext, iBeg);
            return(GMDIST_OVERFLOW);
        }

        if (iNext == pRef->OutBufData.iBeg)
        {
            ds_snzprintf(pRef->strErrorText, sizeof(pRef->strErrorText), "netgamedist: _NetGameDistSendInput with iNext %d and OutBufData.iBeg %d.", iNext, pRef->OutBufData.iBeg);
            return(GMDIST_OVERFLOW);
        }

        // point to buffer
        if (!_SetDataPtr(pRef, &pRef->OutBufData, iLength))
        {
            return(GMDIST_OVERFLOW);
        }
        pData = pRef->OutBufData.pControllerIO+pRef->OutBufData.IOLookUp[pRef->OutBufData.iEnd].uPos;

        // copy data into buffer
        pRef->OutBufData.IOLookUp[pRef->OutBufData.iEnd].uLen = iLength;
        pRef->OutBufData.IOLookUp[pRef->OutBufData.iEnd].uLenSize = iLengthSize;
        ds_memcpy(pData, pBuffer, iLength);

        // save down the time 
        pRef->OutBufData.IOLookUp[pRef->OutBufData.iEnd].uInsertTime = NetTick();

        // incorporate into buffer
        pRef->OutBufData.iEnd = iNext;

        // moved from inside the 'while' below
        // this will update the send time on send attempt, not on actual send.
        // i.e. "queue packets at regular interval", not "queue packets so that
        // they send at regular interval", which is not really doable anyway.
        _NetGameDistUpdateSendTime(pRef);
    }

    // try and send packet
    // prevent sending if we have pending metainfo to send as it will affect the format
    while ((pRef->OutBufData.iBeg != pRef->OutBufData.iEnd) && (pRef->uUpdateMetaInfoBeg == pRef->uUpdateMetaInfoEnd))
    {
        // point to buffer
        pData = pRef->OutBufData.pControllerIO+pRef->OutBufData.IOLookUp[pRef->OutBufData.iBeg].uPos;

        // setup a data input packet
        if (pRef->bActAsServer)
        {
            if (pRef->OutBufData.IOLookUp[pRef->OutBufData.iBeg].uLenSize == 2)
            {
                pRef->MaxPkt.head.kind = GAME_PACKET_INPUT_MULTI_FAT;
            }
            else
            {
                pRef->MaxPkt.head.kind = GAME_PACKET_INPUT_MULTI;
            }
        }
        else
        {
            pRef->MaxPkt.head.kind = GAME_PACKET_INPUT;
        }

        pRef->MaxPkt.head.len = pRef->OutBufData.IOLookUp[pRef->OutBufData.iBeg].uLen;
        ds_memcpy(pRef->MaxPkt.body.data, pData, pRef->MaxPkt.head.len);

        // try and send
        iResult = pRef->pSendProc(pRef->pNetGameLinkRef, (NetGamePacketT*)&pRef->MaxPkt, 1);
        if (iResult > 0)
        {
            // all is well -- remove from buffer
            pRef->OutBufData.iBeg = GMDIST_Modulo(pRef->OutBufData.iBeg+1, PACKET_WINDOW);
        }
        else if (iResult < 0)
        {
            ds_snzprintf(pRef->strErrorText, sizeof(pRef->strErrorText), "netgamedist: GMDIST_SENDPROC_FAILED result is %d.", iResult);

            pRef->iErrCond = GMDIST_SENDPROC_FAILED;
            NetPrintf(("netgamedist: sendproc failed in _NetGameDistSendInput()!\n"));
            return(GMDIST_SENDPROC_FAILED);
        }
        else
        {
            // lower-level transport is out of send buffer space at the moment… exit for now and try again later.
            break;
        }
    }

    // packet was either buffered and/or sent
    return(1);
}

/*F*************************************************************************************************/
/*!
    \Function    _ProcessPacketType

    \Description
        Whenever a packet of a given type is received, this function is called
        to do any specific process.

    \Input *pRef        - reference pointer
    \Input uPacketType  - the packet type

    \Version 02/22/07 (jrainy)
*/
/*************************************************************************************************F*/
static void _ProcessPacketType(NetGameDistRefT *pRef, uint8_t uPacketType)
{
    if (!pRef->bActAsServer && ((uPacketType == GAME_PACKET_INPUT_MULTI) || (uPacketType == GAME_PACKET_INPUT_MULTI_FAT)))
    {
        pRef->bRecvMulti = TRUE;
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameDistSendFlowUpdate

    \Description
        Sends a packet to the server enabling or disabling flow control

    \Input *pRef        - reference pointer

    \Version 09/29/09 (jrainy)
*/
/*************************************************************************************************F*/
static void _NetGameDistSendFlowUpdate(NetGameDistRefT *pRef)
{
    int32_t iResult;

    pRef->MaxPkt.head.kind = GAME_PACKET_INPUT_FLOW;
    pRef->MaxPkt.head.len = 7;

    pRef->MaxPkt.body.data[0] = pRef->bRdySend;
    pRef->MaxPkt.body.data[1] = pRef->bRdyRecv;

    pRef->MaxPkt.body.data[2] = pRef->bLocalCRCValid;
    pRef->MaxPkt.body.data[3] = (pRef->uLocalCRC >> 24);
    pRef->MaxPkt.body.data[4] = (pRef->uLocalCRC >> 16);
    pRef->MaxPkt.body.data[5] = (pRef->uLocalCRC >> 8);
    pRef->MaxPkt.body.data[6] = pRef->uLocalCRC;

    pRef->bLocalCRCValid = FALSE;

    iResult = pRef->pSendProc(pRef->pNetGameLinkRef, (NetGamePacketT*)&pRef->MaxPkt, 1);

    if (iResult < 0)
    {
        NetPrintf(("netgamedist: [%p] Flow update failed (error=%d)!\n", pRef, iResult));
        pRef->bUpdateFlowCtrl = FALSE;
    }
    else if (iResult > 0)
    {
        pRef->bUpdateFlowCtrl = FALSE;
    }
    else
    {
        NetPrintf(("netgamedist: [%p] Flow update deferred (error=overflow)!\n", pRef));
        // nothing to do here, the caller - NetGameDistUpdate - will try again later.
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameDistSendMetaInfo

    \Description
        Sends a packet to the client describing the format of upcoming packets

    \Input *pRef        - reference pointer

    \Version 09/29/09 (jrainy)
*/
/*************************************************************************************************F*/
static void _NetGameDistSendMetaInfo(NetGameDistRefT *pRef)
{
    int32_t iResult;
    NetGameDistMetaInfoT *pMetaInfo;

    while(pRef->uUpdateMetaInfoBeg != pRef->uUpdateMetaInfoEnd)
    {
        pMetaInfo = &pRef->aMetaInfoToSend[pRef->uUpdateMetaInfoBeg];

        pRef->MaxPkt.head.kind = GAME_PACKET_INPUT_META;
        pRef->MaxPkt.head.len = 5;

        pRef->MaxPkt.body.data[0] = pMetaInfo->uVer;
        pRef->MaxPkt.body.data[1] = pMetaInfo->uMask >> 24;
        pRef->MaxPkt.body.data[2] = pMetaInfo->uMask >> 16;
        pRef->MaxPkt.body.data[3] = pMetaInfo->uMask >> 8;
        pRef->MaxPkt.body.data[4] = pMetaInfo->uMask;

        iResult = pRef->pSendProc(pRef->pNetGameLinkRef, (NetGamePacketT*)&pRef->MaxPkt, 1);

        if (iResult < 0)
        {
            NetPrintf(("netgamedist: [%p] Meta info update failed (error=%d)!\n", pRef, iResult));
            pRef->uUpdateMetaInfoBeg = GMDIST_Modulo(pRef->uUpdateMetaInfoBeg + 1, GMDIST_META_ARRAY_SIZE);
        }
        else if (iResult > 0)
        {
            NetPrintf(("netgamedist: [%p] Meta info update sent %d 0x%08x !\n", pRef, pMetaInfo->uVer, pMetaInfo->uMask));
            pRef->uUpdateMetaInfoBeg = GMDIST_Modulo(pRef->uUpdateMetaInfoBeg + 1, GMDIST_META_ARRAY_SIZE);
        }
        else
        {
            NetPrintf(("netgamedist: [%p] Meta info update deferred (error=overflow)!\n", pRef));
            // nothing to do here, the caller - NetGameDistUpdate - will try again later.

            break;
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameDistCountBits

    \Description
        Count the number of bits set in a given uint32_t variable

    \Input *pRef        - reference pointer
    \Input uMask        - mask

    \Output
        uint32_t        - the number of bits set

    \Version 09/26/09 (jrainy)
*/
/*************************************************************************************************F*/
static uint32_t _NetGameDistCountBits(NetGameDistRefT *pRef, uint32_t uMask)
{
    uint32_t uCount = 0;

    while(uMask)
    {
        uCount += (uMask & 1);
        uMask /= 2;
    };

    return(uCount);
}

#if DIRTYCODE_LOGGING
/*F*************************************************************************************************/
/*!
    \Function    _NetGameDistInputCheckLog

    \Description
        Logs the values returned by NetGameDistInputCheck.

    \Input *pRef        - reference pointer
    \Input *pSend       - the pointer to pSend the client passed to InputCheck
    \Input *pRecv       - the pointer to pRecv the client passed to InputCheck

    \Version 12/21/09 (jrainy)
*/
/*************************************************************************************************F*/
static void _NetGameDistInputCheckLog(NetGameDistRefT *pRef, int32_t *pSend, int32_t *pRecv)
{
    uint32_t uCurrentTick = NetTick();
    uint32_t uDelay;

    // Calculate delay since last log
    uDelay = NetTickDiff(uCurrentTick, pRef->uLastInputCheckLogTick);

    // Check if condition to display the trace is met.
    // Condition is: meaningful send/recv info ready OR skip timeout expire
    if ( (pSend && (*pSend==0)) || (pRecv && (*pRecv!=0)) ||     // check meaningful send/recv
         (uDelay >= INPUTCHECK_LOGGING_DELAY) )                    // check skip timeout
    {
        if (pSend && pRecv)
        {
            NetPrintfVerbose((pRef->uVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [%p] exiting NetGameDistInputCheck() tick=%d, *pSend=%d, *pRecv=%d logskipped_count=%d\n",
                pRef, uCurrentTick, *pSend, *pRecv, pRef->uSkippedInputCheckLogCount));
        }
        else if (pSend && !pRecv)
        {
            NetPrintfVerbose((pRef->uVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [%p] exiting NetGameDistInputCheck() tick=%d, *pSend=%d, pRecv=NULL logskipped_count=%d\n",
                pRef, uCurrentTick, *pSend, pRef->uSkippedInputCheckLogCount));
        }
        else if (!pSend && pRecv)
        {
            NetPrintfVerbose((pRef->uVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [%p] exiting NetGameDistInputCheck() tick=%d, pSend=NULL, *pRecv=%d logskipped_count=%d\n",
                pRef, uCurrentTick, *pRecv, pRef->uSkippedInputCheckLogCount));
        }
        else
        {
            NetPrintfVerbose((pRef->uVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [%p] exiting NetGameDistInputCheck() tick=%d, pSend=NULL, pRecv=NULL logskipped_count=%d\n",
                pRef, uCurrentTick, pRef->uSkippedInputCheckLogCount));
        }

        // Re-initialize variables used to skip some logs
        pRef->uSkippedInputCheckLogCount = 0;
        pRef->uLastInputCheckLogTick = uCurrentTick;
    }
    else
    {
        pRef->uSkippedInputCheckLogCount++;
    }
}
#endif

/*F*************************************************************************************************/
/*!
    \Function    _NetGameDistInputPeekRecv

    \Description
        Behaves just as a receive function, taking data from netgamelink. However, we peek first and
        if an overflow is detected, we do not take the packet from netgamelink. In this case, we
        return 0, as if doing was ready to be received

    \Input *pRef     - reference pointer
    \Input *pBuf     - buffer to receive into
    \Input iLen      - available length in buf

    \Output
        int32_t      - recvproc return value, or forced to 0 if we'd be in overflow

    \Version 08/15/11 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _NetGameDistInputPeekRecv(NetGameDistRefT *pRef, NetGamePacketT *pBuf, int32_t iLen)
{
    NetGamePacketT* pPeekPacket = NULL;

    if (pRef->pPeekProc)
    {
        uint32_t uDistMask = (1 << GAME_PACKET_INPUT) |
                             (1 << GAME_PACKET_INPUT_MULTI) |
                             (1 << GAME_PACKET_INPUT_MULTI_FAT);

        (*pRef->pPeekProc)(pRef->pNetGameLinkRef, &pPeekPacket, uDistMask);

        // ok, we have a mean to see what is coming, and we got something.
        if (pPeekPacket)
        {
            // let's check if we have space for it.
            // for completeness, we may check buffer overflow on slots and dropproc, but this seems overkill
            if (_SetDataPtrTry(pRef, &pRef->InpBufData, pPeekPacket->head.len + 1) < 0)
            {
                NetPrintf(("netgamedist: [%p] not receiving from link layer a packet of type %d and length %d because our buffer is full\n", pRef, pPeekPacket->head.kind, pPeekPacket->head.len));
                // act as if the recvproc had nothing.
                return(0);
            }
        }
    }

    return((*pRef->pRecvProc)(pRef->pNetGameLinkRef, pBuf, iLen, TRUE));
}
/*** Public Functions ******************************************************************/

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistCreate

    \Description
        Create the game pClient

    \Input *pNetGameLinkRef - netgamelink module ref
    \Input *pStatProc       - netgamelink stat callback
    \Input *pSendProc       - game send func
    \Input *pRecvProc       - game recv func
    \Input uInBufferSize    - input buffer size, plan around PACKET_WINDOW*n*packets
    \Input uOutBufferSize   - output buffer size, plan around PACKET_WINDOW*packets

    \Output
        NetGameDistRefT * - pointer to module state

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
NetGameDistRefT *NetGameDistCreate(void *pNetGameLinkRef, NetGameDistStatProc *pStatProc, NetGameDistSendProc *pSendProc, NetGameDistRecvProc *pRecvProc, uint32_t uInBufferSize, uint32_t uOutBufferSize )
{
    NetGameDistRefT *pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pRef = DirtyMemAlloc(sizeof(*pRef), NETGAMEDIST_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netgamedist: [%p] unable to allocate module state\n", pRef));
        return(NULL);
    }
    ds_memclr(pRef, sizeof(*pRef));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;
    pRef->iLastSentDelta = -1;

    // Allocate the input and output buffers
    pRef->InpBufData.pControllerIO = DirtyMemAlloc( uInBufferSize, NETGAMEDIST_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData );
    pRef->OutBufData.pControllerIO = DirtyMemAlloc( uOutBufferSize, NETGAMEDIST_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData );

    ds_memclr(pRef->InpBufData.pControllerIO, uInBufferSize);
    ds_memclr(pRef->OutBufData.pControllerIO, uOutBufferSize);

    pRef->InpBufData.uBufLen = uInBufferSize;
    pRef->OutBufData.uBufLen = uOutBufferSize;

    // save the link layer callbacks
    pRef->pNetGameLinkRef = pNetGameLinkRef;
    pRef->pStatProc = pStatProc;
    pRef->pSendProc = pSendProc;
    pRef->pRecvProc = pRecvProc;

    // set default controller exchange rate
    pRef->iInputRate = 50;
    // set the defaults
    pRef->iInputMini = 1;
    pRef->iInputMaxi = 10;

    // Defaults to two players so that the non-multi version behaves as it used to.
    pRef->uTotalPlyrs = 2;

    // Set default verbosity level
    pRef->uVerbose = 1;

    // init check tick counter
    pRef->uLastInputCheckLogTick = NetTick();

    NetPrintf(("netgamedist: module created with PACKET_WINDOW = %d\n", PACKET_WINDOW));

    return(pRef);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistDestroy

    \Description
        Destroy the game pClient

    \Input *pRef     - reference pointer

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
void NetGameDistDestroy(NetGameDistRefT *pRef)
{
    DirtyMemFree( pRef->InpBufData.pControllerIO, NETGAMEDIST_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData );
    DirtyMemFree( pRef->OutBufData.pControllerIO, NETGAMEDIST_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData );
    pRef->InpBufData.pControllerIO = NULL;
    pRef->OutBufData.pControllerIO = NULL;

    // free our memory
    DirtyMemFree(pRef, NETGAMEDIST_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);

}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistStatus

    \Description
        Get status information

    \Input *pRef                     - reference pointer
    \Input iSelect                   - selector
    \Input iValue                    - input value
    \Input *pBuf                     - output buffer
    \Input iBufSize                  - output buffer size

    \Output
        int32_t                      - selector specific return value

    \Notes
        This is read-only data which can be read at any time.  This reference becomes
        invalid when the module is destroyed.

        The count of [?snd..?out] is the number of packets ready to send.  The min of
        [?cmp..?inp] and [?cmp..?out] is the number of packets ready to process (because
        you need a packet from each peer in order to perform processing).

        Selectors are:

    \verbatim
        SELECTOR    RETURN RESULT
        'dcnt'      returns the total inbound dequeued count
        'drop'      returns the total input packet dropped
        'icnt'      returns the total inbound packet count
        'late'      returns the latency from the link stats
        'mult'      returns the number of players and writes their stats to pBuf
        'ocnt'      returns the total outbound packet count
        'pcnt'      returns the total packet processed count (full round trip)
        'plat'      returns the end-to-end latency in number of packets
        'prti'      returns the total packet processed time (full round trip)
        'pwin'      returns PACKET_WINDOW configured for netgamedist
        'qver'      returns the last queried packet version
        'rate'      returns current exchange rate
        'rcrc'      if iValue==0 returns whether we have a value otherwise returns the value itself
        'rrcv'      returns if remote is ready to receive or not
        'rsnd'      returns if remote is ready to send or not
        'stat'      returns the link stats via pBuf
        'wait'      returns total inbound packet wait time;
        'wind'      returns the current exchange window
        '?cmp'      returns the offset of first packet to process in input buffer
        '?cws'      returns if we can send. FALSE if sending would overflow the send queue, iValue is length
        '?inp'      returns the offset of last packet to process in input buffer
        '?out'      returns the offset of last packet to send in output buffer
        '?snd'      returns the offset of first packet to send in output buffer
    \endverbatim

        Unhandled selectors are passed on to NetGameLink

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
int32_t NetGameDistStatus(NetGameDistRefT *pRef, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'dcnt')
    {
        return(pRef->iInboundDequeueCnt);
    }
    if (iSelect == 'drop')
    {
        return(pRef->iInboundDropPktCnt);
    }
    if (iSelect == 'icnt')
    {
        return(pRef->iInboundPktCnt);
    }
    if (iSelect == 'late')
    {
        return(pRef->NetGameLinkStats.late);
    }
    if (iSelect == 'mult')
    {
        // Make user-provide buffer is large enough to receive a pointer
        if ((pBuf != NULL) && (iBufSize >= (int32_t)(sizeof(NetGameDistStatT) * pRef->uTotalPlyrs)))
        {
            ds_memcpy(pBuf, pRef->aRecvStats, sizeof(NetGameDistStatT) * pRef->uTotalPlyrs);
            return(pRef->uTotalPlyrs);
        }
        else
        {
            // unhandled
            return(-1);
        }
    }
    if (iSelect == 'ocnt')
    {
        return(pRef->iOutboundPktCnt);
    }
    if (iSelect == 'plat')
    {
        return(GMDIST_Modulo(pRef->OutBufData.iEnd - GMDIST_Modulo(pRef->InpBufData.iBeg + pRef->iIOOffset, PACKET_WINDOW), PACKET_WINDOW));
    }
    if (iSelect == 'pcnt')
    {
        return(pRef->iDistProcCnt);
    }
    if (iSelect == 'prti')
    {
        return(pRef->iDistProcTimeTotal);
    }
    if (iSelect == 'pwin')
    {
        return(PACKET_WINDOW);
    }
    if (iSelect == 'qver')
    {
        return(pRef->uLastQueriedVersion);
    }
    if (iSelect == 'rate')
    {
        return(pRef->iInputRate);
    }
    if (iSelect == 'rcrc')
    {
        if (iValue)
        {
            int32_t ret = pRef->uRemoteCRC;
            pRef->uRemoteCRC = 0;
            pRef->bRemoteCRCValid = FALSE;
            return(ret);
        }
        else
        {
            return(pRef->bRemoteCRCValid);
        }
    }
    if (iSelect == 'rrcv')
    {
        return(pRef->bRdyRecvRemote);
    }
    if (iSelect == 'rsnd')
    {
        return(pRef->bRdySendRemote);
    }
    if (iSelect == 'stat')
    {
        (pRef->pStatProc)(pRef->pNetGameLinkRef, iSelect, iValue, &pRef->NetGameLinkStats, sizeof(NetGameLinkStatT));

        // Make user-provide buffer is large enough to receive a pointer
        if ((pBuf != NULL) && (iBufSize >= (int32_t)sizeof(NetGameLinkStatT)))
        {
            ds_memcpy(pBuf, &pRef->NetGameLinkStats, sizeof(NetGameLinkStatT));
            return(0);
        }
        else
        {
            // unhandled
            return(-1);
        }
    }
    if (iSelect == 'wait')
    {
        return(pRef->iWaitTimeTotal);
    }
    if (iSelect == 'wind')
    {
        return(pRef->iInputWind);
    }
    if (iSelect == '?cmp')
    {
        return(pRef->InpBufData.iBeg);
    }
    if (iSelect == '?cws')
    {
        int32_t iNext;
        if (iValue < 0)
        {
            return(FALSE);
        }

        iNext = GMDIST_Modulo(pRef->OutBufData.iEnd +1, PACKET_WINDOW);

        // mclouatre April 26th 2018 - unclear to me what the -1 is for in the condition below
        if (!pRef->bActAsServer && (iNext == GMDIST_Modulo(pRef->InpBufData.iBeg + pRef->iIOOffset - 1, PACKET_WINDOW)))
        {
            ds_snzprintf(pRef->strErrorText, sizeof(pRef->strErrorText), "'?cws' failure while comparing indices");
            return(FALSE);
        }
        if (_SetDataPtrTry(pRef, &pRef->OutBufData, iValue) == -1)
        {
            return(FALSE);
        }

        return(TRUE);
    }
    if (iSelect == '?inp')
    {
        return(pRef->InpBufData.iEnd);
    }
    if (iSelect == '?out')
    {
        return(pRef->OutBufData.iEnd);
    }
    if (iSelect == '?snd')
    {
        return(pRef->OutBufData.iBeg);
    }

    // fallthrough to NetGameLinkStatus
    return((pRef->pStatProc)(pRef->pNetGameLinkRef, iSelect, iValue, pBuf, iBufSize));
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistSetServer

    \Description
        Get pointer to status counters

    \Input *pRef            - reference pointer
    \Input bActAsServer     - boolean, whether NetGameDist should send multi-packets

    \Version 02/26/07 (jrainy)
*/
/*************************************************************************************************F*/
void NetGameDistSetServer(NetGameDistRefT *pRef, uint8_t bActAsServer)
{
    // the trigraph is there to make sure we don't store non-{0,1} value in.
    pRef->bActAsServer = bActAsServer ? TRUE : FALSE;
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistMultiSetup

    \Description
        Tells NetGameDist how many players there are in the game, and which one we are.
        Will only affect the order packets are received from NetGameDistInputQueryMulti.

    \Input *pRef            - netgamelink module ref
    \Input iDistIndex       - which player we are
    \Input iTotPlrs         - total number of players

    \Version 02/08/07 (jrainy)
*/
/*************************************************************************************************F*/
void NetGameDistMultiSetup(NetGameDistRefT *pRef, int32_t iDistIndex, int32_t iTotPlrs)
{
    NetPrintf(("netgamedist: [%p] NetGameDistMultiSetup index: %d, total players: %d\n", pRef, iDistIndex, iTotPlrs));

    pRef->uTotalPlyrs = iTotPlrs;
    pRef->uDistIndex = iDistIndex;
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistMetaSetup

    \Description
        Tells NetGameDist whether to enable meta-information sending, and
        sets the mask & the version number for the meta-information.

    \Input *pRef            - netgamelink module ref
    \Input bSparse          - enable or disable meta-information sending (disabling after enabling is currently not-supported on the client)
    \Input uMask            - set the mask for the meta-information
    \Input uVersion         - set the version number for the meta-information.

    \Version 08/29/11 (szhu)
*/
/*************************************************************************************************F*/
void NetGameDistMetaSetup(NetGameDistRefT *pRef, uint8_t bSparse, uint32_t uMask, uint32_t uVersion)
{
    NetPrintf(("netgamedist: [%p] NetGameDistMetaSetup Enabled: %s, Mask: 0x%x, Version: %d\n",
               pRef, bSparse?"TRUE":"FALSE", uMask, uVersion));

    pRef->bSparse = bSparse;
    pRef->aMetaInfoToSend[pRef->uUpdateMetaInfoEnd].uMask = uMask;
    pRef->aMetaInfoToSend[pRef->uUpdateMetaInfoEnd].uPlayerCount = _NetGameDistCountBits(pRef, uMask);
    pRef->aMetaInfoToSend[pRef->uUpdateMetaInfoEnd].uVer = GMDIST_Modulo(uVersion, GMDIST_META_ARRAY_SIZE);
    pRef->uUpdateMetaInfoEnd = GMDIST_Modulo(pRef->uUpdateMetaInfoEnd + 1, GMDIST_META_ARRAY_SIZE);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistInputPeek

    \Description
        See if a completed packet is ready

    \Input *pRef    - reference pointer
    \Input *pType   - will be filled with data type
    \Input *pPeer   - buffer sent by peer
    \Input *pPlen   - length of data in peer buffer. Must pass in the length of pPeer

    \Output
        int32_t     - zero=no data pending, negative=error, positive=data returned

    \Version 02/05/2007 (JLB)
*/
/*************************************************************************************************F*/
int32_t NetGameDistInputPeek(NetGameDistRefT *pRef, uint8_t *pType, void *pPeer, int32_t *pPlen)
{
    uint8_t *pPsrc;
    int32_t iLength;
    uint8_t uInputKind;
    int16_t iLengthSize = 1;
    uint8_t uOffsetLengths;
    uint8_t uOffsetBuffer;
    uint8_t uOffsetTypes;
    uint8_t uOffsetVersion;
    uint16_t uPlayerCount = pRef->uTotalPlyrs;
    uint8_t uVer = 0;


    // if no remote data, do an update
    if (pRef->InpBufData.iBeg == pRef->InpBufData.iEnd)
    {
        NetGameDistUpdate(pRef);
    }

    // no packets from peer?
    if (pRef->InpBufData.iBeg == pRef->InpBufData.iEnd)
    {
        return(0);
    }

    // point to data buffer
    pPsrc = pRef->InpBufData.pControllerIO+pRef->InpBufData.IOLookUp[pRef->InpBufData.iBeg].uPos;

    uInputKind = pPsrc[pRef->InpBufData.IOLookUp[pRef->InpBufData.iBeg].uLen - 1];

    if (uInputKind == GAME_PACKET_INPUT_MULTI_FAT)
    {
        iLengthSize = 2;
    }

    // find the location to lookup the version
    uOffsetVersion = pRef->bRecvMulti ? 1 : 0;

    // extract the version number of the packet, and player count, if the server ever gave us metadata
    if (pRef->bGotMetaInfo && pRef->bRecvMulti)
    {
        uVer = GMDIST_Modulo(pPsrc[uOffsetVersion], GMDIST_META_ARRAY_SIZE);
        uPlayerCount = pRef->aMetaInfo[uVer].uPlayerCount;
        pRef->uLastQueriedVersion = uVer;
    }

    // always pack packets as if there was at least 2 players (to avoid negatively-sized fields)
    if (pRef->bRecvMulti && (pRef->uTotalPlyrs == 1))
    {
        uPlayerCount = 2;
    }

    // compute offsets to various parts
    uOffsetTypes = uOffsetVersion + (pRef->bGotMetaInfo ? 1 : 0);
    uOffsetLengths = uOffsetTypes + (pRef->bRecvMulti ? (uPlayerCount / 2) : 1);
    uOffsetBuffer = uOffsetLengths + (pRef->bRecvMulti ? (iLengthSize * (uPlayerCount - 2)) : 0);

    // copy over the data, -1 to skip the packet kind tacked at the end
    iLength = pRef->InpBufData.IOLookUp[pRef->InpBufData.iBeg].uLen - uOffsetBuffer - 1;
    if (pPeer != NULL)
    {
        //check the passed-in length to prevent overflow
        if (iLength > *pPlen)
        {
            ds_snzprintf(pRef->strErrorText, sizeof(pRef->strErrorText), "NetGameDistInputPeek error. length is %d, *pLen is %d.", iLength, *pPlen);

            // fail if the buffer is insufficient
            *pPlen = iLength;
            return(GMDIST_OVERFLOW);
        }

        ds_memcpy(pPeer, pPsrc + uOffsetBuffer, iLength);
    }

    *pPlen = iLength;

    if (pType != NULL)
    {
        *pType = *(pPsrc + (pRef->bRecvMulti ? 1 : 0));
    }

    #if DIRTYCODE_LOGGING
    NetPrintfVerbose((pRef->uVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [%p] peeked type %d len %d\n", pRef, pType?*pType:0, pPlen?*pPlen:0));
    #endif

    // return success
    return(pRef->aPacketId[pRef->InpBufData.iBeg]);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistInputQuery

    \Description
        See if a completed packet is ready

    \Input *pRef    - reference pointer
    \Input *pOurs   - buffer previously sent with NetGameDistInputLocal
    \Input *pOlen   - length of data in ours buffer
    \Input *pPeer   - buffer sent by peer
    \Input *pPlen   - length of data in peer buffer

    \Output
        int32_t     - zero=no data pending, negative=error, positive=data returned

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
int32_t NetGameDistInputQuery(NetGameDistRefT *pRef, void *pOurs, int32_t *pOlen, void *pPeer, int32_t *pPlen)
{
    void* aInputs[2];
    int32_t aLengths[2];
    uint8_t aTypes[2];
    int32_t iRet;

    if ( pRef->uTotalPlyrs != 2 )
    {
        return(GMDIST_BADSETUP);
    }

    // for a 2 player setup our index is always 0 and the peer is always at 1
    aInputs[0] = pOurs;
    aInputs[1] = pPeer;

    iRet = NetGameDistInputQueryMulti(pRef, aTypes, aInputs, aLengths);

    *pOlen = aLengths[0];
    *pPlen = aLengths[1];

    return(iRet);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistInputQueryMulti

    \Description
        See if a completed packet is ready

    \Input *pRef        - reference pointer
    \Input *pDataTypes  - array of data types (GMDIST_DATA_NONE, GMDIST_DATA_INPUT, GMDIST_DATA_INPUT_DROPPABLE, GMDIST_DATA_DISCONNECT).
    \Input **ppInputs   - array of pointers to inputs to receive.
    \Input *pLen        - array of lengths from the data received.

    \Output
        int32_t         - zero=no data pending, negative=error, positive=data returned

    \Version 02/08/07 (jrainy)
*/
/*************************************************************************************************F*/
int32_t NetGameDistInputQueryMulti(NetGameDistRefT *pRef, uint8_t *pDataTypes, void **ppInputs, int32_t *pLen)
{
    unsigned char *pOsrc;
    uint32_t uIndex, uCount, uPos, uRecvLen;
    unsigned char *pRecvBuf;
    uint8_t *pLengths;
    uint8_t *pTypePos;
    uint8_t uDelta;
    uint8_t uOffsetLengths;
    uint8_t uOffsetBuffer;
    uint8_t uOffsetTypes;
    uint8_t uOffsetVersion;
    uint32_t uOurInp;
    uint16_t uPlayerCount = pRef->uTotalPlyrs;
    uint8_t bCRCRequest = FALSE;
    uint8_t uVer = 0;

    // if waiting on remote data, do an update
    if (pRef->InpBufData.iBeg == pRef->InpBufData.iEnd)
    {
        NetGameDistUpdate(pRef);
    }

    // check for inputs ready to process; in 2p mode we also require a non-empty output buffer to pair with
    if ((pRef->InpBufData.iBeg == pRef->InpBufData.iEnd) || (!pRef->bActAsServer && !pRef->bRecvMulti && (GMDIST_Modulo(pRef->InpBufData.iBeg + pRef->iIOOffset, PACKET_WINDOW) == pRef->OutBufData.iEnd)))
    {
        #if DIRTYCODE_LOGGING
        NetPrintfVerbose((pRef->uVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [%p] exiting NetGameDistInputQueryMulti() retval=0 tick=%d len=n/a type=n/a\n", pRef, NetTick()));
        #endif

        return(0);
    }

    if (ppInputs)
    {
        // access the first byte after the received data
        uint8_t* pPacketKind = pRef->InpBufData.pControllerIO + pRef->InpBufData.IOLookUp[pRef->InpBufData.iBeg].uPos + pRef->InpBufData.IOLookUp[pRef->InpBufData.iBeg].uLen - 1;
        int16_t iLengthSize = 1;

        if (*pPacketKind == GAME_PACKET_INPUT_MULTI_FAT)
        {
            iLengthSize = 2;
        }

        // Go through the received packet and fill all the input data except ours, then copy our own from outlkup

        uCount = 0;
        uPos = 0;
        uRecvLen = pRef->InpBufData.IOLookUp[pRef->InpBufData.iBeg].uLen - 1;
        pRecvBuf = pRef->InpBufData.pControllerIO+pRef->InpBufData.IOLookUp[pRef->InpBufData.iBeg].uPos;
        uDelta = pRef->bRecvMulti ? pRecvBuf[0] : 1;

        uOffsetVersion = pRef->bRecvMulti ? 1 : 0;

        // extract the version number of the packet, and player count, if the server ever gave us metadata
        if (pRef->bGotMetaInfo && pRef->bRecvMulti)
        {
            uVer = GMDIST_Modulo(((char *)pRecvBuf)[uOffsetVersion], GMDIST_META_ARRAY_SIZE);
            uPlayerCount = pRef->aMetaInfo[uVer].uPlayerCount;
            pRef->uLastQueriedVersion = uVer;
        }

        // always pack packets as if there was at least 2 players (to avoid negatively-sized fields)
        if (pRef->bRecvMulti && (uPlayerCount == 1))
        {
            uPlayerCount = 2;
        }

        // compute offsets to various parts
        uOffsetTypes = uOffsetVersion + (pRef->bGotMetaInfo ? 1 : 0);
        uOffsetLengths = uOffsetTypes + (pRef->bRecvMulti ? (uPlayerCount / 2) : 1);
        uOffsetBuffer = uOffsetLengths + (pRef->bRecvMulti ? (iLengthSize * (uPlayerCount - 2)) : 0);

        pLengths = pRecvBuf + uOffsetLengths;
        pTypePos = pRecvBuf + uOffsetTypes;
        pRecvBuf += uOffsetBuffer;

        for (uIndex = 0; uIndex < pRef->uTotalPlyrs; uIndex++)
        {
            /* if this is not our own input and we are either
               sending for everyone
               or this is a player we are including in the current sparse multi-packet
               then, add it in */
            if (uIndex != pRef->uDistIndex)
            {
                if (!pRef->bGotMetaInfo || (pRef->aMetaInfo[uVer].uMask & (1 << uIndex)))
                {
                    /* compute the length of packet for player i.
                       last length is known (ours) second to last is implicit (total - known ones) */
                    if (uCount == (uPlayerCount - 2u))
                    {
                        pLen[uIndex] = uRecvLen - uPos - uOffsetBuffer;
                    }
                    else
                    {
                        if (iLengthSize == 2)
                        {
                            pLen[uIndex] = (pLengths[uCount * iLengthSize] * 256) + pLengths[uCount * iLengthSize + 1];
                        }
                        else
                        {
                            pLen[uIndex] = pLengths[uCount];
                        }
                    }

                    // prevent invalid sizes if bogus data is received
                    if (pLen[uIndex] < 0)
                    {
                        pLen[uIndex] = 0;
                    }

                    // clamp the total read length to the received buffer, to prevent overflow.
                    if (pLen[uIndex] + uPos + uOffsetBuffer > uRecvLen)
                    {
                        NetPrintfVerbose((pRef->uVerbose, 0, "netgamedist: Warning ! Buffer overrun. Packet trimmed.\n"));
                        pLen[uIndex] = uRecvLen - uPos - uOffsetBuffer;
                    }

                    // compute the type of packet for player i. (nibbles)
                    if (uCount % 2)
                    {
                        pDataTypes[uIndex] = *pTypePos >> 4;
                        pTypePos++;
                    }
                    else
                    {
                        pDataTypes[uIndex] = *pTypePos & 0x0f;
                    }

                    // copy the packet for player i.
                    if (((pDataTypes[uIndex] & ~GMDIST_DATA_CRC_REQUEST) == GMDIST_DATA_INPUT) ||
                        ((pDataTypes[uIndex] & ~GMDIST_DATA_CRC_REQUEST) == GMDIST_DATA_INPUT_DROPPABLE))
                    {
                        ds_memcpy(ppInputs[uIndex], pRecvBuf + uPos, pLen[uIndex]);
                        uPos += pLen[uIndex];
                    }
                    else
                    {
                        pLen[uIndex] = 0;
                    }

                    // if crc-checking is disabled locally, clear the "crc request" bit
                    if (!pRef->bCRCChallenges)
                    {
                        pDataTypes[uIndex] &= ~GMDIST_DATA_CRC_REQUEST;
                    }

                    /* set a local variable if any of the input has a crc request in.
                       this is to set the "crc request" bit in local input too, to ease our customers job. */
                    if (pDataTypes[uIndex] & GMDIST_DATA_CRC_REQUEST)
                    {
                        bCRCRequest = TRUE;
                    }

                    uCount++;
                }
                else
                {
                    pDataTypes[uIndex] = GMDIST_DATA_NODATA;
                    pLen[uIndex] = 0;
                }
            }
        }

        if (uDelta)
        {
            uint8_t uDropIndex;

            // update dist process times for dropped inputs (inputs dropped are considered processed)
            for (uDropIndex = 0; uDropIndex < (uDelta - 1) ; uDropIndex++, pRef->iDistProcCnt++)
            {
                uint32_t uCurInp = GMDIST_Modulo(pRef->InpBufData.iBeg + uDropIndex, PACKET_WINDOW);
                pRef->iDistProcTimeTotal += NetTickDiff(NetTick(), pRef->OutBufData.IOLookUp[uCurInp].uInsertTime);
            }

            // point to data buffers. Our local storage has just 1-byte type field
            pRef->iIOOffset += (uDelta - 1);
            uOurInp = GMDIST_Modulo(pRef->InpBufData.iBeg + pRef->iIOOffset, PACKET_WINDOW);
            pOsrc = pRef->OutBufData.pControllerIO + pRef->OutBufData.IOLookUp[uOurInp].uPos + 1;

            // copy over the data
            if (pRef->OutBufData.IOLookUp[uOurInp].uLen != 0)
            {
                pLen[pRef->uDistIndex] = pRef->OutBufData.IOLookUp[uOurInp].uLen - 1;
                ds_memcpy(ppInputs[pRef->uDistIndex], pOsrc, pLen[pRef->uDistIndex]);
                pRef->iDistProcTimeTotal += NetTickDiff(NetTick(), pRef->OutBufData.IOLookUp[uOurInp].uInsertTime);
                pRef->iDistProcCnt++;
                pDataTypes[pRef->uDistIndex] = *(pRef->OutBufData.pControllerIO + pRef->OutBufData.IOLookUp[uOurInp].uPos);
            }
            else
            {
                NetPrintf(("netgamedist: detected invalid pairing index between input and output queue; ignoring"));
                pLen[pRef->uDistIndex] = 0;
                pDataTypes[pRef->uDistIndex] = GMDIST_DATA_NONE;
            }
        }
        else
        {
            pRef->iIOOffset = GMDIST_Modulo(pRef->iIOOffset - 1, PACKET_WINDOW);
            pDataTypes[pRef->uDistIndex] = GMDIST_DATA_NONE;
            pLen[pRef->uDistIndex] = 0;
        }

        if (bCRCRequest)
        {
            pDataTypes[pRef->uDistIndex] |= GMDIST_DATA_CRC_REQUEST;
        }
    }

    // calculate time in queue and add it to the total
    pRef->iWaitTimeTotal += NetTickDiff(NetTick(), pRef->InpBufData.IOLookUp[pRef->InpBufData.iBeg].uInsertTime);
    pRef->iInboundDequeueCnt++;

    // return item from buffer
    pRef->InpBufData.iBeg = GMDIST_Modulo(pRef->InpBufData.iBeg + 1, PACKET_WINDOW);
    // update global sequence number
    pRef->uGlobalSeq += 1;

    #if DIRTYCODE_LOGGING
    NetPrintfVerbose((pRef->uVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [%p] exiting NetGameDistInputQueryMulti() retval=%d tick=%d\n", pRef, pRef->uGlobalSeq, NetTick()));
    NetPrintfVerbose((pRef->uVerbose, NETGAMEDIST_VERBOSITY, "  ==== Dumping player-specific data returned in [OUT] params\n"));
    if (ppInputs)
    {
        for (uIndex = 0; uIndex < pRef->uTotalPlyrs; uIndex++)
        {
            uint8_t uDataType = (pDataTypes[uIndex] & ~GMDIST_DATA_CRC_REQUEST);
            NetPrintfVerbose((pRef->uVerbose, NETGAMEDIST_VERBOSITY, "    == Player %u --> len=%d type=%s\n", uIndex, pLen[uIndex],
                _strNetGameDistDataTypes[(((uDataType >= GMDIST_DATA_NONE) && (uDataType <= GMDIST_DATA_NODATA)) ? uDataType : 0)]));
        }
    }
    NetPrintfVerbose((pRef->uVerbose, NETGAMEDIST_VERBOSITY, "  ==== End of player-specific data dump\n"));
    #endif

    // return the sequence number
    return(pRef->uGlobalSeq);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistInputLocal

    \Description
        Provide local input data

    \Input *pRef    - reference pointer
    \Input *pBuffer - controller data
    \Input iLength  - data length

    \Output
        int32_t     - negative=error (including GMDIST_OVERFLOW=overflow), positive=successfully sent or queued

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
int32_t NetGameDistInputLocal(NetGameDistRefT *pRef, void *pBuffer, int32_t iLength)
{
    if (!pBuffer)
    {
        return(_NetGameDistSendInput(pRef, NULL, iLength, 0));
    }
    pRef->aMultiBuf[0] = GMDIST_DATA_INPUT;
    ds_memcpy(pRef->aMultiBuf + 1, pBuffer, iLength);
    return(_NetGameDistSendInput(pRef, pRef->aMultiBuf, iLength + 1, 1));
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistInputLocalMulti

    \Description
        Provide local input data for all players, including ours, which will be discarded

    \Input *pRef            - reference pointer
    \Input *pTypesArray     - array of data types (GMDIST_DATA_INPUT_DROPPABLE, GMDIST_DATA_INPUT)
    \Input **ppBuffer       - array of controller data
    \Input *pLengthsArray   - array of data length
    \Input iDelta           - game team should pass in 1, other values only usable on server

    \Output
        int32_t             - negative=error (including GMDIST_OVERFLOW*=overflow), positive=successfully sent or queued

    \Version 02/08/07 (jrainy)
*/
/*************************************************************************************************F*/
int32_t NetGameDistInputLocalMulti(NetGameDistRefT *pRef, uint8_t *pTypesArray, void **ppBuffer, int32_t *pLengthsArray, int32_t iDelta)
{
    uint32_t uIndex, uPos, uCount;
    uint8_t uOffsetLengths;
    uint8_t uOffsetBuffer;
    uint8_t uOffsetTypes;
    uint8_t uOffsetVersion;
    uint8_t *pLengths;
    uint8_t *pTypePos;
    uint8_t uOurType, uType;
    uint32_t uSendSize;
    int16_t iLengthSize = 1;
    uint16_t uPlayerCount = pRef->bActAsServer ? pRef->uTotalPlyrs : 1;
    uint16_t uLastPlayer = uPlayerCount;
    int32_t iResult;

    // check if any input forces us to send fat multi-packets.
    for (uIndex = 0; uIndex < uPlayerCount; uIndex++)
    {
        if (pLengthsArray[uIndex] > 0xFF)
        {
            iLengthSize = 2;
        }
    }

    // if we are sending sparse multipacket, adjust the player count accordingly
    if (pRef->bSparse && pRef->bActAsServer)
    {
        uPlayerCount = pRef->aMetaInfoToSend[GMDIST_Modulo(pRef->uUpdateMetaInfoEnd - 1, GMDIST_META_ARRAY_SIZE)].uPlayerCount;
    }

    // always pack multipacket as if there was at least 2 players (simplify the logic)
    if (pRef->bActAsServer && (uPlayerCount == 1))
    {
        uLastPlayer = 2;
        uPlayerCount = 2;
    }

    // compute the offset to various parts of the multipacket
    uOffsetVersion = pRef->bActAsServer ? 1 : 0;
    uOffsetTypes = uOffsetVersion + ((pRef->bSparse && pRef->bActAsServer) ? 1 : 0);
    uOffsetLengths = uOffsetTypes + (pRef->bActAsServer ? (uPlayerCount / 2) : 1);
    uOffsetBuffer = uOffsetLengths + (pRef->bActAsServer ? (iLengthSize * (uPlayerCount - 2)) : 0);

    // if server mode
    if (pRef->bActAsServer)
    {
        // compute the total packet size
        uSendSize = uOffsetBuffer;
        for (uIndex = 0; uIndex < pRef->uTotalPlyrs; uIndex++)
        {
            if (uIndex != pRef->uDistIndex)
            {
                uSendSize += pLengthsArray[uIndex];
            }
        }

        // bail out if the packet would overflow
        if (uSendSize >= NETGAME_DATAPKT_MAXSIZE)
        {
            ds_snzprintf(pRef->strErrorText, sizeof(pRef->strErrorText), "Multipacket bigger than NETGAME_DATAPKT_MAXSIZE (%d). Discarding and reporting overflow.\n", NETGAME_DATAPKT_MAXSIZE);
            NetPrintf(("netgamedist: [%p] Multipacket bigger than NETGAME_DATAPKT_MAXSIZE (%d). Discarding and reporting overflow.\n", pRef, NETGAME_DATAPKT_MAXSIZE));
            return(GMDIST_OVERFLOW_MULTI);
        }

        // mark the version, if we are sending meta information
        if (pRef->bSparse)
        {
            pRef->aMultiBuf[uOffsetVersion] = GMDIST_Modulo(pRef->aMetaInfoToSend[GMDIST_Modulo(pRef->uUpdateMetaInfoEnd - 1, GMDIST_META_ARRAY_SIZE)].uVer, GMDIST_META_ARRAY_SIZE);
        }

        uOurType = (pTypesArray[pRef->uDistIndex] & ~GMDIST_DATA_CRC_REQUEST);
        if (uOurType == GMDIST_DATA_NONE)
        {
            pRef->aMultiBuf[0] = 0;
        }
        else
        {
            if (((iDelta - 1) - pRef->iLastSentDelta) >= PACKET_WINDOW)
            {
                ds_snzprintf(pRef->strErrorText, sizeof(pRef->strErrorText), "GMDIST_OVERFLOW_WINDOW (pRef->m_packetid[pRef->InpBufData.iBeg]-1) is %d, pRef->iLastSentDelta is %d.\n", (pRef->aPacketId[pRef->InpBufData.iBeg]-1), pRef->iLastSentDelta);
                return(GMDIST_OVERFLOW_WINDOW);
            }

            pRef->aMultiBuf[0] = (iDelta-1) - pRef->iLastSentDelta;

            pRef->iLastSentDelta = (iDelta-1);
        }
        pRef->aPacketId[pRef->InpBufData.iBeg] = 0;

        if (uOurType == GMDIST_DATA_NONE)
        {
            pRef->iIOOffset = GMDIST_Modulo(pRef->iIOOffset + 1, PACKET_WINDOW);
        }
    }

    pLengths = (unsigned char *)pRef->aMultiBuf + uOffsetLengths;
    pTypePos = (unsigned char *)pRef->aMultiBuf + uOffsetTypes;

    uPos = uOffsetBuffer;
    uCount = 0;

    for (uIndex = 0; uIndex < uLastPlayer; uIndex++)
    {
        uType = (pTypesArray[uIndex] & ~GMDIST_DATA_CRC_REQUEST);

        if (((uType == GMDIST_DATA_NONE) || (uType == GMDIST_DATA_DISCONNECT)) && !pRef->bActAsServer)
        {
            #if DIRTYCODE_LOGGING
            NetPrintfVerbose((pRef->uVerbose, NETGAMEDIST_VERBOSITY,"netgamedist: [%p] Exiting NetGameDistInputLocalMulti() because of invalid data type.\n", pRef));
            #endif
            return(GMDIST_INVALID);
        }

        // if sending multipackets, don’t send input back to user who originally sent it
        if (pRef->bActAsServer && (uIndex == pRef->uDistIndex))
        {
            continue;
        }
        // if sending sparse multipackets, skip sending blank inputs for users that aren't in the game
        if (pRef->bSparse && ((pRef->aMetaInfoToSend[GMDIST_Modulo(pRef->uUpdateMetaInfoEnd - 1, GMDIST_META_ARRAY_SIZE)].uMask & (1 << uIndex)) == 0))
        {
            continue;
        }

        // put the data in
        ds_memcpy(pRef->aMultiBuf + uPos, ppBuffer[uIndex], pLengthsArray[uIndex]);
        uPos += pLengthsArray[uIndex];

        if (pRef->bActAsServer)
        {
            if (uCount < (uPlayerCount - 2u))
            {
                // we assign the length here for normal packets
                if (iLengthSize == 1)
                {
                    pLengths[uCount] = pLengthsArray[uIndex];
                }
                else
                {
                    // and for fat multipackets
                    pLengths[uCount * 2] = pLengthsArray[uIndex] / 256;
                    pLengths[uCount * 2 + 1] = pLengthsArray[uIndex] % 256;
                }
            }
        }

        if (uCount % 2)
        {
            *pTypePos = (*pTypePos & 0x0f) | ((pTypesArray[uIndex] << 4) & 0xf0);
            pTypePos++;
        }
        else
        {
            *pTypePos = (pTypesArray[uIndex] & 0x0f);
        }
        uCount++;
    }

    if (!NetGameDistStatus(pRef, '?cws', uPos, NULL, 0))
    {
        #if DIRTYCODE_LOGGING
        NetPrintfVerbose((pRef->uVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [%p] Exiting NetGameDistInputLocalMulti() with return value GMDIST_OVERFLOW.\n", pRef));
        #endif
        return(GMDIST_OVERFLOW);
    }

    #if DIRTYCODE_LOGGING
    {
        uint8_t uDataType = (pTypesArray[0] & ~GMDIST_DATA_CRC_REQUEST);
        NetPrintfVerbose((pRef->uVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [%p] NetGameDistInputLocalMulti() called with tick=%d len=%d type=%s\n",
            pRef, NetTick(), pLengthsArray[0], _strNetGameDistDataTypes[(((uDataType >= GMDIST_DATA_NONE) && (uDataType <= GMDIST_DATA_NODATA)) ? uDataType : 0)]));

    }
    #endif

    // increment output packet count
    if ((iResult = _NetGameDistSendInput(pRef, pRef->aMultiBuf, uPos, iLengthSize)) > 0)
    {
        pRef->iOutboundPktCnt++;
    }

    // pass the size of length used to the send input function
    return(iResult);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistInputCheck

    \Description
        Check input status (see how long till next)

    \Input *pRef    - reference pointer
    \Input *pSend   - (optional) stores time until next packet should be sent
    \Input *pRecv   - (optional) stores whether data is available

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
void NetGameDistInputCheck(NetGameDistRefT *pRef, int32_t *pSend, int32_t *pRecv)
{
    int32_t iRemain;
    uint32_t uTick;
    int32_t iInData;
    int32_t iOutData;

    // call the status proc
    (pRef->pStatProc)(pRef->pNetGameLinkRef, 'stat', 0, &pRef->NetGameLinkStats, sizeof(NetGameLinkStatT));

    // get current time
    uTick = NetTick();

    // make sure next send time is initialized
    if (pRef->uInpNext == 0)
    {
        pRef->uInpNext = uTick;
    }

    // see if its time to recalc network parms
    if (uTick > pRef->uInpCalc)
    {
        // set the number of unacknowledged packets permitted
        // (this number must be large enough to cover the network latency
        pRef->iInputWind = (pRef->NetGameLinkStats.late+pRef->iInputRate)/pRef->iInputRate;

        if (pRef->bRecvMulti)
        {
            pRef->iInputWind *= 2;
        }

        if (pRef->iInputWind < pRef->iInputMini)
        {
            pRef->iInputWind = pRef->iInputMini;
        }
        if (pRef->iInputWind > pRef->iInputMaxi)
        {
            pRef->iInputWind = pRef->iInputMaxi;
        }
        if (pRef->iInputWind > 500/pRef->iInputRate)
        {
            pRef->iInputWind = 500/pRef->iInputRate;
        }
        // determine time till next update
        pRef->uInpCalc = uTick + pRef->iInputRate;
    }

    // figure out time remaining until next send
    iRemain = pRef->uInpNext - uTick;

    // clamp to valid rate
    if (iRemain < 0)
    {
        iRemain = 0;
    }
    if (iRemain > pRef->iInputRate *2)
    {
        iRemain = pRef->iInputRate *2;
    }

    _NetGameDistCheckWindow(pRef, iRemain, uTick);

    // figure out time till next send
    iRemain = pRef->uInpNext - uTick;
    // clamp to valid rate
    if (iRemain < 0)
    {
        iRemain = 0;
    }

    // return time until next packet should be sent
    if (pSend != NULL)
    {
        // make sure rate is set
        if (pRef->iInputRate == 0)
        {
            // data is not initialized -- just return a delay
            *pSend = 50;
        }
        else if ((pRef->OutBufData.iBeg != pRef->OutBufData.iEnd) || (!pRef->bRdyRecvRemote))
        {
            // dont send while something in output queue
            *pSend = 10;
        }
        else
        {
            *pSend = iRemain;
        }
    }

    // indicate if a packet is waiting
    if (pRecv != NULL)
    {
        // if waiting on remote data, do an update
        if (pRef->InpBufData.iBeg == pRef->InpBufData.iEnd)
        {
            NetGameDistUpdate(pRef);
        }
        // get input data queue length
        iInData = GMDIST_Modulo(pRef->InpBufData.iEnd - pRef->InpBufData.iBeg, PACKET_WINDOW);
        // get output data queue length (in multi-mode we assume one so we don't gate receiving due to an empty output queue)
        iOutData = !pRef->bRecvMulti ? GMDIST_Modulo(pRef->OutBufData.iEnd - GMDIST_Modulo(pRef->InpBufData.iBeg + pRef->iIOOffset, PACKET_WINDOW), PACKET_WINDOW) : 1;
        // write if data is available (non-zero=available)
        *pRecv = (iInData < iOutData) ? iInData : iOutData;
    }

    #if DIRTYCODE_LOGGING
    _NetGameDistInputCheckLog(pRef, pSend, pRecv);
    #endif
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistInputRate

    \Description
        Set the input rate

    \Input *pRef    - reference pointer
    \Input iRate    - new input rate

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
void NetGameDistInputRate(NetGameDistRefT *pRef, int32_t iRate)
{
    // save rate if changed
    if (iRate > 0)
    {
        pRef->iInputRate = iRate;
    }
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistInputClear

    \Description
        Flush the input queue.

    \Input *pRef     - reference pointer

    \Notes
        This must be done with independent synchronization before and after to avoid issues.

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
void NetGameDistInputClear(NetGameDistRefT *pRef)
{
    // reset buffer pointers
    pRef->OutBufData.iEnd = 0;
    pRef->OutBufData.iBeg = 0;
    pRef->InpBufData.iEnd = 0;
    pRef->InpBufData.iBeg = 0;

    // reset sequence numbers
    pRef->uLocalSeq = 0;
    pRef->uGlobalSeq = 0;

    // reset the iooffset between the two queues.
    pRef->iIOOffset = 0;

    // reset the send time
    pRef->uInpNext = NetTick();
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistControl

    \Description
        Set NetGameDist operation parameters

    \Input *pRef     - reference pointer
    \Input iSelect   - item to tweak
    \Input iValue    - tweak value
    \Input pValue    - pointer tweak value

    \Output
        int32_t         - selector specific

    \Notes

        Selectors are:

    \verbatim
        SELECTOR    DESCRIPTION
        'clri'      clear the local queue of inputs
        'crcs'      enable/disable the reception of CRC challenges
        'lcrc'      to provide the current CRC as request by NetGameDist
        'lrcv'      local recv flow control. Set whether we are ready to receive or not.
        'lsnd'      local send flow control. Set whether we are ready to send or not.
        'maxi'      max window size clamp
        'mini'      min window size clamp
        'spam'      sets debug output verbosity (0...n)
    \endverbatim

    \Version 11/18/08 (jrainy)
*/
/*************************************************************************************************F*/
int32_t NetGameDistControl(NetGameDistRefT *pRef, int32_t iSelect, int32_t iValue, void *pValue)
{
    if (iSelect == 'clri')
    {
        NetGameDistInputClear(pRef);
        return(0);
    }
    if (iSelect == 'crcs')
    {
        pRef->bCRCChallenges = iValue;
        return(0);
    }
    if (iSelect == 'lcrc')
    {
        pRef->bUpdateFlowCtrl = TRUE;
        pRef->uLocalCRC = iValue;
        pRef->bLocalCRCValid = TRUE;
        return(0);
    }
    if (iSelect == 'lrcv')
    {
        if (iValue != pRef->bRdyRecv)
        {
            pRef->bUpdateFlowCtrl = TRUE;
            pRef->bRdyRecv = iValue;
            NetPrintfVerbose((pRef->uVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [%p] flow control change -> %s to receive\n", pRef, (pRef->bRdyRecv ? "ready" : "not ready")));
        }
        return(pRef->bRdyRecv);
    }
    if (iSelect == 'lsnd')
    {
        if (iValue != pRef->bRdySend)
        {
            pRef->bUpdateFlowCtrl = TRUE;
            pRef->bRdySend = iValue;
            NetPrintfVerbose((pRef->uVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [%p] flow control change -> %s to send\n", pRef, (pRef->bRdySend ? "ready" : "not ready")));
        }
        return(pRef->bRdySend);
    }
    if (iSelect == 'maxi')
    {
        if (iValue >= 0)
        {
            pRef->iInputMaxi = iValue;
        }
        return(pRef->iInputMaxi);
    }
    if (iSelect == 'mini')
    {
        if (iValue >= 0)
        {
            pRef->iInputMini = iValue;
        }
        return(pRef->iInputMini);
    }
    if (iSelect == 'spam')
    {
        pRef->uVerbose = iValue;
        return(0);
    }
    // no action
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistUpdate

    \Description
        Perform periodic tasks.

    \Input *pRef         - reference pointer

    \Output
        uint32_t        - current sequence number

    \Notes
        Application must call this every 100ms or so.

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
uint32_t NetGameDistUpdate(NetGameDistRefT *pRef)
{
    NetGamePacketT* pPacket = (NetGamePacketT*)&pRef->MaxPkt;
    unsigned char *pData;
    int32_t iNext, iCurrent;
    uint32_t uIndex, uPos;
    unsigned char *pExisting;
    unsigned char *pIncoming;

    // update the input rate if needed
    NetGameDistInputRate(pRef, 0);

    // send any pending game packets
    NetGameDistInputLocal(pRef, NULL, 0);

    // grab incoming packets
    while (( GMDIST_Modulo(pRef->InpBufData.iEnd + 1, PACKET_WINDOW) != pRef->InpBufData.iBeg) && (_NetGameDistInputPeekRecv(pRef, pPacket, 1) > 0))
    {
        // dispatch the packet according to type
        if ((pPacket->head.kind == GAME_PACKET_INPUT) ||
            (pPacket->head.kind == GAME_PACKET_INPUT_MULTI) ||
            (pPacket->head.kind == GAME_PACKET_INPUT_MULTI_FAT))
        {
            pRef->iInboundPktCnt++;

            iNext = GMDIST_Modulo(pRef->InpBufData.iEnd + 1, PACKET_WINDOW);
            if (pRef->InpBufData.iBeg != pRef->InpBufData.iEnd && !pRef->bRecvMulti && pRef->bActAsServer && pRef->pDropProc)
            {
                iCurrent = GMDIST_Modulo(pRef->InpBufData.iEnd - 1, PACKET_WINDOW);
                pExisting = (pRef->InpBufData.pControllerIO + pRef->InpBufData.IOLookUp[iCurrent].uPos);
                pIncoming = pPacket->body.data;

                if (pRef->pDropProc(pRef, pExisting + 1, pIncoming + 1, pExisting[0], pIncoming[0]))
                {
                    iNext = pRef->InpBufData.iEnd;
                    pRef->iInboundDropPktCnt++;
                    pRef->InpBufData.iEnd = GMDIST_Modulo(pRef->InpBufData.iEnd - 1, PACKET_WINDOW);
                }
            }

            _ProcessPacketType(pRef, pPacket->head.kind);
            // check for overflow
            if (iNext == pRef->InpBufData.iBeg)
            {
                // ignore the packet
                NetPrintf(("netgamedist: [%p] NetGameDistUpdate() - buffer overflow (queue full)!\n", pRef));
                ds_snzprintf(pRef->strErrorText, sizeof(pRef->strErrorText), "NetGameDistUpdate, buffer overflow (queue full).");

                pRef->iErrCond = GMDIST_QUEUE_FULL;
                return((unsigned)(-1));
            }
            else
            {
                // point to data buffer

                // we currently write one byte past the buffer after reserving one extra byte
                // the len set in the data structure includes this extra byte past the packet data
                if (_SetDataPtr(pRef, &pRef->InpBufData, pPacket->head.len + 1))
                {
                    pData = pRef->InpBufData.pControllerIO+pRef->InpBufData.IOLookUp[pRef->InpBufData.iEnd].uPos;
                    pRef->InpBufData.IOLookUp[pRef->InpBufData.iEnd].uLen = pPacket->head.len + 1;
                    // copy into buffer
                    ds_memcpy(pData, pPacket->body.data, pPacket->head.len);
                    pData[pPacket->head.len] = pPacket->head.kind;

                    pRef->aPacketId[pRef->InpBufData.iEnd] = pRef->iInboundPktCnt;

                    // update queue time
                    pRef->InpBufData.IOLookUp[pRef->InpBufData.iEnd].uInsertTime = pPacket->head.when;

                    // add item to buffer
                    pRef->InpBufData.iEnd = iNext;

                    // display number of packets in buffer
                    #if TIMING_DEBUG
                    NetPrintf(("netgamedist: [%p] NetGameDistUpdate() inpbuf=%d packets\n", pRef,
                        GMDIST_Modulo(pRef->InpBufData.iEnd - pRef->InpBufData.iBeg, PACKET_WINDOW)));
                    #endif
                }
                else
                {
                    NetPrintf(("netgamedist: [%p] NetGameDistUpdate() - buffer overflow (memory)!\n", pRef));
                    pRef->iErrCond = GMDIST_QUEUE_MEMORY;
                    return((unsigned)(-1));
                }
            }
        }
        else if (pPacket->head.kind == GAME_PACKET_STATS)
        {
            // clear the contents, if a player leaves the game we don't want their stats to be misrepresented
            ds_memclr(pRef->aRecvStats, sizeof(pRef->aRecvStats));

            for (uPos = 0, uIndex = 0; uPos < pPacket->head.len; uIndex += 1)
            {
                ds_memcpy(&pRef->aRecvStats[uIndex], pPacket->body.data + uPos, sizeof(*pRef->aRecvStats) );
                uPos += sizeof(*pRef->aRecvStats);

                pRef->aRecvStats[uIndex].late = SocketNtohs(pRef->aRecvStats[uIndex].late);
                pRef->aRecvStats[uIndex].bps = SocketNtohs(pRef->aRecvStats[uIndex].bps);
            }
        }
        else if (pPacket->head.kind == GAME_PACKET_INPUT_FLOW)
        {
            pRef->bRdySendRemote = pPacket->body.data[0];
            pRef->bRdyRecvRemote = pPacket->body.data[1];

            if (pPacket->head.len >= 7)
            {
                if (pRef->MaxPkt.body.data[2])
                {
                    pRef->bRemoteCRCValid = TRUE;
                    pRef->uRemoteCRC = (pRef->MaxPkt.body.data[6] |
                                        (pRef->MaxPkt.body.data[5] << 8) |
                                        (pRef->MaxPkt.body.data[4] << 16) |
                                        (pRef->MaxPkt.body.data[3] << 24));
                }
                #if DIRTYCODE_LOGGING
                NetPrintfVerbose((pRef->uVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [%p] bRemoteCRCValid is %d, uRemoteCRC is %d\n", pRef, pRef->bRemoteCRCValid, pRef->uRemoteCRC));
                #endif
            }

            NetPrintf(("netgamedist: [%p] Got GAME_PACKET_INPUT_FLOW (send %d recv %d)\n", pRef, pRef->bRdySendRemote, pRef->bRdyRecvRemote));
        }
        else if (pPacket->head.kind == GAME_PACKET_INPUT_META)
        {
            // process meta-information about sparse multi-packets
            uint8_t uVer;
            uint32_t uMask;

            pRef->bGotMetaInfo = TRUE;

            uVer = GMDIST_Modulo(pPacket->body.data[0], GMDIST_META_ARRAY_SIZE);
            uMask = ((pPacket->body.data[1] << 24) +
                     (pPacket->body.data[2] << 16) +
                     (pPacket->body.data[3] << 8) +
                     (pPacket->body.data[4]));

            NetPrintf(("netgamedist: Got GAME_PACKET_INPUT_META (version %d mask 0x%08x)\n", uVer, uMask));

            pRef->aMetaInfo[uVer].uMask = uMask;
            pRef->aMetaInfo[uVer].uPlayerCount = _NetGameDistCountBits(pRef, uMask);
            pRef->aMetaInfo[uVer].uVer = uVer;
        }
    }

    // update link status
    NetGameDistStatus(pRef, 'stat', 0, NULL, 0);

    // show the history
    #if PING_DEBUG
    _PingHistory(&pRef->NetGameLinkStats);
    #endif

    // send meta-information as needed
    if (pRef->uUpdateMetaInfoBeg != pRef->uUpdateMetaInfoEnd)
    {
        _NetGameDistSendMetaInfo(pRef);
    }

    // send flow control updates as needed
    if (pRef->bUpdateFlowCtrl)
    {
        _NetGameDistSendFlowUpdate(pRef);
    }

    // return current sequence number
    return(pRef->uGlobalSeq);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistSetProc

    \Description
        Sets or override the various procedure pointers.

    \Input *pRef        - reference pointer
    \Input iKind        - kind
    \Input *pProc       - proc

    \Notes
        'drop' should only be used on game servers

    \Version 02/27/07 (jrainy)
*/
/*************************************************************************************************F*/
void NetGameDistSetProc(NetGameDistRefT *pRef, int32_t iKind, void *pProc)
{
    switch(iKind)
    {
    case 'drop':
        pRef->pDropProc = (NetGameDistDropProc *)pProc;
        break;
    case 'peek':
        pRef->pPeekProc = (NetGameDistPeekProc *)pProc;
        break;
    case 'recv':
        pRef->pRecvProc = (NetGameDistRecvProc *)pProc;
        break;
    case 'send':
        pRef->pSendProc = (NetGameDistSendProc *)pProc;
        break;
    case 'stat':
        pRef->pStatProc = (NetGameDistStatProc *)pProc;
        break;
    case 'link':
        pRef->pLinkCtrlProc = (NetGameDistLinkCtrlProc *)pProc;
        break;
    }
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistSendStats

    \Description
        Send stats. Meant to be used by the game server to send stats regarding all clients

    \Input *pRef         - reference pointer
    \Input *pStats       - uTotalPlyrs-sized array of stats

    \Version 03/14/07 (jrainy)
*/
/*************************************************************************************************F*/
void NetGameDistSendStats(NetGameDistRefT *pRef, NetGameDistStatT *pStats)
{
    uint32_t uIndex;
    int32_t iPos;

    pRef->MaxPkt.head.kind = GAME_PACKET_STATS;
    pRef->MaxPkt.head.len = (uint16_t)sizeof(NetGameDistStatT) * pRef->uTotalPlyrs;

    iPos = 0;
    for (uIndex = 0; uIndex<pRef->uTotalPlyrs; uIndex++)
    {
        ds_memcpy(pRef->MaxPkt.body.data + iPos, &pStats[uIndex], sizeof(NetGameDistStatT));
        iPos += sizeof(NetGameDistStatT);
    }

    pRef->pSendProc(pRef->pNetGameLinkRef, (NetGamePacketT*)&pRef->MaxPkt, 1);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistGetError

    \Description
        return non-zero if there ever was an overflow

    \Input *pRef     - reference pointer

    \Output
        int32_t      - 0 if there was no error, positive >0 otherwise.

    \Version 03/23/07 (jrainy)
*/
/*************************************************************************************************F*/
int32_t NetGameDistGetError(NetGameDistRefT *pRef)
{
    return(pRef->iErrCond);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistGetErrorText

    \Description
            copies into the passed buffer the last error text.

    \Input *pRef         - reference pointer
    \Input *pErrorBuffer - buffer to write into
    \Input iBufSize      - buf size

    \Version 08/15/08 (jrainy)
*/
/*************************************************************************************************F*/
void NetGameDistGetErrorText(NetGameDistRefT *pRef, char *pErrorBuffer, int32_t iBufSize)
{
    int32_t iMinSize = sizeof(pRef->strErrorText);

    if (iBufSize < iMinSize)
    {
        iMinSize = iBufSize;
    }

    strncpy(pErrorBuffer, pRef->strErrorText, iMinSize);
    pErrorBuffer[iMinSize - 1] = 0;
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistResetError

    \Description
        reset the overflow error condition to 0.

    \Input *pRef    - reference pointer

    \Version 03/23/07 (jrainy)
*/
/*************************************************************************************************F*/
void NetGameDistResetError(NetGameDistRefT *pRef)
{
    pRef->iErrCond = 0;
}


