#pragma once

#include <algorithm>

template<class Titem>
class List
{
public:
	constexpr List();
	constexpr List(uint32_t InitialSize, bool Initialized = false);
	constexpr List(const Titem* Items, uint32_t Size);

	template<size_t Size>
	constexpr List(const Titem(&Value)[Size]);

	constexpr List(const List& Value);
	constexpr List(List&& Value);

	// Assignment operator
	constexpr List<Titem>& operator=(const List<Titem>& Rhs);

	~List();

	// Adds a copy of the item to the list
	constexpr void Add(const Titem& Item);
	// Adds a copy of the item to the list
	constexpr void Add(Titem&& Item);
	// Adds a range of items to the list
	constexpr void AddRange(const Titem* Items, uint32_t Size);

	// Inserts a copy of the item to the list
	constexpr void Insert(int32_t Index, const Titem& Item);
	// Inserts a copy of the item to the list
	constexpr void Insert(int32_t Index, Titem&& Item);

	template<class... Args>
	// Construct or move the item in the list
	constexpr void EmplaceBack(Args&&... Arguments);

	template<class... Args>
	// Construct or move the item into the list, returning the new item
	constexpr Titem& Emplace(Args&&... Arguments);

	// Clears all items from the list
	constexpr void Clear();
	// Removes an item at the index
	constexpr void RemoveAt(uint32_t Index);
	// Attempts to remove the item from the list
	constexpr bool Remove(Titem& Item);

	// Sorts the list with the default compare routine
	constexpr void Sort();
	// Sorts the list with the compare routine
	template<class Compare>
	constexpr void Sort(Compare Routine);

	// Returns the index of the first occurance of value in the current instance
	constexpr uint32_t IndexOf(const Titem& Item) const noexcept;
	// Returns the index of the last occurance of the value in the current instance
	constexpr uint32_t LastIndexOf(const Titem& Item) const noexcept;
	// Whether or not the list contains the value
	constexpr bool Contains(const Titem& Item) const;

	// Iterator definitions, for for(& :) loop
	constexpr Titem* begin() const noexcept;
	constexpr Titem* end() const noexcept;

	// Returns the count of items in the list
	constexpr uint32_t Count() const;
	// Returns whether or not the list is empty
	constexpr bool Empty() const;

	// Standard logical cast
	constexpr operator Titem*(void) const;

	// Array index operator
	constexpr Titem& operator[](size_t Index);

	// Indicates no matches in the list 
	static constexpr auto InvalidPosition{ static_cast<uint32_t>(-1) };

private:
	Titem* _Buffer;
	uint32_t _BufferSize;
	uint32_t _StoreSize;
	
	constexpr void EnsureCapacity(uint32_t Capacity);
};

template<class Titem>
inline constexpr List<Titem>::List()
	: _Buffer(nullptr), _BufferSize(0), _StoreSize(0)
{
}

template<class Titem>
inline constexpr List<Titem>::List(uint32_t InitialSize, bool Initialized)
	: _Buffer(nullptr), _BufferSize(0), _StoreSize(0)
{
	this->EnsureCapacity(InitialSize);
	
	if (!Initialized)
		this->_StoreSize = 0;	// Must reset this so that we start from 0
}

template<class Titem>
inline constexpr List<Titem>::List(const Titem* Items, uint32_t Size)
	: _Buffer(nullptr), _BufferSize(0), _StoreSize(0)
{
	this->EnsureCapacity(Size);

	if constexpr (std::is_trivially_copyable<Titem>::value)
		std::memcpy(this->_Buffer, Items, Size * sizeof(Titem));
	else
		std::copy(Items, Items + Size, this->_Buffer);
}

