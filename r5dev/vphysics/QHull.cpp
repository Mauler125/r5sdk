#include "core/stdafx.h"
#include "vphysics/QHull.h"

//-----------------------------------------------------------------------------
// Purpose: qhull error and debug prints
//-----------------------------------------------------------------------------
int HQHull_PrintError(char* fmt, va_list args)
{
	vprintf(fmt, args);
	return QHull_PrintError(fmt, args);
}

int HQHull_PrintDebug(char* fmt, va_list args)
{
	vprintf(fmt, args);
	return QHull_PrintDebug(fmt, args);
}
void QHull_Attach()
{
	DetourAttach((LPVOID*)&QHull_PrintDebug, &HQHull_PrintDebug);
	DetourAttach((LPVOID*)&QHull_PrintError, &HQHull_PrintError);
}

void QHull_Detach()
{
	DetourDetach((LPVOID*)&QHull_PrintDebug, &HQHull_PrintDebug);
	DetourDetach((LPVOID*)&QHull_PrintError, &HQHull_PrintError);
}
