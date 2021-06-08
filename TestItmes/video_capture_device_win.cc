// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "stdafx.h"

#include "video_capture_device_win.h"

#include <ks.h>
#include <ksmedia.h>
#include <objbase.h>

#include <algorithm>
#include <list>
#include <utility>

//
//#include "base/feature_list.h"
//#include "base/stl_util.h"
//#include "base/strings/sys_string_conversions.h"
//#include "base/win/scoped_co_mem.h"
//#include "base/win/scoped_variant.h"
//#include "base/win/win_util.h"
//#include "media/base/media_switches.h"
//#include "media/base/timestamp_constants.h"
//#include "media/capture/mojom/image_capture_types.h"
//#include "media/capture/video/blob_utils.h"
//#include "media/capture/video/win/metrics.h"
//#include "media/capture/video/win/video_capture_device_utils_win.h"

//using base::win::ScopedCoMem;
//using base::win::ScopedVariant;
using Microsoft::WRL::ComPtr;

namespace media {

// Check if a Pin matches a category.
bool PinMatchesCategory(IPin* pin, REFGUID category) {
  DCHECK(pin);
  bool found = false;
  ComPtr<IKsPropertySet> ks_property;
  HRESULT hr = pin->QueryInterface(IID_PPV_ARGS(&ks_property));
  if (SUCCEEDED(hr)) {
    GUID pin_category;
    DWORD return_value;
    hr = ks_property->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, nullptr, 0,
                          &pin_category, sizeof(pin_category), &return_value);
    if (SUCCEEDED(hr) && (return_value == sizeof(pin_category))) {
      found = (pin_category == category);
    }
  }
  return found;
}

// Check if a Pin's MediaType matches a given |major_type|.
bool PinMatchesMajorType(IPin* pin, REFGUID major_type) {
  DCHECK(pin);
  AM_MEDIA_TYPE connection_media_type;
  const HRESULT hr = pin->ConnectionMediaType(&connection_media_type);
  return SUCCEEDED(hr) && connection_media_type.majortype == major_type;
}

// Check if the video capture device supports pan, tilt and zoom controls.
// static
VideoCaptureControlSupport VideoCaptureDeviceWin::GetControlSupport(
    ComPtr<IBaseFilter> capture_filter) {
  VideoCaptureControlSupport control_support;
  ComPtr<ICameraControl> camera_control;
  ComPtr<IVideoProcAmp> video_control;

  if (GetCameraAndVideoControls(capture_filter, &camera_control,
                                &video_control)) {
    long min, max, step, default_value, flags;
    control_support.pan = SUCCEEDED(camera_control->getRange_Pan(
                              &min, &max, &step, &default_value, &flags)) &&
                          min < max;
    control_support.tilt = SUCCEEDED(camera_control->getRange_Tilt(
                               &min, &max, &step, &default_value, &flags)) &&
                           min < max;
    control_support.zoom = SUCCEEDED(camera_control->getRange_Zoom(
                               &min, &max, &step, &default_value, &flags)) &&
                           min < max;
  }

  return control_support;
}

// static
void VideoCaptureDeviceWin::GetDeviceCapabilityList(
    ComPtr<IBaseFilter> capture_filter,
    bool query_detailed_frame_rates,
    CapabilityList* out_capability_list) {
  ComPtr<IPin> output_capture_pin(VideoCaptureDeviceWin::GetPin(
      capture_filter.Get(), PINDIR_OUTPUT, PIN_CATEGORY_CAPTURE, GUID_NULL));
  if (!output_capture_pin.Get()) {
    DLOG(ERROR) << "Failed to get capture output pin";
    return;
  }

  GetPinCapabilityList(capture_filter, output_capture_pin,
                       query_detailed_frame_rates, out_capability_list);
}

