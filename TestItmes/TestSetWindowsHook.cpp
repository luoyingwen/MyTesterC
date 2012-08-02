#include "stdafx.h"
#include "Common.h"

typedef int (WINAPI *PFN_INSTALLHOOK)(HWND);
typedef int (WINAPI *PFN_UNINSTALLHOOK)();
typedef int (WINAPI *PFN_INSTALLAPPHOOK)(DWORD);

HMODULE s_hmodHookTool = NULL;

const wchar_t* HOOKDLLPATH = L"D:\\luoyw\\work\\source\\Aries\\Alpha\\Src\\WinIntegration\\x64\\Debug\\WinIntegration.dll";

void TestInstallHook(const TCHAR* szParam)
{
	if (NULL == s_hmodHookTool)
	{
		s_hmodHookTool = ::LoadLibrary(HOOKDLLPATH);
		if (NULL != s_hmodHookTool)
		{
			PFN_INSTALLHOOK s_pfnInstallHook = NULL;
			s_pfnInstallHook = (PFN_INSTALLHOOK)::GetProcAddress(
				s_hmodHookTool, 
				"InstallExplorerMsgHook"
				);
			int nRet = s_pfnInstallHook(g_hwndMainDlg);
			if (nRet == 0)
			{
				LogMessage(_T("Install failed"));
			}
			else
			{
				LogMessage(_T("Install hook successfully, g_hwndMainDlg=%x"), g_hwndMainDlg);
			}

		}
		else
		{
			LogMessage(_T("LoadLibrary Hook dll(%s) return failed."), HOOKDLLPATH);
		}
	}
}

void TestUninstallHook(const TCHAR* szParam)
{
	if (s_hmodHookTool != NULL)
	{
		PFN_UNINSTALLHOOK s_pfnUninstallHook = NULL;
		s_pfnUninstallHook = (PFN_UNINSTALLHOOK)::GetProcAddress(
			s_hmodHookTool, 
			"UninstallExplorerMsgHook"
			);
		int nRet = s_pfnUninstallHook();
		if (nRet == 0)
		{
			LogMessage(_T("Uninstall failed"));
		}
		else
		{
			LogMessage(_T("Uninstall hook successfully"));
		}
		::FreeLibrary(s_hmodHookTool);
		s_hmodHookTool = NULL;
	}
}



//
// Attempts to enable SeDebugPrivilege. This is required by use of
// CreateRemoteThread() under NT/2K
//
BOOL EnableDebugPrivilege()
{
	HANDLE           hToken;
	LUID             sedebugnameValue;
	TOKEN_PRIVILEGES tp;

	if ( !::OpenProcessToken( 
		GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | // to adjust privileges
		TOKEN_QUERY,              // to get old privileges setting
		&hToken 
		) )
	{
		LogMessage(L"OpenProcessToken (Current Process) return failed.");
		return FALSE;
	}
	//
	// Given a privilege's name SeDebugPrivilege, we should locate its local LUID mapping.
	//
	if ( !::LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &sedebugnameValue ) )
	{
		//
		// LookupPrivilegeValue() failed
		//
		LogMessage(L"LookupPrivilegeValue return failed.");
		::CloseHandle( hToken );
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = sedebugnameValue;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if ( !::AdjustTokenPrivileges( hToken, FALSE, &tp, sizeof(tp), NULL, NULL ) )
	{
		//
		// AdjustTokenPrivileges() failed
		//
		LogMessage(L"AdjustTokenPrivileges return failed.");
		::CloseHandle( hToken );
		return FALSE;
	}

	::CloseHandle( hToken );
	return TRUE;
}


void TestCreateRemoteThread(const TCHAR* szParam)
{
	HANDLE hProcess = NULL;
	PSTR pszLibFileRemote = NULL;
	HANDLE hThread = NULL;

	if (szParam == NULL || wcslen(szParam) <= 0)
	{
		LogMessage(L"Please Input ProcessId param");
		return;
	}
	EnableDebugPrivilege();

	DWORD processId = _wtoi(szParam);
	hProcess = ::OpenProcess(
		PROCESS_ALL_ACCESS, // Specifies all possible access flags
		FALSE, 
		processId
		);
	if (hProcess == NULL)
	{
		LogMessage(L"Please input valid processid");
		return;
	}

	char szLibFile[MAX_PATH];
	strcpy_s(szLibFile, "D:\\luoyw\\github\\MyTesterC\\Debug_X64\\ExplorerMsgHook.dll");
	size_t cch = 1 + strlen(szLibFile);

	pszLibFileRemote = (PSTR)::VirtualAllocEx(
					hProcess, 
					NULL, 
					cch, 
					MEM_COMMIT, 
					PAGE_READWRITE
					);
	if (pszLibFileRemote == NULL) 
	{
		LogMessage(L"VirtualAllocEx return failed");
		::CloseHandle(hProcess);
		return;
	}

			// Copy the DLL's pathname to the remote process's address space
	if (!::WriteProcessMemory(
		hProcess, 
		(PVOID)pszLibFileRemote, 
		(PVOID)szLibFile, 
		cch, 
		NULL)) 
	{
		LogMessage(L"WriteProcessMemory return failed");
		::VirtualFreeEx(hProcess, (PVOID)pszLibFileRemote, 0, MEM_RELEASE);
		::CloseHandle(hProcess);
		return;
	}
	PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)::GetProcAddress(::GetModuleHandle(L"Kernel32"), "LoadLibraryA");
	if (pfnThreadRtn == NULL) 
	{
		LogMessage(L"GetProcAddress return failed");
		::VirtualFreeEx(hProcess, (PVOID)pszLibFileRemote, 0, MEM_RELEASE);
		::CloseHandle(hProcess);
		return;
	}

	hThread = ::CreateRemoteThread(
		hProcess, 
		NULL, 
		0, 
		pfnThreadRtn, 
		(PVOID)pszLibFileRemote, 
		0, 
		NULL
		);
	if (hThread == NULL) 
	{
		LogMessage(L"CreateRemoteThread return failed");
		::VirtualFreeEx(hProcess, (PVOID)pszLibFileRemote, 0, MEM_RELEASE);
		::CloseHandle(hProcess);
		return;
	}
	// Wait for the remote thread to terminate
	::WaitForSingleObject(hThread, INFINITE);
	::VirtualFreeEx(hProcess, (PVOID)pszLibFileRemote, 0, MEM_RELEASE);
	::CloseHandle(hThread);
	::CloseHandle(hProcess);
	LogMessage(L"CreateRemoteThread return successfully.");
}
