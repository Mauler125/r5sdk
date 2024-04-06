/*H********************************************************************************/
/*!
    \File testermoduleshost.c

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
int32_t TesterModulesRegisterHostCommands(TesterModulesT *pState)
{
    // tester2 built-ins
    TesterModulesRegister(pState, "help",       &CmdHelp);
    TesterModulesRegister(pState, "history",    &CmdHistory);
    TesterModulesRegister(pState, "!",          &CmdHistory);
    TesterModulesRegister(pState, "memdebug",   &CmdMemDebug);
    TesterModulesRegister(pState, "registry",   &CmdRegistry);
    TesterModulesRegister(pState, "source",     &CmdSource);

    // common tester2 modules
    TesterModulesRegister(pState, "base64",     &CmdBase64);
    TesterModulesRegister(pState, "crypt",      &CmdCrypt);
    TesterModulesRegister(pState, "gamelink",   &CmdGameLink);
    TesterModulesRegister(pState, "hpack",      &CmdHpack);
    TesterModulesRegister(pState, "http",       &CmdHttp);
    TesterModulesRegister(pState, "http2",      &CmdHttp2);
    TesterModulesRegister(pState, "httpmgr",    &CmdHttpMgr);
    TesterModulesRegister(pState, "httpserv",   &CmdHttpServ);
#if (defined(DIRTYCODE_PC))
    TesterModulesRegister(pState, "ic",         &CmdImgConv);
#endif
    TesterModulesRegister(pState, "json",       &CmdJson);
    TesterModulesRegister(pState, "lang",       &CmdLang);
    TesterModulesRegister(pState, "net",        &CmdNet);
    TesterModulesRegister(pState, "netprint",   &CmdNetPrint);
#if defined(DIRTYCODE_PS4) && !defined(DIRTYCODE_PS5)
    TesterModulesRegister(pState, "session",    &CmdSession);
#endif
    TesterModulesRegister(pState, "socket",     &CmdSocket);
#if defined(DIRTYCODE_PC)
    TesterModulesRegister(pState, "stream",     &CmdStream);
#endif
    TesterModulesRegister(pState, "string",     &CmdString);
    TesterModulesRegister(pState, "time",       &CmdTime);
    TesterModulesRegister(pState, "tunnel",     &CmdTunnel);
#if (defined(DIRTYCODE_PS4) || defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_STADIA)) && !defined(DIRTYCODE_PS5)
    TesterModulesRegister(pState, "user",       &CmdUser);
    TesterModulesRegister(pState, "userlist",   &CmdUserList);
#endif
#if (defined(DIRTYCODE_PS4) || defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_STADIA)) && !defined(DIRTYCODE_PS5)
    TesterModulesRegister(pState, "priv",       &CmdPriv);
#endif
    TesterModulesRegister(pState, "utf8",       &CmdUtf8);
    TesterModulesRegister(pState, "ws",         &CmdWS);
    TesterModulesRegister(pState, "xml",        &CmdXml);
    TesterModulesRegister(pState, "qos",        &CmdQos);

    // tester2 modules for non-Xbox platforms
#if !defined(DIRTYCODE_XBOXONE)
    TesterModulesRegister(pState, "demangler",  &CmdDemangler);
    TesterModulesRegister(pState, "upnp",       &CmdUpnp);
#endif

    return(TESTERMODULES_ERROR_NONE);
}