// static
void VideoCaptureDeviceWin::GetPinCapabilityList(
    ComPtr<IBaseFilter> capture_filter,
    ComPtr<IPin> output_capture_pin,
    bool query_detailed_frame_rates,
    CapabilityList* out_capability_list) {
  ComPtr<IAMStreamConfig> stream_config;
  HRESULT hr = output_capture_pin.As(&stream_config);
  if (FAILED(hr)) {
    DLOG(ERROR) << "Failed to get IAMStreamConfig interface from "
                   "capture device: "
                << hr;
    return;
  }

  // Get interface used for getting the frame rate.
  ComPtr<IAMVideoControl> video_control;
  hr = capture_filter.As(&video_control);

  int count = 0, size = 0;
  hr = stream_config->GetNumberOfCapabilities(&count, &size);
  if (FAILED(hr)) {
    DLOG(ERROR) << "GetNumberOfCapabilities failed: "
                << hr;
    return;
  }

  std::unique_ptr<BYTE[]> caps(new BYTE[size]);
  for (int i = 0; i < count; ++i) {
    VideoCaptureDeviceWin::ScopedMediaType media_type;
    hr = stream_config->GetStreamCaps(i, media_type.Receive(), caps.get());
    // GetStreamCaps() may return S_FALSE, so don't use FAILED() or SUCCEED()
    // macros here since they'll trigger incorrectly.
    if (hr != S_OK || !media_type.get()) {
      DLOG(ERROR) << "GetStreamCaps failed: "
                  << hr;
      return;
    }

    if (media_type->majortype == MEDIATYPE_Video &&
        media_type->formattype == FORMAT_VideoInfo) {
      VideoCaptureFormat format;
      format.pixel_format =
          VideoCaptureDeviceWin::TranslateMediaSubtypeToPixelFormat(
              media_type->subtype);
      if (format.pixel_format == PIXEL_FORMAT_UNKNOWN)
        continue;
      VIDEOINFOHEADER* h =
          reinterpret_cast<VIDEOINFOHEADER*>(media_type->pbFormat);
      format.frame_size.SetSize(h->bmiHeader.biWidth, h->bmiHeader.biHeight);

      std::vector<float> frame_rates;
      if (query_detailed_frame_rates && video_control.Get()) {
        // Try to get a better |time_per_frame| from IAMVideoControl. If not,
        // use the value from VIDEOINFOHEADER.
        ScopedCoMem<LONGLONG> time_per_frame_list;
        LONG list_size = 0;
        const SIZE size = {format.frame_size.width,
                           format.frame_size.height};
        hr = video_control->GetFrameRateList(output_capture_pin.Get(), i, size,
                                             &list_size, &time_per_frame_list);
        // Sometimes |list_size| will be > 0, but time_per_frame_list will be
        // NULL. Some drivers may return an HRESULT of S_FALSE which
        // SUCCEEDED() translates into success, so explicitly check S_OK. See
        // http://crbug.com/306237.
        if (hr == S_OK && list_size > 0 && time_per_frame_list) {
          for (int k = 0; k < list_size; k++) {
            LONGLONG time_per_frame = *(time_per_frame_list.get() + k);
            if (time_per_frame <= 0)
              continue;
            frame_rates.push_back(kSecondsToReferenceTime /
                                  static_cast<float>(time_per_frame));
          }
        }
      }

      if (frame_rates.empty() && h->AvgTimePerFrame > 0) {
        frame_rates.push_back(kSecondsToReferenceTime /
                              static_cast<float>(h->AvgTimePerFrame));
      }
      if (frame_rates.empty())
        frame_rates.push_back(0.0f);

      for (const auto& frame_rate : frame_rates) {
        format.frame_rate = frame_rate;
        out_capability_list->emplace_back(i, format, h->bmiHeader);
      }
    }
  }
}

// Finds an IPin on an IBaseFilter given the direction, Category and/or Major
// Type. If either |category| or |major_type| are GUID_NULL, they are ignored.
// static
ComPtr<IPin> VideoCaptureDeviceWin::GetPin(ComPtr<IBaseFilter> capture_filter,
                                           PIN_DIRECTION pin_dir,
                                           REFGUID category,
                                           REFGUID major_type) {
  ComPtr<IPin> pin;
  ComPtr<IEnumPins> pin_enum;
  HRESULT hr = capture_filter->EnumPins(&pin_enum);
  if (pin_enum.Get() == nullptr)
    return pin;

  // Get first unconnected pin.
  hr = pin_enum->Reset();  // set to first pin
  while ((hr = pin_enum->Next(1, &pin, nullptr)) == S_OK) {
    PIN_DIRECTION this_pin_dir = static_cast<PIN_DIRECTION>(-1);
    hr = pin->QueryDirection(&this_pin_dir);
    if (pin_dir == this_pin_dir) {
      if ((category == GUID_NULL || PinMatchesCategory(pin.Get(), category)) &&
          (major_type == GUID_NULL ||
           PinMatchesMajorType(pin.Get(), major_type))) {
        return pin;
      }
    }
    pin.Reset();
  }

  DCHECK(!pin.Get());
  return pin;
}

