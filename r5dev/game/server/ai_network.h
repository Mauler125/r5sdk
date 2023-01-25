#pragma once
#include "game/server/ai_node.h"

//-----------------------------------------------------------------------------
// CAI_Network
//
// Purpose: Stores a node graph through which an AI may pathfind
//-----------------------------------------------------------------------------
class CAI_Network
{
public:
	static void DebugConnectMsg(int node1, int node2, const char* pszFormat, ...);
	void* GetVTable(void) const;
	int GetNumLinks(void) const;
	int GetNumZones(void) const;
	int GetNumHints(void) const;
	int GetNumScriptNodes(void) const;
	int64_t GetNumPathNodes(void) const;

	short GetHint(int nIndex) const;
	CAI_ScriptNode* GetScriptNodes(void) const;
	CAI_Node** GetPathNodes(void) const;

public:
	void* m_pVTable;  // <-- 'this'.

	int m_iNumLinks;              // +0x0008
	char unk1[0x7C];              // +0x000C
	int m_iNumZones;              // +0x0088
	char unk2[0x10];              // +0x008C

	// unk8 on disk
	int unk5;                     // +0x009C
	char unk6[0x4];               // +0x00A0
	int m_iNumHints;              // +0x00A4

	short m_Hints[0x7D0];         // +0x00A8 <-- '2000' hints.
	CAI_ScriptNode* m_ScriptNode; // +0x1048 <-- '[r5apex_ds.exe + 0xc6fd39]'.
	int m_iNumScriptNodes;        // +0x1050

	char pad0[0x14];              // +0x1054 <-- !TODO

	int m_iNumNodes;              // +0x1070
	CAI_Node** m_pAInode;         // +0x1078
};
inline CAI_Network** g_pAINetwork = nullptr;

inline CMemory p_CAI_Network__DebugConnectMsg;
inline auto v_CAI_Network__DebugConnectMsg = p_CAI_Network__DebugConnectMsg.RCast<void (*)(int node1, int node2, const char* pszformat, ...)>();

///////////////////////////////////////////////////////////////////////////////
class VAI_Network : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CAI_Network::DebugConnectMsg", p_CAI_Network__DebugConnectMsg.GetPtr());
		LogVarAdr("g_pAINetwork", reinterpret_cast<uintptr_t>(g_pAINetwork));
	}
	virtual void GetFun(void) const
	{
		p_CAI_Network__DebugConnectMsg = g_GameDll.FindPatternSIMD("4C 89 4C 24 ?? 48 83 EC 18");
		v_CAI_Network__DebugConnectMsg = p_CAI_Network__DebugConnectMsg.RCast<void (*)(int, int, const char*, ...)>(); /*4C 89 4C 24 ?? 48 83 EC 18*/
	}
	virtual void GetVar(void) const
	{
		g_pAINetwork = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 4C 63 91 ?? ?? ?? ??").FindPatternSelf("48 8B").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CAI_Network**>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////
