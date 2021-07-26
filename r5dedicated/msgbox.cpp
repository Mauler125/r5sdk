#include "pch.h"
#include "hooks.h"

//-----------------------------------------------------------------------------
// Engine Error message box
//-----------------------------------------------------------------------------
int HMSG_EngineError(char* fmt, va_list args)
{
	printf("\nENGINE ERROR #####################################\n");
	vprintf(fmt, args);

	///////////////////////////////////////////////////////////////////////////
	return org_MSG_EngineError(fmt, args);
}

void AttachMSGBoxHooks()
{
	DetourAttach((LPVOID*)&org_MSG_EngineError, &HMSG_EngineError);
}

void DetachMSGBoxHooks()
{
	DetourDetach((LPVOID*)&org_MSG_EngineError, &HMSG_EngineError);
}