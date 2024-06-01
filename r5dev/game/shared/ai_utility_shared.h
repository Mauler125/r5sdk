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
	void RunRenderFrame(void);

	void DrawAIScriptNetwork(const CAI_Network* pNetwork,
		const Vector3D& vCameraPos,
		const int iNodeIndex,
		const float flCameraRange,
		const bool bUseDepthBuffer) const;

	void DrawNavMeshBVTree(const dtNavMesh* mesh,
		const Vector3D& vCameraPos,
		const VPlane& vCullPlane,
		const int iBVTreeIndex,
		const float flCameraRange,
		const int nTileRange,
		const bool bUseDepthBuffer) const;

	void DrawNavMeshPortals(const dtNavMesh* mesh,
		const Vector3D& vCameraPos,
		const VPlane& vCullPlane,
		const int iPortalIndex,
		const float flCameraRange,
		const int nTileRange,
		const bool bUseDepthBuffer) const;

	void DrawNavMeshPolys(const dtNavMesh* mesh,
		const Vector3D& vCameraPos,
		const VPlane& vCullPlane,
		const int iPolyIndex,
		const float flCameraRange,
		const int nTileRange,
		const bool bDepthBuffer) const;

	void DrawNavMeshPolyBoundaries(const dtNavMesh* mesh,
		const Vector3D& vCameraPos,
		const VPlane& vCullPlane,
		const int iBoundaryIndex,
		const float flCameraRange,
		const int nTileRange,
		const bool bDepthBuffer) const;

	shortx8 PackNodeLink(int32_t a, int32_t b, int32_t c = 0, int32_t d = 0) const;
	int GetNearestNodeToPos(const CAI_Network* pAINetwork, const Vector3D* vec) const;
	bool IsTileWithinRange(const dtMeshTile* pTile, const VPlane& vPlane, const Vector3D& vCamera, const float flCameraRadius) const;

private:
	Color m_BoxColor;
	Color m_LinkColor;
};

extern CAI_Utility* g_pAIUtility;
#endif // AI_UTILITY_SHARED_H
