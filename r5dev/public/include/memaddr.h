#pragma once

class CMemory
{
public:
	enum class Direction : int
	{
		DOWN = 0,
		UP,
	};

	CMemory(void) = default;
	CMemory(uintptr_t ptr) : ptr(ptr) {}
	CMemory(void* ptr) : ptr(uintptr_t(ptr)) {}

	inline operator uintptr_t(void) const
	{
		return ptr;
	}

	inline operator void*(void) const
	{
		return reinterpret_cast<void*>(ptr);
	}

	inline operator bool(void) const
	{
		return ptr != NULL;
	}

	inline bool operator!= (const CMemory& addr) const
	{
		return ptr != addr.ptr;
	}

	inline bool operator== (const CMemory& addr) const
	{
		return ptr == addr.ptr;
	}

	inline bool operator== (const uintptr_t& addr) const
	{
		return ptr == addr;
	}

	inline uintptr_t GetPtr(void) const
	{
		return ptr;
	}

	template<class T> inline T GetValue(void) const
	{
		return *reinterpret_cast<T*>(ptr);
	}

	template<class T> inline T GetVirtualFunctionIndex(void) const
	{
		return *reinterpret_cast<T*>(ptr) / 8; // Its divided by 8 in x64.
	}

	template<typename T> inline T CCast(void) const
	{
		return (T)ptr;
	}

	template<typename T> inline T RCast(void) const
	{
		return reinterpret_cast<T>(ptr);
	}

	inline CMemory Offset(ptrdiff_t offset) const
	{
		return CMemory(ptr + offset);
	}

	inline CMemory OffsetSelf(ptrdiff_t offset)
	{
		ptr += offset;
		return *this;
	}

	inline CMemory Deref(int deref = 1) const
	{
		uintptr_t reference = ptr;

		while (deref--)
		{
			if (reference)
				reference = *reinterpret_cast<uintptr_t*>(reference);
		}

		return CMemory(reference);
	}

	inline CMemory DerefSelf(int deref = 1)
	{
		while (deref--)
		{
			if (ptr)
				ptr = *reinterpret_cast<uintptr_t*>(ptr);
		}

		return *this;
	}

	bool CheckOpCodes(const vector<uint8_t> vOpcodeArray) const;
	void Patch(vector<uint8_t> vOpcodes) const;
	void PatchString(const string& svString) const;
	CMemory FindPattern(const string& svPattern, const Direction searchDirect = Direction::DOWN, const int opCodesToScan = 512, const ptrdiff_t occurence = 1) const;
	CMemory FindPatternSelf(const string& svPattern, const Direction searchDirect = Direction::DOWN, const int opCodesToScan = 512, const ptrdiff_t occurence = 1);
	CMemory FollowNearCall(ptrdiff_t opcodeOffset = 0x1, ptrdiff_t nextInstructionOffset = 0x5) const;
	CMemory FollowNearCallSelf(ptrdiff_t opcodeOffset = 0x1, ptrdiff_t nextInstructionOffset = 0x5);
	CMemory ResolveRelativeAddress(ptrdiff_t registerOffset = 0x0, ptrdiff_t nextInstructionOffset = 0x4) const;
	CMemory ResolveRelativeAddressSelf(ptrdiff_t registerOffset = 0x0, ptrdiff_t nextInstructionOffset = 0x4);
	static void HookVirtualMethod(uintptr_t virtualTable, void* pHookMethod, void** pOriginalMethod, ptrdiff_t methodIndex);

private:
	uintptr_t ptr = 0;
};
