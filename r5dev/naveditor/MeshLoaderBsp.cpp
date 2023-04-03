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

#include "Pch.h"
#include "NavEditor/Include/MeshLoaderBsp.h"

bool rcMeshLoaderBsp::load(const std::string& /*filename*/)
{
#if 0
	//we expect lumps to be in same dir

	using namespace std;
	std::string vertex_lump_path = filename + ".0003.bsp_lump";
	auto vf=fopen(vertex_lump_path.c_str(), "rb");
	
	if (!vf)
		return false;

	fseek(vf, 0, SEEK_END);
	int fsize=ftell(vf);
	fseek(vf, 0, SEEK_SET);

	m_verts.resize(fsize / sizeof(float));
	if (m_verts[i] = fread(&m_verts[i], 4, m_verts.size(), vf) != m_verts.size())
	{
		fclose(vf);
		return false;
	}
	fclose(vf);

	//TODO: triangles


	// Calculate normals.
	m_normals.resize(m_triCount*3);
	for (int i = 0; i < m_triCount*3; i += 3)
	{
		const float* v0 = &m_verts[m_tris[i]*3];
		const float* v1 = &m_verts[m_tris[i+1]*3];
		const float* v2 = &m_verts[m_tris[i+2]*3];
		float e0[3], e1[3];
		for (int j = 0; j < 3; ++j)
		{
			e0[j] = v1[j] - v0[j];
			e1[j] = v2[j] - v0[j];
		}
		float* n = &m_normals[i];
		n[0] = e0[1]*e1[2] - e0[2]*e1[1];
		n[1] = e0[2]*e1[0] - e0[0]*e1[2];
		n[2] = e0[0]*e1[1] - e0[1]*e1[0];
		float d = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
		if (d > 0)
		{
			d = 1.0f/d;
			n[0] *= d;
			n[1] *= d;
			n[2] *= d;
		}
	}
	
	m_filename = filename;
	return true;
#endif
	return false;
}
