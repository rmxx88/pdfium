// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXBARCODE_CBC_UPCA_H_
#define XFA_FXBARCODE_CBC_UPCA_H_

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxge/include/fx_dib.h"
#include "xfa/fxbarcode/cbc_onecode.h"

class CBC_UPCA : public CBC_OneCode {
 public:
  CBC_UPCA();
  ~CBC_UPCA() override;

  // CBC_CodeBase
  FX_BOOL Encode(const CFX_WideStringC& contents,
                 FX_BOOL isDevice,
                 int32_t& e) override;
  FX_BOOL RenderDevice(CFX_RenderDevice* device,
                       const CFX_Matrix* matrix,
                       int32_t& e) override;
  FX_BOOL RenderBitmap(CFX_DIBitmap*& pOutBitmap, int32_t& e) override;
  BC_TYPE GetType() override;

 private:
  CFX_WideString Preprocess(const CFX_WideStringC& contents);
  CFX_WideString m_renderContents;
};

#endif  // XFA_FXBARCODE_CBC_UPCA_H_
