/*H********************************************************************************/
/*!
    \File protoupnp.h

    \Description
        Implements a simple UPnP client, designed specifically to talk to a UPnP
        router and open up a firewall port for peer-peer communication with a
        remote client.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/23/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _protoupnp_h
#define _protoupnp_h

/*!
\Moduledef ProtoUpnp ProtoUpnp
\Modulemember Proto
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

// status bits for ProtoUpnpStatus('stat')

#define PROTOUPNP_STATUS_DISCOVERED (1)     //!< discovered a upnp device
#define PROTOUPNP_STATUS_DESCRIBED  (2)     //!< described a upnp device
#define PROTOUPNP_STATUS_GOTEXTADDR (4)     //!< got external address for device
#define PROTOUPNP_STATUS_ADDPORTMAP (8)     //!< successfully added port mapping
#define PROTOUPNP_STATUS_DELPORTMAP (16)    //!< successfully deleted port mapping
#define PROTOUPNP_STATUS_FNDPORTMAP (32)    //!< found existing port mapping

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! protoupnp macro element definition
typedef struct ProtoUpnpMacroT
{
    int32_t iControl;
    int32_t iValue;
    int32_t iValue2;
    void    *pValue;
} ProtoUpnpMacroT;

//! opaque module ref
typedef struct ProtoUpnpRefT ProtoUpnpRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module
DIRTYCODE_API ProtoUpnpRefT *ProtoUpnpCreate(void);

// get module ref
DIRTYCODE_API ProtoUpnpRefT *ProtoUpnpGetRef(void);

// destroy the module
DIRTYCODE_API void ProtoUpnpDestroy(ProtoUpnpRefT *pProtoUpnp);

// get module status
DIRTYCODE_API int32_t ProtoUpnpStatus(ProtoUpnpRefT *pProtoUpnp, int32_t iSelect, void *pBuf, int32_t iBufSize);

// protoupnp control
DIRTYCODE_API int32_t ProtoUpnpControl(ProtoUpnpRefT *pProtoUpnp, int32_t iControl, int32_t iValue, int32_t iValue2, const void *pValue);

// update the module
DIRTYCODE_API void ProtoUpnpUpdate(ProtoUpnpRefT *pProtoUpnp);

#ifdef __cplusplus
};
#endif

//@}

#endif // _protoupnp_h

