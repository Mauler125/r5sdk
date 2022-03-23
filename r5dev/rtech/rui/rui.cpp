//=============================================================================//
//
// Purpose: rUI Utilities
//
//=============================================================================//

#include "core/stdafx.h"

#ifndef DEDICATED

#include "rui.h"
#include <tier0/cvar.h>

//-----------------------------------------------------------------------------
// Purpose: Probably responsible to decide if rUI is allowed to draw.
//-----------------------------------------------------------------------------
bool __fastcall HRuiDraw(__int64* a1, __m128* a2, const __m128i* a3, __int64 a4, __m128* a5)
{
	if (!rui_drawEnable->GetBool())
		return false;

	return RuiDraw(a1, a2, a3, a4, a5);
}

void Rui_Attach()
{
	DetourAttach((LPVOID*)&RuiDraw, &HRuiDraw);
}

void Rui_Detach()
{
	DetourDetach((LPVOID*)&RuiDraw, &HRuiDraw);
}

#endif // !DEDICATED