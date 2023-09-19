#pragma once

#include <algorithm>
#include <utility>
#include <memory>
#include "HashHelpers.h"
#include "HashComparer.h"

// A KeyValuePair holds a key and a value from a dictionary.
template<class TKey, class TValue>
struct KeyValuePair : std::pair<TKey, TValue>
{
	constexpr TKey& Key()
	{
		return this->first;
	}

	constexpr TValue& Value()
	{
		return this->second;
	}

	constexpr TKey& Key() const
	{
		return this->first;
	}

	constexpr TValue& Value() const
	{
		return this->second;
	}
};

// A container class that holds keys and values.
template<class TKey, class TValue, class THasher = HashComparer<TKey>>
class Dictionary
{
private:
	typedef KeyValuePair<TKey, TValue> PairType;

	struct Entry
	{
		uint64_t HashCode;	// Lower 63 bits of hash code, -1 if unused
		uint32_t Next;		// Index of next entry, -1 if last
		
		PairType Kvp;

		Entry()
			: HashCode(-1), Next(-1)
		{
		}
	};

public:
	Dictionary();
	Dictionary(uint32_t Capacity);

	constexpr Dictionary(const Dictionary& Value);
	constexpr Dictionary(Dictionary&& Value);

	// Assignment operator
	constexpr Dictionary<TKey, TValue, THasher>& operator=(const Dictionary<TKey, TValue, THasher>& Rhs);
	
	~Dictionary() = default;

	// Adds the specified key and value to the dictionary (Returns: true if added)
	constexpr bool Add(TKey Key, TValue Value);
	// Adds the specified key value pair to the dictionary (Returns: true if added)
	constexpr bool Add(PairType Kvp);

	// Removes a key and value from the dictionary if exists
	constexpr bool Remove(TKey Key);

	// Clears the entries in the dictionary
	constexpr void Clear();

	// Checks whether or not the dictionary contains the key
	constexpr bool ContainsKey(TKey Key);
	// Checks whether or not the dictionary contains the key
	constexpr bool ContainsKey(TKey Key) const;
	// Checks whether or not the dictionary contains the value
	constexpr bool ContainsValue(TValue Value);

	// Attempts to get the value from the specified key
	constexpr bool TryGetValue(TKey Key, TValue& Value);

	// TODO: Keys collection
	// TODO: Values collection

	// Array index operator
	constexpr TValue& operator[](TKey& Key);
	constexpr TValue& operator[](const TKey& Key);
	constexpr TValue& operator[](TKey& Key) const;
	constexpr TValue& operator[](const TKey& Key) const;

	// Define custom iterator for loops
	class DictionaryIterator : public std::iterator<std::forward_iterator_tag, KeyValuePair<TKey, TValue>>
	{
	public:
		DictionaryIterator(const Dictionary<TKey, TValue, THasher>* Dict, uint32_t Index = -1);
		~DictionaryIterator() = default;

		// Increment operator
		DictionaryIterator& operator++()
		{
			MoveNext();
			return *this;
		}

		// Dereference operator
		KeyValuePair<TKey, TValue>& operator*();
		// Const dereference operator
		const KeyValuePair<TKey, TValue>& operator*() const;
		// Pointer access operator
		KeyValuePair<TKey, TValue>* operator->();

		// Inequality operator
		bool operator!=(const DictionaryIterator& Rhs) const;

	protected:
		// Internal cached flags
		const Dictionary<TKey, TValue, THasher>* Dict;
		KeyValuePair<TKey, TValue>* Kvp;
		uint32_t Index;

	private:
		// Move the iterator to the next postion
		void MoveNext();
	};

	// Iterator definitions, for for(& :) loop
	DictionaryIterator begin()
	{
		return DictionaryIterator(this);
	}

	DictionaryIterator end()
	{
		return DictionaryIterator(this, this->_Count);
	}

	// Const iterator definitions, for for(& :) loop
	DictionaryIterator begin() const
	{
		return DictionaryIterator(this);
	}

	DictionaryIterator end() const
	{
		return DictionaryIterator(this, this->_Count);
	}

	// Returns the count of items in the dictionary
	constexpr uint32_t Count() const;

private:
	// Internal buffers
	std::unique_ptr<uint32_t[]> _Buckets;
	std::unique_ptr<Entry[]> _Entries;

	// Internal routine to add to the dictionary
	bool Insert(TKey& Key, TValue& Value, bool Add);
	// Internal routine to setup the dictionary
	void Initialize(uint32_t Capacity);
	// Internal routine to resize the dictionary
	void Resize();
	// Internal routine to resize the dictionary
	void Resize(uint32_t NewSize, bool ForceNewHashCodes);

