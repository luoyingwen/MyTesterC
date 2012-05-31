#include "stdafx.h"
#include "Common.h"

#include <windows.h> 
#include <Dbt.h>
 
UINT WM_SHELLHOOKMESSAGE = 0;
const UINT WM_HOOK_MSG = WM_APP + 0x40;
static bool g_sendHookMsg = false;
HANDLE s_hThread = NULL;
const DWORD CHECK_TIME_INTERVAL = 10*1000;

void ShowWindowInfo(HWND hWnd)
{
	if (hWnd == NULL)
	{
		LogStringMessage(L"ShowWindowInfo hWnd = NULL");
		return;
	}

	TCHAR szClassName[MAX_PATH];
	RealGetWindowClass(hWnd, szClassName, MAX_PATH);
	LogStringMessage(szClassName);

	DWORD dwProcessId = 0;
	DWORD dwThreadId = :: GetWindowThreadProcessId(hWnd, &dwProcessId);
	LogMessage(L"hWnd=%d, threadId=%d, processId=%d", dwThreadId, dwProcessId);
}

bool IsMetroMainUi(HWND hWnd)
{
	if (hWnd == NULL)
		return false;

	TCHAR szClassName[MAX_PATH];
	RealGetWindowClass(hWnd, szClassName, MAX_PATH);
	return _wcsicmp(szClassName, L"ImmersiveLauncher") == 0;
}

VOID SwitchToCurrentApp()
{
	g_sendHookMsg = false;
	HWND hWnd = ::GetForegroundWindow();
	if (hWnd == NULL)
		return;

	if (hWnd == g_hwndMainDlg)
	{
		LogMessage(L"I'm foreground Window");
		return;
	}

	LogMessage(L"\nLog Foreground:");
	ShowWindowInfo(hWnd);
	if (IsMetroMainUi(hWnd))
	{
		LogMessage(L"Switch now");
		::PostMessage(GetShellWindow(), WM_HOOK_MSG, (WPARAM)g_hwndMainDlg, 0);
	}
	else
	{
		 INPUT winKeys[2];
		 winKeys[0].type = INPUT_KEYBOARD;
		 winKeys[1].type = INPUT_KEYBOARD;
		 winKeys[0].ki.wVk = VK_LWIN;
		 winKeys[1].ki.wVk = VK_LWIN;
		 winKeys[0].ki.dwExtraInfo = 0;
		 winKeys[1].ki.dwExtraInfo = 0;
		 
		 winKeys[0].ki.time = 0;
		 winKeys[1].ki.time = 0;
	
		 winKeys[0].ki.wScan = 0;
		 winKeys[1].ki.wScan = 0;

		 winKeys[0].ki.dwFlags = 0;
		 winKeys[1].ki.dwFlags = KEYEVENTF_KEYUP;
		 g_sendHookMsg = true;
		 LogMessage(L"Post Win key");
		::SendInput(2, winKeys, sizeof(INPUT));
	}
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_SHELLHOOKMESSAGE)
	{
		if (wParam == HSHELL_WINDOWACTIVATED)
		{
			LogMessage(L"HSHELL_WINDOWACTIVATED");
			ShowWindowInfo((HWND)lParam);
			//if ( IsMetroMainUi((HWND)lParam))
			//{
			//	::PostMessage(GetShellWindow(), WM_HOOK_MSG, (WPARAM)g_hwndMainDlg, 0);
			//}
		}
		else if (wParam == HSHELL_RUDEAPPACTIVATED)
		{
			LogMessage(L"HSHELL_RUDEAPPACTIVATED");
			ShowWindowInfo((HWND)lParam);
			if ( IsMetroMainUi((HWND)lParam) && g_sendHookMsg)
			{
				g_sendHookMsg = false;
				LogMessage(L"Switch now");
				::PostMessage(GetShellWindow(), WM_HOOK_MSG, (WPARAM)g_hwndMainDlg, 0);
			}
		}
		else if (wParam == HSHELL_WINDOWREPLACING)
		{
			LogMessage(L"HSHELL_WINDOWREPLACING");
			ShowWindowInfo((HWND)lParam);
		}
		else if (wParam == HSHELL_WINDOWREPLACED)
		{
			LogMessage(L"HSHELL_WINDOWREPLACED");
			ShowWindowInfo((HWND)lParam);
		}
		return (INT_PTR)FALSE;
	}
	else if (message == WM_DEVICECHANGE)
	{
		LogMessage(L"WM_DEVICECHANGE");
		return (INT_PTR)FALSE;
	}
	else
	{
		return DefWindowProc(hWnd,message,wParam,lParam);
	}
}

 static HMODULE ModuleFromAddress(PVOID pv) 
{
	MEMORY_BASIC_INFORMATION mbi;
	return ((::VirtualQuery(pv, &mbi, sizeof(mbi)) != 0) 
	        ? (HMODULE) mbi.AllocationBase : NULL);
}
DWORD WINAPI MyThreadFunction( LPVOID lpParam ) 
{ 
    while (true)
	{
		::Sleep(CHECK_TIME_INTERVAL);
		SwitchToCurrentApp();
	}
}
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE, 0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, \
             0xC0, 0x4F, 0xB9, 0x51, 0xED);

void TestReceiveShellHookMsg(const TCHAR* szParam)
{
	if (s_hThread != NULL)
	{
		LogMessage(L"thread already is running");
		return;
	}
	WNDCLASS wc      = {0}; 
	wc.lpfnWndProc   = MainWndProc;
	wc.hInstance     = ModuleFromAddress(TestReceiveShellHookMsg);
	wc.lpszClassName = L"TestReceiveWndClass";
	
	RegisterClass(&wc);
	HWND hWnd = CreateWindow(wc.lpszClassName,NULL,0,0,0,0,0,HWND_MESSAGE,NULL,wc.hInstance,NULL);
	if (!hWnd) 
	{
		LogMessage(L"last error=%d", ::GetLastError());
        return; 
	}

	WM_SHELLHOOKMESSAGE = RegisterWindowMessage(_T("SHELLHOOK"));
	if (WM_SHELLHOOKMESSAGE == 0)
	{
		LogMessage(L"RegisterWindowMessage return faled. LastError=%d", ::GetLastError());
	}
	else
	{
		if (!RegisterShellHookWindow(hWnd))
		{
			LogMessage(L"RegisterShellHookWindow return faled. LastError=%d", ::GetLastError());
		}
	}

	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
	ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));

	NotificationFilter.dbcc_size = sizeof(NotificationFilter);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_reserved = 0;

	//NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;
	CLSIDFromString(L"{A5DCBF10-6530-11D2-901F-00C04FB951ED}", &NotificationFilter.dbcc_classguid);

	HDEVNOTIFY hDevNotify = RegisterDeviceNotification(hWnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);

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