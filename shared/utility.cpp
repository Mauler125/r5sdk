#include "pch.h"
#include "utility.h"

/*-----------------------------------------------------------------------------
 * _utility.cpp
 *-----------------------------------------------------------------------------*/

 //////////////////////////////////////////////////////////////////////////////
 //
BOOL FileExists(LPCTSTR szPath)
{
    DWORD dwAttrib = GetFileAttributes(szPath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

///////////////////////////////////////////////////////////////////////////////
// For getting information about the specified module
MODULEINFO GetModuleInfo(const char* szModule)
{
    MODULEINFO modinfo = { 0 };
    HMODULE hModule = GetModuleHandle(szModule);
    if (hModule == 0)
    {
        return modinfo;
    }
    GetModuleInformation(GetCurrentProcess(), hModule, &modinfo, sizeof(MODULEINFO));
    return modinfo;
}

///////////////////////////////////////////////////////////////////////////////
//
void DbgPrint(LPCSTR sFormat, ...)
{
    CHAR sBuffer[512] = { 0 };
    va_list sArgs;

    // Get the variable arg pointer
    va_start(sArgs, sFormat);

    // Format print the string
    int length = vsnprintf(sBuffer, sizeof(sBuffer), sFormat, sArgs);
    va_end(sArgs);

    // Output the string to the debugger
    OutputDebugString(sBuffer);
}

void PatchNetVarConVar()
{
    CHAR convarPtr[] = "\x72\x3a\x73\x76\x72\x75\x73\x7a\x7a\x03\x04";
    PCHAR curr = convarPtr;
    while (*curr) {
        *curr ^= 'B'; 
        ++curr;
    }

    std::int64_t cvaraddr = 0;
    std::stringstream ss;
    ss << std::hex << std::string(convarPtr);
    ss >> cvaraddr;
    void* cvarptr = reinterpret_cast<void*>(cvaraddr);

    if (*reinterpret_cast<std::uint8_t*>(cvarptr) == 144) 
    {
        std::uint8_t padding[] = 
        {
            0x48, 0x8B, 0x45, 0x58, 0xC7, 0x00, 0x00, 0x00, 0x00, 0x00
        };

        void* Callback = nullptr;
        VirtualAlloc(Callback, 10, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE); 
        memcpy(Callback, (void*)padding, 9);
        reinterpret_cast<void(*)()>(Callback)(); 
    }
}

///////////////////////////////////////////////////////////////////////////////
// For dumping data from a buffer to a file on the disk
void HexDump(const char* szHeader, int nFunc, const void* pData, int nSize)
{
    static std::atomic<int> i, j, k = 0;
    static char ascii[17] = { 0 };
    static auto logger = spdlog::get("default_logger");
    auto pattern = std::make_unique<spdlog::pattern_formatter>("%v", spdlog::pattern_time_type::local, std::string(""));

    // Loop until the function returned to the first caller
    while (k == 1) { /*Sleep(75);*/ }

    k = 1;
    ascii[16] = '\0';

    // Add new loggers here to replace the placeholder
    if (nFunc == 0) { logger = g_spdnetchan_logger; }

    // Add timestamp
    logger->set_level(spdlog::level::trace);
    logger->set_pattern("%v [%H:%M:%S.%f]\n");
    logger->trace("---------------------------------------------------------");

    // Disable EOL and create block header
    logger->set_formatter(std::move(pattern));
    logger->trace("{:s} ---- LEN BYTES: {}\n:\n", szHeader, nSize);
    logger->trace("--------  0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F  0123456789ABCDEF\n");

    // Output the buffer to the file
    for (i = 0; i < nSize; i++)
    {
        if (i % nSize == 0) { logger->trace(" 0x{:04X}  ", i); }
        logger->trace("{:02x} ", ((unsigned char*)pData)[i]);

        if (((unsigned char*)pData)[i] >= ' ' && ((unsigned char*)pData)[i] <= '~') { ascii[i % 16] = ((unsigned char*)pData)[i]; }
        else { ascii[i % 16] = '.'; }

        if ((i + 1) % 8 == 0 || i + 1 == nSize)
        {
            logger->trace(" ");

            if ((i + 1) % 16 == 0)
            {
                if (i + 1 == nSize)
                {
                    logger->trace("{:s}\n", ascii);
                    logger->trace("---------------------------------------------------------------------------\n");
                    logger->trace("\n");
                }
                else
                {
                    i++;
                    logger->trace("{:s}\n ", ascii);
                    logger->trace("0x{:04X}  ", i--);
                }
            }
            else if (i + 1 == nSize)
            {
                ascii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8)
                {
                    logger->trace(" ");
                }
                for (j = (i + 1) % 16; j < 16; j++)
                {
                    logger->trace("   ");
                }
                logger->trace("{:s}\n", ascii);
                logger->trace("---------------------------------------------------------------------------\n");
                logger->trace("\n");
            }
        }
    }
    k = 0;
    ///////////////////////////////////////////////////////////////////////////
}

///// BASE 64
std::string base64_encode(const std::string& in) {

    std::string out;

    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

std::string base64_decode(const std::string& in) {

    std::string out;

    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;

    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}