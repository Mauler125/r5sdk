#pragma once

#include <cstdint>
#include "StringBase.h"
#include <Windows.h>
#include "Win32Error.h"
#include "ListBase.h"
#include "StringBase.h"
#include "RegistryHive.h"
#include "RegistryView.h"
#include "RegistryValueType.h"

namespace Win32
{
	class RegistryKey
	{
	private:
		RegistryKey(HKEY hKey, bool Writable, RegistryView View);
		RegistryKey(HKEY hKey, bool Writable, bool SystemKey, RegistryView View);

	public:
		~RegistryKey();

		// Closes all resources tied to this key
		void Close();
		// Flush all pending changes to the disk
		void Flush();
		// Creates a new subkey
		RegistryKey CreateSubKey(const string& SubKey);
		// Deletes a subkey
		void DeleteSubKey(const string& SubKey, bool ThrowOnMissingSubKey = false);
		// Deletes a subkey and all nested values
		void DeleteSubKeyTree(const string& SubKey, bool ThrowOnMissingSubKey = false);
		// Deletes a value
		void DeleteValue(const string& Name, bool ThrowOnMissingSubKey = false);
		// Opens a subkey of this instance
		RegistryKey OpenSubKey(const string& Name, bool Writable = true);
		// Returns a value
		template<RegistryValueType T>
		auto GetValue(const string& Name);
		// Returns a list of value names
		List<string> GetValueNames();
		// Returns the type of value this key is
		RegistryValueType GetValueKind(const string& Name);
		// Sets a value
		template<RegistryValueType T, typename Tval>
		void SetValue(const string& Name, const Tval& Value);
		// Returns the count of subkeys
		uint32_t GetSubKeyCount();
		// Returns the count of values
		uint32_t GetValueCount();
		// Returns the subkey names
		List<string> GetSubKeyNames();

		// Opens one of the base registry hives on the system
		static RegistryKey OpenBaseKey(RegistryHive Hive, RegistryView View);
		// Returns a key from the specified handle
		static RegistryKey FromHandle(HKEY hKey, RegistryView View);

	private:
		// Cached key flags
		HKEY _Handle;
		uint32_t _State;
		RegistryView _View;
		string _KeyName;

		// Helper routines
		bool IsDirty();
		bool IsSystemKey();
		bool IsWritable();

		// Internal routine to create a subkeu
		RegistryKey CreateSubKeyInternal(const string& SubKey);
		// Internal routine to delete a subkey tree
		void DeleteSubKeyTreeInternal(const string& SubKey);
		// Internal routine to open a subkey
		RegistryKey InternalOpenSubKey(const string& SubKey, bool Writable);
		// Internal routine to get a value
		std::unique_ptr<uint8_t[]> InternalGetValue(const string& Name, uint64_t& ValueSize);
		// Internal routine to set a value
		void InternalSetValue(const string& Name, uint32_t ValueType, uint8_t* Buffer, uint64_t BufferSize);
		// Internal routine to get subkey count
		uint32_t InternalSubKeyCount();
		// Internal routine to get value count
		uint32_t InternalValueCount();
		// Internal routine to get subkey names
		List<string> InternalGetSubKeyNames();
		
		// Cleans up a key name
		static string FixupName(string Name);
		// Cleans up a key path
		static void FixupPath(string& Path);

		// Internal key states
		constexpr static uint32_t STATE_DIRTY = 0x1;
		constexpr static uint32_t STATE_SYSTEMKEY = 0x2;
		constexpr static uint32_t STATE_WRITEACCESS = 0x4;

		// Maximum key lengths
		constexpr static uint32_t MaxKeyLength = 255;
		constexpr static uint32_t MaxValueLength = 16383;