	// Internal routine to find an entry in the dictionary
	uint32_t FindEntry(const TKey& Key) const;

	// Internal cached counts
	uint32_t _Count;
	uint32_t _FreeList;
	uint32_t _FreeCount;
	uint32_t _BucketLength;
};

template<class TKey, class TValue, class THasher>
inline Dictionary<TKey, TValue, THasher>::Dictionary()
	: Dictionary(0)
{
}

template<class TKey, class TValue, class THasher>
inline Dictionary<TKey, TValue, THasher>::Dictionary(uint32_t Capacity)
	: _Count(0), _FreeList(-1), _FreeCount(0), _BucketLength(0)
{
	if (Capacity > 0)
		Initialize(Capacity);
}

template<class TKey, class TValue, class THasher>
inline constexpr Dictionary<TKey, TValue, THasher>::Dictionary(const Dictionary& Value)
{
	this->_Buckets.reset(new uint32_t[Value._BucketLength]);
	std::memcpy(this->_Buckets.get(), Value._Buckets.get(), Value._BucketLength * sizeof(uint32_t));

	this->_Entries.reset(new Entry[Value._BucketLength]);
	std::copy(Value._Entries.get(), Value._Entries.get() + Value._BucketLength, this->_Entries);
	
	this->_Count = Value._Count;
	this->_FreeList = Value._FreeList;
	this->_FreeCount = Value._FreeCount;
	this->_BucketLength = Value._BucketLength;
}

template<class TKey, class TValue, class THasher>
inline constexpr Dictionary<TKey, TValue, THasher>::Dictionary(Dictionary&& Value)
{
	this->_Buckets.reset(Value._Buckets.release());
	this->_Entries.reset(Value._Entries.release());

	this->_Count = Value._Count;
	this->_FreeList = Value._FreeList;
	this->_FreeCount = Value._FreeCount;
	this->_BucketLength = Value._BucketLength;

	Value._Count = 0;
	Value._FreeList = -1;
	Value._FreeCount = 0;
	Value._BucketLength = 0;
}

template<class TKey, class TValue, class THasher>
inline constexpr Dictionary<TKey, TValue, THasher>& Dictionary<TKey, TValue, THasher>::operator=(const Dictionary<TKey, TValue, THasher>& Rhs)
{
	if (Rhs._BucketLength != 0)
	{
		this->_Buckets.reset(new uint32_t[Rhs._BucketLength]());
		std::memcpy(this->_Buckets.get(), Rhs._Buckets.get(), Rhs._BucketLength * sizeof(uint32_t));

		this->_Entries.reset(new Entry[Rhs._BucketLength]);
		std::copy(Rhs._Entries.get(), Rhs._Entries.get() + Rhs._BucketLength, this->_Entries.get());
	}
	else
	{
		this->_Buckets.reset();
		this->_Entries.reset();
	}

	this->_Count = Rhs._Count;
	this->_FreeList = Rhs._FreeList;
	this->_FreeCount = Rhs._FreeCount;
	this->_BucketLength = Rhs._BucketLength;

	return *this;
}

template<class TKey, class TValue, class THasher>
inline constexpr bool Dictionary<TKey, TValue, THasher>::Add(TKey Key, TValue Value)
{
	return Insert(Key, Value, true);
}

template<class TKey, class TValue, class THasher>
inline constexpr bool Dictionary<TKey, TValue, THasher>::Add(PairType Kvp)
{
	return Insert(Kvp.Key(), Kvp.Value(), true);
}

template<class TKey, class TValue, class THasher>
inline constexpr bool Dictionary<TKey, TValue, THasher>::Remove(TKey Key)
{
	if (this->_Buckets == nullptr)
		return false;

	auto HashCode = THasher::GetHashCode(Key) & INT64_MAX;
	auto Bucket = HashCode % this->_BucketLength;
	uint32_t Last = -1;

	for (uint32_t i = this->_Buckets[Bucket]; i != -1; Last = i, i = this->_Entries[i].Next)
	{
		if (this->_Entries[i].HashCode == HashCode && THasher::Equals(this->_Entries[i].Kvp.Key(), Key))
		{
			if (Last == -1)
				this->_Buckets[Bucket] = this->_Entries[i].Next;
			else
				this->_Entries[Last].Next = this->_Entries[i].Next;

			this->_Entries[i].HashCode = -1;
			this->_Entries[i].Next = this->_FreeList;
			this->_Entries[i].Kvp.Key() = {};
			this->_Entries[i].Kvp.Value() = {};

			this->_FreeList = i;
			this->_FreeCount++;

			return true;
		}
	}

	return true;
}

