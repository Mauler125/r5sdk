//===========================================================================//
//
// Purpose: Low-level tier0 interface.
//
//===========================================================================//
#ifndef TIER0_IFACE_H
#define TIER0_IFACE_H

// Module handles; user is responsible for initializing these.
extern CModule g_GameDll;
extern CModule g_SDKDll;

extern CModule g_RadVideoToolsDll;
extern CModule g_RadAudioDecoderDll;
extern CModule g_RadAudioSystemDll;

extern string g_LogSessionUUID;
extern string g_LogSessionDirectory;

#define VAR_NAME(varName)  #varName

#define MEMBER_AT_OFFSET(varType, varName, offset)             \
	varType& varName()                                         \
	{                                                          \
		static int _##varName = offset;                        \
		return *(varType*)((std::uintptr_t)this + _##varName); \
	}

template <typename ReturnType, typename ...Args>
ReturnType CallVFunc(int index, void* thisPtr, Args... args)
{
	return (*reinterpret_cast<ReturnType(__fastcall***)(void*, Args...)>(thisPtr))[index](thisPtr, args...);
}

void LogFunAdr(const char* szFun, uintptr_t nAdr); // Logging function addresses.
void LogVarAdr(const char* szVar, uintptr_t nAdr); // Logging variable addresses.
void LogConAdr(const char* szCon, uintptr_t nAdr); // Logging constant addresses.

#endif // TIER0_IFACE_H
