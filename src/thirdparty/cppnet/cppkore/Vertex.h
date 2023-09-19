#pragma once

#include <cstdint>
#include <memory>
#include "Vector2.h"
#include "Vector3.h"

namespace Assets
{
	using namespace Math;	// For the vector classes

	// Represents a 3D vertex weight.
	class VertexWeight
	{
	public:
		// Initialize a default vertex weight.
		VertexWeight();
		// Initialize a vertex weight with the given information.
		VertexWeight(uint32_t Bone, float Value);

		// The index of the bone we are weighted to.
		uint32_t Bone;
		// The weighting attached to this bone.
		float Value;
	};

	// Represents a 3D vertex color (RGBA).
	class VertexColor
	{
	public:
		// Initialize a white vertex color.
		VertexColor();
		// Initialize a vertex color with the given information.
		VertexColor(uint8_t R, uint8_t G, uint8_t B, uint8_t A);

		// Array index operators
		uint8_t& operator[](uint8_t Index);
		const uint8_t& operator[](uint8_t Index) const;

	private:
		// Internal color data
		uint8_t _Colors[4];
	};

	// Represents a 3D vertex for a mesh.
	class Vertex
	{
	public:
		// Initializes a 3D vertex from the given address of the buffer.
		Vertex(uint8_t* Address, uint8_t MaxInfluence, uint8_t UVLayers);

		// Gets the position of this vertex.
		Vector3& Position();
		// Gets position of this vertex.
		const Vector3& Position() const;
		// Sets the position of this vertex.
		void SetPosition(Vector3 Value);

		// Gets the normal of this vertex.
		Vector3& Normal();
		// Gets normal of this vertex.
		const Vector3& Normal() const;
		// Sets the normal of this vertex.
		void SetNormal(Vector3 Value);

		// Gets the color of this vertex.
		VertexColor& Color();
		// Gets the color of this vertex.
		const VertexColor& Color() const;
		// Sets the color of this vertex.
		void SetColor(VertexColor Value);

		// Gets a UV layer of this vertex.
		Vector2& UVLayers(uint8_t Index);
		// Gets a UV layer of this vertex.
		const Vector2& UVLayers(uint8_t Index) const;
		// Sets a UV layer of this vertex.
		void SetUVLayer(Vector2 Value, uint8_t Index);

		// Gets the total UV layer count
		const uint8_t UVLayerCount() const;

		// Gets a weight for this vertex.
		VertexWeight& Weights(uint8_t Index);
		// Gets a weight for this vertex.
		const VertexWeight& Weights(uint8_t Index) const;
		// Sets a weight for this vertex.
		void SetWeight(VertexWeight Value, uint8_t Index);

		// Gets the total weight count
		const uint8_t WeightCount() const;
		
		// Increment operators
		Vertex& operator++();
		Vertex operator++(int);

		// Indirection operator
		operator Vertex*(void) const;

		// Equality operators
		bool operator==(const Vertex& Rhs) const;
		bool operator!=(const Vertex& Rhs) const;

	private:
		// Internal offset of the vertex in the buffer
		uint8_t* _Offset;
		// Internal sizes
		uint8_t _MaxInfluence;
		uint8_t _UVLayers;

		// Helper routine to calculate the size of a vertex
		uint32_t VertexSize() const;
	};
}