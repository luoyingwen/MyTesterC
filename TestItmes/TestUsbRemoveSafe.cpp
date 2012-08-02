#include "stdafx.h"
#include "Common.h"

#include "D:\\luoyw\\work\\checkinsource\\Aries\\Alpha\\Src\\WinIntegration\\UsbDriveEjector.h"

void TestUsbRemove(const TCHAR* szParam)
{
	if (szParam == NULL || wcslen(szParam) <= 0)
	{
		LogMessage(L"Please input drive letter");
		return;
	}

	wchar_t DriveLetter = szParam[0];
	DriveLetter &= ~0x20; // uppercase

	if ( DriveLetter < 'A' || DriveLetter > 'Z' ) {
		return;
	}

	UsbDriveEjector ejector(DriveLetter);
	if (ejector.Eject())
	{
		TheLogger.Error(L"UsbDriveEjector::Eject return successfully.");
		return;
	}
	else
	{
		TheLogger.Error(L"UsbDriveEjector::Eject return failed");
		return;
	}
}
