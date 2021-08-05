#include "pch.h"
#include "hooks.h"

bool g_bBlockInput = false;

namespace Hooks
{
	bool bToggledDevFlags = true;
	bool bToggledNetTrace = false;
}

void Hooks::InstallHooks()
{
	///////////////////////////////////////////////////////////////////////////////
	// Initialize Minhook
	MH_Initialize();

	///////////////////////////////////////////////////////////////////////////////
	// Hook Squirrel functions
	MH_CreateHook(addr_SQVM_Print, &Hooks::SQVM_Print, NULL);
	MH_CreateHook(addr_SQVM_LoadRson, &Hooks::SQVM_LoadRson, reinterpret_cast<void**>(&originalSQVM_LoadRson));
	MH_CreateHook(addr_SQVM_LoadScript, &Hooks::SQVM_LoadScript, reinterpret_cast<void**>(&originalSQVM_LoadScript));

	///////////////////////////////////////////////////////////////////////////////
	// Hook Game Functions
	MH_CreateHook(addr_CHLClient_FrameStageNotify, &Hooks::FrameStageNotify, reinterpret_cast<void**>(&originalFrameStageNotify));
	MH_CreateHook(addr_CVEngineServer_IsPersistenceDataAvailable, &Hooks::IsPersistenceDataAvailable, reinterpret_cast<void**>(&originalIsPersistenceDataAvailable));

	///////////////////////////////////////////////////////////////////////////////
	// Hook Netchan functions
	MH_CreateHook(addr_NET_PrintFunc, &Hooks::NET_PrintFunc, reinterpret_cast<void**>(&originalNET_PrintFunc));
	MH_CreateHook(addr_NET_ReceiveDatagram, &Hooks::NET_ReceiveDatagram, reinterpret_cast<void**>(&originalNET_ReceiveDatagram));
	MH_CreateHook(addr_NET_SendDatagram, &Hooks::NET_SendDatagram, reinterpret_cast<void**>(&originalNET_SendDatagram));

	///////////////////////////////////////////////////////////////////////////////
	// Hook ConVar | ConCommand functions.
	MH_CreateHook(addr_ConVar_IsFlagSet, &Hooks::ConVar_IsFlagSet, NULL);
	MH_CreateHook(addr_ConCommand_IsFlagSet, &Hooks::ConCommand_IsFlagSet, NULL);

	///////////////////////////////////////////////////////////////////////////////
	// Hook Utility functions
	MH_CreateHook(addr_MSG_EngineError, &Hooks::MSG_EngineError, reinterpret_cast<void**>(&originalMSG_EngineError));

	///////////////////////////////////////////////////////////////////////////////
	// Hook WinAPI
	HMODULE user32dll = GetModuleHandleA("user32.dll");

	if (user32dll)
	{
		void* SetCursorPosPtr = GetProcAddress(user32dll, "SetCursorPos");
		void* ClipCursorPtr = GetProcAddress(user32dll, "ClipCursor");
		void* GetCursorPosPtr = GetProcAddress(user32dll, "GetCursorPos");
		void* ShowCursorPtr = GetProcAddress(user32dll, "ShowCursor");

		MH_CreateHook(SetCursorPosPtr, &Hooks::SetCursorPos, reinterpret_cast<void**>(&originalSetCursorPos));
		MH_CreateHook(ClipCursorPtr, &Hooks::ClipCursor, reinterpret_cast<void**>(&originalClipCursor));
		MH_CreateHook(GetCursorPosPtr, &Hooks::GetCursorPos, reinterpret_cast<void**>(&originalGetCursorPos));
		MH_CreateHook(ShowCursorPtr, &Hooks::ShowCursor, reinterpret_cast<void**>(&originalShowCursor));

		///////////////////////////////////////////////////////////////////////////
		// Enable WinAPI hooks
		MH_EnableHook(SetCursorPosPtr);
		MH_EnableHook(ClipCursorPtr);
		MH_EnableHook(GetCursorPosPtr);
		MH_EnableHook(ShowCursorPtr);
	}

	///////////////////////////////////////////////////////////////////////////////
	// Enable Squirrel hooks
	MH_EnableHook(addr_SQVM_Print);
	MH_EnableHook(addr_SQVM_LoadRson);
	MH_EnableHook(addr_SQVM_LoadScript);

	///////////////////////////////////////////////////////////////////////////////
	// Enable Game hooks
	MH_EnableHook(addr_CHLClient_FrameStageNotify);
	MH_EnableHook(addr_CVEngineServer_IsPersistenceDataAvailable);

	///////////////////////////////////////////////////////////////////////////////
	// Enable Netchan hooks
	MH_EnableHook(addr_NET_PrintFunc);

	///////////////////////////////////////////////////////////////////////////////
	// Enable ConVar | ConCommand hooks
	MH_EnableHook(addr_ConVar_IsFlagSet);
	MH_EnableHook(addr_ConCommand_IsFlagSet);

	///////////////////////////////////////////////////////////////////////////////
    // Enabled Utility hooks
	MH_EnableHook(addr_MSG_EngineError);
}

