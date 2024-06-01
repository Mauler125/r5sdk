/*H********************************************************************************/
/*!
    \File zmemtrack.h

    \Description
        Routines for tracking memory allocations.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 02/15/2005 (jbrookes) First Version, based on jfrank's implementation.
*/
/********************************************************************************H*/

#ifndef _zmemtrack_h
#define _zmemtrack_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

#define ZMEMTRACK_PRINTFLAG_TRACKING        (1)     //!< print more verbose output

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! logging function
typedef void (ZMemtrackLogCbT)(const char *pText, void *pUserData);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// init Zmemtrack module
DIRTYCODE_API void ZMemtrackStartup(void);

// shut down Zmemtrack module
DIRTYCODE_API void ZMemtrackShutdown(void);

// set the logging callback
DIRTYCODE_API void ZMemtrackCallback(ZMemtrackLogCbT *pLoggingCb, void *pUserData);

// track an allocation
DIRTYCODE_API void ZMemtrackAlloc(void *pMem, uint32_t uSize, uint32_t uTag);

// track a free operation
DIRTYCODE_API void ZMemtrackFree(void *pMem, uint32_t *pSize);

// print current tracking info
DIRTYCODE_API void ZMemtrackPrint(uint32_t uFlags, uint32_t uTag, const char *pModuleName);

#ifdef __cplusplus
};
#endif

#endif // _zmemtrack_h

