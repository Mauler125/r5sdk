#ifndef RTECH_IPACKFILE_H
#define RTECH_IPACKFILE_H

#define PLATFORM_PAK_PATH "paks\\Win64\\"
#define PLATFORM_PAK_OVERRIDE_PATH "paks\\Win32\\"

#define INVALID_PAK_HANDLE -1

#define PAK_MAX_TYPES 64
#define PAK_HEADER_MAGIC (('k'<<24)+('a'<<16)+('P'<<8)+'R')

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
struct PakFile_t;

typedef int PakHandle_t;

enum class EPakStatus : int32_t
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

struct PakAssetBinding_t
{
	uint32_t m_extension; // For example '0x6C74616D' for the material asset.
	int m_version;
	const char* m_description; // Description/Name of asset.
	void* m_loadAssetFunction;
	void* m_unloadAssetFunction;
	void* m_replaceAssetFunction;
	void* m_allocAssetFunctions;
	int m_subHeaderSize;
	int m_nativeClassSize; // Native class size, for 'material' it would be CMaterialGlue full size.
	uint32_t m_headerAlignment;
	int unk3;
	// [ PIXIE ]: Should be the full size across Season 0-3.
};

struct PakAsset_t
{
	uint64_t m_guid;
	uint64_t m_padding;
	uint32_t m_headPageIdx;
	uint32_t m_headPageOffset;
	uint32_t m_cpuPageIdx;
	uint32_t m_cpuPageOffset;
	uint64_t m_starpakOffset;
	uint64_t m_starpakOptOffset;
	uint16_t m_pageEnd;
	uint16_t unk1;
	uint32_t m_relationsStartIdx;
	uint32_t m_usesStartIdx;
	uint32_t m_relationsCount;
	uint32_t m_usesCount;
	uint32_t m_assetHeaderSize;
	uint32_t m_version;
	uint32_t m_magic;
};

struct PakAssetShort_t
{
	uint64_t m_guid;
	uint64_t m_padding;
	void* m_head;
	void* m_cpu;
};

struct PakGlobals_t
{
	PakAssetBinding_t m_assetBindings[64]; // [ PIXIE ]: Max possible registered assets on Season 3, 0-2 I did not check yet.
	PakAssetShort_t m_assets[0x40000];
	// End size unknown.
};

struct PakFileHeader_t
{
	uint32_t m_magic;                     // 'RPak'
	uint16_t m_version;                   // R2 = '7' R5 = '8'
	uint8_t  m_flags[0x2];                //
	FILETIME m_fileTime;                  //
	uint64_t m_checksum;                  //
	uint64_t m_compressedSize;            // Compressed size
	uint64_t m_embeddedStarpakOffset;     //
	uint8_t  unk0[0x8];                   //
	uint64_t m_decompressedSize;          // Decompressed size
	uint64_t m_embeddedStarpakSize;       //
	uint8_t  unk1[0x8];                   //

	uint16_t m_starpakReferenceSize;      //
	uint16_t m_starpakOptReferenceSize;   //
	uint16_t m_virtualSegmentCount;       // * 0x10
	uint16_t m_memPageCount;              // * 0xC

	uint32_t m_patchIndex;                //

	uint32_t m_descriptorCount;           //
	uint32_t m_assetEntryCount;           // File entry count
	uint32_t m_guidDescriptorCount;       //
	uint32_t m_relationsCounts;           //

	uint8_t  unk2[0x10];                  //
	uint32_t m_memPageOffset;             // Size not verified. Offsets every page by x amount, if not 0 start of first page has data corresponding for 'patching some page'
	uint8_t  unk3[0x8];                   //
}; static_assert(sizeof(PakFileHeader_t) == 0x80);

struct PakPatchFileHeader_t
{
	size_t m_sizeDisk;
	size_t m_sizeMemory;
};

struct PakPatchDataHeader_t
{
	int m_editStreamSize;
	int m_pageCount;
};

struct PakSegmentHeader_t
{
	int m_flags;
	int m_align;
	size_t m_size;
};

