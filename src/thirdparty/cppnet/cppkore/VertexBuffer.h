#pragma once

#include <cstdint>
#include "Vertex.h"

namespace Assets
{
	using namespace Math;	// For the vector classes

	// A vertex buffer used in a 3D mesh.
	class VertexBuffer
	{
	public:
		// Initialize a blank vertex buffer.
		VertexBuffer();
		// Initialize a vertex buffer with the given max influence and UV layer count.
		VertexBuffer(uint8_t MaxInfluence, uint8_t UVLayers);
		// Initialize a vertex buffer with an initial size, the given max influence, and UV layer count.
		VertexBuffer(uint32_t InitialSize, uint8_t MaxInfluence, uint8_t UVLayers);

		// VertexBuffer copy operator.
		VertexBuffer& operator=(const VertexBuffer& Rhs);

		// Destroys all vertex buffer information.
		~VertexBuffer();

		// Creates a new vertex with the provided position, normal, and color.
		void EmplaceBack(Vector3 Position, Vector3 Normal, VertexColor Color);
		// Creates a new vertex with the provided position, normal, and color and returns the result.
		Vertex Emplace(Vector3 Position, Vector3 Normal, VertexColor Color);
		// Creates a new vertex with the provided position, normal, color, and UV layer.
		void EmplaceBack(Vector3 Position, Vector3 Normal, VertexColor Color, Vector2 UVLayer);
		// Creates a new vertex with the provided position, normal, color, and UV layer and returns the result.
		Vertex Emplace(Vector3 Position, Vector3 Normal, VertexColor Color, Vector2 UVLayer);

		// Clears all vertices from the buffer
		void Clear();
		// Removes a vertex at the index
		void RemoveAt(uint32_t Index);

		// Gets the total UV layer count
		const uint8_t UVLayerCount() const;
		// Gets the total weight count
		const uint8_t WeightCount() const;

		// Returns the count of vertices in the buffer
		uint32_t Count() const;
		// Returns whether or not the buffer is empty
		bool Empty() const;

		// Iterator definitions, for for(& :) loop
		Vertex begin() const noexcept;
		Vertex end() const noexcept;

		// Array index operators
		Vertex operator[](uint32_t Index);
		const Vertex operator[](uint32_t Index) const;

	private:
		// Internal variables that effect the size of the buffer
		uint8_t _MaxInfluence;
		uint8_t _UVLayers;

		// Internal variables for the buffer
		uint8_t* _Buffer;
		uint32_t _BufferSize;
		uint32_t _StoreSize;

		// Helper routine to allocate the buffer
		void EnsureCapacity(uint32_t Capacity);
		// Helper routine to calculate the size of a vertex
		uint32_t VertexSize() const;
	};
}