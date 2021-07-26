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
// For finding a byte pattern in memory of the game process

std::uint8_t* PatternScan(const char* module, const char* signature)
{
    static auto PatternToBytes = [](const char* pattern)
    {
        char* PatternStart = const_cast<char*>(pattern); // Cast const away and get start of pattern.
        char* PatternEnd = PatternStart + std::strlen(pattern); // Get end of pattern.

        std::vector<std::int32_t> Bytes = std::vector<std::int32_t>{ }; // Initialize byte vector.

        for (char* CurrentByte = PatternStart; CurrentByte < PatternEnd; ++CurrentByte)
        {
            if (*CurrentByte == '?') // Is current char(byte) a wildcard?
            {
                ++CurrentByte; // Skip 1 character.

                if (*CurrentByte == '?') // Is it a double wildcard pattern?
                    ++CurrentByte; // If so skip the next space that will come up so we can reach the next byte.

                Bytes.push_back(-1); // Push the byte back as invalid.
            }
            else
            {
                // https://stackoverflow.com/a/43860875/12541255
                // Here we convert our string to a unsigned long integer. We pass our string then we use 16 as the base because we want it as hexadecimal.
                // Afterwards we push the byte into our bytes vector.
                Bytes.push_back(std::strtoul(CurrentByte, &CurrentByte, 16));
            }
        }
        return Bytes;
    };

    const MODULEINFO mInfo = GetModuleInfo(module); // Get module info.
    const DWORD64 SizeOfModule = (DWORD64)mInfo.SizeOfImage; // Grab the module size.
    std::uint8_t* ScanBytes = reinterpret_cast<std::uint8_t*>(mInfo.lpBaseOfDll); // Get the base of the module.

    const std::vector<int> PatternBytes = PatternToBytes(signature); // Convert our pattern to a byte array.
    const std::pair BytesInfo = std::make_pair(PatternBytes.size(), PatternBytes.data()); // Get the size and data of our bytes.

    for (DWORD i = 0ul; i < SizeOfModule - BytesInfo.first; ++i)
    {
        bool FoundAddress = true;

        for (DWORD j = 0ul; j < BytesInfo.first; ++j)
        {
            // If either the current byte equals to the byte in our pattern or our current byte in the pattern is a wildcard
            // our if clause will be false.
            if (ScanBytes[i + j] != BytesInfo.second[j] && BytesInfo.second[j] != -1)
            {
                FoundAddress = false;
                break;
            }
        }

        if (FoundAddress) 
        {
            return &ScanBytes[i];
        }

    }

    return nullptr;
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
    logger->set_pattern("%v [%H:%M:%S.%f]");
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