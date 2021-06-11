// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "video_capture_device_mf_win.h"

#include <d3d11_4.h>
#include <mfapi.h>
#include <mferror.h>
#include <stddef.h>
#include <wincodec.h>

#include <memory>
#include <thread>
#include <utility>

using Microsoft::WRL::ComPtr;

namespace media {

namespace {
// Locks the given buffer using the fastest supported method when constructed,
// and automatically unlocks the buffer when destroyed.
class ScopedBufferLock {
 public:
  explicit ScopedBufferLock(ComPtr<IMFMediaBuffer> buffer)
      : buffer_(std::move(buffer)) {
    if (FAILED(buffer_.As(&buffer_2d_))) {
      LockSlow();
      return;
    }
    // Try lock methods from fastest to slowest: Lock2DSize(), then Lock2D(),
    // then finally LockSlow().
    if (Lock2DSize() || Lock2D()) {
      if (IsContiguous())
        return;
      buffer_2d_->Unlock2D();
    }
    // Fall back to LockSlow() if 2D buffer was unsupported or noncontiguous.
    buffer_2d_ = nullptr;
    LockSlow();
  }

  // Returns whether |buffer_2d_| is contiguous with positive pitch, i.e., the
  // buffer format that the surrounding code expects.
  bool IsContiguous() {
    BOOL is_contiguous;
    return pitch_ > 0 &&
           SUCCEEDED(buffer_2d_->IsContiguousFormat(&is_contiguous)) &&
           is_contiguous &&
           (length_ || SUCCEEDED(buffer_2d_->GetContiguousLength(&length_)));
  }

  bool Lock2DSize() {
    ComPtr<IMF2DBuffer2> buffer_2d_2;
    if (FAILED(buffer_.As(&buffer_2d_2)))
      return false;
    BYTE* data_start;
    return SUCCEEDED(buffer_2d_2->Lock2DSize(MF2DBuffer_LockFlags_Read, &data_,
                                             &pitch_, &data_start, &length_));
  }

  bool Lock2D() { return SUCCEEDED(buffer_2d_->Lock2D(&data_, &pitch_)); }

  void LockSlow() {
    DWORD max_length = 0;
    buffer_->Lock(&data_, &max_length, &length_);
  }

  ~ScopedBufferLock() {
    if (buffer_2d_)
      buffer_2d_->Unlock2D();
    else
      buffer_->Unlock();
  }

  ScopedBufferLock(const ScopedBufferLock&) = delete;
  ScopedBufferLock& operator=(const ScopedBufferLock&) = delete;

  BYTE* data() const { return data_; }
  DWORD length() const { return length_; }

