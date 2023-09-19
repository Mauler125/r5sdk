#ifndef IFILESYSTEM_H
#define IFILESYSTEM_H

#include <tier0/annotations.h>
#include <tier0/threadtools.h>
#include <tier1/utlvector.h>
#include <tier1/utlstring.h>
#include <public/ipackedstore.h>
#include <public/appframework/IAppSystem.h>

typedef void* FileHandle_t;
typedef void* FileNameHandle_t; // !TODO: Check if this is 4 or 8 bytes (model_t was 4 bytes in mem).
typedef void* FileCacheHandle_t;
typedef void* FileFindHandle_t;

#define FILESYSTEM_INVALID_HANDLE	( FileHandle_t )nullptr

#define GAMEINFOPATH_TOKEN		"|gameinfo_path|"
#define BASESOURCEPATHS_TOKEN	"|all_source_engine_paths|"

//---------------------------------------------------------------------------------
// Purpose: Forward declarations
//---------------------------------------------------------------------------------
class KeyValues;
class CUtlBuffer;

//-----------------------------------------------------------------------------
// Structures used by the interface
//-----------------------------------------------------------------------------

struct FileSystemStatistics
{
	CInterlockedUInt	nReads,
		nWrites,
		nBytesRead,
		nBytesWritten,
		nSeeks;
};

struct FileSystemCacheDescriptor
{
	const char* pszFilePath;
	const char* pszFileName;
	const char* pszFileExt;

	void* pUnk0; // Ptr to ptr of some dynamically allocated buffer.

	int nFileSize;
	int nFileFlags; // Might be wrong.

	void* pUnk1; // Ptr to some data; compressed perhaps?

	int nUnk0;
	int nUnk1;

	int nUnk2;
	int nUnk3;
	int nUnk4;

	int nPadding;
};

struct FileSystemCacheBuffer
{
	byte* pData; // Actual file buffer.
	void* pUnk;
	int nUnk0;
	volatile LONG nLock;
	int nUnk1;
	int nUnk2; // Used to index into arrays.
};

struct FileSystemCache
{
	int nUnk0; // Most of the time, this is set to '1'.
	FileSystemCacheDescriptor* pDescriptor;
	FileSystemCacheBuffer* pBuffer;
};

//-----------------------------------------------------------------------------
// File system allocation functions. Client must free on failure
//-----------------------------------------------------------------------------
typedef void* (*FSAllocFunc_t)(const char* pszFileName, unsigned nBytes);


//-----------------------------------------------------------------------------
// Used to display dirty disk error functions
//-----------------------------------------------------------------------------
typedef void (*FSDirtyDiskReportFunc_t)();


//-----------------------------------------------------------------------------
// Used for FileSystem logging
//-----------------------------------------------------------------------------
typedef void (*FileSystemLoggingFunc_t)(const char* fileName, const char* accessType);


//-----------------------------------------------------------------------------
// Asynchronous support types
//-----------------------------------------------------------------------------
DECLARE_POINTER_HANDLE(FSAsyncControl_t);
DECLARE_POINTER_HANDLE(FSAsyncFile_t);
const FSAsyncFile_t FS_INVALID_ASYNC_FILE = (FSAsyncFile_t)(0x0000ffff);

enum FilesystemMountRetval_t
{
	FILESYSTEM_MOUNT_OK = 0,
	FILESYSTEM_MOUNT_FAILED,
};

// search path filtering
enum PathTypeFilter_t
{
	FILTER_NONE               = 0,	// no filtering, all search path types match
	FILTER_CULLPACK           = 1,	// pack based search paths are culled (maps and zips)
	FILTER_CULLNONPACK        = 2,	// non-pack based search paths are culled
	FILTER_CULLLOCALIZED      = 3,	// Ignore localized paths, assumes CULLNONPACK
	FILTER_CULLLOCALIZED_ANY  = 4,	// Ignore any localized paths
};

