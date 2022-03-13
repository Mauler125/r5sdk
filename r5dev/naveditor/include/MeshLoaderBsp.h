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
#ifndef MESHLOADER_BSP
#define MESHLOADER_BSP

#include <string>
#include <vector>
#include <NavEditor/Include/MeshLoaderObj.h>

class rcMeshLoaderBsp:public IMeshLoader
{
public:
	rcMeshLoaderBsp() = default;
	// Explicitly disabled copy constructor and copy assignment operator.
	rcMeshLoaderBsp(const rcMeshLoaderBsp&) =delete;
	rcMeshLoaderBsp& operator=(const rcMeshLoaderBsp&) =delete;

	bool load(const std::string& fileName);

	const float* getVerts() const { return m_verts.data(); }
	const float* getNormals() const { return m_normals.data(); }
	const int* getTris() const { return m_tris.data(); }
	int getVertCount() const { return m_vertCount; }
	int getTriCount() const { return m_triCount; }
	const std::string& getFileName() const { return m_filename; }

private:
	std::string m_filename;
	float m_scale = 1.0;
	std::vector<float> m_verts;
	std::vector<int>  m_tris;
	std::vector<float> m_normals;
	int m_vertCount = 0;
	int m_triCount = 0;

	
};

#endif // MESHLOADER_BSP
