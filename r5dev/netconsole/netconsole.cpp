//=====================================================================================//
//
// Purpose: Lightweight netconsole client.
//
//=====================================================================================//

#include "core/stdafx.h"
#include "core/termutil.h"
#include "tier1/NetAdr2.h"
#include "tier2/socketcreator.h"
#include "netconsole/netconsole.h"

//-----------------------------------------------------------------------------
// purpose: WSA and NETCON systems init
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::Init(void)
{
	WSAData wsaData{};
	int nError = ::WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (nError != 0)
	{
		std::cerr << "Failed to start Winsock via WSAStartup. Error: " << nError << std::endl;
		return false;
	}

	this->TermSetup();

	std::thread tFrame(&CNetCon::RunFrame, this);
	tFrame.detach();

	return true;
}

//-----------------------------------------------------------------------------
// purpose: connect to specified address and port
// Input  : svInAdr - 
//			svInPort - 
// Output : true if connection succeeds, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::Connect(std::string svInAdr, std::string svInPort)
{
	if (svInAdr.size() > 0 && svInPort.size() > 0)
	{
		// Default is [127.0.0.1]:37015
		m_pNetAdr2->SetIPAndPort(svInAdr, svInPort);
	}

	if (m_pSocket->ConnectSocket(*m_pNetAdr2, true) == SOCKET_ERROR)
	{
		std::cerr << "Failed to connect. Error: 'SOCKET_ERROR'. Verify IP and PORT." << std::endl;
		return false;
	}
	std::cout << "Connected to: " << m_pNetAdr2->GetIPAndPort() << std::endl;

	m_abConnEstablished = true;
	return true;
}

//-----------------------------------------------------------------------------
// purpose: WSA and NETCON systems shutdown
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::Shutdown(void)
{
	m_pSocket->CloseAllAcceptedSockets();
	m_abConnEstablished = false;

	int nError = ::WSACleanup();
	if (nError != 0)
	{
		std::cerr << "Failed to stop winsock via WSACleanup. Error: " << nError << std::endl;
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// purpose: terminal setup
//-----------------------------------------------------------------------------
void CNetCon::TermSetup(void)
{
	DWORD dwMode = NULL;
	HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);

	if (!strstr(GetCommandLineA(), "-nocolor"))
	{
		GetConsoleMode(hOutput, &dwMode);
		dwMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;

		if (!SetConsoleMode(hOutput, dwMode)) // Some editions of Windows have 'VirtualTerminalLevel' disabled by default.
		{
			// Warn the user if 'VirtualTerminalLevel' could not be set on users environment.
			MessageBox(NULL, "Failed to set console mode 'VirtualTerminalLevel'.\n"
				"Restart the net console with the '-nocolor'\n"
				"parameter if output logging appears distorted.", "SDK Warning",
				MB_ICONEXCLAMATION | MB_OK);
		}
		AnsiColors_Init();
	}
	else
	{
		m_bNoColor = true;
	}

	CONSOLE_SCREEN_BUFFER_INFOEX sbInfoEx{};
	COLORREF storedBG = sbInfoEx.ColorTable[0];
	sbInfoEx.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);

	GetConsoleScreenBufferInfoEx(hOutput, &sbInfoEx);
	sbInfoEx.ColorTable[0] = 0x0000;
	SetConsoleScreenBufferInfoEx(hOutput, &sbInfoEx);
}

//-----------------------------------------------------------------------------
// purpose: client's main processing loop
//-----------------------------------------------------------------------------
void CNetCon::RunFrame(void)
{
	for (;;)
	{
		if (m_abConnEstablished)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			this->Recv();
		}
		else if (m_abPromptConnect)
		{
			std::cout << "Enter <IP> <PORT>: ";
			m_abPromptConnect = false;
		}
	}
}

