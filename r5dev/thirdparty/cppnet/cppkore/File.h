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
		static void Copy(const String& SourceFileName, const String& DestinationFileName, bool OverWrite = false);
		// Deletes a file permanently
		static void Delete(const String& FilePath);
		// Decrypts a file from a NTFS volume
		static void Decrypt(const String& FilePath);
		// Encrypts a file from a NTFS volume
		static void Encrypt(const String& FilePath);
		// Whether or not the file exists
		static bool Exists(const String& FilePath);
		// Moves an existing file to a new location, overwriting if specified
		static void Move(const String& SourceFileName, const String& DestinationFileName, bool OverWrite = false);

		// Creates a file in a particular path with ReadWrite access
		static std::unique_ptr<FileStream> Create(const String& FilePath);
		// Opens a file in a particular path with the given mode
		static std::unique_ptr<FileStream> Open(const String& FilePath, FileMode Mode);
		// Opens a file in a particular path with the given mode
		static std::unique_ptr<FileStream> Open(const String& FilePath, FileMode Mode, FileAccess Access);
		// Opens a file in a particular path with the given mode
		static std::unique_ptr<FileStream> Open(const String& FilePath, FileMode Mode, FileAccess Access, FileShare Share);
		// Opens a file in a particular path for reading
		static std::unique_ptr<FileStream> OpenRead(const String& FilePath);
		// Opens a file in a particular path for writing
		static std::unique_ptr<FileStream> OpenWrite(const String& FilePath);
		// Opens or creates a file in a particular path for appending
		static std::unique_ptr<FileStream> OpenAppend(const String& FilePath);

		// Reads the entire specified file as a text
		static String ReadAllText(const String& FilePath);
		// Reads the entire specified file as bytes
		static List<uint8_t> ReadAllBytes(const String& FilePath);
		// Reads all the lines in the specified text file
		static List<String> ReadAllLines(const String& FilePath);

		// Writes the specified text to the a file
		static void WriteAllText(const String& FilePath, const String& Text);
		// Writes the specified bytes to the file
		static void WriteAllBytes(const String& FilePath, const List<uint8_t>& Bytes);
		// Writes the specified bytes to the file
		static void WriteAllBytes(const String& FilePath, const uint8_t* Bytes, uint64_t Count);
		// Writes the specified lines to the file
		static void WriteAllLines(const String& FilePath, const List<String>& Lines);
	};
}