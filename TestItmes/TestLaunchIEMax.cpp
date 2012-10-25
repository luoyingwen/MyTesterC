#include "stdafx.h"
#include "Common.h"
#include "../RegKeyUtil.h"
#include <Shellapi.h>

class SoftKeyboardMgr
{
private:
	static void SignalEvent(const TCHAR* szEventName)
	{
		HANDLE hEvent = ::OpenEvent(EVENT_MODIFY_STATE, TRUE, szEventName);
		if (hEvent != NULL)
		{
			::SetEvent(hEvent);
			::CloseHandle(hEvent);
		}
	}
public:
	static void ShowButton()
	{
		SignalEvent(_T("ShowSoftKeyboardEvent"));
	}

	static void HideButton()
	{
		SignalEvent(_T("HideSoftKeyboardEvent"));
	}
};

static void SetIExploreMax()
{
	const BYTE magicMaxBin[] = {0x2c,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xd7,0x01,0x00,0x00,0xa4,0x00,0x00,0x00,0x87,0x06,0x00,0x00,0x47,0x03,0x00,0x00};
	RegKeyUtil::SetBinaryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Internet Explorer\\Main", L"Window_Placement", magicMaxBin, sizeof(magicMaxBin));
}

void LaunchProcesIEAndWaitForExit()
{
	TheLogger.Debug(L"Enter LaunchProcesAndWaitForExit");

	STARTUPINFO startInfo;
	ZeroMemory(&startInfo, sizeof(startInfo));
	startInfo.cb = sizeof(startInfo);
	startInfo.wShowWindow = SW_SHOWMAXIMIZED;

	PROCESS_INFORMATION processInfo;
	ZeroMemory(&processInfo, sizeof(processInfo));
	TheLogger.Debug(L"Create a named event.");
	
	wchar_t szProcessPath[MAX_PATH];
	wcscpy(szProcessPath, L"\"C:\\Program Files\\Internet Explorer\\iexplore.exe\" -noframemerging");

	TheLogger.Debug(L"CreateProcess: Excutable file: %s", szProcessPath);
	if (::CreateProcess(NULL, szProcessPath, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &startInfo, &processInfo))
	{
		::WaitForSingleObject(processInfo.hProcess, INFINITE);
		::TerminateProcess(processInfo.hProcess, 2);

		::CloseHandle(processInfo.hProcess);
		::CloseHandle(processInfo.hThread);
	}
	else
	{
		TheLogger.Error(L"CreateProcess return failed. Path=%s Error=%d", szProcessPath, ::GetLastError());
		ATLASSERT(false);
	}
	TheLogger.Debug(L"Leave LaunchProcesAndWaitForExit");
}

static bool HideTaskBar()
{
	TheLogger.Debug(L"HideTaskBar");
	APPBARDATA  apBar;  
	memset(&apBar,0,sizeof(apBar));  
	apBar.cbSize  =  sizeof(apBar);  
	HWND hTrayWnd  =  FindWindow(L"Shell_TrayWnd", NULL);
	UINT uState = (UINT)SHAppBarMessage(ABM_GETSTATE, &apBar);
	if (hTrayWnd != NULL && uState != ABS_AUTOHIDE)
	{
		memset(&apBar,0,sizeof(apBar));
		apBar.cbSize  =  sizeof(apBar);
		apBar.hWnd  =  hTrayWnd;
		apBar.lParam = ABS_AUTOHIDE;
		TheLogger.Debug(L"Before SHAppBarMessage");
		SHAppBarMessage(ABM_SETSTATE,&apBar);
		TheLogger.Debug(L"End SHAppBarMessage");
		return true;
	}
	TheLogger.Debug(L"HideTaskBar uState = %d hTrayWnd=%d", uState, hTrayWnd);
	return false;
}

