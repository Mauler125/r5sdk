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
#include "DebugUtils/Include/RecastDebugDraw.h"
#include "DebugUtils/Include/DetourDebugDraw.h"

#include "Detour/Include/DetourNavMeshBuilder.h"

#include "game/server/ai_navmesh.h"

struct dtMeshTile;

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

struct TraverseType_s
{
	float minElev;
	float maxElev;
	float minDist;
	float maxDist;
};

enum TraverseType_e // todo(amos): move elsewhere
{
	TRAVERSE_UNUSED_0 = 0,

	TRAVERSE_CROSS_GAP_SMALL,
	TRAVERSE_CLIMB_OBJECT_SMALL,
	TRAVERSE_CROSS_GAP_MEDIUM,

	TRAVERSE_UNUSED_4,
	TRAVERSE_UNUSED_5,
	TRAVERSE_UNUSED_6,

	TRAVERSE_CROSS_GAP_LARGE,

	TRAVERSE_CLIMB_WALL_MEDIUM,
	TRAVERSE_CLIMB_WALL_TALL,
	TRAVERSE_CLIMB_BUILDING,

	TRAVERSE_JUMP_SHORT,
	TRAVERSE_JUMP_MEDIUM,
	TRAVERSE_JUMP_LARGE,

	TRAVERSE_UNUSED_14,
	TRAVERSE_UNUSED_15,

	TRAVERSE_UNKNOWN_16, // USED!!!
	TRAVERSE_UNKNOWN_17, // USED!!!

	TRAVERSE_UNKNOWN_18,
	TRAVERSE_UNKNOWN_19, // NOTE: does not exists in MSET5!!!

	TRAVERSE_CLIMB_TARGET_SMALL,
	TRAVERSE_CLIMB_TARGET_LARGE,

	TRAVERSE_UNUSED_22,
	TRAVERSE_UNUSED_23,

	TRAVERSE_UNKNOWN_24,

	TRAVERSE_UNUSED_25,
	TRAVERSE_UNUSED_26,
	TRAVERSE_UNUSED_27,
	TRAVERSE_UNUSED_28,
	TRAVERSE_UNUSED_29,
	TRAVERSE_UNUSED_30,
	TRAVERSE_UNUSED_31,

	// These aren't traverse type!
	NUM_TRAVERSE_TYPES,
	INVALID_TRAVERSE_TYPE = DT_NULL_TRAVERSE_TYPE
};

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

#if DT_NAVMESH_SET_VERSION >= 9
enum EditorPolyAreas
{
	EDITOR_POLYAREA_JUMP,
	EDITOR_POLYAREA_GROUND,
	EDITOR_POLYAREA_RESERVED,
	EDITOR_POLYAREA_TRIGGER,
};
#else
enum EditorPolyAreas
{
	EDITOR_POLYAREA_GROUND,
	EDITOR_POLYAREA_JUMP,

	// NOTE: not sure if anything beyond EDITOR_POLYAREA_JUMP belongs to MSET5,
	// this needs to be confirmed, for now its been kept in for MSET5.
	EDITOR_POLYAREA_JUMP_REVERSE,
	EDITOR_POLYAREA_TRIGGER,
	EDITOR_POLYAREA_WALLJUMP_LEFT,
	EDITOR_POLYAREA_WALLJUMP_RIGHT,
	EDITOR_POLYAREA_WALLJUMP_LEFT_REVERSE,
	EDITOR_POLYAREA_WALLJUMP_RIGHT_REVERSE,
};
#endif

//enum EditorPolyFlags // note: original poly flags for reference.
//{
//	// Most common polygon flags.
//	EDITOR_POLYFLAGS_WALK		= 1<<0,		// Ability to walk (ground, grass, road)
//	EDITOR_POLYFLAGS_JUMP		= 1<<1,		// Ability to jump.
//	EDITOR_POLYFLAGS_DOOR		= 1<<2,		// Ability to move through doors.
//	EDITOR_POLYFLAGS_SWIM		= 1<<3,		// Ability to swim (water).
//	EDITOR_POLYFLAGS_DISABLED	= 1<<4,		// Disabled polygon
//	EDITOR_POLYFLAGS_ALL		= 0xffff	// All abilities.
//};

