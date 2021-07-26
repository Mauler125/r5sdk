#include "pch.h"
#include "concommand.h"

//-----------------------------------------------------------------------------
// Purpose: test each ConCommand query before execution
// Input  : *cmd - flag
// Output : true if execution is not permitted, false if permitted
//-----------------------------------------------------------------------------
bool HConCommand_IsFlagSet(int* cmd, int flag)
{
	int real_flags = *((cmd + (56 / sizeof(int))));
	if (g_bDebugConsole)
	{
		printf("--------------------------------------------------\n");
		printf(" Flaged: %08X\n", real_flags);
	}
	// Mask off FCVAR_CHEATS and FCVAR_DEVELOPMENTONLY
	real_flags &= 0xFFFFBFFD;
	if (g_bDebugConsole)
	{
		printf(" Masked: %08X\n", real_flags);
		printf(" Verify: %08X\n", flag);
		printf("--------------------------------------------------\n");
	}
	if (flag & 0x80000) { return true; }

	if (!g_bReturnAllFalse) { return (real_flags & flag) != 0; }
	else { return false; } // Returning false on all queries may cause problems
}

void AttachConCommandHooks()
{
	DetourAttach((LPVOID*)&org_ConCommand_IsFlagSet, &HConCommand_IsFlagSet);
}

void DetachConCommandHooks()
{
	DetourDetach((LPVOID*)&org_ConCommand_IsFlagSet, &HConCommand_IsFlagSet);
}