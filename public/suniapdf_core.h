/********************************************
 * 功能描述：1、获取PDF文件信息（页面数量，页面原始宽高）
 * 2、新建page页
 * 3、在PDF页面插入图片
 * 4、合并多个pdf 文件page页到新的pdf文件
 * 5、生成新PDF文件
 * 6、PDF文件自定义内容读写
 * 7、缩放pdf页（下划线、中划线，高亮，注释，手写，path）
 * 创建日期：20240129
 * 修改日期：
 *********************************************/
#ifndef PUBLIC_SUNIAPDF_CORE_H_
#define PUBLIC_SUNIAPDF_CORE_H_
// update on 20240418
/**
 * cpu type
 * 0 arm64-v8a
 * 1 armeabi-v7a
 * 2 x64
 * 3 x86
 **/
#define CPU_TYPE 2
//
#include <stddef.h>
#include <stdio.h>
#include <iostream>
#include "fpdf_edit.h"
#include "fpdf_ppo.h"
#include "fpdf_save.h"
#include "fpdf_text.h"
#include "fpdfview.h"
// update on 20240410
#include "IAuthOpt.h"
#include "IAuthenticate.h"
// test
// #include "/home/rm/Desktop/project/cpp/pdfcore_test/src/test.h"
namespace sunia {
// Exported Class
#ifdef __cplusplus
extern "C" {
#endif
class FPDF_EXPORT PdfCore {
 public:
  /**
   *@brief 构造函数
   **/
  PdfCore();
  /**
   *@brief 析构函数
   **/
  ~PdfCore();
  /**
   * @brief 设置JavaVM 安卓平台下调用接口设置
   * @param[in] vitural_machine virtual machine JavaVM变量
   **/
  void SetJavaVirtualMachine(void* virtual_machine);
  /**
   * @brief 安卓平台下设置上下文
   * @param[in] context 上下文变量
   **/
  void SetJavaContext(void* context);
  /**
   * @brief 鉴权 (优先级:在线->离线->豁免期)
   * @param[in] offline_certificate_info 脱机证书鉴权相关信息结构体
   * @param[in] enc_file_path 授权文件路径(可读可写),例如/xxx/xxx/enc.txt
   *或者为授权文件内容(isString 为true时)
   * @param[in] suc_type 返回校验成功的类型 1:服务端返回的授权码 2:离线授权码
   *3:豁免期
   * @brief 4:脱机离线码 -1:失败
   * @param[out] modules_authority 返回模块权限,逗号隔开 例如(HPV,HBE)
   * @param[in] is_string： false则证书从授权文件中读取，
   *true则证书直接从接口中传入
   * @return 0表示成功，返回详细信息使用GetAuthErrorInfo接口获取
   **/
  int SetAuthenticate(OfflineCertificateInfo* offline_certificate_info,
                      const char* enc_file_path,
                      int& suc_type,
                      char modules_authority[500],
                      bool is_string = false);
  /**
   * @brief 导入pdf文件
   * @param[in] pdf_path 文件路径
   * @param[in] password 密码
   * @return document指针对象
   **/
  FPDF_DOCUMENT LoadDocument(FPDF_STRING pdf_path,
                             FPDF_BYTESTRING password = NULL);
  /**
   * @brief 获取pdf总页数
   * @param[in] doc document指针对象
   * @return 总页数
   **/
  FPDF_RESULT GetPageCount(FPDF_DOCUMENT doc);
  /**
   * @brief 获取页面宽高
   * @param[in] doc document指针对象
   * @param[in] page_index 页索引
   * @param[out] size 返回页面大小
   * @return 1：成功 0：失败
   **/
  FPDF_BOOL GetPageSizeByIndex(FPDF_DOCUMENT doc,
                               int page_index,
                               FS_SIZEF& size);
  /**
   * @brief 获取指定页面渲染成位图
   * @param[in] doc document指针对象
   * @param[in] page_index 页的索引
   * @param[in] image_width 图片宽度
   * @param[in] image_height 图片高度
   * @param[in] first_scan 图片指针首地址
   * @param[in] stride 一行的宽度
   * @return 图片流
   **/
  void* RenderPageToBitmap(FPDF_DOCUMENT doc,
                           int page_index,
                           int image_width,
                           int image_height,
                           void* first_scan,
                           int stride);
  /**
   * @brief 获取指定页面渲染成位图
   * @param[in] doc document指针对象
   * @param[in] page_index 页的索引
   * @param[in] image_width 图片宽度
   * @param[in] image_height 图片高度
   * @param[in] first_scan 图片指针首地址
   * @param[in] stride 一行的宽度
   * @return Bitmap对象
   **/
  FPDF_BITMAP RenderPageToBitmapN(FPDF_DOCUMENT doc,
                                  int page_index,
                                  int image_width,
                                  int image_height,
                                  void* first_scan,
                                  int stride);
  /**
   * @brief 保存位图
   * @param[in] doc document指针对象
   * @param[in] filename 文件名
   * @param[in] buffer 图片流
   * @param[in] width 图片宽度
   * @param[in] height 图片高度
   **/
  void SaveBitmapToFile(FPDF_DOCUMENT doc,
                        const char* filename,
                        const unsigned char* buffer,
                        int width,
                        int height);
  /**
   * @brief 插入位图
   * @param[in] doc document指针对象
   * @param[in] page_index 页索引
   * @param[in] image_windth 图片宽度
   * @param[in] image_height 图片高度
   * @param[in] bitmap_data 图片流
   * @param[in] visible_image
   *是否显示图片,true(显示),false(在AIE中隐藏笔迹图片,在其他pdf阅读器中显示笔迹图片)
   * @return 1:成功 0：失败
   **/
  FPDF_BOOL InsertBitmapToPage(FPDF_DOCUMENT doc,
                               int page_index,
                               FS_POINTF position,
                               float image_width,
                               float image_heigth,
                               const unsigned char* bitmap_data,
                               FPDF_BOOL visible_image = true);
  /**
   * @brief 新建pdf页
   * @param[in] doc document指针对象
   * @param[in] page_index 页索引
   * @param[in] width 页宽
   * @param[in] height 页高
   * @return page指针对象
   **/
  FPDF_PAGE NewPdfPage(FPDF_DOCUMENT doc,
                       int page_index,
                       int width,
                       int height);
  /**
   * @brief 更新pdf缓存 在调用保存接口和刷新缓存之前调用
   * @param[in] page page指针对象
   * @return 1：成功 0失败
   **/
  FPDF_BOOL GenerateContent(FPDF_PAGE page);
  /**
   * @brief 另存为/新建pdf
   * @param[in] doc document指针对象
   * @param[in] file_path 文件名
   * @return 1成功 0失败
   **/
  FPDF_BOOL SavePdf(FPDF_DOCUMENT doc, const char* file_path);
  /**
   * @brief 新建pdf
   * @return document指针对象
   **/
  FPDF_DOCUMENT CreatePdf();
  /**
   * @brief 将多个pdf文件中的页组成一个新的pdf文件
   * @param[in] obj_doc 目标document指针对象
   * @param[in] src_doc 源document指针对象
   * @param[in] indexs 页索引数组
   * @param[in] size 数组长度
   * @return document指针对象
   **/
  FPDF_DOCUMENT CombinePdf(FPDF_DOCUMENT obj_doc,
                           FPDF_DOCUMENT src_doc,
                           int* indexs,
                           int size);
  /**
   * @brief 将多个pdf文件中的页组成一个新的pdf文件
   * @param[in] obj_doc 目标document指针对象
   * @param[in] src_doc 源document指针对象
   * @param[in] indexs 页索引字符串，用-分割（1-3）
   * @param[in] start 目标document插入位置索引
   * @return document指针对象
   **/
  FPDF_DOCUMENT CombinePdf(FPDF_DOCUMENT obj_doc,
                           FPDF_DOCUMENT src_doc,
                           const char* indexs,
                           int start);
  /**
   * @brief 交换pdf页的位置
   * @param[in] doc document指针对象
   * @param[in] page_indices 页索引数组 比如[A, B, C, D]页, 对应下标为[0, 1, 2,
   *3]，Move(doc, [3, 2], 2, 1)，最后的页顺序为 [A, D, C, B]
   * @param[in] page_indices_len 数组长度
   * @param[in] dest_page_index 交换的位置
   * @return 1：成功 0：失败
   **/
  FPDF_BOOL SwapPdfPage(FPDF_DOCUMENT doc,
                        const int* page_indices,
                        unsigned long page_indices_len,
                        int dest_page_index);
  /**
   * @brief 关闭文档
   * @param[in] doc document指针对象
   **/
  void CloseDocument(FPDF_DOCUMENT doc);
  /**
   * @brief
   *改变pdf页大小，增加留白边距，最后的页宽应该是width+horizontal_margin，页高是height+vertical_margin
   * @param[in] src_page page指针对象
   * @param[in] width 页宽
   * @param[in] height 页高
   * @param[in] horizontal_margin 水平边距
   * @param[in] vertical_margin 垂直边距
   * @return 1：成功 0：失败
   **/
  FPDF_BOOL ResizePdfPage(FPDF_PAGE src_page,
                          double width,
                          double height,
                          double horizontal_margin,
                          double vertical_margin);
  /**
   * @brief 获取指定页对象
   * @param[in] doc document文档指针对象
   * @param[in] index 页索引
   * @return page指针对象
   **/
  FPDF_PAGE GetPdfPage(FPDF_DOCUMENT doc, int index);
  /**
   * @brief 获取page页文本内容
   * @param[in] page page指针对象
   * @param[in] obj 页对象
   * @return 字符内容
   **/
  std::string GetPageContent(FPDF_PAGE page, FPDF_PAGEOBJECT obj);
  /**
   * @brief 获取每个层级的第一个大纲对象
   * @param[in] doc document指针对象
   * @param[in] bookmark 大纲指针对象
   *第一次获取大纲对象bookmark为nullptr，后面为获取到的大纲指针对象
   * @return 大纲结构体对象
   **/
  Outlines* GetPdfFirstOutline(FPDF_DOCUMENT doc, FPDF_BOOKMARK bookmark);
  /**
   * @brief 获取同一层级的兄弟大纲
   * @param[in] doc document指针对象
   * @param[in] bookmark 大纲指针对象
   * @return 大纲结构体对象
   **/
  Outlines* GetPdfNextSiblingOutline(FPDF_DOCUMENT doc, FPDF_BOOKMARK bookmark);
  /**
   * @brief 修改大纲名称
   * @param[in] doc document文档指针对象
   * @param[in] src_outline 原大纲名称
   * @param[in] target_outline 目标大纲名称
   * @return 1修改成功 0失败
   **/
  FPDF_BOOL UpdatePdfOutlineItem(FPDF_DOCUMENT doc,
                                 FPDF_WIDESTRING src_outline,
                                 FPDF_WIDESTRING target_outline);
  /**
   * @brief 删除大纲
   * @param[in] doc document文档指针对象
   * @param[in] del_outline 待删除大纲名称
   * @return 1修改成功 0失败
   **/
  FPDF_BOOL DeletePdfOutlineItem(FPDF_DOCUMENT doc,
                                 FPDF_WIDESTRING del_outline);
  /**
   * @brief 添加大纲
   * @param[in] doc document文档指针对象
   * @param[in] current_outline
   *在当前选中所在层级的大纲名称，如果未选中则默认添加到最上层大纲
   * @param[in] add_outline 待添加大纲的名称
   * @param[in] page_index pdf页索引
   * @param[in] x 大纲所在的x坐标
   * @param[in] y 大纲所在的y坐标
   * @param[in] zoom 是否缩放 默认为1
   * @return 1添加成功 0失败
   **/
  FPDF_BOOL AddPdfOutlineItem(FPDF_DOCUMENT doc,
                              FPDF_WIDESTRING current_outline,
                              FPDF_WIDESTRING add_outline,
                              int page_index,
                              double x,
                              double y,
                              double zoom);
  /**
   * @brief 设置ENT数据
   * @param[in] doc document文档指针对象
   * @param[in] ent_data ENT数据
   * @return true成功 false失败
   **/
  FPDF_BOOL SetEntData(FPDF_DOCUMENT doc, const char* ent_data);
  /**
   * @brief 获取ENT数据
   * @param[in] doc document文档指针对象
   * @return ENT数据
   **/
  char* GetEntData(FPDF_DOCUMENT doc);
  /**
   * @brief 添加高亮
   * @param[in] page page指针对象
   * @param[in] annotation_rect 高亮矩形大小
   * @param[in] annotation_content 注解描述信息
   * @param[in] r,g,b,a 填充的颜色
   * @return 1成功 0失败
   **/
  FPDF_BOOL AddHighlight(FPDF_PAGE page,
                         FS_RECTF annotation_rect,
                         FPDF_WIDESTRING annotation_content,
                         int r,
                         int g,
                         int b,
                         int a);
  /**
   * @brief 添加下划线
   * @param[in] page page指针对象
   * @param[in] annotation_rect 下划线矩形大小
   * @param[in] annotation_content 注解描述信息
   * @param[in] r,g,b,a 填充的颜色
   * @return 1成功 0失败
   **/
  FPDF_BOOL AddUnderline(FPDF_PAGE page,
                         FS_RECTF annotation_rect,
                         FPDF_WIDESTRING annotation_content,
                         int r,
                         int g,
                         int b,
                         int a);
  /**
   * @brief 添加删除线
   * @param[in] page page指针对象
   * @param[in] annotation_rect 删除线矩形大小
   * @param[in] annotation_content 注解描述信息
   * @param[in] r,g,b,a 填充的颜色
   * @return 1成功 0失败
   **/
  FPDF_BOOL AddStrikeout(FPDF_PAGE page,
                         FS_RECTF annotation_rect,
                         FPDF_WIDESTRING annotation_content,
                         int r,
                         int g,
                         int b,
                         int a);
  /**
   * @brief 将bgra转rgba 0xFF000000 ->red
   *小端模式，RGBA中R存储在低位，A存储在高位
   * @param[in] buffer 图片流
   * @param[in] width 图片宽
   * @param[in] height 图片高
   **/
  void Bgra2RgbaSmall(unsigned char* buffer, int width, int height);
  /**
   * @brief 将rgba转bgra 0xFF000000 ->red
   *小端模式，RGBA中R存储在低位，A存储在高位
   * @param[in] buffer 图片流
   * @param[in] width 图片宽
   * @param[in] height 图片高
   **/
  void Rgba2BgraSmall(unsigned char* buffer, int width, int height);
  /**
   * @brief 将bgra转rgba 0x000000FF ->red
   *大端模式，RGBA中R存储在高位，A存储在低位
   * @param[in] buffer 图片流
   * @param[in] width 图片宽
   * @param[in] height 图片高
   **/
  void Bgra2RgbaBig(unsigned char* buffer, int width, int height);
  /**
   * @brief 将rgba转bgra 0x000000FF ->red
   *大端模式，RGBA中R存储在高位，A存储在低位
   * @param[in] buffer 图片流
   * @param[in] width 图片宽
   * @param[in] height 图片高
   **/
  void Rgba2BgraBig(unsigned char* buffer, int width, int height);
  /** @brief 测试代码 实验性API 用作下一次迭代 **/
  FPDF_BOOL test(FPDF_PAGE page);
  /** @brief 添加绘制内容 实验性API **/
  FPDF_BOOL InsertPathToPage(FPDF_PAGE page,
                             unsigned int R,
                             unsigned int G,
                             unsigned int B,
                             unsigned int A,
                             float pen_width,
                             FS_QUADPOINTSF* points,
                             int length);

 private:
  /**
   * @brief 处理要缩放的对象包括注释、下划线、中划线以及手绘内容 （不可访问）
   * @param[in] page page指针对象
   * @param[in] x_scale x方向缩放值
   * @param[in] y_scale y方向缩放值
   * @return 1：成功 0：失败
   **/
  FPDF_BOOL ResizePdfPageObject(FPDF_PAGE page, double x_scale, double y_scale);
  /**
   * @brief 处理手绘对象 （不可访问）
   * @param[in] page page指针对象
   * @param[in] x_scale x方向缩放值
   * @param[in] y_scale y方向缩放值
   * @return 1：成功 0：失败
   **/
  void ScanPagePathObj(FPDF_PAGE page, double x_scale, double y_scale);
  /**
   * @brief 处理片段对象 （不可访问）
   * @param[in] page page指针对象
   * @param[in] x_scale x方向缩放值
   * @param[in] y_scale y方向缩放值
   * @return 1：成功 0：失败
   **/
  void ScalePageSegmentsObj(FPDF_PAGE page, double x_scale, double y_scale);
  /**
   * @brief 处理中划线和下划线 （不可访问）
   * @param[in] page page指针对象
   * @param[in] x_scale x方向缩放值
   * @param[in] y_scale y方向缩放值
   * @return 1：成功 0：失败
   **/
  void ScalePageAnnotationObj(FPDF_PAGE page, double x_scale, double y_scale);
  /**
   * @brief 创建注释 （不可访问）
   * @param[in] page page指针对象
   * @param[in] annotation_rect 注解矩形大小
   * @param[in] annotation_content 注解描述信息
   * @param[in] type 注解的类型
   * @param[in] r,g,b,a 填充的颜色
   * @return 1成功 0失败
   **/
  FPDF_BOOL AddAnnotation(FPDF_PAGE page,
                          FS_RECTF annotation_rect,
                          FPDF_WIDESTRING annotation_content,
                          FPDF_ANNOTATION_SUBTYPE type,
                          int r,
                          int g,
                          int b,
                          int a);
  /**
   *@brief 是否鉴权成功
   **/
  bool is_authentication_ = false;
};
#ifdef __cplusplus
}
#endif
}  // namespace sunia
#endif