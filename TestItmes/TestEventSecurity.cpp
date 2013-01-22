#include "stdafx.h"
#include "Common.h"
static const wchar_t* STARTED_EVENT_NAME = L"Global\\TestEvent_security";

void TestCreateEvent(const TCHAR* szParam)
{
	LogStringMessage(__WFILE__);
	HANDLE hStartedEvent = ::CreateEvent(NULL, TRUE, FALSE, STARTED_EVENT_NAME);
	LogMessage(L"Create a named %s event, hEvent = %x, LastError=%d.", STARTED_EVENT_NAME, hStartedEvent, ::GetLastError());
	::ResetEvent(hStartedEvent);
}
void TestOpenEvent(const TCHAR* szParam)
{
	LogStringMessage(__WFILE__);
	HANDLE hStartedEvent = ::OpenEvent(EVENT_MODIFY_STATE, TRUE, STARTED_EVENT_NAME);
	LogMessage(L"OpenEvent a named %s event, hEvent = %x, LastError=%d.", STARTED_EVENT_NAME, hStartedEvent, ::GetLastError());
	::SetEvent(hStartedEvent);
	LogMessage(L"SetEvent returned. LastError=%d.", ::GetLastError());
}
COMMAND_DEFINE(Test, TestCreateEvent);
COMMAND_DEFINE(Test, TestOpenEvent);