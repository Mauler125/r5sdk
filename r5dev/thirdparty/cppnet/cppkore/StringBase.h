#pragma once

#include <algorithm>
#include <string_view>
#include <stdarg.h>
#include <Windows.h>
#include "stdext.h"
#include "ListBase.h"
#include "ImmutableStringBase.h"

template<class Tchar>
class StringBase
{
public:
	constexpr StringBase();
	constexpr StringBase(const uint32_t InitialSize, const Tchar InitialValue = (Tchar)'\0\0', const bool Initialized = true);
	constexpr StringBase(const Tchar* Value);
	constexpr StringBase(const Tchar* Value, size_t ValueSize);
	constexpr StringBase(std::basic_string_view<Tchar> Value);
	constexpr StringBase(ImmutableStringBase<Tchar> Value);

	template<size_t Size>
	constexpr StringBase(const Tchar(&Value)[Size]);

	constexpr StringBase(const StringBase& Value);
	constexpr StringBase(StringBase&& Value) noexcept;

	~StringBase();

	// Returns a null-terminated string
	constexpr const Tchar* ToCString() const;
	// Returns a copy of the current instance
	constexpr StringBase<char> ToString() const;
	// Returns a wide copy of the current instance
	constexpr StringBase<wchar_t> ToWString() const;
	// Returns a string_view of the current instance
	constexpr std::basic_string_view<Tchar> ToStringView() const;
	// Returns the length of the string in characters
	constexpr uint32_t Length() const;

	// Checks the string for equality, returns 0 when a match is found
	constexpr int32_t Compare(const Tchar* Rhs) const noexcept;
	// Checks the string for equality, returns 0 when a match is found
	constexpr int32_t Compare(const StringBase<Tchar>& Rhs) const noexcept;
	// Checks the string for equality, returns 0 when a match is found
	constexpr int32_t Compare(std::basic_string_view<Tchar> Rhs) const noexcept;

	// Creates an array of strings by splitting this string at each occurence of the separator
	constexpr List<StringBase<Tchar>> Split(const Tchar Separator) const;
	// Creates an array of strings by splitting this string at each occurence of the separator
	constexpr List<StringBase<Tchar>> Split(const Tchar* Separator) const;
	// Creates an array of strings by splitting this string at each occurence of the separator
	constexpr List<StringBase<Tchar>> Split(const StringBase<Tchar>& Separator) const;
	// Creates an array of strings by splitting this string at each occurence of the separator
	constexpr List<StringBase<Tchar>> Split(const std::basic_string_view<Tchar> Separator) const;

	// Trims the whitespace from both ends of the string
	constexpr StringBase<Tchar> Trim() const;
	// Trims the whitespace from the start of the string
	constexpr StringBase<Tchar> TrimStart() const;
	// Trims the whitespace from the end of the string
	constexpr StringBase<Tchar> TrimEnd() const;

	// Returns the index of the first occurance of value in the current instance
	constexpr uint32_t IndexOf(const Tchar Rhs, uint32_t Pos = 0) const noexcept;
	// Returns the index of the first occurance of value in the current instance
	constexpr uint32_t IndexOf(const Tchar* Rhs, uint32_t Pos = 0) const;
	// Returns the index of the first occurance of value in the current instance
	constexpr uint32_t IndexOf(const Tchar* Rhs, uint32_t Pos, uint32_t Count) const;
	// Returns the index of the first occurance of value in the current instance
	constexpr uint32_t IndexOf(const StringBase<Tchar>& Rhs, uint32_t Pos = 0) const noexcept;
	// Returns the index of the first occurance of value in the current instance
	constexpr uint32_t IndexOf(const StringBase<Tchar>& Rhs, uint32_t Pos, uint32_t Count) const noexcept;
	// Returns the index of the first occurance of value in the current instance
	constexpr uint32_t IndexOf(std::basic_string_view<Tchar> Rhs, uint32_t Pos = 0) const noexcept;
	// Returns the index of the first occurance of value in the current instance
	constexpr uint32_t IndexOf(std::basic_string_view<Tchar> Rhs, uint32_t Pos, uint32_t Count) const noexcept;

	// Returns the index of the last occurance of value in the current instance
	constexpr uint32_t LastIndexOf(const Tchar Rhs, uint32_t Pos = 0) const noexcept;
	// Returns the index of the last occurance of value in the current instance
	constexpr uint32_t LastIndexOf(const Tchar* Rhs, uint32_t Pos = 0) const;
	// Returns the index of the last occurance of value in the current instance
	constexpr uint32_t LastIndexOf(const Tchar* Rhs, uint32_t Pos, uint32_t Count) const;
	// Returns the index of the last occurance of value in the current instance
	constexpr uint32_t LastIndexOf(const StringBase<Tchar>& Rhs, uint32_t Pos = 0) const noexcept;
	// Returns the index of the last occurance of value in the current instance
	constexpr uint32_t LastIndexOf(const StringBase<Tchar>& Rhs, uint32_t Pos, uint32_t Count) const noexcept;
	// Returns the index of the last occurance of value in the current instance
	constexpr uint32_t LastIndexOf(std::basic_string_view<Tchar> Rhs, uint32_t Pos = 0) const noexcept;
	// Returns the index of the last occurance of value in the current instance
	constexpr uint32_t LastIndexOf(std::basic_string_view<Tchar> Rhs, uint32_t Pos, uint32_t Count) const noexcept;

	// Creates a copy of this string in lower case
	constexpr StringBase<Tchar> ToLower() const;
	// Creates a copy of this string in upper case
	constexpr StringBase<Tchar> ToUpper() const;

	// Replaces all instances of the Old value with New
	constexpr StringBase<Tchar> Replace(const Tchar Old, const Tchar New) const;
	// Replaces all instances of the Old value with New
	constexpr StringBase<Tchar> Replace(const Tchar* Old, const Tchar* New) const;
	// Replaces all instances of the Old value with New
	constexpr StringBase<Tchar> Replace(const StringBase<Tchar>& Old, const StringBase<Tchar>& New) const;
	// Replaces all instances of the Old value with New
	constexpr StringBase<Tchar> Replace(std::basic_string_view<Tchar> Old, std::basic_string_view<Tchar> New) const;

	// Whether or not the string starts with the value
	constexpr bool StartsWith(const Tchar* Rhs) const;
	// Whether or not the string starts with the value
	constexpr bool StartsWith(const StringBase<Tchar>&  Rhs) const;
	// Whether or not the string starts with the value
	constexpr bool StartsWith(std::basic_string_view<Tchar> Rhs) const;

	// Whether or not the string ends with the value
	constexpr bool EndsWith(const Tchar* Rhs) const;
	// Whether or not the string ends with the value
	constexpr bool EndsWith(const StringBase<Tchar>& Rhs) const;
	// Whether or not the string ends with the value
	constexpr bool EndsWith(std::basic_string_view<Tchar> Rhs) const;

	// Whether or not the string contains the value
	constexpr bool Contains(const Tchar* Rhs) const;
	// Whether or not the string contains the value
	constexpr bool Contains(const StringBase<Tchar>& Rhs) const;
	// Whether or not the string contains the value
	constexpr bool Contains(std::basic_string_view<Tchar> Rhs) const;

	// Returns a substring of this string
	constexpr StringBase<Tchar> Substring(uint32_t StartIndex) const;
	// Returns a substring of this string
	constexpr StringBase<Tchar> Substring(uint32_t StartIndex, uint32_t Length) const;

	// Appends a character to this string
	constexpr void Append(Tchar Rhs);
	// Appends a null-terminated string to this string
	constexpr void Append(const Tchar* Rhs);
	// Appends a predetermined size string to this string
	constexpr void Append(Tchar* Rhs, uint32_t Count);
	// Appends a string to this string
	constexpr void Append(const StringBase<Tchar>& Rhs);

	// Standard logical casts
	constexpr operator Tchar*(void) const;
	constexpr operator const Tchar*(void) const;

	// Cast to string_view
	constexpr operator std::basic_string_view<Tchar>(void) const;	// NOTE: Breaks intellisense but IS defined and DOES function...

