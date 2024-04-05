/*H********************************************************************************/
/*!
    \File protoupnp.c

    \Description
        Implements a simple UPnP client, designed specifically to talk to a UPnP
        router and open up a firewall port for peer-peer communication with a
        remote client.

    \Notes
        ProtoUpnp implementation was based on the following documents:

        References:
            [1] UPnP Device Architecture: http://www.upnp.org/specs/arch/UPnP-arch-DeviceArchitecture-v1.0-20080424.pdf
            [2] UPnP Resources: http://upnp.org/sdcps-and-certification/resources/
            [3] Intel Software for UPnP Technology: http://software.intel.com/en-us/articles/intel-software-for-upnp-technology-download-tools/
            [4] Internet Gateway Device v1.0: http://upnp.org/specs/gw/igd1/
            [5] SOAP 1.2: http://www.w3.org/TR/2003/REC-soap12-part0-20030624/
            [6] UPnP compatibility list (old): http://ccgi.mgillespie.plus.com/upnp/test.php

        In addition, many thanks to the Burnout3 team, whose UPnP implementation
        was used as reference.

        UPnP Optional Features:
            [a] - Finite Lease Duration
            [b] - Wildcard in Source IP (Remote Host)
            [c] - Wildcard in External Port
            [d] - Non-matching Internal and External Ports
            [e] - Specific Remote Host
            [f] - Specific External Port

        Tested against the following routers:
            <1> Linksys WRT54G 3.03.06 [b,c,d,e,f]
            <2> Linksys BEFSR41 [b,c?,d,e,f] [c? because GetSpecificPortMapping with wc port fails)
            <3> Netgear WGR614v6 1.0.8 [b,d,f]

        Common UPnP SOAP errors we have seen in usage:

            402: Invalid Args
            403: Undefined
            501: Action Failed
            713: SpecifiedArrayIndexInvalid (GetGenericPortMapping return code, but we don't use that method?)
            714: NoSuchEntryInArray (GetSpecificPortMapping/DeletePortMapping return code, should not be fatal)
            716: WildCardNotPermittedInExtPort (AddPortMapping return code, but we don't ask for a wildcard external port?)
            718: ConflictInMappingEntry (AddPortMapping return code)

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/23/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/xml/xmlparse.h"

#include "DirtySDK/proto/protoupnp.h"

/*** Defines **********************************************************************/

//! define this to allow reuse of a preexisting mapping that matches what we want
#define PROTOUPNP_REUSEMAPPING      (FALSE)

//! define this to use WAN IP address as the source address in the port map if a remote host is unspecified
#define PROTOMANGLE_USEWANIPSRCADDR (FALSE)

//! define this to use verbose variable descriptions in requests
#define PROTOUPNP_FULLVARDESC       (FALSE)

//! addr to send discovery requests to (239.255.255.250)
#define PROTOUPNP_DISCOVERYADDR     (0xEFFFFFFA)

//! port to send discovery requests to
#define PROTOUPNP_DISCOVERYPORT     (1900)

//! interval at which we send out discovery request broadcasts
#define PROTOUPNP_DISCOVERYINTERVAL (15000)

//! maximum number of services parsed
#define PROTOUPNP_MAXSERVICES       (5)

//! default port to map
#define PROTOUPNP_DEFAULTPORTMAP    (3658)

/*** Macros ***********************************************************************/

//! add a soap item of type string
#define _ProtoUpnpSoapRequestAddStr(_pUpnp, _pName, _pData) _ProtoUpnpSoapRequestAdd(_pUpnp, _pName, "string", _pData)

//! add a soap item of type ui2
#define _ProtoUpnpSoapRequestAddUI2(_pUpnp, _pName, _uData) _ProtoUpnpSoapRequestAdd(_pUpnp, _pName, "ui2", _ProtoUpnpIntToStr(_uData))

//! add a soap item of type ui4
#define _ProtoUpnpSoapRequestAddUI4(_pUpnp, _pName, _uData) _ProtoUpnpSoapRequestAdd(_pUpnp, _pName, "ui4", _ProtoUpnpIntToStr(_uData))

//! add a soap item of type boolean
#define _ProtoUpnpSoapRequestAddBln(_pUpnp, _pName, _uData) _ProtoUpnpSoapRequestAdd(_pUpnp, _pName, "boolean", _ProtoUpnpIntToStr(_uData))

/*** Type Definitions *************************************************************/

//! port mapping description
typedef struct ProtoUpnpPortMapT
{
    int32_t iAddr;
    int32_t iPort;
    int32_t iLeaseDuration;
    char strDesc[31];
    uint8_t bEnabled;
} ProtoUpnpPortMapT;

//! upnp service info
typedef struct ProtoUpnpServiceT
{
    char strServiceType[64];                //!< service type description
    char strDescriptionUrl[128];            //!< service description URL
    char strControlUrl[256];                //!< service control url
} ProtoUpnpServiceT;

//! upnp device info
typedef struct ProtoUpnpDeviceT
{
    char                strUrl[128];        //!< string holding device description url
    char                strUrlBase[64];     //!< base url, with addr/port info
    char                strUdn[64];         //!< device universal device name
    char                strModel[127];      //!< device manufacturer and model
    uint8_t             bDiscovered;        //!< TRUE if we've discovered a UPnP device
    uint32_t            uExternalAddress;   //!< external address of device
    ProtoUpnpPortMapT   CurPortMap;         //!< current port mapping
    int32_t             iNumServices;       //!< number of services device supports
    ProtoUpnpServiceT   Services[PROTOUPNP_MAXSERVICES]; //!< list of services device supports
} ProtoUpnpDeviceT;

//! module state
struct ProtoUpnpRefT
{
    int32_t         iRefCount;              //!< module ref count

    // module memory group
    int32_t         iMemGroup;              //!< module mem group id
    void            *pMemGroupUserData;     //!< user data associated with mem group

    SocketT         *pUdp;                  //!< udp socket, used for discovery process
    ProtoHttpRefT   *pProtoHttp;            //!< http module, used for device communication

    int32_t         iClientAddr;            //!< local address

    int32_t         iDiscoveryTimer;        //!< timer used to time discovery sends
    struct sockaddr DiscoveryAddr;          //!< address to send discovery to

    int32_t         iStatus;                //!< current status (PROTOUPNP_STATUS_*)

    int32_t         iService;               //!< current service index

    ProtoUpnpDeviceT Device;

    enum
    {
        ST_IDLE,                            //!< idle state
        ST_DISCOVERY,                       //!< UPnP discovery
        ST_DESCRIPTION,                     //!< UPnP description
        ST_SERVICEDESC,                     //!< service description
        ST_GETSTATEVAR,                     //!< QueryStateVariable request
        ST_GETEXTADDR,                      //!< GetExternalAddress request
        ST_GETPORTMAPPING,                  //!< GetSpecificPortMapping request
        ST_DELPORTMAPPING,                  //!< DeletePortMapping request
        ST_ADDPORTMAPPING,                  //!< AddPortMapping request

        ST_LAST,                            //!< last state
    } eState;

    int32_t         iSendSize;              //!< size of current send buffer
    int32_t         iSentBytes;             //!< amount of data that we have sent
    int32_t         iHttpError;             //!< most recent ProtoHttpRecv() error, if any
    int32_t         iSoapError;             //!< SOAP error

    int32_t         iRemoteHost;            //!< address of remote host
    int32_t         iPortToMapExt;          //!< external port we are trying to map
    int32_t         iPortToMapInt;          //!< internal port we are trying to map
    int32_t         iLeaseDuration;         //!< lease duration to try and set

    const ProtoUpnpMacroT *pCommandList;    //!< pointer to current command list, if any

    uint8_t         bRequestInProgress;     //!< TRUE if a device request is in progress
    uint8_t         bEnableMapping;         //!< TRUE to enable mapping, else FALSE
    uint8_t         bPortIsMapped;          //!< TRUE if port is already mapped
    uint8_t         bVerbose;               //!< TRUE to set verbose mode, else FALSE

    #if DIRTYCODE_DEBUG
    uint8_t         bFakeResponse;          //!< TRUE if faking a response, else FALSE
    #endif

    char            strRequestName[64];     //!< name of current request
    char            strSendBuf[2048];       //!< buffer to construct posts in
    char            strRecvBuf[16*1024];    //!< buffer to receive into

    NetCritT        Crit;                   //!< critical section
};

/*** Variables ********************************************************************/

/*! protoupnp description, sent in soap requests.  note that this string is deliberately
    short because some implementations (e.g. BEFSR41) only store a small number of
    characters (e.g. 11) for the mapping description */
static const char _ProtoUpnp_strDescription[] = "EA Tunnel";

//! protoupnp macro to check for and get the upnp information
static const ProtoUpnpMacroT _ProtoUpnp_cmd_dscg[] =
{
    { 'disc', 0, 0, NULL },     // discovery
    { 'desc', 0, 0, NULL },     // get router description
    { 'gadr', 0, 0, NULL },     // GetExternalIPAddress
    { 0,      0, 0, NULL }
};

//! protoupnp macro to add a port mapping
static const ProtoUpnpMacroT _ProtoUpnp_cmd_addp[] =
{
    { 'gprt', 0, 0, NULL },     // GetSpecificPortMapping
    { 'aprt', 0, 0, NULL },     // AddPortMapping
    { 0,      0, 0, NULL }
};

//! protoupnp macro to discover, describe, and check port mapping
static const ProtoUpnpMacroT _ProtoUpnp_cmd_dscp[] =
{
    { 'disc', 0, 0, NULL },     // discovery
    { 'desc', 0, 0, NULL },     // get router description
    { 'gadr', 0, 0, NULL },     // GetExternalIPAddress
    { 'gprt', 0, 0, NULL },     // GetSpecificPortMapping
    { 0,      0, 0, NULL }
};