struct PakDecompState_t
{
	uint64_t m_inputBuf;
	uint64_t m_outputBuf;
	uint64_t m_inputMask;
	uint64_t m_outputMask;
	uint64_t m_fileSize;
	uint64_t m_decompSize;
	uint64_t m_inputInvMask;
	uint64_t m_outputInvMask;
	uint32_t m_headerOffset;
	uint32_t dword44;
	uint64_t m_fileBytePosition;
	uint64_t m_decompBytePosition;
	uint64_t m_bufferSizeNeeded;
	uint64_t m_currentByte;
	uint32_t m_currentByteBit;
	uint32_t dword6C;
	uint64_t qword70;
	uint64_t m_compressedStreamSize;
	uint64_t m_decompStreamSize;
};

class PakLoadedInfo_t
{
public:
	PakHandle_t m_handle; //0x0000
	EPakStatus m_status; //0x0004
	uint64_t m_nUnk1; //0x0008
	uint32_t m_nUnk2; //0x0010
	uint32_t m_assetCount; //0x0014
	char* m_fileName; //0x0018
	void* m_allocator; //0x0020
	uint64_t* m_assetGuids; //0x0028 size of the array is m_nAssetCount
#if defined (GAMEDLL_S3)
	void* m_virtualSegmentBuffers[4]; //0x0030
	char pad_0050[16]; //0x0050
	void* m_pakInfo; //0x0060
	PakLoadedInfo_t* m_pUnknownLoadedPakInfo; //0x0068
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

struct PakPage_t
{
	uint32_t m_index;
	uint32_t m_offset;
};

struct PakPageHeader_t
{
	uint32_t m_virtualSegmentIndex;
	uint32_t m_pageAlignment;
	uint32_t m_dataSize;
};

struct PakFileStream_t
{
	_QWORD qword0;
	_QWORD qword8;
	_DWORD fileHandle;
	_DWORD gap14[32];
	_BYTE gap94[32];
	unsigned int unsigned_intB4;
	_DWORD dwordB8;
	_BYTE byteBC;
	_BYTE byteBD;
	_BYTE gapBE;
	_BYTE byteBF;
	_BYTE gapC0[256];
	uint8_t* buffer;
	_QWORD qword1C8;
	_QWORD qword1D0;
};

#pragma pack(push, 4)
struct RBitRead // TODO: move to own file?
{
	uint64_t m_dataBuf;
	int m_bitsRemaining;
};
#pragma pack(pop)

struct PakMemoryData_t
{
	uint64_t m_processedPatchedDataSize;
	char* m_patchData;

	uint64_t m_patchDataPtr;
	RBitRead m_bitBuf;
	uint32_t m_patchDataOffset;

	_BYTE patchCommands[64];

	_BYTE PATCH_field_68[64];
	_BYTE PATCH_unk2[256];
	_BYTE PATCH_unk3[256];

	_QWORD field_2A8;

	size_t m_patchSrcSize;
	char* m_patchDstPtr;
	__int64 m_numBytesToProcess_maybe;

	unsigned __int8(__fastcall* patchFunc)(PakFile_t*, unsigned __int64*);

	uint64_t qword2D0;
	uint64_t qword2D8;
	int* qword2E0;
	uint8_t** m_pagePointers;

	PakPatchFileHeader_t* m_patchHeaders;
	short* UnkPatchIndexes;

	char* m_streamingFilePaths;
	char* m_optStreamingFilePaths;

	PakSegmentHeader_t* m_segmentHeaders;
	PakPageHeader_t* m_pageHeaders;

	PakPage_t* m_virtualPointers;
	PakAsset_t* m_assetEntries;

	PakPage_t* m_guidDescriptors;
	uint32_t* m_fileRelations;

	char gap5E0[32];

	PakPatchDataHeader_t* m_patchDataHeader;
	PakAsset_t** m_ppAssetEntries;

	_BYTE gap370[520];

	const char* m_fileName;
	PakFileHeader_t m_pakHeader;
};

struct PakFile_t
{
	int m_numPointers;
	int m_numAssets;
	int m_pageCount;
	int m_pageStart;

	uint32_t m_patchCount;
	uint32_t dword14;

	PakFileStream_t m_fileStream;
	uint64_t m_inputBytePos;
	uint8_t byte1F8;
	char gap1F9[4];
	uint8_t byte1FD;
	short flags_1FE;
	PakDecompState_t m_pakDecompState;
	void* m_decompBuffer;
	size_t m_maxCopySize;
	uint64_t qword298;

	PakMemoryData_t m_memoryData;
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

static_assert(sizeof(PakDecompState_t) == 136);
static_assert(sizeof(PakPatchFileHeader_t) == 16);

#endif // RTECH_IPACKFILE_H
