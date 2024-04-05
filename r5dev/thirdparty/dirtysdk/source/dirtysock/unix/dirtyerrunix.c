/*H********************************************************************************/
/*!
    \File dirtyerrunix.c

    \Description
        Dirtysock debug error routines.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 10/04/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <errno.h> // included so we know where to find network headers
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

#if defined(DIRTYCODE_APPLEIOS) || defined(DIRTYCODE_APPLEOSX)
#include <Security/SecBase.h>
#include <Security/SecureTransport.h>
#endif

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

#if DIRTYSOCK_ERRORNAMES
static DirtyErrT _DirtyErr_List[] =
{
    DIRTYSOCK_ErrorName(EPERM),             // 1   Operation not permitted
    DIRTYSOCK_ErrorName(ENOENT),            // 2   No such file or directory
    DIRTYSOCK_ErrorName(ESRCH),             // 3   No such process
    DIRTYSOCK_ErrorName(EINTR),             // 4   Interrupted system call
    DIRTYSOCK_ErrorName(EIO),               // 5   I/O error
    DIRTYSOCK_ErrorName(ENXIO),             // 6   No such device or address
    DIRTYSOCK_ErrorName(E2BIG),             // 7   Arg list too long
    DIRTYSOCK_ErrorName(ENOEXEC),           // 8   Exec format error
    DIRTYSOCK_ErrorName(EBADF),             // 9   Bad file number
    DIRTYSOCK_ErrorName(ECHILD),            // 10  No child processes
    //DIRTYSOCK_ErrorName(EAGAIN),          // 11  Try again
    DIRTYSOCK_ErrorName(EWOULDBLOCK),       // 11  Operation would block
    DIRTYSOCK_ErrorName(ENOMEM),            // 12  Out of memory
    DIRTYSOCK_ErrorName(EACCES),            // 13  Permission denied
    DIRTYSOCK_ErrorName(EFAULT),            // 14  Bad address
    DIRTYSOCK_ErrorName(ENOTBLK),           // 15  Block device required
    DIRTYSOCK_ErrorName(EBUSY),             // 16  Device or resource busy
    DIRTYSOCK_ErrorName(EEXIST),            // 17  File exists
    DIRTYSOCK_ErrorName(EXDEV),             // 18  Cross-device link
    DIRTYSOCK_ErrorName(ENODEV),            // 19  No such device
    DIRTYSOCK_ErrorName(ENOTDIR),           // 20  Not a directory
    DIRTYSOCK_ErrorName(EISDIR),            // 21  Is a directory
    DIRTYSOCK_ErrorName(EINVAL),            // 22  Invalid argument
    DIRTYSOCK_ErrorName(ENFILE),            // 23  File table overflow
    DIRTYSOCK_ErrorName(EMFILE),            // 24  Too many open files
    DIRTYSOCK_ErrorName(ENOTTY),            // 25  Not a typewriter
    DIRTYSOCK_ErrorName(ETXTBSY),           // 26  Text file busy
    DIRTYSOCK_ErrorName(EFBIG),             // 27  File too large
    DIRTYSOCK_ErrorName(ENOSPC),            // 28  No space left on device
    DIRTYSOCK_ErrorName(ESPIPE),            // 29  Illegal seek
    DIRTYSOCK_ErrorName(EROFS),             // 30  Read-only file system
    DIRTYSOCK_ErrorName(EMLINK),            // 31  Too many links
    DIRTYSOCK_ErrorName(EPIPE),             // 32  Broken pipe
    DIRTYSOCK_ErrorName(EDOM),              // 33  Math argument out of domain of func
    DIRTYSOCK_ErrorName(ERANGE),            // 34  Math result not representable
    DIRTYSOCK_ErrorName(EDEADLK),           // 35  Resource deadlock would occur
    DIRTYSOCK_ErrorName(ENAMETOOLONG),      // 36  File name too long
    DIRTYSOCK_ErrorName(ENOLCK),            // 37  No record locks available
    DIRTYSOCK_ErrorName(ENOSYS),            // 38  Function not implemented
    DIRTYSOCK_ErrorName(ENOTEMPTY),         // 39  Directory not empty
    DIRTYSOCK_ErrorName(ELOOP),             // 40  Too many symbolic links encountered
    DIRTYSOCK_ErrorName(ENOMSG),            // 42  No message of desired type
    DIRTYSOCK_ErrorName(EIDRM),             // 43  Identifier removed
#if defined(DIRTYCODE_LINUX)
    DIRTYSOCK_ErrorName(ECHRNG),            // 44  Channel number out of range
    DIRTYSOCK_ErrorName(EL2NSYNC),          // 45  Level 2 not synchronized
    DIRTYSOCK_ErrorName(EL3HLT),            // 46  Level 3 halted
    DIRTYSOCK_ErrorName(EL3RST),            // 47  Level 3 reset
    DIRTYSOCK_ErrorName(ELNRNG),            // 48  Link number out of range
    DIRTYSOCK_ErrorName(EUNATCH),           // 49  Protocol driver not attached
    DIRTYSOCK_ErrorName(ENOCSI),            // 50  No CSI structure available
    DIRTYSOCK_ErrorName(EL2HLT),            // 51  Level 2 halted
    DIRTYSOCK_ErrorName(EBADE),             // 52  Invalid exchange
    DIRTYSOCK_ErrorName(EBADR),             // 53  Invalid request descriptor
    DIRTYSOCK_ErrorName(EXFULL),            // 54  Exchange full
    DIRTYSOCK_ErrorName(ENOANO),            // 55  No anode
    DIRTYSOCK_ErrorName(EBADRQC),           // 56  Invalid request code
    DIRTYSOCK_ErrorName(EBADSLT),           // 57  Invalid slot
    DIRTYSOCK_ErrorName(EDEADLOCK),         // 58  File locking deadlock error
    DIRTYSOCK_ErrorName(EBFONT),            // 59  Bad font file format
#endif
    DIRTYSOCK_ErrorName(ENOSTR),            // 60  Device not a stream
    DIRTYSOCK_ErrorName(ENODATA),           // 61  No data available
    DIRTYSOCK_ErrorName(ETIME),             // 62  Timer expired
    DIRTYSOCK_ErrorName(ENOSR),             // 63  Out of streams resources
#if defined(DIRTYCODE_LINUX)
    DIRTYSOCK_ErrorName(ENONET),            // 64  Machine is not on the network
    DIRTYSOCK_ErrorName(ENOPKG),            // 65  Package not installed
#endif
    DIRTYSOCK_ErrorName(EREMOTE),           // 66  Object is remote
    DIRTYSOCK_ErrorName(ENOLINK),           // 67  Link has been severed
#if defined(DIRTYCODE_LINUX)
    DIRTYSOCK_ErrorName(EADV),              // 68  Advertise error
    DIRTYSOCK_ErrorName(ESRMNT),            // 69  Srmount error
    DIRTYSOCK_ErrorName(ECOMM),             // 70  Communication error on send
#endif
    DIRTYSOCK_ErrorName(EPROTO),            // 71  Protocol error
    DIRTYSOCK_ErrorName(EMULTIHOP),         // 72  Multihop attempted
#if defined(DIRTYCODE_LINUX)
    DIRTYSOCK_ErrorName(EDOTDOT),           // 73  RFS specific error
#endif
    DIRTYSOCK_ErrorName(EBADMSG),           // 74  Not a data message
    DIRTYSOCK_ErrorName(EOVERFLOW),         // 75  Value too large for defined data type
#if defined(DIRTYCODE_LINUX)
    DIRTYSOCK_ErrorName(ENOTUNIQ),          // 76  Name not unique on network
    DIRTYSOCK_ErrorName(EBADFD),            // 77  File descriptor in bad state
    DIRTYSOCK_ErrorName(EREMCHG),           // 78  Remote address changed
    DIRTYSOCK_ErrorName(ELIBACC),           // 79  Can not access a needed shared library
    DIRTYSOCK_ErrorName(ELIBBAD),           // 80  Accessing a corrupted shared library
    DIRTYSOCK_ErrorName(ELIBSCN),           // 81  .lib section in a.out corrupted
    DIRTYSOCK_ErrorName(ELIBMAX),           // 82  Attempting to link in too many shared libraries
    DIRTYSOCK_ErrorName(ELIBEXEC),          // 83  Cannot exec a shared library directly
#endif
    DIRTYSOCK_ErrorName(EILSEQ),            // 84  Illegal byte sequence
#if defined(DIRTYCODE_LINUX)
    DIRTYSOCK_ErrorName(ERESTART),          // 85  Interrupted system call should be restarted
    DIRTYSOCK_ErrorName(ESTRPIPE),          // 86  Streams pipe error
#endif
    DIRTYSOCK_ErrorName(EUSERS),            // 87  Too many users
    DIRTYSOCK_ErrorName(ENOTSOCK),          // 88  Socket operation on non-socket
    DIRTYSOCK_ErrorName(EDESTADDRREQ),      // 89  Destination address required
    DIRTYSOCK_ErrorName(EMSGSIZE),          // 90  Message too long
    DIRTYSOCK_ErrorName(EPROTOTYPE),        // 91  Protocol wrong type for socket
    DIRTYSOCK_ErrorName(ENOPROTOOPT),       // 92  Protocol not available
    DIRTYSOCK_ErrorName(EPROTONOSUPPORT),   // 93  Protocol not supported
    DIRTYSOCK_ErrorName(ESOCKTNOSUPPORT),   // 94  Socket type not supported
    DIRTYSOCK_ErrorName(EOPNOTSUPP),        // 95  Operation not supported on transport endpoint
    DIRTYSOCK_ErrorName(EPFNOSUPPORT),      // 96  Protocol family not supported
    DIRTYSOCK_ErrorName(EAFNOSUPPORT),      // 97  Address family not supported by protocol
    DIRTYSOCK_ErrorName(EADDRINUSE),        // 98  Address already in use
    DIRTYSOCK_ErrorName(EADDRNOTAVAIL),     // 99  Cannot assign requested address
    DIRTYSOCK_ErrorName(ENETDOWN),          // 100 Network is down
    DIRTYSOCK_ErrorName(ENETUNREACH),       // 101 Network is unreachable
    DIRTYSOCK_ErrorName(ENETRESET),         // 102 Network dropped connection because of reset
    DIRTYSOCK_ErrorName(ECONNABORTED),      // 103 Software caused connection abort
    DIRTYSOCK_ErrorName(ECONNRESET),        // 104 Connection reset by peer
    DIRTYSOCK_ErrorName(ENOBUFS),           // 105 No buffer space available
    DIRTYSOCK_ErrorName(EISCONN),           // 106 Transport endpoint is already connected
    DIRTYSOCK_ErrorName(ENOTCONN),          // 107 Transport endpoint is not connected
    DIRTYSOCK_ErrorName(ESHUTDOWN),         // 108 Cannot send after transport endpoint shutdown
    DIRTYSOCK_ErrorName(ETOOMANYREFS),      // 109 Too many references: cannot splice
    DIRTYSOCK_ErrorName(ETIMEDOUT),         // 110 Connection timed out
    DIRTYSOCK_ErrorName(ECONNREFUSED),      // 111 Connection refused
    DIRTYSOCK_ErrorName(EHOSTDOWN),         // 112 Host is down
    DIRTYSOCK_ErrorName(EHOSTUNREACH),      // 113 No route to host
    DIRTYSOCK_ErrorName(EALREADY),          // 114 Operation already in progress
    DIRTYSOCK_ErrorName(EINPROGRESS),       // 115 Operation now in progress
    DIRTYSOCK_ErrorName(ESTALE),            // 116 Stale NFS file handle
#if defined(DIRTYCODE_LINUX)
    DIRTYSOCK_ErrorName(EUCLEAN),           // 117 Structure needs cleaning
    DIRTYSOCK_ErrorName(ENOTNAM),           // 118 Not a XENIX named type file
    DIRTYSOCK_ErrorName(ENAVAIL),           // 119 No XENIX semaphores available
    DIRTYSOCK_ErrorName(EISNAM),            // 120 Is a named type file
    DIRTYSOCK_ErrorName(EREMOTEIO),         // 121 Remote I/O error
#endif
    DIRTYSOCK_ErrorName(EDQUOT),            // 122 Quota exceeded

#if defined(DIRTYCODE_APPLEIOS) || defined(DIRTYCODE_APPLEOSX)
    DIRTYSOCK_ErrorName(errSecUnimplemented),           // -4    Function or operation not implemented.
    DIRTYSOCK_ErrorName(errSecIO),                      // -35   I/O error (bummers)
    DIRTYSOCK_ErrorName(errSecParam),                   // -50   One or more parameters passed to a function were not valid.
    DIRTYSOCK_ErrorName(errSecAllocate),                // -108  Failed to allocate memory.
    DIRTYSOCK_ErrorName(errSecUserCanceled),            // -128  User canceled the operation.
    DIRTYSOCK_ErrorName(errSecBadReq),                  // -909  Bad parameter or invalid state for operation.

    DIRTYSOCK_ErrorName(errSSLProtocol),                // -9800 SSL protocol error
    DIRTYSOCK_ErrorName(errSSLNegotiation),             // -9801 Cipher Suite negotiation failure
    DIRTYSOCK_ErrorName(errSSLFatalAlert),              // -9802 Fatal alert
    DIRTYSOCK_ErrorName(errSSLWouldBlock),              // -9803 I/O would block (not fatal)
    DIRTYSOCK_ErrorName(errSSLSessionNotFound),         // -9804 attempt to restore an unknown session
    DIRTYSOCK_ErrorName(errSSLClosedGraceful),          // -9805 connection closed gracefully
    DIRTYSOCK_ErrorName(errSSLClosedAbort),             // -9806 connection closed via error
    DIRTYSOCK_ErrorName(errSSLXCertChainInvalid),       // -9806 invalid certificate chain
    DIRTYSOCK_ErrorName(errSSLBadCert),                 // -9808 bad certificate format
    DIRTYSOCK_ErrorName(errSSLCrypto),                  // -9809 underlying cryptographic error
    DIRTYSOCK_ErrorName(errSSLInternal),                // -9810 Internal error
    DIRTYSOCK_ErrorName(errSSLModuleAttach),            // -9811 module attach failure
    DIRTYSOCK_ErrorName(errSSLUnknownRootCert),         // -9812 valid cert chain, untrusted root
    DIRTYSOCK_ErrorName(errSSLNoRootCert),              // -9813 cert chain not verified by root
    DIRTYSOCK_ErrorName(errSSLCertExpired),             // -9814 chain had an expired cert
    DIRTYSOCK_ErrorName(errSSLCertNotYetValid),         // -9815 chain had a cert not yet valid
    DIRTYSOCK_ErrorName(errSSLClosedNoNotify),          // -9816 server closed session with no notification
    DIRTYSOCK_ErrorName(errSSLBufferOverflow),          // -9817 insufficient buffer provided
    DIRTYSOCK_ErrorName(errSSLBadCipherSuite),          // -9818 bad SSLCipherSuite

    DIRTYSOCK_ErrorName(errSSLPeerUnexpectedMsg),       // -9819 unexpected message received
    DIRTYSOCK_ErrorName(errSSLPeerBadRecordMac),        // -9820 bad MAC
    DIRTYSOCK_ErrorName(errSSLPeerDecryptionFail),      // -9821 decryption failed
    DIRTYSOCK_ErrorName(errSSLPeerRecordOverflow),      // -9822 record overflow
    DIRTYSOCK_ErrorName(errSSLPeerDecompressFail),      // -9823 decompression failure
    DIRTYSOCK_ErrorName(errSSLPeerHandshakeFail),       // -9824 handshake failure
    DIRTYSOCK_ErrorName(errSSLPeerBadCert),             // -9825 misc. bad certificate
    DIRTYSOCK_ErrorName(errSSLPeerUnsupportedCert),     // -9826 bad unsupported cert format
    DIRTYSOCK_ErrorName(errSSLPeerCertRevoked),         // -9827 certificate revoked
    DIRTYSOCK_ErrorName(errSSLPeerCertExpired),         // -9828 certificate expired
    DIRTYSOCK_ErrorName(errSSLPeerCertUnknown),         // -9829 unknown certificate
    DIRTYSOCK_ErrorName(errSSLIllegalParam),            // -9830 illegal parameter
    DIRTYSOCK_ErrorName(errSSLPeerUnknownCA),           // -9831 unknown Cert Authority
    DIRTYSOCK_ErrorName(errSSLPeerAccessDenied),        // -9832 access denied
    DIRTYSOCK_ErrorName(errSSLPeerDecodeError),         // -9833 decoding error
    DIRTYSOCK_ErrorName(errSSLPeerDecryptError),        // -9834 decryption error
    DIRTYSOCK_ErrorName(errSSLPeerExportRestriction),   // -9835 export restriction
    DIRTYSOCK_ErrorName(errSSLPeerProtocolVersion),     // -9836 bad protocol version
    DIRTYSOCK_ErrorName(errSSLPeerInsufficientSecurity),// -9837 insufficient security
    DIRTYSOCK_ErrorName(errSSLPeerInternalError),       // -9838 internal error
    DIRTYSOCK_ErrorName(errSSLPeerUserCancelled),       // -9839 user canceled
    DIRTYSOCK_ErrorName(errSSLPeerNoRenegotiation),     // -9840 no renegotiation allowed

    DIRTYSOCK_ErrorName(errSSLPeerAuthCompleted),       // -9841 peer cert is valid, or was ignored if verification disabled
    DIRTYSOCK_ErrorName(errSSLClientCertRequested),     // -9842 server has requested a client cert

    DIRTYSOCK_ErrorName(errSSLHostNameMismatch),        // -9843 peer host name mismatch
    DIRTYSOCK_ErrorName(errSSLConnectionRefused),       // -9844 peer dropped connection before responding
    DIRTYSOCK_ErrorName(errSSLDecryptionFail),          // -9845 decryption failure
    DIRTYSOCK_ErrorName(errSSLBadRecordMac),            // -9846 bad MAC
    DIRTYSOCK_ErrorName(errSSLRecordOverflow),          // -9847 record overflow
    DIRTYSOCK_ErrorName(errSSLBadConfiguration),        // -9848 configuration error
    DIRTYSOCK_ErrorName(errSSLUnexpectedRecord),        // -9849 unexpected (skipped) record in DTLS
    DIRTYSOCK_ErrorName(errSSLWeakPeerEphemeralDHKey),  // -9850 weak ephemeral dh key

    DIRTYSOCK_ErrorName(errSSLClientHelloReceived),     // -9851 SNI

    DIRTYSOCK_ErrorName(errSecNotAvailable),            // -25291 No keychain is available. You may need to restart your computer.
    DIRTYSOCK_ErrorName(errSecAuthFailed),              // -25293 The user name or passphrase you entered is not correct.
    DIRTYSOCK_ErrorName(errSecDuplicateItem),           // -25299 The specified item already exists in the keychain.
    DIRTYSOCK_ErrorName(errSecItemNotFound),            // -25300 The specified item could not be found in the keychain.
    DIRTYSOCK_ErrorName(errSecInteractionNotAllowed),   // -25308 User interaction is not allowed.
    DIRTYSOCK_ErrorName(errSecDecode),                  // -26275 Unable to decode the provided data.

    DIRTYSOCK_ErrorName(errSecVerifyFailed),            // -67808 A cryptographic verification failure has occured.
#endif

    // NULL terminate
    DIRTYSOCK_ListEnd()
};
#endif

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function DirtyErrNameList

    \Description
        This function takes as input a system-specific error code, and either
        resolves it to its define name if it is recognized or formats it as a hex
        number if not.

    \Input *pBuffer - [out] pointer to output buffer to store result
    \Input iBufSize - size of output buffer
    \Input uError   - error code to format
    \Input *pList   - error list to use

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
void DirtyErrNameList(char *pBuffer, int32_t iBufSize, uint32_t uError, const DirtyErrT *pList)
{
    #if DIRTYSOCK_ERRORNAMES
    int32_t iErr;

    for (iErr = 0; pList[iErr].uError != DIRTYSOCK_LISTTERM; iErr++)
    {
        if (pList[iErr].uError == uError)
        {
            ds_strnzcpy(pBuffer, pList[iErr].pErrorName, iBufSize);
            return;
        }
    }
    #endif

    ds_snzprintf(pBuffer, iBufSize, "0x%08x", uError);
}

/*F********************************************************************************/
/*!
    \Function DirtyErrName

    \Description
        This function takes as input a system-specific error code, and either
        resolves it to its define name if it is recognized or formats it as a hex
        number if not.

    \Input *pBuffer - [out] pointer to output buffer to store result
    \Input iBufSize - size of output buffer
    \Input uError   - error code to format

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
void DirtyErrName(char *pBuffer, int32_t iBufSize, uint32_t uError)
{
    #if DIRTYSOCK_ERRORNAMES
    DirtyErrNameList(pBuffer, iBufSize, uError, _DirtyErr_List);
    #endif
}

/*F********************************************************************************/
/*!
    \Function DirtyErrGetNameList

    \Description
        This function takes as input a system-specific error code, and either
        resolves it to its define name if it is recognized or formats it as a hex
        number if not.

    \Input uError   - error code to format
    \Input *pList   - error list to use

    \Output
        const char *- pointer to error name or error formatted in hex

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
const char *DirtyErrGetNameList(uint32_t uError, const DirtyErrT *pList)
{
    static char strName[8][64];
    static uint8_t uLast = 0;
    char *pName = strName[uLast++];
    if (uLast > 7) uLast = 0;
    DirtyErrNameList(pName, sizeof(strName[0]), uError, pList);
    return(pName);
}

/*F********************************************************************************/
/*!
    \Function DirtyErrGetName

    \Description
        This function takes as input a system-specific error code, and either
        resolves it to its define name if it is recognized or formats it as a hex
        number if not.

    \Input uError   - error code to format

    \Output
        const char *- pointer to error name or error formatted in hex

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
const char *DirtyErrGetName(uint32_t uError)
{
    static char strName[64];
    DirtyErrName(strName, sizeof(strName), uError);
    return(strName);
}
