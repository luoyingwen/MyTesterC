#include "stdafx.h"
#include "Common.h"
#include "../RegKeyUtil.h"

#include "../../../work/Master20130111/AriesNew/Alpha/Src/SpeedWatcherService/PrefetcherMgr.h"


void TestStartPrefetchThread(const TCHAR* szParam)
{
	PrefetcherMgr::StartPrefetchThread();
}

COMMAND_DEFINE(Test, TestStartPrefetchThread);
