#include "core/stdafx.h"
#include <tier0/memstd.h>
#include "tier1/utlbuffer.h"
#include <filesystem/filesystem.h>
#include "vpc/rson.h"

RSON::Node_t* RSON::LoadFromBuffer(const char* pszBufferName, char* pBuffer, RSON::eFieldType rootType)
{
	return RSON_LoadFromBuffer(pszBufferName, pBuffer, rootType, 0, NULL);
}

RSON::Node_t* RSON::LoadFromFile(const char* pszFilePath, const char* pPathID)
{
	FileHandle_t file = FileSystem()->Open(pszFilePath, "rt", pPathID);

	if (!file)
		return NULL;

	unsigned int nFileSize = FileSystem()->Size(file);
	std::unique_ptr<char[]> fileBuf(new char[nFileSize + 1]);

	int nRead = FileSystem()->Read(fileBuf.get(), nFileSize, file);
	FileSystem()->Close(file);

	fileBuf[nRead] = '\0';

	RSON::Node_t* node = RSON::LoadFromBuffer(pszFilePath, fileBuf.get(), eFieldType::RSON_OBJECT);
	return node;
}