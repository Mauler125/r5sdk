/*H*************************************************************************************/
/*!
    \File    commall.h

    \Description
        This is the common communication header required for use with any
        of the CommXXXX routines. It provides a unified calling structure
        as well as unified status and error values.

    \Copyright
        Copyright (c) Electronic Arts 2002

    \Version 0.5 02/23/1999 (gschaefer) First Version
    \Version 1.0 02/25/1999 (gschaefer) Alpha release
    \Version 1.1 11/20/2002 (jbrookes) Added Send() flags parameter, protocol definitions.
*/
/*************************************************************************************H*/

#ifndef _commall_h
#define _commall_h

/*!
\Moduledef CommAll CommAll
\Modulemember Comm
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

#define COMM_OFFLINE        1       //!< status - offline
#define COMM_CONNECTING     2       //!< status - connecting
#define COMM_ONLINE         3       //!< status - online
#define COMM_FAILURE        4       //!< status - failure

#define COMM_PENDING        1       //!< pending
#define COMM_NOERROR        0       //!< no error
#define COMM_BADPARM        -1      //!< invalid parameter
#define COMM_BADSTATE       -2      //!< invalid state
#define COMM_BADADDRESS     -3      //!< invalid address
#define COMM_NORESOURCE     -4      //!< no resources available
#define COMM_UNEXPECTED     -5      //!< unexpected
#define COMM_MINBUFFER      -6      //!< minbuffer
#define COMM_NODATA         -7      //!< no data available
#define COMM_INPROGRESS     -8      //!< operation in progress
#define COMM_PORTBOUND      -9      //!< requested local port is already bound

// start of protocol numbering
#define COMM_PROTO_TCP      (1)     //!< TCP protocol
#define COMM_PROTO_UDP      (2)     //!< UDP protocol
#define COMM_PROTO_SRP      (3)     //!< SRP protocol

#define COMM_FLAGS_RELIABLE     0   //!< CommRef->Send() flag - use reliable transmission
#define COMM_FLAGS_UNRELIABLE   1   //!< CommRef->Send() flag - use unreliable transmission
#define COMM_FLAGS_BROADCAST    2   //!< CommRef->Send() flag - use unreliable broadcast transmission

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct CommRef CommRef;

//! common reference type
struct CommRef
{
    //! construct the class
    /*!
     * note: initialized winsock for first class. also creates linked
     *  list of all current instances of the class and worker thread
     *  to do most udp stuff.
     *
     * entry: maxwid=max record width, maxinp/maxout=input/output packet buffer size
     * exit: none
     */
    CommRef* (*Construct)(int32_t maxwid, int32_t maxinp, int32_t maxout);

    //! destruct the class
    /*!
     * entry: none
     * exit: none
     */
    void (*Destroy)(CommRef *what);

    //! resolve an address
    /*!
     * entry: what=endpoint ref, addr=address, buffer=target buffer, buflen=length
     * exit: <0=error, 0=in progress, >0=complete
     */
    int32_t (*Resolve)(struct CommRef *what, const char *addr, char *buf, int32_t len, char iDiv);

    //! stop the resolver
    /*!
     * entry: what=endpoint ref
     * exit: none
     */
    void (*Unresolve)(CommRef *what);

    //! listen for a connection
    /*!
     * entry: addr=port to listen on (only :port portion used)
     * exit: negative=error, zero=ok
     */
    int32_t (*Listen)(CommRef *what, const char *addr);

    //! stop listening
    /*!
     * entry: none
     * exit: negative=error, zero=ok
     */
    int32_t (*Unlisten)(CommRef *what);

    //! initiate a connection to a peer
    /*!
     * note: does not currently perform dns translation
     *
     * entry: addr=address in ip-address:port form
     * exit: negative=error, zero=ok
     */
    int32_t (*Connect)(CommRef *what, const char *addr);

    //! terminate a connection
    /*!
     * entry: none
     * exit: negative=error, zero=ok
     */
    int32_t (*Unconnect)(CommRef *what);

    //! set event callback hook
    /*!
     * Note1: this is an optional callback which is called after new data has been
     *  received and buffered or at other times where the protocol state has
     *  significantly changed. It is called by a private thread so the routines it
     *  uses must be thread safe. It can be used to provide "life" to servers or
     *  other modules which use comm services. The code must not take too long to
     *  execute since it will impact comm performance if it does.
     *
     * Note2: By disabling and enabling this callback at specific times, it is
     *  possible to avoid threading concerns in the upper layer (i.e., if you
     *  disable the callback while the callback is executing, this call will block
     *  until the callback completes).
     *
     * entry: comm reference
     * exit: none
     */
    void (*Callback)(CommRef *what, void (*callback)(CommRef *ref, int32_t event));

    //! return current stream status
    /*!
     * entry: none
     * exit: CONNECTING, OFFLINE, ONLINE or FAILURE
     */
    int32_t (*Status)(CommRef *what);

    //! control connection behavior (optional)
    /*!
     * see specific implementation for entry and exit parameter descriptions
     */
    int32_t (*Control)(CommRef *what, int32_t iControl, int32_t iValue, void *pValue);

    //! return current clock tick
    /*!
     * entry: none
     * exit: elapsed millisecs from some epoch
     */
    uint32_t (*Tick)(CommRef *what);

    //! send a packet
    /*!
     * note: zero length packets may not be sent (they are ignored)
     *
     * entry: buffer=pointer to data, length=length of data, flags=COMM_FLAGS_* (see defines above)
     * exit: negative=error, zero=ok
     */
    int32_t (*Send)(CommRef *what, const void *buffer, int32_t length, uint32_t flags);

    //! peek at waiting packet
    /*!
     * entry: target=target buffer, length=buffer length, when=tick received at
     * exit: negative=nothing pending, else packet length
     */
    int32_t (*Peek)(CommRef *what, void *target, int32_t length, uint32_t *when);

    //! receive a packet from the buffer
    /*!
     * entry: target=target buffer, length=buffer length, what=tick received at
     * exit: negative=error, else packet length
     */
    int32_t (*Recv)(CommRef *what, void *target, int32_t length, uint32_t *when);

    //! send data callback hook
    /*!
     * Note: this is an optional callback which is called immediately before
     *  a packet is transmitted. Due to error handling, it may get called more
     *  than once for the same packet if the packet is sent more than once.
     *
     * entry: same as Send
     * exit: none
     */
    void (*SendCallback)(CommRef *what, void *buffer, int32_t length, uint32_t when);

    //! receive data callback hook
    /*!
     * Note: this is an optional callback which is called immediately after
     *  a packet is received. By the time this function is called, the packet
     *  is available for input via the Recv function.
     *
     * entry: same as Recv
     * exit: none
     */
    void (*RecvCallback)(CommRef *what, void *target, int32_t length, uint32_t when);

    //! module memory group
    int32_t memgroup;
    void *memgrpusrdata;

    //! user definable storage
    int32_t refnum;

    //! user definable reference
    void *refptr;

    //! socket ref
    SocketT *sockptr;

    //! maximum packet width (read only)
    uint16_t maxwid;

    //! maximum number of input packets buffered
    uint8_t maxinp;

    //! maximum number of output packets buffered
    uint8_t maxout;

    //! data transfer statistics (read only)
    int32_t datasent;

    //! data transfer statistics (read only)
    int32_t datarcvd;

    //! data transfer statistics (read only)
    int32_t packsent;

    //! data transfer statistics (read only)
    int32_t packrcvd;

    //! data transfer statistics (read only)
    int32_t packlost;

    //! data tranfer statictics (read only)
    uint32_t packsaved; // tracks the number of packers recovered by commudp redundancy mechanism

    //! data transfer statistics (read only)
    int32_t naksent;

    //! data transfer statistics (read only)
    int32_t overhead;  // tracks the sent overhead including IP4 and UDP header

    //! data transfer statistics (read only)
    int32_t rcvoverhead; // tracks the receive overhead including IP4 and UDP header

    //!< host ip address (read only)
    uint32_t hostip;

    //!< peer ip address (read only)
    uint32_t peerip;

    //!< host port (read only)
    uint16_t hostport;

    //!< peer port (read only)
    uint16_t peerport;

    //!< if packet was received (read only)
    uint8_t bpackrcvd;

    uint8_t _pad[3];
};

