/*H********************************************************************************/
/*!
    \File T2Client.cpp

    \Description
        Defines the entry point for the application.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/21/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#endif

#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)

#include <stdio.h>
#include <stdlib.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/util/jsonformat.h"
#include "DirtySDK/util/jsonparse.h"
#include "libsample/zmem.h"
#include "libsample/zmemtrack.h"
#include "testermodules.h"
#include "testerprofile.h"
#include "testerregistry.h"
#include "testerclientcore.h"
#include "testercomm.h"
#include "T2ClientResource.h"

/*** Defines **********************************************************************/

#define MAX_LOADSTRING (100)

// custom events
#define WM_CLEARSCREEN            (WM_APP+1000)

/*** Type Definitions *************************************************************/

typedef struct T2ClientGeometryT
{
    uint32_t uTopBorder;
    uint32_t uConsoleBottomToCommandTop;
    uint32_t uCommandHeight;
    uint32_t uBottomBorder;
    uint32_t uLeftBorder;
    uint32_t uRightBorder;
} T2ClientGeometryT;

typedef struct T2ClientT
{
    HINSTANCE hInst;                            //!< current instance
    TCHAR strTitle[MAX_LOADSTRING];             //!< The title bar text
    TCHAR strWindowClass[MAX_LOADSTRING];       //!< the main window class name
    char strCurrentProfile[TESTERPROFILE_PROFILEFILENAME_SIZEDEFAULT];  // active profile
    TesterClientCoreT *pClientCore;             //!< pointer to client core structure
    int32_t iTimerID;                               //!< timer used to call NetIdle();
    WNDPROC CommandEditProc;                    //!< edit box handler
    WNDPROC ConsoleEditProc;                    //!< console box handler
    int32_t iCommandHistoryOffset;                  //!< command history offset
    T2ClientGeometryT Geometry;                 //!< window geometry
    char strConnectParams[512];                 //!< connect parameters
    char strScript[256];                        //!< script to execute
    int32_t iScrollbackSize;
} T2ClientT;

/*** forward Declaration ********************************************************************/
VOID CALLBACK _T2ClientTimerCallback(HWND hDlg, UINT uMsg, UINT_PTR pIDEvent, DWORD dTime);


/*** Variables ********************************************************************/

static T2ClientT T2Client = {NULL, "", "", "", NULL, 0, NULL, NULL, 1, {0,0,0,0,0,0}};

/*** External Functions ***********************************************************/

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _T2ClientDisplayOutput

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
static void _T2ClientDisplayOutput(const char *pBuf, int32_t iLen, int32_t iRefcon, void *pRefptr)
{
    int32_t iAdd;
    HWND Ctrl = (HWND)pRefptr;
    char strTempText[16*1024], *pTempText, cPrev;
    
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
    if (SendMessage(Ctrl, WM_GETTEXTLENGTH, 0, 0) > T2Client.iScrollbackSize) {
        SendMessage(Ctrl, EM_SETSEL, 0, 8192);
        SendMessage(Ctrl, EM_REPLACESEL, FALSE, (LPARAM)"");
        iAdd = SendMessage(Ctrl, WM_GETTEXTLENGTH, 0, 0);
        SendMessage(Ctrl, EM_SETSEL, iAdd-1, iAdd);
        SendMessage(Ctrl, EM_REPLACESEL, FALSE, (LPARAM)"");
    }

    iAdd = SendMessage(Ctrl, WM_GETTEXTLENGTH, 0, 0);
    SendMessage(Ctrl, EM_SETSEL, iAdd, iAdd);
    SendMessage(Ctrl, EM_REPLACESEL, FALSE, (LPARAM)strTempText);
    SendMessage(Ctrl, EM_SCROLLCARET, 0, 0);    
}


