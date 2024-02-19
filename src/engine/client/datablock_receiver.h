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

	void StartBlockReceiver(const int transferSize, const double startTime);
	void ResetBlockReceiver(const short transferNr);

	bool ProcessDataBlock(const double startTime, const short transferId, const int transferSize,
		const short counter, const short currentBlockId, const void* const blockBuffer, const int blockBufferBytes);

protected:
	CClientState* m_pClientState;
	bool m_bStartedRecv;
	bool m_bCompletedRecv;
	bool byte12;
	short m_TransferId;
	short m_nTransferNr;
	bool m_bInitialized;
	int m_nTransferSize;
	int m_nTotalBlocks;
	int m_nBlockAckTick;
	double m_flStartTime;

	bool m_BlockStatus[MAX_DATABLOCK_FRAGMENTS];
	char* m_pScratchBuffer;
};

struct ClientDataBlockHeader_s
{
	char reserved[3]; // unused in retail
	bool isCompressed;
};

// virtual methods
inline void*(*ClientDataBlockReceiver__Destructor)(ClientDataBlockReceiver* thisptr);
inline void*(*ClientDataBlockReceiver__AcknowledgeTransmission)(ClientDataBlockReceiver* thisptr);

// non-virtual methods
inline bool (*ClientDataBlockReceiver__ProcessDataBlock)(ClientDataBlockReceiver* thisptr, const double time,
	const short transferId, const int transferSize, const short counter, const short currentBlockId,
	const void* const blockBuffer, const int blockBufferBytes);

///////////////////////////////////////////////////////////////////////////////
class VClientDataBlockReceiver : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("ClientDataBlockReceiver::~ClientDataBlockReceiver", ClientDataBlockReceiver__Destructor);
		LogFunAdr("ClientDataBlockReceiver::AcknowledgeTransmission", ClientDataBlockReceiver__AcknowledgeTransmission);
		LogFunAdr("ClientDataBlockReceiver::ProcessDataBlock", ClientDataBlockReceiver__ProcessDataBlock);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 8B DA 48 8B F9 E8 ?? ?? ?? ?? F6 C3 01 74"
			" 0D BA ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? 48 8B C7 48 8B 5C 24 ?? 48 83 C4 20 5F C3 CC CC CC CC CC CC CC CC CC CC CC CC 48 89 5C 24"
			" ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8D 05 ?? ?? ?? ?? C6 41 12 00").GetPtr(ClientDataBlockReceiver__Destructor);

		g_GameDll.FindPatternSIMD("40 53 48 81 EC ?? ?? ?? ?? 4C 8B 51 08").GetPtr(ClientDataBlockReceiver__AcknowledgeTransmission);

		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC 30 0F B7 44 24 ??")
			.GetPtr(ClientDataBlockReceiver__ProcessDataBlock);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // DATABLOCK_RECEIVER_H