// static
VideoPixelFormat VideoCaptureDeviceWin::TranslateMediaSubtypeToPixelFormat(
    const GUID& sub_type) {
  static struct {
    const GUID& sub_type;
    VideoPixelFormat format;
  } const kMediaSubtypeToPixelFormatCorrespondence[] = {
      {kMediaSubTypeI420, PIXEL_FORMAT_I420},
      {MEDIASUBTYPE_IYUV, PIXEL_FORMAT_I420},
      {MEDIASUBTYPE_RGB24, PIXEL_FORMAT_RGB24},
      {MEDIASUBTYPE_RGB32, PIXEL_FORMAT_ARGB},
      {MEDIASUBTYPE_YUY2, PIXEL_FORMAT_YUY2},
      {MEDIASUBTYPE_MJPG, PIXEL_FORMAT_MJPEG},
      {MEDIASUBTYPE_UYVY, PIXEL_FORMAT_UYVY},
      {MEDIASUBTYPE_ARGB32, PIXEL_FORMAT_ARGB},
      {kMediaSubTypeHDYC, PIXEL_FORMAT_UYVY},
      {kMediaSubTypeY16, PIXEL_FORMAT_Y16},
      {kMediaSubTypeZ16, PIXEL_FORMAT_Y16},
      {kMediaSubTypeINVZ, PIXEL_FORMAT_Y16},
  };
  for (const auto& pixel_format : kMediaSubtypeToPixelFormatCorrespondence) {
    if (sub_type == pixel_format.sub_type)
      return pixel_format.format;
  }
  DVLOG(2) << "Device (also) supports an unknown media type "
           << WStringFromGUID(sub_type).c_str();
  return PIXEL_FORMAT_UNKNOWN;
}

void VideoCaptureDeviceWin::ScopedMediaType::Free() {
  if (!media_type_)
    return;

  DeleteMediaType(media_type_);
  media_type_ = nullptr;
}

AM_MEDIA_TYPE** VideoCaptureDeviceWin::ScopedMediaType::Receive() {
  DCHECK(!media_type_);
  return &media_type_;
}

// Release the format block for a media type.
// http://msdn.microsoft.com/en-us/library/dd375432(VS.85).aspx
void VideoCaptureDeviceWin::ScopedMediaType::FreeMediaType(AM_MEDIA_TYPE* mt) {
  if (mt->cbFormat != 0) {
    CoTaskMemFree(mt->pbFormat);
    mt->cbFormat = 0;
    mt->pbFormat = nullptr;
  }
  if (mt->pUnk != nullptr) {
    NOTREACHED();
    // pUnk should not be used.
    mt->pUnk->Release();
    mt->pUnk = nullptr;
  }
}

// Delete a media type structure that was allocated on the heap.
// http://msdn.microsoft.com/en-us/library/dd375432(VS.85).aspx
void VideoCaptureDeviceWin::ScopedMediaType::DeleteMediaType(
    AM_MEDIA_TYPE* mt) {
  if (mt != nullptr) {
    FreeMediaType(mt);
    CoTaskMemFree(mt);
  }
}

VideoCaptureDeviceWin::VideoCaptureDeviceWin(
    const VideoCaptureDeviceDescriptor& device_descriptor,
    ComPtr<IBaseFilter> capture_filter)
    : device_descriptor_(device_descriptor),
      state_(kIdle),
      capture_filter_(std::move(capture_filter)),
      white_balance_mode_manual_(false),
      exposure_mode_manual_(false),
      focus_mode_manual_(false),
      enable_get_photo_state_(false) {
  // TODO(mcasas): Check that CoInitializeEx() has been called with the
  // appropriate Apartment model, i.e., Single Threaded.
}

VideoCaptureDeviceWin::~VideoCaptureDeviceWin() {
  //DCHECK(thread_checker_.CalledOnValidThread());
  if (media_control_.Get())
    media_control_->Stop();

  if (graph_builder_.Get()) {
    if (sink_filter_) {
      graph_builder_->RemoveFilter(sink_filter_.Get());
      sink_filter_.Reset();
    }

    if (capture_filter_.Get())
      graph_builder_->RemoveFilter(capture_filter_.Get());
  }

  if (capture_graph_builder_.Get())
    capture_graph_builder_.Reset();
}

