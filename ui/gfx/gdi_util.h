// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_GDI_UTIL_H_
#define UI_GFX_GDI_UTIL_H_

#include <windows.h>

#include "base/component_export.h"
#include "ui/gfx/geometry/rect.h"

namespace gfx {

// Creates a BITMAPV4HEADER structure given the bitmap's size.  You probably
// only need to use BMP V4 if you need transparency (alpha channel). This
// function sets the AlphaMask to 0xff000000, meaning this header is for a
// 32-bits-per-pixel ARGB8888 bitmap.
COMPONENT_EXPORT(GFX)
void CreateBitmapV4HeaderForARGB888(int width, int height, BITMAPV4HEADER* hdr);

// Calculate scale to fit an entire page on DC.
COMPONENT_EXPORT(GFX)
float CalculatePageScale(HDC dc, int page_width, int page_height);

// Apply scaling to the DC.
COMPONENT_EXPORT(GFX) bool ScaleDC(HDC dc, float scale_factor);

}  // namespace gfx

#endif  // UI_GFX_GDI_UTIL_H_
