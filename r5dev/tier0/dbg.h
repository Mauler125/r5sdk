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
#include "tier0/dbgflag.h"

bool HushAsserts();
//-----------------------------------------------------------------------------
enum class eDLL_T : int
{
	SERVER = 0, // Game DLL
	CLIENT = 1, // Game DLL
	UI     = 2, // Game DLL
	ENGINE = 3, // Wrapper
	FS     = 4, // File System
	RTECH  = 5, // RTech API
	MS     = 6, // Material System
	NETCON = 7, // Net Console
	COMMON = 8
};

const string sDLL_T[9] = 
{
	"Native(S):",
	"Native(C):",
	"Native(U):",
	"Native(E):",
	"Native(F):",
	"Native(R):",
	"Native(M):",
	"Netcon(X):",
	""
};

const static string sANSI_DLL_T[9] = 
{
	"\033[38;2;059;120;218mNative(S):",
	"\033[38;2;118;118;118mNative(C):",
	"\033[38;2;151;090;118mNative(U):",
	"\033[38;2;204;204;204mNative(E):",
	"\033[38;2;097;214;214mNative(F):",
	"\033[38;2;092;181;089mNative(R):",
	"\033[38;2;192;105;173mNative(M):",
	"\033[38;2;204;204;204mNetcon(X):",
	"\033[38;2;255;204;153m"
};
extern std::mutex s_LogMutex;

//////////////////////////////////////////////////////////////////////////
// Legacy Logging System
//////////////////////////////////////////////////////////////////////////

// These functions do not return.
PLATFORM_INTERFACE void NetMsg(int context, const char* fmt, ...) FMTFUNCTION(2, 3);
PLATFORM_INTERFACE void DevMsg(eDLL_T context, const char* fmt, ...) FMTFUNCTION(2, 3);
PLATFORM_INTERFACE void Warning(eDLL_T context, const char* fmt, ...) FMTFUNCTION(1, 2);
PLATFORM_INTERFACE void Error(eDLL_T context, const char* fmt, ...) FMTFUNCTION(1, 2);

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