//! protoupnp macro to discover, describe, and add port map
static const ProtoUpnpMacroT _ProtoUpnp_cmd_upnp[] =
{
    { 'disc', 0, 0, NULL },     // discovery
    { 'desc', 0, 0, NULL },     // get router description
    { 'gadr', 0, 0, NULL },     // GetExternalIPAddress
    { 'gprt', 0, 0, NULL },     // GetSpecificPortMapping
    { 'aprt', 0, 0, NULL },     // AddPortMapping
    { 0,      0, 0, NULL }
};

//! protoupnp macro to fully test router
static const ProtoUpnpMacroT _ProtoUpnp_cmd_test[] =
{
    // initial test is basically the 'addp' macro plus service description
    { 'disc',  0, 0, NULL },
    { 'desc',  0, 0, NULL },
    { 'sdsc',  0, 0, NULL },
    { 'gadr',  0, 0, NULL },
    { 'gprt',  0, 0, NULL },
    { 'aprt',  0, 0, NULL },
    { 'dprt',  0, 0, NULL },

    // test Wildcard in Source IP optional feature
    { 'host',  0, 0, NULL },    // set host to use wildcard
    { 'aprt',  0, 0, NULL },
    { 'gprt',  0, 0, NULL },
    { 'dprt',  0, 0, NULL },
    { 'host', -1, 0, NULL },    // restore host

    // test Wildcard in External Port optional feature
    { 'extp',  0, 0, NULL },
    { 'aprt',  0, 0, NULL },
    { 'gprt',  0, 0, NULL },
    { 'dprt',  0, 0, NULL },

    // test Non-matching Internal and External Ports optional feature
    { 'extp',  3000, 0, NULL },
    { 'aprt',  0, 0, NULL },
    { 'gprt',  0, 0, NULL },
    { 'dprt',  0, 0, NULL },

    // done
    { 0,       0, 0, NULL }
};

//! map states to control request idents
static uint32_t _ProtoUpnp_aStateMap[] =
{
    'idle',     // ST_IDLE
    'disc',     // ST_DISCOVERY
    'desc',     // ST_DESCRIPTION
    'sdsc',     // ST_SERVICEDESC
    'gvar',     // ST_GETSTATEVAR
    'gadr',     // ST_GETEXTADDR
    'gprt',     // ST_GETPORTMAPPING
    'dprt',     // ST_DELPORTMAPPING
    'aprt',     // ST_ADDPORTMAPPING
};

//! upnp module ref
static ProtoUpnpRefT *_ProtoUpnp_pRef = NULL;

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ProtoUpnpIntToStr

    \Description
        Convert an integer to a string, and return a pointer to the string.

    \Input iVal     - value to convert to string

    \Output
        const char *- pointer to string

    \Version 03/30/2005 (jbrookes)
