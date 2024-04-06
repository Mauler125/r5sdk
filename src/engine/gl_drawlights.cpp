#include "gl_drawlights.h"


void DrawLightSprites(void* unkType)
{
	v_DrawLightSprites(unkType);
}

void VGL_DrawLights::Detour(const bool bAttach) const
{
	// Enable if needed.
	//DetourSetup(&v_DrawLightSprites, &DrawLightSprites, bAttach);
}