 private:
  ComPtr<IMFMediaBuffer> buffer_;
  ComPtr<IMF2DBuffer> buffer_2d_;
  BYTE* data_ = nullptr;
  DWORD length_ = 0;
  LONG pitch_ = 0;
};
void LogError(HRESULT hr) {
  std::cout << " hr = " << hr;
}

bool GetFrameSizeFromMediaType(IMFMediaType* type, gfx::Size* frame_size) {
  UINT32 width32, height32;
  if (FAILED(MFGetAttributeSize(type, MF_MT_FRAME_SIZE, &width32, &height32)))
    return false;
  frame_size->SetSize(width32, height32);
  return true;
}

bool GetFrameRateFromMediaType(IMFMediaType* type, float* frame_rate) {
  UINT32 numerator, denominator;
  if (FAILED(MFGetAttributeRatio(type, MF_MT_FRAME_RATE, &numerator,
                                 &denominator)) ||
      !denominator) {
    return false;
  }
  *frame_rate = static_cast<float>(numerator) / denominator;
  return true;
}

bool GetFormatFromSourceMediaType(IMFMediaType* source_media_type,
                                  bool photo,
                                  bool use_hardware_format,
                                  VideoCaptureFormat* format) {
  GUID major_type_guid;
  if (FAILED(source_media_type->GetGUID(MF_MT_MAJOR_TYPE, &major_type_guid)) ||
      (major_type_guid != MFMediaType_Image &&
       (photo ||
        !GetFrameRateFromMediaType(source_media_type, &format->frame_rate)))) {
    return false;
  }

  GUID sub_type_guid;
  if (FAILED(source_media_type->GetGUID(MF_MT_SUBTYPE, &sub_type_guid)) ||
      !GetFrameSizeFromMediaType(source_media_type, &format->frame_size) ||
      !VideoCaptureDeviceMFWin::GetPixelFormatFromMFSourceMediaSubtype(
          sub_type_guid, use_hardware_format, &format->pixel_format)) {
    return false;
  }

  return true;
}

HRESULT CopyAttribute(IMFAttributes* source_attributes,
                      IMFAttributes* destination_attributes,
                      const GUID& key) {
  PROPVARIANT var;
  PropVariantInit(&var);
  HRESULT hr = source_attributes->GetItem(key, &var);
  if (FAILED(hr))
    return hr;

  hr = destination_attributes->SetItem(key, var);
  PropVariantClear(&var);
  return hr;
}

struct MediaFormatConfiguration {
  GUID mf_source_media_subtype;
  GUID mf_sink_media_subtype;
  VideoPixelFormat pixel_format;
};

bool GetMediaFormatConfigurationFromMFSourceMediaSubtype(
    const GUID& mf_source_media_subtype,
    bool use_hardware_format,
    MediaFormatConfiguration* media_format_configuration) {
  // Special case handling of the NV12 format when using hardware capture
  // to ensure that captured buffers are passed through without copies
  if (use_hardware_format && mf_source_media_subtype == MFVideoFormat_NV12) {
    *media_format_configuration = {MFVideoFormat_NV12, MFVideoFormat_NV12,
                                   PIXEL_FORMAT_NV12};
    return true;
  }
  static const MediaFormatConfiguration kMediaFormatConfigurationMap[] = {
      // IMFCaptureEngine inevitably performs the video frame decoding itself.
      // This means that the sink must always be set to an uncompressed video
      // format.

      // Since chromium uses I420 at the other end of the pipe, MF known video
      // output formats are always set to I420.
      {MFVideoFormat_I420, MFVideoFormat_I420, PIXEL_FORMAT_I420},
      {MFVideoFormat_YUY2, MFVideoFormat_I420, PIXEL_FORMAT_I420},
      {MFVideoFormat_UYVY, MFVideoFormat_I420, PIXEL_FORMAT_I420},
      {MFVideoFormat_RGB24, MFVideoFormat_I420, PIXEL_FORMAT_I420},
      {MFVideoFormat_RGB32, MFVideoFormat_I420, PIXEL_FORMAT_I420},
      {MFVideoFormat_ARGB32, MFVideoFormat_I420, PIXEL_FORMAT_I420},
      {MFVideoFormat_MJPG, MFVideoFormat_I420, PIXEL_FORMAT_I420},
      {MFVideoFormat_NV12, MFVideoFormat_I420, PIXEL_FORMAT_I420},
      {MFVideoFormat_YV12, MFVideoFormat_I420, PIXEL_FORMAT_I420},

      // Depth cameras use 16-bit uncompressed video formats.
      // We ask IMFCaptureEngine to let the frame pass through, without
      // transcoding, since transcoding would lead to precision loss.
      {kMediaSubTypeY16, kMediaSubTypeY16, PIXEL_FORMAT_Y16},
      {kMediaSubTypeZ16, kMediaSubTypeZ16, PIXEL_FORMAT_Y16},
      {kMediaSubTypeINVZ, kMediaSubTypeINVZ, PIXEL_FORMAT_Y16},
      {MFVideoFormat_D16, MFVideoFormat_D16, PIXEL_FORMAT_Y16},

      // Photo type
      {GUID_ContainerFormatJpeg, GUID_ContainerFormatJpeg, PIXEL_FORMAT_MJPEG}};

  for (const auto& kMediaFormatConfiguration : kMediaFormatConfigurationMap) {
    if (kMediaFormatConfiguration.mf_source_media_subtype ==
        mf_source_media_subtype) {
      *media_format_configuration = kMediaFormatConfiguration;
      return true;
    }
  }

  return false;
}

// Calculate sink subtype based on source subtype. |passthrough| is set when
// sink and source are the same and means that there should be no transcoding
// done by IMFCaptureEngine.
HRESULT GetMFSinkMediaSubtype(IMFMediaType* source_media_type,
                              bool use_hardware_format,
                              GUID* mf_sink_media_subtype,
                              bool* passthrough) {
  GUID source_subtype;
  HRESULT hr = source_media_type->GetGUID(MF_MT_SUBTYPE, &source_subtype);
  if (FAILED(hr))
    return hr;
  MediaFormatConfiguration media_format_configuration;
  if (!GetMediaFormatConfigurationFromMFSourceMediaSubtype(
          source_subtype, use_hardware_format, &media_format_configuration))
    return E_FAIL;
  *mf_sink_media_subtype = media_format_configuration.mf_sink_media_subtype;
  *passthrough =
      (media_format_configuration.mf_sink_media_subtype == source_subtype);
  return S_OK;
}

HRESULT ConvertToPhotoSinkMediaType(IMFMediaType* source_media_type,
                                    IMFMediaType* destination_media_type) {
  HRESULT hr =
      destination_media_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Image);
  if (FAILED(hr))
    return hr;

  bool passthrough = false;
  GUID mf_sink_media_subtype;
  hr = GetMFSinkMediaSubtype(source_media_type, /*use_hardware_format=*/false,
                             &mf_sink_media_subtype, &passthrough);
  if (FAILED(hr))
    return hr;

  hr = destination_media_type->SetGUID(MF_MT_SUBTYPE, mf_sink_media_subtype);
  if (FAILED(hr))
    return hr;

