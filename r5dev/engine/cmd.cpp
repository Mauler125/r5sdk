#include "core/stdafx.h"
#include "tier1/cmd.h"
#include "tier1/cvar.h"
#include "engine/cmd.h"

//-----------------------------------------------------------------------------
// Purpose: Sends the entire command line over to the server
// Input  : *args - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool Cmd_ForwardToServer(const CCommand* args)
{
#ifndef DEDICATED
	// Client -> Server command throttling.
	static double flForwardedCommandQuotaStartTime = -1;
	static int nForwardedCommandQuotaCount = 0;

	// No command to forward.
	if (args->ArgC() == 0)
		return false;

	double flStartTime = Plat_FloatTime();
	int nCmdQuotaLimit = cl_quota_stringCmdsPerSecond->GetInt();
	const char* pszCmdString = nullptr;

	// Special case: "cmd whatever args..." is forwarded as "whatever args...";
	// in this case we strip "cmd" from the input.
	if (Q_strcasecmp(args->Arg(0), "cmd") == 0)
		pszCmdString = args->ArgS();
	else
		pszCmdString = args->GetCommandString();

	if (nCmdQuotaLimit)
	{
		if (flStartTime - flForwardedCommandQuotaStartTime >= 1.0)
		{
			flForwardedCommandQuotaStartTime = flStartTime;
			nForwardedCommandQuotaCount = 0;
		}
		++nForwardedCommandQuotaCount;

		if (nForwardedCommandQuotaCount > nCmdQuotaLimit)
		{
			// If we are over quota commands per second, dump this on the floor.
			// If we spam the server with too many commands, it will kick us.
			Warning(eDLL_T::CLIENT, "Command '%s' ignored (submission quota of '%d' per second exceeded!)\n", pszCmdString, nCmdQuotaLimit);
			return false;
		}
	}
	return v_Cmd_ForwardToServer(args);
#else // !DEDICATED
	return false; // Client only.
#endif // DEDICATED
}

///////////////////////////////////////////////////////////////////////////////
void VCmd::Attach() const
{
	DetourAttach((LPVOID*)&v_Cmd_ForwardToServer, &Cmd_ForwardToServer);
}
void VCmd::Detach() const
{
	DetourDetach((LPVOID*)&v_Cmd_ForwardToServer, &Cmd_ForwardToServer);
}
