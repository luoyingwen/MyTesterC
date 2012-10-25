#include "stdafx.h"
#include "Common.h"

void TestSendInputHotKey(const TCHAR* szParam)
{
	::Sleep(30*1000);
	LogStringMessage(__WFILE__);
	INPUT winKeys[2];
	winKeys[0].type = INPUT_KEYBOARD;
	winKeys[1].type = INPUT_KEYBOARD;
	winKeys[0].ki.wVk = VK_VOLUME_MUTE;
	winKeys[1].ki.wVk = VK_VOLUME_MUTE;
	winKeys[0].ki.dwExtraInfo = 0;
	winKeys[1].ki.dwExtraInfo = 0;

	winKeys[0].ki.time = 0;
	winKeys[1].ki.time = 0;

	winKeys[0].ki.wScan = 0;
	winKeys[1].ki.wScan = 0;

	winKeys[0].ki.dwFlags = 0;
	winKeys[1].ki.dwFlags = KEYEVENTF_KEYUP;
	LogMessage(L"Post Mute key");
	::SendInput(2, winKeys, sizeof(INPUT));
}