  return CopyAttribute(source_media_type, destination_media_type,
                       MF_MT_FRAME_SIZE);
}

HRESULT ConvertToVideoSinkMediaType(IMFMediaType* source_media_type,
                                    bool use_hardware_format,
                                    IMFMediaType* sink_media_type) {
  HRESULT hr = sink_media_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  if (FAILED(hr))
    return hr;

  bool passthrough = false;
  GUID mf_sink_media_subtype;
  hr = GetMFSinkMediaSubtype(source_media_type, use_hardware_format,
                             &mf_sink_media_subtype, &passthrough);
  if (FAILED(hr))
    return hr;

  hr = sink_media_type->SetGUID(MF_MT_SUBTYPE, mf_sink_media_subtype);
  // Copying attribute values for passthrough mode is redundant, since the
  // format is kept unchanged, and causes AddStream error MF_E_INVALIDMEDIATYPE.
  if (FAILED(hr) || passthrough)
    return hr;

  hr = CopyAttribute(source_media_type, sink_media_type, MF_MT_FRAME_SIZE);
  if (FAILED(hr))
    return hr;

  hr = CopyAttribute(source_media_type, sink_media_type, MF_MT_FRAME_RATE);
  if (FAILED(hr))
    return hr;

  hr = CopyAttribute(source_media_type, sink_media_type,
                     MF_MT_PIXEL_ASPECT_RATIO);
  if (FAILED(hr))
    return hr;

  return CopyAttribute(source_media_type, sink_media_type,
                       MF_MT_INTERLACE_MODE);
}

const CapabilityWin& GetBestMatchedPhotoCapability(
    ComPtr<IMFMediaType> current_media_type,
    gfx::Size requested_size,
    const CapabilityList& capabilities) {
  gfx::Size current_size;
  GetFrameSizeFromMediaType(current_media_type.Get(), &current_size);

  int requested_height = requested_size.height > 0 ? requested_size.height
                                                     : current_size.height;
  int requested_width = requested_size.width > 0 ? requested_size.width
                                                   : current_size.width;

  const CapabilityWin* best_match = &(*capabilities.begin());
  for (const CapabilityWin& capability : capabilities) {
    int height = capability.supported_format.frame_size.height;
    int width = capability.supported_format.frame_size.width;
    int best_height = best_match->supported_format.frame_size.height;
    int best_width = best_match->supported_format.frame_size.width;

    if (std::abs(height - requested_height) <= std::abs(height - best_height) &&
        std::abs(width - requested_width) <= std::abs(width - best_width)) {
      best_match = &capability;
    }
  }
  return *best_match;
}

HRESULT CreateCaptureEngine(IMFCaptureEngine** engine) {
  ComPtr<IMFCaptureEngineClassFactory> capture_engine_class_factory;
  HRESULT hr = CoCreateInstance(CLSID_MFCaptureEngineClassFactory, nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&capture_engine_class_factory));
  if (FAILED(hr))
    return hr;

  return capture_engine_class_factory->CreateInstance(CLSID_MFCaptureEngine,
                                                      IID_PPV_ARGS(engine));
}

bool GetCameraControlSupport(ComPtr<IAMCameraControl> camera_control,
                             CameraControlProperty control_property) {
  long min, max, step, default_value, flags;
  HRESULT hr = camera_control->GetRange(control_property, &min, &max, &step,
                                        &default_value, &flags);
  return SUCCEEDED(hr) && min < max;
}

HRESULT GetTextureFromMFBuffer(IMFMediaBuffer* mf_buffer,
                               ID3D11Texture2D** texture_out) {
  Microsoft::WRL::ComPtr<IMFDXGIBuffer> dxgi_buffer;
  HRESULT hr = mf_buffer->QueryInterface(IID_PPV_ARGS(&dxgi_buffer));
  DLOG_IF_FAILED_WITH_HRESULT("Failed to retrieve IMFDXGIBuffer", hr);

  Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d_texture;
  if (SUCCEEDED(hr)) {
    hr = dxgi_buffer->GetResource(IID_PPV_ARGS(&d3d_texture));
    DLOG_IF_FAILED_WITH_HRESULT("Failed to retrieve ID3D11Texture2D", hr);
  }

  *texture_out = d3d_texture.Detach();
  if (SUCCEEDED(hr)) {
    DCHECK(*texture_out);
  }
  return hr;
}

void GetTextureSizeAndFormat(ID3D11Texture2D* texture,
                             gfx::Size& size,
                             VideoPixelFormat& format) {
  D3D11_TEXTURE2D_DESC desc;
  texture->GetDesc(&desc);
  size.width = (desc.Width);
  size.height = (desc.Height);

  switch (desc.Format) {
    // Only support NV12
    case DXGI_FORMAT_NV12:
      format = PIXEL_FORMAT_NV12;
      break;
    default:
      DLOG(ERROR) << "Unsupported camera DXGI texture format: " << desc.Format;
      format = PIXEL_FORMAT_UNKNOWN;
      break;
  }
}

