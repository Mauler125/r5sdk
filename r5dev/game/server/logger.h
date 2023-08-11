#include <string>
#include <deque>
#include <queue>
#include <mutex>
#include <thread>
#include <tiny-aes/aes.hpp>


#ifndef logger_h
#define logger_h


namespace LOGGER {


					namespace aesc {
						std::string bbase64Encode(std::vector<uint8_t> bytes_to_encode);
						std::string doEncrypt(const std::string& plainText, const std::vector<uint8_t>& keyBytes, const std::vector<uint8_t>& ivBytes);
						std::vector<uint8_t> hex2bytes(const std::string& hex);
					}



					void LogEvent(const char* filename, const char* logString, bool checkDir, bool encrypt);
					void startLogging();
					void stopLogging(bool sendToAPI);
					std::string getLatestFile(const std::string& directoryPath);
					std::string sanitizeFilename(const std::string& filename);
					std::string getServerIP();
				

					
					std::vector<std::string> splitString(std::string str, const std::string& delimiter);
					void sendLogToAPI(const std::string& logdata);
					size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
					std::string replace_all(std::string str, const std::string& from, const std::string& to);
					std::string url_encode(const std::string& value);
					void writeBufferToFile(const std::deque<std::pair<std::string, std::string>>& buffer);
					void logToFile();
					
}

#endif // logger_h