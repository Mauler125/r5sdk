//===========================================================================//
//
// Purpose: 
//
//===========================================================================//
#pragma once

class CNetCon
{
public:
	bool Init(void);
	bool Shutdown(void);

	void Send(void);
	void Recv(void);

	CNetAdr2* pNetAdr2 = new CNetAdr2("localhost", "37015");
	CSocketCreator* pSocket = new CSocketCreator();
};