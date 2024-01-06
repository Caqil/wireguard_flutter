#include <windows.h>

#include <codecvt>
#include <stdexcept>
#include <string>

namespace wireguard_flutter
{

  std::wstring WriteConfigToTempFile(std::string config)
  {
    WCHAR temp_path[MAX_PATH];
    DWORD temp_path_len = GetTempPath(MAX_PATH, temp_path);
    if (temp_path_len > MAX_PATH || temp_path_len == 0)
    {
      throw std::runtime_error("could not get temporary dir: " + GetLastError());
    }

    WCHAR temp_filename[MAX_PATH];
    UINT temp_filename_result = GetTempFileName(temp_path, L"wg_conf", 0, temp_filename);
    wcscat_s(temp_filename, L".conf");
    if (temp_filename_result == 0)
    {
      throw std::runtime_error("could not get temporary file name: " + GetLastError());
    }

    HANDLE temp_file = CreateFile(temp_filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (temp_file == INVALID_HANDLE_VALUE)
    {
      throw std::runtime_error("unable to create temporary file: " + GetLastError());
    }

    DWORD bytes_written;
    if (!WriteFile(temp_file, config.c_str(), static_cast<DWORD>(config.length()), &bytes_written, NULL))
    {
      throw std::runtime_error("could not write temporary config file:" + GetLastError());
    }

    if (!CloseHandle(temp_file))
    {
      throw std::runtime_error("unable to close temporary file:" + GetLastError());
    }
    return temp_filename;
  }

}
