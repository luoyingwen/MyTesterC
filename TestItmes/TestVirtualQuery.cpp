#include "stdafx.h"
#include "Common.h"
#include <Wtsapi32.h>
#pragma comment(lib, "Wtsapi32.lib")

#include <Userenv.h>
#pragma comment(lib, "Userenv.lib")

void TestVirtualQuery(const TCHAR* szParam)
{
	MEMORY_BASIC_INFORMATION mbi;
    DWORD        dwRetVal;
    HMODULE        hMod = NULL;
	wchar_t szProcessFullPath[MAX_PATH];
	
    dwRetVal = (DWORD)VirtualQuery(TestVirtualQuery, &mbi, sizeof(mbi));
    if(dwRetVal != 0)
        hMod = (HMODULE) mbi.AllocationBase;
	LogMessage(L"hMod = %x", hMod);
	GetModuleFileName(hMod, szProcessFullPath, MAX_PATH);
	PathRemoveFileSpec(szProcessFullPath);
	LogStringMessage(szProcessFullPath);

	PWTS_SESSION_INFO pSessionInfo = NULL;
	DWORD dwCount = 0;
	BOOL bSuccess = WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &dwCount);
	if (bSuccess)
	{
		LogMessage(L"WTSEnumerateSessions return successfully, dwCount = %d", dwCount);
		for (DWORD index = 0; index < dwCount; index++)
		{
			LogMessage(L"sessionid=%d, name=%s, state=%d", pSessionInfo[index].SessionId, pSessionInfo[index].pWinStationName, pSessionInfo[index].State);
			if (pSessionInfo[index].State == WTSActive)
			{
				LogMessage(L"this sesseion is active");
			}
		}
	}
	else
	{
		LogMessage(L"WTSEnumerateSessions return failed");
	}
}

void TestCreateActiveUserApp(const TCHAR* szParam)
{
	BOOL bSuccess = FALSE;
	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi = {0};
	si.cb = sizeof(si);
	HANDLE hToken = NULL;
	HANDLE hDuplicatedToken = NULL;
	LPVOID lpEnvironment = NULL;
	
	DWORD dwSessionID = WTSGetActiveConsoleSessionId();
	while (dwSessionID == 0 || dwSessionID == 0xFFFFFFFF)
	{
		::Sleep(500);
	}
	if (WTSQueryUserToken(dwSessionID, &hToken) == FALSE)
	{
		goto Cleanup;
	}
	
	if (DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &hDuplicatedToken) == FALSE)
	{
		goto Cleanup;
	}

	if (CreateEnvironmentBlock(&lpEnvironment, hDuplicatedToken, FALSE) == FALSE)
	{
		goto Cleanup;
	}

	WCHAR lpszClientPath[MAX_PATH];
	if (GetModuleFileName(NULL, lpszClientPath, MAX_PATH) == 0)
	{
		goto Cleanup;
	}
	PathRemoveFileSpec(lpszClientPath);
	wcscat_s(lpszClientPath, sizeof(lpszClientPath)/sizeof(WCHAR), L"\\TimeServiceClient.exe");

	if (CreateProcessAsUser(hDuplicatedToken, L"c:\\Windows\\System32\\notepad.exe", NULL, NULL, NULL, FALSE,
		NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,
		lpEnvironment, NULL, &si, &pi) == FALSE)
	{
		goto Cleanup;
	}
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	bSuccess = TRUE;

	Cleanup:
	if (!bSuccess)
	{
		LogMessage(L"An error occurred while creating fancy client UI");
	}
	if (hToken != NULL)
		CloseHandle(hToken);
	if (hDuplicatedToken != NULL)
		CloseHandle(hDuplicatedToken);
	if (lpEnvironment != NULL)
		DestroyEnvironmentBlock(lpEnvironment);
}