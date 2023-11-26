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
#include "vpc/keyvalues.h"
#include "tier0/frametask.h"
#include "engine/common.h"
#include "engine/host.h"
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#endif // !CLIENT_DLL
#include "clientstate.h"
#include "common/callback.h"
#include "cdll_engine_int.h"
#include "vgui/vgui_baseui_interface.h"
#include <ebisusdk/EbisuSDK.h>
#include <engine/cmd.h>


//------------------------------------------------------------------------------
// Purpose: returns true if client simulation is paused
//------------------------------------------------------------------------------
bool CClientState::IsPaused() const
{
	return m_bPaused || !*host_initialized || g_pEngineVGui->ShouldPause();
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
        return (float)m_ClockDriftMgr.m_nSimulationTick * g_pCommonHostState->interval_per_tick;
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
// Purpose: gets the client frame time
//------------------------------------------------------------------------------
float CClientState::GetFrameTime() const
{
    if (IsPaused())
    {
        return 0.0f;
    }

    return m_flFrameTime;
}

//------------------------------------------------------------------------------
// Purpose: called when connection to the server has been closed
//------------------------------------------------------------------------------
void CClientState::VConnectionClosing(CClientState* thisptr, const char* szReason)
{
    CClientState__ConnectionClosing(thisptr, szReason);

    // Delay execution to the next frame; this is required to avoid a rare crash.
    // Cannot reload playlists while still disconnecting.
    g_TaskScheduler->Dispatch([]()
        {
            // Reload the local playlist to override the cached
            // one from the server we got disconnected from.
            _DownloadPlaylists_f();
            KeyValues::InitPlaylists();
        }, 0);
}

//------------------------------------------------------------------------------
// Purpose: called when a SVC_ServerTick messages comes in.
// This function has an additional check for the command tick against '-1',
// if it is '-1', we process statistics only. This is required as the game
// no longer can process server ticks every frame unlike previous games.
// Without this, the server CPU and frame time don't get updated to the client.
//------------------------------------------------------------------------------
bool CClientState::VProcessServerTick(CClientState* thisptr, SVC_ServerTick* msg)
{
    if (msg->m_NetTick.m_nCommandTick != -1)
    {
        return CClientState__ProcessServerTick(thisptr, msg);
    }
    else // Statistics only.
    {
        CClientState* const thisptr_ADJ = thisptr->GetShiftedBasePointer();

        CNetChan* const pChan = thisptr_ADJ->m_NetChannel;
        pChan->SetRemoteFramerate(msg->m_NetTick.m_flHostFrameTime, msg->m_NetTick.m_flHostFrameTimeStdDeviation);
        pChan->SetRemoteCPUStatistics(msg->m_NetTick.m_nServerCPU);

        return true;
    }
}

bool CClientState::_ProcessStringCmd(CClientState* thisptr, NET_StringCmd* msg)
{
    CClientState* const thisptr_ADJ = thisptr->GetShiftedBasePointer();

    if (thisptr_ADJ->m_bRestrictServerCommands
#ifndef CLIENT_DLL
        && !g_pServer->IsActive()
#endif // !CLIENT_DLL
        )
    {
        CCommand args;
        args.Tokenize(msg->cmd, cmd_source_t::kCommandSrcInvalid);

        if (args.ArgC() > 0)
        {
            if (!Cbuf_AddTextWithMarkers(msg->cmd,
                eCmdExecutionMarker_Enable_FCVAR_SERVER_CAN_EXECUTE,
                eCmdExecutionMarker_Disable_FCVAR_SERVER_CAN_EXECUTE))
            {
                DevWarning(eDLL_T::CLIENT, "%s: No room for %i execution markers; command \"%s\" ignored\n",
                    __FUNCTION__, 2, msg->cmd);
            }

            return true;
        }
    }
    else
    {
        Cbuf_AddText(Cbuf_GetCurrentPlayer(), msg->cmd, cmd_source_t::kCommandSrcCode);
    }

    return true;
}

//------------------------------------------------------------------------------
// Purpose: get authentication token for current connection context
// Input  : *connectParams - 
//          *reasonBuf     - 
//          reasonBufLen   - 
// Output : true on success, false otherwise
//------------------------------------------------------------------------------
bool CClientState::Authenticate(connectparams_t* connectParams, char* const reasonBuf, const size_t reasonBufLen) const
{
#define FORMAT_ERROR_REASON(fmt, ...) V_snprintf(reasonBuf, reasonBufLen, fmt, ##__VA_ARGS__);

    string msToken; // token returned by the masterserver authorising the client to play online
    string message; // message returned by the masterserver about the result of the auth

    // verify that the client is not lying about their account identity
    // code is immediately discarded upon verification

    const bool ret = g_pMasterServer->AuthForConnection(*g_NucleusID, connectParams->netAdr, g_OriginAuthCode, msToken, message);
    if (!ret)
    {
        FORMAT_ERROR_REASON("%s", message.c_str());
        return false;
    }

    // get full token
    const char* token = msToken.c_str();

    // get a pointer to the delimiter that begins the token's signature
    const char* tokenSignatureDelim = strrchr(token, '.');

    if (!tokenSignatureDelim)
    {
        FORMAT_ERROR_REASON("Invalid token returned by MS");
        return false;
    }

    // replace the delimiter with a null char so the first cvar only takes the header and payload data
    *(char*)tokenSignatureDelim = '\0';
    const size_t sigLength = strlen(tokenSignatureDelim) - 1;

    cl_onlineAuthToken->SetValue(token);

    if (sigLength > 0)
    {
        // get a pointer to the first part of the token signature to store in cl_onlineAuthTokenSignature1
        const char* tokenSignaturePart1 = tokenSignatureDelim + 1;

        cl_onlineAuthTokenSignature1->SetValue(tokenSignaturePart1);

        if (sigLength > 255)
        {
            // get a pointer to the rest of the token signature to store in cl_onlineAuthTokenSignature2
            const char* tokenSignaturePart2 = tokenSignaturePart1 + 255;

            cl_onlineAuthTokenSignature2->SetValue(tokenSignaturePart2);
        }
    }

    return true;
#undef REJECT_CONNECTION
}

bool IsLocalHost(connectparams_t* connectParams)
{
    return (strstr(connectParams->netAdr, "localhost") || strstr(connectParams->netAdr, "127.0.0.1"));
}

void CClientState::VConnect(CClientState* thisptr, connectparams_t* connectParams)
{
    if (cl_onlineAuthEnable->GetBool() && !IsLocalHost(connectParams))
    {
        char authFailReason[512];

        if (!thisptr->Authenticate(connectParams, authFailReason, sizeof(authFailReason)))
        {
            COM_ExplainDisconnection(true, "Failed to authenticate for online play: %s", authFailReason);
            return;
        }
    }

    CClientState__Connect(thisptr, connectParams);
}

void VClientState::Detour(const bool bAttach) const
{
    DetourSetup(&CClientState__ConnectionClosing, &CClientState::VConnectionClosing, bAttach);
    DetourSetup(&CClientState__ProcessStringCmd, &CClientState::_ProcessStringCmd, bAttach);
    DetourSetup(&CClientState__ProcessServerTick, &CClientState::VProcessServerTick, bAttach);
    DetourSetup(&CClientState__Connect, &CClientState::VConnect, bAttach);
}

/////////////////////////////////////////////////////////////////////////////////
CClientState* g_pClientState = nullptr;
CClientState** g_pClientState_Shifted = nullptr;
