#include "stdafx.h"
#include "CastNode.h"

namespace Assets::Exporters
{
	CastProperty::CastProperty()
		: Identifier(CastPropertyId::Byte)
	{
	}

	CastProperty::CastProperty(CastPropertyId Id, const char* Name)
		: Identifier(Id), Name(Name)
	{
		// All property names are lower-case
		this->Name = this->Name.ToLower();
	}

	const uint32_t CastProperty::Length() const
	{
		switch (this->Identifier)
		{
		case CastPropertyId::Byte: return sizeof(CastPropertyHeader) + this->Name.Length() + (sizeof(uint8_t) * this->IntegralValues.Count());
		case CastPropertyId::Short: return sizeof(CastPropertyHeader) + this->Name.Length() + (sizeof(uint16_t) * this->IntegralValues.Count());
		case CastPropertyId::Integer32: return sizeof(CastPropertyHeader) + this->Name.Length() + (sizeof(uint32_t) * this->IntegralValues.Count());
		case CastPropertyId::Integer64: return sizeof(CastPropertyHeader) + this->Name.Length() + (sizeof(uint64_t) * this->IntegralValues.Count());
		case CastPropertyId::Float: return sizeof(CastPropertyHeader) + this->Name.Length() + (sizeof(float) * this->IntegralValues.Count());
		case CastPropertyId::Double: return sizeof(CastPropertyHeader) + this->Name.Length() + (sizeof(double) * this->IntegralValues.Count());
		case CastPropertyId::Vector2 : return sizeof(CastPropertyHeader) + this->Name.Length() + (sizeof(Math::Vector2) * this->IntegralValues.Count());
		case CastPropertyId::Vector3: return sizeof(CastPropertyHeader) + this->Name.Length() + (sizeof(Math::Vector3) * this->IntegralValues.Count());
		case CastPropertyId::Vector4: return sizeof(CastPropertyHeader) + this->Name.Length() + (sizeof(Math::Quaternion) * this->IntegralValues.Count());
		case CastPropertyId::String: return sizeof(CastPropertyHeader) + this->Name.Length() + (StringValue.Length() + sizeof(uint8_t));
		default: return 0;
		}
	}

	void CastProperty::Write(IO::BinaryWriter& Writer) const
	{
		auto Size = (this->Identifier == CastPropertyId::String) ? 1 : this->IntegralValues.Count();
		Writer.Write<CastPropertyHeader>({this->Identifier, (uint16_t)this->Name.Length(), Size});
		Writer.Write(&this->Name[0], 0, this->Name.Length());
		
		switch (this->Identifier)
		{
		case CastPropertyId::Byte:
			for (auto& Value : this->IntegralValues)
				Writer.Write<uint8_t>(Value.Byte);
			break;
		case CastPropertyId::Short:
			for (auto& Value : this->IntegralValues)
				Writer.Write<uint16_t>(Value.Short);
			break;
		case CastPropertyId::Integer32:
			for (auto& Value : this->IntegralValues)
				Writer.Write<uint32_t>(Value.Integer32);
			break;
		case CastPropertyId::Integer64:
			for (auto& Value : this->IntegralValues)
				Writer.Write<uint64_t>(Value.Integer64);
			break;
		case CastPropertyId::Float:
			for (auto& Value : this->IntegralValues)
				Writer.Write<float>(Value.Float);
			break;
		case CastPropertyId::Double:
			for (auto& Value : this->IntegralValues)
				Writer.Write<double>(Value.Double);
			break;
		case CastPropertyId::Vector2:
			for (auto& Value : this->IntegralValues)
				Writer.Write<Math::Vector2>(Value.Vector2);
			break;
		case CastPropertyId::Vector3:
			for (auto& Value : this->IntegralValues)
				Writer.Write<Math::Vector3>(Value.Vector3);
			break;
		case CastPropertyId::Vector4:
			for (auto& Value : this->IntegralValues)
				Writer.Write<Math::Quaternion>(Value.Vector4);
			break;
		case CastPropertyId::String:
			Writer.WriteCString(this->StringValue);
			break;
		}
	}

	void CastProperty::AddByte(uint8_t Value)
	{
		this->IntegralValues.EmplaceBack(Value);
	}

	void CastProperty::AddShort(uint16_t Value)
	{
		this->IntegralValues.EmplaceBack(Value);
	}

	void CastProperty::AddInteger32(uint32_t Value)
	{
		this->IntegralValues.EmplaceBack(Value);
	}

	void CastProperty::AddInteger64(uint64_t Value)
	{
		this->IntegralValues.EmplaceBack(Value);
	}

	void CastProperty::AddFloat(float Value)
	{
		this->IntegralValues.EmplaceBack(Value);
	}

	void CastProperty::AddDouble(double Value)
	{
		this->IntegralValues.EmplaceBack(Value);
	}

	void CastProperty::AddVector2(Math::Vector2 Value)
	{
		this->IntegralValues.EmplaceBack(Value);
	}

	void CastProperty::AddVector3(Math::Vector3 Value)
	{
		this->IntegralValues.EmplaceBack(Value);
	}

	void CastProperty::AddVector4(Math::Quaternion Value)
	{
		this->IntegralValues.EmplaceBack(Value);
	}

	void CastProperty::SetString(const string& Value)
	{
		this->StringValue = Value;
	}

	CastNode::CastNode()
		: Identifier(CastId::Root), Hash(0)
	{
	}

	CastNode::CastNode(CastId Id)
		: Identifier(Id), Hash(0)
	{
	}

	CastNode::CastNode(CastId Id, uint64_t Hash)
		: Identifier(Id), Hash(Hash)
	{
	}

	const uint32_t CastNode::Length() const
	{
		uint32_t Result = sizeof(CastNodeHeader);

		for (auto& Child : this->Children)
			Result += Child.Length();
		for (auto& Property : this->Properties)
			Result += Property.Length();

		return Result;
	}

	void CastNode::Write(IO::BinaryWriter& Writer) const
	{
		Writer.Write<CastNodeHeader>({this->Identifier, this->Length(), this->Hash, this->Properties.Count(), this->Children.Count()});

		for (auto& Prop : Properties)
			Prop.Write(Writer);
		for (auto& Child : Children)
			Child.Write(Writer);
	}

	CastPropertyUnion::CastPropertyUnion()
		: Vector4(0,0,0,0)
	{
	}

	CastPropertyUnion::CastPropertyUnion(uint8_t Value)
		: Byte(Value)
	{
	}

	CastPropertyUnion::CastPropertyUnion(uint16_t Value)
		: Short(Value)
	{
	}

	CastPropertyUnion::CastPropertyUnion(uint32_t Value)
		: Integer32(Value)
	{
	}

	CastPropertyUnion::CastPropertyUnion(uint64_t Value)
		: Integer64(Value)
	{
	}

	CastPropertyUnion::CastPropertyUnion(float Value)
		: Float(Value)
	{
	}

	CastPropertyUnion::CastPropertyUnion(double Value)
		: Double(Value)
	{
	}

	CastPropertyUnion::CastPropertyUnion(Math::Vector2 Value)
		: Vector2(Value)
	{
	}

	CastPropertyUnion::CastPropertyUnion(Math::Vector3 Value)
		: Vector3(Value)
	{
	}

	CastPropertyUnion::CastPropertyUnion(Math::Quaternion Value)
		: Vector4(Value)
	{
	}
}