/*H*************************************************************************************************/
/*!

    \File    netconn.c

    \Description
        Provides network setup and teardown support. Does not actually create any connections.

    \Notes
         None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2001-2017.    ALL RIGHTS RESERVED.

    \Version    1.0        03/12/01 (GWS) First Version

*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtycert.h"
#include "DirtySDK/dirtysock/netconn.h"

/*** Defines **************************************************************************************/

//! maximum number of idle handlers that may be registered
#define NETCONN_MAXIDLEHANDLERS     (64)

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Function Prototypes **************************************************************************/

/*** Variables ************************************************************************************/

static struct
{
    void (*proc)(void *pData, uint32_t uTick);
    void *data;
} _NETidle[NETCONN_MAXIDLEHANDLERS];

static const char _NetConn_HexEncode[16] = "0123456789abcdef";

static uint32_t _NetConn_uLastIdleTick = 0;
static uint8_t _NetConn_bTickInitialized = FALSE;
static uint8_t _NetConn_bNetConnIdleTiming = FALSE;
static uint32_t _NetConn_uMachineId = 0;

/*** Private Functions ****************************************************************************/


/*F********************************************************************************/
/*!
     \Function _NetConnIdleProcs

     \Description
          Process any registered idle handlers.  This is for internal use only.

     \Input uCurTick    - current millisecond tick count

     \Output
         None.

     \Version 05/26/2005 (doneill)
*/
/********************************************************************************F*/
static void _NetConnIdleProcs(uint32_t uCurTick)
{
    int32_t iProc;

    // call other idle handlers
    for (iProc = 0; iProc < NETCONN_MAXIDLEHANDLERS; iProc++)
    {
        if (_NETidle[iProc].proc != NULL)
        {
            (_NETidle[iProc].proc)(_NETidle[iProc].data, uCurTick);
        }
    }
}

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function _NetConnMonitorRate

    \Description
         Monitor the rate at which NetconnIdle is called. Output diagnosis messages

    \Input uCurrTick   - current millisecond tick count
           iTickDiff   - tick difference between previous call

    \Output
        None.

    \Version 06/11/2009 (jrainy)
*/
/********************************************************************************F*/
static void _NetConnMonitorRate(uint32_t uCurrTick, int32_t iTickDiff)
{
    static int32_t iMaxDiff = 0;
    static int32_t iMinDiff = 0;
    static int32_t iRunningCount = 0;
    static int32_t uStartTick = 0;
    static int32_t uLastTick = 0;

    if ((iTickDiff < iMinDiff) || (iTickDiff > iMaxDiff) || (NetTickDiff(uCurrTick, uStartTick) > 2000))
    {
        if (iRunningCount)
        {
            NetPrintf(("netconn: %d netconnidles of %d to %d ms between times %d and %d.\n", iRunningCount, iMinDiff, iMaxDiff, uStartTick, uLastTick));
            uStartTick = uLastTick;
        }
        else
        {
            uStartTick = uCurrTick;
        }
        iRunningCount = 1;
        iMaxDiff = (iTickDiff * 15) / 10;
        iMinDiff = (iTickDiff * 5) / 10;
    }
    else
    {
        iRunningCount++;
    }
    uLastTick = uCurrTick;
}
#endif

