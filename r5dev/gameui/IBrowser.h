#pragma once
#ifndef DEDICATED
#include "networksystem/net_structs.h"
#include "networksystem/r5net.h"


class IBrowser
{
private:
    bool m_bInitialized = false;
public:
public:
    ////////////////////
    //     Funcs      //
    ////////////////////
    IBrowser();
    ~IBrowser();


    void GetServerList();

    void ConnectToServer(const std::string& ip, const int port, const std::string& encKey);
    void ConnectToServer(const std::string& connString, const std::string& encKey);


    void ProcessCommand(const char* command_line);
    void LaunchServer();

    void RegenerateEncryptionKey();
    void ChangeEncryptionKeyTo(const std::string& str);


    ////////////////////
    // Server Browser //
    ////////////////////
public:
    bool m_bActivate = false;

    std::vector<R5Net::NetGameServer> m_vServerList;
    ImGuiTextFilter m_imServerBrowserFilter;
    std::string m_szServerListMessage    = std::string();

    std::map<std::string, std::string> mapArray =
    {
        { "mp_rr_canyonlands_64k_x_64k", "King's Canyon Season 0" },
        { "mp_rr_desertlands_64k_x_64k", "World's Edge Season 3" },
        { "mp_rr_canyonlands_mu1", "King's Canyon Season 2" },
        { "mp_rr_canyonlands_mu1_night", "King's Canyon Season 2 After Dark" },
        { "mp_rr_desertlands_64k_x_64k_nx", "World's Edge Season 3 After Dark" },
        { "mp_lobby", "Lobby Season 3" },
        { "mp_rr_canyonlands_staging", "King's Canyon Firing Range" }
    };

    ////////////////////
    //    Settings    //
    ////////////////////
    std::string m_szMatchmakingHostName;

    ////////////////////
    //   Host Server  //
    ////////////////////
    std::vector<std::string> m_vszMapsList;
    std::vector<std::string> m_vszMapFileNameList;


};

extern IBrowser* g_pIBrowser;
#endif