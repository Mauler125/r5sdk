#ifndef RTECH_IPACKFILE_H
#define RTECH_IPACKFILE_H
#include "tier0/jobthread.h"
#include "tier0/tslist.h"

#include "rtech/iasync.h"
#include "rtech/rstdlib.h"
#include "thirdparty/zstd/zstd.h"

// pak header versions
#define PAK_HEADER_MAGIC (('k'<<24)+('a'<<16)+('P'<<8)+'R')
#define PAK_HEADER_VERSION 8

// pak header flags
#define PAK_HEADER_FLAGS_HAS_MODULE (1<<0)
#define PAK_HEADER_FLAGS_COMPRESSED (1<<8)

// use the ZStd decoder instead of the RTech one
#define PAK_HEADER_FLAGS_ZSTREAM    (1<<9)

// max amount of types at runtime in which assets will be tracked
#define PAK_MAX_TYPES 64
#define PAK_MAX_TYPES_MASK (PAK_MAX_TYPES-1)

// max amount of global pak assets at runtime
#define PAK_MAX_ASSETS 0x40000
#define PAK_MAX_ASSETS_MASK (PAK_MAX_ASSETS-1)

// max amount of segments a pak file could have
#define PAK_MAX_SEGMENTS 20

// max amount of buffers in which segments get copied in
#define PAK_SEGMENT_BUFFER_TYPES 4

// max amount of streaming files that could be opened per set for a pak, so if a
// pak uses more than one set, this number would be used per set
#define PAK_MAX_STREAMING_FILE_HANDLES_PER_SET 4

// max amount of paks that could be loaded at runtime
#define PAK_MAX_HANDLES 512
#define PAK_MAX_HANDLES_MASK (PAK_MAX_HANDLES-1)

// base pak directory containing paks sorted in platform specific subdirectories
#define PAK_BASE_PATH "paks\\"
#define PLATFORM_PAK_PATH PAK_BASE_PATH"Win64\\"

// pak override directory; the system will looks for pak files in this directory
// first before falling back to PLATFORM_PAK_PATH
#define PLATFORM_PAK_OVERRIDE_PATH PAK_BASE_PATH"Win64_override\\"

// the minimum stream ring buffer size when a pak is loaded who's compressed
// size is below PAK_DECODE_IN_RING_BUFFER_SIZE, the system allocates a
// buffer with the size of the compressed pak + the value of this define
// the system should still use the default ring buffer input size for decoding
// as these pak files get decoded in one-go; there is buffer wrapping going on
#define PAK_DECODE_IN_RING_BUFFER_SMALL_SIZE 0x1000
#define PAK_DECODE_IN_RING_BUFFER_SMALL_MASK (PAK_DECODE_IN_RING_BUFFER_SMALL_SIZE-1)

// the input stream ring buffer size for pak decoder before wrapping around
#define PAK_DECODE_IN_RING_BUFFER_SIZE 0x1000000
#define PAK_DECODE_IN_RING_BUFFER_MASK (PAK_DECODE_IN_RING_BUFFER_SIZE-1)

// the output stream ring buffer size in which input buffer gets decoded to, we
// can only decode up to this many bytes before we have to wrap around
#define PAK_DECODE_OUT_RING_BUFFER_SIZE 0x400000
#define PAK_DECODE_OUT_RING_BUFFER_MASK (PAK_DECODE_OUT_RING_BUFFER_SIZE-1)

// the handle that should be returned when a pak failed to load or process
#define INVALID_PAK_HANDLE -1

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
struct PakFile_t;

//-----------------------------------------------------------------------------
// Handle types
//-----------------------------------------------------------------------------
typedef int PakHandle_t;
typedef uint64_t PakGuid_t;

//-----------------------------------------------------------------------------
// Page handle types
//-----------------------------------------------------------------------------
struct PakPageHeader_t
{
	uint32_t segmentIdx;
	uint32_t pageAlignment;
	uint32_t dataSize;
};