//-----------------------------------------------------------------------------
// purpose: gets input IP and port for initialization
//-----------------------------------------------------------------------------
void CNetCon::UserInput(void)
{
	std::string svInput;

	if (std::getline(std::cin, svInput))
	{
		if (strcmp(svInput.c_str(), "nquit") == 0)
		{
			m_bQuitApplication = true;
			return;
		}

		if (m_abConnEstablished)
		{
			this->Send(svInput);
		}
		else // Setup connection from input.
		{
			size_t nPos = svInput.find(" ");
			if (!svInput.empty() && nPos > 0 && nPos < svInput.size())
			{
				std::string svInPort = svInput.substr(nPos + 1);
				std::string svInAdr = svInput.erase(svInput.find(" "));

				if (!this->Connect(svInAdr, svInPort))
				{
					m_abPromptConnect = true;
					return;
				}
			}
			else // Initialize as [127.0.0.1]:37015.
			{

				if (!this->Connect("", ""))
				{
					m_abPromptConnect = true;
					return;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// purpose: send message
//-----------------------------------------------------------------------------
void CNetCon::Send(std::string svMessage)
{
	svMessage.append("\n\r");
	int nSendResult = ::send(m_pSocket->GetAcceptedSocketData(0)->m_hSocket, svMessage.c_str(), svMessage.size(), MSG_NOSIGNAL);
}

//-----------------------------------------------------------------------------
// purpose: receive message
//-----------------------------------------------------------------------------
void CNetCon::Recv(void)
{
	static char szRecvBuf[MAX_NETCONSOLE_INPUT_LEN]{};
	static std::regex rxAnsiExp("\\\033\\[.*?m");
	static std::string svOut;

	{//////////////////////////////////////////////
		char szRecvBuf{};
		int nPendingLen = ::recv(m_pSocket->GetAcceptedSocketData(0)->m_hSocket, &szRecvBuf, sizeof(szRecvBuf), MSG_PEEK);

		if (nPendingLen == SOCKET_ERROR && m_pSocket->IsSocketBlocking())
		{
			return;
		}
		if (nPendingLen <= 0) // EOF or error.
		{
			::closesocket(m_pSocket->GetAcceptedSocketHandle(0));
			std::cout << "Server closed connection." << std::endl;

			m_abPromptConnect = true;
			m_abConnEstablished = false;
			return;
		}
	}//////////////////////////////////////////////

	u_long nReadLen; // Find out how much we have to read.
	::ioctlsocket(m_pSocket->GetAcceptedSocketData(0)->m_hSocket, FIONREAD, &nReadLen);

	if (nReadLen > 0 && nReadLen < MAX_NETCONSOLE_INPUT_LEN - 1)
	{
		int nRecvLen = ::recv(m_pSocket->GetAcceptedSocketData(0)->m_hSocket, szRecvBuf, sizeof(szRecvBuf), MSG_NOSIGNAL);
		svOut = szRecvBuf;

		if (m_bNoColor)
		{
			svOut = std::regex_replace(svOut, rxAnsiExp, "");
		}
		else
		{
			svOut.append(g_svReset.c_str());
		}
		std::cout << svOut.c_str();
		memset(szRecvBuf, '\0', sizeof(szRecvBuf));
	}
}

//-----------------------------------------------------------------------------
// purpose: entrypoint
//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	CNetCon* pNetCon = new CNetCon();
	std::cout << "R5Reloaded TCP net console [Version " << NETCON_VERSION << "]" << std::endl;

	if (!pNetCon->Init())
	{
		return EXIT_FAILURE;
	}

	if (argc >= 3) // Get IP and Port from command line.
	{
		if (!pNetCon->Connect(argv[1], argv[2]))
		{
			return EXIT_FAILURE;
		}
	}

	while (!pNetCon->m_bQuitApplication)
	{
		pNetCon->UserInput();
	}

	if (!pNetCon->Shutdown())
	{
		return EXIT_FAILURE;
	}

	return ERROR_SUCCESS;
}