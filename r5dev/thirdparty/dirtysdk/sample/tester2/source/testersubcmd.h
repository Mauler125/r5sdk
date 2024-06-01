/*H********************************************************************************/
/*!
    \File testersubcmd.h

    \Description
        Helper functions for modules to implement sub-commands.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 05/10/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _testersubcmd_h
#define _testersubcmd_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! subfunction command function prototype
typedef void (T2SubFuncT)(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);

//! subfunction command description
typedef struct T2SubCmdT
{
    char        strName[16];
    T2SubFuncT  *pFunc;
} T2SubCmdT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

//! display list of subcommands
void T2SubCmdUsage(const char *pCmdName, T2SubCmdT *pCmdList);

//! parse commandline to resolve subcommand
T2SubCmdT *T2SubCmdParse(T2SubCmdT *pCmdList, int32_t argc, char *argv[], unsigned char *pHelp);

#ifdef __cplusplus
};
#endif

#endif // _testersubcmd_h