// ptr.index  != UINT32_MAX
// ptr.offset != UINT32_MAX
// ptr.index   < pak->GetPageCount()
// ptr.offset <= pak->GetPageSize(ptr.index)
#define IS_PAKPTR_VALID(pak, ptr) ((ptr)->index != UINT32_MAX && (ptr)->offset != UINT32_MAX && (ptr)->index < (pak)->GetPageCount() && (ptr)->offset <= (pak)->GetPageSize((ptr)->index))
#define ASSERT_PAKPTR_VALID(pak, ptr) Assert(IS_PAKPTR_VALID(pak, ptr), "Invalid pak page pointer")

union PakPage_t
{
	struct
	{
		uint32_t index;
		uint32_t offset;
	};
	void* ptr;
};

//-----------------------------------------------------------------------------
// Enumerations
//-----------------------------------------------------------------------------
enum EPakStatus : int32_t
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

enum EPakStreamSet
{
	STREAMING_SET_MANDATORY = 0,
	STREAMING_SET_OPTIONAL,

	// number of streaming sets
	STREAMING_SET_COUNT
};

struct PakAssetBinding_t
{
	enum EType
	{
		NONE = 0,

		// not registered, assets with type 'NONE'
		// will be stubbed with this value!
		STUB,

		// explicitly registered by pak or code
		REGISTERED
	};

	// for example '0x6C74616D' for the material asset.
	uint32_t extension;
	uint32_t version;

	// description or name of asset.
	const char* description;

	// asset specific load callbacks
	void* loadAssetFunc;
	void* unloadAssetFunc;
	void* replaceAssetFunc;

	CAlignedMemAlloc* allocator;

	unsigned int headerSize;
	unsigned int nativeClassSize; // Native class size, for 'material' it would be CMaterialGlue full size.
	unsigned int headerAlignment;

	// the type of this asset bind
	// NOTE: the asset bind will be stubbed if its 'NONE' in runtime!
	EType type;

	// [ PIXIE ]: Should be the full size across Season 0-3.
};

struct PakAsset_t
{
	// the guid of this asset, which will be used to index into, and retrieve
	// this asset from the hash table
	PakGuid_t guid;
	uint64_t padding; // Unknown.

	PakPage_t headPtr;
	PakPage_t dataPtr;

	// offset to the streaming data in the streaming set
	uint64_t streamingDataFileOffset[STREAMING_SET_COUNT];
	uint16_t pageEnd;

	// the number of remaining dependencies that are yet to be resolved
	uint16_t numRemainingDependencies;
	uint32_t dependentsIndex;
	uint32_t dependenciesIndex;
	uint32_t dependentsCount;
	uint32_t dependenciesCount;

	// size of the asset's header
	uint32_t headerSize;

	// versions of the asset
	uint32_t version;
	uint32_t magic;

	FORCEINLINE uint8_t HashTableIndexForAssetType() const
	{
		return (((0x1020601 * magic) >> 24) & PAK_MAX_TYPES_MASK);
	}
};

struct PakAssetShort_t
{
	PakGuid_t guid;
	uint32_t unk_8;
	uint32_t unk_C;
	void* m_head;
	void* m_cpu;
};

struct PakGlobals_t
{
	PakAssetBinding_t m_assetBindings[PAK_MAX_TYPES]; // [ PIXIE ]: Max possible registered assets on Season 3, 0-2 I did not check yet.
	PakAssetShort_t m_assets[PAK_MAX_ASSETS];
	// end size unknown, but there appears to be stuff below too
};

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

struct PakFileHeader_t
{
	inline uint32_t GetTotalStreamingNamesBufferSize() const
	{
		return(
			streamingFilesBufSize[STREAMING_SET_MANDATORY] +
			streamingFilesBufSize[STREAMING_SET_OPTIONAL]);
	}

	inline uint64_t GetTotalEmbeddedStreamingDataSize() const
	{
		return (
			embeddedStreamingDataSize[STREAMING_SET_MANDATORY] + 
			embeddedStreamingDataSize[STREAMING_SET_OPTIONAL]);
	}

	inline uint64_t GetTotalHeaderSize() const
	{
		uint64_t headerSize = sizeof(PakFileHeader_t);

		// if we have patches, we should include the patch header as well
		if (patchIndex > 0)
			headerSize += sizeof(PakPatchDataHeader_t);

		// the streaming file paths belong to the header as well
		headerSize += GetTotalStreamingNamesBufferSize();
		return headerSize;
	}

