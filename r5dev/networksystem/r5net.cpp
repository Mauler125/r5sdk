// r5net.cpp : Defines the functions for the static library.
//

#include "core/stdafx.h"
#include <networksystem/r5net.h>

namespace R5Net
{
	NetGameServer* LocalServer = new R5Net::NetGameServer{};
}
R5Net::Client* g_pR5net = new R5Net::Client("127.0.0.1:3000");
