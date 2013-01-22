#pragma once

class Prefetcher
{
public:
	virtual DWORD ExportData(DWORD dwProcessId) = 0;
	virtual DWORD Load() = 0;
};