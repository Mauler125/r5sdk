/*H********************************************************************************/
/*!
    \File dirtyerr.h

    \Description
        Dirtysock debug error routines.

    \Copyright
        Copyright (c) 2005 Electronic Arts

    \Version 06/13/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _dirtyerr_h
#define _dirtyerr_h

/*!
\Moduledef DirtyErr DirtyErr
\Modulemember DirtySock
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

#define DIRTYSOCK_ERRORNAMES    (DIRTYCODE_LOGGING && TRUE)
#define DIRTYSOCK_LISTTERM      (0x45454545)

/*** Macros ***********************************************************************/

#if DIRTYSOCK_ERRORNAMES
#define DIRTYSOCK_ErrorName(_iError)    { (uint32_t)_iError, #_iError }
#define DIRTYSOCK_ListEnd()             { DIRTYSOCK_LISTTERM, "" }
#endif

/*** Type Definitions *************************************************************/

typedef struct DirtyErrT
{
    uint32_t uError;
    const char  *pErrorName;
} DirtyErrT;

#ifdef DIRTYCODE_PS4

typedef void (*DirtySockAppErrorCallback)(int32_t errorCode);

#endif

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DIRTYCODE_PS4

//set Application Error Callback
void DirtyErrAppCallbackSet(DirtySockAppErrorCallback pCallback);

//Inovke App Error Callback if set to report sony error code back to application layer
void DirtyErrAppReport(int32_t iError);

#endif

// take a system-specific error code, and either resolve it to its define name or format it as a hex number
DIRTYCODE_API void DirtyErrName(char *pBuffer, int32_t iBufSize, uint32_t uError);

// same as DirtyErrName, but references the specified list
DIRTYCODE_API void DirtyErrNameList(char *pBuffer, int32_t iBufSize, uint32_t uError, const DirtyErrT *pList);

// same as DirtyErrName, except a pointer is returned
DIRTYCODE_API const char *DirtyErrGetName(uint32_t uError);

// same as DirtyErrGetName, but references the specified list
DIRTYCODE_API const char *DirtyErrGetNameList(uint32_t uError, const DirtyErrT *pList);

// create a unique error code for use accross DirtySDK
DIRTYCODE_API uint32_t DirtyErrGetHResult(uint16_t uFacility, int16_t iCode, uint8_t bFailure);

// break a hresult back into its components
DIRTYCODE_API void DirtyErrDecodeHResult(uint32_t hResult, uint16_t* uFacility, int16_t* iCode, uint8_t* bCustomer, uint8_t* bFailure);

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtyerr_h

