/*H********************************************************************************/
/*!
    \File dirtyaddr.h

    \Description
        Definition for portable address type.

    \Copyright
        Copyright (c) Electronic Arts 2004

    \Version 1.0 04/07/2004 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _dirtyaddr_h
#define _dirtyaddr_h

/*!
\Moduledef DirtyAddr DirtyAddr
\Modulemember DirtySock
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

#if defined(DIRTYCODE_XBOXONE) && !defined(DIRTYCODE_GDK)
/* In production, when an xboxone is located behind an OpenWRT based router that has
   an IPv6 connection, the router defaults to assigning ULA prefixes via SLAAC and
   DHCPv6. This results in the console having a global IPv6 address, a link local
   IPv6 address, 2 ULA IPv6 addresses, and an IPv4 address. In such a scenario, 
   the Secure Device Address of the console is large enough that it cannot fit in a
   127-byte DirtyAddrT (size used on other platforms).

   After checking with MS, we got a confirmation that the size of a
   SecureDevicAddress will never exceed 300 bytes. (enforced both in the
   Windows::Networking::XboxLive and the Windows::Xbox::Networking namespaces).

   A call to DirtyAddrSetInfoXboxOne() for a 300-byte SecureDeviceAddress blob
   results in 370 bytes being written in the DirtyAddrT. Consequently, it is safe
   to make the size DIRTYADDR_MACHINEADDR_MAXLEN 372 on xboxone.
*/
#define DIRTYADDR_MACHINEADDR_MAXLEN    (372)
#else
#define DIRTYADDR_MACHINEADDR_MAXLEN    (127)
#endif
#define DIRTYADDR_MACHINEADDR_MAXSIZE   (DIRTYADDR_MACHINEADDR_MAXLEN + 1)

/*** Macros ***********************************************************************/

//! compare two opaque addresses for equality  (same=TRUE, different=FALSE)
#define DirtyAddrCompare(_pAddr1, _pAddr2)  (!strcmp((_pAddr1)->strMachineAddr, (_pAddr2)->strMachineAddr))

/*** Type Definitions *************************************************************/

//! opaque address type
typedef struct DirtyAddrT
{
    char strMachineAddr[DIRTYADDR_MACHINEADDR_MAXSIZE];
} DirtyAddrT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

//! convert a DirtyAddrT to native format
DIRTYCODE_API uint32_t DirtyAddrToHostAddr(void *pOutput, int32_t iBufLen, const DirtyAddrT *pAddr);

//! convert a host-format address to native format
DIRTYCODE_API uint32_t DirtyAddrFromHostAddr(DirtyAddrT *pAddr, const void *pInput);

//! get local address in DirtyAddr form
DIRTYCODE_API uint32_t DirtyAddrGetLocalAddr(DirtyAddrT *pAddr);

#if defined(DIRTYCODE_XBOXONE) && !defined(DIRTYCODE_GDK)
//! get Xbox One extended info into dirtyaddr
DIRTYCODE_API uint8_t DirtyAddrGetInfoXboxOne(const DirtyAddrT *pDirtyAddr, void *pXuid, void *pSecureDeviceAddressBlob, int32_t *pBlobSize);

//! set Xbox One extended info into dirtyaddr
DIRTYCODE_API void DirtyAddrSetInfoXboxOne(DirtyAddrT *pDirtyAddr, const void *pXuid, const void *pSecureDeviceAddressBlob, int32_t iBlobSize);

#endif

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtyaddr_h

