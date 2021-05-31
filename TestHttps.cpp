#include "stdafx.h"
#include "Common.h"
#include "TestItmes/httpsHeader/OcrVerify.h"

void TestHttpsRequest(const TCHAR* szParam)
{
	LogStringMessage(__WFILE__);
	if (RequestAccessRight("lOfP5emffSO0Wq7Hxd0Kd7XIBLf8wCri", "LMhc4AzhypKofTbfNDXyYLRp0iKKF8gL"))
	{
		LogStringMessage(L"Request successfully");

	}
	else
	{
		LogStringMessage(L"Request failed");
	}
}

void TestHttpsVerify(const TCHAR* szParam)
{
	LogStringMessage(__WFILE__);
	if (HasValidAccessRight())
	{
		LogStringMessage(L"Has valid access right");
	}
	else
	{
		LogStringMessage(L"Has no access right");
	}
}

void TestHttpsGetSystemModel(const TCHAR* szParam)
{
	LogStringMessage(__WFILE__);
	CString value;
	if (QuerySystemModel(value))
	{
		const TCHAR* szValue = value;
		LogMessage(L"System model = %s", szValue);
	}
	else
	{
		LogStringMessage(L"Query System modle failed");
	}
}

COMMAND_DEFINE(Test, TestHttpsRequest); 
COMMAND_DEFINE(Test, TestHttpsVerify); 
COMMAND_DEFINE(Test, TestHttpsGetSystemModel);
