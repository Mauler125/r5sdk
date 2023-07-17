#ifndef RTECH_IPACKFILE_H
#define RTECH_IPACKFILE_H

#define PLATFORM_PAK_PATH "paks\\Win64\\"
#define PLATFORM_PAK_OVERRIDE_PATH "paks\\Win32\\"

#define INVALID_PAK_HANDLE -1

#define PAK_MAX_TYPES 64
#define PAK_HEADER_MAGIC (('k'<<24)+('a'<<16)+('P'<<8)+'R')

typedef int RPakHandle_t;

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
#if defined (GAMEDLL_S3)
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
#if !defined (GAMEDLL_S3)
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

struct PakFile_t // !TODO: Map out on S1 and S2!
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
	char gap610[256];
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1) // TODO: needs to be checked.
	char gap710[256];
#if !defined (GAMEDLL_S2)
	char gap810[8];
#endif // !(GAMEDLL_S0) || !(GAMEDLL_S1) || !(GAMEDLL_S2)
#endif
	const char* m_pszFileName;
	RPakHeader_t m_PakHdr;
};

#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
#if !defined (GAMEDLL_S2)
static_assert(sizeof(PakFile_t) == 2208); // S3+
#else
static_assert(sizeof(PakFile_t) == 2200); // S2
#endif // !GAMEDLL_S2
#else
static_assert(sizeof(PakFile_t) == 1944); // S0/S1
#endif // !GAMEDLL_S0 && !GAMEDLL_S1

static_assert(sizeof(RPakDecompState_t) == 136);
static_assert(sizeof(RPakPatchCompressedHeader_t) == 16);

#endif // RTECH_IPACKFILE_H
