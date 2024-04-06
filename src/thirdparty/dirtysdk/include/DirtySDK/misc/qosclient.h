/*H********************************************************************************/
/*!
    \File qosclient.h

    \Description
        Main include for the Quality of Service module.

    \Copyright
        Copyright (c) 2017 Electronic Arts Inc.

    \Version 1.0 04/07/2017 (cvienneau) 2.0
*/
/********************************************************************************H*/

#ifndef _qosclient_h
#define _qosclient_h

/*!
\Moduledef QosClient QosClient
\Modulemember Misc
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/misc/qoscommon.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Forward Declarations *********************************************************/

/*** Type Definitions *************************************************************/

//! opaque module ref
typedef struct QosClientRefT QosClientRefT;

//! event callback function prototype
typedef void (QosClientCallbackT)(QosClientRefT *pQosClient, QosCommonProcessedResultsT *pResults, void *pUserData);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module state
DIRTYCODE_API QosClientRefT *QosClientCreate(QosClientCallbackT *pCallback, void *pUserData, uint16_t uListenPort);

// let the module do processing
DIRTYCODE_API void QosClientUpdate(QosClientRefT *pQosClient);

// begin QOS process managed by the QOS coordinator
DIRTYCODE_API void QosClientStart(QosClientRefT *pQosClient, const char *pStrCoordinatorAddr, uint16_t uPort, const char * strQosProfile);

// change the behavior of the QOS system
DIRTYCODE_API int32_t QosClientControl(QosClientRefT *pQosClient, int32_t iControl, int32_t iValue, void *pValue);

// get the status, of the QOS system
DIRTYCODE_API int32_t QosClientStatus(QosClientRefT *pQosClient, int32_t iSelect, int32_t iData, void *pBuf, int32_t iBufSize);

// destroy the module state
DIRTYCODE_API void QosClientDestroy(QosClientRefT *pQosClient);

#ifdef __cplusplus
}
#endif

//@}

#endif // _filename_h

