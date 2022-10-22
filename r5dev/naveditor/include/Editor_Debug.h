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

#ifndef RECASTSAMPLEDEBUG_H
#define RECASTSAMPLEDEBUG_H

#include "NavEditor/Include/Editor.h"
#include "Detour/Include/DetourNavMesh.h"
#include "Recast/Include/Recast.h"

/// Sample used for random debugging.
class Editor_Debug : public Editor
{
protected:
	rcCompactHeightfield* m_chf;
	rcContourSet* m_cset;
	rcPolyMesh* m_pmesh;

	float m_halfExtents[3];
	float m_center[3];
	float m_bmin[3], m_bmax[3];
	dtPolyRef m_ref;
	
public:
	Editor_Debug();
	virtual ~Editor_Debug();
	
	virtual void handleSettings();
	virtual void handleTools();
	virtual void handleDebugMode();
	virtual void handleClick(const float* s, const float* p, bool shift);
	virtual void handleToggle();
	virtual void handleRender();
	virtual void handleRenderOverlay(double* proj, double* model, int* view);
	virtual void handleMeshChanged(class InputGeom* geom);
	virtual bool handleBuild();

	virtual const float* getBoundsMin();
	virtual const float* getBoundsMax();

private:
	// Explicitly disabled copy constructor and copy assignment operator.
	Editor_Debug(const Editor_Debug&);
	Editor_Debug& operator=(const Editor_Debug&);
};


#endif // RECASTEDITOR_H
