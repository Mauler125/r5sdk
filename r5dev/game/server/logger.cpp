#include "core/stdafx.h"
#include "engine/server/server.h"
#include "game/shared/vscript_shared.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"
//#include "vscript_server.h"
#include "game/server/logger.h"
#include "thirdparty/curl/include/curl/curl.h"
#include <engine/host_state.h>
#include <networksystem/listmanager.h>

#include <tiny-aes/aes.hpp>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <queue>
#include <thread>
#include <condition_variable>
#include <iostream>
#include <cstring>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <string>
#include <vector>
#include <deque>
#include <utility>
#include <atomic>
#include "engine/server/vengineserver_impl.h"



namespace LOGGER {



    namespace aesc {



                        // function to convert hex to bytes for aes // updated

                            std::vector<uint8_t> hex2bytes(const std::string& hex) {
                                if (hex.length() % 2 != 0) {
                                    throw std::invalid_argument("Hex string has an odd length");
                                }

                                std::vector<uint8_t> bytes;

                                for (unsigned int i = 0; i < hex.length(); i += 2) {
                                    std::string byteString = hex.substr(i, 2);
                                    char* end;
                                    long byte = std::strtol(byteString.c_str(), &end, 16);

                                    if (*end != '\0' || byte < 0 || byte > 0xFF) {
                                        throw std::invalid_argument("Invalid hex value: " + byteString);
                                    }

                                    bytes.push_back(static_cast<uint8_t>(byte));
                                }

                                return bytes;
                            }


                            //  initialize here 
                            //void LOGGER::aesc::AES_init_ctx_iv(struct AES_ctx* ctx, const uint8_t* key, const uint8_t* iv);

                            const std::string base64_chars =
                                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "abcdefghijklmnopqrstuvwxyz"
                                "0123456789+/";

                            std::string bbase64Encode(std::vector<uint8_t> bytes_to_encode) {
                                std::string ret;
                                int i = 0;
                                int j = 0;
                                uint8_t char_array_3[3] = { 0 };
                                uint8_t char_array_4[4] = { 0 };

                                for (unsigned int in_len = 0; in_len < bytes_to_encode.size(); ++in_len) {
                                    char_array_3[i++] = bytes_to_encode[in_len];
                                    if (i == 3) {
                                        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                                        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                                        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                                        char_array_4[3] = char_array_3[2] & 0x3f;

                                        for (i = 0; (i < 4); i++)
                                            ret += base64_chars[char_array_4[i]];
                                        i = 0;
                                    }
                                }

                                if (i) {
                                    for (j = i; j < 3; j++)
                                        char_array_3[j] = '\0';

                                    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                                    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                                    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                                    char_array_4[3] = char_array_3[2] & 0x3f;

                                    for (j = 0; (j < i + 1); j++)
                                        ret += base64_chars[char_array_4[j]];

                                    while ((i++ < 3))
                                        ret += '=';
                                }

                                return ret;
                            }


                            // aes encrypt with tiny aes function
                            std::string doEncrypt(const std::string& plainText, const std::vector<uint8_t>& keyBytes, const std::vector<uint8_t>& ivBytes) {
                                
                                // PKCS7 padding
                                std::vector<uint8_t> plainTextBytes(plainText.begin(), plainText.end());
                                size_t length = plainTextBytes.size();
                                size_t padded_length = length + AES_BLOCKLEN - length % AES_BLOCKLEN;
                                plainTextBytes.resize(padded_length);

                                int padding = static_cast<int>(padded_length - length);
                                for (int i = 0; i < padding; i++) {
                                    plainTextBytes[length + i] = static_cast<uint8_t>(padding);
                                }

                                // encrypt - initialize here
                                struct AES_ctx ctx;

                                // initialize here
                                AES_init_ctx_iv(&ctx, keyBytes.data(), ivBytes.data());
                                if (padded_length > UINT32_MAX) {
                                    throw std::runtime_error("padded_length is too large to be safely cast to uint32_t");
                                }

                                AES_CBC_encrypt_buffer(&ctx, plainTextBytes.data(), static_cast<uint32_t>(padded_length));

                                // convert to base64
                                std::string cipherTextBase64 = bbase64Encode(plainTextBytes);

                                return cipherTextBase64;
                            }


     }

