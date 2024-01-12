#ifndef WIREGUARD_FLUTTER_UTILS_H
#define WIREGUARD_FLUTTER_UTILS_H

#include <flutter/encodable_value.h>
#include <windows.h>

#include <sstream>
#include <string>

namespace wireguard_flutter {

const flutter::EncodableValue *ValueOrNull(const flutter::EncodableMap &map, const char *key);

std::string ErrorWithCode(const char *msg, unsigned long error_code);

std::string WideToUtf8(const std::wstring &wstr);

std::wstring Utf8ToWide(const std::string &str);

std::string WideToAnsi(const std::wstring &wstr);

std::wstring AnsiToWide(const std::string &str);

// Pops a message box (useful for debugging native code)
void DebugMessageBox(const char* msg);

}  // namespace wireguard_flutter

#endif