void Hooks::RemoveHooks()
{
	///////////////////////////////////////////////////////////////////////////////
	// Unhook Squirrel functions
	MH_RemoveHook(addr_SQVM_Print);
	MH_RemoveHook(addr_SQVM_LoadRson);
	MH_RemoveHook(addr_SQVM_LoadScript);

	///////////////////////////////////////////////////////////////////////////////
	// Unhook Game Functions
	MH_RemoveHook(addr_CHLClient_FrameStageNotify);
	MH_RemoveHook(addr_CVEngineServer_IsPersistenceDataAvailable);

	///////////////////////////////////////////////////////////////////////////////
	// Unhook Netchan functions
	MH_RemoveHook(addr_NET_PrintFunc);
	MH_RemoveHook(addr_NET_ReceiveDatagram);
	MH_RemoveHook(addr_NET_SendDatagram);

	///////////////////////////////////////////////////////////////////////////////
	// Unhook ConVar | ConCommand functions.
	MH_RemoveHook(addr_ConVar_IsFlagSet);
	MH_RemoveHook(addr_ConCommand_IsFlagSet);

	///////////////////////////////////////////////////////////////////////////////
	// Unhook WinAPI
	HMODULE user32dll = GetModuleHandleA("user32.dll");

	if (user32dll)
	{
		void* SetCursorPosPtr = GetProcAddress(user32dll, "SetCursorPos");
		void* ClipCursorPtr = GetProcAddress(user32dll, "ClipCursor");
		void* GetCursorPosPtr = GetProcAddress(user32dll, "GetCursorPos");
		void* ShowCursorPtr = GetProcAddress(user32dll, "ShowCursor");

		MH_RemoveHook(SetCursorPosPtr);
		MH_RemoveHook(ClipCursorPtr);
		MH_RemoveHook(GetCursorPosPtr);
		MH_RemoveHook(ShowCursorPtr);
	}

	///////////////////////////////////////////////////////////////////////////////
	// Unhook Utility functions
	MH_RemoveHook(addr_MSG_EngineError);

	///////////////////////////////////////////////////////////////////////////////
	// Reset Minhook
	MH_Uninitialize();
}

void Hooks::ToggleNetTrace()
{
	if (!bToggledNetTrace)
	{
		MH_EnableHook(addr_NET_ReceiveDatagram);
		MH_EnableHook(addr_NET_SendDatagram);
		printf("\n");
		printf("+--------------------------------------------------------+\n");
		printf("|>>>>>>>>>>>>>| NETCHANNEL TRACE ACTIVATED |<<<<<<<<<<<<<|\n");
		printf("+--------------------------------------------------------+\n");
		printf("\n");
	}
	else
	{
		MH_DisableHook(addr_NET_ReceiveDatagram);
		MH_DisableHook(addr_NET_SendDatagram);
		printf("\n");
		printf("+--------------------------------------------------------+\n");
		printf("|>>>>>>>>>>>>| NETCHANNEL TRACE DEACTIVATED |<<<<<<<<<<<<|\n");
		printf("+--------------------------------------------------------+\n");
		printf("\n");
	}
	bToggledNetTrace = !bToggledNetTrace;
}

void Hooks::ToggleDevCommands()
{
	if (!bToggledDevFlags)
	{
		MH_EnableHook(addr_ConVar_IsFlagSet);
		MH_EnableHook(addr_ConCommand_IsFlagSet);
		printf("\n");
		printf("+--------------------------------------------------------+\n");
		printf("|>>>>>>>>>>>>>| DEVONLY COMMANDS ACTIVATED |<<<<<<<<<<<<<|\n");
		printf("+--------------------------------------------------------+\n");
		printf("\n");
	}
	else
	{
		MH_DisableHook(addr_ConVar_IsFlagSet);
		MH_DisableHook(addr_ConCommand_IsFlagSet);
		printf("\n");
		printf("+--------------------------------------------------------+\n");
		printf("|>>>>>>>>>>>>| DEVONLY COMMANDS DEACTIVATED |<<<<<<<<<<<<|\n");
		printf("+--------------------------------------------------------+\n");
		printf("\n");
	}
	bToggledDevFlags = !bToggledDevFlags;
}
