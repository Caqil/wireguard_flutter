#include <flutter/encodable_value.h>
#include <windows.h>

#include <sstream>
#include <string>

namespace wireguard_flutter
{

  const flutter::EncodableValue *ValueOrNull(const flutter::EncodableMap &map, const char *key)
  {
    auto it = map.find(flutter::EncodableValue(key));
    if (it == map.end())
    {
      return nullptr;
    }
    return &(it->second);
  }

  std::string ErrorWithCode(const char *msg, unsigned long error_code)
  {
    std::ostringstream builder;
    builder << msg << " (" << error_code << ")";
    return builder.str();
  }

  std::string WideToUtf8(const std::wstring &wstr)
  {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
  }

  std::wstring Utf8ToWide(const std::string &str)
  {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
  }

  std::string WideToAnsi(const std::wstring &wstr)
  {
    int size_needed = WideCharToMultiByte(CP_ACP, 0, &wstr[0], -1, NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
  }

  std::wstring AnsiToWide(const std::string &str)
  {
    int size_needed = MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
  }

  void DebugMessageBox(const char *msg)
  {
    std::string s(msg);
    std::wstring ws = Utf8ToWide(s);
    MessageBox(NULL, &ws[0], L"Debug", MB_OK);
  }

} // namespace wireguard_flutter
