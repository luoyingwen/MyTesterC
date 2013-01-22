// MyTester.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "MyTester.h"
#include "Common.h"
#include <ole2.h>


#define TIMER_ID 1
#define TIMER_TIME_LOGMESSAGE 200 //million seconds
#define MAX_LOG_TEXT 1024*1024
TCHAR* g_szLogBuffer = NULL;
CRITICAL_SECTION g_Critecal_Sec;

CCommand* g_pCommandListHead = NULL;
HWND g_hwndMainDlg = NULL;

#pragma comment(lib, "comctl32.lib")
// Forward declarations of functions included in this code module:
INT_PTR CALLBACK	Dlg_Proc(HWND, UINT, WPARAM, LPARAM);
int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);
	
	memset(&g_Critecal_Sec, 0, sizeof(CRITICAL_SECTION));
	InitializeCriticalSection(&g_Critecal_Sec);

	TheLogger.Open(_T("MyTesterLog.log"), DebugLevel);
	OleInitialize(NULL);
	InitCommonControls();
	LoadLibrary(L"Riched20.dll");
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MYTESTER_DIALOG), NULL, Dlg_Proc);
	OleUninitialize();
	return 0;
}

BOOL ComboboxEx_Initialize()
{
	CCommand* pHead = g_pCommandListHead;
	while(pHead)
	{
		COMBOBOXEXITEM comboItem;
		comboItem.mask = CBEIF_TEXT | CBEIF_LPARAM;
		comboItem.iItem = 0;
		comboItem.pszText = pHead->name;
		comboItem.lParam = (LPARAM ) pHead->func;
		HWND hwndCombo = GetDlgItem(g_hwndMainDlg,IDC_COMBO_COMMANDS);
		::SendMessage(hwndCombo, CBEM_INSERTITEM, 0, (LPARAM)&comboItem);
		pHead = pHead->pNextCommand;
	}
	return g_pCommandListHead != NULL;
}

int ComboboxEx_SetCurSel(int nSelect)
{
	HWND hwndCombo = GetDlgItem(g_hwndMainDlg,IDC_COMBO_COMMANDS);
	return (int)::SendMessage(hwndCombo, CB_SETCURSEL, nSelect, 0L);
}

int ComboboxEx_GetCurSel()
{
	HWND hwndCombo = GetDlgItem(g_hwndMainDlg,IDC_COMBO_COMMANDS);
	return (int)::SendMessage(hwndCombo, CB_GETCURSEL, 0, 0L);
}

BOOL ComboboxEx_GetItem(PCOMBOBOXEXITEM pCBItem)
{
	HWND hwndCombo = GetDlgItem(g_hwndMainDlg, IDC_COMBO_COMMANDS);
	return (BOOL)::SendMessage(hwndCombo, CBEM_GETITEM, 0, (LPARAM)pCBItem);
}

void Edit_AppendText(LPCTSTR lpstrText, BOOL bNoScroll = FALSE, BOOL bCanUndo = FALSE)
{
	HWND hwndEditLog = GetDlgItem(g_hwndMainDlg,IDC_EDIT_LOGS);
	int nCurLen = GetWindowTextLength(hwndEditLog);
	//select end
	{
		::SendMessage(hwndEditLog, EM_SETSEL, nCurLen, nCurLen);
		if(!bNoScroll)
			::SendMessage(hwndEditLog, EM_SCROLLCARET, 0, 0L);
	}
	//replace
	::SendMessage(hwndEditLog, EM_REPLACESEL, (WPARAM) bCanUndo, (LPARAM)lpstrText);
}
void Edit_Clear()
{
	SetWindowText(GetDlgItem(g_hwndMainDlg,IDC_EDIT_LOGS), _T(""));
}

