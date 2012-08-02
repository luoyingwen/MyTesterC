#include "stdafx.h"
#include "Common.h"
#include <gdiplus.h>
using namespace Gdiplus;

#pragma comment(lib, "Gdiplus.lib")

static HMODULE ModuleFromAddress(PVOID pv) 
{
	MEMORY_BASIC_INFORMATION mbi;
	return ((::VirtualQuery(pv, &mbi, sizeof(mbi)) != 0) 
	        ? (HMODULE) mbi.AllocationBase : NULL);
}

static DWORD WINAPI MonitorProc(_In_ void* pv) throw()
{
	HBITMAP bmp;

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	Bitmap* image = new Bitmap(L"D:\\luoyw\\work\\checkinsource\\Aries\\Alpha\\Src\\Resources\\PolarLights\\Images\\background.png");
	Gdiplus::Color clr(0xFF,0xFF,0xFF);	
	image->GetHBITMAP(clr, &bmp);
		
	delete image;
	GdiplusShutdown(gdiplusToken);

	HWND hTarget = CreateWindowEx(WS_EX_NOACTIVATE, L"STATIC", L"", WS_POPUP | WS_BORDER | SS_BITMAP, -1,-1, 1920, 1080, NULL, NULL, ModuleFromAddress(MonitorProc), NULL);
	::SendMessage(hTarget, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP,(LPARAM)bmp);
	::ShowWindow(hTarget, SW_SHOW);
		
	MSG msg; 
	while (true) 
	{ 
		GetMessage(&msg, (HWND) NULL, 0, 0);
		TranslateMessage(&msg); 
		DispatchMessage(&msg); 
	} 

	return 0;
}

HBITMAP CreateBitmap()
{
	//compute background image full path.
	//locate at the same directory with WatcherService
	wchar_t szFilePath[MAX_PATH];
	::GetModuleFileName(NULL, szFilePath, sizeof(szFilePath)/sizeof(wchar_t));
	::PathRemoveFileSpec(szFilePath);
	::PathAppend(szFilePath, L"background.png");

	if (GetFileAttributes(szFilePath) == INVALID_FILE_ATTRIBUTES)
	{
		TheLogger.Warn(L"Can't get file attributes. Error=%d", ::GetLastError());
		return NULL;
	}

	HBITMAP hBitmap = NULL;
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	Bitmap* image = new Bitmap(szFilePath);
	Gdiplus::Color clr(0xFF,0xFF,0xFF);	
	Status status = image->GetHBITMAP(clr, &hBitmap);
	if (status != Ok)
	{
		LogMessage(L"GetHBITMAP return failed. error=%d", status);
	}
		
	delete image;
	GdiplusShutdown(gdiplusToken);
	return hBitmap;
}

void TestCreateWindow(const TCHAR* szParam)
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

void TestDecodePng(const TCHAR* szParam)
{
	HBITMAP bitmap = CreateBitmap();
	LogMessage(L"CreateBitmap return hbitmap = %d", bitmap);
	
}
