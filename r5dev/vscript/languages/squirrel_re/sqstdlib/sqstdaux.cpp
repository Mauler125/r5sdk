//=============================================================================//
//
// Purpose: 
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/tslist.h"
#include "sqvm.h"
#include "sqstdaux.h"

bool g_bSQAuxError = false;
bool g_bSQAuxBadLogic = false;
HSQUIRRELVM g_pErrorVM = nullptr;

SQInteger sqstd_aux_printerror(HSQUIRRELVM v)
{
    g_bSQAuxError = true;
    SQInteger results = v_sqstd_aux_printerror(v);
    g_bSQAuxError = false;
    return results;
}

SQInteger sqstd_aux_badlogic(HSQUIRRELVM v, __m128i* a2, __m128i* a3)
{
    g_pErrorVM = v;

    SQInteger results = v_sqstd_aux_badlogic(v, a2, a3);
    return results;
}

void VSquirrelAUX::Detour(const bool bAttach) const
{
    DetourSetup(&v_sqstd_aux_printerror, &sqstd_aux_printerror, bAttach);
    DetourSetup(&v_sqstd_aux_badlogic, &sqstd_aux_badlogic, bAttach);
}
