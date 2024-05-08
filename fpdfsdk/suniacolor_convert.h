/********************************************
* 功能描述：1、rgba与bgra互转
*
* 创建日期：20240129
* 修改日期：
*********************************************/
#ifndef FPDFSDK_SUNIACOLOR_CONVERT_H_
#define FPDFSDK_SUNIACOLOR_CONVERT_H_
//#pragma once
#include <stddef.h>
#include <stdio.h>
#include <iostream>
namespace sunia
{
// Exported Class
#ifdef __cplusplus
extern "C" {
#endif
class ColorConvert{
 public:
  ColorConvert();
  ~ColorConvert();
  //小端模式，RGBA中R存储在低位，A存储在高位
  void Bgra2RgbaSmall(unsigned char* buffer, int width, int height);
  void Rgba2BgraSmall(unsigned char* buffer, int width, int height);
  // 大端模式，RGBA中R存储在高位，A存储在低位
  void Bgra2RbgaBig(unsigned char* buffer, int width, int height);
  void Rgba2BgraBig(unsigned char* buffer, int width, int height);
};
#ifdef __cplusplus
}
#endif
}
#endif