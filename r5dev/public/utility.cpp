/*-----------------------------------------------------------------------------
 * _utility
 *-----------------------------------------------------------------------------*/

#include "core/stdafx.h"
#include "core/logdef.h"
#include "public/include/utility.h"

///////////////////////////////////////////////////////////////////////////////
// For checking if a specific file exists.
BOOL FileExists(const char* szPath)
{
    return std::filesystem::exists(szPath);
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
// For finding a byte pattern in memory of the process.
BOOL Compare(const unsigned char* pData, const unsigned char* szPattern, const char* szMask)
{
    for (; *szMask; ++szMask, ++pData, ++szPattern)
    {
        if (*szMask == 'x' && *pData != *szPattern)
        {
            return false;
        }
    }
    return (*szMask) == NULL;
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

        spdlog::error("{}\n", messageBuffer);
        LocalFree(messageBuffer);
    }
}

///////////////////////////////////////////////////////////////////////////////
// For dumping data from a buffer to a file on the disk
void HexDump(const char* szHeader, int nFunc, const void* pData, int nSize)
{
    static unsigned char szAscii[17] = {};
    static std::atomic<int> i = {}, j = {}, k = {};
    static std::shared_ptr<spdlog::logger> logger = spdlog::get("default_logger");

    // Loop until the function returned to the first caller.
    while (k == 1) { /*Sleep(75);*/ }

    k = 1;
    szAscii[16] = '\0';

    // Add new loggers here to replace the placeholder.
    if (nFunc == 0) { logger = spdlog::get("netchan_packet_logger"); }

    // Add timestamp.
    logger->set_level(spdlog::level::trace);
    logger->set_pattern("%v [%H:%M:%S.%f]\n");
    logger->trace("---------------------------------------------------------");

    // Disable EOL and create block header.
    logger->set_pattern("%v");
    logger->trace("{:s} ---- LEN BYTES: {}\n:\n", szHeader, nSize);
    logger->trace("--------  0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F  0123456789ABCDEF\n");

    // Output the buffer to the file.
    for (i = 0; i < nSize; i++)
    {
        if (i % nSize == 0) { logger->trace(" 0x{:04X}  ", i); }
        logger->trace("{:02x} ", ((unsigned char*)pData)[i]);

        if (((unsigned char*)pData)[i] >= ' ' && ((unsigned char*)pData)[i] <= '~') { szAscii[i % 16] = ((unsigned char*)pData)[i]; }
        else { szAscii[i % 16] = '.'; }

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
    k = 0;
    ///////////////////////////////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////
// For encoding data in base64.
std::string Base64Encode(const std::string& in)
{
    std::string results;
    int val = 0, valb = -6;

    for (unsigned char c : in)
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
// For decoding data in base64.
std::string Base64Decode(const std::string& in)
{
    std::string results;
    int val = 0, valb = -8;

    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++)
    {
        T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;
    }

    for (unsigned char c : in)
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
// For replacing parts of a given string.
bool StringReplace(std::string& str, const std::string& from, const std::string& to)
{
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
    {
        return false;
    }

    str.replace(start_pos, from.length(), to);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// For creating directories for output streams.
std::string CreateDirectories(std::string svFilePath)
{
    std::filesystem::path fspPathOut(svFilePath);
    std::string results = fspPathOut.u8string();

    StringReplace(svFilePath, "\\ \\", "\\");
    fspPathOut = fspPathOut.parent_path();

    std::filesystem::create_directories(fspPathOut);

    return results;
}

///////////////////////////////////////////////////////////////////////////////
// For converting filepaths to windows filepaths.
std::string ConvertToWinPath(const std::string& input)
{
    char szFilePath[MAX_PATH] = { 0 };
    std::string results;
    sprintf_s(szFilePath, MAX_PATH, "%s", input.c_str());

    // Flip forward slashes in filepath to windows-style backslash
    for (int i = 0; i < strlen(szFilePath); i++)
    {
        if (szFilePath[i] == '/')
        {
            szFilePath[i] = '\\';
        }
    }
    return results = szFilePath;
}

///////////////////////////////////////////////////////////////////////////////
// For escaping special characters in a string.
std::string StringEscape(const std::string& input)
{
    std::string results;
    results.reserve(input.size());
    for (const char c : input)
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
std::string StringUnescape(const std::string& input)
{
    std::string results;
    results.reserve(input.size());
    for (const char c : input)
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
