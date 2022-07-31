#pragma once

/////////////////////////////////////////////////////////////////////////////
// Internals
BOOL IsBadReadPtrV2(void* ptr);
BOOL FileExists(const fs::path& svFilePath);
MODULEINFO GetModuleInfo(const char* szModule);

/////////////////////////////////////////////////////////////////////////////
// Debug
void DbgPrint(LPCSTR sFormat, ...);
void PrintLastError(void);
void HexDump(const char* szHeader, const char* szLogger, const void* pData, int nSize);

/////////////////////////////////////////////////////////////////////////////
// String
bool HasExtension(const string& svInput, const string& svExtension);
string GetExtension(const string& svInput, bool bReturnOriginal = false, bool bKeepDelimiter = false);
string RemoveExtension(const string& svInput);

bool HasFileName(const string& svInput, const string& svFileName);
string GetFileName(const string& svInput, bool bRemoveExtension, bool bWindows = false);
string RemoveFileName(const string& svInput, bool bWindows = false);

string CreateDirectories(string svInput, bool bWindows = false);
string ConvertToWinPath(const string& svInput);
string ConvertToUnixPath(const string& svInput);

string Base64Encode(const string& svInput);
string Base64Decode(const string& svInput);

string UTF8Encode(const wstring& wsvInput);
string UTF8Decode(const string& svInput);
size_t UTF8CharLength(const uint8_t cInput);

bool StringIsDigit(const string& svInput);
bool CompareStringAlphabetically(const string& svA, const string& svB);
bool CompareStringLexicographically(const string& svA, const string& svB);

bool StringReplace(string& svInput, const string& svFrom, const string& svTo);
string StringReplaceC(const string& svInput, const string& svFrom, const string& svTo);
string StringEscape(const string& svInput);
string StringUnescape(const string& svInput);
vector<string> StringSplit(string svInput, char cDelim, size_t nMax = SIZE_MAX);

/////////////////////////////////////////////////////////////////////////////
// Bytes
vector<int> StringToBytes(const string& svInput, bool bNullTerminator);
pair<vector<uint8_t>, string> StringToMaskedBytes(const string& svInput, bool bNullTerminator);
vector<int> PatternToBytes(const string& svInput);
pair<vector<uint8_t>, string> PatternToMaskedBytes(const string& svInput);
vector<int> IntToDigits(int iValue);

/////////////////////////////////////////////////////////////////////////////
// Print
void PrintM128i8(__m128i in);
void PrintM128i16(__m128i in);
void PrintM128i32(__m128i in);
void PrintM128i64(__m128i in);

string PrintPercentageEscape(const string& svInput);

/////////////////////////////////////////////////////////////////////////////
