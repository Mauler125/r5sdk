/*H********************************************************************************/
/*!
    \File netconnunix.c

    \Description
        Provides network setup and teardown support. Does not actually create any
        kind of network connections.

    \Copyright
        Copyright (c) 2010 Electronic Arts Inc.

    \Version 04/05/2010 (jbrookes) First version; a vanilla port to Unix from PS3
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtycert.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/proto/protoupnp.h"
#include "netconncommon.h"

#ifdef DIRTYCODE_APPLEIOS

#include "DirtySDK/dirtysock/iphone/netconnios.h"

#endif

/*** Defines **********************************************************************/

//! UPNP port
#define NETCONN_DEFAULT_UPNP_PORT       (3659)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! private module state
typedef struct NetConnRefT
{
    NetConnCommonRefT Common;           //!< cross-platform netconn data (must come first!)

    enum
    {
        ST_INIT,                        //!< initialization
        ST_CONN,                        //!< bringing up network interface
        ST_IDLE,                        //!< active
    } eState;                           //!< internal connection state

    uint32_t        uConnStatus;        //!< connection status (surfaced to user)

    ProtoUpnpRefT   *pProtoUpnp;        //!< protoupnp module state
    int32_t         iPeerPort;          //!< peer port to be opened by upnp; if zero, still find upnp router but don't open a port
    int32_t         iNumProcCores;      //!< number of processor cores on the system
    int32_t         iThreadCpuAffinity; //!< cpu affinity used for our internal threads
} NetConnRefT;

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

//! global module ref
static NetConnRefT *_NetConn_pRef = NULL;

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function   _NetConnUpdateConnStatus

    \Description
        Update the Connection Status and provide logging of status changes

    \Input *pRef             - pointer to net NetConn module ref
    \Input *uNewConnStatus   - the new conn status

    \Version 01/19/2015 (tcho)
*/
/********************************************************************************F*/
static void _NetConnUpdateConnStatus(NetConnRefT *pRef, uint32_t uNewConnStatus)
{
    #if DIRTYCODE_LOGGING
    int32_t iIndex;
    char strConnStatus[5];

    for (iIndex = 0; iIndex < 4; ++iIndex)
    {
        strConnStatus[iIndex] = ((char *) &uNewConnStatus)[3 - iIndex];
    }

    strConnStatus[4] = 0;

    NetPrintf(("netconnunix: netconn status changed to %s\n", strConnStatus));
    #endif

    pRef->uConnStatus = uNewConnStatus;
}

