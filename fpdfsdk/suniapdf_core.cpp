#include "public/suniapdf_core.h"
#include <vector>
#include "fpdfsdk/suniacolor_convert.h"
#include "fpdfsdk/suniafile_base.h"
#include "fpdfsdk/suniastring_base.h"
#include "public//fpdf_transformpage.h"
#include "public/fpdf_annot.h"
#include "public/fpdf_catalog.h"
#include "public/fpdf_doc.h"
#include "constants/annotation_common.h"
//
namespace sunia {
#ifdef __cplusplus
extern "C" {
#endif
PdfCore::PdfCore() {
  FPDF_InitLibrary();

  // FileBase bs;
  // std::string result = bs.ReadFile("C:\\Users\\Sunia\\Desktop\\6.txt");
  // std::cout << "ReadFile result:" << result << std::endl;
  // auto suc = bs.WriteFile("test.txt", (char*)result.c_str());
  // std::cout << "WriteFile result:" << suc << std::endl;/*  */
}
PdfCore::~PdfCore() {
  FPDF_DestroyLibrary();
}
void PdfCore::CloseDocument(FPDF_DOCUMENT doc) {
  FPDF_CloseDocument(doc);
}
// ����pdf�ļ�
FPDF_DOCUMENT PdfCore::LoadDocument(FPDF_STRING pdf_path,
                                    FPDF_BYTESTRING password) {
  if (!is_authentication_) {
    return nullptr;
  }
  if (pdf_path == nullptr) {
    return nullptr;
  }
  FPDF_DOCUMENT doc = FPDF_LoadDocument(pdf_path, password);
  return doc;
}
// ��ȡpdf��ҳ��
FPDF_RESULT PdfCore::GetPageCount(FPDF_DOCUMENT doc) {
  FPDF_RESULT page_count = 0;
  if (!is_authentication_) {
    return page_count;
  }
  if (doc != nullptr) {
    page_count = FPDF_GetPageCount(doc);
  }
  return page_count;
}
// ��ȡҳ�����
FPDF_BOOL PdfCore::GetPageSizeByIndex(FPDF_DOCUMENT doc,
                                      int page_index,
                                      FS_SIZEF& size) {
  FPDF_BOOL suc = false;
  if (!is_authentication_) {
    return suc;
  }
  if (doc != nullptr) {
    suc = FPDF_GetPageSizeByIndexF(doc, page_index, &size);
  }
  return suc;
}
// ����λͼ
void PdfCore::SaveBitmapToFile(FPDF_DOCUMENT doc,
                               const char* filename,
                               const unsigned char* buffer,
                               int width,
                               int height) {
  if (!is_authentication_) {
    return;
  }
  FILE* file = fopen(filename, "wb");
  if (file) {
    fwrite(buffer, 1, width * height * 4, file);  // Assuming 32-bit RGBA format
    fclose(file);
  } else {
    fprintf(stderr, "Failed to open file for writing: %s\n", filename);
  }
}
// ��ȡָ��ҳ��ָ��λ��λͼ
void* PdfCore::RenderPageToBitmap(FPDF_DOCUMENT doc,
                                  int page_index,
                                  int image_width,
                                  int image_height,
                                  void* first_scan,
                                  int stride) {
  void* bitmap_bufeers = nullptr;
  if (!is_authentication_) {
    return bitmap_bufeers;
  }
  if (doc != nullptr) {
    FPDF_BITMAP bitmap;
    auto* page = FPDF_LoadPage(doc, page_index);
    if (page != nullptr) {
      // bitmap = FPDFBitmap_Create(pageWidth, pageHeight, 0);
      bitmap = FPDFBitmap_CreateEx(image_width, image_height, FPDFBitmap_BGRA,
                                   first_scan, stride);
      /* FPDFBitmap_FillRect(bitmap, 0, 0, pageWidth,
                           pageHeight, 0xFFFFFFFF);*/
      // ��Ⱦҳ�浽λͼ
      FPDF_RenderPageBitmap(bitmap, page, 0, 0, image_width, image_height, 0,
                            /*FPDF_ANNOT*/ 0);
      // ��ȡλͼ������
      bitmap_bufeers = FPDFBitmap_GetBuffer(bitmap);
      FPDF_ClosePage(page);
    }
  }
  return bitmap_bufeers;
}
// ��ȡָ��ҳ��ָ��λ��λͼ
FPDF_BITMAP PdfCore::RenderPageToBitmapN(FPDF_DOCUMENT doc,
                                         int page_index,
                                         int image_width,
                                         int image_height,
                                         void* first_scan,
                                         int stride) {
  FPDF_BITMAP bitmap = nullptr;
  if (!is_authentication_) {
    return bitmap;
  }
  if (doc != nullptr) {
    auto* page = FPDF_LoadPage(doc, page_index);
    if (page != nullptr) {
      bitmap = FPDFBitmap_CreateEx(image_width, image_height, FPDFBitmap_BGRA,
                                   first_scan, stride);
      /* FPDFBitmap_FillRect(bitmap, 0, 0, pageWidth,
                           pageHeight, 0xFFFFFFFF);*/
      // ��Ⱦҳ�浽λͼ
      FPDF_RenderPageBitmap(bitmap, page, 0, 0, image_width, image_height, 0,
                            FPDF_ANNOT);

      FPDF_ClosePage(page);
    }
  }
  return bitmap;
}
// ����λͼ
FPDF_BOOL PdfCore::InsertBitmapToPage(FPDF_DOCUMENT doc,
                                      int page_index,
                                      FS_POINTF position,
                                      float image_width,
                                      float image_heigth,
                                      const unsigned char* bitmap_data,
                                      FPDF_BOOL visible_image) {
  FPDF_BOOL suc = false;
  if (!is_authentication_) {
    return suc;
  }
  FPDF_PAGEOBJECT page_obj = FPDFPageObj_NewImageObj(doc, visible_image);
  auto* page = GetPdfPage(doc, page_index);
  FPDF_BITMAP pdfBitmap =
      FPDFBitmap_CreateEx(image_width, image_heigth, FPDFBitmap_BGRA,
                          (void*)bitmap_data, image_width * 4);
  suc = FPDFImageObj_SetBitmap(&page, 1, page_obj, pdfBitmap);
  FS_RECTF rc(0, 0, 0, 0);
  suc = FPDFPage_GetCropBox(page, &rc.left, &rc.bottom, &rc.right, &rc.top);
  position.x += rc.left;
  position.y -= rc.bottom;
  float page_height = FPDF_GetPageHeightF(page);
  FS_MATRIX matrix{0};
  matrix.a = image_width;
  matrix.d = image_heigth;
  matrix.e = position.x;
  matrix.f = page_height - image_heigth - position.y;
  // ��������ͼƬ��С
  suc = FPDFImageObj_SetMatrix(page_obj, matrix.a, matrix.b, matrix.c, matrix.d,
                               matrix.e, matrix.f);
  FPDFPage_InsertObject(page, page_obj);
  suc = FPDFPage_GenerateContent(page);
  return suc;
}
// �½�pdfҳ
FPDF_PAGE PdfCore::NewPdfPage(FPDF_DOCUMENT doc,
                              int page_index,
                              int width,
                              int height) {
  FPDF_BOOL suc = false;
  FPDF_PAGE page = nullptr;
  if (!is_authentication_) {
    return page;
  }
  if (doc != nullptr) {
    if (page_index < 0) {
      page_index = 0;
    }
    auto page_count = FPDF_GetPageCount(doc);
    if (page_index > page_count) {
      page_index = page_count;
    }
    page = FPDFPage_New(doc, page_index, width, height);
    suc = FPDFPage_GenerateContent(page);
    printf("FPDFPage_New result:%d\n", suc);
  }
  return page;
}
// ����pdf���� �ڵ��ñ���ӿں�ˢ�»���֮ǰ����
FPDF_BOOL PdfCore::GenerateContent(FPDF_PAGE page) {
  FPDF_BOOL suc = false;
  if (!is_authentication_) {
    return suc;
  }
  if (page != nullptr) {
    suc = FPDFPage_GenerateContent(page);
  }
  return suc;
}
// ����Ϊ/�½�pdf
FPDF_BOOL PdfCore::SavePdf(FPDF_DOCUMENT doc, const char* file_path) {
  FPDF_BOOL suc = false;
  if (!is_authentication_) {
    return suc;
  }
  PdfWrite write(file_path);
  suc = FPDF_SaveAsCopy(doc, &write, /* FPDF_INCREMENTAL*/ 0);
  // �ƶ�ҳ��
  // FPDF_MovePages
  return suc;
}
// update on 20240217 ����pdfҳ˳��
FPDF_BOOL PdfCore::SwapPdfPage(FPDF_DOCUMENT doc,
                               const int* page_indices,
                               unsigned long page_indices_len,
                               int dest_page_index) {
  FPDF_BOOL suc = false;
  if (!is_authentication_) {
    return suc;
  }
  suc = FPDF_MovePages(doc, page_indices, page_indices_len, dest_page_index);
  return suc;
}
// �½�pdf
FPDF_DOCUMENT PdfCore::CreatePdf() {
  if (!is_authentication_) {
    return nullptr;
  }
  return FPDF_CreateNewDocument();
}
// �����pdf�ļ��е�ҳ���һ���µ�pdf�ļ�
FPDF_DOCUMENT PdfCore::CombinePdf(FPDF_DOCUMENT obj_doc,
                                  FPDF_DOCUMENT src_doc,
                                  int* indexs,
                                  int size) {
  if (!is_authentication_) {
    return nullptr;
  }
  FPDF_BOOL suc = false;
  suc = FPDF_ImportPagesByIndex(obj_doc, src_doc, indexs, size, 0);
  printf("FPDF_ImportPagesByIndex result:%d", suc);
  return obj_doc;
}
// �����pdf�ļ��е�ҳ���һ���µ�pdf�ļ�
FPDF_DOCUMENT PdfCore::CombinePdf(FPDF_DOCUMENT obj_doc,
                                  FPDF_DOCUMENT src_doc,
                                  const char* indexs,
                                  int start) {
  if (!is_authentication_) {
    return nullptr;
  }
  FPDF_BOOL suc = false;
  suc = FPDF_ImportPages(obj_doc, src_doc, indexs, start);
  printf("FPDF_ImportPages result:%d\n", suc);
  return obj_doc;
}
/**
 * 改变pdf页大小,增加留白边距
 * @src_page page指针对象
 * @width 页宽
 * @height 页高
 * @horizontal_margin 水平边距
 * @vertical_margin 垂直边距
 * @return 1：成功 0：失败
 **/
FPDF_BOOL PdfCore::ResizePdfPage(FPDF_PAGE src_page,
                                 double width,
                                 double height,
                                 double horizontal_margin,
                                 double vertical_margin) {
  FPDF_BOOL suc = false;
  if (!is_authentication_) {
    return suc;
  }
  if (src_page == nullptr) {
    return suc;
  }
  double old_width = FPDF_GetPageWidthF(src_page);
  double old_height = FPDF_GetPageHeightF(src_page);
  FS_MATRIX matrix{0};
  matrix.a = width / old_width;
  matrix.d = height / old_height;
  // update on 20240508 增加留白宽度
  matrix.e = horizontal_margin / 2;
  matrix.f = vertical_margin / 2;
  //
  FS_RECTF page_bounding_rect = {0, 0, 0, 0};
  suc = FPDF_GetPageBoundingBox(src_page, &page_bounding_rect);
  // ����path
  // ScanPagePathObj(src_page, matrix.a, matrix.d);
  // ����Ƭ��
  // ScalePageSegmentsObj(src_page, matrix.a, matrix.d);
  // �����»��ߣ��л��ߣ���������
  ScalePageAnnotationObj(src_page, matrix.a, matrix.d);
  // ����ע�Ͷ���
  // ResizePdfPageObject(src_page, matrix.a, matrix.d);
  // ˢ���ֻ����
  FPDFPage_TransformAnnots(src_page, matrix.a, matrix.b, matrix.c, matrix.d,
                           matrix.e, matrix.f);
  // ����pdf
  if (page_bounding_rect.left != 0 || page_bounding_rect.bottom != 0) {
    float x_scale = matrix.a;
    float y_scale = matrix.d;
    if (matrix.a > 1.0f)  // x
    {
      x_scale = (old_width - width) / width;
    }
    if (matrix.d > 1.0f)  // y
    {
      y_scale = (old_height - height) / height;
    }
    matrix.e = page_bounding_rect.left * x_scale;
    matrix.f = page_bounding_rect.bottom * y_scale;
  }
  suc = FPDFPage_TransFormWithClip(src_page, &matrix, nullptr);
  suc = FPDFPage_GenerateContent(src_page);
  //
  FPDFPage_SetCropBox(src_page, page_bounding_rect.left,
                      page_bounding_rect.bottom,
                      width + horizontal_margin + page_bounding_rect.left,
                      height + vertical_margin + page_bounding_rect.bottom);
  if (matrix.a > 1 || matrix.d > 1)  // �Ŵ�
  {
    FPDFPage_SetMediaBox(src_page, page_bounding_rect.left,
                         page_bounding_rect.bottom,
                         width + horizontal_margin + page_bounding_rect.left,
                         height + vertical_margin + page_bounding_rect.bottom);
  }
  suc = FPDFPage_GenerateContent(src_page);
  return suc;
}
// ����Ҫ���ŵ�ע�Ͷ���
FPDF_BOOL PdfCore::ResizePdfPageObject(FPDF_PAGE page,
                                       double x_scale,
                                       double y_scale) {
  FPDF_BOOL suc = false;
  if (!is_authentication_) {
    return suc;
  }
  // 1 ��ȡ����ҳע�͸���
  int annotation_count = FPDFPage_GetAnnotCount(page);
  for (int i = 0; i < annotation_count; i++) {
    // ��ȡ����ע�Ͷ���
    FPDF_ANNOTATION annotation = FPDFPage_GetAnnot(page, i);
    auto type = FPDFAnnot_GetSubtype(annotation);
    if (type == FPDF_ANNOT_INK) {
      /*suc = FPDFAnnot_SetColor(annotation, FPDFANNOT_COLORTYPE_Color, 0,
                         255,0,255,true);*/
      std::cout << "FPDFAnnot_GetSubtype type:" << FPDF_ANNOT_INK
                << ",suc:" << suc << std::endl;
      continue;
    }
    FS_RECTF rc{0, 0, 0, 0};
    suc = FPDFAnnot_GetRect(annotation, &rc);
    rc.left *= x_scale;
    rc.bottom *= y_scale;
    rc.right *= x_scale;
    rc.top *= y_scale;
    FPDFAnnot_SetRect(annotation, &rc);
    FPDFPage_CloseAnnot(annotation);
    suc = FPDFPage_GenerateContent(page);
  }
  return suc;
}
// ��������ҳobject path ink����
void PdfCore::ScanPagePathObj(FPDF_PAGE page, double x_scale, double y_scale) {
  if (!is_authentication_) {
    return;
  }
  auto count = FPDFPage_GetAnnotCount(page);
  FPDF_BOOL suc = false;
  for (int i = 0; i < count; i++) {
    FPDF_ANNOTATION annotation = FPDFPage_GetAnnot(page, i);
    auto type = FPDFAnnot_GetSubtype(annotation);
    if (type != FPDF_ANNOT_INK) {
      continue;
    }
    // ���û��ƿ���
    auto* annot_obj = FPDFAnnot_GetObject(annotation, i);
    float pen_width = 0.0f;
    suc = FPDFPageObj_GetStrokeWidth(annot_obj, &pen_width);
    suc = FPDFPageObj_SetStrokeWidth(annot_obj, pen_width * x_scale);
    std::cout << "FPDFPageObj_SetStrokeWidth pen_width:" << pen_width
              << ",suc:" << suc << std::endl;
    float x_h;
    float x_v;
    float p_w;
    suc = FPDFAnnot_GetBorder(annotation, &x_h, &x_v, &p_w);
    suc = FPDFAnnot_SetBorder(annotation, 2.0f, 2.0f, 1.0f);
    unsigned int R;
    unsigned int G;
    unsigned int B;
    unsigned int A;
    suc = FPDFAnnot_GetColor(annotation, FPDFANNOT_COLORTYPE_Color, &R, &G, &B,
                             &A);
    suc = FPDFAnnot_SetColor(annotation, FPDFANNOT_COLORTYPE_Color, R, G, B, A,
                             true);
    // test
    unsigned long normal_length_bytes = FPDFAnnot_GetAP(
        annotation, FPDF_ANNOT_APPEARANCEMODE_NORMAL, nullptr, 0);
    StringBase sb;
    std::vector<FPDF_WCHAR> buf =
        std::vector<FPDF_WCHAR>(normal_length_bytes / sizeof(FPDF_WCHAR));

    suc =
        (FPDF_BOOL)FPDFAnnot_GetAP(annotation, FPDF_ANNOT_APPEARANCEMODE_NORMAL,
                                   buf.data(), normal_length_bytes);
    auto result = sb.GetPlatformString(buf.data());
    std::cout << "sb.GetPlatformString result:" << result << std::endl;
    //
    unsigned long size = FPDFAnnot_GetInkListCount(annotation);
    for (int j = 0; j < (int)size; j++) {
      // ��ȡ���е�λ��
      unsigned long path_size =
          FPDFAnnot_GetInkListPath(annotation, j, nullptr, 0);
      // std::vector<FS_POINTF> path_buffer(path_size);
      ///* auto new_path_size = */FPDFAnnot_GetInkListPath(annotation, j,
      //                                    path_buffer.data(), path_size);

      for (int k = 0; k < (int)path_size; k++) {
        /*FS_POINTF new_point = path_buffer.at(k);
        new_point.x *=  x_scale;
        new_point.y *= y_scale;
        path_buffer[k] = new_point;
        std::cout << "path_buffer x:" << new_point.x << ",y:" << new_point.y
                  << std::endl;*/
        auto resut = FPDFAnnot_SetInkListPath(annotation, k, x_scale, y_scale);
        std::cout << "FPDFAnnot_SetInkListPath result:" << resut << std::endl;
      }
    }
    FPDFPage_CloseAnnot(annotation);
  }
  suc = FPDFPage_GenerateContent(page);
  std::cout << "ScanPagePathObj suc:" << suc << std::endl;
}
// ��������ҳPath���� Ƭ��
void PdfCore::ScalePageSegmentsObj(FPDF_PAGE page,
                                   double x_scale,
                                   double y_scale) {
  if (!is_authentication_) {
    return;
  }
  FPDF_BOOL suc = false;
  int obj_count = FPDFPage_CountObjects(page);
  for (int i = 0; i < obj_count; i++) {
    auto* obj = FPDFPage_GetObject(page, i);
    auto type = FPDFPageObj_GetType(obj);
    if (type == FPDF_PAGEOBJ_PATH) {
      auto count = FPDFPath_CountSegments(obj);
      for (int j = 0; j < count; j++) {
        auto* path = FPDFPath_GetPathSegment(obj, j);
        float x = 0.0f;
        float y = 0.0f;
        suc = FPDFPathSegment_GetPoint(path, &x, &y);
        // suc = FPDFPath_MoveTo(obj, x * x_scale, y * y_scale);
        FS_MATRIX matrix;
        suc = FPDFPageObj_GetMatrix(obj, &matrix);
        matrix.a *= x_scale;
        matrix.d *= y_scale;
        matrix.e *= x_scale;
        matrix.f *= y_scale;
        FPDFPath_SetPathSegment(obj, j, x * x_scale, y * y_scale, matrix);
        std::cout << "FPDFPath_SetPathSegment x:" << x * x_scale
                  << ",y:" << y * y_scale << ",suc" << suc << std::endl;
        suc = FPDFPathSegment_GetClose(path);
      }
    }
  }
  suc = FPDFPage_GenerateContent(page);
  std::cout << "ScalePageSegmentsObj suc:" << suc << std::endl;
}
// ��������annotation���� �����»��ߣ��л��ߣ�����
void PdfCore::ScalePageAnnotationObj(FPDF_PAGE page,
                                     double x_scale,
                                     double y_scale) {
  if (!is_authentication_) {
    return;
  }
  FPDF_BOOL suc = false;
  auto count = FPDFPage_GetAnnotCount(page);
  for (int i = 0; i < count; i++) {
    auto* annotation = FPDFPage_GetAnnot(page, i);
    auto type = FPDFAnnot_GetSubtype(annotation);
    if (type == FPDF_ANNOT_INK) {
      continue;
    }
    auto annotation_count = FPDFAnnot_GetObjectCount(annotation);
    for (int j = 0; j < annotation_count; j++) {
      auto size = FPDFAnnot_CountAttachmentPoints(annotation);
      for (int k = 0; k < (int)size; k++) {
        FS_QUADPOINTSF quadpoints;
        suc = FPDFAnnot_GetAttachmentPoints(annotation, k, &quadpoints);
        quadpoints.x1 *= x_scale;
        quadpoints.x2 *= x_scale;
        quadpoints.x3 *= x_scale;
        quadpoints.x4 *= x_scale;
        quadpoints.y1 *= y_scale;
        quadpoints.y2 *= y_scale;
        quadpoints.y3 *= y_scale;
        quadpoints.y4 *= y_scale;
        suc = FPDFAnnot_SetAttachmentPoints(annotation, k, &quadpoints);
      }
    }
  }
  suc = FPDFPage_GenerateContent(page);
  std::cout << "ScaleAnnotationPageObj suc:" << suc << std::endl;
}
// ����
FPDF_BOOL PdfCore::test(FPDF_PAGE page) {
  FPDF_BOOL suc = false;
  FS_POINTF kFirstInkStroke[] = {{80.0f, 50},      {91.0f, 91.0f},
                                 {102.0f, 120.0f}, {153.0f, 153.0f},
                                 {184.0f, 200.0f}, {285.0f, 295.0f}};
  size_t kFirstStrokePointCount = std::size(kFirstInkStroke);

  /* static constexpr FS_POINTF kSecondInkStroke[] = {
       {70.0f, 90.0f}, {71.0f, 91.0f}, {72.0f, 92.0f}};
   static constexpr size_t kSecondStrokePointCount =
   std::size(kSecondInkStroke);*/
  auto* ink_annot = FPDFPage_CreateAnnot(page, FPDF_ANNOT_REDACT);
  FS_RECTF rect1;
  rect1.left = 200.f;
  rect1.bottom = 400.f;
  rect1.right = 500.f;
  rect1.top = 600.f;
  suc = FPDFAnnot_SetRect(ink_annot, &rect1);
  suc = FPDFAnnot_SetColor(ink_annot, FPDFANNOT_COLORTYPE_Color, 255, 192, 0,
                           255);
  static const wchar_t kData[] = L"\xf6\xe4";
  StringBase sb;
  auto text = sb.GetPlatformWString((FPDF_WIDESTRING)kData);
  suc = FPDFAnnot_SetStringValue(ink_annot, "test888888888",
                                 (FPDF_WIDESTRING)text.data());
  suc = FPDFAnnot_AddInkStroke(ink_annot, kFirstInkStroke,
                               kFirstStrokePointCount);
  FPDF_PAGEOBJECT check1 = FPDFPageObj_CreateNewPath(200, 500);
  suc = FPDFPath_LineTo(check1, 100, 200);
  suc = FPDFPath_LineTo(check1, 300, 400);
  suc = FPDFPath_MoveTo(check1, 350, 550);
  suc = FPDFPageObj_SetStrokeColor(check1, 0, 255, 255, 180);
  suc = FPDFPageObj_SetStrokeWidth(check1, 8.35f);
  suc = FPDFPath_SetDrawMode(check1, 0, 1);
  suc = FPDFAnnot_AppendObject(ink_annot, check1);

  std::cout << "FPDFAnnot_AddInkStroke result:" << suc << std::endl;

  auto* annot = FPDFPage_CreateAnnot(page, FPDF_ANNOT_STAMP);
  FS_RECTF rect;

  rect.left = 200.f;
  rect.bottom = 400.f;
  rect.right = 500.f;
  rect.top = 600.f;
  suc = FPDFAnnot_SetRect(annot, &rect);

  // Add a new path to the annotation.
  FPDF_PAGEOBJECT check = FPDFPageObj_CreateNewPath(200, 500);
  suc = FPDFPath_LineTo(check, 300, 400);
  suc = FPDFPath_LineTo(check, 500, 600);
  suc = FPDFPath_MoveTo(check, 350, 550);
  suc = FPDFPath_LineTo(check, 450, 450);
  suc = FPDFPageObj_SetStrokeColor(check, 0, 255, 255, 180);
  suc = FPDFPageObj_SetStrokeWidth(check, 8.35f);
  suc = FPDFPath_SetDrawMode(check, 0, 1);
  // suc = FPDFAnnot_AppendObject(annot, check);
  auto result = FPDFAnnot_GetObjectCount(annot);
  std::cout << "FPDFAnnot_AddInkStroke result:" << result << std::endl;

  suc = FPDFPage_GenerateContent(page);
  return suc;
}
// ���ӻ�������
FPDF_BOOL PdfCore::InsertPathToPage(FPDF_PAGE page,
                                    unsigned int R,
                                    unsigned int G,
                                    unsigned int B,
                                    unsigned int A,
                                    float pen_width,
                                    FS_QUADPOINTSF* points,
                                    int length) {
  // FPDF_BOOL suc = false;
  // auto ink_annot = FPDFPage_CreateAnnot(page, FPDF_ANNOT_INK);
  // FS_RECTF rect;
  // rect.left = 200.f;
  // rect.bottom = 400.f;
  // rect.right = 500.f;
  // rect.top = 600.f;
  // suc = FPDFAnnot_SetRect(ink_annot, &rect);
  // suc = FPDFAnnot_SetColor(ink_annot, FPDFANNOT_COLORTYPE_Color, R, G, B,
  //                          A);
  //
  ////suc = FPDFAnnot_AddInkStroke(ink_annot, points, length);
  // suc = FPDFAnnot_AppendAttachmentPoints(ink_annot, points);
  // FPDF_PAGEOBJECT path_obj = FPDFPageObj_CreateNewPath(10, 300);
  // suc = FPDFPageObj_SetStrokeColor(path_obj, R, G, B, A);
  // suc = FPDFPageObj_SetStrokeWidth(path_obj, pen_width);
  // suc = FPDFPath_SetDrawMode(path_obj, FPDF_FILLMODE_NONE, true);
  // suc = FPDFAnnot_AppendObject(ink_annot, path_obj);
  // suc = GenerateContent(page);
  // suc = FPDFPath_Close(path_obj);
  // FPDFPage_CloseAnnot(ink_annot);
  //// ����pdf
  // FS_MATRIX matrix;
  // FPDFPageObj_GetMatrix(path_obj, &matrix);
  // suc = FPDFPage_TransFormWithClip(page, &matrix, nullptr);
  // return suc;

  // test for page
  FPDF_BOOL suc = false;

  // Create a new ink annotation.
  auto* ink_annot = FPDFPage_CreateAnnot(page, FPDF_ANNOT_INK);
  FS_RECTF rect;
  rect.left = 0.0f;
  rect.bottom = 0.0f;
  rect.right = FPDF_GetPageWidthF(page);
  rect.top = FPDF_GetPageHeightF(page);
  suc = FPDFAnnot_SetRect(ink_annot, &rect);
  suc = FPDFAnnot_SetColor(ink_annot, FPDFANNOT_COLORTYPE_Color, R, G, B, A);
  suc = FPDFAnnot_SetBorder(ink_annot, 1.0f, 1.0f, pen_width);
  /* static constexpr FS_POINTF kFirstInkStroke[] = {
       {10.0f, 30.0f}, {81.0f, 91.0f}, {82.0f, 92.0f},
       {83.0f, 93.0f}, {84.0f, 94.0f}, {85.0f, 95.0f}};
   static constexpr size_t kFirstStrokePointCount = std::size(kFirstInkStroke);

   static constexpr FS_POINTF kSecondInkStroke[] = {
       {170.0f, 90.0f}, {71.0f, 91.0f}, {272.0f, 92.0f}};
   static constexpr size_t kSecondStrokePointCount =
   std::size(kSecondInkStroke);

   static constexpr FS_POINTF kThirdInkStroke[] = {{160.0f, 90.0f},
                                                   {61.0f, 91.0f},
                                                   {32.0f, 92.0f},
                                                   {263.0f, 93.0f},
                                                   {64.0f, 94.0f}};
   static constexpr size_t kThirdStrokePointCount =
   std::size(kThirdInkStroke);*/

  // Negative test: |annot| is passed as nullptr.
  /*suc = FPDFAnnot_AddInkStroke(ink_annot, kFirstInkStroke,
                                       kFirstStrokePointCount);*/

  // suc = FPDFAnnot_AddInkStroke(ink_annot, path_buffer_test_->data(),
  //                              path_buffer_test_->size());

  // Negative test: |annot| is not ink annotation.
  // Create a new highlight annotation.
  // auto highlight_annot =
  //    FPDFPage_CreateAnnot(page, FPDF_ANNOT_HIGHLIGHT);

  // suc = FPDFAnnot_AddInkStroke(highlight_annot, kFirstInkStroke,
  //                                      kFirstStrokePointCount);

  //// Negative test: passing |point_count| as  0.
  // suc = FPDFAnnot_AddInkStroke(ink_annot, kFirstInkStroke, 0);

  //// Negative test: passing |points| array as nullptr.
  // suc = FPDFAnnot_AddInkStroke(ink_annot, nullptr,
  //                                      kFirstStrokePointCount);

  //// Negative test: passing |point_count| more than ULONG_MAX/2.
  // suc = FPDFAnnot_AddInkStroke(ink_annot, kSecondInkStroke,
  //                                      ULONG_MAX / 2 + 1);

  //// InkStroke should get added to ink annotation. Also inklist should get
  //// created.
  // suc = FPDFAnnot_AddInkStroke(ink_annot, kFirstInkStroke,
  //                                     kFirstStrokePointCount);

  //// Adding another inkStroke to ink annotation with all valid paremeters.
  //// InkList already exists in ink_annot.
  // suc = FPDFAnnot_AddInkStroke(ink_annot, kSecondInkStroke,
  //                                     kSecondStrokePointCount);

  //// Adding one more InkStroke to the ink annotation. |point_count| passed is
  //// less than the data available in |buffer|.
  // suc = FPDFAnnot_AddInkStroke(ink_annot, kThirdInkStroke,
  //                                     kThirdStrokePointCount - 1);

  FPDF_PAGEOBJECT path_obj = FPDFPageObj_CreateNewPath(600, 300);
  suc = FPDFPageObj_SetStrokeWidth(path_obj, pen_width);
  suc = FPDFPath_BezierTo(path_obj, 20, 60, 80, 20, 190, 65);
  suc = FPDFPath_BezierTo(path_obj, 120, 30, 10, 60, 60, 150);
  suc = FPDFPageObj_SetStrokeColor(path_obj, R, G, B, A);
  suc = FPDFPath_SetDrawMode(path_obj, FPDF_FILLMODE_NONE, true);
  // ����·������ҳ��
  // FPDFPage_InsertObject(page, path_obj);
  suc = FPDFAnnot_AppendObject(ink_annot, path_obj);
  suc = GenerateContent(page);
  return suc;
}
// ��ȡҳ
FPDF_PAGE PdfCore::GetPdfPage(FPDF_DOCUMENT doc, int index) {
  if (!is_authentication_) {
    return nullptr;
  }
  return FPDF_LoadPage(doc, index);
}
// ��ȡpageҳ�ı�
std::string PdfCore::GetPageContent(FPDF_PAGE page, FPDF_PAGEOBJECT obj) {
  if (!is_authentication_) {
    return nullptr;
  }
  int type = FPDFPageObj_GetType(obj);
  std::string result;
  if (type == FPDF_PAGEOBJ_TEXT) {
    auto* text_page = FPDFText_LoadPage(page);
    unsigned long size = FPDFTextObj_GetText(obj, text_page, nullptr, 0);
    std::vector<unsigned short> buffer(size);

    FPDFTextObj_GetText(obj, text_page, buffer.data(), size);
    StringBase sb;
    // result = sb.GetPlatformWString(buffer.data());
    /*_bstr_t t = result.c_str();
    char* pchar = (char*)t;
    std::string result1 = pchar;*/
    // ʹ�� printf ��ӡ���ֽ��ַ���
    // printf("%s\n", result1.c_str());
    // printf("pdf text:%s\n", result);
    result = sb.GetPlatformString(buffer.data());
    std::cout << result << std::endl;
  }
  return result;
}
// С��ģʽ��RGBA��R�洢�ڵ�λ��A�洢�ڸ�λ
void PdfCore::Bgra2RgbaSmall(unsigned char* buffer, int width, int height) {
  if (!is_authentication_) {
    return;
  }
  ColorConvert color_convert;
  color_convert.Bgra2RgbaSmall(buffer, width, height);
}
void PdfCore::Rgba2BgraSmall(unsigned char* buffer, int width, int height) {
  if (!is_authentication_) {
    return;
  }
  ColorConvert color_convert;
  color_convert.Rgba2BgraSmall(buffer, width, height);
}
// ���ģʽ��RGBA��R�洢�ڸ�λ��A�洢�ڵ�λ
void PdfCore::Bgra2RgbaBig(unsigned char* buffer, int width, int height) {
  if (!is_authentication_) {
    return;
  }
  ColorConvert color_convert;
  color_convert.Bgra2RbgaBig(buffer, width, height);
}
void PdfCore::Rgba2BgraBig(unsigned char* buffer, int width, int height) {
  if (!is_authentication_) {
    return;
  }
  ColorConvert color_convert;
  color_convert.Rgba2BgraBig(buffer, width, height);
}
/**
 * 设置JavaVM 安卓平台下调用接口设置
 * @vitural_machine vitrual machine JavaVM变量
 **/
void PdfCore::SetJavaVirtualMachine(void* virtual_machine) {
#if (CPU_TYPE < 2)
  SetJavaVM(virtual_machine);
#endif
}
/**
 * 安卓平台下设置上下文
 * @context 上下文变量
 **/
void PdfCore::SetJavaContext(void* context) {
#if (CPU_TYPE < 2)
  SetContext(context);
#endif
}
/**
 *鉴权 (优先级:在线->离线->豁免期)
 * @offline_certificate_info 脱机证书鉴权相关信息结构体
 * @enc_file_path 授权文件路径(可读可写),例如/xxx/xxx/enc.txt 或者
 *为授权文件内容(isString 为true时)
 * @suc_type 返回校验成功的类型 1:服务端返回的授权码 2:离线授权码 3:豁免期
 *4:脱机离线码 -1:失败
 * @modules_authority 返回模块权限,逗号隔开 例如(HPV,HBE)
 * @is_string： false则证书从授权文件中读取， true则证书直接从接口中传入
 * @return 0表示成功，返回详细信息使用GetAuthErrorInfo接口获取
 **/
int PdfCore::SetAuthenticate(OfflineCertificateInfo* offline_certificate_info,
                             const char* enc_file_path,
                             int& suc_type,
                             char modules_authority[500],
                             bool is_string) {
  int suc = -1;
#if (CPU_TYPE < 2)
  int suc = Authenticate(offline_certificate_info, enc_file_path, suc_type,
                         modules_authority, is_string);
  if (suc == 0) {
    is_authentication_ = true;
  }
#endif
  return suc;
}
/**
 * 获取每个层级的第一个大纲对象
 * @doc document指针对象
 * @bookmark 大纲指针对象
 *第一次获取大纲对象bookmark为nullptr，后面为获取到的大纲指针对象
 * @return 大纲结构体对象
 **/
Outlines* PdfCore::GetPdfFirstOutline(FPDF_DOCUMENT doc,
                                      FPDF_BOOKMARK bookmark) {
  if (!is_authentication_) {
    return nullptr;
  }
  if (!doc) {
    return nullptr;
  }
  return FPDFBookmark_GetFirstOutline(doc, bookmark);
}
/**
 * 获取同一层级的兄弟大纲
 * @doc document指针对象
 * @bookmark 大纲指针对象
 * @return 大纲结构体对象
 **/
Outlines* PdfCore::GetPdfNextSiblingOutline(FPDF_DOCUMENT doc,
                                            FPDF_BOOKMARK bookmark) {
  if (!is_authentication_) {
    return nullptr;
  }
  if (!doc) {
    return nullptr;
  }
  return FPDFBookmark_GetNextSiblingOutline(doc, bookmark);
}
/**
 * 修改大纲名称
 * @doc document文档指针对象
 * @src_outline 原大纲名称
 * @target_outline 目标大纲名称
 * @return 1修改成功 0失败
 **/
FPDF_BOOL PdfCore::UpdatePdfOutlineItem(FPDF_DOCUMENT doc,
                                        FPDF_WIDESTRING src_outline,
                                        FPDF_WIDESTRING target_outline) {
  FPDF_BOOL suc = false;
  if (!is_authentication_) {
    return suc;
  }
  if (doc == nullptr) {
    return suc;
  }
  StringBase sb;
  ScopedFPDFWideString wide_outline_str =
      sb.GetFPDFWideString((wchar_t*)src_outline);
  FPDF_BOOKMARK outline = FPDFBookmark_Find(doc, wide_outline_str.get());
  if (outline == nullptr) {
    return suc;
  }
  suc = FPDFBookmark_SetTitle(outline, target_outline);
  return suc;
}
/**
 * 删除大纲
 * @doc document文档指针对象
 * @del_outline 待删除大纲名称
 * @return 1修改成功 0失败
 **/
FPDF_BOOL PdfCore::DeletePdfOutlineItem(FPDF_DOCUMENT doc,
                                        FPDF_WIDESTRING del_outline) {
  FPDF_BOOL suc = false;
  if (!is_authentication_) {
    return suc;
  }
  if (doc == nullptr) {
    return suc;
  }
  StringBase sb;
  ScopedFPDFWideString outline_wide_str =
      sb.GetFPDFWideString((wchar_t*)del_outline);
  FPDF_BOOKMARK outline = FPDFBookmark_Find(doc, outline_wide_str.get());
  suc = FPDFBookmark_DelTitle(doc, outline);
  return suc;
}
/**
 * 添加大纲
 * @doc document文档指针对象
 * @current_outline
 *在当前选中所在层级的大纲名称，如果未选中则默认添加到最上层大纲
 * @add_outline 待添加大纲的名称
 * @page_index pdf页索引
 * @x 大纲所在的x坐标
 * @y 大纲所在的y坐标
 * @zoom 是否缩放 默认为1
 * @return 1添加成功 0失败
 **/
FPDF_BOOL PdfCore::AddPdfOutlineItem(FPDF_DOCUMENT doc,
                                     FPDF_WIDESTRING current_outline,
                                     FPDF_WIDESTRING add_outline,
                                     int page_index,
                                     double x,
                                     double y,
                                     double zoom) {

  FPDF_BOOL suc = false;
  if (!is_authentication_) {
    return suc;
  }
  if (!doc) {
    return suc;
  }
  StringBase sb;
  ScopedFPDFWideString outline_wide_str =
      sb.GetFPDFWideString((wchar_t*)current_outline);
  FPDF_BOOKMARK current_bookmark =
      FPDFBookmark_Find(doc, outline_wide_str.get());
  // 如果没有选择当前大纲，则默认选择最上层大纲
  if (!current_bookmark) {
    current_bookmark = FPDFBookmark_GetFirstChild(doc, NULL);
  }
  suc = FPDFBookmark_AddTitle(doc, current_bookmark, add_outline, page_index, x,
                              y, zoom);
  return suc;
}
/**
 * 设置ENT数据
 * @doc document文档指针对象
 * @ent_data ENT数据
 * @return true成功 false失败
 **/
FPDF_BOOL PdfCore::SetEntData(FPDF_DOCUMENT doc, const char* ent_data) {
  FPDF_BOOL suc = false;
  if (!is_authentication_) {
    return suc;
  }
  if (doc == nullptr) {
    return suc;
  }
  suc = FPDF_SetEnt(doc, (char*)ent_data);
  return suc;
}
/**
 * 获取ENT数据
 * @doc document文档指针对象
 * @return ENT数据
 **/
char* PdfCore::GetEntData(FPDF_DOCUMENT doc) {
  return FPDF_GetEnt(doc);
}
/**
 * 添加高亮
 * @page page指针对象
 * @annotation_rect 高亮矩形大小
 * @annotation_content 注解描述信息
 * @r,g,b,a 填充的颜色
 * @return 1成功 0失败
 **/
FPDF_BOOL PdfCore::AddHighlight(FPDF_PAGE page,
                                FS_RECTF annotation_rect,
                                FPDF_WIDESTRING annotation_content,
                                int r,
                                int g,
                                int b,
                                int a) {
  if (!is_authentication_) {
    return false;
  }
  return AddAnnotation(page, annotation_rect, annotation_content,
                       FPDF_ANNOT_HIGHLIGHT, r, g, b, a);
}
/**
 * 添加下划线
 * @page page指针对象
 * @annotation_rect 下划线矩形大小
 * @annotation_content 注解描述信息
 * @r,g,b,a 填充的颜色
 * @return 1成功 0失败
 **/
FPDF_BOOL PdfCore::AddUnderline(FPDF_PAGE page,
                                FS_RECTF annotation_rect,
                                FPDF_WIDESTRING annotation_content,
                                int r,
                                int g,
                                int b,
                                int a) {
  if (!is_authentication_) {
    return false;
  }
  return AddAnnotation(page, annotation_rect, annotation_content,
                       FPDF_ANNOT_UNDERLINE, r, g, b, a);
}
/**
 * 添加删除线
 * @page page指针对象
 * @annotation_rect 删除线矩形大小
 * @annotation_content 注解描述信息
 * @r,g,b,a 填充的颜色
 * @return 1成功 0失败
 **/
FPDF_BOOL PdfCore::AddStrikeout(FPDF_PAGE page,
                                FS_RECTF annotation_rect,
                                FPDF_WIDESTRING annotation_content,
                                int r,
                                int g,
                                int b,
                                int a) {
  if (!is_authentication_) {
    return false;
  }
  return AddAnnotation(page, annotation_rect, annotation_content,
                       FPDF_ANNOT_STRIKEOUT, r, g, b, a);
}
/**
 * 创建注释
 * @page page指针对象
 * @annotation_rect 注解矩形大小
 * @annotation_content 注解描述信息
 * @type 注解的类型
 * @r,g,b,a 填充的颜色
 * @return 1成功 0失败
 **/
FPDF_BOOL PdfCore::AddAnnotation(FPDF_PAGE page,
                                 FS_RECTF annotation_rect,
                                 FPDF_WIDESTRING annotation_content,
                                 FPDF_ANNOTATION_SUBTYPE type,
                                 int r,
                                 int g,
                                 int b,
                                 int a) {
  FPDF_BOOL suc = false;
  if (!is_authentication_) {
    return suc;
  }
  if (!page) {
    return suc;
  }
  FPDF_ANNOTATION annotation = FPDFPage_CreateAnnot(page, type);
  if (!annotation) {
    return suc;
  }
  suc = FPDFAnnot_SetColor(annotation, FPDFANNOT_COLORTYPE_Color, r, g, b, a);
  /*FS_QUADPOINTSF quadpoints = {
      115.80264 ,718.9139232 ,157.211172 ,718.9139232 ,115.80264 ,706.264416
  ,157.211172 ,706.264416,
  };
  FS_RECTF rect{115.75062 ,706.328568 ,157.001868 ,719.2715904};*/
  // 假设矩形的顺序是左下(A)->左上(B)->右上(C)->右下(D)
  // 那么传入point的顺序是2条平行线 A->D,B->C
  FS_QUADPOINTSF annotation_points = {
      annotation_rect.left,  annotation_rect.bottom,  //(A)
      annotation_rect.right, annotation_rect.bottom,  //(D)
      annotation_rect.left,  annotation_rect.top,     //(B)
      annotation_rect.right, annotation_rect.top,     //(C)

  };
  suc = FPDFAnnot_SetRect(annotation, &annotation_rect);
  suc = FPDFAnnot_AppendAttachmentPoints(annotation, &annotation_points);
  suc = FPDFAnnot_SetStringValue(annotation, pdfium::annotation::kContents,
                                 annotation_content);
  // suc = FPDFAnnot_SetAttachmentPoints(annotation, 0, &quadpoints);
  // int result = FPDFAnnot_AddInkStroke(annotation, points, points_size);
  FPDFPage_CloseAnnot(annotation);
  return suc;
}
#ifdef __cplusplus
}
#endif
}  // namespace sunia