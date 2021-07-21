#pragma once

class MemoryAddress
{
public:

	enum class Direction : int
	{
		UP = 0,
		DOWN,
	};

	std::uintptr_t GetPtr()
	{
		return ptr;
	}
	
	MemoryAddress() = default;
	MemoryAddress(std::uintptr_t ptr) : ptr(ptr) {}
	MemoryAddress(void* ptr) : ptr(std::uintptr_t(ptr)) {}

	operator std::uintptr_t() const
	{
		return ptr;
	}

	operator void*()
	{
		return reinterpret_cast<void*>(ptr);
	}

	operator bool()
	{
		return ptr != NULL;
	}

	bool operator!= (const MemoryAddress& addr) const
	{
		return ptr != addr.ptr;
	}

	bool operator== (const MemoryAddress& addr) const
	{
		return ptr == addr.ptr;
	}

	bool operator== (const std::uintptr_t& addr) const
	{
		return ptr == addr;
	}

	template<typename T> T CCast()
	{
		return (T)ptr;
	}

	template<typename T> T RCast()
	{
		return reinterpret_cast<T>(ptr);
	}

	template<class T> T GetValue()
	{
		return *reinterpret_cast<T*>(ptr);
	}

	MemoryAddress Offset(std::ptrdiff_t offset)
	{
		return MemoryAddress(ptr + offset);
	}

	MemoryAddress OffsetSelf(std::ptrdiff_t offset)
	{
		ptr += offset;
		return *this;
	}

	MemoryAddress Deref(int deref = 1)
	{
		std::uintptr_t reference = ptr;

		while (deref--)
		{
			if (reference)
				reference = *reinterpret_cast<std::uintptr_t*>(reference);
		}

		return MemoryAddress(reference);
	}

	MemoryAddress DerefSelf(int deref = 1)
	{
		while (deref--)
		{
			if (ptr)
				ptr = *reinterpret_cast<std::uintptr_t*>(ptr);
		}

		return *this;
	}

	bool CheckOpCodes(const std::vector<std::uint8_t> opcodeArray)
	{
		std::uintptr_t reference = ptr; // Create pointer reference.

		for (auto [byteAtCurrentAddress, i] = std::tuple{ std::uint8_t(), (std::size_t)0 }; i < opcodeArray.size(); i++, reference++) // Loop forward in the ptr class member.
		{
			byteAtCurrentAddress = *reinterpret_cast<std::uint8_t*>(reference); // Get byte at current address.

			if (byteAtCurrentAddress != opcodeArray[i]) // If byte at ptr doesn't equal in the byte array return false.
				return false;
		}

		return true;
	}

	template<class T> T GetVirtualFunctionIndex()
	{
		return *reinterpret_cast<T*>(ptr) / 8; // Its divided by 8 in x64.
	}

	void Patch(std::vector<std::uint8_t> opcodes)
	{
		DWORD oldProt = NULL;

		SIZE_T dwSize = opcodes.size();
		VirtualProtect((void*)ptr, dwSize, PAGE_EXECUTE_READWRITE, &oldProt); // Patch page to be able to read and write to it.

		for (int i = 0; i < opcodes.size(); i++)
		{
			*(std::uint8_t*)(ptr + i) = opcodes[i]; // Write opcodes to address.
		}

		dwSize = opcodes.size();
		VirtualProtect((void*)ptr, dwSize, oldProt, &oldProt); // Restore protection.
	}

