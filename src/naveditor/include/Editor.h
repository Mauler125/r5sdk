//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#ifndef RECASTEDITOR_H
#define RECASTEDITOR_H

#include "Recast/Include/Recast.h"
#include "NavEditor/Include/EditorInterfaces.h"

#include "game/server/ai_navmesh.h"

struct hulldef
{
	const char* name;
	float radius;
	float height;
	float climbHeight;
	int tileSize;
	int cellResolution;
};
extern const hulldef hulls[5];

/// Tool types.
enum EditorToolType
{
	TOOL_NONE = 0,
	TOOL_TILE_EDIT,
	TOOL_TILE_HIGHLIGHT,
	TOOL_TEMP_OBSTACLE,
	TOOL_NAVMESH_TESTER,
	TOOL_NAVMESH_PRUNE,
	TOOL_OFFMESH_CONNECTION,
	TOOL_TRAVERSE_LINK,
	TOOL_CONVEX_VOLUME,
	TOOL_CROWD,
	MAX_TOOLS
};

/// These are just poly areas to use consistent values across the editors.
/// The use should specify these base on his needs.
//enum EditorPolyAreas // note: original poly area's for reference.
//{
//	EDITOR_POLYAREA_GROUND,
//	EDITOR_POLYAREA_JUMP,
//	EDITOR_POLYAREA_ROAD,
//	EDITOR_POLYAREA_DOOR,
//	EDITOR_POLYAREA_GRASS,
//	EDITOR_POLYAREA_WATER,
//};

#if DT_NAVMESH_SET_VERSION >= 7
enum EditorPolyAreas
{
	EDITOR_POLYAREA_JUMP,
	EDITOR_POLYAREA_GROUND,
	EDITOR_POLYAREA_RESERVED,
	EDITOR_POLYAREA_DOOR, // rename to trigger
};
#else
enum EditorPolyAreas
{
	EDITOR_POLYAREA_GROUND,
	EDITOR_POLYAREA_JUMP,
	EDITOR_POLYAREA_DOOR,
};
#endif

enum EditorPolyFlags
{
	EDITOR_POLYFLAGS_WALK		= 0x01,		// Ability to walk (ground, grass, road)
	EDITOR_POLYFLAGS_JUMP		= 0x02,		// Ability to jump.
	EDITOR_POLYFLAGS_DOOR		= 0x04,		// Ability to move through doors.
	EDITOR_POLYFLAGS_SWIM		= 0x08,		// Ability to swim (water).
	EDITOR_POLYFLAGS_DISABLED	= 0x10,		// Disabled polygon
	EDITOR_POLYFLAGS_ALL		= 0xffff	// All abilities.
};

class EditorDebugDraw : public DebugDrawGL
{
public:
	virtual unsigned int areaToCol(unsigned int area);
};

enum EditorPartitionType
{
	EDITOR_PARTITION_WATERSHED,
	EDITOR_PARTITION_MONOTONE,
	EDITOR_PARTITION_LAYERS,
};

struct EditorTool
{
	virtual ~EditorTool() {}
	virtual int type() = 0;
	virtual void init(class Editor* editor) = 0;
	virtual void reset() = 0;
	virtual void handleMenu() = 0;
	virtual void handleClick(const float* s, const float* p, bool shift) = 0;
	virtual void handleRender() = 0;
	virtual void handleRenderOverlay(double* proj, double* model, int* view) = 0;
	virtual void handleToggle() = 0;
	virtual void handleStep() = 0;
	virtual void handleUpdate(const float dt) = 0;
};

struct EditorToolState {
	virtual ~EditorToolState() {}
	virtual void init(class Editor* editor) = 0;
	virtual void reset() = 0;
	virtual void handleRender() = 0;
	virtual void handleRenderOverlay(double* proj, double* model, int* view) = 0;
	virtual void handleUpdate(const float dt) = 0;
};

class Editor
{
protected:
	class InputGeom* m_geom;
	class dtNavMesh* m_navMesh;
	class dtNavMeshQuery* m_navQuery;
	class dtCrowd* m_crowd;

	bool m_filterLowHangingObstacles;
	bool m_filterLedgeSpans;
	bool m_filterWalkableLowHeightSpans;

