#include "stdafx.h"
#include "TextReader.h"

namespace IO
{
	int32_t TextReader::Read(char* Buffer, uint32_t Index, uint32_t Count)
	{
		uint32_t n = 0;

		do
		{
			auto Ch = Read();
			if (Ch == -1)
				break;

			Buffer[Index + n++] = (char)Ch;

		} while (n < Count);

		return n;
	}

	string TextReader::ReadToEnd()
	{
		char Buffer[4097]{};
		uint32_t Length = 0;
		auto Result = string();

		while ((Length = Read(Buffer, 0, 4096)) != 0)
			Result.Append(Buffer, Length);

		return std::move(Result);
	}

	string TextReader::ReadLine()
	{
		auto Result = string();

		while (true)
		{
			auto Ch = Read();
			if (Ch == -1)
				break;

			if (Ch == '\r' || Ch == '\n')
			{
				if (Ch == '\r' && Peek() == '\n')
					Read();

				return Result;
			}

			Result.Append((char)Ch);
		}

		return std::move(Result);
	}
}