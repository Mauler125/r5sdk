//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. =====//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//===========================================================================//
#ifndef ENGINE_MODELINFO_H
#define ENGINE_MODELINFO_H
#include "public/engine/IVModelInfo.h"

//-----------------------------------------------------------------------------
// shared implementation of IVModelInfo
//-----------------------------------------------------------------------------
class CModelInfo : public IVModelInfoClient
{};

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// implementation of IVModelInfo for server
//-----------------------------------------------------------------------------
class CModelInfoServer : public CModelInfo
{};
extern CModelInfoServer* g_pModelInfoServer;
inline CModelInfoServer* g_pModelInfoServer_VFTable;
#endif // CLIENT_DLL

#ifndef DEDICATED
//-----------------------------------------------------------------------------
// implementation of IVModelInfo for client
//-----------------------------------------------------------------------------
class CModelInfoClient : public CModelInfo
{};
extern CModelInfoClient* g_pModelInfoClient;
inline CModelInfoClient* g_pModelInfoClient_VFTable;
#endif // DEDICATED

///////////////////////////////////////////////////////////////////////////////
class VModelInfo : public IDetour
{
	virtual void GetAdr(void) const
	{
#ifndef CLIENT_DLL
		spdlog::debug("| FUN: g_pModelInfoServer                   : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pModelInfoServer));
#endif // CLIENT_DLL
#ifndef DEDICATED
		spdlog::debug("| FUN: g_pModelInfoClient                   : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pModelInfoClient));
#endif // DEDICATED
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
#ifndef CLIENT_DLL
		g_pModelInfoServer_VFTable = g_GameDll.GetVirtualMethodTable(".?AVCModelInfoServer@@").RCast<CModelInfoServer*>();
		g_pModelInfoServer = reinterpret_cast<CModelInfoServer*>(&g_pModelInfoServer_VFTable);
#endif // CLIENT_DLL
#ifndef DEDICATED
		g_pModelInfoClient_VFTable = g_GameDll.GetVirtualMethodTable(".?AVCModelInfoClient@@").RCast<CModelInfoClient*>();
		g_pModelInfoClient = reinterpret_cast<CModelInfoClient*>(&g_pModelInfoClient_VFTable);
#endif // DEDICATED
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VModelInfo);

#endif // ENGINE_MODELINFO_H
