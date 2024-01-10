#include "service_control.h"

#include <windows.h>

#include <stdexcept>
#include <string>

#include "utils.h"
#include <iostream>

namespace wireguard_flutter
{

  class ServiceControlException : public std::exception
  {
  private:
    std::string message_;
    unsigned long error_code_;

  public:
    explicit ServiceControlException(const std::string &msg) : message_(msg), error_code_(0) {}

    ServiceControlException(const std::string &msg, unsigned long errc) : message_(msg), error_code_(errc) {}

    const char *what() const noexcept override
    {
      std::string s = message_ + " (" + std::to_string(error_code_) + ")";
      return s.c_str();
    }

    unsigned long GetErrorCode() const noexcept
    {
      return error_code_;
    }
  };

  void ServiceControl::CreateAndStart(CreateArgs args)
  {
    std::cout << "opening service manager" << std::endl;
    SC_HANDLE service_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (service_manager == NULL)
    {
      throw ServiceControlException("Failed to open service manager", GetLastError());
    }

    std::cout << "opening service" << std::endl;
    SC_HANDLE service = OpenService(service_manager, &service_name_[0], SC_MANAGER_ALL_ACCESS);
    if (service != NULL)
    {
      std::cout << "deleting service" << std::endl;
      DeleteService(service);
      CloseServiceHandle(service);
    }

    std::cout << "creating service" << std::endl;
    service = CreateService(service_manager,                  // SCM database
                            &service_name_[0],                // name of service
                            &service_name_[0],                // service name to display
                            SERVICE_ALL_ACCESS,               // desired access
                            SERVICE_WIN32_OWN_PROCESS,        // service type
                            SERVICE_DEMAND_START,             // start type
                            SERVICE_ERROR_NORMAL,             // error control type
                            args.executable_and_args.c_str(), // path to service's binary
                            NULL,                             // no load ordering group
                            NULL,                             // no tag identifier
                            NULL, //args.dependencies.c_str(),
                            NULL, // LocalSystem account
                            NULL);
    if (service == NULL)
    {
      CloseServiceHandle(service_manager);
      throw ServiceControlException("Failed to create the service", GetLastError());
    }

    std::cout << "setting sid type" << std::endl;
    auto sid_type = SERVICE_SID_TYPE_UNRESTRICTED;
    if (!ChangeServiceConfig2(service, SERVICE_CONFIG_SERVICE_SID_INFO, &sid_type))
    {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      throw ServiceControlException("Failed to configure servivce SID type", GetLastError());
    }

    std::cout << "settings descp" << std::endl;
    SERVICE_DESCRIPTION description = {&args.description[0]};
    if (!ChangeServiceConfig2(service, SERVICE_CONFIG_DESCRIPTION, &description))
    {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      throw ServiceControlException("Failed to configure service description", GetLastError());
    }

    SERVICE_STATUS_PROCESS ssStatus;
    DWORD dwBytesNeeded;

    std::cout << "checking state" << std::endl;
    if (!QueryServiceStatusEx(
            service,                        // handle to service
            SC_STATUS_PROCESS_INFO,         // information level
            (LPBYTE)&ssStatus,              // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded))                // size needed if buffer is too small
    {
      std::cout << "QueryServiceStatusEx failed (%d)\n" << GetLastError() << std::endl;
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      return;
    }

    std::cout << "current state: " << ssStatus.dwCurrentState << std::endl;
    if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
    {
      std::cout << "Cannot start the service because it is already running\n" << std::endl;
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      return;
    }

    std::cout << "Starting service " << this->service_name_.c_str() << std::endl;
    if (!StartService(service, 0, NULL))
    {
      std::cout << "Failed to start the service: " << this->service_name_.c_str() << " " << GetLastError() << std::endl;
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      throw ServiceControlException("Failed to start the service", GetLastError());
    }

    std::cout << "Service started" << std::endl;

    CloseServiceHandle(service);
    CloseServiceHandle(service_manager);
  }

  void ServiceControl::Stop()
  {
    SC_HANDLE service_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (service_manager == NULL)
    {
      throw ServiceControlException("Failed to open service manager", GetLastError());
    }

    SC_HANDLE service = OpenService(service_manager, &service_name_[0], SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (service == NULL)
    {
      CloseServiceHandle(service_manager);
      return;
    }

    SERVICE_STATUS_PROCESS service_status;
    DWORD service_status_bytes_needed;
    if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&service_status, sizeof(SERVICE_STATUS_PROCESS),
                              &service_status_bytes_needed))
    {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      throw ServiceControlException("Failed to query service status", GetLastError());
    }
    if (service_status.dwCurrentState == SERVICE_STOPPED)
    {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      return;
    }

    DWORD start_time = GetTickCount();
    DWORD wait_for_stop_timeout = 15000;
    DWORD wait_time;
    while (service_status.dwCurrentState == SERVICE_STOP_PENDING)
    {
      // Sometimes dwWaitHint gives unreasonably large values (e.g. 120s), thus just checking every 1s.
      // Original logic:
      // wait_time = service_status.dwWaitHint / 10;
      // if (wait_time < 1000)
      //   wait_time = 1000;
      // else if (wait_time > 10000)
      //   wait_time = 10000;
      wait_time = 1000;

      Sleep(wait_time);

      if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&service_status, sizeof(SERVICE_STATUS_PROCESS),
                                &service_status_bytes_needed))
      {
        CloseServiceHandle(service);
        CloseServiceHandle(service_manager);
        throw ServiceControlException("Failed to query service status when stop pending", GetLastError());
      }

      if (service_status.dwCurrentState == SERVICE_STOPPED)
      {
        CloseServiceHandle(service);
        CloseServiceHandle(service_manager);
        return;
      }

      if (GetTickCount() - start_time > wait_for_stop_timeout)
      {
        CloseServiceHandle(service);
        CloseServiceHandle(service_manager);
        throw ServiceControlException("Disconnect timed out");
      }
    }
    if (!ControlService(service, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&service_status))
    {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      throw ServiceControlException("Stop service command failed", GetLastError());
    }

