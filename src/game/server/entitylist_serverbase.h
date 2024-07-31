#ifndef ENTITYLIST_SERVERBASE_H
#define ENTITYLIST_SERVERBASE_H

#include "tier0/threadtools.h"
#include "tier1/string_t.h"
#include "game/shared/ehandle.h"

class CEntInfo
{
public:
	IHandleEntity* m_pEntity;
	int			   m_SerialNumber;
	CEntInfo* m_pPrev;
	CEntInfo* m_pNext;
	string_t		m_iName;
	string_t		m_iClassName;

	inline void		ClearLinks() { m_pPrev = m_pNext = this; }
};

class CBaseEntityList
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

	const CEntInfo* FirstEntInfo()                         const;
	const CEntInfo* NextEntInfo(const CEntInfo* pInfo)     const;
	const CEntInfo* GetEntInfoPtr(const CBaseHandle& hEnt) const;
	const CEntInfo* GetEntInfoPtrByIndex(const int index)  const;

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
	class CEntInfoList
	{
	public:
		CEntInfoList()
		{
			m_pHead = NULL;
			m_pTail = NULL;
		}

		const CEntInfo* Head() const { return m_pHead; }
		const CEntInfo* Tail() const { return m_pTail; }
		CEntInfo* Head() { return m_pHead; }
		CEntInfo* Tail() { return m_pTail; }
		//void			AddToHead( CEntInfo *pElement ) { LinkAfter( NULL, pElement ); }
		//void			AddToTail( CEntInfo *pElement ) { LinkBefore( NULL, pElement ); }

		//void LinkBefore( CEntInfo *pBefore, CEntInfo *pElement );
		//void LinkAfter( CEntInfo *pBefore, CEntInfo *pElement );
		//void Unlink( CEntInfo *pElement );
		//bool IsInList( CEntInfo *pElement );

	private:
		CEntInfo* m_pHead;
		CEntInfo* m_pTail;
	};

	int GetEntInfoIndex(const CEntInfo* pEntInfo) const;

	// The first MAX_EDICTS entities are networkable. The rest are server-only.
	CEntInfo m_EntPtrArray[NUM_ENT_ENTRIES];
	CEntInfoList	m_activeList;
	CEntInfoList	m_freeNonNetworkableList;

	// Sound entities.
	CThreadMutex m_soundEntsMutex;
	IHandleEntity*	m_pSoundEnts[NUM_ENT_ENTRIES];
	ssize_t			m_soundEntCount;
};

// ------------------------------------------------------------------------------------ //
// Inlines.
// ------------------------------------------------------------------------------------ //

inline int CBaseEntityList::GetEntInfoIndex(const CEntInfo* pEntInfo) const
{
	Assert(pEntInfo);
	const int index = (int)(pEntInfo - m_EntPtrArray);
	Assert(index >= 0 && index < NUM_ENT_ENTRIES);
	return index;
}

inline CBaseHandle CBaseEntityList::GetNetworkableHandle(const int iEntity) const
{
	Assert(iEntity >= 0 && iEntity < MAX_EDICTS);
	if (m_EntPtrArray[iEntity].m_pEntity)
		return CBaseHandle(iEntity, m_EntPtrArray[iEntity].m_SerialNumber);
	else
		return CBaseHandle();
}


inline IHandleEntity* CBaseEntityList::LookupEntity(const CBaseHandle& handle) const
{
	if (handle.m_Index == INVALID_EHANDLE_INDEX)
		return NULL;

	const CEntInfo* pInfo = &m_EntPtrArray[handle.GetEntryIndex()];
	if (pInfo->m_SerialNumber == handle.GetSerialNumber())
		return pInfo->m_pEntity;
	else
		return NULL;
}

inline IHandleEntity* CBaseEntityList::LookupEntityByNetworkIndex(const int edictIndex) const
{
	// (Legacy support).
	if (edictIndex < 0)
		return NULL;

	Assert(edictIndex < NUM_ENT_ENTRIES);
	return m_EntPtrArray[edictIndex].m_pEntity;
}


inline CBaseHandle CBaseEntityList::FirstHandle() const
{
	if (!m_activeList.Head())
		return INVALID_EHANDLE_INDEX;

	const int index = GetEntInfoIndex(m_activeList.Head());
	return CBaseHandle(index, m_EntPtrArray[index].m_SerialNumber);
}

inline CBaseHandle CBaseEntityList::NextHandle(const CBaseHandle& hEnt) const
{
	const int iSlot = hEnt.GetEntryIndex();
	const CEntInfo* pNext = m_EntPtrArray[iSlot].m_pNext;
	if (!pNext)
		return INVALID_EHANDLE_INDEX;

	const int index = GetEntInfoIndex(pNext);

	return CBaseHandle(index, m_EntPtrArray[index].m_SerialNumber);
}

inline CBaseHandle CBaseEntityList::InvalidHandle()
{
	return INVALID_EHANDLE_INDEX;
}

inline const CEntInfo* CBaseEntityList::FirstEntInfo() const
{
	return m_activeList.Head();
}

inline const CEntInfo* CBaseEntityList::NextEntInfo(const CEntInfo* pInfo) const
{
	return pInfo->m_pNext;
}

inline const CEntInfo* CBaseEntityList::GetEntInfoPtr(const CBaseHandle& hEnt) const
{
	int iSlot = hEnt.GetEntryIndex();
	return &m_EntPtrArray[iSlot];
}

inline const CEntInfo* CBaseEntityList::GetEntInfoPtrByIndex(const int index) const
{
	return &m_EntPtrArray[index];
}

inline void CBaseEntityList::ForceEntSerialNumber(const int iEntIndex, const int iSerialNumber)
{
	m_EntPtrArray[iEntIndex].m_SerialNumber = iSerialNumber;
}

#endif // ENTITYLIST_SERVERBASE_H