/*F********************************************************************************/
/*!
    \Function _T2ClientCommandEditProc

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
static LRESULT CALLBACK _T2ClientCommandEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    const char *pCommandLine = NULL;

    if (msg == WM_KEYDOWN)
    {
        // adjust if necessary
        if (wParam == VK_UP)
        {
            T2Client.iCommandHistoryOffset--;
            pCommandLine = TesterClientCoreGetHistoricalCommand(T2Client.pClientCore, &(T2Client.iCommandHistoryOffset), NULL, 0);
        }
        else if (wParam == VK_DOWN)
        {
            T2Client.iCommandHistoryOffset++;
            pCommandLine = TesterClientCoreGetHistoricalCommand(T2Client.pClientCore, &(T2Client.iCommandHistoryOffset), NULL, 0);
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
    return(CallWindowProc( static_cast<WNDPROC>(T2Client.CommandEditProc), hWnd, msg, wParam, lParam ));
}


/*F********************************************************************************/
/*!
    \Function _T2ClientConsoleEditProc

    \Description
        Message handler for the console box.  Intercepts messages.

    \Input hDlg     - dialog handle
    \Input uMessage - message identifier
    \Input wParam   - message specifics (pointer to uint32_t)
    \Input lParam   - message specifics (pointer)
    
    \Output LRESULT - message handling status return code

    \Version 04/06/2005 (jfrank)
*/
/********************************************************************************F*/
static LRESULT CALLBACK _T2ClientConsoleEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{    
    int32_t wmId, wmEvent;

    // find out the control ID which requires attention
    wmId    = LOWORD(wParam); 
    wmEvent = HIWORD(wParam);

    if (msg == WM_CONTEXTMENU)
    {
        // Create the popup menu
        HMENU hPopup;
        POINT p;
        
        GetCursorPos(&p);
        hPopup = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE( IDR_CONSOLECONTEXTMENU ));
        hPopup = GetSubMenu(hPopup, 0);
        TrackPopupMenu(hPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON, p.x, p.y, 0, hWnd, NULL);
        return(0);
    }
    else if (msg == WM_COMMAND)
    {
        if (wmId == ID_CONSOLEOPTIONS_CLEAR)
        {
            TesterConsoleT *pConsole;

            if ((pConsole = (TesterConsoleT *)TesterRegistryGetPointer("CONSOLE")) != NULL)
            {
                TesterConsoleClear(pConsole);
                SetWindowText(hWnd, "");
            }
            return(0);
        }
        else if (wmId == ID_CONSOLEOPTIONS_SELECTALL)
        {
            SendMessage(hWnd, EM_SETSEL, 0, -1);           
        }
        else if (wmId == ID_CONSOLEOPTIONS_RECONNECT)
        {
            ZPrintf("T2Client ---> Reconnecting the client and host.\n");
            TesterClientCoreDisconnect(T2Client.pClientCore);
            TesterClientCoreConnect(T2Client.pClientCore, T2Client.strConnectParams);
        }
        else if (wmId == ID_CONSOLEOPTIONS_DISCONNECT)
        {
            ZPrintf("T2Client ---> Disconnecting the client from the host.\n");
            TesterClientCoreDisconnect(T2Client.pClientCore);
        }
    }
    // not handled by this handler - call the original one
    return(CallWindowProc( static_cast<WNDPROC>(T2Client.ConsoleEditProc), hWnd, msg, wParam, lParam));
}


