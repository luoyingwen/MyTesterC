#include "stdafx.h"
#include "Common.h"
#include <Shellapi.h>

static HANDLE g_hProcess = NULL;

static void LaunchTabTipProcess()
{
	TheLogger.Debug(L"Enter LaunchTabTipProcess");
	wchar_t szTabTipPath[MAX_PATH];
	GetWindowsDirectory(szTabTipPath, MAX_PATH);
	szTabTipPath[3] = '\0';
	wcscat_s(szTabTipPath, L"Program Files\\Common Files\\microsoft shared\\ink\\TabTip.exe");

	DWORD fileAttr = ::GetFileAttributes(szTabTipPath);
	if (fileAttr == INVALID_FILE_ATTRIBUTES)
	{
		TheLogger.Error(L"Can't find %s, LastError=%d", szTabTipPath, ::GetLastError());
		return;
	}

	SHELLEXECUTEINFO ShExecInfo;
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = szTabTipPath;
	ShExecInfo.lpParameters = L"-LaunchInkWatson -ManualLaunch";
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_MAXIMIZE;
	ShExecInfo.hInstApp = NULL;

	if (!ShellExecuteEx(&ShExecInfo))
	{
		ATLASSERT(false);
		TheLogger.Error(L"CreateProcess return failed. Path=%s Error=%d", szTabTipPath, ::GetLastError());
	}
	g_hProcess = ShExecInfo.hProcess;
	TheLogger.Debug(L"g_hProcess=%d", g_hProcess);
	TheLogger.Debug(L"Leave LaunchTabTipProcess");
}		

void TestHideSoftkeyboard(const TCHAR* szParam)
{
	if (g_hProcess != NULL)
	{
		::TerminateProcess(g_hProcess, 2);
		g_hProcess = NULL;
	}
}

void TestShowSoftkeyboard(const TCHAR* szParam)
{
	TestHideSoftkeyboard(NULL);
	LaunchTabTipProcess();
}


COMMAND_DEFINE(Test, TestShowSoftkeyboard);
COMMAND_DEFINE(Test, TestHideSoftkeyboard);