HRESULT CopyTextureToGpuMemoryBuffer(ID3D11Texture2D* texture,
                                     HANDLE dxgi_handle) {
  Microsoft::WRL::ComPtr<ID3D11Device> texture_device;
  texture->GetDevice(&texture_device);

  Microsoft::WRL::ComPtr<ID3D11Device1> device1;
  HRESULT hr = texture_device.As(&device1);
  if (FAILED(hr)) {
    DLOG(ERROR) << "Failed to get ID3D11Device1: "
                << hr;
    return hr;
  }

  // Open shared resource from GpuMemoryBuffer on source texture D3D11 device
  Microsoft::WRL::ComPtr<ID3D11Texture2D> target_texture;
  hr = device1->OpenSharedResource1(dxgi_handle, IID_PPV_ARGS(&target_texture));
  if (FAILED(hr)) {
    DLOG(ERROR) << "Failed to open shared camera target texture: "
                << hr;
    return hr;
  }

  Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context;
  texture_device->GetImmediateContext(&device_context);

  Microsoft::WRL::ComPtr<IDXGIKeyedMutex> keyed_mutex;
  hr = target_texture.As(&keyed_mutex);
  DCHECK(SUCCEEDED(hr));

  keyed_mutex->AcquireSync(0, INFINITE);
  device_context->CopySubresourceRegion(target_texture.Get(), 0, 0, 0, 0,
                                        texture, 0, nullptr);
  keyed_mutex->ReleaseSync(0);

  // Need to flush context to ensure that other devices receive updated contents
  // of shared resource
  device_context->Flush();

  return S_OK;
}

}  // namespace

class MFVideoCallback final
    : 
      public IMFCaptureEngineOnSampleCallback,
      public IMFCaptureEngineOnEventCallback {
 public:
  MFVideoCallback(VideoCaptureDeviceMFWin* observer) : observer_(observer) {}

  IFACEMETHODIMP QueryInterface(REFIID riid, void** object) override {
    HRESULT hr = E_NOINTERFACE;
    if (riid == IID_IUnknown) {
      *object = this;
      hr = S_OK;
    } else if (riid == IID_IMFCaptureEngineOnSampleCallback) {
      *object = static_cast<IMFCaptureEngineOnSampleCallback*>(this);
      hr = S_OK;
    } else if (riid == IID_IMFCaptureEngineOnEventCallback) {
      *object = static_cast<IMFCaptureEngineOnEventCallback*>(this);
      hr = S_OK;
    }
    if (SUCCEEDED(hr))
      AddRef();

    return hr;
  }

  IFACEMETHODIMP_(ULONG) AddRef() override {
    //base::RefCountedThreadSafe<MFVideoCallback>::AddRef();
    return 1U;
  }

  IFACEMETHODIMP_(ULONG) Release() override {
    //base::RefCountedThreadSafe<MFVideoCallback>::Release();
    return 1U;
  }

  IFACEMETHODIMP OnEvent(IMFMediaEvent* media_event) override {
    base::AutoLock lock(lock_);
    if (!observer_) {
      return S_OK;
    }
    observer_->OnEvent(media_event);
    return S_OK;
  }

  IFACEMETHODIMP OnSample(IMFSample* sample) override {
    base::AutoLock lock(lock_);
    if (!observer_) {
      return S_OK;
    }
    if (!sample) {
      observer_->OnFrameDropped(
          VideoCaptureFrameDropReason::kWinMediaFoundationReceivedSampleIsNull);
      return S_OK;
    }

    LONGLONG raw_time_stamp = 0;
    sample->GetSampleTime(&raw_time_stamp);

    DWORD count = 0;
    sample->GetBufferCount(&count);

    for (DWORD i = 0; i < count; ++i) {
      ComPtr<IMFMediaBuffer> buffer;
      sample->GetBufferByIndex(i, &buffer);
      if (buffer) {
        observer_->OnIncomingCapturedData(buffer.Get(), raw_time_stamp,
            raw_time_stamp);
      } else {
        observer_->OnFrameDropped(
            VideoCaptureFrameDropReason::
                kWinMediaFoundationGetBufferByIndexReturnedNull);
      }
    }
    return S_OK;
  }

  void Shutdown() {
    base::AutoLock lock(lock_);
    observer_ = nullptr;
  }

 private:
  ~MFVideoCallback() {}

  // Protects access to |observer_|.
  base::Lock lock_;
  VideoCaptureDeviceMFWin* observer_;
};

// static
bool VideoCaptureDeviceMFWin::GetPixelFormatFromMFSourceMediaSubtype(
    const GUID& mf_source_media_subtype,
    bool use_hardware_format,
    VideoPixelFormat* pixel_format) {
  MediaFormatConfiguration media_format_configuration;
  if (!GetMediaFormatConfigurationFromMFSourceMediaSubtype(
          mf_source_media_subtype, use_hardware_format,
          &media_format_configuration))
    return false;

  *pixel_format = media_format_configuration.pixel_format;
  return true;
}

