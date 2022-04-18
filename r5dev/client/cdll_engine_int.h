#pragma once

enum class ClientFrameStage_t : int
{
	FRAME_UNDEFINED = -1, // (haven't run any frames yet)
	FRAME_START,

	// A network packet is being recieved
	FRAME_NET_UPDATE_START,
	// Data has been received and we're going to start calling PostDataUpdate
	FRAME_NET_UPDATE_POSTDATAUPDATE_START,
	// Data has been received and we've called PostDataUpdate on all data recipients
	FRAME_NET_UPDATE_POSTDATAUPDATE_END,
	// We've received all packets, we can now do interpolation, prediction, etc..
	FRAME_NET_UPDATE_END,

	// We're about to start rendering the scene
	FRAME_RENDER_START,
	// We've finished rendering the scene.
	FRAME_RENDER_END,

	FRAME_NET_FULL_FRAME_UPDATE_ON_REMOVE
};

class CHLClient
{
public:
	static void FrameStageNotify(CHLClient* pHLClient, ClientFrameStage_t curStage);
	void PatchNetVarConVar(void) const;

	void* /* CUserCmd* */ GetUserCmd(int sequenceNumber) // @0x1405BB020 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		static int index = 28;
		return CallVFunc<void*>(index, this, sequenceNumber); /*48 83 EC 28 48 8B 05 ? ? ? ? 48 8D 0D ? ? ? ? 44 8B C2*/
	}
};

//#ifndef DEDICATED
/* ==== CHLCLIENT ======================================================================================================================================================= */
inline CMemory p_CHLClient_PostInit;
inline auto CHLClient_PostInit = p_CHLClient_PostInit.RCast<void*(*)(void)>();

inline CMemory p_CHLClient_LevelShutdown;
inline auto CHLClient_LevelShutdown = p_CHLClient_LevelShutdown.RCast<void* (*)(CHLClient* thisptr)>();

inline CMemory p_CHLClient_FrameStageNotify;
inline auto CHLClient_FrameStageNotify = p_CHLClient_FrameStageNotify.RCast<void(*)(CHLClient* thisptr, ClientFrameStage_t frameStage)>();

inline CMemory p_CHLClient_HudProcessInput;
inline auto CHLClient_HudProcessInput = p_CHLClient_HudProcessInput.RCast<void(*)(CHLClient* thisptr, bool bActive)>();

inline bool* cl_time_use_host_tickcount = nullptr;
//#endif // !DEDICATED

inline CHLClient* gHLClient = nullptr;
inline CHLClient* g_pHLClient = nullptr;

///////////////////////////////////////////////////////////////////////////////
void CHLClient_Attach();
void CHLClient_Detach();

///////////////////////////////////////////////////////////////////////////////
class HDll_Engine_Int : public IDetour
{
	virtual void GetAdr(void) const
	{
#ifndef DEDICATED
		std::cout << "| FUN: CHLClient::PostInit                  : 0x" << std::hex << std::uppercase << p_CHLClient_PostInit.GetPtr()         << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CHLClient::LevelShutdown             : 0x" << std::hex << std::uppercase << p_CHLClient_LevelShutdown.GetPtr()    << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CHLClient::HudProcessInput           : 0x" << std::hex << std::uppercase << p_CHLClient_HudProcessInput.GetPtr()  << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CHLClient::FrameStageNotify          : 0x" << std::hex << std::uppercase << p_CHLClient_FrameStageNotify.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "| VAR: cl_time_use_host_tickcount           : 0x" << std::hex << std::uppercase << cl_time_use_host_tickcount            << std::setw(0)    << " |" << std::endl;
#endif // !DEDICATED
		std::cout << "| VAR: gHLClient                            : 0x" << std::hex << std::uppercase << gHLClient                             << std::setw(0)    << " |" << std::endl;
		std::cout << "| VAR: g_pHLClient                          : 0x" << std::hex << std::uppercase << g_pHLClient                             << std::setw(0)    << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CHLClient_PostInit         = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\x3D\x00\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00"), "xxx?????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????");
		p_CHLClient_LevelShutdown    = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x56\x41\x54\x41\x56\x48\x83\xEC\x28\x48\x8B\xF1"), "xxxxxxxxxxxxxx");
		p_CHLClient_FrameStageNotify = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x38\x89\x15\x00\x00\x00\x00"), "xxxxxx????");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CHLClient_PostInit         = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x48\x83\x3D\x00\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00"), "xxxxxxx?????xxx????");
		p_CHLClient_LevelShutdown    = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xF9\x48\x8D\x0D\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxxxxxxxxx????");
		p_CHLClient_FrameStageNotify = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x89\x15\x00\x00\x00\x00"), "xxxxxx????");
#endif
		p_CHLClient_HudProcessInput = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x0F\xB6\x0D\x00\x00\x00\x00\x88\x15\x00\x00\x00\x00"), "xxxxxxx????xx????");

		CHLClient_PostInit         = p_CHLClient_PostInit.RCast<void* (*)(void)>();                                 /*48 83 EC 28 48 83 3D ?? ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ??*/
		CHLClient_LevelShutdown    = p_CHLClient_LevelShutdown.RCast<void* (*)(CHLClient*)>();                      /*48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B F9 48 8D 0D ?? ?? ?? ??*/
		CHLClient_FrameStageNotify = p_CHLClient_FrameStageNotify.RCast<void(*)(CHLClient*, ClientFrameStage_t)>(); /*48 83 EC 28 89 15 ?? ?? ?? ??*/
		CHLClient_HudProcessInput  = p_CHLClient_HudProcessInput.RCast<void(*)(CHLClient*, bool)>();                /*48 83 EC 28 0F B6 0D ?? ?? ?? ?? 88 15 ?? ?? ?? ??*/
		//#endif // !DEDICATED
	}
	virtual void GetVar(void) const
	{
		cl_time_use_host_tickcount = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x80\x3D\x00\x00\x00\x00\x00\x74\x14\x66\x0F\x6E\x05\x00\x00\x00\x00"), "xx?????xxxxxx????").ResolveRelativeAddress(0x2, 0x7).RCast<bool*>();

		gHLClient = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>
			("\x48\x8D\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x30\x48\x8B\xF9"),
			"xxx????xxxxxxxxxxxxx?xxxxxxxx").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CHLClient*>();

		g_pHLClient = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>
			("\x41\x55\x48\x83\xEC\x00\x4C\x63\x91\x00\x00\x00\x00"),
			"xxxxx?xxx????").FindPatternSelf("4C 8B", CMemory::Direction::DOWN, 512, 2).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CHLClient*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HDll_Engine_Int);
