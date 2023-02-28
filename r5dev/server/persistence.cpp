#include "core/stdafx.h"
#include "server/vengineserver_impl.h"
#include "server/persistence.h"

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
bool Persistence_SetXP(int a1, int* a2)
{
	g_pEngineServer->PersistenceAvailable(nullptr, a1);
	return v_Persistence_SetXP(a1, a2);
}
#endif

void VPersistence::Attach() const
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	DetourAttach((LPVOID*)&v_Persistence_SetXP, &Persistence_SetXP);
#endif
}
void VPersistence::Detach() const
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	DetourDetach((LPVOID*)&v_Persistence_SetXP, &Persistence_SetXP);
#endif
}