	// Iterator definitions, for for(& :) loop
	constexpr Tchar* begin() const noexcept;
	constexpr Tchar* end() const noexcept;

	// Standard logical operators
	constexpr StringBase<Tchar>& operator+=(const Tchar Rhs);
	constexpr StringBase<Tchar>& operator+=(const Tchar* Rhs);
	constexpr StringBase<Tchar>& operator+=(const StringBase<Tchar>& Rhs);
	constexpr StringBase<Tchar>& operator+=(std::basic_string_view<Tchar> Rhs);
	constexpr StringBase<Tchar>& operator+=(ImmutableStringBase<Tchar> Rhs);

	// Assignment operators
	constexpr StringBase<Tchar>& operator=(const Tchar* Rhs);
	constexpr StringBase<Tchar>& operator=(const StringBase<Tchar>& Rhs);
	constexpr StringBase<Tchar>& operator=(std::basic_string_view<Tchar> Rhs);
	constexpr StringBase<Tchar>& operator=(ImmutableStringBase<Tchar> Rhs);

	// Equality operators
	constexpr bool operator==(const Tchar* Rhs) const;
	constexpr bool operator==(const StringBase<Tchar>& Rhs) const;
	constexpr bool operator==(std::basic_string_view<Tchar> Rhs) const;

	// Inequality operators
	constexpr bool operator!=(const Tchar* Rhs) const;
	constexpr bool operator!=(const StringBase<Tchar>& Rhs) const;
	constexpr bool operator!=(std::basic_string_view<Tchar> Rhs) const;

	// Complex logical operators
	friend StringBase<Tchar> operator+(StringBase<Tchar> Lhs, const Tchar Rhs)
	{
		Lhs += Rhs;
		return Lhs;
	}
	friend StringBase<Tchar> operator+(StringBase<Tchar> Lhs, const Tchar* Rhs)
	{
		Lhs += Rhs;
		return Lhs;
	}
	friend StringBase<Tchar> operator+(StringBase<Tchar> Lhs, const StringBase<Tchar>& Rhs)
	{
		Lhs += Rhs;
		return Lhs;
	}
	friend StringBase<Tchar> operator+(StringBase<Tchar> Lhs, std::basic_string_view<Tchar>& Rhs)
	{
		Lhs += Rhs;
		return Lhs;
	}
	friend StringBase<Tchar> operator+(StringBase<Tchar> Lhs, ImmutableStringBase<Tchar>& Rhs)
	{
		Lhs += Rhs;
		return Lhs;
	}

	// Array index operator
	constexpr Tchar& operator[](size_t Index);
	constexpr Tchar& operator[](size_t Index) const;

	// Indicates no matches in the string 
	static constexpr auto InvalidPosition{ static_cast<uint32_t>(-1) };

	//
	// Static helper routines
	//

	// Whether or not the string is initialized and not blank
	static constexpr bool IsNullOrEmpty(const StringBase<Tchar>& Rhs);
	// Whether or not the string is initialized and not whitespace
	static constexpr bool IsNullOrWhiteSpace(const StringBase<Tchar>& Rhs);

	// Formats a string based on the provided input
	static constexpr StringBase<Tchar> Format(const Tchar* Format, ...);

	static constexpr StringBase<Tchar> Format(const Tchar* Format, va_list vArgs);


private:
	Tchar* _Buffer;
	uint32_t _BufferSize;
	uint32_t _StoreSize;

	constexpr void EnsureCapacity(uint32_t Capacity);
};

template<class Tchar>
inline constexpr StringBase<Tchar>::StringBase()
	: _Buffer(nullptr), _BufferSize(0), _StoreSize(0)
{
}

template<class Tchar>
inline constexpr StringBase<Tchar>::StringBase(const uint32_t InitialSize, const Tchar InitialValue, const bool Initialized)
	: _Buffer(nullptr), _BufferSize(0), _StoreSize(0)
{
	this->EnsureCapacity(InitialSize);

	if (!Initialized)
		_StoreSize = 0;

	if constexpr (sizeof(Tchar) == sizeof(char))
		std::memset(this->_Buffer, InitialValue, InitialSize);
	else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		std::wmemset(this->_Buffer, InitialValue, InitialSize);
}

template<class Tchar>
inline constexpr StringBase<Tchar>::StringBase(const Tchar* Value)
	: _Buffer(nullptr), _BufferSize(0), _StoreSize(0)
{
	size_t vLen = 0;
	if constexpr (sizeof(Tchar) == sizeof(char))
		vLen = strlen(Value);
	else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		vLen = wcslen(Value);

	this->EnsureCapacity((uint32_t)vLen);

	std::memcpy(this->_Buffer, Value, vLen * sizeof(Tchar));
}

template<class Tchar>
inline constexpr StringBase<Tchar>::StringBase(const Tchar* Value, size_t ValueSize)
	: _Buffer(nullptr), _BufferSize(0), _StoreSize(0)
{
	this->EnsureCapacity((uint32_t)ValueSize);

	std::memcpy(this->_Buffer, Value, ValueSize * sizeof(Tchar));
}

template<class Tchar>
inline constexpr StringBase<Tchar>::StringBase(std::basic_string_view<Tchar> Value)
	: _Buffer(nullptr), _BufferSize(0), _StoreSize(0)
{
	auto vLen = Value.size();
	this->EnsureCapacity((uint32_t)vLen);

	std::memcpy(this->_Buffer, Value.data(), vLen * sizeof(Tchar));
}

template<class Tchar>
inline constexpr StringBase<Tchar>::StringBase(ImmutableStringBase<Tchar> Value)
	: _Buffer(nullptr), _BufferSize(0), _StoreSize(0)
{
	auto vLen = Value.Length();
	this->EnsureCapacity((uint32_t)vLen);

	std::memcpy(this->_Buffer, (Tchar*)Value, vLen * sizeof(Tchar));
}

template<class Tchar>
template<size_t Size>
inline constexpr StringBase<Tchar>::StringBase(const Tchar(&Value)[Size])
	: _Buffer(nullptr), _BufferSize(0), _StoreSize(0)
{
	size_t vLen = 0;
	if constexpr (sizeof(Tchar) == sizeof(char))
		vLen = strlen(Value);
	else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		vLen = wcslen(Value);

	this->EnsureCapacity((uint32_t)vLen);

	std::memcpy(this->_Buffer, Value, vLen * sizeof(Tchar));
}

template<class Tchar>
inline constexpr StringBase<Tchar>::StringBase(const StringBase& Value)
	: StringBase(Value._Buffer, Value._StoreSize)
{
}

template<class Tchar>
inline constexpr StringBase<Tchar>::StringBase(StringBase&& Value) noexcept
{
	this->_Buffer = Value._Buffer;
	this->_BufferSize = Value._BufferSize;
	this->_StoreSize = Value._StoreSize;

	Value._Buffer = nullptr;
	Value._BufferSize = 0;
	Value._StoreSize = 0;
}

template<class Tchar>
inline StringBase<Tchar>::~StringBase()
{
	if (this->_Buffer)
		delete[] this->_Buffer;

	this->_Buffer = nullptr;
	this->_BufferSize = 0;
	this->_StoreSize = 0;
}

template<class Tchar>
inline constexpr const Tchar * StringBase<Tchar>::ToCString() const
{
	return this->_Buffer;
}

template<class Tchar>
inline constexpr StringBase<char> StringBase<Tchar>::ToString() const
{
	if constexpr (sizeof(Tchar) == sizeof(char))
		return StringBase<char>(this->_Buffer, this->_StoreSize);
	else
	{
		auto cbBuffer = WideCharToMultiByte(CP_UTF8, NULL, this->_Buffer, this->_StoreSize, NULL, NULL, NULL, FALSE);
		if (cbBuffer == 0)
			return "";

		auto Result = StringBase<char>(cbBuffer);

		WideCharToMultiByte(CP_UTF8, NULL, this->_Buffer, this->_StoreSize, (char*)Result, cbBuffer, NULL, FALSE);

		return std::move(Result);
	}
}

