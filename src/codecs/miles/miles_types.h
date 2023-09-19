#pragma once

constexpr char MILES_DEFAULT_LANGUAGE[] = "english";

namespace Miles
{
	struct Queue
	{
		char gap0[0x8];
		void* unk;
		char gap10[0x20];
	};

	struct Bank
	{
		// TODO [REXX]: map out this struct and its internal counterpart
	};
}