/********************************************
* ����������1���ļ���д
*
* �������ڣ�20240129
* �޸����ڣ�
*********************************************/
#ifndef FPDFSDK_SUNIAFILE_BASE_H_
#define FPDFSDK_SUNIAFILE_BASE_H_
//#pragma once
#include <stddef.h>
#include <stdio.h>
#include <iostream>
#include <public/fpdf_save.h>
namespace sunia
{
// Exported Class
#ifdef __cplusplus
extern "C" {
#endif
// update on 20240217 д�ļ��� ���ڱ���pdf�ļ�ʱ
struct PdfWrite : public FPDF_FILEWRITE {
  PdfWrite(const char* filename) {
    this->file = fopen(filename, "wb");
    version = 1;
    WriteBlock = WriteBlockCallback;
  }
  ~PdfWrite() {
    if (this->file) {
      fclose(this->file);
    }
  }
  static int FPDF_CALLCONV WriteBlockCallback(FPDF_FILEWRITE* pThis,
                                              const void* data,
                                              unsigned long size) {
    PdfWrite* writer = static_cast<PdfWrite*>(pThis);
    fwrite(data, 1, size, writer->file);
    return (int)size;
  }
  FILE* file = nullptr;
};
class FileBase{
 public:
  FileBase();
  ~FileBase();
  std::string ReadFile(const char* path);
  bool WriteFile(const char* path, char* buffers);
};
#ifdef __cplusplus
}
#endif
}
#endif