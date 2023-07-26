//===========================================================================//
//
// Purpose: A set of utilities to perform requests
//
//===========================================================================//
#include "tier1/cvar.h"
#include "tier2/curlutils.h"

size_t CURLWriteStringCallback(char* contents, const size_t size, const size_t nmemb, void* userp)
{
    reinterpret_cast<string*>(userp)->append(contents, size * nmemb);
    return size * nmemb;
}

CURL* CURLInitRequest(const char* remote, const char* request, string& outResponse, curl_slist*& slist,
    const int timeOut, const bool verifyPeer, const bool debug, const void* writeFunction)
{
    std::function<void(const char*)> fnError = [&](const char* errorMsg)
    {
        Error(eDLL_T::COMMON, NO_ERROR, "CURL: %s\n", errorMsg);
        curl_slist_free_all(slist);
    };

    slist = curl_slist_append(slist, "Content-Type: application/json");
    if (!slist)
    {
        fnError("Slist init failed");
        return nullptr;
    }

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        fnError("Easy init failed");
        return nullptr;
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_URL, remote);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeOut);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CURLWriteStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outResponse);

    curl_easy_setopt(curl, CURLOPT_VERBOSE, debug);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    if (!verifyPeer)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    }

    return curl;
}

CURLcode CURLSubmitRequest(CURL* curl, curl_slist*& slist)
{
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(slist);

    return res;
}

CURLINFO CURLRetrieveInfo(CURL* curl)
{
    CURLINFO status;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_cleanup(curl);

    return status;
}

bool CURLHandleError(CURL* curl, const CURLcode res, string& outMessage, const bool logError)
{
    if (res == CURLE_OK)
        return true;

    const char* curlError = curl_easy_strerror(res);

    if (logError)
        Error(eDLL_T::COMMON, NO_ERROR, "CURL: %s\n", curlError);

    outMessage = curlError;
    curl_easy_cleanup(curl);

    return false;
}

void CURLFormatUrl(string& outUrl, const char* host, const char* api)
{
    outUrl = Format("%s%s%s", "https://", host, api);
}
