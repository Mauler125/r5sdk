/*H********************************************************************************/
/*!
    \File dirtycert.h

    \Description
        This module defines the CA fallback mechanism which is used by ProtoSSL.

    \Copyright
        Copyright (c) 2012 Electronic Arts Inc.

    \Version 01/23/2012 (szhu)
*/
/********************************************************************************H*/

#ifndef _dirtycert_h
#define _dirtycert_h

/*!
\Moduledef NetConnDefs NetConnDefs
\Modulemember DirtySock
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/proto/protossl.h"

/*** Defines **********************************************************************/
#define DIRTYCERT_SERVICENAME_SIZE   (128)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

// opaque module state ref
typedef struct DirtyCertRefT DirtyCertRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create dirtycert module
DIRTYCODE_API int32_t DirtyCertCreate(void);

// release resources and destroy module
DIRTYCODE_API int32_t DirtyCertDestroy(void);

// initiate a CA fetch request
DIRTYCODE_API int32_t DirtyCertCARequestCert(const ProtoSSLCertInfoT *pCertInfo, const char *pHost, int32_t iPort);

// initiate a CA prefetch request
DIRTYCODE_API void DirtyCertCAPreloadCerts(const char *pServiceName);

// if a CA fetch request is complete
DIRTYCODE_API int32_t DirtyCertCARequestDone(int32_t iRequestId);

// release resources used by a CA fetch request
DIRTYCODE_API int32_t DirtyCertCARequestFree(int32_t iRequestId);

// control module behavior
DIRTYCODE_API int32_t DirtyCertControl(int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// get module status
DIRTYCODE_API int32_t DirtyCertStatus(int32_t iStatus, void *pBuffer, int32_t iBufSize);

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtycert_h
