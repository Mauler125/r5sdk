//=====================================================================================//
// 
// Purpose: Lightweight netconsole client.
// 
//=====================================================================================//

#include "core/stdafx.h"
#include "core/termutil.h"
#include "tier1/NetAdr2.h"
#include "tier2/socketcreator.h"
#include "protoc/sv_rcon.pb.h"
#include "protoc/cl_rcon.pb.h"
#include "engine/net.h"
#include "netconsole/netconsole.h"

//-----------------------------------------------------------------------------
// Purpose: WSA and NETCON systems init
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::Init(void)
{
	WSAData wsaData{};
	int nError = ::WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (nError != 0)
	{
		std::cerr << "Failed to start Winsock via WSAStartup: (" << NET_ErrorString(WSAGetLastError()) << ")" << std::endl;
		return false;
	}

	this->TermSetup();

	std::thread tFrame(&CNetCon::RunFrame, this);
	tFrame.detach();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: WSA and NETCON systems shutdown
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::Shutdown(void)
{
	m_pSocket->CloseAllAcceptedSockets();
	m_abConnEstablished = false;

	int nError = ::WSACleanup();
	if (nError != 0)
	{
		std::cerr << "Failed to stop winsock via WSACleanup: (" << NET_ErrorString(WSAGetLastError()) << ")" << std::endl;
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: terminal setup
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
// Purpose: gets input IP and port for initialization
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
			if (strcmp(svInput.c_str(), "disconnect") == 0)
			{
				this->Disconnect();
				return;
			}
			size_t nPos = svInput.find(" ");
			if (!svInput.empty() 
				&& nPos > 0 
				&& nPos < svInput.size() 
				&& nPos != svInput.size())
			{
				std::string svReqVal = svInput.substr(nPos + 1);
				std::string svReqBuf = svInput.erase(svInput.find(" "));

				if (strcmp(svReqBuf.c_str(), "PASS") == 0) // Auth with RCON server.
				{
					std::string svSerialized = this->Serialize(svReqBuf, svReqVal, cl_rcon::request_t::SERVERDATA_REQUEST_AUTH);
					this->Send(svSerialized);
				}
				else // This is a ConVar.
				{
					std::string svSerialized = this->Serialize(svReqBuf, svReqVal, cl_rcon::request_t::SERVERDATA_REQUEST_SETVALUE);
					this->Send(svSerialized);
				}
			}
			else // This is a ConCommand.
			{
				std::string svSerialized = this->Serialize(svInput, "", cl_rcon::request_t::SERVERDATA_REQUEST_EXECCOMMAND);
				this->Send(svSerialized);
			}
		}
		else // Setup connection from input.
		{
			size_t nPos = svInput.find(" ");
			if (!svInput.empty() 
				&& nPos > 0 
				&& nPos < svInput.size() 
				&& nPos != svInput.size())
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
// Purpose: client's main processing loop
//-----------------------------------------------------------------------------
void CNetCon::RunFrame(void)
{
	for (;;)
	{
		if (m_abConnEstablished)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
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
// Purpose: checks if application should be terminated
// Output : true for termination, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::ShouldQuit(void) const
{
	return this->m_bQuitApplication;
}

//-----------------------------------------------------------------------------
// Purpose: connect to specified address and port
// Input  : *svInAdr - 
//			*svInPort - 
// Output : true if connection succeeds, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::Connect(const std::string& svInAdr, const std::string& svInPort)
{
	if (svInAdr.size() > 0 && svInPort.size() > 0)
	{
		// Default is [127.0.0.1]:37015
		m_pNetAdr2->SetIPAndPort(svInAdr, svInPort);
	}

	if (m_pSocket->ConnectSocket(*m_pNetAdr2, true) == SOCKET_ERROR)
	{
		std::cerr << "Failed to connect. Error: (SOCKET_ERROR). Verify IP and PORT." << std::endl;
		return false;
	}
	std::cout << "Connected to: " << m_pNetAdr2->GetIPAndPort() << std::endl;

	m_abConnEstablished = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: disconnect from current session
//-----------------------------------------------------------------------------
void CNetCon::Disconnect(void)
{
	::closesocket(m_pSocket->GetAcceptedSocketHandle(0));
	m_abPromptConnect   = true;
	m_abConnEstablished = false;
}

//-----------------------------------------------------------------------------
// Purpose: send message
// Input  : *svMessage - 
//-----------------------------------------------------------------------------
void CNetCon::Send(const std::string& svMessage) const
{
	int nSendResult = ::send(m_pSocket->GetAcceptedSocketData(0)->m_hSocket, svMessage.c_str(), svMessage.size(), MSG_NOSIGNAL);
	if (nSendResult == SOCKET_ERROR)
	{
		std::cout << "Failed to send message: (SOCKET_ERROR)" << std::endl;
	}
}

//-----------------------------------------------------------------------------
// Purpose: receive message
//-----------------------------------------------------------------------------
void CNetCon::Recv(void)
{
	static char szRecvBuf[MAX_NETCONSOLE_INPUT_LEN]{};

	{//////////////////////////////////////////////
		int nPendingLen = ::recv(m_pSocket->GetAcceptedSocketData(0)->m_hSocket, szRecvBuf, sizeof(szRecvBuf), MSG_PEEK);
		if (nPendingLen == SOCKET_ERROR && m_pSocket->IsSocketBlocking())
		{
			return;
		}
		if (nPendingLen <= 0 && m_abConnEstablished) // EOF or error.
		{
			this->Disconnect();
			std::cout << "Server closed connection" << std::endl;
			return;
		}
	}//////////////////////////////////////////////

	u_long nReadLen; // Find out how much we have to read.
	::ioctlsocket(m_pSocket->GetAcceptedSocketData(0)->m_hSocket, FIONREAD, &nReadLen);

	while (nReadLen > 0)
	{
		memset(szRecvBuf, '\0', sizeof(szRecvBuf));
		int nRecvLen = ::recv(m_pSocket->GetAcceptedSocketData(0)->m_hSocket, szRecvBuf, MIN(sizeof(szRecvBuf), nReadLen), MSG_NOSIGNAL);

		if (nRecvLen == 0 && m_abConnEstablished) // Socket was closed.
		{
			this->Disconnect();
			std::cout << "Server closed connection" << std::endl;
			break;
		}
		if (nRecvLen < 0 && !m_pSocket->IsSocketBlocking())
		{
			break;
		}

		nReadLen -= nRecvLen; // Process what we've got.
		this->ProcessBuffer(szRecvBuf, nRecvLen);
	}
}

//-----------------------------------------------------------------------------
// Purpose: handles input response buffer
// Input  : *pszIn - 
//			nRecvLen - 
//-----------------------------------------------------------------------------
void CNetCon::ProcessBuffer(const char* pszIn, int nRecvLen) const
{
	int nCharsInRespondBuffer = 0;
	char szInputRespondBuffer[MAX_NETCONSOLE_INPUT_LEN]{};

	while (nRecvLen)
	{
		switch (*pszIn)
		{
		case '\r':
		{
			if (nCharsInRespondBuffer)
			{
				sv_rcon::response sv_response = this->Deserialize(szInputRespondBuffer);
				this->ProcessMessage(sv_response);
			}
			nCharsInRespondBuffer = 0;
			break;
		}

		default:
		{
			if (nCharsInRespondBuffer < MAX_NETCONSOLE_INPUT_LEN - 1)
			{
				szInputRespondBuffer[nCharsInRespondBuffer++] = *pszIn;
			}
			break;
		}
		}
		pszIn++;
		nRecvLen--;
	}
}

//-----------------------------------------------------------------------------
// Purpose: processes received message
// Input  : *sv_response - 
//-----------------------------------------------------------------------------
void CNetCon::ProcessMessage(const sv_rcon::response& sv_response) const
{
	static std::regex rxAnsiExp("\\\033\\[.*?m");
	switch (sv_response.responsetype())
	{
		case sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH:
		case sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG:
		{
			std::string svOut = sv_response.responsebuf();
			if (m_bNoColor)
			{
				svOut = std::regex_replace(svOut, rxAnsiExp, "");
			}
			else
			{
				svOut.append(g_svReset.c_str());
			}
			std::cout << svOut.c_str();
			break;
		}
		default:
		{
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: serializes input
// Input  : *svReqBuf - 
//			*svReqVal - 
//			request_t - 
// Output : serialized results as string
//-----------------------------------------------------------------------------
std::string CNetCon::Serialize(const std::string& svReqBuf, const std::string& svReqVal, cl_rcon::request_t request_t) const
{
	cl_rcon::request cl_request;

	cl_request.set_requestid(-1);
	cl_request.set_requesttype(request_t);

	switch (request_t)
	{
		case cl_rcon::request_t::SERVERDATA_REQUEST_SETVALUE:
		case cl_rcon::request_t::SERVERDATA_REQUEST_AUTH:
		{
			cl_request.set_requestbuf(svReqBuf);
			cl_request.set_requestval(svReqVal);
			break;
		}
		case cl_rcon::request_t::SERVERDATA_REQUEST_EXECCOMMAND:
		{
			cl_request.set_requestbuf(svReqBuf);
			break;
		}
	}
	return cl_request.SerializeAsString().append("\r");
}

//-----------------------------------------------------------------------------
// Purpose: de-serializes input
// Input  : *svBuf - 
// Output : de-serialized object
//-----------------------------------------------------------------------------
sv_rcon::response CNetCon::Deserialize(const std::string& svBuf) const
{
	sv_rcon::response sv_response;
	sv_response.ParseFromArray(svBuf.c_str(), static_cast<int>(svBuf.size()));

	return sv_response;
}

//-----------------------------------------------------------------------------
// Purpose: entrypoint
// Input  : argc - 
//			*argv - 
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

	while (!pNetCon->ShouldQuit())
	{
		pNetCon->UserInput();
	}

	if (!pNetCon->Shutdown())
	{
		return EXIT_FAILURE;
	}

	return ERROR_SUCCESS;
}