// Check if the video capture device supports pan, tilt and zoom controls.
// static
VideoCaptureControlSupport VideoCaptureDeviceMFWin::GetControlSupport(
    ComPtr<IMFMediaSource> source) {
  VideoCaptureControlSupport control_support;

  ComPtr<IAMCameraControl> camera_control;
  HRESULT hr = source.As(&camera_control);
  DLOG_IF_FAILED_WITH_HRESULT("Failed to retrieve IAMCameraControl", hr);
  ComPtr<IAMVideoProcAmp> video_control;
  hr = source.As(&video_control);
  DLOG_IF_FAILED_WITH_HRESULT("Failed to retrieve IAMVideoProcAmp", hr);

  // On Windows platform, some Image Capture video constraints and settings are
  // get or set using IAMCameraControl interface while the rest are get or set
  // using IAMVideoProcAmp interface and most device drivers define both of
  // them. So for simplicity GetPhotoState and SetPhotoState support Image
  // Capture API constraints and settings only if both interfaces are available.
  // Therefore, if either of these interface is missing, this backend does not
  // really support pan, tilt nor zoom.
  if (camera_control && video_control) {
    control_support.pan =
        GetCameraControlSupport(camera_control, CameraControl_Pan);
    control_support.tilt =
        GetCameraControlSupport(camera_control, CameraControl_Tilt);
    control_support.zoom =
        GetCameraControlSupport(camera_control, CameraControl_Zoom);
  }

  return control_support;
}

HRESULT VideoCaptureDeviceMFWin::GetDeviceStreamCount(IMFCaptureSource* source,
                                                      DWORD* count) {
  // Sometimes, GetDeviceStreamCount returns an
  // undocumented MF_E_INVALIDREQUEST. Retrying solves the issue.

    HRESULT hr;
    int retry_count = 0;
    do {
        hr = source->GetDeviceStreamCount(count);
        if (FAILED(hr))
        {
            ::Sleep(retry_delay_in_ms_);
        }
        // Give up after some amount of time
    } while (hr == MF_E_INVALIDREQUEST && retry_count++ < max_retry_count_);

    return hr;
}

HRESULT VideoCaptureDeviceMFWin::GetDeviceStreamCategory(
    IMFCaptureSource* source,
    DWORD stream_index,
    MF_CAPTURE_ENGINE_STREAM_CATEGORY* stream_category) {
  // We believe that GetDeviceStreamCategory could be affected by the same
  // behaviour of GetDeviceStreamCount and GetAvailableDeviceMediaType

    HRESULT hr;
    int retry_count = 0;
    do {
        hr = source->GetDeviceStreamCategory(stream_index,
            stream_category);
        if (FAILED(hr))
        {
            ::Sleep(retry_delay_in_ms_);
        }
        // Give up after some amount of time
    } while (hr == MF_E_INVALIDREQUEST && retry_count++ < max_retry_count_);

    return hr;
}

HRESULT VideoCaptureDeviceMFWin::GetAvailableDeviceMediaType(
    IMFCaptureSource* source,
    DWORD stream_index,
    DWORD media_type_index,
    IMFMediaType** type) {
  // Rarely, for some unknown reason, GetAvailableDeviceMediaType returns an
  // undocumented MF_E_INVALIDREQUEST. Retrying solves the issue.
    HRESULT hr;
    int retry_count = 0;
    do {
        hr = source->GetAvailableDeviceMediaType(stream_index,
            media_type_index, type);
        if (FAILED(hr))
        {
            ::Sleep(retry_delay_in_ms_);
        }
        // Give up after some amount of time
    } while (hr == MF_E_INVALIDREQUEST && retry_count++ < max_retry_count_);

    return hr;
}

HRESULT VideoCaptureDeviceMFWin::FillCapabilities(
    IMFCaptureSource* source,
    bool photo,
    CapabilityList* capabilities) {
  DWORD stream_count = 0;
  HRESULT hr = GetDeviceStreamCount(source, &stream_count);
  if (FAILED(hr))
    return hr;

  for (DWORD stream_index = 0; stream_index < stream_count; stream_index++) {
    MF_CAPTURE_ENGINE_STREAM_CATEGORY stream_category;
    hr = GetDeviceStreamCategory(source, stream_index, &stream_category);
    if (FAILED(hr))
      return hr;

    if ((photo && stream_category !=
                      MF_CAPTURE_ENGINE_STREAM_CATEGORY_PHOTO_INDEPENDENT) ||
        (!photo &&
         stream_category != MF_CAPTURE_ENGINE_STREAM_CATEGORY_VIDEO_PREVIEW &&
         stream_category != MF_CAPTURE_ENGINE_STREAM_CATEGORY_VIDEO_CAPTURE)) {
      continue;
    }

    DWORD media_type_index = 0;
    ComPtr<IMFMediaType> type;
    while (SUCCEEDED(hr = GetAvailableDeviceMediaType(
                         source, stream_index, media_type_index, &type))) {
      VideoCaptureFormat format;
      if (GetFormatFromSourceMediaType(
              type.Get(), photo,
              /*use_hardware_format=*/!photo &&
                  static_cast<bool>(dxgi_device_manager_),
              &format))
        capabilities->emplace_back(media_type_index, format, stream_index);
      type.Reset();
      ++media_type_index;
    }
    if (hr == MF_E_NO_MORE_TYPES) {
      hr = S_OK;
    }
    if (FAILED(hr)) {
      return hr;
    }
  }

  return hr;
}

