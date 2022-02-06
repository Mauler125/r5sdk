//===========================================================================//
//
// Purpose: Enumerations for writing out the requests.
//
//===========================================================================//
#pragma once

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
