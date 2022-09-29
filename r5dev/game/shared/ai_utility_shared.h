#ifndef AI_UTILITY_SHARED_H
#define AI_UTILITY_SHARED_H

//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------
class dtNavMesh;
class CAI_Network;
class Vector3D;
class Color;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
class CAI_Utility
{
public:
	CAI_Utility(void);
	void DrawAIScriptNetwork(const CAI_Network* pAINetwork) const;
	void DrawNavMeshBVTree(dtNavMesh* mesh = nullptr) const;
	void DrawNavMeshPortals(dtNavMesh* mesh = nullptr) const;
	void DrawNavMeshPolys(dtNavMesh* mesh = nullptr) const;
	void DrawNavMeshPolyBoundaries(dtNavMesh* mesh = nullptr) const;
	uint64_t PackNodeLink(uint32_t a, uint32_t b) const;
	int64_t GetNearestNodeToPos(const CAI_Network* pAINetwork, const Vector3D* vec) const;

private:
	Color m_BoxColor;
	Color m_LinkColor;
};

extern CAI_Utility* g_pAIUtility;
#endif // AI_UTILITY_SHARED_H
