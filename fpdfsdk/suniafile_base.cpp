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

  file = fopen(path, "rb");
  if (file == NULL) {
    perror("Error opening file");
    return buffers;
  }

  fseek(file, 0, SEEK_END);
  file_size = ftell(file);
  rewind(file);

  buffer = (char*)malloc(file_size + 1);
  if (buffer == NULL) {
    perror("Error allocating memory");
    fclose(file);
    return buffers;
  }

 
  result = fread(buffer, 1, file_size, file);
  if ((int)result != file_size) {
    perror("Error reading file");
    free(buffer);
    fclose(file);
    return buffers;
  }

  buffer[file_size] = '\0';

  printf("File content: %s\n", buffer);
  buffers = buffer;
  
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
