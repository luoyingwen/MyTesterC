#pragma once

#include <atlstr.h>
#include <atltime.h>

#ifdef MYTESTER_PROJECT
#include "Common.h"
#endif

namespace Common
{
	enum LogLevel
	{
		DebugLevel,
		WarnLevel,
		ErrorLevel
	};

	class Logger
	{
	public:
		Logger()
		{
			int ar[2]={0, 1};
			this->logLevel = ErrorLevel;
			this->hLogFile = INVALID_HANDLE_VALUE;
		}
		Logger(LPCTSTR loggerName, LogLevel logLevel)
		{
			Open(loggerName, logLevel);
		}
		~Logger()
		{
			Close();
		}

	public:
		bool Open(LPCTSTR loggerName, LogLevel logLevel)
		{
			Close();
			this->logLevel = logLevel;

			this->hLogFile = INVALID_HANDLE_VALUE;
			CString csLogFile = BuildFilePath(loggerName);
			if (csLogFile.GetLength() > 0)
			{
				this->hLogFile = CreateFile(csLogFile.GetBuffer(), FILE_APPEND_DATA,
					FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if (this->hLogFile != INVALID_HANDLE_VALUE)
				{
					DWORD dwPos;
					if ((dwPos = SetFilePointer(this->hLogFile, 0, NULL, FILE_END)) == INVALID_SET_FILE_POINTER)
					{
						Close();
					}
					else if (dwPos == 0)
					{
	#ifdef UNICODE
						unsigned char szUnicodeHeader[2] = {0xff, 0xfe};
						DWORD dwWriten;
						if (!WriteFile(this->hLogFile, szUnicodeHeader, 2, &dwWriten, NULL))
						{
							Close();
						}
	#endif
					}
				}
			}

			return IsOpened();
		}
	public:
		void Debug(LPCTSTR format, ...)
		{
			if (DebugLevel >= this->logLevel && IsOpened())
			{
				va_list vList;
				va_start(vList, format);
				Log(DebugLevel, format, vList);
				va_end(vList);
			}
		}

		void Warn(LPCTSTR format, ...)
		{
			if (WarnLevel >= this->logLevel  && IsOpened())
			{
				va_list vList;
				va_start(vList, format);
				Log(WarnLevel, format, vList);
				va_end(vList);
			}
		}

		void Error(LPCTSTR format, ...)
		{
			if (ErrorLevel >= this->logLevel  && IsOpened())
			{
				va_list vList;
				va_start(vList, format);
				Log(ErrorLevel, format, vList);
				va_end(vList);
			}
		}

	public:
		void Close()
		{
			if (IsOpened())
			{
				CloseHandle(this->hLogFile);
				this->hLogFile = INVALID_HANDLE_VALUE;
			}
		}

	private:
		void Log(LogLevel level, LPCTSTR format, va_list vList)
		{
			TCHAR szBuffer[MAX_PATH + 4];
			int count = _vsntprintf_s(szBuffer, MAX_PATH, MAX_PATH - 1, format, vList);
			if (count <= 0)
			{
				_tcscat_s(szBuffer, MAX_PATH + 4, _T("..."));
			}
			else
			{
				szBuffer[MAX_PATH] = _T('\0');
			}

			CString csMessage;
			CTime now = CTime::GetCurrentTime();

			csMessage.AppendFormat(_T("%s"), GetLogLevelInfo(level));
			csMessage.AppendFormat(_T(" %s"), Format(now));
			csMessage.AppendFormat(_T(" %s"), szBuffer);
			csMessage.Append(_T("\r\n"));

			DWORD dwWriten;
			if (!WriteFile(this->hLogFile, csMessage.GetBuffer(), csMessage.GetLength() * sizeof(TCHAR), &dwWriten, NULL))
			{
				this->Close();
			}

#ifdef MYTESTER_PROJECT
			LogStringMessage(szBuffer);
#endif
		}

		bool IsOpened()const
		{
			return this->hLogFile != INVALID_HANDLE_VALUE;
		}

		CString BuildFilePath(LPCTSTR loggerName)
		{
			CString csResult;

			TCHAR szTempPath[MAX_PATH];
			DWORD dwResult = GetTempPath(MAX_PATH, szTempPath);
			if (dwResult > 0)
			{
				CTime now = CTime::GetCurrentTime();
				csResult.Append(szTempPath);
				csResult.AppendFormat(_T("%s.%d-%d-%d.txt"), loggerName, now.GetYear(), now.GetMonth(), now.GetDay());
			}

			return csResult;
		}

		CString Format(CTime time)
		{
			return time.Format(_T("%H:%M:%S %Y%m%d"));
		}

		const TCHAR* GetLogLevelInfo(LogLevel level)
		{
			const TCHAR* READAbLELOGLEVEL[5] = {_T("Debug"), _T("Info"), _T("Warn"), _T("Error"), _T("Fatal")}; 
			return READAbLELOGLEVEL[level];
		}

	private:
		LogLevel logLevel;
		HANDLE hLogFile;
	};
}