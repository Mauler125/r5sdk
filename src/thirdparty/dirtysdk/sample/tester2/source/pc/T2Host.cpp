/*H********************************************************************************/
/*!
    \File T2Host.cpp

    \Description
        Main file for Tester2 Host Application.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/22/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)

#include <winuser.h>
#include <crtdbg.h>
#include <stdlib.h>
#include <stdio.h>

#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/util/jsonformat.h"

#include "libsample/zmemtrack.h"
#include "libsample/zlib.h"

#include "testerhostcore.h"
#include "testercomm.h"
#include "testerregistry.h"

#include "t2hostresource.h"

/*** Defines **********************************************************************/

#ifndef GWL_WNDPROC
#define GWL_WNDPROC GWLP_WNDPROC // pc64
#endif

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

TesterHostCoreT *g_pHostCore;   //!< global host core pointer
HWND g_pMainWin = NULL;         //!< pointer to the main window
WNDPROC g_hEditProc;            //!< edit box handler
int32_t g_iTimerID;             //!< handle to global timer to activate callbacks

/*** Private Functions ************************************************************/

#if defined(DIRTYCODE_DLL)
// pull in the dependencies to call DirtyMemFuncSet
extern "C" void* DirtyMemAlloc2(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void* pMemGroupUserData);
extern "C" void DirtyMemFree2(void* pMem, int32_t iMemModule, int32_t iMemGroup, void* pMemGroupUserData); 
#endif

/*F********************************************************************************/
/*!
    \Function _T2HostDisplayOutput

    \Description
        Take input from TesterConsole and dump it to the specified edit box.

    \Input *pBuf    - string containing the debug output to display
    \Input  iLen    - length of buffer
    \Input  iRefcon - user-specified parameter (unused)
    \Input *pRefptr - user-specified parameter (window pointer)
    
    \Output None

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
static void _T2HostDisplayOutput(const char *pBuf, int32_t iLen, int32_t iRefcon, void *pRefptr)
{
    int32_t iAdd;
    HWND Ctrl = (HWND)pRefptr;
    char strTempText[16*1025], *pTempText, cPrev;
    
    // replace cr with crlf for proper display
    for (pTempText = strTempText, cPrev = '\0'; (*pBuf != '\0') && ((pTempText-strTempText) < (sizeof(strTempText)-1)); pBuf++, pTempText++)
    {
        if ((*pBuf == '\n') && (cPrev != '\r'))
        {
            *pTempText++ = '\r';
        }
        *pTempText = cPrev = *pBuf;
    }
    *pTempText = '\0';

    // see if we need to delete old data
    if (SendMessage(Ctrl, WM_GETTEXTLENGTH, 0, 0) > 24000)
    {
        SendMessage(Ctrl, EM_SETSEL, 0, 8192);
        SendMessage(Ctrl, EM_REPLACESEL, FALSE, (LPARAM)"");
        iAdd = SendMessage(Ctrl, WM_GETTEXTLENGTH, 0, 0);
        SendMessage(Ctrl, EM_SETSEL, (WPARAM)(iAdd-1), iAdd);
        SendMessage(Ctrl, EM_REPLACESEL, FALSE, (LPARAM)"");
    }

    iAdd = SendMessage(Ctrl, WM_GETTEXTLENGTH, 0, 0);
    SendMessage(Ctrl, EM_SETSEL, (WPARAM)iAdd, iAdd);
    SendMessage(Ctrl, EM_REPLACESEL, FALSE, (LPARAM)strTempText);
    SendMessage(Ctrl, EM_SCROLLCARET, 0, 0);
}


/*F********************************************************************************/
/*!
    \Function _T2HostTimerCallback

    \Description
        Timer callback - used to activate the TesterCoreIdle function.

    \Input Not used.
    
    \Output None

    \Version 03/22/2005 (jfrank)
*/
/********************************************************************************F*/
VOID CALLBACK _T2HostTimerCallback(HWND hDlg, UINT uMsg, UINT_PTR pIDEvent, DWORD dTime)
{
    // pump the networking layer
    TesterHostCoreIdle(g_pHostCore);
}

