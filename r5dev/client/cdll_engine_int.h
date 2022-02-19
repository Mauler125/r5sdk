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

namespace
{
	/* ==== CHLCLIENT ======================================================================================================================================================= */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS p_CHLClient_FrameStageNotify = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x38\x89\x15\x00\x00\x00\x00", "xxxxxx????");
	void (*CHLClient_FrameStageNotify)(void* rcx, int curStage) = (void(*)(void*, int))p_CHLClient_FrameStageNotify.GetPtr(); /*48 83 EC 38 89 15 ?? ?? ?? ??*/

	ADDRESS p_CHLClient_PostInit = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\x3D\x00\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00", "xxx?????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????");
	void* (*CHLClient_PostInit)() = (void* (*)())p_CHLClient_PostInit.GetPtr(); /*48 83 3D ? ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 48 8D 05 ? ? ? ?*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS p_CHLClient_FrameStageNotify = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x28\x89\x15\x00\x00\x00\x00", "xxxxxx????");
	void (*CHLClient_FrameStageNotify)(void* rcx, int curStage) = (void(*)(void*, int))p_CHLClient_FrameStageNotify.GetPtr(); /*48 83 EC 28 89 15 ?? ?? ?? ??*/

	ADDRESS p_CHLClient_PostInit = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x28\x48\x83\x3D\x00\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00", "xxxxxxx?????xxx????");
	void* (*CHLClient_PostInit)() = (void* (*)())p_CHLClient_PostInit.GetPtr(); /*48 83 EC 28 48 83 3D ? ? ? ? ? 48 8D 05 ? ? ? ?*/
#endif

	bool* cl_time_use_host_tickcount = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x80\x3D\x00\x00\x00\x00\x00\x74\x14\x66\x0F\x6E\x05\x00\x00\x00\x00", "xx?????xxxxxx????").ResolveRelativeAddress(0x2, 0x7).RCast<bool*>();
}

///////////////////////////////////////////////////////////////////////////////
void __fastcall HFrameStageNotify(CHLClient* rcx, ClientFrameStage_t curStage);
void PatchNetVarConVar();

void CHLClient_Attach();
void CHLClient_Detach();

///////////////////////////////////////////////////////////////////////////////
class HDll_Engine_Int : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CHLClient::FrameStageNotify          : 0x" << std::hex << std::uppercase << p_CHLClient_FrameStageNotify.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CHLClient::PostInit                  : 0x" << std::hex << std::uppercase << p_CHLClient_PostInit.GetPtr()         << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: cl_time_use_host_tickcount           : 0x" << std::hex << std::uppercase << cl_time_use_host_tickcount            << std::setw(0)    << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HDll_Engine_Int);
