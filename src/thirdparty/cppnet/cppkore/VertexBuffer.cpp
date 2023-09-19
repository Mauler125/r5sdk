#include "stdafx.h"
#include "VertexBuffer.h"

namespace Assets
{
	VertexBuffer::VertexBuffer()
		: _Buffer(nullptr), _BufferSize(0), _StoreSize(0), _MaxInfluence(1), _UVLayers(1)
	{
	}

	VertexBuffer::VertexBuffer(uint8_t MaxInfluence, uint8_t UVLayers)
		: _Buffer(nullptr), _BufferSize(0), _StoreSize(0), _MaxInfluence(MaxInfluence), _UVLayers(UVLayers)
	{
	}

	VertexBuffer::VertexBuffer(uint32_t InitialSize, uint8_t MaxInfluence, uint8_t UVLayers)
		: _Buffer(nullptr), _BufferSize(0), _StoreSize(0), _MaxInfluence(MaxInfluence), _UVLayers(UVLayers)
	{
		this->EnsureCapacity(InitialSize);
		this->_StoreSize = 0;	// Must reset this so that we start from 0
	}

	VertexBuffer& VertexBuffer::operator=(const VertexBuffer& Rhs)
	{
		if (Rhs._Buffer)
		{
			auto vSize = Rhs.VertexSize();
			this->_Buffer = new uint8_t[vSize * Rhs._BufferSize];
			this->_StoreSize = Rhs._StoreSize;
			this->_BufferSize = Rhs._BufferSize;

			std::memcpy(this->_Buffer, Rhs._Buffer, vSize * Rhs._BufferSize);
		}

		this->_MaxInfluence = Rhs._MaxInfluence;
		this->_UVLayers = Rhs._UVLayers;

		return *this;
	}

	VertexBuffer::~VertexBuffer()
	{
		if (this->_Buffer)
			delete[] this->_Buffer;

		this->_Buffer = nullptr;
		this->_BufferSize = 0;
		this->_StoreSize = 0;
	}

	void VertexBuffer::EmplaceBack(Vector3 Position, Vector3 Normal, VertexColor Color)
	{
		uint64_t nPos = this->_StoreSize;
		this->EnsureCapacity((uint32_t)nPos + 1);

		// Copy the values at expected vertex offset
		nPos *= this->VertexSize();

		// Copy them
		std::memcpy(this->_Buffer + nPos, &Position, sizeof(Vector3));
		std::memcpy(this->_Buffer + nPos + sizeof(Vector3), &Normal, sizeof(Vector3));
		std::memcpy(this->_Buffer + nPos + (sizeof(Vector3) * 2), &Color, sizeof(VertexColor));
	}

	Vertex VertexBuffer::Emplace(Vector3 Position, Vector3 Normal, VertexColor Color)
	{
		uint64_t nPos = this->_StoreSize;
		this->EnsureCapacity((uint32_t)nPos + 1);

		// Copy the values at expected vertex offset
		nPos *= this->VertexSize();

		// Copy them
		std::memcpy(this->_Buffer + nPos, &Position, sizeof(Vector3));
		std::memcpy(this->_Buffer + nPos + sizeof(Vector3), &Normal, sizeof(Vector3));
		std::memcpy(this->_Buffer + nPos + (sizeof(Vector3) * 2), &Color, sizeof(VertexColor));

		// Return vertex controller
		return Vertex(this->_Buffer + nPos, this->_MaxInfluence, this->_UVLayers);
	}

	void VertexBuffer::EmplaceBack(Vector3 Position, Vector3 Normal, VertexColor Color, Vector2 UVLayer)
	{
		uint64_t nPos = this->_StoreSize;
		this->EnsureCapacity((uint32_t)nPos + 1);

		// Copy the values at expected vertex offset
		nPos *= this->VertexSize();

		// Copy them
		std::memcpy(this->_Buffer + nPos, &Position, sizeof(Vector3));
		std::memcpy(this->_Buffer + nPos + sizeof(Vector3), &Normal, sizeof(Vector3));
		std::memcpy(this->_Buffer + nPos + (sizeof(Vector3) * 2), &Color, sizeof(VertexColor));
		std::memcpy(this->_Buffer + nPos + (sizeof(Vector3) * 2) + sizeof(VertexColor), &UVLayer, sizeof(Vector2));
	}