/*F********************************************************************************/
/*!
    \Function _T2ClientCommandConsole

    \Description
        Message handler for command console box.

    \Input hDlg     - dialog handle
    \Input uMessage - message identifier
    \Input wParam   - message specifics (pointer to uint32_t)
    \Input lParam   - message specifics (pointer)
    
    \Output LRESULT - message handling status return code

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
static LRESULT CALLBACK _T2ClientCommandConsole(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    const char strTitlePrefix[] = "TesterClient:";
    char strCommand[TESTERCOMM_COMMANDSIZE_MAX];
    char strTitle[TESTERPROFILE_PROFILEFILENAME_SIZEDEFAULT + sizeof(strTitlePrefix) + 1];
    char *pCommand;
    int32_t wmId, wmEvent;
    HFONT font;
    RECT CommandConsoleRect, ConsoleRect, CommandRect;
    uint32_t uX, uY, uWidth, uHeight;

    // find out the control ID which requires attention
    wmId    = LOWORD(wParam); 
    wmEvent = HIWORD(wParam); 

    switch (uMessage)
    {
    case WM_INITDIALOG:
        {
            char strHostname[128];
            uint16_t aJson[512];

            // get the window geometry
            GetWindowRect(hDlg, &CommandConsoleRect);
            GetWindowRect(GetDlgItem(hDlg, IDC_EDIT_CONSOLE), &ConsoleRect);
            GetWindowRect(GetDlgItem(hDlg, IDC_EDIT_COMMAND), &CommandRect);
            T2Client.Geometry.uLeftBorder = ConsoleRect.left - CommandConsoleRect.left - 4;
            T2Client.Geometry.uRightBorder = CommandConsoleRect.right - ConsoleRect.right - 4;
            T2Client.Geometry.uBottomBorder = CommandConsoleRect.bottom - CommandRect.bottom;
            T2Client.Geometry.uConsoleBottomToCommandTop = CommandRect.top - ConsoleRect.bottom;
            T2Client.Geometry.uCommandHeight = CommandRect.bottom - CommandRect.top;
            T2Client.Geometry.uTopBorder = ConsoleRect.top - 23;

            // set up custom handler for up arrow
            #if defined(_WIN64)
            SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
            T2Client.CommandEditProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr( GetDlgItem(hDlg, IDC_EDIT_COMMAND), GWLP_WNDPROC,(LPARAM)_T2ClientCommandEditProc ));
            T2Client.ConsoleEditProc  = reinterpret_cast<WNDPROC>(SetWindowLongPtr( GetDlgItem(hDlg, IDC_EDIT_CONSOLE), GWLP_WNDPROC,(LPARAM)_T2ClientConsoleEditProc ));
            #else
            SetWindowLong(hDlg, GWL_USERDATA, lParam);
            T2Client.CommandEditProc = reinterpret_cast<WNDPROC>(SetWindowLong( GetDlgItem(hDlg, IDC_EDIT_COMMAND), GWL_WNDPROC,(LPARAM)_T2ClientCommandEditProc ));
            T2Client.ConsoleEditProc  = reinterpret_cast<WNDPROC>(SetWindowLong( GetDlgItem(hDlg, IDC_EDIT_CONSOLE), GWL_WNDPROC,(LPARAM)_T2ClientConsoleEditProc ));
            #endif

            // set the font
            font = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_DONTCARE, "");
            SendDlgItemMessage(hDlg, IDC_EDIT_CONSOLE, WM_SETFONT, (LPARAM)font, 0);

            // set the display function
            TesterClientCoreDisplayFunc(T2Client.pClientCore, _T2ClientDisplayOutput, 0, GetDlgItem(hDlg, IDC_EDIT_CONSOLE));

            SendMessage( GetDlgItem(hDlg, IDC_EDIT_CONSOLE), (UINT) EM_LIMITTEXT, T2Client.iScrollbackSize+8192, 0 );

            // set the title bar
            JsonParse(aJson, sizeof(aJson)/sizeof(*aJson), T2Client.strConnectParams, -1);
            JsonGetString(JsonFind(aJson, "HOSTNAME"), strHostname, sizeof(strHostname), "");
            if (strHostname[0] == '\0')
            {
                ds_strnzcpy(strHostname, "listening", sizeof(strHostname));
            }
            ds_snzprintf(strTitle, sizeof(strTitle), "%s %s (%s)", strTitlePrefix, T2Client.strCurrentProfile, strHostname);
            SetWindowText(hDlg, strTitle);

            // start the timer for IDLE callback handling
            T2Client.iTimerID = SetTimer( GetParent(hDlg), 1, (uint32_t)(1000/60), _T2ClientTimerCallback);
        }

        break;
    case WM_SIZE:

        // get the new main window size
        GetClientRect(hDlg, &CommandConsoleRect);

        // draw the console
        uX = CommandConsoleRect.left + T2Client.Geometry.uLeftBorder;
        uY = CommandConsoleRect.top + T2Client.Geometry.uTopBorder;
        uWidth = (CommandConsoleRect.right - CommandConsoleRect.left) 
            - T2Client.Geometry.uLeftBorder - T2Client.Geometry.uRightBorder;
        uHeight = (CommandConsoleRect.bottom - CommandConsoleRect.top)
            - T2Client.Geometry.uBottomBorder - T2Client.Geometry.uTopBorder
            - T2Client.Geometry.uCommandHeight - T2Client.Geometry.uConsoleBottomToCommandTop;
        MoveWindow(GetDlgItem(hDlg, IDC_EDIT_CONSOLE), uX, uY, uWidth, uHeight, TRUE);

        // draw the command window - shares some stuff with the console
        uY = CommandConsoleRect.bottom - T2Client.Geometry.uBottomBorder
            - T2Client.Geometry.uCommandHeight;
        uHeight = T2Client.Geometry.uCommandHeight;
        MoveWindow(GetDlgItem(hDlg, IDC_EDIT_COMMAND), uX, uY, uWidth, uHeight, TRUE);
        
        break;
    case WM_COMMAND:
        // check for EXIT button or exit icon (upper right X)
        if ((wmId == ID_BUTTON_EXIT) || (wmId == IDCANCEL))
        {
            EndDialog(hDlg, wmId);
            PostQuitMessage(0);
            return(TRUE);
        }
        // check to see if we hit enter
        else if (wmId == IDOK)
        {
            GetWindowText( GetDlgItem(hDlg, IDC_EDIT_COMMAND), &(strCommand[1]), sizeof(strCommand)-2);
    
            // specially process the ! command
            if ((strCommand[1] == '!') || (strCommand[1] == '?'))
            {
                strCommand[0] = strCommand[1];
                strCommand[1] = ' ';
                pCommand = &(strCommand[0]);
            }
            else
            {
                pCommand = &(strCommand[1]);
            }
            TesterClientCoreSendCommand(T2Client.pClientCore, pCommand);
            SetWindowText( GetDlgItem(hDlg, IDC_EDIT_COMMAND), "");
            // reset the offset
            T2Client.iCommandHistoryOffset = 1;
        }
        // end <case WM_COMMAND>
        break;
    }
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function _T2ClientTimerCallback

    \Description
        Timer callback - used to activate the TesterCoreIdle function.

    \Input Not used.
    
    \Output None

    \Version 03/22/2005 (jfrank)
*/
/********************************************************************************F*/
VOID CALLBACK _T2ClientTimerCallback(HWND hDlg, UINT uMsg, UINT_PTR pIDEvent, DWORD dTime)
{
    if (T2Client.pClientCore)
    {
        if(strlen(T2Client.strScript) > 0)
        {
            char strCommand[256];
            sprintf(strCommand, "runscript %s", T2Client.strScript);
            TesterClientCoreSendCommand(T2Client.pClientCore, strCommand);
            T2Client.strScript[0] = '\0';
        }

        // pump the networking layer
        TesterClientCoreIdle(T2Client.pClientCore);
    }
    return;
}

