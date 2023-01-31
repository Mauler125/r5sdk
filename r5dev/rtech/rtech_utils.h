#pragma once
#include "tier0/jobthread.h"
#include "vpklib/packedstore.h"
#include "rtech/rtech_game.h"
#ifndef DEDICATED
#include "public/rendersystem/schema/texture.g.h"
#endif // !DEDICATED

#define PAK_PARAM_SIZE    0xB0
#define DCMP_BUF_SIZE 0x400000

#define RPAKHEADER	(('k'<<24)+('a'<<16)+('P'<<8)+'R')

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

enum class RPakStatus_t : int32_t
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

const static std::map<RPakStatus_t, string> g_PakStatusToString {
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

struct RPakAssetBinding_t
{
	uint32_t m_nExtension; // For example '0x6C74616D' for the material asset.
	int m_iVersion;
	const char* m_szDescription; // Description/Name of asset.
	void* m_pLoadAssetFunction;
	void* m_pUnloadAssetFunction;
	void* m_pReplaceAssetFunction;
	void* m_pAllocAssetFunctions;
	int m_iSubHeaderSize;
	int m_iNativeClassSize; // Native class size, for 'material' it would be CMaterialGlue full size.
	uint32_t m_HeaderAlignment;
	int unk3;
	// [ PIXIE ]: Should be the full size across Season 0-3.
};

struct RPakAssetEntry_t
{
	uint64_t m_Guid;
	uint64_t m_Padding;
	uint32_t m_nHeadPageIdx;
	uint32_t m_nHeadPageOffset;
	uint32_t m_nCpuPageIdx;
	uint32_t m_nCpuPageOffset;
	uint64_t m_nStarpakOffset;
	uint64_t m_nStarpakOptOffset;
	uint16_t m_nPageEnd;
	uint16_t unk1;
	uint32_t m_nRelationsStartIdx;
	uint32_t m_nUsesStartIdx;
	uint32_t m_nRelationsCount;
	uint32_t m_nUsesCount;
	uint32_t m_nAssetHeaderSize;
	uint32_t m_nVersion;
	uint32_t m_nMagic;
};

struct RPakAssetEntryShort
{
	uint64_t m_Guid;
	uint64_t m_Padding;
	void* m_pHead;
	void* m_pCpu;
};

struct RPakGlobals_t
{
	RPakAssetBinding_t m_nAssetBindings[64]; // [ PIXIE ]: Max possible registered assets on Season 3, 0-2 I did not check yet.
	RPakAssetEntryShort m_Assets[0x40000];
	// End size unknown.
};

struct RPakHeader_t
{
	uint32_t m_nMagic;                     // 'RPak'
	uint16_t m_nVersion;                   // R2 = '7' R5 = '8'
	uint8_t  m_nFlags[0x2];                //
	FILETIME m_nFileTime;                  //
	uint64_t m_nHash;                      //
	uint64_t m_nSizeDisk;                  // Compressed size
	uint64_t m_nEmbeddedStarpakOffset;     //
	uint8_t  unk0[0x8];                    //
	uint64_t m_nSizeMemory;                // Decompressed size
	uint64_t m_nEmbeddedStarpakSize;       //
	uint8_t  unk1[0x8];                    //

	uint16_t m_nStarpakReferenceSize;      //
	uint16_t m_nStarpakOptReferenceSize;   //
	uint16_t m_nVirtualSegmentCount;       // * 0x10
	uint16_t m_nMemPageCount;              // * 0xC

	uint32_t m_nPatchIndex;                //

	uint32_t m_nDescriptorCount;           //
	uint32_t m_nAssetEntryCount;           // File entry count
	uint32_t m_nGuidDescriptorCount;       //
	uint32_t m_nRelationsCounts;           //

	uint8_t  unk2[0x10];                   //
	uint32_t m_nMemPageOffset;             // Size not verified. Offsets every page by x amount, if not 0 start of first page has data corresponding for 'patching some page'
	uint8_t  unk3[0x8];                    //
};

struct RPakPatchCompressedHeader_t
{
	uint64_t m_nSizeDisk;
	uint64_t m_nSizeMemory;
};

struct RPakDecompState_t
{
	uint64_t m_nInputBuf;
	uint64_t m_nOut;
	uint64_t m_nMask;
	uint64_t m_nOutMask;
	uint64_t m_nTotalFileLen;
	uint64_t m_nDecompSize;
	uint64_t m_nInvMaskIn;
	uint64_t m_nInvMaskOut;
	uint32_t m_nHeaderOffset;
	uint32_t dword44;
	uint64_t m_nInputBytePos;
	uint64_t m_nDecompPosition;
	uint64_t m_nLengthNeeded;
	uint64_t byte;
	uint32_t m_nByteBitOffset;
	uint32_t dword6C;
	uint64_t qword70;
	uint64_t m_nCompressedStreamSize;
	uint64_t m_nDecompStreamSize;
};

class RPakLoadedInfo_t
{
public:
	RPakHandle_t m_nHandle; //0x0000
	RPakStatus_t m_nStatus; //0x0004
	uint64_t m_nUnk1; //0x0008
	uint32_t m_nUnk2; //0x0010
	uint32_t m_nAssetCount; //0x0014
	char* m_pszFileName; //0x0018
	void* m_pMalloc; //0x0020
	uint64_t* m_pAssetGuids; //0x0028 size of the array is m_nAssetCount
#if defined GAMEDLL_S3
	void* m_pVSegBuffers[4]; //0x0030
	char pad_0050[16]; //0x0050
	void* m_pPakInfo; //0x0060
	RPakLoadedInfo_t* m_pUnknownLoadedPakInfo; //0x0068
	char pad_0070[4]; //0x0070
	int8_t m_nUnk3; //0x0074
	char pad_0075[51]; //0x0075
	uint32_t m_nUnk4; //0x00A8
	uint8_t m_nUnk5; //0x00AC
#endif
#if not defined GAMEDLL_S3
	char pad_0030[128]; //0x0030
	char pad_00B0[48];  //0x00B0
#endif // !GAMEDLL_S3
	uint64_t m_nUnkEnd; //0x00B0/0x00E8
}; //Size: 0x00B8/0x00E8

struct RPakDescriptor_t
{
	uint32_t m_Index;
	uint32_t m_Offset;
};

struct RPakMemPageInfo_t
{
	uint32_t m_nVirtualSegmentIndex;
	uint32_t m_nPageAlignment;
	uint32_t m_nDataSize;
};

struct RPakVirtualSegment_t
{
	uint32_t m_nFlags;
	uint32_t m_nAlignment;
	uint64_t m_nDataSize;
};

struct PakFile_t
{
	int m_nDescCount;
	int m_nProcessedAssetCount;
	int m_nPageEnd;
	int m_nPageStart;
	uint32_t m_nPatchIndex_maybe;
	uint32_t dword14;
	char gap18[184];
	uint32_t unsigned_intD0;
	char gapD4[284];
	uint64_t m_nInputBytePos;
	uint8_t byte1F8;
	char gap1F9[4];
	uint8_t byte1FD;
	uint8_t byte1FE;
	uint8_t byte200;
	RPakDecompState_t m_PakDecompState;
	uint64_t qword288;
	uint64_t qword290;
	uint64_t qword298;
	uint64_t qword2A0;
	char* m_PatchData;
	char gap2B0[696];
	unsigned __int8(__fastcall* pfunc568)(__int64, LARGE_INTEGER*, unsigned __int64);
	uint64_t qword570;
	uint64_t qword578;
	int* qword580;
	uint8_t** m_ppPagePointers;
	void* m_pPatchCompressPairs;
	uint64_t qword598;
	char* m_pszStreamingFilePaths;
	char* m_pszOptStreamingFilePaths;
	void* m_pVirtualSegments;
	RPakMemPageInfo_t* m_pMemPages;
	RPakDescriptor_t* m_pVirtualPointers;
	RPakAssetEntry_t* m_pAssetEntries;
	RPakDescriptor_t* m_pGuidDescriptors;
	uint32_t* m_pFileRelations;
	char gap5E0[40];
	RPakAssetEntry_t** m_ppAssetEntries;
	char gap610[520];
	const char* m_pszFileName;
	RPakHeader_t m_PakHdr;
};

static_assert(sizeof(PakFile_t) == 2208);
static_assert(sizeof(RPakDecompState_t) == 136);
static_assert(sizeof(RPakPatchCompressedHeader_t) == 16);

/* ==== RTECH =========================================================================================================================================================== */
#if not defined DEDICATED
inline CMemory p_RTech_CreateDXTexture;
inline auto RTech_CreateDXTexture = p_RTech_CreateDXTexture.RCast<void(*)(TextureHeader_t*, int64_t)>();

inline CMemory p_GetStreamOverlay;
inline auto GetStreamOverlay = p_GetStreamOverlay.RCast<void(*)(const char* mode, char* buf, size_t bufSize)>();
#endif

// [ PIXIE ]: I'm very unsure about this, but it really seems like it
inline CMemory p_RTech_FindFreeSlotInFiles;
inline auto RTech_FindFreeSlotInFiles = p_RTech_FindFreeSlotInFiles.RCast<int32_t(*)(int32_t*)>();

inline CMemory p_RTech_OpenFile;
inline auto RTech_OpenFile = p_RTech_OpenFile.RCast<int32_t(*)(const char*, void*, int64_t*)>();

inline CMemory p_RTech_RegisterAsset;
inline auto RTech_RegisterAsset = p_RTech_RegisterAsset.RCast<void(*)(int, int, const char*, void*, void*, void*, void*, int, int, uint32_t, int, int)>();

#ifdef GAMEDLL_S3
inline CMemory p_Pak_ProcessGuidRelationsForAsset;
inline auto RTech_Pak_ProcessGuidRelationsForAsset = p_Pak_ProcessGuidRelationsForAsset.RCast<void(__fastcall*)(PakFile_t*, RPakAssetEntry_t*)>();
#endif

inline CMemory p_StreamDB_Init;
inline auto v_StreamDB_Init = p_StreamDB_Init.RCast<void (*)(const char* pszLevelName)>();

inline RPakLoadedInfo_t* g_pLoadedPakInfo;
inline int16_t* g_pRequestedPakCount;
inline int16_t* g_pLoadedPakCount;
inline JobID_t* g_pPakLoadJobID;
inline RPakGlobals_t* g_pPakGlobals;

inline int32_t* s_pFileArray;
inline PSRWLOCK* s_pFileArrayMutex;
inline pFileHandleTracker_t* s_pFileHandles;

inline JobFifoLock_s* g_pPakFifoLock;
inline void* g_pPakFifoLockWrapper; // Pointer to functor that takes the global pak fifolock as argument.
inline bool* g_bPakFifoLockAcquired;

class RTech
{
public:
	uint64_t __fastcall StringToGuid(const char* pData);
	uint8_t __fastcall DecompressPakFile(RPakDecompState_t* state, uint64_t inLen, uint64_t outLen);
	uint64_t __fastcall DecompressPakFileInit(RPakDecompState_t* state, uint8_t* fileBuffer, uint64_t fileSize, uint64_t offNoHeader, uint64_t headerSize);
	RPakLoadedInfo_t* GetPakLoadedInfo(RPakHandle_t nPakId);
	RPakLoadedInfo_t* GetPakLoadedInfo(const char* szPakName);

	static int32_t OpenFile(const CHAR* szFilePath, void* unused, LONGLONG* fileSizeOut);

#ifdef GAMEDLL_S3
	static void PakProcessGuidRelationsForAsset(PakFile_t* pak, RPakAssetEntry_t* asset);
#endif


#if not defined DEDICATED
	static void CreateDXTexture(TextureHeader_t* textureHeader, int64_t cpuArg);
	void** LoadShaderSet(void** VTablePtr);
#endif // !DEDICATED
};

///////////////////////////////////////////////////////////////////////////////
extern RTech* g_pRTech;

///////////////////////////////////////////////////////////////////////////////
class V_RTechUtils : public IDetour
{
	virtual void GetAdr(void) const
	{
#if not defined DEDICATED
		LogFunAdr("RTech::CreateDXTexture", p_RTech_CreateDXTexture.GetPtr());
#endif // !DEDICATED
		LogFunAdr("RTech::FindFreeSlotInFiles", p_RTech_FindFreeSlotInFiles.GetPtr());
		LogFunAdr("RTech::OpenFile", p_RTech_OpenFile.GetPtr());
#if not defined DEDICATED
		LogFunAdr("GetStreamOverlay", p_GetStreamOverlay.GetPtr());
#endif // !DEDICATED
		LogFunAdr("StreamDB_Init", p_StreamDB_Init.GetPtr());
		LogVarAdr("s_FileArray", reinterpret_cast<uintptr_t>(s_pFileArray));
		LogVarAdr("s_FileArrayMutex", reinterpret_cast<uintptr_t>(s_pFileArrayMutex));
		LogVarAdr("s_FileHandles", reinterpret_cast<uintptr_t>(s_pFileHandles));

		LogVarAdr("g_loadedPakInfo", reinterpret_cast<uintptr_t>(g_pLoadedPakInfo));
		LogVarAdr("g_loadedPakCount", reinterpret_cast<uintptr_t>(g_pLoadedPakCount));
		LogVarAdr("g_requestedPakCount", reinterpret_cast<uintptr_t>(g_pRequestedPakCount));

		LogVarAdr("g_pakGlobals", reinterpret_cast<uintptr_t>(g_pPakGlobals));
		LogVarAdr("g_pakLoadJobID", reinterpret_cast<uintptr_t>(g_pPakLoadJobID));

		LogVarAdr("g_pakFifoLock", reinterpret_cast<uintptr_t>(g_pPakFifoLock));
		LogVarAdr("g_pakFifoLockWrapper", reinterpret_cast<uintptr_t>(g_pPakFifoLockWrapper));
		LogVarAdr("g_pakFifoLockAcquired", reinterpret_cast<uintptr_t>(g_bPakFifoLockAcquired));
	}
	virtual void GetFun(void) const 
	{
#if not defined DEDICATED
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_RTech_CreateDXTexture = g_GameDll.FindPatternSIMD("48 8B C4 48 89 48 08 53 55 41 55");
		RTech_CreateDXTexture = p_RTech_CreateDXTexture.RCast<void(*)(TextureHeader_t*, int64_t)>(); /*48 8B C4 48 89 48 08 53 55 41 55*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_RTech_CreateDXTexture = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 4C 8B C7 48 8B D5 48 8B CB 48 83 C4 60").FollowNearCallSelf();
		RTech_CreateDXTexture = p_RTech_CreateDXTexture.RCast<void(*)(TextureHeader_t*, int64_t)>(); /*E8 ? ? ? ? 4C 8B C7 48 8B D5 48 8B CB 48 83 C4 60*/
#endif
		p_GetStreamOverlay = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 80 7C 24 ?? ?? 0F 84 ?? ?? ?? ?? 48 89 9C 24 ?? ?? ?? ??").FollowNearCallSelf();
		GetStreamOverlay = p_GetStreamOverlay.RCast<void(*)(const char*, char*, size_t)>(); /*E8 ? ? ? ? 80 7C 24 ? ? 0F 84 ? ? ? ? 48 89 9C 24 ? ? ? ?*/
#endif // !DEDICATED
		p_StreamDB_Init = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 54 41 56 41 57 48 83 EC 40 48 8B E9");
		v_StreamDB_Init = p_StreamDB_Init.RCast<void (*)(const char*)>(); /*48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 54 41 56 41 57 48 83 EC 40 48 8B E9*/

		p_RTech_FindFreeSlotInFiles = g_GameDll.FindPatternSIMD("44 8B 51 0C 4C 8B C1");
		RTech_FindFreeSlotInFiles   = p_RTech_FindFreeSlotInFiles.RCast<int32_t(*)(int32_t*)>(); /*44 8B 51 0C 4C 8B C1*/

		p_RTech_OpenFile = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 89 85 08 01 ?? ??").FollowNearCallSelf();
		RTech_OpenFile   = p_RTech_OpenFile.RCast<int32_t(*)(const char*, void*, int64_t*)>(); /*E8 ? ? ? ? 89 85 08 01 00 00*/

		p_RTech_RegisterAsset = g_GameDll.FindPatternSIMD("4D 89 42 08").FindPatternSelf("48 89 6C", CMemory::Direction::UP);
		RTech_RegisterAsset   = p_RTech_RegisterAsset.RCast<void(*)(int, int, const char*, void*, void*, void*, void*, int, int, uint32_t, int, int)>(); /*4D 89 42 08*/

#ifdef GAMEDLL_S3
		p_Pak_ProcessGuidRelationsForAsset = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 8B 86 ?? ?? ?? ?? 42 8B 0C B0").FollowNearCallSelf();
		RTech_Pak_ProcessGuidRelationsForAsset = p_Pak_ProcessGuidRelationsForAsset.RCast<void(__fastcall*)(PakFile_t*, RPakAssetEntry_t*)>(); /*E8 ? ? ? ? 48 8B 86 ? ? ? ? 42 8B 0C B0*/
#endif
	}
	virtual void GetVar(void) const
	{
		s_pFileArray      = p_StreamDB_Init.Offset(0x70).FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN, 512, 2).ResolveRelativeAddress(0x3, 0x7).RCast<int32_t*>();
		s_pFileHandles    = p_StreamDB_Init.Offset(0x70).FindPatternSelf("4C 8D", CMemory::Direction::DOWN, 512, 1).ResolveRelativeAddress(0x3, 0x7).RCast<pFileHandleTracker_t*>();
		s_pFileArrayMutex = p_StreamDB_Init.Offset(0x70).FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN, 512, 1).ResolveRelativeAddress(0x3, 0x7).RCast<PSRWLOCK*>();

		g_pLoadedPakInfo     = p_CPakFile_UnloadPak.FindPattern("48 8D 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<RPakLoadedInfo_t*>();
		g_pRequestedPakCount = p_CPakFile_UnloadPak.FindPattern("66 89", CMemory::Direction::DOWN, 450).ResolveRelativeAddressSelf(0x3, 0x7).RCast<int16_t*>();
		g_pLoadedPakCount    = &*g_pRequestedPakCount - 1; // '-1' shifts it back with sizeof(int16_t).

		g_pPakGlobals   = g_GameDll.FindPatternSIMD("48 8D 1D ?? ?? ?? ?? 45 8D 5A 0E").ResolveRelativeAddressSelf(0x3, 0x7).RCast<RPakGlobals_t*>(); /*48 8D 1D ? ? ? ? 45 8D 5A 0E*/
		g_pPakLoadJobID = reinterpret_cast<JobID_t*>(&*g_pLoadedPakCount - 2);

		g_pPakFifoLock         = p_JT_HelpWithAnything.Offset(0x155).FindPatternSelf("48 8D 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<JobFifoLock_s*>();
		g_pPakFifoLockWrapper  = p_JT_HelpWithAnything.Offset(0x1BC).FindPatternSelf("48 8D 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<void*>();
		g_bPakFifoLockAcquired = p_JT_HelpWithAnything.Offset(0x50).FindPatternSelf("C6 05").ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////
