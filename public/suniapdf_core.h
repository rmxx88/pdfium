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
//update on 20240418
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
#include "fpdfview.h"
#include "fpdf_save.h"
#include "fpdf_edit.h"
#include "fpdf_text.h"
#include "fpdf_ppo.h"
//update on 20240410
#include "IAuthOpt.h"
#include "IAuthenticate.h"
//test
//#include "/home/rm/Desktop/project/cpp/pdfcore_test/src/test.h"
namespace sunia
{
// Exported Class
#ifdef __cplusplus
extern "C" {
#endif
class FPDF_EXPORT PdfCore{
 public:
  /**
  *构造函数
  **/
  PdfCore();
  /**
  * 析构函数
  **/
  ~PdfCore();
  /**
  * 设置JavaVM 安卓平台下调用接口设置
  * @vitural_machine virtual machine JavaVM变量
  **/
  void SetJavaVirtualMachine(void* virtual_machine);
  /**
  * 安卓平台下设置上下文
  * @context 上下文变量
  **/
  void SetJavaContext(void* context);
  /**
  *鉴权 (优先级:在线->离线->豁免期)
  * @offline_certificate_info 脱机证书鉴权相关信息结构体
  * @enc_file_path 授权文件路径(可读可写),例如/xxx/xxx/enc.txt 或者 为授权文件内容(isString 为true时)
  * @suc_type 返回校验成功的类型 1:服务端返回的授权码 2:离线授权码 3:豁免期 4:脱机离线码 -1:失败
  * @modules_authority 返回模块权限,逗号隔开 例如(HPV,HBE)
  * @is_string： false则证书从授权文件中读取， true则证书直接从接口中传入
  * @return 0表示成功，返回详细信息使用GetAuthErrorInfo接口获取
  **/
  int SetAuthenticate(OfflineCertificateInfo* offline_certificate_info,const char* enc_file_path,
                     int& suc_type,char modules_authority[500],bool is_string = false);
  /**
  * 导入pdf文件
  * @pdf_path 文件路径
  * @password 密码
  * @return document指针对象
  **/  
  FPDF_DOCUMENT LoadDocument(FPDF_STRING pdf_path,
                             FPDF_BYTESTRING password = NULL);
  /**
  * 获取pdf总页数
  * @doc document指针对象
  * @return 总页数
  **/
  FPDF_RESULT GetPageCount(FPDF_DOCUMENT doc);
  /**
  * 获取页面宽高
  * @doc document指针对象
  * @page_index 页索引
  * @size 返回页面大小
  * @return 1：成功 0：失败
  **/
  FPDF_BOOL GetPageSizeByIndex(FPDF_DOCUMENT doc,int page_index, FS_SIZEF& size);
  /**
  * 获取指定页面渲染成位图
  * @doc document指针对象
  * @page_index 页的索引
  * @image_width 图片宽度
  * @image_height 图片高度
  * @first_scan 图片指针首地址
  * @stride 一行的宽度
  * @return 图片流
  **/
  void* RenderPageToBitmap(FPDF_DOCUMENT doc,
                           int page_index,
                           int image_width,
                           int image_height,
                           void* first_scan,
                           int stride);
  /**
  *获取指定页面渲染成位图
  * @doc document指针对象
  * @page_index 页的索引
  * @image_width 图片宽度
  * @image_height 图片高度
  * @first_scan 图片指针首地址
  * @stride 一行的宽度
  * @return Bitmap对象
  **/
  FPDF_BITMAP RenderPageToBitmapN(FPDF_DOCUMENT doc,
                                  int page_index,
                                  int image_width,
                                  int image_height,
                                  void* first_scan,
                                  int stride);
  /**
  *保存位图
  * @doc document指针对象
  * @filename 文件名
  * @buffer 图片流
  * @width 图片宽度
  * @height 图片高度
  **/
  void SaveBitmapToFile(FPDF_DOCUMENT doc,const char* filename,
                        const unsigned char* buffer,
                        int width,
                        int height);
   /**
   * 插入位图
   * @doc document指针对象
   * @page_index 页索引
   * @image_windth 图片宽度
   * @image_height 图片高度
   * @bitmap_data 图片流
   * @visible_image 是否显示图片,true(显示),false(在AIE中隐藏笔迹图片,在其他pdf阅读器中显示笔迹图片)
   * @return 1:成功 0：失败
   **/
  FPDF_BOOL InsertBitmapToPage(FPDF_DOCUMENT doc,int page_index,
                               FS_POINTF position,
                               float image_width,
                               float image_heigth,
                               const unsigned char* bitmap_data,
                               FPDF_BOOL visible_image = true);
  /**
  * 新建pdf页
  * @doc document指针对象
  * @page_index 页索引
  * @width 页宽
  * @height 页高
  * @return page指针对象
  **/
  FPDF_PAGE NewPdfPage(FPDF_DOCUMENT doc,
                           int page_index,
                           int width,
                           int height);
  /**
  * 更新pdf缓存 在调用保存接口和刷新缓存之前调用
  * @page page指针对象
  * @return 1：成功 0失败
  **/
  FPDF_BOOL GenerateContent(FPDF_PAGE page);
  /**
  * 另存为/新建pdf
  * @doc document指针对象
  * @file_path 文件名
  **/
  FPDF_BOOL SavePdf(FPDF_DOCUMENT doc,const char* file_path);
  /**
  * 新建pdf
  * @return document指针对象
  **/
  FPDF_DOCUMENT CreatePdf();
  /**
  * 将多个pdf文件中的页组成一个新的pdf文件
  * @obj_doc 目标document指针对象
  * @src_doc 源document指针对象
  * @indexs 页索引数组
  * @size 数组长度
  * @return document指针对象
  **/
  FPDF_DOCUMENT CombinePdf(FPDF_DOCUMENT obj_doc,
                       FPDF_DOCUMENT src_doc,
                       int* indexs,
                       int size);
  /**
  * 将多个pdf文件中的页组成一个新的pdf文件
  * @obj_doc 目标document指针对象
  * @src_doc 源document指针对象
  * @indexs 页索引字符串，用-分割（1-3）
  * @start 目标document插入位置索引
  * @return document指针对象
  **/ 
  FPDF_DOCUMENT CombinePdf(FPDF_DOCUMENT obj_doc,
                           FPDF_DOCUMENT src_doc,
                           const char* indexs,
                           int start);
  /**
  * 交换pdf页的位置
  * @doc document指针对象
  * @page_indices 页索引数组
  * @page_indices_len 数组长度
  * @dest_page_index 交换的位置
  * @return 1：成功 0：失败
  **/
  FPDF_BOOL SwapPdfPage(FPDF_DOCUMENT doc,const int* page_indices,
                        unsigned long page_indices_len,
                        int dest_page_index);
  /**
  * 关闭文档
  * @doc document指针对象
  **/
  void CloseDocument(FPDF_DOCUMENT doc);
  /**
  * 改变pdf页大小
  * @src_page page指针对象
  * @width 页宽
  * @height 页高
  * @horizontal_margin 水平边距
  * @vertical_margin 垂直边距
  * @return 1：成功 0：失败
  **/
  FPDF_BOOL ResizePdfPage(FPDF_PAGE src_page,
                          double width,
                          double height,
                          double horizontal_margin,
                          double vertical_margin);
  /**
  * 获取指定页对象
  * @doc document指针对象
  * @index 页索引
  * @return page指针对象
  **/
  FPDF_PAGE GetPdfPage(FPDF_DOCUMENT doc, int index);
  /**
  * 获取page页文本内容
  * @page page指针对象
  * @obj 页对象
  * @return 字符内容
  **/
  std::string GetPageContent(FPDF_PAGE page, FPDF_PAGEOBJECT obj);
  /**
  * 设置ENT数据
  * @doc document文档指针对象
  * @ent_data ENT数据
  * @return true成功 false失败
  **/
  FPDF_BOOL SetEntData(FPDF_DOCUMENT doc,const char* ent_data);
  /**
  * 获取ENT数据
  * @doc document文档指针对象
  * @return ENT数据
  **/
  char* GetEntData(FPDF_DOCUMENT doc);
  /**
  * 将bgra转rgba 0xFF000000 ->red
  * 小端模式，RGBA中R存储在低位，A存储在高位
  * @buffer 图片流
  * @width 图片宽
  * @height 图片高
  **/
  void Bgra2RgbaSmall(unsigned char* buffer, int width, int height);
  /**
  * 将rgba转bgra 0xFF000000 ->red
  * 小端模式，RGBA中R存储在低位，A存储在高位
  * @buffer 图片流
  * @width 图片宽
  * @height 图片高
  **/
  void Rgba2BgraSmall(unsigned char* buffer, int width, int height);
  /**
  * 将bgra转rgba 0x000000FF ->red
  * 大端模式，RGBA中R存储在高位，A存储在低位
  * @buffer 图片流
  * @width 图片宽
  * @height 图片高
  **/
  void Bgra2RgbaBig(unsigned char* buffer, int width, int height);
  /**
  * 将rgba转bgra 0x000000FF ->red
  * 大端模式，RGBA中R存储在高位，A存储在低位
  * @buffer 图片流
  * @width 图片宽
  * @height 图片高
  **/
  void Rgba2BgraBig(unsigned char* buffer, int width, int height);
  //测试代码 用作下一次迭代
  FPDF_BOOL test(FPDF_PAGE page);
  //添加绘制内容
  FPDF_BOOL InsertPathToPage(FPDF_PAGE page,unsigned int R,unsigned int G,
                             unsigned int B,unsigned int A,float pen_width,
                             FS_QUADPOINTSF* points,
                             int length);
 private:
  /**
  * 处理要缩放的对象包括注释、下划线、中划线以及手绘内容
  * @page page指针对象
  * @x_scale x方向缩放值
  * @y_scale y方向缩放值
  * @return 1：成功 0：失败
  **/
  FPDF_BOOL ResizePdfPageObject(FPDF_PAGE page, double x_scale, double y_scale);
  /**
  * 处理手绘对象
  * @page page指针对象
  * @x_scale x方向缩放值
  * @y_scale y方向缩放值
  * @return 1：成功 0：失败
  **/
  void ScanPagePathObj(FPDF_PAGE page, double x_scale, double y_scale);
  /**
  * 处理片段对象
  * @page page指针对象
  * @x_scale x方向缩放值
  * @y_scale y方向缩放值
  * @return 1：成功 0：失败
  **/
  void ScalePageSegmentsObj(FPDF_PAGE page, double x_scale, double y_scale);
  /**
  * 处理中划线和下划线
  * @page page指针对象
  * @x_scale x方向缩放值
  * @y_scale y方向缩放值
  * @return 1：成功 0：失败
  **/
  void ScalePageAnnotationObj(FPDF_PAGE page, double x_scale, double y_scale);
  /**
   *是否鉴权成功
  **/
  bool is_authentication_ = false;
};
#ifdef __cplusplus
}
#endif
}
#endif