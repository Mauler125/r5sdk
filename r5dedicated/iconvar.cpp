#include "pch.h"
#include "iconvar.h"

//-----------------------------------------------------------------------------
// Purpose: test each ConVar query before setting the cvar
// Input  : **cvar - flag
// Output : true if change is not permitted, false if permitted
//-----------------------------------------------------------------------------
bool HConVar_IsFlagSet(int** cvar, int flag)
{
	int real_flags = *(*(cvar + (72 / (sizeof(void*)))) + (56 / sizeof(int)));
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

void AttachIConVarHooks()
{
	DetourAttach((LPVOID*)&org_IConVar_IsFlagSet, &HConVar_IsFlagSet);
}

void DetachIConVarHooks()
{
	DetourDetach((LPVOID*)&org_IConVar_IsFlagSet, &HConVar_IsFlagSet);
}