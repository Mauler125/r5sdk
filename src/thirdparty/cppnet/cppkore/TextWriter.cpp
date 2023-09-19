#include "stdafx.h"
#include "TextWriter.h"

namespace IO
{
	void TextWriter::WriteLine()
	{
		Write(&TextWriter::NewLine[0], 0, 2);
	}

	void TextWriter::WriteLine(const char Value)
	{
		Write(Value);
		WriteLine();
	}

	void TextWriter::WriteLine(const char* Buffer, uint32_t Index, uint32_t Count)
	{
		Write(Buffer, Index, Count);
		WriteLine();
	}
}