#include "public/suniapdf_core.h"
#include "public//fpdf_transformpage.h"
#include "public/fpdf_annot.h"
#include "fpdfsdk/suniafile_base.h"
#include "fpdfsdk/suniastring_base.h"
#include "fpdfsdk/suniacolor_convert.h"
#include <vector>
namespace sunia {
#ifdef __cplusplus
extern "C" {
#endif
PdfCore::PdfCore() {

  FPDF_InitLibrary();
}
PdfCore::~PdfCore() {

  FPDF_DestroyLibrary();
}
void PdfCore::CloseDocument(FPDF_DOCUMENT doc) {

  FPDF_CloseDocument(doc);
}

FPDF_DOCUMENT PdfCore::LoadDocument(FPDF_STRING pdf_path,
  FPDF_BYTESTRING password)
{
  if (pdf_path == nullptr) {
    return nullptr;
  }
  FPDF_DOCUMENT doc = FPDF_LoadDocument(pdf_path, password);
  return doc;
}

FPDF_RESULT PdfCore::GetPageCount(FPDF_DOCUMENT doc) {
  FPDF_RESULT page_count = 0;
  if (doc != nullptr) {
    page_count = FPDF_GetPageCount(doc);
  }
  return page_count;
}

FPDF_BOOL PdfCore::GetPageSizeByIndex(FPDF_DOCUMENT doc,int page_index,
                                      FS_SIZEF& size) {
  FPDF_BOOL suc = false;
  if (doc != nullptr) {
    suc = FPDF_GetPageSizeByIndexF(doc, page_index, &size);
  }
  return suc;
}

void PdfCore::SaveBitmapToFile(FPDF_DOCUMENT doc,const char* filename,
                               const unsigned char* buffer,
                               int width,
                               int height) {
  FILE* file = fopen(filename, "wb");
  if (file) {
    fwrite(buffer, 1, width * height * 4, file);  // Assuming 32-bit RGBA format
    fclose(file);
  } else {
    fprintf(stderr, "Failed to open file for writing: %s\n", filename);
  }
}

void* PdfCore::RenderPageToBitmap(FPDF_DOCUMENT doc,
                                  int page_index,
                                  int image_width,
                                  int image_height,
                                  void* first_scan,
                                  int stride) {
  void* bitmap_bufeers = nullptr;
  if (doc != nullptr) {
    FPDF_BITMAP bitmap;
    auto* page = GetPdfPage(doc, page_index);
    if (page != nullptr) {
      //bitmap = FPDFBitmap_Create(pageWidth, pageHeight, 0);

      bitmap = FPDFBitmap_CreateEx(image_width, image_height, FPDFBitmap_BGRA,
                                   first_scan, stride);
     /* FPDFBitmap_FillRect(bitmap, 0, 0, pageWidth,
                          pageHeight, 0xFFFFFFFF);*/
  
      FPDF_RenderPageBitmap(bitmap, page, 0, 0, image_width, image_height, 0,
                            /*FPDF_ANNOT*/0);

      bitmap_bufeers = FPDFBitmap_GetBuffer(bitmap);
      FPDF_ClosePage(page);
    }
  }
  return bitmap_bufeers;
}

FPDF_BITMAP PdfCore::RenderPageToBitmapN(FPDF_DOCUMENT doc, int page_index,int image_width,
                                         int image_height,void* first_scan,int stride) {

  FPDF_BITMAP bitmap = nullptr;
  if (doc != nullptr) {

    auto* page = GetPdfPage(doc, page_index);
    if (page != nullptr) {
    
      bitmap = FPDFBitmap_CreateEx(image_width, image_height, FPDFBitmap_BGRA,
                                   first_scan, stride);
    
      FPDF_RenderPageBitmap(bitmap, page, 0, 0, image_width, image_height, 0,
                            FPDF_ANNOT);
      FPDF_ClosePage(page);
    }
  }
  return bitmap;
}

FPDF_BOOL PdfCore::InsertBitmapToPage(FPDF_DOCUMENT doc,int page_index,
                                      FS_POINTF position,float image_width,float image_heigth,
                                      const unsigned char* bitmap_data)
{
  FPDF_BOOL suc = false;

  FPDF_PAGEOBJECT page_obj = FPDFPageObj_NewImageObj(doc);
  auto* page = GetPdfPage(doc, page_index);

  FPDF_BITMAP pdfBitmap = FPDFBitmap_CreateEx(image_width, image_heigth, FPDFBitmap_BGRA,
                          (void*)bitmap_data, image_width * 4);

  suc = FPDFImageObj_SetBitmap(&page, 1, page_obj, pdfBitmap);
  FS_RECTF rc(0,0,0,0);
  suc = FPDFPage_GetCropBox(page, &rc.left,&rc.bottom,&rc.right,&rc.top);
  position.x += rc.left;
  position.y -= rc.bottom;
  float page_height = FPDF_GetPageHeightF(page);
  FS_MATRIX matrix{0};
  matrix.a = image_width;
  matrix.d = image_heigth;
  matrix.e = position.x;
  matrix.f = page_height - image_heigth - position.y;

  suc = FPDFImageObj_SetMatrix(page_obj, matrix.a, matrix.b, matrix.c, matrix.d,
                               matrix.e, matrix.f);
  
  FPDFPage_InsertObject(page, page_obj);

  suc = GenerateContent(page);
  return suc;
}

FPDF_PAGE PdfCore::NewPdfPage(FPDF_DOCUMENT doc,
                              int page_index,
                              int width,
                              int height) {
  FPDF_BOOL suc = false;
  FPDF_PAGE page = nullptr;
  if (doc != nullptr) {
    if (page_index < 0) {
      page_index = 0;
    }
    auto page_count = FPDF_GetPageCount(doc);
    if (page_index > page_count) {
      page_index = page_count;
    }
    page = FPDFPage_New(doc, page_index, width, height);
    suc = GenerateContent(page);
    printf("FPDFPage_New result:%d\n", suc);
  }
  return page;
}

FPDF_BOOL PdfCore::GenerateContent(FPDF_PAGE page) {
  FPDF_BOOL suc = false;
  if (page != nullptr)
  {
    suc = FPDFPage_GenerateContent(page);
  }
  return suc;
}

FPDF_BOOL PdfCore::SavePdf(FPDF_DOCUMENT doc,const char* file_path) {
  FPDF_BOOL suc = false;
  PdfWrite write(file_path);
  suc = FPDF_SaveAsCopy(doc, &write, /* FPDF_INCREMENTAL*/0);

  // FPDF_MovePages
  return suc;
}

FPDF_BOOL PdfCore::SwapPdfPage(FPDF_DOCUMENT doc,const int* page_indices,
                               unsigned long page_indices_len,
                               int dest_page_index)
{
  FPDF_BOOL suc = false;
  suc = FPDF_MovePages(doc, page_indices, page_indices_len,
                       dest_page_index);
  return suc;
}

FPDF_DOCUMENT PdfCore::CreatePdf() {
  return FPDF_CreateNewDocument();
}

FPDF_DOCUMENT PdfCore::CombinePdf(FPDF_DOCUMENT obj_doc,
                                  FPDF_DOCUMENT src_doc,
                                  int* indexs,
                                  int size)
{
  FPDF_BOOL suc = false;
  suc = FPDF_ImportPagesByIndex(obj_doc, src_doc, indexs, size, 0);
  printf("FPDF_ImportPagesByIndex result:%d", suc);
  return obj_doc;
}

FPDF_DOCUMENT PdfCore::CombinePdf(FPDF_DOCUMENT obj_doc,
                                  FPDF_DOCUMENT src_doc,
                                  const char* indexs,
                                  int start)
{
  FPDF_BOOL suc = false;
  suc = FPDF_ImportPages(obj_doc, src_doc, indexs, start);
  printf("FPDF_ImportPages result:%d\n", suc);
  return obj_doc;
}

FPDF_BOOL PdfCore::ResizePdfPage(FPDF_PAGE src_page,
                                 double width,
                                 double height)
{
  FPDF_BOOL suc = false;
  if (src_page == nullptr) {
    return suc;
  }
  double old_width = FPDF_GetPageWidth(src_page);
  double old_height = FPDF_GetPageHeight(src_page);
  FS_MATRIX matrix{0};
  matrix.a = width / old_width;
  matrix.d = height / old_height;
  FS_RECTF page_bounding_rect = {0, 0, 0, 0};
  suc = FPDF_GetPageBoundingBox(src_page, &page_bounding_rect);
  
  FPDFPage_TransformAnnots(src_page, matrix.a, matrix.b, matrix.c, matrix.d,
                           matrix.e, matrix.f);

  suc = FPDFPage_TransFormWithClip(src_page, &matrix, nullptr);
  suc = FPDFPage_GenerateContent(src_page);
  //
  FPDFPage_SetCropBox(
      src_page, page_bounding_rect.left, page_bounding_rect.bottom,
      width + page_bounding_rect.left, height + page_bounding_rect.bottom);
  if (matrix.a > 1.0f || matrix.d > 1.0f)
  {
  FPDFPage_SetMediaBox(
      src_page, page_bounding_rect.left, page_bounding_rect.bottom,
      width + page_bounding_rect.left, height + page_bounding_rect.bottom);
  }
  suc = FPDFPage_GenerateContent(src_page);
  return suc;
}

FPDF_BOOL PdfCore::ResizePdfPageObject(FPDF_PAGE page,
                                     double x_scale,
                                     double y_scale) {
  FPDF_BOOL suc = false;

  int annotation_count = FPDFPage_GetAnnotCount(page);
  for (int i = 0; i < annotation_count; i++) {
  
    FPDF_ANNOTATION annotation = FPDFPage_GetAnnot(page, i);
    auto type = FPDFAnnot_GetSubtype(annotation);
    if (type == FPDF_ANNOT_INK)
    {
      /*suc = FPDFAnnot_SetColor(annotation, FPDFANNOT_COLORTYPE_Color, 0,
                         255,0,255,true);*/
      std::cout << "FPDFAnnot_GetSubtype type:" << FPDF_ANNOT_INK << ",suc:" << suc
                << std::endl;
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

void PdfCore::ScanPagePathObj(FPDF_PAGE page,double x_scale,double y_scale)
{
  auto count = FPDFPage_GetAnnotCount(page);
  FPDF_BOOL suc = false;
  for (int i = 0; i < count; i++) {
    FPDF_ANNOTATION annotation = FPDFPage_GetAnnot(page, i);
    auto type = FPDFAnnot_GetSubtype(annotation);
    if (type != FPDF_ANNOT_INK) {
      continue;
    }
    unsigned long annotation_size = FPDFAnnot_GetInkListCount(annotation);
    for (int j = 0; j < (int)annotation_size; j++) {
    
       unsigned long path_size =
           FPDFAnnot_GetInkListPath(annotation, j, nullptr, 0);
       //std::vector<FS_POINTF> path_buffer(path_size);
      ///* auto new_path_size = */FPDFAnnot_GetInkListPath(annotation, j,
      //                                    path_buffer.data(), path_size);

       for (int k = 0; k < (int)path_size; k++)
       {
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

void PdfCore::ScalePageSegmentsObj(FPDF_PAGE page,
                              double x_scale,
                              double y_scale) {
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
         //suc = FPDFPath_MoveTo(obj, x * x_scale, y * y_scale);
         FS_MATRIX matrix;
         suc = FPDFPageObj_GetMatrix(obj, &matrix);
         matrix.a *= x_scale;
         matrix.d *= y_scale;
         matrix.e *= x_scale;
         matrix.f *= y_scale;
         FPDFPath_SetPathSegment(obj, j,x*x_scale,y*y_scale, matrix);
         std::cout << "FPDFPath_SetPathSegment x:" << x * x_scale
                   << ",y:" << y * y_scale << ",suc" << suc << std::endl;
         suc = FPDFPathSegment_GetClose(path);
       }
    }
  }
  suc = FPDFPage_GenerateContent(page);
  std::cout << "ScalePageSegmentsObj suc:" << suc << std::endl;
}

void PdfCore::ScalePageAnnotationObj(FPDF_PAGE page,
                                  double x_scale,
                                  double y_scale) {
  FPDF_BOOL suc = false;
  auto count = FPDFPage_GetAnnotCount(page);
  for (int i = 0; i < count; i++)
  {
    auto* annotation = FPDFPage_GetAnnot(page, i);
    auto type = FPDFAnnot_GetSubtype(annotation);
    if (type == FPDF_ANNOT_INK) {
       continue;
    }
    auto annotation_count = FPDFAnnot_GetObjectCount(annotation);
    for (int j = 0; j < annotation_count; j++)
    {
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

FPDF_BOOL PdfCore::test(FPDF_PAGE page) {
  FPDF_BOOL suc = false;
   FS_POINTF kFirstInkStroke[] = {
      {80.0f, 50}, {91.0f, 91.0f}, {102.0f, 120.0f},
      {153.0f, 153.0f}, {184.0f, 200.0f}, {285.0f, 295.0f}};
   size_t kFirstStrokePointCount = std::size(kFirstInkStroke);
   
 /* static constexpr FS_POINTF kSecondInkStroke[] = {
      {70.0f, 90.0f}, {71.0f, 91.0f}, {72.0f, 92.0f}};
  static constexpr size_t kSecondStrokePointCount = std::size(kSecondInkStroke);*/
   auto* ink_annot = FPDFPage_CreateAnnot(page, FPDF_ANNOT_REDACT);
   FS_RECTF rect1;
   rect1.left = 200.f;
   rect1.bottom = 400.f;
   rect1.right = 500.f;
   rect1.top = 600.f;
   suc = FPDFAnnot_SetRect(ink_annot, &rect1);
  suc = FPDFAnnot_SetColor(ink_annot, FPDFANNOT_COLORTYPE_Color, 255, 192, 0, 255);
   static const wchar_t kData[] = L"\xf6\xe4";
  StringBase sb;
  auto text = sb.GetPlatformWString((FPDF_WIDESTRING)kData);
   suc = FPDFAnnot_SetStringValue(ink_annot, "test888888888", (FPDF_WIDESTRING)text.data());
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
  suc= FPDFAnnot_SetRect(annot, &rect);

  // Add a new path to the annotation.
  FPDF_PAGEOBJECT check = FPDFPageObj_CreateNewPath(200, 500);
  suc = FPDFPath_LineTo(check, 300, 400);
  suc = FPDFPath_LineTo(check, 500, 600);
  suc = FPDFPath_MoveTo(check, 350, 550);
  suc = FPDFPath_LineTo(check, 450, 450);
  suc = FPDFPageObj_SetStrokeColor(check, 0, 255, 255, 180);
  suc = FPDFPageObj_SetStrokeWidth(check, 8.35f);
  suc = FPDFPath_SetDrawMode(check, 0, 1);
  //suc = FPDFAnnot_AppendObject(annot, check);
  auto result = FPDFAnnot_GetObjectCount(annot);
  std::cout << "FPDFAnnot_AddInkStroke result:" << result << std::endl;

  suc = FPDFPage_GenerateContent(page);
  return suc;
}

FPDF_BOOL PdfCore::InsertPathToPage(FPDF_PAGE page,
                           unsigned int R,
                           unsigned int G,
                           unsigned int B,
                           unsigned int A,
                           float pen_width,
                           FS_QUADPOINTSF* points,
                           int length) {
  //FPDF_BOOL suc = false;
  //auto ink_annot = FPDFPage_CreateAnnot(page, FPDF_ANNOT_INK);
  //FS_RECTF rect;
  //rect.left = 200.f;
  //rect.bottom = 400.f;
  //rect.right = 500.f;
  //rect.top = 600.f;
  //suc = FPDFAnnot_SetRect(ink_annot, &rect);
  //suc = FPDFAnnot_SetColor(ink_annot, FPDFANNOT_COLORTYPE_Color, R, G, B,
  //                         A);
  //
  ////suc = FPDFAnnot_AddInkStroke(ink_annot, points, length);
  //suc = FPDFAnnot_AppendAttachmentPoints(ink_annot, points);
  //FPDF_PAGEOBJECT path_obj = FPDFPageObj_CreateNewPath(10, 300);
  //suc = FPDFPageObj_SetStrokeColor(path_obj, R, G, B, A);
  //suc = FPDFPageObj_SetStrokeWidth(path_obj, pen_width);
  //suc = FPDFPath_SetDrawMode(path_obj, FPDF_FILLMODE_NONE, true);
  //suc = FPDFAnnot_AppendObject(ink_annot, path_obj);
  //suc = GenerateContent(page);
  //suc = FPDFPath_Close(path_obj);
  //FPDFPage_CloseAnnot(ink_annot);

  //FS_MATRIX matrix;
  //FPDFPageObj_GetMatrix(path_obj, &matrix);
  //suc = FPDFPage_TransFormWithClip(page, &matrix, nullptr);
  //return suc;

  //test for page
  FPDF_BOOL suc = false;

  // Create a new ink annotation.
  auto* ink_annot=
      FPDFPage_CreateAnnot(page, FPDF_ANNOT_INK);
  FS_RECTF rect;
  rect.left = 0.0f;
  rect.bottom = 0.0f;
  rect.right = FPDF_GetPageWidthF(page);
  rect.top = FPDF_GetPageHeightF(page);
  suc = FPDFAnnot_SetRect(ink_annot, &rect);
  suc = FPDFAnnot_SetColor(ink_annot, FPDFANNOT_COLORTYPE_Color, R, G, B, A);
  suc = FPDFAnnot_SetBorder(ink_annot,1.0f, 1.0f, pen_width);
 /* static constexpr FS_POINTF kFirstInkStroke[] = {
      {10.0f, 30.0f}, {81.0f, 91.0f}, {82.0f, 92.0f},
      {83.0f, 93.0f}, {84.0f, 94.0f}, {85.0f, 95.0f}};
  static constexpr size_t kFirstStrokePointCount = std::size(kFirstInkStroke);

  static constexpr FS_POINTF kSecondInkStroke[] = {
      {170.0f, 90.0f}, {71.0f, 91.0f}, {272.0f, 92.0f}};
  static constexpr size_t kSecondStrokePointCount = std::size(kSecondInkStroke);

  static constexpr FS_POINTF kThirdInkStroke[] = {{160.0f, 90.0f},
                                                  {61.0f, 91.0f},
                                                  {32.0f, 92.0f},
                                                  {263.0f, 93.0f},
                                                  {64.0f, 94.0f}};
  static constexpr size_t kThirdStrokePointCount = std::size(kThirdInkStroke);*/

  // Negative test: |annot| is passed as nullptr.
  /*suc = FPDFAnnot_AddInkStroke(ink_annot, kFirstInkStroke,
                                       kFirstStrokePointCount);*/


  suc = FPDFAnnot_AddInkStroke(ink_annot, path_buffer_test_->data(),
                               path_buffer_test_->size());

  // Negative test: |annot| is not ink annotation.
  // Create a new highlight annotation.
  //auto highlight_annot =
  //    FPDFPage_CreateAnnot(page, FPDF_ANNOT_HIGHLIGHT);

  //suc = FPDFAnnot_AddInkStroke(highlight_annot, kFirstInkStroke,
  //                                     kFirstStrokePointCount);

  //// Negative test: passing |point_count| as  0.
  //suc = FPDFAnnot_AddInkStroke(ink_annot, kFirstInkStroke, 0);

  //// Negative test: passing |points| array as nullptr.
  //suc = FPDFAnnot_AddInkStroke(ink_annot, nullptr,
  //                                     kFirstStrokePointCount);

  //// Negative test: passing |point_count| more than ULONG_MAX/2.
  //suc = FPDFAnnot_AddInkStroke(ink_annot, kSecondInkStroke,
  //                                     ULONG_MAX / 2 + 1);

  //// InkStroke should get added to ink annotation. Also inklist should get
  //// created.
  //suc = FPDFAnnot_AddInkStroke(ink_annot, kFirstInkStroke,
  //                                    kFirstStrokePointCount);


  //// Adding another inkStroke to ink annotation with all valid paremeters.
  //// InkList already exists in ink_annot.
  //suc = FPDFAnnot_AddInkStroke(ink_annot, kSecondInkStroke,
  //                                    kSecondStrokePointCount);

  //// Adding one more InkStroke to the ink annotation. |point_count| passed is
  //// less than the data available in |buffer|.
  //suc = FPDFAnnot_AddInkStroke(ink_annot, kThirdInkStroke,
  //                                    kThirdStrokePointCount - 1);

  FPDF_PAGEOBJECT path_obj = FPDFPageObj_CreateNewPath(600, 300);
  suc = FPDFPageObj_SetStrokeWidth(path_obj, pen_width);
  suc = FPDFPath_BezierTo(path_obj, 20, 60, 80, 20, 190, 65);
  suc = FPDFPath_BezierTo(path_obj, 120, 30, 10, 60, 60, 150);
  suc = FPDFPageObj_SetStrokeColor(path_obj, R, G, B, A);
  suc = FPDFPath_SetDrawMode(path_obj, FPDF_FILLMODE_NONE, true);

  //FPDFPage_InsertObject(page, path_obj);
  suc = FPDFAnnot_AppendObject(ink_annot, path_obj);
  suc = GenerateContent(page);
  return suc;
}

FPDF_PAGE PdfCore::GetPdfPage(FPDF_DOCUMENT doc, int index) {
  return FPDF_LoadPage(doc, index);
}

std::string PdfCore::GetPageContent(FPDF_PAGE page, FPDF_PAGEOBJECT obj) {
  int type = FPDFPageObj_GetType(obj);
  std::string result;
  if (type == FPDF_PAGEOBJ_TEXT) {
    auto* text_page = FPDFText_LoadPage(page);
    unsigned long size = FPDFTextObj_GetText(obj, text_page, nullptr, 0);
    std::vector<unsigned short> buffer(size);

    FPDFTextObj_GetText(obj, text_page, buffer.data(), size);
    StringBase sb;
    result = sb.GetPlatformString(buffer.data());
    std::cout << result << std::endl;
  }
  return result;
}

void PdfCore::Bgra2RgbaSmall(unsigned char* buffer, int width, int height) {
  ColorConvert color_convert;
  color_convert.Bgra2RgbaSmall(buffer, width, height);
}
void PdfCore::Rgba2BgraSmall(unsigned char* buffer, int width, int height) {
  ColorConvert color_convert;
  color_convert.Rgba2BgraSmall(buffer, width, height);
}

void PdfCore::Bgra2RgbaBig(unsigned char* buffer, int width, int height) {
  ColorConvert color_convert;
  color_convert.Bgra2RbgaBig(buffer, width, height);
}
void PdfCore::Rgba2BgraBig(unsigned char* buffer, int width, int height) {
  ColorConvert color_convert;
  color_convert.Rgba2BgraBig(buffer, width, height);
}
#ifdef __cplusplus
}
#endif
}  // namespace sunia