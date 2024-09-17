//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "game/server/ai_network.h"

int g_DebugConnectNode1 = -1;
int g_DebugConnectNode2 = -1;
#define DebuggingConnect( node1, node2 ) ( ( node1 == g_DebugConnectNode1 && node2 == g_DebugConnectNode2 ) || ( node1 == g_DebugConnectNode2 && node2 == g_DebugConnectNode1 ) )

static ConVar ai_ainDebugConnect("ai_ainDebugConnect", "0", FCVAR_DEVELOPMENTONLY, "Debug AIN node connections");

//-----------------------------------------------------------------------------
// Purpose: debug logs node connections
// Input  : node1 - 
//			node2 - 
//			*pszFormat - 
//			... - 
//-----------------------------------------------------------------------------
void CAI_Network::DebugConnectMsg(int node1, int node2, const char* pszFormat, ...)
{
	if (ai_ainDebugConnect.GetBool())
	{
		if (DebuggingConnect(node1, node2))
		{
			static char buf[1024] = {};
			{/////////////////////////////
				va_list args{};
				va_start(args, pszFormat);

				vsnprintf(buf, sizeof(buf), pszFormat, args);

				buf[sizeof(buf) - 1] = '\0';
				va_end(args);
			}/////////////////////////////


			Msg(eDLL_T::SERVER, "%s", buf);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: gets the AI Network VTable
// Output : void*
//-----------------------------------------------------------------------------
void* CAI_Network::GetVTable(void) const
{
	return m_pVTable;
}

//-----------------------------------------------------------------------------
// Purpose: gets the number of node links
// Output : int
//-----------------------------------------------------------------------------
int CAI_Network::NumLinks(void) const
{
	return m_iNumLinks;
}

//-----------------------------------------------------------------------------
// Purpose: gets the number of zones
// input  : idx - 
// Output : int
//-----------------------------------------------------------------------------
int CAI_Network::NumZones(const int idx) const
{
	Assert(idx >= 0 && idx < sizeof(m_iNumZones));
	return m_iNumZones[idx];
}

//-----------------------------------------------------------------------------
// Purpose: gets the number of hints
// Output : int
//-----------------------------------------------------------------------------
int CAI_Network::NumHints(void) const
{
	return m_iNumHints;
}

//-----------------------------------------------------------------------------
// Purpose: gets the number of script nodes
// Output : int
//-----------------------------------------------------------------------------
int CAI_Network::NumScriptNodes(void) const
{
	return m_iNumScriptNodes;
}

//-----------------------------------------------------------------------------
// Purpose: gets the path nodes
// Output : int
//-----------------------------------------------------------------------------
int CAI_Network::NumPathNodes(void) const
{
	return m_iNumNodes;
}

//-----------------------------------------------------------------------------
// Purpose: gets the specified hint from static array
// Input  : nIndex - 
// Output : int
//-----------------------------------------------------------------------------
short CAI_Network::GetHint(int nIndex) const
{
	return m_Hints[nIndex];
}

//-----------------------------------------------------------------------------
// Purpose: gets the pointer to script node array
// Output : CAI_ScriptNode*
//-----------------------------------------------------------------------------
CAI_ScriptNode* CAI_Network::GetScriptNodes(void) const
{
	return m_ScriptNode;
}

//-----------------------------------------------------------------------------
// Purpose: gets the pointer to path node
// Input  : id - 
// Output : CAI_Node**
//-----------------------------------------------------------------------------
CAI_Node* CAI_Network::GetPathNode(int id) const
{
	if (id >= 0 &&
		id < m_iNumNodes)
	{
		return m_pAInode[id];
	}

	Assert(0);
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: adds a path node
// Input  : *origin - 
//          jaw - 
// Output : CAI_Node*
//-----------------------------------------------------------------------------
CAI_Node* CAI_Network::AddPathNode(const Vector3D* origin, const float jaw)
{
	return CAI_Network__AddPathNode(this, origin, jaw);
}

//-----------------------------------------------------------------------------
// Purpose: creates a node link
// Input  : srcID - 
//          destID - 
// Output : CAI_NodeLink*
//-----------------------------------------------------------------------------
CAI_NodeLink* CAI_Network::CreateNodeLink(int srcID, int destID)
{
	return CAI_Network__CreateNodeLink(this, srcID, destID);
}

//-----------------------------------------------------------------------------
void VAI_Network::Detour(const bool bAttach) const
{
	DetourSetup(&CAI_Network__DebugConnectMsg, &CAI_Network::DebugConnectMsg, bAttach);
}
