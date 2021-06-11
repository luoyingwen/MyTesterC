// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Windows specific implementation of VideoCaptureDevice.
// MediaFoundation is used for capturing. MediaFoundation provides its own
// threads for capturing.

#ifndef MEDIA_CAPTURE_VIDEO_WIN_VIDEO_CAPTURE_DEVICE_MF_WIN_H_
#define MEDIA_CAPTURE_VIDEO_WIN_VIDEO_CAPTURE_DEVICE_MF_WIN_H_

#include <mfcaptureengine.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <stdint.h>
#include <strmif.h>
#include <wrl/client.h>

#include <vector>
#include "camerabase.h"
#include "dxgi_device_manager.h"



interface IMFSourceReader;

namespace base {
class Location;
}  // namespace base

namespace media {

class MFVideoCallback;

class VideoCaptureDeviceMFWin {
 public:
  static bool GetPixelFormatFromMFSourceMediaSubtype(const GUID& guid,
                                                     bool use_hardware_format,
                                                     VideoPixelFormat* format);
  static VideoCaptureControlSupport GetControlSupport(
      Microsoft::WRL::ComPtr<IMFMediaSource> source);

  explicit VideoCaptureDeviceMFWin(
      const VideoCaptureDeviceDescriptor& device_descriptor,
      Microsoft::WRL::ComPtr<IMFMediaSource> source,
      DXGIDeviceManager* dxgi_device_manager);
  explicit VideoCaptureDeviceMFWin(
      const VideoCaptureDeviceDescriptor& device_descriptor,
      Microsoft::WRL::ComPtr<IMFMediaSource> source,
      DXGIDeviceManager* dxgi_device_manager,
      Microsoft::WRL::ComPtr<IMFCaptureEngine> engine);

  ~VideoCaptureDeviceMFWin();

  // Opens the device driver for this device.
  bool Init();

  // VideoCaptureDevice implementation.
  void AllocateAndStart(
      const VideoCaptureParams& params,
      std::unique_ptr<Client> client);
  void StopAndDeAllocate();

  // Captured new video data.
  void OnIncomingCapturedData(IMFMediaBuffer* buffer,
                              long reference_time,
                              long timestamp);
  void OnFrameDropped(VideoCaptureFrameDropReason reason);
  void OnEvent(IMFMediaEvent* media_event);


  bool get_use_photo_stream_to_take_photo_for_testing() {
    return !photo_capabilities_.empty();
  }


  void set_max_retry_count_for_testing(int max_retry_count) {
    max_retry_count_ = max_retry_count;
  }

  void set_retry_delay_in_ms_for_testing(int retry_delay_in_ms) {
    retry_delay_in_ms_ = retry_delay_in_ms;
  }

  void set_dxgi_device_manager_for_testing(
      DXGIDeviceManager* dxgi_device_manager) {
    dxgi_device_manager_ = std::move(dxgi_device_manager);
  }

  int camera_rotation() const { return camera_rotation_; }

 private:
  HRESULT ExecuteHresultCallbackWithRetries(
      base::RepeatingCallback<HRESULT()> callback,
      MediaFoundationFunctionRequiringRetry which_function);
  HRESULT GetDeviceStreamCount(IMFCaptureSource* source, DWORD* count);
  HRESULT GetDeviceStreamCategory(
      IMFCaptureSource* source,
      DWORD stream_index,
      MF_CAPTURE_ENGINE_STREAM_CATEGORY* stream_category);
  HRESULT GetAvailableDeviceMediaType(IMFCaptureSource* source,
                                      DWORD stream_index,
                                      DWORD media_type_index,
                                      IMFMediaType** type);

  HRESULT FillCapabilities(IMFCaptureSource* source,
                           bool photo,
                           CapabilityList* capabilities);
  void OnError(VideoCaptureError error,
               HRESULT hr);
  void OnError(VideoCaptureError error,
               const char* message);
  void SendOnStartedIfNotYetSent();
  HRESULT WaitOnCaptureEvent(GUID capture_event_guid);
  HRESULT DeliverTextureToClient(ID3D11Texture2D* texture,
                                 long reference_time,
                                 long timestamp)
      EXCLUSIVE_LOCKS_REQUIRED(lock_);
  void OnIncomingCapturedDataInternal(
      IMFMediaBuffer* buffer,
      long reference_time,
      long timestamp,
      VideoCaptureFrameDropReason& frame_drop_reason);

  VideoFacingMode facing_mode_;
  MFVideoCallback* video_callback_;
  bool is_initialized_;
  int max_retry_count_;
  int retry_delay_in_ms_;

  // Guards the below variables from concurrent access between methods running
  // on |sequence_checker_| and calls to OnIncomingCapturedData() and OnEvent()
  // made by MediaFoundation on threads outside of our control.
  base::Lock lock_;

  std::unique_ptr<Client> client_;
  const Microsoft::WRL::ComPtr<IMFMediaSource> source_;
  Microsoft::WRL::ComPtr<IAMCameraControl> camera_control_;
  Microsoft::WRL::ComPtr<IAMVideoProcAmp> video_control_;
  Microsoft::WRL::ComPtr<IMFCaptureEngine> engine_;
  std::unique_ptr<CapabilityWin> selected_video_capability_;
  CapabilityList photo_capabilities_;
  std::unique_ptr<CapabilityWin> selected_photo_capability_;
  bool is_started_;
  bool has_sent_on_started_to_client_;
  // These flags keep the manual/auto mode between cycles of SetPhotoOptions().
  bool exposure_mode_manual_;
  bool focus_mode_manual_;
  bool white_balance_mode_manual_;
  //base::WaitableEvent capture_initialize_;
  //base::WaitableEvent capture_error_;
  DXGIDeviceManager* dxgi_device_manager_;
  int camera_rotation_;
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_WIN_VIDEO_CAPTURE_DEVICE_MF_WIN_H_
