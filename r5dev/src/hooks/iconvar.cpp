#include "pch.h"
#include "hooks.h"

#ifdef _DEBUG
#define MaskOffCheats
#endif

namespace Hooks
{
	ConVar_IsFlagSetFn originalConVar_IsFlagSet = nullptr;
	ConCommand_IsFlagSetFn originalConCommand_IsFlagSet = nullptr;
}

bool Hooks::ConVar_IsFlagSet(ConVar* cvar, int flag)
{
#ifdef MaskOffCheats
	if (g_bDebugConsole)
	{
		std::cout << "--------------------------------------------------\n";
		std::cout << cvar->m_ConCommandBase.m_pszName << " Flags: " << std::hex << std::uppercase << cvar->m_ConCommandBase.m_nFlags << "\n";
	}
	// Mask off FCVAR_DEVELOPMENTONLY and FCVAR_CHEAT.
	cvar->m_ConCommandBase.m_nFlags &= ~(FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT);
	if (g_bDebugConsole)
	{
		std::cout << cvar->m_ConCommandBase.m_pszName << " Flags: " << std::hex << std::uppercase << cvar->m_ConCommandBase.m_nFlags << "\n";
		std::cout << cvar->m_ConCommandBase.m_pszName << " Verify: " << std::hex << std::uppercase << flag << "\n";
		std::cout << "--------------------------------------------------\n";
	}

	if (flag & FCVAR_RELEASE)
	{
		return true;
	}

	if (g_bReturnAllFalse)
	{
		return false;
	}
	
	return (cvar->m_ConCommandBase.m_nFlags & flag) != 0;
#else
	// Mask off FCVAR_DEVELOPMENTONLY if existing.
	cvar->m_ConCommandBase.m_nFlags &= ~FCVAR_DEVELOPMENTONLY;

	return originalConVar_IsFlagSet(cvar, flag);
#endif
}

bool Hooks::ConCommand_IsFlagSet(ConCommandBase* cmd, int flag)
{
#ifdef MaskOffCheats
	if (g_bDebugConsole)
	{
		std::cout << "--------------------------------------------------\n";
		std::cout << cmd->m_pszName << " Flags: " << std::hex << std::uppercase << cmd->m_nFlags << "\n";
	}
	// Mask off FCVAR_DEVELOPMENTONLY and FCVAR_CHEAT.
	cmd->m_nFlags &= ~(FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT);
	if (g_bDebugConsole)
	{
		std::cout << cmd->m_pszName << " Flags: " << std::hex << std::uppercase << cmd->m_nFlags << "\n";
		std::cout << cmd->m_pszName << " Verify: " << std::hex << std::uppercase << flag << "\n";
		std::cout << "--------------------------------------------------\n";
	}

	if (flag & FCVAR_RELEASE)
	{
		return true;
	}

	if (g_bReturnAllFalse)
	{
		return false;
	}

	return (cmd->m_nFlags & flag) != 0;
#else
	// Mask off FCVAR_DEVELOPMENTONLY if existing.
	cmd->m_nFlags &= ~FCVAR_DEVELOPMENTONLY;

	return originalConCommand_IsFlagSet(cmd, flag);
#endif
}