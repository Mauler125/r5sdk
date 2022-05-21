#pragma once

#include <cstdint>
#include "Vector2.h"
#include "Vector3.h"
#include "Quaternion.h"
#include "StringBase.h"
#include "ListBase.h"
#include "BinaryWriter.h"

namespace Assets::Exporters
{
	enum class CastId : uint32_t
	{
		Root = 0x746F6F72,
		Model = 0x6C646F6D,
		Mesh = 0x6873656D,
		Skeleton = 0x6C656B73,
		Bone = 0x656E6F62,
		Animation = 0x6D696E61,
		Curve = 0x76727563,
		NotificationTrack = 0x6669746E,
		Material = 0x6C74616D,
		File = 0x656C6966,
	};

	enum class CastPropertyId : uint16_t
	{
		Byte = 'b',			// <uint8_t>
		Short = 'h',		// <uint16_t>
		Integer32 = 'i',	// <uint32_t>
		Integer64 = 'l',	// <uint64_t>

		Float = 'f',		// <float>
		Double = 'd',		// <double>

		String = 's',		// Null terminated UTF-8 string

		Vector2 = 'v2',		// Float precision vector XY
		Vector3 = 'v3',		// Float precision vector XYZ
		Vector4 = 'v4'		// Float precision vector XYZW
	};

	struct CastNodeHeader
	{
		CastId Identifier;		// Used to signify which class this node uses
		uint32_t NodeSize;		// Size of all data and sub data following the node
		uint64_t NodeHash;		// Unique hash, like an id, used to link nodes together
		uint32_t PropertyCount;	// The count of properties
		uint32_t ChildCount;	// The count of direct children nodes

		// We must read until the node size hits, and that means we are done.
		// The nodes are in a stack layout, so it's easy to load, FILO order.
	};

	struct CastPropertyHeader
	{
		CastPropertyId Identifier;	// The element type of this property
		uint16_t NameSize;			// The size of the name of this property
		uint32_t ArrayLength;		// The number of elements this property contains (1 for single)

		// Following is UTF-8 string lowercase, size of namesize, NOT null terminated
		// cast_property[ArrayLength] array of data
	};

	union CastPropertyUnion
	{
		uint8_t Byte;
		uint16_t Short;
		uint32_t Integer32;
		uint64_t Integer64;

		float Float;
		double Double;

		Math::Vector2 Vector2;
		Math::Vector3 Vector3;
		Math::Quaternion Vector4;

		CastPropertyUnion();

		explicit CastPropertyUnion(uint8_t Value);
		explicit CastPropertyUnion(uint16_t Value);
		explicit CastPropertyUnion(uint32_t Value);
		explicit CastPropertyUnion(uint64_t Value);
		explicit CastPropertyUnion(float Value);
		explicit CastPropertyUnion(double Value);
		explicit CastPropertyUnion(Math::Vector2 Value);
		explicit CastPropertyUnion(Math::Vector3 Value);
		explicit CastPropertyUnion(Math::Quaternion Value);
	};

	static_assert(sizeof(CastNodeHeader) == 0x18, "CastNode header size mismatch");
	static_assert(sizeof(CastPropertyHeader) == 0x8, "CastProperty header size mismatch");
	static_assert(sizeof(CastPropertyUnion) == 0x10, "CastProperty union size mismatch");

	class CastProperty
	{
	public:
		CastProperty();
		explicit CastProperty(CastPropertyId Id, const char* Name);

		const uint32_t Length() const;

		void Write(IO::BinaryWriter& Writer) const;

		void AddByte(uint8_t Value);
		void AddShort(uint16_t Value);
		void AddInteger32(uint32_t Value);
		void AddInteger64(uint64_t Value);

		void AddFloat(float Value);
		void AddDouble(double Value);

		void AddVector2(Math::Vector2 Value);
		void AddVector3(Math::Vector3 Value);
		void AddVector4(Math::Quaternion Value);

		void SetString(const string& Value);

		CastPropertyId Identifier;
		string Name;

	private:
		List<CastPropertyUnion> IntegralValues;
		string StringValue;
	};

	class CastNode
	{
	public:
		CastNode();
		CastNode(CastId Id);
		CastNode(CastId Id, uint64_t Hash);

		const uint32_t Length() const;

		void Write(IO::BinaryWriter& Writer) const;

		CastId Identifier;
		uint64_t Hash;

		List<CastProperty> Properties;
		List<CastNode> Children;
	};
}