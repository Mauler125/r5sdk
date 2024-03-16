/*H********************************************************************************/
/*!
    \File testermoduleshost.c

    \Description
        Unix-specific module startup.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 04/13/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function TesterModulesRegisterAllCommands

    \Description
        Register all modules for the Unix platform

    \Input *pState - module state
    
    \Output 0=success, error code otherwise

    \Version 04/13/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterModulesRegisterPlatformCommands(TesterModulesT *pState)
{
    #if defined(DIRTYCODE_STADIA)
    TesterModulesRegister(pState, "voice",   &CmdVoice);
    #endif

    // tester2 linux-specific modules
    return(TESTERMODULES_ERROR_NONE);
}