                    //declare vars
                    std::atomic<bool> finished{false};
                    std::mutex finished_mtx;
                    std::mutex cache_mtx;
                    std::unordered_map<std::string, std::string> sanitizedFilenameCache;
                    std::mutex file_mtx;
                    std::thread apiThread;

                    


                // get recent log file from directory [may allow server admin controlled dir in future]
                std::string getLatestFile(const std::string& directoryPath) {
                    std::filesystem::path latestFilePath;
                    std::filesystem::file_time_type latestTime;
                    std::error_code ec;

                    if (!std::filesystem::exists(directoryPath, ec)) {
                        Error(eDLL_T::SERVER,NO_ERROR, "!!! Directory platform/logevents does not exist: %s\n", ec);
                        return "";
                    }

                    if (ec) {
                        Error(eDLL_T::SERVER,NO_ERROR, "!!! Failed to check if directory platform/logevents exists: %s\n", ec);
                        return "";
                    }

                    for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
                        if (entry.is_regular_file()) {
                            if (latestFilePath.empty()) {
                                latestFilePath = entry.path();
                                latestTime = std::filesystem::last_write_time(entry);
                            }
                            else {
                                auto ftime = std::filesystem::last_write_time(entry);
                                if (ftime > latestTime) {
                                    latestTime = ftime;
                                    latestFilePath = entry.path();
                                }
                            }
                        }
                    }

                    if (latestFilePath.empty()) {
                        Error(eDLL_T::SERVER,NO_ERROR, "!!! No logs found in plaform/eventlogs\n");
                        return "";
                    }

