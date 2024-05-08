// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PUBLIC_FPDF_CATALOG_H_
#define PUBLIC_FPDF_CATALOG_H_

// NOLINTNEXTLINE(build/include)
#include "fpdfview.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * Experimental API.
 *
 * Determine if |document| represents a tagged PDF.
 *
 * For the definition of tagged PDF, See (see 10.7 "Tagged PDF" in PDF
 * Reference 1.7).
 *
 *   document - handle to a document.
 *
 * Returns |true| iff |document| is a tagged PDF.
 */
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFCatalog_IsTagged(FPDF_DOCUMENT document);

//update on 20240429 设置ENT数据
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDF_SetEnt(FPDF_DOCUMENT document,char* ent_data);
//update on 20240429 获取ENT数据
FPDF_EXPORT char* FPDF_CALLCONV 
FPDF_GetEnt(FPDF_DOCUMENT document);
//

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // PUBLIC_FPDF_CATALOG_H_
