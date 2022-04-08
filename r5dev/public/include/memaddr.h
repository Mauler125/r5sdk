#pragma once
#include "public/include/utility.h"

class ADDRESS
{
public:

	enum class Direction : int
	{
		DOWN = 0,
		UP,
	};

	std::uintptr_t GetPtr()
	{
		return ptr;
	}
	
	ADDRESS() = default;
	ADDRESS(std::uintptr_t ptr) : ptr(ptr) {}
	ADDRESS(void* ptr) : ptr(std::uintptr_t(ptr)) {}

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

	bool operator!= (const ADDRESS& addr) const
	{
		return ptr != addr.ptr;
	}

	bool operator== (const ADDRESS& addr) const
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

	ADDRESS Offset(std::ptrdiff_t offset)
	{
		return ADDRESS(ptr + offset);
	}

	ADDRESS OffsetSelf(std::ptrdiff_t offset)
	{
		ptr += offset;
		return *this;
	}

	ADDRESS Deref(int deref = 1)
	{
		std::uintptr_t reference = ptr;

		while (deref--)
		{
			if (reference)
				reference = *reinterpret_cast<std::uintptr_t*>(reference);
		}

		return ADDRESS(reference);
	}

	ADDRESS DerefSelf(int deref = 1)
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
			byteAtCurrentAddress = *reinterpret_cast<std::uint8_t*>(reference); // Get byte at current Address.

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
			*(std::uint8_t*)(ptr + i) = opcodes[i]; // Write opcodes to Address.
		}

		dwSize = opcodes.size();
		VirtualProtect((void*)ptr, dwSize, oldProt, &oldProt); // Restore protection.
	}

	void PatchString(const std::string string)
	{
		DWORD oldProt = NULL;

		SIZE_T dwSize = string.size();
		std::vector<char> bytes(string.begin(), string.end());

		VirtualProtect((void*)ptr, dwSize, PAGE_EXECUTE_READWRITE, &oldProt); // Patch page to be able to read and write to it.

		for (int i = 0; i < string.size(); i++)
		{
			*(std::uint8_t*)(ptr + i) = bytes[i]; // Write string to Address.
		}

		dwSize = string.size();
		VirtualProtect((void*)ptr, dwSize, oldProt, &oldProt); // Restore protection.
	}

	ADDRESS FindPatternSelf(const std::string pattern, const Direction searchDirect, const int opCodesToScan = 100, const std::ptrdiff_t occurence = 1)
	{
		static auto PatternToBytes = [](const std::string pattern)
		{
			char* PatternStart = const_cast<char*>(pattern.c_str()); // Cast const away and get start of pattern.
			char* PatternEnd = PatternStart + std::strlen(pattern.c_str()); // Get end of pattern.

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
		std::ptrdiff_t occurences = 0;

		for (long i = 01; i < opCodesToScan + BytesInfo.first; i++)
		{
			bool FoundAddress = true;

			int memOffset = searchDirect == Direction::DOWN ? i : -i;
			
			for (DWORD j = 0ul; j < BytesInfo.first; j++)
			{
				// If either the current byte equals to the byte in our pattern or our current byte in the pattern is a wildcard
				// our if clause will be false.
				std::uint8_t currentByte = *(ScanBytes + memOffset + j);
				if (currentByte != BytesInfo.second[j] && BytesInfo.second[j] != -1)
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
					ptr = std::uintptr_t(&*(ScanBytes + memOffset));
					return *this;
				}
			}

		}

		ptr = std::uintptr_t();
		return *this;
	}

	ADDRESS FindPattern(const std::string pattern, const Direction searchDirect, const int opCodesToScan = 100, const std::ptrdiff_t occurence = 1)
	{
		static auto PatternToBytes = [](const std::string pattern)
		{
			char* PatternStart = const_cast<char*>(pattern.c_str()); // Cast const away and get start of pattern.
			char* PatternEnd = PatternStart + std::strlen(pattern.c_str()); // Get end of pattern.

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
		std::ptrdiff_t occurences = 0;

		for (long i = 01; i < opCodesToScan + BytesInfo.first; i++)
		{
			bool FoundAddress = true;

			int memOffset = searchDirect == Direction::DOWN ? i : -i;

			for (DWORD j = 0ul; j < BytesInfo.first; j++)
			{
				// If either the current byte equals to the byte in our pattern or our current byte in the pattern is a wildcard
				// our if clause will be false.
				std::uint8_t currentByte = *(ScanBytes + memOffset + j);
				if (currentByte != BytesInfo.second[j] && BytesInfo.second[j] != -1)
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
					return ADDRESS(&*(ScanBytes + memOffset));
				}
			}
		}

		return ADDRESS();
	}

	ADDRESS FollowNearCall(std::ptrdiff_t opcodeOffset = 0x1, std::ptrdiff_t nextInstructionOffset = 0x5)
	{
		return ResolveRelativeAddress(opcodeOffset, nextInstructionOffset);
	}

	ADDRESS FollowNearCallSelf(std::ptrdiff_t opcodeOffset = 0x1, std::ptrdiff_t nextInstructionOffset = 0x5)
	{
		return ResolveRelativeAddressSelf(opcodeOffset, nextInstructionOffset);
	}

	ADDRESS ResolveRelativeAddressSelf(std::ptrdiff_t registerOffset = 0x0, std::ptrdiff_t nextInstructionOffset = 0x4)
	{
		// Skip register.
		std::uintptr_t skipRegister = ptr + registerOffset;

		// Get 4-byte long relative Address.
		std::int32_t relativeAddress = *reinterpret_cast<std::int32_t*>(skipRegister);

		// Get location of next instruction.
		std::uintptr_t nextInstruction = ptr + nextInstructionOffset;

		// Get function location via adding relative Address to next instruction.
		ptr = nextInstruction + relativeAddress;
		return *this;
	}

	ADDRESS ResolveRelativeAddress(std::ptrdiff_t registerOffset = 0x0, std::ptrdiff_t nextInstructionOffset = 0x4)
	{
		// Skip register.
		std::uintptr_t skipRegister = ptr + registerOffset;

		// Get 4-byte long relative Address.
		std::int32_t relativeAddress = *reinterpret_cast<std::int32_t*>(skipRegister);

		// Get location of next instruction.
		std::uintptr_t nextInstruction = ptr + nextInstructionOffset;

		// Get function location via adding relative Address to next instruction.
		return ADDRESS(nextInstruction + relativeAddress);
	}

