#include "stdafx.h"
#include "ProcessReader.h"
#include "Pattern.h"

#include "StreamWriter.h"
#include "File.h"

namespace IO
{
	ProcessReader::ProcessReader()
		: ProcessReader(nullptr, false)
	{
	}

	ProcessReader::ProcessReader(const Diagnostics::Process& Process)
		: _LeaveOpen(false)
	{
		this->BaseStream = std::make_unique<ProcessStream>((uint32_t)Process.GetId());
	}

	ProcessReader::ProcessReader(std::unique_ptr<ProcessStream> Stream)
		: ProcessReader(std::move(Stream), false)
	{
	}

	ProcessReader::ProcessReader(std::unique_ptr<ProcessStream> Stream, bool LeaveOpen)
	{
		this->BaseStream = std::move(Stream);
		this->_LeaveOpen = LeaveOpen;
	}

	ProcessReader::ProcessReader(ProcessStream* Stream)
		: ProcessReader(Stream, false)
	{
	}

	ProcessReader::ProcessReader(ProcessStream* Stream, bool LeaveOpen)
	{
		this->BaseStream.reset(Stream);
		this->_LeaveOpen = LeaveOpen;
	}

	ProcessReader::~ProcessReader()
	{
		this->Close();
	}

	std::unique_ptr<uint8_t[]> ProcessReader::Read(uint64_t Count, uint64_t Address, uint64_t& Result)
	{
		auto Buffer = std::make_unique<uint8_t[]>(Count);

		Result = Read(Buffer.get(), 0, Count, Address);

		return Buffer;
	}