    while (service_status.dwCurrentState != SERVICE_STOPPED)
    {
      // Sometimes dwWaitHint gives unreasonably large values (e.g. 120s), thus just checking every 1s.
      // Original logic:
      // Sleep(service_status.dwWaitHint);
      Sleep(1000);

      if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&service_status, sizeof(SERVICE_STATUS_PROCESS),
                                &service_status_bytes_needed))
      {
        CloseServiceHandle(service);
        CloseServiceHandle(service_manager);
        throw ServiceControlException("Failed to query service status after issuing stop command", GetLastError());
      }

      if (service_status.dwCurrentState == SERVICE_STOPPED)
      {
        CloseServiceHandle(service);
        CloseServiceHandle(service_manager);
        return;
      }

      if (GetTickCount() - start_time > wait_for_stop_timeout)
      {
        CloseServiceHandle(service);
        CloseServiceHandle(service_manager);
        throw ServiceControlException("Disconnect timed out");
      }
    }

    CloseServiceHandle(service);
    CloseServiceHandle(service_manager);
  }

  void ServiceControl::Disable()
  {
    SC_HANDLE service_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (service_manager == NULL)
    {
      throw ServiceControlException("Failed to open service manager", GetLastError());
    }

    SC_HANDLE service = OpenService(service_manager, &service_name_[0], SERVICE_CHANGE_CONFIG);
    if (service == NULL)
    {
      CloseServiceHandle(service_manager);
      return;
    }

    auto service_start_type = SERVICE_DISABLED;
    if (!ChangeServiceConfig(service, SERVICE_NO_CHANGE, service_start_type, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL))
    {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      throw ServiceControlException("Failed to disable service", GetLastError());
    }
  }

  std::string ServiceControl::GetStatus() {
    std::cout << "opening service manager" << std::endl;
    SC_HANDLE service_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (service_manager == NULL)
    {
      throw ServiceControlException("Failed to open service manager", GetLastError());
    }

    std::cout << "opening service" << std::endl;
    SC_HANDLE service = OpenService(service_manager, &service_name_[0], SC_MANAGER_ALL_ACCESS);
    if (service == NULL)
    {
      CloseServiceHandle(service_manager);
      CloseServiceHandle(service);
      return "disconnected";
    }

    SERVICE_STATUS_PROCESS ssStatus;
    DWORD dwBytesNeeded;

    std::cout << "checking state" << std::endl;
    if (!QueryServiceStatusEx(
            service,                        // handle to service
            SC_STATUS_PROCESS_INFO,         // information level
            (LPBYTE)&ssStatus,              // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded))                // size needed if buffer is too small
    {
      std::cout << "QueryServiceStatusEx failed (%d)\n" << GetLastError() << std::endl;
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      return "denied";
    }

    std::cout << "current state: " << ssStatus.dwCurrentState << std::endl;
    if (ssStatus.dwCurrentState == SERVICE_STOPPED)
    {
      return "disconnected";
    } else if (ssStatus.dwCurrentState == SERVICE_STOP_PENDING) {
      return "disconnecting";
    } else if (ssStatus.dwCurrentState == SERVICE_START_PENDING) {
      return "connecting";
    } else if (ssStatus.dwCurrentState == SERVICE_RUNNING) {
      return "connected";
    } else if (ssStatus.dwCurrentState == SERVICE_CONTINUE_PENDING) {
      return "reconnecting";
    } else if (ssStatus.dwCurrentState == SERVICE_PAUSE_PENDING) {
      return "disconnecting";
    } else if (ssStatus.dwCurrentState == SERVICE_PAUSED) {
      return "disconnected";
    }

    CloseServiceHandle(service);
    CloseServiceHandle(service_manager);

    return "no_connection";
  }

} // namespace wireguard_dart
