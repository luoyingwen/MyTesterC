#include "stdafx.h"
#include "Common.h"

void TestTemplate(const TCHAR* szParam)
{
	LogStringMessage(__WFILE__);
}
COMMAND_DEFINE(Test, TestTemplate);