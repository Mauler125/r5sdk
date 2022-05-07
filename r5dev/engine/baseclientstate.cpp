//===========================================================================//
//
// Purpose: Implementation of the CBaseClient class.
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
	return *cl_m_bPaused;
}

//------------------------------------------------------------------------------
// Purpose: gets the client time
// Technically doesn't belong here
//------------------------------------------------------------------------------
float CBaseClientState::GetClientTime()
{
    if (*cl_time_use_host_tickcount)
    {
        return (float)(int)*cl_host_tickcount * (float)*client_debugdraw_int_unk;
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
    return *cl_host_tickcount;
}

//------------------------------------------------------------------------------
// Purpose: sets the client simulation tick count
//------------------------------------------------------------------------------
void CBaseClientState::SetClientTickCount(int tick)
{
    *cl_host_tickcount = tick;
}

CBaseClientState* g_pBaseClientState = nullptr;
