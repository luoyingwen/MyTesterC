#pragma once

static const wchar_t* MODULE_SEPERATOR = L"\r\n";//isn't a valid chars of file name. if you change it, you must change fixed data file format.
const DWORD MAX_MODULE_FILE_SIZE = 100 * 1024;

//c:\program files\lenovo\alpha\UNFIXED_PREFETCHER_DATA file contains unfixed dll name.
//ExportData generates fixed full path base these dll name on every machine
static const wchar_t* UNFIXED_PREFETCHER_DATA = L"UnFixedPrefetcher.data";

//c:\program files\lenovo\alpha\FIXED_PREFETCHER_DATA file contains fixed dll name.
//all these dlls locate at c:\windows\system32 folder
static const wchar_t* FIXED_PREFETCHER_DATA = L"FixedPrefetcher.data";

class ModulePrefetcher : public Prefetcher
{
public:
	virtual DWORD ExportData(DWORD dwProcessId)
	{
		TheLogger.Debug(L"Target Process=%d", dwProcessId);
		WriteModuleInfoToDatafile(dwProcessId);
		return 0;
	}

	virtual DWORD Load()
	{
		CString allLoadedModules;
		CString fixedLibraries = BuildDataFilePath(FIXED_PREFETCHER_DATA);
		LoadModuleFromDataFile(fixedLibraries.GetBuffer(), allLoadedModules);

		CString filePath = BuildExportDataFilePath(UNFIXED_PREFETCHER_DATA);
		LoadModuleFromDataFile(filePath.GetBuffer(), allLoadedModules);

		allLoadedModules.MakeUpper();
		LoadAllLibraryInPhysicalMem(allLoadedModules);
		return 0;
	}
private:
	size_t WriteWCharToBuffer(char* contentBuf, size_t bufSize, const wchar_t* content)const
	{
		size_t dwStrLenInByte = sizeof(wchar_t) * wcslen(content);
		if (bufSize < dwStrLenInByte)
		{
			TheLogger.Error(L"Content Buffer too smaller.");
			return 0;
		}
		::memcpy(contentBuf, content, dwStrLenInByte);
		return dwStrLenInByte;
	}

	BOOL WriteBufferToFile(wchar_t* szFileName, char* contentBuf, size_t contentSize)const
	{
		TheLogger.Debug(L"WriteBufferToFile. FileName=%s, size=%d", szFileName, contentSize);
		HANDLE hFile = CreateFile(szFileName,               // file to open
			GENERIC_WRITE,          // open for writing
			FILE_SHARE_READ,       // share for reading
			NULL,                  // default security
			CREATE_ALWAYS,         // Creates a new file, always
			FILE_ATTRIBUTE_NORMAL, // normal file
			NULL);                 // no attr. template

		if (hFile == INVALID_HANDLE_VALUE) 
		{
			TheLogger.Error(L"CreateFile Return failed.Path=%s. Error=%d", szFileName, ::GetLastError());
			return FALSE; 
		}
		DWORD dwWriten = 0;
		::WriteFile(hFile, contentBuf, (DWORD)contentSize, &dwWriten, NULL);
		if (dwWriten != contentSize)
		{
			TheLogger.Error(L"WriteFile return failed. dwWriten=%d, contentSize=%d, lastError=%d", dwWriten, contentSize, ::GetLastError());
			::CloseHandle(hFile);
			return FALSE;
		}
		::CloseHandle(hFile);
		return TRUE;
	}

	BOOL ReadFileDataToBuffer(const wchar_t* szFileName, char* contentBuf, DWORD bufSize, size_t* pReadSize)const
	{
		TheLogger.Debug(L"ReadFileDataToBuffer. %s", szFileName);
		*pReadSize = 0;
		HANDLE hFile = CreateFile(
			szFileName,            // file to open
			GENERIC_READ,          // open for reading
			FILE_SHARE_READ,       // share for reading
			NULL,                  // default security
			OPEN_EXISTING,         // existing file only
			FILE_ATTRIBUTE_NORMAL, // normal file
			NULL);                 // no attr. template

		if (hFile == INVALID_HANDLE_VALUE) 
		{
			TheLogger.Error(L"CreateFile Return failed. FilePath=%s, Error=%d", szFileName, ::GetLastError());
			return FALSE;
		}
		DWORD  dwBytesRead = 0;
		if( FALSE == ReadFile(hFile, contentBuf, bufSize, &dwBytesRead, NULL))
		{
			TheLogger.Error(L"ReadFile Return failed. Error=%d. dwBytesRead=%d", ::GetLastError(), dwBytesRead);
			::CloseHandle(hFile);
			return FALSE;
		}
		*pReadSize = dwBytesRead;
		::CloseHandle(hFile);
		return TRUE;
	}