	uint64_t ProcessReader::Read(uint8_t* Buffer, uint64_t Index, uint64_t Count, uint64_t Address)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		return this->BaseStream->Read(Buffer, Index, Count, Address);
	}

	uint64_t ProcessReader::Read(void* Buffer, uint64_t Index, uint64_t Count, uint64_t Address)
	{
		return Read((uint8_t*)Buffer, Index, Count, Address);
	}

	string ProcessReader::ReadCString(uint64_t Address)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		char iBuffer[0x100]{};
		auto Result = this->BaseStream->Read((uint8_t*)&iBuffer[0], 0, sizeof(iBuffer), Address);
		auto nCharPos = std::memchr(iBuffer, 0x0, Result);

		if (nCharPos != nullptr)
		{
			return std::move(string(iBuffer, (char*)nCharPos - &iBuffer[0]));
		}
		else if (Result != sizeof(iBuffer))
		{
			return std::move(string(iBuffer));
		}

		Address += 0x100;
		string Buffer(iBuffer, sizeof(iBuffer));

		auto tChar = this->Read<char>(Address++);
		while ((uint8_t)tChar > 0)
		{
			Buffer.Append(tChar);
			tChar = this->Read<char>(Address++);
		}

		return std::move(Buffer);
	}

	string ProcessReader::ReadSizeString(uint64_t Size, uint64_t Address)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		auto Buffer = string((uint32_t)Size, '\0');
		this->BaseStream->Read((uint8_t*)&Buffer[0], 0, Size, Address);

		return std::move(Buffer);
	}

	uint32_t ProcessReader::ReadVarInt(uint64_t Address)
	{
		uint32_t Count = 0, Shift = 0;
		uint8_t Byte;

		do
		{
			if (Shift == 5 * 7)
				return 0;

			Byte = this->Read<uint8_t>(Address++);
			Count |= (Byte & 0x7F) << Shift;
			Shift += 7;
		} while ((Byte & 0x80) != 0);

		return Count;
	}

	int64_t ProcessReader::SignatureScan(const string& Signature, bool ScanAllMemory)
	{
		auto BaseAddress = this->GetBaseAddress();
		auto ScanSize = (ScanAllMemory) ? this->GetMemorySize() : this->GetSizeOfCode();

		return this->SignatureScan(Signature, BaseAddress, ScanSize);
	}

	int64_t ProcessReader::SignatureScan(const string& Signature, uint64_t Address, uint64_t Count)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		auto Sig = Data::Pattern(Signature);
		auto ScanBuffer = std::make_unique<uint8_t[]>(Count);

		uint64_t TotalRead = 0;
		for (uint64_t i = Address; i < (Address + Count); i += ProcessReader::DefaultScanBufferSize)
		{
			auto Want = ((Count - TotalRead) > ProcessReader::DefaultScanBufferSize) ? ProcessReader::DefaultScanBufferSize : (Count - TotalRead);
			this->Read(ScanBuffer.get(), TotalRead, Want, i);
			TotalRead += Want;
		}

		auto SearchResult = Sig.Search(ScanBuffer.get(), 0, Count);
		
		if (SearchResult > -1)
			return (SearchResult + Address);

		return SearchResult;
	}

	List<int64_t> ProcessReader::SignatureScanAll(const string & Signature, bool ScanAllMemory)
	{
		auto BaseAddress = this->GetBaseAddress();
		auto ScanSize = (ScanAllMemory) ? this->GetMemorySize() : this->GetSizeOfCode();

		return this->SignatureScanAll(Signature, BaseAddress, ScanSize);
	}

	List<int64_t> ProcessReader::SignatureScanAll(const string & Signature, uint64_t Address, uint64_t Count)
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		auto Sig = Data::Pattern(Signature);
		auto ScanBuffer = std::make_unique<uint8_t[]>(Count);

		uint64_t TotalRead = 0;
		for (uint64_t i = Address; i < (Address + Count); i += ProcessReader::DefaultScanBufferSize)
		{
			auto Want = ((Count - TotalRead) > ProcessReader::DefaultScanBufferSize) ? ProcessReader::DefaultScanBufferSize : (Count - TotalRead);
			this->Read(ScanBuffer.get(), TotalRead, Want, i);
			TotalRead += Want;
		}

		auto ResultList = Sig.SearchAll(ScanBuffer.get(), 0, Count);

		for (auto& Result : ResultList)
			Result += Address;

		return ResultList;
	}

	bool ProcessReader::IsRunning() const
	{
		if (!this->BaseStream)
			return false;

		return (WaitForSingleObject(this->BaseStream->GetProcessHandle(), 0) == WAIT_TIMEOUT);
	}

	uint64_t ProcessReader::GetBaseAddress() const
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetProcessId(this->BaseStream->GetProcessHandle()));
		if (hSnapshot == INVALID_HANDLE_VALUE)
			return NULL;

		uint64_t Result = 0;
		MODULEENTRY32 mEntry;
		mEntry.dwSize = sizeof(MODULEENTRY32);

		if (!Module32First(hSnapshot, &mEntry))
		{
			CloseHandle(hSnapshot);
			return NULL;
		}

		CloseHandle(hSnapshot);
		return (uint64_t)mEntry.modBaseAddr;
	}

	uint64_t ProcessReader::GetMemorySize() const
	{
		if (!this->BaseStream)
			IOError::StreamBaseStream();

		auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetProcessId(this->BaseStream->GetProcessHandle()));
		if (hSnapshot == INVALID_HANDLE_VALUE)
			return NULL;

		uint64_t Result = 0;
		MODULEENTRY32 mEntry;
		mEntry.dwSize = sizeof(MODULEENTRY32);

		if (!Module32First(hSnapshot, &mEntry))
		{
			CloseHandle(hSnapshot);
			return NULL;
		}

		CloseHandle(hSnapshot);
		return (uint64_t)mEntry.modBaseSize;
	}

	uint64_t ProcessReader::GetSizeOfCode() const
	{
		auto BaseAddress = this->GetBaseAddress();

		// Resolve executable code size from the optional header
		auto DOSHeader = this->Read<IMAGE_DOS_HEADER>(BaseAddress);
		auto NTHeader = this->Read<IMAGE_NT_HEADERS>(BaseAddress + DOSHeader.e_lfanew);

		return (uint64_t)NTHeader.OptionalHeader.SizeOfCode;
	}

	ProcessStream* ProcessReader::GetBaseStream() const
	{
		return this->BaseStream.get();
	}

	void ProcessReader::Close()
	{
		// Forcefully reset the stream
		if (this->_LeaveOpen)
			this->BaseStream.release();
		else
			this->BaseStream.reset();
	}
}