template<class Tchar>
inline constexpr StringBase<wchar_t> StringBase<Tchar>::ToWString() const
{
	if (sizeof(Tchar) == sizeof(wchar_t))
		return StringBase<wchar_t>((const wchar_t*)this->_Buffer, this->_StoreSize);
	else
	{
		auto cbBuffer = MultiByteToWideChar(CP_UTF8, NULL, this->_Buffer, this->_StoreSize, NULL, NULL);
		if (cbBuffer == 0)
			return L"";

		auto Result = StringBase<wchar_t>(cbBuffer);

		MultiByteToWideChar(CP_UTF8, NULL, this->_Buffer, this->_StoreSize, (wchar_t*)Result, cbBuffer);

		return std::move(Result);
	}
}

template<class Tchar>
inline constexpr std::basic_string_view<Tchar> StringBase<Tchar>::ToStringView() const
{
	return std::basic_string_view<Tchar>(this->_Buffer, this->_StoreSize);
}

template<class Tchar>
inline constexpr uint32_t StringBase<Tchar>::Length() const
{
	return this->_StoreSize;
}

template<class Tchar>
inline constexpr int32_t StringBase<Tchar>::Compare(const Tchar* Rhs) const noexcept
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);

	size_t RhsSize = 0;
	if constexpr (sizeof(Tchar) == sizeof(char))
		RhsSize = strlen(Rhs) * sizeof(Tchar);
	else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		RhsSize = wcslen(Rhs) * sizeof(Tchar);

	const int Res = std::memcmp(this->_Buffer, Rhs, (size_t)std::min<uint32_t>((uint32_t)LhsSize, (uint32_t)RhsSize));

	if (Res != 0)
		return Res;
	if (LhsSize < RhsSize)
		return (-1);
	if (LhsSize > RhsSize)
		return (1);

	return 0;
}

template<class Tchar>
inline constexpr int32_t StringBase<Tchar>::Compare(const StringBase<Tchar>& Rhs) const noexcept
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);
	auto RhsSize = Rhs._StoreSize * sizeof(Tchar);

	const int Res = std::memcmp(this->_Buffer, Rhs._Buffer, (size_t)std::min<uint32_t>((uint32_t)LhsSize, (uint32_t)RhsSize));

	if (Res != 0)
		return Res;
	if (LhsSize < RhsSize)
		return (-1);
	if (LhsSize > RhsSize)
		return (1);

	return 0;
}

template<class Tchar>
inline constexpr int32_t StringBase<Tchar>::Compare(std::basic_string_view<Tchar> Rhs) const noexcept
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);
	auto RhsSize = Rhs.size() * sizeof(Tchar);

	const int Res = std::memcmp(this->_Buffer, Rhs.data(), (size_t)std::min<uint32_t>((uint32_t)LhsSize, (uint32_t)RhsSize));

	if (Res != 0)
		return Res;
	if (LhsSize < RhsSize)
		return (-1);
	if (LhsSize > RhsSize)
		return (1);

	return 0;
}

template<class Tchar>
inline constexpr List<StringBase<Tchar>> StringBase<Tchar>::Split(const Tchar Delimiter) const
{
	// TODO: This sometimes skips last one: "Imakewins = {".split(' ') missing {
	auto Result = List<StringBase<Tchar>>();

	uint32_t CurrentIndex = 0, LocatedPosition = 0;

	// Optimized for large strings and lots of occurences
	while ((LocatedPosition = this->IndexOf(Delimiter, CurrentIndex)) != StringBase<Tchar>::InvalidPosition)
	{
		Result.Emplace(this->Substring(CurrentIndex, LocatedPosition - CurrentIndex));

		// Advance past the size of old and the position
		CurrentIndex = LocatedPosition + 1;
	}

	if (CurrentIndex != this->_StoreSize)
		Result.Emplace(this->Substring(CurrentIndex, this->_StoreSize - CurrentIndex));

	return Result;
}

template<class Tchar>
inline constexpr List<StringBase<Tchar>> StringBase<Tchar>::Split(const Tchar* Separator) const
{
	auto Result = List<StringBase<Tchar>>();

	uint32_t CurrentIndex = 0, LocatedPosition = 0;

	uint32_t LhsSize = 0;
	if constexpr (sizeof(Tchar) == sizeof(char))
		LhsSize = (uint32_t)strlen(Separator);
	else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		LhsSize = (uint32_t)wcslen(Separator);

	// Optimized for large strings and lots of occurences
	while ((LocatedPosition = this->IndexOf(Separator, CurrentIndex)) != StringBase<Tchar>::InvalidPosition)
	{
		Result.Emplace(this->Substring(CurrentIndex, LocatedPosition - CurrentIndex));

		// Advance past the size of old and the position
		CurrentIndex = LocatedPosition + LhsSize;
	}

	if (CurrentIndex != this->_StoreSize)
		Result.Emplace(this->Substring(CurrentIndex, this->_StoreSize - CurrentIndex));

	return Result;
}

template<class Tchar>
inline constexpr List<StringBase<Tchar>> StringBase<Tchar>::Split(const StringBase<Tchar>& Separator) const
{
	auto Result = List<StringBase<Tchar>>();

	uint32_t CurrentIndex = 0, LocatedPosition = 0;

	auto LhsSize = Separator._StoreSize;

	// Optimized for large strings and lots of occurences
	while ((LocatedPosition = this->IndexOf(Separator, CurrentIndex)) != StringBase<Tchar>::InvalidPosition)
	{
		Result.Emplace(this->Substring(CurrentIndex, LocatedPosition - CurrentIndex));

		// Advance past the size of old and the position
		CurrentIndex = LocatedPosition + LhsSize;
	}

	if (CurrentIndex != this->_StoreSize)
		Result.Emplace(this->Substring(CurrentIndex, this->_StoreSize - CurrentIndex));

	return Result;
}

template<class Tchar>
inline constexpr List<StringBase<Tchar>> StringBase<Tchar>::Split(const std::basic_string_view<Tchar> Separator) const
{
	auto Result = List<StringBase<Tchar>>();

	uint32_t CurrentIndex = 0, LocatedPosition = 0;

	auto LhsSize = Separator.size();

	// Optimized for large strings and lots of occurences
	while ((LocatedPosition = this->IndexOf(Separator, CurrentIndex)) != StringBase<Tchar>::InvalidPosition)
	{
		Result.Emplace(this->Substring(CurrentIndex, LocatedPosition - CurrentIndex));

		// Advance past the size of old and the position
		CurrentIndex = LocatedPosition + LhsSize;
	}

	if (CurrentIndex != this->_StoreSize)
		Result.Emplace(this->Substring(CurrentIndex, this->_StoreSize - CurrentIndex));

	return Result;
}

template<class Tchar>
inline constexpr StringBase<Tchar> StringBase<Tchar>::Trim() const
{
	int64_t Index = 0, Ending = (int64_t)this->_StoreSize - 1;

	for (Index = 0; Index < this->_StoreSize; Index++)
	{
		if constexpr (sizeof(Tchar) == sizeof(char))
		{
			if (!::isspace(this->_Buffer[Index]))
				break;
		}
		else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		{
			if (!::iswspace(this->_Buffer[Index]))
				break;
		}
	}

	for (Ending = (int64_t)this->_StoreSize - 1; Ending >= Index; Ending--)
	{
		if constexpr (sizeof(Tchar) == sizeof(char))
		{
			if (!::isspace(this->_Buffer[Ending]))
				break;
		}
		else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		{
			if (!::iswspace(this->_Buffer[Ending]))
				break;
		}
	}

	auto Length = Ending - Index + 1;
	if (Length == this->_StoreSize)
		return *this;
	else if (Length == 0)
		return "";

	return StringBase<Tchar>(this->_Buffer + Index, (uint32_t)Length);
}