	MemoryAddress FindPatternSelf(const char* pattern, Direction searchDirect, int opCodesToScan = 100, int occurence = 1)
	{
		static auto PatternToBytes = [](const char* pattern)
		{
			char* PatternStart = const_cast<char*>(pattern); // Cast const away and get start of pattern.
			char* PatternEnd = PatternStart + std::strlen(pattern); // Get end of pattern.

			std::vector<std::int32_t> Bytes = std::vector<std::int32_t>{ }; // Initialize byte vector.

			for (char* CurrentByte = PatternStart; CurrentByte < PatternEnd; ++CurrentByte)
			{
				if (*CurrentByte == '?') // Is current char(byte) a wildcard?
				{
					++CurrentByte; // Skip 1 character.

					if (*CurrentByte == '?') // Is it a double wildcard pattern?
						++CurrentByte; // If so skip the next space that will come up so we can reach the next byte.

					Bytes.push_back(-1); // Push the byte back as invalid.
				}
				else
				{
					// https://stackoverflow.com/a/43860875/12541255
					// Here we convert our string to a unsigned long integer. We pass our string then we use 16 as the base because we want it as hexadecimal.
					// Afterwards we push the byte into our bytes vector.
					Bytes.push_back(std::strtoul(CurrentByte, &CurrentByte, 16));
				}
			}
			return Bytes;
		};

		std::uint8_t* ScanBytes = reinterpret_cast<std::uint8_t*>(ptr); // Get the base of the module.

		const std::vector<int> PatternBytes = PatternToBytes(pattern); // Convert our pattern to a byte array.
		const std::pair BytesInfo = std::make_pair(PatternBytes.size(), PatternBytes.data()); // Get the size and data of our bytes.
		int occurences = 0;

		for (auto i = 01; i < opCodesToScan + BytesInfo.first; ++i)
		{
			bool FoundAddress = true;

			int memOffset = searchDirect == Direction::UP ? -i : i;

			for (DWORD j = 0ul; j < BytesInfo.first; ++j)
			{
				// If either the current byte equals to the byte in our pattern or our current byte in the pattern is a wildcard
				// our if clause will be false.
				if (ScanBytes[memOffset + j] != BytesInfo.second[j] && BytesInfo.second[j] != -1)
				{
					FoundAddress = false;
					break;
				}
			}

			if (FoundAddress)
			{
				occurences++;
				if (occurence == occurences)
				{
					ptr = reinterpret_cast<std::uintptr_t>(&ScanBytes[memOffset]);
					return *this;
				}
			}

		}

		ptr = std::uintptr_t();
		return *this;
	}

	MemoryAddress FindPattern(const char* pattern, Direction searchDirect, int opCodesToScan = 100, int occurence = 1)
	{
		static auto PatternToBytes = [](const char* pattern)
		{
			char* PatternStart = const_cast<char*>(pattern); // Cast const away and get start of pattern.
			char* PatternEnd = PatternStart + std::strlen(pattern); // Get end of pattern.

			std::vector<std::int32_t> Bytes = std::vector<std::int32_t>{ }; // Initialize byte vector.

			for (char* CurrentByte = PatternStart; CurrentByte < PatternEnd; ++CurrentByte)
			{
				if (*CurrentByte == '?') // Is current char(byte) a wildcard?
				{
					++CurrentByte; // Skip 1 character.

					if (*CurrentByte == '?') // Is it a double wildcard pattern?
						++CurrentByte; // If so skip the next space that will come up so we can reach the next byte.

					Bytes.push_back(-1); // Push the byte back as invalid.
				}
				else
				{
					// https://stackoverflow.com/a/43860875/12541255
					// Here we convert our string to a unsigned long integer. We pass our string then we use 16 as the base because we want it as hexadecimal.
					// Afterwards we push the byte into our bytes vector.
					Bytes.push_back(std::strtoul(CurrentByte, &CurrentByte, 16));
				}
			}
			return Bytes;
		};

		std::uint8_t* ScanBytes = reinterpret_cast<std::uint8_t*>(ptr); // Get the base of the module.

		const std::vector<int> PatternBytes = PatternToBytes(pattern); // Convert our pattern to a byte array.
		const std::pair BytesInfo = std::make_pair(PatternBytes.size(), PatternBytes.data()); // Get the size and data of our bytes.
		int occurences = 0;

		for (auto i = 01; i < opCodesToScan + BytesInfo.first; ++i)
		{
			bool FoundAddress = true;

			int memOffset = searchDirect == Direction::UP ? -i : i;

			for (DWORD j = 0ul; j < BytesInfo.first; ++j)
			{
				// If either the current byte equals to the byte in our pattern or our current byte in the pattern is a wildcard
				// our if clause will be false.
				if (ScanBytes[memOffset + j] != BytesInfo.second[j] && BytesInfo.second[j] != -1)
				{
					FoundAddress = false;
					break;
				}
			}

			if (FoundAddress)
			{
				occurences++;
				if (occurence == occurences)
				{
					return MemoryAddress(&ScanBytes[memOffset]);
				}
			}

		}

		return MemoryAddress();
	}

