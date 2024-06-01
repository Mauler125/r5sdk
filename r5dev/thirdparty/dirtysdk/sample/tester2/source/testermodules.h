/*H********************************************************************************/
/*!
    \File testermodules.h

    \Description
        Prototypes for tester modules, to be included in platform-specific
        implementations of the tester2 host application.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/22/2005 (jfrank) First Version
*/
/********************************************************************************H*/

#ifndef _testermodules_h
#define _testermodules_h

/*** Include files ****************************************************************/

#include "libsample/zlib.h"

/*** Defines **********************************************************************/

#define TESTERMODULES_COMMANDNAMESIZE_DEFAULT   (16)    //!< max command name length
#define TESTERMODULES_NUMCOMMANDS_DEFAULT       (64)    //!< max number of commands

#define TESTERMODULES_ERROR_NONE                (0)     //!< no error
#define TESTERMODULES_ERROR_NULLPOINTER         (-1)    //!< invalid pointer used
#define TESTERMODULES_ERROR_COMMANDLISTFULL     (-2)    //!< command list full - cannot register another command
#define TESTERMODULES_ERROR_NOSUCHCOMMAND       (-3)    //!< no such command in the list
#define TESTERMODULES_ERROR_NODIRTYSOCK         (-4)    //!< dirtysock isn't started
#define TESTERMODULES_ERROR_REDEFINITON         (-5)    //!< attempt to redefine a command

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

// module state
typedef struct TesterModulesT TesterModulesT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create a testermodules module
TesterModulesT *TesterModulesCreate(void);

// register a module
int32_t TesterModulesRegister(TesterModulesT *pState, const char *pCommand, ZCommand *pFunctionPtr);

// dispatch a command from an incoming command line
int32_t TesterModulesDispatch(TesterModulesT *pState, const char *pCommandLine);

// get help on a command, or NULL for help on all commands
void TesterModulesHelp(TesterModulesT *pState, const char *pCommand);

// return a specific registered module's command line name
int32_t TesterModulesGetCommandName(TesterModulesT *pState, int32_t iCommandNum, char *pBuf, int32_t iBufSize);

// destroy
void TesterModulesDestroy(TesterModulesT *pState);

// ---------------------------- CLIENT SPECIFIC ------------------------------

// modules registered in this function are available to clients on all platforms
int32_t TesterModulesRegisterClientCommands(TesterModulesT *pState);

// ---------------------------- HOST SPECIFIC --------------------------------

// modules registered in this function are available to hosts on all platforms
int32_t TesterModulesRegisterHostCommands(TesterModulesT *pState);

// ---------------------------- PLATFORM SPECIFIC ----------------------------

/* each platform will need to implement a version of this function to
   handle registering any platform-specific functions it will want */
int32_t TesterModulesRegisterPlatformCommands(TesterModulesT *pState);

// ---------------------------- TESTER MODULES -------------------------------

// tester2 built-ins
int32_t CmdExit(ZContext *argz, int32_t argc, char **argv);
int32_t CmdHelp(ZContext *argz, int32_t argc, char **argv);
int32_t CmdHistory(ZContext *argz, int32_t argc, char **argv);
int32_t CmdMemDebug(ZContext *argz, int32_t argc, char **argv);
int32_t CmdRegistry(ZContext *argz, int32_t argc, char **argv);
int32_t CmdRunScript(ZContext *argz, int32_t argc, char **argv);
int32_t CmdSleep(ZContext *argz, int32_t argc, char **argv);
int32_t CmdSource(ZContext *argz, int32_t argc, char **argv);

// tester2 modules
int32_t CmdAdvert(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdBase64(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdCrypt(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdDemangler(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdDriver(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdGameLink(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdHpack(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdHttp(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdHttp2(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdHttpMgr(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdHttpServ(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdImgConv(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdJson(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdLang(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdMbTest(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdNet(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdNetPrint(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdNetPrint(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdQos(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdSecure(ZContext *argz, int32_t argc, char *argv[]);
#if defined(DIRTYCODE_PS4)
int32_t CmdSession(ZContext *argz, int32_t argc, char *argv[]);
#endif
int32_t CmdSocket(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdStream(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdString(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdTime(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdTunnel(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdUpnp(ZContext *argz, int32_t argc, char *argv[]);
#if defined(DIRTYCODE_PS4) || defined (DIRTYCODE_XBOXONE) || defined(DIRTYCODE_STADIA)
int32_t CmdUser(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdUserList(ZContext *argz, int32_t argc, char *argv[]);
#endif
#if defined(DIRTYCODE_PS4) || defined (DIRTYCODE_XBOXONE) || defined(DIRTYCODE_STADIA)
int32_t CmdPriv(ZContext *argz, int32_t argc, char *argv[]);
#endif
int32_t CmdUtf8(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdVoice(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdWS(ZContext *argz, int32_t argc, char *argv[]);
int32_t CmdXml(ZContext *argz, int32_t argc, char *argv[]);

#ifdef __cplusplus
};
#endif

#endif // _testermodules_h

