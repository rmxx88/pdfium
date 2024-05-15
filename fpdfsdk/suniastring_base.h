/********************************************
* ����������1�����ַ������ַ����໥ת��
*
* �������ڣ�20240129
* �޸����ڣ�
*********************************************/
#ifndef FPDFSDK_SUNIASTRING_BASE_H_
#define FPDFSDK_SUNIASTRING_BASE_H_
//#pragma once
#include <stddef.h>
#include <stdio.h>
#include <iostream>
#include <memory>
#include <vector>
#include <public/fpdfview.h>
namespace sunia
{
struct FreeDeleter {
  inline void operator()(void* ptr) const { free(ptr); }
};
using ScopedFPDFWideString = std::unique_ptr<FPDF_WCHAR, sunia::FreeDeleter>;
// Exported Class
#ifdef __cplusplus
extern "C" {
#endif
class StringBase{
 public:
  StringBase();
  ~StringBase();
  std::vector<std::string> StringSplit(const std::string& str,
                                                   char delimiter);
  std::string GetPlatformString(FPDF_WIDESTRING wstr);
  std::wstring GetPlatformWString(FPDF_WIDESTRING wstr);
  //将宽字节转换成pdf内部16进制
  ScopedFPDFWideString GetFPDFWideString(const std::wstring& wstr);
};
#ifdef __cplusplus
}
#endif
}
#endif