void LogStringMessage(const TCHAR* szMsg)
{
	if(szMsg == NULL)
		return;

	EnterCriticalSection(&g_Critecal_Sec);

	size_t nTotalLen = _tcslen(szMsg);
	static TCHAR* szSep = _T("\r\n");
	static size_t nSep = _tcslen(szSep);
	nTotalLen += nSep;
	if(g_szLogBuffer != NULL)
	{
		nTotalLen += _tcslen(g_szLogBuffer);
	}
	nTotalLen += 1;
	TCHAR* szNewBuffer = new TCHAR[nTotalLen];
	if(g_szLogBuffer != NULL)
	{
		_tcscpy_s(szNewBuffer, nTotalLen, g_szLogBuffer);
		_tcscat_s(szNewBuffer, nTotalLen, szMsg);
		delete[] g_szLogBuffer;
		g_szLogBuffer = NULL;
	}
	else
	{
		_tcscpy_s(szNewBuffer, nTotalLen, szMsg);
	}
	_tcscat_s(szNewBuffer, nTotalLen, szSep);
	g_szLogBuffer = szNewBuffer;

	LeaveCriticalSection(&g_Critecal_Sec);
}

void LogMessage(const TCHAR* _Format, ...)
{
	if(_Format == NULL)
		return;

	va_list l;
	va_start(l,_Format);

	TCHAR szLine[MAX_PATH + 4];
	int count = _vsntprintf_s(szLine,MAX_PATH, MAX_PATH - 1, _Format,l);
	if (count <= 0)
		_tcscat_s(szLine, MAX_PATH + 4, _T("..."));
	else
		szLine[MAX_PATH] = '\0';
	va_end(l);

	LogStringMessage(szLine);
}


VOID CALLBACK LogMessageTimerProc(HWND hwnd, UINT message, UINT_PTR idTimer, DWORD dwTime)
{
	if(g_szLogBuffer != NULL)
	{
		EnterCriticalSection(&g_Critecal_Sec);
		Edit_AppendText(g_szLogBuffer);
		delete[] g_szLogBuffer;
		g_szLogBuffer = NULL;
		LeaveCriticalSection(&g_Critecal_Sec);
	}
}

BOOL Dlg_OnInitDialog(HWND hwndDlg, HWND hwndFocus, LPARAM lParam)
{
	g_hwndMainDlg = hwndDlg;
	if(ComboboxEx_Initialize())
	{
		ComboboxEx_SetCurSel(0);
	}
	else
	{
		::EnableWindow(GetDlgItem(hwndDlg,IDC_BTN_EXECUTE), FALSE);
	}

	//set text limit
	HWND hwndEditLog = GetDlgItem(hwndDlg,IDC_EDIT_LOGS);
	::SendMessage(hwndEditLog, EM_SETLIMITTEXT, MAX_LOG_TEXT, 0);

	SetTimer(hwndDlg, TIMER_ID, TIMER_TIME_LOGMESSAGE, LogMessageTimerProc);
	return TRUE;
}

void Dlg_OnExecute()
{
	int iSel = ComboboxEx_GetCurSel();
	HWND hwndParam = GetDlgItem(g_hwndMainDlg, IDC_EDIT_INPUT_PARAM);
	int nParamLen = GetWindowTextLength(hwndParam);
	TCHAR* szParam = NULL;
	if(nParamLen > 0)
	{
		szParam = new TCHAR[nParamLen + 1];
		GetWindowText(hwndParam, szParam, nParamLen + 1);
		szParam[nParamLen] = 0;
	}
	COMBOBOXEXITEM comboItem;
	comboItem.mask = CBEIF_LPARAM;
	comboItem.iItem = iSel;
	ComboboxEx_GetItem(&comboItem);
	MenuFunc func = (MenuFunc)comboItem.lParam;
	if(func != NULL)
		func(szParam);
}

void Dlg_OnClearLogs()
{
	Edit_Clear();
}

void Dlg_OnCommand(HWND hwndDlg, int id, HWND hwndCtrl, UINT codeNotify)
{
	switch(id)
	{
	case IDOK:
	case IDCANCEL:
		{
			EndDialog(hwndDlg, id);
		}
		break;
	case IDC_BTN_EXECUTE:
		{
			Dlg_OnExecute();
		}
		break;
	case IDC_BTN_CLEAR_LOGS:
		{
			Dlg_OnClearLogs();
		}
		break;
	}
}

INT_PTR CALLBACK Dlg_Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)Dlg_OnInitDialog(hDlg, NULL, lParam);

	case WM_COMMAND:
		{
			int idCtrl = LOWORD(wParam);
			HWND hwndCtrl = GetDlgItem(hDlg, idCtrl);
			Dlg_OnCommand(hDlg, idCtrl, hwndCtrl, 0); 
		}
		break;
	}
	return (INT_PTR)FALSE;
}
