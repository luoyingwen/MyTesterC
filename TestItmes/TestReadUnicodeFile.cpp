#include "stdafx.h"
#include "Common.h"
class ReadUnicodeFile
{
public:
	ReadUnicodeFile() 
	{
		_opened = false;
		_hLogFile = INVALID_HANDLE_VALUE;
	}
public:
	bool Open(const wchar_t* szFile)
	{
		_hLogFile = CreateFileW(szFile, FILE_READ_DATA,
			FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (_hLogFile != INVALID_HANDLE_VALUE)
		{
			_opened = true;
		}
		else
		{
			_opened = false;
		}
		return _opened;
	}
	bool ReadLine(CString& line)
	{
		if (!_opened)
		{
			return false;
		}

	}
private:
	bool _opened;
	HANDLE _hLogFile;
};

void TestReadUnicodeFile(const TCHAR* szParam)
{
	LogStringMessage(__WFILE__);
}
COMMAND_DEFINE(Test, TestReadUnicodeFile);