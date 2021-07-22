#include "pch.h"
#include "serverlisting.h"
#include "overlay.h"

void ServerListing::Select()
{
	std::stringstream cmd;
	cmd << "connect " << this->ip << ":" << this->port;
	g_ServerBrowser->ProcessCommand(cmd.str().c_str());
}