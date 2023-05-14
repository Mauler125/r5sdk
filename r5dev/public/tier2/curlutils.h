#ifndef TIER2_CURLUTILS_H
#define TIER2_CURLUTILS_H

extern ConVar* ssl_verify_peer;
extern ConVar* curl_timeout;
extern ConVar* curl_debug;

size_t CURLWriteStringCallback(char* contents, const size_t size, const size_t nmemb, void* userp);

CURL* CURLInitRequest(const char* remote, const char* request, string& outResponse, curl_slist*& slist);
CURLcode CURLSubmitRequest(CURL* curl, curl_slist*& slist);
CURLINFO CURLRetrieveInfo(CURL* curl);

bool CURLHandleError(CURL* curl, CURLcode res, string& outMessage);
void CURLFormatUrl(string& outUrl, const char* host, const char* api);

#endif // !TIER2_CURLUTILS_H