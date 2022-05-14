#ifndef DATABLOCK_H
#define DATABLOCK_H

struct __declspec(align(8)) NetDataBlockReceiver
{
	void* __vftable /*VFT*/;
	void* client; /*CClientState*/
	char m_bStartedRecv;
	char m_bCompletedRecv;
	_BYTE byte12;
	__int16 transfer_id;
	__int16 counter;
	_BYTE m_bInitialized;
	int transfer_size;
	int total_blocks;
	_DWORD blocks_ackd;
	double start_time;
	bool block_status[768];
	char* m_pBigBuffer;
};


#endif // DATABLOCK_H