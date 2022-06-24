/*-----------------------------------------------------------------------------
 * _utility
 *-----------------------------------------------------------------------------*/

#include "core/stdafx.h"
#include "core/logdef.h"
#include "public/include/utility.h"

///////////////////////////////////////////////////////////////////////////////
// For checking if a specific file exists.
BOOL FileExists(const fs::path& svFilePath)
{
    return fs::exists(svFilePath);
}

///////////////////////////////////////////////////////////////////////////////
// For checking if pointer is valid or bad.
BOOL IsBadReadPtrV2(void* ptr)
{
    MEMORY_BASIC_INFORMATION mbi = { 0 };
    if (::VirtualQuery(ptr, &mbi, sizeof(mbi)))
    {
        DWORD mask = (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
        bool b = !(mbi.Protect & mask);
        // check the page is not a guard page
        if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) b = true;
        return b;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// For getting information about the executing module.
MODULEINFO GetModuleInfo(const char* szModule)
{
    MODULEINFO modinfo = { 0 };

    wchar_t szWtext[256]{};
    mbstowcs(szWtext, szModule, strlen(szModule) + 1);

    HMODULE hModule = GetModuleHandle(szWtext);
    if (hModule == INVALID_HANDLE_VALUE)
    {
        return modinfo;
    }
    if (hModule)
    {
        GetModuleInformation(GetCurrentProcess(), hModule, &modinfo, sizeof(MODULEINFO));
    }
    return modinfo;
}

///////////////////////////////////////////////////////////////////////////////
// For finding a pattern in memory of the process with SIMD.
DWORD64 FindPatternSIMD(const char* szModule, const unsigned char* szPattern, const char* szMask)
{
    MODULEINFO mInfo = GetModuleInfo(szModule);
    DWORD64 dwBase = (DWORD64)mInfo.lpBaseOfDll;
    DWORD64 dwSize = (DWORD64)mInfo.SizeOfImage;

    unsigned char* pData = (unsigned char*)dwBase;
    unsigned int length  = (unsigned int)dwSize;

    const unsigned char* end = pData + length - strlen(szMask);
    int num_masks = (int)ceil((float)strlen(szMask) / (float)16);
    int masks[64]; // 64*16 = enough masks for 1024 bytes.
    memset(masks, 0, num_masks * sizeof(int));
    for (int i = 0; i < num_masks; ++i)
    {
        for (int j = strnlen(szMask + i * 16, 16) - 1; j >= 0; --j)
        {
            if (szMask[i * 16 + j] == 'x')
            {
                masks[i] |= 1 << j;
            }
        }
    }
    __m128i xmm1 = _mm_loadu_si128((const __m128i*) szPattern);
    __m128i xmm2, xmm3, msks;
    for (; pData != end; _mm_prefetch((const char*)(++pData + 64), _MM_HINT_NTA))
    {
        if (szPattern[0] == pData[0])
        {
            xmm2 = _mm_loadu_si128((const __m128i*) pData);
            msks = _mm_cmpeq_epi8(xmm1, xmm2);
            if ((_mm_movemask_epi8(msks) & masks[0]) == masks[0])
            {
                for (DWORD64 i = 1; i < num_masks; ++i)
                {
                    xmm2 = _mm_loadu_si128((const __m128i*) (pData + i * 16));
                    xmm3 = _mm_loadu_si128((const __m128i*) (szPattern + i * 16));
                    msks = _mm_cmpeq_epi8(xmm2, xmm3);
                    if ((_mm_movemask_epi8(msks) & masks[i]) == masks[i])
                    {
                        if ((i + 1) == num_masks)
                        {
                            return (DWORD64)pData;
                        }
                    }
                    else goto cont;
                }
                return (DWORD64)pData;
            }
        }cont:;
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// For printing output to the debugger.
void DbgPrint(LPCSTR sFormat, ...)
{
    CHAR sBuffer[512] = { 0 };
    va_list sArgs = {};

    // Get the variable arg pointer.
    va_start(sArgs, sFormat);

    // Format print the string.
    int length = vsnprintf(sBuffer, sizeof(sBuffer), sFormat, sArgs);
    va_end(sArgs);

    wchar_t szWtext[512]{}; // Convert to LPCWSTR.
    mbstowcs(szWtext, sBuffer, strlen(sBuffer) + 1);

    // Output the string to the debugger.
    OutputDebugString(szWtext);
}

///////////////////////////////////////////////////////////////////////////////
// For printing the last error to the console if any.
void PrintLastError(void)
{
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID != NULL)
    {
        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        spdlog::error("{:s}\n", messageBuffer);
        LocalFree(messageBuffer);
    }
}

///////////////////////////////////////////////////////////////////////////////
// For dumping data from a buffer to a file on the disk.
void HexDump(const char* szHeader, const char* szLogger, const void* pData, int nSize)
{
    static unsigned char szAscii[17] = {};
    static std::mutex m;
    static std::atomic<int> i = {}, j = {}, k = {};
    static std::shared_ptr<spdlog::logger> logger = spdlog::get("default_logger");

    m.lock();
    szAscii[16] = '\0';

    if (szLogger)
    {
        logger = spdlog::get(szLogger);
        if (!logger)
        {
            m.unlock();
            assert(logger == nullptr);
            return;
        }
    }

    // Add timestamp.
    logger->set_level(spdlog::level::trace);
    logger->set_pattern("%v [%H:%M:%S.%f]\n");
    logger->trace("---------------------------------------------------------");

    // Disable EOL and create block header.
    logger->set_pattern("%v");
    logger->trace("{:s} ---- LEN BYTES: {:d}\n:\n", szHeader, nSize);
    logger->trace("--------  0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F  0123456789ABCDEF\n");

    // Output the buffer to the file.
    for (i = 0; i < nSize; i++)
    {
        if (i % nSize == 0) { logger->trace(" 0x{:04X}  ", i); }
        logger->trace("{:02x} ", ((unsigned char*)pData)[i]);

        if ((reinterpret_cast<rsig_t>(pData))[i] >= ' ' && 
            (reinterpret_cast<rsig_t>(pData))[i] <= '~')
        {
            szAscii[i % 16] = (reinterpret_cast<rsig_t>(pData))[i];
        }
        else
        {
            szAscii[i % 16] = '.';
        }

        if ((i + 1) % 8 == 0 || i + 1 == nSize)
        {
            logger->trace(" ");

            if ((i + 1) % 16 == 0)
            {
                if (i + 1 == nSize)
                {
                    logger->trace("{:s}\n", szAscii);
                    logger->trace("---------------------------------------------------------------------------\n");
                    logger->trace("\n");
                }
                else
                {
                    i++;
                    logger->trace("{:s}\n ", szAscii);
                    logger->trace("0x{:04X}  ", i--);
                }
            }
            else if (i + 1 == nSize)
            {
                szAscii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8)
                {
                    logger->trace(" ");
                }
                for (j = (i + 1) % 16; j < 16; j++)
                {
                    logger->trace("   ");
                }
                logger->trace("{:s}\n", szAscii);
                logger->trace("---------------------------------------------------------------------------\n");
                logger->trace("\n");
            }
        }
    }
    m.unlock();
    ///////////////////////////////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////
// For checking if file name has a specific extension.
bool HasExtension(const string& svInput, const string& svExtension)
{
    if (svInput.substr(svInput.find_last_of('.') + 1) == svExtension)
    {
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
// For removing file names from the extension.
string GetExtension(const string& svInput, bool bReturnOriginal, bool bKeepDelimiter)
{
    string::size_type nPos = svInput.rfind('.');
    if (nPos != string::npos)
    {
        if (!bKeepDelimiter)
        {
            nPos += 1;
        }
       return svInput.substr(nPos);
    }
    if (bReturnOriginal)
    {
        return svInput;
    }
    return "";
}

///////////////////////////////////////////////////////////////////////////////
// For removing extensions from file names.
string RemoveExtension(const string& svInput)
{
    string::size_type nPos = svInput.find_last_of('.');
    if (nPos == string::npos)
    {
        return svInput;
    }
    return svInput.substr(0, nPos);
}

///////////////////////////////////////////////////////////////////////////////
// For checking file names equality without extension.
bool HasFileName(const string& svInput, const string& svFileName)
{
    if (RemoveExtension(svInput) == RemoveExtension(svFileName))
    {
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
// For removing the path from file names.
string GetFileName(const string& svInput, bool bRemoveExtension, bool bWindows)
{
    string::size_type nPos;
    if (bWindows)
    {
        nPos = svInput.rfind('\\');
    }
    else
    {
        nPos = svInput.rfind('/');
    }
    if (nPos != string::npos)
    {
        if (bRemoveExtension)
        {
            return RemoveExtension(svInput.substr(nPos + 1));
        }
        return svInput.substr(nPos + 1);
    }
    else // File name is not within a path.
    {
        if (bRemoveExtension)
        {
            return RemoveExtension(svInput);
        }
    }
    return svInput;
}

///////////////////////////////////////////////////////////////////////////////
// For removing file names from the path.
string RemoveFileName(const string& svInput, bool bWindows)
{
    string::size_type nPos;
    if (bWindows)
    {
        nPos = svInput.find_last_of('\\');
    }
    else
    {
        nPos = svInput.find_last_of('/');
    }
    if (nPos == string::npos)
    {
        return "";
    }
    return svInput.substr(0, nPos);
}

///////////////////////////////////////////////////////////////////////////////
// For creating directories for output streams.
string CreateDirectories(string svInput, bool bWindows)
{
    if (bWindows)
    {
        StringReplace(svInput, "\\ \\", "\\");
        StringReplace(svInput, " \\", "");
    }
    else
    {
        StringReplace(svInput, "/ /", "/");
        StringReplace(svInput, " /", "");
    }

    fs::path fspPathOut(svInput);
    string results = fspPathOut.u8string();

    fspPathOut = fspPathOut.parent_path();
    fs::create_directories(fspPathOut);

    return results;
}

///////////////////////////////////////////////////////////////////////////////
// For converting filepaths to windows filepaths.
string ConvertToWinPath(const string& svInput)
{
    char szFilePath[MAX_PATH] = { 0 };
    string results;
    sprintf_s(szFilePath, MAX_PATH, "%s", svInput.c_str());

    // Flip forward slashes in filepath to windows-style backslash
    for (size_t i = 0; i < strlen(szFilePath); i++)
    {
        if (szFilePath[i] == '/')
        {
            szFilePath[i] = '\\';
        }
    }
    return results = szFilePath;
}

///////////////////////////////////////////////////////////////////////////////
// For converting filepaths to unix filepaths.
string ConvertToUnixPath(const string& svInput)
{
    char szFilePath[MAX_PATH] = { 0 };
    string results;
    sprintf_s(szFilePath, MAX_PATH, "%s", svInput.c_str());

    // Flip forward slashes in filepath to windows-style backslash
    for (size_t i = 0; i < strlen(szFilePath); i++)
    {
        if (szFilePath[i] == '\\')
        {
            szFilePath[i] = '/';
        }
    }
    return results = szFilePath;
}

///////////////////////////////////////////////////////////////////////////////
// For encoding data in Base64.
string Base64Encode(const string& svInput)
{
    string results;
    int val = 0, valb = -6;

    for (unsigned char c : svInput)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            results.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6)
    {
        results.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (results.size() % 4)
    {
        results.push_back('=');
    }
    return results;
}

///////////////////////////////////////////////////////////////////////////////
// For decoding data in Base64.
string Base64Decode(const string& svInput)
{
    string results;
    int val = 0, valb = -8;

    vector<int> T(256, -1);
    for (int i = 0; i < 64; i++)
    {
        T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;
    }

    for (unsigned char c : svInput)
    {
        if (T[c] == -1)
        {
            break;
        }
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0)
        {
            results.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return results;
}

///////////////////////////////////////////////////////////////////////////////
// For encoding data in UTF8.
string UTF8Encode(const wstring& wsvInput)
{
    string results;
    int nLen = WideCharToMultiByte(CP_UTF8, 0, wsvInput.c_str(), wsvInput.length(), NULL, 0, NULL, NULL);
    if (nLen > 0)
    {
        results.resize(nLen);
        WideCharToMultiByte(CP_UTF8, 0, wsvInput.c_str(), wsvInput.length(), &results[0], nLen, NULL, NULL);
    }
    return results;
}

///////////////////////////////////////////////////////////////////////////////
// For decoding data in UTF8.
string UTF8Decode(const string& svInput)
{
    //struct destructible_codecvt : public std::codecvt<char32_t, char, std::mbstate_t>
    //{
    //    using std::codecvt<char32_t, char, std::mbstate_t>::codecvt;
    //    ~destructible_codecvt() = default;
    //};
    //std::wstring_convert<destructible_codecvt, char32_t> utf32_converter;
    //return utf32_converter.from_bytes(svInput);
    return "";
}

///////////////////////////////////////////////////////////////////////////////
// For obtaining UTF8 character length.
size_t UTF8CharLength(const uint8_t cInput)
{
    if ((cInput & 0xFE) == 0xFC)
        return 6;
    if ((cInput & 0xFC) == 0xF8)
        return 5;
    if ((cInput & 0xF8) == 0xF0)
        return 4;
    else if ((cInput & 0xF0) == 0xE0)
        return 3;
    else if ((cInput & 0xE0) == 0xC0)
        return 2;
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// For checking if a string is a number.
bool StringIsDigit(const string& svInput)
{
    for (char const& c : svInput)
    {
        if (std::isdigit(c) == 0)
        {
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// For comparing input strings alphabetically.
bool CompareStringAlphabetically(const string& svA, const string& svB)
{
    int i = 0;
    while (svA[i] != '\0' && svA[i] == svB[i])
    {
        i++;
    }

    return (svA[i] - svB[i]) < 0;
}

///////////////////////////////////////////////////////////////////////////////
// For comparing input strings lexicographically.
bool CompareStringLexicographically(const string& svA, const string& svB)
{
    return svA < svB;
}

///////////////////////////////////////////////////////////////////////////////
// For replacing parts of a given string by reference.
bool StringReplace(string& svInput, const string& svFrom, const string& svTo)
{
    string::size_type nPos = svInput.find(svFrom);
    if (nPos == string::npos)
    {
        return false;
    }

    svInput.replace(nPos, svFrom.length(), svTo);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// For replacing parts of a given string by value.
string StringReplaceC(const string& svInput, const string& svFrom, const string& svTo)
{
    string results = svInput;
    string::size_type nPos = results.find(svFrom);

    if (nPos == string::npos)
    {
        return results;
    }

    results.replace(nPos, svFrom.length(), svTo);
    return results;
}

///////////////////////////////////////////////////////////////////////////////
// For escaping special characters in a string.
string StringEscape(const string& svInput)
{
    string results;
    results.reserve(svInput.size());

    for (const char c : svInput)
    {
        switch (c)
        {
        //case '\'':  results += "\\'";  break;
        case '\a':  results += "\\a";  break;
        case '\b':  results += "\\b";  break;
        case '\f':  results += "\\f";  break;
        case '\n':  results += "\\n";  break;
        case '\r':  results += "\\r";  break;
        case '\t':  results += "\\t";  break;
        case '\v':  results += "\\v";  break;
        default:    results += c;      break;
        }
    }
    return results;
}

///////////////////////////////////////////////////////////////////////////////
// For unescaping special characters in a string.
string StringUnescape(const string& svInput)
{
    string results;
    results.reserve(svInput.size());

    for (const char c : svInput)
    {
        switch (c)
        {
        //case '\\':   results += "\'";  break;
        case '\\a':  results += "\a";  break;
        case '\\b':  results += "\b";  break;
        case '\\f':  results += "\f";  break;
        case '\\n':  results += "\n";  break;
        case '\\r':  results += "\r";  break;
        case '\\t':  results += "\t";  break;
        case '\\v':  results += "\v";  break;
        default:     results += c;     break;
        }
    }
    return results;
}

///////////////////////////////////////////////////////////////////////////////
// For converting a string to an array of bytes.
vector<int> StringToBytes(const string& svInput, bool bNullTerminator)
{
    char* pszStringStart = const_cast<char*>(svInput.c_str());
    char* pszStringEnd = pszStringStart + strlen(svInput.c_str());
    vector<int> vBytes = vector<int>{ };

    for (char* pszCurrentByte = pszStringStart; pszCurrentByte < pszStringEnd; ++pszCurrentByte)
    {
        // Dereference character and push back the byte.
        vBytes.push_back(*pszCurrentByte);
    }

    if (bNullTerminator)
    {
        vBytes.push_back(0x0);
    }
    return vBytes;
};

///////////////////////////////////////////////////////////////////////////////
// For converting a string pattern with wildcards to an array of bytes.
vector<int> PatternToBytes(const string& svInput)
{
    char* pszPatternStart = const_cast<char*>(svInput.c_str());
    char* pszPatternEnd = pszPatternStart + strlen(svInput.c_str());
    vector<int> vBytes = vector<int>{ };

    for (char* pszCurrentByte = pszPatternStart; pszCurrentByte < pszPatternEnd; ++pszCurrentByte)
    {
        if (*pszCurrentByte == '?')
        {
            ++pszCurrentByte;
            if (*pszCurrentByte == '?')
            {
                ++pszCurrentByte; // Skip double wildcard.
            }
            vBytes.push_back(-1); // Push the byte back as invalid.
        }
        else
        {
            vBytes.push_back(strtoul(pszCurrentByte, &pszCurrentByte, 16));
        }
    }
    return vBytes;
};

///////////////////////////////////////////////////////////////////////////////
// For converting a integer into digits.
vector<int> IntToDigits(int iValue)
{
    vector<int> vDigits;
    for (; iValue > 0; iValue /= 10)
    {
        vDigits.push_back(iValue % 10);
    }
    std::reverse(vDigits.begin(), vDigits.end());
    return vDigits;
}

///////////////////////////////////////////////////////////////////////////////
// For printing __m128i datatypes.
void PrintM128i8(__m128i in)
{
    alignas(16) uint8_t v[16];
    _mm_store_si128(reinterpret_cast<__m128i*>(v), in);
    printf("v16_u8: %x %x %x %x | %x %x %x %x | %x %x %x %x | %x %x %x %x\n",
        v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7],
        v[8], v[9], v[10], v[11], v[12], v[13], v[14], v[15]);
}
void PrintM128i16(__m128i in)
{
    alignas(16) uint16_t v[8];
    _mm_store_si128(reinterpret_cast<__m128i*>(v), in);
    printf("v8_u16: %x %x | %x %x | %x %x | %x %x\n", v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
}
void PrintM128i32(__m128i in)
{
    alignas(16) uint32_t v[4];
    _mm_store_si128(reinterpret_cast<__m128i*>(v), in);
    printf("v4_u32: %x | %x | %x | %x\n", v[0], v[1], v[2], v[3]);
}
void PrintM128i64(__m128i in)
{
    alignas(16) uint64_t v[2];  // uint64_t might give format-string warnings with %llx; it's just long in some ABIs
    _mm_store_si128(reinterpret_cast<__m128i*>(v), in);
    printf("v2_u64: %llx %llx\n", v[0], v[1]);
}

///////////////////////////////////////////////////////////////////////////////
// For escaping the '%' character for *rintf.
string PrintPercentageEscape(const string& svInput)
{
    string results;
    results.reserve(svInput.size());

    for (const char c : svInput)
    {
        switch (c)
        {
        case '%':  results += "%%";  break;
        default:   results += c;     break;
        }
    }
    return results;
}