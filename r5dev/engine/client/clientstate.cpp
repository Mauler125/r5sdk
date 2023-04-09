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
#include "engine/host.h"
#include "engine/client/clientstate.h"


//------------------------------------------------------------------------------
// Purpose: returns true if client simulation is paused
//------------------------------------------------------------------------------
bool CClientState::IsPaused() const
{
	return m_bPaused;
}

//------------------------------------------------------------------------------
// Purpose: returns true if client is fully connected and active
//------------------------------------------------------------------------------
bool CClientState::IsActive(void) const
{
    return m_nSignonState == SIGNONSTATE::SIGNONSTATE_FULL;
};

//------------------------------------------------------------------------------
// Purpose: returns true if client connected but not active
//------------------------------------------------------------------------------
bool CClientState::IsConnected(void) const
{
    return m_nSignonState >= SIGNONSTATE::SIGNONSTATE_CONNECTED;
};

//------------------------------------------------------------------------------
// Purpose: returns true if client is still connecting
//------------------------------------------------------------------------------
bool CClientState::IsConnecting(void) const
{
    return m_nSignonState >= SIGNONSTATE::SIGNONSTATE_NONE;
}

//------------------------------------------------------------------------------
// Purpose: gets the client time
// Technically doesn't belong here
//------------------------------------------------------------------------------
float CClientState::GetClientTime() const
{
    if (m_bClockCorrectionEnabled)
    {
        return (float)m_ClockDriftMgr.m_nSimulationTick * (*(float*)&interval_per_tick); // VERIFY DEREF
    }
    else
    {
        return m_flClockDriftFrameTime;
    }
}

//------------------------------------------------------------------------------
// Purpose: gets the simulation tick count
//------------------------------------------------------------------------------
int CClientState::GetTick() const
{
    return m_ClockDriftMgr.m_nSimulationTick;
}

//------------------------------------------------------------------------------
// Purpose: gets the last-received server tick count
//------------------------------------------------------------------------------
int CClientState::GetServerTickCount() const
{
    return m_ClockDriftMgr.m_nServerTick;
}

//------------------------------------------------------------------------------
// Purpose: sets the server tick count
//------------------------------------------------------------------------------
void CClientState::SetServerTickCount(int tick)
{
    m_ClockDriftMgr.m_nServerTick = tick;
}

//------------------------------------------------------------------------------
// Purpose: gets the client tick count
//------------------------------------------------------------------------------
int CClientState::GetClientTickCount() const
{
    return m_ClockDriftMgr.m_nClientTick;
}

//------------------------------------------------------------------------------
// Purpose: sets the client tick count
//------------------------------------------------------------------------------
void CClientState::SetClientTickCount(int tick)
{
    m_ClockDriftMgr.m_nClientTick = tick;
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
bool CClientState::VProcessServerTick(CClientState* pClientState, SVC_ServerTick* pServerTick)
{
    if (pServerTick->m_NetTick.m_nCommandTick != -1)
    {
        return CClientState__ProcessServerTick(pClientState, pServerTick);
    }
    else // Statistics only.
    {
        char* pShifted = reinterpret_cast<char*>(pClientState) - 0x10; // Shifted due to compiler optimizations.
        CClientState* pClient_Adj = reinterpret_cast<CClientState*>(pShifted);

        CNetChan* pChan = pClient_Adj->m_NetChannel;
        pChan->SetRemoteFramerate(pServerTick->m_NetTick.m_flHostFrameTime, pServerTick->m_NetTick.m_flHostFrameTimeStdDeviation);
        pChan->SetRemoteCPUStatistics(pServerTick->m_NetTick.m_nServerCPU);

        return true;
    }
}

void VClientState::Attach() const
{
    DetourAttach(&CClientState__ProcessServerTick, &CClientState::VProcessServerTick);
}

void VClientState::Detach() const
{
    DetourDetach(&CClientState__ProcessServerTick, &CClientState::VProcessServerTick);
}

/////////////////////////////////////////////////////////////////////////////////
CClientState* g_pClientState = nullptr;
CClientState** g_pClientState_Shifted = nullptr;
