#pragma once

#include <cstdint>
#include "ListBase.h"
#include "StringBase.h"
#include "ImmutableStringBase.h"
#include "File.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"

namespace System
{
	// Represents the type of setting we are using
	enum class SettingType : char
	{
		Boolean = 'B',
		Integer = 'I',
		Integer64 = 'L',
		String = 'S',
		ProtectedString = 'P'
	};

	// Used to store a setting.
	class SettingsObject
	{
	public:
		SettingsObject();
		SettingsObject(string Key, SettingType Type, uint8_t* Value);
		~SettingsObject();

		string Key;
		SettingType Type;
		
		union ValueUn
		{
			char* String;
			bool Boolean;
			uint32_t Integer;
			uint64_t Integer64;
		} Values;

		SettingsObject& operator=(SettingsObject Rhs)
		{
			Key = Rhs.Key;
			Type = Rhs.Type;

			if (Type == SettingType::String || Type == SettingType::ProtectedString)
			{
				if (this->Values.String)
					delete[] this->Values.String;

				auto Len = strlen(Rhs.Values.String);
				this->Values.String = new char[Len + 1]{};
				std::memcpy(this->Values.String, Rhs.Values.String, Len);
			}
			else
			{
				this->Values.Integer64 = Rhs.Values.Integer64;
			}

			return *this;
		}

		bool operator==(SettingsObject Rhs)
		{
			// yes yes yes i know this is bad but it doesn't cause issues for me so who cares
			return Key == Rhs.Key && Type == Rhs.Type;
		}
	};

	// Reads and writes configuration files.
	class Settings
	{
	public:
		Settings() = default;
		~Settings() = default;


		_declspec(noinline) void SetBool(const imstring& Key, bool Value)
		{
			for (auto& Setting : _Settings)
			{
				if (Setting.Key == (const char*)Key)
				{
					Setting.Values.Boolean = Value;
				}
			}
			_Settings.EmplaceBack(Key, SettingType::Boolean, (uint8_t*)&Value);
		}

		_declspec(noinline) void SetInt(const imstring& Key, uint32_t Value)
		{
			for (auto& Setting : _Settings)
			{
				if (Setting.Key == (const char*)Key)
				{
					Setting.Values.Integer = Value;
				}
			}
			_Settings.EmplaceBack(Key, SettingType::Integer, (uint8_t*)&Value);
		}

		template<SettingType T, class V>
		_declspec(noinline) void Set(const imstring& Key, V && Value)
		{
			for (auto& Setting : _Settings)
			{
				if (Setting.Key == (const char*)Key)
				{
					if constexpr (T == SettingType::Boolean)
						Setting.Values.Boolean = Value;
					else if constexpr (T == SettingType::Integer)
						Setting.Values.Integer = Value;
					else if constexpr (T == SettingType::Integer64)
						Setting.Values.Integer64 = Value;
					else if constexpr (T == SettingType::ProtectedString || T == SettingType::String)
					{
						if (Setting.Values.String != NULL)
							delete[] Setting.Values.String;

						if constexpr (std::is_same<V, string>::value)
						{
							Setting.Values.String = new char[Value.Length() + 1]{};
							std::memcpy(Setting.Values.String, (const char*)Value, Value.Length());
						}
						else
						{
							auto Len = strlen(Value);
							Setting.Values.String = new char[Len + 1]{};
							std::memcpy(Setting.Values.String, Value, Len);
						}
					}

					return;
				}
			}
			if constexpr (T == SettingType::String)
				_Settings.EmplaceBack(Key, T, (uint8_t*)Value.ToCString());
			else
				_Settings.EmplaceBack(Key, T, (uint8_t*)&Value);

		}

		template<const SettingType T>
		_declspec(noinline) bool Has(const imstring& Key)
		{
			SettingsObject* Value = nullptr;

			for (auto& Setting : _Settings)
			{
				if (Setting.Key == (const char*)Key)
				{
					Value = &Setting;
					break;
				}
			}

			return (Value != nullptr);
		}

		// for some reason the template one doesn't even check the type so what is the point in having it as a template?
		bool Has(const imstring& Key)
		{
			SettingsObject* Value = nullptr;

			for (auto& Setting : _Settings)
			{
				if (Setting.Key == (const char*)Key)
				{
					Value = &Setting;
					break;
				}
			}

			return (Value != nullptr);
		}

		template<const SettingType T>
		_declspec(noinline) void Remove(const imstring& Key)
		{
			for (SettingsObject& Setting : _Settings)
			{
				if (Setting.Key == (const char*)Key)
				{
					Setting = SettingsObject();
					break;
				}
			}

			return;
		};

		_declspec(noinline) bool GetBool(const imstring& Key)
		{
			SettingsObject* Value = nullptr;

			for (auto& Setting : _Settings)
			{
				if (Setting.Key == (const char*)Key)
				{
					Value = &Setting;
					break;
				}
			}
			if (Value)
				return Value->Values.Boolean;

			return false;
		}

		template<const SettingType T>
		_declspec(noinline) auto Get(const imstring& Key)
		{
			SettingsObject* Value = nullptr;

			for (auto& Setting : _Settings)
			{
				if (Setting.Key == (const char*)Key)
				{
					Value = &Setting;
					break;
				}
			}

			if constexpr (T == SettingType::Boolean)
			{
				if (Value)
					return (bool)Value->Values.Boolean;

				return (bool)false;
			}
			else if constexpr (T == SettingType::Integer)
			{
				if (Value)
					return (uint32_t)Value->Values.Integer;

				return (uint32_t)0;
			}
			else if constexpr (T == SettingType::Integer64)
			{
				if (Value)
					return (uint64_t)Value->Values.Integer64;

				return (uint64_t)0;
			}
			else if constexpr (T == SettingType::String || T == SettingType::ProtectedString)
			{
				if (Value)
					return (string)Value->Values.String;

				return (string)"";
			}
		}

		// Load the config file from the specified path.
		void Load(const string& Path);
		// Save the config file to the specified path.
		void Save(const string& Path);

	private:
		// Internal cached settings
		List<SettingsObject> _Settings;
	};
}