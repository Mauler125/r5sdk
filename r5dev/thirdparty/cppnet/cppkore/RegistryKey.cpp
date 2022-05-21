#include "stdafx.h"
#include "RegistryKey.h"

namespace Win32
{
	RegistryKey::RegistryKey(HKEY hKey, bool Writable, RegistryView View)
		: RegistryKey(hKey, Writable, false, View)
	{
	}

	RegistryKey::RegistryKey(HKEY hKey, bool Writable, bool SystemKey, RegistryView View)
		: _State(0), _Handle(hKey), _View(View), _KeyName("")
	{
		if (SystemKey)
			this->_State |= RegistryKey::STATE_SYSTEMKEY;
		if (Writable)
			this->_State |= RegistryKey::STATE_WRITEACCESS;
	}

	RegistryKey::~RegistryKey()
	{
		this->Close();
	}

	void RegistryKey::Close()
	{
		if (this->_Handle != nullptr && !this->IsSystemKey())
			RegCloseKey(this->_Handle);

		this->_Handle = nullptr;
	}

	void RegistryKey::Flush()
	{
		if (this->_Handle != nullptr && this->IsDirty())
			RegFlushKey(this->_Handle);
	}

	RegistryKey RegistryKey::CreateSubKey(const string& SubKey)
	{
		return this->CreateSubKeyInternal(SubKey);
	}

	void RegistryKey::DeleteSubKey(const string& SubKey, bool ThrowOnMissingSubKey)
	{
		auto kPath = FixupName(SubKey); // Remove multiple slashes to a single slash

		try
		{
			auto Key = this->InternalOpenSubKey(kPath, false);
			if (Key.InternalSubKeyCount() > 0)
				throw Win32Error::RegSubKeyChildren();

			Key.Close();
		}
		catch (...)
		{
			if (ThrowOnMissingSubKey)
				throw Win32Error::RegSubKeyMissing();
		}

		auto dResult = RegDeleteKeyExA(this->_Handle, (const char*)kPath, (int)this->_View, NULL);
		if (dResult != 0)
		{
			if (dResult == ERROR_FILE_NOT_FOUND && ThrowOnMissingSubKey)
				throw Win32Error::RegSubKeyMissing();
		}
	}

	void RegistryKey::DeleteSubKeyTree(const string& SubKey, bool ThrowOnMissingSubKey)
	{
		if (SubKey.Length() == 0 && IsSystemKey())
			throw Win32Error::RegSubKeyMalformed();

		auto kPath = FixupName(SubKey);

		try
		{
			auto Key = this->InternalOpenSubKey(kPath, false);
			if (Key.InternalSubKeyCount() > 0)
			{
				auto SubKeyNames = this->InternalGetSubKeyNames();

				for (auto& KeyName : SubKeyNames)
					this->DeleteSubKeyTreeInternal(KeyName);
			}

			Key.Close();
		}
		catch (...)
		{
			if (ThrowOnMissingSubKey)
				throw Win32Error::RegSubKeyMissing();
		}

		RegDeleteKeyExA(this->_Handle, (const char*)kPath, (int)this->_View, NULL);
	}

	void RegistryKey::DeleteValue(const string& Name, bool ThrowOnMissingSubKey)
	{
		auto hResult = RegDeleteValueA(this->_Handle, (const char*)Name);

		if (hResult == ERROR_FILE_NOT_FOUND || hResult == ERROR_FILENAME_EXCED_RANGE)
			if (ThrowOnMissingSubKey)
				throw Win32Error::RegSubKeyMissing();
	}

	RegistryKey RegistryKey::OpenSubKey(const string& Name, bool Writable)
	{
		HKEY hHandle = nullptr;
		auto hAccess = (Writable) ? (KEY_READ | KEY_WRITE) : KEY_READ;
		auto kPath = FixupName(Name);
		auto hResult = RegOpenKeyExA(this->_Handle, (const char*)kPath, NULL, (hAccess | (int)this->_View), &hHandle);

		if (hResult == ERROR_SUCCESS)
		{
			auto Result = RegistryKey(hHandle, Writable, this->_View);
			Result._KeyName = this->_KeyName + "\\" + kPath;

			return Result;
		}

		throw Win32Error::SystemError(GetLastError());
	}

