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

#ifndef RECASTEDITORSOLOMESH_H
#define RECASTEDITORSOLOMESH_H

#include "Recast/Include/Recast.h"
#include "Detour/Include/DetourNavMesh.h"
#include "NavEditor/Include/Editor.h"
#include "NavEditor/Include/Editor_Common.h"

class Editor_SoloMesh : public Editor_StaticTileMeshCommon
{
protected:
	void cleanup();

public:
	Editor_SoloMesh();
	virtual ~Editor_SoloMesh();
	
	virtual void handleSettings();
	virtual void handleTools();
	virtual void handleDebugMode();
	
	virtual void handleRender();
	virtual void handleRenderOverlay(double* proj, double* model, int* view);
	virtual void handleMeshChanged(class InputGeom* geom);
	virtual bool handleBuild();

private:
	// Explicitly disabled copy constructor and copy assignment operator.
	Editor_SoloMesh(const Editor_SoloMesh&);
	Editor_SoloMesh& operator=(const Editor_SoloMesh&);
};


#endif // RECASTEDITORSOLOMESH_H
