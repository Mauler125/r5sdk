//=====================================================================================//
//
// Purpose: Lightweight netconsole.
//
//=====================================================================================//

#include "core/stdafx.h"
#include "tier1/NetAdr2.h"
#include "tier2/socketcreator.h"
#include "netconsole/netconsole.h"

//-----------------------------------------------------------------------------
// purpose: send datagram
//-----------------------------------------------------------------------------
void CNetCon::Send(void)
{
	char buf[MAX_NETCONSOLE_INPUT_LEN]{};
	std::string svUserInput;

	do
	{
		printf(">");
		std::getline(std::cin, svUserInput);
		svUserInput.append("\n\r");

		int nSendResult = ::send(pSocket->GetAcceptedSocketData(0)->m_hSocket, svUserInput.c_str(), svUserInput.size(), MSG_NOSIGNAL);

		if (nSendResult != SOCKET_ERROR)
		{
			memcpy(buf, "", MAX_NETCONSOLE_INPUT_LEN);
		}
	} while (svUserInput.size() > 0);
}

//-----------------------------------------------------------------------------
// purpose: receive datagram
//-----------------------------------------------------------------------------
void CNetCon::Recv(void)
{
	static char buf[MAX_NETCONSOLE_INPUT_LEN]{};

	for (;;)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		int nRecvLen = ::recv(pSocket->GetAcceptedSocketData(0)->m_hSocket, buf, sizeof(buf), MSG_NOSIGNAL);
		if (nRecvLen > 0 && nRecvLen < MAX_NETCONSOLE_INPUT_LEN - 1)
		{
			buf[nRecvLen + 1] = '\0';
			printf("%s\n", buf);
		}
	}
}

//-----------------------------------------------------------------------------
// purpose: WSA and NETCON systems init
//-----------------------------------------------------------------------------
bool CNetCon::Init(void)
{
	WSAData wsaData{};
	int nError = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nError != 0)
	{
		assert(nError != 0 && "Failed to start Winsock via WSAStartup.");
		return false;
	}

	if (pSocket->ConnectSocket(*pNetAdr2, true) == SOCKET_ERROR)
	{
		assert(nError != 0 && "'pSocket->ConnectSocket()' returned 'SOCKET_ERROR'");
		return false;
	}

	std::thread tRecv(&CNetCon::Recv, this, this->pSocket);
	tRecv.detach();

	return true;
}

//-----------------------------------------------------------------------------
// purpose: WSA and NETCON systems shutdown
//-----------------------------------------------------------------------------
bool CNetCon::Shutdown(void)
{
	pSocket->CloseAllAcceptedSockets();

	int nError = ::WSACleanup();
	if (nError != 0)
	{
		assert(nError != 0 && "Failed to stop winsock via WSACleanup.\n");
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// purpose: entrypoint
//-----------------------------------------------------------------------------
int main(void)
{
	CNetCon* pNetCon = new CNetCon();

	if (!pNetCon->Init())
	{
		return EXIT_FAILURE;
	}

	pNetCon->Send();

	if (!pNetCon->Shutdown())
	{
		return EXIT_FAILURE;
	}

	return ERROR_SUCCESS;
}