/*F********************************************************************************/
/*!
    \Function _T2ClientProfilesLoad

    \Description
        Load all the profile information into the profile dialog combo boxes.

    \Input hDlg     - dialog handle
    
    \Output None

    \Version 03/22/2005 (jfrank)
*/
/********************************************************************************F*/
static void _T2ClientProfilesLoad(HWND hDlg)
{
    TesterProfileEntryT Entry;
    int32_t iEntryNum = 0;

    // get all the entries
    while(TesterClientCoreProfileGet(T2Client.pClientCore, iEntryNum++, &Entry) >= 0)
    {
        // set the dialog combo boxes
        SendMessage( GetDlgItem(hDlg, IDC_COMBO_PROFILENAME), 
            (UINT)CB_INSERTSTRING, (WPARAM)-1, (LPARAM)(LPCTSTR)(Entry.strProfileName));
        SendMessage( GetDlgItem(hDlg, IDC_COMBO_SHARINGLOCATION), 
            (UINT)CB_INSERTSTRING, (WPARAM)-1, (LPARAM)(LPCTSTR)(Entry.strControlDirectory));
        SendMessage( GetDlgItem(hDlg, IDC_COMBO_STARTUPPARAMS), 
            (UINT)CB_INSERTSTRING, (WPARAM)-1, (LPARAM)(LPCTSTR)(Entry.strCommandLine));
        SendMessage( GetDlgItem(hDlg, IDC_COMBO_FILELOGDIRECTORY), 
            (UINT)CB_INSERTSTRING, (WPARAM)-1, (LPARAM)(LPCTSTR)(Entry.strLogDirectory));
        SendMessage( GetDlgItem(hDlg, IDC_COMBO_HOSTNAME),
            (UINT)CB_INSERTSTRING, (WPARAM)-1, (LPARAM)(LPCTSTR)(Entry.strHostname));
    }
}