	inline bool IsCompressed() const
	{
		return flags & PAK_HEADER_FLAGS_COMPRESSED;
	}

	// file versions
	uint32_t magic;       // 'RPak'
	uint16_t version;     // R2 = '7' R5 = '8'

	// pak file flags
	uint16_t flags;

	// when this pak file was built
	FILETIME fileTime;
	uint64_t checksum;

	// compressed size of the pak file, this includes the header
	uint64_t compressedSize;

	// offset to the embedded mandatory and optional streaming data
	// NOTE: this should be NULL if external streaming sets are used
	uint64_t embeddedStreamingDataOffset[STREAMING_SET_COUNT];

	// decompressed size of  this pak, this includes the header
	// NOTE: if the pak is uncompressed, this will equal compressedSize
	uint64_t decompressedSize;

	// size of the embedded mandatory and optional streaming data
	// NOTE: this should be NULL if external streaming sets are used
	uint64_t embeddedStreamingDataSize[STREAMING_SET_COUNT];

	// size of the string array containing paths to external streaming files
	uint16_t streamingFilesBufSize[STREAMING_SET_COUNT];

	// number of segments in this pak; absolute max = PAK_MAX_SEGMENTS
	uint16_t virtualSegmentCount;

	// number of memory pages to allocate for this pak
	uint16_t memPageCount;

	uint16_t patchIndex;

	uint32_t descriptorCount;

	// number of assets in this pak
	uint32_t assetCount;
	uint32_t guidDescriptorCount;
	uint32_t relationsCounts;

	uint8_t  unk2[0x10];

	// size not verified. offsets every page by x amount, if not 0 start of first page has data corresponding for 'patching some page'
	uint32_t memPageOffset;

	uint8_t  unk3[0x8];
}; static_assert(sizeof(PakFileHeader_t) == 0x80);

// segment flags
#define SF_HEAD (0)
#define SF_TEMP (1 << 0) // 0x1
#define SF_CPU  (1 << 1) // 0x2
#define SF_DEV  (1 << 8) // 0x80

struct PakSegmentHeader_t
{
	int typeFlags;
	int dataAlignment;
	size_t dataSize;
};

struct PakSegmentDescriptor_t // TODO: give this a better name!!!
{
	size_t assetTypeCount[PAK_MAX_TYPES];
	int64_t segmentSizes[PAK_MAX_SEGMENTS];

	size_t segmentSizeForType[PAK_SEGMENT_BUFFER_TYPES];
	int segmentAlignmentForType[PAK_SEGMENT_BUFFER_TYPES];
};

struct PakDecoder_t
{
	const uint8_t* inputBuf;
	uint8_t* outputBuf;

	uint64_t inputMask;
	uint64_t outputMask;

	size_t fileSize;
	size_t decompSize;

	uint64_t inputInvMask;
	uint64_t outputInvMask;

	uint32_t headerOffset;
	uint32_t padding; // unused data, available for other stuff

	uint64_t inBufBytePos;
	uint64_t outBufBytePos;

	size_t bufferSizeNeeded;

	// current byte and current bit of byte
	uint64_t currentByte;
	uint32_t currentBit;

	uint32_t dword6C;
	uint64_t qword70;

	union
	{
		size_t compressedStreamSize;
		size_t frameHeaderSize;
	};

	union
	{
		size_t decompressedStreamSize;
		ZSTD_DStream* zstreamContext;
	};
};

struct PakRingBufferFrame_t
{
	size_t bufIndex;
	size_t frameLen;
};

struct PakFile_t;

class PakLoadedInfo_t
{
public:
	struct StreamingInfo_t
	{
		inline void Reset()
		{
			embeddedStarpakName = nullptr;
			streamFileNumber[0] = FS_ASYNC_FILE_INVALID;
			streamFileNumber[1] = FS_ASYNC_FILE_INVALID;
			streamFileNumber[2] = FS_ASYNC_FILE_INVALID;
			streamFileNumber[3] = FS_ASYNC_FILE_INVALID;
			streamFileCount = NULL;
			streamingDisabled = true;
		}

