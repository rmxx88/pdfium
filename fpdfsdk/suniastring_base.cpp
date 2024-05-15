#include "suniastring_base.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
namespace sunia {
#ifdef __cplusplus
extern "C" {
#endif
StringBase::StringBase() {

}
StringBase::~StringBase() {

}
std::vector<std::string> StringBase::StringSplit(const std::string& str,
                                               char delimiter) {
  std::vector<std::string> result;
  size_t pos = 0;
  while (true) {
    size_t found = str.find(delimiter, pos);
    if (found == std::string::npos) {
      break;
    }

    result.push_back(str.substr(pos, found - pos));
    pos = found + 1;
  }
  result.push_back(str.substr(pos));
  return result;
}

std::string StringBase::GetPlatformString(FPDF_WIDESTRING wstr) {
  WideString wide_string = WideStringFromFPDFWideString(wstr);
  return std::string(wide_string.ToUTF8().c_str());
}

std::wstring StringBase::GetPlatformWString(FPDF_WIDESTRING wstr) {
  if (!wstr) {
    return std::wstring();
  }

  size_t characters = 0;
  while (wstr[characters]) {
    ++characters;
  }

  std::wstring platform_string;
  platform_string.reserve(characters);
  for (size_t i = 0; i < characters; ++i) {
    const unsigned char* ptr = reinterpret_cast<const unsigned char*>(&wstr[i]);
    platform_string.push_back(ptr[0] + 256 * ptr[1]);
  }
  return platform_string;
}
ScopedFPDFWideString StringBase::GetFPDFWideString(const std::wstring& wstr) {
  size_t length = sizeof(uint16_t) * (wstr.size() + 1);
  ScopedFPDFWideString result(static_cast<FPDF_WCHAR*>(malloc(length)));
  pdfium::span<uint8_t> result_span(reinterpret_cast<uint8_t*>(result.get()),
                                    length);
  size_t i = 0;
  for (wchar_t w : wstr) {
    result_span[i++] = w & 0xff;
    result_span[i++] = (w >> 8) & 0xff;
  }
  result_span[i++] = 0;
  result_span[i] = 0;
  return result;
}
#ifdef __cplusplus
}
#endif
}  // namespace sunia