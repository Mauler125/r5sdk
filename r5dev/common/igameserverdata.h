//===========================================================================//
//
// Purpose: Enumerations for writing out the requests.
//
//===========================================================================//
#pragma once

typedef int SocketHandle_t;

enum class ServerDataRequestType_t : int
{
	SERVERDATA_REQUESTVALUE = 0,
	SERVERDATA_SETVALUE,
	SERVERDATA_EXECCOMMAND,
	SERVERDATA_AUTH,
	SERVERDATA_VPROF,
	SERVERDATA_REMOVE_VPROF,
	SERVERDATA_TAKE_SCREENSHOT,
	SERVERDATA_SEND_CONSOLE_LOG,
	SERVERDATA_SEND_REMOTEBUG,
};

enum class ServerDataResponseType_t : int
{
	SERVERDATA_RESPONSE_VALUE = 0,
	SERVERDATA_UPDATE,
	SERVERDATA_AUTH_RESPONSE,
	SERVERDATA_VPROF_DATA,
	SERVERDATA_VPROF_GROUPS,
	SERVERDATA_SCREENSHOT_RESPONSE,
	SERVERDATA_CONSOLE_LOG_RESPONSE,
	SERVERDATA_RESPONSE_STRING,
	SERVERDATA_RESPONSE_REMOTEBUG,
};

class CConnectedNetConsoleData
{
public:
	SocketHandle_t m_hSocket                               {};
	int  m_nCharsInCommandBuffer                           {};
	char m_pszInputCommandBuffer[MAX_NETCONSOLE_INPUT_LEN] {};
	bool m_bAuthorized                                     {}; // Set to true after netconsole successfully authed.
	bool m_bInputOnly                                      {}; // If set, don't send spew to this net console.
	int  m_nFailedAttempts                                 {}; // Num failed authentication attempts.
	int  m_nIgnoredMessage                                 {}; // Count how many times client ignored the no-auth message.

	CConnectedNetConsoleData(SocketHandle_t hSocket = -1)
	{
		m_nCharsInCommandBuffer = 0;
		m_bAuthorized           = false;
		m_hSocket               = hSocket;
		m_bInputOnly            = false;
	}
};

/* PACKET FORMAT **********************************

REQUEST:
  int requestID;
  int ServerDataRequestType_t;
  NullTerminatedString (variable or command)
  NullTerminatedString (value)

RESPONSE:
  int requestID;
  int ServerDataResponseType_t;
  NullTerminatedString (variable)
  NullTerminatedString (value)

***************************************************/
