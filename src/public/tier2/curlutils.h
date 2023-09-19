#ifndef TIER2_CURLUTILS_H
#define TIER2_CURLUTILS_H

struct CURLProgress
{
	CURLProgress()
		: curl(nullptr)
		, name(nullptr)
		, cust(nullptr)
		, size(0)
	{}

	CURL* curl;
	const char* name;
	void* cust; // custom pointer to anything.
	size_t size;
};

struct CURLParams
{
	CURLParams()
		: writeFunction(nullptr)
		, statusFunction(nullptr)
		, timeout(0)
		, verifyPeer(false)
		, followRedirect(false)
		, verbose(false)
	{}

	void* writeFunction;
	void* statusFunction;

	int timeout;
	bool verifyPeer;
	bool followRedirect;
	bool verbose;
};

size_t CURLWriteStringCallback(char* contents, const size_t size, const size_t nmemb, string* userp);
size_t CURLWriteFileCallback(void* data, const size_t size, const size_t nmemb, FILE* userp);

bool CURLDownloadFile(const char* remote, const char* savePath, const char* fileName,
	const char* options, curl_off_t dataSize, void* customPointer, const CURLParams& params);

CURL* CURLInitRequest(const char* remote, const char* request, string& outResponse,
	curl_slist*& slist, const CURLParams& params);

CURLcode CURLSubmitRequest(CURL* curl, curl_slist*& slist);
CURLINFO CURLRetrieveInfo(CURL* curl);

bool CURLHandleError(CURL* curl, const CURLcode res, string& outMessage, const bool logError);
void CURLFormatUrl(string& outUrl, const char* host, const char* api);

#endif // !TIER2_CURLUTILS_H