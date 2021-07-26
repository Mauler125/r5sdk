#include "pch.h"
#include "hooks.h"
#include "iconvar.h"
#include "concommand.h"
#include "cvengineserver.h"
#include "cnetchan.h"
#include "sqvm.h"
#include "msgbox.h"
#include "opcodes.h"

//#################################################################################
// MANAGEMENT
//#################################################################################

void InstallHooks()
{
	///////////////////////////////////////////////////////////////////////////////
	// Begin the detour transaction, to hook the the process
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	///////////////////////////////////////////////////////////////////////////////
	// Hook functions
	AttachIConVarHooks();
	AttachConCommandHooks();
	AttachCEngineServerHooks();
	AttachSQVMHooks();
	AttachMSGBoxHooks();

	///////////////////////////////////////////////////////////////////////////////
	// Commit the transaction
	if (DetourTransactionCommit() != NO_ERROR)
	{
		// Failed to hook into the process, terminate
		TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);
	}

	InstallOpcodes();
}

void RemoveHooks()
{
	///////////////////////////////////////////////////////////////////////////////
	// Begin the detour transaction, to unhook the the process
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	///////////////////////////////////////////////////////////////////////////////
	// Unhook functions
	DetachIConVarHooks();
	DetachConCommandHooks();
	DetachCEngineServerHooks();
	DetachSQVMHooks();
	DetachMSGBoxHooks();

	///////////////////////////////////////////////////////////////////////////////
	// Commit the transaction
	DetourTransactionCommit();
}

//#################################################################################
// TOGGLES
//#################################################################################

void ToggleDevCommands()
{
	static bool bDev = true;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	if (!bDev)
	{
		AttachIConVarHooks();
		AttachConCommandHooks();
		printf("\n");
		printf("+--------------------------------------------------------+\n");
		printf("|>>>>>>>>>>>>>| DEVONLY COMMANDS ACTIVATED |<<<<<<<<<<<<<|\n");
		printf("+--------------------------------------------------------+\n");
		printf("\n");

	}
	else
	{
		DetachIConVarHooks();
		DetachConCommandHooks();
		printf("\n");
		printf("+--------------------------------------------------------+\n");
		printf("|>>>>>>>>>>>>| DEVONLY COMMANDS DEACTIVATED |<<<<<<<<<<<<|\n");
		printf("+--------------------------------------------------------+\n");
		printf("\n");
	}

	if (DetourTransactionCommit() != NO_ERROR)
	{
		TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);
	}

	bDev = !bDev;
}

void ToggleNetTrace()
{
	static bool bNet = true;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	if (!bNet)
	{
		AttachCNetChanHooks();
		printf("\n");
		printf("+--------------------------------------------------------+\n");
		printf("|>>>>>>>>>>>>>| NETCHANNEL TRACE ACTIVATED |<<<<<<<<<<<<<|\n");
		printf("+--------------------------------------------------------+\n");
		printf("\n");
	}
	else
	{
		DetachCNetChanHooks();
		printf("\n");
		printf("+--------------------------------------------------------+\n");
		printf("|>>>>>>>>>>>>| NETCHANNEL TRACE DEACTIVATED |<<<<<<<<<<<<|\n");
		printf("+--------------------------------------------------------+\n");
		printf("\n");
	}

	if (DetourTransactionCommit() != NO_ERROR)
	{
		TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);
	}

	bNet = !bNet;
}
