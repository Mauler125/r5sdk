//=============================================================================//
// 
// Purpose: texture streaming and runtime management
// 
//-----------------------------------------------------------------------------
// Some of these structs are based on the presentation held by the developer of
// the texture streaming system in Titanfall 2 and Apex Legends, see the links:
// - https://www.gdcvault.com/play/1024418/Efficient-Texture-Streaming-in-Titanfall
// - https://www.youtube.com/watch?v=q0aKNGH8WbA
//=============================================================================//

#ifndef TEXTURESTREAMING_H
#define TEXTURESTREAMING_H
#include "public/rtech/istreamdb.h"

#define TEXTURE_MAX_STREAMING_TEXTURE_HANDLES 0x4000 // Used for TextureStreamMgr_s::streamingTextures (deprecated).
#define TEXTURE_MAX_STREAMING_TEXTURE_HANDLES_NEW 0x8000 // Used for s_streamingTextureHandles.

struct MaterialGlue_s;
struct TextureAsset_s;

struct TextureStreamMgr_Task_s
{
	TextureAsset_s* textureAsset;

	// The mip level count to load or drop.
	uint8 mipLevelCount;
	char padding[3];

	// The 'cost vs benefit' metric used to partially sort the task list to get
	// the best and worst 16 textures.
	float metric;
};

struct TextureStreamMgr_TaskList_s
{
	// STBSP async file handle and index to the current page.
	int fileHandle;
	int pageIndex;

	// Whether we should update the current page state.
	bool updatePageState;
	int padding;

	// Offset to the page in the STBSP to read up to size bytes.
	uint64 pageOffset;
	uint64 pageSize;

	// - loadBegin points to the first texture load task.
	// - loadEnd points to the last texture load task.
	// - loadLimit points to the absolute end of the load task buffer.
	TextureStreamMgr_Task_s* loadBegin;
	TextureStreamMgr_Task_s* loadEnd;
	TextureStreamMgr_Task_s* loadLimit;

	// - dropBegin points to the first texture drop task.
	// - dropEnd points to the last texture drop task.
	// - dropLimit points to the absolute end of the drop task buffer.
	TextureStreamMgr_Task_s* dropBegin;
	TextureStreamMgr_Task_s* dropEnd;
	TextureStreamMgr_Task_s* dropLimit;
};

enum TextureStreamMode_e : uint8
{
	TSM_OPMODE_LEGACY_PICMIP = 0,
	TSM_OPMODE_DYNAMIC,
	TSM_OPMODE_ALL,
	TSM_OPMODE_NONE,
	TSM_OPMODE_PAUSED,
};

struct TextureStreamMgr_s
{
	bool initialised;
	bool hasResidentPages;
	char filePath[260]; // size=MAX_PATH.
	char gap_105[2];
	int fileHandle; // STBSP file handle.
	char gap_10b[4];
	char* stringBuffer;
	StreamDB_Header_s header;
	StreamDB_ResidentPage_s* residentPages;
	MaterialGlue_s** materials;
	StreamDB_Material_s* materialInfo;
	int64 maxResidentPageSize;
	StreamDB_PageState_s pageStates[4];
	bool unk_320;
	char gap_321[3];
	TextureStreamMode_e texStreamMode;
	int picMip;
	float streamBspBucketBias;
	float streamBspDistScale;
	uint64 highPriorityMemoryBudget;
	uint32 streamBspCellX;
	uint32 streamBspCellY;
	int loadedLinkedTextureCount;
	int totalMipLevelCount;
	int loadedMipLevelCount;
	int unk_34;
	int64 usedStreamingMemory;
	int64 totalStreamingMemory;
	int thisFrame;
	int unk_50;
	Vector3D streamBspCameraPos;
	float streamBspHalfFovX;
	float streamBspViewWidth;

	union
	{
		// Points to the new streaming texture array 's_streamingTextureHandles'.
		// Which has a size of TEXTURE_MAX_STREAMING_TEXTURE_HANDLES_NEW See
		// 's_streamingTextureHandles' for more information regarding this change.
		TextureAsset_s** pStreamingTextures;

		// This is the old streaming texture array, which has been replaced by
		// 's_streamingTextureHandles'. It is no longer used, but its still in
		// this struct as this array is still in the static runtime data of the
		// game executable and we must dedicate this space to this array in
		// order to maintain correct offsets for all the members of this struct
		// that come after this array member.
		TextureAsset_s* streamingTextures[TEXTURE_MAX_STREAMING_TEXTURE_HANDLES];
	};

	uint32_t numLoadedStreamingTextures;
	void* unkHandle0;
	void* unkHandle1;
	void* unkHandle2;
	void* unkHandle3;
	RTL_SRWLOCK textureStreamMgrMutex;
};

