#ifndef DATABLOCK_SENDER_H
#define DATABLOCK_SENDER_H
#include "idatablock.h"

class CClient;

class ServerDataBlockSender : public NetDataBlockSender
{
	friend class CClient;
public:
	virtual ~ServerDataBlockSender() override;
	virtual void SendDataBlock(short unk0, int unk1,
		short unk2, short unk3, const void* buffer, int length) override;

	virtual float GetResendRate() const override;
	virtual const char* GetReceiverName() const override;

protected:
	char pad_0008[56];
	RTL_SRWLOCK LOCK;
	char pad_0048[56];
	CClient* m_pClient;
	char m_bInitialized;
	char m_bStartedTransfer;
	char m_bMultiplayer;
	char field_8B;
	short m_nTransferId;
	short m_nCounter;
	int m_nTransferSize;
	int m_nTotalBlocks;
	int m_nBlockAckTick;
	int m_nCurrentBlock;
	int m_nUnkA0;
	int m_nBlockSendsAttempted;
	float m_flResendRate;
	char pad_00AC[4];
	double m_TimeCurrentSend;
	double m_TimeFirstSend;
	double m_TimeLastSend;
	double m_flBlockTimesArray[DATABLOCK_STATUS_SIZE];
	char m_szDebugName[64];
	bool m_bBlockStatusArray[DATABLOCK_STATUS_SIZE];
	void* m_pData;
	bool m_bAbnormalSending_Maybe;
};

struct ServerDataBlock
{
	char blockBuffer[295312]; // this might be wrong !!!
	void* userData;
	char gapC0008[56];
	ServerDataBlockSender sender;
};

inline CMemory p_ServerDataBlockSender__Destructor;
inline void*(*v_ServerDataBlockSender__Destructor)(ServerDataBlockSender* thisptr);

inline CMemory p_ServerDataBlockSender__SendDataBlock;
inline void* (*v_ServerDataBlockSender__SendDataBlock)(ServerDataBlockSender* thisptr,
	short unk0, int unk1, short unk2, short unk3, const void* buffer, int length);

///////////////////////////////////////////////////////////////////////////////
class VServerDataBlockSender : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("ServerDataBlockSender::~ServerDataBlockSender", p_ServerDataBlockSender__Destructor.GetPtr());
		LogFunAdr("ServerDataBlockSender::SendDataBlock", p_ServerDataBlockSender__SendDataBlock.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_ServerDataBlockSender__Destructor = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 8B DA 48 8B F9 E8 ?? ?? ?? ?? F6 C3 01 74"
			" 0D BA ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? 48 8B C7 48 8B 5C 24 ?? 48 83 C4 20 5F C3 CC CC CC CC CC CC CC CC CC CC CC CC 48 89 5C 24"
			" ?? 48 89 74 24 ?? 57 48 83 EC 20 33 F6 66 C7 81 ?? ?? ?? ?? ?? ??");
		v_ServerDataBlockSender__Destructor = p_ServerDataBlockSender__Destructor.RCast<void* (*)(ServerDataBlockSender*)>();

		p_ServerDataBlockSender__SendDataBlock = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B 99 ?? ?? ?? ??");
		v_ServerDataBlockSender__SendDataBlock = p_ServerDataBlockSender__SendDataBlock.RCast<void* (*)(ServerDataBlockSender*,
			short, int, short, short, const void*, int)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // DATABLOCK_SENDER_H
