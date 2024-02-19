//===========================================================================//
// 
// Purpose: client side datablock receiver
// 
//===========================================================================//
#include "engine/client/clientstate.h"
#include "datablock_receiver.h"
#include "engine/common.h"
#include "engine/host_cmd.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientDataBlockReceiver::~ClientDataBlockReceiver()
{
	ClientDataBlockReceiver__Destructor(this);
}

//-----------------------------------------------------------------------------
// Purpose: send an ack back to the server to let them know
// we received the datablock
//-----------------------------------------------------------------------------
void ClientDataBlockReceiver::AcknowledgeTransmission()
{
	ClientDataBlockReceiver__AcknowledgeTransmission(this);
}

//-----------------------------------------------------------------------------
// Purpose: initialize the data block receiver context
//-----------------------------------------------------------------------------
void ClientDataBlockReceiver::StartBlockReceiver(const int transferSize, const double startTime)
{
	m_bStartedRecv = true;
	m_nTransferSize = transferSize;
	m_nTotalBlocks = transferSize / MAX_DATABLOCK_FRAGMENT_SIZE + (transferSize % MAX_DATABLOCK_FRAGMENT_SIZE != 0);
	m_nBlockAckTick = 0;
	m_flStartTime = startTime;

	memset(m_BlockStatus, 0, sizeof(m_BlockStatus));
}

//-----------------------------------------------------------------------------
// Purpose: reset the data block receiver context
//-----------------------------------------------------------------------------
void ClientDataBlockReceiver::ResetBlockReceiver(const short transferNr)
{
	m_nTransferNr = transferNr;

	m_bStartedRecv = false;
	m_bCompletedRecv = false;

	m_TransferId = 0;
	m_nTotalBlocks = 0;
	m_nBlockAckTick = 0;
	m_flStartTime = 0.0;

	memset(m_BlockStatus, 0, sizeof(m_BlockStatus));
}

//-----------------------------------------------------------------------------
// Purpose: process the recv'd data block and reconstruct the fragmented data
//-----------------------------------------------------------------------------
bool ClientDataBlockReceiver::ProcessDataBlock(const double startTime, const short transferId, const int transferSize,
	const short transferNr, const short currentBlockId, const void* const blockBuffer, const int blockBufferBytes)
{
	// should we process a new transfer?
	if (transferNr != m_nTransferNr)
		ResetBlockReceiver(transferNr);

	m_bInitialized = true;

	// make sure we always receive fragments in order
	if (transferId != m_TransferId || m_bCompletedRecv)
		return false;

	// initialize the receiver if this is the firs fragment
	if (!m_bStartedRecv)
		StartBlockReceiver(transferSize, startTime);

	// received more blocks than the total # expected?
	if (currentBlockId >= m_nTotalBlocks)
		return false;

	// check if we have already copied the data block
	if (!m_BlockStatus[currentBlockId])
	{
		const int scratchBufferOffset = currentBlockId * MAX_DATABLOCK_FRAGMENT_SIZE;

		if (blockBufferBytes + scratchBufferOffset <= m_nTransferSize)
			memcpy(m_pScratchBuffer + scratchBufferOffset + (sizeof(ClientDataBlockHeader_s) -1), blockBuffer, blockBufferBytes);

		++m_nBlockAckTick;
		m_BlockStatus[currentBlockId] = true;
	}

	// check if we have recv'd enough fragments to decode the data
	if (m_nBlockAckTick != m_nTotalBlocks)
		return true;

	AcknowledgeTransmission();
	m_bCompletedRecv = true;

	const ClientDataBlockHeader_s* const pHeader = reinterpret_cast<ClientDataBlockHeader_s*>(m_pScratchBuffer);

	if (pHeader->isCompressed)
	{
		// NOTE: the engine's implementation of this function does NOT free
		// this buffer when a malformed/corrupt LZ4 packet is sent to the
		// receiver; wrapped buffer in unique_ptr to make sure it never leaks!
		std::unique_ptr<char> encodedDataBuf(new char[SNAPSHOT_SCRATCH_BUFFER_SIZE]);

		char* const pEncodedDataBuf = encodedDataBuf.get();
		char* const dataLocation = m_pScratchBuffer + sizeof(ClientDataBlockHeader_s);

		// copy the encoded data in the newly allocated buffer so we can decode back
		// into the data block buffer we copied the encoded data from
		const int compressedSize = m_nTransferSize -1;

		memcpy(pEncodedDataBuf, dataLocation, compressedSize);
		const int numDecode = LZ4_decompress_safe(pEncodedDataBuf, dataLocation, compressedSize, SNAPSHOT_SCRATCH_BUFFER_SIZE);

		if (numDecode < 0)
		{
			Assert(0);

			COM_ExplainDisconnection(true, "LZ4 error decompressing data block from server.\n");
			v_Host_Disconnect(true);

			return false;
		}

		m_nTransferSize = numDecode;
	}
	else
	{
		// truncate the byte that determines whether the data was compressed
		m_nTransferSize--;
	}

	return true;
}

//-----------------------------------------------------------------------------
// NOTE: detoured for 2 reasons:
// 1: when a corrupt or malformed compress packet is sent, the code never freed
//    the temporary copy buffer it made to decode the data into the scratch buf
// 2: exploring other compression algorithms for potential optimizations
//-----------------------------------------------------------------------------
static bool HK_ProcessDataBlock(ClientDataBlockReceiver* receiver, const double startTime,
	const short transferId, const int transferSize, const short transferNr,
	const short currentBlockId, const void* const blockBuffer, const int blockBufferBytes)
{
	return receiver->ProcessDataBlock(startTime, transferId, transferSize, transferNr, 
		currentBlockId, blockBuffer, blockBufferBytes);
}

void VClientDataBlockReceiver::Detour(const bool bAttach) const
{
	DetourAttach(&ClientDataBlockReceiver__ProcessDataBlock, HK_ProcessDataBlock);
}
