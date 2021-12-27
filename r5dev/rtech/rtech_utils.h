#pragma once

#define PAK_HEADER_SIZE   0x80
#define PAK_PARAM_SIZE    0xB0
#define DCMP_BUF_SIZE 0x400000

namespace
{
	/*unk_141313180*/
	// LUT_0 redacted now, split LUT into multiple parts.
#pragma warning( push )
#pragma warning( disable : 4309)
#pragma warning( disable : 4838)
	std::array<int8_t, 512> LUT_0
	{
		4, 254, 252, 8, 4, 239, 17, 249, 4, 253, 252, 7, 4, 5, 255, 244, 4, 254, 252, 16, 4, 239, 17, 246, 4, 253, 252, 251, 4, 6, 255, 11, 4, 254, 252, 8, 4, 239, 17, 248, 4, 253, 252, 12, 4, 5, 255, 247, 4, 254, 252, 16, 4, 239, 17, 245, 4, 253, 252, 250, 4, 6, 255, 243, 4, 254, 252, 8, 4, 239, 17, 249, 4, 253, 252, 7, 4, 5, 255, 244, 4, 254, 252, 16, 4, 239, 17, 246, 4, 253, 252, 251, 4, 6, 255, 14, 4, 254, 252, 8, 4, 239, 17, 248, 4, 253, 252, 12, 4, 5, 255, 9, 4, 254, 252, 16, 4, 239, 17, 245, 4, 253, 252, 250, 4, 6, 255, 241, 4, 254, 252, 8, 4, 239, 17, 249, 4, 253, 252, 7, 4, 5, 255, 244, 4, 254, 252, 16, 4, 239, 17, 246, 4, 253, 252, 251, 4, 6, 255, 13, 4, 254, 252, 8, 4, 239, 17, 248, 4, 253, 252, 12, 4, 5, 255, 247, 4, 254, 252, 16, 4, 239, 17, 245, 4, 253, 252, 250, 4, 6, 255, 242, 4, 254, 252, 8, 4, 239, 17, 249, 4, 253, 252, 7, 4, 5, 255, 244, 4, 254, 252, 16, 4, 239, 17, 246, 4, 253, 252, 251, 4, 6, 255, 15, 4, 254, 252, 8, 4, 239, 17, 248, 4, 253, 252, 12, 4, 5, 255, 10, 4, 254, 252, 16, 4, 239, 17, 245, 4, 253, 252, 250, 4, 6, 255, 240, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 17, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 12, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 9, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 14, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 17, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 11, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 10, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 16, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 17, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 12, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 9, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 15, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 17, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 13, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 10, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 255
	};
	std::array<uint8_t, 512> LUT_200
	{
		2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 6, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 7, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 6, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 6, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 7, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 6, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 6, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 8, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 6, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 8, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 8, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 6, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 8, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 6, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 8, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 8
	};
	std::array<uint8_t, 0x40> LUT_400
	{
		0, 8, 0, 4, 0, 8, 0, 6, 0, 8, 0, 1, 0, 8, 0, 11, 0, 8, 0, 12, 0, 8, 0, 9, 0, 8, 0, 3, 0, 8, 0, 14, 0, 8, 0, 4, 0, 8, 0, 7, 0, 8, 0, 2, 0, 8, 0, 13, 0, 8, 0, 12, 0, 8, 0, 10, 0, 8, 0, 5, 0, 8, 0, 15
	};
	std::array<uint8_t, 0x40> LUT_440
	{
		1, 2, 1, 5, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 5, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 5, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 5, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 6
	};
	std::array<uint32_t, 16> LUT_480
	{
		74, 106, 138, 170, 202, 234, 266, 298, 330, 362, 394, 426, 938, 1450, 9642, 140714
	};
	std::array<uint8_t, 16> LUT_4C0
	{
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 9, 9, 13, 17, 21
	};
	std::array<uint8_t, 8> LUT_4D0
	{
		0, 0, 2, 4, 6, 8, 10, 42
	};
	std::array<uint8_t, 8> LUT_4D8
	{
		0, 1, 1, 1, 1, 1, 5, 5
	};
	std::array<uint8_t, 32> LUT_4E0
	{
		17, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
#pragma warning( pop ) 
}

struct rpak_h
{
	std::uint32_t m_nMagic;                    // 'RPak'
	std::uint16_t m_nVersion;                  // R2 = '7' R5 = '8'
	std::uint8_t  m_nFlags[0x2];               //
	std::uint8_t  m_nHash[0x10];               //
	std::uint64_t m_nSizeDisk;                 // Compressed size
	std::uint64_t m_nEmbeddedStarpakOffset;    //
	std::uint8_t  unk0[0x8];                   //
	std::uint64_t m_nSizeMemory;               // Decompressed size
	std::uint64_t m_nEmbeddedStarpakSize;      //
	std::uint8_t  unk1[0x8];                   //

