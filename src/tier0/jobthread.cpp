
#include "engine/host_cmd.h"
#include "tier0/jobthread.h"


bool JT_IsJobDone(const JobID_t jobId)
{
	return (job_JT_Context[jobId & 0xFFF].field_24 ^ jobId) & 0xFFFFC000;
}

// TODO: confirm this actually returns a JobID_t and not some other int
JobID_t JTGuts_AddJob(JobTypeID_t jobTypeId, JobID_t jobId, void* callbackFunc, void* callbackArg)
{
	const unsigned int jobIndex = JT_AllocateJob();
	return JTGuts_AddJob_Internal(jobTypeId, jobId, callbackFunc, callbackArg, jobIndex, &job_JT_Context[jobIndex]);
}

JobID_t JT_GetCurrentJob()
{
	return *(_DWORD*)(*(_QWORD*)CModule::GetThreadEnvironmentBlock()->ThreadLocalStoragePointer + 12i64);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void* HJT_HelpWithAnything(bool bShouldLoadPak)
{
	// !! Uncomment if needed !!
	// !! Consider initializing this in idetour iface if enabled !!
	//static void* const retaddr = g_GameDll.FindPatternSIMD("48 8B C4 ?? 41 54 41 55 48 81 EC 70 04 ?? ?? F2 0F 10 05 ?? ?? ?? 0B")
	//	.Offset(0x4A0).FindPatternSelf("48 8B ?? ?? ?? ?? 01", CMemory::Direction::DOWN).RCast<void*>();

	void* const results = JT_HelpWithAnything(bShouldLoadPak);

	//if (retaddr != _ReturnAddress()) // Check if this is called after 'PakFile_Init()'.
	//{
	//	return results;
	//}
	// Do stuff here after 'PakFile_Init()'.
	return results;
}

void VJobThread::Detour(const bool bAttach) const
{
	//DetourSetup(&JT_HelpWithAnything, &HJT_HelpWithAnything, bAttach);
}
