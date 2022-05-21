#pragma once

#include <memory>
#include "IOError.h"
#include "ListBase.h"
#include "StringBase.h"
#include "FileStream.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"

//
// Remove Win32 macros
//

namespace IO
{
	class File
	{
	public:
		// Copies an existing file to a new file, overwriting if specified
		static void Copy(const string& SourceFileName, const string& DestinationFileName, bool OverWrite = false);
		// Deletes a file permanently
		static void Delete(const string& FilePath);
		// Decrypts a file from a NTFS volume
		static void Decrypt(const string& FilePath);
		// Encrypts a file from a NTFS volume
		static void Encrypt(const string& FilePath);
		// Whether or not the file exists
		static bool Exists(const string& FilePath);
		// Moves an existing file to a new location, overwriting if specified
		static void Move(const string& SourceFileName, const string& DestinationFileName, bool OverWrite = false);

		// Creates a file in a particular path with ReadWrite access
		static std::unique_ptr<FileStream> Create(const string& FilePath);
		// Opens a file in a particular path with the given mode
		static std::unique_ptr<FileStream> Open(const string& FilePath, FileMode Mode);
		// Opens a file in a particular path with the given mode
		static std::unique_ptr<FileStream> Open(const string& FilePath, FileMode Mode, FileAccess Access);
		// Opens a file in a particular path with the given mode
		static std::unique_ptr<FileStream> Open(const string& FilePath, FileMode Mode, FileAccess Access, FileShare Share);
		// Opens a file in a particular path for reading
		static std::unique_ptr<FileStream> OpenRead(const string& FilePath);
		// Opens a file in a particular path for writing
		static std::unique_ptr<FileStream> OpenWrite(const string& FilePath);
		// Opens or creates a file in a particular path for appending
		static std::unique_ptr<FileStream> OpenAppend(const string& FilePath);

		// Reads the entire specified file as a text
		static string ReadAllText(const string& FilePath);
		// Reads the entire specified file as bytes
		static List<uint8_t> ReadAllBytes(const string& FilePath);
		// Reads all the lines in the specified text file
		static List<string> ReadAllLines(const string& FilePath);

		// Writes the specified text to the a file
		static void WriteAllText(const string& FilePath, const string& Text);
		// Writes the specified bytes to the file
		static void WriteAllBytes(const string& FilePath, const List<uint8_t>& Bytes);
		// Writes the specified bytes to the file
		static void WriteAllBytes(const string& FilePath, const uint8_t* Bytes, uint64_t Count);
		// Writes the specified lines to the file
		static void WriteAllLines(const string& FilePath, const List<string>& Lines);
	};
}