/*F********************************************************************************/
/*!
    \Function _T2HostCommandEditProc

    \Description
        Message handler for the command line edit box.  Intercepts uparrow, etc.

    \Input hDlg     - dialog handle
    \Input uMessage - message identifier
    \Input wParam   - message specifics (pointer to uint32_t)
    \Input lParam   - message specifics (pointer)
    
    \Output LRESULT - message handling status return code

    \Version 04/06/2005 (jfrank)
*/
/********************************************************************************F*/
static LRESULT CALLBACK _T2HostCommandEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    const char *pCommandLine = NULL;

    if (msg == WM_KEYDOWN)
    {
        if (wParam == VK_UP)
        {
            pCommandLine = TesterHostCoreGetHistory(g_pHostCore, -1, NULL, 0);
        }
        else if (wParam == VK_DOWN)
        {
            pCommandLine = TesterHostCoreGetHistory(g_pHostCore, 1, NULL, 0);
        }

        // if we got a key, redraw and quit
        if (pCommandLine != NULL)
        {
            SetWindowText(hWnd, pCommandLine);
            SendMessage(hWnd, EM_SETSEL, 0, -1);
            SendMessage(hWnd, EM_SETSEL, (WPARAM)-1, 0);
            return(0);
        }
    }
    
    // not handled by this handler - call the original one
    return(CallWindowProc(static_cast<WNDPROC>(g_hEditProc), hWnd, msg, wParam, lParam));
}


/*F********************************************************************************/
/*!
    \Function _T2HostDialogProc

    \Description
        Main window dialog handler

    \Input Standard windows WNDPROC
    
    \Output Message special.

    \Version 03/22/2005 (jfrank)
*/
/********************************************************************************F*/
static LRESULT CALLBACK _T2HostDialogProc(HWND win, UINT msg, WPARAM wparm, LPARAM lparm)
{
    // handle close special (delete the class)
    if (msg == WM_DESTROY) 
    {
        return(FALSE);
    }

    // handle init special (create the class)
    if (msg == WM_INITDIALOG) 
    {
        HFONT font = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_DONTCARE, "");
        SendDlgItemMessage(win, IDC_OUTPUT, WM_SETFONT, (WPARAM)font, 0);

        // set up command window handler
        g_hEditProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(GetDlgItem(win, IDC_INPUT), GWL_WNDPROC, (LONG_PTR)_T2HostCommandEditProc));

        // start the timer for IDLE callback handling
        g_iTimerID = (int32_t)SetTimer( win, 1, (uint32_t)(1000/60), _T2HostTimerCallback);
    }

    // close dialog
    if (msg == WM_CLOSE) 
    {
        TesterHostCoreDisconnect(g_pHostCore);
        KillTimer(win, (UINT_PTR)g_iTimerID);
        PostQuitMessage(0);
        return(DefWindowProc(win, msg, wparm, lparm));
    }

    // look for return key
    if ((msg == WM_COMMAND) && (LOWORD(wparm) == IDOK))
    {
        if (g_pHostCore != NULL)
        {
            char strInput[16*1024];

            // get the command line
            GetWindowText(GetDlgItem(win, IDC_INPUT), strInput, sizeof(strInput));
            SetWindowText(GetDlgItem(win, IDC_INPUT), "");

            // local echo
            ZPrintf("\n> %s\n", strInput);

            // and execute the command
            TesterHostCoreDispatch(g_pHostCore, strInput);
        }
    }
    
    // let windows handle
    return(FALSE);
}


