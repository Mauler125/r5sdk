#pragma once

#include <cstdint>
#include "ListBase.h"

#include "Face.h"
#include "Vertex.h"
#include "VertexBuffer.h"

namespace Assets
{
	// A face buffer used in a 3D mesh.
	typedef List<Face> FaceBuffer;
	// A buffer of indices for each UV layer in the mesh.
	typedef List<int32_t> MatIndexBuffer;

	// A container class that holds 3D mesh data.
	class Mesh
	{
	public:
		// Initialize a blank 3D mesh.
		Mesh();
		// Initialize a mesh with the given maximum influence and UV layers.
		Mesh(uint8_t MaxInfluence, uint8_t UVLayers);
		// Initialize a mesh with the given vertex count, maximum influence and UV layers.
		Mesh(uint32_t VertexCount, uint8_t MaxInfluence, uint8_t UVLayers);
		// Initialize a mesh with the given vertex count, face count, maximum influence and UV layers.
		Mesh(uint32_t VertexCount, uint32_t FaceCount, uint8_t MaxInfluence, uint8_t UVLayers);
		// Destroy all 3D mesh data.
		~Mesh() = default;

		// A collection of 3D vertices for this mesh.
		VertexBuffer Vertices;
		// A collection of 3D faces for this mesh.
		FaceBuffer Faces;
		// A collection of material indices for each UV layer.
		MatIndexBuffer MaterialIndices;
	};
}