#if defined(DIRTYCODE_LINUX)
/*F********************************************************************************/
/*!
    \Function _NetConnGetProcLine

    \Description
        Parse a single line of the processor entry file.

    \Input *pLine   - pointer to line to parse
    \Input *pName   - [out] buffer to store parsed field name
    \Input iNameLen - size of name buffer
    \Input *pData   - [out] buffer to store parsed field data
    \Input iDataLen - size of data buffer

    \Version 04/26/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnGetProcLine(const char *pLine, char *pName, int32_t iNameLen, char *pData, int32_t iDataLen)
{
    const char *pParse;

    // skip to end of field name
    for (pParse = pLine;  (*pParse != '\t') && (*pParse != ':'); pParse += 1)
        ;

    // copy field name
    ds_strsubzcpy(pName, iNameLen, pLine, pParse - pLine);

    // skip to field value
    for ( ; (*pParse == ' ') || (*pParse == '\t') || (*pParse == ':'); pParse += 1)
        ;

    // find end of field value
    for (pLine = pParse; (*pParse != '\n') && (*pParse != '\0'); pParse += 1)
        ;

    // copy field value
    ds_strsubzcpy(pData, iDataLen, pLine, pParse - pLine);
}
#endif

#if defined(DIRTYCODE_LINUX)
/*F********************************************************************************/
/*!
    \Function _NetConnGetProcRecord

    \Description
        Parse a single proc record.

    \Input *pProcFile   - proc record file pointer

    \Output
        int32_t         - one=success, zero=no more entries

    \Version 04/26/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetConnGetProcRecord(FILE *pProcFile)
{
    char strBuf[1024], strName[32], strValue[128], *pLine;
    while (((pLine = fgets(strBuf, sizeof(strBuf), pProcFile)) != NULL) && (strBuf[0] != '\n'))
    {
        _NetConnGetProcLine(strBuf, strName, sizeof(strName), strValue, sizeof(strValue));
    }
    return(pLine != NULL);
}
#endif

/*F********************************************************************************/
/*!
    \Function _NetConnGetNumProcs

    \Description
        Get the number of processors on the system.

    \Output
        int32_t     - number of processors in system, or <=0 on failure

    \Version 04/26/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetConnGetNumProcs(void)
{
    #if defined(DIRTYCODE_LINUX)
    FILE *pFile = fopen("/proc/cpuinfo", "r");
    int32_t iNumProcs = -1;

    if (pFile != NULL)
    {
        for (iNumProcs = 0; _NetConnGetProcRecord(pFile) != 0; iNumProcs += 1)
            ;
        fclose(pFile);

        NetPrintf(("netconnunix: parsed %d processor cores from cpuinfo\n", iNumProcs));
    }
    else
    {
        NetPrintf(("netconnunix: could not open proc file\n"));
    }
    return(iNumProcs);
    #else
    return(-1);
    #endif
}

/*F********************************************************************************/
/*!
    \Function _NetConnGetInterfaceType

    \Description
        Get interface type and return it to caller.

    \Input *pRef    - module state

    \Output
        uint32_t    - interface type bitfield (NETCONN_IFTYPE_*)

    \Version 10/08/2009 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _NetConnGetInterfaceType(NetConnRefT *pRef)
{
    uint32_t uIfType = NETCONN_IFTYPE_ETHER;

#if defined(DIRTYCODE_ANDROID)

    uIfType = NETCONN_IFTYPE_NONE;

    if (SocketInfo(NULL, 'eth0', 0, NULL, 0) == 0)
    {
        uIfType = NETCONN_IFTYPE_WIRELESS;
    }

    if (SocketInfo(NULL, 'wan0', 0, NULL, 0) == 0)
    {
        uIfType = NETCONN_IFTYPE_CELL;
    }
    
#elif defined(DIRTYCODE_APPLEIOS)
    
    uIfType = NetConnStatusIos(&(_NetConn_pRef->Common), 'type', 0, NULL, 0);

#endif
    
    return(uIfType);
}

/*F********************************************************************************/
/*!
    \Function _NetConnUpdate

    \Description
        Update status of NetConn module.  This function is called by NetConnIdle.

    \Input *pData   - pointer to NetConn module ref
    \Input uTick    - current tick counter

    \Version 07/18/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnUpdate(void *pData, uint32_t uTick)
{
    NetConnRefT *pRef = (NetConnRefT *)pData;

    // perform idle processing
    SocketControl(NULL, 'idle', uTick, NULL, NULL);

    // wait for network active status
    if (pRef->eState == ST_CONN)
    {
        uint32_t uSocketConnStatus = SocketInfo(NULL, 'conn', 0, NULL, 0);
        
        if (pRef->uConnStatus != uSocketConnStatus)
        {
            _NetConnUpdateConnStatus(pRef, uSocketConnStatus);
        }

        if (pRef->uConnStatus == '+onl')
        {
            // discover upnp router information
            if (pRef->pProtoUpnp != NULL)
            {
                if (pRef->iPeerPort != 0)
                {
                    ProtoUpnpControl(pRef->pProtoUpnp, 'port', pRef->iPeerPort, 0, NULL);
                    ProtoUpnpControl(pRef->pProtoUpnp, 'macr', 'upnp', 0, NULL);
                }
                else
                {
                    ProtoUpnpControl(pRef->pProtoUpnp, 'macr', 'dscg', 0, NULL);
                }
            }

            pRef->eState = ST_IDLE;
        }
    }

    // update connection status while idle
    if (pRef->eState == ST_IDLE)
    {
        // update connection status if not already in an error state
        if ((pRef->uConnStatus >> 24) != '-')
        {
            uint32_t uSocketConnStat = SocketInfo(NULL, 'conn', 0, NULL, 0);
            
            if (pRef->uConnStatus != uSocketConnStat)
            {
                 _NetConnUpdateConnStatus(pRef, uSocketConnStat);
            }
        }
    }

    // if error status, go to idle state from any other state
    if ((pRef->eState != ST_IDLE) && (pRef->uConnStatus >> 24 == '-'))
    {
        pRef->eState = ST_IDLE;
    }
}

/*F********************************************************************************/
/*!
    \Function    _NetConnShutdownInternal

    \Description
        Shutdown the network code and return to idle state for internal use

    \Input  pRef              - netconn ref
    \Input  uShutdownFlags    - shutdown configuration flags

    \Output
        int32_t        - negative=error, else zero

    \Version 1/13/2020 (tcho)
*/
/********************************************************************************F*/
int32_t _NetConnShutdownInternal(NetConnRefT *pRef, uint32_t uShutdownFlags)
{
    int32_t iResult = 0;

    // decrement and check the refcount
    if ((iResult = NetConnCommonCheckRef((NetConnCommonRefT*)pRef)) < 0)
    {
        return(iResult);
    }

    if (_NetConn_pRef != NULL)
    {
        // disconnect network interfaces
        NetConnDisconnect();
    }

    // destroy the upnp ref
    if (pRef->pProtoUpnp != NULL)
    {
        ProtoUpnpDestroy(pRef->pProtoUpnp);
        pRef->pProtoUpnp = NULL;
    }

    // destroy the dirtycert module
    DirtyCertDestroy();

    // shut down protossl
    ProtoSSLShutdown();

    // remove netconn idle task
    NetConnIdleDel(_NetConnUpdate, pRef);

    // shut down Idle handler
    NetConnIdleShutdown();

    // shutdown the network code
    SocketDestroy(0);

    #ifdef DIRTYCODE_APPLEIOS

    //call ios NetConnShutdown
    NetConnShutdownIos(uShutdownFlags);

    #endif

    // common shutdown (must come last as this frees the memory)
    NetConnCommonShutdown(&pRef->Common);
    _NetConn_pRef = NULL;

    return(0);
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function    NetConnStartup

    \Description
        Bring the network connection module to life. Creates connection with IOP
        resources and gets things ready to go. Puts all device drivers into "probe"
        mode so they look for appropriate hardware. Does not actually start any
        network activity.

    \Input *pParams - startup parameters

    \Output
        int32_t     - zero=success, negative=failure

    \Notes
        NetConnRefT::iRefCount serves as a counter for the number of times
        NetConnStartup has been called. This allows us to track how many modules
        are using it and how many times we expect NetConnShutdown to the called.
        In the past we only allowed a single call to NetConnStartup but some
        libraries may need to networking without a guarentee that the game has
        already started it.

        pParams can contain the following terms:

        \verbatim
            -noupnp                 - disable upnp support
            -servicename            - set servicename <game-year-platform> required for SSL use
            -singlethreaded         - start DirtySock in single-threaded mode (typically when used in servers)
            -affinity=<mask as hex> - the cpu affinity for our internal threads
        \endverbatim

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnStartup(const char *pParams)
{
    NetConnRefT *pRef = _NetConn_pRef;
    int32_t iThreadPrio = 10;
    int32_t iRet = 0;
    int32_t iResult = 0;
    char strThreadCpuAffinity[16];
    
    // allow NULL params
    if (pParams == NULL)
    {
        pParams = "";
    }

    // debug display of input params
    NetPrintf(("netconnunix: startup params='%s'\n", pParams));
    
    // common startup
    // pRef shall hold the address of the NetConnRefT after completion if no error occured
    iResult = NetConnCommonStartup(sizeof(*pRef), pParams, (NetConnCommonRefT**)(&pRef));

    // treat the result of the common startup, if already started simply early out
    if (iResult == NETCONN_ERROR_ALREADY_STARTED)
    {
        return(0);
    }
    // otherwise, if an error occured report it
    else if (iResult < 0)
    {
        return(iResult);
    }

    pRef->eState = ST_INIT;
    pRef->iPeerPort = NETCONN_DEFAULT_UPNP_PORT;

    // check for singlethreaded mode
    if (strstr(pParams, "-singlethreaded"))
    {
        iThreadPrio = -1;
    }

    // get the thread cpu affinity setting from our startup params, defaulting to 0x0
    ds_memclr(strThreadCpuAffinity, sizeof(strThreadCpuAffinity));
    NetConnCopyParam(strThreadCpuAffinity, sizeof(strThreadCpuAffinity), "-affinity=", pParams, "0x0");
    pRef->iThreadCpuAffinity =(int32_t) strtol(strThreadCpuAffinity, NULL, 16);

    // create network instance
    if (SocketCreate(iThreadPrio, 0, pRef->iThreadCpuAffinity) != 0)
    {
        NetPrintf(("netconnunix: unable to start up dirtysock\n"));
        _NetConnShutdownInternal(pRef, 0);
        return(NETCONN_ERROR_SOCKET_CREATE);
    }

    // create and configure dirtycert
    if (NetConnDirtyCertCreate(pParams))
    {
        _NetConnShutdownInternal(pRef, 0);
        NetPrintf(("netconnunix: unable to create dirtycert\n"));
        return(NETCONN_ERROR_DIRTYCERT_CREATE);
    }

    // start up protossl
    if (ProtoSSLStartup() < 0)
    {
        _NetConnShutdownInternal(pRef, 0);
        NetPrintf(("netconnunix: unable to start up protossl\n"));
        return(NETCONN_ERROR_PROTOSSL_CREATE);
    }

    // create the upnp module
    if (!strstr(pParams, "-noupnp"))
    {
        pRef->pProtoUpnp = ProtoUpnpCreate();
        if (pRef->pProtoUpnp == NULL)
        {
            _NetConnShutdownInternal(pRef, 0);
            NetPrintf(("netconnunix: unable to start up protoupnp\n"));
            return(NETCONN_ERROR_PROTOUPNP_CREATE);
        }
    }

    // add netconn task handle
    if (NetConnIdleAdd(_NetConnUpdate, pRef) < 0)
    {
        _NetConnShutdownInternal(pRef, 0);
        NetPrintf(("netconnunix: unable to add netconn task handler\n"));
        return(NETCONN_ERROR_INTERNAL);
    }
    
#ifdef DIRTYCODE_APPLEIOS
    //call ios netconn startup
    if ((iRet = NetConnStartupIos(pParams)) < 0)
    {
        _NetConnShutdownInternal(pRef, 0);
        return(iRet);
    }
#endif

    // save ref
    _NetConn_pRef = pRef;
    
    return(iRet);
}

/*F********************************************************************************/
/*!
    \Function    NetConnQuery

    \Description
        Query the list of available connection configurations. This list is loaded
        from the specified device. The list is returned in a simple fixed width
        array with one string per array element. The caller can find the user portion
        of the config name via strchr(item, '#')+1.

    \Input *pDevice - device to scan (mc0:, mc1:, pfs0:, pfs1:)
    \Input *pList   - buffer to store output array in
    \Input iSize    - length of buffer in bytes

    \Output
        int32_t     - negative=error, else number of configurations

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnQuery(const char *pDevice, NetConfigRecT *pList, int32_t iSize)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function    NetConnConnect

    \Description
        Used to bring the networking online with a specific configuration. Uses a
        configuration returned by NetConnQuery.

    \Input *pConfig - unused
    \Input *pOption - asciiz list of config parameters
                      "peerport=<port>" to specify peer port to be opened by upnp.
    \Input iData    - platform-specific

    \Output
        int32_t     - negative=error, zero=success

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnConnect(const NetConfigRecT *pConfig, const char *pOption, int32_t iData)
{
    int32_t iResult = 0;
    NetConnRefT *pRef = _NetConn_pRef;

    // check connection options, if present
    if (pRef->eState == ST_INIT)
    {
        // check for connect options
        if (pOption != NULL)
        {
            const char *pOpt;

            // check for specification of peer port
            if ((pOpt = strstr(pOption, "peerport=")) != NULL)
            {
                pRef->iPeerPort = (int32_t)strtol(pOpt+9, NULL, 10);
            }
        }
        NetPrintf(("netconnunix: upnp peerport=%d %s\n",
            pRef->iPeerPort, (pRef->iPeerPort == NETCONN_DEFAULT_UPNP_PORT ? "(default)" : "(selected via netconnconnect param)")));

        // start up network interface
        SocketControl(NULL, 'conn', 0, NULL, NULL);

        // transition to connecting state
        pRef->eState = ST_CONN;
    }
    else
    {
        NetPrintf(("netconnunix: NetConnConnect() ignored because already connected!\n"));
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function    NetConnDisconnect

    \Description
        Used to bring down the network connection. After calling this, it would
        be necessary to call NetConnConnect to bring the connection back up or
        NetConnShutdown to completely shutdown all network support.

    \Output
        int32_t     - negative=error, zero=success

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnDisconnect(void)
{
    NetConnRefT *pRef = _NetConn_pRef;

    // shut down networking
    if (pRef->eState != ST_INIT)
    {
        // bring down network interface
        SocketControl(NULL, 'disc', 0, NULL, NULL);

        // reset status
        pRef->eState = ST_INIT;
        pRef->uConnStatus = 0;
    }

    // abort upnp operations
    if (pRef->pProtoUpnp != NULL)
    {
        ProtoUpnpControl(pRef->pProtoUpnp, 'abrt', 0, 0, NULL);
    }

    // done
    return(0);
}

/*F********************************************************************************/
/*!
    \Function NetConnControl

    \Description
        Set module behavior based on input selector.

    \Input  iControl    - input selector
    \Input  iValue      - selector input
    \Input  iValue2     - selector input
    \Input *pValue      - selector input
    \Input *pValue2     - selector input

    \Output
        int32_t         - selector result

    \Notes
        iControl can be one of the following:

        \verbatim
            snam: set DirtyCert service name
        \endverbatim

        Unhandled selectors are passed through to NetConnCommonControl()

    \Version 1.0 04/27/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnControl(int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue, void *pValue2)
{
    NetConnRefT *pRef = _NetConn_pRef;

    // make sure module is started before allowing any other control calls
    if (pRef == NULL)
    {
        NetPrintf(("netconnunix: warning - calling NetConnControl() while module is not initialized\n"));
        return(-1);
    }

    // set dirtycert service name
    if (iControl == 'snam')
    {
        return(DirtyCertControl('snam', 0, 0, pValue));
    }
    
    // pass through unhandled selectors to NetConnCommon
    return(NetConnCommonControl(&pRef->Common, iControl, iValue, iValue2, pValue, pValue2));
}

/*F********************************************************************************/
/*!
    \Function    NetConnStatus

    \Description
        Check general network connection status. Different selectors return
        different status attributes.

    \Input iKind    - status selector ('open', 'conn', 'onln')
    \Input iData    - (optional) selector specific
    \Input *pBuf    - (optional) pointer to output buffer
    \Input iBufSize - (optional) size of output buffer

    \Output
        int32_t     - selector specific

    \Notes
        iKind can be one of the following:

        \verbatim
            addr: ip address of client
            affn: thread cpu affinity setting
            bbnd: TRUE if broadband, else FALSE
            conn: connection status: +onl=online, ~<code>=in progress, -<err>=NETCONN_ERROR_*
            hwid: (IOS only) This will return the vendor id in pBuf. pBuf must be at least 17 byte.
            macx: MAC address of client (returned in pBuf)
            ncon: returns the network connectivity level (iOS and Android Only)
            onln: true/false indication of whether network is operational
            open: true/false indication of whether network code running
            proc: number of processor cores on the system (Linux only)
            type: connection type: NETCONN_IFTYPE_* bitfield
            upnp: return protoupnp external port info, if available
            vers: return DirtySDK version
        \endverbatim

        Unhandled selectors are passed through to NetConnCommonStatus()

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnStatus(int32_t iKind, int32_t iData, void *pBuf, int32_t iBufSize)
{
    NetConnRefT *pRef = _NetConn_pRef;

    // initialize output buffer
    if (pBuf != NULL)
    {
        ds_memclr(pBuf, iBufSize);
    }

    // see if network code is initialized
    if (iKind == 'open')
    {
        return(pRef != NULL);
    }
    // return DirtySDK version
    if (iKind == 'vers')
    {
        return(DIRTYSDK_VERSION);
    }

    // make sure module is started before allowing any other status calls
    if (pRef == NULL)
    {
        NetPrintf(("netconnunix: warning - calling NetConnStatus() while module is not initialized\n"));
        return(-1);
    }

    // return the thread cpu affinity setting
    if (iKind == 'affn')
    {
        return(pRef->iThreadCpuAffinity);
    }

    // return broadband (TRUE/FALSE)
    if (iKind == 'bbnd')
    {
        return(TRUE);
    }
    // connection status
    if (iKind == 'conn')
    {
        return(pRef->uConnStatus);
    }
    // see if connected to ISP/LAN
    if (iKind == 'onln')
    {
        return(pRef->uConnStatus == '+onl');
    }

    // return number of processor cores
    if (iKind == 'proc')
    {
        if (pRef->iNumProcCores == 0)
        {
            pRef->iNumProcCores = _NetConnGetNumProcs();
        }
        return(pRef->iNumProcCores);
    }
    // return interface type (more verbose)
    if (iKind == 'type')
    {
        return(_NetConnGetInterfaceType(pRef));
    }
    // return upnp addportmap info, if available
    if (iKind == 'upnp')
    {
        // if protoupnp is available, and we've added a port map, return the external port for the port mapping
        if ((pRef->pProtoUpnp != NULL) && (ProtoUpnpStatus(pRef->pProtoUpnp, 'stat', NULL, 0) & PROTOUPNP_STATUS_ADDPORTMAP))
        {
            return(ProtoUpnpStatus(pRef->pProtoUpnp, 'extp', NULL, 0));
        }
    }
#ifdef DIRTYCODE_ANDROID

    if (iKind == 'ncon')
    {
        if ((SocketInfo(NULL, 'eth0', 0, NULL, 0) == 0) || SocketInfo(NULL, 'wan0', 0, NULL, 0) == 0)
        {
            return(TRUE);
        }
        else
        {
            return(FALSE);
        }
    }

#endif
    

#ifdef DIRTYCODE_APPLEIOS

    //pass unrecgnized options to NetConnStatusIos and Socket Info
    return(NetConnStatusIos(&pRef->Common, iKind, iData, pBuf, iBufSize));

#else

    // pass unrecognized options to NetConnCommon
    return(NetConnCommonStatus(&pRef->Common, iKind, iData, pBuf, iBufSize));

#endif

}

/*F********************************************************************************/
/*!
    \Function    NetConnShutdown

    \Description
        Shutdown the network code and return to idle state.

    \Input  uShutdownFlags  - shutdown configuration flags

    \Output
        int32_t             - negative=error, else zero

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnShutdown(uint32_t uShutdownFlags)
{
    NetConnRefT *pRef = _NetConn_pRef;

    // make sure we've been started
    if (pRef == NULL)
    {
        return(NETCONN_ERROR_NOTACTIVE);
    }

    return(_NetConnShutdownInternal(pRef, uShutdownFlags));
}

/*F********************************************************************************/
/*!
    \Function    NetConnSleep

    \Description
        Sleep the application for some number of milliseconds.

    \Input iMilliSecs    - number of milliseconds to block for

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
void NetConnSleep(int32_t iMilliSecs)
{
    usleep(iMilliSecs*1000);
}

