#pragma once

constexpr char MILES_DEFAULT_LANGUAGE[] = "english";

namespace Miles
{
	constexpr int TEMPLATEID_FLAG_SOURCE = 0x40000000;


	struct Queue
	{
		char gap0[0x8];
		void* unk;
		char gap10[0x20];
	};

	struct BankHeader_t;

	struct Source_t
	{
		BankHeader_t* bank; // reserved on disk - written at runtime
		char gap8[80];
	};

	struct Event_t
	{
		int nameOffset;
		int unkOffset; // offset into BankHeader_t::unk_68 data - some sort of event metadata?
	};

	// internal project data structure
	struct IntProjectData_t
	{
		char gap0[0xE28];
		int bankCount;
		BankHeader_t** loadedBanks;
	};

	struct BankHeader_t
	{
		int magic;     // 'CBNK'
		int version;   // 32
		uint32_t dataSize;

		int bankMagic; // 'BANK'
		const char* bankName;

		void* unk_18;
		IntProjectData_t* project;

		void* unk_28;
		void* unk_30;
		void* unk_38;
		void* unk_40; // used to index into both sources and localised sources

		Source_t* sources;
		Source_t* localizedSources;

		void* unk_58;
		Event_t* events;

		void* unk_68;
		const char* stringTable;

		void* unk_78;
		void* unk_80;
		void* unk_88;

		uint8_t bankIndex;
		// 3 byte padding

		uint32_t localizedSourceCount;
		uint32_t sourceCount;

		uint32_t patchCount;
		uint32_t eventCount;

		uint32_t count_A4;
		uint32_t count_A8;
		uint32_t count_AC;

		uint32_t buildTag;

		uint32_t unk_B4;
		uint32_t someDataSize;

		const char* GetBankName() const
		{
			return bankName;
		}

	};
	static_assert(offsetof(BankHeader_t, project) == 0x20);
	static_assert(offsetof(BankHeader_t, stringTable) == 0x70);
	static_assert(offsetof(BankHeader_t, unk_B4) == 0xB4);


	struct Bank
	{
		void* internalData;
		void* unk_8;

		char* fileData;

		int unk_18;
		char gap_1c[4];

		const Miles::BankHeader_t* GetHeader() const
		{
			return reinterpret_cast<Miles::BankHeader_t*>(fileData);
		}

		const char* GetBankName() const
		{
			return GetHeader()->GetBankName();
		}
	};

	static_assert(sizeof(Bank) == 0x20);
}