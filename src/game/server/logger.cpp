#pragma once
#include <tiny-aes/aes.hpp>
#include "core/stdafx.h"
#include "engine/server/server.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"
#include "thirdparty/curl/include/curl/curl.h"
#include <engine/host_state.h>
#include <networksystem/listmanager.h>
#include <game/shared/vscript_shared.h>
#include "engine/server/vengineserver_impl.h"
#include "logger.h"
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
#include <shared_mutex>
#include <string>
#include <vector>
#include <deque>
#include <utility>
#include <atomic>
#include "public\ifilesystem.h"
#include <algorithm>
#include <cctype>
#include <condition_variable>
#include <future>


//-----------------------------------------------------------------------------
// POINTERS
//-----------------------------------------------------------------------------
LOGGER::Logger* LOGGER::pMkosLogger = nullptr;

//-----------------------------------------------------------------------------
// CONSTANTS
//-----------------------------------------------------------------------------

const std::string SERVER_V = "rc_2.4.5";
const std::string API_KEY = "Kfvtu2TSNKQ7S2pP"; //public
constexpr const char* R5RDEV_CONFIG = "r5rdev_config.json";
const std::string STATS_API = "https://r5r.dev/api/stats5.php";

//-----------------------------------------------------------------------------
// string manipulation split function
//-----------------------------------------------------------------------------

std::vector<std::string> split(const std::string& str, char delimiter) 
{
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = str.find(delimiter);

    while (end != std::string::npos) 
    {
        result.push_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(delimiter, start);
    }

    result.push_back(str.substr(start));
    return result;
}




//-----------------------------------------------------------------------------
// string manipulation sanitize function
//-----------------------------------------------------------------------------

std::string SanitizeString(const std::string & input) 
{
    //appropriate
    auto isDisallowed = [](unsigned char c) {
        return !std::isalnum(c) && c != '-' && c != '_';
        };

    if (std::any_of(input.begin(), input.end(), isDisallowed)) {
        return "";
    }
    return input;
}




namespace LOGGER 
{   

    std::unordered_map<std::string, std::string> g_configMap;
    std::shared_timed_mutex g_configMapMutex;
    std::atomic<int64_t> last_match_id{ 0 };

    //-----------------------------------------------------------------------------
    // r5rdev_config.json config management
    //-----------------------------------------------------------------------------


    void CleanupLogs(IFileSystem* pFileSystem) 
    {
        if (pFileSystem == nullptr) 
        {
            Error(eDLL_T::SERVER, NO_ERROR, "FileSystem pointer is null\n");
            return;
        }

        const char* subDir = GetSetting("logfolder");
        const char* delete_all = GetSetting("server.DELETE_ALL_LOGS");
        bool DEL_ALL = false;

        if (!delete_all || strcmp(delete_all, "") == 0)
        {
            DEL_ALL = false;
        }
        else
        {
            DEL_ALL = (strcmp(delete_all, "true") == 0);
        }

        if (!subDir) 
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Attempted to load an invalid setting value.\n");
            return;
        }

