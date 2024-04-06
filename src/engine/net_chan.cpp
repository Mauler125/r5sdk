//=============================================================================//
//
// Purpose: Netchannel system utilities
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/frametask.h"
#include "tier1/cvar.h"
#include "tier1/keyvalues.h"
#include "common/callback.h"
#include "engine/net.h"
#include "engine/net_chan.h"
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#include "engine/client/client.h"
#include "server/vengineserver_impl.h"
#endif // !CLIENT_DLL

//-----------------------------------------------------------------------------
// Console variables
//-----------------------------------------------------------------------------
static ConVar net_processTimeBudget("net_processTimeBudget", "200", FCVAR_RELEASE, "Net message process time budget in milliseconds (removing netchannel if exceeded).", true, 0.f, false, 0.f, "0 = disabled");

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel resend rate
// Output : float
//-----------------------------------------------------------------------------
float CNetChan::GetResendRate() const
{
	const int64_t totalupdates = this->m_DataFlow[FLOW_INCOMING].totalupdates;

	if (!totalupdates && !this->m_nSequencesSkipped_MAYBE)
		return 0.0f;

	float lossRate = (float)(totalupdates + m_nSequencesSkipped_MAYBE);

	if (totalupdates + m_nSequencesSkipped_MAYBE < 0.0f)
		lossRate += float(2 ^ 64);

	return m_nSequencesSkipped_MAYBE / lossRate;
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
// Purpose: gets the number of bits written in selected stream
//-----------------------------------------------------------------------------
int CNetChan::GetNumBitsWritten(const bool bReliable)
{
    bf_write* pStream = &m_StreamUnreliable;

    if (bReliable)
    {
        pStream = &m_StreamReliable;
    }

    return pStream->GetNumBitsWritten();
}

//-----------------------------------------------------------------------------
// Purpose: gets the number of bits written in selected stream
//-----------------------------------------------------------------------------
int CNetChan::GetNumBitsLeft(const bool bReliable)
{
    bf_write* pStream = &m_StreamUnreliable;

    if (bReliable)
    {
        pStream = &m_StreamReliable;
    }

    return pStream->GetNumBitsLeft();
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
    float netTime; // xmm4_8 (was double)
    int v8; // r13d
    int v9; // r14d
    int v12; // r12d
    int currentindex; // eax
    int nextIndex; // r15d
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

    netTime = (float)*g_pNetTime;
    v8 = flow;
    v9 = inSeqNr;
    netflow_t* pFlow = &pChan->m_DataFlow[flow];
    v12 = outSeqNr;

    netframe_header_t* pFrameHeader = nullptr;
    netframe_t* pFrame = nullptr;

    currentindex = pFlow->currentindex;
    if (outSeqNr > currentindex)
    {
        nextIndex = currentindex + 1;
        if (currentindex + 1 <= outSeqNr)
        {
            // This variable makes sure the loops below do not execute more
            // than NET_FRAMES_BACKUP times. This has to be done as the
            // headers and frame arrays in the netflow_t structure is as
            // large as NET_FRAMES_BACKUP. Any execution past it is futile
            // and only wastes CPU time. Sending an outSeqNr that is higher
            // than the current index by something like a million or more will
            // hang the engine for several milliseconds to several seconds.
            int numPacketFrames = 0;

            v17 = outSeqNr - nextIndex;

            if (v17 + 1 >= 4)
            {
                v18 = nChoked + nDropped;
                v19 = ((unsigned int)(v12 - nextIndex - 3) >> 2) + 1;
                v20 = nextIndex + 2;
                v21 = v17 - 2;
                v22 = v19;
                time = (float)*g_pNetTime;
                nextIndex += 4 * v19;

                do
                {
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

                    // Incremented by four since this loop does four frames
                    // per iteration.
                    numPacketFrames += 4;
                    v21 -= 4;
                    v20 += 4;
                    --v22;
                } while (v22 && numPacketFrames < NET_FRAMES_BACKUP);
                v12 = outSeqNr;
                v8 = flow;
                v9 = inSeqNr;
            }

            // Check if we did not reach NET_FRAMES_BACKUP, else we will
            // execute the 129'th iteration as well. Also check if the next
            // index doesn't exceed the outSeqNr.
            if (numPacketFrames < NET_FRAMES_BACKUP && nextIndex <= v12)
            {
                v30 = v12 - nextIndex;
                v31 = nextIndex;
                v33 = v12 - nextIndex + 1;
                do
                {
                    pFrame = &pFlow->frames[v31 & NET_FRAMES_MASK];
                    pFrameHeader = &pFlow->frame_headers[v31 & NET_FRAMES_MASK];
                    v32 = netTime;
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
                    ++numPacketFrames;
                } while (v33 && numPacketFrames < NET_FRAMES_BACKUP);
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
            v44 = fmax(0.0f, netTime - v42->time);
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
        v40 = (float)((float)(v39 / 127.0) * (float)(v36 - v9)) + netTime - v37;
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
	CNetChan__Shutdown(pChan, szReason, bBadRep, bRemoveNow);
}

//-----------------------------------------------------------------------------
// Purpose: process message
// Input  : *pChan - 
//			*pMsg - 
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool CNetChan::_ProcessMessages(CNetChan* pChan, bf_read* pBuf)
{
#ifndef CLIENT_DLL
    if (!net_processTimeBudget.GetInt() || !ThreadInServerFrameThread())
        return pChan->ProcessMessages(pBuf);

    const double flStartTime = Plat_FloatTime();
    const bool bResult = pChan->ProcessMessages(pBuf);

    if (!pChan->m_MessageHandler) // NetChannel removed?
        return bResult;

    CClient* const pClient = reinterpret_cast<CClient*>(pChan->m_MessageHandler);
    CClientExtended* const pExtended = pClient->GetClientExtended();

    // Reset every second.
    if ((flStartTime - pExtended->GetNetProcessingTimeBase()) > 1.0)
    {
        pExtended->SetNetProcessingTimeBase(flStartTime);
        pExtended->SetNetProcessingTimeMsecs(0.0, 0.0);
    }

    const double flCurrentTime = Plat_FloatTime();
    pExtended->SetNetProcessingTimeMsecs(flStartTime, flCurrentTime);

    if (pExtended->GetNetProcessingTimeMsecs() > net_processTimeBudget.GetFloat())
    {
        Warning(eDLL_T::SERVER, "Removing netchannel '%s' ('%s' exceeded time budget by '%3.1f'ms!)\n",
            pChan->GetName(), pChan->GetAddress(), (pExtended->GetNetProcessingTimeMsecs() - net_processTimeBudget.GetFloat()));
        pClient->Disconnect(Reputation_t::REP_MARK_BAD, "#DISCONNECT_NETCHAN_OVERFLOW");

        return false;
    }

    return bResult;
#else // !CLIENT_DLL
    return pChan->ProcessMessages(pBuf);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: process message
// Input  : *buf - 
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool CNetChan::ProcessMessages(bf_read* buf)
{
    m_bStopProcessing = false;

    const char* showMsgName = net_showmsg->GetString();
    const char* blockMsgName = net_blockmsg->GetString();
    const int netPeak = net_showpeaks->GetInt();

    if (*showMsgName == '0')
    {
        showMsgName = NULL; // dont do strcmp all the time
    }

    if (*blockMsgName == '0')
    {
        blockMsgName = NULL; // dont do strcmp all the time
    }

    if (netPeak > 0 && netPeak < buf->GetNumBytesLeft())
    {
        showMsgName = "1"; // show messages for this packet only
    }

    while (true)
    {
        int cmd = net_NOP;

        while (true)
        {
            if (buf->GetNumBitsLeft() < NETMSG_TYPE_BITS)
                return true; // Reached the end.

            if (!NET_ReadMessageType(&cmd, buf) && buf->m_bOverflow)
            {
                Error(eDLL_T::ENGINE, 0, "%s(%s): Incoming buffer overflow!\n", __FUNCTION__, GetAddress());
                m_MessageHandler->ConnectionCrashed("Buffer overflow in net message");

                return false;
            }

            if (cmd <= net_Disconnect)
                break; // Either a Disconnect or NOP packet; process it below.

            INetMessage* netMsg = FindMessage(cmd);

            if (!netMsg)
            {
                DevWarning(eDLL_T::ENGINE, "%s(%s): Received unknown net message (%i)!\n",
                    __FUNCTION__, GetAddress(), cmd);
                Assert(0);
                return false;
            }

            if (!netMsg->ReadFromBuffer(buf))
            {
                DevWarning(eDLL_T::ENGINE, "%s(%s): Failed reading message '%s'!\n",
                    __FUNCTION__, GetAddress(), netMsg->GetName());
                Assert(0);
                return false;
            }

            if (showMsgName)
            {
                if ((*showMsgName == '1') || !Q_stricmp(showMsgName, netMsg->GetName()))
                {
                    Msg(eDLL_T::ENGINE, "%s(%s): Received: %s\n",
                        __FUNCTION__, GetAddress(), netMsg->ToString());
                }
            }

            if (blockMsgName)
            {
                if ((*blockMsgName == '1') || !Q_stricmp(blockMsgName, netMsg->GetName()))
                {
                    Msg(eDLL_T::ENGINE, "%s(%s): Blocked: %s\n",
                        __FUNCTION__, GetAddress(), netMsg->ToString());

                    continue;
                }
            }

            // Netmessage calls the Process function that was registered by
            // it's MessageHandler.
            m_bProcessingMessages = true;
            const bool bRet = netMsg->Process();
            m_bProcessingMessages = false;

            // This means we were deleted during the processing of that message.
            if (m_bShouldDelete)
            {
                delete this;
                return false;
            }

            // This means our message buffer was freed or invalidated during
            // the processing of that message.
            if (m_bStopProcessing)
                return false;

            if (!bRet)
            {
                DevWarning(eDLL_T::ENGINE, "%s(%s): Failed processing message '%s'!\n",
                    __FUNCTION__, GetAddress(), netMsg->GetName());
                Assert(0);
                return false;
            }

            if (IsOverflowed())
                return false;
        }

        m_bProcessingMessages = true;

        if (cmd == net_NOP) // NOP; continue to next packet.
        {
            m_bProcessingMessages = false;
            continue;
        }
        else if (cmd == net_Disconnect) // Disconnect request.
        {
            char reason[1024];
            buf->ReadString(reason, sizeof(reason), false);

            m_MessageHandler->ConnectionClosing(reason, 1);
            m_bProcessingMessages = false;
        }

        m_bProcessingMessages = false;

        if (m_bShouldDelete)
            delete this;

        return false;
    }
}

bool CNetChan::ReadSubChannelData(bf_read& buf)
{
    // TODO: rebuild this and hook
    return false;
}

//-----------------------------------------------------------------------------
// Purpose: send message
// Input  : &msg - 
//			bForceReliable - 
//			bVoice - 
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool CNetChan::SendNetMsg(INetMessage& msg, const bool bForceReliable, const bool bVoice)
{
	if (remote_address.GetType() == netadrtype_t::NA_NULL)
		return true;

	bf_write* pStream = &m_StreamUnreliable;

	if (msg.IsReliable() || bForceReliable)
		pStream = &m_StreamReliable;

	if (bVoice)
		pStream = &m_StreamVoice;

	if (pStream == &m_StreamUnreliable && pStream->GetNumBytesLeft() < NET_UNRELIABLE_STREAM_MINSIZE)
		return true;

	AcquireSRWLockExclusive(&m_Lock);

	pStream->WriteUBitLong(msg.GetType(), NETMSG_TYPE_BITS);
	const bool ret = msg.WriteToBuffer(pStream);

	ReleaseSRWLockExclusive(&m_Lock);

	return !pStream->IsOverflowed() && ret;
}

//-----------------------------------------------------------------------------
// Purpose: send data
// Input  : &msg - 
//			bReliable - 
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool CNetChan::SendData(bf_write& msg, const bool bReliable)
{
    // Always queue any pending reliable data ahead of the fragmentation buffer

    if (remote_address.GetType() == netadrtype_t::NA_NULL)
        return true;

    if (msg.GetNumBitsWritten() <= 0)
        return true;

    if (msg.IsOverflowed() && !bReliable)
        return true;

    bf_write& buf = bReliable
        ? m_StreamReliable
        : m_StreamUnreliable;

    const int dataBits = msg.GetNumBitsWritten();
    const int bitsLeft = buf.GetNumBitsLeft();

    if (dataBits > bitsLeft)
    {
        if (bReliable)
        {
            Error(eDLL_T::ENGINE, 0, "%s(%s): Data too large for reliable buffer (%i > %i)!\n", 
                __FUNCTION__, GetAddress(), msg.GetNumBytesWritten(), buf.GetNumBytesLeft());

            m_MessageHandler->ChannelDisconnect("reliable buffer is full");
        }

        return false;
    }

    return buf.WriteBits(msg.GetData(), dataBits);
}

//-----------------------------------------------------------------------------
// Purpose: finds a registered net message by type
// Input  : type - 
// Output : net message pointer on success, NULL otherwise
//-----------------------------------------------------------------------------
INetMessage* CNetChan::FindMessage(int type)
{
    int numtypes = m_NetMessages.Count();

    for (int i = 0; i < numtypes; i++)
    {
        if (m_NetMessages[i]->GetType() == type)
            return m_NetMessages[i];
    }

    return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: registers a net message
// Input  : *msg
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetChan::RegisterMessage(INetMessage* msg)
{
    Assert(msg);

    if (FindMessage(msg->GetType()))
    {
        Assert(0); // Duplicate registration!
        return false;
    }

    m_NetMessages.AddToTail(msg);
    msg->SetNetChannel(this);

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: free's the receive data fragment list
//-----------------------------------------------------------------------------
void CNetChan::FreeReceiveList()
{
    m_ReceiveList.blockSize = NULL;
    m_ReceiveList.transferSize = NULL;
    if (m_ReceiveList.buffer)
    {
        delete m_ReceiveList.buffer;
        m_ReceiveList.buffer = nullptr;
    }
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
void VNetChan::Detour(const bool bAttach) const
{
	DetourSetup(&CNetChan__Shutdown, &CNetChan::_Shutdown, bAttach);
	DetourSetup(&CNetChan__FlowNewPacket, &CNetChan::_FlowNewPacket, bAttach);
	DetourSetup(&CNetChan__ProcessMessages, &CNetChan::_ProcessMessages, bAttach);
}
