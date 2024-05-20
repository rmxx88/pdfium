/********************************************
* ����������1����ȡPDF�ļ���Ϣ��ҳ��������ҳ��ԭʼ��ߣ�
* 2���½�pageҳ
* 3����PDFҳ�����ͼƬ
* 4���ϲ����pdf �ļ�pageҳ���µ�pdf�ļ�
* 5��������PDF�ļ�
* 6��PDF�ļ��Զ������ݶ�д
* 7������pdfҳ���»��ߡ��л��ߣ�������ע�ͣ���д��path��
* �������ڣ�20240129
* �޸����ڣ�
*********************************************/
#ifndef PUBLIC_SUNIAPDF_CORE_H_
#define PUBLIC_SUNIAPDF_CORE_H_

#include <stddef.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include "fpdfview.h"
#include "fpdf_save.h"
#include "fpdf_edit.h"
#include "fpdf_text.h"
#include "fpdf_ppo.h"
namespace sunia
{
// Exported Class
#ifdef __cplusplus
extern "C" {
#endif
class FPDF_EXPORT PdfCore{
 public:
  /**
  *���캯��
  **/
  PdfCore();
  /**
  * ��������
  **/
  ~PdfCore();
  /**
  * ����pdf�ļ�
  * @pdf_path �ļ�·��
  * @password ����
  * @return documentָ�����
  **/ 
  FPDF_DOCUMENT LoadDocument(FPDF_STRING pdf_path,
                             FPDF_BYTESTRING password = NULL);
  /**
  * ��ȡpdf��ҳ��
  * @doc documentָ�����
  * @return ��ҳ��
  **/
  FPDF_RESULT GetPageCount(FPDF_DOCUMENT doc);
  /**
  * ��ȡҳ����
  * @doc documentָ�����
  * @page_index ҳ����
  * @size ����ҳ���С
  * @return 1���ɹ� 0��ʧ��
  **/
  FPDF_BOOL GetPageSizeByIndex(FPDF_DOCUMENT doc,int page_index, FS_SIZEF& size);
  /**
  * ��ȡָ��ҳ����Ⱦ��λͼ
  * @doc documentָ�����
  * @page_index ҳ������
  * @image_width ͼƬ���
  * @image_height ͼƬ�߶�
  * @first_scan ͼƬָ���׵�ַ
  * @stride һ�еĿ��
  * @return ͼƬ��
  **/
  void* RenderPageToBitmap(FPDF_DOCUMENT doc,
                           int page_index,
                           int image_width,
                           int image_height,
                           void* first_scan,
                           int stride);
  /**
   *��ȡָ��ҳ����Ⱦ��λͼ
   * @doc documentָ�����
   * @page_index ҳ������
   * @image_width ͼƬ���
   * @image_height ͼƬ�߶�
   * @first_scan ͼƬָ���׵�ַ
   * @stride һ�еĿ��
   * @return Bitmap����
   **/
  FPDF_BITMAP RenderPageToBitmapN(FPDF_DOCUMENT doc,
                                  int page_index,
                                  int image_width,
                                  int image_height,
                                  void* first_scan,
                                  int stride);
  /**
  *����λͼ
  * @doc documentָ�����
  * @filename �ļ���
  * @buffer ͼƬ��
  * @width ͼƬ���
  * @height ͼƬ�߶�
  **/
  void SaveBitmapToFile(FPDF_DOCUMENT doc,const char* filename,
                        const unsigned char* buffer,
                        int width,
                        int height);
  /**
  * ����λͼ
  * @doc documentָ�����
  * @page_index ҳ����
  * @image_windth ͼƬ���
  * @image_height ͼƬ�߶�
  * @bitmap_data ͼƬ��
  * @return 1:�ɹ� 0��ʧ��
  **/
  FPDF_BOOL InsertBitmapToPage(FPDF_DOCUMENT doc,int page_index,
                               FS_POINTF position,
                               float image_width,
                               float image_heigth,
                               const unsigned char* bitmap_data);
  /**
  * �½�pdfҳ
  * @doc documentָ�����
  * @page_index ҳ����
  * @width ҳ��
  * @height ҳ��
  * @return pageָ�����
  **/
  FPDF_PAGE NewPdfPage(FPDF_DOCUMENT doc,
                           int page_index,
                           int width,
                           int height);
  /**
  * ����pdf���� �ڵ��ñ���ӿں�ˢ�»���֮ǰ����
  * @page pageָ�����
  * @return 1���ɹ� 0ʧ��
  **/
  FPDF_BOOL GenerateContent(FPDF_PAGE page);
  /**
  * ���Ϊ/�½�pdf
  * @doc documentָ�����
  * @file_path �ļ���
  **/
  FPDF_BOOL SavePdf(FPDF_DOCUMENT doc,const char* file_path);
  /**
  * �½�pdf
  * @return documentָ�����
  **/
  FPDF_DOCUMENT CreatePdf();
  /**
  * �����pdf�ļ��е�ҳ���һ���µ�pdf�ļ�
  * @obj_doc Ŀ��documentָ�����
  * @src_doc Դdocumentָ�����
  * @indexs ҳ��������
  * @size ���鳤��
  * @return documentָ�����
  **/
  FPDF_DOCUMENT CombinePdf(FPDF_DOCUMENT obj_doc,
                       FPDF_DOCUMENT src_doc,
                       int* indexs,
                       int size);
  /**
  * �����pdf�ļ��е�ҳ���һ���µ�pdf�ļ�
  * @obj_doc Ŀ��documentָ�����
  * @src_doc Դdocumentָ�����
  * @indexs ҳ�����ַ�������-�ָ1-3��
  * @start Ŀ��document����λ������
  * @return documentָ�����
  **/ 
  FPDF_DOCUMENT CombinePdf(FPDF_DOCUMENT obj_doc,
                           FPDF_DOCUMENT src_doc,
                           const char* indexs,
                           int start);
  /**
  * ����pdfҳ��λ��
  * @doc documentָ�����
  * @page_indices ҳ��������
  * @page_indices_len ���鳤��
  * @dest_page_index ������λ��
  * @return 1���ɹ� 0��ʧ��
  **/
  FPDF_BOOL SwapPdfPage(FPDF_DOCUMENT doc,const int* page_indices,
                        unsigned long page_indices_len,
                        int dest_page_index);
  /**
  * �ر��ĵ�
  * @doc documentָ�����
  **/
  void CloseDocument(FPDF_DOCUMENT doc);
  /**
  * �ı�pdfҳ��С
  * @src_page pageָ�����
  * @width ҳ��
  * @height ҳ��
  * @return 1���ɹ� 0��ʧ��
  **/
  FPDF_BOOL ResizePdfPage(FPDF_PAGE src_page,
                          double width,
                          double height);
  /**
  * ��ȡָ��ҳ����
  * @doc documentָ�����
  * @index ҳ����
  * @return pageָ�����
  **/
  FPDF_PAGE GetPdfPage(FPDF_DOCUMENT doc, int index);
  /**
  * ��ȡpageҳ�ı�����
  * @page pageָ�����
  * @obj ҳ����
  * @return �ַ�����
  **/
  std::string GetPageContent(FPDF_PAGE page, FPDF_PAGEOBJECT obj);
  /**
  * ��bgraתrgba 0xFF000000 ->red
  * С��ģʽ��RGBA��R�洢�ڵ�λ��A�洢�ڸ�λ
  * @buffer ͼƬ��
  * @width ͼƬ��
  * @height ͼƬ��
  **/
  void Bgra2RgbaSmall(unsigned char* buffer, int width, int height);
  /**
   * ��rgbaתbgra 0xFF000000 ->red
   * С��ģʽ��RGBA��R�洢�ڵ�λ��A�洢�ڸ�λ
   * @buffer ͼƬ��
   * @width ͼƬ��
   * @height ͼƬ��
   **/
  void Rgba2BgraSmall(unsigned char* buffer, int width, int height);
  /**
   * ��bgraתrgba 0x000000FF ->red
   * ���ģʽ��RGBA��R�洢�ڸ�λ��A�洢�ڵ�λ
   * @buffer ͼƬ��
   * @width ͼƬ��
   * @height ͼƬ��
   **/
  void Bgra2RgbaBig(unsigned char* buffer, int width, int height);
  /**
   * ��rgbaתbgra 0x000000FF ->red
   * ���ģʽ��RGBA��R�洢�ڸ�λ��A�洢�ڵ�λ
   * @buffer ͼƬ��
   * @width ͼƬ��
   * @height ͼƬ��
   **/
  void Rgba2BgraBig(unsigned char* buffer, int width, int height);
  //���Դ��� ������һ�ε���
  FPDF_BOOL test(FPDF_PAGE page);
  //��ӻ�������
  FPDF_BOOL InsertPathToPage(FPDF_PAGE page,unsigned int R,unsigned int G,
                             unsigned int B,unsigned int A,float pen_width,
                             FS_QUADPOINTSF* points,
                             int length);
 private:
  /**
  * ����Ҫ���ŵĶ������ע�͡��»��ߡ��л����Լ��ֻ�����
  * @page pageָ�����
  * @x_scale x��������ֵ
  * @y_scale y��������ֵ
  * @return 1���ɹ� 0��ʧ��
  **/
  FPDF_BOOL ResizePdfPageObject(FPDF_PAGE page, double x_scale, double y_scale);
  /**
  * �����ֻ����
  * @page pageָ�����
  * @x_scale x��������ֵ
  * @y_scale y��������ֵ
  * @return 1���ɹ� 0��ʧ��
  **/
  void ScanPagePathObj(FPDF_PAGE page, double x_scale, double y_scale);
  /**
   * ����Ƭ�ζ���
   * @page pageָ�����
   * @x_scale x��������ֵ
   * @y_scale y��������ֵ
   * @return 1���ɹ� 0��ʧ��
   **/
  void ScalePageSegmentsObj(FPDF_PAGE page, double x_scale, double y_scale);
  /**
   * �����л��ߺ��»���
   * @page pageָ�����
   * @x_scale x��������ֵ
   * @y_scale y��������ֵ
   * @return 1���ɹ� 0��ʧ��
   **/
  void ScalePageAnnotationObj(FPDF_PAGE page, double x_scale, double y_scale);
  
  // ���Զ���
  std::unique_ptr<std::vector<FS_POINTF>> path_buffer_test_ =
      std::make_unique<std::vector<FS_POINTF>>(1000);
};
#ifdef __cplusplus
}
#endif
}
#endif