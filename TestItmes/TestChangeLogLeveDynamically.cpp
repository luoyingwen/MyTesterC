#include "stdafx.h"
#include "Common.h"

static void ChangeDynamicLogLevel(const TCHAR* szMappingTrait, LogLevel logLevel)
{
	CString mappingName;
	mappingName.AppendFormat(_T("Local\\%sLogLevel"), szMappingTrait);

	HANDLE hMapFile;
	LogLevel* pDynamicLevel;

	hMapFile = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,   // read/write access
		FALSE,                 // do not inherit the name
		mappingName);               // name of mapping object

	if (hMapFile == NULL)
	{
		LogMessage(TEXT("Could not open file mapping object (%d).\n"),
			GetLastError());
		return;
	}

	pDynamicLevel = (LogLevel*) MapViewOfFile(hMapFile, // handle to map object
		FILE_MAP_ALL_ACCESS,  // read/write permission
		0,
		0,
		sizeof(LogLevel));

	if (pDynamicLevel == NULL)
	{
		LogMessage(TEXT("Could not map view of file (%d).\n"),
			GetLastError());

		CloseHandle(hMapFile);
		return;
	}

	*pDynamicLevel = logLevel;

	UnmapViewOfFile(pDynamicLevel);
	CloseHandle(hMapFile);
}

void TestChangeLogLevel(const TCHAR* szParam)
{
	LogStringMessage(__WFILE__);
	ChangeDynamicLogLevel(L"MyTesterLog.log", DebugLevel);
	TheLogger.Warn(L"You can show debug leve log. This sentence is printed from debug Level");

	TheLogger.Debug(L"Debug Level Log");
	TheLogger.Warn(L"Warn Level Log");
	TheLogger.Error(L"Error Level Log");
	
	ChangeDynamicLogLevel(L"MyTesterLog.log", WarnLevel);
	TheLogger.Warn(L"You can show warn leve log. This sentence is printed from warn Level");

	TheLogger.Debug(L"Debug Level Log");
	TheLogger.Warn(L"Warn Level Log");
	TheLogger.Error(L"Error Level Log");

	ChangeDynamicLogLevel(L"MyTesterLog.log", ErrorLevel);
	TheLogger.Error(L"You can show error leve log. This sentence is printed from error Level");

	TheLogger.Debug(L"Debug Level Log");
	TheLogger.Warn(L"Warn Level Log");
	TheLogger.Error(L"Error Level Log");

	ChangeDynamicLogLevel(L"MyTesterLog.log", ErrorLevel);
}
COMMAND_DEFINE(Test, TestChangeLogLevel);