/*F********************************************************************************/
/*!
    \Function _T2ClientProfileDialogReset

    \Description
        Reset the contents and selections in the profile dialog box
        and reload all the profiles.

    \Input hDlg     - dialog handle
    
    \Output None

    \Version 03/22/2005 (jfrank)
*/
/********************************************************************************F*/
static void _T2ClientProfileDialogReset(HWND hDlg)
{
    // clear all the lists
    SendMessage( GetDlgItem(hDlg, IDC_COMBO_PROFILENAME),       (UINT)CB_RESETCONTENT,  0, 0);
    SendMessage( GetDlgItem(hDlg, IDC_COMBO_SHARINGLOCATION),   (UINT)CB_RESETCONTENT,  0, 0);
    SendMessage( GetDlgItem(hDlg, IDC_COMBO_STARTUPPARAMS),     (UINT)CB_RESETCONTENT,  0, 0);
    SendMessage( GetDlgItem(hDlg, IDC_COMBO_FILELOGDIRECTORY),  (UINT)CB_RESETCONTENT,  0, 0);
    SendMessage( GetDlgItem(hDlg, IDC_COMBO_HOSTNAME),          (UINT)CB_RESETCONTENT,  0, 0);
    // clear the check boxes
    SendMessage( GetDlgItem(hDlg, IDC_CHECKBOX_LOGGING),        (UINT)BM_SETCHECK, BST_UNCHECKED, 0);
    SendMessage( GetDlgItem(hDlg, IDC_CHECKBOX_STARTNETWORKING),(UINT)BM_SETCHECK, BST_UNCHECKED, 0);
    // load all the profiles
    _T2ClientProfilesLoad(hDlg);
}

