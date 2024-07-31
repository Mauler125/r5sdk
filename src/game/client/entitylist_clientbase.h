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
	// Get an ehandle from a networkable entity's index (note: if there is no entity in that slot,
	// then the ehandle will be invalid and produce NULL).
	CBaseHandle GetNetworkableHandle(const int iEntity) const;

	// ehandles use this in their Get() function to produce a pointer to the entity.
	IHandleEntity* LookupEntity(const CBaseHandle& handle) const;
	IHandleEntity* LookupEntityByNetworkIndex(const int edictIndex) const;

	// Use these to iterate over all the entities.
	CBaseHandle FirstHandle() const;
	CBaseHandle NextHandle(const CBaseHandle& hEnt) const;
	static CBaseHandle InvalidHandle();

	const C_EntInfo* FirstEntInfo()                         const;
	const C_EntInfo* NextEntInfo(const C_EntInfo* pInfo)    const;
	const C_EntInfo* GetEntInfoPtr(const CBaseHandle& hEnt) const;
	const C_EntInfo* GetEntInfoPtrByIndex(const int index)  const;

	// Used by Foundry when an entity is respawned/edited.
	// We force the new entity's ehandle to be the same so anyone pointing at it still gets a valid CBaseEntity out of their ehandle.
	void ForceEntSerialNumber(const int iEntIndex, const int iSerialNumber);

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
		C_EntInfoList()
		{
			m_pHead = NULL;
			m_pTail = NULL;
		}

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

	int GetEntInfoIndex(const C_EntInfo* pEntInfo) const;

	// The first MAX_EDICTS entities are networkable. The rest are client-only.
	C_EntInfo m_EntPtrArray[NUM_ENT_ENTRIES];
	C_EntInfoList	m_activeList;
	C_EntInfoList	m_freeNonNetworkableList;

	// Sound entities.
	CThreadMutex m_soundEntsMutex;
	IHandleEntity*	m_pSoundEnts[NUM_ENT_ENTRIES];
	ssize_t			m_soundEntCount;
};

// ------------------------------------------------------------------------------------ //
// Inlines.
// ------------------------------------------------------------------------------------ //

inline int C_BaseEntityList::GetEntInfoIndex(const C_EntInfo* pEntInfo) const
{
	Assert(pEntInfo);
	const int index = (int)(pEntInfo - m_EntPtrArray);
	Assert(index >= 0 && index < NUM_ENT_ENTRIES);
	return index;
}

inline CBaseHandle C_BaseEntityList::GetNetworkableHandle(const int iEntity) const
{
	Assert(iEntity >= 0 && iEntity < MAX_EDICTS);
	if (m_EntPtrArray[iEntity].m_pEntity)
		return CBaseHandle(iEntity, m_EntPtrArray[iEntity].m_SerialNumber);
	else
		return CBaseHandle();
}


inline IHandleEntity* C_BaseEntityList::LookupEntity(const CBaseHandle& handle) const
{
	if (handle.m_Index == INVALID_EHANDLE_INDEX)
		return NULL;

	const C_EntInfo* pInfo = &m_EntPtrArray[handle.GetEntryIndex()];
	if (pInfo->m_SerialNumber == handle.GetSerialNumber())
		return pInfo->m_pEntity;
	else
		return NULL;
}

inline IHandleEntity* C_BaseEntityList::LookupEntityByNetworkIndex(const int edictIndex) const
{
	// (Legacy support).
	if (edictIndex < 0)
		return NULL;

	Assert(edictIndex < NUM_ENT_ENTRIES);
	return m_EntPtrArray[edictIndex].m_pEntity;
}


inline CBaseHandle C_BaseEntityList::FirstHandle() const
{
	if (!m_activeList.Head())
		return INVALID_EHANDLE_INDEX;

	const int index = GetEntInfoIndex(m_activeList.Head());
	return CBaseHandle(index, m_EntPtrArray[index].m_SerialNumber);
}

inline CBaseHandle C_BaseEntityList::NextHandle(const CBaseHandle& hEnt) const
{
	const int iSlot = hEnt.GetEntryIndex();
	const C_EntInfo* pNext = m_EntPtrArray[iSlot].m_pNext;
	if (!pNext)
		return INVALID_EHANDLE_INDEX;

	const int index = GetEntInfoIndex(pNext);

	return CBaseHandle(index, m_EntPtrArray[index].m_SerialNumber);
}

inline CBaseHandle C_BaseEntityList::InvalidHandle()
{
	return INVALID_EHANDLE_INDEX;
}

inline const C_EntInfo* C_BaseEntityList::FirstEntInfo() const
{
	return m_activeList.Head();
}

inline const C_EntInfo* C_BaseEntityList::NextEntInfo(const C_EntInfo* pInfo) const
{
	return pInfo->m_pNext;
}

inline const C_EntInfo* C_BaseEntityList::GetEntInfoPtr(const CBaseHandle& hEnt) const
{
	const int iSlot = hEnt.GetEntryIndex();
	return &m_EntPtrArray[iSlot];
}

inline const C_EntInfo* C_BaseEntityList::GetEntInfoPtrByIndex(const int index) const
{
	return &m_EntPtrArray[index];
}

inline void C_BaseEntityList::ForceEntSerialNumber(const int iEntIndex, const int iSerialNumber)
{
	m_EntPtrArray[iEntIndex].m_SerialNumber = iSerialNumber;
}

#endif // ENTITYLIST_CLIENTBASE_H