	int m_tileSize;
	float m_cellSize;
	float m_cellHeight;
	float m_agentHeight;
	float m_agentRadius;
	float m_agentMaxClimb;
	float m_agentMaxSlope;
	int m_regionMinSize;
	int m_regionMergeSize;
	int m_edgeMaxLen;
	float m_edgeMaxError;
	int m_vertsPerPoly;
	int m_polyCellRes;
	float m_detailSampleDist;
	float m_detailSampleMaxError;
	int m_partitionType;

	float m_navMeshBMin[3];
	float m_navMeshBMax[3];

	NavMeshType_e m_selectedNavMeshType;
	NavMeshType_e m_loadedNavMeshType;
	const char* m_navmeshName;
	
	EditorTool* m_tool;
	EditorToolState* m_toolStates[MAX_TOOLS];
	
	BuildContext* m_ctx;

	EditorDebugDraw m_dd;
	unsigned int m_navMeshDrawFlags;
	int m_traverseLinkDrawTypes;
	float m_recastDrawOffset[3];
	float m_detourDrawOffset[3];

public:
	std::string m_modelName;

	Editor();
	virtual ~Editor();

	bool loadAll(std::string path, const bool fullPath = false);
	void saveAll(std::string path, const dtNavMesh* mesh);

	bool loadNavMesh(const char* path, const bool fullPath = false);
	
	void setContext(BuildContext* ctx) { m_ctx = ctx; }
	
	void setTool(EditorTool* tool);
	EditorToolState* getToolState(int type) { return m_toolStates[type]; }
	void setToolState(int type, EditorToolState* s) { m_toolStates[type] = s; }

	EditorDebugDraw& getDebugDraw() { return m_dd; }
	const float* getRecastDrawOffset() const { return m_recastDrawOffset; }
	const float* getDetourDrawOffset() const { return m_detourDrawOffset; }

	virtual void handleSettings();
	virtual void handleTools();
	virtual void handleDebugMode();
	virtual void handleClick(const float* s, const float* p, bool shift);
	virtual void handleToggle();
	virtual void handleStep();
	virtual void handleRender();
	virtual void handleRenderOverlay(double* proj, double* model, int* view);
	virtual void handleMeshChanged(class InputGeom* geom);
	virtual bool handleBuild();
	virtual void handleUpdate(const float dt);
	virtual void collectSettings(struct BuildSettings& settings);

	virtual class InputGeom* getInputGeom() { return m_geom; }
	virtual class dtNavMesh* getNavMesh() { return m_navMesh; }
	virtual class dtNavMeshQuery* getNavMeshQuery() { return m_navQuery; }
	virtual class dtCrowd* getCrowd() { return m_crowd; }
	virtual float getAgentRadius() { return m_agentRadius; }
	virtual float getAgentHeight() { return m_agentHeight; }
	virtual float getAgentClimb() { return m_agentMaxClimb; }
	
	inline unsigned int getNavMeshDrawFlags() const { return m_navMeshDrawFlags; }
	inline void setNavMeshDrawFlags(unsigned int flags) { m_navMeshDrawFlags = flags; }

	inline void toggleNavMeshDrawFlag(unsigned int flag) { m_navMeshDrawFlags ^= flag; }

	inline NavMeshType_e getSelectedNavMeshType() const { return m_selectedNavMeshType; }
	inline NavMeshType_e getLoadedNavMeshType() const { return m_loadedNavMeshType; }

	void updateToolStates(const float dt);
	void initToolStates(Editor* editor);
	void resetToolStates();
	void renderToolStates();
	void renderOverlayToolStates(double* proj, double* model, int* view);

	void renderMeshOffsetOptions();
	void renderDetourDebugMenu();
	void renderIntermediateTileMeshOptions();

	void selectNavMeshType(const NavMeshType_e navMeshType);

	void resetCommonSettings();
	void handleCommonSettings();

	void buildStaticPathingData();

private:
	// Explicitly disabled copy constructor and copy assignment operator.
	Editor(const Editor&);
	Editor& operator=(const Editor&);
};


#endif // RECASTEDITOR_H
