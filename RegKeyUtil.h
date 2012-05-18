#pragma once

class RegKeyUtil
{
public:
	static bool DeleteRegValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR pszValueName)
	{
		CRegKey regKey;
		LONG lRet = regKey.Create(hKeyParent, lpszKeyName, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, NULL);
		if(lRet == ERROR_SUCCESS)
		{
			lRet = regKey.DeleteValue(pszValueName);
			if(lRet != ERROR_SUCCESS)
			{
				TheLogger.Error(L"regKey.DeleteValue for %s failed, code = %x.", pszValueName, lRet);
			}
		}
		else
		{
			TheLogger.Error(L"Create RegKey : %s failed", lpszKeyName);
		}

		return lRet == ERROR_SUCCESS;
	}

	static bool SetStringValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR pszValueName, LPCTSTR pszValue)
	{
		CRegKey regKey;
		LONG lRet = regKey.Create(hKeyParent, lpszKeyName, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, NULL);
		if(lRet == ERROR_SUCCESS)
		{
			lRet = regKey.SetStringValue(pszValueName, pszValue);
			if(lRet != ERROR_SUCCESS)
			{
				TheLogger.Error(L"regKey.SetStringValue for %s : %s failed, code = %x.", pszValueName, pszValue, lRet);
			}
		}
		else
		{
			TheLogger.Error(L"Create RegKey : %s failed", lpszKeyName);
		}
		return lRet == ERROR_SUCCESS;
	}

	static bool QueryStringValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR pszValueName, CString& value)
	{
		CRegKey regKey;
		LONG lRet = regKey.Open(hKeyParent, lpszKeyName, KEY_READ);
		if(lRet == ERROR_SUCCESS)
		{
			TCHAR buffer[MAX_PATH + MAX_PATH];
			DWORD dwLen = sizeof(buffer)/sizeof(TCHAR);
			lRet = regKey.QueryStringValue(pszValueName, buffer, &dwLen);
			if(lRet == ERROR_SUCCESS)
			{
				value = buffer;
			}
			else
			{
				TheLogger.Warn(L"regKey.QueryStringValue for %s failed, code = %x.", pszValueName, lRet);
			}
		}
		else
		{
			TheLogger.Error(L"Open RegKey : %s failed", lpszKeyName);
		}
		return lRet == ERROR_SUCCESS;
	}

	static bool SetDWORDValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR pszValueName, DWORD value)
	{
		CRegKey regKey;
		LONG lRet = regKey.Create(hKeyParent, lpszKeyName, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, NULL);
		if(lRet == ERROR_SUCCESS)
		{
			lRet = regKey.SetDWORDValue(pszValueName, value);
			if(lRet != ERROR_SUCCESS)
			{
				TheLogger.Error(L"regKey.SetDWORDValue for %s : %x failed, code = %x.", pszValueName, value, lRet);
			}

		}
		return lRet == ERROR_SUCCESS;
	}

	static bool QueryDWORDValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR pszValueName, DWORD& value)
	{
		CRegKey regKey;
		LONG lRet = regKey.Open(hKeyParent, lpszKeyName, KEY_READ);
		if(lRet == ERROR_SUCCESS)
		{
			DWORD dwReturn = 0;
			lRet = regKey.QueryDWORDValue(pszValueName, dwReturn);
			if(lRet == ERROR_SUCCESS)
			{
				value = dwReturn;
			}
			else
			{
				TheLogger.Warn(L"regKey.QueryDWORDValue for %s failed, code = %x.", pszValueName, lRet);
			}
		}
		else
		{
			TheLogger.Error(L"Open RegKey : %s failed", lpszKeyName);
		}
		return lRet == ERROR_SUCCESS;
	}

	static bool SetBinaryValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR pszValueName, const void* pData, ULONG nBytes)
	{
		CRegKey regKey;
		LONG lRet = regKey.Create(hKeyParent, lpszKeyName, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, NULL);
		if(lRet == ERROR_SUCCESS)
		{
			lRet = regKey.SetBinaryValue(pszValueName, pData, nBytes);
			if(lRet != ERROR_SUCCESS)
			{
				TheLogger.Error(L"regKey.SetBinaryValue for %s failed, code = %x.", pszValueName, lRet);
			}
		}
		return lRet == ERROR_SUCCESS;
	}

	static bool QueryBinaryValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR pszValueName, void* pValue, ULONG* pnBytes)
	{
		CRegKey regKey;
		LONG lRet = regKey.Open(hKeyParent, lpszKeyName, KEY_READ);
		if(lRet == ERROR_SUCCESS)
		{
			DWORD dwReturn = 0;
			lRet = regKey.QueryBinaryValue(pszValueName, pValue, pnBytes);
			if(lRet != ERROR_SUCCESS)
			{
				TheLogger.Warn(L"regKey.QueryBinaryValue for %s failed, code = %x.", pszValueName, lRet);
			}
		}
		else
		{
			TheLogger.Error(L"Open RegKey : %s failed", lpszKeyName);
		}
		return lRet == ERROR_SUCCESS;
	}

	static bool RecurseDeleteRegKey(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR lpszSubKey)
	{
		CRegKey rkParent;
		LONG lRet = rkParent.Create(hKeyParent, lpszKeyName, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, NULL);
		if(lRet == ERROR_SUCCESS)
		{
			lRet = rkParent.RecurseDeleteKey(lpszSubKey);
			if(lRet != ERROR_SUCCESS)
			{
				TheLogger.Error(L"RecurseDeleteKey %s of %s failed.", lpszSubKey, lpszKeyName);
			}
		}
		else
		{
			TheLogger.Error(L"Create key: %s failed", lpszKeyName);
		}

		return lRet == ERROR_SUCCESS;
	}
};