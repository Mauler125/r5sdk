//=============================================================================//
//
// Purpose: 
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/tslist.h"
#include "squirrel/sqvm.h"
#include "squirrel/sqstdaux.h"

bool g_bSQAuxError = false;
bool g_bSQAuxBadLogic = false;
SQInteger sqstd_aux_printerror(HSQUIRRELVM v)
{
    g_bSQAuxError = true;
    SQInteger results = v_sqstd_aux_printerror(v);
    g_bSQAuxError = false;
    return results;
}

SQInteger sqstd_aux_badlogic(HSQUIRRELVM v, __m128i* a2, __m128i* a3)
{
    SQInteger results = v_sqstd_aux_badlogic(v, a2, a3);
    return results;
}

void SQAUX_Attach()
{
    DetourAttach((LPVOID*)&v_sqstd_aux_printerror, &sqstd_aux_printerror);
    DetourAttach((LPVOID*)&v_sqstd_aux_badlogic, &sqstd_aux_badlogic);
}

void SQAUX_Detach()
{
    DetourDetach((LPVOID*)&v_sqstd_aux_printerror, &sqstd_aux_printerror);
    DetourDetach((LPVOID*)&v_sqstd_aux_badlogic, &sqstd_aux_badlogic);
}