// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFDOC_CPDF_BOOKMARK_H_
#define CORE_FPDFDOC_CPDF_BOOKMARK_H_

#include "core/fpdfdoc/cpdf_action.h"
#include "core/fpdfdoc/cpdf_dest.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/widestring.h"

class CPDF_Dictionary;
class CPDF_Document;

class CPDF_Bookmark {
 public:
  CPDF_Bookmark();
  CPDF_Bookmark(const CPDF_Bookmark& that);
  explicit CPDF_Bookmark(RetainPtr<const CPDF_Dictionary> pDict);
  ~CPDF_Bookmark();

  const CPDF_Dictionary* GetDict() const { return m_pDict.Get(); }
  //update on 20240428 返回可修改dict
  CPDF_Dictionary* GetAsMultiDict();
  //
  WideString GetTitle() const;
  //update on 20240422 修改大纲名称
  int SetTitle(WideString outline_name);
  //update on 20240428 删除大纲名称
  int DelTitle(CPDF_IndirectObjectHolder* holder);
  //update on 20240513 添加大纲名称
  int AddTitle(CPDF_Document* doc,
             WideString outline_name,
             int page_index,
             double x,
             double y,
             double zoom);
  //update on 20240514 获取当前大纲指向的页面
  int GetPageIndex(CPDF_Document* doc) const;
  //update on 20240514 获取指定大纲名字的对象指针
  RetainPtr<const CPDF_Dictionary> GetDicItem(const ByteString key);
  //
  CPDF_Dest GetDest(CPDF_Document* pDocument) const;
  CPDF_Action GetAction() const;
  int GetCount() const;

 private:
  RetainPtr<const CPDF_Dictionary> m_pDict;
};

#endif  // CORE_FPDFDOC_CPDF_BOOKMARK_H_
