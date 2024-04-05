//===========================================================================//
// 
// Purpose: client side data block receiver
// 
//===========================================================================//
#ifndef DATABLOCK_RECEIVER_H
#define DATABLOCK_RECEIVER_H
#include "engine/shared/datablock.h"

class CClientState;

class ClientDataBlockReceiver : public NetDataBlockReceiver
{
public:
	virtual void AcknowledgeTransmission() override;

	bool ProcessDataBlock(const double startTime, const short transferId, const int transferSize,
		const short counter, const short currentBlockId, const void* const blockBuffer, const int blockBufferBytes);
};

struct ClientDataBlockHeader_s
{
	char reserved[3]; // unused in retail
	bool isCompressed;
};

// virtual methods
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
		LogFunAdr("ClientDataBlockReceiver::AcknowledgeTransmission", ClientDataBlockReceiver__AcknowledgeTransmission);
		LogFunAdr("ClientDataBlockReceiver::ProcessDataBlock", ClientDataBlockReceiver__ProcessDataBlock);
	}
	virtual void GetFun(void) const
	{
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
