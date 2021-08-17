#pragma once
#include "patterns.h"
#include "structs.h"
#include "hooks.h"
#include "gameclasses.h"
#include "CCompanion.h"
#include "CGameConsole.h"

inline bool g_bDebugLoading = false;
inline bool g_bReturnAllFalse = false;
inline bool g_bDebugConsole = false;
extern bool g_bBlockInput;

namespace Hooks
{
#pragma region CHLClient
	void __fastcall FrameStageNotify(CHLClient* rcx, ClientFrameStage_t curStage);

	using FrameStageNotifyFn = void(__fastcall*)(CHLClient*, ClientFrameStage_t);
	extern FrameStageNotifyFn originalFrameStageNotify;
#pragma endregion

#pragma region Squirrel
	void* SQVM_Print(void* sqvm, char* fmt, ...);
	__int64 SQVM_Warning(void* sqvm, int a2, int a3, int* stringSize, void** string);
	__int64 SQVM_LoadRson(const char* rson_name);
	bool SQVM_LoadScript(void* sqvm, const char* script_path, const char* script_name, int flag);

	using SQVM_WarningFn = __int64(*)(void*, int, int, int*, void**);
	extern SQVM_WarningFn originalSQVM_Warning;

	using SQVM_LoadRsonFn = __int64(*)(const char*);
	extern SQVM_LoadRsonFn originalSQVM_LoadRson;

	using SQVM_LoadScriptFn = bool(*)(void*, const char*, const char*, int);
	extern SQVM_LoadScriptFn originalSQVM_LoadScript;
#pragma endregion

#pragma region CVEngineServer
	bool IsPersistenceDataAvailable(__int64 thisptr, int client);

	using IsPersistenceDataAvailableFn = bool(*)(__int64, int);
	extern IsPersistenceDataAvailableFn originalIsPersistenceDataAvailable;
#pragma endregion

#pragma region NetChannel
	bool NET_ReceiveDatagram(int sock, void* inpacket, bool raw);
	unsigned int NET_SendDatagram(SOCKET s, const char* buf, int len, int flags);
	void NET_PrintFunc(const char* fmt, ...);
	void NetChanShutdown(void* rcx, const char* reason, unsigned __int8 unk1, char unk2);

	using NET_PrintFuncFn = void(*)(const char* fmt, ...);
	extern NET_PrintFuncFn originalNET_PrintFunc;

	using NET_ReceiveDatagramFn = bool(*)(int, void*, bool);
	extern NET_ReceiveDatagramFn originalNET_ReceiveDatagram;

	using NET_SendDatagramFn = unsigned int(*)(SOCKET, const char*, int, int);
	extern NET_SendDatagramFn originalNET_SendDatagram;
	using NetChan_ShutDown = void(*)(void*, const char*, unsigned __int8, char);
	extern NetChan_ShutDown originalNetChanShutDown;
#pragma endregion

#pragma region ConVar
	bool ConVar_IsFlagSet(ConVar* cvar, int flag);
	bool ConCommand_IsFlagSet(ConCommandBase* cmd, int flag);

	using ConVar_IsFlagSetFn = bool(*)(ConVar*, int);
	extern ConVar_IsFlagSetFn originalConVar_IsFlagSet;

	using ConCommand_IsFlagSetFn = bool(*)(ConCommandBase*, int);
	extern ConCommand_IsFlagSetFn originalConCommand_IsFlagSet;

	using Map_CallbackFn = void(*)(void*);
	extern Map_CallbackFn originalMap_Callback;
#pragma endregion

#pragma region WinAPI
	BOOL WINAPI GetCursorPos(LPPOINT lpPoint);
	BOOL WINAPI SetCursorPos(int X, int Y);
	BOOL WINAPI ClipCursor(const RECT* lpRect);
	BOOL WINAPI ShowCursor(BOOL bShow);

	using GetCursorPosFn = BOOL(WINAPI*)(LPPOINT);
	extern GetCursorPosFn originalGetCursorPos;

	using SetCursorPosFn = BOOL(WINAPI*)(int, int);
	extern SetCursorPosFn originalSetCursorPos;

	using ClipCursorFn = BOOL(WINAPI*)(const RECT*);
	extern ClipCursorFn originalClipCursor;

	using ShowCursorFn = BOOL(WINAPI*)(BOOL);
	extern ShowCursorFn originalShowCursor;
#pragma endregion

#pragma region CBaseFileSystem
	void FileSystemWarning(void* thisptr, FileWarningLevel_t level, const char* fmt, ...);

	using FileSystemWarningFn = void(*)(void*, FileWarningLevel_t, const char*, ...);
	extern FileSystemWarningFn originalFileSystemWarning;
#pragma endregion

#pragma region Other
	int MSG_EngineError(char* fmt, va_list args);

	using MSG_EngineErrorFn = int(*)(char*, va_list);
	extern MSG_EngineErrorFn originalMSG_EngineError;
#pragma endregion

	void InstallHooks();
	void RemoveHooks();
	void ToggleNetTrace();
	extern bool bToggledNetTrace;
	void ToggleDevCommands();
	extern bool bToggledDevFlags;
}