/*F********************************************************************************/
/*!
    \Function _T2HostCmdClear

    \Description
        Clear the console

    \Input  *argz   - environment
    \Input   argc   - num args
    \Input **argv   - arg list
    
    \Output  int32_t    - standard return code

    \Version 04/07/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _T2HostCmdClear(ZContext *argz, int32_t argc, char **argv)
{
    TesterConsoleT *pConsole;

    if (argc < 1)
    {
        ZPrintf("   clear the display.\n");
        ZPrintf("   usage: %s\n", argv[0]);
    }
    else
    {
        // clear the display
        SetWindowText(GetDlgItem(g_pMainWin, IDC_OUTPUT), "");
        
        // clear the console
        if ((pConsole = (TesterConsoleT *)TesterRegistryGetPointer("CONSOLE")) != NULL)
        {
            TesterConsoleClear(pConsole);
        }
    }
    return(0);
}


/*F********************************************************************************/
/*!
    \Function _T2HostCmdExit

    \Description
        Quit

    \Input  *argz   - environment
    \Input   argc   - num args
    \Input **argv   - arg list
    
    \Output  int32_t    - standard return code

    \Version 04/05/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _T2HostCmdExit(ZContext *argz, int32_t argc, char **argv)
{
    if (argc >= 1)
    {
        PostQuitMessage(0);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _T2HostRegisterModules

    \Description
        Register client commands (local commands, like exit, history, etc.)

    \Input None
    
    \Output None

    \Version 04/05/2005 (jfrank)
*/
/********************************************************************************F*/
static void _T2HostRegisterModules(void)
{
    TesterHostCoreRegister(g_pHostCore, "exit",     &_T2HostCmdExit);
    TesterHostCoreRegister(g_pHostCore, "clear",    &_T2HostCmdClear);
}


/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function WinMain

    \Description
        Main windows entry point.

    \Input Standard windows startup params
    
    \Output Process exit code

    \Version 03/22/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t APIENTRY WinMain(HINSTANCE inst, HINSTANCE prev, char *cmdline, int32_t show)
{
    char strBase[128] = "", strHostName[128] = "", *pBase, strParams[512];
    MSG msg;

    ZPrintf("\nStarting T2Host.\n\n");

    // check for path argument (indicates file passing)
    if ((pBase = strstr(cmdline, "-path=")) != NULL)
    {
        pBase += strlen("-path=");
        ds_strnzcpy(strBase, pBase, sizeof(strBase));
        ZPrintf("t2host: base path=%s\n", strBase);
    }
   
    // check for connect argument (indicates host connect instead of accept)
    if ((pBase = strstr(cmdline, "-connect=")) != NULL)
    {
        pBase += strlen("-connect=");
        ds_strnzcpy(strHostName, pBase, sizeof(strHostName));
        ZPrintf("t2host: connect=%s\n", strHostName);
    }

#if defined(DIRTYCODE_DLL)

    DirtyMemFuncSet(&DirtyMemAlloc2, &DirtyMemFree2);

#endif

    ZMemtrackStartup();

    // start the network
    NetConnStartup("-servicename=tester2");

    // create the module
    JsonInit(strParams, sizeof(strParams), 0);
    JsonAddStr(strParams, "INPUTFILE", TESTERCOMM_HOSTINPUTFILE);
    JsonAddStr(strParams, "OUTPUTFILE", TESTERCOMM_HOSTOUTPUTFILE);
    JsonAddStr(strParams, "CONTROLDIR", strBase);
    JsonAddStr(strParams, "HOSTNAME", strHostName);
    g_pHostCore = TesterHostCoreCreate(JsonFinish(strParams));

    // create the tester dialog
    g_pMainWin = CreateDialogParam(GetModuleHandle(NULL), "MAIN", HWND_DESKTOP, (DLGPROC)_T2HostDialogProc, 0);

    TesterHostCoreDisplayFunc(g_pHostCore, _T2HostDisplayOutput, 0, GetDlgItem(g_pMainWin, IDC_OUTPUT));

    _T2HostRegisterModules();

    // command-line command?
    if (cmdline[0] != '\0')
    {
        // set the command
        SetWindowText(GetDlgItem(g_pMainWin, IDC_INPUT), cmdline);
        // fake a carriage return
        _T2HostDialogProc(g_pMainWin, WM_COMMAND, IDOK, 0);
    }

    // main message loop
    while (GetMessage(&msg, NULL, 0, 0))
    {
        // pump the host core module
        TesterHostCoreUpdate(g_pHostCore, 1);

        // let dialog manager run
        if (!IsDialogMessage(g_pMainWin, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // give time to zlib
        ZTask();
        ZCleanup();

        // give time to network
        NetConnIdle();
    }

    // kill all active processes
    ZShutdown();

    // kill the host core module
    TesterHostCoreDestroy(g_pHostCore);

    // done with dialog
    DestroyWindow(g_pMainWin);

    // shut down the network
    NetConnShutdown(0);

    ZMemtrackShutdown();

    _CrtDumpMemoryLeaks();

    ZPrintf("\nQuitting T2Host.\n\n");

    return(0);
}


