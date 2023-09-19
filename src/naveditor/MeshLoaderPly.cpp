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
#include "NavEditor/Include/MeshLoaderPly.h"

bool rcMeshLoaderPly::load(const std::string& filename)
{
	using namespace std;

	ifstream input(filename,std::ios::binary);
	
	if (!input.is_open())
		return false;
//we expect and only support!
/*
ply
format binary_little_endian 1.0
element vertex %d
property float x
property float y
property float z
element face %d
property list uchar int vertex_index
end_header
*/
	std::string line;
	getline(input, line);
	if (line != "ply")
		return false;
	getline(input, line);
	if (line != "format binary_little_endian 1.0")
		return false;
	while (true)
	{
		input >> line;
		if (line == "element")
		{
			input >> line;
			if (line == "vertex")
			{
				input >> m_vertCount;
				m_verts.resize(m_vertCount * 3);
			}
			else if (line == "face")
			{	
				input >> m_triCount;
				m_tris.resize(m_triCount * 3);
			}
		}
		else if (line == "end_header")
		{
			break;
		}
		else
		{
			//skip rest of the line
			getline(input, line);
		}
	}

	//skip newline
	input.seekg(1, ios_base::cur);

	for (size_t i = 0; i < m_vertCount; i++)
	{
		//TODO: m_scale?
		if (m_flipAxis)
		{
			input.read((char*)&m_verts[i * 3 + 0], sizeof(float));
			input.read((char*)&m_verts[i * 3 + 2], sizeof(float));
			input.read((char*)&m_verts[i * 3 + 1], sizeof(float));
			m_verts[i * 3 + 1] *= -1;
		}
		else
		{
			input.read((char*)&m_verts[i * 3 + 0], sizeof(float));
			input.read((char*)&m_verts[i * 3 + 1], sizeof(float));
			input.read((char*)&m_verts[i * 3 + 2], sizeof(float));
		}
		
	}

	for (size_t i = 0; i < m_triCount; i++)
	{
		char count;
		input.read(&count, 1);
		if (count != 3)
			return false;
		if (m_flipTris)
		{
			input.read((char*)&m_tris[i * 3 + 0], sizeof(int));
			input.read((char*)&m_tris[i * 3 + 2], sizeof(int));
			input.read((char*)&m_tris[i * 3 + 1], sizeof(int));
		}
		else
		{
			input.read((char*)&m_tris[i * 3 + 0], sizeof(int));
			input.read((char*)&m_tris[i * 3 + 1], sizeof(int));
			input.read((char*)&m_tris[i * 3 + 2], sizeof(int));
		}
	}

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
}
