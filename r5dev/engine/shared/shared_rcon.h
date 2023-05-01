#ifndef SHARED_RCON_H
#define SHARED_RCON_H
#include "base_rcon.h"
#include "protoc/sv_rcon.pb.h"
#include "protoc/cl_rcon.pb.h"

bool CL_NetConSerialize(const CNetConBase* pBase, vector<char>& vecBuf, const char* szReqBuf,
	const char* szReqVal, const cl_rcon::request_t requestType);
bool CL_NetConConnect(CNetConBase* pBase, const char* pHostAdr, const int nHostPort);

CConnectedNetConsoleData* SH_GetNetConData(CNetConBase* pBase, const int iSocket);
SocketHandle_t SH_GetNetConSocketHandle(CNetConBase* pBase, const int iSocket);

#endif // SHARED_RCON_H
