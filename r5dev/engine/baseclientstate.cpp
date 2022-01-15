//===========================================================================//
//
// Purpose: 
//
//===========================================================================//

#include "core/stdafx.h"
#include "client/cdll_engine_int.h"
#include "engine/debugoverlay.h"
#include "engine/baseclientstate.h"


//------------------------------------------------------------------------------
// Purpose: returns true if client simulation is paused
//------------------------------------------------------------------------------
bool CBaseClientState::IsPaused()
{
	return *m_bPaused;
}

// Technically doesn't belong here.
//------------------------------------------------------------------------------
// Purpose: gets the client time
//------------------------------------------------------------------------------
float CBaseClientState::GetClientTime()
{
    if (*scr_drawloading)
    {
        return (float)(int)*host_tickcount * (float)*client_debugdraw_int_unk;
    }
    else
    {
        return *(float*)client_debugdraw_float_unk;
    }
}

//------------------------------------------------------------------------------
// Purpose: gets the client simulation tick count
//------------------------------------------------------------------------------
int CBaseClientState::GetClientTickCount() const
{
    return *host_tickcount;
}

//------------------------------------------------------------------------------
// Purpose: sets the client simulation tick count
//------------------------------------------------------------------------------
void CBaseClientState::SetClientTickCount(int tick)
{
    *host_tickcount = tick;
}

CBaseClientState* g_pBaseClientState = new CBaseClientState();