        if (!pFileSystem->IsDirectory(subDir, "PLATFORM")) 
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Directory does not exist: %s\n", subDir);
            return;
        }

        int64_t sv_maxdir_size_t = GetMaxLogfileSize(GetSetting("server.MAX_LOGFILE_DIR_SIZE"));

        sv_maxdir_size_t = sv_maxdir_size_t < 3 ? 20LL : sv_maxdir_size_t;

        char searchPath[MAX_PATH];
        snprintf(searchPath, sizeof(searchPath), "%s/*.json", subDir);

        struct FileInfo {
            std::string fullPath = "";
            ssize_t fileSize = 0;
            long long fileTime = 20LL;
        };

        std::vector<FileInfo> fileInfos;

        ssize_t dirSize = 0;

        FileFindHandle_t findHandle;

        const char* fn = pFileSystem->FindFirstEx(searchPath, "PLATFORM", &findHandle);

        while (fn != nullptr) 
        {
            if (!pFileSystem->FindIsDirectory(findHandle)) 
            {
                FileInfo info;
                info.fullPath = std::string(subDir) + "/" + fn;
                info.fileSize = pFileSystem->FSize(info.fullPath.c_str(), "PLATFORM");
                info.fileTime = pFileSystem->GetFileTime(info.fullPath.c_str(), "PLATFORM");
                dirSize += info.fileSize;
                fileInfos.push_back(info);
            }
            fn = pFileSystem->FindNext(findHandle);
        }

        pFileSystem->FindClose(findHandle);

        std::sort(fileInfos.begin(), fileInfos.end(), [](const FileInfo& a, const FileInfo& b) {
            return a.fileTime < b.fileTime;
            });

        for (const auto& fileInfo : fileInfos) 
        {
            if ( !DEL_ALL && dirSize <= sv_maxdir_size_t * 1024 * 1024)
            {
                break;
            }

            pFileSystem->RemoveFile(fileInfo.fullPath.c_str(), "PLATFORM");
            dirSize -= fileInfo.fileSize;
            DevMsg(eDLL_T::SERVER, "Removing log: %s\n", fileInfo.fullPath.c_str());
        }

        DevMsg(eDLL_T::SERVER, "Final statlog directory size: %zd bytes\n", dirSize);
    }




    void AddToConfigMap(const rapidjson::Value& value, const std::string& parentKey = "", int depth = 0) 
    {
        if (depth > 30) 
        { 
            Error(eDLL_T::SERVER, NO_ERROR, "Object depth exceeded allocated recursion limit.. \n");
            return;
        }

        for (rapidjson::Value::ConstMemberIterator itr = value.MemberBegin(); itr != value.MemberEnd(); ++itr) 
        {
            std::string key = parentKey.empty() ? itr->name.GetString() : parentKey + "." + itr->name.GetString();

            if (itr->value.IsObject()) 
            {
                AddToConfigMap(itr->value, key, depth + 1);
            }
            else 
            {
                std::string valueStr;
                if (itr->value.IsString()) 
                {
                    valueStr = itr->value.GetString();
                }
                else if (itr->value.IsInt()) 
                {
                    valueStr = std::to_string(itr->value.GetInt());
                }
                else if (itr->value.IsBool()) 
                {
                    valueStr = itr->value.GetBool() ? "true" : "false";
                }
                else if (itr->value.IsDouble()) 
                {
                    valueStr = std::to_string(itr->value.GetDouble());
                }

                if (!valueStr.empty()) 
                {
                    std::lock_guard<std::shared_timed_mutex> lock(g_configMapMutex);
                    g_configMap[key] = valueStr;
                }
            }
        }
    }




    void LoadConfig(IFileSystem* pFileSystem, const char* configFileName) 
    {   
        if (!configFileName)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Error in [LoadConfig] : configFileName was nullptr \n");
        }


        if (!pFileSystem->FileExists(configFileName))
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Missing stats file: %s\n", configFileName);
            return;
        }

        FileHandle_t configFile = pFileSystem->Open(configFileName, "rt");
        if (!configFile)
        {
            DevMsg(eDLL_T::SERVER, "Failed to open config file: %s\n", configFileName);
            return;
        }

        ssize_t fileLength = pFileSystem->Size(configFile);
        std::unique_ptr<char[]> buffer(new char[fileLength + 1]);
        ssize_t bytesRead = pFileSystem->Read(buffer.get(), fileLength, configFile);
        pFileSystem->Close(configFile);

        buffer[bytesRead] = '\0'; 

        rapidjson::Document document;
        if (document.Parse(buffer.get()).HasParseError())
        {
            DevMsg(eDLL_T::SERVER, "JSON parse error: %s\n", rapidjson::GetParseError_En(document.GetParseError()));
            return;
        }

        if (document.IsObject()) 
        {
            AddToConfigMap(document);
            DevMsg(eDLL_T::SERVER, "Loaded R5R.DEV config file: %s \n", configFileName);
        }
        else
        {
            DevMsg(eDLL_T::SERVER, "Failed to load stats config file: File was not a valid json object. \n");
        }
    }



    // get setting value by key
    const char* GetSetting(const char*key)
    {
        if (!key)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Error in [GetSetting] : key was nullptr \n");
            return "";
        }

        std::shared_lock<std::shared_timed_mutex> lock(g_configMapMutex);
        std::unordered_map<std::string, std::string>::const_iterator itr = g_configMap.find(key);

        if (itr != g_configMap.end()) 
        {
            return itr->second.c_str();
        }

        return ""; //is this okay?
    }




    void ReloadConfig(const char* configFileName)
    {
        {
            //probably unsafe asf
            std::lock_guard<std::shared_timed_mutex> lock(g_configMapMutex);
            g_configMap.clear();
        }

        LoadConfig(FileSystem(), configFileName);
    }


    //-----------------------------------------------------------------------------
    // class CURLConnectionPool
    //-----------------------------------------------------------------------------

    CURLConnectionPool::CURLConnectionPool() 
    { 
        ResetPool(); 

        for (int i = 0; i < 5; ++i) 
        {
            CURL* handle = curl_easy_init();
            if (handle) 
            {
                curl_easy_setopt(handle, CURLOPT_TCP_NODELAY, 1L);
                curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);
                pool.push(handle);
            }
            else 
            {
                Error(eDLL_T::SERVER, NO_ERROR, "Failed to initialize CURL handle. \n");
            }
        }
    }

    CURLConnectionPool& CURLConnectionPool::GetInstance() 
    {
        static CURLConnectionPool instance;
        return instance;
    }

    CURL* CURLConnectionPool::GetHandle() 
    {
        std::unique_lock<std::mutex> lock(poolMutex);

        while (pool.empty()) 
        {
            if (!poolCond.wait_for(lock, handleWaitTimeout, [this] { return !pool.empty(); })) 
            {
                if (pool.size() < maxPoolSize) 
                {
                    return CreateHandle();
                }
                else 
                {
                    Error(eDLL_T::SERVER, NO_ERROR, "Timeout waiting for CURL handle and pool is full. \n");
                    return nullptr;
                }
            }
        }

        CURL* handle = pool.front();
        pool.pop();
        //DevMsg(eDLL_T::SERVER, ":: Handle returned \n");
        return handle;
    }

    CURL* CURLConnectionPool::CreateHandle() 
    {
        CURL* handle = curl_easy_init();

        if (handle) 
        {
            curl_easy_setopt(handle, CURLOPT_TCP_NODELAY, 1L);
            curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);
        }
        else 
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Failed to initialize CURL handle. \n");
        }

        return handle;
    }

    bool CURLConnectionPool::HandleCurlResult(CURL* handle, CURLcode res, const char* func)
    {    
        if (res == CURLE_OK) 
        {
            return true;
        }
        else 
        {
            Error(eDLL_T::SERVER, NO_ERROR, "CURL failed in [%s] : %s\n", func, curl_easy_strerror(res));

            switch (res)
            {
                case CURLE_COULDNT_CONNECT:
                case CURLE_COULDNT_RESOLVE_HOST:
                case CURLE_COULDNT_RESOLVE_PROXY:

                    ReturnHandle(handle);
                    return true;
                    break;

                case CURLE_SSL_CONNECT_ERROR:
                case CURLE_SSL_CIPHER:
                case CURLE_SSL_CACERT:

                    DiscardHandle(handle);
                    return false;

                default:
                    DiscardHandle(handle);
            }
        }

        return false;
    }

    void CURLConnectionPool::ReturnHandle(CURL* handle) 
    {
        std::lock_guard<std::mutex> lock(poolMutex);
        if (handle && pool.size() < maxPoolSize) 
        {
            curl_easy_reset(handle);
            pool.push(handle);
            poolCond.notify_one();
        }
        else 
        {
            if (handle) 
            {
                DevMsg(eDLL_T::SERVER, ":: Handle discarded as pool is full \n");
                curl_easy_cleanup(handle);
            }
            else 
            {
                handle = nullptr;
                Error(eDLL_T::SERVER, NO_ERROR, "Attempted to return a null CURL handle to the pool. \n");
            }
        }
    }

    void CURLConnectionPool::DiscardHandle(CURL* handle)
    {
        std::lock_guard<std::mutex> lock(poolMutex);
        if (handle) 
        {
            curl_easy_cleanup(handle);
        }
    }

    void CURLConnectionPool::ResetPool() 
    {
        std::lock_guard<std::mutex> lock(poolMutex);
        while (!pool.empty()) 
        {
            curl_easy_cleanup(pool.front());
            pool.pop();
        }
    }




    CURLConnectionPool::~CURLConnectionPool() 
    {
        ResetPool();
    }
    //END CLASS CURLConnectionPool




    //UTILITY
    int64_t GetMaxLogfileSize(const char* settingValue) 
    {
        if (!settingValue) 
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Max log file size setting is empty.\n");
            return -1;
        }

        char* endPtr;
        int64_t maxSize64_t = std::strtoll(settingValue, &endPtr, 10);

        if (*endPtr != '\0' || maxSize64_t <= 0)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Invalid value for max log file size setting: %s\n", settingValue);
            return -1;
        }

        return maxSize64_t;
    }




    std::string LOGGER::GetEndingMatchID()
    {    
        return std::to_string(last_match_id.load());
    }




    void LOGGER::SaveEndingMatchID()
    {
        last_match_id.store(VScriptCode::Shared::getMatchID());
    }




    std::string LOGGER::replace_all(std::string str, const std::string& from, const std::string& to) 
    {
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        return str;
    }




    std::string LOGGER::url_encode(const std::string& value)
    {
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




    size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) 
    {
        size_t totalSize = size * nmemb;
        userp->append((char*)contents, totalSize);
        return totalSize;
    }




    //////////////////
    /// TASKMANAGER /////////////////////////////////////////////////////////////
    //////////////////


    std::atomic<bool> stop_tasks_flag{false};
    std::queue<std::function<void()>> taskQueue;


    TaskManager& TaskManager::getInstance() 
    {
        static TaskManager instance;
        return instance;
    }




    TaskManager::TaskManager() 
    {
        StartWorkerThread();
    }




    TaskManager::~TaskManager() 
    {
        StopWorkerThread();
    }




    void TaskManager::StopWorkerThread() 
    {
        stop_tasks_flag = true;
        condVar.notify_one();

        if (workerThread.joinable()) 
        {
            workerThread.join();
        }
    }




    void TaskManager::StartWorkerThread() 
    {
        workerThread = std::thread(&TaskManager::ProcessTasks, this);
    }




    void TaskManager::AddTask(const std::function<void()>& task) 
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.push(task);
        condVar.notify_one();
    }




    //keeps map lean by removing map slot for player after assignment in squirrel
    void LOGGER::TaskManager::ResetPlayerStats(const char* player_oid) 
    {
        if (player_oid == nullptr || std::strlen(player_oid) == 0) 
        {
            Error(eDLL_T::SERVER, NO_ERROR, "ResetPlayerStats called with empty or null player_oid. \n");
            return;
        }

        std::string playerOidStr(player_oid);

        std::function<void()> task = [playerOidStr]() 
            {
                std::unique_lock<std::shared_timed_mutex> lock(statsMutex, std::defer_lock);
                if (lock.try_lock_for(std::chrono::milliseconds(10000))) 
                {
                    std::unordered_map<std::string, std::string>::iterator it = playerStatsMap.find(playerOidStr);
                    if (it != playerStatsMap.end()) 
                    {
                        playerStatsMap.erase(it);
                    }
                }
                else 
                {
                    Error(eDLL_T::SERVER, NO_ERROR, "Could not aquire lock to remove player slot in map. \n");
                }
            };

        TaskManager::getInstance().AddTask(task);
    }




    void TaskManager::ProcessTasks() 
    {
       
        while (true) 
        {
            std::function<void()> task;
            bool execute = false;

            { 
                std::unique_lock<std::mutex> lock(queueMutex);
                condVar.wait(lock, [this] { return stop_tasks_flag || !taskQueue.empty(); });

                if (stop_tasks_flag && taskQueue.empty())
                {   
                    lock.unlock();
                    break; //compiler lies
                }
   
                task = taskQueue.front();
                taskQueue.pop();
                execute = true;

            } 

            if (execute) 
            {
                try {
                    task();
                }
                catch (const std::exception& e) {
             
                    Error(eDLL_T::SERVER, NO_ERROR, "Exception in ProcessTasks: %s \n", e.what());
                }
                catch (...) {

                    Error(eDLL_T::SERVER, NO_ERROR, "Unknown exception in ProcessTasks \n");
                }
            }
        }   
    }




   //////////////////////////////
   /// STAT FUNCTIONS FOR SQVM ///////////////////////////////////////////////////////////
   //////////////////////////////

    //input: oid, output: player stats if available in the playerstatsmap comma separated string
    const char* GetKDString(const char* player_oid)
    {
        if (!player_oid)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "[GetKDString] Error: player_oid nullptr \n");
            return "NA";
        }

        std::shared_lock<std::shared_timed_mutex> lock(statsMutex, std::defer_lock);

        if (lock.try_lock_for(std::chrono::milliseconds(500)))
        {
            std::unordered_map<std::string, std::string>::iterator it = playerStatsMap.find(std::string(player_oid));

            if (it != playerStatsMap.end())
            {
                return it->second.c_str();
            }
        }
        else
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Unable to acquire lock while reading stats for player: %s \n", player_oid);
        }

        return "";
    }




 /*********************************
 /     API PLAYER STATS
 /********************************/

    //called by TaskManager::LoadBatchKDStrings
    std::string FetchBatchPlayerStats(const std::vector<std::string>& player_oids)
    {
        if (player_oids.empty())
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Error: player_oid empty \n");
            return "NA";
        }

        CURL* easy_handle = CURLConnectionPool::GetInstance().GetHandle();

        if (!easy_handle)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Failed to acquire curl handle from pool\n");
            return "NA";
        }

        std::string readBuffer;
        std::string identifier = SanitizeString(GetSetting("identifier"));
        std::string url = STATS_API + "?TYPE=batch&KEY=" + API_KEY + "&identifier=" + identifier;

        for (const std::string& oid : player_oids)
        {
            url += "&player_oid[]=" + oid;
        }

        curl_easy_setopt(easy_handle, CURLOPT_URL, url.c_str());
        curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(easy_handle, CURLOPT_TIMEOUT, 4L);

        CURLcode res;
        int retry = 0;
        const char* info;
        const int max_attempts = 3;

        do {
            res = curl_easy_perform(easy_handle);

            if (res == CURLE_OK)
            {
                break;
            }

            info = (retry >= max_attempts) ? "Connection failed. Stats not loaded." : "- Retrying connection...";
            Error(eDLL_T::SERVER, NO_ERROR, "Batch stats-fetch: curl_easy_perform() failed: %s %s\n", curl_easy_strerror(res), info);
            retry++;

        } while (retry < max_attempts);


        bool check = CURLConnectionPool::GetInstance().HandleCurlResult(easy_handle,res,"FetchBatchPlayerStats");

        return check == true ? readBuffer : "NA";
    }



    //needs a rewrite
    //input: string of comma separated oids; fetches from api and sets playerstatsmap foreach as oid->kills,deaths,superglides
    void TaskManager::LoadBatchKDStrings(const std::string& player_oids_str)
    {
        AddTask([player_oids_str, this]() 
        {
            if (player_oids_str.empty())
            {
                Error(eDLL_T::SERVER, NO_ERROR, "Error in [LoadBatchKDStrings] : player_oids_str was empty \n");
                return;
            }

            std::vector<std::string> player_oids = split(player_oids_str, ',');

            std::string stats_json = FetchBatchPlayerStats(player_oids);

            rapidjson::Document document;
            if (document.Parse(stats_json.c_str()).HasParseError()) 
            {
                Error(eDLL_T::SERVER, NO_ERROR, "JSON parsing failed: %s\n",
                    rapidjson::GetParseError_En(document.GetParseError()));
                return;
            }

            if (!document.IsObject()) 
            {
                Error(eDLL_T::SERVER, NO_ERROR, "JSON root is not an object\n");
                return;
            }

            for (rapidjson::Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr) 
            {
                std::string player_oid = itr->name.GetString();

                if (!itr->value.IsObject()) continue;

                const rapidjson::Value& player_stats = itr->value;
                std::string stats_str;
                if (player_stats.HasMember("kills") && player_stats["kills"].IsInt() &&
                    player_stats.HasMember("deaths") && player_stats["deaths"].IsInt() &&
                    player_stats.HasMember("superglides") && player_stats["superglides"].IsInt() &&
                    player_stats.HasMember("playtime") && player_stats["playtime"].IsInt() &&
                    player_stats.HasMember("total_matches") && player_stats["total_matches"].IsInt() &&
                    player_stats.HasMember("score") && player_stats["total_matches"].IsInt()
                    ) 
                {
                    stats_str = std::to_string(player_stats["kills"].GetInt()) + "," +
                    std::to_string(player_stats["deaths"].GetInt()) + "," +
                    std::to_string(player_stats["superglides"].GetInt()) + "," + 
                    std::to_string(player_stats["playtime"].GetInt()) + "," +
                    std::to_string(player_stats["total_matches"].GetInt()) + "," +
                    std::to_string(player_stats["score"].GetInt());
                }
                else
                {
                    Error(eDLL_T::SERVER, NO_ERROR, "LoadBatchKDStrings contained invaliid object members for: %s\n", player_oid.c_str());
                    stats_str = "0,0,0,0,0,0";
                }

                if (player_stats.HasMember("settings") && player_stats["settings"].IsObject()) 
                {
                    const rapidjson::Value& settings = player_stats["settings"];
                    for (rapidjson::Value::ConstMemberIterator m = settings.MemberBegin(); m != settings.MemberEnd(); ++m) 
                    {
                        // appended as key:value
                        stats_str += "," + std::string(m->name.GetString()) + ":" + std::string(m->value.GetString());
                    }
                }

                bool has_lock = false;
                std::unique_lock<std::shared_timed_mutex> lock(statsMutex, std::defer_lock);
                if (lock.try_lock_for(std::chrono::milliseconds(3000))) 
                {
                    playerStatsMap[player_oid] = stats_str;
                    has_lock = true;
                }

                if (!has_lock) 
                {
                    Error(eDLL_T::SERVER, NO_ERROR, "failed to aquire lock to write player stats into map\n");
                }

            }
        });
    }




    //called by TaskManager::LoadKDString
    std::string FetchPlayerStats(const char* player_oid)
    {
        if (!player_oid)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Error: player_oid nullptr \n");
            return "NA";
        }

        CURL* easy_handle = CURLConnectionPool::GetInstance().GetHandle();

        if (!easy_handle) 
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Failed to acquire curl handle from pool\n");
            return "NA";
        }

        std::string readBuffer;
        std::string identifier = SanitizeString(GetSetting("identifier"));
        std::string url = STATS_API + "?KEY=" + API_KEY + "&player_oid=" + player_oid + "&identifier=" + identifier;

        curl_easy_setopt(easy_handle, CURLOPT_URL, url.c_str());
        curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(easy_handle, CURLOPT_TIMEOUT, 3L);

        CURLcode res = curl_easy_perform(easy_handle);

        if (res != CURLE_OK) {
            Error(eDLL_T::SERVER, NO_ERROR, "Stat lookup: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        bool check = CURLConnectionPool::GetInstance().HandleCurlResult(easy_handle, res, "FetchPlayerStats");

        return check == true ? readBuffer : "NA";
    }




    //for individual players, sets player stats in playerstatsmap as i,i,i: kills,deaths,superglides
    void TaskManager::LoadKDString(const char* player_oid)
    {
        if (!player_oid)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Error in [LoadKDString] : player_oid was nullptr\n");
        }

        std::string playerOidStr(player_oid);

        AddTask([playerOidStr, this]() 
        {

            std::string stats = FetchPlayerStats(playerOidStr.c_str());
            
            bool has_lock = false;
            std::unique_lock<std::shared_timed_mutex> lock(statsMutex, std::defer_lock);
            if (lock.try_lock_for(std::chrono::milliseconds(3000))) 
            {
                playerStatsMap[playerOidStr] = stats;
                has_lock = true;
            }

            if (!has_lock) 
            {
                Error(eDLL_T::SERVER, NO_ERROR, "failed to aquire lock to write player stats into map for: %s\n", playerOidStr.c_str());
            }

        });
    }




    /*********************************
    /       API LIVE STATS
    /********************************/

    //separate thread
    void RunUpdateLiveStats(std::string stats_json_copy ) 
    {   
        const char* stats_json = stats_json_copy.c_str();

        if (!stats_json)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "[RunUpdateLiveStats] Failed: Nullptr\n");
            return;
        }

        CURL* curl = CURLConnectionPool::GetInstance().GetHandle();

        if (!curl)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Failed to acquire curl handle from pool\n");
            return;
        }

        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        rapidjson::Document stats;
        stats.Parse(stats_json);
        if (stats.HasParseError()) 
        {   
            size_t offset = stats.GetErrorOffset();
            Error(eDLL_T::SERVER, NO_ERROR, "[RunUpdateLiveStats] Failed to parse stats JSON at offset %zu: %s\n", offset, rapidjson::GetParseError_En(stats.GetParseError()));
            return;
        }

        std::string uniquekey = SanitizeString(GetSetting("uniquekey"));
        std::string identifier = SanitizeString(GetSetting("identifier"));
        std::string serverName = ::IsDedicated() ? hostname->GetString() : g_pServerListManager->m_Server.m_svHostName;

        doc.AddMember("stats", stats, allocator);
        doc.AddMember("servername", rapidjson::Value(serverName.c_str(), allocator), allocator);
        doc.AddMember("KEY", rapidjson::Value(API_KEY.c_str(), allocator), allocator);
        doc.AddMember("uniquekey", rapidjson::Value(uniquekey.c_str(), allocator), allocator);
        doc.AddMember("identifier", rapidjson::Value(identifier.c_str(), allocator), allocator);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        std::string jsonData = buffer.GetString();

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, STATS_API.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

        CURLcode res = curl_easy_perform(curl);

        curl_slist_free_all(headers);

        CURLConnectionPool::GetInstance().HandleCurlResult(curl, res, "RunUpdateLiveStats");

    }




    //used to keep livestats up to date for matchmaking
    void UpdateLiveStats(std::string stats_json)
    {   
        if (stats_json.empty())
        {
            Error(eDLL_T::SERVER, NO_ERROR, "[UpdateLiveStats] failed: stats_json empty\n");
            return;
        }
        
        std::thread updateLiveStatsThread([stats_json]() {
            RunUpdateLiveStats(stats_json);
        });

        updateLiveStatsThread.detach();
    }



    /*********************************
    /     API PLAYER COUNT UPDATES
    /********************************/

    // called by UPDATE_PLAYER_COUNT as separate thread
    void PlayerCountUpdate(const char* action, const char* player, const char* oid, const char* count, const char* DISCORD_HOOK)
    {

        if (!action || !player || !oid || !count || !DISCORD_HOOK)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Error in [PlayerCountUpdate]: nullptr \n");
            return;
        }

        CURL* curl = CURLConnectionPool::GetInstance().GetHandle();

        if (!curl)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Failed to acquire curl handle from pool\n");
            return;
        }
        

        std::string postData = "servername=";
        postData += ::IsDedicated() ? hostname->GetString() : g_pServerListManager->m_Server.m_svHostName;
        postData += "&action=";
        postData += action;
        postData += "&player_name=";
        postData += player;
        postData += "&OID=";
        postData += oid;
        postData += "&current_count=";
        postData += count;
        postData += "&DISCORD_HOOK=";
        postData += DISCORD_HOOK;
        postData += "&KEY=";
        postData += API_KEY;

        curl_easy_setopt(curl, CURLOPT_URL, "https://r5r.dev/api/playercount.php");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        CURLcode res = curl_easy_perform(curl);

        CURLConnectionPool::GetInstance().HandleCurlResult(curl, res, "PlayerCountUpdate");
    }

     //needs re worked - dont allow setting discord hooks in playlist file
    // Sends join/leave data to api for various use. replace DISCORD_HOOK with internal r5r.dev functions via string starting with __apicall_ 
    void UPDATE_PLAYER_COUNT(const char* action, const char* player, const char* OID, const char* count, const char* DISCORD_HOOK)
    {

        if ( !action || !player || !OID || !count || !DISCORD_HOOK )
        {
            return;
        }

        if ( strcmp(DISCORD_HOOK, "") == 0)
        {
            DISCORD_HOOK = GetSetting("webhooks.PLAYERS_WEBHOOK");
        }

        std::thread updateThread(PlayerCountUpdate, action, player, OID, count, DISCORD_HOOK);
        updateThread.detach();

    }




    /*********************************
   /       API END GAME UPDATES
   /********************************/

    //by ref
    void EndMatchUpdate(std::string recap, std::string discord_hook)
    {

        if (recap.empty())
        {
            Error(eDLL_T::SERVER, NO_ERROR, "[EndMatchUpdate] Recap was empty...\n");
            return;
        }

        const std::string matchID = GetEndingMatchID();

        if (matchID.empty())
        {
            Error(eDLL_T::SERVER, NO_ERROR, "[EndMatchUpdate] matchID was empty...\n");
        }

        if (discord_hook.empty())
        {
            discord_hook = std::string(GetSetting("webhooks.MATCHES_WEBHOOK"));
        } 

        LOGGER::Logger& logger = LOGGER::Logger::getInstance();

        const std::string dir_setting = GetSetting("logfolder");
        const std::string filename = logger.GetLatestFile("platform/" + dir_setting, GetEndingMatchID());

        if (filename.empty())
        {
            Error(eDLL_T::SERVER, NO_ERROR, "No log found. Aborting API connection [EndMatchUpdate]...\n");
            return;
        }

        const std::string serverName = ::IsDedicated() ? hostname->GetString() : g_pServerListManager->m_Server.m_svHostName;

        size_t recap_len = recap.length();
        if (recap_len > static_cast<size_t>(INT_MAX))
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Failed to cast recap string size to int, result would be truncated. \n");
            return;
        }

        CURL* curl = CURLConnectionPool::GetInstance().GetHandle();

        if (!curl)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Failed to acquire curl handle from pool\n");
            return;
        }

        //the length should never exceed int.
        char* escaped_recap = curl_easy_escape(curl, recap.c_str(), static_cast<int>(recap_len));

        if (!escaped_recap)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Failed to encode recap string\n");
            CURLConnectionPool::GetInstance().ReturnHandle(curl);
            return;
        }

        std::string postData = "servername=" + serverName +
            "&matchID=" + matchID +
            "&recap=" + std::string(escaped_recap) +
            "&DISCORD_HOOK=" + discord_hook +
            "&KEY=" + API_KEY;

        curl_easy_setopt(curl, CURLOPT_URL, "https://r5r.dev/api/endmatch.php");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        CURLcode res = curl_easy_perform(curl);

        curl_free(escaped_recap); //must free

        CURLConnectionPool::GetInstance().HandleCurlResult(curl, res, "EndMatchUpdate");
    }




    //sends recap data to discord channel
    void NOTIFY_END_OF_MATCH(const char* recap, const char* DISCORD_HOOK)
    {

        if ( !recap || !DISCORD_HOOK )
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Error in [NOTIFY_END_OF_MATCH] : recap or DISCORD_HOOK was nullptr");
            return;
        }

        //making a copy of recap/discord which can potentially be modified in sqvm
        std::string recap_string(recap);
        std::string discord_hook(DISCORD_HOOK);

        std::thread endMatchThread([recap_string, discord_hook]() {
            EndMatchUpdate(recap_string, discord_hook);
            });
        endMatchThread.detach();

    }




    /*********************************
    /     API VERIFY EA ACCOUNT
    /********************************/

    // returns api response status for website ea account verification from r5r.dev
    const std::string VERIFY_EA_ACCOUNT(const std::string& token, const std::string& OID, const std::string& ea_name)
    {
        if (token.empty() || ea_name.empty())
        {
            return "0";
        }

        CURL* curl = CURLConnectionPool::GetInstance().GetHandle();

        if (!curl)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Failed to acquire curl handle from pool\n");
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

        CURLConnectionPool::GetInstance().ReturnHandle(curl);

        bool check = CURLConnectionPool::GetInstance().HandleCurlResult(curl, res, "VERIFY_EA_ACCOUNT");

        Warning(eDLL_T::SERVER, "Response for %s: %s\n", ea_name.c_str(), readBuffer.c_str());

        return check == true ? readBuffer : "8";
    
    }




    /*********************************
   /     API FETCH GLOBAL SETTINGS
   /********************************/

   // returns response settings based on query
    std::string FetchGlobalSettings(const char* query)
    {

        if (!query)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Error: query parameter pointed to a nullptr\n");
            return "";
        }

        CURL* curl = CURLConnectionPool::GetInstance().GetHandle();

        if (!curl)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Failed to acquire curl handle from pool\n");
            return "";
        }

  
        std::string identifier = SanitizeString(GetSetting("identifier"));
        std::string postfields = "KEY=" + API_KEY + "&query=" + query + "&identifier=" + identifier;

        std::string readBuffer;
        curl_easy_setopt(curl, CURLOPT_URL, "https://r5r.dev/api/globalsettings.php");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        CURLcode res = curl_easy_perform(curl);

        bool check = CURLConnectionPool::GetInstance().HandleCurlResult(curl, res, "FetchGlobalStats");

        return check == true ? readBuffer : "";
    }
        

   

   //////////////////////////////
   /// ENCRYPTION OBJECT ///////////////////////////////////////////////////////////
   //////////////////////////////


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

            AES_init_ctx_iv(&ctx, keyBytes.data(), ivBytes.data());
            if (padded_length > UINT32_MAX) {
                throw std::runtime_error("padded_length is too large to be safely cast to uint32_t");
            }
            
            AES_CBC_encrypt_buffer(&ctx, plainTextBytes.data(), static_cast<uint32_t>(padded_length));

            // convert to base64
            std::string cipherTextBase64 = bbase64Encode(plainTextBytes);

            return cipherTextBase64;
        }




     //////////////////////////////
     /// LOGGER OBJECT ///////////////////////////////////////////////////////////
     //////////////////////////////


    /*********************************
    /     Logger instance
    /********************************/

    // constructor
    Logger::Logger() : filePath("")
    {   
        if (FileSystem() != nullptr)
        {
            LoadConfig(FileSystem(), R5RDEV_CONFIG);
        }
        else
        {
            Error(eDLL_T::SERVER, NOERROR, "INIT balls");
        }
        
        keyHex = this->eObj.hex2bytes("c7abf6c3574e60bb7e8c2945ff21ec53");
        ivHex = this->eObj.hex2bytes("ac6fe374229715e475928b70ab53648e");
        setLogState(LogState::Busy, false);
        setLogState(LogState::Ready, true);

        std::string sleeptime = GetSetting("settings.CVAR_LTHREAD_DEBOUNCE"); //disabled
        std::string max_buffer = GetSetting("settings.CVAR_MAX_BUFFER");

        //put into init wrapper to declutter constructor
        if (!max_buffer.empty())
        {
            try
            {
                CVAR_MAX_BUFFER = std::stoi(max_buffer);
            }
            catch (...)
            {
                Error(eDLL_T::SERVER, NO_ERROR, "Invalid data for setting CVAR_MAX_BUFFER\n");
            }
        }
        
        if (!sleeptime.empty())
        {
            try
            {
                CVAR_LTHREAD_DEBOUNCE = std::stoi(sleeptime);
            }
            catch (...)
            {
                CVAR_LTHREAD_DEBOUNCE = 200;
            }
        }
        else
        {
            CVAR_LTHREAD_DEBOUNCE = 200;
        }
    }

    // destructor 
    Logger::~Logger() 
    {
        stopLoggingThread();
    }

    // one instance only
    Logger& Logger::getInstance() 
    {
        static Logger instance;
        pMkosLogger = &instance;
        return instance;
    }




    /*********************************
    /     Utility Functions
    /********************************/

    void Logger::setLogState(LogState flag, bool value)
    {
        std::atomic<u_char> flags = StateBits.load();

        if (value)
        {
            flags |= static_cast<uint8_t>(flag);
        }
        else
        {
            flags &= ~static_cast<uint8_t>(flag);
        }

        StateBits.store(flags);
    }




    bool Logger::getLogState(LogState flag) const
    {
        return (StateBits.load() & static_cast<uint8_t>(flag)) != 0;
    }




    Logger::LogState Logger::intToLogState(int flag)
    {
        switch (flag)
        {
            case 1: return Logger::LogState::Ready;
            case 2: return Logger::LogState::Busy;
            case 3: return Logger::LogState::Safe;
            default: return Logger::LogState::None;
        }
    }



    //TODO: add state check 
    void LOGGER::Logger::startLogging()
    {
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




    std::string LOGGER::Logger::GetLatestFile(const std::string& directoryPath,std::string matchID = "") 
    {   
        if (directoryPath.empty())
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Error [INTERNAL] :[GetLatestFile] : empty.\n");
            return "";
        }

        IFileSystem* pFileSystem = FileSystem();

        if (!matchID.empty())
        {
            std::string Path = directoryPath + "/" + matchID + ".json";
            if (pFileSystem->FileExists(Path.c_str(), "GAME"))
            {
                return Path;
            }
            else
            {
                Error(eDLL_T::SERVER, NO_ERROR, "Specified matchID file %s does not exist.\n", Path.c_str());
                return "";
            }
        }
        
        Error(eDLL_T::SERVER, NO_ERROR, "MatchID was empty.\n");
        return "";
    }




    // function to split a string
    std::vector<std::string> LOGGER::Logger::splitString(std::string str, const std::string& delimiter) 
    {
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



    
    //atomic
    bool Logger::isLogging()
    {
        return !finished.load();
    }



    /*********************************
    /     Tracker & LogEvent Thread
    /********************************/

    // ship to stats server
    void LOGGER::Logger::sendLogToAPI()
    {   
        std::string matchID = GetEndingMatchID();

        if (matchID.empty() || matchID == "0")
        {
            Error(eDLL_T::SERVER, NO_ERROR, "No log found. Aborting API connection...\n");
            return;
        }

        std::string dir_setting = GetSetting("logfolder");
        std::string logFilePath = GetLatestFile("platform/" + dir_setting, matchID);

        if (logFilePath.empty())
        {
            Error(eDLL_T::SERVER, NO_ERROR, "No log found. Aborting API connection...\n");
            return;
        }

        std::string logdata;
        {
            std::lock_guard<std::mutex> flock(file_mtx);
            std::ifstream lastlog(logFilePath);
            if (lastlog.is_open())
            {
                std::stringstream ss;
                ss << lastlog.rdbuf();
                logdata = ss.str();
                lastlog.close();
                Warning(eDLL_T::SERVER, "Attempting to send...\n");
            }
            else
            {
                Error(eDLL_T::SERVER, NO_ERROR, "!!! Couldn't open logfile for shipping.\n");
                return;
            }
        }
        
        CURL* curl = CURLConnectionPool::GetInstance().GetHandle();

        if (!curl) 
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Failed to acquire curl handle from pool\n");
            return;
        }

        DevMsg(eDLL_T::SERVER, "Curl initialized...\n");
        curl_easy_setopt(curl, CURLOPT_URL, "https://r5r.dev/api/tracker.php");
    
        std::string serverName = ::IsDedicated() ? hostname->GetString() : g_pServerListManager->m_Server.m_svHostName;
        std::string serverMap = g_pHostState->m_levelName;
        std::string gameType = KeyValues_GetCurrentPlaylist();
        std::string identifier = GetSetting("identifier");
        std::string uniquekey = GetSetting("apikey");
        
        if (SanitizeString(identifier).empty())
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Invalid characters in identifier. \n");
        }

        if (SanitizeString(uniquekey).empty())
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Invalid characters in uniquekey. \n");
        }

        DevMsg(eDLL_T::SERVER, "Server name: %s \n", serverName.c_str());
        DevMsg(eDLL_T::SERVER, "Server map: %s \n", serverMap.c_str());
        DevMsg(eDLL_T::SERVER, "MatchID to ship: %s \n", matchID.c_str());

        std::string logdata_url_encoded = url_encode(logdata);

        std::string boundary = "----10KEWLBEANS0101017MA4YWxkTrZu0gW132";
        std::ostringstream body;


        body << "--" << boundary << "\r\n";
        body << "Content-Disposition: form-data; name=\"ServerNAME\"\r\n\r\n";
        body << serverName << "\r\n";

        body << "--" << boundary << "\r\n";
        body << "Content-Disposition: form-data; name=\"GameTYPE\"\r\n\r\n";
        body << gameType << "\r\n";

        body << "--" << boundary << "\r\n";
        body << "Content-Disposition: form-data; name=\"MatchID\"\r\n\r\n";
        body << matchID << "\r\n";

        body << "--" << boundary << "\r\n";
        body << "Content-Disposition: form-data; name=\"Map\"\r\n\r\n";
        body << serverMap << "\r\n";

        body << "--" << boundary << "\r\n";
        body << "Content-Disposition: form-data; name=\"CODE\"\r\n\r\n";
        body << API_KEY << "\r\n";

        body << "--" << boundary << "\r\n";
        body << "Content-Disposition: form-data; name=\"identifier\"\r\n\r\n";
        body << identifier << "\r\n";

        body << "--" << boundary << "\r\n";
        body << "Content-Disposition: form-data; name=\"uniquekey\"\r\n\r\n";
        body << uniquekey << "\r\n";

        body << "--" << boundary << "\r\n";
        body << "Content-Disposition: form-data; name=\"Logdata\"; filename=\"logdata.json\"\r\n";
        body << "Content-Type: application/json\r\n\r\n";
        body << logdata_url_encoded << "\r\n";

        body << "--" << boundary << "--\r\n";


        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Content-Type: multipart/form-data; boundary=" + boundary).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set POST fields
        std::string postBody = body.str();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postBody.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postBody.length());

        std::string readBuffer;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK)
            Error(eDLL_T::SERVER, NO_ERROR, "curl failed: %s\n", curl_easy_strerror(res));
        else
            Warning(eDLL_T::SERVER, "Response: %s\n", readBuffer.c_str());

        curl_slist_free_all(headers);

        CURLConnectionPool::GetInstance().HandleCurlResult(curl, res, "sendLogToAPI");

        if (std::strcmp(GetSetting("server.AUTO_DELETE_STATLOGS"), "true") == 0)
        {
            LOGGER::CleanupLogs(FileSystem());
        }

        CallClosure();
    }




    //-----------------------------------------------------------------------------
    // File operations
    //-----------------------------------------------------------------------------


    bool LOGGER::Logger::openLogFile(const std::filesystem::path& Path) 
    {
        if (Path.empty())
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Error [openLogFile] : Path empty\n");
            return false;
        }

        std::lock_guard<std::mutex> fileLock(fileMutex);
        logFile.open(Path, std::ios_base::app);

        if (!logFile) 
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Unable to open log file: '%s'. Check permissions and disk space.\n", Path.string().c_str());
            return false;
        }

        return true;
    }




    void LOGGER::Logger::closeLogFile() 
    {
        std::lock_guard<std::mutex> fileLock(fileMutex);
        if (logFile.is_open()) 
        {
            logFile.close();
        }
    }




    void LOGGER::Logger::writeBufferToFile(const std::deque<std::string> &q_buffer)
    {            
        std::lock_guard<std::mutex> guard(fileMutex);

        if (!logFile.good())
        {
            Error(eDLL_T::SERVER, NO_ERROR, "The log file stream is in a bad state before writing.\n");
            return;
        }

        for (const std::string& logstring : q_buffer)
        {   
            logFile << logstring << "\n";
        }

        logFile.flush();
    }
 


    void LOGGER::Logger::logToFile()
    {
        std::deque<std::string> writeBuffer;

        this->setLogState(LogState::Busy, true);
        this->setLogState(LogState::Ready, true);

        try
        {
            while (true)
            {
                {
                    std::unique_lock<std::mutex> lock(mtx);
                    cvLog.wait(lock, [this] { return !logQueue.empty() || finished; });

                    if (buffer.size() >= CVAR_MAX_BUFFER || (finished && !buffer.empty()))
                    {
                        std::swap(buffer, writeBuffer);
                    }

                    while (!logQueue.empty())
                    {
                        buffer.push_back(logQueue.front());
                        logQueue.pop();
                    }
                }

                if (!writeBuffer.empty())
                {   
                    writeBufferToFile(writeBuffer);
                    writeBuffer.clear();
                }

                if (finished && buffer.empty() && writeBuffer.empty())
                {
                    break;
                }
            }
        }
        catch (const std::exception& e)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Exception in logToFile thread: %s\n", e.what());
        }
        catch (...)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Unknown exception in logToFile thread.\n");
        }

        this->setLogState(LogState::Safe, false);
        this->setLogState(LogState::Busy, false);
        this->setLogState(LogState::Ready, false);
    }





    //-----------------------------------------------------------------------------
    // Thread management
    //-----------------------------------------------------------------------------


    //Log controlled stop logging
    void Logger::stopLoggingThread() 
    {
        
        if (isLogging()) 
        {
            finished = true;
            VScriptCode::Shared::setMatchID(0);

            {
                std::unique_lock<std::mutex> lock(mtx);
                cvLog.notify_all();
            }

            Warning(eDLL_T::SERVER, "Stopping logging thread...\n\n");

            if (logThread.joinable()) 
            {
                logThread.join();
            }
            else
            {
                setLogState(LogState::Busy, false);
            }

            bool wait = WaitForState(LogState::Busy, false, 5000);
            if (!wait)
            {
                Warning(eDLL_T::SERVER, "BUSY STATE FAILED TO RELEASE IN STOPLOGGING\n\n");
                return;
            }

            CallClosure();

        }
        else 
        {
            DevMsg(eDLL_T::RTECH, "No active logging thread.\n\n");
        }
    }




    void LOGGER::Logger::handleNewMatch(const char* matchID)
    {   
        if (!matchID)
        {   
            Error(eDLL_T::SERVER, NO_ERROR, "matchID was nullptr at handlenewmatch\n");
            return;
        }

        if (isLogging()) 
        {
            Warning(eDLL_T::SERVER, "WARNING: LOG THREAD WAS RUNNING DURING HANDLE NEW MATCH\n");
            stopLoggingThread();
        }

        startLogging();
        Warning(eDLL_T::SERVER, ":::::::::::::::::::::::::::::::::::::: Logging thread started :::::::\n");
        Warning(eDLL_T::SERVER, ":::::: Match started with MatchID:  [  %s  ] :::::::\n", matchID);
    }




    void LOGGER::Logger::CallClosure()
    {
        closeLogFile();
        ResetLogPath();
        setLogState(LogState::Busy, false);
        setLogState(LogState::Ready, true);
    }

  
    void LOGGER::Logger::stopLogging(bool sendToAPI)
    {
        std::thread stopThread(&LOGGER::Logger::stopLogging_Async, this, sendToAPI);
        stopThread.detach();
    }
   

    // SQVM controlled-Stop logging true/false for api stat send
    void LOGGER::Logger::stopLogging_Async(bool sendToAPI) //
    {   
        SaveEndingMatchID();

        if (!logThread.joinable()) 
        {
            DevMsg(eDLL_T::RTECH, "No active logging thread to stop.\n");
            return;
        }

        Warning(eDLL_T::SERVER, "Logger thread stopping...");

        {
            std::unique_lock<std::mutex> lock(mtx);
            finished = true;
            VScriptCode::Shared::setMatchID(0);
            cvLog.notify_all();
        }

        bool close = WaitForState(LogState::Busy, false, 5000);

        if (!close) 
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Logger wait time expired during shutdown...\n");
            return;
        }
        else
        {
            Warning(eDLL_T::SERVER, "done..\n");
        }

        if (logThread.joinable())
        {
            logThread.join();
        }

        if (sendToAPI) 
        {   
            DevMsg(eDLL_T::SERVER, " .. Shipping logfile to tracker https://r5r.dev \n");

            {
                std::lock_guard<std::mutex> apilock(file_mtx);
                if (apiThread.joinable()) 
                {
                    apiThread.join();
                }
                apiThread = std::thread(&Logger::sendLogToAPI, this);
            }
        }
        else 
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Stats shipping omitted;\n");
            CallClosure();
        }

    }
    



    void LOGGER::Logger::UpdateMatchId(const std::string& matchId) 
    {
        std::unique_lock<std::shared_mutex> lock(pathMutex);
        currentMatchId = matchId; 
        pCurrentLogPath = nullptr;
    }




    void LOGGER::Logger::ResetLogPath()
    {
        std::unique_lock<std::shared_mutex> writeLock(pathMutex);
        filePath.clear();
        pCurrentLogPath = nullptr;
    }



    std::filesystem::path* LOGGER::Logger::InitializeAndGetLogPath() 
    {
        std::unique_lock<std::shared_mutex> lock(pathMutex);

        if (pCurrentLogPath == nullptr) 
        {
            const char* folder_setting = GetSetting("logfolder");
            std::string folderPathStr = folder_setting ? std::string(folder_setting) : "eventlogs";
            std::filesystem::path dirPath = std::filesystem::path("platform") / folderPathStr;

            bool dir_created = false;

            try 
            {
                dir_created = std::filesystem::create_directories(dirPath);

                if (dir_created) 
                {
                    DevMsg(eDLL_T::SERVER, "Created logging directory: '%s'\n", dirPath.string().c_str());
                }
            }

            catch (const std::filesystem::filesystem_error& e) 
            {
                Error(eDLL_T::SERVER, NO_ERROR, "Failed to create logging directory: '%s'. Error: %s\n", dirPath.string().c_str(), e.what());
                return nullptr;
            }

            filePath = dirPath / (currentMatchId + ".json");
            pCurrentLogPath = &filePath;
        }
        else
        {
            Error(eDLL_T::SERVER, NO_ERROR, "balls");
        }

        return pCurrentLogPath;
    }





    bool Logger::WaitForState(const LogState state, bool flag, const int timeout_in_ms)
    {
        std::chrono::milliseconds timeout(timeout_in_ms);
        std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();

        while (getLogState(state) != flag) 
        { 
            if (std::chrono::steady_clock::now() - start > timeout) 
            {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        return true;
    }




    void LOGGER::Logger::ThreadSleep(int ms)
    {
        long long duration = static_cast<std::chrono::milliseconds::rep>(ms);
        std::this_thread::sleep_for(std::chrono::milliseconds(duration));
    }




    void LOGGER::Logger::InitializeLogThread(bool encrypt)
    {
        setLogState(LogState::Safe, false);
        std::thread initThread(&LOGGER::Logger::InitializeLogThread_Async, this, encrypt);
        initThread.detach();
    }




    void LOGGER::Logger::InitializeLogThread_Async(bool encrypt)
    {   
        bool success = WaitForState(LogState::Busy, false, 5000);

        if (!success) 
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Logger exceeded busy state limit. Aborting logthread start...");
            return;
        }

        if (isLogging())
        {
            stopLoggingThread();
        }

        const std::string matchID = std::to_string(VScriptCode::Shared::getMatchID());
        UpdateMatchId(matchID);
        std::filesystem::path* pFilePath = InitializeAndGetLogPath();

        try
        {   

            if ( !pFilePath)
            {
                Error(eDLL_T::SERVER, NO_ERROR, "Error: [InitializeLogThread_Async] file path nullptr \n");
                return;
            }

            if (matchID.empty())
            {
                Error(eDLL_T::SERVER, NO_ERROR, "[InitializeLogThread_Async] matchID was empty.. failed to start logging\n");
                return;
            }

            handleNewMatch(matchID.c_str());

            if ( pFilePath && !openLogFile(*pFilePath))
            {
                Error(eDLL_T::SERVER, NO_ERROR, "CRITICAL ERROR: Could not open file to write (nullptr?)\n");
                stopLoggingThread();
                return;
            }
        }
        catch (const std::runtime_error& e)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Runtime Error starting logging thread: %s\n", e.what());
            return;
        }
        catch (const std::exception& e)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Exception starting logging thread: %s\n", e.what());
            return;
        }
        catch (...)
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Unknown Exception starting logging thread.\n");
            return;
        }

        std::vector<std::string> startLines; 

        std::string serverName = ::IsDedicated() ? hostname->GetString() : g_pServerListManager->m_Server.m_svHostName;
        std::string serverMap = g_pHostState->m_levelName;
        std::string gameType = KeyValues_GetCurrentPlaylist();
   
        startLines.push_back("|#MatchID:" + matchID);
        startLines.push_back("|#Gameversion:" + SERVER_V);
        startLines.push_back("|#Gametype:" + gameType);
        startLines.push_back("|#ServerName:" + serverName);
        startLines.push_back("|#ServerMAP:" + serverMap);

        if (encrypt)
        {
            std::lock_guard<std::mutex> lock(mtx);
            for (std::string& line : startLines)
            {
                line = this->eObj.doEncrypt(line, keyHex, ivHex);
                logQueue.push(line);
            }
        }
        else
        {
            std::lock_guard<std::mutex> lock(mtx);
            for (std::string& line : startLines)
            {
                logQueue.push(line);
            }
        }
        
        {
            std::unique_lock<std::mutex> lock(mtx);
            cvLog.notify_all();
        }

        setLogState(LogState::Safe, true);
    }




    //-----------------------------------------------------------------------------
    // Called by SQVM for each log event. flags: encrypt true/false
    //-----------------------------------------------------------------------------


    void LOGGER::Logger::LogEvent(const char* logString, bool encrypt)
    {

        if ( !getLogState(LogState::Safe) || finished )
        {
            Error(eDLL_T::SERVER, NO_ERROR, "Tried to queue to log but logthread is not fully initialized. \n");
            return;
        }

        if (!logString)
        {
            Warning(eDLL_T::SERVER, "Logstring @ logevent is: NULLPTR \n");
            return;
        }

        std::vector<std::string> lines = splitString(logString, "\n");

        if (encrypt)
        {
            std::lock_guard<std::mutex> lock(mtx);
            for (std::string& line : lines)
            {
                line = this->eObj.doEncrypt(line, keyHex, ivHex);
                logQueue.push(line);
            }
        }
        else
        {
            std::lock_guard<std::mutex> lock(mtx);
            for (std::string& line : lines)
            {
                logQueue.push(line);
                cvLog.notify_one();
            }
        }

        {
            std::unique_lock<std::mutex> lock(mtx);
            cvLog.notify_all();
        }
    }

} // namespace LOGGER