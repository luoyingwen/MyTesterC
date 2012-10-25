#include "stdafx.h"
#include "Common.h"
#include <Shellapi.h>

void TestRunAsCommonUser(const TCHAR* szParam)
{
	HANDLE hShellProcess = NULL, hShellProcessToken = NULL, hPrimaryToken = NULL;
	HWND hwnd = NULL;
	DWORD dwPID = 0;
	BOOL ret;
	DWORD dwLastErr;
	if (IsUserAnAdmin())
	{
		TheLogger.Debug(L"User is Admins");
	}
	else
	{
		TheLogger.Debug(L"User isn't Admins");
	}

		// Enable SeIncreaseQuotaPrivilege in this process.  (This won't work if current process is not elevated.)
	HANDLE hProcessToken = NULL;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hProcessToken))
	{
		dwLastErr = GetLastError();
		TheLogger.Error(L"OpenProcessToken failed:  ");
		return;
	}
	else
	{
		TOKEN_PRIVILEGES tkp;
		tkp.PrivilegeCount = 1;
		LookupPrivilegeValueW(NULL, SE_INCREASE_QUOTA_NAME, &tkp.Privileges[0].Luid);
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges(hProcessToken, FALSE, &tkp, 0, NULL, NULL);
		dwLastErr = GetLastError();
		CloseHandle(hProcessToken);
		if (ERROR_SUCCESS != dwLastErr)
		{
			TheLogger.Error(L"AdjustTokenPrivileges failed:  ");
			return;
		}
	}

	hwnd = GetShellWindow();
	if (NULL == hwnd)
	{
		TheLogger.Error(L"No desktop shell is present");
		return;
	}

	// Get the PID of the desktop shell process.
	GetWindowThreadProcessId(hwnd, &dwPID);
	if (0 == dwPID)
	{
		TheLogger.Error(L"Unable to get PID of desktop shell.");
		return;
	}

	// Open the desktop shell process in order to query it (get the token)
	hShellProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwPID);
	if (!hShellProcess)
	{
		dwLastErr = GetLastError();
		TheLogger.Error(L"Can't open desktop shell process:  ");
		return;
	}

	// From this point down, we have handles to close, so make sure to clean up.

	bool retval = false;
	// Get the process token of the desktop shell.
	ret = OpenProcessToken(hShellProcess, TOKEN_DUPLICATE, &hShellProcessToken);
	if (!ret)
	{
		dwLastErr = GetLastError();
		TheLogger.Error(L"Can't get process token of desktop shell:  ");
		goto cleanup;
	}

	// Duplicate the shell's process token to get a primary token.
	// Based on experimentation, this is the minimal set of rights required for CreateProcessWithTokenW (contrary to current documentation).
	const DWORD dwTokenRights = TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_SESSIONID;
	ret = DuplicateTokenEx(hShellProcessToken, dwTokenRights, NULL, SecurityImpersonation, TokenPrimary, &hPrimaryToken);
	if (!ret)
	{
		dwLastErr = GetLastError();
		TheLogger.Error(L"Can't get primary token:  ");
		goto cleanup;
	}

	wchar_t szHostProcess[MAX_PATH];
	::GetModuleFileName(NULL, szHostProcess, sizeof(szHostProcess)/sizeof(szHostProcess[0]));

	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	SecureZeroMemory(&si, sizeof(si));
	SecureZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);
	ret = CreateProcessWithTokenW(
		hPrimaryToken,
		0,
		szHostProcess,
		L"/autorun",
		0,
		NULL,
		NULL,
		&si,
		&pi);
	if (!ret)
	{
		dwLastErr = GetLastError();
		TheLogger.Error(L"CreateProcessWithTokenW failed:  ");
		goto cleanup;
	}
	else
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	retval = true;

cleanup:
	// Clean up resources
	CloseHandle(hShellProcessToken);
	CloseHandle(hPrimaryToken);
	CloseHandle(hShellProcess);
}
COMMAND_DEFINE(Test, TestRunAsCommonUser);