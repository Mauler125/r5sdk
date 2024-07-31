#ifndef ENTITYLIST_CLIENTBASE_H
#define ENTITYLIST_CLIENTBASE_H

#include <tier0/threadtools.h>
#include <game/shared/ehandle.h>
#include <game/shared/entitylist_base.h>

class C_BaseEntityList
{
public:

	// Overridables.
protected:

	// These are notifications to the derived class. It can cache info here if it wants.
	virtual void OnAddEntity(IHandleEntity* pEnt, const CBaseHandle& handle) = 0; // NOTE: implemented in engine!

	// It is safe to delete the entity here. We won't be accessing the pointer after
	// calling OnRemoveEntity.
	virtual void OnRemoveEntity(IHandleEntity* pEnt, const CBaseHandle& handle) = 0; // NOTE: implemented in engine!

private:
	// The first MAX_EDICTS entities are networkable. The rest are client-only or server-only.
	CEntInfo m_EntPtrArray[NUM_ENT_ENTRIES];
	CEntInfoList	m_activeList;
	CEntInfoList	m_freeNonNetworkableList;

	// Client Sound (Miles) entities.
	CThreadMutex m_clientSoundEntsMutex;
	IHandleEntity*	m_pClientSoundEnts[NUM_ENT_ENTRIES];
	ssize_t			m_clientSoundEntCount;
};

#endif // ENTITYLIST_CLIENTBASE_H
