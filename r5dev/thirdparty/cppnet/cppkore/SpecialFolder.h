#pragma once

#include <Shlobj.h>

namespace System
{
	enum class SpecialFolder
	{
		//  
		//      Represents the file system directory that serves as a common repository for
		//       application-specific data for the current, roaming user. 
		//       A roaming user works on more than one computer on a network. A roaming user's 
		//       profile is kept on a server on the network and is loaded onto a system when the
		//       user logs on. 
		//  
		ApplicationData = CSIDL_APPDATA,
		//  
		//      Represents the file system directory that serves as a common repository for application-specific data that
		//       is used by all users. 
		//  
		CommonApplicationData = CSIDL_COMMON_APPDATA,
		//  
		//     Represents the file system directory that serves as a common repository for application specific data that
		//       is used by the current, non-roaming user. 
		//  
		LocalApplicationData = CSIDL_LOCAL_APPDATA,
		//  
		//     Represents the file system directory that serves as a common repository for Internet
		//       cookies. 
		//  
		Cookies = CSIDL_COOKIES,
		Desktop = CSIDL_DESKTOP,
		//  
		//     Represents the file system directory that serves as a common repository for the user's
		//       favorite items. 
		//  
		Favorites = CSIDL_FAVORITES,
		//  
		//     Represents the file system directory that serves as a common repository for Internet
		//       history items. 
		//  
		History = CSIDL_HISTORY,
		//  
		//     Represents the file system directory that serves as a common repository for temporary 
		//       Internet files. 
		//  
		InternetCache = CSIDL_INTERNET_CACHE,
		//  
		//      Represents the file system directory that contains
		//       the user's program groups. 
		//  
		Programs = CSIDL_PROGRAMS,
		MyComputer = CSIDL_DRIVES,
		MyMusic = CSIDL_MYMUSIC,
		MyPictures = CSIDL_MYPICTURES,
		//      "My Videos" folder
		MyVideos = CSIDL_MYVIDEO,
		//  
		//     Represents the file system directory that contains the user's most recently used
		//       documents. 
		//  
		Recent = CSIDL_RECENT,
		//  
		//     Represents the file system directory that contains Send To menu items. 
		//  
		SendTo = CSIDL_SENDTO,
		//  
		//     Represents the file system directory that contains the Start menu items. 
		//  
		StartMenu = CSIDL_STARTMENU,
		//  
		//     Represents the file system directory that corresponds to the user's Startup program group. The system
		//       starts these programs whenever any user logs on to Windows NT, or
		//       starts Windows 95 or Windows 98. 
		//  
		Startup = CSIDL_STARTUP,
		//  
		//     System directory.
		//  
		System = CSIDL_SYSTEM,
		//  
		//     Represents the file system directory that serves as a common repository for document
		//       templates. 
		//  
		Templates = CSIDL_TEMPLATES,
		//  
		//     Represents the file system directory used to physically store file objects on the desktop.
		//       This should not be confused with the desktop folder itself, which is
		//       a virtual folder. 
		//  
		DesktopDirectory = CSIDL_DESKTOPDIRECTORY,
		//  
		//     Represents the file system directory that serves as a common repository for documents. 
		//  
		Personal = CSIDL_PERSONAL,
		//          
		// "MyDocuments" is a better name than "Personal"
		//
		MyDocuments = CSIDL_PERSONAL,
		//  
		//     Represents the program files folder. 
		//  
		ProgramFiles = CSIDL_PROGRAM_FILES,
		//  
		//     Represents the folder for components that are shared across applications. 
		//  
		CommonProgramFiles = CSIDL_PROGRAM_FILES_COMMON,
		//
		//      <user name>\Start Menu\Programs\Administrative Tools
		//
		AdminTools = CSIDL_ADMINTOOLS,
		//
		//      USERPROFILE\Local Settings\Application Data\Microsoft\CD Burning
		//
		CDBurning = CSIDL_CDBURN_AREA,
		//
		//      All Users\Start Menu\Programs\Administrative Tools
		//
		CommonAdminTools = CSIDL_COMMON_ADMINTOOLS,
		//
		//      All Users\Documents
		//
		CommonDocuments = CSIDL_COMMON_DOCUMENTS,
		//
		//      All Users\My Music
		//
		CommonMusic = CSIDL_COMMON_MUSIC,
		//
		//      Links to All Users OEM specific apps
		//
		CommonOemLinks = CSIDL_COMMON_OEM_LINKS,
		//
		//      All Users\My Pictures
		//
		CommonPictures = CSIDL_COMMON_PICTURES,
		//
		//      All Users\Start Menu
		//
		CommonStartMenu = CSIDL_COMMON_STARTMENU,
		//
		//      All Users\Start Menu\Programs
		//
		CommonPrograms = CSIDL_COMMON_PROGRAMS,
		//
		//     All Users\Startup
		//
		CommonStartup = CSIDL_COMMON_STARTUP,
		//
		//      All Users\Desktop
		//
		CommonDesktopDirectory = CSIDL_COMMON_DESKTOPDIRECTORY,
		//
		//      All Users\Templates
		//
		CommonTemplates = CSIDL_COMMON_TEMPLATES,
		//
		//      All Users\My Video
		//
		CommonVideos = CSIDL_COMMON_VIDEO,
		//
		//      windows\fonts
		//
		Fonts = CSIDL_FONTS,
		//
		//      %APPDATA%\Microsoft\Windows\Network Shortcuts
		//
		NetworkShortcuts = CSIDL_NETHOOD,
		//
		//      %APPDATA%\Microsoft\Windows\Printer Shortcuts
		//
		PrinterShortcuts = CSIDL_PRINTHOOD,
		//
		//      USERPROFILE
		//
		UserProfile = CSIDL_PROFILE,
		//
		//      x86 Program Files\Common on RISC
		//
		CommonProgramFilesX86 = CSIDL_PROGRAM_FILES_COMMONX86,
		//
		//      x86 C:\Program Files on RISC
		//
		ProgramFilesX86 = CSIDL_PROGRAM_FILESX86,
		//
		//      Resource Directory
		//
		Resources = CSIDL_RESOURCES,
		//
		//      Localized Resource Directory
		//
		LocalizedResources = CSIDL_RESOURCES_LOCALIZED,
		//
		//      %windir%\System32 or %windir%\syswow64
		//
		SystemX86 = CSIDL_SYSTEMX86,
		//
		//      GetWindowsDirectory()
		//
		Windows = CSIDL_WINDOWS,
	};
}