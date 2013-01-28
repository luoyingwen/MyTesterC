#include "stdafx.h"
#include "Common.h"

void TestChangeLogLevel(const TCHAR* szParam)
{
	LogStringMessage(__WFILE__);
	Logger::ChangeDynamicLogLevel(L"MyTesterLog.log", DebugLevel);
	TheLogger.Warn(L"You can show debug leve log. This sentence is printed from debug Level");

	TheLogger.Debug(L"Debug Level Log");
	TheLogger.Warn(L"Warn Level Log");
	TheLogger.Error(L"Error Level Log");
	
	Logger::ChangeDynamicLogLevel(L"MyTesterLog.log", WarnLevel);
	TheLogger.Warn(L"You can show warn leve log. This sentence is printed from warn Level");

	TheLogger.Debug(L"Debug Level Log");
	TheLogger.Warn(L"Warn Level Log");
	TheLogger.Error(L"Error Level Log");

	Logger::ChangeDynamicLogLevel(L"MyTesterLog.log", ErrorLevel);
	TheLogger.Error(L"You can show error leve log. This sentence is printed from error Level");

	TheLogger.Debug(L"Debug Level Log");
	TheLogger.Warn(L"Warn Level Log");
	TheLogger.Error(L"Error Level Log");

	Logger::ChangeDynamicLogLevel(L"MyTesterLog.log", DebugLevel);
}

void TestShowAllLevelLog(const TCHAR* szParam)
{
	TheLogger.Debug(L"Debug Level Log");
	TheLogger.Warn(L"Warn Level Log");
	TheLogger.Error(L"Error Level Log");
}

void TestChangeLogLevelToDebug(const TCHAR* szParam)
{
	Logger::ChangeDynamicLogLevel(L"MyTesterLog.log", DebugLevel);
}

void TestChangeLogLevelToWarn(const TCHAR* szParam)
{
	Logger::ChangeDynamicLogLevel(L"MyTesterLog.log", WarnLevel);
}

void TestChangeLogLevelToError(const TCHAR* szParam)
{
	Logger::ChangeDynamicLogLevel(L"MyTesterLog.log", ErrorLevel);
}

void TestChangeSpeedWatcherServiceLogLevel(const TCHAR* szParam)
{
	Logger::ChangeDynamicLogLevel(L"SpeedWatcherService", DebugLevel);
}

COMMAND_DEFINE(Test, TestChangeLogLevel);
COMMAND_DEFINE(Test, TestShowAllLevelLog);
COMMAND_DEFINE(Test, TestChangeLogLevelToDebug);
COMMAND_DEFINE(Test, TestChangeLogLevelToWarn);
COMMAND_DEFINE(Test, TestChangeLogLevelToError);
COMMAND_DEFINE(Test, TestChangeSpeedWatcherServiceLogLevel);