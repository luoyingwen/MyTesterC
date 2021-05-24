#pragma once
#include "WinHttpClient.h"
#pragma comment(lib, "comsuppw.lib")

static int s_tokenStatus = 0;
#define Success_State 100

std::wstring utf82wstr(const char* utf8str)
{
    static const int MAXLEN = 2 * 1024;
    wchar_t szBuf[MAXLEN] = { 0 };
    MultiByteToWideChar(CP_UTF8, 0, utf8str, strlen(utf8str), szBuf, MAXLEN);
    return szBuf;
}


bool HasValidAccessRight()
{
    if (s_tokenStatus == Success_State)
    {
        return true;
    }
    else
    {
        ::OutputDebugStringW(L"Has no access right");
        return false;
    }
}

bool RequestAccessRight(const char* ak, const char* sk, const char* packageName)
{
    if (packageName == nullptr || strlen(packageName) > MAX_PATH)
    {
        ::OutputDebugStringW(L"Invalid package name");
        return false;
    }

    WinHttpClient postClient(L"https://ocr.lenovo.com/authorization/oauth2/token");

    // Post data.
    string data = "grant_type=client_credentials";
    data.append("&client_id=");
    data.append(ak);
    data.append("&client_secret=");
    data.append(sk);
    postClient.SetAdditionalDataToSend((BYTE*)data.c_str(), data.size());

    // Post headers.
    wstring headers;
    headers.append(L"smartcamera-app-id:");
    headers.append(utf82wstr(packageName));
    headers.append(L"\r\n");
    headers.append(L"smartcamera-device-id:234\r\n");
    headers.append(L"Content-Type: application/x-www-form-urlencoded\r\n");
    headers.append(L"Content-Length: %d\r\n");
    wchar_t szHeaders[MAX_PATH * 10] = L"";
    swprintf_s(szHeaders, MAX_PATH * 10, headers.c_str(), data.size());
    postClient.SetAdditionalRequestHeaders(szHeaders);
    if (!postClient.SendHttpRequest(L"POST", true))
    {
        ::OutputDebugStringW(L"Internet connection error");
        return false;
    }
    wstring httpResponseHeader = postClient.GetResponseHeader();
    wstring httpResponseContent = postClient.GetResponseContent();
    const size_t pos = httpResponseContent.find(L"access_token");
    if (pos != std::string::npos)
    {
        s_tokenStatus = Success_State;
        ::OutputDebugStringW(L"Valid access right");
        return true;
    }
    else
    {
        ::OutputDebugStringW(L"Invalid access right");
        return false;
    }
}
