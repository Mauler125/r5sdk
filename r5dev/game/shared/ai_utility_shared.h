#ifndef AI_UTILITY_SHARED_H
#define AI_UTILITY_SHARED_H

void DrawAIScriptNodes();
void DrawNavMeshBVTree();
void DrawNavMeshPortals();
void DrawNavMeshPolys();
void DrawNavMeshPolyBoundaries();
uint64_t PackNodeLink(uint32_t a, uint32_t b);
int64_t GetNearestNodeToPos(const Vector3D* vec);

#endif // AI_UTILITY_SHARED_H
