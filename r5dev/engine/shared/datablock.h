//===========================================================================//
// 
// Purpose: data block sender & receiver
// 
//===========================================================================//
#ifndef IDATABLOCK_H
#define IDATABLOCK_H

// the maximum size of each data block fragment
#define MAX_DATABLOCK_FRAGMENT_SIZE 1024

// the maximum amount of fragments per data block transfer
#define MAX_DATABLOCK_FRAGMENTS 768

#define DATABLOCK_DEBUG_NAME_LEN 64
#define DATABLOCK_INVALID_BLOCK_NR -1

// the maximum size of a data block fragment packet (encoded header + actual data)
#define DATABLOCK_FRAGMENT_PACKET_SIZE (MAX_DATABLOCK_FRAGMENT_SIZE + 176)

//-----------------------------------------------------------------------------
// Forward decelerations
//-----------------------------------------------------------------------------
class CClient;
class CClientState;

abstract_class NetDataBlockSender
{
public:
	virtual ~NetDataBlockSender();


	virtual void SendDataBlock(const short transferId, const int transferSize,
		const short transferNr, const short blockNr, const uint8_t* const blockData, const int blockSize) = 0;
	virtual float GetResendRate() const = 0;
	virtual const char* GetReceiverName() const = 0;

	void StartBlockSender(const int transferSize, const bool isMultiplayer, const char* const debugName);
	void ResetBlockSender();

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
	char m_szDebugName[DATABLOCK_DEBUG_NAME_LEN];

	// if a data block has been acknowledged by the receiver, we mark that
	// particular block as acknowledged
	bool m_bBlockAckStatus[MAX_DATABLOCK_FRAGMENTS];
	uint8_t* m_pScratchBuffer;
	bool m_bDumbDataBlockInfo;
};

abstract_class NetDataBlockReceiver
{
public:
	virtual ~NetDataBlockReceiver() {};
	// Called when cvar 'net_debugDataBlockReceiver' is set;
	// currently a nullsub in the engine.
	virtual void DebugDataBlockReceiver() {};
	virtual void AcknowledgeTransmission() = 0;

	void StartBlockReceiver(const int transferSize, const double startTime);
	void ResetBlockReceiver(const short transferNr);

protected:
	// the client side 'client' handle
	CClientState* m_pClientState;

	// whether the transfer has been started and completed
	bool m_bStartedRecv;
	bool m_bCompletedRecv;

	bool byte12;

	// the current transfer id, and the global transfer count for this
	// particular client. the transfer nr keeps getting incremented on
	// each new context setup
	short m_TransferId;
	short m_nTransferNr;

	bool m_bInitialized;

	// the total transfer size, and the # amount of blocks the data has been
	// carved into
	int m_nTransferSize;
	int m_nTotalBlocks;

	int m_nBlockAckTick;

	// the time the data block receiver was started for this transfer
	double m_flStartTime;

	// if we successfully processed a data block, we mark it as processed
	bool m_BlockStatus[MAX_DATABLOCK_FRAGMENTS];
	char* m_pScratchBuffer;
};

inline void* (*NetDataBlockSender__Destructor)(NetDataBlockSender* thisptr);
inline void* (*NetDataBlockReceiver__Destructor)(NetDataBlockReceiver* thisptr);

///////////////////////////////////////////////////////////////////////////////
class VNetDataBlock : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("NetDataBlockSender::~NetDataBlockSender", NetDataBlockSender__Destructor);
		LogFunAdr("NetDataBlockReceiver::~NetDataBlockReceiver", NetDataBlockReceiver__Destructor);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 8B DA 48 8B F9 E8 ?? ?? ?? ?? F6 C3 01 74"
			" 0D BA ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? 48 8B C7 48 8B 5C 24 ?? 48 83 C4 20 5F C3 CC CC CC CC CC CC CC CC CC CC CC CC 48 89 5C 24"
			" ?? 48 89 74 24 ?? 57 48 83 EC 20 33 F6 66 C7 81 ?? ?? ?? ?? ?? ??").GetPtr(NetDataBlockSender__Destructor);

		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 8B DA 48 8B F9 E8 ?? ?? ?? ?? F6 C3 01 74"
			" 0D BA ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? 48 8B C7 48 8B 5C 24 ?? 48 83 C4 20 5F C3 CC CC CC CC CC CC CC CC CC CC CC CC 48 89 5C 24"
			" ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8D 05 ?? ?? ?? ?? C6 41 12 00").GetPtr(NetDataBlockReceiver__Destructor);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // IDATABLOCK_H
