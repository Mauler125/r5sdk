//=============================================================================//
//
// Purpose: Netchannel system utilities
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/frametask.h"
#include "tier1/cvar.h"
#include "vpc/keyvalues.h"
#include "common/callback.h"
#include "engine/net.h"
#include "engine/net_chan.h"
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#include "engine/client/client.h"
#include "server/vengineserver_impl.h"
#endif // !CLIENT_DLL


//-----------------------------------------------------------------------------
// Purpose: gets the netchannel network loss
// Output : float
//-----------------------------------------------------------------------------
float CNetChan::GetNetworkLoss() const
{
	int64_t totalupdates = this->m_DataFlow[FLOW_INCOMING].totalupdates;
	if (!totalupdates && !this->m_nSequencesSkipped_MAYBE)
		return 0.0f;

	float loss = (float)(totalupdates + m_nSequencesSkipped_MAYBE);
	if (totalupdates + m_nSequencesSkipped_MAYBE < 0.0f)
		loss += float(2 ^ 64);

	return m_nSequencesSkipped_MAYBE / loss;
}

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel sequence number
// Input  : flow - 
// Output : int
//-----------------------------------------------------------------------------
int CNetChan::GetSequenceNr(int flow) const
{
	if (flow == FLOW_OUTGOING)
	{
		return m_nOutSequenceNr;
	}
	else if (flow == FLOW_INCOMING)
	{
		return m_nInSequenceNr;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel connect time
// Output : double
//-----------------------------------------------------------------------------
double CNetChan::GetTimeConnected(void) const
{
	double t = *g_pNetTime - connect_time;
	return (t > 0.0) ? t : 0.0;
}

//-----------------------------------------------------------------------------
// Purpose: flows a new packet
// Input  : *pChan   - 
//          outSeqNr - 
//          acknr    - 
//          inSeqNr  - 
//          nChoked  - 
//          nDropped - 
//          nSize    - 
//-----------------------------------------------------------------------------
void CNetChan::_FlowNewPacket(CNetChan* pChan, int flow, int outSeqNr, int inSeqNr, int nChoked, int nDropped, int nSize)
{
    float v7; // xmm4_8 (was double)
    int v8; // r13d
    int v9; // r14d
    int v12; // r12d
    int currentindex; // eax
    int v16; // r15d
    int v17; // r8d
    int v18; // ebp
    unsigned int v19; // eax
    int v20; // r9 (was char)
    int v21; // r8d
    __int64 v22; // r14
    float time; // xmm0_4
    __int64 v24; // rdx
    __int64 v25; // rcx
    __int64 v26; // rdx
    __int64 v27; // rcx
    __int64 v28; // rdx
    __int64 v29; // rcx
    int v30; // edx
    int v31; // r8 (was char)
    float v32; // xmm0_4
    __int64 v33; // r9
    __int64 v34; // rax
    __int64 v35; // rdx
    int v36; // r8d
    float v37; // xmm3_4
    __int64 result; // rax
    float v39; // xmm1_4
    float v40; // xmm0_4
    float v41; // xmm1_4
    netframe_header_t* v42; // rdx
    float v43; // xmm0_4
    float v44; // xmm2_4
    float v45; // xmm0_4

    v7 = (float)*g_pNetTime;
    v8 = flow;
    v9 = inSeqNr;
    netflow_t* pFlow = &pChan->m_DataFlow[flow];
    v12 = outSeqNr;

    netframe_header_t* pFrameHeader = nullptr;
    netframe_t* pFrame = nullptr;

    currentindex = pFlow->currentindex;
    if (outSeqNr > currentindex)
    {
        v16 = currentindex + 1;
        if (currentindex + 1 <= outSeqNr)
        {
            v17 = outSeqNr - v16;
            if (v17 + 1 >= 4)
            {
                v18 = nChoked + nDropped;
                v19 = ((unsigned int)(v12 - v16 - 3) >> 2) + 1;
                v20 = v16 + 2;
                v21 = v17 - 2;
                v22 = v19;
                time = (float)*g_pNetTime;
                v16 += 4 * v19;

                int numPacketFrames = 0;

                do
                {
                    ++numPacketFrames;

                    v24 = (v20 - 2) & NET_FRAMES_MASK;
                    v25 = v24;
                    pFlow->frame_headers[v25].time = time;
                    pFlow->frame_headers[v25].valid = 0;
                    pFlow->frame_headers[v25].size = 0;
                    pFlow->frame_headers[v25].latency = -1.0;
                    pFlow->frames[v24].avg_latency = pChan->m_DataFlow[FLOW_OUTGOING].avglatency;
                    pFlow->frame_headers[v25].choked = 0;
                    pFlow->frames[v24].dropped = 0;
                    if (v21 + 2 < v18)
                    {
                        if (v21 + 2 >= nChoked)
                            pFlow->frames[v24].dropped = 1;
                        else
                            pFlow->frame_headers[(v20 - 2) & NET_FRAMES_MASK].choked = 1;
                    }
                    v26 = (v20 - 1) & NET_FRAMES_MASK;
                    v27 = v26;
                    pFlow->frame_headers[v27].time = time;
                    pFlow->frame_headers[v27].valid = 0;
                    pFlow->frame_headers[v27].size = 0;
                    pFlow->frame_headers[v27].latency = -1.0;
                    pFlow->frames[v26].avg_latency = pChan->m_DataFlow[FLOW_OUTGOING].avglatency;
                    pFlow->frame_headers[v27].choked = 0;
                    pFlow->frames[v26].dropped = 0;
                    if (v21 + 1 < v18)
                    {
                        if (v21 + 1 >= nChoked)
                            pFlow->frames[v26].dropped = 1;
                        else
                            pFlow->frame_headers[(v20 - 1) & NET_FRAMES_MASK].choked = 1;
                    }
                    v28 = v20 & NET_FRAMES_MASK;
                    v29 = v28;
                    pFlow->frame_headers[v29].time = time;
                    pFlow->frame_headers[v29].valid = 0;
                    pFlow->frame_headers[v29].size = 0;
                    pFlow->frame_headers[v29].latency = -1.0;
                    pFlow->frames[v28].avg_latency = pChan->m_DataFlow[FLOW_OUTGOING].avglatency;
                    pFlow->frame_headers[v29].choked = 0;
                    pFlow->frames[v28].dropped = 0;
                    if (v21 < v18)
                    {
                        if (v21 >= nChoked)
                            pFlow->frames[v28].dropped = 1;
                        else
                            pFlow->frame_headers[v20 & NET_FRAMES_MASK].choked = 1;
                    }
                    pFrame = &pFlow->frames[(v20 + 1) & NET_FRAMES_MASK];
                    pFrameHeader = &pFlow->frame_headers[(v20 + 1) & NET_FRAMES_MASK];
                    pFrameHeader->time = time;
                    pFrameHeader->valid = 0;
                    pFrameHeader->size = 0;
                    pFrameHeader->latency = -1.0;
                    pFrame->avg_latency = pChan->m_DataFlow[FLOW_OUTGOING].avglatency;
                    pFrameHeader->choked = 0;
                    pFrame->dropped = 0;
                    if (v21 - 1 < v18)
                    {
                        if (v21 - 1 >= nChoked)
                            pFrame->dropped = 1;
                        else
                            pFrameHeader->choked = 1;
                    }
                    v21 -= 4;
                    v20 += 4;
                    --v22;
                } while (v22 && numPacketFrames < NET_FRAMES_BACKUP);
                v12 = outSeqNr;
                v8 = flow;
                v9 = inSeqNr;
            }
            if (v16 <= v12)
            {
                v30 = v12 - v16;
                v31 = v16;
                v33 = v12 - v16 + 1;
                do
                {
                    pFrame = &pFlow->frames[v31 & NET_FRAMES_MASK];
                    pFrameHeader = &pFlow->frame_headers[v31 & NET_FRAMES_MASK];
                    v32 = v7;
                    pFrameHeader->time = v32;
                    pFrameHeader->valid = 0;
                    pFrameHeader->size = 0;
                    pFrameHeader->latency = -1.0;
                    pFrame->avg_latency = pChan->m_DataFlow[FLOW_OUTGOING].avglatency;
                    pFrameHeader->choked = 0;
                    pFrame->dropped = 0;
                    if (v30 < nChoked + nDropped)
                    {
                        if (v30 >= nChoked)
                            pFrame->dropped = 1;
                        else
                            pFrameHeader->choked = 1;
                    }
                    --v30;
                    ++v31;
                    --v33;
                } while (v33);
                v9 = inSeqNr;
            }
        }
        pFrame->dropped = nDropped;
        pFrameHeader->choked = (short)nChoked;
        pFrameHeader->size = nSize;
        pFrameHeader->valid = 1;
        pFrame->avg_latency = pChan->m_DataFlow[FLOW_OUTGOING].avglatency;
    }
    ++pFlow->totalpackets;
    pFlow->currentindex = v12;
    v34 = 544i64;

    if (!v8)
        v34 = 3688i64;

    pFlow->current_frame = pFrame;
    v35 = 548i64;
    v36 = *(_DWORD*)(&pChan->m_bProcessingMessages + v34);
    if (v9 > v36 - NET_FRAMES_BACKUP)
    {
        if (!v8)
            v35 = 3692i64;
        result = (__int64)pChan + 16 * (v9 & NET_FRAMES_MASK);
        v42 = (netframe_header_t*)(result + v35);
        if (v42->valid && v42->latency == -1.0)
        {
            v43 = 0.0;
            v44 = fmax(0.0f, v7 - v42->time);
            v42->latency = v44;
            if (v44 >= 0.0)
                v43 = v44;
            else
                v42->latency = 0.0;
            v45 = v43 + pFlow->latency;
            ++pFlow->totalupdates;
            pFlow->latency = v45;
            pFlow->maxlatency = fmaxf(pFlow->maxlatency, v42->latency);
        }
    }
    else
    {
        if (!v8)
            v35 = 3692i64;

        v37 = *(float*)(&pChan->m_bProcessingMessages + 16 * (v36 & NET_FRAMES_MASK) + v35);
        result = v35 + 16i64 * (((_BYTE)v36 + 1) & NET_FRAMES_MASK);
        v39 = v37 - *(float*)(&pChan->m_bProcessingMessages + result);
        ++pFlow->totalupdates;
        v40 = (float)((float)(v39 / 127.0) * (float)(v36 - v9)) + v7 - v37;
        v41 = fmaxf(pFlow->maxlatency, v40);
        pFlow->latency = v40 + pFlow->latency;
        pFlow->maxlatency = v41;
    }
}

//-----------------------------------------------------------------------------
// Purpose: shutdown netchannel
// Input  : *this - 
//			*szReason - 
//			bBadRep - 
//			bRemoveNow - 
//-----------------------------------------------------------------------------
void CNetChan::_Shutdown(CNetChan* pChan, const char* szReason, uint8_t bBadRep, bool bRemoveNow)
{
	v_NetChan_Shutdown(pChan, szReason, bBadRep, bRemoveNow);
}

//-----------------------------------------------------------------------------
// Purpose: process message
// Input  : *pChan - 
//			*pMsg - 
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool CNetChan::_ProcessMessages(CNetChan* pChan, bf_read* pMsg)
{
#ifndef CLIENT_DLL
	if (!ThreadInServerFrameThread() || !net_processTimeBudget->GetInt())
		return v_NetChan_ProcessMessages(pChan, pMsg);

	const double flStartTime = Plat_FloatTime();
	const bool bResult = v_NetChan_ProcessMessages(pChan, pMsg);

	if (!pChan->m_MessageHandler) // NetChannel removed?
		return bResult;

	CClient* pClient = reinterpret_cast<CClient*>(pChan->m_MessageHandler);
	ServerPlayer_t* pSlot = &g_ServerPlayer[pClient->GetUserID()];

	if (flStartTime - pSlot->m_flLastNetProcessTime >= 1.0 ||
		pSlot->m_flLastNetProcessTime == -1.0)
	{
		pSlot->m_flLastNetProcessTime = flStartTime;
		pSlot->m_flCurrentNetProcessTime = 0.0;
	}
	pSlot->m_flCurrentNetProcessTime +=
		(Plat_FloatTime() * 1000) - (flStartTime * 1000);

	if (pSlot->m_flCurrentNetProcessTime >
		net_processTimeBudget->GetDouble())
	{
		Warning(eDLL_T::SERVER, "Removing netchannel '%s' ('%s' exceeded frame budget by '%3.1f'ms!)\n", 
			pChan->GetName(), pChan->GetAddress(), (pSlot->m_flCurrentNetProcessTime - net_processTimeBudget->GetDouble()));
		pClient->Disconnect(Reputation_t::REP_MARK_BAD, "#DISCONNECT_NETCHAN_OVERFLOW");

		return false;
	}

	return bResult;
#else // !CLIENT_DLL
	return v_NetChan_ProcessMessages(pChan, pMsg);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: send message
// Input  : &msg - 
//			bForceReliable - 
//			bVoice - 
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool CNetChan::SendNetMsg(INetMessage& msg, bool bForceReliable, bool bVoice)
{
	if (remote_address.GetType() == netadrtype_t::NA_NULL)
		return false;

	bf_write* pStream = &m_StreamUnreliable;

	if (msg.IsReliable() || bForceReliable)
		pStream = &m_StreamReliable;

	if (bVoice)
		pStream = &m_StreamVoice;

	if (pStream != &m_StreamUnreliable ||
		pStream->GetNumBytesLeft() >= NET_UNRELIABLE_STREAM_MINSIZE)
	{
		AcquireSRWLockExclusive(&LOCK);

		pStream->WriteUBitLong(msg.GetType(), NETMSG_TYPE_BITS);
		if (!pStream->IsOverflowed())
			msg.WriteToBuffer(pStream);

		ReleaseSRWLockExclusive(&LOCK);
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: check if there is still data in the reliable waiting buffers
//-----------------------------------------------------------------------------
bool CNetChan::HasPendingReliableData(void)
{
	return (m_StreamReliable.GetNumBitsWritten() > 0)
		|| (m_WaitingList.Count() > 0);
}

///////////////////////////////////////////////////////////////////////////////
void VNetChan::Attach() const
{
	DetourAttach((PVOID*)&v_NetChan_Shutdown, &CNetChan::_Shutdown);
	DetourAttach((PVOID*)&v_NetChan_FlowNewPacket, &CNetChan::_FlowNewPacket);
	DetourAttach((PVOID*)&v_NetChan_ProcessMessages, &CNetChan::_ProcessMessages);
}
void VNetChan::Detach() const
{
	DetourDetach((PVOID*)&v_NetChan_Shutdown, &CNetChan::_Shutdown);
    DetourDetach((PVOID*)&v_NetChan_FlowNewPacket, &CNetChan::_FlowNewPacket);
	DetourDetach((PVOID*)&v_NetChan_ProcessMessages, &CNetChan::_ProcessMessages);
}
