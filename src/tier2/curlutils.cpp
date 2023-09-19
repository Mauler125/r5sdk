//===========================================================================//
//
// Purpose: A set of utilities to perform requests
//
//===========================================================================//
#include "tier1/cvar.h"
#include "tier2/curlutils.h"

size_t CURLWriteStringCallback(char* data, const size_t size, const size_t nmemb, string* userp)
{
    userp->append(data, size * nmemb);
    return size * nmemb;
}

size_t CURLWriteFileCallback(void* data, const size_t size, const size_t nmemb, FILE* userp)
{
    const size_t numBytesWritten = fwrite(data, size, nmemb, userp);
    return numBytesWritten;
}

void CURLInitCommonOptions(CURL* curl, const char* remote,
    const void* writeData, const CURLParams& params)
{
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, params.timeout);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, params.verbose);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, params.followRedirect);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, params.verifyPeer);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_URL, remote);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, writeData);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "R5R HTTPS/1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, params.writeFunction);
}

bool CURLDownloadFile(const char* remote, const char* savePath, const char* fileName,
    const char* options, curl_off_t dataSize, void* customPointer, const CURLParams& params)
{
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        Error(eDLL_T::COMMON, NO_ERROR, "CURL: %s\n", "Easy init failed");
        return false;
    }

    string filePath = savePath;

    AppendSlash(filePath);
    filePath.append(fileName);

    FILE* file = fopen(filePath.c_str(), options);
    if (!file)
    {
        Error(eDLL_T::COMMON, NO_ERROR, "CURL: %s\n", "Open file failed");
        curl_easy_cleanup(curl);

        return false;
    }

    CURLProgress progressData;

    progressData.curl = curl;
    progressData.name = fileName;
    progressData.cust = customPointer;
    progressData.size = dataSize;

    CURLInitCommonOptions(curl, remote, file, params);

    if (params.statusFunction)
    {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0l);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progressData);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, params.statusFunction);
    }

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        Error(eDLL_T::COMMON, NO_ERROR, "CURL: Download of file '%s' failed; %s\n",
            fileName, curl_easy_strerror(res));

        curl_easy_cleanup(curl);
        fclose(file);

        return false;
    }

    curl_easy_cleanup(curl);
    fclose(file);

    return true;
}

CURL* CURLInitRequest(const char* remote, const char* request,
    string& outResponse, curl_slist*& slist, const CURLParams& params)
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

    CURLInitCommonOptions(curl, remote, &outResponse, params);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    if (request)
    {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
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
