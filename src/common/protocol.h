#pragma once

/*-----------------------------------------------------------------------------
 * _protocol.h
 *-----------------------------------------------------------------------------*/

 // Largest # of commands to send in a packet
#define NUM_NEW_COMMAND_BITS		4
#define MAX_NEW_COMMANDS			((1 << NUM_NEW_COMMAND_BITS)-1)

 // Max number of history commands to send ( 2 by default ) in case of dropped packets
#define NUM_BACKUP_COMMAND_BITS		3
#define MAX_BACKUP_COMMANDS			((1 << NUM_BACKUP_COMMAND_BITS)-1)

// Maximum amount of backup commands to process on the server.
#define MAX_BACKUP_COMMANDS_PROCESS 64
#define MAX_QUEUED_COMMANDS_PROCESS 432

// The size of the snapshot scratch buffer, which also applies to data block packets
#define SNAPSHOT_SCRATCH_BUFFER_SIZE 786432

enum class SIGNONSTATE : int
{
	SIGNONSTATE_NONE         = 0, // no state yet; about to connect.
	SIGNONSTATE_CHALLENGE    = 1, // client challenging server; all OOB packets.
	SIGNONSTATE_CONNECTED    = 2, // client is connected to server; netchans ready.
	SIGNONSTATE_NEW          = 3, // just got serverinfo and string tables.
	SIGNONSTATE_PRESPAWN     = 4, // received signon buffers.
	SIGNONSTATE_GETTING_DATA = 5, // getting persistence data.
	SIGNONSTATE_SPAWN        = 6, // ready to receive entity packets.
	SIGNONSTATE_FIRST_SNAP   = 7, // received baseline snapshot.
	SIGNONSTATE_FULL         = 8, // we are fully connected; first non-delta packet received.
	SIGNONSTATE_CHANGELEVEL  = 9, // server is changing level; please wait.
};

enum class PERSISTENCE : int
{
	PERSISTENCE_NONE      = 0, // no persistence data for this client yet.
	PERSISTENCE_PENDING   = 1, // pending or processing persistence data.
	PERSISTENCE_AVAILABLE = 2, // persistence is available for this client.
	PERSISTENCE_READY     = 5  // persistence is ready for this client.
};

#define net_NOP        0 // nop command used for padding.
#define net_Disconnect 1 // disconnect, last message in connection.

// each channel packet has 1 byte of FLAG bits
#define PACKET_FLAG_RELIABLE			(1<<0)	// packet contains subchannel stream data
#define PACKET_FLAG_COMPRESSED			(1<<1)	// packet is compressed
#define PACKET_FLAG_ENCRYPTED			(1<<2)  // packet is encrypted
#define PACKET_FLAG_SPLIT				(1<<3)  // packet is split
#define PACKET_FLAG_CHOKED				(1<<4)  // packet was choked by sender
#define PACKET_FLAG_PRESCALED			(1<<5)  // packet was sent by sender with prescaled frame time
#define PACKET_FLAG_LOOPBACK			(1<<6)  // packet was sent from loopback connection
