#ifndef IFILE_H
#define IFILE_H

#include "public/ifilesystem.h"

//-----------------------------------------------------------------------------
// Per-file worker classes
//-----------------------------------------------------------------------------
abstract_class CStdFilesystemFile
{
public:
	virtual ~CStdFilesystemFile() {}
	virtual void FS_setbufsize(unsigned nBytes) = 0;
	virtual void FS_fclose() = 0;
	virtual void FS_fseek(int64 pos, int seekType) = 0;
	virtual long FS_ftell() = 0;
	virtual int FS_feof() = 0;
	virtual size_t FS_fread(void* dest, size_t destSize, size_t size) = 0;
	virtual size_t FS_fwrite(const void* src, size_t size) = 0;
	virtual bool FS_setmode(FileMode_t mode) = 0;
	virtual size_t FS_vfprintf(const char* fmt, va_list list) = 0;
	virtual int FS_ferror() = 0;
	virtual int FS_fflush() = 0;
	virtual char* FS_fgets(char* dest, int destSize) = 0;
	virtual int FS_GetSectorSize() { return 1; }
};

//---------------------------------------------------------

class CStdioFile : public CStdFilesystemFile
{
public:
	// [ !!! IMPLEMENTED IN ENGINE !!! ]
	static CStdioFile* FS_fopen(const char* filename, const char* options, int64* size);

	virtual void FS_setbufsize(unsigned nBytes) = 0;
	virtual void FS_fclose() = 0;
	virtual void FS_fseek(int64 pos, int seekType) = 0;
	virtual long FS_ftell() = 0;
	virtual int FS_feof() = 0;
	virtual size_t FS_fread(void* dest, size_t destSize, size_t size) = 0;
	virtual size_t FS_fwrite(const void* src, size_t size) = 0;
	virtual bool FS_setmode(FileMode_t mode) = 0;
	virtual size_t FS_vfprintf(const char* fmt, va_list list) = 0;
	virtual int FS_ferror() = 0;
	virtual int FS_fflush() = 0;
	virtual char* FS_fgets(char* dest, int destSize) = 0;

#ifdef POSIX
	static CUtlMap< ino_t, CThreadMutex* > m_LockedFDMap;
	static CThreadMutex	m_MutexLockedFD;
#endif
private:
	CStdioFile(FILE* pFile, bool bWriteable)
		: m_pFile(pFile), m_bWriteable(bWriteable)
	{
	}

	FILE* m_pFile;
	bool m_bWriteable;
};

#endif // IFILE_H
