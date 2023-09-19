//=====================================================================================//
// 
// Purpose: Lightweight netconsole client.
// 
//=====================================================================================//

#include "core/stdafx.h"
#include "core/logdef.h"
#include "core/logger.h"
#include "tier0/utility.h"
#include "tier1/NetAdr.h"
#include "tier2/socketcreator.h"
#include "windows/console.h"
#include "protoc/sv_rcon.pb.h"
#include "protoc/cl_rcon.pb.h"
#include "engine/net.h"
#include "engine/shared/shared_rcon.h"
#include "netconsole/netconsole.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNetCon::CNetCon(void)
	: m_bInitialized(false)
	, m_bQuitApplication(false)
	, m_bPromptConnect(true)
	, m_flTickInterval(0.05f)
{
	// Empty character set used for ip addresses if we still need to initiate a
	// connection, as we don't want to break on ':' characters found in an IPv6
	// address.
	CharacterSetBuild(&m_CharacterSet, "");
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
bool CNetCon::Init(const bool bAnsiColor)
{
	g_CoreMsgVCallback = &EngineLoggerSink;

	WSAData wsaData;
	const int nError = ::WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (nError != 0)
	{
		Error(eDLL_T::CLIENT, NO_ERROR, "%s - Failed to start Winsock: (%s)\n", __FUNCTION__, NET_ErrorString(WSAGetLastError()));
		return false;
	}

	m_bInitialized = true;

	TermSetup(bAnsiColor);
	Msg(eDLL_T::NONE, "R5 TCP net console [Version %s]\n", NETCON_VERSION);

	static std::thread frame([this]()
		{
			for (;;)
			{
				RunFrame();
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

	const int nError = ::WSACleanup();
	if (nError == 0)
	{
		bResult = true;
	}
	else // WSACleanup() failed.
	{
		Error(eDLL_T::CLIENT, NO_ERROR, "%s - Failed to stop Winsock: (%s)\n",
			__FUNCTION__, NET_ErrorString(WSAGetLastError()));
	}

	SpdLog_Shutdown();
	Console_Shutdown();

	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: terminal setup
//-----------------------------------------------------------------------------
void CNetCon::TermSetup(const bool bAnsiColor)
{
	SpdLog_Init(bAnsiColor);
	Console_Init(bAnsiColor);
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

		if (IsConnected())
		{
			CCommand cmd;
			cmd.Tokenize(m_Input.c_str());

			if (V_strcmp(cmd.Arg(0), "disconnect") == 0)
			{
				Disconnect("user closed connection");
				return;
			}

			vector<char> vecMsg;

			const SocketHandle_t hSocket = GetSocket();
			bool bSend = false;

			if (cmd.ArgC() > 1)
			{
				if (V_strcmp(cmd.Arg(0), "PASS") == 0) // Auth with RCON server.
				{
					bSend = Serialize(vecMsg, cmd.Arg(1), "",
						cl_rcon::request_t::SERVERDATA_REQUEST_AUTH);
				}
				else // Execute command query.
				{
					bSend = Serialize(vecMsg, cmd.Arg(0), cmd.GetCommandString(),
						cl_rcon::request_t::SERVERDATA_REQUEST_EXECCOMMAND);
				}
			}
			else if (!m_Input.empty()) // Single arg command query.
			{
				bSend = Serialize(vecMsg, m_Input.c_str(), "", cl_rcon::request_t::SERVERDATA_REQUEST_EXECCOMMAND);
			}

			if (bSend) // Only send if serialization process was successful.
			{
				if (!Send(hSocket, vecMsg.data(), int(vecMsg.size())))
				{
					Error(eDLL_T::CLIENT, NO_ERROR, "Failed to send RCON message: (%s)\n", "SOCKET_ERROR");
				}
			}
		}
		else // Setup connection from input.
		{
			CCommand cmd;
			cmd.Tokenize(m_Input.c_str(), cmd_source_t::kCommandSrcCode, &m_CharacterSet);

			if (cmd.ArgC() > 1)
			{
				const char* inAddr = cmd.Arg(0);
				const char* inPort = cmd.Arg(1);

				if (!*inAddr || !*inPort)
				{
					Warning(eDLL_T::CLIENT, "No IP address or port provided\n");
					m_bPromptConnect = true;
					return;
				}

				if (!Connect(inAddr, atoi(inPort)))
				{
					m_bPromptConnect = true;
					return;
				}
			}
			else
			{
				if (!Connect(cmd.GetCommandString()))
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
	if (IsInitialized() && IsConnected())
	{
		std::lock_guard<std::mutex> l(m_Mutex);

		CConnectedNetConsoleData& pData = m_Socket.GetAcceptedSocketData(0);
		Recv(pData);
	}
	else if (m_bPromptConnect)
	{
		Msg(eDLL_T::NONE, "Enter [<IP>]:<PORT> or <IP> <PORT>: ");
		m_bPromptConnect = false;
	}

	std::this_thread::sleep_for(IntervalToDuration(m_flTickInterval));
}

//-----------------------------------------------------------------------------
// Purpose: checks if application should be terminated
// Output : true for termination, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::ShouldQuit(void) const
{
	return m_bQuitApplication;
}

//-----------------------------------------------------------------------------
// Purpose: disconnect from current session
// Input  : *szReason - 
//-----------------------------------------------------------------------------
void CNetCon::Disconnect(const char* szReason)
{
	if (IsConnected())
	{
		if (!szReason)
		{
			szReason = "unknown reason";
		}

		Msg(eDLL_T::CLIENT, "Disconnect: (%s)\n", szReason);
		m_Socket.CloseAcceptedSocket(0);
	}

	m_bPromptConnect = true;
}

//-----------------------------------------------------------------------------
// Purpose: processes received message
// Input  : *pMsgBuf - 
//			nMsgLen - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::ProcessMessage(const char* pMsgBuf, const int nMsgLen)
{
	sv_rcon::response response;
	bool bSuccess = Decode(&response, pMsgBuf, nMsgLen);

	if (!bSuccess)
	{
		Error(eDLL_T::CLIENT, NO_ERROR, "Failed to decode RCON buffer\n");
		return false;
	}

	switch (response.responsetype())
	{
	case sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH:
	{
		if (!response.responseval().empty())
		{
			const long i = strtol(response.responseval().c_str(), NULL, NULL);
			if (!i) // Means we are marked 'input only' on the rcon server.
			{
				vector<char> vecMsg;
				bool ret = Serialize(vecMsg, "", "1", cl_rcon::request_t::SERVERDATA_REQUEST_SEND_CONSOLE_LOG);

				if (ret && !Send(GetSocket(), vecMsg.data(), int(vecMsg.size())))
				{
					Error(eDLL_T::CLIENT, NO_ERROR, "Failed to send RCON message: (%s)\n", "SOCKET_ERROR");
				}
			}
		}

		Msg(eDLL_T::NETCON, "%s", response.responsemsg().c_str());
		break;
	}
	case sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG:
	{
		NetMsg(static_cast<LogType_t>(response.messagetype()),
			static_cast<eDLL_T>(response.messageid()),
			response.responseval().c_str(), "%s", response.responsemsg().c_str());
		break;
	}
	default:
	{
		break;
	}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: serializes message to vector
// Input  : &vecBuf - 
//			*szReqBuf - 
//			*svReqVal - 
//			requestType - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::Serialize(vector<char>& vecBuf, const char* szReqBuf,
	const char* szReqVal, const cl_rcon::request_t requestType) const
{
	return CL_NetConSerialize(this, vecBuf, szReqBuf, szReqVal, requestType);
}

//-----------------------------------------------------------------------------
// Purpose: retrieves the remote socket
// Output : SOCKET_ERROR (-1) on failure
//-----------------------------------------------------------------------------
SocketHandle_t CNetCon::GetSocket(void)
{
	return SH_GetNetConSocketHandle(this, 0);
}

//-----------------------------------------------------------------------------
// Purpose: returns whether the rcon client is initialized
//-----------------------------------------------------------------------------
bool CNetCon::IsInitialized(void) const
{
	return m_bInitialized;
}

//-----------------------------------------------------------------------------
// Purpose: returns whether the rcon client is connected
//-----------------------------------------------------------------------------
bool CNetCon::IsConnected(void)
{
	return (GetSocket() != SOCKET_ERROR);
}

//-----------------------------------------------------------------------------
// Purpose: entrypoint
// Input  : argc - 
//			*argv - 
//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	bool bEnableColor = false;

	for (int i = 0; i < argc; i++)
	{
		if (V_strcmp(argv[i], "-ansicolor") == NULL)
		{
			bEnableColor = true;
			break;
		}
	}

	if (!NetConsole()->Init(bEnableColor))
	{
		return EXIT_FAILURE;
	}

	if (argc >= 3) // Get IP and Port from command line.
	{
		if (!NetConsole()->Connect(argv[1], atoi(argv[2])))
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