                    return latestFilePath.string();
                }

                // clean up log file name

                std::string sanitizeFilename(const std::string& filename) {
                    std::string sanitized;
                    for (char c : filename) {
                        // sanitize for numeric only
                        if ('0' <= c && c <= '9') {
                            sanitized += c;
                        }
                    }

                    if (sanitized.empty()) {
                        // shouldn't be empty right? make sure
                        sanitized = "plznamelog";
                    }

                    // name it .json
                    sanitized += ".json";

                    return sanitized;
                }

                // get machine ip for log API

                std::string getServerIP() {
                    char hostbuffer[256];
                    char IPbuffer[INET6_ADDRSTRLEN] = "";
                    struct addrinfo hints, * info, * p;
                    int localHostname;

                    localHostname = gethostname(hostbuffer, sizeof(hostbuffer));
                    if (localHostname == -1) {
                        perror("gethostname");
                        return "";
                    }

                    memset(&hints, 0, sizeof hints);
                    hints.ai_family = AF_UNSPEC;
                    hints.ai_socktype = SOCK_STREAM;

                    if (getaddrinfo(hostbuffer, NULL, &hints, &info) != 0) {
                        perror("getaddrinfo");
                        return "";
                    }

                    for (p = info; p != NULL; p = p->ai_next) {
                        void* addr;
                        if (p->ai_family == AF_INET) {
                            struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
                            addr = &(ipv4->sin_addr);
                            inet_ntop(AF_INET, addr, IPbuffer, sizeof(IPbuffer));
                            break;  // first local ipv4 address found
                        }
                        else if (p->ai_family == AF_INET6) {
                            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
                            addr = &(ipv6->sin6_addr);
                            if (IPbuffer[0] == 0) {  // ipv6 if not ipv4
                                inet_ntop(AF_INET6, addr, IPbuffer, sizeof(IPbuffer));
                            }
                        }
                    }

                    freeaddrinfo(info);

                    std::cout << "Hostname: " << hostbuffer << ", IP: " << IPbuffer << "\n";

                    return IPbuffer;
                }





               





                // function to split a string

                std::vector<std::string> splitString(std::string str, const std::string& delimiter) {
                    std::vector<std::string> lines;
                    size_t pos = 0;
                    std::string token;
                    while ((pos = str.find(delimiter)) != std::string::npos) {
                        token = str.substr(0, pos);
                        lines.push_back(token);
                        str.erase(0, pos + delimiter.length());
                    }
                    lines.push_back(str);
                    return lines;
                }


                // actual queue/write thread for log

                std::mutex mtx;
                std::condition_variable cvLog;
                std::queue<std::pair<std::string, std::string>> logQueue;
                std::thread logThread;


                // Full i/o
                void writeBufferToFile(const std::deque<std::pair<std::string, std::string>>& buffer) {
                    for (const auto& logPair : buffer) {
                        const std::string& filePath = logPair.first;
                        const std::string& logMessage = logPair.second;
                        std::ofstream logFile;
                        logFile.open(filePath, std::ios_base::app);
                        if (!logFile) {
                            Error(eDLL_T::SERVER, NO_ERROR, "Unable to open log file: '%s'\n", filePath.c_str());
                            return;
                        }
                        logFile << logMessage << "\n";
                        logFile.flush();
                        logFile.close();
                    }
                }

                void logToFile() {
                    try {
                        while (true) {
                            std::unique_lock<std::mutex> lock(mtx);
                            cvLog.wait(lock, [] { return !logQueue.empty() || finished; });

                            std::deque<std::pair<std::string, std::string>> buffer;
                            while (!logQueue.empty()) {
                                buffer.push_back(logQueue.front());
                                logQueue.pop();

                                if (buffer.size() == 3) { // buffer max of 3 - maybe increase for better performance
                                    writeBufferToFile(buffer);
                                    buffer.clear();
                                }
                            }

                            //flush all if no more queues are to be made
                            if (!buffer.empty()) {
                                writeBufferToFile(buffer);
                                buffer.clear();
                            }

                            lock.unlock();
                            if (finished) {
                                break;
                            }
                        }
                    }
                    catch (const std::exception& e) {
                        Error(eDLL_T::SERVER,NO_ERROR, "Exception in logToFile thread: %s\n", e.what());
                    }
                    catch (...) {
                        Error(eDLL_T::SERVER,NO_ERROR, "Unknown exception in logToFile thread.\n");
                    }
                }


                // called when logevent is flagged with dir check bool

                void startLogging() {
                    std::lock_guard<std::mutex> slock(finished_mtx);
                    finished = false;
                    try {
                        logThread = std::thread(logToFile);
                    }
                    catch (const std::exception& e) {
                        Error(eDLL_T::SERVER, NO_ERROR, "Exception when starting logToFile thread: %s\n", e.what());
                    }
                    catch (...) {
                        Error(eDLL_T::SERVER, NO_ERROR, "Unknown exception when starting logToFile thread.\n");
                    }
                }

                // communitate with log api

                size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
                    size_t totalSize = size * nmemb;
                    userp->append((char*)contents, totalSize);
                    return totalSize;
                }


                // replace_all function for use in url encoder

                std::string replace_all(std::string str, const std::string& from, const std::string& to) {
                    size_t start_pos = 0;
                    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
                        str.replace(start_pos, from.length(), to);
                        start_pos += to.length();
                    }
                    return str;
                }

                // encode for specific r5r.dev api usage

                std::string url_encode(const std::string& value) {
                    CURL* curl = curl_easy_init();
                    std::string retStr;
                    if (curl) {
                        std::string new_value = replace_all(value, "\n", "<br/>");
                        char* output = curl_easy_escape(curl, new_value.c_str(), static_cast<int>(new_value.length()));
                        if (output) {
                            retStr = std::string(output);
                            curl_free(output);
                        }
                        curl_easy_cleanup(curl);
                    }
                    return retStr;
                }

                // ship file to log api

                void sendLogToAPI(const std::string& logdata) {
                    CURL* curl;
                    CURLcode res;

                    curl_global_init(CURL_GLOBAL_DEFAULT);
                    DevMsg(eDLL_T::SERVER, "Curl initialized...\n");

                    curl = curl_easy_init();
                    if (curl) {
                        curl_easy_setopt(curl, CURLOPT_URL, "https://r5r.dev/api/tracker.php");

                        std::string filename = getLatestFile("platform/eventlogs");

                        if (filename.empty()) {
                            Error(eDLL_T::SERVER, NO_ERROR, "No log found. Aborting API connection...\n");
                            return;
                        }

                        // sanitize
                        std::string sanitizedFilename = sanitizeFilename(filename);

                        // extract just the sanitized filename to use as the MatchID
                        std::string matchID;
                        std::size_t pos = sanitizedFilename.rfind(".");
                        if (pos != std::string::npos) {
                            matchID = sanitizedFilename.substr(0, pos);
                        }
                        else {
                            matchID = sanitizedFilename;
                        }

                        std::string serverName = g_pServerListManager->m_Server.m_svHostName;
                        std::string serverMap = g_pServerListManager->m_Server.m_svHostMap;

                        DevMsg(eDLL_T::SERVER, "Server name: %s \n", serverName.c_str());
                        DevMsg(eDLL_T::SERVER, "Server map: %s \n", serverMap.c_str());
                        DevMsg(eDLL_T::SERVER, "MatchID to ship: %s \n", matchID.c_str());

                        std::string logdata_url_encoded = url_encode(logdata);

                        std::string postData = "ServerNAME=" + serverName +
                            "&ServerADDR=" + getServerIP() +
                            "&GameTYPE=" + g_pServerListManager->m_Server.m_svPlaylist +
                            "&MatchID=" + matchID +
                            "&Logdata=" + logdata_url_encoded +
                            "&Map=" + serverMap +
                            "&CODE=" + "vypKpraa5wNk4YAes2QF"; //API code is generated for each major api version change

                        std::string readBuffer;
                        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

                        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
                        curl_easy_setopt(curl, CURLOPT_POST, 1L);


                        res = curl_easy_perform(curl);


                        if (res != CURLE_OK)
                            Error(eDLL_T::SERVER, NO_ERROR, "curl failed: %s\n", curl_easy_strerror(res));
                        else
                            Warning(eDLL_T::SERVER, "Response: %s\n", readBuffer.c_str());

                        curl_easy_cleanup(curl);
                    }

                    curl_global_cleanup();
                }




                // shut down the logger thread and ship file. called from squirrel vm



                void stopLogging(bool sendToAPI) {
                    std::lock_guard<std::mutex> slock(finished_mtx);
                    if (!logThread.joinable()) {
                        DevMsg(eDLL_T::RTECH, "No active logging thread to stop.\n");
                        return;
                    }

                    Warning(eDLL_T::SERVER, "Logger thread stopping...\n");

                    {
                        std::unique_lock<std::mutex> lock(mtx);
                        finished = true;
                        cvLog.notify_all();
                    }

                    logThread.join();

                    {
                        std::lock_guard<std::mutex> cacheLock(cache_mtx);
                        sanitizedFilenameCache.clear();
                    }

                    if (sendToAPI) {
                        std::string logFilePath = getLatestFile("platform/eventlogs");
                        std::string logData;
                        {
                            std::lock_guard<std::mutex> flock(file_mtx);
                            std::ifstream logFile(logFilePath);
                            if (logFile.is_open()) {
                                std::stringstream ss;
                                ss << logFile.rdbuf();
                                logData = ss.str();
                                logFile.close();
                                Warning(eDLL_T::SERVER, "Attempting to send...");
                            }
                            else {
                                Error(eDLL_T::SERVER, NO_ERROR, "!!! Couldn't open logfile for shipping.\n");
                                return;
                            }
                        }

                        DevMsg(eDLL_T::SERVER, " .. Shipping logfile to tracker https://r5r.dev \n");

                        {
                            std::lock_guard<std::mutex> apilock(file_mtx);
                            if (apiThread.joinable()) {
                                apiThread.join();
                            }
                            apiThread = std::thread(sendLogToAPI, logData);
                        }
                    }
                    else {
                        Error(eDLL_T::SERVER,NO_ERROR, "Stats shipping omitted; game shutdown early or disabled\n");
                    }
                }




                void LogEvent(const char* filename, const char* logString, bool checkDir, bool encrypt) {
                    std::filesystem::path dirPath = "platform/eventlogs";
                    std::lock_guard<std::mutex> cacheLock(cache_mtx);
                    std::unordered_map<std::string, std::string>::iterator it = sanitizedFilenameCache.find(filename);
                    std::string sanitizedFilename;

                    if (it != sanitizedFilenameCache.end()) {
                        sanitizedFilename = it->second;
                    }
                    else {
                        sanitizedFilename = sanitizeFilename(filename);
                        sanitizedFilenameCache[filename] = sanitizedFilename;
                    }

                    if (sanitizedFilename.length() > 80) {
                        Error(eDLL_T::SERVER, NO_ERROR, "Log name too long, aborting write '%s'\n");
                        return;
                    }

                    std::filesystem::path filePath = dirPath / sanitizedFilename;
                    std::string start_lines;

                    if (checkDir) {
                        if (logThread.joinable()) {
                            std::lock_guard<std::mutex> slock(finished_mtx);
                            finished = true;
                            logThread.join();
                        }

                        try {
                            startLogging();
                            Warning(eDLL_T::SERVER, "::::::::::::::::::::::::::::: Logging thread started :::::::\n");
                        }
                        catch (...) {
                            Error(eDLL_T::SERVER, NO_ERROR, "Error starting logging thread.\n");
                            return;
                        }

                        try {
                            if (!std::filesystem::exists(dirPath)) {
                                std::filesystem::create_directory(dirPath);
                                DevMsg(eDLL_T::SERVER, "Success creating new EventLog directory: '%s'\n", dirPath.c_str());
                            }
                        }
                        catch (const std::filesystem::filesystem_error&) {
                            Error(eDLL_T::SERVER, NO_ERROR, "Unable to create EventLog directory: '%s'. Check that platform folder is not read only\n", dirPath.c_str());
                            return;
                        }

                        std::string serverName = g_pServerListManager->m_Server.m_svHostName;
                        std::string serverMap = g_pServerListManager->m_Server.m_svHostMap;
                        std::string serverPlaylist = g_pServerListManager->m_Server.m_svPlaylist;
                        std::string serverIP = getServerIP();

                        start_lines += "\n|#Gameversion:rc_2.7";
                        start_lines += "\n|#Gametype:" + serverPlaylist;
                        start_lines += "\n|#ServerADDR:" + serverIP;
                        start_lines += "\n|#ServerName:" + serverName;
                        start_lines += "\n|#ServerMAP:" + serverMap + "\n";

                        if (encrypt) {
                            std::vector<uint8_t> keyHex = LOGGER::aesc::hex2bytes("{REDACTED}");
                            std::vector<uint8_t> ivHex = LOGGER::aesc::hex2bytes("{REDACTED}");

                            start_lines = LOGGER::aesc::doEncrypt(start_lines, keyHex, ivHex);
                        }
                    }

                    std::vector<std::string> lines = splitString(logString, "\n");

                    if (encrypt) {
                        std::vector<uint8_t> keyHex = LOGGER::aesc::hex2bytes("{REDACTED}");
                        std::vector<uint8_t> ivHex = LOGGER::aesc::hex2bytes("{REDACTED}");

                        for (std::string& line : lines) {
                            line = LOGGER::aesc::doEncrypt(line, keyHex, ivHex);
                        }
                    }

                    if (checkDir) {
                        std::lock_guard<std::mutex> lock(mtx);
                        logQueue.push(std::make_pair(filePath.string(), start_lines));
                    }

                    for (const std::string& line : lines) {
                        std::lock_guard<std::mutex> lock(mtx);
                        logQueue.push(std::make_pair(filePath.string(), line));
                    }

                    cvLog.notify_all();
                }

}