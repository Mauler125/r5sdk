//===========================================================================//
// 
// Purpose: server side datablock sender
// 
//===========================================================================//
#include "engine/client/client.h"
#include "datablock_sender.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ServerDataBlockSender::~ServerDataBlockSender()
{
	ServerDataBlockSender__Destructor(this);
}

//-----------------------------------------------------------------------------
// Purpose: sends the datablock
//-----------------------------------------------------------------------------
void ServerDataBlockSender::SendDataBlock(const short transferId, const int transferSize,
	const short transferNr, const short blockNr, const uint8_t* const blockData, const int blockSize)
{
	ServerDataBlockSender__SendDataBlock(this, transferId, transferSize,
		transferNr, blockNr, blockData, blockSize);
}

//-----------------------------------------------------------------------------
// Purpose: gets the resend rate
//-----------------------------------------------------------------------------
float ServerDataBlockSender::GetResendRate() const
{
    const CClient* const pClient = m_pClient;

    if (!pClient)
        return 0.0f;

    const CNetChan* const pChan = pClient->GetNetChan();

    if (!pChan)
        return 0.0f;

    if (m_bStartedTransfer)
        return 0.0f;

    const float netResendRate = pChan->GetResendRate();

    if (netResendRate < net_datablock_networkLossForSlowSpeed->GetFloat())
    {
        return m_flResendRate;
    }

    return netResendRate;
}

//-----------------------------------------------------------------------------
// Purpose: gets the receiver name (client name as registered on the server)
//-----------------------------------------------------------------------------
const char* ServerDataBlockSender::GetReceiverName() const
{
    return m_pClient->m_szServerName;
}

//-----------------------------------------------------------------------------
// Purpose: initialize the data block sender context
//-----------------------------------------------------------------------------
void ServerDataBlockSender::StartBlockSender(const int transferSize, const bool isMultiplayer, const char* const debugName)
{
	m_bMultiplayer = isMultiplayer;
	m_nBlockAckTick = 0;
	m_nTransferSize = transferSize + sizeof(ServerDataBlockHeader_s);

	// calculate the number of data blocks we have, which get sent individually
	// to the receiver
	m_nTotalBlocks = m_nTransferSize / MAX_DATABLOCK_FRAGMENT_SIZE + (m_nTransferSize % MAX_DATABLOCK_FRAGMENT_SIZE != 0);

	strncpy(m_szDebugName, debugName, sizeof(m_szDebugName));
	m_szDebugName[sizeof(m_szDebugName) - 1] = '\0';

	// null status memory
	memset(m_bBlockAckStatus, 0, sizeof(m_bBlockAckStatus));
	memset(m_flBlockSendTimes, 0, sizeof(m_flBlockSendTimes));

	m_bInitialized = true;
	m_bStartedTransfer = false;

	const double currentTime = Plat_FloatTime();

	m_TimeLastSend = currentTime;
	m_TimeCurrentSend = currentTime;
	m_TimeFirstSend = currentTime;

	m_nTotalSizeRemaining = 4096;
	m_nBlockSendsAttempted = 0;
}

//-----------------------------------------------------------------------------
// Purpose: reset the data block sender context
//-----------------------------------------------------------------------------
void ServerDataBlockSender::ResetBlockSender(void)
{
	if (!m_bInitialized)
		return;

	m_bInitialized = false;
	m_bStartedTransfer = false;

	m_nTransferId = 0;
	m_nTransferSize = 0;
	m_nTotalBlocks = 0;
	m_nBlockAckTick = 0;

	m_TimeCurrentSend = 0.0;
	m_TimeFirstSend = 0.0;

	m_nTotalSizeRemaining = 0;

	m_TimeLastSend = 0.0;
	m_szDebugName[0] = '\0';
	m_bDumbDataBlockInfo = false;
	m_nCurrentBlock = -1;
	m_nBlockSendsAttempted = 0;

	memset(m_bBlockAckStatus, 0, sizeof(m_bBlockAckStatus));
	memset(m_flBlockSendTimes, 0, sizeof(m_flBlockSendTimes));

	m_nTransferNr++;
}

//-----------------------------------------------------------------------------
// Purpose: write the whole data in the data block scratch buffer
//-----------------------------------------------------------------------------
void ServerDataBlockSender::WriteDataBlock(const uint8_t* const sourceData, const int dataSize,
	const bool isMultiplayer, const char* const debugName)
{
	AcquireSRWLockExclusive(&m_Lock);

	ServerDataBlockHeader_s* const pHeader = reinterpret_cast<ServerDataBlockHeader_s*>(m_pScratchBuffer);
	bool copyRaw = true;

	int actualDataSize = dataSize;

	if (net_compressDataBlock->GetBool())
	{
		const int encodedSize = LZ4_compress_fast((const char*)sourceData, (char*)m_pScratchBuffer + sizeof(ServerDataBlockHeader_s),
			dataSize, SNAPSHOT_SCRATCH_BUFFER_SIZE, net_compressDataBlockLzAcceleration->GetInt());

		// this shouldn't happen at all
		if (!encodedSize)
		{
			Assert(0);
			Error(eDLL_T::SERVER, 0, "LZ4 error compressing data block for client.\n");
		}

		// make sure the encoded data is smaller than the raw data, in some cases
		// this might turn larger which means we should just send raw data
		else if (encodedSize < dataSize)
		{
			actualDataSize = encodedSize;

			pHeader->isCompressed = true;
			copyRaw = false;
		}
	}

	// in case no compression was performed, we send the raw data
	if (copyRaw)
	{
		// this should equal the dataSize at this point, even if compression failed
		Assert(actualDataSize == dataSize);

		pHeader->isCompressed = false;
		memcpy(m_pScratchBuffer + sizeof(ServerDataBlockHeader_s), sourceData, actualDataSize);
	}

	// create the context
	StartBlockSender(actualDataSize, isMultiplayer, debugName);

	ReleaseSRWLockExclusive(&m_Lock);
}
