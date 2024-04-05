/*H********************************************************************************/
/*!
    \File netgamedistserv.c

    \Description
        Server module to handle 2+ NetGameDist connections in a client/server
        architecture.

    \Copyright
        Copyright (c) 2007 Electronic Arts Inc.

    \Version 02/01/2007 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/game/netgamepkt.h"
#include "DirtySDK/game/netgamedistserv.h"

/*** Defines **********************************************************************/

#define MONITOR_HIGHWATER               (0)
#define DISTSERV_MAX_MULTIPACKET_LENGTH (1100)
#define DISTSERV_MAX_FIXEDRATE_MULTIPLE (3)
#define DISTSERV_DEFAULT_FIXEDRATE      (33)

/*** Type Definitions *************************************************************/

//! client state
typedef struct NetGameDistServClientT
{
    NetGameDistRefT *pGameDist;         //!< client dist ref
    char strName[32];                   //!< client name
    uint8_t aLocalInput[NETGAME_DATAPKT_MAXSIZE]; //!< local input buffer (TODO - this will be a queue)
    int32_t iLocalInputSize;            //!< size of input data
    int32_t iPeekKey;                   //!< delta at the time of our peek.
    uint8_t bLocalInput;                //!< whether we have local input or not
    uint8_t iLocalInputType;            //!< type of input
    uint8_t bInitialized;               //!< true if client is initialized
    uint8_t bDisconnected;              //!< whether the client is disconnected or not
    int32_t iDisconnectReason;          //!< why the disconnect
    int32_t iCount;                     //!< total number of packets received
    int32_t iCRC;                       //!< latest client-side CRC received
    uint8_t uCRCValid;                  //!< whether we got a CRC response to the last request
    uint8_t bRemoteRcv;                 //!< last 'rrcv' value (is client ready to recv)
    uint8_t bRemoteSnd;                 //!< last 'rsnd' value (is client ready to send)
    uint8_t bReallyPeeked;              //!< indicate whether we peeked  this client before InputLocalMulti this frame (to address delayed input scenarios)
    uint32_t uNakSent;                  //!< previous nak sent stat
    uint32_t uPackLost;                 //!< previous packet loss stat
    char strDiscReason[GMDIST_ERROR_SIZE];
} NetGameDistServClientT;

//! client list
typedef struct NetGameDistServClientListT
{
    int32_t iNumClients;                //!< current number of clients in list
    int32_t iMaxClients;                //!< max number of clients in list
    NetGameDistServClientT Clients[1];  //!< variable-length array of clients
} NetGameDistServClientListT;

//! module state
struct NetGameDistServT
{
    //! module memory group
    int32_t iMemGroup;
    void *pMemGroupUserData;

    //! verbose debug output level
    int32_t iVerbosity;

    //! Last stats update time
    uint32_t uLastStatsUpdate;

    // debugging
    uint32_t aPeekTimes[32];
    uint32_t aRecvTimes[32];
    uint32_t aSendTimes[32];
    uint32_t aEscalate[32];
    int32_t iFixedRate;

    uint32_t uNbFramesNoData;

    uint32_t uCRCRate;                  //!< number of frames between challenges
    uint32_t uCRCResponseLimit;         //!< number of frames allowed to receive response
    uint32_t uCRCRemainingFrames;       //!< number of frames remaining until next challenge
    uint32_t uCRCResponseCountdown;     //!< number of frames remaining to accept CRC responses

    uint8_t uSendThreshold;

    #if MONITOR_HIGHWATER
    int32_t iHighWaterInputQueue;
    int32_t iHighWaterOutputQueue;
    uint8_t uHighWaterChanged;
    #endif

    int32_t iFirstClient;
    uint32_t uLastFlowUpdateTime;
    uint32_t uOutputMultiPacketCount;  //!< total output dist multi packet
    uint32_t uCurStatClientCount;      //!< current stat client count
    uint8_t bClientCountChanged;       //!< did the client count change between stat sampling interval?
    uint8_t bFlowEnabled;              //!< whether all clients are ready to receive.
    uint8_t bFlowEnabledChanged;       //!< did flow control change between stat sampling interval?
    uint8_t bNoInputMode;              //!< TRUE means: outbound multipackets are no longer generated because inbound packets have not been coming in for multiple frames
    uint8_t bNoInputModeChanged;       //!< did no input mode change between stat sampling interval?
    uint8_t uMultiPacketVersion;       //!< sparse multi-packet version number. Changes when player list changes
    uint8_t _pad[2];

    NetGameDistServLoggingCbT *pLoggingCb;  //!< logging callback
    void *pUserData;                        //!< logging userdata

    //! variable-length array of clients -- must come last!
    NetGameDistServClientListT ClientList;
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _NetGameDistServLogPrintf

    \Description
        Log printing for netgamedistserv module

    \Input *pDistServ   - module state
    \Input *pFormat     - printf format string
    \Input ...          - variable argument list

