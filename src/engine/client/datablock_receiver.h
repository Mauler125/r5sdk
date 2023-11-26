#ifndef DATABLOCK_RECEIVER_H
#define DATABLOCK_RECEIVER_H
#include "idatablock.h"

class CClientState;

class ClientDataBlockReceiver : public NetDataBlockReceiver
{
	friend class CClientState;
public:
	virtual ~ClientDataBlockReceiver();
	virtual void AcknowledgeTransmission() override;

protected:
	CClientState* m_pClientState;
	bool m_bStartedRecv;
	bool m_bCompletedRecv;
	bool byte12;
	short m_TransferId;
	short m_Counter;
	bool m_bInitialized;
	int m_nTransferSize;
	int m_nTotalBlocks;
	int m_nBlockAckTick;
	double m_flStartTime;
	bool m_BlockStatus[DATABLOCK_STATUS_SIZE];
	void* m_pBigBuffer;
};

inline CMemory p_ClientDataBlockReceiver__Destructor;
inline void*(*v_ClientDataBlockReceiver__Destructor)(ClientDataBlockReceiver* thisptr);

inline CMemory p_ClientDataBlockReceiver__AcknowledgeTransmission;
inline void*(*v_ClientDataBlockReceiver__AcknowledgeTransmission)(ClientDataBlockReceiver* thisptr);

///////////////////////////////////////////////////////////////////////////////
class VClientDataBlockReceiver : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("ClientDataBlockReceiver::~ClientDataBlockReceiver", p_ClientDataBlockReceiver__Destructor.GetPtr());
		LogFunAdr("ClientDataBlockReceiver::AcknowledgeTransmission", p_ClientDataBlockReceiver__AcknowledgeTransmission.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_ClientDataBlockReceiver__Destructor = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 8B DA 48 8B F9 E8 ?? ?? ?? ?? F6 C3 01 74"
			" 0D BA ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? 48 8B C7 48 8B 5C 24 ?? 48 83 C4 20 5F C3 CC CC CC CC CC CC CC CC CC CC CC CC 48 89 5C 24"
			" ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8D 05 ?? ?? ?? ?? C6 41 12 00");
		v_ClientDataBlockReceiver__Destructor = p_ClientDataBlockReceiver__Destructor.RCast<void* (*)(ClientDataBlockReceiver*)>();

		p_ClientDataBlockReceiver__AcknowledgeTransmission = g_GameDll.FindPatternSIMD("40 53 48 81 EC ?? ?? ?? ?? 4C 8B 51 08");
		v_ClientDataBlockReceiver__AcknowledgeTransmission = p_ClientDataBlockReceiver__AcknowledgeTransmission.RCast<void* (*)(ClientDataBlockReceiver*)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // DATABLOCK_RECEIVER_H
