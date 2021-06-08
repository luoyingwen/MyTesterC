// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Implement a simple base class for a DirectShow input pin. It may only be
// used in a single threaded apartment.

#ifndef MEDIA_CAPTURE_VIDEO_WIN_PIN_BASE_WIN_H_
#define MEDIA_CAPTURE_VIDEO_WIN_PIN_BASE_WIN_H_

// Avoid including strsafe.h via dshow as it will cause build warnings.
#define NO_DSHOW_STRSAFE
#include <dshow.h>
#include <wrl/client.h>

namespace media {

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_WIN_PIN_BASE_WIN_H_
