#ifndef TIER2_CURLUTILS_H
#define TIER2_CURLUTILS_H

size_t CURLWriteStringCallback(char* contents, size_t size, size_t nmemb, void* userp);

CURL* CURLInitRequest(const string& hostname, const string& request, string& response, curl_slist* slist);
CURLcode CURLSubmitRequest(CURL* curl, curl_slist* slist);
CURLINFO CURLRetrieveInfo(CURL* curl);

bool CURLHandleError(CURL* curl, CURLcode res, string& outMessage);
void CURLFormatUrl(string& url, const string& host, const string& api);

#endif // !TIER2_CURLUTILS_H