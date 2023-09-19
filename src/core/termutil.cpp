#include "core/stdafx.h"
#include "core/termutil.h"

const char* g_svGreyF   = "";
const char* g_svRedF    = "";
const char* g_svGreenF  = "";
const char* g_svBlueF   = "";
const char* g_svYellowF = "";

const char* g_svGreyB   = "";
const char* g_svRedB    = "";
const char* g_svGreenB  = "";
const char* g_svBlueB   = "";
const char* g_svYellowB = "";

const char* g_svReset   = "";

//-----------------------------------------------------------------------------
// Purpose: sets the global ansi escape sequences.
// If '-ansicolor' has not been passed to the sdk the char will be empty.
//-----------------------------------------------------------------------------
void AnsiColors_Init()
{
	g_svGreyF   = "\033[38;2;204;204;204;48;2;000;000;000m";
	g_svRedF    = "\033[38;2;255;000;000;48;2;000;000;000m";
	g_svGreenF  = "\033[38;2;000;255;000;48;2;000;000;000m";
	g_svBlueF   = "\033[38;2;000;000;255;48;2;000;000;000m";
	g_svYellowF = "\033[38;2;255;255;000;48;2;000;000;000m";

	g_svGreyB   = "\033[38;2;000;000;000;48;2;204;204;204m";
	g_svRedB    = "\033[38;2;000;000;000;48;2;255;000;000m";
	g_svGreenB  = "\033[38;2;000;000;000;48;2;000;255;000m";
	g_svBlueB   = "\033[38;2;000;000;000;48;2;000;000;255m";
	g_svYellowB = "\033[38;2;000;000;000;48;2;255;255;000m";

	g_svReset  = "\033[38;2;204;204;204;48;2;000;000;000m";
}