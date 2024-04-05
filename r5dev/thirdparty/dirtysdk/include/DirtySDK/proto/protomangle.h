/*H*************************************************************************************************/
/*!

    \File    protomangle.h

    \Description
        This module encapsulates use of the EA.Com Demangler service.

    \Notes
        The ProtoMangle client was developed from version 1.1 of the Peer Connection Service
        Protocol Specification:

            http://docs.online.ea.com/infrastructure/demangler/peer-connection-service-protocol.doc

        Please see the following documentation for information on the Demangler service:

            http://docs.online.ea.com/infrastructure/demangler/demangler.html
            http://docs.online.ea.com/infrastructure/demangler/peer-connection-service-reqs.doc
            http://docs.online.ea.com/infrastructure/demangler/peer-connection-service-engineering-spec.doc
            http://docs.online.ea.com/infrastructure/demangler/Peer-Connection-Service-Architecture.doc

        Original work done by C&C: Red Alert 2, on which the Demangler service is based:

            http://www.worldwide.ea.com/articles/render/attachment.aspx?id=919

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2003.  ALL RIGHTS RESERVED.

    \Version    1.0        04/03/2003 (JLB) First Version
*/
/*************************************************************************************************H*/

#ifndef _protomangle_h
#define _protomangle_h

/*!
\Moduledef ProtoMangle ProtoMangle
\Modulemember Proto
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

//! max cookie length, including terminator
#define PROTOMANGLE_STRCOOKIE_MAX   (64)

//! max gameFeatureID length, including terminator
#define PROTOMANGLE_STRGAMEID_MAX   (32)

//! max LKey length, including terminator
#define PROTOMANGLE_STRLKEY_MAX     (64)

//! max server name length, including terminator
#define PROTOMANGLE_STRSERVER_MAX   (32)

/*
    Default server name/port
*/

//! default server name
#define PROTOMANGLE_SERVER          ("demangler.ea.com")

//! development server
#define PROTOMANGLE_TEST_SERVER     ("peach.online.ea.com")

//! default server port
#define PROTOMANGLE_PORT            (3658)

//! response codes
typedef enum ProtoMangleStatusE
{
    PROTOMANGLE_STATUS_CONNECTED,
    PROTOMANGLE_STATUS_FAILED,

    PROTOMANGLE_NUMSTATUS
} ProtoMangleStatusE;

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct ProtoMangleRefT ProtoMangleRefT;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// allocate module state and prepare for use
DIRTYCODE_API ProtoMangleRefT *ProtoMangleCreate(const char *pServer, int32_t iPort, const char *pGameID, const char *pLKey);

// destroy the module and release its state
DIRTYCODE_API void ProtoMangleDestroy(ProtoMangleRefT *pRef);

// give time to module to do its thing (should be called periodically to allow module to perform work)
DIRTYCODE_API void ProtoMangleUpdate(ProtoMangleRefT *pRef);

// connect to demangler server
DIRTYCODE_API void ProtoMangleConnect(ProtoMangleRefT *pRef, int32_t iGamePort, const char *pSessID);

// connect to demangler server (extended)
DIRTYCODE_API void ProtoMangleConnect2(ProtoMangleRefT *pRef, int32_t iGamePort, const char *pSessID, int32_t iSessIDLen);

// connect to demangler server, using the given socket ref to issue UDP probes instead of creating one
DIRTYCODE_API void ProtoMangleConnectSocket(ProtoMangleRefT *pRef, intptr_t uSocketRef, const char *pSessID);

// get result
DIRTYCODE_API int32_t ProtoMangleComplete(ProtoMangleRefT *pRef, int32_t *pAddr, int32_t *pPort);

// submit result to server
DIRTYCODE_API int32_t ProtoMangleReport(ProtoMangleRefT *pRef, ProtoMangleStatusE eStatus, int32_t iLatency);

// protomangle control
DIRTYCODE_API int32_t ProtoMangleControl(ProtoMangleRefT *pRef, int32_t iControl, int32_t iValue, int32_t iValue2, const void *pValue);

// get module status based on selector
DIRTYCODE_API int32_t ProtoMangleStatus(ProtoMangleRefT *pRef, int32_t iSelect, void *pBuf, int32_t iBufSize);

// find a secure template for given network parameters
#if defined(DIRTYCODE_XBOXONE) && !defined(DIRTYCODE_GDK)
const char *ProtoMangleFindTemplate(char *pStrTemplateName, int32_t iTemplateNameSize, int32_t iLocalPort, int32_t iRemotePort, uint8_t bTcp, int32_t iVerbose);
#endif

#ifdef __cplusplus
}
#endif

//@}

#endif // _protomangle_h