bool VideoCaptureDeviceWin::Init() {
  //DCHECK(thread_checker_.CalledOnValidThread());
  output_capture_pin_ = GetPin(capture_filter_.Get(), PINDIR_OUTPUT,
                               PIN_CATEGORY_CAPTURE, GUID_NULL);
  if (!output_capture_pin_.Get()) {
    DLOG(ERROR) << "Failed to get capture output pin";
    return false;
  }

  // Create the sink filter used for receiving Captured frames.
  sink_filter_ = new SinkFilter(this);
  if (sink_filter_ == nullptr) {
    DLOG(ERROR) << "Failed to create sink filter";
    return false;
  }

  input_sink_pin_ = sink_filter_->GetPin(0);

  HRESULT hr =
      ::CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
                         IID_PPV_ARGS(&graph_builder_));
  DLOG_IF_FAILED_WITH_HRESULT("Failed to create capture filter", hr);
  if (FAILED(hr))
    return false;

  hr = ::CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC,
                          IID_PPV_ARGS(&capture_graph_builder_));
  DLOG_IF_FAILED_WITH_HRESULT("Failed to create the Capture Graph Builder", hr);
  if (FAILED(hr))
    return false;

  hr = capture_graph_builder_->SetFiltergraph(graph_builder_.Get());
  DLOG_IF_FAILED_WITH_HRESULT("Failed to give graph to capture graph builder",
                              hr);
  if (FAILED(hr))
    return false;

  hr = graph_builder_.As(&media_control_);
  DLOG_IF_FAILED_WITH_HRESULT("Failed to create media control builder", hr);
  if (FAILED(hr))
    return false;

  hr = graph_builder_->AddFilter(capture_filter_.Get(), nullptr);
  DLOG_IF_FAILED_WITH_HRESULT("Failed to add the capture device to the graph",
                              hr);
  if (FAILED(hr))
    return false;

  hr = graph_builder_->AddFilter(sink_filter_.Get(), nullptr);
  DLOG_IF_FAILED_WITH_HRESULT("Failed to add the sink filter to the graph", hr);
  if (FAILED(hr))
    return false;

  // The following code builds the upstream portions of the graph, for example
  // if a capture device uses a Windows Driver Model (WDM) driver, the graph may
  // require certain filters upstream from the WDM Video Capture filter, such as
  // a TV Tuner filter or an Analog Video Crossbar filter. We try using the more
  // prevalent MEDIATYPE_Interleaved first.
  ComPtr<IAMStreamConfig> stream_config;

  hr = capture_graph_builder_->FindInterface(
      &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, capture_filter_.Get(),
      IID_IAMStreamConfig, (void**)&stream_config);
  if (FAILED(hr)) {
    hr = capture_graph_builder_->FindInterface(
        &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, capture_filter_.Get(),
        IID_IAMStreamConfig, (void**)&stream_config);
    DLOG_IF_FAILED_WITH_HRESULT("Failed to find CapFilter:IAMStreamConfig", hr);
  }

  return CreateCapabilityMap();
}

