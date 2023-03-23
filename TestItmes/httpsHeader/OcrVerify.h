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


std::string wchar2utf8(const wchar_t* str)
{
    static const int MAXLEN = 2 * 1024;
    char szBuf[MAXLEN] = { 0 };
    WideCharToMultiByte(CP_UTF8, 0, str, wcslen(str), szBuf, MAXLEN, NULL, NULL);
    return szBuf;
}

int ParseHex(wchar_t one)
{
    if (one >= '0' && one <= '9')
    {
        return one - '0';
    }
    else if (one >= 'a' && one <= 'f')
    {
        return one - 'a' + 10;
    }
    else
    {
        assert(0);
        return 0;
    }
}

std::wstring hexstring2whar(std::wstring& original)
{
    wstring result;
    int length = original.length();
    bool lastCharIsUnicode = false;
    const wchar_t* strRaw = original.c_str();
    for (int i = 0; i < length; i++)
    {
        if (strRaw[i] == '"' 
            && i + 7 < length
            && strRaw[i + 1] == '\\'
            && strRaw[i + 2] == 'u'
            && (strRaw[i + 7] == '"' || strRaw[i + 7] == '\\')
            )
        {
            int part1 = ParseHex(strRaw[i + 3]);
            int part2 = ParseHex(strRaw[i + 4]);
            int part3 = ParseHex(strRaw[i + 5]);
            int part4 = ParseHex(strRaw[i + 6]);
            i += 6;
            result.append(1, '"');
            //add unicode char
            wchar_t target = part1 * 4096 + part2 * 256 + part3 * 16 + part4;
            result.append(1, target);
            lastCharIsUnicode = true;
        }
        else if (lastCharIsUnicode
            && i + 6 < length
            && strRaw[i] == '\\'
            && strRaw[i + 1] == 'u'
            && (strRaw[i + 6] == '"' || strRaw[i + 6] == '\\')
            )
        {
            int part1 = ParseHex(strRaw[i + 2]);
            int part2 = ParseHex(strRaw[i + 3]);
            int part3 = ParseHex(strRaw[i + 4]);
            int part4 = ParseHex(strRaw[i + 5]);
            i += 5;
            //add unicode char
            wchar_t target = part1 * 4096 + part2 * 256 + part3 * 16 + part4;
            result.append(1, target);
            lastCharIsUnicode = true;
        }
        else
        {
            result.append(1, strRaw[i]);
            lastCharIsUnicode = false;
        }
    }
    assert(result.length() > 0);
    return result;
}

bool RequestAccessRight(const char* ak, const char* sk)
{
    WinHttpClient postClient(L"https://nlp-test.xue.lenovo.com.cn/nlp-tools");

    // Post data.
    wchar_t* teststr = L"总有人间一两风";
    char* testchar = (char*)teststr;
    wchar_t* content = L"{ \"type\":\"fine\",\"sentence\" : \"总有人间一两风，填我人十万八千梦\" }";

    string data = wchar2utf8(content);
    postClient.SetAdditionalDataToSend((BYTE*)data.c_str(), data.size());

    // Post headers.
    wstring headers;
    headers.append(L"Content-Type: application/json; charset=UTF-8\r\n");
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
    ::OutputDebugStringW(httpResponseContent.c_str());
    
    wstring newResponse = hexstring2whar(httpResponseContent);
    ::OutputDebugStringW(newResponse.c_str());

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
