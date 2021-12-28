#pragma once

#include "core/stdafx.h"
#include <windows/id3dx.h>
#include <networksystem/r5net.h>
#include <gameui/IConsole.h>

#ifndef DEDICATED

class CDevPalette 
{
public:
	void Draw(bool* bDraw)
	{

        ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);
        ImGui::SetWindowPos(ImVec2(-1000, 50), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin("Dev Palette", bDraw))
        {
            ImGui::End();
            return;
        }
        if (ImGui::Button("Get Stats"))
        {
            auto response = g_pR5net->GetGlobalStats();
            switch (response.status)
            {
            case R5Net::EResponseStatus::SUCCESS:
            {
                g_GameConsole->AddLog("SUCCESS: %d players, %d servers", response.noPlayers, response.noServers);
                break;
            }
            default:
            {
                g_GameConsole->AddLog("ERROR: %s", response.error.c_str());
            }
            }
        }
        if (ImGui::Button("Send Fake Server Request"))
        {
            R5Net::UpdateGameServerMSRequest request{};
            request.gameServer.name = "TestNameServer";
            request.gameServer.playlist = "yoopp";
            request.gameServer.gamePort = 8089;


            auto response = g_pR5net->UpdateMyGameServer(request);
            switch (response.status)
            {
            case R5Net::EResponseStatus::SUCCESS:
            {
                g_GameConsole->AddLog("SUCCESS: %s", response.gameServer.name.c_str());
                break;
            }
            default:
            {
                g_GameConsole->AddLog("ERROR: %s", response.error.c_str());
            }
            }
        }
        if (*bDraw == NULL)
        {
            g_bShowBrowser = false;
        }
        ImGui::End();
	}
};

void DrawDevPalette(bool* bDraw);

extern CDevPalette* g_DevPalette;
#endif