#include "serverlisting.h"
#include "overlay.h"
#include "httplib.h"

void ServerListing::Select()
{
	std::stringstream cmd;
	cmd << "connect " << this->ip;
	g_GameConsole->ProcessCommand(cmd.str().c_str());
}