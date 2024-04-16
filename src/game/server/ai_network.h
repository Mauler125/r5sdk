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
	int NumLinks(void) const;
	int NumZones(const int idx) const;
	int NumHints(void) const;
	int NumScriptNodes(void) const;
	int NumPathNodes(void) const;

	short GetHint(int nIndex) const;
	CAI_ScriptNode* GetScriptNodes(void) const;

	CAI_Node* AddPathNode(const Vector3D* origin, const float jaw);
	CAI_Node* GetPathNode(int id) const;

	CAI_NodeLink* CreateNodeLink(int srcID, int destID);

public:
	void* m_pVTable;  // <-- 'this'.

	int m_iNumLinks;              // +0x0008
	int m_nUnk0;

	CAI_HullData m_HullData[MAX_HULLS];
	int m_iNumZones[MAX_HULLS];   // +0x0088

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

inline CAI_Node*(*CAI_Network__AddPathNode)(CAI_Network* pNetwork, const Vector3D* origin, float yaw);
inline CAI_NodeLink* (*CAI_Network__CreateNodeLink)(CAI_Network* pNetwork, int srcID, int destID);
inline void(*CAI_Network__DebugConnectMsg)(int node1, int node2, const char* pszformat, ...);

///////////////////////////////////////////////////////////////////////////////
class VAI_Network : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CAI_Network::AddPathNode", CAI_Network__AddPathNode);
		LogFunAdr("CAI_Network::CreateNodeLink", CAI_Network__CreateNodeLink);
		LogFunAdr("CAI_Network::DebugConnectMsg", CAI_Network__DebugConnectMsg);
		LogVarAdr("g_pAINetwork", g_pAINetwork);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 30 48 8B B9 ?? ?? ?? ?? 48 8B F2 0F 29 74 24 ??").GetPtr(CAI_Network__AddPathNode);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 57 41 56 48 83 EC 20 49 63 E8").GetPtr(CAI_Network__CreateNodeLink);
		g_GameDll.FindPatternSIMD("4C 89 4C 24 ?? 48 83 EC 18").GetPtr(CAI_Network__DebugConnectMsg);
	}
	virtual void GetVar(void) const
	{
		g_pAINetwork = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 4C 63 91 ?? ?? ?? ??").FindPatternSelf("48 8B").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CAI_Network**>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
