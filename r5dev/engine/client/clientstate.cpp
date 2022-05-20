//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// clientstate.cpp: implementation of the CClientState class.
//
/////////////////////////////////////////////////////////////////////////////////
#include "core/stdafx.h"
#include "client/cdll_engine_int.h"
#include "engine/debugoverlay.h"
#include "engine/client/clientstate.h"


//------------------------------------------------------------------------------
// Purpose: returns true if client simulation is paused
//------------------------------------------------------------------------------
bool CClientState::IsPaused()
{
	return m_bPaused;
}

//------------------------------------------------------------------------------
// Purpose: gets the client time
// Technically doesn't belong here
//------------------------------------------------------------------------------
float CClientState::GetClientTime()
{
    if (*cl_time_use_host_tickcount)
    {
        return (float)(int)m_ClockDriftMgr.m_nClientTick * (float)*client_debugdraw_int_unk;
    }
    else
    {
        return *(float*)client_debugdraw_float_unk;
    }
}

//------------------------------------------------------------------------------
// Purpose: gets the client simulation tick count
//------------------------------------------------------------------------------
int CClientState::GetClientTickCount() const
{
    return m_ClockDriftMgr.m_nClientTick;
}

//------------------------------------------------------------------------------
// Purpose: sets the client simulation tick count
//------------------------------------------------------------------------------
void CClientState::SetClientTickCount(int tick)
{
    m_ClockDriftMgr.m_nClientTick = tick;
}

/////////////////////////////////////////////////////////////////////////////////
CClientState* g_pClientState = nullptr;