// search path querying (bit flags)
enum
{
	PATH_IS_NORMAL      = 0x00, // normal path, not pack based
	PATH_IS_PACKFILE    = 0x01, // path is a pack file
	PATH_IS_MAPPACKFILE = 0x02, // path is a map pack file
	PATH_IS_DVDDEV      = 0x04, // path is the dvddev cache
};
typedef uint32 PathTypeQuery_t;

#define IS_PACKFILE( n ) ( n & ( PATH_IS_PACKFILE | PATH_IS_MAPPACKFILE ) )
#define IS_DVDDEV( n )   ( n & PATH_IS_DVDDEV )

enum class SearchPathAdd_t : int
{
	PATH_ADD_TO_HEAD,         // First path searched
	PATH_ADD_TO_TAIL,         // Last path searched
	PATH_ADD_TO_TAIL_ATINDEX, // First path searched
};

enum class FileWarningLevel_t : int
{
	FILESYSTEM_WARNING = -1,                        // A problem!
	FILESYSTEM_WARNING_QUIET = 0,                   // Don't print anything
	FILESYSTEM_WARNING_REPORTUNCLOSED,              // On shutdown, report names of files left unclosed
	FILESYSTEM_WARNING_REPORTUSAGE,                 // Report number of times a file was opened, closed
	FILESYSTEM_WARNING_REPORTALLACCESSES,           // Report all open/close events to console ( !slow! )
	FILESYSTEM_WARNING_REPORTALLACCESSES_READ,      // Report all open/close/read events to the console ( !slower! )
	FILESYSTEM_WARNING_REPORTALLACCESSES_READWRITE, // Report all open/close/read/write events to the console ( !slower! )
	FILESYSTEM_WARNING_REPORTALLACCESSES_ASYNC      // Report all open/close/read/write events and all async I/O file events to the console ( !slower(est)! )
};

#define FILESYSTEM_MAX_SEARCH_PATHS 128

enum FileMode_t
{
	FM_BINARY,
	FM_TEXT
};

enum FileType_t
{
	FT_NORMAL,
	FT_PACK_BINARY,
	FT_PACK_TEXT,
	FT_MEMORY_BINARY,
	FT_MEMORY_TEXT
};

enum FileSystemSeek_t
{
	FILESYSTEM_SEEK_HEAD = SEEK_SET,
	FILESYSTEM_SEEK_CURRENT = SEEK_CUR,
	FILESYSTEM_SEEK_TAIL = SEEK_END,
};

