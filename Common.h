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

//ÿһ�����Զ���Ϊ���������ͺ͹���
//�����������ͬ���ʹ��ͬһ������
//��ͬ�Ĳ��Թ��ܶ��岻ͬ��func
//CategoryΪһ�����ַ���������Ҫ��C++���ַ�����ʾ������˫���ŵ�Test
//funcΪһ���������˺����޷���ֵ��������һ���ַ�������

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