VideoCaptureDeviceMFWin::VideoCaptureDeviceMFWin(
    const VideoCaptureDeviceDescriptor& device_descriptor,
    ComPtr<IMFMediaSource> source,
    DXGIDeviceManager* dxgi_device_manager)
    : VideoCaptureDeviceMFWin(device_descriptor,
                              source,
                              std::move(dxgi_device_manager),
                              nullptr) {}

VideoCaptureDeviceMFWin::VideoCaptureDeviceMFWin(
    const VideoCaptureDeviceDescriptor& device_descriptor,
    ComPtr<IMFMediaSource> source,
    DXGIDeviceManager* dxgi_device_manager,
    ComPtr<IMFCaptureEngine> engine)
    : facing_mode_(device_descriptor.facing),
      is_initialized_(false),
      max_retry_count_(200),
      retry_delay_in_ms_(50),
      source_(source),
      engine_(engine),
      is_started_(false),
      has_sent_on_started_to_client_(false),
      exposure_mode_manual_(false),
      focus_mode_manual_(false),
      white_balance_mode_manual_(false),
      capture_initialize_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                          base::WaitableEvent::InitialState::NOT_SIGNALED),
      // We never want to reset |capture_error_|.
      capture_error_(base::WaitableEvent::ResetPolicy::MANUAL,
                     base::WaitableEvent::InitialState::NOT_SIGNALED),
      dxgi_device_manager_(std::move(dxgi_device_manager)) {
}

VideoCaptureDeviceMFWin::~VideoCaptureDeviceMFWin() {
 
  if (video_callback_) {
    video_callback_->Shutdown();
  }
}

bool VideoCaptureDeviceMFWin::Init() {
  DCHECK(!is_initialized_);
  HRESULT hr;

  hr = source_.As(&camera_control_);
  DLOG_IF_FAILED_WITH_HRESULT("Failed to retrieve IAMCameraControl", hr);

  hr = source_.As(&video_control_);
  DLOG_IF_FAILED_WITH_HRESULT("Failed to retrieve IAMVideoProcAmp", hr);

  if (!engine_) {
    hr = CreateCaptureEngine(&engine_);
    if (FAILED(hr)) {
      LogError(hr);
      return false;
    }
  }

  ComPtr<IMFAttributes> attributes;
  hr = MFCreateAttributes(&attributes, 1);
  if (FAILED(hr)) {
    LogError(hr);
    return false;
  }

  hr = attributes->SetUINT32(MF_CAPTURE_ENGINE_USE_VIDEO_DEVICE_ONLY, TRUE);
  if (FAILED(hr)) {
    LogError(hr);
    return false;
  }

  if (dxgi_device_manager_) {
    dxgi_device_manager_->RegisterInCaptureEngineAttributes(attributes.Get());
  }

  video_callback_ = new MFVideoCallback(this);
  hr = engine_->Initialize(video_callback_, attributes.Get(), nullptr,
                           source_.Get());
  if (FAILED(hr)) {
    LogError(hr);
    return false;
  }

  hr = WaitOnCaptureEvent(MF_CAPTURE_ENGINE_INITIALIZED);
  if (FAILED(hr)) {
    LogError(hr);
    return false;
  }

  is_initialized_ = true;
  return true;
}

