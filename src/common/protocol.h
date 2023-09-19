#pragma once

/*-----------------------------------------------------------------------------
 * _protocol.h
 *-----------------------------------------------------------------------------*/

 // Max number of history commands to send ( 2 by default ) in case of dropped packets
#define NUM_BACKUP_COMMAND_BITS		4 // Originally 3 bits.
#define MAX_BACKUP_COMMANDS			((1 << NUM_BACKUP_COMMAND_BITS)-1) // 15 in R5; see 'CL_Move'.

// Maximum amount of backup commands to process on the server.
#define MAX_BACKUP_COMMANDS_PROCESS (MAX_BACKUP_COMMANDS+1) * NUM_BACKUP_COMMAND_BITS
#define MAX_QUEUED_COMMANDS_PROCESS 0x1B0

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
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
	PERSISTENCE_READY     = 3  // persistence is ready for this client.
#else
	PERSISTENCE_READY     = 5  // persistence is ready for this client.
#endif
};

#define net_NOP        0 // nop command used for padding.
#define net_Disconnect 1 // disconnect, last message in connection.
