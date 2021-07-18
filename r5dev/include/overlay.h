#pragma once
#include "imgui.h"
#include "serverlisting.h"
#include "json.hpp"
#include <vector>
/////////////////////////////////////////////////////////////////////////////
// Initialization
void PrintDXAddress();
void InstallDXHooks();
void RemoveDXHooks();
void ShowGameConsole(bool* p_open);

/////////////////////////////////////////////////////////////////////////////
// Internals
int Stricmp(const char* s1, const char* s2);
int Strnicmp(const char* s1, const char* s2, int n);
char* Strdup(const char* s);
void  Strtrim(char* s);

/////////////////////////////////////////////////////////////////////////////
// Globals
inline ImVector<char*>       Items;

inline std::string OriginUID = "1010417302770";

/////////////////////////////////////////////////////////////////////////////

using json = nlohmann::json;
void RunConsoleCommand(std::string command);

/////////////////////////////////////////////////////////////////////////////
// ServerBrowser

class CCompanion
{
public:
    enum class ESection {
        ServerBrowser,
        HostServer,
        Settings
    } CurrentSection;

    enum class EHostStatus {
        NotHosting,
        WaitingForStateChange,
        Hosting,
        ConnectedToSomeoneElse
    };

    CCompanion();
    ////////////////////
    // Server Browser //
    ////////////////////
    ImVector<ServerListing*>       ServerList;
    ServerListing* SelectedServer;

    ImGuiTextFilter ServerBrowserFilter;

    char ServerConnStringBuffer[256] = { 0 };

    ////////////////////
    //    Settings    //
    ////////////////////
    char MatchmakingServerStringBuffer[256] = { 0 };


    ////////////////////
    //   Host Server  //
    ////////////////////
    std::vector<std::string> MapsList;
    std::string* SelectedMap = nullptr;
    char ServerNameBuffer[64] = { 0 };
    bool StartAsDedi;
    EHostStatus HostingStatus = EHostStatus::NotHosting;

    void RefreshServerList();

    void SendHostingPostRequest();
    void SetSection(ESection section);
    void CompMenu();
    void ServerBrowserSection();
    void SettingsSection();
    void HostServerSection();
    void Draw(const char* title, bool* p_open);
    void UpdateHostingStatus();

    std::string GetGameStateLastMap();
};