void VideoCaptureDeviceWin::AllocateAndStart(
    const VideoCaptureParams& params,
    std::unique_ptr<Client> client) {
  //DCHECK(thread_checker_.CalledOnValidThread());
  if (state_ != kIdle)
    return;

  client_ = std::move(client);

  // Get the camera capability that best match the requested format.
  const CapabilityWin found_capability =
      GetBestMatchedCapability(params.requested_format, capabilities_);

  // Reduce the frame rate if the requested frame rate is lower
  // than the capability.
  float frame_rate = params.requested_format.frame_rate;
  if (frame_rate > found_capability.supported_format.frame_rate)
      frame_rate = found_capability.supported_format.frame_rate;

  ComPtr<IAMStreamConfig> stream_config;
  HRESULT hr = output_capture_pin_.As(&stream_config);
  if (FAILED(hr)) {
    SetErrorState(
        media::VideoCaptureError::kWinDirectShowCantGetCaptureFormatSettings,
        "Can't get the Capture format settings", hr);
    return;
  }

  int count = 0, size = 0;
  hr = stream_config->GetNumberOfCapabilities(&count, &size);
  if (FAILED(hr)) {
    SetErrorState(
        media::VideoCaptureError::kWinDirectShowFailedToGetNumberOfCapabilities,
        "Failed to GetNumberOfCapabilities", hr);
    return;
  }

  std::unique_ptr<BYTE[]> caps(new BYTE[size]);
  ScopedMediaType media_type;

  // Get the windows capability from the capture device.
  // GetStreamCaps can return S_FALSE which we consider an error. Therefore the
  // FAILED macro can't be used.
  hr = stream_config->GetStreamCaps(found_capability.media_type_index,
                                    media_type.Receive(), caps.get());
  if (hr != S_OK) {
    SetErrorState(media::VideoCaptureError::
                      kWinDirectShowFailedToGetCaptureDeviceCapabilities,
                  "Failed to get capture device capabilities", hr);
    return;
  }
  if (media_type->formattype == FORMAT_VideoInfo) {
    VIDEOINFOHEADER* h =
        reinterpret_cast<VIDEOINFOHEADER*>(media_type->pbFormat);
    if (frame_rate > 0)
      h->AvgTimePerFrame = kSecondsToReferenceTime / frame_rate;
  }
  // Set the sink filter to request this format.
  sink_filter_->SetRequestedMediaFormat(
      found_capability.supported_format.pixel_format, frame_rate,
      found_capability.info_header);
  // Order the capture device to use this format.
  hr = stream_config->SetFormat(media_type.get());
  if (FAILED(hr)) {
    SetErrorState(media::VideoCaptureError::
                      kWinDirectShowFailedToSetCaptureDeviceOutputFormat,
                  "Failed to set capture device output format", hr);
    return;
  }
  capture_format_ = found_capability.supported_format;

  SetAntiFlickerInCaptureFilter(params);

  if (media_type->subtype == kMediaSubTypeHDYC) {
    // HDYC pixel format, used by the DeckLink capture card, needs an AVI
    // decompressor filter after source, let |graph_builder_| add it.
    hr = graph_builder_->Connect(output_capture_pin_.Get(),
                                 input_sink_pin_.Get());
  } else {
    hr = graph_builder_->ConnectDirect(output_capture_pin_.Get(),
                                       input_sink_pin_.Get(), nullptr);
  }

  if (FAILED(hr)) {
    SetErrorState(
        media::VideoCaptureError::kWinDirectShowFailedToConnectTheCaptureGraph,
        "Failed to connect the Capture graph.", hr);
    return;
  }

  hr = media_control_->Pause();
  if (FAILED(hr)) {
    SetErrorState(
        media::VideoCaptureError::kWinDirectShowFailedToPauseTheCaptureDevice,
        "Failed to pause the Capture device", hr);
    return;
  }

  // Start capturing.
  hr = media_control_->Run();
  if (FAILED(hr)) {
    SetErrorState(
        media::VideoCaptureError::kWinDirectShowFailedToStartTheCaptureDevice,
        "Failed to start the Capture device.", hr);
    return;
  }

  client_->OnStarted();
  state_ = kCapturing;
}

void VideoCaptureDeviceWin::StopAndDeAllocate() {
  //DCHECK(thread_checker_.CalledOnValidThread());
  if (state_ != kCapturing)
    return;

  HRESULT hr = media_control_->Stop();
  if (FAILED(hr)) {
    SetErrorState(
        media::VideoCaptureError::kWinDirectShowFailedToStopTheCaptureGraph,
        "Failed to stop the capture graph.", hr);
    return;
  }

  graph_builder_->Disconnect(output_capture_pin_.Get());
  graph_builder_->Disconnect(input_sink_pin_.Get());

  client_.reset();
  state_ = kIdle;
}


// static
bool VideoCaptureDeviceWin::GetCameraAndVideoControls(
    ComPtr<IBaseFilter> capture_filter,
    ICameraControl** camera_control,
    IVideoProcAmp** video_control) {
  DCHECK(camera_control);
  DCHECK(video_control);
  DCHECK(!*camera_control);
  DCHECK(!*video_control);

  ComPtr<IKsTopologyInfo> info;
  HRESULT hr = capture_filter.As(&info);
  if (FAILED(hr)) {
    DLOG_IF_FAILED_WITH_HRESULT("Failed to obtain the topology info.", hr);
    return false;
  }

  DWORD num_nodes = 0;
  hr = info->get_NumNodes(&num_nodes);
  if (FAILED(hr)) {
    DLOG_IF_FAILED_WITH_HRESULT("Failed to obtain the number of nodes.", hr);
    return false;
  }

  // Every UVC camera is expected to have a single ICameraControl and a single
  // IVideoProcAmp nodes, and both are needed; ignore any unlikely later ones.
  GUID node_type;
  for (size_t i = 0; i < num_nodes; i++) {
    info->get_NodeType(i, &node_type);
    if (IsEqualGUID(node_type, KSNODETYPE_VIDEO_CAMERA_TERMINAL)) {
      hr = info->CreateNodeInstance(i, IID_PPV_ARGS(camera_control));
      if (SUCCEEDED(hr))
        break;
      DLOG_IF_FAILED_WITH_HRESULT("Failed to retrieve the ICameraControl.", hr);
      return false;
    }
  }
  for (size_t i = 0; i < num_nodes; i++) {
    info->get_NodeType(i, &node_type);
    if (IsEqualGUID(node_type, KSNODETYPE_VIDEO_PROCESSING)) {
      hr = info->CreateNodeInstance(i, IID_PPV_ARGS(video_control));
      if (SUCCEEDED(hr))
        break;
      DLOG_IF_FAILED_WITH_HRESULT("Failed to retrieve the IVideoProcAmp.", hr);
      return false;
    }
  }
  return *camera_control && *video_control;
}