template<class TKey, class TValue, class THasher>
inline constexpr void Dictionary<TKey, TValue, THasher>::Clear()
{
	if (this->_Count > 0)
	{
		this->_Buckets.reset();
		this->_Entries.reset();

		this->_FreeList = -1;
		this->_Count = 0;
		this->_FreeCount = 0;
	}
}

template<class TKey, class TValue, class THasher>
inline constexpr bool Dictionary<TKey, TValue, THasher>::ContainsKey(TKey Key)
{
	return (FindEntry(Key) != -1);
}

template<class TKey, class TValue, class THasher>
inline constexpr bool Dictionary<TKey, TValue, THasher>::ContainsKey(TKey Key) const
{
	return (FindEntry(Key) != -1);
}

template<class TKey, class TValue, class THasher>
inline constexpr bool Dictionary<TKey, TValue, THasher>::ContainsValue(TValue Value)
{
	for (uint32_t i = 0; i < this->_Count; i++)
		if (this->_Entries[i].HashCode != -1 && this->_Entries[i].Value == Value)
			return true;

	return false;
}

template<class TKey, class TValue, class THasher>
inline constexpr bool Dictionary<TKey, TValue, THasher>::TryGetValue(TKey Key, TValue& Value)
{
	auto Index = FindEntry(Key);

	if (Index != -1)
	{
		Value = this->_Entries[Index].Kvp.Value();
		return true;
	}

	Value = {};
	return false;
}

template<class TKey, class TValue, class THasher>
inline constexpr TValue& Dictionary<TKey, TValue, THasher>::operator[](TKey& Key)
{
	auto Index = FindEntry(Key);

	if (Index == -1)
		throw std::exception();

	return this->_Entries[Index].Kvp.Value();
}

template<class TKey, class TValue, class THasher>
inline constexpr TValue& Dictionary<TKey, TValue, THasher>::operator[](const TKey& Key)
{
	auto Index = FindEntry(Key);

	if (Index == -1)
		throw std::exception();

	return this->_Entries[Index].Kvp.Value();
}

template<class TKey, class TValue, class THasher>
inline constexpr TValue& Dictionary<TKey, TValue, THasher>::operator[](TKey& Key) const
{
	auto Index = FindEntry(Key);

	if (Index == -1)
		throw std::exception();

	return this->_Entries[Index].Kvp.Value();
}

template<class TKey, class TValue, class THasher>
inline constexpr TValue& Dictionary<TKey, TValue, THasher>::operator[](const TKey& Key) const
{
	auto Index = FindEntry(Key);

	if (Index == -1)
		throw std::exception();

	return this->_Entries[Index].Kvp.Value();
}

template<class TKey, class TValue, class THasher>
inline constexpr uint32_t Dictionary<TKey, TValue, THasher>::Count() const
{
	return (this->_Count - this->_FreeCount);
}

template<class TKey, class TValue, class THasher>
inline bool Dictionary<TKey, TValue, THasher>::Insert(TKey& Key, TValue& Value, bool Add)
{
	if (this->_Buckets == nullptr)
		Initialize(0);

	auto HashCode = THasher::GetHashCode(Key) & INT64_MAX;
	auto TargetBucket = HashCode % this->_BucketLength;

	for (uint32_t i = this->_Buckets[TargetBucket]; i != -1; i = this->_Entries[i].Next)
	{
		if (this->_Entries[i].HashCode == HashCode && THasher::Equals(this->_Entries[i].Kvp.Key(), Key))
		{
			if (Add)
				return false;

			this->_Entries[i].Kvp.Value() = Value;
			return false;
		}
	}

	uint32_t Index = 0;

	if (this->_FreeCount > 0)
	{
		Index = this->_FreeList;
		this->_FreeList = this->_Entries[Index].Next;
		this->_FreeCount--;
	}
	else 
	{
		if (this->_Count == this->_BucketLength)
		{
			Resize();
			TargetBucket = HashCode % this->_BucketLength;
		}

		Index = this->_Count;
		this->_Count++;
	}

	this->_Entries[Index].HashCode = HashCode;
	this->_Entries[Index].Next = this->_Buckets[TargetBucket];
	this->_Entries[Index].Kvp.Key() = Key;
	this->_Entries[Index].Kvp.Value() = Value;

	this->_Buckets[TargetBucket] = Index;

	return true;
}