	List<string> RegistryKey::GetValueNames()
	{
		auto nValues = this->InternalValueCount();
		auto Result = List<string>();

		for (uint32_t i = 0; i < nValues; i++)
		{
			char NameBuffer[MaxKeyLength + 1]{};
			uint32_t NameLength = (MaxKeyLength + 1);

			auto hResult = RegEnumValueA(this->_Handle, i, NameBuffer, (LPDWORD)&NameLength, NULL, NULL, NULL, NULL);
			if (hResult == ERROR_SUCCESS)
				Result.EmplaceBack(NameBuffer);
		}

		return Result;
	}

	RegistryValueType RegistryKey::GetValueKind(const string& Name)
	{
		DWORD Type = 0, DataSize = 0;
		auto qResult = RegQueryValueExA(this->_Handle, (const char*)Name, NULL, &Type, NULL, &DataSize);

		switch (Type)
		{
		case REG_DWORD:
			return RegistryValueType::Dword;
		case REG_DWORD_BIG_ENDIAN:
			return RegistryValueType::DwordBigEndian;
		case REG_SZ:
			return RegistryValueType::String;
		case REG_MULTI_SZ:
			return RegistryValueType::MultiString;
		case REG_BINARY:
			return RegistryValueType::Binary;
		case REG_LINK:
			return RegistryValueType::SymbolicLink;
		case REG_RESOURCE_LIST:
			return RegistryValueType::ResourceList;
		case REG_FULL_RESOURCE_DESCRIPTOR:
			return RegistryValueType::FullResourceDescriptor;
		case REG_QWORD:
			return RegistryValueType::Qword;
		case REG_RESOURCE_REQUIREMENTS_LIST:
			return RegistryValueType::ResourceRequirementsList;
		}

		return RegistryValueType::None;
	}

	uint32_t RegistryKey::GetSubKeyCount()
	{
		return this->InternalSubKeyCount();
	}

	uint32_t RegistryKey::GetValueCount()
	{
		return this->InternalValueCount();
	}

	List<string> RegistryKey::GetSubKeyNames()
	{
		return this->InternalGetSubKeyNames();
	}

	RegistryKey RegistryKey::OpenBaseKey(RegistryHive Hive, RegistryView View)
	{
		auto Index = ((int)Hive) & 0x0FFFFFFF;
		auto Key = RegistryKey((HKEY)Hive, true, true, View);

		Key._KeyName = RegistryKey::HiveNames[Index];

		return Key;
	}

	RegistryKey RegistryKey::FromHandle(HKEY hKey, RegistryView View)
	{
		return RegistryKey(hKey, true, View);
	}

	bool RegistryKey::IsDirty()
	{
		return (this->_State & RegistryKey::STATE_DIRTY) != 0;
	}

	bool RegistryKey::IsSystemKey()
	{
		return (this->_State & RegistryKey::STATE_SYSTEMKEY) != 0;
	}

	bool RegistryKey::IsWritable()
	{
		return (this->_State & RegistryKey::STATE_WRITEACCESS) != 0;
	}

	RegistryKey RegistryKey::CreateSubKeyInternal(const string& SubKey)
	{
		auto kPath = FixupName(SubKey);	// Remove multiple slashes to a single slash

		try
		{
			// Test if the key already exists
			return RegistryKey::InternalOpenSubKey(kPath, this->IsWritable());
		}
		catch (...)
		{
			// Doesn't exist, can be created
			HKEY hHandle = nullptr;
			DWORD Disposition = 0;
			auto hResult = RegCreateKeyExA(this->_Handle, (const char*)kPath, NULL, NULL, REG_OPTION_NON_VOLATILE, ((KEY_READ | KEY_WRITE) | (int)this->_View), NULL, &hHandle, &Disposition);

			if (hResult == ERROR_SUCCESS)
			{
				auto nKey = RegistryKey(hHandle, true, this->_View);

				if (kPath.Length() == 0)
					nKey._KeyName = _KeyName;
				else
					nKey._KeyName = _KeyName + "\\" + kPath;

				return nKey;
			}
		}

		throw Win32Error::SystemError(GetLastError());
	}

