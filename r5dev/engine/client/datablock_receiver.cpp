//===========================================================================//
// 
// Purpose: client side datablock receiver
// 
//===========================================================================//
#include "engine/client/clientstate.h"
#include "datablock_receiver.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientDataBlockReceiver::~ClientDataBlockReceiver()
{
	v_ClientDataBlockReceiver__Destructor(this);
}

//-----------------------------------------------------------------------------
// Purpose: send an ack back to the server to let them know
// we received the datablock
//-----------------------------------------------------------------------------
void ClientDataBlockReceiver::AcknowledgeTransmission()
{
	v_ClientDataBlockReceiver__AcknowledgeTransmission(this);
}
