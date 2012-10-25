#pragma once

class CCommand;
typedef void (*MenuFunc)(const TCHAR* szParam);
extern CCommand* g_pCommandListHead;
class CCommand
{
public:
	CCommand(const TCHAR* szCategory, const TCHAR* szFuncName, MenuFunc f)
	{
		_tcscpy_s(name, MAX_PATH, szCategory);
		_tcscat_s(name, MAX_PATH, _T("_"));
		_tcscat_s(name, MAX_PATH, szFuncName);

		func = f;
		pNextCommand = g_pCommandListHead;
		g_pCommandListHead = this;
	}
public:
	TCHAR name[MAX_PATH];
	MenuFunc func;
	//next command
	CCommand* pNextCommand;
};

//每一个测试定义为：测试类型和功能
//测试类型最好同类的使用同一种类型
//不同的测试功能定义不同的func
//Category为一任意字符串，不需要用C++等字符串表示，如无双引号的Test
//func为一个函数，此函数无返回值，但带有一个字符串参数

//example:
//void TestCommand(const TCHAR* szParam)
//{
//	::MessageBox(NULL, _T("test"), _T("test"),MB_OK);
//}
//COMMAND_DEFINE(Test, TestCommand);

#define COMMAND_DEFINE(Category, func) \
	extern void func(const TCHAR* szParam);\
	CCommand g_##Category##func(_T(#Category), _T(#func), func)

void LogStringMessage(const TCHAR* szMsg);
void LogMessage(const TCHAR* _Format,...);

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)