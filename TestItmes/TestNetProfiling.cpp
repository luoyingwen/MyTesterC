#include "stdafx.h"
#include "Common.h"

#define BUFSIZE 4096
static const wchar_t* PROFILER_CLSID = L"{9E2B38F2-7355-4C61-A54F-434B7AC266C0}";
static const wchar_t* PROFILER_DLL_NAME = L"WinIntegration.dll";

bool EnvStrCat(wchar_t* szEnvBuf, size_t nTotalLen, size_t& nStartPos, wchar_t* szNewVar)
{
	if (szEnvBuf == NULL || szNewVar == NULL)
	{
		TheLogger.Error(L"EnvStrCat. szEnvBuf or szNewVar parameter is invalid.");
		return false;
	}
	size_t nVarLen = wcslen(szNewVar);
	if (nStartPos + nVarLen > nTotalLen)
	{
		TheLogger.Error(L"EnvStrCat. EnvBuf is too small.");
		return false;
	}

	wcscpy(szEnvBuf + nStartPos, szNewVar);
	nStartPos += nVarLen + 1;
	return true;
}

void TestNetProfiling(const TCHAR* szParam)
{
	LogStringMessage(L"TestNetProfiling.");
	wchar_t szEnvBuf[BUFSIZE];

	LPTCH lpvEnv;
	LPTCH lpszVariable;
 
    // Get a pointer to the environment block. 
 
    lpvEnv = GetEnvironmentStrings();

    // If the returned pointer is NULL, exit.
    if (lpvEnv == NULL)
    {
        TheLogger.Error(L"GetEnvironmentStrings failed (%d)\n", GetLastError()); 
        return;
    }
 
    // Variable strings are separated by NULL byte, and the block is 
    // terminated by a NULL byte. 

    lpszVariable = lpvEnv;

	size_t nStartPos = 0;
    while (*lpszVariable)
    {
		TheLogger.Debug(L"env variable = %s", lpszVariable);
        EnvStrCat(szEnvBuf, sizeof(szEnvBuf) / sizeof(szEnvBuf[0]), nStartPos, lpszVariable);
        lpszVariable += lstrlen(lpszVariable) + 1;
    }
    FreeEnvironmentStrings(lpvEnv);

	//setup profiler environment variable. TODO:already have these variables.
	//1. set COR_ENABLE_PROFILING=1
	EnvStrCat(szEnvBuf, sizeof(szEnvBuf) / sizeof(szEnvBuf[0]), nStartPos, L"COR_ENABLE_PROFILING=1");
	wchar_t szBuf[MAX_PATH];
	//2. set COR_PROFILER={CLSID of profiler}
	swprintf(szBuf, MAX_PATH, L"COR_PROFILER=%s", PROFILER_CLSID);
	EnvStrCat(szEnvBuf, sizeof(szEnvBuf) / sizeof(szEnvBuf[0]), nStartPos, szBuf);
	//3. set COR_PROFILER_PATH=full path of the profiler DLL
	wchar_t szProfileDllPath[MAX_PATH];
	::GetModuleFileName(NULL, szProfileDllPath, sizeof(szProfileDllPath)/sizeof(szProfileDllPath[0]));
	::PathRemoveFileSpec(szProfileDllPath);
	::PathAppend(szProfileDllPath, PROFILER_DLL_NAME);
	swprintf(szBuf, MAX_PATH, L"COR_PROFILER_PATH=%s", szProfileDllPath);
	EnvStrCat(szEnvBuf, sizeof(szEnvBuf) / sizeof(szEnvBuf[0]), nStartPos, szBuf);

	lpszVariable = szEnvBuf;
    while (*lpszVariable)
    {
		TheLogger.Debug(L"env variable = %s", lpszVariable);
        lpszVariable += lstrlen(lpszVariable) + 1;
    }
}
