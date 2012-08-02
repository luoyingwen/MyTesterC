#include "stdafx.h"
#include "Common.h"

typedef enum tagPOINTER_INPUT_TYPE {
  PT_POINTER   = 0x00000001,
  PT_TOUCH     = 0x00000002,
  PT_PEN       = 0x00000003,
  PT_MOUSE     = 0x00000004 
} POINTER_INPUT_TYPE;

// RegisterPointerInputTarget in User32.DLL
typedef BOOL (WINAPI *pfnRegisterPointerInputTarget)(
	IN	HWND hwnd,
    IN	POINTER_INPUT_TYPE  pointerType
    );

HMODULE s_hModUser32 = NULL;
const wchar_t* USER32PATH = L"User32.dll";
void TestRegisterPointerInputTarget(const TCHAR* szParam)
{
	if (NULL == s_hModUser32)
	{
		s_hModUser32 = ::LoadLibrary(USER32PATH);
		if (NULL != s_hModUser32)
		{
			pfnRegisterPointerInputTarget pfnRegister = NULL;
			pfnRegister = (pfnRegisterPointerInputTarget)::GetProcAddress(
				s_hModUser32, 
				"RegisterPointerInputTarget"
				);
			int nRet = pfnRegister(g_hwndMainDlg, PT_POINTER);
			if (nRet == 0)
			{
				LogMessage(_T("RegisterPointerInputTarget failed"));
			}
			else
			{
				LogMessage(_T("RegisterPointerInputTarget successfully, g_hwndMainDlg=%x"), g_hwndMainDlg);
			}

		}
		else
		{
			LogMessage(_T("LoadLibrary dll(%s) return failed."), USER32PATH);
		}
	}
}

void TestFindWindow(const TCHAR* szParam)
{
	HWND hWnd = ::FindWindow(L"EdgeUiInputWndClass", NULL);
	while (hWnd)
	{
		LogMessage(L"hWnd=%x", hWnd);
		hWnd = ::FindWindowEx(NULL, hWnd, L"EdgeUiInputWndClass", NULL);
	}
}
