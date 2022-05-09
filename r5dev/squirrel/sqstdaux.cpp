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
SQInteger sqstd_aux_printerror(HSQUIRRELVM v)
{
    g_bSQAuxError = true;
    SQInteger results = v_sqstd_aux_printerror(v);
    g_bSQAuxError = false;
    return results;
}

void SQAUX_Attach()
{
    DetourAttach((LPVOID*)&v_sqstd_aux_printerror, &sqstd_aux_printerror);
}

void SQAUX_Detach()
{
    DetourDetach((LPVOID*)&v_sqstd_aux_printerror, &sqstd_aux_printerror);
}