		char* embeddedStarpakName;
		int streamFileNumber[PAK_MAX_STREAMING_FILE_HANDLES_PER_SET];
		int streamFileCount;
		bool streamingDisabled;
		char padding_maybe[3];
	};

	PakHandle_t handle;
	EPakStatus status;
	uint64_t m_nUnk1;

	// the log level of the pak, this is also used for errors if a pak failed
	// to load; the higher the level, the more important this pak file is
	int logLevel;

	uint32_t assetCount;
	const char* fileName;
	CAlignedMemAlloc* allocator;
	PakGuid_t* assetGuids; //size of the array is m_nAssetCount
	void* segmentBuffers[PAK_SEGMENT_BUFFER_TYPES];
	_QWORD qword50;
	FILETIME fileTime;
	PakFile_t* pakFile;
	StreamingInfo_t streamInfo[STREAMING_SET_COUNT];
	uint32_t fileHandle;
	uint8_t m_nUnk5;
	uint64_t hModule;

}; //Size: 0x00B8/0x00E8

struct PakFileStream_t
{
	struct Descriptor
	{
		size_t dataOffset;
		size_t compressedSize;
		size_t decompressedSize;

		// NOTE: if this is set, the game sets 'PakMemoryData_t::m_processedPatchedDataSize'
		// to 'dataOffset'; else its getting set to 'sizeof(PakFileHeader_t)'.
		bool isCompressed;
	};

	_QWORD qword0;
	_QWORD qword8;
	int fileHandle; // TODO: Unsigned?
	int gap14[32];  // TODO: Unsigned?
	_BYTE gap94[32];
	unsigned int unsigned_intB4;
	_DWORD dwordB8;
	_BYTE byteBC;
	bool finishedLoadingPatches;
	_BYTE gapBE;
	_BYTE byteBF;
	Descriptor m_descriptors[8];
	uint8_t* buffer;
	_QWORD qword1C8;
	_QWORD bytesStreamed;
};

typedef struct PakPatchFuncs_s
{
	typedef bool (*PatchFunc_t)(PakFile_t* const pak, size_t* const numAvailableBytes);

	enum EPatchCommands
	{
		PATCH_CMD0,
		PATCH_CMD1,
		PATCH_CMD2,
		PATCH_CMD3,
		PATCH_CMD4,
		PATCH_CMD5, // Same as cmd4.
		PATCH_CMD6,

		// !!! NOT A CMD !!!
		PATCH_CMD_COUNT
	};

	inline PatchFunc_t operator[](ssize_t i) const
	{
		Assert((i >= 0) && (i < SDK_ARRAYSIZE(patchFuncs)));
		return patchFuncs[i];
	}

	PatchFunc_t patchFuncs[PATCH_CMD_COUNT];

} PakPatchFuncs_t;

struct PakMemoryData_t
{
	uint64_t processedPatchedDataSize;
	char* patchData; // pointer to patch stream data

	char* patchDataPtr;
	RBitRead bitBuf;
	uint32_t patchDataOffset;

	_BYTE patchCommands[64];

	_BYTE PATCH_field_68[64];
	_BYTE PATCH_unk2[256];
	_BYTE PATCH_unk3[256];

	_QWORD field_2A8;

	// number of bytes remaining in the patch stream data
	size_t patchSrcSize;

	// pointer to the location in the pak that a patch command is writing to
	char* patchDstPtr;

	size_t numBytesToProcess_maybe;
	PakPatchFuncs_s::PatchFunc_t patchFunc;

	uint64_t qword2D0;
	PakHandle_t pakId;
	JobID_t unkJobID;
	int* qword2E0;
	uint8_t** memPageBuffers;

	PakPatchFileHeader_t* patchHeaders;
	unsigned short* patchIndices;

	char* streamingFilePaths[STREAMING_SET_COUNT];

	PakSegmentHeader_t* segmentHeaders;
	PakPageHeader_t* pageHeaders;

	PakPage_t* virtualPointers;
	PakAsset_t* assetEntries;