/*** Public functions *****************************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    NetConnMAC

    \Description
        Return the network MAC address string to the caller

    \Output
        const char * - textual MAC address string

    \Version    1.0        04/29/01 (GWS) First Version
*/
/*************************************************************************************************F*/
const char *NetConnMAC(void)
{
    static union
    {
        unsigned char uMac[16];
    } Stack;
    static char strMAC[16] = "";

    // see if we need to query mac address
    if (strMAC[0] == 0)
    {
        // get MAC address
        if (NetConnStatus('macx', 0, Stack.uMac, sizeof(Stack.uMac)) >= 0)
        {
            // format into string
            strMAC[0] = '$';
            strMAC[1] = _NetConn_HexEncode[Stack.uMac[0]>>4];
            strMAC[2] = _NetConn_HexEncode[Stack.uMac[0]&15];
            strMAC[3] = _NetConn_HexEncode[Stack.uMac[1]>>4];
            strMAC[4] = _NetConn_HexEncode[Stack.uMac[1]&15];
            strMAC[5] = _NetConn_HexEncode[Stack.uMac[2]>>4];
            strMAC[6] = _NetConn_HexEncode[Stack.uMac[2]&15];
            strMAC[7] = _NetConn_HexEncode[Stack.uMac[3]>>4];
            strMAC[8] = _NetConn_HexEncode[Stack.uMac[3]&15];
            strMAC[9] = _NetConn_HexEncode[Stack.uMac[4]>>4];
            strMAC[10] = _NetConn_HexEncode[Stack.uMac[4]&15];
            strMAC[11] = _NetConn_HexEncode[Stack.uMac[5]>>4];
            strMAC[12] = _NetConn_HexEncode[Stack.uMac[5]&15];
            strMAC[13] = 0;
        }
    }

    return(strMAC);
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnElapsed

    \Description
        Return elapsed time in milliseconds. The epoch (zero time) is not defined.
        This function should be used to determine elapsed time by calling once,
        saving the result, then later calling again and subtracting the original
        result from the new result. This will give the elapsed time in millisecs.

    \Output
        uint32_t     - elapsed milliseconds

    \Version    1.0        03/10/01 (GWS) First Version
*/
/*************************************************************************************************F*/
uint32_t NetConnElapsed(void)
{
    return(NetTick());
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnIdleAdd

    \Description
        Add an idle handler that will get called periodically. The frequency of calls
        is not guarenteed, but should be 60Hz min.

    \Input *proc    - callback function
    \Input *data    - reference data provided to callback function

    \Output
        int32_t       - negative=error, zero=success

    \Version    1.0        03/10/01 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t NetConnIdleAdd(void (*proc)(void *data, uint32_t tick), void *data)
{
    int32_t i;

    // locate slot and add
    for (i = 0; i < NETCONN_MAXIDLEHANDLERS; ++i)
    {
        // make sure it's not already added
        if ((_NETidle[i].proc == proc) && (_NETidle[i].data == data))
        {
            NetPrintf(("netconn: ignoring add of an idle handler that is already registered\n"));
            return(-1);
        }

        // if there's space in the table, add the function
        if (_NETidle[i].proc == NULL)
        {
            _NETidle[i].proc = proc;
            _NETidle[i].data = data;
            return(0);
        }
    }

    // warn the user that the add failed
    NetPrintf(("netconn: unable to add new idle handler as table is full\n"));

    // no space in table
    return(-2);
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnIdleDel

    \Description
        Remove a previously added idle handler. The data parameter must match what
        was given when the function was added.

    \Input *proc    - callback function
    \Input *data    - same value given to NetConnIdleAdd

    \Output
        int32_t       - negative=error, zero=success

    \Version    1.0       03/10/01 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t NetConnIdleDel(void (*proc)(void *data, uint32_t tick), void *data)
{
    int32_t i;

    // locate slot and del
    for (i = 0; i < NETCONN_MAXIDLEHANDLERS; ++i)
    {
        if ((_NETidle[i].proc == proc) && (_NETidle[i].data == data))
        {
            _NETidle[i].proc = NULL;
            _NETidle[i].data = NULL;
            return(0);
        }
    }

    // warn the user the handler did not exist
    NetPrintf(("netconn: ignoring delete of an idle handler that is not registered\n"));

    // not in table
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnIdle

    \Description
        Provide "life" to the network code.  This function needs to be called periodically
        (typically once per game loop, generally speaking 5-10hz minimum) for the network code to
        function properly.

    \Version    1.0        03/10/01 (GWS) First Version
*/
/*************************************************************************************************F*/
void NetConnIdle(void)
{
    uint32_t uCurTick;

    // make sure module is available
    if (NetConnStatus('open', 0, NULL, 0) == 0)
    {
        return;
    }

    // get current tick count
    uCurTick = NetTick();

    // initialized last tick counter?
    if (_NetConn_bTickInitialized == FALSE)
    {
        _NetConn_uLastIdleTick = uCurTick - 5;
        _NetConn_bTickInitialized = TRUE;
    }

    // debug timing of idle rate
    #if DIRTYCODE_LOGGING
    if (_NetConn_bNetConnIdleTiming)
    {
        int32_t iTickDiff = NetTickDiff(uCurTick, _NetConn_uLastIdleTick);
        _NetConnMonitorRate(uCurTick, iTickDiff);
    }
    #endif

    _NetConn_uLastIdleTick = uCurTick;

    // call registered idle handlers
    _NetConnIdleProcs(uCurTick);
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnIdleShutdown

    \Description
        Shut down the NetConnIdle handler.
        This function is intended for internal use only.  It should not be called by an application.

    \Version    1.0        06/16/04 (JLB) First Version
*/
/*************************************************************************************************F*/
void NetConnIdleShutdown(void)
{
    int32_t iProc;

    for (iProc = 0; iProc < NETCONN_MAXIDLEHANDLERS; iProc++)
    {
        if (_NETidle[iProc].proc != NULL)
        {
            NetPrintf(("netconn: removing idle handler at shutdown\n"));
            _NETidle[iProc].proc = NULL;
            _NETidle[iProc].data = NULL;
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnTiming

    \Description
        Enable or disable the timing of netconnidles

    \Input uEnableTiming    - on or off

    \Output
        None.

    \Version    1.0        11/06/09 (jrainy) First Version
*/
/*************************************************************************************************F*/
void NetConnTiming(uint8_t uEnableTiming)
{
    _NetConn_bNetConnIdleTiming = uEnableTiming;
}

#if DIRTYCODE_LOGGING
#define MONITOR_SIZE 200
#define MONITOR_VARIABLES 64
#define MONITOR_NAME_SIZE 64
void NetConnMonitorValue(const char* pName, int32_t iValue)
{
    int32_t iIndex, iPrintIndex;
    static char strNames[MONITOR_VARIABLES][MONITOR_NAME_SIZE] = {{0}};
    static int32_t iMonitoredValues[MONITOR_VARIABLES][MONITOR_SIZE] = {{0}};
    static int32_t iMonitoredCount[MONITOR_VARIABLES] = {0};

    for(iIndex = 0; iIndex < MONITOR_VARIABLES; iIndex++)
    {
        if ((!ds_strnicmp(pName, strNames[iIndex], MONITOR_NAME_SIZE)) || (strNames[iIndex][0] == 0))
        {
            break;
        }
    }

    if (iIndex == MONITOR_VARIABLES)
    {
        NetPrintf(("netconn: too many monitored variables\n"));
        return;
    }

    if (!strNames[iIndex][0])
    {
        ds_strnzcpy(strNames[iIndex], pName, MONITOR_NAME_SIZE);
    }

    iMonitoredValues[iIndex][iMonitoredCount[iIndex]++] = iValue;

    if (iMonitoredCount[iIndex] == MONITOR_SIZE)
    {
        NetPrintf(("NetConn: displaying monitored values \"%s\"\n", strNames[iIndex]));
        for(iPrintIndex = 0; iPrintIndex < MONITOR_SIZE; iPrintIndex += 10)
        {
            NetPrintf(("%d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
                iMonitoredValues[iIndex][iPrintIndex + 0],
                iMonitoredValues[iIndex][iPrintIndex + 1],
                iMonitoredValues[iIndex][iPrintIndex + 2],
                iMonitoredValues[iIndex][iPrintIndex + 3],
                iMonitoredValues[iIndex][iPrintIndex + 4],
                iMonitoredValues[iIndex][iPrintIndex + 5],
                iMonitoredValues[iIndex][iPrintIndex + 6],
                iMonitoredValues[iIndex][iPrintIndex + 7],
                iMonitoredValues[iIndex][iPrintIndex + 8],
                iMonitoredValues[iIndex][iPrintIndex + 9]));
        }
        iMonitoredCount[iIndex] = 0;
    }



}

#endif

/*F********************************************************************************/
/*!
    \Function NetConnMachineId

    \Description
        Gets a unique id for this machine.

    \Output
        uint32_t        - machine id of this machine.

    \Version 01/31/2014 (cvienneau)
*/
/********************************************************************************F*/
uint32_t NetConnMachineId(void)
{
    return(_NetConn_uMachineId);
}

/*F********************************************************************************/
/*!
    \Function NetConnSetMachineId

    \Description
        Sets a unique id for this machine.

    \Input uMachineId        - new value

    \Version 01/31/2014 (cvienneau)
*/
/********************************************************************************F*/
void NetConnSetMachineId(uint32_t uMachineId)
{
    _NetConn_uMachineId = uMachineId;
    NetPrintf(("netconn: machineId set to %x\n", uMachineId));
}


/*F********************************************************************************/
/*!
    \Function NetConnCopyParam

    \Description
        Copy a command-line parameter.

    \Input *pDst        - output buffer
    \Input iDstLen      - output buffer length
    \Input *pParamName  - name of parameter to check for
    \Input *pSrc        - input string to look for parameters in
    \Input *pDefault    - default string to use if paramname not found

    \Output
        int32_t         - number of bytes written

    \Version 07/18/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnCopyParam(char *pDst, int32_t iDstLen, const char *pParamName, const char *pSrc, const char *pDefault)
{
    int32_t iIndex;

    // find parameter
    if ((pSrc = strstr(pSrc, pParamName)) == NULL)
    {
        // copy in default
        ds_strnzcpy(pDst, pDefault, iDstLen);
        return((int32_t)strlen(pDefault));
    }

    // skip parameter name
    pSrc += strlen(pParamName);

    // make sure buffer has enough room
    if (--iDstLen < 0)
    {
        return(0);
    }

    // copy the string
    for (iIndex = 0; (iIndex < iDstLen) && (pSrc[iIndex] != '\0') && (pSrc[iIndex] != ' '); iIndex++)
    {
        pDst[iIndex] = pSrc[iIndex];
    }

    // write null terminator and return number of bytes written
    pDst[iIndex] = '\0';
    return(iIndex);
}

/*F********************************************************************************/
/*!
    \Function NetConnDirtyCertCreate

    \Description
        Create DirtyCert, intitialize service name if provided

    \Input *pParams - input params

    \Output
        int32_t     - negative=failure, else success

    \Version 03/28/2013 (jbrookes)
    */
/********************************************************************************F*/
int32_t NetConnDirtyCertCreate(const char *pParams)
{
    char strServiceName[DIRTYCERT_SERVICENAME_SIZE];

    // create the dirtycert module
    if (DirtyCertCreate() != 0)
    {
        NetConnShutdown(0);
        NetPrintf(("netconn: unable to create dirtycert\n"));
        return(-1);
    }
    // check for servicename
    if (strstr(pParams, "-servicename=") != NULL)
    {
        // get service name
        NetConnCopyParam(strServiceName, sizeof(strServiceName), "-servicename=", pParams, "");
        // set service name in dirtycert
        DirtyCertControl('snam', 0, 0, strServiceName);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function NetConnGetEnvStr

    \Description
        Translate specified NETCONN_PLATENV_ define into an environment string

    \Output
        const char *    - pointer to env str

    \Version 07/09/2013 (jbrookes)
    */
/********************************************************************************F*/
const char *NetConnGetEnvStr(void)
{
    const char *pEnv;
    switch(NetConnStatus('envi', 0, NULL, 0))
    {
        case NETCONN_PLATENV_DEV:
        pEnv = "dev";
        break;

        case NETCONN_PLATENV_TEST:
        pEnv = "test";
        break;

        case NETCONN_PLATENV_CERT:
        pEnv = "cert";
        break;

        case NETCONN_PLATENV_PROD:
        pEnv = "prod";
        break;

        default:
        NetPrintf(("netconn: could not get env str\n"));
        pEnv = "unkn";
        break;
    }
    return(pEnv);
}

