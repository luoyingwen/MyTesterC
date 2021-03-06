#pragma once

#include <atlstr.h>
#include <atltime.h>
#include <Shlobj.h>
#include <atlsecurity.h>

#ifdef MYTESTER_PROJECT
#include "Common.h"
#endif

namespace Common
{
	const LONGLONG MAX_LOGFILE_SIZE = 128 * 1024;//If file size is larger than this value, Logger File will be delete before opening the file.

	using namespace ATL;
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
			this->defaultLogLevel = ErrorLevel;
			this->hLogFile = INVALID_HANDLE_VALUE;
			this->pDynamicLevel = NULL;
			this->hdynamicMapFile = NULL;
		}
		~Logger()
		{
			Close();
		}

	public:
		bool Open(LPCTSTR loggerName, LogLevel logLevel)
		{
			Close();
			this->defaultLogLevel = logLevel;

			this->hLogFile = INVALID_HANDLE_VALUE;
			CString csLogFile = BuildFilePath(loggerName);
			if (csLogFile.GetLength() > 0)
			{
				LARGE_INTEGER fileSize =  GetFileSize(csLogFile.GetBuffer());
				if (fileSize.QuadPart > MAX_LOGFILE_SIZE)
				{
					::DeleteFile(csLogFile.GetBuffer());
				}

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
			if (IsOpened())
			{
				CreateDynamicLogLevel(loggerName, logLevel);
			}

			return IsOpened();
		}
		
		void Close()
		{
			if (IsOpened())
			{
				CloseHandle(this->hLogFile);
				this->hLogFile = INVALID_HANDLE_VALUE;
			}

			if (this->pDynamicLevel != NULL)
			{
				::UnmapViewOfFile(this->pDynamicLevel);
			}
			if (this->hdynamicMapFile != NULL)
			{
				::CloseHandle(this->hdynamicMapFile);
			}
		}
	public:
		void Debug(LPCTSTR format, ...)
		{
			if (DebugLevel >= GetLogLevel() && IsOpened())
			{
				va_list vList;
				va_start(vList, format);
				Log(DebugLevel, format, vList, true);
				va_end(vList);
			}
		}

		void Warn(LPCTSTR format, ...)
		{
			if (WarnLevel >= GetLogLevel() && IsOpened())
			{
				va_list vList;
				va_start(vList, format);
				Log(WarnLevel, format, vList, true);
				va_end(vList);
			}
		}

		void Error(LPCTSTR format, ...)
		{
			if (ErrorLevel >= GetLogLevel() && IsOpened())
			{
				va_list vList;
				va_start(vList, format);
				Log(ErrorLevel, format, vList, true);
				va_end(vList);
			}
		}

		void DebugWithoutExtra(LPCTSTR format, ...)
		{
			if (DebugLevel >= GetLogLevel() && IsOpened())
			{
				va_list vList;
				va_start(vList, format);
				Log(DebugLevel, format, vList, false);
				va_end(vList);
			}
		}
	public:
		static bool ChangeDynamicLogLevel(const TCHAR* szMappingTrait, LogLevel logLevel)
		{
			CString mappingName;
			GetMappingName(mappingName, szMappingTrait);

			HANDLE hMapFile = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, mappingName);
			if (hMapFile == NULL)
			{
				return false;
			}

			LogLevel* pDynamicLevel = (LogLevel*)::MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(LogLevel));
			if (pDynamicLevel == NULL)
			{
				::CloseHandle(hMapFile);
				return false;
			}

			*pDynamicLevel = logLevel;
			::UnmapViewOfFile(pDynamicLevel);
			::CloseHandle(hMapFile);
			return true;
		}
	private:
		void Log(LogLevel level, LPCTSTR format, va_list vList, bool withExtra)
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
			if (withExtra)
			{
				CTime now = CTime::GetCurrentTime();
				csMessage.AppendFormat(_T("%s"), GetLogLevelInfo(level));
				csMessage.AppendFormat(_T(" %s"), Format(now));
			}
			csMessage.AppendFormat(_T(" %s"), szBuffer);
			if (withExtra && csMessage.ReverseFind(_T('\n')) == -1)
			{
				csMessage.Append(_T("\r\n"));
			}

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
			if (SHGetSpecialFolderPath(NULL, szTempPath, CSIDL_COMMON_APPDATA, FALSE))
			{
				::PathAppend(szTempPath, L"Lenovo\\");
				::CreateDirectory(szTempPath, NULL);
				::PathAppend(szTempPath, L"Alpha\\");
				::CreateDirectory(szTempPath, NULL);
				::PathAppend(szTempPath, L"log\\");
				::CreateDirectory(szTempPath, NULL);

				csResult.Append(szTempPath);
				csResult.AppendFormat(_T("%s.txt"), loggerName);
			}

			return csResult;
		}

		CString Format(CTime time)
		{
			return time.Format(_T("%H:%M:%S %Y%m%d"));
		}

		const TCHAR* GetLogLevelInfo(LogLevel level)
		{
			const TCHAR* READAbLELOGLEVEL[5] = {_T("Debug"), _T("Warn"), _T("Error")}; 
			return READAbLELOGLEVEL[level];
		}

		void CreateDynamicLogLevel(const TCHAR* szMappingTrait, LogLevel logLevel)
		{
			CString mappingName;
			GetMappingName(mappingName, szMappingTrait);
			Debug(mappingName.GetBuffer());

			SECURITY_ATTRIBUTES  sa;
			sa.nLength = sizeof(SECURITY_ATTRIBUTES);
			sa.bInheritHandle = FALSE;  
			TCHAR * szSD = TEXT("D:")    // Discretionary ACL
				TEXT("(A;OICI;GA;;;AU)") // Allow //GRGWGX
				TEXT("(A;OICI;GA;;;BA)");    // Allow full control to administrators
			if (!::ConvertStringSecurityDescriptorToSecurityDescriptor(szSD, SDDL_REVISION_1, &(sa.lpSecurityDescriptor), NULL))
			{
				Error(L"ConvertStringSecurityDescriptorToSecurityDescriptor() failed");
				return;
			}

			hdynamicMapFile = ::CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, sizeof(LogLevel), mappingName);
			if (hdynamicMapFile == NULL)
			{
				Error(L"CreateDynamicLogLevel. CreateFileMapping return failed. error=%d", ::GetLastError());
				return;
			}

			this->pDynamicLevel = (LogLevel*)::MapViewOfFile(this->hdynamicMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(LogLevel));
			if (this->pDynamicLevel == NULL)
			{
				Error(L"CreateDynamicLogLevel. MapViewOfFile return failed. error=%d", ::GetLastError());
				::CloseHandle(this->hdynamicMapFile);
				this->hdynamicMapFile = NULL;
				return;
			}
			*this->pDynamicLevel = logLevel;
		}

		LogLevel GetLogLevel()const
		{
			if (this->pDynamicLevel != NULL)
				return *this->pDynamicLevel;
			else
				return defaultLogLevel;
		}

		static bool IsAdmin()
		{
			CAccessToken token;
			bool isAdmin = false;
			token.CheckTokenMembership(Sids::Admins(), &isAdmin);
			return isAdmin;
		}

		static void GetMappingName(CString& mappingName, const TCHAR* szMappingTrait)
		{
			if (IsAdmin())
			{
				mappingName.AppendFormat(_T("Global\\%sLogLevel"), szMappingTrait);
			}
			else
			{
				mappingName.AppendFormat(_T("Local\\%sLogLevel"), szMappingTrait);
			}
		}

		static LARGE_INTEGER GetFileSize(LPCTSTR fileName)
		{
			LARGE_INTEGER fileSize = {0};
			HANDLE hFile = CreateFile(fileName, FILE_READ_DATA,
					FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				::GetFileSizeEx(hFile, &fileSize);
				::CloseHandle(hFile);
			}
			return fileSize;
		}

	private:
		LogLevel defaultLogLevel;
		HANDLE hLogFile;

		HANDLE hdynamicMapFile;
		LogLevel* pDynamicLevel;
	};
}