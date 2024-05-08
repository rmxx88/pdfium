#include "suniafile_base.h"
namespace sunia {
#ifdef __cplusplus
extern "C" {
#endif
FileBase::FileBase() {

}
FileBase::~FileBase() {

}
std::string FileBase::ReadFile(const char* path) {
  std::string buffers;
  FILE* file;
  char* buffer;
  long file_size;
  size_t result;
  // 打开文件以读取
  file = fopen(path, "rb");
  if (file == NULL) {
    perror("Error opening file");
    return buffers;
  }
  // 获取文件大小
  fseek(file, 0, SEEK_END);
  file_size = ftell(file);
  rewind(file);
  // 分配内存用于存储文件内容
  buffer = (char*)malloc(file_size + 1);
  if (buffer == NULL) {
    perror("Error allocating memory");
    fclose(file);
    return buffers;
  }

  // 读取文件内容到缓冲区
  result = fread(buffer, 1, file_size, file);
  if ((int)result != file_size) {
    perror("Error reading file");
    free(buffer);
    fclose(file);
    return buffers;
  }
  // 在缓冲区末尾添加字符串结束符
  buffer[file_size] = '\0';
  // 输出文件内容
  printf("File content: %s\n", buffer);
  buffers = buffer;
  // 释放内存并关闭文件
  free(buffer);
  fclose(file);
  return buffers;
}
bool FileBase::WriteFile(const char* path, char* buffers) {
  bool suc = false;
  FILE* file = fopen(path, "wb");
  if (!file) {
    return buffers;
  }
  size_t bytes_written = fwrite(buffers, sizeof(char), sizeof(buffers), file);
  if (bytes_written != 0) {
    suc = true;
  }
  fclose(file);
  return suc;
}
#ifdef __cplusplus
}
#endif
}  // namespace sunia