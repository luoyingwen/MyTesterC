// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mf_helpers.h"
#include "camerabase.h"

#include <d3d11.h>

namespace media {

Microsoft::WRL::ComPtr<IMFSample> CreateEmptySampleWithBuffer(
    uint32_t buffer_length,
    int align) {
    DCHECK(buffer_length, 0U);

  Microsoft::WRL::ComPtr<IMFSample> sample;
  HRESULT hr = MFCreateSample(&sample);
  DLOG_IF_FAILED_WITH_HRESULT("MFCreateSample failed", hr);

  Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
  if (align == 0) {
    // Note that MFCreateMemoryBuffer is same as MFCreateAlignedMemoryBuffer
    // with the align argument being 0.
    hr = MFCreateMemoryBuffer(buffer_length, &buffer);
  } else {
    hr = MFCreateAlignedMemoryBuffer(buffer_length, align - 1, &buffer);
  }
  DLOG_IF_FAILED_WITH_HRESULT("Failed to create memory buffer for sample", hr);

  hr = sample->AddBuffer(buffer.Get());
  DLOG_IF_FAILED_WITH_HRESULT("Failed to add buffer to sample", hr);

  buffer->SetCurrentLength(0);
  return sample;
}

MediaBufferScopedPointer::MediaBufferScopedPointer(IMFMediaBuffer* media_buffer)
    : media_buffer_(media_buffer),
      buffer_(nullptr),
      max_length_(0),
      current_length_(0) {
  HRESULT hr = media_buffer_->Lock(&buffer_, &max_length_, &current_length_);
  DCHECK(SUCCEEDED(hr));
}

MediaBufferScopedPointer::~MediaBufferScopedPointer() {
  HRESULT hr = media_buffer_->Unlock();
  DCHECK(SUCCEEDED(hr));
}

HRESULT CopyCoTaskMemWideString(LPCWSTR in_string, LPWSTR* out_string) {
  if (!in_string || !out_string) {
    return E_INVALIDARG;
  }

  size_t size = (wcslen(in_string) + 1) * sizeof(wchar_t);
  LPWSTR copy = reinterpret_cast<LPWSTR>(CoTaskMemAlloc(size));
  if (!copy)
    return E_OUTOFMEMORY;

  wcscpy(copy, in_string);
  *out_string = copy;
  return S_OK;
}

HRESULT SetDebugName(ID3D11DeviceChild* d3d11_device_child,
                     const char* debug_string) {
  return d3d11_device_child->SetPrivateData(WKPDID_D3DDebugObjectName,
                                            strlen(debug_string), debug_string);
}

}  // namespace media
