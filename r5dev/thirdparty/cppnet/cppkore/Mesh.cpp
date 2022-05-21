#include "stdafx.h"
#include "Mesh.h"

namespace Assets
{
	Mesh::Mesh()
		: Mesh(0, 0, 1, 1)
	{
	}

	Mesh::Mesh(uint8_t MaxInfluence, uint8_t UVLayers)
		: Mesh(0, 0, MaxInfluence, UVLayers)
	{
	}

	Mesh::Mesh(uint32_t VertexCount, uint8_t MaxInfluence, uint8_t UVLayers)
		: Mesh(VertexCount, 0, MaxInfluence, UVLayers)
	{
	}

	Mesh::Mesh(uint32_t VertexCount, uint32_t FaceCount, uint8_t MaxInfluence, uint8_t UVLayers)
		: Vertices(VertexCount, MaxInfluence, UVLayers), Faces(FaceCount)
	{
	}
}