	void RegistryKey::DeleteSubKeyTreeInternal(const string& SubKey)
	{
		try
		{
			auto Key = this->InternalOpenSubKey(SubKey, true);
			if (Key.InternalSubKeyCount() > 0)
			{
				auto SubKeyNames = Key.InternalGetSubKeyNames();

				for (auto& KeyName : SubKeyNames)
					Key.DeleteSubKeyTreeInternal(KeyName);
			}

			Key.Close();
		}
		catch (...)
		{
			// Nothing
		}

		RegDeleteKeyExA(this->_Handle, (const char*)SubKey, (int)this->_View, NULL);
	}

	RegistryKey RegistryKey::InternalOpenSubKey(const string& Name, bool Writable)
	{
		HKEY hHandle = nullptr;
		auto hAccess = (Writable) ? (KEY_READ | KEY_WRITE) : KEY_READ;
		auto hResult = RegOpenKeyExA(this->_Handle, (const char*)Name, NULL, (hAccess | (int)this->_View), &hHandle);

		if (hResult == ERROR_SUCCESS)
		{
			auto Result = RegistryKey(hHandle, Writable, this->_View);
			Result._KeyName = this->_KeyName + "\\" + Name;

			return Result;
		}

		throw Win32Error::SystemError(GetLastError());
	}

	std::unique_ptr<uint8_t[]> RegistryKey::InternalGetValue(const string& Name, uint64_t& ValueSize)
	{
		DWORD Type = 0, DataSize = 0;
		auto qResult = RegQueryValueExA(this->_Handle, (const char*)Name, NULL, &Type, NULL, &DataSize);

		if (qResult != ERROR_SUCCESS)
			throw Win32Error::SystemError(GetLastError());

		auto ResultBuffer = std::make_unique<uint8_t[]>(DataSize);
		RegQueryValueExA(this->_Handle, (const char*)Name, NULL, &Type, ResultBuffer.get(), &DataSize);

		ValueSize = DataSize;

		return ResultBuffer;
	}

	void RegistryKey::InternalSetValue(const string& Name, uint32_t ValueType, uint8_t* Buffer, uint64_t BufferSize)
	{
		auto hResult = RegSetValueExA(this->_Handle, (const char*)Name, NULL, ValueType, Buffer, (DWORD)BufferSize);
		if (hResult != ERROR_SUCCESS)
			throw Win32Error::SystemError(GetLastError());
	}

	uint32_t RegistryKey::InternalSubKeyCount()
	{
		DWORD SubKeys = 0, Junk = 0;
		RegQueryInfoKeyA(this->_Handle, NULL, NULL, NULL, &SubKeys, NULL, NULL, &Junk, NULL, NULL, NULL, NULL);

		return (uint32_t)SubKeys;
	}

	uint32_t RegistryKey::InternalValueCount()
	{
		DWORD Values = 0, Junk = 0;
		RegQueryInfoKeyA(this->_Handle, NULL, NULL, NULL, &Junk, NULL, NULL, &Values, NULL, NULL, NULL, NULL);

		return (uint32_t)Values;
	}

	List<string> RegistryKey::InternalGetSubKeyNames()
	{
		auto Result = List<string>();
		auto sCount = this->InternalSubKeyCount();

		char Buffer[RegistryKey::MaxKeyLength + 1]{};
		DWORD NameLength = RegistryKey::MaxKeyLength + 1;

		for (uint32_t i = 0; i < sCount; i++)
		{
			NameLength = RegistryKey::MaxKeyLength + 1;
			auto hQuery = RegEnumKeyExA(this->_Handle, i, Buffer, &NameLength, NULL, NULL, NULL, NULL);

			if (hQuery == 0)
				Result.EmplaceBack(Buffer);
		}

		return Result;
	}

	string RegistryKey::FixupName(string Name)
	{
		RegistryKey::FixupPath(Name);

		auto eChar = Name.Length() - 1;
		if (eChar >= 0 && Name[eChar] == '\\')
			return Name.Substring(0, eChar);

		return Name;
	}

	void RegistryKey::FixupPath(string& Path)
	{
		// Replace double slash with single slashes
		Path = Path.Replace("\\\\", "\\");
	}
}