enum EditorPolyFlags
{
	// Most common polygon flags.
	EDITOR_POLYFLAGS_WALK				= 1<<0,		// Ability to walk (ground, grass, road).
	EDITOR_POLYFLAGS_SKIP				= 1<<1,     // Skipped during AIN script nodes generation, NavMesh_RandomPositions, dtNavMeshQuery::findLocalNeighbourhood, etc.
	EDITOR_POLYFLAGS_UNK0				= 1<<2,     // Unknown, most polygon have this flag.

	// Off-mesh connection flags
	EDITOR_POLYFLAGS_JUMP				= 1<<3,		// Ability to jump (exclusively used on off-mesh connection polygons).
	EDITOR_POLYFLAGS_JUMP_LINKED		= 1<<4,		// Off-mesh connections who's start and end verts link to other polygons need this flag.

	EDITOR_POLYFLAGS_UNK2				= 1<<5,		// Unknown, no use cases found yet.

	// Only used along with poly area 'EDITOR_POLYAREA_TRIGGER'.
	EDITOR_POLYFLAGS_OBSTACLE			= 1<<6,		// Unknown, used for small road blocks and other small but easily climbable obstacles.
	EDITOR_POLYFLAGS_UNK4				= 1<<7,		// Unknown, no use cases found yet.
	EDITOR_POLYFLAGS_DISABLED			= 1<<8,		// Used for ToggleNPCPathsForEntity. Also, see [r5apex_ds + 0xC96EA8]. Used for toggling poly's when a door closes during runtime.
													// Also used to disable poly's in the navmesh file itself when we do happen to build navmesh on lava or other very hazardous areas.
	EDITOR_POLYFLAGS_HAZARD				= 1<<9,		// see [r5apex_ds + 0xC96ED0], used for hostile objects such as electric fences.
	EDITOR_POLYFLAGS_DOOR				= 1<<10,	// See [r5apex_ds + 0xECBAE0], used for large bunker style doors (vertical and horizontal opening ones), perhaps also shooting cover hint?.
	EDITOR_POLYFLAGS_UNK8				= 1<<11,	// Unknown, no use cases found yet.
	EDITOR_POLYFLAGS_UNK9				= 1<<12,	// Unknown, no use cases found yet.
	EDITOR_POLYFLAGS_DOOR_BREACHABLE	= 1<<13,	// Used for doors that need to be breached, such as the Explosive Holds doors.

	EDITOR_POLYFLAGS_ALL				= 0xffff	// All abilities.
};

struct TraverseLinkPolyPair
{
	TraverseLinkPolyPair(const dtPoly* p1, const dtPoly* p2)
	{
		if (p1 > p2)
			rdSwap(p1, p2);

		poly1 = p1;
		poly2 = p2;
	}

	bool operator<(const TraverseLinkPolyPair& other) const
	{
		if (poly1 < other.poly1)
			return true;
		if (poly1 > other.poly1)
			return false;

		return poly2 < other.poly2;
	}

	const dtPoly* poly1;
	const dtPoly* poly2;
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
	bool m_traverseRayDynamicOffset;
	bool m_buildBvTree;

	int m_minTileBits;
	int m_maxTileBits;
	int m_tileSize;
	float m_cellSize;
	float m_cellHeight;
	float m_agentHeight;
	float m_agentRadius;
	float m_agentMaxClimb;
	float m_agentMaxSlope;
	float m_traverseRayExtraOffset;
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
	dtDisjointSet m_djs[DT_MAX_TRAVERSE_TABLES];
	std::map<TraverseLinkPolyPair, unsigned int> m_traverseLinkPolyMap;

	EditorDebugDraw m_dd;
	unsigned int m_navMeshDrawFlags;
	duDrawTraverseLinkParams m_traverseLinkParams;
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

	inline const char* getModelName() const { return m_modelName.c_str(); }

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

	void connectTileTraverseLinks(dtMeshTile* const baseTile, const bool linkToNeighbor); // Make private.
	bool createTraverseLinks();

	void createTraverseTableParams(dtTraverseTableCreateParams* params);

	void buildStaticPathingData();

	bool createStaticPathingData(const dtTraverseTableCreateParams* params);
	bool updateStaticPathingData(const dtTraverseTableCreateParams* params);

private:
	// Explicitly disabled copy constructor and copy assignment operator.
	Editor(const Editor&);
	Editor& operator=(const Editor&);
};


#endif // RECASTEDITOR_H