template<class Titem>
template<size_t Size>
inline constexpr List<Titem>::List(const Titem(&Value)[Size])
	: _Buffer(nullptr), _BufferSize(0), _StoreSize(0)
{
	this->EnsureCapacity(Size);

	if constexpr (std::is_trivially_copyable<Titem>::value)
		std::memcpy(this->_Buffer, &Value[0], Size * sizeof(Titem));
	else
		std::copy(&Value[0], &Value[0] + Size, this->_Buffer);
}

template<class Titem>
template<class... Args>
inline constexpr void List<Titem>::EmplaceBack(Args&&... Arguments)
{
	auto nPos = this->_StoreSize;
	this->EnsureCapacity(nPos + 1);
	
	new (&this->_Buffer[nPos]) Titem(std::forward<Args>(Arguments)...);
}

template<class Titem>
template<class... Args>
inline constexpr Titem& List<Titem>::Emplace(Args&&... Arguments)
{
	auto nPos = this->_StoreSize;
	this->EnsureCapacity(nPos + 1);

	new (&this->_Buffer[nPos]) Titem(std::forward<Args>(Arguments)...);

	return this->_Buffer[nPos];
}

template<class Titem>
inline constexpr List<Titem>::List(const List& Value)
	: List<Titem>(Value._Buffer, Value._StoreSize)
{
}

template<class Titem>
inline constexpr List<Titem>::List(List&& Value)
{
	this->_Buffer = Value._Buffer;
	this->_BufferSize = Value._BufferSize;
	this->_StoreSize = Value._StoreSize;

	Value._Buffer = nullptr;
	Value._BufferSize = 0;
	Value._StoreSize = 0;
}

template<class Titem>
inline constexpr List<Titem>& List<Titem>::operator=(const List<Titem>& Rhs)
{
	this->Clear();
	this->AddRange(Rhs.begin(), Rhs.Count());

	return *this;
}

template<class Titem>
inline List<Titem>::~List()
{
	if (this->_Buffer)
		delete[] this->_Buffer;

	this->_Buffer = nullptr;
	this->_BufferSize = 0;
	this->_StoreSize = 0;
}

template<class Titem>
inline constexpr void List<Titem>::Add(const Titem& Item)
{
	this->EmplaceBack(Item);
}

template<class Titem>
inline constexpr void List<Titem>::Add(Titem&& Item)
{
	this->EmplaceBack(std::move(Item));
}

template<class Titem>
inline constexpr void List<Titem>::AddRange(const Titem* Items, uint32_t Size)
{
	auto nPos = this->_StoreSize;
	this->EnsureCapacity(this->_StoreSize + Size);

	if constexpr (std::is_trivially_copyable<Titem>::value)
		std::memcpy(this->_Buffer + nPos, Items, Size * sizeof(Titem));
	else
		std::copy(Items, Items + Size, this->_Buffer + nPos);
}

template<class Titem>
inline constexpr void List<Titem>::Insert(int32_t Index, const Titem& Item)
{
	auto nPos = this->_StoreSize;
	this->EnsureCapacity(nPos + 1);

	if constexpr (std::is_trivially_copyable<Titem>::value)
		std::memmove(this->_Buffer + Index + 1, this->_Buffer + Index, (nPos - Index) * sizeof(Titem));
	else
		std::copy_backward(this->_Buffer + Index, this->_Buffer + nPos, this->_Buffer + nPos + 1);

	new (&this->_Buffer[Index]) Titem(Item);
}

template<class Titem>
inline constexpr void List<Titem>::Insert(int32_t Index, Titem&& Item)
{
	auto nPos = this->_StoreSize;
	this->EnsureCapacity(nPos + 1);

	if constexpr (std::is_trivially_copyable<Titem>::value)
		std::memmove(this->_Buffer + Index + 1, this->_Buffer + Index, (nPos - Index) * sizeof(Titem));
	else
		std::copy_backward(this->_Buffer + Index, this->_Buffer + nPos, this->_Buffer + nPos + 1);

	new (&this->_Buffer[Index]) Titem(std::move(Item));
}

