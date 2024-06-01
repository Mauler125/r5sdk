//===========================================================================//
// 
// Purpose: server side data block sender
// 
//===========================================================================//
#ifndef DATABLOCK_SENDER_H
#define DATABLOCK_SENDER_H
#include "engine/shared/datablock.h"

class CClient;

class ServerDataBlockSender : public NetDataBlockSender
{
public:
	virtual void SendDataBlock(const short transferId, const int transferSize, const short transferNr,
		const short blockNr, const uint8_t* const blockData, const int blockSize) override;

	virtual float GetResendRate() const override;
	virtual const char* GetReceiverName() const override;

	void WriteDataBlock(const uint8_t* const sourceData, const int dataSize, const bool isMultiplayer, const char* const debugName);
};

struct ServerDataBlock
{
	void* userData;
	char gapC0008[56];
	ServerDataBlockSender sender;
};

struct ServerDataBlockHeader_s
{
	bool isCompressed;
};

inline void* (*ServerDataBlockSender__SendDataBlock)(ServerDataBlockSender* thisptr,
	const short transferId, const int transferSize, const short transferNr,
	const short blockNr, const uint8_t* const blockData, const int blockSize);

///////////////////////////////////////////////////////////////////////////////
class VServerDataBlockSender : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("ServerDataBlockSender::SendDataBlock", ServerDataBlockSender__SendDataBlock);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B 99 ?? ?? ?? ??").GetPtr(ServerDataBlockSender__SendDataBlock);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // DATABLOCK_SENDER_H