template<class Tchar>
inline constexpr StringBase<Tchar> StringBase<Tchar>::TrimStart() const
{
	int64_t Index = 0, Ending = (int64_t)this->_StoreSize - 1;

	for (Index = 0; Index < this->_StoreSize; Index++)
	{
		if constexpr (sizeof(Tchar) == sizeof(char))
		{
			if (!::isspace(this->_Buffer[Index]))
				break;
		}
		else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		{
			if (!::iswspace(this->_Buffer[Index]))
				break;
		}
	}

	auto Length = Ending - Index + 1;
	if (Length == this->_StoreSize)
		return *this;
	else if (Length == 0)
		return "";

	return StringBase<Tchar>(this->_Buffer + Index, (uint32_t)Length);
}

template<class Tchar>
inline constexpr StringBase<Tchar> StringBase<Tchar>::TrimEnd() const
{
	int64_t Index = 0, Ending = (int64_t)this->_StoreSize - 1;

	for (Ending = (int64_t)this->_StoreSize - 1; Ending >= Index; Ending--)
	{
		if constexpr (sizeof(Tchar) == sizeof(char))
		{
			if (!::isspace(this->_Buffer[Ending]))
				break;
		}
		else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		{
			if (!::iswspace(this->_Buffer[Ending]))
				break;
		}
	}

	auto Length = Ending - Index + 1;
	if (Length == this->_StoreSize)
		return *this;
	else if (Length == 0)
		return "";

	return StringBase<Tchar>(this->_Buffer + Index, (uint32_t)Length);
}

template<class Tchar>
inline constexpr uint32_t StringBase<Tchar>::IndexOf(const Tchar Rhs, uint32_t Pos) const noexcept
{
	if (this->_StoreSize == 0 || Pos >= this->_StoreSize)
		return StringBase<Tchar>::InvalidPosition;

	auto fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::memchr((void*)(this->_Buffer + Pos), (int32_t)Rhs, this->_StoreSize) : (Tchar*)std::wmemchr((const wchar_t*)(this->_Buffer + Pos), (wchar_t)Rhs, this->_StoreSize);
	if (fChPos != nullptr)
		return (uint32_t)(fChPos - this->_Buffer);

	return StringBase<Tchar>::InvalidPosition;
}

template<class Tchar>
inline constexpr uint32_t StringBase<Tchar>::IndexOf(const Tchar* Rhs, uint32_t Pos) const
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);

	size_t RhsSize = 0;
	if constexpr (sizeof(Tchar) == sizeof(char))
		RhsSize = strlen(Rhs) * sizeof(Tchar);
	else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		RhsSize = wcslen(Rhs) * sizeof(Tchar);

	if (RhsSize == 0 || RhsSize > LhsSize || Pos >= this->_StoreSize)
		return StringBase<Tchar>::InvalidPosition;

	auto fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::strchr((const char*)(this->_Buffer + Pos), (int32_t)Rhs[0]) : (Tchar*)std::wcschr((const wchar_t*)(this->_Buffer + Pos), (wchar_t)Rhs[0]);
	if (fChPos != nullptr)
	{
		while (std::memcmp(fChPos, Rhs, RhsSize))
		{
			fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::strchr((const char*)(fChPos + 1), (int32_t)Rhs[0]) : (Tchar*)std::wcschr((const wchar_t*)(fChPos + 1), (wchar_t)Rhs[0]);
			if (!fChPos)
				break;
		}

		if (fChPos != nullptr)
			return (uint32_t)(fChPos - this->_Buffer);
	}

	return StringBase<Tchar>::InvalidPosition;
}

template<class Tchar>
inline constexpr uint32_t StringBase<Tchar>::IndexOf(const Tchar* Rhs, uint32_t Pos, uint32_t Count) const
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);

	size_t RhsSize = 0;
	if constexpr (sizeof(Tchar) == sizeof(char))
		RhsSize = strlen(Rhs) * sizeof(Tchar);
	else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		RhsSize = wcslen(Rhs) * sizeof(Tchar);

	if (RhsSize == 0 || RhsSize > LhsSize || Pos >= this->_StoreSize || RhsSize < (Count * sizeof(Tchar)))
		return StringBase<Tchar>::InvalidPosition;

	RhsSize = (Count * sizeof(Tchar));

	auto fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::strchr((const char*)(this->_Buffer + Pos), (int32_t)Rhs[0]) : (Tchar*)std::wcschr((const wchar_t*)(this->_Buffer + Pos), (wchar_t)Rhs[0]);
	if (fChPos != nullptr)
	{
		while (std::memcmp(fChPos, Rhs, RhsSize))
		{
			fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::strchr((const char*)(fChPos + 1), (int32_t)Rhs[0]) : (Tchar*)std::wcschr((const wchar_t*)(fChPos + 1), (wchar_t)Rhs[0]);
			if (!fChPos)
				break;
		}

		if (fChPos != nullptr)
			return (uint32_t)(fChPos - this->_Buffer);
	}

	return StringBase<Tchar>::InvalidPosition;
}

template<class Tchar>
inline constexpr uint32_t StringBase<Tchar>::IndexOf(const StringBase<Tchar>& Rhs, uint32_t Pos) const noexcept
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);
	auto RhsSize = Rhs._StoreSize * sizeof(Tchar);

	if (RhsSize == 0 || RhsSize > LhsSize || Pos >= this->_StoreSize)
		return StringBase<Tchar>::InvalidPosition;

	auto fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::strchr((const char*)(this->_Buffer + Pos), (int32_t)Rhs._Buffer[0]) : (Tchar*)std::wcschr((const wchar_t*)(this->_Buffer + Pos), (wchar_t)Rhs._Buffer[0]);
	if (fChPos != nullptr)
	{
		while (std::memcmp(fChPos, Rhs._Buffer, RhsSize))
		{
			fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::strchr((const char*)(fChPos + 1), (int32_t)Rhs._Buffer[0]) : (Tchar*)std::wcschr((const wchar_t*)(fChPos + 1), (wchar_t)Rhs._Buffer[0]);
			if (!fChPos)
				break;
		}

		if (fChPos != nullptr)
			return (uint32_t)(fChPos - this->_Buffer);
	}

	return StringBase<Tchar>::InvalidPosition;
}

template<class Tchar>
inline constexpr uint32_t StringBase<Tchar>::IndexOf(const StringBase<Tchar>& Rhs, uint32_t Pos, uint32_t Count) const noexcept
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);
	auto RhsSize = Rhs._StoreSize * sizeof(Tchar);

	if (RhsSize == 0 || RhsSize > LhsSize || Pos >= this->_StoreSize || Rhs._StoreSize < Count)
		return StringBase<Tchar>::InvalidPosition;

	RhsSize = (Count * sizeof(Tchar));

	auto fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::strchr((void*)(this->_Buffer + Pos), (int32_t)Rhs._Buffer[0]) : (Tchar*)std::wcschr((const wchar_t*)(this->_Buffer + Pos), (wchar_t)Rhs._Buffer[0]);
	if (fChPos != nullptr)
	{
		while (std::memcmp(fChPos, Rhs._Buffer, RhsSize))
		{
			fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::strchr((void*)(fChPos + 1), (int32_t)Rhs._Buffer[0]) : (Tchar*)std::wcschr((const wchar_t*)(fChPos + 1), (wchar_t)Rhs._Buffer[0]);
			if (!fChPos)
				break;
		}

		if (fChPos != nullptr)
			return (uint32_t)(fChPos - this->_Buffer);
	}

	return StringBase<Tchar>::InvalidPosition;
}

template<class Tchar>
inline constexpr uint32_t StringBase<Tchar>::IndexOf(std::basic_string_view<Tchar> Rhs, uint32_t Pos) const noexcept
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);
	auto RhsSize = Rhs.size() * sizeof(Tchar);

	if (RhsSize == 0 || RhsSize > LhsSize || Pos >= this->_StoreSize)
		return StringBase<Tchar>::InvalidPosition;

	auto fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::strchr((void*)(this->_Buffer + Pos), (int32_t)Rhs[0]) : (Tchar*)std::wcschr((const wchar_t*)(this->_Buffer + Pos), (wchar_t)Rhs[0]);
	if (fChPos != nullptr)
	{
		while (std::memcmp(fChPos, Rhs.data(), RhsSize))
		{
			fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::strchr((void*)(fChPos + 1), (int32_t)Rhs[0]) : (Tchar*)std::wcschr((const wchar_t*)(fChPos + 1), (wchar_t)Rhs[0]);
			if (!fChPos)
				break;
		}

		if (fChPos != nullptr)
			return (uint32_t)(fChPos - this->_Buffer);
	}

	return StringBase<Tchar>::InvalidPosition;
}

