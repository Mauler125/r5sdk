#pragma once
#include <tiny-aes/aes.hpp>
#include "core/stdafx.h"
#include "engine/server/server.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"
#include "game/server/logger.h"
#include "thirdparty/curl/include/curl/curl.h"
#include <engine/host_state.h>
#include <networksystem/listmanager.h>
#include <game/shared/vscript_shared.h>
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
const std::string SERVER_V = "rc_2.4.5";
const std::string API_KEY = "Kfvtu2TSNKQ7S2pP";

namespace LOGGER {

   


    size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        size_t totalSize = size * nmemb;
        userp->append((char*)contents, totalSize);
        return totalSize;
    }


     /*********************************
     /       API END GAME UPDATES
     /********************************/

    void EndMatchUpdate(const std::string& recap, const std::string& DISCORD_HOOK) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            Error(eDLL_T::SERVER, NO_ERROR, "Failed to initialize curl\n");
            return;
        }

        LOGGER::Logger& logger = LOGGER::Logger::getInstance();
        std::string filename = logger.getLatestFile("platform/eventlogs");
        std::string sanitizedFilename = logger.sanitizeFilename(filename);

        if (filename.empty()) {
            Error(eDLL_T::SERVER, NO_ERROR, "No log found. Aborting API connection...\n");
            return;
        }

        // extract just the sanitized filename to use as the MatchID
        std::string matchID;
        std::size_t pos = sanitizedFilename.rfind(".");
        if (pos != std::string::npos) {
            matchID = sanitizedFilename.substr(0, pos);
        }
        else {
            matchID = sanitizedFilename;
        }

        std::string serverName = ::IsDedicated() ? hostname->GetString() : g_pServerListManager->m_Server.m_svHostName;

        std::string postData = "servername=" + serverName +
            "&matchID=" + matchID +
            "&recap=" + recap +
            "&DISCORD_HOOK=" + DISCORD_HOOK +
            "&KEY=" + API_KEY;

        curl_easy_setopt(curl, CURLOPT_URL, "https://r5r.dev/api/endmatch.php");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            Error(eDLL_T::SERVER, NO_ERROR, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
    }

    const std::string NOTIFY_END_OF_MATCH(const std::string& recap, const std::string& DISCORD_HOOK) {
        std::thread endMatchThread(EndMatchUpdate, recap, DISCORD_HOOK);
        endMatchThread.detach();
        return "1";
    }

    /*********************************
    /     API PLAYER COUNT UPDATES
    /********************************/

    const std::string UPDATE_PLAYER_COUNT(const std::string& action, const std::string& player, const std::string& OID, const std::string& count, const std::string& DISCORD_HOOK) {
        if (action.empty() || player.empty()) {
            return "0";
        }

        // launch thread
        std::thread updateThread(PlayerCountUpdate, action, player, OID, count, DISCORD_HOOK);
        updateThread.detach();

        return "1"; 
    }

    // separate thread
    void PlayerCountUpdate(const std::string& action, const std::string& player, const std::string& OID, const std::string& count, const std::string& DISCORD_HOOK) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            Error(eDLL_T::SERVER, NO_ERROR, "Failed to initialize curl\n");
            return;
        }

        std::string servername = ::IsDedicated() ? hostname->GetString() : g_pServerListManager->m_Server.m_svHostName;

        std::string postData =
            "servername=" + servername +
            "&action=" + action +
            "&player_name=" + player +
            "&OID=" + OID +
            "&current_count=" + count +
            "&DISCORD_HOOK=" + DISCORD_HOOK +
            "&KEY=" + API_KEY;

        curl_easy_setopt(curl, CURLOPT_URL, "https://r5r.dev/api/playercount.php");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            Error(eDLL_T::SERVER, NO_ERROR, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
    }

    /*********************************
    /     API VERIFY EA ACCOUNT
    /********************************/


    const std::string VERIFY_EA_ACCOUNT(const std::string& token, const std::string& OID, const std::string& ea_name) {
        if (token.empty() || ea_name.empty()) {
            return "0";
        }

        CURL* curl = curl_easy_init();
        if (!curl) {
            Error(eDLL_T::SERVER, NO_ERROR, "Failed to initialize curl\n");
            return "8";
        }

        std::string postData =
            "token=" + token +
            "&ea_acc=" + ea_name +
            "&OID=" + OID +
            "&KEY=" + API_KEY;

        std::string readBuffer;
        curl_easy_setopt(curl, CURLOPT_URL, "https://r5r.dev/api/verify.php");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            Error(eDLL_T::SERVER, NO_ERROR, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            return "8";
        }

        Warning(eDLL_T::SERVER, "Response for %s: %s\n", ea_name.c_str(), readBuffer.c_str());
        return readBuffer.c_str();
    }



    const std::string LOGGER::Encryption::base64_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";


        // created with eObj && only encryption bool true
        std::vector<uint8_t> LOGGER::Encryption::hex2bytes(const std::string& hex) {
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

        // created with eObj && only encryption bool true
        std::string LOGGER::Encryption::bbase64Encode(std::vector<uint8_t> bytes_to_encode) {
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

        // created with eObj && only encryption bool true
        std::string LOGGER::Encryption::doEncrypt(const std::string& plainText, const std::vector<uint8_t>& keyBytes, const std::vector<uint8_t>& ivBytes) {
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

  
    /*********************************
    /     Logger instance
    /********************************/

    // blank constructor
    Logger::Logger() {
        
    }

    // destructor 
    Logger::~Logger() {
        stopLoggingThread();
    }

    // one instance only
    Logger& Logger::getInstance() {
        static Logger instance;
        return instance;
    }


    /*********************************
    /     Utility Functions
    /********************************/

    std::string LOGGER::Logger::getLatestFile(const std::string& directoryPath) {
        std::filesystem::path latestFilePath;
        std::filesystem::file_time_type latestTime;
        std::error_code ec;

        if (!std::filesystem::exists(directoryPath, ec)) {
            Error(eDLL_T::SERVER, NO_ERROR, "!!! Directory platform/logevents does not exist: %s\n", ec);
            return "";
        }

        if (ec) {
            Error(eDLL_T::SERVER, NO_ERROR, "!!! Failed to check if directory platform/logevents exists: %s\n", ec);
            return "";
        }

        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (entry.is_regular_file()) {
                if (latestFilePath.empty()) {
                    latestFilePath = entry.path();
                    latestTime = std::filesystem::last_write_time(entry);
                }
                else {
                    std::filesystem::file_time_type ftime = std::filesystem::last_write_time(entry);
                    if (ftime > latestTime) {
                        latestTime = ftime;
                        latestFilePath = entry.path();
                    }
                }
            }
        }

        if (latestFilePath.empty()) {
            Error(eDLL_T::SERVER, NO_ERROR, "!!! No log files found in platform/eventlogs\n");
            return "";
        }

        return latestFilePath.string();
    }

    // sanitize just in case
    std::string LOGGER::Logger::sanitizeFilename(const std::string& filename) {
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


    // function to split a string

    std::vector<std::string> LOGGER::Logger::splitString(std::string str, const std::string& delimiter) {
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


    /*********************************
    /     Tracker & LogEvent Thread
    /********************************/

    // ship to stats server
    void LOGGER::Logger::sendLogToAPI(const std::string& logdata) {
        CURL* curl;
        CURLcode res;


        curl = curl_easy_init();
        if (curl) {

            DevMsg(eDLL_T::SERVER, "Curl initialized...\n");
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

            std::string serverName = ::IsDedicated() ? hostname->GetString() : g_pServerListManager->m_Server.m_svHostName;
            std::string serverMap = g_pHostState->m_levelName;
            std::string gameType = KeyValues_GetCurrentPlaylist();

            DevMsg(eDLL_T::SERVER, "Server name: %s \n", serverName.c_str());
            DevMsg(eDLL_T::SERVER, "Server map: %s \n", serverMap.c_str());
            DevMsg(eDLL_T::SERVER, "MatchID to ship: %s \n", matchID.c_str());

            std::string logdata_url_encoded = url_encode(logdata);

            std::string postData =
                "ServerNAME=" + serverName +
                "&GameTYPE=" + gameType +
                "&MatchID=" + matchID +
                "&Logdata=" + logdata_url_encoded +
                "&Map=" + serverMap +
                "&CODE=" + API_KEY; //API code is generated for each major api version change

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
        else {

            Error(eDLL_T::SERVER, NO_ERROR, "Failed to initialize curl\n");
            return;
        }

        //curl_global_cleanup();
    }

    // the file writer
    void LOGGER::Logger::writeBufferToFile(const std::deque<std::pair<std::string, std::string>>& buffer) {
        std::ofstream logFile;
        std::string currentFilePath;
        for (const auto& logPair : buffer) {
            const std::string& filePath = logPair.first;
            const std::string& logMessage = logPair.second;

            if (filePath != currentFilePath) {
                if (logFile.is_open()) {
                    logFile.close();
                }
                logFile.open(filePath, std::ios_base::app);
                if (!logFile) {
                   
                    Error(eDLL_T::SERVER, NO_ERROR, "Unable to create or open log file: '%s'. Check permissions and disk space\n", filePath.c_str());
                    return;
                }
                currentFilePath = filePath;
            }

            logFile << logMessage << "\n";
        }
        if (logFile.is_open()) {
            logFile.close(); 
        }
    }

    // queue based log writing
    void LOGGER::Logger::logToFile() {
        try {
            while (true) {
                std::unique_lock<std::mutex> lock(mtx);
                cvLog.wait(lock, [this] { return !logQueue.empty() || finished; });

                std::deque<std::pair<std::string, std::string>> buffer;
                while (!logQueue.empty()) {
                    buffer.push_back(logQueue.front());
                    logQueue.pop();

                    if (buffer.size() == 30) { // buffer max of 30 - for performance
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
            Error(eDLL_T::SERVER, NO_ERROR, "Exception in logToFile thread: %s\n", e.what());
        }
        catch (...) {
            Error(eDLL_T::SERVER, NO_ERROR, "Unknown exception in logToFile thread.\n");
        }
    }

    std::string LOGGER::replace_all(std::string str, const std::string& from, const std::string& to) {
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        return str;
    }

    std::string LOGGER::url_encode(const std::string& value) {
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


    // log controlled start logging
    void LOGGER::Logger::startLogging() {
        finished = false;
        try {
            logThread = std::thread(&Logger::logToFile, this);
        }
        catch (const std::exception& e) {
            Error(eDLL_T::SERVER, NO_ERROR, "Exception when starting logToFile thread: %s\n", e.what());
        }
        catch (...) {
            Error(eDLL_T::SERVER, NO_ERROR, "Unknown exception when starting logToFile thread.\n");
        }
    }

    //Log controlled stop logging

    void LOGGER::Logger::stopLoggingThread() {
        std::unique_lock<std::mutex> lock(mtx);
        if (!finished) {
            finished = true;
            VScriptCode::Shared::setMatchID(0);
            {
                cvLog.notify_all();
            }

            {
                std::lock_guard<std::mutex> cacheLock(cache_mtx);
                sanitizedFilenameCache.clear();
            }
            Warning(eDLL_T::SERVER, "Stopping logging thread...\n\n");
            lock.unlock();
            if (logThread.joinable()) {
                logThread.join();
            }
        }
        else {
            DevMsg(eDLL_T::RTECH, "No active logging thread.\n\n");
        }
    }

    bool Logger::isLogging() {
        return !finished;
    }

    void Logger::handleNewMatch(const std::string& sanitizedFilename) {
        if (isLogging()) {
            stopLoggingThread();
        }

        std::string mID = sanitizedFilename.substr(0, sanitizedFilename.size() - 5);
        startLogging();
        Warning(eDLL_T::SERVER, ":::::::::::::::::::::::::::::::::::::: Logging thread started :::::::\n");
        Warning(eDLL_T::SERVER, ":::::: Match started with MatchID:  [  %s  ] :::::::\n", mID.c_str());
    }

    // SQVM controlled-Stop logging true/false for api stat send
    void LOGGER::Logger::stopLogging(bool sendToAPI) {
        if (!logThread.joinable()) {
            DevMsg(eDLL_T::RTECH, "No active logging thread to stop.\n");
            return;
        }

        Warning(eDLL_T::SERVER, "Logger thread stopping...\n");

        {
            std::lock_guard<std::mutex> cacheLock(cache_mtx);
            sanitizedFilenameCache.clear();
        }

        {
            finished = true;
            VScriptCode::Shared::setMatchID(0);
            {
                std::unique_lock<std::mutex> lock(mtx);
                cvLog.notify_all();
            }
        }

        logThread.join();

        //old cache clear

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
                apiThread = std::thread(&Logger::sendLogToAPI, this, logData);
            }
        }
        else {
            Error(eDLL_T::SERVER, NO_ERROR, "Stats shipping omitted;\n");
        }
    }

    
    // LogEvent called via squirrel scripts
    void LOGGER::Logger::LogEvent(const char* filename, const char* logString, bool checkDir, bool encrypt) {
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
            if (isLogging()) {
                stopLoggingThread();
            }

            try {
                handleNewMatch(sanitizedFilename);
            }
            catch (const std::runtime_error& e) {
                Error(eDLL_T::SERVER, NO_ERROR, "Runtime Error starting logging thread: %s\n", e.what());
                return;
            }
            catch (const std::exception& e) {
                Error(eDLL_T::SERVER, NO_ERROR, "Exception starting logging thread: %s\n", e.what());
                return;
            }
            catch (...) {
                Error(eDLL_T::SERVER, NO_ERROR, "Unknown Exception starting logging thread.\n");
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


            std::string serverName = ::IsDedicated() ? hostname->GetString() : g_pServerListManager->m_Server.m_svHostName;
            std::string serverMap = g_pHostState->m_levelName;
            std::string gameType = KeyValues_GetCurrentPlaylist();
            int64_t matchID = VScriptCode::Shared::getMatchID();
            std::string curMatchID = std::to_string(matchID);
            std::string curVersion = SERVER_V;
            //std::string curMatchID = sanitizedFilename;
            start_lines += "\n|#MatchID:" + curMatchID;
            start_lines += "\n|#Gameversion:" + curVersion;
            start_lines += "\n|#Gametype:" + gameType;
            start_lines += "\n|#ServerName:" + serverName;
            start_lines += "\n|#ServerMAP:" + serverMap;


            std::vector<std::string> startLines = splitString(start_lines, "\n");

            if (encrypt) {
                std::vector<uint8_t> keyHex = this->eObj.hex2bytes("c7abf6c3574e60bb7e8c2945ff21ec53");
                std::vector<uint8_t> ivHex = this->eObj.hex2bytes("ac6fe374229715e475928b70ab53648e");
                for (std::string& line : startLines) {
                    line = this->eObj.doEncrypt(line, keyHex, ivHex);
                }
            }

            std::string processedStartLines;
            for (const std::string& line : startLines) {
                processedStartLines += line + "\n";
            }

            std::lock_guard<std::mutex> lock(mtx);
            logQueue.push(std::make_pair(filePath.string(), processedStartLines));
        }

        std::vector<std::string> lines = splitString(logString, "\n");

        if (encrypt) {
            std::vector<uint8_t> keyHex = this->eObj.hex2bytes("c7abf6c3574e60bb7e8c2945ff21ec53");
            std::vector<uint8_t> ivHex = this->eObj.hex2bytes("ac6fe374229715e475928b70ab53648e");
            for (std::string& line : lines) {
                line = this->eObj.doEncrypt(line, keyHex, ivHex);
            }
        }

        for (const std::string& line : lines) {
            std::lock_guard<std::mutex> lock(mtx);
            logQueue.push(std::make_pair(filePath.string(), line));
        }

        {
            std::unique_lock<std::mutex> lock(mtx);
            cvLog.notify_all();
        }

    }


} // namespace LOGGER