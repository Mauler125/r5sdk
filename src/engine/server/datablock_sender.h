#ifndef DATABLOCK_SENDER_H
#define DATABLOCK_SENDER_H
#include "idatablock.h"

class CClient;

class ServerDataBlockSender : public NetDataBlockSender
{
	friend class CClient;
public:
	virtual ~ServerDataBlockSender() override;
	virtual void SendDataBlock(const short transferId, const int transferSize, const short transferNr,
		const short blockNr, const uint8_t* const blockData, const int blockSize) override;

	virtual float GetResendRate() const override;
	virtual const char* GetReceiverName() const override;

	void StartBlockSender(const int transferSize, const bool isMultiplayer, const char* const debugName);
	void ResetBlockSender();

	void WriteDataBlock(const uint8_t* const sourceData, const int dataSize, const bool isMultiplayer, const char* const debugName);

protected:
	char pad_0008[56];
	RTL_SRWLOCK m_Lock;
	char pad_0048[56];

	// the server side client handle that is our 'receiving' end
	CClient* m_pClient;

	char m_bInitialized;
	char m_bStartedTransfer;

	char m_bMultiplayer;
	char field_8B;

	// the current transfer id, and the global transfer count for this
	// particular client. the transfer nr keeps getting incremented on
	// each new context setup
	short m_nTransferId;
	short m_nTransferNr;

	// the total transfer size for the data, and the number of blocks this data
	// has been carved up to
	int m_nTransferSize;
	int m_nTotalBlocks;

	// last block that has been ack'd, and the current block that is pending to
	// be sent to the receiver
	int m_nBlockAckTick;
	int m_nCurrentBlock;

	// the total number of bytes remaining to be sent, and the number of times
	// we attempted to send data blocks
	int m_nTotalSizeRemaining;
	int m_nBlockSendsAttempted;

	// the resend rate for this connection, which depends of the stability/loss
	// and other factors computed from the netchan
	float m_flResendRate;
	char pad_00AC[4]; // padding, in case we want to stuff our own vars in here

	// times used to determine when a data block has been sent, and how long it
	// took to get this out and acknowledged
	double m_TimeCurrentSend;
	double m_TimeFirstSend;
	double m_TimeLastSend;

	// the last time we attempted to send this block, this gets updated when
	// a data block hasn't been acknowledged in time and is being resent
	double m_flBlockSendTimes[MAX_DATABLOCK_FRAGMENTS];

	// the debug name used when details get dumped to the console
	char m_szDebugName[MAX_DATABLOCK_DEBUG_NAME];

	// if a data block has been acknowledged by the receiver, we mark that
	// particular block as acknowledged
	bool m_bBlockAckStatus[MAX_DATABLOCK_FRAGMENTS];
	uint8_t* m_pScratchBuffer;
	bool m_bDumbDataBlockInfo;
};

struct ServerDataBlock
{
	char blockBuffer[295312]; // this might be wrong !!!
	void* userData;
	char gapC0008[56];
	ServerDataBlockSender sender;
};

struct ServerDataBlockHeader_s
{
	bool isCompressed;
};

inline void*(*ServerDataBlockSender__Destructor)(ServerDataBlockSender* thisptr);
inline void* (*ServerDataBlockSender__SendDataBlock)(ServerDataBlockSender* thisptr,
	const short transferId, const int transferSize, const short transferNr,
	const short blockNr, const uint8_t* const blockData, const int blockSize);

///////////////////////////////////////////////////////////////////////////////
class VServerDataBlockSender : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("ServerDataBlockSender::~ServerDataBlockSender", ServerDataBlockSender__Destructor);
		LogFunAdr("ServerDataBlockSender::SendDataBlock", ServerDataBlockSender__SendDataBlock);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 8B DA 48 8B F9 E8 ?? ?? ?? ?? F6 C3 01 74"
			" 0D BA ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? 48 8B C7 48 8B 5C 24 ?? 48 83 C4 20 5F C3 CC CC CC CC CC CC CC CC CC CC CC CC 48 89 5C 24"
			" ?? 48 89 74 24 ?? 57 48 83 EC 20 33 F6 66 C7 81 ?? ?? ?? ?? ?? ??").GetPtr(ServerDataBlockSender__Destructor);

		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B 99 ?? ?? ?? ??").GetPtr(ServerDataBlockSender__SendDataBlock);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // DATABLOCK_SENDER_H
