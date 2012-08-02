#include "stdafx.h"
#include "Common.h"
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

	::KillTimer(NULL, timerId);

	timerId = SetTimer(NULL, 1, 1,(TIMERPROC) MyTimerProc); // timer callback 
	GetSystemCurrentTime(&begin);

	TheLogger.Warn(L"Duration=%I64d",cost);
}

void TestTimerInterval(const TCHAR* szParam)
{
	LogStringMessage(L"TestTimerInterval");

	timerId = SetTimer(NULL, 1, 1,(TIMERPROC) MyTimerProc); // timer callback 
	GetSystemCurrentTime(&begin);
}

void TestStopTimer(const TCHAR* szParam)
{
	::KillTimer(NULL, timerId);
}

void TestPostTimerMsg(const TCHAR* szParam)
{
	if (szParam == NULL || _tcslen(szParam) <= 0)
	{
		TheLogger.SetAutoNewLineForLog(false);
		TheLogger.Debug(L"Please Input Windows Handle.");
		TheLogger.Debug(L"Please Input Windows Handle.\r\n");
		return;
	}
	int handle = _ttoi(szParam);
	TheLogger.Debug(L"Handle=%d", handle);
	if(::PostMessage((HWND)handle, WM_TIMER, 2, 0) == 0)
	{
		TheLogger.Error(L"PostMessage return failed. Error=%d", ::GetLastError());
	}
}