#include "stdafx.h"
#include "Settings.h"

namespace System
{
	// Our protected string key
	constexpr unsigned char KeyData[] = { 0x4C, 0x39, 0x56, 0x75, 0x42, 0x52, 0x65, 0x75, 0x70, 0x35, 0x31, 0x77, 0x4C, 0x51, 0x64, 0x61 };

	SettingsObject::SettingsObject()
		: Key(), Type(SettingType::Boolean), Values()
	{
	}

	SettingsObject::SettingsObject(string Key, SettingType Type, uint8_t* Value)
		: Key(Key), Type(Type), Values()
	{
		switch (Type)
		{
		case SettingType::Boolean:
			Values.Boolean = *(bool*)Value;
			break;
		case SettingType::Integer:
			Values.Integer = *(uint32_t*)Value;
			break;
		case SettingType::Integer64:
			Values.Integer64 = *(uint64_t*)Value;
			break;
		case SettingType::String:
		case SettingType::ProtectedString:
		{
			auto Len = strlen((char*)Value);
			Values.String = new char[Len + 1]{};
			std::memcpy(Values.String, Value, Len);
		}
		break;
		}
	}

	SettingsObject::~SettingsObject()
	{
		if (Values.String != NULL && (Type == SettingType::ProtectedString))
			delete[] Values.String;
	}

	void Settings::Load(const string& Path)
	{
		auto Reader = IO::BinaryReader(IO::File::OpenRead(Path));

		if (Reader.Read<uint32_t>() != 0x4746434B)
			return;
		if (Reader.Read<uint16_t>() != 0x1)
			return;

		auto KeyCount = Reader.Read<uint32_t>();

		for (uint32_t i = 0; i < KeyCount; i++)
		{
			auto Type = (SettingType)Reader.Read<char>();
			auto Key = Reader.ReadNetString();

			switch (Type)
			{
			case SettingType::Boolean:
			{
				auto Val = Reader.Read<bool>();
				_Settings.EmplaceBack(Key, Type, (uint8_t*)&Val);
			}
			break;
			case SettingType::Integer:
			{
				auto Val = Reader.Read<uint32_t>();
				_Settings.EmplaceBack(Key, Type, (uint8_t*)&Val);
			}
			break;
			case SettingType::Integer64:
			{
				auto Val = Reader.Read<uint64_t>();
				_Settings.EmplaceBack(Key, Type, (uint8_t*)&Val);
			}
			break;
			case SettingType::String:
			{
				auto Val = Reader.ReadNetString();
				_Settings.EmplaceBack(Key, Type, (uint8_t*)&Val[0]);
			}
			break;
			case SettingType::ProtectedString:
			{
				auto Val = Reader.ReadNetString();

				for (uint32_t i = 0; i < Val.Length(); i++)
					Val[i] ^= KeyData[i % sizeof(KeyData)];

				_Settings.EmplaceBack(Key, Type, (uint8_t*)&Val[0]);
			}
			break;

			default:
				return;
			}
		}
	}

	void Settings::Save(const string& Path)
	{
		auto Writer = IO::BinaryWriter(IO::File::Create(Path));

		Writer.Write<uint32_t>(0x4746434B);
		Writer.Write<uint16_t>(0x1);
		Writer.Write<uint32_t>(this->_Settings.Count());

		for (auto& Setting : _Settings)
		{
			Writer.Write<char>((char)Setting.Type);
			Writer.WriteNetString(Setting.Key);

			switch (Setting.Type)
			{
			case SettingType::Boolean:
				Writer.Write<bool>(Setting.Values.Boolean);
				break;
			case SettingType::Integer:
				Writer.Write<uint32_t>(Setting.Values.Integer);
				break;
			case SettingType::Integer64:
				Writer.Write<uint64_t>(Setting.Values.Integer64);
				break;
			case SettingType::String:
				Writer.WriteNetString(Setting.Values.String);
				break;
			case SettingType::ProtectedString:
			{
				auto Value = string(Setting.Values.String);
				auto ValueLen = Value.Length();

				for (uint32_t i = 0; i < ValueLen; i++)
					Value[i] ^= KeyData[i % sizeof(KeyData)];

				Writer.WriteNetString(Value);
			}
			break;
			}
		}
	}
}