void VideoCaptureDeviceMFWin::AllocateAndStart(
    const VideoCaptureParams& params,
    std::unique_ptr<Client> client) {

  base::AutoLock lock(lock_);

  client_ = std::move(client);
  DCHECK(!is_started_);

  if (!engine_) {
    OnError(VideoCaptureError::kWinMediaFoundationEngineIsNull, E_FAIL);
    return;
  }

  ComPtr<IMFCaptureSource> source;
  HRESULT hr = engine_->GetSource(&source);
  if (FAILED(hr)) {
    OnError(VideoCaptureError::kWinMediaFoundationEngineGetSourceFailed,
            hr);
    return;
  }

  hr = FillCapabilities(source.Get(), true, &photo_capabilities_);
  if (FAILED(hr)) {
    OnError(VideoCaptureError::kWinMediaFoundationFillPhotoCapabilitiesFailed,
            hr);
    return;
  }

  if (!photo_capabilities_.empty()) {
    selected_photo_capability_ =
        std::make_unique<CapabilityWin>(photo_capabilities_.front());
  }

  CapabilityList video_capabilities;
  hr = FillCapabilities(source.Get(), false, &video_capabilities);
  if (FAILED(hr)) {
    OnError(VideoCaptureError::kWinMediaFoundationFillVideoCapabilitiesFailed,
            hr);
    return;
  }

  if (video_capabilities.empty()) {
    OnError(VideoCaptureError::kWinMediaFoundationNoVideoCapabilityFound,
            "No video capability found");
    return;
  }

  const CapabilityWin best_match_video_capability =
      GetBestMatchedCapability(params.requested_format, video_capabilities);
  ComPtr<IMFMediaType> source_video_media_type;
  hr = GetAvailableDeviceMediaType(
      source.Get(), best_match_video_capability.stream_index,
      best_match_video_capability.media_type_index, &source_video_media_type);
  if (FAILED(hr)) {
    OnError(
        VideoCaptureError::kWinMediaFoundationGetAvailableDeviceMediaTypeFailed,
        hr);
    return;
  }

  hr = source->SetCurrentDeviceMediaType(
      best_match_video_capability.stream_index, source_video_media_type.Get());
  if (FAILED(hr)) {
    OnError(
        VideoCaptureError::kWinMediaFoundationSetCurrentDeviceMediaTypeFailed,
        hr);
    return;
  }

  ComPtr<IMFCaptureSink> sink;
  hr = engine_->GetSink(MF_CAPTURE_ENGINE_SINK_TYPE_PREVIEW, &sink);
  if (FAILED(hr)) {
    OnError(VideoCaptureError::kWinMediaFoundationEngineGetSinkFailed,
            hr);
    return;
  }

  ComPtr<IMFCapturePreviewSink> preview_sink;
  hr = sink->QueryInterface(IID_PPV_ARGS(&preview_sink));
  if (FAILED(hr)) {
    OnError(VideoCaptureError::
                kWinMediaFoundationSinkQueryCapturePreviewInterfaceFailed,
            hr);
    return;
  }

  hr = preview_sink->RemoveAllStreams();
  if (FAILED(hr)) {
    OnError(VideoCaptureError::kWinMediaFoundationSinkRemoveAllStreamsFailed,
            hr);
    return;
  }

  ComPtr<IMFMediaType> sink_video_media_type;
  hr = MFCreateMediaType(&sink_video_media_type);
  if (FAILED(hr)) {
    OnError(
        VideoCaptureError::kWinMediaFoundationCreateSinkVideoMediaTypeFailed,
        hr);
    return;
  }

  hr = ConvertToVideoSinkMediaType(
      source_video_media_type.Get(),
      /*use_hardware_format=*/static_cast<bool>(dxgi_device_manager_),
      sink_video_media_type.Get());
  if (FAILED(hr)) {
    OnError(
        VideoCaptureError::kWinMediaFoundationConvertToVideoSinkMediaTypeFailed,
        hr);
    return;
  }

  DWORD dw_sink_stream_index = 0;
  hr = preview_sink->AddStream(best_match_video_capability.stream_index,
                               sink_video_media_type.Get(), nullptr,
                               &dw_sink_stream_index);
  if (FAILED(hr)) {
    OnError(VideoCaptureError::kWinMediaFoundationSinkAddStreamFailed,
            hr);
    return;
  }

  hr = preview_sink->SetSampleCallback(dw_sink_stream_index,
                                       video_callback_);
  if (FAILED(hr)) {
    OnError(VideoCaptureError::kWinMediaFoundationSinkSetSampleCallbackFailed,
            hr);
    return;
  }

  // Note, that it is not sufficient to wait for
  // MF_CAPTURE_ENGINE_PREVIEW_STARTED as an indicator that starting capture has
  // succeeded. If the capture device is already in use by a different
  // application, MediaFoundation will still emit
  // MF_CAPTURE_ENGINE_PREVIEW_STARTED, and only after that raise an error
  // event. For the lack of any other events indicating success, we have to wait
  // for the first video frame to arrive before sending our |OnStarted| event to
  // |client_|.
  has_sent_on_started_to_client_ = false;
  hr = engine_->StartPreview();
  if (FAILED(hr)) {
    OnError(VideoCaptureError::kWinMediaFoundationEngineStartPreviewFailed,
            hr);
    return;
  }

  selected_video_capability_ =
      std::make_unique<CapabilityWin>(best_match_video_capability);

  is_started_ = true;
}

void VideoCaptureDeviceMFWin::StopAndDeAllocate() {
  base::AutoLock lock(lock_);

  if (is_started_ && engine_)
    engine_->StopPreview();
  is_started_ = false;

  client_.reset();
}

