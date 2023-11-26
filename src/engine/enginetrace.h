#pragma once

#include "public/engine/IEngineTrace.h"
#include "public/cmodel.h"
#include "public/trace.h"
#include "mathlib/mathlib.h"

class CEngineTrace : public IEngineTrace
{};

/* ==== CENGINETRACE ======================================================================================================================================================= */
#ifndef CLIENT_DLL
class CEngineTraceServer : public CEngineTrace
{};
inline CEngineTraceServer* g_pEngineTraceServer = nullptr;
inline CEngineTraceServer* g_pEngineTraceServerVFTable = nullptr;
#endif // CLIENT_DLL
#ifndef DEDICATED

class CEngineTraceClient : public CEngineTrace
{};
inline CEngineTraceClient* g_pEngineTraceClient = nullptr;
#endif // DEDICATED

///////////////////////////////////////////////////////////////////////////////
class VEngineTrace : public IDetour
{
	virtual void GetAdr(void) const
	{
#ifndef CLIENT_DLL
		LogVarAdr("g_pEngineTraceServer", reinterpret_cast<uintptr_t>(g_pEngineTraceServer));
#endif // CLIENT_DLL
#ifndef DEDICATED
		LogVarAdr("g_pEngineTraceClient", reinterpret_cast<uintptr_t>(g_pEngineTraceClient));
#endif // DEDICATED
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const
	{
#ifndef CLIENT_DLL
		g_pEngineTraceServerVFTable = g_GameDll.GetVirtualMethodTable(".?AVCEngineTraceServer@@").RCast<CEngineTraceServer*>();
		g_pEngineTraceServer = reinterpret_cast<CEngineTraceServer*>(&g_pEngineTraceServerVFTable); // Must be done for virtual calls.
#endif // CLIENT_DLL
	}
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
