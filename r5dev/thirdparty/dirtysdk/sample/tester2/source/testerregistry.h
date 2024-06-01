/*H********************************************************************************/
/*!
    \File testerregistry.h

    \Description
        Maintains a global registry for the tester2 application.

    \Notes
        This module works a little differently than other DirtySock modules.
        Because of the nature of a registry (a global collector of shared
        information) there is no pointer to pass around - Create simply
        allocates memory of a given size and Destroy frees this memory.
        Registry entries are stored in tagfield format.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 04/08/2005 (jfrank) First Version
*/
/********************************************************************************H*/

#ifndef _testerregistry_h
#define _testerregistry_h

/*** Include files ****************************************************************/

#define TESTERREGISTRY_ERROR_NONE                    (0)    //!< no error
#define TESTERREGISTRY_ERROR_NOTINITIALIZED         (-1)    //!< create not called
#define TESTERREGISTRY_ERROR_NOSUCHENTRY            (-2)    //!< entry requested does not exist
#define TESTERREGISTRY_ERROR_OUTOFSPACE             (-3)    //!< no more room left in the registry
#define TESTERREGISTRY_ERROR_BADDATA                (-4)    //!< bad data passed to a function

/*** Defines **********************************************************************/

#define TESTERREGISTRY_SIZE_DEFAULT             (4096)  //!< default registry size

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// pass in a registry size, or (-1) for default size, allocates that much memory.
// calling create multiple times frees the previous registry (no leak)
void TesterRegistryCreate(int32_t iSize);

// set a registry entry - pointer
int32_t TesterRegistrySetPointer(const char *pEntryName, const void *pPtr);
// set a registry entry - number
int32_t TesterRegistrySetNumber(const char *pEntryName, const int32_t iNum);
// set a registry entry - string
int32_t TesterRegistrySetString(const char *pEntryName, const char *pStr);

// get a registry entry - pointer - return error code (0 for none)
void *TesterRegistryGetPointer(const char *pEntryName);
// get a registry entry - pointer - return error code (0 for none)
int32_t TesterRegistryGetNumber(const char *pEntryName, int32_t *pNum);
// get a registry entry - pointer - return error code (0 for none)
int32_t TesterRegistryGetString(const char *pEntryName, char *pBuf, int32_t iBufSize);

// print the registry
void TesterRegistryPrint(void);

// destroy a registry
void TesterRegistryDestroy(void);

#ifdef __cplusplus
};
#endif

#endif // _testerregistry_h

