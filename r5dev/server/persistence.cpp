#include "core/stdafx.h"
#include "server/vengineserver_impl.h"
#include "server/persistence.h"

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
bool Persistence_SetXP(int a1, int* a2)
{
	HIVEngineServer__PersistenceAvailable(nullptr, a1);
	return v_Persistence_SetXP(a1, a2);
}
#endif

void Persistence_Attach()
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	DetourAttach((LPVOID*)&v_Persistence_SetXP, &Persistence_SetXP);
#endif
}
void Persistence_Detach()
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	DetourDetach((LPVOID*)&v_Persistence_SetXP, &Persistence_SetXP);
#endif
}