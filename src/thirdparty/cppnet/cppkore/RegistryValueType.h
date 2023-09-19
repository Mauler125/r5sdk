#pragma once

namespace Win32
{
	// Represents the possible value types in the Registry
	enum class RegistryValueType
	{
		None,
		String,
		Binary,
		Dword,
		DwordBigEndian,
		SymbolicLink,
		MultiString,
		ResourceList,
		FullResourceDescriptor,
		ResourceRequirementsList,
		Qword,
	};

	// Represents a raw buffer in the registry
	struct RegistryBinaryBlob
	{
		// The data buffer
		std::unique_ptr<uint8_t[]> Buffer;
		// The size of the buffer
		uint64_t Size;
	};
}