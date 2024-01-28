//=============================================================================//
//
// Purpose: FileSystem class utilizing standard libraries.
// ----------------------------------------------------------------------------
// NOTE: use this for standalone/tools projects
//=============================================================================//
#include "tier0/utility.h"
#include "filesystem_std.h"

ssize_t CBaseFileSystem::Read(void* pOutput, ssize_t size, FileHandle_t file)
{
	return fread(pOutput, sizeof(uint8_t), size, (FILE*)file);
}

ssize_t CBaseFileSystem::Write(void const* pInput, ssize_t size, FileHandle_t file)
{
	return fwrite(pInput, sizeof(uint8_t), size, (FILE*)file);
}

FileHandle_t CBaseFileSystem::Open(const char* pFileName, const char* pOptions, const char* pPathID, int64_t unknown)
{
	NOTE_UNUSED(unknown);

	char fullPath[1024];
	snprintf(fullPath, sizeof(fullPath), "%s", pFileName);

	V_FixSlashes(fullPath);

	return (FileHandle_t)fopen(fullPath, pOptions);
}

void CBaseFileSystem::Close(FileHandle_t file)
{
	fclose((FILE*)file);
}

void CBaseFileSystem::Seek(FileHandle_t file, ssize_t pos, FileSystemSeek_t seekType)
{
	fseek((FILE*)file, (long)pos, seekType);
}

ptrdiff_t CBaseFileSystem::Tell(FileHandle_t file)
{
	return ftell((FILE*)file);
}

ssize_t CBaseFileSystem::FSize(const char* pFileName, const char* pPathID)
{
	char fullPath[1024];
	snprintf(fullPath, sizeof(fullPath), "%s", pFileName);

	V_FixSlashes(fullPath);

	FILE* fp = fopen(fullPath, "rb");
	if (!fp)
		return 0;

	fseek(fp, 0, SEEK_END);
	ptrdiff_t size = Tell(fp);

	fclose(fp);
	return size;
}

ssize_t CBaseFileSystem::Size(FileHandle_t file)
{
	fseek((FILE*)file, 0, SEEK_END);
	ptrdiff_t size = Tell((FILE*)file);

	fseek((FILE*)file, 0, SEEK_SET);
	return size;
}

void CBaseFileSystem::Flush(FileHandle_t file)
{
	fflush((FILE*)file);
}

bool CBaseFileSystem::Precache(const char* pFileName, const char* pPathID)
{
	NOTE_UNUSED(pPathID);
	NOTE_UNUSED(pFileName);
	return true;
}

bool CBaseFileSystem::FileExists(const char* pFileName, const char* pPathID)
{
	char fullPath[1024];
	snprintf(fullPath, sizeof(fullPath), "%s", pFileName);

	V_FixSlashes(fullPath);

	FILE* fp = fopen(fullPath, "rb");
	if (!fp)
		return false;

	fclose(fp);
	return true;
}

bool CBaseFileSystem::IsFileWritable(char const* pFileName, const char* pPathID)
{
	char fullPath[1024];
	snprintf(fullPath, sizeof(fullPath), "%s", pFileName);

	V_FixSlashes(fullPath);

	FILE* fp = fopen(fullPath, "a");
	if (!fp)
		return false;

	fclose(fp);
	return true;
}

bool CBaseFileSystem::SetFileWritable(char const* pFileName, bool writable, const char* pPathID)
{
	NOTE_UNUSED(pPathID);
	NOTE_UNUSED(writable);
	NOTE_UNUSED(pFileName);
	return true;
}

long long CBaseFileSystem::GetFileTime(const char* pFileName, const char* pPathID)
{
	char fullPath[1024];
	snprintf(fullPath, sizeof(fullPath), "%s", pFileName);

	V_FixSlashes(fullPath);

	struct stat result;

	// On POSIX systems, st_mtime is the time of last
	// modification in seconds since the epoch.
	if (stat(fullPath, &result) == NULL)
		return (long)result.st_mtime;
	else
		return -1;
}

bool CBaseFileSystem::GetOptimalIOConstraints(FileHandle_t hFile, uint64_t* pOffsetAlign, uint64_t* pSizeAlign, uint64_t* pBufferAlign)
{
	if (pOffsetAlign)
		*pOffsetAlign = 1;
	if (pSizeAlign)
		*pSizeAlign = 1;
	if (pBufferAlign)
		*pBufferAlign = 1;
	return false;
}

