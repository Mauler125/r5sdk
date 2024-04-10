#ifndef SHARED_RCON_H
#define SHARED_RCON_H
#include "base_rcon.h"
#include "protoc/netcon.pb.h"

#ifndef _TOOLS
extern ConVar rcon_debug;
extern ConVar rcon_encryptframes;
extern ConVar rcon_key;

#ifndef CLIENT_DLL
extern void RCON_InitServerAndTrySyncKeys(const char* pPassword);
#endif // !CLIENT_DLL
#ifndef DEDICATED
extern void RCON_InitClientAndTrySyncKeys();
#endif // !DEDICATED
#endif // _TOOLS

bool SV_NetConSerialize(const CNetConBase* pBase, vector<char>& vecBuf, const char* pResponseMsg, const char* pResponseVal,
	const netcon::response_e responseType, const int nMessageId, const int nMessageType, const bool bEncrypt, const bool bDebug);

bool CL_NetConSerialize(const CNetConBase* pBase, vector<char>& vecBuf, const char* szReqBuf,
	const char* szReqVal, const netcon::request_e requestType, const bool bEncrypt, const bool bDebug);
bool CL_NetConConnect(CNetConBase* pBase, const char* pHostAdr, const int nHostPort);

bool SH_NetConPackEnvelope(const CNetConBase* pBase, vector<char>& outMsgBuf, const size_t nMsgLen, google::protobuf::MessageLite* inMsg, const bool bEncrypt, const bool bDebug);
bool SH_NetConUnpackEnvelope(const CNetConBase* pBase, const char* pMsgBuf, const size_t nMsgLen, google::protobuf::MessageLite* outMsg, const bool bDebug);

CConnectedNetConsoleData* SH_GetNetConData(CNetConBase* pBase, const int iSocket);
SocketHandle_t SH_GetNetConSocketHandle(CNetConBase* pBase, const int iSocket);

#endif // SHARED_RCON_H
