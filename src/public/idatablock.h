#ifndef IDATABLOCK_H
#define IDATABLOCK_H

#define DATABLOCK_STATUS_SIZE 0x300

abstract_class NetDataBlockSender
{
public:
	virtual ~NetDataBlockSender() {};
	virtual void SendDataBlock(short unk0, int unk1,
		short unk2, short unk3, const void* buffer, int length) = 0;
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
