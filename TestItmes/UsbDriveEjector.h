#pragma once

#include <Setupapi.h>
#include <winioctl.h>
#include <winioctl.h>
#include <cfgmgr32.h>

#pragma comment(lib, "Setupapi.lib")

const int MAX_TRY_COUNT = 3;

class UsbDriveEjector
{
private:
	const wchar_t _driveNumber;
public:
	UsbDriveEjector(wchar_t driveNumber):_driveNumber(driveNumber)
	{
	}
public:
	bool Eject()
	{
		wchar_t szRootPath[] = L"X:\\";   // "X:\"  -> for GetDriveType
		szRootPath[0] = _driveNumber;

		wchar_t szDevicePath[] = L"X:";   // "X:"   -> for QueryDosDevice
		szDevicePath[0] = _driveNumber;

		wchar_t szVolumeAccessPath[] = L"\\\\.\\X:";   // "\\.\X:"  -> to open the volume
		szVolumeAccessPath[4] = _driveNumber;

		long deviceNumber = -1;
		if (!GetDeviceNumber(szVolumeAccessPath, deviceNumber))
		{
			TheLogger.Error(L"UsbDriveEjector::GetDeviceNumber, return failed.");
			return false;
		}

		// get the drive type which is required to match the device numbers correctely
		UINT driveType = GetDriveType(szRootPath);
		TheLogger.Debug(L"GetDriveType=%x", driveType);
		if (driveType != DRIVE_REMOVABLE && driveType != DRIVE_FIXED)
		{
			TheLogger.Error(L"Invalid Drive Type. Type=%d.", driveType);
			return false;
		}

		// get the dos device name (like \device\floppy0) to decide if it's a floppy or not - who knows a better way?
		wchar_t szDosDeviceName[MAX_PATH];
		long res = QueryDosDevice(szDevicePath, szDosDeviceName, MAX_PATH);
		if ( !res ) {
			TheLogger.Error(L"QueryDosDevice return failed. devicePath=%s Error=%d", szDevicePath, ::GetLastError());
			return false;
		}
		
		bool IsFloppy = (wcsstr(szDosDeviceName, L"\\Floppy") != NULL); // who knows a better way?
		if (IsFloppy)
		{
			TheLogger.Error(L"IsFloppy. szDosDeviceName=%s", szDosDeviceName);
			return false;
		}

		DEVINST DevInst = GetDrivesDevInstByDeviceNumber(deviceNumber);
		if ( DevInst == 0 ) {
			return false;
		}

		PNP_VETO_TYPE VetoType = PNP_VetoTypeUnknown; 
		wchar_t VetoNameW[MAX_PATH];
		VetoNameW[0] = 0;
		bool bSuccess = false;

		// get drives's parent, e.g. the USB bridge, the SATA port, an IDE channel with two drives!
		DEVINST DevInstParent = 0;
		res = CM_Get_Parent(&DevInstParent, DevInst, 0); 

		for ( int tries = 0; tries < MAX_TRY_COUNT; tries++ ) { // sometimes we need some tries...

			VetoNameW[0] = 0;
			res = CM_Request_Device_EjectW(DevInstParent, &VetoType, VetoNameW, MAX_PATH, 0);

			bSuccess = (res==CR_SUCCESS && VetoType==PNP_VetoTypeUnknown);
			if ( bSuccess )  { 
				break;
			}

			Sleep(500); // required to give the next tries a chance!
		}

		if ( bSuccess ) {
			TheLogger.Debug(L"Eject Success\n\n");
			return true;
		}
		else
		{
			TheLogger.Error(L"Eject failed Result=0x%2X", res);
			if ( VetoNameW[0] ) {
				TheLogger.Error(L"Eject failed VetoName=%s)", VetoNameW);
			}		
			return false;
		}
	}
private:
	bool GetDeviceNumber(LPCWSTR lpFileName, long& deviceNumber)const
	{
		deviceNumber = -1;

		// open the storage volume
		HANDLE hVolume = CreateFile(lpFileName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
		if (hVolume == INVALID_HANDLE_VALUE)
		{
			TheLogger.Error(L"CreateFile %s return failed. Error=%d", lpFileName, ::GetLastError());
			return false;
		}

		// get the volume's device number
		STORAGE_DEVICE_NUMBER sdn;
		DWORD dwBytesReturned = 0;
		long res = DeviceIoControl(hVolume, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &dwBytesReturned, NULL);
		if ( res )
		{
			deviceNumber = sdn.DeviceNumber;
		}
		else
		{
			TheLogger.Error(L"DeviceIoControl return failed. Error=%d", ::GetLastError());
		}

		///test begin
		STORAGE_DEVICE_DESCRIPTOR deviceDesc;
		GetDriveProperty(hVolume, &deviceDesc);

		CloseHandle(hVolume);
		return deviceNumber != -1;
	}

	BOOL GetDriveProperty(HANDLE hDevice, PSTORAGE_DEVICE_DESCRIPTOR pDevDesc)const
	{
		STORAGE_PROPERTY_QUERY Query; // 查询输入参数
		DWORD dwOutBytes;    // IOCTL输出数据长度
		BOOL bResult;     // IOCTL返回值

		// 指定查询方式
		Query.PropertyId = StorageDeviceProperty;
		Query.QueryType = PropertyStandardQuery;

		// 用IOCTL_STORAGE_QUERY_PROPERTY取设备属性信息
		bResult = ::DeviceIoControl(hDevice,  // 设备句柄
			IOCTL_STORAGE_QUERY_PROPERTY,  // 取设备属性信息
			&Query, sizeof(STORAGE_PROPERTY_QUERY), // 输入数据缓冲区
			pDevDesc, pDevDesc->Size,  // 输出数据缓冲区
			&dwOutBytes,    // 输出数据长度
			(LPOVERLAPPED)NULL);   // 用同步I/O 
		char* szSerialNum = NULL;
		 if (pDevDesc->SerialNumberOffset != 0)
		 {
            szSerialNum = (PCHAR) ((PBYTE)pDevDesc + pDevDesc->SerialNumberOffset);			
		 }

		 char* szProductId = NULL;
		 if (pDevDesc->ProductIdOffset != 0)
		 {
            szProductId = (PCHAR) ((PBYTE)pDevDesc + pDevDesc->ProductIdOffset);			
		 }
		 char* szVendorIdOffset = NULL;
		 if (pDevDesc->VendorIdOffset != 0)
		 {
            szVendorIdOffset = (PCHAR) ((PBYTE)pDevDesc + pDevDesc->VendorIdOffset);			
		 }

		 char* szProductRevisionOffset = NULL;
		 if (pDevDesc->ProductRevisionOffset != 0)
		 {
            szProductRevisionOffset = (PCHAR) ((PBYTE)pDevDesc + pDevDesc->ProductRevisionOffset);			
		 }
		return bResult;
	}
	
	//----------------------------------------------------------------------
	// returns the device instance handle of a storage volume or 0 on error
	//----------------------------------------------------------------------
	DEVINST GetDrivesDevInstByDeviceNumber(long DeviceNumber)
	{
		DEVINST devInstance = 0;

		GUID* guid = (GUID*)&GUID_DEVINTERFACE_DISK;

		// Get device interface info set handle for all devices attached to system
		HDEVINFO hDevInfo = SetupDiGetClassDevs(guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (hDevInfo == INVALID_HANDLE_VALUE)
		{
			TheLogger.Error(L"SetupDiGetClassDevs return failed. Error=%d", ::GetLastError());
			return 0;
		}

		// Retrieve a context structure for a device interface of a device information set
		DWORD dwIndex = 0;

		BYTE Buf[1024];
		PSP_DEVICE_INTERFACE_DETAIL_DATA pspdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)Buf;
		SP_DEVICE_INTERFACE_DATA         spdid;
		SP_DEVINFO_DATA                  spdd;
	
		spdid.cbSize = sizeof(spdid);

		while ( true )	{
			long res = SetupDiEnumDeviceInterfaces(hDevInfo, NULL, guid, dwIndex, &spdid);
			if ( !res ) {
				break;
			}

			DWORD dwSize = 0;
			SetupDiGetDeviceInterfaceDetail(hDevInfo, &spdid, NULL, 0, &dwSize, NULL); // check the buffer size

			if (dwSize != 0 && dwSize <= sizeof(Buf) ) {

				pspdidd->cbSize = sizeof(*pspdidd); // 5 Bytes!

				ZeroMemory(&spdd, sizeof(spdd));
				spdd.cbSize = sizeof(spdd);

				res = SetupDiGetDeviceInterfaceDetail(hDevInfo, &spdid, pspdidd, dwSize, &dwSize, &spdd);
				if ( res ) {
					long targetDeviceNumber = 0;
					if (GetDeviceNumber(pspdidd->DevicePath, targetDeviceNumber) && targetDeviceNumber == DeviceNumber)
					{
						TheLogger.Debug(L"Found Device. DevicePath=%s", pspdidd->DevicePath);
						devInstance = spdd.DevInst;
						break;
					}
				}
			}
			dwIndex++;
		}

		SetupDiDestroyDeviceInfoList(hDevInfo);
		return devInstance;
	}
};