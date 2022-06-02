#pragma once
#include <thirdparty/spdlog/include/sinks/basic_file_sink.h>

/////////////////////////////////////////////////////////////////////////////
// Internals
BOOL IsBadReadPtrV2(void* ptr);
BOOL FileExists(const char* szPath);
MODULEINFO GetModuleInfo(const char* szModule);
DWORD64 FindPatternSIMD(const char* szModule, const unsigned char* szPattern, const char* szMask);

/////////////////////////////////////////////////////////////////////////////
// Utility
void DbgPrint(LPCSTR sFormat, ...);
void PrintLastError(void);
void HexDump(const char* szHeader, const char* szLogger, const void* pData, int nSize);

string GetExtension(const string& svInput);
string RemoveExtension(const string& svInput);

string GetFileName(const string& svInput, bool bRemoveExtension, bool bWindows = false);
string RemoveFileName(const string& svInput, bool bWindows = false);

string CreateDirectories(string svInput);
string ConvertToWinPath(const string& svInput);
string ConvertToUnixPath(const string& svInput);

string Base64Encode(const string& svInput);
string Base64Decode(const string& svInput);

string UTF8Encode(const wstring& wsvInput);
u32string UTF8Decode(const string& svInput);

bool StringIsDigit(const string& svInput);
bool CompareStringAlphabetically(const string& svA, const string& svB);
bool CompareStringLexicographically(const string& svA, const string& svB);

bool StringReplace(string& svInput, const string& svFrom, const string& svTo);
string StringReplaceC(const string& svInput, const string& svFrom, const string& svTo);
string StringEscape(const string& svInput);
string StringUnescape(const string& svInput);

vector<int> StringToBytes(const string& svInput, bool bNullTerminator);
vector<int> PatternToBytes(const string& svInput);
vector<int> IntToDigits(int iValue);

void PrintM128i8(__m128i in);
void PrintM128i16(__m128i in);
void PrintM128i32(__m128i in);
void PrintM128i64(__m128i in);

string PrintPercentageEscape(const string& svInput);

/////////////////////////////////////////////////////////////////////////////
