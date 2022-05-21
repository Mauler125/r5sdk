#pragma once

#include <cstdint>
#include <memory>
#include "Stream.h"
#include "ListBase.h"
#include "ZipEntry.h"

namespace Compression
{
	// Manages compressed Zip archives and decompresses entries.
	class ZipArchive
	{
	public:
		ZipArchive();
		ZipArchive(std::unique_ptr<IO::Stream> Stream);
		ZipArchive(std::unique_ptr<IO::Stream> Stream, bool LeaveOpen);
		ZipArchive(IO::Stream* Stream);
		ZipArchive(IO::Stream* Stream, bool LeaveOpen);
		~ZipArchive();

		// TODO: RemoveEntry();
		// TODO: AddDirectory();

		// Read all the file records in the central directory
		List<ZipEntry> ReadEntries();

		// Add a file into the ZipArchive.
		ZipEntry AddFile(ZipCompressionMethod Method, const string& Path, const string& FileNameInZip, const string& Comment = "");
		// Add a stream into the ZipArchive.
		ZipEntry AddStream(ZipCompressionMethod Method, const string& FileNameInZip, IO::Stream* Stream, const string& Comment = "");

		// Extract the entry to the provided file path.
		void ExtractFile(ZipEntry& Entry, const string& FileName);
		// Extract the entry to the provided stream.
		void ExtractFile(ZipEntry& Entry, IO::Stream* Stream);
		// Extract the entry to the provided stream.
		void ExtractFile(ZipEntry& Entry, std::unique_ptr<IO::Stream>& Stream);

		// Get the underlying stream
		IO::Stream* GetBaseStream() const;
		// Close the ZipArchive and underlying stream
		void Close();

	private:
		std::unique_ptr<IO::Stream> BaseStream;
		std::unique_ptr<uint8_t[]> _CentralDir;
		uint64_t _CentralDirLength;
		uint64_t _ExistingFiles;
		bool _LeaveOpen;

		// Internal helper routine to parse the end of central directory record.
		void ReadFileInfo();
		// Internal helper routine to calculate file offset.
		uint32_t GetFileOffset(uint32_t HeaderOffset);
		// Internal helper routine to read extra info.
		void ReadExtraInfo(uint64_t i, ZipEntry& Entry);
		// Internal helper routine to clean a file name
		string NormalizeFileName(const string& FileName);
		// Internal routine to calculate crc and sizes
		void UpdateCrcAndSizes(ZipEntry& Entry);
		// Internal routine to write the file header
		void WriteLocalHeader(ZipEntry& Entry);
		// Internal routine to create extra info
		void CreateExtraInfo(ZipEntry& Entry, IO::Stream* Output);
		// Internal routine to get maximum 32bit size
		uint32_t Get32BitSize(uint64_t Size);
	};
}