static void ShowTaskBar()
{
	TheLogger.Debug(L"ShowTaskBar");
	HWND hTrayWnd  =  FindWindow(L"Shell_TrayWnd", NULL);
	if (hTrayWnd != NULL)
	{
		APPBARDATA  apBar;  
		memset(&apBar,0,sizeof(apBar));
		apBar.cbSize  =  sizeof(apBar);
		apBar.hWnd  =  hTrayWnd;
		apBar.lParam = ABS_ALWAYSONTOP;
		TheLogger.Debug(L"Before SHAppBarMessage");
		SHAppBarMessage(ABM_SETSTATE,&apBar);
		TheLogger.Debug(L"End SHAppBarMessage");
	}
	else
	{
		TheLogger.Error(L"FindWindow Shell_TrayWnd return NULL");
	}
}

static void SetThreadDesktopToDefault()
{
	wchar_t szDefault[MAX_PATH];
	wcscpy_s(szDefault, L"Default");
	HDESK hDefault = ::OpenDesktop(szDefault, 0, TRUE, GENERIC_ALL);
	TheLogger.Debug(L"OpenDesktop return hDefault = %x", hDefault);
	if (hDefault != NULL)
	{
		ATLASSERT(hDefault);
		if(::SetThreadDesktop(hDefault))
		{
			TheLogger.Debug(L"SetThreadDesktop return successfully");
		}
		else
		{
			TheLogger.Error(L"SetThreadDesktop return failed. Error=%d", ::GetLastError());
			ATLASSERT(false);
		}
	}
	else
	{
		TheLogger.Error(L"OpenDesktop return failed. Error= %d", ::GetLastError());
	}
}

static DWORD WINAPI LaunchProc(_In_ void* pv) throw()
{
	SetThreadDesktopToDefault();
	if (pv != NULL)
	{
		HideTaskBar();
	}
	else
	{
		ShowTaskBar();
	}
	return 0;
}

void TestHideTaskbar(const TCHAR* szParam)
{
	DWORD dwThreadID;
	HANDLE hThread = ::CreateThread(NULL, 0, LaunchProc, TestHideTaskbar, 0, &dwThreadID);
	if (hThread == NULL)
	{
		TheLogger.Error(L"CreateThread failed. Error=%d", ::GetLastError());			
		ATLASSERT(false);
	}
	else
	{
		::CloseHandle(hThread);
	}
	::Sleep(1*1000);
}

void TestShowTaskbar(const TCHAR* szParam)
{
	DWORD dwThreadID;
	HANDLE hThread = ::CreateThread(NULL, 0, LaunchProc, NULL, 0, &dwThreadID);
	if (hThread == NULL)
	{
		TheLogger.Error(L"CreateThread failed. Error=%d", ::GetLastError());			
		ATLASSERT(false);
	}
	else
	{
		::CloseHandle(hThread);
	}
	::Sleep(1*1000);
}

void TestGetAutoHideBar(const TCHAR* szParam)
{
	APPBARDATA  apBar;  
	memset(&apBar,0,sizeof(apBar));  
	apBar.cbSize  =  sizeof(apBar);  
	UINT uState = (UINT)SHAppBarMessage(ABM_GETSTATE, &apBar);  //设置任务栏自动隐藏
	TheLogger.Debug(L"uState=%x", uState);
	if (uState == ABS_AUTOHIDE)
	{
		TheLogger.Debug(L"autoHide");
	}
}

void TestLaunchIEMax(const TCHAR* szParam)
{
	LogStringMessage(__WFILE__);
	SoftKeyboardMgr::ShowButton();
	TestHideTaskbar(NULL);
	SetIExploreMax();
	LaunchProcesIEAndWaitForExit();
	TestShowTaskbar(NULL);
	SoftKeyboardMgr::HideButton();
}

void TestShowSoftKeyboard(const TCHAR* szParam)
{
	LogStringMessage(__WFILE__);
	SoftKeyboardMgr::ShowButton();
}
void TestHideSoftKeyboard(const TCHAR* szParam)
{
	LogStringMessage(__WFILE__);
	SoftKeyboardMgr::HideButton();
}
COMMAND_DEFINE(Test, TestHideTaskbar);
COMMAND_DEFINE(Test, TestShowTaskbar);
COMMAND_DEFINE(Test, TestGetAutoHideBar);
COMMAND_DEFINE(Test, TestLaunchIEMax);
COMMAND_DEFINE(Test, TestShowSoftKeyboard);
COMMAND_DEFINE(Test, TestHideSoftKeyboard);