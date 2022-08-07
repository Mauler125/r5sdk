#pragma once

/*-----------------------------------------------------------------------------
 * _protocol.h
 *-----------------------------------------------------------------------------*/

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
