#ifndef IDATABLOCK_H
#define IDATABLOCK_H

// the maximum size of each data block fragment
#define MAX_DATABLOCK_FRAGMENT_SIZE 1024

// the maximum amount of fragments per data block
#define MAX_DATABLOCK_FRAGMENTS 768

#define MAX_DATABLOCK_DEBUG_NAME 64

abstract_class NetDataBlockSender
{
public:
	virtual ~NetDataBlockSender() {};
	virtual void SendDataBlock(const short transferId, const int transferSize,
		const short transferNr, const short blockNr, const uint8_t* const blockData, const int blockSize) = 0;
	virtual float GetResendRate() const = 0;
	virtual const char* GetReceiverName() const = 0;
};

abstract_class NetDataBlockReceiver
{
public:
	virtual ~NetDataBlockReceiver() {};
	// Called when cvar 'net_debugDataBlockReceiver' is set;
	// currently a nullsub in the engine.
	virtual void DebugDataBlockReceiver() {};
	virtual void AcknowledgeTransmission() = 0;
};

#endif // IDATABLOCK_H
