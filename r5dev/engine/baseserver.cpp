//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// baseserver.cpp: implementation of the CBaseServer class.
//
/////////////////////////////////////////////////////////////////////////////////
#include "core/stdafx.h"
#include "common/protocol.h"
#include "engine/baseserver.h"
#include "engine/baseclient.h"

//---------------------------------------------------------------------------------
// Purpose: Gets the number of human players on the server
// Output : 
// !TODO  : Rebuild properly..
//---------------------------------------------------------------------------------
int64_t CBaseServer::GetNumHumanPlayers(void) const
{
    uint32_t nHumans = 0;
    if (SHIDWORD(*g_dwMaxClients) > 0)
    {
        bool v13 = false;
        uint32_t v14 = 0;
        int32_t* v11 = reinterpret_cast<int*>(&*m_Clients); // CUtlVector<CBaseClient*> m_Clients.
        int64_t nHumanCount = HIDWORD(*g_dwMaxClients);
        do
        {
            if (*(v11 - 124) >= static_cast<int>(SIGNONSTATE::SIGNONSTATE_CONNECTED)) // m_Client[i]->IsConnected().
                v13 = *v11 == 0;
            else
                v13 = 0;
            v14 = nHumans + 1;
            if (!v13)
                v14 = nHumans;
            v11 += 0x12930;
            nHumans = v14;
            --nHumanCount;
        }     while (nHumanCount);
    }
    return nHumans;
}

//---------------------------------------------------------------------------------
// Purpose: Gets the number of fake clients on the server
// Output : 
//---------------------------------------------------------------------------------
int64_t CBaseServer::GetNumFakeClients(void) const
{
    // !TODO: Needs partial CBaseClient class rebuild.
    return NULL;
}

CBaseServer* g_pServer = new CBaseServer(); // !TODO: Replace with engine global if found.