	Vertex VertexBuffer::Emplace(Vector3 Position, Vector3 Normal, VertexColor Color, Vector2 UVLayer)
	{
		uint64_t nPos = this->_StoreSize;
		this->EnsureCapacity((uint32_t)nPos + 1);

		// Copy the values at expected vertex offset
		nPos *= this->VertexSize();

		// Copy them
		std::memcpy(this->_Buffer + nPos, &Position, sizeof(Vector3));
		std::memcpy(this->_Buffer + nPos + sizeof(Vector3), &Normal, sizeof(Vector3));
		std::memcpy(this->_Buffer + nPos + (sizeof(Vector3) * 2), &Color, sizeof(VertexColor));
		std::memcpy(this->_Buffer + nPos + (sizeof(Vector3) * 2) + sizeof(VertexColor), &UVLayer, sizeof(Vector2));

		// Return vertex controller
		return Vertex(this->_Buffer + nPos, this->_MaxInfluence, this->_UVLayers);
	}

	void VertexBuffer::Clear()
	{
		if (this->_Buffer)
			delete[] this->_Buffer;

		this->_Buffer = nullptr;
		this->_BufferSize = 0;
		this->_StoreSize = 0;
	}

	void VertexBuffer::RemoveAt(uint32_t Index)
	{
		if (Index >= this->_StoreSize)
			return;

		auto vSize = this->VertexSize();

		this->_StoreSize--;
		if (Index < this->_StoreSize)
			std::memcpy(this->_Buffer + Index, this->_Buffer + (Index + 1), (this->_StoreSize - Index) * vSize);

		std::memset(&this->_Buffer[this->_StoreSize], 0, vSize);
	}

	const uint8_t VertexBuffer::UVLayerCount() const
	{
		return this->_UVLayers;
	}

	const uint8_t VertexBuffer::WeightCount() const
	{
		return this->_MaxInfluence;
	}

	uint32_t VertexBuffer::Count() const
	{
		return this->_StoreSize;
	}

	bool VertexBuffer::Empty() const
	{
		return (this->_StoreSize == 0);
	}

	Vertex VertexBuffer::begin() const noexcept
	{
		return Vertex(this->_Buffer, this->_MaxInfluence, this->_UVLayers);
	}

	Vertex VertexBuffer::end() const noexcept
	{
		return Vertex(this->_Buffer + (this->_StoreSize * this->VertexSize()), this->_MaxInfluence, this->_UVLayers);
	}

	Vertex VertexBuffer::operator[](uint32_t Index)
	{
		return Vertex(this->_Buffer + (Index * this->VertexSize()), this->_MaxInfluence, this->_UVLayers);
	}

	const Vertex VertexBuffer::operator[](uint32_t Index) const
	{
		return Vertex(this->_Buffer + (Index * this->VertexSize()), this->_MaxInfluence, this->_UVLayers);
	}

	void VertexBuffer::EnsureCapacity(uint32_t Capacity)
	{
		// Ensure that we have a proper buffer size for the string here, this is in units, NOT bytes...
		// Check to ensure we aren't wasting our time first...
		if (Capacity <= this->_BufferSize)
		{
			this->_StoreSize = Capacity;
			return;
		}

		// Pre-calculate the size of a single vertex (Position Normal) (UVLayers) (Weights) (Colors)
		auto vSize = this->VertexSize();

		auto nCapacity = Capacity;
		if (nCapacity < 16)
			nCapacity = 16;

		if (nCapacity < (this->_BufferSize + (this->_BufferSize / 2)))
			nCapacity = (this->_BufferSize + (this->_BufferSize / 2));

		auto tBuffer = this->_Buffer;
		this->_Buffer = new uint8_t[nCapacity * vSize]();

		if (tBuffer)
		{
			std::memcpy(this->_Buffer, tBuffer, this->_BufferSize * vSize);
			delete[] tBuffer;
		}

		this->_BufferSize = nCapacity;
		this->_StoreSize = Capacity;
	}

	uint32_t VertexBuffer::VertexSize() const
	{
		return (sizeof(Vector3) * 2) + (sizeof(Vector2) * this->_UVLayers) + (sizeof(VertexWeight) * this->_MaxInfluence) + sizeof(VertexColor) + sizeof(uint8_t);
	}
}
