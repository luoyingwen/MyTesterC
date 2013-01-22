#include "stdafx.h"
#include "Common.h"
#include <Ntddscsi.h>
#include <Setupapi.h>
#include <winioctl.h>
#include <winioctl.h>

#define ATA_BUF_SIZE 256
typedef USHORT ATA_DATA_TYPE;
typedef struct
{
   ATA_PASS_THROUGH_EX apt;
   ATA_DATA_TYPE dataBuf[ATA_BUF_SIZE];
}ATA_PASS_THROUGH_WITH_BUFFERS, *PATA_PASS_THROUGH_WITH_BUFFERS;
	static wchar_t GetSystemDrive()
	{
		PWSTR system32Path;
		HRESULT hr = SHGetKnownFolderPath(FOLDERID_System, 0, NULL, &system32Path);
		if (hr != S_OK)
		{
			TheLogger.Error(L"SHGetKnownFolderPath return faled. error=%x", hr);
			return 'C';
		}
		else
		{
			wchar_t drive = system32Path[0];
			::CoTaskMemFree(system32Path);
			return drive;
		}
	}


	static bool IsSsdDisk()
	{
		//////////////////////////////////////////////////////////////////////////////////////////
		// reference
		// http://blogs.technet.com/b/filecab/archive/2009/11/25/windows-7-disk-defragmenter-user-interface-overview.aspx#How2
		
		HANDLE hDevice; 
		BOOL bResult; 
		DWORD dwOutBytes;

		// Query Device Number
		wchar_t szVolumeAccessPath[] = L"\\\\?\\X:";   // "\\.\X:"  -> to open the volume
		szVolumeAccessPath[4] = GetSystemDrive();
		HANDLE hVolume = ::CreateFile(szVolumeAccessPath, GENERIC_READ, FILE_SHARE_READ| FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);   
		if (hVolume == INVALID_HANDLE_VALUE)
		{
			TheLogger.Error(L"CreateFile %s error %d", szVolumeAccessPath, ::GetLastError());
			return false;
		}
		
		STORAGE_DEVICE_NUMBER sdn;
		bResult = DeviceIoControl(hVolume,
			IOCTL_STORAGE_GET_DEVICE_NUMBER, // dwIoControlCode
			NULL,  0,
			(LPVOID)&sdn, (DWORD)sizeof(STORAGE_DEVICE_NUMBER),
			(LPDWORD)&dwOutBytes, NULL);
		CloseHandle(hVolume);

		if (!bResult || sdn.DeviceNumber < 0)
		{
			TheLogger.Error(L"%s IOCTL_STORAGE_GET_DEVICE_NUMBER error %d", szVolumeAccessPath, GetLastError());
			return false;
		}

		//////////////////////////////////////////////////////
		TCHAR szPhysicalDisk[MAX_PATH];
		wsprintf(szPhysicalDisk, L"\\\\.\\PhysicalDrive%d", sdn.DeviceNumber);

		hDevice = ::CreateFile(szPhysicalDisk,
			GENERIC_READ| GENERIC_WRITE,
			FILE_SHARE_READ, NULL,
			OPEN_EXISTING, 0, NULL);       
		if (hDevice == INVALID_HANDLE_VALUE)
		{
			TheLogger.Error(L"%s IsSsdDisk error %d", szPhysicalDisk, GetLastError());
			return false;
		}

		////////////////////////////////////////
		//no seek penalty
		STORAGE_PROPERTY_QUERY   Query;
		Query.PropertyId = StorageDeviceSeekPenaltyProperty;  
		Query.QueryType = PropertyStandardQuery;  

		PDEVICE_SEEK_PENALTY_DESCRIPTOR desc = (PDEVICE_SEEK_PENALTY_DESCRIPTOR)malloc(sizeof(DEVICE_SEEK_PENALTY_DESCRIPTOR));
		bResult = ::DeviceIoControl(hDevice,    
			IOCTL_STORAGE_QUERY_PROPERTY ,         
			&Query, sizeof(STORAGE_PROPERTY_QUERY),                              
			desc, sizeof(DEVICE_SEEK_PENALTY_DESCRIPTOR), 
			&dwOutBytes, NULL);                 

		if (bResult)
		{
			if (desc->IncursSeekPenalty == false)
			{
				TheLogger.Debug(L"%s IsSsdDisk result TRUE, IncursSeekPenalty = false", szPhysicalDisk);
				::CloseHandle(hDevice);
				return true;
			}
		}

		// ///////////////////////////////////////////
		// // is_ATA_nominal_disc_rate
		// need more test case.

		// PATA_PASS_THROUGH_EX apt = (PATA_PASS_THROUGH_EX)malloc(sizeof(ATA_PASS_THROUGH_WITH_BUFFERS));
		// memset(apt, 0, sizeof(ATA_PASS_THROUGH_WITH_BUFFERS));
		// apt->Length = sizeof(ATA_PASS_THROUGH_EX);
		// apt->AtaFlags = ATA_FLAGS_DATA_IN;
		// apt->DataTransferLength = ATA_BUF_SIZE * sizeof(ATA_DATA_TYPE);
		// apt->DataBufferOffset = FIELD_OFFSET(ATA_PASS_THROUGH_WITH_BUFFERS, dataBuf);;
		// apt->TimeOutValue = 3;//second
		// apt->CurrentTaskFile[6] = 0xEC;//http://www.techtalkz.com/microsoft-device-drivers/485379-ioctl_ata_pass_through.html

		// bResult = ::DeviceIoControl(hDevice,
		//     IOCTL_ATA_PASS_THROUGH, 
		//     apt, sizeof(ATA_PASS_THROUGH_WITH_BUFFERS), 
		//     apt, sizeof(ATA_PASS_THROUGH_WITH_BUFFERS),
		//     &dwOutBytes, NULL); 


		// if (bResult)
		// {
		//     PATA_PASS_THROUGH_WITH_BUFFERS aptBuf = (PATA_PASS_THROUGH_WITH_BUFFERS)apt;
		//     if ( aptBuf->dataBuf[217] == 1 )
		//     {
		//TheLogger.Debug(L"%s IsSsdDisk result TRUE. dataBuf[217] == 1", szPhysicalDisk);
		//         ::CloseHandle(hDevice);
		//         return true;
		//     }
		// }

		TheLogger.Debug(L"%s IsSsdDisk result FALSE", szPhysicalDisk);
		::CloseHandle(hDevice);
		return false;
	}

void TestCheckSSD(const TCHAR* szParam)
{
	IsSsdDisk();
}

COMMAND_DEFINE(Test, TestCheckSSD);