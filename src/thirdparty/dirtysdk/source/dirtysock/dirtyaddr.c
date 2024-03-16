/*H********************************************************************************/
/*!
    \File dirtyaddr.c

    \Description
        Opaque address functions.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 04/09/2004 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyaddr.h"
#include "DirtySDK/dirtysock/netconn.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function DirtyAddrToHostAddr

    \Description
        Convert from DirtyAddrT to native format.

    \Input *pOutput     - [out] storage for native format address
    \Input iBufLen      - length of output buffer
    \Input *pAddr       - source address to convert

    \Output
        uint32_t    - number of characters consumed from input, or zero on failure

    \Notes
        On common platforms, a DirtyAddrT is a string-encoded network-order address
        field.  This function converts that input to a binary-encoded host-order
        address.

    \Version 04/09/2004 (jbrookes)
*/
/********************************************************************************F*/
uint32_t DirtyAddrToHostAddr(void *pOutput, int32_t iBufLen, const DirtyAddrT *pAddr)
{
    uint32_t uAddress;
    
    // make sure our buffer is big enough
    if (iBufLen < (signed)sizeof(uAddress))
    {
        NetPrintf(("dirtyaddr: output buffer too small\n"));
        return(0);
    }
    
    // convert from string to int32_t
    uAddress = (uint32_t)strtoul(&pAddr->strMachineAddr[1], NULL, 16);

    // convert from network order to host order
    uAddress = SocketNtohl(uAddress);

    // copy to output
    ds_memcpy(pOutput, &uAddress, sizeof(uAddress));
    return(9);
}

/*F********************************************************************************/
/*!
    \Function DirtyAddrFromHostAddr

    \Description
        Convert from native format to DirtyAddrT.

    \Input *pAddr       - [out] storage for output DirtyAddrT
    \Input *pInput      - pointer to native format address

    \Output
        uint32_t    - TRUE if successful, else FALSE

    \Notes
        On common platforms, a DirtyAddrT is a string-encoded network-order address
        field.  This function converts a binary-encoded host-order address to that
        format.

    \Version 04/09/2004 (jbrookes)
*/
/********************************************************************************F*/
uint32_t DirtyAddrFromHostAddr(DirtyAddrT *pAddr, const void *pInput)
{
    uint32_t uAddress;

    // make sure output buffer is okay
    if ((pInput == NULL) || (((intptr_t)pInput & 0x3) != 0))
    {
        return(FALSE);
    }

    uAddress = SocketHtonl(*(const uint32_t *)pInput);
    ds_snzprintf(pAddr->strMachineAddr, sizeof(pAddr->strMachineAddr), "$%08x", uAddress);
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function DirtyAddrGetLocalAddr

    \Description
        Get the local address in DirtyAddr form.

    \Input *pAddr       - [out] storage for output DirtyAddrT

    \Output
        uint32_t    - TRUE if successful, else FALSE

    \Version 09/29/2004 (jbrookes)
*/
/********************************************************************************F*/
uint32_t DirtyAddrGetLocalAddr(DirtyAddrT *pAddr)
{
    uint32_t uLocalAddr = NetConnStatus('addr', 0, NULL, 0);
    DirtyAddrFromHostAddr(pAddr, &uLocalAddr);
    return(TRUE);
}
