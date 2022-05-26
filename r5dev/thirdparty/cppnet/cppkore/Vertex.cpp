#include "stdafx.h"
#include "Vertex.h"

namespace Assets
{
	Vertex::Vertex(uint8_t* Address, uint8_t MaxInfluence, uint8_t UVLayers)
		: _Offset(Address), _MaxInfluence(MaxInfluence), _UVLayers(UVLayers)
	{
	}

	Vector3& Vertex::Position()
	{
		return *((Vector3*)this->_Offset);
	}

	const Vector3& Vertex::Position() const
	{
		return *((Vector3*)this->_Offset);
	}

	void Vertex::SetPosition(Vector3 Value)
	{
		std::memcpy(this->_Offset, &Value, sizeof(Vector3));
	}

	Vector3& Vertex::Normal()
	{
		return *((Vector3*)(this->_Offset + sizeof(Vector3)));
	}

	const Vector3& Vertex::Normal() const
	{
		return *((Vector3*)(this->_Offset + sizeof(Vector3)));
	}

	void Vertex::SetNormal(Vector3 Value)
	{
		std::memcpy(this->_Offset + sizeof(Vector3), &Value, sizeof(Vector3));
	}

	VertexColor& Vertex::Color()
	{
		return *((VertexColor*)(this->_Offset + (sizeof(Vector3) * 2)));
	}

	const VertexColor& Vertex::Color() const
	{
		return *((VertexColor*)(this->_Offset + (sizeof(Vector3) * 2)));
	}

	void Vertex::SetColor(VertexColor Value)
	{
		std::memcpy(this->_Offset + (sizeof(Vector3) * 2), &Value, sizeof(VertexColor));
	}

	Vector2& Vertex::UVLayers(uint8_t Index)
	{
		return *((Vector2*)(this->_Offset + (sizeof(Vector3) * 2) + sizeof(VertexColor) + (sizeof(Vector2) * Index)));
	}

	const Vector2& Vertex::UVLayers(uint8_t Index) const
	{
		return *((Vector2*)(this->_Offset + (sizeof(Vector3) * 2) + sizeof(VertexColor) + (sizeof(Vector2) * Index)));
	}

	void Vertex::SetUVLayer(Vector2 Value, uint8_t Index)
	{
		std::memcpy(this->_Offset + (sizeof(Vector3) * 2) + sizeof(VertexColor) + (sizeof(Vector2) * Index), &Value, sizeof(Vector2));
	}

	const uint8_t Vertex::UVLayerCount() const
	{
		return this->_UVLayers;
	}

	VertexWeight& Vertex::Weights(uint8_t Index)
	{
		return *((VertexWeight*)(this->_Offset + (sizeof(Vector3) * 2) + sizeof(VertexColor) + (sizeof(Vector2) * this->_UVLayers) + (sizeof(VertexWeight) * Index)));
	}

	const VertexWeight& Vertex::Weights(uint8_t Index) const
	{
		return *((VertexWeight*)(this->_Offset + (sizeof(Vector3) * 2) + sizeof(VertexColor) + (sizeof(Vector2) * this->_UVLayers) + (sizeof(VertexWeight) * Index)));
	}

	void Vertex::SetWeight(VertexWeight Value, uint8_t Index)
	{
		// When we set the weight, adjust the internal max count...
		if ((Index + 1) > WeightCount())
			*((uint8_t*)(this->_Offset + (sizeof(Vector3) * 2) + sizeof(VertexColor) + (sizeof(Vector2) * this->_UVLayers) + (sizeof(VertexWeight) * this->_MaxInfluence))) = Index + 1;

		// Copy the weight to the buffer offset
		std::memcpy(this->_Offset + (sizeof(Vector3) * 2) + sizeof(VertexColor) + (sizeof(Vector2) * this->_UVLayers) + (sizeof(VertexWeight) * Index), &Value, sizeof(VertexWeight));
	}

	const uint8_t Vertex::WeightCount() const
	{
		// This is the total used weight count, as in, maximum index used by SetWeight();
		return *((uint8_t*)(this->_Offset + (sizeof(Vector3) * 2) + sizeof(VertexColor) + (sizeof(Vector2) * this->_UVLayers) + (sizeof(VertexWeight) * this->_MaxInfluence)));
	}

	Vertex& Vertex::operator++()
	{
		this->_Offset += this->VertexSize();
		return *this;
	}

	Vertex Vertex::operator++(int)
	{
		auto Result = (*this);
		++(*this);
		return Result;
	}

	Vertex::operator Vertex*(void) const
	{
		return (Vertex*)this;
	}

	bool Vertex::operator==(const Vertex& Rhs) const
	{
		return (this->_Offset == Rhs._Offset);
	}

	bool Vertex::operator!=(const Vertex& Rhs) const
	{
		return !(*this == Rhs);
	}

	uint32_t Vertex::VertexSize() const
	{
		return (sizeof(Vector3) * 2) + (sizeof(Vector2) * this->_UVLayers) + (sizeof(VertexWeight) * this->_MaxInfluence) + sizeof(VertexColor) + sizeof(uint8_t);
	}

	VertexColor::VertexColor() 
		: _Colors{ 255, 255, 255, 255 }
	{
	}

	VertexColor::VertexColor(uint8_t R, uint8_t G, uint8_t B, uint8_t A)
		: _Colors{ R, G, B, A }
	{
	}

	uint8_t& VertexColor::operator[](uint8_t Index)
	{
		return this->_Colors[Index];
	}

	const uint8_t& VertexColor::operator[](uint8_t Index) const
	{
		return this->_Colors[Index];
	}

	VertexWeight::VertexWeight()
		: Bone(0), Value(1)
	{
	}

	VertexWeight::VertexWeight(uint32_t Bone, float Value)
		: Bone(Bone), Value(Value)
	{
	}
}
