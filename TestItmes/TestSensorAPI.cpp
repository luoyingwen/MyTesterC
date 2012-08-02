#include "stdafx.h"
#include "Common.h"
#include <sensorsapi.h>
#include <sensors.h>
#pragma comment(lib, "Sensorsapi.lib")
#pragma comment(lib, "PortableDeviceGuids.lib")
void TestSensorManager(const TCHAR* szParam)
{
	LogStringMessage(L"Hello world.");
	// Create the sensor manager.
	CComQIPtr<ISensorManager> pSensorManager;
	CComQIPtr<ISensorCollection> pSensorColl;
	CComQIPtr<ISensor> pSensor;
	HRESULT hr = pSensorManager.CoCreateInstance(CLSID_SensorManager, NULL, CLSCTX_INPROC_SERVER);

	if(pSensorManager == NULL)
	{
		LogMessage(L"pSensorManager == NULL");
		return;
	}


	hr = pSensorManager->GetSensorsByCategory(SENSOR_CATEGORY_ALL, &pSensorColl);
  
	if(SUCCEEDED(hr))
	{
		ULONG ulCount = 0;

		// Verify that the collection contains
		// at least one sensor.
		hr = pSensorColl->GetCount(&ulCount);

		LogMessage(L"\nSensor count =%d.\n", ulCount);
		if(SUCCEEDED(hr))
		{
			if(ulCount < 1)
			{
				LogStringMessage(L"\nNo sensors of the requested category.\n");
				hr = E_UNEXPECTED;
			}
		}
	}
	else
	{
		LogMessage(L"SensorColl == NULL");
	}

	if(SUCCEEDED(hr))
	{
		// Get the first available sensor.
		hr = pSensorColl->GetAt(0, &pSensor);
		if (pSensor != NULL)
		{
			LogMessage(L"Query first Sensor successfully");
		}
	}
}
