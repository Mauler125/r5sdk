#include "pch.h"
#include "serverlisting.h"
#include "CCompanion.h"

void ServerListing::Select()
{
	std::stringstream cmd;
	cmd << "connect " << this->ip << ":" << this->port;
	g_ServerBrowser->ProcessCommand(cmd.str().c_str());
}