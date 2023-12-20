#ifndef LOGGER_H
#define LOGGER_H
#include <string>
#include <deque>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <unordered_map>
#include <atomic>
#include <vector>


namespace LOGGER {

    class Encryption {
    public:
        std::vector<uint8_t> hex2bytes(const std::string& hex);
        std::string bbase64Encode(std::vector<uint8_t> bytes_to_encode);
        std::string doEncrypt(const std::string& plainText, const std::vector<uint8_t>& keyBytes, const std::vector<uint8_t>& ivBytes);
        static const std::string base64_chars;
        ~Encryption() = default;
    };

    class Logger {
    public:
        static Logger& getInstance();

        // Main Logging Functions
        void LogEvent(const char* filename, const char* logString, bool checkDir, bool encrypt);
        void startLogging();
        void stopLogging(bool sendToAPI);

        // Check if logging is currently active
        void stopLoggingThread();
        bool isLogging();

        // Utility Functions
        void handleNewMatch(const std::string& sanitizedFilename);
        std::string getLatestFile(const std::string& directoryPath);
        std::string sanitizeFilename(const std::string& filename);
        std::vector<std::string> splitString(std::string str, const std::string& delimiter);
        void sendLogToAPI(const std::string& logdata);

    private:
        Logger();
        ~Logger();

        // ensures only one instance
        Logger(const Logger&) = delete; 
        Logger& operator=(const Logger&) = delete;

        // aes encryption object
        Encryption eObj;

        // communicate with API / write log file
        void writeBufferToFile(const std::deque<std::pair<std::string, std::string>>& buffer);
        void logToFile();

        // synchronization
        std::atomic<bool> finished{true};
        std::mutex cache_mtx;
        std::unordered_map<std::string, std::string> sanitizedFilenameCache;
        std::mutex file_mtx;
        std::thread apiThread;
        std::mutex mtx;
        std::condition_variable cvLog;
        std::queue<std::pair<std::string, std::string>> logQueue;
        std::thread logThread;
    };

    // Functions for sendtoapi
    std::string url_encode(const std::string& value);
    std::string replace_all(std::string str, const std::string& from, const std::string& to);

    //Funciton for verify
    const std::string VERIFY_EA_ACCOUNT(const std::string& token, const std::string& OID, const std::string& ea_name);

    //Api call to player count
    void PlayerCountUpdate(const std::string& action, const std::string& player, const std::string& OID, const std::string& count, const std::string& DISCORD_HOOK);
    const std::string UPDATE_PLAYER_COUNT(const std::string& action, const std::string& player, const std::string& OID, const std::string& count, const std::string& DISCORD_HOOK);

    //Api call for end game
    void EndMatchUpdate(const std::string& recap, const std::string& DISCORD_HOOK);
    const std::string NOTIFY_END_OF_MATCH(const std::string& recap, const std::string& DISCORD_HOOK);
    
}

#endif // LOGGER_H