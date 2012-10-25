#include "stdafx.h"
#include "Common.h"
#include <Shellapi.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <Audioclient.h>
#define REFTIMES_PER_SEC  10000000
void TestVolumeUpAndDown(const TCHAR* szParam)
{
	LogStringMessage(__WFILE__);
	CComQIPtr<IMMDeviceEnumerator> deviceEnumerator;
	HRESULT hr = deviceEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER);
	if (FAILED(hr))
	{
		TheLogger.Error(L"deviceEnumerator.CoCreateInstance return failed. hr = %x", hr);
		return;
	}
	CComQIPtr<IMMDevice> defaultDevice;
	hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
	if (FAILED(hr))
	{
		TheLogger.Error(L"deviceEnumerator->GetDefaultAudioEndpoint return failed. hr = %x", hr);
		return;
	}

	CComQIPtr<IAudioClient> pAudioClient;
	hr = defaultDevice->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&pAudioClient);
	if (FAILED(hr))
	{
		TheLogger.Error(L"defaultDevice->Activate return failed. hr = %x", hr);
		return;
	}

	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
	WAVEFORMATEX *pwfx = NULL;
	hr = pAudioClient->GetMixFormat(&pwfx);
	if (FAILED(hr))
	{
		TheLogger.Error(L"pAudioClient->GetMixFormat return failed. hr = %x", hr);
		return;
	}
	hr = pAudioClient->Initialize( 
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 
        hnsRequestedDuration, 
        hnsRequestedDuration, 
        pwfx, 
        NULL);
	if (FAILED(hr))
	{
		TheLogger.Error(L"pAudioClient->Initialize return failed. hr = %x", hr);
		return;
	}

	CComQIPtr<ISimpleAudioVolume> pSimpleAudioVolume;
	hr = pAudioClient->GetService(__uuidof(ISimpleAudioVolume), (LPVOID *)&pSimpleAudioVolume);
	if (FAILED(hr))
	{
		TheLogger.Error(L"pAudioClient->GetService return failed. hr = %x", hr);
		return;
	}
	hr = pSimpleAudioVolume->SetMasterVolume(0.5, NULL);
	if (FAILED(hr))
	{
		TheLogger.Error(L"pAudioClient->SetMasterVolume return failed. hr = %x", hr);
		return;
	}
	hr = pSimpleAudioVolume->SetMute(TRUE, NULL);
	if (FAILED(hr))
	{
		TheLogger.Error(L"pAudioClient->SetMute return failed. hr = %x", hr);
		return;
	}
}
COMMAND_DEFINE(Test, TestVolumeUpAndDown);