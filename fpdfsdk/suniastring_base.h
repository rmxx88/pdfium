/********************************************
* 功能描述：1、宽字符串与字符串相互转换
*
* 创建日期：20240129
* 修改日期：
*********************************************/
#ifndef FPDFSDK_SUNIASTRING_BASE_H_
#define FPDFSDK_SUNIASTRING_BASE_H_
//#pragma once
#include <stddef.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <public/fpdfview.h>
namespace sunia
{
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
};
#ifdef __cplusplus
}
#endif
}
#endif