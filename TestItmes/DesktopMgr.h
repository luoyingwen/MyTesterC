#pragma once
#include "stdafx.h"

class DesktopMgr
{
private:
	static const ACCESS_MASK AccessRights = DESKTOP_JOURNALRECORD | DESKTOP_JOURNALPLAYBACK | DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE | DESKTOP_WRITEOBJECTS | DESKTOP_SWITCHDESKTOP | DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL | DESKTOP_READOBJECTS;
public:
	static void LaunchProcess(const wchar_t* szDesktopName, const wchar_t* szProcessPath)
	{
		HDESK hCurrentInput = ::OpenInputDesktop(0, TRUE, AccessRights);
		if (hCurrentInput != NULL)
		{
			HDESK hNewDesktop = ::CreateDesktop(szDesktopName, NULL, NULL, 0, AccessRights, NULL);
			if (hNewDesktop != NULL)
			{
				STARTUPINFO startInfo;
				ZeroMemory(&startInfo, sizeof(startInfo));
				startInfo.cb = sizeof(startInfo);
				startInfo.lpDesktop = (wchar_t*)szDesktopName;

				PROCESS_INFORMATION processInfo;
				ZeroMemory(&processInfo, sizeof(processInfo));
				if (::CreateProcess(NULL, (wchar_t*)szProcessPath, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &startInfo, &processInfo))
				{
					::Sleep(2000);
					if (::SwitchDesktop(hNewDesktop))
					{
						::WaitForSingleObject( processInfo.hProcess, INFINITE );
						::SwitchDesktop(hCurrentInput);
					}
					else
					{
						//Kill process.
						TheLogger.Error(L"SwitchDesktop return failed. Error=%d", ::GetLastError());
						::TerminateProcess(processInfo.hProcess, 2);
						::WaitForSingleObject( processInfo.hProcess, INFINITE );
					}

				    ::CloseHandle(processInfo.hProcess);
					::CloseHandle(processInfo.hThread);
				}
				else
				{
					TheLogger.Error(L"CreateProcess return failed. Path=%s Error=%d", szProcessPath, ::GetLastError());
				}
				::CloseDesktop(hNewDesktop);
			}
			else
			{
				TheLogger.Error(L"CreateDesktop return failed. Error=%d", ::GetLastError());
			}
			::CloseDesktop(hCurrentInput);
		}
		else
		{
			TheLogger.Error(L"OpenInputDesktop return failed. Error=%d", ::GetLastError());
		}
	}
};