bool CBaseFileSystem::ReadToBuffer(FileHandle_t hFile, CUtlBuffer& buf, ssize_t nMaxBytes, FSAllocFunc_t pfnAlloc)
{
	ssize_t nBytesToRead = Size(hFile);
	if (nBytesToRead == 0)
	{
		// no data in file
		return true;
	}

	if (nMaxBytes > 0)
	{
		// can't read more than file has
		nBytesToRead = MIN(nMaxBytes, nBytesToRead);
	}

	ssize_t nBytesRead = 0;
	ptrdiff_t nBytesOffset = 0;

	ptrdiff_t iStartPos = Tell(hFile);

	if (nBytesToRead != 0)
	{
		ssize_t nBytesDestBuffer = nBytesToRead;
		uint64_t nSizeAlign = 0, nBufferAlign = 0, nOffsetAlign = 0;

		bool bBinary = !(buf.IsText() && !buf.ContainsCRLF());

		if (bBinary && !IsPosix() && !buf.IsExternallyAllocated() && !pfnAlloc &&
			(buf.TellPut() == 0) && (buf.TellGet() == 0) && (iStartPos % 4 == 0) &&
			GetOptimalIOConstraints(hFile, &nOffsetAlign, &nSizeAlign, &nBufferAlign))
		{
			// correct conditions to allow an optimal read
			if (iStartPos % nOffsetAlign != 0)
			{
				// move starting position back to nearest alignment
				nBytesOffset = (iStartPos % nOffsetAlign);
				Assert((iStartPos - nBytesOffset) % nOffsetAlign == 0);
				Seek(hFile, -nBytesOffset, FILESYSTEM_SEEK_CURRENT);

				// going to read from aligned start, increase target buffer size by offset alignment
				nBytesDestBuffer += nBytesOffset;
			}

			// snap target buffer size to its size alignment
			// add additional alignment slop for target pointer adjustment
			nBytesDestBuffer = AlignValue(nBytesDestBuffer, nSizeAlign) + nBufferAlign;
		}

		AssertMsg(!pfnAlloc, "Custom allocators not yet supported!");

		//if (!pfnAlloc)
		{
			buf.EnsureCapacity(nBytesDestBuffer + buf.TellPut());
		}
		//else
		//{
		//	// caller provided allocator
		//	void* pMemory = (*pfnAlloc)(g_pszReadFilename, nBytesDestBuffer);
		//	buf.SetExternalBuffer(pMemory, nBytesDestBuffer, 0, buf.GetFlags() & ~CUtlBuffer::EXTERNAL_GROWABLE);
		//}

		ssize_t seekGet = -1;
		if (nBytesDestBuffer != nBytesToRead)
		{
			// doing optimal read, align target pointer
			ssize_t nAlignedBase = AlignValue((byte*)buf.Base(), nBufferAlign) - (byte*)buf.Base();
			buf.SeekPut(CUtlBuffer::SEEK_HEAD, nAlignedBase);

			// the buffer read position is slid forward to ignore the addtional
			// starting offset alignment
			seekGet = nAlignedBase + nBytesOffset;
		}

		nBytesRead = ReadEx(buf.PeekPut(), nBytesDestBuffer - nBufferAlign, nBytesToRead + nBytesOffset, hFile);
		buf.SeekPut(CUtlBuffer::SEEK_CURRENT, nBytesRead);

		if (seekGet != -1)
		{
			// can only seek the get after data has been put, otherwise buffer sets overflow error
			buf.SeekGet(CUtlBuffer::SEEK_HEAD, seekGet);
		}

		Seek(hFile, iStartPos + (nBytesRead - nBytesOffset), FILESYSTEM_SEEK_HEAD);
	}

	return (nBytesRead != 0);
}

bool CBaseFileSystem::ReadFile(const char* pFileName, const char* pPath, CUtlBuffer& buf, ssize_t nMaxBytes, ptrdiff_t nStartingByte, FSAllocFunc_t pfnAlloc)
{
	//CHECK_DOUBLE_SLASHES(pFileName);

	bool bBinary = !(buf.IsText() && !buf.ContainsCRLF());

	FileHandle_t fp = Open(pFileName, (bBinary) ? "rb" : "rt", pPath);
	if (!fp)
		return false;

	if (nStartingByte != 0)
	{
		Seek(fp, nStartingByte, FILESYSTEM_SEEK_HEAD);
	}

	AssertMsg(!pfnAlloc, "Custom allocators not yet supported!");

	//if (pfnAlloc)
	//{
	//	g_pszReadFilename = (char*)pFileName;
	//}

	bool bSuccess = ReadToBuffer(fp, buf, nMaxBytes, pfnAlloc);

	Close(fp);

	return bSuccess;
}

bool CBaseFileSystem::WriteFile(const char* pFileName, const char* pPath, CUtlBuffer& buf)
{
	const char* pWriteFlags = "wb";
	if (buf.IsText() && !buf.ContainsCRLF())
	{
		pWriteFlags = "wt";
	}

	FileHandle_t fp = Open(pFileName, pWriteFlags, pPath);
	if (!fp)
		return false;

	ssize_t nBytesWritten = Write(buf.Base(), buf.TellMaxPut(), fp);

	Close(fp);
	return (nBytesWritten != 0);
}

ssize_t CBaseFileSystem::ReadEx(void* pOutput, ssize_t /*destSize*/, ssize_t size, FileHandle_t file)
{
	if (!file)
	{
		assert(0); // Tried to Read NULL file handle!
		return 0;
	}
	if (size < 0)
	{
		return 0;
	}

	const ssize_t nRet = fread(pOutput, sizeof(uint8_t), size, (FILE*)file);
	return nRet;

}

int CBaseFileSystem::CreateDirHierarchy(const char* pPath, const char* pPathID)
{
	NOTE_UNUSED(pPathID);
	return ::CreateDirHierarchy(pPath);
}

bool CBaseFileSystem::IsDirectory(const char* pPath, const char* pPathID)
{
	NOTE_UNUSED(pPathID);
	return ::IsDirectory(pPath);
}

char* CBaseFileSystem::ReadLine(char* maxChars, ssize_t maxOutputLength, FileHandle_t file)
{
	return fgets(maxChars, (int)maxOutputLength, (FILE*)file);
}

CUtlString CBaseFileSystem::ReadString(FileHandle_t pFile)
{
	CUtlString result;
	char c = '\0';

	do
	{
		Read(&c, sizeof(char), pFile);

		if (c)
			result += c;

	} while (c);

	return result;
}