	std::uint16_t m_nStarpakReferenceSize;     //
	std::uint16_t m_nStarpakOptReferenceSize;  //
	std::uint16_t m_nVirtualSegmentCount;      // * 0x10
	std::uint16_t m_nVirtualSegmentBlockCount; // * 0xC

	std::uint32_t m_nPatchIndex;               //

	std::uint32_t m_nUnknownThirdBlockCount;   //
	std::uint32_t m_nAssetEntryCount;          // File entry count
	std::uint32_t m_nUnknownFifthBlockCount;   //
	std::uint32_t m_nUnknownSixedBlockCount;   //

	std::uint8_t  unk2[0x1C];                  //
};

struct __declspec(align(8)) rpak_patch_compress_header
{
	std::uint64_t m_nSizeDisk;
	std::uint64_t m_nSizeMemory;
};

struct __declspec(align(8)) rpak_decomp_state
{
	std::uint64_t m_nInputBuf;
	std::uint64_t m_nOut;
	std::uint64_t m_nMask;
	std::uint64_t m_nOutMask;
	std::uint64_t m_nTotalFileLen;
	std::uint64_t m_nDecompSize;
	std::uint64_t m_nInvMaskIn;
	std::uint64_t m_nInvMaskOut;
	std::uint32_t header_skip_bytes_bs;
	std::uint32_t dword44;
	std::uint64_t input_byte_pos;
	std::uint64_t m_nDecompPosition;
	std::uint64_t len_needed;
	std::uint64_t byte;
	std::uint32_t byte_bit_offset;
	std::uint32_t dword6C;
	std::uint64_t qword70;
	std::uint64_t m_nCompressedStreamSize;
	std::uint64_t m_nDecompStreamSize;
};

namespace
{
	/* ==== RTECH =========================================================================================================================================================== */
	//DWORD64 p_RTech_Decompress = FindPatternV2("r5apex.exe", (const unsigned char*)"\x4C\x89\x44\x24\x18\x48\x89\x54\x24\x10\x53\x48\x83\xEC\x50\x48", "xxxxxxxxxxxxxxxx");
	//char (*RTech_Decompress)(int64_t* parameter, std::uint64_t input, std::uint64_t output) = (char (*)(std::int64_t*, std::uint64_t, std::uint64_t))p_RTech_Decompress; /*4C 89 44 24 18 48 89 54 24 10 53 48 83 EC 50 48*/

	//DWORD64 p_RTech_DecompressedSize = FindPatternV2("r5apex.exe", (const unsigned char*)"\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x18\x48\x89\x74\x24\x20\x48\x89\x54\x24\x10\x57\x41\x54\x41\x55\x41\x56\x41\x57\x4C\x8B\x74", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
	//std::int64_t (*RTech_DecompressedSize)(std::int64_t parameter, std::uint8_t* input, std::int64_t magic, std::int64_t a4, std::int64_t a5) = (std::int64_t (*)(std::int64_t, std::uint8_t*, std::int64_t, std::int64_t, std::int64_t))p_RTech_DecompressedSize; /*48 89 5C 24 08 48 89 6C 24 18 48 89 74 24 20 48 89 54 24 10 57 41 54 41 55 41 56 41 57 4C 8B 74*/
}

class RTech
{
public:
	std::uint64_t __fastcall StringToGuid(const char* pData);
	std::uint8_t __fastcall DecompressPakFile(rpak_decomp_state* state, std::uint64_t inLen, std::uint64_t outLen);
	std::uint32_t __fastcall DecompressPakFileInit(rpak_decomp_state* state, std::uint8_t* fileBuffer, std::int64_t fileSize, std::int64_t offNoHeader, std::int64_t headerSize);
};

///////////////////////////////////////////////////////////////////////////////
extern RTech* g_pRtech;