enum TextureStreamMemory_e
{
	TML_TRACKER_UNFREE,

	TML_TRACKER_UNKNOWN_1, // Appears unused by the retail runtime.
	TML_TRACKER_UNKNOWN_2, // Appears unused by the retail runtime.

	TML_TRACKER_UNUSABE,

	// Not a memory tracker!
	STREAMING_TEXTURES_MEMORY_LATENCY_FRAME_COUNT,
};

inline void(*v_StreamDB_Init)(const char* const pszLevelName);
inline void(*v_StreamDB_CreditWorldTextures)(TextureStreamMgr_TaskList_s* const taskList);
inline void(*v_StreamDB_CreditWorldTextures_Legacy)(TextureStreamMgr_TaskList_s* const taskList);
inline void(*v_StreamDB_CreditModelTextures)(TextureAsset_s** const textureAssets, const int textureCount, __int64 a3, __int64 a4, unsigned int a5, const Vector3D* const pViewOrigin, const float tanOfHalfFov, const float viewWidthPixels, int a9);

inline void(*TextureStreamMgr_GetStreamOverlay)(const char* const mode, char* const buf, const size_t bufSize);
inline const char* (*TextureStreamMgr_DrawStreamOverlayToInterface)(void* thisptr, uint8_t* a2, void* unused, void* debugOverlayIface);

inline ssize_t* g_textureStreamMemoryUsed = nullptr; // array size = STREAMING_TEXTURES_MEMORY_LATENCY_FRAME_COUNT.
inline ssize_t* g_textureStreamMemoryTarget = nullptr; // pointer to single size var.

inline TextureStreamMgr_s* s_textureStreamMgr;

///////////////////////////////////////////////////////////////////////////////
class VTextureStreaming : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("StreamDB_Init", v_StreamDB_Init);
		LogFunAdr("StreamDB_CreditWorldTextures", v_StreamDB_CreditWorldTextures);
		LogFunAdr("StreamDB_CreditWorldTextures_Legacy", v_StreamDB_CreditWorldTextures_Legacy);

		LogFunAdr("StreamDB_CreditModelTextures", v_StreamDB_CreditModelTextures);

		LogFunAdr("TextureStreamMgr_GetStreamOverlay", TextureStreamMgr_GetStreamOverlay);
		LogFunAdr("TextureStreamMgr_DrawStreamOverlayToInterface", TextureStreamMgr_DrawStreamOverlayToInterface);

		LogVarAdr("g_textureStreamMemoryUsed", g_textureStreamMemoryUsed);
		LogVarAdr("g_textureStreamMemoryTarget", g_textureStreamMemoryTarget);

		LogVarAdr("s_textureStreamMgr", s_textureStreamMgr);
	}
	virtual void GetFun(void) const
	{
		Module_FindPattern(g_GameDll, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 54 41 56 41 57 48 83 EC 40 48 8B E9").GetPtr(v_StreamDB_Init);

		Module_FindPattern(g_GameDll, "E8 ?? ?? ?? ?? EB ?? 48 8B CF E8 ?? ?? ?? ?? 4C 8D 25").FollowNearCallSelf().GetPtr(v_StreamDB_CreditWorldTextures);
		Module_FindPattern(g_GameDll, "E8 ?? ?? ?? ?? 4C 8D 25 ?? ?? ?? ?? 4C 89 64 24").FollowNearCallSelf().GetPtr(v_StreamDB_CreditWorldTextures_Legacy);

		Module_FindPattern(g_GameDll, "4C 89 44 24 ?? 89 54 24 ?? 48 89 4C 24 ?? 55 56").GetPtr(v_StreamDB_CreditModelTextures);

		Module_FindPattern(g_GameDll, "E8 ?? ?? ?? ?? 80 7C 24 ?? ?? 0F 84 ?? ?? ?? ?? 48 89 9C 24 ?? ?? ?? ??").FollowNearCallSelf().GetPtr(TextureStreamMgr_GetStreamOverlay);
		Module_FindPattern(g_GameDll, "41 56 B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 C6 02 ??").GetPtr(TextureStreamMgr_DrawStreamOverlayToInterface);
	}
	virtual void GetVar(void) const
	{
		CMemory(TextureStreamMgr_DrawStreamOverlayToInterface).Offset(0x2D).FindPatternSelf("48 8B 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr(g_textureStreamMemoryUsed);
		CMemory(TextureStreamMgr_DrawStreamOverlayToInterface).Offset(0x1C).FindPatternSelf("48 8B 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr(g_textureStreamMemoryTarget);

		CMemory(v_StreamDB_Init).FindPattern("C6 05").ResolveRelativeAddressSelf(0x2, 0x7).GetPtr(s_textureStreamMgr);
	}
	virtual void GetCon(void) const
	{ }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // TEXTURESTREAMING_H