// typedef versions for discrete declarations
typedef CommRef *(CommAllConstructT)(int32_t maxwid, int32_t maxinp, int32_t maxout);
typedef void (CommAllDestroyT)(CommRef *what);
typedef int32_t (CommAllResolveT)(struct CommRef *what, const char *addr, char *buf, int32_t len, char iDiv);
typedef void (CommAllUnresolveT)(CommRef *what);
typedef int32_t (CommAllListenT)(CommRef *what, const char *addr);
typedef int32_t (CommAllUnlistenT)(CommRef *what);
typedef int32_t (CommAllConnectT)(CommRef *what, const char *addr);
typedef int32_t (CommAllUnconnectT)(CommRef *what);
typedef void (CommAllCallbackT)(CommRef *what, void (*callback)(CommRef *ref, int32_t event));
typedef int32_t (CommAllStatusT)(CommRef *what);
typedef int32_t (CommAllControlT)(CommRef *what, int32_t iControl, int32_t iValue, void *pValue);
typedef uint32_t (CommAllTickT)(CommRef *what);
typedef int32_t (CommAllSendT)(CommRef *what, const void *buffer, int32_t length, uint32_t flags);
typedef int32_t (CommAllPeekT)(CommRef *what, void *target, int32_t length, uint32_t *when);
typedef int32_t (CommAllRecvT)(CommRef *what, void *target, int32_t length, uint32_t *when);
typedef void (CommAllSendCallbackT)(CommRef *what, const void *, int32_t length, uint32_t when);
typedef void (CommAllRecvCallbackT)(CommRef *what, void *target, int32_t length, uint32_t when);
typedef void (CommAllEventCallbackT)(CommRef *what, int32_t event);


/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

// known protocol constructors (these also appear in the .h file for the
// protocol in question, but are included here for convenience) -- see
// protocol .h file for more details

/*
 * Construct the class
 *
 * entry: maxwid=max record width, maxinp/maxout=input/output packet buffer size
 * exit: none
 */

#if 0
//typedef struct CommUDPRef CommUDPRef;
CommRef *CommUDPConstruct(int32_t maxwid, int32_t maxinp, int32_t maxout);

//typedef struct CommSRPRef CommSRPRef;
CommRef *CommSRPConstruct(int32_t maxwid, int32_t maxinp, int32_t maxout);
#endif

//@}

#endif // _commall_h









