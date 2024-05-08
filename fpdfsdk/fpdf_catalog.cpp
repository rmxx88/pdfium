// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/fpdf_catalog.h"

#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
//update on 20240506
#include "core/fpdfapi/parser/cpdf_name.h"
//
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFCatalog_IsTagged(FPDF_DOCUMENT document) {
  CPDF_Document* pDoc = CPDFDocumentFromFPDFDocument(document);
  if (!pDoc)
    return false;

  const CPDF_Dictionary* pCatalog = pDoc->GetRoot();
  if (!pCatalog)
    return false;

  RetainPtr<const CPDF_Dictionary> pMarkInfo = pCatalog->GetDictFor("MarkInfo");
  return pMarkInfo && pMarkInfo->GetIntegerFor("Marked") != 0;
}
// update on 20240429 设置ENT数据
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDF_SetEnt(FPDF_DOCUMENT document,char* ent_data){
  FPDF_BOOL suc = false;
  CPDF_Document* pDoc = CPDFDocumentFromFPDFDocument(document);
  if (!pDoc) {
    return suc;
  }
  CPDF_Dictionary* ent_dic = pDoc->GetMutableRoot();
  if (!ent_dic) {
    return suc;
  }
  ent_dic->SetNewFor<CPDF_Name>("Ent_Data",ent_data);
  suc = true;
  return suc;
}
// update on 20240429 获取ENT数据
FPDF_EXPORT char* FPDF_CALLCONV 
FPDF_GetEnt(FPDF_DOCUMENT document){
  CPDF_Document* pDoc = CPDFDocumentFromFPDFDocument(document);
  if (!pDoc) {
    return nullptr;
  }
  const CPDF_Dictionary* ent_dic = pDoc->GetRoot();
  if (!ent_dic) {
    return nullptr;
  }
  ByteString result = ent_dic->GetNameFor("Ent_Data");
  return (char*)result.c_str();
}