	void GetFilter(CString& filterData)const
	{
		char* filterBuf = new char[MAX_MODULE_FILE_SIZE + 2];//alloc more 2 bytes for filling 0 which is needed by string
		size_t readSize = 0;
		CString unFixedLibraries = BuildDataFilePath(UNFIXED_PREFETCHER_DATA);
		//file header contain unicode flag
		if (ReadFileDataToBuffer(unFixedLibraries.GetBuffer(), filterBuf, MAX_MODULE_FILE_SIZE, &readSize))
		{
			ATLASSERT(filterBuf[0] == (char)0xff);
			ATLASSERT(filterBuf[1] == (char)0xfe);
			ATLASSERT(readSize <= MAX_MODULE_FILE_SIZE);
			if (readSize > 2 && readSize <= MAX_MODULE_FILE_SIZE && filterBuf[0] == (char)0xff && filterBuf[1] == (char)0xfe)
			{
				filterBuf[readSize] = '\0';
				filterBuf[readSize + 1] = '\0';
				filterData = (wchar_t*)(filterBuf + 2); //copy  filter data
			}
		}
		delete filterBuf;
	}

	BOOL ListProcessModules( DWORD dwPID, char* contentBuf, size_t dwBufSize, size_t* pWriteLen, const CString& filterData)const
	{
		*pWriteLen = 0;
		HANDLE hModuleSnap = INVALID_HANDLE_VALUE; 
		MODULEENTRY32 me32; 

		hModuleSnap = ::CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, dwPID ); 
		if( hModuleSnap == INVALID_HANDLE_VALUE ) 
		{ 
			TheLogger.Error(L"CreateToolhelp32Snapshot (of modules) return failed. Error=%d", ::GetLastError()); 
			return( FALSE ); 
		} 
		me32.dwSize = sizeof( MODULEENTRY32 ); 
		if( !::Module32First( hModuleSnap, &me32 ) ) 
		{ 
			TheLogger.Error( L"Module32First return failed. error=%d", ::GetLastError());
			::CloseHandle( hModuleSnap);
			return( FALSE ); 
		}

		do 
		{ 
			size_t nLen = wcslen(me32.szExePath);
			wchar_t* extension = me32.szExePath + nLen + 1 - sizeof(".dll");
			
			CString moduleName(me32.szModule);
			moduleName.MakeUpper();//filterData is upper

			if (nLen > sizeof(".dll") && _wcsicmp(extension, L".dll") == 0  //is dll
				&& wcsstr(me32.szExePath, L"WinSxS") == NULL //not in WinSxs folder
				&& filterData.Find(moduleName) != -1
				)
			{
				TheLogger.Debug(L"%s", me32.szExePath);
				*pWriteLen += WriteWCharToBuffer(contentBuf + *pWriteLen, dwBufSize - *pWriteLen, me32.szExePath);
				*pWriteLen += WriteWCharToBuffer(contentBuf + *pWriteLen, dwBufSize - *pWriteLen, MODULE_SEPERATOR);
			}
		} while( ::Module32Next( hModuleSnap, &me32 ) ); 