template<class Tchar>
inline constexpr uint32_t StringBase<Tchar>::IndexOf(std::basic_string_view<Tchar> Rhs, uint32_t Pos, uint32_t Count) const noexcept
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);
	auto RhsSize = Rhs.size() * sizeof(Tchar);

	if (RhsSize == 0 || RhsSize > LhsSize || Pos >= this->_StoreSize || Rhs.size() < Count)
		return StringBase<Tchar>::InvalidPosition;

	RhsSize = (Count * sizeof(Tchar));

	auto fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::strchr((void*)(this->_Buffer + Pos), (int32_t)Rhs[0]) : (Tchar*)std::wcschr((const wchar_t*)(this->_Buffer + Pos), (wchar_t)Rhs[0]);
	if (fChPos != nullptr)
	{
		while (std::memcmp(fChPos, Rhs.data(), RhsSize))
		{
			fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::strchr((void*)(fChPos + 1), (int32_t)Rhs[0]) : (Tchar*)std::wcschr((const wchar_t*)(fChPos + 1), (wchar_t)Rhs[0]);
			if (!fChPos)
				break;
		}

		if (fChPos != nullptr)
			return (uint32_t)(fChPos - this->_Buffer);
	}

	return StringBase<Tchar>::InvalidPosition;
}

template<class Tchar>
inline constexpr uint32_t StringBase<Tchar>::LastIndexOf(const Tchar Rhs, uint32_t Pos) const noexcept
{
	if (this->_StoreSize == 0 || Pos >= this->_StoreSize)
		return StringBase<Tchar>::InvalidPosition;

	auto fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::memrchr((void*)(this->_Buffer + Pos), (int32_t)Rhs, this->_StoreSize) : (Tchar*)std::wmemrchr((const wchar_t*)(this->_Buffer + Pos), (wchar_t)Rhs, this->_StoreSize);
	if (fChPos != nullptr)
		return (uint32_t)(fChPos - this->_Buffer);

	return StringBase<Tchar>::InvalidPosition;
}

template<class Tchar>
inline constexpr uint32_t StringBase<Tchar>::LastIndexOf(const Tchar* Rhs, uint32_t Pos) const
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);

	size_t RhsSize = 0;
	if constexpr (sizeof(Tchar) == sizeof(char))
		RhsSize = strlen(Rhs) * sizeof(Tchar);
	else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		RhsSize = wcslen(Rhs) * sizeof(Tchar);

	if (RhsSize == 0 || RhsSize > LhsSize || Pos >= this->_StoreSize)
		return StringBase<Tchar>::InvalidPosition;

	auto fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::memrchr((void*)(this->_Buffer + Pos), (int32_t)Rhs[0], (size_t)this->_StoreSize) : (Tchar*)std::wmemrchr((const wchar_t*)(this->_Buffer + Pos), (wchar_t)Rhs[0], (size_t)this->_StoreSize);
	if (fChPos != nullptr)
	{
		while (std::memcmp(fChPos, Rhs, RhsSize))
		{
			fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::memrchr((void*)(fChPos + 1), (int32_t)Rhs[0], (size_t)this->_StoreSize) : (Tchar*)std::wmemrchr((const wchar_t*)(fChPos + 1), (wchar_t)Rhs[0], (size_t)this->_StoreSize);
			if (!fChPos)
				break;
		}

		if (fChPos != nullptr)
			return (uint32_t)(fChPos - this->_Buffer);
	}

	return StringBase<Tchar>::InvalidPosition;
}

template<class Tchar>
inline constexpr uint32_t StringBase<Tchar>::LastIndexOf(const Tchar* Rhs, uint32_t Pos, uint32_t Count) const
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);

	size_t RhsSize = 0;
	if constexpr (sizeof(Tchar) == sizeof(char))
		RhsSize = strlen(Rhs) * sizeof(Tchar);
	else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		RhsSize = wcslen(Rhs) * sizeof(Tchar);

	if (RhsSize == 0 || RhsSize > LhsSize || Pos >= this->_StoreSize || RhsSize < (Count * sizeof(Tchar)))
		return StringBase<Tchar>::InvalidPosition;

	RhsSize = (Count * sizeof(Tchar));

	auto fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::memrchr((void*)(this->_Buffer + Pos), (int32_t)Rhs[0], (size_t)this->_StoreSize) : (Tchar*)std::wmemrchr((const wchar_t*)(this->_Buffer + Pos), (wchar_t)Rhs[0], (size_t)this->_StoreSize);
	if (fChPos != nullptr)
	{
		while (std::memcmp(fChPos, Rhs, RhsSize))
		{
			fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::memrchr((void*)(fChPos + 1), (int32_t)Rhs[0], (size_t)this->_StoreSize) : (Tchar*)std::wmemrchr((const wchar_t*)(fChPos + 1), (wchar_t)Rhs[0], (size_t)this->_StoreSize);
			if (!fChPos)
				break;
		}

		if (fChPos != nullptr)
			return (uint32_t)(fChPos - this->_Buffer);
	}

	return StringBase<Tchar>::InvalidPosition;
}

template<class Tchar>
inline constexpr uint32_t StringBase<Tchar>::LastIndexOf(const StringBase<Tchar>& Rhs, uint32_t Pos) const noexcept
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);
	auto RhsSize = Rhs._StoreSize * sizeof(Tchar);

	if (RhsSize == 0 || RhsSize > LhsSize || Pos >= this->_StoreSize)
		return StringBase<Tchar>::InvalidPosition;

	auto fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::memrchr((void*)(this->_Buffer + Pos), (int32_t)Rhs._Buffer[0], (size_t)this->_StoreSize) : (Tchar*)std::wmemrchr((const wchar_t*)(this->_Buffer + Pos), (wchar_t)Rhs._Buffer[0], (size_t)this->_StoreSize);
	if (fChPos != nullptr)
	{
		while (std::memcmp(fChPos, Rhs._Buffer, RhsSize))
		{
			fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::memrchr((void*)(fChPos + 1), (int32_t)Rhs._Buffer[0], (size_t)this->_StoreSize) : (Tchar*)std::wmemrchr((const wchar_t*)(fChPos + 1), (wchar_t)Rhs._Buffer[0], (size_t)this->_StoreSize);
			if (!fChPos)
				break;
		}

		if (fChPos != nullptr)
			return (uint32_t)(fChPos - this->_Buffer);
	}

	return StringBase<Tchar>::InvalidPosition;
}

template<class Tchar>
inline constexpr uint32_t StringBase<Tchar>::LastIndexOf(const StringBase<Tchar>& Rhs, uint32_t Pos, uint32_t Count) const noexcept
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);
	auto RhsSize = Rhs._StoreSize * sizeof(Tchar);

	if (RhsSize == 0 || RhsSize > LhsSize || Pos >= this->_StoreSize || Rhs._StoreSize < Count)
		return StringBase<Tchar>::InvalidPosition;

	RhsSize = (Count * sizeof(Tchar));

	auto fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::memrchr((void*)(this->_Buffer + Pos), (int32_t)Rhs._Buffer[0], (size_t)this->_StoreSize) : (Tchar*)std::wmemrchr((const wchar_t*)(this->_Buffer + Pos), (wchar_t)Rhs._Buffer[0], (size_t)this->_StoreSize);
	if (fChPos != nullptr)
	{
		while (std::memcmp(fChPos, Rhs._Buffer, RhsSize))
		{
			fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::memrchr((void*)(fChPos + 1), (int32_t)Rhs._Buffer[0], (size_t)this->_StoreSize) : (Tchar*)std::wmemrchr((const wchar_t*)(fChPos + 1), (wchar_t)Rhs._Buffer[0], (size_t)this->_StoreSize);
			if (!fChPos)
				break;
		}

		if (fChPos != nullptr)
			return (uint32_t)(fChPos - this->_Buffer);
	}

	return StringBase<Tchar>::InvalidPosition;
}

