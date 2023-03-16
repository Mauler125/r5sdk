//===== Copyright (c) Valve Corporation, All rights reserved. ========//
//
// Purpose:  
//
// $NoKeywords: $
//
//====================================================================//
#ifndef DBG_H
#define DBG_H
#define Assert assert
#define AssertDbg assert
#define Verify( _exp ) ( _exp )
#include "tier0/dbgflag.h"

bool HushAsserts();
//-----------------------------------------------------------------------------
enum class EGlobalContext_t : int
{
	GLOBAL_NONE = -4,
	SCRIPT_SERVER,
	SCRIPT_CLIENT,
	SCRIPT_UI,
	NATIVE_SERVER,
	NATIVE_CLIENT,
	NATIVE_UI,
	NATIVE_ENGINE,
	NATIVE_FS,
	NATIVE_RTECH,
	NATIVE_MS,
	NATIVE_AUDIO,
	NATIVE_VIDEO,
	NETCON_S,
	COMMON_C,
	WARNING_C,
	ERROR_C,
	NONE
};

enum class eDLL_T : int
{
	SERVER = 0, // server.dll                (GameDLL)
	CLIENT = 1, // client.dll                (GameDLL)
	UI     = 2, // ui.dll                    (GameDLL)
	ENGINE = 3, // engine.dll                (Wrapper)
	FS     = 4, // filesystem_stdio.dll      (FileSystem API)
	RTECH  = 5, // rtech_game.dll            (RTech API)
	MS     = 6, // materialsystem_dx11.dll   (MaterialSystem API)
	AUDIO  = 7, // binkawin64/mileswin64.dll (AudioSystem API)
	VIDEO  = 8, // bink2w64                  (VideoSystem API)
	NETCON = 9, // netconsole impl           (RCON wire)
	COMMON = 10 // general                   (No specific subsystem)
};

static const string sDLL_T[11] = 
{
	"Native(S):",
	"Native(C):",
	"Native(U):",
	"Native(E):",
	"Native(F):",
	"Native(R):",
	"Native(M):",
	"Native(A):",
	"Native(V):",
	"Netcon(X):",
	""
};

static const string sANSI_DLL_T[11] =
{
	"\033[38;2;059;120;218mNative(S):",
	"\033[38;2;118;118;118mNative(C):",
	"\033[38;2;151;090;118mNative(U):",
	"\033[38;2;204;204;204mNative(E):",
	"\033[38;2;097;214;214mNative(F):",
	"\033[38;2;092;181;089mNative(R):",
	"\033[38;2;192;077;173mNative(M):",
	"\033[38;2;238;108;030mNative(A):",
	"\033[38;2;185;000;235mNative(V):",
	"\033[38;2;204;204;204mNetcon(X):",
	"\033[38;2;255;204;153m"
};


static const std::regex rANSI_EXP("\\\033\\[.*?m");

extern std::mutex g_LogMutex;

//////////////////////////////////////////////////////////////////////////
// Legacy Logging System
//////////////////////////////////////////////////////////////////////////

// These functions do not return.
PLATFORM_INTERFACE void NetMsg(EGlobalContext_t context, const char* fmt, ...) FMTFUNCTION(2, 3);
PLATFORM_INTERFACE void DevMsg(eDLL_T context, const char* fmt, ...) FMTFUNCTION(2, 3);
PLATFORM_INTERFACE void Warning(eDLL_T context, const char* fmt, ...) FMTFUNCTION(2, 3);
PLATFORM_INTERFACE void Error(eDLL_T context, const UINT code, const char* fmt, ...) FMTFUNCTION(3, 4);

// You can use this macro like a runtime assert macro.
// If the condition fails, then Error is called with the message. This macro is called
// like AssertMsg, where msg must be enclosed in parenthesis:
//
// ErrorIfNot( bCondition, ("a b c %d %d %d", 1, 2, 3) );
#define ErrorIfNot( condition, msg ) \
	if ( (condition) )		\
		;					\
	else 					\
	{						\
		Error msg;			\
	}

//-----------------------------------------------------------------------------
// Templates to assist in validating pointers:

// Have to use these stubs so we don't have to include windows.h here.
PLATFORM_INTERFACE void _AssertValidReadPtr(void* ptr, int count = 1);
PLATFORM_INTERFACE void _AssertValidWritePtr(void* ptr, int count = 1);
PLATFORM_INTERFACE void _AssertValidReadWritePtr(void* ptr, int count = 1);
PLATFORM_INTERFACE void _AssertValidStringPtr(const TCHAR* ptr, int maxchar);

#ifdef DBGFLAG_ASSERT
inline void AssertValidStringPtr(const TCHAR* ptr, int maxchar = 0xFFFFFF) { _AssertValidStringPtr(ptr, maxchar); }
template<class T> inline void AssertValidReadPtr(T* ptr, int count = 1) { _AssertValidReadPtr((void*)ptr, count); }
template<class T> inline void AssertValidWritePtr(T* ptr, int count = 1) { _AssertValidWritePtr((void*)ptr, count); }
template<class T> inline void AssertValidReadWritePtr(T* ptr, int count = 1) { _AssertValidReadWritePtr((void*)ptr, count); }
#define AssertValidThis() AssertValidReadWritePtr(this,sizeof(*this))

#else

inline void AssertValidStringPtr(const TCHAR* ptr, int maxchar = 0xFFFFFF) {	}
template<class T> inline void AssertValidReadPtr(T* ptr, int count = 1) {  }
template<class T> inline void AssertValidWritePtr(T* ptr, int count = 1) {  }
template<class T> inline void AssertValidReadWritePtr(T* ptr, int count = 1) {  }
#define AssertValidThis() 
#endif
#endif /* DBG_H */