		::CloseHandle( hModuleSnap );
		return( TRUE ); 
	}

	void WriteModuleInfoToDatafile(DWORD dwProcessId)const
	{
		CString filterData;
		GetFilter(filterData);
		filterData.MakeUpper();

		char* contentBuf = new char[MAX_MODULE_FILE_SIZE + 2];
		//file header contain unicode flag
		contentBuf[0] = (char)0xff;
		contentBuf[1] = (char)0xfe;
		char* moduleHead = contentBuf + 2;

		size_t writeLen = 0;
		ListProcessModules(dwProcessId, moduleHead, MAX_MODULE_FILE_SIZE, &writeLen, filterData);
		if (writeLen > 0)
		{
			CString filePath = BuildExportDataFilePath(UNFIXED_PREFETCHER_DATA);
			WriteBufferToFile(filePath.GetBuffer(), contentBuf, writeLen + moduleHead - contentBuf);
		}
		else
		{
			TheLogger.Error(L"Can't get any module path data.");
		}
		delete contentBuf;
	}

	void LoadModuleFromDataFile(const wchar_t* szFileName, CString& loadedModules)
	{
		char* contentBuf = new char[MAX_MODULE_FILE_SIZE + 2];//alloc more 2 bytes for filling 0 which is needed by string
		size_t readSize = 0;
		//file header contain unicode flag
		if (ReadFileDataToBuffer(szFileName, contentBuf, MAX_MODULE_FILE_SIZE, &readSize))
		{
			ATLASSERT(contentBuf[0] == (char)0xff);
			ATLASSERT(contentBuf[1] == (char)0xfe);
			ATLASSERT(readSize <= MAX_MODULE_FILE_SIZE);
			if (readSize > 2 && readSize <= MAX_MODULE_FILE_SIZE && contentBuf[0] == (char)0xff && contentBuf[1] == (char)0xfe)
			{
				contentBuf[readSize] = '\0';
				contentBuf[readSize + 1] = '\0';
				wchar_t* pData = (wchar_t*)(contentBuf + 2);
				wchar_t* pNextSeg = wcsstr(pData, MODULE_SEPERATOR);
				while(pNextSeg != NULL)
				{
					wchar_t segment[MAX_PATH];
					memset(segment, 0, MAX_PATH * sizeof(wchar_t));
					DWORD segLen = pNextSeg - pData;
					memcpy(segment, pData, segLen * sizeof(wchar_t));

					CString dllPath(segment);
					dllPath.Trim();
					if (dllPath.GetLength() > 0)
					{
						LoadOneModule(dllPath.GetBuffer(), loadedModules);
					}
					pData += segLen + wcslen(MODULE_SEPERATOR);
					pNextSeg = wcsstr(pData, MODULE_SEPERATOR);
				}
			}
			else
			{
				TheLogger.Error(L"Data File content error.");
			}
		}
		delete contentBuf;
	}

	BOOL LoadAllLibraryInPhysicalMem(const CString& allLoadedModules)
	{ 
		HANDLE hModuleSnap = INVALID_HANDLE_VALUE; 
		MODULEENTRY32 me32; 

		hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, GetCurrentProcessId() ); 
		if( hModuleSnap == INVALID_HANDLE_VALUE ) 
		{ 
			TheLogger.Error(L"CreateToolhelp32Snapshot (of modules)"); 
			return( FALSE ); 
		} 
		me32.dwSize = sizeof( MODULEENTRY32 ); 
		if( !Module32First( hModuleSnap, &me32 ) ) 
		{ 
			TheLogger.Error(L"Module32First");  // Show cause of failure 
			CloseHandle( hModuleSnap );     // Must clean up the snapshot object! 
			return( FALSE ); 
		} 
		do 
		{ 
			TheLogger.Debug(L"%s", me32.szExePath);

			CString moduleName(me32.szModule);
			moduleName.MakeUpper();//filterData is upper
			if (allLoadedModules.Find(moduleName) != -1)
			{
				QueryAndLockOneModule((PVOID)me32.modBaseAddr, me32.modBaseSize);
			}
		} while( Module32Next( hModuleSnap, &me32 ) ); 

		CloseHandle( hModuleSnap ); 
		return( TRUE ); 
	} 

	static CString BuildExportDataFilePath(LPCTSTR loggerName)
	{
		CString csResult;

		TCHAR szTempPath[MAX_PATH];
		if (::SHGetSpecialFolderPath(NULL, szTempPath, CSIDL_COMMON_APPDATA, FALSE))
		{
			::PathAppend(szTempPath, L"Lenovo\\");
			::CreateDirectory(szTempPath, NULL);
			::PathAppend(szTempPath, L"Alpha\\");
			::CreateDirectory(szTempPath, NULL);
			::PathAppend(szTempPath, L"log\\");
			::CreateDirectory(szTempPath, NULL);

			csResult.Append(szTempPath);
			csResult.AppendFormat(_T("%s"), loggerName);
		}

		return csResult;
	}

	static CString BuildDataFilePath(LPCTSTR loggerName)
	{
		CString csResult;

		TCHAR szTempPath[MAX_PATH];
		::GetModuleFileName(NULL, szTempPath, sizeof(szTempPath)/sizeof(szTempPath[0]));
		::PathRemoveFileSpec(szTempPath);
		::PathAppend(szTempPath, loggerName);

		csResult = szTempPath;
		return csResult;
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

	static SIZE_T QueryAndLockOneModule(LPVOID lpAddress, SIZE_T range)
	{
		SIZE_T totalSize = 0;
		while(1)
		{
			LPVOID curAddress = (BYTE*)lpAddress + totalSize;
	
			MEMORY_BASIC_INFORMATION memInfo;
			SIZE_T actualSize = ::VirtualQuery(curAddress, &memInfo, sizeof(memInfo));
			if (actualSize == 0)
			{
				TheLogger.Error(L"VirtualQuery return failed. lasterror=%d", ::GetLastError());
				break;
			}
			else
			{
				TheLogger.Error(L"memory info:AllocationProtect=%x,  RegionSize=%d, State=%x, Protect=%x, Type=%x", memInfo.AllocationProtect, memInfo.RegionSize, memInfo.State, memInfo.Protect, memInfo.Type);
				//if (memInfo.Protect == PAGE_EXECUTE
				//	|| memInfo.Protect == PAGE_EXECUTE_READ
				//	|| memInfo.Protect == PAGE_EXECUTE_READWRITE
				//	|| memInfo.Protect == PAGE_EXECUTE_WRITECOPY
				//	|| memInfo.Protect == PAGE_READONLY
				//	|| memInfo.Protect == PAGE_WRITECOPY
				//	)
				{
					TheLogger.Debug(L"Lock this region");
					::VirtualLock(curAddress, memInfo.RegionSize);
				}
				totalSize += memInfo.RegionSize;
			}
			if (totalSize >= range)
			{
				break;
			}
		}
		return totalSize;
	}
	static void LoadOneModule(const wchar_t* szDllPath, CString& loadedModules)
	{
		TheLogger.Debug(L"LoadLibrary %s", szDllPath);
		HMODULE hLib = ::LoadLibrary(szDllPath);
		if (hLib == NULL)
		{
			TheLogger.Error(L"Load %s failed.", szDllPath);
		}
		else
		{
			const wchar_t* fileName = wcsrchr(szDllPath, '\\');
			if (fileName != NULL)
			{
				loadedModules.Append(fileName + 1);
			}
			else
			{
				loadedModules.Append(szDllPath);
			}
		}
	}
};