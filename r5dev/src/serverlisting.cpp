#include "pch.h"
#include "serverlisting.h"
#include "overlay.h"

void ServerListing::Select()
{
	std::stringstream cmd;
	cmd << "connect " << this->ip;
	g_GameConsole->ProcessCommand(cmd.str().c_str());
}