template<class Titem>
inline constexpr void List<Titem>::Clear()
{
	if (this->_Buffer)
		delete[] this->_Buffer;

	this->_Buffer = nullptr;
	this->_BufferSize = 0;
	this->_StoreSize = 0;
}

template<class Titem>
inline constexpr void List<Titem>::RemoveAt(uint32_t Index)
{
	if (Index >= this->_StoreSize)
		return;

	this->_StoreSize--;
	if (Index < this->_StoreSize)
		std::move(this->_Buffer + (Index + 1), this->end() + 1, this->_Buffer + Index);

	this->_Buffer[this->_StoreSize] = Titem();
}

template<class Titem>
inline constexpr bool List<Titem>::Remove(Titem& Item)
{
	auto Index = this->IndexOf(Item);
	if (Index != List<Titem>::InvalidPosition)
	{
		this->RemoveAt(Index);
		return true;
	}

	return false;
}

template<class Titem>
inline constexpr void List<Titem>::Sort()
{
	std::stable_sort(this->begin(), this->end());
}

template<class Titem>
template<class Compare>
inline constexpr void List<Titem>::Sort(Compare Routine)
{
	std::stable_sort(this->begin(), this->end(), Routine);
}

template<class Titem>
inline constexpr uint32_t List<Titem>::IndexOf(const Titem& Item) const noexcept
{
	for (uint32_t i = 0; i < this->_StoreSize; i++)
	{
		if (this->_Buffer[i] == Item)
			return i;
	}

	return List<Titem>::InvalidPosition;
}

template<class Titem>
inline constexpr uint32_t List<Titem>::LastIndexOf(const Titem& Item) const noexcept
{
	for (uint32_t i = this->_StoreSize; i >= 0; --i)
	{
		if (this->_Buffer[i] == Item)
			return i;
	}

	return List<Titem>::InvalidPosition;
}

template<class Titem>
inline constexpr bool List<Titem>::Contains(const Titem& Item) const
{
	return (this->IndexOf(Item) != List<Titem>::InvalidPosition);
}

template<class Titem>
inline constexpr Titem* List<Titem>::begin() const noexcept
{
	return this->_Buffer;
}

template<class Titem>
inline constexpr Titem* List<Titem>::end() const noexcept
{
	return (this->_Buffer + this->_StoreSize);
}

template<class Titem>
inline constexpr uint32_t List<Titem>::Count() const
{
	return this->_StoreSize;
}

template<class Titem>
inline constexpr bool List<Titem>::Empty() const
{
	return (this->Count() == 0);
}

template<class Titem>
inline constexpr List<Titem>::operator Titem*(void) const
{
	return this->_Buffer;
}

template<class Titem>
inline constexpr Titem& List<Titem>::operator[](size_t Index)
{
	return this->_Buffer[Index];
}

template<class Titem>
inline constexpr void List<Titem>::EnsureCapacity(uint32_t Capacity)
{
	// Ensure that we have a proper buffer size for the string here, this is in units, NOT bytes...
	// Check to ensure we aren't wasting our time first...
	if (Capacity <= this->_BufferSize)
	{
		this->_StoreSize = Capacity;
		return;
	}

	auto nCapacity = Capacity;
	if (nCapacity < 16)
		nCapacity = 16;

	if (nCapacity < (this->_BufferSize + (this->_BufferSize / 2)))
		nCapacity = (this->_BufferSize + (this->_BufferSize / 2));

	auto tBuffer = this->_Buffer;
	this->_Buffer = new Titem[nCapacity]();

	if (tBuffer)
	{
		if constexpr (std::is_trivially_copyable<Titem>::value)
			std::memcpy(this->_Buffer, tBuffer, this->_BufferSize * sizeof(Titem));
		else
			std::copy(tBuffer, tBuffer + this->_BufferSize, this->_Buffer);

		delete[] tBuffer;
	}

	this->_BufferSize = nCapacity;
	this->_StoreSize = Capacity;
}
