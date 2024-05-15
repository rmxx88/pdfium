// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfdoc/cpdf_bookmark.h"

#include <utility>

#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxge/dib/fx_dib.h"
//update on 20240428
#include "fpdfsdk/suniastring_base.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfdoc/cpdf_dest.h"
#include "core/fpdfapi/parser/cpdf_indirect_object_holder.h"
//
CPDF_Bookmark::CPDF_Bookmark() = default;

CPDF_Bookmark::CPDF_Bookmark(const CPDF_Bookmark& that) = default;

CPDF_Bookmark::CPDF_Bookmark(RetainPtr<const CPDF_Dictionary> pDict)
    : m_pDict(std::move(pDict)) {}

CPDF_Bookmark::~CPDF_Bookmark() = default;

WideString CPDF_Bookmark::GetTitle() const {
  if (!m_pDict)
    return WideString();

  RetainPtr<const CPDF_String> pString =
      ToString(m_pDict->GetDirectObjectFor("Title"));
  if (!pString)
    return WideString();

  WideString title = pString->GetUnicodeText();
  size_t len = title.GetLength();
  if (!len)
    return WideString();

  DataVector<wchar_t> buf(len);
  for (size_t i = 0; i < len; i++) {
    wchar_t w = title[i];
    buf[i] = w > 0x20 ? w : 0x20;
  }
  return WideString(buf.data(), len);
}
// update on 20240422 修改大纲名称
int CPDF_Bookmark::SetTitle(WideString outline_name) {
  int suc = 0;
  if (!m_pDict) {
    return suc;
  }
  RetainPtr<CPDF_Dictionary> dic(
      const_cast<CPDF_Dictionary*>(m_pDict.Get()));
  RetainPtr<CPDF_Object> obj = dic->GetMutableDirectObjectFor("Title");
  RetainPtr<CPDF_String> convert_str =
      pdfium::MakeRetain<CPDF_String>(nullptr, outline_name.c_str());
  obj->SetString(convert_str->GetString());
  suc = 1;
  return suc;
}
// update on 20240428 删除大纲名称
int CPDF_Bookmark::DelTitle(CPDF_IndirectObjectHolder* holder) {
  int suc = 0;
  if (!m_pDict) {
    return suc;
  }
  RetainPtr<CPDF_Dictionary> dic(GetAsMultiDict());
  //前向节点
  RetainPtr<CPDF_Dictionary> prev_bookmark = dic->GetMutableDictFor("Prev");
  //后向节点
  RetainPtr<CPDF_Dictionary> next_bookmark = dic->GetMutableDictFor("Next");
  if (prev_bookmark) {
    if (!next_bookmark) {
      prev_bookmark->RemoveFor("Next");
    } else {
      prev_bookmark->SetNewFor<CPDF_Reference>("Next", holder,
                                               next_bookmark->GetObjNum());
    }
  }
  if (next_bookmark) {
    if (!prev_bookmark) {
      next_bookmark->RemoveFor("Prev");
    } else {
      next_bookmark->SetNewFor<CPDF_Reference>("Prev", holder,
                                               prev_bookmark->GetObjNum());
    }
    
  }
  suc = true;
  //
  return suc;
}
// update on 20240513 添加大纲名称
int CPDF_Bookmark::AddTitle(CPDF_Document* doc,
                            WideString outline_name,
                            int page_index,
                            double x,
                            double y,
                            double zoom) {
  int suc = 0;
  CPDF_IndirectObjectHolder* holder = (CPDF_IndirectObjectHolder*)doc;
  RetainPtr<CPDF_Dictionary> add_bookmark = holder->NewIndirect<CPDF_Dictionary>();
  RetainPtr<CPDF_String> convert_str =
      pdfium::MakeRetain<CPDF_String>(nullptr, outline_name.c_str());
  add_bookmark->SetNewFor<CPDF_String>("Title", L"");

  RetainPtr<CPDF_Dictionary> dic(GetAsMultiDict());
  RetainPtr<CPDF_Dictionary> parent_bookmark = dic->GetMutableDictFor("Parent");
  RetainPtr<CPDF_Dictionary> next_bookmark = dic->GetMutableDictFor("Next");
  if (!parent_bookmark) {
    return suc;
  }
  //指向前向节点
  add_bookmark->SetNewFor<CPDF_Reference>("Prev", holder,
                                          dic->GetObjNum());
  //指向父节点
  add_bookmark->SetNewFor<CPDF_Reference>("Parent", holder,
                                          parent_bookmark->GetObjNum());
  RetainPtr<CPDF_Object> obj = add_bookmark->GetMutableDirectObjectFor("Title");
  obj->SetString(convert_str->GetString());
  //设置点击新增大纲跳转到对应位置
  RetainPtr<CPDF_Array> array =  holder->NewIndirect<CPDF_Array>();
  array->AppendNew<CPDF_Number>(page_index);  // 页索引
  array->AppendNew<CPDF_Name>("XYZ");
  array->AppendNew<CPDF_Number>((float)x);  // X坐标
  array->AppendNew<CPDF_Number>((float)y);  // Y坐标
  array->AppendNew<CPDF_Number>((float)zoom);  // Zoom 缩放
  //CPDF_Dest::Create(doc, m_pDict->GetDirectObjectFor("Dest"))
  add_bookmark->SetNewFor<CPDF_Reference>("Dest", holder, array->GetObjNum());
  //
  //指向后向节点
  add_bookmark->SetNewFor<CPDF_Reference>("Next", holder,
                                          next_bookmark->GetObjNum());
  //将前向节点的next指向新增的节点
  dic->SetNewFor<CPDF_Reference>("Next", holder,
                                 add_bookmark->GetObjNum());
  //将后向节点的prev指向新增的节点
  RetainPtr<CPDF_Dictionary> next_prev_bookmark = next_bookmark->GetMutableDictFor("Prev");
  if (next_prev_bookmark) {
    next_prev_bookmark->SetNewFor<CPDF_Reference>("Prev", holder,
                                                  add_bookmark->GetObjNum());
  }
  suc = 1;
  return suc;
}
//
// update on 20240428 返回可修改dict
CPDF_Dictionary* CPDF_Bookmark::GetAsMultiDict() {
  RetainPtr<CPDF_Dictionary> dic(const_cast<CPDF_Dictionary*>(m_pDict.Get()));
  return dic;
}
//
CPDF_Dest CPDF_Bookmark::GetDest(CPDF_Document* pDocument) const {
  if (!m_pDict)
    return CPDF_Dest(nullptr);
  return CPDF_Dest::Create(pDocument, m_pDict->GetDirectObjectFor("Dest"));
}

CPDF_Action CPDF_Bookmark::GetAction() const {
  return CPDF_Action(m_pDict ? m_pDict->GetDictFor("A") : nullptr);
}

int CPDF_Bookmark::GetCount() const {
  return m_pDict->GetIntegerFor("Count");
}
// 获取当前大纲指向的页面
int CPDF_Bookmark::GetPageIndex(CPDF_Document* doc) const {
  int index = 0;
  CPDF_Dest dest = GetDest(doc);
  const CPDF_Array* array = dest.GetArray();
  if (array)
  index = array->GetIntegerAt(0);
  return index;
}
// update on 20240514 获取指定大纲名字的对象指针
RetainPtr<const CPDF_Dictionary> CPDF_Bookmark::GetDicItem(const ByteString key){
  return m_pDict->GetDictFor(key);
}