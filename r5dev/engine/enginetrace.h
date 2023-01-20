#pragma once

#include "public/engine/IEngineTrace.h"
#include "public/cmodel.h"
#include "public/trace.h"
#include "mathlib/mathlib.h"

class CEngineTrace : public IEngineTrace
{
public:
};

/* ==== CENGINETRACE ======================================================================================================================================================= */

inline CEngineTrace* g_pEngineTrace = nullptr;

///////////////////////////////////////////////////////////////////////////////
void CEngineTrace_Attach();
void CEngineTrace_Detach();

///////////////////////////////////////////////////////////////////////////////
class VEngineTrace : public IDetour
{
	virtual void GetAdr(void) const { }
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VEngineTrace);