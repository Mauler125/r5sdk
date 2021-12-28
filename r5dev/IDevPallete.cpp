#include "core/stdafx.h"

#ifndef DEDICATED
#include "IDevPalette.h"



//#############################################################################
// ENTRYPOINT
//#############################################################################

CDevPalette* g_DevPalette = nullptr;

void DrawDevPalette(bool* bDraw)
{
    static CDevPalette devPalette;
    static bool AssignPtr = []()
    {
        g_DevPalette = &devPalette;
        return true;
    } ();
    if (*bDraw) devPalette.Draw(bDraw);
}

///////////////////////////////////////////////////////////////////////////
#endif