template<class Tchar>
inline constexpr uint32_t StringBase<Tchar>::LastIndexOf(std::basic_string_view<Tchar> Rhs, uint32_t Pos) const noexcept
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);
	auto RhsSize = Rhs.size() * sizeof(Tchar);

	if (RhsSize == 0 || RhsSize > LhsSize || Pos >= this->_StoreSize)
		return StringBase<Tchar>::InvalidPosition;

	auto fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::memrchr((void*)(this->_Buffer + Pos), (int32_t)Rhs[0], (size_t)this->_StoreSize) : (Tchar*)std::wmemrchr((const wchar_t*)(this->_Buffer + Pos), (wchar_t)Rhs[0], (size_t)this->_StoreSize);
	if (fChPos != nullptr)
	{
		while (std::memcmp(fChPos, Rhs.data(), RhsSize))
		{
			fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::memrchr((void*)(fChPos + 1), (int32_t)Rhs[0], (size_t)this->_StoreSize) : (Tchar*)std::wmemrchr((const wchar_t*)(fChPos + 1), (wchar_t)Rhs[0], (size_t)this->_StoreSize);
			if (!fChPos)
				break;
		}

		if (fChPos != nullptr)
			return (uint32_t)(fChPos - this->_Buffer);
	}

	return StringBase<Tchar>::InvalidPosition;
}

template<class Tchar>
inline constexpr uint32_t StringBase<Tchar>::LastIndexOf(std::basic_string_view<Tchar> Rhs, uint32_t Pos, uint32_t Count) const noexcept
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);
	auto RhsSize = Rhs.size() * sizeof(Tchar);

	if (RhsSize == 0 || RhsSize > LhsSize || Pos >= this->_StoreSize || Rhs.size() < Count)
		return StringBase<Tchar>::InvalidPosition;

	RhsSize = (Count * sizeof(Tchar));

	auto fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::memrchr((void*)(this->_Buffer + Pos), (int32_t)Rhs[0], (size_t)this->_StoreSize) : (Tchar*)std::wmemrchr((const wchar_t*)(this->_Buffer + Pos), (wchar_t)Rhs[0], (size_t)this->_StoreSize);
	if (fChPos != nullptr)
	{
		while (std::memcmp(fChPos, Rhs.data(), RhsSize))
		{
			fChPos = (sizeof(Tchar) == sizeof(char)) ? (Tchar*)std::memrchr((void*)(fChPos + 1), (int32_t)Rhs[0], (size_t)this->_StoreSize) : (Tchar*)std::wmemrchr((const wchar_t*)(fChPos + 1), (wchar_t)Rhs[0], (size_t)this->_StoreSize);
			if (!fChPos)
				break;
		}

		if (fChPos != nullptr)
			return (uint32_t)(fChPos - this->_Buffer);
	}

	return StringBase<Tchar>::InvalidPosition;
}

template<class Tchar>
inline constexpr StringBase<Tchar> StringBase<Tchar>::ToLower() const
{
	auto Result = StringBase<Tchar>(this->_Buffer, this->_StoreSize);

	std::transform(Result._Buffer, Result._Buffer + Result._StoreSize, Result._Buffer, (sizeof(Tchar) == sizeof(char)) ? ::tolower : (int(__cdecl*)(int))::towlower);

	return std::move(Result);
}

template<class Tchar>
inline constexpr StringBase<Tchar> StringBase<Tchar>::ToUpper() const
{
	auto Result = StringBase<Tchar>(this->_Buffer, this->_StoreSize);

	std::transform(Result._Buffer, Result._Buffer + Result._StoreSize, Result._Buffer, (sizeof(Tchar) == sizeof(char)) ? ::toupper : (int(__cdecl*)(int))::towupper);

	return std::move(Result);
}

template<class Tchar>
inline constexpr StringBase<Tchar> StringBase<Tchar>::Replace(const Tchar Old, const Tchar New) const
{
	auto Result = StringBase<Tchar>(this->_Buffer, this->_StoreSize);

	for (uint32_t i = 0; i < this->_StoreSize; i++)
	{
		if (Result[i] == Old)
			Result[i] = New;
	}

	return std::move(Result);
}

template<class Tchar>
inline constexpr StringBase<Tchar> StringBase<Tchar>::Replace(const Tchar* Old, const Tchar* New) const
{
	uint32_t CurrentIndex = 0, LocatedPosition = 0;

	uint32_t LhsSize = 0;
	if constexpr (sizeof(Tchar) == sizeof(char))
		LhsSize = (uint32_t)strlen(Old);
	else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		LhsSize = (uint32_t)wcslen(Old);

	auto Result = StringBase<Tchar>();

	// Optimized for large strings and lots of occurences
	while ((LocatedPosition = this->IndexOf(Old, CurrentIndex)) != StringBase<Tchar>::InvalidPosition)
	{
		Result += this->Substring(CurrentIndex, LocatedPosition - CurrentIndex);
		Result += New;

		// Advance past the size of old and the position
		CurrentIndex = LocatedPosition + LhsSize;
	}

	if (CurrentIndex != this->_StoreSize)
		Result += this->Substring(CurrentIndex, this->_StoreSize - CurrentIndex);

	return std::move(Result);
}

template<class Tchar>
inline constexpr StringBase<Tchar> StringBase<Tchar>::Replace(const StringBase<Tchar>& Old, const StringBase<Tchar>& New) const
{
	uint32_t CurrentIndex = 0, LocatedPosition = 0;
	uint32_t LhsSize = Old._StoreSize;

	auto Result = StringBase<Tchar>();

	// Optimized for large strings and lots of occurences
	while ((LocatedPosition = this->IndexOf(Old, CurrentIndex)) != StringBase<Tchar>::InvalidPosition)
	{
		Result += this->Substring(CurrentIndex, LocatedPosition - CurrentIndex);
		Result += New;

		// Advance past the size of old and the position
		CurrentIndex = LocatedPosition + LhsSize;
	}

	if (CurrentIndex != this->_StoreSize)
		Result += this->Substring(CurrentIndex, this->_StoreSize - CurrentIndex);

	return std::move(Result);
}

template<class Tchar>
inline constexpr StringBase<Tchar> StringBase<Tchar>::Replace(std::basic_string_view<Tchar> Old, std::basic_string_view<Tchar> New) const
{
	uint32_t CurrentIndex = 0, LocatedPosition = 0;
	uint32_t LhsSize = (uint32_t)Old.size();

	auto Result = StringBase<Tchar>();

	// Optimized for large strings and lots of occurences
	while ((LocatedPosition = this->IndexOf(Old, CurrentIndex)) != StringBase<Tchar>::InvalidPosition)
	{
		Result += this->Substring(CurrentIndex, LocatedPosition - CurrentIndex);
		Result += New;

		// Advance past the size of old and the position
		CurrentIndex = LocatedPosition + LhsSize;
	}

	if (CurrentIndex != this->_StoreSize)
		Result += this->Substring(CurrentIndex, this->_StoreSize - CurrentIndex);

	return std::move(Result);
}

template<class Tchar>
inline constexpr bool StringBase<Tchar>::StartsWith(const Tchar* Rhs) const
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);

	uint32_t RhsSize = 0;
	if constexpr (sizeof(Tchar) == sizeof(char))
		RhsSize = (uint32_t)strlen(Rhs) * sizeof(Tchar);
	else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		RhsSize = (uint32_t)wcslen(Rhs) * sizeof(Tchar);

	if (RhsSize > LhsSize || RhsSize == 0)
		return false;

	return (std::memcmp(this->_Buffer, Rhs, RhsSize) == 0);
}

template<class Tchar>
inline constexpr bool StringBase<Tchar>::StartsWith(const StringBase<Tchar>& Rhs) const
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);
	auto RhsSize = Rhs._StoreSize * sizeof(Tchar);

	if (RhsSize > LhsSize || RhsSize == 0)
		return false;

	return (std::memcmp(this->_Buffer, Rhs._Buffer, RhsSize) == 0);
}

