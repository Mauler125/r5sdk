#pragma once

#include <cstdint>

template<class Tchar>
class ImmutableStringBase
{
public:
	constexpr ImmutableStringBase(const Tchar* Value);

	// Returns a null-terminated string
	constexpr const Tchar* ToCString() const;
	// Returns the length of the string in characters
	constexpr uint32_t Length() const;

	// Standard logical casts
	constexpr operator Tchar*(void) const;
	constexpr operator const Tchar*(void) const;

	// Iterator definitions, for for(& :) loop
	constexpr Tchar* begin() const noexcept;
	constexpr Tchar* end() const noexcept;

	// Array index operator
	constexpr const Tchar& operator[](uint32_t Index) const;

private:
	uint32_t _StoreSize;
	const Tchar* _Buffer;

	// Calculates the length of the string, including the null-terminator
	constexpr uint32_t ExpressionLength(const Tchar* Value);
};

template<class Tchar>
inline constexpr ImmutableStringBase<Tchar>::ImmutableStringBase(const Tchar* Value)
	: _StoreSize(ExpressionLength(Value)), _Buffer(Value)
{
}

template<class Tchar>
inline constexpr const Tchar* ImmutableStringBase<Tchar>::ToCString() const
{
	return (const Tchar*)&this->_Buffer[0];
}

template<class Tchar>
inline constexpr uint32_t ImmutableStringBase<Tchar>::Length() const
{
	return this->_StoreSize;
}

template<class Tchar>
inline constexpr ImmutableStringBase<Tchar>::operator Tchar*(void) const
{
	return (Tchar*)&this->_Buffer[0];
}

template<class Tchar>
inline constexpr ImmutableStringBase<Tchar>::operator const Tchar*(void) const
{
	return (const Tchar*)&this->_Buffer[0];
}

template<class Tchar>
inline constexpr Tchar* ImmutableStringBase<Tchar>::begin() const noexcept
{
	return (Tchar*)&this->_Buffer[0];
}

template<class Tchar>
inline constexpr Tchar* ImmutableStringBase<Tchar>::end() const noexcept
{
	return (Tchar*)&this->_Buffer[this->_StoreSize];
}

template<class Tchar>
inline constexpr const Tchar& ImmutableStringBase<Tchar>::operator[](uint32_t Index) const
{
	return this->_Buffer[Index];
}

template<class Tchar>
inline constexpr uint32_t ImmutableStringBase<Tchar>::ExpressionLength(const Tchar* Value)
{
	if constexpr (sizeof(Tchar) == sizeof(char))
		return (uint32_t)strlen(Value);
	else
		return (uint32_t)wcslen(Value);
}

//
// Predefined immutable string types
//

// An immutable string
using imstring = ImmutableStringBase<char>;
// An immutable string
using ImString = ImmutableStringBase<char>;
// An immutable wide string
using wimstring = ImmutableStringBase<wchar_t>;
// An immutable wide string
using WImString = ImmutableStringBase<wchar_t>;