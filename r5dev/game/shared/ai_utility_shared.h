#ifndef AI_UTILITY_SHARED_H
#define AI_UTILITY_SHARED_H

//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------
struct dtMeshTile;
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
	__m128i PackNodeLink(int32_t a, int32_t b, int32_t c = 0, int32_t d = 0) const;
	int GetNearestNodeToPos(const CAI_Network* pAINetwork, const Vector3D* vec) const;
	bool IsTileWithinRange(const dtMeshTile* pTile, const Vector3D& vCamera, const float flCameraRadius) const;

private:
	Color m_BoxColor;
	Color m_LinkColor;
};

extern CAI_Utility* g_pAIUtility;
#endif // AI_UTILITY_SHARED_H