*/
/********************************************************************************F*/
static const char *_ProtoUpnpIntToStr(int32_t iVal)
{
    static char _strVal[16];
    ds_snzprintf(_strVal, sizeof(_strVal), "%d", iVal);
    return(_strVal);
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpFindUrl

    \Description
         Find the url following the http://host:port/ portion of a url.

    \Input *pUrl        - pointer to source url to parse

    \Output
        const char *    - pointer to url or NULL

    \Version 03/23/2005 (jbrookes)
*/
/********************************************************************************F*/
static const char *_ProtoUpnpFindUrl(const char *pUrl)
{
    // make sure it's http (or https)
    if ((pUrl = ds_stristr(pUrl, "http")) == NULL)
    {
        return(NULL);
    }

    // skip past double slash
    if ((pUrl = ds_stristr(pUrl, "//")) == NULL)
    {
        return(NULL);
    }
    pUrl += 2;

    // next slash is the start of the url
    return(strchr(pUrl, '/'));
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpMakeFullUrl

    \Description
         Create a full url for the given device.

    \Input *pDevice - pointer to device
    \Input *pBuffer - [out] storage for full url
    \Input iBufSize - size of output buffer
    \Input *pUrl    - pointer to input url (which may be relative or absolute)

    \Version 11/07/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoUpnpMakeFullUrl(ProtoUpnpDeviceT *pDevice, char *pBuffer, int iBufSize, const char *pUrl)
{
    // clear output url buffer
    ds_memclr(pBuffer, iBufSize);

    // prepend url base if we weren't given an absolute url
    if (ds_strnicmp(pUrl, "http", 4))
    {
        ds_strnzcpy(pBuffer, pDevice->strUrlBase, iBufSize);
        // if relative url does not start with a slash, add one here
        if (*pUrl != '/')
        {
            NetPrintf(("protoupnp: relative url does not start with a forward slash; adding one\n"));
            ds_strnzcat(pBuffer, "/", iBufSize);
        }
    }
    ds_strnzcat(pBuffer, pUrl, iBufSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpReset

    \Description
         Reset module state

    \Input *pProtoUpnp  - pointer to module state

    \Version 07/14/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoUpnpReset(ProtoUpnpRefT *pProtoUpnp)
{
    // init device structure
    ds_memclr(&pProtoUpnp->Device, sizeof(pProtoUpnp->Device));

    // force immediate discovery broadcast
    pProtoUpnp->iDiscoveryTimer = NetTick() - PROTOUPNP_DISCOVERYINTERVAL;

    // clear status
    pProtoUpnp->iStatus = 0;

    // reset service index
    pProtoUpnp->iService = 0;
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpError

    \Description
         Handle error condition

    \Input *pProtoUpnp  - pointer to module state
    \Input *pReason     - reason for error

    \Version 10/07/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoUpnpError(ProtoUpnpRefT *pProtoUpnp, const char *pReason)
{
    // abort current command list?
    //pProtoUpnp->pCommandList = NULL;

    // reset to idle state
    pProtoUpnp->eState = ST_IDLE;

    // output reason to debug output
    NetPrintf(("protoupnp: error -- %s\n", pReason));
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpSendDiscoveryRequest

    \Description
        Send a discovery request

    \Input *pProtoUpnp  - pointer to module state

    \Version 03/23/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoUpnpSendDiscoveryRequest(ProtoUpnpRefT *pProtoUpnp)
{
    /*! HTTP-formatted packet to send in UDP discovery broadcast
        [1] 1.2.2 Discovery: Search: Request with M-SEARCH */
    static const char _strDiscoveryPacket[] =
        "M-SEARCH * HTTP/1.1\r\n" \
        "Host:239.255.255.250:1900\r\n" \
        "ST:urn:schemas-upnp-org:device:WANConnectionDevice:1\r\n" \
        "Man:\"ssdp:discover\"\r\n" \
        "MX:3\r\n" \
        "\r\n";

    // send discovery multicast
    #if DIRTYCODE_LOGGING
    if (pProtoUpnp->bVerbose)
    {
        NetPrintf(("protoupnp: multicasting discovery request\n"));
    }
    #endif
    SocketSendto(pProtoUpnp->pUdp, _strDiscoveryPacket, sizeof(_strDiscoveryPacket), 0,
        &pProtoUpnp->DiscoveryAddr, sizeof(pProtoUpnp->DiscoveryAddr));
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpParseDiscoveryResponse

    \Description
        Parse a discovery response

    \Input *pProtoUpnp  - pointer to module state
    \Input *pResponse   - pointer to response

    \Output
        int32_t             - zero=failure, else success

    \Notes
        The discovery response is an HTTP response in a UDP packet.  Since
        ProtoHttp does not support UDP, we do the parsing inline here.

    \Version 03/23/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoUpnpParseDiscoveryResponse(ProtoUpnpRefT *pProtoUpnp, const char *pResponse)
{
    const char strLocation[] = "Location:";
    const char *pLocation, *pUrl;
    int32_t iStrLen;

    // echo response to debug output
    #if DIRTYCODE_LOGGING
    if (pProtoUpnp->bVerbose)
    {
        NetPrintf(("protoupnp: received discovery response:\n"));
        NetPrintWrap(pProtoUpnp->strRecvBuf, 80);
    }
    #endif

    // make sure we have a valid header
    if (strncmp(pResponse, "HTTP", 4))
    {
        NetPrintf(("protoupnp: ignoring non-http response\n"));
        return(0);
    }

    /*! check for substring that must be included in a valid discovery response
        [1] 1.2.3 Discovery: Search: Response */
    if (!ds_stristr(pResponse, "urn:schemas-upnp-org:device:wanconnectiondevice"))
    {
        NetPrintf(("protoupnp: ignoring non-discovery response\n"));
        return(0);
    }

    // find and skip location header
    if ((pLocation = ds_stristr(pResponse, strLocation)) == NULL)
    {
        NetPrintf(("protoupnp: response did not include a location header\n"));
        return(0);
    }
    pLocation += sizeof(strLocation) - 1;

    // skip to location body
    while((*pLocation != '\0') && (*pLocation <= ' '))
    {
        pLocation += 1;
    }

    // copy url, leaving room for null termination
    for (iStrLen = 0; iStrLen < (signed)(sizeof(pProtoUpnp->Device.strUrl) - 1); iStrLen++)
    {
        if ((*pLocation == '\0') || (*pLocation == '\r') || (*pLocation == '\n'))
        {
            break;
        }
        pProtoUpnp->Device.strUrl[iStrLen] = *pLocation++;
    }

    // null terminate url
    pProtoUpnp->Device.strUrl[iStrLen] = '\0';
    NetPrintf(("protoupnp: found upnp device '%s'\n", pProtoUpnp->Device.strUrl));

    // extract address/port
    if ((pUrl = _ProtoUpnpFindUrl(pProtoUpnp->Device.strUrl)) != NULL)
    {
        ds_strsubzcpy(pProtoUpnp->Device.strUrlBase, sizeof(pProtoUpnp->Device.strUrlBase), pProtoUpnp->Device.strUrl, (int32_t)(pUrl-pProtoUpnp->Device.strUrl));
        NetPrintf(("protoupnp: parsed base url '%s'\n", pProtoUpnp->Device.strUrlBase));
    }

    // mark device as valid and set status
    pProtoUpnp->Device.bDiscovered = TRUE;

    // reset to idle state
    pProtoUpnp->eState = ST_IDLE;
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpGetRemoteHost

    \Description
        Get string-formatted address for remote host used in UPnP service requests.

    \Input *pProtoUpnp  - pointer to module state
    \Input *pBuffer     - buffer
    \Input iBufSize     - buffer size

    \Version 11/07/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoUpnpGetRemoteHost(ProtoUpnpRefT *pProtoUpnp, char *pBuffer, int32_t iBufSize)
{
    uint32_t uRemoteHost;

    // get remote host address
    #if PROTOMANGLE_USEWANIPSRCADDR
    uRemoteHost = (pProtoUpnp->iRemoteHost == -1) ? pProtoUpnp->Device.uExternalAddress : (unsigned)pProtoUpnp->iRemoteHost;
    #else
    uRemoteHost = (pProtoUpnp->iRemoteHost == -1) ? 0 : (unsigned)pProtoUpnp->iRemoteHost;
    #endif

    // return formatted host string
    if (uRemoteHost != 0)
    {
        SocketInAddrGetText(uRemoteHost, pBuffer, iBufSize);
    }
    else
    {
        *pBuffer = '\0';
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpHttpReset

    \Description
        Reset HTTP-tracking variables prior to initiating an HTTP transaction.

    \Input *pProtoUpnp  - pointer to module state

    \Version 10/10/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoUpnpHttpReset(ProtoUpnpRefT *pProtoUpnp)
{
    pProtoUpnp->iHttpError = 0;
    pProtoUpnp->iSoapError = 0;
    pProtoUpnp->bRequestInProgress = TRUE;
    ds_memclr(pProtoUpnp->strRecvBuf, sizeof(pProtoUpnp->strRecvBuf));
    if (pProtoUpnp->pProtoHttp != NULL)
    {
        ProtoHttpControl(pProtoUpnp->pProtoHttp, 'keep', 0, 0, NULL);
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpHttpWaitResponse

    \Description
        Wait for an HTTP response.

    \Input *pProtoUpnp  - pointer to module state

    \Output
        int32_t             - negative=failure, zero=in progress, positve=success

    \Version 03/23/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoUpnpHttpWaitResponse(ProtoUpnpRefT *pProtoUpnp)
{
    int32_t iRecvResult;

    // if faking a response, data is already in buffer waiting for us
    #if DIRTYCODE_DEBUG
    if (pProtoUpnp->bFakeResponse)
    {
        pProtoUpnp->bFakeResponse = FALSE;
        pProtoUpnp->bRequestInProgress = FALSE;
        pProtoUpnp->eState = ST_IDLE;
        return(2);
    }
    #endif

    // give some time to ProtoHttp
    ProtoHttpUpdate(pProtoUpnp->pProtoHttp);

    // send any data that is still needed to be sent
    if (pProtoUpnp->iSentBytes < pProtoUpnp->iSendSize)
    {
        int32_t iResult;
        if ((iResult = ProtoHttpSend(pProtoUpnp->pProtoHttp, pProtoUpnp->strSendBuf+pProtoUpnp->iSentBytes, pProtoUpnp->iSendSize-pProtoUpnp->iSentBytes)) >= 0)
        {
            pProtoUpnp->iSentBytes += iResult;
        }
        else
        {
            NetPrintf(("protoupnp: %s request send failed for service %s with result %d\n", pProtoUpnp->strRequestName,
                pProtoUpnp->Device.Services[pProtoUpnp->iService].strServiceType, iResult));
            pProtoUpnp->iHttpError = iResult;
            pProtoUpnp->bRequestInProgress = FALSE;
            pProtoUpnp->eState = ST_IDLE;
            pProtoUpnp->iSentBytes = 0;
            return(-1);
        }
    }

    // read data
    if ((iRecvResult = ProtoHttpRecvAll(pProtoUpnp->pProtoHttp, pProtoUpnp->strRecvBuf, sizeof(pProtoUpnp->strRecvBuf))) >= 0)
    {
        // echo response to debug output
        #if DIRTYCODE_LOGGING
        if (pProtoUpnp->bVerbose)
        {
            NetPrintf(("\n"));
            NetPrintWrap(pProtoUpnp->strRecvBuf, 80);
        }
        #endif

        // reset transaction state
        pProtoUpnp->bRequestInProgress = FALSE;
        pProtoUpnp->eState = ST_IDLE;
        pProtoUpnp->iSentBytes = 0;

        // check for zero-length response (has been shown to happen in some Windows ICS error responses)
        if (iRecvResult == 0)
        {
            return(-1);
        }
        // success
        return(1);
    }
    else if ((iRecvResult < 0) && (iRecvResult != PROTOHTTP_RECVWAIT))
    {
        NetPrintf(("protoupnp: %s request receive failed for service %s with result %d\n", pProtoUpnp->strRequestName,
            pProtoUpnp->Device.Services[pProtoUpnp->iService].strServiceType, iRecvResult));
        pProtoUpnp->iHttpError = iRecvResult;
        pProtoUpnp->bRequestInProgress = FALSE;
        pProtoUpnp->eState = ST_IDLE;
        pProtoUpnp->iSentBytes = 0;
        return(-1);
    }

    // return pending result
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpSoapFormatRequestHeader

    \Description
        Format SOAP header for request given by pRequestName, using the currently
        active service.

    \Input *pProtoUpnp      - pointer to module state
    \Input *pRequestName    - pointer to request name

    \Version 03/27/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoUpnpSoapFormatRequestHeader(ProtoUpnpRefT *pProtoUpnp, const char *pRequestName)
{
    char strHdrExtra[256];

    // set up extended header fields
    ds_snzprintf(strHdrExtra, sizeof(strHdrExtra),
        "Content-Type: text/xml; charset=\"utf-8\"\r\n"
        "SOAPAction: \"%s#%s\"\r\n"
        "Cache-Control: no-cache\r\n",
        pProtoUpnp->Device.Services[pProtoUpnp->iService].strServiceType,
        pRequestName);

    // tell ProtoHttp to use this for the extended header info
    ProtoHttpControl(pProtoUpnp->pProtoHttp, 'apnd', 0, 0, strHdrExtra);
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpSoapRequestOpen

    \Description
        Format a SOAP request based on pRequestName and the given variable-argument
        list that constructs the SOAP request body, for the currently active
        service.

    \Input *pProtoUpnp      - pointer to module state
    \Input *pRequestName    - pointer to request name

    \Version 03/30/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoUpnpSoapRequestOpen(ProtoUpnpRefT *pProtoUpnp, const char *pRequestName)
{
    char *pBuf = pProtoUpnp->strSendBuf;
    int32_t iBufSize = sizeof(pProtoUpnp->strSendBuf);

    // set up the header
    _ProtoUpnpSoapFormatRequestHeader(pProtoUpnp, pRequestName);

    // set up soap envelope start
    pProtoUpnp->iSendSize = ds_snzprintf(pBuf, iBufSize,
        "<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">\r\n"
        " <s:Body>\r\n"
        "  <u:%s xmlns:u=\"%s\">\r\n",
        pRequestName,
        pProtoUpnp->Device.Services[pProtoUpnp->iService].strServiceType);

    // save request name
    ds_strnzcpy(pProtoUpnp->strRequestName, pRequestName, sizeof(pProtoUpnp->strRequestName));
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpSoapRequestAdd

    \Description
        Add a value to a soap request.  This function should not be used directly,
        the _ProtoUpnpSoapRequestAddStr, AddUI2, AddUI4, and AddBln macros should be
        used instead.

    \Input *pProtoUpnp      - pointer to module state
    \Input *pName           - pointer to item name
    \Input *pType           - pointer to item type
    \Input *pData           - pointer to item data


    \Version 03/30/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoUpnpSoapRequestAdd(ProtoUpnpRefT *pProtoUpnp, const char *pName, const char *pType, const char *pData)
{
    char *pBuf = pProtoUpnp->strSendBuf + pProtoUpnp->iSendSize;
    int32_t iBufSize = sizeof(pProtoUpnp->strSendBuf) - pProtoUpnp->iSendSize;

    #if PROTOUPNP_FULLVARDESC
    pProtoUpnp->iSendSize += ds_snzprintf(pBuf, iBufSize,
        "   <%s xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"%s\">%s</%s>\r\n",
        pName, pType, pData, pName);
    #else
    pProtoUpnp->iSendSize += ds_snzprintf(pBuf, iBufSize, "   <%s>%s</%s>\r\n", pName, pData, pName);
    #endif
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpSoapRequestClose

    \Description
        Close the soap request.

    \Input *pProtoUpnp      - pointer to module state

    \Version 03/30/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoUpnpSoapRequestClose(ProtoUpnpRefT *pProtoUpnp)
{
    char *pBuf = pProtoUpnp->strSendBuf + pProtoUpnp->iSendSize;
    int32_t iBufSize = sizeof(pProtoUpnp->strSendBuf) - pProtoUpnp->iSendSize;

    // append soap envelope end
    pProtoUpnp->iSendSize += ds_snzprintf(pBuf, iBufSize,
        "  </u:%s>\r\n"
        " </s:Body>\r\n"
        "</s:Envelope>\r\n\r\n",
        pProtoUpnp->strRequestName);
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpSoapRequestPost

    \Description
        Post a formatted SOAP request to gateway device.

    \Input *pProtoUpnp      - pointer to module state

    \Version 03/27/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoUpnpSoapRequestPost(ProtoUpnpRefT *pProtoUpnp)
{
    int32_t iResult;

    if (pProtoUpnp->bRequestInProgress == TRUE)
    {
        _ProtoUpnpError(pProtoUpnp, "a soap request is already in progress");
        return;
    }

    // print request to debug output
    NetPrintf(("protoupnp: initiating %s request for service %s\n", pProtoUpnp->strRequestName,
        pProtoUpnp->Device.Services[pProtoUpnp->iService].strServiceType));

    // initiate external address fetch operation
    _ProtoUpnpHttpReset(pProtoUpnp);
    if ((iResult = ProtoHttpPost(pProtoUpnp->pProtoHttp,
        pProtoUpnp->Device.Services[pProtoUpnp->iService].strControlUrl,
        pProtoUpnp->strSendBuf, pProtoUpnp->iSendSize,
        FALSE)) >= 0)
    {
        pProtoUpnp->iSentBytes = iResult;
    }
    else
    {
        NetPrintf(("protoupnp: %s request failed for service %s with result %d\n", pProtoUpnp->strRequestName,
            pProtoUpnp->Device.Services[pProtoUpnp->iService].strServiceType, iResult));
        pProtoUpnp->iHttpError = iResult;
        pProtoUpnp->bRequestInProgress = FALSE;
        pProtoUpnp->eState = ST_IDLE;
        return;
    }

    // verbose output?
    #if DIRTYCODE_LOGGING
    if (pProtoUpnp->bVerbose)
    {
        NetPrintWrap(pProtoUpnp->strSendBuf, 80);
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpSoapWaitResponse

    \Description
        Poll for a soap response.  If there is an error response, this function
        parses it and stores the value internally.

    \Input *pProtoUpnp      - pointer to module state

    \Version 03/30/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoUpnpSoapWaitResponse(ProtoUpnpRefT *pProtoUpnp)
{
    ProtoHttpResponseE eResponse;
    int32_t iResult;

    // wait until http transaction completes
    if ((iResult = _ProtoUpnpHttpWaitResponse(pProtoUpnp)) == 0)
    {
        return(iResult);
    }
    #if DIRTYCODE_DEBUG
    else if (iResult == 2)
    {
        return(iResult);
    }
    #endif

    // check for error response
    eResponse = ProtoHttpStatus(pProtoUpnp->pProtoHttp, 'code', NULL, 0);

    // if an error
    if (eResponse != PROTOHTTP_RESPONSE_SUCCESSFUL)
    {
        // if it is a soap error, parse the response
        if (eResponse == PROTOHTTP_RESPONSE_SERVERERROR)
        {
            const char *pXml, *pXml2;

            // find UPnPError
            if ((pXml = XmlFind(pProtoUpnp->strRecvBuf, "%*:Envelope.%*:Body.%*:Fault.detail.UPnPError")) != NULL)
            {
                // get error code
                if ((pXml2 = XmlFind(pXml, ".errorCode")) != NULL)
                {
                    pProtoUpnp->iSoapError = XmlContentGetInteger(pXml2, 0);
                }

                // get error description
                #if DIRTYCODE_LOGGING
                {
                    char strErrorText[96] = "unknown";
                    if ((pXml2 = XmlFind(pXml, ".errorDescription")) != NULL)
                    {
                        XmlContentGetString(pXml2, strErrorText, sizeof(strErrorText), "");
                    }
                    NetPrintf(("protoupnp:    soap error %d (%s) in response to %s request\n",
                        pProtoUpnp->iSoapError, strErrorText, pProtoUpnp->strRequestName));
                }
                #endif
            }
        }

        iResult = -1;
    }

    // return result to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpXmlGetAddress

    \Description
        Get an address from XML field pName.

    \Input *pXml        - pointer to current location in xml file
    \Input *pName       - name of field to get
    \Input *pAddress    - [out] output buffer to store address

    \Output
        int32_t         - negative=not found, else found

    \Version 10/10/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoUpnpXmlGetAddress(const char *pXml, const char *pName, int32_t *pAddress)
{
    if ((pXml = XmlFind(pXml, pName)) != NULL)
    {
        *pAddress = XmlContentGetAddress(pXml, 0);
        return(0);
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpXmlGetBoolean

    \Description
        Get a boolean from XML field pName.

    \Input *pXml    - pointer to current location in xml file
    \Input *pName   - name of field to get
    \Input *pBoolean - [out] output buffer to store boolean

    \Output
        int32_t     - negative=not found, else found

    \Version 10/10/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoUpnpXmlGetBoolean(const  char *pXml, const char *pName, uint8_t *pBoolean)
{
    if ((pXml = XmlFind(pXml, pName)) != NULL)
    {
        *pBoolean = (uint8_t)XmlContentGetInteger(pXml, 0);
        return(0);
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpXmlGetInteger

    \Description
        Get an integer from XML field pName.

    \Input *pXml    - pointer to current location in xml file
    \Input *pName   - name of field to get
    \Input *pInteger - [out] output buffer to store integer

    \Output
        int32_t         - negative=not found, else found

    \Version 10/10/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoUpnpXmlGetInteger(const char *pXml, const char *pName, int32_t *pInteger)
{
    if ((pXml = XmlFind(pXml, pName)) != NULL)
    {
        *pInteger = XmlContentGetInteger(pXml, 0);
        return(0);
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpXmlGetString

    \Description
        Get a string from XML field pName.

    \Input *pXml    - pointer to current location in xml file
    \Input *pName   - name of field to get
    \Input *pBuffer - [out] output buffer to store string
    \Input iBufSize - size of output buffer

    \Output
        int32_t         - negative=not found, else found

    \Version 10/10/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoUpnpXmlGetString(const char *pXml, const char *pName, char *pBuffer, int32_t iBufSize)
{
    ds_memclr(pBuffer, iBufSize);
    if ((pXml = XmlFind(pXml, pName)) != NULL)
    {
        return(XmlContentGetString(pXml, pBuffer, iBufSize, ""));
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpXmlParseDescription

    \Description
        Parse a device description response

    \Input *pProtoUpnp  - pointer to module state

    \Output
        int32_t             - negative=failure, else success

    \Version 03/23/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoUpnpXmlParseDescription(ProtoUpnpRefT *pProtoUpnp)
{
    char strContentA[256], strContentB[256], strContentC[256];
    ProtoUpnpDeviceT *pDevice = &pProtoUpnp->Device;
    const char *pXml, *pXml2;

    // parse optional URLBase field if present
    if ((pXml = XmlFind(pProtoUpnp->strRecvBuf, "root.URLBase")) != NULL)
    {
        // get urlbase string
        int32_t iUrlBaseLen = XmlContentGetString(pXml, pProtoUpnp->Device.strUrlBase, sizeof(pProtoUpnp->Device.strUrlBase), "");
        if (iUrlBaseLen > 0)
        {
            // if urlbase ends in a trailing slash, trim it
            if (pProtoUpnp->Device.strUrlBase[iUrlBaseLen-1] == '/')
            {
                NetPrintf(("protoupnp: trimming trailing slash from parsed URLBase string\n"));
                pProtoUpnp->Device.strUrlBase[iUrlBaseLen-1] = '\0';
            }
            NetPrintf(("protoupnp: parsed URLBase=%s\n", pProtoUpnp->Device.strUrlBase));
        }
        else
        {
            NetPrintf(("protoupnp: unable to parse URLBase field\n"));
        }
    }

    // find a WANConnectionDevice
    NetPrintf(("protoupnp: parsing xml response for a WANConnectionDevice:\n"));
    for (pXml = XmlFind(pProtoUpnp->strRecvBuf, "root.device"); pXml != NULL; )
    {
        // get deviceType
        pXml2 = XmlFind(pXml, ".deviceType");

        // get deviceType contents
        XmlContentGetString(pXml2, strContentA, sizeof(strContentA), "");
        NetPrintf(("protoupnp:    deviceType=%s\n", strContentA));

        // is this the deviceType we are looking for?
        if (ds_stristr(strContentA, "WANConnectionDevice"))
        {
            // get UDN
            _ProtoUpnpXmlGetString(pXml2, "UDN", pDevice->strUdn, sizeof(pDevice->strUdn));
            // get manufacturer name
            _ProtoUpnpXmlGetString(pXml2, "manufacturer", strContentA, sizeof(strContentA));
            // get model name
            _ProtoUpnpXmlGetString(pXml2, "modelName", strContentB, sizeof(strContentB));
            // get model number (firmware rev)
            _ProtoUpnpXmlGetString(pXml2, "modelNumber", strContentC, sizeof(strContentC));

            // format model name
            ds_snzprintf(pProtoUpnp->Device.strModel, sizeof(pProtoUpnp->Device.strModel), "%s %s %s",
                strContentA, strContentB, strContentC);
            NetPrintf(("protoupnp:    device=%s UDN=%s\n", pProtoUpnp->Device.strModel, pDevice->strUdn));
            break;
        }

        // first try to advance through recursion
        if ((pXml2 = XmlFind(pXml, ".deviceList.device")) == NULL)
        {
            // if no child device lists, skip to next element
            pXml2 = XmlSkip(pXml);
        }
        pXml = pXml2;
    }

    // find any services associated with this device
    NetPrintf(("protoupnp: parsing xml response for WANConnection Services:\n"));
    for (pXml = XmlFind(pXml, ".serviceList.service"); pXml != NULL; pXml = XmlNext(pXml))
    {
        // get serviceType header
        if (_ProtoUpnpXmlGetString(pXml, ".serviceType", strContentB, sizeof(strContentB)) >= 0)
        {
            // get serviceType contents
            NetPrintf(("protoupnp:    serviceType=%s\n", strContentB));

            // if it's not a serviceType we want, ignore it
            if (!ds_stristr(strContentB, "connection"))
            {
                continue;
            }

            // if we are out of space to store it, continue
            if (pDevice->iNumServices >= PROTOUPNP_MAXSERVICES)
            {
                NetPrintf(("protoupnp: service list full, ignoring this entry\n"));
                continue;
            }

            // if this a serviceType we want, find the controlUrl
            if (_ProtoUpnpXmlGetString(pXml, ".controlURL", strContentA, sizeof(strContentA)) >= 0)
            {
                ProtoUpnpServiceT *pService = &pDevice->Services[pDevice->iNumServices];

                // create controlURL
                _ProtoUpnpMakeFullUrl(&pProtoUpnp->Device, pService->strControlUrl, sizeof(pService->strControlUrl), strContentA);
                NetPrintf(("protoupnp:       controlURL=%s\n", pService->strControlUrl));

                // create serviceURL
                _ProtoUpnpXmlGetString(pXml, ".SCPDURL", strContentA, sizeof(strContentA));
                _ProtoUpnpMakeFullUrl(&pProtoUpnp->Device, pService->strDescriptionUrl, sizeof(pService->strDescriptionUrl), strContentA);
                NetPrintf(("protoupnp:       SCPDURL=%s\n", pService->strDescriptionUrl));

                // save serviceType
                ds_strnzcpy(pService->strServiceType, strContentB, sizeof(pService->strServiceType));
                NetPrintf(("protoupnp:       serviceType=%s\n", pService->strServiceType));

                pDevice->iNumServices += 1;
            }
        }
    }

    return((pDevice->iNumServices > 0) ? 0 : -1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpXmlParseGetExtAddr

    \Description
        Parse a service GetExternalIpAddress response

    \Input *pProtoUpnp  - pointer to module state

    \Output
        int32_t             - negative=failure, else success

    \Version 03/25/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoUpnpXmlParseGetExtAddr(ProtoUpnpRefT *pProtoUpnp)
{
    const char *pXml;

    // parse response and look for external address
    NetPrintf(("protoupnp: parsing xml response to GetExternalIPAddress request:\n"));
    if ((pXml = XmlFind(pProtoUpnp->strRecvBuf, "%*:Envelope.%*:Body.%*:GetExternalIPAddressResponse")) != NULL)
    {
        // first, try to get contents ($$note - try this with a Netgear as it is different from the Linksys)
        if ((pProtoUpnp->Device.uExternalAddress = XmlContentGetAddress(pXml, 0)) == 0)
        {
            // if that didn't work, try NewExternalIPAddress
            _ProtoUpnpXmlGetAddress(pXml, ".NewExternalIPAddress", (int32_t *)&pProtoUpnp->Device.uExternalAddress);
        }
        // display parsed address
        NetPrintf(("protoupnp:    IPAddress=%a\n", pProtoUpnp->Device.uExternalAddress));
    }

    return((pProtoUpnp->Device.uExternalAddress != 0) ? 0 : -1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpXmlParseGetPortMapping

    \Description
        Parse a service GetSpecificPortMappingEntry response

    \Input *pProtoUpnp  - pointer to module state

    \Version 10/07/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoUpnpXmlParseGetPortMapping(ProtoUpnpRefT *pProtoUpnp)
{
    const char *pXml;

    // parse response and look for external address
    NetPrintf(("protoupnp: parsing xml response to GetSpecificPortMappingEntry request:\n"));
    if ((pXml = XmlFind(pProtoUpnp->strRecvBuf, "%*:Envelope.%*:Body.%*:GetSpecificPortMappingEntryResponse")) != NULL)
    {
        // parse addr
        _ProtoUpnpXmlGetAddress(pXml, ".NewInternalClient", &pProtoUpnp->Device.CurPortMap.iAddr);
        // parse port
        _ProtoUpnpXmlGetInteger(pXml, ".NewInternalPort", &pProtoUpnp->Device.CurPortMap.iPort);
        // parse enabled
        _ProtoUpnpXmlGetBoolean(pXml, ".NewEnabled", &pProtoUpnp->Device.CurPortMap.bEnabled);
        // parse description
        _ProtoUpnpXmlGetString(pXml, ".NewPortMappingDescription", pProtoUpnp->Device.CurPortMap.strDesc,
            sizeof(pProtoUpnp->Device.CurPortMap.strDesc));

        // debug output
        NetPrintf(("protoupnp: found port mapping ->%a:%d enabled=%s (%s)\n",
           pProtoUpnp->Device.CurPortMap.iAddr, pProtoUpnp->Device.CurPortMap.iPort,
           pProtoUpnp->Device.CurPortMap.bEnabled ? "true" : "false",
           pProtoUpnp->Device.CurPortMap.strDesc));
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoUpnpIdle

    \Description
        NetConn idle function to update the ProtoUpnp module.

    \Input *pData   - pointer to module state
    \Input uTick    - current tick count

    \Notes
        This function is installed as a NetConn Idle function.  NetConnIdle()
        must be regularly polled for this function to be called.

    \Version 1.0 02/01/2008 (cadam) First Version
*/
/********************************************************************************F*/
static void _ProtoUpnpIdle(void *pData, uint32_t uTick)
{
    ProtoUpnpRefT *pRef = _ProtoUpnp_pRef;

    ProtoUpnpUpdate(pRef);
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoUpnpCreate

    \Description
        Create the ProtoUpnp module state.

    \Output
        ProtoUpnpRefT * - pointer to module state, or NULL

    \Version 03/23/2005 (jbrookes)
*/
/********************************************************************************F*/
ProtoUpnpRefT *ProtoUpnpCreate(void)
{
    ProtoUpnpRefT *pProtoUpnp;
    int32_t iMemGroup, iResult;
    void *pMemGroupUserData;
    struct sockaddr BindAddr;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // if already created, just ref it
    if (_ProtoUpnp_pRef != NULL)
    {
        _ProtoUpnp_pRef->iRefCount += 1;
        return(_ProtoUpnp_pRef);
    }

    // allocate and initialize module state
    if ((pProtoUpnp = DirtyMemAlloc(sizeof(*pProtoUpnp), PROTOUPNP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        return(NULL);
    }
    ds_memclr(pProtoUpnp, sizeof(*pProtoUpnp));
    pProtoUpnp->iMemGroup = iMemGroup;
    pProtoUpnp->pMemGroupUserData = pMemGroupUserData;

    // open udp socket
    if ((pProtoUpnp->pUdp = SocketOpen(AF_INET, SOCK_DGRAM, 0)) == NULL)
    {
        NetPrintf(("protoupnp: unable to open udp socket\n"));
        DirtyMemFree(pProtoUpnp, PROTOUPNP_MEMID, pProtoUpnp->iMemGroup, pProtoUpnp->pMemGroupUserData);
        return(NULL);
    }
    // bind udp socket
    SockaddrInit(&BindAddr, AF_INET);
    if ((iResult = SocketBind(pProtoUpnp->pUdp, &BindAddr, sizeof(BindAddr))) == SOCKERR_NONE)
    {
        #if DIRTYCODE_LOGGING
        SocketInfo(pProtoUpnp->pUdp, 'bind', 0, &BindAddr, sizeof(BindAddr));
        NetPrintf(("protoupnp: bound discovery socket to port %d\n", SockaddrInGetPort(&BindAddr)));
        #endif
    }
    else
    {
        NetPrintf(("protoupnp: error %d binding discovery socket\n", iResult));
        SocketClose(pProtoUpnp->pUdp);
        DirtyMemFree(pProtoUpnp, PROTOUPNP_MEMID, pProtoUpnp->iMemGroup, pProtoUpnp->pMemGroupUserData);
        return(NULL);
    }

    // allocate protohttp module
    if ((pProtoUpnp->pProtoHttp = ProtoHttpCreate(1024)) == NULL)
    {
        NetPrintf(("protoupnp: unable to create protohttp module\n"));
        SocketClose(pProtoUpnp->pUdp);
        DirtyMemFree(pProtoUpnp, PROTOUPNP_MEMID, pProtoUpnp->iMemGroup, pProtoUpnp->pMemGroupUserData);
        return(NULL);
    }

    // setup critical section
    NetCritInit(&pProtoUpnp->Crit, "protoupnp");

    // set to quiet mode
    ProtoHttpControl(pProtoUpnp->pProtoHttp, 'spam', FALSE, 0, NULL);

    // set up discovery sendto address
    SockaddrInit(&pProtoUpnp->DiscoveryAddr, AF_INET);
    SockaddrInSetAddr(&pProtoUpnp->DiscoveryAddr, PROTOUPNP_DISCOVERYADDR);
    SockaddrInSetPort(&pProtoUpnp->DiscoveryAddr, PROTOUPNP_DISCOVERYPORT);

    // set initial state
    pProtoUpnp->eState = ST_IDLE;
    pProtoUpnp->iLeaseDuration = 4*60*60;   // four hours
    pProtoUpnp->iPortToMapExt = pProtoUpnp->iPortToMapInt = PROTOUPNP_DEFAULTPORTMAP;
    pProtoUpnp->bEnableMapping = TRUE;
    pProtoUpnp->iRemoteHost = -1;

    // add protoupnp task handle
    NetConnIdleAdd(_ProtoUpnpIdle, pProtoUpnp);

    // save ref and return module pointer to caller
    pProtoUpnp->iRefCount = 1;
    _ProtoUpnp_pRef = pProtoUpnp;

    return(pProtoUpnp);
}

/*F********************************************************************************/
/*!
    \Function ProtoUpnpGetRef

    \Description
        Get ProtoUpnp reference

    \Version 03/23/2005 (jbrookes)
*/
/********************************************************************************F*/
ProtoUpnpRefT *ProtoUpnpGetRef(void)
{
    return(_ProtoUpnp_pRef);
}

/*F********************************************************************************/
/*!
    \Function ProtoUpnpDestroy

    \Description
        Destroy the ProtoUpnp module.

    \Input *pProtoUpnp  - pointer to module state

    \Version 03/23/2005 (jbrookes)
*/
/********************************************************************************F*/
void ProtoUpnpDestroy(ProtoUpnpRefT *pProtoUpnp)
{
    // if we aren't the last, just decrement the refcount and return
    if (--pProtoUpnp->iRefCount > 0)
    {
        return;
    }

    // destroy ProtoHttp module
    ProtoHttpDestroy(pProtoUpnp->pProtoHttp);

    // close udp socket
    SocketClose(pProtoUpnp->pUdp);

    // destroy critical section
    NetCritKill(&pProtoUpnp->Crit);

    // del protoupnp task handle
    NetConnIdleDel(_ProtoUpnpIdle, pProtoUpnp);

    // release module memory
    DirtyMemFree(pProtoUpnp, PROTOUPNP_MEMID, pProtoUpnp->iMemGroup, pProtoUpnp->pMemGroupUserData);

    // clear ref
    _ProtoUpnp_pRef = NULL;
}

/*F********************************************************************************/
/*!
    \Function ProtoUpnpStatus

    \Description
        Get information from the ProtoUpnp module.

    \Input *pProtoUpnp  - pointer to module state
    \Input iSelect      - info selector
    \Input *pBuf        - [out] pointer to output buffer
    \Input iBufSize     - size of output buffer

    \Output
        int32_t             - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'body' - get most recent http response body
            'ctrl' - return control ident for current operation
            'disc' - TRUE if a UPnP device has been discovered, else FALSE
            'dnam' - device name ("manufacturer modelName modelNumber")
            'done' - returns TRUE if in IDLE state and no macro is currently being processed
            'durn' - device URN
            'extn' - device external (WAN) IP address
            'extp' - returns external port used in mapping operations
            'idle' - return if module is idle or not
            'intp' - returns internal port used in mapping operations
            'lerr' - most recent error result (http or soap)
            'macr' - return current macro being executed, or zero if none, + name copied to pBuf
            'rbdy' - get most recent htto request body
            'stat' - module status (PROTOUPNP_STATUS_*)
        \endverbatim

    \Version 10/10/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoUpnpStatus(ProtoUpnpRefT *pProtoUpnp, int32_t iSelect, void *pBuf, int32_t iBufSize)
{
    // return most recent http body
    if (iSelect == 'body')
    {
        ds_strnzcpy(pBuf, pProtoUpnp->strRecvBuf, iBufSize);
        return(0);
    }
    // return control ident for current operation
    if (iSelect == 'ctrl')
    {
        return(_ProtoUpnp_aStateMap[pProtoUpnp->eState]);
    }
    // have we discovered a device?
    if (iSelect == 'disc')
    {
        return(pProtoUpnp->Device.bDiscovered);
    }
    // return device name
    if (iSelect == 'dnam')
    {
        ds_strnzcpy(pBuf, pProtoUpnp->Device.strModel, iBufSize);
        return(0);
    }
    if (iSelect == 'done')
    {
        return((pProtoUpnp->pCommandList == NULL) && (pProtoUpnp->eState == ST_IDLE));
    }
    // return device urn
    if (iSelect == 'durn')
    {
        ds_strnzcpy(pBuf, pProtoUpnp->Device.strUdn, iBufSize);
        return(0);
    }
    // return device external (WAN) IP address
    if (iSelect == 'extn')
    {
        return(pProtoUpnp->Device.uExternalAddress);
    }
    // return external port used in map operations
    if (iSelect == 'extp')
    {
        return(pProtoUpnp->iPortToMapExt);
    }
    // return if idle or not
    if (iSelect == 'idle')
    {
        return(pProtoUpnp->eState == ST_IDLE);
    }
    // set source port
    if (iSelect == 'intp')
    {
        return(pProtoUpnp->iPortToMapInt);
    }
    // last error
    if (iSelect == 'lerr')
    {
        if (pProtoUpnp->iSoapError != 0)
        {
            return(pProtoUpnp->iSoapError);
        }
        else if (pProtoUpnp->iHttpError != -1)
        {
            return(pProtoUpnp->iHttpError);
        }
        else
        {
            return(0);
        }
    }
    // return current macro being executed
    if (iSelect == 'macr')
    {
        int32_t iMacro = (pProtoUpnp->pCommandList != NULL) ? pProtoUpnp->pCommandList->iControl : 0;
        if (pBuf != NULL)
        {
            ds_strnzcpy(pBuf, pProtoUpnp->strRequestName, iBufSize);
        }
        return(iMacro);
    }
    // most recent request body
    if (iSelect == 'rbdy')
    {
        ds_strnzcpy(pBuf, pProtoUpnp->strSendBuf, iBufSize);
        return(0);
    }
    // return module status
    if (iSelect == 'stat')
    {
        return(pProtoUpnp->iStatus);
    }
    // try passing it to ProtoHttp
    return(ProtoHttpStatus(pProtoUpnp->pProtoHttp, iSelect, pBuf, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function ProtoUpnpControl

    \Description
        Control ProtoUpnp behavior

    \Input *pProtoUpnp  - pointer to module state
    \Input iControl     - control selector
    \Input iValue       - selector specifc
    \Input iValue2      - selector specific
    \Input *pValue      - selector specific

    \Output
        int32_t             - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'abrt' - abort current operation
            'aprt' - start AddPortMapping request
            'desc' - initiate description
            'disc' - initiate discovery
            'dprt' - start DeletePortMapping request
            'extp' - set external port
            'gadr' - start GetExternalIPAddress request
            'gprt' - start GetSpecificPortMappingEntry request
            'host' - host address to allow traffic through firewall from
            'intp' - set internal port
            'ldur' - set lease duration
            'macr' - execute a macro.  iValue=macro id, or pValue=pointer to user macro
            'addp' - macro to add a new port mapping
            'port' - set internal and external ports
            'spam' - enable/disable verbose output
        \endverbatim

    \Version 05/24/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoUpnpControl(ProtoUpnpRefT *pProtoUpnp, int32_t iControl, int32_t iValue, int32_t iValue2, const void *pValue)
{
    char strAddrText[20];

    // abort
    if (iControl == 'abrt')
    {
        NetCritEnter(&pProtoUpnp->Crit);
        pProtoUpnp->eState = ST_IDLE;
        pProtoUpnp->bRequestInProgress = FALSE;
        NetCritLeave(&pProtoUpnp->Crit);
        return(0);
    }
    // set dest port
    if (iControl == 'extp')
    {
        NetPrintf(("protoupnp: set external port to %d\n", iValue));
        pProtoUpnp->iPortToMapExt = iValue;
        return(0);
    }
    // fake a command
    #if DIRTYCODE_DEBUG
    if (iControl == 'fake')
    {
        const int32_t _StateMap[] = { 'idle', 'disc', 'desc', 'sdsc', 'gvar', 'gext', 'gprt', 'dprt', 'aprt' };
        int32_t iState;

        NetCritEnter(&pProtoUpnp->Crit);

        // copy input into receive buffer
        ds_strnzcpy(pProtoUpnp->strRecvBuf, pValue, sizeof(pProtoUpnp->strRecvBuf));

        // fake the command
        if (iValue == 'disc')
        {
            // parse the response & update status
            NetPrintf(("protoupnp: faking 'disc' command\n"));
            _ProtoUpnpParseDiscoveryResponse(pProtoUpnp, pProtoUpnp->strRecvBuf);
            pProtoUpnp->iStatus |= PROTOUPNP_STATUS_DISCOVERED;
        }
        else
        {
            for (iState = 0; iState < ST_LAST; iState++)
            {
                if (_StateMap[iState] == iValue)
                {
                    NetPrintf(("protoupnp: faking '%c%c%c%c' command\n",
                        (iValue>>24)&0xff, (iValue>>16)&0xff, (iValue>>8)&0xff,(iValue>>0)&0xff));
                    pProtoUpnp->bFakeResponse = TRUE;
                    pProtoUpnp->eState = iState;
                    break;
                }
            }
        }

        NetCritLeave(&pProtoUpnp->Crit);
        return(0);
    }
    #endif
    // set remote host
    if (iControl == 'host')
    {
        NetPrintf(("protoupnp: set remote host to %a\n", iValue));
        pProtoUpnp->iRemoteHost = iValue;
        return(0);
    }
    // set source port
    if (iControl == 'intp')
    {
        NetPrintf(("protoupnp: set internal port to %d\n", iValue));
        pProtoUpnp->iPortToMapInt = iValue;
        return(0);
    }
    // set lease duration
    if (iControl == 'ldur')
    {
        NetPrintf(("protoupnp: set lease duration to %d\n", iValue));
        pProtoUpnp->iLeaseDuration = iValue;
        return(0);
    }
    // set both internal and external ports
    if (iControl == 'port')
    {
        NetPrintf(("protoupnp: setting internal and external port to %d\n", iValue));
        pProtoUpnp->iPortToMapInt = pProtoUpnp->iPortToMapExt = iValue;
        return(0);
    }
    // set verbose
    if (iControl == 'spam')
    {
        ProtoHttpControl(pProtoUpnp->pProtoHttp, iControl, iValue, 0, NULL);
        pProtoUpnp->bVerbose = iValue;
        return(0);
    }

    /*
        The following control functions can only be initiated in IDLE state
    */

    if ((pProtoUpnp->eState != ST_IDLE) || (pProtoUpnp->bRequestInProgress))
    {
        NetPrintf(("protoupnp: '%c%c%c%c' only valid in IDLE state\n",
            (iControl>>24)&0xff, (iControl>>16)&0xff, (iControl>>8)&0xff, iControl&0xff));
        return(-1);
    }

    // execute a macro
    if (iControl == 'macr')
    {
        const ProtoUpnpMacroT *pMacro;

        // is it a built-in, set up command list to execute
        if (iValue == 'dscg')
        {
            pProtoUpnp->pCommandList = _ProtoUpnp_cmd_dscg;
        }
        else if (iValue == 'dscp')
        {
            pProtoUpnp->pCommandList = _ProtoUpnp_cmd_dscp;
        }
        else if (iValue == 'addp')
        {
            pProtoUpnp->pCommandList = _ProtoUpnp_cmd_addp;
        }
        else if (iValue == 'upnp')
        {
            pProtoUpnp->pCommandList = _ProtoUpnp_cmd_upnp;
        }
        else if (iValue == 'test')
        {
            pProtoUpnp->pCommandList = _ProtoUpnp_cmd_test;
        }
        else // user-specified macro
        {
            pProtoUpnp->pCommandList = pValue;
        }
        // start macro
        pMacro = pProtoUpnp->pCommandList;
        NetPrintf(("protoupnp: command list start -> %c%c%c%c\n",
            (pMacro->iControl>>24)&0xff, (pMacro->iControl>>16)&0xff,
            (pMacro->iControl>>8)&0xff, pMacro->iControl&0xff));
        ProtoUpnpControl(pProtoUpnp, pMacro->iControl, pMacro->iValue, pMacro->iValue2, pMacro->pValue);
        return(0);
    }

    // start discovery
    if (iControl == 'disc')
    {
        // reset module
        _ProtoUpnpReset(pProtoUpnp);

        NetPrintf(("protoupnp: searching for upnp devices\n"));

        // set to discovery state
        pProtoUpnp->eState = ST_DISCOVERY;
        return(0);
    }

    // only allow subsequent operations if we've discovered a device
    if (pProtoUpnp->Device.bDiscovered != TRUE)
    {
        NetPrintf(("protoupnp: '%c%c%c%c' only valid if a device has been discovered\n",
            (iControl>>24)&0xff, (iControl>>16)&0xff, (iControl>>8)&0xff, iControl&0xff));
        return(-1);
    }

    // start description
    if (iControl == 'desc')
    {
        // initiate description fetch operation
        NetPrintf(("protoupnp: sending description request\n"));
        ds_strnzcpy(pProtoUpnp->strRequestName, "Description", sizeof(pProtoUpnp->strRequestName));
        _ProtoUpnpHttpReset(pProtoUpnp);
        ProtoHttpGet(pProtoUpnp->pProtoHttp, pProtoUpnp->Device.strUrl, FALSE);

        // set to description state
        pProtoUpnp->eState = ST_DESCRIPTION;
        return(0);
    }

    // only allow subsequent operations if we've described a device
    if (pProtoUpnp->Device.iNumServices == 0)
    {
        NetPrintf(("protoupnp: '%c%c%c%c' only valid if a device has been described\n",
            (iControl>>24)&0xff, (iControl>>16)&0xff, (iControl>>8)&0xff, iControl&0xff));
        return(-1);
    }

    // start addportmapping request
    if (iControl == 'aprt')
    {
        // if we haven't already, acquire client address
        if (pProtoUpnp->iClientAddr == 0)
        {
            // get local address
            pProtoUpnp->iClientAddr = NetConnStatus('addr', 0, NULL, 0);
        }

        // if a mapping doesn't exist, go ahead and map it
        if (!pProtoUpnp->bPortIsMapped)
        {
            // get remote host address
            _ProtoUpnpGetRemoteHost(pProtoUpnp, strAddrText, sizeof(strAddrText));

            // post AddPortMapping request
            NetPrintf(("protoupnp: addportmap %s:%d->%a:%d duration=%d enabled=%s\n", strAddrText, pProtoUpnp->iPortToMapExt, pProtoUpnp->iClientAddr,
                pProtoUpnp->iPortToMapInt, pProtoUpnp->iLeaseDuration, pProtoUpnp->bEnableMapping ? "true" : "false"));

            // format the request
            _ProtoUpnpSoapRequestOpen(pProtoUpnp, "AddPortMapping");
            _ProtoUpnpSoapRequestAddStr(pProtoUpnp, "NewRemoteHost", strAddrText);
            _ProtoUpnpSoapRequestAddUI2(pProtoUpnp, "NewExternalPort", pProtoUpnp->iPortToMapExt);
            _ProtoUpnpSoapRequestAddStr(pProtoUpnp, "NewProtocol", "UDP");
            _ProtoUpnpSoapRequestAddUI2(pProtoUpnp, "NewInternalPort", pProtoUpnp->iPortToMapInt);
            _ProtoUpnpSoapRequestAddStr(pProtoUpnp, "NewInternalClient", SocketInAddrGetText(pProtoUpnp->iClientAddr, strAddrText, sizeof(strAddrText)));
            _ProtoUpnpSoapRequestAddBln(pProtoUpnp, "NewEnabled", pProtoUpnp->bEnableMapping);
            _ProtoUpnpSoapRequestAddStr(pProtoUpnp, "NewPortMappingDescription", _ProtoUpnp_strDescription);
            _ProtoUpnpSoapRequestAddUI4(pProtoUpnp, "NewLeaseDuration", pProtoUpnp->iLeaseDuration);
            _ProtoUpnpSoapRequestClose(pProtoUpnp);
            _ProtoUpnpSoapRequestPost(pProtoUpnp);

            // set to poll for response
            pProtoUpnp->eState = ST_ADDPORTMAPPING;
        }
        else
        {
            #if PROTOUPNP_REUSEMAPPING
            // do we already have the mapping we want?
            if ((pProtoUpnp->Device.CurPortMap.iAddr == pProtoUpnp->iClientAddr) &&
                (pProtoUpnp->Device.CurPortMap.iPort == pProtoUpnp->iPortToMapInt) &&
                (pProtoUpnp->Device.CurPortMap.bEnabled == pProtoUpnp->bEnableMapping) &&
                (!strcmp(pProtoUpnp->Device.CurPortMap.strDesc, _ProtoUpnp_strDescription)))
            {
                NetPrintf(("protoupnp: reusing already existing port mapping\n"));
            }
            else
            #endif
            {
                // we have to delete the current mapping before we can add a new one
                ProtoUpnpControl(pProtoUpnp, 'dprt', 0, 0, NULL);

                // if this is a command-list command, don't consume it
                if (pProtoUpnp->pCommandList != NULL)
                {
                    pProtoUpnp->pCommandList -= 1;
                }
            }
        }
        return(0);
    }
    // start delportmapping request
    if (iControl == 'dprt')
    {
        // get remote host address
        _ProtoUpnpGetRemoteHost(pProtoUpnp, strAddrText, sizeof(strAddrText));

        // post DeletePortMapping request
        _ProtoUpnpSoapRequestOpen(pProtoUpnp, "DeletePortMapping");
        _ProtoUpnpSoapRequestAddStr(pProtoUpnp, "NewRemoteHost", strAddrText);
        _ProtoUpnpSoapRequestAddUI2(pProtoUpnp, "NewExternalPort", pProtoUpnp->iPortToMapExt);
        _ProtoUpnpSoapRequestAddStr(pProtoUpnp, "NewProtocol", "UDP");
        _ProtoUpnpSoapRequestClose(pProtoUpnp);
        _ProtoUpnpSoapRequestPost(pProtoUpnp);

        // set to poll for response
        pProtoUpnp->eState = ST_DELPORTMAPPING;
        return(0);
    }
    // start getexternalipaddress request
    if (iControl == 'gadr')
    {
        // post GetExternalIPAddress request
        _ProtoUpnpSoapRequestOpen(pProtoUpnp, "GetExternalIPAddress");
        _ProtoUpnpSoapRequestClose(pProtoUpnp);
        _ProtoUpnpSoapRequestPost(pProtoUpnp);

        // set to getextaddr state
        pProtoUpnp->eState = ST_GETEXTADDR;
        return(0);
    }
    // start getgenericportmapping request
    if (iControl == 'ggpt')
    {
        // post GetSpecificPortMappingEntry request
        _ProtoUpnpSoapRequestOpen(pProtoUpnp, "GetGenericPortMappingEntry");
        _ProtoUpnpSoapRequestAddUI2(pProtoUpnp, "NewPortMappingIndex", iValue);
        _ProtoUpnpSoapRequestClose(pProtoUpnp);
        _ProtoUpnpSoapRequestPost(pProtoUpnp);

        // set to poll for response
        pProtoUpnp->eState = ST_GETPORTMAPPING;
        return(0);
    }
    // start getspecificportmapping request
    if (iControl == 'gprt')
    {
        // get remote host address
        _ProtoUpnpGetRemoteHost(pProtoUpnp, strAddrText, sizeof(strAddrText));

        // post GetSpecificPortMappingEntry request
        _ProtoUpnpSoapRequestOpen(pProtoUpnp, "GetSpecificPortMappingEntry");
        _ProtoUpnpSoapRequestAddStr(pProtoUpnp, "NewRemoteHost", strAddrText);
        _ProtoUpnpSoapRequestAddUI2(pProtoUpnp, "NewExternalPort", pProtoUpnp->iPortToMapExt);
        _ProtoUpnpSoapRequestAddStr(pProtoUpnp, "NewProtocol", "UDP");
        _ProtoUpnpSoapRequestClose(pProtoUpnp);
        _ProtoUpnpSoapRequestPost(pProtoUpnp);

        // set to poll for response
        pProtoUpnp->eState = ST_GETPORTMAPPING;
        return(0);
    }
    // start QueryStateVariable request
    if (iControl == 'gvar')
    {
        // post QueryStateVariable request
        _ProtoUpnpSoapRequestOpen(pProtoUpnp, "QueryStateVariable");
        _ProtoUpnpSoapRequestAddStr(pProtoUpnp, "varName", pValue);
        _ProtoUpnpSoapRequestClose(pProtoUpnp);
        _ProtoUpnpSoapRequestPost(pProtoUpnp);

        // set to poll for response
        pProtoUpnp->eState = ST_GETSTATEVAR;
        return(0);
    }
    // get service description
    if (iControl == 'sdsc')
    {
        // initiate description fetch operation
        NetPrintf(("protoupnp: sending service description request\n"));
        ds_strnzcpy(pProtoUpnp->strRequestName, "ServiceDesc", sizeof(pProtoUpnp->strRequestName));
        _ProtoUpnpHttpReset(pProtoUpnp);
        ProtoHttpGet(pProtoUpnp->pProtoHttp, pProtoUpnp->Device.Services[pProtoUpnp->iService].strDescriptionUrl, FALSE);

        // set to description state
        pProtoUpnp->eState = ST_SERVICEDESC;
        return(0);
    }
    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ProtoUpnpUpdate

    \Description
        Update the ProtoUpnp module.

    \Input *pProtoUpnp  - pointer to module state

    \Version 03/23/2005 (jbrookes)
*/
/********************************************************************************F*/
void ProtoUpnpUpdate(ProtoUpnpRefT *pProtoUpnp)
{
    int32_t iCurTick = NetTick();
    int32_t iResult;

    NetCritEnter(&pProtoUpnp->Crit);

    // update in idle state
    if (pProtoUpnp->eState == ST_IDLE)
    {
        // update command list
        if (pProtoUpnp->pCommandList != NULL)
        {
            // get next command
            const ProtoUpnpMacroT *pMacro = ++pProtoUpnp->pCommandList;

            // process command
            if (pMacro->iControl != 0)
            {
                NetPrintf(("protoupnp: command list -> %c%c%c%c\n",
                    (pMacro->iControl>>24)&0xff, (pMacro->iControl>>16)&0xff,
                    (pMacro->iControl>>8)&0xff, pMacro->iControl&0xff));
                ProtoUpnpControl(pProtoUpnp, pMacro->iControl, pMacro->iValue, pMacro->iValue2, pMacro->pValue);
            }
            else
            {
                NetPrintf(("protoupnp: command list complete\n"));
                pProtoUpnp->pCommandList = NULL;
            }
        }
    }

    // update during discovery process
    if (pProtoUpnp->eState == ST_DISCOVERY)
    {
        struct sockaddr RemoteAddr;
        int32_t iResponseSize, iAddrLen = sizeof(RemoteAddr);

        // send out a discovery broadcast?
        if (NetTickDiff(iCurTick, pProtoUpnp->iDiscoveryTimer) >= PROTOUPNP_DISCOVERYINTERVAL)
        {
            // send discovery request
            _ProtoUpnpSendDiscoveryRequest(pProtoUpnp);

            // set timer for next broadcast
            pProtoUpnp->iDiscoveryTimer = iCurTick;
        }

        // poll for discovery response
        if ((iResponseSize = SocketRecvfrom(pProtoUpnp->pUdp, pProtoUpnp->strRecvBuf, SOCKET_MAXUDPRECV, 0, &RemoteAddr, &iAddrLen)) > 0)
        {
            // add null-termination
            pProtoUpnp->strRecvBuf[iResponseSize] = '\0';

            // parse the response
            _ProtoUpnpParseDiscoveryResponse(pProtoUpnp, pProtoUpnp->strRecvBuf);

            // update status
            pProtoUpnp->iStatus |= PROTOUPNP_STATUS_DISCOVERED;
        }
    }

    // update during description process
    if (pProtoUpnp->eState == ST_DESCRIPTION)
    {
        // check for response
        if ((iResult = _ProtoUpnpHttpWaitResponse(pProtoUpnp)) > 0)
        {
            // parse the response
            if (_ProtoUpnpXmlParseDescription(pProtoUpnp) >= 0)
            {
                pProtoUpnp->iStatus |= PROTOUPNP_STATUS_DESCRIBED;
            }
            else
            {
                _ProtoUpnpError(pProtoUpnp, "parsing description");
            }
        }
        else if (iResult < 0)
        {
            _ProtoUpnpError(pProtoUpnp, "getting description");
        }
    }

    // update during service description process
    if (pProtoUpnp->eState == ST_SERVICEDESC)
    {
        // check for response
        if ((iResult = _ProtoUpnpHttpWaitResponse(pProtoUpnp)) > 0)
        {
            // $$ todo - handle service description response
        }
        else if (iResult < 0)
        {
            _ProtoUpnpError(pProtoUpnp, "getting service description");
        }
    }

    // update during QueryStateVariable request
    if (pProtoUpnp->eState == ST_GETSTATEVAR)
    {
        // check for response
        if ((iResult = _ProtoUpnpHttpWaitResponse(pProtoUpnp)) > 0)
        {
            // $$ todo - handle querystatevariable response
        }
        else if (iResult < 0)
        {
            _ProtoUpnpError(pProtoUpnp, "querying state variable");
        }
    }

    // update during GetExternalIPAddress request
    if (pProtoUpnp->eState == ST_GETEXTADDR)
    {
        // check for response
        if ((iResult = _ProtoUpnpSoapWaitResponse(pProtoUpnp)) > 0)
        {
            // parse the response
            if (_ProtoUpnpXmlParseGetExtAddr(pProtoUpnp) >= 0)
            {
                pProtoUpnp->iStatus |= PROTOUPNP_STATUS_GOTEXTADDR;
            }
            else
            {
                // didn't find it, so check the next service
                if (pProtoUpnp->iService < (pProtoUpnp->Device.iNumServices-1))
                {
                    pProtoUpnp->iService += 1;
                    ProtoUpnpControl(pProtoUpnp, 'gadr', 0, 0, NULL);
                }
            }
        }
        else if (iResult < 0)
        {
            _ProtoUpnpError(pProtoUpnp, "getting external address");
        }
    }

    // update during GetSpecificPortMappingEntry request
    if (pProtoUpnp->eState == ST_GETPORTMAPPING)
    {
        // check for response
        if ((iResult = _ProtoUpnpSoapWaitResponse(pProtoUpnp)) != 0)
        {
            // if we got a success response, that means a mapping already exists
            if (iResult > 0)
            {
                NetPrintf(("protoupnp:    mapping already exists\n"));
                _ProtoUpnpXmlParseGetPortMapping(pProtoUpnp);
                pProtoUpnp->iStatus |= PROTOUPNP_STATUS_FNDPORTMAP;
                pProtoUpnp->bPortIsMapped = TRUE;
            }
            else if (pProtoUpnp->iSoapError == 714)
            {
                NetPrintf(("protoupnp:    mapping does not exist\n"));
                pProtoUpnp->bPortIsMapped = FALSE;
            }
            else if (pProtoUpnp->iSoapError == 501)
            {
                // not supported? try continuing on anyway (BEFSR41)
                NetPrintf(("protoupnp:    unable to query mapping\n"));
                pProtoUpnp->bPortIsMapped = FALSE;
            }
            else // some other error, so bail
            {
                _ProtoUpnpError(pProtoUpnp, "getting portmapping");
            }
        }
    }

    // update during DeletePortMapping request
    if (pProtoUpnp->eState == ST_DELPORTMAPPING)
    {
        // check for response
        if ((iResult = _ProtoUpnpSoapWaitResponse(pProtoUpnp)) != 0)
        {
            // if we got a success response, that means we've successfully deleted the mapping
            if (iResult > 0)
            {
                NetPrintf(("protoupnp:    deleted port mapping\n"));
                pProtoUpnp->iStatus |= PROTOUPNP_STATUS_DELPORTMAP;
                pProtoUpnp->bPortIsMapped = FALSE;
            }
            else if (pProtoUpnp->iSoapError == 714)
            {
                NetPrintf(("protoupnp:    mapping does not exist\n"));
                pProtoUpnp->bPortIsMapped = FALSE;
            }
            else // some other error, so bail
            {
                _ProtoUpnpError(pProtoUpnp, "deleting port mapping");
            }
        }
    }

    // update during AddPortMapping request
    if (pProtoUpnp->eState == ST_ADDPORTMAPPING)
    {
        // check for response
        if ((iResult = _ProtoUpnpSoapWaitResponse(pProtoUpnp)) != 0)
        {
            // if we got a success response, that means the mapping succeeded
            if (iResult > 0)
            {
                pProtoUpnp->iStatus |= PROTOUPNP_STATUS_ADDPORTMAP;
                NetPrintf(("protoupnp:    port mapping added\n"));
            }
            else
            {
                uint8_t bRetry = TRUE;
                if (pProtoUpnp->iSoapError == 716)
                {
                    NetPrintf(("protoupnp:    wildcard not permitted in ext port\n"));
                    bRetry = FALSE;
                }
                else if (pProtoUpnp->iSoapError == 725)
                {
                    NetPrintf(("protoupnp:    finite lease duration not supported -- trying again with infinite\n"));
                    pProtoUpnp->iLeaseDuration = 0;
                }
                else if (pProtoUpnp->iSoapError == 726)
                {
                    NetPrintf(("protoupnp:    specific remote host not supported -- trying again with wildcard\n"));
                    pProtoUpnp->iRemoteHost = 0;
                }
                else if (pProtoUpnp->iLeaseDuration != 0)
                {
                    NetPrintf(("protoupnp:    unknown error adding port mapping -- trying again with infinite lease duration\n"));
                    pProtoUpnp->iLeaseDuration = 0;
                }
                else if (pProtoUpnp->iRemoteHost != 0)
                {
                    NetPrintf(("protoupnp:    unknown error adding port mapping -- trying again with wildcard source IP\n"));
                    pProtoUpnp->iRemoteHost = 0;
                }
                else
                {
                    // nothing else to try, so just fail
                    bRetry = FALSE;
                }

                // retry or fail?
                if (bRetry)
                {
                    ProtoUpnpControl(pProtoUpnp, 'aprt', 0, 0, NULL);
                }
                else
                {
                    _ProtoUpnpError(pProtoUpnp, "adding port mapping");
                }
            }
        }
    }

    NetCritLeave(&pProtoUpnp->Crit);
}
