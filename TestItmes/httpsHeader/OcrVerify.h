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

void LogErrorMessage(const TCHAR* _Format, ...)
{
    if (_Format == NULL)
        return;

    va_list l;
    va_start(l, _Format);

    TCHAR szLine[MAX_PATH + 4];
    int count = _vsntprintf_s(szLine, MAX_PATH, MAX_PATH - 1, _Format, l);
    if (count <= 0)
        _tcscat_s(szLine, MAX_PATH + 4, _T("..."));
    else
        szLine[MAX_PATH] = '\0';
    va_end(l);

    ::OutputDebugStringW(szLine);
}

bool QueryStringValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR pszValueName, CString& value)
{
    CRegKey regKey;
    LONG lRet = regKey.Open(hKeyParent, lpszKeyName, KEY_READ);
    if (lRet == ERROR_SUCCESS)
    {
        TCHAR buffer[MAX_PATH + MAX_PATH];
        DWORD dwLen = sizeof(buffer) / sizeof(TCHAR);
        lRet = regKey.QueryStringValue(pszValueName, buffer, &dwLen);
        if (lRet == ERROR_SUCCESS)
        {
            value = buffer;
        }
        else
        {
            LogErrorMessage(L"regKey.QueryStringValue for %s failed, code = %x.", pszValueName, lRet);
        }
    }
    else
    {
        LogErrorMessage(L"Open RegKey : %s failed", lpszKeyName);
    }
    return lRet == ERROR_SUCCESS;
}

bool QuerySystemModel(CString& value)
{
    return QueryStringValue(HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\SystemInformation",
        L"SystemProductName",
        value);
}

bool QueryHardwareId(CString& value)
{
    return QueryStringValue(HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\SystemInformation",
        L"ComputerHardwareId",
        value);
}


bool RequestAccessRight(const char* ak, const char* sk)
{
    CString systemModel;
    CString hardwareId;
    if (!QuerySystemModel(systemModel))
        return false;

    systemModel = L"com.lenovotab.camera"; //TODO:  test only
    if (!QueryHardwareId(hardwareId))
        hardwareId = "unknown_hardwareId";

    const wchar_t* szPackageName = systemModel;
    if (szPackageName == nullptr || wcslen(szPackageName) > MAX_PATH)
    {
        ::OutputDebugStringW(L"Invalid system model");
        return false;
    }

    if (ak == nullptr || strlen(ak) > MAX_PATH || sk == nullptr || strlen(sk) > MAX_PATH)
    {
        ::OutputDebugStringW(L"Invalid ak & sk");
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
    headers.append(szPackageName);
    headers.append(L"\r\n");
    headers.append(L"smartcamera-device-id:");
    headers.append(hardwareId);
    headers.append(L"\r\n");
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