template<class Tchar>
inline constexpr bool StringBase<Tchar>::StartsWith(std::basic_string_view<Tchar> Rhs) const
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);
	auto RhsSize = Rhs.size() * sizeof(Tchar);

	if (RhsSize > LhsSize || RhsSize == 0)
		return false;

	return (std::memcmp(this->_Buffer, Rhs.data(), RhsSize) == 0);
}

template<class Tchar>
inline constexpr bool StringBase<Tchar>::EndsWith(const Tchar* Rhs) const
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);

	uint32_t RhsSize = 0;
	if constexpr (sizeof(Tchar) == sizeof(char))
		RhsSize = (uint32_t)strlen(Rhs) * sizeof(Tchar);
	else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		RhsSize = (uint32_t)wcslen(Rhs) * sizeof(Tchar);

	if (RhsSize > LhsSize || RhsSize == 0)
		return false;

	return (std::memcmp((char*)this->_Buffer + (LhsSize - RhsSize), Rhs, RhsSize) == 0);
}

template<class Tchar>
inline constexpr bool StringBase<Tchar>::EndsWith(const StringBase<Tchar>& Rhs) const
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);
	auto RhsSize = Rhs._StoreSize * sizeof(Tchar);

	if (RhsSize > LhsSize || RhsSize == 0)
		return false;

	return (std::memcmp((char*)this->_Buffer + (LhsSize - RhsSize), Rhs._Buffer, RhsSize) == 0);
}

template<class Tchar>
inline constexpr bool StringBase<Tchar>::EndsWith(std::basic_string_view<Tchar> Rhs) const
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);
	auto RhsSize = Rhs.size() * sizeof(Tchar);

	if (RhsSize > LhsSize || RhsSize == 0)
		return false;

	return (std::memcmp((char*)this->_Buffer + (LhsSize - RhsSize), Rhs.data(), RhsSize) == 0);
}

template<class Tchar>
inline constexpr bool StringBase<Tchar>::Contains(const Tchar* Rhs) const
{
	return (this->IndexOf(Rhs) != StringBase<Tchar>::InvalidPosition);
}

template<class Tchar>
inline constexpr bool StringBase<Tchar>::Contains(const StringBase<Tchar>& Rhs) const
{
	return (this->IndexOf(Rhs) != StringBase<Tchar>::InvalidPosition);
}

template<class Tchar>
inline constexpr bool StringBase<Tchar>::Contains(std::basic_string_view<Tchar> Rhs) const
{
	return (this->IndexOf(Rhs) != StringBase<Tchar>::InvalidPosition);
}

template<class Tchar>
inline constexpr StringBase<Tchar> StringBase<Tchar>::Substring(uint32_t StartIndex) const
{
	return this->Substring(StartIndex, this->_StoreSize - StartIndex);
}

template<class Tchar>
inline constexpr StringBase<Tchar> StringBase<Tchar>::Substring(uint32_t StartIndex, uint32_t Length) const
{
	if (Length > 0 && StartIndex < this->_StoreSize && (StartIndex + Length) <= this->_StoreSize)
	{
		auto Result = StringBase<Tchar>(Length);

		std::memcpy(Result._Buffer, this->_Buffer + StartIndex, Length * sizeof(Tchar));

		return Result;
	}

	return StringBase<Tchar>();
}

template<class Tchar>
inline constexpr void StringBase<Tchar>::Append(Tchar Rhs)
{
	auto nPos = this->_StoreSize;
	this->EnsureCapacity(nPos + 1);
	this->_Buffer[nPos] = Rhs;
}

template<class Tchar>
inline constexpr void StringBase<Tchar>::Append(const Tchar* Rhs)
{
	size_t vLen = 0;
	if constexpr (sizeof(Tchar) == sizeof(char))
		vLen = strlen(Rhs);
	else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		vLen = wcslen(Rhs);

	auto cPosition = this->_StoreSize;
	this->EnsureCapacity((uint32_t)vLen + this->_StoreSize);

	std::memcpy(this->_Buffer + cPosition, Rhs, vLen * sizeof(Tchar));
}

template<class Tchar>
inline constexpr void StringBase<Tchar>::Append(Tchar* Rhs, uint32_t Count)
{
	auto cPosition = this->_StoreSize;
	this->EnsureCapacity((uint32_t)Count + this->_StoreSize);

	std::memcpy(this->_Buffer + cPosition, Rhs, Count * sizeof(Tchar));
}

template<class Tchar>
inline constexpr void StringBase<Tchar>::Append(const StringBase<Tchar>& Rhs)
{
	auto vLen = Rhs.Length();
	auto cPosition = this->_StoreSize;
	this->EnsureCapacity((uint32_t)vLen + this->_StoreSize);

	std::memcpy(this->_Buffer + cPosition, Rhs._Buffer, vLen * sizeof(Tchar));
}

template<class Tchar>
inline constexpr StringBase<Tchar>::operator Tchar*(void) const
{
	return this->_Buffer;
}

template<class Tchar>
inline constexpr StringBase<Tchar>::operator const Tchar*(void) const
{
	return (const Tchar*)this->_Buffer;
}

template<class Tchar>
inline constexpr StringBase<Tchar>::operator std::basic_string_view<Tchar>(void) const
{
	return std::basic_string_view<Tchar>(this->_Buffer, this->_StoreSize);
}

template<class Tchar>
inline constexpr Tchar& StringBase<Tchar>::operator[](size_t Index)
{
	return this->_Buffer[Index];
}

template<class Tchar>
inline constexpr Tchar& StringBase<Tchar>::operator[](size_t Index) const
{
	return this->_Buffer[Index];
}

template<class Tchar>
inline constexpr StringBase<Tchar>& StringBase<Tchar>::operator+=(const Tchar* Rhs)
{
	size_t vLen = 0;
	if constexpr (sizeof(Tchar) == sizeof(char))
		vLen = strlen(Rhs);
	else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		vLen = wcslen(Rhs);

	auto cPosition = this->_StoreSize;
	this->EnsureCapacity((uint32_t)vLen + this->_StoreSize);

	std::memcpy(this->_Buffer + cPosition, Rhs, vLen * sizeof(Tchar));

	return *this;
}

template<class Tchar>
inline constexpr Tchar * StringBase<Tchar>::begin() const noexcept
{
	return this->_Buffer;
}

template<class Tchar>
inline constexpr Tchar * StringBase<Tchar>::end() const noexcept
{
	return (this->_Buffer + this->_StoreSize);
}

template<class Tchar>
inline constexpr StringBase<Tchar>& StringBase<Tchar>::operator+=(const Tchar Rhs)
{
	auto nPos = this->_StoreSize;
	this->EnsureCapacity(nPos + 1);
	this->_Buffer[nPos] = Rhs;

	return *this;
}

template<class Tchar>
inline constexpr StringBase<Tchar>& StringBase<Tchar>::operator+=(const StringBase<Tchar>& Rhs)
{
	size_t vLen = Rhs._StoreSize;

	auto cPosition = this->_StoreSize;
	this->EnsureCapacity((uint32_t)vLen + this->_StoreSize);

	std::memcpy(this->_Buffer + cPosition, Rhs._Buffer, vLen * sizeof(Tchar));

	return *this;
}

template<class Tchar>
inline constexpr StringBase<Tchar>& StringBase<Tchar>::operator+=(std::basic_string_view<Tchar> Rhs)
{
	size_t vLen = Rhs.size();

	auto cPosition = this->_StoreSize;
	this->EnsureCapacity((uint32_t)vLen + this->_StoreSize);

	std::memcpy(this->_Buffer + cPosition, Rhs.data(), vLen * sizeof(Tchar));

	return *this;
}

template<class Tchar>
inline constexpr StringBase<Tchar>& StringBase<Tchar>::operator+=(ImmutableStringBase<Tchar> Rhs)
{
	size_t vLen = Rhs.Length();

	auto cPosition = this->_StoreSize;
	this->EnsureCapacity((uint32_t)vLen + this->_StoreSize);

	std::memcpy(this->_Buffer + cPosition, (Tchar*)Rhs, vLen * sizeof(Tchar));

	return *this;
}

