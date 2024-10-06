//=============================================================================//
//
// Purpose: Post-mortem crash reporter
//
//=============================================================================//
#include "tier0/crashhandler.h"
#include "tier0/cpu.h"
#include "tier0/commandline.h"
#include "tier2/curlutils.h"
#include "tier2/crashreporter.h"

static ConVar backtrace_enabled("backtrace_enabled", "1", FCVAR_RELEASE, "Whether to report fatal errors to the collection server");
static ConVar backtrace_hostname("backtrace_hostname", "submit.backtrace.io", FCVAR_RELEASE, "Holds the error collection server hostname");
static ConVar backtrace_universe("backtrace_universe", "r5reloaded", FCVAR_RELEASE, "Holds the error collection server hosted instance");
static ConVar backtrace_token("backtrace_token", "f178fd48d89c8fec7f8b6404ae6dae591c330fd3e2599cab888788033944ec98", FCVAR_RELEASE, "Holds the error collection server submission token");

static inline bool CrashReporter_ShowMessageBox()
{
	if (MessageBoxA(NULL, 
		"The program encountered a critical error and was terminated.\n"
		"Would you like to send this report to help solve the problem?",
		"Critical Error Detected", MB_ICONERROR | MB_YESNO) == IDYES)
	{
		return true;
	}

	return false;
}

static inline bool CrashReporter_ShouldSubmitReport()
{
	if (!ConVar_IsRegistered())
	{
		// Can't check if the user accepted the EULA or not, show a prompt instead.
		if (!CrashReporter_ShowMessageBox())
			return false;
	}
	else
	{
		if (!backtrace_enabled.GetBool())
			return false;

		if (!IsEULAUpToDate())
			return false;
	}

	return true;
}

static inline string CrashReporter_FormatAttributes(const CCrashHandler* const handler)
{
	const CPUInformation& pi = GetCPUInformation();
	const CrashHardWareInfo_s& hi = handler->GetHardwareInfo();

	const char* const format = "uuid=%s&"  "build_id=%lld&"
		"cpu_model=%s&" "cpu_speed=%lf GHz&" "gpu_model=%s&" "gpu_flags=%lu&"
		"ram_phys_total=%.2lf MiB&" "ram_phys_avail=%.2lf MiB&"
		"ram_virt_total=%.2lf MiB&" "ram_virt_avail=%.2lf MiB&"
		"disk_total=%.2lf MiB&" "disk_avail=%.2lf MiB";

	const DISPLAY_DEVICE& dd = hi.displayDevice;
	const MEMORYSTATUSEX& ms = hi.memoryStatus;

	return Format(format, g_LogSessionUUID.c_str(), g_SDKDll.GetNTHeaders()->FileHeader.TimeDateStamp,
		pi.m_szProcessorBrand, (f64)(pi.m_Speed / 1000000000.0), dd.DeviceString, dd.StateFlags,
		(f64)(ms.ullTotalPhys    / (1024.0*1024.0)), (f64)(ms.ullAvailPhys    / (1024.0*1024.0)),
		(f64)(ms.ullTotalVirtual / (1024.0*1024.0)), (f64)(ms.ullAvailVirtual / (1024.0*1024.0)),
		(f64)(hi.totalDiskSpace  / (1024.0*1024.0)), (f64)(hi.availDiskSpace  / (1024.0*1024.0)));
}

void CrashReporter_SubmitToCollector(const CCrashHandler* const handler)
{
	if (!CommandLine()->CheckParm("-nomessagebox"))
		handler->CreateMessageProcess();

	if (!CrashReporter_ShouldSubmitReport())
		return;

	curl_slist* slist = nullptr;
	slist = CURLSlistAppend(slist, "Expect:");

	if (!slist)
		return; // failure.

	const string attributes = CrashReporter_FormatAttributes(handler);
	const string hostName = Format("%s%s/%s/%s/%s&%s", 
		"https://", backtrace_hostname.GetString(), backtrace_universe.GetString(), backtrace_token.GetString(), "minidump", attributes.c_str());
	const string miniDumpPath = Format("%s/%s.dmp", g_LogSessionDirectory.c_str(), "minidump");

	CURLParams params;
	params.readFunction = &CURLReadFileCallback;
	params.writeFunction = &CURLWriteStringCallback;
	params.timeout = curl_timeout.GetInt();
	params.verifyPeer = ssl_verify_peer.GetBool();
	params.verbose = curl_debug.GetBool();

	CURLUploadFile(hostName.c_str(), miniDumpPath.c_str(), "rb", nullptr, true, slist, params);
}
