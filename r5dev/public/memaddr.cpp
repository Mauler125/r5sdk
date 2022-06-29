//===========================================================================//
//
// Purpose: Implementation of the CMemory class.
//
//===========================================================================//
#include "core/stdafx.h"
#include "public/include/utility.h"
#include "public/include/memaddr.h"

//-----------------------------------------------------------------------------
// Purpose: check array of opcodes starting from current address
// Input  : vOpcodeArray - 
// Output : true if equal, false otherwise
//-----------------------------------------------------------------------------
bool CMemory::CheckOpCodes(const vector<uint8_t> vOpcodeArray) const
{
	uintptr_t ref = ptr;

	// Loop forward in the ptr class member.
	for (auto [byteAtCurrentAddress, i] = std::tuple{ uint8_t(), (size_t)0 }; i < vOpcodeArray.size(); i++, ref++)
	{
		byteAtCurrentAddress = *reinterpret_cast<uint8_t*>(ref);

		// If byte at ptr doesn't equal in the byte array return false.
		if (byteAtCurrentAddress != vOpcodeArray[i])
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: patch array of opcodes starting from current address
// Input  : vOpcodeArray - 
//-----------------------------------------------------------------------------
void CMemory::Patch(const vector<uint8_t> vOpcodeArray) const
{
	DWORD oldProt = NULL;

	SIZE_T dwSize = vOpcodeArray.size();
	VirtualProtect(reinterpret_cast<void*>(ptr), dwSize, PAGE_EXECUTE_READWRITE, &oldProt); // Patch page to be able to read and write to it.

	for (int i = 0; i < vOpcodeArray.size(); i++)
	{
		*reinterpret_cast<uint8_t*>(ptr + i) = vOpcodeArray[i]; // Write opcodes to Address.
	}

	dwSize = vOpcodeArray.size();
	VirtualProtect(reinterpret_cast<void*>(ptr), dwSize, oldProt, &oldProt); // Restore protection.
}

//-----------------------------------------------------------------------------
// Purpose: patch string constant at current address
// Input  : &svString - 
//-----------------------------------------------------------------------------
void CMemory::PatchString(const string& svString) const
{
	DWORD oldProt = NULL;

	SIZE_T dwSize = svString.size();
	vector<char> bytes(svString.begin(), svString.end());

	VirtualProtect(reinterpret_cast<void*>(ptr), dwSize, PAGE_EXECUTE_READWRITE, &oldProt); // Patch page to be able to read and write to it.

	for (int i = 0; i < svString.size(); i++)
	{
		*reinterpret_cast<uint8_t*>(ptr + i) = bytes[i]; // Write string to Address.
	}

	dwSize = svString.size();
	VirtualProtect(reinterpret_cast<void*>(ptr), dwSize, oldProt, &oldProt); // Restore protection.
}

//-----------------------------------------------------------------------------
// Purpose: find array of bytes in process memory
// Input  : *szPattern - 
//			searchDirect - 
//			opCodesToScan - 
//			occurence - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CMemory::FindPattern(const string& svPattern, const Direction searchDirect, const int opCodesToScan, const ptrdiff_t occurence) const
{
	uint8_t* pScanBytes = reinterpret_cast<uint8_t*>(ptr); // Get the base of the module.

	const vector<int> PatternBytes = PatternToBytes(svPattern); // Convert our pattern to a byte array.
	const pair bytesInfo = std::make_pair(PatternBytes.size(), PatternBytes.data()); // Get the size and data of our bytes.
	ptrdiff_t occurences = 0;

	for (long i = 01; i < opCodesToScan + bytesInfo.first; i++)
	{
		bool bFound = true;
		int nMemOffset = searchDirect == Direction::DOWN ? i : -i;

		for (DWORD j = 0ul; j < bytesInfo.first; j++)
		{
			// If either the current byte equals to the byte in our pattern or our current byte in the pattern is a wildcard
			// our if clause will be false.
			uint8_t currentByte = *(pScanBytes + nMemOffset + j);
			_mm_prefetch(reinterpret_cast<const char*>(currentByte + nMemOffset + 64), _MM_HINT_T0); // precache some data in L1.
			if (currentByte != bytesInfo.second[j] && bytesInfo.second[j] != -1)
			{
				bFound = false;
				break;
			}
		}

		if (bFound)
		{
			occurences++;
			if (occurence == occurences)
			{
				return CMemory(&*(pScanBytes + nMemOffset));
			}
		}
	}

	return CMemory();
}

//-----------------------------------------------------------------------------
// Purpose: find array of bytes in process memory starting from current address
// Input  : *szPattern - 
//			searchDirect - 
//			opCodesToScan - 
//			occurence - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CMemory::FindPatternSelf(const string& svPattern, const Direction searchDirect, const int opCodesToScan, const ptrdiff_t occurence)
{
	uint8_t* pScanBytes = reinterpret_cast<uint8_t*>(ptr); // Get the base of the module.

	const vector<int> PatternBytes = PatternToBytes(svPattern); // Convert our pattern to a byte array.
	const pair bytesInfo = std::make_pair(PatternBytes.size(), PatternBytes.data()); // Get the size and data of our bytes.
	ptrdiff_t occurences = 0;

	for (long i = 01; i < opCodesToScan + bytesInfo.first; i++)
	{
		bool bFound = true;
		int nMemOffset = searchDirect == Direction::DOWN ? i : -i;

		for (DWORD j = 0ul; j < bytesInfo.first; j++)
		{
			// If either the current byte equals to the byte in our pattern or our current byte in the pattern is a wildcard
			// our if clause will be false.
			uint8_t currentByte = *(pScanBytes + nMemOffset + j);
			_mm_prefetch(reinterpret_cast<const char*>(currentByte + nMemOffset + 64), _MM_HINT_T0); // precache some data in L1.
			if (currentByte != bytesInfo.second[j] && bytesInfo.second[j] != -1)
			{
				bFound = false;
				break;
			}
		}

		if (bFound)
		{
			occurences++;
			if (occurence == occurences)
			{
				ptr = uintptr_t(&*(pScanBytes + nMemOffset));
				return *this;
			}
		}
	}

	ptr = uintptr_t();
	return *this;
}

//-----------------------------------------------------------------------------
// Purpose: ResolveRelativeAddress wrapper
// Input  : opcodeOffset - 
//			nextInstructionOffset - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CMemory::FollowNearCall(const ptrdiff_t opcodeOffset, const ptrdiff_t nextInstructionOffset) const
{
	return ResolveRelativeAddress(opcodeOffset, nextInstructionOffset);
}

//-----------------------------------------------------------------------------
// Purpose: ResolveRelativeAddressSelf wrapper
// Input  : opcodeOffset - 
//			nextInstructionOffset - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CMemory::FollowNearCallSelf(const ptrdiff_t opcodeOffset, const ptrdiff_t nextInstructionOffset)
{
	return ResolveRelativeAddressSelf(opcodeOffset, nextInstructionOffset);
}

//-----------------------------------------------------------------------------
// Purpose: resolves the relative pointer to offset
// Input  : registerOffset - 
//			nextInstructionOffset - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CMemory::ResolveRelativeAddress(const ptrdiff_t registerOffset, const ptrdiff_t nextInstructionOffset) const
{
	// Skip register.
	uintptr_t skipRegister = ptr + registerOffset;

	// Get 4-byte long relative Address.
	int32_t relativeAddress = *reinterpret_cast<int32_t*>(skipRegister);

	// Get location of next instruction.
	uintptr_t nextInstruction = ptr + nextInstructionOffset;

	// Get function location via adding relative Address to next instruction.
	return CMemory(nextInstruction + relativeAddress);
}

//-----------------------------------------------------------------------------
// Purpose: resolves the relative pointer to offset from current address
// Input  : registerOffset - 
//			nextInstructionOffset - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CMemory::ResolveRelativeAddressSelf(const ptrdiff_t registerOffset, const ptrdiff_t nextInstructionOffset)
{
	// Skip register.
	uintptr_t skipRegister = ptr + registerOffset;

	// Get 4-byte long relative Address.
	int32_t relativeAddress = *reinterpret_cast<int32_t*>(skipRegister);

	// Get location of next instruction.
	uintptr_t nextInstruction = ptr + nextInstructionOffset;

	// Get function location via adding relative Address to next instruction.
	ptr = nextInstruction + relativeAddress;
	return *this;
}

//-----------------------------------------------------------------------------
// Purpose: patch virtual method to point to a user set function
// Input  : virtualTable - 
//			pHookMethod - 
//          methodIndex -
//          pOriginalMethod -
// Output : void** via pOriginalMethod
//-----------------------------------------------------------------------------
void CMemory::HookVirtualMethod(const uintptr_t virtualTable, const void* pHookMethod, const ptrdiff_t methodIndex, void** ppOriginalMethod)
{
	DWORD oldProt = NULL;

	// Calculate delta to next virtual method.
	uintptr_t virtualMethod = virtualTable + (methodIndex * sizeof(ptrdiff_t));

	// Preserve original function.
	uintptr_t originalFunction = *reinterpret_cast<uintptr_t*>(virtualMethod);

	// Set page for current virtual method to execute n read n write.
	VirtualProtect(reinterpret_cast<void*>(virtualMethod), sizeof(virtualMethod), PAGE_EXECUTE_READWRITE, &oldProt);

	// Set virtual method to our hook.
	*reinterpret_cast<uintptr_t*>(virtualMethod) = reinterpret_cast<uintptr_t>(pHookMethod);

	// Restore original page.
	VirtualProtect(reinterpret_cast<void*>(virtualMethod), sizeof(virtualMethod), oldProt, &oldProt);

	// Move original function into argument.
	*ppOriginalMethod = reinterpret_cast<void*>(originalFunction);
}