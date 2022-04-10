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
	void FrameStageNotify(ClientFrameStage_t curStage) // @0x1405C0740 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		static int index = 58;
		CallVFunc<void>(index, this, curStage); /*48 83 EC 28 89 15 ?? ?? ?? ??*/
	}

	void* /* CUserCmd* */ GetUserCmd(int sequenceNumber) // @0x1405BB020 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		static int index = 28;
		return CallVFunc<void*>(index, this, sequenceNumber); /*48 83 EC 28 48 8B 05 ? ? ? ? 48 8D 0D ? ? ? ? 44 8B C2*/
	}
};

/* ==== CHLCLIENT ======================================================================================================================================================= */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory p_CHLClient_PostInit = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\x3D\x00\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00"), "xxx?????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????");
inline auto CHLClient_PostInit = p_CHLClient_PostInit.RCast<void*(*)(void)>(); /*48 83 3D ? ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 48 8D 05 ? ? ? ?*/

inline CMemory p_CHLClient_LevelShutdown = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x56\x41\x54\x41\x56\x48\x83\xEC\x28\x48\x8B\xF1"), "xxxxxxxxxxxxxx");
inline auto CHLClient_LevelShutdown = p_CHLClient_LevelShutdown.RCast<void*(*)(void* thisptr)>(); /*40 53 56 41 54 41 56 48 83 EC 28 48 8B F1*/

inline CMemory p_CHLClient_FrameStageNotify = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x38\x89\x15\x00\x00\x00\x00"), "xxxxxx????");
inline auto CHLClient_FrameStageNotify = p_CHLClient_FrameStageNotify.RCast<void(*)(void* rcx, ClientFrameStage_t frameStage)>(); /*48 83 EC 38 89 15 ?? ?? ?? ??*/

#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory p_CHLClient_PostInit = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x48\x83\x3D\x00\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00"), "xxxxxxx?????xxx????");
inline auto CHLClient_PostInit = p_CHLClient_PostInit.RCast<void*(*)(void)>(); /*48 83 EC 28 48 83 3D ? ? ? ? ? 48 8D 05 ? ? ? ?*/

inline CMemory p_CHLClient_LevelShutdown = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xF9\x48\x8D\x0D\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxxxxxxxxx????");
inline auto CHLClient_LevelShutdown = p_CHLClient_LevelShutdown.RCast<void* (*)(void* thisptr)>(); /*48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B F9 48 8D 0D ? ? ? ?*/

inline CMemory p_CHLClient_FrameStageNotify = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x89\x15\x00\x00\x00\x00"), "xxxxxx????");
inline auto CHLClient_FrameStageNotify = p_CHLClient_FrameStageNotify.RCast<void(*)(void* thisptr, ClientFrameStage_t frameStage)>(); /*48 83 EC 28 89 15 ?? ?? ?? ??*/
#endif
inline CMemory p_CHLClient_HudProcessInput = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x0F\xB6\x0D\x00\x00\x00\x00\x88\x15\x00\x00\x00\x00"), "xxxxxxx????xx????");
inline auto CHLClient_HudProcessInput = p_CHLClient_HudProcessInput.RCast<void(*)(void* thisptr, bool bActive)>(); /*48 83 EC 28 0F B6 0D ? ? ? ? 88 15 ? ? ? ?*/

inline bool* cl_time_use_host_tickcount = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x80\x3D\x00\x00\x00\x00\x00\x74\x14\x66\x0F\x6E\x05\x00\x00\x00\x00"), "xx?????xxxxxx????").ResolveRelativeAddress(0x2, 0x7).RCast<bool*>();

///////////////////////////////////////////////////////////////////////////////
void __fastcall HFrameStageNotify(CHLClient* rcx, ClientFrameStage_t frameStage);
void PatchNetVarConVar();

void CHLClient_Attach();
void CHLClient_Detach();

///////////////////////////////////////////////////////////////////////////////
class HDll_Engine_Int : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: CHLClient::PostInit                  : 0x" << std::hex << std::uppercase << p_CHLClient_PostInit.GetPtr()         << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CHLClient::LevelShutdown             : 0x" << std::hex << std::uppercase << p_CHLClient_LevelShutdown.GetPtr()    << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CHLClient::HudProcessInput           : 0x" << std::hex << std::uppercase << p_CHLClient_HudProcessInput.GetPtr()  << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CHLClient::FrameStageNotify          : 0x" << std::hex << std::uppercase << p_CHLClient_FrameStageNotify.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "| VAR: cl_time_use_host_tickcount           : 0x" << std::hex << std::uppercase << cl_time_use_host_tickcount            << std::setw(0)    << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HDll_Engine_Int);