enum
{
	FILESYSTEM_INVALID_FIND_HANDLE = -1
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
abstract_class IBaseFileSystem
{
public:
	virtual ssize_t			Read(void* pOutput, ssize_t size, FileHandle_t file) = 0;
	virtual ssize_t			Write(void const* pInput, ssize_t size, FileHandle_t file) = 0;

	// if pathID is NULL, all paths will be searched for the file
	virtual FileHandle_t	Open(const char* pFileName, const char* pOptions, const char* pPathID = 0, int64_t unknown = 0) = 0;
	virtual void			Close(FileHandle_t file) = 0;

	virtual void			Seek(FileHandle_t file, ptrdiff_t pos, FileSystemSeek_t seekType) = 0;
	virtual ptrdiff_t		Tell(FileHandle_t file) = 0;
	virtual ssize_t			FSize(const char* pFileName, const char* pPathID = 0) = 0; // Gets optimized away if it isn't named differently or used.
	virtual ssize_t			Size(FileHandle_t file) = 0;

	virtual void			Flush(FileHandle_t file) = 0;
	virtual bool			Precache(const char* pFileName, const char* pPathID = 0) = 0;

	virtual bool			FileExists(const char* pFileName, const char* pPathID = 0) = 0;
	virtual bool			IsFileWritable(char const* pFileName, const char* pPathID = 0) = 0;
	virtual bool			SetFileWritable(char const* pFileName, bool writable, const char* pPathID = 0) = 0;

	virtual long long		GetFileTime(const char* pFileName, const char* pPathID = 0) = 0;

	//--------------------------------------------------------
	// Reads/writes files to utlbuffers. Use this for optimal read performance when doing open/read/close.
	//--------------------------------------------------------
	virtual bool			ReadFile(const char* pFileName, const char* pPath, CUtlBuffer& buf, ssize_t nMaxBytes = 0, ptrdiff_t nStartingByte = 0, FSAllocFunc_t pfnAlloc = NULL) = 0;
	virtual bool			WriteFile(const char* pFileName, const char* pPath, CUtlBuffer& buf) = 0;
	virtual bool			UnzipFile(const char* pFileName, const char* pPath, const char* pDestination) = 0;
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
abstract_class IFileSystem : public IAppSystem, public IBaseFileSystem
{
public:
	//--------------------------------------------------------
	virtual bool			IsSteam() const = 0;

	// Supplying an extra app id will mount this app in addition 
	// to the one specified in the environment variable "steamappid"
	// 
	// If nExtraAppId is < -1, then it will mount that app ID only.
	// (Was needed by the dedicated server b/c the "SteamAppId" env var only gets passed to steam.dll
	// at load time, so the dedicated couldn't pass it in that way).
	virtual	FilesystemMountRetval_t MountSteamContent(int nExtraAppId = -1) = 0;

#if !defined(GAMEDLL_S0) && !defined(GAMEDLL_S1) && !defined (GAMEDLL_S2)
	virtual bool InitFeatureFlags() = 0;
	virtual bool InitFeatureFlags(const char* pszFlagSetFile) = 0;
#endif // !GAMEDLL_S0 || !GAMEDLL_S1 || GAMEDLL_S2

	virtual void AddSearchPath(const char* pPath, const char* pPathID, SearchPathAdd_t addType) = 0;
	virtual bool RemoveSearchPath(const char* pPath, const char* pPathID) = 0;
	virtual void			RemoveAllSearchPaths(void) = 0;              // Remove all search paths (including write path?)
	virtual void			RemoveSearchPaths(const char* szPathID) = 0; // Remove search paths associated with a given pathID
	// This is for optimization. If you mark a path ID as "by request only", then files inside it
	// will only be accessed if the path ID is specifically requested. Otherwise, it will be ignored.
	// If there are currently no search paths with the specified path ID, then it will still
	// remember it in case you add search paths with this path ID.
	virtual void			MarkPathIDByRequestOnly(const char* pPathID, bool bRequestOnly) = 0;
	// converts a partial path into a full path
	virtual const char* RelativePathToFullPath(const char* pFileName, const char* pPathID, char* pLocalPath, ssize_t localPathBufferSize, PathTypeFilter_t pathFilter = FILTER_NONE, PathTypeQuery_t* pPathType = NULL) = 0;
#if IsGameConsole()
	// Given a relative path, gets the PACK file that contained this file and its offset and size. Can be used to prefetch a file to a HDD for caching reason.
	virtual bool            GetPackFileInfoFromRelativePath(const char* pFileName, const char* pPathID, char* pPackPath, ssize_t nPackPathBufferSize, ptrdiff_t& nPosition, ssize_t& nLength) = 0;
#endif
	// Returns the search path, each path is separated by ;s. Returns the length of the string returned
	virtual ssize_t			GetSearchPath(const char* pathID, bool bGetPackFiles, char* pPath, ssize_t nMaxLen) = 0;
	virtual bool			AddPackFile(const char* fullpath, const char* pathID) = 0; 	// interface for custom pack files > 4Gb

	//--------------------------------------------------------
	// File manipulation operations
	//--------------------------------------------------------
	virtual void			RemoveFile(char const* pRelativePath, const char* pathID = 0) = 0;                  // Deletes a file (on the WritePath)
	virtual bool			RenameFile(char const* pOldPath, char const* pNewPath, const char* pathID = 0) = 0; // Renames a file (on the WritePath)
	virtual int				CreateDirHierarchy(const char* path, const char* pathID = 0) = 0;                   // create a local directory structure
	virtual bool			IsDirectory(const char* pFileName, const char* pathID = 0) = 0;                     // File I/O and info
	virtual ssize_t			FileTimeToString(char* pStrip, ssize_t maxCharsIncludingTerminator, long fileTime) = 0; // Returns the string size

	//--------------------------------------------------------
	// Open file operations
	//--------------------------------------------------------

	virtual void			SetBufferSize(FileHandle_t file/*, ssize_t nBytes*/) = 0; // Amos: unsure if this is accurate, no longer takes the second argument?
	virtual bool			IsOk(FileHandle_t file) = 0;
	virtual bool			EndOfFile(FileHandle_t file) = 0;
	virtual char*			ReadLine(char* pOutput, size_t maxChars, FileHandle_t file) = 0;
#if ! defined(SWIG)
	// Don't let SWIG see the PRINTF_FORMAT_STRING attribute or it will complain.
	virtual ssize_t			FPrintf(FileHandle_t file, PRINTF_FORMAT_STRING const char* pFormat, ...) FMTFUNCTION(3, 4) = 0;
#else
	virtual ssize_t			FPrintf(FileHandle_t file, const char* pFormat, ...) FMTFUNCTION(3, 4) = 0;
#endif

	//--------------------------------------------------------
	// Dynamic library operations
	//--------------------------------------------------------
	// load/unload modules
	virtual CSysModule* LoadModule(const char* pFileName, const char* pPathID = 0, bool bValidatedDllOnly = true) = 0;
	virtual void        UnloadModule(CSysModule* pModule) = 0;

	//--------------------------------------------------------
	// File searching operations
	//--------------------------------------------------------
	// FindFirst/FindNext. Also see FindFirstEx.
	virtual const char* FindFirst(const char* pWildCard, FileFindHandle_t* pHandle) = 0;
	virtual const char* FindNext(FileFindHandle_t handle) = 0;
	virtual bool        FindIsDirectory(FileFindHandle_t handle) = 0;
	virtual void        FindClose(FileFindHandle_t handle) = 0;

	// Same as FindFirst, but you can filter by path ID, which can make it faster.
	virtual const char* FindFirstEx(
		const char* pWildCard,
		const char* pPathID,
		FileFindHandle_t* pHandle
	) = 0;

	// Searches for a file in all paths and results absolute path names for the file, works in pack files (zip and vpk) too
	// Lets you search for something like sound/sound.cache and get a list of every sound cache
	virtual void			FindFileAbsoluteList(CUtlVector<CUtlString>& outAbsolutePathNames, const char* pWildCard, const char* pPathID) = 0;

	//--------------------------------------------------------
	// File name and directory operations
	//--------------------------------------------------------

	// FIXME: This method is obsolete! Use RelativePathToFullPath instead!
	// converts a partial path into a full path
	virtual const char* GetLocalPath(const char* pFileName, char* pLocalPath, ssize_t localPathBufferSize) = 0;

	// Returns true on success ( based on current list of search paths, otherwise false if 
	//  it can't be resolved )
	virtual bool			FullPathToRelativePath(const char* pFullpath, char* pRelative, ssize_t maxlen) = 0;

	// Gets the current working directory
	virtual bool			GetCurrentDirectory(char* pDirectory, unsigned int maxlen) = 0; // Last parameter is a DWORD passed to 'GetCurrentDirectoryA()' internally.

	//--------------------------------------------------------
	// Filename dictionary operations
	//--------------------------------------------------------

	virtual FileNameHandle_t	FindOrAddFileName(char const* pFileName) = 0;
	virtual bool				String(const FileNameHandle_t& handle, char* buf, ssize_t buflen) = 0;

	//--------------------------------------------------------
	// Asynchronous file operations
	//--------------------------------------------------------

//--------------- [ !!! AMOS: !!! ALL ASYNC METHODS ARE UNIMPLEMENTED !!! PURECALL !!! ] ---------------//
	//------------------------------------
	// Global operations
	//------------------------------------
	virtual void PureCall0() = 0;
	virtual void PureCall1() = 0;
	virtual void PureCall2() = 0;
	virtual void PureCall3() = 0;
	virtual void PureCall4() = 0;
	virtual void PureCall5() = 0;

	//--------------------------------------------------------
	// Debugging operations
	//--------------------------------------------------------

	// Dump to printf/OutputDebugString the list of files that have not been closed
	virtual void			PrintOpenedFiles(void) = 0;
	virtual void			PrintSearchPaths(void) = 0;

	// output
	virtual void			SetWarningFunc(void (*pfnWarning)(const char* fmt, ...)) = 0;
	virtual void			SetWarningLevel(FileWarningLevel_t level) = 0;
	virtual void			AddLoggingFunc(void (*pfnLogFunc)(const char* fileName, const char* accessType)) = 0;
	virtual void			RemoveLoggingFunc(FileSystemLoggingFunc_t logFunc) = 0;

	virtual __int64 __fastcall sub_14038C240(__int64 a1) = 0;
	virtual __int64 __fastcall sub_14038C380(__int64 a1) = 0;
	virtual __int64 __fastcall sub_14038C400(__int64 a1, __int64 a2) = 0;

	// Returns the file system statistics retrieved by the implementation.  Returns NULL if not supported.
	virtual const FileSystemStatistics* GetFilesystemStatistics() = 0;

	//--------------------------------------------------------
	// Start of new functions after Lost Coast release (7/05)
	//--------------------------------------------------------

	virtual FileHandle_t	OpenEx(const char* pFileName, const char* pOptions, unsigned flags = 0, const char* pathID = 0/*, char** ppszResolvedFilename = NULL*/) = 0;

	// Extended version of read provides more context to allow for more optimal reading
	virtual ssize_t			ReadEx(void* pOutput, ssize_t sizeDest, ssize_t size, FileHandle_t file) = 0;
	virtual ssize_t			ReadFileEx(const char* pFileName, const char* pPath, void** ppBuf, bool bNullTerminate = false, bool bOptimalAlloc = false, ssize_t nMaxBytes = 0, ptrdiff_t nStartingByte = 0, FSAllocFunc_t pfnAlloc = NULL) = 0;

	virtual FileNameHandle_t	FindFileName(char const* pFileName) = 0;

#if defined( TRACK_BLOCKING_IO )
	virtual void			EnableBlockingFileAccessTracking(bool state) = 0;
	virtual bool			IsBlockingFileAccessEnabled() const = 0;

	virtual IBlockingFileItemList* RetrieveBlockingFileAccessInfo() = 0;
#endif

	virtual void SetupPreloadData() = 0;
	virtual void DiscardPreloadData() = 0;

	// Fixme, we could do these via a string embedded into the compiled data, etc...
	enum KeyValuesPreloadType_t
	{
		TYPE_VMT,
		TYPE_SOUNDEMITTER,
		TYPE_SOUNDSCAPE,
		TYPE_SOUNDOPERATORS,
		TYPE_LEVELSETTINGS,
		TYPE_COMMON,
		NUM_PRELOAD_TYPES
	};

	// If the "PreloadedData" hasn't been purged, then this'll try and instance the KeyValues using the fast path of compiled keyvalues loaded during startup.
	// Otherwise, it'll just fall through to the regular KeyValues loading routines
	virtual KeyValues* LoadKeyValues(KeyValuesPreloadType_t type, char const* filename, char const* pPathID = 0) = 0;
	virtual bool		LoadKeyValues(KeyValues& head, KeyValuesPreloadType_t type, char const* filename, char const* pPathID = 0) = 0;

	virtual bool		GetFileTypeForFullPath(char const* pFullPath, wchar_t* buf, size_t bufSizeInBytes) = 0;

	//--------------------------------------------------------
	//--------------------------------------------------------
	virtual bool		ReadToBuffer(FileHandle_t hFile, CUtlBuffer& buf, ssize_t nMaxBytes = 0, FSAllocFunc_t pfnAlloc = NULL) = 0;

	//--------------------------------------------------------
	// Optimal IO operations
	//--------------------------------------------------------
	virtual bool GetOptimalIOConstraints(FileHandle_t hFile, uint64_t* pOffsetAlign, uint64_t* pSizeAlign, uint64_t* pBufferAlign) = 0;
	virtual void* AllocOptimalReadBuffer(ptrdiff_t nOffset = 0/*!!! UNUSED !!!*/, ssize_t nSize = 0) = 0;
	virtual void FreeOptimalReadBuffer(void*) = 0;


	virtual bool __fastcall sub_140383E00(__int64 a2) = 0;
	virtual bool sub_1403836A0() = 0;
	virtual __int64 __fastcall sub_140384310(int a1) = 0;
	virtual __int64 __fastcall CheckVPKMode(int nMode) = 0; // Checks if the VPK mode equals the mode input.


	//--------------------------------------------------------
	// Cache/VPK operations
	//--------------------------------------------------------
	virtual bool ReadFromCache(const char* pPath, FileSystemCache* pCache) = 0;

	virtual bool __fastcall sub_14037FFA0(__int64 a1, unsigned int a2, __int64 a3) = 0;

	virtual void SetVPKCacheModeClient() = 0; // g_nVPKCacheMode = 1;
	virtual void SetVPKCacheModeServer() = 0; // g_nVPKCacheMode = 2;
	virtual bool IsVPKCacheEnabled() = 0;     // g_nVPKCacheMode != 0;

	virtual __int64 __fastcall PrecacheTaskItem(__int64 a1) = 0;

	virtual void ResetItemCacheSize(int edx) = 0;
	virtual void __fastcall sub_140380100(__int64 a1) = 0;
	virtual void __fastcall sub_140380230(char a2) = 0;
	virtual void* __fastcall sub_1403801F0(const void* a1, unsigned int a2) = 0;
	virtual void __fastcall sub_140380220(__int64 a1) = 0;
	virtual bool ResetItemCache() = 0;
	virtual char __fastcall sub_1403836D0(int a1, char* a2, unsigned int a3) = 0;
	virtual __int64 __fastcall sub_140383840(unsigned int a1, __int64 a2, char* a3, unsigned int BufferCount) = 0;
	virtual const char** __fastcall sub_140383760(unsigned int a1) = 0;
	virtual __int64 __fastcall sub_140383A20(const char* a1) = 0;

	virtual VPKData_t* MountVPKFile(const char* pVpkPath) = 0;
	virtual const char* UnmountVPKFile(const char* pBasename) = 0;

	virtual void __fastcall sub_140383370() = 0;
	virtual void __fastcall sub_140383560() = 0;

	virtual unsigned __int64 __fastcall PnpCtxRegQueryInfoKey(__int64 a1, char* a2, unsigned int* a3, unsigned __int64 a4, unsigned __int64 a5, void(__fastcall* a6)(unsigned __int64)) = 0;
	virtual char* sub_1403842B0() = 0;
	virtual __int64 __fastcall sub_1403842C0(__int64 a1, unsigned int a2, __int64 a3) = 0;

	virtual char __fastcall LoadMainVPK(const char* pszVPKFile) = 0;

	virtual __int64 sub_140380080() = 0;
	virtual char __fastcall sub_14038B530(const char* a1, unsigned __int8* a2, char* a3, __int64 Count) = 0;
	virtual __int64 __fastcall sub_14038C830(unsigned __int16* a1) = 0;
	virtual __int64 __fastcall sub_140388360(char* a1, __int64 a2) = 0;
	virtual __int64 __fastcall sub_140384C60(char* a1, unsigned __int64 a2) = 0;
	virtual void __fastcall sub_140382A80() = 0;
	virtual __int64 __fastcall sub_14038CC90(int a1, unsigned int a2, __int64 a3, __int64 a4) = 0;
	virtual __int64 __fastcall UserMathErrorFunction() = 0;
};

#endif // IFILESYSTEM_H