/*F********************************************************************************/
/*!
    \Function _T2ClientProfileSave

    \Description
        Save the currently selected profile using the profile manager

    \Input hDlg     - dialog handle
    \Input *pEntry  - [out,optional] storage for entry
    
    \Version 03/22/2005 (jfrank)
*/
/********************************************************************************F*/
static void _T2ClientProfileSave(HWND hDlg, TesterProfileEntryT *pEntry)
{
    TesterProfileEntryT Entry;
    ds_memclr(&Entry, sizeof(Entry));

    // create the profile entry parts
    GetWindowText(GetDlgItem(hDlg, IDC_COMBO_PROFILENAME),      Entry.strProfileName,      sizeof(Entry.strProfileName)-1);
    GetWindowText(GetDlgItem(hDlg, IDC_COMBO_SHARINGLOCATION),  Entry.strControlDirectory, sizeof(Entry.strControlDirectory)-1);
    GetWindowText(GetDlgItem(hDlg, IDC_COMBO_STARTUPPARAMS),    Entry.strCommandLine,      sizeof(Entry.strCommandLine)-1);
    GetWindowText(GetDlgItem(hDlg, IDC_COMBO_FILELOGDIRECTORY), Entry.strLogDirectory,     sizeof(Entry.strLogDirectory)-1);
    GetWindowText(GetDlgItem(hDlg, IDC_COMBO_HOSTNAME),         Entry.strHostname,         sizeof(Entry.strHostname)-1);
    Entry.uLogEnable      = (unsigned char)SendMessage( GetDlgItem(hDlg, IDC_CHECKBOX_LOGGING),         (UINT)BM_GETCHECK, 0, 0);
    Entry.uNetworkStartup = (unsigned char)SendMessage( GetDlgItem(hDlg, IDC_CHECKBOX_STARTNETWORKING), (UINT)BM_GETCHECK, 0, 0);

    // make sure we have a profile name
    if (Entry.strProfileName[0] == '\0')
    {
        ds_snzprintf(Entry.strProfileName, sizeof(Entry.strProfileName), "profile-%d\n", NetRand(32*1024));
    }

    // add the profile
    TesterClientCoreProfileAdd(T2Client.pClientCore, &Entry);

    // copy back to user
    if (pEntry != NULL)
    {
        ds_memcpy(pEntry, &Entry, sizeof(*pEntry));
    }
}


/*F********************************************************************************/
/*!
    \Function _T2ClientSetActiveProfile

    \Description
        Set the active profile in the profile list

    \Input  hDlg          - dialog handle
    \Input  iItemNum      - item number in the list
    
    \Output None

    \Version 03/28/2005 (jfrank)
*/
/********************************************************************************F*/
static void _T2ClientSetActiveProfile(HWND hDlg, int32_t iItemNum)
{
    TesterProfileEntryT Entry;
    //char strProfile[TESTERPROFILE_PROFILEFILENAME_SIZEDEFAULT];
    //char strPlatform[TESTERPROFILE_PLATFORM_SIZEDEFAULT];
    //uint32_t uLogEnable, uNetworkStartup;

    // determine the numeric settings for the profile
    iItemNum = TesterClientCoreProfileGet(T2Client.pClientCore, iItemNum, &Entry);

    // set the defaults
    SendMessage( GetDlgItem(hDlg, IDC_COMBO_PROFILENAME),       (UINT)CB_SETCURSEL, iItemNum, 0);
    SendMessage( GetDlgItem(hDlg, IDC_COMBO_SHARINGLOCATION),   (UINT)CB_SETCURSEL, iItemNum, 0);
    SendMessage( GetDlgItem(hDlg, IDC_COMBO_STARTUPPARAMS),     (UINT)CB_SETCURSEL, iItemNum, 0);
    SendMessage( GetDlgItem(hDlg, IDC_COMBO_FILELOGDIRECTORY),  (UINT)CB_SETCURSEL, iItemNum, 0);
    SendMessage( GetDlgItem(hDlg, IDC_COMBO_HOSTNAME),          (UINT)CB_SETCURSEL, iItemNum, 0);
    SendMessage( GetDlgItem(hDlg, IDC_CHECKBOX_LOGGING),        (UINT)BM_SETCHECK, (Entry.uLogEnable      ? BST_CHECKED : BST_UNCHECKED), 0);
    SendMessage( GetDlgItem(hDlg, IDC_CHECKBOX_STARTNETWORKING),(UINT)BM_SETCHECK, (Entry.uNetworkStartup ? BST_CHECKED : BST_UNCHECKED), 0);
}