	MemoryAddress FollowNearCall(std::ptrdiff_t opcodeOffset = 0x1, std::ptrdiff_t nextInstructionOffset = 0x5)
	{
		// Skip E9 opcode.
		std::uintptr_t skipOpCode = ptr + opcodeOffset;

		// Get 4-byte long relative address.
		std::int32_t relativeAddress = *reinterpret_cast<std::int32_t*>(skipOpCode);

		// Get location of next instruction.
		std::uintptr_t nextInstruction = ptr + nextInstructionOffset;

		// Get function location via adding relative address to next instruction.
		return MemoryAddress(nextInstruction + relativeAddress);
	}

	MemoryAddress FollowNearCallSelf(std::ptrdiff_t opcodeOffset = 0x1, std::ptrdiff_t nextInstructionOffset = 0x5)
	{
		// Skip E9 opcode.
		std::uintptr_t skipOpCode = ptr + opcodeOffset;

		// Get 4-byte long relative address.
		std::int32_t relativeAddress = *reinterpret_cast<std::int32_t*>(skipOpCode);

		// Get location of next instruction.
		std::uintptr_t nextInstruction = ptr + nextInstructionOffset;

		// Get function location via adding relative address to next instruction.
		ptr = nextInstruction + relativeAddress;
		return *this;
	}
	
private:
	std::uintptr_t ptr = 0;
};

class Module
{
public:
	Module() = default;
	Module(std::string moduleName) : moduleName(moduleName)
	{
		const MODULEINFO mInfo = GetModuleInfo(moduleName.c_str()); // Get module info.
		sizeOfModule = (DWORD64)mInfo.SizeOfImage; // Grab the module size.
		moduleBase = (std::uintptr_t)mInfo.lpBaseOfDll; // Grab module base.
	}

	MemoryAddress PatternSearch(const char* signature)
	{
		static auto PatternToBytes = [](const char* pattern)
		{
			char* PatternStart = const_cast<char*>(pattern); // Cast const away and get start of pattern.
			char* PatternEnd = PatternStart + std::strlen(pattern); // Get end of pattern.

			std::vector<std::int32_t> Bytes = std::vector<std::int32_t>{ }; // Initialize byte vector.

			for (char* CurrentByte = PatternStart; CurrentByte < PatternEnd; ++CurrentByte)
			{
				if (*CurrentByte == '?') // Is current char(byte) a wildcard?
				{
					++CurrentByte; // Skip 1 character.

					if (*CurrentByte == '?') // Is it a double wildcard pattern?
						++CurrentByte; // If so skip the next space that will come up so we can reach the next byte.

					Bytes.push_back(-1); // Push the byte back as invalid.
				}
				else
				{
					// https://stackoverflow.com/a/43860875/12541255
					// Here we convert our string to a unsigned long integer. We pass our string then we use 16 as the base because we want it as hexadecimal.
					// Afterwards we push the byte into our bytes vector.
					Bytes.push_back(std::strtoul(CurrentByte, &CurrentByte, 16));
				}
			}
			return Bytes;
		};

		std::uint8_t* ScanBytes = reinterpret_cast<std::uint8_t*>(moduleBase); // Get the base of the module.

		const std::vector<int> PatternBytes = PatternToBytes(signature); // Convert our pattern to a byte array.
		const std::pair BytesInfo = std::make_pair(PatternBytes.size(), PatternBytes.data()); // Get the size and data of our bytes.

		for (DWORD i = 0ul; i < sizeOfModule - BytesInfo.first; ++i)
		{
			bool FoundAddress = true;

			for (DWORD j = 0ul; j < BytesInfo.first; ++j)
			{
				// If either the current byte equals to the byte in our pattern or our current byte in the pattern is a wildcard
				// our if clause will be false.
				if (ScanBytes[i + j] != BytesInfo.second[j] && BytesInfo.second[j] != -1)
				{
					FoundAddress = false;
					break;
				}
			}

			if (FoundAddress)
			{
				return MemoryAddress(&ScanBytes[i]);
			}
		}

		return MemoryAddress();
	}

	std::uintptr_t GetModuleBase()
	{
		return moduleBase;
	}

private:
	std::string moduleName = std::string();
	std::uintptr_t moduleBase = 0;
	DWORD64 sizeOfModule = 0;
};