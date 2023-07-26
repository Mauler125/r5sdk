#ifndef TIER2_CURLUTILS_H
#define TIER2_CURLUTILS_H

struct CURLProgress
{
	CURL* priv;
	size_t size;
};

size_t CURLWriteStringCallback(char* contents, const size_t size, const size_t nmemb, void* userp);

CURL* CURLInitRequest(const char* remote, const char* request, string& outResponse, curl_slist*& slist,
	const int timeOut, const bool verifyPeer, const bool debug, const void* writeFunction = CURLWriteStringCallback);
CURLcode CURLSubmitRequest(CURL* curl, curl_slist*& slist);
CURLINFO CURLRetrieveInfo(CURL* curl);

bool CURLHandleError(CURL* curl, const CURLcode res, string& outMessage, const bool logError);
void CURLFormatUrl(string& outUrl, const char* host, const char* api);

#endif // !TIER2_CURLUTILS_H