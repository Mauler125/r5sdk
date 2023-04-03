//=====================================================================================//
// 
// Purpose: Lightweight netconsole client.
// 
//=====================================================================================//

#include "core/stdafx.h"
#include "core/termutil.h"
#include "core/logdef.h"
#include "tier1/NetAdr.h"
#include "tier2/socketcreator.h"
#include "windows/console.h"
#include "protoc/sv_rcon.pb.h"
#include "protoc/cl_rcon.pb.h"
#include "public/utility/utility.h"
#include "engine/net.h"
#include "netconsole/netconsole.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNetCon::CNetCon(void)
	: m_bInitialized(false)
	, m_bQuitApplication(false)
	, m_bPromptConnect(true)
	, m_bConnEstablished(false)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNetCon::~CNetCon(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: WSA and NETCON systems init
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::Init(void)
{
	WSAData wsaData;
	const int nError = ::WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (nError != 0)
	{
		Error(eDLL_T::NONE, NO_ERROR, "%s - Failed to start Winsock: (%s)\n", __FUNCTION__, NET_ErrorString(WSAGetLastError()));
		return false;
	}

	this->TermSetup();
	DevMsg(eDLL_T::NONE, "R5 TCP net console [Version %s]\n", NETCON_VERSION);

	static std::thread frame([this]()
		{
			for (;;)
			{
				this->RunFrame();
			}
		});
	frame.detach();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: WSA and NETCON systems shutdown
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::Shutdown(void)
{
	bool bResult = false;

	m_Socket.CloseAllAcceptedSockets();
	m_bConnEstablished = false;

	const int nError = ::WSACleanup();
	if (nError == 0)
	{
		bResult = true;
	}
	else // WSACleanup() failed.
	{
		Error(eDLL_T::NONE, NO_ERROR, "%s - Failed to stop Winsock: (%s)\n", __FUNCTION__, NET_ErrorString(WSAGetLastError()));
	}

	SpdLog_Shutdown();
	Console_Shutdown();

	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: terminal setup
//-----------------------------------------------------------------------------
void CNetCon::TermSetup(void)
{
	g_svCmdLine = GetCommandLineA();

	SpdLog_Init();
	SpdLog_PostInit();
	Console_Init();
}

//-----------------------------------------------------------------------------
// Purpose: gets input IP and port for initialization
//-----------------------------------------------------------------------------
void CNetCon::UserInput(void)
{
	if (std::getline(std::cin, m_Input))
	{
		if (m_Input.compare("nquit") == 0)
		{
			m_bQuitApplication = true;
			return;
		}

		std::lock_guard<std::mutex> l(m_Mutex);
		if (m_bConnEstablished)
		{
			if (m_Input.compare("disconnect") == 0)
			{
				this->Disconnect();
				return;
			}

			const std::vector<std::string> vSubStrings = StringSplit(m_Input, ' ', 2);
			if (vSubStrings.size() > 1)
			{
				if (vSubStrings[0].compare("PASS") == 0) // Auth with RCON server.
				{
					std::string svSerialized = this->Serialize(vSubStrings[1], "", cl_rcon::request_t::SERVERDATA_REQUEST_AUTH);
					this->Send(svSerialized);
				}
				else if (vSubStrings[0].compare("SET") == 0) // Set value query.
				{
					if (vSubStrings.size() > 2)
					{
						std::string svSerialized = this->Serialize(vSubStrings[1], vSubStrings[2], cl_rcon::request_t::SERVERDATA_REQUEST_SETVALUE);
						this->Send(svSerialized);
					}
				}
				else // Execute command query.
				{
					std::string svSerialized = this->Serialize(m_Input, "", cl_rcon::request_t::SERVERDATA_REQUEST_EXECCOMMAND);
					this->Send(svSerialized);
				}
			}
			else if (!m_Input.empty()) // Single arg command query.
			{
				std::string svSerialized = this->Serialize(m_Input, "", cl_rcon::request_t::SERVERDATA_REQUEST_EXECCOMMAND);
				this->Send(svSerialized);
			}
		}
		else // Setup connection from input.
		{
			const std::vector<std::string> vSubStrings = StringSplit(m_Input, ' ', 2);
			if (vSubStrings.size() > 1)
			{
				const string::size_type nPos = m_Input.find(' ');
				if (nPos > 0
					&& nPos < m_Input.size()
					&& nPos != m_Input.size())
				{
					std::string svInPort = m_Input.substr(nPos + 1);
					std::string svInAdr = m_Input.erase(m_Input.find(' '));

					if (!this->Connect(svInAdr, svInPort))
					{
						m_bPromptConnect = true;
						return;
					}
				}
			}
			else // Initialize as [ip]:port.
			{
				if (!this->Connect(m_Input, ""))
				{
					m_bPromptConnect = true;
					return;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: clears the input buffer
//-----------------------------------------------------------------------------
void CNetCon::ClearInput(void)
{
	m_Input.clear();
}

//-----------------------------------------------------------------------------
// Purpose: client's main processing loop
//-----------------------------------------------------------------------------
void CNetCon::RunFrame(void)
{
	if (m_bConnEstablished)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		std::lock_guard<std::mutex> l(m_Mutex);

		this->Recv();
	}
	else if (m_bPromptConnect)
	{
		DevMsg(eDLL_T::NONE, "Enter [<IP>]:<PORT> or <IP> <PORT>: ");
		m_bPromptConnect = false;
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
	if (!svInAdr.empty() && !svInPort.empty()) // Construct from ip port
	{
		string svLocalHost;
		if (svInAdr.compare("localhost") == 0)
		{
			char szHostName[512];
			if (!gethostname(szHostName, sizeof(szHostName)))
			{
				svLocalHost = szHostName;
			}
		}

		const string svFull = Format("[%s]:%s", svLocalHost.empty() ? svInAdr.c_str() : svLocalHost.c_str(), svInPort.c_str());
		if (!m_Address.SetFromString(svFull.c_str(), true))
		{
			Warning(eDLL_T::CLIENT, "Failed to set RCON address: %s\n", svFull.c_str());
		}
	}
	else if (!svInAdr.empty()) // construct from [ip]:port
	{
		if (!m_Address.SetFromString(svInAdr.c_str(), true))
		{
			Warning(eDLL_T::CLIENT, "Failed to set RCON address: %s\n", svInAdr.c_str());
		}
	}
	else
	{
		Warning(eDLL_T::NONE, "No IP address provided\n");
		return false;
	}

	if (m_Socket.ConnectSocket(m_Address, true) == SOCKET_ERROR)
	{
		Error(eDLL_T::NONE, NO_ERROR, "Failed to connect: error(%s); verify IP and PORT\n", "SOCKET_ERROR");
		return false;
	}
	DevMsg(eDLL_T::NONE, "Connected to: %s\n", m_Address.ToString());

	m_bConnEstablished = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: disconnect from current session
//-----------------------------------------------------------------------------
void CNetCon::Disconnect(void)
{
	m_Socket.CloseAcceptedSocket(0);
	m_bPromptConnect = true;
	m_bConnEstablished = false;
}

//-----------------------------------------------------------------------------
// Purpose: send message
// Input  : *svMessage - 
//-----------------------------------------------------------------------------
void CNetCon::Send(const std::string& svMessage) const
{
	std::ostringstream ssSendBuf;
	const u_long nLen = htonl(u_long(svMessage.size()));

	ssSendBuf.write(reinterpret_cast<const char*>(&nLen), sizeof(u_long));
	ssSendBuf.write(svMessage.data(), svMessage.size());

	const int nSendResult = ::send(m_Socket.GetAcceptedSocketData(0)->m_hSocket,
		ssSendBuf.str().data(), static_cast<int>(ssSendBuf.str().size()), MSG_NOSIGNAL);
	if (nSendResult == SOCKET_ERROR)
	{
		Error(eDLL_T::NONE, NO_ERROR, "Failed to send message (%s)\n", "SOCKET_ERROR");
	}
}

//-----------------------------------------------------------------------------
// Purpose: receive message
//-----------------------------------------------------------------------------
void CNetCon::Recv(void)
{
	static char szRecvBuf[1024];
	CConnectedNetConsoleData* pData = m_Socket.GetAcceptedSocketData(0);

	{//////////////////////////////////////////////
		const int nPendingLen = ::recv(pData->m_hSocket, szRecvBuf, sizeof(char), MSG_PEEK);
		if (nPendingLen == SOCKET_ERROR && m_Socket.IsSocketBlocking())
		{
			return;
		}
		if (nPendingLen <= 0 && m_bConnEstablished) // EOF or error.
		{
			this->Disconnect();
			DevMsg(eDLL_T::NONE, "Server closed connection\n");
			return;
		}
	}//////////////////////////////////////////////

	u_long nReadLen; // Find out how much we have to read.
	::ioctlsocket(pData->m_hSocket, FIONREAD, &nReadLen);

	while (nReadLen > 0)
	{
		const int nRecvLen = ::recv(pData->m_hSocket, szRecvBuf, MIN(sizeof(szRecvBuf), nReadLen), MSG_NOSIGNAL);
		if (nRecvLen == 0 && m_bConnEstablished) // Socket was closed.
		{
			this->Disconnect();
			DevMsg(eDLL_T::NONE, "Server closed connection\n");
			break;
		}
		if (nRecvLen < 0 && !m_Socket.IsSocketBlocking())
		{
			Error(eDLL_T::NONE, NO_ERROR, "RCON Cmd: recv error (%s)\n", NET_ErrorString(WSAGetLastError()));
			break;
		}

		nReadLen -= nRecvLen; // Process what we've got.
		this->ProcessBuffer(szRecvBuf, nRecvLen, pData);
	}
}

//-----------------------------------------------------------------------------
// Purpose: parses input response buffer using length-prefix framing
// Input  : *pRecvBuf - 
//			nRecvLen - 
//			*pData - 
//-----------------------------------------------------------------------------
void CNetCon::ProcessBuffer(const char* pRecvBuf, int nRecvLen, CConnectedNetConsoleData* pData)
{
	while (nRecvLen > 0)
	{
		if (pData->m_nPayloadLen)
		{
			if (pData->m_nPayloadRead < pData->m_nPayloadLen)
			{
				pData->m_RecvBuffer[pData->m_nPayloadRead++] = *pRecvBuf;

				pRecvBuf++;
				nRecvLen--;
			}
			if (pData->m_nPayloadRead == pData->m_nPayloadLen)
			{
				this->ProcessMessage(this->Deserialize(std::string(
					reinterpret_cast<char*>(pData->m_RecvBuffer.data()), pData->m_nPayloadLen)));

				pData->m_nPayloadLen = 0;
				pData->m_nPayloadRead = 0;
			}
		}
		else if (pData->m_nPayloadRead < sizeof(int)) // Read size field.
		{
			pData->m_RecvBuffer[pData->m_nPayloadRead++] = *pRecvBuf;

			pRecvBuf++;
			nRecvLen--;
		}
		else // Build prefix.
		{
			pData->m_nPayloadLen = ntohl(*reinterpret_cast<u_long*>(&pData->m_RecvBuffer[0]));
			pData->m_nPayloadRead = 0;

			if (pData->m_nPayloadLen < 0 ||
				pData->m_nPayloadLen > pData->m_RecvBuffer.max_size())
			{
				Error(eDLL_T::NONE, NO_ERROR, "RCON Cmd: sync error (%zu)\n", pData->m_nPayloadLen);
				this->Disconnect(); // Out of sync (irrecoverable).

				break;
			}
			else
			{
				pData->m_RecvBuffer.resize(pData->m_nPayloadLen);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: processes received message
// Input  : *sv_response - 
//-----------------------------------------------------------------------------
void CNetCon::ProcessMessage(const sv_rcon::response& sv_response) const
{
	switch (sv_response.responsetype())
	{
	case sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH:
	{
		if (!sv_response.responseval().empty())
		{
			const long i = strtol(sv_response.responseval().c_str(), NULL, NULL);
			if (!i) // sv_rcon_sendlogs is not set.
			{
				string svLogQuery = this->Serialize("", "", cl_rcon::request_t::SERVERDATA_REQUEST_SEND_CONSOLE_LOG);
				this->Send(svLogQuery);
			}
		}

		DevMsg(eDLL_T::NETCON, "%s", PrintPercentageEscape(sv_response.responsemsg()).c_str());
		break;
	}
	case sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG:
	{
		NetMsg(static_cast<LogType_t>(sv_response.messagetype()),
			static_cast<eDLL_T>(sv_response.messageid()), sv_response.responseval().c_str(),
			PrintPercentageEscape(sv_response.responsemsg()).c_str());
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
std::string CNetCon::Serialize(const std::string& svReqBuf, const std::string& svReqVal, const cl_rcon::request_t request_t) const
{
	cl_rcon::request cl_request;

	cl_request.set_messageid(-1);
	cl_request.set_requesttype(request_t);

	switch (request_t)
	{
	case cl_rcon::request_t::SERVERDATA_REQUEST_SETVALUE:
	case cl_rcon::request_t::SERVERDATA_REQUEST_AUTH:
	{
		cl_request.set_requestmsg(svReqBuf);
		cl_request.set_requestval(svReqVal);
		break;
	}
	case cl_rcon::request_t::SERVERDATA_REQUEST_EXECCOMMAND:
	{
		cl_request.set_requestmsg(svReqBuf);
		break;
	}
	}
	return cl_request.SerializeAsString();
}

//-----------------------------------------------------------------------------
// Purpose: de-serializes input
// Input  : *svBuf - 
// Output : de-serialized object
//-----------------------------------------------------------------------------
sv_rcon::response CNetCon::Deserialize(const std::string& svBuf) const
{
	sv_rcon::response sv_response;
	sv_response.ParseFromArray(svBuf.data(), static_cast<int>(svBuf.size()));

	return sv_response;
}

//-----------------------------------------------------------------------------
// Purpose: entrypoint
// Input  : argc - 
//			*argv - 
//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	if (!NetConsole()->Init())
	{
		return EXIT_FAILURE;
	}

	if (argc >= 3) // Get IP and Port from command line.
	{
		if (!NetConsole()->Connect(argv[1], argv[2]))
		{
			return EXIT_FAILURE;
		}
	}

	while (!NetConsole()->ShouldQuit())
	{
		NetConsole()->UserInput();
		NetConsole()->ClearInput();
	}

	if (!NetConsole()->Shutdown())
	{
		return EXIT_FAILURE;
	}

	return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------
// singleton
//-----------------------------------------------------------------------------
CNetCon g_NetCon;
inline CNetCon* NetConsole()
{
	return &g_NetCon;
}