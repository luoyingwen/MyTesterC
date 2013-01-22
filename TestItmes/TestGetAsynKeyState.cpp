#include "stdafx.h"
#include "Common.h"

static DWORD WINAPI MonitorProc(_In_ void* pv) throw()
{
	while(1)
	{
		::Sleep(60);
		SHORT state = ::GetAsyncKeyState(VK_LWIN);
		LogMessage(L"state=%x", state);
	}
	return 0;
}

void TestGetAsyncKeyState(const TCHAR* szParam)
{
	DWORD dwThreadID;
	HANDLE hThread = ::CreateThread(NULL, 0, MonitorProc, NULL, 0, &dwThreadID);
	if (hThread == NULL)
	{
		TheLogger.Error(L"CreateThread failed. Error=%d", ::GetLastError());			
	}
	else
	{
		::CloseHandle(hThread);
	}

	LogStringMessage(L"TestCreateWindow");
}

COMMAND_DEFINE(Test, TestGetAsyncKeyState);