void VideoCaptureDeviceMFWin::OnIncomingCapturedData(
    IMFMediaBuffer* buffer,
    long reference_time,
    long timestamp) {
  VideoCaptureFrameDropReason frame_drop_reason =
      VideoCaptureFrameDropReason::kNone;
  OnIncomingCapturedDataInternal(buffer, reference_time, timestamp,
                                 frame_drop_reason);
  if (frame_drop_reason != VideoCaptureFrameDropReason::kNone) {
    OnFrameDropped(frame_drop_reason);
  }
}
void VideoCaptureDeviceMFWin::OnIncomingCapturedDataInternal(
    IMFMediaBuffer* buffer,
    long reference_time,
    long timestamp,
    VideoCaptureFrameDropReason& frame_drop_reason) {
  base::AutoLock lock(lock_);

  SendOnStartedIfNotYetSent();

  bool delivered_texture = false;

  if (client_.get()) {
    if (!has_sent_on_started_to_client_) {
      has_sent_on_started_to_client_ = true;
      client_->OnStarted();
    }
  }

  ScopedBufferLock locked_buffer(buffer);
  if (!locked_buffer.data()) {
    DLOG(ERROR) << "Locked buffer delivered nullptr";
    frame_drop_reason = VideoCaptureFrameDropReason::
        kWinMediaFoundationLockingBufferDelieveredNullptr;
    return;
  }

  if (!delivered_texture && client_.get()) {
    // TODO(julien.isorce): retrieve the color space information using Media
    // Foundation api, MFGetAttributeSize/MF_MT_VIDEO_PRIMARIES,in order to
    // build a gfx::ColorSpace. See http://crbug.com/959988.
    client_->OnIncomingCapturedData(
        locked_buffer.data(), locked_buffer.length(),
        selected_video_capability_->supported_format);
  }
}

void VideoCaptureDeviceMFWin::OnFrameDropped(
    VideoCaptureFrameDropReason reason) {
  base::AutoLock lock(lock_);

  SendOnStartedIfNotYetSent();

  if (client_.get()) {
    client_->OnFrameDropped(reason);
  }
}

void VideoCaptureDeviceMFWin::OnEvent(IMFMediaEvent* media_event) {
  base::AutoLock lock(lock_);

  HRESULT hr;
  GUID capture_event_guid = GUID_NULL;

  media_event->GetStatus(&hr);
  media_event->GetExtendedType(&capture_event_guid);
  // TODO(http://crbug.com/1093521): Add cases for Start
  // MF_CAPTURE_ENGINE_PREVIEW_STARTED and MF_CAPTURE_ENGINE_PREVIEW_STOPPED
  // When MF_CAPTURE_ENGINE_ERROR is returned the captureengine object is no
  // longer valid.
  if (capture_event_guid == MF_CAPTURE_ENGINE_ERROR || FAILED(hr)) {
    capture_error_.Signal();
    // There should always be a valid error
    hr = SUCCEEDED(hr) ? E_UNEXPECTED : hr;
  } else if (capture_event_guid == MF_CAPTURE_ENGINE_INITIALIZED) {
    capture_initialize_.Signal();
  }

  if (FAILED(hr))
    OnError(VideoCaptureError::kWinMediaFoundationGetMediaEventStatusFailed,
            hr);
}

void VideoCaptureDeviceMFWin::OnError(VideoCaptureError error,
                                      HRESULT hr) {
  OnError(error, "hrError");
}

void VideoCaptureDeviceMFWin::OnError(VideoCaptureError error,
                                      const char* message) {
  if (!client_.get())
    return;

  client_->OnError(error, message);
}

void VideoCaptureDeviceMFWin::SendOnStartedIfNotYetSent() {
  if (!client_ || has_sent_on_started_to_client_)
    return;
  has_sent_on_started_to_client_ = true;
  client_->OnStarted();
}

HRESULT VideoCaptureDeviceMFWin::WaitOnCaptureEvent(GUID capture_event_guid) {
  HRESULT hr = S_OK;
  HANDLE events[] = {nullptr, capture_error_.handle()};

  // TODO(http://crbug.com/1093521): Add cases for Start
  // MF_CAPTURE_ENGINE_PREVIEW_STARTED and MF_CAPTURE_ENGINE_PREVIEW_STOPPED
  if (capture_event_guid == MF_CAPTURE_ENGINE_INITIALIZED) {
    events[0] = capture_initialize_.handle();
  } else {
    // no registered event handle for the event requested
    hr = E_NOTIMPL;
    LogError(hr);
    return hr;
  }

  DWORD wait_result =
      ::WaitForMultipleObjects(base::size(events), events, FALSE, INFINITE);
  switch (wait_result) {
    case WAIT_OBJECT_0:
      break;
    case WAIT_FAILED:
      hr = HRESULT_FROM_WIN32(::GetLastError());
      LogError(hr);
      break;
    default:
      hr = E_UNEXPECTED;
      LogError(hr);
      break;
  }
  return hr;
}
}  // namespace media
