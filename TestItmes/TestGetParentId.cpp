#include "stdafx.h"
#include "Common.h"
#include <Winternl.h>

// NtQueryInformationProcess in NTDLL.DLL
typedef NTSTATUS (NTAPI *pfnNtQueryInformationProcess)(
	IN	HANDLE ProcessHandle,
    IN	PROCESSINFOCLASS ProcessInformationClass,
    OUT	PVOID ProcessInformation,
    IN	ULONG ProcessInformationLength,
    OUT	PULONG ReturnLength	OPTIONAL
    );

pfnNtQueryInformationProcess gNtQueryInformationProcess;

void TestGetParentId(const TCHAR* szParam)
{
	PROCESS_BASIC_INFORMATION pbi;
	ULONG nRetLen = 0;

	HMODULE hNtDll = LoadLibrary(_T("ntdll.dll"));
	if(hNtDll == NULL) return;

	gNtQueryInformationProcess = (pfnNtQueryInformationProcess)GetProcAddress(hNtDll, "NtQueryInformationProcess");
	if(gNtQueryInformationProcess == NULL) {
		FreeLibrary(hNtDll);
		return ;
	}

	gNtQueryInformationProcess(::GetCurrentProcess(), ProcessBasicInformation, (void*)&pbi, sizeof(pbi), &nRetLen);
	LogMessage(L"Parent ProcessId=%d, CurrentProcessId=%d", pbi.Reserved3, GetCurrentProcessId());

	FreeLibrary(hNtDll);
}
