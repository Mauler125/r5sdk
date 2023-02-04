//===========================================================================//
//
// Purpose: A set of utilities to perform requests
//
//===========================================================================//
#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "curlutils.h"

size_t CURLWriteStringCallback(char* contents, size_t size, size_t nmemb, void* userp)
{
    reinterpret_cast<string*>(userp)->append(contents, size * nmemb);
    return size * nmemb;
}

CURL* CURLInitRequest(const string& hostname, const string& request, string& response, curl_slist* slist)
{
    std::function<void(const char*)> fnError = [&](const char* errorMsg)
    {
        Error(eDLL_T::ENGINE, NO_ERROR, "CURL: %s\n", errorMsg);
        curl_slist_free_all(slist);
        return nullptr;
    };

    slist = curl_slist_append(slist, "Content-Type: application/json");
    if (!slist)
    {
        fnError("Slist init failed");
    }

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        fnError("Easy init failed");
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_URL, hostname.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CURLWriteStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    curl_easy_setopt(curl, CURLOPT_VERBOSE, curl_debug->GetBool());

    if (!ssl_verify_peer->GetBool())
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    }

    return curl;
}

CURLcode CURLSubmitRequest(CURL* curl, curl_slist* slist)
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

bool CURLHandleError(CURL* curl, CURLcode res, string& outMessage)
{
    if (res == CURLE_OK)
        return true;

    const char* curlError = curl_easy_strerror(res);
    Error(eDLL_T::ENGINE, NO_ERROR, "CURL: %s\n", curlError);

#ifndef DEDICATED // Error already gets logged to the console.
    outMessage = curlError;
#endif // !DEDICATED
    curl_easy_cleanup(curl);

    return false;
}

void CURLFormatUrl(string& url, const string& host, const string& api)
{
    url = fmt::format("{:s}{:s}{:s}", "https://", host, api);
}