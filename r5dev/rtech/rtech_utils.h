#pragma once

#define PAK_HEADER_SIZE   0x80
#define PAK_PARAM_SIZE    0xB0
#define DCMP_BUF_SIZE 0x400000

/*unk_141313180*/
// LUT_0 redacted now, split LUT into multiple parts.
#pragma warning( push )
#pragma warning( disable : 4309)
#pragma warning( disable : 4838)
inline std::array<int8_t, 512> LUT_0
	{
		4, 254, 252, 8, 4, 239, 17, 249, 4, 253, 252, 7, 4, 5, 255, 244, 4, 254, 252, 16, 4, 239, 17, 246, 4, 253, 252, 251, 4, 6, 255, 11, 4, 254, 252, 8, 4, 239, 17, 248, 4, 253, 252, 12, 4, 5, 255, 247, 4, 254, 252, 16, 4, 239, 17, 245, 4, 253, 252, 250, 4, 6, 255, 243, 4, 254, 252, 8, 4, 239, 17, 249, 4, 253, 252, 7, 4, 5, 255, 244, 4, 254, 252, 16, 4, 239, 17, 246, 4, 253, 252, 251, 4, 6, 255, 14, 4, 254, 252, 8, 4, 239, 17, 248, 4, 253, 252, 12, 4, 5, 255, 9, 4, 254, 252, 16, 4, 239, 17, 245, 4, 253, 252, 250, 4, 6, 255, 241, 4, 254, 252, 8, 4, 239, 17, 249, 4, 253, 252, 7, 4, 5, 255, 244, 4, 254, 252, 16, 4, 239, 17, 246, 4, 253, 252, 251, 4, 6, 255, 13, 4, 254, 252, 8, 4, 239, 17, 248, 4, 253, 252, 12, 4, 5, 255, 247, 4, 254, 252, 16, 4, 239, 17, 245, 4, 253, 252, 250, 4, 6, 255, 242, 4, 254, 252, 8, 4, 239, 17, 249, 4, 253, 252, 7, 4, 5, 255, 244, 4, 254, 252, 16, 4, 239, 17, 246, 4, 253, 252, 251, 4, 6, 255, 15, 4, 254, 252, 8, 4, 239, 17, 248, 4, 253, 252, 12, 4, 5, 255, 10, 4, 254, 252, 16, 4, 239, 17, 245, 4, 253, 252, 250, 4, 6, 255, 240, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 17, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 12, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 9, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 14, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 17, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 11, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 10, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 16, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 17, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 12, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 9, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 15, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 17, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 13, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 10, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 255
	};
inline std::array<uint8_t, 512> LUT_200
	{
		2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 6, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 7, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 6, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 6, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 7, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 6, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 6, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 8, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 6, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 8, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 8, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 6, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 8, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 6, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 8, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 8
	};
inline std::array<uint8_t, 0x40> LUT_400
	{
		0, 8, 0, 4, 0, 8, 0, 6, 0, 8, 0, 1, 0, 8, 0, 11, 0, 8, 0, 12, 0, 8, 0, 9, 0, 8, 0, 3, 0, 8, 0, 14, 0, 8, 0, 4, 0, 8, 0, 7, 0, 8, 0, 2, 0, 8, 0, 13, 0, 8, 0, 12, 0, 8, 0, 10, 0, 8, 0, 5, 0, 8, 0, 15
	};
inline std::array<uint8_t, 0x40> LUT_440
	{
		1, 2, 1, 5, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 5, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 5, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 5, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 6
	};
inline std::array<uint32_t, 16> LUT_480
	{
		74, 106, 138, 170, 202, 234, 266, 298, 330, 362, 394, 426, 938, 1450, 9642, 140714
	};
inline std::array<uint8_t, 16> LUT_4C0
	{
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 9, 9, 13, 17, 21
	};
inline std::array<uint8_t, 8> LUT_4D0
	{
		0, 0, 2, 4, 6, 8, 10, 42
	};
inline std::array<uint8_t, 8> LUT_4D8
	{
		0, 1, 1, 1, 1, 1, 5, 5
	};
