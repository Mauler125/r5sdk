///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PLATFORM_STADIA)
#include <stdlib.h>
#include <ctype.h>
#include <stdlib.h>

EATHREADLIB_API bool isUint(const char* buffer)
{
	if (buffer == nullptr || *buffer == '\0')
		return false;

	while (isspace(*buffer) && *buffer != '\0')
		++buffer;

	return *buffer != '\0' && isdigit(*buffer);
}

EATHREADLIB_API EA::Thread::ThreadAffinityMask parseSet(char const* buffer, size_t len)
{
	EA::Thread::ThreadAffinityMask mask = 0;
	if (buffer == nullptr || len == 0)
		return mask;

	char const* ptr = buffer;
	char const* end_ptr = buffer + len;
	if (!isUint(ptr))
		return mask;

	uint32_t prevVal = strtoul(ptr, (char**)&ptr, 10);
	uint32_t val = 0;

	mask |= 1ULL << prevVal;

	char op;
	while (ptr < end_ptr)
	{
		op = *ptr;
		++ptr;
		switch (op)
		{
		case '-':

			if (!isUint(ptr))
				return mask;
			val = strtoul(ptr, (char**)&ptr, 10);
			for (int i = prevVal + 1; i <= val; ++i)
			{
				mask |= 1ULL << i;
			}
			break;
		case ',':
			if (!isUint(ptr))
				return mask;
			prevVal = strtoul(ptr, (char**)&ptr, 10);
			mask |= 1ULL << prevVal;
			break;
		default:
			EAT_ASSERT_MSG(isspace(op), "Invalid op token.");
			break;
		}
	}
	return mask;
}

EATHREADLIB_API bool readLine(const char* fileName, char* buffer, size_t len)
{
	if (fileName == nullptr || buffer == nullptr || len == 0)
		return false;

	memset(buffer, 0, len);

	FILE* fp = fopen(fileName, "r");
	if (fp == nullptr)
		return false;

	bool ret = fgets(buffer, len, fp) != nullptr;
	fclose(fp);
	if (ret)
	{
		size_t slen = strlen(buffer);
		if (buffer[slen - 1] == '\n')
			buffer[slen - 1] = '\0';
	}

	return ret;
}

EATHREADLIB_API bool getProcessCpuSetCGroup(char* cgroup, size_t len)
{
	return readLine("/proc/self/cpuset", cgroup, len);
}

EATHREADLIB_API EA::Thread::ThreadAffinityMask getInstanceCpuSet()
{
	static const char* instanceFileName = "/sys/devices/system/cpu/present";

	EA::Thread::ThreadAffinityMask mask = 0;
	char buf[256];
	if (!readLine(instanceFileName, buf, sizeof(buf)))
		return mask;

	return parseSet(buf, strlen(buf));
}

EATHREADLIB_API EA::Thread::ThreadAffinityMask initValidCpuAffinityMask()
{
	char cgroup[32];
	if (!getProcessCpuSetCGroup(cgroup, sizeof(cgroup)))
	{
		return getInstanceCpuSet();
	}
	char cpuSetFileName[256];
	sprintf(cpuSetFileName, "/sys/fs/cgroup/cpuset%s/cpuset.cpus", cgroup);

	char buf[256];
	if (!readLine(cpuSetFileName, buf, sizeof(buf)))
		return getInstanceCpuSet();

	return parseSet(buf, strlen(buf));
}

EATHREADLIB_API EA::Thread::ThreadAffinityMask getValidCpuAffinityMask()
{
	static EA::Thread::ThreadAffinityMask mask = initValidCpuAffinityMask();
	return mask;
}

EATHREADLIB_API int initAvailableCpuCount()
{
	EA::Thread::ThreadAffinityMask mask = getValidCpuAffinityMask();
	
	int count = 1 & mask;
	EAT_ASSERT_MSG(count, "First available cpu is not 0");
	bool prevVal = count;

	for (int i = 1; i < sizeof(mask); ++i)
	{
		bool val = (1 << i) & mask;
		EAT_ASSERT_MSG(!val || prevVal, "Non-contiguous range of available cpus");
		count += val;
		prevVal = val;
	}

	return count;
}

EATHREADLIB_API int getAvailableCpuCount()
{
	static int count = initAvailableCpuCount();
	return count;
}

#endif
