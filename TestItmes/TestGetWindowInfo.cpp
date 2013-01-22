#include "stdafx.h"
#include "Common.h"

#include <windows.h> 
 
static HANDLE s_hThread = NULL;
const DWORD CHECK_TIME_INTERVAL = 10*1000;
const wchar_t* NAVIGATION_CLASS_NAME = L"NativeHWNDHost";
const wchar_t* DATE_TIME_CLASS_NAME = L"DirectUIHWND";
const wchar_t* NAVIGATION_WINDOW_TEXT = L"Charm Bar";
const wchar_t* DATE_TIME_WINDOW_TEXT = L"";
const wchar_t* METRO_UI_CLASS_NAME = L"ImmersiveLauncher";

static void ShowWindowInfo(HWND hWnd)
{
	if (hWnd == NULL)
	{
		LogStringMessage(L"ShowWindowInfo hWnd = NULL");
		return;
	}

	TCHAR szClassName[MAX_PATH];
	RealGetWindowClass(hWnd, szClassName, MAX_PATH);
	LogStringMessage(szClassName);

	GetWindowText(hWnd, szClassName, MAX_PATH);
	LogStringMessage(szClassName);

	DWORD dwProcessId = 0;
	DWORD dwThreadId = :: GetWindowThreadProcessId(hWnd, &dwProcessId);
	LogMessage(L"hWnd=%x, threadId=%x, processId=%x", hWnd, dwThreadId, dwProcessId);
}


static VOID ShowWndInfo()
{
	POINT pt;
	pt.x = 1900;
	pt.x = 1060;
	HWND hWnd = ::GetForegroundWindow();
	if (hWnd == NULL)
	{
		LogMessage(L"Not Found window");
		//return;
	}	
	ShowWindowInfo(hWnd);
}

static DWORD WINAPI MyThreadFunction( LPVOID lpParam ) 
{ 
    while (true)
	{
		::Sleep(CHECK_TIME_INTERVAL);
			//HWND hwnd = FindWindow(DATE_TIME_CLASS_NAME,DATE_TIME_WINDOW_TEXT);
	ShowWndInfo();
	}
}

void TestGetWindowInfo(const TCHAR* szParam)
{
	if (s_hThread != NULL)
	{
		LogMessage(L"thread already is running");
		return;
	}

	DWORD dwThreadId = 0;
	s_hThread = CreateThread( 
            NULL,                   // default security attributes
            0,                      // use default stack size  
            MyThreadFunction,       // thread function name
            NULL,          // argument to thread function 
            0,                      // use default creation flags 
            &dwThreadId);   // returns the thread identifier 
	if (s_hThread == NULL)
	{
		LogMessage(L"CreateThread return failed. error=%d", ::GetLastError());
	}

	LogStringMessage(L"Hello world.");
}



void TestHideNavigationWnd(const TCHAR* szParam)
{
	HWND hwnd = FindWindow(DATE_TIME_CLASS_NAME,DATE_TIME_WINDOW_TEXT);
	ShowWindowInfo(hwnd);
}

COMMAND_DEFINE(Test, TestGetWindowInfo);