inline std::array<uint8_t, 32> LUT_4E0
	{
		17, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
#pragma warning( pop ) 

enum class RPakStatus_t : std::int32_t
{
	PAK_STATUS_FREED = 0,
	PAK_STATUS_LOAD_PENDING = 1,
	PAK_STATUS_REPAK_RUNNING = 2,
	PAK_STATUS_REPAK_DONE = 3,
	PAK_STATUS_LOAD_STARTING = 4,
	PAK_STATUS_LOAD_PAKHDR = 5,
	PAK_STATUS_LOAD_PATCH_INIT = 6,
	PAK_STATUS_LOAD_PATCH_EDIT_STREAM = 7,
	PAK_STATUS_LOAD_ASSETS = 8,
	PAK_STATUS_LOADED = 9,
	PAK_STATUS_UNLOAD_PENDING = 10,
	PAK_STATUS_FREE_PENDING = 11,
	PAK_STATUS_CANCELING = 12,
	PAK_STATUS_ERROR = 13,
	PAK_STATUS_INVALID_PAKHANDLE = 14,
	PAK_STATUS_BUSY = 15
};

const std::map<RPakStatus_t, std::string> RPakStatusToString {
	{ RPakStatus_t::PAK_STATUS_FREED,                  "PAK_STATUS_FREED" },
	{ RPakStatus_t::PAK_STATUS_LOAD_PENDING,           "PAK_STATUS_LOAD_PENDING" },
	{ RPakStatus_t::PAK_STATUS_REPAK_RUNNING,          "PAK_STATUS_REPAK_RUNNING" },
	{ RPakStatus_t::PAK_STATUS_REPAK_DONE,             "PAK_STATUS_REPAK_DONE" },
	{ RPakStatus_t::PAK_STATUS_LOAD_STARTING,          "PAK_STATUS_LOAD_STARTING" },
	{ RPakStatus_t::PAK_STATUS_LOAD_PAKHDR,            "PAK_STATUS_LOAD_PAKHDR" },
	{ RPakStatus_t::PAK_STATUS_LOAD_PATCH_INIT,        "PAK_STATUS_LOAD_PATCH_INIT" },
	{ RPakStatus_t::PAK_STATUS_LOAD_PATCH_EDIT_STREAM, "PAK_STATUS_LOAD_PATCH_EDIT_STREAM" },
	{ RPakStatus_t::PAK_STATUS_LOAD_ASSETS,            "PAK_STATUS_LOAD_ASSETS" },
	{ RPakStatus_t::PAK_STATUS_LOADED,                 "PAK_STATUS_LOADED" },
	{ RPakStatus_t::PAK_STATUS_UNLOAD_PENDING,         "PAK_STATUS_UNLOAD_PENDING" },
	{ RPakStatus_t::PAK_STATUS_FREE_PENDING,           "PAK_STATUS_FREE_PENDING" },
	{ RPakStatus_t::PAK_STATUS_CANCELING,              "PAK_STATUS_CANCELING" },
	{ RPakStatus_t::PAK_STATUS_ERROR,                  "PAK_STATUS_ERROR" },
	{ RPakStatus_t::PAK_STATUS_INVALID_PAKHANDLE,      "PAK_STATUS_INVALID_PAKHANDLE" },
	{ RPakStatus_t::PAK_STATUS_BUSY,                   "PAK_STATUS_BUSY" },
};


struct RPakHeader_t
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

	std::uint32_t m_nDescriptorCount;          //
	std::uint32_t m_nAssetEntryCount;          // File entry count
	std::uint32_t m_nGuidDescriptorCount;      //
	std::uint32_t m_nRelationsCounts;         //

	std::uint8_t  unk2[0x1C];                  //
};

struct __declspec(align(8)) RPakPatchCompressedHeader_t
{
	std::uint64_t m_nSizeDisk;
	std::uint64_t m_nSizeMemory;
};

struct __declspec(align(8)) RPakDecompState_t
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
	std::uint64_t m_nLengthNeeded;
	std::uint64_t byte;
	std::uint32_t byte_bit_offset;
	std::uint32_t dword6C;
	std::uint64_t qword70;
	std::uint64_t m_nCompressedStreamSize;
	std::uint64_t m_nDecompStreamSize;
};

class RPakLoadedInfo_t
{
public:
	std::int32_t m_nPakId; //0x0000
	RPakStatus_t m_nStatus; //0x0004
	std::uint64_t m_nUnk1; //0x0008
	std::uint32_t m_nUnk2; //0x0010
	std::uint32_t m_nAssetCount; //0x0014
	char* m_pszFileName; //0x0018
	void* m_pUnk1; //0x0020
	std::uint64_t* m_pAssetGuids; //0x0028 size of the array is m_nAssetCount
	char pad_0030[128]; //0x0030
	std::uint64_t m_nUnkEnd; //0x00B0
}; //Size: 0x00B8

/* ==== RTECH =========================================================================================================================================================== */
inline RPakLoadedInfo_t* g_pLoadedPakInfo;
inline std::int16_t* s_pLoadedPakCount;

class RTech
{
public:
	std::uint64_t __fastcall StringToGuid(const char* pData);
	std::uint8_t __fastcall DecompressPakFile(RPakDecompState_t* state, std::uint64_t inLen, std::uint64_t outLen);
	std::uint32_t __fastcall DecompressPakFileInit(RPakDecompState_t* state, std::uint8_t* fileBuffer, std::int64_t fileSize, std::int64_t offNoHeader, std::int64_t headerSize);
	RPakLoadedInfo_t GetPakLoadedInfo(int nPakId);
};


///////////////////////////////////////////////////////////////////////////////
extern RTech* g_pRTech;

///////////////////////////////////////////////////////////////////////////////
class VPakFile : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| VAR: g_pLoadedPakInfo                     : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pLoadedPakInfo ));
		spdlog::debug("| VAR: s_pLoadedPakCount                    : {:#18x} |\n", reinterpret_cast<uintptr_t>(s_pLoadedPakCount));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		CMemory localRef = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x30\x8B\xC1"), "xxxx?xxxx?xxxxxxx");

		g_pLoadedPakInfo = localRef.FindPattern("48 8D 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<RPakLoadedInfo_t*>();
		s_pLoadedPakCount = localRef.FindPattern("66 89", CMemory::Direction::DOWN, 450).ResolveRelativeAddressSelf(0x3, 0x7).RCast<std::int16_t*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VPakFile);
