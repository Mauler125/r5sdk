#include "core/stdafx.h"
#include "tier0/basetypes.h"
#include "engine/debugoverlay.h"

//-----------------------------------------------------------------------------
// Purpose: enables 'DrawAllOverlays()'
//-----------------------------------------------------------------------------
void DebugOverlays_Init()
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	p_DrawAllOverlays.Offset(0x189).Patch({ 0x83, 0x3F, 0x02 });  // Default value in memory is 0x2, condition is 0x4. Patch to fulfill condition.
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	p_DrawAllOverlays.Offset(0x188).Patch({ 0x83, 0x3F, 0x02 });  // Default value in memory is 0x2, condition is 0x4. Patch to fulfill condition.
#endif
}
