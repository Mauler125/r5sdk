#ifndef LOGGER_H
#define LOGGER_H
#include <string>
#include <deque>
#include <queue>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <condition_variable>
#include <unordered_map>
#include <atomic>
#include <vector>
#include "thirdparty/curl/include/curl/curl.h"

namespace LOGGER 
{
    

    class Encryption 
    {
        public:
            std::vector<uint8_t> hex2bytes(const std::string& hex);
            std::string bbase64Encode(std::vector<uint8_t> bytes_to_encode);
            std::string doEncrypt(const std::string& plainText, const std::vector<uint8_t>& keyBytes, const std::vector<uint8_t>& ivBytes);
            static const std::string base64_chars;
            ~Encryption() = default;
    };

    class Logger 
    {
        public:

            static Logger& getInstance();
            

            enum class LogState : uint8_t {
                None = 0,
                Ready = 1 << 0,
                Busy = 1 << 1,
                Safe = 1 << 2
            };

            LogState intToLogState(int flag);
  
            // Main Logging Functions
            bool getLogState(LogState flag) const; //bitstate callable from sqvm
            void InitializeLogThread(bool encrypt); //callable from sqvm
            void LogEvent(const char* logString, bool encrypt); //callable from sqvm
            void stopLogging(bool sendToAPI); //callable from sqvm //wrapper
            bool isLogging(); //callable from sqvm


            //utility
            std::string GetLatestFile(const std::string& directoryPath, std::string matchID);
            void stopLoggingThread(); 
            void startLogging();  
            void sendLogToAPI();
            void ThreadSleep(int ms);
            bool WaitForState(const LogState state, bool flag, const int timeout_in_ms);

            //file management
            void logToFile();
            void CallClosure();  
            bool openLogFile(const std::filesystem::path& filePath);
            void closeLogFile();

            std::ofstream logFile; //manual handle management >.>
            std::mutex fileMutex;

            //vars
            int CVAR_LTHREAD_DEBOUNCE = 1;
            size_t CVAR_MAX_BUFFER = 30;

        private:

            Logger();
            ~Logger();

            // ensures only one instance
            Logger(const Logger&) = delete; 
            Logger& operator=(const Logger&) = delete;

            // pointers
            std::filesystem::path* pCurrentLogPath = nullptr;
            std::filesystem::path filePath;
            std::string currentMatchId;

            // aes encryption object
            Encryption eObj;

            std::vector<uint8_t> keyHex;
            std::vector<uint8_t> ivHex;

            //structs
            std::deque<std::string> buffer;

            // func
            void InitializeLogThread_Async(bool encrypt);
            void setLogState(LogState flag, bool value); //bit state
            void stopLogging_Async(bool sendToAPI);
            void writeBufferToFile(const std::deque<std::string>& q_buffer);
            void UpdateMatchId(const std::string& matchId);
            std::filesystem::path* InitializeAndGetLogPath();
            void ResetLogPath();
            void handleNewMatch(const char* matchID);
            std::vector<std::string> splitString(std::string str, const std::string& delimiter);        

            // synchronization
            std::atomic<uint8_t> StateBits;
            std::atomic<bool> finished{true};
            std::mutex file_mtx;
            std::thread apiThread;
            std::mutex mtx;
            std::condition_variable cvLog;
            std::queue<std::string> logQueue;
            std::thread logThread;
            std::shared_mutex pathMutex;
    };

    class TaskManager 
    {
        public:
            static TaskManager& getInstance();

            void AddTask(const std::function<void()>& task);
            void LoadKDString(const char* player_oid);
            void ResetPlayerStats(const char* player_oid);
            void LoadBatchKDStrings(const std::string& player_oids_str);

        private:
            TaskManager();
            ~TaskManager();
            TaskManager(const TaskManager&) = delete;
            TaskManager& operator=(const TaskManager&) = delete;

            void StartWorkerThread();
            void StopWorkerThread();
            void ProcessTasks();

            std::queue<std::function<void()>> taskQueue;
            std::mutex queueMutex;
            std::condition_variable condVar;
            std::thread workerThread;
            std::atomic<bool> stop_tasks_flag{false};
    };

    class CURLConnectionPool 
    {
        public:

            static CURLConnectionPool& GetInstance();

            CURL* GetHandle();
            bool HandleCurlResult(CURL* handle, CURLcode res, const char* func);
            void DiscardHandle(CURL* handle);
            void ReturnHandle(CURL* handle);
            void ResetPool();
            ~CURLConnectionPool();

        private:

            std::queue<CURL*> pool;
            std::mutex poolMutex;

            CURLConnectionPool();

            CURLConnectionPool(const CURLConnectionPool&) = delete;
            CURLConnectionPool& operator=(const CURLConnectionPool&) = delete;

            std::condition_variable poolCond;
            const size_t maxPoolSize = 5;
            const std::chrono::seconds handleWaitTimeout = std::chrono::seconds(3);

            CURL* CreateHandle();
    };

    //pointers
    extern LOGGER::Logger* pMkosLogger;


    //maintenance
    void CleanupLogs(IFileSystem* pFileSystem);
    void SaveEndingMatchID();
    std::string GetEndingMatchID();
    

    //settings
    void AddToConfigMap(const rapidjson::Value& value, const std::string& parentKey, int depth);
    void LoadConfig(IFileSystem* pFileSystem, const char* configFileName);
    void ReloadConfig(const char* configFileName);
    const char* GetSetting(const char* key);
    int64_t GetMaxLogfileSize(const char* settingValue);

    // Functions for sendtoapi
    std::string url_encode(const std::string& value);
    std::string replace_all(std::string str, const std::string& from, const std::string& to);

    //Funciton for verify
    const std::string VERIFY_EA_ACCOUNT(const std::string& token, const std::string& OID, const std::string& ea_name);

    //Api call to player count
    void PlayerCountUpdate(const char* action, const char* player, const char* OID, const char* count, const char* DISCORD_HOOK);
    void UPDATE_PLAYER_COUNT(const char* action, const char* player, const char* OID, const char* count, const char* DISCORD_HOOK);

    //Api call for end game
    void EndMatchUpdate( std::string recap, std::string DISCORD_HOOK);
    void NOTIFY_END_OF_MATCH(const char* recap, const char* DISCORD_HOOK);

    //Api calls for stats
    static std::unordered_map<std::string, std::string> playerStatsMap;
    static std::shared_timed_mutex statsMutex;
    std::string FetchPlayerStats(const char* player_oid); // on player connect if batch is complete only
    std::string FetchBatchPlayerStats(const std::vector<std::string>& player_oids); //on startup only
    const char* GetKDString(const char* player_oid); //on startup / player connect
    void RunUpdateLiveStats(std::string stats_json); //onshutdown dispatch thread
    void UpdateLiveStats(std::string stats_json); //onshutdown 
    std::string FetchGlobalSettings(const char* query);//on startup init
    
}

#endif // LOGGER_H