    \Version 06/26/2019 (eesponda)
*/
/********************************************************************************F*/
static void _NetGameDistServLogPrintf(NetGameDistServT *pDistServ, const char *pFormat, ...)
{
    char strText[2048];
    int32_t iOffset = 0;
    va_list Args;

    // format prefix
    iOffset += ds_snzprintf(strText+iOffset, sizeof(strText)-iOffset, "netgamedistserv: ");

    // format output
    va_start(Args, pFormat);
    iOffset += ds_vsnprintf(strText+iOffset, sizeof(strText)-iOffset, pFormat, Args);
    va_end(Args);

    // forward to callback if registered
    if ((pDistServ != NULL) && (pDistServ->pLoggingCb != NULL))
    {
        pDistServ->pLoggingCb(strText, pDistServ->pUserData);
    }
    else
    {
        NetPrintf(("%s", strText));
    }
}

/*F********************************************************************************/
/*!
    \Function _NetGameDistServLogPrintfVerbose

    \Description
        Log printing for netgamedistserv module at varying verbosity levels

    \Input *pDistServ   - module state
    \Input iCheckLevel  - the level we are checking our internal iVerbosity against
    \Input *pFormat     - printf format string
    \Input ...          - variable argument list

    \Version 06/26/2019 (eesponda)
*/
/********************************************************************************F*/
static void _NetGameDistServLogPrintfVerbose(NetGameDistServT *pDistServ, int32_t iCheckLevel, const char *pFormat, ...)
{
    char strText[2048];
    va_list Args;

    // no-op
    if (pDistServ->iVerbosity <= iCheckLevel)
    {
        return;
    }

    // format output
    va_start(Args, pFormat);
    ds_vsnprintf(strText, sizeof(strText), pFormat, Args);
    va_end(Args);

    // format to the logging function
    _NetGameDistServLogPrintf(pDistServ, "%s", strText);
}

/*F********************************************************************************/
/*!
    \Function _NetGameDistServSanityCheckIn

    \Description
        Perform sanity checking on input.

    \Input *pDistServ   - module state
    \Input iClient      - client to check
    \Input iRet         - result from NetGameDistInputPeek()

    \Version 01/01/2007 (jrainy)
*/
/********************************************************************************F*/
static void _NetGameDistServSanityCheckIn(NetGameDistServT *pDistServ, int32_t iClient, int32_t iRet)
{
    int32_t iDelay;
    uint32_t uCurTick;

    uCurTick = NetTick();

    // no need to perform sanity checking on input when flow is disabled
    if (pDistServ->bFlowEnabled)
    {
        iDelay = uCurTick - pDistServ->aRecvTimes[iClient];
        if (iRet > 0)
        {
            pDistServ->aRecvTimes[iClient] = uCurTick;

            if (iDelay < 200)
            {
                pDistServ->aEscalate[iClient] = 0;
            }
        }
        if ((iDelay > 200) && pDistServ->aRecvTimes[iClient] && (pDistServ->aEscalate[iClient] < 1))
        {
            _NetGameDistServLogPrintfVerbose(pDistServ, 0, "no input in 200 ms from client %d\n", iClient);
            pDistServ->aEscalate[iClient] = 1;
        }
        else if ((iDelay > 500) && pDistServ->aRecvTimes[iClient] && (pDistServ->aEscalate[iClient] < 2))
        {
            _NetGameDistServLogPrintfVerbose(pDistServ, 0, "no input in 500 ms from client %d\n", iClient);
            pDistServ->aEscalate[iClient] = 2;
        }
        else if ((iDelay > 1000) && pDistServ->aRecvTimes[iClient])
        {
            _NetGameDistServLogPrintfVerbose(pDistServ, 0, "no input in 1000 ms from client %d\n", iClient);
            pDistServ->aRecvTimes[iClient] = uCurTick;
            pDistServ->aEscalate[iClient] = 0;
        }

        iDelay = uCurTick - pDistServ->aPeekTimes[iClient];
        pDistServ->aPeekTimes[iClient] = uCurTick;
        if (iDelay > (pDistServ->iFixedRate * DISTSERV_MAX_FIXEDRATE_MULTIPLE))
        {
            _NetGameDistServLogPrintfVerbose(pDistServ, 0, "no peek in %d ms from client %d\n", iDelay, iClient);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _NetGameDistServSanityCheckOut

    \Description
        Perform sanity checking on output.

    \Input *pDistServ   - module state
    \Input iClient      - client to check
    \Input iRet         - result from NetGameDistInputLocalMulti()

    \Version 01/01/2007 (jrainy)
*/
/********************************************************************************F*/
static void _NetGameDistServSanityCheckOut(NetGameDistServT *pDistServ, int32_t iClient, int32_t iRet)
{
    int32_t iDelay;
    uint32_t uCurTick;

    uCurTick = NetTick();

    iDelay = uCurTick - pDistServ->aSendTimes[iClient];
    if (iRet == 1)
    {
        pDistServ->aSendTimes[iClient] = uCurTick;
    }
    if ((iDelay > (pDistServ->iFixedRate * DISTSERV_MAX_FIXEDRATE_MULTIPLE)) && pDistServ->aSendTimes[iClient])
    {
        _NetGameDistServLogPrintfVerbose(pDistServ, 0, "no send in %d ms to client %d\n", iDelay, iClient);
        pDistServ->aSendTimes[iClient] = uCurTick;
    }
}

/*F********************************************************************************/
/*!
    \Function _NetGameDistServFlowControl

    \Description
        Send flow control update from the server as appropriate

    \Input *pDistServ   - module state

    \Version 12/03/2007 (jrainy)
*/
/********************************************************************************F*/
static void _NetGameDistServFlowControl(NetGameDistServT *pDistServ)
{
    int32_t iClient;
    NetGameDistServClientT *pDistClient;
    uint8_t bPrevFlowEnabled;          //!< previous reported stat flow enabled boolean 

    bPrevFlowEnabled = pDistServ->bFlowEnabled;
    pDistServ->bFlowEnabled = TRUE;

    for (iClient = 0; iClient < pDistServ->ClientList.iMaxClients; iClient++)
    {
        pDistClient = &pDistServ->ClientList.Clients[iClient];

        if (pDistClient->bInitialized && !pDistClient->bDisconnected)
        {
            if (!pDistClient->bRemoteRcv)
            {
                pDistServ->bFlowEnabled = FALSE;
            }
        }
    }

    _NetGameDistServLogPrintf(pDistServ, "sending flow update lrcv %d\n", pDistServ->bFlowEnabled);
    for (iClient = 0; iClient < pDistServ->ClientList.iMaxClients; iClient++)
    {
        pDistClient = &pDistServ->ClientList.Clients[iClient];

        if (pDistClient->bInitialized && !pDistClient->bDisconnected)
        {
            NetGameDistControl(pDistClient->pGameDist, 'lrcv', pDistServ->bFlowEnabled, NULL);
        }
    }

    /* If flow control changed, flag the boolean used to remember that it happened at least once
       during the metrics sampling interval. */
    if (bPrevFlowEnabled != pDistServ->bFlowEnabled)
    {
        if (pDistServ->bFlowEnabledChanged == FALSE)
        {
            _NetGameDistServLogPrintfVerbose(pDistServ, 1, "metric sampling interval marked invalid for idpps and inputdrop because flow control changed (flow enabled = %d)\n", pDistServ->bFlowEnabled);
        }
        pDistServ->bFlowEnabledChanged = TRUE;
    }
}

/*F********************************************************************************/
/*!
    \Function _NetGameDistServDiscClient

    \Description
        Destroy game dist refs.

    \Input *pDistServ       - module state
    \Input *pDistClient     - client to disconnect from
    \Input iDistClient      - index of client
    \Input iReason          - disconnection reason

    \Version 04/26/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _NetGameDistServDiscClient(NetGameDistServT *pDistServ, NetGameDistServClientT *pDistClient, int32_t iDistClient, int32_t iReason)
{
    // mark as disconnected
    if (!pDistClient->bDisconnected)
    {
        _NetGameDistServLogPrintf(pDistServ, "disconnecting from client %d\n", iDistClient);
        pDistClient->bDisconnected = TRUE;
        pDistClient->iDisconnectReason = iReason;
    }
    else if (pDistClient->pGameDist != NULL)
    {
        _NetGameDistServLogPrintf(pDistServ, "warning -- client %d has gamedist but is disconnected\n", iDistClient);
    }

    // destroy dist
    if (pDistClient->pGameDist != NULL)
    {
        NetGameDistGetErrorText(pDistClient->pGameDist, pDistClient->strDiscReason, sizeof(pDistClient->strDiscReason));

        _NetGameDistServLogPrintfVerbose(pDistServ, 0, "deleting dist ref for client %s/%d\n", pDistClient->strName, iDistClient);
        NetGameDistDestroy(pDistClient->pGameDist);
        pDistClient->pGameDist = NULL;
    }

    _NetGameDistServFlowControl(pDistServ);
}

/*F********************************************************************************/
/*!
    \Function _NetGameDistUpdateMulti

    \Description
        Update Multi configuration when a client is added or removed

    \Input *pDistServ   - module state

    \Version 02/13/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _NetGameDistUpdateMulti(NetGameDistServT *pDistServ)
{
    NetGameDistServClientT *pDistClient;
    int32_t iClient;
    int32_t iLastClient = 0;
    uint32_t uMask = 0;

    for (iClient = 0; iClient < pDistServ->ClientList.iMaxClients; iClient++)
    {
        pDistClient = &pDistServ->ClientList.Clients[iClient];
        if (pDistClient->bInitialized)
        {
            iLastClient = iClient;

            // set the bits in the mask for all initialized players
            uMask |= (1 << iClient);
        }
    }

    // increment the multipacket version
    pDistServ->uMultiPacketVersion++;

    for (iClient = 0; iClient < pDistServ->ClientList.iMaxClients; iClient++)
    {
        pDistClient = &pDistServ->ClientList.Clients[iClient];
        if (pDistClient->pGameDist != NULL)
        {
            _NetGameDistServLogPrintf(pDistServ, "issuing NetGameDistMultiSetup %d %d\n", iClient, iLastClient + 1);
            NetGameDistMultiSetup(pDistClient->pGameDist, iClient, iLastClient + 1);

            _NetGameDistServLogPrintf(pDistServ, "issuing NetGameDistMetaSetup 1 0x%0x %d\n", uMask, pDistServ->uMultiPacketVersion);
            NetGameDistMetaSetup(pDistClient->pGameDist, 1, uMask, pDistServ->uMultiPacketVersion);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _NetGameDistServDrop

    \Description
        Default implementation of drop function.

    \Input *pLinkRef        - module state
    \Input *pExisting       - input packet at end of the input queue
    \Input *pIncoming       - newly received input packet
    \Input uTypeExisting    - type of the packet in the queue
    \Input uTypeIncoming    - type of the new packet

    \Output
        int8_t              - TRUE if the input packet can be dropped, FALSE otherwise

    \Version 02/27/2007 (jrainy)
*/
/********************************************************************************F*/
static int8_t _NetGameDistServDrop(void *pLinkRef, void *pExisting, void *pIncoming, uint8_t uTypeExisting, uint8_t uTypeIncoming)
{
    return(uTypeExisting == GMDIST_DATA_INPUT_DROPPABLE);
}

/*F********************************************************************************/
/*!
    \Function _NetGameDistServHandleCRCResponses

    \Description
        Handle the CRC responses. Check the current response status of all clients

    \Input *pDistServ   - module state

    \Notes
        Can only compare 64 different CRCs. If we have over 64 clients, the function
        will still function correctly, as long as there is more than 64 different
        CRCs being returned by the clients (highly unlikely). If there's too many
        different CRCs received, we'll pick the one with majority from the 64 first
        clients.

    \Version 03/23/2009 (jrainy)
*/
/********************************************************************************F*/
static void _NetGameDistServHandleCRCResponses(NetGameDistServT *pDistServ)
{
    NetGameDistServClientT *pDistClient;
    int32_t iClient,iIndex;
    uint8_t uWaiting = FALSE;
    uint8_t uTied = FALSE;
    int32_t iCRCs[64] = {0};
    int32_t iOccurences[64] = {0};
    int32_t iBestCRC = 0;
    int32_t iBestOccurences = 0;

    // for all the current clients
    for (iClient = 0; iClient < pDistServ->ClientList.iMaxClients; iClient++)
    {
        pDistClient = &pDistServ->ClientList.Clients[iClient];

        // skip uninitialized/disconnected clients
        if (!pDistClient->bInitialized || pDistClient->bDisconnected)
        {
            continue;
        }

        // mark us waiting if this client didn't send its CRC yet
        if (!pDistClient->uCRCValid)
        {
            uWaiting = TRUE;
        }
        else
        {
            // For all the CRC seen already
            for(iIndex = 0; iIndex < 64; iIndex++)
            {
                // count the repeats
                if (pDistClient->iCRC == iCRCs[iIndex])
                {
                    iOccurences[iIndex]++;

                    if (iOccurences[iIndex] > iBestOccurences)
                    {
                        iBestOccurences = iOccurences[iIndex];
                        iBestCRC = iCRCs[iIndex];
                    }

                    break;
                }
                else if (iOccurences[iIndex] == 0)
                {
                    // remember the newly seen CRCs
                    iCRCs[iIndex] = pDistClient->iCRC;
                    iOccurences[iIndex] = 1;

                    if (iOccurences[iIndex] > iBestOccurences)
                    {
                        iBestOccurences = iOccurences[iIndex];
                        iBestCRC = iCRCs[iIndex];
                    }

                    break;
                }
            }
        }
    }

    // if the wait is over, (received all responses, or number of frames)
    if (!uWaiting || (pDistServ->uCRCResponseCountdown == 0))
    {
        for(iIndex = 0; iIndex < 64; iIndex++)
        {
            if ((iOccurences[iIndex] == iBestOccurences) && (iCRCs[iIndex] != iBestCRC))
            {
                uTied = TRUE;
            }
        }

        for (iClient = 0; iClient < pDistServ->ClientList.iMaxClients; iClient++)
        {
            pDistClient = &pDistServ->ClientList.Clients[iClient];

            // skip disconnected clients
            if (!pDistClient->bInitialized || pDistClient->bDisconnected)
            {
                continue;
            }

            if (!pDistClient->uCRCValid || (pDistClient->iCRC != iBestCRC) || uTied)
            {
                _NetGameDistServLogPrintf(pDistServ, "client %d failed CRC challenge: %d %d %d %d\n", iClient, pDistClient->uCRCValid, pDistClient->iCRC, iBestCRC, uTied);

                if (uTied)
                {
                    _NetGameDistServDiscClient(pDistServ, pDistClient, iClient, GMDIST_DESYNCED_ALL_PLAYERS);
                }
                else
                {
                    _NetGameDistServDiscClient(pDistServ, pDistClient, iClient, GMDIST_DESYNCED);
                }
            }
            else
            {
                _NetGameDistServLogPrintfVerbose(pDistServ, 2, "client %d passed CRC challenge: %d\n", iClient, pDistClient->iCRC);
            }

            pDistClient->uCRCValid = FALSE;
            pDistClient->iCRC = 0;
        }

        // go back to challenge mode
        pDistServ->uCRCResponseCountdown = 0;
        pDistServ->uCRCRemainingFrames = pDistServ->uCRCRate;
    }
}

/*F********************************************************************************/
/*!
    \Function _NetGameDistServPrepareInputs

    \Description
        Prepare the inputs for sending a multipacket

    \Input *pDistServ       - module state
    \Input **pInputs       - array of pointers to set to data buffers
    \Input *pInputSizes    - array of sizes to set
    \Input *pInputTypes    - array of types to set
    \Input *pInputUsed     - array of booleans, indicates whether a given player's input was used
    \Input *pInputDropped  - array of booleans, indicates whether a given player's input was dropped

    \Output
        uint8_t             - indicates whether any data was available and prepared

    \Version 09/22/2009 (jrainy)
*/
/********************************************************************************F*/
static uint8_t _NetGameDistServPrepareInputs(NetGameDistServT *pDistServ, void** pInputs, int32_t* pInputSizes, uint8_t* pInputTypes, uint8_t* pInputUsed, uint8_t* pInputDropped)
{
    int32_t iClient, iIndex;
    int32_t iRunningLength, iRunningLengthDroppable;
    int32_t iAllowed, iAllowedDroppable;
    uint8_t bDataAvailable;

    NetGameDistServClientT *pDistClient;

    for (iClient = 0; iClient < pDistServ->ClientList.iMaxClients; iClient++)
    {
        pInputSizes[iClient] = 0;
        pInputTypes[iClient] = GMDIST_DATA_NONE;
        pInputUsed[iClient] = FALSE;
    }

    iRunningLength = 0;
    iRunningLengthDroppable = 0;

    for (iIndex = 0, bDataAvailable = FALSE; iIndex < pDistServ->ClientList.iMaxClients; iIndex++)
    {
        iClient = (iIndex + pDistServ->iFirstClient) % pDistServ->ClientList.iMaxClients;
        pDistClient = &pDistServ->ClientList.Clients[iClient];

        if (pDistClient->bLocalInput)
        {
            if (pDistClient->iLocalInputType == GMDIST_DATA_INPUT_DROPPABLE)
            {
                iRunningLengthDroppable += pDistClient->iLocalInputSize;
            }
            else
            {
                if (iRunningLength + pDistClient->iLocalInputSize < DISTSERV_MAX_MULTIPACKET_LENGTH)
                {
                    iRunningLength += pDistClient->iLocalInputSize;
                }
            }
        }
    }

    iAllowed = iRunningLength;
    iAllowedDroppable = DISTSERV_MAX_MULTIPACKET_LENGTH - iAllowed;
    iRunningLength = 0;
    iRunningLengthDroppable = 0;

    for (iIndex = 0, bDataAvailable = FALSE; iIndex < pDistServ->ClientList.iMaxClients; iIndex++)
    {
        iClient = (iIndex + pDistServ->iFirstClient) % pDistServ->ClientList.iMaxClients;
        pDistClient = &pDistServ->ClientList.Clients[iClient];
        pInputDropped[iClient] = FALSE;

        pInputs[iClient] = pDistClient->aLocalInput;
        if (!pDistClient->bInitialized || pDistClient->bDisconnected)
        {
            pInputSizes[iClient] = 0;
            pInputTypes[iClient] = GMDIST_DATA_DISCONNECT;
        }
        else if (pDistClient->bLocalInput)
        {
            if (pDistClient->iLocalInputType == GMDIST_DATA_INPUT_DROPPABLE)
            {
                iRunningLengthDroppable += pDistClient->iLocalInputSize;
                if (iRunningLengthDroppable > iAllowedDroppable)
                {
                    pInputDropped[iClient] = TRUE;

                    _NetGameDistServLogPrintfVerbose(pDistServ, 1, "dropping packet for iClient %d, iIndex %d\n", iClient, iIndex);
                    continue;
                }
            }
            else
            {
                iRunningLength += pDistClient->iLocalInputSize;
                if (iRunningLength > iAllowed)
                {
                    _NetGameDistServLogPrintfVerbose(pDistServ, 1, "delaying packet for iClient %d, iIndex %d\n", iClient, iIndex);
                    continue;
                }
            }

            pInputSizes[iClient] = pDistClient->iLocalInputSize;
            pInputTypes[iClient] = pDistClient->iLocalInputType;
            bDataAvailable = TRUE;
        }

        pInputUsed[iClient] = TRUE;

        pDistClient->iCount += pDistClient->iPeekKey;
    }

    return(bDataAvailable);
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function NetGameDistServCreate

    \Description
        Create the NetGameDistServ module.

    \Input iMaxClients      - maximum number of clients supported
    \Input iVerbosity       - debug verbose level

    \Output
        NetGameDistServT *  - module state, or NULL if create failed

    \Version 02/01/2007 (jbrookes)
*/
/********************************************************************************F*/
NetGameDistServT *NetGameDistServCreate(int32_t iMaxClients, int32_t iVerbosity)
{
    NetGameDistServT *pDistServ;
    int32_t iMemSize;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // calculate memory size based on number of clients
    iMemSize = sizeof(*pDistServ) + (sizeof(pDistServ->ClientList.Clients[0]) * (iMaxClients - 1));

    // allocate and init module state
    if ((pDistServ = DirtyMemAlloc(iMemSize, NETGAMEDISTSERV_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        _NetGameDistServLogPrintf(NULL, "could not allocate module state\n");
        return(NULL);
    }
    ds_memclr(pDistServ, iMemSize);
    pDistServ->iMemGroup = iMemGroup;
    pDistServ->pMemGroupUserData = pMemGroupUserData;

    // init other module state
    pDistServ->ClientList.iMaxClients = iMaxClients;
    pDistServ->iVerbosity = iVerbosity;
    pDistServ->uSendThreshold = 4;
    pDistServ->uLastFlowUpdateTime = NetTick();
    pDistServ->uCRCResponseCountdown = 0;
    pDistServ->uCRCRemainingFrames = pDistServ->uCRCRate;
    pDistServ->iFixedRate = DISTSERV_DEFAULT_FIXEDRATE;

    // return module ref to caller
    return(pDistServ);
}

/*F********************************************************************************/
/*!
    \Function NetGameDistServDestroy

    \Description
        Destroy the NetGameDistServ module.

    \Input *pDistServ   - module state

    \Version 02/01/2007 (jbrookes)
*/
/********************************************************************************F*/
void NetGameDistServDestroy(NetGameDistServT *pDistServ)
{
    DirtyMemFree(pDistServ, NETGAMEDISTSERV_MEMID, pDistServ->iMemGroup, pDistServ->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function NetGameDistServAddClient

    \Description
        Add a client to the client list

    \Input *pDistServ   - module state
    \Input iClient      - index of slot to add client to
    \Input *pLinkRef    - link ref to create dist with
    \Input *pClientName - name of client

    \Output
        int32_t         - negative=error, else success

    \Version 02/05/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetGameDistServAddClient(NetGameDistServT *pDistServ, int32_t iClient, NetGameLinkRefT *pLinkRef, const char *pClientName)
{
    // ref given client index
    NetGameDistServClientT *pDistClient = &pDistServ->ClientList.Clients[iClient];

    // make sure this slot isn't already taken
    if (pDistClient->bInitialized && !pDistClient->bDisconnected)
    {
        _NetGameDistServLogPrintf(pDistServ, "skipping add of client to slot %d when slot is not empty\n", iClient);
        return(-1);
    }

    // save info and mark as initialized
    ds_memclr(pDistClient, sizeof(*pDistClient));
    ds_strnzcpy(pDistClient->strName, pClientName, sizeof(pDistClient->strName));
    pDistClient->bInitialized = TRUE;

    // create dist ref for client
    _NetGameDistServLogPrintfVerbose(pDistServ, 0, "creating dist ref for client %s\n", pDistClient->strName);
    DirtyMemGroupEnter(pDistServ->iMemGroup, pDistServ->pMemGroupUserData);
    pDistClient->pGameDist = NetGameDistCreate(pLinkRef,
        (NetGameDistStatProc *)NetGameLinkStatus,
        (NetGameDistSendProc *)NetGameLinkSend,
        (NetGameDistRecvProc *)NetGameLinkRecv,
        GMDIST_DEFAULT_BUFFERSIZE_IN * 20,
        GMDIST_DEFAULT_BUFFERSIZE_OUT * 20);
    DirtyMemGroupLeave();

    NetGameDistSetServer(pDistClient->pGameDist, TRUE);
    NetGameDistSetProc(pDistClient->pGameDist, 'drop', (void *)_NetGameDistServDrop);

    // update client count
    pDistServ->ClientList.iNumClients++;

    // this call is required for dirtycast to announce "no longer receiving" upon join-in-progress
    // DirtyCast will reenter the "receiving" state when the joiner explicitly uses NetGameDistControl('lrcv')
    _NetGameDistServFlowControl(pDistServ);

    // update dist multi configuration
    _NetGameDistUpdateMulti(pDistServ);

    // return success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function NetGameDistServDelClient

    \Description
        Delete a client from the client list

    \Input *pDistServ   - module state
    \Input iClient      - index of slot to delete client from

    \Output
        int32_t         - negative=error, else success

    \Version 02/05/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetGameDistServDelClient(NetGameDistServT *pDistServ, int32_t iClient)
{
    // ref given client index
    NetGameDistServClientT *pDistClient;

    // make sure this is a valid index
    if (iClient >= pDistServ->ClientList.iMaxClients)
    {
        _NetGameDistServLogPrintf(pDistServ, "skipping delete of client %d not in dist list\n", iClient);
        return(-1);
    }

    // ref client
    pDistClient = &pDistServ->ClientList.Clients[iClient];

    // disconnect from client
    _NetGameDistServDiscClient(pDistServ, pDistClient, iClient, GMDIST_DELETED);

    // clear slot in client list
    ds_memclr(pDistClient, sizeof(*pDistClient));

    // update client count
    pDistServ->ClientList.iNumClients--;

    // this used to be done only while game was not started
    // with meta-information, we can now do it at any time
    _NetGameDistUpdateMulti(pDistServ);

    // return success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function NetGameDistServDiscClient

    \Description
        Mark the specified client as disconnect. This will prevent sends and update

    \Input *pDistServ   - module state
    \Input iClient      - index of slot holding client to update

    \Output
        int32_t         - negative=error, else success

    \Version 03/15/2007 (jrainy)
*/
/********************************************************************************F*/
int32_t NetGameDistServDiscClient(NetGameDistServT *pDistServ, int32_t iClient)
{
    // make sure this is a valid index
    if (iClient >= pDistServ->ClientList.iMaxClients)
    {
        _NetGameDistServLogPrintf(pDistServ, "skipping disc of client %d not in dist list\n", iClient);
        return(-1);
    }

    // destroy connection refs and mark as disconnected
    _NetGameDistServDiscClient(pDistServ, &pDistServ->ClientList.Clients[iClient], iClient, GMDIST_DISCONNECTED);
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function NetGameDistServUpdateClient

    \Description
        Update the Dist ref for the specified client.

    \Input *pDistServ   - module state
    \Input iClient      - index of slot holding client to update

    \Output
        int32_t         - negative=disconnected, else zero

    \Version 02/05/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetGameDistServUpdateClient(NetGameDistServT *pDistServ, int32_t iClient)
{
    NetGameDistServClientT *pDistClient = &pDistServ->ClientList.Clients[iClient];
    int32_t iDistErr=0;

    // don't update uninitialized slots
    if (!pDistClient->bInitialized)
    {
        return(0);
    }

    // don't update disconnected clients
    if (pDistClient->bDisconnected)
    {
        return(pDistClient->iDisconnectReason ? pDistClient->iDisconnectReason : -1);
    }

    // update client's dist ref
    NetGameDistUpdate(pDistClient->pGameDist);

    // check for error
    iDistErr = NetGameDistGetError(pDistClient->pGameDist);
    if (iDistErr != 0)
    {
        _NetGameDistServLogPrintf(pDistServ, "NetGameDistGetError() returned %d\n", iDistErr);
        _NetGameDistServDiscClient(pDistServ, pDistClient, iClient, iDistErr);
        return(iDistErr);
    }

    // return status
    return(0);
}

/*F********************************************************************************/
/*!
    \Function NetGameDistServUpdate

    \Description
        Update the DistServ module.  This function is expected to be called at the
        desired output rate.

    \Input *pDistServ   - module state

    \Version 02/12/2007 (jrainy)
*/
/********************************************************************************F*/
void NetGameDistServUpdate(NetGameDistServT *pDistServ)
{
    NetGameDistServClientT *pDistClient;
    int32_t iClient, iResult;
    int32_t aInputSizes[GMDIST_MAX_CLIENTS];
    uint8_t aInputTypes[GMDIST_MAX_CLIENTS];
    uint8_t aInputUsed[GMDIST_MAX_CLIENTS];
    uint8_t aInputDropped[GMDIST_MAX_CLIENTS];
    NetGameDistStatT aStats[GMDIST_MAX_CLIENTS];
    NetGameLinkStatT Stat;
    uint8_t bDataAvailable;
    uint8_t bRemoteRcv, bRemoteSnd;
    uint8_t uCRCevent = FALSE;
    uint32_t uSendThreshold;
    void *pInputs[GMDIST_MAX_CLIENTS];

    int32_t iPeekLength[GMDIST_MAX_CLIENTS];
    int32_t iPeekResult[GMDIST_MAX_CLIENTS] = {0};
    uint8_t iType[GMDIST_MAX_CLIENTS];
    static char aLocalInput[GMDIST_MAX_CLIENTS][NETGAME_DATAPKT_MAXSIZE];
    uint8_t uNewFlow[GMDIST_MAX_CLIENTS];
    uint32_t uNumClientsInitConn = 0;
    uint32_t uNow = NetTick();
    uint8_t bPrevNoInputMode;          //!< previous no input mode

    // every 100 sends, send stats to clients
    if ((pDistServ->uLastStatsUpdate == 0) || (NetTickDiff(uNow, pDistServ->uLastStatsUpdate) > 2000))
    {
        pDistServ->uLastStatsUpdate = uNow;

        // gather stats for all connected clients
        for (iClient = 0; iClient < pDistServ->ClientList.iMaxClients; iClient++)
        {
            pDistClient = &pDistServ->ClientList.Clients[iClient];
            if (pDistClient->bInitialized && !pDistClient->bDisconnected)
            {
                NetGameDistStatus(pDistClient->pGameDist, 'stat', 0, &Stat, sizeof(NetGameLinkStatT));
                aStats[iClient].late = SocketHtons(Stat.late);
                aStats[iClient].bps = SocketHtons(Stat.outbps);
                aStats[iClient].pps = (uint8_t)Stat.outpps;
                aStats[iClient].slen = (uint8_t)NetGameDistStatus(pDistClient->pGameDist, 'slen', 0, NULL, 0);
                aStats[iClient].naksent = (uint8_t)(Stat.lnaksent - pDistClient->uNakSent);
                aStats[iClient].plost = (uint8_t)(Stat.lpacklost - pDistClient->uPackLost);
                pDistClient->uNakSent = Stat.lnaksent;
                pDistClient->uPackLost = Stat.lpacklost;
            }
            else
            {
                ds_memclr(&aStats[iClient], sizeof(aStats[0]));
            }
        }

        // broadcast updated stat info to all connected clients
        for (iClient = 0; iClient < pDistServ->ClientList.iMaxClients; iClient++)
        {
            pDistClient = &pDistServ->ClientList.Clients[iClient];
            if(pDistClient->bInitialized && !pDistClient->bDisconnected)
            {
                NetGameDistSendStats(pDistClient->pGameDist, aStats);
            }
        }
    }

    #if MONITOR_HIGHWATER
    for (iClient = 0; iClient < pDistServ->ClientList.iMaxClients; iClient++)
    {
        int32_t iRet;

        pDistClient = &pDistServ->ClientList.Clients[iClient];
        if(pDistClient->bInitialized && !pDistClient->bDisconnected)
        {
            iRet = GMDIST_Modulo(NetGameDistStatus(pDistClient->pGameDist, '?inp', 0, NULL, 0) - NetGameDistStatus(pDistClient->pGameDist, '?cmp', 0, NULL, 0), NetGameDistStatus(NULL, 'pwin', 0, NULL, 0));
            if (iRet > pDistServ->iHighWaterInputQueue)
            {
                pDistServ->iHighWaterInputQueue = iRet;
                pDistServ->uHighWaterChanged = TRUE;
            }

            iRet = GMDIST_Modulo(NetGameDistStatus(pDistClient->pGameDist, '?out', 0, NULL, 0) - NetGameDistStatus(pDistClient->pGameDist, '?snd', 0, NULL, 0), NetGameDistStatus(NULL, 'pwin', 0, NULL, 0));
            if (iRet > pDistServ->iHighWaterOutputQueue)
            {
                pDistServ->iHighWaterOutputQueue = iRet;
                pDistServ->uHighWaterChanged = TRUE;
            }
        }
    }
    #endif

    for (iClient = 0; iClient < pDistServ->ClientList.iMaxClients; iClient++)
    {
        pDistClient = &pDistServ->ClientList.Clients[iClient];

        if (pDistClient->bInitialized && !pDistClient->bDisconnected)
        {
            if (!pDistClient->bLocalInput)
            {
                int32_t iLength = sizeof(pDistClient->aLocalInput);
                // check for incoming data on this dist
                iResult = NetGameDistInputPeek(pDistClient->pGameDist, &pDistClient->iLocalInputType, pDistClient->aLocalInput, &iLength);
                pDistClient->bReallyPeeked = TRUE;
                _NetGameDistServSanityCheckIn(pDistServ, iClient, iResult);

                if (iResult > 0)
                {
                    pDistClient->iLocalInputSize = iLength;
                    pDistClient->bLocalInput = TRUE;
                    pDistClient->iPeekKey = iResult;
                }
                else if (iResult < 0)
                {
                    _NetGameDistServLogPrintf(pDistServ, "peek error for client %d\n", iClient);
                    _NetGameDistServDiscClient(pDistServ, pDistClient, iClient, GMDIST_PEEK_ERROR);
                    continue;
                }
            }

            bRemoteRcv = NetGameDistStatus(pDistClient->pGameDist,'rrcv', 0, NULL, 0);
            bRemoteSnd = NetGameDistStatus(pDistClient->pGameDist,'rsnd', 0, NULL, 0);

            if ((bRemoteRcv != pDistClient->bRemoteRcv) || (bRemoteSnd != pDistClient->bRemoteSnd))
            {
                pDistClient->bRemoteRcv = bRemoteRcv;
                pDistClient->bRemoteSnd = bRemoteSnd;

                _NetGameDistServLogPrintf(pDistServ, "client %d got GAME_PACKET_INPUT_FLOW (send %d, recv %d)\n", iClient, pDistClient->bRemoteSnd, pDistClient->bRemoteRcv);
                _NetGameDistServFlowControl(pDistServ);
                pDistServ->uLastFlowUpdateTime = NetTick();
            }
        }
    }

    bDataAvailable = _NetGameDistServPrepareInputs(pDistServ, pInputs, aInputSizes, aInputTypes, aInputUsed, aInputDropped);

    if (!bDataAvailable)
    {
        pDistServ->uNbFramesNoData++;
    }
    else
    {
        pDistServ->uNbFramesNoData = 0;
    }

    uSendThreshold = pDistServ->uSendThreshold;

    if (!pDistServ->bFlowEnabled)
    {
        uSendThreshold = 1;
    }

    // don't send if there was no data available for a while
    if ((uSendThreshold == 0) || (pDistServ->uNbFramesNoData < uSendThreshold))
    {
        if (pDistServ->uCRCRemainingFrames)
        {
            if (pDistServ->bFlowEnabled)
            {
                pDistServ->uCRCRemainingFrames--;
            }
            if (pDistServ->uCRCRemainingFrames == 0)
            {
                int32_t iClientCount = 0;

                // if we happen to send to only one client, below,
                for (iClient = 0; iClient < pDistServ->ClientList.iMaxClients; iClient++)
                {
                    pDistClient = &pDistServ->ClientList.Clients[iClient];
                    if (pDistClient->bInitialized && (!pDistClient->bDisconnected))
                    {
                        iClientCount++;
                    }
                }

                if (iClientCount <= 1)
                {
                    // don't send CRC request to single players
                    pDistServ->uCRCRemainingFrames = pDistServ->uCRCRate;
                    pDistServ->uCRCResponseCountdown = 0;
                }
                else
                {
                    for (iClient = 0; iClient < pDistServ->ClientList.iMaxClients; iClient++)
                    {
                        aInputTypes[iClient] |= GMDIST_DATA_CRC_REQUEST;
                        _NetGameDistServLogPrintfVerbose(pDistServ, 2, "setting GMDIST_DATA_CRC_REQUEST in aInputTypes[%d]\n", iClient);
                    }

                    pDistServ->uCRCResponseCountdown = pDistServ->uCRCResponseLimit;
                }
            }
        }

        bPrevNoInputMode = pDistServ->bNoInputMode;
        pDistServ->bNoInputMode = FALSE;

        // scan through client list, and queue all inputs for sending
        for (iClient = 0; iClient < pDistServ->ClientList.iMaxClients; iClient++)
        {
            uNewFlow[iClient] = FALSE;

            pDistClient = &pDistServ->ClientList.Clients[iClient];

            // skip disconnected clients
            if (!pDistClient->bInitialized || pDistClient->bDisconnected)
            {
                continue;
            }

            uNumClientsInitConn++;

            if (pDistServ->uCRCResponseCountdown)
            {
                if (NetGameDistStatus(pDistClient->pGameDist, 'rcrc', 0, NULL, 0))
                {
                    pDistClient->iCRC = NetGameDistStatus(pDistClient->pGameDist, 'rcrc', 1, NULL, 0);
                    pDistClient->uCRCValid = TRUE;

                    _NetGameDistServLogPrintfVerbose(pDistServ, 2, "got CRC response %d back from client %d\n", pDistClient->iCRC, iClient);

                    uCRCevent = TRUE;
                }
            }

            if (!pDistClient->bReallyPeeked)
            {
                iPeekLength[iClient] = sizeof(aLocalInput[iClient]);
                iPeekResult[iClient] = NetGameDistInputPeek(pDistClient->pGameDist,  &(iType[iClient]), &(aLocalInput[iClient]), &(iPeekLength[iClient]));

                if (iPeekResult[iClient] > 0)
                {
                    uNewFlow[iClient] = TRUE;
                }
            }

            pDistClient->bReallyPeeked = FALSE;

            // queue input into client
            if ((iResult = NetGameDistInputLocalMulti(pDistClient->pGameDist, aInputTypes, pInputs, aInputSizes, pDistClient->iPeekKey)) == 1)
            {
                pDistServ->uOutputMultiPacketCount++;
                _NetGameDistServLogPrintfVerbose(pDistServ, 2, "queued input into client %s\n", pDistClient->strName);
            }
            else
            {
                _NetGameDistServLogPrintf(pDistServ, "NetGameDistInputLocal() failed for client %s (err=%d)\n", pDistClient->strName, iResult);

                if (iResult == GMDIST_INVALID)
                {
                    _NetGameDistServDiscClient(pDistServ, pDistClient, iClient, GMDIST_INPUTLOCAL_FAILED_INVALID);
                }
                else if (iResult == GMDIST_OVERFLOW_MULTI)
                {
                    _NetGameDistServDiscClient(pDistServ, pDistClient, iClient, GMDIST_INPUTLOCAL_FAILED_MULTI);
                }
                else if (iResult == GMDIST_OVERFLOW_WINDOW)
                {
                    _NetGameDistServDiscClient(pDistServ, pDistClient, iClient, GMDIST_INPUTLOCAL_FAILED_WINDOW);
                }
                else
                {
                    _NetGameDistServDiscClient(pDistServ, pDistClient, iClient, GMDIST_INPUTLOCAL_FAILED);
                }
                continue;
            }

            if (aInputUsed[iClient] || aInputDropped[iClient])
            {
                pDistClient->bLocalInput = FALSE;
                pDistClient->iPeekKey = 0;
            }

            // sanity check output
            _NetGameDistServSanityCheckOut(pDistServ, iClient, iResult);

            // advance the state
            if ((iResult = NetGameDistInputQueryMulti(pDistClient->pGameDist, NULL, NULL, NULL)) > 0)
            {
                _NetGameDistServLogPrintfVerbose(pDistServ, 2, "sent input %d\n", iResult);
            }
            else if (iResult)
            {
                _NetGameDistServLogPrintf(pDistServ, "NetGameDistInputQueryMulti() failed (err=%d)\n", iResult);
                _NetGameDistServDiscClient(pDistServ, pDistClient, iClient, GMDIST_INPUTQUERY_FAILED);
                continue;
            }

            if (uNewFlow[iClient] && !aInputUsed[iClient])
            {
                _NetGameDistServLogPrintf(pDistServ, "we're getting behind on large packet delaying for client %d\n", iClient);
                _NetGameDistServDiscClient(pDistServ, pDistClient, iClient, GMDIST_INPUTQUERY_FAILED);
                continue;
            }
        }

        /* If the client count changed, flag the boolean used to remember that it happened at least once
           during the metrics sampling interval. */
        if (pDistServ->uCurStatClientCount != uNumClientsInitConn)
        {
            if (pDistServ->bClientCountChanged == FALSE)
            {
                _NetGameDistServLogPrintfVerbose(pDistServ, 1, "metric sampling interval marked invalid for idpps, odmpps and inputdrop because client count changed (client count = %d)\n", pDistServ->uCurStatClientCount);
            }
            pDistServ->bClientCountChanged = TRUE;
            pDistServ->uCurStatClientCount = uNumClientsInitConn;
        }

        for (iClient = 0; iClient < pDistServ->ClientList.iMaxClients; iClient++)
        {
            pDistClient = &pDistServ->ClientList.Clients[iClient];

            if (uNewFlow[iClient])
            {
                pDistClient->iLocalInputType = iType[iClient];
                pDistClient->iLocalInputSize = iPeekLength[iClient];
                pDistClient->iPeekKey = iPeekResult[iClient];
                pDistClient->bLocalInput = TRUE;

                ds_memcpy_s(pDistClient->aLocalInput, sizeof(pDistClient->aLocalInput), aLocalInput[iClient], sizeof(aLocalInput[iClient]));

                _NetGameDistServSanityCheckIn(pDistServ, iClient, iPeekResult[iClient]);
            }
        }

        // if something occured, or we have a active countdown
        if (pDistServ->uCRCResponseCountdown || uCRCevent)
        {
            // countdown
            if (pDistServ->bFlowEnabled && pDistServ->uCRCResponseCountdown)
            {
                pDistServ->uCRCResponseCountdown--;
            }

            // if something occured or we just reached zero.
            if (uCRCevent || (pDistServ->uCRCResponseCountdown == 0))
            {
                _NetGameDistServHandleCRCResponses(pDistServ);
            }
        }
    }
    else
    {
        bPrevNoInputMode = pDistServ->bNoInputMode;
        pDistServ->bNoInputMode = TRUE;
    }

    /* If 'no input mode' changed, flag the boolean used to remember that it happened at least once
       during the metrics sampling interval. */
    if (bPrevNoInputMode != pDistServ->bNoInputMode)
    {
        if (pDistServ->bNoInputModeChanged == FALSE)
        {
            _NetGameDistServLogPrintfVerbose(pDistServ, 1, "metric sampling interval marked invalid for odmpps because no input mode changed (no input mode = %d)\n", pDistServ->bNoInputMode);
        }
        pDistServ->bNoInputModeChanged = TRUE;
    }

    // $$ jrainy -- fixed up the below code to work without collapsing but introduced a very slight
    // side-effect if we have a list with a big empty area (Alice, [Empty], [Empty], [Empty], [Empty], Bob)
    // Bob will be able to get his oversized packets through more easily than Alice.
    // $$ TODO: A better change would be to increment iFirstclient to the next allocated/initialized
    // spot instead of by just 1. (minor for now)
    if (pDistServ->ClientList.iMaxClients != 0)
    {
        pDistServ->iFirstClient = (pDistServ->iFirstClient + 1) % pDistServ->ClientList.iMaxClients;
    }
}

/*F********************************************************************************/
/*!
    \Function NetGameDistServHighWaterChanged

    \Description
        Return whether the highwater mark changed, and the current highwater values.

    \Input *pDistServ               - module state
    \Input pHighWaterInputQueue     - input queue
    \Input pHighWaterOutputQueue    - output queue

    \Output
        uint8_t                     - whether the highwater mark changed since last call

    \Version 06/05/2008 (jrainy)
*/
/********************************************************************************F*/
uint8_t NetGameDistServHighWaterChanged(NetGameDistServT *pDistServ, int32_t* pHighWaterInputQueue, int32_t* pHighWaterOutputQueue)
{
    #if MONITOR_HIGHWATER
    uint8_t bChanged = pDistServ->uHighWaterChanged;

    if (pHighWaterInputQueue != NULL)
    {
        *pHighWaterInputQueue = pDistServ->iHighWaterInputQueue;
    }
    if (pHighWaterOutputQueue != NULL)
    {
        *pHighWaterOutputQueue = pDistServ->iHighWaterOutputQueue;
    }

    pDistServ->uHighWaterChanged = FALSE;
    return(bChanged);
    #else
    return(FALSE);
    #endif
}

/*F********************************************************************************/
/*!
    \Function NetGameDistServExplainError

    \Description
        return the lastest error reported by netgamedist, for client iClient

    \Input *pDistServ           - module state
    \Input iClient              - client to get the error for.

    \Output
        char *                  - the latest netgamedist error

    \Version 08/15/2008 (jrainy)
*/
/********************************************************************************F*/
char* NetGameDistServExplainError(NetGameDistServT *pDistServ, int32_t iClient)
{
    return(pDistServ->ClientList.Clients[iClient].strDiscReason);
}

/*F********************************************************************************/
/*!
    \Function NetGameDistServControl

    \Description
     Control behavior of module.

    \Input *pDistServ   - pointer to module state
    \Input iControl     - status selector
    \Input iValue       - control value
    \Input iValue2      - control value
    \Input *pValue      - control value

    \Notes
        iControl can be one of the following:

        \verbatim
            'crcc' - set the CRC challenge send rate in number of frames (0 to disable)
            'crcr' - set the CRC max response times in number of frames (0 makes it infinite)
            'mpty' - set the number(iValue) of empty frames needed to pause traffic
            'rate' - set the fixed rate the server has been configured to run at via iValue
            'rsta' - reset stat variables (flow control/no input mode bool)
        \endverbatim

    \Version 03/17/2009 (jrainy)
*/
/********************************************************************************F*/
int32_t NetGameDistServControl(NetGameDistServT *pDistServ, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'crcc')
    {
        if ((uint32_t)iValue != pDistServ->uCRCRate)
        {
            //set the rate
            pDistServ->uCRCRate = iValue;

            //start counting down
            pDistServ->uCRCRemainingFrames = iValue;
            _NetGameDistServLogPrintf(pDistServ, "setting CRC rate to %d\n", iValue);

        }
        return(0);
    }
    if (iControl == 'crcr')
    {
        pDistServ->uCRCResponseLimit = iValue;
        return(0);
    }
    if (iControl == 'mpty')
    {
        pDistServ->uSendThreshold = iValue;
        return(0);
    }
    if (iControl == 'rate')
    {
        pDistServ->iFixedRate = iValue;
        return(0);
    }
    if (iControl == 'rsta')
    {
        pDistServ->bClientCountChanged = FALSE;
        pDistServ->bFlowEnabledChanged = FALSE;
        pDistServ->bNoInputModeChanged = FALSE;

        return(0);
    }

    return(-1);
}

/*F********************************************************************************/
/*!
    \Function NetGameDistServStatus2

    \Description
        Get status information.

    \Input *pDistServ   - pointer to module state
    \Input iSelect      - status selector
    \Input iValue       - selector specific
    \Input *pBuf        - [out] storage for selector-specific output
    \Input iBufSize     - size of output buffer

    \Output
        int32_t         - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'clnu' - returns the current client count used for stat (note this not the same as iNumClients in client list)
            'drop' - returns total dropped input packet count in pValue (uint32_t) for client index in iValue, the value is invalid if GameServerDistStatus() return -1;
            'ftim' - flow time. Time (ms) since the last flow control packet was received.
            'icnt' - returns total input dist packet count in pValue (uint32_t) for client index in iValue, the value is invalid if GameServerDistStatus() return -1;
            'ninp' - whether we are currently in the mode where no inputs are sent due to stall in input arrival
            'ocnt' - reutrns output dist multi packet total in pValue (uint32_t), the value is invalid if GameServerDistStatus() return -1;
        \endverbatim

    \Version 09/18/2019 (tcho)
*/
/********************************************************************************F*/
int32_t NetGameDistServStatus2(NetGameDistServT *pDistServ, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    // total number of clients counted in the update loop
    if (iSelect == 'clnu')
    {
        return(pDistServ->uCurStatClientCount);
    }

    // total input packet dropped count for a client or total input dist packet count per client
    if ((iSelect == 'drop') || (iSelect == 'icnt'))
    {
        uint32_t uInputDropCount;
        uint32_t uInputCount;
        NetGameDistServClientT *pDistClient;

        if ((iValue < 0) || (iValue >= pDistServ->ClientList.iMaxClients))
        {
            // invalid index
            return(-1);
        }

        if ((pBuf == NULL) || (iBufSize < (int32_t)sizeof(uint32_t)))
        {
            // invalid output parameter
            return(-2);
        }

        pDistClient = &pDistServ->ClientList.Clients[iValue];
        
        // only report a value if the client is initialized and connected
        if (pDistClient->bInitialized && !pDistClient->bDisconnected)
        {
            if (iSelect == 'drop')
            {
                uInputDropCount = NetGameDistStatus(pDistClient->pGameDist, 'drop', 0, NULL, 0);
                ds_memcpy(pBuf, &uInputDropCount, sizeof(uint32_t));
            }
            
            if (iSelect == 'icnt')
            {
                uInputCount = NetGameDistStatus(pDistClient->pGameDist, 'icnt', 0, NULL, 0);
                ds_memcpy(pBuf, &uInputCount, sizeof(uint32_t));
            }

            if ((pDistServ->bClientCountChanged == TRUE) || (pDistServ->bFlowEnabledChanged == TRUE) || (pDistServ->bFlowEnabled == FALSE))
            {
                /* value copied in pBuf is valid but sampling interval should be ignored because conditions are not met
                   to properly calculate per-client rate of drops or per-client rate of inbound inputs. */
                return(1);
            }

            return(0);
        }
        else
        {
            ds_memclr(pBuf, iBufSize);
            return(-3);
        }
    }

    if (iSelect == 'ftim')
    {
        return(NetTickDiff(NetTick(), pDistServ->uLastFlowUpdateTime));
    }

    if (iSelect == 'ninp')
    {
        return(pDistServ->bNoInputMode);
    }

    // total output multi dist count per server
    if (iSelect == 'ocnt')
    {
        if ((iValue < 0) || (iValue >= pDistServ->ClientList.iMaxClients))
        {
            // invalid index
            return(-1);
        }

        if ((pBuf == NULL) || (iBufSize < (int32_t)sizeof(uint32_t)))
        {
            // invalid output parameter
            return(-2);
        }

        ds_memcpy(pBuf, &pDistServ->uOutputMultiPacketCount, sizeof(uint32_t));

        if ((pDistServ->uCurStatClientCount == 0) || (pDistServ->bClientCountChanged == TRUE) || (pDistServ->bNoInputModeChanged == TRUE) || (pDistServ->bNoInputMode == TRUE))
        {
            /* value copied in pBuf is valid but sampling interval should be ignored because conditions are not met
               to properly calculate rate of outbound multipackets */
            return(1);
        }

        return(0);
    }

    return(-1);
}

/*F********************************************************************************/
/*!
    \Function NetGameDistServStatus

    \Description
        Get status information.

    \Input *pDistServ   - pointer to module state
    \Input iSelect      - status selector
    \Input *pBuf        - [out] storage for selector-specific output
    \Input iBufSize     - size of output buffer

    \Output
        int32_t         - selector specific

    \Notes
        falls through to NetGameDistServStatus2()

    \Version 24/09/2012 (jrainy) first version
*/
/********************************************************************************F*/
int32_t NetGameDistServStatus(NetGameDistServT *pDistServ, int32_t iSelect, void *pBuf, int32_t iBufSize)
{
    return(NetGameDistServStatus2(pDistServ, iSelect, 0, pBuf, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function NetGameDistServSetLoggingCallback

    \Description
        Set the logging callback

    \Input *pDistServ   - pointer to module state
    \Input *pLoggingCb  - logging callback
    \Input *pUserData   - logging userdata

    \Version 06/26/2019 (eesponda)
*/
/********************************************************************************F*/
void NetGameDistServSetLoggingCallback(NetGameDistServT *pDistServ, NetGameDistServLoggingCbT *pLoggingCb, void *pUserData)
{
    pDistServ->pLoggingCb = pLoggingCb;
    pDistServ->pUserData = pUserData;
}
