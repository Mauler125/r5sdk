#pragma once

class MemoryAddress
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

	MemoryAddress FindPatternSelf(const std::string pattern, const Direction searchDirect, const int opCodesToScan = 100, const std::ptrdiff_t occurence = 1)
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

	MemoryAddress FindPattern(const std::string pattern, const Direction searchDirect, const int opCodesToScan = 100, const std::ptrdiff_t occurence = 1)
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
					return MemoryAddress(&*(ScanBytes + memOffset));
				}
			}
		}

		return MemoryAddress();
	}

	MemoryAddress FollowNearCall(std::ptrdiff_t opcodeOffset = 0x1, std::ptrdiff_t nextInstructionOffset = 0x5)
	{
		return ResolveRelativeAddress(opcodeOffset, nextInstructionOffset);
	}

	MemoryAddress FollowNearCallSelf(std::ptrdiff_t opcodeOffset = 0x1, std::ptrdiff_t nextInstructionOffset = 0x5)
	{
		return ResolveRelativeAddressSelf(opcodeOffset, nextInstructionOffset);
	}

	MemoryAddress ResolveRelativeAddressSelf(std::ptrdiff_t registerOffset = 0x1, std::ptrdiff_t nextInstructionOffset = 0x4)
	{
		// Skip register.
		std::uintptr_t skipRegister = ptr + registerOffset;

		// Get 4-byte long relative address.
		std::int32_t relativeAddress = *reinterpret_cast<std::int32_t*>(skipRegister);

		// Get location of next instruction.
		std::uintptr_t nextInstruction = ptr + nextInstructionOffset;

		// Get function location via adding relative address to next instruction.
		ptr = nextInstruction + relativeAddress;
		return *this;
	}

	MemoryAddress ResolveRelativeAddress(std::ptrdiff_t registerOffset = 0x1, std::ptrdiff_t nextInstructionOffset = 0x4)
	{
		// Skip register.
		std::uintptr_t skipRegister = ptr + registerOffset;

		// Get 4-byte long relative address.
		std::int32_t relativeAddress = *reinterpret_cast<std::int32_t*>(skipRegister);

		// Get location of next instruction.
		std::uintptr_t nextInstruction = ptr + nextInstructionOffset;

		// Get function location via adding relative address to next instruction.
		return MemoryAddress(nextInstruction + relativeAddress);
	}

private:
	std::uintptr_t ptr = 0;
};

class Module
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
		std::uintptr_t sectionStartAddress = 0; // Start memory address of section.
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

	Module() = default;
	Module(std::string moduleName) : moduleName(moduleName)
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

	MemoryAddress PatternSearch(const std::string pattern, const std::ptrdiff_t patternOccurence = 1)
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
			return MemoryAddress();

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
					return MemoryAddress(&StartOfCodeSection[i]); // If yes return it.

				latestOccurence = &StartOfCodeSection[i]; // Stash latest occurence.
			}
		}

		return MemoryAddress(latestOccurence);
	}

	MemoryAddress GetExportedFunction(const std::string functionName)
	{
		if (!dosHeader || dosHeader->e_magic != IMAGE_DOS_SIGNATURE) // Is dosHeader valid?
			return MemoryAddress();

		if (!ntHeaders || ntHeaders->Signature != IMAGE_NT_SIGNATURE) // Is ntHeader valid?
			return MemoryAddress();

		// Get the location of IMAGE_EXPORT_DIRECTORY for this module by adding the IMAGE_DIRECTORY_ENTRY_EXPORT relative virtual address onto our module base address.
		IMAGE_EXPORT_DIRECTORY* ImageExportDirectory = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(moduleBase + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		if (!ImageExportDirectory)
			return MemoryAddress();

		// Are there any exported functions?
		if (!ImageExportDirectory->NumberOfFunctions)
			return MemoryAddress();

		// Get the location of the functions via adding the relative virtual address from the struct into our module base address.
		DWORD* AddressOfFunctionsPtr = reinterpret_cast<DWORD*>(moduleBase + ImageExportDirectory->AddressOfFunctions);
		if (!AddressOfFunctionsPtr)
			return MemoryAddress();

		// Get the names of the functions via adding the relative virtual address from the struct into our module base address.
		DWORD* AddressOfNamePtr = reinterpret_cast<DWORD*>(moduleBase + ImageExportDirectory->AddressOfNames);
		if (!AddressOfNamePtr)
			return MemoryAddress();

		// Get the ordinals of the functions via adding the relative virtual address from the struct into our module base address.
		DWORD* AddressOfOrdinalsPtr = reinterpret_cast<DWORD*>(moduleBase + ImageExportDirectory->AddressOfNameOrdinals);
		if (!AddressOfOrdinalsPtr)
			return MemoryAddress();

		for (std::size_t i = 0; i < ImageExportDirectory->NumberOfFunctions; i++) // Iterate through all the functions.
		{
			// Get virtual relative address of the function name. Then add module base address to get the actual location.
			std::string ExportFunctionName = reinterpret_cast<char*>(reinterpret_cast<DWORD*>(moduleBase + AddressOfNamePtr[i]));

			if (ExportFunctionName.compare(functionName) == 0) // Is this our wanted exported function?
			{
				// Get the function ordinal. Then grab the relative virtual address of our wanted function. Then add module base address so we get the actual location.
				return MemoryAddress(moduleBase + AddressOfFunctionsPtr[reinterpret_cast<WORD*>(AddressOfOrdinalsPtr)[i]]); // Return as MemoryAddress class.
			}
		}
		return MemoryAddress();
	}
	
	MemoryAddress FindAddressForString(const std::string string, bool nullTerminator)
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
			return MemoryAddress();

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
				return MemoryAddress(&StartOfRdata[i]);
			}
		}

		return MemoryAddress();
	}

	MemoryAddress StringSearch(const std::string string, const std::ptrdiff_t occurence = 1, bool nullTerminator = false)
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
			return MemoryAddress();

		MemoryAddress stringAddress = FindAddressForString(string, nullTerminator); // Get address for the string in the .rdata section.
		if (!stringAddress)
			return MemoryAddress();

		std::uint8_t* latestOccurence = nullptr;
		std::ptrdiff_t occurencesFound = 0;

		std::uint8_t* StartOfCodeSection = reinterpret_cast<std::uint8_t*>(textSection.sectionStartAddress); // Get the start of the .text section.

		for (DWORD i = 0ul; i < textSection.sectionSize - 0x5; i++)
		{
			byte byte = StartOfCodeSection[i];
			if (byte == 0x8D) // is it a LEA instruction?
			{
				MemoryAddress skipOpCode = MemoryAddress((std::uintptr_t)&StartOfCodeSection[i]).OffsetSelf(0x2); // Skip next 2 opcodes, those being the instruction and then the register.

				std::int32_t relativeAddress = skipOpCode.GetValue<std::int32_t>(); // Get 4-byte long string relative address

				std::uintptr_t nextInstruction = skipOpCode.Offset(0x4).GetPtr(); // Get location of next instruction.

				MemoryAddress potentialLocation = MemoryAddress(nextInstruction + relativeAddress); // Get potential string location.

				if (potentialLocation == stringAddress)
				{
					occurencesFound++; // Increment occurences found counter.
					if (occurence == occurencesFound) // Is it the occurence we want?
						return MemoryAddress(&StartOfCodeSection[i]); // If yes return it.

					latestOccurence = &StartOfCodeSection[i]; // Stash latest occurence.
				}
			}
		}
		return MemoryAddress(latestOccurence);
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