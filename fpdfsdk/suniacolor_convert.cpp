#include "suniacolor_convert.h"
namespace sunia {
#ifdef __cplusplus
extern "C" {
#endif
ColorConvert::ColorConvert() {

}
ColorConvert::~ColorConvert() {

}

void ColorConvert::Bgra2RgbaSmall(unsigned char* buffer, int width, int height) {
  unsigned int* rbga = (unsigned int*)buffer;
  for (int i = 0; i < width * height; i++) {
    rbga[i] = (rbga[i] & 0xFF000000) |          // ______AA
              ((rbga[i] & 0x00FF0000) >> 16) |  // RR______
              (rbga[i] & 0x0000FF00) |          // __GG____
              ((rbga[i] & 0x000000FF) << 16);   // ____BB__
  }
}

void ColorConvert::Rgba2BgraSmall(unsigned char* buffer, int width, int height) {
  unsigned int* bgra = (unsigned int*)buffer;
  for (int i = 0; i < width * height; i++) {
    bgra[i] = (bgra[i] & 0xFF000000) |          // ______AA
              ((bgra[i] & 0x00FF0000) >> 16) |  // BB______
              (bgra[i] & 0x0000FF00) |          // __GG____
              ((bgra[i] & 0x000000FF) << 16);   // ____RR__
  }
}

void ColorConvert::Bgra2RbgaBig(unsigned char* buffer, int width, int height) {
  unsigned int* rbga = (unsigned int*)buffer;
  for (int i = 0; i < width * height; i++) {
    rbga[i] = (rbga[i] & 0x000000FF) |          // ______AA
              ((rbga[i] & 0x0000FF00) << 16) |  // RR______
              (rbga[i] & 0x00FF0000) |          // __GG____
              ((rbga[i] & 0xFF000000) >> 16);   // ____BB__
  }
}

void ColorConvert::Rgba2BgraBig(unsigned char* buffer, int width, int height) {
  unsigned int* bgra = (unsigned int*)buffer;
  for (int i = 0; i < width * height; i++) {
    bgra[i] = (bgra[i] & 0x000000FF) |          // ______AA
              ((bgra[i] & 0x0000FF00) << 16) |  // BB______
              (bgra[i] & 0x00FF0000) |          // __GG____
              ((bgra[i] & 0xFF000000) >> 16);   // ____RR__
  }
}
#ifdef __cplusplus
}
#endif
}  // namespace sunia