/*F********************************************************************************/
/*!
    \Function _T2ClientProfileSelect

    \Description
        Message handler for profile select dialog.

    \Input hDlg     - dialog handle
    \Input uMessage - message identifier
    \Input wParam   - message specifics (pointer to uint32_t)
    \Input lParam   - message specifics (pointer)
    
    \Output LRESULT - message handling status return code

    \Version 03/21/2005 (jfrank)
*/
/********************************************************************************F*/
static LRESULT CALLBACK _T2ClientProfileSelect(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    TesterProfileEntryT Entry;
    int32_t wmId, wmEvent;
    int32_t iItemNum;

    // find out the control ID which requires attention
    wmId    = LOWORD(wParam); 
    wmEvent = HIWORD(wParam); 

    switch (uMessage)
    {
    case WM_INITDIALOG:
        // wipe out all the lists and start over
        _T2ClientProfileDialogReset(hDlg);
        // activate the default profile
        _T2ClientSetActiveProfile(hDlg, -1);
        break;
    case WM_COMMAND:
        // check for EXIT button or exit icon (upper right X)
        if ((wmId == IDCANCEL) || (wmId == ID_BUTTON_EXIT))
        {
            EndDialog(hDlg, wmId);
            PostQuitMessage(0);
            return(TRUE);
        }
        // check for action on the profile name
        else if (wmId == IDC_COMBO_PROFILENAME)
        {
            if (wmEvent == CBN_SELENDOK)
            {
                // we've selected something in the profile list
                iItemNum = SendMessage( GetDlgItem(hDlg, IDC_COMBO_PROFILENAME), (UINT)CB_GETCURSEL, 0, 0);
                _T2ClientSetActiveProfile(hDlg, iItemNum);
            }
        }
        // check for action on the sharing location
        else if (wmId == IDC_COMBO_SHARINGLOCATION)
        {
            // nothing to do
        }
        // check for action on the sharing location
        else if (wmId == IDC_COMBO_HOSTNAME)
        {
            // nothing to do
        }
        // check for saving the profile
        else if (wmId == IDC_BUTTON_SAVEPROFILE)
        {
            GetWindowText( GetDlgItem(hDlg, IDC_COMBO_PROFILENAME), Entry.strProfileName, sizeof(Entry.strProfileName)-1);
            if (strlen(Entry.strProfileName) > 0)
            {
                // save the current profile manually
                _T2ClientProfileSave(hDlg, NULL);
                // reset the window state
                _T2ClientProfileDialogReset(hDlg);
                // now set the current one as default and display it
                TesterClientCoreProfileDefault(T2Client.pClientCore, Entry.strProfileName);
                _T2ClientSetActiveProfile(hDlg, -1);
            }
        }
        // check for deleting the profile
        else if (wmId == IDC_BUTTON_DELETEPROFILE)
        {
            ds_memclr((void *)Entry.strProfileName, sizeof(Entry.strProfileName));
            GetWindowText( GetDlgItem(hDlg, IDC_COMBO_PROFILENAME), Entry.strProfileName, sizeof(Entry.strProfileName)-1);
            // delete the current profile
            TesterClientCoreProfileDelete(T2Client.pClientCore, Entry.strProfileName); 
            // reset the window state
            _T2ClientProfileDialogReset(hDlg);
        }
        // connect?
        else if (wmId == ID_BUTTON_CONNECT)
        {
            char buffer[64];

            // save the current profile manually
            _T2ClientProfileSave(hDlg, &Entry);
            // set it as the default
            TesterClientCoreProfileDefault(T2Client.pClientCore, Entry.strProfileName);
            ds_strnzcpy(T2Client.strCurrentProfile, Entry.strProfileName, sizeof(T2Client.strCurrentProfile));
            // get profile index
            iItemNum = SendMessage(GetDlgItem(hDlg, IDC_COMBO_PROFILENAME), (UINT)CB_GETCURSEL, 0, 0);

            // create the specified entry parameters
            JsonInit(T2Client.strConnectParams, sizeof(T2Client.strConnectParams), 0);
            JsonAddInt(T2Client.strConnectParams, "PROFILENUM", iItemNum);
            JsonAddStr(T2Client.strConnectParams, "PROFILENAME", Entry.strProfileName);

            JsonAddStr(T2Client.strConnectParams, "INPUTFILE", TESTERCOMM_CLIENTINPUTFILE);
            JsonAddStr(T2Client.strConnectParams, "OUTPUTFILE", TESTERCOMM_CLIENTOUTPUTFILE);
            JsonAddStr(T2Client.strConnectParams, "CONTROLDIR", Entry.strControlDirectory);
            JsonAddStr(T2Client.strConnectParams, "HOSTNAME", Entry.strHostname);
 
            // connect the client core
            TesterClientCoreConnect(T2Client.pClientCore, JsonFinish(T2Client.strConnectParams));

            // start the timer for IDLE callback handling
            T2Client.iTimerID = SetTimer( GetParent(hDlg), 1, (uint32_t)(1000/60), _T2ClientTimerCallback);

            GetWindowText(GetDlgItem(hDlg, IDC_SCROLLBACKSIZE), buffer, sizeof(buffer));

            T2Client.iScrollbackSize = atol(buffer)*1024;
            if (T2Client.iScrollbackSize<512000)
            {
                T2Client.iScrollbackSize = 512000;
            }

            // and kill this dialog
            EndDialog(hDlg, wmId);
            return(TRUE);
        }

        // end <case WM_COMMAND>
        break;
    }
    return(FALSE);
}


