
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
  
  void Bgra2RgbaSmall(unsigned char* buffer, int width, int height);
  void Rgba2BgraSmall(unsigned char* buffer, int width, int height);
  
  void Bgra2RbgaBig(unsigned char* buffer, int width, int height);
  void Rgba2BgraBig(unsigned char* buffer, int width, int height);
};
#ifdef __cplusplus
}
#endif
}
#endif