// Implements SinkFilterObserver::SinkFilterObserver.
void VideoCaptureDeviceWin::FrameReceived(const uint8_t* buffer,
                                          int length,
                                          const VideoCaptureFormat& format,
                                          long timestamp,
                                          bool flip_y) {
  // TODO(julien.isorce): retrieve the color space information using the
  // DirectShow api, AM_MEDIA_TYPE::VIDEOINFOHEADER2::dwControlFlags. If
  // AMCONTROL_COLORINFO_PRESENT, then reinterpret dwControlFlags as a
  // DXVA_ExtendedFormat. Then use its fields DXVA_VideoPrimaries,
  // DXVA_VideoTransferMatrix, DXVA_VideoTransferFunction and
  // DXVA_NominalRangeto build a gfx::ColorSpace. See http://crbug.com/959992.
  client_->OnIncomingCapturedData(buffer, length, format);
}

void VideoCaptureDeviceWin::FrameDropped(VideoCaptureFrameDropReason reason) {
  client_->OnFrameDropped(reason);
}

bool VideoCaptureDeviceWin::CreateCapabilityMap() {
  //DCHECK(thread_checker_.CalledOnValidThread());
  GetPinCapabilityList(capture_filter_, output_capture_pin_,
                       true /* query_detailed_frame_rates */, &capabilities_);
  return !capabilities_.empty();
}

// Set the power line frequency removal in |capture_filter_| if available.
void VideoCaptureDeviceWin::SetAntiFlickerInCaptureFilter(
    const VideoCaptureParams& params) {
    const PowerLineFrequency power_line_frequency = PowerLineFrequency::FREQUENCY_60HZ;
  if (power_line_frequency != PowerLineFrequency::FREQUENCY_50HZ &&
      power_line_frequency != PowerLineFrequency::FREQUENCY_60HZ) {
    return;
  }
  ComPtr<IKsPropertySet> ks_propset;
  DWORD type_support = 0;
  HRESULT hr;
  if (SUCCEEDED(hr = capture_filter_.As(&ks_propset)) &&
      SUCCEEDED(hr = ks_propset->QuerySupported(
                    PROPSETID_VIDCAP_VIDEOPROCAMP,
                    KSPROPERTY_VIDEOPROCAMP_POWERLINE_FREQUENCY,
                    &type_support)) &&
      (type_support & KSPROPERTY_SUPPORT_SET)) {
    KSPROPERTY_VIDEOPROCAMP_S data = {};
    data.Property.Set = PROPSETID_VIDCAP_VIDEOPROCAMP;
    data.Property.Id = KSPROPERTY_VIDEOPROCAMP_POWERLINE_FREQUENCY;
    data.Property.Flags = KSPROPERTY_TYPE_SET;
    data.Value =
        (power_line_frequency == PowerLineFrequency::FREQUENCY_50HZ) ? 1 : 2;
    data.Flags = KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    hr = ks_propset->Set(PROPSETID_VIDCAP_VIDEOPROCAMP,
                         KSPROPERTY_VIDEOPROCAMP_POWERLINE_FREQUENCY, &data,
                         sizeof(data), &data, sizeof(data));
    DLOG_IF_FAILED_WITH_HRESULT("Anti-flicker setting failed", hr);
  }
}

void VideoCaptureDeviceWin::SetErrorState(media::VideoCaptureError error,
                                          const std::string& reason,
                                          HRESULT hr) {
  //DCHECK(thread_checker_.CalledOnValidThread());
  DLOG_IF_FAILED_WITH_HRESULT(reason, hr);
  state_ = kError;
  client_->OnError(error, reason);
}
}  // namespace media
