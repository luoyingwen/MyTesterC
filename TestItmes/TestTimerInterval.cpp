#include "stdafx.h"
#include "Common.h"
#include <Shellapi.h>
#include <peninputpanel.h>
typedef VOID (WINAPI *pGetSystemTimePreciseAsFileTime)(LPFILETIME lpSystemTimeAsFileTime);

FILETIME begin;
UINT_PTR timerId;
static void GetSystemCurrentTime(LPFILETIME lpSystemTimeAsFileTime)
{
	static pGetSystemTimePreciseAsFileTime timeFunction = NULL;
	if (timeFunction == NULL)
	{
		HANDLE hDll = LoadLibrary(L"kernel32");
		FARPROC pfn = GetProcAddress((HMODULE)hDll, "GetSystemTimePreciseAsFileTime");
		if (pfn != NULL)
		{
			timeFunction = (pGetSystemTimePreciseAsFileTime)pfn;
		}
	}
	if (timeFunction != NULL)
	{
		timeFunction(lpSystemTimeAsFileTime);
	}		
}

VOID CALLBACK MyTimerProc( 
    HWND hwnd,        // handle to window for timer messages 
    UINT message,     // WM_TIMER message 
    UINT idTimer,     // timer identifier 
    DWORD dwTime)     // current system time 
{	
	FILETIME end;
	GetSystemCurrentTime(&end);

	ULARGE_INTEGER uliBegin;
	uliBegin.HighPart = begin.dwHighDateTime;
	uliBegin.LowPart = begin.dwLowDateTime;
						
	ULARGE_INTEGER uliEnd;
	uliEnd.HighPart = end.dwHighDateTime;
	uliEnd.LowPart = end.dwLowDateTime;			
		
	ULONGLONG cost = uliEnd.QuadPart - uliBegin.QuadPart;
	cost /= 10000;

	const wchar_t* ALPHA_SHELL_CAPTION = L"IPTip_Main_Window";

	DWORD WM_DESKBAND_CLICKED =
    ::RegisterWindowMessage(_TEXT("TabletInputPanelDeskBandClicked"));

	DWORD WM_DOCK_BUTTON_PRESSED = ::RegisterWindowMessage(_TEXT("IPTipDockButtonPressed"));
	HWND hWnd = ::FindWindow(ALPHA_SHELL_CAPTION, NULL);
	while (hWnd)
	{
		TheLogger.Debug(L"Fould Alpha Window hWnd=%x", hWnd);
		//::SetWindowPos(hWnd, HWND_TOPMOST, -1, -1, 100, 100, SWP_SHOWWINDOW);
		::PostMessage(hWnd, WM_DOCK_BUTTON_PRESSED, 0, 0);
		hWnd = ::FindWindowEx(NULL, hWnd, ALPHA_SHELL_CAPTION, NULL);
	}
	//::KillTimer(NULL, timerId);

	//timerId = SetTimer(NULL, 1, 1,(TIMERPROC) MyTimerProc); // timer callback 
	//GetSystemCurrentTime(&begin);

	TheLogger.Warn(L"Duration=%I64d",cost);
}

void LaunchTabTipProcess()
{
	TheLogger.Debug(L"LaunchTabTipProcess");
	wchar_t szTabTipPath[MAX_PATH];
	wcscpy_s(szTabTipPath, L"C:\\Program Files\\Common Files\\microsoft shared\\ink\\TabTip.exe");

	DWORD fileAttr = ::GetFileAttributes(szTabTipPath);
	if (fileAttr == INVALID_FILE_ATTRIBUTES)
	{
		TheLogger.Error(L"Can't find %s, LastError=%d", szTabTipPath, ::GetLastError());
		return;
	}

	SHELLEXECUTEINFO ShExecInfo;
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = NULL;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = szTabTipPath;
	ShExecInfo.lpParameters = L"/SeekDesktop";
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_MAXIMIZE;
	ShExecInfo.hInstApp = NULL;

	if (!ShellExecuteEx(&ShExecInfo))
	{
		ATLASSERT(false);
		TheLogger.Error(L"CreateProcess return failed. Path=%s Error=%d", szTabTipPath, ::GetLastError());
	}
}


void TestTimerInterval(const TCHAR* szParam)
{
	LogStringMessage(L"TestTimerInterval");

	CComQIPtr<ITextInputPanel> text_input_panel;
	HRESULT hRes = text_input_panel.CoCreateInstance(__uuidof(TextInputPanel));
	hRes = text_input_panel->SetInPlaceVisibility(TRUE);
	//timerId = SetTimer(NULL, 1, 1 * 1000,(TIMERPROC) MyTimerProc); // timer callback 
	//GetSystemCurrentTime(&begin);
	//LaunchTabTipProcess();
}

void TestStopTimer(const TCHAR* szParam)
{
	::KillTimer(NULL, timerId);
}

void TestPostTimerMsg(const TCHAR* szParam)
{
	if (szParam == NULL || _tcslen(szParam) <= 0)
	{
		TheLogger.Debug(L"Please Input Windows Handle.");
		TheLogger.Debug(L"Please Input Windows Handle.\r\n");
		return;
	}
	int handle = _ttoi(szParam);
	TheLogger.Debug(L"Handle=%d", handle);
	if(::PostMessage((HWND)handle, WM_USER + 35, 0x201, 0) == 0)
	{
		TheLogger.Error(L"PostMessage return failed. Error=%d", ::GetLastError());
	}
}