/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function WinMain

    \Description
        Main() routine.  Starts GUI.

    \Input hInstance     - window instance
    \Input hPrevInstance - previous (calling) window instance
    \Input lpCmdLine     - command line parameters
    \Input iCmdShow      - show/hide the window
    
    \Output 0 for success

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                        LPTSTR    lpCmdLine, int32_t       iCmdShow)
{
    ZPrintf(("\nStarting T2Client.\n\n"));

    ZMemtrackStartup();

    // start up networking
    NetConnStartup("");

    // create and connect the core module
    T2Client.pClientCore = TesterClientCoreCreate(lpCmdLine);
    
    if(lpCmdLine[0] == 0)
    {
        DialogBox(T2Client.hInst, (LPCTSTR)IDD_PROFILESELECT,  NULL, (DLGPROC)_T2ClientProfileSelect);
    }
    else
    {
        char strPlatform[256];
        char strHost[256];
        char strLogFile[256];
        char strControlDir[256];

        ds_strnzcpy(strPlatform, strtok(lpCmdLine, " "), sizeof(strPlatform));
        ds_strnzcpy(strHost, strtok(NULL, " "), sizeof(strHost));
        ds_strnzcpy(T2Client.strScript, strtok(NULL, " "), sizeof(T2Client.strScript));
        ds_strnzcpy(strControlDir, strtok(NULL, " "), sizeof(strControlDir));
        ds_strnzcpy(strLogFile, strtok(NULL, " "), sizeof(strLogFile));

        ds_strnzcpy(T2Client.strCurrentProfile, "auto", sizeof(T2Client.strCurrentProfile));

        JsonInit(T2Client.strConnectParams, sizeof(T2Client.strConnectParams), 0);
        JsonAddStr(T2Client.strConnectParams, "INPUTFILE", TESTERCOMM_CLIENTINPUTFILE);
        JsonAddStr(T2Client.strConnectParams, "OUTPUTFILE", TESTERCOMM_CLIENTOUTPUTFILE);
        JsonAddStr(T2Client.strConnectParams, "LOGFILE", strLogFile);
        JsonAddStr(T2Client.strConnectParams, "CONTROLDIR", strControlDir);
        JsonAddStr(T2Client.strConnectParams, "HOSTNAME", strHost);
        // connect the client core
        TesterClientCoreConnect(T2Client.pClientCore, JsonFinish(T2Client.strConnectParams));
    }
    DialogBox(T2Client.hInst, (LPCTSTR)IDD_COMMANDCONSOLE, NULL, (DLGPROC)_T2ClientCommandConsole);

    // disconnect and destroy the client core module
    TesterClientCoreDestroy(T2Client.pClientCore);

    // shut down networking
    NetConnShutdown(0);

    ZMemtrackShutdown();

    ZPrintf(("Quitting T2Client.\n"));

    // return success
    return(0);
}


