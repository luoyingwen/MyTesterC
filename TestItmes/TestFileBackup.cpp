#include "stdafx.h"
#include "Common.h"
#include "TestItmes\UsbDriveEjector.h"

const wchar_t* TARGET_DISK_GUID = L"DC11E9DE-A0F3-4469-B3B2-BA532FD1ED97";

void TestFileBackup(const TCHAR* szParam)
{
	LogStringMessage(__WFILE__);
	UsbDriveEjector ejector('f');
	ejector.Eject();
}