template<class Tchar>
inline constexpr StringBase<Tchar>& StringBase<Tchar>::operator=(const Tchar* Rhs)
{
	if (this->_Buffer)
		delete[] this->_Buffer;
	this->_Buffer = nullptr;
	this->_BufferSize = 0;

	size_t vLen = 0;
	if constexpr (sizeof(Tchar) == sizeof(char))
		vLen = strlen(Rhs);
	else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		vLen = wcslen(Rhs);

	this->EnsureCapacity((uint32_t)vLen);

	std::memcpy(this->_Buffer, Rhs, vLen * sizeof(Tchar));

	return *this;
}

template<class Tchar>
inline constexpr StringBase<Tchar>& StringBase<Tchar>::operator=(const StringBase<Tchar>& Rhs)
{
	if (this->_Buffer)
		delete[] this->_Buffer;
	this->_Buffer = nullptr;
	this->_BufferSize = 0;

	auto vLen = Rhs._StoreSize;
	this->EnsureCapacity((uint32_t)vLen);

	std::memcpy(this->_Buffer, Rhs._Buffer, vLen * sizeof(Tchar));

	return *this;
}

template<class Tchar>
inline constexpr StringBase<Tchar>& StringBase<Tchar>::operator=(std::basic_string_view<Tchar> Rhs)
{
	if (this->_Buffer)
		delete[] this->_Buffer;
	this->_Buffer = nullptr;
	this->_BufferSize = 0;

	auto vLen = Rhs.size();
	this->EnsureCapacity((uint32_t)vLen);

	std::memcpy(this->_Buffer, Rhs.data(), vLen * sizeof(Tchar));

	return *this;
}

template<class Tchar>
inline constexpr StringBase<Tchar>& StringBase<Tchar>::operator=(ImmutableStringBase<Tchar> Rhs)
{
	if (this->_Buffer)
		delete[] this->_Buffer;
	this->_Buffer = nullptr;
	this->_BufferSize = 0;

	auto vLen = Rhs.Length();
	this->EnsureCapacity((uint32_t)vLen);

	std::memcpy(this->_Buffer, (Tchar*)Rhs, vLen * sizeof(Tchar));

	return *this;
}

template<class Tchar>
inline constexpr bool StringBase<Tchar>::operator==(const Tchar* Rhs) const
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);

	uint32_t RhsSize = 0;
	if constexpr (sizeof(Tchar) == sizeof(char))
		RhsSize = (uint32_t)strlen(Rhs) * sizeof(Tchar);
	else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		RhsSize = (uint32_t)wcslen(Rhs) * sizeof(Tchar);

	if (LhsSize != RhsSize)
		return false;

	return (std::memcmp(this->_Buffer, Rhs, RhsSize) == 0);
}

template<class Tchar>
inline constexpr bool StringBase<Tchar>::operator==(const StringBase<Tchar>& Rhs) const
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);
	auto RhsSize = Rhs._StoreSize * sizeof(Tchar);

	if (LhsSize != RhsSize)
		return false;

	return (std::memcmp(this->_Buffer, Rhs._Buffer, RhsSize) == 0);
}

template<class Tchar>
inline constexpr bool StringBase<Tchar>::operator==(std::basic_string_view<Tchar> Rhs) const
{
	auto LhsSize = this->_StoreSize * sizeof(Tchar);
	auto RhsSize = Rhs.size() * sizeof(Tchar);

	if (LhsSize != RhsSize)
		return false;

	return (std::memcmp(this->_Buffer, Rhs.data(), RhsSize) == 0);
}

template<class Tchar>
inline constexpr bool StringBase<Tchar>::operator!=(const Tchar* Rhs) const
{
	return !(*this == Rhs);
}

template<class Tchar>
inline constexpr bool StringBase<Tchar>::operator!=(const StringBase<Tchar>& Rhs) const
{
	return !(*this == Rhs);
}

template<class Tchar>
inline constexpr bool StringBase<Tchar>::operator!=(std::basic_string_view<Tchar> Rhs) const
{
	return !(*this == Rhs);
}

template<class Tchar>
inline constexpr bool StringBase<Tchar>::IsNullOrEmpty(const StringBase<Tchar>& Rhs)
{
	return (Rhs._Buffer == nullptr || Rhs._StoreSize == 0);
}

template<class Tchar>
inline constexpr bool StringBase<Tchar>::IsNullOrWhiteSpace(const StringBase<Tchar>& Rhs)
{
	if (Rhs._Buffer == nullptr)
		return true;

	for (uint32_t i = 0; i < Rhs._StoreSize; i++)
	{
		if constexpr (sizeof(Tchar) == sizeof(char))
		{
			if (!::isspace(Rhs._Buffer[i]))
				return false;
		}
		else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		{
			if (!::iswspace(Rhs._Buffer[i]))
				return false;
		}
	}

	return true;
}

template<class Tchar>
inline constexpr StringBase<Tchar> StringBase<Tchar>::Format(const Tchar* Format, ...)
{
	va_list vArgs;
	va_start(vArgs, Format);

#pragma warning(suppress: 4996)
	auto BufferSize = (sizeof(Tchar) == sizeof(char)) ? vsnprintf(nullptr, 0, Format, vArgs) : _vsnwprintf(nullptr, 0, (const wchar_t*)Format, vArgs);
	auto Result = StringBase<Tchar>((uint32_t)BufferSize);

	if constexpr (sizeof(Tchar) == sizeof(char))
		vsnprintf(Result._Buffer, BufferSize + 1, Format, vArgs);
	else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		_vsnwprintf(Result._Buffer, BufferSize + 1, (const wchar_t*)Format, vArgs);

	va_end(vArgs);

	return std::move(Result);
}

// this is for a specific use case. make sure that va_start and va_end are called externally when using this
template<class Tchar>
inline constexpr StringBase<Tchar> StringBase<Tchar>::Format(const Tchar* Format, va_list vArgs)
{
#pragma warning(suppress: 4996)
	auto BufferSize = (sizeof(Tchar) == sizeof(char)) ? vsnprintf(nullptr, 0, Format, vArgs) : _vsnwprintf(nullptr, 0, (const wchar_t*)Format, vArgs);
	auto Result = StringBase<Tchar>((uint32_t)BufferSize);

	if constexpr (sizeof(Tchar) == sizeof(char))
		vsnprintf(Result._Buffer, BufferSize + 1, Format, vArgs);
	else if constexpr (sizeof(Tchar) == sizeof(wchar_t))
		_vsnwprintf(Result._Buffer, BufferSize + 1, (const wchar_t*)Format, vArgs);

	return std::move(Result);
}

template<class Tchar>
constexpr inline void StringBase<Tchar>::EnsureCapacity(uint32_t Capacity)
{
	// Ensure that we have a proper buffer size for the string here, this is in units, NOT bytes...
	// All StringBase<Tchar> classes are null-terminated, so we must make sure we set this up properly...
	// Check to ensure we aren't wasting our time first...
	if (Capacity < this->_BufferSize)
	{
		this->_StoreSize = Capacity;
		return;
	}

	auto nCapacity = Capacity + 1;
	if (nCapacity < 16)
		nCapacity = 17;

	if (nCapacity < (this->_BufferSize + (this->_BufferSize / 2)) + 1)
		nCapacity = (this->_BufferSize + (this->_BufferSize / 2)) + 1;

	auto nBuffer = new Tchar[nCapacity]();

	if (this->_Buffer)
	{
		std::memcpy(nBuffer, this->_Buffer, this->_BufferSize * sizeof(Tchar));
		delete[] this->_Buffer;
	}

	this->_Buffer = nBuffer;

	this->_BufferSize = nCapacity - 1;
	this->_StoreSize = Capacity;
}

//
// Predefiend string types
//

using string = StringBase<char>;
using String = StringBase<char>;
using wstring = StringBase<wchar_t>;
using WString = StringBase<wchar_t>;