/*H********************************************************************************/
/*!
    \File testermodulesclient.c

    \Description
        PC specific module startup.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 04/11/2005 (jfrank) First Version
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
    \Function TesterModulesRegisterHostCommands

    \Description
        Register all PC-specific modules

    \Input *pState - module state
    
    \Output 0=success, error code otherwise

    \Version 04/11/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterModulesRegisterClientCommands(TesterModulesT *pState)
{
    TesterModulesRegister(pState, "run",        &CmdRunScript);
    TesterModulesRegister(pState, "runscript",  &CmdRunScript);
    TesterModulesRegister(pState, "history",    &CmdHistory);
    TesterModulesRegister(pState, "!",          &CmdHistory);
    TesterModulesRegister(pState, "reg",        &CmdRegistry);
    TesterModulesRegister(pState, "registry",   &CmdRegistry);
    TesterModulesRegister(pState, "sleep",   &CmdSleep);
    return(TESTERMODULES_ERROR_NONE);
}

