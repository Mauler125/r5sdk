#include "core/stdafx.h"
#include "vengineserver_impl.h"
#include "persistence.h"

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
bool Persistence_SetXP(int a1, int* a2)
{
	g_pEngineServer->PersistenceAvailable(nullptr, a1);
	return v_Persistence_SetXP(a1, a2);
}
#endif

void VPersistence::Detour(const bool bAttach) const
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	Setup(&v_Persistence_SetXP, &Persistence_SetXP, bAttach);
#endif
}
