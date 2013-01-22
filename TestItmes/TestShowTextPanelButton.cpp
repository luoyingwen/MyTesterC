#include "stdafx.h"
#include "resource.h"
#include "Common.h"
#include <Shellapi.h>
#include "..\..\..\work\Master20130111\AriesNew\Alpha\Src\WinIntegration\ShowTextPanelButton.h"

class SoftKeyboardMgr
{
private:
       static void SignalEvent(const TCHAR* szEventName)
       {
              HANDLE hEvent = ::OpenEvent(EVENT_MODIFY_STATE, TRUE, szEventName);
              if (hEvent != NULL)
              {
                     ::SetEvent(hEvent);
                     ::CloseHandle(hEvent);
              }
       }
public:
       static void ShowButton()
       {
              SignalEvent(_T("ShowSoftKeyboardEvent"));
       }

       static void HideButton()
       {
              SignalEvent(_T("HideSoftKeyboardEvent"));
       }
};


void TestShowButton(const TCHAR* szParam)
{
	//ShowTextPanelButton::StartMonitorThread();
	SoftKeyboardMgr::ShowButton();
}

void TestHideButton(const TCHAR* szParam)
{
	SoftKeyboardMgr::HideButton();
}

void TestStartThread(const TCHAR* szParam)
{
	ShowTextPanelButton::StartMonitorThread();
}

COMMAND_DEFINE(Test, TestShowButton);
COMMAND_DEFINE(Test, TestHideButton);
COMMAND_DEFINE(Test, TestStartThread);