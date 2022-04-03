//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "core/stdafx.h"
#include "tier0/cvar.h"
#include "engine/sys_utils.h"
#include "game/server/ai_network.h"

int g_DebugConnectNode1 = -1;
int g_DebugConnectNode2 = -1;
#define DebuggingConnect( node1, node2 ) ( ( node1 == g_DebugConnectNode1 && node2 == g_DebugConnectNode2 ) || ( node1 == g_DebugConnectNode2 && node2 == g_DebugConnectNode1 ) )

//-----------------------------------------------------------------------------
// Purpose: debug logs node connections
// Input  : node1 - 
//			node2 - 
//			*pszFormat - 
//			... - 
//-----------------------------------------------------------------------------
void CAI_Network::DebugConnectMsg(int node1, int node2, const char* pszFormat, ...)
{
	if (ai_ainDebugConnect->GetBool())
	{
		if (DebuggingConnect(node1, node2))
		{
			static char buf[1024] = {};
			{/////////////////////////////
				va_list args{};
				va_start(args, pszFormat);

				vsnprintf(buf, sizeof(buf), pszFormat, args);

				buf[sizeof(buf) - 1] = 0;
				va_end(args);
			}/////////////////////////////


			DevMsg(eDLL_T::SERVER, "%s", buf);
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
int CAI_Network::GetNumLinks(void) const
{
	return m_iNumLinks;
}

//-----------------------------------------------------------------------------
// Purpose: gets the number of zones
// Output : int
//-----------------------------------------------------------------------------
int CAI_Network::GetNumZones(void) const
{
	return m_iNumZones;
}

//-----------------------------------------------------------------------------
// Purpose: gets the number of hints
// Output : int
//-----------------------------------------------------------------------------
int CAI_Network::GetNumHints(void) const
{
	return m_iNumHints;
}

//-----------------------------------------------------------------------------
// Purpose: gets the number of script nodes
// Output : int
//-----------------------------------------------------------------------------
int CAI_Network::GetNumScriptNodes(void) const
{
	return m_iNumScriptNodes;
}

//-----------------------------------------------------------------------------
// Purpose: gets the path nodes
// Output : int64_t
//-----------------------------------------------------------------------------
int64_t CAI_Network::GetNumPathNodes(void) const
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
// Purpose: gets the pointer to path nodes
// Output : CAI_Node**
//-----------------------------------------------------------------------------
CAI_Node** CAI_Network::GetPathNodes(void) const
{
	return m_pAInode;
}

//-----------------------------------------------------------------------------
void CAI_Network_Attach()
{
	DetourAttach((LPVOID*)&v_CAI_Network__DebugConnectMsg, &CAI_Network::DebugConnectMsg);
}

void CAI_Network_Detach()
{
	DetourDetach((LPVOID*)&v_CAI_Network__DebugConnectMsg, &CAI_Network::DebugConnectMsg);
}
