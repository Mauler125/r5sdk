//===========================================================================//
//
// Purpose: Enumerations for writing out the requests.
//
//===========================================================================//
#pragma once

typedef int SocketHandle_t;

enum class ServerDataRequestType_t : int
{
	SERVERDATA_REQUEST_VALUE = 0,
	SERVERDATA_REQUEST_SETVALUE,
	SERVERDATA_REQUEST_EXECCOMMAND,
	SERVERDATA_REQUEST_AUTH,
	SERVERDATA_REQUEST_SEND_CONSOLE_LOG,
	SERVERDATA_REQUEST_SEND_REMOTEBUG,
};

enum class ServerDataResponseType_t : int
{
	SERVERDATA_RESPONSE_VALUE = 0,
	SERVERDATA_RESPONSE_UPDATE,
	SERVERDATA_RESPONSE_AUTH,
	SERVERDATA_RESPONSE_CONSOLE_LOG,
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
