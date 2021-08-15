#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	ConVar_IsFlagSetFn originalConVar_IsFlagSet = nullptr;
	ConCommand_IsFlagSetFn originalConCommand_IsFlagSet = nullptr;
	Map_CallbackFn originalMap_Callback = nullptr;
}

bool Hooks::ConVar_IsFlagSet(ConVar* cvar, int flag)
{
#ifdef _DEBUG
	if (g_bDebugConsole)
	{
		printf("--------------------------------------------------\n");
		printf(" Flaged: %08X\n", cvar->m_ConCommandBase.m_nFlags);
	}
	// Mask off FCVAR_CHEATS and FCVAR_DEVELOPMENTONLY
	cvar->m_ConCommandBase.m_nFlags &= 0xFFFFBFFD;
	if (g_bDebugConsole)
	{
		printf(" Masked: %08X\n", cvar->m_ConCommandBase.m_nFlags);
		printf(" Verify: %08X\n", flag);
		printf("--------------------------------------------------\n");
	}

	if (flag & FCVAR_RELEASE)
	{
		return true;
	}

	if (!g_bReturnAllFalse)
	{
		return (cvar->m_ConCommandBase.m_nFlags & flag) != 0;
	}
	else
	{
		return false;
	}
#else
	// Mask off FCVAR_DEVELOPMENTONLY if existing.
	cvar->m_ConCommandBase.m_nFlags &= ~FCVAR_DEVELOPMENTONLY;

	return originalConVar_IsFlagSet(cvar, flag);
#endif
}

bool Hooks::ConCommand_IsFlagSet(ConCommandBase* cmd, int flag)
{
#ifdef _DEBUG
	if (g_bDebugConsole)
	{
		printf("--------------------------------------------------\n");
		printf(" Flaged: %08X\n", cmd->m_nFlags);
	}
	// Mask off FCVAR_CHEATS and FCVAR_DEVELOPMENTONLY
	cmd->m_nFlags &= 0xFFFFBFFD;
	if (g_bDebugConsole)
	{
		printf(" Masked: %08X\n", cmd->m_nFlags);
		printf(" Verify: %08X\n", flag);
		printf("--------------------------------------------------\n");
	}

	if (flag & FCVAR_RELEASE)
	{
		return true;
	}

	if (!g_bReturnAllFalse)
	{
		return (cmd->m_nFlags & flag) != 0;
	}
	else
	{
		return false;
	}
#else
	// Mask off FCVAR_DEVELOPMENTONLY if existing.
	cmd->m_nFlags &= ~FCVAR_DEVELOPMENTONLY;

	return originalConCommand_IsFlagSet(cmd, flag);
#endif
}