		// Internal hive names
		constexpr static const char* const HiveNames[] = 
		{
			"HKEY_CLASSES_ROOT",
			"HKEY_CURRENT_USER",
			"HKEY_LOCAL_MACHINE",
			"HKEY_USERS",
			"HKEY_PERFORMANCE_DATA",
			"HKEY_CURRENT_CONFIG",
			"HKEY_DYN_DATA"
		};
	};

	template<RegistryValueType T>
	inline auto RegistryKey::GetValue(const string& Name)
	{
		uint64_t BufferSize = 0;
		auto Buffer = this->InternalGetValue(Name, BufferSize);

		if constexpr (T == RegistryValueType::Dword)
		{
			return *(uint32_t*)Buffer.get();
		}
		else if constexpr (T == RegistryValueType::DwordBigEndian)
		{
			return _byteswap_ulong(*(uint32_t*)Buffer.get());
		}
		else if constexpr (T == RegistryValueType::Qword)
		{
			return *(uint64_t*)Buffer.get();
		}
		else if constexpr (T == RegistryValueType::String)
		{
			return string((const char*)Buffer.get());
		}
		else if constexpr (T == RegistryValueType::MultiString)
		{
			auto Result = List<string>();
			uint64_t Position = 0;

			// We must parse each string as we go, and ensure we haven't reached the buffer size
			while (Position < BufferSize)
			{
				// Read the str with strlen() because it doesn't store the size of each one
				auto cStr = string((const char*)(Buffer.get() + Position));

				// Shift based on the size + null char...
				Position += cStr.Length() + sizeof(char);

				// Move the str into it's list
				Result.EmplaceBack(std::move(cStr));
			}

			return Result;
		}
		else if constexpr (T == RegistryValueType::Binary)
		{
			RegistryBinaryBlob Blob;

			Blob.Buffer = std::move(Buffer);
			Blob.Size = BufferSize;

			return Blob;
		}

		return nullptr;
	}

	template<RegistryValueType T, typename Tval>
	inline void RegistryKey::SetValue(const string& Name, const Tval& Value)
	{
		if constexpr (T == RegistryValueType::Dword)
		{
			uint32_t LittleEndian = (uint32_t)Value;
			this->InternalSetValue(Name, REG_DWORD, (uint8_t*)&LittleEndian, sizeof(uint32_t));
		}
		else if constexpr (T == RegistryValueType::DwordBigEndian)
		{
			uint32_t BigEndian = _byteswap_ulong((uint32_t)Value);
			this->InternalSetValue(Name, REG_DWORD_BIG_ENDIAN, (uint8_t*)&BigEndian, sizeof(uint32_t));
		}
		else if constexpr (T == RegistryValueType::Qword)
		{
			uint64_t LittleEndian = (uint64_t)Value;
			this->InternalSetValue(Name, REG_QWORD, (uint8_t*)&LittleEndian, sizeof(uint64_t));
		}
		else if constexpr (T == RegistryValueType::String)
		{
			this->InternalSetValue(Name, REG_SZ, (uint8_t*)&Value[0], Value.Length() + sizeof(char));
		}
		else if constexpr (T == RegistryValueType::MultiString)
		{
			// Calculate the resulting buffer size
			uint32_t BufferSize = 0;
			for (auto& Str : Value)
				BufferSize += Str.Length() + sizeof(char);
			auto Buffer = std::make_unique<uint8_t[]>(BufferSize);

			BufferSize = 0;

			// Iterate and copy the entire string including the null-char which is in StringBase<>
			for (auto& Str : Value)
			{
				std::memcpy(Buffer.get() + BufferSize, &Str[0], Str.Length() + sizeof(char));
				BufferSize += Str.Length() + sizeof(char);
			}

			// Pass our temporary buffer of all strings to the value
			this->InternalSetValue(Name, REG_MULTI_SZ, (uint8_t*)Buffer.get(), BufferSize);
		}
		else if constexpr (T == RegistryValueType::Binary)
		{
			this->InternalSetValue(Name, REG_BINARY, (uint8_t*)Value.Buffer, Value.Size);
		}
	}
}