private:
	std::uintptr_t ptr = 0;
};

class MODULE
{
public:

	struct ModuleSections
	{
		ModuleSections() = default;
		ModuleSections(std::string sectionName, std::uintptr_t sectionStartAddress, DWORD sectionSize) : sectionName(sectionName), sectionStartAddress(sectionStartAddress), sectionSize(sectionSize) {}

		bool IsSectionValid()
		{
			return sectionSize != 0;
		}

		std::string sectionName = std::string(); // Name of section.
		std::uintptr_t sectionStartAddress = 0; // Start memory Address of section.
		DWORD sectionSize = 0; // Size of section.
	};

	ModuleSections GetSectionByName(const std::string sectionName)
	{
		for (ModuleSections& currentSection : moduleSections)
		{
			if (currentSection.sectionName.compare(sectionName) == 0)
				return currentSection;
		}

		return ModuleSections();
	}

	MODULE() = default;
	MODULE(std::string moduleName) : moduleName(moduleName)
	{
		const MODULEINFO mInfo = GetModuleInfo(moduleName.c_str()); // Get module info.
		sizeOfModule = (DWORD64)mInfo.SizeOfImage; // Grab the module size.
		moduleBase = (std::uintptr_t)mInfo.lpBaseOfDll; // Grab module base.

		dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(moduleBase); // Get dosHeader.
		ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS64*>(moduleBase + dosHeader->e_lfanew); // Get ntHeaders.

		const IMAGE_SECTION_HEADER* hSection = IMAGE_FIRST_SECTION(ntHeaders); // Get first image section.

		for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) // Loop through the sections.
		{
			const IMAGE_SECTION_HEADER& currentSection = hSection[i]; // Get current section.
			moduleSections.push_back(ModuleSections(std::string(reinterpret_cast<const char*>(currentSection.Name)), (std::uintptr_t)(DWORD64)(moduleBase + currentSection.VirtualAddress), currentSection.SizeOfRawData)); // Push back a struct with the section data.
		}
	}

	ADDRESS PatternSearch(const std::string pattern, const std::ptrdiff_t patternOccurence = 1)
	{
		static auto PatternToBytes = [](const std::string pattern)
		{
			char* PatternStart = const_cast<char*>(pattern.c_str()); // Cast const away and get start of pattern.
			char* PatternEnd = PatternStart + std::strlen(pattern.c_str()); // Get end of pattern.

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

		ModuleSections textSection = GetSectionByName(".text"); // Get the .text section.
		if (!textSection.IsSectionValid())
			return ADDRESS();

		const std::vector<std::int32_t> PatternBytes = PatternToBytes(pattern); // Convert our pattern to a byte array.
		const std::pair BytesInfo = std::make_pair(PatternBytes.size(), PatternBytes.data()); // Get the size and data of our bytes.

		std::uint8_t* latestOccurence = nullptr;
		std::ptrdiff_t occurencesFound = 0;

		std::uint8_t* StartOfCodeSection = reinterpret_cast<std::uint8_t*>(textSection.sectionStartAddress); // Get start of .text section.

		for (DWORD i = 0ul; i < textSection.sectionSize - BytesInfo.first; i++)
		{
			bool FoundAddress = true;

			for (DWORD j = 0ul; j < BytesInfo.first; j++)
			{
				// If either the current byte equals to the byte in our pattern or our current byte in the pattern is a wildcard
				// our if clause will be false.
				if (StartOfCodeSection[i + j] != BytesInfo.second[j] && BytesInfo.second[j] != -1)
				{
					FoundAddress = false;
					break;
				}
			}

			if (FoundAddress)
			{
				occurencesFound++; // Increment occurences found counter.
				if (patternOccurence == occurencesFound) // Is it the occurence we want?
					return ADDRESS(&StartOfCodeSection[i]); // If yes return it.

				latestOccurence = &StartOfCodeSection[i]; // Stash latest occurence.
			}
		}

		return ADDRESS(latestOccurence);
	}

	ADDRESS FindPatternSIMD(const std::uint8_t* szPattern, const char* szMask)
	{
		ModuleSections mInfo = GetSectionByName(".text"); // Get the .text section.
		if (!mInfo.IsSectionValid())
		{
			return ADDRESS();
		}

		uint64_t nBase = static_cast<uint64_t>(mInfo.sectionStartAddress);
		uint64_t nSize = static_cast<uint64_t>(mInfo.sectionSize);

		const uint8_t* pData = reinterpret_cast<uint8_t*>(nBase);
		const uint8_t* pEnd = pData + static_cast<uint32_t>(nSize) - strlen(szMask);

		int masks[64]; // 64*16 = enough masks for 1024 bytes.
		int num_masks = static_cast<int>(ceil(static_cast<float>(strlen(szMask)) / 16.f));

		memset(masks, '\0', num_masks * sizeof(int));
		for (int64_t i = 0; i < num_masks; ++i)
		{
			for (int64_t j = strnlen(szMask + i * 16, 16) - 1; j >= 0; --j)
			{
				if (szMask[i * 16 + j] == 'x')
				{
					masks[i] |= 1 << j;
				}
			}
		}
		__m128i xmm1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(szPattern));
		__m128i xmm2, xmm3, msks;
		for (; pData != pEnd; _mm_prefetch(reinterpret_cast<const char*>(++pData + 64), _MM_HINT_NTA))
		{
			if (szPattern[0] == pData[0])
			{
				xmm2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pData));
				msks = _mm_cmpeq_epi8(xmm1, xmm2);
				if ((_mm_movemask_epi8(msks) & masks[0]) == masks[0])
				{
					for (DWORD64 i = 1; i < num_masks; ++i)
					{
						xmm2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>((pData + i * 16)));
						xmm3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>((szPattern + i * 16)));
						msks = _mm_cmpeq_epi8(xmm2, xmm3);
						if ((_mm_movemask_epi8(msks) & masks[i]) == masks[i])
						{
							if ((i + 1) == num_masks)
							{
								return static_cast<ADDRESS>(const_cast<uint8_t*>(pData));
							}
						}
						else
						{
							goto cont;
						}
					}
					return static_cast<ADDRESS>((&*(const_cast<uint8_t*>(pData))));
				}
			}cont:;
		}
		return ADDRESS();
	}

	ADDRESS GetExportedFunction(const std::string functionName)
	{
		if (!dosHeader || dosHeader->e_magic != IMAGE_DOS_SIGNATURE) // Is dosHeader valid?
			return ADDRESS();

		if (!ntHeaders || ntHeaders->Signature != IMAGE_NT_SIGNATURE) // Is ntHeader valid?
			return ADDRESS();

		// Get the location of IMAGE_EXPORT_DIRECTORY for this module by adding the IMAGE_DIRECTORY_ENTRY_EXPORT relative virtual Address onto our module base Address.
		IMAGE_EXPORT_DIRECTORY* ImageExportDirectory = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(moduleBase + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		if (!ImageExportDirectory)
			return ADDRESS();

		// Are there any exported functions?
		if (!ImageExportDirectory->NumberOfFunctions)
			return ADDRESS();

		// Get the location of the functions via adding the relative virtual Address from the struct into our module base Address.
		DWORD* AddressOfFunctionsPtr = reinterpret_cast<DWORD*>(moduleBase + ImageExportDirectory->AddressOfFunctions);
		if (!AddressOfFunctionsPtr)
			return ADDRESS();

		// Get the names of the functions via adding the relative virtual Address from the struct into our module base Address.
		DWORD* AddressOfNamePtr = reinterpret_cast<DWORD*>(moduleBase + ImageExportDirectory->AddressOfNames);
		if (!AddressOfNamePtr)
			return ADDRESS();

		// Get the ordinals of the functions via adding the relative virtual Address from the struct into our module base Address.
		DWORD* AddressOfOrdinalsPtr = reinterpret_cast<DWORD*>(moduleBase + ImageExportDirectory->AddressOfNameOrdinals);
		if (!AddressOfOrdinalsPtr)
			return ADDRESS();

		for (std::size_t i = 0; i < ImageExportDirectory->NumberOfFunctions; i++) // Iterate through all the functions.
		{
			// Get virtual relative Address of the function name. Then add module base Address to get the actual location.
			std::string ExportFunctionName = reinterpret_cast<char*>(reinterpret_cast<DWORD*>(moduleBase + AddressOfNamePtr[i]));

			if (ExportFunctionName.compare(functionName) == 0) // Is this our wanted exported function?
			{
				// Get the function ordinal. Then grab the relative virtual Address of our wanted function. Then add module base Address so we get the actual location.
				return ADDRESS(moduleBase + AddressOfFunctionsPtr[reinterpret_cast<WORD*>(AddressOfOrdinalsPtr)[i]]); // Return as Address class.
			}
		}
		return ADDRESS();
	}
	
	ADDRESS FindAddressForString(const std::string string, bool nullTerminator)
	{
		static auto StringToBytes = [](const std::string string, bool nullTerminator)
		{
			char* StringStart = const_cast<char*>(string.c_str()); // Cast const away and get start of string.
			char* StringEnd = StringStart + std::strlen(string.c_str()); // Get end of string.

			std::vector<std::int32_t> Bytes = std::vector<std::int32_t>{ }; // Initialize byte vector.

			for (char* CurrentByte = StringStart; CurrentByte < StringEnd; ++CurrentByte) // Loop through all the characters in the .rdata string.
			{
				Bytes.push_back(*CurrentByte); // Dereference character and push back the byte.
			}

			if (nullTerminator) // Does the string have a null terminator at the end of it?
				Bytes.push_back(0x0); // If yes push back 0 at the end of the byte array.

			return Bytes;
		};

		ModuleSections rdataSection = GetSectionByName(".rdata"); // .Get rdata section, we only loop through here because most important strings are in the .rdata section.
		if (!rdataSection.IsSectionValid())
			return ADDRESS();

		std::vector<std::int32_t> stringBytes = StringToBytes(string, nullTerminator); // Convert our string to a byte array.
		const std::pair BytesInfo = std::make_pair(stringBytes.size(), stringBytes.data()); // Get the size and data of our bytes.

		std::uint8_t* StartOfRdata = reinterpret_cast<std::uint8_t*>(rdataSection.sectionStartAddress); // Get start of .rdata section.

		for (DWORD i = 0ul; i < rdataSection.sectionSize - BytesInfo.first; i++)
		{
			bool FoundAddress = true;

			// If either the current byte equals to the byte in our pattern or our current byte in the pattern is a wildcard
			// our if clause will be false.
			for (DWORD j = 0ul; j < BytesInfo.first; j++)
			{
				if (StartOfRdata[i + j] != BytesInfo.second[j] && BytesInfo.second[j] != -1)
				{
					FoundAddress = false;
					break;
				}
			}

			if (FoundAddress)
			{
				return ADDRESS(&StartOfRdata[i]);
			}
		}

		return ADDRESS();
	}

	ADDRESS StringSearch(const std::string string, const std::ptrdiff_t occurence = 1, bool nullTerminator = false)
	{
		static auto PatternToBytes = [](const std::string pattern)
		{
			char* PatternStart = const_cast<char*>(pattern.c_str()); // Cast const away and get start of pattern.
			char* PatternEnd = PatternStart + std::strlen(pattern.c_str()); // Get end of pattern.

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

		ModuleSections textSection = GetSectionByName(".text"); // Get the .text section.
		if (!textSection.IsSectionValid())
			return ADDRESS();

		ADDRESS stringAddress = FindAddressForString(string, nullTerminator); // Get Address for the string in the .rdata section.
		if (!stringAddress)
			return ADDRESS();

		std::uint8_t* latestOccurence = nullptr;
		std::ptrdiff_t occurencesFound = 0;

		std::uint8_t* StartOfCodeSection = reinterpret_cast<std::uint8_t*>(textSection.sectionStartAddress); // Get the start of the .text section.

		for (DWORD i = 0ul; i < textSection.sectionSize - 0x5; i++)
		{
			byte byte = StartOfCodeSection[i];
			if (byte == 0x8D) // is it a LEA instruction?
			{
				ADDRESS skipOpCode = ADDRESS((std::uintptr_t)&StartOfCodeSection[i]).OffsetSelf(0x2); // Skip next 2 opcodes, those being the instruction and then the register.

				std::int32_t relativeAddress = skipOpCode.GetValue<std::int32_t>(); // Get 4-byte long string relative Address

				std::uintptr_t nextInstruction = skipOpCode.Offset(0x4).GetPtr(); // Get location of next instruction.

				ADDRESS potentialLocation = ADDRESS(nextInstruction + relativeAddress); // Get potential string location.

				if (potentialLocation == stringAddress)
				{
					occurencesFound++; // Increment occurences found counter.
					if (occurence == occurencesFound) // Is it the occurence we want?
						return ADDRESS(&StartOfCodeSection[i]); // If yes return it.

					latestOccurence = &StartOfCodeSection[i]; // Stash latest occurence.
				}
			}
		}
		return ADDRESS(latestOccurence);
	}

	std::uintptr_t GetModuleBase()
	{
		return moduleBase;
	}

	std::string GetModuleName()
	{
		return moduleName;
	}

private:
	std::string moduleName = std::string();
	std::uintptr_t moduleBase = 0;
	DWORD64 sizeOfModule = 0;
	IMAGE_NT_HEADERS64* ntHeaders = nullptr;
	IMAGE_DOS_HEADER* dosHeader = nullptr;
	std::vector<ModuleSections> moduleSections = {};
};