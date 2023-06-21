//===========================================================================//
// 
// Purpose: server side datablock sender
// 
//===========================================================================//
#include "engine/client/client.h"
#include "datablock_sender.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ServerDataBlockSender::~ServerDataBlockSender()
{
	v_ServerDataBlockSender__Destructor(this);
}

//-----------------------------------------------------------------------------
// Purpose: sends the datablock
//-----------------------------------------------------------------------------
void ServerDataBlockSender::SendDataBlock(short unk0, int unk1,
    short unk2, short unk3, const void* buffer, int length)
{
	v_ServerDataBlockSender__SendDataBlock(this, unk0, unk1,
        unk2, unk3, buffer, length);
}

//-----------------------------------------------------------------------------
// Purpose: gets the resend rate
//-----------------------------------------------------------------------------
float ServerDataBlockSender::GetResendRate() const
{
    float flRet = 0.0f;

    if (!m_pClient)
        return flRet;

    CNetChan* pChan = m_pClient->GetNetChan();
    if (!pChan)
        return flRet;

    if (!m_bStartedTransfer)
    {
        flRet = pChan->GetNetworkLoss();

        if (flRet < net_datablock_networkLossForSlowSpeed->GetFloat())
        {
            return m_flResendRate;
        }
    }

    return flRet;
}

//-----------------------------------------------------------------------------
// Purpose: gets the receiver name (client name as registered on the server)
//-----------------------------------------------------------------------------
const char* ServerDataBlockSender::GetReceiverName() const
{
    return m_pClient->m_szServerName;
}