template<class TKey, class TValue, class THasher>
inline void Dictionary<TKey, TValue, THasher>::Initialize(uint32_t Capacity)
{
	auto Size = HashHelpers::GetPrime(Capacity);
	
	this->_Buckets.reset(new uint32_t[Size]);
	std::memset(this->_Buckets.get(), 0xFF, sizeof(uint32_t) * Size);

	this->_Entries.reset(new Entry[Size]);
	this->_FreeList = -1;
	this->_BucketLength = Size;
}

template<class TKey, class TValue, class THasher>
inline void Dictionary<TKey, TValue, THasher>::Resize()
{
	Resize(HashHelpers::ExpandPrime(this->_Count), false);
}

template<class TKey, class TValue, class THasher>
inline void Dictionary<TKey, TValue, THasher>::Resize(uint32_t NewSize, bool ForceNewHashCodes)
{
	auto NewBuckets = new uint32_t[NewSize];
	std::memset(NewBuckets, 0xFF, NewSize * sizeof(uint32_t));

	auto NewEntries = new Entry[NewSize]{};
	std::copy(this->_Entries.get(), this->_Entries.get() + this->_Count, NewEntries);

	if (ForceNewHashCodes)
	{
		for (uint32_t i = 0; i < this->_Count; i++)
		{
			if (NewEntries[i].HashCode != -1)
				NewEntries[i].HashCode = (THasher::GetHashCode(NewEntries[i].Kvp.Key()) & INT64_MAX);
		}
	}

	for (uint32_t i = 0; i < this->_Count; i++)
	{
		if (NewEntries[i].HashCode != -1)
		{
			auto Bucket = NewEntries[i].HashCode % NewSize;
			NewEntries[i].Next = NewBuckets[Bucket];
			NewBuckets[Bucket] = i;
		}
	}

	this->_Buckets.reset(NewBuckets);
	this->_Entries.reset(NewEntries);

	this->_BucketLength = NewSize;
}

template<class TKey, class TValue, class THasher>
inline uint32_t Dictionary<TKey, TValue, THasher>::FindEntry(const TKey& Key) const
{
	// Strip const modifiers
	TKey& KeyUse = *((TKey*)(&Key));

	if (this->_Buckets == nullptr)
		return -1;

	auto HashCode = THasher::GetHashCode(KeyUse) & INT64_MAX;
	for (uint32_t i = this->_Buckets[HashCode % this->_BucketLength]; i != -1; i = this->_Entries[i].Next)
	{
		if (this->_Entries[i].HashCode == HashCode && THasher::Equals(this->_Entries[i].Kvp.Key(), KeyUse))
			return i;
	}

	return -1;
}

template<class TKey, class TValue, class THasher>
inline Dictionary<TKey, TValue, THasher>::DictionaryIterator::DictionaryIterator(const Dictionary<TKey, TValue, THasher>* Dict, uint32_t Index)
	: Dict(Dict), Kvp(nullptr), Index(Index)
{
	if (Index == -1)
		MoveNext();	// Assigns first kvp
}

template<class TKey, class TValue, class THasher>
inline KeyValuePair<TKey, TValue>& Dictionary<TKey, TValue, THasher>::DictionaryIterator::operator*()
{
	return *Kvp;
}

template<class TKey, class TValue, class THasher>
inline const KeyValuePair<TKey, TValue>& Dictionary<TKey, TValue, THasher>::DictionaryIterator::operator*() const
{
	return *Kvp;
}

template<class TKey, class TValue, class THasher>
inline KeyValuePair<TKey, TValue>* Dictionary<TKey, TValue, THasher>::DictionaryIterator::operator->()
{
	return Kvp;
}

template<class TKey, class TValue, class THasher>
inline bool Dictionary<TKey, TValue, THasher>::DictionaryIterator::operator!=(const DictionaryIterator& Rhs) const
{
	if (this->Index != Rhs.Index)
		return true;

	return false;
}

template<class TKey, class TValue, class THasher>
inline void Dictionary<TKey, TValue, THasher>::DictionaryIterator::MoveNext()
{
	Index++;

	while (Index < this->Dict->_Count)
	{
		if (this->Dict->_Entries[Index].HashCode != -1)
		{
			Kvp = &this->Dict->_Entries[Index].Kvp;
			return;
		}

		Index++;
	}

	Index = this->Dict->_Count;
}
