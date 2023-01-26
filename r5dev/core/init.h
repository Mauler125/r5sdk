#pragma once

void SDK_Init();
void SDK_Shutdown();

void Systems_Init();
void Systems_Shutdown();

void WinSock_Init();
void WinSock_Shutdown();
void QuerySystemInfo();
void CheckCPU();

void DetourInit();
void DetourAddress();
void DetourRegister();