	PakPage_t* guidDescriptors;
	uint32_t* fileRelations;

	char gap5E0[32];

	PakPatchDataHeader_t* patchDataHeader;
	PakAsset_t** ppAssetEntries;

	int someAssetCount;
	int numShiftedPointers;

	// array of sizes/offsets in the SF_HEAD segment buffer
	__int64 unkAssetTypeBindingSizes[PAK_MAX_TYPES];

	const char* fileName;
	PakFileHeader_t pakHeader;
};

struct PakFile_t
{
	int numProcessedPointers;
	uint32_t processedAssetCount;
	int processedPageCount;
	int firstPageIdx;

	uint32_t patchCount;
	uint32_t dword14;

	PakFileStream_t fileStream;
	uint64_t inputBytePos;
	uint8_t byte1F8;
	char gap1F9[4];
	uint8_t byte1FD;
	bool isOffsetted_MAYBE;
	bool isCompressed;
	PakDecoder_t pakDecoder;
	uint8_t* decompBuffer;
	size_t maxCopySize;
	uint64_t qword298;

	PakMemoryData_t memoryData;

	inline const char* GetName() const
	{
		return memoryData.fileName;
	}

	inline const PakFileHeader_t& GetHeader() const
	{
		return memoryData.pakHeader;
	}

	inline uint32_t GetAssetCount() const
	{
		return GetHeader().assetCount;
	}

	inline uint32_t GetPointerCount() const
	{
		return GetHeader().descriptorCount;
	}

	// --- pages ---
	inline uint16_t GetPageCount() const
	{
		return GetHeader().memPageCount;
	}

	inline bool IsPageOffsetValid(uint32_t index, uint32_t offset) const
	{
		// validate page index
		if (index == UINT32_MAX || index > GetPageCount())
			return false;

		// !TODO! find some other way of validating offset within page
		// commented because otherwise we incorrectly return false a lot.
		// because of pointer shifting, the offset can't be easily validated like this
		// 
		//if (offset == UINT32_MAX || && offset > GetPageSize(index))
		//	return false;

		return true;
	}

	inline const PakPageHeader_t* GetPageHeader(const uint32_t i) const
	{
		assert(i != UINT32_MAX && i < GetPageCount());

		return &memoryData.pageHeaders[i];
	}

	inline uint32_t GetPageSize(const uint32_t i) const
	{
		assert(i != UINT32_MAX && i < GetPageCount());
		return memoryData.pageHeaders[i].dataSize;
	}

	inline void* GetPointerForPageOffset(const uint32_t index, const uint32_t offset) const
	{
		assert(IsPageOffsetValid(index, offset));

		return memoryData.memPageBuffers[index] + offset;
	}

	inline void* GetPointerForPageOffset(const PakPage_t& ptr) const
	{
		assert(IsPageOffsetValid(ptr.index, ptr.offset));

		return memoryData.memPageBuffers[ptr.index] + ptr.offset;
	}

	inline void* GetPointerForPageOffset(const PakPage_t* ptr) const
	{
		assert(IsPageOffsetValid(ptr->index, ptr->offset));

		return memoryData.memPageBuffers[ptr->index] + ptr->offset;
	}

	// --- segments ---
	inline uint16_t GetSegmentCount() const
	{
		return GetHeader().virtualSegmentCount;
	}

	inline const PakSegmentHeader_t* GetSegmentHeader(const uint32_t i) const
	{
		assert(i < GetSegmentCount());

		return &memoryData.segmentHeaders[i];
	}
};

struct UnknownPakStruct_t
{
	uint32_t unk_0;
	int unk_4;
	int unk_8;
	char gap_C[0x9D404];
	int unk_array_9D410[512];
	char gap_9DC04[0x7F800];
};


static_assert(sizeof(UnknownPakStruct_t) == 0x11D410);
static_assert(sizeof(PakFile_t) == 2208); // S3+
static_assert(sizeof(PakLoadedInfo_t) == 184);
static_assert(sizeof(PakDecoder_t) == 136);
static_assert(sizeof(PakPatchFileHeader_t) == 16);

#endif // RTECH_IPACKFILE_H
