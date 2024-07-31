#ifndef ENTITYLIST_CLIENTBASE_H
#define ENTITYLIST_CLIENTBASE_H

#include "tier0/threadtools.h"
#include "game/shared/ehandle.h"

class C_EntInfo
{
public:
	IHandleEntity* m_pEntity;
	int			   m_SerialNumber;
	C_EntInfo* m_pPrev;
	C_EntInfo* m_pNext;

	inline void		ClearLinks() { m_pPrev = m_pNext = this; }
};

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
	class C_EntInfoList
	{
	public:
		C_EntInfoList();

		const C_EntInfo* Head() const { return m_pHead; }
		const C_EntInfo* Tail() const { return m_pTail; }
		C_EntInfo* Head() { return m_pHead; }
		C_EntInfo* Tail() { return m_pTail; }
		//void			AddToHead( C_EntInfo *pElement ) { LinkAfter( NULL, pElement ); }
		//void			AddToTail( C_EntInfo *pElement ) { LinkBefore( NULL, pElement ); }

		//void LinkBefore( C_EntInfo *pBefore, C_EntInfo *pElement );
		//void LinkAfter( C_EntInfo *pBefore, C_EntInfo *pElement );
		//void Unlink( C_EntInfo *pElement );
		//bool IsInList( C_EntInfo *pElement );

	private:
		C_EntInfo* m_pHead;
		C_EntInfo* m_pTail;
	};

	// The first MAX_EDICTS entities are networkable. The rest are client-only.
	C_EntInfo m_EntPtrArray[NUM_ENT_ENTRIES];
	C_EntInfoList	m_activeList;
	C_EntInfoList	m_freeNonNetworkableList;

	// Sound entities.
	CThreadMutex m_soundEntsMutex;
	IHandleEntity*	m_pSoundEnts[NUM_ENT_ENTRIES];
	ssize_t			m_soundEntCount;
};

#endif // ENTITYLIST_CLIENTBASE_H
